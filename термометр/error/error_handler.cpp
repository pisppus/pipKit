#include "config.h"
#include "system.h"
#include "error/error_handler.h"
#include "icons/error_icon.h"
#include "fonts/WixDisplay_Bold16.h"

struct ErrorMessage
{
  const char *message;
  const char *code;
};

// -------------------------------- Сообщения об ошибках --------------------------------
const ErrorMessage ERROR_MESSAGES[] = {
    {"Unknown error.", "0xUNKNOWN"},          // Неизвестная ошибка
    {"LittleFS mount failed!", "0xLFS_MNT"},  // Ошибка монтирования файловой системы
    {"Dir creation failed!", "0xLFS_MKDIR"},  // Ошибка создания директории
    {"File creation failed!", "0xLFS_FILE"},  // Ошибка создания файла
    {"Sprite creation failed!", "0xSPR_MEM"}, // Ошибка создания спрайта
    {"No profiles found!", "0xNOPROF"},       // Нет профилей
    {"Too many profiles!", "0xBADPROF"}       // Слишком много профилей
};
//---------------------------------------------------------------------------------------

static ErrorInfo errorList[MAX_ERRORS];
int errorCount = 0;
int currErrorId = 0;
static TFT_eSprite *errorSprite = nullptr;
bool isTransition = false;
static unsigned long animStart = 0;
static int nextErrorId = 0;
static unsigned long lastScrollTime = 0;

void errorCall(ErrorCode code, ErrorPriority priority, String detail)
{
  for (int i = 0; i < errorCount; i++)
  {
    if (errorList[i].code == code && errorList[i].active && errorList[i].detail == detail)
    {
      errorList[i].priority = priority;
      return;
    }
  }

  if (errorCount < MAX_ERRORS)
  {
    int insertPos = errorCount;
    for (int i = 0; i < errorCount; i++)
    {
      if (priority < errorList[i].priority)
      {
        insertPos = i;
        break;
      }
    }

    if (insertPos < errorCount)
    {
      for (int i = errorCount; i > insertPos; i--)
      {
        errorList[i] = errorList[i - 1];
      }
    }

    errorList[insertPos].code = code;
    errorList[insertPos].priority = priority;
    errorList[insertPos].detail = detail;
    errorList[insertPos].active = true;
    errorList[insertPos].scrollPos = 0;

    if (errorSprite != nullptr)
    {
      errorSprite->loadFont(WixDisplay_Bold16);
      errorList[insertPos].textWidth = errorSprite->textWidth(detail);
    }
    else
    {
      errorList[insertPos].textWidth = 0;
    }

    errorCount++;
    currErrorId = 0;
  }
}

void initErrorHandler(TFT_eSPI *tft)
{
  if (errorSprite == nullptr)
  {
    errorSprite = new TFT_eSprite(tft);
    errorSprite->createSprite(240, 60);
  }
}

void switchErrorAnim()
{
  if (isTransition || errorCount <= 1)
    return;

  isTransition = true;
  animStart = millis();
  nextErrorId = (currErrorId + 1) % errorCount;
}

static void drawErrorContent(TFT_eSprite &s, ErrorCode errorCode, const String &errorDetail, uint8_t opacity, int scrollX)
{
  s.fillSprite(TFT_BLACK);
  const uint16_t grayColor = s.color565(140 * opacity / 255, 140 * opacity / 255, 140 * opacity / 255);
  const uint16_t orangeColor = s.color565(252 * opacity / 255, 123 * opacity / 255, 3 * opacity / 255);
  const ErrorMessage &errorInfo = (errorCode >= 0 && errorCode <= TOO_MANY_PROFILES_ERROR)
                                      ? ERROR_MESSAGES[errorCode]
                                      : ERROR_MESSAGES[0];

  s.loadFont(WixDisplay_Bold16);
  s.setTextColor(grayColor);
  int centerX = (s.width() - s.textWidth(errorInfo.message)) / 2;
  s.drawString(errorInfo.message, centerX, 5);

  if (errorDetail.length() > 0)
  {
    int detailWidth = s.textWidth(errorDetail);

    if (detailWidth > s.width() - 40)
    {
      s.setViewport((s.width() - 180) / 2, 21, 180, 15);
      const int totalScrollWidth = detailWidth + 10 * 2;
      int effScrollX = scrollX % totalScrollWidth;
      if (effScrollX > 0)
        effScrollX -= totalScrollWidth;
      s.drawString(errorDetail, effScrollX + 10, 0);

      if (effScrollX + detailWidth + 10 < 180)
      {
        s.drawString(errorDetail, effScrollX + detailWidth + 10 * 2, 0);
      }

      s.resetViewport();
      s.fillRect((s.width() - 180) / 2, 21, 15, 15, TFT_BLACK);
      s.fillRect((s.width() - 180) / 2 + 180 - 15, 21, 15, 15, TFT_BLACK);
    }
    else
    {
      s.drawString(errorDetail, (s.width() - detailWidth) / 2, 21);
    }
  }

  const char *codeLabel = "Code: ";
  int16_t codeStartX = (s.width() - (s.textWidth(codeLabel) + s.textWidth(errorInfo.code))) / 2;
  s.setTextColor(grayColor);
  s.drawString(codeLabel, codeStartX, 39);
  s.setTextColor(orangeColor);
  s.drawString(errorInfo.code, codeStartX + s.textWidth(codeLabel), 39);
}

