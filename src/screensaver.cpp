// screensaver.cpp
#include "screensaver.h"
#include "display_init.h"
#include "fish_data.h"
#include "grass_data.h"
#include <Arduino.h>
#include <math.h>

extern lgfx::LGFX_Device* tft;

namespace {

    constexpr int SPRITE_W = 64;
    constexpr int SPRITE_H = 64;
    constexpr int SCREEN_W = 360;
    constexpr int SCREEN_H = 360;

    constexpr float CENTER_X = SCREEN_W * 0.5f;
    constexpr float CENTER_Y = SCREEN_H * 0.5f;

    constexpr float SWIM_RADIUS = 170.0f;

    // Масштаб отрисовки рыбок: 1.35 = +35 %
    constexpr float FISH_ZOOM = 1.3f;
    constexpr float FISH_SIZE = SPRITE_W * FISH_ZOOM;   // ≈ 86.4
    constexpr float FISH_HALF = FISH_SIZE * 0.5f;       // ≈ 43.2

    // Тёмный бирюзовый: R=0, G=64, B=64
    constexpr uint16_t BG_WATER    = 0x0618;
    constexpr uint16_t TRANSPARENT = 0x0000;

    const uint16_t* FRAMES_BLUE [4] = { fish_blue_f0,  fish_blue_f1,  fish_blue_f2,  fish_blue_f3  };
    const uint16_t* FRAMES_GREEN[4] = { fish_green_f0, fish_green_f1, fish_green_f2, fish_green_f3 };
    const uint16_t* FRAMES_RED  [4] = { fish_red_f0,   fish_red_f1,   fish_red_f2,   fish_red_f3   };

    lgfx::LGFX_Sprite* SPR_BLUE [4] = { nullptr, nullptr, nullptr, nullptr };
    lgfx::LGFX_Sprite* SPR_GREEN[4] = { nullptr, nullptr, nullptr, nullptr };
    lgfx::LGFX_Sprite* SPR_RED  [4] = { nullptr, nullptr, nullptr, nullptr };


    constexpr int GRASS_W = 50;
    constexpr int GRASS_H = 50;

    // Позиции кустов (левый верхний угол)
    constexpr int GRASS1_X = 300;
    constexpr int GRASS1_Y = 240;
    constexpr int GRASS2_X = 100;
    constexpr int GRASS2_Y = 240;
    // Период смены кадра водорослей (мс). Один цикл = GRASS_FRAME_MS * 2
    constexpr uint32_t GRASS_FRAME_MS = 500;

    // Размах покачивания по X (пиксели)
    constexpr float GRASS_SWAY_AMPL = 3.0f;

    constexpr float GRASS_ZOOM = 6.0f;

    // Храним оба кадра каждой водоросли
    lgfx::LGFX_Sprite* s_grass1[2] = { nullptr, nullptr };
    lgfx::LGFX_Sprite* s_grass2[2] = { nullptr, nullptr };

    struct Fish {
        lgfx::LGFX_Sprite** sprites;
        float x, y;            // ЦЕНТР рыбки
        float vx;
        int   frameIdx;
        uint32_t lastFrameMs;
    };

    Fish fishes[4];
    bool s_active = false;
    lgfx::LGFX_Sprite* s_canvas = nullptr;

    float frand(float lo, float hi) {
        return lo + (random(1000) / 1000.0f) * (hi - lo);
    }

void buildGrassSprite(const uint16_t* f0, const uint16_t* f1,
                          lgfx::LGFX_Sprite* out[2]) {
        const uint16_t* src[2] = { f0, f1 };
        for (int i = 0; i < 2; ++i) {
            out[i] = new lgfx::LGFX_Sprite(tft);
            out[i]->setColorDepth(16);
            out[i]->setPsram(true);
            if (!out[i]->createSprite(GRASS_W, GRASS_H)) {
                Serial.printf("Grass sprite alloc FAILED (frame %d)\n", i);
                delete out[i];
                out[i] = nullptr;
                continue;
            }
            out[i]->fillSprite(TRANSPARENT);
            for (int y = 0; y < GRASS_H; ++y) {
                for (int x = 0; x < GRASS_W; ++x) {
                    uint16_t p = src[i][y * GRASS_W + x];
                    if (p != TRANSPARENT) out[i]->drawPixel(x, y, p);
                }
            }
        }
    }

void buildSprites(const uint16_t* frames[4], lgfx::LGFX_Sprite* out[4]) {
        for (int i = 0; i < 4; ++i) {
            out[i] = new lgfx::LGFX_Sprite(tft);
            out[i]->setColorDepth(16);
            out[i]->setPsram(true);
            if (!out[i]->createSprite(SPRITE_W, SPRITE_H)) {
                Serial.printf("Fish sprite alloc FAILED (frame %d)\n", i);
                delete out[i];
                out[i] = nullptr;
                continue;
            }
            out[i]->fillSprite(TRANSPARENT);   // фон спрайта = прозрачный
                for (int y = 0; y < SPRITE_H; ++y) {
                for (int x = 0; x < SPRITE_W; ++x) {
                 uint16_t p = frames[i][y * SPRITE_W + x];
                 // Копируем ВСЁ, кроме пикселей, которые уже равны TRANSPARENT.
                // Тогда фон спрайта останется прозрачным.
               if (p != TRANSPARENT) {
                out[i]->drawPixel(x, y, p);
        }
    }
}
        }
    }

void initFish(Fish& f, lgfx::LGFX_Sprite** sprites, float yCenter) {
        f.sprites = sprites;
        f.x       = CENTER_X + frand(-60.0f, 60.0f);
        f.y       = yCenter;
        f.vx      = (random(2) ? 1 : -1) * frand(0.3f, 0.8f);
        f.frameIdx = 0;
        f.lastFrameMs = millis();
    }

}  // namespace

