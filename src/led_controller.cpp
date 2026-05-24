#include "led_controller.h"
#include "config.h"
#include "logger.h"
#include "skylink_config.h"

LedController::LedController(int pin)
    : pin(pin),
      autoPattern(LedPattern::Off),
      activePattern(LedPattern::Off),
      manualOverride(false),
      pinLevel(false),
      lastToggleMs(0) {}

void LedController::begin() {
    pinMode(pin, OUTPUT);
    applyPin(false);
}

unsigned long LedController::blinkIntervalMs(LedPattern p) const {
    switch (p) {
        case LedPattern::BlinkFast:
            return SKYLINK_LED_ARMED_BLINK_MS;
        case LedPattern::BlinkSlow:
            return SKYLINK_LED_MAVLINK_BLINK_MS;
        default:
            return 0;
    }
}

void LedController::applyPin(bool on) {
    pinLevel = on;
    digitalWrite(pin, on ? HIGH : LOW);
}

void LedController::setAutoPattern(LedPattern pattern) {
    if (manualOverride) return;
    if (autoPattern == pattern) return;

    autoPattern = pattern;
    activePattern = pattern;
    lastToggleMs = millis();

    if (pattern == LedPattern::Solid) {
        applyPin(true);
    } else if (pattern == LedPattern::Off) {
        applyPin(false);
    } else {
        applyPin(false);
    }

    logger.info(String("LED auto → ") + getPatternName());
}

void LedController::setManual(bool on) {
    manualOverride = true;
    activePattern = on ? LedPattern::Solid : LedPattern::Off;
    applyPin(on);
    logger.info(String("LED manual → ") + (on ? "ON" : "OFF"));
}

void LedController::toggle() {
    setManual(!pinLevel);
}

void LedController::clearManual() {
    manualOverride = false;
    activePattern = autoPattern;
    lastToggleMs = millis();
    if (autoPattern == LedPattern::Solid) {
        applyPin(true);
    } else {
        applyPin(false);
    }
}

void LedController::update() {
    if (manualOverride) return;

    activePattern = autoPattern;
    const unsigned long interval = blinkIntervalMs(activePattern);
    if (interval == 0) return;

    const unsigned long now = millis();
    if (now - lastToggleMs >= interval) {
        lastToggleMs = now;
        applyPin(!pinLevel);
    }
}

bool LedController::getState() const {
    return pinLevel;
}

LedPattern LedController::getPattern() const {
    return manualOverride ? activePattern : autoPattern;
}

const char* LedController::getPatternName() const {
    switch (getPattern()) {
        case LedPattern::Solid:
            return "solid";
        case LedPattern::BlinkSlow:
            return "blink_slow";
        case LedPattern::BlinkFast:
            return "blink_fast";
        default:
            return "off";
    }
}

LedController ledController(LED_BUILTIN_PIN);
