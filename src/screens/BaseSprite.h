#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "../core/Logger.h"

class BaseSprite
{
protected:
    TFT_eSprite *sprite = nullptr;

public:
    BaseSprite() {}
    virtual ~BaseSprite() { free(); }

    // Init: Allocates memory
    void init(TFT_eSPI &tft)
    {
        if (sprite)
            return; // Already init

        // Dynamic Allocation Strategy
        sprite = new TFT_eSprite(&tft);

        // Try 16bpp (High Color) - Requires ~150KB
        sprite->setColorDepth(16);
        if (sprite->createSprite(tft.width(), tft.height()) == nullptr)
        {
            Logger::log("GFX", "Alloc 16bpp Failed. Trying 8bpp...");

            // Try 8bpp (256 Colors) - Requires ~75KB
            sprite->setColorDepth(8);
            if (sprite->createSprite(tft.width(), tft.height()) == nullptr)
            {
                Logger::log("GFX", "Alloc 8bpp Failed. Trying 4bpp...");

                // Try 4bpp (16 Colors) - Requires ~38KB
                sprite->setColorDepth(4);
                if (sprite->createSprite(tft.width(), tft.height()) == nullptr)
                {
                    Logger::log("GFX", "CRITICAL: Sprite Alloc Failed! (%dx%d)", tft.width(), tft.height());
                    delete sprite;
                    sprite = nullptr;
                    return;
                }
            }
        }

        // 8bpp uses native 332 color mapping (256 colors).
        // Do NOT call createPalette() — a limited palette causes
        // out-of-range index lookups with standard RGB565 colors.

        sprite->setTextFont(2);
    }

    // Free: Releases memory
    void free()
    {
        if (sprite)
        {
            sprite->deleteSprite();
            delete sprite;
            sprite = nullptr;
        }
    }

    // Drawing Helpers standardizing the Retro UX
    void clear()
    {
        if (sprite)
            sprite->fillSprite(TFT_BLACK);
    }

    void push()
    {
        if (sprite)
            sprite->pushSprite(0, 0);
    }

    void drawHeader(const char *title)
    {
        if (!sprite)
            return;
        sprite->setTextColor(TFT_BLACK, TFT_GREEN);
        sprite->fillRect(0, 0, sprite->width(), 16, TFT_GREEN);
        sprite->drawString(title, 2, 0, 2);
        // Reset to standard data style
        sprite->setTextColor(TFT_GREEN, TFT_BLACK);
    }

    void drawSeparator(int y)
    {
        if (!sprite)
            return;
        sprite->drawFastHLine(0, y, sprite->width(), TFT_DARKGREEN);
    }

    // Get Raw Access if needed (Discouraged)
    TFT_eSprite *getRaw() { return sprite; }

    // Safety check
    bool isReady() { return sprite != nullptr; }
};
