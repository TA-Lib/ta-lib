#!/usr/bin/env node
// Times one function's PUBLIC batch call two ways, ta-lib/js and the default
// entry with the WASM engine forced, to feed ta_codegen/input/wasm_routing.yaml
// through scripts/wasm_routing_table.py.
//
// Usage: node scripts/wasm_routing_bench.mjs <package-dir> <FUNC> [--bars=30,60,...]
//                                            [--rounds=N] [--sample-ms=20] [--warm-ms=1000]
//                                            [--form=options|buffered|abstract]
//
// <package-dir> is an installed `ta-lib` package, or the library directory with
// dist/ built (bytes module included). Prints one JSON line per (function, bars).
// Run it inside `scripts/quiet.py measure`; scripts/wasm_routing_run.sh drives it.
// It re-executes itself under `--single-threaded-gc` when started without it.
//
// --form picks the call both arms time: `options` (the default, and the only
// form the table takes) passes an options object and lets the call allocate its
// outputs; `buffered` passes output buffers allocated once per bar count;
// `abstract` calls a parameter holder of ta-lib/js/abstract against one of
// ta-lib/abstract, both bound once per bar count.
//
// One function per process: V8's tier-up, inlining and GC state left behind by
// another function's calls would bias whichever arm runs after them.

import { spawnSync } from 'node:child_process';
import { readFileSync } from 'node:fs';
import { resolve, join } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';
import { PerformanceObserver, constants as perfConstants, performance } from 'node:perf_hooks';

// V8's parallel GC can fall into a run of full GCs that free almost nothing and
// slow both arms for the rest of the process; the single-threaded GC does not.
if (!process.execArgv.some((a) => a === '--single-threaded-gc' || a === '--single_threaded_gc')) {
  const child = spawnSync(process.execPath, [...process.execArgv, '--single-threaded-gc', fileURLToPath(import.meta.url), ...process.argv.slice(2)], { stdio: 'inherit' });
  if (child.error) throw child.error;
  if (child.signal) process.kill(process.pid, child.signal);
  process.exit(child.status ?? 1);
}

const FORMS = ['options', 'buffered', 'abstract'];
const DEFAULT_BARS = [30, 60, 125, 250, 500, 1000, 2500, 5000, 10000, 50000, 100000, 200000];

function usage(msg) {
  if (msg) console.error(msg);
  console.error('usage: wasm_routing_bench.mjs <package-dir> <FUNC> [--bars=a,b,...] [--rounds=N] [--sample-ms=MS] [--warm-ms=MS] [--form=options|buffered|abstract]');
  process.exit(2);
}

const positional = [];
const opt = { bars: DEFAULT_BARS, rounds: 15, sampleMs: 20, warmMs: 1000, form: 'options' };
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
  else if (k === 'form') opt.form = v;
  else usage(`unknown option --${k}`);
}
if (positional.length !== 2) usage();
if (!opt.bars.every((b) => Number.isInteger(b) && b > 0)) usage('--bars takes positive integers');
if (!(Number.isInteger(opt.rounds) && opt.rounds >= 1)) usage('--rounds takes a positive integer');
if (!FORMS.includes(opt.form)) usage(`--form takes ${FORMS.join(', ')}`);
const [pkgArg, FUNC] = positional;
const bars = [...new Set(opt.bars)].sort((a, b) => a - b);

// --- the package's own entries, as a consumer's import would resolve them ---
const pkgDir = resolve(pkgArg);
const manifest = JSON.parse(readFileSync(join(pkgDir, 'package.json'), 'utf8'));
if (manifest.name !== 'ta-lib') usage(`${pkgDir} is not the ta-lib package`);
const fileOf = (target) => pathToFileURL(join(pkgDir, target)).href;
const exp = manifest.exports;
const emit = (o) => process.stdout.write(JSON.stringify(o) + '\n');
const nodeEntry = fileOf(exp['.'].node.default);
const engine = await import(nodeEntry);
const js = await import(fileOf(exp['./js'].default));
// The routed abstract entry loads every engine module, and the more code a
// process loads, the likelier the GC storm above: metadata comes from the pure one.
const pureAbstract = exp['./js/abstract'];
if (opt.form === 'abstract' && pureAbstract === undefined) {
  emit({ func: positional[1], form: opt.form, error: 'this package has no ta-lib/js/abstract, so no routed ta-lib/abstract to time against it' });
  process.exit(1);
}
const meta = await import(fileOf((pureAbstract ?? exp['./abstract']).default));
const routedAbstract = opt.form === 'abstract' ? await import(fileOf(exp['./abstract'].node?.default ?? exp['./abstract'].default)) : null;
// The hooks must be the module instance the entry itself imports, or setting the
// mode changes nothing; engineStatus() after a forced call proves it below.
const hooks = await import(new URL('../runtime/wasm.js', nodeEntry).href);

