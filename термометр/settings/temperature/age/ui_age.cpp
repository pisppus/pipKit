#include "system.h"
#include "config.h"
#include "settings/temperature/age/ui_age.h"
#include "settings/temperature/age/sys_age.h"
#include "settings/ui-helpers/menu_option/option_selector.h"
#include "settings/ui-helpers/menu_option/option_compact.h"

static const char *AGE_OPTIONS_EN[] = {"0-1 month", "1-12 months", "1-12 years", "13-17 years", "18-64 years", "65+ years"};
static const char *AGE_OPTIONS_RU[] = {"0-1 месяц", "1-12 месяцев", "1-12 лет", "13-17 лет", "18-64 года", "65+ лет"};
static const size_t AGE_OPTIONS_COUNT = sizeof(AGE_OPTIONS_EN) / sizeof(AGE_OPTIONS_EN[0]);

static OptionSelector &getAgeSelector()
{
    static SelectorData data{
        "Age",
        "Возраст",
        "The age group settings are",
        "Настройки возрастной группы",
        "saved when you exit.",
        "сохраняются при выходе."};

    data.config.animDuration = 500;
    data.config.saveOnChange = false;
    data.config.saveOnExit = true;

    static OptionSelector *instance = nullptr;
    if (!instance)
    {
        instance = new OptionSelector(
            data,
            AGE_OPTIONS_COUNT,
            [](int optionId, bool isRussian) -> String
            {
                return isRussian ? String(AGE_OPTIONS_RU[optionId]) : String(AGE_OPTIONS_EN[optionId]);
            },
            [](int optionId)
            {
                saveAge(optionId);
            },
            []() -> int
            {
                return getAge();
            });
    }
    return *instance;
}

GENERATE_OPTION_FUNCTIONS(Age, getAgeSelector)