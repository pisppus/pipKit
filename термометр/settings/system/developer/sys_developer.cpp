#include <LittleFS.h>
#include "settings/system/developer/sys_developer.h"

String getCpuFreqString() {
    return String(ESP.getCpuFreqMHz()) + " MHz";
}
String getFreeHeapString() {
    return String(ESP.getFreeHeap()) + " B";
}
String getFsUsageString() {
    return String(LittleFS.usedBytes() / 1024) + "/" + String(LittleFS.totalBytes() / 1024) + " KB";
}

String getResetReasonString() {
    switch (esp_reset_reason()) {
        case ESP_RST_UNKNOWN:   return "Unknown";
        case ESP_RST_POWERON:   return "Power On";
        case ESP_RST_EXT:       return "External Pin";
        case ESP_RST_SW:        return "Software";
        case ESP_RST_PANIC:     return "Panic";
        case ESP_RST_INT_WDT:   return "Interrupt WDT";
        case ESP_RST_TASK_WDT:  return "Task WDT";
        case ESP_RST_WDT:       return "Other WDT";
        case ESP_RST_DEEPSLEEP: return "Deep Sleep";
        case ESP_RST_BROWNOUT:  return "Brownout";
        case ESP_RST_SDIO:      return "SDIO";
        default:                return "N/A";
    }
} 