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

```text
Transmitter (ESP32-C3 + Arcade Button)

                  ESP32-C3 SuperMini
               +---------------------+
GPIO 3 ---| BUTTON_PIN          |
GPIO10 ---| LED_PIN             |
GND ------| GND                 |
               +---------------------+
                   |        |      |
                   |        |      +------------------------------+
                   |        +-------------+                       |
                   |                      |                       |
                   v                      v                       v
         Arcade Button Switch      Arcade LED (+)         Arcade LED (-)
            [ NO contact ]            [ anode ]             [ cathode ]
                   |                      |                       |
                   +----------------------+-----------------------+
                                                      |
                                                    GND
```

| ESP32-C3 Pin | Connects To | Notes |
|---|---|---|
| GPIO 3  | Arcade switch terminal 1 | `BUTTON_PIN` |
| GND     | Arcade switch terminal 2 | Active-low with pullup |
| GPIO 10 | Arcade LED (+) | `LED_PIN` |
| GND     | Arcade LED (-) | Built-in 200Ω resistor in button |

### Receiver (rack)
- WS2812B data → GPIO 5 (with 330Ω resistor on data line)
- WS2812B power → 5V supply (not from ESP32!)
- Reset buttons → RESET_PINS[] to GND (internal pullups used)
- Buzzer → GPIO 21

```text
Receiver Rack (ESP32-S3 + LEDs + Reset Buttons + Buzzer)

                                 +----------------------+
                                 |      ESP32-S3        |
                                 |                      |
GPIO 5 ---------------| LED_DATA             |
GPIO21 ---------------| BUZZER_PIN           |
GPIO12 ---------------| RESET Court 1        |
GPIO13 ---------------| RESET Court 2        |
GPIO14 ---------------| RESET Court 3        |
GPIO15 ---------------| RESET Court 4        |
GPIO16 ---------------| RESET Court 5        |
GPIO17 ---------------| RESET Court 6        |
GPIO18 ---------------| RESET Court 7        |
GPIO19 ---------------| RESET Court 8        |
GND ------------------| GND                  |
                                 +----------------------+

WS2812B Strip:
   ESP32 GPIO5 --[330Ω]--> DIN
   5V supply  -----------> +5V
   5V supply GND --------> GND
   ESP32 GND ------------> GND  (common ground required)

Reset buttons (one per court):
   GPIO12..19 ----[button]---- GND
   (uses internal pull-up, so press = LOW)

Buzzer:
   GPIO21 --------------------> Buzzer signal (+)
   GND -----------------------> Buzzer (-)
```

| ESP32-S3 Pin | Connects To | Notes |
|---|---|---|
| GPIO 5  | WS2812B DIN (through 330Ω) | `LED_DATA_PIN` |
| GPIO 21 | Buzzer signal (+) | `BUZZER_PIN` |
| GPIO 12 | Reset button Court 1 | To GND when pressed |
| GPIO 13 | Reset button Court 2 | To GND when pressed |
| GPIO 14 | Reset button Court 3 | To GND when pressed |
| GPIO 15 | Reset button Court 4 | To GND when pressed |
| GPIO 16 | Reset button Court 5 | To GND when pressed |
| GPIO 17 | Reset button Court 6 | To GND when pressed |
| GPIO 18 | Reset button Court 7 | To GND when pressed |
| GPIO 19 | Reset button Court 8 | To GND when pressed |
| GND     | LED strip GND + button return + buzzer (-) | Common ground required |

## Getting Started

### 1) Install tools

1. Install **Arduino IDE 2.x** from [arduino.cc](https://www.arduino.cc/en/software)
2. Add **ESP32 board support**:
   - Arduino IDE → Settings/Preferences → Additional Boards Manager URLs
   - Add: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Go to Boards Manager, search `esp32`, and install **esp32 by Espressif Systems**
3. Install **FastLED** from Library Manager

### 2) Wire hardware

1. Assemble one receiver rack and up to 8 court transmitters using the wiring tables above
2. Double-check power:
   - WS2812B strip must use a 5V supply
   - ESP32 boards can stay on USB during development

### 2.1) Breadboard prototyping (no-solder first pass)

Use this to validate code and radio links before permanent wiring.

1. Start with **one transmitter** + **one receiver** only
2. Use a breadboard and Dupont jumper wires
3. Prototype transmitter wiring:
   - ESP32-C3 `GPIO3` → momentary button leg A
   - ESP32-C3 `GND` → button leg B
   - ESP32-C3 `GPIO10` → LED anode (+)
   - ESP32-C3 `GND` → LED cathode (-)
4. Prototype receiver wiring:
   - ESP32-S3 `GPIO5` → WS2812B DIN (through ~330Ω resistor)
   - 5V supply `+` → WS2812B `+5V`
   - 5V supply `-` → WS2812B `GND`
   - ESP32-S3 `GND` → same ground rail (common ground)
   - ESP32-S3 `GPIO12` → reset button leg A, leg B → `GND`
   - ESP32-S3 `GPIO21` → buzzer `+`, buzzer `-` → `GND`
5. Keep ESP32 boards on USB power while prototyping; power LED strip from external 5V
6. Flash and test with `COURT_ID=1` first, then scale to additional courts

Prototype tips:
- Breadboard rail labels can be inconsistent; verify continuity with a multimeter
- If LEDs behave randomly, check that all grounds are tied together
- Keep WS2812B data wire short during initial testing

### 3) Configure receiver and transmitters

1. Flash `get_mac_address.ino` to the receiver ESP32-S3
2. Open Serial Monitor and copy the receiver MAC address
3. In `transmitter/config.h`, set:
   - `uint8_t RECEIVER_MAC[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};`
4. For each transmitter unit, set a unique `COURT_ID` (1 to 8)
5. In `receiver/config.h`, confirm pin assignments and `NUM_COURTS`

### 4) Flash firmware

1. **Receiver:**
   - Board: `ESP32S3 Dev Module`
   - Sketch: `receiver.ino`
2. **Transmitters (each unit):**
   - Board: `ESP32C3 Dev Module`
   - Sketch: `transmitter.ino`
3. Select the correct serial port before each upload
4. If upload fails, hold **BOOT** while connecting/uploading

### 4.1) USB + programming notes (ESP32-S3 / ESP32-C3)

1. Most ESP32-S3 and ESP32-C3 dev boards include an onboard USB port (usually USB-C, sometimes micro-USB)
2. Use a **data-capable USB cable** (charge-only cables will power the board but uploads fail)
3. In Arduino IDE:
   - Select board (`ESP32S3 Dev Module` or `ESP32C3 Dev Module`)
   - Select the serial port under Tools → Port
4. If upload stalls at connecting:
   - Hold **BOOT**
   - Click Upload
   - Release **BOOT** when "Connecting..." appears
   - Tap **RESET** once if needed
5. Different dev boards may expose either native USB CDC or USB-to-UART, but both should appear as a serial port in Arduino IDE

### 5) Verify operation

1. Press a court button transmitter
2. Confirm rack LED segment for that court turns green and buzzer chirps
3. Press that court's reset button on the rack and confirm LEDs turn off

## How It Works

1. Players finish → press court button
2. Button wakes from deep sleep → sends court ID via ESP-NOW → flashes LED → sleeps
3. Rack lights up that court's LED segment green + buzzer chirp
4. Next group grabs paddles → presses reset button on rack → LEDs turn off
