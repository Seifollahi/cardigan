# Launch posts

Fill in `[store link]` once the Rebble listing is live, then post. Lead with
the framed screenshot (`docs/screenshots/framed_barcode.png`).

## r/pebble

**Title:**
> Cardigan — your loyalty cards as scannable barcodes on your Pebble (open source, all 7 platforms incl. Time 2 and Round 2)

**Body:**
> Two things annoyed me: a keyring full of loyalty fobs, and digging my phone
> out at the till. My Pebble is already on my wrist, its screen is always on,
> and e-paper renders 1-bit barcodes beautifully. So — Cardigan.
>
> **What it does**
>
> - Real, scannable **Code 128 and QR codes**, rendered pixel-perfect for
>   e-paper.
> - **One press from launcher to code**: it opens on your last-used card, and
>   Up/Down flips between favourites while you're still in the queue.
> - Cards are managed from a settings page in the Pebble app — add, edit,
>   reorder, recolour, barcode or QR. Syncs in about a second.
> - Long codes auto-rotate to use the full screen height. On the round
>   watches, codes are fitted to the *circle* rather than a bounding box, so
>   nothing disappears under the bezel. If a barcode physically can't fit
>   your display, it becomes a QR code automatically.
> - Fully offline. No accounts, no network calls, no tracking. Up to 12 cards
>   on the watch.
> - **All seven platforms**: aplite, basalt, chalk, diorite, emery
>   (Pebble Time 2), flint (Pebble 2 Duo), gabbro (Pebble Round 2).
>
> **The bit I'm actually proud of**
>
> I don't trust "it looks like a barcode" as a test. So CI renders the
> watchapp's real framebuffer at every display size, then **decodes those
> frames with zxing** — an actual scanner library — and fails the build if
> the text doesn't come back byte-for-byte. That caught a genuine bug before
> release: barcodes were being clipped by the bezel on the round watches and
> would have scanned as the *wrong number*.
>
> The whole suite runs without the Pebble SDK or a watch — the real C code
> runs against a behavioural stub of the Pebble API on your dev machine.
>
> Store: [store link] · Source: https://github.com/Seifollahi/cardigan (MIT)
>
> Feedback very welcome, especially scanner compatibility reports from real
> tills — that's the one thing I can't test from here.

## Rebble Discord (#app-showcase)

> **Cardigan 1.0** — the wallet you wear 🧶
> Loyalty cards as scannable Code 128/QR on your Pebble. Opens straight to
> your last-used card, Up/Down cycles favourites, cards managed from the
> phone app's settings page. Long codes auto-rotate, round displays fit codes
> to the circle, and anything that can't fit becomes a QR automatically.
> Fully offline, all seven platforms incl. Time 2, 2 Duo and Round 2.
> Every release is scanner-verified in CI — zxing decodes the watchapp's own
> rendered framebuffer, so a barcode that wouldn't scan fails the build.
> Store: [store link] · MIT source: https://github.com/Seifollahi/cardigan

## Hacker News (Show HN, optional)

**Title:**
> Show HN: Cardigan – loyalty cards as scannable barcodes on a Pebble watch

**First comment:**
> The interesting constraint is that the watch has ~4KB of persistent
> storage, no FPU worth using, and a 144–260px screen. So the watch does no
> encoding at all: the phone turns a card number into run-length bar widths
> or packed QR bits, and the watch just fills rectangles with integer maths.
> One card is a 240-byte struct, sized so it fits a single persist key.
>
> The testing approach might be the more reusable idea. Rendering code is
> usually tested by eye, but "does this barcode scan?" has an objective
> answer — so CI renders the real framebuffer and decodes it with zxing.
> A wrong barcode is worse than no barcode, because a truncated Code 128 can
> carry a valid checksum for a different, shorter number.
>
> https://github.com/Seifollahi/cardigan

## Tips

- Post when the store listing is live, so the top comment isn't "how do I
  install it".
- Have `framed_tallmode.png` ready when someone asks about long membership
  numbers on the small watches — it answers itself.
- `docs/screenshots/all_platforms.png` is the reply for "does it work on my
  round one".
- Expect "why not just use your phone" — the honest answer is that it's two
  seconds faster and your hands are usually full. Don't oversell it.
