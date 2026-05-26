# Skylink Project — Complete Analysis & Ordering Guide

**Date:** May 26, 2026  
**Current State:** SITL simulation verified working (ArduCopter 4.8.0), FW Build 12, FS Build 15  
**Goal:** Order components → assemble real drone → port Skylink from SITL to hardware

---

## 1. PROJECT OVERVIEW

Skylink is an **ESP32-based WiFi MAVLink bridge** that turns a browser into a **Ground Control Station (GCS)** for ArduPilot.

### Architecture
```
Browser GCS ──WiFi──► ESP32 (Skylink) ──TCP (SITL) / UART (HW)──► ArduPilot
(index.html +            WebSocket /ws          flight_controller.cpp    SITL or
 gcs.js + gcs_map.js)    web_server.cpp                              Pixhawk 2.4.8
```

- **ESP32 firmware:** PlatformIO, Arduino framework, ESP Async WebServer, MAVLink
- **Browser UI:** Tabbed (Map/Fly/Status/Log), Leaflet map, telemetry at 5 Hz WebSocket
- **SITL mode:** ESP32 connects to GCS PC's LAN IP on TCP 5763
- **HW mode:** ESP32 UART2 (GPIO16/17) ↔ Pixhawk TELEM2, 115200 8N1

### Current Feature Set (Phases 0–5: ✅ Done)
- WiFi manager with JSON network config + auto-reconnect
- Async web server + WebSocket command dispatch
- MAVLink telemetry parsing (ATTITUDE, GLOBAL_POSITION_INT, GPS_RAW_INT, SYS_STATUS, HOME_POSITION, COMMAND_ACK, STATUSTEXT)
- Flight commands: arm/disarm, set mode, takeoff, land, RTL, MOVE_BODY, YAW_RELATIVE, GOTO_LATLON, GOTO_ALT, LOITER_HERE, emergency stop
- Map: Leaflet with drone marker (rotated by yaw), home marker, trail, geofence circle, click-to-fly
- Safety: firmware enforces GUIDED mode, GPS ≥ 3, min AGL 2m, move caps, geofence 1000m
- LED: WiFi status + MAVLink link + armed state on built-in LED
- NTP time sync, OTA updates, LittleFS file system

---

## 2. BUGS & ISSUES FOUND

### 🔴 Critical Issues

#### Bug 1: MOVE_BODY Z-axis direction is inverted
**File:** `src/flight_controller.cpp` line 216–264  
The NED (North-East-Down) convention means positive Z = **down**. The `moveBody()` function sends z as-is from the WebSocket command. But in `gcs.js:238`, Numpad8 sends `z: -dist` and Numpad2 sends `z: dist`. This means Numpad8 (Up) sends **negative Z** = **up** ✓, Numpad2 (Down) sends **positive Z** = **down** ✓.  

**But** the firmware's `SKYLINK_MOVE_BODY_MAX_M` cap (200m) applies equally to all axes, meaning z can be +200m or -200m. The AGL check only checks altitude > 2m, but doesn't clamp the resulting z to prevent the copter from descending below the AGL floor. If a MOVE_BODY with z=+200m is sent at 10m altitude, the copter would try to go 200m down, hitting the ground.

**Fix needed:** Add a firmware check in `moveBody()` to ensure `current_relative_alt + z_meters >= SKYLINK_MOVE_MIN_AGL_M` before sending the command.

#### Bug 2: WS queue overflow still possible under load
**File:** `src/web_server.cpp`, `main.cpp`  
The sendHeartbeat() + sendPendingFcEvents() pair runs every 200ms. The heartbeat is ~1KB JSON. With `WS_MAX_QUEUED_MESSAGES=64` and `setCloseClientOnQueueFull(false)`, each WS client can buffer up to 64 messages. At 5Hz, that's ~12.8 seconds of buffer. But if the browser tab is backgrounded or the network has a glitch, the queue fills up and the AsyncWebServer library starts dropping frames silently. There's no feedback to the user that telemetry is being dropped.

**Fix:** Add a queue depth indicator in the UI header chips (e.g., "WS ● (99%)" when near full). Or reduce to 2Hz fallback when no client ACK received.

