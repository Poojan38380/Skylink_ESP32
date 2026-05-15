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
