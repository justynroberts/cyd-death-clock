// timezones.h - POSIX TZ strings + labels for the captive portal
#pragma once
#include <stdint.h>

struct TimeZone {
    const char* label;   // human-friendly
    const char* posix;   // string for setenv("TZ", ...)
};

// Common zones. POSIX TZ encodes the DST rules so the device handles
// transitions automatically without needing online tzdata.
static const TimeZone TIMEZONES[] = {
    { "UTC",                 "UTC0" },
    { "UK (London)",         "GMT0BST,M3.5.0/1,M10.5.0/2" },
    { "Ireland (Dublin)",    "IST-1GMT0,M10.5.0,M3.5.0/1" },
    { "Central Europe",      "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Eastern Europe",      "EET-2EEST,M3.5.0/3,M10.5.0/4" },
    { "US Eastern",          "EST5EDT,M3.2.0,M11.1.0" },
    { "US Central",          "CST6CDT,M3.2.0,M11.1.0" },
    { "US Mountain",         "MST7MDT,M3.2.0,M11.1.0" },
    { "US Pacific",          "PST8PDT,M3.2.0,M11.1.0" },
    { "India (Kolkata)",     "IST-5:30" },
    { "Japan (Tokyo)",       "JST-9" },
    { "Australia East",      "AEST-10AEDT,M10.1.0,M4.1.0/3" },
    { "New Zealand",         "NZST-12NZDT,M9.5.0,M4.1.0/3" },
};

static const uint8_t TIMEZONE_COUNT = sizeof(TIMEZONES) / sizeof(TIMEZONES[0]);

inline const TimeZone& getTimezone(uint8_t idx) {
    if (idx >= TIMEZONE_COUNT) idx = 1;  // default UK
    return TIMEZONES[idx];
}
