#include "system.h"
#include "config.h"
#include "settings/ui-helpers/menu_option/option_selector.h"
#include "settings/system/language/sys_language.h"
#include "fonts/WixDisplay_Bold14.h"
#include "fonts/WixDisplay_Bold19.h"
#include "icons/global_icon.h"

OptionSelector::OptionSelector(const SelectorData &data, int numOptions,
                               OptionFormatter fmt, OptionSaver save, OptionGetter get)
    : data(data), numOptions(numOptions), formatter(fmt), saver(save), getter(get),
      currentID(0), animStart(0), prevID(0), targetID(0), direction(0) {}

void OptionSelector::updateLangCache() const
{
  if (!langCached)
  {
    isRussian = (getCurrlanguage() == "Русский");
    langCached = true;
  }
}

void OptionSelector::drawStatic(TFT_eSprite &sprite) const
{
  sprite.fillSmoothRoundRect(10, 40, 220, 40, 12, sprite.color565(12, 12, 12));
  sprite.drawSmoothRoundRect(10, 40, 12, 12, 220, 40, sprite.color565(28, 28, 28), sprite.color565(10, 10, 10));
  sprite.setTextDatum(CC_DATUM);
  sprite.loadFont(WixDisplay_Bold19);
  sprite.setTextColor(TFT_WHITE);
  sprite.drawString(formatter(currentID, isRussian), 120, 62);
}

void OptionSelector::drawAnimated(TFT_eSprite &sprite, float progress) const
{
  sprite.fillSmoothRoundRect(10, 40, 220, 40, 12, sprite.color565(12, 12, 12));
  sprite.drawSmoothRoundRect(10, 40, 12, 12, 220, 40, sprite.color565(28, 28, 28), sprite.color565(10, 10, 10));
  sprite.setViewport(15, 45, 210, 30);
  sprite.fillRect(0, 0, 210, 30, sprite.color565(12, 12, 12));
  sprite.setTextDatum(CC_DATUM);
  sprite.loadFont(WixDisplay_Bold19);

  float easedProgress = easeInOutCubic(progress);
  float offset = 140 * easedProgress * direction;

  // Рисуем предыдущий текст
  float prevX = 105 - offset;
  if (prevX > -35 && prevX < 245 && easedProgress < 0.95f)
  {
    uint8_t alpha = static_cast<uint8_t>(255 * (1.0f - easedProgress));
    sprite.setTextColor(sprite.color565(alpha, alpha, alpha));
    sprite.drawString(formatter(prevID, isRussian), static_cast<int>(prevX), 17);
  }

  // Рисуем текущий текст
  float currX = 105 + 140 * (1.0f - easedProgress) * direction;
  if (currX > -35 && currX < 245 && easedProgress > 0.05f)
  {
    uint8_t alpha = static_cast<uint8_t>(255 * easedProgress);
    sprite.setTextColor(sprite.color565(alpha, alpha, alpha));
    sprite.drawString(formatter(targetID, isRussian), static_cast<int>(currX), 17);
  }

  sprite.resetViewport();
}

void OptionSelector::draw(TFT_eSprite &sprite) const
{
  updateLangCache();
  sprite.fillSprite(TFT_BLACK);
  drawHeader(sprite, data.titleEN, true, data.titleRU);

  if (animating && (millis() - animStart >= data.config.animDuration))
  {
    const_cast<OptionSelector *>(this)->animating = false;
    const_cast<OptionSelector *>(this)->currentID = targetID;
  }

  if (animating)
  {
    float progress = min(1.0f, static_cast<float>(millis() - animStart) / data.config.animDuration);
    drawAnimated(sprite, progress);
  }
  else
  {
    drawStatic(sprite);
  }

  if (data.config.showArrows)
  {
    drawIcon(sprite, arrow_left, 20, 50, 20, 20);
    drawIcon(sprite, arrow_right, 200, 50, 20, 20);
  }

  if (data.config.showDescription && data.descEN)
  {
    sprite.loadFont(WixDisplay_Bold14);
    sprite.setTextColor(sprite.color565(70, 70, 70));
    langDraw(sprite, data.descEN, data.descRU, 10, 93);
    if (data.desc2EN)
      langDraw(sprite, data.desc2EN, data.desc2RU, 10, 108);
  }
}

void OptionSelector::handleInput(bool up)
{
  prevID = animating ? targetID : currentID;
  direction = up ? 1 : -1;
  targetID = (prevID + direction + numOptions) % numOptions;
  animating = true;
  animStart = millis();

  if (saver && data.config.saveOnChange)
    saver(targetID);
}

void OptionSelector::enter()
{
  if (getter)
  {
    int id = getter();
    currentID = (id >= 0 && id < numOptions) ? id : 0;
  }
  animating = false;
  langCached = false;
}

void OptionSelector::exit()
{
  if (saver && data.config.saveOnExit && !animating)
    saver(currentID);
}

void OptionSelector::setCurrentOption(int id, bool save)
{
  if (id >= 0 && id < numOptions)
  {
    currentID = targetID = id;
    animating = false;

    if (save && saver && data.config.saveOnChange)
      saver(currentID);
  }
}