#### Bug 3: `canExecuteGuidedMoveUnlocked()` uses `relative_alt` from `GLOBAL_POSITION_INT` but falls back to `altitude` from `VFR_HUD`
**File:** `src/flight_controller.cpp` line 200–207  
```cpp
const float agl = telemetry.relative_alt > 0.0f ? telemetry.relative_alt : telemetry.altitude;
if (agl < SKYLINK_MOVE_MIN_AGL_M) return false;
```
If `relative_alt` is 0 (not yet received from SITL) but `altitude` (barometric) is > 0, this works. But if the vehicle is at 0 relative_alt (on ground, pre-takeoff) AND altitude is also 0, this blocks all moves correctly. However if `relative_alt` is > 0 but very small (e.g., 0.1m after a brief touch-and-go), it's still used, and the check `0.1 < 2.0` blocks moves — which is correct but confusing if the pilot expects to be able to move at 1m altitude.

**Suggestion:** Add a STATUS_TEXT push when moves are rejected with the specific reason, so the pilot sees "MOVE rejected: below 2m AGL" in the Log tab.

### 🟡 Moderate Issues

#### Bug 4: `altitude` field overwritten by `GLOBAL_POSITION_INT`
**File:** `src/flight_controller.cpp` line 780–781  
```cpp
telemetry.relative_alt = pos.relative_alt / 1000.0f;
telemetry.altitude = telemetry.relative_alt;  // BUG: overwrites VFR_HUD altitude
```
When `GLOBAL_POSITION_INT` arrives, it replaces `telemetry.altitude` with the relative altitude (which is actually relative to home, not MSL). This means the altitude shown in the UI is always relative altitude, not absolute MSL altitude. For SITL both are essentially equivalent, but on a real Pixhawk, the barometric altitude (from VFR_HUD) and GPS relative altitude can differ significantly. The UI field labelled "Altitude" should really be "Relative Alt" for accuracy.

#### Bug 5: Keyboard throttle too aggressive for some users
**File:** `data/gcs.js` line 213  
```js
const throttle = CFG.keyboardMoveThrottleMs || 500;
```
The keyboard fires at most once per 500ms, which means max 2 keypresses/second. With a 200m move distance, a single press sends a 200m move. Combined with the firmware's 500ms debounce (`SKYLINK_CMD_DEBOUNCE_MS`), this creates a system where:
- User presses Arrow key → immediately sends MOVE_BODY
- 500ms pass → user presses again → sends another

But the actual move execution is rate-limited by ArduPilot's `WPNAV_*` tuning params. A 200m move may take 10+ seconds to complete. If the user presses again mid-move, they queue another move which ArduPilot will execute after the first completes. This is fine behavior but not documented.

**Fix:** Document in the UI: "Each move replaces the previous — wait for ACK before sending another."

#### Bug 6: `selectPresetDistance` not called on page load
**File:** `data/gcs.js` line 90–98  
The distance presets default to 1m active via the HTML class, but `selectedMoveM` starts at 1 and `updateDistanceLabel()` is called in `initMoveControls()`. Looking at the code flow: `initMoveControls()` → `updateDistanceLabel()` and `selectPresetDistance(1)` is never called, but the preset button for 1m has class `active` in HTML. The initial state is consistent: selectedMoveM=1, 1m button highlighted, label shows "Active: 1 m". This actually works correctly.

No bug here on closer inspection — the initial state is consistent.

### 🟢 Minor Issues

#### Bug 7: `preflightMinBatteryPct` threshold logic
**File:** `data/gcs.js` line 518  
```js
const batOk = bat < 0 || bat > minBat;
```
If `bat` is 100, `batOk` = `100 < 0 || 100 > 20` = `false || true` = `true` ✓  
If `bat` is 10, `batOk` = `10 < 0 || 10 > 20` = `false || false` = `false` ✓  
But if `bat` is 0 (unknown battery), `batOk` = `0 < 0 || 0 > 20` = `false || false` = `false` ✗  
This means **unknown battery (0%) shows as FAIL** in preflight. On SITL, battery_remaining is often 0 because there's no power module. The preflight check prevents arming if bat=0+20 condition fails, blocking arming even in SITL.

**Fix on SITL:** The `SYS_STATUS` in SITL sometimes sends `battery_remaining = -1` for unknown. If `d.battery` is -1, `batOk` = `-1 < 0` = `true` ✓. But in your telemetry code, `int battery_remaining = 0;` defaults to 0. So SITL with -1 sends 0 → `batOk = false`.

**Fix:** Change default to -1 in the struct, or check `bat < 0` more broadly.

#### Bug 8: `altitude` displayed vs `relative_alt`
**File:** `data/gcs.js` line 590  
```js
set('tl-alt', ... + (Number(d.altitude) || 0).toFixed(1) + ' m');
```
This uses `d.altitude` which, per Bug 4, is overwritten with `relative_alt`. The UI shows "Altitude" but it's actually "Relative Alt". Should be labelled consistently.

---

