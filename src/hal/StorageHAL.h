#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "../core/Config.h"
#include "../core/Logger.h"

class StorageHAL {
public:
    static StorageHAL& getInstance();
    void begin();

    void incrementBootCounter();
    uint32_t getBootCounter();
    
    void setSafeModeFlag(bool active);
    bool getSafeModeFlag();

    void updateLastBootTime();
    bool isQuickReboot(); // Detects if last reboot was < 60s ago

    // Geo Cache
    void saveGeo(const String& tz, long offset);
    bool loadGeo(String& tz, long& offset);

private:
    StorageHAL() = default;
    Preferences _prefs;
    const char* TAG = "StorageHAL";
};
