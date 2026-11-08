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
export function tanLookbackPositional(_core: Core = Core.DEFAULT): number {
  return 0;
}

/** @internal */
export function tanDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function tanImpl(
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
    outReal[outIdx] = Math.tan(inReal[i]);
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function tanPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('TAN', 'startIdx', startIdx);
  endIdx = requireInt32('TAN', 'endIdx', endIdx);
  requireIndexRange('TAN', startIdx, endIdx);
  const guardStart = clampedStart('TAN', startIdx, tanLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('TAN', 'real', inReal, guardInLen);
  requireOutput('TAN', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('TAN', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = tanImpl(core, startIdx, endIdx, inRealF64, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('TAN', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link tan}, {@link tanLookback}, {@link tanDisplayShift},
 * {@link tanOpen} and {@link tanOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface TanOptions {
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

/** What {@link tan} returns, and {@link tanOpenAndFill} with its stream. */
export interface TanResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /** tangent of each input. `values[k]` is the value at input bar `begIdx + k`. */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link tan} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link TanOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function tanLookback(options?: TanOptions): number {
  const o = optionsOf('TAN lookback', options);
  return tanLookbackPositional(coreOf('TAN lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link tan}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link TanOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function tanDisplayShift(outputIdx: number, options?: TanOptions): number {
  const o = optionsOf('TAN displayShift', options);
  return tanDisplayShiftPositional(outputIdx, coreOf('TAN displayShift', o.core));
}

/**
 * Element-wise tangent of the input series.
 *
 * Formula and more info at
 * [ta-lib.org/functions/tan](https://ta-lib.org/functions/tan).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link tanLookback} is a success with empty outputs.
 *
 * @param real - input values.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link TanOptions}.
 * @returns `begIdx` and the values; see {@link TanResult}. Each output is a view
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
 * import { tan } from 'ta-lib';
 *
 * const series = new Float64Array(252); // your data, oldest first
 *
 * const { begIdx, values } = tan(series);
 * console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
 * ```
 * @see [`atan`](https://ta-lib.org/functions/atan)
 * @see [`sin`](https://ta-lib.org/functions/sin)
 * @see [`cos`](https://ta-lib.org/functions/cos)
 * @see [`tanh`](https://ta-lib.org/functions/tanh)
 */
export function tan(real: ArrayLike<number>, options?: TanOptions): TanResult {
  const o = optionsOf('TAN', options);
  if (o.endIdx === undefined) {
    requireSeries('TAN', 'real', real);
  }
  const core = coreOf('TAN', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, tanLookbackPositional(core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = tanPositional(startIdx, endIdx, real, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live TAN stream: one value per closed bar, bit-identical to {@link tan} over
 * the same series. Open it with {@link tanOpen} or {@link tanOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/tan](https://ta-lib.org/functions/tan).
 */
export class TanStream {
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
  static of(core: Core): TanStream {
    return new TanStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link tan} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('TAN advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: input values.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('TAN update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('TAN update', 'real');
    }
    tanStepImpl(this, real);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: input values.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number): number {
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('TAN peek', 'real');
    }
    const inReal = real;
    let cur_outReal = 0;
    cur_outReal = Math.tan(inReal);
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
  clone(): TanStream {
    const c = new TanStream(this.core);
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function tanStepImpl(sp: TanStream, inReal: number): void {
  sp.cur_outReal = Math.tan(inReal);
}

function tanOpenImpl(
  sp: TanStream,
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
    outReal[((outIdx * outStride) | 0)] = Math.tan(inReal[i]);
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function tanOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): TanStream {
  const sp = TanStream.of(core);
  const retCode = tanOpenImpl(sp, inReal, startIdx, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('TAN openAndFill', inReal.length, startIdx, tanLookbackPositional(core));
  }
  throw streamFailure('TAN openAndFill', retCode);
}

/** @internal */
export function tanOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
): TanStream {
  const sp = TanStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = tanOpenImpl(sp, inReal, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('TAN open', inReal.length, startIdx, tanLookbackPositional(core));
  }
  throw streamFailure('TAN open', retCode);
}

/** @internal */
export function tanOpenPositional(
  inReal: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): TanStream {
  requireArgument('TAN open', 'real', inReal);
  requireHistory('TAN open', 'real', inReal.length);
  return tanOpenInternal(core, float64(inReal), 0);
}

/** @internal */
export function tanOpenAndFillPositional(
  inReal: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): TanStream {
  requireArgument('TAN openAndFill', 'real', inReal);
  requireHistory('TAN openAndFill', 'real', inReal.length);
  const guardOutLen = openFillCount('TAN openAndFill', inReal.length, tanLookbackPositional(core));
  requireOutput('TAN openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('TAN openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('TAN openAndFill', 'outReal', 'real');
  }
  return tanOpenAndFillInternal(core, inRealF64, 0, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live TAN stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link tan} there.
 *
 * @param real - Warm-up history. input values.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link TanOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `tanLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core` or an absent
 *   input.
 *
 * @example
 * ```ts
 * import { tanOpen } from 'ta-lib';
 *
 * const series = Float64Array.from({ length: 252 }, (_, i) =>
 *   -1.2 + 2.4 * i / 251.0);
 * const stream = tanOpen(series);
 * const peeked = stream.peek(0.504); // a forming bar: commits nothing
 * const updated = stream.update(0.504); // the bar closes
 * ```
 */
export function tanOpen(real: ArrayLike<number>, options?: TanOptions): TanStream {
  const o = optionsOf('TAN open', options);
  const core = coreOf('TAN open', o.core);
  return tanOpenPositional(real, core);
}

/**
 * {@link tanOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link tan}.
 *
 * @param real - Warm-up history. input values.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link TanOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link tan} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `tanLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input
 *   or a buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { tanOpenAndFill } from 'ta-lib';
 *
 * const series = Float64Array.from({ length: 252 }, (_, i) =>
 *   -1.2 + 2.4 * i / 251.0);
 * const { stream, begIdx, values } = tanOpenAndFill(series);
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function tanOpenAndFill(real: ArrayLike<number>, options?: TanOptions): TanResult & { stream: TanStream } {
  const o = optionsOf('TAN openAndFill', options);
  const core = coreOf('TAN openAndFill', o.core);
  const capacity = fillCapacity(tanLookbackPositional(core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = tanOpenAndFillPositional(real, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
