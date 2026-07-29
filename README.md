<p align="center">
  <img src="docs/brand/banner.png" alt="Cardigan — the wallet you wear" width="100%">
</p>

<p align="center">
  <a href="https://github.com/Seifollahi/cardigan/actions"><img src="https://img.shields.io/github/actions/workflow/status/Seifollahi/cardigan/test.yml?label=tests&logo=github" alt="CI"></a>
  <img src="https://img.shields.io/badge/license-MIT-blue" alt="MIT">
  <img src="https://img.shields.io/badge/platforms-7%20—%20aplite%20to%20Round%202-FF4C00" alt="platforms">
  <img src="https://img.shields.io/badge/scanner_verified-zxing--cpp-success" alt="scanner verified">
</p>

**Cardigan** is a loyalty-card wallet for Pebble smartwatches. Your cards live
on your wrist as real, scannable **Code 128 barcodes and QR codes** — one
button press from the launcher to the till, no phone in hand, works fully
offline. Runs on all seven platforms: the 2013 original through the
**Pebble Time 2** (emery), **Pebble 2 Duo** (flint) and **Pebble Round 2**
(gabbro), square and round alike.

<p align="center">
  <img src="docs/screenshots/framed_barcode.png" height="290">
  <img src="docs/screenshots/framed_qr.png" height="290">
  <img src="docs/screenshots/framed_quick.png" height="290">
  <img src="docs/screenshots/framed_tallmode.png" height="290">
</p>

## Why it's fast at the till

Open Cardigan and your **last-used card is already on screen**. Up/Down hops
between favourites. That's the whole checkout interaction. Everything else —
balance page, favourite/quick/delete actions, the full card list — is one
press deeper.

Cards are managed from a settings page in the Pebble phone app: add, edit,
reorder, recolour (true Pebble 64-colour palette), choose barcode or QR.
Saving syncs to the watch in about a second over Bluetooth LE.

## Engineering under the constraints

The watch never encodes anything. The phone pre-encodes every card and the
watch just fills rectangles — integer math only, 1-bit black-on-white for
maximum scanner contrast on e-paper.

| Constraint | Decision |
|---|---|
| `persist` values max **256 B**, ~4 KB total | One card = one packed 240 B record (`_Static_assert`ed); 12 cards max. |
| AppMessage one-in-flight, small buffers | One card per message, sequential ACK queue with retry; sync skipped entirely when the card set is unchanged (no flash wear, no spurious vibe). |
| Small heap, no FPU budget | Phone ships Code 128 as run-length bar widths (1 B each) and QR as packed module bits (≤ v3 = 106 B). |
| Narrow displays vs. long codes | Set C + B→C switching halves digit-code width; barcodes auto-rotate 90°, and a full-height "tall mode" drops the chrome when needed. The phone knows the connected watch and falls back to QR when a barcode exceeds that display's budget. A code that still can't fit is never drawn clipped — the number is shown large instead, because a truncated barcode scans as the *wrong* number. |
| Round displays (chalk, gabbro) | Codes are fitted against the circle, not a bounding box: a rectangle fits iff (w/2)² + (h/2)² ≤ r², solved with an integer square root. |
| B&W watches (aplite/diorite/flint) | `PBL_IF_COLOR_ELSE` throughout — same source, seven platforms. |
| Battery | Backlight only pulses while a code is on screen; static frames, minimal redraws. |

## Install

- **Rebble store**: search "Cardigan" (Tools).
- **Sideload**: grab `cardigan.pbw` from [Releases](../../releases) and open it
  with the Pebble app, or `pebble install --phone <ip>`.

## Build

Requires the [Pebble SDK](https://developer.rebble.io/sdk/) and the ARM
toolchain (`brew install arm-none-eabi-gcc` on macOS):

```sh
pebble build
pebble install --emulator emery      # Pebble Time 2
pebble emu-app-config                # card manager in your browser
```

`build_mac.sh` does all of the above from scratch, including toolchain setup.

## Releasing

```sh
bash release.sh          # test → build → tag → publish the .pbw
bash release.sh 1.1.0    # bump the version first
```

The `.pbw` is built locally by design: the public Pebble SDK containers are
pinned to SDK 4.5, which predates the `flint` and `gabbro` platforms, so CI
can only compile a legacy subset as a smoke test. CI's real job here is the
scanner-verified test suite, which needs no SDK at all.

## Testing (no SDK required)

`test/` contains a host-side suite that runs the whole pipeline: the real
phone JS executes under node, its captured AppMessage stream replays through
the real C code, the card window renders into a framebuffer, and
**zxing-cpp decodes the output** — if a scanner can't read it, the test fails.

```sh
NODE_PATH=test/shims node test/run_pkjs.js

SRC="test/stub/pebble_stub.c test/harness.c src/c/storage.c src/c/comm.c \
     src/c/card_window.c src/c/menu_window.c"
FLAGS="-std=c11 -Wall -Wextra -Werror -Wno-unused-parameter -Itest/stub -Isrc/c"
gcc $FLAGS -DPBL_COLOR             $SRC -o test/out/harness
gcc $FLAGS -DPBL_COLOR -DPBL_ROUND $SRC -o test/out/harness_round

./test/out/harness       200 228 test/out/emery_  --logic   # Pebble Time 2
./test/out/harness       144 168 test/out/basalt_           # 144x168 family
./test/out/harness_round 180 180 test/out/chalk_            # Time Round
./test/out/harness_round 260 260 test/out/gabbro_           # Round 2

python3 test/scan_check.py test/out/emery_ test/out/basalt_ \
                           test/out/chalk_ test/out/gabbro_
```

Current status: **15/15 logic checks** (sync protocol, favourites carousel,
action menu, reboot persistence, interrupted-sync self-heal) and **24/24
scanner decodes** across all four display geometries. CI runs this on every
push. `docs/screenshots/all_platforms.png` shows the rasterizer's real
output on each.

## Architecture

```
src/c/          watchapp: storage (persist), comm (AppMessage), menu + card windows
src/pkjs/       phone: Code 128 & QR encoders, sync queue, data-URI settings page
test/           behavioral SDK stub, functional harness, scanner verification
docs/           brand assets, screenshots, store listing copy
```

Sync protocol and message keys are documented in [`src/c/wallet.h`](src/c/wallet.h).

## Contributing

Issues and PRs welcome — see [CONTRIBUTING.md](CONTRIBUTING.md). The test
suite runs on any machine with gcc, node, and python; you don't need the SDK
or a watch to hack on most of the app.

## License

MIT © Mo Seifollahi. Bundles [qrcode-generator](https://github.com/kazuhikoarase/qrcode-generator)
(MIT, © Kazuhiko Arase). "QR Code" is a registered trademark of DENSO WAVE.
Not affiliated with Core Devices or the Rebble Foundation — Pebble is their
trademark and their delightful hardware.
