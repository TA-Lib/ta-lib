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
 *  072404 AC     Creation
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { CandleSettingType, Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function cdlhighwaveLookbackPositional(core: Core = Core.DEFAULT): number {
  const BodyShort_avgPeriod = core.candleSettings[CandleSettingType.BODY_SHORT].avgPeriod;
  const ShadowVeryLong_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_VERY_LONG].avgPeriod;
  return (BodyShort_avgPeriod > ShadowVeryLong_avgPeriod ? BodyShort_avgPeriod : ShadowVeryLong_avgPeriod);
}

/** @internal */
export function cdlhighwaveDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdlhighwaveImpl(
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
  const BodyShort_rangeType: number = core.candleSettings[CandleSettingType.BODY_SHORT].rangeType;
  const BodyShort_avgPeriod = core.candleSettings[CandleSettingType.BODY_SHORT].avgPeriod;
  const BodyShort_factor = core.candleSettings[CandleSettingType.BODY_SHORT].factor;
  const ShadowVeryLong_rangeType: number = core.candleSettings[CandleSettingType.SHADOW_VERY_LONG].rangeType;
  const ShadowVeryLong_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_VERY_LONG].avgPeriod;
  const ShadowVeryLong_factor = core.candleSettings[CandleSettingType.SHADOW_VERY_LONG].factor;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlhighwaveLookbackPositional(core);
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
  ShadowPeriodTotal = 0;
  ShadowTrailingIdx = startIdx - ShadowVeryLong_avgPeriod;
  i = BodyTrailingIdx;
  while (i < startIdx) {
    BodyPeriodTotal += ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = ShadowTrailingIdx;
  while (i < startIdx) {
    ShadowPeriodTotal += ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - short real body
   * - very long upper and lower shadow
   * The meaning of "short" and "very long" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (Math.abs(inClose[i] - inOpen[i]) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (BodyPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) && (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod !== 0) ? (ShadowPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowVeryLong_rangeType === 2) ? 2.0 : 1.0)))) && (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod !== 0) ? (ShadowPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowVeryLong_rangeType === 2) ? 2.0 : 1.0))))) {
      outInteger[outIdx++] = ((((inClose[i] >= inOpen[i]) ? 1 : -1) * 100) | 0);
    } else {
      outInteger[outIdx++] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyPeriodTotal += ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyShort_rangeType === 0) ? (Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : ((BodyShort_rangeType === 1) ? (inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) : ((BodyShort_rangeType === 2) ? ((inHigh[BodyTrailingIdx] - (((inClose[BodyTrailingIdx]) >= (inOpen[BodyTrailingIdx])) ? (inClose[BodyTrailingIdx]) : (inOpen[BodyTrailingIdx]))) + ((((inClose[BodyTrailingIdx]) >= (inOpen[BodyTrailingIdx])) ? (inOpen[BodyTrailingIdx]) : (inClose[BodyTrailingIdx])) - inLow[BodyTrailingIdx])) : 0.0)));
    ShadowPeriodTotal += ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose[ShadowTrailingIdx] - inOpen[ShadowTrailingIdx])) : ((ShadowVeryLong_rangeType === 1) ? (inHigh[ShadowTrailingIdx] - inLow[ShadowTrailingIdx]) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh[ShadowTrailingIdx] - (((inClose[ShadowTrailingIdx]) >= (inOpen[ShadowTrailingIdx])) ? (inClose[ShadowTrailingIdx]) : (inOpen[ShadowTrailingIdx]))) + ((((inClose[ShadowTrailingIdx]) >= (inOpen[ShadowTrailingIdx])) ? (inOpen[ShadowTrailingIdx]) : (inClose[ShadowTrailingIdx])) - inLow[ShadowTrailingIdx])) : 0.0)));
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
export function cdlhighwavePositional(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDLHIGHWAVE', 'startIdx', startIdx);
  endIdx = requireInt32('CDLHIGHWAVE', 'endIdx', endIdx);
  requireIndexRange('CDLHIGHWAVE', startIdx, endIdx);
  const guardStart = clampedStart('CDLHIGHWAVE', startIdx, cdlhighwaveLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLHIGHWAVE', 'prices.open', inOpen, guardInLen);
  requireLength('CDLHIGHWAVE', 'prices.high', inHigh, guardInLen);
  requireLength('CDLHIGHWAVE', 'prices.low', inLow, guardInLen);
  requireLength('CDLHIGHWAVE', 'prices.close', inClose, guardInLen);
  requireOutput('CDLHIGHWAVE', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLHIGHWAVE', 'outInteger', 'prices.open');
  }
  if (overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLHIGHWAVE', 'outInteger', 'prices.high');
  }
  if (overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLHIGHWAVE', 'outInteger', 'prices.low');
  }
  if (overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLHIGHWAVE', 'outInteger', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdlhighwaveImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLHIGHWAVE', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cdlhighwave}, {@link cdlhighwaveLookback},
 * {@link cdlhighwaveDisplayShift}, {@link cdlhighwaveOpen} and
 * {@link cdlhighwaveOpenAndFill}. Every key is optional: an omitted or `undefined`
 * key takes its default, and a call ignores the keys it has no use for.
 */
export interface CdlhighwaveOptions {
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
 * What {@link cdlhighwave} returns, and {@link cdlhighwaveOpenAndFill} with its
 * stream.
 */
export interface CdlhighwaveResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * On a hit, +100 when the candle is white (close >= open) or -100 when black
   * (close \< open); 0 otherwise. Sign denotes color, NOT bull/bear. `values[k]` is
   * the value at input bar `begIdx + k`.
   */
  values: Int32Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link cdlhighwave} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlhighwaveOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlhighwaveLookback(options?: CdlhighwaveOptions): number {
  const o = optionsOf('CDLHIGHWAVE lookback', options);
  return cdlhighwaveLookbackPositional(coreOf('CDLHIGHWAVE lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdlhighwave}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlhighwaveOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlhighwaveDisplayShift(outputIdx: number, options?: CdlhighwaveOptions): number {
  const o = optionsOf('CDLHIGHWAVE displayShift', options);
  return cdlhighwaveDisplayShiftPositional(outputIdx, coreOf('CDLHIGHWAVE displayShift', o.core));
}

/**
 * Single-candle pattern: a short real body with both a very long upper and a very
 * long lower shadow. Signals market indecision; the output sign reports only
 * candle color, not a bullish/bearish direction. A hit marks indecision
 * (long-legged candle); not directional - sign encodes only the candle's color.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlhighwave](https://ta-lib.org/functions/cdlhighwave).
 *
 * **Notes**
 * - Bulkowski's testing found the High-Wave candle acts as a reversal only 51% of
 *   the time — statistically indistinguishable from random — which he notes
 *   actually agrees with the pattern's theoretical meaning of pure indecision.
 *   ([thepatternsite.com](https://thepatternsite.com/HighWave.html))
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cdlhighwaveLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `open`: Open price of
 *   each bar. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link CdlhighwaveOptions}.
 * @returns `begIdx` and the values; see {@link CdlhighwaveResult}. Each output is
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
 * import { cdlhighwave } from 'ta-lib';
 *
 * const open = new Float64Array(252); // your data, oldest first
 * const high = new Float64Array(252); // your data, oldest first
 * const low = new Float64Array(252); // your data, oldest first
 * const close = new Float64Array(252); // your data, oldest first
 *
 * const { begIdx, values } = cdlhighwave({ open, high, low, close });
 * console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
 * ```
 * @see [`cdllongleggeddoji`](https://ta-lib.org/functions/cdllongleggeddoji)
 * @see [`cdlspinningtop`](https://ta-lib.org/functions/cdlspinningtop)
 * @see [`cdlrickshawman`](https://ta-lib.org/functions/cdlrickshawman)
 * @see [`cdldoji`](https://ta-lib.org/functions/cdldoji)
 */
export function cdlhighwave(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlhighwaveOptions): CdlhighwaveResult {
  requireArgument('CDLHIGHWAVE', 'prices', prices);
  const o = optionsOf('CDLHIGHWAVE', options);
  if (o.endIdx === undefined) {
    requireSeries('CDLHIGHWAVE', 'prices.open', prices.open);
    requireSeries('CDLHIGHWAVE', 'prices.high', prices.high);
    requireSeries('CDLHIGHWAVE', 'prices.low', prices.low);
    requireSeries('CDLHIGHWAVE', 'prices.close', prices.close);
  }
  const core = coreOf('CDLHIGHWAVE', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cdlhighwaveLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const range = cdlhighwavePositional(startIdx, endIdx, prices.open, prices.high, prices.low, prices.close, outInteger, core);
  return { begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CDLHIGHWAVE stream: one value per closed bar, bit-identical to
 * {@link cdlhighwave} over the same series. Open it with {@link cdlhighwaveOpen}
 * or {@link cdlhighwaveOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlhighwave](https://ta-lib.org/functions/cdlhighwave).
 */
export class CdlhighwaveStream {
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
  cs_BodyShort_rangeType: number = 0;
  /** @internal */
  cs_BodyShort_avgPeriod: number = 0;
  /** @internal */
  cs_BodyShort_factor: number = 0;
  /** @internal */
  cs_ShadowVeryLong_rangeType: number = 0;
  /** @internal */
  cs_ShadowVeryLong_avgPeriod: number = 0;
  /** @internal */
  cs_ShadowVeryLong_factor: number = 0;
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
  static of(core: Core): CdlhighwaveStream {
    return new CdlhighwaveStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cdlhighwave} reports them over the same bars. Each accepted `update` and
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
      throw streamFailure('CDLHIGHWAVE advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw streamFailure('CDLHIGHWAVE update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(open) || !Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('CDLHIGHWAVE update', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    cdlhighwaveStepImpl(this, open, high, low, close);
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
      throw nonFiniteBar('CDLHIGHWAVE peek', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
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
    const ShadowVeryLong_rangeType = sp.cs_ShadowVeryLong_rangeType;
    const ShadowVeryLong_avgPeriod = sp.cs_ShadowVeryLong_avgPeriod;
    const ShadowVeryLong_factor = sp.cs_ShadowVeryLong_factor;
    if (Math.abs(inClose - inOpen) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (sp.BodyPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) && (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod !== 0) ? (sp.ShadowPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowVeryLong_rangeType === 2) ? 2.0 : 1.0)))) && (((inClose >= inOpen) ? inOpen : inClose) - inLow) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod !== 0) ? (sp.ShadowPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowVeryLong_rangeType === 2) ? 2.0 : 1.0))))) {
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
  clone(): CdlhighwaveStream {
    const c = new CdlhighwaveStream(this.core);
    c.BodyPeriodTotal = this.BodyPeriodTotal;
    c.ShadowPeriodTotal = this.ShadowPeriodTotal;
    c.ringPos_BodyTrailingIdx = this.ringPos_BodyTrailingIdx;
    c.ringCap_BodyTrailingIdx = this.ringCap_BodyTrailingIdx;
    c.ring_BodyTrailingIdx_derived = this.ring_BodyTrailingIdx_derived.slice();
    c.ringPos_ShadowTrailingIdx = this.ringPos_ShadowTrailingIdx;
    c.ringCap_ShadowTrailingIdx = this.ringCap_ShadowTrailingIdx;
    c.ring_ShadowTrailingIdx_derived = this.ring_ShadowTrailingIdx_derived.slice();
    c.cs_BodyShort_rangeType = this.cs_BodyShort_rangeType;
    c.cs_BodyShort_avgPeriod = this.cs_BodyShort_avgPeriod;
    c.cs_BodyShort_factor = this.cs_BodyShort_factor;
    c.cs_ShadowVeryLong_rangeType = this.cs_ShadowVeryLong_rangeType;
    c.cs_ShadowVeryLong_avgPeriod = this.cs_ShadowVeryLong_avgPeriod;
    c.cs_ShadowVeryLong_factor = this.cs_ShadowVeryLong_factor;
    c.cur_outInteger = this.cur_outInteger;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cdlhighwaveStepImpl(sp: CdlhighwaveStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  const BodyShort_rangeType = sp.cs_BodyShort_rangeType;
  const BodyShort_avgPeriod = sp.cs_BodyShort_avgPeriod;
  const BodyShort_factor = sp.cs_BodyShort_factor;
  const ShadowVeryLong_rangeType = sp.cs_ShadowVeryLong_rangeType;
  const ShadowVeryLong_avgPeriod = sp.cs_ShadowVeryLong_avgPeriod;
  const ShadowVeryLong_factor = sp.cs_ShadowVeryLong_factor;
  if (sp.ringCap_BodyTrailingIdx === 0) {
    sp.ring_BodyTrailingIdx_derived[0] = ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
  if (sp.ringCap_ShadowTrailingIdx === 0) {
    sp.ring_ShadowTrailingIdx_derived[0] = ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
  if (Math.abs(inClose - inOpen) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (sp.BodyPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) && (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod !== 0) ? (sp.ShadowPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowVeryLong_rangeType === 2) ? 2.0 : 1.0)))) && (((inClose >= inOpen) ? inOpen : inClose) - inLow) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod !== 0) ? (sp.ShadowPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowVeryLong_rangeType === 2) ? 2.0 : 1.0))))) {
    sp.cur_outInteger = ((((inClose >= inOpen) ? 1 : -1) * 100) | 0);
  } else {
    sp.cur_outInteger = 0;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  sp.BodyPeriodTotal += ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0))) - sp.ring_BodyTrailingIdx_derived[sp.ringPos_BodyTrailingIdx];
  sp.ShadowPeriodTotal += ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0))) - sp.ring_ShadowTrailingIdx_derived[sp.ringPos_ShadowTrailingIdx];
  sp.ring_BodyTrailingIdx_derived[sp.ringPos_BodyTrailingIdx] = ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  sp.ringPos_BodyTrailingIdx = sp.ringPos_BodyTrailingIdx + 1;
  if (sp.ringPos_BodyTrailingIdx >= sp.ringCap_BodyTrailingIdx) {
    sp.ringPos_BodyTrailingIdx = 0;
  }
  sp.ring_ShadowTrailingIdx_derived[sp.ringPos_ShadowTrailingIdx] = ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  sp.ringPos_ShadowTrailingIdx = sp.ringPos_ShadowTrailingIdx + 1;
  if (sp.ringPos_ShadowTrailingIdx >= sp.ringCap_ShadowTrailingIdx) {
    sp.ringPos_ShadowTrailingIdx = 0;
  }
}

