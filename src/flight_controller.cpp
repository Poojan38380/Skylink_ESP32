#include "flight_controller.h"
#include "logger.h"
#include "skylink_config.h"
#include <math.h>

#ifdef SITL_MODE
FlightController::FlightController() {}
#else
FlightController::FlightController(HardwareSerial& serial) : fcSerial(serial) {}
#endif

bool FlightController::takeMutex(TickType_t timeout) {
    if (!fcMutex) return true;
    return xSemaphoreTake(fcMutex, timeout) == pdTRUE;
}

void FlightController::giveMutex() {
    if (fcMutex) xSemaphoreGive(fcMutex);
}

bool FlightController::isUsableSitlHost(const String& host) {
    if (host.length() == 0) return false;
    if (host == "127.0.0.1" || host == "0.0.0.0" || host == "::1") return false;

    IPAddress ip;
    if (!ip.fromString(host)) return false;

    // ESP32 cannot reach its own loopback; reject non-routable targets
    if (ip[0] == 127) return false;

    return true;
}

void FlightController::begin() {
    fcMutex = xSemaphoreCreateMutex();

#ifdef SITL_MODE
    logger.info("Initializing FlightController in [SITL MODE] via TCP port " + String(sitlPort));
    sitlClient.setTimeout(SKYLINK_SITL_CONNECT_TIMEOUT_MS);
#else
    logger.info("Initializing FlightController in [HARDWARE MODE] via UART2");
    fcSerial.begin(115200, SERIAL_8N1, 16, 17);
#endif

    mavlinkActive = false;
    messageIntervalsSent = false;
    lastMavlinkRx = 0;
    statusLineCount = 0;
    statusLineHead = 0;
    eventQueueHead = 0;
    eventQueueTail = 0;
}

void FlightController::sendMavlinkPacket(mavlink_message_t* msg) {
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buf, msg);

#ifdef SITL_MODE
    if (!sitlClient.connected()) return;
    sitlClient.write(buf, len);
#else
    fcSerial.write(buf, len);
#endif
}

void FlightController::setMessageInterval(uint32_t msgId, int32_t intervalUs) {
    mavlink_message_t msg;
    mavlink_msg_command_long_pack(
        1, 255, &msg,
        1, 1,
        MAV_CMD_SET_MESSAGE_INTERVAL,
        0,
        (float)msgId,
        (float)intervalUs,
        0, 0, 0, 0, 0
    );
    sendMavlinkPacket(&msg);
}

void FlightController::requestMessageIntervals() {
    setMessageInterval(MAVLINK_MSG_ID_ATTITUDE, 100000);
    setMessageInterval(MAVLINK_MSG_ID_GLOBAL_POSITION_INT, 200000);
    setMessageInterval(MAVLINK_MSG_ID_GPS_RAW_INT, 500000);
    messageIntervalsSent = true;
}

void FlightController::requestDataStreams() {
    mavlink_message_t msg;

    const uint8_t streams[] = {
        MAV_DATA_STREAM_ALL,
        MAV_DATA_STREAM_RAW_SENSORS,
        MAV_DATA_STREAM_EXTRA1,
        MAV_DATA_STREAM_EXTRA2,
        MAV_DATA_STREAM_EXTRA3,
        MAV_DATA_STREAM_POSITION,
    };

    for (uint8_t stream : streams) {
        mavlink_msg_request_data_stream_pack(
            1, 255, &msg,
            1, 1,
            stream,
            4,
            1
        );
        sendMavlinkPacket(&msg);
    }

    requestMessageIntervals();
}

void FlightController::setCopterMode(uint8_t customMode) {
    mavlink_message_t msg;
    mavlink_msg_command_long_pack(
        1, 255, &msg,
        1, 1,
        MAV_CMD_DO_SET_MODE,
        0,
        MAV_MODE_FLAG_CUSTOM_MODE_ENABLED,
        customMode,
        0, 0, 0, 0, 0
    );
    sendMavlinkPacket(&msg);
}

void FlightController::setFlightMode(uint8_t customMode) {
    if (!takeMutex()) return;
    logger.info("Setting flight mode: " + String(customMode));
    setCopterMode(customMode);
    giveMutex();
}

void FlightController::sendArmDisarm(bool state) {
    mavlink_message_t msg;
    mavlink_msg_command_long_pack(
        1, 255, &msg,
        1, 1,
        MAV_CMD_COMPONENT_ARM_DISARM,
        0,
        state ? 1.0f : 0.0f,
        0, 0, 0, 0, 0, 0
    );
    sendMavlinkPacket(&msg);
}

