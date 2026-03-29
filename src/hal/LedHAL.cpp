#include "LedHAL.h"
#include "../core/Logger.h"

LedHAL::LedHAL() : _strip(Config::LED_COUNT, Config::LED_PIN, NEO_GRB + NEO_KHZ800) {}

LedHAL &LedHAL::getInstance()
{
    static LedHAL instance;
    return instance;
}

void LedHAL::init()
{
    _strip.begin();
    _strip.setBrightness(13); // 5%
    _strip.show();
    Logger::log(TAG, "LED Strip Init");
}

void LedHAL::set(uint8_t r, uint8_t g, uint8_t b)
{
    _strip.setPixelColor(0, _strip.Color(r, g, b));
    _strip.show();
}
