#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WebServer.h>
#include <Arduino.h>

class WebServerModule {
private:
    WebServer server;
    const int port;
    
    void handleRoot();
    void handleConfigPage();
    void handleNotFound();
    void handleDeviceInfo();
    void handleWiFiStatus();
    void handleSystemStatus();
    void handleLEDControl();
    void handleWiFiConfig();
    
    String generateDashboardHTML();
    String generateConfigHTML();
    
    void updateLED(bool state);

public:
    WebServerModule(int port = 80);
    
    void begin();
    void handle();
};

extern WebServerModule webServerModule;

#endif // WEB_SERVER_H
