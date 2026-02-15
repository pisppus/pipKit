#include "config.h"
#include "system.h"
#include <cmath>
#include "main/select_profile.h"
#include "fonts/WixDisplay_Bold17.h"
#include "fonts/WixDisplay_Bold14.h"
#include "icons/global_icon.h"

// ------------------------------------------- Профили ----------------------------------------------
const char *profileNames[] = {"Profile 1", "Profile 2", "Profile 3"};               // Имена профилей (М)
uint16_t profileColors[] = {TFT_RED, TFT_ORANGE, TFT_YELLOW};                       // Цвета профилей (М)
const int profileNum = sizeof(profileNames) / sizeof(profileNames[0]);         // Количество профилей
bool profileSelect = false;                                                      // Выбран ли профиль
// --------------------------------------------------------------------------------------------------

void drawProfile(TFT_eSprite &sprite)
{
  sprite.fillSprite(TFT_BLACK);
  sprite.loadFont(WixDisplay_Bold17);
  int textWidth = sprite.textWidth(profileNames[selectProfile]);
  sprite.setTextColor(TFT_WHITE);
  sprite.drawString(profileNames[selectProfile], (SCREEN_WIDTH - textWidth) / 2, 10);

  drawDotsIndicator(sprite, selectProfile, 0, 0.0f, false, 0);
  sprite.fillSmoothRoundRect(100, 48, 40, 40, 8, profileColors[selectProfile], TFT_BLACK);
  sprite.drawSmoothRoundRect(94, 43, 12, 12, 51, 50, TFT_WHITE, sprite.color565(85, 85, 85));

  sprite.fillSmoothCircle(25, 67, 10, sprite.color565(40, 40, 40), sprite.color565(15, 15, 15));
  drawIcon(sprite, arrow_left, 15, 58, 20, 20);
  sprite.fillSmoothCircle(215, 67, 10, sprite.color565(40, 40, 40), sprite.color565(15, 15, 15));
  drawIcon(sprite, arrow_right, 206, 58, 20, 20);
  sprite.unloadFont();
}

void drawDotsIndicator(TFT_eSprite &sprite, int currProfileId, int prevProfileId, float animProgress, bool animate, int animDirection)
{
  int dotsY = SCREEN_HEIGHT - 13;
  int dotsStartX = (SCREEN_WIDTH - (profileNum - 1) * 15) / 2;
  for (int i = 0; i < profileNum; i++)
  {
    sprite.fillSmoothCircle(dotsStartX + i * 15, SCREEN_HEIGHT - 13, 3, sprite.color565(100, 100, 100), sprite.color565(50, 50, 50));
  }

  if (animate && profileNum > 1)
  {
    float x_prev = dotsStartX + prevProfileId * 15;
    float x_curr = dotsStartX + currProfileId * 15;
    float drawn_left_x, drawn_right_x;
    if (animProgress < 0.5f)
    {
      float normalized_p = animProgress * 2.0f;
      if (animDirection > 0)
      {
        drawn_left_x = x_prev;
        drawn_right_x = x_prev + (x_curr - x_prev) * normalized_p;
      }
      else
      {
        drawn_right_x = x_prev;
        drawn_left_x = x_prev + (x_curr - x_prev) * normalized_p;
      }
    }
    else
    {
      float normalized_p = (animProgress - 0.5f) * 2.0f;
      if (animDirection > 0)
      {
        drawn_right_x = x_curr;
        drawn_left_x = x_prev + (x_curr - x_prev) * normalized_p;
      }
      else
      {
        drawn_left_x = x_curr;
        drawn_right_x = x_prev + (x_curr - x_prev) * normalized_p;
      }
    }

    if (drawn_left_x > drawn_right_x)
    {
      float temp = drawn_left_x;
      drawn_left_x = drawn_right_x;
      drawn_right_x = temp;
    }
    sprite.fillSmoothCircle(round(drawn_left_x), dotsY, 3, TFT_WHITE, sprite.color565(100, 100, 100));
    sprite.fillSmoothCircle(round(drawn_right_x), dotsY, 3, TFT_WHITE, sprite.color565(100, 100, 100));
    if (abs(drawn_right_x - drawn_left_x) > 3 * 0.5f)
    {
      sprite.fillSmoothRoundRect(round(drawn_left_x), dotsY - 3, round(drawn_right_x - drawn_left_x), 7, 0, TFT_WHITE, sprite.color565(100, 100, 100));
    }
  }
  else
  {
    sprite.fillSmoothCircle(dotsStartX + currProfileId * 15, dotsY, 3, TFT_WHITE, sprite.color565(100, 100, 100));
  }
}

