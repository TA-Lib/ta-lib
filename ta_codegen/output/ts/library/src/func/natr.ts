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
 *  060306 MF     Initial Version
 *  070526 MF,CC  Fix #98: partial-range calls normalized with a close
 *                from the wrong bar (TR-buffer-relative index).
 *  070626 MF,CC  Speed optimization: True Range computed inline in a
 *                single pass (bit-exact, no temporary buffer).
 *  082326 MF,CC  Fix #253. Test the close exactly instead of against the fixed
 *                TA_IS_ZERO band, which zeroed the output for any instrument
 *                quoted small enough to fall under it.
 *  090326 MF,CC  #338 Two-coefficient Wilder step; no divide in the
 *                loop-carried chain.
 */

import { FuncUnstId } from '../enums.ts';
import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function natrLookbackPositional(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  /* The ATR lookback is the sum of:
   *    1 + (optInTimePeriod - 1)
   *
   * Where 1 is for the True Range, and
   * (optInTimePeriod-1) is for the simple
   * moving average.
   */
  return optInTimePeriod + core.unstableCount(FuncUnstId.NATR, ((optInTimePeriod > 1) ? ((10 * optInTimePeriod) | 0) : 0), ((optInTimePeriod > 1) ? ((19 * optInTimePeriod) | 0) : 0));
}

