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
 *  110226 MF,CC  First Version
 */

// The streaming entry points' argument rules, which no server reaches: every
// server opens on a full, correctly sized history and feeds finite bars.

import assert from 'node:assert/strict';
import { test } from 'node:test';

import { type Arr, L, N, type Param, type Range, series } from './corpus.ts';
import { bytes, filled, type Named, namedSpecs, optsOf, outOfDomain, outputsOf, rejects, untouched } from './contract.ts';
import * as lib from '../src/index.ts';
import { Core, MAType } from '../src/index.ts';
import { FuncFlags, Functions } from '../src/metadata/index.ts';

type Stream = {
  update(...args: unknown[]): unknown;
  peek(...args: unknown[]): unknown;
  value(...args: unknown[]): unknown;
  outRange(): Range;
  advance(): void;
  clone(): Stream;
};

const SPECS = namedSpecs();
const inputsOf = (s: Named): Param[] => s.params.filter((p) => p.kind === 'input');
const dflt = (s: Named): number[] => optsOf(s).map((p) => (p.kind === 'real' ? Core.REAL_DEFAULT : Core.INTEGER_DEFAULT));

/** The parameter vectors the fill bound is checked at: the defaults, and MA's identity arm and a sub-stream arm. */
function variants(s: Named): number[][] {
  return s.fn === 'MA' ? [dflt(s), [1, MAType.EMA], [30, MAType.EMA]] : [dflt(s)];
}

function history(s: Named, data: Record<string, Float64Array>, n: number): Float64Array[] {
  return inputsOf(s).map((p) => data[p.name].subarray(0, n));
}

/** The stream's outputs after `verb`, in batch output order. */
function read(s: Named, h: Stream, verb: 'update' | 'peek' | 'value', bars: number[]): number[] {
  if (s.outClass === null) return [h[verb](...bars) as number];
  const sink = new (L[s.outClass] as new () => Record<string, number>)();
  h[verb](...bars, sink);
  return Object.keys(sink).map((k) => sink[k]);
}

function sameBits(a: number[], b: number[]): boolean {
  return a.length === b.length && a.every((x, i) => Object.is(x, b[i]));
}

test('every streaming registry function has an opener, and the sweeps below cover each one', () => {
  const streaming = Functions.all().filter((f) => f.hasFlags(FuncFlags.STREAMING));
  assert.equal(SPECS.length, streaming.length);
  assert.deepEqual(SPECS.map((s) => s.fn).sort(), streaming.map((f) => f.name).sort());
  for (const f of Functions.all()) {
    if (f.hasFlags(FuncFlags.STREAMING)) continue;
    const stem = SPECS.find((s) => s.fn === f.name)?.name ?? f.name.replace(/_/g, '').toLowerCase();
    assert.equal(L[`${stem}Open`], undefined, `${f.name}: batch-only by its flags, yet it has an opener`);
  }
});

test('an opener names its own function: one bar is too short for every function with a lookback', () => {
  const data = series(41);
  let own = 0;
  let opened = 0;
  for (const s of SPECS) {
    const opts = dflt(s);
    const lb = s.lookback(...opts) as number;
    const one = history(s, data, 1);
    const outs = outputsOf(s).map((o) => filled(o, 1));
    if (lb > 0) {
      rejects(() => s.open(...one, ...opts), 'TA_INSUFFICIENT_HISTORY', `${s.fn} open`, `${s.fn} open: history has length 1, needs ${lb + 1}`);
      rejects(() => s.fill(...one, ...opts, ...outs), 'TA_INSUFFICIENT_HISTORY', `${s.fn} openAndFill`, `${s.fn} openAndFill: history has length 1, needs ${lb + 1}`);
      own++;
    } else {
      assert.deepEqual((s.open(...one, ...opts) as Stream).outRange(), new lib.OutRange(0, 1), `${s.fn} open on one bar`);
      assert.deepEqual((s.fill(...one, ...opts, ...outs) as Stream).outRange(), new lib.OutRange(0, 1), `${s.fn} openAndFill on one bar`);
      opened++;
    }
  }
  assert.equal(own + opened, SPECS.length);
  assert.ok(own > 0 && opened > 0, `${own} rejecting, ${opened} zero-lookback`);
});

