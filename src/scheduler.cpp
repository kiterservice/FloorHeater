#include "scheduler.h"
#include "timers.h"
#include "rtc.h"
#include "regulator.h"
#include <Arduino.h>

namespace {
    bool s_lastInside  = false;
    bool s_initialized = false;

    // Возвращает «сейчас внутри таймера». ok=false, если RTC не работает
    bool computeNowInside(bool& ok) {
        ok = false;
        if (!rtcIsRunning()) return false;
        const int minutes = rtcGetMinutesOfDay();
        if (minutes < 0) return false;
        ok = true;
        const uint8_t slot = (uint8_t)(minutes / 30);
        return timersIsActiveAt(slot);
    }
}

void schedulerReset() {
    s_initialized = false;
    s_lastInside  = false;
}

bool schedulerPoll() {
    bool ok = false;
    const bool nowInside = computeNowInside(ok);
    if (!ok) return false;   // RTC не работает — таймеры игнорируем

    if (!s_initialized) {
        s_initialized = true;
        s_lastInside  = nowInside;
        return false;
    }

    if (nowInside == s_lastInside) return false;

    s_lastInside = nowInside;
    if (nowInside) regulatorEnable();
    else           regulatorDisable();
    Serial.printf("Scheduler: %s timer window\n", nowInside ? "inside" : "outside");
    return true;
}

bool schedulerIsTimerActive() {
    return s_initialized && s_lastInside;
}

void schedulerOnTimerSetupExit() {
    bool ok = false;
    const bool nowInside = computeNowInside(ok);
    if (!ok) return;

    s_initialized = true;
    s_lastInside  = nowInside;

    // Согласно вашему правилу: при выходе из настройки — если внутри таймера,
    // нагрев включается. Если снаружи — состояние не трогаем.
    if (nowInside) {
        regulatorEnable();
        Serial.println("Scheduler: exit setup -> inside window, heating ON");
    } else {
        Serial.println("Scheduler: exit setup -> outside window");
    }
}