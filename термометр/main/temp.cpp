#include "system.h"
#include "config.h"
#include <cmath>
#include "main/temp.h"
#include "settings/system/language/sys_language.h"
#include "fonts/WixDisplay_Bold16.h"
#include "fonts/WixDisplay_Bold17.h"
#include "fonts/WixDisplay_Bold18.h"
#include "fonts/WixDisplay_Bold19.h"
#include "fonts/LexendExa_ExtraBold53.h"
#include <esp_random.h> // Добавляем библиотеку ESP32 для аппаратного RNG

// ----------------------- Определение цветов температур (М) -----------------------
uint16_t FROST_COLOR = TFT_eSPI().color565(153, 0, 255); // Фиолетовый
uint16_t CHILLY_COLOR = TFT_eSPI().color565(0, 0, 255);  // Синий
uint16_t NORMAL_COLOR = TFT_eSPI().color565(0, 112, 0);  // Зелёный
uint16_t FEVER_COLOR = TFT_eSPI().color565(255, 85, 0);  // Оранжевый
uint16_t DANGER_COLOR = TFT_eSPI().color565(255, 0, 0);  // Красный
uint16_t INF_COLOR = TFT_eSPI().color565(102, 102, 102); // Серый
// ---------------------------------------------------------------------------------

float currTemp = 36.6;
static uint16_t targetColor = NORMAL_COLOR;
static uint16_t currentColor = NORMAL_COLOR;
static unsigned long lastUpdate = 0;
static unsigned long t0 = 0;
static bool isAnimate = false;
static float animProgres = 0.0f;
static String currLabelText = "NORMAL";
static uint16_t currLabelColor = NORMAL_COLOR;
static int currRectW = 0;
static int labelY = 6;
static String targLabelText = currLabelText;
static uint16_t targLabelColor = currLabelColor;
static int targRectW = 0;

struct Note
{
  String line1EN;
  String line2EN;
  String line1RU;
  String line2RU;
};

static bool isNoteAnimating = false;
static float noteAnimProgress = 0.0f;
static unsigned long notet0 = 0;
static Note currNote = {"Healthy temperature", "reading. No concerns.", "Здоровые показатели", "температуры. Всё хорошо."};
static Note targNote = {"Healthy temperature", "reading. No concerns.", "Здоровые показатели", "температуры. Всё хорошо."};

// ---------------------- Структура для диапазонов температур ----------------------
struct TempRange
{                     //
  float minTemp;      // Мин температура диапазона
  float maxTemp;      // Макс температура диапазона
  bool minInclusive;  // Вкл ли минимальная граница
  bool maxInclusive;  // Вкл ли максимальная граница
  uint16_t color;     // Цвет фона и метки
  String labelTextEN; // Текст метки (англ)
  String labelTextRU; // Текст метки (рус)
  Note noteTexts[4];  // Массив из 4 вариантов текста NOTE
}; //
//----------------------------------------------------------------------------------

