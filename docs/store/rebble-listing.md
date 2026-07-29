# Rebble store listing — copy & assets

**Portal:** https://dev-portal.rebble.io → Create app → Watchapp

## Fields

- **Title:** Cardigan
- **Category:** Tools & Utilities
- **Website:** https://github.com/Seifollahi/cardigan
- **Source:** https://github.com/Seifollahi/cardigan

**Short description (one-liner):**

> The wallet you wear — loyalty cards as scannable barcodes and QR codes, one button press from your wrist.

**Full description:**

> Leave the keyring fobs at home. Cardigan keeps your loyalty cards on your
> Pebble as real, scannable Code 128 barcodes and QR codes — rendered
> pixel-perfect for e-paper and readable by laser and camera scanners.
>
> ONE PRESS AT THE TILL — Cardigan opens straight to your last-used card.
> Up/Down hops between favourites. No phone, no signal, fully offline.
>
> MANAGE FROM YOUR PHONE — add, edit, reorder and colour-code cards in the
> Pebble app's settings page. Changes sync to the watch in about a second.
>
> MADE FOR EVERY PEBBLE — from the 2013 original to the Pebble Time 2.
> Long codes rotate automatically to use the full screen; if a barcode can't
> fit your watch's display, Cardigan switches it to a QR code by itself.
>
> Stores up to 12 cards on the watch. Balance display, favourites, and a
> quick-card action menu included. Open source (MIT) on GitHub.

**Release notes (1.0.0):** First release — see CHANGELOG.md.

## Asset checklist

Ready to upload — exact sizes the portal requires, in `docs/store/assets/`:

| Asset | Spec | File |
|---|---|---|
| Appstore banner | 720×320 PNG | `assets/banner_720x320.png` |
| Large icon | 144×144 PNG | `assets/icon_large_144.png` |
| Small icon | 48×48 PNG | `assets/icon_small_48.png` |
| Screenshots | native per platform: 144×168 (aplite/basalt/diorite), 180×180 (chalk), 200×228 (emery) | `pebble screenshot` against each emulator |

Regenerate any of them with `python3 docs/store/make_assets.py` (requires Pillow).

Screenshot capture, per platform:

```sh
pebble install --emulator emery && pebble screenshot emery_1.png
pebble install --emulator basalt && pebble screenshot basalt_1.png
pebble install --emulator diorite && pebble screenshot diorite_1.png
pebble install --emulator chalk && pebble screenshot chalk_1.png
pebble install --emulator aplite && pebble screenshot aplite_1.png
```

Shoot three per platform: quick card (barcode), card list, balance page.
The store wants raw native-resolution shots — the framed marketing images in
`docs/screenshots/` are for GitHub/social, not the store uploader.

## Before submitting

- [ ] Replace `[link]` placeholders with the live store URL once published
- [ ] `pebble build` with final version number in package.json
- [ ] Verify chalk in the emulator (round-display clipping) or drop chalk
- [ ] Test sideload of the exact .pbw you upload
