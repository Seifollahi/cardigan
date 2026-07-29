#!/usr/bin/env bash
# Cut a Cardigan release: test, build, tag, publish the .pbw to GitHub.
#
#   bash release.sh            # uses the version in package.json
#   bash release.sh 1.1.0      # sets that version first
#
# The .pbw is built locally on purpose: the public Pebble SDK Docker images
# are stuck on SDK 4.5, which predates the flint and gabbro platforms.
set -euo pipefail
cd "$(dirname "$0")"

VERSION="${1:-$(python3 -c 'import json;print(json.load(open("package.json"))["version"])')}"

if [ -n "${1:-}" ]; then
  python3 - "$VERSION" <<'PY'
import json, sys
p = json.load(open("package.json"))
p["version"] = sys.argv[1]
json.dump(p, open("package.json", "w"), indent=2)
open("package.json", "a").write("\n")
print("package.json version ->", sys.argv[1])
PY
fi

echo "==> Releasing Cardigan v$VERSION"

# ---- 1. the same suite CI runs -------------------------------------
echo "==> Host test suite"
NODE_PATH=test/shims node test/run_pkjs.js >/dev/null
mkdir -p test/out
SRC="test/stub/pebble_stub.c test/harness.c src/c/storage.c src/c/comm.c
     src/c/card_window.c src/c/menu_window.c"
FLAGS="-std=c11 -Wall -Wextra -Werror -Wno-unused-parameter -Itest/stub -Isrc/c"
# shellcheck disable=SC2086
gcc $FLAGS -DPBL_COLOR             $SRC -o test/out/harness
# shellcheck disable=SC2086
gcc $FLAGS -DPBL_COLOR -DPBL_ROUND $SRC -o test/out/harness_round
./test/out/harness       200 228 test/out/emery_  --logic | tail -1
./test/out/harness       144 168 test/out/basalt_ | tail -1
./test/out/harness_round 180 180 test/out/chalk_  | tail -1
./test/out/harness_round 260 260 test/out/gabbro_ | tail -1
if ! python3 -c "import zxingcpp, PIL" 2>/dev/null; then
  echo "==> Installing scanner deps (zxing-cpp, pillow)…"
  pip3 install --quiet zxing-cpp pillow 2>/dev/null \
    || pip3 install --quiet --break-system-packages zxing-cpp pillow 2>/dev/null \
    || true
fi
if python3 -c "import zxingcpp, PIL" 2>/dev/null; then
  python3 test/scan_check.py test/out/emery_ test/out/basalt_ \
                             test/out/chalk_ test/out/gabbro_ | tail -1
else
  echo "!! scanner deps unavailable — skipping scan verification."
  echo "   install with: pip3 install zxing-cpp pillow"
  read -r -p "   continue the release without it? [y/N] " ans
  [ "$ans" = "y" ] || [ "$ans" = "Y" ] || exit 1
fi

# ---- 2. build the watchapp -----------------------------------------
echo "==> pebble build"
pebble clean >/dev/null 2>&1 || true
pebble build

PBW=$(ls build/*.pbw 2>/dev/null | head -1)
[ -n "$PBW" ] || { echo "no .pbw produced"; exit 1; }
echo "==> Built $PBW ($(du -h "$PBW" | cut -f1))"
echo "    platforms inside:"
unzip -l "$PBW" | awk '/\/app.bin/ {split($4,a,"/"); print "      " a[1]}'

# ---- 3. tag & publish ----------------------------------------------
if ! git diff --quiet || ! git diff --cached --quiet; then
  git add -A
  git commit -m "Release v$VERSION"
fi
git push

if git rev-parse "v$VERSION" >/dev/null 2>&1; then
  echo "==> tag v$VERSION already exists locally"
else
  git tag "v$VERSION"
fi
git push origin "v$VERSION" || true

RELEASE_PBW="build/cardigan-$VERSION.pbw"
cp "$PBW" "$RELEASE_PBW"

if command -v gh >/dev/null 2>&1; then
  if gh release view "v$VERSION" >/dev/null 2>&1; then
    echo "==> updating existing release v$VERSION"
    gh release upload "v$VERSION" "$RELEASE_PBW" --clobber
  else
    gh release create "v$VERSION" "$RELEASE_PBW" \
      --title "Cardigan $VERSION" --notes-file CHANGELOG.md
  fi
  echo "==> Published: $(gh release view "v$VERSION" --json url -q .url)"
else
  echo "==> gh CLI not found — upload $RELEASE_PBW manually at:"
  echo "    https://github.com/Seifollahi/cardigan/releases/new?tag=v$VERSION"
fi
