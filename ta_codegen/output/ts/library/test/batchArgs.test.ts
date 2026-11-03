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

// H15: views alias by bytes, never by identity. An output shares no byte with
// another output, nor with an input unless it is that input in place: the same
// buffer, start and length. H17: an integer argument that is not an int32, or a real one that
// is not a number, is TA_BAD_PARAM before any range check, and the sentinels
// select the defaults.

import assert from 'node:assert/strict';
import { test } from 'node:test';

import { type Arr, type Fn, isBadParam, L, N, type Param, type Spec, series, specs } from './corpus.ts';
import * as lib from '../src/index.ts';
import { Core, TALibError } from '../src/index.ts';

const SPECS = specs();
const TAIL = 4;

const inputsOf = (s: Spec): Param[] => s.params.filter((p) => p.kind === 'input');
const outputsOf = (s: Spec): Param[] => s.params.filter((p) => p.kind === 'output');
const optsOf = (s: Spec): Param[] => s.params.filter((p) => p.kind !== 'input' && p.kind !== 'output');
const sentinels = (s: Spec): number[] => optsOf(s).map((p) => (p.kind === 'real' ? Core.REAL_DEFAULT : Core.INTEGER_DEFAULT));

function bytes(a: Arr, n = a.length): Uint8Array {
  return new Uint8Array(a.buffer, a.byteOffset, n * a.BYTES_PER_ELEMENT);
}

function fresh(p: Param, n: number): Arr {
  return p.int32 ? new Int32Array(n).fill(-7) : new Float64Array(n).fill(Number.NaN);
}

function view(p: Param, buf: ArrayBuffer, byteOffset: number, n: number): Arr {
  return p.int32 ? new Int32Array(buf, byteOffset, n) : new Float64Array(buf, byteOffset, n);
}

function call(spec: Spec, ins: ArrayLike<number>[], opts: number[], outs: Arr[]): { begIdx: number; count: number } {
  return spec.batch(0, N - 1, ...ins, ...opts, ...outs) as { begIdx: number; count: number };
}

function throwsCode(fn: () => unknown, code: string, what: string): void {
  assert.throws(fn, (e: unknown) => e instanceof TALibError && e.code === code, what);
}

test('an output on an input, through a separate view of the same memory, gives the reference bits', () => {
  const data = series(21);
  let checked = 0;
  for (const spec of SPECS) {
    const ins = inputsOf(spec).map((p) => data[p.name]);
    const opts = sentinels(spec);
    const refOuts = outputsOf(spec).map((p) => fresh(p, N));
    const ref = call(spec, ins, opts, refOuts);
    if (ref.count === 0) continue;
    outputsOf(spec).forEach((o, k) => {
      if (o.int32) return;
      ins.forEach((_, i) => {
        const buf = new ArrayBuffer(8 * (3 + N + TAIL));
        const shared = new Float64Array(buf, 24, N + TAIL);
        shared.set(ins[i]);
        shared.fill(-1.25, N);
        const aliasIns = ins.map((a, j) => (j === i ? shared.subarray(0, N) : a));
        const outs = outputsOf(spec).map((p, j) => (j === k ? new Float64Array(buf, 24, N) : fresh(p, N)));
        const what = `${spec.name}: out${k} on in${i}`;
        const got = call(spec, aliasIns, opts, outs);
        assert.deepEqual(got, ref, `${what}: range`);
        outs.forEach((a, j) => assert.deepEqual(bytes(a, ref.count), bytes(refOuts[j], ref.count), `${what}: output ${j}`));
        assert.ok(shared.subarray(N).every((x) => x === -1.25), `${what}: wrote past endIdx`);
        checked++;
      });
    });
  }
  assert.ok(checked > SPECS.length, `only ${checked} in-place calls`);
});

