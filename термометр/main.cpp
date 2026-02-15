#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <cmath>
#include "system.h"
#include "config.h"
#include "error/error_handler.h"
#include "main/logo.h"
#include "main/select_profile.h"
#include "main/temp.h"
#include "main/graph.h"
#include "settings/global/settings.h"
#include "settings/global/display_settings.h"
#include "settings/global/system_settings.h"
#include "settings/global/temp_settings.h"
#include "settings/display/brightness/sys_brightness.h"
#include "settings/display/brightness/ui_brightness.h"
#include "settings/display/standby/sys_standby.h"
#include "settings/display/standby/ui_standby.h"
#include "settings/system/language/sys_language.h"
#include "settings/system/language/ui_language.h"
#include "settings/system/about/ui_about.h"
#include "settings/system/developer/ui_developer.h"
#include "settings/system/reset/ui_reset.h"
#include "settings/system/time/ui_time.h"
#include "settings/temperature/age/ui_age.h"
#include "settings/temperature/units/ui_units.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite currSprite = TFT_eSprite(&tft);
TFT_eSprite nextSprite = TFT_eSprite(&tft);
TFT_eSprite tempSprite = TFT_eSprite(&tft);
TFT_eSprite sprite = TFT_eSprite(&tft);

// ---------------------- Состояния системы ----------------------
int currState = LOGO;
int prevState = LOGO;
int selectProfile = 0;

// --------------------------- Анимация --------------------------
bool isAnimating = false;
int animationType = NONE;
int animationProgress = 0;
int animationDirection = 0;
unsigned long lastAnimationTime = 0;
bool isTransitionComplete = true;

// --------------------- Анимация логотипа ----------------------
bool isAnimLogo = false;
unsigned long InlogoAnim = 0;
uint8_t logoTargetBrightness = 0;

// --------------------- Обработка кнопок ----------------------
unsigned long btnLowPressTime = 0;
bool btnLowLongPress = false;
unsigned long btnUpPressTime = 0;
bool btnUpLongPress = false;
static bool up_button_locked = false;
static bool low_button_locked = false;

void drawScreen(int state, TFT_eSprite &sprite);

// Отладочная функция для вывода структуры файловой системы
void listAllFiles(const char *dirname, String indent = "")
{
  File root = LittleFS.open(dirname);
  if (!root)
    return void(Serial.println(indent + "Failed to open directory"));
  if (!root.isDirectory())
    return void(Serial.println(indent + "Not a directory"));

  File file;
  while (file = root.openNextFile())
  {
    String name = file.name();
    int slash = name.lastIndexOf('/');
    String nameOnly = (slash != -1) ? name.substring(slash + 1) : name;

    if (file.isDirectory())
    {
      Serial.println(indent + "📁 " + nameOnly + "/");
      listAllFiles(file.path(), indent + "  ");
    }
    else
    {
      Serial.printf("%s📄 %s (%u bytes)\n", indent.c_str(), nameOnly.c_str(), file.size());

      if ((name.endsWith(".json") || name.endsWith(".txt")) && file.size() > 0 && file.size() < 4096)
      {
        Serial.println(indent + "  ┌─ Содержимое ──────────────────────");
        while (file.available())
          Serial.println(indent + "  │ " + file.readStringUntil('\n'));
        Serial.println(indent + "  └───────────────────────────────────");
      }
    }
    file.close();
  }
  root.close();
}

void initProfileSettings(int profileId)
{
  String path = "/storage/profiles/" + String(profileNames[profileId]) + "/settings.json";
  File f = LittleFS.open(path, "w");
  if (!f)
    return errorCall(LFS_FILE_ERROR, ERR_HIGH, path);

  JsonDocument doc;
  doc["brightness"] = DEF_BRIGHTNESS;
  doc["standby_time"] = DEF_STANDBY_TIME;
  doc["language"] = DEF_LANGUAGE;
  doc["unit_temp"] = DEF_UNIT_TEMP;

  if (serializeJson(doc, f) == 0)
    errorCall(LFS_FILE_ERROR, ERR_HIGH, "JSON write to " + path);
  f.close();
}

