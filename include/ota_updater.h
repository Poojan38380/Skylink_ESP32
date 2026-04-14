#ifndef OTA_UPDATER_H
#define OTA_UPDATER_H

#include <Arduino.h>
#include <ArduinoOTA.h>

class OTAUpdater {
private:
    String hostname;
    bool isUpdating;
    
    void setupHandlers();

public:
    OTAUpdater(const String& hostname = "esp32-skylink");
    
    void begin();
    void handle();
    bool isUpdatingInProgress();
};

extern OTAUpdater otaUpdater;

#endif // OTA_UPDATER_H