void FlightController::arm(bool state) {
    if (!takeMutex()) return;

    if (!mavlinkActive) {
        logger.warning("Cannot arm — no MAVLink link to autopilot");
        giveMutex();
        return;
    }

    logger.info(state ? "Sending command: ARM Drone" : "Sending command: DISARM Drone");
    sendArmDisarm(state);
    giveMutex();
}

void FlightController::takeoff(float altitudeMeters) {
    if (!takeMutex()) return;

    if (!mavlinkActive) {
        logger.warning("Cannot takeoff — no MAVLink link to autopilot");
        giveMutex();
        return;
    }

    logger.info("Sending command: TAKEOFF to " + String(altitudeMeters) + "m");

    mavlink_message_t msg;
    mavlink_msg_command_long_pack(
        1, 255, &msg,
        1, 1,
        MAV_CMD_NAV_TAKEOFF,
        0,
        0, 0, 0, 0,
        0, 0,
        altitudeMeters
    );
    sendMavlinkPacket(&msg);
    giveMutex();
}

void FlightController::land() {
    if (!takeMutex()) return;
    logger.info("Setting flight mode: LAND");
    setCopterMode(COPTER_MODE_LAND);
    giveMutex();
}

void FlightController::returnToLaunch() {
    if (!takeMutex()) return;
    logger.info("Setting flight mode: RTL");
    setCopterMode(COPTER_MODE_RTL);
    giveMutex();
}

bool FlightController::canExecuteGuidedMoveUnlocked() const {
    if (!mavlinkActive) return false;
    if (!telemetry.armed) return false;
    if (telemetry.flight_mode != COPTER_MODE_GUIDED) return false;
    if (telemetry.gps_fix < 3) return false;
    const float agl = telemetry.relative_alt > 0.0f ? telemetry.relative_alt : telemetry.altitude;
    if (agl < SKYLINK_MOVE_MIN_AGL_M) return false;
    return true;
}

static float clampMove(float v) {
    if (v > SKYLINK_MOVE_BODY_MAX_M) return SKYLINK_MOVE_BODY_MAX_M;
    if (v < -SKYLINK_MOVE_BODY_MAX_M) return -SKYLINK_MOVE_BODY_MAX_M;
    return v;
}

void FlightController::moveBody(float xMeters, float yMeters, float zMeters) {
    if (!takeMutex()) return;

    const uint32_t now = millis();
    if (now - lastGuidedCmdMs < SKYLINK_CMD_DEBOUNCE_MS) {
        giveMutex();
        return;
    }

    if (!canExecuteGuidedMoveUnlocked()) {
        logger.warning("MOVE_BODY rejected (need armed GUIDED, GPS 3D, min AGL)");
        giveMutex();
        return;
    }

    const float x = clampMove(xMeters);
    const float y = clampMove(yMeters);
    const float z = clampMove(zMeters);
    if (x == 0.0f && y == 0.0f && z == 0.0f) {
        giveMutex();
        return;
    }

    const uint16_t typeMask =
        POSITION_TARGET_TYPEMASK_VX_IGNORE |
        POSITION_TARGET_TYPEMASK_VY_IGNORE |
        POSITION_TARGET_TYPEMASK_VZ_IGNORE |
        POSITION_TARGET_TYPEMASK_AX_IGNORE |
        POSITION_TARGET_TYPEMASK_AY_IGNORE |
        POSITION_TARGET_TYPEMASK_AZ_IGNORE |
        POSITION_TARGET_TYPEMASK_YAW_IGNORE |
        POSITION_TARGET_TYPEMASK_YAW_RATE_IGNORE;

    mavlink_message_t msg;
    mavlink_msg_set_position_target_local_ned_pack(
        1, 255, &msg,
        millis(),
        1, 1,
        MAV_FRAME_BODY_OFFSET_NED,
        typeMask,
        x, y, z,
        0, 0, 0,
        0, 0, 0,
        0, 0
    );
    sendMavlinkPacket(&msg);
    lastGuidedCmdMs = now;
    giveMutex();
}