test('openAndFill refuses an absent output naming it, and an empty one even on a history too short to open', () => {
  const data = series(42);
  let absent = 0;
  let empty = 0;
  for (const s of SPECS) {
    const opts = dflt(s);
    const lb = s.lookback(...opts) as number;
    const full = history(s, data, N);
    outputsOf(s).forEach((o, k) => {
      const outs = (n: number): unknown[] => outputsOf(s).map((p) => filled(p, n));
      for (const none of [null, undefined]) {
        const a = outs(N);
        a[k] = none;
        rejects(() => s.fill(...full, ...opts, ...a), 'TA_BAD_PARAM', `${s.fn}: ${o.name}=${none}`, `${s.fn} openAndFill: ${o.name} is absent`);
        absent++;
      }
      const a = outs(N);
      a[k] = filled(o, 0);
      rejects(() => s.fill(...full, ...opts, ...a), 'TA_BAD_PARAM', `${s.fn}: empty ${o.name}`, `${s.fn} openAndFill: ${o.name} has length 0, needs ${N - lb}`);
      empty++;
      if (lb === 0) return;
      const b = outs(1);
      b[k] = filled(o, 0);
      rejects(() => s.fill(...history(s, data, lb), ...opts, ...b), 'TA_BAD_PARAM', `${s.fn}: empty ${o.name}, short history`, `${s.fn} openAndFill: ${o.name} is empty`);
      empty++;
    });
    if (lb > 0) {
      rejects(() => s.fill(...history(s, data, lb), ...opts, ...outputsOf(s).map((p) => filled(p, 1))), 'TA_INSUFFICIENT_HISTORY', `${s.fn}: one element each`);
    }
  }
  assert.equal(absent, 2 * SPECS.reduce((n, s) => n + outputsOf(s).length, 0));
  assert.ok(empty > SPECS.length, `only ${empty} empty outputs`);
});

/** A history claiming `n` bars; only its length is ever read, because the opener refuses it first. */
function claiming(n: number): ArrayLike<number> {
  return { length: n } as ArrayLike<number>;
}

test('an empty history is an index fault ahead of every presence check, and one past INDEX_MAX + 1 bars is the end-index fault', () => {
  let probes = 0;
  let legs = 0;
  let inclusive = 0;
  for (const s of SPECS) {
    const opts = dflt(s);
    const ins = inputsOf(s);
    const none = outputsOf(s).map(() => null);
    const empty = ins.map(() => new Float64Array(0));
    rejects(() => s.open(...empty, ...opts), 'TA_OUT_OF_RANGE_START_INDEX', `${s.fn} open: empty`, `${s.fn} open: startIdx out of range`);
    rejects(() => s.fill(...empty, ...opts, ...none), 'TA_OUT_OF_RANGE_START_INDEX', `${s.fn} openAndFill: empty, no outputs`, `${s.fn} openAndFill: startIdx out of range`);
    if (ins.length > 1) {
      const firstOnly = ins.map((_, k) => (k === 0 ? new Float64Array(0) : null));
      rejects(() => s.open(...firstOnly, ...opts), 'TA_OUT_OF_RANGE_START_INDEX', `${s.fn} open: empty history, absent legs`);
      rejects(() => s.fill(...firstOnly, ...opts, ...none), 'TA_OUT_OF_RANGE_START_INDEX', `${s.fn} openAndFill: empty history, absent legs`);
      legs++;
    }
    const past = ins.map(() => claiming(Core.INDEX_MAX + 2));
    rejects(() => s.open(...past, ...opts), 'TA_OUT_OF_RANGE_END_INDEX', `${s.fn} open: past INDEX_MAX + 1`, `${s.fn} open: endIdx out of range`);
    rejects(() => s.fill(...past, ...opts, ...none), 'TA_OUT_OF_RANGE_END_INDEX', `${s.fn} openAndFill: past INDEX_MAX + 1`, `${s.fn} openAndFill: endIdx out of range`);
    probes++;

    if (ins.length !== 1) continue;
    // INDEX_MAX + 1 bars is the largest history: report that length to the range check, then one bar to the copy.
    let reads = 0;
    const edge = { 0: 100, get length() { return reads++ === 0 ? Core.INDEX_MAX + 1 : 1; } } as ArrayLike<number>;
    try {
      s.open(edge, ...opts);
    } catch (e) {
      assert.ok(e instanceof lib.TALibError && e.code === 'TA_INSUFFICIENT_HISTORY', `${s.fn} open: INDEX_MAX + 1 bars refused: ${String(e)}`);
    }
    assert.ok(reads >= 2, `${s.fn} open: the length was read ${reads} time(s)`);
    inclusive++;
  }
  assert.equal(probes, SPECS.length);
  assert.equal(legs + inclusive, SPECS.length);
  assert.ok(legs > 0 && inclusive > 0, `${legs} multi-leg, ${inclusive} single-input`);
});

