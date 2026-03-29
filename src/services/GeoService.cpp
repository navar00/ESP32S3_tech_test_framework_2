#include "GeoService.h"

bool GeoService::fetchAndApply()
{
    HTTPClient http;
    Logger::log(TAG, "Querying ip-api.com...");

    // Use IP-API to get location info based on IP
    if (http.begin("http://ip-api.com/json/?fields=timezone,offset"))
    {
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK)
        {
            String payload = http.getString();

            // Parse JSON
            StaticJsonDocument<512> doc;
            DeserializationError error = deserializeJson(doc, payload);

            if (!error)
            {
                _tz = doc["timezone"].as<String>();
                _offset = doc["offset"].as<long>();

                Logger::log(TAG, "Geo Success: %s, Offset: %ld", _tz.c_str(), _offset);

                // Cache it
                StorageHAL::getInstance().saveGeo(_tz, _offset);
                http.end();
                return true;
            }
            else
            {
                Logger::log(TAG, "JSON Parse Error");
            }
        }
        else
        {
            Logger::log(TAG, "HTTP Error: %d", httpCode);
        }
        http.end();
    }
    else
    {
        Logger::log(TAG, "Unable to connect");
    }

    // Fallback to cache
    Logger::log(TAG, "Attempting Fallback to Cache...");
    if (StorageHAL::getInstance().loadGeo(_tz, _offset))
    {
        Logger::log(TAG, "Restored from Cache: %s", _tz.c_str());
        return true;
    }

    return false;
}

String GeoService::getTimezone()
{
    return _tz;
}

long GeoService::getOffset()
{
    return _offset;
}
