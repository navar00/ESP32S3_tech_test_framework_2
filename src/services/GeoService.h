#pragma once
#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "../core/Logger.h"
#include "../hal/StorageHAL.h"

class GeoService
{
public:
    bool fetchAndApply();
    String getTimezone();
    long getOffset();

private:
    String _tz = "UTC";
    long _offset = 0;
    const char *TAG = "GeoService";
};
