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
 *  110426 MF,CC  First Version
 */

// The call form every public entry point shares: inputs, then one optional
// object of parameters, range, `core` and buffers, every key optional. Swept
// over every function, since a generated wrapper per function could get one
// key wrong on its own.

import assert from 'node:assert/strict';
import { test } from 'node:test';

import { type Arr, N, series } from './corpus.ts';
import { bytes, filled, type Named, namedSpecs, optsOf, outOfDomain, outputsOf, rejects, sentinels, untouched } from './contract.ts';
import { Core, OutRange } from '../src/index.ts';

const SPECS = namedSpecs();
const data = series(61);

type Result = Record<string, unknown>;
type Stream = { outRange(): OutRange; value(...a: unknown[]): unknown };

const insOf = (s: Named, n = N): Float64Array[] => s.params.filter((p) => p.kind === 'input').map((p) => data[p.name].subarray(0, n));
const outsIn = (s: Named, r: Result): Arr[] => outputsOf(s).map((p) => r[p.result as string] as Arr);

function sameResult(s: Named, got: Result, want: Result, what: string): void {
  assert.equal(got.begIdx, want.begIdx, `${s.fn} ${what}: begIdx`);
  const w = outsIn(s, want);
  outsIn(s, got).forEach((a, j) => assert.deepEqual(bytes(a), bytes(w[j]), `${s.fn} ${what}: output ${j}`));
}

/** The stream's range and current outputs, in output order. */
function state(h: Stream): unknown[] {
  const v = h.value();
  const r = h.outRange();
  const vals = typeof v === 'number' ? [v] : Object.values(v as Record<string, number>);
  return [r.begIdx, r.count, ...vals];
}

test('an omitted, an undefined and a sentinel parameter give the same bits at every entry point', () => {
  let probes = 0;
  for (const s of SPECS) {
    const ins = insOf(s);
    const args = s.args(ins);
    const sent = sentinels(s);
    const ref = s.options(sent);
    const want = s.api.batch(...args, ref) as Result;
    const wantLb = s.api.lookback(ref);
    const wantShift = s.api.shift(0, ref);
    const wantOpen = state(s.api.open(...args, ref) as Stream);
    const wantFill = s.api.fill(...args, ref) as Result;
    const forms: [string, Result | undefined][] = [['no options', undefined], ['empty options', {}]];
    optsOf(s).forEach((p, k) => {
      const omitted = s.options(sent);
      delete omitted[p.key];
      const undef = s.options(sent);
      undef[p.key] = undefined;
      const value = s.options(sent);
      value[p.key] = p.dflt;
      forms.push([`${p.key} omitted`, omitted], [`${p.key} undefined`, undef], [`${p.key} = ${p.dflt}`, value]);
      assert.notEqual(sent[k], p.dflt, `${s.fn}: ${p.key}'s sentinel is its default`);
    });
    for (const [what, o] of forms) {
      sameResult(s, s.api.batch(...args, o) as Result, want, what);
      assert.equal(s.api.lookback(o), wantLb, `${s.fn} ${what}: lookback`);
      assert.equal(s.api.shift(0, o), wantShift, `${s.fn} ${what}: display shift`);
      assert.deepEqual(state(s.api.open(...args, o) as Stream), wantOpen, `${s.fn} ${what}: open`);
      sameResult(s, s.api.fill(...args, o) as Result, wantFill, `${what} openAndFill`);
      probes++;
    }
  }
  assert.equal(probes, SPECS.reduce((n, s) => n + 2 + 3 * optsOf(s).length, 0));
});

