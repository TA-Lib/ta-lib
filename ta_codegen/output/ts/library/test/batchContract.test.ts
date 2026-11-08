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

// The batch argument rules no cross-language gate reaches: every server hands
// each array the full series, so an absent, empty or short array, the order of
// the rejections and a rejected call's writes are seen only here.

import assert from 'node:assert/strict';
import { test } from 'node:test';

import { type Arr, N, type Param, series } from './corpus.ts';
import { assertNamesRange, bytes, caught, filled, type Named, namedSpecs, optsOf, outOfDomain, outputsOf, rejects, untouched } from './contract.ts';
import * as lib from '../src/index.ts';
import { Core, MAType, RetCode } from '../src/index.ts';

const SPECS = namedSpecs();
const isArray = (p: Param): boolean => p.kind === 'input' || p.kind === 'output';
const lookbackOf = (s: Named): number => s.lookback(...optsOf(s).map(sentinel)) as number;

function sentinel(p: Param): number {
  return p.kind === 'real' ? Core.REAL_DEFAULT : Core.INTEGER_DEFAULT;
}

/** Every argument after the range, in signature order: `inLen`-long inputs, `outLen`-long outputs. */
function argsOf(s: Named, data: Record<string, Float64Array>, inLen: number, outLen: number): unknown[] {
  return s.params.map((p) =>
    p.kind === 'input' ? data[p.name].subarray(0, inLen) : p.kind === 'output' ? filled(p, outLen) : sentinel(p),
  );
}

function call(s: Named, startIdx: number, endIdx: number, args: unknown[]): lib.OutRange {
  return s.batch(startIdx, endIdx, ...args) as lib.OutRange;
}

function outputsIn(s: Named, args: unknown[]): Arr[] {
  return s.params.flatMap((p, k) => (p.kind === 'output' ? [args[k] as Arr] : []));
}

/** The last bar of a range that produces nothing, or -1 when every range produces something. */
function subLookbackEnd(s: Named): number {
  const lb = lookbackOf(s);
  return lb > 0 ? Math.min(lb, N) - 1 : -1;
}

test('an absent array is TA_BAD_PARAM naming it, on every range; an undefined output is allocated instead', () => {
  const data = series(31);
  let probes = 0;
  let producingNothing = 0;
  let allocated = 0;
  let expected = 0;
  for (const s of SPECS) {
    const e = subLookbackEnd(s);
    const arrays = s.params.filter(isArray).length;
    for (const end of e >= 0 ? [N - 1, e] : [N - 1]) {
      expected += 2 * arrays;
      const ref = s.batchFull(0, end, ...argsOf(s, data, N, N));
      s.params.forEach((p, k) => {
        if (!isArray(p)) return;
        for (const absent of [null, undefined]) {
          const args = argsOf(s, data, N, N);
          args[k] = absent;
          const what = `${s.fn}(0, ${end}) ${p.name}=${absent}`;
          probes++;
          if (end !== N - 1) producingNothing++;
          if (p.kind === 'output' && absent === undefined) {
            // An omitted buffer is allocated: the same range and bits as the caller's.
            const got = s.batchFull(0, end, ...args);
            assert.deepEqual(got.range, ref.range, `${what}: range`);
            got.outs.forEach((x, j) => assert.deepEqual(bytes(x), bytes(ref.outs[j]), `${what}: output ${j}`));
            allocated++;
            continue;
          }
          // An input is named by its public argument, an output by its buffer key.
          rejects(() => call(s, 0, end, args), 'TA_BAD_PARAM', what, `${s.fn}: ${p.key} is absent`);
        }
      });
    }
  }
  assert.equal(probes, expected);
  assert.ok(producingNothing > SPECS.length, `only ${producingNothing} probes on a range that produces nothing`);
  assert.equal(allocated, SPECS.reduce((n, s) => n + outputsOf(s).length * (subLookbackEnd(s) >= 0 ? 2 : 1), 0));
});

