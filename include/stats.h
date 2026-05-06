// stats.h - life-expectancy calculations shared by display and web API.
#pragma once

#include <Arduino.h>
#include <time.h>
#include "life_tables.h"
#include "settings.h"

struct LifeStats {
    float ageYears;
    float remainingYears;
    float pct;            // 0..1, fraction of expected life elapsed
    long  years, remDays; // breakdown of `showSecs`
    long  hrs, mins, secs;
    int   projYear, projMonth, projDay;  // 1-based month/day
};

inline bool _statsIsLeap(int y) {
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

// Compute everything shown on screen. mode: 0 = remaining, 1 = lived.
inline LifeStats computeStats(const Settings& s, time_t now, int mode) {
    struct tm dob = {};
    dob.tm_year = s.dobYear - 1900;
    dob.tm_mon  = s.dobMonth - 1;
    dob.tm_mday = s.dobDay;
    dob.tm_hour = 12;
    time_t dobT = mktime(&dob);

    double livedSecs = difftime(now, dobT);
    if (livedSecs < 0) livedSecs = 0;

    LifeStats st{};
    st.ageYears       = livedSecs / (365.2425 * 86400.0);
    st.remainingYears = lifeExpectancyRemaining(st.ageYears, s.sex);

    // Use double — time_t is 32-bit signed on ESP32, projections past 2038
    // wrap if held in time_t.
    double remainingSecs = (double)st.remainingYears * 365.2425 * 86400.0;
    if (remainingSecs < 0) remainingSecs = 0;

    double total = livedSecs + remainingSecs;
    st.pct = total > 0 ? (float)(livedSecs / total) : 0.0f;
    if (st.pct > 1.0f) st.pct = 1.0f;

    double showSecs = (mode == 0) ? remainingSecs : livedSecs;
    long totalDays = (long)(showSecs / 86400);
    st.hrs     = (long)(showSecs / 3600) % 24;
    st.mins    = (long)(showSecs / 60)   % 60;
    st.secs    = (long)showSecs          % 60;
    st.years   = totalDays / 365;
    st.remDays = totalDays % 365;

    // Projected date — add years/days to current date in tm fields,
    // manual month rollover (no time_t to avoid 2038 wrap).
    struct tm proj;
    localtime_r(&now, &proj);
    int addYears  = (int)st.remainingYears;
    double frac   = ((double)st.remainingYears - addYears) * 365.2425;
    proj.tm_year += addYears;
    proj.tm_mday += (int)frac;
    static const int dpm[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    while (true) {
        int dim = dpm[proj.tm_mon] +
                  ((proj.tm_mon == 1 && _statsIsLeap(proj.tm_year + 1900)) ? 1 : 0);
        if (proj.tm_mday <= dim) break;
        proj.tm_mday -= dim;
        proj.tm_mon++;
        if (proj.tm_mon >= 12) { proj.tm_mon = 0; proj.tm_year++; }
    }
    st.projYear  = proj.tm_year + 1900;
    st.projMonth = proj.tm_mon + 1;
    st.projDay   = proj.tm_mday;
    return st;
}
