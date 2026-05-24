# MAVLink Command Reference — GCS Upgrade

Mapping from dashboard actions to ArduPilot (Copter, GUIDED-centric).  
All commands require **GUIDED** unless noted. Firmware enforces mode + caps from [OPEN_DECISIONS.md](./OPEN_DECISIONS.md).

---

## Existing (MVP)

| UI action | WebSocket `command` | MAVLink |
|-----------|---------------------|---------|
| Set mode | `SET_FLIGHT_MODE` | `MAV_CMD_DO_SET_MODE` |
| Arm / Disarm | `ARM_DRONE` / `DISARM_DRONE` | `MAV_CMD_COMPONENT_ARM_DISARM` |
| Takeoff | `TAKEOFF` `{ altitude }` | `MAV_CMD_NAV_TAKEOFF` |
| Land / RTL | `LAND` / `RTL` | mode `LAND` / `RTL` |

---

## Phase 4 — Relative tap controls

Uses **body-offset** frame so “forward” is nose direction regardless of heading.

| UI action | WebSocket | MAVLink | Notes |
|-----------|-----------|---------|-------|
| Forward N m | `MOVE_BODY` `{ "x": N, "y": 0, "z": 0 }` | `SET_POSITION_TARGET_LOCAL_NED` | `MAV_FRAME_BODY_OFFSET_NED`, position mask |
| Back N m | `MOVE_BODY` `{ "x": -N, ... }` | same | cap \|N\| ≤ 20 m |
| Strafe left/right | `MOVE_BODY` `{ "y": ±N }` | same | |
| Up/down N m | `MOVE_BODY` `{ "z": -N }` | same | NED: z negative = up |
| Yaw right 90° | `YAW_RELATIVE` `{ "deg": 90 }` | `MAV_CMD_CONDITION_YAW` | relative, rate limited |
| Yaw left | `YAW_RELATIVE` `{ "deg": -90 }` | same | |

** Preconditions:** armed, GUIDED, altitude ≥ min AGL, GPS 3D fix.

**Do not use** continuous `RC_OVERRIDE` for these patterns.

---

## Phase 5 — Map tap commands

| UI action | WebSocket | MAVLink |
|-----------|-----------|---------|
| Click map “Fly here” | `GOTO_LATLON` `{ "lat", "lon", "alt" }` | `MAV_CMD_DO_REPOSITION` or `SET_POSITION_TARGET_GLOBAL_INT` |
| Loiter here | `SET_FLIGHT_MODE` `LOITER` + optional reposition | mode 5 + reposition |

Validate click distance from home ≤ geofence radius (UI + firmware).

---

## Phase 5 — Altitude & hold

| UI action | WebSocket | MAVLink |
|-----------|-----------|---------|
| Climb/descend to alt | `GOTO_ALT` `{ "alt": meters AMSL or relative }` | `MAV_CMD_DO_CHANGE_ALTITUDE` |
| Pause / hover | `SET_FLIGHT_MODE` `LOITER` | mode 5 |

---

## Phase 2+ — Telemetry (inbound parse)

| MAVLink message | Field usage |
|-----------------|-------------|
| `HEARTBEAT` | armed, `custom_mode` |
| `ATTITUDE` | roll, pitch, yaw |
| `GLOBAL_POSITION_INT` | lat, lon, alt, relative_alt, vx, vy, vz |
| `GPS_RAW_INT` | sats, fix_type |
| `SYS_STATUS` | battery |
| `VFR_HUD` | alt, groundspeed |
| `COMMAND_ACK` | success/fail for last command |
| `STATUSTEXT` | pilot messages, arm errors |
| `HOME_POSITION` | home lat/lon (optional) |

---

## GCS heartbeat (outbound, existing)

`HEARTBEAT` type `MAV_TYPE_GCS` at 1 Hz — required so ArduPilot accepts commands.

---

## Stream requests

Keep `REQUEST_DATA_STREAM` @ 4 Hz on connect + every 10 s.  
**Upgrade:** prefer `MAV_CMD_SET_MESSAGE_INTERVAL` for MAVLink2 per-message rates (cleaner on SERIAL2).

Suggested intervals (µs):

| Message ID | Interval (µs) | Rate |
|------------|---------------|------|
| ATTITUDE | 100000 | 10 Hz |
| GLOBAL_POSITION_INT | 200000 | 5 Hz |
| GPS_RAW_INT | 500000 | 2 Hz |

Implement in Phase 3 when map needs smooth motion.
