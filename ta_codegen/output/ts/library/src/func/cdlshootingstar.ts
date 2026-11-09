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
 *  AC       Angelo Ciceri
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  103004 AC     Creation
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 *  103126 MF,CC  Nest the cheap, selective pre-check conjuncts in an outer if
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { CandleSettingType, Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function cdlshootingstarLookbackPositional(core: Core = Core.DEFAULT): number {
  const BodyShort_avgPeriod = core.candleSettings[CandleSettingType.BODY_SHORT].avgPeriod;
  const ShadowLong_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_LONG].avgPeriod;
  const ShadowVeryShort_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].avgPeriod;
  return ((BodyShort_avgPeriod > ShadowLong_avgPeriod ? BodyShort_avgPeriod : ShadowLong_avgPeriod) > ShadowVeryShort_avgPeriod ? (BodyShort_avgPeriod > ShadowLong_avgPeriod ? BodyShort_avgPeriod : ShadowLong_avgPeriod) : ShadowVeryShort_avgPeriod) + 1;
}

/** @internal */
export function cdlshootingstarDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdlshootingstarImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): RetCode {
  let BodyPeriodTotal = 0;
  let ShadowLongPeriodTotal = 0;
  let ShadowVeryShortPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let BodyTrailingIdx = 0;
  let ShadowLongTrailingIdx = 0;
  let ShadowVeryShortTrailingIdx = 0;
  let lookbackTotal = 0;
  const BodyShort_rangeType: number = core.candleSettings[CandleSettingType.BODY_SHORT].rangeType;
  const BodyShort_avgPeriod = core.candleSettings[CandleSettingType.BODY_SHORT].avgPeriod;
  const BodyShort_factor = core.candleSettings[CandleSettingType.BODY_SHORT].factor;
  const ShadowLong_rangeType: number = core.candleSettings[CandleSettingType.SHADOW_LONG].rangeType;
  const ShadowLong_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_LONG].avgPeriod;
  const ShadowLong_factor = core.candleSettings[CandleSettingType.SHADOW_LONG].factor;
  const ShadowVeryShort_rangeType: number = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].rangeType;
  const ShadowVeryShort_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].avgPeriod;
  const ShadowVeryShort_factor = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].factor;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlshootingstarLookbackPositional(core);
  /* Move up the start index if there is not
   * enough initial data.
   */
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  /* Do the calculation using tight loops. */
  /* Add-up the initial period, except for the last value. */
  BodyPeriodTotal = 0;
  BodyTrailingIdx = startIdx - BodyShort_avgPeriod;
  ShadowLongPeriodTotal = 0;
  ShadowLongTrailingIdx = startIdx - ShadowLong_avgPeriod;
  ShadowVeryShortPeriodTotal = 0;
  ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
  i = BodyTrailingIdx;
  while (i < startIdx) {
    BodyPeriodTotal += ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = ShadowLongTrailingIdx;
  while (i < startIdx) {
    ShadowLongPeriodTotal += ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = ShadowVeryShortTrailingIdx;
  while (i < startIdx) {
    ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - small real body
   * - long upper shadow
   * - no, or very short, lower shadow
   * - gap up from prior real body
   * The meaning of "short", "very short" and "long" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (((inOpen[i] < inClose[i] ? inOpen[i] : inClose[i]) > (inOpen[i - 1] > inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1])) && /* gap up */
        Math.abs(inClose[i] - inOpen[i]) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (BodyPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) && /* small rb */
        (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowLong_factor * (((ShadowLong_avgPeriod !== 0) ? (ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowLong_rangeType === 2) ? 2.0 : 1.0)))) && /* long upper shadow */
        (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0))))) /* very short lower shadow */
    {
      outInteger[outIdx++] = -100;
    } else {
      outInteger[outIdx++] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyPeriodTotal += ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyShort_rangeType === 0) ? (Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : ((BodyShort_rangeType === 1) ? (inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) : ((BodyShort_rangeType === 2) ? ((inHigh[BodyTrailingIdx] - (((inClose[BodyTrailingIdx]) >= (inOpen[BodyTrailingIdx])) ? (inClose[BodyTrailingIdx]) : (inOpen[BodyTrailingIdx]))) + ((((inClose[BodyTrailingIdx]) >= (inOpen[BodyTrailingIdx])) ? (inOpen[BodyTrailingIdx]) : (inClose[BodyTrailingIdx])) - inLow[BodyTrailingIdx])) : 0.0)));
    ShadowLongPeriodTotal += ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[ShadowLongTrailingIdx] - inOpen[ShadowLongTrailingIdx])) : ((ShadowLong_rangeType === 1) ? (inHigh[ShadowLongTrailingIdx] - inLow[ShadowLongTrailingIdx]) : ((ShadowLong_rangeType === 2) ? ((inHigh[ShadowLongTrailingIdx] - (((inClose[ShadowLongTrailingIdx]) >= (inOpen[ShadowLongTrailingIdx])) ? (inClose[ShadowLongTrailingIdx]) : (inOpen[ShadowLongTrailingIdx]))) + ((((inClose[ShadowLongTrailingIdx]) >= (inOpen[ShadowLongTrailingIdx])) ? (inOpen[ShadowLongTrailingIdx]) : (inClose[ShadowLongTrailingIdx])) - inLow[ShadowLongTrailingIdx])) : 0.0)));
    ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx] - inOpen[ShadowVeryShortTrailingIdx])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[ShadowVeryShortTrailingIdx] - inLow[ShadowVeryShortTrailingIdx]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[ShadowVeryShortTrailingIdx] - (((inClose[ShadowVeryShortTrailingIdx]) >= (inOpen[ShadowVeryShortTrailingIdx])) ? (inClose[ShadowVeryShortTrailingIdx]) : (inOpen[ShadowVeryShortTrailingIdx]))) + ((((inClose[ShadowVeryShortTrailingIdx]) >= (inOpen[ShadowVeryShortTrailingIdx])) ? (inOpen[ShadowVeryShortTrailingIdx]) : (inClose[ShadowVeryShortTrailingIdx])) - inLow[ShadowVeryShortTrailingIdx])) : 0.0)));
    i += 1;
    BodyTrailingIdx += 1;
    ShadowLongTrailingIdx += 1;
    ShadowVeryShortTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlshootingstarPositional(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDLSHOOTINGSTAR', 'startIdx', startIdx);
  endIdx = requireInt32('CDLSHOOTINGSTAR', 'endIdx', endIdx);
  requireIndexRange('CDLSHOOTINGSTAR', startIdx, endIdx);
  const guardStart = clampedStart('CDLSHOOTINGSTAR', startIdx, cdlshootingstarLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLSHOOTINGSTAR', 'prices.open', inOpen, guardInLen);
  requireLength('CDLSHOOTINGSTAR', 'prices.high', inHigh, guardInLen);
  requireLength('CDLSHOOTINGSTAR', 'prices.low', inLow, guardInLen);
  requireLength('CDLSHOOTINGSTAR', 'prices.close', inClose, guardInLen);
  requireOutput('CDLSHOOTINGSTAR', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLSHOOTINGSTAR', 'outInteger', 'prices.open');
  }
  if (overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLSHOOTINGSTAR', 'outInteger', 'prices.high');
  }
  if (overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLSHOOTINGSTAR', 'outInteger', 'prices.low');
  }
  if (overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLSHOOTINGSTAR', 'outInteger', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdlshootingstarImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLSHOOTINGSTAR', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cdlshootingstar}, {@link cdlshootingstarLookback},
 * {@link cdlshootingstarDisplayShift}, {@link cdlshootingstarOpen} and
 * {@link cdlshootingstarOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface CdlshootingstarOptions {
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
   * buffer or with an input.
   */
  outInteger?: Int32Array<ArrayBuffer> | undefined;
}

/**
 * What {@link cdlshootingstar} returns, and {@link cdlshootingstarOpenAndFill}
 * with its stream.
 */
export interface CdlshootingstarResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * -100 when the shooting star is detected, 0 otherwise. Only ever emits negative
   * (bearish); never +100. `values[k]` is the value at input bar `begIdx + k`.
   */
  values: Int32Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link cdlshootingstar} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlshootingstarOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlshootingstarLookback(options?: CdlshootingstarOptions): number {
  const o = optionsOf('CDLSHOOTINGSTAR lookback', options);
  return cdlshootingstarLookbackPositional(coreOf('CDLSHOOTINGSTAR lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdlshootingstar}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlshootingstarOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlshootingstarDisplayShift(outputIdx: number, options?: CdlshootingstarOptions): number {
  const o = optionsOf('CDLSHOOTINGSTAR displayShift', options);
  return cdlshootingstarDisplayShiftPositional(outputIdx, coreOf('CDLSHOOTINGSTAR displayShift', o.core));
}

/**
 * Single-candle pattern: a small real body with a long upper shadow and
 * little-to-no lower shadow that gaps up from the prior candle's real body.
 * Bearish reversal signal. A hit (-100) flags a bearish reversal at the top of an
 * uptrend.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlshootingstar](https://ta-lib.org/functions/cdlshootingstar).
 *
 * **Notes**
 * - A preceding uptrend is not verified.
 * - Bulkowski found this reverses only 59% of the time — "near random," summarized
 *   in his words as "this candle looks better than it performs" — ranking 55th of
 *   103 patterns.
 *   ([thepatternsite.com](https://thepatternsite.com/ShootingStar.html))
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cdlshootingstarLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `open`: Open price of
 *   each bar. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link CdlshootingstarOptions}.
 * @returns `begIdx` and the values; see {@link CdlshootingstarResult}. Each output
 *   is a view of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, an input too short for the range, a buffer that is empty, too short or of
 *   the wrong type or a buffer sharing memory with a `Float64Array` input it is not.
 *   Checked before anything is written.
 *
 * @example
 * ```ts
 * import { cdlshootingstar } from 'ta-lib';
 *
 * const open = new Float64Array(252); // your data, oldest first
 * const high = new Float64Array(252); // your data, oldest first
 * const low = new Float64Array(252); // your data, oldest first
 * const close = new Float64Array(252); // your data, oldest first
 *
 * const { begIdx, values } = cdlshootingstar({ open, high, low, close });
 * console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
 * ```
 * @see [`cdlinvertedhammer`](https://ta-lib.org/functions/cdlinvertedhammer)
 * @see [`cdlhangingman`](https://ta-lib.org/functions/cdlhangingman)
 * @see [`cdlhammer`](https://ta-lib.org/functions/cdlhammer)
 * @see [`cdlgravestonedoji`](https://ta-lib.org/functions/cdlgravestonedoji)
 */
export function cdlshootingstar(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlshootingstarOptions): CdlshootingstarResult {
  requireArgument('CDLSHOOTINGSTAR', 'prices', prices);
  const o = optionsOf('CDLSHOOTINGSTAR', options);
  if (o.endIdx === undefined) {
    requireSeries('CDLSHOOTINGSTAR', 'prices.open', prices.open);
    requireSeries('CDLSHOOTINGSTAR', 'prices.high', prices.high);
    requireSeries('CDLSHOOTINGSTAR', 'prices.low', prices.low);
    requireSeries('CDLSHOOTINGSTAR', 'prices.close', prices.close);
  }
  const core = coreOf('CDLSHOOTINGSTAR', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cdlshootingstarLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const range = cdlshootingstarPositional(startIdx, endIdx, prices.open, prices.high, prices.low, prices.close, outInteger, core);
  return { begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CDLSHOOTINGSTAR stream: one value per closed bar, bit-identical to
 * {@link cdlshootingstar} over the same series. Open it with
 * {@link cdlshootingstarOpen} or {@link cdlshootingstarOpenAndFill}; there is
 * nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlshootingstar](https://ta-lib.org/functions/cdlshootingstar).
 */
export class CdlshootingstarStream {
  /** @internal */
  core: Core;
  /** @internal */
  BodyPeriodTotal: number = 0;
  /** @internal */
  ShadowLongPeriodTotal: number = 0;
  /** @internal */
  ShadowVeryShortPeriodTotal: number = 0;
  /** @internal */
  lag1_inOpen: number = 0;
  /** @internal */
  lag1_inClose: number = 0;
  /** @internal */
  ringPos_BodyTrailingIdx: number = 0;
  /** @internal */
  ringCap_BodyTrailingIdx: number = 0;
  /** @internal */
  ring_BodyTrailingIdx_derived: Float64Array = new Float64Array(1);
  /** @internal */
  ringPos_ShadowLongTrailingIdx: number = 0;
  /** @internal */
  ringCap_ShadowLongTrailingIdx: number = 0;
  /** @internal */
  ring_ShadowLongTrailingIdx_derived: Float64Array = new Float64Array(1);
  /** @internal */
  ringPos_ShadowVeryShortTrailingIdx: number = 0;
  /** @internal */
  ringCap_ShadowVeryShortTrailingIdx: number = 0;
  /** @internal */
  ring_ShadowVeryShortTrailingIdx_derived: Float64Array = new Float64Array(1);
  /** @internal */
  cs_BodyShort_rangeType: number = 0;
  /** @internal */
  cs_BodyShort_avgPeriod: number = 0;
  /** @internal */
  cs_BodyShort_factor: number = 0;
  /** @internal */
  cs_ShadowLong_rangeType: number = 0;
  /** @internal */
  cs_ShadowLong_avgPeriod: number = 0;
  /** @internal */
  cs_ShadowLong_factor: number = 0;
  /** @internal */
  cs_ShadowVeryShort_rangeType: number = 0;
  /** @internal */
  cs_ShadowVeryShort_avgPeriod: number = 0;
  /** @internal */
  cs_ShadowVeryShort_factor: number = 0;
  /** @internal */
  cur_outInteger: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): CdlshootingstarStream {
    return new CdlshootingstarStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cdlshootingstar} reports them over the same bars. Each accepted `update`
   * and each `advance` adds one to `count`.
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
      throw streamFailure('CDLSHOOTINGSTAR advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param open - The bar's `open` value: Open price of each bar.
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(open: number, high: number, low: number, close: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('CDLSHOOTINGSTAR update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(open) || !Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('CDLSHOOTINGSTAR update', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    cdlshootingstarStepImpl(this, open, high, low, close);
    this.outRangeCount++;
    return this.cur_outInteger;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param open - The bar's `open` value: Open price of each bar.
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(open: number, high: number, low: number, close: number): number {
    if (!Number.isFinite(open) || !Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('CDLSHOOTINGSTAR peek', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inOpen = open;
    const inHigh = high;
    const inLow = low;
    const inClose = close;
    let cur_outInteger = 0;
    const BodyShort_rangeType = sp.cs_BodyShort_rangeType;
    const BodyShort_avgPeriod = sp.cs_BodyShort_avgPeriod;
    const BodyShort_factor = sp.cs_BodyShort_factor;
    const ShadowLong_rangeType = sp.cs_ShadowLong_rangeType;
    const ShadowLong_avgPeriod = sp.cs_ShadowLong_avgPeriod;
    const ShadowLong_factor = sp.cs_ShadowLong_factor;
    const ShadowVeryShort_rangeType = sp.cs_ShadowVeryShort_rangeType;
    const ShadowVeryShort_avgPeriod = sp.cs_ShadowVeryShort_avgPeriod;
    const ShadowVeryShort_factor = sp.cs_ShadowVeryShort_factor;
    if (((inOpen < inClose ? inOpen : inClose) > (sp.lag1_inOpen > sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose)) && /* gap up */
        Math.abs(inClose - inOpen) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (sp.BodyPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) && /* small rb */
        (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) > ((ShadowLong_factor * (((ShadowLong_avgPeriod !== 0) ? (sp.ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowLong_rangeType === 2) ? 2.0 : 1.0)))) && /* long upper shadow */
        (((inClose >= inOpen) ? inOpen : inClose) - inLow) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (sp.ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0))))) /* very short lower shadow */
    {
      cur_outInteger = -100;
    } else {
      cur_outInteger = 0;
    }
    return cur_outInteger;
  }

  /** The value at the last bar this stream counted; `peek` does not change it. */
  value(): number {
    return this.cur_outInteger;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): CdlshootingstarStream {
    const c = new CdlshootingstarStream(this.core);
    c.BodyPeriodTotal = this.BodyPeriodTotal;
    c.ShadowLongPeriodTotal = this.ShadowLongPeriodTotal;
    c.ShadowVeryShortPeriodTotal = this.ShadowVeryShortPeriodTotal;
    c.lag1_inOpen = this.lag1_inOpen;
    c.lag1_inClose = this.lag1_inClose;
    c.ringPos_BodyTrailingIdx = this.ringPos_BodyTrailingIdx;
    c.ringCap_BodyTrailingIdx = this.ringCap_BodyTrailingIdx;
    c.ring_BodyTrailingIdx_derived = this.ring_BodyTrailingIdx_derived.slice();
    c.ringPos_ShadowLongTrailingIdx = this.ringPos_ShadowLongTrailingIdx;
    c.ringCap_ShadowLongTrailingIdx = this.ringCap_ShadowLongTrailingIdx;
    c.ring_ShadowLongTrailingIdx_derived = this.ring_ShadowLongTrailingIdx_derived.slice();
    c.ringPos_ShadowVeryShortTrailingIdx = this.ringPos_ShadowVeryShortTrailingIdx;
    c.ringCap_ShadowVeryShortTrailingIdx = this.ringCap_ShadowVeryShortTrailingIdx;
    c.ring_ShadowVeryShortTrailingIdx_derived = this.ring_ShadowVeryShortTrailingIdx_derived.slice();
    c.cs_BodyShort_rangeType = this.cs_BodyShort_rangeType;
    c.cs_BodyShort_avgPeriod = this.cs_BodyShort_avgPeriod;
    c.cs_BodyShort_factor = this.cs_BodyShort_factor;
    c.cs_ShadowLong_rangeType = this.cs_ShadowLong_rangeType;
    c.cs_ShadowLong_avgPeriod = this.cs_ShadowLong_avgPeriod;
    c.cs_ShadowLong_factor = this.cs_ShadowLong_factor;
    c.cs_ShadowVeryShort_rangeType = this.cs_ShadowVeryShort_rangeType;
    c.cs_ShadowVeryShort_avgPeriod = this.cs_ShadowVeryShort_avgPeriod;
    c.cs_ShadowVeryShort_factor = this.cs_ShadowVeryShort_factor;
    c.cur_outInteger = this.cur_outInteger;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cdlshootingstarStepImpl(sp: CdlshootingstarStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  let ringPosL_BodyTrailingIdx = 0;
  let ringPosL_ShadowLongTrailingIdx = 0;
  let ringPosL_ShadowVeryShortTrailingIdx = 0;
  const BodyShort_rangeType = sp.cs_BodyShort_rangeType;
  const BodyShort_avgPeriod = sp.cs_BodyShort_avgPeriod;
  const BodyShort_factor = sp.cs_BodyShort_factor;
  const ShadowLong_rangeType = sp.cs_ShadowLong_rangeType;
  const ShadowLong_avgPeriod = sp.cs_ShadowLong_avgPeriod;
  const ShadowLong_factor = sp.cs_ShadowLong_factor;
  const ShadowVeryShort_rangeType = sp.cs_ShadowVeryShort_rangeType;
  const ShadowVeryShort_avgPeriod = sp.cs_ShadowVeryShort_avgPeriod;
  const ShadowVeryShort_factor = sp.cs_ShadowVeryShort_factor;
  if (sp.ringCap_BodyTrailingIdx === 0) {
    sp.ring_BodyTrailingIdx_derived[0] = ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
  if (sp.ringCap_ShadowLongTrailingIdx === 0) {
    sp.ring_ShadowLongTrailingIdx_derived[0] = ((ShadowLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
  if (sp.ringCap_ShadowVeryShortTrailingIdx === 0) {
    sp.ring_ShadowVeryShortTrailingIdx_derived[0] = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
  if (((inOpen < inClose ? inOpen : inClose) > (sp.lag1_inOpen > sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose)) && /* gap up */
      Math.abs(inClose - inOpen) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (sp.BodyPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) && /* small rb */
      (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) > ((ShadowLong_factor * (((ShadowLong_avgPeriod !== 0) ? (sp.ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowLong_rangeType === 2) ? 2.0 : 1.0)))) && /* long upper shadow */
      (((inClose >= inOpen) ? inOpen : inClose) - inLow) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (sp.ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0))))) /* very short lower shadow */
  {
    sp.cur_outInteger = -100;
  } else {
    sp.cur_outInteger = 0;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  sp.BodyPeriodTotal += ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0))) - sp.ring_BodyTrailingIdx_derived[sp.ringPos_BodyTrailingIdx];
  sp.ShadowLongPeriodTotal += ((ShadowLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0))) - sp.ring_ShadowLongTrailingIdx_derived[sp.ringPos_ShadowLongTrailingIdx];
  sp.ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0))) - sp.ring_ShadowVeryShortTrailingIdx_derived[sp.ringPos_ShadowVeryShortTrailingIdx];
  sp.lag1_inOpen = inOpen;
  sp.lag1_inClose = inClose;
  ringPosL_BodyTrailingIdx = sp.ringPos_BodyTrailingIdx;
  sp.ring_BodyTrailingIdx_derived[ringPosL_BodyTrailingIdx] = ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  ringPosL_BodyTrailingIdx = ringPosL_BodyTrailingIdx + 1;
  ringPosL_BodyTrailingIdx = ringPosL_BodyTrailingIdx & (ringPosL_BodyTrailingIdx - sp.ringCap_BodyTrailingIdx >> 31);
  sp.ringPos_BodyTrailingIdx = ringPosL_BodyTrailingIdx;
  ringPosL_ShadowLongTrailingIdx = sp.ringPos_ShadowLongTrailingIdx;
  sp.ring_ShadowLongTrailingIdx_derived[ringPosL_ShadowLongTrailingIdx] = ((ShadowLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  ringPosL_ShadowLongTrailingIdx = ringPosL_ShadowLongTrailingIdx + 1;
  ringPosL_ShadowLongTrailingIdx = ringPosL_ShadowLongTrailingIdx & (ringPosL_ShadowLongTrailingIdx - sp.ringCap_ShadowLongTrailingIdx >> 31);
  sp.ringPos_ShadowLongTrailingIdx = ringPosL_ShadowLongTrailingIdx;
  ringPosL_ShadowVeryShortTrailingIdx = sp.ringPos_ShadowVeryShortTrailingIdx;
  sp.ring_ShadowVeryShortTrailingIdx_derived[ringPosL_ShadowVeryShortTrailingIdx] = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  ringPosL_ShadowVeryShortTrailingIdx = ringPosL_ShadowVeryShortTrailingIdx + 1;
  ringPosL_ShadowVeryShortTrailingIdx = ringPosL_ShadowVeryShortTrailingIdx & (ringPosL_ShadowVeryShortTrailingIdx - sp.ringCap_ShadowVeryShortTrailingIdx >> 31);
  sp.ringPos_ShadowVeryShortTrailingIdx = ringPosL_ShadowVeryShortTrailingIdx;
}

function cdlshootingstarOpenImpl(
  sp: CdlshootingstarStream,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let BodyPeriodTotal = 0;
  let ShadowLongPeriodTotal = 0;
  let ShadowVeryShortPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let BodyTrailingIdx = 0;
  let ShadowLongTrailingIdx = 0;
  let ShadowVeryShortTrailingIdx = 0;
  let lookbackTotal = 0;
  const historyLen = inOpen.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inHigh.length !== inOpen.length || inLow.length !== inOpen.length || inClose.length !== inOpen.length) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const BodyShort_rangeType: number = core.candleSettings[CandleSettingType.BODY_SHORT].rangeType;
  const BodyShort_avgPeriod = core.candleSettings[CandleSettingType.BODY_SHORT].avgPeriod;
  const BodyShort_factor = core.candleSettings[CandleSettingType.BODY_SHORT].factor;
  const ShadowLong_rangeType: number = core.candleSettings[CandleSettingType.SHADOW_LONG].rangeType;
  const ShadowLong_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_LONG].avgPeriod;
  const ShadowLong_factor = core.candleSettings[CandleSettingType.SHADOW_LONG].factor;
  const ShadowVeryShort_rangeType: number = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].rangeType;
  const ShadowVeryShort_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].avgPeriod;
  const ShadowVeryShort_factor = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].factor;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlshootingstarLookbackPositional(core);
  /* Move up the start index if there is not
   * enough initial data.
   */
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Do the calculation using tight loops. */
  /* Add-up the initial period, except for the last value. */
  BodyPeriodTotal = 0;
  BodyTrailingIdx = startIdx - BodyShort_avgPeriod;
  ShadowLongPeriodTotal = 0;
  ShadowLongTrailingIdx = startIdx - ShadowLong_avgPeriod;
  ShadowVeryShortPeriodTotal = 0;
  ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
  i = BodyTrailingIdx;
  while (i < startIdx) {
    BodyPeriodTotal += ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = ShadowLongTrailingIdx;
  while (i < startIdx) {
    ShadowLongPeriodTotal += ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = ShadowVeryShortTrailingIdx;
  while (i < startIdx) {
    ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - small real body
   * - long upper shadow
   * - no, or very short, lower shadow
   * - gap up from prior real body
   * The meaning of "short", "very short" and "long" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (((inOpen[i] < inClose[i] ? inOpen[i] : inClose[i]) > (inOpen[i - 1] > inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1])) && /* gap up */
        Math.abs(inClose[i] - inOpen[i]) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (BodyPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) && /* small rb */
        (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowLong_factor * (((ShadowLong_avgPeriod !== 0) ? (ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowLong_rangeType === 2) ? 2.0 : 1.0)))) && /* long upper shadow */
        (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0))))) /* very short lower shadow */
    {
      outInteger[((outIdx++ * outStride) | 0)] = -100;
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyPeriodTotal += ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyShort_rangeType === 0) ? (Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : ((BodyShort_rangeType === 1) ? (inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) : ((BodyShort_rangeType === 2) ? ((inHigh[BodyTrailingIdx] - (((inClose[BodyTrailingIdx]) >= (inOpen[BodyTrailingIdx])) ? (inClose[BodyTrailingIdx]) : (inOpen[BodyTrailingIdx]))) + ((((inClose[BodyTrailingIdx]) >= (inOpen[BodyTrailingIdx])) ? (inOpen[BodyTrailingIdx]) : (inClose[BodyTrailingIdx])) - inLow[BodyTrailingIdx])) : 0.0)));
    ShadowLongPeriodTotal += ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[ShadowLongTrailingIdx] - inOpen[ShadowLongTrailingIdx])) : ((ShadowLong_rangeType === 1) ? (inHigh[ShadowLongTrailingIdx] - inLow[ShadowLongTrailingIdx]) : ((ShadowLong_rangeType === 2) ? ((inHigh[ShadowLongTrailingIdx] - (((inClose[ShadowLongTrailingIdx]) >= (inOpen[ShadowLongTrailingIdx])) ? (inClose[ShadowLongTrailingIdx]) : (inOpen[ShadowLongTrailingIdx]))) + ((((inClose[ShadowLongTrailingIdx]) >= (inOpen[ShadowLongTrailingIdx])) ? (inOpen[ShadowLongTrailingIdx]) : (inClose[ShadowLongTrailingIdx])) - inLow[ShadowLongTrailingIdx])) : 0.0)));
    ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx] - inOpen[ShadowVeryShortTrailingIdx])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[ShadowVeryShortTrailingIdx] - inLow[ShadowVeryShortTrailingIdx]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[ShadowVeryShortTrailingIdx] - (((inClose[ShadowVeryShortTrailingIdx]) >= (inOpen[ShadowVeryShortTrailingIdx])) ? (inClose[ShadowVeryShortTrailingIdx]) : (inOpen[ShadowVeryShortTrailingIdx]))) + ((((inClose[ShadowVeryShortTrailingIdx]) >= (inOpen[ShadowVeryShortTrailingIdx])) ? (inOpen[ShadowVeryShortTrailingIdx]) : (inClose[ShadowVeryShortTrailingIdx])) - inLow[ShadowVeryShortTrailingIdx])) : 0.0)));
    i += 1;
    BodyTrailingIdx += 1;
    ShadowLongTrailingIdx += 1;
    ShadowVeryShortTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const cap_BodyTrailingIdx = i - BodyTrailingIdx;
  if (cap_BodyTrailingIdx < 0 || cap_BodyTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_BodyTrailingIdx = cap_BodyTrailingIdx > 0 ? cap_BodyTrailingIdx : 1;
  const capRing_BodyTrailingIdx_derived = new Float64Array(allocN_BodyTrailingIdx);
  for (let fillJ = historyLen - cap_BodyTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_BodyTrailingIdx_derived[fillJ - (historyLen - cap_BodyTrailingIdx)] = ((BodyShort_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyShort_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyShort_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  const cap_ShadowLongTrailingIdx = i - ShadowLongTrailingIdx;
  if (cap_ShadowLongTrailingIdx < 0 || cap_ShadowLongTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_ShadowLongTrailingIdx = cap_ShadowLongTrailingIdx > 0 ? cap_ShadowLongTrailingIdx : 1;
  const capRing_ShadowLongTrailingIdx_derived = new Float64Array(allocN_ShadowLongTrailingIdx);
  for (let fillJ = historyLen - cap_ShadowLongTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_ShadowLongTrailingIdx_derived[fillJ - (historyLen - cap_ShadowLongTrailingIdx)] = ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((ShadowLong_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((ShadowLong_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  const cap_ShadowVeryShortTrailingIdx = i - ShadowVeryShortTrailingIdx;
  if (cap_ShadowVeryShortTrailingIdx < 0 || cap_ShadowVeryShortTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_ShadowVeryShortTrailingIdx = cap_ShadowVeryShortTrailingIdx > 0 ? cap_ShadowVeryShortTrailingIdx : 1;
  const capRing_ShadowVeryShortTrailingIdx_derived = new Float64Array(allocN_ShadowVeryShortTrailingIdx);
  for (let fillJ = historyLen - cap_ShadowVeryShortTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_ShadowVeryShortTrailingIdx_derived[fillJ - (historyLen - cap_ShadowVeryShortTrailingIdx)] = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  sp.BodyPeriodTotal = BodyPeriodTotal;
  sp.ShadowLongPeriodTotal = ShadowLongPeriodTotal;
  sp.ShadowVeryShortPeriodTotal = ShadowVeryShortPeriodTotal;
  sp.lag1_inOpen = inOpen[historyLen - 1];
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.ringPos_BodyTrailingIdx = 0;
  sp.ringCap_BodyTrailingIdx = cap_BodyTrailingIdx;
  sp.ring_BodyTrailingIdx_derived = capRing_BodyTrailingIdx_derived;
  sp.ringPos_ShadowLongTrailingIdx = 0;
  sp.ringCap_ShadowLongTrailingIdx = cap_ShadowLongTrailingIdx;
  sp.ring_ShadowLongTrailingIdx_derived = capRing_ShadowLongTrailingIdx_derived;
  sp.ringPos_ShadowVeryShortTrailingIdx = 0;
  sp.ringCap_ShadowVeryShortTrailingIdx = cap_ShadowVeryShortTrailingIdx;
  sp.ring_ShadowVeryShortTrailingIdx_derived = capRing_ShadowVeryShortTrailingIdx_derived;
  sp.cs_BodyShort_rangeType = BodyShort_rangeType;
  sp.cs_BodyShort_avgPeriod = BodyShort_avgPeriod;
  sp.cs_BodyShort_factor = BodyShort_factor;
  sp.cs_ShadowLong_rangeType = ShadowLong_rangeType;
  sp.cs_ShadowLong_avgPeriod = ShadowLong_avgPeriod;
  sp.cs_ShadowLong_factor = ShadowLong_factor;
  sp.cs_ShadowVeryShort_rangeType = ShadowVeryShort_rangeType;
  sp.cs_ShadowVeryShort_avgPeriod = ShadowVeryShort_avgPeriod;
  sp.cs_ShadowVeryShort_factor = ShadowVeryShort_factor;
  sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlshootingstarOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): CdlshootingstarStream {
  const sp = CdlshootingstarStream.of(core);
  const retCode = cdlshootingstarOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLSHOOTINGSTAR openAndFill', inOpen.length, startIdx, cdlshootingstarLookbackPositional(core));
  }
  throw streamFailure('CDLSHOOTINGSTAR openAndFill', retCode);
}

/** @internal */
export function cdlshootingstarOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): CdlshootingstarStream {
  const sp = CdlshootingstarStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdlshootingstarOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLSHOOTINGSTAR open', inOpen.length, startIdx, cdlshootingstarLookbackPositional(core));
  }
  throw streamFailure('CDLSHOOTINGSTAR open', retCode);
}

/** @internal */
export function cdlshootingstarOpenPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): CdlshootingstarStream {
  requireArgument('CDLSHOOTINGSTAR open', 'prices.open', inOpen);
  requireHistory('CDLSHOOTINGSTAR open', 'prices.open', inOpen.length);
  requireArgument('CDLSHOOTINGSTAR open', 'prices.high', inHigh);
  requireArgument('CDLSHOOTINGSTAR open', 'prices.low', inLow);
  requireArgument('CDLSHOOTINGSTAR open', 'prices.close', inClose);
  requireHistoryLength('CDLSHOOTINGSTAR open', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLSHOOTINGSTAR open', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLSHOOTINGSTAR open', 'prices.close', inClose.length, inOpen.length);
  return cdlshootingstarOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/** @internal */
export function cdlshootingstarOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): CdlshootingstarStream {
  requireArgument('CDLSHOOTINGSTAR openAndFill', 'prices.open', inOpen);
  requireHistory('CDLSHOOTINGSTAR openAndFill', 'prices.open', inOpen.length);
  requireArgument('CDLSHOOTINGSTAR openAndFill', 'prices.high', inHigh);
  requireArgument('CDLSHOOTINGSTAR openAndFill', 'prices.low', inLow);
  requireArgument('CDLSHOOTINGSTAR openAndFill', 'prices.close', inClose);
  const guardOutLen = openFillCount('CDLSHOOTINGSTAR openAndFill', inOpen.length, cdlshootingstarLookbackPositional(core));
  requireHistoryLength('CDLSHOOTINGSTAR openAndFill', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLSHOOTINGSTAR openAndFill', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLSHOOTINGSTAR openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('CDLSHOOTINGSTAR openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('CDLSHOOTINGSTAR openAndFill', 'outInteger', 'prices.open');
  }
  if (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CDLSHOOTINGSTAR openAndFill', 'outInteger', 'prices.high');
  }
  if (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CDLSHOOTINGSTAR openAndFill', 'outInteger', 'prices.low');
  }
  if (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('CDLSHOOTINGSTAR openAndFill', 'outInteger', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLSHOOTINGSTAR openAndFill', 'outInteger', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLSHOOTINGSTAR openAndFill', 'outInteger', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLSHOOTINGSTAR openAndFill', 'outInteger', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLSHOOTINGSTAR openAndFill', 'outInteger', 'prices.close');
  }
  return cdlshootingstarOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}

/**
 * Opens a live CDLSHOOTINGSTAR stream over the warm-up history: `value()` starts
 * at the last history bar, bit-identical to {@link cdlshootingstar} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlshootingstarOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlshootingstarLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input or histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdlshootingstarOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdlshootingstarOpen({ open, high, low, close });
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdlshootingstarOpen(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlshootingstarOptions): CdlshootingstarStream {
  requireArgument('CDLSHOOTINGSTAR open', 'prices', prices);
  const o = optionsOf('CDLSHOOTINGSTAR open', options);
  const core = coreOf('CDLSHOOTINGSTAR open', o.core);
  return cdlshootingstarOpenPositional(prices.open, prices.high, prices.low, prices.close, core);
}

/**
 * {@link cdlshootingstarOpen} that also computes the outputs over the whole
 * history in the same pass, bit-identically to {@link cdlshootingstar}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CdlshootingstarOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cdlshootingstar} returns them; the range is
 *   the stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlshootingstarLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, histories of different lengths or a buffer too short,
 *   of the wrong type or starting where another buffer or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdlshootingstarOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = cdlshootingstarOpenAndFill({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cdlshootingstarOpenAndFill(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlshootingstarOptions): CdlshootingstarResult & { stream: CdlshootingstarStream } {
  requireArgument('CDLSHOOTINGSTAR openAndFill', 'prices', prices);
  const o = optionsOf('CDLSHOOTINGSTAR openAndFill', options);
  const core = coreOf('CDLSHOOTINGSTAR openAndFill', o.core);
  const capacity = fillCapacity(cdlshootingstarLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const stream = cdlshootingstarOpenAndFillPositional(prices.open, prices.high, prices.low, prices.close, outInteger, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}
