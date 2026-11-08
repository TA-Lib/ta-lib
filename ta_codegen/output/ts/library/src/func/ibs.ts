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
 *  092926 KL,CC  Creation (#468).
 *  092926 MF,CC  Store the quotient, then overwrite it, so the loop vectorizes.
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function ibsLookbackPositional(_core: Core = Core.DEFAULT): number {
  /* One bar in, one bar out: the value reads only its own bar, so there is
   * nothing to warm up. bop.c:19-22's lookback, and stochf_lookback(1,1,SMA)
   * agrees, since ma_lookback returns 0 for a period of 1 (ma.c:28-29).
   */
  return 0;
}

/** @internal */
export function ibsDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function ibsImpl(
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
  let outIdx = 0;
  let i = 0;
  let range = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  /* IBS = (Close - Low)/(High - Low): where the close sits inside its own
   * bar, 0 at the low and 1 at the high.
   */
  outIdx = 0;
  for (i = startIdx; i <= endIdx; i += 1) {
    /* An exact test: the range of one bar carries the quote unit, so any
     * band would flip every bar of a small enough instrument to 0.5. Spelled
     * <= so a NaN input keeps the quotient's NaN.
     *
     * Store, then overwrite: gcc keeps a guarded or selected quotient
     * scalar under -ftrapping-math.
     */
    range = inHigh[i] - inLow[i];
    outReal[outIdx] = (inClose[i] - inLow[i]) / range;
    if (range <= 0.0) {
      outReal[outIdx] = 0.5;
    }
    outIdx += 1;
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function ibsPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('IBS', 'startIdx', startIdx);
  endIdx = requireInt32('IBS', 'endIdx', endIdx);
  requireIndexRange('IBS', startIdx, endIdx);
  const guardStart = clampedStart('IBS', startIdx, ibsLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('IBS', 'prices.high', inHigh, guardInLen);
  requireLength('IBS', 'prices.low', inLow, guardInLen);
  requireLength('IBS', 'prices.close', inClose, guardInLen);
  requireOutput('IBS', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outReal, inHighF64)) {
    throw sharedMemory('IBS', 'outReal', 'prices.high');
  }
  if (overlapsPartly(outReal, inLowF64)) {
    throw sharedMemory('IBS', 'outReal', 'prices.low');
  }
  if (overlapsPartly(outReal, inCloseF64)) {
    throw sharedMemory('IBS', 'outReal', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = ibsImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('IBS', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link ibs}, {@link ibsLookback}, {@link ibsDisplayShift},
 * {@link ibsOpen} and {@link ibsOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface IbsOptions {
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

/** What {@link ibs} returns, and {@link ibsOpenAndFill} with its stream. */
export interface IbsResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Position of the close within the bar's range, 0 to 1 on a bar with range.
   * `values[k]` is the value at input bar `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link ibs} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link IbsOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function ibsLookback(options?: IbsOptions): number {
  const o = optionsOf('IBS lookback', options);
  return ibsLookbackPositional(coreOf('IBS lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link ibs}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link IbsOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function ibsDisplayShift(outputIdx: number, options?: IbsOptions): number {
  const o = optionsOf('IBS displayShift', options);
  return ibsDisplayShiftPositional(outputIdx, coreOf('IBS displayShift', o.core));
}

/**
 * Internal Bar Strength: where the close sits inside its own bar's range. It is 0
 * when the bar closes at its low, 1 at its high and 0.5 at the midpoint, and it
 * reads one bar only, with no window and no state. It is the closing half of a
 * stochastic oscillator taken over a single bar, which is how the paper that named
 * the effect describes it. Low readings say the session ended on weakness, high
 * readings on strength, and the published use is mean reversion on the next bar
 * rather than trend confirmation.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ibs](https://ta-lib.org/functions/ibs).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link ibsLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `high`: High price of
 *   each bar. `low`: Low price of each bar. `close`: Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link IbsOptions}.
 * @returns `begIdx` and the values; see {@link IbsResult}. Each output is a view
 *   of its buffer, the caller's or a new one.
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
 * import { ibs } from 'ta-lib';
 *
 * const high = new Float64Array(252); // your data, oldest first
 * const low = new Float64Array(252); // your data, oldest first
 * const close = new Float64Array(252); // your data, oldest first
 *
 * const { begIdx, values } = ibs({ high, low, close });
 * console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
 * ```
 */
export function ibs(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: IbsOptions): IbsResult {
  requireArgument('IBS', 'prices', prices);
  const o = optionsOf('IBS', options);
  if (o.endIdx === undefined) {
    requireSeries('IBS', 'prices.high', prices.high);
    requireSeries('IBS', 'prices.low', prices.low);
    requireSeries('IBS', 'prices.close', prices.close);
  }
  const core = coreOf('IBS', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, ibsLookbackPositional(core), prices.high, prices.low, prices.close);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = ibsPositional(startIdx, endIdx, prices.high, prices.low, prices.close, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live IBS stream: one value per closed bar, bit-identical to {@link ibs} over
 * the same series. Open it with {@link ibsOpen} or {@link ibsOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ibs](https://ta-lib.org/functions/ibs).
 */
export class IbsStream {
  /** @internal */
  core: Core;
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
  static of(core: Core): IbsStream {
    return new IbsStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link ibs} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('IBS advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw streamFailure('IBS update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('IBS update', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    ibsStepImpl(this, high, low, close);
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
      throw nonFiniteBar('IBS peek', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const inHigh = high;
    const inLow = low;
    const inClose = close;
    let range = 0;
    let cur_outReal = 0;
    /* An exact test: the range of one bar carries the quote unit, so any
     * band would flip every bar of a small enough instrument to 0.5. Spelled
     * <= so a NaN input keeps the quotient's NaN.
     *
     * Store, then overwrite: gcc keeps a guarded or selected quotient
     * scalar under -ftrapping-math.
     */
    range = inHigh - inLow;
    cur_outReal = (inClose - inLow) / range;
    if (range <= 0.0) {
      cur_outReal = 0.5;
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
  clone(): IbsStream {
    const c = new IbsStream(this.core);
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function ibsStepImpl(sp: IbsStream, inHigh: number, inLow: number, inClose: number): void {
  let range = 0;
  /* An exact test: the range of one bar carries the quote unit, so any
   * band would flip every bar of a small enough instrument to 0.5. Spelled
   * <= so a NaN input keeps the quotient's NaN.
   *
   * Store, then overwrite: gcc keeps a guarded or selected quotient
   * scalar under -ftrapping-math.
   */
  range = inHigh - inLow;
  sp.cur_outReal = (inClose - inLow) / range;
  if (range <= 0.0) {
    sp.cur_outReal = 0.5;
  }
}

function ibsOpenImpl(
  sp: IbsStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let outIdx = 0;
  let i = 0;
  let range = 0;
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
  /* IBS = (Close - Low)/(High - Low): where the close sits inside its own
   * bar, 0 at the low and 1 at the high.
   */
  outIdx = 0;
  for (i = startIdx; i <= endIdx; i += 1) {
    /* An exact test: the range of one bar carries the quote unit, so any
     * band would flip every bar of a small enough instrument to 0.5. Spelled
     * <= so a NaN input keeps the quotient's NaN.
     *
     * Store, then overwrite: gcc keeps a guarded or selected quotient
     * scalar under -ftrapping-math.
     */
    range = inHigh[i] - inLow[i];
    outReal[((outIdx * outStride) | 0)] = (inClose[i] - inLow[i]) / range;
    if (range <= 0.0) {
      outReal[((outIdx * outStride) | 0)] = 0.5;
    }
    outIdx += 1;
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function ibsOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): IbsStream {
  const sp = IbsStream.of(core);
  const retCode = ibsOpenImpl(sp, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('IBS openAndFill', inHigh.length, startIdx, ibsLookbackPositional(core));
  }
  throw streamFailure('IBS openAndFill', retCode);
}

/** @internal */
export function ibsOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): IbsStream {
  const sp = IbsStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = ibsOpenImpl(sp, inHigh, inLow, inClose, startIdx + 1 < inHigh.length ? inHigh.length - 1 : startIdx, outBegIdx, outNBElement, sink_outReal, 0);
  if (retCode === RetCode.SUCCESS) {
    const begIdx = ibsLookbackPositional(core);
    sp.outRangeBegIdx = begIdx < startIdx ? startIdx : begIdx;
    sp.outRangeCount = inHigh.length - sp.outRangeBegIdx;
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('IBS open', inHigh.length, startIdx, ibsLookbackPositional(core));
  }
  throw streamFailure('IBS open', retCode);
}

/** @internal */
export function ibsOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): IbsStream {
  requireArgument('IBS open', 'prices.high', inHigh);
  requireHistory('IBS open', 'prices.high', inHigh.length);
  requireArgument('IBS open', 'prices.low', inLow);
  requireArgument('IBS open', 'prices.close', inClose);
  requireHistoryLength('IBS open', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('IBS open', 'prices.close', inClose.length, inHigh.length);
  return ibsOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0);
}

/** @internal */
export function ibsOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): IbsStream {
  requireArgument('IBS openAndFill', 'prices.high', inHigh);
  requireHistory('IBS openAndFill', 'prices.high', inHigh.length);
  requireArgument('IBS openAndFill', 'prices.low', inLow);
  requireArgument('IBS openAndFill', 'prices.close', inClose);
  const guardOutLen = openFillCount('IBS openAndFill', inHigh.length, ibsLookbackPositional(core));
  requireHistoryLength('IBS openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('IBS openAndFill', 'prices.close', inClose.length, inHigh.length);
  requireOutput('IBS openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('IBS openAndFill', 'outReal', 'prices.high');
  }
  if (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('IBS openAndFill', 'outReal', 'prices.low');
  }
  if (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('IBS openAndFill', 'outReal', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outReal, inHighF64)) {
    throw sharedMemory('IBS openAndFill', 'outReal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outReal, inLowF64)) {
    throw sharedMemory('IBS openAndFill', 'outReal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outReal, inCloseF64)) {
    throw sharedMemory('IBS openAndFill', 'outReal', 'prices.close');
  }
  return ibsOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live IBS stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link ibs} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar. `close`: Close
 *   price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link IbsOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `ibsLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input
 *   or histories of different lengths.
 *
 * @example
 * ```ts
 * import { ibsOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = ibsOpen({ high, low, close });
 * const peeked = stream.peek(101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function ibsOpen(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: IbsOptions): IbsStream {
  requireArgument('IBS open', 'prices', prices);
  const o = optionsOf('IBS open', options);
  const core = coreOf('IBS open', o.core);
  return ibsOpenPositional(prices.high, prices.low, prices.close, core);
}

/**
 * {@link ibsOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link ibs}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar. `close`: Close
 *   price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link IbsOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link ibs} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `ibsLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { ibsOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = ibsOpenAndFill({ high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function ibsOpenAndFill(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: IbsOptions): IbsResult & { stream: IbsStream } {
  requireArgument('IBS openAndFill', 'prices', prices);
  const o = optionsOf('IBS openAndFill', options);
  const core = coreOf('IBS openAndFill', o.core);
  const capacity = fillCapacity(ibsLookbackPositional(core), prices.high, prices.low, prices.close);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = ibsOpenAndFillPositional(prices.high, prices.low, prices.close, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
