#include "timers.h"
#include <Arduino.h>

namespace {
    TimerPair s_timers[2] = { {0, 0}, {0, 0} };
    constexpr int SLOTS_PER_DAY = 48;    // 24 ч × 2 слота
    constexpr int MAX_DURATION  = 8;     // 4 часа = 8 слотов

    uint8_t wrapSlot(int v) {
        while (v < 0)             v += SLOTS_PER_DAY;
        while (v >= SLOTS_PER_DAY) v -= SLOTS_PER_DAY;
        return (uint8_t)v;
    }
}

void timersInit() {
    s_timers[0] = { 0, 0 };
    s_timers[1] = { 0, 0 };
}

TimerPair timersGet(int idx) {
    if (idx < 0 || idx > 1) return { 0, 0 };
    return s_timers[idx];
}

bool timersIsSet(int idx) {
    const auto& t = s_timers[idx];
    return t.startSlot != t.endSlot;
}

void timersAdjust(int idx, bool isStart, int deltaSlots) {
    if (idx < 0 || idx > 1) return;
    auto& t = s_timers[idx];
    if (isStart) t.startSlot = wrapSlot((int)t.startSlot + deltaSlots);
    else         t.endSlot   = wrapSlot((int)t.endSlot   + deltaSlots);
}

// Возвращает набор слотов, которые покрывает таймер
namespace {
    void collectSlots(const TimerPair& t, bool out[SLOTS_PER_DAY]) {
        for (int i = 0; i < SLOTS_PER_DAY; ++i) out[i] = false;
        if (t.startSlot == t.endSlot) return;
        int s = t.startSlot;
        int e = t.endSlot;
        int i = s;
        while (true) {
            out[i] = true;
            i = (i + 1) % SLOTS_PER_DAY;
            if (i == e) break;
            if (i == s) break;   // защита от зацикливания
        }
    }
}

int timersValidate() {
    // Проверка длительности (только для включённых таймеров)
    for (int k = 0; k < 2; ++k) {
        const auto& t = s_timers[k];
        if (t.startSlot == t.endSlot) continue;
        int dur = (t.endSlot - t.startSlot + SLOTS_PER_DAY) % SLOTS_PER_DAY;
        if (dur > MAX_DURATION) return 1;
    }

    // Проверка пересечения
    if (timersIsSet(0) && timersIsSet(1)) {
        bool a[SLOTS_PER_DAY], b[SLOTS_PER_DAY];
        collectSlots(s_timers[0], a);
        collectSlots(s_timers[1], b);
        for (int i = 0; i < SLOTS_PER_DAY; ++i) {
            if (a[i] && b[i]) return 2;
        }
    }
    return 0;
}

bool timersIsActiveAt(uint8_t currentSlot) {
    for (int k = 0; k < 2; ++k) {
        const auto& t = s_timers[k];
        if (t.startSlot == t.endSlot) continue;
        int s = t.startSlot;
        int e = t.endSlot;
        int i = s;
        while (true) {
            if (i == currentSlot) return true;
            i = (i + 1) % SLOTS_PER_DAY;
            if (i == e) break;
            if (i == s) break;
        }
    }
    return false;
}