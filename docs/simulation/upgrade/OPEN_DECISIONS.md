# Open Decisions — Confirm Before Coding

Defaults are proposed below. Change any row before starting Phase 4+.

| # | Question | Proposed default | Your answer |
|---|----------|------------------|-------------|
| 1 | Max single relative move (forward/strafe) | **20 m** per command | |
| 2 | Max yaw step per command | **90°** (allow 45° / 90° / 180° presets only) | |
| 3 | Min altitude for relative moves (AGL) | **2 m** — reject if below | |
| 4 | Map tile source | **OpenStreetMap** via Leaflet (no API key) | |
| 5 | Home position on map | First **3D GPS fix** after SITL connect | |
| 6 | Click-to-fly max radius from home | **100 m** in SITL (config constant) | |
| 7 | Command rate limit (ESP32) | **2 commands / second** per client | |
| 8 | Split `index.html` into multiple LittleFS files? | **Yes** — `index.html` + `gcs.css` + `gcs.js` | |
| 9 | Arm confirmation | **Hold 1.5 s** on ARM button | |
| 10 | LED: MAVLink lost while WiFi up | **Slow blink** (500 ms) | |

---

## No further blockers

With defaults accepted, implementation can proceed without more product questions.  
Technical choices are fixed in [ARCHITECTURE.md](./ARCHITECTURE.md).
