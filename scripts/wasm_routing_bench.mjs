#!/usr/bin/env node
// Times one function's PUBLIC batch call two ways, ta-lib/js and the default
// entry with the WASM engine forced, to feed ta_codegen/input/wasm_routing.yaml
// through scripts/wasm_routing_table.py.
//
// Usage: node scripts/wasm_routing_bench.mjs <package-dir> <FUNC> [--bars=30,60,...]
//                                            [--rounds=N] [--sample-ms=20] [--warm-ms=1000]
//
// <package-dir> is an installed `ta-lib` package, or the library directory with
// dist/ built (bytes module included). Prints one JSON line per (function, bars).
// Run it inside `scripts/quiet.py measure`; scripts/wasm_routing_run.sh drives it.
//
// One function per process: V8's tier-up, inlining and GC state left behind by
// another function's calls would bias whichever arm runs after them.

import { readFileSync } from 'node:fs';
import { resolve, join } from 'node:path';
import { pathToFileURL } from 'node:url';

const DEFAULT_BARS = [30, 60, 125, 250, 500, 1000, 2500, 5000, 10000, 50000];

function usage(msg) {
  if (msg) console.error(msg);
  console.error('usage: wasm_routing_bench.mjs <package-dir> <FUNC> [--bars=a,b,...] [--rounds=N] [--sample-ms=MS] [--warm-ms=MS]');
  process.exit(2);
}

const positional = [];
const opt = { bars: DEFAULT_BARS, rounds: 15, sampleMs: 20, warmMs: 1000 };
for (const a of process.argv.slice(2)) {
  const m = /^--([a-z-]+)=(.*)$/.exec(a);
  if (!m) {
    positional.push(a);
    continue;
  }
  const [, k, v] = m;
  if (k === 'bars') opt.bars = v.split(',').map(Number);
  else if (k === 'rounds') opt.rounds = Number(v);
  else if (k === 'sample-ms') opt.sampleMs = Number(v);
  else if (k === 'warm-ms') opt.warmMs = Number(v);
  else usage(`unknown option --${k}`);
}
if (positional.length !== 2) usage();
if (!opt.bars.every((b) => Number.isInteger(b) && b > 0)) usage('--bars takes positive integers');
if (!(Number.isInteger(opt.rounds) && opt.rounds >= 1)) usage('--rounds takes a positive integer');
const [pkgArg, FUNC] = positional;
const bars = [...new Set(opt.bars)].sort((a, b) => a - b);

// --- the package's own entries, as a consumer's import would resolve them ---
const pkgDir = resolve(pkgArg);
const manifest = JSON.parse(readFileSync(join(pkgDir, 'package.json'), 'utf8'));
if (manifest.name !== 'ta-lib') usage(`${pkgDir} is not the ta-lib package`);
const fileOf = (target) => pathToFileURL(join(pkgDir, target)).href;
const exp = manifest.exports;
const nodeEntry = fileOf(exp['.'].node.default);
const engine = await import(nodeEntry);
const js = await import(fileOf(exp['./js'].default));
const meta = await import(fileOf(exp['./metadata'].default));
// The hooks must be the module instance the entry itself imports, or setting the
// mode changes nothing; engineStatus() after a forced call proves it below.
const hooks = await import(new URL('../runtime/wasm.js', nodeEntry).href);

const info = meta.Functions.byName(FUNC);
if (info === undefined) usage(`${FUNC}: not a function of this package`);
const want = FUNC.replace(/_/g, '').toLowerCase();
const fname = Object.keys(js).find((k) => typeof js[k] === 'function' && k.toLowerCase() === want);
if (fname === undefined) usage(`${FUNC}: no batch function in ta-lib/js`);
const jsFn = js[fname];
const wasmFn = engine[fname];
const emit = (o) => process.stdout.write(JSON.stringify(o) + '\n');

if (wasmFn === jsFn) {
  emit({ func: FUNC, wasm: false, note: 'no WASM export: the default entry re-exports ta-lib/js' });
  process.exit(0);
}

const lookback = js[fname + 'Lookback']({});
const outKey = info.outputs[0].key;