test('an empty output is refused, even on a range that produces nothing, where one element is enough', () => {
  const data = series(32);
  let empty = 0;
  let controls = 0;
  for (const s of SPECS) {
    const lb = lookbackOf(s);
    const e = subLookbackEnd(s);
    outputsOf(s).forEach((o) => {
      const k = s.params.indexOf(o);
      const full = argsOf(s, data, N, N);
      full[k] = filled(o, 0);
      rejects(() => call(s, 0, N - 1, full), 'TA_BAD_PARAM', `${s.fn}: empty ${o.name}`, `${s.fn}: ${o.key} has length 0, needs ${N - lb}`);
      empty++;
      if (e < 0) return;
      for (const none of [filled(o, 0), filled(o, 3).subarray(1, 1)]) {
        const args = argsOf(s, data, N, 1);
        args[k] = none;
        rejects(() => call(s, 0, e, args), 'TA_BAD_PARAM', `${s.fn}(0, ${e}): empty ${o.name}`, `${s.fn}: ${o.key} is empty`);
        empty++;
      }
    });
    if (e < 0) continue;
    const args = argsOf(s, data, N, 1);
    assert.deepEqual(call(s, 0, e, args), new lib.OutRange(0, 0), `${s.fn}(0, ${e}) with one-element outputs`);
    for (const a of outputsIn(s, args)) assert.ok(untouched(a), `${s.fn}: a range that produces nothing wrote an output`);
    controls++;
  }
  assert.ok(controls > SPECS.length / 2 && empty > 2 * controls, `${controls} controls, ${empty} empty outputs`);
});

test('an input that does not reach endIdx is refused on every range, one that produces nothing included', () => {
  const data = series(33);
  let full = 0;
  let producingNothing = 0;
  let expected = 0;
  const probed = new Set<string>();
  for (const s of SPECS) {
    const e = subLookbackEnd(s);
    s.params.forEach((p, k) => {
      if (p.kind !== 'input') return;
      const args = argsOf(s, data, N, N);
      args[k] = data[p.name].subarray(0, N - 1);
      rejects(() => call(s, 0, N - 1, args), 'TA_BAD_PARAM', `${s.fn}: short ${p.name}`, `${s.fn}: ${p.key} has length ${N - 1}, needs ${N}`);
      full++;
      if (e < 0) return;
      expected++;
      const sub = argsOf(s, data, e + 1, 1);
      sub[k] = data[p.name].subarray(0, e);
      const msg = `${s.fn}: ${p.key} has length ${e}, needs ${e + 1}`;
      rejects(() => call(s, 0, e, sub), 'TA_BAD_PARAM', `${s.fn}(0, ${e}): short ${p.name}`, msg);
      producingNothing++;
      probed.add(s.fn);
    });
    if (e >= 0) assert.deepEqual(call(s, 0, e, argsOf(s, data, e + 1, 1)), new lib.OutRange(0, 0), `${s.fn}: inputs reaching endIdx`);
  }
  assert.equal(producingNothing, expected);
  assert.ok(full > SPECS.length && producingNothing > SPECS.length / 2, `${full} full, ${producingNothing} sub-lookback`);
  for (const fn of ['APO', 'PPO', 'PVO', 'SMA']) assert.ok(probed.has(fn), `${fn} was not probed on a range that produces nothing`);
});

test('each output is bounded by what the call produces, named on its own, and a rejected call writes nothing', () => {
  const data = series(34);
  let short = 0;
  let held = 0;
  for (const s of SPECS) {
    const lb = lookbackOf(s);
    assert.ok(lb >= 0 && lb < N, `${s.fn}: lookback ${lb}`);
    const produced = N - lb;
    const exact = argsOf(s, data, N, produced);
    assert.deepEqual(call(s, 0, N - 1, exact), new lib.OutRange(lb, produced), `${s.fn}: exactly-sized outputs`);
    for (const a of outputsIn(s, exact)) assert.ok(!untouched(a), `${s.fn}: an accepted call left an output as filled`);
    const expectHeld = (args: unknown[], what: string): void => {
      for (const a of outputsIn(s, args)) assert.ok(untouched(a), `${s.fn}: ${what} wrote an output`);
      held++;
    };
    outputsOf(s).forEach((o) => {
      const k = s.params.indexOf(o);
      const args = argsOf(s, data, N, produced);
      args[k] = filled(o, produced - 1);
      rejects(() => call(s, 0, N - 1, args), 'TA_BAD_PARAM', `${s.fn}: ${o.name} one short`, `${s.fn}: ${o.key} has length ${produced - 1}, needs ${produced}`);
      short++;
      expectHeld(args, `${o.name} one short`);
    });
    const roomy = (): unknown[] => argsOf(s, data, N, N);
    let args = roomy();
    caught(() => call(s, -1, N - 1, args), `${s.fn}: startIdx -1`);
    expectHeld(args, 'startIdx -1');
    args = roomy();
    caught(() => call(s, 5, 4, args), `${s.fn}: endIdx < startIdx`);
    expectHeld(args, 'endIdx < startIdx');
    optsOf(s).forEach((p, j) => {
      const a = roomy();
      a[s.params.indexOf(p)] = outOfDomain(s, j);
      caught(() => call(s, 0, N - 1, a), `${s.fn}: ${p.name} out of its domain`);
      expectHeld(a, `${p.name} out of its domain`);
    });
  }
  assert.ok(short >= SPECS.length && held > 3 * SPECS.length, `${short} short outputs, ${held} rejected calls`);
});

