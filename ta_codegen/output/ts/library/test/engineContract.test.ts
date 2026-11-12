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

import * as routed from 'ta-lib';
import { beta, CoreBuilder, ema, FuncUnstId, ma, macd, MAType, ready, TALibError } from 'ta-lib';
import * as js from 'ta-lib/js';
import { hooks } from './engineHooks.ts';

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
  assert.deepEqual(Object.keys(a), Object.keys(b), where);
  assert.equal(a.begIdx, b.begIdx, `${where}: begIdx`);
  for (const key of Object.keys(b).filter((k) => k !== 'begIdx')) {
    const x = a[key] as Float64Array;
    const y = b[key] as Float64Array;
    assert.equal(x.constructor, y.constructor, `${where}: ${key} type`);
    assert.ok(bits(x).equals(bits(y)), `${where}: ${key} differs`);
  }
}

/** Both calls' outcomes compared; returns the fall-throughs the ta-lib call made, and its outcome. */
function check(where: string, viaDefault: () => unknown, viaJs: () => unknown): { fellThrough: number; got: Outcome } {
  hooks.setEngineMode('force');
  const before = hooks.fallThroughCount();
  let got: Outcome;
  try {
    got = outcome(viaDefault);
  } finally {
    hooks.setEngineMode('table');
  }
  const fellThrough = hooks.fallThroughCount() - before;
  same(got, outcome(viaJs), where);
  return { fellThrough, got };
}

test('a caller-supplied output buffer falls through, and the result is a view of it', () => {
  const mine = new Float64Array(N);
  const r = check('outReal', () => ema(close, { outReal: mine }), () => js.ema(close, { outReal: new Float64Array(N) }));
  assert.equal(r.fellThrough, 1);
  assert.equal((r.got.value?.values as Float64Array).buffer, mine.buffer);
});

test('one buffer for two outputs is rejected with the ta-lib/js error', () => {
  const shared = new Float64Array(N);
  const r = check(
    'outMacd = outMacdSignal',
    () => macd(close, { outMacd: shared, outMacdSignal: shared }),
    () => js.macd(close, { outMacd: shared, outMacdSignal: shared }),
  );
  assert.equal(r.fellThrough, 1);
  assert.ok(r.got.error instanceof TALibError);
});

test('a Core other than Core.DEFAULT falls through', () => {
  const core = new CoreBuilder().unstablePeriod(FuncUnstId.EMA, 7).build();
  const r = check('core', () => ema(close, { timePeriod: 10, core }), () => js.ema(close, { timePeriod: 10, core }));
  assert.equal(r.fellThrough, 1);
});

test('startIdx and endIdx are routed; out of range they are rejected as ta-lib/js rejects them', () => {
  const inRange = check('10..200', () => ema(close, { startIdx: 10, endIdx: 200 }), () => js.ema(close, { startIdx: 10, endIdx: 200 }));
  assert.equal(inRange.fellThrough, 0);
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

test('a number[] and a Float32Array are routed and give the bits of ta-lib/js', () => {
  const list = Array.from(close);
  const narrow = Float32Array.from(close);
  assert.equal(check('number[]', () => ema(list), () => js.ema(list)).fellThrough, 0);
  assert.equal(check('Float32Array', () => ema(narrow), () => js.ema(narrow)).fellThrough, 0);
});

test('MA type 7 (MAMA) falls through to ta-lib/js; 99 is rejected as ta-lib/js rejects it', () => {
  assert.equal(MAType.MAMA, 7);
  const mama = check('maType 7', () => ma(close, { maType: MAType.MAMA }), () => js.ma(close, { maType: MAType.MAMA }));
  assert.equal(mama.fellThrough, 1);
  assert.equal(mama.got.error, undefined);
  const bad = 99 as MAType;
  const r = check('maType 99', () => ma(close, { maType: bad }), () => js.ma(close, { maType: bad }));
  assert.equal(r.fellThrough, 1);
  assert.ok(r.got.error instanceof TALibError);
});

test('a Proxy input, and an element whose valueOf grows WASM memory by a re-entrant call, give the ta-lib/js result', () => {
  const overList = new Proxy(Array.from(close), {});
  assert.equal(check('Proxy over number[]', () => ema(overList), () => js.ema(overList)).fellThrough, 0);
  const target = close.slice();
  const overTyped = new Proxy(target, {
    get: (t, key) => {
      const v: unknown = Reflect.get(t, key);
      return typeof v === 'function' ? v.bind(t) : v;
    },
  });
  const forwarded = check('Proxy over Float64Array', () => ema(overTyped), () => js.ema(overTyped));
  assert.equal(forwarded.fellThrough, 1);
  assert.equal(forwarded.got.error, undefined);
  const bare = new Proxy(close.slice(), {});
  assert.equal(check('Proxy over Float64Array, no traps', () => ema(bare), () => js.ema(bare)).fellThrough, 1);

  check('warm-up', () => ema(close), () => js.ema(close));
  const before = hooks.wasmMemoryBytes();
  assert.ok(before > 0, 'no live instance after a routed call');
  const big = new Float64Array(4_000_000).fill(1);
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
  const r = check('valueOf grows memory', () => ema(series), () => js.ema(series));
  assert.ok(grown > before, `memory ${before} -> ${grown}: the re-entrant call did not grow it`);
  assert.equal(r.fellThrough, 0);
});

test('the two entries export the same names, and only the default entry has ready', async () => {
  const names = Object.keys(js).sort();
  assert.deepEqual(Object.keys(routed).sort(), [...names, 'ready'].sort());
  assert.ok(!names.includes('ready'));
  assert.deepEqual(await ready(), { engine: 'wasm', reason: null });
});
