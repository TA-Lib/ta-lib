/* TA-LIB Copyright (c) 1999-2026, Mario Fortier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither name of author nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  111226 MF,CC  First Version
 */

// The default entry's argument contract: whatever the WASM path declines, the
// call answers or throws exactly as ta-lib/js does. Each case runs in force
// mode, which routes every call regardless of the routing table and falls
// through as the table mode would, and states how many fall-throughs it makes.

import assert from 'node:assert/strict';
import { test } from 'node:test';
import vm from 'node:vm';

import * as routed from 'ta-lib';
import { beta, Core, CoreBuilder, ema, FuncUnstId, ma, macd, MAType, OutRange, ready, TALibError } from 'ta-lib';
import * as js from 'ta-lib/js';
import { hooks } from './engineHooks.ts';

const { WASM_BASE64 } = (await import(new URL('../runtime/wasmBytes.js', import.meta.resolve('ta-lib')).href)) as { WASM_BASE64: string };

const N = 300;
const close = Float64Array.from({ length: N }, (_, i) => Math.round((100 + 10 * Math.sin(i / 9) + i / 40) * 100) / 100);

type Outcome = { value?: Record<string, unknown>; error?: unknown };

function outcome(call: () => unknown): Outcome {
  try {
    return { value: call() as Record<string, unknown> };
  } catch (error) {
    return { error };
  }
}

function bits(a: Float64Array | Int32Array): Buffer {
  return Buffer.from(a.buffer, a.byteOffset, a.byteLength);
}

function same(got: Outcome, want: Outcome, where: string): void {
  if (want.error !== undefined) {
    assert.ok(want.error instanceof Error, `${where}: ta-lib/js threw ${String(want.error)}`);
    assert.ok(got.error instanceof Error, `${where}: ta-lib gave ${String(got.error)}, not ${want.error.message}`);
    assert.equal(got.error.constructor, want.error.constructor, where);
    assert.equal(got.error.message, want.error.message, where);
    if (want.error instanceof TALibError) assert.equal((got.error as TALibError).code, want.error.code, where);
    return;
  }
  assert.equal(got.error, undefined, `${where}: ${String(got.error)}`);
  const a = got.value as Record<string, unknown>;
  const b = want.value as Record<string, unknown>;
  if (b instanceof OutRange) {
    assert.ok(a instanceof OutRange, `${where}: ta-lib gave no OutRange`);
    assert.deepEqual([a.begIdx, a.count], [b.begIdx, b.count], where);
    return;
  }
  assert.deepEqual(Object.keys(a), Object.keys(b), where);
  assert.equal(a.begIdx, b.begIdx, `${where}: begIdx`);
  for (const key of Object.keys(b).filter((k) => k !== 'begIdx')) {
    const x = a[key] as Float64Array;
    const y = b[key] as Float64Array;
    assert.equal(x.constructor, y.constructor, `${where}: ${key} type`);
    assert.ok(bits(x).equals(bits(y)), `${where}: ${key} differs`);
  }
}

type Checked = { fellThrough: number; ran: number; got: Outcome };

/** Both calls' outcomes compared; returns the fall-throughs the ta-lib call made, the batch calls the module counted, and its outcome. */
function check(where: string, viaDefault: () => unknown, viaJs: () => unknown): Checked {
  hooks.setEngineMode('force');
  const before = hooks.fallThroughCount();
  const entered = hooks.moduleCalls();
  let got: Outcome;
  try {
    got = outcome(viaDefault);
  } finally {
    hooks.setEngineMode('table');
  }
  const fellThrough = hooks.fallThroughCount() - before;
  const ran = hooks.moduleCalls() - entered;
  same(got, outcome(viaJs), where);
  return { fellThrough, ran, got };
}

function ranInWasm(r: Checked, where: string): void {
  assert.equal(r.fellThrough, 0, `${where}: fell through`);
  assert.equal(r.ran, 1, `${where}: the module counted ${r.ran} batch calls, not one`);
}

