#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config.h"
#include "settings/system/reset/sys_reset.h"
#include "error/error_handler.h"

static const char *KEY_BRIGHTNESS = "brightness";
static const char *KEY_STANDBY_TIME = "standby_time";
static const char *KEY_LANGUAGE = "language";
static const char *KEY_UNIT_TEMP = "unit_temp";

static String getSettingsPath(int profileId)
{
    if (profileId < 0 || profileId >= profileNum || profileNames[profileId] == NULL || profileNames[profileId][0] == '\0')
        return "";
    return "/storage/profiles/" + String(profileNames[profileId]) + "/settings.json";
}

static String getDirectoryPath(int profileId)
{
    if (profileId < 0 || profileId >= profileNum || profileNames[profileId] == NULL || profileNames[profileId][0] == '\0')
        return "";
    return "/storage/profiles/" + String(profileNames[profileId]);
}

void FactoryReset(int profileId)
{
    String dirPath = getDirectoryPath(profileId);
    if (dirPath.isEmpty())
        return;

    String path = getSettingsPath(profileId);

    if (!LittleFS.exists(dirPath))
    {
        if (!LittleFS.mkdir(dirPath))
        {
            errorCall(LFS_MKDIR_ERROR, ERR_HIGH, "Reset failed: cannot create directory");
            return;
        }
    }

    JsonDocument doc;
    doc[KEY_BRIGHTNESS] = DEF_BRIGHTNESS;
    doc[KEY_STANDBY_TIME] = DEF_STANDBY_TIME;
    doc[KEY_LANGUAGE] = DEF_LANGUAGE;
    doc[KEY_UNIT_TEMP] = DEF_UNIT_TEMP;

    File file = LittleFS.open(path, "w");
    if (!file)
    {
        errorCall(FILE_WRITE_ERROR, ERR_HIGH, "Reset failed: cannot open file for writing");
        return;
    }

    if (serializeJson(doc, file) == 0)
    {
        errorCall(JSON_WRITE_ERROR, ERR_HIGH, "Reset failed: file write error");
        file.close();
        return;
    }

    file.close();
    ESP.restart();
}