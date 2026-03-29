#pragma once
#include "IScreen.h"
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include "../core/Logger.h"
#include "BaseSprite.h"

class ScreenBLEScan : public IScreen
{
private:
    BLEScan *pBLEScan;
    BaseSprite display;
    const int SCAN_TIME = 5; // Seconds

    std::string getDeviceNature(BLEAdvertisedDevice &device)
    {
        // 1. HID Check
        if (device.isAdvertisingService(BLEUUID((uint16_t)0x1812)))
            return "HID/GAMEPAD";

        // Appearance Check
        if (device.haveAppearance())
        {
            uint16_t app = device.getAppearance();
            // HID Generic (960), Keyboard (961), Mouse (962), Joystick (963), Gamepad (964)
            if (app >= 960 && app <= 970)
                return "HID/GAMEPAD";
        }

        // 2. Audio/Phone
        if (device.isAdvertisingService(BLEUUID((uint16_t)0x180E)) ||
            device.isAdvertisingService(BLEUUID((uint16_t)0x110B)))
            return "AUDIO";

        // 3. Other known UUIDs could be added here

        return "UNK";
    }

    void displayResults(BLEScanResults &results)
    {
        if (!display.isReady())
            return;
        TFT_eSprite *sprite = display.getRaw();

        display.clear();
        display.drawHeader(" BLE RADAR (WiFi OFF) ");

        // Columns Header
        int y = 20;
        sprite->setTextColor(TFT_DARKGREEN, TFT_BLACK);
        sprite->drawString("MAC", 4, y, 2);
        sprite->drawString("RSSI", 90, y, 2);
        sprite->drawString("TYPE", 140, y, 2);
        sprite->drawString("NAME", 190, y, 2);

        y += 18;
        display.drawSeparator(y - 2);

        sprite->setTextColor(TFT_GREEN, TFT_BLACK);

        int count = results.getCount();
        int max_lines = 10;

        for (int i = 0; i < count; i++)
        {
            BLEAdvertisedDevice device = results.getDevice(i);
            int rssi = device.getRSSI();
            std::string name = device.getName();
            std::string macStr = device.getAddress().toString();
            std::string nature = getDeviceNature(device);

            // LOGGING
            Logger::log("BLE", "Found: %s | RSSI: %d | Nature: %s | Name: %s",
                        macStr.c_str(), rssi, nature.c_str(), name.empty() ? "(null)" : name.c_str());

            // UI Display
            if (i < max_lines)
            {
                // Compact MAC: Show last 2 bytes for space or full if small font
                // Using 2 bytes (XX:XX) from end
                const char *macRaw = macStr.c_str();
                // macStr is "XX:XX:XX:XX:XX:XX" (17 chars)
                const char *macDisplay = (macStr.length() > 5) ? (macRaw + 12) : macRaw;

                // Name Truncate
                const char *nameDisplay = name.empty() ? "-" : name.c_str();
                char nameBuf[9];
                snprintf(nameBuf, sizeof(nameBuf), "%.8s", nameDisplay);

                sprite->drawString(macDisplay, 4, y, 2);
                sprite->drawNumber(rssi, 90, y, 2);
                sprite->drawString(nature.c_str(), 140, y, 2);
                sprite->drawString(nameBuf, 190, y, 2);

                y += 18;
            }
        }

        // Footer
        int footerY = 220;
        display.drawSeparator(footerY);

        char buf[40];
        snprintf(buf, 40, "DEVICES: %d", count);
        sprite->drawString(buf, 4, footerY + 5, 2);
        sprite->drawString("BTN: EXIT", 160, footerY + 5, 2);

        display.push();
    }

public:
    const char *getName() override { return "BLE_SCANNER"; }

    void onEnter() override
    {
        IScreen::onEnter();

        Logger::log("BLE", "Entering Screen. Turning WiFi OFF.");
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        delay(100);

        // Traffic Light: Blue for BLE
        LedHAL::getInstance().set(0, 0, 255);

        // Init Engine
        display.init(DisplayHAL::getInstance().getRaw());

        // Init BLE
        // Note: BLEDevice::init is safe to call multiple times?
        // Usually yes, or check if initialized.
        // BLEDevice::getInitialized() is not standard, keeping internal static flag is safer.
        static bool bleInit = false;
        if (!bleInit)
        {
            BLEDevice::init(Config::SYS_NAME);
            bleInit = true;
        }

        pBLEScan = BLEDevice::getScan();
        pBLEScan->setActiveScan(true);
        pBLEScan->setInterval(100);
        pBLEScan->setWindow(99);

        startScan();
    }

    void onExit() override
    {
        Logger::log("BLE", "Exiting Screen. Stopping Scan. WiFi ON.");

        // Stop any ongoing scan (if we were async)
        pBLEScan->stop();
        pBLEScan->clearResults();

        display.free();

        // Restore WiFi
        WiFi.mode(WIFI_STA);
        // We don't connect here, just enable the radio as requested ("subir el wifi")
    }

    void onLoop() override {}

    void startScan()
    {
        if (!display.isReady())
            return;
        TFT_eSprite *sprite = display.getRaw();

        // Scanning State
        display.clear();
        display.drawHeader(" BLE RADAR ");

        int y = 60;
        sprite->setTextDatum(MC_DATUM); // Middle Center
        sprite->drawString("WiFi OFF", 120, y, 4);
        y += 40;
        sprite->drawString("SCANNING...", 120, y, 2);

        // Pseudo progress bar
        sprite->drawRect(40, 140, 160, 10, TFT_DARKGREY);

        sprite->setTextDatum(TL_DATUM); // Reset

        display.push();

        // Animation while blocking (not possible if blocking)
        // Check if we can do async next time. For now blocking.

        BLEScanResults foundDevices = pBLEScan->start(SCAN_TIME, false);
        displayResults(foundDevices);
    }
};
