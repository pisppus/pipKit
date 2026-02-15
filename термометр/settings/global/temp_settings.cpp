#include "system.h"
#include "config.h"
#include "settings/global/temp_settings.h"
#include "settings/ui-helpers/list_menu/list_selector.h"
#include "settings/ui-helpers/list_menu/list_compact.h"
#include "icons/settings/temp_icon.h"

const char *tempSettingEN[] = {"Age", "Meas. units", nullptr};
const char *tempSettingRU[] = {"Возраст", "Единицы измерения", nullptr};
const int numTempSettings = sizeof(tempSettingEN) / sizeof(tempSettingEN[0]);
const uint16_t *tempSettingIcons[] = {age, units};

static ListMenu &getTempMenu()
{
  static ListMenu *menu = nullptr;
  if (!menu)
  {
    menu = ListMenu::createMenu(
        "Temperature", "Температура",
        tempSettingEN, tempSettingRU,
        (const uint16_t **)tempSettingIcons,
        false,
        true);
  }
  return *menu;
}

LIST_MENU_FUNCS(TempSettings, getTempMenu)