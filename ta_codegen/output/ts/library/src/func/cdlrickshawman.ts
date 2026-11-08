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
 *  011505 AC     Creation
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 *  103126 MF,CC  Nest the cheap, selective pre-check conjuncts in an outer if
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { CandleSettingType, Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function cdlrickshawmanLookbackPositional(core: Core = Core.DEFAULT): number {
  const BodyDoji_avgPeriod = core.candleSettings[CandleSettingType.BODY_DOJI].avgPeriod;
  const Near_avgPeriod = core.candleSettings[CandleSettingType.NEAR].avgPeriod;
  const ShadowLong_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_LONG].avgPeriod;
  return ((BodyDoji_avgPeriod > ShadowLong_avgPeriod ? BodyDoji_avgPeriod : ShadowLong_avgPeriod) > Near_avgPeriod ? (BodyDoji_avgPeriod > ShadowLong_avgPeriod ? BodyDoji_avgPeriod : ShadowLong_avgPeriod) : Near_avgPeriod);
}

/** @internal */
export function cdlrickshawmanDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdlrickshawmanImpl(
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
  let BodyDojiPeriodTotal = 0;
  let ShadowLongPeriodTotal = 0;
  let NearPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let BodyDojiTrailingIdx = 0;
  let ShadowLongTrailingIdx = 0;
  let NearTrailingIdx = 0;
  let lookbackTotal = 0;
  const BodyDoji_rangeType: number = core.candleSettings[CandleSettingType.BODY_DOJI].rangeType;
  const BodyDoji_avgPeriod = core.candleSettings[CandleSettingType.BODY_DOJI].avgPeriod;
  const BodyDoji_factor = core.candleSettings[CandleSettingType.BODY_DOJI].factor;
  const Near_rangeType: number = core.candleSettings[CandleSettingType.NEAR].rangeType;
  const Near_avgPeriod = core.candleSettings[CandleSettingType.NEAR].avgPeriod;
  const Near_factor = core.candleSettings[CandleSettingType.NEAR].factor;
  const ShadowLong_rangeType: number = core.candleSettings[CandleSettingType.SHADOW_LONG].rangeType;
  const ShadowLong_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_LONG].avgPeriod;
  const ShadowLong_factor = core.candleSettings[CandleSettingType.SHADOW_LONG].factor;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlrickshawmanLookbackPositional(core);
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
  BodyDojiPeriodTotal = 0;
  BodyDojiTrailingIdx = startIdx - BodyDoji_avgPeriod;
  ShadowLongPeriodTotal = 0;
  ShadowLongTrailingIdx = startIdx - ShadowLong_avgPeriod;
  NearPeriodTotal = 0;
  NearTrailingIdx = startIdx - Near_avgPeriod;
  i = BodyDojiTrailingIdx;
  while (i < startIdx) {
    BodyDojiPeriodTotal += ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = ShadowLongTrailingIdx;
  while (i < startIdx) {
    ShadowLongPeriodTotal += ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = NearTrailingIdx;
  while (i < startIdx) {
    NearPeriodTotal += ((Near_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((Near_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((Near_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  /* Proceed with the calculation for the requested range.
   *
   * Must have:
   * - doji body
   * - two long shadows
   * - body near the midpoint of the high-low range
   * The meaning of "doji" and "near" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (Math.abs(inClose[i] - inOpen[i]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod !== 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyDoji_rangeType === 2) ? 2.0 : 1.0)))) && /* doji */
        (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) > ((ShadowLong_factor * (((ShadowLong_avgPeriod !== 0) ? (ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowLong_rangeType === 2) ? 2.0 : 1.0)))) && /* long shadow */
        (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowLong_factor * (((ShadowLong_avgPeriod !== 0) ? (ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowLong_rangeType === 2) ? 2.0 : 1.0)))) && /* long shadow */
        ((inOpen[i] < inClose[i] ? inOpen[i] : inClose[i]) <= inLow[i] + (inHigh[i] - inLow[i]) / 2 + ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((Near_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((Near_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) && /* body near midpoint */
         (inOpen[i] > inClose[i] ? inOpen[i] : inClose[i]) >= inLow[i] + (inHigh[i] - inLow[i]) / 2 - ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((Near_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((Near_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0))))))
    {
      outInteger[outIdx++] = 100;
    } else {
      outInteger[outIdx++] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyDojiPeriodTotal += ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : ((BodyDoji_rangeType === 1) ? (inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) : ((BodyDoji_rangeType === 2) ? ((inHigh[BodyDojiTrailingIdx] - (((inClose[BodyDojiTrailingIdx]) >= (inOpen[BodyDojiTrailingIdx])) ? (inClose[BodyDojiTrailingIdx]) : (inOpen[BodyDojiTrailingIdx]))) + ((((inClose[BodyDojiTrailingIdx]) >= (inOpen[BodyDojiTrailingIdx])) ? (inOpen[BodyDojiTrailingIdx]) : (inClose[BodyDojiTrailingIdx])) - inLow[BodyDojiTrailingIdx])) : 0.0)));
    ShadowLongPeriodTotal += ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[ShadowLongTrailingIdx] - inOpen[ShadowLongTrailingIdx])) : ((ShadowLong_rangeType === 1) ? (inHigh[ShadowLongTrailingIdx] - inLow[ShadowLongTrailingIdx]) : ((ShadowLong_rangeType === 2) ? ((inHigh[ShadowLongTrailingIdx] - (((inClose[ShadowLongTrailingIdx]) >= (inOpen[ShadowLongTrailingIdx])) ? (inClose[ShadowLongTrailingIdx]) : (inOpen[ShadowLongTrailingIdx]))) + ((((inClose[ShadowLongTrailingIdx]) >= (inOpen[ShadowLongTrailingIdx])) ? (inOpen[ShadowLongTrailingIdx]) : (inClose[ShadowLongTrailingIdx])) - inLow[ShadowLongTrailingIdx])) : 0.0)));
    NearPeriodTotal += ((Near_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((Near_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((Near_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((Near_rangeType === 0) ? (Math.abs(inClose[NearTrailingIdx] - inOpen[NearTrailingIdx])) : ((Near_rangeType === 1) ? (inHigh[NearTrailingIdx] - inLow[NearTrailingIdx]) : ((Near_rangeType === 2) ? ((inHigh[NearTrailingIdx] - (((inClose[NearTrailingIdx]) >= (inOpen[NearTrailingIdx])) ? (inClose[NearTrailingIdx]) : (inOpen[NearTrailingIdx]))) + ((((inClose[NearTrailingIdx]) >= (inOpen[NearTrailingIdx])) ? (inOpen[NearTrailingIdx]) : (inClose[NearTrailingIdx])) - inLow[NearTrailingIdx])) : 0.0)));
    i += 1;
    BodyDojiTrailingIdx += 1;
    ShadowLongTrailingIdx += 1;
    NearTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlrickshawmanPositional(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDLRICKSHAWMAN', 'startIdx', startIdx);
  endIdx = requireInt32('CDLRICKSHAWMAN', 'endIdx', endIdx);
  requireIndexRange('CDLRICKSHAWMAN', startIdx, endIdx);
  const guardStart = clampedStart('CDLRICKSHAWMAN', startIdx, cdlrickshawmanLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLRICKSHAWMAN', 'prices.open', inOpen, guardInLen);
  requireLength('CDLRICKSHAWMAN', 'prices.high', inHigh, guardInLen);
  requireLength('CDLRICKSHAWMAN', 'prices.low', inLow, guardInLen);
  requireLength('CDLRICKSHAWMAN', 'prices.close', inClose, guardInLen);
  requireOutput('CDLRICKSHAWMAN', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLRICKSHAWMAN', 'outInteger', 'prices.open');
  }
  if (overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLRICKSHAWMAN', 'outInteger', 'prices.high');
  }
  if (overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLRICKSHAWMAN', 'outInteger', 'prices.low');
  }
  if (overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLRICKSHAWMAN', 'outInteger', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdlrickshawmanImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLRICKSHAWMAN', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cdlrickshawman}, {@link cdlrickshawmanLookback},
 * {@link cdlrickshawmanDisplayShift}, {@link cdlrickshawmanOpen} and
 * {@link cdlrickshawmanOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface CdlrickshawmanOptions {
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
 * What {@link cdlrickshawman} returns, and {@link cdlrickshawmanOpenAndFill} with
 * its stream.
 */
export interface CdlrickshawmanResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * +100 when the pattern is present, 0 otherwise. Never -100; the positive value
   * does not imply bullish, it signals uncertainty. `values[k]` is the value at
   * input bar `begIdx + k`.
   */
  values: Int32Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link cdlrickshawman} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlrickshawmanOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlrickshawmanLookback(options?: CdlrickshawmanOptions): number {
  const o = optionsOf('CDLRICKSHAWMAN lookback', options);
  return cdlrickshawmanLookbackPositional(coreOf('CDLRICKSHAWMAN lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdlrickshawman}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlrickshawmanOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlrickshawmanDisplayShift(outputIdx: number, options?: CdlrickshawmanOptions): number {
  const o = optionsOf('CDLRICKSHAWMAN displayShift', options);
  return cdlrickshawmanDisplayShiftPositional(outputIdx, coreOf('CDLRICKSHAWMAN displayShift', o.core));
}

/**
 * Single-candle doji with two long shadows whose body sits near the midpoint of
 * the high-low range. It is a neutral indecision signal, not a directional
 * (bullish/bearish) reversal. A hit marks market indecision/uncertainty; neutral,
 * neither bullish nor bearish.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlrickshawman](https://ta-lib.org/functions/cdlrickshawman).
 *
 * **Notes**
 * - Bulkowski's verdict: "The rickshaw man candle may look pretty on the chart but
 *   it has no investment implications that I have been able to find" — his testing
 *   shows it continues only 51% of the time, statistically random.
 *   ([thepatternsite.com](https://thepatternsite.com/RickshawMan.html))
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cdlrickshawmanLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `open`: Open price of
 *   each bar. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link CdlrickshawmanOptions}.
 * @returns `begIdx` and the values; see {@link CdlrickshawmanResult}. Each output
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
 * import { cdlrickshawman } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { begIdx, values } = cdlrickshawman({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`cdllongleggeddoji`](https://ta-lib.org/functions/cdllongleggeddoji)
 * @see [`cdldoji`](https://ta-lib.org/functions/cdldoji)
 * @see [`cdlhighwave`](https://ta-lib.org/functions/cdlhighwave)
 */
export function cdlrickshawman(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlrickshawmanOptions): CdlrickshawmanResult {
  requireArgument('CDLRICKSHAWMAN', 'prices', prices);
  const o = optionsOf('CDLRICKSHAWMAN', options);
  if (o.endIdx === undefined) {
    requireSeries('CDLRICKSHAWMAN', 'prices.open', prices.open);
    requireSeries('CDLRICKSHAWMAN', 'prices.high', prices.high);
    requireSeries('CDLRICKSHAWMAN', 'prices.low', prices.low);
    requireSeries('CDLRICKSHAWMAN', 'prices.close', prices.close);
  }
  const core = coreOf('CDLRICKSHAWMAN', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cdlrickshawmanLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const range = cdlrickshawmanPositional(startIdx, endIdx, prices.open, prices.high, prices.low, prices.close, outInteger, core);
  return { begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CDLRICKSHAWMAN stream: one value per closed bar, bit-identical to
 * {@link cdlrickshawman} over the same series. Open it with
 * {@link cdlrickshawmanOpen} or {@link cdlrickshawmanOpenAndFill}; there is
 * nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlrickshawman](https://ta-lib.org/functions/cdlrickshawman).
 */
export class CdlrickshawmanStream {
  /** @internal */
  core: Core;
  /** @internal */
  BodyDojiPeriodTotal: number = 0;
  /** @internal */
  ShadowLongPeriodTotal: number = 0;
  /** @internal */
  NearPeriodTotal: number = 0;
  /** @internal */
  ringPos_BodyDojiTrailingIdx: number = 0;
  /** @internal */
  ringCap_BodyDojiTrailingIdx: number = 0;
  /** @internal */
  ring_BodyDojiTrailingIdx_derived: Float64Array = new Float64Array(1);
  /** @internal */
  ringPos_NearTrailingIdx: number = 0;
  /** @internal */
  ringCap_NearTrailingIdx: number = 0;
  /** @internal */
  ring_NearTrailingIdx_derived: Float64Array = new Float64Array(1);
  /** @internal */
  ringPos_ShadowLongTrailingIdx: number = 0;
  /** @internal */
  ringCap_ShadowLongTrailingIdx: number = 0;
  /** @internal */
  ring_ShadowLongTrailingIdx_derived: Float64Array = new Float64Array(1);
  /** @internal */
  cs_BodyDoji_rangeType: number = 0;
  /** @internal */
  cs_BodyDoji_avgPeriod: number = 0;
  /** @internal */
  cs_BodyDoji_factor: number = 0;
  /** @internal */
  cs_Near_rangeType: number = 0;
  /** @internal */
  cs_Near_avgPeriod: number = 0;
  /** @internal */
  cs_Near_factor: number = 0;
  /** @internal */
  cs_ShadowLong_rangeType: number = 0;
  /** @internal */
  cs_ShadowLong_avgPeriod: number = 0;
  /** @internal */
  cs_ShadowLong_factor: number = 0;
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
  static of(core: Core): CdlrickshawmanStream {
    return new CdlrickshawmanStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cdlrickshawman} reports them over the same bars. Each accepted `update`
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
      throw streamFailure('CDLRICKSHAWMAN advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw streamFailure('CDLRICKSHAWMAN update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(open) || !Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('CDLRICKSHAWMAN update', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    cdlrickshawmanStepImpl(this, open, high, low, close);
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
      throw nonFiniteBar('CDLRICKSHAWMAN peek', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inOpen = open;
    const inHigh = high;
    const inLow = low;
    const inClose = close;
    let cur_outInteger = 0;
    const BodyDoji_rangeType = sp.cs_BodyDoji_rangeType;
    const BodyDoji_avgPeriod = sp.cs_BodyDoji_avgPeriod;
    const BodyDoji_factor = sp.cs_BodyDoji_factor;
    const Near_rangeType = sp.cs_Near_rangeType;
    const Near_avgPeriod = sp.cs_Near_avgPeriod;
    const Near_factor = sp.cs_Near_factor;
    const ShadowLong_rangeType = sp.cs_ShadowLong_rangeType;
    const ShadowLong_avgPeriod = sp.cs_ShadowLong_avgPeriod;
    const ShadowLong_factor = sp.cs_ShadowLong_factor;
    if (Math.abs(inClose - inOpen) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod !== 0) ? (sp.BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyDoji_rangeType === 1) ? (inHigh - inLow) : ((BodyDoji_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyDoji_rangeType === 2) ? 2.0 : 1.0)))) && /* doji */
        (((inClose >= inOpen) ? inOpen : inClose) - inLow) > ((ShadowLong_factor * (((ShadowLong_avgPeriod !== 0) ? (sp.ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowLong_rangeType === 2) ? 2.0 : 1.0)))) && /* long shadow */
        (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) > ((ShadowLong_factor * (((ShadowLong_avgPeriod !== 0) ? (sp.ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowLong_rangeType === 2) ? 2.0 : 1.0)))) && /* long shadow */
        ((inOpen < inClose ? inOpen : inClose) <= inLow + (inHigh - inLow) / 2 + ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((Near_rangeType === 1) ? (inHigh - inLow) : ((Near_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) && /* body near midpoint */
         (inOpen > inClose ? inOpen : inClose) >= inLow + (inHigh - inLow) / 2 - ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((Near_rangeType === 1) ? (inHigh - inLow) : ((Near_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0))))))
    {
      cur_outInteger = 100;
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
  clone(): CdlrickshawmanStream {
    const c = new CdlrickshawmanStream(this.core);
    c.BodyDojiPeriodTotal = this.BodyDojiPeriodTotal;
    c.ShadowLongPeriodTotal = this.ShadowLongPeriodTotal;
    c.NearPeriodTotal = this.NearPeriodTotal;
    c.ringPos_BodyDojiTrailingIdx = this.ringPos_BodyDojiTrailingIdx;
    c.ringCap_BodyDojiTrailingIdx = this.ringCap_BodyDojiTrailingIdx;
    c.ring_BodyDojiTrailingIdx_derived = this.ring_BodyDojiTrailingIdx_derived.slice();
    c.ringPos_NearTrailingIdx = this.ringPos_NearTrailingIdx;
    c.ringCap_NearTrailingIdx = this.ringCap_NearTrailingIdx;
    c.ring_NearTrailingIdx_derived = this.ring_NearTrailingIdx_derived.slice();
    c.ringPos_ShadowLongTrailingIdx = this.ringPos_ShadowLongTrailingIdx;
    c.ringCap_ShadowLongTrailingIdx = this.ringCap_ShadowLongTrailingIdx;
    c.ring_ShadowLongTrailingIdx_derived = this.ring_ShadowLongTrailingIdx_derived.slice();
    c.cs_BodyDoji_rangeType = this.cs_BodyDoji_rangeType;
    c.cs_BodyDoji_avgPeriod = this.cs_BodyDoji_avgPeriod;
    c.cs_BodyDoji_factor = this.cs_BodyDoji_factor;
    c.cs_Near_rangeType = this.cs_Near_rangeType;
    c.cs_Near_avgPeriod = this.cs_Near_avgPeriod;
    c.cs_Near_factor = this.cs_Near_factor;
    c.cs_ShadowLong_rangeType = this.cs_ShadowLong_rangeType;
    c.cs_ShadowLong_avgPeriod = this.cs_ShadowLong_avgPeriod;
    c.cs_ShadowLong_factor = this.cs_ShadowLong_factor;
    c.cur_outInteger = this.cur_outInteger;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cdlrickshawmanStepImpl(sp: CdlrickshawmanStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  const BodyDoji_rangeType = sp.cs_BodyDoji_rangeType;
  const BodyDoji_avgPeriod = sp.cs_BodyDoji_avgPeriod;
  const BodyDoji_factor = sp.cs_BodyDoji_factor;
  const Near_rangeType = sp.cs_Near_rangeType;
  const Near_avgPeriod = sp.cs_Near_avgPeriod;
  const Near_factor = sp.cs_Near_factor;
  const ShadowLong_rangeType = sp.cs_ShadowLong_rangeType;
  const ShadowLong_avgPeriod = sp.cs_ShadowLong_avgPeriod;
  const ShadowLong_factor = sp.cs_ShadowLong_factor;
  if (sp.ringCap_BodyDojiTrailingIdx === 0) {
    sp.ring_BodyDojiTrailingIdx_derived[0] = ((BodyDoji_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyDoji_rangeType === 1) ? (inHigh - inLow) : ((BodyDoji_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
  if (sp.ringCap_NearTrailingIdx === 0) {
    sp.ring_NearTrailingIdx_derived[0] = ((Near_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((Near_rangeType === 1) ? (inHigh - inLow) : ((Near_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
  if (sp.ringCap_ShadowLongTrailingIdx === 0) {
    sp.ring_ShadowLongTrailingIdx_derived[0] = ((ShadowLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
  if (Math.abs(inClose - inOpen) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod !== 0) ? (sp.BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyDoji_rangeType === 1) ? (inHigh - inLow) : ((BodyDoji_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyDoji_rangeType === 2) ? 2.0 : 1.0)))) && /* doji */
      (((inClose >= inOpen) ? inOpen : inClose) - inLow) > ((ShadowLong_factor * (((ShadowLong_avgPeriod !== 0) ? (sp.ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowLong_rangeType === 2) ? 2.0 : 1.0)))) && /* long shadow */
      (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) > ((ShadowLong_factor * (((ShadowLong_avgPeriod !== 0) ? (sp.ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowLong_rangeType === 2) ? 2.0 : 1.0)))) && /* long shadow */
      ((inOpen < inClose ? inOpen : inClose) <= inLow + (inHigh - inLow) / 2 + ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((Near_rangeType === 1) ? (inHigh - inLow) : ((Near_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) && /* body near midpoint */
       (inOpen > inClose ? inOpen : inClose) >= inLow + (inHigh - inLow) / 2 - ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((Near_rangeType === 1) ? (inHigh - inLow) : ((Near_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0))))))
  {
    sp.cur_outInteger = 100;
  } else {
    sp.cur_outInteger = 0;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  sp.BodyDojiPeriodTotal += ((BodyDoji_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyDoji_rangeType === 1) ? (inHigh - inLow) : ((BodyDoji_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0))) - sp.ring_BodyDojiTrailingIdx_derived[sp.ringPos_BodyDojiTrailingIdx];
  sp.ShadowLongPeriodTotal += ((ShadowLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0))) - sp.ring_ShadowLongTrailingIdx_derived[sp.ringPos_ShadowLongTrailingIdx];
  sp.NearPeriodTotal += ((Near_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((Near_rangeType === 1) ? (inHigh - inLow) : ((Near_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0))) - sp.ring_NearTrailingIdx_derived[sp.ringPos_NearTrailingIdx];
  sp.ring_BodyDojiTrailingIdx_derived[sp.ringPos_BodyDojiTrailingIdx] = ((BodyDoji_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyDoji_rangeType === 1) ? (inHigh - inLow) : ((BodyDoji_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  sp.ringPos_BodyDojiTrailingIdx = sp.ringPos_BodyDojiTrailingIdx + 1;
  if (sp.ringPos_BodyDojiTrailingIdx >= sp.ringCap_BodyDojiTrailingIdx) {
    sp.ringPos_BodyDojiTrailingIdx = 0;
  }
  sp.ring_NearTrailingIdx_derived[sp.ringPos_NearTrailingIdx] = ((Near_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((Near_rangeType === 1) ? (inHigh - inLow) : ((Near_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  sp.ringPos_NearTrailingIdx = sp.ringPos_NearTrailingIdx + 1;
  if (sp.ringPos_NearTrailingIdx >= sp.ringCap_NearTrailingIdx) {
    sp.ringPos_NearTrailingIdx = 0;
  }
  sp.ring_ShadowLongTrailingIdx_derived[sp.ringPos_ShadowLongTrailingIdx] = ((ShadowLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  sp.ringPos_ShadowLongTrailingIdx = sp.ringPos_ShadowLongTrailingIdx + 1;
  if (sp.ringPos_ShadowLongTrailingIdx >= sp.ringCap_ShadowLongTrailingIdx) {
    sp.ringPos_ShadowLongTrailingIdx = 0;
  }
}

function cdlrickshawmanOpenImpl(
  sp: CdlrickshawmanStream,
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
  let BodyDojiPeriodTotal = 0;
  let ShadowLongPeriodTotal = 0;
  let NearPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let BodyDojiTrailingIdx = 0;
  let ShadowLongTrailingIdx = 0;
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
  const BodyDoji_rangeType: number = core.candleSettings[CandleSettingType.BODY_DOJI].rangeType;
  const BodyDoji_avgPeriod = core.candleSettings[CandleSettingType.BODY_DOJI].avgPeriod;
  const BodyDoji_factor = core.candleSettings[CandleSettingType.BODY_DOJI].factor;
  const Near_rangeType: number = core.candleSettings[CandleSettingType.NEAR].rangeType;
  const Near_avgPeriod = core.candleSettings[CandleSettingType.NEAR].avgPeriod;
  const Near_factor = core.candleSettings[CandleSettingType.NEAR].factor;
  const ShadowLong_rangeType: number = core.candleSettings[CandleSettingType.SHADOW_LONG].rangeType;
  const ShadowLong_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_LONG].avgPeriod;
  const ShadowLong_factor = core.candleSettings[CandleSettingType.SHADOW_LONG].factor;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlrickshawmanLookbackPositional(core);
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
  BodyDojiPeriodTotal = 0;
  BodyDojiTrailingIdx = startIdx - BodyDoji_avgPeriod;
  ShadowLongPeriodTotal = 0;
  ShadowLongTrailingIdx = startIdx - ShadowLong_avgPeriod;
  NearPeriodTotal = 0;
  NearTrailingIdx = startIdx - Near_avgPeriod;
  i = BodyDojiTrailingIdx;
  while (i < startIdx) {
    BodyDojiPeriodTotal += ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = ShadowLongTrailingIdx;
  while (i < startIdx) {
    ShadowLongPeriodTotal += ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = NearTrailingIdx;
  while (i < startIdx) {
    NearPeriodTotal += ((Near_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((Near_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((Near_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  /* Proceed with the calculation for the requested range.
   *
   * Must have:
   * - doji body
   * - two long shadows
   * - body near the midpoint of the high-low range
   * The meaning of "doji" and "near" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (Math.abs(inClose[i] - inOpen[i]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod !== 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyDoji_rangeType === 2) ? 2.0 : 1.0)))) && /* doji */
        (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) > ((ShadowLong_factor * (((ShadowLong_avgPeriod !== 0) ? (ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowLong_rangeType === 2) ? 2.0 : 1.0)))) && /* long shadow */
        (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowLong_factor * (((ShadowLong_avgPeriod !== 0) ? (ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowLong_rangeType === 2) ? 2.0 : 1.0)))) && /* long shadow */
        ((inOpen[i] < inClose[i] ? inOpen[i] : inClose[i]) <= inLow[i] + (inHigh[i] - inLow[i]) / 2 + ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((Near_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((Near_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) && /* body near midpoint */
         (inOpen[i] > inClose[i] ? inOpen[i] : inClose[i]) >= inLow[i] + (inHigh[i] - inLow[i]) / 2 - ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((Near_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((Near_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0))))))
    {
      outInteger[((outIdx++ * outStride) | 0)] = 100;
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyDojiPeriodTotal += ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : ((BodyDoji_rangeType === 1) ? (inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) : ((BodyDoji_rangeType === 2) ? ((inHigh[BodyDojiTrailingIdx] - (((inClose[BodyDojiTrailingIdx]) >= (inOpen[BodyDojiTrailingIdx])) ? (inClose[BodyDojiTrailingIdx]) : (inOpen[BodyDojiTrailingIdx]))) + ((((inClose[BodyDojiTrailingIdx]) >= (inOpen[BodyDojiTrailingIdx])) ? (inOpen[BodyDojiTrailingIdx]) : (inClose[BodyDojiTrailingIdx])) - inLow[BodyDojiTrailingIdx])) : 0.0)));
    ShadowLongPeriodTotal += ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[ShadowLongTrailingIdx] - inOpen[ShadowLongTrailingIdx])) : ((ShadowLong_rangeType === 1) ? (inHigh[ShadowLongTrailingIdx] - inLow[ShadowLongTrailingIdx]) : ((ShadowLong_rangeType === 2) ? ((inHigh[ShadowLongTrailingIdx] - (((inClose[ShadowLongTrailingIdx]) >= (inOpen[ShadowLongTrailingIdx])) ? (inClose[ShadowLongTrailingIdx]) : (inOpen[ShadowLongTrailingIdx]))) + ((((inClose[ShadowLongTrailingIdx]) >= (inOpen[ShadowLongTrailingIdx])) ? (inOpen[ShadowLongTrailingIdx]) : (inClose[ShadowLongTrailingIdx])) - inLow[ShadowLongTrailingIdx])) : 0.0)));
    NearPeriodTotal += ((Near_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((Near_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((Near_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((Near_rangeType === 0) ? (Math.abs(inClose[NearTrailingIdx] - inOpen[NearTrailingIdx])) : ((Near_rangeType === 1) ? (inHigh[NearTrailingIdx] - inLow[NearTrailingIdx]) : ((Near_rangeType === 2) ? ((inHigh[NearTrailingIdx] - (((inClose[NearTrailingIdx]) >= (inOpen[NearTrailingIdx])) ? (inClose[NearTrailingIdx]) : (inOpen[NearTrailingIdx]))) + ((((inClose[NearTrailingIdx]) >= (inOpen[NearTrailingIdx])) ? (inOpen[NearTrailingIdx]) : (inClose[NearTrailingIdx])) - inLow[NearTrailingIdx])) : 0.0)));
    i += 1;
    BodyDojiTrailingIdx += 1;
    ShadowLongTrailingIdx += 1;
    NearTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const cap_BodyDojiTrailingIdx = i - BodyDojiTrailingIdx;
  if (cap_BodyDojiTrailingIdx < 0 || cap_BodyDojiTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_BodyDojiTrailingIdx = cap_BodyDojiTrailingIdx > 0 ? cap_BodyDojiTrailingIdx : 1;
  const capRing_BodyDojiTrailingIdx_derived = new Float64Array(allocN_BodyDojiTrailingIdx);
  for (let fillJ = historyLen - cap_BodyDojiTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_BodyDojiTrailingIdx_derived[fillJ - (historyLen - cap_BodyDojiTrailingIdx)] = ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyDoji_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyDoji_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  const cap_NearTrailingIdx = i - NearTrailingIdx;
  if (cap_NearTrailingIdx < 0 || cap_NearTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_NearTrailingIdx = cap_NearTrailingIdx > 0 ? cap_NearTrailingIdx : 1;
  const capRing_NearTrailingIdx_derived = new Float64Array(allocN_NearTrailingIdx);
  for (let fillJ = historyLen - cap_NearTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_NearTrailingIdx_derived[fillJ - (historyLen - cap_NearTrailingIdx)] = ((Near_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((Near_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((Near_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
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
  sp.BodyDojiPeriodTotal = BodyDojiPeriodTotal;
  sp.ShadowLongPeriodTotal = ShadowLongPeriodTotal;
  sp.NearPeriodTotal = NearPeriodTotal;
  sp.ringPos_BodyDojiTrailingIdx = 0;
  sp.ringCap_BodyDojiTrailingIdx = cap_BodyDojiTrailingIdx;
  sp.ring_BodyDojiTrailingIdx_derived = capRing_BodyDojiTrailingIdx_derived;
  sp.ringPos_NearTrailingIdx = 0;
  sp.ringCap_NearTrailingIdx = cap_NearTrailingIdx;
  sp.ring_NearTrailingIdx_derived = capRing_NearTrailingIdx_derived;
  sp.ringPos_ShadowLongTrailingIdx = 0;
  sp.ringCap_ShadowLongTrailingIdx = cap_ShadowLongTrailingIdx;
  sp.ring_ShadowLongTrailingIdx_derived = capRing_ShadowLongTrailingIdx_derived;
  sp.cs_BodyDoji_rangeType = BodyDoji_rangeType;
  sp.cs_BodyDoji_avgPeriod = BodyDoji_avgPeriod;
  sp.cs_BodyDoji_factor = BodyDoji_factor;
  sp.cs_Near_rangeType = Near_rangeType;
  sp.cs_Near_avgPeriod = Near_avgPeriod;
  sp.cs_Near_factor = Near_factor;
  sp.cs_ShadowLong_rangeType = ShadowLong_rangeType;
  sp.cs_ShadowLong_avgPeriod = ShadowLong_avgPeriod;
  sp.cs_ShadowLong_factor = ShadowLong_factor;
  sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlrickshawmanOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): CdlrickshawmanStream {
  const sp = CdlrickshawmanStream.of(core);
  const retCode = cdlrickshawmanOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLRICKSHAWMAN openAndFill', inOpen.length, startIdx, cdlrickshawmanLookbackPositional(core));
  }
  throw streamFailure('CDLRICKSHAWMAN openAndFill', retCode);
}

/** @internal */
export function cdlrickshawmanOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): CdlrickshawmanStream {
  const sp = CdlrickshawmanStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdlrickshawmanOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLRICKSHAWMAN open', inOpen.length, startIdx, cdlrickshawmanLookbackPositional(core));
  }
  throw streamFailure('CDLRICKSHAWMAN open', retCode);
}

/** @internal */
export function cdlrickshawmanOpenPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): CdlrickshawmanStream {
  requireArgument('CDLRICKSHAWMAN open', 'prices.open', inOpen);
  requireHistory('CDLRICKSHAWMAN open', 'prices.open', inOpen.length);
  requireArgument('CDLRICKSHAWMAN open', 'prices.high', inHigh);
  requireArgument('CDLRICKSHAWMAN open', 'prices.low', inLow);
  requireArgument('CDLRICKSHAWMAN open', 'prices.close', inClose);
  requireHistoryLength('CDLRICKSHAWMAN open', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLRICKSHAWMAN open', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLRICKSHAWMAN open', 'prices.close', inClose.length, inOpen.length);
  return cdlrickshawmanOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/** @internal */
export function cdlrickshawmanOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): CdlrickshawmanStream {
  requireArgument('CDLRICKSHAWMAN openAndFill', 'prices.open', inOpen);
  requireHistory('CDLRICKSHAWMAN openAndFill', 'prices.open', inOpen.length);
  requireArgument('CDLRICKSHAWMAN openAndFill', 'prices.high', inHigh);
  requireArgument('CDLRICKSHAWMAN openAndFill', 'prices.low', inLow);
  requireArgument('CDLRICKSHAWMAN openAndFill', 'prices.close', inClose);
  const guardOutLen = openFillCount('CDLRICKSHAWMAN openAndFill', inOpen.length, cdlrickshawmanLookbackPositional(core));
  requireHistoryLength('CDLRICKSHAWMAN openAndFill', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLRICKSHAWMAN openAndFill', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLRICKSHAWMAN openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('CDLRICKSHAWMAN openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('CDLRICKSHAWMAN openAndFill', 'outInteger', 'prices.open');
  }
  if (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CDLRICKSHAWMAN openAndFill', 'outInteger', 'prices.high');
  }
  if (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CDLRICKSHAWMAN openAndFill', 'outInteger', 'prices.low');
  }
  if (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('CDLRICKSHAWMAN openAndFill', 'outInteger', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLRICKSHAWMAN openAndFill', 'outInteger', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLRICKSHAWMAN openAndFill', 'outInteger', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLRICKSHAWMAN openAndFill', 'outInteger', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLRICKSHAWMAN openAndFill', 'outInteger', 'prices.close');
  }
  return cdlrickshawmanOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}

/**
 * Opens a live CDLRICKSHAWMAN stream over the warm-up history: `value()` starts at
 * the last history bar, bit-identical to {@link cdlrickshawman} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlrickshawmanOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlrickshawmanLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input or histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdlrickshawmanOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdlrickshawmanOpen({ open, high, low, close });
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdlrickshawmanOpen(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlrickshawmanOptions): CdlrickshawmanStream {
  requireArgument('CDLRICKSHAWMAN open', 'prices', prices);
  const o = optionsOf('CDLRICKSHAWMAN open', options);
  const core = coreOf('CDLRICKSHAWMAN open', o.core);
  return cdlrickshawmanOpenPositional(prices.open, prices.high, prices.low, prices.close, core);
}

/**
 * {@link cdlrickshawmanOpen} that also computes the outputs over the whole history
 * in the same pass, bit-identically to {@link cdlrickshawman}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CdlrickshawmanOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cdlrickshawman} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlrickshawmanLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, histories of different lengths or a buffer too short,
 *   of the wrong type or starting where another buffer or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdlrickshawmanOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = cdlrickshawmanOpenAndFill({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cdlrickshawmanOpenAndFill(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlrickshawmanOptions): CdlrickshawmanResult & { stream: CdlrickshawmanStream } {
  requireArgument('CDLRICKSHAWMAN openAndFill', 'prices', prices);
  const o = optionsOf('CDLRICKSHAWMAN openAndFill', options);
  const core = coreOf('CDLRICKSHAWMAN openAndFill', o.core);
  const capacity = fillCapacity(cdlrickshawmanLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const stream = cdlrickshawmanOpenAndFillPositional(prices.open, prices.high, prices.low, prices.close, outInteger, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}
