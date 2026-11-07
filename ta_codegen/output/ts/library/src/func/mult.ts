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
export function multLookbackPositional(_core: Core = Core.DEFAULT): number {
  return 0;
}

/** @internal */
export function multDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function multImpl(
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
  outIdx = 0;
  i = startIdx;
  while (i <= endIdx) {
    outReal[outIdx] = inReal0[i] * inReal1[i];
    outIdx += 1;
    i += 1;
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function multPositional(
  startIdx: number,
  endIdx: number,
  inReal0: ArrayLike<number>,
  inReal1: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('MULT', 'startIdx', startIdx);
  endIdx = requireInt32('MULT', 'endIdx', endIdx);
  requireIndexRange('MULT', startIdx, endIdx);
  const guardStart = clampedStart('MULT', startIdx, multLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('MULT', 'real0', inReal0, guardInLen);
  requireLength('MULT', 'real1', inReal1, guardInLen);
  requireOutput('MULT', 'outReal', outReal, guardOutLen, Float64Array);
  const inReal0F64 = float64(inReal0);
  const inReal1F64 = float64(inReal1);
  if (overlapsPartly(outReal, inReal0F64)) {
    throw sharedMemory('MULT', 'outReal', 'real0');
  }
  if (overlapsPartly(outReal, inReal1F64)) {
    throw sharedMemory('MULT', 'outReal', 'real1');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = multImpl(core, startIdx, endIdx, inReal0F64, inReal1F64, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('MULT', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link mult}, {@link multLookback}, {@link multDisplayShift},
 * {@link multOpen} and {@link multOpenAndFill}. Every key is optional: an omitted
 * or `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface MultOptions {
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
  outReal?: Float64Array | undefined;
}

/** What {@link mult} returns, and {@link multOpenAndFill} with its stream. */
export interface MultResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Product of the two inputs at each index. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link mult} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MultOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function multLookback(options?: MultOptions): number {
  const o = optionsOf('MULT lookback', options);
  return multLookbackPositional(coreOf('MULT lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link mult}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MultOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function multDisplayShift(outputIdx: number, options?: MultOptions): number {
  const o = optionsOf('MULT displayShift', options);
  return multDisplayShiftPositional(outputIdx, coreOf('MULT displayShift', o.core));
}

/**
 * Element-wise multiplication of two input series.
 *
 * Formula and more info at
 * [ta-lib.org/functions/mult](https://ta-lib.org/functions/mult).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link multLookback} is a success with empty outputs.
 *
 * @param real0 - First operand series.
 * @param real1 - Second operand series.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link MultOptions}.
 * @returns `begIdx` and the values; see {@link MultResult}. Each output is a view
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
 * import { mult } from 'ta-lib';
 *
 * const data0 = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const data1 = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i + 0.7));
 * const { begIdx, values } = mult(data0, data1);
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`add`](https://ta-lib.org/functions/add)
 * @see [`sub`](https://ta-lib.org/functions/sub)
 * @see [`div`](https://ta-lib.org/functions/div)
 */
export function mult(real0: ArrayLike<number>, real1: ArrayLike<number>, options?: MultOptions): MultResult {
  const o = optionsOf('MULT', options);
  if (o.endIdx === undefined) {
    requireSeries('MULT', 'real0', real0);
    requireSeries('MULT', 'real1', real1);
  }
  const core = coreOf('MULT', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real0.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, multLookbackPositional(core), real0, real1);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = multPositional(startIdx, endIdx, real0, real1, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live MULT stream: one value per closed bar, bit-identical to {@link mult} over
 * the same series. Open it with {@link multOpen} or {@link multOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/mult](https://ta-lib.org/functions/mult).
 */
export class MultStream {
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
  static of(core: Core): MultStream {
    return new MultStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link mult} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('MULT advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real0 - The bar's `real0` value: First operand series.
   * @param real1 - The bar's `real1` value: Second operand series.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real0: number, real1: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('MULT update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real0) || !Number.isFinite(real1)) {
      throw nonFiniteBar('MULT update', !Number.isFinite(real0) ? 'real0' : 'real1');
    }
    multStepImpl(this, real0, real1);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real0 - The bar's `real0` value: First operand series.
   * @param real1 - The bar's `real1` value: Second operand series.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real0: number, real1: number): number {
    if (!Number.isFinite(real0) || !Number.isFinite(real1)) {
      throw nonFiniteBar('MULT peek', !Number.isFinite(real0) ? 'real0' : 'real1');
    }
    const inReal0 = real0;
    const inReal1 = real1;
    let cur_outReal = 0;
    cur_outReal = inReal0 * inReal1;
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
  clone(): MultStream {
    const c = new MultStream(this.core);
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function multStepImpl(sp: MultStream, inReal0: number, inReal1: number): void {
  sp.cur_outReal = inReal0 * inReal1;
}

function multOpenImpl(
  sp: MultStream,
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
  outIdx = 0;
  i = startIdx;
  while (i <= endIdx) {
    outReal[((outIdx * outStride) | 0)] = inReal0[i] * inReal1[i];
    outIdx += 1;
    i += 1;
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function multOpenAndFillInternal(
  core: Core,
  inReal0: Float64Array,
  inReal1: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): MultStream {
  const sp = MultStream.of(core);
  const retCode = multOpenImpl(sp, inReal0, inReal1, startIdx, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MULT openAndFill', inReal0.length, startIdx, multLookbackPositional(core));
  }
  throw streamFailure('MULT openAndFill', retCode);
}

/** @internal */
export function multOpenInternal(
  core: Core,
  inReal0: Float64Array,
  inReal1: Float64Array,
  startIdx: number,
): MultStream {
  const sp = MultStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = multOpenImpl(sp, inReal0, inReal1, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MULT open', inReal0.length, startIdx, multLookbackPositional(core));
  }
  throw streamFailure('MULT open', retCode);
}

/** @internal */
export function multOpenPositional(
  inReal0: ArrayLike<number>,
  inReal1: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): MultStream {
  requireArgument('MULT open', 'real0', inReal0);
  requireHistory('MULT open', 'real0', inReal0.length);
  requireArgument('MULT open', 'real1', inReal1);
  requireHistoryLength('MULT open', 'real1', inReal1.length, inReal0.length);
  return multOpenInternal(core, float64(inReal0), float64(inReal1), 0);
}

/** @internal */
export function multOpenAndFillPositional(
  inReal0: ArrayLike<number>,
  inReal1: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): MultStream {
  requireArgument('MULT openAndFill', 'real0', inReal0);
  requireHistory('MULT openAndFill', 'real0', inReal0.length);
  requireArgument('MULT openAndFill', 'real1', inReal1);
  const guardOutLen = openFillCount('MULT openAndFill', inReal0.length, multLookbackPositional(core));
  requireHistoryLength('MULT openAndFill', 'real1', inReal1.length, inReal0.length);
  requireOutput('MULT openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inReal0F64 = float64(inReal0);
  const inReal1F64 = float64(inReal1);
  if (outReal.buffer === inReal0F64.buffer && outReal.byteOffset === inReal0F64.byteOffset) {
    throw sharedMemory('MULT openAndFill', 'outReal', 'real0');
  }
  if (outReal.buffer === inReal1F64.buffer && outReal.byteOffset === inReal1F64.byteOffset) {
    throw sharedMemory('MULT openAndFill', 'outReal', 'real1');
  }
  if (guardOutLen > 0 && overlaps(outReal, inReal0F64)) {
    throw sharedMemory('MULT openAndFill', 'outReal', 'real0');
  }
  if (guardOutLen > 0 && overlaps(outReal, inReal1F64)) {
    throw sharedMemory('MULT openAndFill', 'outReal', 'real1');
  }
  return multOpenAndFillInternal(core, inReal0F64, inReal1F64, 0, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live MULT stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link mult} there.
 *
 * @param real0 - Warm-up history. First operand series.
 * @param real1 - Warm-up history. Second operand series.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MultOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `multLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input
 *   or histories of different lengths.
 *
 * @example
 * ```ts
 * import { multOpen } from 'ta-lib';
 *
 * const data0 = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const data1 = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i + 0.7));
 * const stream = multOpen(data0, data1);
 * const peeked = stream.peek(100.9, 101.3); // a forming bar: commits nothing
 * const updated = stream.update(100.9, 101.3); // the bar closes
 * ```
 */
export function multOpen(real0: ArrayLike<number>, real1: ArrayLike<number>, options?: MultOptions): MultStream {
  const o = optionsOf('MULT open', options);
  const core = coreOf('MULT open', o.core);
  return multOpenPositional(real0, real1, core);
}

/**
 * {@link multOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link mult}.
 *
 * @param real0 - Warm-up history. First operand series.
 * @param real1 - Warm-up history. Second operand series.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link MultOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link mult} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `multLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { multOpenAndFill } from 'ta-lib';
 *
 * const data0 = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const data1 = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i + 0.7));
 * const { stream, begIdx, values } = multOpenAndFill(data0, data1);
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function multOpenAndFill(real0: ArrayLike<number>, real1: ArrayLike<number>, options?: MultOptions): MultResult & { stream: MultStream } {
  const o = optionsOf('MULT openAndFill', options);
  const core = coreOf('MULT openAndFill', o.core);
  const capacity = fillCapacity(multLookbackPositional(core), real0, real1);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = multOpenAndFillPositional(real0, real1, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
