#include "TimeService.h"
#include "../hal/Hal.h"

void TimeService::sync(long offsetSec)
{
    _offset = offsetSec;

    Logger::log(TAG, "Iniciando peticion NTP (Offset: %ld)...", offsetSec);
    Logger::log(TAG, "Servidores: pool.ntp.org, time.nist.gov");

    // configTime with 0 offset, handling it manually in getFormattedTime
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    struct tm timeinfo;
    int retries = 0;
    const int MAX_RETRIES = 20; // Hasta 10 segundos (20 * 500ms)

    Logger::log(TAG, "Esperando sincronizacion NTP...");

    while (retries < MAX_RETRIES)
    {
        // getLocalTime checks if time is set (year > 2016)
        // param: timeout en ms que el ESP espera para re-checkear la flag
        Logger::log(TAG, "NTP Check Intento #%d/%d...", retries + 1, MAX_RETRIES);

        if (getLocalTime(&timeinfo, 500))
        {
            _lastSyncMillis = millis();
            Logger::log(TAG, "NTP Sync EXITO en intento %d!", retries + 1);
            Logger::log(TAG, "Hora seteada: %s", getFormattedTime().c_str());
            return;
        }

        retries++;
        // Hacer un delay manual para no saturar y forzar a que pasen al menos 500ms
        delay(500);
        // Alimentar watchdog porque este loop puede durar hasta 10 segundos
        WatchdogHAL::getInstance().feed();
    }

    Logger::log(TAG, "ERROR: NTP Sync Timeout tras %d intentos.", MAX_RETRIES);
}

String TimeService::getFormattedTime()
{
    struct tm timeinfo;

    // getLocalTime es la forma más segura del core de Arduino/ESP32
    if (!getLocalTime(&timeinfo, 10))
    {
        return "N/A";
    }

    // CRITICAL: En ESP32-S3 time_t es 64-bit pero el SDK puede escribir solo 32-bit,
    // dejando basura en los 32-bit altos. Truncamos a 32-bit para seguridad.
    time_t now = 0;
    time(&now);
    uint32_t safeNow = (uint32_t)now;

    if (safeNow < 100000)
        return "N/A";

    // Sumar el offset (en segundos)
    time_t local_now = (time_t)safeNow + _offset;
    struct tm local_tm;
    gmtime_r(&local_now, &local_tm);

    if (local_tm.tm_year < (2016 - 1900))
    {
        return "N/A";
    }

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
             local_tm.tm_year + 1900, local_tm.tm_mon + 1, local_tm.tm_mday, local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec);

    return String(buffer);
}

String TimeService::getTimeOnly()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 10))
    {
        return "N/A";
    }

    time_t now = 0;
    time(&now);
    uint32_t safeNow = (uint32_t)now;

    if (safeNow < 100000)
        return "N/A";

    time_t local_now = (time_t)safeNow + _offset;
    struct tm local_tm;
    gmtime_r(&local_now, &local_tm);

    if (local_tm.tm_year < (2016 - 1900))
    {
        return "N/A";
    }

    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d",
             local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec);

    return String(buffer);
}

bool TimeService::getTimeParts(int &hour, int &min, int &sec)
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 10))
        return false;

    time_t now = 0;
    time(&now);
    uint32_t safeNow = (uint32_t)now;
    if (safeNow < 100000)
        return false;

    time_t local_now = (time_t)safeNow + _offset;
    struct tm local_tm;
    gmtime_r(&local_now, &local_tm);

    if (local_tm.tm_year < (2016 - 1900))
        return false;

    hour = local_tm.tm_hour;
    min = local_tm.tm_min;
    sec = local_tm.tm_sec;
    return true;
}

bool TimeService::getDateParts(int &year, int &month, int &day, int &wday)
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 10))
        return false;

    time_t now = 0;
    time(&now);
    uint32_t safeNow = (uint32_t)now;
    if (safeNow < 100000)
        return false;

    time_t local_now = (time_t)safeNow + _offset;
    struct tm local_tm;
    gmtime_r(&local_now, &local_tm);

    if (local_tm.tm_year < (2016 - 1900))
        return false;

    year = local_tm.tm_year + 1900;
    month = local_tm.tm_mon + 1;
    day = local_tm.tm_mday;
    wday = local_tm.tm_wday; // 0=Sun, 1=Mon,...6=Sat
    return true;
}
unsigned long TimeService::getSecsSinceSync()
{
    if (_lastSyncMillis == 0)
        return 0;
    return (millis() - _lastSyncMillis) / 1000;
}

unsigned long TimeService::getSecsUntilNextSync()
{
    unsigned long elapsed = getSecsSinceSync();
    if (elapsed >= NTP_SYNC_INTERVAL)
        return 0;
    return NTP_SYNC_INTERVAL - elapsed;
}