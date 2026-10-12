// buttons.cpp
#include "buttons.h"
#include <Arduino.h>

namespace {

    constexpr int PIN_POWER = 37;
    constexpr int PIN_MINUS = 40;
    constexpr int PIN_PLUS  = 38;
    constexpr int PIN_SPARE = 39;

    // Антидребезг: состояние считаем устойчивым, если оно
    // не менялось дольше DEBOUNCE_MS миллисекунд.
    constexpr uint32_t DEBOUNCE_MS = 30;

    struct ButtonState {
        int      pin;
        Button   id;
        bool     lastRaw;       // последнее «сырое» прочтение
        bool     stable;        // устойчивое состояние
        uint32_t lastChangeMs;  // когда lastRaw изменилось
    };

    // По умолчанию INPUT_PULLUP: HIGH = отпущена, LOW = нажата.
    ButtonState buttons[] = {
        { PIN_POWER, Button::POWER, true, true, 0 },
        { PIN_MINUS, Button::MINUS, true, true, 0 },
        { PIN_PLUS,  Button::PLUS,  true, true, 0 },
        { PIN_SPARE, Button::SPARE, true, true, 0 },
    };
    constexpr int BTN_COUNT = sizeof(buttons) / sizeof(buttons[0]);
}

void buttonsInit() {
    for (auto& b : buttons) {
        pinMode(b.pin, INPUT_PULLUP);
    }
}

Button buttonsPoll() {
    const uint32_t now = millis();
    Button event = Button::NONE;

    for (auto& b : buttons) {
        const bool raw = digitalRead(b.pin);

        // Изменилось сырое состояние — перезапускаем таймер антидребезга
        if (raw != b.lastRaw) {
            b.lastRaw      = raw;
            b.lastChangeMs = now;
        }

        // Сырое состояние устоялось достаточно долго?
        if ((now - b.lastChangeMs) >= DEBOUNCE_MS && b.stable != raw) {
            const bool wasHigh = b.stable;
            b.stable = raw;

            // Фронт HIGH -> LOW = нажатие (INPUT_PULLUP)
            if (wasHigh && !b.stable && event == Button::NONE) {
                event = b.id;   // вернём только одно событие за вызов
            }
        }
    }

    return event;
}