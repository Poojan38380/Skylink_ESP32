#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Arduino.h>

class LedController {
private:
    int pin;
    bool state;

public:
    LedController(int pin);
    void begin();
    void set(bool newState);
    void toggle();
    bool getState();
};

extern LedController ledController;

#endif // LED_CONTROLLER_H
