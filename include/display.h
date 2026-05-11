// display.h - rendering the countdown UI (Outfit VLW fonts + visual fx)
#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <time.h>
#include "life_tables.h"
#include "settings.h"
#include "themes.h"
#include "timezones.h"
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

    // Setup-mode instructions screen. Shown while the captive portal AP is up.
    void showSetup(const String& apName) {
        const Theme& t = getTheme(0);
        _tft.fillScreen(t.bg);

        // Title
        _tft.loadFont(outfit_medium_22);
        _tft.setTextDatum(TC_DATUM);
        _tft.setTextColor(t.accent, t.bg);
        _tft.drawString("[ SETUP MODE ]", 160, 8);
        _tft.unloadFont();

        // Accent divider
        _tft.drawFastHLine(20, 36, 280, t.accent);

        // Step 1 label
        _tft.loadFont(outfit_regular_16);
        _tft.setTextDatum(TC_DATUM);
        _tft.setTextColor(t.muted, t.bg);
        _tft.drawString("1.  Join WiFi network", 160, 46);
        _tft.unloadFont();

        // AP name (highlighted)
        _tft.loadFont(outfit_medium_22);
        _tft.setTextColor(t.text, t.bg);
        _tft.drawString(apName, 160, 70);
        _tft.unloadFont();

        // Step 2 label
        _tft.loadFont(outfit_regular_16);
        _tft.setTextColor(t.muted, t.bg);
        _tft.drawString("2.  Open in browser", 160, 108);
        _tft.unloadFont();

        // IP address — big and bold
        _tft.loadFont(outfit_semibold_48);
        _tft.setTextColor(t.accent, t.bg);
        _tft.drawString("192.168.4.1", 160, 132);
        _tft.unloadFont();

        // Footer hints
        _tft.loadFont(outfit_medium_12);
        _tft.setTextDatum(BC_DATUM);
        _tft.setTextColor(t.muted, t.bg);
        _tft.drawString("Captive page should open automatically", 160, 222);
        _tft.drawString("Long-press 3s during use to return here", 160, 238);
        _tft.unloadFont();
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

    // Stash network info for the info-card mode. Called once after WiFi+mDNS
    // are up. Strings are copied.
    void setNetworkInfo(const String& host, const String& ip,
                        const String& ssid) {
        _host = host; _ip = ip; _ssid = ssid;
    }

    // Main render. mode: 0 = remaining, 1 = lived, 2 = network info card.
    void render(const Settings& s, const LifeStats& st, int mode) {
        const Theme& t = getTheme(s.theme);
        const long secs = st.secs;
        const bool tick = (secs & 1);

        _spr.fillSprite(t.bg);

        drawDustMotes(t);
        drawHeader(t, s, st, mode);
        drawECG(t, secs);

        if (mode == 2) {
            drawInfoCard(t, s);
        } else {
            drawHero(t, st);
            drawSubline(t, st, mode);
            drawClock(t, st, tick);
        }

        drawProgress(t, st);
        drawQuote(t);
        drawFooter(t, st, mode);

        if (strcmp(t.name, "matrix") == 0) drawScanlines();

        _spr.pushSprite(0, 0);
    }

private:
    TFT_eSPI&   _tft;
    TFT_eSprite _spr;

    // Network info populated by setNetworkInfo(); used by mode-2 card.
    String _host;
    String _ip;
    String _ssid;

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
        const char* title = (mode == 0) ? "MEMENTO MORI"
                          : (mode == 1) ? "TIME LIVED"
                                        : "NETWORK";
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

    }

    // Subline drawn after the hero block. Pulled out so we can pass mode
    // and switch "remaining" / "lived".
    void drawSubline(const Theme& t, const LifeStats& st, int mode) {
        _spr.loadFont(outfit_regular_16);
        _spr.setTextDatum(TC_DATUM);
        _spr.setTextColor(t.text, t.bg);
        char sub[40];
        snprintf(sub, sizeof(sub), "%ld days %s",
                 st.remDays, mode == 0 ? "remaining" : "lived");
        _spr.drawString(sub, 160, 134);
        _spr.unloadFont();
    }

    // ── CLOCK BAND (y=152..195, font48 = 43px tall) ────────────────────
    // Bar starts at y=198, so clock must end by y=195. Bumped up from 156.
    void drawClock(const Theme& t, const LifeStats& st, bool tick) {
        // Hourglass icons flanking the clock, vertically centred (clock midline
        // at y ≈ 173). Icon is 17px tall, so top at y=165.
        drawIcon       (40,  165, icon_hourglass, ICON_HOURGLASS_W, ICON_HOURGLASS_H,
                        tick ? t.accent : t.muted);
        drawIconMirror (264, 165, icon_hourglass, ICON_HOURGLASS_W, ICON_HOURGLASS_H,
                        tick ? t.accent : t.muted, true, false);

        _spr.loadFont(outfit_semibold_48);
        char hm[12], ss[8];
        snprintf(hm, sizeof(hm), "%02ld:%02ld:", st.hrs, st.mins);
        snprintf(ss, sizeof(ss), "%02ld", st.secs);
        int hmW = _spr.textWidth(hm);
        int ssW = _spr.textWidth(ss);
        int xStart = (320 - (hmW + ssW)) / 2;

        _spr.setTextDatum(TL_DATUM);
        _spr.setTextColor(t.text, t.bg);
        _spr.drawString(hm, xStart, 152);
        _spr.setTextColor(tick ? t.accent : t.text, t.bg);
        _spr.drawString(ss, xStart + hmW, 152);
        _spr.unloadFont();
    }

    // ── PROGRESS BAR (y=198..208) — segmented w/ subtle gradient.
    // No pct text overlay — the filled segment count and the percentage in
    // the bar already communicate it visually. Saves a whole row.
    void drawProgress(const Theme& t, const LifeStats& st) {
        const int segCount = 30;
        const int segW = 8, segH = 10, gap = 2;
        int totalW = segCount * segW + (segCount - 1) * gap;
        int barX   = (320 - totalW) / 2;
        int filled = (int)(segCount * st.pct + 0.5f);

        for (int i = 0; i < segCount; ++i) {
            int x = barX + i * (segW + gap);
            uint16_t c;
            if (i < filled) {
                c = (i == filled - 1) ? blend(t.accent, t.text, 96) : t.accent;
            } else {
                c = t.panel;
            }
            _spr.fillRect(x, 198, segW, segH, c);
        }
    }

    // ── QUOTE (BC y=222) ───────────────────────────────────────────────
    // No ornaments — text alone, in accent, with em-dashes for framing.
    void drawQuote(const Theme& t) {
        _spr.loadFont(outfit_medium_12);
        _spr.setTextDatum(BC_DATUM);
        _spr.setTextColor(t.accent, t.bg);
        _spr.drawString("- Make Every Day Count -", 160, 222);
        _spr.unloadFont();
    }

    // ── FOOTER (BC y=240) ──────────────────────────────────────────────
    // Bottom edge anchored; 18-row gap above quote (y=222) leaves visible
    // breathing room since font12 height is ~14 rows.
    void drawFooter(const Theme& t, const LifeStats& st, int mode) {
        _spr.loadFont(outfit_medium_12);
        _spr.setTextDatum(BC_DATUM);
        _spr.setTextColor(t.muted, t.bg);

        char foot[64];
        if (mode == 2) {
            // Info mode footer — WiFi SSID (truncated if long).
            String ssid = _ssid.length() > 30 ? _ssid.substring(0, 30) + "..." : _ssid;
            snprintf(foot, sizeof(foot), "WiFi: %s", ssid.c_str());
        } else {
            static const char* MON[] = {"Jan","Feb","Mar","Apr","May","Jun",
                                        "Jul","Aug","Sep","Oct","Nov","Dec"};
            int mIdx = (st.projMonth >= 1 && st.projMonth <= 12) ? st.projMonth - 1 : 0;
            snprintf(foot, sizeof(foot), "Projected: %02d %s %d   |   %.2f%% lived",
                     st.projDay, MON[mIdx], st.projYear, st.pct * 100.0f);
        }
        _spr.drawString(foot, 160, 238);
        _spr.unloadFont();
    }

    // ── INFO CARD (mode 2) — network details, replaces hero + clock ─────
    void drawInfoCard(const Theme& t, const Settings& s) {
        // y=46:    label "Web access:"
        // y=64:    URL (Outfit Medium 22, accent)
        // y=98:    label "IP address:"
        // y=124:   IP (Outfit SemiBold 48, accent — digits-only font)
        // y=176:   theme + tz mini-row

        _spr.loadFont(outfit_regular_16);
        _spr.setTextDatum(TC_DATUM);
        _spr.setTextColor(t.muted, t.bg);
        _spr.drawString("Web access:", 160, 46);
        _spr.unloadFont();

        String url = _host.length() ? (_host + ".local") : String("not connected");
        _spr.loadFont(outfit_medium_22);
        _spr.setTextColor(t.accent, t.bg);
        _spr.drawString(url, 160, 66);
        _spr.unloadFont();

        _spr.loadFont(outfit_regular_16);
        _spr.setTextColor(t.muted, t.bg);
        _spr.drawString("IP address:", 160, 98);
        _spr.unloadFont();

        _spr.loadFont(outfit_semibold_48);
        _spr.setTextColor(t.accent, t.bg);
        _spr.drawString(_ip.length() ? _ip : String("—"), 160, 122);
        _spr.unloadFont();

        // Theme + timezone line
        _spr.loadFont(outfit_medium_12);
        _spr.setTextColor(t.muted, t.bg);
        char meta[48];
        snprintf(meta, sizeof(meta), "Theme: %s   |   TZ: %s",
                 getTheme(s.theme).label,
                 (s.tz < TIMEZONE_COUNT ? TIMEZONES[s.tz].label : "—"));
        _spr.drawString(meta, 160, 180);
        _spr.unloadFont();
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
