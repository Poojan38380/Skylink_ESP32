# Skylink — Production & Real-Flight Implementation Plan
**Version:** 1.0 | **Date:** 5 June 2026

---

## Overview

Skylink transitions from a WiFi-only simulation-oriented GCS to a **production-grade, remotely-accessible autonomous drone control system**. This plan covers six phases: ground-testing hardening → authentication & session control → UI production cleanup → local-network hardening → hybrid remote access via MQTT/Vercel → full field readiness.

---

## Architecture Decision (Approved: Hybrid)

```
                    ┌──────────────────────────────────────────────────────────────────┐
                    │                    HYBRID SKYLINK ARCHITECTURE                    │
                    └──────────────────────────────────────────────────────────────────┘

  ┌──────────────────────────────────────────────────────────────────────────────────────┐
  │  PATH A: LOCAL (Same WiFi — Zero Latency)                                            │
  │                                                                                      │
  │  Browser ──WiFi──► ESP32 WebSocket (ws://<ESP32_IP>/ws) ──UART──► Pixhawk           │
  │  (GCS on ESP32)                                                                      │
  └──────────────────────────────────────────────────────────────────────────────────────┘

  ┌──────────────────────────────────────────────────────────────────────────────────────┐
  │  PATH B: REMOTE (Over 4G Internet — Any Location)                                   │
  │                                                                                      │
  │  Browser ──Internet──► Vercel Dashboard ──MQTT──► HiveMQ Broker ──MQTT──► ESP32     │
  │  (Hosted on Vercel)                                               ──UART──► Pixhawk  │
  └──────────────────────────────────────────────────────────────────────────────────────┘

  ┌──────────────────────────────────────────────────────────────────────────────────────┐
  │  FAILSAFE (Dual Layer — Hardware + Software)                                         │
  │                                                                                      │
  │  ESP32 detects modem/internet loss ──UART──► MAVLink RTL command to Pixhawk          │
  │  (Layer 1, immediate, software)                                                      │
  │                                                                                      │
  │  Pixhawk FS_GCS_ENABLE=1 ──► RTL after 5s of no GCS heartbeat                      │
  │  (Layer 2, autonomous, hardware-level, always on)                                    │
  └──────────────────────────────────────────────────────────────────────────────────────┘
```

---

## Phase 1 — Ground Testing & Pre-Flight Hardening
**Goal:** Make the dashboard into a comprehensive ground-test tool. Verify every system before attaching propellers to fly.
**Files affected:** `data/index.html`, `data/gcs.js`, `data/gcs.css`, `data/gcs_config.js`, `src/web_server.cpp`, `src/flight_controller.cpp`

### 1.1 Fix the SITL Chip in Hardware Mode
**Problem:** `chip-sitl` is always visible in the header, showing "SITL ○" even in real hardware mode. This is confusing and unprofessional.

**Fix in `data/index.html`:**
- Wrap `chip-sitl` in a container that is initially `hidden` in hardware mode.

**Fix in `data/gcs.js` (`updateLinkChips()`):**
- In hardware mode (`simulation === false`), completely hide the `chip-sitl` DOM element (`el.hidden = true`).
- Replace the SITL label with a "HW" or "LINK" chip that shows MAVLink UART status instead.

### 1.2 Add a "LIVE AIRCRAFT" Warning Banner
**Problem:** In simulation mode, a yellow "Simulation mode" banner is shown. In hardware mode, nothing. The user has no prominent visual indicator that real motors and a real drone are under control.

**Fix in `data/index.html`:**
- Add a red `<div class="live-banner" id="live-banner" hidden>⚡ LIVE AIRCRAFT — Real motors active. Propellers are dangerous.</div>` below the sim-banner.

**Fix in `data/gcs.js`:**
- In the `HEARTBEAT` handler, if `d.simulation === false`, show `live-banner` and hide `sim-banner`.
- If `d.simulation === true`, show `sim-banner` and hide `live-banner`.

### 1.3 Remove "Waiting for MAVLink — open dashboard while SITL is running" Message
**Problem:** `gcs.js` line 534-536 shows a SITL-specific message in the preflight banner when MAVLink is not connected.

