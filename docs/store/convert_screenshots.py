#!/usr/bin/env python3
"""Convert screenshots for upload, and rendered frames for review.

    python3 docs/store/convert_screenshots.py --store    (or: make store-shots)
    python3 docs/store/convert_screenshots.py --frames   (or: make frames)

--store
    Turns emulator captures in docs/store/screenshots/ into JPEGs, which is
    what the Rebble portal accepts. Verifies each one is exactly its
    platform's native resolution and refuses to silently resize — an
    upscaled screenshot looks soft next to everyone else's.

--frames
    Turns the test suite's .pgm renders into .png. PGM is fine for
    zxing but neither GitHub nor a browser will preview it, so CI
    uploads the PNGs instead.

    These renders are NOT store material: the host stub doesn't rasterize
    system fonts, so card names and captions are missing, and the
    framebuffer is greyscale. Store screenshots must come from the
    emulator (`make screens`).
"""
import argparse
import glob
import os
import sys

try:
    from PIL import Image
except ImportError:
    sys.exit("Pillow is required:  pip install pillow")

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
STORE_DIR = os.path.join(ROOT, "docs", "store", "screenshots")
FRAME_DIR = os.path.join(ROOT, "test", "out")
FRAME_PNG = os.path.join(FRAME_DIR, "png")

# Native resolution per platform — the portal expects exactly these.
NATIVE = {
    "aplite":  (144, 168),
    "basalt":  (144, 168),
    "diorite": (144, 168),
    "flint":   (144, 168),
    "chalk":   (180, 180),
    "emery":   (200, 228),
    "gabbro":  (260, 260),
}


def convert_store():
    if not os.path.isdir(STORE_DIR):
        sys.exit(f"no {STORE_DIR} — capture screenshots first with `make screens`")

    sources = sorted(
        p for p in glob.glob(os.path.join(STORE_DIR, "*"))
        if os.path.splitext(p)[1].lower() in (".png", ".pgm", ".bmp", ".gif")
    )
    if not sources:
        sys.exit(f"no images in {STORE_DIR} — run `make screens` first")

    ok, bad = 0, 0
    for src in sources:
        base = os.path.basename(src)
        platform = base.split("_")[0].lower()
        im = Image.open(src)
        want = NATIVE.get(platform)

        if want and im.size != want:
            print(f"  !! {base}: {im.width}x{im.height}, expected "
                  f"{want[0]}x{want[1]} — not converted")
            bad += 1
            continue
        if not want:
            print(f"  ?  {base}: unknown platform prefix, converting anyway")

        dst = os.path.splitext(src)[0] + ".jpg"
        # quality 95, no subsampling: barcodes are high-contrast edges and
        # chroma subsampling smears them.
        im.convert("RGB").save(dst, "JPEG", quality=95, subsampling=0, optimize=True)
        size_kb = os.path.getsize(dst) / 1024
        print(f"  ok {os.path.basename(dst)}  {im.width}x{im.height}  {size_kb:.0f} KB")
        ok += 1

    print(f"\n{ok} converted" + (f", {bad} wrong size" if bad else ""))
    if bad:
        print("Re-capture the wrong-sized ones with `make screens` — do not "
              "resize them by hand.")
        return 1
    return 0


def convert_frames():
    pgms = sorted(glob.glob(os.path.join(FRAME_DIR, "*.pgm")))
    if not pgms:
        sys.exit("no rendered frames — run `make render` first")
    os.makedirs(FRAME_PNG, exist_ok=True)
    for src in pgms:
        dst = os.path.join(FRAME_PNG,
                           os.path.splitext(os.path.basename(src))[0] + ".png")
        Image.open(src).save(dst, "PNG", optimize=True)
    print(f"{len(pgms)} frames -> {os.path.relpath(FRAME_PNG, ROOT)}/")
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--store", action="store_true",
                    help="emulator captures -> JPEG for the Rebble portal")
    ap.add_argument("--frames", action="store_true",
                    help="test renders -> PNG so they can be previewed")
    args = ap.parse_args()

    if not (args.store or args.frames):
        ap.print_help()
        return 2
    rc = 0
    if args.frames:
        rc |= convert_frames()
    if args.store:
        rc |= convert_store()
    return rc


if __name__ == "__main__":
    sys.exit(main())
