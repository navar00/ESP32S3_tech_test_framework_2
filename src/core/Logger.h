#pragma once
#include <Arduino.h>
#include "../core/Config.h"

class Logger
{
public:
    static void init()
    {
        Serial.begin(Config::SERIAL_BAUD);
        delay(500);
        Serial.println("\n--- [ SYSTEM LOG START ] ---");
    }

    static void log(const char *tag, const char *format, ...)
    {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        Serial.printf("[%8lu] [%-10s] %s\n", millis(), tag, buffer);
    }
};