void FlightController::yawRelative(float degrees) {
    if (!takeMutex()) return;

    const uint32_t now = millis();
    if (now - lastGuidedCmdMs < SKYLINK_CMD_DEBOUNCE_MS) {
        giveMutex();
        return;
    }

    if (!canExecuteGuidedMoveUnlocked()) {
        logger.warning("YAW_RELATIVE rejected (need armed GUIDED, GPS 3D, min AGL)");
        giveMutex();
        return;
    }

    float deg = degrees;
    if (deg > SKYLINK_YAW_MAX_DEG) deg = SKYLINK_YAW_MAX_DEG;
    if (deg < -SKYLINK_YAW_MAX_DEG) deg = -SKYLINK_YAW_MAX_DEG;
    if (deg == 0.0f) {
        giveMutex();
        return;
    }

    const int direction = deg >= 0.0f ? 1 : -1;
    const float absDeg = fabsf(deg);

    mavlink_message_t msg;
    mavlink_msg_command_long_pack(
        1, 255, &msg,
        1, 1,
        MAV_CMD_CONDITION_YAW,
        0,
        absDeg,
        25.0f,
        (float)direction,
        1.0f,
        0, 0, 0
    );
    sendMavlinkPacket(&msg);
    lastGuidedCmdMs = now;
    giveMutex();
}

void FlightController::sendRCOverride(uint16_t roll, uint16_t pitch, uint16_t throttle, uint16_t yaw) {
    if (!takeMutex()) return;

    mavlink_message_t msg;
    mavlink_msg_rc_channels_override_pack(
        1, 255, &msg,
        1, 1,
        roll, pitch, throttle, yaw,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    );
    sendMavlinkPacket(&msg);
    giveMutex();
}

void FlightController::sendHeartbeat() {
    mavlink_message_t msg;
    mavlink_msg_heartbeat_pack(
        1, 255, &msg,
        MAV_TYPE_GCS,
        MAV_AUTOPILOT_INVALID,
        MAV_MODE_FLAG_MANUAL_INPUT_ENABLED,
        0,
        MAV_STATE_ACTIVE
    );
    sendMavlinkPacket(&msg);
}

void FlightController::emergencyStop() {
    if (!takeMutex()) return;
    logger.warning("SAFETY TRIGGERED: EMERGENCY DISARM!");
    sendRCOverride(1500, 1500, 1000, 1500);
    sendArmDisarm(false);
    giveMutex();
}

FCTelemetry FlightController::getTelemetry() {
    FCTelemetry copy;
    if (takeMutex(10)) {
        copy = telemetry;
        giveMutex();
    }
    return copy;
}

bool FlightController::isConnected() {
    bool connected = false;
    if (takeMutex(10)) {
        connected = mavlinkActive;
        giveMutex();
    }
    return connected;
}

bool FlightController::isSitlTcpConnected() {
#ifdef SITL_MODE
    return sitlClient.connected();
#else
    return true;
#endif
}

#ifdef SITL_MODE
bool FlightController::isSitlHostConfigured() {
    return sitlHostConfigured;
}

String FlightController::getSitlHost() {
    return sitlHost;
}

uint16_t FlightController::getSitlPort() {
    return sitlPort;
}

void FlightController::setSITLHost(const String& host) {
    if (!isUsableSitlHost(host)) {
        logger.warning("Ignoring invalid SITL host: " + host + " (use your PC's LAN IP, not 127.0.0.1)");
        return;
    }

    if (!takeMutex()) return;

    if (sitlHost != host) {
        sitlHost = host;
        sitlHostConfigured = true;
        logger.info("SITL host set to GCS machine: " + sitlHost + ":" + String(sitlPort));
        if (sitlClient.connected()) {
            sitlClient.stop();
        }
        mavlinkActive = false;
        messageIntervalsSent = false;
        lastReconnectAttempt = 0;
    }
    giveMutex();
}

void FlightController::maintainSitlConnection(uint32_t now) {
    if (!sitlHostConfigured) return;

    if (!sitlClient.connected()) {
        mavlinkActive = false;
        messageIntervalsSent = false;
        if (now - lastReconnectAttempt < SKYLINK_SITL_RECONNECT_INTERVAL_MS) return;

        lastReconnectAttempt = now;
        logger.info("Attempting connection to SITL at " + sitlHost + ":" + String(sitlPort));

        if (sitlClient.connect(sitlHost.c_str(), sitlPort)) {
            logger.info("Connected to ArduPilot SITL (TCP " + String(sitlPort) + ")");
            requestDataStreams();
            lastStreamRequest = now;
            sendHeartbeat();
            lastGcsHeartbeat = now;
        } else {
            logger.warning("SITL connection failed. Start SITL: sim_vehicle.py -v ArduCopter (no --out=tcpin:5763)");
        }
        return;
    }

    if (mavlinkActive && (now - lastMavlinkRx > SKYLINK_MAVLINK_TIMEOUT_MS)) {
        logger.warning("MAVLink link timed out — reconnecting");
        sitlClient.stop();
        mavlinkActive = false;
        messageIntervalsSent = false;
    }
}
#endif

