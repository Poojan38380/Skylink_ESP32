#include <Arduino.h>
#include "config.h"
#include "logger.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "config_manager.h"
#include "ota_updater.h"
#include "time_sync.h"
#include "led_controller.h"

unsigned long lastHeartbeat = 0;
unsigned long lastWSHeartbeat = 0;
unsigned long lastTimeSync = 0;

void setup() {
    Serial.begin(115200);
    
    logger.info("================================");
    logger.info("Skylink ESP32 Starting");
    logger.info("================================");
    
    ledController.begin();
    
    // Initialize ConfigManager
    configManager.begin();
    
    // Initialize WiFi
    wifiManager.begin();
    
    // Initialize OTA
    otaUpdater.begin();
    
    // Initialize NTP Time Sync
    timeSync.begin();
    
    // Initialize Web Server
    webServerModule.begin(); // Async server can start before WiFi is fully up
    
    logger.info("Setup complete");
}

void loop() {
    wifiManager.handle();
    otaUpdater.handle();
    
    // Note: webServerModule.handle() is NOT needed for AsyncWebServer
    
    unsigned long now = millis();
    
    // Handle time sync periodically
    if (now - lastTimeSync >= 60000) { // Check every minute
        lastTimeSync = now;
        timeSync.handle();
    }
    
    // Send WebSocket Heartbeat (Telemetric demo)
    if (wifiManager.isConnected() && (now - lastWSHeartbeat >= 3000)) {
        lastWSHeartbeat = now;
        webServerModule.sendHeartbeat();
    }
    
    // Serial Heartbeat
    if (now - lastHeartbeat >= HEARTBEAT_INTERVAL) {
        lastHeartbeat = now;
        
        String timeStr = timeSync.isTimeSynced() ? timeSync.getTimestamp() : "Syncing...";
        
        logger.info("Heartbeat | Time: " + timeStr +
                   " | WiFi: " + wifiManager.getSSID() + 
                   " | IP: " + wifiManager.getIPAddress() + 
                   " | Signal: " + String(wifiManager.getSignalStrength()) + " dBm");
    }
}
