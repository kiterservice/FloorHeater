// network.h
#pragma once

#include <stdint.h>

// Запуск подключения к Wi-Fi (не блокирующий — возвращает управление сразу).
// Подключение идёт в фоне, статус проверяется через networkIsConnected().
void networkInit(const char* ssid, const char* password);

// Обработка сетевых задач. Вызывать в loop() как можно чаще.
// Содержит: реконнект при потере, обновление времени, OTA (когда добавим).
void networkLoop();

// Подключены ли к Wi-Fi
bool networkIsConnected();

// Синхронизировано ли время (по NTP)
bool networkTimeSynced();

// Текущее время в часах и минутах (или -1, если время не синхронизировано)
int networkGetHour();
int networkGetMinute();
int networkGetDayOfYear();