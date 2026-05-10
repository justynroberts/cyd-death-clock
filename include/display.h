// display.h - rendering the countdown UI (Outfit VLW fonts + visual fx)
#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <time.h>
#include "life_tables.h"
#include "settings.h"
#include "themes.h"
#include "stats.h"
#include "icons.h"
#include "fonts_outfit.h"

class DeathDisplay {
public:
    DeathDisplay(TFT_eSPI& tft) : _tft(tft), _spr(&tft) {}

    void begin(int rotation, int brightnessPct) {
        _tft.init();
        _tft.setRotation(rotation);
        // CYD panel inverts in hardware — compensate with INVON.
        _tft.invertDisplay(true);
        _tft.fillScreen(getTheme(0).bg);

        // PWM backlight on TFT_BL (GPIO21), LEDC ch 0
        ledcSetup(0, 5000, 8);
        ledcAttachPin(21, 0);
        int duty = constrain(brightnessPct, 10, 100) * 255 / 100;
        ledcWrite(0, duty);

        // 16bpp full-frame back-buffer (~153KB) for atomic, flicker-free draw.
        _spr.setColorDepth(16);
        if (!_spr.createSprite(320, 240)) {
            _spr.setColorDepth(8);
            _spr.createSprite(320, 240);
        }

        seedDust();
    }

    // Boot/error status — uses 22px Outfit on the live display (no sprite).
    void showStatus(const String& line1, const String& line2 = "") {
        const Theme& t = getTheme(0);
        _tft.fillScreen(t.bg);
        _tft.loadFont(outfit_medium_22);
        _tft.setTextDatum(MC_DATUM);
        _tft.setTextColor(t.text, t.bg);
        _tft.drawString(line1, 160, 110);
        if (line2.length()) {
            _tft.setTextColor(t.muted, t.bg);
            _tft.drawString(line2, 160, 138);
        }
        _tft.unloadFont();
    }

    // Main render. mode: 0 = remaining, 1 = lived.
    void render(const Settings& s, const LifeStats& st, int mode) {
        const Theme& t = getTheme(s.theme);
        const long secs = st.secs;
        const bool tick = (secs & 1);  // alternates every second

        _spr.fillSprite(t.bg);

        drawDustMotes(t);
        drawHeader(t, s, st, mode);
        drawECG(t, secs);
        drawHero(t, st);
        drawClock(t, st, tick);
        drawProgress(t, st);
        drawQuote(t);
        drawFooter(t, st);
        drawPulseRing(t, secs);

        // Per-theme overlays
        if (strcmp(t.name, "matrix") == 0) drawScanlines();

        _spr.pushSprite(0, 0);
    }

private:
    TFT_eSPI&   _tft;
    TFT_eSprite _spr;

    // Dust motes — 12 sub-pixel particles drifting upward
    struct Mote { int16_t x, y; uint8_t speed, phase; };
    static const int MOTE_COUNT = 14;
    Mote _motes[MOTE_COUNT];

    void seedDust() {
        for (int i = 0; i < MOTE_COUNT; i++) {
            _motes[i].x     = random(0, 320);
            _motes[i].y     = random(0, 240);
            _motes[i].speed = 1 + random(0, 3);
            _motes[i].phase = random(0, 255);
        }
    }

    // ── ICON BLIT — 1bpp packed mask ───────────────────────────────────
    void drawIcon(int x, int y, const uint8_t* mask, int w, int h, uint16_t color) {
        for (int row = 0; row < h; row++) {
            for (int col = 0; col < w; col++) {
                int bitIdx = row * ((w + 7) / 8) * 8 + col;
                int byteIdx = bitIdx / 8;
                int bit     = 7 - (bitIdx & 7);
                uint8_t b = pgm_read_byte(&mask[byteIdx]);
                if (b & (1 << bit)) {
                    _spr.drawPixel(x + col, y + row, color);
                }
            }
        }
    }

    // Same as above but tiled/scaled — used for ornament that needs no scaling
    // (kept as separate method in case we add scaled icons later).

