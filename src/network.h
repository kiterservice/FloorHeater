// network.h
#pragma once

#include <stdint.h>

// Запуск подключения к Wi-Fi (не блокирующий — возвращает управление сразу).
// Подключение идёт в фоне, статус проверяется через networkIsConnected().
void networkInit(const char* ssid, const char* password);

// Обработка сетевых задач. Вызывать в loop() как можно чаще.
// Содержит: реконнект при потере Wi-Fi, запуск OTA, NTP, периодический лог.
void networkLoop();

// Подключены ли к Wi-Fi
bool networkIsConnected();

// Синхронизировано ли время (по NTP)
bool networkTimeSynced();

// Готов ли OTA-сервер (поднялся только после первого подключения к Wi-Fi)
bool networkOtaReady();

// Текущее время (или -1, если время не синхронизировано)
int networkGetHour();
int networkGetMinute();
int networkGetDayOfYear();