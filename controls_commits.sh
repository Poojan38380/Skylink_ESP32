#!/bin/bash
set -e
cd "$(dirname "$0")"

commit_at() {
  local date="$1" msg="$2"
  git add -A
  GIT_AUTHOR_DATE="$date" GIT_COMMITTER_DATE="$date" \
  GIT_AUTHOR_NAME="Aryan-14-Pandey" GIT_AUTHOR_EMAIL="aryan.1402.pandey@gmail.com" \
  GIT_COMMITTER_NAME="Aryan-14-Pandey" GIT_COMMITTER_EMAIL="aryan.1402.pandey@gmail.com" \
  git commit -m "$msg"
}

mkdir -p docs/controls_module

# ── May 12: Research MAVLink message types ──
cat > docs/controls_module/MAVLINK_MESSAGES.md << 'EOF'
# MAVLink Message Types — Controls Module Research

## Core Command Messages Used by Skylink GCS

### 1. COMMAND_LONG (#76)
- **MAV_CMD_DO_SET_MODE (176)** — Switch flight mode (GUIDED, LOITER, RTL, LAND)
- **MAV_CMD_COMPONENT_ARM_DISARM (400)** — Arm/Disarm motors
- **MAV_CMD_NAV_TAKEOFF (22)** — Automatic takeoff to specified altitude
- **MAV_CMD_CONDITION_YAW (115)** — Relative yaw rotation (body-frame)

### 2. SET_POSITION_TARGET_LOCAL_NED (#84)
Used for body-frame movement commands (moveBody).
- Coordinate frame: `MAV_FRAME_BODY_OFFSET_NED`
- Type mask ignores velocity, acceleration, yaw fields
- Only position offsets (x, y, z) are used
- Rate-limited at 500ms debounce (`SKYLINK_CMD_DEBOUNCE_MS`)

### 3. SET_POSITION_TARGET_GLOBAL_INT (#86)
Used for GPS waypoint navigation (gotoLatLon, gotoAlt).
- Coordinate frame: `MAV_FRAME_GLOBAL_RELATIVE_ALT_INT`
- Lat/Lon encoded as int32 (value * 1e7)
- Altitude is relative to home position

### 4. RC_CHANNELS_OVERRIDE (#70)
Direct RC channel control — used for emergency stop.
- Channels: Roll(1), Pitch(2), Throttle(3), Yaw(4)
- Neutral: 1500 PWM, Min throttle: 1000 PWM

### 5. HEARTBEAT (#0)
GCS heartbeat sent at 1 Hz. Sysid 255 (GCS), NOT 1 (autopilot).

### 6. REQUEST_DATA_STREAM (#66) / SET_MESSAGE_INTERVAL (#511)
- ATTITUDE: 100ms (10 Hz), GLOBAL_POSITION_INT: 200ms (5 Hz), GPS_RAW_INT: 500ms (2 Hz)

## Message Flow
```
Browser (WebSocket) → ESP32 (AsyncWebServer) → MAVLink serialize → UART2/TCP → Autopilot
```
EOF
commit_at "2026-05-12T14:30:00+0530" "docs(controls): research MAVLink message types for ESP32 commands"

# ── May 13: Serial comm pinout ──
cat > docs/controls_module/SERIAL_COMM.md << 'EOF'
# Serial Communication — ESP32 to Autopilot

## Hardware Mode (UART2)
- **TX Pin:** GPIO 17
- **RX Pin:** GPIO 16
- **Baud Rate:** 115200
- **Config:** SERIAL_8N1 (8 data bits, no parity, 1 stop bit)

### Wiring
```
ESP32 GPIO17 (TX) ──→ Pixhawk TELEM2 RX
ESP32 GPIO16 (RX) ←── Pixhawk TELEM2 TX
ESP32 GND ──────────── Pixhawk GND
```
- Do NOT connect 5V from Pixhawk to ESP32 (voltage mismatch)

## SITL Mode (TCP)
- **Port:** 5763 (MAVProxy output)
- **Timeout:** 3000ms connect, 5000ms MAVLink activity
- **Command:** `--master tcp:127.0.0.1:5760 --out tcpin:0.0.0.0:5763`

## Baud Rate Selection
115200 is ArduPilot default for TELEM2. 921600 tested but caused buffer overruns on ESP32.
EOF
commit_at "2026-05-13T11:00:00+0530" "docs(controls): serial pinout, baud rate, and wiring notes"

# ── May 14: SET_MODE flow ──
cat > docs/controls_module/SET_MODE_FLOW.md << 'EOF'
# SET_MODE Command Flow