test('every history leg must be exactly the history length', () => {
  const data = series(43);
  const h = N - 1;
  let probes = 0;
  for (const s of SPECS) {
    const opts = dflt(s);
    const ins = inputsOf(s);
    ins.forEach((p, k) => {
      if (k === 0) return;
      for (const len of [h - 1, h + 1]) {
        const legs = ins.map((q, j) => data[q.name].subarray(0, j === k ? len : h));
        const outs = outputsOf(s).map((o) => filled(o, N));
        rejects(() => s.open(...legs, ...opts), 'TA_BAD_PARAM', `${s.fn} open: ${p.name} of ${len}`, `${s.fn} open: ${p.name} has length ${len}, needs ${h}`);
        rejects(() => s.fill(...legs, ...opts, ...outs), 'TA_BAD_PARAM', `${s.fn} openAndFill: ${p.name} of ${len}`, `${s.fn} openAndFill: ${p.name} has length ${len}, needs ${h}`);
        for (const o of outs) assert.ok(untouched(o), `${s.fn} openAndFill: a rejected leg wrote an output`);
        probes++;
      }
    });
  }
  assert.equal(probes, 2 * SPECS.reduce((n, s) => n + Math.max(0, inputsOf(s).length - 1), 0));
  assert.ok(probes > 100, `only ${probes} leg probes`);
});

test('an absent history or leg is TA_BAD_PARAM naming it at both openers, and a rejected fill writes nothing', () => {
  const data = series(45);
  let probes = 0;
  for (const s of SPECS) {
    const opts = dflt(s);
    const ins = inputsOf(s);
    ins.forEach((p, k) => {
      for (const none of [null, undefined]) {
        const legs: unknown[] = ins.map((q) => data[q.name].subarray(0, N));
        legs[k] = none;
        const outs = outputsOf(s).map((o) => filled(o, N));
        rejects(() => s.open(...legs, ...opts), 'TA_BAD_PARAM', `${s.fn} open: ${p.name}=${none}`, `${s.fn} open: ${p.name} is absent`);
        rejects(() => s.fill(...legs, ...opts, ...outs), 'TA_BAD_PARAM', `${s.fn} openAndFill: ${p.name}=${none}`, `${s.fn} openAndFill: ${p.name} is absent`);
        for (const o of outs) assert.ok(untouched(o), `${s.fn} openAndFill: an absent ${p.name} wrote an output`);
        probes++;
      }
    });
  }
  assert.equal(probes, 2 * SPECS.reduce((n, s) => n + inputsOf(s).length, 0));
  assert.ok(probes > 2 * SPECS.length, `only ${probes} presence probes`);
});

