#include <Arduino.h>
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "core/Config.h"
#include "core/Logger.h"
#include "core/ScreenManager.h"
#include "core/BootOrchestrator.h"
#include "hal/Hal.h"

// Includes for Screens
#include "screens/ScreenStatus.h"
#include "screens/ScreenGameOfLife.h"
#include "screens/ScreenAnalogClock.h"
#include "screens/ScreenFlipClock.h"
#include "screens/ScreenPalette.h"
#include "services/WebService.h"

// --- Global Context ---
volatile bool timerFlag = false;
esp_timer_handle_t tickTimer = nullptr;
BootOrchestrator bootOrchestrator;
// ScreenManager is a Singleton, accessed via getInstance()

// --- Timer Callback (runs in esp_timer task, NOT in ISR) ---
static void onTickTimer(void * /*arg*/)
{
    timerFlag = true;
}

// --- Setup ---
void setup()
{
    // 1. Core Init
    Serial.begin(Config::SERIAL_BAUD);
    Logger::init();

    // 2. Hardware Init (Pre-Boot)
    LedHAL::getInstance().init();
    LedHAL::getInstance().set(255, 128, 0); // Amber Status

    // Boot heap snapshot (helps diagnose sprite alloc regressions)
    Logger::log("MAIN", "Boot heap free=%u largest=%u",
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));

    // 3. Boot Orchestrator (Blocking)
    bootOrchestrator.run();

    // 4. Runtime Init
    LedHAL::getInstance().set(0, 255, 0); // Green Status

    // Reduce Watchdog for tight loop
    WatchdogHAL::getInstance().begin(Config::WDT_TIMEOUT_LOOP);

    // 5. Timer Setup (1 Hz) — esp_timer (IDF v5 native, callback runs in esp_timer task).
    const esp_timer_create_args_t tickArgs = {
        .callback = &onTickTimer,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "tick_1hz",
        .skip_unhandled_events = true};
    if (esp_timer_create(&tickArgs, &tickTimer) == ESP_OK)
    {
        esp_timer_start_periodic(tickTimer, 1000000ULL); // 1 s
    }
    else
    {
        Logger::log("MAIN", "esp_timer_create failed");
    }

    // 6. Button Setup
    pinMode(Config::BTN_PIN, INPUT_PULLUP);

    // 7. Screen Manager Setup
    // Note: ScreenManager does not have a setup() method in this version,
    // it inits on first use or via add().

    ScreenManager::getInstance().add(new ScreenStatus());
    ScreenManager::getInstance().add(new ScreenGameOfLife());
    ScreenManager::getInstance().add(new ScreenAnalogClock());
    ScreenManager::getInstance().add(new ScreenFlipClock());
    ScreenManager::getInstance().add(new ScreenPalette());

    ScreenManager::getInstance().switchTo(0);

    Logger::log("MAIN", "System Active. Loop Start.");
}

void loop()
{
    // 1. Watchdog
    WatchdogHAL::getInstance().feed();

    // 2. Services (BLE input retired in F1; ScreenBLEScan owns its own scan lifecycle)

    // 3. Timer Event
    if (timerFlag)
    {
        timerFlag = false;
        ScreenManager::getInstance().dispatchTimer();
    }

    // 4. Input (Navigation)
    // Simple Debounce for Button
    static unsigned long lastBtnPress = 0;
    if (digitalRead(Config::BTN_PIN) == LOW)
    {
        if (millis() - lastBtnPress > Config::BTN_DEBOUNCE)
        {
            lastBtnPress = millis();
            ScreenManager::getInstance().next();
        }
    }

    // 5. Screen Loop
    ScreenManager::getInstance().loop();
    // 5. Web Server
    WebService::getInstance().handleClient();
    delay(5);
}
