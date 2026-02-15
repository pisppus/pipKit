#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config.h"
#include "settings/temperature/age/sys_age.h"
#include "error/error_handler.h"

static const char *AGE_KEY = "age";
static const int DEFAULT_AGE = 4; // Взрослый (18-64 года)
extern bool profileSelect;

static String getSettingsPath(int profileId)
{
  if (profileId < 0 || profileId >= profileNum || profileNames[profileId] == NULL || profileNames[profileId][0] == '\0')
    return "";

  return "/storage/profiles/" + String(profileNames[profileId]) + "/settings.json";
}

int getAge()
{
  return profileSelect ? getAge(selectProfile) : DEFAULT_AGE;
}

int getAge(int profileId)
{
  String path = getSettingsPath(profileId);
  if (path.isEmpty() || !LittleFS.exists(path))
    return DEFAULT_AGE;

  File file = LittleFS.open(path, "r");
  if (!file)
    return DEFAULT_AGE;

  JsonDocument filter;
  filter[AGE_KEY] = true;

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file, DeserializationOption::Filter(filter));
  file.close();

  if (error)
    return DEFAULT_AGE;

  return doc[AGE_KEY] | DEFAULT_AGE;
}

void saveAge(int ageGroup)
{
  if (profileSelect)
    saveAge(selectProfile, ageGroup);
}

void saveAge(int profileId, int ageGroup)
{
  String path = getSettingsPath(profileId);
  if (path.isEmpty())
    return;

  if (LittleFS.exists(path))
  {
    JsonDocument doc;
    File file = LittleFS.open(path, "r");
    if (file)
    {
      DeserializationError error = deserializeJson(doc, file);
      file.close();

      if (!error && doc[AGE_KEY] == ageGroup)
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

  doc[AGE_KEY] = ageGroup;
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