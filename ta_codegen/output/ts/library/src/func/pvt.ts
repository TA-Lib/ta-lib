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
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090426 MF,CC  First version (issue #364).
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function pvtLookbackPositional(_core: Core = Core.DEFAULT): number {
  /* This function have no lookback needed. */
  return 0;
}

/** @internal */
export function pvtDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function pvtImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inClose: Float64Array,
  inVolume: Float64Array,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let i = 0;
  let outIdx = 0;
  let prevPVT = 0;
  let prevClose = 0;
  let tempClose = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  prevPVT = 0.0;
  prevClose = inClose[startIdx];
  outIdx = 0;
  for (i = startIdx; i <= endIdx; i += 1) {
    tempClose = inClose[i];
    /* Exact test, never an epsilon band: a band carries the quote unit and
     * would zero the indicator for every instrument priced under it (#253).
     * A zero previous close contributes nothing rather than Inf/NaN (#112).
     */
    if (prevClose !== 0.0) {
      prevPVT += (tempClose - prevClose) / prevClose * inVolume[i];
    }
    outReal[outIdx++] = prevPVT;
    prevClose = tempClose;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function pvtPositional(
  startIdx: number,
  endIdx: number,
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('PVT', 'startIdx', startIdx);
  endIdx = requireInt32('PVT', 'endIdx', endIdx);
  requireIndexRange('PVT', startIdx, endIdx);
  const guardStart = clampedStart('PVT', startIdx, pvtLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('PVT', 'prices.close', inClose, guardInLen);
  requireLength('PVT', 'prices.volume', inVolume, guardInLen);
  requireOutput('PVT', 'outReal', outReal, guardOutLen, Float64Array);
  const inCloseF64 = float64(inClose);
  const inVolumeF64 = float64(inVolume);
  if (overlapsPartly(outReal, inCloseF64)) {
    throw sharedMemory('PVT', 'outReal', 'prices.close');
  }
  if (overlapsPartly(outReal, inVolumeF64)) {
    throw sharedMemory('PVT', 'outReal', 'prices.volume');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = pvtImpl(core, startIdx, endIdx, inCloseF64, inVolumeF64, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('PVT', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link pvt}, {@link pvtLookback}, {@link pvtDisplayShift},
 * {@link pvtOpen} and {@link pvtOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface PvtOptions {
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

/** What {@link pvt} returns, and {@link pvtOpenAndFill} with its stream. */
export interface PvtResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Cumulative price volume trend, seeded at zero. `values[k]` is the value at input
   * bar `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link pvt} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link PvtOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function pvtLookback(options?: PvtOptions): number {
  const o = optionsOf('PVT lookback', options);
  return pvtLookbackPositional(coreOf('PVT lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link pvt}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link PvtOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function pvtDisplayShift(outputIdx: number, options?: PvtOptions): number {
  const o = optionsOf('PVT displayShift', options);
  return pvtDisplayShiftPositional(outputIdx, coreOf('PVT displayShift', o.core));
}

/**
 * Price Volume Trend: a running cumulative total of each bar's volume weighted by
 * that bar's fractional price change. It is the On Balance Volume idea with
 * partial credit — where OBV adds or subtracts a bar's whole volume on the sign of
 * the move, PVT adds only the fraction of that volume proportional to the size of
 * the move. Read the slope and the divergences, not the level: the total's zero
 * point is arbitrary, so only the shape of the curve carries information. A rising
 * PVT while price is flat says volume is accumulating on the up moves; a falling
 * PVT while price rises is the classic bearish divergence.
 *
 * Formula and more info at
 * [ta-lib.org/functions/pvt](https://ta-lib.org/functions/pvt).
 *
 * **Notes**
 * - **The input is assumed positive.** The previous close is the divisor. Near
 *   zero the divisor is small and the output very large; no error is returned. A
 *   series that reaches or crosses zero, such as an oscillator or a spread, has no
 *   meaningful result here.
 * - The absolute level is arbitrary and depends on where the accumulation started,
 *   so two ranges over the same data give curves of the same shape at different
 *   offsets.
 * - Some libraries scale the per-bar term by 100. This implementation follows the
 *   fractional definition, which every primary reference below states.
 * - A bar whose previous close is exactly zero contributes nothing and the running
 *   total is carried forward unchanged, rather than dividing by zero. A flat
 *   stretch of the output can therefore mean either genuine zero net accumulation
 *   or a run of zero previous closes.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link pvtLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `close`: Close price of
 *   each bar. `volume`: Volume of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link PvtOptions}.
 * @returns `begIdx` and the values; see {@link PvtResult}. Each output is a view
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
 * import { pvt } from 'ta-lib';
 *
 * const close = new Float64Array(252); // your data, oldest first
 * const volume = new Float64Array(252); // your data, oldest first
 *
 * const { begIdx, values } = pvt({ close, volume });
 * console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
 * ```
 * @see [`obv`](https://ta-lib.org/functions/obv)
 * @see [`nvi`](https://ta-lib.org/functions/nvi)
 * @see [`pvi`](https://ta-lib.org/functions/pvi)
 * @see [`pvo`](https://ta-lib.org/functions/pvo)
 * @see [`ad`](https://ta-lib.org/functions/ad)
 */
export function pvt(prices: { open?: ArrayLike<number> | undefined; high?: ArrayLike<number> | undefined; low?: ArrayLike<number> | undefined; close: ArrayLike<number>; volume: ArrayLike<number>; openInterest?: ArrayLike<number> | undefined }, options?: PvtOptions): PvtResult {
  requireArgument('PVT', 'prices', prices);
  const o = optionsOf('PVT', options);
  if (o.endIdx === undefined) {
    requireSeries('PVT', 'prices.close', prices.close);
    requireSeries('PVT', 'prices.volume', prices.volume);
  }
  const core = coreOf('PVT', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.close.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, pvtLookbackPositional(core), prices.close, prices.volume);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = pvtPositional(startIdx, endIdx, prices.close, prices.volume, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live PVT stream: one value per closed bar, bit-identical to {@link pvt} over
 * the same series. Open it with {@link pvtOpen} or {@link pvtOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/pvt](https://ta-lib.org/functions/pvt).
 */
export class PvtStream {
  /** @internal */
  core: Core;
  /** @internal */
  prevPVT: number = 0;
  /** @internal */
  prevClose: number = 0;
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
  static of(core: Core): PvtStream {
    return new PvtStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link pvt} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('PVT advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param close - The bar's `close` value: Close price of each bar.
   * @param volume - The bar's `volume` value: Volume of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(close: number, volume: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('PVT update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(close) || !Number.isFinite(volume)) {
      throw nonFiniteBar('PVT update', !Number.isFinite(close) ? 'close' : 'volume');
    }
    pvtStepImpl(this, close, volume);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param close - The bar's `close` value: Close price of each bar.
   * @param volume - The bar's `volume` value: Volume of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(close: number, volume: number): number {
    if (!Number.isFinite(close) || !Number.isFinite(volume)) {
      throw nonFiniteBar('PVT peek', !Number.isFinite(close) ? 'close' : 'volume');
    }
    const sp = this;
    const inClose = close;
    const inVolume = volume;
    let tempClose = 0;
    let cur_outReal = 0;
    let prevPVT = sp.prevPVT;
    tempClose = inClose;
    /* Exact test, never an epsilon band: a band carries the quote unit and
     * would zero the indicator for every instrument priced under it (#253).
     * A zero previous close contributes nothing rather than Inf/NaN (#112).
     */
    if (sp.prevClose !== 0.0) {
      prevPVT += (tempClose - sp.prevClose) / sp.prevClose * inVolume;
    }
    cur_outReal = prevPVT;
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
  clone(): PvtStream {
    const c = new PvtStream(this.core);
    c.prevPVT = this.prevPVT;
    c.prevClose = this.prevClose;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function pvtStepImpl(sp: PvtStream, inClose: number, inVolume: number): void {
  let tempClose = 0;
  tempClose = inClose;
  /* Exact test, never an epsilon band: a band carries the quote unit and
   * would zero the indicator for every instrument priced under it (#253).
   * A zero previous close contributes nothing rather than Inf/NaN (#112).
   */
  if (sp.prevClose !== 0.0) {
    sp.prevPVT += (tempClose - sp.prevClose) / sp.prevClose * inVolume;
  }
  sp.cur_outReal = sp.prevPVT;
  sp.prevClose = tempClose;
}

function pvtOpenImpl(
  sp: PvtStream,
  inClose: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let i = 0;
  let outIdx = 0;
  let prevPVT = 0;
  let prevClose = 0;
  let tempClose = 0;
  const historyLen = inClose.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inVolume.length !== inClose.length) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  prevPVT = 0.0;
  prevClose = inClose[startIdx];
  outIdx = 0;
  for (i = startIdx; i <= endIdx; i += 1) {
    tempClose = inClose[i];
    /* Exact test, never an epsilon band: a band carries the quote unit and
     * would zero the indicator for every instrument priced under it (#253).
     * A zero previous close contributes nothing rather than Inf/NaN (#112).
     */
    if (prevClose !== 0.0) {
      prevPVT += (tempClose - prevClose) / prevClose * inVolume[i];
    }
    outReal[((outIdx++ * outStride) | 0)] = prevPVT;
    prevClose = tempClose;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  sp.prevPVT = prevPVT;
  sp.prevClose = prevClose;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function pvtOpenAndFillInternal(
  core: Core,
  inClose: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): PvtStream {
  const sp = PvtStream.of(core);
  const retCode = pvtOpenImpl(sp, inClose, inVolume, startIdx, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('PVT openAndFill', inClose.length, startIdx, pvtLookbackPositional(core));
  }
  throw streamFailure('PVT openAndFill', retCode);
}

/** @internal */
export function pvtOpenInternal(
  core: Core,
  inClose: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
): PvtStream {
  const sp = PvtStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = pvtOpenImpl(sp, inClose, inVolume, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('PVT open', inClose.length, startIdx, pvtLookbackPositional(core));
  }
  throw streamFailure('PVT open', retCode);
}

/** @internal */
export function pvtOpenPositional(
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): PvtStream {
  requireArgument('PVT open', 'prices.close', inClose);
  requireHistory('PVT open', 'prices.close', inClose.length);
  requireArgument('PVT open', 'prices.volume', inVolume);
  requireHistoryLength('PVT open', 'prices.volume', inVolume.length, inClose.length);
  return pvtOpenInternal(core, float64(inClose), float64(inVolume), 0);
}

/** @internal */
export function pvtOpenAndFillPositional(
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): PvtStream {
  requireArgument('PVT openAndFill', 'prices.close', inClose);
  requireHistory('PVT openAndFill', 'prices.close', inClose.length);
  requireArgument('PVT openAndFill', 'prices.volume', inVolume);
  const guardOutLen = openFillCount('PVT openAndFill', inClose.length, pvtLookbackPositional(core));
  requireHistoryLength('PVT openAndFill', 'prices.volume', inVolume.length, inClose.length);
  requireOutput('PVT openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inCloseF64 = float64(inClose);
  const inVolumeF64 = float64(inVolume);
  if (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('PVT openAndFill', 'outReal', 'prices.close');
  }
  if (outReal.buffer === inVolumeF64.buffer && outReal.byteOffset === inVolumeF64.byteOffset) {
    throw sharedMemory('PVT openAndFill', 'outReal', 'prices.volume');
  }
  if (guardOutLen > 0 && overlaps(outReal, inCloseF64)) {
    throw sharedMemory('PVT openAndFill', 'outReal', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outReal, inVolumeF64)) {
    throw sharedMemory('PVT openAndFill', 'outReal', 'prices.volume');
  }
  return pvtOpenAndFillInternal(core, inCloseF64, inVolumeF64, 0, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live PVT stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link pvt} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `close`: Close price of each bar. `volume`: Volume of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link PvtOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `pvtLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input
 *   or histories of different lengths.
 *
 * @example
 * ```ts
 * import { pvtOpen } from 'ta-lib';
 *
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const stream = pvtOpen({ close, volume });
 * const peeked = stream.peek(100.9, 12345); // a forming bar: commits nothing
 * const updated = stream.update(100.9, 12345); // the bar closes
 * ```
 */
export function pvtOpen(prices: { open?: ArrayLike<number> | undefined; high?: ArrayLike<number> | undefined; low?: ArrayLike<number> | undefined; close: ArrayLike<number>; volume: ArrayLike<number>; openInterest?: ArrayLike<number> | undefined }, options?: PvtOptions): PvtStream {
  requireArgument('PVT open', 'prices', prices);
  const o = optionsOf('PVT open', options);
  const core = coreOf('PVT open', o.core);
  return pvtOpenPositional(prices.close, prices.volume, core);
}

/**
 * {@link pvtOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link pvt}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `close`: Close price of each bar. `volume`: Volume of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link PvtOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link pvt} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `pvtLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { pvtOpenAndFill } from 'ta-lib';
 *
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const { stream, begIdx, values } = pvtOpenAndFill({ close, volume });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function pvtOpenAndFill(prices: { open?: ArrayLike<number> | undefined; high?: ArrayLike<number> | undefined; low?: ArrayLike<number> | undefined; close: ArrayLike<number>; volume: ArrayLike<number>; openInterest?: ArrayLike<number> | undefined }, options?: PvtOptions): PvtResult & { stream: PvtStream } {
  requireArgument('PVT openAndFill', 'prices', prices);
  const o = optionsOf('PVT openAndFill', options);
  const core = coreOf('PVT openAndFill', o.core);
  const capacity = fillCapacity(pvtLookbackPositional(core), prices.close, prices.volume);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = pvtOpenAndFillPositional(prices.close, prices.volume, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