void initializeStorage()
{
  if (!LittleFS.begin(true))
    return errorCall(LFS_MOUNT_ERROR, ERR_CRITICAL, "LittleFS");

  for (const char *dir : {"/storage", "/storage/system", "/storage/profiles"})
  {
    if (!LittleFS.exists(dir) && !LittleFS.mkdir(dir))
      return errorCall(LFS_MKDIR_ERROR, ERR_HIGH, dir);
  }

  JsonDocument doc;
  String path = "/storage/system/sysinfo.json";
  File f = LittleFS.open(path, "r");

  doc["boot_count"] = (f && deserializeJson(doc, f) == DeserializationError::Ok) ? doc["boot_count"].as<int>() + 1 : 1;
  if (f)
    f.close();

  doc["firmware_version"] = FW_VERSION;
  f = LittleFS.open(path, "w");
  if (!f || serializeJson(doc, f) == 0)
    errorCall(LFS_FILE_ERROR, ERR_HIGH, path);
  if (f)
    f.close();

  for (int i = 0; i < profileNum; i++)
  {
    String pPath = "/storage/profiles/" + String(profileNames[i]);
    if (!LittleFS.exists(pPath) && !LittleFS.mkdir(pPath))
    {
      errorCall(LFS_MKDIR_ERROR, ERR_HIGH, pPath);
      continue;
    }

    String sPath = pPath + "/settings.json";
    f = LittleFS.open(sPath, "r");
    bool needsInit = !f || f.size() == 0;
    if (f)
      f.close();
    if (needsInit)
      initProfileSettings(i);

    String tPath = pPath + "/temp_data.dat";
    if (!LittleFS.exists(tPath))
    {
      f = LittleFS.open(tPath, "w");
      if (!f)
        errorCall(LFS_FILE_ERROR, ERR_HIGH, tPath);
      else
        f.close();
    }
  }

  // Отладочная информация
  Serial.println("\n--- File System Structure ---");
  listAllFiles("/", "");
  Serial.println("---------------------------\n");
}

inline float fastEase(float t)
{
  return t < 0.5f ? 2 * t * t : 2 * (t -= 0.5f) * (1 - t) + 0.5f;
}

void drawScaledSprite(TFT_eSprite &dest, TFT_eSprite &src, int w, int h, int x, int y)
{
  int xr = (SCREEN_WIDTH << 8) / w, yr = (SCREEN_HEIGHT << 8) / h;
  for (int i = 0, sy = 0; i < h; i++, sy += yr)
  {
    for (int j = 0, sx = 0; j < w; j++, sx += xr)
    {
      dest.drawPixel(x + j, y + i, src.readPixel(sx >> 8, sy >> 8));
    }
  }
}

void updateAnimation()
{
  float p = fastEase((float)animationProgress / 30);
  tempSprite.fillSprite(TFT_BLACK);

  if (animationType != ROLLER)
  {
    int dir = (animationType == SLIDE_UP) ? -1 : 1;
    float scale = 1.0f - p * 0.15f;
    int w = round(SCREEN_WIDTH * scale), h = round(SCREEN_HEIGHT * scale);

    drawScaledSprite(tempSprite, currSprite, w, h, (SCREEN_WIDTH - w) / 2, round(p * 45 * dir));

    int slideY = round(p * SCREEN_HEIGHT);
    tempSprite.pushImage(0, (dir == -1) ? SCREEN_HEIGHT - slideY : slideY - SCREEN_HEIGHT,
                         SCREEN_WIDTH, SCREEN_HEIGHT, (uint16_t *)nextSprite.getPointer());
  }
  else
  {
    updateRollerAnim(tempSprite, p, animationDirection);
  }

  tempSprite.pushSprite(0, 0);

  if (++animationProgress > 30)
  {
    isAnimating = false;
    isTransitionComplete = true;
    if (animationType == ROLLER)
    {
      drawProfile(currSprite);
      currSprite.pushSprite(0, 0);
    }
    else
    {
      nextSprite.pushSprite(0, 0);
      drawScreen(currState, currSprite);
    }
  }
}

