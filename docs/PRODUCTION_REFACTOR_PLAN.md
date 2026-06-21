# Skylink v1 Production Refactor Plan

Date: 2026-06-21  
Branch target: `refactor/production-safety-phase-1` and follow-up phase branches  
Primary goal: turn the v1 working prototype into a production-grade, safety-first internet-capable drone GCS after simulation, bench, and staged real-world validation.

## Executive summary

Skylink v1 is still the better base because it is the known working system and its behavior is easier to reason about. Skylink v2 contains several valuable upgrade ideas, but it should not be copied wholesale. v2 improves architecture, safety modeling, WiFi behavior, authentication concept, UI direction, and test discipline. It also introduces new concurrency and security risks that are not acceptable for flight-critical control.

The refactor strategy is therefore:

1. Keep v1 as the base.
2. Fix immediate safety bugs first.
3. Pull proven v2 ideas into v1 one at a time.
4. Verify each phase in SITL before hardware.
5. Move to props-off bench tests, then restrained/tethered tests, then very limited field tests.
6. Do not rely on internet control as the only recovery/control path until failsafe behavior has been proven.

## Non-negotiable safety stance

Internet drone control has different failure modes than RC:

- latency and jitter can spike unpredictably;
- cellular NAT/carrier behavior can break sessions;
- WiFi/cellular reconnect code can starve flight-control loops if written badly;
- authentication mistakes can expose command channels;
- browser UI state can be stale or optimistic;
- the companion computer can lose link while the autopilot continues flying.

For real drone tests, software must not be the reason for a crash. The autopilot must remain the final authority, and Pixhawk failsafes must be configured and tested independently of Skylink.

Recommended operational rule:

- Skylink may become the primary GCS only after staged validation.
- A proven recovery path must exist before distance tests: RC, RTL, geofence, GCS failsafe, battery failsafe, EKF/GPS failsafe, and a tested lost-link behavior.
- Do not disable safety checks for convenience except in controlled bench-only contexts, and document/revert every temporary parameter change.

## What v2 did better than v1

### 1. Better task separation

v2 splits MAVLink, safety monitoring, and web/UI work into separate FreeRTOS tasks. That is the right direction because v1 currently runs too much in a single loop. In v1, blocking WiFi reconnect, NTP sync, OTA handling, or filesystem/web behavior can delay MAVLink processing.

What to adopt:

- eventually isolate MAVLink RX/TX into a high-priority task;
- keep web/UI work away from flight-critical loops;
- use bounded queues between command ingestion and MAVLink transmission.

What not to copy blindly:

- v2 allows multiple tasks to write MAVLink bytes, which risks interleaved/corrupt MAVLink packets. The production design should have one MAVLink TX owner.

### 2. Explicit safety monitor concept

v2 adds a SafetyMonitor with states and command gates. This is a major conceptual improvement over v1, where safety checks are scattered across command handlers and browser UI.

What to adopt:

- a central command validator;
- explicit states such as disconnected, preflight, ready, armed, flying, landing, failsafe;
- command rejection reasons surfaced to the UI;
- state transition tests.

What to fix:

- shared state must be protected by mutex/atomic access;
- failsafe states must handle weird armed/non-GUIDED cases explicitly;
- command lifecycle must be ACK/state driven, not timing driven.

### 3. Non-blocking WiFi reconnect idea

v2’s WiFi manager is closer to what production needs because it avoids long blocking loops during reconnect. v1 can currently block for seconds while attempting WiFi networks, starving the rest of the main loop.

What to adopt:

- non-blocking WiFi state machine;
- time-budgeted connection attempts;
- no long `delay()` calls in flight-critical runtime.

### 4. Authentication concept

v2 introduces login/session/token behavior. v1 has no authentication and exposes WebSocket commands to any same-network client.

What to adopt:

- authentication before command access;
- command authorization separate from static asset serving;
- maintenance-only OTA;
- no unauthenticated config leaks.

What to redesign:

- no hardcoded production password;
- no tokens in logs;
- avoid token in query strings where practical;
- rate-limit auth attempts;
- do not store bearer tokens in `localStorage`;
- prefer a secure deployment architecture for internet use, not raw ESP32 exposure.

