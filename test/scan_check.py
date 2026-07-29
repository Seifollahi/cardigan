#!/usr/bin/env python3
"""Scanner verification: decode the C rasterizer's PGM output with
zxing-cpp (a real barcode/QR reader) and compare against the codes the
phone originally encoded. Usage: scan_check.py <prefix> [<prefix>...]"""
import glob
import os
import re
import sys

import zxingcpp
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))

expected = {}
with open(os.path.join(HERE, "out", "expected.txt")) as f:
    for line in f:
        idx, typ, code = line.rstrip("\n").split("|", 2)
        expected[int(idx)] = (typ, code)

total = fails = 0
for prefix in sys.argv[1:]:
    print(f"== {prefix} ==")
    for path in sorted(glob.glob(prefix + "card_*.pgm")):
        idx = int(re.search(r"card_(\d+)\.pgm$", path).group(1))
        typ, code = expected[idx]
        img = Image.open(path).convert("L")
        img = img.resize((img.width * 4, img.height * 4), Image.NEAREST)
        results = zxingcpp.read_barcodes(img)
        total += 1
        if not results:
            fails += 1
            print(f"  FAIL {os.path.basename(path)}: nothing decoded "
                  f"(expected {typ} {code!r})")
            continue
        r = results[0]
        got, fmt = r.text, str(r.format)
        ok_text = got == code
        ok_fmt = ("QR" in fmt) == (typ == "QR")
        status = "ok  " if (ok_text and ok_fmt) else "FAIL"
        if not (ok_text and ok_fmt):
            fails += 1
        print(f"  {status} {os.path.basename(path)}: {fmt} {got!r}"
              + ("" if ok_text else f"  != expected {code!r}"))

print(f"\n{total} scans, {fails} failures")
sys.exit(1 if fails else 0)
