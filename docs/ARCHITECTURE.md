# Architecture

Why Cardigan is built the way it is. If you're changing the rendering or
the sync protocol, read this first.

## The one rule: the watch is dumb

The watch never encodes anything. The phone turns a card number into
ready-to-rasterize bytes, and the watch fills rectangles.

This isn't stylistic. A Pebble has no FPU worth using, a small heap, and an
app that must stay responsive on a 10-year-old MCU. A Code 128 encoder on
the watch would mean a 107-entry pattern table, checksum arithmetic and
string handling in the draw path. Instead:

| Format | What the phone sends | Bytes |
|---|---|---|
| Code 128 | run-length bar/space widths, starting with a bar, each 1–4 | 1 per run |
| QR | module bits, row-major, MSB first | ⌈n²/8⌉, ≤ 106 for v3 |

`card_window.c` walks that array and calls `graphics_fill_rect`. Integer
arithmetic only — the single square root (for round displays) is an integer
Newton iteration so we never link libm.

## Storage: one card, one persist key

Pebble's `persist_write_data` caps a value at 256 bytes and an app's total
budget is around 4 KB. So a card is a packed 240-byte struct:

```c
name[24] · code[28] · type · color · flags · qr_size · points · data_len · data[180]
```

`_Static_assert(sizeof(Card) <= PERSIST_DATA_MAX_LENGTH)` makes a violation
a compile error rather than a runtime surprise. Twelve cards is 2.9 KB,
comfortably inside budget, and the whole set lives in static RAM at runtime
so the draw path never allocates.

## Sync: one card per message, idempotent

AppMessage is strictly one-in-flight, so the phone builds a queue and sends
the next item on ACK, with backoff and three retries.

```
SYNC_BEGIN {total}          → watch stages into persist
CARD {index, …, data} × N
SYNC_END                    → commit, reload UI, vibrate
```

Cards are staged under their final keys and committed on `SYNC_END`. If the
link dies mid-sync, the watch reboots into whatever was last committed —
the phone is the source of truth and re-pushes the full set on the next
connect. The watch can also ask (`REQUEST_SYNC`) when it starts up empty.

The phone hashes the card set and **skips the sync entirely when nothing
changed**. Without that, every restart of the Pebble app would rewrite 2.9 KB
of flash and buzz the user's wrist for no reason.

## Fitting a code to the screen

The hard constraint is that a barcode's width is set by its data, not by
your layout. A 16-digit card in Code 128 set C is ~123 modules; a 144-pixel
watch cannot show that horizontally at 1 px per module with quiet zones.

The strategy, in order:

1. **Compress.** Set C packs two digits per symbol. Odd-length digit strings
   start in set B and switch to C, which is still far narrower than pure B.
2. **Rotate.** If it doesn't fit across, draw it down the screen — 168 or
   228 pixels of height is more than the width.
3. **Tall mode.** If it still doesn't fit, drop the header to a thin strip
   and the caption entirely, giving the rotated code nearly the whole screen.
4. **Fall back to QR.** The phone knows which watch is connected
   (`getActiveWatchInfo`) and its pixel budget; a barcode that would exceed
   it is sent as a QR code instead.
5. **Refuse.** If nothing fits, the watch draws the number in large type
   rather than a clipped barcode.

Step 5 matters more than it looks. A truncated Code 128 can still carry a
valid checksum for a *different, shorter number* — a scanner will happily
read the wrong card. Showing no barcode is strictly safer than showing a
plausible wrong one.

### Round displays

On chalk (180×180) and gabbro (260×260) the usable area is a circle, so a
bounding box is the wrong model. A rectangle centred on the display fits iff

```
(w/2)² + (h/2)² ≤ r²
```

`max_code_w()` solves that for the width at a given bar thickness. This is
what stops a barcode running under the bezel — and it's exactly the bug the
round render tests caught before release.

## Testing without hardware

`test/stub/` implements enough of the Pebble API to run the real app code on
a dev machine: in-memory `persist`, a grayscale framebuffer behind
`graphics_*`, a window stack that routes clicks, AppMessage delivery, and
ActionMenu capture.

That means `test/harness.c` drives the *actual* `storage.c`, `comm.c` and
`card_window.c` — not a reimplementation — and the frames it writes are what
the watch would draw. `test/scan_check.py` then decodes those frames with
zxing-cpp, so "does this scan?" is a build-time question rather than a
question you answer at a supermarket till.

The stub is only as good as its fidelity to the SDK. When the real compiler
disagrees with `test/stub/pebble.h`, the stub is wrong — fix the stub.
(It has been wrong twice: a `const` on `action_menu_open`, and a framebuffer
too small for gabbro's 260×260, which silently truncated that platform's
tests until it was caught.)