test('two outputs sharing any memory are TA_BAD_PARAM; apart in one buffer they are two arrays', () => {
  const data = series(22);
  let pairs = 0;
  for (const spec of SPECS) {
    const outs = outputsOf(spec);
    if (outs.length < 2) continue;
    const ins = inputsOf(spec).map((p) => data[p.name]);
    const opts = sentinels(spec);
    const refOuts = outs.map((p) => fresh(p, N));
    const ref = call(spec, ins, opts, refOuts);
    for (let a = 0; a < outs.length; a++) {
      for (let b = a + 1; b < outs.length; b++) {
        const buf = new ArrayBuffer(8 * 2 * (N + 1));
        const same = outs.map((p, j) => (j === a || j === b ? view(p, buf, 8, N) : fresh(p, N)));
        throwsCode(() => call(spec, ins, opts, same), 'TA_BAD_PARAM', `${spec.name}: out${a} and out${b} at one address`);
        const offset = outs.map((p, j) => (j === a ? view(p, buf, 0, N) : j === b ? view(p, buf, 8, N) : fresh(p, N)));
        throwsCode(() => call(spec, ins, opts, offset), 'TA_BAD_PARAM', `${spec.name}: out${b} inside out${a}`);
        const apart = outs.map((p, j) => (j === a ? view(p, buf, 0, N) : j === b ? view(p, buf, 8 * (N + 1), N) : fresh(p, N)));
        assert.deepEqual(call(spec, ins, opts, apart), ref, `${spec.name}: out${a} and out${b} apart in one buffer`);
        apart.forEach((x, j) => assert.deepEqual(bytes(x, ref.count), bytes(refOuts[j], ref.count), `${spec.name}: output ${j}`));
        pairs++;
      }
    }
  }
  assert.ok(pairs > 0);
});

test('an output sharing memory with an input it is not is TA_BAD_PARAM, and the input is left as it was', () => {
  const data = series(26);
  let rejected = 0;
  for (const spec of SPECS) {
    const ins = inputsOf(spec).map((p) => data[p.name]);
    const opts = sentinels(spec);
    outputsOf(spec).forEach((o, k) => {
      ins.forEach((_, i) => {
        const placements: [string, (buf: ArrayBuffer) => Arr][] = [['one element in', (buf) => view(o, buf, 8, N)]];
        if (!o.int32) placements.push(['same start, longer', (buf) => new Float64Array(buf, 0, N + 1)]);
        for (const [how, place] of placements) {
          const buf = new ArrayBuffer(8 * (N + 2));
          const shared = new Float64Array(buf, 0, N);
          shared.set(ins[i]);
          const aliasIns = ins.map((a, j) => (j === i ? shared : a));
          const outs = outputsOf(spec).map((p, j) => (j === k ? place(buf) : fresh(p, N)));
          throwsCode(() => call(spec, aliasIns, opts, outs), 'TA_BAD_PARAM', `${spec.name}: out${k} on in${i}, ${how}`);
          assert.deepEqual(shared, ins[i], `${spec.name}: out${k} on in${i}, ${how}: input written`);
          rejected++;
        }
      });
    });
  }
  assert.ok(rejected > 2 * SPECS.length, `only ${rejected} rejections`);
});