test('caller output buffers are filled in WASM, and each result is a view of its buffer', () => {
  const mine = new Float64Array(N + 7).fill(-1);
  const r = check('outReal', () => ema(close, { outReal: mine }), () => js.ema(close, { outReal: new Float64Array(N + 7).fill(-1) }));
  ranInWasm(r, 'outReal');
  const values = r.got.value?.values as Float64Array;
  assert.equal(values.buffer, mine.buffer);
  assert.equal(values.byteOffset, 0);
  assert.ok(mine.subarray(values.length).every((v) => v === -1), 'an element past the count was written');
  const signal = new Float64Array(N);
  const partial = check(
    'outMacdSignal alone',
    () => macd(close, { outMacdSignal: signal }),
    () => js.macd(close, { outMacdSignal: new Float64Array(N) }),
  );
  ranInWasm(partial, 'outMacdSignal alone');
  assert.equal((partial.got.value?.outMacdSignal as Float64Array).buffer, signal.buffer);
  assert.notEqual((partial.got.value?.outMacd as Float64Array).buffer, signal.buffer);
});

const wanted = N - js.emaLookback({ timePeriod: 10 });
const sharedInput = close.slice();

/** Buffers ta-lib/js rejects: each falls through before the module, and the call throws the ta-lib/js error without writing to any of them. */
const REJECTED: [string, () => { outReal?: ArrayBufferView }][] = [
  ['a Float32Array', () => ({ outReal: new Float32Array(N) })],
  ['an Int32Array for a real output', () => ({ outReal: new Int32Array(N) })],
  ['one element too short', () => ({ outReal: new Float64Array(wanted - 1) })],
  ['empty', () => ({ outReal: new Float64Array(0) })],
  ['overlapping the input', () => ({ outReal: new Float64Array(sharedInput.buffer, 8, N - 1) })],
];

test('an output buffer ta-lib/js rejects falls through as buffer before the module and is left untouched', () => {
  for (const [where, make] of REJECTED) {
    const mine = make().outReal!;
    const before = Buffer.from(new Uint8Array(mine.buffer).slice());
    const r = check(where, () => ema(sharedInput, { timePeriod: 10, outReal: mine as never }), () => js.ema(sharedInput, { timePeriod: 10, outReal: make().outReal as never }));
    assert.ok(r.got.error instanceof TALibError, `${where}: ta-lib/js accepted it`);
    assert.equal(r.fellThrough, 1, where);
    assert.equal(hooks.lastFallThrough(), 'buffer', where);
    assert.equal(r.ran, 0, `${where}: entered the module`);
    assert.ok(Buffer.from(new Uint8Array(mine.buffer)).equals(before), `${where}: the buffer was written`);
  }
  const shared = new ArrayBuffer(N * 8 * 2);
  const outMacd = new Float64Array(shared, 0, N);
  const outMacdSignal = new Float64Array(shared, 8, N);
  const r = check(
    'two outputs overlapping',
    () => macd(close, { outMacd, outMacdSignal }),
    () => js.macd(close, { outMacd, outMacdSignal }),
  );
  assert.ok(r.got.error instanceof TALibError);
  assert.equal(r.fellThrough, 1);
  assert.equal(hooks.lastFallThrough(), 'buffer');
  assert.equal(r.ran, 0);
  assert.ok(new Float64Array(shared).every((v) => v === 0), 'two overlapping outputs: a buffer was written');
});

test('an output that is the input itself falls through as buffer and gets the ta-lib/js values; a Float64Array subclass output runs in WASM', () => {
  const inPlace = close.slice();
  const r = check('in place', () => ema(inPlace, { outReal: inPlace }), () => js.ema(close.slice(), { outReal: new Float64Array(N) }));
  assert.equal(r.got.error, undefined);
  assert.equal(r.fellThrough, 1);
  assert.equal(hooks.lastFallThrough(), 'buffer');
  assert.equal(r.ran, 0);
  class Mine extends Float64Array {}
  const mine = new Mine(N);
  const sub = check('a Float64Array subclass', () => ema(close, { outReal: mine }), () => js.ema(close, { outReal: new Mine(N) }));
  ranInWasm(sub, 'a Float64Array subclass');
  assert.equal((sub.got.value?.values as Float64Array).buffer, mine.buffer);
});

type Positional = (...args: unknown[]) => OutRange;
const entry = import.meta.resolve('ta-lib');
const routedEma = (await import(new URL('./func/ema.js', entry).href)) as { emaPositional: Positional };
const jsEma = (await import(new URL('../func/ema.js', entry).href)) as { emaPositional: Positional };
const routedMacd = (await import(new URL('./func/macd.js', entry).href)) as { macdPositional: Positional };
const jsMacd = (await import(new URL('../func/macd.js', entry).href)) as { macdPositional: Positional };
const DEFAULTS = [Core.INTEGER_DEFAULT, Core.INTEGER_DEFAULT, Core.INTEGER_DEFAULT];