void updateRollerAnim(TFT_eSprite &tempSprite, float progress, int animationDirection)
{
  tempSprite.fillSprite(TFT_BLACK);
  int prevIdx = selectProfile - animationDirection;
  if (prevIdx < 0)
    prevIdx = profileNum - 1;
  if (prevIdx >= profileNum)
    prevIdx = 0;

  drawDotsIndicator(tempSprite, selectProfile, prevIdx, progress, true, animationDirection);

  float easedProgress = progress;
  float alphaProgress = 0.0f;
  if (progress >= 0.1f && progress <= 0.9f)
  {
    alphaProgress = (progress - 0.1f) / (0.9f - 0.1f);
  }
  else if (progress > 0.9f)
  {
    alphaProgress = 1.0f;
  }

  // Уходящий текст
  tempSprite.loadFont(progress > 0.4f ? WixDisplay_Bold14 : WixDisplay_Bold17);
  int prevTextWidth = tempSprite.textWidth(profileNames[prevIdx]);
  uint8_t prevAlpha = 255 * (1.0f - alphaProgress);
  int prevX = (240 - prevTextWidth) / 2 - (int)(progress * 100 * animationDirection);
  int yPosPrev = (progress > 0.4f) ? 12 : 10;

  if (prevX + prevTextWidth > 0 && prevX < 240)
  {
    tempSprite.setTextColor(tempSprite.color565(prevAlpha, prevAlpha, prevAlpha), TFT_BLACK);
    tempSprite.drawString(profileNames[prevIdx], prevX, yPosPrev);
  }

  // Приходящий текст
  tempSprite.loadFont(progress < 0.6f ? WixDisplay_Bold14 : WixDisplay_Bold17);
  int nextTextWidth = tempSprite.textWidth(profileNames[selectProfile]);
  uint8_t nextAlpha = 255 * alphaProgress;
  int nextX = (240 - nextTextWidth) / 2 + (int)((1.0f - progress) * 100 * animationDirection);
  int yPosNext = (progress < 0.6f) ? 12 : 10;

  if (nextX + nextTextWidth > 0 && nextX < 240)
  {
    tempSprite.setTextColor(tempSprite.color565(nextAlpha, nextAlpha, nextAlpha), TFT_BLACK);
    tempSprite.drawString(profileNames[selectProfile], nextX, yPosNext);
  }
  tempSprite.unloadFont();

  int animOffset = (int)(progress * 150) * animationDirection;

  for (int offset = -2; offset <= 2; offset++)
  {
    int index = (prevIdx + offset + profileNum) % profileNum;
    int x = 120 + (offset * 150 - animOffset);

    if (x + 40 >= 0 && x <= 240)
    {
      float distFromCenter = abs(x - 120) / (float)150;
      float scale = 1.0f - 0.5f * min(distFromCenter, 1.0f);
      int scaledSize = 40 * scale;
      int y = 68 - scaledSize / 2;
      int yOffset = (int)((distFromCenter * distFromCenter) * 15.0f);

      uint16_t bgColor = profileColors[index];
      if (distFromCenter > 0.2)
      {
        uint8_t r = (bgColor >> 11) & 0x1F;
        uint8_t g = (bgColor >> 5) & 0x3F;
        uint8_t b = bgColor & 0x1F;
        float dimFactor = max(0.4f, 1.0f - distFromCenter);
        bgColor = tempSprite.color565(r * dimFactor * 8, g * dimFactor * 4, b * dimFactor * 8);
      }

      bool onScreen = (x - scaledSize / 2 < 260 && x + scaledSize / 2 > 0);
      bool betweenArrows = (x + scaledSize / 2 > 35 && x - scaledSize / 2 < 205);

      if (onScreen && betweenArrows)
      {
        tempSprite.fillSmoothRoundRect(x - scaledSize / 2, y + yOffset, scaledSize, scaledSize, 8 * scale, bgColor, TFT_BLACK);
        if (distFromCenter < 0.2)
        {
          tempSprite.drawSmoothRoundRect(x - scaledSize / 2 - 6, y + yOffset - 5, 12, 12, scaledSize + 11, scaledSize + 10, TFT_WHITE, tempSprite.color565(85, 85, 85));
        }
      }
    }
  }
  tempSprite.fillSmoothCircle(25, 67, 10, tempSprite.color565(40, 40, 40), tempSprite.color565(15, 15, 15));
  drawIcon(tempSprite, arrow_left, 15, 58, 20, 20);
  tempSprite.fillSmoothCircle(215, 67, 10, tempSprite.color565(40, 40, 40), tempSprite.color565(15, 15, 15));
  drawIcon(tempSprite, arrow_right, 206, 58, 20, 20);
  tempSprite.pushSprite(0, 0);
}