# Quick Start — SITL on Asus Vivobook

Abbreviated checklist. Full detail: [successful_run_guide.md](./successful_run_guide.md) · Agent index: [README.md](./README.md).

---

## Before you start

- [ ] WSL mirrored networking in `~/.wslconfig` (see [simulation_test_plan.md](./simulation_test_plan.md) §1.2)
- [ ] ESP32 on same WiFi as laptop
- [ ] Firmware flashed: `pio run --target uploadfs --target upload`

---

## 1. WSL — SITL

```bash
cd ~/ardupilot/ArduCopter
sim_vehicle.py -v ArduCopter
```

Wait for `STABILIZE>` and `SERIAL2 on TCP port 5763` (no bind error).

---

## 2. Mission Planner (optional)

TCP → `127.0.0.1` → port **`5762`**

---

## 3. ESP32 dashboard (required)

1. `pio device monitor` — confirm WiFi IP
2. Browser → `http://<ESP32_IP>/`
3. Confirm **SITL LINK → MAVLink OK**

---

## 4. Fly

1. **GUIDED** → **SET MODE**
2. **ARM DRONE**
3. **AUTONOMOUS TAKEOFF** (e.g. 5 m) within 10 s of arm

---

## UI buttons (current)

| Button | WebSocket command |
|--------|-------------------|
| SET MODE | `SET_FLIGHT_MODE` |
| ARM DRONE | `ARM_DRONE` (auto-forces GUIDED) |
| AUTONOMOUS TAKEOFF | `TAKEOFF` |
| LAND | `LAND` |
| RTL | `RTL` |
| DISARM | `DISARM_DRONE` |

---

## If something breaks

See troubleshooting table in [successful_run_guide.md](./successful_run_guide.md#troubleshooting).

**Most common mistake:** `sim_vehicle.py --out=tcpin:0.0.0.0:5763` — remove it.
