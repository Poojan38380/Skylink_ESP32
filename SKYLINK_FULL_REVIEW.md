# Skylink — Definitive Technical Review & Fix Report

> **Based on:** Full codebase review (`src/`, `include/`, `data/`) + IEEE paper (`Skylink_v2-7_IEEE_Submission.md`)  
> **Purpose:** Find everything that needs fixing before real flight, and chart the path to Skylink v3.

---

## Part 1 — Paper vs. Code: Discrepancies & Misrepresentations

These are places where the paper says one thing but the code does something different. These are **not just cosmetic** — some of them directly affect safety.

---

### DISCREPANCY 1 — AGL Floor Value: Paper says 2.0 m, Code has 0.8 m

**Paper (Section V-D, Table IV, Table VIII):**
> `h_min = 2 m` (AGL check). Table IV: *"Altitude floor (AGL) for MOVE_BODY / GOTO_ALT — 2.0 m"*

**Code (`include/skylink_config.h`, line 36):**
```cpp
#define SKYLINK_MOVE_MIN_AGL_M  0.8f    // allows GUIDED moves from ~1 m AGL (was 2.0)
```

**The problem:** The AGL floor was quietly lowered from 2.0m to 0.8m at some point, but the paper was never updated. The paper's safety claims, its Table IV, and its Table VIII constraint matrix all report **2.0 m** — which is factually wrong. The actual firmware threshold is **0.8 m**, meaning the drone can be commanded to move when it is only 80 cm off the ground.

This also combines with the acknowledged AGL floor *defect* (when `relative_alt ≤ 0`, MSL altitude is used as fallback, bypassing the check entirely). The paper says the defect bypasses a **2 m floor**; in reality it bypasses a **0.8 m floor** — a different safety claim.

**What to fix:**
- In the paper: update all references to `h_min = 2 m` → `h_min = 0.8 m`, OR restore the code to 2.0m if 2.0m was the intended design.
- In the code: fix the AGL defect regardless — the fallback to MSL altitude must be removed.

---

### DISCREPANCY 2 — Geofence Radius: Paper says 1,000 m, Code enforces 200 m

This is the **most dangerous discrepancy** in the entire paper.

**Paper (Table III, Table IV, Table VIII, Section IV-B, Phase 4 acceptance criterion):**
> *"GOTO_LATLON: radius ≤ 1,000 m"*  
> *"Geofence radius (GOTO_LATLON): 1,000 m"*  
> *"Geofence block (>1,000 m) ✓"*  
> *"Map tab: ... 1,000 m geofence circle"*

**Code (`include/skylink_config.h`, line 37):**
```cpp
#define SKYLINK_GOTO_MAX_RADIUS_M  200.0f  // tighter geofence for close field tests (was 1000)
```

**Code (`data/gcs_config.js`, line 10):**
```js
geofenceRadiusM: 1000,  // frontend uses 1000m
```

**The result is a split-brain system:**
- The **map** draws a 1,000 m geofence circle visually
- The GCS **allows the operator to click** any point within 1,000 m of home
- The **firmware silently rejects** any `GOTO_LATLON` target beyond 200 m — no error message is shown to the user
- The drone just doesn't go. The operator has no idea why.

This means the paper's SITL verification "Geofence block (>1,000 m) ✓" actually tested a **1,000 m firmware limit** (since the value was 1000m at the time of testing), but the deployed firmware now enforces **200 m**. The verification result in the paper is no longer reproducible with the current code.

**What to fix:**
- **Immediately:** Synchronize `gcs_config.js:geofenceRadiusM` and `skylink_config.h:SKYLINK_GOTO_MAX_RADIUS_M` to the same value.
- Add a firmware-side error event that tells the GCS *why* GOTO_LATLON was rejected (geofence exceeded), so the user sees feedback instead of silence.
- Update the paper to reflect whichever value is chosen as authoritative.

---

### DISCREPANCY 3 — `TAKEOFF` Has No Firmware-Side Guards (Armed, GUIDED, GPS)

**Paper (Table III):**
> `TAKEOFF: MAVLink active; alt param; bound 1–50 m` — *Filtering Location: Browser JS*

**Code (`src/flight_controller.cpp`, lines 154–177):**
```cpp
void FlightController::takeoff(float altitudeMeters) {
    if (!takeMutex()) return;
    if (!mavlinkActive) {          // Only checks MAVLink is active
        logger.warning("Cannot takeoff — no MAVLink link to autopilot");
        giveMutex(); return;
    }
    // NO armed check
    // NO GUIDED mode check
    // NO GPS fix check
    // NO altitude bounds check
    sendMavlinkPacket(&msg);  // Just sends it
    giveMutex();
}
```

The paper is **accurate** that filtering is browser-only, but this is itself a design flaw that the paper treats as a "limitation" rather than a bug. In practice, a TAKEOFF sent via any custom WebSocket client (or a browser crash/replay) bypasses all checks and ArduPilot may accept it in any mode. **In STABILIZE mode with TAKEOFF received, the drone spins up motors uncontrolled.**

**What to fix (firmware):**
```cpp
void FlightController::takeoff(float altitudeMeters) {
    if (!takeMutex()) return;
    if (!mavlinkActive) { logger.warning("..."); giveMutex(); return; }
    if (!telemetry.armed) { logger.warning("Cannot takeoff — not armed"); giveMutex(); return; }
    if (telemetry.flight_mode != COPTER_MODE_GUIDED) {
        logger.warning("Cannot takeoff — not in GUIDED mode");
        giveMutex(); return;
    }
    if (telemetry.gps_fix < 3) { logger.warning("Cannot takeoff — GPS fix < 3D"); giveMutex(); return; }
    // Clamp altitude
    const float alt = constrain(altitudeMeters, 1.0f, 50.0f);
    ...
}
```

