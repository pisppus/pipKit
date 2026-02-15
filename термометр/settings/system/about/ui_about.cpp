#include "system.h"
#include "config.h"
#include "settings/system/about/ui_about.h"
#include "settings/system/about/sys_about.h"
#include "fonts/WixDisplay_Bold14.h"

void drawAbout(TFT_eSprite &sprite)
{
    sprite.fillSprite(TFT_BLACK);
    drawHeader(sprite, "About", true, "О системе");

    sprite.fillSmoothRoundRect(5, 35, 230, 95, 12, sprite.color565(12, 12, 12));
    sprite.drawSmoothRoundRect(5, 35, 12, 12, 230, 95, sprite.color565(28, 28, 28), sprite.color565(10, 10, 10));

    String fw_version;
    int boot_count;
    getSysInfo(fw_version, boot_count);

    sprite.loadFont(WixDisplay_Bold14);
    sprite.setTextDatum(TL_DATUM);
    sprite.setTextColor(TFT_SILVER);
    langDraw(sprite, "Firmware:", "Прошивка:", 16, 44);
    langDraw(sprite, "Uptime:", "Время работы:", 16, 66);
    langDraw(sprite, "Boot Count:", "Кол-во запусков:", 16, 88);
    langDraw(sprite, "Core:", "Ядро:", 16, 110);

    sprite.setTextColor(TFT_WHITE);
    sprite.drawString(fw_version, 147, 44);
    sprite.drawString(getUptimeString(), 147, 66);
    sprite.drawString(String(boot_count), 147, 88);
    sprite.drawString(ESP.getSdkVersion(), 147, 110);
}

void updateAbout(TFT_eSprite &sprite)
{
    drawAbout(sprite);
    sprite.pushSprite(0, 0);
}