#include "system.h"
#include "config.h"
#include "settings/global/display_settings.h"
#include "settings/ui-helpers/list_menu/list_selector.h"
#include "settings/ui-helpers/list_menu/list_compact.h"
#include "icons/settings/display_icon.h"

const char *displayItemsEN[] = {"Brightness", "Standby time", nullptr};
const char *displayItemsRU[] = {"Яркость", "Время ожидания", nullptr};
const uint16_t *displayIcons[] = {brightness, standby};

static ListMenu &getDisplayMenu()
{
    static ListMenu *menu = nullptr;
    if (!menu)
    {
        menu = ListMenu::createMenu(
            "Display", "Экран",
            displayItemsEN, displayItemsRU,
            displayIcons,
            false,
            true);
    }
    return *menu;
}

LIST_MENU_FUNCS(DisplaySettings, getDisplayMenu)