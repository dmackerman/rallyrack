// ============================================
// RECEIVER / RACK CONTROLLER CONFIG
// ============================================

#define NUM_COURTS 8

// Reset buttons on the rack (one per court)
// QT Py S3 pin labels for courts 1-8
const int RESET_PINS[NUM_COURTS] = {D0, D1, D2, D3, D4, D5, D6, D7};

// Buzzer
#define BUZZER_PIN D10
#define BUZZER_FREQ 2000
#define BUZZER_MS 150

// OLED display (I2C)
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_I2C_ADDR 0x3C
#define OLED_UPDATE_MS 500
#define OLED_PAGE_MS 2500

// Debounce
#define DEBOUNCE_MS 200