### 5. Better UI direction

v2 has a cleaner UI direction: login page, modal flows, clearer state panels, and better action grouping. v1 UI was useful as a prototype but is not production-grade.

What to adopt:

- cockpit-style layout with clear flight state hierarchy;
- explicit readiness checklist;
- dangerous actions behind confirm flows;
- command progress: queued, sent, ACK accepted/denied, state confirmed;
- disabled buttons based on firmware-reported gate state, not browser guesses.

### 6. Smaller movement cap

v2 reduces guided body movement from v1’s 200 m cap to 20 m. That is much safer for early field testing.

What to adopt:

- conservative default movement caps;
- separate simulation and hardware limits;
- explicit operator confirmation for larger movements.

### 7. Test discipline

v2 includes unit-test direction and ground/field protocols. v1 needs this discipline before real-world refactoring.

What to adopt:

- build checks for SITL and hardware environments;
- command validation tests;
- simulated MAVLink scenarios;
- manual test checklists that match code behavior.

## Critical v1 problems found

### 1. Emergency stop deadlock

`FlightController::emergencyStop()` takes the flight-controller mutex and then calls `sendRCOverride()`, which tries to take the same non-recursive mutex again. This can deadlock before the force-disarm command is sent.

Risk: emergency stop may freeze instead of disarming.

Fix direction: add an unlocked RC override sender used only while the mutex is already held, or restructure emergency stop so no nested lock is taken.

### 2. Blocking WiFi reconnect can starve MAVLink

`WiFiManager::reconnectDirect()` uses blocking delays and loops. Initial WiFi connect can also block for a long time per configured network.

Risk: MAVLink handling, heartbeat, telemetry, and command processing can be delayed during network instability.

Fix direction: replace with a non-blocking connection state machine in a later phase. Phase 1 documents this but does not rewrite WiFi yet unless needed for safety smoke tests.

### 3. Blocking time sync

`getLocalTime(..., 5000)` can block for up to five seconds.

Risk: main loop starvation during unsynced or poor-network operation.

Fix direction: make time sync non-blocking or isolate it from flight-critical loop.

### 4. Single-loop architecture

v1’s main loop sequentially runs WiFi, OTA, flight controller, telemetry send, and LED logic.

Risk: any blocking subsystem affects flight-control communication.

Fix direction: later split into bounded work units/tasks. MAVLink must get priority.

### 5. Takeoff command lacks firmware-level gates

v1 takeoff currently checks only MAVLink activity before sending `MAV_CMD_NAV_TAKEOFF`.

Risk: UI mistakes or stale browser state can send takeoff when not armed, not GUIDED, no GPS, already flying, or with unsafe altitude.

Fix direction: firmware must reject unsafe takeoff before sending MAVLink.

### 6. AGL fallback bug

Guided movement uses `relative_alt` when positive, otherwise falls back to `altitude`. `altitude` is not a safe substitute for AGL.

Risk: ground/low-altitude movement can be allowed due to barometric/MSL altitude.

Fix direction: require valid positive relative altitude for movement gates; do not fall back to `altitude`.

### 7. UI ARM/TAKEOFF sequencing is timing-based

The browser sends GUIDED then ARM after a fixed 400 ms delay. It does not wait for autopilot ACK/state confirmation.

Risk: commands can execute out of order or under stale assumptions.

Fix direction: command lifecycle must be ACK/state driven.

### 8. UI `prompt()` for autonomous takeoff

Blocking browser prompt is not production-grade and can freeze UX flow.

Fix direction: replace with a modal that shows readiness, risks, and exact command outcome.

### 9. Browser-side preflight is advisory only

The UI shows readiness, but firmware does not consistently enforce the same gate set.

Risk: browser bugs can bypass safety.

Fix direction: firmware is the authority; UI mirrors firmware gate state.

### 10. Cached heartbeat can look like stable autopilot state

Browser stability counters update from WebSocket telemetry frequency, not true independent autopilot heartbeat/state transitions.

Risk: repeated cached telemetry can look like confirmed stability.

