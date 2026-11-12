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

import { MAType } from '../src/enums.ts';
import { atr } from '../src/func/atr.ts';
import { bbands } from '../src/func/bbands.ts';
import { minmaxindex } from '../src/func/minmaxindex.ts';
import { sma, smaLookback } from '../src/func/sma.ts';
import { Core } from '../src/runtime/core.ts';
import { OutRange } from '../src/index.ts';
import { TALibError } from '../src/runtime/errors.ts';
import { FuncFlags, FunctionDescription, Functions, getRuntimeInfo, InputType, OptInputType } from '../src/metadata/index.ts';
import type { FuncInfo } from '../src/metadata/index.ts';

const N = 300;

function series(seed: number): Float64Array {
  const a = new Float64Array(N);
  let x = seed;
  let v = 100;
  for (let i = 0; i < N; i++) {
    x = (x * 1103515245 + 12345) % 2147483648;
    v += (x / 2147483648 - 0.5) * 2;
    a[i] = Math.round(v * 100) / 100;
  }
  return a;
}

const close = series(1);
const high = close.map((c, i) => c + 1 + (i % 3) * 0.25);
const low = close.map((c, i) => c - 1 - (i % 5) * 0.25);
const close2 = series(7);
const high2 = close2.map((c) => c + 2);
const low2 = close2.map((c) => c - 2);

function info(name: string): FuncInfo {
  const f = Functions.byName(name);
  assert.ok(f !== undefined, name);
  return f;
}

function rejects(code: string, fn: () => unknown): void {
  assert.throws(fn, (e: unknown) => e instanceof TALibError && e.code === code);
}

/** A public result's range, for comparison with the holder's `OutRange`. */
function rangeOf(begIdx: number, values: Float64Array | Int32Array): OutRange {
  return new OutRange(begIdx, values.length);
}

function bits(a: Float64Array | Int32Array): Uint8Array {
  return new Uint8Array(a.buffer, a.byteOffset, a.byteLength);
}

test('the registry enumerates every function once, name-sorted', () => {
  const all = Functions.all();
  assert.ok(all.length > 0);
  const names = all.map((f) => f.name);
  assert.deepEqual(names, [...names].sort());
  assert.equal(new Set(names).size, names.length);
  assert.deepEqual(Functions.groups(), [...new Set(all.map((f) => f.group))]);
  assert.ok(Object.isFrozen(all) && Object.isFrozen(all[0]) && Object.isFrozen(all[0].inputs));
});

test('byName folds ASCII case only', () => {
  assert.equal(Functions.byName('sma'), Functions.byName('SMA'));
  assert.equal(Functions.byName('Sma')?.name, 'SMA');
  assert.equal(Functions.byName('ſma'), undefined);
  assert.equal(Functions.byName('NOPE'), undefined);
});

test('rows describe the signature', () => {
  const b = info('BBANDS');
  assert.equal(b.inputs[0].type, InputType.REAL);
  const ma = b.optInputs[3];
  assert.equal(ma.type, OptInputType.INTEGER_LIST);
  assert.ok(ma.type === OptInputType.INTEGER_LIST && ma.values.some((v) => v.name === 'EMA' && v.value === MAType.EMA));
  assert.equal(info('ATR').inputs[0].type, InputType.PRICE);
  assert.ok(info('ATR').hasFlags(FuncFlags.UNSTABLE_PERIOD));
});

