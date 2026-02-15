#include "system.h"
#include "settings/system/time/ui_time.h"
#include "fonts/WixDisplay_Bold17.h"

void enterTime()
{
}

void exitTime()
{
}

void drawTime(TFT_eSprite &sprite)
{
    sprite.fillSprite(TFT_BLACK);
    drawHeader(sprite, "Time", true, "Время");
    sprite.loadFont(WixDisplay_Bold17);
    sprite.setTextDatum(MC_DATUM);
    sprite.setTextColor(TFT_WHITE);
    sprite.drawString("Пока тут пусто", 120, 80);
    sprite.setTextDatum(TL_DATUM);
}

void updateTime(TFT_eSprite &sprite)
{
}

void handleTime(bool up)
{
} 