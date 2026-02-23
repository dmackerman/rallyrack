# RallyRack — Project Context for Claude

## What This Is

Pickleball court availability tracker. Arcade buttons (transmitters) sit at each court; players press to mark a court in-use or free. A central OLED display (receiver) shows all court states and average game durations in real time.

---

## Hardware

### Receiver
- **Board:** Adafruit QT Py ESP32-S3 (`adafruit_qtpy_esp32s3_n4r2`)
  - 4MB flash, 2MB PSRAM — NOT the 8MB `nopsram` variant
- **Display:** 128×64 monochrome OLED (SSD1306), address `0x3D` (NOT the common `0x3C`)
- **I2C:** STEMMA QT connector → GPIO 41 (SDA), GPIO 40 (SCL)
  - Must call `Wire.begin(41, 40)` before `display.begin()`
- **Port:** `/dev/cu.usbmodem1101`
- **Upload:** Sometimes needs BOOT+RESET to enter bootloader. Close the serial monitor before uploading — the port will be busy otherwise.

### Transmitter (one per court)
- **Board:** ESP32-C3 DevKitM-1 (`esp32-c3-devkitm-1`)
- **Button:** GPIO `GPIO_NUM_3` (wake-capable, active LOW with internal pullup)
- **LED:** GPIO 10, LEDC channel 0 at 5kHz 8-bit PWM
- **Port:** `/dev/cu.usbserial-110`

---

## Running PlatformIO

PlatformIO is not on `$PATH`. Always use the full path:

```sh
~/.platformio/penv/bin/pio run --target upload --environment receiver
~/.platformio/penv/bin/pio run --target upload --environment transmitter
~/.platformio/penv/bin/pio device monitor --environment receiver
```

Default build environment is `receiver` (set in `platformio.ini`).

---

## Architecture

### Protocol
ESP-NOW (no WiFi router needed). Single shared struct in `include/rallyrack_config.h`:

```cpp
struct CourtPacket {
  uint8_t courtId;  // 1-based (1–8)
  uint8_t occupied; // 1 = in use, 0 = available
};
```

Receiver MAC (hardcoded): `{0xB4, 0x3A, 0x45, 0xB0, 0xD5, 0x14}`

### Transmitter State Machine
1. Boot → load `occupied` from NVS (`Preferences("court")`, key `"occupied"`, default `false`)
2. Check wakeup cause:
   - `ESP_SLEEP_WAKEUP_GPIO` = button woke us from occupied sleep → toggle to available
3. If **available**: `sendState(false)` → enter `availableLoop()` (stays awake, pulses LED triangle wave, heartbeats every 30s, polls button)
   - Button press → toggle to occupied → `sendState(true)` → flash LED → sleep
4. If **occupied**: `sendState(true)` → LED solid 255 → 500ms → deep sleep
   - Sleep wakeups: timer (heartbeat) + GPIO (button press to free court)

### Receiver State
All 8 courts default to `available=true` at boot. `availableSinceMs[]` seeded to `millis()` at end of `setup()` so "Now" column starts counting immediately.

State transitions driven by `onReceive()` (ESP-NOW callback):
- `occupied=1` + `!courtInUse[i]` → available→occupied transition; sets `gameStartedCourtId` to trigger animation
- `occupied=0` + `!courtAvailable[i]` → occupied→available transition; records game duration into rolling average; triggers 5-second "Court X open!" alert
- Heartbeats (same state, no change) → logged only

**Average metric** = rolling average of **game duration** (how long a court was in use before being freed). Updated on occupied→available transition using Welford's online formula:
```cpp
avgWaitMs[i] += (gameMs - avgWaitMs[i]) / waitSamples[i];
```

---

## Display Layout

### Splash (boot)
- "RallyRack" — `FreeMonoBold9pt7b`, centered, baseline y=22
- "Wait time tracker" — default 6×8 font, centered, y=38

### Home Screen (normal view)
```
RallyRack (bold)          Avg:Xm
#  Status      Now    Avg
──────────────────────────────
#1 Open         --     0m
#2 Started       3m    5m
#3 Open          7m    5m
#4 Open          1m    0m
```
- Title: `FreeMonoBold9pt7b`, baseline y=12
- Headers + data: default 6×8 font
- Column x offsets: `#`=0, `Status`=18, `Now`=78, `Avg`=108
- Separator line at y=19 (after font switch to header row at y=16)
- Court rows: y=22, step 10px → rows at 22, 32, 42, 52

