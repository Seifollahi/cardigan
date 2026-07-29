<p align="center">
  <img src="docs/brand/banner.png" alt="Cardigan — the wallet you wear" width="100%">
</p>

<p align="center">
  <a href="https://github.com/USERNAME/cardigan/actions"><img src="https://img.shields.io/github/actions/workflow/status/USERNAME/cardigan/test.yml?label=tests&logo=github" alt="CI"></a>
  <img src="https://img.shields.io/badge/license-MIT-blue" alt="MIT">
  <img src="https://img.shields.io/badge/platforms-aplite%20·%20basalt%20·%20chalk%20·%20diorite%20·%20emery-FF4C00" alt="platforms">
  <img src="https://img.shields.io/badge/scanner_verified-zxing--cpp-success" alt="scanner verified">
</p>

**Cardigan** is a loyalty-card wallet for Pebble smartwatches. Your cards live
on your wrist as real, scannable **Code 128 barcodes and QR codes** — one
button press from the launcher to the till, no phone in hand, works fully
offline. Designed for the **Pebble Time 2** and running on every Pebble from
the 2013 original to the Pebble 2 Duo.

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
| Narrow displays vs. long codes | Set C + B→C switching halves digit-code width; barcodes auto-rotate 90°, and a full-height "tall mode" drops the chrome when needed. The phone knows the connected watch and falls back to QR when a barcode exceeds that display's budget. |
| B&W watches (aplite/diorite) | `PBL_IF_COLOR_ELSE` throughout — same source, five platforms. |
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

## Testing (no SDK required)

`test/` contains a host-side suite that runs the whole pipeline: the real
phone JS executes under node, its captured AppMessage stream replays through
the real C code, the card window renders into a framebuffer, and
**zxing-cpp decodes the output** — if a scanner can't read it, the test fails.

```sh
NODE_PATH=test/shims node test/run_pkjs.js
gcc -std=c11 -Wall -Wextra -DPBL_COLOR -Itest/stub -Isrc/c \
  test/stub/pebble_stub.c test/harness.c src/c/storage.c src/c/comm.c \
  src/c/card_window.c src/c/menu_window.c -o test/out/harness
./test/out/harness 200 228 test/out/emery_ --logic
./test/out/harness 144 168 test/out/basalt_
python3 test/scan_check.py test/out/emery_ test/out/basalt_
```

Current status: 15/15 logic checks (sync protocol, favourites carousel,
action menu, reboot persistence, interrupted-sync self-heal) and 12/12
scanner decodes across both display sizes. CI runs this on every push.

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
