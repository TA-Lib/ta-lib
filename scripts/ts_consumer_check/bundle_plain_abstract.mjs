// Bundled by ts_package_check.py with esbuild under one engine-less condition,
// then run. Exits 1 unless a call through ta-lib/abstract gives the bits of
// ta-lib/js, getRuntimeInfo answers as ta-lib/js/abstract's does, and ready()
// resolves to the TypeScript engine, the WebAssembly one unavailable.
import { Functions, getRuntimeInfo, ready } from 'ta-lib/abstract';
import { getRuntimeInfo as pureRuntimeInfo } from 'ta-lib/js/abstract';
import * as js from 'ta-lib/js';

const close = Float64Array.from({ length: 500 }, (_, i) => Math.round((100 + 10 * Math.sin(i / 7)) * 100) / 100);
const bits = (a) => Buffer.from(a.buffer, a.byteOffset, a.byteLength).toString('hex');
const out = new Float64Array(close.length);
const range = Functions.byName('EMA').newCall().setInput(0, close).setOptInput(0, 10).setOutput(0, out).call();
const want = js.ema(close, { timePeriod: 10 });
if (range.begIdx !== want.begIdx || bits(out.subarray(0, range.count)) !== bits(want.values)) {
  console.error('EMA differs from ta-lib/js');
  process.exit(1);
}
for (const key of ['wasm.engine', 'wasm.reason']) {
  if (getRuntimeInfo(key) !== pureRuntimeInfo(key)) {
    console.error(`getRuntimeInfo('${key}') is ${getRuntimeInfo(key)}, ta-lib/js/abstract answers ${pureRuntimeInfo(key)}`);
    process.exit(1);
  }
}
const status = await ready();
if (JSON.stringify(status) !== '{"engine":"js","reason":"unavailable"}' || !Object.isFrozen(status)) {
  console.error(`ready() resolved to ${JSON.stringify(status)}`);
  process.exit(1);
}
console.log(`bundle_plain_abstract.mjs: EMA bit-identical to ta-lib/js, ready() resolved to ${JSON.stringify(status)}`);
