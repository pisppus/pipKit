#include "system.h"
#include "config.h"
#include "settings/global/settings.h"
#include "settings/display/standby/sys_standby.h"
#include "settings/display/standby/ui_standby.h"
#include "settings/system/language/sys_language.h"
#include "settings/ui-helpers/menu_option/option_selector.h"
#include "settings/ui-helpers/menu_option/option_compact.h"

static const uint16_t STANDBY_TIMES[] = {0, 15, 30, 60, 120, 300, 600};
static const size_t STANDBY_COUNT = sizeof(STANDBY_TIMES) / sizeof(STANDBY_TIMES[0]);

static OptionSelector &getStandbySelector()
{
    static SelectorData data(
        "Standby Time",
        "Время ожидания",
        "Standby mode settings",
        "Настройки режима ожидания",
        "are saved after exiting.",
        "сохраняются после выхода.");

    data.config.animDuration = 500;
    data.config.saveOnChange = false;
    data.config.saveOnExit = true;

    static OptionSelector selector(
        data, STANDBY_COUNT,
        [](int id, bool isRus) -> String
        {
            uint16_t seconds = STANDBY_TIMES[id];
            if (seconds == 0)
                return isRus ? "Выкл" : "Off";
            if (seconds < 60)
                return String(seconds) + (isRus ? " сек" : " sec");
            return String(seconds / 60) + (isRus ? " мин" : " min");
        },
        [](int id)
        {
            uint16_t seconds = STANDBY_TIMES[id];
            setStandbyTime(seconds);
            saveStandbyTime(selectProfile, seconds);
        },
        []() -> int
        {
            uint16_t current = getStandbyTime(selectProfile);
            for (size_t i = 0; i < STANDBY_COUNT; i++)
            {
                if (STANDBY_TIMES[i] == current)
                    return i;
            }
            return 0;
        });

    return selector;
}

GENERATE_OPTION_FUNCTIONS(Standby, getStandbySelector)