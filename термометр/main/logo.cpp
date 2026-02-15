#include "config.h"
#include "system.h"
#include "main/logo.h"
#include "settings/display/brightness/sys_brightness.h"
#include "settings/display/standby/sys_standby.h"
#include "settings/system/language/sys_language.h"
#include "fonts/krona_one28.h"
#include "fonts/WixDisplay_Bold16.h"

extern bool isAnimLogo;
extern unsigned long InlogoAnim;
extern uint8_t logoTargetBrightness;
extern int errorCount;
extern const int profileNum;
extern int selectProfile;
extern bool profileSelect;

void showLogo(TFT_eSprite &sprite)
{
  sprite.fillSprite(TFT_BLACK);

  // Заголовок
  sprite.loadFont(krona_one28);
  sprite.setTextColor(TFT_WHITE);
  int16_t titleHeight = sprite.fontHeight();
  int16_t titleWidth = sprite.textWidth("PISPPUS");

  // Описание
  sprite.loadFont(WixDisplay_Bold16);
  int16_t descHeight = sprite.fontHeight();
  int16_t descWidth = sprite.textWidth("Digital Thermometer");

  // Позиционирование и отрисовка
  int16_t startY = (SCREEN_HEIGHT - titleHeight - 7 - descHeight) / 2;
  sprite.loadFont(krona_one28);
  sprite.drawString("PISPPUS", (SCREEN_WIDTH - titleWidth) / 2, startY);
  sprite.loadFont(WixDisplay_Bold16);
  sprite.setTextColor(TFT_LIGHTGREY);
  sprite.drawString("Digital Thermometer", (SCREEN_WIDTH - descWidth) / 2, startY + titleHeight + 7);
  sprite.pushSprite(0, 0);
  sprite.unloadFont();
}

void updateLogo()
{
  unsigned long elapsed = millis() - InlogoAnim;

  if (elapsed < 800)
    setBrightness((uint8_t)((float)elapsed / 800.0 * logoTargetBrightness));

  if (elapsed >= 2000)
  {
    isAnimLogo = false;
    setBrightness(logoTargetBrightness);

    if (errorCount)
    {
      changeState(ERROR, SLIDE_UP);
    }
    else if (profileNum == 1)
    {
      selectProfile = 0;
      profileSelect = true;
      loadBrightness(selectProfile);
      loadStandbyTime(selectProfile);
      loadLanguage(selectProfile);
      changeState(MAIN, SLIDE_UP);
    }
    else
    {
      changeState(PROFILE, SLIDE_UP);
    }
  }
}