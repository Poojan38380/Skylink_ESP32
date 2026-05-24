# Skylink GCS Upgrade — Documentation Index

Post-MVP roadmap to evolve the web dashboard into a **map-first, tap-to-fly GCS** perfected in SITL before Pixhawk hardware.

**Product decisions (locked):**

| Topic | Decision |
|-------|----------|
| Target | SITL-first until Pixhawk arrives |
| UI paradigm | **Map-first** (primary viewport) |
| Control style | Tap commands + **relative moves** (e.g. forward 5 m, yaw 90° right) |
| Missions / AUTO | **Later** (Phase 7+) |
| LED | Onboard ESP32 LED only (link / state indicator) |
| Operators | Single browser session |

---

## Documents

| Document | Contents |
|----------|----------|
| [GCS_UPGRADE_ROADMAP.md](./GCS_UPGRADE_ROADMAP.md) | **Main plan** — phases, deliverables, acceptance tests, file map |
| [ARCHITECTURE.md](./ARCHITECTURE.md) | Modular code layout, performance rules, WebSocket protocol |
| [MAVLINK_COMMANDS.md](./MAVLINK_COMMANDS.md) | Per-feature MAVLink messages and safety caps |
| [CONFIG_REFERENCE.md](./CONFIG_REFERENCE.md) | All tunable constants (firmware + browser) |
| [TESTING_STRATEGY.md](./TESTING_STRATEGY.md) | **Testing pyramid & phased test plan** |
| [OPEN_DECISIONS.md](./OPEN_DECISIONS.md) | Locked product defaults |

---

## Suggested read order

1. [OPEN_DECISIONS.md](./OPEN_DECISIONS.md) — answer or accept defaults  
2. [GCS_UPGRADE_ROADMAP.md](./GCS_UPGRADE_ROADMAP.md) — implement phase by phase  
3. [ARCHITECTURE.md](./ARCHITECTURE.md) — when touching firmware or splitting files  

---

## Link from simulation ops

Day-to-day SITL startup remains unchanged: [../successful_run_guide.md](../successful_run_guide.md).
