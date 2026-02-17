// Pickleball Court Button - Transmitter
// ESP32-C3 DevKitM-01 + arcade button + LED
// Wakes on button press, sends court ID, sleeps.

#include <esp_now.h>
#include <WiFi.h>
#include "config.h"

volatile bool sendDone = false;
volatile bool sendOk = false;

void onSent(const uint8_t *mac, esp_now_send_status_t status) {
  sendOk = (status == ESP_NOW_SEND_SUCCESS);
  sendDone = true;
}

void flashLED(int times, int ms) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(ms);
    digitalWrite(LED_PIN, LOW);
    if (i < times - 1) delay(ms);
  }
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Init WiFi in station mode (required for ESP-NOW)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    flashLED(5, 100);  // rapid flash = init error
    esp_deep_sleep_start();
  }

  esp_now_register_send_cb(onSent);

  // Register receiver as peer
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, RECEIVER_MAC, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  // Send court ID
  uint8_t courtId = COURT_ID;
  esp_now_send(RECEIVER_MAC, &courtId, sizeof(courtId));

  // Wait for send callback
  unsigned long start = millis();
  while (!sendDone && (millis() - start < SEND_TIMEOUT_MS)) {
    delay(10);
  }

  // Flash confirmation
  if (sendOk) {
    flashLED(1, LED_FLASH_MS);   // one solid flash = success
  } else {
    flashLED(3, 100);            // three quick flashes = fail
  }

  // Go to deep sleep, wake on button press
  esp_deep_sleep_enable_gpio_wakeup(1ULL << BUTTON_PIN, ESP_GPIO_WAKEUP_GPIO_LOW);
  esp_deep_sleep_start();
}

void loop() {
  // Never reached — device sleeps after setup
}
