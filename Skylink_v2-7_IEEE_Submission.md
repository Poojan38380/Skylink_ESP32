# Skylink: An ESP32-Hosted, Browser-Based Ground Control Station for ArduPilot UAVs with Dual SITL/Hardware Architecture, Firmware Deployment Verification, and Preliminary Safety Layer

**Authors:** Aryan Pandey, Aditya Garg, Karan Agarwal, Poojan Goyani
**Affiliation:** Department of Computer Science and Engineering, PDPM Indian Institute of Information Technology, Design & Manufacturing (IIITDM), Jabalpur, India
**Supervisor:** Dr. Akshay Pandey

**Index Terms:** Ground control station; ESP32; MAVLink; ArduPilot; software-in-the-loop simulation; embedded systems; hardware implementation; UAV

---

## Abstract

Ground control stations (GCS) for ArduPilot-based unmanned aerial vehicles are typically hosted on laptop-class platforms requiring at least 512 MB of RAM. This paper presents **Skylink**, an ESP32-hosted, browser-based GCS prototype that serves a complete web interface over Wi-Fi, parses MAVLink v2 telemetry, and dispatches fifteen flight commands on a resource-constrained microcontroller (240 MHz dual-core Xtensa LX6, 327 KB SRAM) running FreeRTOS and the Arduino framework.

Skylink integrates:
- A MAVLink v2 parser and command dispatcher
- An asynchronous WebSocket server delivering telemetry at 5 Hz
- A Primary Flight Display (PFD) with artificial horizon and altitude tape
- A Leaflet.js interactive map with live drone position tracking and click-to-fly navigation
- A preflight safety checklist
- A preliminary firmware-level command-filtering layer for altitude, yaw, geofence, and command-rate constraints — **with the MOVE_BODY altitude-floor defect explicitly treated as unresolved pending correction and retesting**

A compile-time dual-mode design supports both ArduPilot SITL simulation (TCP) and physical Pixhawk hardware (UART):
- The **SITL path is fully verified**.
- **Hardware firmware has been deployed to and confirmed operational** on a physical ESP32-WROOM-32 board, with Wi-Fi association, LittleFS-served dashboard, WebSocket communication, LED state machine, and build-version integrity all confirmed on-device.
- **UART/Pixhawk communication and controlled flight testing remain as the immediate next steps.**

Five development phases with explicit acceptance criteria were completed and SITL-verified. A minimum runtime free heap of 51 KB was observed under peak SITL load, and the application binary occupies 860 KB of the 1,310 KB application partition.

> To the authors' knowledge, within the surveyed literature, this is the first combination of a self-hosted browser GCS, a firmware-level command-filtering layer, and a dual SITL/hardware compile-time design on a single ESP32-class device.

---

## I. Introduction

The ArduPilot ecosystem supports a wide range of deployed UAVs across agricultural, infrastructure-inspection, and research domains [1]. Standard operator interfaces — Mission Planner [2] and QGroundControl [3] — require dedicated laptops/desktops with graphical OSes, limiting field deployability and imposing non-trivial cost at scale.

Web-based GCS architectures decouple operators from dedicated hardware: a browser on any network-connected device can serve as the pilot interface, with protocol logic executing on a companion computer.

- **Burke [4]**: browser-accessible GCS on a Raspberry Pi companion computer (512 MB+ RAM), real-flight validated command and telemetry.
- **Atwal [5]**: Vue.js/MQTT web application backed by a dedicated PC, replacing a desktop tool.
- **DroneBridge for ESP32 [7]**: shows ESP32 (327 KB SRAM, FreeRTOS, no general-purpose OS) can perform reliable MAVLink relay over Wi-Fi — but only as a transparent serial-to-Wi-Fi bridge with **no self-hosted dashboard, no command semantics, no safety filtering**.

**Gap closed by Skylink:** combining on a single ESP32-class device — a self-hosted single-page browser application, a full WebSocket command/telemetry layer, a MAVLink parser/dispatcher, a geofenced interactive map, a preliminary firmware-level command-filtering layer, and a dual compile-time SITL/UART transport design. No external computer, cloud subscription, or backend service is required.

Three-tier design (Fig. 2): the ESP32 simultaneously serves as web server, WebSocket hub, MAVLink parser, and flight command gateway. Development followed a **SITL-first methodology**: all command paths validated in ArduCopter SITL before hardware assembly; hardware-mode firmware subsequently deployed to and confirmed operational on a physical ESP32-WROOM-32 board.

### Contributions