/** Three MACD outputs over one ArrayBuffer, and the input in it too, at the element offsets given. */
function arena(at: [input: number, macd: number, signal: number, hist: number]): { memory: ArrayBuffer; input: Float64Array; outs: Float64Array<ArrayBuffer>[] } {
  const memory = new ArrayBuffer(8 * (Math.max(...at) + N + 4));
  new Float64Array(memory).fill(-3);
  const input = new Float64Array(memory, 8 * at[0], N);
  input.set(close);
  return { memory, input, outs: at.slice(1).map((k) => new Float64Array(memory, 8 * k, N)) };
}

test('outputs at disjoint offsets of one ArrayBuffer, beside the input in it, run in WASM with the ta-lib/js bytes, in both call forms', () => {
  const layouts: [string, [number, number, number, number]][] = [
    ['apart', [3, N + 5, 2 * N + 7, 3 * N + 9]],
    ['each touching the next', [0, N, 2 * N, 3 * N]],
  ];
  for (const [label, at] of layouts) {
    const a = arena(at);
    const b = arena(at);
    const keyed = (x: Float64Array<ArrayBuffer>[]) => ({ outMacd: x[0], outMacdSignal: x[1], outMacdHist: x[2] });
    const where = `options, ${label}`;
    ranInWasm(check(where, () => macd(a.input, keyed(a.outs)), () => js.macd(b.input, keyed(b.outs))), where);
    assert.ok(Buffer.from(a.memory).equals(Buffer.from(b.memory)), `${where}: the bytes differ`);
    const p = arena(at);
    const q = arena(at);
    const pos = `positional, ${label}`;
    const r = check(pos, () => routedMacd.macdPositional(0, N - 1, p.input, ...DEFAULTS, ...p.outs), () => jsMacd.macdPositional(0, N - 1, q.input, ...DEFAULTS, ...q.outs));
    ranInWasm(r, pos);
    assert.ok(Buffer.from(p.memory).equals(Buffer.from(q.memory)), `${pos}: the bytes differ`);
  }
});

test('outputs sharing one element with each other or with the input fall through as buffer and answer as ta-lib/js does, in both call forms', () => {
  const layouts: [string, [number, number, number, number]][] = [
    ['two outputs sharing one element', [0, N, 2 * N - 1, 3 * N]],
    ['an output sharing one element with the input', [0, N - 1, 2 * N, 3 * N]],
    ['an output exactly over the input', [0, 0, N, 2 * N]],
  ];
  for (const [label, at] of layouts) {
    for (const form of ['options', 'positional']) {
      const a = arena(at);
      const b = arena(at);
      const run = (x: ReturnType<typeof arena>, viaJs: boolean): unknown =>
        form === 'options'
          ? (viaJs ? js.macd : macd)(x.input, { outMacd: x.outs[0], outMacdSignal: x.outs[1], outMacdHist: x.outs[2] })
          : (viaJs ? jsMacd : routedMacd).macdPositional(0, N - 1, x.input, ...DEFAULTS, ...x.outs);
      const where = `${form}, ${label}`;
      const r = check(where, () => run(a, false), () => run(b, true));
      assert.equal(r.fellThrough, 1, where);
      assert.equal(hooks.lastFallThrough(), 'buffer', where);
      assert.equal(r.ran, 0, `${where}: entered the module`);
      assert.ok(Buffer.from(a.memory).equals(Buffer.from(b.memory)), `${where}: the bytes differ`);
    }
  }
});

