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

import { FuncUnstId } from '../enums.ts';
import { RetCode, TALibError } from './errors.ts';

/** The candle attribute a {@link CandleSetting} describes; C's order. */
export const CandleSettingType = Object.freeze({
  BODY_LONG: 0,
  BODY_VERY_LONG: 1,
  BODY_SHORT: 2,
  BODY_DOJI: 3,
  SHADOW_LONG: 4,
  SHADOW_VERY_LONG: 5,
  SHADOW_SHORT: 6,
  SHADOW_VERY_SHORT: 7,
  NEAR: 8,
  FAR: 9,
  EQUAL: 10,
  /** Selects every setting, for {@link CoreBuilder.restoreCandleDefault} only. Pinned at 11; not a count. */
  ALL_CANDLE_SETTINGS: 11,
} as const);
export type CandleSettingType = (typeof CandleSettingType)[keyof typeof CandleSettingType];

/** How a candle range is measured; C's order. */
export const RangeType = Object.freeze({
  REAL_BODY: 0,
  HIGH_LOW: 1,
  SHADOWS: 2,
} as const);
export type RangeType = (typeof RangeType)[keyof typeof RangeType];

const INDEX_MAX = 100000000;
const UNSTABLE_AUTO_PREC_4 = INDEX_MAX + 4;
const UNSTABLE_AUTO_PREC_8 = INDEX_MAX + 8;
const CANDLE_SETTING_COUNT = CandleSettingType.ALL_CANDLE_SETTINGS;
const FUNC_UNST_COUNT =
  Math.max(...Object.values(FuncUnstId).filter((id) => id !== FuncUnstId.ALL)) + 1;

/** One candlestick threshold: `factor` times the average of `rangeType` over `avgPeriod` previous candles. */
export class CandleSetting {
  readonly settingType: CandleSettingType;
  readonly rangeType: RangeType;
  /** 0 means no averaging. */
  readonly avgPeriod: number;
  readonly factor: number;

  constructor(settingType: CandleSettingType, rangeType: RangeType, avgPeriod: number, factor: number) {
    this.settingType = settingType;
    this.rangeType = rangeType;
    this.avgPeriod = avgPeriod;
    this.factor = factor;
    Object.freeze(this);
  }
}

const DEFAULT_CANDLE_SETTINGS: readonly CandleSetting[] = [
  new CandleSetting(CandleSettingType.BODY_LONG, RangeType.REAL_BODY, 10, 1.0),
  new CandleSetting(CandleSettingType.BODY_VERY_LONG, RangeType.REAL_BODY, 10, 3.0),
  new CandleSetting(CandleSettingType.BODY_SHORT, RangeType.REAL_BODY, 10, 1.0),
  new CandleSetting(CandleSettingType.BODY_DOJI, RangeType.HIGH_LOW, 10, 0.1),
  new CandleSetting(CandleSettingType.SHADOW_LONG, RangeType.REAL_BODY, 0, 1.0),
  new CandleSetting(CandleSettingType.SHADOW_VERY_LONG, RangeType.REAL_BODY, 0, 2.0),
  new CandleSetting(CandleSettingType.SHADOW_SHORT, RangeType.SHADOWS, 10, 1.0),
  new CandleSetting(CandleSettingType.SHADOW_VERY_SHORT, RangeType.HIGH_LOW, 10, 0.1),
  new CandleSetting(CandleSettingType.NEAR, RangeType.HIGH_LOW, 5, 0.2),
  new CandleSetting(CandleSettingType.FAR, RangeType.HIGH_LOW, 5, 0.6),
  new CandleSetting(CandleSettingType.EQUAL, RangeType.HIGH_LOW, 5, 0.05),
];

function badParam(message: string): TALibError {
  return new TALibError(RetCode.BAD_PARAM, message);
}

function isIntIn(v: number, lo: number, hi: number): boolean {
  return Number.isInteger(v) && v >= lo && v <= hi;
}

function isCandleSlot(settingType: number): boolean {
  return isIntIn(settingType, 0, CANDLE_SETTING_COUNT - 1);
}

/**
 * Configures a {@link Core}. A rejected call throws {@link TALibError}
 * `TA_BAD_PARAM`, as C's setter returns it, and writes nothing.
 */
export class CoreBuilder {
  /** @internal */
  readonly unstablePeriods: Int32Array = new Int32Array(FUNC_UNST_COUNT);
  /** @internal */
  readonly candleSettings: CandleSetting[] = DEFAULT_CANDLE_SETTINGS.slice();

  /**
   * Extra warm-up bars for one function, or for every function given
   * `FuncUnstId.ALL`: 0 to {@link Core.INDEX_MAX}, or an Auto level such as
   * {@link Core.UNSTABLE_AUTO_PREC_4}.
   */
  unstablePeriod(id: FuncUnstId, period: number): this {
    if (!isIntIn(period, 0, INDEX_MAX) && period !== UNSTABLE_AUTO_PREC_4 && period !== UNSTABLE_AUTO_PREC_8) {
      throw badParam(`unstablePeriod must be in 0..${INDEX_MAX} or an UNSTABLE_AUTO level, got ${period}`);
    }
    if (id === FuncUnstId.ALL) {
      this.unstablePeriods.fill(period);
      return this;
    }
    if (!isIntIn(id, 0, FUNC_UNST_COUNT - 1)) {
      throw badParam(`unstablePeriod: ${id} is not a FuncUnstId`);
    }
    this.unstablePeriods[id] = period;
    return this;
  }