## 3. ORDERING GUIDE — What to Buy

### 3.1 WHAT YOU ALREADY HAVE (usable for drone)

| Component | Owned? | Use in Drone |
|-----------|--------|-------------|
| **ESP32 Dev Board** (30-pin) | ✅ | **Companion computer** — WiFi MAVLink bridge to browser |
| **Arduino Uno R3** | ✅ | Bench testing, sensor prototyping (not in final drone) |
| **Breadboard** (830pt) | ✅ | Prototyping circuits |
| **Jumper Wires** | ✅ | Temporary connections |
| **Perfboards** (2 sizes) | ✅ | Permanent circuit mounting |
| **XL4015 Buck Converter** | ✅ | Bench power supply testing (not flight-rated) |
| **Power Bank Boards** (2×) | ✅ | Bench power for ESP32 (not flight-rated) |
| **RFID Fobs + Cards** | ✅ | Not used in drone |

### 3.2 TARGET DRONE CONFIGURATION

Based on the research docs, the recommended build is:

**Frame:** F450 Quadcopter Frame Kit (450mm wheelbase, glass fiber + nylon arms)  
**FC:** Pixhawk 2.4.8 (STM32F427, runs ArduPilot Copter)  
**Companion:** ESP32 (your board)  
**Mission Planner PC:** Any laptop via USB to Pixhawk

### 3.3 PRIORITY ORDERING LIST

#### Tier 1 — Must Buy (Drone Won't Fly Without These)

| # | Item | Estimated Price (INR) | Vendor | Link Reference |
|---|------|---------------------|--------|---------------|
| 1 | **Pixhawk 2.4.8 Full Combo Set** (includes FC + GPS + Power Module + all accessories) | ₹10,342 | Evelta | `docs/components/kits/pixhawk_248_evelta_combo.md` |
| 2 | **F450 Quadcopter Frame Kit** (frame + 4× A2212 1000KV motors + 4× SimonK 30A ESCs + 1045 props) | ₹3,999 | Robocraze | `docs/components/kits/f450_quadcopter_frame_kit.md` |
| 3 | **3S LiPo Battery** (2200–5200mAh, XT60 connector) | ₹1,500–₹3,000 | Robocraze / RC hobby | e.g. 3S 5200mAh for 15min flight |

**Tier 1 Total:** ~₹15,800–₹17,300

#### Tier 2 — Strongly Recommended

| # | Item | Estimated Price (INR) | Why |
|---|------|---------------------|-----|
| 4 | **RC Transmitter + Receiver** (Flysky FS-i6X or Radiomaster TX12 + SBUS RX) | ₹3,000–₹6,000 | Manual override, mandatory for safe testing per Pixhawk roadmap |
| 5 | **LiPo Charger** (balance charger, e.g. IMAX B6) | ₹1,500–₹2,500 | Essential for safe LiPo charging |
| 6 | **LiPo Safe Bag** (fireproof) | ₹300–₹500 | Safety for charging/storage |
| 7 | **XT60 connectors + 14-16AWG silicone wire** | ₹200–₹500 | Power connections |

**Tier 2 Total:** ~₹5,000–₹9,500

#### Tier 3 — Nice to Have (Optional)

| # | Item | Estimated Price (INR) | Why |
|---|------|---------------------|-----|
| 8 | **6-pin DF13 crimp connector kit** | ₹200–₹500 | For clean Pixhawk ↔ ESP32 wiring |
| 9 | **Propeller set (1045, spare)** | ₹200–₹400 | Spares for crashes |
| 10 | **GPS Mast (replacement/extra)** | ₹200–₹500 | Already included, but good to have spare |
| 11 | **Micro USB cable (data)** | ₹200–₹300 | For Pixhawk ↔ Mission Planner USB |
| 12 | **4G Dongle (SIM7600G-H)** | ₹5,196 | For BVLOS cellular telemetry (advanced) |
| 13 | **LiPo voltage checker / alarm** | ₹200–₹400 | Prevents over-discharge |

**Tier 3 Total:** ~₹6,200–₹7,300

### 3.4 TOTAL BUDGET

| Tier | Range (INR) |
|------|------------|
| Minimum (Tier 1) | ~₹15,800–₹17,300 |
| Good (Tiers 1+2) | ~₹20,800–₹26,800 |
| Full (Tiers 1+2+3) | ~₹27,000–₹34,100 |

### 3.5 Which Pixhawk Kit to Buy?

