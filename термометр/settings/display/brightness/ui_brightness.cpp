#include "system.h"
#include "config.h"
#include "settings/display/brightness/ui_brightness.h"
#include "settings/display/brightness/sys_brightness.h"
#include "settings/global/settings.h"
#include "fonts/WixDisplay_Bold14.h"
#include "fonts/LexendExa_Medium17.h"

static uint8_t currentBrightness;

void enterBrightness()
{
    currentBrightness = map(getBrightness(selectProfile), 2, 100, 0, 100);
}

void exitBrightness()
{
    saveBrightness(selectProfile, map(currentBrightness, 0, 100, 2, 100));
}

void drawBrightness(TFT_eSprite &sprite)
{
    sprite.fillSprite(TFT_BLACK);
    drawHeader(sprite, "Brightness", true, "Яркость");

    sprite.fillSmoothRoundRect(10, 32, SCREEN_WIDTH - 20, 54, 12, sprite.color565(12, 12, 12));
    sprite.drawSmoothRoundRect(10, 32, 12, 12, SCREEN_WIDTH - 20, 54, sprite.color565(28, 28, 28), sprite.color565(10, 10, 10));

    sprite.setTextDatum(ML_DATUM);
    sprite.loadFont(LexendExa_Medium17);
    sprite.drawString(String(currentBrightness) + "%", 20, 49);

    sprite.fillSmoothRoundRect(20, 66, SCREEN_WIDTH - 40, 10, 8, TFT_DARKGREY);
    int fill_w = map(currentBrightness, 0, 100, 0, SCREEN_WIDTH - 40);
    if (fill_w > 0)
        sprite.fillSmoothRoundRect(20, 66, fill_w, 10, 8, TFT_BLUE);

    sprite.loadFont(WixDisplay_Bold14);
    sprite.setTextColor(sprite.color565(80, 80, 80));
    sprite.setTextDatum(TL_DATUM);
    langDraw(sprite, "Brightness settings are saved", "Настройки яркости сохраня", 10, 97);
    langDraw(sprite, "when you exit the menu.", "ются после выхода.", 10, 112);
}

void updateBrightness(TFT_eSprite &sprite)
{
    drawBrightness(sprite);
    sprite.pushSprite(0, 0);
}

void handleBrightness(bool up)
{
    currentBrightness = constrain(currentBrightness + (up ? 1 : -1), 0, 100);
    setBrightness(map(currentBrightness, 0, 100, 2, 100));
}