test('given an endIdx, an index fault outranks an absent or short argument and a bad parameter an absent or short buffer; with endIdx omitted, an absent input is named first', () => {
  const data = series(35);
  let index = 0;
  let param = 0;
  let presence = 0;
  for (const s of SPECS) {
    const inK = s.params.findIndex((p) => p.kind === 'input');
    const outK = s.params.findIndex((p) => p.kind === 'output');
    const inName = s.params[inK].name;
    const inKey = s.params[inK].key;
    const without = (k: number): unknown[] => {
      const a = argsOf(s, data, N, N);
      a[k] = null;
      return a;
    };
    const shortIn = (): unknown[] => {
      const a = argsOf(s, data, N, N);
      a[inK] = data[inName].subarray(0, 1);
      return a;
    };
    // Given a range, the order every backend shares: the range first.
    rejects(() => call(s, -1, N - 1, without(inK)), 'TA_OUT_OF_RANGE_START_INDEX', `${s.fn}: startIdx -1, no ${inName}`, `${s.fn}: startIdx must be in 0..${Core.INDEX_MAX}, got -1`);
    rejects(() => call(s, 0, Core.INDEX_MAX + 1, without(inK)), 'TA_OUT_OF_RANGE_END_INDEX', `${s.fn}: endIdx past INDEX_MAX, no ${inName}`, `${s.fn}: endIdx must be in 0..${Core.INDEX_MAX}, got ${Core.INDEX_MAX + 1}`);
    // An omitted endIdx reads the inputs' length, so an absent one is named first.
    const omitted = undefined as unknown as number;
    rejects(() => call(s, -1, omitted, without(inK)), 'TA_BAD_PARAM', `${s.fn}: startIdx -1, endIdx omitted, no ${inName}`, `${s.fn}: ${inKey} is absent`);
    rejects(() => call(s, Core.INDEX_MAX + 5, omitted, without(inK)), 'TA_BAD_PARAM', `${s.fn}: startIdx past INDEX_MAX, endIdx omitted, no ${inName}`, `${s.fn}: ${inKey} is absent`);
    presence += 2;
    rejects(() => call(s, -1, N - 1, shortIn()), 'TA_OUT_OF_RANGE_START_INDEX', `${s.fn}: startIdx -1, short ${inName}`, `${s.fn}: startIdx must be in 0..${Core.INDEX_MAX}, got -1`);
    rejects(() => call(s, 50, 10, without(outK)), 'TA_OUT_OF_RANGE_END_INDEX', `${s.fn}: endIdx < startIdx, no output`, `${s.fn}: endIdx must be in 50..${Core.INDEX_MAX}, got 10`);
    rejects(() => call(s, 0, Core.INDEX_MAX + 1, shortIn()), 'TA_OUT_OF_RANGE_END_INDEX', `${s.fn}: endIdx past INDEX_MAX, short ${inName}`, `${s.fn}: endIdx must be in 0..${Core.INDEX_MAX}, got ${Core.INDEX_MAX + 1}`);
    rejects(() => call(s, Core.INDEX_MAX + 5, Core.INDEX_MAX + 9, without(outK)), 'TA_OUT_OF_RANGE_START_INDEX', `${s.fn}: startIdx past INDEX_MAX, no output`, `${s.fn}: startIdx must be in 0..${Core.INDEX_MAX}, got ${Core.INDEX_MAX + 5}`);
    index += 4;
    rejects(() => call(s, 0, N - 1, without(inK)), 'TA_BAD_PARAM', `${s.fn}: a valid range reports the absent ${inName}`, `${s.fn}: ${inKey} is absent`);

    optsOf(s).forEach((p, j) => {
      const pk = s.params.indexOf(p);
      const bad = outOfDomain(s, j);
      const noIn = without(inK);
      noIn[pk] = bad;
      const whyNoIn = `${s.fn}: ${p.name}=${bad} and no ${inName}`;
      assertNamesRange(s, j, s.fn, bad, rejects(() => call(s, 0, N - 1, noIn), 'TA_BAD_PARAM', whyNoIn), whyNoIn);
      rejects(() => call(s, 0, omitted, noIn), 'TA_BAD_PARAM', `${s.fn}: ${p.name}=${bad}, endIdx omitted, no ${inName}`, `${s.fn}: ${inKey} is absent`);
      presence++;
      const shortOut = argsOf(s, data, N, N);
      shortOut[outK] = filled(s.params[outK], 1);
      const emptyOut = argsOf(s, data, N, N);
      emptyOut[outK] = filled(s.params[outK], 0);
      for (const [what, args] of [[`a one-element ${inName}`, shortIn()], ['no output', without(outK)], ['a one-element output', shortOut], ['an empty output', emptyOut]] as const) {
        const a = args.slice();
        a[pk] = bad;
        const why = `${s.fn}: ${p.name}=${bad} and ${what}`;
        assertNamesRange(s, j, s.fn, bad, rejects(() => call(s, 0, N - 1, a), 'TA_BAD_PARAM', why), why);
        param++;
      }
    });
  }
  assert.equal(index, 4 * SPECS.length);
  assert.equal(presence, SPECS.reduce((n, s) => n + 2 + optsOf(s).length, 0));
  assert.ok(param > 4 * SPECS.length, `only ${param} parameter-order probes`);
});

