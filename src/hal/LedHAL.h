#pragma once
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "../core/Config.h"

class LedHAL
{
public:
    static LedHAL &getInstance();
    void init();
    void set(uint8_t r, uint8_t g, uint8_t b);

private:
    LedHAL();
    Adafruit_NeoPixel _strip;
    const char *TAG = "LedHAL";
};
