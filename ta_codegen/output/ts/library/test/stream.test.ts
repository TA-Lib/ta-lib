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
 *  110126 MF,CC  First Version
 */

// Every streamable function, on seeded series: Open then update replays the
// batch output bit for bit, and so do OpenAndFill, a clone, and a peek ahead
// of each update. The signatures come from the generated modules themselves.

import assert from 'node:assert/strict';
import { test } from 'node:test';

import { type Arr, L, N, type Range, r2, type Spec, series, specs } from './corpus.ts';
import * as lib from '../src/index.ts';
import { Core, MAType, TALibError } from '../src/index.ts';

type Stream = {
  update(...args: unknown[]): unknown;
  peek(...args: unknown[]): unknown;
  value(...args: unknown[]): unknown;
  outRange(): Range;
  advance(): void;
  clone(): Stream;
};

/** Parameter vectors to try: the defaults, small periods, and each MA type in turn. */
function variants(spec: Spec): number[][] {
  const opts = spec.params.filter((p) => p.kind === 'int' || p.kind === 'real' || p.kind === 'enum');
  const dflt: number[] = opts.map((p) => (p.kind === 'real' ? Core.REAL_DEFAULT : Core.INTEGER_DEFAULT));
  const vs: number[][] = [dflt];
  for (const period of [1, 2, 7]) {
    if (opts.some((p) => p.kind === 'int')) vs.push(opts.map((p, k) => (p.kind === 'int' ? period : dflt[k])));
  }
  opts.forEach((p, k) => {
    if (p.kind !== 'enum') return;
    for (const t of Object.values(MAType)) {
      const v = dflt.slice();
      v[k] = t;
      vs.push(v);
    }
  });
  return vs;
}

function same(a: number, b: number): boolean {
  return Object.is(a, b);
}

type Case = {
  spec: Spec;
  opts: number[];
  ins: Float64Array[];
  outs: Arr[];
  range: Range;
};

/** The batch result over the whole series, or null for a vector the batch rejects. */
function batchCase(spec: Spec, data: Record<string, Float64Array>, opts: number[]): Case | null {
  const ins = spec.params.filter((p) => p.kind === 'input').map((p) => data[p.name]);
  const outs = spec.params.filter((p) => p.kind === 'output').map((p) => (p.int32 ? new Int32Array(N) : new Float64Array(N)));
  let range: Range;
  try {
    range = spec.batch(0, N - 1, ...ins, ...opts, ...outs) as Range;
  } catch (e) {
    if (e instanceof TALibError && e.code === 'TA_BAD_PARAM') return null;
    throw e;
  }
  if (range.count < 4) return null;
  return { spec, opts, ins, outs, range };
}

/**
 * The stream's current outputs, in batch output order. A multi-output peek or
 * value reads into a given sink, which it must return, and into none, whose new
 * one must hold the same bits; an update, which cannot run twice, takes a sink
 * on even bars and none on odd ones.
 */
function read(c: Case, s: Stream, verb: 'update' | 'peek' | 'value', t: number): number[] {
  const bars = verb === 'value' ? [] : c.ins.map((a) => a[t]);
  if (c.spec.outClass === null) return [s[verb](...bars) as number];
  const Out = L[c.spec.outClass] as new () => Record<string, number>;
  const keys = Object.keys(new Out());
  const fresh = (): number[] => {
    const r = s[verb](...bars) as Record<string, number>;
    assert.ok(r instanceof Out, `${c.spec.name} ${verb}: no sink, not a ${c.spec.outClass}`);
    assert.deepEqual(Object.keys(r), keys, `${c.spec.name} ${verb}: the new sink's fields`);
    return keys.map((k) => r[k]);
  };
  if (verb === 'update' && t % 2 === 1) return fresh();
  const sink = new Out();
  assert.equal(s[verb](...bars, sink), sink, `${c.spec.name} ${verb}: the given sink is not returned`);
  const got = keys.map((k) => sink[k]);
  if (verb !== 'update') {
    const again = fresh();
    assert.ok(got.every((x, j) => Object.is(x, again[j])), `${c.spec.name} ${verb}: ${again} without a sink, ${got} with one`);
  }
  return got;
}

