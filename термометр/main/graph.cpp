#include "system.h"
#include "config.h"
#include "main/graph.h"
#include "fonts/WixDisplay_Bold17.h"

const int DAYS_IN_MONTH = 31;
bool animationComplete = false;
unsigned long animStartTime = 0;

// День, длина блока, значение, цвет
struct GraphData
{
  uint8_t day, len, value;
  uint16_t color;
};

const GraphData graphBlocks[] PROGMEM = {
    {0, 1, 71, TFT_RED_BAR},
    {1, 1, 57, TFT_ORANGE_BAR},
    {2, 1, 71, TFT_RED_BAR},
    {3, 5, 29, TFT_BLUE_BAR},
    {8, 3, 15, TFT_PURPLE_BAR},
    {11, 3, 29, TFT_BLUE_BAR},
    {14, 1, 57, TFT_ORANGE_BAR},
    {15, 2, 43, TFT_GREEN_BAR},
    {17, 1, 15, TFT_PURPLE_BAR},
    {18, 1, 29, TFT_BLUE_BAR},
    {19, 1, 43, TFT_GREEN_BAR},
    {20, 1, 57, TFT_ORANGE_BAR},
    {21, 1, 71, TFT_RED_BAR},
    {22, 1, 57, TFT_ORANGE_BAR},
    {23, 1, 29, TFT_BLUE_BAR},
    {24, 1, 15, TFT_PURPLE_BAR},
    {25, 1, 71, TFT_RED_BAR},
    {26, 1, 43, TFT_GREEN_BAR},
    {27, 1, 57, TFT_ORANGE_BAR},
    {28, 1, 43, TFT_GREEN_BAR},
    {29, 1, 15, TFT_PURPLE_BAR},
    {30, 1, 43, TFT_GREEN_BAR}};

uint16_t darkenColor(uint16_t color, float factor)
{
  uint8_t r = ((color >> 11) & 0x1F) * factor;
  uint8_t g = ((color >> 5) & 0x3F) * factor;
  uint8_t b = (color & 0x1F) * factor;
  return (r << 11) | (g << 5) | b;
}

void drawMonth(TFT_eSprite &sprite)
{
  const char *monthText = "Sept 2024";
  sprite.loadFont(WixDisplay_Bold17);
  int16_t w = sprite.textWidth(monthText) + 14;
  int16_t h = sprite.fontHeight() + 6;
  int16_t x = (SCREEN_WIDTH - w) / 2;
  sprite.fillSmoothRoundRect(x, 5, w, h, 6, sprite.color565(0, 30, 255), TFT_BLACK);
  sprite.setTextColor(TFT_WHITE);
  sprite.drawString(monthText, x + 7, 9);
  sprite.unloadFont();
}

void drawGraph(TFT_eSprite &sprite)
{
  const int maxHeight = 70, bottom = 110, barWidth = (SCREEN_WIDTH - 10) / DAYS_IN_MONTH;
  const int radius = max(1, barWidth / 3);

  float progress = 1.0f;
  if (!animationComplete)
  {
    progress = min(1.0f, (float)(millis() - animStartTime) / 1000);
    progress = 1.0f - pow(1.0f - progress, 2); // ease-out
  }

  int currentDay = 0;
  for (const auto &block : graphBlocks)
  {
    while (currentDay < block.day)
    {
      int x = currentDay * barWidth;
      sprite.fillSmoothRoundRect(x, bottom - maxHeight, barWidth - 2, maxHeight,
                                 radius, darkenColor(TFT_DARKGREY, 0.2f), TFT_BLACK);
      currentDay++;
    }

    for (int i = 0; i < block.len && currentDay < DAYS_IN_MONTH; i++, currentDay++)
    {
      int x = currentDay * barWidth;
      int bgHeight = maxHeight;
      int activeHeight = map(block.value, 0, 100, 0, maxHeight) * progress;

      sprite.fillSmoothRoundRect(x, bottom - bgHeight, barWidth - 2, bgHeight,
                                 radius, darkenColor(block.color, 0.2f), TFT_BLACK);
      if (activeHeight > 0)
      {
        sprite.fillSmoothRoundRect(x, bottom - activeHeight, barWidth - 2, activeHeight,
                                   radius, block.color, TFT_BLACK);
      }
    }
  }

  sprite.loadFont(WixDisplay_Bold17);
  sprite.setTextColor(TFT_WHITE);
  sprite.drawString("01", 2, 117);
  sprite.drawString("31", SCREEN_WIDTH - sprite.textWidth("31") - 2, 117);
  sprite.setTextColor(TFT_DARKGREY);
  sprite.drawString(". . . . . . . . . . . . . . . . . .", 20, 113);
  sprite.unloadFont();
}

void showGraph(TFT_eSprite &sprite)
{
  sprite.fillSprite(TFT_BLACK);
  drawStatusBar(sprite);
  drawMonth(sprite);
  drawGraph(sprite);
}