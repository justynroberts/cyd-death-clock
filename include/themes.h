// themes.h - color palettes for the display
#pragma once

#include <stdint.h>

#define RGB565(r,g,b) ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))

struct Theme {
    const char* name;     // short id, used by the portal <select>
    const char* label;    // human-friendly label for portal UI
    uint16_t bg;
    uint16_t panel;
    uint16_t text;
    uint16_t muted;
    uint16_t accent;
    uint16_t danger;
};

static const Theme THEMES[] = {
    // 0: Dark — original Uber-ish black + green
    { "dark", "Dark (default)",
      RGB565(0x0A,0x0A,0x0B),  // bg
      RGB565(0x15,0x16,0x1A),  // panel
      RGB565(0xE8,0xE8,0xEA),  // text
      RGB565(0x88,0x8A,0x92),  // muted
      RGB565(0x00,0xD5,0x63),  // accent (green)
      RGB565(0xFF,0x4D,0x4D) },// danger

    // 1: Light — off-white background, deep green accent
    { "light", "Light",
      RGB565(0xF5,0xF5,0xF7),
      RGB565(0xFF,0xFF,0xFF),
      RGB565(0x1A,0x1A,0x1F),
      RGB565(0x6C,0x6E,0x75),
      RGB565(0x00,0x90,0x40),
      RGB565(0xC0,0x39,0x2B) },

    // 2: Matrix — pure black, electric green everywhere
    { "matrix", "Matrix",
      RGB565(0x00,0x00,0x00),
      RGB565(0x00,0x20,0x00),
      RGB565(0x33,0xFF,0x33),
      RGB565(0x00,0x80,0x00),
      RGB565(0x00,0xFF,0x55),
      RGB565(0xFF,0x00,0x00) },

    // 3: Amber — phosphor CRT terminal vibes
    { "amber", "Amber CRT",
      RGB565(0x05,0x02,0x00),
      RGB565(0x1A,0x10,0x00),
      RGB565(0xFF,0xB0,0x00),
      RGB565(0x88,0x58,0x00),
      RGB565(0xFF,0xD0,0x00),
      RGB565(0xFF,0x40,0x00) },

    // 4: Crimson — black + deep red, ominous
    { "crimson", "Crimson",
      RGB565(0x08,0x04,0x04),
      RGB565(0x1A,0x0A,0x0A),
      RGB565(0xF0,0xD0,0xD0),
      RGB565(0x80,0x60,0x60),
      RGB565(0xFF,0x30,0x30),
      RGB565(0xFF,0x80,0x80) },

    // 5: Ocean — deep navy + cyan
    { "ocean", "Ocean",
      RGB565(0x00,0x10,0x20),
      RGB565(0x00,0x20,0x35),
      RGB565(0xD0,0xE0,0xF0),
      RGB565(0x60,0x80,0xA0),
      RGB565(0x00,0xD0,0xD0),
      RGB565(0xFF,0x80,0x80) },
};

static const uint8_t THEME_COUNT = sizeof(THEMES) / sizeof(THEMES[0]);

inline const Theme& getTheme(uint8_t idx) {
    if (idx >= THEME_COUNT) idx = 0;
    return THEMES[idx];
}
