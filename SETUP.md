# RallyRack Setup Guide

This guide walks you through the initial setup of your RallyRack wireless court availability system.

## Setting Up the Receiver MAC Address

Before you can start using the system, you need to:

1. **Discover the Receiver MAC Address** from your receiver ESP32
2. **Configure All Devices** with this MAC address
3. **Flash Firmware** to each device

### Step 1: Get the Receiver's MAC Address

1. **Connect Receiver to Computer**
   - Plug the Adafruit QT Py S3 (receiver) into your computer via USB

2. **Flash the MAC Address Utility**
   ```bash
   pio run -e get_mac_address -t upload
   ```

3. **Open Serial Monitor**
   ```bash
   pio device monitor -b 115200
   ```

4. **Copy the MAC Address**
   - You'll see output like:
   ```
   Receiver MAC Address: 3A:5B:9C:2E:F1:D4
   ```
   - Write this down or copy it to your clipboard

5. **Close Serial Monitor**
   - Press `Ctrl+C` to exit

### Step 2: Update Configuration Files

All devices must know the receiver's MAC address to send messages to it.

1. **Open [include/rallyrack_config.h](include/rallyrack_config.h)**

2. **Find this line** (around line 53):
   ```cpp
   uint8_t RECEIVER_MAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
   ```

3. **Replace with your MAC**
   - Convert the MAC address format from `AA:BB:CC:DD:EE:FF` to `{0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}`
   - Example: `3A:5B:9C:2E:F1:D4` becomes:
   ```cpp
   uint8_t RECEIVER_MAC[] = {0x3A, 0x5B, 0x9C, 0x2E, 0xF1, 0xD4};
   ```

4. **Save the file**

### Step 3: Configure Court IDs for Transmitters

Each court button (transmitter) needs a unique ID from 1-8.

1. **For each transmitter ESP32-C3:**
   - Open [include/rallyrack_config.h](include/rallyrack_config.h)
   - Find line 47: `#define COURT_ID 1`
   - Change the number (1-8) for each device
   - Save

2. **Alternative (Simpler):**
   - You can do this by editing the file temporarily for each device
   - Flash one transmitter with COURT_ID=1, then change it to 2, flash another, etc.

### Step 4: Flash Firmware to Devices

#### Flash the Receiver

1. **Connect receiver (Adafruit QT Py S3) to your computer**
2. **Run:**
   ```bash
   pio run -e receiver -t upload
   ```
3. **Disconnect receiver**

#### Flash Each Transmitter

For each court button (1-8):

1. **Set COURT_ID in [include/rallyrack_config.h](include/rallyrack_config.h)**
   - Edit the line: `#define COURT_ID X` where X is 1-8

2. **Connect transmitter (ESP32-C3 DevKitM-01) to your computer**

3. **Run:**
   ```bash
   pio run -e transmitter -t upload
   ```

4. **Disconnect transmitter**

5. **Repeat for next court button** (change COURT_ID, flash next device)

### Step 5: Test the System

1. **Power on the Receiver**
   - Connect to the 5V power supply
   - The OLED display should show "RallyRack" startup screen
   - After a few seconds, it will show "Wait time tracker" and be ready

2. **Power on a Transmitter**
   - Connect battery to an ESP32-C3 court button
   - OR connect via USB for testing

3. **Press the Button**
   - The button should light up once (success flash)
   - The receiver OLED should show that court as available
   - You should hear a buzzer chirp on the receiver

4. **Press the Reset Button**
   - On the receiver, there's a reset button for court 1 (GPIO D0 on QT Py S3)
   - Pressing it marks the court as no longer available
   - It calculates wait time statistics

### Troubleshooting

#### "Button doesn't light up"
- Check battery connection (3.7V LiPo should show LED)
- Verify LED_PIN (GPIO 10) wiring to ground

#### "Receiver doesn't show court as available"
- Check RECEIVER_MAC is correctly configured
- Verify both devices are powered on
- Try pressing the button again (may take a moment)
- Check serial output: `pio device monitor -b 115200`

#### "ESP-NOW not working"
- Both devices need to be in WIFI_STA mode (not AP mode)
- They must be on the same RF channel (configured as 0 by default)
- Range is typically 100-250m line-of-sight

#### "OLED display doesn't show anything"
- Check I2C address (0x3C) matches your display
- Check SDA (GPIO 4) and SCL (GPIO 5) on QT Py S3
- Verify display is powered (should see backlight)

### Environment Variables for Batch Flashing

To simplify flashing multiple transmitters, you can use a script:

```bash
#!/bin/bash
for court_id in {1..8}; do
  echo "Flashing Court $court_id..."
  # Temporarily edit config
  sed -i "" "s/#define COURT_ID.*/#define COURT_ID $court_id/" include/rallyrack_config.h
  pio run -e transmitter -t upload
done
```

## Configuration Reference

All settings are in [include/rallyrack_config.h](include/rallyrack_config.h):

| Setting | Default | Notes |
|---------|---------|-------|
| `NUM_COURTS` | 8 | Number of court buttons |
| `COURT_ID` | 1 | Transmitter court ID (1-8) |
| `RECEIVER_MAC` | placeholder | Set to your receiver's MAC address |
| `DEBOUNCE_MS` | 200 | Reset button debounce (milliseconds) |
| `LED_FLASH_MS` | 300 | Confirmation LED flash duration |
| `SEND_TIMEOUT_MS` | 1000 | Max wait for ESP-NOW send confirmation |
| `BUZZER_FREQ` | 2000 | Buzzer frequency (Hz) |
| `BUZZER_MS` | 150 | Buzzer duration (milliseconds) |
| `OLED_UPDATE_MS` | 500 | OLED refresh rate |
| `OLED_PAGE_MS` | 2500 | Page switch interval for 8-court display |

## Building and Testing Without Hardware

You can still compile the code to catch errors:

```bash
# Build all environments
pio run

# Build specific environment
pio run -e receiver
pio run -e transmitter
pio run -e get_mac_address
```

If hardware isn't connected, you'll get a build success but no upload. That's expected.
