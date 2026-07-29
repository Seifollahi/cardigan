#!/usr/bin/env bash
# Capture native-resolution store screenshots from the Pebble emulator.
#
#   bash docs/store/capture_screenshots.sh              # all platforms
#   bash docs/store/capture_screenshots.sh emery gabbro # just these
#
# Produces docs/store/screenshots/<platform>_<n>_<name>.png at the exact
# native size the Rebble portal expects:
#   aplite/basalt/diorite/flint 144x168 · chalk 180x180
#   emery 200x228 · gabbro 260x260
#
# The emulator can't be driven headlessly, so this pauses and tells you
# which buttons to press. Arrow keys = Up/Down, Enter = Select, Esc = Back.
set -uo pipefail
cd "$(dirname "$0")/../.."

OUT="docs/store/screenshots"
mkdir -p "$OUT"

PLATFORMS=("$@")
if [ ${#PLATFORMS[@]} -eq 0 ]; then
  PLATFORMS=(aplite basalt chalk diorite emery flint gabbro)
fi

# shot label -> what to do before capturing
SHOTS=(
  "1_quickcard|The app opens on your last-used card. Nothing to press."
  "2_qr|Press DOWN until a QR card (Rail Pass / City Gym) is showing."
  "3_balance|Press ENTER (flips to the balance page)."
  "4_list|Press ESC (back to the card list)."
  "5_actions|Press ENTER on a card, then ENTER again (action menu)."
)

echo "==> Building"
pebble build || { echo "build failed"; exit 1; }

for P in "${PLATFORMS[@]}"; do
  echo
  echo "================ $P ================"
  pebble install --emulator "$P" || { echo "skip $P"; continue; }
  sleep 3

  for entry in "${SHOTS[@]}"; do
    name="${entry%%|*}"
    hint="${entry#*|}"
    echo
    echo "  [$P $name] $hint"
    read -r -p "  press Enter here when the emulator shows it… " _
    pebble screenshot --no-open "$OUT/${P}_${name}.png" \
      && echo "  saved $OUT/${P}_${name}.png"
  done

  pebble kill >/dev/null 2>&1
done

echo
echo "==> Done. Verifying sizes:"
python3 - <<'PY'
import glob, os
try:
    from PIL import Image
except ImportError:
    print("(install Pillow to verify sizes)"); raise SystemExit
EXPECT = {"aplite":(144,168),"basalt":(144,168),"diorite":(144,168),
          "flint":(144,168),"chalk":(180,180),"emery":(200,228),
          "gabbro":(260,260)}
for p in sorted(glob.glob("docs/store/screenshots/*.png")):
    plat = os.path.basename(p).split("_")[0]
    w,h = Image.open(p).size
    want = EXPECT.get(plat)
    flag = "ok " if want == (w,h) else "!! "
    print(f"  {flag}{os.path.basename(p)}  {w}x{h}" +
          ("" if want == (w,h) else f"  expected {want[0]}x{want[1]}"))
PY
