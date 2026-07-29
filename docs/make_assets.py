#!/usr/bin/env python3
"""Regenerate every Cardigan brand and store asset.

    make assets            (or: python3 docs/make_assets.py)

Everything visual in this repo comes out of this one script, so the brand
can never drift between the README, the store listing and the app icon.
Marketing images embed *real* frames from the test suite rather than
mock-ups — run `make render` first if test/out/ is empty.

Outputs
    resources/images/menu_icon.png     25x25  1-bit launcher silhouette
    docs/brand/mark.png                the card mark on its own
    docs/brand/banner.png              1280x400 README hero
    docs/store/assets/banner_720x320.png
    docs/store/assets/icon_large_144.png
    docs/store/assets/icon_small_48.png
    docs/screenshots/framed_*.png      marketing shots (GitHub / social)
    docs/screenshots/all_platforms.png every geometry, round ones masked
"""
import os
import sys

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    sys.exit("Pillow is required:  pip install pillow")

# ---------------------------------------------------------------- palette
ORANGE = (255, 76, 0)        # Yarn Orange   #FF4C00
CHARCOAL = (23, 25, 31)      # Charcoal      #17191F
CREAM = (251, 247, 239)      # Cream         #FBF7EF
INK = (18, 20, 27)           # page/backdrop #12141B
MUTED = (155, 160, 175)

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
BRAND = os.path.join(ROOT, "docs", "brand")
STORE = os.path.join(ROOT, "docs", "store", "assets")
SHOTS = os.path.join(ROOT, "docs", "screenshots")
FRAMES = os.path.join(ROOT, "test", "out")


def font(size, bold=True):
    for path in (
        "/System/Library/Fonts/Supplemental/Arial{}.ttf".format(" Bold" if bold else ""),
        "/usr/share/fonts/truetype/dejavu/DejaVuSans{}.ttf".format("-Bold" if bold else ""),
    ):
        if os.path.exists(path):
            return ImageFont.truetype(path, size)
    return ImageFont.load_default()


def frame(name):
    """A real render from the test suite, or None if it hasn't been run."""
    path = os.path.join(FRAMES, name)
    return Image.open(path).convert("RGB") if os.path.exists(path) else None


# ------------------------------------------------------------------- mark
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


def knit_backdrop(w, h, colour=(28, 31, 40)):
    im = Image.new("RGB", (w, h), INK)
    d = ImageDraw.Draw(im)
    for y in range(0, h, 26):
        for x in range(-20, w + 40, 44):
            d.arc([x, y, x + 44, y + 30], 200, 340, fill=colour, width=3)
    return im


def watch_mock(im, dst, xy, radius=34):
    """Draw a watch bezel around a screen render and paste it."""
    x, y = xy
    d = ImageDraw.Draw(dst)
    d.rounded_rectangle([x - 26, y - 26, x + im.width + 26, y + im.height + 26],
                        radius=radius, fill=(60, 63, 72), outline=(92, 96, 107), width=3)
    d.rounded_rectangle([x - 12, y - 12, x + im.width + 12, y + im.height + 12],
                        radius=18, fill=(8, 9, 11))
    dst.paste(im, (x, y))


