#include "NetService.h"

bool NetService::connectBestRSSI(uint32_t timeoutMs)
{
    Logger::log(TAG, "Scanning for networks...");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    int n = WiFi.scanNetworks();
    if (n == 0)
    {
        Logger::log(TAG, "No networks found");
        return false;
    }

    // Find best match in Config
    int bestIndex = -1;
    int bestRSSI = -1000;
    const char *targetSSID = nullptr;
    const char *targetPass = nullptr;

    for (int i = 0; i < n; ++i)
    {
        String scanSSID = WiFi.SSID(i);
        for (int j = 0; j < Config::WIFI_NETWORK_COUNT; ++j)
        {
            if (scanSSID.equals(Config::WIFI_NETWORKS[j].ssid))
            {
                if (WiFi.RSSI(i) > bestRSSI)
                {
                    bestRSSI = WiFi.RSSI(i);
                    targetSSID = Config::WIFI_NETWORKS[j].ssid;
                    targetPass = Config::WIFI_NETWORKS[j].password;
                }
            }
        }
    }

    if (!targetSSID)
    {
        Logger::log(TAG, "No known networks found");
        return false;
    }

    Logger::log(TAG, "Connecting to %s (RSSI: %d)...", targetSSID, bestRSSI);
    WiFi.begin(targetSSID, targetPass);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        if (millis() - start > timeoutMs)
        {
            Logger::log(TAG, "Connection timeout");
            return false;
        }
        delay(100);
    }

    Logger::log(TAG, "Connected! IP: %s", WiFi.localIP().toString().c_str());
    return true;
}

void NetService::disconnect()
{
    WiFi.disconnect(true);
}

bool NetService::isConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

String NetService::getIP()
{
    return WiFi.localIP().toString();
}

String NetService::getSSID()
{
    return WiFi.SSID();
}
