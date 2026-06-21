# Phase 8 Real Hardware Readiness Checklist

This checklist is the minimum gate before moving Skylink v1 from SITL to real Pixhawk hardware.

Phase 5 authentication is deferred for local WiFi testing only. Do not expose Skylink to the public internet/cellular network until Phase 5 is complete.

## Absolutely required before props-on flight

### 1. Correct firmware target

- Flash `esp32dev_hw`, not `esp32dev_sitl`.
- Confirm dashboard build shows the current firmware/FS build and no FS mismatch.
- Confirm the simulation banner is not treated as proof of real hardware safety.

### 2. Props-off bench test

- Remove all props.
- Power Pixhawk safely through the power module.
- Connect ESP32 UART2 to Pixhawk TELEM2:
  - ESP32 RX2 GPIO16 receives Pixhawk TX
  - ESP32 TX2 GPIO17 sends to Pixhawk RX
  - common ground required
- Confirm dashboard receives real MAVLink heartbeat, attitude, GPS, battery, mode, and arm state.
- Confirm emergency disarm sends the force-disarm path.

### 3. Pixhawk/ArduPilot parameters

Verify in Mission Planner before flight:

- TELEM2 protocol and baud are MAVLink at 115200.
- Frame type, motor order, motor direction, accelerometer, compass, radio/failsafe, and ESC calibration are correct.
- RTL altitude, geofence, battery failsafe, EKF/GPS failsafe, and GCS failsafe behavior are known.
- Do not rely on Skylink as the only recovery path.

### 4. Conservative Skylink hardware limits

Hardware builds now use intentionally small first-flight limits:

- body move max: 2 m
- goto radius: 10 m
- goto/takeoff max altitude: 5 m
- minimum guided-action AGL: 1 m
- yaw max per command: 45°

Do not expand these until several boring low-altitude tests pass.

### 5. Local WiFi-only test boundary

Auth is deferred, so test only on a trusted private WiFi network.

- Do not connect Skylink command access to public internet.
- Do not use cellular/long-range internet control yet.
- Keep another recovery path available, preferably RC transmitter and/or a ready Mission Planner link.

### 6. Props-on first hover gate

Only after props-off tests pass:

- Use open area.
- Use low battery risk, good GPS, low wind.
- Start with very low altitude, 2 m or less if safe for the frame/ground effect.
- Keep movement steps at 0.5–1 m initially.
- Test only: arm, takeoff, hold, land, emergency disarm readiness.
- Review logs after every attempt before increasing scope.

## Deferred until after Phase 6 / Phase 8 smoke tests

### Deferred Phase 4 hardening

- bounded pending command table if overlapping ACK-capable commands become necessary;
- automated wrong-source MAVLink packet tests;
- automated ACK accepted/rejected/timeout tests.

### Deferred Phase 5 security/authentication

- authentication before WebSocket commands;
- protect or disable unauthenticated OTA;
- prevent sensitive config exposure;
- auth rate limiting and lockout;
- production credential model;
- internet/cellular secure deployment model.