test('call is the public tier, bit for bit', () => {
  const s = new Float64Array(N);
  const d = new Float64Array(N);
  const rs = info('SMA').newCall().setInput(0, close).setOptInput(0, 20).setOutput(0, s).call(0, N - 1);
  const rd = sma(close, { startIdx: 0, endIdx: N - 1, timePeriod: 20, outReal: d });
  assert.deepEqual(rs, rangeOf(rd.begIdx, rd.values));
  assert.deepEqual(bits(s), bits(d));

  const hs = [new Float64Array(N), new Float64Array(N), new Float64Array(N)];
  const hd = [new Float64Array(N), new Float64Array(N), new Float64Array(N)];
  const h = info('BBANDS').newCall().setInput(0, close).setOptInput(0, 10).setOptInput(1, 1.5).setOptInput(3, MAType.EMA);
  hs.forEach((a, k) => h.setOutput(k, a));
  const rb = bbands(close, {
    startIdx: 5,
    endIdx: N - 1,
    timePeriod: 10,
    nbDevUp: 1.5,
    nbDevDn: Core.REAL_DEFAULT,
    maType: MAType.EMA,
    outUpperBand: hd[0],
    outMiddleBand: hd[1],
    outLowerBand: hd[2],
  });
  assert.deepEqual(h.call(5, N - 1), rangeOf(rb.begIdx, rb.outUpperBand));
  for (let k = 0; k < 3; k++) assert.deepEqual(bits(hs[k]), bits(hd[k]));

  const as = new Float64Array(N);
  const ad = new Float64Array(N);
  const ra = info('ATR').newCall().setPriceInput(0, { high, low, close }).setOutput(0, as).call(0, N - 1);
  const rt = atr({ high, low, close }, { startIdx: 0, endIdx: N - 1, timePeriod: Core.INTEGER_DEFAULT, outReal: ad });
  assert.deepEqual(ra, rangeOf(rt.begIdx, rt.values));
  assert.deepEqual(bits(as), bits(ad));

  const is = [new Int32Array(N), new Int32Array(N)];
  const id = [new Int32Array(N), new Int32Array(N)];
  const rm = info('MINMAXINDEX').newCall().setInput(0, close).setOutput(0, is[0]).setOutput(1, is[1]).call(0, N - 1);
  const rx = minmaxindex(close, { startIdx: 0, endIdx: N - 1, timePeriod: Core.INTEGER_DEFAULT, outMinIdx: id[0], outMaxIdx: id[1] });
  assert.deepEqual(rm, rangeOf(rx.begIdx, rx.outMinIdx));
  assert.deepEqual(bits(is[0]), bits(id[0]));
  assert.deepEqual(bits(is[1]), bits(id[1]));
});

test('a public-tier rejection surfaces as the direct call does', () => {
  const h = info('SMA').newCall().setInput(0, close).setOutput(0, new Float64Array(N)).setOptInput(0, 0);
  rejects('TA_BAD_PARAM', () => h.call(0, N - 1));
  rejects('TA_BAD_PARAM', () => info('SMA').newCall().setInput(0, close.subarray(0, 10)).setOutput(0, new Float64Array(N)).call(0, N - 1));
  rejects('TA_OUT_OF_RANGE_END_INDEX', () => info('SMA').newCall().setInput(0, close).setOutput(0, new Float64Array(N)).call(5, 4));
});

test('lookback and displayShift follow the bound parameters', () => {
  const h = info('SMA').newCall();
  assert.equal(h.lookback(), smaLookback({ timePeriod: Core.INTEGER_DEFAULT }));
  h.setOptInput(0, 7);
  assert.equal(h.lookback(), 6);
  h.setOptInput(0, 100001);
  assert.equal(h.lookback(), -1);
  assert.equal(info('SMA').newCall().displayShift(0), 0);
  assert.equal(info('SMA').newCall().displayShift(1), -2147483648);
});

test('a rejected price re-bind leaves the previous bundle whole', () => {
  const run = (h: ReturnType<FuncInfo['newCall']>): Uint8Array => {
    const out = new Float64Array(N);
    h.setOutput(0, out).call(0, N - 1);
    return bits(out);
  };
  const h = info('ATR').newCall().setPriceInput(0, { high, low, close });
  const first = run(h);
  rejects('TA_BAD_PARAM', () => h.setPriceInput(0, { high: high2, close: close2 }));
  rejects('TA_BAD_PARAM', () => h.setPriceInput(0, { high: high2, low: low2, close: 5 as unknown as Float64Array }));
  assert.deepEqual(run(h), first);
  h.setPriceInput(0, { high: high2, low: low2, close: close2 });
  const direct = new Float64Array(N);
  atr({ high: high2, low: low2, close: close2 }, { startIdx: 0, endIdx: N - 1, timePeriod: Core.INTEGER_DEFAULT, outReal: direct });
  assert.deepEqual(run(h), bits(direct));
});

test('every other rejected setter leaves the holder as it found it', () => {
  const out = new Float64Array(N);
  const h = info('SMA').newCall().setInput(0, close).setOptInput(0, 9).setOutput(0, out);
  rejects('TA_BAD_PARAM', () => h.setOptInput(0, 2.5));
  rejects('TA_BAD_PARAM', () => h.setOptInput(0, 2 ** 31));
  rejects('TA_BAD_PARAM', () => h.setOptInput(1, 3));
  rejects('TA_BAD_PARAM', () => h.setInput(0, null as unknown as Float64Array));
  rejects('TA_BAD_PARAM', () => h.setInput(1, close));
  rejects('TA_INVALID_PARAM_HOLDER_TYPE', () => h.setOutput(0, new Int32Array(N)));
  rejects('TA_BAD_PARAM', () => h.setOutput(0, null as unknown as Float64Array));
  assert.equal(h.lookback(), 8);
  const d = new Float64Array(N);
  const r9 = sma(close, { startIdx: 0, endIdx: N - 1, timePeriod: 9, outReal: d });
  assert.deepEqual(h.call(0, N - 1), rangeOf(r9.begIdx, r9.values));
  assert.deepEqual(bits(out), bits(d));

  rejects('TA_INVALID_PARAM_HOLDER_TYPE', () => info('ATR').newCall().setInput(0, close));
  rejects('TA_INVALID_PARAM_HOLDER_TYPE', () => info('SMA').newCall().setPriceInput(0, { close }));
  rejects('TA_INVALID_PARAM_HOLDER_TYPE', () => info('MINMAXINDEX').newCall().setOutput(0, new Float64Array(N)));
  rejects('TA_BAD_PARAM', () => info('SMA').newCall({} as Core));
});