test('the fill output bound from both sides: exact accepted, one short refused writing nothing, roomy bit-identical', () => {
  const data = series(44);
  let exact = 0;
  let short = 0;
  for (const s of SPECS) {
    const ins = history(s, data, N);
    for (const opts of variants(s)) {
      const what = `${s.fn}(${opts.join(',')}) openAndFill`;
      const lb = s.lookback(...opts) as number;
      assert.ok(lb >= 0 && lb < N, `${what}: lookback ${lb}`);
      const produced = N - lb;
      const fit = outputsOf(s).map((o) => filled(o, produced));
      assert.deepEqual((s.fill(...ins, ...opts, ...fit) as Stream).outRange(), new lib.OutRange(lb, produced), `${what}: exact`);
      for (const o of fit) assert.ok(!untouched(o), `${what}: an exact fill left an output as filled`);
      exact++;
      outputsOf(s).forEach((o, k) => {
        const outs = outputsOf(s).map((p, j) => filled(p, j === k ? produced - 1 : produced));
        rejects(() => s.fill(...ins, ...opts, ...outs), 'TA_BAD_PARAM', `${what}: ${o.name} one short`, `${s.fn} openAndFill: ${o.name} has length ${produced - 1}, needs ${produced}`);
        for (const a of outs) assert.ok(untouched(a), `${what}: a rejected fill wrote an output`);
        short++;
      });
      const roomy = outputsOf(s).map((o) => filled(o, N));
      s.fill(...ins, ...opts, ...roomy);
      roomy.forEach((a: Arr, j) => {
        assert.deepEqual(bytes(a, produced), bytes(fit[j]), `${what}: roomy output ${j}`);
        assert.ok(untouched(a.subarray(produced)), `${what}: wrote past the produced count`);
      });
    }
  }
  assert.equal(exact, SPECS.length + 2);
  assert.equal(short, SPECS.reduce((n, s) => n + outputsOf(s).length * variants(s).length, 0));
});

test('a parameter out of its domain outranks an absent output at the opener', () => {
  const data = series(45);
  let probes = 0;
  for (const s of SPECS) {
    const ins = history(s, data, N);
    const none = outputsOf(s).map(() => null);
    optsOf(s).forEach((p, k) => {
      const opts = dflt(s);
      opts[k] = outOfDomain(s, k);
      const what = `${s.fn}: ${p.name}=${opts[k]}`;
      rejects(() => s.open(...ins, ...opts), 'TA_BAD_PARAM', `${what} open`, `${s.fn} open: bad parameter...`);
      rejects(() => s.fill(...ins, ...opts, ...none), 'TA_BAD_PARAM', `${what} openAndFill, no outputs`, `${s.fn} openAndFill: bad parameter...`);
      probes++;
    });
  }
  assert.equal(probes, SPECS.reduce((n, s) => n + optsOf(s).length, 0));
});

test('a non-finite real parameter is refused at every entry point, and its lookback and display shift reject it', () => {
  const data = series(46);
  let probes = 0;
  let realParams = 0;
  for (const s of SPECS) {
    const ins = history(s, data, N);
    const shift = L[`${s.name}DisplayShift`] as (...a: unknown[]) => number;
    optsOf(s).forEach((p, k) => {
      if (p.kind !== 'real') return;
      realParams++;
      for (const v of [Number.NaN, Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY]) {
        const opts = dflt(s);
        opts[k] = v;
        const what = `${s.fn}: ${p.name}=${v}`;
        const outs = (): Arr[] => outputsOf(s).map((o) => filled(o, N));
        const batchOuts = outs();
        rejects(() => s.batch(0, N - 1, ...ins, ...opts, ...batchOuts), 'TA_BAD_PARAM', `${what} batch`, `${s.fn}: bad parameter...`);
        for (const a of batchOuts) assert.ok(untouched(a), `${what}: a rejected batch call wrote`);
        rejects(() => s.open(...ins, ...opts), 'TA_BAD_PARAM', `${what} open`);
        rejects(() => s.fill(...ins, ...opts, ...outs()), 'TA_BAD_PARAM', `${what} openAndFill`);
        assert.equal(s.lookback(...opts), -1, `${what} lookback`);
        assert.equal(shift(...opts, 0), -2147483648, `${what} display shift`);
        probes++;
      }
    });
  }
  assert.equal(probes, 3 * realParams);
  assert.ok(realParams > 20, `only ${realParams} real parameters`);
});