---

### DISCREPANCY 4 — `ARMING_CHECK = 0` Must Not Be Forgotten

**Paper (Section VII-D):**
> *"The indoor motor-spin test procedure requires `ARMING_CHECK = 0` to bypass GPS-related arming checks during bench validation; this setting must be restored to its operational value (`ARMING_CHECK = 1`) before any outdoor or flight testing."*

This is correctly disclosed in the paper, but there is **no enforcement mechanism** in the code or process to ensure this happens. There is no runtime check, no warning, no documentation in the code itself.

**What to fix:**
- Add a startup check: if `ARMING_CHECK` comes back as 0 in any arming-related status, log a loud warning: `"SAFETY: ARMING_CHECK=0 detected — restore before flight!"`
- This requires parsing the `PARAM_VALUE` MAVLink message for `ARMING_CHECK` — doable with a one-time param request on startup.

---

### DISCREPANCY 5 — Paper Table IV Lists `GOTO_LATLON` Geofence but `MOVE_BODY` Geofence is "Not Applied"

**Paper (Table VIII):**
> `MOVE_BODY: Geofence — Not applied`

This is accurate for the *current* code — but it's a design oversight that should be flagged more strongly. A 200 m body-frame movement command (the current cap) from the edge of the geofence could send the drone outside the permitted zone. The geofence only applies to absolute GPS positions in `GOTO_LATLON`, not to relative body-frame moves.

---

### DISCREPANCY 6 — Paper Claims `h_min = 2 m` but the AGL Defect Makes It `h = MSL_altitude`

**Paper (Section VII-D):**
> *"when `telemetry.relative_alt` is zero or negative ... code falls back to MSL altitude (`telemetry.altitude`), which may be non-zero at ground level — causing the **2 m AGL floor** to be bypassed."*

**Code (`src/flight_controller.cpp`, line 198):**
```cpp
const float agl = telemetry.relative_alt > 0.0f ? telemetry.relative_alt : telemetry.altitude;
if (agl < SKYLINK_MOVE_MIN_AGL_M) return false;
```

The defect is real and described correctly — except the paper says "2 m floor" when the actual value is now 0.8 m. But more importantly: a drone on the ground at a field site at 400 m MSL elevation has `telemetry.altitude ≈ 400m`, `telemetry.relative_alt = 0`. The fallback gives `agl = 400`, which is **way above 0.8m**, so the check PASSES and the drone would accept a MOVE_BODY or GOTO_ALT command while still on the ground. **This is a crash cause.**

**Fix:**
```cpp
// CORRECT fix: require relative_alt to be valid (> 0) as a hard precondition
bool FlightController::canExecuteGuidedMoveUnlocked() const {
    if (!mavlinkActive) return false;
    if (!telemetry.armed) return false;
    if (telemetry.flight_mode != COPTER_MODE_GUIDED) return false;
    if (telemetry.gps_fix < 3) return false;
    // FIXED: only use relative_alt; reject if not valid
    if (telemetry.relative_alt <= 0.0f) return false;   // home not established
    if (telemetry.relative_alt < SKYLINK_MOVE_MIN_AGL_M) return false;
    return true;
}
```

---

---

## Part 2 — Critical Bugs (Crash Causes)

These directly cause or contribute to the drone crashing on takeoff.

---

### BUG A — Takeoff Command Sent Before ARM Is Confirmed by Autopilot

