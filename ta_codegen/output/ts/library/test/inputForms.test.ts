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

// D5/D7: any ArrayLike input gives exactly the bits its Float64Array form
// gives, in the batch tier and in OpenAndFill, on every function.

import assert from 'node:assert/strict';
import { test } from 'node:test';

import { type Arr, isBadParam, N, type Spec, series, specs } from './corpus.ts';
import { Core } from '../src/index.ts';

const SPECS = specs();

type Result = { range: string; outs: Arr[] };

function poisoned(int32: boolean, n: number): Arr {
  return int32 ? new Int32Array(n).fill(-7) : new Float64Array(n).fill(Number.NaN);
}

function bytes(a: Arr): Uint8Array {
  return new Uint8Array(a.buffer, a.byteOffset, a.byteLength);
}

function batch(spec: Spec, ins: ArrayLike<number>[], opts: number[]): Result | null {
  const outs = spec.params.filter((p) => p.kind === 'output').map((p) => poisoned(p.int32, N));
  try {
    const r = spec.batch(0, N - 1, ...ins, ...opts, ...outs) as { begIdx: number; count: number };
    return { range: `${r.begIdx}+${r.count}`, outs };
  } catch (e) {
    if (isBadParam(e)) return null;
    throw e;
  }
}

function fill(spec: Spec, ins: ArrayLike<number>[], opts: number[], lb: number): Result | null {
  const outs = spec.params.filter((p) => p.kind === 'output').map((p) => poisoned(p.int32, N - lb));
  try {
    const s = spec.fill(...ins, ...opts, ...outs) as { outRange(): { begIdx: number; count: number } };
    const r = s.outRange();
    return { range: `${r.begIdx}+${r.count}`, outs };
  } catch (e) {
    if (isBadParam(e)) return null;
    throw e;
  }
}

function sameBits(spec: Spec, what: string, got: Result | null, want: Result | null): void {
  assert.ok(want !== null && got !== null, `${spec.name} ${what}: rejected`);
  assert.equal(got.range, want.range, `${spec.name} ${what}: range`);
  got.outs.forEach((o, j) => {
    assert.deepEqual(bytes(o), bytes(want.outs[j]), `${spec.name} ${what}: output ${j} differs in its bits`);
  });
}

function vectors(spec: Spec): number[][] {
  const opts = spec.params.filter((p) => p.kind !== 'input' && p.kind !== 'output');
  const dflt = opts.map((p) => (p.kind === 'real' ? Core.REAL_DEFAULT : Core.INTEGER_DEFAULT));
  const small = opts.map((p, k) => (p.kind === 'int' ? 3 : dflt[k]));
  return opts.some((p) => p.kind === 'int') ? [dflt, small] : [dflt];
}

test('number[], numeric strings and Float32Array give the Float64Array bits on every function', () => {
  const data = series(11);
  const covered = new Set<string>();
  let filled = 0;
  for (const spec of SPECS) {
    const f64 = spec.params.filter((p) => p.kind === 'input').map((p) => data[p.name]);
    const numbers = f64.map((a) => Array.from(a));
    const strings = f64.map((a) => Array.from(a, (x) => String(x)) as unknown as number[]);
    const f32 = f64.map((a) => Float32Array.from(a));
    const widened = f32.map((a) => Float64Array.from(a));
    for (const opts of vectors(spec)) {
      const want = batch(spec, f64, opts);
      if (want === null) continue;
      sameBits(spec, 'number[]', batch(spec, numbers, opts), want);
      sameBits(spec, 'numeric strings', batch(spec, strings, opts), want);
      sameBits(spec, 'Float32Array', batch(spec, f32, opts), batch(spec, widened, opts));
      if (Number(want.range.split('+')[1]) > 0) covered.add(spec.name);

      const lb = spec.lookback(...opts) as number;
      const wantFill = fill(spec, f64, opts, lb);
      if (wantFill === null) continue;
      sameBits(spec, 'openAndFill numeric strings', fill(spec, strings, opts, lb), wantFill);
      sameBits(spec, 'openAndFill Float32Array', fill(spec, f32, opts, lb), fill(spec, widened, opts, lb));
      filled++;
    }
  }
  assert.deepEqual(SPECS.map((s) => s.name).filter((n) => !covered.has(n)), [], 'functions with no produced value');
  assert.ok(filled >= SPECS.length, `only ${filled} OpenAndFill comparisons`);
});
