/*
 * Unit tests for the phone-side encoders.
 *
 * The Code 128 test is a true round trip: the emitted bar/space widths are
 * re-assembled into the binary symbol stream, split into 11-bit symbols,
 * looked up in the spec table, checksum-verified, and decoded back to text.
 * If the encoder is wrong in a way a scanner would notice, this fails.
 *
 *   node test/test_encoders.js
 */
'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');

const code128 = require('../src/pkjs/code128');
const qrcode = require('../src/pkjs/qrcode');

let passed = 0;
function test(name, fn) {
  try {
    fn();
    passed++;
    console.log('  ok   ' + name);
  } catch (err) {
    console.log('  FAIL ' + name + '\n       ' + err.message);
    process.exitCode = 1;
  }
}

// ---------------------------------------------------------------- Code 128

// The spec table, read back out of the encoder source so the test can't
// silently drift onto its own private copy.
const BARS = fs.readFileSync(path.join(__dirname, '../src/pkjs/code128.js'), 'utf8')
  .match(/var BARS = \[([\s\S]*?)\];/)[1]
  .split(',').map(s => s.trim()).filter(Boolean);

const START_B = 104, START_C = 105, STOP = 106;
const STOP_PATTERN = '1100011101011';

function widthsToSymbols(widths) {
  // widths alternate bar,space,... starting with a bar
  let bin = '';
  widths.forEach((w, i) => { bin += (i % 2 ? '0' : '1').repeat(w); });

  assert.ok(bin.endsWith(STOP_PATTERN), 'stop pattern missing/incorrect');
  const body = bin.slice(0, bin.length - STOP_PATTERN.length);
  assert.strictEqual(body.length % 11, 0, 'symbol stream is not a multiple of 11 bits');

  const syms = [];
  for (let p = 0; p < body.length; p += 11) {
    const chunk = body.substr(p, 11);
    // the table stores patterns as numbers, so leading zeros are absent
    const idx = BARS.indexOf(String(Number(chunk)));
    assert.notStrictEqual(idx, -1, 'unknown symbol pattern ' + chunk);
    syms.push(idx);
  }
  return syms;
}

function decode(text) {
  const widths = code128.encode(text);
  assert.ok(widths, 'encoder returned null for ' + JSON.stringify(text));
  assert.ok(widths.every(w => w >= 1 && w <= 4), 'module width outside 1..4');

  const syms = widthsToSymbols(widths);
  const start = syms[0];
  const check = syms[syms.length - 1];
  const values = syms.slice(1, -1);

  let sum = start;
  values.forEach((v, i) => { sum += v * (i + 1); });
  assert.strictEqual(sum % 103, check, 'checksum mismatch');

  // replay the value stream through the set-switching state machine
  let out = '';
  let mode = start === START_C ? 'C' : 'B';
  for (const v of values) {
    if (mode === 'B' && v === 99) { mode = 'C'; continue; }        // CODE C
    if (mode === 'C' && v === 100) { mode = 'B'; continue; }       // CODE B
    out += mode === 'C' ? String(v).padStart(2, '0')
                        : String.fromCharCode(v + 32);
  }
  return { text: out, symbols: syms.length, widths };
}

const SAMPLES = [
  '6011562390123456',   // 16 digits  -> set C throughout
  '634004123456789',    // 15 digits  -> set B then switch to C
  '111879560028',       // 12 digits
  '9276000123456789',
  'RP-88-2201-556-X',   // set B, punctuation
  'GX2024-0091',
  'A1',                 // shortest mixed
  '12',                 // shortest numeric
  '20831009442'         // 11 digits, odd
];

console.log('Code 128 round trip');
SAMPLES.forEach(s => {
  test(JSON.stringify(s), () => {
    const r = decode(s);
    assert.strictEqual(r.text, s, `decoded ${JSON.stringify(r.text)}`);
  });
});

