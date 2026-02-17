// ============================================
// RECEIVER / RACK CONTROLLER CONFIG
// ============================================

#define NUM_COURTS 8

// WS2812B LED strip
#define LED_DATA_PIN   5
#define LEDS_PER_COURT 4
#define NUM_LEDS       (NUM_COURTS * LEDS_PER_COURT)  // 32 total
#define LED_BRIGHTNESS 80  // 0-255, keep low for prototype

// Reset buttons on the rack (one per court)
// These are the GPIO pins for courts 1-8
const int RESET_PINS[NUM_COURTS] = {12, 13, 14, 15, 16, 17, 18, 19};

// Buzzer
#define BUZZER_PIN     21
#define BUZZER_FREQ    2000
#define BUZZER_MS      150

// Debounce
#define DEBOUNCE_MS    200

// LED color (GRB order for WS2812B)
#define COLOR_AVAILABLE 0x00FF00  // green
#define COLOR_OFF       0x000000