test('an out-of-range parameter names its key, its value and its range at every entry point, on both sides of every integer range', () => {
  const data = series(37);
  let ints = 0;
  let others = 0;
  for (const s of SPECS) {
    const ins = s.params.filter((p) => p.kind === 'input').map((p) => data[p.name]);
    optsOf(s).forEach((p, k) => {
      const values: number[] = [];
      if (p.kind === 'int') {
        const o = s.info.optInputs[k] as { min: number; max: number };
        if (o.min - 1 > Core.INTEGER_MIN) values.push(o.min - 1);
        if (o.max + 1 <= Core.INTEGER_MAX) values.push(o.max + 1);
      } else {
        values.push(outOfDomain(s, k));
      }
      for (const v of values) {
        const opts = optsOf(s).map(sentinel);
        opts[k] = v;
        const what = `${s.fn}: ${p.name}=${v}`;
        const outs = outputsOf(s).map((o) => filled(o, N));
        assertNamesRange(s, k, s.fn, v, rejects(() => call(s, 0, N - 1, [...ins, ...opts, ...outs]), 'TA_BAD_PARAM', what), what);
        assertNamesRange(s, k, `${s.fn} open`, v, rejects(() => s.open(...ins, ...opts), 'TA_BAD_PARAM', `${what} open`), `${what} open`);
        assertNamesRange(s, k, `${s.fn} openAndFill`, v, rejects(() => s.fill(...ins, ...opts, ...outs), 'TA_BAD_PARAM', `${what} openAndFill`), `${what} openAndFill`);
        if (p.kind === 'int') ints++;
        else others++;
      }
    });
  }
  assert.ok(ints > SPECS.length, `only ${ints} integer bounds probed`);
  assert.ok(others > 50, `only ${others} real or MAType parameters probed`);
});

test('parameters each in range but rejected together claim no range', () => {
  const inp = series(38).inClose;
  const cases: [string, () => unknown][] = [
    ['MAVP', () => lib.mavp(inp, inp, { minPeriod: 20, maxPeriod: 10 })],
    ['MAVP open', () => lib.mavpOpen(inp, inp, { minPeriod: 20, maxPeriod: 10 })],
    ['MAVP openAndFill', () => lib.mavpOpenAndFill(inp, inp, { minPeriod: 20, maxPeriod: 10 })],
  ];
  for (const [entry, fn] of cases) {
    rejects(fn, 'TA_BAD_PARAM', entry, `${entry}: minPeriod, maxPeriod, maType are each in range but rejected; see the function's documentation`);
  }
  const prices = { high: inp, low: inp };
  for (const [entry, fn] of [
    ['FRAMA', () => lib.frama(prices, { timePeriod: 15 })],
    ['FRAMA open', () => lib.framaOpen(prices, { timePeriod: 15 })],
    ['FRAMA openAndFill', () => lib.framaOpenAndFill(prices, { timePeriod: 15 })],
  ] as const) {
    rejects(fn, 'TA_BAD_PARAM', entry, `${entry}: timePeriod is in range but rejected; see the function's documentation`);
  }
});