| Option | Includes GPS? | Price | Best For |
|--------|-------------|-------|----------|
| **ReadytoSky FC-110 Base Set** (Evelta) | ❌ No GPS | ₹10,342 | If you already have a GPS module |
| **Full Combo Set** (Evelta) | ✅ Neo-M8N GPS | ~₹12,000–₹14,000 | **Recommended** — everything in one box |
| **Pixhawk 2.4.8 Basic GPS Combo** (Robokits) | ✅ Basic GPS | ~₹10,000 | Budget option |

**My recommendation:** Go with the **Full Combo Set** (Evelta) — it includes the Neo-M8N GPS+compass, power module, buzzer, safety switch, shock mount, I2C splitter, OLED display, PPM encoder, SD card, and RGB LED. You won't need to hunt for missing parts.

### 3.6 Where to Order (India)
| Item | Vendor | Notes |
|------|--------|-------|
| Pixhawk 2.4.8 Full Combo | [Evelta.com](https://evelta.com) | Trusted indian electronics distributor |
| F450 Frame Kit | [Robocraze.com](https://robocraze.com) | Direct link in docs |
| LiPo Battery | Robocraze or Amazon India | 3S 5200mAh, XT60 |
| RC TX/RX | Amazon India / Robu.in / Quadkopters.in | Flysky FS-i6X is cheapest option |
| LiPo Charger | Robocraze / Amazon | IMAX B6 or clone |

---

## 4. DOCUMENTS UPDATES NEEDED

### 4.1 Outdated / Needs Updates

| Document | Issue |
|----------|-------|
| `README.md` line 241–247 | Mentions "5 Development Phases" — project now has Phases 0–6 + hardware roadmap. Needs update. Also line 82: says "verified May 2026" — good. |
| `docs/simulation/README.md` | Needs WebSocket command table added (currently references IMPLEMENTATION_HANDOFF.md). |
| `docs/simulation/upgrade/GCS_UPGRADE_ROADMAP.md` | Phase 5 acceptance checklist items (lines 223–226) still have unchecked boxes even though Phase 5 is marked ✅ Done. Inconsistency. |
| `docs/simulation/upgrade/IMPLEMENTATION_HANDOFF.md` line 23 | Phase 6 shows ❌ Not started. The roadmap says Phase 6 is "Polish & safety." Should check if any Phase 6 items were partially done. |
| `docs/PIXHAWK_HARDWARE_ROADMAP.md` line 33 | References file:/// URLs for component docs — these work locally but will break if docs are moved. Use relative paths. |
| `docs/components/kits/f450_quadcopter_frame_kit.md` | The DF13 pinout diagram shows TELEM2 wiring with ESP32 — good. But doesn't mention the F450's max payload for ESP32+Pixhawk+GPS mast weight. |
| `docs/components/kits/pixhawk_248_evelta_combo.md` | Line 6–7: Product URL points to same URL as the ReadytoSky base set. The "Full Combo Set" may be a different Evelta product page. Verify the actual URL. |
| `docs/components/networking/waveshare_sim7600g_h_4g_dongle.md` | Documents integration the ESP32 doesn't yet have code for. This is a forward-looking doc, which is fine. But mark it as "future / not yet implemented" in the header. |
| `data/gcs_config.js` line 7 | `fsBuild: 15` — must match `SKYLINK_FS_BUILD 15` in `skylink_config.h`. Currently both 15 ✓. But the file comment says "Must match data/skylink_build.json" — the JSON also has `"fs": 15`. All three agree. ✓ |

### 4.2 Missing Documents

| Needed Document | What It Should Cover |
|----------------|---------------------|
| **Field Checklist** (for real flights) | Pre-flight, in-flight, post-flight procedures, safety, weather limits, RC override check |
| **BOM Spreadsheet** | Complete bill of materials with prices, vendor links, quantities |
| **Hardware Wiring Diagram** (single doc) | Unified diagram showing ESP32 → Pixhawk → PDB → ESCs → Motors → Battery → GPS → RC |
| **Troubleshooting Quick Reference** | Common hardware issues (no GPS fix, MAVLink timeout, arm denied) with fixes |

---

## 5. PHASE 6 — WHAT'S LEFT TO DO

From the roadmap, Phase 6 (Polish & Safety) hasn't been started:

| Task | Status | Notes |
|------|--------|-------|
| Hold-to-arm (1.5s with progress) | ❌ Not done | `armHoldMs` defined in config but no UI implementation |
| Command rate limiter (2/s) | ❌ Not done | `SKYLINK_CMD_RATE_LIMIT_PER_SEC` defined but not enforced |
| LINK_STATUS unified event (1Hz) | ❌ Not done | Reserved constant, not implemented |
| Collapsible/filtered comms log | ❌ Not done | Log exists but no filtering |
| Command history (last 10) | ❌ Not done | Track in sessionStorage |
| GOTO_ALT UI button | ❌ Not done | Firmware supports it, no UI button |
| ArduPilot structured error codes | ❌ Not done | ERR_NOT_ARMED, ERR_NO_GPS in JSON responses |
| Update simulation docs | ❌ Not done | WebSocket table, new UI flow |
| Screenshots/GIFs for BTP report | ❌ Not done | Needed for thesis |
| Full regression test script | ❌ Not done | Manual test checklist |

---

## 6. RECOMMENDATIONS

### Software
1. **Create two PlatformIO environments** before hardware arrives (as per `PIXHAWK_HARDWARE_ROADMAP.md`):
   ```ini
   [env:esp32dev_sitl]
   extends = env:esp32dev
   build_flags = ${env:esp32dev.build_flags} -D SITL_MODE

   [env:esp32dev_hw]
   extends = env:esp32dev
   build_flags = ${env:esp32dev.build_flags}
   ```

2. **Implement firmware-side rate limiter** — prevents command flooding even if UI sends fast
3. **Add battery = -1 (unknown) handling** — SITL mode should bypass battery preflight check
4. **Fix relative_alt vs altitude Bug 4** before hardware — label distinction matters for real flights
5. **Add safety check in moveBody** to prevent z-axis moves below min AGL

### Hardware Assembly (When parts arrive)
1. **Phase H0** — Set up dual PlatformIO envs, practice SITL regression
2. **Phase H1** — Bench assembly (no props), mount Pixhawk on vibration dampener, wire PDB
3. **Phase H2** — ESP32 ↔ Pixhawk UART (GPIO16/17 ↔ TELEM2, 115200 8N1)
4. **Phase H3** — Mission Planner calibration (accelerometer, compass, radio, ESC)
5. **Phase H4** — Tethered hover (props on, RC override ready), slowly increasing altitude

### Safety
1. **Props OFF on bench — always.** Even with a "software glitch", a 1045 prop at 1000KV can cause serious injury.
2. **Test RC override BEFORE Skylink arm.** Make sure the RC receiver's failsafe cuts motors.
3. **First flights with Skylink** should have a second person holding an RC with kill switch.
4. **Set FENCE_ENABLE=1** and `RTL_ALT=15m` in ArduPilot parameters before outdoor testing.
5. **LiPo charging only in fireproof bag, never unattended.**
6. **Always verify motor directions (CW/CCW) before mounting props.**

---

## 7. COMPLETE FILE INVENTORY

### Source Code (11 files)
```
src/main.cpp              — Entry, telemetry loop, LED update
src/flight_controller.cpp  — MAVLink bridge, TCP/UART, commands (831 lines)
src/web_server.cpp         — Async web server, WS dispatch (358 lines)
src/wifi_manager.cpp       — WiFi scan + JSON config + reconnect (173 lines)
src/config_manager.cpp     — EEPROM Preferences (66 lines)
src/led_controller.cpp     — LED patterns
src/logger.cpp             — Serial logging
src/config.cpp             — Pin/timing definitions
src/ota_updater.cpp        — OTA firmware updates
src/time_sync.cpp          — NTP time sync
src/build_info.cpp         — Firmware/FS build comparison
```

### Headers (12 files)
```
include/config.h, config_manager.h, flight_controller.h, led_controller.h,
logger.h, ota_updater.h, skylink_config.h, time_sync.h, web_server.h,
wifi_manager.h, ws_json.h, build_info.h
```

### Frontend (5 files)
```
data/index.html            — Main GCS UI shell (305 lines)
data/gcs.js                — Application logic (848 lines)
data/gcs_map.js            — Leaflet map integration
data/gcs.css               — Styling
data/gcs_config.js         — UI tunables (45 lines)
data/lib/leaflet/          — Vendored Leaflet library
```

### Documentation (22+ files)
```
README.md                    — Project overview
docs/
├── PIXHAWK_HARDWARE_ROADMAP.md  — Hardware bring-up plan
├── diagrams/                     — Architecture diagrams
├── esp32_research/               — ESP32 setup guides
├── map_feature/                  — Map feature development notes
├── simulation/
│   ├── upgrade/                  — GCS upgrade docs (8 files)
│   ├── successful_run_guide.md   — SITL run procedure
│   └── start_simulation.md       — Quick start
├── components/
│   ├── kits/                     — F450, Pixhawk research docs
│   ├── networking/               — 4G dongle research
│   └── WHAT_I_ALREADY_HAVE/      — Your component inventory
```