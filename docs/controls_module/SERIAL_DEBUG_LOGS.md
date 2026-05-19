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
