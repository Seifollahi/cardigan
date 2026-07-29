# Launch posts

## r/pebble

**Title:**
> I built Cardigan — your loyalty cards as scannable barcodes on your Pebble (open source, works on everything from the 2013 OG to the Time 2)

**Body:**
> Two things annoyed me: a keyring full of loyalty fobs, and digging out my
> phone at the till. My Pebble is already on my wrist, its screen is always
> on, and e-paper renders 1-bit barcodes beautifully. So — Cardigan.
>
> What it does:
>
> - Real, scannable Code 128 + QR codes, rendered pixel-perfect for e-paper.
>   I verify every build by decoding the watch's actual framebuffer output
>   with zxing — if a scanner can't read it, CI fails.
> - One press from launcher to code: it opens on your last-used card, and
>   Up/Down flips between favourites while you're in the queue.
> - Cards are managed from a settings page in the Pebble app — add, edit,
>   reorder, recolour, barcode or QR. Syncs in about a second.
> - Long codes auto-rotate to use the full screen height; if your watch's
>   display physically can't fit a barcode, it becomes a QR automatically.
> - Fully offline, up to 12 cards on-watch, all five platforms
>   (aplite/basalt/chalk/diorite/emery).
>
> It's MIT-licensed and the repo has a fun testing story: the whole app runs
> on your dev machine without the SDK — real phone JS under node, real C
> under a behavioral stub, output decoded by an actual scanner library.
>
> Store: [Rebble store link] · Source: https://github.com/USERNAME/cardigan
>
> Feedback very welcome, especially scanner compatibility reports from
> different tills.

## Rebble Discord (#app-showcase)

> **Cardigan 1.0** — the wallet you wear 🧶
> Loyalty cards as scannable Code 128/QR on your Pebble. Opens straight to
> your last-used card, Up/Down cycles favourites, cards managed from the
> phone app settings page. Auto-rotates long codes, auto-falls-back to QR on
> small screens, fully offline. All five platforms incl. emery.
> Every release is scanner-verified in CI (zxing decodes the actual
> framebuffer output).
> Store: [link] · MIT source: https://github.com/USERNAME/cardigan

## Tips

- Post the r/pebble thread with the framed screenshot
  (`docs/screenshots/framed_barcode.png`) as the image.
- Best timing: when the store listing is live, so the top comment isn't
  "how do I install it".
- Have `framed_tallmode.png` ready to reply with when someone asks about
  long membership numbers on the small watches — it answers itself.
