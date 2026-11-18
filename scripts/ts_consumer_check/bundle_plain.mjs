// Bundled by ts_package_check.py with esbuild under one engine-less condition,
// then run. Exits 1 unless a call through ta-lib gives the bits of ta-lib/js
// and ready() resolves to the TypeScript engine, the WebAssembly one unavailable.
import { ema, ready } from 'ta-lib';
import * as js from 'ta-lib/js';

const close = Float64Array.from({ length: 500 }, (_, i) => Math.round((100 + 10 * Math.sin(i / 7)) * 100) / 100);
const bits = (a) => Buffer.from(a.buffer, a.byteOffset, a.byteLength).toString('hex');
const got = ema(close, { timePeriod: 10 });
const want = js.ema(close, { timePeriod: 10 });
if (got.begIdx !== want.begIdx || bits(got.values) !== bits(want.values)) {
  console.error('EMA differs from ta-lib/js');
  process.exit(1);
}
const status = await ready();
if (JSON.stringify(status) !== '{"engine":"js","reason":"unavailable"}' || !Object.isFrozen(status)) {
  console.error(`ready() resolved to ${JSON.stringify(status)}`);
  process.exit(1);
}
console.log(`bundle_plain.mjs: EMA bit-identical to ta-lib/js, ready() resolved to ${JSON.stringify(status)}`);
