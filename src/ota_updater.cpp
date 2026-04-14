#include "ota_updater.h"
#include "logger.h"

OTAUpdater::OTAUpdater(const String& hostname) : hostname(hostname), isUpdating(false) {
}

void OTAUpdater::setupHandlers() {
    ArduinoOTA.onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        logger.info("OTA Start: " + type);
    });
    
    ArduinoOTA.onEnd([]() {
        logger.info("\nOTA End");
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        static unsigned long lastUpdate = 0;
        unsigned long now = millis();
        if (now - lastUpdate > 1000) {
            uint8_t percent = progress * 100 / total;
            logger.info("OTA Progress: " + String(percent) + "%");
            lastUpdate = now;
        }
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
        logger.error("OTA Error: " + String(error));
        if (error == OTA_AUTH_ERROR) {
            logger.error("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            logger.error("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            logger.error("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            logger.error("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            logger.error("End Failed");
        }
    });
}

void OTAUpdater::begin() {
    logger.info("Setting up OTA updater: " + hostname);
    
    ArduinoOTA.setHostname(hostname.c_str());
    setupHandlers();
    ArduinoOTA.begin();
    
    logger.info("OTA updater started. Listen for updates on: " + hostname);
}

void OTAUpdater::handle() {
    ArduinoOTA.handle();
}

bool OTAUpdater::isUpdatingInProgress() {
    return isUpdating;
}

OTAUpdater otaUpdater("esp32-skylink");
