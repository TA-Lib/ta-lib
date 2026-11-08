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
 *  120904 AC     Creation
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 *  103126 MF,CC  Pre-check the cheap tests in an outer if, the candle averages inside
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireReal, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { CandleSettingType, Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function cdldarkcloudcoverLookbackPositional(optInPenetration: number, core: Core = Core.DEFAULT): number {
  if (typeof optInPenetration !== 'number') {
    return -1;
  }
  if (optInPenetration === Core.REAL_DEFAULT) {
    optInPenetration = 0.5;
  } else if (!(optInPenetration >= 0e0 && optInPenetration <= Core.REAL_MAX)) {
    return -1;
  }
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  return BodyLong_avgPeriod + 1;
}

/** @internal */
export function cdldarkcloudcoverDisplayShiftPositional(optInPenetration: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (cdldarkcloudcoverLookbackPositional(optInPenetration, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdldarkcloudcoverImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  optInPenetration: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): RetCode {
  let BodyLongPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let BodyLongTrailingIdx = 0;
  let lookbackTotal = 0;
  const BodyLong_rangeType: number = core.candleSettings[CandleSettingType.BODY_LONG].rangeType;
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  const BodyLong_factor = core.candleSettings[CandleSettingType.BODY_LONG].factor;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInPenetration === Core.REAL_DEFAULT) {
    optInPenetration = 0.5;
  } else if (!(optInPenetration >= 0e0 && optInPenetration <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdldarkcloudcoverLookbackPositional(optInPenetration, core);
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
  BodyLongPeriodTotal = 0;
  BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
  i = BodyLongTrailingIdx;
  while (i < startIdx) {
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first candle: long white candle
   * - second candle: black candle that opens above previous day high and closes within previous day real body;
   * Greg Morris wants the close to be below the midpoint of the previous real body
   * The meaning of "long" is specified with TA_SetCandleSettings, the penetration of the first real body is specified
   * with optInPenetration
   */
  outIdx = 0;
  do {
    if (((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === 1 && /* 1st: white */
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === -1 &&        /* 2nd: black */
        inOpen[i] > inHigh[i - 1] &&                          /* open above prior high */
        Math.abs(inClose[i - 1] - inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* long */
        inClose[i] > inOpen[i - 1] &&                         /* close within prior body */
        inClose[i] < inClose[i - 1] - Math.abs(inClose[i - 1] - inOpen[i - 1]) * optInPenetration)
    {
      outInteger[outIdx++] = -100;
    } else {
      outInteger[outIdx++] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 1] - inOpen[BodyLongTrailingIdx - 1])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyLongTrailingIdx - 1] - inLow[BodyLongTrailingIdx - 1]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyLongTrailingIdx - 1] - (((inClose[BodyLongTrailingIdx - 1]) >= (inOpen[BodyLongTrailingIdx - 1])) ? (inClose[BodyLongTrailingIdx - 1]) : (inOpen[BodyLongTrailingIdx - 1]))) + ((((inClose[BodyLongTrailingIdx - 1]) >= (inOpen[BodyLongTrailingIdx - 1])) ? (inOpen[BodyLongTrailingIdx - 1]) : (inClose[BodyLongTrailingIdx - 1])) - inLow[BodyLongTrailingIdx - 1])) : 0.0)));
    i += 1;
    BodyLongTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['penetration', Core.REAL_DEFAULT, 0e0, Core.REAL_MAX, 'in 0..3e37']] as const;

/** @internal */
export function cdldarkcloudcoverPositional(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInPenetration: number,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDLDARKCLOUDCOVER', 'startIdx', startIdx);
  endIdx = requireInt32('CDLDARKCLOUDCOVER', 'endIdx', endIdx);
  optInPenetration = requireReal('CDLDARKCLOUDCOVER', 'penetration', optInPenetration);
  requireIndexRange('CDLDARKCLOUDCOVER', startIdx, endIdx);
  const guardLookback = cdldarkcloudcoverLookbackPositional(optInPenetration, core);
  if (guardLookback < 0) {
    throw rejectedParams('CDLDARKCLOUDCOVER', PARAM_RANGES, optInPenetration);
  }
  const guardStart = clampedStart('CDLDARKCLOUDCOVER', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLDARKCLOUDCOVER', 'prices.open', inOpen, guardInLen);
  requireLength('CDLDARKCLOUDCOVER', 'prices.high', inHigh, guardInLen);
  requireLength('CDLDARKCLOUDCOVER', 'prices.low', inLow, guardInLen);
  requireLength('CDLDARKCLOUDCOVER', 'prices.close', inClose, guardInLen);
  requireOutput('CDLDARKCLOUDCOVER', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLDARKCLOUDCOVER', 'outInteger', 'prices.open');
  }
  if (overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLDARKCLOUDCOVER', 'outInteger', 'prices.high');
  }
  if (overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLDARKCLOUDCOVER', 'outInteger', 'prices.low');
  }
  if (overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLDARKCLOUDCOVER', 'outInteger', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdldarkcloudcoverImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, optInPenetration, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLDARKCLOUDCOVER', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cdldarkcloudcover}, {@link cdldarkcloudcoverLookback},
 * {@link cdldarkcloudcoverDisplayShift}, {@link cdldarkcloudcoverOpen} and
 * {@link cdldarkcloudcoverOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface CdldarkcloudcoverOptions {
  /**
   * Fraction of candle 1's real body that candle 2's close must penetrate below
   * close\[i-1]; larger values require deeper penetration (default 0.5; minimum 0;
   * `Core.REAL_DEFAULT` selects the default).
   */
  penetration?: number | undefined;
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
 * What {@link cdldarkcloudcover} returns, and {@link cdldarkcloudcoverOpenAndFill}
 * with its stream.
 */
export interface CdldarkcloudcoverResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * -100 when the pattern is detected (always bearish), 0 otherwise; never emits
   * +100. `values[k]` is the value at input bar `begIdx + k`.
   */
  values: Int32Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link cdldarkcloudcover} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdldarkcloudcoverOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdldarkcloudcoverLookback(options?: CdldarkcloudcoverOptions): number {
  const o = optionsOf('CDLDARKCLOUDCOVER lookback', options);
  return cdldarkcloudcoverLookbackPositional(orDefault(o.penetration, Core.REAL_DEFAULT), coreOf('CDLDARKCLOUDCOVER lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdldarkcloudcover}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdldarkcloudcoverOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdldarkcloudcoverDisplayShift(outputIdx: number, options?: CdldarkcloudcoverOptions): number {
  const o = optionsOf('CDLDARKCLOUDCOVER displayShift', options);
  return cdldarkcloudcoverDisplayShiftPositional(orDefault(o.penetration, Core.REAL_DEFAULT), outputIdx, coreOf('CDLDARKCLOUDCOVER displayShift', o.core));
}

/**
 * A two-candle bearish reversal pattern: a long white candle followed by a black
 * candle that opens above the prior high and closes deep into the prior white body
 * past a penetration threshold. Signals a potential top. A hit (-100) is a bearish
 * reversal signal, most meaningful after an uptrend.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdldarkcloudcover](https://ta-lib.org/functions/cdldarkcloudcover).
 *
 * **Notes**
 * - Does not verify the preceding uptrend the bearish reversal classically
 *   assumes.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cdldarkcloudcoverLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `open`: Open price of
 *   each bar. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link CdldarkcloudcoverOptions}.
 * @returns `begIdx` and the values; see {@link CdldarkcloudcoverResult}. Each
 *   output is a view of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, a real parameter that is not a number, a parameter outside its range, an
 *   input too short for the range, a buffer that is empty, too short or of the wrong
 *   type or a buffer sharing memory with a `Float64Array` input it is not. Checked
 *   before anything is written.
 *
 * @example
 * ```ts
 * import { cdldarkcloudcover } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { begIdx, values } = cdldarkcloudcover({ open, high, low, close }, {
 *   penetration: 0.5,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`cdlpiercing`](https://ta-lib.org/functions/cdlpiercing)
 * @see [`cdlengulfing`](https://ta-lib.org/functions/cdlengulfing)
 * @see [`cdlonneck`](https://ta-lib.org/functions/cdlonneck)
 */
export function cdldarkcloudcover(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdldarkcloudcoverOptions): CdldarkcloudcoverResult {
  requireArgument('CDLDARKCLOUDCOVER', 'prices', prices);
  const o = optionsOf('CDLDARKCLOUDCOVER', options);
  if (o.endIdx === undefined) {
    requireSeries('CDLDARKCLOUDCOVER', 'prices.open', prices.open);
    requireSeries('CDLDARKCLOUDCOVER', 'prices.high', prices.high);
    requireSeries('CDLDARKCLOUDCOVER', 'prices.low', prices.low);
    requireSeries('CDLDARKCLOUDCOVER', 'prices.close', prices.close);
  }
  const optInPenetration = orDefault(o.penetration, Core.REAL_DEFAULT);
  const core = coreOf('CDLDARKCLOUDCOVER', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cdldarkcloudcoverLookbackPositional(optInPenetration, core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const range = cdldarkcloudcoverPositional(startIdx, endIdx, prices.open, prices.high, prices.low, prices.close, optInPenetration, outInteger, core);
  return { begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CDLDARKCLOUDCOVER stream: one value per closed bar, bit-identical to
 * {@link cdldarkcloudcover} over the same series. Open it with
 * {@link cdldarkcloudcoverOpen} or {@link cdldarkcloudcoverOpenAndFill}; there is
 * nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdldarkcloudcover](https://ta-lib.org/functions/cdldarkcloudcover).
 */
export class CdldarkcloudcoverStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInPenetration: number = 0;
  /** @internal */
  BodyLongPeriodTotal: number = 0;
  /** @internal */
  lag1_inOpen: number = 0;
  /** @internal */
  lag1_inHigh: number = 0;
  /** @internal */
  lag1_inLow: number = 0;
  /** @internal */
  lag1_inClose: number = 0;
  /** @internal */
  ringPos_BodyLongTrailingIdx: number = 0;
  /** @internal */
  ringCap_BodyLongTrailingIdx: number = 2;
  /** @internal */
  ringLag_BodyLongTrailingIdx: number = 0;
  /** @internal */
  ring_BodyLongTrailingIdx_derived: Float64Array = new Float64Array(2);
  /** @internal */
  cs_BodyLong_rangeType: number = 0;
  /** @internal */
  cs_BodyLong_avgPeriod: number = 0;
  /** @internal */
  cs_BodyLong_factor: number = 0;
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
  static of(core: Core): CdldarkcloudcoverStream {
    return new CdldarkcloudcoverStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cdldarkcloudcover} reports them over the same bars. Each accepted
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
      throw streamFailure('CDLDARKCLOUDCOVER advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw streamFailure('CDLDARKCLOUDCOVER update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(open) || !Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('CDLDARKCLOUDCOVER update', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    cdldarkcloudcoverStepImpl(this, open, high, low, close);
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
      throw nonFiniteBar('CDLDARKCLOUDCOVER peek', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inOpen = open;
    const inClose = close;
    let cur_outInteger = 0;
    const BodyLong_rangeType = sp.cs_BodyLong_rangeType;
    const BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
    const BodyLong_factor = sp.cs_BodyLong_factor;
    if (((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === 1 && /* 1st: white */
        ((inClose >= inOpen) ? 1 : -1) === -1 &&                /* 2nd: black */
        inOpen > sp.lag1_inHigh &&                              /* open above prior high */
        Math.abs(sp.lag1_inClose - sp.lag1_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* long */
        inClose > sp.lag1_inOpen &&                             /* close within prior body */
        inClose < sp.lag1_inClose - Math.abs(sp.lag1_inClose - sp.lag1_inOpen) * sp.optInPenetration)
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
  clone(): CdldarkcloudcoverStream {
    const c = new CdldarkcloudcoverStream(this.core);
    c.optInPenetration = this.optInPenetration;
    c.BodyLongPeriodTotal = this.BodyLongPeriodTotal;
    c.lag1_inOpen = this.lag1_inOpen;
    c.lag1_inHigh = this.lag1_inHigh;
    c.lag1_inLow = this.lag1_inLow;
    c.lag1_inClose = this.lag1_inClose;
    c.ringPos_BodyLongTrailingIdx = this.ringPos_BodyLongTrailingIdx;
    c.ringCap_BodyLongTrailingIdx = this.ringCap_BodyLongTrailingIdx;
    c.ringLag_BodyLongTrailingIdx = this.ringLag_BodyLongTrailingIdx;
    c.ring_BodyLongTrailingIdx_derived = this.ring_BodyLongTrailingIdx_derived.slice();
    c.cs_BodyLong_rangeType = this.cs_BodyLong_rangeType;
    c.cs_BodyLong_avgPeriod = this.cs_BodyLong_avgPeriod;
    c.cs_BodyLong_factor = this.cs_BodyLong_factor;
    c.cur_outInteger = this.cur_outInteger;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cdldarkcloudcoverStepImpl(sp: CdldarkcloudcoverStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  const BodyLong_rangeType = sp.cs_BodyLong_rangeType;
  const BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
  const BodyLong_factor = sp.cs_BodyLong_factor;
  sp.ring_BodyLongTrailingIdx_derived[sp.ringPos_BodyLongTrailingIdx] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType === 1) ? (inHigh - inLow) : ((BodyLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  if (((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === 1 && /* 1st: white */
      ((inClose >= inOpen) ? 1 : -1) === -1 &&                /* 2nd: black */
      inOpen > sp.lag1_inHigh &&                              /* open above prior high */
      Math.abs(sp.lag1_inClose - sp.lag1_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* long */
      inClose > sp.lag1_inOpen &&                             /* close within prior body */
      inClose < sp.lag1_inClose - Math.abs(sp.lag1_inClose - sp.lag1_inOpen) * sp.optInPenetration)
  {
    sp.cur_outInteger = -100;
  } else {
    sp.cur_outInteger = 0;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  sp.BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0))) - sp.ring_BodyLongTrailingIdx_derived[(((sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 1) % sp.ringCap_BodyLongTrailingIdx) | 0)];
  sp.lag1_inOpen = inOpen;
  sp.lag1_inHigh = inHigh;
  sp.lag1_inLow = inLow;
  sp.lag1_inClose = inClose;
  sp.ringPos_BodyLongTrailingIdx = sp.ringPos_BodyLongTrailingIdx + 1;
  if (sp.ringPos_BodyLongTrailingIdx >= sp.ringCap_BodyLongTrailingIdx) {
    sp.ringPos_BodyLongTrailingIdx = 0;
  }
}

function cdldarkcloudcoverOpenImpl(
  sp: CdldarkcloudcoverStream,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInPenetration: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let BodyLongPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let BodyLongTrailingIdx = 0;
  let lookbackTotal = 0;
  const historyLen = inOpen.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inHigh.length !== inOpen.length || inLow.length !== inOpen.length || inClose.length !== inOpen.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInPenetration === Core.REAL_DEFAULT) {
    optInPenetration = 0.5;
  } else if (!(optInPenetration >= 0e0 && optInPenetration <= Core.REAL_MAX)) {
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
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdldarkcloudcoverLookbackPositional(optInPenetration, core);
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
  BodyLongPeriodTotal = 0;
  BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
  i = BodyLongTrailingIdx;
  while (i < startIdx) {
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first candle: long white candle
   * - second candle: black candle that opens above previous day high and closes within previous day real body;
   * Greg Morris wants the close to be below the midpoint of the previous real body
   * The meaning of "long" is specified with TA_SetCandleSettings, the penetration of the first real body is specified
   * with optInPenetration
   */
  outIdx = 0;
  do {
    if (((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === 1 && /* 1st: white */
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === -1 &&        /* 2nd: black */
        inOpen[i] > inHigh[i - 1] &&                          /* open above prior high */
        Math.abs(inClose[i - 1] - inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* long */
        inClose[i] > inOpen[i - 1] &&                         /* close within prior body */
        inClose[i] < inClose[i - 1] - Math.abs(inClose[i - 1] - inOpen[i - 1]) * optInPenetration)
    {
      outInteger[((outIdx++ * outStride) | 0)] = -100;
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 1] - inOpen[BodyLongTrailingIdx - 1])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyLongTrailingIdx - 1] - inLow[BodyLongTrailingIdx - 1]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyLongTrailingIdx - 1] - (((inClose[BodyLongTrailingIdx - 1]) >= (inOpen[BodyLongTrailingIdx - 1])) ? (inClose[BodyLongTrailingIdx - 1]) : (inOpen[BodyLongTrailingIdx - 1]))) + ((((inClose[BodyLongTrailingIdx - 1]) >= (inOpen[BodyLongTrailingIdx - 1])) ? (inOpen[BodyLongTrailingIdx - 1]) : (inClose[BodyLongTrailingIdx - 1])) - inLow[BodyLongTrailingIdx - 1])) : 0.0)));
    i += 1;
    BodyLongTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const capLag_BodyLongTrailingIdx = i - BodyLongTrailingIdx;
  const cap_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx + 2;
  if (capLag_BodyLongTrailingIdx < 0 || cap_BodyLongTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_BodyLongTrailingIdx = cap_BodyLongTrailingIdx > 0 ? cap_BodyLongTrailingIdx : 1;
  const capRing_BodyLongTrailingIdx_derived = new Float64Array(allocN_BodyLongTrailingIdx);
  for (let fillJ = historyLen - cap_BodyLongTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_BodyLongTrailingIdx_derived[fillJ % cap_BodyLongTrailingIdx] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyLong_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyLong_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  sp.optInPenetration = optInPenetration;
  sp.BodyLongPeriodTotal = BodyLongPeriodTotal;
  sp.lag1_inOpen = inOpen[historyLen - 1];
  sp.lag1_inHigh = inHigh[historyLen - 1];
  sp.lag1_inLow = inLow[historyLen - 1];
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.ringPos_BodyLongTrailingIdx = historyLen % cap_BodyLongTrailingIdx;
  sp.ringCap_BodyLongTrailingIdx = cap_BodyLongTrailingIdx;
  sp.ringLag_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx;
  sp.ring_BodyLongTrailingIdx_derived = capRing_BodyLongTrailingIdx_derived;
  sp.cs_BodyLong_rangeType = BodyLong_rangeType;
  sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
  sp.cs_BodyLong_factor = BodyLong_factor;
  sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cdldarkcloudcoverOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInPenetration: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): CdldarkcloudcoverStream {
  const sp = CdldarkcloudcoverStream.of(core);
  const retCode = cdldarkcloudcoverOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, optInPenetration, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLDARKCLOUDCOVER openAndFill', inOpen.length, startIdx, cdldarkcloudcoverLookbackPositional(optInPenetration, core));
  }
  if (retCode === RetCode.BAD_PARAM && cdldarkcloudcoverLookbackPositional(optInPenetration, core) < 0) {
    throw rejectedParams('CDLDARKCLOUDCOVER openAndFill', PARAM_RANGES, optInPenetration);
  }
  throw streamFailure('CDLDARKCLOUDCOVER openAndFill', retCode);
}

/** @internal */
export function cdldarkcloudcoverOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInPenetration: number,
): CdldarkcloudcoverStream {
  const sp = CdldarkcloudcoverStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdldarkcloudcoverOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, optInPenetration, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLDARKCLOUDCOVER open', inOpen.length, startIdx, cdldarkcloudcoverLookbackPositional(optInPenetration, core));
  }
  if (retCode === RetCode.BAD_PARAM && cdldarkcloudcoverLookbackPositional(optInPenetration, core) < 0) {
    throw rejectedParams('CDLDARKCLOUDCOVER open', PARAM_RANGES, optInPenetration);
  }
  throw streamFailure('CDLDARKCLOUDCOVER open', retCode);
}

/** @internal */
export function cdldarkcloudcoverOpenPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInPenetration: number,
  core: Core = Core.DEFAULT,
): CdldarkcloudcoverStream {
  optInPenetration = requireReal('CDLDARKCLOUDCOVER open', 'penetration', optInPenetration);
  requireArgument('CDLDARKCLOUDCOVER open', 'prices.open', inOpen);
  requireHistory('CDLDARKCLOUDCOVER open', 'prices.open', inOpen.length);
  requireArgument('CDLDARKCLOUDCOVER open', 'prices.high', inHigh);
  requireArgument('CDLDARKCLOUDCOVER open', 'prices.low', inLow);
  requireArgument('CDLDARKCLOUDCOVER open', 'prices.close', inClose);
  requireHistoryLength('CDLDARKCLOUDCOVER open', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLDARKCLOUDCOVER open', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLDARKCLOUDCOVER open', 'prices.close', inClose.length, inOpen.length);
  return cdldarkcloudcoverOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0, optInPenetration);
}

/** @internal */
export function cdldarkcloudcoverOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInPenetration: number,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): CdldarkcloudcoverStream {
  optInPenetration = requireReal('CDLDARKCLOUDCOVER openAndFill', 'penetration', optInPenetration);
  requireArgument('CDLDARKCLOUDCOVER openAndFill', 'prices.open', inOpen);
  requireHistory('CDLDARKCLOUDCOVER openAndFill', 'prices.open', inOpen.length);
  requireArgument('CDLDARKCLOUDCOVER openAndFill', 'prices.high', inHigh);
  requireArgument('CDLDARKCLOUDCOVER openAndFill', 'prices.low', inLow);
  requireArgument('CDLDARKCLOUDCOVER openAndFill', 'prices.close', inClose);
  const guardLookback = cdldarkcloudcoverLookbackPositional(optInPenetration, core);
  if (guardLookback < 0) {
    throw rejectedParams('CDLDARKCLOUDCOVER openAndFill', PARAM_RANGES, optInPenetration);
  }
  const guardOutLen = openFillCount('CDLDARKCLOUDCOVER openAndFill', inOpen.length, guardLookback);
  requireHistoryLength('CDLDARKCLOUDCOVER openAndFill', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLDARKCLOUDCOVER openAndFill', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLDARKCLOUDCOVER openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('CDLDARKCLOUDCOVER openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('CDLDARKCLOUDCOVER openAndFill', 'outInteger', 'prices.open');
  }
  if (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CDLDARKCLOUDCOVER openAndFill', 'outInteger', 'prices.high');
  }
  if (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CDLDARKCLOUDCOVER openAndFill', 'outInteger', 'prices.low');
  }
  if (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('CDLDARKCLOUDCOVER openAndFill', 'outInteger', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLDARKCLOUDCOVER openAndFill', 'outInteger', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLDARKCLOUDCOVER openAndFill', 'outInteger', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLDARKCLOUDCOVER openAndFill', 'outInteger', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLDARKCLOUDCOVER openAndFill', 'outInteger', 'prices.close');
  }
  return cdldarkcloudcoverOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, optInPenetration, new MInteger(), new MInteger(), outInteger);
}

/**
 * Opens a live CDLDARKCLOUDCOVER stream over the warm-up history: `value()` starts
 * at the last history bar, bit-identical to {@link cdldarkcloudcover} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdldarkcloudcoverOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdldarkcloudcoverLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, a real parameter that is not a number, a parameter
 *   outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdldarkcloudcoverOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdldarkcloudcoverOpen({ open, high, low, close }, {
 *   penetration: 0.5,
 * });
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdldarkcloudcoverOpen(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdldarkcloudcoverOptions): CdldarkcloudcoverStream {
  requireArgument('CDLDARKCLOUDCOVER open', 'prices', prices);
  const o = optionsOf('CDLDARKCLOUDCOVER open', options);
  const optInPenetration = orDefault(o.penetration, Core.REAL_DEFAULT);
  const core = coreOf('CDLDARKCLOUDCOVER open', o.core);
  return cdldarkcloudcoverOpenPositional(prices.open, prices.high, prices.low, prices.close, optInPenetration, core);
}

/**
 * {@link cdldarkcloudcoverOpen} that also computes the outputs over the whole
 * history in the same pass, bit-identically to {@link cdldarkcloudcover}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CdldarkcloudcoverOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cdldarkcloudcover} returns them; the range is
 *   the stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdldarkcloudcoverLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, a real parameter that is not a number, a parameter
 *   outside its range, histories of different lengths or a buffer too short, of the
 *   wrong type or starting where another buffer or a `Float64Array` input starts,
 *   or, on a history long enough to open, one sharing memory with another buffer or
 *   a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdldarkcloudcoverOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = cdldarkcloudcoverOpenAndFill({ open, high, low, close }, {
 *   penetration: 0.5,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cdldarkcloudcoverOpenAndFill(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdldarkcloudcoverOptions): CdldarkcloudcoverResult & { stream: CdldarkcloudcoverStream } {
  requireArgument('CDLDARKCLOUDCOVER openAndFill', 'prices', prices);
  const o = optionsOf('CDLDARKCLOUDCOVER openAndFill', options);
  const optInPenetration = orDefault(o.penetration, Core.REAL_DEFAULT);
  const core = coreOf('CDLDARKCLOUDCOVER openAndFill', o.core);
  const capacity = fillCapacity(cdldarkcloudcoverLookbackPositional(optInPenetration, core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const stream = cdldarkcloudcoverOpenAndFillPositional(prices.open, prices.high, prices.low, prices.close, optInPenetration, outInteger, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}