Fix direction: track fresh autopilot heartbeat age and state-confirm timestamps.

### 11. Unknown flight mode defaults to STABILIZE

Malformed `SET_FLIGHT_MODE` commands currently parse to STABILIZE.

Risk: typo or malicious command can change flight mode unexpectedly.

Fix direction: reject unknown modes instead of defaulting.

### 12. Raw RC override exposed

v1 accepts raw RC override commands over unauthenticated WebSocket.

Risk: unsafe PWM values or unauthorized clients can directly affect controls.

Fix direction: remove from normal UI path or heavily gate/range-limit it; keep only maintenance/debug mode if needed.

### 13. No authentication

Any same-network client can command the aircraft.

Risk: unacceptable for internet or shared-network use.

Fix direction: introduce authentication and isolate command access before any internet exposure.

### 14. OTA always enabled without password

OTA is active in the flight runtime with no password.

Risk: field attack surface and runtime instability.

Fix direction: maintenance-only OTA with authentication or compile-time disable in flight builds.

### 15. MAVLink link validity is too broad

v1 marks MAVLink active on any parsed MAVLink packet before confirming it came from the intended autopilot heartbeat/source.

Risk: wrong-source packets can make the system believe the flight controller is connected.

Fix direction: only vehicle-heartbeat/source-validated traffic should establish connection; telemetry handlers should filter source.

### 16. Telemetry source filtering incomplete

Most telemetry handlers update state from any MAVLink message without checking sysid/compid.

Risk: companion/noise/wrong-system messages can corrupt UI state and safety decisions.

Fix direction: filter all vehicle telemetry to expected sysid/compid.

### 17. GPS_RAW_INT can overwrite fused position

`GPS_RAW_INT` updates latitude/longitude, while `GLOBAL_POSITION_INT` is the EKF-fused source.

Risk: map/UI can jump or use less appropriate source.

Fix direction: use `GLOBAL_POSITION_INT` for position; use `GPS_RAW_INT` for fix/sats only.

### 18. Stream request is not periodically renewed

The stream request interval constant exists but is not meaningfully used.

Risk: telemetry streams can degrade after reconnect or autopilot behavior changes.

Fix direction: periodically renew stream request while connected.

### 19. JSON buffer tightness

The telemetry buffer may become tight as fields grow.

Risk: dropped WebSocket JSON under richer telemetry/event payloads.

Fix direction: increase buffer and monitor serialization failures.

### 20. Movement and geofence defaults need hardware profiles

Some defaults are too broad for early real tests.

Risk: accidental large movements.

Fix direction: conservative hardware defaults, separate simulation profile, explicit escalation for larger limits.

### 21. GOTO silently switches to GUIDED

`GOTO_LATLON` silently sets GUIDED if not already there.

Risk: mode changes can happen as a side effect of a movement command.

Fix direction: require GUIDED explicitly or command confirmation flow.

### 22. Risky documentation guidance

Some docs mention disabling safety checks or holding a drone while issuing takeoff-like flows.

Risk: unsafe procedures get repeated during testing.

Fix direction: revise docs toward props-off, motor test, SITL, and tethered procedures.

### 23. Build identity split-brain

Firmware build, filesystem build, and frontend config must be manually kept in sync.

Risk: browser and firmware mismatch after flashing.

Fix direction: generated build identity or automated validation.

## v2 problems to avoid copying

1. Multiple tasks can write MAVLink bytes. Production should have exactly one MAVLink TX owner.
2. Shared state is accessed across tasks without sufficient mutex/atomic protection.
3. Authentication defaults are weak: hardcoded password, token logging, token in localStorage/query string, no rate limiting.
4. Static assets/configs can still be exposed despite login redirects.
5. Link validity is still too broad.
6. Unknown mode parsing can still default to STABILIZE-like behavior.
7. Command lifecycle is not fully ACK/state driven.
8. Queue/mutex creation failures are not checked everywhere.
9. Snapshot failure can return zeroed safe-looking dummy state.
10. `LittleFS.begin(true)` can erase filesystem on mount failure, which is dev-friendly but field-dangerous.
11. Tests are useful but limited.

