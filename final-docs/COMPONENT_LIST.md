# Skylink Drone — Final Component List
**As of: 5 June 2026 | Status: Assembled & Ground-Tested**

---

## Hardware Components

| # | Component | Source | Qty | Notes |
|---|-----------|--------|-----|-------|
| 1 | **Pixhawk 2.4.8 Full Combo Set with GPS** | Evelta | 1 | Includes Pixhawk FC, Neo-M8N GPS+compass, power module, safety switch, buzzer, I2C splitter, PPM encoder, mounting hardware |
| 2 | **F450 Quadcopter Frame** | Robocraze Kit | 1 | 450mm class X-frame with integrated PDB |
| 3 | **A2212 KV1000 Brushless Motors** | Robocraze Kit | 4 | 1000KV, 3S compatible |
| 4 | **30A SimonK ESCs** | Robocraze Kit | 4 | SimonK firmware, BEC capable |
| 5 | **1045 Propellers (2 Pairs)** | Robocraze Kit | 2 pairs | 10-inch 4.5-pitch; CW + CCW |
| 6 | **GenX 11.1V 3S 5200mAh 40C/80C LiPo** | Robokits | 1 | XT60 connector; provides ~14 min hover |
| 7 | **IMAX B6AC 80W Charger/Discharger 1-6S** | Robokits | 1 | Balance charge at 1C (5.2A) |
| 8 | **7Semi EC200U LTE 4G GNSS Module** | Robodo | 1 | Quectel EC200U-CN; USB-C; provides internet connectivity |
| 9 | **ESP32 DevKit v1** | Already owned | 1 | GPIO16 RX2, GPIO17 TX2 → Pixhawk TELEM2 |
| 10 | **XL4015 Buck Converter (5A)** | Already owned | 1 | Steps LiPo (11.1V) down to 5V for ESP32 |
| 11 | **XT60 Male Pigtail** | Robocraze | 1 | Battery power tap / wiring extension |
| 12 | **Heat Shrink Tube (4mm, 2m)** | Robocraze | 1 | Wire insulation on ESC/motor bullet connectors |
| 13 | **I2C Splitter Board** | Evelta (in kit) | 1 | Splits I2C bus to GPS compass + RGB LED + OLED |
| 14 | **External RGB Status LED** | Evelta (in kit) | 1 | I2C address 0x55; mirrors Pixhawk arm/GPS status |
| 15 | **I2C 0.96" OLED Display (SSD1306)** | Evelta (in kit) | 1 | 128x64, shows telemetry; I2C address 0x3C |

---

## Software Components

| # | Component | Description |
|---|-----------|-------------|
| 1 | **ArduCopter V3.6.8 (NuttX)** | Flight controller firmware on Pixhawk 2.4.8 |
| 2 | **Skylink ESP32 Firmware** | PlatformIO (Arduino) project; MAVLink bridge + AsyncWebServer GCS |
| 3 | **Skylink GCS Dashboard** | Single-page web app (HTML/CSS/JS); served from ESP32 LittleFS |
| 4 | **Mission Planner** | Ground-side tool for calibration and parameter configuration |

---

## Key Configuration Reference

| Parameter | Value |
|-----------|-------|
| **Pixhawk TELEM2 → ESP32** | GPIO17(TX) → TELEM2 RX; GPIO16(RX) → TELEM2 TX; GND common |
| **Serial Baud** | 115200 |
| **SERIAL2_PROTOCOL** | 2 (MAVLink 2) |
| **Frame** | F450 Quad-X (FRAME_CLASS=1, FRAME_TYPE=1) |
| **Battery** | 3S 5200mAh; `BATT_MONITOR=4`, safe land at 10.8V |
| **ESP32 IP** | Dynamic DHCP (typically 10.62.43.219 on home network) |
| **Skylink GCS URL** | `http://<ESP32_IP>/` from same Wi-Fi |
| **I2C Bus** | Pixhawk I2C → Splitter → [0x1E GPS, 0x55 RGB LED, 0x3C OLED] |

---

## Status Checklist (as of 5 June 2026)

- [x] Frame assembled, motors and ESCs mounted
- [x] Pixhawk mounted and configured (FRAME_CLASS, FRAME_TYPE, SERIAL2, BATT_MONITOR)
- [x] GPS + compass calibrated
- [x] I2C splitter connected (GPS, RGB LED, OLED all working)
- [x] ESP32 running hardware-mode Skylink firmware
- [x] MAVLink link established (UART2 / TELEM2)
- [x] Battery monitor showing voltage on OLED and GCS
- [x] Web GCS accessible over Wi-Fi
- [ ] Accelerometer full 6-position calibration
- [ ] ESC calibration
- [ ] First outdoor GPS-guided hover test
- [ ] Remote (4G) access configured
- [ ] Authentication added to dashboard
