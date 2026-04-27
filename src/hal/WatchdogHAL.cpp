#include "WatchdogHAL.h"
#include "esp_system.h"
#include "esp_task_wdt.h"

WatchdogHAL &WatchdogHAL::getInstance()
{
    static WatchdogHAL instance;
    return instance;
}

void WatchdogHAL::begin(uint32_t timeoutMs)
{
    // Configure Task Watchdog (TWDT).
    // NOTE: arduino-esp32 v3.x stock still ships the legacy
    // esp_task_wdt_init(timeout_seconds, panic) signature; the IDF v5 native
    // esp_task_wdt_config_t / esp_task_wdt_reconfigure surface is not exposed
    // through the Arduino layer yet. Migrate when upstream lands it.
    // See DevGuidelines §11.4.
    esp_err_t err = esp_task_wdt_init(timeoutMs / 1000, true);
    if (err != ESP_OK)
    {
        Logger::log(TAG, "WDT init failed: %s", esp_err_to_name(err));
    }
    // Subscribe current task (idempotent if already added)
    esp_err_t add = esp_task_wdt_add(NULL);
    if (add != ESP_OK && add != ESP_ERR_INVALID_ARG)
    {
        Logger::log(TAG, "WDT add failed: %s", esp_err_to_name(add));
    }
    Logger::log(TAG, "WDT Enabled: %u ms", (unsigned)timeoutMs);
}

void WatchdogHAL::feed()
{
    esp_task_wdt_reset();
}

String WatchdogHAL::getResetReason()
{
    esp_reset_reason_t reason = esp_reset_reason();
    switch (reason)
    {
    case ESP_RST_POWERON:
        return "Power On";
    case ESP_RST_EXT:
        return "External Pin";
    case ESP_RST_SW:
        return "Software";
    case ESP_RST_PANIC:
        return "Panic/Exception";
    case ESP_RST_INT_WDT:
        return "Interrupt WDT";
    case ESP_RST_TASK_WDT:
        return "Task WDT";
    case ESP_RST_WDT:
        return "Other WDT";
    case ESP_RST_DEEPSLEEP:
        return "Deep Sleep";
    case ESP_RST_BROWNOUT:
        return "Brownout";
    case ESP_RST_SDIO:
        return "SDIO";
    default:
        return "Unknown";
    }
}