test('set C is actually used for even digit strings', () => {
  const r = decode('6011562390123456');
  // 16 digits in set C = 8 data symbols + start + check + stop = 11
  assert.ok(r.symbols <= 11, 'expected set C compression, got ' + r.symbols + ' symbols');
});

test('odd digit strings switch B->C instead of staying in B', () => {
  const wide = code128.encode('634004123456789');
  const units = wide.reduce((a, b) => a + b, 0);
  // pure set B would be 15 data symbols ~ 200 units; B->C should be far less
  assert.ok(units < 160, 'expected B->C switching, got ' + units + ' units');
});

test('rejects characters outside set B', () => {
  assert.strictEqual(code128.encode('café'), null);
  assert.strictEqual(code128.encode('中文'), null);
});

test('rejects empty and over-long input', () => {
  assert.strictEqual(code128.encode(''), null);
  assert.strictEqual(code128.encode('X'.repeat(27)), null);
});

// --------------------------------------------------------------------- QR

console.log('QR encoding');

const MAX_QR_MODULES = 29;    // must match src/pkjs/index.js

function qrModules(text, level) {
  const qr = qrcode(0, level || 'M');
  qr.addData(String(text));
  qr.make();
  return qr.getModuleCount();
}

['RP-88-2201-556-X', 'GX2024-0091', '6011562390123456'].forEach(s => {
  test(`${JSON.stringify(s)} fits v3 (<= ${MAX_QR_MODULES} modules)`, () => {
    const n = qrModules(s);
    assert.ok(n <= MAX_QR_MODULES, `${n} modules is too big for the watch buffer`);
  });
});

test('packed QR bits fit the 180-byte card payload', () => {
  const n = qrModules('RP-88-2201-556-X');
  const bytes = Math.ceil((n * n) / 8);
  assert.ok(bytes <= 180, `${bytes} bytes exceeds CARD_DATA_MAX`);
});

test('bit packing round-trips (row-major, MSB first)', () => {
  const qr = qrcode(0, 'M');
  qr.addData('GX2024-0091');
  qr.make();
  const n = qr.getModuleCount();
  const bytes = new Array(Math.ceil((n * n) / 8)).fill(0);
  for (let r = 0; r < n; r++) {
    for (let c = 0; c < n; c++) {
      if (qr.isDark(r, c)) {
        const bit = r * n + c;
        bytes[bit >> 3] |= (1 << (7 - (bit & 7)));
      }
    }
  }
  // unpack exactly the way src/c/card_window.c qr_module() does
  for (let r = 0; r < n; r++) {
    for (let c = 0; c < n; c++) {
      const bit = r * n + c;
      const got = (bytes[bit >> 3] >> (7 - (bit & 7))) & 1;
      assert.strictEqual(!!got, qr.isDark(r, c), `module ${r},${c} mismatch`);
    }
  }
});

// ---------------------------------------------------------------- payloads

console.log('Card payload limits');

const CARD_NAME_LEN = 24, CARD_CODE_LEN = 28, CARD_DATA_MAX = 180;

test('C struct limits match wallet.h', () => {
  const h = fs.readFileSync(path.join(__dirname, '../src/c/wallet.h'), 'utf8');
  const pick = k => parseInt(h.match(new RegExp('#define\\s+' + k + '\\s+(\\d+)'))[1], 10);
  assert.strictEqual(pick('CARD_NAME_LEN'), CARD_NAME_LEN);
  assert.strictEqual(pick('CARD_CODE_LEN'), CARD_CODE_LEN);
  assert.strictEqual(pick('CARD_DATA_MAX'), CARD_DATA_MAX);
});

test('every sample barcode fits CARD_DATA_MAX', () => {
  SAMPLES.forEach(s => {
    const w = code128.encode(s);
    if (w) assert.ok(w.length <= CARD_DATA_MAX, s + ' needs ' + w.length + ' bytes');
  });
});

console.log(`\n${passed} assertions passed` +
            (process.exitCode ? ' — WITH FAILURES' : ''));
