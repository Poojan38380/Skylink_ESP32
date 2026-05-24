# 🛰️ Flight Simulation Successful Run Guide

This document contains the exact, verified sequence and commands required to start your ArduPilot SITL simulation, flash your ESP32 flight controller bridge, and operate the Ground Control Station dashboard successfully.

---

## 📋 Standard Operational Sequence

Due to ArduPilot's safety watchdogs and failsafes, you **must** follow this exact sequence to arm and fly the drone without triggering safety disarms:

1. **Enable WSL mirrored networking** (one-time, see `simulation_test_plan.md` Step 1.2).
2. **Boot SITL in WSL2** with TCP port **5763** exposed for the ESP32.
3. **Flash ESP32** (`uploadfs` + `upload`) and open the serial monitor.
4. **Open the Skylink dashboard** in Chrome (`http://<ESP32_IP>/`) — this sets the SITL host to your PC's LAN IP.
5. **Connect Mission Planner** via TCP `127.0.0.1:5762` (separate port — no conflict).
6. Wait for dashboard **SITL LINK → MAVLink OK**.
7. Select **GUIDED** mode → **SET MODE** → **ARM** → **AUTONOMOUS TAKEOFF** within 10 seconds of arming.

---

## 💻 1. Critical Commands: WSL2 ArduPilot SITL

Run these commands inside your **WSL Ubuntu** terminal:

```bash
cd ~/ardupilot/ArduCopter

# Default SITL — ArduCopter already opens TCP 5763 (SERIAL2) for the ESP32.
# Do NOT add --out=tcpin:0.0.0.0:5763 (that steals the port and SITL will fail to bind).
sim_vehicle.py -v ArduCopter
```

> **Port map (instance 0):** `5760` = MavProxy · `5762` = Mission Planner (SERIAL1) · `5763` = Skylink ESP32 (SERIAL2)

If you see `bind failed on port 5763 - Address already in use`, kill stale processes first:

```bash
sim_vehicle.py -v ArduCopter --stop
# or: fuser -k 5760/tcp 5762/tcp 5763/tcp 2>/dev/null; sleep 1
```

### Manual Flight Commands (Inside MAVProxy Terminal)

```text
mode guided
arm throttle
takeoff 5
```

---

## 🔌 2. Critical Commands: PlatformIO & ESP32

Run in PowerShell from `d:\btp_skylink\Skylink`:

```powershell
pio run --target uploadfs --target upload
pio device monitor
```

---

## 🎛️ 3. Connecting Mission Planner (GCS)

1. Open **Mission Planner** on Windows.
2. Select **TCP** (not UDP).
3. Click **CONNECT** → IP: `127.0.0.1` → Port: **`5762`**
4. Click **OK**.

Do **not** use port 5763 — that is reserved for the ESP32 bridge.

---

## 🌐 4. Custom GCS Web Dashboard Sequence

1. Open Chrome: `http://10.85.201.219/` (use your ESP32's IP).
2. Confirm **SITL LINK** shows `MAVLink OK` (may take up to 10s after SITL starts).
3. Select **GUIDED** → click **SET MODE**.
4. Click **⚡ ARM DRONE**.
5. Click **🚀 AUTONOMOUS TAKEOFF** → enter `5` (meters). Takeoff within 10s of arming.
6. Use **⬇ LAND** or **🏠 RTL** to recover.

---

## 🛠️ Troubleshooting

| Symptom | Fix |
|---------|-----|
| ESP tries `127.0.0.1` | Open the dashboard first (sets LAN IP from your browser) |
| `bind failed on port 5763` | Remove `--out=tcpin:...:5763`; run plain `sim_vehicle.py -v ArduCopter` |
| `SITL connection failed` | Enable WSL mirrored networking; open dashboard first; verify port 5763 |
| Mission Planner works, ESP32 doesn't | MP uses 5762; ESP32 must use **5763** |
| Arms then instant disarm | Use **GUIDED** mode, not STABILIZE |
| Telemetry all zeros | Wait for MAVLink OK; streams are requested automatically |

Verify from Windows PowerShell:

```powershell
Test-NetConnection 127.0.0.1 -Port 5762
Test-NetConnection <YOUR_PC_LAN_IP> -Port 5763
```
