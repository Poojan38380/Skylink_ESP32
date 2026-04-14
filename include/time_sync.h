#ifndef TIME_SYNC_H
#define TIME_SYNC_H

#include <Arduino.h>
#include <time.h>

class TimeSync {
private:
    const char* ntpServer1 = "pool.ntp.org";
    const char* ntpServer2 = "time.nist.gov";
    long gmtOffsetSec;
    int daylightOffsetSec;
    bool isSynced;
    
public:
    TimeSync(long gmtOffsetSec = 0, int daylightOffsetSec = 0);
    
    void begin();
    bool handle();
    
    String getFormattedTime();
    String getFormattedDate();
    String getTimestamp();
    bool isTimeSynced();
    time_t getCurrentTime();
};

extern TimeSync timeSync;

#endif // TIME_SYNC_H
