#pragma once
#include "IScreen.h"
#include "BaseSprite.h"
#include <esp_mac.h>
#include "../services/TimeService.h"
#include <WiFi.h>

// Variable global (o de singleton) externa que gestiona el servicio, o usar la C-time lib directamente
// Por simplicidad, leeremos la hora del sistema ESP32 directamente ya que NTP la configuró a nivel SO.

class ScreenStatus : public IScreen
{
    bool pendingUpdate = true;
    BaseSprite display; // Composition over Inheritance for rendering engine

public:
    const char *getName() override { return "STATUS_SYS"; }

    void onEnter() override
    {
        IScreen::onEnter();

        // Traffic Light Standard: Status = Orange
        LedHAL::getInstance().set(255, 165, 0);

        // Log MAC Addresses for SixaxisPairTool
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        Logger::log("STATUS", "MAC WiFi: %s", macStr);

        // Init Engine
        display.init(DisplayHAL::getInstance().getRaw());

        // Force immediate update
        updateValues();
    }

    void onExit() override
    {
        display.free();
    }

    void onLoop() override
    {
        if (pendingUpdate)
        {
            updateValues();
            pendingUpdate = false;
        }
    }

    void on1SecTimer() override
    {
        pendingUpdate = true;
    }

private:
    void updateValues()
    {
        if (!display.isReady())
            return;
        TFT_eSprite *sprite = display.getRaw();

        display.clear(); // Standard Clear

        // --- Compact Header (Standardized) ---
        display.drawHeader(" SYSTEM MONITOR ");

        // --- Body (Density Mode: Font 2, Leading 18px) ---
        int y = 20;
        int x_col1 = 4;
        int x_val1 = 90;
        int line_h = 16; // Reduced to fit more info

        sprite->setTextColor(TFT_GREEN, TFT_BLACK);

        // Row 1
        sprite->drawString("Time:", x_col1, y, 2);

        // Retrieve safely formatted time from TimeService singleton (HH:MM:SS only)
        String timeStr = TimeService::getInstance().getTimeOnly();
        char buf[32];
        sprite->drawString(timeStr, x_val1, y, 2);

        // Row 2
        y += line_h;
        sprite->drawString("Uptime:", x_col1, y, 2);
        snprintf(buf, 32, "%lu s", millis() / 1000);
        sprite->drawString(buf, x_val1, y, 2);

        // Row 3
        y += line_h;
        sprite->drawString("CPU Freq:", x_col1, y, 2);
        snprintf(buf, 32, "%d MHz", ESP.getCpuFreqMHz());
        sprite->drawString(buf, x_val1, y, 2);

        // Row 4
        y += line_h;
        sprite->drawString("Int Temp:", x_col1, y, 2);
        float temp = temperatureRead();
        snprintf(buf, 32, "%.1f C", temp);
        sprite->drawString(buf, x_val1, y, 2);

        // ─── MEMORY Section (2-column table) ───
        y += line_h;
        display.drawSeparator(y + 4);
        sprite->setTextColor(TFT_DARKGREY, TFT_BLACK);
        sprite->drawString(" MEMORY ", 136, y, 1);
        sprite->setTextColor(TFT_GREEN, TFT_BLACK);
        y += line_h;

        int x_lbl2 = 162;
        int x_v2 = 228;

        sprite->drawString("Heap:", x_col1, y, 2);
        snprintf(buf, 32, "%d KB", ESP.getFreeHeap() / 1024);
        sprite->drawString(buf, 64, y, 2);
        sprite->drawString("Blk:", x_lbl2, y, 2);
        snprintf(buf, 32, "%d KB", ESP.getMaxAllocHeap() / 1024);
        sprite->drawString(buf, x_v2, y, 2);

        y += line_h;
        sprite->drawString("MinFr:", x_col1, y, 2);
        snprintf(buf, 32, "%d KB", ESP.getMinFreeHeap() / 1024);
        sprite->drawString(buf, 64, y, 2);
        sprite->drawString("Flash:", x_lbl2, y, 2);
        snprintf(buf, 32, "%d MB", ESP.getFlashChipSize() / (1024 * 1024));
        sprite->drawString(buf, x_v2, y, 2);

        // ─── NETWORK Section ───
        y += line_h;
        display.drawSeparator(y + 4);
        sprite->setTextColor(TFT_DARKGREY, TFT_BLACK);
        sprite->drawString(" NETWORK ", 133, y, 1);
        sprite->setTextColor(TFT_GREEN, TFT_BLACK);
        y += line_h;

        sprite->drawString("SSID:", x_col1, y, 2);
        if (WiFi.status() == WL_CONNECTED)
        {
            String ssid = WiFi.SSID();
            if (ssid.length() > 10)
                ssid = ssid.substring(0, 10) + "..";
            snprintf(buf, 32, "%s (%ddBm)", ssid.c_str(), WiFi.RSSI());
            sprite->drawString(buf, x_val1, y, 2);
        }
        else
        {
            sprite->drawString("--", x_val1, y, 2);
        }

        y += line_h;
        sprite->drawString("IP:", x_col1, y, 2);
        if (WiFi.status() == WL_CONNECTED)
        {
            sprite->drawString(WiFi.localIP().toString(), x_val1, y, 2);
        }
        else
        {
            sprite->drawString("Offline", x_val1, y, 2);
        }

        y += line_h;
        sprite->drawString("MAC:", x_col1, y, 2);
        sprite->drawString(WiFi.macAddress(), x_val1, y, 2);

        y += line_h;
        sprite->drawString("Web:", x_col1, y, 2);
        if (WiFi.status() == WL_CONNECTED)
        {
            snprintf(buf, 32, "%s [%s]",
                     WebService::getInstance().getURL().c_str(),
                     WebService::getInstance().getPin().c_str());
            sprite->drawString(buf, x_val1, y, 2);
        }
        else
        {
            sprite->drawString("--", x_val1, y, 2);
        }

        // Footer
        display.drawSeparator(225);

        // Blink cursor small
        static bool blink = false;
        sprite->drawString("> SYSTEM OK", 4, 228, 2);
        if (blink)
            sprite->fillRect(100, 228, 8, 14, TFT_GREEN);
        blink = !blink;

        display.push();
    }
};
