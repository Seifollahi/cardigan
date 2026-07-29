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
make test || { echo "tests failed — refusing to release"; exit 1; }

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
