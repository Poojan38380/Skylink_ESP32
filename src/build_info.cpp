#include "build_info.h"
#include "skylink_config.h"
#include "logger.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

static uint16_t fsBuildOnFlash = 0;

void buildInfoBegin() {
    fsBuildOnFlash = 0;

    // LittleFS is mounted by WiFiManager when loading wifi_networks.json
    if (!LittleFS.exists("/skylink_build.json")) {
        logger.warning("Missing /skylink_build.json — run: pio run --target uploadfs");
        return;
    }

    File file = LittleFS.open("/skylink_build.json", "r");
    if (!file) {
        logger.warning("Cannot open /skylink_build.json");
        return;
    }

    JsonDocument doc;
    if (deserializeJson(doc, file)) {
        logger.warning("Invalid /skylink_build.json");
        file.close();
        return;
    }
    file.close();

    fsBuildOnFlash = doc["fs"] | 0;
}

uint16_t getFirmwareBuild() {
    return SKYLINK_FIRMWARE_BUILD;
}

uint16_t getFsBuildOnFlash() {
    return fsBuildOnFlash;
}

uint16_t getFsBuildExpected() {
    return SKYLINK_FS_BUILD;
}

bool isFsBuildMatch() {
    return fsBuildOnFlash > 0 && fsBuildOnFlash == SKYLINK_FS_BUILD;
}