test('a call ta-lib/js rejects for its range, or for a parameter when buffers are passed, runs no element valueOf on the routed path either, in both call forms', () => {
  let calls = 0;
  const counted = (): number[] =>
    Array.from(close, (v) => ({
      valueOf(): number {
        calls++;
        return v;
      },
    })) as unknown as number[];
  const cases: [string, (viaJs: boolean) => unknown][] = [
    ['options, accepted', (j) => (j ? js.ema : ema)(counted(), { timePeriod: 10 })],
    ['options, an output one element short', (j) => (j ? js.ema : ema)(counted(), { timePeriod: 10, outReal: new Float64Array(wanted - 1) })],
    ['options, endIdx past the input', (j) => (j ? js.ema : ema)(counted(), { endIdx: N })],
    ['options, startIdx after endIdx', (j) => (j ? js.ema : ema)(counted(), { startIdx: 50, endIdx: 40 })],
    ['options, timePeriod 0 into a buffer', (j) => (j ? js.ema : ema)(counted(), { timePeriod: 0, outReal: new Float64Array(N) })],
    ['positional, an output one element short', (j) => (j ? jsEma : routedEma).emaPositional(0, N - 1, counted(), 10, new Float64Array(wanted - 1))],
    ['positional, endIdx past the input', (j) => (j ? jsEma : routedEma).emaPositional(0, N, counted(), 10, new Float64Array(N + 1))],
    ['positional, timePeriod 0', (j) => (j ? jsEma : routedEma).emaPositional(0, N - 1, counted(), 0, new Float64Array(N))],
  ];
  for (const [where, call] of cases) {
    hooks.setEngineMode('force');
    const before = hooks.fallThroughCount();
    const entered = hooks.moduleCalls();
    calls = 0;
    let got: Outcome;
    try {
      got = outcome(() => call(false));
    } finally {
      hooks.setEngineMode('table');
    }
    const mine = calls;
    const r: Checked = { fellThrough: hooks.fallThroughCount() - before, ran: hooks.moduleCalls() - entered, got };
    calls = 0;
    same(got, outcome(() => call(true)), where);
    if (where.endsWith('accepted')) {
      ranInWasm(r, where);
      assert.equal(mine, N, `${where}: valueOf ran ${mine} times`);
      continue;
    }
    assert.ok(got.error instanceof TALibError, `${where}: ta-lib/js accepted it`);
    assert.equal(calls, 0, `${where}: ta-lib/js ran valueOf ${calls} times`);
    assert.equal(mine, 0, `${where}: the routed call ran valueOf ${mine} times`);
    assert.equal(r.fellThrough, 1, where);
    assert.equal(r.ran, 0, where);
  }
});

test('the positional tier runs in WASM into the caller buffer, and falls through as buffer, untouched, on one ta-lib/js rejects', () => {
  const call = (f: Positional, input: Float64Array, out: unknown): Outcome => outcome(() => f(0, N - 1, input, 10, out));
  const mine = new Float64Array(N).fill(-1);
  const theirs = new Float64Array(N).fill(-1);
  hooks.setEngineMode('strict');
  let entered = hooks.moduleCalls();
  let got: Outcome;
  try {
    got = call(routedEma.emaPositional, close, mine);
  } finally {
    hooks.setEngineMode('table');
  }
  assert.equal(hooks.moduleCalls() - entered, 1, 'the positional tier did not run in the module');
  const want = call(jsEma.emaPositional, close, theirs);
  const a = got.value as unknown as OutRange;
  const b = want.value as unknown as OutRange;
  assert.ok(a instanceof OutRange);
  assert.deepEqual([a.begIdx, a.count], [b.begIdx, b.count]);
  assert.ok(bits(mine).equals(bits(theirs)));
  for (const [where, make] of [...REJECTED, ['absent', () => ({})]] as [string, () => { outReal?: ArrayBufferView }][]) {
    const out = make().outReal;
    const before = out === undefined ? null : Buffer.from(new Uint8Array(out.buffer).slice());
    hooks.setEngineMode('force');
    const fell = hooks.fallThroughCount();
    entered = hooks.moduleCalls();
    let r: Outcome;
    try {
      r = call(routedEma.emaPositional, sharedInput, out);
    } finally {
      hooks.setEngineMode('table');
    }
    const w = call(jsEma.emaPositional, sharedInput, make().outReal);
    assert.ok(w.error instanceof TALibError, `${where}: ta-lib/js accepted it`);
    assert.ok(r.error instanceof TALibError, `${where}: ${String(r.error)}`);
    assert.equal(r.error.message, w.error.message, where);
    assert.equal(hooks.fallThroughCount() - fell, 1, where);
    assert.equal(hooks.lastFallThrough(), 'buffer', where);
    assert.equal(hooks.moduleCalls(), entered, `${where}: entered the module`);
    if (out !== undefined) assert.ok(Buffer.from(new Uint8Array(out.buffer)).equals(before!), `${where}: the buffer was written`);
  }
});

