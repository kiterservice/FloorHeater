
#pragma once

enum class Button {
    NONE, POWER, MINUS, PLUS, SPARE
};

enum class ButtonEventType {
    NONE,
    SHORT_PRESS,
    LONG_PRESS,
    REPEAT
};

struct ButtonEvent {
    ButtonEventType type;
    Button          button;
};

void        buttonsInit();
ButtonEvent buttonsPoll();
