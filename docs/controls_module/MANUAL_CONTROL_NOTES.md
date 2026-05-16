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
