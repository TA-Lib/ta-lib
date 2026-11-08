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

import assert from 'node:assert/strict';
import { test } from 'node:test';

import { Core, CoreBuilder, FuncUnstId, OutRange, TALibError, atr, cdldoji, ema, macd, sma, smaLookback, smaOpen } from 'ta-lib';

const close = Float64Array.from([1, 2, 3, 4, 5, 6, 7, 8, 9, 10]);

test('sma through the package entry point', () => {
  const out = new Float64Array(close.length);
  const r = sma(close, { timePeriod: 3, outReal: out });
  assert.deepEqual(Object.keys(r).sort(), ['begIdx', 'values']);
  assert.ok(r.values instanceof Float64Array && r.values.buffer === out.buffer && r.values.byteOffset === 0, 'values is a view of outReal');
  assert.equal(smaLookback({ timePeriod: 3 }), 2);
  assert.equal(r.begIdx, 2);
  assert.equal(r.values.length, 8);
  assert.deepEqual(Array.from(out.subarray(0, r.values.length)), [2, 3, 4, 5, 6, 7, 8, 9]);
  assert.deepEqual(Array.from(sma(close, { timePeriod: 3 }).values), [2, 3, 4, 5, 6, 7, 8, 9]);
  const s = smaOpen(close, { timePeriod: 3 }).outRange();
  assert.ok(s instanceof OutRange);
  assert.deepEqual([s.begIdx, s.count], [2, 8]);
});

test('a number[] input gives the Float64Array bits', () => {
  const a = new Float64Array(close.length);
  const b = new Float64Array(close.length);
  sma(close, { timePeriod: 4, outReal: a });
  sma(Array.from(close), { timePeriod: 4, outReal: b });
  assert.deepEqual(b, a);
});

test('a rejected argument throws TALibError with a stable code', () => {
  assert.throws(() => sma(close, { timePeriod: 0, outReal: new Float64Array(close.length) }), (e: unknown) => {
    assert.ok(e instanceof TALibError);
    assert.equal(e.code, 'TA_BAD_PARAM');
    return true;
  });
});

test('a Core built with an unstable period changes where the output starts', () => {
  const core = new CoreBuilder().unstablePeriod(FuncUnstId.EMA, 3).build();
  const a = new Float64Array(close.length);
  const b = new Float64Array(close.length);
  const plain = ema(close, { timePeriod: 3, outReal: a, core: Core.DEFAULT });
  const tuned = ema(close, { timePeriod: 3, outReal: b, core });
  assert.equal(tuned.begIdx, plain.begIdx + 3);
});

test('every output is an ArrayBuffer-backed typed array, by type and in fact', () => {
  // Compiles only while the result fields are Float64Array<ArrayBuffer> / Int32Array<ArrayBuffer>.
  const allocated: ArrayBuffer = sma(close, { timePeriod: 3 }).values.buffer;
  assert.ok(allocated instanceof ArrayBuffer && !(allocated instanceof SharedArrayBuffer));
  const own = new Float64Array(close.length);
  const view: Float64Array<ArrayBuffer> = sma(close, { timePeriod: 3, outReal: own }).values;
  assert.equal(view.buffer, own.buffer);
  const multi: ArrayBuffer = macd(close).outMacd.buffer;
  assert.ok(multi instanceof ArrayBuffer);
  const bars = { open: close, high: close, low: close, close };
  const pattern: ArrayBuffer = cdldoji(bars).values.buffer;
  assert.ok(pattern instanceof ArrayBuffer);
  const shared = new Float64Array(new SharedArrayBuffer(8 * close.length));
  // @ts-expect-error a buffer over a SharedArrayBuffer is not an outReal: the type is the rule, the call does not inspect it
  const r = sma(close, { timePeriod: 3, outReal: shared });
  assert.equal(r.values.length, 8);
});

test('a price object may carry more components than the function reads', () => {
  // An inline literal, so the excess-property check runs: the unread components must be declared, not merely tolerated.
  const full = atr({ open: close, high: close, low: close, close, volume: close }, { timePeriod: 3 });
  const needed = atr({ high: close, low: close, close }, { timePeriod: 3 });
  // @ts-expect-error a misspelt component is still rejected at compile time
  atr({ high: close, low: close, close, hgih: close }, { timePeriod: 3 });
  assert.deepEqual(Array.from(full.values), Array.from(needed.values));
  assert.equal(full.begIdx, needed.begIdx);
  // @ts-expect-error a component the function reads is still required, and absent at run time it is TA_BAD_PARAM
  assert.throws(() => atr({ high: close, low: close }, { timePeriod: 3 }), (e: unknown) => e instanceof TALibError && e.code === 'TA_BAD_PARAM');
});
