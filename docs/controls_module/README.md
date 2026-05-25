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
