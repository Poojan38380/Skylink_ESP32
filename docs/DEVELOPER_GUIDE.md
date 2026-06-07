# Skylink ESP32 — Build System & Dashboard Troubleshooting Guide

> **Audience:** Future developers, AI assistants, and maintainers.  
> **Scope:** PlatformIO build environments, FS/firmware versioning, WebSocket behaviour, and MAVLink dashboard status chips.

---

## 1. The Two Build Environments (CRITICAL)

`platformio.ini` defines **two separate firmware builds**:

| Environment | Command flag | Use case |
|---|---|---|
| `esp32dev_sitl` | `-D SITL_MODE` | Software-in-the-loop simulation — connects to ArduPilot SITL over TCP |
| `esp32dev_hw` | *(none)* | Real hardware — reads MAVLink from UART2 (Serial2, pins 16/17) |

**These produce completely different binaries.** The default environment (first in `platformio.ini`) is `esp32dev_sitl`.

### Common mistake: running the wrong environment
```
# WRONG – uploads SITL firmware (default env):
pio run --target upload

# CORRECT – uploads hardware firmware:
pio run --target upload -e esp32dev_hw

# CORRECT – uploads SITL firmware:
pio run --target upload -e esp32dev_sitl
```

### How to verify which firmware is running (serial monitor)
```
[INFO] Initializing FlightController in [HARDWARE MODE] via UART2  ← hw build
[INFO] Initializing FlightController in [SITL MODE] via TCP port 5763  ← sitl build
```

### Dashboard tells you immediately
- **Simulation banner** ("Simulation mode — SITL only…") visible → SITL firmware is running.
- **Simulation banner** hidden → Hardware firmware is running.
- The banner is driven by `SKYLINK_SIMULATION` which is a compile-time `constexpr bool` tied to `SITL_MODE`.

---

## 2. The Three-Part Build Version System

The build version check exists to detect when the filesystem (LittleFS) and firmware binary are out of sync.

### Components

| File | Role | What to change |
|---|---|---|
| `include/skylink_config.h` | `#define SKYLINK_FS_BUILD N` — firmware's *expectation* | When bumping FS version (before `uploadfs`) |
| `data/skylink_build.json` | `{"fs": N}` — what is actually written to flash | Must always equal `skylink_config.h` |
| `data/gcs_config.js` | `fsBuild: N` — browser's expectation for mismatch warning | Must always equal `skylink_config.h` |

**All three must have the same number.** The firmware reads `skylink_build.json` at boot via `buildInfoBegin()` and compares it to the compiled `SKYLINK_FS_BUILD`. A mismatch causes `isFsBuildMatch()` to return `false`, which makes the dashboard build tag show `FW ? · FS ?`.

### The correct bump procedure (before every `uploadfs`)
1. Increment `N` by 1 in **all three files** simultaneously.
2. Run `pio run --target uploadfs` (uploads the filesystem — does NOT touch firmware).
3. Run `pio run --target upload -e esp32dev_hw` (re-flashes firmware — needed so it knows to expect new `N`).
4. Hard-refresh browser (`Ctrl+Shift+R`).

### What happens if you forget step 3 (common mistake)
```
# Serial log after uploadfs without re-flashing firmware:
[INFO] Build FW:12 | FS flash:16 (expected 15) MISMATCH — run uploadfs
```
The fix is NOT to re-run uploadfs. The FS is already correct. The **firmware** needs to be re-flashed to update its compiled-in expected value.

---

## 3. Why Dashboard Chips Go Red (Decoded)

The header shows four chips: `WS`, `SITL`, `MAV`, `WiFi`. Each maps to a field in the HEARTBEAT WebSocket event.

```
WS chip   → d.ws_connected         (true if any WS client connected)
SITL chip → d.sitl_tcp_connected   (SITL mode: TCP to ArduPilot; HW mode: always true)
MAV chip  → d.mav_connected        (true if MAVLink HEARTBEAT received from autopilot)
WiFi chip → d.wifi_connected       (true if ESP32 has WiFi association)
```

