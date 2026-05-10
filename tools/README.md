# Asset generation tools

Regenerates the PROGMEM headers consumed by the firmware. Re-run after font
or icon source changes; the resulting `.h` files are committed to the repo so
no asset-gen step is needed in the build.

## Fonts (Outfit VLW)

```bash
pip install --break-system-packages freetype-py
# place TTFs at: Outfit-{Regular,Medium,SemiBold,Bold}.ttf next to make_vlw.py
python3 make_vlw.py        # → outfit_*.vlw
python3 vlw_to_h.py        # → include/fonts_outfit.h
```

The 96px Bold and 48px SemiBold fonts are restricted to digits + colon to
keep flash usage down. Body sizes (22/16/12) contain full printable ASCII.

Outfit is licensed under SIL OFL 1.1 (see Outfit-Fonts on GitHub).

## Icons

```bash
python3 make_glyphs.py     # → include/icons.h
```

Icons are defined as ASCII art in `make_glyphs.py` and packed 1-bit row-major
MSB-first. The firmware tints them per-theme at blit time.
