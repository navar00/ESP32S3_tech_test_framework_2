#pragma once
#include "IScreen.h"
#include "BaseSprite.h"
#include "../services/TimeService.h"

/**
 * @class ScreenFlipClock
 * @brief Flip-clock style time display (HH:MM, 24h).
 *
 * DESIGN:
 * - Custom 7-segment rendering via fillRect (no font scaling issues).
 * - Works perfectly on 8bpp palettized sprites.
 * - Only HH:MM shown as oversized digits; seconds as small text.
 * - Blinking colon separator every second.
 * - NTP resync countdown at the bottom.
 * - "Retro Terminal" aesthetic: green segments on dark panels.
 */
class ScreenFlipClock : public IScreen
{
private:
    BaseSprite display;
    bool pendingUpdate = true;
    bool colonVisible = true;
    int prevH = -1, prevM = -1;

    // ─── Layout Constants (320×240, HH:MM only) ───
    static const int PANEL_W = 72;   // Width of each digit panel
    static const int PANEL_H = 180;  // Height of each digit panel
    static const int PANEL_R = 6;    // Corner radius
    static const int PANEL_Y = 28;   // Y position (vertically centered)
    static const int GAP = 2;        // Gap between panels in a pair
    static const int COLON_GAP = 14; // Space for the colon separator

    // 7-segment digit dimensions (inside panel)
    static const int DIG_W = 56;  // Digit bounding box width
    static const int DIG_H = 160; // Digit bounding box height
    static const int SEG_T = 12;  // Segment thickness

public:
    const char *getName() override { return "FLIP_CLOCK"; }

    void onEnter() override
    {
        IScreen::onEnter();
        display.init(DisplayHAL::getInstance().getRaw());
        LedHAL::getInstance().set(0, 80, 80);
        prevH = prevM = -1;
        pendingUpdate = true;
    }

    void onExit() override
    {
        display.free();
    }

    void on1SecTimer() override
    {
        pendingUpdate = true;
        colonVisible = !colonVisible;
    }

    void onLoop() override
    {
        if (!pendingUpdate)
            return;
        pendingUpdate = false;

        if (!display.isReady())
            return;

        int hh = 0, mm = 0, ss = 0;
        bool timeOk = TimeService::getInstance().getTimeParts(hh, mm, ss);

        TFT_eSprite *spr = display.getRaw();
        display.clear();

        // ─── Header (muted green bar, black text, left-aligned like other screens) ───
        spr->fillRect(0, 0, 320, 16, TFT_DARKGREEN);
        spr->setTextColor(TFT_BLACK, TFT_DARKGREEN);
        spr->drawString(" FLIP CLOCK ", 2, 0, 2);
        spr->setTextColor(TFT_GREEN, TFT_BLACK);

        if (timeOk)
        {
            // ─── Centered layout: [D1][D2] : [D3][D4] ───
            int pairW = PANEL_W * 2 + GAP;
            int totalW = pairW * 2 + COLON_GAP;
            int startX = (320 - totalW) / 2;

            int xHH = startX;
            int xColonX = startX + pairW + COLON_GAP / 2;
            int xMM = startX + pairW + COLON_GAP;

            // Draw 4 digit panels
            drawDigitPanel(spr, xHH, PANEL_Y, hh / 10);
            drawDigitPanel(spr, xHH + PANEL_W + GAP, PANEL_Y, hh % 10);
            drawDigitPanel(spr, xMM, PANEL_Y, mm / 10);
            drawDigitPanel(spr, xMM + PANEL_W + GAP, PANEL_Y, mm % 10);

            // Blinking colon
            drawColon(spr, xColonX, PANEL_Y, colonVisible);

            prevH = hh;
            prevM = mm;

            // ─── Footer: :SS centered + NTP right-aligned ───
            int footY = PANEL_Y + PANEL_H + 4;
            spr->setTextColor(TFT_DARKGREEN, TFT_BLACK);

            char secBuf[8];
            snprintf(secBuf, sizeof(secBuf), ":%02d", ss);
            spr->drawCentreString(secBuf, 160, footY, 2);

            unsigned long secsLeft = TimeService::getInstance().getSecsUntilNextSync();
            int cMin = secsLeft / 60;
            int cSec = secsLeft % 60;
            char ntpBuf[32];
            snprintf(ntpBuf, sizeof(ntpBuf), "NTP %02d:%02d", cMin, cSec);
            spr->drawRightString(ntpBuf, 316, footY + 4, 1);
        }
        else
        {
            spr->setTextColor(TFT_DARKGREEN, TFT_BLACK);
            spr->drawCentreString("Syncing NTP...", 160, 110, 4);
        }

        display.push();
    }

private:
    // ─── Draw a single digit inside a flip-clock panel ───
    void drawDigitPanel(TFT_eSprite *spr, int px, int py, int digit)
    {
        // 7-segment encoding: bits = gfedcba
        static const uint8_t SEGS[10] = {
            0x3F, 0x06, 0x5B, 0x4F, 0x66, // 0-4
            0x6D, 0x7D, 0x07, 0x7F, 0x6F  // 5-9
        };

        int halfH = PANEL_H / 2;

        // Panel background: top half lighter, bottom half darker
        spr->fillRoundRect(px, py, PANEL_W, PANEL_H, PANEL_R, 0x1082);
        spr->fillRect(px + 1, py, PANEL_W - 2, halfH, 0x18C3);
        // Re-round top corners
        spr->fillRoundRect(px, py, PANEL_W, PANEL_R * 2, PANEL_R, 0x18C3);

        // Split line (flip hinge)
        spr->drawFastHLine(px + 3, py + halfH, PANEL_W - 6, TFT_BLACK);
        spr->drawFastHLine(px + 2, py + halfH - 1, PANEL_W - 4, 0x0841);
        spr->drawFastHLine(px + 2, py + halfH + 1, PANEL_W - 4, 0x0841);

        // Panel border
        spr->drawRoundRect(px, py, PANEL_W, PANEL_H, PANEL_R, TFT_DARKGREEN);

        // Draw the 7-segment digit centered inside the panel
        int dx = px + (PANEL_W - DIG_W) / 2;
        int dy = py + (PANEL_H - DIG_H) / 2;

        if (digit >= 0 && digit <= 9)
        {
            draw7Seg(spr, dx, dy, SEGS[digit]);
        }
    }