1. An ESP32-hosted, browser-based GCS prototype integrating web-interface serving, WebSocket telemetry, and MAVLink command/telemetry handling on a resource-constrained microcontroller — first to combine self-hosted browser, preliminary command-filtering layer, dual SITL/hardware compile-time design, and command dispatch on an ESP32-class device (per surveyed literature), with firmware deployment to physical hardware confirmed.
2. A compile-time dual-mode transport abstraction (SITL TCP / UART hardware) enabling the complete GCS firmware to be validated against ArduPilot SITL before any physical hardware is assembled. UART pathway implemented in hardware-ready firmware and deployed to a physical ESP32-WROOM-32 board; Pixhawk bench/flight validation is the immediate next step.
3. A preliminary firmware-level command-filtering layer enforcing altitude floors, geofence radii, yaw limits, and command-rate limits independently of autopilot parameters. **The AGL floor for MOVE_BODY has a known defect (Section VII-D), scheduled for correction before hardware flight.**
4. A five-phase, phase-gated development methodology for embedded GCS systems with explicit acceptance criteria, demonstrated and SITL-verified across all five phases (Table V), with hardware firmware subsequently deployed to and confirmed operational on a physical ESP32 board.

---

## II. Related Work

### A. Desktop GCS Applications
Mission Planner [2] and QGroundControl [3] provide comprehensive UAV ground control on Windows/Linux/macOS. Both require installation on systems with ≥1 GB RAM; mature functionality but substantial hardware dependency.

### B. Web-Based GCS Systems
- **Burke [4]** — WebGCS: fully AI-generated browser-accessible GCS on Raspberry Pi companion computer; achieved arm/disarm, takeoff, land, RTL, real-time map tracking, click-to-fly in real flight; ~100 person-hours of development.
- **Atwal [5]** — Vue.js/DroneKit/MQTT web GCS with functional parity to a desktop tool, including geofence creation; required dedicated PC backend.
- **IoT-integrated GCS [6]** — real-time WebSocket telemetry visualization over 100 m flights; also required PC backend.

All three execute protocol logic on general-purpose computers (512 MB–GB class RAM); none co-locates browser-serving, MAVLink parsing, and command-filtering on the same microcontroller.

### C. ESP32 as MAVLink Platform
**DroneBridge for ESP32 [7]** implements bidirectional MAVLink relay over Wi-Fi, up to 1 km range in ESP-NOW Long-Range mode with AES-GCM-256 encryption. It is a **relay, not a GCS**: passes MAVLink bytes to an external PC application, no command semantics, no dashboard, no safety filtering.

Skylink differs by integrating a complete self-hosted browser interface, a preliminary command-filtering layer, and a dual SITL/UART compile-time design — with hardware firmware deployment confirmed on physical hardware — subjected to a structured SITL-based verification methodology.

---

## III. Problem Statement

Let **G** denote a GCS providing:
- (i) a MAVLink telemetry endpoint **T**
- (ii) a command interface **C** supporting a defined core command set
- (iii) a human-readable browser interface **U**
- (iv) a command-filtering safety layer **S**

Prior web-based GCS systems realize G = {T, C, U, S} but require a general-purpose computer H (≥512 MB RAM) to execute {T, C, S}. DroneBridge realizes only T on an ESP32.

**Research question:** Can a useful approximation of G = {T, C, U, S} covering core interactive UAV operations be realized on a microcontroller M with RAM ≤ 327 KB and no general-purpose OS, with command-stack functionality verified in SITL and hardware firmware confirmed deployable to physical hardware?

---

## IV. System Design

### A. Hardware Platform

Skylink targets the **ESP32-WROOM-32** [10]:
- Dual-core Xtensa LX6 @ 240 MHz
- 327 KB usable SRAM
- 4 MB flash (1.31 MB application partition)
- Integrated 802.11 b/g/n Wi-Fi @ 2.4 GHz
- Cost: ~$3–5 USD

In hardware mode, firmware initializes `HardwareSerial2` on **GPIO 16 (RX)** / **GPIO 17 (TX)** at **115200 baud, 8N1**, connecting to the Pixhawk 2.4.8 TELEM2 port.

Hardware wiring (Fig. 1) includes: Pixhawk autopilot, four brushless motors with SimonK 30A ESCs, power distribution board (PDB), 3S LiPo battery, 5V/3A BEC, and Ublox NEO-M8N GPS/compass module. This wiring has been assembled on physical hardware; hardware firmware confirmed operational (Section VI-C); UART/Pixhawk MAVLink communication bench testing is the immediate next validation step.

### B. Three-Tier Architecture

**Top tier — Client GCS Dashboard**
- 4,347-line single-page application (HTML/CSS/JS); 1,947 handwritten lines excluding bundled Leaflet library; 147 KB Leaflet flash asset
- Served from LittleFS flash, rendered in any web browser
- Four tabs: **Map, Fly, Status, Log**
- Map tab: Leaflet.js interactive map with yaw-rotated drone marker, home marker, 60-point position trail, 1,000 m geofence circle, click-to-fly confirmation sheet
- Leaflet library bundled offline (works without internet)
- PFD with artificial horizon and altitude tape
- Same SPA asset bundle served identically in both SITL and hardware compile-time configurations