test('a Core other than Core.DEFAULT falls through without loading the engine', () => {
  const core = new CoreBuilder().unstablePeriod(FuncUnstId.EMA, 7).build();
  hooks.resetEngine();
  hooks.provideWasmBytes(WASM_BASE64);
  const r = check('core', () => ema(close, { timePeriod: 10, core }), () => js.ema(close, { timePeriod: 10, core }));
  assert.equal(r.fellThrough, 1);
  assert.equal(hooks.lastFallThrough(), 'options');
  assert.deepEqual(hooks.engineStatus(), { engine: 'js', reason: 'not-loaded' }, 'a call that fell through on its core loaded the engine');
  assert.equal(hooks.wasmMemoryBytes(), 0);
  ranInWasm(check('then Core.DEFAULT', () => ema(close, { timePeriod: 10 }), () => js.ema(close, { timePeriod: 10 })), 'then Core.DEFAULT');
  assert.deepEqual(hooks.engineStatus(), { engine: 'wasm', reason: null });
});

test('startIdx and endIdx are routed; out of range they are rejected as ta-lib/js rejects them', () => {
  const inRange = check('10..200', () => ema(close, { startIdx: 10, endIdx: 200 }), () => js.ema(close, { startIdx: 10, endIdx: 200 }));
  ranInWasm(inRange, '10..200');
  for (const range of [{ endIdx: N }, { startIdx: -1 }, { startIdx: 50, endIdx: 40 }, { endIdx: 2 ** 31 }]) {
    const r = check(JSON.stringify(range), () => ema(close, range), () => js.ema(close, range));
    assert.equal(r.fellThrough, 1, JSON.stringify(range));
    assert.ok(r.got.error instanceof TALibError, JSON.stringify(range));
  }
});

test('an integer parameter of 2.5 is rejected as ta-lib/js rejects it', () => {
  const r = check('timePeriod 2.5', () => ema(close, { timePeriod: 2.5 }), () => js.ema(close, { timePeriod: 2.5 }));
  assert.equal(r.fellThrough, 1);
  assert.ok(r.got.error instanceof TALibError);
});

test('inputs of different lengths fall through', () => {
  const longer = Float64Array.from({ length: N + 5 }, (_, i) => 50 + (i % 7));
  const r = check('beta', () => beta(close, longer), () => js.beta(close, longer));
  assert.equal(r.fellThrough, 1);
  assert.equal(r.got.error, undefined);
});

test('an empty input is rejected as ta-lib/js rejects it', () => {
  const empty = new Float64Array(0);
  const r = check('empty', () => ema(empty), () => js.ema(empty));
  assert.equal(r.fellThrough, 1);
  assert.ok(r.got.error instanceof TALibError);
});

test('a number[], a Float32Array, a Float64Array subclass and views over a SharedArrayBuffer and a resizable ArrayBuffer are routed and give the bits of ta-lib/js', () => {
  class Mine extends Float64Array {}
  const Resizable = ArrayBuffer as unknown as new (byteLength: number, options: { maxByteLength: number }) => ArrayBuffer;
  const over = (memory: ArrayBufferLike): Float64Array => {
    const a = new Float64Array(memory);
    a.set(close);
    return a;
  };
  const inputs: [string, ArrayLike<number>][] = [
    ['number[]', Array.from(close)],
    ['Float32Array', Float32Array.from(close)],
    ['Float64Array subclass', Mine.from(close)],
    ['SharedArrayBuffer view', over(new SharedArrayBuffer(N * 8))],
    ['resizable ArrayBuffer view', over(new Resizable(N * 8, { maxByteLength: 2 * N * 8 }))],
  ];
  for (const [where, input] of inputs) {
    ranInWasm(check(where, () => ema(input), () => js.ema(input)), where);
    const into = `${where}, into a buffer`;
    ranInWasm(check(into, () => ema(input, { outReal: new Float64Array(N) }), () => js.ema(input, { outReal: new Float64Array(N) })), into);
  }
  const out = over(new SharedArrayBuffer(N * 8)) as Float64Array<ArrayBuffer>;
  ranInWasm(check('a SharedArrayBuffer output', () => ema(close, { outReal: out }), () => js.ema(close, { outReal: new Float64Array(N) })), 'a SharedArrayBuffer output');
});

