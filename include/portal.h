// portal.h - Web server: captive setup mode + remote ops mode
//
// Two modes share most routes (/scan, /themes, /save, /api, /config):
//   run()        — blocking AP + DNS hijack, used during first-time setup
//   runRemote()  — non-blocking STA mode + mDNS, served while clock runs
//   tick()       — must be called from main loop in remote mode
//
// /save in either mode reboots the device after a brief delay so the
// browser receives the response.
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "settings.h"
#include "themes.h"
#include "timezones.h"
#include "portal_html.h"
#include "status_html.h"

class CaptivePortal {
public:
    CaptivePortal(SettingsStore& store) : _store(store), _server(80) {}

    // ── Setup mode (blocking) ────────────────────────────────────────────
    void run() {
        WiFi.mode(WIFI_AP);
        uint64_t mac = ESP.getEfuseMac();
        char apName[32];
        snprintf(apName, sizeof(apName), "CYD-DeathClock-%04X", (uint16_t)(mac & 0xFFFF));

        IPAddress apIP(192, 168, 4, 1);
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
        WiFi.softAP(apName);
        Serial.printf("[portal] AP up: %s -> http://192.168.4.1\n", apName);
        _apName = String(apName);

        _dns.start(53, "*", apIP);

        setupSharedRoutes();
        // OS captive-portal probes — only register in setup mode
        auto probeRedir = [this]() { redirect(); };
        _server.on("/generate_204", HTTP_GET, probeRedir);          // Android
        _server.on("/gen_204",      HTTP_GET, probeRedir);
        _server.on("/hotspot-detect.html", HTTP_GET, probeRedir);   // iOS/macOS
        _server.on("/library/test/success.html", HTTP_GET, probeRedir);
        _server.on("/connecttest.txt", HTTP_GET, probeRedir);       // Windows
        _server.on("/redirect",     HTTP_GET, probeRedir);
        _server.on("/ncsi.txt",     HTTP_GET, probeRedir);
        _server.onNotFound(probeRedir);

        _server.begin();

        _saveRequested = false;
        while (!_saveRequested) {
            _dns.processNextRequest();
            _server.handleClient();
            delay(2);
        }

        Serial.println("[portal] Settings saved, rebooting in 3s");
        delay(3000);
        ESP.restart();
    }

    // ── Remote ops mode (non-blocking) ──────────────────────────────────
    void runRemote() {
        // Stable hostname tied to MAC suffix.
        uint64_t mac = ESP.getEfuseMac();
        char hbuf[32];
        snprintf(hbuf, sizeof(hbuf), "deathclock-%04x", (uint16_t)(mac & 0xFFFF));
        _hostname = String(hbuf);

        WiFi.setHostname(_hostname.c_str());
        if (MDNS.begin(_hostname.c_str())) {
            MDNS.addService("http", "tcp", 80);
            Serial.printf("[remote] mdns: http://%s.local/\n", _hostname.c_str());
        }

        setupSharedRoutes();
        _server.onNotFound([this]() {
            _server.send(404, "text/plain", "Not found");
        });
        _server.begin();
        Serial.printf("[remote] http://%s/\n", WiFi.localIP().toString().c_str());
        _remoteActive = true;
    }

    // Call from main loop while in remote mode.
    void tick() {
        if (!_remoteActive) return;
        _server.handleClient();
        // /save sets _saveRequested; reboot after a short delay so the HTTP
        // response can flush.
        if (_saveRequested) {
            if (_rebootAt == 0) _rebootAt = millis() + 1200;
            if (millis() >= _rebootAt) {
                Serial.println("[remote] save reboot");
                ESP.restart();
            }
        }
    }

    // Live data fed in by the main loop each render tick. Used by /api.
    void setLive(long years, long days, long hrs, long mins, long secs,
                 float pct, int mode) {
        _liveYears = years; _liveDays = days; _liveHrs = hrs;
        _liveMins  = mins;  _liveSecs = secs;
        _livePct   = pct;   _liveMode = mode;
    }

