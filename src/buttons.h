// buttons.h
#pragma once

enum class Button {
    NONE,
    POWER,    // GPIO 37 — вкл/выкл регулятора
    MINUS,    // GPIO 38 — уменьшить температуру
    PLUS,     // GPIO 39 — увеличить температуру
    SPARE     // GPIO 40 — резерв
};

// Инициализация пинов (INPUT_PULLUP)
void buttonsInit();

// Опрос всех кнопок. Возвращает Button::NONE,
// если никаких новых событий нажатия не было.
Button buttonsPoll();