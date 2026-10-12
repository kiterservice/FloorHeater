// regulator.h
#pragma once

enum class RegulatorState {
    OFF,       // регулятор выключен, нагреватель принудительно off
    HEATING,   // температура ниже цели, нагреватель включён
    HOLDING    // температура у цели, нагреватель выключен
};

// Инициализация пина нагревателя (сразу в неактивное состояние)
void regulatorInit();

// Обновление состояния и управление пином.
// measured — текущая температура, target — целевая, sensorOK — валидны ли показания.
// Возвращает актуальное состояние регулятора.
RegulatorState regulatorUpdate(float measured, float target, bool sensorOK);

void regulatorToggle();      // кнопка POWER
void regulatorEnable();
void regulatorDisable();
bool regulatorIsEnabled();

RegulatorState regulatorGetState();