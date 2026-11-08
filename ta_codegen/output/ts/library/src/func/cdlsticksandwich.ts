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
 *  032005 AC     Creation
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 *  103126 MF,CC  Nest the cheap, selective pre-check conjuncts in an outer if
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { CandleSettingType, Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function cdlsticksandwichLookbackPositional(core: Core = Core.DEFAULT): number {
  const Equal_avgPeriod = core.candleSettings[CandleSettingType.EQUAL].avgPeriod;
  return Equal_avgPeriod + 2;
}

/** @internal */
export function cdlsticksandwichDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdlsticksandwichImpl(
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
  let EqualPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let EqualTrailingIdx = 0;
  let lookbackTotal = 0;
  const Equal_rangeType: number = core.candleSettings[CandleSettingType.EQUAL].rangeType;
  const Equal_avgPeriod = core.candleSettings[CandleSettingType.EQUAL].avgPeriod;
  const Equal_factor = core.candleSettings[CandleSettingType.EQUAL].factor;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlsticksandwichLookbackPositional(core);
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
  EqualPeriodTotal = 0;
  EqualTrailingIdx = startIdx - Equal_avgPeriod;
  i = EqualTrailingIdx;
  while (i < startIdx) {
    EqualPeriodTotal += ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first candle: black candle
   * - second candle: white candle that trades only above the prior close (low > prior close)
   * - third candle: black candle with the close equal to the first candle's close
   * The meaning of "equal" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === -1 && /* first black */
        ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === 1 &&  /* second white */
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === -1 &&         /* third black */
        inLow[i - 1] > inClose[i - 2] &&                       /* 2nd low > prior close */
        inClose[i] <= inClose[i - 2] + ((Equal_factor * (((Equal_avgPeriod !== 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st and 3rd same close */
        inClose[i] >= inClose[i - 2] - ((Equal_factor * (((Equal_avgPeriod !== 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))))
    {
      outInteger[outIdx++] = 100;
    } else {
      outInteger[outIdx++] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    EqualPeriodTotal += ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0))) - ((Equal_rangeType === 0) ? (Math.abs(inClose[EqualTrailingIdx - 2] - inOpen[EqualTrailingIdx - 2])) : ((Equal_rangeType === 1) ? (inHigh[EqualTrailingIdx - 2] - inLow[EqualTrailingIdx - 2]) : ((Equal_rangeType === 2) ? ((inHigh[EqualTrailingIdx - 2] - (((inClose[EqualTrailingIdx - 2]) >= (inOpen[EqualTrailingIdx - 2])) ? (inClose[EqualTrailingIdx - 2]) : (inOpen[EqualTrailingIdx - 2]))) + ((((inClose[EqualTrailingIdx - 2]) >= (inOpen[EqualTrailingIdx - 2])) ? (inOpen[EqualTrailingIdx - 2]) : (inClose[EqualTrailingIdx - 2])) - inLow[EqualTrailingIdx - 2])) : 0.0)));
    i += 1;
    EqualTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlsticksandwichPositional(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDLSTICKSANDWICH', 'startIdx', startIdx);
  endIdx = requireInt32('CDLSTICKSANDWICH', 'endIdx', endIdx);
  requireIndexRange('CDLSTICKSANDWICH', startIdx, endIdx);
  const guardStart = clampedStart('CDLSTICKSANDWICH', startIdx, cdlsticksandwichLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLSTICKSANDWICH', 'prices.open', inOpen, guardInLen);
  requireLength('CDLSTICKSANDWICH', 'prices.high', inHigh, guardInLen);
  requireLength('CDLSTICKSANDWICH', 'prices.low', inLow, guardInLen);
  requireLength('CDLSTICKSANDWICH', 'prices.close', inClose, guardInLen);
  requireOutput('CDLSTICKSANDWICH', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLSTICKSANDWICH', 'outInteger', 'prices.open');
  }
  if (overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLSTICKSANDWICH', 'outInteger', 'prices.high');
  }
  if (overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLSTICKSANDWICH', 'outInteger', 'prices.low');
  }
  if (overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLSTICKSANDWICH', 'outInteger', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdlsticksandwichImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLSTICKSANDWICH', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cdlsticksandwich}, {@link cdlsticksandwichLookback},
 * {@link cdlsticksandwichDisplayShift}, {@link cdlsticksandwichOpen} and
 * {@link cdlsticksandwichOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface CdlsticksandwichOptions {
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
 * What {@link cdlsticksandwich} returns, and {@link cdlsticksandwichOpenAndFill}
 * with its stream.
 */
export interface CdlsticksandwichResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * +100 when the pattern is present, 0 otherwise. Never -100 — Stick Sandwich is
   * always bullish. `values[k]` is the value at input bar `begIdx + k`.
   */
  values: Int32Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link cdlsticksandwich} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlsticksandwichOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlsticksandwichLookback(options?: CdlsticksandwichOptions): number {
  const o = optionsOf('CDLSTICKSANDWICH lookback', options);
  return cdlsticksandwichLookbackPositional(coreOf('CDLSTICKSANDWICH lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdlsticksandwich}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlsticksandwichOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlsticksandwichDisplayShift(outputIdx: number, options?: CdlsticksandwichOptions): number {
  const o = optionsOf('CDLSTICKSANDWICH displayShift', options);
  return cdlsticksandwichDisplayShiftPositional(outputIdx, coreOf('CDLSTICKSANDWICH displayShift', o.core));
}

/**
 * A three-candle bullish reversal pattern: two black candles (1st and 3rd)
 * sandwiching a white candle, where the 3rd black candle closes at the same level
 * as the 1st (the "bread"). A hit signals a bullish reversal (significant in a
 * downtrend, which the function does not verify).
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlsticksandwich](https://ta-lib.org/functions/cdlsticksandwich).
 *
 * **Notes**
 * - Although classically a bullish reversal (and TA-Lib only emits +100),
 *   Bulkowski's testing found it actually acts as a bearish continuation 62% of
 *   the time — despite that, it still ranks a respectable 14th of 103 patterns for
 *   overall performance.
 *   ([thepatternsite.com](https://thepatternsite.com/StickSandwich.html))
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cdlsticksandwichLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `open`: Open price of
 *   each bar. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link CdlsticksandwichOptions}.
 * @returns `begIdx` and the values; see {@link CdlsticksandwichResult}. Each
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
 * import { cdlsticksandwich } from 'ta-lib';
 *
 * const open = new Float64Array(252); // your data, oldest first
 * const high = new Float64Array(252); // your data, oldest first
 * const low = new Float64Array(252); // your data, oldest first
 * const close = new Float64Array(252); // your data, oldest first
 *
 * const { begIdx, values } = cdlsticksandwich({ open, high, low, close });
 * console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
 * ```
 * @see [`cdlmatchinglow`](https://ta-lib.org/functions/cdlmatchinglow)
 * @see [`cdlhomingpigeon`](https://ta-lib.org/functions/cdlhomingpigeon)
 */
export function cdlsticksandwich(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlsticksandwichOptions): CdlsticksandwichResult {
  requireArgument('CDLSTICKSANDWICH', 'prices', prices);
  const o = optionsOf('CDLSTICKSANDWICH', options);
  if (o.endIdx === undefined) {
    requireSeries('CDLSTICKSANDWICH', 'prices.open', prices.open);
    requireSeries('CDLSTICKSANDWICH', 'prices.high', prices.high);
    requireSeries('CDLSTICKSANDWICH', 'prices.low', prices.low);
    requireSeries('CDLSTICKSANDWICH', 'prices.close', prices.close);
  }
  const core = coreOf('CDLSTICKSANDWICH', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cdlsticksandwichLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const range = cdlsticksandwichPositional(startIdx, endIdx, prices.open, prices.high, prices.low, prices.close, outInteger, core);
  return { begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CDLSTICKSANDWICH stream: one value per closed bar, bit-identical to
 * {@link cdlsticksandwich} over the same series. Open it with
 * {@link cdlsticksandwichOpen} or {@link cdlsticksandwichOpenAndFill}; there is
 * nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlsticksandwich](https://ta-lib.org/functions/cdlsticksandwich).
 */
export class CdlsticksandwichStream {
  /** @internal */
  core: Core;
  /** @internal */
  EqualPeriodTotal: number = 0;
  /** @internal */
  lag1_inOpen: number = 0;
  /** @internal */
  lag2_inOpen: number = 0;
  /** @internal */
  lag1_inHigh: number = 0;
  /** @internal */
  lag2_inHigh: number = 0;
  /** @internal */
  lag1_inLow: number = 0;
  /** @internal */
  lag2_inLow: number = 0;
  /** @internal */
  lag1_inClose: number = 0;
  /** @internal */
  lag2_inClose: number = 0;
  /** @internal */
  ringPos_EqualTrailingIdx: number = 0;
  /** @internal */
  ringCap_EqualTrailingIdx: number = 3;
  /** @internal */
  ringLag_EqualTrailingIdx: number = 0;
  /** @internal */
  ring_EqualTrailingIdx_derived: Float64Array = new Float64Array(3);
  /** @internal */
  cs_Equal_rangeType: number = 0;
  /** @internal */
  cs_Equal_avgPeriod: number = 0;
  /** @internal */
  cs_Equal_factor: number = 0;
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
  static of(core: Core): CdlsticksandwichStream {
    return new CdlsticksandwichStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cdlsticksandwich} reports them over the same bars. Each accepted `update`
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
      throw streamFailure('CDLSTICKSANDWICH advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw streamFailure('CDLSTICKSANDWICH update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(open) || !Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('CDLSTICKSANDWICH update', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    cdlsticksandwichStepImpl(this, open, high, low, close);
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
      throw nonFiniteBar('CDLSTICKSANDWICH peek', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inOpen = open;
    const inClose = close;
    let cur_outInteger = 0;
    const Equal_rangeType = sp.cs_Equal_rangeType;
    const Equal_avgPeriod = sp.cs_Equal_avgPeriod;
    const Equal_factor = sp.cs_Equal_factor;
    if (((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === -1 && /* first black */
        ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === 1 &&  /* second white */
        ((inClose >= inOpen) ? 1 : -1) === -1 &&                 /* third black */
        sp.lag1_inLow > sp.lag2_inClose &&                       /* 2nd low > prior close */
        inClose <= sp.lag2_inClose + ((Equal_factor * (((Equal_avgPeriod !== 0) ? (sp.EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Equal_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Equal_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st and 3rd same close */
        inClose >= sp.lag2_inClose - ((Equal_factor * (((Equal_avgPeriod !== 0) ? (sp.EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Equal_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Equal_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))))
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
  clone(): CdlsticksandwichStream {
    const c = new CdlsticksandwichStream(this.core);
    c.EqualPeriodTotal = this.EqualPeriodTotal;
    c.lag1_inOpen = this.lag1_inOpen;
    c.lag2_inOpen = this.lag2_inOpen;
    c.lag1_inHigh = this.lag1_inHigh;
    c.lag2_inHigh = this.lag2_inHigh;
    c.lag1_inLow = this.lag1_inLow;
    c.lag2_inLow = this.lag2_inLow;
    c.lag1_inClose = this.lag1_inClose;
    c.lag2_inClose = this.lag2_inClose;
    c.ringPos_EqualTrailingIdx = this.ringPos_EqualTrailingIdx;
    c.ringCap_EqualTrailingIdx = this.ringCap_EqualTrailingIdx;
    c.ringLag_EqualTrailingIdx = this.ringLag_EqualTrailingIdx;
    c.ring_EqualTrailingIdx_derived = this.ring_EqualTrailingIdx_derived.slice();
    c.cs_Equal_rangeType = this.cs_Equal_rangeType;
    c.cs_Equal_avgPeriod = this.cs_Equal_avgPeriod;
    c.cs_Equal_factor = this.cs_Equal_factor;
    c.cur_outInteger = this.cur_outInteger;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cdlsticksandwichStepImpl(sp: CdlsticksandwichStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  const Equal_rangeType = sp.cs_Equal_rangeType;
  const Equal_avgPeriod = sp.cs_Equal_avgPeriod;
  const Equal_factor = sp.cs_Equal_factor;
  sp.ring_EqualTrailingIdx_derived[sp.ringPos_EqualTrailingIdx] = ((Equal_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((Equal_rangeType === 1) ? (inHigh - inLow) : ((Equal_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  if (((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === -1 && /* first black */
      ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === 1 &&  /* second white */
      ((inClose >= inOpen) ? 1 : -1) === -1 &&                 /* third black */
      sp.lag1_inLow > sp.lag2_inClose &&                       /* 2nd low > prior close */
      inClose <= sp.lag2_inClose + ((Equal_factor * (((Equal_avgPeriod !== 0) ? (sp.EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Equal_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Equal_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st and 3rd same close */
      inClose >= sp.lag2_inClose - ((Equal_factor * (((Equal_avgPeriod !== 0) ? (sp.EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Equal_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Equal_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))))
  {
    sp.cur_outInteger = 100;
  } else {
    sp.cur_outInteger = 0;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  sp.EqualPeriodTotal += ((Equal_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Equal_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Equal_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0))) - sp.ring_EqualTrailingIdx_derived[(((sp.ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - sp.ringLag_EqualTrailingIdx - 2) % sp.ringCap_EqualTrailingIdx) | 0)];
  sp.lag2_inOpen = sp.lag1_inOpen;
  sp.lag1_inOpen = inOpen;
  sp.lag2_inHigh = sp.lag1_inHigh;
  sp.lag1_inHigh = inHigh;
  sp.lag2_inLow = sp.lag1_inLow;
  sp.lag1_inLow = inLow;
  sp.lag2_inClose = sp.lag1_inClose;
  sp.lag1_inClose = inClose;
  sp.ringPos_EqualTrailingIdx = sp.ringPos_EqualTrailingIdx + 1;
  if (sp.ringPos_EqualTrailingIdx >= sp.ringCap_EqualTrailingIdx) {
    sp.ringPos_EqualTrailingIdx = 0;
  }
}

function cdlsticksandwichOpenImpl(
  sp: CdlsticksandwichStream,
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
  let EqualPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
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
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlsticksandwichLookbackPositional(core);
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
  EqualPeriodTotal = 0;
  EqualTrailingIdx = startIdx - Equal_avgPeriod;
  i = EqualTrailingIdx;
  while (i < startIdx) {
    EqualPeriodTotal += ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first candle: black candle
   * - second candle: white candle that trades only above the prior close (low > prior close)
   * - third candle: black candle with the close equal to the first candle's close
   * The meaning of "equal" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === -1 && /* first black */
        ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === 1 &&  /* second white */
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === -1 &&         /* third black */
        inLow[i - 1] > inClose[i - 2] &&                       /* 2nd low > prior close */
        inClose[i] <= inClose[i - 2] + ((Equal_factor * (((Equal_avgPeriod !== 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st and 3rd same close */
        inClose[i] >= inClose[i - 2] - ((Equal_factor * (((Equal_avgPeriod !== 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))))
    {
      outInteger[((outIdx++ * outStride) | 0)] = 100;
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    EqualPeriodTotal += ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0))) - ((Equal_rangeType === 0) ? (Math.abs(inClose[EqualTrailingIdx - 2] - inOpen[EqualTrailingIdx - 2])) : ((Equal_rangeType === 1) ? (inHigh[EqualTrailingIdx - 2] - inLow[EqualTrailingIdx - 2]) : ((Equal_rangeType === 2) ? ((inHigh[EqualTrailingIdx - 2] - (((inClose[EqualTrailingIdx - 2]) >= (inOpen[EqualTrailingIdx - 2])) ? (inClose[EqualTrailingIdx - 2]) : (inOpen[EqualTrailingIdx - 2]))) + ((((inClose[EqualTrailingIdx - 2]) >= (inOpen[EqualTrailingIdx - 2])) ? (inOpen[EqualTrailingIdx - 2]) : (inClose[EqualTrailingIdx - 2])) - inLow[EqualTrailingIdx - 2])) : 0.0)));
    i += 1;
    EqualTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const capLag_EqualTrailingIdx = i - EqualTrailingIdx;
  const cap_EqualTrailingIdx = capLag_EqualTrailingIdx + 3;
  if (capLag_EqualTrailingIdx < 0 || cap_EqualTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_EqualTrailingIdx = cap_EqualTrailingIdx > 0 ? cap_EqualTrailingIdx : 1;
  const capRing_EqualTrailingIdx_derived = new Float64Array(allocN_EqualTrailingIdx);
  for (let fillJ = historyLen - cap_EqualTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_EqualTrailingIdx_derived[fillJ % cap_EqualTrailingIdx] = ((Equal_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((Equal_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((Equal_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  sp.EqualPeriodTotal = EqualPeriodTotal;
  sp.lag1_inOpen = inOpen[historyLen - 1];
  sp.lag2_inOpen = inOpen[historyLen - 2];
  sp.lag1_inHigh = inHigh[historyLen - 1];
  sp.lag2_inHigh = inHigh[historyLen - 2];
  sp.lag1_inLow = inLow[historyLen - 1];
  sp.lag2_inLow = inLow[historyLen - 2];
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.lag2_inClose = inClose[historyLen - 2];
  sp.ringPos_EqualTrailingIdx = historyLen % cap_EqualTrailingIdx;
  sp.ringCap_EqualTrailingIdx = cap_EqualTrailingIdx;
  sp.ringLag_EqualTrailingIdx = capLag_EqualTrailingIdx;
  sp.ring_EqualTrailingIdx_derived = capRing_EqualTrailingIdx_derived;
  sp.cs_Equal_rangeType = Equal_rangeType;
  sp.cs_Equal_avgPeriod = Equal_avgPeriod;
  sp.cs_Equal_factor = Equal_factor;
  sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlsticksandwichOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): CdlsticksandwichStream {
  const sp = CdlsticksandwichStream.of(core);
  const retCode = cdlsticksandwichOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLSTICKSANDWICH openAndFill', inOpen.length, startIdx, cdlsticksandwichLookbackPositional(core));
  }
  throw streamFailure('CDLSTICKSANDWICH openAndFill', retCode);
}

/** @internal */
export function cdlsticksandwichOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): CdlsticksandwichStream {
  const sp = CdlsticksandwichStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdlsticksandwichOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLSTICKSANDWICH open', inOpen.length, startIdx, cdlsticksandwichLookbackPositional(core));
  }
  throw streamFailure('CDLSTICKSANDWICH open', retCode);
}

/** @internal */
export function cdlsticksandwichOpenPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): CdlsticksandwichStream {
  requireArgument('CDLSTICKSANDWICH open', 'prices.open', inOpen);
  requireHistory('CDLSTICKSANDWICH open', 'prices.open', inOpen.length);
  requireArgument('CDLSTICKSANDWICH open', 'prices.high', inHigh);
  requireArgument('CDLSTICKSANDWICH open', 'prices.low', inLow);
  requireArgument('CDLSTICKSANDWICH open', 'prices.close', inClose);
  requireHistoryLength('CDLSTICKSANDWICH open', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLSTICKSANDWICH open', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLSTICKSANDWICH open', 'prices.close', inClose.length, inOpen.length);
  return cdlsticksandwichOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/** @internal */
export function cdlsticksandwichOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): CdlsticksandwichStream {
  requireArgument('CDLSTICKSANDWICH openAndFill', 'prices.open', inOpen);
  requireHistory('CDLSTICKSANDWICH openAndFill', 'prices.open', inOpen.length);
  requireArgument('CDLSTICKSANDWICH openAndFill', 'prices.high', inHigh);
  requireArgument('CDLSTICKSANDWICH openAndFill', 'prices.low', inLow);
  requireArgument('CDLSTICKSANDWICH openAndFill', 'prices.close', inClose);
  const guardOutLen = openFillCount('CDLSTICKSANDWICH openAndFill', inOpen.length, cdlsticksandwichLookbackPositional(core));
  requireHistoryLength('CDLSTICKSANDWICH openAndFill', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLSTICKSANDWICH openAndFill', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLSTICKSANDWICH openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('CDLSTICKSANDWICH openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('CDLSTICKSANDWICH openAndFill', 'outInteger', 'prices.open');
  }
  if (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CDLSTICKSANDWICH openAndFill', 'outInteger', 'prices.high');
  }
  if (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CDLSTICKSANDWICH openAndFill', 'outInteger', 'prices.low');
  }
  if (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('CDLSTICKSANDWICH openAndFill', 'outInteger', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLSTICKSANDWICH openAndFill', 'outInteger', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLSTICKSANDWICH openAndFill', 'outInteger', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLSTICKSANDWICH openAndFill', 'outInteger', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLSTICKSANDWICH openAndFill', 'outInteger', 'prices.close');
  }
  return cdlsticksandwichOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}

/**
 * Opens a live CDLSTICKSANDWICH stream over the warm-up history: `value()` starts
 * at the last history bar, bit-identical to {@link cdlsticksandwich} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlsticksandwichOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlsticksandwichLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input or histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdlsticksandwichOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdlsticksandwichOpen({ open, high, low, close });
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdlsticksandwichOpen(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlsticksandwichOptions): CdlsticksandwichStream {
  requireArgument('CDLSTICKSANDWICH open', 'prices', prices);
  const o = optionsOf('CDLSTICKSANDWICH open', options);
  const core = coreOf('CDLSTICKSANDWICH open', o.core);
  return cdlsticksandwichOpenPositional(prices.open, prices.high, prices.low, prices.close, core);
}

/**
 * {@link cdlsticksandwichOpen} that also computes the outputs over the whole
 * history in the same pass, bit-identically to {@link cdlsticksandwich}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CdlsticksandwichOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cdlsticksandwich} returns them; the range is
 *   the stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlsticksandwichLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, histories of different lengths or a buffer too short,
 *   of the wrong type or starting where another buffer or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdlsticksandwichOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = cdlsticksandwichOpenAndFill({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cdlsticksandwichOpenAndFill(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlsticksandwichOptions): CdlsticksandwichResult & { stream: CdlsticksandwichStream } {
  requireArgument('CDLSTICKSANDWICH openAndFill', 'prices', prices);
  const o = optionsOf('CDLSTICKSANDWICH openAndFill', options);
  const core = coreOf('CDLSTICKSANDWICH openAndFill', o.core);
  const capacity = fillCapacity(cdlsticksandwichLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const stream = cdlsticksandwichOpenAndFillPositional(prices.open, prices.high, prices.low, prices.close, outInteger, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}
