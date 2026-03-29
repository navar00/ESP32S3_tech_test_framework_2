#pragma once
#include <Arduino.h>

// ╔════════════════════════════════════════════════════════════╗
// ║  TEMPLATE — Copy this file as Config.h and fill in your  ║
// ║  WiFi credentials. Config.h is in .gitignore.            ║
// ╚════════════════════════════════════════════════════════════╝

namespace Config
{
    constexpr char SYS_NAME[] = "TechTest_v1";
    constexpr char SYS_VERSION[] = "1.0.0-ble";
    constexpr int LED_PIN = 38;
    constexpr int LED_COUNT = 1;
    constexpr long SERIAL_BAUD = 115200;

    // Peripherals
    constexpr int BTN_PIN = 0;             // BOOT Button
    constexpr uint32_t BTN_DEBOUNCE = 250; // ms

    // WiFi Configuration — FILL IN YOUR CREDENTIALS
    struct WifiAP
    {
        const char *ssid;
        const char *password;
    };
    constexpr WifiAP WIFI_NETWORKS[] = {
        {"YOUR_SSID_1", "YOUR_PASSWORD_1"},
        {"YOUR_SSID_2", "YOUR_PASSWORD_2"}};
    constexpr int WIFI_NETWORK_COUNT = sizeof(WIFI_NETWORKS) / sizeof(WifiAP);

    // NVS Namespaces & Keys
    constexpr char NVS_NAMESPACE[] = "virtuoso";
    constexpr char NVS_KEY_BOOT_COUNT[] = "boot_count";
    constexpr char NVS_KEY_LAST_BOOT[] = "last_boot";
    constexpr char NVS_KEY_TZ[] = "geo_tz";
    constexpr char NVS_KEY_OFFSET[] = "geo_off";

    // Watchdog
    constexpr uint32_t WDT_TIMEOUT_BOOT = 8000; // 8s during boot
    constexpr uint32_t WDT_TIMEOUT_LOOP = 3000; // 3s runtime
}
