#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config.h"
#include "error/error_handler.h"
#include "settings/system/about/sys_about.h"

String getUptimeString()
{
    unsigned long total_seconds = millis() / 1000;
    char uptimeStr[12];
    snprintf(uptimeStr, sizeof(uptimeStr), "%02lu:%02lu:%02lu",
             total_seconds / 3600,
             (total_seconds % 3600) / 60,
             total_seconds % 60);
    return String(uptimeStr);
}

void getSysInfo(String &fw_version, int &boot_count)
{
    fw_version = "N/A";
    boot_count = 0;

    File file = LittleFS.open("/storage/system/sysinfo.json", "r");
    if (!file)
        return;

    JsonDocument filter;
    filter["firmware_version"] = true;
    filter["boot_count"] = true;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file, DeserializationOption::Filter(filter));
    file.close();

    if (error)
        return;

    fw_version = doc["firmware_version"] | "N/A";
    boot_count = doc["boot_count"] | 0;
}