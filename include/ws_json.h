#ifndef WS_JSON_H
#define WS_JSON_H

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include "skylink_config.h"

// Serialize JSON to WebSocket without heap String allocation
inline bool wsSendJson(AsyncWebSocketClient* client, JsonDocument& doc) {
    if (!client) return false;
    char buf[SKYLINK_JSON_BUFFER_SIZE];
    size_t len = serializeJson(doc, buf, sizeof(buf));
    if (len >= sizeof(buf)) return false;
    client->text(buf, len);
    return true;
}

inline bool wsBroadcastJson(AsyncWebSocket& ws, JsonDocument& doc) {
    if (ws.count() == 0 || !ws.availableForWriteAll()) return false;
    char buf[SKYLINK_JSON_BUFFER_SIZE];
    size_t len = serializeJson(doc, buf, sizeof(buf));
    if (len >= sizeof(buf)) return false;
    ws.textAll(buf, len);
    return true;
}

inline void wsSendError(AsyncWebSocketClient* client, const char* message) {
    JsonDocument doc;
    doc["v"] = SKYLINK_PROTOCOL_VERSION;
    doc["type"] = "event";
    doc["event"] = "ERROR";
    doc["message"] = message;
    wsSendJson(client, doc);
}

#endif // WS_JSON_H
