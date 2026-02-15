#include <TFT_eSPI.h>
#include <pgmspace.h>
#include "system.h"
#include "config.h"
#include "settings/system/language/sys_language.h"
#include "fonts/LexendExa_Medium15.h"
#include "fonts/LexendExa_Medium17.h"
#include "fonts/WixDisplay_Bold17.h"
#include "fonts/WixDisplay_Bold18.h"
#include "fonts/WixDisplay_Bold19.h"
#include "icons/global_icon.h"

float easeInOutCubic(float t)
{
    if (t < 0.5)
    {
        return 4 * t * t * t;
    }
    else
    {
        float f = ((2 * t) - 2);
        return 0.5 * f * f * f + 1;
    }
}

uint16_t interpolateColor(uint16_t fromColor, uint16_t toColor, float progress)
{
    uint8_t fromR = fromColor >> 11, fromG = (fromColor >> 5) & 0x3F, fromB = fromColor & 0x1F;
    uint8_t toR = toColor >> 11, toG = (toColor >> 5) & 0x3F, toB = toColor & 0x1F;
    return ((uint8_t)(fromR + (toR - fromR) * progress) << 11) |
           ((uint8_t)(fromG + (toG - fromG) * progress) << 5) |
           (uint8_t)(fromB + (toB - fromB) * progress);
}

void drawButton(TFT_eSprite &s, int x, int y, int w, int h, int radius, const String &textEn, const String &textRu,
                uint16_t bgColor, uint16_t brdColor)
{
    s.fillSmoothRoundRect(x, y, w, h, radius, bgColor, s.color565(5, 5, 5));
    s.drawSmoothRoundRect(x, y, radius, radius, w, h, brdColor, bgColor);

    s.loadFont(WixDisplay_Bold17);
    s.setTextColor(TFT_WHITE);
    String text = getCurrlanguage() == "English" ? textEn : textRu;
    int textW = s.textWidth(text);
    int textH = s.fontHeight();
    langDraw(s, textEn.c_str(), textRu.c_str(), x + w / 2 - textW / 2, y + h / 2 - textH / 2 + 1);
}

void drawHeader(TFT_eSprite &sprite, const char *titleEN, bool showArrow, const char *titleRU)
{
    sprite.loadFont(WixDisplay_Bold17);
    sprite.setTextColor(TFT_WHITE);
    const char *actualTitleRU = titleRU ? titleRU : titleEN;
    langDraw(sprite, titleEN, actualTitleRU, 120, 10, TC_DATUM);

    if (showArrow)
    {
        drawIcon(sprite, arrow_left, 10, 7, 20, 20);
    }
}

void drawStatusBar(TFT_eSprite &sprite)
{
    sprite.setTextColor(TFT_WHITE);
    sprite.loadFont(LexendExa_Medium17);
    sprite.drawString(TIME_STRING, 8, 8);
    sprite.unloadFont();
    battIndicator(197, 8, BATTERY_LEVEL, sprite);
}

void battIndicator(int x, int y, int batteryLvl, TFT_eSprite &sprite)
{
    sprite.fillSmoothRoundRect(x, y, 30, 15, 4, TFT_WHITE, TFT_TRANSPARENT);
    sprite.fillSmoothRoundRect(x + 32, y + 4, 2, 7, 2, TFT_WHITE, TFT_TRANSPARENT);

    if (BATTERY_TYPE)
    {
        // Цифровой индикатор
        sprite.loadFont(LexendExa_Medium15);
        sprite.setTextColor(TFT_BLACK);
        sprite.drawString(String(batteryLvl), x + 5, y + 1);
        sprite.unloadFont();
    }
    else
    {
        // Графический индикатор
        sprite.fillSmoothRoundRect(x + 1, y + 1, 28, 13, 4, TFT_BLACK, sprite.color565(60, 60, 60));
        int fillWidth = constrain((batteryLvl * 26) / 100, 0, 26);
        if (fillWidth > 0)
        {
            uint16_t batteryColor = (batteryLvl <= 20) ? TFT_RED : sprite.color565(0, 214, 4);
            sprite.fillSmoothRoundRect(x + 2, y + 2, fillWidth, 11, 2, batteryColor, TFT_BLACK);
        }
    }
}

void drawIcon(TFT_eSprite &sprite, const uint16_t *iconData, int x, int y, int width, int height)
{
    const int totalPixels = width * height;
    uint16_t buffer[totalPixels];
    memcpy_P(buffer, iconData, totalPixels * sizeof(uint16_t));

    for (int i = 0; i < totalPixels; i++)
    {
        if (buffer[i] != 0x0000)
        {
            sprite.drawPixel(x + (i % width), y + (i / width), buffer[i]);
        }
    }
}

void langDraw(TFT_eSprite &sprite, const char *textEN, const char *textRU, int x, int y, uint8_t datum)
{
    uint8_t prevDatum = sprite.getTextDatum();
    sprite.setTextDatum(datum);
    sprite.drawString((getCurrlanguage() == "Русский") ? textRU : textEN, x, y);
    sprite.setTextDatum(prevDatum);
}