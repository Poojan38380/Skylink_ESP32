# 🛰️ Quick-Start Guide: Launching SITL Simulation on Poojan's Asus Vivobook
This document provides the exact, streamlined sequence to boot up your flight simulation environment and connect all ground control systems.

---

## 📋 Pre-Flight Checklist
Before launching, ensure:
1. Your laptop is connected to your local WiFi.
2. Your physical ESP32 is powered on and connected to the same WiFi.
3. You have completed the WSL2 **Mirrored Networking** configuration (`.wslconfig`).

---

## 🚀 Step 1: Start ArduPilot SITL (Headless)
Running SITL in **headless mode** is the most stable and performant way to run it under WSL2 on your Asus Vivobook.

1. Open **VS Code** (or standard PowerShell) and enter the WSL Linux environment:
   ```powershell
   wsl
   ```
2. Navigate to your high-speed Linux home directory:
   ```bash
   cd ~
   ```
3. Move to the ArduCopter flight controller directory:
   ```bash
   cd ~/ardupilot/ArduCopter
   ```
4. Start the flight simulator with an open external TCP port:
   ```bash
   sim_vehicle.py -v ArduCopter --out=tcpin:0.0.0.0:5763
   ```
   *   **What to look for**: Wait about 10–20 seconds. Once the build finishes, you will see a scrolling command line ending with the prompt: `STABILIZE>` or `MAV>`. Your simulated flight controller is now live and accepting external TCP connections on port 5763!

---

## 🎛️ Step 2: Launch & Connect Mission Planner (GCS)
We connect Mission Planner via **TCP** to completely bypass Windows UDP port conflicts:

1. Open **Mission Planner** on your Windows desktop.
2. In the top-right corner connection options:
   *   Change the dropdown from **UDP** to **TCP**.
   *   Leave the baud rate dropdown as default.
3. Click the **CONNECT** button (top-right).
4. Enter the local loopback host IP:
   ```text
   127.0.0.1
   ```
5. Enter the TCP port:
   ```text
   5762
   ```
6. Click **OK**.
   *   **Result**: Mission Planner will download parameters from the active WSL2 simulation. Once complete, you will see your simulated quadcopter active on the map and the HUD instruments showing dynamic updates!

---

## 🔌 Step 3: Run the ESP32 & Custom GCS Dashboard
Now that your virtual drone is running in the simulator and connected to Mission Planner, let's connect your physical ESP32 and browser dashboard.

1. With your ESP32 flashed and connected to your WiFi, it will automatically search your local network and connect to your laptop at `<YOUR_LAPTOP_IP_ADDRESS>:5763` (the dynamic TCP port).
2. Check your PlatformIO Serial Monitor. You should see:
   *   `[INFO] SITL Host dynamically updated to GCS IP: <YOUR_LAPTOP_IP>`
   *   `[INFO] Connected to ArduPilot SITL Socket!`
   *   `[INFO] Stream verified. MAVLink Handshake Established!`
3. Open a web browser (like Google Chrome) on your laptop and navigate to your ESP32's IP address:
   ```text
   http://<YOUR_ESP32_IP_ADDRESS>/
   ```
4. **Congratulations!** Your glassmorphic dashboard is now live, displaying **real physics-based telemetry** (Altitude, Speed, Battery, Sats, and GPS Coordinates) streaming directly from your WSL2 flight simulator over WiFi!

---

## 🎮 Basic Flight Control Commands

### Via Custom GCS Dashboard:
*   **Arming**: Click the **⚡ ARM DRONE** button. The propeller icons will start spinning in Mission Planner, and the drone status badge will turn red and show `ARMED / FLYING`.
*   **Takeoff**: Click **🚀 CLIMB (RC TAKE OFF)** to instruct the drone to climb to a safe hover altitude.
*   **Landing**: Click **⬇ LAND DRONE** to gently land the drone.
*   **Emergency Stop**: Click **🛑 DISARM** to immediately cut the motors.

### Via MAVProxy WSL Terminal:
If you want to command the drone directly from your Linux console, type these commands in the `STABILIZE>` prompt:
*   **Change Mode to Guided**:
    ```text
    mode guided
    ```
*   **Arm the motors**:
    ```text
    arm throttle
    ```
*   **Takeoff to 10 meters**:
    ```text
    takeoff 10
    ```
