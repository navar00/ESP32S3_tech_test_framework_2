#include "BootConsoleScreen.h"

// Removed update() implementation because it is now defined inline in header

void BootConsoleScreen::draw(TFT_eSprite *sprite)
{
    sprite->fillScreen(TFT_BLACK);

    // ─── Layout constants (320×240) ───
    const int BAR_X = 4;   // Vertical bar left edge
    const int BAR_W = 10;  // Vertical bar width
    const int BAR_Y = 4;   // Top of bar
    const int BAR_H = 232; // Bar height (almost full screen)
    const int LOG_X = 22;  // Log text left edge
    const int LOG_Y = 4;   // First log line Y
    const int LINE_H = 12; // Line spacing (Font 1 ~8px + gap)

    // ─── Vertical Progress Bar (left edge) ───
    sprite->drawRect(BAR_X, BAR_Y, BAR_W, BAR_H, TFT_DARKGREEN);
    int filled = (BAR_H - 4) * _progress / 100;
    // Fill from top down
    if (filled > 0)
    {
        sprite->fillRect(BAR_X + 2, BAR_Y + 2, BAR_W - 4, filled, TFT_GREEN);
    }

    // ─── Log area (right of bar, full height) ───
    sprite->setTextFont(1); // 8px monospace
    int y = LOG_Y;

    // Title line
    sprite->setTextColor(TFT_GREEN, TFT_BLACK);
    sprite->setCursor(LOG_X, y);
    sprite->print("BOOT ORCHESTRATOR");
    y += LINE_H + 2;

    // Separator
    sprite->drawFastHLine(LOG_X, y, 290, TFT_DARKGREEN);
    y += 4;

    // Step lines (incremental log)
    for (size_t i = 0; i < _logs.size(); i++)
    {
        // Prefix with > for current (last) step, . for completed
        bool isCurrent = (i == _logs.size() - 1 && _progress < 100);
        if (isCurrent)
        {
            sprite->setTextColor(TFT_GREEN, TFT_BLACK);
            sprite->setCursor(LOG_X, y);
            sprite->printf("> %s", _logs[i].c_str());
        }
        else
        {
            sprite->setTextColor(TFT_DARKGREEN, TFT_BLACK);
            sprite->setCursor(LOG_X, y);
            sprite->printf("  %s", _logs[i].c_str());
        }
        y += LINE_H;
    }

    // ─── Footer: step name + percentage ───
    int footY = BAR_Y + BAR_H - 10;
    sprite->setTextColor(TFT_DARKGREEN, TFT_BLACK);
    char footBuf[40];
    snprintf(footBuf, sizeof(footBuf), "[%s] %d%%", _currentStep.c_str(), _progress);
    sprite->drawRightString(footBuf, 316, footY, 1);
}

void BootConsoleScreen::setProgress(uint8_t p, const char *status)
{
    _progress = p;
    _currentStep = String(status);
}

void BootConsoleScreen::appendLog(const char *msg)
{
    if (_logs.size() >= MAX_LOGS)
    {
        _logs.erase(_logs.begin());
    }
    _logs.push_back(String(msg));
}