static TempRange ranges[] = {
    {-INFINITY, 35.0, false, false, FROST_COLOR, "FROST", "МОРОЗ", {{"CRITICAL: Severe life-", "threatening hypothermia!", "КРИТИЧНО: Тяжёлая", "гипотермия угрожает!"}, {"DANGER: Extreme cold", "exposure. Warm up fast!", "ОПАСНО: Критическое", "переохлаждение тела!"}, {"ALERT: Life-threatening", "temperature detected.", "ТРЕВОГА: Температура", "угрожает жизни!"}, {"EMERGENCY: High risk of", "frostbite and death.", "ЭКСТРЕННО: Высокий", "риск обморожения!"}}},
    {35.0, 35.5, true, false, CHILLY_COLOR, "CHILLY", "ХОЛОДНО", {{"Mild hypothermia found.", "Warm up body gradually.", "Лёгкая гипотермия.", "Согревайтесь плавно."}, {"Temperature below the", "normal range. Stay warm.", "Температура понижена.", "Сохраняйте тепло."}, {"Body temperature feels", "cool. Avoid cold areas.", "Организм охлаждён.", "Избегайте холода."}, {"Lower than normal temp.", "Need to warm up slowly.", "Ниже нормы. Нужно", "согреваться медленно."}}},
    {35.5, 37.0, true, true, NORMAL_COLOR, "NORMAL", "НОРМА", {{"Normal body temperature", "range. Everything fine.", "Нормальная температура", "тела. Всё в порядке."}, {"Within healthy normal", "range. Ideal: 36.6°C.", "В здоровых пределах.", "Норма: 36.6°C."}, {"Healthy temperature", "reading. No concerns.", "Здоровые показатели.", "Беспокойств нет."}, {"Optimal temperature for", "body. Continue normally.", "Оптимальная для", "организма. Всё хорошо."}}},
    {37.0, 39.0, false, true, FEVER_COLOR, "FEVER", "ЖАР", {{"Mild fever detected in", "body. Rest and hydrate.", "Субфебрильная", "лихорадка. Отдых, питьё."}, {"Temperature elevated", "above normal. Monitor.", "Повышена выше нормы.", "Контролируйте состояние."}, {"Body running mild fever.", "Give yourself good rest.", "Лёгкий жар. Нужен", "покой и наблюдение."}, {"Degrees above normal", "range. Consider meds.", "Выше нормы. Возможны", "жаропонижающие."}}},
    {39.0, 42.1, false, true, DANGER_COLOR, "DANGER", "ОПАСНО", {{"HIGH FEVER ALERT:", "Need medical help now!", "ВЫСОКАЯ ЛИХОРАДКА:", "Срочно нужен врач!"}, {"CRITICAL: Dangerous", "fever. Call doctor fast.", "КРИТИЧНО: Опасный", "жар. Вызывайте врача!"}, {"SEVERE: Very high body", "temperature. Doctor now.", "ТЯЖЕЛО: Очень высокая", "температура. К врачу!"}, {"ALERT: Health at serious", "risk. Emergency care.", "ТРЕВОГА: Серьёзная", "угроза здоровью!"}}},
    {42.1, INFINITY, false, false, INF_COLOR, "ERROR", "ОШИБКА", {{"Measurement error found:", "Non-human temperature.", "Ошибка измерения:", "Нечеловеческие значения."}, {"Invalid sensor readings.", "Check device calibration.", "Неверные показания", "датчика. Проверьте."}, {"Outside human biological", "range. Switch modes.", "Вне биологических", "пределов человека."}, {"System malfunction error.", "Recalibrate the device.", "Сбой системы.", "Перекалибруйте прибор."}}}};
static const int numRanges = sizeof(ranges) / sizeof(ranges[0]);
static int prevRangeIdx = -1;
static bool sensorError = false;

static bool readTemperature(float &temp)
{

  if (Serial.available() > 0)
  {
    String tempStr = Serial.readStringUntil('\n');
    if (tempStr.length() > 0)
    {
      float newTemp = tempStr.toFloat();
      if ((newTemp != 0.0f || tempStr == "0") && newTemp > -50.0f && newTemp < 100.0f)
      {
        temp = newTemp;
        return true;
      }
    }
  }
  return false;
}

static int getRangeIdx(float temp)
{
  for (int i = 0; i < numRanges; i++)
  {
    TempRange range = ranges[i];
    bool lower_ok = range.minInclusive ? (temp >= range.minTemp) : (temp > range.minTemp);
    bool upper_ok = range.maxInclusive ? (temp <= range.maxTemp) : (temp < range.maxTemp);
    if (lower_ok && upper_ok)
    {
      return i;
    }
  }
  return -1;
}

static float intensity_lut[256];
static bool lut_populated = false;
static void populateIntensityLut()
{
  for (int i = 0; i < 256; i++)
  {
    float ratio = (float)i / 255.0f;
    intensity_lut[i] = 1.0f - sqrt(ratio);
  }
}

static float easeInQuad(float t)
{
  return t * t;
}
static float easeOutQuad(float t)
{
  return 1 - (1 - t) * (1 - t);
}
static float easeOutBack(float t)
{
  const float c1 = 1.70158;
  const float c3 = c1 + 1;
  return 1 + c3 * pow(t - 1, 3) + c1 * pow(t - 1, 2);
}

static void labelGo(String newText, uint16_t newColor, int newWidth)
{
  if (!isAnimate)
  {
    isAnimate = true;
    currLabelText = targLabelText;
    currLabelColor = targLabelColor;
    currRectW = targRectW;
    t0 = millis();
    animProgres = 0.0f;
  }
  targLabelText = newText;
  targLabelColor = newColor;
  targRectW = newWidth;
}

static void noteGo(Note newNote)
{
  if (!isNoteAnimating)
  {
    isNoteAnimating = true;
    currNote = targNote;
    notet0 = millis();
    noteAnimProgress = 0.0f;
  }
  targNote = newNote;
}

// Функция для получения случайного числа из аппаратного RNG
static uint32_t getHardwareRandomNumber(uint32_t min, uint32_t max)
{
  uint32_t randomValue = esp_random(); // Используем аппаратный RNG ESP32
  return min + (randomValue % (max - min));
}