void drawScreen(int state, TFT_eSprite &sprite)
{
  sprite.fillSprite(TFT_BLACK);
  static const struct
  {
    int state;
    void (*func)(TFT_eSprite &);
  } drawTable[] = {
      {PROFILE, drawProfile},
      {MAIN, showTemp},
      {CHART, showGraph},
      {SETTINGS, drawSettings},
      {DISPLAY_SETTINGS, drawDisplaySettings},
      {BRIGHTNESS_SETTINGS, drawBrightness},
      {STANDBY_SETTINGS, drawStandby},
      {TEMPERATURE_SETTINGS, drawTempSettings},
      {AGE_SETTINGS, drawAge},
      {UNITS_SETTINGS, drawUnits},
      {LANGUAGE_SETTINGS, drawLanguage},
      {TIME_SETTINGS, drawTime},
      {SYSTEM_SETTINGS, drawSysSettings},
      {ABOUT_SYSTEM, drawAbout},
      {DEVELOPER_INFO, drawDeveloper},
      {RESET_INFO, drawResetInfo},
      {RESET_FINAL_CONFIRM, drawResetConfirm}};

  for (const auto &entry : drawTable)
    if (entry.state == state)
    {
      entry.func(sprite);
      return;
    }

  if (state == ERROR)
    ErrorHandler(sprite);
}

void changeState(int newState, int animType)
{
  if (isAnimating)
    return;

  struct StateInfo
  {
    int state;
    void (*enterFunc)();
    void (*exitFunc)();
  };

  static const StateInfo stateTable[] = {
      {SETTINGS, enterSettings, nullptr},
      {DISPLAY_SETTINGS, enterDisplaySettings, nullptr},
      {BRIGHTNESS_SETTINGS, enterBrightness, exitBrightness},
      {STANDBY_SETTINGS, enterStandby, exitStandby},
      {TEMPERATURE_SETTINGS, enterTempSettings, nullptr},
      {AGE_SETTINGS, enterAge, exitAge},
      {UNITS_SETTINGS, enterUnits, exitUnits},
      {LANGUAGE_SETTINGS, enterLanguage, nullptr},
      {TIME_SETTINGS, enterTime, exitTime},
      {SYSTEM_SETTINGS, enterSysSettings, nullptr},
      {RESET_INFO, enterResetInfo, nullptr},
      {RESET_FINAL_CONFIRM, enterResetConfirm, nullptr}};

  auto findState = [](int state) -> const StateInfo *
  {
    for (const auto &entry : stateTable)
      if (entry.state == state)
        return &entry;
    return nullptr;
  };

  const StateInfo *currInfo = findState(currState);
  if (currInfo && currInfo->exitFunc)
  {
    currInfo->exitFunc();
  }

  prevState = currState;
  currState = newState;

  const StateInfo *newInfo = findState(newState);
  if (newInfo && newInfo->enterFunc)
  {
    newInfo->enterFunc();
  }

  if (animType != NONE)
  {
    isAnimating = true;
    isTransitionComplete = false;
    animationType = animType;
    animationProgress = 0;
    lastAnimationTime = millis();
    drawScreen(prevState, currSprite);
    drawScreen(newState, nextSprite);
  }
  else
  {
    drawScreen(newState, currSprite);
    currSprite.pushSprite(0, 0);
    isTransitionComplete = true;
  }
}

