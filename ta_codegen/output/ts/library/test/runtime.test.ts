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
 *  110126 MF,CC  First Version
 */

import assert from 'node:assert/strict';
import { test } from 'node:test';

import { FuncUnstId, MAType } from '../src/enums.ts';
import { cdldoji, cdldojiLookback } from '../src/func/cdldoji.ts';
import { ema, emaLookback } from '../src/func/ema.ts';
import { CandleSetting, CandleSettingType, Core, CoreBuilder, RangeType } from '../src/runtime/core.ts';
import { RetCode, TALibError } from '../src/runtime/errors.ts';
import * as flags from '../src/metadata/flags.ts';
import { OutRange } from '../src/runtime/outRange.ts';

function assertBadParam(fn: () => unknown): void {
  assert.throws(fn, (e: unknown) => {
    assert.ok(e instanceof TALibError);
    assert.equal(e.name, 'TALibError');
    assert.equal(e.code, 'TA_BAD_PARAM');
    assert.equal(e.retCode, 2);
    return true;
  });
}

test('RetCode numbers are C TA_RetCode', () => {
  assert.deepEqual(RetCode, {
    SUCCESS: 0,
    BAD_PARAM: 2,
    ALLOC_ERR: 3,
    INVALID_HANDLE: 6,
    INVALID_PARAM_HOLDER_TYPE: 8,
    INPUT_NOT_ALL_INITIALIZE: 10,
    OUTPUT_NOT_ALL_INITIALIZE: 11,
    OUT_OF_RANGE_START_INDEX: 12,
    OUT_OF_RANGE_END_INDEX: 13,
    INSUFFICIENT_HISTORY: 17,
    INTERNAL_ERROR: 5000,
  });
});

test('TALibError carries code, retCode and message', () => {
  for (const [name, value] of Object.entries(RetCode)) {
    if (value === RetCode.SUCCESS) continue;
    const e = new TALibError(value, 'SMA: msg');
    assert.ok(e instanceof Error);
    assert.equal(e.code, `TA_${name}`);
    assert.equal(e.retCode, value);
    assert.equal(e.message, 'SMA: msg');
    assert.equal(e.name, 'TALibError');
    assert.match(String(e), /^TALibError: SMA: msg$/);
  }
});

test('sentinels match C', () => {
  assert.equal(Core.INTEGER_DEFAULT, -(2 ** 31));
  assert.equal(Core.INTEGER_MIN, -(2 ** 31) + 1);
  assert.equal(Core.INTEGER_MAX, 2 ** 31 - 1);
  assert.equal(Core.REAL_DEFAULT, -4e37);
  assert.equal(Core.REAL_MIN, -3e37);
  assert.equal(Core.REAL_MAX, 3e37);
  assert.equal(Core.INDEX_MAX, 100000000);
  assert.equal(Core.UNSTABLE_AUTO_PREC_4, 100000004);
  assert.equal(Core.UNSTABLE_AUTO_PREC_8, 100000008);
});

test('Core.DEFAULT has zero unstable periods and the C candle defaults', () => {
  const d = Core.DEFAULT;
  assert.equal(d.unstablePeriods.length, 37);
  assert.ok(d.unstablePeriods.every((p) => p === 0));
  const expected: [number, number, number][] = [
    [RangeType.REAL_BODY, 10, 1.0],
    [RangeType.REAL_BODY, 10, 3.0],
    [RangeType.REAL_BODY, 10, 1.0],
    [RangeType.HIGH_LOW, 10, 0.1],
    [RangeType.REAL_BODY, 0, 1.0],
    [RangeType.REAL_BODY, 0, 2.0],
    [RangeType.SHADOWS, 10, 1.0],
    [RangeType.HIGH_LOW, 10, 0.1],
    [RangeType.HIGH_LOW, 5, 0.2],
    [RangeType.HIGH_LOW, 5, 0.6],
    [RangeType.HIGH_LOW, 5, 0.05],
  ];
  assert.equal(d.candleSettings.length, expected.length);
  d.candleSettings.forEach((s, i) => {
    assert.equal(s.settingType, i);
    assert.deepEqual([s.rangeType, s.avgPeriod, s.factor], expected[i]);
  });
});

test('unstablePeriod sets one id, ALL sets every id, and Auto levels resolve', () => {
  const one = new CoreBuilder().unstablePeriod(FuncUnstId.EMA, 7).build();
  assert.equal(one.unstablePeriod(FuncUnstId.EMA), 7);
  assert.equal(one.unstablePeriod(FuncUnstId.ADX), 0);
  assert.equal(one.unstablePeriod(FuncUnstId.FISHER), 0);

  const all = Core.builder().unstablePeriod(FuncUnstId.ALL, 3).build();
  assert.ok(all.unstablePeriods.every((p) => p === 3));

  const auto = new CoreBuilder()
    .unstablePeriod(FuncUnstId.RSI, Core.UNSTABLE_AUTO_PREC_4)
    .unstablePeriod(FuncUnstId.EMA, Core.UNSTABLE_AUTO_PREC_8)
    .unstablePeriod(FuncUnstId.ATR, Core.INDEX_MAX)
    .build();
  assert.equal(auto.unstableCount(FuncUnstId.RSI, 40, 80), 40);
  assert.equal(auto.unstableCount(FuncUnstId.EMA, 40, 80), 80);
  assert.equal(auto.unstableCount(FuncUnstId.ATR, 40, 80), Core.INDEX_MAX);
  assert.equal(auto.unstableCount(FuncUnstId.ADX, 40, 80), 0);
});