  /**
   * Overrides one candlestick setting. `avgPeriod` is in 0 to
   * {@link Core.INDEX_MAX}, and `factor` is finite and not negative.
   */
  candleSetting(settingType: CandleSettingType, rangeType: RangeType, avgPeriod: number, factor: number): this {
    if (!isCandleSlot(settingType)) {
      throw badParam(`candleSetting: ${settingType} is not a single CandleSettingType`);
    }
    if (!isIntIn(rangeType, RangeType.REAL_BODY, RangeType.SHADOWS)) {
      throw badParam(`candleSetting: ${rangeType} is not a RangeType`);
    }
    if (!isIntIn(avgPeriod, 0, INDEX_MAX)) {
      throw badParam(`candleSetting: avgPeriod must be in 0..${INDEX_MAX}, got ${avgPeriod}`);
    }
    if (!(typeof factor === 'number' && factor >= 0 && factor <= Number.MAX_VALUE)) {
      throw badParam(`candleSetting: factor must be finite and not negative, got ${factor}`);
    }
    this.candleSettings[settingType] = new CandleSetting(
      (settingType | 0) as CandleSettingType,
      (rangeType | 0) as RangeType,
      avgPeriod | 0,
      factor,
    );
    return this;
  }

  /** Restores one candlestick setting to its default, or every one given `ALL_CANDLE_SETTINGS`. */
  restoreCandleDefault(settingType: CandleSettingType): this {
    if (settingType === CandleSettingType.ALL_CANDLE_SETTINGS) {
      for (let i = 0; i < CANDLE_SETTING_COUNT; i++) {
        this.candleSettings[i] = DEFAULT_CANDLE_SETTINGS[i];
      }
      return this;
    }
    if (!isCandleSlot(settingType)) {
      throw badParam(`restoreCandleDefault: ${settingType} is not a CandleSettingType`);
    }
    this.candleSettings[settingType] = DEFAULT_CANDLE_SETTINGS[settingType];
    return this;
  }

  /** The builder stays usable, and later calls do not reach the built `Core`. */
  build(): Core {
    return new Core(this);
  }
}

/**
 * The settings every function call reads: unstable periods and candle
 * settings. Immutable; derive a changed one with {@link Core.toBuilder}.
 */
export class Core {
  /** Selects a real optional parameter's default; C's `TA_REAL_DEFAULT`. */
  static readonly REAL_DEFAULT = -4e37;
  static readonly REAL_MIN = -3e37;
  static readonly REAL_MAX = 3e37;
  /** Selects an integer optional parameter's default; C's `TA_INTEGER_DEFAULT`. */
  static readonly INTEGER_DEFAULT = -2147483648;
  static readonly INTEGER_MIN = -2147483647;
  static readonly INTEGER_MAX = 2147483647;
  /** Largest `startIdx` or `endIdx`. It bounds the API domain, not accuracy. */
  static readonly INDEX_MAX = INDEX_MAX;
  /** Auto unstable period: discard output until its first 4 significant digits no longer depend on where the data starts. */
  static readonly UNSTABLE_AUTO_PREC_4 = UNSTABLE_AUTO_PREC_4;
  /** As {@link Core.UNSTABLE_AUTO_PREC_4}, to 8 significant digits. */
  static readonly UNSTABLE_AUTO_PREC_8 = UNSTABLE_AUTO_PREC_8;

  /** Zero unstable periods and the default candle settings. */
  static readonly DEFAULT: Core = new Core();

  /** @internal Indexed by `FuncUnstId`. */
  readonly unstablePeriods: readonly number[];
  /** @internal Indexed by `CandleSettingType`. */
  readonly candleSettings: readonly CandleSetting[];

  /** `new Core(b)` is `b.build()`; `new Core()` has the defaults of {@link Core.DEFAULT}. */
  constructor(builder: CoreBuilder = new CoreBuilder()) {
    // Frozen because `readonly` binds TypeScript callers only. An Int32Array
    // cannot be frozen.
    this.unstablePeriods = Object.freeze(Array.from(builder.unstablePeriods));
    this.candleSettings = Object.freeze(builder.candleSettings.slice());
    Object.freeze(this);
  }

  static builder(): CoreBuilder {
    return new CoreBuilder();
  }

  /** A builder seeded with this `Core`'s settings. */
  toBuilder(): CoreBuilder {
    const builder = new CoreBuilder();
    builder.unstablePeriods.set(this.unstablePeriods);
    for (let i = 0; i < CANDLE_SETTING_COUNT; i++) {
      builder.candleSettings[i] = this.candleSettings[i];
    }
    return builder;
  }

  /** The unstable period set for `id`. `FuncUnstId.ALL` names no single function and is rejected. */
  unstablePeriod(id: FuncUnstId): number {
    if (!isIntIn(id, 0, FUNC_UNST_COUNT - 1)) {
      throw badParam(`unstablePeriod: ${id} is not a single FuncUnstId`);
    }
    return this.unstablePeriods[id];
  }

  /** @internal The count for one call: the stored one, or under an Auto level that level's count. */
  unstableCount(slot: number, prec4: number, prec8: number): number {
    const stored = this.unstablePeriods[slot];
    return stored <= INDEX_MAX ? stored : stored === UNSTABLE_AUTO_PREC_4 ? prec4 : prec8;
  }

  static {
    Object.freeze(this);
  }
}
