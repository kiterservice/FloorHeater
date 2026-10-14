// ui.h
#pragma once
#include <LovyanGFX.hpp>

// Все экраны интерфейса
enum class UIScreen {
    WAIT,          // серый,    измеренная температура
    SET_TEMP,      // синий,    устанавливаемая температура
    WORK,          // красный,  текущая температура
    HOLD,          // зелёный,  текущая температура
    SENSOR_ERROR   // чёрный,   5 белых точек по кольцу
};

// Инициализация модуля: запоминаем указатель на дисплей
void uiInit(lgfx::LGFX_Device* gfx);

// Отрисовка экрана с заданной температурой (для SENSOR_ERROR температура игнорируется)
void uiDraw(UIScreen screen, float temperature);

void uiDrawTimerSetup(int cursor,
                      uint8_t s1, uint8_t e1,
                      uint8_t s2, uint8_t e2);

void uiSetTimerIconVisible(bool visible);