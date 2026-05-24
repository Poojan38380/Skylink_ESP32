#include "web_server.h"
#include "logger.h"
#include "wifi_manager.h"
#include "led_controller.h"
#include "time_sync.h"
#include "flight_controller.h"
#include <LittleFS.h>

WebServerModule::WebServerModule(int port) : server(port), ws("/ws") {
}

void WebServerModule::onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
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

static uint8_t parseFlightMode(const String& modeName) {
    if (modeName == "GUIDED") return COPTER_MODE_GUIDED;
    if (modeName == "RTL") return COPTER_MODE_RTL;
    if (modeName == "LAND") return COPTER_MODE_LAND;
    return COPTER_MODE_STABILIZE;
}

void WebServerModule::handleWebSocketMessage(void *arg, uint8_t *data, size_t len, AsyncWebSocketClient *client) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, data, len);

        if (error) {
            logger.error("WebSocket JSON parse error: " + String(error.c_str()));
            return;
        }

        String type = doc["type"] | "";
        if (type == "command") {
            String command = doc["command"] | "";

            if (command == "LED_SET") {
                bool value = doc["value"] | false;
                ledController.set(value);
                sendAppState();
            }
            else if (command == "LED_TOGGLE") {
                ledController.toggle();
                sendAppState();
            }
            else if (command == "SET_FLIGHT_MODE") {
                String mode = doc["mode"] | "GUIDED";
                flightController.setFlightMode(parseFlightMode(mode));
            }
            else if (command == "ARM_DRONE") {
                flightController.arm(true);
            }
            else if (command == "DISARM_DRONE") {
                flightController.arm(false);
            }
            else if (command == "TAKEOFF") {
                float alt = doc["altitude"] | 5.0f;
                flightController.takeoff(alt);
            }
            else if (command == "LAND") {
                flightController.land();
            }
            else if (command == "RTL") {
                flightController.returnToLaunch();
            }
            else if (command == "RC_OVERRIDE") {
                uint16_t r = doc["roll"] | 1500;
                uint16_t p = doc["pitch"] | 1500;
                uint16_t t = doc["throttle"] | 1000;
                uint16_t y = doc["yaw"] | 1500;
                flightController.sendRCOverride(r, p, t, y);
            }
            else if (command == "PING") {
                JsonDocument response;
                response["type"] = "event";
                response["event"] = "PONG";
                response["timestamp"] = timeSync.getCurrentTime();

                String output;
                serializeJson(response, output);
                client->text(output);
            }
            else {
                logger.warning("Unknown WebSocket command: " + command);
                JsonDocument err;
                err["type"] = "event";
                err["event"] = "ERROR";
                err["message"] = "Unknown command: " + command;
                String output;
                serializeJson(err, output);
                client->text(output);
            }
        }
    }
}

void WebServerModule::sendAppState() {
    JsonDocument doc;
    doc["type"] = "event";
    doc["event"] = "LED_STATE";
    doc["value"] = ledController.getState();
    doc["timestamp"] = timeSync.getCurrentTime();

    String output;
    serializeJson(doc, output);
    ws.textAll(output);
}

void WebServerModule::sendHeartbeat() {
    JsonDocument doc;
    FCTelemetry fc = flightController.getTelemetry();

    doc["type"] = "event";
    doc["event"] = "HEARTBEAT";
    doc["uptime"] = millis() / 1000;
    doc["timestamp"] = timeSync.getCurrentTime();

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
    doc["flight_mode"] = fc.flight_mode;
    doc["sitl_connected"] = flightController.isConnected();
#ifdef SITL_MODE
    doc["sitl_host"] = flightController.getSitlHost();
    doc["sitl_port"] = flightController.getSitlPort();
    doc["sitl_host_ready"] = flightController.isSitlHostConfigured();
#endif

    String output;
    serializeJson(doc, output);
    ws.textAll(output);
}

void WebServerModule::begin() {
    logger.info("Starting Async Web Server and WebSocket on /ws");

    ws.onEvent([this](AsyncWebSocket *s, AsyncWebSocketClient *c, AwsEventType t, void *a, uint8_t *d, size_t l) {
        this->onWsEvent(s, c, t, a, d, l);
    });
    server.addHandler(&ws);

    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    server.on("/health", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    server.begin();
    logger.info("Web server started!");
}

WebServerModule webServerModule(80);
