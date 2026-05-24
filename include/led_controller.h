#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Arduino.h>

enum class LedPattern : uint8_t {
    Off = 0,
    Solid,
    BlinkSlow,
    BlinkFast
};

class LedController {
private:
    int pin;
    LedPattern autoPattern;
    LedPattern activePattern;
    bool manualOverride;
    bool pinLevel;
    unsigned long lastToggleMs;

    void applyPin(bool on);
    unsigned long blinkIntervalMs(LedPattern p) const;

public:
    explicit LedController(int pin);

    void begin();
    void update();

    void setAutoPattern(LedPattern pattern);
    void setManual(bool on);
    void toggle();
    void clearManual();

    bool getState() const;
    LedPattern getPattern() const;
    const char* getPatternName() const;
};

extern LedController ledController;

#endif // LED_CONTROLLER_H
