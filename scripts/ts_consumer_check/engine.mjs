// Usage: node engine.mjs
// Exits 1 unless, in strict mode, a call through the default entry runs in WASM
// and gives the bits ta-lib/js gives. Strict mode throws instead of falling
// through, so a success here is a WASM answer.
import { ema, macd, ready } from 'ta-lib';
import * as js from 'ta-lib/js';

const hooks = await import(new URL('../runtime/wasm.js', import.meta.resolve('ta-lib')).href);
const close = Float64Array.from({ length: 500 }, (_, i) => Math.round((100 + 10 * Math.sin(i / 7) + i / 50) * 100) / 100);
const bits = (a) => Buffer.from(a.buffer, a.byteOffset, a.byteLength).toString('hex');

hooks.setEngineMode('strict');
const got = [ema(close, { timePeriod: 20 }), macd(close)];
const want = [js.ema(close, { timePeriod: 20 }), js.macd(close)];
for (let k = 0; k < got.length; k++) {
  for (const key of Object.keys(want[k])) {
    const same = key === 'begIdx' ? got[k][key] === want[k][key] : bits(got[k][key]) === bits(want[k][key]);
    if (!same) {
      console.error(`call ${k}: ${key} differs from ta-lib/js`);
      process.exit(1);
    }
  }
}
const status = await ready();
if (status.engine !== 'wasm' || hooks.fallThroughCount() !== 0) {
  console.error(`engine ${JSON.stringify(status)}, ${hooks.fallThroughCount()} fall-through(s)`);
  process.exit(1);
}
console.log(`engine.mjs: EMA and MACD ran in WASM in strict mode, bit-identical to ta-lib/js (${JSON.stringify(status)})`);
