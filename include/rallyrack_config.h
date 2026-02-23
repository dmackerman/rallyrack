#pragma once

#include <stdint.h>

#define NUM_COURTS 8

// ============================================
// SHARED PACKET PROTOCOL
// ============================================

struct CourtPacket
{
  uint8_t courtId;  // 1-based court number
  uint8_t occupied; // 1 = in use, 0 = available
};

// ============================================
// RECEIVER / RACK CONTROLLER CONFIG
// ============================================

// OLED display (I2C)
// QT Py ESP32-S3 SDA/SCL header pins (same as STEMMA QT)
#define OLED_SDA 41
#define OLED_SCL 40
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_I2C_ADDR 0x3D
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
#define HEARTBEAT_SEC 15       // re-broadcast state every N seconds
#define FAULT_TIMEOUT_MS 45000 // 3× heartbeat — court goes Fault if no packet received

// ============================================
// SHARED CONFIG
// ============================================

// Receiver MAC address
uint8_t RECEIVER_MAC[] = {0xB4, 0x3A, 0x45, 0xB0, 0xD5, 0x14};