test('call refuses an unbound input or output', () => {
  rejects('TA_INPUT_NOT_ALL_INITIALIZE', () => info('SMA').newCall().setOutput(0, new Float64Array(N)).call(0, N - 1));
  rejects('TA_OUTPUT_NOT_ALL_INITIALIZE', () => info('SMA').newCall().setInput(0, close).call(0, N - 1));
});

test('getRuntimeInfo and FunctionDescription', () => {
  assert.equal(getRuntimeInfo('vmath.transcendental'), 0);
  // No entry with an engine is imported here: it is not loaded.
  assert.equal(getRuntimeInfo('wasm.engine'), 0);
  assert.equal(getRuntimeInfo('wasm.reason'), 1);
  rejects('TA_BAD_PARAM', () => getRuntimeInfo('no.such.key'));
  const xml = FunctionDescription.xml();
  assert.ok(xml.startsWith('<?xml'));
  assert.ok(xml.includes('<Abbreviation>SMA</Abbreviation>'));
});

test('call defaults its range as the typed call does', () => {
  const whole = new Float64Array(N);
  const explicit = new Float64Array(N);
  const r = info('SMA').newCall().setInput(0, close).setOptInput(0, 20).setOutput(0, whole).call();
  const e = info('SMA').newCall().setInput(0, close).setOptInput(0, 20).setOutput(0, explicit).call(0, N - 1);
  assert.deepEqual([r.begIdx, r.count], [e.begIdx, e.count]);
  assert.deepEqual(Array.from(whole), Array.from(explicit));
  const from25 = info('SMA').newCall().setInput(0, close).setOptInput(0, 20).setOutput(0, new Float64Array(N)).call(25);
  assert.deepEqual([from25.begIdx, from25.count], [25, N - 25]);
  // The default reads the input's length, so an empty one is the typed call's rejection, not a range fault.
  rejects('TA_BAD_PARAM', () => info('SMA').newCall().setInput(0, new Float64Array(0)).setOutput(0, new Float64Array(1)).call());
  const priced = info('ATR').newCall().setPriceInput(0, { high, low, close }).setOutput(0, new Float64Array(N)).call();
  assert.deepEqual([priced.begIdx, priced.count], [14, N - 14]);
});

test('rows carry the typed call keys, and setOptInput takes the key as well as the position', () => {
  const s = info('SMA');
  assert.deepEqual([s.inputs[0].key, s.optInputs[0].key, s.outputs[0].key], ['real', 'timePeriod', 'values']);
  const st = info('STOCH');
  assert.equal(st.inputs[0].key, 'prices');
  assert.deepEqual(st.optInputs.map((o) => o.key), ['fastKPeriod', 'slowKPeriod', 'slowKMaType', 'slowDPeriod', 'slowDMaType']);
  assert.deepEqual(st.outputs.map((o) => o.key), ['outSlowK', 'outSlowD']);
  assert.deepEqual(info('MAVP').inputs.map((i) => i.key), ['real', 'periods']);
  assert.deepEqual(info('BETA').inputs.map((i) => i.key), ['real0', 'real1']);
  const byKey = new Float64Array(N);
  const byIndex = new Float64Array(N);
  const rk = info('SMA').newCall().setInput(0, close).setOptInput('timePeriod', 20).setOutput(0, byKey).call();
  const ri = info('SMA').newCall().setInput(0, close).setOptInput(0, 20).setOutput(0, byIndex).call();
  assert.deepEqual([rk.begIdx, rk.count], [ri.begIdx, ri.count]);
  assert.deepEqual(Array.from(byKey), Array.from(byIndex));
  rejects('TA_BAD_PARAM', () => info('SMA').newCall().setOptInput('timeperiod', 20));
  rejects('TA_BAD_PARAM', () => info('SMA').newCall().setOptInput('optInTimePeriod', 20));
});
