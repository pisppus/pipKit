#include "system.h"
#include "config.h"
#include "settings/global/settings.h"
#include "settings/ui-helpers/list_menu/list_selector.h"
#include "settings/ui-helpers/list_menu/list_compact.h"
#include "fonts/WixDisplay_Bold17.h"
#include "icons/settings/setting_icon.h"

static const char *settingItemsEN[] = {"Display", "Temperature", "WiFi", "Bluetooth", "System", nullptr};
static const char *settingItemsRU[] = {"Экран", "Температура", "WiFi", "Bluetooth", "Система", nullptr};
static const uint16_t *settingIcons[] = {display, temperature, wifi, bluetooth, systemd};

static bool isSelectionMode = false;
static ListMenu &getSettingsMenu()
{
    static ListMenu *menu = nullptr;
    if (!menu)
    {
        menu = ListMenu::createMenu(
            "Settings", "Настройки",
            settingItemsEN, settingItemsRU,
            settingIcons,
            true,
            false);
    }
    return *menu;
}

LIST_MENU_FUNCS_NDRW(Settings, getSettingsMenu)

void drawSettings(TFT_eSprite &sprite)
{
    if (isSelectionMode)
    {
        getSettingsMenu().draw(sprite);
        return;
    }
    sprite.fillSprite(TFT_BLACK);
    sprite.loadFont(WixDisplay_Bold17);
    sprite.setViewport((SCREEN_WIDTH - 231) / 2, 0, 231, SCREEN_HEIGHT);

    const uint16_t defBg = sprite.color565(12, 12, 12);
    const uint16_t defBorder = sprite.color565(20, 20, 20);

    static int itemCount = -1;
    if (itemCount == -1)
    {
        itemCount = 0;
        while (settingItemsEN[itemCount] != nullptr)
        {
            itemCount++;
        }
    }

    for (int i = 0; i < itemCount; i++)
    {
        const int y = i * 35 + 31;

        sprite.fillSmoothRoundRect(2, y, SCREEN_WIDTH - 13, 30, 7, defBg);
        sprite.drawSmoothRoundRect(2, y, 7, 7, SCREEN_WIDTH - 13, 30, defBorder, defBg);
        drawIcon(sprite, settingIcons[i], 12, y + 5, 20, 20);
        langDraw(sprite, settingItemsEN[i], settingItemsRU[i], 37, y + 16, ML_DATUM);
    }

    sprite.resetViewport();
    sprite.fillRect(0, 0, SCREEN_WIDTH, 31, TFT_BLACK);
    drawHeader(sprite, "Settings", false, "Настройки");
}

void toggleSelectionMode()
{
    isSelectionMode = !isSelectionMode;
}

bool getSelectionMode()
{
    return isSelectionMode;
}