**In hardware mode:**
- `WS` = green as soon as browser connects.
- `SITL` = green always (hardcoded `return true` in `isSitlTcpConnected()` for non-SITL builds).
- `MAV` = green once UART2 receives valid MAVLink heartbeat from FC.
- `WiFi` = green as soon as WiFi connects.

**Red chips in hardware mode always mean one of:**
1. SITL firmware is running (not HW firmware) → fix: `pio run --target upload -e esp32dev_hw`
2. FC is not powered / UART cable disconnected / wrong baud rate
3. Browser cache is stale — hard refresh with `Ctrl+Shift+R`

---

## 4. Multiple WebSocket Clients Problem

### Symptom
Serial monitor shows:
```
WebSocket client #1 connected from 192.168.x.x
WebSocket client #2 connected from 192.168.x.x   (same IP, new connection)
```

### Why it happens
The AsyncWebServer library does not automatically close old WebSocket connections when a new one opens from the same browser. Every page refresh, tab open, or browser reconnect creates a new WS client. The old one lingers until it times out (up to 60+ seconds).

### Why it matters for flight safety
The ESP32 broadcasts ALL events (HEARTBEAT, ACK, STATUSTEXT) to **every connected client**. If two browser tabs are open and both execute the takeoff state machine independently, **both will send ARM and TAKEOFF commands** to the real drone. The `_takeoffInProgress` mutex flag in `gcs.js` only protects within a single tab.

### Fix implemented (web_server.cpp)
```cpp
case WS_EVT_CONNECT:
    ws.cleanupClients(1); // Evict stale connections — enforce max 1 active client
    client->setCloseClientOnQueueFull(false);
    ...
```
`ws.cleanupClients(1)` is an AsyncWebServer API that closes all but the most recent N clients. Placing it at the top of `WS_EVT_CONNECT` (before accepting the new client fully) ensures only 1 WS connection survives at any time.

### After the fix
The serial monitor will show only **one** WebSocket client entry per session, even after page refreshes. Old connections are immediately evicted rather than waiting to time out.

---

## 5. The `uploadfs` vs `upload` Distinction

| Command | What it does | Affects firmware? | Affects files? |
|---|---|---|---|
| `pio run --target upload` | Flashes the compiled C++ binary (`.bin`) | ✅ Yes | ❌ No |
| `pio run --target uploadfs` | Uploads LittleFS filesystem image (HTML/JS/CSS/JSON) | ❌ No | ✅ Yes |

**These are completely independent operations.** Running one does NOT update the other. Both are required when:
- You change any file in `data/` AND
- You change any C++ source (`src/` or `include/`)

### Which `-e` environment to use?
`uploadfs` is environment-agnostic — the filesystem partition is the same regardless of SITL/HW. You can use either env flag or none (defaults to `esp32dev_sitl`).

`upload` is environment-critical — always specify `-e esp32dev_hw` for real drone flights.

---

## 6. Takeoff State Machine Architecture (gcs.js)

The autonomous takeoff sequence is an ACK-driven state machine:

```
autonomousTakeoff()
  ├── Guard: _takeoffInProgress mutex (prevents re-entry / double-click)
  ├── Reset flightUiState.guided = false, stableCount = 0  (clear stale state)
  ├── sendCmd('SET_FLIGHT_MODE', {mode: 'GUIDED'})
  ├── Poll every 200ms for flightUiState.guided === true (timeout: 10s)
  │   └── When confirmed → _doArmAndTakeoff(meters)
  │       ├── Reset flightUiState.armed = false, stableCount = 0
  │       ├── sendCmd('ARM_DRONE')
  │       ├── Poll every 200ms for flightUiState.armed AND flightUiState.guided (timeout: 10s)
  │       └── When both confirmed → sendCmd('TAKEOFF', {altitude})
  └── On any timeout → log error, release mutex
```

### syncFlightUiState() — The stability filter
State is confirmed only after **3 consecutive identical heartbeats** (`flightStateStableSamples: 3` in `gcs_config.js`). This prevents false positives from transient telemetry.

UART heartbeats from ArduPilot come at ~1 Hz. The browser WS telemetry interval is 200ms. So you need at least ~3 seconds for a mode change to be "confirmed" by the JS state machine. Timeouts below 5s are unreliable. Current timeouts are 10s each.

