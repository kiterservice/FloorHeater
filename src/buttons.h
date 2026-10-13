/*
// buttons.h
#pragma once

enum class Button {
    NONE,
    POWER,    // GPIO 37 — вкл/выкл регулятора
    MINUS,    // GPIO 40 — уменьшить температуру
    PLUS,     // GPIO 38 — увеличить температуру
    SPARE     // GPIO 39 — резерв
};

// Инициализация пинов (INPUT_PULLUP)
void buttonsInit();

// Опрос всех кнопок. Возвращает Button::NONE,
// если никаких новых событий нажатия не было.
Button buttonsPoll();
*/

#pragma once

enum class Button {
    NONE, POWER, MINUS, PLUS, SPARE
};

enum class ButtonEventType {
    NONE,
    SHORT_PRESS,
    LONG_PRESS
};

struct ButtonEvent {
    ButtonEventType type;
    Button          button;
};

void        buttonsInit();
ButtonEvent buttonsPoll();
