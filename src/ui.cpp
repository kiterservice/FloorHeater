// ui.cpp
#include "ui.h"
#include "display_config.h"
#include "etoronum70.h"

static lgfx::LGFX_Device*   s_gfx    = nullptr;
static lgfx::LGFX_Sprite*   s_canvas = nullptr;   // полноэкранный холст

namespace {

    constexpr int SCREEN_W  = 360;
    constexpr int SCREEN_H  = 360;
    constexpr int CENTER_X  = 180;
    constexpr int CENTER_Y  = 180;

    constexpr float TEXT_ANGLE = -45.0f;

    constexpr int SPRITE_W  = 320;
    constexpr int SPRITE_H  = 200;

    constexpr uint16_t BG_WAIT     = 0x8410;
    constexpr uint16_t BG_SET_TEMP = 0x867E;
    constexpr uint16_t BG_WORK     = 0xFAF9;
    constexpr uint16_t BG_HOLD     = 0xFE46;
    constexpr uint16_t BG_ERROR    = TFT_BLACK;

    constexpr int DOT_RADIUS   = 12;
    constexpr int RING_RADIUS  = 100;
    constexpr int DOT_COUNT    = 5;

    // Рисуем текст во внутренний маленький спрайт, потом с поворотом
    // впечатываем его в холст-приёмник.
    void drawRotatedTextInto(lgfx::LGFX_Sprite* dst,
                             const char* text,
                             uint16_t fgColor,
                             uint16_t bgColor)
    {
        lgfx::LGFX_Sprite sprite(dst);
        sprite.setColorDepth(16);

        if (!sprite.createSprite(SPRITE_W, SPRITE_H)) {
            Serial.println("Text sprite allocation FAILED!");
            return;
        }

        sprite.setFont(&ETORONUM70pt7b);
        sprite.setTextSize(1.5);
        sprite.fillSprite(bgColor);
        sprite.setTextColor(fgColor, bgColor);
        sprite.setTextDatum(lgfx::middle_center);
        sprite.drawString(text, SPRITE_W / 2, SPRITE_H / 2);

        sprite.pushRotateZoom(dst, CENTER_X, CENTER_Y,
                              TEXT_ANGLE, 1.0f, 1.0f, 0.5f);
        sprite.deleteSprite();
    }

    void formatTemperature(char* buf, size_t bufSize, float temp) {
        snprintf(buf, bufSize, "%.1f", temp);
    }

    void drawErrorRingInto(lgfx::LGFX_Sprite* dst) {
        for (int i = 0; i < DOT_COUNT; ++i) {
            float a = (i * 360.0f / DOT_COUNT - 90.0f) * PI / 180.0f;
            int x = CENTER_X + (int)(RING_RADIUS * cosf(a));
            int y = CENTER_Y + (int)(RING_RADIUS * sinf(a));
            dst->fillCircle(x, y, DOT_RADIUS, TFT_WHITE);
        }
    }

}  // namespace

// ────────────────────────────────────────────────────────────────────
void uiInit(lgfx::LGFX_Device* gfx)
{
    s_gfx = gfx;

    s_canvas = new lgfx::LGFX_Sprite(gfx);
    s_canvas->setColorDepth(16);
    s_canvas->setPsram(true);   // 253 КБ — только в PSRAM

    if (!s_canvas->createSprite(SCREEN_W, SCREEN_H)) {
        Serial.println("Canvas allocation FAILED — no PSRAM?");
        delete s_canvas;
        s_canvas = nullptr;
        return;
    }

    Serial.printf("Canvas OK, %d bytes in PSRAM\n", SCREEN_W * SCREEN_H * 2);
}

void uiDraw(UIScreen screen, float temperature)
{
    if (!s_gfx || !s_canvas) return;

    uint16_t bg;
    bool     withText = true;

    switch (screen) {
    case UIScreen::WAIT:         bg = BG_WAIT;     break;
    case UIScreen::SET_TEMP:     bg = BG_SET_TEMP; break;
    case UIScreen::WORK:         bg = BG_WORK;     break;
    case UIScreen::HOLD:         bg = BG_HOLD;     break;
    case UIScreen::SENSOR_ERROR: bg = BG_ERROR; withText = false; break;
    default:                     bg = BG_WAIT;     break;
    }

    // 1. Заливаем холст фоном — это происходит ВНЕ экрана, мерцания нет
    s_canvas->fillScreen(bg);

    // 2. Рисуем на холст содержимое
    if (withText) {
        char buf[16];
        formatTemperature(buf, sizeof(buf), temperature);
        drawRotatedTextInto(s_canvas, buf, TFT_WHITE, bg);
    } else {
        drawErrorRingInto(s_canvas);
    }

    // 3. Один раз выводим готовый холст на дисплей — цельным куском
    s_canvas->pushSprite(s_gfx, 0, 0);
}
