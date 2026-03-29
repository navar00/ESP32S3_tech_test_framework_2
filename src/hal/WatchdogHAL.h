#pragma once
#include <Arduino.h>
#include <esp_task_wdt.h>
#include "../core/Config.h"
#include "../core/Logger.h"

class WatchdogHAL {
public:
    static WatchdogHAL& getInstance();
    void begin(uint32_t timeoutMs);
    void feed();
    String getResetReason();

private:
    WatchdogHAL() = default;
    const char* TAG = "WDTHAL";
};
