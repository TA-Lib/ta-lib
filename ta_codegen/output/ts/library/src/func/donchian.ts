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
 *  KL       Kevin Lin
 *  MF       Mario Fortier
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090526 KL     First version (issue #342).
 *  090326 MF     Drop optInLag; the window ends at the current bar.
 */

import { badSink, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function donchianLookbackPositional(optInTimePeriod: number, _core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  return optInTimePeriod - 1;
}

/** @internal */
export function donchianDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (donchianLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 3) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function donchianImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
): RetCode {
  let lowest = 0;
  let highest = 0;
  let tmpLow = 0;
  let tmpHigh = 0;
  let outIdx = 0;
  let nbInitialElementNeeded = 0;
  let trailingIdx = 0;
  let lowestIdx = 0;
  let highestIdx = 0;
  let today = 0;
  let i = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if ((outRealUpperBand.buffer === outRealMiddleBand.buffer && outRealUpperBand.byteOffset === outRealMiddleBand.byteOffset) || (outRealUpperBand.buffer === outRealLowerBand.buffer && outRealUpperBand.byteOffset === outRealLowerBand.byteOffset) || (outRealMiddleBand.buffer === outRealLowerBand.buffer && outRealMiddleBand.byteOffset === outRealLowerBand.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  /* Donchian Channels over the optInTimePeriod bars ending at the current
   * bar:
   *
   *    Upper  = Highest High of the window
   *    Lower  = Lowest  Low  of the window
   *    Middle = (Upper + Lower) / 2
   *
   * The window includes the current bar, matching every other library and
   * charting platform. A breakout rule compares the current bar against the
   * PREVIOUS bar's band, which is where the one-bar offset belongs.
   *
   * Upper/Middle/Lower are bit-identical to MAX(high,N)/MIDPRICE(N)/MIN(low,N).
   */
  /* Identify the minimum number of price bar needed
   * to identify at least one output over the specified
   * period.
   */
  nbInitialElementNeeded = optInTimePeriod - 1;
  /* Move up the start index if there is not
   * enough initial data.
   */
  if (startIdx < nbInitialElementNeeded) {
    startIdx = nbInitialElementNeeded;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  /* Proceed with the calculation for the requested range.
   * Note that this algorithm allows the input and
   * output to be the same buffer: every position written (outIdx) sits
   * at or below trailingIdx, the oldest position any later bar reads.
   *
   * The highest high and lowest low of the window are cached with their
   * indices; the window is rescanned only when a cached extremum drops
   * out of it (same approach as MIN/MAX/WILLR and MIDPRICE).
   */
  outIdx = 0;
  today = startIdx;
  trailingIdx = startIdx - nbInitialElementNeeded;
  highestIdx = -1;
  highest = 0.0;
  lowestIdx = -1;
  lowest = 0.0;
  while (today <= endIdx) {
    tmpHigh = inHigh[today];
    tmpLow = inLow[today];
    if (highestIdx < trailingIdx) {
      highestIdx = trailingIdx;
      highest = inHigh[highestIdx];
      i = highestIdx;
      while (++i <= today) {
        tmpHigh = inHigh[i];
        if (tmpHigh > highest) {
          highestIdx = i;
          highest = tmpHigh;
        }
      }
    } else if (tmpHigh >= highest) {
      highestIdx = today;
      highest = tmpHigh;
    }
    if (lowestIdx < trailingIdx) {
      lowestIdx = trailingIdx;
      lowest = inLow[lowestIdx];
      i = lowestIdx;
      while (++i <= today) {
        tmpLow = inLow[i];
        if (tmpLow < lowest) {
          lowestIdx = i;
          lowest = tmpLow;
        }
      }
    } else if (tmpLow <= lowest) {
      lowestIdx = today;
      lowest = tmpLow;
    }
    outRealUpperBand[outIdx] = highest;
    outRealLowerBand[outIdx] = lowest;
    outRealMiddleBand[outIdx] = (highest + lowest) / 2.0;
    outIdx += 1;
    trailingIdx += 1;
    today += 1;
  }
  /* Keep the outBegIdx relative to the
   * caller input before returning.
   */
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000']] as const;

/** @internal */
export function donchianPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('DONCHIAN', 'startIdx', startIdx);
  endIdx = requireInt32('DONCHIAN', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('DONCHIAN', 'timePeriod', optInTimePeriod);
  requireIndexRange('DONCHIAN', startIdx, endIdx);
  const guardLookback = donchianLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('DONCHIAN', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('DONCHIAN', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('DONCHIAN', 'prices.high', inHigh, guardInLen);
  requireLength('DONCHIAN', 'prices.low', inLow, guardInLen);
  requireOutput('DONCHIAN', 'outUpperBand', outRealUpperBand, guardOutLen, Float64Array);
  requireOutput('DONCHIAN', 'outMiddleBand', outRealMiddleBand, guardOutLen, Float64Array);
  requireOutput('DONCHIAN', 'outLowerBand', outRealLowerBand, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlapsPartly(outRealUpperBand, inHighF64)) {
    throw sharedMemory('DONCHIAN', 'outUpperBand', 'prices.high');
  }
  if (overlapsPartly(outRealUpperBand, inLowF64)) {
    throw sharedMemory('DONCHIAN', 'outUpperBand', 'prices.low');
  }
  if (overlapsPartly(outRealMiddleBand, inHighF64)) {
    throw sharedMemory('DONCHIAN', 'outMiddleBand', 'prices.high');
  }
  if (overlapsPartly(outRealMiddleBand, inLowF64)) {
    throw sharedMemory('DONCHIAN', 'outMiddleBand', 'prices.low');
  }
  if (overlapsPartly(outRealLowerBand, inHighF64)) {
    throw sharedMemory('DONCHIAN', 'outLowerBand', 'prices.high');
  }
  if (overlapsPartly(outRealLowerBand, inLowF64)) {
    throw sharedMemory('DONCHIAN', 'outLowerBand', 'prices.low');
  }
  if (overlaps(outRealUpperBand, outRealMiddleBand)) {
    throw sharedMemory('DONCHIAN', 'outUpperBand', 'outMiddleBand');
  }
  if (overlaps(outRealUpperBand, outRealLowerBand)) {
    throw sharedMemory('DONCHIAN', 'outUpperBand', 'outLowerBand');
  }
  if (overlaps(outRealMiddleBand, outRealLowerBand)) {
    throw sharedMemory('DONCHIAN', 'outMiddleBand', 'outLowerBand');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = donchianImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInTimePeriod, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('DONCHIAN', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link donchian}, {@link donchianLookback},
 * {@link donchianDisplayShift}, {@link donchianOpen} and
 * {@link donchianOpenAndFill}. Every key is optional: an omitted or `undefined`
 * key takes its default, and a call ignores the keys it has no use for.
 */
export interface DonchianOptions {
  /**
   * Number of bars in the extrema window (default 20; range 2 to 100000;
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
   * Buffer for `outUpperBand`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outUpperBand?: Float64Array<ArrayBuffer> | undefined;
  /**
   * Buffer for `outMiddleBand`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outMiddleBand?: Float64Array<ArrayBuffer> | undefined;
  /**
   * Buffer for `outLowerBand`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outLowerBand?: Float64Array<ArrayBuffer> | undefined;
}

/** What {@link donchian} returns, and {@link donchianOpenAndFill} with its stream. */
export interface DonchianResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Highest high of the window. `outUpperBand[k]` is the value at input bar
   * `begIdx + k`.
   */
  outUpperBand: Float64Array<ArrayBuffer>;
  /**
   * Midpoint of the upper and lower bands. `outMiddleBand[k]` is the value at input
   * bar `begIdx + k`.
   */
  outMiddleBand: Float64Array<ArrayBuffer>;
  /**
   * Lowest low of the window. `outLowerBand[k]` is the value at input bar
   * `begIdx + k`.
   */
  outLowerBand: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link donchian} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link DonchianOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function donchianLookback(options?: DonchianOptions): number {
  const o = optionsOf('DONCHIAN lookback', options);
  return donchianLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('DONCHIAN lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link donchian}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link DonchianOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function donchianDisplayShift(outputIdx: number, options?: DonchianOptions): number {
  const o = optionsOf('DONCHIAN displayShift', options);
  return donchianDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('DONCHIAN displayShift', o.core));
}

/**
 * Donchian Channels: three overlap lines built from rolling price extrema. The
 * upper band is the highest high and the lower band the lowest low over the
 * period; the middle line is their midpoint. Richard Donchian's original four-week
 * rule — generally credited as the first published systematic trend-following
 * system — buys a break above the high of the preceding weeks and sells a break
 * below their low.
 *
 * Formula and more info at
 * [ta-lib.org/functions/donchian](https://ta-lib.org/functions/donchian).
 *
 * **Notes**
 * - The window includes the current bar, matching TradingView
 *   (`ta.highest`/`ta.lowest`), NinjaTrader, ta4j, pandas-ta and every other
 *   library that ships Donchian Channels.
 * - A breakout rule compares the current bar against the **previous** bar's band —
 *   `High[t] > Upper[t-1]` — which is where the one-bar offset belongs. Reading
 *   `Upper[t]` against `High[t]` can never signal, because `High[t]` is inside the
 *   window that produced it.
 * - Upper, Middle and Lower are bit-identical to `MAX(high, N)`, `MIDPRICE(N)` and
 *   `MIN(low, N)`. DONCHIAN computes all three in one pass under the name users
 *   look for.
 * - The middle line is the channel midpoint, not a moving average of price.
 * - No smoothing or recursion is involved, so there is no unstable period: outputs
 *   are exact from the first bar.
 *
 * `outUpperBand[k]` is the value at input bar `begIdx + k`; every output is
 * exactly as long as the number of values written, and nothing is padded. A range
 * that ends before {@link donchianLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `high`: High price of
 *   each bar. `low`: Low price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link DonchianOptions}.
 * @returns `begIdx` and the values; see {@link DonchianResult}. Each output is a
 *   view of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, an integer parameter that is not an int32, a parameter outside its range,
 *   an input too short for the range, a buffer that is empty, too short or of the
 *   wrong type, two buffers sharing memory or a buffer sharing memory with a
 *   `Float64Array` input it is not. Checked before anything is written.
 *
 * @example
 * ```ts
 * import { donchian } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const {
 *   begIdx,
 *   outUpperBand,
 *   outMiddleBand,
 *   outLowerBand,
 * } = donchian({ high, low }, { timePeriod: 20 });
 * // outUpperBand[k] is the value at bar begIdx + k
 * ```
 */
export function donchian(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: DonchianOptions): DonchianResult {
  requireArgument('DONCHIAN', 'prices', prices);
  const o = optionsOf('DONCHIAN', options);
  if (o.endIdx === undefined) {
    requireSeries('DONCHIAN', 'prices.high', prices.high);
    requireSeries('DONCHIAN', 'prices.low', prices.low);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('DONCHIAN', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, donchianLookbackPositional(optInTimePeriod, core), prices.high, prices.low);
  const outRealUpperBand = o.outUpperBand === undefined ? new Float64Array(capacity) : o.outUpperBand;
  const outRealMiddleBand = o.outMiddleBand === undefined ? new Float64Array(capacity) : o.outMiddleBand;
  const outRealLowerBand = o.outLowerBand === undefined ? new Float64Array(capacity) : o.outLowerBand;
  const range = donchianPositional(startIdx, endIdx, prices.high, prices.low, optInTimePeriod, outRealUpperBand, outRealMiddleBand, outRealLowerBand, core);
  return { begIdx: range.begIdx, outUpperBand: outRealUpperBand.subarray(0, range.count), outMiddleBand: outRealMiddleBand.subarray(0, range.count), outLowerBand: outRealLowerBand.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live DONCHIAN stream: one value per closed bar, bit-identical to
 * {@link donchian} over the same series. Open it with {@link donchianOpen} or
 * {@link donchianOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/donchian](https://ta-lib.org/functions/donchian).
 */
export class DonchianStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  lowest: number = 0;
  /** @internal */
  highest: number = 0;
  /** @internal */
  trailingIdx: number = 0;
  /** @internal */
  lowestIdx: number = 0;
  /** @internal */
  highestIdx: number = 0;
  /** @internal */
  i: number = 0;
  /** @internal */
  today: number = 0;
  /** @internal */
  xMask: number = 0;
  /** @internal */
  x_inHigh: Float64Array = new Float64Array(1);
  /** @internal */
  x_inLow: Float64Array = new Float64Array(1);
  /** @internal */
  cur_outRealUpperBand: number = 0;
  /** @internal */
  cur_outRealMiddleBand: number = 0;
  /** @internal */
  cur_outRealLowerBand: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): DonchianStream {
    return new DonchianStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link donchian} reports them over the same bars. Each accepted `update` and
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
      throw streamFailure('DONCHIAN advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns its outputs.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param out - Receives the outputs and is returned; a new {@link DonchianOut}
   *   when omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included, before anything moves.
   *   `TALibError` with code `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached
   *   bar `Core.INDEX_MAX`.
   */
  update(high: number, low: number, out?: DonchianOut): DonchianOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('DONCHIAN update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('DONCHIAN update', 'DonchianOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low)) {
      throw nonFiniteBar('DONCHIAN update', !Number.isFinite(high) ? 'high' : 'low');
    }
    donchianStepImpl(this, high, low);
    this.outRangeCount++;
    if (out === undefined) {
      out = new DonchianOut();
    }
    out.outUpperBand = this.cur_outRealUpperBand;
    out.outMiddleBand = this.cur_outRealMiddleBand;
    out.outLowerBand = this.cur_outRealLowerBand;
    return out;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param out - Receives the outputs and is returned; a new {@link DonchianOut}
   *   when omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included.
   */
  peek(high: number, low: number, out?: DonchianOut): DonchianOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('DONCHIAN peek', 'DonchianOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low)) {
      throw nonFiniteBar('DONCHIAN peek', !Number.isFinite(high) ? 'high' : 'low');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
    let tmpLow = 0;
    let tmpHigh = 0;
    let cur_outRealLowerBand = 0;
    let cur_outRealMiddleBand = 0;
    let cur_outRealUpperBand = 0;
    let highest = sp.highest;
    let highestIdx = sp.highestIdx;
    let i = sp.i;
    let lowest = sp.lowest;
    let lowestIdx = sp.lowestIdx;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    let pkSlot1 = -1;
    let pkVal1 = 0;
    pkSlot0 = sp.today & sp.xMask;
    pkVal0 = inHigh;
    pkSlot1 = sp.today & sp.xMask;
    pkVal1 = inLow;
    tmpHigh = ((sp.today & sp.xMask) !== pkSlot0) ? sp.x_inHigh[sp.today & sp.xMask] : pkVal0;
    tmpLow = ((sp.today & sp.xMask) !== pkSlot1) ? sp.x_inLow[sp.today & sp.xMask] : pkVal1;
    if (highestIdx < sp.trailingIdx) {
      highestIdx = sp.trailingIdx;
      highest = ((highestIdx & sp.xMask) !== pkSlot0) ? sp.x_inHigh[highestIdx & sp.xMask] : pkVal0;
      i = highestIdx;
      while (++i <= sp.today) {
        tmpHigh = ((i & sp.xMask) !== pkSlot0) ? sp.x_inHigh[i & sp.xMask] : pkVal0;
        if (tmpHigh > highest) {
          highestIdx = i;
          highest = tmpHigh;
        }
      }
    } else if (tmpHigh >= highest) {
      highestIdx = sp.today;
      highest = tmpHigh;
    }
    if (lowestIdx < sp.trailingIdx) {
      lowestIdx = sp.trailingIdx;
      lowest = ((lowestIdx & sp.xMask) !== pkSlot1) ? sp.x_inLow[lowestIdx & sp.xMask] : pkVal1;
      i = lowestIdx;
      while (++i <= sp.today) {
        tmpLow = ((i & sp.xMask) !== pkSlot1) ? sp.x_inLow[i & sp.xMask] : pkVal1;
        if (tmpLow < lowest) {
          lowestIdx = i;
          lowest = tmpLow;
        }
      }
    } else if (tmpLow <= lowest) {
      lowestIdx = sp.today;
      lowest = tmpLow;
    }
    cur_outRealUpperBand = highest;
    cur_outRealLowerBand = lowest;
    cur_outRealMiddleBand = (highest + lowest) / 2.0;
    if (out === undefined) {
      out = new DonchianOut();
    }
    out.outUpperBand = cur_outRealUpperBand;
    out.outMiddleBand = cur_outRealMiddleBand;
    out.outLowerBand = cur_outRealLowerBand;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link DonchianOut}
   *   when omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a sink that is not an object,
   *   `null` included.
   */
  value(out?: DonchianOut): DonchianOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('DONCHIAN value', 'DonchianOut');
    }
    if (out === undefined) {
      out = new DonchianOut();
    }
    out.outUpperBand = this.cur_outRealUpperBand;
    out.outMiddleBand = this.cur_outRealMiddleBand;
    out.outLowerBand = this.cur_outRealLowerBand;
    return out;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): DonchianStream {
    const c = new DonchianStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.lowest = this.lowest;
    c.highest = this.highest;
    c.trailingIdx = this.trailingIdx;
    c.lowestIdx = this.lowestIdx;
    c.highestIdx = this.highestIdx;
    c.i = this.i;
    c.today = this.today;
    c.xMask = this.xMask;
    c.x_inHigh = this.x_inHigh.slice();
    c.x_inLow = this.x_inLow.slice();
    c.cur_outRealUpperBand = this.cur_outRealUpperBand;
    c.cur_outRealMiddleBand = this.cur_outRealMiddleBand;
    c.cur_outRealLowerBand = this.cur_outRealLowerBand;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one DONCHIAN bar, as `update`, `peek` and `value` return them.
 * Passed back to one of them, it is filled and returned again, with no allocation;
 * it holds its values until the next call that writes it. A new one holds `NaN`.
 */
export class DonchianOut {
  /** Highest high of the window. */
  outUpperBand: number = NaN;
  /** Midpoint of the upper and lower bands. */
  outMiddleBand: number = NaN;
  /** Lowest low of the window. */
  outLowerBand: number = NaN;
}

function donchianStepImpl(sp: DonchianStream, inHigh: number, inLow: number): void {
  let tmpLow = 0;
  let tmpHigh = 0;
  sp.x_inHigh[sp.today & sp.xMask] = inHigh;
  sp.x_inLow[sp.today & sp.xMask] = inLow;
  tmpHigh = sp.x_inHigh[sp.today & sp.xMask];
  tmpLow = sp.x_inLow[sp.today & sp.xMask];
  if (sp.highestIdx < sp.trailingIdx) {
    sp.highestIdx = sp.trailingIdx;
    sp.highest = sp.x_inHigh[sp.highestIdx & sp.xMask];
    sp.i = sp.highestIdx;
    while (++sp.i <= sp.today) {
      tmpHigh = sp.x_inHigh[sp.i & sp.xMask];
      if (tmpHigh > sp.highest) {
        sp.highestIdx = sp.i;
        sp.highest = tmpHigh;
      }
    }
  } else if (tmpHigh >= sp.highest) {
    sp.highestIdx = sp.today;
    sp.highest = tmpHigh;
  }
  if (sp.lowestIdx < sp.trailingIdx) {
    sp.lowestIdx = sp.trailingIdx;
    sp.lowest = sp.x_inLow[sp.lowestIdx & sp.xMask];
    sp.i = sp.lowestIdx;
    while (++sp.i <= sp.today) {
      tmpLow = sp.x_inLow[sp.i & sp.xMask];
      if (tmpLow < sp.lowest) {
        sp.lowestIdx = sp.i;
        sp.lowest = tmpLow;
      }
    }
  } else if (tmpLow <= sp.lowest) {
    sp.lowestIdx = sp.today;
    sp.lowest = tmpLow;
  }
  sp.cur_outRealUpperBand = sp.highest;
  sp.cur_outRealLowerBand = sp.lowest;
  sp.cur_outRealMiddleBand = (sp.highest + sp.lowest) / 2.0;
  sp.trailingIdx += 1;
  sp.today += 1;
}

function donchianOpenImpl(
  sp: DonchianStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
  outStride: number,
): RetCode {
  let lowest = 0;
  let highest = 0;
  let tmpLow = 0;
  let tmpHigh = 0;
  let outIdx = 0;
  let nbInitialElementNeeded = 0;
  let trailingIdx = 0;
  let lowestIdx = 0;
  let highestIdx = 0;
  let today = 0;
  let i = 0;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Donchian Channels over the optInTimePeriod bars ending at the current
   * bar:
   *
   *    Upper  = Highest High of the window
   *    Lower  = Lowest  Low  of the window
   *    Middle = (Upper + Lower) / 2
   *
   * The window includes the current bar, matching every other library and
   * charting platform. A breakout rule compares the current bar against the
   * PREVIOUS bar's band, which is where the one-bar offset belongs.
   *
   * Upper/Middle/Lower are bit-identical to MAX(high,N)/MIDPRICE(N)/MIN(low,N).
   */
  /* Identify the minimum number of price bar needed
   * to identify at least one output over the specified
   * period.
   */
  nbInitialElementNeeded = optInTimePeriod - 1;
  /* Move up the start index if there is not
   * enough initial data.
   */
  if (startIdx < nbInitialElementNeeded) {
    startIdx = nbInitialElementNeeded;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Proceed with the calculation for the requested range.
   * Note that this algorithm allows the input and
   * output to be the same buffer: every position written (outIdx) sits
   * at or below trailingIdx, the oldest position any later bar reads.
   *
   * The highest high and lowest low of the window are cached with their
   * indices; the window is rescanned only when a cached extremum drops
   * out of it (same approach as MIN/MAX/WILLR and MIDPRICE).
   */
  outIdx = 0;
  today = startIdx;
  trailingIdx = startIdx - nbInitialElementNeeded;
  highestIdx = -1;
  highest = 0.0;
  lowestIdx = -1;
  lowest = 0.0;
  while (today <= endIdx) {
    tmpHigh = inHigh[today];
    tmpLow = inLow[today];
    if (highestIdx < trailingIdx) {
      highestIdx = trailingIdx;
      highest = inHigh[highestIdx];
      i = highestIdx;
      while (++i <= today) {
        tmpHigh = inHigh[i];
        if (tmpHigh > highest) {
          highestIdx = i;
          highest = tmpHigh;
        }
      }
    } else if (tmpHigh >= highest) {
      highestIdx = today;
      highest = tmpHigh;
    }
    if (lowestIdx < trailingIdx) {
      lowestIdx = trailingIdx;
      lowest = inLow[lowestIdx];
      i = lowestIdx;
      while (++i <= today) {
        tmpLow = inLow[i];
        if (tmpLow < lowest) {
          lowestIdx = i;
          lowest = tmpLow;
        }
      }
    } else if (tmpLow <= lowest) {
      lowestIdx = today;
      lowest = tmpLow;
    }
    outRealUpperBand[((outIdx * outStride) | 0)] = highest;
    outRealLowerBand[((outIdx * outStride) | 0)] = lowest;
    outRealMiddleBand[((outIdx * outStride) | 0)] = (highest + lowest) / 2.0;
    outIdx += 1;
    trailingIdx += 1;
    today += 1;
  }
  /* Keep the outBegIdx relative to the
   * caller input before returning.
   */
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  const capX = today - trailingIdx + 1;
  if (capX < 1 || capX > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  let physX = 1;
  while (physX < capX) {
    physX <<= 1;
  }
  const capX_inHigh = new Float64Array(physX);
  const capX_inLow = new Float64Array(physX);
  for (let fillJ = historyLen - capX; fillJ < historyLen; fillJ++) {
    capX_inHigh[fillJ & (physX - 1)] = inHigh[fillJ];
    capX_inLow[fillJ & (physX - 1)] = inLow[fillJ];
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.lowest = lowest;
  sp.highest = highest;
  sp.trailingIdx = trailingIdx;
  sp.lowestIdx = lowestIdx;
  sp.highestIdx = highestIdx;
  sp.i = i;
  sp.today = today;
  sp.xMask = physX - 1;
  sp.x_inHigh = capX_inHigh;
  sp.x_inLow = capX_inLow;
  sp.cur_outRealUpperBand = outRealUpperBand[(outNBElement.value - 1) * outStride];
  sp.cur_outRealMiddleBand = outRealMiddleBand[(outNBElement.value - 1) * outStride];
  sp.cur_outRealLowerBand = outRealLowerBand[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function donchianOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
): DonchianStream {
  const sp = DonchianStream.of(core);
  const retCode = donchianOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('DONCHIAN openAndFill', inHigh.length, startIdx, donchianLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && donchianLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('DONCHIAN openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('DONCHIAN openAndFill', retCode);
}

/** @internal */
export function donchianOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): DonchianStream {
  const sp = DonchianStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outRealUpperBand = new Float64Array(1);
  const sink_outRealMiddleBand = new Float64Array(1);
  const sink_outRealLowerBand = new Float64Array(1);
  const retCode = donchianOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outRealUpperBand, sink_outRealMiddleBand, sink_outRealLowerBand, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('DONCHIAN open', inHigh.length, startIdx, donchianLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && donchianLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('DONCHIAN open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('DONCHIAN open', retCode);
}

/** @internal */
export function donchianOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): DonchianStream {
  optInTimePeriod = requireInt32('DONCHIAN open', 'timePeriod', optInTimePeriod);
  requireArgument('DONCHIAN open', 'prices.high', inHigh);
  requireHistory('DONCHIAN open', 'prices.high', inHigh.length);
  requireArgument('DONCHIAN open', 'prices.low', inLow);
  requireHistoryLength('DONCHIAN open', 'prices.low', inLow.length, inHigh.length);
  return donchianOpenInternal(core, float64(inHigh), float64(inLow), 0, optInTimePeriod);
}

/** @internal */
export function donchianOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
  core: Core = Core.DEFAULT,
): DonchianStream {
  optInTimePeriod = requireInt32('DONCHIAN openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('DONCHIAN openAndFill', 'prices.high', inHigh);
  requireHistory('DONCHIAN openAndFill', 'prices.high', inHigh.length);
  requireArgument('DONCHIAN openAndFill', 'prices.low', inLow);
  const guardLookback = donchianLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('DONCHIAN openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('DONCHIAN openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('DONCHIAN openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireOutput('DONCHIAN openAndFill', 'outUpperBand', outRealUpperBand, guardOutLen, Float64Array);
  requireOutput('DONCHIAN openAndFill', 'outMiddleBand', outRealMiddleBand, guardOutLen, Float64Array);
  requireOutput('DONCHIAN openAndFill', 'outLowerBand', outRealLowerBand, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (outRealUpperBand.buffer === inHighF64.buffer && outRealUpperBand.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('DONCHIAN openAndFill', 'outUpperBand', 'prices.high');
  }
  if (outRealUpperBand.buffer === inLowF64.buffer && outRealUpperBand.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('DONCHIAN openAndFill', 'outUpperBand', 'prices.low');
  }
  if (outRealMiddleBand.buffer === inHighF64.buffer && outRealMiddleBand.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('DONCHIAN openAndFill', 'outMiddleBand', 'prices.high');
  }
  if (outRealMiddleBand.buffer === inLowF64.buffer && outRealMiddleBand.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('DONCHIAN openAndFill', 'outMiddleBand', 'prices.low');
  }
  if (outRealLowerBand.buffer === inHighF64.buffer && outRealLowerBand.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('DONCHIAN openAndFill', 'outLowerBand', 'prices.high');
  }
  if (outRealLowerBand.buffer === inLowF64.buffer && outRealLowerBand.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('DONCHIAN openAndFill', 'outLowerBand', 'prices.low');
  }
  if (outRealUpperBand.buffer === outRealMiddleBand.buffer && outRealUpperBand.byteOffset === outRealMiddleBand.byteOffset) {
    throw sharedMemory('DONCHIAN openAndFill', 'outUpperBand', 'outMiddleBand');
  }
  if (outRealUpperBand.buffer === outRealLowerBand.buffer && outRealUpperBand.byteOffset === outRealLowerBand.byteOffset) {
    throw sharedMemory('DONCHIAN openAndFill', 'outUpperBand', 'outLowerBand');
  }
  if (outRealMiddleBand.buffer === outRealLowerBand.buffer && outRealMiddleBand.byteOffset === outRealLowerBand.byteOffset) {
    throw sharedMemory('DONCHIAN openAndFill', 'outMiddleBand', 'outLowerBand');
  }
  if (guardOutLen > 0 && overlaps(outRealUpperBand, inHighF64)) {
    throw sharedMemory('DONCHIAN openAndFill', 'outUpperBand', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outRealUpperBand, inLowF64)) {
    throw sharedMemory('DONCHIAN openAndFill', 'outUpperBand', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outRealMiddleBand, inHighF64)) {
    throw sharedMemory('DONCHIAN openAndFill', 'outMiddleBand', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outRealMiddleBand, inLowF64)) {
    throw sharedMemory('DONCHIAN openAndFill', 'outMiddleBand', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outRealLowerBand, inHighF64)) {
    throw sharedMemory('DONCHIAN openAndFill', 'outLowerBand', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outRealLowerBand, inLowF64)) {
    throw sharedMemory('DONCHIAN openAndFill', 'outLowerBand', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outRealUpperBand, outRealMiddleBand)) {
    throw sharedMemory('DONCHIAN openAndFill', 'outUpperBand', 'outMiddleBand');
  }
  if (guardOutLen > 0 && overlaps(outRealUpperBand, outRealLowerBand)) {
    throw sharedMemory('DONCHIAN openAndFill', 'outUpperBand', 'outLowerBand');
  }
  if (guardOutLen > 0 && overlaps(outRealMiddleBand, outRealLowerBand)) {
    throw sharedMemory('DONCHIAN openAndFill', 'outMiddleBand', 'outLowerBand');
  }
  return donchianOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outRealUpperBand, outRealMiddleBand, outRealLowerBand);
}

/**
 * Opens a live DONCHIAN stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link donchian} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link DonchianOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `donchianLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range
 *   or histories of different lengths.
 *
 * @example
 * ```ts
 * import { donchianOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = donchianOpen({ high, low }, { timePeriod: 20 });
 * const peeked = stream.peek(101.4, 99.1); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1); // the bar closes
 * ```
 */
export function donchianOpen(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: DonchianOptions): DonchianStream {
  requireArgument('DONCHIAN open', 'prices', prices);
  const o = optionsOf('DONCHIAN open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('DONCHIAN open', o.core);
  return donchianOpenPositional(prices.high, prices.low, optInTimePeriod, core);
}

/**
 * {@link donchianOpen} that also computes the outputs over the whole history in
 * the same pass, bit-identically to {@link donchian}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link DonchianOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link donchian} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `donchianLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { donchianOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const {
 *   stream,
 *   begIdx,
 *   outUpperBand,
 *   outMiddleBand,
 *   outLowerBand,
 * } = donchianOpenAndFill({ high, low }, { timePeriod: 20 });
 * // outUpperBand[k] is the value at bar begIdx + k
 * ```
 */
export function donchianOpenAndFill(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: DonchianOptions): DonchianResult & { stream: DonchianStream } {
  requireArgument('DONCHIAN openAndFill', 'prices', prices);
  const o = optionsOf('DONCHIAN openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('DONCHIAN openAndFill', o.core);
  const capacity = fillCapacity(donchianLookbackPositional(optInTimePeriod, core), prices.high, prices.low);
  const outRealUpperBand = o.outUpperBand === undefined ? new Float64Array(capacity) : o.outUpperBand;
  const outRealMiddleBand = o.outMiddleBand === undefined ? new Float64Array(capacity) : o.outMiddleBand;
  const outRealLowerBand = o.outLowerBand === undefined ? new Float64Array(capacity) : o.outLowerBand;
  const stream = donchianOpenAndFillPositional(prices.high, prices.low, optInTimePeriod, outRealUpperBand, outRealMiddleBand, outRealLowerBand, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outUpperBand: outRealUpperBand.subarray(0, range.count), outMiddleBand: outRealMiddleBand.subarray(0, range.count), outLowerBand: outRealLowerBand.subarray(0, range.count) };
}
