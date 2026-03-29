#pragma once
#include "../screens/IScreen.h"
#include "../hal/DisplayHAL.h"
#include <vector>

class BootConsoleScreen : public IScreen
{
public:
    // IScreen Implementation
    const char *getName() override { return "BOOT"; }
    void onLoop() override {}
    void onEnter() override {}
    void onExit() override {}

    // Custom
    // void update() override; // Removed
    void draw(TFT_eSprite *sprite);

    // Custom controls
    void setProgress(uint8_t p, const char *status);
    void appendLog(const char *msg);

private:
    uint8_t _progress = 0;
    String _currentStep = "BOOT";
    std::vector<String> _logs;
    const int MAX_LOGS = 18;
};
