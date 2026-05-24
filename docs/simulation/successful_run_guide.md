# 🛰️ Flight Simulation Successful Run Guide

This document contains the exact, verified sequence and commands required to start your ArduPilot SITL simulation, flash your ESP32 flight controller bridge, and operate the Ground Control Station dashboard successfully.

---

## 📋 Standard Operational Sequence

Due to ArduPilot's safety watchdogs and failsafes, you **must** follow this exact sequence to arm and fly the drone without triggering safety disarms:

1. **Boot SITL in WSL2** (exposing external TCP bindings).
2. **Boot the ESP32 & Connect WebSocket Client** (browser opens GCS dashboard).
3. **Connect Mission Planner** via TCP to view the 3D map.
4. **Transition to GUIDED Mode** (required for autonomous takeoff and safe ground arming).
5. **Arm the Autopilot**.
6. **Trigger Takeoff Immediately** (must be within 10 seconds of arming to prevent safety auto-disarm).

---

## 💻 1. Critical Commands: WSL2 ArduPilot SITL

Run these commands inside your **WSL Ubuntu** terminal:

```bash
# 1. Navigate to the ArduCopter vehicle directory
cd ~/ardupilot/ArduCopter

# 2. Launch the simulator binding to all interfaces (exposes Serial 2 on TCP 5763)
sim_vehicle.py -v ArduCopter -A --sim-address=0.0.0.0
```

### Manual Flight Commands (Inside MAVProxy Terminal)
If you are testing directly in the WSL terminal, run this exact sequence to fly:
```text
# 1. Switch to Guided mode
mode guided

# 2. Arm the throttle
arm throttle

# 3. Takeoff immediately to 5 meters
takeoff 5
```

---

## 🔌 2. Critical Commands: PlatformIO & ESP32

Run these commands in your Windows terminal inside the project directory `d:\btp_skylink\Skylink`:

### Combined Compile & Upload (Firmware + Dashboard HTML)
Always run both targets to ensure that your C++ code and the LittleFS browser interface are fully in sync:
```powershell
# Compile and flash the C++ firmware AND upload the LittleFS files
pio run --target uploadfs --target upload
```

### Monitor Serial Outputs
```powershell
# Open the serial monitor (closes automatically when uploading)
pio device monitor
```

---

## 🎛️ 3. Connecting Mission Planner (GCS)

1. Open **Mission Planner** on Windows.
2. In the top-right corner, select **TCP** from the dropdown (do **not** use UDP).
3. Click **CONNECT**.
4. Enter IP address: `127.0.0.1`
5. Enter Port: `5762`
6. Click **OK**.

---

## 🌐 4. Custom GCS Web Dashboard Sequence

1. Open Google Chrome and navigate to your ESP32's IP:
   ```text
   http://10.85.201.219/
   ```
2. Verify that **live telemetry** (Altitude, Battery, Speed, Sats, GPS Coordinates) instantly populates and updates at 4Hz.
3. In the **AUTOPILOT FLIGHT MODE** dropdown, select **GUIDED (AUTO CONTROL)**.
4. Click **⚡ ARM DRONE**.
   * *Verify that the Link Status shows `● ARMED / FLYING` in red and the LED indicator turns yellow.*
5. Click **🚀 AUTONOMOUS TAKEOFF** and enter `5` (meters).
   * *The virtual quadcopter will climb to 5 meters. Observe the altitude climbing live on your GCS screen!*
6. Click **⬇ LAND DRONE** or **🏠 RTL (HOME)** to autonomously return the vehicle safely to the ground.
