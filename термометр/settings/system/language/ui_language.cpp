#include "system.h"
#include "config.h"
#include "settings/global/settings.h"
#include "settings/system/language/sys_language.h"
#include "settings/system/language/ui_language.h"
#include "settings/ui-helpers/menu_option/option_selector.h"
#include "settings/ui-helpers/menu_option/option_compact.h"

static const String LANGUAGE_OPTIONS[] = {"English", "Русский"};
static const size_t LANGUAGE_COUNT = sizeof(LANGUAGE_OPTIONS) / sizeof(LANGUAGE_OPTIONS[0]);

static OptionSelector &getLanguageSelector()
{
    static SelectorData data(
        "Language",
        "Язык",
        "Language changes are",
        "Изменения языка",
        "applied immediately.",
        "применяются сразу.");

    data.config.animDuration = 500;
    data.config.saveOnChange = true;
    data.config.saveOnExit = false;

    static OptionSelector selector(
        data,
        LANGUAGE_COUNT,
        [](int id, bool) -> String
        {
            return LANGUAGE_OPTIONS[id];
        },
        [](int id)
        {
            saveLanguageID(selectProfile, id);
        },
        []() -> int
        {
            return getLanguageID(selectProfile);
        });

    return selector;
}

OPTION_BASE_FUNCS(Language, getLanguageSelector)

void handleLanguage(bool up)
{
    getLanguageSelector().handleInput(up);
    getLanguageSelector().invalidateLanguageCache();
}