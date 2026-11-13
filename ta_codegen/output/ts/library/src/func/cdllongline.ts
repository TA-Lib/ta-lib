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
 *  071704 AC     Creation
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { CandleSettingType, Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function cdllonglineLookbackPositional(core: Core = Core.DEFAULT): number {
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  const ShadowShort_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_SHORT].avgPeriod;
  return (BodyLong_avgPeriod > ShadowShort_avgPeriod ? BodyLong_avgPeriod : ShadowShort_avgPeriod);
}

/** @internal */
export function cdllonglineDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdllonglineImpl(
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
  let ShadowPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let BodyTrailingIdx = 0;
  let ShadowTrailingIdx = 0;
  let lookbackTotal = 0;
  const BodyLong_rangeType: number = core.candleSettings[CandleSettingType.BODY_LONG].rangeType;
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  const BodyLong_factor = core.candleSettings[CandleSettingType.BODY_LONG].factor;
  const ShadowShort_rangeType: number = core.candleSettings[CandleSettingType.SHADOW_SHORT].rangeType;
  const ShadowShort_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_SHORT].avgPeriod;
  const ShadowShort_factor = core.candleSettings[CandleSettingType.SHADOW_SHORT].factor;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdllonglineLookbackPositional(core);
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
  BodyTrailingIdx = startIdx - BodyLong_avgPeriod;
  ShadowPeriodTotal = 0;
  ShadowTrailingIdx = startIdx - ShadowShort_avgPeriod;
  i = BodyTrailingIdx;
  while (i < startIdx) {
    BodyPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = ShadowTrailingIdx;
  while (i < startIdx) {
    ShadowPeriodTotal += ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - long real body
   * - short upper and lower shadow
   * The meaning of "long" and "short" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (Math.abs(inClose[i] - inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) < ((ShadowShort_factor * (((ShadowShort_avgPeriod !== 0) ? (ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowShort_rangeType === 2) ? 2.0 : 1.0)))) && (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) < ((ShadowShort_factor * (((ShadowShort_avgPeriod !== 0) ? (ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowShort_rangeType === 2) ? 2.0 : 1.0))))) {
      outInteger[outIdx++] = ((((inClose[i] >= inOpen[i]) ? 1 : -1) * 100) | 0);
    } else {
      outInteger[outIdx++] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyTrailingIdx] - (((inClose[BodyTrailingIdx]) >= (inOpen[BodyTrailingIdx])) ? (inClose[BodyTrailingIdx]) : (inOpen[BodyTrailingIdx]))) + ((((inClose[BodyTrailingIdx]) >= (inOpen[BodyTrailingIdx])) ? (inOpen[BodyTrailingIdx]) : (inClose[BodyTrailingIdx])) - inLow[BodyTrailingIdx])) : 0.0)));
    ShadowPeriodTotal += ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[ShadowTrailingIdx] - inOpen[ShadowTrailingIdx])) : ((ShadowShort_rangeType === 1) ? (inHigh[ShadowTrailingIdx] - inLow[ShadowTrailingIdx]) : ((ShadowShort_rangeType === 2) ? ((inHigh[ShadowTrailingIdx] - (((inClose[ShadowTrailingIdx]) >= (inOpen[ShadowTrailingIdx])) ? (inClose[ShadowTrailingIdx]) : (inOpen[ShadowTrailingIdx]))) + ((((inClose[ShadowTrailingIdx]) >= (inOpen[ShadowTrailingIdx])) ? (inOpen[ShadowTrailingIdx]) : (inClose[ShadowTrailingIdx])) - inLow[ShadowTrailingIdx])) : 0.0)));
    i += 1;
    BodyTrailingIdx += 1;
    ShadowTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function cdllonglinePositional(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDLLONGLINE', 'startIdx', startIdx);
  endIdx = requireInt32('CDLLONGLINE', 'endIdx', endIdx);
  requireIndexRange('CDLLONGLINE', startIdx, endIdx);
  const guardStart = clampedStart('CDLLONGLINE', startIdx, cdllonglineLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLLONGLINE', 'prices.open', inOpen, guardInLen);
  requireLength('CDLLONGLINE', 'prices.high', inHigh, guardInLen);
  requireLength('CDLLONGLINE', 'prices.low', inLow, guardInLen);
  requireLength('CDLLONGLINE', 'prices.close', inClose, guardInLen);
  requireOutput('CDLLONGLINE', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLLONGLINE', 'outInteger', 'prices.open');
  }
  if (overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLLONGLINE', 'outInteger', 'prices.high');
  }
  if (overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLLONGLINE', 'outInteger', 'prices.low');
  }
  if (overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLLONGLINE', 'outInteger', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdllonglineImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLLONGLINE', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cdllongline}, {@link cdllonglineLookback},
 * {@link cdllonglineDisplayShift}, {@link cdllonglineOpen} and
 * {@link cdllonglineOpenAndFill}. Every key is optional: an omitted or `undefined`
 * key takes its default, and a call ignores the keys it has no use for.
 */
export interface CdllonglineOptions {
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
 * What {@link cdllongline} returns, and {@link cdllonglineOpenAndFill} with its
 * stream.
 */
export interface CdllonglineResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * +100 on a white (close>=open) long line, -100 on a black long line, 0 when no
   * pattern. `values[k]` is the value at input bar `begIdx + k`.
   */
  values: Int32Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link cdllongline} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdllonglineOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdllonglineLookback(options?: CdllonglineOptions): number {
  const o = optionsOf('CDLLONGLINE lookback', options);
  return cdllonglineLookbackPositional(coreOf('CDLLONGLINE lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdllongline}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdllonglineOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdllonglineDisplayShift(outputIdx: number, options?: CdllonglineOptions): number {
  const o = optionsOf('CDLLONGLINE displayShift', options);
  return cdllonglineDisplayShiftPositional(outputIdx, coreOf('CDLLONGLINE displayShift', o.core));
}

/**
 * A single-candle pattern: a long real body with short upper and short lower
 * shadow. The signal direction follows the candle color (bullish if white, bearish
 * if black). Signals strong directional conviction on the bar. Not intrinsically a
 * reversal or continuation signal.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdllongline](https://ta-lib.org/functions/cdllongline).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cdllonglineLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `open`: Open price of
 *   each bar. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link CdllonglineOptions}.
 * @returns `begIdx` and the values; see {@link CdllonglineResult}. Each output is
 *   a view of its buffer, the caller's or a new one.
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
 * import { cdllongline } from 'ta-lib';
 *
 * const open = new Float64Array(252); // your data, oldest first
 * const high = new Float64Array(252); // your data, oldest first
 * const low = new Float64Array(252); // your data, oldest first
 * const close = new Float64Array(252); // your data, oldest first
 *
 * const { begIdx, values } = cdllongline({ open, high, low, close });
 * console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
 * ```
 * @see [`cdlshortline`](https://ta-lib.org/functions/cdlshortline)
 * @see [`cdlclosingmarubozu`](https://ta-lib.org/functions/cdlclosingmarubozu)
 * @see [`cdlmarubozu`](https://ta-lib.org/functions/cdlmarubozu)
 * @see [`cdllongleggeddoji`](https://ta-lib.org/functions/cdllongleggeddoji)
 */
export function cdllongline(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdllonglineOptions): CdllonglineResult {
  requireArgument('CDLLONGLINE', 'prices', prices);
  const o = optionsOf('CDLLONGLINE', options);
  if (o.endIdx === undefined) {
    requireSeries('CDLLONGLINE', 'prices.open', prices.open);
    requireSeries('CDLLONGLINE', 'prices.high', prices.high);
    requireSeries('CDLLONGLINE', 'prices.low', prices.low);
    requireSeries('CDLLONGLINE', 'prices.close', prices.close);
  }
  const core = coreOf('CDLLONGLINE', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cdllonglineLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const range = cdllonglinePositional(startIdx, endIdx, prices.open, prices.high, prices.low, prices.close, outInteger, core);
  return { begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CDLLONGLINE stream: one value per closed bar, bit-identical to
 * {@link cdllongline} over the same series. Open it with {@link cdllonglineOpen}
 * or {@link cdllonglineOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdllongline](https://ta-lib.org/functions/cdllongline).
 */
export class CdllonglineStream {
  /** @internal */
  core: Core;
  /** @internal */
  BodyPeriodTotal: number = 0;
  /** @internal */
  ShadowPeriodTotal: number = 0;
  /** @internal */
  ringPos_BodyTrailingIdx: number = 0;
  /** @internal */
  ringCap_BodyTrailingIdx: number = 0;
  /** @internal */
  ring_BodyTrailingIdx_derived: Float64Array = new Float64Array(1);
  /** @internal */
  ringPos_ShadowTrailingIdx: number = 0;
  /** @internal */
  ringCap_ShadowTrailingIdx: number = 0;
  /** @internal */
  ring_ShadowTrailingIdx_derived: Float64Array = new Float64Array(1);
  /** @internal */
  cs_BodyLong_rangeType: number = 0;
  /** @internal */
  cs_BodyLong_avgPeriod: number = 0;
  /** @internal */
  cs_BodyLong_factor: number = 0;
  /** @internal */
  cs_ShadowShort_rangeType: number = 0;
  /** @internal */
  cs_ShadowShort_avgPeriod: number = 0;
  /** @internal */
  cs_ShadowShort_factor: number = 0;
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
  static of(core: Core): CdllonglineStream {
    return new CdllonglineStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cdllongline} reports them over the same bars. Each accepted `update` and
   * each `advance` adds one to `count`.
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
      throw streamFailure('CDLLONGLINE advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw streamFailure('CDLLONGLINE update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(open) || !Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('CDLLONGLINE update', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    cdllonglineStepImpl(this, open, high, low, close);
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
      throw nonFiniteBar('CDLLONGLINE peek', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inOpen = open;
    const inHigh = high;
    const inLow = low;
    const inClose = close;
    let cur_outInteger = 0;
    const BodyLong_rangeType = sp.cs_BodyLong_rangeType;
    const BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
    const BodyLong_factor = sp.cs_BodyLong_factor;
    const ShadowShort_rangeType = sp.cs_ShadowShort_rangeType;
    const ShadowShort_avgPeriod = sp.cs_ShadowShort_avgPeriod;
    const ShadowShort_factor = sp.cs_ShadowShort_factor;
    if (Math.abs(inClose - inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType === 1) ? (inHigh - inLow) : ((BodyLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) < ((ShadowShort_factor * (((ShadowShort_avgPeriod !== 0) ? (sp.ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowShort_rangeType === 2) ? 2.0 : 1.0)))) && (((inClose >= inOpen) ? inOpen : inClose) - inLow) < ((ShadowShort_factor * (((ShadowShort_avgPeriod !== 0) ? (sp.ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowShort_rangeType === 2) ? 2.0 : 1.0))))) {
      cur_outInteger = ((((inClose >= inOpen) ? 1 : -1) * 100) | 0);
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
  clone(): CdllonglineStream {
    const c = new CdllonglineStream(this.core);
    c.BodyPeriodTotal = this.BodyPeriodTotal;
    c.ShadowPeriodTotal = this.ShadowPeriodTotal;
    c.ringPos_BodyTrailingIdx = this.ringPos_BodyTrailingIdx;
    c.ringCap_BodyTrailingIdx = this.ringCap_BodyTrailingIdx;
    c.ring_BodyTrailingIdx_derived = this.ring_BodyTrailingIdx_derived.slice();
    c.ringPos_ShadowTrailingIdx = this.ringPos_ShadowTrailingIdx;
    c.ringCap_ShadowTrailingIdx = this.ringCap_ShadowTrailingIdx;
    c.ring_ShadowTrailingIdx_derived = this.ring_ShadowTrailingIdx_derived.slice();
    c.cs_BodyLong_rangeType = this.cs_BodyLong_rangeType;
    c.cs_BodyLong_avgPeriod = this.cs_BodyLong_avgPeriod;
    c.cs_BodyLong_factor = this.cs_BodyLong_factor;
    c.cs_ShadowShort_rangeType = this.cs_ShadowShort_rangeType;
    c.cs_ShadowShort_avgPeriod = this.cs_ShadowShort_avgPeriod;
    c.cs_ShadowShort_factor = this.cs_ShadowShort_factor;
    c.cur_outInteger = this.cur_outInteger;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cdllonglineStepImpl(sp: CdllonglineStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  let ringPosL_BodyTrailingIdx = 0;
  let ringPosL_ShadowTrailingIdx = 0;
  const BodyLong_rangeType = sp.cs_BodyLong_rangeType;
  const BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
  const BodyLong_factor = sp.cs_BodyLong_factor;
  const ShadowShort_rangeType = sp.cs_ShadowShort_rangeType;
  const ShadowShort_avgPeriod = sp.cs_ShadowShort_avgPeriod;
  const ShadowShort_factor = sp.cs_ShadowShort_factor;
  if (sp.ringCap_BodyTrailingIdx === 0) {
    sp.ring_BodyTrailingIdx_derived[0] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType === 1) ? (inHigh - inLow) : ((BodyLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
  if (sp.ringCap_ShadowTrailingIdx === 0) {
    sp.ring_ShadowTrailingIdx_derived[0] = ((ShadowShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
  if (Math.abs(inClose - inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType === 1) ? (inHigh - inLow) : ((BodyLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) < ((ShadowShort_factor * (((ShadowShort_avgPeriod !== 0) ? (sp.ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowShort_rangeType === 2) ? 2.0 : 1.0)))) && (((inClose >= inOpen) ? inOpen : inClose) - inLow) < ((ShadowShort_factor * (((ShadowShort_avgPeriod !== 0) ? (sp.ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowShort_rangeType === 2) ? 2.0 : 1.0))))) {
    sp.cur_outInteger = ((((inClose >= inOpen) ? 1 : -1) * 100) | 0);
  } else {
    sp.cur_outInteger = 0;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  sp.BodyPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType === 1) ? (inHigh - inLow) : ((BodyLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0))) - sp.ring_BodyTrailingIdx_derived[sp.ringPos_BodyTrailingIdx];
  sp.ShadowPeriodTotal += ((ShadowShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0))) - sp.ring_ShadowTrailingIdx_derived[sp.ringPos_ShadowTrailingIdx];
  ringPosL_BodyTrailingIdx = sp.ringPos_BodyTrailingIdx;
  sp.ring_BodyTrailingIdx_derived[ringPosL_BodyTrailingIdx] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType === 1) ? (inHigh - inLow) : ((BodyLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  ringPosL_BodyTrailingIdx = ringPosL_BodyTrailingIdx + 1;
  ringPosL_BodyTrailingIdx = ringPosL_BodyTrailingIdx & (ringPosL_BodyTrailingIdx - sp.ringCap_BodyTrailingIdx >> 31);
  sp.ringPos_BodyTrailingIdx = ringPosL_BodyTrailingIdx;
  ringPosL_ShadowTrailingIdx = sp.ringPos_ShadowTrailingIdx;
  sp.ring_ShadowTrailingIdx_derived[ringPosL_ShadowTrailingIdx] = ((ShadowShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  ringPosL_ShadowTrailingIdx = ringPosL_ShadowTrailingIdx + 1;
  ringPosL_ShadowTrailingIdx = ringPosL_ShadowTrailingIdx & (ringPosL_ShadowTrailingIdx - sp.ringCap_ShadowTrailingIdx >> 31);
  sp.ringPos_ShadowTrailingIdx = ringPosL_ShadowTrailingIdx;
}

function cdllonglineOpenImpl(
  sp: CdllonglineStream,
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
  let ShadowPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let BodyTrailingIdx = 0;
  let ShadowTrailingIdx = 0;
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
  const BodyLong_rangeType: number = core.candleSettings[CandleSettingType.BODY_LONG].rangeType;
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  const BodyLong_factor = core.candleSettings[CandleSettingType.BODY_LONG].factor;
  const ShadowShort_rangeType: number = core.candleSettings[CandleSettingType.SHADOW_SHORT].rangeType;
  const ShadowShort_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_SHORT].avgPeriod;
  const ShadowShort_factor = core.candleSettings[CandleSettingType.SHADOW_SHORT].factor;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdllonglineLookbackPositional(core);
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
  BodyTrailingIdx = startIdx - BodyLong_avgPeriod;
  ShadowPeriodTotal = 0;
  ShadowTrailingIdx = startIdx - ShadowShort_avgPeriod;
  i = BodyTrailingIdx;
  while (i < startIdx) {
    BodyPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = ShadowTrailingIdx;
  while (i < startIdx) {
    ShadowPeriodTotal += ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - long real body
   * - short upper and lower shadow
   * The meaning of "long" and "short" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (Math.abs(inClose[i] - inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) < ((ShadowShort_factor * (((ShadowShort_avgPeriod !== 0) ? (ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowShort_rangeType === 2) ? 2.0 : 1.0)))) && (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) < ((ShadowShort_factor * (((ShadowShort_avgPeriod !== 0) ? (ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowShort_rangeType === 2) ? 2.0 : 1.0))))) {
      outInteger[((outIdx++ * outStride) | 0)] = ((((inClose[i] >= inOpen[i]) ? 1 : -1) * 100) | 0);
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyTrailingIdx] - (((inClose[BodyTrailingIdx]) >= (inOpen[BodyTrailingIdx])) ? (inClose[BodyTrailingIdx]) : (inOpen[BodyTrailingIdx]))) + ((((inClose[BodyTrailingIdx]) >= (inOpen[BodyTrailingIdx])) ? (inOpen[BodyTrailingIdx]) : (inClose[BodyTrailingIdx])) - inLow[BodyTrailingIdx])) : 0.0)));
    ShadowPeriodTotal += ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[ShadowTrailingIdx] - inOpen[ShadowTrailingIdx])) : ((ShadowShort_rangeType === 1) ? (inHigh[ShadowTrailingIdx] - inLow[ShadowTrailingIdx]) : ((ShadowShort_rangeType === 2) ? ((inHigh[ShadowTrailingIdx] - (((inClose[ShadowTrailingIdx]) >= (inOpen[ShadowTrailingIdx])) ? (inClose[ShadowTrailingIdx]) : (inOpen[ShadowTrailingIdx]))) + ((((inClose[ShadowTrailingIdx]) >= (inOpen[ShadowTrailingIdx])) ? (inOpen[ShadowTrailingIdx]) : (inClose[ShadowTrailingIdx])) - inLow[ShadowTrailingIdx])) : 0.0)));
    i += 1;
    BodyTrailingIdx += 1;
    ShadowTrailingIdx += 1;
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
    capRing_BodyTrailingIdx_derived[fillJ - (historyLen - cap_BodyTrailingIdx)] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyLong_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyLong_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  const cap_ShadowTrailingIdx = i - ShadowTrailingIdx;
  if (cap_ShadowTrailingIdx < 0 || cap_ShadowTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_ShadowTrailingIdx = cap_ShadowTrailingIdx > 0 ? cap_ShadowTrailingIdx : 1;
  const capRing_ShadowTrailingIdx_derived = new Float64Array(allocN_ShadowTrailingIdx);
  for (let fillJ = historyLen - cap_ShadowTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_ShadowTrailingIdx_derived[fillJ - (historyLen - cap_ShadowTrailingIdx)] = ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((ShadowShort_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((ShadowShort_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  sp.BodyPeriodTotal = BodyPeriodTotal;
  sp.ShadowPeriodTotal = ShadowPeriodTotal;
  sp.ringPos_BodyTrailingIdx = 0;
  sp.ringCap_BodyTrailingIdx = cap_BodyTrailingIdx;
  sp.ring_BodyTrailingIdx_derived = capRing_BodyTrailingIdx_derived;
  sp.ringPos_ShadowTrailingIdx = 0;
  sp.ringCap_ShadowTrailingIdx = cap_ShadowTrailingIdx;
  sp.ring_ShadowTrailingIdx_derived = capRing_ShadowTrailingIdx_derived;
  sp.cs_BodyLong_rangeType = BodyLong_rangeType;
  sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
  sp.cs_BodyLong_factor = BodyLong_factor;
  sp.cs_ShadowShort_rangeType = ShadowShort_rangeType;
  sp.cs_ShadowShort_avgPeriod = ShadowShort_avgPeriod;
  sp.cs_ShadowShort_factor = ShadowShort_factor;
  sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

function cdllonglineOpenSinkFrame(
  sp: CdllonglineStream,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): RetCode {
  const core = sp.core;
  let BodyPeriodTotal = 0;
  let ShadowPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let BodyTrailingIdx = 0;
  let ShadowTrailingIdx = 0;
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
  const BodyLong_rangeType: number = core.candleSettings[CandleSettingType.BODY_LONG].rangeType;
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  const BodyLong_factor = core.candleSettings[CandleSettingType.BODY_LONG].factor;
  const ShadowShort_rangeType: number = core.candleSettings[CandleSettingType.SHADOW_SHORT].rangeType;
  const ShadowShort_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_SHORT].avgPeriod;
  const ShadowShort_factor = core.candleSettings[CandleSettingType.SHADOW_SHORT].factor;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdllonglineLookbackPositional(core);
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
  BodyTrailingIdx = startIdx - BodyLong_avgPeriod;
  ShadowPeriodTotal = 0;
  ShadowTrailingIdx = startIdx - ShadowShort_avgPeriod;
  i = BodyTrailingIdx;
  while (i < startIdx) {
    BodyPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = ShadowTrailingIdx;
  while (i < startIdx) {
    ShadowPeriodTotal += ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - long real body
   * - short upper and lower shadow
   * The meaning of "long" and "short" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  while (i < endIdx) {
    outIdx += 1;
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyTrailingIdx] - (((inClose[BodyTrailingIdx]) >= (inOpen[BodyTrailingIdx])) ? (inClose[BodyTrailingIdx]) : (inOpen[BodyTrailingIdx]))) + ((((inClose[BodyTrailingIdx]) >= (inOpen[BodyTrailingIdx])) ? (inOpen[BodyTrailingIdx]) : (inClose[BodyTrailingIdx])) - inLow[BodyTrailingIdx])) : 0.0)));
    ShadowPeriodTotal += ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[ShadowTrailingIdx] - inOpen[ShadowTrailingIdx])) : ((ShadowShort_rangeType === 1) ? (inHigh[ShadowTrailingIdx] - inLow[ShadowTrailingIdx]) : ((ShadowShort_rangeType === 2) ? ((inHigh[ShadowTrailingIdx] - (((inClose[ShadowTrailingIdx]) >= (inOpen[ShadowTrailingIdx])) ? (inClose[ShadowTrailingIdx]) : (inOpen[ShadowTrailingIdx]))) + ((((inClose[ShadowTrailingIdx]) >= (inOpen[ShadowTrailingIdx])) ? (inOpen[ShadowTrailingIdx]) : (inClose[ShadowTrailingIdx])) - inLow[ShadowTrailingIdx])) : 0.0)));
    i += 1;
    BodyTrailingIdx += 1;
    ShadowTrailingIdx += 1;
  }
  if (Math.abs(inClose[i] - inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) < ((ShadowShort_factor * (((ShadowShort_avgPeriod !== 0) ? (ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowShort_rangeType === 2) ? 2.0 : 1.0)))) && (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) < ((ShadowShort_factor * (((ShadowShort_avgPeriod !== 0) ? (ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowShort_rangeType === 2) ? 2.0 : 1.0))))) {
    outInteger[0] = ((((inClose[i] >= inOpen[i]) ? 1 : -1) * 100) | 0);
    outIdx += 1;
  } else {
    outInteger[0] = 0;
    outIdx += 1;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  BodyPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyTrailingIdx] - (((inClose[BodyTrailingIdx]) >= (inOpen[BodyTrailingIdx])) ? (inClose[BodyTrailingIdx]) : (inOpen[BodyTrailingIdx]))) + ((((inClose[BodyTrailingIdx]) >= (inOpen[BodyTrailingIdx])) ? (inOpen[BodyTrailingIdx]) : (inClose[BodyTrailingIdx])) - inLow[BodyTrailingIdx])) : 0.0)));
  ShadowPeriodTotal += ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[ShadowTrailingIdx] - inOpen[ShadowTrailingIdx])) : ((ShadowShort_rangeType === 1) ? (inHigh[ShadowTrailingIdx] - inLow[ShadowTrailingIdx]) : ((ShadowShort_rangeType === 2) ? ((inHigh[ShadowTrailingIdx] - (((inClose[ShadowTrailingIdx]) >= (inOpen[ShadowTrailingIdx])) ? (inClose[ShadowTrailingIdx]) : (inOpen[ShadowTrailingIdx]))) + ((((inClose[ShadowTrailingIdx]) >= (inOpen[ShadowTrailingIdx])) ? (inOpen[ShadowTrailingIdx]) : (inClose[ShadowTrailingIdx])) - inLow[ShadowTrailingIdx])) : 0.0)));
  i += 1;
  BodyTrailingIdx += 1;
  ShadowTrailingIdx += 1;
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
    capRing_BodyTrailingIdx_derived[fillJ - (historyLen - cap_BodyTrailingIdx)] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyLong_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyLong_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  const cap_ShadowTrailingIdx = i - ShadowTrailingIdx;
  if (cap_ShadowTrailingIdx < 0 || cap_ShadowTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_ShadowTrailingIdx = cap_ShadowTrailingIdx > 0 ? cap_ShadowTrailingIdx : 1;
  const capRing_ShadowTrailingIdx_derived = new Float64Array(allocN_ShadowTrailingIdx);
  for (let fillJ = historyLen - cap_ShadowTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_ShadowTrailingIdx_derived[fillJ - (historyLen - cap_ShadowTrailingIdx)] = ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((ShadowShort_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((ShadowShort_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  sp.BodyPeriodTotal = BodyPeriodTotal;
  sp.ShadowPeriodTotal = ShadowPeriodTotal;
  sp.ringPos_BodyTrailingIdx = 0;
  sp.ringCap_BodyTrailingIdx = cap_BodyTrailingIdx;
  sp.ring_BodyTrailingIdx_derived = capRing_BodyTrailingIdx_derived;
  sp.ringPos_ShadowTrailingIdx = 0;
  sp.ringCap_ShadowTrailingIdx = cap_ShadowTrailingIdx;
  sp.ring_ShadowTrailingIdx_derived = capRing_ShadowTrailingIdx_derived;
  sp.cs_BodyLong_rangeType = BodyLong_rangeType;
  sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
  sp.cs_BodyLong_factor = BodyLong_factor;
  sp.cs_ShadowShort_rangeType = ShadowShort_rangeType;
  sp.cs_ShadowShort_avgPeriod = ShadowShort_avgPeriod;
  sp.cs_ShadowShort_factor = ShadowShort_factor;
  sp.cur_outInteger = outInteger[0];
  return RetCode.SUCCESS;
}

/** @internal */
export function cdllonglineOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): CdllonglineStream {
  const sp = CdllonglineStream.of(core);
  const retCode = cdllonglineOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLLONGLINE openAndFill', inOpen.length, startIdx, cdllonglineLookbackPositional(core));
  }
  throw streamFailure('CDLLONGLINE openAndFill', retCode);
}

/** @internal */
export function cdllonglineOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): CdllonglineStream {
  const sp = CdllonglineStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdllonglineOpenSinkFrame(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLLONGLINE open', inOpen.length, startIdx, cdllonglineLookbackPositional(core));
  }
  throw streamFailure('CDLLONGLINE open', retCode);
}

/** @internal */
export function cdllonglineOpenPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): CdllonglineStream {
  requireArgument('CDLLONGLINE open', 'prices.open', inOpen);
  requireHistory('CDLLONGLINE open', 'prices.open', inOpen.length);
  requireArgument('CDLLONGLINE open', 'prices.high', inHigh);
  requireArgument('CDLLONGLINE open', 'prices.low', inLow);
  requireArgument('CDLLONGLINE open', 'prices.close', inClose);
  requireHistoryLength('CDLLONGLINE open', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLLONGLINE open', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLLONGLINE open', 'prices.close', inClose.length, inOpen.length);
  return cdllonglineOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/** @internal */
export function cdllonglineOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): CdllonglineStream {
  requireArgument('CDLLONGLINE openAndFill', 'prices.open', inOpen);
  requireHistory('CDLLONGLINE openAndFill', 'prices.open', inOpen.length);
  requireArgument('CDLLONGLINE openAndFill', 'prices.high', inHigh);
  requireArgument('CDLLONGLINE openAndFill', 'prices.low', inLow);
  requireArgument('CDLLONGLINE openAndFill', 'prices.close', inClose);
  const guardOutLen = openFillCount('CDLLONGLINE openAndFill', inOpen.length, cdllonglineLookbackPositional(core));
  requireHistoryLength('CDLLONGLINE openAndFill', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLLONGLINE openAndFill', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLLONGLINE openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('CDLLONGLINE openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('CDLLONGLINE openAndFill', 'outInteger', 'prices.open');
  }
  if (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CDLLONGLINE openAndFill', 'outInteger', 'prices.high');
  }
  if (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CDLLONGLINE openAndFill', 'outInteger', 'prices.low');
  }
  if (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('CDLLONGLINE openAndFill', 'outInteger', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLLONGLINE openAndFill', 'outInteger', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLLONGLINE openAndFill', 'outInteger', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLLONGLINE openAndFill', 'outInteger', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLLONGLINE openAndFill', 'outInteger', 'prices.close');
  }
  return cdllonglineOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}

/**
 * Opens a live CDLLONGLINE stream over the warm-up history: `value()` starts at
 * the last history bar, bit-identical to {@link cdllongline} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdllonglineOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdllonglineLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input or histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdllonglineOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdllonglineOpen({ open, high, low, close });
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdllonglineOpen(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdllonglineOptions): CdllonglineStream {
  requireArgument('CDLLONGLINE open', 'prices', prices);
  const o = optionsOf('CDLLONGLINE open', options);
  const core = coreOf('CDLLONGLINE open', o.core);
  return cdllonglineOpenPositional(prices.open, prices.high, prices.low, prices.close, core);
}

/**
 * {@link cdllonglineOpen} that also computes the outputs over the whole history in
 * the same pass, bit-identically to {@link cdllongline}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CdllonglineOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cdllongline} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdllonglineLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, histories of different lengths or a buffer too short,
 *   of the wrong type or starting where another buffer or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdllonglineOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = cdllonglineOpenAndFill({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cdllonglineOpenAndFill(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdllonglineOptions): CdllonglineResult & { stream: CdllonglineStream } {
  requireArgument('CDLLONGLINE openAndFill', 'prices', prices);
  const o = optionsOf('CDLLONGLINE openAndFill', options);
  const core = coreOf('CDLLONGLINE openAndFill', o.core);
  const capacity = fillCapacity(cdllonglineLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const stream = cdllonglineOpenAndFillPositional(prices.open, prices.high, prices.low, prices.close, outInteger, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}