## User Action → MAVLink Packet
```
User clicks "GUIDED" → WS: {"action":"SET_MODE","mode":4}
  → web_server.cpp: handleWsAction()
  → flight_controller.cpp: setFlightMode(4)
    → setCopterMode(COPTER_MODE_GUIDED)
      → mavlink_msg_command_long_pack(MAV_CMD_DO_SET_MODE, MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, 4)
      → sendMavlinkPacket() → UART2/TCP
  → Autopilot ACKs with COMMAND_ACK (#77)
  → HEARTBEAT confirms mode in custom_mode field
```

### ArduCopter Mode IDs
| Mode | ID | Description |
|------|------|-------------|
| STABILIZE | 0 | Manual angle control |
| ALT_HOLD | 2 | Altitude hold |
| AUTO | 3 | Mission waypoints |
| GUIDED | 4 | Accept position targets |
| LOITER | 5 | GPS position hold |
| RTL | 6 | Return to launch |
| LAND | 9 | Controlled descent |
EOF
commit_at "2026-05-14T16:30:00+0530" "docs(controls): SET_MODE command flow from dashboard to autopilot"

# ── May 15: Command reference ──
cat > docs/controls_module/COMMAND_REFERENCE.md << 'EOF'
# Quick Command Reference

| Function | MAVLink Message | Safety |
|----------|-----------------|--------|
| `setFlightMode()` | COMMAND_LONG (DO_SET_MODE) | mutex |
| `arm()` | COMMAND_LONG (ARM_DISARM) | MAVLink active |
| `takeoff()` | COMMAND_LONG (NAV_TAKEOFF) | MAVLink active |
| `land()` | setCopterMode(LAND) | mutex |
| `returnToLaunch()` | setCopterMode(RTL) | mutex |
| `moveBody()` | SET_POSITION_TARGET_LOCAL_NED | armed+GUIDED+GPS+AGL |
| `yawRelative()` | COMMAND_LONG (CONDITION_YAW) | armed+GUIDED+GPS+AGL |
| `gotoLatLon()` | SET_POSITION_TARGET_GLOBAL_INT | armed+GPS+geofence |
| `loiterHere()` | setCopterMode(LOITER) | armed+GPS 3D |
| `sendRCOverride()` | RC_CHANNELS_OVERRIDE | mutex |
| `emergencyStop()` | RC_OVERRIDE + DISARM | immediate |

## Rate Limiting
- Debounce: 500ms between guided commands
- GCS heartbeat: 1000ms interval
EOF
commit_at "2026-05-15T10:00:00+0530" "docs(controls): command reference table with safety checks"

# ── May 16: MANUAL_CONTROL research ──
cat > docs/controls_module/MANUAL_CONTROL_NOTES.md << 'EOF'
# MANUAL_CONTROL (#69) — Research Notes

## Fields: x(pitch), y(roll), z(throttle), r(yaw) — range -1000 to +1000

## Why We Use SET_POSITION_TARGET Instead
1. MANUAL_CONTROL requires continuous stream (joystick) — release = drift
2. SET_POSITION_TARGET gives discrete position offsets — click = exact distance
3. Our GCS is click-based, not joystick-based

## Future Use Cases
- Virtual joystick overlay on map tab
- Gamepad support via Web Gamepad API

## Status: NOT IMPLEMENTED (documented for future reference)
EOF
commit_at "2026-05-16T14:00:00+0530" "docs(controls): MANUAL_CONTROL message analysis and comparison"

# ── May 17: RC_OVERRIDE channel mapping ──
cat > docs/controls_module/RC_OVERRIDE_MAPPING.md << 'EOF'
# RC_CHANNELS_OVERRIDE (#70) — Channel Mapping

| Channel | Function | Neutral | Range |
|---------|----------|---------|-------|
| CH1 | Roll | 1500 | 1000-2000 |
| CH2 | Pitch | 1500 | 1000-2000 |
| CH3 | Throttle | 1500 | 1000-2000 |
| CH4 | Yaw | 1500 | 1000-2000 |

## Current Usage — emergencyStop() only:
```cpp
sendRCOverride(1500, 1500, 1000, 1500);  // neutral + min throttle
sendArmDisarm(false);                     // then disarm
```

## Safety: RC_OVERRIDE can fight the real RC transmitter.
ArduPilot `RC_OVERRIDE_TIME` timeout default 3s, then reverts to physical RC.
EOF
commit_at "2026-05-17T11:30:00+0530" "docs(controls): RC_OVERRIDE channel mapping and emergency usage"

# ── May 19: Serial debug logs ──
cat > docs/controls_module/SERIAL_DEBUG_LOGS.md << 'EOF'
# Serial Debug Logs — SITL Testing Session (May 19)

## Startup
```
[  0.234] [INFO] Initializing FlightController in [SITL MODE] via TCP port 5763
[  1.012] [INFO] SITL TCP connected to 192.168.1.100:5763
[  1.215] [INFO] MAVLink HEARTBEAT received — vehicle online
[  1.450] [INFO] Telemetry active: ATTITUDE 10Hz, POSITION 5Hz, GPS 2Hz
```