const char* FlightController::flightModeName(uint8_t customMode) {
    switch (customMode) {
        case COPTER_MODE_STABILIZE: return "STABILIZE";
        case COPTER_MODE_ACRO: return "ACRO";
        case COPTER_MODE_ALT_HOLD: return "ALT_HOLD";
        case COPTER_MODE_AUTO: return "AUTO";
        case COPTER_MODE_GUIDED: return "GUIDED";
        case COPTER_MODE_LOITER: return "LOITER";
        case COPTER_MODE_RTL: return "RTL";
        case COPTER_MODE_CIRCLE: return "CIRCLE";
        case COPTER_MODE_LAND: return "LAND";
        default: return "UNKNOWN";
    }
}

void FlightController::pushStatusLine(const char* text, uint8_t severity) {
    if (!text) return;

    char line[SKYLINK_STATUSTEXT_MAX_LEN + 1];
    strncpy(line, text, SKYLINK_STATUSTEXT_MAX_LEN);
    line[SKYLINK_STATUSTEXT_MAX_LEN] = '\0';

    for (int i = 0; line[i]; ++i) {
        if (line[i] < 32 && line[i] != '\t') line[i] = ' ';
    }

    strncpy(statusLines[statusLineHead], line, SKYLINK_STATUSTEXT_MAX_LEN);
    statusLines[statusLineHead][SKYLINK_STATUSTEXT_MAX_LEN] = '\0';
    statusLineHead = (statusLineHead + 1) % SKYLINK_STATUSTEXT_RING_LINES;
    if (statusLineCount < SKYLINK_STATUSTEXT_RING_LINES) {
        statusLineCount++;
    }

    FCEvent ev;
    ev.type = FCEventType::StatusText;
    ev.severity = severity;
    strncpy(ev.text, line, SKYLINK_STATUSTEXT_MAX_LEN);
    ev.text[SKYLINK_STATUSTEXT_MAX_LEN] = '\0';
    pushEvent(ev);
}

void FlightController::pushEvent(const FCEvent& event) {
    int nextTail = (eventQueueTail + 1) % SKYLINK_FC_EVENT_QUEUE_SIZE;
    if (nextTail == eventQueueHead) {
        eventQueueHead = (eventQueueHead + 1) % SKYLINK_FC_EVENT_QUEUE_SIZE;
    }
    eventQueue[eventQueueTail] = event;
    eventQueueTail = nextTail;
}

bool FlightController::popEvent(FCEvent& out) {
    if (!takeMutex(5)) return false;
    if (eventQueueHead == eventQueueTail) {
        giveMutex();
        return false;
    }
    out = eventQueue[eventQueueHead];
    eventQueueHead = (eventQueueHead + 1) % SKYLINK_FC_EVENT_QUEUE_SIZE;
    giveMutex();
    return true;
}

void FlightController::appendStatusTexts(JsonArray arr) {
    if (!takeMutex(5)) return;

    if (statusLineCount == 0) {
        giveMutex();
        return;
    }

    const int start = (statusLineHead - statusLineCount + SKYLINK_STATUSTEXT_RING_LINES) % SKYLINK_STATUSTEXT_RING_LINES;
    for (int i = 0; i < statusLineCount; ++i) {
        const int idx = (start + i) % SKYLINK_STATUSTEXT_RING_LINES;
        arr.add(statusLines[idx]);
    }
    giveMutex();
}

void FlightController::handle() {
    uint32_t now = millis();

#ifdef SITL_MODE
    maintainSitlConnection(now);
    if (!sitlClient.connected()) return;
#endif

    if (!takeMutex(0)) return;

    if (mavlinkActive && (now - lastGcsHeartbeat >= SKYLINK_MAVLINK_GCS_HEARTBEAT_MS)) {
        lastGcsHeartbeat = now;
        sendHeartbeat();
    }

    if (mavlinkActive && (now - lastStreamRequest >= SKYLINK_MAVLINK_STREAM_REQUEST_MS)) {
        lastStreamRequest = now;
        requestDataStreams();
    } else if (mavlinkActive && !messageIntervalsSent) {
        requestMessageIntervals();
    }

#ifdef SITL_MODE
    while (sitlClient.connected() && sitlClient.available() > 0) {
        handleIncomingByte(sitlClient.read());
    }
#else
    while (fcSerial.available() > 0) {
        handleIncomingByte(fcSerial.read());
    }
#endif

    giveMutex();
}