**Middle tier — Companion Computer (ESP32)**
- Firmware implementing `AsyncWebServer`, WebSocket server, MAVLink parser, running on FreeRTOS
- Acts simultaneously as HTTP server, WebSocket hub, and MAVLink protocol gateway
- Deployed to physical ESP32 board and confirmed operational (Section VI-C)

**Bottom tier — Vehicle Autopilot**
- Pixhawk 2.4.8 running ArduCopter (hardware mode, assembled, awaiting UART bench test), OR
- ArduCopter SITL over TCP (SITL mode, fully validated)
- Communicates with middle tier via MAVLink v2

### C. Firmware Module Structure

Nine C++ modules built with PlatformIO and the Arduino framework.

**Table I — Skylink Firmware Modules** (nine modules sum to 1,778 LOC; reported total 1,893 includes `main.cpp`/`setup.cpp` ≈115 LOC)

| Module | LOC | Role |
|---|---|---|
| FlightController | 843 | MAVLink v2 parse/dispatch, telemetry, mutex |
| WebServerModule | 358 | AsyncWebServer, WS dispatch, JSON |
| WiFiManager | 173 | Auto-connect, RSSI monitor |
| LedController | 107 | LED state machine (4 patterns) |
| TimeSync | 67 | NTP synchronisation |
| ConfigManager | 66 | EEPROM/Preferences |
| OTAUpdater | 61 | Wireless firmware update |
| Logger | 54 | Timestamped serial output |
| BuildInfo | 49 | FW/FS version matching |
| main.cpp (setup) | ≈115 | Initialisation, task creation |
| **Total (.cpp)** | **1,893** | |
| **Total (.h)** | **730** | |

The `FlightController` module (843 LOC) carries the largest share of complexity: MAVLink v2 parse-dispatch loop and the command-filtering layer.

### D. Dual-Mode Compile-Time Architecture

A single compile flag (`-D SITL_MODE`) selects the MAVLink transport layer:

- **SITL mode:** `FlightController` opens a `WiFiClient` TCP connection to port **5763** on the PC running ArduCopter SITL.
- **Hardware mode:** uses `HardwareSerial2` on GPIO 16/17.

All MAVLink logic, safety checks, and telemetry processing are **identical** in both modes; only the byte I/O layer differs.

`isSitlTcpConnected()` returns `true` unconditionally in hardware mode, so the dashboard's MAVLink-active indicator correctly reflects the physical UART connection state once a HEARTBEAT is received.

This produces **two distinct firmware binaries**:
- SITL binary — verified
- Hardware binary — deployed and confirmed bootable; Pixhawk UART communication pending bench test

This is the key enabler of SITL-first development for embedded GCS firmware.

---

## V. Methods

### A. MAVLink Protocol Implementation

Uses `okalachev/MAVLink` Arduino library (**v2.0.29**) [11]. Each received byte fed into `mavlink_parse_char()` on `MAVLINK_COMM_0`; completed frames dispatched by message ID.

**Table II — MAVLink Message Types Parsed**

| Message | Fields Extracted |
|---|---|
| HEARTBEAT | Armed state, flight mode |
| SYS_STATUS | Battery voltage, remaining % |
| ATTITUDE | Roll, pitch, yaw (rad → deg) |
| VFR_HUD | Altitude (MSL), groundspeed |
| GLOBAL_POSITION_INT | Lat, lon, rel. alt, velocity |
| GPS_RAW_INT | Satellites, fix type |
| HOME_POSITION | Home lat/lon |
| COMMAND_ACK | Command ID, result code |
| STATUSTEXT | Autopilot text (5-entry ring) |

- GCS heartbeat: `MAV_TYPE_GCS`, system ID **255**, component ID **190**, transmitted at **1 Hz**.
- Data stream requests use `REQUEST_DATA_STREAM` (MAVLink legacy API, Message ID 66) targeting six stream groups (ALL, RAW_SENSORS, EXTRA1–EXTRA3, POSITION) at **4 Hz**. Chosen because it is universally supported across ArduPilot firmware versions, unlike `MAV_CMD_SET_MESSAGE_INTERVAL`, which returns `UNSUPPORTED` on the target Pixhawk 2.4.8 firmware.
- All outgoing MAVLink packets use `sysid=255` as source identifier for consistent identity with autopilot's connection-tracking logic.

### B. WebSocket Command Protocol

Commands transmitted as JSON objects over the `/ws` WebSocket endpoint.

