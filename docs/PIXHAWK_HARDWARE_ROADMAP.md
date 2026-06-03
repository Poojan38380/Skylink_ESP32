# Pixhawk 2.4.8 Hardware Roadmap — Skylink on a Real Drone

**Audience:** You (operator), future AI agents, and thesis reviewers.  
**Goal:** Move from **SITL + browser GCS** to a **real quadcopter** built around a [Pixhawk 2.4.8–class flight controller](https://evelta.com/pixhawk-2-4-8-px4-autopilot-32-bit-flight-controller-for-rc-drone-uav/) (e.g. ReadytoSky FC-110 kit) while keeping the same Skylink ESP32 + web dashboard stack.

**Software already in repo:** MAVLink bridge, map-first GCS, guided moves, click-to-fly — see [simulation/upgrade/IMPLEMENTATION_HANDOFF.md](simulation/upgrade/IMPLEMENTATION_HANDOFF.md).

---

## 1. What you are building

```
┌─────────────────────────────────────────────────────────────────────────┐
│  Operator phone/laptop  ──WiFi──►  ESP32 (Skylink)  ──UART──►  Pixhawk │
│       Browser GCS                      Web server              ArduPilot │
│       (same UI as SITL)              MAVLink GCS                  │    │
│                                                                     ▼    │
│                                                              Motors / GPS │
│                                                              Power module │
└─────────────────────────────────────────────────────────────────────────┘

Optional (recommended for setup & safety):
  Mission Planner ──USB──► Pixhawk   (calibration, params, logs — NOT shared with ESP32 port)
  RC transmitter ──SBUS/PPM──► Pixhawk   (manual override, arm/disarm backup)
```

**Skylink’s role:** WiFi companion computer. It is **not** the autopilot. The Pixhawk runs **ArduPilot (ArduCopter)** and executes flight; the ESP32 sends MAVLink commands and relays telemetry to the browser.

---

## 2. Kit reference (Pixhawk 2.4.8 set)

Typical [Pixhawk 2.4.8 kit](https://evelta.com/pixhawk-2-4-8-px4-autopilot-32-bit-flight-controller-for-rc-drone-uav/) includes:
*(See detailed research reports: [Pixhawk 2.4.8 Full Combo Set](file:///d:/btp_skylink/Skylink/docs/components/kits/pixhawk_248_evelta_combo.md) and [ReadytoSky Pixhawk 2.4.8 Flight Controller Set](file:///d:/btp_skylink/Skylink/docs/components/kits/readytosky_fc110_pixhawk_set.md))*

| Item | Role |
|------|------|
| Pixhawk 2.4.8 FC | STM32F427, runs ArduPilot when flashed |
| Buzzer + safety switch | Arm safety, audible alerts |
| Power module | Battery voltage/current to FC; often 5 V on telemetry port |
| PPM encoder / I2C splitter | Peripherals |
| Shock mount, SD card | Logging, mechanical isolation |

**You still need (not always in FC-only listing):**
*(See detailed frame/propulsion research report: [F450 Quadcopter Frame Kit](file:///d:/btp_skylink/Skylink/docs/components/kits/f450_quadcopter_frame_kit.md))*

| Component | Why |
|-----------|-----|
| **GPS + compass** (e.g. Neo-M8N) | Position, map, GUIDED, geofence, RTL |
| **Quad frame** | Airframe |
| **4× ESC + motors + props** | Propulsion |
| **LiPo battery** (cell count per frame/motors) | Power |
| **RC radio** (TX + RX) | Manual control, mandatory for safe testing |
| **USB cable** (micro USB on 2.4.8) | Mission Planner, firmware, calibration |

### Firmware choice: ArduPilot, not PX4 (for Skylink)

The Evelta listing mentions **PX4** compatibility. **Skylink is written for ArduPilot MAVLink** (modes like `GUIDED`, `LOITER`, `RTL`, `MAV_CMD_NAV_TAKEOFF`, copter `custom_mode` values).

| Firmware | Skylink GCS |
|----------|-------------|
| **ArduCopter 4.x** (recommended) | ✅ Supported |
| PX4 | ❌ Different mode/command model — would need a separate port |

**Action:** Flash **ArduPilot Copter** stable for your board (Mission Planner → Install Firmware, or [ArduPilot docs](https://ardupilot.org/copter/docs/common-autopilots.html)). Use the same workflow you already use in SITL, but on silicon.

---

## 3. SITL vs hardware — what changes

| Topic | SITL (today) | Pixhawk (target) |
|-------|----------------|------------------|
| ESP32 ↔ autopilot | TCP to PC port **5763** | **UART2** GPIO **16/17**, **115200** 8N1 |
| Build flag | `-D SITL_MODE` in `platformio.ini` | **Remove** `SITL_MODE` |
| `flightController` instance | `FlightController()` + WiFiClient | `FlightController(Serial2)` |
| SITL host from browser IP | Yes (`setSITLHost`) | N/A |
| Browser UI | Same `data/gcs.js` | Same; `simulation: false` in heartbeat |
| Banner | “SIMULATION MODE” | Should show **LIVE AIRCRAFT** (Phase H-UI — not fully styled yet) |
| Mission Planner | TCP **5762** to SITL | **USB** to Pixhawk only |
| Risk | Virtual crash | **Real injury** — props off on bench |

**Code paths (already implemented):**

```41:43:src/flight_controller.cpp
    logger.info("Initializing FlightController in [HARDWARE MODE] via UART2");
    fcSerial.begin(115200, SERIAL_8N1, 16, 17);
```

```827:831:src/flight_controller.cpp
#ifdef SITL_MODE
FlightController flightController;
#else
FlightController flightController(Serial2);
#endif
```

No JavaScript changes are required for basic MAVLink bridging; GCS commands are identical.

---

## 4. Recommended roadmap (phases)

Complete **GCS Phase 6** (hold-to-arm, rate limiter, docs) in SITL first if possible — then hardware. Hardware phases:

### Phase H0 — Prerequisites (software & process)

**Duration:** 1–2 days  

- [ ] SITL regression passes: arm → takeoff → `MOVE_BODY` → `GOTO_LATLON` → LOITER → RTL → disarm ([upgrade/TESTING_STRATEGY.md](simulation/upgrade/TESTING_STRATEGY.md)).
- [ ] Read [simulation/upgrade/IMPLEMENTATION_HANDOFF.md](simulation/upgrade/IMPLEMENTATION_HANDOFF.md) §6 (WebSocket queue, heartbeat filter).
- [ ] Create **two PlatformIO environments** (recommended):

```ini
; platformio.ini — example pattern
[env:esp32dev_sitl]
extends = env:esp32dev
build_flags =
  ${env:esp32dev.build_flags}
  -D SITL_MODE

[env:esp32dev_hw]
extends = env:esp32dev
build_flags =
  ${env:esp32dev.build_flags}
  ; no SITL_MODE
```

Flash simulation: `pio run -e esp32dev_sitl --target upload`  
Flash hardware: `pio run -e esp32dev_hw --target upload`

- [ ] Order / verify full BOM (frame, ESCs, GPS, RC, battery, props).

**Exit:** You can switch builds without editing code by hand.

---

### Phase H1 — Bench assembly (no props)

**Duration:** 2–5 days  

**Safety:** **Propellers removed.** Battery disconnected or only FC USB powered where noted.

- [ ] Mount Pixhawk on frame with damper; mount GPS on mast (clear sky view).
- [ ] Wire motors/ESCs to MAIN OUT per ArduPilot docs for your frame (X quad).
- [ ] Connect power module: battery → PM → FC; verify polarity.
- [ ] Connect **RC receiver** to RC IN (SBUS/PPM as supported).
- [ ] Connect **safety switch** and **buzzer** per kit instructions.
- [ ] **Do not** power ESCs from full battery until motor directions are verified in MP.

**Exit:** FC boots, MP connects over USB, gyro/accel live in HUD.

---

### Phase H2 — ESP32 ↔ Pixhawk UART link

**Duration:** 1 day  

#### Wiring (TELEM2 = SERIAL2 on most Pixhawk 2.4.x)

Cross-connect TX/RX. Common **6-pin TELEM** order (verify your silkscreen / vendor diagram):

```
    ESP32 DevKit                    Pixhawk TELEM2 (SERIAL2)
   ┌──────────────┐                ┌──────────────┐
   │ GPIO16 (RX2) │◄───────────────│ TX           │
   │ GPIO17 (TX2) │───────────────►│ RX           │
   │ GND          │────────────────│ GND          │
   │ 5V (VIN)     │◄── optional ───│ VCC (5V)     │  ← only if PM/FC provides suitable 5V
   └──────────────┘                └──────────────┘
```

| Rule | Detail |
|------|--------|
| Logic | 3.3 V TTL on both sides — **no** 5 V on ESP32 GPIO |
| Ground | **Mandatory** common GND |
| One master per UART | **Do not** connect Mission Planner to TELEM2 while ESP32 is wired — use **USB** for MP |
| Power | Prefer powering ESP32 via USB during bench; if using TELEM 5V, use **VIN/5V** pin, not 3V3 |

#### ArduPilot parameters (Mission Planner → Config → Full Parameter List)

Set for **SERIAL2** (TELEM2):

| Parameter | Value | Meaning |
|-----------|-------|---------|
| `SERIAL2_PROTOCOL` | **2** | MAVLink 2 |
| `SERIAL2_BAUD` | **115** | 115200 baud |
| `SR2_POSITION` | **2** or higher | GPS position rate (Hz) |
| `SR2_EXTRA1` | **10** | Attitude |
| `SR2_EXT_STAT` | **2** | Extra status / battery |

Also set (copter safety — adjust per frame):

| Parameter | Suggested starting point |
|-----------|---------------------------|
| `FRAME_CLASS` / `FRAME_TYPE` | Quad X |
| `FS_THR_ENABLE` | RC failsafe enabled |
| `FENCE_ENABLE` | 1 when testing outdoors (optional; Skylink also checks 1000 m) |
| `RTL_ALT` | 15–30 m for field tests |
| `PILOT_THR_FILT` | default |

Write params → **Save** → power-cycle Pixhawk.

#### ESP32 firmware

1. Build **`esp32dev_hw`** (no `SITL_MODE`).
2. `pio run -e esp32dev_hw --target upload`
3. Serial monitor: expect `Initializing FlightController in [HARDWARE MODE] via UART2`
4. Open browser → `http://<esp-ip>/` → link chips: **MAV** should go green when heartbeats arrive.

**Exit:** Dashboard shows attitude/GPS/battery from real FC with props off.

---

### Phase H3 — Calibration & configuration

**Duration:** 1–2 days  

In **Mission Planner** (USB):

- [ ] Accelerometer calibration (all orientations).
- [ ] Compass calibration (outdoor, away from metal).
- [ ] Radio calibration (endpoints, mode switch → `GUIDED`, `RTL`, `STABILIZE`).
- [ ] ESC calibration (ArduPilot procedure; **still no props** or use separate ESC tester).
- [ ] Motor test (MP Motor test sliders) — **props off**, verify order and direction.
- [ ] GPS lock outdoors: 3D fix, HDOP reasonable, map shows correct location on Skylink.

**Exit:** MP prearm checks pass; Skylink preflight shows GPS ≥ 3, MAVLink OK.

---

### Phase H4 — Tethered / low hover (props on, controlled)

**Duration:** several sessions  

**Safety:** helmet, clear area, fire extinguisher, second person, RC override ready, **geofence** and **RTL** tested.

Progression:

1. [ ] Arm/disarm from **RC only** first.
2. [ ] Arm from Skylink **GUIDED** with RC in override — short takeoff to 2–3 m.
3. [ ] Small `MOVE_BODY` 1 m; verify direction matches map heading.
4. [ ] `GOTO_LATLON` short hop inside geofence.
5. [ ] **LOITER** → **RTL** from map overlay.
6. [ ] Log review: STATUSTEXT arm errors visible in Log tab.

Tune ArduPilot if Skylink commands lag or overshoot:

- `WPNAV_SPEED`, `WPNAV_RADIUS`, `LOIT_BRK_ACCEL`, `PSC_*`

**Exit:** Repeatable 5 m hover and 10 m reposition without MP connected to TELEM2.

---

### Phase H5 — Skylink UI hardening for live flight

**Duration:** 3–5 days (software)  

Align with [simulation/upgrade/GCS_UPGRADE_ROADMAP.md](simulation/upgrade/GCS_UPGRADE_ROADMAP.md) Phase 6 + hardware items:

- [ ] Replace SIM banner with **LIVE AIRCRAFT** (red) when `simulation: false`.
- [ ] Hold-to-arm 1.5 s (`armHoldMs` in `gcs_config.js`).
- [ ] Firmware command rate limiter (`SKYLINK_CMD_RATE_LIMIT_PER_SEC`).
- [ ] Optional: structured errors `ERR_NOT_ARMED`, `ERR_NO_GPS`.
- [ ] Preflight **blocks arm in UI**; consider firmware reject on arm if GPS &lt; 3.
- [ ] Field checklist doc in `docs/` (props, battery, RC, geofence, weather).

**Exit:** Demo-safe UI for thesis / presentation.

---

### Phase H6 — Field operations & compliance

**Duration:** ongoing  

- [ ] Outdoor test site chosen (open, legal).
- [ ] India DGCA / local drone rules understood and followed (registration, zones, height limits where applicable).
- [ ] Maintenance log: battery cycles, prop checks, parameter backup (`.param` from MP).
- [ ] Optional: laptop `.tlog` via MP on USB during demo flights.

---

## 5. Software checklist (agent-friendly)

| Step | Action |
|------|--------|
| 1 | Finish SITL Phase 6 polish |
| 2 | Add `esp32dev_hw` env; keep `esp32dev_sitl` for daily dev |
| 3 | Flash HW firmware; verify UART wiring |
| 4 | Set `SERIAL2_*` and `SR2_*` params |
| 5 | `uploadfs` if UI banner changes |
| 6 | Bench MAVLink without props |
| 7 | Calibrate in MP |
| 8 | Prop-on flights per Phase H4 |

**Do not:**

- Run SITL and hardware firmware on the same ESP32 without reflashing.
- Connect MP and ESP32 to the **same** UART.
- Flash **PX4** expecting current GCS commands to work unchanged.
- Raise WS telemetry above **5 Hz** without queue safeguards ([IMPLEMENTATION_HANDOFF.md](simulation/upgrade/IMPLEMENTATION_HANDOFF.md)).

---

## 6. Troubleshooting

| Symptom | Likely cause | Fix |
|---------|----------------|-----|
| MAV chip red, no telemetry | Wrong TX/RX swap, baud, or `SERIAL2_PROTOCOL` | Swap wires; set 115200 MAVLink2 |
| Intermittent MAVLink | Loose GND, USB noise, weak 5V | Solid GND; power ESP32 from USB |
| GPS never ≥ 3 | Indoor, bad GPS mount | Go outside; check `GPS_TYPE` / wiring |
| Arm denied in UI | Real prearm from FC | Read STATUSTEXT in Log; fix in MP |
| Commands no motion | Not GUIDED, not armed, throttle failsafe | Mode select; RC mid throttle |
| Map click rejected | Beyond 1000 m geofence | Move target closer to home |
| Mode flicker | Another GCS on same UART | Only ESP32 on TELEM2 |
| Works in SITL, not HW | Still `SITL_MODE` build | Flash `esp32dev_hw` |

---

## 7. Differences: Pixhawk 2.4.8 vs Pixhawk 6C

Your repo’s older guide references **Pixhawk 6C** ([simulation_test_plan.md](simulation/simulation_test_plan.md)). For **2.4.8**:

| Item | 2.4.8 (typical) | 6C |
|------|------------------|-----|
| USB | Micro USB | USB-C |
| Connector | DF13 6-pin TELEM | JST GH |
| MCU | F427 class | Faster F7/H7 |
| Skylink UART code | **Same** GPIO 16/17 | **Same** |

Always confirm **TELEM2 = SERIAL2** in your board’s ArduPilot hwdef / silkscreen.

---

## 8. Relationship to GCS upgrade phases

```
GCS upgrade (browser + ESP32 features)
  Phase 0–5  ✅  (map, moves, goto, loiter)
  Phase 6    ⏳  (polish — do before aggressive field tests)
  Phase 7+   ⏳  (missions, AUTO)

Hardware roadmap (this document)
  H0–H4  Physical bring-up
  H5     Live-flight UI
  H6     Field ops & compliance
```

---

## 9. Document map

| Doc | Use |
|-----|-----|
| [simulation/upgrade/IMPLEMENTATION_HANDOFF.md](simulation/upgrade/IMPLEMENTATION_HANDOFF.md) | Current software state, bugs fixed |
| [simulation/upgrade/GCS_UPGRADE_ROADMAP.md](simulation/upgrade/GCS_UPGRADE_ROADMAP.md) | GCS feature phases |
| [simulation/simulation_test_plan.md](simulation/simulation_test_plan.md) | Original wiring § Phase 6 (6C-oriented) |
| [simulation/successful_run_guide.md](simulation/successful_run_guide.md) | SITL ports 5760/5762/5763 |
| [simulation/upgrade/MAVLINK_COMMANDS.md](simulation/upgrade/MAVLINK_COMMANDS.md) | Command ↔ MAVLink table |
| [simulation/upgrade/CONFIG_REFERENCE.md](simulation/upgrade/CONFIG_REFERENCE.md) | Tunables |
| [components/kits/pixhawk_248_evelta_combo.md](components/kits/pixhawk_248_evelta_combo.md) | Pixhawk 2.4.8 Full Combo Set Reference |
| [components/kits/readytosky_fc110_pixhawk_set.md](components/kits/readytosky_fc110_pixhawk_set.md) | ReadytoSky Pixhawk 2.4.8 Base Autopilot Set Reference |
| [components/kits/f450_quadcopter_frame_kit.md](components/kits/f450_quadcopter_frame_kit.md) | F450 Quadcopter Frame & Propulsion Kit Reference |
| [components/kits/power_and_insulation_accessories.md](components/kits/power_and_insulation_accessories.md) | Power Routing & Insulation Accessories Reference (XT60 & Heat Shrink) |
| [components/networking/waveshare_sim7600g_h_4g_dongle.md](components/networking/waveshare_sim7600g_h_4g_dongle.md) | Waveshare SIM7600G-H 4G LTE Dongle & GNSS Reference |
| [components/networking/7semi_ec200u_4g_dongle.md](components/networking/7semi_ec200u_4g_dongle.md) | 7Semi EC200U 4G LTE Cat 1 Dongle & GNSS Reference |
| [components/networking/7semi_ec200u_mini_industrial_modem.md](components/networking/7semi_ec200u_mini_industrial_modem.md) | 7Semi EC200U LTE 4G GNSS Mini Industrial Modem (26V Range) Reference |
| [components/networking/7semi_ec200g_mini_industrial_modem.md](components/networking/7semi_ec200g_mini_industrial_modem.md) | 7Semi EC200G-CN LTE Cat 1 4G GNSS Mini Industrial Modem Reference |
| [components/networking/7semi_ec200u_nano_modem.md](components/networking/7semi_ec200u_nano_modem.md) | 7Semi EC200U-CN LTE 4G GNSS Ultra-compact Nano Modem Reference |
| [components/networking/graylogix_ec200u_usb_modem.md](components/networking/graylogix_ec200u_usb_modem.md) | Graylogix EC200U 4G USB Modem Reference |
| [components/networking/7semi_ec200u_enclosed_usb_modem.md](components/networking/7semi_ec200u_enclosed_usb_modem.md) | 7Semi EC200U Enclosed USB-C Modem Reference |
| [components/networking/7semi_ec200u_smart_modem_arduino.md](components/networking/7semi_ec200u_smart_modem_arduino.md) | 7Semi EC200U IoT Smart Modem with Inbuilt Arduino Reference |
| [components/networking/internet_connectivity_alternatives.md](components/networking/internet_connectivity_alternatives.md) | Skylink Internet Connectivity Alternatives Evaluation |
| [components/kits/pixhawk_248_robokits_combo.md](components/kits/pixhawk_248_robokits_combo.md) | Pixhawk 2.4.8 Basic GPS Combo Kit Reference (Robokits) |
| [components/kits/pixhawk_quartz_standalone_comparison.md](components/kits/pixhawk_quartz_standalone_comparison.md) | Quartz Components Standalone Pixhawk 2.4.8 Comparison Reference |
| [components/final_reports/skylink_final_bom_report.md](components/final_reports/skylink_final_bom_report.md) | Skylink Final BOM & Internet-Flight Integration Reference |
| [components/final_reports/cellular_modems_and_premium_alternatives.md](components/final_reports/cellular_modems_and_premium_alternatives.md) | High-End Cellular Modems & Premium UAV Upgrades Brainstorming |
| [components/final_reports/modem_vs_dongle_comparison.md](components/final_reports/modem_vs_dongle_comparison.md) | 7Semi EC200U Industrial Modem vs USB Dongle Comparison |
| [components/final_reports/skylink_complete_assembly_guide.md](components/final_reports/skylink_complete_assembly_guide.md) | **Complete Assembly, Wiring, Firmware & First Flight Guide** |

---

## 10. Prompt for next AI agent

```text
Implement Pixhawk hardware support per docs/PIXHAWK_HARDWARE_ROADMAP.md.
Start with platformio.ini env esp32dev_hw (no SITL_MODE) and LIVE AIRCRAFT UI banner.
Do not change MAVLink command semantics. Reference IMPLEMENTATION_HANDOFF.md for WS limits.
User hardware: Pixhawk 2.4.8 + GPS + power module; UART2 on ESP32 GPIO16/17 @ 115200.
```

---

*Last updated: May 2026 — aligned with Skylink FW 12 / FS 15 and GCS Phases 0–5.*