// --- input series ---
// Opens gap away from the previous close so candlestick patterns fire; prices
// have two decimals (dyadic values hide rounding differences), volume is positive.
function series(n) {
  let s = 0x2545f491;
  const rnd = () => {
    s = (s + 0x6d2b79f5) | 0;
    let t = Math.imul(s ^ (s >>> 15), 1 | s);
    t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
  const r2 = (x) => Math.round(x * 100) / 100;
  const o = new Float64Array(n), h = new Float64Array(n), l = new Float64Array(n);
  const c = new Float64Array(n), v = new Float64Array(n), oi = new Float64Array(n);
  let prev = 100;
  let trend = 1;
  for (let i = 0; i < n; i++) {
    // Runs of one colour, dojis and shaven bars, so the multi-bar and
    // marubozu-based patterns fire too.
    if (rnd() < 0.15) trend = -trend;
    const gap = (rnd() < 0.5 ? -1 : 1) * prev * (rnd() < 0.2 ? 0.02 + 0.03 * rnd() : 0.002 + 0.01 * rnd());
    let open = r2(prev + gap);
    const sign = rnd() < 0.7 ? trend : -trend;
    const body = rnd() < 0.1 ? 0 : sign * (0.002 + 0.03 * rnd()) * open;
    let close = r2(open + body);
    if (close < 20 || close > 500) close = r2(open - body);
    open = Math.max(open, 1);
    close = Math.max(close, 1);
    o[i] = open;
    c[i] = close;
    const shaven = rnd() < 0.12;
    h[i] = shaven || rnd() < 0.1 ? Math.max(open, close) : r2(Math.max(open, close) * (1 + 0.015 * rnd() * rnd()));
    l[i] = shaven || rnd() < 0.1 ? Math.min(open, close) : r2(Math.min(open, close) * (1 - 0.015 * rnd() * rnd()));
    v[i] = 1000 + Math.floor(rnd() * 100000);
    oi[i] = 5000 + Math.floor(rnd() * 50000);
    prev = close;
  }
  return { open: o, high: h, low: l, close: c, volume: v, openInterest: oi };
}

const { InputType, InputFlags } = meta;
const COMPONENTS = [
  [InputFlags.PRICE_OPEN, 'open'],
  [InputFlags.PRICE_HIGH, 'high'],
  [InputFlags.PRICE_LOW, 'low'],
  [InputFlags.PRICE_CLOSE, 'close'],
  [InputFlags.PRICE_VOLUME, 'volume'],
  [InputFlags.PRICE_OPENINTEREST, 'openInterest'],
];
// Real inputs take close, then open, high, low in call order (BETA, CORREL, ADD...).
const REAL_ORDER = ['close', 'open', 'high', 'low'];
const maxBars = bars[bars.length - 1];
const base = series(maxBars);

// Special case, keyed on the input's name: MAVP's `periods` must hold periods
// within the default [minPeriod, maxPeriod], or every bar clamps to one period.
function periods(n) {
  const lo = info.optInputs.find((p) => p.key === 'minPeriod')?.defaultValue ?? 2;
  const hi = info.optInputs.find((p) => p.key === 'maxPeriod')?.defaultValue ?? 30;
  const a = new Float64Array(n);
  for (let i = 0; i < n; i++) a[i] = lo + ((i * 7919) % (hi - lo + 1));
  return a;
}

function argsFor(n) {
  const args = [];
  let reals = 0;
  for (const input of info.inputs) {
    if (input.type === InputType.PRICE) {
      const p = {};
      for (const [flag, name] of COMPONENTS) if (input.flags & flag) p[name] = base[name].slice(0, n);
      args.push(p);
    } else if (input.type === InputType.REAL) {
      args.push(input.key === 'periods' ? periods(n) : base[REAL_ORDER[reals++ % REAL_ORDER.length]].slice(0, n));
    } else {
      console.error(`${FUNC}: input ${input.paramName} of type ${input.type} is not supported`);
      process.exit(2);
    }
  }
  return args;
}

// --- timing ---
const sink = new Float64Array(16);
globalThis.__wasmRoutingSink = sink;

// Each arm's loop is compiled from its own source (new Function), so the two
// loops have separate feedback: a shared loop would see two call targets and
// stop inlining either.
function runner(fn, args) {
  const params = args.map((_, i) => 'a' + i);
  const body =
    `return function run(reps) {\n` +
    `  const t0 = process.hrtime.bigint();\n` +
    `  for (let i = 0; i < reps; i++) {\n` +
    `    const r = f(${params.join(', ')}, {});\n` +
    `    sink[i & 15] = r[${JSON.stringify(outKey)}][0];\n` +
    `  }\n` +
    `  return Number(process.hrtime.bigint() - t0);\n` +
    `};`;
  return new Function('f', 'sink', 'process', ...params, body)(fn, sink, process, ...args);
}

const bitsOf = (a) => Buffer.from(a.buffer, a.byteOffset, a.byteLength);
function sameBits(a, b) {
  const ka = Object.keys(a).sort(), kb = Object.keys(b).sort();
  if (ka.join() !== kb.join()) return false;
  for (const k of ka) {
    if (k === 'begIdx') {
      if (a[k] !== b[k]) return false;
    } else if (!(ArrayBuffer.isView(a[k]) && ArrayBuffer.isView(b[k])) || a[k].constructor !== b[k].constructor || !bitsOf(a[k]).equals(bitsOf(b[k]))) {
      return false;
    }
  }
  return true;
}

const sampleNs = opt.sampleMs * 1e6;
// From the fastest per-call time seen, over at least three runs: a first call at
// a new size can pay a recompile that would otherwise shrink the sample tenfold.
function calibrate(run) {
  let reps = 1, perCall = Infinity, long = 0;
  while (long < 3) {
    const t = Math.max(run(reps), 1);
    perCall = Math.min(perCall, t / reps);
    if (t >= sampleNs / 2) long++;
    reps = Math.min(reps * 100, Math.max(1, Math.ceil(sampleNs / perCall)));
  }
  return Math.max(1, Math.round(sampleNs / perCall));
}

function stats(ns) {
  const s = [...ns].sort((a, b) => a - b);
  const q = (p) => {
    const x = p * (s.length - 1), i = Math.floor(x);
    return i + 1 < s.length ? s[i] + (s[i + 1] - s[i]) * (x - i) : s[i];
  };
  const median = q(0.5);
  const r = (x) => Math.round(x * 100) / 100;
  return {
    min: r(s[0]),
    median: r(median),
    max: r(s[s.length - 1]),
    spread: r((100 * (s[s.length - 1] - s[0])) / median),
    iqr: r((100 * (q(0.75) - q(0.25))) / median),
  };
}

hooks.setEngineMode('force');
const measured = bars.filter((n) => n >= lookback + 2);
for (const n of bars) {
  if (n < lookback + 2) emit({ func: FUNC, bars: n, lookback, skip: 'bars below lookback + 2' });
}

let warmed = false;
for (const n of measured) {
  const args = argsFor(n);

  hooks.resetFallThroughs();
  const got = wasmFn(...args, {});
  const fell = hooks.fallThroughCount();
  const status = hooks.engineStatus();
  const ref = jsFn(...args, {});
  const equal = sameBits(got, ref);
  if (fell !== 0 || status.engine !== 'wasm' || !equal) {
    emit({ func: FUNC, bars: n, error: 'forced call did not run in WASM with the bits of ta-lib/js', fallThroughs: fell, reason: hooks.lastFallThrough(), engine: status, bitsEqual: equal });
    process.exit(1);
  }
  const out = ref[outKey];
  let nonzero = 0;
  for (let i = 0; i < out.length; i++) if (out[i] !== 0) nonzero++;

  const arms = { js: runner(jsFn, args), wasm: runner(wasmFn, args) };
  if (!warmed) {
    // Past TurboFan tier-up for both arms, alternating so neither is warmer.
    const until = { js: opt.warmMs * 1e6, wasm: opt.warmMs * 1e6 };
    let reps = 1;
    while (until.js > 0 || until.wasm > 0) {
      for (const k of ['js', 'wasm']) if (until[k] > 0) until[k] -= arms[k](reps);
      reps = Math.min(reps * 2, 1 << 20);
    }
    warmed = true;
  }
  const reps = { js: calibrate(arms.js), wasm: calibrate(arms.wasm) };
  arms.js(reps.js);
  arms.wasm(reps.wasm);

  const ns = { js: [], wasm: [] };
  for (let r = 0; r < opt.rounds; r++) {
    for (const k of r % 2 === 0 ? ['js', 'wasm'] : ['wasm', 'js']) ns[k].push(arms[k](reps[k]) / reps[k]);
  }
  const fellDuring = hooks.fallThroughCount();
  if (fellDuring !== 0) {
    emit({ func: FUNC, bars: n, error: 'fall-through while timing', fallThroughs: fellDuring, reason: hooks.lastFallThrough() });
    process.exit(1);
  }
  const js_ = { reps: reps.js, ...stats(ns.js) };
  const wasm_ = { reps: reps.wasm, ...stats(ns.wasm) };
  emit({
    func: FUNC,
    bars: n,
    lookback,
    rounds: opt.rounds,
    bitsEqual: true,
    fallThroughs: 0,
    nonzero,
    outputs: out.length,
    js: js_,
    wasm: wasm_,
    ratio: Math.round((wasm_.median / js_.median) * 10000) / 10000,
  });
}
