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

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link add} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function addLookback(core: Core = Core.DEFAULT): number {
  void core;
  return 0;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link add}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function addDisplayShift(outputIdx: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function addImpl(
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
  for (i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1) {
    outReal[outIdx] = inReal0[i] + inReal1[i];
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/**
 * Element-wise addition of two input series.
 *
 * Formula and more info at
 * [ta-lib.org/functions/add](https://ta-lib.org/functions/add).
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link addLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal0 - First operand series.
 * @param inReal1 - Second operand series.
 * @param outReal - Element-wise sum of the two inputs. Must hold the values
 *   produced and never be empty; the input length always suffices.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The range written: `begIdx` is the first bar with a value, `count` how
 *   many were written.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for an argument that is not an int32, an
 *   array too short for the range, an empty output or an output sharing memory with
 *   a `Float64Array` input it is not. Checked before anything is written.
 *
 * @example
 * ```ts
 * import { add } from 'ta-lib';
 *
 * const data0 = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const data1 = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i + 0.7));
 * const out = new Float64Array(data0.length);
 * const { begIdx, count } = add(0, data0.length - 1, data0, data1, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`sub`](https://ta-lib.org/functions/sub)
 * @see [`mult`](https://ta-lib.org/functions/mult)
 * @see [`div`](https://ta-lib.org/functions/div)
 */
export function add(
  startIdx: number,
  endIdx: number,
  inReal0: ArrayLike<number>,
  inReal1: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('ADD', 'startIdx', startIdx);
  endIdx = requireInt32('ADD', 'endIdx', endIdx);
  requireIndexRange('ADD', startIdx, endIdx);
  const guardStart = clampedStart('ADD', startIdx, addLookback(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('ADD', 'inReal0', inReal0, guardInLen);
  requireLength('ADD', 'inReal1', inReal1, guardInLen);
  requireOutput('ADD', 'outReal', outReal, guardOutLen, Float64Array);
  const inReal0F64 = float64(inReal0);
  const inReal1F64 = float64(inReal1);
  if (overlapsPartly(outReal, inReal0F64) || overlapsPartly(outReal, inReal1F64)) {
    throw failure('ADD', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = addImpl(core, startIdx, endIdx, inReal0F64, inReal1F64, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('ADD', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live ADD stream: one value per closed bar, bit-identical to {@link add} over
 * the same series. Open it with {@link addOpen} or {@link addOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/add](https://ta-lib.org/functions/add).
 */
export class AddStream {
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
  static of(core: Core): AddStream {
    return new AddStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link add} reports them over the same bars. Each accepted `update` and each
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
      throw failure('ADD advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal0 - The bar's `inReal0` value: First operand series.
   * @param inReal1 - The bar's `inReal1` value: Second operand series.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal0: number, inReal1: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('ADD update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal0) || !Number.isFinite(inReal1)) {
      throw nonFiniteBar('ADD update', !Number.isFinite(inReal0) ? 'inReal0' : 'inReal1');
    }
    addStepImpl(this, inReal0, inReal1);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal0 - The bar's `inReal0` value: First operand series.
   * @param inReal1 - The bar's `inReal1` value: Second operand series.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal0: number, inReal1: number): number {
    if (!Number.isFinite(inReal0) || !Number.isFinite(inReal1)) {
      throw nonFiniteBar('ADD peek', !Number.isFinite(inReal0) ? 'inReal0' : 'inReal1');
    }
    let cur_outReal = 0;
    cur_outReal = inReal0 + inReal1;
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
  clone(): AddStream {
    const c = new AddStream(this.core);
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function addStepImpl(sp: AddStream, inReal0: number, inReal1: number): void {
  sp.cur_outReal = inReal0 + inReal1;
}

function addOpenImpl(
  sp: AddStream,
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
  if (historyLen < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inReal1.length !== inReal0.length) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  for (i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1) {
    outReal[((outIdx * outStride) | 0)] = inReal0[i] + inReal1[i];
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function addOpenAndFillInternal(
  core: Core,
  inReal0: Float64Array,
  inReal1: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): AddStream {
  const sp = AddStream.of(core);
  const retCode = addOpenImpl(sp, inReal0, inReal1, startIdx, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ADD openAndFill', inReal0.length, startIdx, addLookback(core));
  }
  throw streamFailure('ADD openAndFill', retCode);
}

/** @internal */
export function addOpenInternal(
  core: Core,
  inReal0: Float64Array,
  inReal1: Float64Array,
  startIdx: number,
): AddStream {
  const sp = AddStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = addOpenImpl(sp, inReal0, inReal1, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ADD open', inReal0.length, startIdx, addLookback(core));
  }
  throw streamFailure('ADD open', retCode);
}

/**
 * Opens a live ADD stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link add} there.
 *
 * @param inReal0 - Warm-up history. First operand series. Every history has the
 *   same length.
 * @param inReal1 - Warm-up history. Second operand series. Every history has the
 *   same length.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `addLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   histories of different lengths.
 *
 * @example
 * ```ts
 * import { addOpen } from 'ta-lib';
 *
 * const data0 = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const data1 = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i + 0.7));
 * const stream = addOpen(data0, data1);
 * const peeked = stream.peek(100.9, 101.3); // a forming bar: commits nothing
 * const updated = stream.update(100.9, 101.3); // the bar closes
 * ```
 */
export function addOpen(
  inReal0: ArrayLike<number>,
  inReal1: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): AddStream {
  requireArgument('ADD open', 'inReal0', inReal0);
  requireHistory('ADD open', inReal0.length);
  requireArgument('ADD open', 'inReal1', inReal1);
  requireHistoryLength('ADD open', 'inReal1', inReal1.length, inReal0.length);
  return addOpenInternal(core, float64(inReal0), float64(inReal1), 0);
}

/**
 * {@link addOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link add}. The range written is the stream's
 * `outRange()`.
 *
 * @param inReal0 - Warm-up history. First operand series. Every history has the
 *   same length.
 * @param inReal1 - Warm-up history. Second operand series. Every history has the
 *   same length.
 * @param outReal - Element-wise sum of the two inputs. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `addLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   histories of different lengths or an output too short or starting where another
 *   output or a `Float64Array` input starts, or, on a history long enough to open,
 *   one sharing memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { addOpenAndFill } from 'ta-lib';
 *
 * const data0 = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const data1 = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i + 0.7));
 * const out = new Float64Array(data0.length);
 * const stream = addOpenAndFill(data0, data1, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function addOpenAndFill(
  inReal0: ArrayLike<number>,
  inReal1: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): AddStream {
  requireArgument('ADD openAndFill', 'inReal0', inReal0);
  requireHistory('ADD openAndFill', inReal0.length);
  requireArgument('ADD openAndFill', 'inReal1', inReal1);
  const guardOutLen = openFillCount('ADD openAndFill', inReal0.length, addLookback(core));
  requireHistoryLength('ADD openAndFill', 'inReal1', inReal1.length, inReal0.length);
  requireOutput('ADD openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inReal0F64 = float64(inReal0);
  const inReal1F64 = float64(inReal1);
  if ((outReal.buffer === inReal0F64.buffer && outReal.byteOffset === inReal0F64.byteOffset) || (outReal.buffer === inReal1F64.buffer && outReal.byteOffset === inReal1F64.byteOffset)) {
    throw streamFailure('ADD openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inReal0F64) || overlaps(outReal, inReal1F64))) {
    throw streamFailure('ADD openAndFill', RetCode.BAD_PARAM);
  }
  return addOpenAndFillInternal(core, inReal0F64, inReal1F64, 0, new MInteger(), new MInteger(), outReal);
}
