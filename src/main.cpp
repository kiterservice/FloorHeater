// main.cpp
#include <Arduino.h>
#include <math.h>
#include "display_init.h"
#include "ui.h"
#include "buttons.h"
#include "thermistor.h"
#include "regulator.h"
#include "screensaver.h"

lgfx::LGFX_Device* tft = nullptr;

UIScreen currentScreen = UIScreen::WAIT;

float targetTemp   = 25.0f;
float measuredTemp = NAN;
bool  sensorErrorActive = false;

uint32_t lastSetTempMs    = 0;
uint32_t lastSensorReadMs = 0;

constexpr uint32_t SET_TEMP_TIMEOUT_MS   = 5000;
constexpr uint32_t SENSOR_READ_PERIOD_MS = 500;
constexpr float    SENSOR_MIN_C          = -10.0f;
constexpr float    SENSOR_MAX_C          =  35.0f;

constexpr uint8_t  BRIGHTNESS_NORMAL = 128;
constexpr uint8_t  BRIGHTNESS_DIMMED = 24;
constexpr uint32_t FADE_STEP_MS      = 15;
constexpr int      FADE_STEP         = 4;

bool     backlightEnabled  = true;
bool     transitionActive  = false;
uint8_t  currentBrightness = BRIGHTNESS_NORMAL;
uint32_t lastFadeMs        = 0;

constexpr uint32_t SCREENSAVER_TIMEOUT_MS = 10000;
constexpr uint32_t BACKLIGHT_OFF_TIMEOUT_MS = 300000;   // 5 мин до гашения подсветки
uint32_t lastActivityMs = 0;

float quantize05(float t) { return roundf(t * 2.0f) * 0.5f; }

UIScreen screenFromRegulator(RegulatorState s) {
    switch (s) {
    case RegulatorState::OFF:     return UIScreen::WAIT;
    case RegulatorState::HEATING: return UIScreen::WORK;
    case RegulatorState::HOLDING: return UIScreen::HOLD;
    }
    return UIScreen::WAIT;
}

void requestTransition() {
    if (transitionActive) return;
    transitionActive = true;
}

UIScreen uiActiveScreen() {
    return sensorErrorActive ? UIScreen::SENSOR_ERROR : currentScreen;
}

float uiActiveValue() {
    if (currentScreen == UIScreen::SET_TEMP) return targetTemp;
    if (isnan(measuredTemp))                 return 0.0f;
    return quantize05(measuredTemp);
}

void fadeLoop() {
    if (millis() - lastFadeMs < FADE_STEP_MS) return;
    lastFadeMs = millis();

    uint8_t target;
    if (transitionActive)      target = BRIGHTNESS_DIMMED;
    else if (backlightEnabled) target = BRIGHTNESS_NORMAL;
    else                       target = 0;

    if (currentBrightness == target) return;

    if (currentBrightness < target)
        currentBrightness = (uint8_t)min((int)target, currentBrightness + FADE_STEP);
    else
        currentBrightness = (uint8_t)max((int)target, currentBrightness - FADE_STEP);

    tft->setBrightness(currentBrightness);
}

void applyTransition() {
    if (!transitionActive || currentBrightness != BRIGHTNESS_DIMMED) return;
    uiDraw(uiActiveScreen(), uiActiveValue());
    transitionActive = false;
}

void processSensor(bool uiActive) {
    if (millis() - lastSensorReadMs < SENSOR_READ_PERIOD_MS) return;
    lastSensorReadMs = millis();

    const float t   = thermistorReadCelsius();
    const bool  bad = isnan(t) || t < SENSOR_MIN_C || t > SENSOR_MAX_C;

    const bool wasError = sensorErrorActive;
    sensorErrorActive = bad;
    measuredTemp = t;

    // Регулятор обновляем ВСЕГДА — даже когда UI скрыт хранителем.
    // Это критично для безопасности: если температура достигнет цели
    // или датчик отвалится, нагреватель должен быть выключен сразу,
    // а не после выхода из хранителя.
    RegulatorState rs = regulatorUpdate(t, targetTemp, !bad);

    // Если UI сейчас не работает (хранитель) — на этом всё.
    // Никаких requestTransition() и uiDraw() — они будут мешать анимации.
    if (!uiActive) return;

    if (wasError != sensorErrorActive) {
        requestTransition();
        return;
    }

    if (bad) return;

    if (currentScreen != UIScreen::SET_TEMP) {
        UIScreen want = screenFromRegulator(rs);
        if (want != currentScreen) {
            currentScreen = want;
            requestTransition();
            return;
        }

        static float lastShownQ = NAN;
        const float q = quantize05(t);
        if (isnan(lastShownQ) || fabsf(q - lastShownQ) >= 0.01f) {
            lastShownQ = q;
            uiDraw(currentScreen, q);
        }
    }
}

