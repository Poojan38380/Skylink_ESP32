#include "time_sync.h"
#include "logger.h"

TimeSync::TimeSync(long gmtOffsetSec, int daylightOffsetSec) 
    : gmtOffsetSec(gmtOffsetSec), daylightOffsetSec(daylightOffsetSec), isSynced(false) {
}

void TimeSync::begin() {
    logger.info("Configuring NTP time sync");
    logger.info("NTP Servers: " + String(ntpServer1) + ", " + String(ntpServer2));
    
    configTime(gmtOffsetSec, daylightOffsetSec, ntpServer1, ntpServer2);
}

bool TimeSync::handle() {
    if (!isSynced) {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo, 0)) {
            isSynced = true;
            logger.info("Time synchronized: " + getFormattedTime());
            return true;
        }
    }
    return isSynced;
}

String TimeSync::getFormattedTime() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 0)) {
        char timeStr[20];
        strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
        return String(timeStr);
    }
    return "N/A";
}

String TimeSync::getFormattedDate() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 0)) {
        char dateStr[20];
        strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", &timeinfo);
        return String(dateStr);
    }
    return "N/A";
}

String TimeSync::getTimestamp() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 0)) {
        char timestamp[30];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);
        return String(timestamp);
    }
    return getFormattedTime();
}

bool TimeSync::isTimeSynced() {
    return isSynced;
}

time_t TimeSync::getCurrentTime() {
    time_t now;
    time(&now);
    return now;
}

TimeSync timeSync(0, 0);