## Phase plan

### Phase 0 — Baseline and safety envelope

Goal: freeze what currently works and define the safe test envelope.

Actions:

- build both SITL and hardware environments;
- build filesystem image;
- document branch, build numbers, and test commands;
- record current expected Pixhawk parameters;
- decide real-world test limits: max altitude, radius, movement step, link-loss behavior.

Verification:

- both firmware envs build;
- filesystem image builds;
- operator checklist exists.

Status: baseline builds were verified before Phase 1.

### Phase 1 — Immediate flight-safety bug fixes

Goal: eliminate command-path bugs that could directly contribute to unsafe behavior.

Actions:

- fix emergency-stop mutex deadlock;
- reject TAKEOFF unless firmware sees safe state;
- remove AGL fallback to barometric/MSL altitude;
- reject unknown flight modes instead of defaulting to STABILIZE;
- stop `GPS_RAW_INT` from overwriting fused position;
- periodically renew data-stream requests;
- surface firmware rejections as UI events where possible.

Verification:

- SITL and hardware firmware builds pass;
- manual SITL check confirms rejected commands produce clear events/logs;
- emergency stop path cannot self-deadlock.

### Phase 2 — Non-blocking runtime hygiene

Goal: remove main-loop starvation sources.

Actions:

- make WiFi reconnect non-blocking;
- make time sync non-blocking;
- disable or isolate OTA in flight builds;
- audit all `delay()` and blocking calls;
- add loop latency instrumentation.

Verification:

- simulated WiFi loss does not halt MAVLink handling;
- telemetry heartbeat remains stable during reconnect attempts.

Status after Phase 2 implementation:

- [x] Initial WiFi connection no longer performs a blocking scan/connect loop.
- [x] WiFi reconnect is now a non-blocking priority-ordered state machine.
- [x] Time sync checks now use zero-timeout `getLocalTime()` calls.
- [x] Unsynced time retries happen every second without blocking MAVLink handling.
- [x] OTA startup is deferred until WiFi is connected.
- [x] OTA handler is serviced only while the vehicle is disarmed.
- [x] SITL TCP reconnect attempts are skipped while WiFi is disconnected.
- [x] SITL TCP reconnect attempts use a short timeout to avoid multi-second stalls while WiFi status is stale.
- [x] SITL and hardware firmware builds pass.

### Phase 3 — Central safety monitor and command validator

Goal: make firmware, not the browser, the source of safety truth.

Actions:

- create central command validation layer;
- define state machine;
- expose gate state and reject reasons to UI;
- make ARM, TAKEOFF, MOVE, GOTO, LAND, RTL, LOITER rules explicit.
- treat browser/GCS state as stale after WebSocket reconnect until fresh firmware and autopilot telemetry has arrived;
- rate-limit or deduplicate repeated dangerous commands such as RTL/LAND/ARM/TAKEOFF;
- require an active GCS/client heartbeat window for non-emergency commands.

Verification:

- tests for every command/state combination;
- UI buttons follow firmware gates.

Status after Phase 3 slice 1:

- [x] Added a central WebSocket command validator before command dispatch.
- [x] Flight commands are rejected during the post-reconnect settle window.
- [x] Flight commands are rejected when MAVLink is not active.
- [x] Duplicate repeated flight commands are rejected for a short dedupe window.
- [x] Flight commands are rate-limited to reduce button-spam and reconnect surprises.
- [x] `EMERGENCY_STOP`, `PING`, and LED commands remain exempt from the flight-command gate.
- [x] Rejection reasons are sent back to the dashboard as `ERROR` events and logged on serial.
- [x] Command validation uses a longer MAVLink mutex wait to avoid false `no active MAVLink link` rejects during brief MAVLink processing contention.
- [x] SITL and hardware firmware builds pass.

Remaining Phase 3 work:

- define a richer firmware safety state machine;
- add broader command/state tests.

Status after Phase 3 slice 2:

