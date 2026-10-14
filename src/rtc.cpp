// rtc.cpp
#include "rtc.h"
#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>

namespace {
    constexpr int PIN_SDA = 7;
    constexpr int PIN_SCL = 14;
    RTC_DS1307 s_rtc;
    bool s_ok = false;
}

void rtcInit() {
    Wire.begin(PIN_SDA, PIN_SCL);

    if (!s_rtc.begin()) {
        Serial.println("RTC: DS1307 не найден!");
        s_ok = false;
        return;
    }

    if (!s_rtc.isrunning()) {
        Serial.println("RTC: часы остановлены — проверьте батарейку");
        s_ok = false;
        return;
    }
    
    DateTime now = s_rtc.now();
    if (now.year() < 2024) {
        Serial.println("RTC: время невалидно (год < 2024)");
        s_ok = false;
        return;
    }

    s_ok = true;
    Serial.printf("RTC: OK, время %02d:%02d:%02d\n",
                  now.hour(), now.minute(), now.second());
}

bool rtcIsRunning() { return s_ok; }

int rtcGetMinutesOfDay() {
    if (!s_ok) return -1;
    DateTime now = s_rtc.now();
    return now.hour() * 60 + now.minute();
}

int rtcGetHour() {
    if (!s_ok) return -1;
    return s_rtc.now().hour();
}

int rtcGetMinute() {
    if (!s_ok) return -1;
    return s_rtc.now().minute();
}