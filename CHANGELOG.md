# Changelog

## 1.0.0 — 2026-07-29

First public release.

- Scannable Code 128 and QR codes rendered 1-bit on e-paper; verified by
  decoding the rasterizer's output with zxing-cpp on every display size.
- One-press checkout: launches straight into the last-used card; Up/Down
  cycles favourites at the till.
- Full card manager in the Pebble app settings page: add, edit, reorder,
  recolour (Pebble 64-colour palette), Code 128 or QR, with validation.
- Adaptive rendering: Code 128 set C with B→C switching, 90° auto-rotation,
  full-height tall mode on small displays, and automatic QR fallback when a
  barcode exceeds the connected watch's pixel budget.
- Smart sync: one card per AppMessage with ACK/retry; skipped entirely when
  nothing changed (no persist flash wear, no spurious vibration);
  interrupted syncs self-heal on the next connect.
- Round-display support: codes are fitted against the circle (integer
  sqrt, no FPU), so nothing clips on Pebble Time Round or Round 2.
- A code that cannot fit a display is never drawn clipped — the number is
  shown large instead, since a truncated barcode scans as a wrong number.
- Platforms: all seven — aplite, basalt, chalk, diorite, emery
  (Pebble Time 2), flint (Pebble 2 Duo), gabbro (Pebble Round 2).
