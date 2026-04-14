#include <Arduino.h>
#include "config.h"
#include "logger.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "config_manager.h"

unsigned long lastHeartbeat = 0;
bool ledState = false;

void setup() {
    Serial.begin(115200);
    
    logger.info("================================");
    logger.info("Skylink ESP32 Starting");
    logger.info("================================");
    
    pinMode(LED_BUILTIN_PIN, OUTPUT);
    digitalWrite(LED_BUILTIN_PIN, LOW);
    
    // Initialize ConfigManager
    configManager.begin();
    
    // Load WiFi credentials from EEPROM
    String ssid = configManager.getWiFiSSID();
    String password = configManager.getWiFiPassword();
    
    if (configManager.hasWiFiCredentials()) {
        logger.info("Using saved WiFi credentials: " + ssid);
    } else {
        logger.info("Using default WiFi credentials from config.h");
    }
    
    // Initialize WiFi
    wifiManager.begin();
    
    // Initialize Web Server
    if (wifiManager.isConnected()) {
        webServerModule.begin();
    }
    
    logger.info("Setup complete");
}

void loop() {
    wifiManager.handle();
    
    // Handle web server requests
    if (wifiManager.isConnected()) {
        webServerModule.handle();
    }
    
    unsigned long now = millis();
    if (now - lastHeartbeat >= HEARTBEAT_INTERVAL) {
        lastHeartbeat = now;
        
        ledState = !ledState;
        digitalWrite(LED_BUILTIN_PIN, ledState ? HIGH : LOW);
        
        logger.info("Heartbeat | WiFi: " + wifiManager.getSSID() + 
                   " | IP: " + wifiManager.getIPAddress() + 
                   " | Signal: " + String(wifiManager.getSignalStrength()) + " dBm");
    }
}
