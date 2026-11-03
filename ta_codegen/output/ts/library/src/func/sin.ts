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
 *  082507 MF     Initial Version
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link sin} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function sinLookback(core: Core = Core.DEFAULT): number {
  void core;
  return 0;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link sin}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function sinDisplayShift(outputIdx: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function sinImpl(
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
    outReal[outIdx] = Math.sin(inReal[i]);
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/**
 * Element-wise sine of the input series.
 *
 * Formula and more info at
 * [ta-lib.org/functions/sin](https://ta-lib.org/functions/sin).
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link sinLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - Input values (radians)
 * @param outReal - Sine of each input. Must hold the values produced and never be
 *   empty; the input length always suffices.
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
 * import { sin } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const { begIdx, count } = sin(0, data.length - 1, data, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`cos`](https://ta-lib.org/functions/cos)
 * @see [`tan`](https://ta-lib.org/functions/tan)
 * @see [`asin`](https://ta-lib.org/functions/asin)
 */
export function sin(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('SIN', 'startIdx', startIdx);
  endIdx = requireInt32('SIN', 'endIdx', endIdx);
  requireIndexRange('SIN', startIdx, endIdx);
  const guardStart = clampedStart('SIN', startIdx, sinLookback(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('SIN', 'inReal', inReal, guardInLen);
  requireLength('SIN', 'outReal', outReal, guardOutLen);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw failure('SIN', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = sinImpl(core, startIdx, endIdx, inRealF64, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('SIN', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live SIN stream: one value per closed bar, bit-identical to {@link sin} over
 * the same series. Open it with {@link sinOpen} or {@link sinOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/sin](https://ta-lib.org/functions/sin).
 */
export class SinStream {
  /** @internal */
  core: Core;
  /** @internal */
  cur_outReal: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  /** @internal */
  constructor(core: Core) {
    this.core = core;
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link sin} reports them over the same bars. Each accepted `update` and each
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
      throw failure('SIN advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: Input values (radians)
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('SIN update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('SIN update', 'inReal');
    }
    sinStepImpl(this, inReal);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Input values (radians)
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number): number {
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('SIN peek', 'inReal');
    }
    let cur_outReal = 0;
    cur_outReal = Math.sin(inReal);
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
  clone(): SinStream {
    const c = new SinStream(this.core);
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function sinStepImpl(sp: SinStream, inReal: number): void {
  sp.cur_outReal = Math.sin(inReal);
}

function sinOpenImpl(
  sp: SinStream,
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
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  for (i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1) {
    outReal[((outIdx * outStride) | 0)] = Math.sin(inReal[i]);
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function sinOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): SinStream {
  const sp = new SinStream(core);
  const retCode = sinOpenImpl(sp, inReal, startIdx, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SIN openAndFill', inReal.length, startIdx, sinLookback(core));
  }
  throw streamFailure('SIN openAndFill', retCode);
}

/** @internal */
export function sinOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
): SinStream {
  const sp = new SinStream(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = sinOpenImpl(sp, inReal, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SIN open', inReal.length, startIdx, sinLookback(core));
  }
  throw streamFailure('SIN open', retCode);
}

/**
 * Opens a live SIN stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link sin} there.
 *
 * @param inReal - Warm-up history. Input values (radians)
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `sinLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one.
 *
 * @example
 * ```ts
 * import { sinOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = sinOpen(data);
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function sinOpen(
  inReal: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): SinStream {
  requireArgument('SIN open', 'inReal', inReal);
  requireHistory('SIN open', inReal.length);
  return sinOpenInternal(core, float64(inReal), 0);
}

/**
 * {@link sinOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link sin}. The range written is the stream's
 * `outRange()`.
 *
 * @param inReal - Warm-up history. Input values (radians)
 * @param outReal - Sine of each input. Must hold `historyLen - lookback` values
 *   and share no memory with an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `sinLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an output too short or starting where another output
 *   or a `Float64Array` input starts, or, on a history long enough to open, one
 *   sharing memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { sinOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const stream = sinOpenAndFill(data, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function sinOpenAndFill(
  inReal: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): SinStream {
  requireArgument('SIN openAndFill', 'inReal', inReal);
  requireHistory('SIN openAndFill', inReal.length);
  const guardOutLen = openFillCount('SIN openAndFill', inReal.length, sinLookback(core));
  requireLength('SIN openAndFill', 'outReal', outReal, guardOutLen);
  const inRealF64 = float64(inReal);
  if ((outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset)) {
    throw streamFailure('SIN openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inRealF64))) {
    throw streamFailure('SIN openAndFill', RetCode.BAD_PARAM);
  }
  return sinOpenAndFillInternal(core, inRealF64, 0, new MInteger(), new MInteger(), outReal);
}
