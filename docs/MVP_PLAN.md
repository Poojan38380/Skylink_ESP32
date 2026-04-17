# ESP32 Internet-Based Remote Control MVP (Phase 1 – Local Only)

## 1. Overview

This document defines a minimum viable product (MVP) for demonstrating **internet-based UAV remote control concepts** using only an **ESP32 development board** and its built-in LED / GPIO.
The MVP is intentionally **local-only** (within a single Wi-Fi network) but designed so the architecture can later be moved into the cloud to enable control from anywhere (Phase 2), and eventually mapped to real drone motion via a flight controller (Phase 3).

This document is written for coding agents who will implement firmware and a simple web client.

## 2. Goals and Non-Goals

### 2.1 Goals

- Demonstrate **end-to-end remote control** of an ESP32 GPIO via a **browser UI over IP**.
- Use a **persistent, low-latency channel** (WebSocket or equivalent) between browser and ESP32.
- Show **bidirectional communication**:
  - Browser → ESP32: control commands (e.g., LED ON/OFF).
  - ESP32 → Browser: state updates (e.g., LED state, heartbeat).
- Make the design **extensible** so the same message schema and logic can later be:
  - Routed through a **cloud backend** (Phase 2).
  - Reused to send **UAV control commands** to a flight controller (Phase 3).

### 2.2 Non-Goals (for this MVP)

- No external sensors, motors, or RC receivers.
- No real drone hardware or flight controller integration yet.
- No cloud/VPS deployment or NAT traversal (local Wi-Fi only).
- No user authentication or advanced security (only basic safety like CORS and simple tokens if needed).

## 3. Personas and User Stories

### 3.1 Personas

- **Student Developer (Primary)**: Wants a clean, working demo plus codebase to extend later toward cloud control and UAV integration.
- **Professor / Reviewer**: Wants to see a convincing, technically sound MVP that clearly demonstrates: 
  - Real-time control over IP.
  - A clear, realistic path to cloud and drone integration.

### 3.2 Core User Stories

1. **US-1: Local Remote Control**  
   As a student, I want to **open a web page in my browser and toggle the ESP32 LED in real time**, so I can demonstrate basic internet-like remote control behavior.

2. **US-2: State Feedback**  
   As a student, I want the **web page to show the current LED state and connection status**, so the professor can see that communication is truly bidirectional.

3. **US-3: Extensibility**  
   As a student, I want the **protocol and architecture to be easily pluggable into a future cloud backend and drone flight controller**, so this MVP becomes the first step of an “internet-based UAV control” system.

## 4. System Architecture (Phase 1 – Option 3C)

### 4.1 High-Level Architecture

Components:

- **ESP32 Firmware**
  - Connects to an existing Wi-Fi network (station mode) or creates its own AP (configurable).
  - Hosts an HTTP server and a WebSocket endpoint.
  - Controls the built-in LED / a chosen GPIO.
  - Manages an in-memory state (LED state, last command, connection status, heartbeat).

- **Browser Web Client**
  - Static HTML/JS/CSS, served directly from ESP32 HTTP server.
  - Opens a WebSocket connection back to ESP32.
  - Sends control commands from buttons / switches in the UI.
  - Displays real-time state updates from the ESP32.

- **Network**
  - Single Wi-Fi network (same subnet) connecting browser device and ESP32.
  - No internet or router configuration is strictly required for this MVP.

### 4.2 Communication Pattern

- Use a **WebSocket connection** between browser and ESP32 for:
  - Sending JSON-encoded control messages from browser to ESP32.
  - Streaming JSON-encoded state updates from ESP32 to browser.
- HTTP is used only for:
  - Serving the static web page and JS bundle.
  - Optionally a simple REST endpoint for health-check.

## 5. Detailed Functional Requirements

### 5.1 Wi-Fi Configuration

- ESP32 shall support two modes (configurable at compile-time or via simple config header):
  - **Mode A – Station**: Connect to an existing Wi-Fi network with SSID/PASSWORD defined in code.
  - **Mode B – Access Point**: Create its own Wi-Fi AP (e.g., `ESP32-UAV-MVP`) with a simple password.
- For the MVP, a compile-time configuration is sufficient; no dynamic config UI is required.

