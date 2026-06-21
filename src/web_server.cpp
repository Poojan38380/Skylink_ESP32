#include "web_server.h"
#include "logger.h"
#include "wifi_manager.h"
#include "led_controller.h"
#include "time_sync.h"
#include "flight_controller.h"
#include "skylink_config.h"
#include "build_info.h"
#include "ws_json.h"
#include <LittleFS.h>

namespace {

bool parseFlightMode(const String& modeName, uint8_t& mode) {
    if (modeName == "GUIDED") {
        mode = COPTER_MODE_GUIDED;
        return true;
    }
    if (modeName == "LOITER") {
        mode = COPTER_MODE_LOITER;
        return true;
    }
    if (modeName == "RTL") {
        mode = COPTER_MODE_RTL;
        return true;
    }
    if (modeName == "LAND") {
        mode = COPTER_MODE_LAND;
        return true;
    }
    return false;
}

void sendClientError(AsyncWebSocketClient* client, const String& message) {
    JsonDocument err;
    err["v"] = SKYLINK_PROTOCOL_VERSION;
    err["type"] = "event";
    err["event"] = "ERROR";
    err["message"] = message;
    wsSendJson(client, err);
}

using WsCommandHandler = void (*)(JsonDocument&, AsyncWebSocketClient*);

void handleLedSet(JsonDocument& doc, AsyncWebSocketClient* client) {
    (void)client;
    ledController.setManual(doc["value"] | false);
    webServerModule.sendAppState();
}

void handleLedToggle(JsonDocument& doc, AsyncWebSocketClient* client) {
    (void)doc;
    (void)client;
    ledController.toggle();
    webServerModule.sendAppState();
}

void handleSetFlightMode(JsonDocument& doc, AsyncWebSocketClient* client) {
    String mode = doc["mode"] | "GUIDED";
    uint8_t customMode = 0;
    if (!parseFlightMode(mode, customMode)) {
        logger.warning("SET_FLIGHT_MODE rejected: unknown mode " + mode);
        sendClientError(client, "SET_FLIGHT_MODE rejected: unknown mode " + mode);
        return;
    }
    flightController.setFlightMode(customMode);
}

void handleArm(JsonDocument& doc, AsyncWebSocketClient* client) {
    (void)doc;
    (void)client;
    flightController.arm(true);
}

void handleDisarm(JsonDocument& doc, AsyncWebSocketClient* client) {
    (void)doc;
    (void)client;
    flightController.arm(false);
}

void handleEmergencyStop(JsonDocument& doc, AsyncWebSocketClient* client) {
    (void)doc;
    (void)client;
    flightController.emergencyStop();
}

void handleTakeoff(JsonDocument& doc, AsyncWebSocketClient* client) {
    (void)client;
    float alt = doc["altitude"] | 5.0f;
    flightController.takeoff(alt);
}

void handleLand(JsonDocument& doc, AsyncWebSocketClient* client) {
    (void)doc;
    (void)client;
    flightController.land();
}

void handleRtl(JsonDocument& doc, AsyncWebSocketClient* client) {
    (void)doc;
    (void)client;
    flightController.returnToLaunch();
}

void handleRcOverride(JsonDocument& doc, AsyncWebSocketClient* client) {
    (void)client;
    uint16_t r = doc["roll"] | 1500;
    uint16_t p = doc["pitch"] | 1500;
    uint16_t t = doc["throttle"] | 1000;
    uint16_t y = doc["yaw"] | 1500;
    flightController.sendRCOverride(r, p, t, y);
}

void handleMoveBody(JsonDocument& doc, AsyncWebSocketClient* client) {
    (void)client;
    flightController.moveBody(
        doc["x"] | 0.0f,
        doc["y"] | 0.0f,
        doc["z"] | 0.0f
    );
}

void handleYawRelative(JsonDocument& doc, AsyncWebSocketClient* client) {
    (void)client;
    flightController.yawRelative(doc["deg"] | 0.0f);
}

void handleGotoLatLon(JsonDocument& doc, AsyncWebSocketClient* client) {
    (void)client;
    flightController.gotoLatLon(
        doc["lat"] | 0.0,
        doc["lon"] | 0.0,
        doc["alt"] | 5.0f
    );
}

void handleGotoAlt(JsonDocument& doc, AsyncWebSocketClient* client) {
    (void)client;
    flightController.gotoAlt(doc["alt"] | 5.0f);
}

void handleLoiterHere(JsonDocument& doc, AsyncWebSocketClient* client) {
    (void)doc;
    (void)client;
    flightController.loiterHere();
}

void handlePing(JsonDocument& doc, AsyncWebSocketClient* client) {
    (void)doc;
    JsonDocument response;
    response["v"] = SKYLINK_PROTOCOL_VERSION;
    response["type"] = "event";
    response["event"] = "PONG";
    response["timestamp"] = timeSync.getCurrentTime();
    wsSendJson(client, response);
}

struct WsCommandEntry {
    const char* name;
    WsCommandHandler handler;
};

const WsCommandEntry kCommands[] = {
    {"LED_SET", handleLedSet},
    {"LED_TOGGLE", handleLedToggle},
    {"SET_FLIGHT_MODE", handleSetFlightMode},
    {"ARM_DRONE", handleArm},
    {"DISARM_DRONE", handleDisarm},
    {"EMERGENCY_STOP", handleEmergencyStop},
    {"TAKEOFF", handleTakeoff},
    {"LAND", handleLand},
    {"RTL", handleRtl},
    {"MOVE_BODY", handleMoveBody},
    {"YAW_RELATIVE", handleYawRelative},
    {"GOTO_LATLON", handleGotoLatLon},
    {"GOTO_ALT", handleGotoAlt},
    {"LOITER_HERE", handleLoiterHere},
    {"RC_OVERRIDE", handleRcOverride},
    {"PING", handlePing},
};

bool isFlightCommand(const String& command) {
    return command == "SET_FLIGHT_MODE" ||
           command == "ARM_DRONE" ||
           command == "DISARM_DRONE" ||
           command == "TAKEOFF" ||
           command == "LAND" ||
           command == "RTL" ||
           command == "MOVE_BODY" ||
           command == "YAW_RELATIVE" ||
           command == "GOTO_LATLON" ||
           command == "GOTO_ALT" ||
           command == "LOITER_HERE" ||
           command == "RC_OVERRIDE";
}

enum class SafetyState : uint8_t {
    Disconnected = 0,
    Settling,
    Preflight,
    ReadyToArm,
    ArmedGround,
    Flying,
    Landing,
    FailsafeOrStale
};

struct SafetySnapshot {
    SafetyState state = SafetyState::Disconnected;
    const char* name = "DISCONNECTED";
    String reason = "No active link";
    bool gateReady = false;
    bool guided = false;
    bool gpsOk = false;
    bool moveAglOk = false;
};

const char* safetyStateName(SafetyState state) {
    switch (state) {
        case SafetyState::Disconnected: return "DISCONNECTED";
        case SafetyState::Settling: return "SETTLING";
        case SafetyState::Preflight: return "PREFLIGHT";
        case SafetyState::ReadyToArm: return "READY_TO_ARM";
        case SafetyState::ArmedGround: return "ARMED_GROUND";
        case SafetyState::Flying: return "FLYING";
        case SafetyState::Landing: return "LANDING";
        case SafetyState::FailsafeOrStale: return "FAILSAFE_OR_STALE";
        default: return "UNKNOWN";
    }
}

const char* commandStatusName(uint8_t status) {
    switch ((FCCommandStatus)status) {
        case FCCommandStatus::Idle: return "IDLE";
        case FCCommandStatus::Pending: return "PENDING";
        case FCCommandStatus::Accepted: return "ACCEPTED";
        case FCCommandStatus::Rejected: return "REJECTED";
        case FCCommandStatus::Timeout: return "TIMEOUT";
        default: return "UNKNOWN";
    }
}

bool dispatchCommand(const String& command, JsonDocument& doc, AsyncWebSocketClient* client) {
    for (const auto& entry : kCommands) {
        if (command == entry.name) {
            entry.handler(doc, client);
            return true;
        }
    }
    return false;
}

}  // namespace

