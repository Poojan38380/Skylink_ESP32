# Skylink ESP32 — Flight Controller Simulation Test Plan
## Software-in-the-Loop (SITL) & GCS Dashboard Integration Guide

This guide provides a detailed, phase-wise implementation and testing workflow to build and test a bidirectional flight controller bridge on your ESP32. You will bridge a real-time ArduPilot flight simulator running on your computer directly to your physical ESP32 via WiFi, and view live physics-based telemetry on your custom Ground Control Station (GCS) dashboard—all **before** purchasing or wiring a physical Pixhawk 6C flight controller.

---

## 🛰️ Architecture of the Simulation Stack

During simulation testing, the physical UART wire is replaced with a virtual **WiFi TCP connection** over your local network:

```
┌───────────────────────────────────────┐
│     GCS Dashboard (Web Browser)       │
└──────────────────┬────────────────────┘
                   │ WebSocket (Port 80 /ws)
                   ▼
┌───────────────────────────────────────┐
│        ESP32 Dev Module (Skylink)     │
└──────────────────┬────────────────────┘
                   │ MAVLink over WiFi TCP (Port 5760 or 5762)
                   ▼
 ───────────────── [WiFi Network Boundary] ─────────────────
                   ▼
┌───────────────────────────────────────┐
│           Windows 11 Host             │
│  ┌─────────────────────────────────┐  │
│  │   ArduPilot SITL (Copter)       │◀─┼─┐  (MAVLink UDP 14550)
│  │   Running on WSL2 (Ubuntu)      │  │ │
│  └─────────────────────────────────┘  │ │
│  ┌─────────────────────────────────┐  │ │
│  │     Mission Planner (GCS)       │──┼─┘
│  │   Secondary 3D map & status     │  │
│  └─────────────────────────────────┘  │
└───────────────────────────────────────┘
```

---

## 📅 Phase 1: Environment Provisioning (PC & WSL2)

In this phase, you will configure Windows Subsystem for Linux (WSL2), build the ArduPilot SITL environment, and set up the networking bridge.

### Step 1.1: Enable WSL2 and Install Ubuntu
If you do not have WSL2 installed:
1. Open **PowerShell** as an Administrator and run:
   ```powershell
   wsl --install
   ```
2. Restart your computer when prompted.
3. Upon reboot, a terminal will open and prompt you to create a Linux username and password for Ubuntu.

### Step 1.2: Enable Mirrored Networking Mode (Critical for ESP32 Connection)
WSL2 normally operates behind a virtual NAT, which prevents external devices (like your ESP32) from reaching ports hosted inside Linux. **Mirrored Networking** shares your Windows machine's IP address directly with Linux.

