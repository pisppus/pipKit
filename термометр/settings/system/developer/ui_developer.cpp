#include "system.h"
#include "config.h"
#include "settings/system/developer/ui_developer.h"
#include "settings/system/developer/sys_developer.h"
#include "fonts/WixDisplay_Bold14.h"
#include "fonts/WixDisplay_Bold17.h"

void drawDeveloper(TFT_eSprite &sprite)
{
    sprite.fillSprite(TFT_BLACK);
    drawHeader(sprite, "Developer", true, "Разработчику");

    sprite.fillSmoothRoundRect(5, 35, 230, 95, 12, sprite.color565(12, 12, 12));
    sprite.drawSmoothRoundRect(5, 35, 12, 12, 230, 95, sprite.color565(28, 28, 28), sprite.color565(10, 10, 10));

    sprite.loadFont(WixDisplay_Bold14);
    sprite.setTextDatum(TL_DATUM);
    sprite.setTextColor(TFT_SILVER);
    langDraw(sprite, "CPU Freq:", "Частота:", 16, 45);
    langDraw(sprite, "Free Heap:", "Своб. память:", 16, 67);
    langDraw(sprite, "FS Usage:", "Исп. памяти:", 16, 89);
    langDraw(sprite, "Reset:", "Сброс:", 16, 111);

    sprite.setTextColor(TFT_WHITE);
    sprite.drawString(getCpuFreqString(), 140, 45);
    sprite.drawString(getFreeHeapString(), 140, 67);
    sprite.drawString(getFsUsageString(), 140, 89);
    sprite.drawString(getResetReasonString(), 140, 111);
}

void updateDeveloper(TFT_eSprite &sprite)
{
    drawDeveloper(sprite);
    sprite.pushSprite(0, 0);
}