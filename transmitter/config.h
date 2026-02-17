// ============================================
// TRANSMITTER CONFIG
// Change COURT_ID for each button unit (1-8)
// ============================================

#define COURT_ID 1  // <-- CHANGE THIS PER UNIT (1-8)

// Receiver MAC address (get this from get_mac_address utility)
// Replace with your actual receiver MAC
uint8_t RECEIVER_MAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Pin assignments
#define BUTTON_PIN    GPIO_NUM_3   // wake-capable GPIO on ESP32-C3
#define LED_PIN       10           // confirmation LED

// Timing
#define LED_FLASH_MS  300
#define SEND_TIMEOUT_MS 1000