```json
// Command (browser -> ESP32):
{"v":1,"type":"command","command":"TAKEOFF","altitude":5.0}
{"v":1,"type":"command","command":"MOVE_BODY",
"x":3.0,"y":0.0,"z":0.0}

// ACK (ESP32 -> browser, forwarded from autopilot COMMAND_ACK):
{"v":1,"type":"event","event":"ACK",
"command":22,"result":0,"result_name":"ACCEPTED","ok":true}

// Telemetry heartbeat (ESP32 -> browser, 5 Hz):
{"v":1,"type":"event","event":"HEARTBEAT",
"armed":false,"flight_mode":4,"flight_mode_name":"GUIDED",
"altitude":5.2,"relative_alt":5.1,
"lat":-35.363,"lng":149.165,
"battery_remaining":85,"gps_fix":3,"gps_sats":12,
"mav_connected":true,"wifi_connected":true,"ws_connected":true}
```

Commands flow browser → ESP32; telemetry heartbeats flow ESP32 → browser at 5 Hz. Command acknowledgements originate from the autopilot's `COMMAND_ACK` response, forwarded as ACK events.

See Table III for all fifteen dispatched commands with preconditions and filtering locations.

### C. Concurrency and Thread Safety

- Runs on FreeRTOS.
- `SemaphoreHandle_t fcMutex` guards all access to the shared `FCTelemetry` struct between the main `loop()` task and `AsyncWebServer` ISR callbacks.
- Non-blocking `takeMutex(0)` used in `handle()` to avoid starving the Wi-Fi stack.
- A **single WebSocket client** is enforced via `ws.cleanupClients(1)` on connect; multiple simultaneous clients would receive duplicate flight commands — a safety hazard in hardware mode.

### D. Firmware-Level Command Filtering

Safety filtering applied at the firmware layer, independently of autopilot parameters.

**Motion-command gate for MOVE_BODY:**

```
canMove = M (MAVLink) AND A (Armed) AND G (GUIDED) AND F (GPS-3D) AND (h >= h_min)
```

where `h` = current relative altitude, `h_min = 2 m` (AGL check).

- The armed predicate `A` is consistent with the implemented precondition set in Table III.
- The same `canExecuteGuidedMoveUnlocked()` precondition function is invoked by the `GOTO_ALT` dispatch path, so the AGL check and its associated fallback defect (Section VII-D) apply identically to **both `MOVE_BODY` and `GOTO_ALT`**.

Safety constants (Table IV) are **compile-time values** in `skylink_config.h`, requiring a firmware reflash to modify — ensuring filtering thresholds cannot be relaxed by runtime configuration or operator input.

See Table VIII for the full command-level constraint coverage matrix.

### E. Phase-Gated Development Methodology

All features implemented and acceptance-criteria verified in ArduCopter SITL before hardware assembly. See Table V for the five development phases.

### F. Experimental Setup

- **ESP32:** ESP32-WROOM-32 development board (physical hardware), PlatformIO v6.x, Arduino framework
- **ArduPilot:** ArduCopter SITL [12] 4.8.0-dev, WSL2 Ubuntu 22.04, mirrored networking
- **SITL ports:** TCP 5760 (MAVProxy), 5762 (Mission Planner observer), 5763 (Skylink)
- **Libraries:** ESPAsyncWebServer v3.0.0 [13], ArduinoJson v7.0.0 [14], MAVLink v2.0.29 [11]
- **Build:** SITL validation used FW Build 12, FS Build 15 (24 May 2026). Hardware-mode firmware subsequently deployed at FW Build 12 with FS Build 17 (current hardware build); FS Build 17 incorporates minor LittleFS filesystem corrections applied after SITL validation completed.
- **SITL validation scope:** All command-path results are from the SITL (TCP) path. Hardware-mode firmware deployed and confirmed operational on the physical ESP32 board; UART/Pixhawk MAVLink communication bench testing is the immediate next step.

---

## VI. Results

### A. Memory Footprint

Central feasibility question: can a complete protocol stack coexist within 327 KB SRAM?

**Table VI — Skylink Resource Profile (ESP32, Build 12, SITL mode)**

| Metric | Value | Reference | Method |
|---|---|---|---|
| Min. free heap (runtime) | 51 KB | 327 KB SRAM | `ESP.getMinFreeHeap()` |
| App flash usage | 860 KB | 1,310 KB partition | PlatformIO build |
| Flash utilisation | 65.6% | 1,310 KB partition | build report |
| Firmware LOC (.cpp) | 1,893 | — | cloc |
| Browser LOC (excl. Leaflet) | 1,947 | — | cloc |
| Leaflet.js asset | 147 KB | — | file size |

`ESP.getMinFreeHeap()` records the minimum free heap observed since boot (runtime low-water mark). 51 KB during SITL operation demonstrates meaningful dynamic heap headroom at peak load.

**Four architectural decisions contributing to heap efficiency:**
1. Static asset serving from LittleFS without RAM buffering
2. A fixed-size `FCTelemetry` struct updated in-place
3. A compile-time JSON buffer of 1,152 bytes
4. `AsyncWebServer` ISR processing without per-connection RAM allocation

These design choices are equally relevant to hardware-mode operation, since the firmware binary is architecturally identical between SITL and hardware builds.

### B. SITL Functional Verification

