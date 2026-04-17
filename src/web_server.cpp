#include "web_server.h"
#include "logger.h"
#include "wifi_manager.h"
#include "led_controller.h"
#include "time_sync.h"
#include <LittleFS.h>

WebServerModule::WebServerModule(int port) : server(port), ws("/ws") {
}

void WebServerModule::onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            logger.info("WebSocket client #" + String(client->id()) + " connected from " + client->remoteIP().toString());
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
    doc["type"] = "event";
    doc["event"] = "HEARTBEAT";
    doc["uptime"] = millis() / 1000;
    doc["timestamp"] = timeSync.getCurrentTime();
    
    // Simulate UAV telemetry
    float baseAlt = 12.0;
    float noise = (float)(random(-50, 50)) / 100.0;
    doc["altitude"] = baseAlt + noise;
    doc["battery"] = 87; // Constant for now
    doc["speed"] = 0.0;
    doc["lat"] = 19.0760;
    doc["lng"] = 72.8777;
    
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

    // Serve the test console / dashboard from LittleFS
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    server.on("/health", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    server.begin();
    logger.info("Web server started!");
}

WebServerModule webServerModule(80);
