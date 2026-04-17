#include "led_controller.h"
#include "config.h"
#include "logger.h"

LedController::LedController(int pin) : pin(pin), state(false) {
}

void LedController::begin() {
    pinMode(pin, OUTPUT);
    set(false); // Default to OFF
}

void LedController::set(bool newState) {
    state = newState;
    digitalWrite(pin, state ? HIGH : LOW);
    logger.info("LED " + String(state ? "ON" : "OFF"));
}

void LedController::toggle() {
    set(!state);
}

bool LedController::getState() {
    return state;
}

LedController ledController(LED_BUILTIN_PIN);