// Структура для отслеживания показанных текстов
static struct
{
  bool used[6][4];    // Отслеживание использованных текстов для каждого диапазона
  int lastIndices[6]; // Последний показанный индекс для каждого диапазона
} textHistory;

// Инициализация истории показа текстов
static void initTextHistory()
{
  for (int i = 0; i < 6; i++)
  {
    textHistory.lastIndices[i] = -1;
    for (int j = 0; j < 4; j++)
    {
      textHistory.used[i][j] = false;
    }
  }
}

// Выбор следующего текста, гарантирующий отсутствие повторений
static int getNextTextIndex(int rangeIndex)
{
  // Проверка все ли тексты были показаны
  bool allUsed = true;
  for (int i = 0; i < 4; i++)
  {
    if (!textHistory.used[rangeIndex][i])
    {
      allUsed = false;
      break;
    }
  }

  // Если все тексты были показаны, сбрасываем историю для этого диапазона
  if (allUsed)
  {
    for (int i = 0; i < 4; i++)
    {
      textHistory.used[rangeIndex][i] = false;
    }
    // Оставляем последний показанный как использованный
    if (textHistory.lastIndices[rangeIndex] >= 0)
    {
      textHistory.used[rangeIndex][textHistory.lastIndices[rangeIndex]] = true;
    }
  }

  // Выбираем случайный индекс из неиспользованных
  int attempts = 0;
  int newIndex;
  do
  {
    newIndex = getHardwareRandomNumber(0, 4);
    attempts++;
    // Если слишком много попыток, выбираем первый неиспользованный
    if (attempts > 10)
    {
      for (int i = 0; i < 4; i++)
      {
        if (!textHistory.used[rangeIndex][i])
        {
          newIndex = i;
          break;
        }
      }
      break;
    }
  } while (textHistory.used[rangeIndex][newIndex]);

  // Отмечаем текст как использованный
  textHistory.used[rangeIndex][newIndex] = true;
  textHistory.lastIndices[rangeIndex] = newIndex;

  return newIndex;
}

