#include "timers.h"
#include <Arduino.h>
#include <Preferences.h>

namespace {
    TimerPair s_timers[2] = { {0, 0}, {0, 0} };
    constexpr int SLOTS_PER_DAY = 48;
    constexpr int MAX_DURATION  = 8;

    Preferences prefs;
    constexpr const char* NVS_NAMESPACE = "floorheat";
    constexpr const char* KEY_T0_START  = "t0s";
    constexpr const char* KEY_T0_END    = "t0e";
    constexpr const char* KEY_T1_START  = "t1s";
    constexpr const char* KEY_T1_END    = "t1e";

    uint8_t wrapSlot(int v) {
        while (v < 0)              v += SLOTS_PER_DAY;
        while (v >= SLOTS_PER_DAY) v -= SLOTS_PER_DAY;
        return (uint8_t)v;
    }

    void loadFromNvs() {
        prefs.begin(NVS_NAMESPACE, true);   // read-only
        s_timers[0].startSlot = prefs.getUChar(KEY_T0_START, 0);
        s_timers[0].endSlot   = prefs.getUChar(KEY_T0_END,   0);
        s_timers[1].startSlot = prefs.getUChar(KEY_T1_START, 0);
        s_timers[1].endSlot   = prefs.getUChar(KEY_T1_END,   0);
        prefs.end();

        // Защита от мусора в NVS: слоты не должны превышать 48
        for (int k = 0; k < 2; ++k) {
            if (s_timers[k].startSlot > SLOTS_PER_DAY) s_timers[k].startSlot = 0;
            if (s_timers[k].endSlot   > SLOTS_PER_DAY) s_timers[k].endSlot   = 0;
        }
    }

    void saveToNvs() {
        prefs.begin(NVS_NAMESPACE, false);  // read-write
        prefs.putUChar(KEY_T0_START, s_timers[0].startSlot);
        prefs.putUChar(KEY_T0_END,   s_timers[0].endSlot);
        prefs.putUChar(KEY_T1_START, s_timers[1].startSlot);
        prefs.putUChar(KEY_T1_END,   s_timers[1].endSlot);
        prefs.end();

        Serial.printf("Timers saved: T0 %02d:%02d-%02d:%02d  T1 %02d:%02d-%02d:%02d\n",
                      s_timers[0].startSlot / 2, (s_timers[0].startSlot % 2) * 30,
                      s_timers[0].endSlot   / 2, (s_timers[0].endSlot   % 2) * 30,
                      s_timers[1].startSlot / 2, (s_timers[1].startSlot % 2) * 30,
                      s_timers[1].endSlot   / 2, (s_timers[1].endSlot   % 2) * 30);
    }
}

void timersInit() {
    loadFromNvs();
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
    // Сохраняем в NVS на каждое изменение. Запись ~10 мс — не мешает UI.
    saveToNvs();
}

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
            if (i == s) break;
        }
    }
}

int timersValidate() {
    for (int k = 0; k < 2; ++k) {
        const auto& t = s_timers[k];
        if (t.startSlot == t.endSlot) continue;
        int dur = (t.endSlot - t.startSlot + SLOTS_PER_DAY) % SLOTS_PER_DAY;
        if (dur > MAX_DURATION) return 1;
    }

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