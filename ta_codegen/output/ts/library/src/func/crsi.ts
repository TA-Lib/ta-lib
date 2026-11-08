/*! TA-LIB Copyright (c) 1999-2026, Mario Fortier
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
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  092226 MF,CC  Initial version (#431).
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { PercentrankStream, percentrankLookbackPositional, percentrankOpenAndFillInternal, percentrankPositional } from './percentrank.ts';
import { RocpStream, rocpLookbackPositional, rocpOpenAndFillInternal, rocpPositional } from './rocp.ts';
import { RsiStream, rsiLookbackPositional, rsiOpenAndFillInternal, rsiPositional } from './rsi.ts';

/** @internal */
export function crsiLookbackPositional(optInTimePeriod: number, optInStreakPeriod: number, optInRankPeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if ((optInStreakPeriod | 0) !== optInStreakPeriod) {
    return -1;
  }
  if ((optInRankPeriod | 0) !== optInRankPeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 3;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  if (optInStreakPeriod === Core.INTEGER_DEFAULT) {
    optInStreakPeriod = 2;
  } else if (optInStreakPeriod < 2 || optInStreakPeriod > 100000) {
    return -1;
  }
  if (optInRankPeriod === Core.INTEGER_DEFAULT) {
    optInRankPeriod = 100;
  } else if (optInRankPeriod < 2 || optInRankPeriod > 10000) {
    return -1;
  }
  let retValue = 0;
  retValue = rsiLookbackPositional(optInTimePeriod, core);
  retValue = (retValue > (rsiLookbackPositional(optInStreakPeriod, core) + 1) ? retValue : (rsiLookbackPositional(optInStreakPeriod, core) + 1));
  retValue = (retValue > (percentrankLookbackPositional(optInRankPeriod, core) + rocpLookbackPositional(1, core)) ? retValue : (percentrankLookbackPositional(optInRankPeriod, core) + rocpLookbackPositional(1, core)));
  return retValue;
}

/** @internal */
export function crsiDisplayShiftPositional(optInTimePeriod: number, optInStreakPeriod: number, optInRankPeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (crsiLookbackPositional(optInTimePeriod, optInStreakPeriod, optInRankPeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function crsiImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  optInStreakPeriod: number,
  optInRankPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let tempRSI!: Float64Array;
  let tempStreak!: Float64Array;
  let tempStreakRSI!: Float64Array;
  let lookbackTotal = 0;
  let anchorIdx = 0;
  let today = 0;
  let outIdx = 0;
  let i = 0;
  let offsetRSI = 0;
  let offsetStreak = 0;
  const tempBegIdx = new MInteger();
  const rsiNb = new MInteger();
  const streakNb = new MInteger();
  const rocNb = new MInteger();
  let prevClose = 0;
  let close = 0;
  let streak = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 3;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInStreakPeriod === Core.INTEGER_DEFAULT) {
    optInStreakPeriod = 2;
  } else if (optInStreakPeriod < 2 || optInStreakPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInRankPeriod === Core.INTEGER_DEFAULT) {
    optInRankPeriod = 100;
  } else if (optInRankPeriod < 2 || optInRankPeriod > 10000) {
    return RetCode.BAD_PARAM;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = crsiLookbackPositional(optInTimePeriod, optInStreakPeriod, optInRankPeriod, core);
  if (lookbackTotal > endIdx) {
    return RetCode.SUCCESS;
  }
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Both RSI legs start on the bar lookbackTotal before startIdx (the streak,
   * a difference, one bar later), so the leg with the shorter lookback warms
   * up over the spare bars instead of starting cold at its own lookback.
   */
  anchorIdx = startIdx - lookbackTotal;
  tempStreak = new Float64Array(endIdx - anchorIdx);
  tempRSI = new Float64Array(endIdx - anchorIdx - rsiLookbackPositional(optInTimePeriod, core) + 1);
  tempStreakRSI = new Float64Array(endIdx - anchorIdx - rsiLookbackPositional(optInStreakPeriod, core));
  streak = 0.0;
  prevClose = inReal[anchorIdx];
  outIdx = 0;
  today = anchorIdx + 1;
  while (today <= endIdx) {
    close = inReal[today];
    if (close > prevClose) {
      streak = (streak > 0.0) ? streak + 1.0 : 1.0;
    } else if (close < prevClose) {
      streak = (streak < 0.0) ? streak - 1.0 : -1.0;
    } else {
      streak = 0.0;
    }
    tempStreak[outIdx++] = streak;
    prevClose = close;
    today += 1;
  }
  const _xr0 = rsiPositional(0, outIdx - 1, tempStreak, optInStreakPeriod, tempStreakRSI, core);
  tempBegIdx.value = _xr0.begIdx;
  streakNb.value = _xr0.count;
  const _xr1 = rsiPositional(anchorIdx + rsiLookbackPositional(optInTimePeriod, core), endIdx, inReal, optInTimePeriod, tempRSI, core);
  tempBegIdx.value = _xr1.begIdx;
  rsiNb.value = _xr1.count;
  /* The streak is consumed: its buffer holds the returns. */
  const _xr2 = rocpPositional(startIdx - optInRankPeriod, endIdx, inReal, 1, tempStreak, core);
  tempBegIdx.value = _xr2.begIdx;
  rocNb.value = _xr2.count;
  const _xr3 = percentrankPositional(0, rocNb.value - 1, tempStreak, optInRankPeriod, outReal, core);
  outBegIdx.value = _xr3.begIdx;
  outNBElement.value = _xr3.count;
  offsetRSI = rsiNb.value - outNBElement.value;
  offsetStreak = streakNb.value - outNBElement.value;
  for (i = 0; i < outNBElement.value; i += 1) {
    outReal[i] = (tempRSI[i + offsetRSI] + tempStreakRSI[i + offsetStreak] + outReal[i]) / 3.0;
  }
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['streakPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['rankPeriod', Core.INTEGER_DEFAULT, 2, 10000, 'in 2..10000']] as const;

/** @internal */
export function crsiPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInStreakPeriod: number,
  optInRankPeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CRSI', 'startIdx', startIdx);
  endIdx = requireInt32('CRSI', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('CRSI', 'timePeriod', optInTimePeriod);
  optInStreakPeriod = requireInt32('CRSI', 'streakPeriod', optInStreakPeriod);
  optInRankPeriod = requireInt32('CRSI', 'rankPeriod', optInRankPeriod);
  requireIndexRange('CRSI', startIdx, endIdx);
  const guardLookback = crsiLookbackPositional(optInTimePeriod, optInStreakPeriod, optInRankPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('CRSI', PARAM_RANGES, optInTimePeriod, optInStreakPeriod, optInRankPeriod);
  }
  const guardStart = clampedStart('CRSI', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CRSI', 'real', inReal, guardInLen);
  requireOutput('CRSI', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('CRSI', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = crsiImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, optInStreakPeriod, optInRankPeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CRSI', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link crsi}, {@link crsiLookback}, {@link crsiDisplayShift},
 * {@link crsiOpen} and {@link crsiOpenAndFill}. Every key is optional: an omitted
 * or `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface CrsiOptions {
  /**
   * Period of the RSI of the closes (default 3; range 2 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  timePeriod?: number | undefined;
  /**
   * Period of the RSI of the up/down streak (default 2; range 2 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  streakPeriod?: number | undefined;
  /**
   * Number of earlier one-bar returns each return is ranked against (default 100;
   * range 2 to 10000; `Core.INTEGER_DEFAULT` selects the default).
   */
  rankPeriod?: number | undefined;
  /**
   * First bar of the requested range (default 0). Bars before it are still read as
   * warm-up. Batch calls only.
   */
  startIdx?: number | undefined;
  /**
   * Last bar of the requested range, inclusive (default the last bar of the first
   * input). Batch calls only.
   */
  endIdx?: number | undefined;
  /**
   * Unstable periods and candle settings to compute with; `Core.DEFAULT` when
   * omitted.
   */
  core?: Core | undefined;
  /**
   * Buffer for `values`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outReal?: Float64Array<ArrayBuffer> | undefined;
}

/** What {@link crsi} returns, and {@link crsiOpenAndFill} with its stream. */
export interface CrsiResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * The averaged reading, 0 to 100. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link crsi} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CrsiOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function crsiLookback(options?: CrsiOptions): number {
  const o = optionsOf('CRSI lookback', options);
  return crsiLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.streakPeriod, Core.INTEGER_DEFAULT), orDefault(o.rankPeriod, Core.INTEGER_DEFAULT), coreOf('CRSI lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link crsi}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CrsiOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function crsiDisplayShift(outputIdx: number, options?: CrsiOptions): number {
  const o = optionsOf('CRSI displayShift', options);
  return crsiDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.streakPeriod, Core.INTEGER_DEFAULT), orDefault(o.rankPeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('CRSI displayShift', o.core));
}

/**
 * Connors RSI: a 0 to 100 oscillator, the plain average of three short-term
 * momentum readings on that same scale. The first is Wilder's RSI of the closes
 * over `timePeriod` bars. The second is an RSI over `streakPeriod` bars of the
 * up/down streak, the signed length of the current run of higher or lower closes
 * (+3 after three higher closes in a row, -2 after two lower ones); an unchanged
 * close resets the streak to 0. The third is the percent rank of the bar's one-bar
 * return, (close - previous close) / previous close, among the `rankPeriod`
 * returns before it: the percentage of them strictly below it. High readings mark
 * a short-term overbought market and low readings an oversold one.
 *
 * Formula and more info at
 * [ta-lib.org/functions/crsi](https://ta-lib.org/functions/crsi).
 *
 * **Notes**
 * - CRSI has no unstable period of its own: both RSI legs take `FuncUnstId.RSI`.
 *   They start warming up on the first bar the call reads (the streak's RSI one
 *   bar later, the streak being a change), so a leg that needs fewer bars than the
 *   lookback warms up over the spare ones.
 * - Finite input is a precondition. A NaN close does not propagate: the output
 *   stays finite and no error is reported, but the values that follow it are
 *   wrong.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link crsiLookback} is a success with empty outputs.
 *
 * @param real - Close price series.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link CrsiOptions}.
 * @returns `begIdx` and the values; see {@link CrsiResult}. Each output is a view
 *   of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, an integer parameter that is not an int32, a parameter outside its range,
 *   an input too short for the range, a buffer that is empty, too short or of the
 *   wrong type or a buffer sharing memory with a `Float64Array` input it is not.
 *   Checked before anything is written.
 *
 * @example
 * ```ts
 * import { crsi } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = crsi(data, {
 *   timePeriod: 3,
 *   streakPeriod: 2,
 *   rankPeriod: 100,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`rsi`](https://ta-lib.org/functions/rsi)
 * @see [`percentrank`](https://ta-lib.org/functions/percentrank)
 * @see [`stochrsi`](https://ta-lib.org/functions/stochrsi)
 */
export function crsi(real: ArrayLike<number>, options?: CrsiOptions): CrsiResult {
  const o = optionsOf('CRSI', options);
  if (o.endIdx === undefined) {
    requireSeries('CRSI', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInStreakPeriod = orDefault(o.streakPeriod, Core.INTEGER_DEFAULT);
  const optInRankPeriod = orDefault(o.rankPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('CRSI', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, crsiLookbackPositional(optInTimePeriod, optInStreakPeriod, optInRankPeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = crsiPositional(startIdx, endIdx, real, optInTimePeriod, optInStreakPeriod, optInRankPeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CRSI stream: one value per closed bar, bit-identical to {@link crsi} over
 * the same series. Open it with {@link crsiOpen} or {@link crsiOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/crsi](https://ta-lib.org/functions/crsi).
 */
export class CrsiStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  optInStreakPeriod: number = 0;
  /** @internal */
  optInRankPeriod: number = 0;
  /** @internal */
  prevClose: number = 0;
  /** @internal */
  streak: number = 0;
  /** @internal */
  cur_outReal: number = 0;
  /** @internal */
  sub0!: RsiStream;
  /** @internal */
  sub1!: RsiStream;
  /** @internal */
  sub2!: RocpStream;
  /** @internal */
  sub3!: PercentrankStream;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): CrsiStream {
    return new CrsiStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link crsi} reports them over the same bars. Each accepted `update` and each
   * `advance` adds one to `count`.
   */
  outRange(): OutRange {
    return new OutRange(this.outRangeBegIdx, this.outRangeCount);
  }

  /**
   * Counts one bar this stream was not fed: `outRange()` advances by one and
   * `value()` keeps answering the previous output.
   *
   * @throws `TALibError` with code `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has
   *   reached bar `Core.INDEX_MAX`.
   */
  advance(): void {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('CRSI advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: Close price series.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('CRSI update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('CRSI update', 'real');
    }
    crsiStepImpl(this, real);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: Close price series.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number): number {
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('CRSI peek', 'real');
    }
    const sp = this;
    const inReal = real;
    let cur_tempStreak = 0;
    let cur_tempStreakRSI = 0;
    let cur_tempRSI = 0;
    let cur_outReal = 0;
    let close = 0;
    let streak = sp.streak;
    close = inReal;
    if (close > sp.prevClose) {
      streak = (streak > 0.0) ? streak + 1.0 : 1.0;
    } else if (close < sp.prevClose) {
      streak = (streak < 0.0) ? streak - 1.0 : -1.0;
    } else {
      streak = 0.0;
    }
    cur_tempStreak = streak;
    cur_tempStreakRSI = sp.sub0.peek(cur_tempStreak);
    cur_tempRSI = sp.sub1.peek(inReal);
    cur_tempStreak = sp.sub2.peek(inReal);
    cur_outReal = sp.sub3.peek(cur_tempStreak);
    cur_outReal = (cur_tempRSI + cur_tempStreakRSI + cur_outReal) / 3.0;
    return cur_outReal;
  }

  /** The value at the last bar this stream counted; `peek` does not change it. */
  value(): number {
    return this.cur_outReal;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): CrsiStream {
    const c = new CrsiStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.optInStreakPeriod = this.optInStreakPeriod;
    c.optInRankPeriod = this.optInRankPeriod;
    c.prevClose = this.prevClose;
    c.streak = this.streak;
    c.cur_outReal = this.cur_outReal;
    c.sub0 = this.sub0.clone();
    c.sub1 = this.sub1.clone();
    c.sub2 = this.sub2.clone();
    c.sub3 = this.sub3.clone();
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function crsiStepImpl(sp: CrsiStream, inReal: number): void {
  let close = 0;
  let cur_tempStreak = 0;
  let cur_tempStreakRSI = 0;
  let cur_tempRSI = 0;
  let cur_outReal = 0;
  close = inReal;
  if (close > sp.prevClose) {
    sp.streak = (sp.streak > 0.0) ? sp.streak + 1.0 : 1.0;
  } else if (close < sp.prevClose) {
    sp.streak = (sp.streak < 0.0) ? sp.streak - 1.0 : -1.0;
  } else {
    sp.streak = 0.0;
  }
  cur_tempStreak = sp.streak;
  sp.prevClose = close;
  cur_tempStreakRSI = sp.sub0.update(cur_tempStreak);
  cur_tempRSI = sp.sub1.update(inReal);
  cur_tempStreak = sp.sub2.update(inReal);
  cur_outReal = sp.sub3.update(cur_tempStreak);
  cur_outReal = (cur_tempRSI + cur_tempStreakRSI + cur_outReal) / 3.0;
  sp.cur_outReal = cur_outReal;
}

function crsiOpenImpl(
  sp: CrsiStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInStreakPeriod: number,
  optInRankPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let tempRSI!: Float64Array;
  let tempStreak!: Float64Array;
  let tempStreakRSI!: Float64Array;
  let lookbackTotal = 0;
  let anchorIdx = 0;
  let today = 0;
  let outIdx = 0;
  let i = 0;
  let offsetRSI = 0;
  let offsetStreak = 0;
  const tempBegIdx = new MInteger();
  const rsiNb = new MInteger();
  const streakNb = new MInteger();
  const rocNb = new MInteger();
  let prevClose = 0;
  let close = 0;
  let streak = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 3;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInStreakPeriod === Core.INTEGER_DEFAULT) {
    optInStreakPeriod = 2;
  } else if (optInStreakPeriod < 2 || optInStreakPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInRankPeriod === Core.INTEGER_DEFAULT) {
    optInRankPeriod = 100;
  } else if (optInRankPeriod < 2 || optInRankPeriod > 10000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen < crsiLookbackPositional(optInTimePeriod, optInStreakPeriod, optInRankPeriod, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const sc_outReal = outStride === 1 ? outReal : new Float64Array(historyLen);
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = crsiLookbackPositional(optInTimePeriod, optInStreakPeriod, optInRankPeriod, core);
  if (lookbackTotal > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Both RSI legs start on the bar lookbackTotal before startIdx (the streak,
   * a difference, one bar later), so the leg with the shorter lookback warms
   * up over the spare bars instead of starting cold at its own lookback.
   */
  anchorIdx = startIdx - lookbackTotal;
  tempStreak = new Float64Array(endIdx - anchorIdx);
  tempRSI = new Float64Array(endIdx - anchorIdx - rsiLookbackPositional(optInTimePeriod, core) + 1);
  tempStreakRSI = new Float64Array(endIdx - anchorIdx - rsiLookbackPositional(optInStreakPeriod, core));
  streak = 0.0;
  prevClose = inReal[anchorIdx];
  outIdx = 0;
  today = anchorIdx + 1;
  while (today <= endIdx) {
    close = inReal[today];
    if (close > prevClose) {
      streak = (streak > 0.0) ? streak + 1.0 : 1.0;
    } else if (close < prevClose) {
      streak = (streak < 0.0) ? streak - 1.0 : -1.0;
    } else {
      streak = 0.0;
    }
    tempStreak[outIdx++] = streak;
    prevClose = close;
    today += 1;
  }
  const sub0 = rsiOpenAndFillInternal(core, tempStreak.slice(0, (outIdx - 1) + 1), 0, optInStreakPeriod, tempBegIdx, streakNb, tempStreakRSI);
  const sub1 = rsiOpenAndFillInternal(core, inReal, anchorIdx + rsiLookbackPositional(optInTimePeriod, core), optInTimePeriod, tempBegIdx, rsiNb, tempRSI);
  /* The streak is consumed: its buffer holds the returns. */
  const sub2 = rocpOpenAndFillInternal(core, inReal, startIdx - optInRankPeriod, 1, tempBegIdx, rocNb, tempStreak);
  const sub3 = percentrankOpenAndFillInternal(core, tempStreak.slice(0, (rocNb.value - 1) + 1), 0, optInRankPeriod, outBegIdx, outNBElement, sc_outReal);
  offsetRSI = rsiNb.value - outNBElement.value;
  offsetStreak = streakNb.value - outNBElement.value;
  for (i = 0; i < outNBElement.value; i += 1) {
    sc_outReal[i] = (tempRSI[i + offsetRSI] + tempStreakRSI[i + offsetStreak] + sc_outReal[i]) / 3.0;
  }
  outBegIdx.value = startIdx;
  if (outNBElement.value < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.optInStreakPeriod = optInStreakPeriod;
  sp.optInRankPeriod = optInRankPeriod;
  sp.prevClose = prevClose;
  sp.streak = streak;
  sp.sub0 = sub0;
  sp.sub1 = sub1;
  sp.sub2 = sub2;
  sp.sub3 = sub3;
  sp.cur_outReal = sc_outReal[outNBElement.value - 1];
  return RetCode.SUCCESS;
}

/** @internal */
export function crsiOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInStreakPeriod: number,
  optInRankPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): CrsiStream {
  const sp = CrsiStream.of(core);
  const retCode = crsiOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInStreakPeriod, optInRankPeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CRSI openAndFill', inReal.length, startIdx, crsiLookbackPositional(optInTimePeriod, optInStreakPeriod, optInRankPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && crsiLookbackPositional(optInTimePeriod, optInStreakPeriod, optInRankPeriod, core) < 0) {
    throw rejectedParams('CRSI openAndFill', PARAM_RANGES, optInTimePeriod, optInStreakPeriod, optInRankPeriod);
  }
  throw streamFailure('CRSI openAndFill', retCode);
}

/** @internal */
export function crsiOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInStreakPeriod: number,
  optInRankPeriod: number,
): CrsiStream {
  const sp = CrsiStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = crsiOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInStreakPeriod, optInRankPeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CRSI open', inReal.length, startIdx, crsiLookbackPositional(optInTimePeriod, optInStreakPeriod, optInRankPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && crsiLookbackPositional(optInTimePeriod, optInStreakPeriod, optInRankPeriod, core) < 0) {
    throw rejectedParams('CRSI open', PARAM_RANGES, optInTimePeriod, optInStreakPeriod, optInRankPeriod);
  }
  throw streamFailure('CRSI open', retCode);
}

/** @internal */
export function crsiOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInStreakPeriod: number,
  optInRankPeriod: number,
  core: Core = Core.DEFAULT,
): CrsiStream {
  optInTimePeriod = requireInt32('CRSI open', 'timePeriod', optInTimePeriod);
  optInStreakPeriod = requireInt32('CRSI open', 'streakPeriod', optInStreakPeriod);
  optInRankPeriod = requireInt32('CRSI open', 'rankPeriod', optInRankPeriod);
  requireArgument('CRSI open', 'real', inReal);
  requireHistory('CRSI open', 'real', inReal.length);
  return crsiOpenInternal(core, float64(inReal), 0, optInTimePeriod, optInStreakPeriod, optInRankPeriod);
}

/** @internal */
export function crsiOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInStreakPeriod: number,
  optInRankPeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): CrsiStream {
  optInTimePeriod = requireInt32('CRSI openAndFill', 'timePeriod', optInTimePeriod);
  optInStreakPeriod = requireInt32('CRSI openAndFill', 'streakPeriod', optInStreakPeriod);
  optInRankPeriod = requireInt32('CRSI openAndFill', 'rankPeriod', optInRankPeriod);
  requireArgument('CRSI openAndFill', 'real', inReal);
  requireHistory('CRSI openAndFill', 'real', inReal.length);
  const guardLookback = crsiLookbackPositional(optInTimePeriod, optInStreakPeriod, optInRankPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('CRSI openAndFill', PARAM_RANGES, optInTimePeriod, optInStreakPeriod, optInRankPeriod);
  }
  const guardOutLen = openFillCount('CRSI openAndFill', inReal.length, guardLookback);
  requireOutput('CRSI openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('CRSI openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('CRSI openAndFill', 'outReal', 'real');
  }
  return crsiOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, optInStreakPeriod, optInRankPeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live CRSI stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link crsi} there.
 *
 * @param real - Warm-up history. Close price series.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CrsiOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `crsiLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { crsiOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = crsiOpen(data, {
 *   timePeriod: 3,
 *   streakPeriod: 2,
 *   rankPeriod: 100,
 * });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function crsiOpen(real: ArrayLike<number>, options?: CrsiOptions): CrsiStream {
  const o = optionsOf('CRSI open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInStreakPeriod = orDefault(o.streakPeriod, Core.INTEGER_DEFAULT);
  const optInRankPeriod = orDefault(o.rankPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('CRSI open', o.core);
  return crsiOpenPositional(real, optInTimePeriod, optInStreakPeriod, optInRankPeriod, core);
}

/**
 * {@link crsiOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link crsi}.
 *
 * @param real - Warm-up history. Close price series.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CrsiOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link crsi} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `crsiLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or a
 *   buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { crsiOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = crsiOpenAndFill(data, {
 *   timePeriod: 3,
 *   streakPeriod: 2,
 *   rankPeriod: 100,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function crsiOpenAndFill(real: ArrayLike<number>, options?: CrsiOptions): CrsiResult & { stream: CrsiStream } {
  const o = optionsOf('CRSI openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInStreakPeriod = orDefault(o.streakPeriod, Core.INTEGER_DEFAULT);
  const optInRankPeriod = orDefault(o.rankPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('CRSI openAndFill', o.core);
  const capacity = fillCapacity(crsiLookbackPositional(optInTimePeriod, optInStreakPeriod, optInRankPeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = crsiOpenAndFillPositional(real, optInTimePeriod, optInStreakPeriod, optInRankPeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
