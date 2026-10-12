// display_init.cpp
#include "display_init.h"
#include "display_config.h"

// ── Константы подсветки ──────────────────────────────────────────────
static constexpr uint8_t BACKLIGHT_ON_BRIGHTNESS = 128;

// Глобальный указатель на дисплей — нужен для функций управления подсветкой
lgfx::LGFX_Device* g_display = nullptr;

// Текущее состояние подсветки (при старте — включена)
static bool s_backlightOn = true;

// ── Класс для конфигурации дисплея ───────────────────────────────────
class LGFX : public lgfx::LGFX_Device {
    lgfx::Bus_SPI _bus_instance;
    lgfx::Panel_GC9B72 _panel_instance;
    lgfx::Light_PWM _light_instance;

public:
    LGFX() {
        // --- Настройка SPI-шины ---
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = TFT_SPI_FREQ;
            cfg.freq_read  = 16000000;
            cfg.spi_3wire  = true;
            cfg.use_lock   = true;
            cfg.dma_channel = 0;  // DMA CH_AUTO не работает с GC9B72 на ESP32-S2

            cfg.pin_sclk = TFT_SCLK;
            cfg.pin_mosi = TFT_MOSI;
            cfg.pin_miso = -1;
            cfg.pin_dc   = TFT_DC;

            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        // --- Настройка панели ---
        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs           = TFT_CS;
            cfg.pin_rst          = TFT_RST;
            cfg.pin_busy         = -1;

            cfg.memory_width     = TFT_WIDTH;
            cfg.memory_height    = TFT_HEIGHT;
            cfg.panel_width      = TFT_WIDTH;
            cfg.panel_height     = TFT_HEIGHT;

            cfg.offset_x         = 0;
            cfg.offset_y         = 0;
            cfg.offset_rotation  = 0;

            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits  = 1;
            cfg.readable         = false;
            cfg.invert           = false;
            cfg.rgb_order        = true;   // без этого R и B перепутаны
            cfg.dlen_16bit       = false;
            cfg.bus_shared       = false;

            _panel_instance.config(cfg);
        }

        // --- Настройка подсветки ---
        {
            auto cfg = _light_instance.config();
            cfg.pin_bl      = TFT_BL;
            cfg.invert      = false;
            cfg.freq        = 44100;
            cfg.pwm_channel = 7;

            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance);
        }

        setPanel(&_panel_instance);
    }
};

// ── Инициализация дисплея ────────────────────────────────────────────
lgfx::LGFX_Device* initDisplay() {
    static LGFX lcd;
    lcd.init();
    lcd.setBrightness(BACKLIGHT_ON_BRIGHTNESS);
    g_display = &lcd;
    s_backlightOn = true;
    return &lcd;
}

// ── Управление подсветкой ────────────────────────────────────────────
bool toggleBacklight() {
    if (!g_display) return s_backlightOn;

    s_backlightOn = !s_backlightOn;
    g_display->setBrightness(s_backlightOn ? BACKLIGHT_ON_BRIGHTNESS : 0);
    return s_backlightOn;
}

void setBacklight(bool on) {
    if (!g_display) return;
    s_backlightOn = on;
    g_display->setBrightness(on ? BACKLIGHT_ON_BRIGHTNESS : 0);
}