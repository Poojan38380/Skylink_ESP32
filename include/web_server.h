#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

struct FCTelemetry;

class WebServerModule {
private:
    AsyncWebServer server;
    AsyncWebSocket ws;
    uint32_t lastWsConnectMs = 0;
    uint32_t lastFlightCommandMs = 0;
    uint32_t lastSameCommandMs = 0;
    String lastFlightCommand;
    
    void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
    void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, AsyncWebSocketClient *client);
    bool validateCommand(const String& command, AsyncWebSocketClient* client);
    void rejectCommand(AsyncWebSocketClient* client, const String& message);
    bool isFlightCommandGateReady(String& reason) const;
    void appendSafetyState(JsonDocument& doc, const FCTelemetry& fc, bool cmdGateReady, const String& cmdGateReason);

public:
    WebServerModule(int port = 80);
    void begin();
    void sendAppState();
    void sendHeartbeat();
    void sendPendingFcEvents();
    int getWsClientCount() const;
};

extern WebServerModule webServerModule;

#endif // WEB_SERVER_H
