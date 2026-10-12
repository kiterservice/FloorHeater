// thermistor.h
#pragma once

// Инициализация ADC-пина
void thermistorInit();

// Возвращает температуру в °C. При ошибке — NAN.
float thermistorReadCelsius();

// Диагностика (для Serial)
float thermistorLastVoltage_mV();
float thermistorLastResistance();