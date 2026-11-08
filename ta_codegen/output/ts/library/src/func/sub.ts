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
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090807 MF     Initial Version
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function subLookbackPositional(_core: Core = Core.DEFAULT): number {
  return 0;
}

/** @internal */
export function subDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function subImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inReal0: Float64Array,
  inReal1: Float64Array,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let outIdx = 0;
  let i = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  /* Default return values */
  for (i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1) {
    outReal[outIdx] = inReal0[i] - inReal1[i];
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function subPositional(
  startIdx: number,
  endIdx: number,
  inReal0: ArrayLike<number>,
  inReal1: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('SUB', 'startIdx', startIdx);
  endIdx = requireInt32('SUB', 'endIdx', endIdx);
  requireIndexRange('SUB', startIdx, endIdx);
  const guardStart = clampedStart('SUB', startIdx, subLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('SUB', 'real0', inReal0, guardInLen);
  requireLength('SUB', 'real1', inReal1, guardInLen);
  requireOutput('SUB', 'outReal', outReal, guardOutLen, Float64Array);
  const inReal0F64 = float64(inReal0);
  const inReal1F64 = float64(inReal1);
  if (overlapsPartly(outReal, inReal0F64)) {
    throw sharedMemory('SUB', 'outReal', 'real0');
  }
  if (overlapsPartly(outReal, inReal1F64)) {
    throw sharedMemory('SUB', 'outReal', 'real1');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = subImpl(core, startIdx, endIdx, inReal0F64, inReal1F64, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('SUB', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link sub}, {@link subLookback}, {@link subDisplayShift},
 * {@link subOpen} and {@link subOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface SubOptions {
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

/** What {@link sub} returns, and {@link subOpenAndFill} with its stream. */
export interface SubResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Per-element difference real0 - real1. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link sub} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SubOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function subLookback(options?: SubOptions): number {
  const o = optionsOf('SUB lookback', options);
  return subLookbackPositional(coreOf('SUB lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link sub}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SubOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function subDisplayShift(outputIdx: number, options?: SubOptions): number {
  const o = optionsOf('SUB displayShift', options);
  return subDisplayShiftPositional(outputIdx, coreOf('SUB displayShift', o.core));
}

/**
 * Element-wise subtraction of two input series.
 *
 * Formula and more info at
 * [ta-lib.org/functions/sub](https://ta-lib.org/functions/sub).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link subLookback} is a success with empty outputs.
 *
 * @param real0 - Minuend series.
 * @param real1 - Subtrahend series.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link SubOptions}.
 * @returns `begIdx` and the values; see {@link SubResult}. Each output is a view
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
 * import { sub } from 'ta-lib';
 *
 * const series0 = new Float64Array(252); // your data, oldest first
 * const series1 = new Float64Array(252); // your data, oldest first
 *
 * const { begIdx, values } = sub(series0, series1);
 * console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
 * ```
 * @see [`add`](https://ta-lib.org/functions/add)
 * @see [`mult`](https://ta-lib.org/functions/mult)
 * @see [`div`](https://ta-lib.org/functions/div)
 */
export function sub(real0: ArrayLike<number>, real1: ArrayLike<number>, options?: SubOptions): SubResult {
  const o = optionsOf('SUB', options);
  if (o.endIdx === undefined) {
    requireSeries('SUB', 'real0', real0);
    requireSeries('SUB', 'real1', real1);
  }
  const core = coreOf('SUB', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real0.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, subLookbackPositional(core), real0, real1);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = subPositional(startIdx, endIdx, real0, real1, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live SUB stream: one value per closed bar, bit-identical to {@link sub} over
 * the same series. Open it with {@link subOpen} or {@link subOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/sub](https://ta-lib.org/functions/sub).
 */
export class SubStream {
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
  static of(core: Core): SubStream {
    return new SubStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link sub} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('SUB advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real0 - The bar's `real0` value: Minuend series.
   * @param real1 - The bar's `real1` value: Subtrahend series.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real0: number, real1: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('SUB update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real0) || !Number.isFinite(real1)) {
      throw nonFiniteBar('SUB update', !Number.isFinite(real0) ? 'real0' : 'real1');
    }
    subStepImpl(this, real0, real1);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real0 - The bar's `real0` value: Minuend series.
   * @param real1 - The bar's `real1` value: Subtrahend series.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real0: number, real1: number): number {
    if (!Number.isFinite(real0) || !Number.isFinite(real1)) {
      throw nonFiniteBar('SUB peek', !Number.isFinite(real0) ? 'real0' : 'real1');
    }
    const inReal0 = real0;
    const inReal1 = real1;
    let cur_outReal = 0;
    cur_outReal = inReal0 - inReal1;
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
  clone(): SubStream {
    const c = new SubStream(this.core);
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function subStepImpl(sp: SubStream, inReal0: number, inReal1: number): void {
  sp.cur_outReal = inReal0 - inReal1;
}

function subOpenImpl(
  sp: SubStream,
  inReal0: Float64Array,
  inReal1: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let outIdx = 0;
  let i = 0;
  const historyLen = inReal0.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inReal1.length !== inReal0.length) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Default return values */
  for (i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1) {
    outReal[((outIdx * outStride) | 0)] = inReal0[i] - inReal1[i];
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function subOpenAndFillInternal(
  core: Core,
  inReal0: Float64Array,
  inReal1: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): SubStream {
  const sp = SubStream.of(core);
  const retCode = subOpenImpl(sp, inReal0, inReal1, startIdx, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SUB openAndFill', inReal0.length, startIdx, subLookbackPositional(core));
  }
  throw streamFailure('SUB openAndFill', retCode);
}

/** @internal */
export function subOpenInternal(
  core: Core,
  inReal0: Float64Array,
  inReal1: Float64Array,
  startIdx: number,
): SubStream {
  const sp = SubStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = subOpenImpl(sp, inReal0, inReal1, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SUB open', inReal0.length, startIdx, subLookbackPositional(core));
  }
  throw streamFailure('SUB open', retCode);
}

/** @internal */
export function subOpenPositional(
  inReal0: ArrayLike<number>,
  inReal1: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): SubStream {
  requireArgument('SUB open', 'real0', inReal0);
  requireHistory('SUB open', 'real0', inReal0.length);
  requireArgument('SUB open', 'real1', inReal1);
  requireHistoryLength('SUB open', 'real1', inReal1.length, inReal0.length);
  return subOpenInternal(core, float64(inReal0), float64(inReal1), 0);
}

/** @internal */
export function subOpenAndFillPositional(
  inReal0: ArrayLike<number>,
  inReal1: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): SubStream {
  requireArgument('SUB openAndFill', 'real0', inReal0);
  requireHistory('SUB openAndFill', 'real0', inReal0.length);
  requireArgument('SUB openAndFill', 'real1', inReal1);
  const guardOutLen = openFillCount('SUB openAndFill', inReal0.length, subLookbackPositional(core));
  requireHistoryLength('SUB openAndFill', 'real1', inReal1.length, inReal0.length);
  requireOutput('SUB openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inReal0F64 = float64(inReal0);
  const inReal1F64 = float64(inReal1);
  if (outReal.buffer === inReal0F64.buffer && outReal.byteOffset === inReal0F64.byteOffset) {
    throw sharedMemory('SUB openAndFill', 'outReal', 'real0');
  }
  if (outReal.buffer === inReal1F64.buffer && outReal.byteOffset === inReal1F64.byteOffset) {
    throw sharedMemory('SUB openAndFill', 'outReal', 'real1');
  }
  if (guardOutLen > 0 && overlaps(outReal, inReal0F64)) {
    throw sharedMemory('SUB openAndFill', 'outReal', 'real0');
  }
  if (guardOutLen > 0 && overlaps(outReal, inReal1F64)) {
    throw sharedMemory('SUB openAndFill', 'outReal', 'real1');
  }
  return subOpenAndFillInternal(core, inReal0F64, inReal1F64, 0, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live SUB stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link sub} there.
 *
 * @param real0 - Warm-up history. Minuend series.
 * @param real1 - Warm-up history. Subtrahend series.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SubOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `subLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input
 *   or histories of different lengths.
 *
 * @example
 * ```ts
 * import { subOpen } from 'ta-lib';
 *
 * const series0 = Float64Array.from({ length: 252 }, (_, i) =>
 *   10 * Math.sin(0.1 * i));
 * const series1 = Float64Array.from({ length: 252 }, (_, i) =>
 *   -5 + 10 * i / 251.0);
 * const stream = subOpen(series0, series1);
 * const peeked = stream.peek(4.2, 2.1); // a forming bar: commits nothing
 * const updated = stream.update(4.2, 2.1); // the bar closes
 * ```
 */
export function subOpen(real0: ArrayLike<number>, real1: ArrayLike<number>, options?: SubOptions): SubStream {
  const o = optionsOf('SUB open', options);
  const core = coreOf('SUB open', o.core);
  return subOpenPositional(real0, real1, core);
}

/**
 * {@link subOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link sub}.
 *
 * @param real0 - Warm-up history. Minuend series.
 * @param real1 - Warm-up history. Subtrahend series.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link SubOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link sub} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `subLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { subOpenAndFill } from 'ta-lib';
 *
 * const series0 = Float64Array.from({ length: 252 }, (_, i) =>
 *   10 * Math.sin(0.1 * i));
 * const series1 = Float64Array.from({ length: 252 }, (_, i) =>
 *   -5 + 10 * i / 251.0);
 * const { stream, begIdx, values } = subOpenAndFill(series0, series1);
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function subOpenAndFill(real0: ArrayLike<number>, real1: ArrayLike<number>, options?: SubOptions): SubResult & { stream: SubStream } {
  const o = optionsOf('SUB openAndFill', options);
  const core = coreOf('SUB openAndFill', o.core);
  const capacity = fillCapacity(subLookbackPositional(core), real0, real1);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = subOpenAndFillPositional(real0, real1, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
