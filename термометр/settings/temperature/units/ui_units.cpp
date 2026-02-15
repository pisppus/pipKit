#include "system.h"
#include "config.h"
#include "settings/temperature/units/ui_units.h"
#include "settings/temperature/units/sys_units.h"
#include "settings/ui-helpers/menu_option/option_selector.h"
#include "settings/ui-helpers/menu_option/option_compact.h"

static const char *TEMP_UNITS[] = {"°C", "°F"};
static const size_t TEMP_UNITS_COUNT = sizeof(TEMP_UNITS) / sizeof(TEMP_UNITS[0]);

static OptionSelector &getUnitsSelector()
{
    static SelectorData data{
        "Meas. units",
        "Единицы измерения",
        "The temperature units are",
        "Единицы измерения темпера",
        "saved when you exit.",
        "туры сохраняются при выходе."};

    data.config.animDuration = 500;
    data.config.saveOnChange = false;
    data.config.saveOnExit = true;

    static OptionSelector *instance = nullptr;
    if (!instance)
    {
        instance = new OptionSelector(
            data,
            TEMP_UNITS_COUNT,
            [](int optionId, bool isRussian) -> String
            {
                return String(TEMP_UNITS[optionId]);
            },
            [](int optionId)
            {
                saveTempUnit(optionId);
            },
            []() -> int
            {
                return getTempUnit();
            });
    }
    return *instance;
}

int getCurrentUnit()
{
    return getUnitsSelector().getCurrentOption();
}

void setCurrentUnit(int unit, bool save)
{
    getUnitsSelector().setCurrentOption(unit, save);
}

GENERATE_OPTION_FUNCTIONS(Units, getUnitsSelector)