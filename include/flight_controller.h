#ifndef FLIGHT_CONTROLLER_H
#define FLIGHT_CONTROLLER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <mavlink.h>
#include <ArduinoJson.h>
#include "skylink_config.h"

#ifdef SITL_MODE
#include <WiFi.h>
#include <WiFiClient.h>
#endif

enum CopterMode : uint8_t {
    COPTER_MODE_STABILIZE = 0,
    COPTER_MODE_ACRO     = 1,
    COPTER_MODE_ALT_HOLD = 2,
    COPTER_MODE_AUTO     = 3,
    COPTER_MODE_GUIDED   = 4,
    COPTER_MODE_LOITER   = 5,
    COPTER_MODE_RTL      = 6,
    COPTER_MODE_CIRCLE   = 7,
    COPTER_MODE_LAND     = 9,
};

struct FCTelemetry {
    bool armed = false;
    float roll = 0.0f;
    float pitch = 0.0f;
    float yaw = 0.0f;
    float altitude = 0.0f;
    float relative_alt = 0.0f;
    float speed = 0.0f;
    float battery_voltage = 0.0f;
    int battery_remaining = 0;
    double latitude = 0.0;
    double longitude = 0.0;
    int gps_sats = 0;
    int gps_fix = 0;
    uint8_t flight_mode = 0;
    bool home_valid = false;
    double home_latitude = 0.0;
    double home_longitude = 0.0;
    bool autopilot_heartbeat_fresh = false;
    uint32_t autopilot_heartbeat_age_ms = 0;
    bool command_pending = false;
    uint8_t command_status = 0;
    uint8_t command_result = 0;
    uint16_t command_mav_id = 0;
    uint32_t command_age_ms = 0;
    char command_name[24] = {};
};

enum class FCCommandStatus : uint8_t {
    Idle = 0,
    Pending,
    Accepted,
    Rejected,
    Timeout
};

struct FCPendingCommand {
    bool active = false;
    FCCommandStatus status = FCCommandStatus::Idle;
    uint16_t mav_command = 0;
    uint8_t result = MAV_RESULT_ACCEPTED;
    uint32_t started_ms = 0;
    uint32_t timeout_ms = SKYLINK_MAVLINK_COMMAND_ACK_TIMEOUT_MS;
    char name[24] = {};
};

enum class FCEventType : uint8_t {
    Ack = 0,
    StatusText
};

struct FCEvent {
    FCEventType type = FCEventType::Ack;
    uint16_t ack_command = 0;
    uint8_t ack_result = 0;
    uint8_t severity = 0;
    char text[SKYLINK_STATUSTEXT_MAX_LEN + 1] = {};
};

class FlightController {
private:
    FCTelemetry telemetry;
    uint32_t lastGcsHeartbeat = 0;
    uint32_t lastReconnectAttempt = 0;
    uint32_t lastStreamRequest = 0;
    uint32_t lastMavlinkRx = 0;
    uint32_t lastAutopilotHeartbeatRx = 0;
    uint32_t lastGuidedCmdMs = 0;
    bool mavlinkActive = false;
    bool messageIntervalsSent = false;
    FCPendingCommand pendingCommand;

    char statusLines[SKYLINK_STATUSTEXT_RING_LINES][SKYLINK_STATUSTEXT_MAX_LEN + 1];
    int statusLineCount = 0;
    int statusLineHead = 0;

    FCEvent eventQueue[SKYLINK_FC_EVENT_QUEUE_SIZE];
    int eventQueueHead = 0;
    int eventQueueTail = 0;

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
    void sendRCOverrideUnlocked(uint16_t roll, uint16_t pitch, uint16_t throttle, uint16_t yaw);
    void handleIncomingByte(uint8_t byte);
    void processMavlinkMessage(mavlink_message_t* msg);
    void requestDataStreams();
    void setCopterMode(uint8_t customMode);
    void sendArmDisarm(bool state, bool force = false);
    bool canExecuteGuidedMoveUnlocked() const;
    void pushStatusLine(const char* text, uint8_t severity);
    void pushEvent(const FCEvent& event);
    void rejectCommandUnlocked(const char* text);
    void startPendingCommandUnlocked(const char* name, uint16_t mavCommand, uint32_t timeoutMs = SKYLINK_MAVLINK_COMMAND_ACK_TIMEOUT_MS);
    void completePendingCommandUnlocked(uint16_t mavCommand, uint8_t result);
    void updatePendingCommandTimeoutUnlocked(uint32_t now);
    void failPendingCommandUnlocked(const char* reason);
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
    void moveBody(float xMeters, float yMeters, float zMeters);
    void yawRelative(float degrees);
    void gotoLatLon(double lat, double lon, float altRelMeters);
    void gotoAlt(float altRelMeters);
    void loiterHere();
    void sendRCOverride(uint16_t roll, uint16_t pitch, uint16_t throttle, uint16_t yaw);
    void sendHeartbeat();
    void emergencyStop();

    FCTelemetry getTelemetry();
    bool isConnected(TickType_t timeout = 10);
    bool isAutopilotHeartbeatFresh(TickType_t timeout = 10);
    bool isSitlTcpConnected();
    bool popEvent(FCEvent& out);
    void appendStatusTexts(JsonArray arr);
    static const char* flightModeName(uint8_t customMode);

#ifdef SITL_MODE
    bool isSitlHostConfigured();
    String getSitlHost();
    uint16_t getSitlPort();
    void setSITLHost(const String& host);
#endif
};

extern FlightController flightController;

#endif // FLIGHT_CONTROLLER_H
