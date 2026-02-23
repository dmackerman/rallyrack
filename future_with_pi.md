# RallyRack – Power & Architecture Planning Notes

## Overview

This document summarizes key decisions and open questions from an engineering planning session covering power management, hardware selection, and system architecture for the RallyRack project.

---

## Hardware: Per-Court Unit

- **Microcontroller:** Adafruit QT Py ESP32-C3
- Handles all game logic, button inputs, LED control
- Supports both WiFi and BLE

---

## Power Chain

### The Problem
The Adafruit Micro Lipo Charger outputs raw LiPo voltage (3.7V nominal, 4.2V fully charged). The ESP32-C3 DevKitM's 5V pin expects ~4.5–5V to properly feed its onboard LDO regulator. Connecting the Micro Lipo directly to the 5V pin results in undervoltage and potential brownouts, especially under WiFi load.

### The Solution
A boost converter is required between the Micro Lipo output and the DevKit's 5V pin.

### Recommended Options (Adafruit)

| Option | Part | Notes |
|--------|------|-------|
| Keep existing Micro Lipo charger | [MiniBoost 5V @ 1A - TPS61023](https://www.adafruit.com/product/4654) (~$5) | Boost-only, wire BAT out → MiniBoost IN → DevKit 5V |
| Replace Micro Lipo entirely | [PowerBoost 1000C](https://www.adafruit.com/product/2465) (~$20) | Charger + boost + load-sharing in one board. Cleaner for a finished product. |

**Recommendation:** PowerBoost 1000C for a more integrated, reliable power circuit.

### Honest Gaps
- Current draw under real conditions (WiFi/BLE active, LEDs on, buttons firing) has not been measured.
- Battery sizing is unvalidated. A 2000mAh LiPo *might* get 4 hours — or 90 minutes. Real measurement required before any runtime claims.
- Adafruit breakout boards are prototyping-grade. Thermal behavior over hours of continuous use in an enclosure is untested.
- Productizing will eventually require moving to a more integrated power management solution, not daisy-chained breakout boards.

---

## Deployment Context

- **Venue:** Indoor facility, ~6,000 sq ft
- **Courts:** 8
- **Target runtime:** 4 hours continuous
- **Average game time:** 12–25 minutes

---

## Display: OLED → Deprecate

The OLED display on each unit is considered temporary. The long-term goal is a centralized dashboard, removing the per-unit display entirely and reducing per-unit power draw.

---

## System Architecture

### Communication
- **Protocol:** BLE (preferred over WiFi)
  - Lower power draw — meaningful for 4-hour runtime goal
  - Sufficient range for a 6,000 sq ft indoor facility (~30–60 ft max between units and hub)
  - iOS (CoreBluetooth) and Raspberry Pi both support BLE natively
- **Open validation needed:** 8 simultaneous BLE connections to one receiver, stable over a full 4-hour session, in a facility with ambient RF interference (phones, speakers, etc.)

### Dashboard: Raspberry Pi (Near-Term)
Rather than building an iOS app immediately, the near-term plan is a **Raspberry Pi with a touchscreen** acting as a central dashboard.

**Advantages:**
- Faster path to a working multi-court view
- No iOS development required
- Can receive BLE from all 8 QT Py units and display a court overview
- Can sit at front desk or scorer's table

**Tradeoffs:**
- Adds another device to power, maintain, and transport
- May become redundant if an iOS app is built later

### iOS App (Future State)
An iPad-based app remains the long-term goal. Deferred until the Pi-based dashboard validates the concept.

---

## Open Items

- [ ] Measure actual current draw per unit under realistic game conditions
- [ ] Validate BLE range and stability across 8 simultaneous connections in target venue
- [ ] Define what the Pi dashboard displays (pending OLED screenshot from DMoney)
- [ ] Size battery based on real measurements
- [ ] Evaluate PowerBoost 1000C as Micro Lipo replacement