**File:** [`data/gcs.js`](file:///d:/btp_skylink/Skylink/data/gcs.js#L920-L957)

**Root cause:** `_doArmAndTakeoff()` polls `flightUiState.armed` every 200 ms. This state requires **3 stable consecutive heartbeats** (`flightStateStableSamples = 3`) before updating, meaning it takes **at least 600 ms after the FC actually arms** to change. But the poll fires every 200 ms, so TAKEOFF is sent as early as 200 ms after seeing the armed state — which may itself be stale by 600 ms.

**Worse:** The ARM command itself is subject to UART latency. The sequence is:
1. t=0: `ARM_DRONE` sent via WebSocket → UART → ArduPilot
2. t=~50ms: ArduPilot processes ARM
3. t=~50ms: ArduPilot sends COMMAND_ACK (ARM accepted)
4. t=~100ms: HEARTBEAT with armed=true arrives
5. t=~300-500ms: `flightUiState.armed` flips (after 3 stable HBs)
6. t=~300-700ms: **TAKEOFF is sent** ← this is CORRECT timing only if the poll fires after step 5

But if the poll fires at t=200ms (before step 5), armed is still false → poll waits.  
If at t=400ms armed just became true → TAKEOFF sent. **This is actually fine in theory.**

**The real race condition** occurs when:
- The drone was previously armed and then disarmed (e.g., a failed takeoff attempt)
- `flightUiState.armed` is invalidated at line 925 (`flightUiState.armed = false`) but stale **heartbeats are still coming in**
- The next heartbeat arrives with `armed = true` (ArduPilot re-armed faster than expected)
- `stableCount` immediately reaches 3 and `_doArmAndTakeoff()` sends TAKEOFF before the new ARM sequence is complete

**The actual crash scenario:** Operator clicks TAKEOFF twice rapidly. First attempt: drone arms. Second click: `_takeoffInProgress` guard fires (correctly blocks). But `flightUiState.armed` is still `true` from the first arm. When the first takeoff fails for any reason, the state is reset but the second attempt immediately sees `armed=true` and sends TAKEOFF before ArduPilot has processed the first disarm.

**Fix:** Use the ACK event mechanism that already exists. When `COMMAND_ACK` for `MAV_CMD_COMPONENT_ARM_DISARM` with `result=0` arrives, THEN send TAKEOFF (with a 300ms delay for motor spinup). This is the correct, deterministic approach.

```js
// In ws.onmessage handler, add:
case 'ACK':
    if (d.command === 400 && d.ok && _waitingForArmAck) {
        _waitingForArmAck = false;
        setTimeout(() => sendCmd('TAKEOFF', { altitude: _pendingTakeoffAlt }), 300);
    }
    break;
```

---

### BUG B — `autonomousTakeoff()` Uses `prompt()` — Blocks WebSocket Thread

**File:** [`data/gcs.js`](file:///d:/btp_skylink/Skylink/data/gcs.js#L968)

`window.prompt()` suspends **all JavaScript execution** including WebSocket message processing. If the operator has the prompt dialog open for 3+ seconds and a heartbeat comes in, that heartbeat is queued. When prompt() closes, the message queue drains all at once — potentially updating `flightUiState` with multiple messages simultaneously and causing incorrect stable-count behavior.

More critically: if the WS connection drops while `prompt()` is blocking, the `ws.onclose` handler fires immediately after prompt returns. The reconnect timer starts, and the takeoff sequence is now running against a disconnected WS.

**Fix:** Replace with a non-blocking HTML modal, similar to `goto-sheet` already in the UI:
```html
<div class="takeoff-sheet" id="takeoff-sheet" hidden>
  <h3>Takeoff altitude</h3>
  <input type="number" id="takeoff-alt" min="1" max="50" value="3">
  <button onclick="confirmTakeoff()">Launch</button>
  <button onclick="cancelTakeoff()">Cancel</button>
</div>
```

---

### BUG C — WiFi Reconnect Blocks Arduino Loop for Up to 30 Seconds

**File:** [`src/wifi_manager.cpp`](file:///d:/btp_skylink/Skylink/src/wifi_manager.cpp#L107-L131)

```cpp
bool WiFiManager::reconnectDirect() {
    for (const auto& savedNet : savedNetworks) {
        WiFi.begin(savedNet.ssid.c_str(), savedNet.password.c_str());
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);   // ← BLOCKING! Up to 10 seconds per network
            attempts++;
        }
    }
}
```

With 3 saved networks, this blocks for **up to 30 seconds**. During this time:
- `flightController.handle()` does NOT run
- MAVLink bytes pile up in the serial buffer (2048 bytes max) and are dropped
- The ESP32 stops sending GCS heartbeats
- ArduPilot's GCS failsafe (`FS_GCS_ENABLE=1`) triggers after **5 seconds without heartbeat** → initiates RTL mid-flight

**This is a known crash cause for outdoor flights** where phone WiFi drops momentarily.

**Fix:** Non-blocking state machine:
```cpp
void WiFiManager::handle() {
    if (isWiFiConnected()) { reconnectState = CONNECTED; return; }
    uint32_t now = millis();
    switch(reconnectState) {
        case IDLE:
            if (now - lastReconnectAttempt < reconnectInterval) return;
            lastReconnectAttempt = now;
            currentNetIdx = 0;
            WiFi.begin(savedNetworks[0].ssid.c_str(), ...);
            reconnectState = CONNECTING;
            connectStartMs = now;
            break;
        case CONNECTING:
            if (WiFi.status() == WL_CONNECTED) { reconnectState = CONNECTED; return; }
            if (now - connectStartMs > 10000) {
                currentNetIdx++;
                if (currentNetIdx >= savedNetworks.size()) {
                    reconnectState = IDLE; return;
                }
                WiFi.begin(savedNetworks[currentNetIdx].ssid.c_str(), ...);
                connectStartMs = now;
            }
            break;
    }
}
```

---

### BUG D — AGL Floor Defect (Acknowledged in Paper, Not Yet Fixed)

**File:** [`src/flight_controller.cpp`](file:///d:/btp_skylink/Skylink/src/flight_controller.cpp#L198)

Explained in Discrepancy 6 above. The fix is to reject commands when `relative_alt <= 0` rather than falling back to MSL altitude.

Additionally: the same `canExecuteGuidedMoveUnlocked()` is called by `moveBody()`, `yawRelative()`, and `gotoAlt()`. The fix covers all three.

---

### BUG E — `armSequence()` Arms 400ms After Mode Switch With No Confirmation

**File:** [`data/gcs.js`](file:///d:/btp_skylink/Skylink/data/gcs.js#L906-L918)

```js
function armSequence(selectId = 'mode-select') {
    sendCmd('SET_FLIGHT_MODE', { mode: 'GUIDED' });
    setTimeout(() => sendCmd('ARM_DRONE'), CFG.armModeDelayMs || 400);
}
```

**Problem:** The ARM command fires 400ms after the SET_FLIGHT_MODE command regardless of whether the mode switch succeeded. If GUIDED fails (e.g., GPS not ready — GUIDED requires GPS lock to enter), the drone arms in its current mode (STABILIZE). When the operator then clicks TAKEOFF, it's sent in STABILIZE mode — **ArduPilot executes this as a high-throttle pitch-up**, causing the drone to flip and crash.

The `autonomousTakeoff()` function does this correctly (waits for GUIDED confirmation via heartbeat). The `armSequence()` (arm-only button) does not.

**Fix:** Either:
1. Make `armSequence()` wait for GUIDED confirmation like `autonomousTakeoff()` does, or
2. Add a clear warning in the UI: "ARM sets GUIDED mode — ensure GPS 3D fix before arming"

---

### BUG F — No Periodic MAVLink Stream Re-request

**File:** [`src/flight_controller.cpp`](file:///d:/btp_skylink/Skylink/src/flight_controller.cpp#L695-L729)

`requestDataStreams()` is called once on MAVLink connection establishment. The constant `SKYLINK_MAVLINK_STREAM_REQUEST_MS = 10000` is defined but **never used**. ArduPilot stream rates reset after an autopilot reboot, and can be dropped under high CPU load.

**Fix (add to `handle()`):**
```cpp
if (mavlinkActive && !messageIntervalsSent) {
    requestDataStreams();
    lastStreamRequest = now;
    messageIntervalsSent = true;
}
// Re-request every 10 seconds to handle autopilot reboots
if (mavlinkActive && (now - lastStreamRequest > SKYLINK_MAVLINK_STREAM_REQUEST_MS)) {
    lastStreamRequest = now;
    requestDataStreams();
}
```

---

---

## Part 3 — High Priority Issues

---

### ISSUE 1 — MAVLink Timeout 5 Seconds is Too Long for Active Flight

**File:** [`include/skylink_config.h`](file:///d:/btp_skylink/Skylink/include/skylink_config.h#L29)

```cpp
#define SKYLINK_MAVLINK_TIMEOUT_MS 5000
```

During active flight, 5 seconds of silent UART is catastrophic — the drone flies its last command for 5 seconds before the ESP32 even acknowledges the problem. ArduPilot's own GCS failsafe would normally kick in (RTL after 5s of no heartbeat), but since Skylink *keeps sending heartbeats even with a dead UART link*, ArduPilot's failsafe never triggers.

**Fix:** Reduce to **2000ms**. Add logic: if MAVLink times out while armed, **immediately stop sending heartbeats** so ArduPilot's own GCS failsafe kicks in.

---

### ISSUE 2 — No Action When WS Client Count Drops to 0 During Flight

**File:** [`src/main.cpp`](file:///d:/btp_skylink/Skylink/src/main.cpp#L85-L91)

If the operator's device disconnects from WiFi mid-flight (phone battery dies, walks out of range), the WebSocket closes, `getWsClientCount()` → 0. The drone keeps flying. Nothing happens except ArduPilot's GCS failsafe (only if `FS_GCS_ENABLE=1` which is disabled in bench mode).

**Fix:**
```cpp
// In loop(), after webServerModule.sendHeartbeat():
static uint32_t lastClientSeen = 0;
if (webServerModule.getWsClientCount() > 0) {
    lastClientSeen = now;
} else if (flightController.getTelemetry().armed &&
           now - lastClientSeen > 10000) {
    // No GCS for 10s while armed — command loiter/RTL
    logger.warning("GCS link lost for 10s while armed — commanding RTL");
    flightController.returnToLaunch();
}
```

---

### ISSUE 3 — `GPS_RAW_INT` Overwrites EKF-Fused Position

**File:** [`src/flight_controller.cpp`](file:///d:/btp_skylink/Skylink/src/flight_controller.cpp#L810-L813)

```cpp
case MAVLINK_MSG_ID_GPS_RAW_INT: {
    if (gps.fix_type >= 2) {
        telemetry.latitude = gps.lat / 1e7;   // ← overwrites EKF position
        telemetry.longitude = gps.lon / 1e7;  // ← overwrites EKF position
    }
}
```

`GLOBAL_POSITION_INT` provides the **EKF-fused** position estimate (GPS + baro + IMU). `GPS_RAW_INT` provides raw GPS-only coordinates. These two messages arrive interleaved and race to update `telemetry.latitude/longitude`. The raw GPS should NOT overwrite the fused estimate.

**Fix:** Remove the lat/lon update from `GPS_RAW_INT` handler. Keep only sat count and fix type:
```cpp
case MAVLINK_MSG_ID_GPS_RAW_INT: {
    telemetry.gps_sats = gps.satellites_visible;
    telemetry.gps_fix = gps.fix_type;
    // DO NOT update latitude/longitude here — use GLOBAL_POSITION_INT only
}
```

---

### ISSUE 4 — Yaw Telemetry Sent as Signed (-180 to +180), Map Treats as Compass (0-360)

**File:** [`src/flight_controller.cpp`](file:///d:/btp_skylink/Skylink/src/flight_controller.cpp#L783)

`att.yaw * 57.2958f` produces **-180 to +180 degrees**. The map's `droneIcon(heading)` renders this as a CSS rotation. A negative value (northwest quadrant) rotates the drone icon the wrong way. `gcs_map.js` has `normalizeHeading()` but `updateFromTelemetry()` passes `d.yaw` directly to `droneIcon()` without normalizing.

**Fix in firmware:**
```cpp
float yawDeg = att.yaw * 57.2958f;
if (yawDeg < 0.0f) yawDeg += 360.0f;
telemetry.yaw = yawDeg;
```

---

### ISSUE 5 — MOVE_BODY_MAX_M = 200m is Dangerously Large

**File:** [`include/skylink_config.h`](file:///d:/btp_skylink/Skylink/include/skylink_config.h#L34)

```cpp
#define SKYLINK_MOVE_BODY_MAX_M 200.0f
```

This is 200 meters per body-frame move command. In ArduPilot GUIDED mode, `SET_POSITION_TARGET_LOCAL_NED` with a 200m offset will send the drone flying 200m in one command. During field testing at 5m altitude, this command could send the drone out of sight in seconds. The paper acknowledges this value but doesn't flag it as dangerous.

**Fix:** Reduce to **20.0f** for field use, or add a separate cap:
```cpp
#define SKYLINK_MOVE_BODY_MAX_M    20.0f   // horizontal
#define SKYLINK_MOVE_BODY_DOWN_MAX_M  3.0f  // vertical descent (extra conservative)
```

---

### ISSUE 6 — `emergencyStop()` Delivery Order Not Guaranteed

**File:** [`src/flight_controller.cpp`](file:///d:/btp_skylink/Skylink/src/flight_controller.cpp#L521-L527)

```cpp
void FlightController::emergencyStop() {
    sendRCOverride(1500, 1500, 1000, 1500);  // throttle to minimum
    sendArmDisarm(false, true);              // force disarm
}
```

Both packets are sent in the same mutex lock. While UART is sequential, the RC override is one packet and disarm is another. If ArduPilot processes disarm first (race in ArduPilot's own task scheduler), the motors cut power with whatever throttle they were at, which can cause abrupt drops from height.

**Fix:**
1. Send disarm first (more urgent), RC override second
2. Send disarm **twice** — ArduPilot sometimes ignores a single force-disarm in flight
3. After disarm, clear RC overrides (set all channels to `UINT16_MAX` = passthrough)

---

### ISSUE 7 — JSON Buffer (1,152 bytes) Can Silently Truncate Heartbeat

**File:** [`include/skylink_config.h`](file:///d:/btp_skylink/Skylink/include/skylink_config.h#L12)

The heartbeat JSON serializes ~25 fields + statustext array (up to 5 × 50 chars = 250 bytes of text) + JSON overhead. Estimated peak size: ~1,000–1,100 bytes, very close to the 1,152 byte limit. When `serializeJson()` exceeds the buffer, it truncates silently, `wsBroadcastJson()` returns `false`, and the GCS receives no update for that cycle.

During a critical flight phase (just armed, motors spinning), a missed heartbeat cycle means the GCS UI freezes for 200ms — operators may see this as a UI glitch and click a button again, causing duplicate commands.

**Fix:** Increase to **2048 bytes**. On ESP32 with 51 KB free heap, this is trivially affordable.

---

### ISSUE 8 — No Hardware Watchdog

There is no ESP32 watchdog timer configured. A FreeRTOS deadlock (e.g., mutex never released due to exception), a WiFi driver stall, or any other MCU hang will silently freeze the firmware. The UART stops. MAVLink goes silent. ArduPilot's GCS failsafe eventually triggers RTL — but only if `FS_GCS_ENABLE=1`, which may be disabled from bench mode.

**Fix:**
```cpp
// In setup():
esp_task_wdt_init(10, true);  // 10-second watchdog, panic on timeout
esp_task_wdt_add(NULL);       // register main loop task

// In loop():
esp_task_wdt_reset();  // pet the watchdog each loop
```

---

---

## Part 4 — Medium Priority Issues

---

### ISSUE 9 — GOTO_LATLON Auto-Switches to GUIDED Without Hard Precondition

**File:** [`src/flight_controller.cpp`](file:///d:/btp_skylink/Skylink/src/flight_controller.cpp#L367-L369)

```cpp
if (telemetry.flight_mode != COPTER_MODE_GUIDED) {
    setCopterMode(COPTER_MODE_GUIDED);  // silently switches mode
}
```

The paper correctly notes this (Table VIII footnote †). But the silent mode switch is dangerous: if the drone is in LOITER (hold position), the operator clicks "Fly here", GOTO_LATLON fires, firmware switches to GUIDED, and the drone immediately starts flying to the target. The operator may not have intended this sequence.

**Fix:** Either reject if not in GUIDED (require operator to explicitly switch), or show a confirmation dialog before sending.

---

### ISSUE 10 — `statusLineCount` Not Reset on Mutex Held Across `appendStatusTexts`

**File:** [`src/flight_controller.cpp`](file:///d:/btp_skylink/Skylink/src/flight_controller.cpp#L679-L693)

`appendStatusTexts()` acquires the mutex and reads `statusLines`. `pushStatusLine()` is called from `processMavlinkMessage()` which is called from `handle()` which also holds the mutex. This means `pushStatusLine()` is implicitly called with the mutex already held — which is correct, but undocumented and fragile.

Any future caller of `pushStatusLine()` from outside the mutex will cause a data race. The field `statusLineCount` and ring buffer pointers could be in a partially-updated state.

**Fix:** Add a mutex assert or clear comment: `// REQUIRES fcMutex to be held by caller`

---

### ISSUE 11 — StatusText Events Silently Dropped in `sendPendingFcEvents()`

**File:** [`src/web_server.cpp`](file:///d:/btp_skylink/Skylink/src/web_server.cpp#L317-L319)

```cpp
if (ev.type == FCEventType::StatusText) {
    continue;  // silent drop — these are forwarded via heartbeat statustext[] array instead
}
```

StatusText events are pushed to the event queue in `processMavlinkMessage()` but then silently dropped here. Fast-firing autopilot error messages (pre-arm failures, EKF errors) can arrive faster than the 5 Hz heartbeat cycle and fill the 6-entry event queue. If 6 errors arrive between heartbeat cycles, the 6th overwrites the 1st, and the user may never see the first error.

**Fix:** Either remove StatusText from the event queue (dead code cleanup) or increase the queue size and properly forward them as individual WS events.

---

### ISSUE 12 — `configManager` WiFi Credentials Are Dead Code

**File:** [`src/config_manager.cpp`](file:///d:/btp_skylink/Skylink/src/config_manager.cpp)

`saveWiFiCredentials()`, `getWiFiSSID()`, `getWiFiPassword()`, `hasWiFiCredentials()` are implemented and return values, but WiFi is loaded from `data/wifi_networks.json`. These Preferences-based functions are never called from WiFiManager. Either use them (load from Preferences as fallback when LittleFS fails) or remove them.

---

### ISSUE 13 — No Input Validation for GOTO_LATLON Coordinates in Firmware

**File:** [`src/web_server.cpp`](file:///d:/btp_skylink/Skylink/src/web_server.cpp#L102-L109)

If a WebSocket message arrives with `lat: 0, lon: 0` (JSON parsing default, or malformed message), the firmware calls `gotoLatLon(0.0, 0.0, 5.0)`. The haversine geofence check computes distance from home to (0,0) — thousands of km — and correctly rejects it. But the rejection is silent (no error event sent to GCS).

**Fix:** Add explicit coordinate validation:
```cpp
if (fabs(lat) < 1e-4 && fabs(lon) < 1e-4) {
    logger.warning("GOTO_LATLON rejected: null-island coordinates");
    giveMutex(); return;
}
```

---

### ISSUE 14 — Stream Rate Uniform 4 Hz — Attitude Should Be 10 Hz

**File:** [`src/flight_controller.cpp`](file:///d:/btp_skylink/Skylink/src/flight_controller.cpp#L92-L100)

All six stream groups are requested at 4 Hz. The ATTITUDE message (roll/pitch/yaw) drives the artificial horizon PFD. At 4 Hz, the PFD updates every 250ms — noticeably laggy during dynamic flight. Attitude should be at 10 Hz minimum for a useful flight display.

**Fix:** Request streams individually:
```cpp
// Attitude at 10 Hz
mavlink_msg_request_data_stream_pack(GCS_SYSID, GCS_COMPID, &msg, 1, 1,
    MAV_DATA_STREAM_EXTRA1, 10, 1);
sendMavlinkPacket(&msg);

// Position at 5 Hz
mavlink_msg_request_data_stream_pack(GCS_SYSID, GCS_COMPID, &msg, 1, 1,
    MAV_DATA_STREAM_POSITION, 5, 1);
sendMavlinkPacket(&msg);

// Battery/status at 2 Hz
mavlink_msg_request_data_stream_pack(GCS_SYSID, GCS_COMPID, &msg, 1, 1,
    MAV_DATA_STREAM_EXTRA2, 2, 1);
sendMavlinkPacket(&msg);
```

---

### ISSUE 15 — `logger.cpp` Uses Unsafe `sprintf`

**File:** [`src/logger.cpp`](file:///d:/btp_skylink/Skylink/src/logger.cpp#L21-L23)

```cpp
char timestamp[20];
sprintf(timestamp, "[%02lu:%02lu:%02lu]", hours % 24, minutes % 60, seconds % 60);
```

Use `snprintf(timestamp, sizeof(timestamp), ...)` to prevent buffer overflow on unexpected values.

---

---

## Part 5 — Low Priority / Code Quality

| # | Issue | File | Fix |
|---|---|---|---|
| L1 | `gotoAlt()` uses stale telemetry lat/lon for position hold | `flight_controller.cpp:441` | Consider `MAV_CMD_CONDITION_CHANGE_ALT` instead |
| L2 | `SKYLINK_MAVLINK_STREAM_REQUEST_MS` defined but never used | `skylink_config.h:28` | Implement periodic re-request (see BUG F) |
| L3 | `SKYLINK_CMD_RATE_LIMIT_PER_SEC` defined but never enforced | `skylink_config.h:41` | Implement a token-bucket rate limiter in firmware |
| L4 | Build version must be manually synced across 3 files | `skylink_config.h`, `gcs_config.js`, `skylink_build.json` | Automate with a PlatformIO pre-build script |
| L5 | WS client evict on new connection creates brief gap | `web_server.cpp:174` | Delay eviction by 200ms to allow new client to establish |
| L6 | `isSitlTcpConnected()` returns `true` always in HW mode | `flight_controller.cpp:551` | The paper notes this correctly; document the convention |
| L7 | Map default coords are SITL default (-35, 149) (Canberra, Australia) | `gcs_config.js:22-23` | Change to IIITDM Jabalpur coords (23.17, 79.96) |
| L8 | Google Fonts loaded from internet in `index.html` | `index.html:11` | Bundle fonts to LittleFS for offline field use |

---

---

## Part 6 — Complete Fix Priority Matrix

| Priority | Bug/Issue | Crash Risk | Fix Effort | Fix First? |
|---|---|---|---|---|
| 🔴 CRITICAL | AGL floor defect (relative_alt fallback) | **CRASH** | Low | **Yes** |
| 🔴 CRITICAL | TAKEOFF no armed/GUIDED/GPS check in firmware | **CRASH** | Low | **Yes** |
| 🔴 CRITICAL | WiFi reconnect blocks loop 30s (triggers RTL) | **CRASH** | High | **Yes** |
| 🔴 CRITICAL | Geofence split-brain (map=1000m, firmware=200m) | Misleading | Low | **Yes** |
| 🔴 HIGH | `autonomousTakeoff()` uses blocking `prompt()` | High | Medium | Yes |
| 🔴 HIGH | `armSequence()` arms without GUIDED confirmation | **CRASH** | Low | Yes |
| 🔴 HIGH | No periodic stream re-request | Data loss | Trivial | Yes |
| 🟠 HIGH | MAVLink timeout 5s too long | High | Trivial | Yes |
| 🟠 HIGH | No GCS failsafe on WS client disconnect during flight | High | Medium | Yes |
| 🟠 HIGH | GPS_RAW_INT overwrites EKF position | Medium | Low | Yes |
| 🟠 HIGH | Yaw sign convention bug on map | Low | Trivial | Yes |
| 🟠 HIGH | MOVE_BODY_MAX 200m dangerously large | Medium | Trivial | Yes |
| 🟠 HIGH | emergencyStop delivery order not guaranteed | Medium | Low | Yes |
| 🟠 HIGH | JSON buffer 1152 may truncate heartbeat | Medium | Trivial | Yes |
| 🟠 HIGH | No hardware watchdog | High | Low | Yes |
| 🟡 MED | GOTO_LATLON silent mode switch | Medium | Low | Soon |
| 🟡 MED | StatusText events silently dropped | Low | Low | Soon |
| 🟡 MED | AGL floor value mismatch (paper vs code) | Paper error | Trivial | Soon |
| 🟡 MED | Attitude stream rate 4Hz (should be 10Hz) | UX | Low | Soon |
| 🟡 MED | configManager WiFi creds are dead code | Code quality | Low | Later |
| 🟡 MED | logger.cpp unsafe sprintf | Low | Trivial | Later |
| 🟢 LOW | Map default coords point to Australia | UX | Trivial | Later |
| 🟢 LOW | Google Fonts requires internet | UX | Medium | Later |
| 🟢 LOW | Build version manual sync across 3 files | Process | Medium | Later |

---

---

## Part 7 — Skylink v3 Architecture Blueprint

> If you ever build the next version from scratch, here is what would make it **100× better** than the current Skylink.

---

### A. Replace the Blocking Arduino Loop with a Proper RTOS Multi-Task Design

**Current problem:** One Arduino loop handles WiFi, MAVLink, OTA, LED, time sync — all sequentially. A single blocking call (WiFi reconnect, OTA check) stalls everything.

**v3 design:**
```
Task 1 (Core 0, Priority HIGH):   MAVLink RX/TX — dedicated UART task with DMA
Task 2 (Core 0, Priority HIGH):   Safety monitor — heartbeat watchdog, arm/disarm state
Task 3 (Core 1, Priority MED):    WebSocket/HTTP server (already on Core 1 in AsyncWebServer)
Task 4 (Core 1, Priority LOW):    Logger, LED, NTP, OTA
```

Use `xQueueSend/Receive` for inter-task communication instead of the mutex pattern. The safety task can kill the WiFi reconnect attempt if armed — never block MAVLink for networking.

---

### B. ACK-Driven Command Sequencing (Not Time-Based)

**Current problem:** All critical sequences (ARM → TAKEOFF, MODE → ARM) rely on `setTimeout()` with fixed delays. This races with real FC timing.

**v3 design:** Every command waits for `COMMAND_ACK` before proceeding. Build a proper async command queue:

```
sendCmd(ARM) → await ACK(ARM_DISARM, ACCEPTED, timeout=5s)
             → sendCmd(TAKEOFF) → await ACK(TAKEOFF, ACCEPTED, timeout=5s)
             → begin altitude monitoring
```

On timeout: abort sequence, notify GCS, log reason. On rejection: parse result code, show human-readable reason.

---

### C. State Machine for Vehicle State (Not Heartbeat Polling)

**Current problem:** `flightUiState` is polled from periodic heartbeats with a stable-count requirement. This introduces 600ms latency minimum on all state changes.

**v3 design:** Maintain a full vehicle state machine in firmware:

```
States: UNKNOWN → DISARMED → ARMING → ARMED_GROUNDED → 
        TAKING_OFF → HOVERING → MANEUVERING → LANDING → LANDED
```

Transitions driven by ACK messages + heartbeat changes. Each state has allowed commands. The GCS sends commands; firmware validates against current state and rejects with reason. No more guessing from JS.

---

### D. Firmware-Side Command Validation for ALL Commands

**Current problem:** TAKEOFF, LAND, RTL, RC_OVERRIDE have zero firmware-side safety checks.

**v3 design:** Every command has a firmware-side precondition check:

```cpp
struct CommandRule {
    bool requireArmed;
    bool requireGuided;
    bool requireGPS3D;
    bool requireMinAGL;
    float maxParam1;
    float maxParam2;
};

static const CommandRule rules[NUM_COMMANDS] = {
    [CMD_TAKEOFF]  = {.requireArmed=true, .requireGuided=true, .requireGPS3D=true, .maxParam1=50.0f},
    [CMD_MOVE_BODY]= {.requireArmed=true, .requireGuided=true, .requireGPS3D=true, .requireMinAGL=true, .maxParam1=20.0f},
    ...
};
```

One validation function checks all commands against their rules. Browser is untrusted; firmware is the authority.

---

### E. Authenticated WebSocket Protocol

**Current problem:** Any device on the WiFi can send `{"type":"command","command":"ARM_DRONE"}` and arm the drone. Zero authentication.

**v3 design:**
1. On first connection, GCS sends a token (pre-shared key, set during setup)
2. Firmware verifies token in the HTTP WebSocket upgrade header
3. All subsequent commands include an HMAC-SHA256 signature
4. Commands without valid signature are dropped and the connection is closed

This can be implemented with the ESP32's hardware SHA accelerator — no performance cost.

---

### F. Non-Blocking WiFi with Intelligent Flight-Mode Reconnect

**Current problem:** WiFi drops, firmware blocks for 30s trying to reconnect, MAVLink dies.

**v3 design:**
1. WiFi reconnect is fully non-blocking (state machine, no `delay()`)
2. During reconnect while armed: DO NOT attempt reconnect (it would interfere with MAVLink)
3. Instead, switch to "disconnected armed" mode: continue sending GCS heartbeats to ArduPilot (so it doesn't trigger GCS failsafe), log the disconnect, attempt reconnect only after landing
4. When operator reconnects, show a "connection restored — vehicle in [mode]" banner

---

### G. Per-Message-ID Telemetry Rate Control

**Current problem:** All streams at 4 Hz via deprecated `REQUEST_DATA_STREAM`.

**v3 design:** Use `MAV_CMD_SET_MESSAGE_INTERVAL` (works on ArduCopter 4.x):
```
ATTITUDE:            50 Hz  (IMU display, smooth PFD)
GLOBAL_POSITION_INT: 10 Hz  (map update)
VFR_HUD:             10 Hz  (altitude/speed display)
GPS_RAW_INT:          2 Hz  (satellite count, fix type)
SYS_STATUS:           2 Hz  (battery)
HOME_POSITION:        1 Hz  (geofence center)
HEARTBEAT:            1 Hz  (armed/mode state)
```

Total MAVLink bandwidth at 115200 baud is ~11,520 bytes/sec. Even at 50 Hz ATTITUDE (~28 bytes each = 1400 bytes/sec), there is headroom for everything.

---

### H. Persistent Flight Log to LittleFS

**Current problem:** After a crash, you have no record of what happened. The serial log is gone. There's no way to know what the last command was, what the GPS fix was, or what error ArduPilot reported.

**v3 design:** Log to LittleFS in a circular file:
```
/logs/flight_YYYYMMDD_HHMMSS.log  (new file per arm cycle)
```
Each line: `timestamp, lat, lon, alt, mode, armed, battery, last_cmd, last_ack, statustext`

Accessible via OTA or a `/log` HTTP endpoint. Limit total log space to 500 KB (2 weeks of daily flights).

---

### I. Upgrade to ArduCopter 4.5+ (Critical for Stability)

**Current setup:** Pixhawk 2.4.8 with ArduCopter 3.6.8 (2018-era)

**ArduCopter 4.5 adds:**
- EKF3 with GPS glitch detection (auto-switches to barometer when GPS jumps)
- Pre-arm GPS accuracy check (refuses to arm if HDOP > 2.0)
- Motor vibration compensation via harmonic notch filter
- Significantly better LOITER hold in wind
- `MAV_CMD_SET_MESSAGE_INTERVAL` support (v3 needs this)
- Better COMMAND_ACK latency and reliability
- Improved EKF convergence time (less waiting for stable state)

The hardware cost of upgrading is zero — same Pixhawk, new firmware.

---

### J. Hardware Watchdog + Dual-MCU Safety Architecture

**Current problem:** If the ESP32 firmware crashes or deadlocks, nothing stops the drone.

**v3 design:**
- Enable ESP32 Task Watchdog Timer (10s timeout → reboot)
- Add a second MCU (ATtiny85, $0.50): monitors ESP32's "I'm alive" GPIO pin. If the pin stops pulsing for >3s, the ATtiny forces the Pixhawk TX line to ground, cutting ESP32's MAVLink output. ArduPilot detects GCS link loss → RTL. This is a hardware-level failsafe that survives ESP32 firmware crashes.

---

### K. Encrypted Communication + VPN for BVLOS

**Current problem:** WiFi GCS works only within ~50m indoors, ~100m outdoors.

**v3 design:**
1. Add **Waveshare SIM7600G-H** 4G LTE module to ESP32 (paper mentions this in future work)
2. ESP32 connects to a WireGuard VPN server on a cloud VM
3. Operator accesses GCS from anywhere via HTTPS/WSS through VPN
4. MAVLink encrypted end-to-end via WireGuard

This enables BVLOS operations from anywhere with cell coverage.

---

### L. MessagePack Instead of JSON for WebSocket Protocol

**Current problem:** Heartbeat JSON is ~800-1000 bytes at 5 Hz = ~5 KB/s upstream from ESP32.

**v3 design:** Use MessagePack binary encoding: ~40-50% size reduction. ArduinoJson 7 supports MessagePack natively. At 2.5 KB/s, even a congested 2.4 GHz WiFi channel handles this without dropped messages.

---

### M. Offline Map Tiles Cached in LittleFS

**Current problem:** Map requires internet to load OpenStreetMap tiles. At a field site with no internet, the map is blank.

**v3 design:** Pre-cache map tiles for a 5km radius at zoom levels 14-18 into LittleFS (or SD card). Estimated storage: ~50-100 MB for a useful area (requires SD card). The ESP32 serves tiles from SD like a local tile server — completely offline.

---

### N. Multi-Operator Support with Command Arbitration

**Current problem:** Single WebSocket client enforced. A second operator (safety pilot) can't observe without interrupting the primary operator.

**v3 design:**
- **Observer clients**: read-only telemetry, no commands (no limit on count)
- **Operator client**: exclusive command access (one at a time)
- Command lock: operator must "take control" with a button press; releases after 30s of inactivity
- Safety pilot: always has emergency disarm override regardless of command lock

---

### O. Natural Language Interface (MCP Integration)

The paper's own future work mentions this (Reference [8]: Ramos-Silva and Burke, 2026). In v3, integrate an MCP server that allows operators to say:

> *"Take off to 5 meters and hold position"*  
> *"Move 3 meters forward slowly"*  
> *"Return home"*

The LLM interprets intent → generates structured commands → sends through the existing WS API. The ESP32 firmware doesn't change; only the GCS frontend adds an MCP client.

---

## Summary — What To Fix Immediately Before Real Flight

1. **Fix the AGL floor defect** in `canExecuteGuidedMoveUnlocked()` — reject when `relative_alt <= 0`
2. **Add firmware guards to `takeoff()`** — check armed, GUIDED mode, GPS 3D fix, clamp altitude 1-50m
3. **Rewrite WiFi reconnect as non-blocking** state machine — eliminate the 30-second blocking loop
4. **Fix geofence split-brain** — synchronize `SKYLINK_GOTO_MAX_RADIUS_M` and `geofenceRadiusM` to same value
5. **Replace `prompt()` with modal dialog** in `autonomousTakeoff()`
6. **Add ACK-based TAKEOFF triggering** — wait for `COMMAND_ACK(ARM, ACCEPTED)` before sending TAKEOFF
7. **Add GCS failsafe** — RTL when WS client count = 0 for >10s while armed
8. **Add hardware watchdog** — 10-second timeout, panic on trigger
9. **Reduce MAVLink timeout to 2s** — and stop sending heartbeats on timeout so ArduPilot's failsafe activates
10. **Add periodic stream re-request** every 10 seconds using the already-defined constant

None of these require major refactoring. They are all targeted, small changes. Fix these ten items and the system is dramatically safer for real flight.

---

*Full codebase reviewed: 11 source files, 13 header files, 3 frontend JS files, 1 HTML file. Paper reviewed: all 9 sections + all 9 tables. Discrepancies identified by direct code comparison against paper claims.*
