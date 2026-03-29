#pragma once
#include "IScreen.h"
#include "../hal/InputHAL.h"
#include "BaseSprite.h"

extern "C"
{
#include <uni.h>
}

// Mode: MONITOR
// Shows the status of connected Bluepad32 controllers (Sixaxis/DS4)

class ScreenGamepad : public IScreen
{
private:
    BaseSprite display;

    // Helper to check connection
    bool isConnected(uni_hid_device_t *d)
    {
        return (d != nullptr); // In native, if we have the pointer in InputHAL, it is connected
    }

public:
    const char *getName() override { return "GAMEPAD_MONITOR"; }

    void onEnter() override
    {
        IScreen::onEnter();
        display.init(DisplayHAL::getInstance().getRaw());
        LedHAL::getInstance().set(0, 0, 255); // Blue for Gamepad
    }

    void onExit() override
    {
        display.free();
    }

    void onLoop() override
    {
        if (!display.isReady())
            return;
        TFT_eSprite *sprite = display.getRaw();

        display.clear();
        display.drawHeader(" CONTROLLER STATUS ");

        uni_hid_device_t *myCtl = nullptr;
        // Find first valid controller
        InputHAL &input = InputHAL::getInstance();
        for (int i = 0; i < BP32_MAX_CONTROLLERS; i++)
        {
            if (input.controllers[i] != nullptr)
            {
                myCtl = input.controllers[i];
                break;
            }
        }

        int y = 30;
        sprite->setTextColor(TFT_GREEN, TFT_BLACK);

        if (myCtl)
        {
            char buf[256];
            snprintf(buf, sizeof(buf), "Model: %s", myCtl->name);
            sprite->drawString(buf, 4, y, 2);
            y += 20;

            snprintf(buf, sizeof(buf), "Battery: %d %%", myCtl->controller.battery);
            sprite->drawString(buf, 4, y, 2);
            y += 20;

            // Axis dump
            snprintf(buf, sizeof(buf), "LX:%4d LY:%4d", myCtl->controller.gamepad.axis_x, myCtl->controller.gamepad.axis_y);
            sprite->drawString(buf, 4, y, 2);
            y += 20;

            snprintf(buf, sizeof(buf), "RX:%4d RY:%4d", myCtl->controller.gamepad.axis_rx, myCtl->controller.gamepad.axis_ry);
            sprite->drawString(buf, 4, y, 2);
            y += 20;

            snprintf(buf, sizeof(buf), "Buttons: 0x%04X", myCtl->controller.gamepad.buttons);
            sprite->drawString(buf, 4, y, 2);
            y += 20;

            if (myCtl->controller.gamepad.buttons & BUTTON_A)
                sprite->fillCircle(200, 100, 10, TFT_RED);
            if (myCtl->controller.gamepad.buttons & BUTTON_B)
                sprite->fillCircle(230, 100, 10, TFT_YELLOW);
            if (myCtl->controller.gamepad.buttons & BUTTON_X)
                sprite->fillCircle(200, 70, 10, TFT_BLUE);
            if (myCtl->controller.gamepad.buttons & BUTTON_Y)
                sprite->fillCircle(230, 70, 10, TFT_GREEN);
        }
        else
        {
            sprite->drawString("NO CONTROLLER", 60, 100, 4);
            sprite->drawString("Searching...", 80, 140, 2);

            String mac = "MAC: " + InputHAL::getInstance().getLocalMacAddress();
            sprite->drawString(mac, 60, 170, 2);

            // Show MAC Address helper
            sprite->drawString("Use SixaxisPairTool", 20, 200, 2);
        }

        display.drawSeparator(220);
        display.push();
    }
};