WebServerModule::WebServerModule(int port) : server(port), ws("/ws") {
}

void WebServerModule::onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            ws.cleanupClients(1); // Evict stale connections — enforce max 1 active client
            client->setCloseClientOnQueueFull(false);
            lastWsConnectMs = millis();
            lastFlightCommandMs = 0;
            lastSameCommandMs = 0;
            lastFlightCommand = "";
            logger.info("WebSocket client #" + String(client->id()) + " connected from " + client->remoteIP().toString());
#ifdef SITL_MODE
            flightController.setSITLHost(client->remoteIP().toString());
#endif
            sendAppState();
            break;
        case WS_EVT_DISCONNECT:
            logger.info("WebSocket client #" + String(client->id()) + " disconnected");
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(arg, data, len, client);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void WebServerModule::handleWebSocketMessage(void *arg, uint8_t *data, size_t len, AsyncWebSocketClient *client) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (!(info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)) {
        return;
    }

    JsonDocument doc;
    if (deserializeJson(doc, data, len)) {
        logger.error("WebSocket JSON parse error");
        return;
    }

    const String type = doc["type"] | "";
    if (type != "command") {
        return;
    }

    const String command = doc["command"] | "";
    if (!validateCommand(command, client)) {
        return;
    }

    if (dispatchCommand(command, doc, client)) {
        return;
    }

    logger.warning("Unknown WebSocket command: " + command);
    JsonDocument err;
    err["v"] = SKYLINK_PROTOCOL_VERSION;
    err["type"] = "event";
    err["event"] = "ERROR";
    err["message"] = "Unknown command: " + command;
    wsSendJson(client, err);
}