    const String& apName()   const { return _apName; }
    const String& hostname() const { return _hostname; }

private:
    SettingsStore& _store;
    DNSServer      _dns;
    WebServer      _server;
    bool           _saveRequested = false;
    bool           _remoteActive  = false;
    uint32_t       _rebootAt      = 0;
    String         _apName;
    String         _hostname;

    // Live data snapshot (updated each second by main loop).
    long  _liveYears = 0, _liveDays = 0, _liveHrs = 0, _liveMins = 0, _liveSecs = 0;
    float _livePct   = 0;
    int   _liveMode  = 0;

    // ── Routes shared between captive and remote ────────────────────────
    void setupSharedRoutes() {
        _server.on("/",        HTTP_GET, [this]() {
            // Captive mode: portal config form. Remote: live status page.
            if (_remoteActive) _server.send_P(200, "text/html", STATUS_HTML);
            else               _server.send_P(200, "text/html", PORTAL_HTML);
        });
        _server.on("/config", HTTP_GET, [this]() {
            _server.send_P(200, "text/html", PORTAL_HTML);
        });
        _server.on("/scan",   HTTP_GET,  [this]() { handleScan(); });
        _server.on("/themes", HTTP_GET,  [this]() { handleThemes(); });
        _server.on("/timezones", HTTP_GET, [this]() { handleTimezones(); });
        _server.on("/api",    HTTP_GET,  [this]() { handleApi(); });
        _server.on("/save",   HTTP_POST, [this]() { handleSave(); });
    }

    void redirect() {
        _server.sendHeader("Location", "http://192.168.4.1/", true);
        _server.send(302, "text/plain", "");
    }

