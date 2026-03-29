#pragma once
#include "IScreen.h"
#include "BaseSprite.h"
#include "../services/TimeService.h"
#include <math.h>

/**
 * @class ScreenAnalogClock
 * @brief Analog Clock + Monthly Calendar.
 *
 * LAYOUT (320×240):
 * - Header: muted green bar (TFT_DARKGREEN), black text, left-aligned.
 * - Left half (0-159): Analog clock centered, small digital HH:MM:SS below.
 * - Right half (160-319): Month/Year title + weekday headers + 6-row calendar grid.
 *   Current day highlighted with a red box.
 * - "Retro Terminal" aesthetic (green/black).
 */
class ScreenAnalogClock : public IScreen
{
private:
    BaseSprite display;

    // ─── Clock geometry (left half, vertically centered) ───
    static const int CX = 80;
    static const int CY = 128;
    static const int R_FACE = 78; // Face radius
    static const int R_MARK = 74; // Outer tick end
    static const int R_H = 44;    // Hour hand
    static const int R_M = 62;    // Minute hand
    static const int R_S = 68;    // Second hand

    // ─── Calendar geometry (right half, fills vertical) ───
    static const int CAL_X = 162; // Calendar left edge
    static const int CAL_Y = 34;  // Weekday headers Y
    static const int CELL_W = 22; // Cell width  (22×7 = 154)
    static const int CELL_H = 30; // Cell height (30×6 = 180 + header ≈ full height)

    // Pre-calculated trig for 12 ticks
    float cos12[12];
    float sin12[12];

    bool pendingUpdate = true;

    // ─── Calendar helper: days in month ───
    static int daysInMonth(int y, int m)
    {
        static const int dm[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0))
            return 29;
        return dm[m];
    }

    // ─── Day-of-week for any date (Tomohiko Sakamoto) ───
    // Returns 0=Sun, 1=Mon ... 6=Sat
    static int dow(int y, int m, int d)
    {
        static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
        if (m < 3)
            y--;
        return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
    }

public:
    const char *getName() override { return "ANALOG_CLOCK"; }

    void onEnter() override
    {
        IScreen::onEnter();
        display.init(DisplayHAL::getInstance().getRaw());

        for (int i = 0; i < 12; i++)
        {
            float a = i * 30.0f * 0.0174532925f;
            sin12[i] = sinf(a);
            cos12[i] = cosf(a);
        }

        LedHAL::getInstance().set(0, 128, 128);
        pendingUpdate = true;
    }

    void onExit() override { display.free(); }

    void on1SecTimer() override { pendingUpdate = true; }

    void onLoop() override
    {
        if (!pendingUpdate)
            return;
        pendingUpdate = false;
        if (!display.isReady())
            return;

        TFT_eSprite *spr = display.getRaw();
        display.clear();

        // ═══════════ HEADER (muted green, like FlipClock) ═══════════
        spr->fillRect(0, 0, 320, 16, TFT_DARKGREEN);
        spr->setTextColor(TFT_BLACK, TFT_DARKGREEN);
        spr->drawString(" SYSTEM TIME ", 2, 0, 2);
        spr->setTextColor(TFT_GREEN, TFT_BLACK);

        // ─── Get time & date ───
        int hh = 0, mm = 0, ss = 0;
        bool timeOk = TimeService::getInstance().getTimeParts(hh, mm, ss);
        int yr = 0, mo = 0, dy = 0, wd = 0;
        bool dateOk = TimeService::getInstance().getDateParts(yr, mo, dy, wd);

        // ═══════════ LEFT HALF: ANALOG CLOCK ═══════════
        drawClockFace(spr);

        if (timeOk)
        {
            int hh12 = hh % 12;
            drawHand(spr, ss * 6.0f, R_S, 2, TFT_RED);                  // Second
            drawHand(spr, mm * 6.0f + ss * 0.1f, R_M, 3, TFT_GREEN);    // Minute
            drawHand(spr, hh12 * 30.0f + mm * 0.5f, R_H, 5, TFT_GREEN); // Hour
            spr->fillCircle(CX, CY, 4, TFT_DARKGREEN);

            // Small digital time below clock
            char tbuf[12];
            snprintf(tbuf, sizeof(tbuf), "%02d:%02d:%02d", hh, mm, ss);
            spr->setTextColor(TFT_DARKGREEN, TFT_BLACK);
            spr->drawCentreString(tbuf, CX, CY + R_FACE + 12, 1);
        }
        else
        {
            spr->fillCircle(CX, CY, 4, TFT_DARKGREEN);
            spr->setTextColor(TFT_DARKGREEN, TFT_BLACK);
            spr->drawCentreString("--:--:--", CX, CY + R_FACE + 12, 1);
        }

        // ═══════════ RIGHT HALF: CALENDAR ═══════════
        if (dateOk)
            drawCalendar(spr, yr, mo, dy);
        else
        {
            spr->setTextColor(TFT_DARKGREEN, TFT_BLACK);
            spr->drawCentreString("No date", 240, 120, 2);
        }

        // Vertical divider
        spr->drawFastVLine(160, 16, 224, 0x0841);

        display.push();
    }

