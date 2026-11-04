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
import { caught, filled, type Named, namedSpecs, optsOf, outOfDomain, outputsOf, rejects, untouched } from './contract.ts';
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

test('an absent array, null or undefined, is TA_BAD_PARAM naming it, on every range', () => {
  const data = series(31);
  let probes = 0;
  let producingNothing = 0;
  let expected = 0;
  for (const s of SPECS) {
    const e = subLookbackEnd(s);
    const arrays = s.params.filter(isArray).length;
    for (const end of e >= 0 ? [N - 1, e] : [N - 1]) {
      expected += 2 * arrays;
      s.params.forEach((p, k) => {
        if (!isArray(p)) return;
        for (const absent of [null, undefined]) {
          const args = argsOf(s, data, N, N);
          args[k] = absent;
          rejects(() => call(s, 0, end, args), 'TA_BAD_PARAM', `${s.fn}(0, ${end}) ${p.name}=${absent}`, `${s.fn}: ${p.name} is absent`);
          probes++;
          if (end !== N - 1) producingNothing++;
        }
      });
    }
  }
  assert.equal(probes, expected);
  assert.ok(producingNothing > SPECS.length, `only ${producingNothing} probes on a range that produces nothing`);
});

test('an empty output is an absent one, even on a range that produces nothing, where one element is enough', () => {
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
      rejects(() => call(s, 0, N - 1, full), 'TA_BAD_PARAM', `${s.fn}: empty ${o.name}`, `${s.fn}: ${o.name} has length 0, needs ${N - lb}`);
      empty++;
      if (e < 0) return;
      for (const none of [filled(o, 0), filled(o, 3).subarray(1, 1)]) {
        const args = argsOf(s, data, N, 1);
        args[k] = none;
        rejects(() => call(s, 0, e, args), 'TA_BAD_PARAM', `${s.fn}(0, ${e}): empty ${o.name}`, `${s.fn}: ${o.name} is empty`);
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
      rejects(() => call(s, 0, N - 1, args), 'TA_BAD_PARAM', `${s.fn}: short ${p.name}`, `${s.fn}: ${p.name} has length ${N - 1}, needs ${N}`);
      full++;
      if (e < 0) return;
      expected++;
      const sub = argsOf(s, data, e + 1, 1);
      sub[k] = data[p.name].subarray(0, e);
      rejects(() => call(s, 0, e, sub), 'TA_BAD_PARAM', `${s.fn}(0, ${e}): short ${p.name}`, `${s.fn}: ${p.name} has length ${e}, needs ${e + 1}`);
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
      rejects(() => call(s, 0, N - 1, args), 'TA_BAD_PARAM', `${s.fn}: ${o.name} one short`, `${s.fn}: ${o.name} has length ${produced - 1}, needs ${produced}`);
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

test('an index fault outranks an absent argument, and a bad parameter outranks an absent or short buffer', () => {
  const data = series(35);
  let index = 0;
  let param = 0;
  for (const s of SPECS) {
    const inK = s.params.findIndex((p) => p.kind === 'input');
    const outK = s.params.findIndex((p) => p.kind === 'output');
    const inName = s.params[inK].name;
    const without = (k: number): unknown[] => {
      const a = argsOf(s, data, N, N);
      a[k] = null;
      return a;
    };
    rejects(() => call(s, -1, N - 1, without(inK)), 'TA_OUT_OF_RANGE_START_INDEX', `${s.fn}: startIdx -1, no ${inName}`, `${s.fn}: startIdx out of range`);
    rejects(() => call(s, 50, 10, without(outK)), 'TA_OUT_OF_RANGE_END_INDEX', `${s.fn}: endIdx < startIdx, no output`, `${s.fn}: endIdx out of range`);
    rejects(() => call(s, 0, Core.INDEX_MAX + 1, without(inK)), 'TA_OUT_OF_RANGE_END_INDEX', `${s.fn}: endIdx past INDEX_MAX, no ${inName}`, `${s.fn}: endIdx out of range`);
    rejects(() => call(s, Core.INDEX_MAX + 5, Core.INDEX_MAX + 9, without(outK)), 'TA_OUT_OF_RANGE_START_INDEX', `${s.fn}: startIdx past INDEX_MAX, no output`, `${s.fn}: startIdx out of range`);
    index += 4;
    rejects(() => call(s, 0, N - 1, without(inK)), 'TA_BAD_PARAM', `${s.fn}: a valid range reports the absent ${inName}`, `${s.fn}: ${inName} is absent`);

    optsOf(s).forEach((p, j) => {
      const pk = s.params.indexOf(p);
      const bad = outOfDomain(s, j);
      const shortOut = argsOf(s, data, N, N);
      shortOut[outK] = filled(s.params[outK], 1);
      const emptyOut = argsOf(s, data, N, N);
      emptyOut[outK] = filled(s.params[outK], 0);
      for (const [what, args] of [[`no ${inName}`, without(inK)], ['no output', without(outK)], ['a one-element output', shortOut], ['an empty output', emptyOut]] as const) {
        const a = args.slice();
        a[pk] = bad;
        rejects(() => call(s, 0, N - 1, a), 'TA_BAD_PARAM', `${s.fn}: ${p.name}=${bad} and ${what}`, `${s.fn}: bad parameter...`);
        param++;
      }
    });
  }
  assert.equal(index, 4 * SPECS.length);
  assert.ok(param > 4 * SPECS.length, `only ${param} parameter-order probes`);
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

  code('OUT_OF_RANGE_START_INDEX', () => lib.sma(-1, 50, inp, 10, out), 'negative startIdx');
  code('OUT_OF_RANGE_END_INDEX', () => lib.sma(50, 10, inp, 10, out), 'endIdx < startIdx');
  code('BAD_PARAM', () => lib.sma(0, 50, inp, 0, out), 'an out-of-range period');
  code('BAD_PARAM', () => lib.macd(0, 199, inp, 12, 26, 9, out, out, new Float64Array(200)), 'two outputs sharing one array');
  code('BAD_PARAM', () => lib.sma(0, 199, null as unknown as Float64Array, 10, out), 'an absent input');
  code('BAD_PARAM', () => lib.sma(0, 199, inp, 10, new Float64Array(3)), 'a short output');
  code('BAD_PARAM', () => lib.ma(0, 199, inp, 10, 99 as MAType, out), 'an MA type outside its domain');

  const lb30 = lib.smaLookback(30);
  code('INSUFFICIENT_HISTORY', () => lib.smaOpen(inp.subarray(0, lb30), 30), 'a short history');
  code('INSUFFICIENT_HISTORY', () => lib.smaOpen(new Float64Array(0), 30), 'an empty history');
  code('BAD_PARAM', () => lib.smaOpen(inp, 0), 'an out-of-range period at open');
  code('BAD_PARAM', () => lib.bbandsOpenAndFill(inp, 20, 2, 2, MAType.SMA, out, out, new Float64Array(200)), 'aliased fill outputs');
  code('BAD_PARAM', () => lib.smaOpenAndFill(inp, 30, inp), 'a fill output on its input');
  code('BAD_PARAM', () => lib.mamaOpenAndFill(inp, 0.5, 0.05, null as unknown as Float64Array, new Float64Array(200)), 'a fill without its first output');

  rejects(() => lib.smaOpen(inp.subarray(0, lb30), 30), 'TA_INSUFFICIENT_HISTORY', 'short history message', `SMA open: history has length ${lb30}, needs ${lb30 + 1}`);
  const maLb = lib.maLookback(10, MAType.EMA);
  rejects(() => lib.maOpenAndFill(inp.subarray(0, maLb), 10, MAType.EMA, out), 'TA_INSUFFICIENT_HISTORY', 'MA fill', `MA openAndFill: history has length ${maLb}, needs ${maLb + 1}`);

  assert.deepEqual(
    [RetCode.SUCCESS, RetCode.BAD_PARAM, RetCode.ALLOC_ERR, RetCode.OUT_OF_RANGE_START_INDEX, RetCode.OUT_OF_RANGE_END_INDEX, RetCode.INSUFFICIENT_HISTORY, RetCode.INTERNAL_ERROR],
    [0, 2, 3, 12, 13, 17, 5000],
  );
  // ALLOC_ERR is never thrown here, and INTERNAL_ERROR needs a corrupted state.
  assert.deepEqual([...seen].sort(), ['TA_BAD_PARAM', 'TA_INSUFFICIENT_HISTORY', 'TA_OUT_OF_RANGE_END_INDEX', 'TA_OUT_OF_RANGE_START_INDEX']);
});