**Fix:**
- Check `d.simulation` flag and show either a SITL-specific or hardware-specific error message.
- Hardware mode message: `"Waiting for MAVLink — check TELEM2 wiring and SERIAL2_PROTOCOL parameter."`

### 1.4 Add a "Motor Test" Panel to the Dashboard (Ground Testing)
**Goal:** Be able to spin each motor (1-4) individually from the dashboard without propellers, just like Mission Planner's Motor Test page.

**New ESP32 command:** `MOTOR_TEST` — takes `{motor: 1-4, throttle_pct: 5-20, duration_ms: 2000}`.
**Implementation:**
- `web_server.cpp`: Add `handleMotorTest()` command handler.
- `flight_controller.cpp`: Add `motorTest(uint8_t motor_idx, float throttle_pct, uint32_t duration_ms)` that sends `MAV_CMD_DO_MOTOR_TEST` over MAVLink.
- `data/index.html`: Add a new **"Test" tab** in the tab bar (visible only when `simulation === false`).
- `data/gcs.js`: Add UI for the test tab with 4 motor buttons and a throttle slider (5%-20% range, capped for safety).

**Safety gates on Motor Test:**
- Only available when `DISARMED`.
- Throttle capped at 20% maximum.
- Duration capped at 3000ms.
- Requires the user to tick a "I confirm propellers are removed" checkbox before any motor test button is enabled.

### 1.5 Enhanced Preflight Diagnostics Panel
**Goal:** Expand the preflight checklist to show the actual values causing a failure, not just pass/fail.

**New checks to add:**
- Accelerometer calibration status (from AHRS health statustext)
- Compass calibration status
- Pixhawk safety switch state
- Number of GPS satellites (show actual count, e.g. "8 sats" not just pass/fail)
- Battery voltage (show actual "11.8V" not just pass/fail)
- MAVLink link latency

---

## Phase 2 — Authentication & Single-Client Session Control
**Goal:** Password-protect the dashboard. Enforce that only one authenticated session controls the drone at a time.

### 2.1 Login Page (Browser-Side)
**New file:** `data/login.html`
- Minimal, professional login page served from LittleFS at `http://<ESP32>/login`.
- Input field for password, submit button.
- On submit, sends `POST /auth` with `{password: "..."}`.
- If correct, receives a session token, stores it in `sessionStorage`, and redirects to `/` (main GCS).
- If incorrect, shows "Wrong password" with a 2-second lockout.

### 2.2 Session Token System (ESP32 Firmware)
**New file:** `src/auth_manager.cpp` + `include/auth_manager.h`

```
┌──────────────────────────────────────────────────────┐
│ AuthManager                                          │
│                                                      │
│ - const char* _password (from skylink_config.h)      │
│ - char _token[32]       (random alphanumeric)        │
│ - bool _tokenActive     (is someone logged in)       │
│ - uint32_t _lastActivity (millis of last WS message) │
│ - uint32_t SESSION_TIMEOUT_MS = 30 minutes           │
│                                                      │
│ + bool checkPassword(const String& pw)               │
│ + String generateToken()                             │
│ + bool validateToken(const String& token)            │
│ + void revokeToken()                                 │
│ + void resetActivity()                               │
│ + bool isExpired()                                   │
└──────────────────────────────────────────────────────┘
```

**Token generation:** Use `esp_random()` (hardware RNG on ESP32) to fill a 24-character base-62 string. Zero malloc, stack-allocated.

### 2.3 HTTP Auth Endpoint
**In `web_server.cpp`:**
- Add `POST /auth` endpoint.
  - Checks password against `SKYLINK_PASSWORD` from config.
  - If correct and no active session: generate token, return `{"ok": true, "token": "..."}`.
  - If correct but session already active: return `{"ok": false, "reason": "session_active"}` (so the second user is denied unless they know the password AND the session has expired).
  - **Special override:** If the new login is correct AND the existing session has been idle > 30 min, revoke old session and grant new one. This lets a reconnecting operator regain control.
  - If wrong password: return `{"ok": false, "reason": "wrong_password"}` with 429 after 3 attempts.

