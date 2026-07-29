/*
 * Code 128 encoder (phone side).
 * Output: array of run-length bar/space widths (starts with a bar),
 * ready for the watch's integer rasterizer.
 *
 * Pattern table (11-bit symbols + 13-bit stop) from the Code 128
 * specification, as published in JsBarcode (MIT, github.com/lindell/JsBarcode).
 */
'use strict';

var BARS = [
  11011001100, 11001101100, 11001100110, 10010011000, 10010001100,
  10001001100, 10011001000, 10011000100, 10001100100, 11001001000,
  11001000100, 11000100100, 10110011100, 10011011100, 10011001110,
  10111001100, 10011101100, 10011100110, 11001110010, 11001011100,
  11001001110, 11011100100, 11001110100, 11101101110, 11101001100,
  11100101100, 11100100110, 11101100100, 11100110100, 11100110010,
  11011011000, 11011000110, 11000110110, 10100011000, 10001011000,
  10001000110, 10110001000, 10001101000, 10001100010, 11010001000,
  11000101000, 11000100010, 10110111000, 10110001110, 10001101110,
  10111011000, 10111000110, 10001110110, 11101110110, 11010001110,
  11000101110, 11011101000, 11011100010, 11011101110, 11101011000,
  11101000110, 11100010110, 11101101000, 11101100010, 11100011010,
  11101111010, 11001000010, 11110001010, 10100110000, 10100001100,
  10010110000, 10010000110, 10000101100, 10000100110, 10110010000,
  10110000100, 10011010000, 10011000010, 10000110100, 10000110010,
  11000010010, 11001010000, 11110111010, 11000010100, 10001111010,
  10100111100, 10010111100, 10010011110, 10111100100, 10011110100,
  10011110010, 11110100100, 11110010100, 11110010010, 11011011110,
  11011110110, 11110110110, 10101111000, 10100011110, 10001011110,
  10111101000, 10111100010, 11110101000, 11110100010, 10111011110,
  10111101110, 11101011110, 11110101110, 11010000100, 11010010000,
  11010011100, 1100011101011
];

var START_B = 104, START_C = 105, STOP = 106, CODE_C = 99;

/**
 * Encode text as Code 128. Digit strings use set C (two digits per
 * symbol); odd-length digit strings emit the first digit in set B
 * then switch to C (CODE_C), which roughly halves symbol count vs.
 * pure set B. Returns array of widths, or null if the text contains
 * characters outside set B.
 */
function encode(text) {
  text = String(text);
  if (!text.length || text.length > 26) return null;

  var values = [];
  var start;
  var allDigits = /^[0-9]+$/.test(text);

  if (allDigits && text.length % 2 === 0) {
    start = START_C;
    for (var i = 0; i < text.length; i += 2) {
      values.push(parseInt(text.substr(i, 2), 10));
    }
  } else if (allDigits && text.length >= 5) {
    // odd length: first digit in set B, then switch to set C
    start = START_B;
    values.push(text.charCodeAt(0) - 32);
    values.push(CODE_C);
    for (var p = 1; p < text.length; p += 2) {
      values.push(parseInt(text.substr(p, 2), 10));
    }
  } else {
    start = START_B;
    for (var j = 0; j < text.length; j++) {
      var cc = text.charCodeAt(j);
      if (cc < 32 || cc > 127) return null;   // outside set B
      values.push(cc - 32);
    }
  }

  var checksum = start;
  for (var k = 0; k < values.length; k++) checksum += values[k] * (k + 1);
  checksum %= 103;

  var symbols = [start].concat(values, [checksum, STOP]);

  // full binary string -> run-length widths
  var bin = '';
  for (var s = 0; s < symbols.length; s++) bin += String(BARS[symbols[s]]);

  var widths = [];
  var run = 1;
  for (var b = 1; b <= bin.length; b++) {
    if (b < bin.length && bin[b] === bin[b - 1]) { run++; continue; }
    widths.push(run);
    run = 1;
  }
  // widths[0] is a bar ('1') by construction; max run length is 4
  return widths.length <= 180 ? widths : null;
}

module.exports = { encode: encode };