test('an omitted or undefined startIdx is 0, and endIdx the last bar of the first input', () => {
  let probes = 0;
  let firstShorter = 0;
  for (const s of SPECS) {
    const ins = insOf(s);
    const args = s.args(ins);
    const at = (startIdx: number, endIdx: number): Result => s.api.batch(...args, { startIdx, endIdx }) as Result;
    const call = (o: Result | undefined): Result => s.api.batch(...args, o) as Result;
    sameResult(s, call(undefined), at(0, N - 1), 'no range');
    sameResult(s, call({ startIdx: undefined, endIdx: undefined }), at(0, N - 1), 'undefined range');
    sameResult(s, call({ startIdx: 100 }), at(100, N - 1), 'startIdx only');
    sameResult(s, call({ endIdx: 150 }), at(0, 150), 'endIdx only');
    sameResult(s, call({ startIdx: 100, endIdx: undefined }), at(100, N - 1), 'endIdx undefined');
    assert.ok(outsIn(s, at(100, 150)).every((a) => a.length > 0), `${s.fn}: bars 100..150 produce nothing`);
    probes++;
    if (ins.length < 2) continue;
    // The first input bounds the default range; a later one shorter than it is refused.
    const shortFirst = s.args(ins.map((a, k) => (k === 0 ? a.subarray(0, N - 10) : a)));
    sameResult(s, s.api.batch(...shortFirst) as Result, at(0, N - 11), 'a shorter first input');
    const shortLast = s.args(ins.map((a, k) => (k === ins.length - 1 ? a.subarray(0, N - 10) : a)));
    rejects(() => s.api.batch(...shortLast), 'TA_BAD_PARAM', `${s.fn}: a shorter later input`, `${s.fn}: ${s.params.filter((p) => p.kind === 'input')[ins.length - 1].key} has length ${N - 10}, needs ${N}`);
    firstShorter++;
  }
  assert.equal(probes, SPECS.length);
  assert.ok(firstShorter > SPECS.length / 2, `only ${firstShorter} multi-input functions`);
});

test('a caller buffer comes back as a view of its own memory from its first element, as long as the values written', () => {
  let views = 0;
  for (const s of SPECS) {
    const ins = insOf(s);
    const args = s.args(ins);
    const want = s.api.batch(...args) as Result;
    const count = outsIn(s, want)[0].length;
    for (const offset of [0, 3]) {
      const bufs = outputsOf(s).map((p) => filled(p, N + offset + 2).subarray(offset, offset + N));
      const got = s.api.batch(...args, s.options([], bufs)) as Result;
      sameResult(s, got, want, `buffers at element ${offset}`);
      outsIn(s, got).forEach((a, j) => {
        const b = bufs[j];
        assert.ok(a.buffer === b.buffer, `${s.fn}: output ${j} is not in its buffer`);
        assert.equal(a.byteOffset, b.byteOffset, `${s.fn}: output ${j} byteOffset`);
        assert.equal(a.length, count, `${s.fn}: output ${j} length`);
        assert.ok(untouched(b.subarray(count)), `${s.fn}: output ${j} written past the count`);
        assert.ok(untouched(new (b.constructor as typeof Float64Array)(b.buffer, 0, offset) as Arr), `${s.fn}: output ${j} written before its buffer`);
        views++;
      });
    }
    const lb = s.api.lookback() as number;
    const fbufs = outputsOf(s).map((p) => filled(p, N));
    const f = s.api.fill(...args, s.options([], fbufs)) as Result;
    outsIn(s, f).forEach((a, j) => {
      assert.ok(a.buffer === fbufs[j].buffer && a.byteOffset === 0 && a.length === N - lb, `${s.fn} openAndFill: output ${j} is not a view of its buffer`);
      views++;
    });
  }
  assert.equal(views, 3 * SPECS.reduce((n, s) => n + outputsOf(s).length, 0));
});

test('a partly given multi-output buffer set: each given buffer is a view, the rest are allocated, all with the same bits', () => {
  let probes = 0;
  for (const s of SPECS) {
    const outs = outputsOf(s);
    if (outs.length < 2) continue;
    const args = s.args(insOf(s));
    const all = (): Arr[] => outs.map((p) => filled(p, N));
    const want = s.api.batch(...args, s.options([], all())) as Result;
    const wantFill = s.api.fill(...args, s.options([], all())) as Result;
    outs.forEach((p, k) => {
      for (const how of ['others omitted', 'others undefined']) {
        const given = filled(p, N);
        const bufs = outs.map((_, j) => (j === k ? given : undefined));
        const o = s.options([], bufs);
        if (how === 'others omitted') for (const q of outs) if (q !== p) delete o[q.key];
        for (const [what, got, ref] of [
          ['batch', s.api.batch(...args, o) as Result, want],
          ['openAndFill', s.api.fill(...args, o) as Result, wantFill],
        ] as const) {
          sameResult(s, got, ref, `${what} with only ${p.key}, ${how}`);
          const r = outsIn(s, got);
          assert.ok(r[k].buffer === given.buffer && r[k].byteOffset === 0, `${s.fn} ${what}: ${p.key} is not a view of its buffer`);
          r.forEach((a, j) => {
            if (j === k) return;
            assert.ok(a instanceof (outs[j].int32 ? Int32Array : Float64Array), `${s.fn} ${what}: allocated ${outs[j].key} type`);
            assert.ok(a.buffer !== given.buffer && r.every((b, i) => i === j || b.buffer !== a.buffer), `${s.fn} ${what}: allocated ${outs[j].key} shares memory`);
          });
          probes++;
        }
      }
    });
  }
  assert.ok(probes > 100, `only ${probes} partial buffer sets`);
});