void screensaverInit() {
    s_canvas = new lgfx::LGFX_Sprite(tft);
    s_canvas->setColorDepth(16);
    s_canvas->setPsram(true);
    if (!s_canvas->createSprite(SCREEN_W, SCREEN_H)) {
        Serial.println("Screensaver canvas FAILED");
        delete s_canvas;
        s_canvas = nullptr;
        return;
    }

    buildSprites(FRAMES_BLUE,  SPR_BLUE);
    buildSprites(FRAMES_GREEN, SPR_GREEN);
    buildSprites(FRAMES_RED,   SPR_RED);

    buildGrassSprite(grass_f0, grass_f1, s_grass1);
    buildGrassSprite(grass_f0, grass_f1, s_grass2);

    Serial.println("Screensaver ready");
}

bool screensaverIsActive() { return s_active; }

void screensaverEnter() {
    if (s_active || !s_canvas) return;
    s_active = true;

    // Шесть дорожек по Y: от верхней к нижней.
    // Разброс ±100 от центра — рыбки не выходят за круг радиусом 170.
    initFish(fishes[0], SPR_BLUE,  CENTER_Y - 100.0f);
    initFish(fishes[1], SPR_GREEN, CENTER_Y - 60.0f);
    initFish(fishes[2], SPR_RED,   CENTER_Y - 20.0f);
    initFish(fishes[3], SPR_RED,  CENTER_Y + 20.0f);
    //initFish(fishes[4], SPR_GREEN, CENTER_Y + 60.0f);
    //initFish(fishes[5], SPR_RED,   CENTER_Y + 100.0f);
}

void screensaverExit() {
    s_active = false;
}

void screensaverTick(uint32_t nowMs) {
    if (!s_active || !s_canvas) return;

    s_canvas->fillScreen(BG_WATER);

     // Водоросли — фон. Стоят на месте, только меняют кадр (колышутся).
    const int grassFrame = ((nowMs / GRASS_FRAME_MS) % 2);   // 0 или 1

    if (s_grass1[grassFrame]) {
        s_grass1[grassFrame]->pushRotateZoom(
            s_canvas,
            GRASS1_X + GRASS_W * 0.5f,          // центр по X — без сдвига
            GRASS1_Y + GRASS_H * 0.5f,          // центр по Y
            0.0f,                             // поворот, как у рыбок
            GRASS_ZOOM, GRASS_ZOOM,             // масштаб 3×
            TRANSPARENT);
    }

    if (s_grass2[grassFrame]) {
        s_grass2[grassFrame]->pushRotateZoom(
            s_canvas,
            GRASS2_X + GRASS_W * 0.5f,
            GRASS2_Y + GRASS_H * 0.5f,
            0.0f,
            GRASS_ZOOM, GRASS_ZOOM,
            TRANSPARENT);
    }

    // Максимальное удаление центра рыбки от центра экрана,
    // чтобы рыбка целиком оставалась в круге радиусом SWIM_RADIUS
    const float maxCenterDist = SWIM_RADIUS - FISH_HALF;

    for (auto& f : fishes) {
        // Анимация и движение — каждые 120 мс.
        // Скорость ×3.0f
        if (nowMs - f.lastFrameMs >= 120) {
            f.lastFrameMs = nowMs;
            f.frameIdx = (f.frameIdx + 1) % 4;
            f.x += f.vx * 3.0f;
        }

        // Покачивание по Y для живости
        const float yWobble = sinf(nowMs * 0.002f + f.x * 0.05f) * 5.0f;
        const float drawY   = f.y + yWobble;

        // Границы круга для текущей Y
        const float dy    = drawY - CENTER_Y;
        const float dyC   = constrain(dy, -maxCenterDist, maxCenterDist);
        const float maxDx = sqrtf(maxCenterDist * maxCenterDist - dyC * dyC);
        const float minX  = CENTER_X - maxDx;
        const float maxX  = CENTER_X + maxDx;

        // Отскок с рандомизацией направления
        if (f.x < minX) {
            f.x  = minX;
            f.vx = frand(0.5f, 1.5f);                   // вправо
            f.y += frand(-20.0f, 20.0f);
            f.y  = constrain(f.y, CENTER_Y - 100.0f, CENTER_Y + 100.0f);
        }
        if (f.x > maxX) {
            f.x  = maxX;
            f.vx = -frand(0.5f, 1.5f);                  // влево
            f.y += frand(-20.0f, 20.0f);
            f.y  = constrain(f.y, CENTER_Y - 100.0f, CENTER_Y + 100.0f);
        }

        lgfx::LGFX_Sprite* spr = f.sprites[f.frameIdx];
        if (!spr) continue;

        // Ориентация.
        // Исходные спрайты смотрят ВЛЕВО (голова в −X).
        //   vx < 0 (плывёт влево)  → оригинал
        //   vx > 0 (плывёт вправо) → зеркалим через отрицательный zoom_x
        const float zoomX = (f.vx > 0) ? -FISH_ZOOM : FISH_ZOOM;
        const float zoomY = FISH_ZOOM;

       spr->pushRotateZoom(s_canvas, f.x, drawY, 0.0f, zoomX, zoomY, TRANSPARENT);
    }

        // Холст — без поворота (рисуется как есть),
    // а на экран выводим один раз с поворотом −45°.
    s_canvas->pushRotateZoom(tft,
                             CENTER_X, CENTER_Y,
                             -45.0f,
                             1.0f, 1.0f,
                             TRANSPARENT);
}