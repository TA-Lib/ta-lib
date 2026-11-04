/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AC       Angelo Ciceri
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  010802 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  110206 AC   Change volume and open interest to double
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link obv} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function obvLookback(core: Core = Core.DEFAULT): number {
  void core;
  /* This function have no lookback needed. */
  return 0;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link obv}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function obvDisplayShift(outputIdx: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function obvImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  inVolume: Float64Array,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let i = 0;
  let outIdx = 0;
  let prevReal = 0;
  let tempReal = 0;
  let prevOBV = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  prevOBV = inVolume[startIdx];
  prevReal = inReal[startIdx];
  outIdx = 0;
  for (i = startIdx; i <= endIdx; i += 1) {
    tempReal = inReal[i];
    if (tempReal > prevReal) {
      prevOBV += inVolume[i];
    } else if (tempReal < prevReal) {
      prevOBV -= inVolume[i];
    }
    outReal[outIdx++] = prevOBV;
    prevReal = tempReal;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * On Balance Volume: a running cumulative total of volume, added on up-price bars
 * and subtracted on down-price bars. Relates volume flow to price direction.
 *
 * Formula and more info at
 * [ta-lib.org/functions/obv](https://ta-lib.org/functions/obv).
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link obvLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - Price series, typically close.
 * @param inVolume - Volume of each bar.
 * @param outReal - Cumulative on-balance volume. Must hold the values produced and
 *   never be empty; the input length always suffices.
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
 * import { obv } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const out = new Float64Array(data.length);
 * const { begIdx, count } = obv(0, data.length - 1, data, volume, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function obv(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('OBV', 'startIdx', startIdx);
  endIdx = requireInt32('OBV', 'endIdx', endIdx);
  requireIndexRange('OBV', startIdx, endIdx);
  const guardStart = clampedStart('OBV', startIdx, obvLookback(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('OBV', 'inReal', inReal, guardInLen);
  requireLength('OBV', 'inVolume', inVolume, guardInLen);
  requireOutput('OBV', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  const inVolumeF64 = float64(inVolume);
  if (overlapsPartly(outReal, inRealF64) || overlapsPartly(outReal, inVolumeF64)) {
    throw failure('OBV', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = obvImpl(core, startIdx, endIdx, inRealF64, inVolumeF64, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('OBV', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live OBV stream: one value per closed bar, bit-identical to {@link obv} over
 * the same series. Open it with {@link obvOpen} or {@link obvOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/obv](https://ta-lib.org/functions/obv).
 */
export class ObvStream {
  /** @internal */
  core: Core;
  /** @internal */
  prevReal: number = 0;
  /** @internal */
  prevOBV: number = 0;
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
  static of(core: Core): ObvStream {
    return new ObvStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link obv} reports them over the same bars. Each accepted `update` and each
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
      throw failure('OBV advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: Price series, typically close.
   * @param inVolume - The bar's `inVolume` value: Volume of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number, inVolume: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('OBV update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal) || !Number.isFinite(inVolume)) {
      throw nonFiniteBar('OBV update', !Number.isFinite(inReal) ? 'inReal' : 'inVolume');
    }
    obvStepImpl(this, inReal, inVolume);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Price series, typically close.
   * @param inVolume - The bar's `inVolume` value: Volume of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number, inVolume: number): number {
    if (!Number.isFinite(inReal) || !Number.isFinite(inVolume)) {
      throw nonFiniteBar('OBV peek', !Number.isFinite(inReal) ? 'inReal' : 'inVolume');
    }
    const sp = this;
    let tempReal = 0;
    let cur_outReal = 0;
    let prevOBV = sp.prevOBV;
    tempReal = inReal;
    if (tempReal > sp.prevReal) {
      prevOBV += inVolume;
    } else if (tempReal < sp.prevReal) {
      prevOBV -= inVolume;
    }
    cur_outReal = prevOBV;
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
  clone(): ObvStream {
    const c = new ObvStream(this.core);
    c.prevReal = this.prevReal;
    c.prevOBV = this.prevOBV;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function obvStepImpl(sp: ObvStream, inReal: number, inVolume: number): void {
  let tempReal = 0;
  tempReal = inReal;
  if (tempReal > sp.prevReal) {
    sp.prevOBV += inVolume;
  } else if (tempReal < sp.prevReal) {
    sp.prevOBV -= inVolume;
  }
  sp.cur_outReal = sp.prevOBV;
  sp.prevReal = tempReal;
}

function obvOpenImpl(
  sp: ObvStream,
  inReal: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let i = 0;
  let outIdx = 0;
  let prevReal = 0;
  let tempReal = 0;
  let prevOBV = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inVolume.length !== inReal.length) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  prevOBV = inVolume[startIdx];
  prevReal = inReal[startIdx];
  outIdx = 0;
  for (i = startIdx; i <= endIdx; i += 1) {
    tempReal = inReal[i];
    if (tempReal > prevReal) {
      prevOBV += inVolume[i];
    } else if (tempReal < prevReal) {
      prevOBV -= inVolume[i];
    }
    outReal[((outIdx++ * outStride) | 0)] = prevOBV;
    prevReal = tempReal;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  sp.prevReal = prevReal;
  sp.prevOBV = prevOBV;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function obvOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): ObvStream {
  const sp = ObvStream.of(core);
  const retCode = obvOpenImpl(sp, inReal, inVolume, startIdx, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('OBV openAndFill', inReal.length, startIdx, obvLookback(core));
  }
  throw streamFailure('OBV openAndFill', retCode);
}

/** @internal */
export function obvOpenInternal(
  core: Core,
  inReal: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
): ObvStream {
  const sp = ObvStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = obvOpenImpl(sp, inReal, inVolume, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('OBV open', inReal.length, startIdx, obvLookback(core));
  }
  throw streamFailure('OBV open', retCode);
}

/**
 * Opens a live OBV stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link obv} there.
 *
 * @param inReal - Warm-up history. Price series, typically close. Every history
 *   has the same length.
 * @param inVolume - Warm-up history. Volume of each bar. Every history has the
 *   same length.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `obvLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for histories of different lengths.
 *
 * @example
 * ```ts
 * import { obvOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const stream = obvOpen(data, volume);
 * const peeked = stream.peek(100.9, 12345); // a forming bar: commits nothing
 * const updated = stream.update(100.9, 12345); // the bar closes
 * ```
 */
export function obvOpen(
  inReal: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): ObvStream {
  requireArgument('OBV open', 'inReal', inReal);
  requireHistory('OBV open', inReal.length);
  requireArgument('OBV open', 'inVolume', inVolume);
  requireHistoryLength('OBV open', 'inVolume', inVolume.length, inReal.length);
  return obvOpenInternal(core, float64(inReal), float64(inVolume), 0);
}

/**
 * {@link obvOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link obv}. The range written is the stream's
 * `outRange()`.
 *
 * @param inReal - Warm-up history. Price series, typically close. Every history
 *   has the same length.
 * @param inVolume - Warm-up history. Volume of each bar. Every history has the
 *   same length.
 * @param outReal - Cumulative on-balance volume. Must hold `historyLen - lookback`
 *   values and share no memory with an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `obvLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for histories of different lengths or an output too
 *   short or starting where another output or a `Float64Array` input starts, or, on
 *   a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { obvOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const out = new Float64Array(data.length);
 * const stream = obvOpenAndFill(data, volume, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function obvOpenAndFill(
  inReal: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): ObvStream {
  requireArgument('OBV openAndFill', 'inReal', inReal);
  requireHistory('OBV openAndFill', inReal.length);
  requireArgument('OBV openAndFill', 'inVolume', inVolume);
  const guardOutLen = openFillCount('OBV openAndFill', inReal.length, obvLookback(core));
  requireHistoryLength('OBV openAndFill', 'inVolume', inVolume.length, inReal.length);
  requireOutput('OBV openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  const inVolumeF64 = float64(inVolume);
  if ((outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) || (outReal.buffer === inVolumeF64.buffer && outReal.byteOffset === inVolumeF64.byteOffset)) {
    throw streamFailure('OBV openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inRealF64) || overlaps(outReal, inVolumeF64))) {
    throw streamFailure('OBV openAndFill', RetCode.BAD_PARAM);
  }
  return obvOpenAndFillInternal(core, inRealF64, inVolumeF64, 0, new MInteger(), new MInteger(), outReal);
}
