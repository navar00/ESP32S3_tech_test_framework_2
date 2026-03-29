#include "WatchdogHAL.h"
#include "esp_system.h"
#include "esp_task_wdt.h"

WatchdogHAL& WatchdogHAL::getInstance() {
    static WatchdogHAL instance;
    return instance;
}

void WatchdogHAL::begin(uint32_t timeoutMs) {
    // Configure Task Watchdog (TWDT)
    esp_task_wdt_init(timeoutMs / 1000, true); // true = panic on timeout
    esp_task_wdt_add(NULL); // Add current thread
    Logger::log(TAG, "WDT Enabled: %d ms", timeoutMs);
}

void WatchdogHAL::feed() {
    esp_task_wdt_reset();
}

String WatchdogHAL::getResetReason() {
    esp_reset_reason_t reason = esp_reset_reason();
    switch (reason) {
        case ESP_RST_POWERON: return "Power On";
        case ESP_RST_EXT: return "External Pin";
        case ESP_RST_SW: return "Software";
        case ESP_RST_PANIC: return "Panic/Exception";
        case ESP_RST_INT_WDT: return "Interrupt WDT";
        case ESP_RST_TASK_WDT: return "Task WDT";
        case ESP_RST_WDT: return "Other WDT";
        case ESP_RST_DEEPSLEEP: return "Deep Sleep";
        case ESP_RST_BROWNOUT: return "Brownout";
        case ESP_RST_SDIO: return "SDIO";
        default: return "Unknown";
    }
}
