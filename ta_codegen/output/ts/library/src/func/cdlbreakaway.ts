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
export function cdlbreakawayLookbackPositional(core: Core = Core.DEFAULT): number {
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  return BodyLong_avgPeriod + 4;
}

/** @internal */
export function cdlbreakawayDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdlbreakawayImpl(
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
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlbreakawayLookbackPositional(core);
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
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first candle: long black (white)
   * - second candle: black (white) day whose body gaps down (up)
   * - third candle: black or white day with lower (higher) high and lower (higher) low than prior candle's
   * - fourth candle: black (white) day with lower (higher) high and lower (higher) low than prior candle's
   * - fifth candle: white (black) day that closes inside the gap, erasing the prior 3 days
   * The meaning of "long" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) === ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : -1) && /* 1st, 2nd, 4th same color, 5th opposite */
        ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : -1) === ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) &&
        ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === (-((inClose[i] >= inOpen[i]) ? 1 : -1) | 0) &&
        Math.abs(inClose[i - 4] - inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st long */
        (((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) === -1 && /* when 1st is black: */
          ((inOpen[i - 3] > inClose[i - 3] ? inOpen[i - 3] : inClose[i - 3]) < (inOpen[i - 4] < inClose[i - 4] ? inOpen[i - 4] : inClose[i - 4])) && /* 2nd gaps down */
          inHigh[i - 2] < inHigh[i - 3] &&
          inLow[i - 2] < inLow[i - 3] &&                        /* 3rd has lower high and low than 2nd */
          inHigh[i - 1] < inHigh[i - 2] &&
          inLow[i - 1] < inLow[i - 2] &&                        /* 4th has lower high and low than 3rd */
          inClose[i] > inOpen[i - 3] &&
          inClose[i] < inClose[i - 4] ||                        /* 5th closes inside the gap */
         ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) === 1 &&  /* when 1st is white: */
          ((inOpen[i - 3] < inClose[i - 3] ? inOpen[i - 3] : inClose[i - 3]) > (inOpen[i - 4] > inClose[i - 4] ? inOpen[i - 4] : inClose[i - 4])) && /* 2nd gaps up */
          inHigh[i - 2] > inHigh[i - 3] &&
          inLow[i - 2] > inLow[i - 3] &&                        /* 3rd has higher high and low than 2nd */
          inHigh[i - 1] > inHigh[i - 2] &&
          inLow[i - 1] > inLow[i - 2] &&                        /* 4th has higher high and low than 3rd */
          inClose[i] < inOpen[i - 3] &&
          inClose[i] > inClose[i - 4]))                         /* 5th closes inside the gap */
    {
      outInteger[outIdx++] = ((((inClose[i] >= inOpen[i]) ? 1 : -1) * 100) | 0);
    } else {
      outInteger[outIdx++] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0))) - ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 4] - inOpen[BodyLongTrailingIdx - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyLongTrailingIdx - 4] - inLow[BodyLongTrailingIdx - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyLongTrailingIdx - 4] - (((inClose[BodyLongTrailingIdx - 4]) >= (inOpen[BodyLongTrailingIdx - 4])) ? (inClose[BodyLongTrailingIdx - 4]) : (inOpen[BodyLongTrailingIdx - 4]))) + ((((inClose[BodyLongTrailingIdx - 4]) >= (inOpen[BodyLongTrailingIdx - 4])) ? (inOpen[BodyLongTrailingIdx - 4]) : (inClose[BodyLongTrailingIdx - 4])) - inLow[BodyLongTrailingIdx - 4])) : 0.0)));
    i += 1;
    BodyLongTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlbreakawayPositional(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDLBREAKAWAY', 'startIdx', startIdx);
  endIdx = requireInt32('CDLBREAKAWAY', 'endIdx', endIdx);
  requireIndexRange('CDLBREAKAWAY', startIdx, endIdx);
  const guardStart = clampedStart('CDLBREAKAWAY', startIdx, cdlbreakawayLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLBREAKAWAY', 'prices.open', inOpen, guardInLen);
  requireLength('CDLBREAKAWAY', 'prices.high', inHigh, guardInLen);
  requireLength('CDLBREAKAWAY', 'prices.low', inLow, guardInLen);
  requireLength('CDLBREAKAWAY', 'prices.close', inClose, guardInLen);
  requireOutput('CDLBREAKAWAY', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLBREAKAWAY', 'outInteger', 'prices.open');
  }
  if (overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLBREAKAWAY', 'outInteger', 'prices.high');
  }
  if (overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLBREAKAWAY', 'outInteger', 'prices.low');
  }
  if (overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLBREAKAWAY', 'outInteger', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdlbreakawayImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLBREAKAWAY', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cdlbreakaway}, {@link cdlbreakawayLookback},
 * {@link cdlbreakawayDisplayShift}, {@link cdlbreakawayOpen} and
 * {@link cdlbreakawayOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface CdlbreakawayOptions {
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
 * What {@link cdlbreakaway} returns, and {@link cdlbreakawayOpenAndFill} with its
 * stream.
 */
export interface CdlbreakawayResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * +100 when the fifth candle is white (bullish breakaway), -100 when it is black
   * (bearish breakaway), 0 otherwise. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Int32Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link cdlbreakaway} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlbreakawayOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlbreakawayLookback(options?: CdlbreakawayOptions): number {
  const o = optionsOf('CDLBREAKAWAY lookback', options);
  return cdlbreakawayLookbackPositional(coreOf('CDLBREAKAWAY lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdlbreakaway}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlbreakawayOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlbreakawayDisplayShift(outputIdx: number, options?: CdlbreakawayOptions): number {
  const o = optionsOf('CDLBREAKAWAY displayShift', options);
  return cdlbreakawayDisplayShiftPositional(outputIdx, coreOf('CDLBREAKAWAY displayShift', o.core));
}

/**
 * A five-candle reversal pattern: a long first candle, a same-colored second
 * candle that gaps away from it by its real body, two more candles extending the
 * move, and an opposite-colored fifth candle that closes back inside the gap.
 * Emits a bullish signal (bottom reversal) or bearish signal (top reversal).
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlbreakaway](https://ta-lib.org/functions/cdlbreakaway).
 *
 * **Notes**
 * - Does not verify the prior trend the pattern classically assumes (a breakaway
 *   matters most against a preceding move).
 * - Bulkowski's data shows a directional asymmetry TA-Lib's symmetric output
 *   doesn't capture: bullish Breakaway reverses only 59% of the time ("near
 *   random"), while bearish Breakaway reverses 63% of the time overall.
 *   ([thepatternsite.com](https://thepatternsite.com/BullBreakaway.html))
 * - Rare: a few years of one symbol's daily bars may show none. Bulkowski ranks
 *   the bullish form 97th and the bearish 98th of 103 patterns by how often they
 *   occur ([bullish](https://thepatternsite.com/BullBreakaway.html),
 *   [bearish](https://thepatternsite.com/BearBreakaway.html)).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cdlbreakawayLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `open`: Open price of
 *   each bar. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link CdlbreakawayOptions}.
 * @returns `begIdx` and the values; see {@link CdlbreakawayResult}. Each output is
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
 * import { cdlbreakaway } from 'ta-lib';
 *
 * const open = new Float64Array(252); // your data, oldest first
 * const high = new Float64Array(252); // your data, oldest first
 * const low = new Float64Array(252); // your data, oldest first
 * const close = new Float64Array(252); // your data, oldest first
 *
 * const { begIdx, values } = cdlbreakaway({ open, high, low, close });
 * console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
 * ```
 * @see [`cdlgapsidesidewhite`](https://ta-lib.org/functions/cdlgapsidesidewhite)
 * @see [`cdlrisefall3methods`](https://ta-lib.org/functions/cdlrisefall3methods)
 * @see [`cdl3linestrike`](https://ta-lib.org/functions/cdl3linestrike)
 */
export function cdlbreakaway(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlbreakawayOptions): CdlbreakawayResult {
  requireArgument('CDLBREAKAWAY', 'prices', prices);
  const o = optionsOf('CDLBREAKAWAY', options);
  if (o.endIdx === undefined) {
    requireSeries('CDLBREAKAWAY', 'prices.open', prices.open);
    requireSeries('CDLBREAKAWAY', 'prices.high', prices.high);
    requireSeries('CDLBREAKAWAY', 'prices.low', prices.low);
    requireSeries('CDLBREAKAWAY', 'prices.close', prices.close);
  }
  const core = coreOf('CDLBREAKAWAY', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cdlbreakawayLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const range = cdlbreakawayPositional(startIdx, endIdx, prices.open, prices.high, prices.low, prices.close, outInteger, core);
  return { begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CDLBREAKAWAY stream: one value per closed bar, bit-identical to
 * {@link cdlbreakaway} over the same series. Open it with {@link cdlbreakawayOpen}
 * or {@link cdlbreakawayOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlbreakaway](https://ta-lib.org/functions/cdlbreakaway).
 */
export class CdlbreakawayStream {
  /** @internal */
  core: Core;
  /** @internal */
  BodyLongPeriodTotal: number = 0;
  /** @internal */
  lag1_inOpen: number = 0;
  /** @internal */
  lag2_inOpen: number = 0;
  /** @internal */
  lag3_inOpen: number = 0;
  /** @internal */
  lag4_inOpen: number = 0;
  /** @internal */
  lag1_inHigh: number = 0;
  /** @internal */
  lag2_inHigh: number = 0;
  /** @internal */
  lag3_inHigh: number = 0;
  /** @internal */
  lag4_inHigh: number = 0;
  /** @internal */
  lag1_inLow: number = 0;
  /** @internal */
  lag2_inLow: number = 0;
  /** @internal */
  lag3_inLow: number = 0;
  /** @internal */
  lag4_inLow: number = 0;
  /** @internal */
  lag1_inClose: number = 0;
  /** @internal */
  lag2_inClose: number = 0;
  /** @internal */
  lag3_inClose: number = 0;
  /** @internal */
  lag4_inClose: number = 0;
  /** @internal */
  ringPos_BodyLongTrailingIdx: number = 0;
  /** @internal */
  ringCap_BodyLongTrailingIdx: number = 5;
  /** @internal */
  ringLag_BodyLongTrailingIdx: number = 0;
  /** @internal */
  ring_BodyLongTrailingIdx_derived: Float64Array = new Float64Array(5);
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
  static of(core: Core): CdlbreakawayStream {
    return new CdlbreakawayStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cdlbreakaway} reports them over the same bars. Each accepted `update` and
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
      throw streamFailure('CDLBREAKAWAY advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw streamFailure('CDLBREAKAWAY update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(open) || !Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('CDLBREAKAWAY update', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    cdlbreakawayStepImpl(this, open, high, low, close);
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
      throw nonFiniteBar('CDLBREAKAWAY peek', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inOpen = open;
    const inClose = close;
    let cur_outInteger = 0;
    const BodyLong_rangeType = sp.cs_BodyLong_rangeType;
    const BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
    const BodyLong_factor = sp.cs_BodyLong_factor;
    if (((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) === ((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : -1) && /* 1st, 2nd, 4th same color, 5th opposite */
        ((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : -1) === ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) &&
        ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === (-((inClose >= inOpen) ? 1 : -1) | 0) &&
        Math.abs(sp.lag4_inClose - sp.lag4_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag4_inClose - sp.lag4_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag4_inHigh - sp.lag4_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag4_inHigh - (((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inClose) : (sp.lag4_inOpen))) + ((((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inOpen) : (sp.lag4_inClose)) - sp.lag4_inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st long */
        (((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) === -1 && /* when 1st is black: */
          ((sp.lag3_inOpen > sp.lag3_inClose ? sp.lag3_inOpen : sp.lag3_inClose) < (sp.lag4_inOpen < sp.lag4_inClose ? sp.lag4_inOpen : sp.lag4_inClose)) && /* 2nd gaps down */
          sp.lag2_inHigh < sp.lag3_inHigh &&
          sp.lag2_inLow < sp.lag3_inLow &&                        /* 3rd has lower high and low than 2nd */
          sp.lag1_inHigh < sp.lag2_inHigh &&
          sp.lag1_inLow < sp.lag2_inLow &&                        /* 4th has lower high and low than 3rd */
          inClose > sp.lag3_inOpen &&
          inClose < sp.lag4_inClose ||                            /* 5th closes inside the gap */
         ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) === 1 &&  /* when 1st is white: */
          ((sp.lag3_inOpen < sp.lag3_inClose ? sp.lag3_inOpen : sp.lag3_inClose) > (sp.lag4_inOpen > sp.lag4_inClose ? sp.lag4_inOpen : sp.lag4_inClose)) && /* 2nd gaps up */
          sp.lag2_inHigh > sp.lag3_inHigh &&
          sp.lag2_inLow > sp.lag3_inLow &&                        /* 3rd has higher high and low than 2nd */
          sp.lag1_inHigh > sp.lag2_inHigh &&
          sp.lag1_inLow > sp.lag2_inLow &&                        /* 4th has higher high and low than 3rd */
          inClose < sp.lag3_inOpen &&
          inClose > sp.lag4_inClose))                             /* 5th closes inside the gap */
    {
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
  clone(): CdlbreakawayStream {
    const c = new CdlbreakawayStream(this.core);
    c.BodyLongPeriodTotal = this.BodyLongPeriodTotal;
    c.lag1_inOpen = this.lag1_inOpen;
    c.lag2_inOpen = this.lag2_inOpen;
    c.lag3_inOpen = this.lag3_inOpen;
    c.lag4_inOpen = this.lag4_inOpen;
    c.lag1_inHigh = this.lag1_inHigh;
    c.lag2_inHigh = this.lag2_inHigh;
    c.lag3_inHigh = this.lag3_inHigh;
    c.lag4_inHigh = this.lag4_inHigh;
    c.lag1_inLow = this.lag1_inLow;
    c.lag2_inLow = this.lag2_inLow;
    c.lag3_inLow = this.lag3_inLow;
    c.lag4_inLow = this.lag4_inLow;
    c.lag1_inClose = this.lag1_inClose;
    c.lag2_inClose = this.lag2_inClose;
    c.lag3_inClose = this.lag3_inClose;
    c.lag4_inClose = this.lag4_inClose;
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

function cdlbreakawayStepImpl(sp: CdlbreakawayStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  let ringPosL_BodyLongTrailingIdx = 0;
  const BodyLong_rangeType = sp.cs_BodyLong_rangeType;
  const BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
  const BodyLong_factor = sp.cs_BodyLong_factor;
  sp.ring_BodyLongTrailingIdx_derived[sp.ringPos_BodyLongTrailingIdx] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType === 1) ? (inHigh - inLow) : ((BodyLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  if (((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) === ((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : -1) && /* 1st, 2nd, 4th same color, 5th opposite */
      ((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : -1) === ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) &&
      ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === (-((inClose >= inOpen) ? 1 : -1) | 0) &&
      Math.abs(sp.lag4_inClose - sp.lag4_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag4_inClose - sp.lag4_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag4_inHigh - sp.lag4_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag4_inHigh - (((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inClose) : (sp.lag4_inOpen))) + ((((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inOpen) : (sp.lag4_inClose)) - sp.lag4_inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st long */
      (((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) === -1 && /* when 1st is black: */
        ((sp.lag3_inOpen > sp.lag3_inClose ? sp.lag3_inOpen : sp.lag3_inClose) < (sp.lag4_inOpen < sp.lag4_inClose ? sp.lag4_inOpen : sp.lag4_inClose)) && /* 2nd gaps down */
        sp.lag2_inHigh < sp.lag3_inHigh &&
        sp.lag2_inLow < sp.lag3_inLow &&                        /* 3rd has lower high and low than 2nd */
        sp.lag1_inHigh < sp.lag2_inHigh &&
        sp.lag1_inLow < sp.lag2_inLow &&                        /* 4th has lower high and low than 3rd */
        inClose > sp.lag3_inOpen &&
        inClose < sp.lag4_inClose ||                            /* 5th closes inside the gap */
       ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) === 1 &&  /* when 1st is white: */
        ((sp.lag3_inOpen < sp.lag3_inClose ? sp.lag3_inOpen : sp.lag3_inClose) > (sp.lag4_inOpen > sp.lag4_inClose ? sp.lag4_inOpen : sp.lag4_inClose)) && /* 2nd gaps up */
        sp.lag2_inHigh > sp.lag3_inHigh &&
        sp.lag2_inLow > sp.lag3_inLow &&                        /* 3rd has higher high and low than 2nd */
        sp.lag1_inHigh > sp.lag2_inHigh &&
        sp.lag1_inLow > sp.lag2_inLow &&                        /* 4th has higher high and low than 3rd */
        inClose < sp.lag3_inOpen &&
        inClose > sp.lag4_inClose))                             /* 5th closes inside the gap */
  {
    sp.cur_outInteger = ((((inClose >= inOpen) ? 1 : -1) * 100) | 0);
  } else {
    sp.cur_outInteger = 0;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  sp.BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag4_inClose - sp.lag4_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag4_inHigh - sp.lag4_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag4_inHigh - (((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inClose) : (sp.lag4_inOpen))) + ((((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inOpen) : (sp.lag4_inClose)) - sp.lag4_inLow)) : 0.0))) - sp.ring_BodyLongTrailingIdx_derived[(((sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 4) % sp.ringCap_BodyLongTrailingIdx) | 0)];
  sp.lag4_inOpen = sp.lag3_inOpen;
  sp.lag3_inOpen = sp.lag2_inOpen;
  sp.lag2_inOpen = sp.lag1_inOpen;
  sp.lag1_inOpen = inOpen;
  sp.lag4_inHigh = sp.lag3_inHigh;
  sp.lag3_inHigh = sp.lag2_inHigh;
  sp.lag2_inHigh = sp.lag1_inHigh;
  sp.lag1_inHigh = inHigh;
  sp.lag4_inLow = sp.lag3_inLow;
  sp.lag3_inLow = sp.lag2_inLow;
  sp.lag2_inLow = sp.lag1_inLow;
  sp.lag1_inLow = inLow;
  sp.lag4_inClose = sp.lag3_inClose;
  sp.lag3_inClose = sp.lag2_inClose;
  sp.lag2_inClose = sp.lag1_inClose;
  sp.lag1_inClose = inClose;
  ringPosL_BodyLongTrailingIdx = sp.ringPos_BodyLongTrailingIdx;
  ringPosL_BodyLongTrailingIdx = ringPosL_BodyLongTrailingIdx + 1;
  ringPosL_BodyLongTrailingIdx = ringPosL_BodyLongTrailingIdx & (ringPosL_BodyLongTrailingIdx - sp.ringCap_BodyLongTrailingIdx >> 31);
  sp.ringPos_BodyLongTrailingIdx = ringPosL_BodyLongTrailingIdx;
}

function cdlbreakawayOpenImpl(
  sp: CdlbreakawayStream,
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
  lookbackTotal = cdlbreakawayLookbackPositional(core);
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
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first candle: long black (white)
   * - second candle: black (white) day whose body gaps down (up)
   * - third candle: black or white day with lower (higher) high and lower (higher) low than prior candle's
   * - fourth candle: black (white) day with lower (higher) high and lower (higher) low than prior candle's
   * - fifth candle: white (black) day that closes inside the gap, erasing the prior 3 days
   * The meaning of "long" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) === ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : -1) && /* 1st, 2nd, 4th same color, 5th opposite */
        ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : -1) === ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) &&
        ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === (-((inClose[i] >= inOpen[i]) ? 1 : -1) | 0) &&
        Math.abs(inClose[i - 4] - inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st long */
        (((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) === -1 && /* when 1st is black: */
          ((inOpen[i - 3] > inClose[i - 3] ? inOpen[i - 3] : inClose[i - 3]) < (inOpen[i - 4] < inClose[i - 4] ? inOpen[i - 4] : inClose[i - 4])) && /* 2nd gaps down */
          inHigh[i - 2] < inHigh[i - 3] &&
          inLow[i - 2] < inLow[i - 3] &&                        /* 3rd has lower high and low than 2nd */
          inHigh[i - 1] < inHigh[i - 2] &&
          inLow[i - 1] < inLow[i - 2] &&                        /* 4th has lower high and low than 3rd */
          inClose[i] > inOpen[i - 3] &&
          inClose[i] < inClose[i - 4] ||                        /* 5th closes inside the gap */
         ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) === 1 &&  /* when 1st is white: */
          ((inOpen[i - 3] < inClose[i - 3] ? inOpen[i - 3] : inClose[i - 3]) > (inOpen[i - 4] > inClose[i - 4] ? inOpen[i - 4] : inClose[i - 4])) && /* 2nd gaps up */
          inHigh[i - 2] > inHigh[i - 3] &&
          inLow[i - 2] > inLow[i - 3] &&                        /* 3rd has higher high and low than 2nd */
          inHigh[i - 1] > inHigh[i - 2] &&
          inLow[i - 1] > inLow[i - 2] &&                        /* 4th has higher high and low than 3rd */
          inClose[i] < inOpen[i - 3] &&
          inClose[i] > inClose[i - 4]))                         /* 5th closes inside the gap */
    {
      outInteger[((outIdx++ * outStride) | 0)] = ((((inClose[i] >= inOpen[i]) ? 1 : -1) * 100) | 0);
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0))) - ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 4] - inOpen[BodyLongTrailingIdx - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyLongTrailingIdx - 4] - inLow[BodyLongTrailingIdx - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyLongTrailingIdx - 4] - (((inClose[BodyLongTrailingIdx - 4]) >= (inOpen[BodyLongTrailingIdx - 4])) ? (inClose[BodyLongTrailingIdx - 4]) : (inOpen[BodyLongTrailingIdx - 4]))) + ((((inClose[BodyLongTrailingIdx - 4]) >= (inOpen[BodyLongTrailingIdx - 4])) ? (inOpen[BodyLongTrailingIdx - 4]) : (inClose[BodyLongTrailingIdx - 4])) - inLow[BodyLongTrailingIdx - 4])) : 0.0)));
    i += 1;
    BodyLongTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const capLag_BodyLongTrailingIdx = i - BodyLongTrailingIdx;
  const cap_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx + 5;
  if (capLag_BodyLongTrailingIdx < 0 || cap_BodyLongTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_BodyLongTrailingIdx = cap_BodyLongTrailingIdx > 0 ? cap_BodyLongTrailingIdx : 1;
  const capRing_BodyLongTrailingIdx_derived = new Float64Array(allocN_BodyLongTrailingIdx);
  for (let fillJ = historyLen - cap_BodyLongTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_BodyLongTrailingIdx_derived[fillJ % cap_BodyLongTrailingIdx] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyLong_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyLong_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  sp.BodyLongPeriodTotal = BodyLongPeriodTotal;
  sp.lag1_inOpen = inOpen[historyLen - 1];
  sp.lag2_inOpen = inOpen[historyLen - 2];
  sp.lag3_inOpen = inOpen[historyLen - 3];
  sp.lag4_inOpen = inOpen[historyLen - 4];
  sp.lag1_inHigh = inHigh[historyLen - 1];
  sp.lag2_inHigh = inHigh[historyLen - 2];
  sp.lag3_inHigh = inHigh[historyLen - 3];
  sp.lag4_inHigh = inHigh[historyLen - 4];
  sp.lag1_inLow = inLow[historyLen - 1];
  sp.lag2_inLow = inLow[historyLen - 2];
  sp.lag3_inLow = inLow[historyLen - 3];
  sp.lag4_inLow = inLow[historyLen - 4];
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.lag2_inClose = inClose[historyLen - 2];
  sp.lag3_inClose = inClose[historyLen - 3];
  sp.lag4_inClose = inClose[historyLen - 4];
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

function cdlbreakawayOpenSinkFrame(
  sp: CdlbreakawayStream,
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
  lookbackTotal = cdlbreakawayLookbackPositional(core);
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
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first candle: long black (white)
   * - second candle: black (white) day whose body gaps down (up)
   * - third candle: black or white day with lower (higher) high and lower (higher) low than prior candle's
   * - fourth candle: black (white) day with lower (higher) high and lower (higher) low than prior candle's
   * - fifth candle: white (black) day that closes inside the gap, erasing the prior 3 days
   * The meaning of "long" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  while (i < endIdx) {
    outIdx += 1;
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0))) - ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 4] - inOpen[BodyLongTrailingIdx - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyLongTrailingIdx - 4] - inLow[BodyLongTrailingIdx - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyLongTrailingIdx - 4] - (((inClose[BodyLongTrailingIdx - 4]) >= (inOpen[BodyLongTrailingIdx - 4])) ? (inClose[BodyLongTrailingIdx - 4]) : (inOpen[BodyLongTrailingIdx - 4]))) + ((((inClose[BodyLongTrailingIdx - 4]) >= (inOpen[BodyLongTrailingIdx - 4])) ? (inOpen[BodyLongTrailingIdx - 4]) : (inClose[BodyLongTrailingIdx - 4])) - inLow[BodyLongTrailingIdx - 4])) : 0.0)));
    i += 1;
    BodyLongTrailingIdx += 1;
  }
  if (((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) === ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : -1) && /* 1st, 2nd, 4th same color, 5th opposite */
      ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : -1) === ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) &&
      ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === (-((inClose[i] >= inOpen[i]) ? 1 : -1) | 0) &&
      Math.abs(inClose[i - 4] - inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st long */
      (((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) === -1 && /* when 1st is black: */
        ((inOpen[i - 3] > inClose[i - 3] ? inOpen[i - 3] : inClose[i - 3]) < (inOpen[i - 4] < inClose[i - 4] ? inOpen[i - 4] : inClose[i - 4])) && /* 2nd gaps down */
        inHigh[i - 2] < inHigh[i - 3] &&
        inLow[i - 2] < inLow[i - 3] &&                        /* 3rd has lower high and low than 2nd */
        inHigh[i - 1] < inHigh[i - 2] &&
        inLow[i - 1] < inLow[i - 2] &&                        /* 4th has lower high and low than 3rd */
        inClose[i] > inOpen[i - 3] &&
        inClose[i] < inClose[i - 4] ||                        /* 5th closes inside the gap */
       ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) === 1 &&  /* when 1st is white: */
        ((inOpen[i - 3] < inClose[i - 3] ? inOpen[i - 3] : inClose[i - 3]) > (inOpen[i - 4] > inClose[i - 4] ? inOpen[i - 4] : inClose[i - 4])) && /* 2nd gaps up */
        inHigh[i - 2] > inHigh[i - 3] &&
        inLow[i - 2] > inLow[i - 3] &&                        /* 3rd has higher high and low than 2nd */
        inHigh[i - 1] > inHigh[i - 2] &&
        inLow[i - 1] > inLow[i - 2] &&                        /* 4th has higher high and low than 3rd */
        inClose[i] < inOpen[i - 3] &&
        inClose[i] > inClose[i - 4]))                         /* 5th closes inside the gap */
  {
    outInteger[0] = ((((inClose[i] >= inOpen[i]) ? 1 : -1) * 100) | 0);
    outIdx += 1;
  } else {
    outInteger[0] = 0;
    outIdx += 1;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0))) - ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 4] - inOpen[BodyLongTrailingIdx - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyLongTrailingIdx - 4] - inLow[BodyLongTrailingIdx - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyLongTrailingIdx - 4] - (((inClose[BodyLongTrailingIdx - 4]) >= (inOpen[BodyLongTrailingIdx - 4])) ? (inClose[BodyLongTrailingIdx - 4]) : (inOpen[BodyLongTrailingIdx - 4]))) + ((((inClose[BodyLongTrailingIdx - 4]) >= (inOpen[BodyLongTrailingIdx - 4])) ? (inOpen[BodyLongTrailingIdx - 4]) : (inClose[BodyLongTrailingIdx - 4])) - inLow[BodyLongTrailingIdx - 4])) : 0.0)));
  i += 1;
  BodyLongTrailingIdx += 1;
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const capLag_BodyLongTrailingIdx = i - BodyLongTrailingIdx;
  const cap_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx + 5;
  if (capLag_BodyLongTrailingIdx < 0 || cap_BodyLongTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_BodyLongTrailingIdx = cap_BodyLongTrailingIdx > 0 ? cap_BodyLongTrailingIdx : 1;
  const capRing_BodyLongTrailingIdx_derived = new Float64Array(allocN_BodyLongTrailingIdx);
  for (let fillJ = historyLen - cap_BodyLongTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_BodyLongTrailingIdx_derived[fillJ % cap_BodyLongTrailingIdx] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyLong_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyLong_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  sp.BodyLongPeriodTotal = BodyLongPeriodTotal;
  sp.lag1_inOpen = inOpen[historyLen - 1];
  sp.lag2_inOpen = inOpen[historyLen - 2];
  sp.lag3_inOpen = inOpen[historyLen - 3];
  sp.lag4_inOpen = inOpen[historyLen - 4];
  sp.lag1_inHigh = inHigh[historyLen - 1];
  sp.lag2_inHigh = inHigh[historyLen - 2];
  sp.lag3_inHigh = inHigh[historyLen - 3];
  sp.lag4_inHigh = inHigh[historyLen - 4];
  sp.lag1_inLow = inLow[historyLen - 1];
  sp.lag2_inLow = inLow[historyLen - 2];
  sp.lag3_inLow = inLow[historyLen - 3];
  sp.lag4_inLow = inLow[historyLen - 4];
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.lag2_inClose = inClose[historyLen - 2];
  sp.lag3_inClose = inClose[historyLen - 3];
  sp.lag4_inClose = inClose[historyLen - 4];
  sp.ringPos_BodyLongTrailingIdx = historyLen % cap_BodyLongTrailingIdx;
  sp.ringCap_BodyLongTrailingIdx = cap_BodyLongTrailingIdx;
  sp.ringLag_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx;
  sp.ring_BodyLongTrailingIdx_derived = capRing_BodyLongTrailingIdx_derived;
  sp.cs_BodyLong_rangeType = BodyLong_rangeType;
  sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
  sp.cs_BodyLong_factor = BodyLong_factor;
  sp.cur_outInteger = outInteger[0];
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlbreakawayOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): CdlbreakawayStream {
  const sp = CdlbreakawayStream.of(core);
  const retCode = cdlbreakawayOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLBREAKAWAY openAndFill', inOpen.length, startIdx, cdlbreakawayLookbackPositional(core));
  }
  throw streamFailure('CDLBREAKAWAY openAndFill', retCode);
}

