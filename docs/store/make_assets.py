#!/usr/bin/env python3
"""Generate Cardigan's Rebble store assets at the portal's exact sizes.

    python3 docs/store/make_assets.py        (requires Pillow)

Outputs to docs/store/assets/:
    banner_720x320.png   Appstore banner (required for apps)
    icon_large_144.png   large app-locker icon
    icon_small_48.png    small app-locker icon

The banner embeds a real render from the test suite, so marketing art can
never drift from what the app actually draws.
"""
import os

from PIL import Image, ImageDraw, ImageFont

ORANGE = (255, 76, 0)
CHARCOAL = (23, 25, 31)
CREAM = (251, 247, 239)
BG = (18, 20, 27)

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT = os.path.join(ROOT, "docs", "store", "assets")
RENDER = os.path.join(ROOT, "test", "out", "emery_card_0.pgm")


def font(size, bold=True):
    for path in (
        "/System/Library/Fonts/Supplemental/Arial{}.ttf".format(" Bold" if bold else ""),
        "/usr/share/fonts/truetype/dejavu/DejaVuSans{}.ttf".format("-Bold" if bold else ""),
    ):
        if os.path.exists(path):
            return ImageFont.truetype(path, size)
    return ImageFont.load_default()


def mark(size, detail="full"):
    """Cardigan mark: a loyalty card wearing a knit collar."""
    s = size
    im = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    u = s / 100.0
    x0, y0, x1, y1 = 6 * u, 14 * u, 94 * u, 86 * u
    radius = 10 * u
    d.rounded_rectangle([x0, y0, x1, y1], radius=radius, fill=CREAM,
                        outline=CHARCOAL, width=max(2, int(4.5 * u)))

    band = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    bd = ImageDraw.Draw(band)
    bd.rectangle([x0, y0, x1, y0 + 26 * u], fill=ORANGE + (255,))
    if detail == "full":
        for row in (y0 + 11 * u, y0 + 21 * u):
            step, xx = 13 * u, x0
            while xx < x1:
                bd.arc([xx, row - 7 * u, xx + step, row + 5 * u], 200, 340,
                       fill=CREAM + (235,), width=max(2, int(2.6 * u)))
                xx += step
    mask = Image.new("L", (s, s), 0)
    ImageDraw.Draw(mask).rounded_rectangle([x0, y0, x1, y1], radius=radius, fill=255)
    im.paste(band, (0, 0),
             Image.composite(band.getchannel("A"), Image.new("L", (s, s), 0), mask))

    d = ImageDraw.Draw(im)
    top, bot = y0 + 36 * u, y1 - 9 * u
    bars = ([(14, 4), (20, 2.5), (25, 5), (33, 2.5), (38, 3), (44, 6),
             (53, 2.5), (58, 4), (65, 2.5), (70, 5), (78, 3)] if detail == "full"
            else [(15, 7), (27, 4), (36, 9), (50, 4), (58, 7), (70, 4)])
    for bx, bw in bars:
        d.rectangle([x0 + bx * u, top, x0 + (bx + bw) * u, bot], fill=CHARCOAL)
    return im


def main():
    os.makedirs(OUT, exist_ok=True)

    large = Image.new("RGB", (144, 144), (250, 246, 238))
    m = mark(126, "full")
    large.paste(m, (9, 9), m)
    large.save(os.path.join(OUT, "icon_large_144.png"))

    small = Image.new("RGB", (48, 48), (250, 246, 238))
    m = mark(48, "simple")          # thicker bars survive the shrink
    small.paste(m, (0, 0), m)
    small.save(os.path.join(OUT, "icon_small_48.png"))

    bn = Image.new("RGB", (720, 320), BG)
    d = ImageDraw.Draw(bn)
    for y in range(0, 320, 22):                     # knit texture
        for x in range(-20, 740, 36):
            d.arc([x, y, x + 36, y + 25], 200, 340, fill=(28, 31, 40), width=2)
    m = mark(150, "full")
    bn.paste(m, (34, 85), m)
    d.text((196, 86), "Cardigan", font=font(58), fill=CREAM)
    d.text((200, 156), "THE WALLET YOU WEAR", font=font(19), fill=ORANGE)
    d.text((200, 192), "Loyalty cards as scannable", font=font(16, False), fill=(155, 160, 175))
    d.text((200, 214), "barcodes & QR. One press.", font=font(16, False), fill=(155, 160, 175))

    if os.path.exists(RENDER):
        shot = Image.open(RENDER).convert("RGB").resize((160, 182), Image.LANCZOS)
        wx, wy = 528, 69
        d.rounded_rectangle([wx - 19, wy - 19, wx + 179, wy + 201], radius=26,
                            fill=(58, 61, 70), outline=(92, 96, 107), width=3)
        d.rounded_rectangle([wx - 9, wy - 9, wx + 169, wy + 191], radius=14, fill=(8, 9, 11))
        bn.paste(shot, (wx, wy))
    else:
        print("note: run the test suite first to embed a real render")
    bn.save(os.path.join(OUT, "banner_720x320.png"))

    for name in ("banner_720x320", "icon_large_144", "icon_small_48"):
        im = Image.open(os.path.join(OUT, name + ".png"))
        print(f"{name}.png  {im.size[0]}x{im.size[1]}")


if __name__ == "__main__":
    main()
