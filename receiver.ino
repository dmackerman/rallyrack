// Pickleball Paddle Rack - Receiver / Controller
// ESP32-S3 + WS2812B LED strip + reset buttons + buzzer
// Listens for court-available signals, lights up slots, resets on button press.

#include <esp_now.h>
#include <WiFi.h>
#include <FastLED.h>
#include "config.h"

CRGB leds[NUM_LEDS];
bool courtAvailable[NUM_COURTS] = {false};
unsigned long lastResetPress[NUM_COURTS] = {0};

// Called when an ESP-NOW packet arrives
void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len < 1) return;

  uint8_t courtId = data[0];
  if (courtId < 1 || courtId > NUM_COURTS) return;

  courtAvailable[courtId - 1] = true;

  // Buzzer chirp
  tone(BUZZER_PIN, BUZZER_FREQ, BUZZER_MS);
}

void updateLEDs() {
  for (int court = 0; court < NUM_COURTS; court++) {
    int startLed = court * LEDS_PER_COURT;
    CRGB color = courtAvailable[court] ? CRGB::Green : CRGB::Black;

    for (int i = 0; i < LEDS_PER_COURT; i++) {
      leds[startLed + i] = color;
    }
  }
  FastLED.show();
}

void checkResetButtons() {
  unsigned long now = millis();

  for (int court = 0; court < NUM_COURTS; court++) {
    if (digitalRead(RESET_PINS[court]) == LOW) {  // pressed (pulled low)
      if (now - lastResetPress[court] > DEBOUNCE_MS) {
        courtAvailable[court] = false;
        lastResetPress[court] = now;
      }
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Init reset button pins
  for (int i = 0; i < NUM_COURTS; i++) {
    pinMode(RESET_PINS[i], INPUT_PULLUP);
  }

  // Init buzzer
  pinMode(BUZZER_PIN, OUTPUT);

  // Init LED strip
  FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(LED_BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  // Init WiFi + ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(onReceive);

  Serial.println("Rack controller ready");
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
}

void loop() {
  checkResetButtons();
  updateLEDs();
  delay(50);
}
