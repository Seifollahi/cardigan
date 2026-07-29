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
> MADE FOR EVERY PEBBLE — all seven platforms, from the 2013 original to the
> Pebble Time 2, Pebble 2 Duo and Pebble Round 2. Long codes rotate
> automatically to use the full screen; on round watches codes are fitted to
> the circle so nothing hides under the bezel; and if a barcode can't fit
> your display at all, Cardigan switches it to a QR code by itself.
>
> TESTED LIKE IT MATTERS — every release is verified by decoding the watch's
> own rendered screen with a real scanner library. A barcode that wouldn't
> scan fails the build.
>
> Stores up to 12 cards on the watch. Balance display, favourites, and a
> quick-card action menu included. No accounts, no network access, no
> tracking. Open source (MIT) on GitHub.

**Release notes (1.0.0):** First release — see CHANGELOG.md.

**The .pbw to upload** comes from the tagged CI build: Actions → *build* →
`cardigan-pbw` artifact, or the binary attached to the GitHub release. Don't
upload a local development build.

## Asset checklist

Ready to upload — exact sizes the portal requires, in `docs/store/assets/`:

| Asset | Spec | File |
|---|---|---|
| Appstore banner | 720×320 PNG | `assets/banner_720x320.png` |
| Large icon | 144×144 PNG | `assets/icon_large_144.png` |
| Small icon | 48×48 PNG | `assets/icon_small_48.png` |

Regenerate any of them with `python3 docs/make_assets.py` (requires Pillow).

## Screenshots

The portal takes up to 5 per platform, at native resolution:

| Platform | Watch | Size |
|---|---|---|
| aplite | Pebble / Pebble Steel | 144×168 |
| basalt | Pebble Time / Time Steel | 144×168 |
| diorite | Pebble 2 | 144×168 |
| flint | Pebble 2 Duo | 144×168 |
| chalk | Pebble Time Round | 180×180 |
| emery | Pebble Time 2 | 200×228 |
| gabbro | Pebble Round 2 | 260×260 |

**Format: the portal accepts JPEG.** Capture them with the guided helper —
it installs each emulator in turn, prompts you for the button presses
between shots, then converts everything to JPEG automatically:

```sh
make screens                                      # every platform
bash docs/store/capture_screenshots.sh emery      # just one
make store-shots                                  # re-convert only
```

Output lands in `docs/store/screenshots/` as `<platform>_<n>_<name>.jpg`,
with the PNG originals kept as masters. Five shots per platform: quick card,
QR card, balance page, card list, action menu.

The converter **refuses to resize**. If a capture isn't its platform's exact
native resolution it's reported and skipped — re-capture it rather than
scaling, because an upscaled screenshot looks soft next to everyone else's
in the store listing.

> The `.pgm` frames in `test/out/` are **not** store screenshots. They come
> from the host test suite, which doesn't rasterize system fonts, so card
> names and captions are missing and the framebuffer is greyscale. They
> exist to prove the barcodes scan, not to sell the app.

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

- [ ] `pebble build` with final version number in package.json
- [ ] Confirm the SDK accepts `flint` and `gabbro` in targetPlatforms
      (drop them if your SDK predates those platforms)
- [ ] Capture screenshots for every platform you ship
- [ ] Test sideload of the exact .pbw you upload
- [ ] Replace `[link]` placeholders in launch-posts.md once the store URL exists