/** @internal */
export function cdlbreakawayOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): CdlbreakawayStream {
  const sp = CdlbreakawayStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdlbreakawayOpenSinkFrame(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLBREAKAWAY open', inOpen.length, startIdx, cdlbreakawayLookbackPositional(core));
  }
  throw streamFailure('CDLBREAKAWAY open', retCode);
}

/** @internal */
export function cdlbreakawayOpenPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): CdlbreakawayStream {
  requireArgument('CDLBREAKAWAY open', 'prices.open', inOpen);
  requireHistory('CDLBREAKAWAY open', 'prices.open', inOpen.length);
  requireArgument('CDLBREAKAWAY open', 'prices.high', inHigh);
  requireArgument('CDLBREAKAWAY open', 'prices.low', inLow);
  requireArgument('CDLBREAKAWAY open', 'prices.close', inClose);
  requireHistoryLength('CDLBREAKAWAY open', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLBREAKAWAY open', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLBREAKAWAY open', 'prices.close', inClose.length, inOpen.length);
  return cdlbreakawayOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/** @internal */
export function cdlbreakawayOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): CdlbreakawayStream {
  requireArgument('CDLBREAKAWAY openAndFill', 'prices.open', inOpen);
  requireHistory('CDLBREAKAWAY openAndFill', 'prices.open', inOpen.length);
  requireArgument('CDLBREAKAWAY openAndFill', 'prices.high', inHigh);
  requireArgument('CDLBREAKAWAY openAndFill', 'prices.low', inLow);
  requireArgument('CDLBREAKAWAY openAndFill', 'prices.close', inClose);
  const guardOutLen = openFillCount('CDLBREAKAWAY openAndFill', inOpen.length, cdlbreakawayLookbackPositional(core));
  requireHistoryLength('CDLBREAKAWAY openAndFill', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLBREAKAWAY openAndFill', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLBREAKAWAY openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('CDLBREAKAWAY openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('CDLBREAKAWAY openAndFill', 'outInteger', 'prices.open');
  }
  if (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CDLBREAKAWAY openAndFill', 'outInteger', 'prices.high');
  }
  if (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CDLBREAKAWAY openAndFill', 'outInteger', 'prices.low');
  }
  if (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('CDLBREAKAWAY openAndFill', 'outInteger', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLBREAKAWAY openAndFill', 'outInteger', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLBREAKAWAY openAndFill', 'outInteger', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLBREAKAWAY openAndFill', 'outInteger', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLBREAKAWAY openAndFill', 'outInteger', 'prices.close');
  }
  return cdlbreakawayOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}