void updateDisplay()
{
  updateBrightnessAnim();

  if (!isAnimating && !isAnimLogo && !isAnimBrightness)
    checkStandby();

  if (isAnimLogo)
  {
    updateLogo();
    return;
  }

  if (isAnimating)
  {
    updateAnimation();
    return;
  }

  if (currState == STANDBY_SETTINGS && isStandbyAnim())
  {
    updStandbyAnim(sprite);
    return;
  }

  if (currState == LANGUAGE_SETTINGS && isLanguageAnim())
  {
    updLanguageAnim(sprite);
    return;
  }

  if (currState == AGE_SETTINGS && isAgeAnim())
  {
    updAgeAnim(sprite);
    return;
  }

  if (currState == UNITS_SETTINGS && isUnitsAnim())
  {
    updUnitsAnim(sprite);
    return;
  }

  struct UpdateEntry
  {
    int state;
    void (*updateFunc)(TFT_eSprite &);
    bool needPushSprite;
  };

  static const UpdateEntry updateTable[] = {
      {MAIN, showTemp, true},
      {CHART, showGraph, false},
      {SETTINGS, updateSettings, false},
      {DISPLAY_SETTINGS, updateDisplaySettings, false},
      {BRIGHTNESS_SETTINGS, updateBrightness, false},
      {STANDBY_SETTINGS, updateStandby, false},
      {TEMPERATURE_SETTINGS, updateTempSettings, false},
      {AGE_SETTINGS, updateAge, false},
      {UNITS_SETTINGS, updateUnits, false},
      {LANGUAGE_SETTINGS, updateLanguage, false},
      {TIME_SETTINGS, updateTime, false},
      {SYSTEM_SETTINGS, updateSysSettings, false},
      {ABOUT_SYSTEM, updateAbout, false},
      {DEVELOPER_INFO, updateDeveloper, false},
      {RESET_INFO, updateResetInfo, true},
      {RESET_FINAL_CONFIRM, updateResetConfirm, true},
      {ERROR, ErrorHandler, true}};

  for (const auto &entry : updateTable)
  {
    if (entry.state == currState)
    {
      entry.updateFunc(sprite);
      if (entry.needPushSprite)
        sprite.pushSprite(0, 0);
      return;
    }
  }
}

void handleForward()
{
  if (currState >= MAIN && currState <= SETTINGS)
  {
    int currentMenu = currState - MAIN;
    int nextMenu = (currentMenu + 1) % 4 + MAIN;
    changeState(nextMenu, SLIDE_UP);
  }
}

void handleBackward()
{
  if (currState >= MAIN && currState <= SETTINGS)
  {
    int currentMenu = currState - MAIN;
    int prevMenu = (currentMenu - 1 + 4) % 4 + MAIN;
    changeState(prevMenu, SLIDE_DOWN);
  }
}

void handleSelect()
{
  if (currState == PROFILE)
  {
    profileSelect = true;
    initialBrightness = DEF_BRIGHTNESS;
    targBrightnessValue = getBrightness(selectProfile);

    if (initialBrightness != targBrightnessValue)
    {
      isAnimBrightness = true;
      brightnessAnimStart = millis();
    }
    else
    {
      setBrightness(targBrightnessValue);
    }

    loadStandbyTime(selectProfile);
    loadLanguage(selectProfile);
    changeState(MAIN, SLIDE_UP);
  }
}

void handleProfileForward()
{
  if (currState != PROFILE)
    return;

  selectProfile = (selectProfile + 1) % profileNum;
  animationDirection = 1;
  animationType = ROLLER;
  isAnimating = true;
  animationProgress = 0;
  lastAnimationTime = millis();
}

void handleStandbyMode()
{
  bool btnUp = !digitalRead(BTN_UP), btnLow = !digitalRead(BTN_LOW);
  bool anyBtn = btnUp || btnLow;

  if (isInStandbyMode())
  {
    if (anyBtn)
    {
      resetStandbyTimer();
    }
  }
  else
  {
    if (anyBtn)
    {
      resetStandbyTimer();
    }
  }
}