### The "stale state" bug (fixed)
Before the fix, `flightUiState.guided` and `flightUiState.armed` were never reset at the start of a new takeoff attempt. If a prior attempt left `guided = true`, the next attempt's `waitForGuided` interval would fire immediately on the first tick, skipping the actual mode-switch confirmation. This caused TAKEOFF to be sent before the FC was ready.

---

## 7. Quick Reference: Checklist Before Any Flight Test

```
□ 1. FC powered and UART2 cable connected (pins 16 RX, 17 TX)
□ 2. GPS has 3D fix (≥6 sats in the field, visible in Status tab)
□ 3. Serial monitor shows: [HARDWARE MODE], MAV connected
□ 4. Browser dashboard: MAV chip = green, no simulation banner
□ 5. Build tag shows matching FW/FS numbers (not "FW ? · FS ?")
□ 6. Only 1 WebSocket client in serial log
□ 7. Status tab: all preflight checks passing
□ 8. ARMING_CHECK = 0 in Mission Planner if bypassing hardware checks
□ 9. Hard refresh browser before flight (Ctrl+Shift+R)
```

---

## 8. Incident Log — Build Version Bug (2026-06-07)

**Symptom:** Dashboard showed `Build FW ? · FS ?`, simulation banner, all chips red.

**Root cause chain:**
1. `uploadfs` bumped FS=15→16 in `skylink_build.json` and `gcs_config.js`
2. `skylink_config.h` was NOT updated — still had `SKYLINK_FS_BUILD = 15`
3. Firmware was re-uploaded via `pio run --target uploadfs` (uploads FS, NOT firmware)
4. Running firmware expected FS=15, found FS=16 on flash → mismatch
5. Additionally, previous firmware upload had used `esp32dev_sitl` (default) not `esp32dev_hw`
6. SITL firmware running → `SKYLINK_SIMULATION = true` → simulation banner visible
7. SITL firmware with no TCP SITL connection → `mavlinkActive = false` → MAV chip red

**Fix applied:**
- `skylink_config.h`: `SKYLINK_FS_BUILD` 15 → 16
- `web_server.cpp`: Added `ws.cleanupClients(1)` on WS connect
- Command to run: `pio run --target upload -e esp32dev_hw`

---

## 9. MAVLink Source System ID Bug (CRITICAL — Root Cause of Slow ACKs)

### Symptom
```
14:18:23  TX   SET_FLIGHT_MODE → {"mode":"GUIDED"}
14:18:33  ERR  [TAKEOFF ABORTED] GUIDED mode not confirmed after 10 s
14:18:47  ACK  Command 176: ACCEPTED      ← ACK arrived 24 seconds AFTER command!
```
The mode change is ACKed, but 24 seconds after it was sent — far past the 10-second timeout.

### Root Cause
Every outgoing MAVLink packet has a **source system ID (sysid)**. The GCS heartbeat correctly used `sysid=255` (the universal GCS identifier). But ALL command packets (`command_long_pack`, `request_data_stream_pack`, etc.) were using `sysid=1` — the **same as the autopilot itself**.

```cpp
// WRONG (before fix) — tells ArduPilot this command comes from sysid=1 (the autopilot)
mavlink_msg_command_long_pack(1, 255, &msg, 1, 1, MAV_CMD_DO_SET_MODE, ...)

// CORRECT (after fix) — consistent GCS identity across all packets
mavlink_msg_command_long_pack(255, MAV_COMP_ID_MISSIONPLANNER, &msg, 1, 1, MAV_CMD_DO_SET_MODE, ...)
```

ArduPilot uses the source sysid for:
1. **Connection tracking** — it associates commands with the GCS connection established via heartbeat. sysid=1 commands come from an "unknown" system, not from the registered GCS.
2. **Command authorization** — some command types require the sender to be a known GCS.
3. **Queue priority** — unrecognized senders may be queued behind trusted ones.

Sending heartbeats from sysid=255 but commands from sysid=1 creates an identity split. ArduPilot processes the commands eventually, but with significant delay.