test('MA type 7 (MAMA) falls through to ta-lib/js; 99 is rejected as ta-lib/js rejects it', () => {
  assert.equal(MAType.MAMA, 7);
  const mama = check('maType 7', () => ma(close, { maType: MAType.MAMA }), () => js.ma(close, { maType: MAType.MAMA }));
  assert.equal(mama.fellThrough, 1);
  assert.equal(hooks.lastFallThrough(), 'declined');
  assert.equal(mama.ran, 0);
  assert.equal(mama.got.error, undefined);
  const bad = 99 as MAType;
  const r = check('maType 99', () => ma(close, { maType: bad }), () => js.ma(close, { maType: bad }));
  assert.equal(r.fellThrough, 1);
  assert.ok(r.got.error instanceof TALibError);
});

test('an element whose valueOf grows WASM memory by a re-entrant call gives the ta-lib/js result', () => {
  check('warm-up', () => ema(close), () => js.ema(close));
  let before = hooks.wasmMemoryBytes();
  assert.ok(before > 0, 'no live instance after a routed call');
  // Each size outgrows the scratch the previous ones left, so every re-entrant call moves it.
  for (const size of [1_000_000, 2_000_000, 4_000_000, 8_000_000]) {
    const where = `valueOf grows memory by a ${size}-bar call`;
    const big = new Float64Array(size).fill(1);
    let grown = 0;
    const list: unknown[] = Array.from(close);
    list[100] = {
      valueOf(): number {
        if (grown === 0) {
          ema(big, { timePeriod: 3 });
          grown = hooks.wasmMemoryBytes();
        }
        return close[100];
      },
    };
    const series = list as number[];
    const r = check(where, () => ema(series), () => js.ema(series));
    assert.ok(grown > before, `${where}: memory ${before} -> ${grown}: the re-entrant call did not grow it`);
    assert.equal(r.fellThrough, 0, `${where}: fell through as '${hooks.lastFallThrough()}'`);
    assert.equal(r.ran, 2, `${where}: the module did not count the re-entrant call and the outer one`);
    before = grown;
  }
});

test('a declined MA type or a call over the budget, routed by the table, converts no input and never enters the module', () => {
  let calls = 0;
  const counted = (): number[] =>
    Array.from(close, (v) => ({
      valueOf(): number {
        calls++;
        return v;
      },
    })) as unknown as number[];
  const count = (call: () => unknown): { got: Outcome; calls: number } => {
    calls = 0;
    const got = outcome(call);
    return { got, calls };
  };
  const previous = hooks.setBudget(hooks.BUDGET);
  try {
    for (const [where, reason, budget, options] of [
      ['maType MAMA', 'declined', hooks.BUDGET, { maType: MAType.MAMA }],
      ['one byte over the budget', 'budget', N * 16 - 1, {}],
    ] as const) {
      hooks.setBudget(budget);
      const fell = hooks.fallThroughCount();
      const entered = hooks.moduleCalls();
      const mine = count(() => routed.apo(counted(), options));
      const ran = hooks.moduleCalls() - entered;
      const fellThrough = hooks.fallThroughCount() - fell;
      hooks.setBudget(hooks.BUDGET);
      const theirs = count(() => js.apo(counted(), options));
      same(mine.got, theirs.got, where);
      assert.ok(theirs.calls > 0, `${where}: ta-lib/js converted nothing`);
      assert.equal(mine.calls, theirs.calls, `${where}: valueOf ran ${mine.calls} times, ta-lib/js runs it ${theirs.calls} times`);
      assert.equal(ran, 0, `${where}: entered the module`);
      assert.equal(fellThrough, 1, where);
      assert.equal(hooks.lastFallThrough(), reason, where);
    }
  } finally {
    hooks.setBudget(previous);
  }
});

test('a Float64Array of another realm is converted as ta-lib/js converts it, and runs in WASM with its bits, in both call forms', () => {
  const foreign = vm.runInNewContext('new Float64Array(n)', { n: N }) as Float64Array;
  foreign.set(close);
  assert.ok(!(foreign instanceof Float64Array), 'the array is of this realm');
  ranInWasm(check('options', () => ema(foreign, { timePeriod: 10 }), () => js.ema(foreign, { timePeriod: 10 })), 'options');
  const mine = new Float64Array(N).fill(-1);
  const theirs = new Float64Array(N).fill(-1);
  const r = check('positional', () => routedEma.emaPositional(0, N - 1, foreign, 10, mine), () => jsEma.emaPositional(0, N - 1, foreign, 10, theirs));
  ranInWasm(r, 'positional');
  assert.ok(bits(mine).equals(bits(theirs)), 'positional: the buffers differ');
});

