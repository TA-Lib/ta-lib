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
 *  121104 AC     Creation
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 *  103126 MF,CC  Nest the cheap, selective pre-check conjuncts in an outer if
 *  110526 MF,CC  Carry the ranges of the previous bars instead of recomputing them
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { CandleSettingType, Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function cdl3linestrikeLookbackPositional(core: Core = Core.DEFAULT): number {
  const Near_avgPeriod = core.candleSettings[CandleSettingType.NEAR].avgPeriod;
  return Near_avgPeriod + 3;
}

/** @internal */
export function cdl3linestrikeDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdl3linestrikeImpl(
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
  const NearPeriodTotal = new Float64Array(4);
  let NearPeriodTotalRange2 = 0;
  let NearPeriodTotalTrailRange2 = 0;
  let NearPeriodTotalRange3 = 0;
  let NearPeriodTotalTrailRange3 = 0;
  let i = 0;
  let outIdx = 0;
  let NearTrailingIdx = 0;
  let lookbackTotal = 0;
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
  lookbackTotal = cdl3linestrikeLookbackPositional(core);
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
  NearPeriodTotal[3] = 0;
  NearPeriodTotal[2] = 0;
  NearTrailingIdx = startIdx - Near_avgPeriod;
  i = NearTrailingIdx;
  while (i < startIdx) {
    NearPeriodTotal[3] = NearPeriodTotal[3] + ((Near_rangeType === 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType === 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType === 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)));
    NearPeriodTotal[2] = NearPeriodTotal[2] + ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - three white soldiers (three black crows): three white (black) candlesticks with consecutively higher (lower) closes,
   * each opening within or near the previous real body
   * - fourth candle: black (white) candle that opens above (below) prior candle's close and closes below (above)
   * the first candle's open
   * The meaning of "near" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  NearPeriodTotalRange3 = ((Near_rangeType === 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType === 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType === 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)));
  NearPeriodTotalTrailRange3 = ((Near_rangeType === 0) ? (Math.abs(inClose[NearTrailingIdx - 3] - inOpen[NearTrailingIdx - 3])) : ((Near_rangeType === 1) ? (inHigh[NearTrailingIdx - 3] - inLow[NearTrailingIdx - 3]) : ((Near_rangeType === 2) ? ((inHigh[NearTrailingIdx - 3] - (((inClose[NearTrailingIdx - 3]) >= (inOpen[NearTrailingIdx - 3])) ? (inClose[NearTrailingIdx - 3]) : (inOpen[NearTrailingIdx - 3]))) + ((((inClose[NearTrailingIdx - 3]) >= (inOpen[NearTrailingIdx - 3])) ? (inOpen[NearTrailingIdx - 3]) : (inClose[NearTrailingIdx - 3])) - inLow[NearTrailingIdx - 3])) : 0.0)));
  do {
    if (((inClose[i - 3] >= inOpen[i - 3]) ? 1 : -1) === ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) && /* three with same color */
        ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) &&
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === (-((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) | 0) && /* 4th opposite color */
        inOpen[i - 2] >= (inOpen[i - 3] < inClose[i - 3] ? inOpen[i - 3] : inClose[i - 3]) - ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType === 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType === 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd opens within/near 1st rb */
        inOpen[i - 2] <= (inOpen[i - 3] > inClose[i - 3] ? inOpen[i - 3] : inClose[i - 3]) + ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType === 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType === 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
        inOpen[i - 1] >= (inOpen[i - 2] < inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2]) - ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) && /* 3rd opens within/near 2nd rb */
        inOpen[i - 1] <= (inOpen[i - 2] > inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2]) + ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
        (((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === 1 &&  /* if three white */
          inClose[i - 1] > inClose[i - 2] &&
          inClose[i - 2] > inClose[i - 3] &&                    /* consecutive higher closes */
          inOpen[i] > inClose[i - 1] &&                         /* 4th opens above prior close */
          inClose[i] < inOpen[i - 3] ||                         /* 4th closes below 1st open */
         ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === -1 && /* if three black */
          inClose[i - 1] < inClose[i - 2] &&
          inClose[i - 2] < inClose[i - 3] &&                    /* consecutive lower closes */
          inOpen[i] < inClose[i - 1] &&                         /* 4th opens below prior close */
          inClose[i] > inOpen[i - 3]))                          /* 4th closes above 1st open */
    {
      outInteger[outIdx++] = ((((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) * 100) | 0);
    } else {
      outInteger[outIdx++] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    NearPeriodTotalRange2 = ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
    NearPeriodTotalTrailRange2 = ((Near_rangeType === 0) ? (Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : ((Near_rangeType === 1) ? (inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) : ((Near_rangeType === 2) ? ((inHigh[NearTrailingIdx - 2] - (((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inClose[NearTrailingIdx - 2]) : (inOpen[NearTrailingIdx - 2]))) + ((((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inOpen[NearTrailingIdx - 2]) : (inClose[NearTrailingIdx - 2])) - inLow[NearTrailingIdx - 2])) : 0.0)));
    NearPeriodTotal[3] = NearPeriodTotal[3] + (NearPeriodTotalRange3 - NearPeriodTotalTrailRange3);
    NearPeriodTotal[2] = NearPeriodTotal[2] + (NearPeriodTotalRange2 - NearPeriodTotalTrailRange2);
    NearPeriodTotalRange3 = NearPeriodTotalRange2;
    NearPeriodTotalTrailRange3 = NearPeriodTotalTrailRange2;
    i += 1;
    NearTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function cdl3linestrikePositional(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDL3LINESTRIKE', 'startIdx', startIdx);
  endIdx = requireInt32('CDL3LINESTRIKE', 'endIdx', endIdx);
  requireIndexRange('CDL3LINESTRIKE', startIdx, endIdx);
  const guardStart = clampedStart('CDL3LINESTRIKE', startIdx, cdl3linestrikeLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDL3LINESTRIKE', 'prices.open', inOpen, guardInLen);
  requireLength('CDL3LINESTRIKE', 'prices.high', inHigh, guardInLen);
  requireLength('CDL3LINESTRIKE', 'prices.low', inLow, guardInLen);
  requireLength('CDL3LINESTRIKE', 'prices.close', inClose, guardInLen);
  requireOutput('CDL3LINESTRIKE', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDL3LINESTRIKE', 'outInteger', 'prices.open');
  }
  if (overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDL3LINESTRIKE', 'outInteger', 'prices.high');
  }
  if (overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDL3LINESTRIKE', 'outInteger', 'prices.low');
  }
  if (overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDL3LINESTRIKE', 'outInteger', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdl3linestrikeImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDL3LINESTRIKE', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cdl3linestrike}, {@link cdl3linestrikeLookback},
 * {@link cdl3linestrikeDisplayShift}, {@link cdl3linestrikeOpen} and
 * {@link cdl3linestrikeOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface Cdl3linestrikeOptions {
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
 * What {@link cdl3linestrike} returns, and {@link cdl3linestrikeOpenAndFill} with
 * its stream.
 */
export interface Cdl3linestrikeResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * +100 for a white (rising) three-line strike, -100 for a black (falling)
   * three-line strike, 0 otherwise. Sign is the color of the first three candles:
   * candlecolor(i-1)*100. `values[k]` is the value at input bar `begIdx + k`.
   */
  values: Int32Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link cdl3linestrike} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link Cdl3linestrikeOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdl3linestrikeLookback(options?: Cdl3linestrikeOptions): number {
  const o = optionsOf('CDL3LINESTRIKE lookback', options);
  return cdl3linestrikeLookbackPositional(coreOf('CDL3LINESTRIKE lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdl3linestrike}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link Cdl3linestrikeOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdl3linestrikeDisplayShift(outputIdx: number, options?: Cdl3linestrikeOptions): number {
  const o = optionsOf('CDL3LINESTRIKE displayShift', options);
  return cdl3linestrikeDisplayShiftPositional(outputIdx, coreOf('CDL3LINESTRIKE displayShift', o.core));
}

/**
 * A four-candle pattern: three same-color candles with consecutively higher (or
 * lower) closes, each opening within or near the prior real body, then a fourth
 * opposite-color candle that opens beyond the third close and closes past the
 * first candle's open. TA-Lib emits a signed continuation-style signal keyed to
 * the color of the first three candles, traditionally read as significant only
 * inside a trend matching those three candles.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdl3linestrike](https://ta-lib.org/functions/cdl3linestrike).
 *
 * **Notes**
 * - Does not verify the surrounding trend the pattern classically assumes for
 *   significance.
 * - TA-Lib's sign follows the classic continuation reading. Thomas Bulkowski's
 *   statistical study of the pattern (*Encyclopedia of Candlestick Charts*) found
 *   the opposite in practice — it acted as a reversal far more often than a
 *   continuation — so traders who follow his research read this pattern's signal
 *   in the opposite direction from what its sign here suggests.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cdl3linestrikeLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `open`: Open price of
 *   each bar. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link Cdl3linestrikeOptions}.
 * @returns `begIdx` and the values; see {@link Cdl3linestrikeResult}. Each output
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
 * import { cdl3linestrike } from 'ta-lib';
 *
 * const open = new Float64Array(252); // your data, oldest first
 * const high = new Float64Array(252); // your data, oldest first
 * const low = new Float64Array(252); // your data, oldest first
 * const close = new Float64Array(252); // your data, oldest first
 *
 * const { begIdx, values } = cdl3linestrike({ open, high, low, close });
 * console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
 * ```
 * @see [`cdl3whitesoldiers`](https://ta-lib.org/functions/cdl3whitesoldiers)
 * @see [`cdl3blackcrows`](https://ta-lib.org/functions/cdl3blackcrows)
 */
export function cdl3linestrike(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: Cdl3linestrikeOptions): Cdl3linestrikeResult {
  requireArgument('CDL3LINESTRIKE', 'prices', prices);
  const o = optionsOf('CDL3LINESTRIKE', options);
  if (o.endIdx === undefined) {
    requireSeries('CDL3LINESTRIKE', 'prices.open', prices.open);
    requireSeries('CDL3LINESTRIKE', 'prices.high', prices.high);
    requireSeries('CDL3LINESTRIKE', 'prices.low', prices.low);
    requireSeries('CDL3LINESTRIKE', 'prices.close', prices.close);
  }
  const core = coreOf('CDL3LINESTRIKE', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cdl3linestrikeLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const range = cdl3linestrikePositional(startIdx, endIdx, prices.open, prices.high, prices.low, prices.close, outInteger, core);
  return { begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CDL3LINESTRIKE stream: one value per closed bar, bit-identical to
 * {@link cdl3linestrike} over the same series. Open it with
 * {@link cdl3linestrikeOpen} or {@link cdl3linestrikeOpenAndFill}; there is
 * nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdl3linestrike](https://ta-lib.org/functions/cdl3linestrike).
 */
export class Cdl3linestrikeStream {
  /** @internal */
  core: Core;
  /** @internal */
  NearPeriodTotal: Float64Array = new Float64Array(4);
  /** @internal */
  NearPeriodTotalRange3: number = 0;
  /** @internal */
  NearPeriodTotalTrailRange3: number = 0;
  /** @internal */
  lag1_inOpen: number = 0;
  /** @internal */
  lag2_inOpen: number = 0;
  /** @internal */
  lag3_inOpen: number = 0;
  /** @internal */
  lag1_inHigh: number = 0;
  /** @internal */
  lag2_inHigh: number = 0;
  /** @internal */
  lag3_inHigh: number = 0;
  /** @internal */
  lag1_inLow: number = 0;
  /** @internal */
  lag2_inLow: number = 0;
  /** @internal */
  lag3_inLow: number = 0;
  /** @internal */
  lag1_inClose: number = 0;
  /** @internal */
  lag2_inClose: number = 0;
  /** @internal */
  lag3_inClose: number = 0;
  /** @internal */
  ringPos_NearTrailingIdx: number = 0;
  /** @internal */
  ringCap_NearTrailingIdx: number = 3;
  /** @internal */
  ringLag_NearTrailingIdx: number = 0;
  /** @internal */
  ring_NearTrailingIdx_derived: Float64Array = new Float64Array(3);
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
  static of(core: Core): Cdl3linestrikeStream {
    return new Cdl3linestrikeStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cdl3linestrike} reports them over the same bars. Each accepted `update`
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
      throw streamFailure('CDL3LINESTRIKE advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw streamFailure('CDL3LINESTRIKE update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(open) || !Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('CDL3LINESTRIKE update', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    cdl3linestrikeStepImpl(this, open, high, low, close);
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
      throw nonFiniteBar('CDL3LINESTRIKE peek', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inOpen = open;
    const inClose = close;
    let cur_outInteger = 0;
    const Near_rangeType = sp.cs_Near_rangeType;
    const Near_avgPeriod = sp.cs_Near_avgPeriod;
    const Near_factor = sp.cs_Near_factor;
    if (((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : -1) === ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) && /* three with same color */
        ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) &&
        ((inClose >= inOpen) ? 1 : -1) === (-((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) | 0) && /* 4th opposite color */
        sp.lag2_inOpen >= (sp.lag3_inOpen < sp.lag3_inClose ? sp.lag3_inOpen : sp.lag3_inClose) - ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag3_inClose - sp.lag3_inOpen)) : ((Near_rangeType === 1) ? (sp.lag3_inHigh - sp.lag3_inLow) : ((Near_rangeType === 2) ? ((sp.lag3_inHigh - (((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inClose) : (sp.lag3_inOpen))) + ((((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inOpen) : (sp.lag3_inClose)) - sp.lag3_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd opens within/near 1st rb */
        sp.lag2_inOpen <= (sp.lag3_inOpen > sp.lag3_inClose ? sp.lag3_inOpen : sp.lag3_inClose) + ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag3_inClose - sp.lag3_inOpen)) : ((Near_rangeType === 1) ? (sp.lag3_inHigh - sp.lag3_inLow) : ((Near_rangeType === 2) ? ((sp.lag3_inHigh - (((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inClose) : (sp.lag3_inOpen))) + ((((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inOpen) : (sp.lag3_inClose)) - sp.lag3_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
        sp.lag1_inOpen >= (sp.lag2_inOpen < sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose) - ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) && /* 3rd opens within/near 2nd rb */
        sp.lag1_inOpen <= (sp.lag2_inOpen > sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose) + ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
        (((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === 1 &&  /* if three white */
          sp.lag1_inClose > sp.lag2_inClose &&
          sp.lag2_inClose > sp.lag3_inClose &&                    /* consecutive higher closes */
          inOpen > sp.lag1_inClose &&                             /* 4th opens above prior close */
          inClose < sp.lag3_inOpen ||                             /* 4th closes below 1st open */
         ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === -1 && /* if three black */
          sp.lag1_inClose < sp.lag2_inClose &&
          sp.lag2_inClose < sp.lag3_inClose &&                    /* consecutive lower closes */
          inOpen < sp.lag1_inClose &&                             /* 4th opens below prior close */
          inClose > sp.lag3_inOpen))                              /* 4th closes above 1st open */
    {
      cur_outInteger = ((((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) * 100) | 0);
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
  clone(): Cdl3linestrikeStream {
    const c = new Cdl3linestrikeStream(this.core);
    c.NearPeriodTotal = this.NearPeriodTotal.slice();
    c.NearPeriodTotalRange3 = this.NearPeriodTotalRange3;
    c.NearPeriodTotalTrailRange3 = this.NearPeriodTotalTrailRange3;
    c.lag1_inOpen = this.lag1_inOpen;
    c.lag2_inOpen = this.lag2_inOpen;
    c.lag3_inOpen = this.lag3_inOpen;
    c.lag1_inHigh = this.lag1_inHigh;
    c.lag2_inHigh = this.lag2_inHigh;
    c.lag3_inHigh = this.lag3_inHigh;
    c.lag1_inLow = this.lag1_inLow;
    c.lag2_inLow = this.lag2_inLow;
    c.lag3_inLow = this.lag3_inLow;
    c.lag1_inClose = this.lag1_inClose;
    c.lag2_inClose = this.lag2_inClose;
    c.lag3_inClose = this.lag3_inClose;
    c.ringPos_NearTrailingIdx = this.ringPos_NearTrailingIdx;
    c.ringCap_NearTrailingIdx = this.ringCap_NearTrailingIdx;
    c.ringLag_NearTrailingIdx = this.ringLag_NearTrailingIdx;
    c.ring_NearTrailingIdx_derived = this.ring_NearTrailingIdx_derived.slice();
    c.cs_Near_rangeType = this.cs_Near_rangeType;
    c.cs_Near_avgPeriod = this.cs_Near_avgPeriod;
    c.cs_Near_factor = this.cs_Near_factor;
    c.cur_outInteger = this.cur_outInteger;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cdl3linestrikeStepImpl(sp: Cdl3linestrikeStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  let NearPeriodTotalRange2 = 0;
  let NearPeriodTotalTrailRange2 = 0;
  let ringPosL_NearTrailingIdx = 0;
  const Near_rangeType = sp.cs_Near_rangeType;
  const Near_avgPeriod = sp.cs_Near_avgPeriod;
  const Near_factor = sp.cs_Near_factor;
  sp.ring_NearTrailingIdx_derived[sp.ringPos_NearTrailingIdx] = ((Near_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((Near_rangeType === 1) ? (inHigh - inLow) : ((Near_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  if (((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : -1) === ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) && /* three with same color */
      ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) &&
      ((inClose >= inOpen) ? 1 : -1) === (-((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) | 0) && /* 4th opposite color */
      sp.lag2_inOpen >= (sp.lag3_inOpen < sp.lag3_inClose ? sp.lag3_inOpen : sp.lag3_inClose) - ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag3_inClose - sp.lag3_inOpen)) : ((Near_rangeType === 1) ? (sp.lag3_inHigh - sp.lag3_inLow) : ((Near_rangeType === 2) ? ((sp.lag3_inHigh - (((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inClose) : (sp.lag3_inOpen))) + ((((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inOpen) : (sp.lag3_inClose)) - sp.lag3_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd opens within/near 1st rb */
      sp.lag2_inOpen <= (sp.lag3_inOpen > sp.lag3_inClose ? sp.lag3_inOpen : sp.lag3_inClose) + ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag3_inClose - sp.lag3_inOpen)) : ((Near_rangeType === 1) ? (sp.lag3_inHigh - sp.lag3_inLow) : ((Near_rangeType === 2) ? ((sp.lag3_inHigh - (((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inClose) : (sp.lag3_inOpen))) + ((((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inOpen) : (sp.lag3_inClose)) - sp.lag3_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
      sp.lag1_inOpen >= (sp.lag2_inOpen < sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose) - ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) && /* 3rd opens within/near 2nd rb */
      sp.lag1_inOpen <= (sp.lag2_inOpen > sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose) + ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
      (((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === 1 &&  /* if three white */
        sp.lag1_inClose > sp.lag2_inClose &&
        sp.lag2_inClose > sp.lag3_inClose &&                    /* consecutive higher closes */
        inOpen > sp.lag1_inClose &&                             /* 4th opens above prior close */
        inClose < sp.lag3_inOpen ||                             /* 4th closes below 1st open */
       ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === -1 && /* if three black */
        sp.lag1_inClose < sp.lag2_inClose &&
        sp.lag2_inClose < sp.lag3_inClose &&                    /* consecutive lower closes */
        inOpen < sp.lag1_inClose &&                             /* 4th opens below prior close */
        inClose > sp.lag3_inOpen))                              /* 4th closes above 1st open */
  {
    sp.cur_outInteger = ((((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) * 100) | 0);
  } else {
    sp.cur_outInteger = 0;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  NearPeriodTotalRange2 = ((Near_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)));
  NearPeriodTotalTrailRange2 = sp.ring_NearTrailingIdx_derived[(((sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - 2) % sp.ringCap_NearTrailingIdx) | 0)];
  sp.NearPeriodTotal[3] = sp.NearPeriodTotal[3] + (sp.NearPeriodTotalRange3 - sp.NearPeriodTotalTrailRange3);
  sp.NearPeriodTotal[2] = sp.NearPeriodTotal[2] + (NearPeriodTotalRange2 - NearPeriodTotalTrailRange2);
  sp.NearPeriodTotalRange3 = NearPeriodTotalRange2;
  sp.NearPeriodTotalTrailRange3 = NearPeriodTotalTrailRange2;
  sp.lag3_inOpen = sp.lag2_inOpen;
  sp.lag2_inOpen = sp.lag1_inOpen;
  sp.lag1_inOpen = inOpen;
  sp.lag3_inHigh = sp.lag2_inHigh;
  sp.lag2_inHigh = sp.lag1_inHigh;
  sp.lag1_inHigh = inHigh;
  sp.lag3_inLow = sp.lag2_inLow;
  sp.lag2_inLow = sp.lag1_inLow;
  sp.lag1_inLow = inLow;
  sp.lag3_inClose = sp.lag2_inClose;
  sp.lag2_inClose = sp.lag1_inClose;
  sp.lag1_inClose = inClose;
  ringPosL_NearTrailingIdx = sp.ringPos_NearTrailingIdx;
  ringPosL_NearTrailingIdx = ringPosL_NearTrailingIdx + 1;
  ringPosL_NearTrailingIdx = ringPosL_NearTrailingIdx & (ringPosL_NearTrailingIdx - sp.ringCap_NearTrailingIdx >> 31);
  sp.ringPos_NearTrailingIdx = ringPosL_NearTrailingIdx;
}

function cdl3linestrikeOpenImpl(
  sp: Cdl3linestrikeStream,
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
  const NearPeriodTotal = new Float64Array(4);
  let NearPeriodTotalRange2 = 0;
  let NearPeriodTotalTrailRange2 = 0;
  let NearPeriodTotalRange3 = 0;
  let NearPeriodTotalTrailRange3 = 0;
  let i = 0;
  let outIdx = 0;
  let NearTrailingIdx = 0;
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
  const Near_rangeType: number = core.candleSettings[CandleSettingType.NEAR].rangeType;
  const Near_avgPeriod = core.candleSettings[CandleSettingType.NEAR].avgPeriod;
  const Near_factor = core.candleSettings[CandleSettingType.NEAR].factor;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdl3linestrikeLookbackPositional(core);
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
  NearPeriodTotal[3] = 0;
  NearPeriodTotal[2] = 0;
  NearTrailingIdx = startIdx - Near_avgPeriod;
  i = NearTrailingIdx;
  while (i < startIdx) {
    NearPeriodTotal[3] = NearPeriodTotal[3] + ((Near_rangeType === 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType === 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType === 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)));
    NearPeriodTotal[2] = NearPeriodTotal[2] + ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - three white soldiers (three black crows): three white (black) candlesticks with consecutively higher (lower) closes,
   * each opening within or near the previous real body
   * - fourth candle: black (white) candle that opens above (below) prior candle's close and closes below (above)
   * the first candle's open
   * The meaning of "near" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  NearPeriodTotalRange3 = ((Near_rangeType === 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType === 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType === 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)));
  NearPeriodTotalTrailRange3 = ((Near_rangeType === 0) ? (Math.abs(inClose[NearTrailingIdx - 3] - inOpen[NearTrailingIdx - 3])) : ((Near_rangeType === 1) ? (inHigh[NearTrailingIdx - 3] - inLow[NearTrailingIdx - 3]) : ((Near_rangeType === 2) ? ((inHigh[NearTrailingIdx - 3] - (((inClose[NearTrailingIdx - 3]) >= (inOpen[NearTrailingIdx - 3])) ? (inClose[NearTrailingIdx - 3]) : (inOpen[NearTrailingIdx - 3]))) + ((((inClose[NearTrailingIdx - 3]) >= (inOpen[NearTrailingIdx - 3])) ? (inOpen[NearTrailingIdx - 3]) : (inClose[NearTrailingIdx - 3])) - inLow[NearTrailingIdx - 3])) : 0.0)));
  do {
    if (((inClose[i - 3] >= inOpen[i - 3]) ? 1 : -1) === ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) && /* three with same color */
        ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) &&
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === (-((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) | 0) && /* 4th opposite color */
        inOpen[i - 2] >= (inOpen[i - 3] < inClose[i - 3] ? inOpen[i - 3] : inClose[i - 3]) - ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType === 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType === 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd opens within/near 1st rb */
        inOpen[i - 2] <= (inOpen[i - 3] > inClose[i - 3] ? inOpen[i - 3] : inClose[i - 3]) + ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType === 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType === 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
        inOpen[i - 1] >= (inOpen[i - 2] < inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2]) - ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) && /* 3rd opens within/near 2nd rb */
        inOpen[i - 1] <= (inOpen[i - 2] > inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2]) + ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
        (((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === 1 &&  /* if three white */
          inClose[i - 1] > inClose[i - 2] &&
          inClose[i - 2] > inClose[i - 3] &&                    /* consecutive higher closes */
          inOpen[i] > inClose[i - 1] &&                         /* 4th opens above prior close */
          inClose[i] < inOpen[i - 3] ||                         /* 4th closes below 1st open */
         ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === -1 && /* if three black */
          inClose[i - 1] < inClose[i - 2] &&
          inClose[i - 2] < inClose[i - 3] &&                    /* consecutive lower closes */
          inOpen[i] < inClose[i - 1] &&                         /* 4th opens below prior close */
          inClose[i] > inOpen[i - 3]))                          /* 4th closes above 1st open */
    {
      outInteger[((outIdx++ * outStride) | 0)] = ((((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) * 100) | 0);
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    NearPeriodTotalRange2 = ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
    NearPeriodTotalTrailRange2 = ((Near_rangeType === 0) ? (Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : ((Near_rangeType === 1) ? (inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) : ((Near_rangeType === 2) ? ((inHigh[NearTrailingIdx - 2] - (((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inClose[NearTrailingIdx - 2]) : (inOpen[NearTrailingIdx - 2]))) + ((((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inOpen[NearTrailingIdx - 2]) : (inClose[NearTrailingIdx - 2])) - inLow[NearTrailingIdx - 2])) : 0.0)));
    NearPeriodTotal[3] = NearPeriodTotal[3] + (NearPeriodTotalRange3 - NearPeriodTotalTrailRange3);
    NearPeriodTotal[2] = NearPeriodTotal[2] + (NearPeriodTotalRange2 - NearPeriodTotalTrailRange2);
    NearPeriodTotalRange3 = NearPeriodTotalRange2;
    NearPeriodTotalTrailRange3 = NearPeriodTotalTrailRange2;
    i += 1;
    NearTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const capLag_NearTrailingIdx = i - NearTrailingIdx;
  const cap_NearTrailingIdx = capLag_NearTrailingIdx + 3;
  if (capLag_NearTrailingIdx < 0 || cap_NearTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_NearTrailingIdx = cap_NearTrailingIdx > 0 ? cap_NearTrailingIdx : 1;
  const capRing_NearTrailingIdx_derived = new Float64Array(allocN_NearTrailingIdx);
  for (let fillJ = historyLen - cap_NearTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_NearTrailingIdx_derived[fillJ % cap_NearTrailingIdx] = ((Near_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((Near_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((Near_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  sp.NearPeriodTotal = NearPeriodTotal;
  sp.NearPeriodTotalRange3 = NearPeriodTotalRange3;
  sp.NearPeriodTotalTrailRange3 = NearPeriodTotalTrailRange3;
  sp.lag1_inOpen = inOpen[historyLen - 1];
  sp.lag2_inOpen = inOpen[historyLen - 2];
  sp.lag3_inOpen = inOpen[historyLen - 3];
  sp.lag1_inHigh = inHigh[historyLen - 1];
  sp.lag2_inHigh = inHigh[historyLen - 2];
  sp.lag3_inHigh = inHigh[historyLen - 3];
  sp.lag1_inLow = inLow[historyLen - 1];
  sp.lag2_inLow = inLow[historyLen - 2];
  sp.lag3_inLow = inLow[historyLen - 3];
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.lag2_inClose = inClose[historyLen - 2];
  sp.lag3_inClose = inClose[historyLen - 3];
  sp.ringPos_NearTrailingIdx = historyLen % cap_NearTrailingIdx;
  sp.ringCap_NearTrailingIdx = cap_NearTrailingIdx;
  sp.ringLag_NearTrailingIdx = capLag_NearTrailingIdx;
  sp.ring_NearTrailingIdx_derived = capRing_NearTrailingIdx_derived;
  sp.cs_Near_rangeType = Near_rangeType;
  sp.cs_Near_avgPeriod = Near_avgPeriod;
  sp.cs_Near_factor = Near_factor;
  sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cdl3linestrikeOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): Cdl3linestrikeStream {
  const sp = Cdl3linestrikeStream.of(core);
  const retCode = cdl3linestrikeOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDL3LINESTRIKE openAndFill', inOpen.length, startIdx, cdl3linestrikeLookbackPositional(core));
  }
  throw streamFailure('CDL3LINESTRIKE openAndFill', retCode);
}

/** @internal */
export function cdl3linestrikeOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): Cdl3linestrikeStream {
  const sp = Cdl3linestrikeStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdl3linestrikeOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDL3LINESTRIKE open', inOpen.length, startIdx, cdl3linestrikeLookbackPositional(core));
  }
  throw streamFailure('CDL3LINESTRIKE open', retCode);
}

/** @internal */
export function cdl3linestrikeOpenPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): Cdl3linestrikeStream {
  requireArgument('CDL3LINESTRIKE open', 'prices.open', inOpen);
  requireHistory('CDL3LINESTRIKE open', 'prices.open', inOpen.length);
  requireArgument('CDL3LINESTRIKE open', 'prices.high', inHigh);
  requireArgument('CDL3LINESTRIKE open', 'prices.low', inLow);
  requireArgument('CDL3LINESTRIKE open', 'prices.close', inClose);
  requireHistoryLength('CDL3LINESTRIKE open', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDL3LINESTRIKE open', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDL3LINESTRIKE open', 'prices.close', inClose.length, inOpen.length);
  return cdl3linestrikeOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/** @internal */
export function cdl3linestrikeOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): Cdl3linestrikeStream {
  requireArgument('CDL3LINESTRIKE openAndFill', 'prices.open', inOpen);
  requireHistory('CDL3LINESTRIKE openAndFill', 'prices.open', inOpen.length);
  requireArgument('CDL3LINESTRIKE openAndFill', 'prices.high', inHigh);
  requireArgument('CDL3LINESTRIKE openAndFill', 'prices.low', inLow);
  requireArgument('CDL3LINESTRIKE openAndFill', 'prices.close', inClose);
  const guardOutLen = openFillCount('CDL3LINESTRIKE openAndFill', inOpen.length, cdl3linestrikeLookbackPositional(core));
  requireHistoryLength('CDL3LINESTRIKE openAndFill', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDL3LINESTRIKE openAndFill', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDL3LINESTRIKE openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('CDL3LINESTRIKE openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('CDL3LINESTRIKE openAndFill', 'outInteger', 'prices.open');
  }
  if (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CDL3LINESTRIKE openAndFill', 'outInteger', 'prices.high');
  }
  if (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CDL3LINESTRIKE openAndFill', 'outInteger', 'prices.low');
  }
  if (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('CDL3LINESTRIKE openAndFill', 'outInteger', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDL3LINESTRIKE openAndFill', 'outInteger', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDL3LINESTRIKE openAndFill', 'outInteger', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDL3LINESTRIKE openAndFill', 'outInteger', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDL3LINESTRIKE openAndFill', 'outInteger', 'prices.close');
  }
  return cdl3linestrikeOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}

/**
 * Opens a live CDL3LINESTRIKE stream over the warm-up history: `value()` starts at
 * the last history bar, bit-identical to {@link cdl3linestrike} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link Cdl3linestrikeOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdl3linestrikeLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input or histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdl3linestrikeOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdl3linestrikeOpen({ open, high, low, close });
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdl3linestrikeOpen(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: Cdl3linestrikeOptions): Cdl3linestrikeStream {
  requireArgument('CDL3LINESTRIKE open', 'prices', prices);
  const o = optionsOf('CDL3LINESTRIKE open', options);
  const core = coreOf('CDL3LINESTRIKE open', o.core);
  return cdl3linestrikeOpenPositional(prices.open, prices.high, prices.low, prices.close, core);
}

/**
 * {@link cdl3linestrikeOpen} that also computes the outputs over the whole history
 * in the same pass, bit-identically to {@link cdl3linestrike}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link Cdl3linestrikeOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cdl3linestrike} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdl3linestrikeLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, histories of different lengths or a buffer too short,
 *   of the wrong type or starting where another buffer or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdl3linestrikeOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = cdl3linestrikeOpenAndFill({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cdl3linestrikeOpenAndFill(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: Cdl3linestrikeOptions): Cdl3linestrikeResult & { stream: Cdl3linestrikeStream } {
  requireArgument('CDL3LINESTRIKE openAndFill', 'prices', prices);
  const o = optionsOf('CDL3LINESTRIKE openAndFill', options);
  const core = coreOf('CDL3LINESTRIKE openAndFill', o.core);
  const capacity = fillCapacity(cdl3linestrikeLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const stream = cdl3linestrikeOpenAndFillPositional(prices.open, prices.high, prices.low, prices.close, outInteger, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}
