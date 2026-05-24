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
4. Start the flight simulator (do **not** use `--out=tcpin:0.0.0.0:5763` — it conflicts with SITL's native SERIAL2 port):
   ```bash
   sim_vehicle.py -v ArduCopter
   ```
   *   **What to look for**: ArduCopter log shows `SERIAL2 on TCP port 5763` with no bind error, then `STABILIZE>` or `MAV>`. The ESP32 connects to **TCP 5763** on your PC's LAN IP after you open the web dashboard.

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

1. **Open the web dashboard first** on the same PC that runs SITL (`http://<ESP32_IP>/`). This sets the SITL host to your laptop's LAN IP.
2. Check PlatformIO Serial Monitor for:
   *   `[INFO] SITL host set to GCS machine: <YOUR_LAPTOP_IP>:5763`
   *   `[INFO] Connected to ArduPilot SITL (TCP 5763)`
3. The dashboard **SITL LINK** row should show `MAVLink OK`.
4. If you have not opened the dashboard yet, navigate to:
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
