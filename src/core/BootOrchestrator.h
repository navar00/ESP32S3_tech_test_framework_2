#pragma once
#include <Arduino.h>
#include "../screens/BootConsoleScreen.h"
#include "../hal/DisplayHAL.h"
#include "../hal/WatchdogHAL.h"
#include "../hal/StorageHAL.h"
#include "../services/NetService.h"
#include "../services/GeoService.h"
#include "../services/TimeService.h"
#include "../services/WebService.h"

class BootOrchestrator
{
public:
    void run();

private:
    BootConsoleScreen _console;
    NetService _net;
    GeoService _geo;
    TFT_eSprite *_sprite = nullptr;

    // Used to render the console during blocking operations
    void refreshUI();
    void logToUI(const char *fmt, ...);
};