### 5.2 HTTP Server

- ESP32 shall host a minimal HTTP server on port 80 (or 8080 if easier).
- Endpoints:
  - `GET /` → Serves `index.html` containing the web UI.
  - `GET /app.js` (or similar) → Serves client-side JS file.
  - Optionally: `GET /health` → Returns basic JSON `{ "status": "ok" }`.

### 5.3 WebSocket Endpoint

- ESP32 shall expose a WebSocket endpoint, e.g. `ws://<esp32_ip>/ws`.
- Browser client connects on page load and maintains a persistent connection.
- If connection drops, browser should auto-retry with exponential backoff.

### 5.4 Message Schema (JSON over WebSocket)

#### 5.4.1 Browser → ESP32 (Commands)

Message format:

```json
{
  "type": "command",
  "command": "LED_SET",
  "value": true
}
```

Supported commands (Phase 1):

- `LED_SET`  
  - `value`: boolean – `true` = ON, `false` = OFF.

- `LED_TOGGLE`  
  - No extra fields required; ESP32 toggles LED state.

- `PING`  
  - No extra fields; ESP32 replies with `PONG` message.

Any unknown command should result in an error response but not crash the firmware.

#### 5.4.2 ESP32 → Browser (State / Events)

Generic message format:

```json
{
  "type": "event",
  "event": "LED_STATE",
  "value": true,
  "timestamp": 1710000000
}
```

Supported events:

- `LED_STATE`  
  - `value`: boolean – current LED state (ON/OFF).  
  - Emitted:
    - On every successful LED command.
    - On browser initial connection (to sync UI).

- `PONG`  
  - Response to `PING` command.

- `HEARTBEAT`  
  - Periodic message (e.g., every 2–5 seconds) indicating the ESP32 is alive.  
  - Payload may include additional fields later (e.g., fake telemetry like `altitude`, `battery`).

- `ERROR`  
  - `message`: short string describing what went wrong.

Timestamps can be simple `millis()` or `esp_timer_get_time()`; strict wall-clock accuracy is not required.

### 5.5 LED / GPIO Control

- Use the on-board LED (e.g., GPIO 2 or the board’s default LED pin) as the **primary output**.
- ESP32 shall:
  - Initialize the LED GPIO as output.
  - Update LED state immediately upon receiving a valid `LED_SET` or `LED_TOGGLE` command.
  - Maintain internal variable `led_state` (boolean) and include it in event messages.

### 5.6 Browser UI Requirements

- Single-page web app layout (minimal but clear):
  - **Connection status indicator** (Connected / Disconnected).
  - **LED state indicator** (text + colored icon or simple circle).
  - **Buttons**:
    - `LED ON`
    - `LED OFF`
    - `TOGGLE`
  - Optionally: a simple log area showing recent messages.

- Behavior:
  - On page load: open WebSocket, show “Connecting…”, then “Connected” or “Error”.
  - On button press: send corresponding command JSON to ESP32.
  - On `LED_STATE` event: update UI indicator to ON/OFF.
  - On connection loss: show “Disconnected” and auto-retry.

### 5.7 Error Handling

- ESP32 must avoid crashes on malformed JSON or unknown commands:
  - Log error on serial.
  - Optionally send `ERROR` event to browser.
- Browser should handle unexpected messages gracefully (ignore unknown `type` / `event`).

## 6. Non-Functional Requirements

- **Reliability**: ESP32 should run continuously for several hours without firmware crashes.
- **Responsiveness**: End-to-end command-to-LED latency should feel “instant” to a human (< 200 ms in typical LAN conditions).
- **Simplicity**: Code should be easy to read and extend; minimal dependencies beyond standard ESP32 libraries (Arduino core or ESP-IDF).
- **Observability**:
  - Serial logs for debugging (connection events, commands received, errors).
  - Optional simple counters (e.g., number of messages processed).

## 7. Implementation Notes for Coding Agents

- Platform: ESP32 (DevKit) using either **Arduino Core** or **ESP-IDF**. Choose whichever is most straightforward for WebSocket + HTTP server support.
- For Arduino Core, you may use well-known libraries (e.g., `ESPAsyncWebServer`, `AsyncTCP`) if available in this environment.
- Ensure code is modular:
  - `wifi_setup` module.
  - `http_server` module (serving static content).
  - `websocket_handler` module (JSON message handling).
  - `led_controller` module (GPIO state).