test('a real parameter that is not a number, an integer or range bound that is not an int32, and a non-object prices fall through', () => {
  const cases: [string, string, () => unknown, () => unknown][] = [
    ["t3 vFactor '0.7'", 'parameter', () => routed.t3(close, { vFactor: '0.7' as never }), () => js.t3(close, { vFactor: '0.7' as never })],
    ['t3 vFactor true', 'parameter', () => routed.t3(close, { vFactor: true as never }), () => js.t3(close, { vFactor: true as never })],
    ["sar acceleration '0.02'", 'parameter', () => routed.sar({ high: close, low: close }, { acceleration: '0.02' as never }), () => js.sar({ high: close, low: close }, { acceleration: '0.02' as never })],
    ['timePeriod 2**32 + 5', 'parameter', () => ema(close, { timePeriod: 2 ** 32 + 5 }), () => js.ema(close, { timePeriod: 2 ** 32 + 5 })],
    ['startIdx 2.5', 'options', () => ema(close, { startIdx: 2.5 }), () => js.ema(close, { startIdx: 2.5 })],
    ['prices 42', 'input', () => routed.atr(42 as never), () => js.atr(42 as never)],
  ];
  for (const [where, reason, viaDefault, viaJs] of cases) {
    const r = check(where, viaDefault, viaJs);
    assert.equal(r.fellThrough, 1, where);
    assert.equal(hooks.lastFallThrough(), reason, where);
    assert.equal(r.ran, 0, where);
  }
});

test('an element whose conversion throws falls through and throws the ta-lib/js error; a Float64Array view at an offset runs in WASM', () => {
  const big: unknown[] = Array.from(close);
  big[5] = 5n;
  const thrower: unknown[] = Array.from(close);
  thrower[7] = {
    valueOf(): number {
      throw new RangeError('valueOf refused');
    },
  };
  for (const [where, list] of [['a BigInt element', big], ['a throwing valueOf', thrower]] as const) {
    const series = list as number[];
    const r = check(where, () => ema(series), () => js.ema(series));
    assert.ok(r.got.error !== undefined, `${where}: ta-lib/js accepted it`);
    assert.equal(r.fellThrough, 1, where);
    assert.equal(hooks.lastFallThrough(), 'input', where);
  }
  const wide = new Float64Array(N + 16);
  wide.set(close, 8);
  const view = wide.subarray(8, 8 + N);
  assert.equal(view.byteOffset, 64);
  ranInWasm(check('subarray at byte 64', () => ema(view, { timePeriod: 9 }), () => js.ema(view, { timePeriod: 9 })), 'subarray at byte 64');
});

test('a call over the memory budget falls through as budget, and one at or under it runs in WASM', () => {
  const bytes = N * 16;
  const previous = hooks.setBudget(bytes + 1);
  try {
    assert.equal(previous, hooks.BUDGET, 'the default budget is not BUDGET');
    ranInWasm(check('one byte under', () => ema(close), () => js.ema(close)), 'one byte under');
    hooks.setBudget(bytes);
    ranInWasm(check('at the budget', () => ema(close), () => js.ema(close)), 'at the budget');
    hooks.setBudget(bytes - 1);
    const over = check('one byte over', () => ema(close), () => js.ema(close));
    assert.equal(over.fellThrough, 1, 'one byte over: did not fall through');
    assert.equal(hooks.lastFallThrough(), 'budget', 'one byte over');
    assert.equal(over.ran, 0, 'one byte over: entered the module');
  } finally {
    hooks.setBudget(previous);
  }
});

