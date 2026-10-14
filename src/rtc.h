// rtc.h
#pragma once
#include <stdint.h>

void rtcInit();

// true — часы идут, время валидное
bool rtcIsRunning();

// Текущее время суток, минуты от начала (0..1439), -1 при ошибке
int rtcGetMinutesOfDay();

// Удобные обёртки
int rtcGetHour();     // 0..23
int rtcGetMinute();   // 0..59