void showTemp(TFT_eSprite &sprite)
{
  // Инициализация истории показа текстов и генератора случайных чисел
  static bool initialized = false;
  if (!initialized)
  {
    // Используем несколько источников энтропии для инициализации
    uint32_t seed = analogRead(A0);
    seed = (seed << 8) | analogRead(A3);
    seed = (seed << 8) | (millis() & 0xFF);
    seed = (seed << 8) | (micros() & 0xFF);
    srand(seed);
    initTextHistory();
    initialized = true;
  }

  float newTemp;
  bool readSuccess = readTemperature(newTemp);

  if (readSuccess)
  {
    sensorError = false;
    if (newTemp != currTemp)
    {
      currTemp = newTemp;
      int currRangeIdx = getRangeIdx(currTemp);

      if (currRangeIdx != prevRangeIdx)
      {
        unsigned long now = millis();
        float progress = min(1.0f, (float)(now - lastUpdate) / 750);
        currentColor = interpolateColor(currentColor, targetColor, progress);
        TempRange newRange = ranges[currRangeIdx];
        targetColor = newRange.color;
        lastUpdate = millis();

        String newLabelText;
        String currentLang = getCurrlanguage();
        if (currentLang == "Русский")
        {
          newLabelText = newRange.labelTextRU;
        }
        else
        {
          newLabelText = newRange.labelTextEN;
        }

        uint16_t newLabelColor = newRange.color;
        sprite.loadFont(WixDisplay_Bold18);
        int newRectW = sprite.textWidth(newLabelText) + 6 * 2;

        labelGo(newLabelText, newLabelColor, newRectW);

        // Улучшенный выбор случайного текста с использованием аппаратного RNG
        int randomIndex = getNextTextIndex(currRangeIdx);
        Note newNote = newRange.noteTexts[randomIndex];
        noteGo(newNote);

        prevRangeIdx = currRangeIdx;
      }
    }
  }
  else
  {
    sensorError = true;
  }

  if (!lut_populated)
  {
    populateIntensityLut();
    lut_populated = true;
  }

  unsigned long now = millis();
  float progress = min(1.0f, (float)(now - lastUpdate) / 750);
  uint16_t interpolatedColor = interpolateColor(currentColor, targetColor, progress);
  static const float centerX = 120.0;
  static const float centerY = 135.0;
  static const float max_distance_squared = (centerX * centerX) + (centerY * centerY);
  uint8_t baseR = (interpolatedColor >> 11) & 0x1F;
  uint8_t baseG = (interpolatedColor >> 5) & 0x3F;
  uint8_t baseB = interpolatedColor & 0x1F;
  const float ratio_multiplier = 255.0f / max_distance_squared;
  sprite.fillSprite(TFT_BLACK);
  for (int y = 0; y < SCREEN_HEIGHT; y++)
  {
    float dy = y - centerY;
    float dy_squared = dy * dy;
    for (int x = centerX; x < SCREEN_WIDTH; x++)
    {
      float dx = x - centerX;
      float distance_squared = dx * dx + dy_squared;
      int index = (int)(distance_squared * ratio_multiplier);
      if (index > 255)
        index = 255;
      float intensity = intensity_lut[index];
      uint8_t newR = (uint8_t)(baseR * intensity);
      uint8_t newG = (uint8_t)(baseG * intensity);
      uint8_t newB = (uint8_t)(baseB * intensity);
      uint16_t pixelColor = ((newR & 0x1F) << 11) | ((newG & 0x3F) << 5) | (newB & 0x1F);
      sprite.drawPixel(x, y, pixelColor);
      int mirroredX = (2 * centerX) - x - 1;
      if (mirroredX >= 0)
      {
        sprite.drawPixel(mirroredX, y, pixelColor);
      }
    }
  }

  drawStatusBar(sprite);

  if (isAnimate)
  {
    animProgres = min(1.0f, (float)(now - t0) / 750);
    if (animProgres < 0.5f)
    {
      float t = easeInQuad(animProgres / 0.5f);
      labelY = 6 + (-30 - 6) * t;
    }
    else
    {
      float t = easeOutBack((animProgres - 0.5f) / 0.5f);
      labelY = -30 + (6 - -30) * t;
      if (animProgres >= 0.5f && currLabelText != targLabelText)
      {
        currLabelText = targLabelText;
        currLabelColor = targLabelColor;
        currRectW = targRectW;
      }
    }
    if (animProgres >= 1.0f)
    {
      isAnimate = false;
      labelY = 6;
    }
  }

  int noteY = 93;
  int noteTargetY = 93;
  int noteHiddenY = 150;
  if (isNoteAnimating)
  {
    noteAnimProgress = min(1.0f, (float)(now - notet0) / 750);
    if (noteAnimProgress < 0.5f)
    {
      float t = easeInQuad(noteAnimProgress / 0.5f);
      noteY = noteY + (noteHiddenY - noteY) * t;
    }
    else
    {
      float t = easeOutBack((noteAnimProgress - 0.5f) / 0.5f);
      noteY = noteHiddenY + (noteTargetY - noteHiddenY) * t;
      if (noteAnimProgress >= 0.5f && currNote.line1EN != targNote.line1EN)
      {
        currNote = targNote;
      }
    }
    if (noteAnimProgress >= 1.0f)
    {
      isNoteAnimating = false;
      noteY = noteTargetY;
    }
  }

  sprite.loadFont(WixDisplay_Bold16);
  sprite.setTextColor(TFT_WHITE);
  int rectWidth = sprite.textWidth(currLabelText) + 12;
  sprite.fillSmoothRoundRect((SCREEN_WIDTH - rectWidth) / 2, labelY - 1, rectWidth, 20, 6, currLabelColor, TFT_TRANSPARENT);
  int textX = (SCREEN_WIDTH - rectWidth) / 2 + 6;
  int textY = labelY + (20 - sprite.fontHeight()) / 2;
  sprite.drawString(currLabelText, textX, textY);

  String currentLang = getCurrlanguage();
  if (currentLang == "Русский")
  {
    sprite.drawString(currNote.line1RU, 7, noteY);
    if (currNote.line2RU != "")
    {
      sprite.drawString(currNote.line2RU, 7, noteY + 19);
    }
  }
  else
  {
    sprite.drawString(currNote.line1EN, 7, noteY);
    if (currNote.line2EN != "")
    {
      sprite.drawString(currNote.line2EN, 7, noteY + 19);
    }
  }

  sprite.loadFont(LexendExa_ExtraBold53);
  sprite.setTextColor(TFT_WHITE);

  String valueStr;
  if (sensorError)
  {
    valueStr = "--.-";
  }
  else
  {
    valueStr = String(currTemp, 1);
  }

  String unitStr = "°C";
  int valueWidth = sprite.textWidth(valueStr);
  int valueX = 155 - valueWidth - 5;
  sprite.drawString(valueStr, valueX, 40);
  sprite.drawString(unitStr, 155, 40);

  if (progress >= 1.0)
  {
    currentColor = targetColor;
  }
}