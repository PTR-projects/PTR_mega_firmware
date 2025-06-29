#include "ResetReason.h"
#include "esp_system.h"
#include "esp_log.h"

static const char *TAG = "RESET_REASON";

const char* get_reset_reason_string(void)
{
    esp_reset_reason_t reason = esp_reset_reason();
    switch (reason) {
        case ESP_RST_UNKNOWN:   return "Unknown";
        case ESP_RST_POWERON:   return "Power-on reset";
        case ESP_RST_EXT:       return "External pin reset";
        case ESP_RST_SW:        return "Software reset via esp_restart";
        case ESP_RST_PANIC:     return "Panic/exception reset";
        case ESP_RST_INT_WDT:   return "Interrupt watchdog reset";
        case ESP_RST_TASK_WDT:  return "Task watchdog reset";
        case ESP_RST_WDT:       return "Other watchdog reset";
        case ESP_RST_DEEPSLEEP: return "Exiting deep sleep";
        case ESP_RST_BROWNOUT:  return "Brownout reset (voltage dip)";
        case ESP_RST_SDIO:      return "Reset over SDIO";
        default:                return "Invalid or unknown reason";
    }
}

void log_reset_reason(void)
{
    ESP_LOGI(TAG, "Last reset reason: %s", get_reset_reason_string());
}