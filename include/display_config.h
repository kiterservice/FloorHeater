// display_config.h
#pragma once

// Пины дисплея
#define TFT_SCLK 13
#define TFT_MOSI 12
#define TFT_RST  11
#define TFT_DC   10
#define TFT_CS   8
#define TFT_BL   9

// Разрешение дисплея
#define TFT_WIDTH  360
#define TFT_HEIGHT 360

// Частота SPI
#define TFT_SPI_FREQ 80000000 // 40 MHz