void handleButtonInput()
{
  static bool lastUp, lastLow, upLong, lowLong;
  static unsigned long upTime, lowTime;
  bool btnUp = !digitalRead(BTN_UP), btnLow = !digitalRead(BTN_LOW);

  if (isAnimating || isTransition || isRecentStandbyExit() || !isTransitionComplete)
  {
    lastUp = btnUp;
    lastLow = btnLow;
    return;
  }

  // Проверка длинных нажатий
  if (btnUp && !lastUp)
  {
    upTime = millis();
    upLong = false;
  }
  if (btnLow && !lastLow)
  {
    lowTime = millis();
    lowLong = false;
  }
  bool upLP = btnUp && !upLong && (millis() - upTime > LONG_PRESS) ? (upLong = true) : false;
  bool lowLP = btnLow && !lowLong && (millis() - lowTime > LONG_PRESS) ? (lowLong = true) : false;

  // Блокируем кнопки после длинного нажатия
  if (upLP)
    up_button_locked = true;
  if (lowLP)
    low_button_locked = true;

  // Разблокируем при отпускании
  if (!btnUp)
    up_button_locked = false;
  if (!btnLow)
    low_button_locked = false;

  // Типы нажатий: Short - короткое, Click - нажатие
  bool upS = !btnUp && lastUp && !upLong, lowS = !btnLow && lastLow && !lowLong;
  bool upC = btnUp && !lastUp && !up_button_locked, lowC = btnLow && !lastLow && !low_button_locked;

  // Экраны подтверждения
  if (currState == RESET_INFO)
  {
    if (lowLP)
      changeState(SYSTEM_SETTINGS, SLIDE_DOWN);
    else
      handleResetInfoInput(upS || lowS, upLP || lowLP);
  }
  else if (currState == RESET_FINAL_CONFIRM)
  {
    handleResetConfirmInput(upS || lowS, upLP || lowLP, selectProfile);
  }
  // Экран ошибки
  else if (currState == ERROR)
  {
    if (upC)
      switchErrorAnim();
    if (lowC)
      changeState((prevState != ERROR && prevState != LOGO) ? prevState : MAIN, SLIDE_DOWN);
  }
  // Информационные экраны - любая кнопка возвращает назад
  else if (currState == ABOUT_SYSTEM || currState == DEVELOPER_INFO)
  {
    if (upC || lowC)
      changeState(SYSTEM_SETTINGS, SLIDE_DOWN);
  }
  // Главное меню настроек
  else if (currState == SETTINGS)
  {
    if (lowLP)
      getSelectionMode() ? toggleSelectionMode() : changeState(MAIN, SLIDE_DOWN);
    else if (lowS)
      getSelectionMode() ? handleSettings(false) : handleBackward();
    if (upLP)
    {
      if (getSelectionMode())
      {
        int s = getSelectedSettings();
        if (s == 0)
          changeState(DISPLAY_SETTINGS, SLIDE_UP);
        else if (s == 1)
          changeState(TEMPERATURE_SETTINGS, SLIDE_UP);
        else if (s == 4)
          changeState(SYSTEM_SETTINGS, SLIDE_UP);
      }
      else
        toggleSelectionMode();
    }
    else if (upS)
      getSelectionMode() ? handleSettings(true) : handleForward();
  }
  // Настройки дисплея
  else if (currState == DISPLAY_SETTINGS)
  {
    if (lowLP)
      changeState(SETTINGS, SLIDE_DOWN);
    else if (lowS)
      handleDisplaySettings(false);
    if (upLP)
    {
      int s = getSelectedDisplaySettings();
      changeState(s == 0 ? BRIGHTNESS_SETTINGS : STANDBY_SETTINGS, SLIDE_UP);
    }
    else if (upS)
      handleDisplaySettings(true);
  }
  // Настройки температуры
  else if (currState == TEMPERATURE_SETTINGS)
  {
    if (lowLP)
      changeState(SETTINGS, SLIDE_DOWN);
    else if (lowS)
      handleTempSettings(false);
    if (upLP)
    {
      int s = getSelectedTempSettings();
      changeState(s == 0 ? AGE_SETTINGS : UNITS_SETTINGS, SLIDE_UP);
    }
    else if (upS)
      handleTempSettings(true);
  }
  // Настройка возраста
  else if (currState == AGE_SETTINGS)
  {
    if (lowLP)
      changeState(TEMPERATURE_SETTINGS, SLIDE_DOWN);
    else if (lowS || upC)
      handleAge(upC);
  }
  // Настройка единиц измерения
  else if (currState == UNITS_SETTINGS)
  {
    if (lowLP)
      changeState(TEMPERATURE_SETTINGS, SLIDE_DOWN);
    else if (lowS || upC)
      handleUnits(upC);
  }
  // Системные настройки
  else if (currState == SYSTEM_SETTINGS)
  {
    if (lowLP)
      changeState(SETTINGS, SLIDE_DOWN);
    else if (lowS)
      handleSysSettings(false);
    if (upLP)
    {
      int s = getSelectedSysSettings();
      int states[] = {LANGUAGE_SETTINGS, TIME_SETTINGS, ABOUT_SYSTEM, DEVELOPER_INFO, RESET_INFO};
      if (s < 5)
        changeState(states[s], SLIDE_UP);
    }
    else if (upS)
      handleSysSettings(true);
  }
  // Настройка яркости - низ уменьшает, верх увеличивает
  else if (currState == BRIGHTNESS_SETTINGS)
  {
    if (lowLP)
      changeState(DISPLAY_SETTINGS, SLIDE_DOWN);
    else if (lowS || upC)
      handleBrightness(upC);
  }
  // Настройка ждущего режима
  else if (currState == STANDBY_SETTINGS)
  {
    if (lowLP)
      changeState(DISPLAY_SETTINGS, SLIDE_DOWN);
    else if (lowS || upC)
      handleStandby(upC);
  }
  // Настройка языка
  else if (currState == LANGUAGE_SETTINGS)
  {
    if (lowLP)
      changeState(SYSTEM_SETTINGS, SLIDE_DOWN);
    else if (lowS || upC)
      handleLanguage(upC);
  }
  // Настройка времени
  else if (currState == TIME_SETTINGS)
  {
    if (lowLP)
      changeState(SYSTEM_SETTINGS, SLIDE_DOWN);
    else if (lowS || upC)
      handleTime(upC);
  }
  // Все остальные экраны - базовая навигация
  else
  {
    if (upC)
      (currState >= MAIN && currState <= SETTINGS) ? handleForward() : (currState == PROFILE ? handleSelect() : handleSelect());
    if (lowC)
      (currState >= MAIN && currState <= SETTINGS) ? handleBackward() : (currState == PROFILE ? handleProfileForward() : handleSelect());
  }

  lastUp = btnUp;
  lastLow = btnLow;
}

