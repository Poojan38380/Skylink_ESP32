# Flight Simulation — Verified Run Guide

**Last verified:** 24 May 2026 · ArduCopter SITL 4.8.0-dev · ESP32 Skylink bridge · Mission Planner TCP.

This is the canonical procedure. Follow the steps in order.

---

## Prerequisites (one-time)

1. **WSL2 Ubuntu** with ArduPilot built (`~/ardupilot/ArduCopter`).
2. **Mirrored networking** in `C:\Users\<you>\.wslconfig`:
   ```ini
   [wsl2]
   networkingMode=mirrored
   ```
   Then: `wsl --shutdown` and reopen WSL.
3. **PlatformIO** on Windows (`d:\btp_skylink\Skylink`).
4. PC, ESP32, and phone/laptop browser on the **same WiFi**.

---

## Port map (instance `-I0`)

| TCP port | ArduPilot serial | Client |
|----------|------------------|--------|
| **5760** | SERIAL0 | MavProxy (internal) |
| **5762** | SERIAL1 | Mission Planner |
| **5763** | SERIAL2 | Skylink ESP32 |

> **Critical:** Do **not** run `sim_vehicle.py --out=tcpin:0.0.0.0:5763`. MavProxy and ArduCopter both try to bind 5763 → `bind failed on port 5763` and SITL dies.

---

## Startup sequence (every session)

### Step 1 — WSL: start SITL

```bash
cd ~/ardupilot/ArduCopter
sim_vehicle.py -v ArduCopter
```

**Success in ArduCopter xterm:**
```text
SERIAL0 on TCP port 5760
SERIAL1 on TCP port 5762
SERIAL2 on TCP port 5763
Connection on serial port 5760
```

**Success in MavProxy:** `STABILIZE>` prompt, `Detected vehicle 1:1`, no `Connection refused`.

**If ports stuck:**
```bash
sim_vehicle.py -v ArduCopter --stop
fuser -k 5760/tcp 5762/tcp 5763/tcp 2>/dev/null; sleep 2
```

---

### Step 2 — Windows: flash ESP32 (when code/UI changed)

```powershell
cd d:\btp_skylink\Skylink
pio run --target uploadfs --target upload
pio device monitor
```

Always use **both** `uploadfs` and `upload` if `data/index.html` changed.

---

### Step 3 — Windows: Mission Planner (optional map/HUD)

1. Connection type: **TCP**
2. Host: `127.0.0.1`
3. Port: **`5762`**
4. Connect → wait for parameters and map.

---

### Step 4 — Browser: Skylink dashboard (required for ESP32↔SITL)

1. Open `http://<ESP32_IP>/` (e.g. `http://10.85.201.219/`) on the **same PC that runs WSL/SITL**.
2. Dashboard header: **LINK ACTIVE**.
3. Link panel: **SITL LINK → ● MAVLink OK**.

**Success in serial monitor:**
```text
[INFO] WebSocket client #1 connected from 10.85.201.74
[INFO] SITL host set to GCS machine: 10.85.201.74:5763
[INFO] Attempting connection to SITL at 10.85.201.74:5763
[INFO] Connected to ArduPilot SITL (TCP 5763)
```

**Success in Mission Planner messages (when arming from dashboard):**
```text
Arming motors
```

---

### Step 5 — Fly from dashboard

1. Flight mode: **GUIDED (required for arm/takeoff)**
2. Click **SET MODE**
3. Click **⚡ ARM DRONE**
4. Within **~10 seconds**, click **🚀 AUTONOMOUS TAKEOFF** → enter altitude (e.g. `5` m)
5. To recover: **⬇ LAND** or **🏠 RTL**, or **🛑 DISARM**

**Do not** arm in STABILIZE from the web UI — ArduPilot will auto-disarm on the ground.

---

## Manual control (MAVProxy in WSL)

```text
mode guided
arm throttle
takeoff 5
```

---

## Verification checklist

| Check | How |
|-------|-----|
| SITL ports bound | ArduCopter log: 5760/5762/5763, no bind error |
| MP connected | Map + HUD live on TCP 5762 |
| ESP32 WiFi | Serial: `WiFi connected! IP: …` |
| ESP32 ↔ SITL | Serial: `Connected to ArduPilot SITL (TCP 5763)` |
| Dashboard telemetry | Altitude, battery, GPS updating |
| Arm works | MP log: `Arming motors` |

**PowerShell port tests:**
```powershell
Test-NetConnection 127.0.0.1 -Port 5762
Test-NetConnection <YOUR_PC_LAN_IP> -Port 5763
```

---

## Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| `bind failed on port 5763` | `--out=tcpin:...:5763` used | Plain `sim_vehicle.py -v ArduCopter` only |
| ESP tries `127.0.0.1:5762` | Old firmware flashed | Re-run `uploadfs` + `upload` (port is **5763**) |
| `Unknown WebSocket command: SET_FLIGHT_MODE` | Old firmware | Re-flash firmware + LittleFS |
| `SITL connection failed` | Dashboard not opened / no mirrored WSL | Open dashboard on SITL PC; fix `.wslconfig` |
| `Connection reset` on 127.0.0.1 | ESP32 targeting itself | Open dashboard to set LAN IP |
| Arms then instant disarm | STABILIZE mode | GUIDED → SET MODE → ARM |
| `SIM Hit ground` / disarm | Crash or land command | Normal after failed takeoff; re-arm in GUIDED |
| Telemetry zeros | No MAVLink yet | Wait for `MAVLink OK`; streams auto-requested |

---

## Shutdown

1. Disarm in dashboard or MP.
2. WSL: `Ctrl+C` in sim_vehicle terminal, or `sim_vehicle.py -v ArduCopter --stop`.
3. Optional: close Mission Planner.

---

## Related docs

- [README.md](./README.md) — doc index for AI agents
- [agent_handoff.md](./agent_handoff.md) — design lessons and code rules
- [start_simulation.md](./start_simulation.md) — abbreviated Vivobook quick start