All fifteen API commands and nine MAVLink message types verified in ArduCopter 4.8.0 SITL before hardware assembly.

**Table VII — SITL Functional Verification (ArduCopter 4.8.0-dev, 24 May 2026, FW Build 12, FS Build 15)**
*All results are simulation-only; hardware-path Pixhawk results are not reported in this work.*

| Capability | SITL Pass |
|---|---|
| Wi-Fi connect + dashboard load | ✓ |
| MAVLink link establishment (<5 s) | ✓ |
| 9-message telemetry streaming (5 Hz) | ✓ |
| Preflight checklist evaluation | ✓ |
| GUIDED mode change | ✓ |
| Arm / Disarm | ✓ |
| Autonomous takeoff (5 m) | ✓ |
| Land / RTL | ✓ |
| MOVE_BODY (6 directions, 4 presets)† | ✓ |
| YAW_RELATIVE (45°/90°; 180° rejected)‡ | ✓ |
| GOTO_LATLON (click-to-fly) | ✓ |
| Geofence block (>1,000 m) | ✓ |
| LED state machine (4 states) | ✓ |

† MOVE_BODY verified in SITL. AGL floor has a known defect; retesting required after fix before hardware flight.
‡ YAW_RELATIVE: 45° and 90° executed; 180° exceeded the 90° cap and was correctly rejected (`COMMAND_ACK result ≠ 0` confirmed).

### C. Hardware Firmware Deployment Verification

Following completion of the five SITL development phases, the hardware-mode firmware (compiled without `SITL_MODE`, targeting `HardwareSerial2` on GPIO 16/17) was deployed to the physical ESP32-WROOM-32 development board and **confirmed operational**.

**Operational confirmation established by observing:**

1. Firmware init log line `[INFO] Initializing FlightController in [HARDWARE MODE] via UART2` — confirms correct binary running on physical device (FW Build 12, FS Build 17).
2. Successful Wi-Fi association, HTTP dashboard serving from LittleFS flash, and WebSocket connection establishment on physical hardware — confirms web-serving stack, LittleFS file system, and Wi-Fi driver operate correctly on-device.
3. LED state machine operation (four distinct blink patterns: Wi-Fi connecting, Wi-Fi connected, MAVLink active, armed) — verified by direct observation on physical board LED, confirming FreeRTOS task scheduling and LED controller module operate on hardware.
4. Correct build-version verification at boot: `[INFO] Build FW:12 | FS:17 MATCH` — confirms firmware and filesystem version integrity on the physical device.

**Next hardware-validation gate:** an indoor motor-spin test (no propellers), verifying ESC arming, motor direction, and TAKEOFF command acceptance before any outdoor flight.

**Immediate subsequent step:** UART/Pixhawk MAVLink communication bench testing — confirming telemetry parsing, command dispatch, and ACK handling over the physical UART connection.

### D. Comparison with Prior GCS Architectures

See Table IX for positioning against prior systems across key dimensions, including validation evidence level.

---

## VII. Discussion

### A. Feasibility of a Resource-Constrained Microcontroller GCS

The minimum free heap of 51 KB during SITL operation establishes a feasibility data point: a protocol stack encompassing telemetry parsing, command dispatch, WebSocket serving, static file serving, and command filtering can coexist within 327 KB SRAM in a SITL-validated context.

The subsequent deployment of hardware firmware to a physical ESP32 board, with confirmed Wi-Fi/web-serving/LED operation on-device, provides a **first real-hardware feasibility data point** for this GCS architecture, independent of SITL command-path validation.

Prior web-based GCS work has consistently assumed a general-purpose computer with ≥512 MB RAM is the minimum viable platform for protocol logic; Skylink challenges this assumption, though UART/Pixhawk communication validation and controlled flight testing are needed to confirm the finding.

**Cost implication at scale:**
- Per-unit GCS host cost: ~$3–5 (ESP32) vs. ~$15–35 (Raspberry Pi Zero 2W)
- At 1,000-unit deployment scale: ~$10,000–$30,000 hardware difference
- Lower power draw: ESP32 active Wi-Fi ~240 mA @ 3.3V vs. ~500 mA for RPi Zero
- Smaller physical footprint — more suitable for weight-constrained UAV integrations
- *These figures are indicative; precise comparison would require a full bill-of-materials analysis.*

### B. SITL-First as an Embedded GCS Development Methodology

The dual-mode compile-time transport switch — a single `#ifdef` block in `FlightController.cpp` — means the transport layer is the only difference between the SITL-verified firmware and the hardware-deployed firmware. All GCS command and filtering logic can therefore be fully exercised against a simulated vehicle before any hardware is assembled, ensuring the command/safety logic exercised during SITL testing is identical to the code running on the physical device — reducing hardware bring-up risk substantially, and supporting hardware deployment confidence (Section VI-C) with a repeatable regression framework.

### C. Comparison with Burke WebGCS

