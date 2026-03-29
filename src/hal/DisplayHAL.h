#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h> // Library dependency
#include "../core/Logger.h"

class DisplayHAL
{
public:
    static DisplayHAL &getInstance();

    // Lifecycle
    void init(); // Renamed from begin() to match previous API

    // Accessors
    TFT_eSPI &getRaw(); // Renamed from getTFT() to match previous API
    int16_t width() const;
    int16_t height() const;

    // Utils
    void clear();
    void fillScreen(uint32_t color);

private:
    DisplayHAL();
    TFT_eSPI _tft;
    int16_t _width{0}, _height{0};
    const char *TAG = "DisplayHAL";
};
