#include "flight_controller.h"
#include "logger.h"
#include "skylink_config.h"

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
    lastMavlinkRx = 0;
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
            4,  // 4 Hz
            1   // start streaming
        );
        sendMavlinkPacket(&msg);
    }
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
        lastReconnectAttempt = 0;
    }
    giveMutex();
}

void FlightController::maintainSitlConnection(uint32_t now) {
    if (!sitlHostConfigured) return;

    if (!sitlClient.connected()) {
        mavlinkActive = false;
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
    }
}
#endif

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
        case MAVLINK_MSG_ID_GPS_RAW_INT: {
            mavlink_gps_raw_int_t gps;
            mavlink_msg_gps_raw_int_decode(msg, &gps);
            telemetry.latitude = gps.lat / 10000000.0;
            telemetry.longitude = gps.lon / 10000000.0;
            telemetry.gps_sats = gps.satellites_visible;
            telemetry.gps_fix = gps.fix_type;
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
