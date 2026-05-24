# Open Decisions — Locked Defaults

**Status:** Approved May 2026. Implemented in `include/skylink_config.h` and `data/gcs_config.js`.

| # | Decision | Value |
|---|----------|-------|
| 1 | Max single relative move | **20 m** |
| 2 | Max yaw step | **90°** (presets 45 / 90 / 180) |
| 3 | Min altitude for moves | **2 m** AGL |
| 4 | Map tiles | **OpenStreetMap** + Leaflet |
| 5 | Home position | First **3D GPS fix** |
| 6 | Click-to-fly radius | **100 m** |
| 7 | Command rate limit | **2 / s** |
| 8 | Split dashboard assets | **Yes** (`index.html`, `gcs.css`, `gcs.js`, `gcs_config.js`) |
| 9 | Arm confirmation | **Hold 1.5 s** (Phase 6 UI) |
| 10 | LED MAVLink lost | **Slow blink 500 ms** (Phase 1) |

See [CONFIG_REFERENCE.md](./CONFIG_REFERENCE.md) for file paths.
