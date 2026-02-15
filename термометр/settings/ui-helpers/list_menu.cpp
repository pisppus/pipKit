#include <algorithm>
#include "system.h"
#include "config.h"
#include "settings/ui-helpers/list_menu/list_selector.h"
#include "settings/system/language/sys_language.h"
#include "fonts/WixDisplay_Bold17.h"
#include "icons/global_icon.h"

ListMenu::ListMenu(const MenuData &menuData, ItemFormatter fmt, ItemHandler select,
                   ItemGetter get, ItemAvailable avail)
    : data(menuData), formatter(fmt), handler(select), getter(get), availability(avail),
      selectedItem(0), prevSelectedItem(0), animating(false), scrollAnimating(false),
      animStartTime(0), scrollAnimStart(0), scrollActivity(0),
      currScrollY(0), targetScrollY(0), startScrollY(0), maxScrollY(0),
      colorsInitialized(false), fontLoaded(false), langCached(false), isRussian(false)
{
    calculateMaxScroll();

    if (!formatter)
    {
        formatter = [](int, bool) -> String
        { return ""; };
    }
}

void ListMenu::updateLangCache() const
{
    if (!langCached)
    {
        isRussian = (getCurrlanguage() == "Русский");
        langCached = true;
    }
}

void ListMenu::initializeColors(TFT_eSprite &sprite)
{
    if (!colorsInitialized)
    {
        const auto &cfg = data.config;
        cachedDefBg = sprite.color565(cfg.defaultBgColor.r, cfg.defaultBgColor.g, cfg.defaultBgColor.b);
        cachedDefBorder = sprite.color565(cfg.defaultBorderColor.r, cfg.defaultBorderColor.g, cfg.defaultBorderColor.b);
        cachedSelBg = sprite.color565(cfg.selectBgColor.r, cfg.selectBgColor.g, cfg.selectBgColor.b);
        cachedSelBorder = sprite.color565(cfg.selectBorderColor.r, cfg.selectBorderColor.g, cfg.selectBorderColor.b);
        colorsInitialized = true;
    }
}

ListMenu *ListMenu::createMenu(const char *titleEN, const char *titleRU,
                               const char *itemsEN[], const char *itemsRU[],
                               const uint16_t **icons, bool enableScroll, bool showHeaderArrow)
{
    if (!itemsEN)
        return nullptr;

    int count = 0;
    while (itemsEN[count])
        count++;

    auto menu = new ListMenu(
        MenuData(titleEN, titleRU, count, icons),
        [itemsEN, itemsRU](int id, bool isRussian) -> String
        {
            return isRussian ? itemsRU[id] : itemsEN[id];
        });

    menu->data.config.enableScroll = enableScroll;
    menu->data.config.showHeaderArrow = showHeaderArrow;

    return menu;
}

void ListMenu::enter()
{
    int getterValue = getter ? getter() : 0;
    selectedItem = prevSelectedItem = (getterValue >= 0 && getterValue < data.itemCount) ? getterValue : 0;
    animating = scrollAnimating = false;
    langCached = false;
    fontLoaded = false;
    currScrollY = targetScrollY = data.config.enableScroll ? (selectedItem * 35.0f - 31.0f) : 0.0f;
    scrollActivity = millis();
}