### Fix Applied
Defined constants at the top of `flight_controller.cpp`:
```cpp
#define GCS_SYSID  255
#define GCS_COMPID MAV_COMP_ID_MISSIONPLANNER  // 190
```
All 9 MAVLink pack calls in the file now use `GCS_SYSID, GCS_COMPID` as source.

### Rule for Future Developers
> Every MAVLink message sent FROM the ESP32 GCS must use `sysid=255`. This must match the sysid in `sendHeartbeat()`. Never use `sysid=1` for outgoing GCS commands.

---

## 10. Command 511 UNSUPPORTED — MAV_CMD_SET_MESSAGE_INTERVAL

### Symptom
```
ERR  Command 511: UNSUPPORTED
ERR  Command 511: UNSUPPORTED   (appears 3x on each connection)
```

### Explanation
`MAV_CMD_SET_MESSAGE_INTERVAL` (511) is a newer MAVLink command to request specific message rates. The ArduPilot firmware on this hardware does not support it (returns UNSUPPORTED).

The code was calling both:
1. `REQUEST_DATA_STREAM` (legacy, always works) — via `requestDataStreams()`
2. `MAV_CMD_SET_MESSAGE_INTERVAL` (new, unsupported here) — via `requestMessageIntervals()`

This caused 3 UNSUPPORTED errors per reconnect, flooding the dashboard with ERR log entries.

### Fix Applied
Removed `requestMessageIntervals()` entirely. `requestDataStreams()` already handles all stream requests via the legacy API which is universally supported. The `setMessageInterval()` and `requestMessageIntervals()` functions were deleted.

---

## 11. WebSocket Client #1 Disconnected Immediately — Feature or Bug?

### Observation
```
[INFO] WebSocket client #1 disconnected
[INFO] WebSocket client #2 connected from 172.26.206.74
```
Client #1 disconnects immediately when client #2 connects.

### Answer: This is a FEATURE (the cleanup fix working correctly)
`ws.cleanupClients(1)` was added to `onWsEvent` on `WS_EVT_CONNECT`. When client #2 connects, the handler evicts client #1. This is intentional — it prevents stale browser connections from accumulating and receiving duplicate events.

### Previously (before fix)
Both clients would remain connected. Both would receive every HEARTBEAT/ACK broadcast. If both tabs had the takeoff button visible, both could independently trigger ARM + TAKEOFF.

---

## 12. Mission Planner "Missing Params 734 vs 733"

### Symptom
```
Missing Params 734 vs 733
at MAVLinkInterface.<getParamListAsync>...
```
Error appears when connecting Pixhawk to Mission Planner via USB.

### Explanation
This is a **Mission Planner bug**, not a Pixhawk or Skylink bug. It occurs when:
- The FC reports it has N parameters in a list summary
- Mission Planner receives only N-1 before the transfer completes
- The off-by-one triggers an exception in MP's parameter validation

**This does NOT indicate a corrupted parameter set or hardware fault.** The parameters loaded correctly — MP just had a counting discrepancy in its internal state machine.

### Fix
Simply close the error dialog and retry the parameter download. If it fails repeatedly:
- Reduce the Mission Planner → Config → Planner → "Mavlink" baud rate
- Use a shorter, higher-quality USB cable
- Disconnect and reconnect

---

## 13. Incident Log — MAVLink sysid Bug (2026-06-07)

**Symptom:** GUIDED mode confirmation timeout after 10s. ACK for mode change arrives 24 seconds late. TAKEOFF command never executes despite ARMING_CHECK=0 and 11 GPS satellites.

**Root cause:** All `mavlink_msg_command_long_pack()` calls used `sysid=1, compid=255` as the source identifier. The GCS heartbeat used `sysid=255`. ArduPilot received commands from an unregistered system, causing significant processing delay.

**Also fixed in same session:**
- Removed `MAV_CMD_SET_MESSAGE_INTERVAL` (511) calls — returned UNSUPPORTED by this FC
- Removed orphaned `setMessageInterval()` and `requestMessageIntervals()` declarations

**Files changed:**
- `src/flight_controller.cpp` — 9 pack calls updated + 2 functions deleted
- `include/flight_controller.h` — 2 orphaned declarations removed
