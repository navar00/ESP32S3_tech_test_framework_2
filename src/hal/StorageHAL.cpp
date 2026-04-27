#include "StorageHAL.h"

StorageHAL &StorageHAL::getInstance()
{
    static StorageHAL instance;
    return instance;
}

bool StorageHAL::begin()
{
    // Preferences::begin returns true on success. Namespace must be <=15 bytes.
    _ready = _prefs.begin(Config::NVS_NAMESPACE, false);
    if (!_ready)
    {
        Logger::log(TAG, "NVS begin failed (ns='%s'). Storage disabled.", Config::NVS_NAMESPACE);
    }
    return _ready;
}

void StorageHAL::incrementBootCounter()
{
    if (!_ready)
        return;
    uint32_t count = _prefs.getUInt(Config::NVS_KEY_BOOT_COUNT, 0);
    size_t written = _prefs.putUInt(Config::NVS_KEY_BOOT_COUNT, count + 1);
    if (written != sizeof(uint32_t))
    {
        Logger::log(TAG, "putUInt(boot_count) wrote %u (expected %u)",
                    (unsigned)written, (unsigned)sizeof(uint32_t));
        return;
    }
    Logger::log(TAG, "Boot Count: %u", count + 1);
}

uint32_t StorageHAL::getBootCounter()
{
    if (!_ready)
        return 0;
    return _prefs.getUInt(Config::NVS_KEY_BOOT_COUNT, 0);
}

void StorageHAL::updateLastBootTime()
{
    if (!_ready)
        return;
    size_t written = _prefs.putULong64(Config::NVS_KEY_LAST_BOOT, millis());
    if (written != sizeof(uint64_t))
    {
        Logger::log(TAG, "putULong64(last_boot) wrote %u (expected %u)",
                    (unsigned)written, (unsigned)sizeof(uint64_t));
    }
}

void StorageHAL::saveGeo(const String &tz, long offset)
{
    if (!_ready)
        return;
    size_t w1 = _prefs.putString(Config::NVS_KEY_TZ, tz);
    size_t w2 = _prefs.putLong(Config::NVS_KEY_OFFSET, offset);
    if (w1 == 0 || w2 != sizeof(long))
    {
        Logger::log(TAG, "saveGeo partial write tz=%u off=%u",
                    (unsigned)w1, (unsigned)w2);
        return;
    }
    Logger::log(TAG, "Geo Cached: %s (Offset: %ld)", tz.c_str(), offset);
}

bool StorageHAL::loadGeo(String &tz, long &offset)
{
    if (!_ready)
        return false;
    if (!_prefs.isKey(Config::NVS_KEY_TZ))
        return false;
    tz = _prefs.getString(Config::NVS_KEY_TZ);
    offset = _prefs.getLong(Config::NVS_KEY_OFFSET, 0);
    return true;
}
