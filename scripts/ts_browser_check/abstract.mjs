// The page ts_browser_check.py serves under /abstract/: ta-lib/abstract is the
// only entry it imports. Each call made before ready() falls through to the
// TypeScript of ta-lib/js, so it is the reference the same call after ready()
// must match. It posts one verdict to /verdict/abstract.
import { compiles, thrown } from './guard.mjs';
import { Functions, InputType, OutputType, ready } from 'ta-lib/abstract';
import * as hooks from './node_modules/ta-lib/dist/runtime/wasm.js';

const assertions = [];
const check = (name, ok, detail) => assertions.push({ name, ok: Boolean(ok), detail });

const n = 500;
const price = (i, k) => Math.round((100 + 10 * Math.sin(i / 7 + k) + i / 50 + 3 * Math.cos(i / 3)) * 100) / 100;
const close = Float64Array.from({ length: n }, (_, i) => price(i, 0));
const open = Float64Array.from({ length: n }, (_, i) => price(i, 0.9));
const prices = {
  open,
  high: Float64Array.from({ length: n }, (_, i) => Math.max(open[i], close[i]) + (i % 5) * 0.37),
  low: Float64Array.from({ length: n }, (_, i) => Math.min(open[i], close[i]) - (i % 3) * 0.41),
  close,
};
const names = ['EMA', 'BBANDS', 'CDLENGULFING'];

function call(name) {
  const info = Functions.byName(name);
  const h = info.newCall();
  info.inputs.forEach((input, i) => (input.type === InputType.PRICE ? h.setPriceInput(i, prices) : h.setInput(i, close)));
  const outs = info.outputs.map((o) => (o.type === OutputType.INTEGER ? new Int32Array(n) : new Float64Array(n)));
  outs.forEach((out, k) => h.setOutput(k, out));
  const range = h.call(0, n - 1);
  return { range, outs };
}

const hex = (a) => Array.from(new Uint8Array(a.buffer, a.byteOffset, a.byteLength), (b) => b.toString(16).padStart(2, '0')).join('');
const same = (a, b) => a.range.begIdx === b.range.begIdx && a.range.count === b.range.count && a.outs.every((x, k) => hex(x) === hex(b.outs[k]));

async function run() {
  hooks.setEngineMode('force');
  const reference = new Map(names.map((name) => [name, call(name)]));
  check('abstract-before-ready-stayed-in-typescript',
    hooks.moduleCalls() === 0 && hooks.fallThroughCount() === names.length && hooks.lastFallThrough() === 'not-loaded',
    `module calls ${hooks.moduleCalls()}, fall-throughs ${hooks.fallThroughCount()} (${hooks.lastFallThrough()}) for ${names.length} calls`);
  hooks.resetFallThroughs();
  await fetch('/mark/abstract/before-ready', { method: 'POST' });

  const status = await ready();
  check('abstract-ready-resolves-wasm', status.engine === 'wasm' && status.reason === null, JSON.stringify(status));
  check('abstract-compiled-asynchronously', compiles.sync === 0 && compiles.async >= 1,
    `${compiles.sync} synchronous WebAssembly.Module construction(s), ${compiles.async} asynchronous compile(s)`);

  hooks.setEngineMode('strict');
  for (const name of names) {
    const entered = hooks.moduleCalls();
    let got;
    try {
      got = call(name);
    } catch (e) {
      check(`abstract-strict-bits-${name}`, false, `threw ${e}`);
      continue;
    }
    const ran = hooks.moduleCalls() - entered;
    const ok = ran === 1 && same(got, reference.get(name));
    check(`abstract-strict-bits-${name}`, ok && hooks.fallThroughCount() === 0,
      `the module counted ${ran} batch call(s), ${hooks.fallThroughCount()} fall-through(s), bits ${same(got, reference.get(name)) ? 'equal to' : 'differ from'} the TypeScript answer`);
  }
}

let failure = null;
try {
  await run();
} catch (e) {
  failure = String(e && e.stack ? e.stack : e);
}
await new Promise((resolve) => setTimeout(resolve, 0));
check('abstract-nothing-threw', failure === null && thrown.length === 0, failure ?? (thrown.join('; ') || 'no error or unhandled rejection'));
await fetch('/verdict/abstract', {
  method: 'POST',
  headers: { 'content-type': 'application/json' },
  body: JSON.stringify({ userAgent: navigator.userAgent, assertions }),
});
