# CYD Death Clock

A memento mori countdown clock for the ESP32-2432S028R (the classic 2.8" "Cheap Yellow Display"). Calculates expected time remaining using UK ONS National Life Tables and counts down second by second.

![status: ready to flash]

## What it does

- On first boot, brings up a captive portal AP called `CYD-DeathClock-XXXX`
- You connect with a phone, set WiFi credentials, your DOB and sex
- Saves to NVS, reboots, connects to your WiFi, syncs time over NTP
- Shows time remaining (or time lived) as years / days / HH:MM:SS, plus a percentage-of-life progress bar and projected death date
- Tap the screen to toggle between "remaining" and "lived" views
- Long press (3s) to re-enter config mode

## Method

Uses **UK ONS National Life Tables 2020-2022 period life expectancy**. Period LE assumes mortality stays at current levels - so it's deliberately conservative compared to cohort projections. The lookup is interpolated linearly between integer ages for sub-year precision.

Obviously this is not medical or actuarial advice. It's a clock. The grim reaper does not consult Github before scheduling visits.

## Build

```bash
pio run -t upload
pio device monitor
```

Requires PlatformIO. The TFT_eSPI library is configured entirely via `build_flags` in `platformio.ini` so no fiddling with `User_Setup.h` in the lib folder.

## First-time setup

1. Flash the firmware
2. Open WiFi settings on your phone, connect to `CYD-DeathClock-XXXX`
3. Captive portal should pop up automatically (it works on iOS, Android, modern Windows). If not, browse to `http://192.168.4.1`
4. Hit "Scan nearby" to pick your network, enter password
5. Enter DOB (year/month/day) and sex
6. Save - device reboots and starts counting

## Re-configuring

Two ways:

- Hold a finger on the touchscreen while powering on (release after ~2s once you see "Setup mode")
- Long-press (3s) during normal operation

## Pin reference (ESP32-2432S028R)

| Function | GPIO |
|----------|------|
| TFT MISO | 12 |
| TFT MOSI | 13 |
| TFT SCK | 14 |
| TFT CS | 15 |
| TFT DC | 2 |
| TFT BL | 21 |
| Touch CS | 33 |
| Touch IRQ | 36 |
| Touch CLK | 25 |
| Touch MISO | 39 |
| Touch MOSI | 32 |

The touch controller (XPT2046) lives on a separate SPI bus from the TFT - this is the bit that catches everyone out. The code sets up `touchSPI` with explicit pins so it doesn't fight the display bus.

## Gotchas hit while building this

- TFT_eSPI's `User_Setup.h` is a pain when you've got multiple boards. Putting all the `-D` flags in `platformio.ini` keeps the project self-contained
- The `21` backlight pin uses LEDC PWM for brightness - direct `digitalWrite` works but PWM lets you dim it without touching TFT settings
- iOS captive portal detection hits `/hotspot-detect.html`, Android hits `/generate_204`, Windows hits `/connecttest.txt`. All three need handlers or the portal won't auto-pop
- Setting timezone via `configTzTime` with a POSIX TZ string handles BST automatically. `configTime` with an offset doesn't

## File layout

```
.
├── platformio.ini          # board + TFT_eSPI build flags
├── include/
│   ├── config.h            # (removed - using captive portal now)
│   ├── settings.h          # NVS-backed Settings struct
│   ├── life_tables.h       # ONS LE lookup
│   ├── portal_html.h       # captive portal HTML page
│   ├── portal.h            # AP + DNS + web server
│   └── display.h           # TFT rendering
└── src/
    └── main.cpp            # boot flow, touch, render loop
```

## TODO / improvements

- Per-country life table selection (US SSA, etc.)
- Cohort-projected LE option for the optimists
- Mounjaro / lifestyle adjustment factors (only half joking)
- OTA updates
- Battery indicator if powered from a Li-ion pack
