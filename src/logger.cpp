#include "logger.h"

Logger::Logger(LogLevel level) : currentLevel(level) {
}

const char* Logger::levelToString(LogLevel level) {
    switch (level) {
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO: return "INFO";
        case LOG_WARNING: return "WARNING";
        case LOG_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

String Logger::getTimestamp() {
    unsigned long seconds = millis() / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    
    char timestamp[20];
    sprintf(timestamp, "[%02lu:%02lu:%02lu]", hours % 24, minutes % 60, seconds % 60);
    return String(timestamp);
}

void Logger::debug(const String& message) {
    if (currentLevel <= LOG_DEBUG) {
        Serial.printf("%s [DEBUG] %s\n", getTimestamp().c_str(), message.c_str());
    }
}

void Logger::info(const String& message) {
    if (currentLevel <= LOG_INFO) {
        Serial.printf("%s [INFO] %s\n", getTimestamp().c_str(), message.c_str());
    }
}

void Logger::warning(const String& message) {
    if (currentLevel <= LOG_WARNING) {
        Serial.printf("%s [WARNING] %s\n", getTimestamp().c_str(), message.c_str());
    }
}

void Logger::error(const String& message) {
    if (currentLevel <= LOG_ERROR) {
        Serial.printf("%s [ERROR] %s\n", getTimestamp().c_str(), message.c_str());
    }
}

void Logger::setLogLevel(LogLevel level) {
    currentLevel = level;
}

Logger logger(LOG_INFO);
