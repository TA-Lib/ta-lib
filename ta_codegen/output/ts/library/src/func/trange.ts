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
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function trangeLookbackPositional(_core: Core = Core.DEFAULT): number {
  return 1;
}

/** @internal */
export function trangeDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function trangeImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let today = 0;
  let outIdx = 0;
  let val2 = 0;
  let val3 = 0;
  let greatest = 0;
  let tempCY = 0;
  let tempLT = 0;
  let tempHT = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  /* True Range is the greatest of the following:
   *
   *  val1 = distance from today's high to today's low.
   *  val2 = distance from yesterday's close to today's high.
   *  val3 = distance from yesterday's close to today's low.
   *
   * Some books and software makes the first TR value to be
   * the (high - low) of the first bar. This function instead
   * ignore the first price bar, and only output starting at the
   * second price bar are valid. This is done for avoiding
   * inconsistency.
   */
  /* Move up the start index if there is not
   * enough initial data.
   * Always one price bar gets consumed.
   */
  if (startIdx < 1) {
    startIdx = 1;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  outIdx = 0;
  today = startIdx;
  while (today <= endIdx) {
    /* Find the greatest of the 3 values. */
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    outReal[outIdx++] = greatest;
    today += 1;
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function trangePositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('TRANGE', 'startIdx', startIdx);
  endIdx = requireInt32('TRANGE', 'endIdx', endIdx);
  requireIndexRange('TRANGE', startIdx, endIdx);
  const guardStart = clampedStart('TRANGE', startIdx, trangeLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('TRANGE', 'prices.high', inHigh, guardInLen);
  requireLength('TRANGE', 'prices.low', inLow, guardInLen);
  requireLength('TRANGE', 'prices.close', inClose, guardInLen);
  requireOutput('TRANGE', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outReal, inHighF64)) {
    throw sharedMemory('TRANGE', 'outReal', 'prices.high');
  }
  if (overlapsPartly(outReal, inLowF64)) {
    throw sharedMemory('TRANGE', 'outReal', 'prices.low');
  }
  if (overlapsPartly(outReal, inCloseF64)) {
    throw sharedMemory('TRANGE', 'outReal', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = trangeImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('TRANGE', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link trange}, {@link trangeLookback}, {@link trangeDisplayShift},
 * {@link trangeOpen} and {@link trangeOpenAndFill}. Every key is optional: an
 * omitted or `undefined` key takes its default, and a call ignores the keys it has
 * no use for.
 */
export interface TrangeOptions {
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

/** What {@link trange} returns, and {@link trangeOpenAndFill} with its stream. */
export interface TrangeResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /** True Range value per bar. `values[k]` is the value at input bar `begIdx + k`. */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link trange} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link TrangeOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function trangeLookback(options?: TrangeOptions): number {
  const o = optionsOf('TRANGE lookback', options);
  return trangeLookbackPositional(coreOf('TRANGE lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link trange}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link TrangeOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function trangeDisplayShift(outputIdx: number, options?: TrangeOptions): number {
  const o = optionsOf('TRANGE displayShift', options);
  return trangeDisplayShiftPositional(outputIdx, coreOf('TRANGE displayShift', o.core));
}

/**
 * True Range: the greatest of today's high-low span and the two gaps between
 * yesterday's close and today's high/low. Base volatility measure used to build
 * ATR/NATR. Larger values mean wider or gappier bars (higher volatility).
 *
 * Formula and more info at
 * [ta-lib.org/functions/trange](https://ta-lib.org/functions/trange).
 *
 * **Notes**
 * - The first bar produces no value because it has no prior close; unlike some
 *   definitions, it does not fall back to the high-low range for that bar.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link trangeLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `high`: High price of
 *   each bar. `low`: Low price of each bar. `close`: Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link TrangeOptions}.
 * @returns `begIdx` and the values; see {@link TrangeResult}. Each output is a
 *   view of its buffer, the caller's or a new one.
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
 * import { trange } from 'ta-lib';
 *
 * const high = new Float64Array(252); // your data, oldest first
 * const low = new Float64Array(252); // your data, oldest first
 * const close = new Float64Array(252); // your data, oldest first
 *
 * const { begIdx, values } = trange({ high, low, close });
 * console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
 * ```
 * @see [`atr`](https://ta-lib.org/functions/atr)
 * @see [`natr`](https://ta-lib.org/functions/natr)
 */
export function trange(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: TrangeOptions): TrangeResult {
  requireArgument('TRANGE', 'prices', prices);
  const o = optionsOf('TRANGE', options);
  if (o.endIdx === undefined) {
    requireSeries('TRANGE', 'prices.high', prices.high);
    requireSeries('TRANGE', 'prices.low', prices.low);
    requireSeries('TRANGE', 'prices.close', prices.close);
  }
  const core = coreOf('TRANGE', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, trangeLookbackPositional(core), prices.high, prices.low, prices.close);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = trangePositional(startIdx, endIdx, prices.high, prices.low, prices.close, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live TRANGE stream: one value per closed bar, bit-identical to {@link trange}
 * over the same series. Open it with {@link trangeOpen} or
 * {@link trangeOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/trange](https://ta-lib.org/functions/trange).
 */
export class TrangeStream {
  /** @internal */
  core: Core;
  /** @internal */
  lag1_inClose: number = 0;
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
  static of(core: Core): TrangeStream {
    return new TrangeStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link trange} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('TRANGE advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(high: number, low: number, close: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('TRANGE update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('TRANGE update', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    trangeStepImpl(this, high, low, close);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(high: number, low: number, close: number): number {
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('TRANGE peek', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
    let val2 = 0;
    let val3 = 0;
    let greatest = 0;
    let tempCY = 0;
    let tempLT = 0;
    let tempHT = 0;
    let cur_outReal = 0;
    /* Find the greatest of the 3 values. */
    tempLT = inLow;
    tempHT = inHigh;
    tempCY = sp.lag1_inClose;
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    cur_outReal = greatest;
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
  clone(): TrangeStream {
    const c = new TrangeStream(this.core);
    c.lag1_inClose = this.lag1_inClose;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function trangeStepImpl(sp: TrangeStream, inHigh: number, inLow: number, inClose: number): void {
  let val2 = 0;
  let val3 = 0;
  let greatest = 0;
  let tempCY = 0;
  let tempLT = 0;
  let tempHT = 0;
  /* Find the greatest of the 3 values. */
  tempLT = inLow;
  tempHT = inHigh;
  tempCY = sp.lag1_inClose;
  greatest = tempHT - tempLT;
  /* val1 */
  val2 = Math.abs(tempCY - tempHT);
  if (val2 > greatest) {
    greatest = val2;
  }
  val3 = Math.abs(tempCY - tempLT);
  if (val3 > greatest) {
    greatest = val3;
  }
  sp.cur_outReal = greatest;
  sp.lag1_inClose = inClose;
}

function trangeOpenImpl(
  sp: TrangeStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let today = 0;
  let outIdx = 0;
  let val2 = 0;
  let val3 = 0;
  let greatest = 0;
  let tempCY = 0;
  let tempLT = 0;
  let tempHT = 0;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length || inClose.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* True Range is the greatest of the following:
   *
   *  val1 = distance from today's high to today's low.
   *  val2 = distance from yesterday's close to today's high.
   *  val3 = distance from yesterday's close to today's low.
   *
   * Some books and software makes the first TR value to be
   * the (high - low) of the first bar. This function instead
   * ignore the first price bar, and only output starting at the
   * second price bar are valid. This is done for avoiding
   * inconsistency.
   */
  /* Move up the start index if there is not
   * enough initial data.
   * Always one price bar gets consumed.
   */
  if (startIdx < 1) {
    startIdx = 1;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outIdx = 0;
  today = startIdx;
  while (today <= endIdx) {
    /* Find the greatest of the 3 values. */
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    outReal[((outIdx++ * outStride) | 0)] = greatest;
    today += 1;
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function trangeOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): TrangeStream {
  const sp = TrangeStream.of(core);
  const retCode = trangeOpenImpl(sp, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('TRANGE openAndFill', inHigh.length, startIdx, trangeLookbackPositional(core));
  }
  throw streamFailure('TRANGE openAndFill', retCode);
}

/** @internal */
export function trangeOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): TrangeStream {
  const sp = TrangeStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = trangeOpenImpl(sp, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('TRANGE open', inHigh.length, startIdx, trangeLookbackPositional(core));
  }
  throw streamFailure('TRANGE open', retCode);
}

/** @internal */
export function trangeOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): TrangeStream {
  requireArgument('TRANGE open', 'prices.high', inHigh);
  requireHistory('TRANGE open', 'prices.high', inHigh.length);
  requireArgument('TRANGE open', 'prices.low', inLow);
  requireArgument('TRANGE open', 'prices.close', inClose);
  requireHistoryLength('TRANGE open', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('TRANGE open', 'prices.close', inClose.length, inHigh.length);
  return trangeOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0);
}

/** @internal */
export function trangeOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): TrangeStream {
  requireArgument('TRANGE openAndFill', 'prices.high', inHigh);
  requireHistory('TRANGE openAndFill', 'prices.high', inHigh.length);
  requireArgument('TRANGE openAndFill', 'prices.low', inLow);
  requireArgument('TRANGE openAndFill', 'prices.close', inClose);
  const guardOutLen = openFillCount('TRANGE openAndFill', inHigh.length, trangeLookbackPositional(core));
  requireHistoryLength('TRANGE openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('TRANGE openAndFill', 'prices.close', inClose.length, inHigh.length);
  requireOutput('TRANGE openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('TRANGE openAndFill', 'outReal', 'prices.high');
  }
  if (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('TRANGE openAndFill', 'outReal', 'prices.low');
  }
  if (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('TRANGE openAndFill', 'outReal', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outReal, inHighF64)) {
    throw sharedMemory('TRANGE openAndFill', 'outReal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outReal, inLowF64)) {
    throw sharedMemory('TRANGE openAndFill', 'outReal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outReal, inCloseF64)) {
    throw sharedMemory('TRANGE openAndFill', 'outReal', 'prices.close');
  }
  return trangeOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live TRANGE stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link trange} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar. `close`: Close
 *   price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link TrangeOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `trangeLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input or histories of different lengths.
 *
 * @example
 * ```ts
 * import { trangeOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = trangeOpen({ high, low, close });
 * const peeked = stream.peek(101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function trangeOpen(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: TrangeOptions): TrangeStream {
  requireArgument('TRANGE open', 'prices', prices);
  const o = optionsOf('TRANGE open', options);
  const core = coreOf('TRANGE open', o.core);
  return trangeOpenPositional(prices.high, prices.low, prices.close, core);
}

/**
 * {@link trangeOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link trange}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar. `close`: Close
 *   price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link TrangeOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link trange} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `trangeLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, histories of different lengths or a buffer too short, of the wrong type
 *   or starting where another buffer or a `Float64Array` input starts, or, on a
 *   history long enough to open, one sharing memory with another buffer or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { trangeOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = trangeOpenAndFill({ high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function trangeOpenAndFill(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: TrangeOptions): TrangeResult & { stream: TrangeStream } {
  requireArgument('TRANGE openAndFill', 'prices', prices);
  const o = optionsOf('TRANGE openAndFill', options);
  const core = coreOf('TRANGE openAndFill', o.core);
  const capacity = fillCapacity(trangeLookbackPositional(core), prices.high, prices.low, prices.close);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = trangeOpenAndFillPositional(prices.high, prices.low, prices.close, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
