#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

class WebServerModule {
private:
    AsyncWebServer server;
    AsyncWebSocket ws;
    
    void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
    void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, AsyncWebSocketClient *client);

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