1. Open Windows Explorer and navigate to your user profile directory (e.g., `C:\Users\Poojan\`).
2. Create a new text file named `.wslconfig` (ensure it does not end with `.txt`).
3. Add the following lines to the file:
   ```ini
   [wsl2]
   networkingMode=mirrored
   ```
4. Save and close the file.
5. In PowerShell, restart WSL to apply the configuration:
   ```powershell
   wsl --shutdown
   ```
6. Open your WSL2 (Ubuntu) application. It is now sharing your Windows PC's local IP address!

### Step 1.3: Download and Build ArduPilot SITL
Open your WSL2 terminal and execute the following commands to install the compiler toolchain and clone ArduPilot:

```bash
# Update Ubuntu package indices
sudo apt update && sudo apt upgrade -y

# Clone ArduPilot repository
git clone --recursive https://github.com/ArduPilot/ardupilot.git
cd ardupilot

# Run the automatic environment prerequisite installation script
# Choose option 'y' to install all required packages
Tools/environment_install/install-prereqs-ubuntu.sh -y

# Reload your shell profile to apply path variables
. ~/.profile

# Navigate to the Copter directory and build the simulator
cd ArduCopter
sim_vehicle.py -v ArduCopter --console --map
```
*   **Verification**: After a few minutes, you should see three windows open: a command console, a 2D map window, and the main SITL terminal displaying `APM: Copter Solo-1.0.0...`. The system is successfully simulating a quadcopter.
*   **Close SITL** for now by typing `ctrl+c` or `quit` in the main terminal.

---

## 📅 Phase 1.5: Mission Planner Installation & Setup

Mission Planner acts as a secondary ground station to verify physics, adjust drone parameters, and visually track drone movement in 3D.

### Step 1.5.1: Install Mission Planner
1. Download the latest Windows installer (`.msi`) from the official website:
   👉 [ArduPilot Mission Planner Downloads](https://ardupilot.org/planner/docs/mission-planner-installation.html)
2. Run the downloaded installer.
3. Accept the license agreement and proceed with the installation. **Allow the installer to install the device drivers** when prompted (these are required to connect to real flight controllers via USB later).

### Step 1.5.2: Verify Local Connection to SITL
1. Start ArduPilot SITL in your WSL2 terminal:
   ```bash
   cd ~/ardupilot/ArduCopter
   sim_vehicle.py -v ArduCopter --console --map
   ```
2. Open **Mission Planner** on your Windows desktop.
3. In the top-right corner, set the connection drop-down to **UDP** (do NOT click Connect yet).
4. Click **CONNECT**.
5. A prompt will ask for a port. Enter **14550** (the default output port where SITL broadcasts telemetry).
6. Mission Planner will download parameters from the simulation. Once complete, you will see a green line showing the quadcopter's home position on the map and active HUD instruments.

---

## 📅 Phase 2: PlatformIO MAVLink & Build Configuration

Now, prepare your ESP32 PlatformIO project to compile MAVLink libraries and support network sockets.

### Step 2.1: Modify `platformio.ini`
Open your `platformio.ini` in VS Code and append the MAVLink library and a compile flag.

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
upload_speed = 115200
upload_port = COM6

board_build.filesystem = littlefs

lib_deps =
  mathieucarbou/AsyncTCP @ ^3.0.0
  mathieucarbou/ESP Async WebServer @ ^3.0.0
  bblanchon/ArduinoJson @ ^7.0.0
  okalachev/MAVLink v2 library for Arduino @ ^2.0.29   ; Add MAVLink library

build_flags =
  -D SITL_MODE                                         ; Enable TCP socket for simulation
```
> **Note**: When you move to a physical Pixhawk, simply comment out the `-D SITL_MODE` line to switch the communication interface to physical Hardware UART2!

---

## 📅 Phase 3: C++ FlightController Bridging Code

Create a unified C++ class on the ESP32 that handles sending and receiving MAVLink frames. It seamlessly switches between TCP (simulation) and UART (hardware).

### Step 3.1: Create Header `include/flight_controller.h`
Create a new header file [flight_controller.h](file:///d:/btp_skylink/Skylink/include/flight_controller.h):

```cpp
#ifndef FLIGHT_CONTROLLER_H
#define FLIGHT_CONTROLLER_H

#include <Arduino.h>
#include <MAVLink.h>

#ifdef SITL_MODE
#include <WiFiClient.h>
#endif

// Decoded Flight Controller telemetry state
struct FCTelemetry {
    bool armed = false;
    float roll = 0.0;
    float pitch = 0.0;
    float yaw = 0.0;
    float altitude = 0.0;
    float speed = 0.0;
    float battery_voltage = 0.0;
    int battery_remaining = 0;
    double latitude = 0.0;
    double longitude = 0.0;
    int gps_sats = 0;
    int gps_fix = 0;
};

class FlightController {
private:
    FCTelemetry telemetry;
    uint32_t lastHeartbeatSent = 0;
    uint32_t lastRCCommand = 0;
    bool isConnectedToFC = false;

#ifdef SITL_MODE
    WiFiClient sitlClient;
    const char* sitlHost = "127.0.0.1"; // Thanks to Mirrored Mode, WSL2 is mapped to localhost!
    const uint16_t sitlPort = 5760;     // SITL default TCP port
#else
    HardwareSerial& fcSerial;
#endif

    // Internal MAVLink handling helper functions
    void sendMavlinkPacket(mavlink_message_t* msg);
    void handleIncomingByte(uint8_t byte);
    void processMavlinkMessage(mavlink_message_t* msg);

public:
#ifdef SITL_MODE
    FlightController();
#else
    FlightController(HardwareSerial& serial);
#endif

    void begin();
    void handle();

    // Outbound Actuator Controls
    void arm(bool state);
    void sendRCOverride(uint16_t roll, uint16_t pitch, uint16_t throttle, uint16_t yaw);
    void sendHeartbeat();
    void emergencyStop();

    // Inbound Telemetry Getters
    FCTelemetry getTelemetry();
    bool isConnected();
};

extern FlightController flightController;

#endif // FLIGHT_CONTROLLER_H
```

### Step 3.2: Create Source `src/flight_controller.cpp`
Create a new implementation file [flight_controller.cpp](file:///d:/btp_skylink/Skylink/src/flight_controller.cpp):

```cpp
#include "flight_controller.h"
#include "logger.h"

#ifdef SITL_MODE
FlightController::FlightController() {}
#else
FlightController::FlightController(HardwareSerial& serial) : fcSerial(serial) {}
#endif

void FlightController::begin() {
#ifdef SITL_MODE
    logger.info("Initializing FlightController in [SITL MODE] via TCP");
#else
    logger.info("Initializing FlightController in [HARDWARE MODE] via UART2");
    fcSerial.begin(115200, SERIAL_8N1, 16, 17); // RX2 = GPIO16, TX2 = GPIO17
#endif
    isConnectedToFC = false;
}

void FlightController::sendMavlinkPacket(mavlink_message_t* msg) {
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buf, msg);

#ifdef SITL_MODE
    if (sitlClient.connected()) {
        sitlClient.write(buf, len);
    }
#else
    fcSerial.write(buf, len);
#endif
}

void FlightController::arm(bool state) {
    logger.info(state ? "Sending command: ARM Drone" : "Sending command: DISARM Drone");
    
    mavlink_message_t msg;
    mavlink_msg_command_long_pack(
        1, 255, &msg,               // System ID 1, Component ID 255 (GCS)
        1, 1,                       // Target System 1, Target Component 1 (Autopilot)
        MAV_CMD_COMPONENT_ARM_DISARM, // Command ID
        0,                          // Confirmation
        state ? 1.0f : 0.0f,        // Param 1 (1 to arm, 0 to disarm)
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f // Param 2-7 (unused)
    );
    sendMavlinkPacket(&msg);
}

void FlightController::sendRCOverride(uint16_t roll, uint16_t pitch, uint16_t throttle, uint16_t yaw) {
    mavlink_message_t msg;
    // MAVLink channels: 1=Roll, 2=Pitch, 3=Throttle, 4=Yaw (Values: 1000 - 2000 ms)
    mavlink_msg_rc_channels_override_pack(
        1, 255, &msg,
        1, 1,
        roll, pitch, throttle, yaw,
        0, 0, 0, 0                  // Channels 5-8 (unused)
    );
    sendMavlinkPacket(&msg);
}

void FlightController::sendHeartbeat() {
    mavlink_message_t msg;
    mavlink_msg_heartbeat_pack(
        1, 255, &msg,
        MAV_TYPE_GCS,               // Identify as Ground Control Station
        MAV_AUTOPILOT_INVALID,      // No autopilot algorithm running on GCS
        MAV_MODE_FLAG_MANUAL_INPUT_ENABLED,
        0,
        MAV_STATE_ACTIVE
    );
    sendMavlinkPacket(&msg);
}

void FlightController::emergencyStop() {
    logger.warning("SAFETY TRIGGERED: EMERGENCY DISARM!");
    // Cut throttle and disarm instantly
    sendRCOverride(1500, 1500, 1000, 1500);
    arm(false);
}

FCTelemetry FlightController::getTelemetry() {
    return telemetry;
}

bool FlightController::isConnected() {
    return isConnectedToFC;
}

void FlightController::handle() {
    uint32_t now = millis();

    // 1. Maintain Connection (SITL TCP Client socket loop)
#ifdef SITL_MODE
    if (!sitlClient.connected()) {
        if (now - lastHeartbeatSent >= 5000) { // Retry connection every 5s
            lastHeartbeatSent = now;
            logger.info("Attempting connection to SITL at " + String(sitlHost) + ":" + String(sitlPort));
            if (sitlClient.connect(sitlHost, sitlPort)) {
                logger.info("Connected to ArduPilot SITL Socket!");
                isConnectedToFC = true;
            } else {
                logger.warning("SITL connection failed. Is the simulation running?");
                isConnectedToFC = false;
            }
        }
    }
#endif

    // 2. Stream Outbound Heartbeats to Autopilot (Required by ArduPilot every 1s)
    if (isConnectedToFC && (now - lastHeartbeatSent >= 1000)) {
        lastHeartbeatSent = now;
        sendHeartbeat();
    }

    // 3. Process Inbound Serial Bytes
#ifdef SITL_MODE
    while (sitlClient.connected() && sitlClient.available() > 0) {
        handleIncomingByte(sitlClient.read());
    }
#else
    while (fcSerial.available() > 0) {
        handleIncomingByte(fcSerial.read());
    }
#endif
}

void FlightController::handleIncomingByte(uint8_t byte) {
    mavlink_message_t msg;
    mavlink_status_t status;
    
    // Parse stream byte-by-byte into full packet frames
    if (mavlink_parse_char(MAVLINK_COMM_0, byte, &msg, &status)) {
        processMavlinkMessage(&msg);
    }
}

void FlightController::processMavlinkMessage(mavlink_message_t* msg) {
    isConnectedToFC = true; // Heartbeat packet frame verified

    switch (msg->msgid) {
        case MAVLINK_MSG_ID_HEARTBEAT: {
            mavlink_heartbeat_t hb;
            mavlink_msg_heartbeat_decode(msg, &hb);
            telemetry.armed = (hb.base_mode & MAV_MODE_FLAG_SAFETY_ARMED);
            break;
        }
        case MAVLINK_MSG_ID_SYS_STATUS: {
            mavlink_sys_status_t sys;
            mavlink_msg_sys_status_decode(msg, &sys);
            telemetry.battery_voltage = sys.voltage_battery / 1000.0f; // mV -> Volts
            telemetry.battery_remaining = sys.battery_remaining;       // %
            break;
        }
        case MAVLINK_MSG_ID_ATTITUDE: {
            mavlink_attitude_t att;
            mavlink_msg_attitude_decode(msg, &att);
            telemetry.roll = att.roll * 57.2958f;   // Radians -> Degrees
            telemetry.pitch = att.pitch * 57.2958f;
            telemetry.yaw = att.yaw * 57.2958f;
            break;
        }
        case MAVLINK_MSG_ID_VFR_HUD: {
            mavlink_attitude_t vfr;
            mavlink_vfr_hud_t hud;
            mavlink_msg_vfr_hud_decode(msg, &hud);
            telemetry.altitude = hud.alt;           // Meters
            telemetry.speed = hud.groundspeed;      // m/s
            break;
        }
        case MAVLINK_MSG_ID_GPS_RAW_INT: {
            mavlink_gps_raw_int_t gps;
            mavlink_msg_gps_raw_int_decode(msg, &gps);
            telemetry.latitude = gps.lat / 10000000.0;  // Scale 1e7 -> Decimal degrees
            telemetry.longitude = gps.lon / 10000000.0;
            telemetry.gps_sats = gps.satellites_visible;
            telemetry.gps_fix = gps.fix_type;           // 3 = 3D Fix
            break;
        }
    }
}

// Instantiate the global object
#ifdef SITL_MODE
FlightController flightController;
#else
FlightController flightController(Serial2);
#endif
```

---

## 📅 Phase 4: GCS WebSocket Telemetry & Parser Binding

Bind the flight controller data directly to the `/ws` WebSockets server.

### Step 4.1: Integrate Flight Controller in `src/main.cpp`
Open `src/main.cpp` and modify it to include and loop `flightController`.

```cpp
// Add to include section (approx line 10)
#include "flight_controller.h"

// In setup() (approx line 35)
void setup() {
    // ... existing setup items ...
    flightController.begin();
    logger.info("Setup complete");
}

// In loop() (approx line 43)
void loop() {
    wifiManager.handle();
    otaUpdater.handle();
    flightController.handle(); // <-- PROCESS MAVLINK packets constantly

    unsigned long now = millis();
    // ...
```

### Step 4.2: Replace Mock Data with Real Telemetry in `src/web_server.cpp`
Open `src/web_server.cpp` and modify two sections:
1.  **Incoming WebSocket Command Parser** (Map GUI inputs to MAVLink packets).
2.  **Outbound Telemetry Encoder** (Change mock numbers to actual SITL values).

```cpp
// 1. In handleWebSocketMessage (approx line 40)
// Replace existing ledController control commands with flight commands:
if (command == "LED_SET") {
    bool value = doc["value"] | false;
    ledController.set(value);
    sendAppState();
} 
else if (command == "LED_TOGGLE") {
    ledController.toggle();
    sendAppState();
}
// Add flight control mappings:
else if (command == "ARM_DRONE") {
    flightController.arm(true);
}
else if (command == "DISARM_DRONE") {
    flightController.arm(false);
}
else if (command == "RC_OVERRIDE") {
    // Expects json format: {"type":"command","command":"RC_OVERRIDE","roll":1500,"pitch":1500,"throttle":1500,"yaw":1500}
    uint16_t r = doc["roll"] | 1500;
    uint16_t p = doc["pitch"] | 1500;
    uint16_t t = doc["throttle"] | 1000;
    uint16_t y = doc["yaw"] | 1500;
    flightController.sendRCOverride(r, p, t, y);
}

// 2. In sendHeartbeat() (approx line 89)
// Replace the entire method with the following code to feed REAL flight simulation data to your browser:
void WebServerModule::sendHeartbeat() {
    JsonDocument doc;
    FCTelemetry fc = flightController.getTelemetry();

    doc["type"] = "event";
    doc["event"] = "HEARTBEAT";
    doc["uptime"] = millis() / 1000;
    doc["timestamp"] = timeSync.getCurrentTime();
    
    // Live MAVLink values instead of dummy variables!
    doc["armed"] = fc.armed;
    doc["altitude"] = fc.altitude;
    doc["battery"] = fc.battery_remaining;
    doc["battery_v"] = fc.battery_voltage;
    doc["speed"] = fc.speed;
    doc["lat"] = fc.latitude;
    doc["lng"] = fc.longitude;
    doc["sats"] = fc.gps_sats;
    doc["gps_fix"] = fc.gps_fix;
    
    // Send pitch/roll orientation as dynamic elements
    doc["roll"] = fc.roll;
    doc["pitch"] = fc.pitch;

    String output;
    serializeJson(doc, output);
    ws.textAll(output);
}
```

### Step 4.3: Add ARM/DISARM Buttons and Flight Commands to GCS Dashboard (`data/index.html`)
Open `data/index.html` and update the Control and Telemetry cards to bind the real flight functions.

1.  **Replace the LED Command Buttons section (approx line 765)** with flight controls:
    ```html
    <div class="btn-row">
      <button class="btn-on" id="btn-arm" disabled onclick="sendCmd('ARM_DRONE')">⚡ ARM DRONE</button>
      <button class="btn-off" id="btn-disarm" disabled onclick="sendCmd('DISARM_DRONE')">🛑 DISARM</button>
    </div>
    <div class="btn-row">
      <button class="btn-toggle" id="btn-liftoff" disabled onclick="simulateTakeoff()">🚀 CLIMB (RC TAKE OFF)</button>
    </div>
    <div class="btn-row" style="margin-top:4px">
      <button class="btn-ping" id="btn-land" disabled onclick="sendCmd('RC_OVERRIDE', {roll:1500, pitch:1500, throttle:1100, yaw:1500})">⬇ LAND DRONE</button>
    </div>
    ```
2.  **Add a Takeoff Function helper script** in the `<script>` tag at the bottom of the page:
    ```javascript
    function simulateTakeoff() {
      log('SYS', 'tag-sys', 'Sending RC Takeoff sequence...');
      // Override throttle high (e.g. 1700ms) to climb, other channels neutral (1500ms)
      sendCmd('RC_OVERRIDE', { roll: 1500, pitch: 1500, throttle: 1700, yaw: 1500 });
      
      // Auto return throttle to hover state (1500ms) after 3.5 seconds
      setTimeout(() => {
        log('SYS', 'tag-sys', 'Entering stable hover...');
        sendCmd('RC_OVERRIDE', { roll: 1500, pitch: 1500, throttle: 1500, yaw: 1500 });
      }, 3500);
    }
    ```
3.  **Update `updateTelemetry` Javascript (approx line 933)** to process the true telemetry fields:
    ```javascript
    function updateTelemetry(d) {
      document.getElementById('tl-alt').textContent = d.altitude.toFixed(1);
      document.getElementById('tl-speed').textContent = d.speed.toFixed(1);
      document.getElementById('tl-lat').textContent = d.lat.toFixed(6) + '°N';
      document.getElementById('tl-lng').textContent = d.lng.toFixed(6) + '°E';
      document.getElementById('lnk-ip').textContent = connectedIP;
      document.getElementById('lnk-uptime').textContent = fmtUptime(d.uptime);

      // Arm Status
      const stateBadge = document.getElementById('lnk-state');
      if (d.armed) {
        stateBadge.textContent = "● ARMED / FLYING";
        stateBadge.className = "stat-value bad"; // Show red alarm when armed
      } else {
        stateBadge.textContent = "○ DISARMED / SAFE";
        stateBadge.className = "stat-value good";
      }

      // Battery
      const bat = d.battery;
      document.getElementById('tl-bat').textContent = bat + "% (" + d.battery_v.toFixed(1) + "V)";
      const bar = document.getElementById('bat-bar');
      bar.style.width = bat + '%';
      bar.style.background = bat > 50 ? 'var(--green)' : bat > 20 ? 'var(--orange)' : 'var(--red)';

      // Signal Strength (Parse GPS Sats instead of RSSI to indicate localization quality)
      const sb = document.getElementById('signal-bars');
      if (d.sats > 9) sb.className = 'signal-bars s5';
      else if (d.sats > 6) sb.className = 'signal-bars s4';
      else if (d.sats > 3) sb.className = 'signal-bars s3';
      else sb.className = 'signal-bars s1';

      document.getElementById('last-hb-text').textContent = 'Live telemetry | Sats: ' + d.sats;
    }
    ```

---

## 📅 Phase 5: End-to-End Simulation Test Run Checklist

Once your code is written and flashed, use this sequential checklist to test the entire loop:

### Step 5.1: Start the Simulation on your PC
1. Open your **WSL2 (Ubuntu)** terminal.
2. Launch SITL:
   ```bash
   cd ~/ardupilot/ArduCopter
   sim_vehicle.py -v ArduCopter --console --map
   ```
3. Open **Mission Planner** on your Windows Desktop, select **UDP**, and click **Connect** on port 14550. Confirm the status updates properly.

### Step 5.2: Boot the ESP32 and Connect to Dashboard
1. Power on your physical ESP32 module.
2. Check your Serial monitor to verify it connects to your WiFi router successfully.
3. Observe the logs:
   - `[INFO] Attempting connection to SITL at 127.0.0.1:5760`
   - `[INFO] Connected to ArduPilot SITL Socket!`
   - `[INFO] Stream verified. MAVLink Handshake Established!`
4. Open your web browser and navigate to `http://<ESP32_IP_ADDRESS>/`.
5. Verify the **Skylink GCS Dashboard** connection indicator turns green (`LINK ACTIVE`).
6. Observe the telemetry panel: **GPS coordinates, Battery metrics, and Altitude are updating in real time, mirroring ArduPilot's physics engine!**

### Step 5.3: Execute the Arm and Hover Flight Sequence
1. In your browser's **GCS Dashboard**, click the **⚡ ARM DRONE** button.
2. **Watch the Terminals**:
   - The ESP32 logs show `[INFO] Sending command: ARM Drone`.
   - The ArduPilot SITL terminal outputs `APM: Arming motors`.
   - Mission Planner's HUD reads **ARMED** in red, and the quadcopter propeller icons start rotating!
3. Click the **🚀 CLIMB (RC TAKE OFF)** button.
   - The dashboard triggers an RC Channel 3 (Throttle) override up to 1700ms.
   - Watch the **Altitude** metric climb steadily in your GCS Dashboard, Mission Planner HUD, and the 2D simulator map.
   - After 3.5 seconds, throttle drops back to 1500ms, and the altitude stabilizes.
4. Click the **🛑 DISARM / EMERGENCY STOP** button.
   - Autopilot cuts motors instantly, and the drone falls back to the ground (altitude resets to 0m).

---

## 📅 Phase 6: Pixhawk 6C Hardware Migration Path

After fully validating the code in simulation, follow this hardware wiring and software parameter roadmap to migrate to the **Pixhawk 6C flight controller**.

### Step 6.1: Physical Serial Port Wiring Diagram
Ensure the flight controller and ESP32 share a common ground reference.

```
       ESP32 DEV MODULE                   PIXHAWK 6C (TELEM2)
    ┌────────────────────┐               ┌────────────────────┐
    │                    │               │                    │
    │         GPIO16(RX) ├◄──────────────┤ TX                 │ (Pin 2)
    │                    │               │                    │
    │         GPIO17(TX) ├──────────────►│ RX                 │ (Pin 3)
    │                    │               │                    │
    │                GND ├───────────────┤ GND                │ (Pin 6)
    │                    │               │                    │
    │      External 5V   ├◄──────────────┤ VCC_5V             │ (Pin 1)
    │      (V5 / VIN pin)│               │                    │
    └────────────────────┘               └────────────────────┘
```
> [!WARNING]
> **Logic Level Voltage Check**: Pixhawk serial ports operate on standard **3.3V TTL logic levels**, which matches the ESP32's native 3.3V pin inputs. No logic level converter is required. However, **do NOT supply more than 3.3V to the ESP32 GPIO16/17 pins**, and verify that the 5V VCC power supply wire is connected strictly to the ESP32 `VIN` or `5V` pin (not `3V3`).

### Step 6.2: Configure Pixhawk MAVLink Parameters
Before the Pixhawk will talk to your ESP32, you must configure the parameters on the corresponding serial telemetry port (`TELEM2` corresponds to `SERIAL2` in ArduPilot).

1. Connect the Pixhawk 6C directly to your computer using a USB-C cable.
2. Open **Mission Planner** and connect to the board via Auto/COM port.
3. Navigate to **CONFIG** -> **Full Parameter List**.
4. Search for the following parameters and change their values:
   *   `SERIAL2_PROTOCOL` = **2** (This configures MAVLink 2 protocol on the port).
   *   `SERIAL2_BAUD` = **115** (Sets the UART baud rate to **115200 bps**, matching the ESP32 code).
   *   `SR2_POSITION` = **2** (Instructs Pixhawk to push GPS coordinates at 2Hz).
   *   `SR2_EXTRA1` = **10** (Pushes attitude, pitch, and roll telemetry at 10Hz).
   *   `SR2_SYS_STATUS` = **2** (Pushes battery status updates at 2Hz).
5. Click **Write Params** to save these settings to the Pixhawk's internal flash memory.
6. Power-cycle the Pixhawk.

### Step 6.3: Disable SITL Mode in ESP32 Code
To prepare your ESP32 to connect to the real flight controller over physical wires:
1. Open your `platformio.ini` file.
2. Comment out or delete the `-D SITL_MODE` build flag:
   ```ini
   build_flags =
     ; -D SITL_MODE   <-- Commented out for physical hardware connection!
   ```
3. Rebuild and flash the ESP32 (`pio run --target upload`). The class will automatically route all data through Hardware Serial UART2!

### Step 6.4: Critical Bench Safety Rules
> [!CAUTION]
> **REMOVE PROPELLERS BEFORE BENCH TESTING**: Never, under any circumstances, power on or connect a drone to a computer with the propellers attached. A sudden telemetry timeout, programming bug, or misconfigured RC channel could spin the motors at 100% capacity and cause severe injury. Only attach propellers when executing real outdoor field flights.
