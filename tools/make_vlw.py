#!/usr/bin/env python3
"""
Generate TFT_eSPI .vlw smooth-font files from a TTF.

VLW format (big-endian throughout):
  Header (16 bytes):
    uint32 gCount        - number of glyphs
    uint32 vlw_version   - format version (currently 0x000B = 11)
    uint32 fontHeight    - max ascent in pixels
    uint32 fontDescent   - max descent in pixels (positive)
  For each glyph (28 bytes header + width*height alpha bytes):
    uint32 unicode       - codepoint
    uint32 height        - bitmap height
    uint32 width         - bitmap width
    uint32 xAdvance      - cursor advance
    int32  dY            - distance from cursor baseline up to bitmap top
                            (positive value; subtracted from baseline by renderer)
    int32  dX            - left bearing (signed)
    uint32 reserved      - 0
  Bitmap: width*height bytes, alpha 0..255.
"""
import freetype, struct, sys, os

# Full printable ASCII (used by body / title fonts).
GLYPHS_ALL = list(range(0x20, 0x7F))  # 0x20 ... 0x7E

# Minimal set for hero (digits) and clock (digits + colon).
# Including space + a couple of punctuation chars in case we need them.
GLYPHS_DIGITS = [ord(c) for c in "0123456789:., -%"]

def render_vlw(ttf_path: str, px: int, out_path: str, glyphs=GLYPHS_ALL):
    face = freetype.Face(ttf_path)
    face.set_pixel_sizes(0, px)

    # First pass: render every glyph to capture metrics.
    rendered = []
    max_ascent = 0
    max_descent = 0
    for cp in glyphs:
        face.load_char(chr(cp), freetype.FT_LOAD_RENDER | freetype.FT_LOAD_TARGET_NORMAL)
        bm = face.glyph.bitmap
        w, h = bm.width, bm.rows
        top = face.glyph.bitmap_top    # distance from baseline up to bitmap top (pixels, positive above)
        left = face.glyph.bitmap_left  # left bearing
        adv = face.glyph.advance.x >> 6
        ascent = top
        descent = max(0, h - top)
        max_ascent  = max(max_ascent, ascent)
        max_descent = max(max_descent, descent)
        # Copy buffer eagerly — face state mutates each load_char.
        pixels = bytes(bm.buffer)
        rendered.append((cp, w, h, adv, top, left, pixels))

    with open(out_path, "wb") as f:
        # 24-byte header: gCount, version, yAdvance (orig px size), padding,
        # ascent, descent. (TFT_eSPI 2.5.x reads 6 fields here, even though it
        # only uses count/ascent/descent — the others must be present.)
        f.write(struct.pack(">IIIIII",
                            len(rendered),
                            0x0000000B,   # version
                            px,           # yAdvance / original font size
                            0,            # padding
                            max_ascent,
                            max_descent))
        # Glyph table
        for cp, w, h, adv, top, left, pixels in rendered:
            # TFT_eSPI's "dY" stored as positive: ascent (rows above baseline).
            f.write(struct.pack(">IIIIiiI",
                                cp, h, w, adv, top, left, 0))
            f.write(pixels)
    print(f"  {out_path}: {len(rendered)} glyphs, h+d={max_ascent}+{max_descent}, "
          f"size={os.path.getsize(out_path)} B")


# (TTF, px, output, glyph-set)
SPECS = [
    ("Outfit-Bold.ttf",     96, "outfit_bold_96.vlw",     GLYPHS_DIGITS),
    ("Outfit-SemiBold.ttf", 48, "outfit_semibold_48.vlw", GLYPHS_DIGITS),
    ("Outfit-Medium.ttf",   22, "outfit_medium_22.vlw",   GLYPHS_ALL),
    ("Outfit-Regular.ttf",  16, "outfit_regular_16.vlw",  GLYPHS_ALL),
    ("Outfit-Medium.ttf",   12, "outfit_medium_12.vlw",   GLYPHS_ALL),
]

if __name__ == "__main__":
    base = os.path.dirname(os.path.abspath(__file__))
    for ttf, px, out, glyphs in SPECS:
        ttf_path = os.path.join(base, ttf)
        out_path = os.path.join(base, out)
        print(f"Rendering {ttf} @ {px}px -> {out}")
        render_vlw(ttf_path, px, out_path, glyphs)