function expectBatch(c: Case, got: number[], t: number, what: string): void {
  const k = t - c.range.begIdx;
  got.forEach((g, j) => {
    const want = c.outs[j][k];
    assert.ok(same(g, want), `${c.spec.name}(${c.opts.join(',')}) ${what} at bar ${t} output ${j}: ${g} vs batch ${want}`);
  });
}

function sameRange(r: Range, begIdx: number, count: number, what: string): void {
  assert.equal(r.begIdx, begIdx, `${what}: outRange begIdx`);
  assert.equal(r.count, count, `${what}: outRange count`);
}

function replay(c: Case): void {
  const { spec, opts, ins, range } = c;
  const lb = range.begIdx;
  const h = Math.min(N, Math.max(lb + 1, N >> 1));
  const history = ins.map((a) => a.slice(0, h));
  const s = spec.open(...history, ...opts) as Stream;
  sameRange(s.outRange(), lb, h - lb, `${spec.name} open`);
  expectBatch(c, read(c, s, 'value', h - 1), h - 1, 'value after open');

  const fromArrays = spec.open(...history.map((a) => Array.from(a)), ...opts) as Stream;
  expectBatch(c, read(c, fromArrays, 'value', h - 1), h - 1, 'value after an open from number[]');

  const mid = h + ((N - h) >> 1);
  let forkA: Stream | null = null;
  let forkB: Stream | null = null;
  for (let t = h; t < N; t++) {
    if (t === mid) {
      forkA = s.clone();
      forkB = s.clone();
      for (let k = 0; k < 8 && mid + k < N; k++) {
        forkB.update(...ins.map((a) => r2(a[mid + k] * 1.07 + 0.5)), ...(spec.outClass ? [new (L[spec.outClass] as new () => object)()] : []));
      }
      const skip = s.clone();
      const before = read(c, skip, 'value', t - 1);
      skip.advance();
      sameRange(skip.outRange(), lb, t - lb + 1, `${spec.name} advance`);
      assert.deepEqual(read(c, skip, 'value', t - 1), before, `${spec.name}: advance moved value()`);
    }
    const peeked = read(c, s, 'peek', t);
    expectBatch(c, read(c, s, 'value', t - 1), t - 1, 'value after a peek');
    const committed = read(c, s, 'update', t);
    expectBatch(c, committed, t, 'update after a peek');
    expectBatch(c, peeked, t, 'peek');
    sameRange(s.outRange(), lb, t - lb + 1, `${spec.name} update`);
  }
  if (forkA !== null) {
    for (let t = mid; t < N; t++) expectBatch(c, read(c, forkA, 'update', t), t, 'clone update');
  }

  const fillOuts = c.outs.map((o) => (o instanceof Int32Array ? new Int32Array(N - lb) : new Float64Array(N - lb)));
  const f = spec.fill(...ins, ...opts, ...fillOuts) as Stream;
  sameRange(f.outRange(), range.begIdx, range.count, `${spec.name} openAndFill`);
  fillOuts.forEach((o, j) => {
    for (let k = 0; k < range.count; k++) {
      assert.ok(same(o[k], c.outs[j][k]), `${spec.name}(${opts.join(',')}) openAndFill output ${j} at ${k}: ${o[k]} vs ${c.outs[j][k]}`);
    }
  });
  expectBatch(c, read(c, f, 'value', N - 1), N - 1, 'value after openAndFill');
}

const SPECS = specs();

test('every function module exports a stream', () => {
  assert.ok(SPECS.length >= 200, `only ${SPECS.length} streams found`);
});

for (const seed of [1, 2, 3]) {
  test(`open, update, peek, clone and fill replay batch bitwise (seed ${seed})`, () => {
    const data = series(seed);
    let cases = 0;
    const covered = new Set<string>();
    for (const spec of SPECS) {
      for (const opts of variants(spec)) {
        const c = batchCase(spec, data, opts);
        if (c === null) continue;
        try {
          replay(c);
        } catch (e) {
          // An MA type with no streaming arm is refused at open.
          if (e instanceof TALibError && e.code === 'TA_BAD_PARAM' && spec.params.some((p) => p.kind === 'enum')) continue;
          throw e;
        }
        cases++;
        covered.add(spec.name);
      }
    }
    assert.equal(covered.size, SPECS.length, `uncovered: ${SPECS.map((s) => s.name).filter((n) => !covered.has(n)).join(' ')}`);
    assert.ok(cases > SPECS.length, `only ${cases} cases`);
  });
}

