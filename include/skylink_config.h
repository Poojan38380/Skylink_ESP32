#ifndef SKYLINK_CONFIG_H
#define SKYLINK_CONFIG_H

#include <Arduino.h>

// =============================================================================
// Skylink tunables — edit here (see docs/simulation/upgrade/CONFIG_REFERENCE.md)
// =============================================================================

// --- Protocol ---
#define SKYLINK_PROTOCOL_VERSION        1
#define SKYLINK_JSON_BUFFER_SIZE        2048

// --- WebSocket / dashboard telemetry ---
#define SKYLINK_WS_TELEMETRY_INTERVAL_MS  200   // 5 Hz to browser (avoids AsyncWS queue overflow)
#define SKYLINK_WS_TELEMETRY_HZ           5
#define SKYLINK_WS_MAX_EVENTS_PER_LOOP    3
#define SKYLINK_STATUSTEXT_RING_LINES     5
#define SKYLINK_STATUSTEXT_MAX_LEN        50
#define SKYLINK_FC_EVENT_QUEUE_SIZE       6
#define SKYLINK_WS_LINK_STATUS_INTERVAL_MS  1000
#define SKYLINK_WS_RECONNECT_SETTLE_MS    1500
#define SKYLINK_WS_FLIGHT_CMD_MIN_INTERVAL_MS  250
#define SKYLINK_WS_FLIGHT_CMD_DEDUPE_MS   1500

// --- SITL / MAVLink (flight_controller) ---
#define SKYLINK_SITL_TCP_PORT           5763
#define SKYLINK_SITL_CONNECT_TIMEOUT_MS  3000
#define SKYLINK_SITL_RECONNECT_TIMEOUT_MS  250
#define SKYLINK_SITL_RECONNECT_INTERVAL_MS  5000
#define SKYLINK_MAVLINK_GCS_HEARTBEAT_MS    1000
#define SKYLINK_MAVLINK_STREAM_REQUEST_MS   10000
#define SKYLINK_MAVLINK_TIMEOUT_MS          5000
#define SKYLINK_MAVLINK_VEHICLE_SYSID       1
#define SKYLINK_MAVLINK_VEHICLE_COMPID      1

// --- Flight safety caps (enforced in firmware from Phase 4+) ---
#define SKYLINK_MOVE_BODY_MAX_M         200.0f
#define SKYLINK_GOTO_ALT_MAX_M          30.0f   // capped at 30 m for low-altitude field testing
#define SKYLINK_MOVE_MIN_AGL_M          0.8f    // allows GUIDED moves from ~1 m AGL (was 2.0)
#define SKYLINK_GOTO_MAX_RADIUS_M       200.0f  // tighter geofence for close field tests (was 1000)
#define SKYLINK_YAW_MAX_DEG             90

// --- Command rate limit (Phase 6 enforcement; constant defined now) ---
#define SKYLINK_CMD_RATE_LIMIT_PER_SEC  2
#define SKYLINK_CMD_DEBOUNCE_MS         500

// --- LED link patterns (Phase 1) ---
#define SKYLINK_LED_MAVLINK_BLINK_MS    500
#define SKYLINK_LED_ARMED_BLINK_MS      200

// --- Build identity (bump when flashing; see CONFIG_REFERENCE.md) ---
// FIRMWARE: increment before `pio run --target upload`
#define SKYLINK_FIRMWARE_BUILD          12
// FS: increment before `pio run --target uploadfs` (must match data/skylink_build.json + gcs_config.js)
#define SKYLINK_FS_BUILD                17

#ifdef SITL_MODE
constexpr bool SKYLINK_SIMULATION = true;
#else
constexpr bool SKYLINK_SIMULATION = false;
#endif

#endif // SKYLINK_CONFIG_H