void ListMenu::draw(TFT_eSprite &sprite)
{
    updateLangCache();
    initializeColors(sprite);

    if (scrollAnimating)
    {
        unsigned long elapsed = millis() - scrollAnimStart;
        if (elapsed >= 250)
        {
            currScrollY = targetScrollY;
            scrollAnimating = false;
        }
        else
        {
            currScrollY = startScrollY + (targetScrollY - startScrollY) * easeInOutCubic(elapsed / 250.0f);
        }
    }

    sprite.fillSprite(TFT_BLACK);

    if (!fontLoaded)
    {
        sprite.loadFont(WixDisplay_Bold17);
        fontLoaded = true;
    }

    const int viewportX = (SCREEN_WIDTH - 231) / 2;
    sprite.setViewport(viewportX, 0, 231, SCREEN_HEIGHT);

    float colorProgress = 1.0f;
    if (animating)
    {
        unsigned long elapsed = millis() - animStartTime;
        animating = elapsed < 250;
        if (animating)
            colorProgress = easeInOutCubic(elapsed / 250.0f);
    }

    const bool hasIcons = data.icons != nullptr;
    const int screenWidth_13 = SCREEN_WIDTH - 13;
    const int rectWidth = screenWidth_13;
    const int iconX = 12;
    const int textX = 37;

    int *availableIndices = nullptr;
    int availableCount = 0;

    if (availability && data.itemCount > 0)
    {
        availableIndices = new int[data.itemCount];
        for (int i = 0; i < data.itemCount; i++)
        {
            if (availability(i))
            {
                availableIndices[availableCount++] = i;
            }
        }
    }

    for (int idx = 0; idx < (availability ? availableCount : data.itemCount); idx++)
    {
        int i = availability ? availableIndices[idx] : idx;
        int y = data.config.enableScroll ? (i * 35 - round(currScrollY)) : (i * 35 + 31);

        if (y + 35 < 0 || y > SCREEN_HEIGHT)
            continue;

        uint16_t bg, border;

        if (i == selectedItem)
        {
            bg = animating ? interpolateColor(cachedDefBg, cachedSelBg, colorProgress) : cachedSelBg;
            border = animating ? interpolateColor(cachedDefBorder, cachedSelBorder, colorProgress) : cachedSelBorder;
        }
        else if (i == prevSelectedItem && animating)
        {
            bg = interpolateColor(cachedSelBg, cachedDefBg, colorProgress);
            border = interpolateColor(cachedSelBorder, cachedDefBorder, colorProgress);
        }
        else
        {
            bg = cachedDefBg;
            border = cachedDefBorder;
        }

        sprite.fillSmoothRoundRect(2, y, rectWidth, 30, 7, bg);
        sprite.drawSmoothRoundRect(2, y, 7, 7, rectWidth, 30, border, bg);

        if (hasIcons && data.icons[i])
            drawIcon(sprite, data.icons[i], iconX, y + 5, 20, 20);

        String itemText = formatter(i, isRussian);
        sprite.setTextColor(TFT_WHITE);
        langDraw(sprite, itemText.c_str(), itemText.c_str(), textX, y + 16, ML_DATUM);
    }

    if (availableIndices)
    {
        delete[] availableIndices;
        availableIndices = nullptr;
    }

    sprite.resetViewport();
    sprite.fillRect(0, 0, SCREEN_WIDTH, 31, TFT_BLACK);
    drawHeader(sprite, data.titleEN, data.config.showHeaderArrow, data.titleRU);

    unsigned long timeSince = millis() - scrollActivity;
    if (scrollAnimating)
    {
        scrollActivity = millis();
        timeSince = 0;
    }

    uint8_t alpha = timeSince < 1500 ? 255 : (timeSince < 2000 ? 255 * (1.0f - easeInOutCubic((timeSince - 1500) / 500.0f)) : 0);

    if (alpha > 0 && maxScrollY > 0)
    {
        float handleH = std::max(10.0f, SCREEN_HEIGHT * SCREEN_HEIGHT / (data.itemCount * 35.0f));
        float handleY = (SCREEN_HEIGHT - handleH) * std::max(0.0f, currScrollY) / maxScrollY;
        sprite.fillSmoothRoundRect(SCREEN_WIDTH - 3, round(handleY), 3, round(handleH), 2, cachedDefBorder);
    }

    sprite.setTextDatum(TL_DATUM);
}

void ListMenu::handleInput(bool up)
{
    if (animating || scrollAnimating)
        return;

    const int direction = up ? -1 : 1;
    int newSelection = selectedItem;
    int attempts = 0;

    do
    {
        newSelection = (newSelection + direction + data.itemCount) % data.itemCount;
        if (++attempts >= data.itemCount)
            return;
    } while (availability && !availability(newSelection));

    prevSelectedItem = selectedItem;
    selectedItem = newSelection;
    animating = true;
    animStartTime = millis();

    if (data.config.enableScroll)
    {
        const float newTargetY = std::min(selectedItem * 35.0f - 31.0f, maxScrollY);

        if (std::abs(newTargetY - targetScrollY) > 0.1f)
        {
            startScrollY = currScrollY;
            targetScrollY = newTargetY;
            scrollAnimating = true;
            scrollAnimStart = millis();
        }
    }
}

void ListMenu::setSelectedItem(int id)
{
    if (id < 0 || id >= data.itemCount || (availability && !availability(id)))
    {
        return;
    }
    selectedItem = id;
    animating = false;
    if (data.config.enableScroll)
    {
        const float itemPosition = selectedItem * 35.0f - 31.0f;
        targetScrollY = currScrollY = std::min(std::max(0.0f, itemPosition), maxScrollY);
        scrollAnimating = false;
    }
    scrollActivity = millis();
}