    void handleScan() {
        int n = WiFi.scanNetworks();
        String json = "[";
        for (int i = 0; i < n && i < 20; i++) {
            if (i > 0) json += ",";
            String ssid = WiFi.SSID(i);
            ssid.replace("\"", "\\\"");
            json += "{\"ssid\":\"" + ssid + "\",";
            json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
            json += "\"lock\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false");
            json += "}";
        }
        json += "]";
        WiFi.scanDelete();
        _server.send(200, "application/json", json);
    }

    void handleTimezones() {
        String json = "[";
        for (uint8_t i = 0; i < TIMEZONE_COUNT; i++) {
            if (i > 0) json += ",";
            json += "{\"idx\":" + String(i)
                  + ",\"label\":\"" + String(TIMEZONES[i].label) + "\""
                  + ",\"posix\":\"" + String(TIMEZONES[i].posix) + "\"}";
        }
        json += "]";
        _server.send(200, "application/json", json);
    }

    void handleThemes() {
        String json = "[";
        for (uint8_t i = 0; i < THEME_COUNT; i++) {
            if (i > 0) json += ",";
            const Theme& t = THEMES[i];
            char hex[3][8];
            snprintf(hex[0], 8, "#%06X", rgb565ToHex(t.bg));
            snprintf(hex[1], 8, "#%06X", rgb565ToHex(t.accent));
            snprintf(hex[2], 8, "#%06X", rgb565ToHex(t.text));
            json += "{\"idx\":" + String(i)
                  + ",\"name\":\""   + String(t.name)  + "\""
                  + ",\"label\":\""  + String(t.label) + "\""
                  + ",\"bg\":\""     + String(hex[0])  + "\""
                  + ",\"accent\":\"" + String(hex[1])  + "\""
                  + ",\"text\":\""   + String(hex[2])  + "\"}";
        }
        json += "]";
        _server.send(200, "application/json", json);
    }

    // Returns current settings (no password) + live counter data so the
    // status page and the config page (pre-fill) can share one endpoint.
    void handleApi() {
        Settings s = _store.load();
        const Theme& th = getTheme(s.theme);
        char dob[16];
        snprintf(dob, sizeof(dob), "%04d-%02d-%02d", s.dobYear, s.dobMonth, s.dobDay);

        char accentHex[8], bgHex[8], textHex[8];
        snprintf(accentHex, sizeof(accentHex), "#%06X", rgb565ToHex(th.accent));
        snprintf(bgHex,     sizeof(bgHex),     "#%06X", rgb565ToHex(th.bg));
        snprintf(textHex,   sizeof(textHex),   "#%06X", rgb565ToHex(th.text));

        String j = "{";
        j += "\"hostname\":\"" + _hostname + "\"";
        j += ",\"ip\":\"" + WiFi.localIP().toString() + "\"";
        j += ",\"ssid\":\"" + s.ssid + "\"";
        j += ",\"dob\":\"" + String(dob) + "\"";
        j += ",\"sex\":\"" + String(s.sex) + "\"";
        j += ",\"theme\":" + String(s.theme);
        j += ",\"theme_label\":\"" + String(th.label) + "\"";
        j += ",\"tz\":" + String(s.tz);
        j += ",\"tz_label\":\"" + String(getTimezone(s.tz).label) + "\"";
        j += ",\"colors\":{\"bg\":\"" + String(bgHex) +
             "\",\"accent\":\"" + String(accentHex) +
             "\",\"text\":\""   + String(textHex)   + "\"}";
        j += ",\"mode\":" + String(_liveMode);
        j += ",\"years\":" + String(_liveYears);
        j += ",\"days\":"  + String(_liveDays);
        j += ",\"hrs\":"   + String(_liveHrs);
        j += ",\"mins\":"  + String(_liveMins);
        j += ",\"secs\":"  + String(_liveSecs);
        j += ",\"pct\":"   + String(_livePct, 6);
        j += "}";
        _server.send(200, "application/json", j);
    }

    void handleSave() {
        String body = _server.arg("plain");
        Settings s;
        s.ssid     = extractStr(body, "ssid");
        s.password = extractStr(body, "pass");
        s.dobYear  = extractInt(body, "dy");
        s.dobMonth = extractInt(body, "dm");
        s.dobDay   = extractInt(body, "dd");
        String sx  = extractStr(body, "sex");
        s.sex      = sx.length() ? sx[0] : 'M';
        int themeIdx = extractInt(body, "theme");
        s.theme    = (themeIdx < 0 || themeIdx >= THEME_COUNT) ? 0 : (uint8_t)themeIdx;
        int tzIdx  = extractInt(body, "tz");
        s.tz       = (tzIdx < 0 || tzIdx >= TIMEZONE_COUNT) ? 1 : (uint8_t)tzIdx;
        s.configured = true;

        // Empty password is allowed in remote mode if the user only wants to
        // change theme/DOB and leave the existing WiFi creds alone.
        if (_remoteActive && s.password.length() == 0) {
            Settings cur = _store.load();
            s.password = cur.password;
        }

        if (!s.isValid()) {
            _server.send(400, "text/plain", "Invalid settings");
            return;
        }

        _store.save(s);
        _server.send(200, "text/plain", "OK");
        _saveRequested = true;
        // tick() (or run()'s loop) will reboot after the response flushes.
    }

    static uint32_t rgb565ToHex(uint16_t c) {
        uint8_t r = (c >> 11) & 0x1F;
        uint8_t g = (c >> 5)  & 0x3F;
        uint8_t b =  c        & 0x1F;
        uint8_t r8 = (r << 3) | (r >> 2);
        uint8_t g8 = (g << 2) | (g >> 4);
        uint8_t b8 = (b << 3) | (b >> 2);
        return ((uint32_t)r8 << 16) | ((uint32_t)g8 << 8) | b8;
    }

    static String extractStr(const String& body, const char* key) {
        String pat = String("\"") + key + "\":\"";
        int i = body.indexOf(pat);
        if (i < 0) return "";
        i += pat.length();
        int j = body.indexOf('"', i);
        if (j < 0) return "";
        return body.substring(i, j);
    }
    static int extractInt(const String& body, const char* key) {
        String pat = String("\"") + key + "\":";
        int i = body.indexOf(pat);
        if (i < 0) return 0;
        i += pat.length();
        while (i < (int)body.length() && (body[i] == ' ' || body[i] == '"')) i++;
        int j = i;
        while (j < (int)body.length() && (isdigit(body[j]) || body[j] == '-')) j++;
        return body.substring(i, j).toInt();
    }
};