Burke [4] demonstrated a browser-accessible GCS on a Raspberry Pi, achieving arm/disarm, takeoff, land, RTL, real-time map tracking, and click-to-fly **in real flight**. Skylink implements a comparable set of core operator functions in SITL and has demonstrated hardware firmware deployment on physical hardware.

**The functional comparison cannot be stated as equivalence** because Burke validated in real flight while Skylink has been validated only in SITL for command paths; real-flight validation is pending.

Skylink's contribution: demonstrating this architecture on hardware ~three orders of magnitude more RAM-constrained, at lower host-device cost, with a preliminary command-filtering layer. A direct benchmark or user study against Mission Planner, QGroundControl, or Burke WebGCS is left to future work.

### D. Limitations

**1. SITL-only command-path validation.** All reported command-path results are from the SITL (TCP) path. The UART pathway intended for Pixhawk hardware is implemented and the hardware firmware deployed/confirmed bootable; however UART/Pixhawk MAVLink communication has not yet been bench-tested. No latency, jitter, packet-loss, RSSI, or range measurements available.

**2. Known AGL floor defect.** In `canExecuteGuidedMoveUnlocked()`, when `telemetry.relative_alt` is zero or negative (e.g., before home position is established), the code falls back to MSL altitude (`telemetry.altitude`), which may be non-zero at ground level — causing the 2 m AGL floor to be **bypassed**. Because `GOTO_ALT` invokes the same precondition function as `MOVE_BODY`, both commands are affected.
> **This defect must be corrected and retested in SITL before any hardware flight is attempted.**

The indoor motor-spin test procedure requires `ARMING_CHECK = 0` to bypass GPS-related arming checks during bench validation; this setting must be restored to its operational value (`ARMING_CHECK = 1`) before any outdoor or flight testing.

**3. TAKEOFF altitude bound enforced only in the browser.** Clamped to 1–50 m in browser JavaScript only; firmware forwards the parameter without an independent cap. Any WebSocket client speaking the documented JSON protocol could bypass this bound. A firmware-side cap is planned as an immediate remediation item (Section VIII).

**4. RC_OVERRIDE safety gap.** Passes raw RC channel values with no safety gate; may bypass higher-level safety rules. Should be restricted to controlled SITL testing until an explicit safety policy is added.

**5. Memory interpretation.** `ESP.getMinFreeHeap()` returns the minimum free heap since boot, not an absolute measure of SRAM utilization. Future revisions should distinguish free heap, minimum free heap, static RAM allocation, stack usage, and application-flash occupancy from the linker map.

**6. No security.** MAVLink message signing and WebSocket authentication are not implemented. The system exposes arming and navigation commands over an unauthenticated Wi-Fi channel — appropriate only for controlled laboratory or SITL testing.

**7. Single WebSocket client.** Multiple simultaneous clients are not supported; a second connection displaces the first.

**8. No formal usability study.** No evaluation against Mission Planner or QGroundControl has been conducted.

---

## VIII. Future Work

**Immediate priorities:**
1. Fix the `moveBody()` AGL floor defect and retest all altitude-boundary cases (`MOVE_BODY` and `GOTO_ALT`) in SITL
2. Add an independent firmware-side cap on the TAKEOFF altitude parameter, removing reliance on the browser-only 1–50 m bound
3. RSSI and range characterisation at 10–200 m

**Medium-term:**
4. WebSocket authentication and MAVLink v2 message signing
5. RC_OVERRIDE safety policy
6. Multi-client arbitration with command priority
7. Mission upload in AUTO mode using the waypoint protocol
8. Formal usability study using NASA-TLX and SUS instruments
9. Correct memory profiling (static DRAM from linker map, peak stack depth)

**Long-term:**
10. Natural-language control interface via the Model Context Protocol (MCP), as demonstrated by Ramos-Silva and Burke [8]
11. Offline map tile caching in LittleFS
12. 4G/LTE beyond-line-of-sight relay using the Waveshare SIM7600G-H module

---

## IX. Conclusion

This paper presented Skylink — an ESP32-hosted, browser-based GCS prototype for ArduPilot UAV applications. The implementation combines a complete browser interface served from LittleFS flash, WebSocket telemetry exchange at 5 Hz, MAVLink v2 message parsing across nine message types, a PFD with artificial horizon and altitude tape, and fifteen API command dispatch on an ESP32-class microcontroller (327 KB SRAM, FreeRTOS, Arduino framework).

A compile-time dual-mode transport abstraction enables the complete GCS firmware to be validated against ArduPilot SITL before hardware assembly. Five development phases with explicit acceptance criteria were completed and SITL-verified. A minimum runtime free heap of 51 KB and a flash occupancy of 860 KB (65.6% of the application partition) were observed under peak SITL load.

