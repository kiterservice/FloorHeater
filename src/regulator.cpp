// regulator.cpp
#include "regulator.h"
#include <Arduino.h>

namespace {

    constexpr int  PIN_HEATER          = 5;
    constexpr bool HEATER_ACTIVE_LEVEL = HIGH;

    // Гистерезис: вкл при measured < target - H, выкл при measured >= target + H.
    // Полоса вокруг цели = 2·H = 1.0 °C.
    constexpr float HYSTERESIS = 0.5f;

    bool           s_enabled  = false;
    bool           s_heaterOn = false;
    RegulatorState s_state    = RegulatorState::OFF;

    void applyHeaterPin(bool on) {
        digitalWrite(PIN_HEATER, on ? HEATER_ACTIVE_LEVEL : !HEATER_ACTIVE_LEVEL);
        s_heaterOn = on;
    }

}  // namespace

void regulatorInit() {
    // Сначала выставляем пин в неактивное состояние — чтобы при старте
    // МК нагреватель гарантированно был выключен.
    pinMode(PIN_HEATER, OUTPUT);
    applyHeaterPin(false);
    s_enabled = false;
    s_state   = RegulatorState::OFF;
}

void regulatorEnable()  { s_enabled = true; }

void regulatorDisable() {
    s_enabled = false;
    applyHeaterPin(false);
    s_state = RegulatorState::OFF;
}

void regulatorToggle() {
    if (s_enabled) regulatorDisable();
    else           regulatorEnable();
}

bool regulatorIsEnabled() { return s_enabled; }

RegulatorState regulatorUpdate(float measured, float target, bool sensorOK) {
    // 1. Аварийная защита: если датчик не в порядке — нагреватель off
    if (!sensorOK) {
        applyHeaterPin(false);
        s_state = s_enabled ? RegulatorState::HOLDING : RegulatorState::OFF;
        return s_state;
    }

    // 2. Регулятор выключен — просто держим нагрев off
    if (!s_enabled) {
        applyHeaterPin(false);
        s_state = RegulatorState::OFF;
        return s_state;
    }

    // 3. Гистерезис
    if (s_heaterOn) {
        if (measured >= target + HYSTERESIS) {
            applyHeaterPin(false);
            s_state = RegulatorState::HOLDING;
        } else {
            s_state = RegulatorState::HEATING;
        }
    } else {
        if (measured < target - HYSTERESIS) {
            applyHeaterPin(true);
            s_state = RegulatorState::HEATING;
        } else {
            s_state = RegulatorState::HOLDING;
        }
    }

    return s_state;
}

RegulatorState regulatorGetState() { return s_state; }