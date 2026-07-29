## What this changes

<!-- One or two sentences. Link the issue if there is one. -->

## Checklist

- [ ] `make test` passes (unit tests, lint, render, scanner verification)
- [ ] If rendering changed: I looked at the rendered frames — CI attaches
      them as the `rendered-frames` artifact
- [ ] If the AppMessage protocol changed: `STORE_VERSION` bumped and
      `wallet.h` updated
- [ ] If a platform was added: it has a `BARCODE_BUDGET` entry in
      `src/pkjs/index.js`
- [ ] No real loyalty card numbers anywhere in the diff

## Tested on

<!-- e.g. emery emulator + Pebble 2 Duo on Android, or "host suite only" -->