test('an overlap names both arrays: every pair of outputs, and every output over every input, in batch and openAndFill', () => {
  const data = series(39);
  let outPairs = 0;
  let outOverIn = 0;
  const view = (o: Param, buf: ArrayBuffer, byteOffset: number): Arr =>
    o.int32 ? new Int32Array(buf, byteOffset, N) : new Float64Array(buf, byteOffset, N);
  for (const s of SPECS) {
    const ins = s.params.filter((p) => p.kind === 'input');
    const outs = outputsOf(s);
    const opts = optsOf(s).map(sentinel);
    const legs = (): Float64Array[] => ins.map((p) => data[p.name]);
    const both = (what: string, l: Float64Array[], o: Arr[], a: string, b: string): void => {
      rejects(() => call(s, 0, N - 1, [...l, ...opts, ...o]), 'TA_BAD_PARAM', what, `${s.fn}: ${a} and ${b} share memory`);
      rejects(() => s.fill(...l, ...opts, ...o), 'TA_BAD_PARAM', `${what} openAndFill`, `${s.fn} openAndFill: ${a} and ${b} share memory`);
    };
    outs.forEach((a, j) => {
      outs.forEach((b, m) => {
        if (m <= j) return;
        const buf = new ArrayBuffer(N * 8);
        const o = outs.map((q) => filled(q, N));
        o[j] = view(a, buf, 0);
        o[m] = view(b, buf, 0);
        both(`${s.fn}: ${a.name} on ${b.name}`, legs(), o, a.key, b.key);
        outPairs++;
      });
      ins.forEach((p, k) => {
        const host = new Float64Array(N + 1);
        host.set(data[p.name]);
        const l = legs();
        l[k] = host.subarray(0, N);
        const o = outs.map((q) => filled(q, N));
        o[j] = view(a, host.buffer, 8);
        both(`${s.fn}: ${a.name} over ${p.name}`, l, o, a.key, p.key);
        outOverIn++;
      });
    });
  }
  assert.ok(outPairs > 20, `only ${outPairs} output pairs`);
  assert.equal(outOverIn, SPECS.reduce((n, s) => n + outputsOf(s).length * s.params.filter((p) => p.kind === 'input').length, 0));
});

test('messages name what the caller typed', () => {
  const close = series(40).inClose;
  const high = series(40).inHigh;
  const cases: [() => unknown, string][] = [
    [() => lib.sma(close, { timePeriod: 0 }), 'SMA: timePeriod must be in 1..100000, got 0'],
    [() => lib.sma(close, { timePeriod: 1.5 }), 'SMA: timePeriod is not an int32, got 1.5'],
    [() => lib.sma(close, { endIdx: close.length }), `SMA: real has length ${close.length}, needs ${close.length + 1}`],
    [() => lib.ma(close, { maType: 99 as MAType }), 'MA: maType must be a MAType, got 99'],
    [() => lib.bbands(close, { nbDevUp: Number.NaN }), 'BBANDS: nbDevUp must be in -3e37..3e37, got NaN'],
    [() => lib.mavp(close, close, { minPeriod: 20, maxPeriod: 10 }), "MAVP: minPeriod, maxPeriod, maType are each in range but rejected; see the function's documentation"],
    [() => lib.macd(close, { outMacd: close.slice(), outMacdSignal: undefined, outMacdHist: new Int32Array(N) as never }), 'MACD: outMacdHist is not a Float64Array'],
    [() => { const b = new Float64Array(N); return lib.macd(close, { outMacd: b, outMacdSignal: b }); }, 'MACD: outMacd and outMacdSignal share memory'],
    [() => lib.atr({ high, low: undefined as never, close }), 'ATR: prices.low is absent'],
    [() => lib.atrOpen({ high, low: high.subarray(1), close }), `ATR open: prices.low has length ${N - 1}, needs ${N}`],
    [() => lib.cdldoji({ open: close, high, low: close, close }, { outInteger: new Float64Array(N) as never }), 'CDLDOJI: outInteger is not an Int32Array'],
  ];
  for (const [fn, message] of cases) rejects(fn, 'TA_BAD_PARAM', message, message);
});

