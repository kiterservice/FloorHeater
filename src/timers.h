#pragma once
#include <stdint.h>

// Слоты по 30 минут: 0 = 00:00, 48 = 24:00.
// start == end → таймер выключен.
struct TimerPair {
    uint8_t startSlot;
    uint8_t endSlot;
};

void      timersInit();
TimerPair timersGet(int idx);           // idx: 0 или 1
void      timersAdjust(int idx, bool isStart, int deltaSlots);
bool      timersIsSet(int idx);         // start != end

// 0 = OK, иначе код ошибки:
//   1 = длительность > 4 часов
//   2 = таймеры пересекаются
int       timersValidate();

// Внутри ли текущий слот (0..48) одного из таймеров
bool      timersIsActiveAt(uint8_t currentSlot);