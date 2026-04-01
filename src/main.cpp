#include <Arduino.h>
#include "core/Config.h"
#include "core/Logger.h"
#include "core/ScreenManager.h"
#include "core/BootOrchestrator.h"
#include "hal/Hal.h"
#include "hal/InputHAL.h"

// Includes for Screens
#include "screens/ScreenStatus.h"
#include "screens/ScreenGameOfLife.h"
#include "screens/ScreenGamepad.h"
#include "screens/ScreenAnalogClock.h"
#include "screens/ScreenFlipClock.h"
#include "services/WebService.h"

// --- Global Context ---
volatile bool timerFlag = false;
hw_timer_t *timer = NULL;
BootOrchestrator bootOrchestrator;
// ScreenManager is a Singleton, accessed via getInstance()

// --- Interrupts ---
void IRAM_ATTR onTimer()
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

    // 3. Boot Orchestrator (Blocking)
    bootOrchestrator.run();

    // 4. Runtime Init
    LedHAL::getInstance().set(0, 255, 0); // Green Status
    // DESACTIVADO BLE: Nos centramos en WiFi
    // InputHAL::getInstance().init();       // Bluetooth Stack (re-init or first init)

    // Reduce Watchdog for tight loop
    WatchdogHAL::getInstance().begin(Config::WDT_TIMEOUT_LOOP);

    // 5. Timer Setup (1 Hz)
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 1000000, true);
    timerAlarmEnable(timer);

    // 6. Button Setup
    pinMode(Config::BTN_PIN, INPUT_PULLUP);

    // 7. Screen Manager Setup
    // Note: ScreenManager does not have a setup() method in this version,
    // it inits on first use or via add().

    ScreenManager::getInstance().add(new ScreenStatus());
    ScreenManager::getInstance().add(new ScreenGameOfLife());
    // DESACTIVADO BLE: No cargamos el Gamepad monitor
    // ScreenManager::getInstance().add(new ScreenGamepad());
    ScreenManager::getInstance().add(new ScreenAnalogClock());
    ScreenManager::getInstance().add(new ScreenFlipClock());

    ScreenManager::getInstance().switchTo(0);

    Logger::log("MAIN", "System Active. Loop Start.");
}

void loop()
{
    // 1. Watchdog
    WatchdogHAL::getInstance().feed();

    // 2. Services
    // DESACTIVADO BLE:
    // InputHAL::getInstance().update(); // Bluepad32

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
