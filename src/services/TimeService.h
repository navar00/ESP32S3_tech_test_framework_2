#pragma once
#include <Arduino.h>
#include "time.h"
#include "../core/Logger.h"

class TimeService
{
public:
    static TimeService &getInstance()
    {
        static TimeService instance;
        return instance;
    }

    void sync(long offsetSec);
    String getFormattedTime();
    String getTimeOnly();
    bool getTimeParts(int &hour, int &min, int &sec);
    bool getDateParts(int &year, int &month, int &day, int &wday);
    unsigned long getSecsSinceSync();
    unsigned long getSecsUntilNextSync();

    static const unsigned long NTP_SYNC_INTERVAL = 3600; // ESP32 SNTP default: 1 hour

private:
    TimeService() {} // Constructor privado
    long _offset = 0;
    unsigned long _lastSyncMillis = 0;
    const char *TAG = "TimeService";
};
