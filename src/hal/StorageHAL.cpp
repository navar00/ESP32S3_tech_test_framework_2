#include "StorageHAL.h"

StorageHAL& StorageHAL::getInstance() {
    static StorageHAL instance;
    return instance;
}

void StorageHAL::begin() {
    _prefs.begin(Config::NVS_NAMESPACE, false);
}

void StorageHAL::incrementBootCounter() {
    uint32_t count = _prefs.getUInt(Config::NVS_KEY_BOOT_COUNT, 0);
    _prefs.putUInt(Config::NVS_KEY_BOOT_COUNT, count + 1);
    Logger::log(TAG, "Boot Count: %u", count + 1);
}

uint32_t StorageHAL::getBootCounter() {
    return _prefs.getUInt(Config::NVS_KEY_BOOT_COUNT, 0);
}

void StorageHAL::updateLastBootTime() {
    _prefs.putULong64(Config::NVS_KEY_LAST_BOOT, millis());
}

bool StorageHAL::isQuickReboot() {
    // Note: millis() resets on reboot. We can't use absolute time without RTC.
    // Instead we rely on restart counters or verify if we have a way to know REAL time.
    // If we don't have RTC, this "QuickReboot" check is tricky solely with NVS unless 
    // we saved a timestamp from *external* time source. 
    // For now, we will assume we check against a "uptime" saved at shutdown if graceful,
    // or rely on the Reset Reason (Exception/Panic).
    // Simplified strategy: We count consecutive crashes.
    // TODO: Implement crash counter logic if needed. 
    return false; 
}

void StorageHAL::saveGeo(const String& tz, long offset) {
    _prefs.putString(Config::NVS_KEY_TZ, tz);
    _prefs.putLong(Config::NVS_KEY_OFFSET, offset);
    Logger::log(TAG, "Geo Cached: %s (Offset: %ld)", tz.c_str(), offset);
}

bool StorageHAL::loadGeo(String& tz, long& offset) {
    if (!_prefs.isKey(Config::NVS_KEY_TZ)) return false;
    tz = _prefs.getString(Config::NVS_KEY_TZ);
    offset = _prefs.getLong(Config::NVS_KEY_OFFSET, 0);
    return true;
}
