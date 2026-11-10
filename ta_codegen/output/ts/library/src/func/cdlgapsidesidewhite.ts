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
 *  020605 AC     Creation
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 *  103126 MF,CC  Pre-check the two colour tests in an outer if, the gaps inside
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { CandleSettingType, Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function cdlgapsidesidewhiteLookbackPositional(core: Core = Core.DEFAULT): number {
  const Equal_avgPeriod = core.candleSettings[CandleSettingType.EQUAL].avgPeriod;
  const Near_avgPeriod = core.candleSettings[CandleSettingType.NEAR].avgPeriod;
  return (Near_avgPeriod > Equal_avgPeriod ? Near_avgPeriod : Equal_avgPeriod) + 2;
}

/** @internal */
export function cdlgapsidesidewhiteDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdlgapsidesidewhiteImpl(
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
  let NearPeriodTotal = 0;
  let EqualPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let NearTrailingIdx = 0;
  let EqualTrailingIdx = 0;
  let lookbackTotal = 0;
  const Equal_rangeType: number = core.candleSettings[CandleSettingType.EQUAL].rangeType;
  const Equal_avgPeriod = core.candleSettings[CandleSettingType.EQUAL].avgPeriod;
  const Equal_factor = core.candleSettings[CandleSettingType.EQUAL].factor;
  const Near_rangeType: number = core.candleSettings[CandleSettingType.NEAR].rangeType;
  const Near_avgPeriod = core.candleSettings[CandleSettingType.NEAR].avgPeriod;
  const Near_factor = core.candleSettings[CandleSettingType.NEAR].factor;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlgapsidesidewhiteLookbackPositional(core);
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
  NearPeriodTotal = 0;
  EqualPeriodTotal = 0;
  NearTrailingIdx = startIdx - Near_avgPeriod;
  EqualTrailingIdx = startIdx - Equal_avgPeriod;
  i = NearTrailingIdx;
  while (i < startIdx) {
    NearPeriodTotal += ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    i += 1;
  }
  i = EqualTrailingIdx;
  while (i < startIdx) {
    EqualPeriodTotal += ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - upside or downside gap (between the bodies)
   * - first candle after the window: white candlestick
   * - second candle after the window: white candlestick with similar size (near the same) and about the same
   *   open (equal) of the previous candle
   * - the second candle does not close the window
   * The meaning of "near" and "equal" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === 1 && /* 2nd: white */
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === 1 &&         /* 3rd: white */
        (((inOpen[i - 1] < inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1]) > (inOpen[i - 2] > inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2])) && /* upside or downside gap between the 1st candle and both the next 2 candles */
          ((inOpen[i] < inClose[i] ? inOpen[i] : inClose[i]) > (inOpen[i - 2] > inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2])) ||
         ((inOpen[i - 1] > inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1]) < (inOpen[i - 2] < inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2])) &&
          ((inOpen[i] > inClose[i] ? inOpen[i] : inClose[i]) < (inOpen[i - 2] < inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2]))) &&
        Math.abs(inClose[i] - inOpen[i]) >= Math.abs(inClose[i - 1] - inOpen[i - 1]) - ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) && /* same size 2 and 3 */
        Math.abs(inClose[i] - inOpen[i]) <= Math.abs(inClose[i - 1] - inOpen[i - 1]) + ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
        inOpen[i] >= inOpen[i - 1] - ((Equal_factor * (((Equal_avgPeriod !== 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))) && /* same open 2 and 3 */
        inOpen[i] <= inOpen[i - 1] + ((Equal_factor * (((Equal_avgPeriod !== 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))))
    {
      outInteger[outIdx++] = ((inOpen[i - 1] < inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1]) > (inOpen[i - 2] > inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2])) ? 100 : -100;
    } else {
      outInteger[outIdx++] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    NearPeriodTotal += ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((Near_rangeType === 0) ? (Math.abs(inClose[NearTrailingIdx - 1] - inOpen[NearTrailingIdx - 1])) : ((Near_rangeType === 1) ? (inHigh[NearTrailingIdx - 1] - inLow[NearTrailingIdx - 1]) : ((Near_rangeType === 2) ? ((inHigh[NearTrailingIdx - 1] - (((inClose[NearTrailingIdx - 1]) >= (inOpen[NearTrailingIdx - 1])) ? (inClose[NearTrailingIdx - 1]) : (inOpen[NearTrailingIdx - 1]))) + ((((inClose[NearTrailingIdx - 1]) >= (inOpen[NearTrailingIdx - 1])) ? (inOpen[NearTrailingIdx - 1]) : (inClose[NearTrailingIdx - 1])) - inLow[NearTrailingIdx - 1])) : 0.0)));
    EqualPeriodTotal += ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((Equal_rangeType === 0) ? (Math.abs(inClose[EqualTrailingIdx - 1] - inOpen[EqualTrailingIdx - 1])) : ((Equal_rangeType === 1) ? (inHigh[EqualTrailingIdx - 1] - inLow[EqualTrailingIdx - 1]) : ((Equal_rangeType === 2) ? ((inHigh[EqualTrailingIdx - 1] - (((inClose[EqualTrailingIdx - 1]) >= (inOpen[EqualTrailingIdx - 1])) ? (inClose[EqualTrailingIdx - 1]) : (inOpen[EqualTrailingIdx - 1]))) + ((((inClose[EqualTrailingIdx - 1]) >= (inOpen[EqualTrailingIdx - 1])) ? (inOpen[EqualTrailingIdx - 1]) : (inClose[EqualTrailingIdx - 1])) - inLow[EqualTrailingIdx - 1])) : 0.0)));
    i += 1;
    NearTrailingIdx += 1;
    EqualTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlgapsidesidewhitePositional(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDLGAPSIDESIDEWHITE', 'startIdx', startIdx);
  endIdx = requireInt32('CDLGAPSIDESIDEWHITE', 'endIdx', endIdx);
  requireIndexRange('CDLGAPSIDESIDEWHITE', startIdx, endIdx);
  const guardStart = clampedStart('CDLGAPSIDESIDEWHITE', startIdx, cdlgapsidesidewhiteLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLGAPSIDESIDEWHITE', 'prices.open', inOpen, guardInLen);
  requireLength('CDLGAPSIDESIDEWHITE', 'prices.high', inHigh, guardInLen);
  requireLength('CDLGAPSIDESIDEWHITE', 'prices.low', inLow, guardInLen);
  requireLength('CDLGAPSIDESIDEWHITE', 'prices.close', inClose, guardInLen);
  requireOutput('CDLGAPSIDESIDEWHITE', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLGAPSIDESIDEWHITE', 'outInteger', 'prices.open');
  }
  if (overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLGAPSIDESIDEWHITE', 'outInteger', 'prices.high');
  }
  if (overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLGAPSIDESIDEWHITE', 'outInteger', 'prices.low');
  }
  if (overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLGAPSIDESIDEWHITE', 'outInteger', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdlgapsidesidewhiteImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLGAPSIDESIDEWHITE', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cdlgapsidesidewhite}, {@link cdlgapsidesidewhiteLookback},
 * {@link cdlgapsidesidewhiteDisplayShift}, {@link cdlgapsidesidewhiteOpen} and
 * {@link cdlgapsidesidewhiteOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface CdlgapsidesidewhiteOptions {
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
 * What {@link cdlgapsidesidewhite} returns, and
 * {@link cdlgapsidesidewhiteOpenAndFill} with its stream.
 */
export interface CdlgapsidesidewhiteResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * +100 for an up-gap (bullish continuation), -100 for a down-gap (bearish
   * continuation), 0 when no pattern. Sign is set solely by the C2-vs-C1 gap
   * direction (realbodygapup ? 100 : -100). `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Int32Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link cdlgapsidesidewhite} consumes before its
 * first value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlgapsidesidewhiteOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlgapsidesidewhiteLookback(options?: CdlgapsidesidewhiteOptions): number {
  const o = optionsOf('CDLGAPSIDESIDEWHITE lookback', options);
  return cdlgapsidesidewhiteLookbackPositional(coreOf('CDLGAPSIDESIDEWHITE lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdlgapsidesidewhite}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlgapsidesidewhiteOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlgapsidesidewhiteDisplayShift(outputIdx: number, options?: CdlgapsidesidewhiteOptions): number {
  const o = optionsOf('CDLGAPSIDESIDEWHITE displayShift', options);
  return cdlgapsidesidewhiteDisplayShiftPositional(outputIdx, coreOf('CDLGAPSIDESIDEWHITE displayShift', o.core));
}

/**
 * A three-candle pattern: a first candle followed by two white candles of similar
 * body size that both gap the same direction (up or down) from the first candle's
 * real body and open at about the same level. It is a continuation signal whose
 * sign reports the gap direction; the code does not verify a prior trend.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlgapsidesidewhite](https://ta-lib.org/functions/cdlgapsidesidewhite).
 *
 * **Notes**
 * - Does not verify the prior trend the continuation signal classically assumes.
 * - Bulkowski's data shows the bullish form continues as labeled 66% of the time;
 *   the bearish form's 56% continuation rate is "near random", and Bulkowski
 *   cautions the bearish sample is too thin to trust.
 *   ([thepatternsite.com](https://thepatternsite.com/SidebySideWhiteLinesBull.html))
 * - Rare in Bulkowski's tests: he ranks the bullish form 73rd and the bearish 86th
 *   of 103 patterns by how often they occur
 *   ([bullish](https://thepatternsite.com/SidebySideWhiteLinesBull.html),
 *   [bearish](https://thepatternsite.com/SidebySideWhiteLinesBear.html)). It fires
 *   more often in crypto: 18 times in [about 16,000 daily
 *   bars](https://ta-lib.org/api/candlestick-outputs/#how-often-patterns-fire).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cdlgapsidesidewhiteLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `open`: Open price of
 *   each bar. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see
 *   {@link CdlgapsidesidewhiteOptions}.
 * @returns `begIdx` and the values; see {@link CdlgapsidesidewhiteResult}. Each
 *   output is a view of its buffer, the caller's or a new one.
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
 * import { cdlgapsidesidewhite } from 'ta-lib';
 *
 * const open = new Float64Array(252); // your data, oldest first
 * const high = new Float64Array(252); // your data, oldest first
 * const low = new Float64Array(252); // your data, oldest first
 * const close = new Float64Array(252); // your data, oldest first
 *
 * const { begIdx, values } = cdlgapsidesidewhite({ open, high, low, close });
 * console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
 * ```
 * @see [`cdltasukigap`](https://ta-lib.org/functions/cdltasukigap)
 * @see [`cdlxsidegap3methods`](https://ta-lib.org/functions/cdlxsidegap3methods)
 */
export function cdlgapsidesidewhite(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlgapsidesidewhiteOptions): CdlgapsidesidewhiteResult {
  requireArgument('CDLGAPSIDESIDEWHITE', 'prices', prices);
  const o = optionsOf('CDLGAPSIDESIDEWHITE', options);
  if (o.endIdx === undefined) {
    requireSeries('CDLGAPSIDESIDEWHITE', 'prices.open', prices.open);
    requireSeries('CDLGAPSIDESIDEWHITE', 'prices.high', prices.high);
    requireSeries('CDLGAPSIDESIDEWHITE', 'prices.low', prices.low);
    requireSeries('CDLGAPSIDESIDEWHITE', 'prices.close', prices.close);
  }
  const core = coreOf('CDLGAPSIDESIDEWHITE', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cdlgapsidesidewhiteLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const range = cdlgapsidesidewhitePositional(startIdx, endIdx, prices.open, prices.high, prices.low, prices.close, outInteger, core);
  return { begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CDLGAPSIDESIDEWHITE stream: one value per closed bar, bit-identical to
 * {@link cdlgapsidesidewhite} over the same series. Open it with
 * {@link cdlgapsidesidewhiteOpen} or {@link cdlgapsidesidewhiteOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlgapsidesidewhite](https://ta-lib.org/functions/cdlgapsidesidewhite).
 */
export class CdlgapsidesidewhiteStream {
  /** @internal */
  core: Core;
  /** @internal */
  NearPeriodTotal: number = 0;
  /** @internal */
  EqualPeriodTotal: number = 0;
  /** @internal */
  lag1_inOpen: number = 0;
  /** @internal */
  lag2_inOpen: number = 0;
  /** @internal */
  lag1_inHigh: number = 0;
  /** @internal */
  lag1_inLow: number = 0;
  /** @internal */
  lag1_inClose: number = 0;
  /** @internal */
  lag2_inClose: number = 0;
  /** @internal */
  ringPos_EqualTrailingIdx: number = 0;
  /** @internal */
  ringCap_EqualTrailingIdx: number = 2;
  /** @internal */
  ringLag_EqualTrailingIdx: number = 0;
  /** @internal */
  ring_EqualTrailingIdx_derived: Float64Array = new Float64Array(2);
  /** @internal */
  ringPos_NearTrailingIdx: number = 0;
  /** @internal */
  ringCap_NearTrailingIdx: number = 2;
  /** @internal */
  ringLag_NearTrailingIdx: number = 0;
  /** @internal */
  ring_NearTrailingIdx_derived: Float64Array = new Float64Array(2);
  /** @internal */
  cs_Equal_rangeType: number = 0;
  /** @internal */
  cs_Equal_avgPeriod: number = 0;
  /** @internal */
  cs_Equal_factor: number = 0;
  /** @internal */
  cs_Near_rangeType: number = 0;
  /** @internal */
  cs_Near_avgPeriod: number = 0;
  /** @internal */
  cs_Near_factor: number = 0;
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
  static of(core: Core): CdlgapsidesidewhiteStream {
    return new CdlgapsidesidewhiteStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cdlgapsidesidewhite} reports them over the same bars. Each accepted
   * `update` and each `advance` adds one to `count`.
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
      throw streamFailure('CDLGAPSIDESIDEWHITE advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw streamFailure('CDLGAPSIDESIDEWHITE update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(open) || !Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('CDLGAPSIDESIDEWHITE update', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    cdlgapsidesidewhiteStepImpl(this, open, high, low, close);
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
      throw nonFiniteBar('CDLGAPSIDESIDEWHITE peek', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inOpen = open;
    const inClose = close;
    let cur_outInteger = 0;
    const Equal_rangeType = sp.cs_Equal_rangeType;
    const Equal_avgPeriod = sp.cs_Equal_avgPeriod;
    const Equal_factor = sp.cs_Equal_factor;
    const Near_rangeType = sp.cs_Near_rangeType;
    const Near_avgPeriod = sp.cs_Near_avgPeriod;
    const Near_factor = sp.cs_Near_factor;
    if (((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === 1 && /* 2nd: white */
        ((inClose >= inOpen) ? 1 : -1) === 1 &&                 /* 3rd: white */
        (((sp.lag1_inOpen < sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose) > (sp.lag2_inOpen > sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose)) && /* upside or downside gap between the 1st candle and both the next 2 candles */
          ((inOpen < inClose ? inOpen : inClose) > (sp.lag2_inOpen > sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose)) ||
         ((sp.lag1_inOpen > sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose) < (sp.lag2_inOpen < sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose)) &&
          ((inOpen > inClose ? inOpen : inClose) < (sp.lag2_inOpen < sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose))) &&
        Math.abs(inClose - inOpen) >= Math.abs(sp.lag1_inClose - sp.lag1_inOpen) - ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) && /* same size 2 and 3 */
        Math.abs(inClose - inOpen) <= Math.abs(sp.lag1_inClose - sp.lag1_inOpen) + ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
        inOpen >= sp.lag1_inOpen - ((Equal_factor * (((Equal_avgPeriod !== 0) ? (sp.EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))) && /* same open 2 and 3 */
        inOpen <= sp.lag1_inOpen + ((Equal_factor * (((Equal_avgPeriod !== 0) ? (sp.EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))))
    {
      cur_outInteger = ((sp.lag1_inOpen < sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose) > (sp.lag2_inOpen > sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose)) ? 100 : -100;
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
  clone(): CdlgapsidesidewhiteStream {
    const c = new CdlgapsidesidewhiteStream(this.core);
    c.NearPeriodTotal = this.NearPeriodTotal;
    c.EqualPeriodTotal = this.EqualPeriodTotal;
    c.lag1_inOpen = this.lag1_inOpen;
    c.lag2_inOpen = this.lag2_inOpen;
    c.lag1_inHigh = this.lag1_inHigh;
    c.lag1_inLow = this.lag1_inLow;
    c.lag1_inClose = this.lag1_inClose;
    c.lag2_inClose = this.lag2_inClose;
    c.ringPos_EqualTrailingIdx = this.ringPos_EqualTrailingIdx;
    c.ringCap_EqualTrailingIdx = this.ringCap_EqualTrailingIdx;
    c.ringLag_EqualTrailingIdx = this.ringLag_EqualTrailingIdx;
    c.ring_EqualTrailingIdx_derived = this.ring_EqualTrailingIdx_derived.slice();
    c.ringPos_NearTrailingIdx = this.ringPos_NearTrailingIdx;
    c.ringCap_NearTrailingIdx = this.ringCap_NearTrailingIdx;
    c.ringLag_NearTrailingIdx = this.ringLag_NearTrailingIdx;
    c.ring_NearTrailingIdx_derived = this.ring_NearTrailingIdx_derived.slice();
    c.cs_Equal_rangeType = this.cs_Equal_rangeType;
    c.cs_Equal_avgPeriod = this.cs_Equal_avgPeriod;
    c.cs_Equal_factor = this.cs_Equal_factor;
    c.cs_Near_rangeType = this.cs_Near_rangeType;
    c.cs_Near_avgPeriod = this.cs_Near_avgPeriod;
    c.cs_Near_factor = this.cs_Near_factor;
    c.cur_outInteger = this.cur_outInteger;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cdlgapsidesidewhiteStepImpl(sp: CdlgapsidesidewhiteStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  let ringPosL_EqualTrailingIdx = 0;
  let ringPosL_NearTrailingIdx = 0;
  const Equal_rangeType = sp.cs_Equal_rangeType;
  const Equal_avgPeriod = sp.cs_Equal_avgPeriod;
  const Equal_factor = sp.cs_Equal_factor;
  const Near_rangeType = sp.cs_Near_rangeType;
  const Near_avgPeriod = sp.cs_Near_avgPeriod;
  const Near_factor = sp.cs_Near_factor;
  sp.ring_EqualTrailingIdx_derived[sp.ringPos_EqualTrailingIdx] = ((Equal_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((Equal_rangeType === 1) ? (inHigh - inLow) : ((Equal_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  sp.ring_NearTrailingIdx_derived[sp.ringPos_NearTrailingIdx] = ((Near_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((Near_rangeType === 1) ? (inHigh - inLow) : ((Near_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  if (((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === 1 && /* 2nd: white */
      ((inClose >= inOpen) ? 1 : -1) === 1 &&                 /* 3rd: white */
      (((sp.lag1_inOpen < sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose) > (sp.lag2_inOpen > sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose)) && /* upside or downside gap between the 1st candle and both the next 2 candles */
        ((inOpen < inClose ? inOpen : inClose) > (sp.lag2_inOpen > sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose)) ||
       ((sp.lag1_inOpen > sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose) < (sp.lag2_inOpen < sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose)) &&
        ((inOpen > inClose ? inOpen : inClose) < (sp.lag2_inOpen < sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose))) &&
      Math.abs(inClose - inOpen) >= Math.abs(sp.lag1_inClose - sp.lag1_inOpen) - ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) && /* same size 2 and 3 */
      Math.abs(inClose - inOpen) <= Math.abs(sp.lag1_inClose - sp.lag1_inOpen) + ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
      inOpen >= sp.lag1_inOpen - ((Equal_factor * (((Equal_avgPeriod !== 0) ? (sp.EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))) && /* same open 2 and 3 */
      inOpen <= sp.lag1_inOpen + ((Equal_factor * (((Equal_avgPeriod !== 0) ? (sp.EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))))
  {
    sp.cur_outInteger = ((sp.lag1_inOpen < sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose) > (sp.lag2_inOpen > sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose)) ? 100 : -100;
  } else {
    sp.cur_outInteger = 0;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  sp.NearPeriodTotal += ((Near_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0))) - sp.ring_NearTrailingIdx_derived[(((sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - 1) % sp.ringCap_NearTrailingIdx) | 0)];
  sp.EqualPeriodTotal += ((Equal_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0))) - sp.ring_EqualTrailingIdx_derived[(((sp.ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - sp.ringLag_EqualTrailingIdx - 1) % sp.ringCap_EqualTrailingIdx) | 0)];
  sp.lag2_inOpen = sp.lag1_inOpen;
  sp.lag1_inOpen = inOpen;
  sp.lag1_inHigh = inHigh;
  sp.lag1_inLow = inLow;
  sp.lag2_inClose = sp.lag1_inClose;
  sp.lag1_inClose = inClose;
  ringPosL_EqualTrailingIdx = sp.ringPos_EqualTrailingIdx;
  ringPosL_EqualTrailingIdx = ringPosL_EqualTrailingIdx + 1;
  ringPosL_EqualTrailingIdx = ringPosL_EqualTrailingIdx & (ringPosL_EqualTrailingIdx - sp.ringCap_EqualTrailingIdx >> 31);
  sp.ringPos_EqualTrailingIdx = ringPosL_EqualTrailingIdx;
  ringPosL_NearTrailingIdx = sp.ringPos_NearTrailingIdx;
  ringPosL_NearTrailingIdx = ringPosL_NearTrailingIdx + 1;
  ringPosL_NearTrailingIdx = ringPosL_NearTrailingIdx & (ringPosL_NearTrailingIdx - sp.ringCap_NearTrailingIdx >> 31);
  sp.ringPos_NearTrailingIdx = ringPosL_NearTrailingIdx;
}

function cdlgapsidesidewhiteOpenImpl(
  sp: CdlgapsidesidewhiteStream,
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
  let NearPeriodTotal = 0;
  let EqualPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let NearTrailingIdx = 0;
  let EqualTrailingIdx = 0;
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
  const Equal_rangeType: number = core.candleSettings[CandleSettingType.EQUAL].rangeType;
  const Equal_avgPeriod = core.candleSettings[CandleSettingType.EQUAL].avgPeriod;
  const Equal_factor = core.candleSettings[CandleSettingType.EQUAL].factor;
  const Near_rangeType: number = core.candleSettings[CandleSettingType.NEAR].rangeType;
  const Near_avgPeriod = core.candleSettings[CandleSettingType.NEAR].avgPeriod;
  const Near_factor = core.candleSettings[CandleSettingType.NEAR].factor;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlgapsidesidewhiteLookbackPositional(core);
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
  NearPeriodTotal = 0;
  EqualPeriodTotal = 0;
  NearTrailingIdx = startIdx - Near_avgPeriod;
  EqualTrailingIdx = startIdx - Equal_avgPeriod;
  i = NearTrailingIdx;
  while (i < startIdx) {
    NearPeriodTotal += ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    i += 1;
  }
  i = EqualTrailingIdx;
  while (i < startIdx) {
    EqualPeriodTotal += ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - upside or downside gap (between the bodies)
   * - first candle after the window: white candlestick
   * - second candle after the window: white candlestick with similar size (near the same) and about the same
   *   open (equal) of the previous candle
   * - the second candle does not close the window
   * The meaning of "near" and "equal" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === 1 && /* 2nd: white */
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === 1 &&         /* 3rd: white */
        (((inOpen[i - 1] < inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1]) > (inOpen[i - 2] > inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2])) && /* upside or downside gap between the 1st candle and both the next 2 candles */
          ((inOpen[i] < inClose[i] ? inOpen[i] : inClose[i]) > (inOpen[i - 2] > inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2])) ||
         ((inOpen[i - 1] > inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1]) < (inOpen[i - 2] < inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2])) &&
          ((inOpen[i] > inClose[i] ? inOpen[i] : inClose[i]) < (inOpen[i - 2] < inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2]))) &&
        Math.abs(inClose[i] - inOpen[i]) >= Math.abs(inClose[i - 1] - inOpen[i - 1]) - ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) && /* same size 2 and 3 */
        Math.abs(inClose[i] - inOpen[i]) <= Math.abs(inClose[i - 1] - inOpen[i - 1]) + ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
        inOpen[i] >= inOpen[i - 1] - ((Equal_factor * (((Equal_avgPeriod !== 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))) && /* same open 2 and 3 */
        inOpen[i] <= inOpen[i - 1] + ((Equal_factor * (((Equal_avgPeriod !== 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))))
    {
      outInteger[((outIdx++ * outStride) | 0)] = ((inOpen[i - 1] < inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1]) > (inOpen[i - 2] > inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2])) ? 100 : -100;
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    NearPeriodTotal += ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((Near_rangeType === 0) ? (Math.abs(inClose[NearTrailingIdx - 1] - inOpen[NearTrailingIdx - 1])) : ((Near_rangeType === 1) ? (inHigh[NearTrailingIdx - 1] - inLow[NearTrailingIdx - 1]) : ((Near_rangeType === 2) ? ((inHigh[NearTrailingIdx - 1] - (((inClose[NearTrailingIdx - 1]) >= (inOpen[NearTrailingIdx - 1])) ? (inClose[NearTrailingIdx - 1]) : (inOpen[NearTrailingIdx - 1]))) + ((((inClose[NearTrailingIdx - 1]) >= (inOpen[NearTrailingIdx - 1])) ? (inOpen[NearTrailingIdx - 1]) : (inClose[NearTrailingIdx - 1])) - inLow[NearTrailingIdx - 1])) : 0.0)));
    EqualPeriodTotal += ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((Equal_rangeType === 0) ? (Math.abs(inClose[EqualTrailingIdx - 1] - inOpen[EqualTrailingIdx - 1])) : ((Equal_rangeType === 1) ? (inHigh[EqualTrailingIdx - 1] - inLow[EqualTrailingIdx - 1]) : ((Equal_rangeType === 2) ? ((inHigh[EqualTrailingIdx - 1] - (((inClose[EqualTrailingIdx - 1]) >= (inOpen[EqualTrailingIdx - 1])) ? (inClose[EqualTrailingIdx - 1]) : (inOpen[EqualTrailingIdx - 1]))) + ((((inClose[EqualTrailingIdx - 1]) >= (inOpen[EqualTrailingIdx - 1])) ? (inOpen[EqualTrailingIdx - 1]) : (inClose[EqualTrailingIdx - 1])) - inLow[EqualTrailingIdx - 1])) : 0.0)));
    i += 1;
    NearTrailingIdx += 1;
    EqualTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const capLag_EqualTrailingIdx = i - EqualTrailingIdx;
  const cap_EqualTrailingIdx = capLag_EqualTrailingIdx + 2;
  if (capLag_EqualTrailingIdx < 0 || cap_EqualTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_EqualTrailingIdx = cap_EqualTrailingIdx > 0 ? cap_EqualTrailingIdx : 1;
  const capRing_EqualTrailingIdx_derived = new Float64Array(allocN_EqualTrailingIdx);
  for (let fillJ = historyLen - cap_EqualTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_EqualTrailingIdx_derived[fillJ % cap_EqualTrailingIdx] = ((Equal_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((Equal_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((Equal_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  const capLag_NearTrailingIdx = i - NearTrailingIdx;
  const cap_NearTrailingIdx = capLag_NearTrailingIdx + 2;
  if (capLag_NearTrailingIdx < 0 || cap_NearTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_NearTrailingIdx = cap_NearTrailingIdx > 0 ? cap_NearTrailingIdx : 1;
  const capRing_NearTrailingIdx_derived = new Float64Array(allocN_NearTrailingIdx);
  for (let fillJ = historyLen - cap_NearTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_NearTrailingIdx_derived[fillJ % cap_NearTrailingIdx] = ((Near_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((Near_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((Near_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  sp.NearPeriodTotal = NearPeriodTotal;
  sp.EqualPeriodTotal = EqualPeriodTotal;
  sp.lag1_inOpen = inOpen[historyLen - 1];
  sp.lag2_inOpen = inOpen[historyLen - 2];
  sp.lag1_inHigh = inHigh[historyLen - 1];
  sp.lag1_inLow = inLow[historyLen - 1];
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.lag2_inClose = inClose[historyLen - 2];
  sp.ringPos_EqualTrailingIdx = historyLen % cap_EqualTrailingIdx;
  sp.ringCap_EqualTrailingIdx = cap_EqualTrailingIdx;
  sp.ringLag_EqualTrailingIdx = capLag_EqualTrailingIdx;
  sp.ring_EqualTrailingIdx_derived = capRing_EqualTrailingIdx_derived;
  sp.ringPos_NearTrailingIdx = historyLen % cap_NearTrailingIdx;
  sp.ringCap_NearTrailingIdx = cap_NearTrailingIdx;
  sp.ringLag_NearTrailingIdx = capLag_NearTrailingIdx;
  sp.ring_NearTrailingIdx_derived = capRing_NearTrailingIdx_derived;
  sp.cs_Equal_rangeType = Equal_rangeType;
  sp.cs_Equal_avgPeriod = Equal_avgPeriod;
  sp.cs_Equal_factor = Equal_factor;
  sp.cs_Near_rangeType = Near_rangeType;
  sp.cs_Near_avgPeriod = Near_avgPeriod;
  sp.cs_Near_factor = Near_factor;
  sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlgapsidesidewhiteOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): CdlgapsidesidewhiteStream {
  const sp = CdlgapsidesidewhiteStream.of(core);
  const retCode = cdlgapsidesidewhiteOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLGAPSIDESIDEWHITE openAndFill', inOpen.length, startIdx, cdlgapsidesidewhiteLookbackPositional(core));
  }
  throw streamFailure('CDLGAPSIDESIDEWHITE openAndFill', retCode);
}

/** @internal */
export function cdlgapsidesidewhiteOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): CdlgapsidesidewhiteStream {
  const sp = CdlgapsidesidewhiteStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdlgapsidesidewhiteOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLGAPSIDESIDEWHITE open', inOpen.length, startIdx, cdlgapsidesidewhiteLookbackPositional(core));
  }
  throw streamFailure('CDLGAPSIDESIDEWHITE open', retCode);
}

/** @internal */
export function cdlgapsidesidewhiteOpenPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): CdlgapsidesidewhiteStream {
  requireArgument('CDLGAPSIDESIDEWHITE open', 'prices.open', inOpen);
  requireHistory('CDLGAPSIDESIDEWHITE open', 'prices.open', inOpen.length);
  requireArgument('CDLGAPSIDESIDEWHITE open', 'prices.high', inHigh);
  requireArgument('CDLGAPSIDESIDEWHITE open', 'prices.low', inLow);
  requireArgument('CDLGAPSIDESIDEWHITE open', 'prices.close', inClose);
  requireHistoryLength('CDLGAPSIDESIDEWHITE open', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLGAPSIDESIDEWHITE open', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLGAPSIDESIDEWHITE open', 'prices.close', inClose.length, inOpen.length);
  return cdlgapsidesidewhiteOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/** @internal */
export function cdlgapsidesidewhiteOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): CdlgapsidesidewhiteStream {
  requireArgument('CDLGAPSIDESIDEWHITE openAndFill', 'prices.open', inOpen);
  requireHistory('CDLGAPSIDESIDEWHITE openAndFill', 'prices.open', inOpen.length);
  requireArgument('CDLGAPSIDESIDEWHITE openAndFill', 'prices.high', inHigh);
  requireArgument('CDLGAPSIDESIDEWHITE openAndFill', 'prices.low', inLow);
  requireArgument('CDLGAPSIDESIDEWHITE openAndFill', 'prices.close', inClose);
  const guardOutLen = openFillCount('CDLGAPSIDESIDEWHITE openAndFill', inOpen.length, cdlgapsidesidewhiteLookbackPositional(core));
  requireHistoryLength('CDLGAPSIDESIDEWHITE openAndFill', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLGAPSIDESIDEWHITE openAndFill', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLGAPSIDESIDEWHITE openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('CDLGAPSIDESIDEWHITE openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('CDLGAPSIDESIDEWHITE openAndFill', 'outInteger', 'prices.open');
  }
  if (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CDLGAPSIDESIDEWHITE openAndFill', 'outInteger', 'prices.high');
  }
  if (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CDLGAPSIDESIDEWHITE openAndFill', 'outInteger', 'prices.low');
  }
  if (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('CDLGAPSIDESIDEWHITE openAndFill', 'outInteger', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLGAPSIDESIDEWHITE openAndFill', 'outInteger', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLGAPSIDESIDEWHITE openAndFill', 'outInteger', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLGAPSIDESIDEWHITE openAndFill', 'outInteger', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLGAPSIDESIDEWHITE openAndFill', 'outInteger', 'prices.close');
  }
  return cdlgapsidesidewhiteOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}

/**
 * Opens a live CDLGAPSIDESIDEWHITE stream over the warm-up history: `value()`
 * starts at the last history bar, bit-identical to {@link cdlgapsidesidewhite}
 * there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlgapsidesidewhiteOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlgapsidesidewhiteLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input or histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdlgapsidesidewhiteOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdlgapsidesidewhiteOpen({ open, high, low, close });
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdlgapsidesidewhiteOpen(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlgapsidesidewhiteOptions): CdlgapsidesidewhiteStream {
  requireArgument('CDLGAPSIDESIDEWHITE open', 'prices', prices);
  const o = optionsOf('CDLGAPSIDESIDEWHITE open', options);
  const core = coreOf('CDLGAPSIDESIDEWHITE open', o.core);
  return cdlgapsidesidewhiteOpenPositional(prices.open, prices.high, prices.low, prices.close, core);
}

/**
 * {@link cdlgapsidesidewhiteOpen} that also computes the outputs over the whole
 * history in the same pass, bit-identically to {@link cdlgapsidesidewhite}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CdlgapsidesidewhiteOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cdlgapsidesidewhite} returns them; the range
 *   is the stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlgapsidesidewhiteLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, histories of different lengths or a buffer too short,
 *   of the wrong type or starting where another buffer or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdlgapsidesidewhiteOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = cdlgapsidesidewhiteOpenAndFill({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cdlgapsidesidewhiteOpenAndFill(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlgapsidesidewhiteOptions): CdlgapsidesidewhiteResult & { stream: CdlgapsidesidewhiteStream } {
  requireArgument('CDLGAPSIDESIDEWHITE openAndFill', 'prices', prices);
  const o = optionsOf('CDLGAPSIDESIDEWHITE openAndFill', options);
  const core = coreOf('CDLGAPSIDESIDEWHITE openAndFill', o.core);
  const capacity = fillCapacity(cdlgapsidesidewhiteLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const stream = cdlgapsidesidewhiteOpenAndFillPositional(prices.open, prices.high, prices.low, prices.close, outInteger, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}