function cdlhighwaveOpenImpl(
  sp: CdlhighwaveStream,
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
  const BodyShort_rangeType: number = core.candleSettings[CandleSettingType.BODY_SHORT].rangeType;
  const BodyShort_avgPeriod = core.candleSettings[CandleSettingType.BODY_SHORT].avgPeriod;
  const BodyShort_factor = core.candleSettings[CandleSettingType.BODY_SHORT].factor;
  const ShadowVeryLong_rangeType: number = core.candleSettings[CandleSettingType.SHADOW_VERY_LONG].rangeType;
  const ShadowVeryLong_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_VERY_LONG].avgPeriod;
  const ShadowVeryLong_factor = core.candleSettings[CandleSettingType.SHADOW_VERY_LONG].factor;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlhighwaveLookbackPositional(core);
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
  ShadowPeriodTotal = 0;
  ShadowTrailingIdx = startIdx - ShadowVeryLong_avgPeriod;
  i = BodyTrailingIdx;
  while (i < startIdx) {
    BodyPeriodTotal += ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = ShadowTrailingIdx;
  while (i < startIdx) {
    ShadowPeriodTotal += ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - short real body
   * - very long upper and lower shadow
   * The meaning of "short" and "very long" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (Math.abs(inClose[i] - inOpen[i]) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (BodyPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) && (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod !== 0) ? (ShadowPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowVeryLong_rangeType === 2) ? 2.0 : 1.0)))) && (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod !== 0) ? (ShadowPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowVeryLong_rangeType === 2) ? 2.0 : 1.0))))) {
      outInteger[((outIdx++ * outStride) | 0)] = ((((inClose[i] >= inOpen[i]) ? 1 : -1) * 100) | 0);
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyPeriodTotal += ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyShort_rangeType === 0) ? (Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : ((BodyShort_rangeType === 1) ? (inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) : ((BodyShort_rangeType === 2) ? ((inHigh[BodyTrailingIdx] - (((inClose[BodyTrailingIdx]) >= (inOpen[BodyTrailingIdx])) ? (inClose[BodyTrailingIdx]) : (inOpen[BodyTrailingIdx]))) + ((((inClose[BodyTrailingIdx]) >= (inOpen[BodyTrailingIdx])) ? (inOpen[BodyTrailingIdx]) : (inClose[BodyTrailingIdx])) - inLow[BodyTrailingIdx])) : 0.0)));
    ShadowPeriodTotal += ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose[ShadowTrailingIdx] - inOpen[ShadowTrailingIdx])) : ((ShadowVeryLong_rangeType === 1) ? (inHigh[ShadowTrailingIdx] - inLow[ShadowTrailingIdx]) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh[ShadowTrailingIdx] - (((inClose[ShadowTrailingIdx]) >= (inOpen[ShadowTrailingIdx])) ? (inClose[ShadowTrailingIdx]) : (inOpen[ShadowTrailingIdx]))) + ((((inClose[ShadowTrailingIdx]) >= (inOpen[ShadowTrailingIdx])) ? (inOpen[ShadowTrailingIdx]) : (inClose[ShadowTrailingIdx])) - inLow[ShadowTrailingIdx])) : 0.0)));
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
    capRing_BodyTrailingIdx_derived[fillJ - (historyLen - cap_BodyTrailingIdx)] = ((BodyShort_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyShort_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyShort_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  const cap_ShadowTrailingIdx = i - ShadowTrailingIdx;
  if (cap_ShadowTrailingIdx < 0 || cap_ShadowTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_ShadowTrailingIdx = cap_ShadowTrailingIdx > 0 ? cap_ShadowTrailingIdx : 1;
  const capRing_ShadowTrailingIdx_derived = new Float64Array(allocN_ShadowTrailingIdx);
  for (let fillJ = historyLen - cap_ShadowTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_ShadowTrailingIdx_derived[fillJ - (historyLen - cap_ShadowTrailingIdx)] = ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((ShadowVeryLong_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  sp.BodyPeriodTotal = BodyPeriodTotal;
  sp.ShadowPeriodTotal = ShadowPeriodTotal;
  sp.ringPos_BodyTrailingIdx = 0;
  sp.ringCap_BodyTrailingIdx = cap_BodyTrailingIdx;
  sp.ring_BodyTrailingIdx_derived = capRing_BodyTrailingIdx_derived;
  sp.ringPos_ShadowTrailingIdx = 0;
  sp.ringCap_ShadowTrailingIdx = cap_ShadowTrailingIdx;
  sp.ring_ShadowTrailingIdx_derived = capRing_ShadowTrailingIdx_derived;
  sp.cs_BodyShort_rangeType = BodyShort_rangeType;
  sp.cs_BodyShort_avgPeriod = BodyShort_avgPeriod;
  sp.cs_BodyShort_factor = BodyShort_factor;
  sp.cs_ShadowVeryLong_rangeType = ShadowVeryLong_rangeType;
  sp.cs_ShadowVeryLong_avgPeriod = ShadowVeryLong_avgPeriod;
  sp.cs_ShadowVeryLong_factor = ShadowVeryLong_factor;
  sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlhighwaveOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): CdlhighwaveStream {
  const sp = CdlhighwaveStream.of(core);
  const retCode = cdlhighwaveOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLHIGHWAVE openAndFill', inOpen.length, startIdx, cdlhighwaveLookbackPositional(core));
  }
  throw streamFailure('CDLHIGHWAVE openAndFill', retCode);
}

/** @internal */
export function cdlhighwaveOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): CdlhighwaveStream {
  const sp = CdlhighwaveStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdlhighwaveOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLHIGHWAVE open', inOpen.length, startIdx, cdlhighwaveLookbackPositional(core));
  }
  throw streamFailure('CDLHIGHWAVE open', retCode);
}

/** @internal */
export function cdlhighwaveOpenPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): CdlhighwaveStream {
  requireArgument('CDLHIGHWAVE open', 'prices.open', inOpen);
  requireHistory('CDLHIGHWAVE open', 'prices.open', inOpen.length);
  requireArgument('CDLHIGHWAVE open', 'prices.high', inHigh);
  requireArgument('CDLHIGHWAVE open', 'prices.low', inLow);
  requireArgument('CDLHIGHWAVE open', 'prices.close', inClose);
  requireHistoryLength('CDLHIGHWAVE open', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLHIGHWAVE open', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLHIGHWAVE open', 'prices.close', inClose.length, inOpen.length);
  return cdlhighwaveOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/** @internal */
export function cdlhighwaveOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): CdlhighwaveStream {
  requireArgument('CDLHIGHWAVE openAndFill', 'prices.open', inOpen);
  requireHistory('CDLHIGHWAVE openAndFill', 'prices.open', inOpen.length);
  requireArgument('CDLHIGHWAVE openAndFill', 'prices.high', inHigh);
  requireArgument('CDLHIGHWAVE openAndFill', 'prices.low', inLow);
  requireArgument('CDLHIGHWAVE openAndFill', 'prices.close', inClose);
  const guardOutLen = openFillCount('CDLHIGHWAVE openAndFill', inOpen.length, cdlhighwaveLookbackPositional(core));
  requireHistoryLength('CDLHIGHWAVE openAndFill', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLHIGHWAVE openAndFill', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLHIGHWAVE openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('CDLHIGHWAVE openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('CDLHIGHWAVE openAndFill', 'outInteger', 'prices.open');
  }
  if (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CDLHIGHWAVE openAndFill', 'outInteger', 'prices.high');
  }
  if (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CDLHIGHWAVE openAndFill', 'outInteger', 'prices.low');
  }
  if (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('CDLHIGHWAVE openAndFill', 'outInteger', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLHIGHWAVE openAndFill', 'outInteger', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLHIGHWAVE openAndFill', 'outInteger', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLHIGHWAVE openAndFill', 'outInteger', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLHIGHWAVE openAndFill', 'outInteger', 'prices.close');
  }
  return cdlhighwaveOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}

/**
 * Opens a live CDLHIGHWAVE stream over the warm-up history: `value()` starts at
 * the last history bar, bit-identical to {@link cdlhighwave} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlhighwaveOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlhighwaveLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input or histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdlhighwaveOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdlhighwaveOpen({ open, high, low, close });
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdlhighwaveOpen(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlhighwaveOptions): CdlhighwaveStream {
  requireArgument('CDLHIGHWAVE open', 'prices', prices);
  const o = optionsOf('CDLHIGHWAVE open', options);
  const core = coreOf('CDLHIGHWAVE open', o.core);
  return cdlhighwaveOpenPositional(prices.open, prices.high, prices.low, prices.close, core);
}

/**
 * {@link cdlhighwaveOpen} that also computes the outputs over the whole history in
 * the same pass, bit-identically to {@link cdlhighwave}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CdlhighwaveOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cdlhighwave} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlhighwaveLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, histories of different lengths or a buffer too short,
 *   of the wrong type or starting where another buffer or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdlhighwaveOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = cdlhighwaveOpenAndFill({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cdlhighwaveOpenAndFill(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlhighwaveOptions): CdlhighwaveResult & { stream: CdlhighwaveStream } {
  requireArgument('CDLHIGHWAVE openAndFill', 'prices', prices);
  const o = optionsOf('CDLHIGHWAVE openAndFill', options);
  const core = coreOf('CDLHIGHWAVE openAndFill', o.core);
  const capacity = fillCapacity(cdlhighwaveLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const stream = cdlhighwaveOpenAndFillPositional(prices.open, prices.high, prices.low, prices.close, outInteger, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}
