# Changelog

All notable changes to Cardigan. This project follows
[semantic versioning](https://semver.org).

## 1.1.0 — 2026-08-04

### Features

- **Till-Mode Backlight**: Barcodes no longer dim mid-scan! The backlight stays on for 30 seconds while a card is open. Need more time? Just flick your wrist to extend it.
- **Delta Sync**: Updating your cards on the phone is now lightning fast. Cardigan only syncs what changed, saving your watch's battery and flash memory.
- **True Pebble Palette**: The settings page now features a custom 64-color picker, guaranteeing the color you choose on your phone matches what you see on your wrist.
- **Accessibility Hint**: Added a clear explanation when a barcode is too long to display on your specific watch geometry.

## 1.0.0 — 2026-07-29

First public release.

### Features

- Scannable **Code 128 and QR codes** rendered 1-bit on e-paper, verified by
  decoding the rasterizer's own output with zxing-cpp on every display size.
- **One-press checkout**: launches straight into the last-used card; Up/Down
  cycles favourites at the till.
- Full **card manager** in the Pebble app settings page — add, edit, reorder,
  recolour (Pebble 64-colour palette), Code 128 or QR — with validation that
  refuses codes the watch can't render.
- Balance page, favourites, and an action menu (favourite / set quick card /
  delete) on the watch.

### Rendering

- Code 128 **set C with B→C switching**, which roughly halves the width of
  digit-only cards.
- Automatic **90° rotation** and a full-height "tall mode" for long codes on
  small displays.
- Automatic **QR fallback** when a barcode exceeds the connected watch's
  pixel budget (`getActiveWatchInfo`).
- **Round-display support**: codes are fitted against the circle
  ((w/2)² + (h/2)² ≤ r², integer sqrt, no FPU) rather than a bounding box.
- A code that cannot fit is **never drawn clipped** — the number is shown
  large instead, because a truncated barcode can scan as a valid *wrong*
  number.

### Sync

- One card per AppMessage with ACK and retry; staged into persist and
  committed on `SYNC_END`, so an interrupted sync self-heals on reconnect.
- Sync is **skipped entirely when the card set is unchanged**, avoiding
  needless persist-flash writes and a spurious vibration on every phone-app
  restart.

### Platforms

All seven: `aplite`, `basalt`, `chalk`, `diorite`, `emery` (Pebble Time 2),
`flint` (Pebble 2 Duo), `gabbro` (Pebble Round 2).

### Project

- Host test suite that needs no SDK and no watch: encoder unit tests, real
  app code driven through a behavioural Pebble API stub, and scanner
  verification of the rendered frames. `make test` runs all of it.
- CI runs the suite on every push and attaches rendered frames as artifacts.