### 2.4 WebSocket Token Validation
**In `web_server.cpp` (`onWsEvent`):**
- On `WS_EVT_CONNECT`:
  - Read the `token` query parameter from the WebSocket URL: `ws://<ESP32>/ws?token=...`.
  - Validate via `authManager.validateToken(token)`.
  - If invalid: immediately close the socket with close code 4401.
  - If valid but another client already connected: immediately close with code 4409 (conflict), and the browser shows a "Another session is active" error.
- On `WS_EVT_DISCONNECT`:
  - Mark the controller client as gone. The token is still valid for 30 min (for reconnects after refresh).

**Single Client Enforcement Rule (approved):**
- Second browser with **correct password**: Kicks the old browser, grants access.
- Second browser with **wrong/no password**: Rejected.

### 2.5 Static File Protection
**In `web_server.cpp`:**
- Add a middleware that checks `Cookie: skylink-token=...` or query param on all static file requests.
- Unauthenticated `GET /` redirects to `/login`.
- Only `/login`, `/login.html`, `/favicon.ico`, and `/health` are publicly accessible.

### 2.6 Config (Password Storage)
**In `include/skylink_config.h`:**
```c
// --- Authentication ---
#define SKYLINK_PASSWORD          "your_password_here"   // Change this!
#define SKYLINK_SESSION_TIMEOUT_MS  1800000              // 30 minutes
#define SKYLINK_MAX_AUTH_FAILURES   3                    // Lockout after 3 wrong attempts
```

---

## Phase 3 — UI Production Cleanup & Polish
**Goal:** Transform the dashboard from a dev/sim tool to a professional production GCS.

### 3.1 Remove / Hide All SITL-Specific UI Elements in Hardware Mode
- `chip-sitl` → hidden in HW mode, show `chip-hw` instead (green when MAV connected).
- SITL settings row on Status tab → hidden in HW mode.
- All references to "SITL is running" in text → conditional on simulation flag.
- The `simulationBanner` config → controlled by server-sent `simulation` field, not a static config flag.

### 3.2 Live Aircraft Banner & Color Scheme
- Red top bar in live mode: `⚡ LIVE AIRCRAFT MODE — Flying a real drone`
- Yellow sim banner remains for SITL.
- Add a subtle "LIVE" pill badge in the header that glows red when armed.

### 3.3 Improved Status Tab — Connection Info
Current "SITL / MAV" field on Status tab shows SITL-specific info.
**Fix:** Show instead:
- Hardware mode: `MAVLink OK · UART2 · 115200`
- SITL mode: `SITL · <host>:<port>`

### 3.4 Ground Test Tab UI
New **"Ground Test"** tab (only shown when `simulation === false`):
- **Motor Test Panel:** 4 motor buttons (Motor 1–4), throttle % slider (5–20%), "Test" button, safety checkbox.
- **System Diagnostics Panel:** Display raw Pixhawk STATUSTEXT messages, AHRS health, sensor states.
- **Calibration Reminders:** Visual checklist of calibrations needed before flight.

### 3.5 Takeoff Dialog Improvement
Replace the ugly `prompt()` dialog (browser native) for takeoff altitude with an elegant in-page modal dialog matching the design language.

---

## Phase 4 — Dual-Layer Failsafe & Edge Case Handling
**Goal:** Make the system provably safe when hardware components fail or disconnect.

### 4.1 Internet/Modem Disconnect Detection (ESP32 Firmware)
**New mechanism in `src/modem_watchdog.cpp`:**

The EC200U modem is connected via USB. When the modem disconnects:
- Its USB device disappears from the system.
- The ESP32 loses the ability to publish MQTT heartbeats.

**Detection approach:** ESP32 pings a known IP (e.g., `8.8.8.8` via AT command `AT+QPING`) every 10 seconds when in flight (armed). If 2 consecutive pings fail, the ESP32 concludes internet is gone and triggers failsafe.