test('partial overlaps in BBANDS, MAVP and SUPERTREND are TA_BAD_PARAM; whole-array in place is not', () => {
  const n = 64;
  const src = series(27).inClose.slice(0, n);
  const big = new Float64Array(n + 40);

  throwsCode(
    () => lib.bbands(0, n - 1, src, 5, 2, 2, lib.MAType.SMA, big.subarray(0, n), big.subarray(10, 10 + n), new Float64Array(n)),
    'TA_BAD_PARAM',
    'outputs overlapping at an offset',
  );
  throwsCode(
    () => lib.bbands(0, n - 1, src, 5, 2, 2, lib.MAType.SMA, big.subarray(0, n), big.subarray(0, n + 1), new Float64Array(n)),
    'TA_BAD_PARAM',
    'outputs sharing a start but differing in length',
  );

  const buf = new Float64Array(n + 40);
  buf.set(src);
  throwsCode(
    () => lib.bbands(0, n - 1, buf.subarray(0, n), 5, 2, 2, lib.MAType.SMA, buf.subarray(20, 20 + n), new Float64Array(n), new Float64Array(n)),
    'TA_BAD_PARAM',
    'an output partially overlapping an input',
  );

  const per = Float64Array.from({ length: n }, (_, i) => 5 + (i % 10));
  const mbuf = new Float64Array(n + 40);
  mbuf.set(src);
  throwsCode(
    () => lib.mavp(0, n - 1, mbuf.subarray(0, n), per, 2, 20, lib.MAType.SMA, mbuf.subarray(15, 15 + n)),
    'TA_BAD_PARAM',
    'MAVP with a partially overlapping output',
  );

  const shared = new Float64Array(n);
  throwsCode(
    () => lib.supertrend(0, n - 1, src, src, src, 10, 3, shared, new Int32Array(shared.buffer)),
    'TA_BAD_PARAM',
    "SUPERTREND's int direction output on its real trend output",
  );
  throwsCode(
    () => lib.supertrend(0, n - 1, src, src, src, 10, 3, shared, new Int32Array(shared.buffer, 8, n)),
    'TA_BAD_PARAM',
    "SUPERTREND's int direction output inside its real trend output",
  );
  const st = lib.supertrend(0, n - 1, src, src, src, 10, 3, new Float64Array(n), new Int32Array(n));
  assert.ok(st.count > 0, 'disjoint mixed-type outputs');

  const reference = new Float64Array(n);
  const ref = lib.sma(0, n - 1, src, 5, reference);
  const inplace = src.slice();
  assert.deepEqual(lib.sma(0, n - 1, inplace, 5, inplace), ref, 'whole-array in place');
  assert.ok(ref.count > 0);
  assert.deepEqual(bytes(inplace, ref.count), bytes(reference, ref.count), 'in place is bit-identical to the disjoint call');

  const bb = lib.bbands(0, n - 1, src, 5, 2, 2, lib.MAType.SMA, new Float64Array(n), new Float64Array(n), new Float64Array(n));
  assert.ok(bb.count > 0, 'disjoint outputs');

  // A non-Float64Array input is copied before the body reads it, so an output
  // over its bytes is legal and computes from the caller's values.
  const fbuf = new ArrayBuffer(8 * n);
  const f32 = new Float32Array(fbuf, 0, n);
  f32.set(src);
  const expect = new Float64Array(n);
  const want = lib.sma(0, n - 1, Float64Array.from(f32), 5, expect);
  const over = new Float64Array(fbuf, 0, n);
  assert.deepEqual(lib.sma(0, n - 1, f32, 5, over), want, 'an output over a Float32Array input');
  assert.deepEqual(bytes(over, want.count), bytes(expect, want.count), 'over a Float32Array input: bit-identical');
});

test('a partial overlap is checked last: an index fault on the same call answers its own code', () => {
  const buf = series(28).inClose.slice(0, 300);
  const input = buf.subarray(0, 200);
  const out = buf.subarray(1, 201);
  throwsCode(() => lib.sma(-1, 199, input, 10, out), 'TA_OUT_OF_RANGE_START_INDEX', 'with a negative startIdx');
  throwsCode(() => lib.sma(50, 10, input, 10, out), 'TA_OUT_OF_RANGE_END_INDEX', 'with endIdx below startIdx');
  throwsCode(() => lib.sma(0, 199, input, 10, out), 'TA_BAD_PARAM', 'alone');
});

const NOT_INT32 = [1.5, -0.5, 2 ** 31, -(2 ** 31) - 1, Number.NaN, Number.POSITIVE_INFINITY, '7' as unknown as number];