test('every failure carries its code, and the probes reach exactly the batch and streaming vocabulary', () => {
  const seen = new Set<string>();
  const code = (want: keyof typeof RetCode, fn: () => unknown, what: string): void => {
    const e = rejects(fn, `TA_${want}`, what);
    assert.equal(e.retCode, RetCode[want], `${what}: retCode`);
    seen.add(e.code);
  };
  const data = series(36);
  const inp = data.inClose.subarray(0, 200);
  const out = new Float64Array(200);

  code('OUT_OF_RANGE_START_INDEX', () => lib.sma(inp, { startIdx: -1, endIdx: 50, timePeriod: 10, outReal: out }), 'negative startIdx');
  code('OUT_OF_RANGE_END_INDEX', () => lib.sma(inp, { startIdx: 50, endIdx: 10, timePeriod: 10, outReal: out }), 'endIdx < startIdx');
  code('BAD_PARAM', () => lib.sma(inp, { startIdx: 0, endIdx: 50, timePeriod: 0, outReal: out }), 'an out-of-range period');
  code('BAD_PARAM', () => lib.macd(inp, { fastPeriod: 12, slowPeriod: 26, signalPeriod: 9, outMacd: out, outMacdSignal: out, outMacdHist: new Float64Array(200) }), 'two outputs sharing one array');
  code('BAD_PARAM', () => lib.sma(null as unknown as Float64Array, { startIdx: 0, endIdx: 199, timePeriod: 10, outReal: out }), 'an absent input');
  code('BAD_PARAM', () => lib.sma(new Float64Array(0), { timePeriod: 10 }), 'an empty input');
  code('BAD_PARAM', () => lib.sma(inp, { timePeriod: 10, outReal: new Float64Array(3) }), 'a short output');
  code('BAD_PARAM', () => lib.ma(inp, { timePeriod: 10, maType: 99 as MAType, outReal: out }), 'an MA type outside its domain');

  const lb30 = lib.smaLookback({ timePeriod: 30 });
  code('INSUFFICIENT_HISTORY', () => lib.smaOpen(inp.subarray(0, lb30), { timePeriod: 30 }), 'a short history');
  code('INSUFFICIENT_HISTORY', () => lib.smaOpen(new Float64Array(0), { timePeriod: 30 }), 'an empty history');
  code('BAD_PARAM', () => lib.smaOpen(new Float64Array(0), { timePeriod: 0 }), 'an out-of-range period on an empty history');
  code('BAD_PARAM', () => lib.smaOpen(inp, { timePeriod: 0 }), 'an out-of-range period at open');
  code('BAD_PARAM', () => lib.bbandsOpenAndFill(inp, { timePeriod: 20, nbDevUp: 2, nbDevDn: 2, maType: MAType.SMA, outUpperBand: out, outMiddleBand: out, outLowerBand: new Float64Array(200) }), 'aliased fill outputs');
  code('BAD_PARAM', () => lib.smaOpenAndFill(inp, { timePeriod: 30, outReal: inp }), 'a fill output on its input');
  code('BAD_PARAM', () => lib.mamaOpenAndFill(inp, { fastLimit: 0.5, slowLimit: 0.05, outMama: null as unknown as Float64Array<ArrayBuffer>, outFama: new Float64Array(200) }), 'a fill with a null first output');

  rejects(() => lib.smaOpen(inp.subarray(0, lb30), { timePeriod: 30 }), 'TA_INSUFFICIENT_HISTORY', 'short history message', `SMA open: history has length ${lb30}, needs ${lb30 + 1}`);
  const maLb = lib.maLookback({ timePeriod: 10, maType: MAType.EMA });
  rejects(() => lib.maOpenAndFill(inp.subarray(0, maLb), { timePeriod: 10, maType: MAType.EMA, outReal: out }), 'TA_INSUFFICIENT_HISTORY', 'MA fill', `MA openAndFill: history has length ${maLb}, needs ${maLb + 1}`);

  assert.deepEqual(
    [RetCode.SUCCESS, RetCode.BAD_PARAM, RetCode.ALLOC_ERR, RetCode.OUT_OF_RANGE_START_INDEX, RetCode.OUT_OF_RANGE_END_INDEX, RetCode.INSUFFICIENT_HISTORY, RetCode.INTERNAL_ERROR],
    [0, 2, 3, 12, 13, 17, 5000],
  );
  // ALLOC_ERR is never thrown here, and INTERNAL_ERROR needs a corrupted state.
  assert.deepEqual([...seen].sort(), ['TA_BAD_PARAM', 'TA_INSUFFICIENT_HISTORY', 'TA_OUT_OF_RANGE_END_INDEX', 'TA_OUT_OF_RANGE_START_INDEX']);
});
