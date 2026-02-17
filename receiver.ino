// Pickleball Paddle Rack - Receiver / Controller
// Adafruit QT Py S3 + reset buttons + buzzer + OLED
// Listens for court-available signals, updates OLED, resets on button press.

#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
bool courtAvailable[NUM_COURTS] = {false};
unsigned long lastResetPress[NUM_COURTS] = {0};
unsigned long availableSinceMs[NUM_COURTS] = {0};
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
    unsigned long nowWaitMs = 0;
    if (courtAvailable[court] && availableSinceMs[court] > 0)
    {
      nowWaitMs = now - availableSinceMs[court];
    }

    unsigned long nowWaitMin = minutesFromMs(nowWaitMs);
    unsigned long avgMin = minutesFromMs((unsigned long)(avgWaitMs[court] + 0.5f));

    display.setCursor(0, 20 + (row * 11));
    if (courtAvailable[court])
    {
      display.printf("C%d N:%2lum A:%2lum", court + 1, nowWaitMin, avgMin);
    }
    else
    {
      display.printf("C%d N: -- A:%2lum", court + 1, avgMin);
    }
  }

  display.display();
}

// Called when an ESP-NOW packet arrives
void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
  if (len < 1)
    return;

  uint8_t courtId = data[0];
  if (courtId < 1 || courtId > NUM_COURTS)
    return;

  int courtIndex = courtId - 1;
  if (!courtAvailable[courtIndex])
  {
    courtAvailable[courtIndex] = true;
    availableSinceMs[courtIndex] = millis();
    Serial.printf("[AVAILABLE] Court %d now open\n", courtId);
  }
  else
  {
    unsigned long nowWaitMs = millis() - availableSinceMs[courtIndex];
    Serial.printf("[AVAILABLE] Court %d already open (now=%lum)\n", courtId, minutesFromMs(nowWaitMs));
  }

  // Buzzer chirp
  tone(BUZZER_PIN, BUZZER_FREQ, BUZZER_MS);
}

void checkResetButtons()
{
  unsigned long now = millis();

  for (int court = 0; court < NUM_COURTS; court++)
  {
    if (digitalRead(RESET_PINS[court]) == LOW)
    { // pressed (pulled low)
      if (now - lastResetPress[court] > DEBOUNCE_MS)
      {
        if (courtAvailable[court] && availableSinceMs[court] > 0)
        {
          unsigned long waitMs = now - availableSinceMs[court];
          waitSamples[court]++;
          avgWaitMs[court] += (waitMs - avgWaitMs[court]) / waitSamples[court];

          unsigned long waitMin = minutesFromMs(waitMs);
          unsigned long avgMin = minutesFromMs((unsigned long)(avgWaitMs[court] + 0.5f));
          Serial.printf("[RESET] Court %d wait=%lum avg=%lum n=%lu\n",
                        court + 1,
                        waitMin,
                        avgMin,
                        (unsigned long)waitSamples[court]);
        }
        else
        {
          Serial.printf("[RESET] Court %d pressed with no active wait\n", court + 1);
        }

        courtAvailable[court] = false;
        availableSinceMs[court] = 0;
        lastResetPress[court] = now;
      }
    }
  }
}

void setup()
{
  Serial.begin(115200);

  // Init reset button pins
  for (int i = 0; i < NUM_COURTS; i++)
  {
    pinMode(RESET_PINS[i], INPUT_PULLUP);
  }

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

  // Init OLED
  oledReady = display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR);
  if (oledReady)
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("RallyRack");
    display.setCursor(0, 12);
    display.print("Wait time tracker");
    display.display();
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
  checkResetButtons();
  updateDisplay();
  delay(50);
}