    // ─── Custom 7-segment renderer using fillRect ───
    void draw7Seg(TFT_eSprite *spr, int x, int y, uint8_t segs)
    {
        // Vertical segment height: fill the space between 3 horizontal segments
        int vH = (DIG_H - 3 * SEG_T) / 2; // = (105 - 27) / 2 = 39

        // Segment color
        uint16_t on = TFT_GREEN;
        uint16_t dim = 0x0841; // Very dark grey (ghosted unlit segments)

        // a - top horizontal
        spr->fillRect(x + SEG_T, y, DIG_W - 2 * SEG_T, SEG_T,
                      (segs & 0x01) ? on : dim);

        // b - top-right vertical
        spr->fillRect(x + DIG_W - SEG_T, y + SEG_T, SEG_T, vH,
                      (segs & 0x02) ? on : dim);

        // c - bottom-right vertical
        spr->fillRect(x + DIG_W - SEG_T, y + 2 * SEG_T + vH, SEG_T, vH,
                      (segs & 0x04) ? on : dim);

        // d - bottom horizontal
        spr->fillRect(x + SEG_T, y + DIG_H - SEG_T, DIG_W - 2 * SEG_T, SEG_T,
                      (segs & 0x08) ? on : dim);

        // e - bottom-left vertical
        spr->fillRect(x, y + 2 * SEG_T + vH, SEG_T, vH,
                      (segs & 0x10) ? on : dim);

        // f - top-left vertical
        spr->fillRect(x, y + SEG_T, SEG_T, vH,
                      (segs & 0x20) ? on : dim);

        // g - middle horizontal
        spr->fillRect(x + SEG_T, y + SEG_T + vH, DIG_W - 2 * SEG_T, SEG_T,
                      (segs & 0x40) ? on : dim);
    }

    // ─── Blinking colon ───
    void drawColon(TFT_eSprite *spr, int x, int y, bool visible)
    {
        uint16_t color = visible ? TFT_GREEN : TFT_DARKGREEN;
        int dotR = 6;
        int cy1 = y + PANEL_H / 3;
        int cy2 = y + (PANEL_H * 2) / 3;

        spr->fillCircle(x, cy1, dotR, color);
        spr->fillCircle(x, cy2, dotR, color);
    }
};