**Failsafe sequence:**
```
1. Internet ping fails × 2
2. ESP32 logs: "INTERNET LOST — triggering RTL"
3. ESP32 → MAVLink → SET_MODE(RTL) over UART2
4. ESP32 → also tries MAVLink COMMAND_LONG(NAV_RETURN_TO_LAUNCH)
5. Pixhawk FS_GCS_ENABLE=1 triggers RTL independently after 5s no heartbeat (Layer 2)
6. LED: fast red blink pattern
```

**Key safety note:** The ESP32 continues to send MAVLink heartbeats to the Pixhawk over UART2 even when the internet is down, so Layer 2 (FS_GCS_ENABLE) only triggers if the ESP32 itself crashes or reboots.

### 4.2 ESP32 Crash / Reboot Mid-Flight
If the ESP32 loses power or crashes while the drone is in the air:
- MAVLink heartbeats to Pixhawk stop within 1 heartbeat cycle (1 second).
- `FS_GCS_ENABLE=1` on the Pixhawk triggers RTL after 5 seconds of no heartbeat. **(Layer 2 covers this case — no additional code needed.)**

Verify `FS_GCS_ENABLE=1` is set before every flight in the pre-flight checklist.

### 4.3 WebSocket Client Disconnect During Flight
If the browser disconnects mid-flight (mobile screen turns off, network blip):
- The drone **continues on its last command** (GUIDED/LOITER).
- The Pixhawk keeps getting MAVLink heartbeats from the ESP32 (over UART, unaffected by browser).
- The browser auto-reconnects via the existing exponential-backoff retry logic.
- On reconnect, it gets the full telemetry state from the next HEARTBEAT broadcast.

**Enhancement:** When the client reconnects after a session token timeout (30 min), redirect to login instead of auto-connecting to a stale socket.

### 4.4 MAVLink Link Loss (UART Between ESP32 and Pixhawk)
If the UART cable between ESP32 and Pixhawk disconnects:
- `flightController.isConnected()` returns `false`.
- `chip-mav` turns red on the dashboard.
- **If currently armed:** ESP32 detects `mav_connected = false` and `fc.armed = true` in the telemetry struct, then immediately sends an RTL command (best effort — though the cable is disconnected, this serves as a log entry and retry attempt).
- Pixhawk `FS_GCS_ENABLE=1` handles the real recovery since it stops receiving GCS heartbeats.

### 4.5 Low Battery Failsafe
- Already handled by ArduCopter's built-in `FS_BATT_VOLTAGE` and `FS_BATT_MAH` parameters.
- **Dashboard enhancement:** When `battery_v < 10.8` (3.6V/cell), show a full-screen red blinking overlay: `⚠ CRITICAL BATTERY — LAND NOW`.

---

## Phase 5 — Remote Access via Hybrid MQTT + Vercel Dashboard
**Goal:** Access the Skylink GCS from anywhere in the world via the 4G modem.

### 5.1 Infrastructure Setup

**Cloud services needed (all free tiers sufficient):**
| Service | Purpose | Free tier |
|---------|---------|-----------|
| **HiveMQ Cloud** | MQTT broker | 10,000 messages/day, 100 connections |
| **Vercel** | Host the remote web dashboard | Unlimited hobby projects |
| **Cloudflare** | Optional: custom domain + DNS | Free |

### 5.2 ESP32 MQTT Client (New Firmware Module)
**New file:** `src/mqtt_client.cpp` + `include/mqtt_client.h`

**Library:** `PubSubClient` for Arduino — lightweight, works on ESP32.

**Topics:**
```
skylink/telemetry        → ESP32 publishes heartbeat JSON (same format as WebSocket HEARTBEAT)
skylink/commands         → ESP32 subscribes; Vercel publishes flight commands
skylink/ack              → ESP32 publishes command acknowledgments
skylink/status           → ESP32 publishes connection/health status
```

**Auth:** MQTT broker credentials stored in `skylink_config.h`. TLS connection to HiveMQ (port 8883).

**Publish rate:** 2 Hz when armed (reduce to 0.5 Hz when disarmed to save bandwidth).