private:
    // ─── Draw clock face (ticks + circle) ───
    void drawClockFace(TFT_eSprite *spr)
    {
        spr->drawCircle(CX, CY, R_FACE + 3, TFT_DARKGREEN);

        for (int i = 0; i < 12; i++)
        {
            int inner = (i % 3 == 0) ? R_MARK - 12 : R_MARK - 6;
            int x1 = CX + inner * sin12[i];
            int y1 = CY - inner * cos12[i];
            int x2 = CX + R_MARK * sin12[i];
            int y2 = CY - R_MARK * cos12[i];
            spr->drawLine(x1, y1, x2, y2, TFT_GREEN);
        }
    }

    // ─── Draw a clock hand using fillTriangle (8bpp safe) ───
    void drawHand(TFT_eSprite *spr, float deg, int len, int baseHW, uint16_t col)
    {
        static constexpr float D2R = 0.0174532925f;
        float rad = (deg - 90.0f) * D2R;
        float perpRad = rad + 1.5708f; // +90° perpendicular

        // Tip of the hand
        int tx = CX + (int)(len * cosf(rad));
        int ty = CY + (int)(len * sinf(rad));

        // Base points (perpendicular to hand direction)
        float bcos = baseHW * cosf(perpRad);
        float bsin = baseHW * sinf(perpRad);
        int bx1 = CX + (int)bcos;
        int by1 = CY + (int)bsin;
        int bx2 = CX - (int)bcos;
        int by2 = CY - (int)bsin;

        // Main hand (tapered triangle)
        spr->fillTriangle(tx, ty, bx1, by1, bx2, by2, col);

        // Counter-weight tail (short, behind center)
        int tailLen = len / 5;
        int ttx = CX - (int)(tailLen * cosf(rad));
        int tty = CY - (int)(tailLen * sinf(rad));
        spr->fillTriangle(ttx, tty, bx1, by1, bx2, by2, col);
    }

    // ─── Draw monthly calendar with red box on today ───
    void drawCalendar(TFT_eSprite *spr, int yr, int mo, int today)
    {
        // ── Month / Year title ──
        static const char *MON[] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        char title[20];
        snprintf(title, sizeof(title), "%s %d", MON[mo], yr);
        spr->setTextColor(TFT_GREEN, TFT_BLACK);
        spr->drawCentreString(title, CAL_X + (CELL_W * 7) / 2, 20, 2);

        // ── Weekday headers (Mon-Sun) ──
        static const char *HD[] = {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};
        spr->setTextColor(TFT_DARKGREEN, TFT_BLACK);
        int hdY = CAL_Y + 8; // Extra gap below month title
        for (int c = 0; c < 7; c++)
        {
            int hx = CAL_X + c * CELL_W + CELL_W / 2;
            spr->drawCentreString(HD[c], hx, hdY, 1);
        }

        // Separator under headers
        spr->drawFastHLine(CAL_X, hdY + 12, CELL_W * 7, TFT_DARKGREEN);

        // ── Fill day numbers ──
        int firstDow = dow(yr, mo, 1);     // 0=Sun
        int startCol = (firstDow + 6) % 7; // Mon=0 based
        int nDays = daysInMonth(yr, mo);

        int row = 0, col = startCol;
        int gridY = hdY + 15; // Start just below separator

        for (int d = 1; d <= nDays; d++)
        {
            int cellX = CAL_X + col * CELL_W;
            int cellY = gridY + row * CELL_H;
            int textY = cellY + (CELL_H - 16) / 2; // Vertically center font 2 (~16px)

            char db[4];
            snprintf(db, sizeof(db), "%d", d);

            if (d == today)
            {
                // Red box around today
                spr->drawRect(cellX + 1, cellY + 1, CELL_W - 2, CELL_H - 2, TFT_RED);
                spr->setTextColor(TFT_RED, TFT_BLACK);
            }
            else
            {
                spr->setTextColor((col >= 5) ? TFT_DARKGREEN : TFT_GREEN, TFT_BLACK);
            }

            spr->drawCentreString(db, cellX + CELL_W / 2, textY, 2);

            col++;
            if (col >= 7)
            {
                col = 0;
                row++;
            }
        }
    }
};