Following SITL validation, the hardware-mode firmware was **deployed to and confirmed operational** on a physical ESP32-WROOM-32 board, with Wi-Fi association, LittleFS-served dashboard, WebSocket communication, LED state machine, and build-version integrity all confirmed on-device.

> To the authors' knowledge, within the surveyed literature, this is the first system combining a self-hosted browser GCS, a firmware-level command-filtering layer, and a dual SITL/hardware compile-time design on a single ESP32-class device.

The current evidence supports the feasibility of this architecture; **UART/Pixhawk bench testing, complete command-safety coverage (including correction of the known `moveBody()` AGL floor defect), communication security, and controlled flight validation remain as the immediate next steps.**

---

## Acknowledgements

The authors thank Dr. Akshay Pandey for guidance and supervision throughout this project, and IIITDM Jabalpur for laboratory access. This work was carried out as a Bachelor of Technology project.

---

## Appendix: Detailed Tables

### Table III — Skylink WebSocket API: All Fifteen Commands

Commands marked † are system utilities. Safety constraints for motion-affecting commands detailed in Table VIII. The TAKEOFF altitude bound is enforced in browser JavaScript (1–50 m); the firmware forwards the parameter without an independent cap.

| Command | Preconditions / Notes | Filtering Location | SITL Verified |
|---|---|---|---|
| ARM_DRONE | MAVLink active | Firmware | ✓ |
| DISARM_DRONE | Always accepted | — | ✓ |
| SET_FLIGHT_MODE | Mode name string | Firmware | ✓ |
| TAKEOFF | MAVLink active; alt param; bound 1–50 m | Browser JS | ✓ |
| LAND | Sets LAND mode | — | ✓ |
| RTL | Sets RTL mode | — | ✓ |
| MOVE_BODY | GUIDED + armed + GPS-3D + AGL ≥ 2 m*; cap 200 m/axis | Firmware | ✓ |
| YAW_RELATIVE | GUIDED + armed + GPS-3D; \|Δθ\| ≤ 90° | Firmware | ✓ |
| GOTO_LATLON | Armed + GPS-3D + radius ≤ 1,000 m; alt 2–50 m | Firmware | ✓ |
| GOTO_ALT | GUIDED + armed + GPS-3D + AGL ≥ 2 m*; alt 2–50 m | Firmware | ✓ |
| LOITER_HERE | Armed + GPS-3D; sets LOITER mode | Firmware | ✓ |
| RC_OVERRIDE | Raw RC channel values; no safety filtering‡ | None | ✓ |
| LED_SET † | Manual LED control | — | ✓ |
| LED_TOGGLE † | Toggle LED state | — | ✓ |
| PING † | Round-trip latency measurement | — | ✓ |

\* AGL floor has a known defect; see Section VII-D.
‡ RC_OVERRIDE may bypass higher-level safety rules; restrict to controlled SITL testing until a safety policy is defined.

### Table IV — Firmware Safety Filter Constants (`skylink_config.h`)

All values are compile-time constants. The TAKEOFF altitude bound is enforced in browser JavaScript only; the firmware does not independently cap this parameter.

| Parameter | Value |
|---|---|
| Altitude floor (AGL) for MOVE_BODY / GOTO_ALT* | 2.0 m |
| MOVE_BODY per-axis displacement cap | 200.0 m |
| GOTO_ALT / GOTO_LATLON altitude cap | 50.0 m |
| Geofence radius (GOTO_LATLON) | 1,000 m |
| YAW_RELATIVE per-command maximum | 90° |
| Command debounce interval | 500 ms |
| Arm hold (UI) | 1,500 ms |

\* AGL floor has a known defect; see Section VII-D.

### Table V — Five-Phase SITL-First Development Methodology

All five phases completed in SITL; hardware firmware subsequently deployed to physical ESP32 and confirmed operational.

| Phase | Objective | Acceptance Criterion |
|---|---|---|
| 1 | Transport + heartbeat | ESP32 connects to SITL TCP; receives HEARTBEAT within 5 s |
| 2 | Telemetry parse + dashboard | All 9 MAVLink types parsed; dashboard updates at 5 Hz |
| 3 | Command dispatch | ARM, TAKEOFF, LAND, RTL, MOVE_BODY issue correctly; COMMAND_ACK received |
| 4 | Map + click-to-fly | GOTO_LATLON triggered from map click; geofence blocks >1,000 m |
| 5 | Command-filtering layer + LED | All Table IV constraints compile-time enforced; LED state machine functional |

### Table VIII — Command-by-Constraint Safety Coverage Matrix

| Command | Armed | GUIDED | GPS-3D | AGL ≥ 2 m | Geofence | Cap |
|---|---|---|---|---|---|---|
| TAKEOFF | — | — | — | — | — | Browser-only |
| MOVE_BODY | Enforced | Enforced | Enforced | Defect* | Not applied | Enforced (200 m) |
| YAW_RELATIVE | Enforced | Enforced | Enforced | — | — | Enforced (90°) |
| GOTO_LATLON | Enforced | Auto† | Enforced | — | Enforced | Enforced (50 m alt) |
| GOTO_ALT | Enforced | Enforced | Enforced | Defect* | — | Enforced (50 m) |
| RC_OVERRIDE | — | — | — | — | — | None |

