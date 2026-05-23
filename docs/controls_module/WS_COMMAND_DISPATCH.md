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
