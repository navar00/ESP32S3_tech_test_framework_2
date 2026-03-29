#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include "../core/Config.h"
#include "../core/Logger.h"

class NetService
{
public:
    bool connectBestRSSI(uint32_t timeoutMs);
    void disconnect();
    bool isConnected();
    String getIP();
    String getSSID();

private:
    const char *TAG = "NetService";
};