test('a non-finite bar is refused naming its input and moves nothing; the next good bar counts once, and advance once', () => {
  const data = series(47);
  const h = N >> 1;
  let refused = 0;
  let resumed = 0;
  for (const s of SPECS) {
    const opts = dflt(s);
    const ins = inputsOf(s);
    const handle = s.open(...history(s, data, h), ...opts) as Stream;
    const control = handle.clone();
    const good = ins.map((p) => data[p.name][h]);
    const range = handle.outRange();
    const held = read(s, handle, 'value', []);
    ins.forEach((p, k) => {
      for (const v of [Number.NaN, Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY]) {
        const bars = good.map((g, j) => (j === k ? v : g));
        for (const verb of ['update', 'peek'] as const) {
          rejects(() => read(s, handle, verb, bars), 'TA_BAD_PARAM', `${s.fn} ${verb}: ${p.name}=${v}`, `${s.fn} ${verb}: ${p.name} is not finite`);
          assert.deepEqual(handle.outRange(), range, `${s.fn} ${verb}: a refused bar moved the range`);
          assert.ok(sameBits(read(s, handle, 'value', []), held), `${s.fn} ${verb}: a refused bar moved value()`);
          refused++;
        }
      }
    });
    const peeked = read(s, handle, 'peek', good);
    assert.deepEqual(handle.outRange(), range, `${s.fn}: a good peek moved the range`);
    assert.ok(sameBits(read(s, handle, 'value', []), held), `${s.fn}: a good peek moved value()`);
    const got = read(s, handle, 'update', good);
    const want = read(s, control, 'update', good);
    assert.deepEqual(handle.outRange(), new lib.OutRange(range.begIdx, range.count + 1), `${s.fn}: the good bar after the refusals`);
    assert.ok(sameBits(got, want) && sameBits(peeked, got), `${s.fn}: ${got} after the refusals, ${want} without them, ${peeked} peeked`);
    assert.ok(sameBits(read(s, handle, 'value', []), got), `${s.fn}: value() after the good bar`);
    handle.advance();
    assert.deepEqual(handle.outRange(), new lib.OutRange(range.begIdx, range.count + 2), `${s.fn}: advance`);
    assert.ok(sameBits(read(s, handle, 'value', []), got), `${s.fn}: advance moved value()`);
    resumed++;
  }
  assert.equal(refused, 6 * SPECS.reduce((n, s) => n + inputsOf(s).length, 0));
  assert.equal(resumed, SPECS.length);
});

test('the last bar a stream counts is INDEX_MAX: past it advance and update refuse with the end-index code, and peek still answers', () => {
  const data = series(48);
  const h = 60;
  const close = data.inClose.subarray(0, h);
  const periods = data.inPeriods.subarray(0, h);
  const cases: [string, Stream, number[]][] = [
    ['SMA', lib.smaOpen(close, 14) as unknown as Stream, [data.inClose[h]]],
    ['MA', lib.maOpen(close, 1, MAType.SMA) as unknown as Stream, [data.inClose[h]]],
    ['MAVP', lib.mavpOpen(close, periods, 2, 30, MAType.SMA) as unknown as Stream, [data.inClose[h], data.inPeriods[h]]],
  ];
  for (const [fn, s, bar] of cases) {
    const at = s.outRange();
    for (let i = at.begIdx + at.count; i <= Core.INDEX_MAX; i++) s.advance();
    const full = s.outRange();
    assert.equal(full.begIdx, at.begIdx, fn);
    assert.equal(full.begIdx + full.count, Core.INDEX_MAX + 1, `${fn}: the last bar counted`);
    const held = s.value();
    rejects(() => s.advance(), 'TA_OUT_OF_RANGE_END_INDEX', `${fn} advance`, `${fn} advance: endIdx out of range`);
    rejects(() => s.update(...bar), 'TA_OUT_OF_RANGE_END_INDEX', `${fn} update`, `${fn} update: endIdx out of range`);
    rejects(() => s.advance(), 'TA_OUT_OF_RANGE_END_INDEX', `${fn} advance again`, `${fn} advance: endIdx out of range`);
    assert.ok(Number.isFinite(s.peek(...bar)), `${fn}: peek counts no bar`);
    assert.deepEqual(s.outRange(), full, `${fn}: a refused call moved the range`);
    assert.ok(Object.is(s.value(), held), `${fn}: a refused call moved value()`);
  }
});

