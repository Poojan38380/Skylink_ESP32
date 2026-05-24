#include "flight_controller.h"
#include "logger.h"

#ifdef SITL_MODE
FlightController::FlightController() {}
#else
FlightController::FlightController(HardwareSerial& serial) : fcSerial(serial) {}
#endif

void FlightController::begin() {
#ifdef SITL_MODE
    logger.info("Initializing FlightController in [SITL MODE] via TCP");
#else
    logger.info("Initializing FlightController in [HARDWARE MODE] via UART2");
    fcSerial.begin(115200, SERIAL_8N1, 16, 17); // RX2 = GPIO16, TX2 = GPIO17
#endif
    isConnectedToFC = false;
}

void FlightController::sendMavlinkPacket(mavlink_message_t* msg) {
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buf, msg);

#ifdef SITL_MODE
    if (sitlClient.connected()) {
        sitlClient.write(buf, len);
    }
#else
    fcSerial.write(buf, len);
#endif
}

void FlightController::arm(bool state) {
    logger.info(state ? "Sending command: ARM Drone" : "Sending command: DISARM Drone");
    
    mavlink_message_t msg;
    mavlink_msg_command_long_pack(
        1, 255, &msg,               // System ID 1, Component ID 255 (GCS)
        1, 1,                       // Target System 1, Target Component 1 (Autopilot)
        MAV_CMD_COMPONENT_ARM_DISARM, // Command ID
        0,                          // Confirmation
        state ? 1.0f : 0.0f,        // Param 1 (1 to arm, 0 to disarm)
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f // Param 2-7 (unused)
    );
    sendMavlinkPacket(&msg);
}

void FlightController::sendRCOverride(uint16_t roll, uint16_t pitch, uint16_t throttle, uint16_t yaw) {
    mavlink_message_t msg;
    // MAVLink channels: 1=Roll, 2=Pitch, 3=Throttle, 4=Yaw (Values: 1000 - 2000 ms)
    mavlink_msg_rc_channels_override_pack(
        1, 255, &msg,
        1, 1,
        roll, pitch, throttle, yaw,
        0, 0, 0, 0                  // Channels 5-8 (unused)
    );
    sendMavlinkPacket(&msg);
}

void FlightController::sendHeartbeat() {
    mavlink_message_t msg;
    mavlink_msg_heartbeat_pack(
        1, 255, &msg,
        MAV_TYPE_GCS,               // Identify as Ground Control Station
        MAV_AUTOPILOT_INVALID,      // No autopilot algorithm running on GCS
        MAV_MODE_FLAG_MANUAL_INPUT_ENABLED,
        0,
        MAV_STATE_ACTIVE
    );
    sendMavlinkPacket(&msg);
}

void FlightController::emergencyStop() {
    logger.warning("SAFETY TRIGGERED: EMERGENCY DISARM!");
    // Cut throttle and disarm instantly
    sendRCOverride(1500, 1500, 1000, 1500);
    arm(false);
}

FCTelemetry FlightController::getTelemetry() {
    return telemetry;
}

bool FlightController::isConnected() {
    return isConnectedToFC;
}

void FlightController::handle() {
    uint32_t now = millis();

    // 1. Maintain Connection (SITL TCP Client socket loop)
#ifdef SITL_MODE
    if (!sitlClient.connected()) {
        if (now - lastHeartbeatSent >= 5000) { // Retry connection every 5s
            lastHeartbeatSent = now;
            logger.info("Attempting connection to SITL at " + String(sitlHost) + ":" + String(sitlPort));
            if (sitlClient.connect(sitlHost, sitlPort)) {
                logger.info("Connected to ArduPilot SITL Socket!");
                isConnectedToFC = true;
            } else {
                logger.warning("SITL connection failed. Is the simulation running?");
                isConnectedToFC = false;
            }
        }
    }
#endif

    // 2. Stream Outbound Heartbeats to Autopilot (Required by ArduPilot every 1s)
    if (isConnectedToFC && (now - lastHeartbeatSent >= 1000)) {
        lastHeartbeatSent = now;
        sendHeartbeat();
    }

    // 3. Process Inbound Serial Bytes
#ifdef SITL_MODE
    while (sitlClient.connected() && sitlClient.available() > 0) {
        handleIncomingByte(sitlClient.read());
    }
#else
    while (fcSerial.available() > 0) {
        handleIncomingByte(fcSerial.read());
    }
#endif
}

void FlightController::handleIncomingByte(uint8_t byte) {
    mavlink_message_t msg;
    mavlink_status_t status;
    
    // Parse stream byte-by-byte into full packet frames
    if (mavlink_parse_char(MAVLINK_COMM_0, byte, &msg, &status)) {
        processMavlinkMessage(&msg);
    }
}

void FlightController::processMavlinkMessage(mavlink_message_t* msg) {
    isConnectedToFC = true; // Heartbeat packet frame verified

    switch (msg->msgid) {
        case MAVLINK_MSG_ID_HEARTBEAT: {
            mavlink_heartbeat_t hb;
            mavlink_msg_heartbeat_decode(msg, &hb);
            telemetry.armed = (hb.base_mode & MAV_MODE_FLAG_SAFETY_ARMED);
            break;
        }
        case MAVLINK_MSG_ID_SYS_STATUS: {
            mavlink_sys_status_t sys;
            mavlink_msg_sys_status_decode(msg, &sys);
            telemetry.battery_voltage = sys.voltage_battery / 1000.0f; // mV -> Volts
            telemetry.battery_remaining = sys.battery_remaining;       // %
            break;
        }
        case MAVLINK_MSG_ID_ATTITUDE: {
            mavlink_attitude_t att;
            mavlink_msg_attitude_decode(msg, &att);
            telemetry.roll = att.roll * 57.2958f;   // Radians -> Degrees
            telemetry.pitch = att.pitch * 57.2958f;
            telemetry.yaw = att.yaw * 57.2958f;
            break;
        }
        case MAVLINK_MSG_ID_VFR_HUD: {
            mavlink_vfr_hud_t hud;
            mavlink_msg_vfr_hud_decode(msg, &hud);
            telemetry.altitude = hud.alt;           // Meters
            telemetry.speed = hud.groundspeed;      // m/s
            break;
        }
        case MAVLINK_MSG_ID_GPS_RAW_INT: {
            mavlink_gps_raw_int_t gps;
            mavlink_msg_gps_raw_int_decode(msg, &gps);
            telemetry.latitude = gps.lat / 10000000.0;  // Scale 1e7 -> Decimal degrees
            telemetry.longitude = gps.lon / 10000000.0;
            telemetry.gps_sats = gps.satellites_visible;
            telemetry.gps_fix = gps.fix_type;           // 3 = 3D Fix
            break;
        }
    }
}

// Instantiate the global object
#ifdef SITL_MODE
FlightController flightController;
#else
FlightController flightController(Serial2);
#endif