- [x] Heartbeat telemetry exposes firmware command gate state and reason.
- [x] Heartbeat telemetry exposes per-action permissions for set mode, arm, disarm, takeoff, land, RTL, loiter, move, goto, and emergency stop.
- [x] Dashboard flight buttons now use firmware-provided permissions instead of browser-only assumptions.
- [x] Dashboard keeps flight controls disabled after WebSocket open until the first heartbeat provides command permissions.
- [x] WebSocket JSON buffer increased to fit the richer heartbeat payload.
- [x] JS syntax check, SITL firmware build, hardware firmware build, and LittleFS build pass.

Status after Phase 3 slice 3:

- [x] Firmware now publishes an explicit safety state in heartbeat telemetry.
- [x] Safety states currently include `DISCONNECTED`, `SETTLING`, `PREFLIGHT`, `READY_TO_ARM`, `ARMED_GROUND`, `FLYING`, `LANDING`, and `FAILSAFE_OR_STALE`.
- [x] Firmware publishes `safety_reason` alongside the state.
- [x] Command permissions are derived from the safety state instead of independent browser-side assumptions.
- [x] Dashboard header shows a safety-state chip.
- [x] Dashboard preflight banners include the firmware safety state and reason.
- [x] JS syntax check, SITL firmware build, hardware firmware build, and LittleFS build pass.

### Phase 4 — MAVLink robustness

Goal: make MAVLink source filtering and command lifecycle production-grade.

Actions:

- filter all vehicle telemetry by expected sysid/compid;
- track autopilot heartbeat freshness separately from any MAVLink byte;
- add command pending table;
- correlate COMMAND_ACK with sent commands;
- confirm state transitions after accepted ACKs;
- prevent duplicate/dangling command sends.

Verification:

- SITL tests for ACK accepted/denied/timeout;
- wrong-source packet tests do not alter state.

Status after Phase 4 slice 1:

- [x] Autopilot heartbeat freshness is tracked separately from generic MAVLink traffic.
- [x] Heartbeat telemetry now publishes `autopilot_heartbeat_fresh` and `autopilot_heartbeat_age_ms`.
- [x] Command gate now requires a fresh autopilot heartbeat, not only a recently active MAVLink stream.
- [x] Dashboard MAV chip distinguishes live MAVLink from stale autopilot heartbeat.
- [x] Preflight MAV item requires fresh autopilot heartbeat.
- [x] Status stream text shows autopilot heartbeat age/staleness.
- [x] JS syntax check, SITL firmware build, hardware firmware build, and LittleFS build pass.

Status after Phase 4 slice 2:

- [x] Added a one-slot firmware command lifecycle tracker for ACK-capable `COMMAND_LONG` sends.
- [x] Tracked commands currently include mode changes, ARM/DISARM, TAKEOFF, LAND, RTL, LOITER, yaw, and emergency disarm.
- [x] `COMMAND_ACK` is correlated with the currently pending MAVLink command id.
- [x] Pending commands move to `ACCEPTED`, `REJECTED`, or `TIMEOUT`.
- [x] Pending commands are failed as `TIMEOUT` when WiFi, SITL, or MAVLink link loss makes the ACK unknowable.
- [x] Heartbeat telemetry exposes `command_pending`, `command_name`, `command_status_name`, `command_result`, `command_mav_id`, and `command_age_ms`.
- [x] Dashboard logs command lifecycle transitions so the operator can see pending/accepted/rejected/timeout events.
- [x] This slice intentionally does not yet make the browser command flow ACK-driven; the dashboard can see lifecycle state, but sequencing still needs the next scheduler/state-confirmation slice.
- [x] Position-target movement messages are not ACK-tracked because they do not use `COMMAND_ACK`.

Status after Phase 4 slice 3:

- [x] Dashboard static assets now use a versioned query string so stale browser JS is easier to avoid after `uploadfs`.
- [x] LittleFS build was bumped to 18 across firmware config, browser config, and `skylink_build.json`.
- [x] Browser command gate snapshots now expire, preventing commands from being sent from stale heartbeat state.
- [x] Browser blocks normal flight commands while firmware reports an ACK-capable command pending.
- [x] Browser applies client-side command spacing before normal flight commands.
- [x] Manual ARM flow now waits for SET_MODE ACK, GUIDED state confirmation, ARM ACK, and ARMED state confirmation.
- [x] Autonomous takeoff now waits for SET_MODE ACK, GUIDED state confirmation, ARM ACK, ARMED+GUIDED state confirmation, TAKEOFF ACK.
- [x] Removed the old timer-only takeoff state machine.

