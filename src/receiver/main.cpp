// Pickleball Paddle Rack - Receiver / Controller
// Adafruit QT Py S3 + buzzer + OLED
// Receives court state (occupied/available) from transmitters via ESP-NOW.

#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"

bool courtAvailable[NUM_COURTS] = {false};
bool courtInUse[NUM_COURTS] = {false};
unsigned long availableSinceMs[NUM_COURTS] = {0};
unsigned long inUseSinceMs[NUM_COURTS] = {0};
float avgWaitMs[NUM_COURTS] = {0};
uint32_t waitSamples[NUM_COURTS] = {0};
bool oledReady = false;
unsigned long lastOledUpdate = 0;
int8_t alertCourtId = -1;       // court showing full-screen alert (-1 = none)
unsigned long alertUntilMs = 0; // when to return to normal display
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
    display.setTextSize(2);
    // First line: "Court X"
    char line1[12];
    snprintf(line1, sizeof(line1), "Court %d", alertCourtId);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(line1, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((OLED_WIDTH - w) / 2, 14);
    display.print(line1);
    // Second line: "open!"
    display.getTextBounds("open!", 0, 0, &x1, &y1, &w, &h);
    display.setCursor((OLED_WIDTH - w) / 2, 38);
    display.print("open!");
    display.display();
    return;
  }
  else
  {
    alertCourtId = -1;
  }

  // Normal view: courts 1-4
  unsigned long overallMs = globalAverageWaitMs();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("RallyRack Wait Avg");
  display.setCursor(0, 8);
  display.print("Overall: ");
  display.print(minutesFromMs(overallMs));
  display.print("m");

  for (int i = 0; i < 4; i++)
  {
    unsigned long avgMin = minutesFromMs((unsigned long)(avgWaitMs[i] + 0.5f));
    display.setCursor(0, 20 + (i * 11));

    if (courtInUse[i] && inUseSinceMs[i] > 0)
    {
      unsigned long inUseMin = minutesFromMs(now - inUseSinceMs[i]);
      display.printf("C%d U:%2lum A:%2lum", i + 1, inUseMin, avgMin);
    }
    else if (courtAvailable[i] && availableSinceMs[i] > 0)
    {
      unsigned long nowWaitMin = minutesFromMs(now - availableSinceMs[i]);
      display.printf("C%d N:%2lum A:%2lum", i + 1, nowWaitMin, avgMin);
    }
    else
    {
      display.printf("C%d N: -- A:%2lum", i + 1, avgMin);
    }
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

  if (pkt.occupied)
  {
    if (!courtInUse[i])
    {
      // Transition: available → occupied
      // Record how long the court waited before being claimed
      if (courtAvailable[i] && availableSinceMs[i] > 0)
      {
        unsigned long waitMs = now - availableSinceMs[i];
        waitSamples[i]++;
        avgWaitMs[i] += (waitMs - avgWaitMs[i]) / waitSamples[i];
        Serial.printf("[OCCUPIED] Court %d claimed, wait=%lum avg=%lum\n",
                      pkt.courtId,
                      minutesFromMs(waitMs),
                      minutesFromMs((unsigned long)(avgWaitMs[i] + 0.5f)));
      }
      else
      {
        Serial.printf("[OCCUPIED] Court %d now in use\n", pkt.courtId);
      }
      courtAvailable[i] = false;
      availableSinceMs[i] = 0;
      courtInUse[i] = true;
      inUseSinceMs[i] = now;
      tone(BUZZER_PIN, BUZZER_FREQ, BUZZER_MS);
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
      courtInUse[i] = false;
      inUseSinceMs[i] = 0;
      courtAvailable[i] = true;
      availableSinceMs[i] = now;
      Serial.printf("[AVAILABLE] Court %d now open\n", pkt.courtId);
      tone(BUZZER_PIN, BUZZER_FREQ, BUZZER_MS);
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
  delay(3000); // wait for serial monitor to connect

  // Init buzzer
  pinMode(BUZZER_PIN, OUTPUT);

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
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("RallyRack");
    display.setCursor(0, 12);
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
}

void loop()
{
  updateDisplay();
  delay(20);
}
