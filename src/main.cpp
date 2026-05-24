#include <Arduino.h>
#include "config.h"
#include "logger.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "config_manager.h"
#include "ota_updater.h"
#include "time_sync.h"
#include "led_controller.h"
#include "flight_controller.h"
#include "skylink_config.h"
#include "build_info.h"

unsigned long lastHeartbeat = 0;
unsigned long lastWSHeartbeat = 0;
unsigned long lastTimeSync = 0;

void setup() {
    Serial.begin(115200);
    
    logger.info("================================");
    logger.info("Skylink ESP32 Starting");
    logger.info("================================");

    ledController.begin();
    
    configManager.begin();
    wifiManager.begin();

    buildInfoBegin();
    logger.info("Build FW:" + String(getFirmwareBuild()) +
                " | FS flash:" + String(getFsBuildOnFlash()) +
                " (expected " + String(getFsBuildExpected()) + ")" +
                (isFsBuildMatch() ? " OK" : " MISMATCH — run uploadfs"));
    
    // Initialize OTA
    otaUpdater.begin();
    
    // Initialize NTP Time Sync
    timeSync.begin();
    
    // Initialize Flight Controller Bridge
    flightController.begin();
    
    // Initialize Web Server
    webServerModule.begin(); // Async server can start before WiFi is fully up
    
    logger.info("Setup complete");
}

void loop() {
    wifiManager.handle();
    otaUpdater.handle();
    flightController.handle();
    
    // Note: webServerModule.handle() is NOT needed for AsyncWebServer
    
    unsigned long now = millis();
    
    // Handle time sync periodically
    if (now - lastTimeSync >= 60000) { // Check every minute
        lastTimeSync = now;
        timeSync.handle();
    }
    
    // Send WebSocket Heartbeat (Telemetric demo)
    if (wifiManager.isConnected() && (now - lastWSHeartbeat >= SKYLINK_WS_TELEMETRY_INTERVAL_MS)) {
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
