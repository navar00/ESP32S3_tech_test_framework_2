#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "esp_heap_caps.h"
#include "../core/Logger.h"

class BaseSprite
{
protected:
    TFT_eSprite *sprite = nullptr;

    // Heap snapshot helper: logs free + largest contiguous internal block
    static void logHeap(const char *stage)
    {
        Logger::log("GFX", "%s heap free=%u largest=%u",
                    stage,
                    (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                    (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
    }

public:
    BaseSprite() {}
    virtual ~BaseSprite() { free(); }

    // Init: Allocates memory
    void init(TFT_eSPI &tft)
    {
        if (sprite)
            return; // Already init

        // Required contiguous bytes per depth (240*320 = 76800 px).
        const size_t req16 = (size_t)tft.width() * tft.height() * 2; // ~150 KB
        const size_t req8 = (size_t)tft.width() * tft.height() * 1;  // ~75  KB
        const size_t req4 = (size_t)tft.width() * tft.height() / 2;  // ~38  KB
        const size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);

        // Dynamic Allocation Strategy
        sprite = new TFT_eSprite(&tft);

        // Try 16bpp only if largest contiguous block can fit it; else skip directly to 8bpp.
        bool allocated = false;
        if (largest >= req16)
        {
            logHeap("Alloc 16bpp pre");
            sprite->setColorDepth(16);
            if (sprite->createSprite(tft.width(), tft.height()) != nullptr)
            {
                allocated = true;
            }
            else
            {
                Logger::log("GFX", "Alloc 16bpp Failed. Trying 8bpp...");
            }
        }
        else
        {
            Logger::log("GFX", "Skip 16bpp (need %u, largest %u). Trying 8bpp...",
                        (unsigned)req16, (unsigned)largest);
        }

        if (!allocated)
        {
            // Try 8bpp (256 Colors) - Requires ~75KB
            logHeap("Alloc 8bpp pre");
            sprite->setColorDepth(8);
            if (sprite->createSprite(tft.width(), tft.height()) != nullptr)
            {
                allocated = true;
            }
            else
            {
                Logger::log("GFX", "Alloc 8bpp Failed. Trying 4bpp...");

                // Try 4bpp (16 Colors) - Requires ~38KB
                logHeap("Alloc 4bpp pre");
                sprite->setColorDepth(4);
                if (sprite->createSprite(tft.width(), tft.height()) != nullptr)
                {
                    allocated = true;
                }
                else
                {
                    Logger::log("GFX", "CRITICAL: Sprite Alloc Failed! (%dx%d) need4=%u largest=%u",
                                tft.width(), tft.height(), (unsigned)req4,
                                (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
                    delete sprite;
                    sprite = nullptr;
                    return;
                }
            }
        }
        (void)req8; // depth-8 size kept for documentation/future telemetry

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
