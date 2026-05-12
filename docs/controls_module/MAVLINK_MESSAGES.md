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
