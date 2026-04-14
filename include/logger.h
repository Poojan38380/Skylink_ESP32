#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

enum LogLevel {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARNING = 2,
    LOG_ERROR = 3
};

class Logger {
private:
    LogLevel currentLevel;
    
    const char* levelToString(LogLevel level);
    String getTimestamp();

public:
    Logger(LogLevel level = LOG_INFO);
    
    void debug(const String& message);
    void info(const String& message);
    void warning(const String& message);
    void error(const String& message);
    
    void setLogLevel(LogLevel level);
};

extern Logger logger;

#endif // LOGGER_H