/** @internal */
export function natrDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (natrLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function natrImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let i = 0;
  let outIdx = 0;
  let today = 0;
  let lookbackTotal = 0;
  let nbATR = 0;
  let prevATR = 0;
  let periodTotal = 0;
  let tempValue = 0;
  let wAlpha = 0;
  let wBeta = 0;
  let val2 = 0;
  let val3 = 0;
  let greatest = 0;
  let tempCY = 0;
  let tempLT = 0;
  let tempHT = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  /* This function is very similar as ATR, except
   * it is being normalized as follow:
   *
   *    NATR = (ATR(period) / Close) * 100
   *
   *
   * Normalization make the ATR function more relevant
   * in the folllowing scenario:
   *    - Long term analysis where the price changes drastically.
   *    - Cross-market or cross-security ATR comparison.
   *
   * More Info:
   *      Technical Analysis of Stock & Commodities (TASC)
   *      May 2006 by John Forman
   */
  /* Average True Range is the greatest of the following:
   *
   *  val1 = distance from today's high to today's low.
   *  val2 = distance from yesterday's close to today's high.
   *  val3 = distance from yesterday's close to today's low.
   *
   * These value are averaged for the specified period using
   * Wilder method. This method have an unstable period comparable
   * to an Exponential Moving Average (EMA).
   */
  outBegIdx.value = 0;
  outNBElement.value = 0;
  /* Adjust startIdx to account for the lookback period. */
  lookbackTotal = natrLookbackPositional(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  /* wAlpha is derived FROM wBeta, never the reverse: only that order makes
   * wAlpha + wBeta exactly 1 (Sterbenz -- wBeta lands in [0.5, 1)), and it
   * measures closer to the exact recursion than the 1/period-first spelling
   * at nearly every period. The order is a gated contract, not a preference:
   * swapping it reddens the frozen-release comparison.
   */
  wBeta = (optInTimePeriod - 1) / optInTimePeriod;
  wAlpha = 1.0 - wBeta;
  /* The True Range of each bar is computed inline in a single
   * pass. No temporary buffer is needed.
   *
   * The arithmetic order below is the bit-exactness contract
   * (do not reorder):
   *  - True Range: start from high-low, then compare/replace
   *    with the two previous-close distances, in that order.
   *  - Seed: the first 'period' True Range values are summed,
   *    accumulated from 0.0 in input order, then divided by
   *    the period.
   *  - Wilder smoothing: ONE statement. Splitting it back
   *    unfuses the multiply-add and puts a second latency on
   *    the recurrence's dependency chain.
   *
   * Each output is normalized by the close of its own bar; a
   * close of zero yields 0.0.
   *
   * In-place (outReal being one of the input arrays) is
   * supported: each output is written only after every input
   * read at or before its bar, and the output index is always
   * smaller than the bar index of any remaining read.
   */
  /* The first True Range needs the two price bars at
   * startIdx-lookbackTotal+1 (a previous close is consumed).
   */
  today = startIdx - lookbackTotal + 1;
  /* Seed the ATR with a simple average of the True Range
   * for the first 'period' bars.
   */
  periodTotal = 0.0;
  i = optInTimePeriod;
  while (i-- > 0) {
    /* Find the greatest of the 3 values. */
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    periodTotal += greatest;
    today += 1;
  }
  prevATR = periodTotal / optInTimePeriod;
  /* Skip the unstable period. */
  i = lookbackTotal - optInTimePeriod;
  while (i !== 0) {
    /* Find the greatest of the 3 values. */
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    prevATR = fma(wBeta, prevATR, wAlpha * greatest);
    today += 1;
    i -= 1;
  }
  /* Now start to write the final NATR in the caller
   * provided outReal.
   */
  outIdx = 1;
  if (optInTimePeriod <= 1) {
    /* Period 1 is the raw True Range and is deliberately NOT normalized,
     * which is the TRANGE delegation this path replaced.
     */
    outReal[0] = prevATR;
  } else {
    /* NATR is the ATR as a percentage of the close, so it is scale-free and
     * the divisor only has to be non-zero. An exact test, not the fixed
     * TA_IS_ZERO band it used to be: a close carries the quote unit, and that
     * band zeroed the whole output for any instrument quoted below it (#253).
     */
    tempValue = inClose[startIdx];
    if (tempValue !== 0.0) {
      outReal[0] = prevATR / tempValue * 100.0;
    } else {
      outReal[0] = 0.0;
    }
  }
  /* Now do the number of requested NATR. */
  nbATR = endIdx - startIdx + 1;
  while (--nbATR !== 0) {
    /* Find the greatest of the 3 values. */
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    prevATR = fma(wBeta, prevATR, wAlpha * greatest);
    if (optInTimePeriod <= 1) {
      outReal[outIdx] = prevATR;
    } else {
      tempValue = inClose[today];
      if (tempValue !== 0.0) {
        outReal[outIdx] = prevATR / tempValue * 100.0;
      } else {
        outReal[outIdx] = 0.0;
      }
    }
    outIdx += 1;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000']] as const;

/** @internal */
export function natrPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('NATR', 'startIdx', startIdx);
  endIdx = requireInt32('NATR', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('NATR', 'timePeriod', optInTimePeriod);
  requireIndexRange('NATR', startIdx, endIdx);
  const guardLookback = natrLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('NATR', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('NATR', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('NATR', 'prices.high', inHigh, guardInLen);
  requireLength('NATR', 'prices.low', inLow, guardInLen);
  requireLength('NATR', 'prices.close', inClose, guardInLen);
  requireOutput('NATR', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outReal, inHighF64)) {
    throw sharedMemory('NATR', 'outReal', 'prices.high');
  }
  if (overlapsPartly(outReal, inLowF64)) {
    throw sharedMemory('NATR', 'outReal', 'prices.low');
  }
  if (overlapsPartly(outReal, inCloseF64)) {
    throw sharedMemory('NATR', 'outReal', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = natrImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('NATR', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link natr}, {@link natrLookback}, {@link natrDisplayShift},
 * {@link natrOpen} and {@link natrOpenAndFill}. Every key is optional: an omitted
 * or `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface NatrOptions {
  /**
   * Smoothing period for the true range average (default 14; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  timePeriod?: number | undefined;
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

/** What {@link natr} returns, and {@link natrOpenAndFill} with its stream. */
export interface NatrResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * ATR as a percentage of the close. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link natr} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link NatrOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function natrLookback(options?: NatrOptions): number {
  const o = optionsOf('NATR lookback', options);
  return natrLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('NATR lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link natr}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link NatrOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function natrDisplayShift(outputIdx: number, options?: NatrOptions): number {
  const o = optionsOf('NATR displayShift', options);
  return natrDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('NATR displayShift', o.core));
}

/**
 * Average True Range expressed as a percentage of the current close, making
 * volatility comparable across price levels and securities. Same computation as
 * ATR, then normalized by close. Higher values mean greater relative volatility;
 * unit is percent of price.
 *
 * Formula and more info at
 * [ta-lib.org/functions/natr](https://ta-lib.org/functions/natr).
 *
 * **Notes**
 * - **The input is assumed positive.** The close is the divisor. Where it is
 *   exactly 0 the output is 0. Near zero the divisor is small and the output very
 *   large; no error is returned. A series that reaches or crosses zero, such as an
 *   oscillator or a spread, has no meaningful result here.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link natrLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `high`: High price of
 *   each bar. `low`: Low price of each bar. `close`: Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link NatrOptions}.
 * @returns `begIdx` and the values; see {@link NatrResult}. Each output is a view
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
 * import { natr } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { begIdx, values } = natr({ high, low, close }, { timePeriod: 14 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`atr`](https://ta-lib.org/functions/atr)
 * @see [`trange`](https://ta-lib.org/functions/trange)
 * @see [`sma`](https://ta-lib.org/functions/sma)
 */
export function natr(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: NatrOptions): NatrResult {
  requireArgument('NATR', 'prices', prices);
  const o = optionsOf('NATR', options);
  if (o.endIdx === undefined) {
    requireSeries('NATR', 'prices.high', prices.high);
    requireSeries('NATR', 'prices.low', prices.low);
    requireSeries('NATR', 'prices.close', prices.close);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('NATR', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, natrLookbackPositional(optInTimePeriod, core), prices.high, prices.low, prices.close);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = natrPositional(startIdx, endIdx, prices.high, prices.low, prices.close, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live NATR stream: one value per closed bar, bit-identical to {@link natr} over
 * the same series. Open it with {@link natrOpen} or {@link natrOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/natr](https://ta-lib.org/functions/natr).
 */
export class NatrStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  prevATR: number = 0;
  /** @internal */
  wAlpha: number = 0;
  /** @internal */
  wBeta: number = 0;
  /** @internal */
  lag1_inClose: number = 0;
  /** @internal */
  cur_outReal: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): NatrStream {
    return new NatrStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link natr} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('NATR advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(high: number, low: number, close: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('NATR update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('NATR update', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    natrStepImpl(this, high, low, close);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(high: number, low: number, close: number): number {
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('NATR peek', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
    const inClose = close;
    let tempValue = 0;
    let val2 = 0;
    let val3 = 0;
    let greatest = 0;
    let tempCY = 0;
    let tempLT = 0;
    let tempHT = 0;
    let cur_outReal = 0;
    let prevATR = sp.prevATR;
    /* Find the greatest of the 3 values. */
    tempLT = inLow;
    tempHT = inHigh;
    tempCY = sp.lag1_inClose;
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    prevATR = fma(sp.wBeta, prevATR, sp.wAlpha * greatest);
    if (sp.optInTimePeriod <= 1) {
      cur_outReal = prevATR;
    } else {
      tempValue = inClose;
      if (tempValue !== 0.0) {
        cur_outReal = prevATR / tempValue * 100.0;
      } else {
        cur_outReal = 0.0;
      }
    }
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
  clone(): NatrStream {
    const c = new NatrStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.prevATR = this.prevATR;
    c.wAlpha = this.wAlpha;
    c.wBeta = this.wBeta;
    c.lag1_inClose = this.lag1_inClose;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function natrStepImpl(sp: NatrStream, inHigh: number, inLow: number, inClose: number): void {
  let tempValue = 0;
  let val2 = 0;
  let val3 = 0;
  let greatest = 0;
  let tempCY = 0;
  let tempLT = 0;
  let tempHT = 0;
  /* Find the greatest of the 3 values. */
  tempLT = inLow;
  tempHT = inHigh;
  tempCY = sp.lag1_inClose;
  greatest = tempHT - tempLT;
  /* val1 */
  val2 = Math.abs(tempCY - tempHT);
  if (val2 > greatest) {
    greatest = val2;
  }
  val3 = Math.abs(tempCY - tempLT);
  if (val3 > greatest) {
    greatest = val3;
  }
  sp.prevATR = fma(sp.wBeta, sp.prevATR, sp.wAlpha * greatest);
  if (sp.optInTimePeriod <= 1) {
    sp.cur_outReal = sp.prevATR;
  } else {
    tempValue = inClose;
    if (tempValue !== 0.0) {
      sp.cur_outReal = sp.prevATR / tempValue * 100.0;
    } else {
      sp.cur_outReal = 0.0;
    }
  }
  sp.lag1_inClose = inClose;
}

function natrOpenImpl(
  sp: NatrStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let i = 0;
  let outIdx = 0;
  let today = 0;
  let lookbackTotal = 0;
  let nbATR = 0;
  let prevATR = 0;
  let periodTotal = 0;
  let tempValue = 0;
  let wAlpha = 0;
  let wBeta = 0;
  let val2 = 0;
  let val3 = 0;
  let greatest = 0;
  let tempCY = 0;
  let tempLT = 0;
  let tempHT = 0;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length || inClose.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* This function is very similar as ATR, except
   * it is being normalized as follow:
   *
   *    NATR = (ATR(period) / Close) * 100
   *
   *
   * Normalization make the ATR function more relevant
   * in the folllowing scenario:
   *    - Long term analysis where the price changes drastically.
   *    - Cross-market or cross-security ATR comparison.
   *
   * More Info:
   *      Technical Analysis of Stock & Commodities (TASC)
   *      May 2006 by John Forman
   */
  /* Average True Range is the greatest of the following:
   *
   *  val1 = distance from today's high to today's low.
   *  val2 = distance from yesterday's close to today's high.
   *  val3 = distance from yesterday's close to today's low.
   *
   * These value are averaged for the specified period using
   * Wilder method. This method have an unstable period comparable
   * to an Exponential Moving Average (EMA).
   */
  outBegIdx.value = 0;
  outNBElement.value = 0;
  /* Adjust startIdx to account for the lookback period. */
  lookbackTotal = natrLookbackPositional(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* wAlpha is derived FROM wBeta, never the reverse: only that order makes
   * wAlpha + wBeta exactly 1 (Sterbenz -- wBeta lands in [0.5, 1)), and it
   * measures closer to the exact recursion than the 1/period-first spelling
   * at nearly every period. The order is a gated contract, not a preference:
   * swapping it reddens the frozen-release comparison.
   */
  wBeta = (optInTimePeriod - 1) / optInTimePeriod;
  wAlpha = 1.0 - wBeta;
  /* The True Range of each bar is computed inline in a single
   * pass. No temporary buffer is needed.
   *
   * The arithmetic order below is the bit-exactness contract
   * (do not reorder):
   *  - True Range: start from high-low, then compare/replace
   *    with the two previous-close distances, in that order.
   *  - Seed: the first 'period' True Range values are summed,
   *    accumulated from 0.0 in input order, then divided by
   *    the period.
   *  - Wilder smoothing: ONE statement. Splitting it back
   *    unfuses the multiply-add and puts a second latency on
   *    the recurrence's dependency chain.
   *
   * Each output is normalized by the close of its own bar; a
   * close of zero yields 0.0.
   *
   * In-place (outReal being one of the input arrays) is
   * supported: each output is written only after every input
   * read at or before its bar, and the output index is always
   * smaller than the bar index of any remaining read.
   */
  /* The first True Range needs the two price bars at
   * startIdx-lookbackTotal+1 (a previous close is consumed).
   */
  today = startIdx - lookbackTotal + 1;
  /* Seed the ATR with a simple average of the True Range
   * for the first 'period' bars.
   */
  periodTotal = 0.0;
  i = optInTimePeriod;
  while (i-- > 0) {
    /* Find the greatest of the 3 values. */
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    periodTotal += greatest;
    today += 1;
  }
  prevATR = periodTotal / optInTimePeriod;
  /* Skip the unstable period. */
  i = lookbackTotal - optInTimePeriod;
  while (i !== 0) {
    /* Find the greatest of the 3 values. */
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    prevATR = fma(wBeta, prevATR, wAlpha * greatest);
    today += 1;
    i -= 1;
  }
  /* Now start to write the final NATR in the caller
   * provided outReal.
   */
  outIdx = 1;
  if (optInTimePeriod <= 1) {
    /* Period 1 is the raw True Range and is deliberately NOT normalized,
     * which is the TRANGE delegation this path replaced.
     */
    outReal[((0 * outStride) | 0)] = prevATR;
  } else {
    /* NATR is the ATR as a percentage of the close, so it is scale-free and
     * the divisor only has to be non-zero. An exact test, not the fixed
     * TA_IS_ZERO band it used to be: a close carries the quote unit, and that
     * band zeroed the whole output for any instrument quoted below it (#253).
     */
    tempValue = inClose[startIdx];
    if (tempValue !== 0.0) {
      outReal[((0 * outStride) | 0)] = prevATR / tempValue * 100.0;
    } else {
      outReal[((0 * outStride) | 0)] = 0.0;
    }
  }
  /* Now do the number of requested NATR. */
  nbATR = endIdx - startIdx + 1;
  while (--nbATR !== 0) {
    /* Find the greatest of the 3 values. */
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    prevATR = fma(wBeta, prevATR, wAlpha * greatest);
    if (optInTimePeriod <= 1) {
      outReal[((outIdx * outStride) | 0)] = prevATR;
    } else {
      tempValue = inClose[today];
      if (tempValue !== 0.0) {
        outReal[((outIdx * outStride) | 0)] = prevATR / tempValue * 100.0;
      } else {
        outReal[((outIdx * outStride) | 0)] = 0.0;
      }
    }
    outIdx += 1;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  sp.optInTimePeriod = optInTimePeriod;
  sp.prevATR = prevATR;
  sp.wAlpha = wAlpha;
  sp.wBeta = wBeta;
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function natrOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): NatrStream {
  const sp = NatrStream.of(core);
  const retCode = natrOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('NATR openAndFill', inHigh.length, startIdx, natrLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && natrLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('NATR openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('NATR openAndFill', retCode);
}

/** @internal */
export function natrOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): NatrStream {
  const sp = NatrStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = natrOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('NATR open', inHigh.length, startIdx, natrLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && natrLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('NATR open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('NATR open', retCode);
}

/** @internal */
export function natrOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): NatrStream {
  optInTimePeriod = requireInt32('NATR open', 'timePeriod', optInTimePeriod);
  requireArgument('NATR open', 'prices.high', inHigh);
  requireHistory('NATR open', 'prices.high', inHigh.length);
  requireArgument('NATR open', 'prices.low', inLow);
  requireArgument('NATR open', 'prices.close', inClose);
  requireHistoryLength('NATR open', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('NATR open', 'prices.close', inClose.length, inHigh.length);
  return natrOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInTimePeriod);
}

/** @internal */
export function natrOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): NatrStream {
  optInTimePeriod = requireInt32('NATR openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('NATR openAndFill', 'prices.high', inHigh);
  requireHistory('NATR openAndFill', 'prices.high', inHigh.length);
  requireArgument('NATR openAndFill', 'prices.low', inLow);
  requireArgument('NATR openAndFill', 'prices.close', inClose);
  const guardLookback = natrLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('NATR openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('NATR openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('NATR openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('NATR openAndFill', 'prices.close', inClose.length, inHigh.length);
  requireOutput('NATR openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('NATR openAndFill', 'outReal', 'prices.high');
  }
  if (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('NATR openAndFill', 'outReal', 'prices.low');
  }
  if (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('NATR openAndFill', 'outReal', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outReal, inHighF64)) {
    throw sharedMemory('NATR openAndFill', 'outReal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outReal, inLowF64)) {
    throw sharedMemory('NATR openAndFill', 'outReal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outReal, inCloseF64)) {
    throw sharedMemory('NATR openAndFill', 'outReal', 'prices.close');
  }
  return natrOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live NATR stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link natr} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar. `close`: Close
 *   price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link NatrOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `natrLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or
 *   histories of different lengths.
 *
 * @example
 * ```ts
 * import { natrOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = natrOpen({ high, low, close }, { timePeriod: 14 });
 * const peeked = stream.peek(101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function natrOpen(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: NatrOptions): NatrStream {
  requireArgument('NATR open', 'prices', prices);
  const o = optionsOf('NATR open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('NATR open', o.core);
  return natrOpenPositional(prices.high, prices.low, prices.close, optInTimePeriod, core);
}

/**
 * {@link natrOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link natr}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar. `close`: Close
 *   price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link NatrOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link natr} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `natrLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { natrOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = natrOpenAndFill({ high, low, close }, {
 *   timePeriod: 14,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function natrOpenAndFill(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: NatrOptions): NatrResult & { stream: NatrStream } {
  requireArgument('NATR openAndFill', 'prices', prices);
  const o = optionsOf('NATR openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('NATR openAndFill', o.core);
  const capacity = fillCapacity(natrLookbackPositional(optInTimePeriod, core), prices.high, prices.low, prices.close);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = natrOpenAndFillPositional(prices.high, prices.low, prices.close, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
