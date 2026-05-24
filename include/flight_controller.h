#ifndef FLIGHT_CONTROLLER_H
#define FLIGHT_CONTROLLER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <mavlink.h>
#include "skylink_config.h"

#ifdef SITL_MODE
#include <WiFiClient.h>
#endif

// ArduCopter custom_mode values (SERIAL_CONTROL / SET_MODE)
enum CopterMode : uint8_t {
    COPTER_MODE_STABILIZE = 0,
    COPTER_MODE_GUIDED    = 4,
    COPTER_MODE_RTL       = 6,
    COPTER_MODE_LAND      = 9,
};

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
    uint8_t flight_mode = 0;
};

class FlightController {
private:
    FCTelemetry telemetry;
    uint32_t lastGcsHeartbeat = 0;
    uint32_t lastReconnectAttempt = 0;
    uint32_t lastStreamRequest = 0;
    uint32_t lastMavlinkRx = 0;
    bool mavlinkActive = false;

    SemaphoreHandle_t fcMutex = nullptr;

#ifdef SITL_MODE
    WiFiClient sitlClient;
    String sitlHost;
    bool sitlHostConfigured = false;
    static const uint16_t sitlPort = SKYLINK_SITL_TCP_PORT;
#else
    HardwareSerial& fcSerial;
#endif

    void sendMavlinkPacket(mavlink_message_t* msg);
    void handleIncomingByte(uint8_t byte);
    void processMavlinkMessage(mavlink_message_t* msg);
    void requestDataStreams();
    void setCopterMode(uint8_t customMode);
    void sendArmDisarm(bool state);
    bool takeMutex(TickType_t timeout = portMAX_DELAY);
    void giveMutex();
    static bool isUsableSitlHost(const String& host);

#ifdef SITL_MODE
    void maintainSitlConnection(uint32_t now);
#endif

public:
#ifdef SITL_MODE
    FlightController();
#else
    FlightController(HardwareSerial& serial);
#endif

    void begin();
    void handle();

    void arm(bool state);
    void setFlightMode(uint8_t customMode);
    void takeoff(float altitudeMeters);
    void land();
    void returnToLaunch();
    void sendRCOverride(uint16_t roll, uint16_t pitch, uint16_t throttle, uint16_t yaw);
    void sendHeartbeat();
    void emergencyStop();

    FCTelemetry getTelemetry();
    bool isConnected();
    bool isSitlTcpConnected();
#ifdef SITL_MODE
    bool isSitlHostConfigured();
    String getSitlHost();
    uint16_t getSitlPort();
    void setSITLHost(const String& host);
#endif
};

extern FlightController flightController;

#endif // FLIGHT_CONTROLLER_H
