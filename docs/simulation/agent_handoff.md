# 🤝 Developer & AI Agent Handoff Retrospective

This handoff document details the technical lessons, breakthroughs, and architectural errors encountered during this integration session. It is designed to serve as a high-fidelity learning manual for future developers and AI agents working on the Skylink flight controller bridge.

---

## 🟢 The "Right Things" (What Went Well & Key Breakthroughs)

These architectural choices proved highly successful and should be preserved in all future iterations:

### 1. FreeRTOS Mutex Synchronization (Thread-Safety)
*   **The Problem**: The `AsyncWebServer` and WebSocket callbacks execute on Core 0 (LwIP network stack thread), while the main `loop()` and MAVLink socket read/write loops run on Core 1. Performing I/O operations (socket writes, state changes, or Serial prints) concurrently without synchronization resulted in garbled console lines, socket resets (`errno 104`), and hard ESP32 crashes.
*   **The Fix**: Introduced FreeRTOS Mutexes (`fcMutex` and `serialMutex`) to wrap all public methods in the flight controller and logging, ensuring 100% thread-safe concurrency.

### 2. Autopilot Data Stream Request (`REQUEST_DATA_STREAM`)
*   **The Problem**: ArduPilot does **not** stream telemetry packets (`SYS_STATUS`, `VFR_HUD`, `GPS_RAW_INT`, `ATTITUDE`) on secondary telemetry lines (Serial 2 / TCP 5763) by default. Without an explicit rate request, all telemetry values on the GCS dashboard remained stuck at zero/empty.
*   **The Fix**: Implemented the standard MAVLink `REQUEST_DATA_STREAM` packet at 4Hz upon connection and periodically (every 10 seconds) inside `handle()`, ensuring telemetry flows continuously.

### 3. Transition to Autonomous GUIDED Commands
*   **The Problem**: The initial dashboard used manual virtual-joystick pulse overrides (`RC_OVERRIDE` to $1700\,\text{ms}$) to simulate takeoff. Over high-latency WiFi / WebSockets, raw stick overrides are unstable, dangerous, and instantly trigger throttle failsafes in manual `STABILIZE` mode.
*   **The Fix**: Upgraded the entire system to support clean, professional autonomous MAVLink commands (`SET_MODE` to GUIDED/LAND/RTL, and `MAV_CMD_NAV_TAKEOFF`).

### 4. Dynamic GCS Host IP Auto-Capture
*   **The Fix**: Instead of hardcoding the laptop's IP, we captured `client->remoteIP()` inside the ESP32's WebSocket connection callback. This dynamically updates the SITL host IP transparently when the user connects their browser, bypassing all local IP configuration issues.

---

## 🔴 The "Wrong Things" (Pitfalls, Mistakes, & Gotchas)

These are the primary architectural bugs and debugging oversights encountered during this session. **Do not repeat these mistakes:**

### 1. The Failsafe / STABILIZE Mode Arming Assumption
*   **The Mistake**: Assuming the drone could be armed in manual `STABILIZE` mode and sit idle on the ground.
*   **The Lesson**: In manual modes, ArduPilot will instantly disarm itself if it does not receive a continuous stream of RC overrides or if throttle remains at zero. Always use **GUIDED** mode for autonomous or web-controlled arming and operations.

### 2. Overwriting the Target Dropdown with Current Telemetry
*   **The Mistake**: In the initial Guided mode dashboard design, the live telemetry parser blindly set `modeSelect.value = d.flight_mode`. 
*   **The Lesson**: Because the flight mode defaults to `0` (STABILIZE) before the first active telemetry packet is parsed, the dropdown was immediately reset to `STABILIZE`. When the user clicked Arm, it read `0` and armed in STABILIZE, leading to instant auto-disarms. **Keep Target Control Selectors isolated from live telemetry readouts.**

### 3. Forgetting the `uploadfs` target
*   **The Mistake**: Proposing the standard PlatformIO `upload` command without executing `uploadfs`.
*   **The Lesson**: `upload` only flashes the compiled C++ firmware. It does **not** upload files inside the `/data` folder to the ESP32's LittleFS filesystem. Always execute both targets (`pio run --target uploadfs --target upload`) when frontend assets change.

---

## 🧭 Recommendations for Future Agents

1.  **Bench Safety & Portability**: The `flight_controller` code contains a `-D SITL_MODE` build flag in `platformio.ini`. To transition from SITL to the physical Pixhawk 6C (TELEM2 serial lines), simply comment out this flag. The codebase will automatically reconfigure itself to communicate over hardware UART2 (`Serial2` / GPIO16 & GPIO17) instead of TCP sockets.
2.  **Maintain Mutex Locks**: Any future outbound commands added to `flight_controller.cpp` **must** acquire the `fcMutex` prior to calling `sendMavlinkPacket(...)` to maintain absolute thread safety.
