#ifndef FLIGHT_CONTROLLER_H
#define FLIGHT_CONTROLLER_H

#include <Arduino.h>
#include <mavlink.h>

#ifdef SITL_MODE
#include <WiFiClient.h>
#endif

// Decoded Flight Controller telemetry state
struct FCTelemetry {
    bool armed = false;
    float roll = 0.0;
    float pitch = 0.0;
    float yaw = 0.0;
    float altitude = 0.0;
    float speed = 0.0;
    float battery_voltage = 0.0;
    int battery_remaining = 0;
    double latitude = 0.0;
    double longitude = 0.0;
    int gps_sats = 0;
    int gps_fix = 0;
};

class FlightController {
private:
    FCTelemetry telemetry;
    uint32_t lastHeartbeatSent = 0;
    uint32_t lastRCCommand = 0;
    bool isConnectedToFC = false;

#ifdef SITL_MODE
    WiFiClient sitlClient;
    const char* sitlHost = "127.0.0.1"; // Thanks to Mirrored Mode, WSL2 is mapped to localhost!
    const uint16_t sitlPort = 5760;     // SITL default TCP port
#else
    HardwareSerial& fcSerial;
#endif

    // Internal MAVLink handling helper functions
    void sendMavlinkPacket(mavlink_message_t* msg);
    void handleIncomingByte(uint8_t byte);
    void processMavlinkMessage(mavlink_message_t* msg);

public:
#ifdef SITL_MODE
    FlightController();
#else
    FlightController(HardwareSerial& serial);
#endif

    void begin();
    void handle();

    // Outbound Actuator Controls
    void arm(bool state);
    void sendRCOverride(uint16_t roll, uint16_t pitch, uint16_t throttle, uint16_t yaw);
    void sendHeartbeat();
    void emergencyStop();

    // Inbound Telemetry Getters
    FCTelemetry getTelemetry();
    bool isConnected();
};

extern FlightController flightController;

#endif // FLIGHT_CONTROLLER_H
