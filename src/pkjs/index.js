/*
 * Pebble Wallet — phone companion (PebbleKit JS)
 *
 * All heavy lifting happens here, on the phone:
 *   - Code 128 encoding -> bar/space width bytes
 *   - QR encoding       -> packed module bits (<= version 3 = 29 modules)
 * The watch receives ready-to-rasterize bytes, one card per AppMessage,
 * sent sequentially on ACK (AppMessage is strictly one-in-flight).
 */
'use strict';

var keys    = require('message_keys');
var code128 = require('./code128');
var qrcode  = require('./qrcode');
var configPage = require('./config');

var OP_SYNC_BEGIN = 1, OP_CARD = 2, OP_SYNC_END = 3;
var OP_REQUEST_SYNC = 10, OP_CARD_USED = 11;

var MAX_CARDS = 12;          // watch persist budget: 12 x 240 B records
var MAX_QR_MODULES = 29;     // QR version 3; 29*29 bits = 106 B < 180 B

// Largest barcode (in modules) each display can show at 1 px/module,
// rotated in the watch's full-height "tall mode" (h - 16 header - pads).
// Round displays (chalk, gabbro) are limited by the widest chord that
// still clears the bezel at the barcode's height, not by the diameter.
var BARCODE_BUDGET = {
  emery:   196,   // 200x228
  gabbro:  208,   // 260x260 round (Pebble Round 2)
  basalt:  136,   // 144x168
  diorite: 136,   // 144x168 B&W
  flint:   136,   // 144x168 B&W (Pebble 2 Duo)
  aplite:  136,   // 144x168 B&W (original Pebble)
  chalk:   132    // 180x180 round
};

function barcodeBudget() {
  try {
    var info = Pebble.getActiveWatchInfo();
    if (info && info.platform && BARCODE_BUDGET[info.platform]) {
      return BARCODE_BUDGET[info.platform];
    }
  } catch (e) { /* emulator / old FW */ }
  return 136;                // safe default: smallest rectangular display
}

// ---------------- card store (localStorage) ----------------

var DEFAULT_CARDS = [
  { name: 'Starbucks',      code: '6011562390123456', type: 'CODE128', color: '#00AA55', fav: true,  points: 128  },
  { name: 'Tesco Clubcard', code: '634004123456789',  type: 'CODE128', color: '#0055AA', fav: true,  points: 2450 },
  { name: 'IKEA Family',    code: '9276000123456789', type: 'CODE128', color: '#FFAA00', fav: false, points: 310  },
  { name: 'Rail Pass',      code: 'RP-88-2201-556-X', type: 'QR',      color: '#005555', fav: true,  points: 12   },
  { name: 'City Gym',       code: 'GX2024-0091',      type: 'QR',      color: '#5500AA', fav: false, points: 46   },
  { name: 'Costco',         code: '111879560028',     type: 'CODE128', color: '#AA0000', fav: false, points: 89   }
];

function loadCards() {
  try {
    var raw = localStorage.getItem('cards');
    if (raw) return JSON.parse(raw);
  } catch (e) { /* fall through */ }
  saveCards(DEFAULT_CARDS);
  return DEFAULT_CARDS.slice();
}

function saveCards(cards) {
  localStorage.setItem('cards', JSON.stringify(cards.slice(0, MAX_CARDS)));
}

// ---------------- encoding ----------------

// '#RRGGBB' -> Pebble GColor8 low 6 bits (RGB222)
function colorByte(hex) {
  var m = /^#?([0-9a-f]{6})$/i.exec(String(hex || ''));
  if (!m) return 0x03;                       // default: blue-ish
  var v = parseInt(m[1], 16);
  var r = (v >> 16) & 0xFF, g = (v >> 8) & 0xFF, b = v & 0xFF;
  return ((r >> 6) << 4) | ((g >> 6) << 2) | (b >> 6);
}

function encodeQR(text) {
  var levels = ['M', 'L'];                   // degrade gracefully to fit v3
  for (var li = 0; li < levels.length; li++) {
    try {
      var qr = qrcode(0, levels[li]);        // typeNumber 0 = auto
      qr.addData(String(text));
      qr.make();
      var n = qr.getModuleCount();
      if (n > MAX_QR_MODULES) continue;
      var bytes = new Array(Math.ceil(n * n / 8) | 0);
      for (var i = 0; i < bytes.length; i++) bytes[i] = 0;
      for (var r = 0; r < n; r++) {
        for (var c = 0; c < n; c++) {
          if (qr.isDark(r, c)) {
            var bit = r * n + c;
            bytes[bit >> 3] |= (1 << (7 - (bit & 7)));
          }
        }
      }
      return { size: n, bytes: bytes };
    } catch (e) { /* too long for this level; try next */ }
  }
  return null;
}