void WebServerModule::rejectCommand(AsyncWebSocketClient* client, const String& message) {
    logger.warning(message);
    sendClientError(client, message);
}

bool WebServerModule::validateCommand(const String& command, AsyncWebSocketClient* client) {
    if (command == "EMERGENCY_STOP" || command == "PING" ||
        command == "LED_SET" || command == "LED_TOGGLE") {
        return true;
    }

    if (!isFlightCommand(command)) {
        return true;
    }

    const uint32_t now = millis();

    if (!client || client->status() != WS_CONNECTED) {
        rejectCommand(client, command + " rejected: WebSocket client is not active");
        return false;
    }

    String gateReason;
    if (!isFlightCommandGateReady(gateReason)) {
        rejectCommand(client, command + " rejected: " + gateReason);
        return false;
    }

    const FCTelemetry fc = flightController.getTelemetry();
    if (fc.command_pending) {
        rejectCommand(client, command + " rejected: waiting for " + String(fc.command_name[0] ? fc.command_name : "pending command"));
        return false;
    }

    if (command == lastFlightCommand && now - lastSameCommandMs < SKYLINK_WS_FLIGHT_CMD_DEDUPE_MS) {
        rejectCommand(client, command + " rejected: duplicate command too soon");
        return false;
    }

    if (now - lastFlightCommandMs < SKYLINK_WS_FLIGHT_CMD_MIN_INTERVAL_MS) {
        rejectCommand(client, command + " rejected: flight command rate limit");
        return false;
    }

    lastFlightCommandMs = now;
    lastSameCommandMs = now;
    lastFlightCommand = command;
    return true;
}

bool WebServerModule::isFlightCommandGateReady(String& reason) const {
    const uint32_t now = millis();

    if (getWsClientCount() == 0) {
        reason = "no active WebSocket client";
        return false;
    }

    if (now - lastWsConnectMs < SKYLINK_WS_RECONNECT_SETTLE_MS) {
        reason = "waiting for fresh state after WebSocket reconnect";
        return false;
    }

    if (!flightController.isConnected(50)) {
        reason = "no active MAVLink link";
        return false;
    }

    if (!flightController.isAutopilotHeartbeatFresh(50)) {
        reason = "autopilot heartbeat stale";
        return false;
    }

    reason = "ready";
    return true;
}