    // ── HEADER (y=0..28) ────────────────────────────────────────────────
    void drawHeader(const Theme& t, const Settings& s,
                    const LifeStats& st, int mode) {
        // Skull icon at left, theme accent
        drawIcon(8, 2, icon_skull, ICON_SKULL_W, ICON_SKULL_H, t.accent);

        // Title text
        _spr.loadFont(outfit_medium_22);
        _spr.setTextDatum(TL_DATUM);
        _spr.setTextColor(t.accent, t.bg);
        const char* title = (mode == 0) ? "MEMENTO MORI" : "TIME LIVED";
        _spr.drawString(title, 38, 4);

        // Right side: sex + age, smaller
        _spr.unloadFont();
        _spr.loadFont(outfit_medium_12);
        _spr.setTextDatum(TR_DATUM);
        _spr.setTextColor(t.muted, t.bg);
        char hdrR[24];
        snprintf(hdrR, sizeof(hdrR), "%c  %.1f yrs", s.sex, st.ageYears);
        _spr.drawString(hdrR, 312, 8);
        _spr.unloadFont();
    }

    // ── ECG / pulse waveform under header (y=28..40) ───────────────────
    // Draws a stylised heartbeat that scrolls left 1px per render.
    void drawECG(const Theme& t, long secs) {
        const int y0 = 33;     // baseline
        const int y1 = 31;
        // Phase shift via secs so the trace appears to scroll.
        // Width 296 leaves a 12px margin on each side.
        const int phase = (int)(secs * 6) % 60;
        for (int x = 12; x < 308; x++) {
            int local = (x - 12 + phase) % 60;
            int yy = y0;
            // QRS-style spike shape across positions 20..30 of the 60-cycle
            if      (local == 22) yy = y0 - 1;
            else if (local == 23) yy = y0 + 2;
            else if (local == 24) yy = y0 - 8;   // Q
            else if (local == 25) yy = y0 - 12;  // R peak
            else if (local == 26) yy = y0 + 6;   // S
            else if (local == 27) yy = y0 + 1;
            else if (local == 28) yy = y0;
            else if (local == 32) yy = y0 - 2;   // T wave
            else if (local == 33) yy = y0 - 3;
            else if (local == 34) yy = y0 - 2;
            _spr.drawPixel(x, yy, t.accent);
            // Subtle trail (1px below baseline) for thickness
            if (yy != y0) _spr.drawPixel(x, yy + 1, dimColor(t.accent, 64));
        }
        // Baseline track (faint)
        _spr.drawFastHLine(12, y0 + 4, 296, t.panel);
    }

    // ── HERO BLOCK — corner brackets + giant years (y=46..130) ─────────
    void drawHero(const Theme& t, const LifeStats& st) {
        // Big years number with glow
        char big[16];
        snprintf(big, sizeof(big), "%ld", st.years);
        _spr.loadFont(outfit_bold_96);
        _spr.setTextDatum(MC_DATUM);

        // 3-pass glow: low alpha tint at offsets, then sharp accent on top.
        // RGB565 has no alpha, so we fake glow with dimmed blends to bg.
        uint16_t glow1 = blend(t.accent, t.bg, 64);   // ~25% accent
        uint16_t glow2 = blend(t.accent, t.bg, 144);  // ~56%
        for (int dx = -2; dx <= 2; dx++) for (int dy = -2; dy <= 2; dy++) {
            if (dx == 0 && dy == 0) continue;
            int d = abs(dx) + abs(dy);
            if (d > 3) continue;
            _spr.setTextColor((d == 1 ? glow2 : glow1), t.bg);
            _spr.drawString(big, 160 + dx, 90 + dy);
        }
        _spr.setTextColor(t.accent, t.bg);
        _spr.drawString(big, 160, 90);
        int bigW = _spr.textWidth(big);
        _spr.unloadFont();

        // Corner brackets framing the number
        int frameW = bigW + 56;
        int frameH = 84;
        int fx = 160 - frameW / 2;
        int fy = 90 - 42;
        // Top-left
        drawIcon(fx,                     fy,                  icon_bracket_tl, 12, 12, t.muted);
        // Top-right (mirror horizontally)
        drawIconMirror(fx + frameW - 12, fy,                  icon_bracket_tl, 12, 12, t.muted, true, false);
        // Bottom-left (mirror vertically)
        drawIconMirror(fx,                fy + frameH - 12,   icon_bracket_tl, 12, 12, t.muted, false, true);
        // Bottom-right (mirror both)
        drawIconMirror(fx + frameW - 12, fy + frameH - 12,    icon_bracket_tl, 12, 12, t.muted, true, true);

        // Subline under the number
        _spr.loadFont(outfit_regular_16);
        _spr.setTextDatum(TC_DATUM);
        _spr.setTextColor(t.text, t.bg);
        char sub[40];
        snprintf(sub, sizeof(sub), "%ld days remaining", st.remDays);
        _spr.drawString(sub, 160, 134);
        _spr.unloadFont();
    }