test('a partial overlap is checked after the history, a start address shared before it', () => {
  const buf = series(46).inClose.slice(0, 120);
  const lb = lib.smaLookback(30);
  rejects(() => lib.smaOpenAndFill(buf.subarray(0, lb), 30, buf.subarray(1, 1 + lb)), 'TA_INSUFFICIENT_HISTORY', 'a partial overlap on a history too short to open');
  rejects(() => lib.smaOpenAndFill(buf.subarray(0, lb + 10), 30, buf.subarray(1, 11 + lb)), 'TA_BAD_PARAM', 'a partial overlap on a history long enough to open');
  rejects(() => lib.smaOpenAndFill(buf.subarray(0, lb), 30, buf.subarray(0, 5)), 'TA_BAD_PARAM', 'an output starting on the input, short history');

  const maLb = lib.maLookback(30, MAType.EMA);
  rejects(() => lib.maOpenAndFill(buf.subarray(0, maLb), 30, MAType.EMA, buf.subarray(1, 1 + maLb)), 'TA_INSUFFICIENT_HISTORY', 'MA: a partial overlap on a history too short to open');
  rejects(() => lib.maOpenAndFill(buf.subarray(0, maLb + 10), 30, MAType.EMA, buf.subarray(1, 11 + maLb)), 'TA_BAD_PARAM', 'MA: a partial overlap on a history long enough to open');
  rejects(() => lib.maOpenAndFill(buf.subarray(0, maLb), 30, MAType.EMA, buf.subarray(0, 5)), 'TA_BAD_PARAM', 'MA: an output starting on the input, short history');

  const mvLb = lib.mavpLookback(2, 30, MAType.SMA);
  const per = new Float64Array(120).fill(10);
  rejects(() => lib.mavpOpenAndFill(buf.subarray(0, mvLb), per.subarray(0, mvLb), 2, 30, MAType.SMA, buf.subarray(1, 1 + mvLb)), 'TA_INSUFFICIENT_HISTORY', 'MAVP: a partial overlap on a history too short to open');
  rejects(() => lib.mavpOpenAndFill(buf.subarray(0, mvLb + 10), per.subarray(0, mvLb + 10), 2, 30, MAType.SMA, buf.subarray(1, 11 + mvLb)), 'TA_BAD_PARAM', 'MAVP: a partial overlap on a history long enough to open');
  rejects(() => lib.mavpOpenAndFill(buf.subarray(0, mvLb), per.subarray(0, mvLb), 2, 30, MAType.SMA, buf.subarray(0, 5)), 'TA_BAD_PARAM', 'MAVP: an output starting on the input, short history');

  const hlc = series(47).inClose.slice(0, 64);
  const outs = new Float64Array(200);
  const acLb = lib.accbandsLookback(20);
  const shortHlc = hlc.slice(0, acLb);
  rejects(() => lib.accbandsOpenAndFill(shortHlc, shortHlc, shortHlc, 20, outs.subarray(0, 50), outs.subarray(1, 51), outs.subarray(100, 150)), 'TA_INSUFFICIENT_HISTORY', 'two outputs overlapping in part, short history');
  rejects(() => lib.accbandsOpenAndFill(hlc, hlc, hlc, 20, outs.subarray(0, 50), outs.subarray(1, 51), outs.subarray(100, 150)), 'TA_BAD_PARAM', 'two outputs overlapping in part, history long enough');
  rejects(() => lib.accbandsOpenAndFill(shortHlc, shortHlc, shortHlc, 20, outs.subarray(0, 50), outs.subarray(0, 40), outs.subarray(100, 150)), 'TA_BAD_PARAM', 'two outputs starting at one address, short history');

  const dojiLb = lib.cdldojiLookback();
  const ohlc = series(48).inClose.slice(0, dojiLb + 40);
  const h = ohlc.subarray(0, dojiLb);
  const intOver = new Int32Array(ohlc.buffer, 8, Math.max(1, dojiLb));
  rejects(() => lib.cdldojiOpenAndFill(h, h, h, h, intOver), 'TA_INSUFFICIENT_HISTORY', 'an int output over a real input in part, short history');
  rejects(() => lib.cdldojiOpenAndFill(h, h, h, h, new Int32Array(ohlc.buffer, 0, Math.max(1, dojiLb))), 'TA_BAD_PARAM', 'an int output starting on a real input, short history');
});