void WebServerModule::appendSafetyState(JsonDocument& doc, const FCTelemetry& fc, bool cmdGateReady, const String& cmdGateReason) {
    SafetySnapshot safety;
    safety.gateReady = cmdGateReady;
    safety.guided = (fc.flight_mode == COPTER_MODE_GUIDED);
    safety.gpsOk = (fc.gps_fix >= 3);
    safety.moveAglOk = (fc.relative_alt >= SKYLINK_MOVE_MIN_AGL_M);

    if (!cmdGateReady) {
        if (cmdGateReason.indexOf("reconnect") >= 0) {
            safety.state = SafetyState::Settling;
        } else if (cmdGateReason.indexOf("MAVLink") >= 0 || cmdGateReason.indexOf("WebSocket") >= 0) {
            safety.state = SafetyState::Disconnected;
        } else {
            safety.state = SafetyState::FailsafeOrStale;
        }
        safety.reason = cmdGateReason;
    } else if (fc.armed && fc.flight_mode == COPTER_MODE_LAND) {
        safety.state = SafetyState::Landing;
        safety.reason = "Landing";
    } else if (fc.armed && safety.moveAglOk) {
        safety.state = SafetyState::Flying;
        safety.reason = safety.guided ? "Flying in GUIDED" : "Flying outside GUIDED";
    } else if (fc.armed) {
        safety.state = SafetyState::ArmedGround;
        safety.reason = safety.guided ? "Armed on ground in GUIDED" : "Armed on ground outside GUIDED";
    } else if (!safety.gpsOk) {
        safety.state = SafetyState::Preflight;
        safety.reason = "Waiting for GPS 3D fix";
    } else {
        safety.state = SafetyState::ReadyToArm;
        safety.reason = safety.guided ? "Ready to arm" : "Ready; GUIDED required before arm/takeoff";
    }

    safety.name = safetyStateName(safety.state);

    const bool armed = fc.armed;
    const bool flying = (safety.state == SafetyState::Flying);
    const bool armedGround = (safety.state == SafetyState::ArmedGround);
    const bool readyToArm = (safety.state == SafetyState::ReadyToArm);
    const bool landing = (safety.state == SafetyState::Landing);

    doc["safety_state"] = (uint8_t)safety.state;
    doc["safety_state_name"] = safety.name;
    doc["safety_reason"] = safety.reason;
    doc["cmd_gate_ready"] = cmdGateReady;
    doc["cmd_gate_reason"] = cmdGateReason;
    doc["can_set_mode"] = cmdGateReady;
    doc["can_arm"] = (safety.state == SafetyState::ReadyToArm);
    doc["can_disarm"] = cmdGateReady && armed;
    const bool canAutoTakeoffFromDisarmed = readyToArm && !armed;
    const bool canTakeoffFromArmedGround = armedGround && safety.guided;
    doc["can_takeoff"] = cmdGateReady && safety.gpsOk &&
        (canAutoTakeoffFromDisarmed || canTakeoffFromArmedGround);
    doc["can_land"] = cmdGateReady && armed;
    doc["can_rtl"] = cmdGateReady && armed && !landing;
    doc["can_loiter"] = flying && safety.gpsOk;
    doc["can_move"] = flying && safety.guided && safety.gpsOk;
    doc["can_goto"] = flying && safety.guided && safety.gpsOk && fc.home_valid;
    doc["can_emergency_stop"] = getWsClientCount() > 0 && armed;
}

void WebServerModule::sendAppState() {
    JsonDocument doc;
    doc["v"] = SKYLINK_PROTOCOL_VERSION;
    doc["type"] = "event";
    doc["event"] = "LED_STATE";
    doc["value"] = ledController.getState();
    doc["timestamp"] = timeSync.getCurrentTime();
    wsBroadcastJson(ws, doc);
}

int WebServerModule::getWsClientCount() const {
    return ws.count();
}

