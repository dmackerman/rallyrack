// Pickleball Court Button - Transmitter
// ESP32-C3 DevKitM-01 + arcade button + LED
// Toggles court occupied/available state on button press.
// Persists state in NVS across reboots.
// Heartbeats current state every HEARTBEAT_SEC seconds.

#include <esp_now.h>
#include <esp_sleep.h>
#include <WiFi.h>
#include <Preferences.h>
#include "config.h"

volatile bool sendDone = false;
volatile bool sendOk = false;
Preferences prefs;

void onSent(const uint8_t *mac, esp_now_send_status_t status)
{
  (void)mac;
  sendOk = (status == ESP_NOW_SEND_SUCCESS);
  sendDone = true;
}

void setLED(uint8_t brightness)
{
  ledcWrite(0, brightness);
}

// 1 long flash = now occupied, 2 quick flashes = now available
void ledStateConfirm(bool occupied)
{
  if (occupied)
  {
    setLED(255);
    delay(600);
    setLED(0);
  }
  else
  {
    for (int i = 0; i < 2; i++)
    {
      setLED(255);
      delay(120);
      setLED(0);
      if (i < 1)
        delay(120);
    }
  }
}

void ledError()
{
  for (int i = 0; i < 5; i++)
  {
    setLED(255);
    delay(80);
    setLED(0);
    delay(80);
  }
}

// Brief dim blink to show heartbeat fired (unobtrusive)
void ledHeartbeat()
{
  setLED(40);
  delay(80);
  setLED(0);
}

bool sendState(bool occupied)
{
  CourtPacket pkt;
  pkt.courtId = COURT_ID;
  pkt.occupied = occupied ? 1 : 0;

  sendDone = false;
  sendOk = false;
  esp_now_send(RECEIVER_MAC, (uint8_t *)&pkt, sizeof(pkt));

  unsigned long start = millis();
  while (!sendDone && (millis() - start < SEND_TIMEOUT_MS))
    delay(10);

  return sendOk;
}

void setup()
{
  ledcSetup(0, 5000, 8);
  ledcAttachPin(LED_PIN, 0);
  setLED(0);

  // Load persisted state
  prefs.begin("court", false);
  bool occupied = prefs.getBool("occupied", false); // default: available

  // Check what woke us up
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  bool isButtonPress = (cause == ESP_SLEEP_WAKEUP_GPIO);
  bool isHeartbeat = (cause == ESP_SLEEP_WAKEUP_TIMER);

  if (isButtonPress)
  {
    // Toggle state immediately and show feedback before sending
    occupied = !occupied;
    prefs.putBool("occupied", occupied);
    setLED(255); // instant on so button press feels responsive
  }

  prefs.end();

  // Init WiFi + ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK)
  {
    ledError();
    goto sleep;
  }

  esp_now_register_send_cb(onSent);

  {
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, RECEIVER_MAC, 6);
    peer.channel = 0;
    peer.encrypt = false;
    esp_now_add_peer(&peer);
  }

  if (sendState(occupied))
  {
    setLED(0);
    if (isHeartbeat)
      ledHeartbeat();
    else
      ledStateConfirm(occupied);
  }
  else
  {
    ledError();
  }

sleep:
  esp_sleep_enable_timer_wakeup((uint64_t)HEARTBEAT_SEC * 1000000ULL);
  esp_deep_sleep_enable_gpio_wakeup(1ULL << BUTTON_PIN, ESP_GPIO_WAKEUP_GPIO_LOW);
  esp_deep_sleep_start();
}

void loop()
{
  // Never reached
}
