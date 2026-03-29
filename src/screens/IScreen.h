#pragma once
#include <Arduino.h>
#include "../hal/Hal.h"

class IScreen
{
public:
    virtual const char *getName() = 0;
    virtual void onEnter() { DisplayHAL::getInstance().clear(); }
    virtual void onLoop() = 0;
    virtual void onExit() {}
    virtual void on1SecTimer() {} // Called by timer interrupt context logic
    virtual ~IScreen() {}
};