// Build the AppMessage dictionary for one card; null if unencodable.
function buildCardMessage(card, index) {
  var dict = {};
  dict[keys.OP]     = OP_CARD;
  dict[keys.INDEX]  = index;
  dict[keys.NAME]   = String(card.name  || 'Card').slice(0, 23);
  dict[keys.CODE]   = String(card.code  || '').slice(0, 27);
  dict[keys.COLOR]  = colorByte(card.color);
  dict[keys.FLAGS]  = card.fav ? 1 : 0;
  dict[keys.POINTS] = Math.max(0, Math.min(65535, (card.points | 0)));

  function asQR() {
    var qr = encodeQR(card.code);
    if (!qr) return null;
    dict[keys.TYPE]   = 1;
    dict[keys.QRSIZE] = qr.size;
    dict[keys.DATA]   = qr.bytes;
    return dict;
  }
  function asBarcode() {
    var widths = code128.encode(card.code);
    if (!widths) return null;
    var units = widths.reduce(function (a, b) { return a + b; }, 0);
    if (units > barcodeBudget()) return null;   // won't fit this display
    dict[keys.TYPE]   = 0;
    dict[keys.QRSIZE] = 0;
    dict[keys.DATA]   = widths;
    return dict;
  }

  // honour the requested format, fall back to the other when the
  // connected watch can't display it
  var msg = (card.type === 'QR') ? (asQR() || asBarcode())
                                 : (asBarcode() || asQR());
  if (!msg) console.log('Cardigan: cannot encode "' + card.name + '", skipped');
  return msg;
}

// ---------------- sequential sync queue ----------------

var syncing = false;

// force=false skips the sync when the card set hasn't changed since the
// last successful push: avoids rewriting the watch's persist flash and
// buzzing the wrist every time the phone app restarts.
function syncAll(force) {
  if (syncing) return;
  var cards = loadCards().slice(0, MAX_CARDS);
  var sig = JSON.stringify(cards.map(function (c) {
    return [c.name, c.code, c.type, c.color, !!c.fav, c.points | 0];
  }));
  if (!force && localStorage.getItem('synced_sig') === sig) {
    console.log('Cardigan: cards unchanged, skipping sync');
    return;
  }
  var queue = [];
  var msgs  = [];

  for (var i = 0; i < cards.length; i++) {
    var m = buildCardMessage(cards[i], msgs.length);
    if (m) msgs.push(m);
  }

  var begin = {};
  begin[keys.OP] = OP_SYNC_BEGIN;
  begin[keys.TOTAL] = msgs.length;
  queue.push(begin);
  queue = queue.concat(msgs);
  var end = {};
  end[keys.OP] = OP_SYNC_END;
  queue.push(end);

  syncing = true;
  var at = 0, retries = 0;

  function next() {
    if (at >= queue.length) {
      syncing = false;
      localStorage.setItem('synced_sig', sig);
      console.log('Cardigan: sync complete');
      return;
    }
    Pebble.sendAppMessage(queue[at],
      function () { at++; retries = 0; next(); },
      function (e) {
        if (++retries <= 3) { setTimeout(next, 250 * retries); }
        else { syncing = false; console.log('Cardigan: sync failed at ' + at); }
      });
  }
  next();
}

// ---------------- events ----------------

Pebble.addEventListener('ready', function () {
  console.log('Cardigan: PKJS ready');
  syncAll(false);                 // no-op when nothing changed
});

Pebble.addEventListener('appmessage', function (e) {
  var p = e.payload;
  if (p.OP === OP_REQUEST_SYNC) syncAll(true);   // watch asked: always push
  else if (p.OP === OP_CARD_USED && typeof p.INDEX === 'number') {
    var cards = loadCards();
    if (cards[p.INDEX]) { cards[p.INDEX].lastUsed = Date.now(); saveCards(cards); }
  }
});

Pebble.addEventListener('showConfiguration', function () {
  Pebble.openURL(configPage.buildURL(loadCards()));
});

Pebble.addEventListener('webviewclosed', function (e) {
  if (!e || !e.response) return;
  try {
    var cards = JSON.parse(decodeURIComponent(e.response));
    if (Object.prototype.toString.call(cards) === '[object Array]') {
      saveCards(cards);
      syncAll(true);              // user just edited: always push
    }
  } catch (err) {
    console.log('Cardigan: bad config payload');
  }
});
