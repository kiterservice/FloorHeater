// display_init.h
#pragma once

#include <LovyanGFX.hpp>

lgfx::LGFX_Device* initDisplay();

// Включение/выключение подсветки.
// Возвращает новое состояние: true = подсветка включена.
bool toggleBacklight();

// Установить состояние подсветки явно
void setBacklight(bool on);