#include <ArduinoJson.h>
#include <LittleFS.h>
#include "config.h"
#include "settings/system/language/sys_language.h"
#include "error/error_handler.h"

static const char *LANGUAGE_KEY = "language";
static const String LANGUAGE_OPTIONS[] = {"English", "Русский"};
static const size_t LANGUAGE_COUNT = sizeof(LANGUAGE_OPTIONS) / sizeof(LANGUAGE_OPTIONS[0]);
static int currentLanguageIndex = DEF_LANGUAGE;

static String getSettingsPath(int profileId)
{
  if (profileId < 0 || profileId >= profileNum)
    return "";
  return "/storage/profiles/" + String(profileNames[profileId]) + "/settings.json";
}

String getCurrlanguage()
{
  return LANGUAGE_OPTIONS[currentLanguageIndex];
}

void loadLanguage(int profileId)
{
  currentLanguageIndex = getLanguageID(profileId);
}

String getLanguage(int profileId)
{
  return LANGUAGE_OPTIONS[getLanguageID(profileId)];
}

int getLanguageID(int profileId)
{
  String path = getSettingsPath(profileId);
  if (path.isEmpty() || !LittleFS.exists(path))
    return DEF_LANGUAGE;

  File file = LittleFS.open(path, "r");
  if (!file)
    return DEF_LANGUAGE;

  JsonDocument filter;
  filter[LANGUAGE_KEY] = true;

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file, DeserializationOption::Filter(filter));
  file.close();

  if (error)
    return DEF_LANGUAGE;

  return doc[LANGUAGE_KEY] | DEF_LANGUAGE;
}

void saveLanguage(int profileId, String languageStr)
{
  for (size_t i = 0; i < LANGUAGE_COUNT; i++)
  {
    if (LANGUAGE_OPTIONS[i] == languageStr)
    {
      saveLanguageID(profileId, i);
      return;
    }
  }
  saveLanguageID(profileId, DEF_LANGUAGE);
}

void saveLanguageID(int profileId, int languageIndex)
{
  String path = getSettingsPath(profileId);
  if (path.isEmpty())
    return;

  if (languageIndex == getLanguageID(profileId))
  {
    currentLanguageIndex = languageIndex;
    return;
  }

  String dirPath = "/storage/profiles/" + String(profileNames[profileId]);
  if (!LittleFS.exists(dirPath) && !LittleFS.mkdir(dirPath))
  {
    errorCall(LFS_FILE_ERROR, ERR_HIGH, "Cannot create directory: " + dirPath);
    return;
  }

  JsonDocument doc;
  File file = LittleFS.open(path, "r");
  if (file)
  {
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (error)
    {
      doc.clear();
    }
  }

  doc[LANGUAGE_KEY] = languageIndex;
  currentLanguageIndex = languageIndex;

  file = LittleFS.open(path, "w");
  if (!file)
  {
    errorCall(LFS_FILE_ERROR, ERR_HIGH, "Write " + path);
    return;
  }

  if (serializeJson(doc, file) == 0)
    errorCall(LFS_FILE_ERROR, ERR_HIGH, "Serialize " + path);

  file.close();
}