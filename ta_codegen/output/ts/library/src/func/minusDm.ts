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
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY    Description
 *  -------------------------------------------------------------------
 *  010802 MF    Template creation.
 *  052603 MF    Adapt code to compile with .NET Managed C++
 *  091326 MF,CC #411 Wilder step without a divide or a branch.
 */

import { FuncUnstId } from '../enums.ts';
import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function minusDmLookbackPositional(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  if (optInTimePeriod > 1) {
    return optInTimePeriod + core.unstableCount(FuncUnstId.MINUS_DM, ((optInTimePeriod > 1) ? (((((10 * (((2 * optInTimePeriod) | 0) - 1)) | 0) + 1) / 2) | 0) : 0), ((optInTimePeriod > 1) ? (((((19 * (((2 * optInTimePeriod) | 0) - 1)) | 0) + 1) / 2) | 0) : 0)) - 1;
  } else {
    return 1;
  }
}

/** @internal */
export function minusDmDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (minusDmLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function minusDmImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let today = 0;
  let lookbackTotal = 0;
  let outIdx = 0;
  let prevHigh = 0;
  let prevLow = 0;
  let tempReal = 0;
  let invPeriod = 0;
  let prevMinusDM = 0;
  let diffP = 0;
  let diffM = 0;
  let minusDM1 = 0;
  let i = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  /*
   * The DM1 (one period) is base on the largest part of
   * today's range that is outside of yesterdays range.
   *
   * The following 7 cases explain how the +DM and -DM are
   * calculated on one period:
   *
   * Case 1:                       Case 2:
   *    C|                        A|
   *     |                         | C|
   *     | +DM1 = (C-A)           B|  | +DM1 = 0
   *     | -DM1 = 0                   | -DM1 = (B-D)
   * A|  |                           D|
   *  | D|
   * B|
   *
   * Case 3:                       Case 4:
   *    C|                           C|
   *     |                        A|  |
   *     | +DM1 = (C-A)            |  | +DM1 = 0
   *     | -DM1 = 0               B|  | -DM1 = (B-D)
   * A|  |                            |
   *  |  |                           D|
   * B|  |
   *    D|
   *
   * Case 5:                      Case 6:
   * A|                           A| C|
   *  | C| +DM1 = 0                |  |  +DM1 = 0
   *  |  | -DM1 = 0                |  |  -DM1 = 0
   *  | D|                         |  |
   * B|                           B| D|
   *
   *
   * Case 7:
   *
   *    C|
   * A|  |
   *  |  | +DM=0
   * B|  | -DM=0
   *    D|
   *
   * In case 3 and 4, the rule is that the smallest delta between
   * (C-A) and (B-D) determine which of +DM or -DM is zero.
   *
   * In case 7, (C-A) and (B-D) are equal, so both +DM and -DM are
   * zero.
   *
   * The rules remain the same when A=B and C=D (when the highs
   * equal the lows).
   *
   * When calculating the DM over a period > 1, the one-period DM
   * for the desired period are initialy sum. In other word,
   * for a -DM14, sum the -DM1 for the first 14 days (that's
   * 13 values because there is no DM for the first day!)
   * Subsequent DM are calculated using the Wilder's
   * smoothing approach:
   *
   *                                    Previous -DM14
   *  Today's -DM14 = Previous -DM14 -  -------------- + Today's -DM1
   *                                         14
   *
   * Reference:
   *    New Concepts In Technical Trading Systems, J. Welles Wilder Jr
   */
  lookbackTotal = minusDmLookbackPositional(optInTimePeriod, core);
  /* Adjust startIdx to account for the lookback period. */
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  /* Indicate where the next output should be put
   * in the outReal.
   */
  outIdx = 0;
  /* Trap the case where no smoothing is needed. */
  if (optInTimePeriod <= 1) {
    /* No smoothing needed. Just do a simple DM1
     * for each price bar.
     */
    outBegIdx.value = startIdx;
    today = startIdx - 1;
    prevHigh = inHigh[today];
    prevLow = inLow[today];
    while (today < endIdx) {
      today += 1;
      tempReal = inHigh[today];
      diffP = tempReal - prevHigh;
      /* Plus Delta */
      prevHigh = tempReal;
      tempReal = inLow[today];
      diffM = prevLow - tempReal;
      /* Minus Delta */
      prevLow = tempReal;
      minusDM1 = (diffM > 0.0) ? diffM : 0.0;
      minusDM1 = (diffP < diffM) ? minusDM1 : 0.0;
      outReal[outIdx++] = minusDM1;
    }
    outNBElement.value = outIdx;
    return RetCode.SUCCESS;
  }
  invPeriod = 1.0 / optInTimePeriod;
  /* Process the initial DM */
  outBegIdx.value = startIdx;
  prevMinusDM = 0.0;
  today = startIdx - lookbackTotal;
  prevHigh = inHigh[today];
  prevLow = inLow[today];
  i = optInTimePeriod - 1;
  while (i-- > 0) {
    today += 1;
    tempReal = inHigh[today];
    diffP = tempReal - prevHigh;
    /* Plus Delta */
    prevHigh = tempReal;
    tempReal = inLow[today];
    diffM = prevLow - tempReal;
    /* Minus Delta */
    prevLow = tempReal;
    /* -DM1 = diffM when diffP < diffM and diffM > 0: the select takes the
     * first test and the max the second, as a non-positive delta cannot raise
     * the sum. gcc keeps a branch if a select, not the max, ends the step.
     */
    tempReal = diffM - diffP;
    minusDM1 = (tempReal > 0.0) ? diffM : 0.0;
    tempReal = prevMinusDM + minusDM1;
    prevMinusDM = (prevMinusDM > tempReal) ? prevMinusDM : tempReal;
  }
  /* Process subsequent DM */
  /* Skip the unstable period. */
  i = lookbackTotal - (optInTimePeriod - 1);
  while (i-- !== 0) {
    today += 1;
    tempReal = inHigh[today];
    diffP = tempReal - prevHigh;
    /* Plus Delta */
    prevHigh = tempReal;
    tempReal = inLow[today];
    diffM = prevLow - tempReal;
    /* Minus Delta */
    prevLow = tempReal;
    tempReal = diffM - diffP;
    minusDM1 = (tempReal > 0.0) ? diffM : 0.0;
    tempReal = prevMinusDM - prevMinusDM * invPeriod;
    prevMinusDM = tempReal + minusDM1;
    prevMinusDM = (tempReal > prevMinusDM) ? tempReal : prevMinusDM;
  }
  /* Now start to write the output in
   * the caller provided outReal.
   */
  outReal[0] = prevMinusDM;
  outIdx = 1;
  while (today < endIdx) {
    today += 1;
    tempReal = inHigh[today];
    diffP = tempReal - prevHigh;
    /* Plus Delta */
    prevHigh = tempReal;
    tempReal = inLow[today];
    diffM = prevLow - tempReal;
    /* Minus Delta */
    prevLow = tempReal;
    tempReal = diffM - diffP;
    minusDM1 = (tempReal > 0.0) ? diffM : 0.0;
    tempReal = prevMinusDM - prevMinusDM * invPeriod;
    prevMinusDM = tempReal + minusDM1;
    prevMinusDM = (tempReal > prevMinusDM) ? tempReal : prevMinusDM;
    outReal[outIdx++] = prevMinusDM;
  }
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000']] as const;

/** @internal */
export function minusDmPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('MINUS_DM', 'startIdx', startIdx);
  endIdx = requireInt32('MINUS_DM', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('MINUS_DM', 'timePeriod', optInTimePeriod);
  requireIndexRange('MINUS_DM', startIdx, endIdx);
  const guardLookback = minusDmLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('MINUS_DM', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('MINUS_DM', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('MINUS_DM', 'prices.high', inHigh, guardInLen);
  requireLength('MINUS_DM', 'prices.low', inLow, guardInLen);
  requireOutput('MINUS_DM', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlapsPartly(outReal, inHighF64)) {
    throw sharedMemory('MINUS_DM', 'outReal', 'prices.high');
  }
  if (overlapsPartly(outReal, inLowF64)) {
    throw sharedMemory('MINUS_DM', 'outReal', 'prices.low');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = minusDmImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('MINUS_DM', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link minusDm}, {@link minusDmLookback},
 * {@link minusDmDisplayShift}, {@link minusDmOpen} and {@link minusDmOpenAndFill}.
 * Every key is optional: an omitted or `undefined` key takes its default, and a
 * call ignores the keys it has no use for.
 */
export interface MinusDmOptions {
  /**
   * Wilder smoothing period (default 14; range 1 to 100000; `Core.INTEGER_DEFAULT`
   * selects the default).
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
   * Buffer for `values`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outReal?: Float64Array<ArrayBuffer> | undefined;
}

/** What {@link minusDm} returns, and {@link minusDmOpenAndFill} with its stream. */
export interface MinusDmResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Smoothed minus directional movement. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link minusDm} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MinusDmOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function minusDmLookback(options?: MinusDmOptions): number {
  const o = optionsOf('MINUS_DM lookback', options);
  return minusDmLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('MINUS_DM lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link minusDm}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MinusDmOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function minusDmDisplayShift(outputIdx: number, options?: MinusDmOptions): number {
  const o = optionsOf('MINUS_DM displayShift', options);
  return minusDmDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('MINUS_DM displayShift', o.core));
}

/**
 * Minus Directional Movement, the downward component of Wilder's directional
 * movement system. Measures Wilder-smoothed downward price motion over the period.
 * Higher -DM indicates stronger downward directional movement.
 *
 * Formula and more info at
 * [ta-lib.org/functions/minus_dm](https://ta-lib.org/functions/minus_dm).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link minusDmLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `high`: High price of
 *   each bar. `low`: Low price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link MinusDmOptions}.
 * @returns `begIdx` and the values; see {@link MinusDmResult}. Each output is a
 *   view of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, an integer parameter that is not an int32, a parameter outside its range,
 *   an input too short for the range, a buffer that is empty, too short or of the
 *   wrong type or a buffer sharing memory with a `Float64Array` input it is not.
 *   Checked before anything is written.
 *
 * @example
 * ```ts
 * import { minusDm } from 'ta-lib';
 *
 * const high = new Float64Array(252); // your data, oldest first
 * const low = new Float64Array(252); // your data, oldest first
 *
 * const { begIdx, values } = minusDm({ high, low }, { timePeriod: 14 });
 * console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
 * ```
 * @see [`plusDm`](https://ta-lib.org/functions/plus_dm)
 * @see [`minusDi`](https://ta-lib.org/functions/minus_di)
 * @see [`plusDi`](https://ta-lib.org/functions/plus_di)
 * @see [`dx`](https://ta-lib.org/functions/dx)
 * @see [`adx`](https://ta-lib.org/functions/adx)
 * @see [`adxr`](https://ta-lib.org/functions/adxr)
 */
export function minusDm(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: MinusDmOptions): MinusDmResult {
  requireArgument('MINUS_DM', 'prices', prices);
  const o = optionsOf('MINUS_DM', options);
  if (o.endIdx === undefined) {
    requireSeries('MINUS_DM', 'prices.high', prices.high);
    requireSeries('MINUS_DM', 'prices.low', prices.low);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MINUS_DM', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, minusDmLookbackPositional(optInTimePeriod, core), prices.high, prices.low);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = minusDmPositional(startIdx, endIdx, prices.high, prices.low, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live MINUS_DM stream: one value per closed bar, bit-identical to
 * {@link minusDm} over the same series. Open it with {@link minusDmOpen} or
 * {@link minusDmOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/minus_dm](https://ta-lib.org/functions/minus_dm).
 */
export class MinusDmStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  prevHigh: number = 0;
  /** @internal */
  prevLow: number = 0;
  /** @internal */
  invPeriod: number = 0;
  /** @internal */
  prevMinusDM: number = 0;
  /** @internal */
  cur_outReal: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): MinusDmStream {
    return new MinusDmStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link minusDm} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('MINUS_DM advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(high: number, low: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('MINUS_DM update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(high) || !Number.isFinite(low)) {
      throw nonFiniteBar('MINUS_DM update', !Number.isFinite(high) ? 'high' : 'low');
    }
    minusDmStepImpl(this, high, low);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(high: number, low: number): number {
    if (!Number.isFinite(high) || !Number.isFinite(low)) {
      throw nonFiniteBar('MINUS_DM peek', !Number.isFinite(high) ? 'high' : 'low');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
    let cur_outReal = 0;
    if (sp.optInTimePeriod <= 1) {
      let tempReal = 0;
      let diffP = 0;
      let diffM = 0;
      let minusDM1 = 0;
      let prevHigh = sp.prevHigh;
      let prevLow = sp.prevLow;
      tempReal = inHigh;
      diffP = tempReal - prevHigh;
      /* Plus Delta */
      prevHigh = tempReal;
      tempReal = inLow;
      diffM = prevLow - tempReal;
      /* Minus Delta */
      prevLow = tempReal;
      minusDM1 = (diffM > 0.0) ? diffM : 0.0;
      minusDM1 = (diffP < diffM) ? minusDM1 : 0.0;
      cur_outReal = minusDM1;
    } else {
      let tempReal = 0;
      let diffP = 0;
      let diffM = 0;
      let minusDM1 = 0;
      let prevHigh = sp.prevHigh;
      let prevLow = sp.prevLow;
      let prevMinusDM = sp.prevMinusDM;
      tempReal = inHigh;
      diffP = tempReal - prevHigh;
      /* Plus Delta */
      prevHigh = tempReal;
      tempReal = inLow;
      diffM = prevLow - tempReal;
      /* Minus Delta */
      prevLow = tempReal;
      tempReal = diffM - diffP;
      minusDM1 = (tempReal > 0.0) ? diffM : 0.0;
      tempReal = prevMinusDM - prevMinusDM * sp.invPeriod;
      prevMinusDM = tempReal + minusDM1;
      prevMinusDM = (tempReal > prevMinusDM) ? tempReal : prevMinusDM;
      cur_outReal = prevMinusDM;
    }
    return cur_outReal;
  }

  /** The value at the last bar this stream counted; `peek` does not change it. */
  value(): number {
    return this.cur_outReal;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): MinusDmStream {
    const c = new MinusDmStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.prevHigh = this.prevHigh;
    c.prevLow = this.prevLow;
    c.invPeriod = this.invPeriod;
    c.prevMinusDM = this.prevMinusDM;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function minusDmStepImpl(sp: MinusDmStream, inHigh: number, inLow: number): void {
  if (sp.optInTimePeriod <= 1) {
    let tempReal = 0;
    let diffP = 0;
    let diffM = 0;
    let minusDM1 = 0;
    tempReal = inHigh;
    diffP = tempReal - sp.prevHigh;
    /* Plus Delta */
    sp.prevHigh = tempReal;
    tempReal = inLow;
    diffM = sp.prevLow - tempReal;
    /* Minus Delta */
    sp.prevLow = tempReal;
    minusDM1 = (diffM > 0.0) ? diffM : 0.0;
    minusDM1 = (diffP < diffM) ? minusDM1 : 0.0;
    sp.cur_outReal = minusDM1;
  } else {
    let tempReal = 0;
    let diffP = 0;
    let diffM = 0;
    let minusDM1 = 0;
    tempReal = inHigh;
    diffP = tempReal - sp.prevHigh;
    /* Plus Delta */
    sp.prevHigh = tempReal;
    tempReal = inLow;
    diffM = sp.prevLow - tempReal;
    /* Minus Delta */
    sp.prevLow = tempReal;
    tempReal = diffM - diffP;
    minusDM1 = (tempReal > 0.0) ? diffM : 0.0;
    tempReal = sp.prevMinusDM - sp.prevMinusDM * sp.invPeriod;
    sp.prevMinusDM = tempReal + minusDM1;
    sp.prevMinusDM = (tempReal > sp.prevMinusDM) ? tempReal : sp.prevMinusDM;
    sp.cur_outReal = sp.prevMinusDM;
  }
}

function minusDmOpenImpl(
  sp: MinusDmStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod <= 1) {
    let today = 0;
    let lookbackTotal = 0;
    let outIdx = 0;
    let prevHigh = 0;
    let prevLow = 0;
    let tempReal = 0;
    let invPeriod = 0;
    let prevMinusDM = 0;
    let diffP = 0;
    let diffM = 0;
    let minusDM1 = 0;
    /*
     * The DM1 (one period) is base on the largest part of
     * today's range that is outside of yesterdays range.
     *
     * The following 7 cases explain how the +DM and -DM are
     * calculated on one period:
     *
     * Case 1:                       Case 2:
     *    C|                        A|
     *     |                         | C|
     *     | +DM1 = (C-A)           B|  | +DM1 = 0
     *     | -DM1 = 0                   | -DM1 = (B-D)
     * A|  |                           D|
     *  | D|
     * B|
     *
     * Case 3:                       Case 4:
     *    C|                           C|
     *     |                        A|  |
     *     | +DM1 = (C-A)            |  | +DM1 = 0
     *     | -DM1 = 0               B|  | -DM1 = (B-D)
     * A|  |                            |
     *  |  |                           D|
     * B|  |
     *    D|
     *
     * Case 5:                      Case 6:
     * A|                           A| C|
     *  | C| +DM1 = 0                |  |  +DM1 = 0
     *  |  | -DM1 = 0                |  |  -DM1 = 0
     *  | D|                         |  |
     * B|                           B| D|
     *
     *
     * Case 7:
     *
     *    C|
     * A|  |
     *  |  | +DM=0
     * B|  | -DM=0
     *    D|
     *
     * In case 3 and 4, the rule is that the smallest delta between
     * (C-A) and (B-D) determine which of +DM or -DM is zero.
     *
     * In case 7, (C-A) and (B-D) are equal, so both +DM and -DM are
     * zero.
     *
     * The rules remain the same when A=B and C=D (when the highs
     * equal the lows).
     *
     * When calculating the DM over a period > 1, the one-period DM
     * for the desired period are initialy sum. In other word,
     * for a -DM14, sum the -DM1 for the first 14 days (that's
     * 13 values because there is no DM for the first day!)
     * Subsequent DM are calculated using the Wilder's
     * smoothing approach:
     *
     *                                    Previous -DM14
     *  Today's -DM14 = Previous -DM14 -  -------------- + Today's -DM1
     *                                         14
     *
     * Reference:
     *    New Concepts In Technical Trading Systems, J. Welles Wilder Jr
     */
    lookbackTotal = minusDmLookbackPositional(optInTimePeriod, core);
    /* Adjust startIdx to account for the lookback period. */
    if (startIdx < lookbackTotal) {
      startIdx = lookbackTotal;
    }
    /* Make sure there is still something to evaluate. */
    if (startIdx > endIdx) {
      outBegIdx.value = 0;
      outNBElement.value = 0;
      return RetCode.INSUFFICIENT_HISTORY;
    }
    /* Indicate where the next output should be put
     * in the outReal.
     */
    outIdx = 0;
    /* Trap the case where no smoothing is needed. */
    /* No smoothing needed. Just do a simple DM1
     * for each price bar.
     */
    outBegIdx.value = startIdx;
    today = startIdx - 1;
    prevHigh = inHigh[today];
    prevLow = inLow[today];
    while (today < endIdx) {
      today += 1;
      tempReal = inHigh[today];
      diffP = tempReal - prevHigh;
      /* Plus Delta */
      prevHigh = tempReal;
      tempReal = inLow[today];
      diffM = prevLow - tempReal;
      /* Minus Delta */
      prevLow = tempReal;
      minusDM1 = (diffM > 0.0) ? diffM : 0.0;
      minusDM1 = (diffP < diffM) ? minusDM1 : 0.0;
      outReal[((outIdx++ * outStride) | 0)] = minusDM1;
    }
    outNBElement.value = outIdx;
    sp.optInTimePeriod = optInTimePeriod;
    sp.prevHigh = prevHigh;
    sp.prevLow = prevLow;
    sp.invPeriod = invPeriod;
    sp.prevMinusDM = prevMinusDM;
    sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
    return RetCode.SUCCESS;
  } else {
    let today = 0;
    let lookbackTotal = 0;
    let outIdx = 0;
    let prevHigh = 0;
    let prevLow = 0;
    let tempReal = 0;
    let invPeriod = 0;
    let prevMinusDM = 0;
    let diffP = 0;
    let diffM = 0;
    let minusDM1 = 0;
    let i = 0;
    /*
     * The DM1 (one period) is base on the largest part of
     * today's range that is outside of yesterdays range.
     *
     * The following 7 cases explain how the +DM and -DM are
     * calculated on one period:
     *
     * Case 1:                       Case 2:
     *    C|                        A|
     *     |                         | C|
     *     | +DM1 = (C-A)           B|  | +DM1 = 0
     *     | -DM1 = 0                   | -DM1 = (B-D)
     * A|  |                           D|
     *  | D|
     * B|
     *
     * Case 3:                       Case 4:
     *    C|                           C|
     *     |                        A|  |
     *     | +DM1 = (C-A)            |  | +DM1 = 0
     *     | -DM1 = 0               B|  | -DM1 = (B-D)
     * A|  |                            |
     *  |  |                           D|
     * B|  |
     *    D|
     *
     * Case 5:                      Case 6:
     * A|                           A| C|
     *  | C| +DM1 = 0                |  |  +DM1 = 0
     *  |  | -DM1 = 0                |  |  -DM1 = 0
     *  | D|                         |  |
     * B|                           B| D|
     *
     *
     * Case 7:
     *
     *    C|
     * A|  |
     *  |  | +DM=0
     * B|  | -DM=0
     *    D|
     *
     * In case 3 and 4, the rule is that the smallest delta between
     * (C-A) and (B-D) determine which of +DM or -DM is zero.
     *
     * In case 7, (C-A) and (B-D) are equal, so both +DM and -DM are
     * zero.
     *
     * The rules remain the same when A=B and C=D (when the highs
     * equal the lows).
     *
     * When calculating the DM over a period > 1, the one-period DM
     * for the desired period are initialy sum. In other word,
     * for a -DM14, sum the -DM1 for the first 14 days (that's
     * 13 values because there is no DM for the first day!)
     * Subsequent DM are calculated using the Wilder's
     * smoothing approach:
     *
     *                                    Previous -DM14
     *  Today's -DM14 = Previous -DM14 -  -------------- + Today's -DM1
     *                                         14
     *
     * Reference:
     *    New Concepts In Technical Trading Systems, J. Welles Wilder Jr
     */
    lookbackTotal = minusDmLookbackPositional(optInTimePeriod, core);
    /* Adjust startIdx to account for the lookback period. */
    if (startIdx < lookbackTotal) {
      startIdx = lookbackTotal;
    }
    /* Make sure there is still something to evaluate. */
    if (startIdx > endIdx) {
      outBegIdx.value = 0;
      outNBElement.value = 0;
      return RetCode.INSUFFICIENT_HISTORY;
    }
    /* Indicate where the next output should be put
     * in the outReal.
     */
    outIdx = 0;
    /* Trap the case where no smoothing is needed. */
    invPeriod = 1.0 / optInTimePeriod;
    /* Process the initial DM */
    outBegIdx.value = startIdx;
    prevMinusDM = 0.0;
    today = startIdx - lookbackTotal;
    prevHigh = inHigh[today];
    prevLow = inLow[today];
    i = optInTimePeriod - 1;
    while (i-- > 0) {
      today += 1;
      tempReal = inHigh[today];
      diffP = tempReal - prevHigh;
      /* Plus Delta */
      prevHigh = tempReal;
      tempReal = inLow[today];
      diffM = prevLow - tempReal;
      /* Minus Delta */
      prevLow = tempReal;
      /* -DM1 = diffM when diffP < diffM and diffM > 0: the select takes the
       * first test and the max the second, as a non-positive delta cannot raise
       * the sum. gcc keeps a branch if a select, not the max, ends the step.
       */
      tempReal = diffM - diffP;
      minusDM1 = (tempReal > 0.0) ? diffM : 0.0;
      tempReal = prevMinusDM + minusDM1;
      prevMinusDM = (prevMinusDM > tempReal) ? prevMinusDM : tempReal;
    }
    /* Process subsequent DM */
    /* Skip the unstable period. */
    i = lookbackTotal - (optInTimePeriod - 1);
    while (i-- !== 0) {
      today += 1;
      tempReal = inHigh[today];
      diffP = tempReal - prevHigh;
      /* Plus Delta */
      prevHigh = tempReal;
      tempReal = inLow[today];
      diffM = prevLow - tempReal;
      /* Minus Delta */
      prevLow = tempReal;
      tempReal = diffM - diffP;
      minusDM1 = (tempReal > 0.0) ? diffM : 0.0;
      tempReal = prevMinusDM - prevMinusDM * invPeriod;
      prevMinusDM = tempReal + minusDM1;
      prevMinusDM = (tempReal > prevMinusDM) ? tempReal : prevMinusDM;
    }
    /* Now start to write the output in
     * the caller provided outReal.
     */
    outReal[((0 * outStride) | 0)] = prevMinusDM;
    outIdx = 1;
    while (today < endIdx) {
      today += 1;
      tempReal = inHigh[today];
      diffP = tempReal - prevHigh;
      /* Plus Delta */
      prevHigh = tempReal;
      tempReal = inLow[today];
      diffM = prevLow - tempReal;
      /* Minus Delta */
      prevLow = tempReal;
      tempReal = diffM - diffP;
      minusDM1 = (tempReal > 0.0) ? diffM : 0.0;
      tempReal = prevMinusDM - prevMinusDM * invPeriod;
      prevMinusDM = tempReal + minusDM1;
      prevMinusDM = (tempReal > prevMinusDM) ? tempReal : prevMinusDM;
      outReal[((outIdx++ * outStride) | 0)] = prevMinusDM;
    }
    outNBElement.value = outIdx;
    sp.optInTimePeriod = optInTimePeriod;
    sp.prevHigh = prevHigh;
    sp.prevLow = prevLow;
    sp.invPeriod = invPeriod;
    sp.prevMinusDM = prevMinusDM;
    sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
    return RetCode.SUCCESS;
  }
}

/** @internal */
export function minusDmOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): MinusDmStream {
  const sp = MinusDmStream.of(core);
  const retCode = minusDmOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MINUS_DM openAndFill', inHigh.length, startIdx, minusDmLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && minusDmLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('MINUS_DM openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('MINUS_DM openAndFill', retCode);
}

/** @internal */
export function minusDmOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): MinusDmStream {
  const sp = MinusDmStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = minusDmOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MINUS_DM open', inHigh.length, startIdx, minusDmLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && minusDmLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('MINUS_DM open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('MINUS_DM open', retCode);
}

/** @internal */
export function minusDmOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): MinusDmStream {
  optInTimePeriod = requireInt32('MINUS_DM open', 'timePeriod', optInTimePeriod);
  requireArgument('MINUS_DM open', 'prices.high', inHigh);
  requireHistory('MINUS_DM open', 'prices.high', inHigh.length);
  requireArgument('MINUS_DM open', 'prices.low', inLow);
  requireHistoryLength('MINUS_DM open', 'prices.low', inLow.length, inHigh.length);
  return minusDmOpenInternal(core, float64(inHigh), float64(inLow), 0, optInTimePeriod);
}

/** @internal */
export function minusDmOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): MinusDmStream {
  optInTimePeriod = requireInt32('MINUS_DM openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('MINUS_DM openAndFill', 'prices.high', inHigh);
  requireHistory('MINUS_DM openAndFill', 'prices.high', inHigh.length);
  requireArgument('MINUS_DM openAndFill', 'prices.low', inLow);
  const guardLookback = minusDmLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('MINUS_DM openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('MINUS_DM openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('MINUS_DM openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireOutput('MINUS_DM openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('MINUS_DM openAndFill', 'outReal', 'prices.high');
  }
  if (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('MINUS_DM openAndFill', 'outReal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outReal, inHighF64)) {
    throw sharedMemory('MINUS_DM openAndFill', 'outReal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outReal, inLowF64)) {
    throw sharedMemory('MINUS_DM openAndFill', 'outReal', 'prices.low');
  }
  return minusDmOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live MINUS_DM stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link minusDm} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MinusDmOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `minusDmLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range
 *   or histories of different lengths.
 *
 * @example
 * ```ts
 * import { minusDmOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = minusDmOpen({ high, low }, { timePeriod: 14 });
 * const peeked = stream.peek(101.4, 99.1); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1); // the bar closes
 * ```
 */
export function minusDmOpen(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: MinusDmOptions): MinusDmStream {
  requireArgument('MINUS_DM open', 'prices', prices);
  const o = optionsOf('MINUS_DM open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MINUS_DM open', o.core);
  return minusDmOpenPositional(prices.high, prices.low, optInTimePeriod, core);
}

/**
 * {@link minusDmOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link minusDm}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link MinusDmOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link minusDm} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `minusDmLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { minusDmOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = minusDmOpenAndFill({ high, low }, {
 *   timePeriod: 14,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function minusDmOpenAndFill(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: MinusDmOptions): MinusDmResult & { stream: MinusDmStream } {
  requireArgument('MINUS_DM openAndFill', 'prices', prices);
  const o = optionsOf('MINUS_DM openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MINUS_DM openAndFill', o.core);
  const capacity = fillCapacity(minusDmLookbackPositional(optInTimePeriod, core), prices.high, prices.low);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = minusDmOpenAndFillPositional(prices.high, prices.low, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