- Provide clear build and flash instructions in comments or a separate short section.

## 8. Phase 2: Upgrade Path to Option 3A (Cloud Backend)

This MVP is intentionally local-only, but the protocol and architecture are chosen so they can be moved to the cloud with minimal changes.

### 8.1 Conceptual Changes

- **Today (Phase 1)**:
  - Browser ↔ WebSocket ↔ ESP32 (direct, same LAN).

- **Phase 2 (Option 3A)**:
  - ESP32 becomes a **WebSocket (or MQTT/HTTP) client** that connects out to a **cloud backend**.
  - Browser connects to the **cloud backend** (via HTTPS/WebSocket), not directly to ESP32.
  - Cloud backend forwards JSON messages between browser and ESP32, possibly for many devices.

### 8.2 Reuse of Message Schema

- Keep the JSON message format identical:
  - Browser → Cloud → ESP32: same `type: "command"`, `command`, `value` fields.
  - ESP32 → Cloud → Browser: same `type: "event"`, `event`, `value` fields.
- Only the **transport and routing** layers change:
  - Instead of a single WebSocket directly between browser and ESP32, there are two hops via the cloud.

### 8.3 Minimal Required Changes for Phase 2

- ESP32:
  - Replace WebSocket server with a **WebSocket/MQTT client** that dials out to a known URL and authenticates.
- Cloud Backend:
  - Provide WebSocket endpoint for browsers.
  - Maintain per-device sessions.
  - Forward messages between browser sessions and corresponding ESP32 sessions.
- Browser:
  - Change WebSocket URL from `ws://<esp32_ip>/ws` to `wss://<cloud_host>/ws` (or similar).

## 9. Phase 3: Mapping to Real Drone Motion

Ultimately, this ESP32 communication pattern should be adapted to control a real UAV.

### 9.1 Integration with Flight Controller

- Connect ESP32 to a flight controller (FC) via **UART (serial)**.
- ESP32 acts as a bridge between **network messages** and **flight controller commands**.
- Two main strategies:

1. **MAVLink-based Integration**:
   - Use MAVLink protocol over UART between ESP32 and FC.
   - ESP32 translates JSON commands (e.g., `SET_ATTITUDE`, `SET_VELOCITY`, `ARM`, `TAKEOFF`) into appropriate MAVLink messages.
   - ESP32 forwards MAVLink telemetry back to browser/cloud as JSON `event` messages.

2. **RC Channel Emulation**:
   - ESP32 outputs PWM/PPM/SBUS-like signals or uses a dedicated RC input protocol supported by the FC.
   - Network commands modify virtual stick positions (roll/pitch/yaw/throttle) or flight mode switches.

### 9.2 Control Modes to Aim For

- **Supervised Autonomy (Recommended)**:
  - Use network link mainly for high-level commands: waypoints, modes (e.g., AUTO, LOITER), simple velocity/altitude setpoints.
  - Let FC handle stabilization, failsafe, and low-level control loops.

- **Full Teleoperation (Advanced)**:
  - High-rate network commands directly map to stick inputs.
  - Requires strong, low-latency, reliable internet (likely via 4G/5G and a robust transport like QUIC or WebRTC data channels).

### 9.3 Safety and Failsafes

- FC must be configured with robust failsafes:
  - Return-to-Home or Land on link loss or low battery.
  - Geofencing and altitude limits.
- ESP32 should not be a single point of catastrophic failure:
  - On loss of connection, FC should revert to safe autonomous behavior.

## 10. Summary

This MVP defines a **local-only, browser-to-ESP32 real-time control system** using WebSockets and JSON messages, with the built-in LED serving as a stand-in for UAV actuators.
The architecture and message schema are deliberately chosen so that, with minimal changes, they can be:

- Upgraded to a **cloud-based control system** (ESP32 and browser both talking to a backend server).
- Extended to **control real UAV motion** by bridging network commands to a flight controller via MAVLink or RC channel emulation.

Coding agents should implement Phase 1 exactly as described, keeping modules clean and message formats stable to enable smooth evolution into Phase 2 and Phase 3.