Status after Phase 4 slice 4:

- [x] Fixed a browser ACK race by registering ACK waiters before transmitting commands.
- [x] Browser now marks ACK-capable commands locally pending immediately after TX, instead of waiting for the next heartbeat.
- [x] Firmware command validation now rejects overlapping normal flight commands while an ACK-capable command is pending.
- [x] Autonomous takeoff is state-aware: it skips redundant mode/arm commands when already GUIDED/armed on ground.
- [x] Autonomous takeoff refuses to start if the vehicle already appears airborne.
- [x] Browser command-gate staleness window is aligned to a realistic telemetry/UI window instead of a 1.2s false-failure window.
- [x] `can_takeoff` now matches the autonomous takeoff behavior: allowed from disarmed ready-to-arm or armed-ground GUIDED states.
- [x] Developer and indoor motor test docs now describe the ACK/state-driven 6-step takeoff flow.

Status after Phase 4 slice 5:

- [x] Manual mode changes now use an ACK/state-confirmed sequence instead of raw `sendCmd`.
- [x] LAND, RTL, and LOITER dashboard actions now use ACK/state-confirmed mode sequencing.
- [x] Yaw commands now use the browser ACK helper instead of fire-and-forget logging.
- [x] Browser has a general command-in-progress mutex so LAND/RTL/LOITER/mode/yaw cannot overlap with arm/takeoff.
- [x] Firmware now accepts `STABILIZE` from the dashboard mode dropdown instead of rejecting a visible UI option.
- [x] Firmware build bumped to 13 and FS build bumped to 19 for this command-lifecycle slice.

Status after Phase 4 slice 6:

- [x] Replaced unsafe ArduPilot LOITER-mode button behavior with a GUIDED hold-position command at the current lat/lon/relative altitude.
- [x] LOITER/Hold now requires armed GUIDED flight, GPS 3D, and minimum safe AGL before it is enabled/sent.
- [x] LAND and RTL remain explicit autopilot recovery mode changes with ACK/state confirmation.
- [x] Firmware build bumped to 14 and FS build bumped to 20 for the LOITER/Hold safety correction.

Remaining Phase 4 work:

- Optional later hardening: replace the one-slot pending tracker with a small bounded pending command table if overlapping ACK-capable commands are needed.
- Optional later hardening: add automated wrong-source packet and ACK-timeout tests.

Deferred after Phase 6:

- Phase 4 optional hardening:
  - replace the one-slot pending tracker with a bounded pending command table if overlapping ACK-capable commands become necessary;
  - add automated wrong-source packet tests;
  - add automated ACK accepted/rejected/timeout tests.
- Phase 5 security/authentication:
  - add authentication before WebSocket commands;
  - remove unauthenticated sensitive static/config exposure;
  - disable or protect unauthenticated OTA;
  - add auth rate limiting and lockout;
  - remove hardcoded production credentials;
  - document secure deployment model for internet/cellular use.

### Phase 5 — Security/authentication

Goal: make command access safe enough for controlled network testing.

Actions:

- add authentication before WebSocket commands;
- remove unauthenticated sensitive static/config exposure;
- disable unauthenticated OTA;
- add rate limiting and lockout for auth;
- avoid hardcoded production credentials;
- document secure deployment model for internet use.

Verification:

- unauthenticated client cannot command aircraft;
- token/password does not appear in serial logs;
- failed login attempts are rate-limited.

### Phase 6 — UI redesign

Goal: replace prototype UI with a modern, clean, robust GCS interface.

Actions:

- design cockpit layout around state, map, command timeline, checklist, and alerts;
- replace prompts with modals;
- show command lifecycle and reject reasons;
- separate normal pilot controls from maintenance/debug controls;
- make dangerous actions visually distinct and confirmation-gated.
- increase the number of visible dashboard logs/status messages;
- keep a browser-side offline log/command backlog so the operator can review what happened while the network was gone;
- resync and display missed firmware/autopilot events after reconnect where feasible.

