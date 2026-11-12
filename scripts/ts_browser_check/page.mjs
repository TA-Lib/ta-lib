// The page ts_browser_check.py serves under /plain/ and, with a CSP that
// forbids WebAssembly, under /csp/. It posts one verdict to /verdict/<mode>.
import { compiles, thrown } from './guard.mjs';
import { cdlengulfing, ema, macd, ready, sma, t3 } from 'ta-lib';
import * as js from 'ta-lib/js';
import * as hooks from './node_modules/ta-lib/dist/runtime/wasm.js';

const mode = location.pathname.split('/')[1];
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
const calls = [
  ['ema', () => ema(close, { timePeriod: 20 }), () => js.ema(close, { timePeriod: 20 })],
  ['t3', () => t3(close, { timePeriod: 5, vFactor: 0.7 }), () => js.t3(close, { timePeriod: 5, vFactor: 0.7 })],
  ['macd', () => macd(close), () => js.macd(close)],
  ['sma', () => sma(close, { timePeriod: 30 }), () => js.sma(close, { timePeriod: 30 })],
  ['cdlengulfing', () => cdlengulfing(prices), () => js.cdlengulfing(prices)],
];

const hex = (a) => Array.from(new Uint8Array(a.buffer, a.byteOffset, a.byteLength), (b) => b.toString(16).padStart(2, '0')).join('');
function differs(got, want) {
  for (const key of Object.keys(want)) {
    const same = ArrayBuffer.isView(want[key]) ? ArrayBuffer.isView(got[key]) && hex(got[key]) === hex(want[key]) : got[key] === want[key];
    if (!same) return key;
  }
  return null;
}

async function mark(name) {
  await fetch(`/mark/${mode}/${name}`, { method: 'POST' });
}

async function plain() {
  hooks.setEngineMode('force');
  const before = hooks.moduleCalls();
  const early = sma(close, { timePeriod: 30 });
  const diff = differs(early, js.sma(close, { timePeriod: 30 }));
  check('before-ready-call-stayed-in-typescript',
    hooks.moduleCalls() === before && before === 0 && hooks.fallThroughCount() === 1 && hooks.lastFallThrough() === 'not-loaded' && diff === null,
    `module calls ${before} -> ${hooks.moduleCalls()}, fall-throughs ${hooks.fallThroughCount()} (${hooks.lastFallThrough()}), bits ${diff === null ? 'equal' : `differ at ${diff}`}`);
  hooks.resetFallThroughs();
  await mark('before-ready');

  const status = await ready();
  check('ready-resolves-wasm', status.engine === 'wasm' && status.reason === null, JSON.stringify(status));
  check('compiled-asynchronously', compiles.sync === 0 && compiles.async >= 1,
    `${compiles.sync} synchronous WebAssembly.Module construction(s), ${compiles.async} asynchronous compile(s)`);

  hooks.setEngineMode('strict');
  let ran = 0;
  for (const [name, wasmCall, jsCall] of calls) {
    const entered = hooks.moduleCalls();
    let got;
    try {
      got = wasmCall();
    } catch (e) {
      check(`strict-bits-${name}`, false, `threw ${e}`);
      continue;
    }
    ran += hooks.moduleCalls() - entered;
    const where = differs(got, jsCall());
    check(`strict-bits-${name}`, where === null, where === null ? 'bit-identical to ta-lib/js' : `${where} differs from ta-lib/js`);
  }
  check('module-counter-advanced', ran === calls.length && hooks.fallThroughCount() === 0,
    `the module counted ${ran} batch call(s) for ${calls.length}, ${hooks.fallThroughCount()} fall-through(s)`);
}

async function csp() {
  const status = await ready();
  check('csp-ready-resolves-js-with-reason', status.engine === 'js' && status.reason === 'compile', JSON.stringify(status));
  hooks.setEngineMode('force');
  const bad = [];
  for (const [name, wasmCall, jsCall] of calls) {
    try {
      const where = differs(wasmCall(), jsCall());
      if (where !== null) bad.push(`${name}: ${where} differs`);
    } catch (e) {
      bad.push(`${name}: threw ${e}`);
    }
  }
  check('csp-calls-answer-from-typescript', bad.length === 0 && hooks.moduleCalls() === 0 && hooks.fallThroughCount() === calls.length,
    bad.length ? bad.join('; ') : `${calls.length} calls bit-identical to ta-lib/js, ${hooks.moduleCalls()} module call(s), ${hooks.fallThroughCount()} fall-through(s) (${hooks.lastFallThrough()})`);
  check('csp-compiled-asynchronously', compiles.sync === 0, `${compiles.sync} synchronous WebAssembly.Module construction(s)`);
}

let failure = null;
try {
  await (mode === 'csp' ? csp() : plain());
} catch (e) {
  failure = String(e && e.stack ? e.stack : e);
}
await new Promise((resolve) => setTimeout(resolve, 0));
check(`${mode}-nothing-threw`, failure === null && thrown.length === 0, failure ?? (thrown.join('; ') || 'no error or unhandled rejection'));
await fetch(`/verdict/${mode}`, {
  method: 'POST',
  headers: { 'content-type': 'application/json' },
  body: JSON.stringify({ userAgent: navigator.userAgent, assertions }),
});