void WebServerModule::sendHeartbeat() {
    if (getWsClientCount() == 0) return;

    JsonDocument doc;
    const FCTelemetry fc = flightController.getTelemetry();
    String cmdGateReason;
    const bool cmdGateReady = isFlightCommandGateReady(cmdGateReason);

    doc["v"] = SKYLINK_PROTOCOL_VERSION;
    doc["type"] = "event";
    doc["event"] = "HEARTBEAT";
    doc["uptime"] = millis() / 1000;
    doc["timestamp"] = timeSync.getCurrentTime();
    doc["simulation"] = SKYLINK_SIMULATION;
    doc["fw_build"] = getFirmwareBuild();
    doc["fs_build"] = getFsBuildOnFlash();
    doc["fs_build_expected"] = getFsBuildExpected();
    doc["fs_build_ok"] = isFsBuildMatch();

    doc["armed"] = fc.armed;
    doc["altitude"] = fc.altitude;
    doc["battery"] = fc.battery_remaining;
    doc["battery_v"] = fc.battery_voltage;
    doc["speed"] = fc.speed;
    doc["lat"] = fc.latitude;
    doc["lng"] = fc.longitude;
    doc["sats"] = fc.gps_sats;
    doc["gps_fix"] = fc.gps_fix;
    doc["roll"] = fc.roll;
    doc["pitch"] = fc.pitch;
    doc["yaw"] = fc.yaw;
    doc["flight_mode"] = fc.flight_mode;
    doc["flight_mode_name"] = FlightController::flightModeName(fc.flight_mode);
    doc["relative_alt"] = fc.relative_alt;
    doc["home_valid"] = fc.home_valid;
    if (fc.home_valid) {
        doc["home_lat"] = fc.home_latitude;
        doc["home_lng"] = fc.home_longitude;
    }

    JsonArray statusArr = doc["statustext"].to<JsonArray>();
    flightController.appendStatusTexts(statusArr);

    doc["sitl_connected"] = flightController.isConnected();
    doc["mav_connected"] = flightController.isConnected();
    doc["autopilot_heartbeat_fresh"] = fc.autopilot_heartbeat_fresh;
    doc["autopilot_heartbeat_age_ms"] = fc.autopilot_heartbeat_age_ms;
    doc["command_pending"] = fc.command_pending;
    doc["command_name"] = fc.command_name;
    doc["command_status"] = fc.command_status;
    doc["command_status_name"] = commandStatusName(fc.command_status);
    doc["command_result"] = fc.command_result;
    doc["command_mav_id"] = fc.command_mav_id;
    doc["command_age_ms"] = fc.command_age_ms;
    doc["sitl_tcp_connected"] = flightController.isSitlTcpConnected();
    doc["wifi_connected"] = wifiManager.isConnected();
    doc["wifi_rssi"] = wifiManager.getSignalStrength();
    doc["ws_connected"] = (getWsClientCount() > 0);
    doc["ws_clients"] = getWsClientCount();
    appendSafetyState(doc, fc, cmdGateReady, cmdGateReason);
#ifdef SITL_MODE
    doc["sitl_host"] = flightController.getSitlHost();
    doc["sitl_port"] = flightController.getSitlPort();
    doc["sitl_host_ready"] = flightController.isSitlHostConfigured();
#endif

    wsBroadcastJson(ws, doc);
}

namespace {

const char* mavResultName(uint8_t result) {
    switch (result) {
        case MAV_RESULT_ACCEPTED: return "ACCEPTED";
        case MAV_RESULT_TEMPORARILY_REJECTED: return "TEMP_REJECTED";
        case MAV_RESULT_DENIED: return "DENIED";
        case MAV_RESULT_UNSUPPORTED: return "UNSUPPORTED";
        case MAV_RESULT_FAILED: return "FAILED";
        default: return "UNKNOWN";
    }
}

}  // namespace

void WebServerModule::sendPendingFcEvents() {
    if (getWsClientCount() == 0) return;

    FCEvent ev;
    int sent = 0;
    while (sent < SKYLINK_WS_MAX_EVENTS_PER_LOOP && flightController.popEvent(ev)) {
        JsonDocument doc;
        doc["v"] = SKYLINK_PROTOCOL_VERSION;
        doc["type"] = "event";
        doc["timestamp"] = timeSync.getCurrentTime();
        if (ev.type == FCEventType::StatusText) {
            doc["event"] = "STATUSTEXT";
            doc["severity"] = ev.severity;
            doc["text"] = ev.text;
        } else {
            doc["event"] = "ACK";
            doc["command"] = ev.ack_command;
            doc["result"] = ev.ack_result;
            doc["result_name"] = mavResultName(ev.ack_result);
            doc["ok"] = (ev.ack_result == MAV_RESULT_ACCEPTED);
        }

        if (wsBroadcastJson(ws, doc)) {
            sent++;
        } else {
            break;
        }
    }
}

void WebServerModule::begin() {
    logger.info("Starting Async Web Server and WebSocket on /ws");

    ws.onEvent([this](AsyncWebSocket *s, AsyncWebSocketClient *c, AwsEventType t, void *a, uint8_t *d, size_t l) {
        this->onWsEvent(s, c, t, a, d, l);
    });
    server.addHandler(&ws);

    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    server.on("/health", HTTP_GET, [](AsyncWebServerRequest *request) {
        char buf[192];
        snprintf(buf, sizeof(buf),
            "{\"status\":\"ok\",\"v\":%d,\"fw\":%u,\"fs\":%u,\"fs_expected\":%u,\"fs_ok\":%s,\"simulation\":%s}",
            SKYLINK_PROTOCOL_VERSION,
            (unsigned)getFirmwareBuild(),
            (unsigned)getFsBuildOnFlash(),
            (unsigned)getFsBuildExpected(),
            isFsBuildMatch() ? "true" : "false",
            SKYLINK_SIMULATION ? "true" : "false");
        request->send(200, "application/json", buf);
    });

    server.begin();
    logger.info("Web server started!");
}

WebServerModule webServerModule(80);
