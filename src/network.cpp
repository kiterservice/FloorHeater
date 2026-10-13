// network.cpp
#include "network.h"
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <ArduinoOTA.h>

namespace {

    const char* s_ssid     = nullptr;
    const char* s_password = nullptr;

    bool     s_wifiStarted     = false;
    bool     s_timeConfigured  = false;
    bool     s_otaReady        = false;
    uint32_t s_lastReconnectMs = 0;

    // Не пытаться реконнектиться чаще, чем раз в 30 секунд
    constexpr uint32_t RECONNECT_PERIOD_MS = 30000;

    // NTP
    const char* NTP_SERVER1 = "pool.ntp.org";
    const char* NTP_SERVER2 = "time.nist.gov";
    const char* TZ_INFO     = "MSK-3";   // Москва, UTC+3

    // OTA
    const char* OTA_HOSTNAME = "warmfield";

}  // namespace

void networkInit(const char* ssid, const char* password) {
    s_ssid     = ssid;
    s_password = password;
    s_wifiStarted = true;

    WiFi.persistent(false);          // не писать в NVS на каждый begin
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.setSleep(false);            // не уходить в энергосбережение
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    WiFi.begin(s_ssid, s_password);

    Serial.printf("Wi-Fi: подключение к \"%s\"...\n", s_ssid);
    // НЕ ждём подключения здесь. loop() продолжит работу,
    // и UI/регулятор/датчик будут работать независимо от сети.
}

bool networkIsConnected() {
    return WiFi.status() == WL_CONNECTED;
}

bool networkTimeSynced() {
    return s_timeConfigured && (time(nullptr) > 1700000000);  // ~ноябрь 2023
}

bool networkOtaReady() {
    return s_otaReady;
}

void networkLoop() {
    const uint32_t now = millis();

    // 1. Реконнект при потере связи
    if (s_wifiStarted
        && WiFi.status() != WL_CONNECTED
        && (now - s_lastReconnectMs > RECONNECT_PERIOD_MS))
    {
        s_lastReconnectMs = now;
        Serial.println("Wi-Fi: попытка переподключения...");
        WiFi.disconnect();
        WiFi.begin(s_ssid, s_password);
    }

    // 2. Запуск OTA — один раз, когда Wi-Fi реально подключился
    if (!s_otaReady && WiFi.status() == WL_CONNECTED) {
        ArduinoOTA.setHostname(OTA_HOSTNAME);
        // ArduinoOTA.setPassword("ota_password");   // при желании
        ArduinoOTA.begin();
        s_otaReady = true;
        Serial.print("OTA: ready, IP=");
        Serial.println(WiFi.localIP());
    }

    // 3. Настройка NTP — один раз, когда Wi-Fi подключился
    if (!s_timeConfigured && WiFi.status() == WL_CONNECTED) {
        configTime(0, 0, NTP_SERVER1, NTP_SERVER2);
        setenv("TZ", TZ_INFO, 1);
        tzset();
        s_timeConfigured = true;
        Serial.print("Wi-Fi: подключено, IP=");
        Serial.println(WiFi.localIP());
        Serial.println("NTP: синхронизация времени...");
    }

    // 4. Периодический лог статуса (для отладки)
    static uint32_t lastStatusMs = 0;
    if (now - lastStatusMs > 10000) {
        lastStatusMs = now;
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("Wi-Fi OK, IP=%s, RSSI=%d dBm, время %s\n",
              WiFi.localIP().toString().c_str(),
              WiFi.RSSI(),
              networkTimeSynced() ? "синхронизировано" : "нет");
        } else {
            Serial.println("Wi-Fi: не подключён");
        }
    }
}

int networkGetHour() {
    struct tm t;
    if (!getLocalTime(&t, 0)) return -1;
    return t.tm_hour;
}

int networkGetMinute() {
    struct tm t;
    if (!getLocalTime(&t, 0)) return -1;
    return t.tm_min;
}

int networkGetDayOfYear() {
    struct tm t;
    if (!getLocalTime(&t, 0)) return -1;
    return t.tm_yday;
}