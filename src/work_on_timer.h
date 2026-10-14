// work_on_timer.h
#pragma once
#include <stdint.h>

constexpr int WORK_ON_TIMER_W = 50;
constexpr int WORK_ON_TIMER_H = 50;

// Заполняет буфер 50×50 RGB565 прототипом значка.
// Фон прозрачный (0x0000 — ключ прозрачности для pushImage).
inline void workOnTimerFill(uint16_t* buf) {
    constexpr uint16_t TRANSP = 0x0000;
    constexpr uint16_t YELLOW = 0xFFE0;
    constexpr uint16_t ORANGE = 0xFD20;

    for (int i = 0; i < WORK_ON_TIMER_W * WORK_ON_TIMER_H; ++i) buf[i] = TRANSP;

    constexpr int cx = 25, cy = 25;
    constexpr int rOuter2 = 22 * 22;
    constexpr int rInner2 = 18 * 18;

    // Кольцо
    for (int y = 0; y < 50; ++y) {
        for (int x = 0; x < 50; ++x) {
            const int dx = x - cx, dy = y - cy;
            const int d2 = dx * dx + dy * dy;
            if (d2 <= rOuter2 && d2 >= rInner2) buf[y * 50 + x] = YELLOW;
        }
    }

    // Буква T
    for (int y = 15; y <= 19; ++y)
        for (int x = 14; x <= 35; ++x) buf[y * 50 + x] = ORANGE;
    for (int y = 19; y <= 36; ++y)
        for (int x = 22; x <= 27; ++x) buf[y * 50 + x] = ORANGE;
}