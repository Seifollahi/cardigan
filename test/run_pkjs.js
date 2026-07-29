/*
 * Runs the REAL phone-side pipeline (index.js + code128.js + qrcode.js)
 * under node with a Pebble mock, captures the AppMessage stream, and
 * writes it in a simple line format for the C harness:
 *
 *   out/cards.txt    BEGIN <total> / CARD <idx>|name|code|type|color|flags|qrsize|points|hexdata / END
 *   out/expected.txt <idx>|<type>|<code text>     (scanner ground truth)
 *
 * Usage: NODE_PATH=test/shims node test/run_pkjs.js
 */
'use strict';
var fs = require('fs'), path = require('path');
var K = require('message_keys');

var msgs = [];
global.localStorage = {
  _d: {},
  getItem: function (k) { return this._d.hasOwnProperty(k) ? this._d[k] : null; },
  setItem: function (k, v) { this._d[k] = String(v); },
  removeItem: function (k) { delete this._d[k]; }
};
global.Pebble = {
  _l: {},
  addEventListener: function (ev, cb) { this._l[ev] = cb; },
  sendAppMessage: function (dict, ok, err) { msgs.push(dict); setImmediate(ok); },
  openURL: function () {}
};
global.console.log = console.log.bind(console);

require('../src/pkjs/index.js');
Pebble._l.ready();                       // triggers syncAll()

setTimeout(function () {
  var outDir = path.join(__dirname, 'out');
  if (!fs.existsSync(outDir)) fs.mkdirSync(outDir, { recursive: true });

  var lines = [], expected = [];
  msgs.forEach(function (m) {
    var op = m[K.OP];
    if (op === 1) lines.push('BEGIN ' + m[K.TOTAL]);
    else if (op === 3) lines.push('END');
    else if (op === 2) {
      var hex = (m[K.DATA] || []).map(function (b) {
        return ('0' + (b & 0xFF).toString(16)).slice(-2);
      }).join('');
      lines.push(['CARD ' + m[K.INDEX], m[K.NAME], m[K.CODE], m[K.TYPE],
                  m[K.COLOR], m[K.FLAGS], m[K.QRSIZE], m[K.POINTS], hex].join('|'));
      expected.push([m[K.INDEX], m[K.TYPE] ? 'QR' : 'CODE128', m[K.CODE]].join('|'));
    }
  });

  fs.writeFileSync(path.join(outDir, 'cards.txt'), lines.join('\n') + '\n');
  fs.writeFileSync(path.join(outDir, 'expected.txt'), expected.join('\n') + '\n');
  console.log('captured ' + msgs.length + ' messages (' + expected.length + ' cards)');

  // report encoded sizes vs. display budgets
  msgs.filter(function (m) { return m[K.OP] === 2; }).forEach(function (m) {
    if (m[K.TYPE] === 0) {
      var units = m[K.DATA].reduce(function (a, b) { return a + b; }, 0);
      console.log('  #' + m[K.INDEX] + ' ' + m[K.NAME] + ': barcode ' + units +
                  ' units (emery w:192 h:216 | basalt w:132 h:156)');
    } else {
      console.log('  #' + m[K.INDEX] + ' ' + m[K.NAME] + ': QR ' +
                  m[K.QRSIZE] + 'x' + m[K.QRSIZE] + ' modules');
    }
  });
}, 50);
