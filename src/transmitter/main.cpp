// Pickleball Court Button - Transmitter
// ESP32-C3 DevKitM-01 + arcade button + LED
// Toggles court occupied/available state on button press.
// Persists state in NVS across reboots.
// When available: stays awake, pulses LED, polls button.
// When occupied:  LED solid at 100%, deep sleeps with heartbeat + GPIO wakeup.

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

bool initEspNow()
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK)
    return false;

  esp_now_register_send_cb(onSent);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, RECEIVER_MAC, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
  return true;
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

// Stay awake, pulse LED, poll button. Returns when button pressed.
void availableLoop()
{
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  unsigned long lastHeartbeat = millis();
  unsigned long lastPress = 0;

  while (true)
  {
    // Triangle wave pulse: 0→255→0 over ~2 seconds
    uint16_t phase = (millis() / 4) % 510;
    uint8_t brightness = (phase > 255) ? (510 - phase) : phase;
    setLED(brightness);

    // Heartbeat re-broadcast while waiting
    if (millis() - lastHeartbeat >= (uint32_t)HEARTBEAT_SEC * 1000)
    {
      sendState(false);
      lastHeartbeat = millis();
    }

    // Poll button with debounce
    if (digitalRead(BUTTON_PIN) == LOW && millis() - lastPress > DEBOUNCE_MS)
    {
      lastPress = millis();
      setLED(255); // instant feedback
      return;      // caller will handle toggle + send
    }

    delay(10);
  }
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

  if (isButtonPress)
  {
    // Woke from sleep via button — only happens when occupied, so toggle to available
    occupied = false;
    prefs.putBool("occupied", occupied);
  }

  prefs.end();

  if (!initEspNow())
  {
    ledError();
    goto sleep;
  }

  if (occupied)
  {
    // Court in use: LED solid, broadcast, sleep
    setLED(255);
    sendState(true);
    // Stay lit briefly so user sees confirmation, then sleep
    delay(500);
    goto sleep;
  }
  else
  {
    // Court available: broadcast initial state, then enter pulse loop
    sendState(false);

    // Pulse loop blocks until button pressed
    availableLoop();

    // Button was pressed — toggle to occupied
    prefs.begin("court", false);
    prefs.putBool("occupied", true);
    prefs.end();

    sendState(true);
    // LED already at 255 from availableLoop, hold briefly
    delay(500);
    goto sleep;
  }

sleep:
  setLED(0);
  // When occupied: wake on heartbeat timer OR button press (to toggle back to available)
  // When available: we never reach sleep — we're in the pulse loop
  esp_sleep_enable_timer_wakeup((uint64_t)HEARTBEAT_SEC * 1000000ULL);
  esp_deep_sleep_enable_gpio_wakeup(1ULL << BUTTON_PIN, ESP_GPIO_WAKEUP_GPIO_LOW);
  esp_deep_sleep_start();
}

void loop()
{
  // Never reached
}