const info = meta.Functions.byName(FUNC);
if (info === undefined) usage(`${FUNC}: not a function of this package`);
const want = FUNC.replace(/_/g, '').toLowerCase();
// A reserved word is exported with a trailing underscore (`var_`); the lookback keeps the bare stem.
const fname = Object.keys(js).find((k) => typeof js[k] === 'function' && k.replace(/_$/, '').toLowerCase() === want);
if (fname === undefined) usage(`${FUNC}: no batch function in ta-lib/js`);
const jsFn = js[fname];
const wasmFn = engine[fname];

if (wasmFn === jsFn) {
  emit({ func: FUNC, form: opt.form, wasm: false, note: 'no WASM export: the default entry re-exports ta-lib/js' });
  process.exit(0);
}

const lookback = js[fname.replace(/_$/, '') + 'Lookback']({});
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
function runner(fn, bound, call, read) {
  const names = Object.keys(bound);
  const body =
    `return function run(reps) {\n` +
    `  const t0 = process.hrtime.bigint();\n` +
    `  for (let i = 0; i < reps; i++) {\n` +
    `    const r = ${call};\n` +
    `    sink[i & 15] = ${read};\n` +
    `  }\n` +
    `  return Number(process.hrtime.bigint() - t0);\n` +
    `};`;
  return new Function('f', 'sink', 'process', ...names, body)(fn, sink, process, ...Object.values(bound));
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

const sameBuffers = (a, b) => a.every((x, k) => bitsOf(x).equals(bitsOf(b[k])));
const blank = (n) => info.outputs.map((o) => (o.type === meta.OutputType.REAL ? new Float64Array(n) : new Int32Array(n)));

function holder(lib, args, outs) {
  const f = lib.Functions.byName(FUNC);
  const h = f.newCall();
  f.inputs.forEach((input, i) => (input.type === lib.InputType.PRICE ? h.setPriceInput(i, args[i]) : h.setInput(i, args[i])));
  outs.forEach((o, k) => h.setOutput(k, o));
  return h;
}

// Each arm: `once()` makes the call the bit check compares, `run` is its timed
// loop; both make the same call on the same bound state.
function armsFor(n, args) {
  const named = Object.fromEntries(args.map((a, i) => ['a' + i, a]));
  const list = Object.keys(named).join(', ');
  const read = `r[${JSON.stringify(outKey)}][0]`;
  if (opt.form === 'options') {
    const arm = (fn) => ({ once: () => fn(...args, {}), run: runner(fn, named, `f(${list}, {})`, read) });
    return { js: arm(jsFn), wasm: arm(wasmFn), same: sameBits, out: (ref) => ref[outKey] };
  }
  if (opt.form === 'buffered') {
    const arm = (fn) => {
      const bufs = blank(n);
      const o = Object.fromEntries(info.outputs.map((x, k) => [x.key === 'values' ? x.paramName : x.key, bufs[k]]));
      return { bufs, once: () => fn(...args, o), run: runner(fn, { ...named, o }, `f(${list}, o)`, read) };
    };
    const a = { js: arm(jsFn), wasm: arm(wasmFn) };
    const into = (r, bufs) => info.outputs.every((x, k) => r[x.key].buffer === bufs[k].buffer);
    return {
      ...a,
      same: (got, ref) => into(got, a.wasm.bufs) && into(ref, a.js.bufs) && sameBits(got, ref) && sameBuffers(a.wasm.bufs, a.js.bufs),
      out: (ref) => ref[outKey],
    };
  }
  const arm = (lib) => {
    const bufs = blank(n);
    const h = holder(lib, args, bufs);
    return { bufs, once: () => h.call(0, n - 1), run: runner(h, { e: n - 1 }, 'f.call(0, e)', 'r.count') };
  };
  const a = { js: arm(meta), wasm: arm(routedAbstract) };
  return {
    ...a,
    same: (got, ref) => got.begIdx === ref.begIdx && got.count === ref.count && sameBuffers(a.wasm.bufs, a.js.bufs),
    out: (ref) => a.js.bufs[0].subarray(0, ref.count),
  };
}

const NOT_WASM = {
  options: 'forced call did not run in WASM with the bits of ta-lib/js',
  buffered: 'forced call into caller buffers did not run in WASM with the bits of ta-lib/js (a package from before buffered calls ran in WASM fails here)',
  abstract: 'forced ta-lib/abstract call did not run in WASM with the bits of ta-lib/js/abstract (a package whose ta-lib/abstract is not routed fails here)',
};

const settle = () => new Promise((r) => setImmediate(r));
// Node creates a gc entry from a native immediate after the collection, so the
// entries of a synchronous timed loop exist only once the event loop has turned.
function watchMajorGcs() {
  const seen = [];
  const observer = new PerformanceObserver((list) => seen.push(...list.getEntries()));
  observer.observe({ type: 'gc' });
  const t0 = performance.now();
  return async () => {
    const t1 = performance.now();
    await settle();
    await settle();
    seen.push(...observer.takeRecords());
    observer.disconnect();
    return seen.filter((e) => (e.detail?.kind ?? e.kind) === perfConstants.NODE_PERFORMANCE_GC_MAJOR && e.startTime >= t0 && e.startTime <= t1).length;
  };
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
  if (n < lookback + 2) emit({ func: FUNC, form: opt.form, bars: n, lookback, skip: 'bars below lookback + 2' });
}

let warmed = false;
for (const n of measured) {
  const form = armsFor(n, argsFor(n));

  hooks.resetFallThroughs();
  const entered = hooks.moduleCalls();
  const got = form.wasm.once();
  const moduleCalls = hooks.moduleCalls() - entered;
  const fell = hooks.fallThroughCount();
  const status = hooks.engineStatus();
  const ref = form.js.once();
  const equal = form.same(got, ref);
  if (fell !== 0 || moduleCalls !== 1 || status.engine !== 'wasm' || !equal) {
    emit({ func: FUNC, form: opt.form, bars: n, error: NOT_WASM[opt.form], fallThroughs: fell, moduleCalls, reason: hooks.lastFallThrough(), engine: status, bitsEqual: equal });
    process.exit(1);
  }
  const out = form.out(ref);
  let nonzero = 0;
  for (let i = 0; i < out.length; i++) if (out[i] !== 0) nonzero++;

  const arms = { js: form.js.run, wasm: form.wasm.run };
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
  const majorGcs = watchMajorGcs();
  for (let r = 0; r < opt.rounds; r++) {
    for (const k of r % 2 === 0 ? ['js', 'wasm'] : ['wasm', 'js']) ns[k].push(arms[k](reps[k]) / reps[k]);
  }
  const majors = await majorGcs();
  const fellDuring = hooks.fallThroughCount();
  if (fellDuring !== 0) {
    emit({ func: FUNC, form: opt.form, bars: n, error: 'fall-through while timing', fallThroughs: fellDuring, reason: hooks.lastFallThrough() });
    process.exit(1);
  }
  const js_ = { reps: reps.js, ...stats(ns.js) };
  const wasm_ = { reps: reps.wasm, ...stats(ns.wasm) };
  emit({
    func: FUNC,
    form: opt.form,
    bars: n,
    lookback,
    rounds: opt.rounds,
    bitsEqual: true,
    fallThroughs: 0,
    majorGcs: majors,
    nonzero,
    outputs: out.length,
    js: js_,
    wasm: wasm_,
    ratio: Math.round((wasm_.median / js_.median) * 10000) / 10000,
  });
}
