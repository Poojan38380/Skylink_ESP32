#ifndef BUILD_INFO_H
#define BUILD_INFO_H

#include <Arduino.h>

// Loaded from LittleFS /skylink_build.json at boot
void buildInfoBegin();
uint16_t getFirmwareBuild();
uint16_t getFsBuildOnFlash();
uint16_t getFsBuildExpected();
bool isFsBuildMatch();

#endif // BUILD_INFO_H