test('an empty series is TA_BAD_PARAM naming it when endIdx is omitted, given a range the range is checked first, and an absent price object is named too', () => {
  let probes = 0;
  let priced = 0;
  for (const s of SPECS) {
    const ins = insOf(s);
    const inputs = s.params.filter((p) => p.kind === 'input');
    inputs.forEach((p, k) => {
      for (const empty of [new Float64Array(0), [] as number[]]) {
        const args = s.args(ins.map((a, j) => (j === k ? empty : a)));
        // endIdx omitted: the default reads the length, so the empty series is named first.
        for (const o of [undefined, { startIdx: 0 }, { startIdx: -1 }]) {
          rejects(() => s.api.batch(...args, o), 'TA_BAD_PARAM', `${s.fn}: empty ${p.key}, ${JSON.stringify(o)}`, `${s.fn}: ${p.key} is empty`);
          probes++;
        }
        // endIdx given: every backend's order, the range ahead of the input's length.
        rejects(() => s.api.batch(...args, { startIdx: 0, endIdx: 0 }), 'TA_BAD_PARAM', `${s.fn}: empty ${p.key}, range 0..0`, `${s.fn}: ${p.key} has length 0, needs 1`);
        rejects(() => s.api.batch(...args, { startIdx: -1, endIdx: N - 1 }), 'TA_OUT_OF_RANGE_START_INDEX', `${s.fn}: empty ${p.key}, startIdx -1`, `${s.fn}: startIdx must be in 0..${Core.INDEX_MAX}, got -1`);
        probes += 2;
      }
    });
    const priceArg = inputs.find((p) => p.key.includes('.'))?.key.split('.')[0];
    if (priceArg === undefined) continue;
    const at = [...new Set(inputs.map((p) => p.key.split('.')[0]))].indexOf(priceArg);
    for (const absent of [null, undefined]) {
      const args = s.args(ins);
      args[at] = absent;
      rejects(() => s.api.batch(...args), 'TA_BAD_PARAM', `${s.fn}: ${priceArg}=${absent}`, `${s.fn}: ${priceArg} is absent`);
      rejects(() => s.api.open(...args), 'TA_BAD_PARAM', `${s.fn} open: ${priceArg}=${absent}`, `${s.fn} open: ${priceArg} is absent`);
      rejects(() => s.api.fill(...args), 'TA_BAD_PARAM', `${s.fn} openAndFill: ${priceArg}=${absent}`, `${s.fn} openAndFill: ${priceArg} is absent`);
    }
    priced++;
  }
  assert.equal(probes, 10 * SPECS.reduce((n, s) => n + s.params.filter((p) => p.kind === 'input').length, 0));
  assert.ok(priced > SPECS.length / 2, `only ${priced} functions on prices`);
});

test('Core.LOOKBACK_INVALID and Core.DISPLAY_SHIFT_INVALID are what a rejected parameter or output index answers', () => {
  assert.equal(Core.LOOKBACK_INVALID, -1);
  assert.equal(Core.DISPLAY_SHIFT_INVALID, -2147483648);
  let probes = 0;
  for (const s of SPECS) {
    const n = outputsOf(s).length;
    assert.ok((s.api.lookback() as number) >= 0, `${s.fn}: default lookback`);
    assert.equal(s.api.shift(n), Core.DISPLAY_SHIFT_INVALID, `${s.fn}: output index ${n}`);
    assert.equal(s.api.shift(-1), Core.DISPLAY_SHIFT_INVALID, `${s.fn}: output index -1`);
    optsOf(s).forEach((p, k) => {
      const o = s.options(sentinels(s));
      o[p.key] = outOfDomain(s, k);
      assert.equal(s.api.lookback(o), Core.LOOKBACK_INVALID, `${s.fn}: ${p.key}=${String(o[p.key])} lookback`);
      for (let j = 0; j < n; j++) assert.equal(s.api.shift(j, o), Core.DISPLAY_SHIFT_INVALID, `${s.fn}: ${p.key}=${String(o[p.key])} display shift ${j}`);
      probes++;
    });
  }
  assert.equal(probes, SPECS.reduce((n, s) => n + optsOf(s).length, 0));
});

