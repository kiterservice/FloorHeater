#include "buttons.h"
#include <Arduino.h>

namespace {
    constexpr int PIN_POWER = 38;
    constexpr int PIN_MINUS = 40;
    constexpr int PIN_PLUS  = 37;
    constexpr int PIN_SPARE = 39;

    constexpr uint32_t DEBOUNCE_MS        = 30;
    constexpr uint32_t LONG_PRESS_MS      = 5000;
    constexpr uint32_t REPEAT_START_MS    = 500;    // через 0.5 c после нажатия
    constexpr uint32_t REPEAT_INTERVAL_MS = 300;    // каждые 0.3 c

    struct ButtonState {
        int      pin;
        Button   id;
        bool     lastRaw;
        bool     stable;
        uint32_t lastChangeMs;
        uint32_t pressStartMs;
        bool     longFired;
        bool     repeatStarted;
        uint32_t lastRepeatMs;
        bool     allowRepeat;
        bool     allowLongPress;
    };

    ButtonState buttons[] = {
        { PIN_POWER, Button::POWER, true, true, 0, 0, false, false, 0, false, true  },
        { PIN_MINUS, Button::MINUS, true, true, 0, 0, false, false, 0, true,  false },
        { PIN_PLUS,  Button::PLUS,  true, true, 0, 0, false, false, 0, true,  false },
        { PIN_SPARE, Button::SPARE, true, true, 0, 0, false, false, 0, false, false },
    };
}

void buttonsInit() {
    for (auto& b : buttons) pinMode(b.pin, INPUT_PULLUP);
}

ButtonEvent buttonsPoll() {
    const uint32_t now = millis();
    ButtonEvent result = { ButtonEventType::NONE, Button::NONE };

    for (auto& b : buttons) {
        const bool raw = digitalRead(b.pin);

        if (raw != b.lastRaw) { b.lastRaw = raw; b.lastChangeMs = now; }

        if ((now - b.lastChangeMs) >= DEBOUNCE_MS && b.stable != raw) {
            const bool wasPressed = (b.stable == false);
            b.stable = raw;

            if (!wasPressed && !b.stable) {
                // Нажали
                b.pressStartMs  = now;
                b.longFired     = false;
                b.repeatStarted = false;
                b.lastRepeatMs  = now;
            } else if (wasPressed && b.stable) {
                // Отпустили
                if (!b.longFired && !b.repeatStarted
                    && result.type == ButtonEventType::NONE) {
                    result = { ButtonEventType::SHORT_PRESS, b.id };
                }
                b.longFired     = false;
                b.repeatStarted = false;
            }
        }

        // События «удержания»
        if (b.stable == false) {
            // Автоповтор
            if (b.allowRepeat
                && (now - b.pressStartMs) >= REPEAT_START_MS
                && (now - b.lastRepeatMs) >= REPEAT_INTERVAL_MS) {
                b.lastRepeatMs  = now;
                b.repeatStarted = true;
                if (result.type == ButtonEventType::NONE) {
                    result = { ButtonEventType::REPEAT, b.id };
                }
            }
            // Долгое нажатие
            if (b.allowLongPress && !b.longFired
                && (now - b.pressStartMs) >= LONG_PRESS_MS) {
                b.longFired = true;
                if (result.type == ButtonEventType::NONE) {
                    result = { ButtonEventType::LONG_PRESS, b.id };
                }
            }
        }
    }
    return result;
}