void processTimeout() {
    if (currentScreen != UIScreen::SET_TEMP) return;
    if (millis() - lastSetTempMs < SET_TEMP_TIMEOUT_MS) return;
    currentScreen = screenFromRegulator(regulatorGetState());
    requestTransition();
}

void processButtons() {
    Button b = buttonsPoll();
        
    if (b == Button::NONE) return;
        lastActivityMs = millis();   // любая активность сбрасывает таймер
    if (b == Button::SPARE) {
        if (transitionActive) {
            transitionActive = false;
            uiDraw(uiActiveScreen(), uiActiveValue());
        }
        backlightEnabled = !backlightEnabled;
        return;
    }

    if (transitionActive) return;
    if (sensorErrorActive) return;

    switch (b) {
    case Button::POWER:
        if (currentScreen == UIScreen::SET_TEMP) {
            currentScreen = screenFromRegulator(regulatorGetState());
        } else {
            regulatorToggle();
            regulatorUpdate(measuredTemp, targetTemp, !sensorErrorActive);
            currentScreen = screenFromRegulator(regulatorGetState());
        }
        requestTransition();
        break;

    case Button::PLUS: {
        const bool entering = (currentScreen != UIScreen::SET_TEMP);
        currentScreen = UIScreen::SET_TEMP;
        targetTemp += 0.5f;
        lastSetTempMs = millis();
        regulatorUpdate(measuredTemp, targetTemp, !sensorErrorActive);
        if (entering) requestTransition();
        else          uiDraw(currentScreen, targetTemp);
        break;
    }

    case Button::MINUS: {
        const bool entering = (currentScreen != UIScreen::SET_TEMP);
        currentScreen = UIScreen::SET_TEMP;
        targetTemp -= 0.5f;
        lastSetTempMs = millis();
        regulatorUpdate(measuredTemp, targetTemp, !sensorErrorActive);
        if (entering) requestTransition();
        else          uiDraw(currentScreen, targetTemp);
        break;
    }

    default: break;
    }
}

void setup() {
    regulatorInit();

    tft = initDisplay();
    if (!tft) return;

    uiInit(tft);
    buttonsInit();
    thermistorInit();
    screensaverInit();
    lastActivityMs = millis();

    measuredTemp = thermistorReadCelsius();
    sensorErrorActive = isnan(measuredTemp)
                     || measuredTemp < SENSOR_MIN_C
                     || measuredTemp > SENSOR_MAX_C;

    currentScreen = screenFromRegulator(regulatorGetState());

    tft->setBrightness(BRIGHTNESS_NORMAL);
    currentBrightness = BRIGHTNESS_NORMAL;

    uiDraw(uiActiveScreen(), uiActiveValue());
}

void loop() {
    fadeLoop();

    // ─── Хранитель ────────────────────────────────────────────────
    if (screensaverIsActive()) {
        // Регулятор работает, UI — нет
        processSensor(false);
        screensaverTick(millis());

        // Дополнительный таймаут: через 5 минут бездействия гасим подсветку.
        // Рыбки продолжают плавать, но экран становится невидимым.
        if (backlightEnabled
            && (millis() - lastActivityMs > BACKLIGHT_OFF_TIMEOUT_MS)) {
            backlightEnabled = false;   // fadeLoop плавно приведёт яркость к 0
            Serial.println("Backlight off (long inactivity)");
        }

        Button b = buttonsPoll();
        if (b != Button::NONE) {
            screensaverExit();
            lastActivityMs = millis();

            // Возвращаем подсветку, если её погасили по длинному таймауту
            if (!backlightEnabled) {
                backlightEnabled = true;
            }

            // Синхронизируем экран с реальным состоянием регулятора:
            // пока рыбки плавали, он мог сменить HEATING ↔ HOLDING,
            // а currentScreen остался прежним.
            if (!sensorErrorActive && currentScreen != UIScreen::SET_TEMP) {
                currentScreen = screenFromRegulator(regulatorGetState());
            }

            // Плавный выход — через диммирование, а не мгновенную перерисовку
            requestTransition();
        }
        return;
    }

    // ─── Вход в хранитель ─────────────────────────────────────────
    if (!transitionActive
        && (millis() - lastActivityMs > SCREENSAVER_TIMEOUT_MS)) {
        screensaverEnter();
        return;
    }

    // ─── Обычная логика ──────────────────────────────────────────
    applyTransition();
    processSensor(true);
    processTimeout();
    processButtons();
}