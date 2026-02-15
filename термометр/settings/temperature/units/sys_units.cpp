#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config.h"
#include "settings/temperature/units/sys_units.h"
#include "error/error_handler.h"

static const char *UNIT_TEMP_KEY = "unit_temp";
extern bool profileSelect;

static String getSettingsPath(int profileId)
{
  if (profileId < 0 || profileId >= profileNum || profileNames[profileId] == NULL || profileNames[profileId][0] == '\0')
    return "";

  return "/storage/profiles/" + String(profileNames[profileId]) + "/settings.json";
}

int getTempUnit()
{
  return profileSelect ? getTempUnit(selectProfile) : DEF_UNIT_TEMP;
}

int getTempUnit(int profileId)
{
  String path = getSettingsPath(profileId);
  if (path.isEmpty() || !LittleFS.exists(path))
    return DEF_UNIT_TEMP;

  File file = LittleFS.open(path, "r");
  if (!file)
    return DEF_UNIT_TEMP;

  JsonDocument filter;
  filter[UNIT_TEMP_KEY] = true;

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file, DeserializationOption::Filter(filter));
  file.close();

  if (error)
    return DEF_UNIT_TEMP;

  return doc[UNIT_TEMP_KEY] | DEF_UNIT_TEMP;
}

void saveTempUnit(int unit)
{
  if (profileSelect)
    saveTempUnit(selectProfile, unit);
}

void saveTempUnit(int profileId, int unit)
{
  String path = getSettingsPath(profileId);
  if (path.isEmpty())
    return;

  if (unit == getTempUnit(profileId))
    return;

  String dirPath = "/storage/profiles/" + String(profileNames[profileId]);
  if (!LittleFS.exists(dirPath) && !LittleFS.mkdir(dirPath))
  {
    errorCall(LFS_MKDIR_ERROR, ERR_HIGH, "Cannot create directory: " + dirPath);
    return;
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

  doc[UNIT_TEMP_KEY] = unit;

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