Verification:

- UI can be operated under stress without ambiguity;
- disabled/enabled state matches firmware gates;
- mobile/tablet/desktop layouts tested.

Logging requirement added after Phase 2 WiFi-loss test:

- The dashboard must show more than the current small message window.
- The operator should still see locally-known logs after WebSocket loss.
- After reconnect, the UI should make it obvious which commands were sent before loss, which were not sent, and what firmware/autopilot events arrived after reconnection.

Status after Phase 6 slice 1:

- [x] Dashboard log visible window increased from 40 to 120 entries.
- [x] Browser now persists the last 300 local log entries in `localStorage`.
- [x] Local logs are restored after refresh/reconnect so network-loss context remains visible.
- [x] Commands attempted while WebSocket is down now log an explicit `not sent: WebSocket disconnected` message.
- [x] Log tab height increased and made scrollable for longer debugging sessions.
- [x] FS build bumped to 21 for persistent-log dashboard assets.

### Phase 7 — Simulation campaign

Goal: prove behavior in SITL before hardware.

Actions:

- scripted SITL scenarios:
  - no MAVLink;
  - no GPS;
  - not GUIDED;
  - not armed;
  - takeoff accepted/denied;
  - link drop;
  - reconnect;
  - RTL/LAND;
  - geofence rejection;
  - battery failsafe simulation where possible.

Verification:

- scenarios pass repeatedly;
- logs are reviewed;
- no browser-only safety dependency remains.

### Phase 8 — Bench and props-off hardware

Goal: verify hardware wiring and Pixhawk behavior without thrust risk.

Actions:

- props removed;
- Pixhawk powered safely;
- TELEM2 MAVLink verified;
- GCS connects over WiFi;
- mode/arm behavior checked carefully;
- motor test only through safe protocol;
- failsafe parameters verified.

Verification:

- no unexpected mode/arm/motor behavior;
- emergency stop sends force-disarm path;
- logs match expected command lifecycle.

### Phase 9 — Tethered and limited field tests

Goal: staged real-world proof without uncontrolled distance risk.

Actions:

- very low altitude;
- small radius;
- conservative movement steps;
- tested RTL;
- tested link-loss behavior;
- second operator ready if possible;
- no long-range/cellular-only test until WiFi/local tests are boringly reliable.

Verification:

- successful repeatable flights;
- no unexpected command, mode, or failsafe behavior;
- post-flight logs reviewed before expanding envelope.

## Immediate Phase 1 checklist

- [x] Add plan document.
- [x] Create phase-1 branch.
- [x] Fix emergency-stop deadlock.
- [x] Add firmware TAKEOFF gates.
- [x] Require relative altitude for guided movement.
- [x] Reject unknown `SET_FLIGHT_MODE`.
- [x] Keep fused position from `GLOBAL_POSITION_INT`.
- [x] Renew stream requests periodically.
- [x] Emit firmware rejection events to browser.
- [x] Build SITL and hardware firmware.

## Notes for future UI design

The production UI should feel like a flight deck, not a demo page. The information hierarchy should be:

1. aircraft state and safety gate;
2. link quality and heartbeat freshness;
3. map and position;
4. command timeline;
5. pilot controls;
6. maintenance/debug controls.

Dangerous controls should never be visually casual. ARM, TAKEOFF, DISARM, EMERGENCY STOP, RTL, LAND, and GOTO need clear affordances, confirmation where appropriate, and firmware-backed status.

## Open questions before internet/cellular tests

1. What exact Pixhawk firmware/version and parameters will be used?
2. What is the intended failsafe if ESP32 loses internet but Pixhawk is still flying?
3. Will an RC transmitter remain active as backup during early tests?
4. What maximum altitude/radius is acceptable for first field tests?
5. What modem/SIM topology will be used: direct public IP, VPN, relay server, or tunnel?
6. How will logs be collected after failed/partial flights?

These should be answered before moving beyond local WiFi and SITL.
