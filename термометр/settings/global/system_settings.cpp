#include "config.h"
#include "system.h"
#include "settings/global/system_settings.h"
#include "settings/ui-helpers/list_menu/list_selector.h"
#include "settings/ui-helpers/list_menu/list_compact.h"
#include "fonts/WixDisplay_Bold17.h"
#include "icons/settings/system_icon.h"

const char *sysMenuEN[] = {"Language", "Time", "About", "Developer", "Reset Settings", nullptr};
const char *sysMenuRU[] = {"Язык", "Время", "О системе", "Разработчику", "Сброс настроек", nullptr};
const uint16_t *sysIcons[] = {language, timed, about, developer, reset};
const int numItems = sizeof(sysMenuEN) / sizeof(sysMenuEN[0]);

static ListMenu &getSysMenu()
{
    static ListMenu *menu = nullptr;
    if (!menu)
    {
        menu = ListMenu::createMenu(
            "System", "Система",
            sysMenuEN, sysMenuRU,
            (const uint16_t **)sysIcons,
            true,
            true);
    }
    return *menu;
}

LIST_MENU_FUNCS(SysSettings, getSysMenu)