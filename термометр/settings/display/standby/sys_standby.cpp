#include <ArduinoJson.h>
#include <LittleFS.h>
#include "config.h"
#include "settings/display/standby/sys_standby.h"
#include "settings/display/brightness/sys_brightness.h"
#include "error/error_handler.h"

extern bool profileSelect;
static const char *STANDBY_TIME_KEY = "standby_time";
static uint16_t currentStandbyTime = DEF_STANDBY_TIME;
static unsigned long lastActivityTime = 0;
static bool standbyModeActive = false;
static unsigned long lastStandbyExitTime = 0;
static uint8_t savedBrightnessValue = DEF_BRIGHTNESS;

// Переменные анимации
static bool isStandbyAnimBrightness = false;
static unsigned long animStartTime = 0;
static uint8_t startBrightness = 0;
static uint8_t targetBrightness = 0;

static String getSettingsPath(int profileId)
{
  if (profileId < 0 || profileId >= profileNum || profileNames[profileId] == NULL || profileNames[profileId][0] == '\0')
    return "";

  return "/storage/profiles/" + String(profileNames[profileId]) + "/settings.json";
}

void setupStandby()
{
  lastActivityTime = millis();
}

bool checkStandby()
{
  if (!currentStandbyTime)
    return false;

  if (isStandbyAnimBrightness)
    updStandbyBrightnessAnim();

  if (standbyModeActive)
    return true;

  if (millis() - lastActivityTime >= (unsigned long)currentStandbyTime * 1000)
  {
    enterStandbyMode();
    return true;
  }

  return false;
}

void updStandbyBrightnessAnim()
{
  if (!isStandbyAnimBrightness)
    return;

  unsigned long elapsed = millis() - animStartTime;
  if (elapsed >= 1000)
  {
    setBrightness(targetBrightness);
    isStandbyAnimBrightness = false;
  }
  else
  {
    float progress = (float)elapsed / 1000.0f;
    setBrightness(startBrightness + (targetBrightness - startBrightness) * progress);
  }
}

void enterStandbyMode()
{
  if (standbyModeActive)
    return;

  standbyModeActive = true;
  savedBrightnessValue = profileSelect ? getBrightness(selectProfile) : getCurrentBrightness();
  startBrightness = getCurrentBrightness();
  targetBrightness = 10;
  isStandbyAnimBrightness = true;
  animStartTime = millis();
}

void exitStandbyMode()
{
  if (!standbyModeActive)
    return;

  standbyModeActive = false;
  lastStandbyExitTime = millis();
  startBrightness = getCurrentBrightness();
  targetBrightness = savedBrightnessValue;
  isStandbyAnimBrightness = true;
  animStartTime = millis();
}

void resetStandbyTimer()
{
  lastActivityTime = millis();
  if (standbyModeActive)
    exitStandbyMode();
}

bool isRecentStandbyExit()
{
  return (millis() - lastStandbyExitTime) < 500;
}

void loadStandbyTime(int profileId)
{
  setStandbyTime(getStandbyTime(profileId));
}

uint16_t getStandbyTime(int profileId)
{
  String path = getSettingsPath(profileId);
  if (path.isEmpty() || !LittleFS.exists(path))
    return DEF_STANDBY_TIME;

  File file = LittleFS.open(path, "r");
  if (!file)
    return DEF_STANDBY_TIME;

  JsonDocument filter;
  filter[STANDBY_TIME_KEY] = true;

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file, DeserializationOption::Filter(filter));
  file.close();

  if (error)
    return DEF_STANDBY_TIME;

  return doc[STANDBY_TIME_KEY] | DEF_STANDBY_TIME;
}

void saveStandbyTime(int profileId, uint16_t seconds)
{
  String path = getSettingsPath(profileId);
  if (path.isEmpty())
    return;

  if (seconds == getStandbyTime(profileId))
    return;

  if (!LittleFS.exists(path))
  {
    String dirPath = "/storage/profiles/" + String(profileNames[profileId]);
    if (!LittleFS.exists(dirPath) && !LittleFS.mkdir(dirPath))
    {
      errorCall(LFS_MKDIR_ERROR, ERR_HIGH, "Cannot create directory: " + dirPath);
      return;
    }
  }

  JsonDocument doc;
  File file = LittleFS.open(path, "r");
  if (file)
  {
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (error)
      doc.clear();
  }

  doc[STANDBY_TIME_KEY] = seconds;
  file = LittleFS.open(path, "w");
  if (!file)
  {
    errorCall(FILE_WRITE_ERROR, ERR_HIGH, "Cannot open for write: " + path);
    return;
  }

  if (serializeJson(doc, file) == 0)
    errorCall(JSON_WRITE_ERROR, ERR_HIGH, "Failed to write JSON to " + path);

  file.close();
  setStandbyTime(seconds);
}

void setStandbyTime(uint16_t seconds)
{
  currentStandbyTime = seconds;
  resetStandbyTimer();
}

bool isInStandbyMode()
{
  return standbyModeActive;
}

bool isExitingStandby()
{
  return isStandbyAnimBrightness && !standbyModeActive;
}