static float easeInOutQuint(float t)
{
  return t < 0.5 ? 16 * t * t * t * t * t : 1 + 16 * (--t) * t * t * t * t;
}

void ErrorHandler(TFT_eSprite &sprite)
{
  if (errorCount == 0)
    return;

  ErrorCode errorCode = errorList[currErrorId].code;
  String errorDetail = errorList[currErrorId].detail;

  if (errorDetail != "" && !isTransition)
  {
    if (errorList[currErrorId].textWidth == 0 && errorSprite != nullptr)
    {
      errorSprite->loadFont(WixDisplay_Bold16);
      errorList[currErrorId].textWidth = errorSprite->textWidth(errorDetail);
    }

    unsigned long now = millis();
    if (now - lastScrollTime > 20)
    {
      errorList[currErrorId].scrollPos -= 2;
      if (errorList[currErrorId].scrollPos < -10000)
      {
        errorList[currErrorId].scrollPos = 0;
      }
      lastScrollTime = now;
    }
  }

  sprite.fillSprite(TFT_BLACK);
  drawIcon(sprite, error, (SCREEN_WIDTH - 47) / 2, 6, 47, 47);
  sprite.loadFont(WixDisplay_Bold16);
  sprite.setTextColor(TFT_WHITE);
  sprite.setTextDatum(MC_DATUM);
  sprite.drawString("ERROR", SCREEN_WIDTH / 2, 66);

  if (isTransition)
  {
    unsigned long elapsed = millis() - animStart;
    if (elapsed >= 700)
    {
      isTransition = false;
      currErrorId = nextErrorId;
    }

    float progress = easeInOutQuint((float)elapsed / 700);
    uint8_t currentOpacity = (uint8_t)(255.0f * (1.0f - progress));
    uint8_t nextOpacity = (uint8_t)(255.0f * progress);
    float fullWidth = SCREEN_WIDTH * 1.1f;
    float currentXf = -progress * fullWidth;
    float nextXf = SCREEN_WIDTH - progress * SCREEN_WIDTH;
    int currentX = (int)(currentXf + 0.5f);
    int nextX = (int)(nextXf + 0.5f);

    drawErrorContent(*errorSprite, errorList[currErrorId].code,
                     errorList[currErrorId].detail, currentOpacity,
                     errorList[currErrorId].scrollPos);
    errorSprite->pushToSprite(&sprite, currentX, 75, TFT_BLACK);

    drawErrorContent(*errorSprite, errorList[nextErrorId].code,
                     errorList[nextErrorId].detail, nextOpacity,
                     errorList[nextErrorId].scrollPos);
    errorSprite->pushToSprite(&sprite, nextX, 75, TFT_BLACK);
  }
  else
  {
    drawErrorContent(*errorSprite, errorList[currErrorId].code,
                     errorList[currErrorId].detail, 255,
                     errorList[currErrorId].scrollPos);
    errorSprite->pushToSprite(&sprite, 0, 75, TFT_BLACK);
  }

  if (errorCount > 1)
  {
    sprite.loadFont(WixDisplay_Bold16);
    String errorCounter = String(currErrorId + 1) + "/" + String(errorCount);
    int rectWidth = sprite.textWidth(errorCounter) + 6 * 2;
    sprite.fillSmoothRoundRect(SCREEN_WIDTH - rectWidth - 10, 10, rectWidth, 20, 7, sprite.color565(30, 30, 30));
    sprite.drawSmoothRoundRect(SCREEN_WIDTH - rectWidth - 10, 10, 7, 7, rectWidth, 20, sprite.color565(50, 50, 50), sprite.color565(10, 10, 10));
    sprite.setTextColor(TFT_WHITE);
    sprite.setTextDatum(MC_DATUM);
    sprite.drawString(errorCounter, SCREEN_WIDTH - rectWidth - 10 + rectWidth / 2, 22);
    sprite.setTextDatum(TL_DATUM);
  }

  sprite.unloadFont();
}