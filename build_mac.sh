#!/usr/bin/env bash
# Pebble Wallet — build & run in the Pebble Time 2 emulator on macOS.
# Installs the Rebble pebble-tool (via uv) and the SDK on first run.
set -euo pipefail

cd "$(dirname "$0")"
echo "==> Project: $(pwd)"

# --- 1. uv (Python tool manager used by the modern pebble-tool) -------
if ! command -v uv >/dev/null 2>&1; then
  if command -v brew >/dev/null 2>&1; then
    echo "==> Installing uv via Homebrew…"
    brew install uv
  else
    echo "==> Installing uv…"
    curl -LsSf https://astral.sh/uv/install.sh | sh
    export PATH="$HOME/.local/bin:$PATH"
  fi
fi

# --- 1b. ARM cross-compiler (required by the SDK, installed separately)
if ! command -v arm-none-eabi-gcc >/dev/null 2>&1; then
  echo "==> Installing ARM embedded toolchain…"
  if command -v brew >/dev/null 2>&1; then
    brew install arm-none-eabi-gcc
  else
    echo "ERROR: arm-none-eabi-gcc not found and Homebrew unavailable."
    echo "Install it from https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads"
    exit 1
  fi
fi
arm-none-eabi-gcc --version | head -1

# --- 2. pebble-tool ---------------------------------------------------
if ! command -v pebble >/dev/null 2>&1; then
  echo "==> Installing pebble-tool…"
  uv tool install pebble-tool
  export PATH="$HOME/.local/bin:$PATH"
fi
pebble --version

# --- 3. SDK core (first run only; ~200 MB, a few minutes) -------------
if ! pebble --version 2>/dev/null | grep -q "active SDK"; then
  echo "==> Installing latest SDK (first run only)…"
  pebble sdk install latest || true
fi

# --- 4. Build ----------------------------------------------------------
echo "==> Building (emery / basalt / chalk / diorite)…"
pebble build

# --- 5. Run on the Pebble Time 2 emulator ------------------------------
echo "==> Launching emery (Pebble Time 2) emulator…"
pebble install --emulator emery --logs

# Useful afterwards:
#   pebble emu-app-config             # open the card-manager settings page
#   pebble install --emulator diorite # B&W Pebble 2 Duo
#   pebble install --phone <IP>       # sideload to Android (Developer Connection)
