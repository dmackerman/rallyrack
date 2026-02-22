# RallyRack

```text
██████╗  █████╗ ██╗     ██╗  ██╗   ██╗██████╗  █████╗  ██████╗██╗  ██╗
██╔══██╗██╔══██╗██║     ██║  ╚██╗ ██╔╝██╔══██╗██╔══██╗██╔════╝██║ ██╔╝
██████╔╝███████║██║     ██║   ╚████╔╝ ██████╔╝███████║██║     █████╔╝
██╔══██╗██╔══██║██║     ██║    ╚██╔╝  ██╔══██╗██╔══██║██║     ██╔═██╗
██║  ██║██║  ██║███████╗███████╗██║   ██║  ██║██║  ██║╚██████╗██║  ██╗
╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚══════╝╚═╝   ╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝
```

Wireless court availability system using ESP32 + ESP-NOW.

## Shopping List (Adafruit)

Full 8-court setup, all from [adafruit.com](https://www.adafruit.com).

### Core Electronics

| Item | Adafruit PID | Qty | Unit | Subtotal |
|---|---|---:|---:|---:|
| Arcade Button with LED - 30mm Translucent Green | [3487](https://www.adafruit.com/product/3487) | 8 | $2.50 | $20.00 |
| Arcade Button Quick-Connect Wires 0.187" (10-pack) | [3835](https://www.adafruit.com/product/3835) | 1 | $4.95 | $4.95 |
| Adafruit QT Py S3 w/ 2MB PSRAM (receiver) | [5700](https://www.adafruit.com/product/5700) | 1 | $12.50 | $12.50 |
| ESP32-C3 DevKitM-01 (transmitters) | [5337](https://www.adafruit.com/product/5337) | 8 | $9.95 | $79.60 |
| Monochrome 1.3" 128x64 OLED - STEMMA QT | [938](https://www.adafruit.com/product/938) | 1 | $19.95 | $19.95 |
| Tactile Button switch 6mm (20-pack, for resets) | [367](https://www.adafruit.com/product/367) | 1 | $2.50 | $2.50 |
| Piezo Buzzer PS1240 | [160](https://www.adafruit.com/product/160) | 1 | $1.50 | $1.50 |
| 5V 2A switching power supply - UL Listed | [276](https://www.adafruit.com/product/276) | 1 | $7.95 | $7.95 |
| 470Ω resistors 1/4W (25-pack) | [2781](https://www.adafruit.com/product/2781) | 1 | $0.75 | $0.75 |
| Premium M/M Jumper Wires 20×6" | [1957](https://www.adafruit.com/product/1957) | 2 | $1.95 | $3.90 |
| **Core subtotal** | | | | **$153.60** |

### Power + Portable Button Parts

| Item | Adafruit PID | Qty | Unit | Subtotal |
|---|---|---:|---:|---:|
| Lithium Ion Polymer Battery 3.7V 350mAh | [2750](https://www.adafruit.com/product/2750) | 8 | $6.95 | $55.60 |
| Adafruit Micro-Lipo Charger (MicroUSB) | [1904](https://www.adafruit.com/product/1904) | 8 | $6.95 | $55.60 |
| **Portable subtotal** | | | | **$111.20** |

### Prototyping

| Item | Adafruit PID | Qty | Unit | Subtotal |
|---|---|---:|---:|---:|
| Full Sized Breadboard 830 Tie Points | [239](https://www.adafruit.com/product/239) | 2 | $5.95 | $11.90 |
| **Prototyping subtotal** | | | | **$11.90** |

### Budget Summary

- **All electronics (core + portable + prototyping):** about **$277**
- You will also need basic tools (soldering iron, solder, wire stripper, multimeter, heat-shrink) which Adafruit does not carry — source from a hardware store or Mouser/DigiKey

## Hardware

### Per Court Button (×8)
- ESP32-C3 DevKitM-01 ([Adafruit 5337](https://www.adafruit.com/product/5337))
- [Adafruit 30mm Translucent LED Arcade Button](https://www.adafruit.com/product/3487) (Green or any color)
  - Built-in dual LEDs with 200Ω resistor
  - Works at 3.3V (2mA, dimmer) or 5V (10mA, brighter)
  - Includes switch (NO) and LED contacts
- LiPo battery ([Adafruit 2750](https://www.adafruit.com/product/2750)) + Micro-Lipo charger ([Adafruit 1904](https://www.adafruit.com/product/1904))
- [0.187" Quick-Connect Wire Pairs](https://www.adafruit.com/product/3835) for easy assembly

### Rack Controller (×1)
- Adafruit QT Py S3 w/ 2MB PSRAM ([Adafruit 5700](https://www.adafruit.com/product/5700))
- 8× momentary push buttons (reset)
- Passive buzzer ([Adafruit 160](https://www.adafruit.com/product/160))
- 1.3" I2C OLED display (SSD1306, 128x64) ([Adafruit 938](https://www.adafruit.com/product/938))
- 5V power supply ([Adafruit 276](https://www.adafruit.com/product/276))

## Wiring

### Transmitter (each Court button)
- **Button switch contacts:** one to BUTTON_PIN (GPIO 3), other to GND
- **Button LED contacts:** positive (+) to LED_PIN (GPIO 10), negative (-) to GND
  - **No external resistor needed** - 200Ω resistor built into button
  - Works directly from ESP32 GPIO pin (3.3V)
  - LED will be slightly dimmer at 3.3V vs 5V, but adequate for visibility

```text
Transmitter (ESP32-C3 + Arcade Button)

                  ESP32-C3 DevKitM-01
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
- Reset buttons → D0..D7 to GND (internal pullups used)
- Buzzer → D10
- OLED (SSD1306 I2C) → SDA/SCL + power

```text
Receiver Rack (QT Py S3 + Reset Buttons + Buzzer + OLED)

                                 +----------------------+
                                 |   QT Py ESP32-S3     |
                                 |                      |
D10 ------------------| BUZZER_PIN           |
D0  ------------------| RESET Court 1        |
D1  ------------------| RESET Court 2        |
D2  ------------------| RESET Court 3        |
D3  ------------------| RESET Court 4        |
D4  ------------------| RESET Court 5        |
D5  ------------------| RESET Court 6        |
D6  ------------------| RESET Court 7        |
D7  ------------------| RESET Court 8        |
GND ------------------| GND                  |
                                 +----------------------+

Reset buttons (one per court):
   D0..D7 --------[button]---- GND
   (uses internal pull-up, so press = LOW)

Buzzer:
   D10 -----------------------> Buzzer signal (+)
   GND -----------------------> Buzzer (-)

OLED (SSD1306 I2C):
   QT Py 3V3 -----------------> OLED VCC
   QT Py GND -----------------> OLED GND
   QT Py SDA -----------------> OLED SDA
   QT Py SCL -----------------> OLED SCL
```

| QT Py S3 Pin | Connects To | Notes |
|---|---|---|
| D10 | Buzzer signal (+) | `BUZZER_PIN` |
| D0  | Reset button Court 1 | To GND when pressed |
| D1  | Reset button Court 2 | To GND when pressed |
| D2  | Reset button Court 3 | To GND when pressed |
| D3  | Reset button Court 4 | To GND when pressed |
| D4  | Reset button Court 5 | To GND when pressed |
| D5  | Reset button Court 6 | To GND when pressed |
| D6  | Reset button Court 7 | To GND when pressed |
| D7  | Reset button Court 8 | To GND when pressed |
| SDA | OLED SDA | I2C data line |
| SCL | OLED SCL | I2C clock line |
| GND | Button return + buzzer (-) | Common ground required |

## Getting Started

### 1) Install tools

1. Install **PlatformIO** (either option below):
   - **VS Code extension:** install `PlatformIO IDE`
   - **CLI:** install `platformio` (`pio`) and verify with `pio --version`
2. Clone this repo and open it as a PlatformIO project root (folder containing `platformio.ini`)
3. PlatformIO will automatically install board packages and libraries on first build/upload

### 1.1) Project targets (PlatformIO environments)

- `receiver` → QT Py S3 rack controller firmware
- `transmitter` → ESP32-C3 court button firmware
- `get_mac_address` → utility to print receiver MAC

Code locations:
- `src/receiver/main.cpp`
- `src/transmitter/main.cpp`
- `src/get_mac_address/main.cpp`
- `include/rallyrack_config.h`
- `receiver/config.h`
- `transmitter/config.h`

### 2) Wire hardware

1. Assemble one receiver rack and up to 8 court transmitters using the wiring tables above
2. Double-check power:
   - ESP32 boards can stay on USB during development

### 2.1) Electronics primer (read this first if you're new)

Before wiring anything, here are the concepts you need:

**What is a breadboard?**
A breadboard is a plastic block with a grid of holes. Metal clips inside connect the holes in rows, so you can connect components without soldering. The key rules:
- The two long rails running down each side (usually marked `+` and `−`, or red/blue) are **power rails** — every hole in the same rail is connected. Use these for 3.3V/5V and GND.
- The short rows in the middle (lettered A–E on one side, F–J on the other) are **tie points** — every hole in the same row (e.g. A1 through E1) is connected across that row.
- The center gap separates the two halves; a chip or ESP32 board straddles the gap so its left and right pins fall into separate rows.

**What are jumper wires?**
Short wires with push-in connectors (Dupont wires). Male-to-male wires plug into breadboard holes. Match wire colors to your connections to avoid confusion: red = power, black = GND, any other color = signal.

**What is GND?**
GND (ground) is the common reference point for all voltages. Every component must share the same GND. If you power the ESP32 via USB and the OLED from the ESP32's 3V3 pin, their GNDs must be connected — otherwise the signals have no reference and nothing works.

**Active-low buttons with pull-ups**
The reset buttons use the ESP32's internal **pull-up resistor**. This means:
- When the button is **not pressed**, the pin reads HIGH (3.3V) through the pull-up.
- When the button **is pressed**, it connects the pin directly to GND, pulling it LOW.
- The firmware detects a press by looking for a LOW reading. You do not need an external resistor.

**I2C — what is it?**
I2C (Inter-Integrated Circuit) is a two-wire communication protocol. It uses:
- **SDA** — Serial Data
- **SCL** — Serial Clock

Only two wires carry data between the QT Py S3 and the OLED display.

---

### 2.2) Breadboard prototyping — step by step

Start with **one transmitter + one receiver** only. Do not wire all 8 courts yet — get one working first.

#### What you need

- 1× ESP32-C3 DevKitM-01 (transmitter)
- 1× QT Py ESP32-S3 (receiver)
- 1× 30mm LED Arcade Button (Adafruit 3487)
- 1× small momentary tactile button (reset, Adafruit 367)
- 1× Piezo buzzer (Adafruit 160)
- 1× 1.3" OLED display (Adafruit 938)
- 2× full-size breadboards (830 tie points)
- Dupont male-to-male jumper wires (assorted colors)
- 2× USB-C cables (data-capable, not charge-only)
- 2× computers or USB hubs to power both boards simultaneously

---

#### Part A: Transmitter breadboard (ESP32-C3 + Arcade Button)

The transmitter detects a button press and flashes an LED.

**Step 1 — Seat the ESP32-C3 on the breadboard**

Place the ESP32-C3 DevKitM-01 across the center gap of your breadboard so:
- The left row of pins falls in columns A–E
- The right row of pins falls in columns F–J
- Leave a few rows of empty space above and below for wires

The USB-C port should face off the end of the board.

**Step 2 — Identify the pins you need**

Look at the ESP32-C3 board. The pin labels are silkscreened on the PCB (tiny text next to each pin). You need:
- `GPIO3` — button signal input
- `GPIO10` — LED output
- Any `GND` pin — there are several, use whichever is closest

> Tip: Adafruit's product page for #5337 has a full pinout diagram — look it up on your phone while you work.

**Step 3 — Wire the button switch (NO contact)**

The arcade button has **four terminals** underneath: two pairs. One pair is the **switch** (NO = Normally Open), the other is the **LED**.

1. Identify the two switch terminals (they are usually labeled or come with a wiring guide in the box). When in doubt, test continuity with a multimeter across each pair — the pair that beeps only when the button is pressed is the switch.
2. Push a **yellow** jumper wire from the `GPIO3` row on the breadboard to one switch terminal.
3. Push a **black** jumper wire from any `GND` pin row on the breadboard to the other switch terminal.

When pressed, `GPIO3` will be pulled to GND → firmware detects LOW → court is marked available.

**Step 4 — Wire the button LED**

1. Identify the two LED terminals on the arcade button (the other pair of contacts).
2. Push a **red** jumper wire from the `GPIO10` row on the breadboard to the LED **anode (+)** terminal.
3. Push a **black** jumper wire from any `GND` pin row on the breadboard to the LED **cathode (−)** terminal.

> The 200Ω resistor is already built into the button body — no external resistor needed.

**Step 5 — Power the transmitter**

Plug the ESP32-C3 into a USB-C port. The onboard power LED should illuminate. You do not need a battery for breadboard testing — USB powers everything.

---

#### Part B: Receiver breadboard (QT Py S3 + Tactile Reset Button + Buzzer + OLED)

**Step 6 — Seat the QT Py S3 on the second breadboard**

The QT Py S3 is a small board. Seat it at one end of the breadboard, straddling the center gap, with the USB-C port facing off the edge.

**Step 7 — Connect the power rails**

The QT Py exposes a `3V3` pin and a `GND` pin. Connect these to the breadboard's power rails so other components can tap into them easily:
1. **Red** wire from QT Py `3V3` pin → `+` rail on breadboard
2. **Black** wire from QT Py `GND` pin → `−` rail on breadboard

From now on you can reach 3.3V power by plugging into the `+` rail, and ground by plugging into the `−` rail.

**Step 8 — Wire the first tactile reset button (Court 1)**

Tactile push buttons have 4 legs arranged in two pairs. The legs on the **same short side** are connected internally — those are your two contacts.

1. Seat the tactile button across the center gap of the breadboard (legs straddle the gap).
2. **Yellow** wire from QT Py `D0` pin → one leg of the button (short-side pair A).
3. **Black** wire from the `−` rail (GND) → the other leg of the button (short-side pair B).

When pressed: `D0` is pulled to GND → firmware detects LOW → Court 1 is reset.

> To add more courts later, repeat this for `D1` through `D7` with additional buttons.

**Step 9 — Wire the buzzer**

The piezo buzzer has two pins, often marked `+` and `−` (or the positive leg is longer, like an LED).

1. **Red** wire from QT Py `D10` → buzzer `+` (signal / positive leg)
2. **Black** wire from `−` rail (GND) → buzzer `−` (negative leg)

**Step 10 — Wire the OLED display**

The OLED display has 4 pins: `VCC`, `GND`, `SDA`, `SCL`. The Adafruit 938 comes with a STEMMA QT connector or standard 0.1" header pins.

1. **Red** wire from `+` rail (3.3V) → OLED `VCC`
2. **Black** wire from `−` rail (GND) → OLED `GND`
3. **Green** wire from QT Py `SDA` → OLED `SDA`
4. **Yellow** wire from QT Py `SCL` → OLED `SCL`

> The QT Py S3 also has a **STEMMA QT** JST connector on the board edge. If your OLED has a STEMMA QT cable, you can plug it straight in — no breadboard wiring needed for the OLED at all.

**Step 11 — Power the receiver**

Plug the QT Py S3 into a USB-C port. Check:
- The OLED lights up (may show garbage until firmware is flashed — that's fine)
- The board's power LED illuminates
- No components feel warm or hot (if anything is hot, immediately unplug and check wiring)

---

#### Part C: Sanity checks before flashing

Before loading any firmware, do a quick visual check:

- [ ] Every GND wire connects back to the `−` rail or a board GND pin
- [ ] No bare wire legs are touching each other (short circuits)
- [ ] OLED VCC goes to 3.3V, **not** 5V — the display is 3.3V logic
- [ ] Buzzer and button have correct polarity (+/−)
- [ ] Both boards are powered via USB

---

#### Common beginner mistakes to avoid

| Mistake | What happens | Fix |
|---|---|---|
| GND not shared between components | Signal pins float; nothing works | Run a black wire between all GND pins and the `−` rail |
| OLED powered from 5V instead of 3.3V | Display may be damaged | Use the `3V3` pin only |
| Wrong button terminals (LED vs switch) | Button presses don't register, or LED is always on | Test with a multimeter in continuity mode |
| Charge-only USB cable | Board doesn't enumerate; firmware upload fails | Use a data-capable cable |
| Jumper wire not fully seated | Intermittent connection | Push firmly until the connector clicks into the breadboard |

### 3) Configure receiver and transmitters

1. Build/upload the MAC utility to the receiver QT Py S3:
   - `pio run -e get_mac_address -t upload`
2. Open Serial Monitor at 115200 baud — the MAC prints on startup (e.g. `AA:BB:CC:DD:EE:FF`)
   - `pio device monitor -b 115200`
3. In `transmitter/config.h`, paste it in as:
   - `uint8_t RECEIVER_MAC[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};`
4. For each transmitter unit, set a unique `COURT_ID` (1 to 8)
5. In `receiver/config.h`, confirm receiver-specific pin assignments
6. In `include/rallyrack_config.h`, confirm shared settings like `NUM_COURTS`

### 4) Flash firmware

1. **Receiver:**
   - Env: `receiver`
   - Command: `pio run -e receiver -t upload`
2. **Transmitters (each unit):**
   - Env: `transmitter`
   - Command: `pio run -e transmitter -t upload`
3. If needed, set an explicit serial port:
   - `pio run -e receiver -t upload --upload-port /dev/cu.usbmodemXXXX`
   - `pio run -e transmitter -t upload --upload-port /dev/cu.usbmodemXXXX`
4. If upload fails, hold **BOOT** while connecting/uploading

### 4.1) USB + programming notes (QT Py S3 / ESP32-C3)

1. QT Py S3 and ESP32-C3 DevKitM-01 both include onboard USB-C
2. Use a **data-capable USB cable** (charge-only cables will power the board but uploads fail)
3. In PlatformIO:
   - Choose env (`receiver`, `transmitter`, or `get_mac_address`)
   - Use `--upload-port` when multiple serial devices are connected
4. If upload stalls at connecting:
   - Hold **BOOT**
   - Run upload command
   - Release **BOOT** when "Connecting..." appears in console
   - Tap **RESET** once if needed
5. Different dev boards may expose either native USB CDC or USB-to-UART, but both should appear as serial devices for PlatformIO

### 5) Verify operation

1. Press a court button transmitter
2. Confirm buzzer chirps and OLED shows that court as available
3. Press that court's reset button on the rack and confirm OLED updates

### 6) Wait-time display + serial debug (MVP)

The receiver tracks per-court wait times and updates a running average when each court is reset.

- OLED header shows overall average wait time (minutes)
- OLED auto-pages every few seconds:
   - Page 1: Courts 1-4
   - Page 2: Courts 5-8
- Per-court OLED format:
   - `N` = live wait now (while court is available)
   - `A` = running average wait for that court

Open Serial Monitor at `115200` to view debug events:

- Court becomes available: `[AVAILABLE] Court 3 now open`
- Duplicate available while already open: `[AVAILABLE] Court 3 already open (now=2m)`
- Court reset with sample update: `[RESET] Court 3 wait=7m avg=5m n=12`
- Reset with no active wait: `[RESET] Court 3 pressed with no active wait`

## How It Works

1. Players finish → press court button
2. Button wakes from deep sleep → sends court ID via ESP-NOW → flashes LED → sleeps
3. Rack buzzer chirps + OLED shows court as available + starts wait timer
4. OLED shows live per-court wait (`N`) and running average (`A`)
5. Next group grabs paddles → presses reset button on rack → OLED updates and average recalculates

## Testing & Development

### Unit Tests (No Hardware)

RallyRack includes 21 unit tests that validate all receiver logic without any hardware:

```bash
# Run all tests
pio test -e test

# Verbose output
pio test -e test -vv
```

Tests cover:
- Time calculations and rounding
- Court state machine (idle → available → in-use → returned)
- Wait-time averaging across multiple cycles
- Display formatting and rendering
- Debouncing logic
- Multi-court independence
- Edge cases and boundary conditions

Tests run instantly (~400ms) and catch regressions before flashing hardware.

### OLED Preview (No Hardware)

You can render a text preview of the OLED screen on your computer using the same fixture state as the unit tests:

```bash
# Render both pages
pio run -e oled_preview -t run

# Render a single page
pio run -e oled_preview -t run -D run_args="--page 1"
```

Example output:

```text
RallyRack Wait Avg
Overall: 3m  P1/2
C1 U: 4m A: 2m
C2 N: 1m A: 3m
C3 N: -- A: 2m
C4 N: 5m A: 4m
```

### Building Without Hardware

```bash
# Build all environments (no upload without hardware)
pio run

# Build specific environments
pio run -e receiver
pio run -e transmitter
pio run -e get_mac_address
```

### Display Output Format

When testing, courts display in one of these states:

- `C1 N: -- A: 2m` — Court **idle**, average wait is 2 minutes
- `C1 N: 5m A: 2m` — Court **available** for 5 minutes now, historically 2-minute average
- `C1 U: 4m A: 2m` — Court **in use** for 4 minutes, historical average 2 minutes

## Setup & Deployment

See [SETUP.md](SETUP.md) for:
- MAC address discovery and configuration
- Per-unit customization (court IDs)
- Flashing all devices
- Troubleshooting
