#pragma once
#include <vector>
#include <Arduino.h>
#include "../screens/IScreen.h"
#include "Logger.h"

class ScreenManager
{
private:
    std::vector<IScreen *> screens;
    int currentIndex = -1;
    IScreen *currentScreen = nullptr;

    ScreenManager() {} // Singleton private constructor

public:
    static ScreenManager &getInstance()
    {
        static ScreenManager instance;
        return instance;
    }

    void add(IScreen *screen)
    {
        screens.push_back(screen);
        Logger::log("MGR", "Added screen: %s", screen->getName());
    }

    void switchTo(int index)
    {
        if (screens.empty())
            return;

        // Wrap around
        if (index >= (int)screens.size())
            index = 0;
        if (index < 0)
            index = screens.size() - 1;

        if (currentScreen)
        {
            currentScreen->onExit();
        }

        currentIndex = index;
        currentScreen = screens[currentIndex];

        Logger::log("MGR", "Switching to: %s", currentScreen->getName());
        currentScreen->onEnter();
    }

    void next()
    {
        switchTo(currentIndex + 1);
    }

    void dispatchTimer()
    {
        if (currentScreen)
        {
            currentScreen->on1SecTimer();
        }
    }

    void loop()
    {
        if (currentScreen)
        {
            currentScreen->onLoop();
        }
    }

    IScreen *getCurrent() { return currentScreen; }

    IScreen *getScreenByName(String name)
    {
        for (IScreen *s : screens)
        {
            if (String(s->getName()) == name)
                return s;
        }
        return nullptr;
    }
};
