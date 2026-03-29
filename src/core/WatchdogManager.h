#pragma once
#include <Arduino.h>
#include <esp_task_wdt.h>
#include "Logger.h"
#include "../hal/Hal.h"

// WDT Configuration
#define WDT_TIMEOUT_SECONDS 8

class WatchdogManager
{
public:
    static void init()
    {
        // 1. Analyze Reset Reason
        esp_reset_reason_t reason = esp_reset_reason();
        logResetReason(reason);

        // 2. Visual Alert if Crash detected
        if (reason == ESP_RST_PANIC || reason == ESP_RST_WDT || reason == ESP_RST_SW)
        {
            alertCrash();
        }

        // 3. Initialize Watchdog
        // Add loop task to WDT. If loop() doesn't feed WDT for WDT_TIMEOUT_SECONDS, reset.
        esp_task_wdt_init(WDT_TIMEOUT_SECONDS, true); // true = panic causes reset
        esp_task_wdt_add(NULL);                       // Add current thread (Main Loop)

        Logger::log("WDT", "Watchdog active. Timeout: %ds", WDT_TIMEOUT_SECONDS);
    }

    static void feed()
    {
        esp_task_wdt_reset();
    }

private:
    static void logResetReason(esp_reset_reason_t reason)
    {
        const char *rStr = "UNK";
        switch (reason)
        {
        case ESP_RST_POWERON:
            rStr = "POWERON";
            break;
        case ESP_RST_EXT:
            rStr = "EXT_PIN";
            break;
        case ESP_RST_SW:
            rStr = "SW_RESET";
            break;
        case ESP_RST_PANIC:
            rStr = "PANIC/EXCEPTION";
            break;
        case ESP_RST_INT_WDT:
            rStr = "INT_WDT";
            break;
        case ESP_RST_TASK_WDT:
            rStr = "TASK_WDT";
            break;
        case ESP_RST_WDT:
            rStr = "OTHER_WDT";
            break;
        case ESP_RST_DEEPSLEEP:
            rStr = "DEEP_SLEEP";
            break;
        case ESP_RST_BROWNOUT:
            rStr = "BROWNOUT";
            break;
        case ESP_RST_SDIO:
            rStr = "SDIO";
            break;
        default:
            break;
        }
        Logger::log("BOOT", "Reset Reason: %s", rStr);
    }

    static void alertCrash()
    {
        Logger::log("WDT", "CRASH DETECTED! Blinking RED...");
        // Red Blink 5 times
        for (int i = 0; i < 5; i++)
        {
            LedHAL::getInstance().set(255, 0, 0); // RED
            delay(200);
            LedHAL::getInstance().set(0, 0, 0);
            delay(200);
        }
    }
};