test('unstablePeriod rejects out-of-domain periods and ids, writing nothing', () => {
  const b = new CoreBuilder().unstablePeriod(FuncUnstId.EMA, 5);
  for (const bad of [-1, Core.INDEX_MAX + 1, Core.UNSTABLE_AUTO_PREC_4 + 1, 1.5, NaN, Infinity]) {
    assertBadParam(() => b.unstablePeriod(FuncUnstId.EMA, bad));
    assertBadParam(() => b.unstablePeriod(FuncUnstId.ALL, bad));
  }
  assertBadParam(() => b.unstablePeriod(FuncUnstId.EMA, '3' as unknown as number));
  for (const bad of [-1, 37, 65534, 2.5, NaN]) {
    assertBadParam(() => b.unstablePeriod(bad as FuncUnstId, 1));
  }
  const core = b.build();
  assert.equal(core.unstablePeriod(FuncUnstId.EMA), 5);
  assert.equal(core.unstablePeriods.reduce((a, p) => a + p, 0), 5);
  assertBadParam(() => core.unstablePeriod(FuncUnstId.ALL));
  assertBadParam(() => core.unstablePeriod(37 as FuncUnstId));
});

test('candleSetting validates every argument, writing nothing on a rejection', () => {
  const b = new CoreBuilder();
  const T = CandleSettingType;
  assertBadParam(() => b.candleSetting(T.ALL_CANDLE_SETTINGS, RangeType.HIGH_LOW, 5, 1));
  assertBadParam(() => b.candleSetting(-1 as CandleSettingType, RangeType.HIGH_LOW, 5, 1));
  assertBadParam(() => b.candleSetting(12 as CandleSettingType, RangeType.HIGH_LOW, 5, 1));
  assertBadParam(() => b.candleSetting(T.NEAR, 3 as RangeType, 5, 1));
  assertBadParam(() => b.candleSetting(T.NEAR, -1 as RangeType, 5, 1));
  for (const bad of [-1, Core.INDEX_MAX + 1, 2.5, NaN]) {
    assertBadParam(() => b.candleSetting(T.NEAR, RangeType.HIGH_LOW, bad, 1));
  }
  for (const bad of [-0.1, NaN, Infinity, -Infinity, '1' as unknown as number]) {
    assertBadParam(() => b.candleSetting(T.NEAR, RangeType.HIGH_LOW, 5, bad));
  }
  assert.deepEqual(b.build().candleSettings, Core.DEFAULT.candleSettings);

  const core = b.candleSetting(T.NEAR, RangeType.REAL_BODY, Core.INDEX_MAX, 0).build();
  const s = core.candleSettings[T.NEAR];
  assert.deepEqual([s.settingType, s.rangeType, s.avgPeriod, s.factor], [T.NEAR, RangeType.REAL_BODY, Core.INDEX_MAX, 0]);
  assert.equal(Core.DEFAULT.candleSettings[T.NEAR].avgPeriod, 5);
});

test('a -0 setting is stored as +0, so no lookback or count carries the sign', () => {
  const core = new CoreBuilder()
    .candleSetting(-0 as CandleSettingType, -0 as RangeType, -0, 1)
    .candleSetting(CandleSettingType.BODY_DOJI, RangeType.HIGH_LOW, -0, 0.1)
    .unstablePeriod(FuncUnstId.EMA, -0)
    .build();
  const s = core.candleSettings[CandleSettingType.BODY_LONG];
  for (const v of [s.settingType, s.rangeType, s.avgPeriod, cdldojiLookback(core), core.unstablePeriod(FuncUnstId.EMA)]) {
    assert.ok(Object.is(v, 0), `got ${Object.is(v, -0) ? '-0' : String(v)}`);
  }
});

test('restoreCandleDefault restores one setting or all of them', () => {
  const T = CandleSettingType;
  const b = new CoreBuilder()
    .candleSetting(T.NEAR, RangeType.REAL_BODY, 1, 1)
    .candleSetting(T.FAR, RangeType.REAL_BODY, 1, 1);
  const one = b.restoreCandleDefault(T.NEAR).build();
  assert.equal(one.candleSettings[T.NEAR], Core.DEFAULT.candleSettings[T.NEAR]);
  assert.equal(one.candleSettings[T.FAR].avgPeriod, 1);
  const all = b.restoreCandleDefault(T.ALL_CANDLE_SETTINGS).build();
  assert.deepEqual(all.candleSettings, Core.DEFAULT.candleSettings);
  assertBadParam(() => b.restoreCandleDefault(12 as CandleSettingType));
  assertBadParam(() => b.restoreCandleDefault(-1 as CandleSettingType));
});