function assertBadParam(fn: () => unknown): void {
  assert.throws(fn, (e: unknown) => e instanceof TALibError && e.code === 'TA_BAD_PARAM');
}

test('OpenAndFill refuses an output sharing memory with an input or another output', () => {
  const data = series(4);
  const buf = new ArrayBuffer(8 * 3 * N);
  const input = new Float64Array(buf, 0, N);
  input.set(data.inClose);
  const lb = lib.smaLookback({ timePeriod: 10 });
  const n = N - lb;
  assertBadParam(() => lib.smaOpenAndFill(input, { timePeriod: 10, outReal: input }));
  assertBadParam(() => lib.smaOpenAndFill(input, { timePeriod: 10, outReal: new Float64Array(buf, 8 * (N - 1), n) }));
  assertBadParam(() => lib.smaOpenAndFill(input, { timePeriod: 10, outReal: new Float64Array(buf, 0, n) }));
  const apart = new Float64Array(buf, 8 * N, n);
  const s = lib.smaOpenAndFill(input, { timePeriod: 10, outReal: apart });
  const want = new Float64Array(n);
  lib.sma(data.inClose, { timePeriod: 10, outReal: want });
  assert.ok(apart.every((x, k) => same(x, want[k])), 'a disjoint view of the same buffer fills like a fresh array');
  assert.equal(s.stream.outRange().count, n);
  assert.ok(s.values.buffer === buf && s.values.byteOffset === apart.byteOffset && s.values.length === n, 'the fill returns a view of its buffer');

  const macdOpts = { fastPeriod: 12, slowPeriod: 26, signalPeriod: 9 };
  const m = N - lib.macdLookback(macdOpts);
  const shared = new Float64Array(buf, 8 * N, 2 * N);
  assertBadParam(() => lib.macdOpenAndFill(data.inClose, { ...macdOpts, outMacd: shared.subarray(0, m), outMacdSignal: shared.subarray(m - 1, 2 * m - 1), outMacdHist: new Float64Array(m) }));
  lib.macdOpenAndFill(data.inClose, { ...macdOpts, outMacd: shared.subarray(0, m), outMacdSignal: shared.subarray(m, 2 * m), outMacdHist: new Float64Array(m) });

  const ohlc = new Float64Array(buf, 0, N);
  ohlc.set(data.inOpen);
  const flags = new Int32Array(buf, 0, N);
  assertBadParam(() => lib.cdldojiOpenAndFill({ open: ohlc, high: data.inHigh, low: data.inLow, close: data.inClose }, { outInteger: flags }));
});

test('a rejected bar moves nothing, and the ceiling and history rules answer first', () => {
  const data = series(5);
  const s = lib.smaOpen(data.inClose, { timePeriod: 10 });
  const before = s.value();
  const range = s.outRange();
  assertBadParam(() => s.update(Number.NaN));
  assertBadParam(() => s.peek(Number.POSITIVE_INFINITY));
  assert.ok(same(s.value(), before));
  assert.deepEqual(s.outRange(), range);
  assert.throws(() => lib.smaOpen(new Float64Array(0), { timePeriod: 10 }), (e: unknown) => e instanceof TALibError && e.code === 'TA_INSUFFICIENT_HISTORY');
  assert.throws(() => lib.smaOpen(data.inClose.slice(0, 5), { timePeriod: 10 }), (e: unknown) => e instanceof TALibError && e.code === 'TA_INSUFFICIENT_HISTORY');
  assertBadParam(() => lib.smaOpen(data.inClose, { timePeriod: 1.5 }));
  assertBadParam(() => lib.smaOpen(data.inClose, { timePeriod: 0 }));
});

test('MAVP clamps a period too large for an int before it narrows, in every tier', () => {
  const data = series(6);
  const periods = data.inPeriods.map((_, i) => (i % 3 === 0 ? 1e18 : i % 3 === 1 ? 2147483648 : 3e9));
  const opts = { minPeriod: 2, maxPeriod: 30, maType: MAType.SMA };
  const lb = lib.mavpLookback(opts);
  const want = new Float64Array(N - lb);
  lib.mavp(data.inClose, periods, { ...opts, outReal: want });
  const filled = new Float64Array(N - lb);
  lib.mavpOpenAndFill(data.inClose, periods, { ...opts, outReal: filled });
  assert.ok(filled.every((x, k) => same(x, want[k])), 'openAndFill');
  const h = N >> 1;
  const s = lib.mavpOpen(data.inClose.slice(0, h), periods.slice(0, h), opts);
  for (let t = h; t < N; t++) {
    const p = s.peek(data.inClose[t], periods[t]);
    const u = s.update(data.inClose[t], periods[t]);
    assert.ok(same(p, want[t - lb]) && same(u, want[t - lb]), `bar ${t}: ${p}/${u} vs ${want[t - lb]}`);
  }
});