/**
 * Opens a live CDLBREAKAWAY stream over the warm-up history: `value()` starts at
 * the last history bar, bit-identical to {@link cdlbreakaway} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlbreakawayOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlbreakawayLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input or histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdlbreakawayOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdlbreakawayOpen({ open, high, low, close });
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdlbreakawayOpen(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlbreakawayOptions): CdlbreakawayStream {
  requireArgument('CDLBREAKAWAY open', 'prices', prices);
  const o = optionsOf('CDLBREAKAWAY open', options);
  const core = coreOf('CDLBREAKAWAY open', o.core);
  return cdlbreakawayOpenPositional(prices.open, prices.high, prices.low, prices.close, core);
}

/**
 * {@link cdlbreakawayOpen} that also computes the outputs over the whole history
 * in the same pass, bit-identically to {@link cdlbreakaway}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CdlbreakawayOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cdlbreakaway} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlbreakawayLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, histories of different lengths or a buffer too short,
 *   of the wrong type or starting where another buffer or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdlbreakawayOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = cdlbreakawayOpenAndFill({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cdlbreakawayOpenAndFill(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlbreakawayOptions): CdlbreakawayResult & { stream: CdlbreakawayStream } {
  requireArgument('CDLBREAKAWAY openAndFill', 'prices', prices);
  const o = optionsOf('CDLBREAKAWAY openAndFill', options);
  const core = coreOf('CDLBREAKAWAY openAndFill', o.core);
  const capacity = fillCapacity(cdlbreakawayLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const stream = cdlbreakawayOpenAndFillPositional(prices.open, prices.high, prices.low, prices.close, outInteger, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}
