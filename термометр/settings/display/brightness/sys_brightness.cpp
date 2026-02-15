#include <ArduinoJson.h>
#include <LittleFS.h>
#include "config.h"
#include "settings/display/brightness/sys_brightness.h"
#include "error/error_handler.h"

static const char *BRIGHTNESS_KEY = "brightness";
static uint8_t currentBrightnessValue = DEF_BRIGHTNESS;

bool isAnimBrightness = false;
unsigned long brightnessAnimStart = 0;
uint8_t initialBrightness = 0;
uint8_t targBrightnessValue = 0;

static String getSettingsPath(int profileId)
{
  if (profileId < 0 || profileId >= profileNum || profileNames[profileId] == NULL || profileNames[profileId][0] == '\0')
    return "";

  return "/storage/profiles/" + String(profileNames[profileId]) + "/settings.json";
}

void setupBrightness()
{
  ledcSetup(0, 5000, 10);
  ledcAttachPin(TFT_BL, 0);
  setBrightness(DEF_BRIGHTNESS);
}

void setBrightness(uint8_t value)
{
  value = constrain(value, 0, 100);
  currentBrightnessValue = value;
  ledcWrite(0, map(value, 0, 100, 0, 1023));
}

uint8_t getCurrentBrightness()
{
  return currentBrightnessValue;
}

void loadBrightness(int profileId)
{
  setBrightness(getBrightness(profileId));
}

uint8_t getBrightness(int profileId)
{
  String path = getSettingsPath(profileId);
  if (path.isEmpty() || !LittleFS.exists(path))
    return DEF_BRIGHTNESS;

  File file = LittleFS.open(path, "r");
  if (!file)
    return DEF_BRIGHTNESS;

  JsonDocument filter;
  filter[BRIGHTNESS_KEY] = true;

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file, DeserializationOption::Filter(filter));
  file.close();

  if (error)
    return DEF_BRIGHTNESS;

  uint8_t brightness = doc[BRIGHTNESS_KEY] | DEF_BRIGHTNESS;
  return constrain(brightness, 0, 100);
}

void saveBrightness(int profileId, uint8_t value)
{
  String path = getSettingsPath(profileId);
  if (path.isEmpty())
    return;

  value = constrain(value, 0, 100);

  if (LittleFS.exists(path))
  {
    JsonDocument filter;
    filter[BRIGHTNESS_KEY] = true;

    JsonDocument checkDoc;
    File file = LittleFS.open(path, "r");
    if (file)
    {
      DeserializationError error = deserializeJson(checkDoc, file, DeserializationOption::Filter(filter));
      file.close();

      if (!error && checkDoc[BRIGHTNESS_KEY] == value)
        return;
    }
  }
  else
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

  doc[BRIGHTNESS_KEY] = value;

  file = LittleFS.open(path, "w");
  if (!file)
  {
    errorCall(FILE_WRITE_ERROR, ERR_HIGH, "Cannot open for write: " + path);
    return;
  }

  if (serializeJson(doc, file) == 0)
    errorCall(JSON_WRITE_ERROR, ERR_HIGH, "Failed to write JSON to " + path);

  file.close();
}

void updateBrightnessAnim()
{
  if (!isAnimBrightness)
    return;

  unsigned long elapsed = millis() - brightnessAnimStart;
  if (elapsed >= 1000)
  {
    setBrightness(targBrightnessValue);
    isAnimBrightness = false;
    return;
  }

  float progress = (float)elapsed / 1000;
  setBrightness(initialBrightness + (targBrightnessValue - initialBrightness) * progress);
}