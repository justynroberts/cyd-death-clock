// main.cpp - CYD Memento Mori
//
// Boot flow:
//   1. Init NVS settings
//   2. If unconfigured OR touch held >2s during boot: launch captive portal
//   3. Connect to WiFi, sync NTP
//   4. Render loop - 1 Hz update, touch toggles between modes,
//      long-press (3s) re-enters config

#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <time.h>

#include "settings.h"
#include "portal.h"
#include "display.h"
#include "stats.h"
#include "timezones.h"

// CYD touch pins (XPT2046 on HSPI)
#define XPT_CS    33
#define XPT_IRQ   36

// Use a dedicated SPI bus for the touch controller. The TFT uses VSPI
// configured via TFT_eSPI build flags; touch goes on a separate SPI to avoid
// fighting the TFT for the bus.
SPIClass touchSPI(VSPI);
XPT2046_Touchscreen ts(XPT_CS, XPT_IRQ);
TFT_eSPI tft = TFT_eSPI();

SettingsStore store;
DeathDisplay  disp(tft);
CaptivePortal remote(store);   // long-lived for ops mode (web UI on home WiFi)

int      viewMode      = 0;        // 0 = remaining, 1 = lived
uint32_t lastRender    = 0;
uint32_t touchDownAt   = 0;
bool     touchActive   = false;
bool     longPressFired = false;

static bool waitForTime(uint32_t timeoutMs) {
    uint32_t start = millis();
    time_t now = 0;
    while (millis() - start < timeoutMs) {
        time(&now);
        if (now > 1700000000) return true;  // got real epoch (post-2023)
        delay(200);
    }
    return false;
}

static bool connectWifi(const Settings& s, uint32_t timeoutMs) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(s.ssid.c_str(), s.password.c_str());
    Serial.printf("[wifi] Connecting to '%s'...\n", s.ssid.c_str());
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
        delay(250);
        Serial.print(".");
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[wifi] Connected: %s\n", WiFi.localIP().toString().c_str());
        return true;
    }
    Serial.println("[wifi] Connect failed");
    return false;
}

static void launchPortal() {
    CaptivePortal portal(store);
    disp.showSetup(portal.apName());
    portal.run();   // never returns - reboots on save
}

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n=== CYD Memento Mori ===");

    store.begin();

    // Touch on its own SPI
    touchSPI.begin(25, 39, 32, XPT_CS);
    ts.begin(touchSPI);
    ts.setRotation(1);

    disp.begin(1, 80);
    disp.showStatus("Memento Mori", "starting...");

    // If touch held during boot, force config mode
    delay(800);
    if (ts.tirqTouched() && ts.touched()) {
        Serial.println("[boot] touch held -> portal");
        launchPortal();
    }

    Settings s = store.load();
    if (!s.isValid()) {
        Serial.println("[boot] no valid settings -> portal");
        launchPortal();
    }

    // WiFi + NTP
    disp.showStatus("Connecting WiFi", s.ssid);
    if (!connectWifi(s, 20000)) {
        disp.showStatus("WiFi failed", "Touch to reconfigure");
        // Wait 10s for user to touch, otherwise retry on reboot
        uint32_t until = millis() + 10000;
        while (millis() < until) {
            if (ts.tirqTouched() && ts.touched()) launchPortal();
            delay(50);
        }
        ESP.restart();
    }

    // POSIX timezone from settings — DST handled automatically.
    const TimeZone& tz = getTimezone(s.tz);
    Serial.printf("[time] tz: %s (%s)\n", tz.label, tz.posix);
    configTzTime(tz.posix, "pool.ntp.org", "time.nist.gov");
    disp.showStatus("Syncing time", "NTP...");
    if (!waitForTime(15000)) {
        disp.showStatus("Time sync failed", "Will retry");
        delay(2000);
    }

    // Start the remote web UI + mDNS responder. From now on the device is
    // reachable at http://<hostname>.local/ on the home network.
    remote.runRemote();
    disp.showStatus(remote.hostname() + ".local",
                    WiFi.localIP().toString());
    delay(3500);
}

void loop() {
    Settings s = store.load();
    uint32_t now = millis();

    // --- Touch handling ---
    bool touched = ts.tirqTouched() && ts.touched();

    if (touched && !touchActive) {
        touchActive    = true;
        touchDownAt    = now;
        longPressFired = false;
    } else if (touched && touchActive) {
        if (!longPressFired && (now - touchDownAt) > 3000) {
            // Long press - reconfigure
            longPressFired = true;
            Serial.println("[touch] long press -> portal");
            launchPortal();
        }
    } else if (!touched && touchActive) {
        // Released
        if (!longPressFired && (now - touchDownAt) > 50 && (now - touchDownAt) < 800) {
            // Short tap - toggle mode
            viewMode = 1 - viewMode;
            lastRender = 0;  // force redraw
        }
        touchActive = false;
    }

    // --- 1 Hz render + remote API live data ---
    if (now - lastRender >= 1000) {
        lastRender = now;
        time_t epoch;
        time(&epoch);
        if (epoch > 1700000000) {
            LifeStats st = computeStats(s, epoch, viewMode);
            disp.render(s, st, viewMode);
            remote.setLive(st.years, st.remDays, st.hrs, st.mins, st.secs,
                           st.pct, viewMode);
        } else {
            disp.showStatus("Waiting for time", "...");
        }
    }

    // Service web requests every loop iteration so /api stays responsive
    remote.tick();

    delay(20);
}