test('a non-int32 integer parameter or index is TA_BAD_PARAM before any range check', () => {
  const data = series(23);
  let rejected = 0;
  for (const spec of SPECS) {
    const ins = inputsOf(spec).map((p) => data[p.name]);
    const opts = sentinels(spec);
    const outs = () => outputsOf(spec).map((p) => fresh(p, N));
    for (const bad of NOT_INT32) {
      throwsCode(() => spec.batch(bad, N - 1, ...ins, ...opts, ...outs()), 'TA_BAD_PARAM', `${spec.name}: startIdx ${String(bad)}`);
      throwsCode(() => spec.batch(0, bad, ...ins, ...opts, ...outs()), 'TA_BAD_PARAM', `${spec.name}: endIdx ${String(bad)}`);
      rejected += 2;
      optsOf(spec).forEach((p, k) => {
        if (p.kind === 'real') return;
        const v = opts.slice();
        v[k] = bad;
        throwsCode(() => spec.batch(0, N - 1, ...ins, ...v, ...outs()), 'TA_BAD_PARAM', `${spec.name}: ${p.name} ${String(bad)}`);
        throwsCode(() => spec.open(...ins, ...v), 'TA_BAD_PARAM', `${spec.name} open: ${p.name} ${String(bad)}`);
        rejected += 2;
      });
    }
    throwsCode(() => spec.batch(-1, N - 1, ...ins, ...opts, ...outs()), 'TA_OUT_OF_RANGE_START_INDEX', `${spec.name}: startIdx -1`);
    throwsCode(() => spec.batch(0, Core.INDEX_MAX + 1, ...ins, ...opts, ...outs()), 'TA_OUT_OF_RANGE_END_INDEX', `${spec.name}: endIdx past INDEX_MAX`);
  }
  assert.ok(rejected > 2 * NOT_INT32.length * SPECS.length);
});

const NOT_NUMBER = ['0.02', null, undefined, true] as unknown as number[];

test('a real parameter that is not a number is TA_BAD_PARAM, and its lookback and display shift reject it', () => {
  const data = series(25);
  let realParams = 0;
  let rejected = 0;
  for (const spec of SPECS) {
    const ins = inputsOf(spec).map((p) => data[p.name]);
    const opts = sentinels(spec);
    const outs = () => outputsOf(spec).map((p) => fresh(p, N));
    const shift = L[`${spec.name}DisplayShift`] as Fn;
    optsOf(spec).forEach((p, k) => {
      if (p.kind !== 'real') return;
      realParams++;
      for (const bad of NOT_NUMBER) {
        const v = opts.slice();
        v[k] = bad;
        const what = `${spec.name}: ${p.name} ${String(bad)}`;
        throwsCode(() => spec.batch(0, N - 1, ...ins, ...v, ...outs()), 'TA_BAD_PARAM', what);
        throwsCode(() => spec.open(...ins, ...v), 'TA_BAD_PARAM', `${what} (open)`);
        throwsCode(() => spec.fill(...ins, ...v, ...outs()), 'TA_BAD_PARAM', `${what} (openAndFill)`);
        assert.equal(spec.lookback(...v), -1, `${what} (lookback)`);
        assert.equal(shift(...v, 0), -2147483648, `${what} (display shift)`);
        rejected++;
      }
    });
  }
  assert.ok(realParams > 0 && SPECS.some((s) => s.name === 'sar' && optsOf(s).every((p) => p.kind === 'real')));
  assert.equal(rejected, realParams * NOT_NUMBER.length);
});

test('INT_MIN and -4e37 select the documented defaults, in the batch tier and at open', () => {
  const data = series(24);
  let compared = 0;
  for (const spec of SPECS) {
    const ins = inputsOf(spec).map((p) => data[p.name]);
    const explicit = optsOf(spec).map((p) => p.dflt as number);
    const viaSentinel = outputsOf(spec).map((p) => fresh(p, N));
    const viaValue = outputsOf(spec).map((p) => fresh(p, N));
    const a = call(spec, ins, sentinels(spec), viaSentinel);
    const b = call(spec, ins, explicit, viaValue);
    assert.deepEqual(a, b, `${spec.name}: range`);
    viaSentinel.forEach((x, j) => assert.deepEqual(bytes(x), bytes(viaValue[j]), `${spec.name}: output ${j}`));
    if (a.count === 0) continue;
    const fill = (opts: number[]): Arr[] | null => {
      const outs = outputsOf(spec).map((p) => fresh(p, a.count));
      try {
        (spec.fill as Fn)(...ins, ...opts, ...outs);
      } catch (e) {
        if (isBadParam(e)) return null;
        throw e;
      }
      return outs;
    };
    const fa = fill(sentinels(spec));
    const fb = fill(explicit);
    assert.equal(fa === null, fb === null, `${spec.name}: open accepts one form only`);
    fa?.forEach((x, j) => assert.deepEqual(bytes(x), bytes((fb as Arr[])[j]), `${spec.name} openAndFill: output ${j}`));
    compared++;
  }
  assert.ok(compared > 0);
});
