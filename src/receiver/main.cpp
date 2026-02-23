// Pickleball Paddle Rack - Receiver / Controller
// Adafruit QT Py S3 + OLED
// Receives court state (occupied/available) from transmitters via ESP-NOW.

#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include <math.h>
#include <Fonts/FreeMonoBold9pt7b.h>

bool courtAvailable[NUM_COURTS] = {true, true, true, true, true, true, true, true};
bool courtInUse[NUM_COURTS] = {false};
unsigned long availableSinceMs[NUM_COURTS] = {0};
unsigned long inUseSinceMs[NUM_COURTS] = {0};
float avgWaitMs[NUM_COURTS] = {0};
uint32_t waitSamples[NUM_COURTS] = {0};
unsigned long lastHeardMs[NUM_COURTS] = {0}; // last packet time per court (fault detection)
bool oledReady = false;
unsigned long lastOledUpdate = 0;
int8_t alertCourtId = -1;                // court showing full-screen alert (-1 = none)
unsigned long alertUntilMs = 0;          // when to return to normal display
volatile int8_t gameStartedCourtId = -1; // triggers game-started animation in loop()
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

unsigned long globalAverageWaitMs()
{
  uint64_t weightedSum = 0;
  uint32_t sampleCount = 0;

  for (int i = 0; i < NUM_COURTS; i++)
  {
    weightedSum += (uint64_t)avgWaitMs[i] * waitSamples[i];
    sampleCount += waitSamples[i];
  }

  if (sampleCount == 0)
    return 0;

  return weightedSum / sampleCount;
}

unsigned long minutesFromMs(unsigned long valueMs)
{
  return (valueMs + 30000UL) / 60000UL;
}

void fmtMMSS(char *buf, size_t sz, unsigned long ms)
{
  unsigned long totalSec = ms / 1000;
  unsigned long m = totalSec / 60;
  unsigned long s = totalSec % 60;
  if (m > 99)
    m = 99; // cap at 99:59
  snprintf(buf, sz, "%02lu:%02lu", m, s);
}

void animateGameStarted(uint8_t courtNum)
{
  display.setFont(NULL); // ensure default font throughout animation
  char courtLine[12];
  snprintf(courtLine, sizeof(courtLine), "Court %d", courtNum);

  const int FRAME_MS = 40;
  const int PHASE1 = 19; // ball-bounce frames
  const int TOTAL = 37;  // total frames (~1.5 s)

  for (int f = 0; f < TOTAL; f++)
  {
    int16_t bx, by;
    uint16_t tw, th;

    if (f < PHASE1)
    {
      // Phase 1: bouncing ball + text slides in from top
      display.invertDisplay(false);
      display.clearDisplay();

      float bt = (float)f / (PHASE1 - 1); // 0 → 1
      int ballX = 6 + (int)(bt * 116);
      float bouncePhase = bt * 3.0f * 3.14159f; // 3 arcs
      float damping = 1.0f - bt * 0.55f;
      int ballY = 56 - (int)(fabsf(sinf(bouncePhase)) * 30.0f * damping);

      // Filled ball with tiny black holes — pickleball look
      display.fillCircle(ballX, ballY, 4, SSD1306_WHITE);
      display.drawPixel(ballX - 1, ballY - 1, SSD1306_BLACK);
      display.drawPixel(ballX + 1, ballY - 1, SSD1306_BLACK);
      display.drawPixel(ballX, ballY + 1, SSD1306_BLACK);

      // "Court X" slides down from above the screen
      int slideY = (f >= 9) ? 2 : (-16 + f * 2);
      display.setTextSize(2);
      display.getTextBounds(courtLine, 0, 0, &bx, &by, &tw, &th);
      display.setCursor((OLED_WIDTH - tw) / 2, slideY);
      display.print(courtLine);

      // "game started!" fades in halfway through
      if (f >= 10)
      {
        display.setTextSize(1);
        display.getTextBounds("game started!", 0, 0, &bx, &by, &tw, &th);
        display.setCursor((OLED_WIDTH - tw) / 2, 26);
        display.print("game started!");
      }
    }
    else
    {
      // Phase 2: static text + double border + 3 invert flashes
      int p2f = f - PHASE1;
      bool invert = (p2f < 6) && (p2f % 2 == 0);
      display.invertDisplay(invert);
      display.clearDisplay();

      display.setTextSize(2);
      display.getTextBounds(courtLine, 0, 0, &bx, &by, &tw, &th);
      display.setCursor((OLED_WIDTH - tw) / 2, 10);
      display.print(courtLine);

      display.setTextSize(1);
      display.getTextBounds("game started!", 0, 0, &bx, &by, &tw, &th);
      display.setCursor((OLED_WIDTH - tw) / 2, 36);
      display.print("game started!");

      // Double border for a stadium feel
      display.drawRect(0, 0, OLED_WIDTH, OLED_HEIGHT, SSD1306_WHITE);
      display.drawRect(2, 2, OLED_WIDTH - 4, OLED_HEIGHT - 4, SSD1306_WHITE);
    }

    display.display();
    delay(FRAME_MS);
  }

  display.invertDisplay(false);
  display.setFont(NULL);
}

