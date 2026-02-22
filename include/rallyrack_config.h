#pragma once

#include <stdint.h>

#define NUM_COURTS 8

// Some ESP32 core variants (including PlatformIO's QT Py S3 package)
// do not provide D0..D10 aliases. Provide compatibility defaults.
#ifndef D0
#define D0 0
#endif
#ifndef D1
#define D1 1
#endif
#ifndef D2
#define D2 2
#endif
#ifndef D3
#define D3 3
#endif
#ifndef D4
#define D4 4
#endif
#ifndef D5
#define D5 5
#endif
#ifndef D6
#define D6 6
#endif
#ifndef D7
#define D7 7
#endif
#ifndef D10
#define D10 10
#endif

// ============================================
// RECEIVER / RACK CONTROLLER CONFIG
// ============================================

// Reset buttons on the rack (one per court)
// Court 1: A0 (GPIO 17) | Courts 2-8: A2, A3, TX, RX, MOSI, MISO, SCK
const int RESET_PINS[NUM_COURTS] = {17, 7, 8, 6, 5, 35, 37, 36};

// LED pins inside each reset button (-1 = not wired yet)
// Court 1: A1 (GPIO 18)
const int RESET_LED_PINS[NUM_COURTS] = {18, -1, -1, -1, -1, -1, -1, -1};

// Buzzer
#define BUZZER_PIN D10
#define BUZZER_FREQ 2000
#define BUZZER_MS 150

// OLED display (I2C)
// QT Py ESP32-S3 SDA/SCL header pins (same as STEMMA QT)
#define OLED_SDA 41
#define OLED_SCL 40
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_I2C_ADDR 0x3C
#define OLED_UPDATE_MS 500
#define OLED_PAGE_MS 2500

// Debounce
#define DEBOUNCE_MS 200

// ============================================
// TRANSMITTER / BUTTON CONFIG (per unit)
// ============================================

#define COURT_ID 1 // <-- CHANGE THIS PER UNIT (1-8)

// Pin assignments
#define BUTTON_PIN GPIO_NUM_3 // wake-capable GPIO on ESP32-C3
#define LED_PIN 10            // confirmation LED

// Timing
#define LED_FLASH_MS 300
#define SEND_TIMEOUT_MS 1000

// ============================================
// SHARED CONFIG
// ============================================

// Receiver MAC address (get this from get_mac_address utility)
// Replace with your actual receiver MAC
uint8_t RECEIVER_MAC[] = {0xB4, 0x3A, 0x45, 0xB0, 0xD5, 0x14};