test('a multi-output stream refuses a sink that is not an object before anything moves, returns a new one when none is given, and takes a structural one', () => {
  const data = series(7);
  const h = N >> 1;
  let checked = 0;
  let overwritten = 0;
  for (const spec of SPECS) {
    if (spec.outClass === null) continue;
    const Out = L[spec.outClass] as new () => Record<string, number>;
    const keys = Object.keys(new Out());
    assert.ok(keys.length > 1 && keys.every((k) => k.startsWith('out') && Number.isNaN(new Out()[k])), `${spec.name}: a new sink's fields ${keys}`);
    const ins = spec.params.filter((p) => p.kind === 'input').map((p) => data[p.name]);
    const opts = spec.params.filter((p) => p.kind !== 'input' && p.kind !== 'output').map((p) => (p.kind === 'real' ? Core.REAL_DEFAULT : Core.INTEGER_DEFAULT));
    const s = spec.open(...ins.map((a) => a.slice(0, h)), ...opts) as Stream;
    const control = s.clone();
    const range = s.outRange();
    const bars = ins.map((a) => a[h]);
    for (const bad of [null, 0]) {
      assertBadParam(() => s.update(...bars, bad));
      assertBadParam(() => s.peek(...bars, bad));
      assertBadParam(() => s.value(bad));
    }
    assert.deepEqual(s.outRange(), range, `${spec.name}: a refused update counted the bar`);
    const want = new Out();
    assert.equal(control.peek(...bars, want), want, `${spec.name}: peek returns its sink`);
    for (const [verb, got] of [['peek', s.peek(...bars)], ['peek undefined', s.peek(...bars, undefined)], ['value', s.value()]] as const) {
      const ref = verb === 'value' ? control.value(new Out()) : want;
      assert.ok(got instanceof Out, `${spec.name} ${verb}: no sink, not a ${spec.outClass}`);
      for (const k of keys) assert.ok(same((got as Record<string, number>)[k], (ref as Record<string, number>)[k]), `${spec.name}.${k} ${verb}`);
    }
    assert.deepEqual(s.outRange(), range, `${spec.name}: a peek without a sink counted the bar`);
    const twin = s.clone();
    const got: Record<string, number> = Object.fromEntries(keys.map((k) => [k, 0]));
    assert.equal(s.update(...bars, got), got, `${spec.name}: update returns its sink`);
    assert.equal(control.update(...bars, want), want, `${spec.name}: update returns its sink`);
    const fresh = twin.update(...bars) as Record<string, number>;
    assert.ok(fresh instanceof Out, `${spec.name}: update without a sink`);
    assert.deepEqual(s.outRange(), control.outRange(), spec.name);
    assert.deepEqual(twin.outRange(), control.outRange(), `${spec.name}: update without a sink counts the bar`);
    for (const k of keys) {
      assert.ok(same(got[k], want[k]), `${spec.name}.${k}: ${got[k]} after a refused update vs ${want[k]}`);
      assert.ok(same(fresh[k], want[k]), `${spec.name}.${k}: ${fresh[k]} without a sink vs ${want[k]}`);
    }
    // A peek into the sink the update filled overwrites every field.
    const next = ins.map((a) => a[h + 1]);
    assert.equal(s.peek(...next, got), got, `${spec.name}: peek returns the update's sink`);
    const ahead = control.peek(...next) as Record<string, number>;
    for (const k of keys) assert.ok(same(got[k], ahead[k]), `${spec.name}.${k}: ${got[k]} peeked into the update's sink vs ${ahead[k]}`);
    if (keys.some((k) => !same(ahead[k], want[k]))) overwritten++;
    checked++;
  }
  assert.ok(checked > 0);
  assert.ok(overwritten > checked / 2, `the next bar's peek changed the sink for only ${overwritten} of ${checked} functions`);
});
