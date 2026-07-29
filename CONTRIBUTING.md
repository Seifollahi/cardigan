# Contributing to Cardigan

Thanks for helping! A few things that make this repo easy to work on:

## You don't need the SDK for most changes

The test suite runs on any machine with `gcc`, `node` and `python3`. It
exercises the real C code and the real phone JS end to end — including
decoding rendered barcodes with an actual scanner library:

```sh
make deps    # once
make test
```

That covers unit tests, lint (colour/B&W/round C builds under `-Werror`),
rendering at all four display geometries, and scanner verification. Run
`make` on its own to see every target.

All checks must pass before a PR merges — CI enforces it and attaches the
rendered frames to the run so reviewers can see pixel changes. If you touch
the rendering or encoding paths, add a case rather than weakening a check.

## For watch-visible changes

Build with the [Pebble SDK](https://developer.rebble.io/sdk/) and eyeball at
least one square and one round platform:

```sh
make run                                  # emery, 200x228 colour
pebble install --emulator diorite         # 144x168 B&W
pebble install --emulator gabbro          # 260x260 round
```

## Read this before a big change

[`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) explains why the watch does no
encoding, how codes are fitted to round displays, and why a clipped barcode
is treated as a bug rather than a cosmetic issue.

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
