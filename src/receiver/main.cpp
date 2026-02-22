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
unsigned long lastPageSwitch = 0;
uint8_t oledPage = 0;
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

  if (now - lastPageSwitch >= OLED_PAGE_MS)
  {
    oledPage = (oledPage + 1) % 2;
    lastPageSwitch = now;
  }

  unsigned long overallMs = globalAverageWaitMs();

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("RallyRack Wait Avg");
  display.setCursor(0, 8);
  display.print("Overall: ");
  display.print(minutesFromMs(overallMs));
  display.print("m");
  display.setCursor(82, 8);
  display.print("P");
  display.print(oledPage + 1);
  display.print("/2");

  int baseCourt = oledPage * 4;
  for (int row = 0; row < 4; row++)
  {
    int court = baseCourt + row;
    unsigned long avgMin = minutesFromMs((unsigned long)(avgWaitMs[court] + 0.5f));

    display.setCursor(0, 20 + (row * 11));

    if (courtInUse[court] && inUseSinceMs[court] > 0)
    {
      // Court is in use - show in-use timer
      unsigned long inUseMs = now - inUseSinceMs[court];
      unsigned long inUseMin = minutesFromMs(inUseMs);
      display.printf("C%d U:%2lum A:%2lum", court + 1, inUseMin, avgMin);
    }
    else if (courtAvailable[court] && availableSinceMs[court] > 0)
    {
      // Court is available - show available timer
      unsigned long nowWaitMs = now - availableSinceMs[court];
      unsigned long nowWaitMin = minutesFromMs(nowWaitMs);
      display.printf("C%d N:%2lum A:%2lum", court + 1, nowWaitMin, avgMin);
    }
    else
    {
      // Court is idle - show average only
      display.printf("C%d N: -- A:%2lum", court + 1, avgMin);
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