test('a built Core is independent of its builder, and toBuilder copies', () => {
  const b = new CoreBuilder().unstablePeriod(FuncUnstId.EMA, 1);
  const first = b.build();
  b.unstablePeriod(FuncUnstId.EMA, 2).candleSetting(CandleSettingType.FAR, RangeType.SHADOWS, 1, 1);
  assert.equal(first.unstablePeriod(FuncUnstId.EMA), 1);
  assert.equal(first.candleSettings[CandleSettingType.FAR].avgPeriod, 5);

  const derived = first.toBuilder().unstablePeriod(FuncUnstId.RSI, 9).build();
  assert.equal(derived.unstablePeriod(FuncUnstId.EMA), 1);
  assert.equal(derived.unstablePeriod(FuncUnstId.RSI), 9);
  assert.equal(first.unstablePeriod(FuncUnstId.RSI), 0);
  assert.equal(Core.DEFAULT.unstablePeriod(FuncUnstId.EMA), 0);
  assert.deepEqual(new Core().candleSettings, Core.DEFAULT.candleSettings);
});

test('no write through a reachable property changes Core.DEFAULT or another Core', () => {
  const T = CandleSettingType;
  const n = 40;
  const close = Float64Array.from({ length: n }, (_, i) => 100 + 5 * Math.sin(i / 3) + (i % 7) * 0.37);
  const open = close.map((c, i) => c + ((i % 3) - 1) * 0.01);
  const high = close.map((c) => c + 1);
  const low = close.map((c) => c - 1);
  // `undefined` takes the default parameter, so Core.DEFAULT is read at call time.
  const observe = (core: Core | undefined): number[] => {
    const e = new Float64Array(n);
    const d = new Int32Array(n);
    const er = ema(0, n - 1, close, 5, e, core);
    const dr = cdldoji(0, n - 1, open, high, low, close, d, core);
    return [emaLookback(5, core), cdldojiLookback(core), er.begIdx, er.count, ...e, dr.begIdx, dr.count, ...d];
  };
  type Loose = Record<string, unknown>;
  const other = new CoreBuilder().build();
  const changed = new CoreBuilder()
    .unstablePeriod(FuncUnstId.ALL, 9)
    .candleSetting(T.BODY_DOJI, RangeType.HIGH_LOW, 30, 1)
    .build();
  const writes: [string, () => void][] = [];
  for (const [who, core] of [['Core.DEFAULT', Core.DEFAULT], ['another Core', other]] as const) {
    writes.push(
      [`${who}.unstablePeriods[EMA]`, () => { (core.unstablePeriods as number[])[FuncUnstId.EMA] = 9; }],
      [`${who}.unstablePeriods`, () => { (core as unknown as Loose).unstablePeriods = changed.unstablePeriods; }],
      [`${who}.candleSettings[BODY_DOJI]`, () => { (core.candleSettings as CandleSetting[])[T.BODY_DOJI] = changed.candleSettings[T.BODY_DOJI]; }],
      [`${who}.candleSettings`, () => { (core as unknown as Loose).candleSettings = changed.candleSettings; }],
      [`${who}.candleSettings[BODY_DOJI].avgPeriod`, () => { (core.candleSettings[T.BODY_DOJI] as unknown as Loose).avgPeriod = 30; }],
    );
  }
  writes.push(
    ["a builder's default CandleSetting", () => { (new CoreBuilder().candleSettings[T.BODY_DOJI] as unknown as Loose).avgPeriod = 30; }],
    ['Core.DEFAULT itself', () => { (Core as unknown as Loose).DEFAULT = changed; }],
  );
  const want = [observe(undefined), observe(other)];
  assert.notDeepEqual(observe(changed), want[0], 'the replacement settings must be observable');
  for (const [name, write] of writes) {
    assert.throws(write, TypeError, `${name} accepted a write`);
    assert.deepEqual([observe(undefined), observe(other)], want, `a write to ${name} leaked`);
  }
  assert.equal(writes.length, 12);
});

test('OutRange', () => {
  const r = new OutRange(19, 81);
  const { begIdx, count } = r;
  assert.deepEqual([begIdx, count, r.isEmpty()], [19, 81, false]);
  assert.ok(OutRange.EMPTY.isEmpty());
  assert.ok(Object.isFrozen(OutRange.EMPTY));
});

test('every exported enum and flag object is frozen: a write cannot redirect a generated read', () => {
  const objects: Record<string, object> = { MAType, FuncUnstId, RetCode, CandleSettingType, RangeType, ...flags };
  for (const [name, o] of Object.entries(objects)) {
    assert.ok(Object.isFrozen(o), `${name} is writable`);
    const [key, before] = Object.entries(o)[0];
    assert.throws(() => {
      (o as Record<string, unknown>)[key] = -1;
    }, TypeError, name);
    assert.equal((o as Record<string, unknown>)[key], before, name);
  }
  assert.ok(Object.keys(flags).length >= 7, `${Object.keys(flags).length} flag objects`);
});
