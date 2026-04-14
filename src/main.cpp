#include <Arduino.h>
#include "config.h"
#include "logger.h"
#include "wifi_manager.h"

unsigned long lastHeartbeat = 0;
bool ledState = false;

void setup() {
    Serial.begin(115200);
    
    logger.info("================================");
    logger.info("Skylink ESP32 Starting");
    logger.info("================================");
    
    pinMode(LED_BUILTIN_PIN, OUTPUT);
    digitalWrite(LED_BUILTIN_PIN, LOW);
    
    wifiManager.begin();
    
    logger.info("Setup complete");
}

void loop() {
    wifiManager.handle();
    
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
