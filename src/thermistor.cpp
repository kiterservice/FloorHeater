// thermistor.cpp
#include "thermistor.h"
#include <Arduino.h>
#include <math.h>

namespace {

    constexpr int      PIN_NTC       = 3;
    constexpr uint32_t VCC_MV        = 3300;
    constexpr float    R_BALLAST     = 9800.0f;
    constexpr float    R0            = 15000.0f;  // NTC 10K при 25 °C
    constexpr float    T0_K          = 298.15f;
    constexpr float    BETA          = 3950.0f;

    constexpr int      SAMPLES       = 7;
    constexpr uint32_t SAMPLE_DELAY_US = 200;

    constexpr float    R_NTC_MAX     = 60000.0f;  // ≈ −5 °C — обрыв выше этого
    constexpr float    R_NTC_MIN     = 100.0f;

    float s_lastVoltage_mV = 0.0f;
    float s_lastResistance = 0.0f;

    float median(float* arr, int n) {
        for (int i = 1; i < n; ++i) {
            float key = arr[i];
            int j = i - 1;
            while (j >= 0 && arr[j] > key) { arr[j + 1] = arr[j]; --j; }
            arr[j + 1] = key;
        }
        return arr[n / 2];
    }

    float trimmedMean(const float* arr, int n) {
        float sum = 0.0f;
        int count = 0;
        for (int i = 1; i < n - 1; ++i) { sum += arr[i]; ++count; }
        return (count > 0) ? (sum / count) : arr[n / 2];
    }
}

void thermistorInit() {
    analogReadResolution(12);
    analogSetPinAttenuation(PIN_NTC, ADC_11db);
}

float thermistorReadCelsius() {
    float samples[SAMPLES];
    for (int i = 0; i < SAMPLES; ++i) {
        int raw = analogRead(PIN_NTC);
        samples[i] = raw * (float)VCC_MV / 4095.0f;
        delayMicroseconds(SAMPLE_DELAY_US);
    }

    float v_mv = trimmedMean(samples, SAMPLES);
    s_lastVoltage_mV = v_mv;

    if (v_mv < 1.0f || v_mv > (VCC_MV - 1.0f)) {
        s_lastResistance = NAN;
        return NAN;
    }

    const float vcc = (float)VCC_MV;
    float r_ntc = R_BALLAST * v_mv / (vcc - v_mv);
    s_lastResistance = r_ntc;

    if (r_ntc > R_NTC_MAX || r_ntc < R_NTC_MIN) return NAN;

    float invT = (1.0f / T0_K) + (1.0f / BETA) * logf(r_ntc / R0);
    float T_K  = 1.0f / invT;
    return T_K - 273.15f;
}

float thermistorLastVoltage_mV() { return s_lastVoltage_mV; }
float thermistorLastResistance() { return s_lastResistance; }