**Command handling:** Vercel dashboard publishes a JSON command to `skylink/commands`. The ESP32 receives it, validates the auth token embedded in the MQTT message, and calls the same `dispatchCommand()` function already used by WebSocket.

### 5.3 Vercel Remote Dashboard (New Next.js App)
**New project:** `skylink-remote/` directory in the repository.

**Tech stack:** Next.js 14 (App Router) + MQTT.js WebSocket client + Leaflet for map.

**Features:**
- Login page with the same shared password.
- Real-time telemetry display (map, altitude, battery, mode, armed state).
- Flight command buttons (Arm/Disarm, Mode, RTL, Land, GOTO).
- Connection status: shows if ESP32 is online (last MQTT message < 5 seconds ago).
- Read-only mode when a local WebSocket session is the primary operator (dual-session protection: the local operator has priority; remote gets view-only unless local disconnects).

**Session priority logic:**
```
ESP32 tracks:
- local_client_active: bool  (someone connected via local WebSocket)
- remote_client_active: bool (someone connected via MQTT/Vercel)

Rules:
- If local is active: remote gets VIEW ONLY (telemetry yes, commands NO)
- If local is inactive: remote gets full control
- Local always has priority (lower latency = safer)
```

### 5.4 4G Modem Configuration (EC200U)
For MQTT to work over 4G, the EC200U must be in data mode:

```
AT commands needed (sent from ESP32 UART):
AT+QCFG="usbnet",1       → Set to ECM/RNDIS mode (provides internet to the ESP32)
AT+QICSGP=1,1,"airtelgprs.com","","",0  → APN (change for your carrier)
AT+QIOPEN=...             → Open TCP socket for MQTT (or use AT+QMTOPEN)
```

The preferred approach for the ESP32 to use the modem's internet is via **AT+QMTOPEN** (Quectel MQTT commands built into the EC200U firmware), which handles the MQTT stack on the modem chip itself. This is simpler than running PubSubClient on the ESP32.

### 5.5 Modem Connectivity Watchdog Integration
The modem watchdog (from Phase 4.1) is the same component that:
1. Detects internet loss → triggers RTL.
2. Monitors MQTT last-publish time to verify modem is actually sending data.

---

## Phase 6 — Field Readiness & First Real Flight
**Goal:** Final verification before the first outdoor GPS-guided flight.

### 6.1 Outdoor Pre-Flight Checklist (In Dashboard)
Before every real flight, the dashboard should enforce completion of this checklist. **All items must be green** before the Arm button is enabled.

| Check | Source | Pass Condition |
|-------|--------|----------------|
| WiFi Connected | ESP32 | Signal > -80 dBm |
| MAVLink Active | ESP32 UART | `mav_connected = true`, heartbeats < 1.5s ago |
| Battery Voltage | Pixhawk BATT_MONITOR | > 11.4V (3.8V/cell) |
| Battery % | Pixhawk | > 20% |
| GPS 3D Fix | Pixhawk | `gps_fix >= 3` |
| GPS Sats | Pixhawk | >= 8 satellites |
| AHRS Healthy | Pixhawk STATUSTEXT | No "Unhealthy AHRS" messages in last 10s |
| Home Position Set | Pixhawk | `home_valid = true` |
| Disarmed | Pixhawk | `armed = false` |
| Propellers Confirmed | User Checkbox | User must manually check "Propellers are secure" |
| FS_GCS_ENABLE | Known config | User confirms (shown as reminder) |

### 6.2 Calibration Procedure (One-Time, Before First Flight)
Do these in Mission Planner (USB) before the first field flight:
1. **Accelerometer (6-position):** All 6 orientations.
2. **Compass:** Outdoors, rotate all axes.
3. **ESC Calibration:** All-at-once ArduPilot method.
4. **Radio Calibration:** Optional but recommended for backup.

### 6.3 First Flight Protocol
1. **Start with propellers removed** → verify all dashboard preflight items green.
2. **Arm test** → verify motor spin direction (M1=CW, M2=CCW, M3=CW, M4=CCW for X frame).
3. **Attach propellers** → verify correct CW/CCW per motor.
4. **Tethered test** → arm outdoors, 10-20% throttle, confirm drone response.
5. **First hover** → 1.5m altitude, LOITER, check stability, land.
6. **First GPS move** → 2m altitude, move 3m North, return, land.

