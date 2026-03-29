#pragma once
#include <Arduino.h>
#include "../core/Logger.h"

extern "C"
{
#include <uni.h>
}

#define BP32_MAX_CONTROLLERS 4

class InputHAL
{
private:
    static InputHAL *instance;
    InputHAL(); // Private Constructor

public:
    static InputHAL &getInstance();

    uni_hid_device_t *controllers[BP32_MAX_CONTROLLERS];

    void init();
    void update();
    void printAddress();
    String getLocalMacAddress();

    // Callbacks needed by C glue code
    void onConnected(uni_hid_device_t *d);
    void onDisconnected(uni_hid_device_t *d);
};