test('options that are not an object are TA_BAD_PARAM at every entry point, and one object serves them all', () => {
  let probes = 0;
  for (const s of SPECS) {
    const ins = insOf(s);
    const args = s.args(ins);
    const bads: [unknown, string][] = [[null, 'null'], [5, '5'], ['x', '"x"'], [true, 'true'], [[], 'an array'], [new Float64Array(2), 'a typed array or DataView']];
    for (const [bad, shown] of bads) {
      const what = `${s.fn} options ${shown}`;
      for (const [verb, fn] of [
        ['', () => s.api.batch(...args, bad)],
        [' lookback', () => s.api.lookback(bad)],
        [' displayShift', () => s.api.shift(0, bad)],
        [' open', () => s.api.open(...args, bad)],
        [' openAndFill', () => s.api.fill(...args, bad)],
      ] as const) {
        rejects(fn, 'TA_BAD_PARAM', `${what}${verb}`, `${s.fn}${verb}: options must be a plain object, got ${shown}`);
        probes++;
      }
    }
    // The batch call's whole object, range and buffers included, at the calls that ignore them.
    const sent = sentinels(s);
    const lb = s.api.lookback(s.options(sent)) as number;
    const every = { ...s.options(sent, outputsOf(s).map((p) => filled(p, N))), startIdx: 7, endIdx: N - 2, core: Core.DEFAULT };
    assert.equal(s.api.lookback(every), lb, `${s.fn}: lookback over the batch object`);
    assert.equal(s.api.shift(0, every), s.api.shift(0, s.options(sent)), `${s.fn}: display shift over the batch object`);
    assert.deepEqual(state(s.api.open(...args, every) as Stream), state(s.api.open(...args) as Stream), `${s.fn}: open over the batch object`);
    const r = s.api.batch(...args, every) as Result;
    assert.equal(r.begIdx, Math.max(7, lb), `${s.fn}: the batch object's range`);
    assert.equal(outsIn(s, r)[0].length, N - 2 - Math.max(7, lb) + 1, `${s.fn}: the batch object's count`);
  }
  assert.equal(probes, 30 * SPECS.length);
});

test('a core that is not a Core is TA_BAD_PARAM at every entry point, and an undefined one is Core.DEFAULT', () => {
  let probes = 0;
  for (const s of SPECS) {
    const ins = insOf(s);
    const args = s.args(ins);
    const hint = '; a Core built by another copy of ta-lib does not count';
    for (const [bad, tail] of [[null, ''], [{}, hint], [5, '']] as const) {
      const o = { core: bad };
      const what = `${s.fn} core ${JSON.stringify(bad)}`;
      for (const [verb, fn] of [
        ['', () => s.api.batch(...args, o)],
        [' lookback', () => s.api.lookback(o)],
        [' displayShift', () => s.api.shift(0, o)],
        [' open', () => s.api.open(...args, o)],
        [' openAndFill', () => s.api.fill(...args, o)],
      ] as const) {
        rejects(fn, 'TA_BAD_PARAM', `${what}${verb}`, `${s.fn}${verb}: core is not a Core${tail}`);
        probes++;
      }
    }
    assert.equal(s.api.lookback({ core: undefined }), s.api.lookback({ core: Core.DEFAULT }), `${s.fn}: undefined core at lookback`);
    assert.deepEqual(outsIn(s, s.api.batch(...args, { core: undefined }) as Result), outsIn(s, s.api.batch(...args, { core: Core.DEFAULT }) as Result), `${s.fn}: undefined core at batch`);
  }
  assert.equal(probes, 15 * SPECS.length);
});
