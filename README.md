# Pickleball Paddle Rack System

Wireless court availability system using ESP32 + ESP-NOW.

## Hardware

### Per Court Button (×8)
- ESP32-C3 SuperMini
- [Adafruit 30mm Translucent LED Arcade Button](https://www.adafruit.com/product/3487) (Green or any color)
  - Built-in dual LEDs with 200Ω resistor
  - Works at 3.3V (2mA, dimmer) or 5V (10mA, brighter)
  - Includes switch (NO) and LED contacts
- LiPo battery + TP4056 charger
- Optional: [0.11" Quick-Connect Wire Pairs](https://www.adafruit.com/product/1152) for easy assembly

### Rack Controller (×1)
- ESP32-S3 dev board
- WS2812B LED strip (32 LEDs = 4 per court)
- 8× momentary push buttons (reset)
- Passive buzzer
- 5V power supply

## Wiring

### Transmitter (each Court button)
- **Button switch contacts:** one to BUTTON_PIN (GPIO 3), other to GND
- **Button LED contacts:** positive (+) to LED_PIN (GPIO 10), negative (-) to GND
  - **No external resistor needed** - 200Ω resistor built into button
  - Works directly from ESP32 GPIO pin (3.3V)
  - LED will be slightly dimmer at 3.3V vs 5V, but adequate for visibility

### Receiver (rack)
- WS2812B data → GPIO 5 (with 330Ω resistor on data line)
- WS2812B power → 5V supply (not from ESP32!)
- Reset buttons → RESET_PINS[] to GND (internal pullups used)
- Buzzer → GPIO 21

## Setup Steps

1. **Get receiver MAC address**
   - Flash `utilities/get_mac_address/` to the receiver ESP32-S3
   - Open Serial Monitor, copy the MAC address
   
2. **Update transmitter config**
   - Edit `transmitter/config.h`
   - Paste MAC as: `uint8_t RECEIVER_MAC[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};`

3. **Flash transmitters**
   - For each court button unit, change `COURT_ID` in `config.h` (1 through 8)
   - Flash `transmitter/transmitter.ino`

4. **Flash receiver**
   - Adjust pin assignments in `receiver/config.h` if needed
   - Flash `receiver/receiver.ino`

## Arduino IDE Setup

1. **Install Arduino IDE 2.x** from [arduino.cc](https://www.arduino.cc/en/software)

2. **Add ESP32 board support:**
   - File → Preferences → Additional Board Manager URLs
   - Add: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Tools → Board → Boards Manager → Search "esp32" → Install "esp32 by Espressif Systems"

3. **Install FastLED library:**
   - Tools → Manage Libraries → Search "FastLED" → Install

4. **Select board:**
   - Transmitters: "ESP32C3 Dev Module"
   - Receiver: "ESP32S3 Dev Module"

5. **Connect ESP32 via USB-C:**
   - If upload fails, hold BOOT button while connecting
   - Select correct COM/Serial port in Tools → Port

## How It Works

1. Players finish → press court button
2. Button wakes from deep sleep → sends court ID via ESP-NOW → flashes LED → sleeps
3. Rack lights up that court's LED segment green + buzzer chirp
4. Next group grabs paddles → presses reset button on rack → LEDs turn off