    // ── CLOCK BAND (y=156..188) ─────────────────────────────────────────
    void drawClock(const Theme& t, const LifeStats& st, bool tick) {
        // Hourglass icon left of clock (animated: alternates frames)
        drawIcon(40, 158, icon_hourglass, ICON_HOURGLASS_W, ICON_HOURGLASS_H,
                 tick ? t.accent : t.muted);

        // Mirror right
        drawIconMirror(264, 158, icon_hourglass, ICON_HOURGLASS_W, ICON_HOURGLASS_H,
                       tick ? t.accent : t.muted, true, false);

        // Time text — split into "HH:MM:" and "SS" so seconds tint per tick
        _spr.loadFont(outfit_semibold_48);
        char hm[12], ss[8];
        snprintf(hm, sizeof(hm), "%02ld:%02ld:", st.hrs, st.mins);
        snprintf(ss, sizeof(ss), "%02ld", st.secs);
        int hmW = _spr.textWidth(hm);
        int ssW = _spr.textWidth(ss);
        int totalW = hmW + ssW;
        int xStart = (320 - totalW) / 2;

        _spr.setTextDatum(TL_DATUM);
        _spr.setTextColor(t.text, t.bg);
        _spr.drawString(hm, xStart, 156);
        _spr.setTextColor(tick ? t.accent : t.text, t.bg);
        _spr.drawString(ss, xStart + hmW, 156);
        _spr.unloadFont();
    }

    // ── PROGRESS BAR (y=200..212) — segmented w/ subtle gradient ───────
    void drawProgress(const Theme& t, const LifeStats& st) {
        const int segCount = 30;
        const int segW = 8, segH = 12, gap = 2;
        int totalW = segCount * segW + (segCount - 1) * gap;
        int barX   = (320 - totalW) / 2;
        int filled = (int)(segCount * st.pct + 0.5f);

        for (int i = 0; i < segCount; ++i) {
            int x = barX + i * (segW + gap);
            uint16_t c;
            if (i < filled) {
                // Filled — gradient from accent to lighter near top
                c = (i == filled - 1) ? blend(t.accent, t.text, 96) : t.accent;
            } else {
                c = t.panel;
            }
            _spr.fillRect(x, 200, segW, segH, c);
        }

        // Pct text below
        _spr.loadFont(outfit_medium_12);
        _spr.setTextDatum(TC_DATUM);
        _spr.setTextColor(t.muted, t.bg);
        char pctS[16];
        snprintf(pctS, sizeof(pctS), "%.2f%% lived", st.pct * 100.0f);
        _spr.drawString(pctS, 160, 215);
        _spr.unloadFont();
    }

    // ── QUOTE (y=222..236) ─────────────────────────────────────────────
    void drawQuote(const Theme& t) {
        // Ornament left + right of quote
        drawIcon(64,  226, icon_ornament, ICON_ORNAMENT_W, ICON_ORNAMENT_H, t.muted);
        drawIcon(224, 226, icon_ornament, ICON_ORNAMENT_W, ICON_ORNAMENT_H, t.muted);

        _spr.loadFont(outfit_medium_12);
        _spr.setTextDatum(TC_DATUM);
        _spr.setTextColor(t.accent, t.bg);
        _spr.drawString("Make Every Day Count", 160, 224);
        _spr.unloadFont();
    }

