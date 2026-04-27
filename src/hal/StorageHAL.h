#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "../core/Config.h"
#include "../core/Logger.h"

class StorageHAL
{
public:
    static StorageHAL &getInstance();

    // Returns true if NVS opened successfully. Subsequent ops no-op if false.
    bool begin();
    bool isReady() const { return _ready; }

    void incrementBootCounter();
    uint32_t getBootCounter();

    void updateLastBootTime();

    // Geo Cache
    void saveGeo(const String &tz, long offset);
    bool loadGeo(String &tz, long &offset);

private:
    StorageHAL() = default;
    Preferences _prefs;
    bool _ready = false;
    static constexpr const char *TAG = "STG";
};
