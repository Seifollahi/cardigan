# Security & privacy

## What Cardigan stores, and where

Cardigan is offline by design. It makes **no network requests of any kind** —
no analytics, no telemetry, no remote sync.

| Data | Where it lives |
|---|---|
| Card names, numbers, colours, balances | `localStorage` in the Pebble phone app, and `persist` storage on the watch (max 12 cards, 240 bytes each) |
| Anything else | nowhere |

Card numbers travel only over the Bluetooth link between your phone and
your own watch, as part of the normal PebbleKit AppMessage protocol. They
are not encrypted beyond whatever the Bluetooth pairing provides — treat a
paired watch as you would a printed loyalty card in your wallet.

**Loyalty card numbers are not payment credentials**, but they can be worth
something (points balances, occasionally personal details attached to an
account). Don't sideload builds from sources you don't trust.

## Reporting a vulnerability

Open a [security advisory](https://github.com/Seifollahi/cardigan/security/advisories/new),
or email the address on the maintainer's GitHub profile. Please don't file a
public issue for anything exploitable.

Expect an acknowledgement within a week. This is a hobby project maintained
in spare time — there is no paid support or bounty, but genuine reports will
be taken seriously and credited.

## When filing any issue

Never paste a real card number into an issue, PR, or log excerpt. Describe
the code instead: format (Code 128 / QR), length, digits-only or mixed.