    // ── FOOTER (y=232..240) ────────────────────────────────────────────
    void drawFooter(const Theme& t, const LifeStats& st) {
        static const char* MON[] = {"Jan","Feb","Mar","Apr","May","Jun",
                                    "Jul","Aug","Sep","Oct","Nov","Dec"};
        char foot[40];
        int mIdx = (st.projMonth >= 1 && st.projMonth <= 12) ? st.projMonth - 1 : 0;
        snprintf(foot, sizeof(foot), "Projected: %02d %s %d",
                 st.projDay, MON[mIdx], st.projYear);
        _spr.loadFont(outfit_medium_12);
        _spr.setTextDatum(BC_DATUM);
        _spr.setTextColor(t.muted, t.bg);
        _spr.drawString(foot, 160, 240);
        _spr.unloadFont();
    }

    // ── PULSE RING — radial expanding ring around clock baseline ───────
    void drawPulseRing(const Theme& t, long secs) {
        // Two concentric rings that pulse at the start of each second.
        int cx = 160, cy = 172;
        int r = 6 + ((millis() / 50) % 12);
        uint8_t alpha = 200 - r * 12;
        if (alpha > 30) {
            uint16_t c = blend(t.accent, t.bg, alpha);
            _spr.drawCircle(cx, cy, r, c);
        }
    }

    // ── DUST MOTES — drifting particles across the entire frame ────────
    void drawDustMotes(const Theme& t) {
        for (int i = 0; i < MOTE_COUNT; i++) {
            Mote& m = _motes[i];
            m.y -= m.speed;
            m.phase += 7;
            if (m.y < 0) {
                m.y = 240 + random(0, 30);
                m.x = random(0, 320);
            }
            // Slight horizontal drift via phase
            int xx = m.x + (int)(m.phase / 64) - 2;
            uint16_t c = (m.speed > 1) ? t.muted : t.panel;
            _spr.drawPixel(xx, m.y, c);
        }
    }

    // ── MATRIX scanlines overlay ───────────────────────────────────────
    void drawScanlines() {
        for (int y = 0; y < 240; y += 3) {
            _spr.drawFastHLine(0, y, 320, 0x0000);  // dim every 3rd row
        }
    }

    // ── COLOR HELPERS ──────────────────────────────────────────────────
    // Linear blend of two RGB565 colors: alpha 0..255.
    static uint16_t blend(uint16_t fg, uint16_t bg, uint8_t alpha) {
        uint8_t fr = (fg >> 11) & 0x1F;
        uint8_t fgrn = (fg >> 5) & 0x3F;
        uint8_t fb = fg & 0x1F;
        uint8_t br = (bg >> 11) & 0x1F;
        uint8_t bgrn = (bg >> 5) & 0x3F;
        uint8_t bb = bg & 0x1F;
        uint16_t r = (fr * alpha + br * (255 - alpha)) / 255;
        uint16_t g = (fgrn * alpha + bgrn * (255 - alpha)) / 255;
        uint16_t b = (fb * alpha + bb * (255 - alpha)) / 255;
        return (r << 11) | (g << 5) | b;
    }

    // Dim a color toward black by alpha.
    static uint16_t dimColor(uint16_t c, uint8_t alpha) {
        return blend(c, 0x0000, alpha);
    }

    // Mirror an icon as it's drawn (used for symmetric corner brackets and
    // the right-side hourglass).
    void drawIconMirror(int x, int y, const uint8_t* mask, int w, int h,
                        uint16_t color, bool mx, bool my) {
        for (int row = 0; row < h; row++) {
            for (int col = 0; col < w; col++) {
                int srcRow = my ? (h - 1 - row) : row;
                int srcCol = mx ? (w - 1 - col) : col;
                int bitIdx = srcRow * ((w + 7) / 8) * 8 + srcCol;
                int byteIdx = bitIdx / 8;
                int bit     = 7 - (bitIdx & 7);
                uint8_t b = pgm_read_byte(&mask[byteIdx]);
                if (b & (1 << bit)) {
                    _spr.drawPixel(x + col, y + row, color);
                }
            }
        }
    }
};
