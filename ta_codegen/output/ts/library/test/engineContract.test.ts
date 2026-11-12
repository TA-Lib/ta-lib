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

type Checked = { fellThrough: number; ran: number; got: Outcome };

/** Both calls' outcomes compared; returns the fall-throughs the ta-lib call made, the batch calls the module counted, and its outcome. */
function check(where: string, viaDefault: () => unknown, viaJs: () => unknown): Checked {
  hooks.setEngineMode('force');
  const before = hooks.fallThroughCount();
  const entered = hooks.moduleCalls();
  let got: Outcome;
  try {
    got = outcome(viaDefault);
  } finally {
    hooks.setEngineMode('table');
  }
  const fellThrough = hooks.fallThroughCount() - before;
  const ran = hooks.moduleCalls() - entered;
  same(got, outcome(viaJs), where);
  return { fellThrough, ran, got };
}

function ranInWasm(r: Checked, where: string): void {
  assert.equal(r.fellThrough, 0, `${where}: fell through`);
  assert.equal(r.ran, 1, `${where}: the module counted ${r.ran} batch calls, not one`);
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
  ranInWasm(inRange, '10..200');
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
  ranInWasm(check('number[]', () => ema(list), () => js.ema(list)), 'number[]');
  ranInWasm(check('Float32Array', () => ema(narrow), () => js.ema(narrow)), 'Float32Array');
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
  ranInWasm(check('Proxy over number[]', () => ema(overList), () => js.ema(overList)), 'Proxy over number[]');
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
  assert.equal(r.ran, 2, 'the module did not count the re-entrant call and the outer one');
});

test('a Float64Array whose length property claims more elements than it has falls through', () => {
  check('prime', () => ema(new Float64Array(400).fill(12345.5)), () => js.ema(new Float64Array(400).fill(12345.5)));
  const a = Float64Array.from({ length: 10 }, (_, i) => i + 1);
  Object.defineProperty(a, 'length', { value: 100 });
  const r = check('length 100 over 10', () => routed.sma(a, { timePeriod: 5 }), () => js.sma(a, { timePeriod: 5 }));
  assert.equal(r.fellThrough, 1);
});

test('a Float64Array whose length claims fewer elements than it has falls through, and later calls are unharmed', () => {
  const b = new Float64Array(100_000).fill(-1e300);
  Object.defineProperty(b, 'length', { value: 10 });
  const r = check('length 10 over 100000', () => routed.sma(b, { timePeriod: 3 }), () => js.sma(b, { timePeriod: 3 }));
  assert.equal(r.fellThrough, 1);
  class Short extends Float64Array {}
  Object.defineProperty(Short.prototype, 'length', { get: () => 10 });
  const c = Short.from(close);
  assert.equal(check('subclass length getter', () => routed.sma(c, { timePeriod: 3 }), () => js.sma(c, { timePeriod: 3 })).fellThrough, 1);
  ranInWasm(check('after', () => macd(close), () => js.macd(close)), 'after');
});

test('the routed path accepts exactly the options objects ta-lib/js accepts', () => {
  class Options {
    timePeriod = 9;
  }
  const shapes: [string, unknown][] = [
    ['plain object', { timePeriod: 9 }],
    ['null-prototype object', Object.assign(Object.create(null) as object, { timePeriod: 9 })],
    ['class instance', new Options()],
    ['frozen object', Object.freeze({ timePeriod: 9 })],
    ['array', [9]],
    ['typed array', new Float64Array(3)],
    ['null', null],
    ['function', () => 9],
    ['Proxy', new Proxy({ timePeriod: 9 }, {})],
  ];
  let accepted = 0;
  for (const [where, options] of shapes) {
    const opts = options as Parameters<typeof ema>[1];
    const r = check(where, () => ema(close, opts), () => js.ema(close, opts));
    const jsAccepts = r.got.error === undefined;
    assert.equal(hooks.routedOptions(opts) !== null, jsAccepts, `${where}: the routed predicate disagrees with ta-lib/js`);
    if (jsAccepts) ranInWasm(r, where);
    else assert.equal(r.fellThrough, 1, where);
    if (jsAccepts) accepted++;
  }
  assert.equal(accepted, 5);
});

test('an instance past the high-water mark is dropped after its call, and the next call still answers in WASM', () => {
  ranInWasm(check('warm-up', () => ema(close), () => js.ema(close)), 'warm-up');
  assert.ok(hooks.wasmMemoryBytes() > 0);
  const previous = hooks.setHighWater(0);
  try {
    for (const where of ['past the mark', 'after the reset']) {
      ranInWasm(check(where, () => ema(close, { timePeriod: 12 }), () => js.ema(close, { timePeriod: 12 })), where);
      assert.equal(hooks.wasmMemoryBytes(), 0, `${where}: the instance was kept`);
    }
  } finally {
    hooks.setHighWater(previous);
  }
  ranInWasm(check('below the mark', () => ema(close), () => js.ema(close)), 'below the mark');
  assert.ok(hooks.wasmMemoryBytes() > 0);
  assert.deepEqual(hooks.engineStatus(), { engine: 'wasm', reason: null });
});

test('the two entries export the same names, and only the default entry has ready', async () => {
  const names = Object.keys(js).sort();
  assert.deepEqual(Object.keys(routed).sort(), [...names, 'ready'].sort());
  assert.ok(!names.includes('ready'));
  assert.deepEqual(await ready(), { engine: 'wasm', reason: null });
});
