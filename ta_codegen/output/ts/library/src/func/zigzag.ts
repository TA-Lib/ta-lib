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
 *  MF       Mario Fortier
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  100826 KL,CC  Creation (#487).
 *  101126 MF,CC  The extension step is a select; the pivot index is derived (#546).
 */

import { badSink, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireReal, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function zigzagLookbackPositional(optInSensitivity: number, optInMinTrendLength: number, _core: Core = Core.DEFAULT): number {
  if (typeof optInSensitivity !== 'number') {
    return -1;
  }
  if ((optInMinTrendLength | 0) !== optInMinTrendLength) {
    return -1;
  }
  if (optInSensitivity === Core.REAL_DEFAULT) {
    optInSensitivity = 5.0;
  } else if (!(optInSensitivity >= 0e0 && optInSensitivity <= 1e2)) {
    return -1;
  }
  if (optInMinTrendLength === Core.INTEGER_DEFAULT) {
    optInMinTrendLength = 1;
  } else if (optInMinTrendLength < 1 || optInMinTrendLength > 100000) {
    return -1;
  }
  /* The first bar at which a reversal off the seed can fire. The gate counts
   * from the pivot's OWN bar and the seed is a pivot like any other, so the
   * earliest reversal is optInMinTrendLength bars after the seed, which sits
   * that many bars before the first output.
   *
   * Independent of the sensitivity: the threshold decides WHETHER a reversal
   * fires, never how early it may.
   */
  return optInMinTrendLength;
}

/** @internal */
export function zigzagDisplayShiftPositional(optInSensitivity: number, optInMinTrendLength: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (zigzagLookbackPositional(optInSensitivity, optInMinTrendLength, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 3) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function zigzagImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  optInSensitivity: number,
  optInMinTrendLength: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outZigZag: Float64Array,
  outTrend: Int32Array,
  outPivotIdx: Int32Array,
): RetCode {
  let lookbackTotal = 0;
  let today = 0;
  let outIdx = 0;
  let trend = 0;
  let barsSincePivot = 0;
  let barIdx = 0;
  let isExt = 0;
  let s = 0;
  let up = 0;
  let dn = 0;
  let pivot = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInSensitivity === Core.REAL_DEFAULT) {
    optInSensitivity = 5.0;
  } else if (!(optInSensitivity >= 0e0 && optInSensitivity <= 1e2)) {
    return RetCode.BAD_PARAM;
  }
  if (optInMinTrendLength === Core.INTEGER_DEFAULT) {
    optInMinTrendLength = 1;
  } else if (optInMinTrendLength < 1 || optInMinTrendLength > 100000) {
    return RetCode.BAD_PARAM;
  }
  if ((outZigZag.buffer === outTrend.buffer && outZigZag.byteOffset === outTrend.byteOffset) || (outZigZag.buffer === outPivotIdx.buffer && outZigZag.byteOffset === outPivotIdx.byteOffset) || (outTrend.buffer === outPivotIdx.buffer && outTrend.byteOffset === outPivotIdx.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  lookbackTotal = zigzagLookbackPositional(optInSensitivity, optInMinTrendLength, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  /* Percent in, fraction inside. Each factor is rounded once, here, rather
   * than rebuilt per bar: the threshold is `pivot * up` in binary64, which is
   * the expression order the oracle uses.
   */
  s = optInSensitivity / 100.0;
  up = 1.0 + s;
  dn = 1.0 - s;
  /* The seed: a down leg whose extreme is the low of the bar
   * optInMinTrendLength before the first output. Because the gate counts the
   * seed bar like every other pivot, the first reversal can fire exactly at
   * the first output bar, not one later.
   */
  today = startIdx - lookbackTotal;
  trend = -1;
  pivot = inLow[today];
  barIdx = today;
  barsSincePivot = 0;
  today = today + 1;
  outIdx = 0;
  while (today <= endIdx) {
    /* The pivot's bar is barIdx - barsSincePivot. Both are carried: a stream
     * has no cursor to read a bar number from.
     */
    barsSincePivot = barsSincePivot + 1;
    barIdx = barIdx + 1;
    /* Reversal is tested BEFORE extension. On an outside bar that both makes
     * a new extreme and clears the threshold, the leg reverses and the old
     * pivot stays where it was; testing extension first would move the pivot
     * and lose the reversal, which is a whole leg of difference rather than a
     * rounding.
     */
    if (trend === -1) {
      if (inHigh[today] >= pivot * up && barsSincePivot >= optInMinTrendLength) {
        trend = 1;
        pivot = inHigh[today];
        barsSincePivot = 0;
      } else {
        /* Ties extend: at an equal price the pivot moves to the later bar,
         * which only outPivotIdx shows.
         *
         * A select, not a branch: on moving prices the extension is taken
         * on a share of the bars no predictor follows.
         */
        isExt = (inLow[today] <= pivot ? 1 : 0);
        pivot = (pivot < inLow[today]) ? pivot : inLow[today];
        barsSincePivot = barsSincePivot - ((isExt * barsSincePivot) | 0);
      }
    } else if (inLow[today] <= pivot * dn && barsSincePivot >= optInMinTrendLength) {
      trend = -1;
      pivot = inLow[today];
      barsSincePivot = 0;
    } else {
      isExt = (inHigh[today] >= pivot ? 1 : 0);
      pivot = (pivot > inHigh[today]) ? pivot : inHigh[today];
      barsSincePivot = barsSincePivot - ((isExt * barsSincePivot) | 0);
    }
    if (today >= startIdx) {
      outZigZag[outIdx] = pivot;
      outTrend[outIdx] = trend;
      outPivotIdx[outIdx] = barIdx - barsSincePivot;
      outIdx = outIdx + 1;
    }
    today = today + 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['sensitivity', Core.REAL_DEFAULT, 0e0, 1e2, 'in 0..100'], ['minTrendLength', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000']] as const;

/** @internal */
export function zigzagPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInSensitivity: number,
  optInMinTrendLength: number,
  outZigZag: Float64Array,
  outTrend: Int32Array,
  outPivotIdx: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('ZIGZAG', 'startIdx', startIdx);
  endIdx = requireInt32('ZIGZAG', 'endIdx', endIdx);
  optInSensitivity = requireReal('ZIGZAG', 'sensitivity', optInSensitivity);
  optInMinTrendLength = requireInt32('ZIGZAG', 'minTrendLength', optInMinTrendLength);
  requireIndexRange('ZIGZAG', startIdx, endIdx);
  const guardLookback = zigzagLookbackPositional(optInSensitivity, optInMinTrendLength, core);
  if (guardLookback < 0) {
    throw rejectedParams('ZIGZAG', PARAM_RANGES, optInSensitivity, optInMinTrendLength);
  }
  const guardStart = clampedStart('ZIGZAG', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('ZIGZAG', 'prices.high', inHigh, guardInLen);
  requireLength('ZIGZAG', 'prices.low', inLow, guardInLen);
  requireOutput('ZIGZAG', 'outZigZag', outZigZag, guardOutLen, Float64Array);
  requireOutput('ZIGZAG', 'outTrend', outTrend, guardOutLen, Int32Array);
  requireOutput('ZIGZAG', 'outPivotIdx', outPivotIdx, guardOutLen, Int32Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlapsPartly(outZigZag, inHighF64)) {
    throw sharedMemory('ZIGZAG', 'outZigZag', 'prices.high');
  }
  if (overlapsPartly(outZigZag, inLowF64)) {
    throw sharedMemory('ZIGZAG', 'outZigZag', 'prices.low');
  }
  if (overlaps(outTrend, inHighF64)) {
    throw sharedMemory('ZIGZAG', 'outTrend', 'prices.high');
  }
  if (overlaps(outTrend, inLowF64)) {
    throw sharedMemory('ZIGZAG', 'outTrend', 'prices.low');
  }
  if (overlaps(outPivotIdx, inHighF64)) {
    throw sharedMemory('ZIGZAG', 'outPivotIdx', 'prices.high');
  }
  if (overlaps(outPivotIdx, inLowF64)) {
    throw sharedMemory('ZIGZAG', 'outPivotIdx', 'prices.low');
  }
  if (overlaps(outZigZag, outTrend)) {
    throw sharedMemory('ZIGZAG', 'outZigZag', 'outTrend');
  }
  if (overlaps(outZigZag, outPivotIdx)) {
    throw sharedMemory('ZIGZAG', 'outZigZag', 'outPivotIdx');
  }
  if (overlaps(outTrend, outPivotIdx)) {
    throw sharedMemory('ZIGZAG', 'outTrend', 'outPivotIdx');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = zigzagImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInSensitivity, optInMinTrendLength, outBegIdx, outNBElement, outZigZag, outTrend, outPivotIdx);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('ZIGZAG', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link zigzag}, {@link zigzagLookback}, {@link zigzagDisplayShift},
 * {@link zigzagOpen} and {@link zigzagOpenAndFill}. Every key is optional: an
 * omitted or `undefined` key takes its default, and a call ignores the keys it has
 * no use for.
 */
export interface ZigzagOptions {
  /**
   * Minimum move away from the current extreme that reverses the leg, in percent
   * (default 5; range 0 to 100; `Core.REAL_DEFAULT` selects the default).
   */
  sensitivity?: number | undefined;
  /**
   * Minimum number of bars between two pivots (default 1; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  minTrendLength?: number | undefined;
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
   * Buffer for `outZigZag`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outZigZag?: Float64Array<ArrayBuffer> | undefined;
  /**
   * Buffer for `outTrend`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input.
   */
  outTrend?: Int32Array<ArrayBuffer> | undefined;
  /**
   * Buffer for `outPivotIdx`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input.
   */
  outPivotIdx?: Int32Array<ArrayBuffer> | undefined;
}

/** What {@link zigzag} returns, and {@link zigzagOpenAndFill} with its stream. */
export interface ZigzagResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Price of the swing extreme currently being tracked. `outZigZag[k]` is the value
   * at input bar `begIdx + k`.
   */
  outZigZag: Float64Array<ArrayBuffer>;
  /**
   * +1 while the leg is up, -1 while it is down. `outTrend[k]` is the value at input
   * bar `begIdx + k`.
   */
  outTrend: Int32Array<ArrayBuffer>;
  /**
   * Absolute index, into the input, of the bar that extreme sits on.
   * `outPivotIdx[k]` is the value at input bar `begIdx + k`.
   */
  outPivotIdx: Int32Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link zigzag} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link ZigzagOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function zigzagLookback(options?: ZigzagOptions): number {
  const o = optionsOf('ZIGZAG lookback', options);
  return zigzagLookbackPositional(orDefault(o.sensitivity, Core.REAL_DEFAULT), orDefault(o.minTrendLength, Core.INTEGER_DEFAULT), coreOf('ZIGZAG lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link zigzag}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link ZigzagOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function zigzagDisplayShift(outputIdx: number, options?: ZigzagOptions): number {
  const o = optionsOf('ZIGZAG displayShift', options);
  return zigzagDisplayShiftPositional(orDefault(o.sensitivity, Core.REAL_DEFAULT), orDefault(o.minTrendLength, Core.INTEGER_DEFAULT), outputIdx, coreOf('ZIGZAG displayShift', o.core));
}

/**
 * The percent-filter swing line. A leg runs from the last confirmed pivot to the
 * most extreme price since it, and it reverses only when price moves at least
 * `sensitivity` percent away from that extreme and at least `minTrendLength` bars
 * have passed since the pivot's own bar. Achelis describes the chart form and
 * warns that *"the last 'leg' displayed in a Zig Zag chart can change"*. This
 * function does not emit the chart line, which is drawn with hindsight. It emits
 * the causal state the line is drawn from, one row per bar and final once emitted:
 * the price of the swing extreme currently being tracked, the absolute bar index
 * of that extreme, and whether the current leg is up or down. The chart is rebuilt
 * exactly from the outputs and the seed. A pivot is confirmed at bar `i > begIdx`
 * whenever `outTrend[i]` differs from `outTrend[i-1]`, and that pivot is
 * `(outPivotIdx[i-1], outZigZag[i-1])`. When `outTrend[begIdx]` is +1 a reversal
 * fired on the first output bar, and the pivot it confirmed is the seed,
 * `(begIdx - minTrendLength, low[begIdx - minTrendLength])`, which no output row
 * carries. The last, still-open pivot is `(outPivotIdx[last], outZigZag[last])` —
 * the leg Achelis warns about.
 *
 * Formula and more info at
 * [ta-lib.org/functions/zigzag](https://ta-lib.org/functions/zigzag).
 *
 * **Notes**
 * - **Reversal is tested before extension.** On an outside bar that both makes a
 *   new extreme and clears the threshold, the leg reverses and the old pivot stays
 *   where it was. The other order moves the pivot to the new extreme and defers
 *   the reversal to the next bar, against a threshold that has just moved further
 *   away: on the 252-bar regtest corpus at 5 percent that costs one bar (R TTR,
 *   which takes that order, places the swing low at 148 where this places it at
 *   147), but a range narrower than the sensitivity after the outside bar keeps it
 *   from ever firing, and the leg is then lost rather than delayed.
 * - **Ties extend.** The comparisons are `<=` and `>=`, so at an equal price the
 *   pivot moves to the later bar: `outZigZag` does not change and `outPivotIdx`
 *   does. That movement is the only thing the index output carries and the price
 *   output cannot.
 * - **The gate counts from the pivot's own bar, the seed included.** No reversal
 *   can fire before the first output bar; the first one can fire exactly there.
 * - **The seed is a low.** The first reversal is therefore always upward. A down
 *   reversal compares a bar's low with the tracked high, so a rising series stays
 *   on its up leg only while no low falls `sensitivity` percent under that high on
 *   a bar the gate allows.
 * - **The input must be positive.** The threshold is a percent of the pivot: at a
 *   pivot of 0 it sits on the pivot and below 0 on the wrong side of it, so the
 *   leg reverses on nearly every bar the gate allows. The output is undefined for
 *   such input and no error is returned. A series that crosses zero, such as an
 *   oscillator or a spread, needs an absolute reversal amount, which this function
 *   does not have.
 * - The lookback is `minTrendLength` and does not depend on the sensitivity: the
 *   threshold decides whether a reversal fires, never how early it may.
 * - `sensitivity` is a percent, as Achelis, StockCharts, TTR, Skender and
 *   TradingView take it. 0 and 100 are degenerate but defined: at 0 a reversal
 *   fires on almost every bar the gate allows, and at 100 no downward reversal
 *   fires on positive prices.
 * - The value at a bar depends on where the caller started, through the seed; the
 *   function is `path_dependent` for that reason, as SAR and SUPERTREND are.
 *
 * `outZigZag[k]` is the value at input bar `begIdx + k`; every output is exactly
 * as long as the number of values written, and nothing is padded. A range that
 * ends before {@link zigzagLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `high`: High price of
 *   each bar. `low`: Low price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link ZigzagOptions}.
 * @returns `begIdx` and the values; see {@link ZigzagResult}. Each output is a
 *   view of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, an integer parameter that is not an int32, a real parameter that is not a
 *   number, a parameter outside its range, an input too short for the range, a
 *   buffer that is empty, too short or of the wrong type, two buffers sharing memory
 *   or a buffer sharing memory with a `Float64Array` input it is not. Checked before
 *   anything is written.
 *
 * @example
 * ```ts
 * import { zigzag } from 'ta-lib';
 *
 * const high = new Float64Array(252); // your data, oldest first
 * const low = new Float64Array(252); // your data, oldest first
 *
 * const {
 *   begIdx,
 *   outZigZag,
 *   outTrend,
 *   outPivotIdx,
 * } = zigzag({ high, low }, { sensitivity: 5, minTrendLength: 1 });
 * console.log(outZigZag.length + ' values from bar ' + begIdx + '; last outZigZag = ' + outZigZag[outZigZag.length - 1]);
 * ```
 * @see [`sar`](https://ta-lib.org/functions/sar)
 * @see [`supertrend`](https://ta-lib.org/functions/supertrend)
 * @see [`minmaxindex`](https://ta-lib.org/functions/minmaxindex)
 * @see [`midprice`](https://ta-lib.org/functions/midprice)
 */
export function zigzag(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: ZigzagOptions): ZigzagResult {
  requireArgument('ZIGZAG', 'prices', prices);
  const o = optionsOf('ZIGZAG', options);
  if (o.endIdx === undefined) {
    requireSeries('ZIGZAG', 'prices.high', prices.high);
    requireSeries('ZIGZAG', 'prices.low', prices.low);
  }
  const optInSensitivity = orDefault(o.sensitivity, Core.REAL_DEFAULT);
  const optInMinTrendLength = orDefault(o.minTrendLength, Core.INTEGER_DEFAULT);
  const core = coreOf('ZIGZAG', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, zigzagLookbackPositional(optInSensitivity, optInMinTrendLength, core), prices.high, prices.low);
  const outZigZag = o.outZigZag === undefined ? new Float64Array(capacity) : o.outZigZag;
  const outTrend = o.outTrend === undefined ? new Int32Array(capacity) : o.outTrend;
  const outPivotIdx = o.outPivotIdx === undefined ? new Int32Array(capacity) : o.outPivotIdx;
  const range = zigzagPositional(startIdx, endIdx, prices.high, prices.low, optInSensitivity, optInMinTrendLength, outZigZag, outTrend, outPivotIdx, core);
  return { begIdx: range.begIdx, outZigZag: outZigZag.subarray(0, range.count), outTrend: outTrend.subarray(0, range.count), outPivotIdx: outPivotIdx.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live ZIGZAG stream: one value per closed bar, bit-identical to {@link zigzag}
 * over the same series. Open it with {@link zigzagOpen} or
 * {@link zigzagOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/zigzag](https://ta-lib.org/functions/zigzag).
 */
export class ZigzagStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInSensitivity: number = 0;
  /** @internal */
  optInMinTrendLength: number = 0;
  /** @internal */
  trend: number = 0;
  /** @internal */
  barsSincePivot: number = 0;
  /** @internal */
  barIdx: number = 0;
  /** @internal */
  up: number = 0;
  /** @internal */
  dn: number = 0;
  /** @internal */
  pivot: number = 0;
  /** @internal */
  cur_outZigZag: number = 0;
  /** @internal */
  cur_outTrend: number = 0;
  /** @internal */
  cur_outPivotIdx: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): ZigzagStream {
    return new ZigzagStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link zigzag} reports them over the same bars. Each accepted `update` and each
   * `advance` adds one to `count`.
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
      throw streamFailure('ZIGZAG advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns its outputs.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param out - Receives the outputs and is returned; a new {@link ZigzagOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included, before anything moves.
   *   `TALibError` with code `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached
   *   bar `Core.INDEX_MAX`.
   */
  update(high: number, low: number, out?: ZigzagOut): ZigzagOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('ZIGZAG update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('ZIGZAG update', 'ZigzagOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low)) {
      throw nonFiniteBar('ZIGZAG update', !Number.isFinite(high) ? 'high' : 'low');
    }
    zigzagStepImpl(this, high, low);
    this.outRangeCount++;
    if (out === undefined) {
      out = new ZigzagOut();
    }
    out.outZigZag = this.cur_outZigZag;
    out.outTrend = this.cur_outTrend;
    out.outPivotIdx = this.cur_outPivotIdx;
    return out;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param out - Receives the outputs and is returned; a new {@link ZigzagOut} when
   *   omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included.
   */
  peek(high: number, low: number, out?: ZigzagOut): ZigzagOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('ZIGZAG peek', 'ZigzagOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low)) {
      throw nonFiniteBar('ZIGZAG peek', !Number.isFinite(high) ? 'high' : 'low');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
    let isExt = 0;
    let barIdx = sp.barIdx;
    let barsSincePivot = sp.barsSincePivot;
    let cur_outPivotIdx = 0;
    let cur_outTrend = 0;
    let cur_outZigZag = 0;
    let pivot = sp.pivot;
    let trend = sp.trend;
    /* The pivot's bar is barIdx - barsSincePivot. Both are carried: a stream
     * has no cursor to read a bar number from.
     */
    barsSincePivot = barsSincePivot + 1;
    barIdx = barIdx + 1;
    /* Reversal is tested BEFORE extension. On an outside bar that both makes
     * a new extreme and clears the threshold, the leg reverses and the old
     * pivot stays where it was; testing extension first would move the pivot
     * and lose the reversal, which is a whole leg of difference rather than a
     * rounding.
     */
    if (trend === -1) {
      if (inHigh >= pivot * sp.up && barsSincePivot >= sp.optInMinTrendLength) {
        trend = 1;
        pivot = inHigh;
        barsSincePivot = 0;
      } else {
        /* Ties extend: at an equal price the pivot moves to the later bar,
         * which only outPivotIdx shows.
         *
         * A select, not a branch: on moving prices the extension is taken
         * on a share of the bars no predictor follows.
         */
        isExt = (inLow <= pivot ? 1 : 0);
        pivot = (pivot < inLow) ? pivot : inLow;
        barsSincePivot = barsSincePivot - ((isExt * barsSincePivot) | 0);
      }
    } else if (inLow <= pivot * sp.dn && barsSincePivot >= sp.optInMinTrendLength) {
      trend = -1;
      pivot = inLow;
      barsSincePivot = 0;
    } else {
      isExt = (inHigh >= pivot ? 1 : 0);
      pivot = (pivot > inHigh) ? pivot : inHigh;
      barsSincePivot = barsSincePivot - ((isExt * barsSincePivot) | 0);
    }
    cur_outZigZag = pivot;
    cur_outTrend = trend;
    cur_outPivotIdx = barIdx - barsSincePivot;
    if (out === undefined) {
      out = new ZigzagOut();
    }
    out.outZigZag = cur_outZigZag;
    out.outTrend = cur_outTrend;
    out.outPivotIdx = cur_outPivotIdx;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link ZigzagOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a sink that is not an object,
   *   `null` included.
   */
  value(out?: ZigzagOut): ZigzagOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('ZIGZAG value', 'ZigzagOut');
    }
    if (out === undefined) {
      out = new ZigzagOut();
    }
    out.outZigZag = this.cur_outZigZag;
    out.outTrend = this.cur_outTrend;
    out.outPivotIdx = this.cur_outPivotIdx;
    return out;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): ZigzagStream {
    const c = new ZigzagStream(this.core);
    c.optInSensitivity = this.optInSensitivity;
    c.optInMinTrendLength = this.optInMinTrendLength;
    c.trend = this.trend;
    c.barsSincePivot = this.barsSincePivot;
    c.barIdx = this.barIdx;
    c.up = this.up;
    c.dn = this.dn;
    c.pivot = this.pivot;
    c.cur_outZigZag = this.cur_outZigZag;
    c.cur_outTrend = this.cur_outTrend;
    c.cur_outPivotIdx = this.cur_outPivotIdx;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one ZIGZAG bar, as `update`, `peek` and `value` return them.
 * Passed back to one of them, it is filled and returned again, with no allocation;
 * it holds its values until the next call that writes it. A new one holds `NaN`.
 */
export class ZigzagOut {
  /** Price of the swing extreme currently being tracked. */
  outZigZag: number = NaN;
  /** +1 while the leg is up, -1 while it is down. */
  outTrend: number = NaN;
  /** Absolute index, into the input, of the bar that extreme sits on. */
  outPivotIdx: number = NaN;
}

function zigzagStepImpl(sp: ZigzagStream, inHigh: number, inLow: number): void {
  let isExt = 0;
  /* The pivot's bar is barIdx - barsSincePivot. Both are carried: a stream
   * has no cursor to read a bar number from.
   */
  sp.barsSincePivot = sp.barsSincePivot + 1;
  sp.barIdx = sp.barIdx + 1;
  /* Reversal is tested BEFORE extension. On an outside bar that both makes
   * a new extreme and clears the threshold, the leg reverses and the old
   * pivot stays where it was; testing extension first would move the pivot
   * and lose the reversal, which is a whole leg of difference rather than a
   * rounding.
   */
  if (sp.trend === -1) {
    if (inHigh >= sp.pivot * sp.up && sp.barsSincePivot >= sp.optInMinTrendLength) {
      sp.trend = 1;
      sp.pivot = inHigh;
      sp.barsSincePivot = 0;
    } else {
      /* Ties extend: at an equal price the pivot moves to the later bar,
       * which only outPivotIdx shows.
       *
       * A select, not a branch: on moving prices the extension is taken
       * on a share of the bars no predictor follows.
       */
      isExt = (inLow <= sp.pivot ? 1 : 0);
      sp.pivot = (sp.pivot < inLow) ? sp.pivot : inLow;
      sp.barsSincePivot = sp.barsSincePivot - ((isExt * sp.barsSincePivot) | 0);
    }
  } else if (inLow <= sp.pivot * sp.dn && sp.barsSincePivot >= sp.optInMinTrendLength) {
    sp.trend = -1;
    sp.pivot = inLow;
    sp.barsSincePivot = 0;
  } else {
    isExt = (inHigh >= sp.pivot ? 1 : 0);
    sp.pivot = (sp.pivot > inHigh) ? sp.pivot : inHigh;
    sp.barsSincePivot = sp.barsSincePivot - ((isExt * sp.barsSincePivot) | 0);
  }
  sp.cur_outZigZag = sp.pivot;
  sp.cur_outTrend = sp.trend;
  sp.cur_outPivotIdx = sp.barIdx - sp.barsSincePivot;
}

function zigzagOpenImpl(
  sp: ZigzagStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInSensitivity: number,
  optInMinTrendLength: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outZigZag: Float64Array,
  outTrend: Int32Array,
  outPivotIdx: Int32Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let lookbackTotal = 0;
  let today = 0;
  let outIdx = 0;
  let trend = 0;
  let barsSincePivot = 0;
  let barIdx = 0;
  let isExt = 0;
  let s = 0;
  let up = 0;
  let dn = 0;
  let pivot = 0;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInSensitivity === Core.REAL_DEFAULT) {
    optInSensitivity = 5.0;
  } else if (!(optInSensitivity >= 0e0 && optInSensitivity <= 1e2)) {
    return RetCode.BAD_PARAM;
  }
  if (optInMinTrendLength === Core.INTEGER_DEFAULT) {
    optInMinTrendLength = 1;
  } else if (optInMinTrendLength < 1 || optInMinTrendLength > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  lookbackTotal = zigzagLookbackPositional(optInSensitivity, optInMinTrendLength, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Percent in, fraction inside. Each factor is rounded once, here, rather
   * than rebuilt per bar: the threshold is `pivot * up` in binary64, which is
   * the expression order the oracle uses.
   */
  s = optInSensitivity / 100.0;
  up = 1.0 + s;
  dn = 1.0 - s;
  /* The seed: a down leg whose extreme is the low of the bar
   * optInMinTrendLength before the first output. Because the gate counts the
   * seed bar like every other pivot, the first reversal can fire exactly at
   * the first output bar, not one later.
   */
  today = startIdx - lookbackTotal;
  trend = -1;
  pivot = inLow[today];
  barIdx = today;
  barsSincePivot = 0;
  today = today + 1;
  outIdx = 0;
  while (today <= endIdx) {
    /* The pivot's bar is barIdx - barsSincePivot. Both are carried: a stream
     * has no cursor to read a bar number from.
     */
    barsSincePivot = barsSincePivot + 1;
    barIdx = barIdx + 1;
    /* Reversal is tested BEFORE extension. On an outside bar that both makes
     * a new extreme and clears the threshold, the leg reverses and the old
     * pivot stays where it was; testing extension first would move the pivot
     * and lose the reversal, which is a whole leg of difference rather than a
     * rounding.
     */
    if (trend === -1) {
      if (inHigh[today] >= pivot * up && barsSincePivot >= optInMinTrendLength) {
        trend = 1;
        pivot = inHigh[today];
        barsSincePivot = 0;
      } else {
        /* Ties extend: at an equal price the pivot moves to the later bar,
         * which only outPivotIdx shows.
         *
         * A select, not a branch: on moving prices the extension is taken
         * on a share of the bars no predictor follows.
         */
        isExt = (inLow[today] <= pivot ? 1 : 0);
        pivot = (pivot < inLow[today]) ? pivot : inLow[today];
        barsSincePivot = barsSincePivot - ((isExt * barsSincePivot) | 0);
      }
    } else if (inLow[today] <= pivot * dn && barsSincePivot >= optInMinTrendLength) {
      trend = -1;
      pivot = inLow[today];
      barsSincePivot = 0;
    } else {
      isExt = (inHigh[today] >= pivot ? 1 : 0);
      pivot = (pivot > inHigh[today]) ? pivot : inHigh[today];
      barsSincePivot = barsSincePivot - ((isExt * barsSincePivot) | 0);
    }
    if (today >= startIdx) {
      outZigZag[((outIdx * outStride) | 0)] = pivot;
      outTrend[((outIdx * outStride) | 0)] = trend;
      outPivotIdx[((outIdx * outStride) | 0)] = barIdx - barsSincePivot;
      outIdx = outIdx + 1;
    }
    today = today + 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  sp.optInSensitivity = optInSensitivity;
  sp.optInMinTrendLength = optInMinTrendLength;
  sp.trend = trend;
  sp.barsSincePivot = barsSincePivot;
  sp.barIdx = barIdx;
  sp.up = up;
  sp.dn = dn;
  sp.pivot = pivot;
  sp.cur_outZigZag = outZigZag[(outNBElement.value - 1) * outStride];
  sp.cur_outTrend = outTrend[(outNBElement.value - 1) * outStride];
  sp.cur_outPivotIdx = outPivotIdx[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function zigzagOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInSensitivity: number,
  optInMinTrendLength: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outZigZag: Float64Array,
  outTrend: Int32Array,
  outPivotIdx: Int32Array,
): ZigzagStream {
  const sp = ZigzagStream.of(core);
  const retCode = zigzagOpenImpl(sp, inHigh, inLow, startIdx, optInSensitivity, optInMinTrendLength, outBegIdx, outNBElement, outZigZag, outTrend, outPivotIdx, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ZIGZAG openAndFill', inHigh.length, startIdx, zigzagLookbackPositional(optInSensitivity, optInMinTrendLength, core));
  }
  if (retCode === RetCode.BAD_PARAM && zigzagLookbackPositional(optInSensitivity, optInMinTrendLength, core) < 0) {
    throw rejectedParams('ZIGZAG openAndFill', PARAM_RANGES, optInSensitivity, optInMinTrendLength);
  }
  throw streamFailure('ZIGZAG openAndFill', retCode);
}

/** @internal */
export function zigzagOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInSensitivity: number,
  optInMinTrendLength: number,
): ZigzagStream {
  const sp = ZigzagStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outZigZag = new Float64Array(1);
  const sink_outTrend = new Int32Array(1);
  const sink_outPivotIdx = new Int32Array(1);
  const retCode = zigzagOpenImpl(sp, inHigh, inLow, startIdx, optInSensitivity, optInMinTrendLength, outBegIdx, outNBElement, sink_outZigZag, sink_outTrend, sink_outPivotIdx, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ZIGZAG open', inHigh.length, startIdx, zigzagLookbackPositional(optInSensitivity, optInMinTrendLength, core));
  }
  if (retCode === RetCode.BAD_PARAM && zigzagLookbackPositional(optInSensitivity, optInMinTrendLength, core) < 0) {
    throw rejectedParams('ZIGZAG open', PARAM_RANGES, optInSensitivity, optInMinTrendLength);
  }
  throw streamFailure('ZIGZAG open', retCode);
}

/** @internal */
export function zigzagOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInSensitivity: number,
  optInMinTrendLength: number,
  core: Core = Core.DEFAULT,
): ZigzagStream {
  optInSensitivity = requireReal('ZIGZAG open', 'sensitivity', optInSensitivity);
  optInMinTrendLength = requireInt32('ZIGZAG open', 'minTrendLength', optInMinTrendLength);
  requireArgument('ZIGZAG open', 'prices.high', inHigh);
  requireHistory('ZIGZAG open', 'prices.high', inHigh.length);
  requireArgument('ZIGZAG open', 'prices.low', inLow);
  requireHistoryLength('ZIGZAG open', 'prices.low', inLow.length, inHigh.length);
  return zigzagOpenInternal(core, float64(inHigh), float64(inLow), 0, optInSensitivity, optInMinTrendLength);
}

/** @internal */
export function zigzagOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInSensitivity: number,
  optInMinTrendLength: number,
  outZigZag: Float64Array,
  outTrend: Int32Array,
  outPivotIdx: Int32Array,
  core: Core = Core.DEFAULT,
): ZigzagStream {
  optInSensitivity = requireReal('ZIGZAG openAndFill', 'sensitivity', optInSensitivity);
  optInMinTrendLength = requireInt32('ZIGZAG openAndFill', 'minTrendLength', optInMinTrendLength);
  requireArgument('ZIGZAG openAndFill', 'prices.high', inHigh);
  requireHistory('ZIGZAG openAndFill', 'prices.high', inHigh.length);
  requireArgument('ZIGZAG openAndFill', 'prices.low', inLow);
  const guardLookback = zigzagLookbackPositional(optInSensitivity, optInMinTrendLength, core);
  if (guardLookback < 0) {
    throw rejectedParams('ZIGZAG openAndFill', PARAM_RANGES, optInSensitivity, optInMinTrendLength);
  }
  const guardOutLen = openFillCount('ZIGZAG openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('ZIGZAG openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireOutput('ZIGZAG openAndFill', 'outZigZag', outZigZag, guardOutLen, Float64Array);
  requireOutput('ZIGZAG openAndFill', 'outTrend', outTrend, guardOutLen, Int32Array);
  requireOutput('ZIGZAG openAndFill', 'outPivotIdx', outPivotIdx, guardOutLen, Int32Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (outZigZag.buffer === inHighF64.buffer && outZigZag.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('ZIGZAG openAndFill', 'outZigZag', 'prices.high');
  }
  if (outZigZag.buffer === inLowF64.buffer && outZigZag.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('ZIGZAG openAndFill', 'outZigZag', 'prices.low');
  }
  if (outTrend.buffer === inHighF64.buffer && outTrend.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('ZIGZAG openAndFill', 'outTrend', 'prices.high');
  }
  if (outTrend.buffer === inLowF64.buffer && outTrend.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('ZIGZAG openAndFill', 'outTrend', 'prices.low');
  }
  if (outPivotIdx.buffer === inHighF64.buffer && outPivotIdx.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('ZIGZAG openAndFill', 'outPivotIdx', 'prices.high');
  }
  if (outPivotIdx.buffer === inLowF64.buffer && outPivotIdx.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('ZIGZAG openAndFill', 'outPivotIdx', 'prices.low');
  }
  if (outZigZag.buffer === outTrend.buffer && outZigZag.byteOffset === outTrend.byteOffset) {
    throw sharedMemory('ZIGZAG openAndFill', 'outZigZag', 'outTrend');
  }
  if (outZigZag.buffer === outPivotIdx.buffer && outZigZag.byteOffset === outPivotIdx.byteOffset) {
    throw sharedMemory('ZIGZAG openAndFill', 'outZigZag', 'outPivotIdx');
  }
  if (outTrend.buffer === outPivotIdx.buffer && outTrend.byteOffset === outPivotIdx.byteOffset) {
    throw sharedMemory('ZIGZAG openAndFill', 'outTrend', 'outPivotIdx');
  }
  if (guardOutLen > 0 && overlaps(outZigZag, inHighF64)) {
    throw sharedMemory('ZIGZAG openAndFill', 'outZigZag', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outZigZag, inLowF64)) {
    throw sharedMemory('ZIGZAG openAndFill', 'outZigZag', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outTrend, inHighF64)) {
    throw sharedMemory('ZIGZAG openAndFill', 'outTrend', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outTrend, inLowF64)) {
    throw sharedMemory('ZIGZAG openAndFill', 'outTrend', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outPivotIdx, inHighF64)) {
    throw sharedMemory('ZIGZAG openAndFill', 'outPivotIdx', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outPivotIdx, inLowF64)) {
    throw sharedMemory('ZIGZAG openAndFill', 'outPivotIdx', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outZigZag, outTrend)) {
    throw sharedMemory('ZIGZAG openAndFill', 'outZigZag', 'outTrend');
  }
  if (guardOutLen > 0 && overlaps(outZigZag, outPivotIdx)) {
    throw sharedMemory('ZIGZAG openAndFill', 'outZigZag', 'outPivotIdx');
  }
  if (guardOutLen > 0 && overlaps(outTrend, outPivotIdx)) {
    throw sharedMemory('ZIGZAG openAndFill', 'outTrend', 'outPivotIdx');
  }
  return zigzagOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInSensitivity, optInMinTrendLength, new MInteger(), new MInteger(), outZigZag, outTrend, outPivotIdx);
}

/**
 * Opens a live ZIGZAG stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link zigzag} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link ZigzagOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `zigzagLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a real parameter that is not a
 *   number, a parameter outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { zigzagOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = zigzagOpen({ high, low }, { sensitivity: 5, minTrendLength: 1 });
 * const peeked = stream.peek(101.4, 99.1); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1); // the bar closes
 * ```
 */
export function zigzagOpen(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: ZigzagOptions): ZigzagStream {
  requireArgument('ZIGZAG open', 'prices', prices);
  const o = optionsOf('ZIGZAG open', options);
  const optInSensitivity = orDefault(o.sensitivity, Core.REAL_DEFAULT);
  const optInMinTrendLength = orDefault(o.minTrendLength, Core.INTEGER_DEFAULT);
  const core = coreOf('ZIGZAG open', o.core);
  return zigzagOpenPositional(prices.high, prices.low, optInSensitivity, optInMinTrendLength, core);
}

/**
 * {@link zigzagOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link zigzag}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link ZigzagOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link zigzag} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `zigzagLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a real parameter that is not a
 *   number, a parameter outside its range, histories of different lengths or a
 *   buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { zigzagOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const {
 *   stream,
 *   begIdx,
 *   outZigZag,
 *   outTrend,
 *   outPivotIdx,
 * } = zigzagOpenAndFill({ high, low }, { sensitivity: 5, minTrendLength: 1 });
 * // outZigZag[k] is the value at bar begIdx + k
 * ```
 */
export function zigzagOpenAndFill(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: ZigzagOptions): ZigzagResult & { stream: ZigzagStream } {
  requireArgument('ZIGZAG openAndFill', 'prices', prices);
  const o = optionsOf('ZIGZAG openAndFill', options);
  const optInSensitivity = orDefault(o.sensitivity, Core.REAL_DEFAULT);
  const optInMinTrendLength = orDefault(o.minTrendLength, Core.INTEGER_DEFAULT);
  const core = coreOf('ZIGZAG openAndFill', o.core);
  const capacity = fillCapacity(zigzagLookbackPositional(optInSensitivity, optInMinTrendLength, core), prices.high, prices.low);
  const outZigZag = o.outZigZag === undefined ? new Float64Array(capacity) : o.outZigZag;
  const outTrend = o.outTrend === undefined ? new Int32Array(capacity) : o.outTrend;
  const outPivotIdx = o.outPivotIdx === undefined ? new Int32Array(capacity) : o.outPivotIdx;
  const stream = zigzagOpenAndFillPositional(prices.high, prices.low, optInSensitivity, optInMinTrendLength, outZigZag, outTrend, outPivotIdx, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outZigZag: outZigZag.subarray(0, range.count), outTrend: outTrend.subarray(0, range.count), outPivotIdx: outPivotIdx.subarray(0, range.count) };
}