# ------------------------------------------------------------------ icons
def build_launcher_icon():
    """25x25. Pebble reduces menu icons to 1 bit: opaque pixels are drawn in
    the launcher's foreground colour, transparent ones vanish — so this must
    be a silhouette, with the barcode knocked *out* of the card."""
    im = Image.new("RGBA", (25, 25), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    white, clear = (255, 255, 255, 255), (0, 0, 0, 0)
    d.rounded_rectangle([1, 3, 23, 21], radius=3, fill=white)
    d.rectangle([3, 8, 21, 8], fill=clear)                    # collar line
    for x, w in [(3, 1), (5, 2), (8, 1), (10, 1), (12, 2), (15, 1), (17, 1), (19, 2)]:
        d.rectangle([x, 11, x + w - 1, 19], fill=clear)       # bars
    out = os.path.join(ROOT, "resources", "images", "menu_icon.png")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    im.save(out)
    return out


def build_store_icons():
    large = Image.new("RGB", (144, 144), (250, 246, 238))
    m = mark(126, "full")
    large.paste(m, (9, 9), m)
    large.save(os.path.join(STORE, "icon_large_144.png"))

    small = Image.new("RGB", (48, 48), (250, 246, 238))
    m = mark(48, "simple")          # thicker bars survive the shrink
    small.paste(m, (0, 0), m)
    small.save(os.path.join(STORE, "icon_small_48.png"))


# ---------------------------------------------------------------- banners
def build_readme_banner():
    """1280x400 hero for the top of the README."""
    bn = knit_backdrop(1280, 400)
    d = ImageDraw.Draw(bn)
    m = mark(240, "full")
    bn.paste(m, (70, (400 - m.height) // 2), m)
    d.text((360, 110), "Cardigan", font=font(92), fill=CREAM)
    d.text((366, 226), "THE WALLET YOU WEAR", font=font(30), fill=ORANGE)
    d.text((366, 280), "Loyalty cards on your Pebble.", font=font(24, False), fill=MUTED)
    d.text((366, 314), "Scannable Code 128 + QR, offline.", font=font(24, False), fill=MUTED)

    shot = frame("emery_card_0.pgm")
    if shot:
        watch_mock(shot.resize((200, 228), Image.NEAREST), bn, (1000, 60))
    bn.save(os.path.join(BRAND, "banner.png"))


def build_store_banner():
    """720x320 — the size the Rebble portal requires."""
    bn = Image.new("RGB", (720, 320), INK)
    d = ImageDraw.Draw(bn)
    for y in range(0, 320, 22):
        for x in range(-20, 740, 36):
            d.arc([x, y, x + 36, y + 25], 200, 340, fill=(28, 31, 40), width=2)
    m = mark(150, "full")
    bn.paste(m, (34, 85), m)
    d.text((196, 86), "Cardigan", font=font(58), fill=CREAM)
    d.text((200, 156), "THE WALLET YOU WEAR", font=font(19), fill=ORANGE)
    d.text((200, 192), "Loyalty cards as scannable", font=font(16, False), fill=MUTED)
    d.text((200, 214), "barcodes & QR. One press.", font=font(16, False), fill=MUTED)

    shot = frame("emery_card_0.pgm")
    if shot:
        sh = shot.resize((160, 182), Image.LANCZOS)
        wx, wy = 528, 69
        d.rounded_rectangle([wx - 19, wy - 19, wx + 179, wy + 201], radius=26,
                            fill=(58, 61, 70), outline=(92, 96, 107), width=3)
        d.rounded_rectangle([wx - 9, wy - 9, wx + 169, wy + 191], radius=14, fill=(8, 9, 11))
        bn.paste(sh, (wx, wy))
    bn.save(os.path.join(STORE, "banner_720x320.png"))


# ------------------------------------------------------------ screenshots
FRAMED = [
    ("emery_card_0.pgm", "framed_barcode.png",  "One press. Scannable at the till."),
    ("emery_card_3.pgm", "framed_qr.png",       "QR cards, pixel-perfect on e-paper."),
    ("emery_quick.pgm",  "framed_quick.png",    "Quick card: favourites on Up/Down."),
    ("basalt_card_1.pgm", "framed_tallmode.png", "Long codes auto-rotate full-screen."),
]


def build_framed_shots():
    made = 0
    for src, dst, caption in FRAMED:
        shot = frame(src)
        if not shot:
            continue
        s = 2
        shot = shot.resize((shot.width * s, shot.height * s), Image.NEAREST)
        W, H = shot.width + 160, shot.height + 220
        im = Image.new("RGB", (W, H), INK)
        d = ImageDraw.Draw(im)
        x, y = (W - shot.width) // 2, 90
        d.rounded_rectangle([x - 34, y - 34, x + shot.width + 34, y + shot.height + 34],
                            radius=46, fill=(60, 63, 72), outline=(95, 99, 110), width=4)
        d.rounded_rectangle([x - 16, y - 16, x + shot.width + 16, y + shot.height + 16],
                            radius=24, fill=(8, 9, 11))
        im.paste(shot, (x, y))
        tw = d.textlength(caption, font=font(26))
        d.text(((W - tw) / 2, H - 84), caption, font=font(26), fill=CREAM)
        tw2 = d.textlength("Cardigan", font=font(20))
        d.text(((W - tw2) / 2, 36), "Cardigan", font=font(20), fill=ORANGE)
        im.save(os.path.join(SHOTS, dst))
        made += 1
    return made


ROWS = [
    ("emery 200x228 — Pebble Time 2", "emery_", False),
    ("gabbro 260x260 round — Pebble Round 2", "gabbro_", True),
    ("chalk 180x180 round — Pebble Time Round", "chalk_", True),
    ("basalt / diorite / flint / aplite 144x168", "basalt_", False),
]


def build_platform_sheet():
    def circle_mask(im):
        m = Image.new("L", im.size, 0)
        ImageDraw.Draw(m).ellipse([0, 0, im.width - 1, im.height - 1], fill=255)
        out = Image.new("RGB", im.size, (30, 32, 40))
        out.paste(im, (0, 0), m)
        return out

    rows = []
    for label, pre, rnd in ROWS:
        ims = []
        for i in (0, 1, 3, 4, 5):
            f = frame(f"{pre}card_{i}.pgm")
            if not f:
                continue
            f = f.convert("RGB").resize((f.width * 2, f.height * 2), Image.NEAREST)
            ims.append(circle_mask(f) if rnd else f)
        if ims:
            rows.append((label, ims))
    if not rows:
        return False

    pad, hdr = 16, 26
    W = max(sum(i.width for i in ims) + pad * (len(ims) + 1) for _, ims in rows)
    H = sum(hdr + max(i.height for i in ims) + pad for _, ims in rows) + pad
    sheet = Image.new("RGB", (W, H), INK)
    d = ImageDraw.Draw(sheet)
    y = pad
    for label, ims in rows:
        d.text((pad, y + 4), label, font=font(15), fill=ORANGE)
        y += hdr
        x = pad
        for im in ims:
            sheet.paste(im, (x, y))
            d.rectangle([x - 1, y - 1, x + im.width, y + im.height], outline=(55, 60, 74))
            x += im.width + pad
        y += max(i.height for i in ims) + pad
    sheet.save(os.path.join(SHOTS, "all_platforms.png"))
    return True


def main():
    for d in (BRAND, STORE, SHOTS):
        os.makedirs(d, exist_ok=True)

    build_launcher_icon()
    mark(320, "full").save(os.path.join(BRAND, "mark.png"))
    build_store_icons()
    build_readme_banner()
    build_store_banner()

    if not frame("emery_card_0.pgm"):
        print("note: test/out/ is empty — run `make render` so marketing art\n"
              "      embeds real frames instead of leaving the watch blank.")
    n = build_framed_shots()
    sheet = build_platform_sheet()

    print("regenerated:")
    for p in ("resources/images/menu_icon.png", "docs/brand/mark.png",
              "docs/brand/banner.png", "docs/store/assets/banner_720x320.png",
              "docs/store/assets/icon_large_144.png",
              "docs/store/assets/icon_small_48.png"):
        full = os.path.join(ROOT, p)
        if os.path.exists(full):
            print(f"  {p}  {Image.open(full).size[0]}x{Image.open(full).size[1]}")
    print(f"  docs/screenshots/framed_*.png  ({n} files)")
    if sheet:
        print("  docs/screenshots/all_platforms.png")


if __name__ == "__main__":
    main()
