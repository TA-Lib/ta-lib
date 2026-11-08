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
export function log10LookbackPositional(_core: Core = Core.DEFAULT): number {
  return 0;
}

/** @internal */
export function log10DisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function log10Impl(
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
    outReal[outIdx] = Math.log10(inReal[i]);
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function log10Positional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('LOG10', 'startIdx', startIdx);
  endIdx = requireInt32('LOG10', 'endIdx', endIdx);
  requireIndexRange('LOG10', startIdx, endIdx);
  const guardStart = clampedStart('LOG10', startIdx, log10LookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('LOG10', 'real', inReal, guardInLen);
  requireOutput('LOG10', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('LOG10', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = log10Impl(core, startIdx, endIdx, inRealF64, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('LOG10', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link log10}, {@link log10Lookback}, {@link log10DisplayShift},
 * {@link log10Open} and {@link log10OpenAndFill}. Every key is optional: an
 * omitted or `undefined` key takes its default, and a call ignores the keys it has
 * no use for.
 */
export interface Log10Options {
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

/** What {@link log10} returns, and {@link log10OpenAndFill} with its stream. */
export interface Log10Result {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Base-10 logarithm of each input. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link log10} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link Log10Options}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function log10Lookback(options?: Log10Options): number {
  const o = optionsOf('LOG10 lookback', options);
  return log10LookbackPositional(coreOf('LOG10 lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link log10}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link Log10Options}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function log10DisplayShift(outputIdx: number, options?: Log10Options): number {
  const o = optionsOf('LOG10 displayShift', options);
  return log10DisplayShiftPositional(outputIdx, coreOf('LOG10 displayShift', o.core));
}

/**
 * Element-wise base-10 logarithm of the input series.
 *
 * Formula and more info at
 * [ta-lib.org/functions/log10](https://ta-lib.org/functions/log10).
 *
 * **Notes**
 * - The logarithm is defined only for positive values: a negative input gives NaN,
 *   and a zero input gives negative infinity.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link log10Lookback} is a success with empty outputs.
 *
 * @param real - Input values.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link Log10Options}.
 * @returns `begIdx` and the values; see {@link Log10Result}. Each output is a view
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
 * import { log10 } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = log10(data);
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`ln`](https://ta-lib.org/functions/ln)
 * @see [`exp`](https://ta-lib.org/functions/exp)
 */
export function log10(real: ArrayLike<number>, options?: Log10Options): Log10Result {
  const o = optionsOf('LOG10', options);
  if (o.endIdx === undefined) {
    requireSeries('LOG10', 'real', real);
  }
  const core = coreOf('LOG10', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, log10LookbackPositional(core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = log10Positional(startIdx, endIdx, real, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live LOG10 stream: one value per closed bar, bit-identical to {@link log10}
 * over the same series. Open it with {@link log10Open} or
 * {@link log10OpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/log10](https://ta-lib.org/functions/log10).
 */
export class Log10Stream {
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
  static of(core: Core): Log10Stream {
    return new Log10Stream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link log10} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('LOG10 advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: Input values.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('LOG10 update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('LOG10 update', 'real');
    }
    log10StepImpl(this, real);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: Input values.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number): number {
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('LOG10 peek', 'real');
    }
    const inReal = real;
    let cur_outReal = 0;
    cur_outReal = Math.log10(inReal);
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
  clone(): Log10Stream {
    const c = new Log10Stream(this.core);
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function log10StepImpl(sp: Log10Stream, inReal: number): void {
  sp.cur_outReal = Math.log10(inReal);
}

function log10OpenImpl(
  sp: Log10Stream,
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
    outReal[((outIdx * outStride) | 0)] = Math.log10(inReal[i]);
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function log10OpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): Log10Stream {
  const sp = Log10Stream.of(core);
  const retCode = log10OpenImpl(sp, inReal, startIdx, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('LOG10 openAndFill', inReal.length, startIdx, log10LookbackPositional(core));
  }
  throw streamFailure('LOG10 openAndFill', retCode);
}

/** @internal */
export function log10OpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
): Log10Stream {
  const sp = Log10Stream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = log10OpenImpl(sp, inReal, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('LOG10 open', inReal.length, startIdx, log10LookbackPositional(core));
  }
  throw streamFailure('LOG10 open', retCode);
}

/** @internal */
export function log10OpenPositional(
  inReal: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): Log10Stream {
  requireArgument('LOG10 open', 'real', inReal);
  requireHistory('LOG10 open', 'real', inReal.length);
  return log10OpenInternal(core, float64(inReal), 0);
}

/** @internal */
export function log10OpenAndFillPositional(
  inReal: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): Log10Stream {
  requireArgument('LOG10 openAndFill', 'real', inReal);
  requireHistory('LOG10 openAndFill', 'real', inReal.length);
  const guardOutLen = openFillCount('LOG10 openAndFill', inReal.length, log10LookbackPositional(core));
  requireOutput('LOG10 openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('LOG10 openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('LOG10 openAndFill', 'outReal', 'real');
  }
  return log10OpenAndFillInternal(core, inRealF64, 0, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live LOG10 stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link log10} there.
 *
 * @param real - Warm-up history. Input values.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link Log10Options}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `log10Lookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core` or an absent
 *   input.
 *
 * @example
 * ```ts
 * import { log10Open } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = log10Open(data);
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function log10Open(real: ArrayLike<number>, options?: Log10Options): Log10Stream {
  const o = optionsOf('LOG10 open', options);
  const core = coreOf('LOG10 open', o.core);
  return log10OpenPositional(real, core);
}

/**
 * {@link log10Open} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link log10}.
 *
 * @param real - Warm-up history. Input values.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link Log10Options}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link log10} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `log10Lookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input or a buffer too short, of the wrong type or starting where another buffer
 *   or a `Float64Array` input starts, or, on a history long enough to open, one
 *   sharing memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { log10OpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = log10OpenAndFill(data);
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function log10OpenAndFill(real: ArrayLike<number>, options?: Log10Options): Log10Result & { stream: Log10Stream } {
  const o = optionsOf('LOG10 openAndFill', options);
  const core = coreOf('LOG10 openAndFill', o.core);
  const capacity = fillCapacity(log10LookbackPositional(core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = log10OpenAndFillPositional(real, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
