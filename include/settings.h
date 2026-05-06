// settings.h - persistent config in NVS via Preferences
#pragma once

#include <Preferences.h>
#include <Arduino.h>

struct Settings {
    String  ssid;
    String  password;
    int     dobYear;
    int     dobMonth;
    int     dobDay;
    char    sex;        // 'M' or 'F'
    uint8_t theme;      // index into THEMES[]; out-of-range falls back to 0
    bool    configured; // true once user has saved valid settings

    bool isValid() const {
        if (!configured) return false;
        if (ssid.length() == 0) return false;
        if (dobYear < 1900 || dobYear > 2025) return false;
        if (dobMonth < 1 || dobMonth > 12) return false;
        if (dobDay < 1 || dobDay > 31) return false;
        if (sex != 'M' && sex != 'F') return false;
        return true;
    }
};

class SettingsStore {
public:
    void begin() {
        prefs.begin("deathclock", false);
    }

    Settings load() {
        Settings s;
        s.ssid       = prefs.getString("ssid", "");
        s.password   = prefs.getString("pass", "");
        s.dobYear    = prefs.getInt("dy", 0);
        s.dobMonth   = prefs.getInt("dm", 0);
        s.dobDay     = prefs.getInt("dd", 0);
        s.sex        = (char)prefs.getUChar("sex", 'M');
        s.theme      = prefs.getUChar("theme", 0);
        s.configured = prefs.getBool("cfg", false);
        return s;
    }

    void save(const Settings& s) {
        prefs.putString("ssid", s.ssid);
        prefs.putString("pass", s.password);
        prefs.putInt("dy", s.dobYear);
        prefs.putInt("dm", s.dobMonth);
        prefs.putInt("dd", s.dobDay);
        prefs.putUChar("sex", (uint8_t)s.sex);
        prefs.putUChar("theme", s.theme);
        prefs.putBool("cfg", true);
    }

    void clear() {
        prefs.clear();
    }

private:
    Preferences prefs;
};