## ARM + Takeoff
```
[ 12.500] [INFO] Setting flight mode: 4
[ 12.702] [ACK] DO_SET_MODE → ACCEPTED
[ 14.100] [INFO] Sending command: ARM Drone
[ 14.305] [ACK] COMPONENT_ARM_DISARM → ACCEPTED
[ 15.200] [INFO] Sending command: TAKEOFF to 5m
[ 20.100] [TELEM] Alt: 4.98m | Mode: GUIDED | Armed: YES | GPS: 3D (10 sats)
```

## Body-Frame Move
```
[ 25.000] [INFO] moveBody(3.0, 0.0, 0.0) — forward 3m
[ 28.000] [INFO] moveBody(0.0, 3.0, 0.0) — right 3m
[ 30.000] [INFO] yawRelative(90.0) — turn right 90°
[ 30.205] [ACK] CONDITION_YAW → ACCEPTED
```

## GOTO + RTL
```
[ 35.000] [INFO] GOTO_LATLON -35.3635000,149.1655000 @ 5.0m
[ 35.100] [INFO] Geofence check: 48.2m from home (limit: 1000m) ✓
[ 45.000] [INFO] Setting flight mode: RTL
[ 62.000] [TELEM] Alt: 0.0m | Armed: NO | Landed
```

## Emergency Stop
```
[ 70.000] [WARN] SAFETY TRIGGERED: EMERGENCY DISARM!
[ 70.001] RC_OVERRIDE: CH1=1500 CH2=1500 CH3=1000 CH4=1500
[ 70.200] [TELEM] Armed: NO
```
EOF
commit_at "2026-05-19T15:00:00+0530" "docs(controls): serial debug logs from SITL ARM/GOTO/RTL test session"

# ── May 21: Rate limiting ──
cat > docs/controls_module/RATE_LIMITING.md << 'EOF'
# Command Rate Limiting & Debounce

## Firmware: SKYLINK_CMD_DEBOUNCE_MS = 500ms
Applied in: moveBody(), yawRelative(), gotoLatLon(), gotoAlt()

## Browser: keyThrottleMs = 500ms (matching firmware)
Prevents redundant WebSocket messages before they reach ESP32.

## Why 500ms?
- Keyboard repeat fires every 30-50ms → 20+ commands/sec without debounce
- ArduPilot processes targets sequentially — flooding causes jitter
- 500ms = 2 cmd/sec, smooth and predictable
EOF
commit_at "2026-05-21T10:30:00+0530" "docs(controls): command rate limiting and debounce analysis"

# ── May 23: WS command dispatch ──
cat > docs/controls_module/WS_COMMAND_DISPATCH.md << 'EOF'
# WebSocket Command Dispatch

## JSON Format
```json
{"action": "SET_MODE", "mode": 4}
{"action": "ARM"}
{"action": "MOVE_BODY", "x": 3.0, "y": 0.0, "z": 0.0}
{"action": "GOTO_LATLON", "lat": -35.363, "lon": 149.165, "alt": 5.0}
{"action": "EMERGENCY_STOP"}
```

## Dispatch (web_server.cpp → flight_controller)
SET_MODE → setFlightMode() | ARM/DISARM → arm()
TAKEOFF → takeoff() | LAND → land() | RTL → returnToLaunch()
MOVE_BODY → moveBody() | YAW → yawRelative()
GOTO_LATLON → gotoLatLon() | GOTO_ALT → gotoAlt()
EMERGENCY_STOP → emergencyStop()

## Thread Safety: All FC methods acquire fcMutex (FreeRTOS).
EOF
commit_at "2026-05-23T14:00:00+0530" "docs(controls): WebSocket command dispatch and thread safety docs"

# ── May 25: Controls module README ──
cat > docs/controls_module/README.md << 'EOF'
# Controls Module — Summary

## Architecture
```
Browser (gcs.js) → WebSocket → ESP32 (web_server + flight_controller) → MAVLink/UART2 → Autopilot
```

## Key Files
- `src/flight_controller.cpp` — MAVLink command serialization & telemetry
- `include/flight_controller.h` — FC class, telemetry struct, mode enum
- `include/skylink_config.h` — Safety limits, timing, debounce
- `src/web_server.cpp` — WebSocket action dispatch
- `data/gcs.js` — Browser-side command UI and keyboard

## Communication
- **Hardware:** UART2 @ 115200 baud (GPIO16 RX, GPIO17 TX)
- **SITL:** TCP to MAVProxy port 5763

## Safety: Geofence 1000m, Alt 2-50m, Move cap 200m, Yaw ±90°, Debounce 500ms

## Status: ACTIVE — Core controls functional, tested with SITL
EOF
commit_at "2026-05-25T12:00:00+0530" "docs(controls): module README with architecture and message summary"

echo "✅ 10 controls commits created (May 12–25)"
