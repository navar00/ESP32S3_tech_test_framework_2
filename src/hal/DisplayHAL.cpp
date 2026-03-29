#include "DisplayHAL.h"

DisplayHAL::DisplayHAL() : _tft() {}

DisplayHAL &DisplayHAL::getInstance()
{
    static DisplayHAL instance;
    return instance;
}

void DisplayHAL::init()
{
    Logger::log(TAG, "Initializing TFT (Virtuoso HAL)...");
    _tft.begin();
    _tft.setRotation(1); // Landscape
    _tft.fillScreen(TFT_BLACK);
    _tft.setTextFont(2);

    _width = _tft.width();
    _height = _tft.height();

    Logger::log(TAG, "Display Ready: %dx%d", _width, _height);
}

TFT_eSPI &DisplayHAL::getRaw()
{
    return _tft;
}

int16_t DisplayHAL::width() const { return _width; }
int16_t DisplayHAL::height() const { return _height; }

void DisplayHAL::clear()
{
    _tft.fillScreen(TFT_BLACK);
}

void DisplayHAL::fillScreen(uint32_t color)
{
    _tft.fillScreen(color);
}