---

## Summary: File Change Map

### ESP32 Firmware Changes

| File | Change |
|------|--------|
| `include/skylink_config.h` | + `SKYLINK_PASSWORD`, `SKYLINK_SESSION_TIMEOUT_MS`, `SKYLINK_MAX_AUTH_FAILURES` |
| `include/auth_manager.h` | **NEW** — Session token manager interface |
| `src/auth_manager.cpp` | **NEW** — Token generation, validation, expiry |
| `include/modem_watchdog.h` | **NEW** — EC200U internet health monitor |
| `src/modem_watchdog.cpp` | **NEW** — Ping-based internet detection + RTL trigger |
| `include/mqtt_client.h` | **NEW** — MQTT remote bridge interface |
| `src/mqtt_client.cpp` | **NEW** — MQTT publish/subscribe via EC200U |
| `src/web_server.cpp` | + Auth middleware, `/auth` endpoint, WS token validation, single-client enforcement, motor test handler |
| `src/flight_controller.cpp` | + `motorTest()` MAVLink command, RTL-on-demand function |
| `src/main.cpp` | + modemWatchdog.begin(), mqttClient.begin(), loop updates |

### Web Dashboard Changes

| File | Change |
|------|--------|
| `data/login.html` | **NEW** — Login page with password form |
| `data/index.html` | + Live Aircraft banner, Ground Test tab, improved header chips |
| `data/gcs.js` | + Auth token flow, SITL/HW mode chip logic, sim/live banner logic, motor test UI, improved preflight panel, takeoff modal |
| `data/gcs.css` | + Login styles, Live banner styles, Ground Test tab styles, critical battery overlay |
| `data/gcs_config.js` | + `simulationBanner: false` (now controlled by server flag, not static config) |

### New Remote Dashboard

| Path | Description |
|------|-------------|
| `skylink-remote/` | Next.js 14 app for Vercel deployment |
| `skylink-remote/app/page.tsx` | Remote GCS dashboard |
| `skylink-remote/app/login/page.tsx` | Remote login page |
| `skylink-remote/lib/mqtt.ts` | MQTT client using MQTT.js |

### Documentation

| File | Description |
|------|-------------|
| `final-docs/COMPONENT_LIST.md` | ✅ Created — Hardware and software inventory |
| `final-docs/PRODUCTION_PLAN.md` | This document |
| `final-docs/FIELD_FLIGHT_CHECKLIST.md` | To be created in Phase 6 |

---

## Risk Register

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|-----------|
| MQTT latency too high for safe flight commands | Medium | High | **Always use local WebSocket for actual flight commands. MQTT is telemetry only (view-only when local is active).** |
| Session token stored in ESP32 RAM lost on reboot | Low | Low | On reboot mid-flight, Pixhawk FS_GCS_ENABLE triggers RTL. On ground, user re-logs in. |
| 4G modem not getting internet (APN config wrong) | Medium | Medium | Phase 4 internet ping detects this. Local WiFi dashboard still works. |
| Second operator takes control during critical maneuver | Low | High | Local client always has priority. Remote is view-only when local is active. Password required to kick local session. |
| ESP32 runs out of RAM with MQTT + Auth + WebServer | Medium | High | Measure heap after each phase. AsyncWebServer is lean. PubSubClient is ~5KB. Auth tokens are stack-allocated. |

---

## Execution Order

```
Phase 1 (Ground Testing UI)           → 1-2 days coding
Phase 2 (Auth + Single Session)       → 2-3 days coding
Phase 3 (UI Production Cleanup)       → 1 day coding
Phase 4 (Failsafe + Edge Cases)       → 2-3 days coding
Field Tests (after Phase 1-4)         → 1 day outdoors
Phase 5 (MQTT + Vercel Remote)        → 3-5 days coding
Phase 6 (First Real Flight)           → 1 day outdoors
```