void FlightController::handleIncomingByte(uint8_t byte) {
    mavlink_message_t msg;
    mavlink_status_t status;

    if (mavlink_parse_char(MAVLINK_COMM_0, byte, &msg, &status)) {
        processMavlinkMessage(&msg);
    }
}

void FlightController::processMavlinkMessage(mavlink_message_t* msg) {
    lastMavlinkRx = millis();
    mavlinkActive = true;

    switch (msg->msgid) {
        case MAVLINK_MSG_ID_HEARTBEAT: {
            mavlink_heartbeat_t hb;
            mavlink_msg_heartbeat_decode(msg, &hb);
            telemetry.armed = (hb.base_mode & MAV_MODE_FLAG_SAFETY_ARMED);
            telemetry.flight_mode = hb.custom_mode;
            break;
        }
        case MAVLINK_MSG_ID_SYS_STATUS: {
            mavlink_sys_status_t sys;
            mavlink_msg_sys_status_decode(msg, &sys);
            telemetry.battery_voltage = sys.voltage_battery / 1000.0f;
            telemetry.battery_remaining = sys.battery_remaining;
            break;
        }
        case MAVLINK_MSG_ID_ATTITUDE: {
            mavlink_attitude_t att;
            mavlink_msg_attitude_decode(msg, &att);
            telemetry.roll = att.roll * 57.2958f;
            telemetry.pitch = att.pitch * 57.2958f;
            telemetry.yaw = att.yaw * 57.2958f;
            break;
        }
        case MAVLINK_MSG_ID_VFR_HUD: {
            mavlink_vfr_hud_t hud;
            mavlink_msg_vfr_hud_decode(msg, &hud);
            telemetry.altitude = hud.alt;
            telemetry.speed = hud.groundspeed;
            break;
        }
        case MAVLINK_MSG_ID_GLOBAL_POSITION_INT: {
            mavlink_global_position_int_t pos;
            mavlink_msg_global_position_int_decode(msg, &pos);
            telemetry.latitude = pos.lat / 1e7;
            telemetry.longitude = pos.lon / 1e7;
            telemetry.relative_alt = pos.relative_alt / 1000.0f;
            telemetry.altitude = telemetry.relative_alt;
            const float vx = pos.vx / 100.0f;
            const float vy = pos.vy / 100.0f;
            telemetry.speed = sqrtf(vx * vx + vy * vy);
            break;
        }
        case MAVLINK_MSG_ID_GPS_RAW_INT: {
            mavlink_gps_raw_int_t gps;
            mavlink_msg_gps_raw_int_decode(msg, &gps);
            if (gps.fix_type >= 2) {
                telemetry.latitude = gps.lat / 1e7;
                telemetry.longitude = gps.lon / 1e7;
            }
            telemetry.gps_sats = gps.satellites_visible;
            telemetry.gps_fix = gps.fix_type;
            break;
        }
        case MAVLINK_MSG_ID_HOME_POSITION: {
            mavlink_home_position_t home;
            mavlink_msg_home_position_decode(msg, &home);
            telemetry.home_latitude = home.latitude / 1e7;
            telemetry.home_longitude = home.longitude / 1e7;
            telemetry.home_valid = true;
            break;
        }
        case MAVLINK_MSG_ID_COMMAND_ACK: {
            mavlink_command_ack_t ack;
            mavlink_msg_command_ack_decode(msg, &ack);
            FCEvent ev;
            ev.type = FCEventType::Ack;
            ev.ack_command = ack.command;
            ev.ack_result = ack.result;
            pushEvent(ev);
            break;
        }
        case MAVLINK_MSG_ID_STATUSTEXT: {
            mavlink_statustext_t st;
            mavlink_msg_statustext_decode(msg, &st);
            pushStatusLine(st.text, st.severity);
            break;
        }
        default:
            break;
    }
}

#ifdef SITL_MODE
FlightController flightController;
#else
FlightController flightController(Serial2);
#endif
