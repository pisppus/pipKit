#include "system.h"
#include "config.h"
#include "settings/system/reset/ui_reset.h"
#include "settings/system/reset/sys_reset.h"
#include "settings/system/language/sys_language.h"
#include "fonts/WixDisplay_Bold17.h"
#include "fonts/WixDisplay_Bold14.h"

static bool continueActive = false;
static int confirmSelection = 0;

void enterResetInfo() { continueActive = false; }
void enterResetConfirm() { confirmSelection = 0; }

void drawResetInfo(TFT_eSprite &s)
{
    const uint16_t BTN_GREY = s.color565(12, 12, 12);
    const uint16_t BTN_BLUE = s.color565(0, 0, 138);
    const uint16_t BRD_BLUE = s.color565(0, 0, 200);
    const uint16_t BRD_GREY = s.color565(28, 28, 28);
    const uint16_t TEXT_GREY = s.color565(70, 70, 70);

    s.fillSprite(TFT_BLACK);
    drawHeader(s, "Reset Settings", true, "Сброс настроек");

    s.loadFont(WixDisplay_Bold14);
    s.setTextColor(TEXT_GREY);
    langDraw(s, "This function will reset every", "Эта функция сбросит все", 10, 38);
    langDraw(s, "thing your profile settings to", "ваши настройки профиля к", 10, 55);
    langDraw(s, "original values.", "первоначальным значениям.", 10, 72);

    drawButton(s, 55, 95, 130, 30, 9, "Continue", "Продолжить",
               continueActive ? BTN_BLUE : BTN_GREY,
               continueActive ? BRD_BLUE : BRD_GREY);
}

void drawResetConfirm(TFT_eSprite &s)
{
    const uint16_t BTN_GREY = s.color565(12, 12, 12);
    const uint16_t BTN_BLUE = s.color565(0, 0, 138);
    const uint16_t BTN_RED = s.color565(138, 0, 0);
    const uint16_t BRD_BLUE = s.color565(0, 0, 200);
    const uint16_t BRD_RED = s.color565(200, 0, 0);
    const uint16_t BRD_GREY = s.color565(28, 28, 28);
    const uint16_t TEXT_GREY = s.color565(70, 70, 70);

    s.fillSprite(TFT_BLACK);
    drawHeader(s, "Confirm", true, "Подтверждение");

    s.loadFont(WixDisplay_Bold14);
    s.setTextColor(TEXT_GREY);
    langDraw(s, "This action cannot be undone.", "Это действие нельзя откатить.", 10, 38);
    langDraw(s, "Are you sure?", "Вы уверены?", 70, 55);

    drawButton(s, 25, 95, 80, 30, 9, "Exit", "Выход",
               (confirmSelection == 0) ? BTN_BLUE : BTN_GREY,
               (confirmSelection == 0) ? BRD_BLUE : BRD_GREY);

    drawButton(s, 125, 95, 80, 30, 9, "Yes", "Да",
               (confirmSelection == 1) ? BTN_RED : BTN_GREY,
               (confirmSelection == 1) ? BRD_RED : BRD_GREY);
}

void updateResetInfo(TFT_eSprite &s) { drawResetInfo(s); }
void updateResetConfirm(TFT_eSprite &s) { drawResetConfirm(s); }

void handleResetInfoInput(bool short_press, bool long_press)
{
    if (short_press && !continueActive)
    {
        continueActive = true;
    }
    else if (long_press && continueActive)
    {
        changeState(RESET_FINAL_CONFIRM, SLIDE_UP);
    }
}

void handleResetConfirmInput(bool short_press, bool long_press, int profileId)
{
    if (short_press)
    {
        confirmSelection = 1 - confirmSelection;
    }
    else if (long_press)
    {
        if (confirmSelection == 1)
        {
            FactoryReset(profileId);
        }
        else
        {
            changeState(SYSTEM_SETTINGS, SLIDE_DOWN);
        }
    }
}