void setup()
{
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(0x0000);
  Serial.begin(115200);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_LOW, INPUT_PULLUP);

  initErrorHandler(&tft);
  initializeStorage();
  setupBrightness();
  setupStandby();

  currSprite.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);
  nextSprite.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);
  tempSprite.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);
  sprite.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);

  if (!currSprite.created() || !nextSprite.created() || !tempSprite.created() || !sprite.created())
  {
    errorCall(SPRITE_CREATE_ERROR, ERR_CRITICAL, "");
    while (true)
      delay(1000);
  }

  setBrightness(0);
  showLogo(currSprite);
  isAnimLogo = true;
  InlogoAnim = millis();
  logoTargetBrightness = DEF_BRIGHTNESS;

  if (!errorCount && (profileNum == 0 || profileNum > 10))
  {
    errorCall(profileNum == 0 ? NO_PROFILES_ERROR : TOO_MANY_PROFILES_ERROR, ERR_MEDIUM, "");
  }
}

void loop()
{
  // Отладочная команда для вывода файловой системы
  if (Serial.available() > 0)
  {
    String command = Serial.peek() == 'l' ? Serial.readStringUntil('\n') : "";

    if (command == "ls")
    {
      Serial.println("\n--- File System Structure (Command Triggered) ---");
      listAllFiles("/", "");
      Serial.println("-------------------------------------------------");
    }
  }

  if (errorCount && currState != ERROR && currState != LOGO)
  {
    changeState(ERROR, SLIDE_UP);
  }

  handleStandbyMode();
  handleButtonInput();
  updateDisplay();
}