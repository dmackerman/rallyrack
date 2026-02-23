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
| 5V 2A switching power supply - UL Listed | [276](https://www.adafruit.com/product/276) | 1 | $7.95 | $7.95 |
| 470Ω resistors 1/4W (25-pack) | [2781](https://www.adafruit.com/product/2781) | 1 | $0.75 | $0.75 |
| Premium M/M Jumper Wires 20×6" | [1957](https://www.adafruit.com/product/1957) | 2 | $1.95 | $3.90 |
| **Core subtotal** | | | | **$149.15** |

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

- **All electronics (core + portable + prototyping):** about **$272**
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
- 1.3" I2C OLED display (SSD1306, 128x64) via STEMMA QT ([Adafruit 938](https://www.adafruit.com/product/938))
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
- OLED (SSD1306 I2C) → STEMMA QT connector (GPIO 41 SDA, GPIO 40 SCL)
- No reset buttons or buzzer — courts are controlled entirely by the transmitter buttons

```text
Receiver Rack (QT Py S3 + OLED)

                                 +----------------------+
                                 |   QT Py ESP32-S3     |
                                 |                      |
                                 |  STEMMA QT port  ----+----> OLED (SSD1306)
                                 |  (GPIO 41 SDA,       |
                                 |   GPIO 40 SCL)       |
                                 |                      |
                                 +----------------------+

OLED (1.3" SSD1306, I2C address 0x3D):
   QT Py STEMMA QT --------> OLED STEMMA QT cable (plug-and-play)
     OR
   QT Py 3V3 -----------------> OLED VCC
   QT Py GND -----------------> OLED GND
   GPIO 41 (SDA) -------------> OLED SDA
   GPIO 40 (SCL) -------------> OLED SCL
```

| QT Py S3 Pin | Connects To | Notes |
|---|---|---|
| STEMMA QT | OLED STEMMA QT cable | Easiest — plug-and-play |
| GPIO 41 | OLED SDA | If wiring manually |
| GPIO 40 | OLED SCL | If wiring manually |
| 3V3 | OLED VCC | If wiring manually |
| GND | OLED GND | If wiring manually |

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
The transmitter arcade button uses the ESP32-C3's internal **pull-up resistor**. This means:
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
- 1× 1.3" OLED display (Adafruit 938)
- 1× STEMMA QT cable (usually included with Adafruit 938)
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

#### Part B: Receiver breadboard (QT Py S3 + OLED)

**Step 6 — Seat the QT Py S3 on the second breadboard**

The QT Py S3 is a small board. Seat it at one end of the breadboard, straddling the center gap, with the USB-C port facing off the edge.

**Step 7 — Wire the OLED display**

The Adafruit 938 OLED has a **STEMMA QT** JST connector, and so does the QT Py S3. This is the easiest possible connection:

1. Plug a STEMMA QT cable from the QT Py S3's STEMMA QT port directly into the OLED.

That's it. No breadboard wiring needed. Power, ground, SDA, and SCL are all handled by the cable.

> If you need to wire manually instead: connect GPIO 41 → SDA, GPIO 40 → SCL, 3V3 → VCC, GND → GND.

> The OLED I2C address for the Adafruit 938 is **0x3D** (not the common 0x3C).

**Step 8 — Power the receiver**

Plug the QT Py S3 into a USB-C port. Check:
- The OLED lights up (may show garbage until firmware is flashed — that's fine)
- The board's power LED illuminates
- No components feel warm or hot (if anything is hot, immediately unplug and check wiring)

---

#### Part C: Sanity checks before flashing

Before loading any firmware, do a quick visual check:

- [ ] STEMMA QT cable is fully clicked into both connectors, or manual wires are correctly pinned
- [ ] OLED VCC goes to 3.3V, **not** 5V — the display is 3.3V logic
- [ ] No bare wire legs are touching each other (short circuits)
- [ ] Both boards are powered via USB

---

#### Common beginner mistakes to avoid

| Mistake | What happens | Fix |
|---|---|---|
| GND not shared between components | Signal pins float; nothing works | Run a black wire between all GND pins and the `−` rail |
| OLED powered from 5V instead of 3.3V | Display may be damaged | Use the `3V3` pin only |
| STEMMA QT cable not fully clicked in | OLED stays blank | Press connectors firmly until they click |
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

1. Press a court button transmitter — the LED flashes briefly and the board goes into deep sleep
2. Confirm OLED shows that court as **Started** with a running MM:SS timer
3. Press the same court button again to free the court
4. Confirm OLED shows that court as **Open** and the average game duration updates

### 6) Display layout

The receiver tracks per-court game durations and updates a rolling average after each game ends.

- Header: **RallyRack** (bold) + global `Avg:Xm` in the top-right corner
- OLED auto-pages every 2.5 seconds:
   - Page 1: Courts 1–4
   - Page 2: Courts 5–8
- Column headers: `#  Status  Now  Avg`
- Per-court status values:
   - `Open` — court is free; `Now` column shows `--`
   - `Started` — game in progress; `Now` shows a live `MM:SS` timer
   - `Fault` — no signal received for >45 seconds; `Now` shows `??`
   - `---` — court has never been heard from since boot

Example page 1:
```text
RallyRack                Avg:4m
#  Status      Now    Avg
------------------------------
1 Started 04:00 2m
2 Open -- 3m
3 --- -- 0m
4 Open -- 4m
```

When a game ends, a 5-second full-screen alert shows `Court X / open!`. When a game starts, a ~1.5-second animation plays (bouncing ball + slide-in text).

Open Serial Monitor at `115200` to view state-change events.

## How It Works

1. **Game starts** → player presses the court's arcade button
2. Transmitter wakes from deep sleep → sends `occupied=1` via ESP-NOW → LED solid → goes back to deep sleep
3. Receiver OLED plays a short animation, then shows the court as **Started** with a live MM:SS game timer
4. **Game ends** → player presses the button again
5. Transmitter wakes (GPIO interrupt) → toggles state → sends `occupied=0` via ESP-NOW → flashes LED → stays awake (available loop)
6. Receiver records game duration into a rolling average, shows a 5-second **"Court X open!"** alert, then returns to the main screen with the court listed as **Open**
7. While a court is occupied, the transmitter sends heartbeat packets every 15 seconds so the receiver knows it's still alive
8. If no packet is received for 45 seconds, the court shows **Fault** until contact is restored

## Testing & Development

### Unit Tests (No Hardware)

RallyRack includes 24 unit tests that validate all receiver logic without any hardware:

```bash
# Run all tests
pio test -e test

# Verbose output
pio test -e test -vv
```

Tests cover:
- Time calculations, rounding, and MM:SS formatting
- Court state machine (idle → available → started → open)
- Game duration averaging (Welford's online algorithm)
- Display text formatting (`Started MM:SS`, `Open --`, `Fault ??`)
- Fault detection and automatic recovery
- Debounce logic
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
RallyRack                Avg:4m
#  Status      Now    Avg
------------------------------
1 Started 04:00 2m
2 Open -- 3m
3 --- -- 2m
4 Open -- 4m
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

- `1 --- -- 0m` — Court **idle** (never heard from), no average yet
- `1 Open -- 2m` — Court **open** (free to play), historical average 2 minutes; `Now` always shows `--` for open courts
- `1 Started 04:00 2m` — Court **in use** for 4 minutes, historical average 2 minutes
- `1 Fault ?? 2m` — Court **faulted** (no signal >45s), historical average 2 minutes

## Setup & Deployment

See [SETUP.md](SETUP.md) for:
- MAC address discovery and configuration
- Per-unit customization (court IDs)
- Flashing all devices
- Troubleshooting
