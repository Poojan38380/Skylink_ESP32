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

uint8_t parseFlightMode(const String& modeName) {
    if (modeName == "GUIDED") return COPTER_MODE_GUIDED;
    if (modeName == "LOITER") return COPTER_MODE_LOITER;
    if (modeName == "RTL") return COPTER_MODE_RTL;
    if (modeName == "LAND") return COPTER_MODE_LAND;
    return COPTER_MODE_STABILIZE;
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
    (void)client;
    String mode = doc["mode"] | "GUIDED";
    flightController.setFlightMode(parseFlightMode(mode));
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
            client->setCloseClientOnQueueFull(false);
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
    doc["sitl_tcp_connected"] = flightController.isSitlTcpConnected();
    doc["wifi_connected"] = wifiManager.isConnected();
    doc["wifi_rssi"] = wifiManager.getSignalStrength();
    doc["ws_connected"] = (getWsClientCount() > 0);
    doc["ws_clients"] = getWsClientCount();
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
        if (ev.type == FCEventType::StatusText) {
            continue;
        }

        JsonDocument doc;
        doc["v"] = SKYLINK_PROTOCOL_VERSION;
        doc["type"] = "event";
        doc["timestamp"] = timeSync.getCurrentTime();
        doc["event"] = "ACK";
        doc["command"] = ev.ack_command;
        doc["result"] = ev.ack_result;
        doc["result_name"] = mavResultName(ev.ack_result);
        doc["ok"] = (ev.ack_result == MAV_RESULT_ACCEPTED);

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