Status values: `"Open"`, `"Started"`, `"---"`

### Alert Screen (court freed, shown 5 seconds)
- "Court X" — `FreeMonoBold9pt7b` size 2×, centered, baseline y=26
- "open!" — `FreeMonoBold9pt7b` size 1×, centered, baseline y=54

### Game Started Animation (~1.5s, 37 frames × 40ms)
- **Phase 1 (frames 0–18):** Bouncing pickleball (filled circle + 3 black pixels as holes) travels left→right with 3 damped arcs. "Court X" slides in from above. "game started!" appears at frame 10.
- **Phase 2 (frames 19–36):** Static "Court X / game started!" with double border. 3 invert flashes (every other frame for first 6 frames of phase 2).
- Uses default font only (`setFont(NULL)` at entry and exit).

**Important:** Custom GFX fonts use **baseline** cursor (not top-left). Always call `getTextBounds()` and offset the `setCursor()` x by `-x1` when centering. After any custom font block, call `display.setFont(NULL)` to restore default.

---

## Key Config (`include/rallyrack_config.h`)

| Define | Value | Notes |
|---|---|---|
| `NUM_COURTS` | 8 | Array size; only courts 1–4 shown on display |
| `OLED_SDA` | 41 | STEMMA QT pin |
| `OLED_SCL` | 40 | STEMMA QT pin |
| `OLED_I2C_ADDR` | 0x3D | Found via I2C scanner — not the typical 0x3C |
| `COURT_ID` | 1 | **Change per transmitter unit (1–8)** |
| `BUTTON_PIN` | GPIO_NUM_3 | Wake-capable on ESP32-C3 |
| `LED_PIN` | 10 | LEDC channel 0 |
| `HEARTBEAT_SEC` | 30 | Re-broadcast interval while occupied |

---

## Libraries

| Library | Version | Used by |
|---|---|---|
| Adafruit SSD1306 | 2.5.16 | Receiver |
| Adafruit GFX | 1.12.4 | Receiver |
| `<Fonts/FreeMonoBold9pt7b.h>` | (bundled with GFX) | Receiver |
| WiFi, esp_now | (ESP-IDF bundled) | Both |
| Preferences | (ESP-IDF bundled) | Transmitter |
| esp_sleep | (ESP-IDF bundled) | Transmitter |

---

## Known Issues / Gotchas

- **Port busy on upload:** Close the serial monitor first. The monitor holds exclusive lock on the serial port.
- **Port name can change:** After entering/exiting bootloader mode the port may change from `/dev/cu.usbmodem101` → `/dev/cu.usbmodem1101`. Check `ls /dev/cu.*` and update `platformio.ini` if upload fails.
- **LEDC channels on ESP32-S3:** Only channels 0–7 exist (not 15). Receiver doesn't use LEDC; transmitter uses channel 0.
- **Custom font baseline:** GFX custom fonts set cursor at the text *baseline*, not the top-left. Text will render above the y coordinate you set. Use `getTextBounds()` to measure and adjust.
- **`volatile` on shared ISR variables:** `gameStartedCourtId` is set from the ESP-NOW receive callback (runs on a different task) and read from `loop()`. It's declared `volatile int8_t`.
- **No buzzer:** A PS1240 passive piezo was wired and coded, then removed entirely. Do not re-add it — LEDC channel conflicts with the LED on the receiver.

---

## File Map

```
include/
  rallyrack_config.h     — shared CourtPacket struct + all #defines
receiver/
  config.h               — thin wrapper: #include "../include/rallyrack_config.h"
transmitter/
  config.h               — thin wrapper: #include "../include/rallyrack_config.h"
src/
  receiver/main.cpp      — OLED display, ESP-NOW listener, animations
  transmitter/main.cpp   — toggle state machine, NVS, deep sleep, LED PWM
  get_mac_address/main.cpp — utility: prints receiver MAC to serial
  oled_preview/main.cpp  — desktop preview (native build)
platformio.ini
```

---

## Deploying Multiple Transmitters

Each transmitter unit needs a unique `COURT_ID`. Before flashing a new unit:
1. Edit `include/rallyrack_config.h`: set `#define COURT_ID <N>` (1–8)
2. Upload transmitter environment
3. Reset `COURT_ID` to 1 (or leave it — the next unit will need a different number)

The receiver handles all 8 court IDs automatically; only courts 1–4 are shown on the 64px display.
