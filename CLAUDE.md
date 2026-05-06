# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

ESP32 firmware for the **ESP32-2432S028R** ("Cheap Yellow Display", 2.8" ILI9341 + XPT2046 resistive touch). A memento-mori countdown clock: enter DOB + sex via captive portal, device fetches NTP time and renders time-remaining (or time-lived) every second using UK ONS 2020-2022 period life tables.

## Build / flash / monitor

PlatformIO project (env: `cyd`). No Arduino IDE setup needed.

```bash
pio run                      # build only
pio run -t upload            # build + flash over USB
pio device monitor           # serial @ 115200
pio run -t clean             # nuke .pio build dir
```

There are no unit tests — this is firmware, validation is on-device.

## Architecture

Single-binary Arduino-framework firmware. Boot flow lives in `src/main.cpp`; everything else is header-only classes in `include/`.

```
main.cpp ──┬── SettingsStore (settings.h)   NVS-backed config (Preferences lib, namespace "deathclock")
           ├── CaptivePortal (portal.h)     AP + DNSServer hijack + WebServer on 192.168.4.1, blocks until save then reboots
           │     └── portal_html.h          static HTML/JS for the config page
           ├── DeathDisplay (display.h)     TFT_eSPI rendering, owns backlight PWM on GPIO21 (LEDC ch 0)
           └── life_tables.h                LE_MALE[101]/LE_FEMALE[101] + linear-interp lookup
```

Boot decision tree (in `setup()`):
1. Init NVS, init touch on its own SPI bus, init display.
2. If touch is held during boot **or** `Settings::isValid()` is false → `launchPortal()` (never returns; `ESP.restart()` after save).
3. Otherwise connect WiFi (20s timeout → portal fallback), then `configTzTime` for UK with auto-BST, NTP sync (15s).

Render loop (`loop()`): 1 Hz tick. Touch FSM tracks press duration — short tap (50–800ms) toggles `viewMode` between "remaining" and "lived"; long-press (>3s) re-enters portal.

## Hardware-specific things that bite

- **Touch is on a separate SPI bus from the TFT.** `touchSPI.begin(25, 39, 32, 33)` is set up explicitly in `main.cpp`. The TFT bus is configured entirely via `-D` flags in `platformio.ini` (no `User_Setup.h` editing). Don't try to share buses — it deadlocks.
- **All TFT_eSPI config lives in `platformio.ini` `build_flags`.** Changing pins/driver means editing those, not the library's `User_Setup.h`. `USER_SETUP_LOADED=1` suppresses the lib's default include.
- **Backlight is PWM via LEDC on GPIO21**, channel 0, 5kHz, 8-bit. `DeathDisplay::begin(rotation, brightnessPct)` owns this — don't `digitalWrite(21, …)` elsewhere.
- **Captive portal probe paths matter.** iOS hits `/hotspot-detect.html`, Android `/generate_204` + `/gen_204`, Windows `/connecttest.txt` + `/ncsi.txt`. All are explicitly handled in `portal.h` plus `onNotFound` redirects everything else. Removing these breaks auto-popup.
- **Time check uses `epoch > 1700000000`** as the "NTP synced" signal — anything below that is the ESP32's pre-NTP boot epoch.
- **Timezone is hardcoded** to `GMT0BST,M3.5.0/1,M10.5.0/2` (UK with auto-BST) in `setup()`. If you make TZ configurable, do it via `Settings` and re-call `configTzTime` after load.

## Settings persistence

`Preferences` namespace `"deathclock"`, keys: `ssid`, `pass`, `dy`, `dm`, `dd`, `sex` (uchar), `cfg` (bool). Validation in `Settings::isValid()` — note `dobYear` upper bound is 2025; bump it if you're working past then.

To wipe config without reflashing: long-press touch for 3s during normal operation (re-enters portal), or hold touch during boot.

## Life-table data

`LE_MALE`/`LE_FEMALE` are period LE (conservative) from ONS 2020-2022, indexed by integer age 0–100. `lifeExpectancyRemaining(ageYears, sex)` linearly interpolates between integer ages. Beyond 100 it clamps. If swapping in cohort tables or other countries (US SSA, etc. — listed in README TODO), keep the same `[101]` shape and the lookup keeps working.

## Conventions

- Header-only classes; no separate `.cpp` files in `src/` other than `main.cpp`. Keep it that way unless something genuinely needs a TU.
- Serial logs are tagged `[wifi]`, `[portal]`, `[boot]`, `[touch]` — match the convention when adding new log sites.
- The `data/` directory exists for SPIFFS/LittleFS but is currently empty; portal HTML is compiled in via `portal_html.h`, not served from flash FS.
