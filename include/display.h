// display.h - rendering the countdown UI
#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <time.h>
#include "life_tables.h"
#include "settings.h"
#include "themes.h"
#include "stats.h"

class DeathDisplay {
public:
    DeathDisplay(TFT_eSPI& tft) : _tft(tft), _spr(&tft) {}

    void begin(int rotation, int brightnessPct) {
        _tft.init();
        _tft.setRotation(rotation);
        // CYD panels invert colors in hardware. Send INVON to compensate so
        // a black framebuffer pixel actually shows as black on the screen.
        // Symptom of wrong polarity: Amber theme renders as blue on white.
        _tft.invertDisplay(true);
        _tft.fillScreen(getTheme(0).bg);
        // PWM backlight on TFT_BL
        ledcSetup(0, 5000, 8);
        ledcAttachPin(21, 0);
        int duty = constrain(brightnessPct, 10, 100) * 255 / 100;
        ledcWrite(0, duty);

        // Full-screen back-buffer to eliminate flicker.
        // Try 16bpp first (~153KB); fall back to 8bpp (~76KB) if heap can't fit it.
        _spr.setColorDepth(16);
        if (!_spr.createSprite(320, 240)) {
            _spr.setColorDepth(8);
            _spr.createSprite(320, 240);
        }
    }

    void showStatus(const String& line1, const String& line2 = "") {
        // Status screens (boot, errors, portal) always use the default theme
        // since we may not have settings loaded yet.
        const Theme& t = getTheme(0);
        _tft.fillScreen(t.bg);
        _tft.setTextDatum(MC_DATUM);
        _tft.setTextColor(t.text, t.bg);
        _tft.drawString(line1, 160, 110, 4);
        if (line2.length()) {
            _tft.setTextColor(t.muted, t.bg);
            _tft.drawString(line2, 160, 140, 2);
        }
    }

    // Draw with pre-computed stats (computed once per tick in main loop and
    // shared with the web API so /api shows the same numbers as the screen).
    void render(const Settings& s, const LifeStats& st, int mode) {
        // Pull frequently-used stat fields into locals for readability
        const float ageYears = st.ageYears;
        const long  years    = st.years;
        const long  remDays  = st.remDays;
        const long  hrs      = st.hrs;
        const long  mins     = st.mins;
        const long  secs     = st.secs;
        const float pct      = st.pct;

        // --- draw to sprite back-buffer ---
        const Theme& t = getTheme(s.theme);
        _spr.fillSprite(t.bg);

        // ── Header band (y=2..20) ───────────────────────────────────────
        const char* title = (mode == 0) ? "[ MEMENTO MORI ]" : "[ TIME LIVED ]";
        _spr.setTextDatum(TL_DATUM);
        _spr.setTextColor(t.accent, t.bg);
        _spr.drawString(title, 12, 4, 2);

        _spr.setTextDatum(TR_DATUM);
        _spr.setTextColor(t.muted, t.bg);
        char hdrR[24];
        snprintf(hdrR, sizeof(hdrR), "%c  %.1f yrs", s.sex, ageYears);
        _spr.drawString(hdrR, 308, 4, 2);

        // Top divider (double-line for weight)
        _spr.drawFastHLine(12, 22, 296, t.accent);
        _spr.drawFastHLine(12, 23, 296, t.panel);

        // ── Hero: big years (y=28..103, font 8 = 75px) ───────────────────
        char big[16];
        snprintf(big, sizeof(big), "%ld", years);
        _spr.setTextDatum(MC_DATUM);
        _spr.setTextColor(t.accent, t.bg);
        _spr.drawString(big, 160, 65, 8);

        // Underline accent under the big number — width tracks the digits
        int bigW = _spr.textWidth(big, 8);
        int ulW = bigW + 24;
        _spr.fillRect(160 - ulW/2, 106, ulW, 2, t.accent);

        // Subline: "yrs · N days remaining"
        char sub[40];
        snprintf(sub, sizeof(sub), "yrs  -  %ld days %s",
                 remDays, mode == 0 ? "remaining" : "lived");
        _spr.setTextDatum(TC_DATUM);
        _spr.setTextColor(t.text, t.bg);
        _spr.drawString(sub, 160, 112, 2);

        // ── Clock band (y=132..158, font 4 = 26px) ──────────────────────
        // Split into "HH:MM:" + "SS" so seconds can flash on odd ticks.
        char hm[8], ss[4];
        snprintf(hm, sizeof(hm), "%02ld:%02ld:", hrs, mins);
        snprintf(ss, sizeof(ss), "%02ld", secs);
        int hmW    = _spr.textWidth(hm, 4);
        int ssW    = _spr.textWidth(ss, 4);
        int clkX   = (320 - (hmW + ssW)) / 2;
        _spr.setTextDatum(TL_DATUM);
        _spr.setTextColor(t.text, t.bg);
        _spr.drawString(hm, clkX, 132, 4);
        _spr.setTextColor((secs & 1) ? t.accent : t.text, t.bg);
        _spr.drawString(ss, clkX + hmW, 132, 4);

        // Pulsing dots flanking the clock — alive-indicator
        uint16_t dotCol = (secs & 1) ? t.accent : t.panel;
        _spr.fillCircle(clkX - 16,            145, 4, dotCol);
        _spr.fillCircle(clkX + hmW + ssW + 16, 145, 4, dotCol);

        // Bottom divider
        _spr.drawFastHLine(12, 164, 296, t.accent);
        _spr.drawFastHLine(12, 165, 296, t.panel);

        // ── Segmented progress bar (y=170..182) ──────────────────────────
        const int segCount = 30;
        const int segW     = 8;
        const int segH     = 12;
        const int gap      = 2;
        int totalW   = segCount * segW + (segCount - 1) * gap;
        int barStart = (320 - totalW) / 2;
        int filled   = (int)(segCount * pct + 0.5f);
        for (int i = 0; i < segCount; ++i) {
            int x = barStart + i * (segW + gap);
            uint16_t c = (i < filled) ? t.accent : t.panel;
            _spr.fillRect(x, 170, segW, segH, c);
        }

        // ── Bottom band — all three lines use BC datum so their bottom
        //    edges are exact and gaps between them are guaranteed.
        _spr.setTextDatum(BC_DATUM);

        char pctS[16];
        snprintf(pctS, sizeof(pctS), "%.2f%% lived", pct * 100.0f);
        _spr.setTextColor(t.muted, t.bg);
        _spr.drawString(pctS, 160, 200, 2);

        _spr.setTextColor(t.accent, t.bg);
        _spr.drawString("- Make Every Day Count -", 160, 220, 2);

        static const char* MON[] = {"Jan","Feb","Mar","Apr","May","Jun",
                                    "Jul","Aug","Sep","Oct","Nov","Dec"};
        char foot[40];
        int monIdx = (st.projMonth >= 1 && st.projMonth <= 12) ? st.projMonth - 1 : 0;
        snprintf(foot, sizeof(foot), "Projected: %02d %s %d",
                 st.projDay, MON[monIdx], st.projYear);
        _spr.setTextColor(t.muted, t.bg);
        _spr.drawString(foot, 160, 238, 2);

        // Single blit to display — atomic, no flicker.
        _spr.pushSprite(0, 0);
    }

private:
    TFT_eSPI&   _tft;
    TFT_eSprite _spr;
};