\* AGL floor defect: when `relative_alt` ≤ 0 (e.g., before home position is established), code falls back to MSL altitude, which may be non-zero at ground level, causing the 2 m floor to be bypassed. Both MOVE_BODY and GOTO_ALT call the same `canExecuteGuidedMoveUnlocked()` precondition function and are therefore both affected. Fix and retest before any hardware flight.
† GOTO_LATLON issues a mode-change command if not in GUIDED, rather than rejecting; GUIDED is not enforced as a hard precondition.

### Table IX — Comparison of Web-Based UAV GCS Architectures

Evidence level indicates the highest-confidence validation reported. Skylink command-path results are SITL-only; hardware firmware deployment to physical ESP32 has been confirmed. Burke WebGCS [4] includes real-flight evidence.
Sources: Skylink – this work; Burke WebGCS – Burke [4]; Atwal DashApp – Atwal [5] (Bachelor's thesis); IoT GCS – Lestari et al. [6]; DroneBridge – open-source project [7]; AeroGCS Enterprise – commercial product [9].

| System | Platform | Self-Hosted | API Cmds | Safety Layer | Security | Evidence Level |
|---|---|---|---|---|---|---|
| Skylink (this work) | ESP32, 327 KB | Yes | 15ᵃ | Firmware (partialᵇ) | None | SITL + HW firmware deployed |
| Burke WebGCS [4] | RPi, 512 MB+ | Yes | ≈10 | Python | Not stated | Real flight |
| Atwal DashApp [5] | PC + backend | No | >10 | DroneKit | n/a | Simulation |
| IoT GCS [6] | PC + backend | No | Few | None | n/a | 100 m flights |
| DroneBridge [7] | ESP32, 327 KB | No (relay) | 0 | None | AES-GCM-256 | Deployed |
| AeroGCS Ent. [9] | Cloud | Cloud-only | Many | Cloud | Cloud | Commercial |

ᵃ 15 total API commands (Table III): 12 flight-related, 3 system utilities.
ᵇ Safety constants implemented; AGL floor for MOVE_BODY and GOTO_ALT has a known defect (Section VII-D).

---

## References

[1] ArduPilot Development Team, "ArduPilot: Open Source Autopilot," ardupilot.org, 2025. https://ardupilot.org

[2] M. Oborne, "Mission Planner Ground Control Station," ardupilot.org, 2025. https://ardupilot.org/planner

[3] QGroundControl Development Team, "QGroundControl," docs.qgroundcontrol.com, 2025. https://docs.qgroundcontrol.com

[4] P. J. Burke, "AI generated drone command and control station hosted in the sky," *npj Artificial Intelligence*, vol. 2, Article 43, 2026. doi: 10.1038/s44387-026-00101-6

[5] J. S. Atwal, "Drone control and monitoring by means of a web application," Bachelor's Degree Final Project, Universitat Politècnica de Catalunya (UPC), Sept. 2023.

[6] P. E. W. Lestari, M. Ridwan, R. F. Ramadhan, and F. Nisfa, "Real-Time IoT-Integrated Ground Control Station (GCS) for Unmanned Aerial Vehicle (UAV) Monitoring System," *Journal of Electrical Engineering and Education*, vol. 9, no. 2, Oct. 2025. doi: 10.21070/jeeeu.v9i2.1710

[7] D. Feger and contributors (DroneBridge), "DroneBridge for ESP32: A secure and transparent telemetry link," GitHub, 2019–2025. https://github.com/DroneBridge/ESP32

[8] J. N. Ramos-Silva and P. J. Burke, "A Universal Large Language Model–Drone Command and Control Interface," arXiv:2601.15486, 2026. doi: 10.48550/arXiv.2601.15486

[9] PDRL, "AeroGCS Enterprise User Manual–Drone Control Center," aerogcs.com, 2025. https://aerogcs.com

[10] Espressif Systems, "ESP32 Series Datasheet," v3.5, 2023. https://www.espressif.com/en/support/documents/technical-documents

[11] MAVLink Development Team, "MAVLink Micro Air Vehicle Communication Protocol," mavlink.io, 2025. https://mavlink.io

[12] ArduPilot Development Team, "SITL – Software In The Loop Simulation," ardupilot.org, 2025. https://ardupilot.org/dev/docs/sitl-simulator-software-in-the-loop.html

[13] M. Carbou, "ESP Async WebServer," GitHub, 2024. https://github.com/mathieucarbou/ESPAsyncWebServer

[14] B. Fischler, "ArduinoJson: Efficient JSON library for embedded C++," arduinojson.org, 2024. https://arduinojson.org