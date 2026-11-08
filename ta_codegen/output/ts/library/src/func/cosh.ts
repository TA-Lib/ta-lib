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

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function coshLookbackPositional(_core: Core = Core.DEFAULT): number {
  return 0;
}

/** @internal */
export function coshDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function coshImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
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
  for (i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1) {
    outReal[outIdx] = Math.cosh(inReal[i]);
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function coshPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('COSH', 'startIdx', startIdx);
  endIdx = requireInt32('COSH', 'endIdx', endIdx);
  requireIndexRange('COSH', startIdx, endIdx);
  const guardStart = clampedStart('COSH', startIdx, coshLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('COSH', 'real', inReal, guardInLen);
  requireOutput('COSH', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('COSH', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = coshImpl(core, startIdx, endIdx, inRealF64, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('COSH', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cosh}, {@link coshLookback}, {@link coshDisplayShift},
 * {@link coshOpen} and {@link coshOpenAndFill}. Every key is optional: an omitted
 * or `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface CoshOptions {
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

/** What {@link cosh} returns, and {@link coshOpenAndFill} with its stream. */
export interface CoshResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Hyperbolic cosine of each input. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link cosh} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CoshOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function coshLookback(options?: CoshOptions): number {
  const o = optionsOf('COSH lookback', options);
  return coshLookbackPositional(coreOf('COSH lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cosh}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CoshOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function coshDisplayShift(outputIdx: number, options?: CoshOptions): number {
  const o = optionsOf('COSH displayShift', options);
  return coshDisplayShiftPositional(outputIdx, coreOf('COSH displayShift', o.core));
}

/**
 * Element-wise hyperbolic cosine of the input series.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cosh](https://ta-lib.org/functions/cosh).
 *
 * **Notes**
 * - An input beyond about 710.48 in magnitude has a result too large for a double,
 *   so those elements come out positive infinity.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link coshLookback} is a success with empty outputs.
 *
 * @param real - Input values to transform.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link CoshOptions}.
 * @returns `begIdx` and the values; see {@link CoshResult}. Each output is a view
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
 * import { cosh } from 'ta-lib';
 *
 * const series = new Float64Array(252); // your data, oldest first
 *
 * const { begIdx, values } = cosh(series);
 * console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
 * ```
 * @see [`sinh`](https://ta-lib.org/functions/sinh)
 * @see [`tanh`](https://ta-lib.org/functions/tanh)
 * @see [`cos`](https://ta-lib.org/functions/cos)
 */
export function cosh(real: ArrayLike<number>, options?: CoshOptions): CoshResult {
  const o = optionsOf('COSH', options);
  if (o.endIdx === undefined) {
    requireSeries('COSH', 'real', real);
  }
  const core = coreOf('COSH', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, coshLookbackPositional(core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = coshPositional(startIdx, endIdx, real, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live COSH stream: one value per closed bar, bit-identical to {@link cosh} over
 * the same series. Open it with {@link coshOpen} or {@link coshOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cosh](https://ta-lib.org/functions/cosh).
 */
export class CoshStream {
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
  static of(core: Core): CoshStream {
    return new CoshStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cosh} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('COSH advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: Input values to transform.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('COSH update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('COSH update', 'real');
    }
    coshStepImpl(this, real);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: Input values to transform.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number): number {
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('COSH peek', 'real');
    }
    const inReal = real;
    let cur_outReal = 0;
    cur_outReal = Math.cosh(inReal);
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
  clone(): CoshStream {
    const c = new CoshStream(this.core);
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function coshStepImpl(sp: CoshStream, inReal: number): void {
  sp.cur_outReal = Math.cosh(inReal);
}

function coshOpenImpl(
  sp: CoshStream,
  inReal: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let outIdx = 0;
  let i = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  for (i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1) {
    outReal[((outIdx * outStride) | 0)] = Math.cosh(inReal[i]);
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function coshOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): CoshStream {
  const sp = CoshStream.of(core);
  const retCode = coshOpenImpl(sp, inReal, startIdx, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('COSH openAndFill', inReal.length, startIdx, coshLookbackPositional(core));
  }
  throw streamFailure('COSH openAndFill', retCode);
}

/** @internal */
export function coshOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
): CoshStream {
  const sp = CoshStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = coshOpenImpl(sp, inReal, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('COSH open', inReal.length, startIdx, coshLookbackPositional(core));
  }
  throw streamFailure('COSH open', retCode);
}

/** @internal */
export function coshOpenPositional(
  inReal: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): CoshStream {
  requireArgument('COSH open', 'real', inReal);
  requireHistory('COSH open', 'real', inReal.length);
  return coshOpenInternal(core, float64(inReal), 0);
}

/** @internal */
export function coshOpenAndFillPositional(
  inReal: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): CoshStream {
  requireArgument('COSH openAndFill', 'real', inReal);
  requireHistory('COSH openAndFill', 'real', inReal.length);
  const guardOutLen = openFillCount('COSH openAndFill', inReal.length, coshLookbackPositional(core));
  requireOutput('COSH openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('COSH openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('COSH openAndFill', 'outReal', 'real');
  }
  return coshOpenAndFillInternal(core, inRealF64, 0, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live COSH stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link cosh} there.
 *
 * @param real - Warm-up history. Input values to transform.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CoshOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `coshLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core` or an absent
 *   input.
 *
 * @example
 * ```ts
 * import { coshOpen } from 'ta-lib';
 *
 * const series = Float64Array.from({ length: 252 }, (_, i) =>
 *   -5 + 10 * i / 251.0);
 * const stream = coshOpen(series);
 * const peeked = stream.peek(2.1); // a forming bar: commits nothing
 * const updated = stream.update(2.1); // the bar closes
 * ```
 */
export function coshOpen(real: ArrayLike<number>, options?: CoshOptions): CoshStream {
  const o = optionsOf('COSH open', options);
  const core = coreOf('COSH open', o.core);
  return coshOpenPositional(real, core);
}

/**
 * {@link coshOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link cosh}.
 *
 * @param real - Warm-up history. Input values to transform.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CoshOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cosh} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `coshLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input
 *   or a buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { coshOpenAndFill } from 'ta-lib';
 *
 * const series = Float64Array.from({ length: 252 }, (_, i) =>
 *   -5 + 10 * i / 251.0);
 * const { stream, begIdx, values } = coshOpenAndFill(series);
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function coshOpenAndFill(real: ArrayLike<number>, options?: CoshOptions): CoshResult & { stream: CoshStream } {
  const o = optionsOf('COSH openAndFill', options);
  const core = coreOf('COSH openAndFill', o.core);
  const capacity = fillCapacity(coshLookbackPositional(core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = coshOpenAndFillPositional(real, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