void updateDisplay()
{
  if (!oledReady)
    return;

  unsigned long now = millis();
  if (now - lastOledUpdate < OLED_UPDATE_MS)
    return;

  lastOledUpdate = now;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Full-screen alert: "Court X open!"
  if (alertCourtId >= 0 && now < alertUntilMs)
  {
    int16_t x1, y1;
    uint16_t w, h;
    char line1[12];
    snprintf(line1, sizeof(line1), "Court %d", alertCourtId);
    // "Court X" — large, centered, baseline at y=26
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextSize(2);
    display.getTextBounds(line1, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((OLED_WIDTH - w) / 2 - x1, 26);
    display.print(line1);
    // "open!" — smaller, centered, baseline at y=54
    display.setTextSize(1);
    display.getTextBounds("open!", 0, 0, &x1, &y1, &w, &h);
    display.setCursor((OLED_WIDTH - w) / 2 - x1, 54);
    display.print("open!");
    display.setFont(NULL);
    display.display();
    return;
  }
  else
  {
    alertCourtId = -1;
  }

  // Normal view: courts 1-4
  // Column x positions (px): # @ 0, Status @ 18, Now @ 78, Avg @ 108
  unsigned long overallMs = globalAverageWaitMs();
  display.setTextSize(1);

  // Row 1: title (bold via double-print) + overall avg right-aligned
  display.setCursor(0, 0);
  display.print("RallyRack");
  display.setCursor(1, 0);
  display.print("RallyRack");
  {
    char ovBuf[10];
    snprintf(ovBuf, sizeof(ovBuf), "Avg:%lum", minutesFromMs(overallMs));
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(ovBuf, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(OLED_WIDTH - w, 0);
    display.print(ovBuf);
  }

  // Row 2: column headers aligned to data columns
  display.setCursor(0, 10);
  display.print("#");
  display.setCursor(18, 10);
  display.print("Status");
  display.setCursor(78, 10);
  display.print("Now");
  display.setCursor(108, 10);
  display.print("Avg");
  display.drawFastHLine(0, 19, OLED_WIDTH, SSD1306_WHITE);

  // Court rows
  for (int i = 0; i < 4; i++)
  {
    int rowY = 22 + (i * 10);
    unsigned long avgMin = minutesFromMs((unsigned long)(avgWaitMs[i] + 0.5f));

    char numStr[4];
    char statusStr[8];
    char nowStr[7]; // MM:SS + null
    char avgStr[6]; // "99m" + null

    snprintf(numStr, sizeof(numStr), "%d", i + 1);
    snprintf(avgStr, sizeof(avgStr), "%2lum", avgMin);

    if (courtInUse[i] && inUseSinceMs[i] > 0)
    {
      bool fault = (lastHeardMs[i] > 0) &&
                   (now - lastHeardMs[i] > FAULT_TIMEOUT_MS);
      if (fault)
      {
        snprintf(statusStr, sizeof(statusStr), "Fault");
        snprintf(nowStr, sizeof(nowStr), "  ??");
      }
      else
      {
        snprintf(statusStr, sizeof(statusStr), "Started");
        fmtMMSS(nowStr, sizeof(nowStr), now - inUseSinceMs[i]);
      }
    }
    else if (courtAvailable[i])
    {
      snprintf(statusStr, sizeof(statusStr), "Open");
      snprintf(nowStr, sizeof(nowStr), "  --");
    }
    else
    {
      snprintf(statusStr, sizeof(statusStr), "---");
      snprintf(nowStr, sizeof(nowStr), " --");
    }

    display.setCursor(0, rowY);
    display.print(numStr);
    display.setCursor(18, rowY);
    display.print(statusStr);
    display.setCursor(78, rowY);
    display.print(nowStr);
    display.setCursor(108, rowY);
    display.print(avgStr);
  }

  display.display();
}

// Called when an ESP-NOW packet arrives
void onReceive(const uint8_t *mac, const uint8_t *data, int len)
{
  (void)mac;

  if (len < (int)sizeof(CourtPacket))
    return;

  CourtPacket pkt;
  memcpy(&pkt, data, sizeof(pkt));

  if (pkt.courtId < 1 || pkt.courtId > NUM_COURTS)
    return;

  int i = pkt.courtId - 1;
  unsigned long now = millis();
  lastHeardMs[i] = now; // stamp on every packet — used for fault detection

  if (pkt.occupied)
  {
    if (!courtInUse[i])
    {
      // Transition: available → occupied
      Serial.printf("[OCCUPIED] Court %d now in use\n", pkt.courtId);
      courtAvailable[i] = false;
      availableSinceMs[i] = 0;
      courtInUse[i] = true;
      inUseSinceMs[i] = now;
      gameStartedCourtId = (int8_t)pkt.courtId;
    }
    else
    {
      Serial.printf("[HEARTBEAT] Court %d still in use\n", pkt.courtId);
    }
  }
  else
  {
    if (!courtAvailable[i])
    {
      // Transition: occupied → available
      // Record how long the game lasted and update rolling average
      if (inUseSinceMs[i] > 0)
      {
        unsigned long gameMs = now - inUseSinceMs[i];
        waitSamples[i]++;
        avgWaitMs[i] += (gameMs - avgWaitMs[i]) / waitSamples[i];
        Serial.printf("[AVAILABLE] Court %d open after %lum, avg game=%lum\n",
                      pkt.courtId,
                      minutesFromMs(gameMs),
                      minutesFromMs((unsigned long)(avgWaitMs[i] + 0.5f)));
      }
      else
      {
        Serial.printf("[AVAILABLE] Court %d now open\n", pkt.courtId);
      }
      courtInUse[i] = false;
      inUseSinceMs[i] = 0;
      courtAvailable[i] = true;
      availableSinceMs[i] = now;
      // Trigger full-screen alert
      alertCourtId = pkt.courtId;
      alertUntilMs = now + 5000;
    }
    else
    {
      Serial.printf("[HEARTBEAT] Court %d still available\n", pkt.courtId);
    }
  }
}

void setup()
{
  Serial.begin(115200);

  // Init WiFi + ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(onReceive);

  // I2C scan
  Wire.begin(OLED_SDA, OLED_SCL);
  Serial.println("Scanning I2C bus...");
  for (uint8_t addr = 1; addr < 127; addr++)
  {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0)
    {
      Serial.printf("  Found device at 0x%02X\n", addr);
    }
  }

  // Init OLED
  oledReady = display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR);
  if (oledReady)
  {
    Serial.println("OLED init OK");
    display.ssd1306_command(SSD1306_DISPLAYON);
    display.dim(false);
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    // "RallyRack" in FreeMonoBold9pt7b, centered, baseline y=22
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextSize(1);
    {
      int16_t x1, y1;
      uint16_t w, h;
      display.getTextBounds("RallyRack", 0, 0, &x1, &y1, &w, &h);
      display.setCursor((OLED_WIDTH - w) / 2 - x1, 22);
    }
    display.print("RallyRack");
    display.setFont(NULL);
    display.setTextSize(1);
    // "Wait time tracker" default font, centered below
    {
      int16_t x1, y1;
      uint16_t w, h;
      display.getTextBounds("Wait time tracker", 0, 0, &x1, &y1, &w, &h);
      display.setCursor((OLED_WIDTH - w) / 2, 38);
    }
    display.print("Wait time tracker");
    display.display();
    Serial.println("OLED splash drawn");
  }
  else
  {
    Serial.println("OLED init failed");
  }

  Serial.println("Rack controller ready");
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());

  // Seed open timestamps so "Now" shows time-since-boot for default-open courts
  unsigned long bootMs = millis();
  for (int i = 0; i < NUM_COURTS; i++)
    availableSinceMs[i] = bootMs;
}

void loop()
{
  if (gameStartedCourtId >= 0)
  {
    uint8_t courtId = (uint8_t)gameStartedCourtId;
    gameStartedCourtId = -1;
    if (oledReady)
      animateGameStarted(courtId);
    lastOledUpdate = 0; // force display refresh after animation
  }
  updateDisplay();
  delay(20);
}