test('an export refuses a slice outside its frame as a frame defect, and a misaligned or overlapping one as declined', () => {
  const n = 64;
  // A scratch larger than the frame: the frame is what was asked for, not what the buffer holds.
  assert.ok(hooks.engineFor(n * 64) && hooks.wasmFrame(n * 64) !== null, 'no engine');
  const w = hooks.wasmFrame(n * 16);
  assert.ok(w !== null, 'no engine');
  const at = w.base;
  w.f64.set(close.subarray(0, n), at / 8);
  const batch = (inAt: number, outAt: number, outLen = n): number => w.x.ema_batch(0, n - 1, inAt, n, 10, outAt, outLen);
  const reason = (code: number): string => {
    assert.ok(code < 0, `the export answered ${code}`);
    hooks.refused(code);
    return hooks.lastFallThrough();
  };
  assert.ok(batch(at, at + n * 8) > 0, 'the export refused a call inside its frame');
  assert.equal(reason(batch(at, at + n * 8 + 8)), 'frame', 'an output 8 bytes past the frame');
  assert.equal(reason(batch(at - 8, at + n * 8)), 'frame', 'an input 8 bytes before the frame');
  assert.equal(reason(batch(at + n * 16 + 8, at + n * 8)), 'frame', 'an input that starts past the frame');
  assert.equal(reason(batch(at, at + n * 8 + 4, n - 1)), 'declined', 'a misaligned output');
  assert.equal(reason(batch(at, at + 8 * 8)), 'declined', 'an output overlapping the input');
});

test('an export that grows linear memory itself answers with the range and bits of ta-lib/js', () => {
  hooks.resetEngine();
  hooks.provideWasmBytes(WASM_BASE64);
  const n = 2_000_000;
  const series = Float64Array.from({ length: n }, (_, i) => Math.round((100 + 10 * Math.sin(i / 9) + (i % 97) / 10) * 100) / 100);
  assert.ok(hooks.engineFor(n * 24) && hooks.wasmFrame(n * 24) !== null, 'no engine');
  const before = hooks.wasmMemoryBytes();
  const entered = hooks.moduleCalls();
  hooks.setEngineMode('strict');
  let got: Outcome;
  try {
    got = outcome(() => routed.stochrsi(series));
  } finally {
    hooks.setEngineMode('table');
  }
  const after = hooks.wasmMemoryBytes();
  same(got, outcome(() => js.stochrsi(series)), 'stochrsi over 2,000,000 bars');
  assert.equal(hooks.moduleCalls() - entered, 1);
  assert.ok(after > before, `memory ${before} -> ${after}: the export did not grow it beyond its frame`);
});

test('the routed path accepts exactly the options objects ta-lib/js accepts', () => {
  class Options {
    timePeriod = 9;
  }
  const shapes: [string, unknown][] = [
    ['plain object', { timePeriod: 9 }],
    ['null-prototype object', Object.assign(Object.create(null) as object, { timePeriod: 9 })],
    ['class instance', new Options()],
    ['frozen object', Object.freeze({ timePeriod: 9 })],
    ['array', [9]],
    ['typed array', new Float64Array(3)],
    ['null', null],
    ['function', () => 9],
    ['Proxy', new Proxy({ timePeriod: 9 }, {})],
  ];
  let accepted = 0;
  for (const [where, options] of shapes) {
    const opts = options as Parameters<typeof ema>[1];
    const r = check(where, () => ema(close, opts), () => js.ema(close, opts));
    const jsAccepts = r.got.error === undefined;
    assert.equal(hooks.routedOptions(opts) !== null, jsAccepts, `${where}: the routed predicate disagrees with ta-lib/js`);
    if (jsAccepts) ranInWasm(r, where);
    else assert.equal(r.fellThrough, 1, where);
    if (jsAccepts) accepted++;
  }
  assert.equal(accepted, 5);
});

test('an instance past the high-water mark is dropped after its call, and the next call still answers in WASM', () => {
  ranInWasm(check('warm-up', () => ema(close), () => js.ema(close)), 'warm-up');
  assert.ok(hooks.wasmMemoryBytes() > 0);
  const previous = hooks.setHighWater(0);
  try {
    for (const where of ['past the mark', 'after the reset']) {
      ranInWasm(check(where, () => ema(close, { timePeriod: 12 }), () => js.ema(close, { timePeriod: 12 })), where);
      assert.equal(hooks.wasmMemoryBytes(), 0, `${where}: the instance was kept`);
    }
  } finally {
    hooks.setHighWater(previous);
  }
  ranInWasm(check('below the mark', () => ema(close), () => js.ema(close)), 'below the mark');
  assert.ok(hooks.wasmMemoryBytes() > 0);
  assert.deepEqual(hooks.engineStatus(), { engine: 'wasm', reason: null });
});

test('the two entries export the same names, and only the default entry has ready', async () => {
  const names = Object.keys(js).sort();
  assert.deepEqual(Object.keys(routed).sort(), [...names, 'ready'].sort());
  assert.ok(!names.includes('ready'));
  assert.deepEqual(await ready(), { engine: 'wasm', reason: null });
});
