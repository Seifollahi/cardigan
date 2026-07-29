# Contributing to Cardigan

Thanks for helping! A few things that make this repo easy to work on:

## You don't need the SDK for most changes

The functional test suite runs on any machine with `gcc`, `node`, and
`python3` — it exercises the real C code and the real phone JS end to end,
including decoding rendered barcodes with an actual scanner library:

```sh
NODE_PATH=test/shims node test/run_pkjs.js
gcc -std=c11 -Wall -Wextra -DPBL_COLOR -Itest/stub -Isrc/c \
  test/stub/pebble_stub.c test/harness.c src/c/storage.c src/c/comm.c \
  src/c/card_window.c src/c/menu_window.c -o test/out/harness
./test/out/harness 200 228 test/out/emery_ --logic
./test/out/harness 144 168 test/out/basalt_
pip install zxing-cpp pillow
python3 test/scan_check.py test/out/emery_ test/out/basalt_
```

All checks must pass before a PR merges (CI enforces this). If you touch the
rendering or encoding paths, add a case rather than weakening a check.

## For watch-visible changes

Build with the [Pebble SDK](https://developer.rebble.io/sdk/) and eyeball at
least `emery` (colour, 200×228) and `diorite` (B&W, 144×168):

```sh
pebble build && pebble install --emulator emery
```

## Ground rules

- The watch stays dumb: all encoding happens on the phone. Don't add
  encoding tables or floating point to `src/c/`.
- One card must keep fitting in a single 256 B persist record — the
  `_Static_assert` in `wallet.h` is not negotiable.
- If you change the AppMessage protocol, bump `STORE_VERSION`, update the
  stub in `test/`, and document the change in `wallet.h`.
- Keep the API stub (`test/stub/pebble.h`) faithful to the real SDK — when
  the real compiler disagrees with the stub, the stub is wrong.

## Reporting bugs

Include your watch model, firmware, phone OS, and — if it's a scanning
problem — the code format and length (never post your real card numbers).
