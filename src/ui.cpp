// ui.cpp
#include "ui.h"
#include "display_config.h"
#include "etoronum70.h"
#include "work_on_timer.h"
#include "rtc.h"

static bool     s_showTimerIcon = false;
static uint16_t s_timerIconBuf[WORK_ON_TIMER_W * WORK_ON_TIMER_H];

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
        sprite.setPsram(true); 

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

    workOnTimerFill(s_timerIconBuf);
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
    // Значок «работа по таймеру» — на всех экранах
    if (s_showTimerIcon) {
        constexpr int ICON_X = 300;
        constexpr int ICON_Y = 180;
        s_canvas->pushImage(ICON_X, ICON_Y,
                            WORK_ON_TIMER_W, WORK_ON_TIMER_H,
                            s_timerIconBuf, 0x0000);
    }
    // 3. Один раз выводим готовый холст на дисплей — цельным куском
    s_canvas->pushSprite(s_gfx, 0, 0);
}


void uiDrawTimerSetup(int cursor,
                      uint8_t s1, uint8_t e1,
                      uint8_t s2, uint8_t e2)
{
    if (!s_gfx || !s_canvas) return;

    // Мягкий тёмно-синий фон — низкая насыщенность, не «режет» глаз.
    // Альтернативы: 0x2124 (графитовый), 0x1904 (тёмный хаки),
    //               0x2985 (тёмный сине-зелёный).
    constexpr uint16_t BG_TIMER = 0xD65B;
    s_canvas->fillScreen(BG_TIMER);

    // Формат HH:MM
    auto fmt = [](char* buf, size_t n, uint8_t slot) {
        int h = slot / 2;
        int m = (slot % 2) * 30;
        snprintf(buf, n, "%02d:%02d", h, m);
    };

    char b1[8], b2[8], b3[8], b4[8];
    fmt(b1, sizeof(b1), s1);
    fmt(b2, sizeof(b2), e1);
    fmt(b3, sizeof(b3), s2);
    fmt(b4, sizeof(b4), e2);

    constexpr int TIMER_FONT_SIZE = 4;
    constexpr int CHAR_W = 6 * TIMER_FONT_SIZE;   // 24 px
    constexpr int GAP    = CHAR_W;                // один пробел
    constexpr int LINE_H = 8 * TIMER_FONT_SIZE;   // 32 px

    const int colX1 = 50;
    const int colX2 = colX1 + CHAR_W * 5 + GAP;   // 20 + 120 + 24 = 164

    // Строки чуть выше, чтобы снизу осталось место для текущего времени
    const int rowY1 = 100;
    const int rowY2 = 190;

    s_canvas->setFont(&fonts::Font0);
    s_canvas->setTextSize(TIMER_FONT_SIZE);
    s_canvas->setTextColor(TFT_WHITE, BG_TIMER);
    s_canvas->setTextDatum(lgfx::top_left);

    s_canvas->drawString(b1, colX1, rowY1);
    s_canvas->drawString(b2, colX2, rowY1);
    s_canvas->drawString(b3, colX1, rowY2);
    s_canvas->drawString(b4, colX2, rowY2);

    // Подсветка выбранного значения
    const int cursorX = (cursor % 2 == 0) ? colX1 : colX2;
    const int cursorY = (cursor < 2)      ? rowY1 : rowY2;
    const int boxW = CHAR_W * 5;
    const int boxH = LINE_H;

    s_canvas->drawRect(cursorX - 4, cursorY - 4, boxW + 8, boxH + 8, TFT_YELLOW);
    s_canvas->drawRect(cursorX - 5, cursorY - 5, boxW + 10, boxH + 10, TFT_YELLOW);

    // Текущее время снизу — чуть меньше и приглушённее, чтобы не спорить с таймерами
    if (rtcIsRunning()) {
        char timeBuf[8];
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d",
                 rtcGetHour(), rtcGetMinute());

        s_canvas->setTextSize(4);
        s_canvas->setTextColor(0xBDF7, BG_TIMER);   // светло-серый
        s_canvas->setTextDatum(lgfx::middle_center);
        s_canvas->drawString(timeBuf, 180, 285);
    }

    // Поворот на выводе — как и раньше
    s_canvas->pushRotateZoom(s_gfx, 180, 180, -45.0f, 1.0f, 1.0f, 0x0000);
}

void uiSetTimerIconVisible(bool visible) {   // ← новое
    s_showTimerIcon = visible;
}