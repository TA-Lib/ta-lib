/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  112605 MF   Fix outBegIdx when startIdx != 0
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link medprice} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function medpriceLookback(core: Core = Core.DEFAULT): number {
  void core;
  /* This function have no lookback needed. */
  return 0;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link medprice}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function medpriceDisplayShift(outputIdx: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function medpriceImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
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
  /* MEDPRICE = (High + Low ) / 2
   * This is the high and low of the same price bar.
   *
   * See MIDPRICE to use instead the highest high and lowest
   * low over multiple price bar.
   */
  outIdx = 0;
  for (i = startIdx; i <= endIdx; i += 1) {
    outReal[outIdx++] = (inHigh[i] + inLow[i]) / 2.0;
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/**
 * Median Price: the midpoint of each bar's high and low. A price-transform
 * overlay.
 *
 * Formula and more info at
 * [ta-lib.org/functions/medprice](https://ta-lib.org/functions/medprice).
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link medpriceLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param outReal - Midpoint of each bar's high and low. Must hold the values
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
 * import { medprice } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(high.length);
 * const { begIdx, count } = medprice(0, high.length - 1, high, low, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`midprice`](https://ta-lib.org/functions/midprice)
 * @see [`avgprice`](https://ta-lib.org/functions/avgprice)
 * @see [`typprice`](https://ta-lib.org/functions/typprice)
 * @see [`wclprice`](https://ta-lib.org/functions/wclprice)
 */
export function medprice(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('MEDPRICE', 'startIdx', startIdx);
  endIdx = requireInt32('MEDPRICE', 'endIdx', endIdx);
  requireIndexRange('MEDPRICE', startIdx, endIdx);
  const guardStart = clampedStart('MEDPRICE', startIdx, medpriceLookback(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('MEDPRICE', 'inHigh', inHigh, guardInLen);
  requireLength('MEDPRICE', 'inLow', inLow, guardInLen);
  requireLength('MEDPRICE', 'outReal', outReal, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlapsPartly(outReal, inHighF64) || overlapsPartly(outReal, inLowF64)) {
    throw failure('MEDPRICE', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = medpriceImpl(core, startIdx, endIdx, inHighF64, inLowF64, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('MEDPRICE', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live MEDPRICE stream: one value per closed bar, bit-identical to
 * {@link medprice} over the same series. Open it with {@link medpriceOpen} or
 * {@link medpriceOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/medprice](https://ta-lib.org/functions/medprice).
 */
export class MedpriceStream {
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
   * {@link medprice} reports them over the same bars. Each accepted `update` and
   * each `advance` adds one to `count`.
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
      throw failure('MEDPRICE advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('MEDPRICE update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('MEDPRICE update', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    medpriceStepImpl(this, inHigh, inLow);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number): number {
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('MEDPRICE peek', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    let cur_outReal = 0;
    cur_outReal = (inHigh + inLow) / 2.0;
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
  clone(): MedpriceStream {
    const c = new MedpriceStream(this.core);
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function medpriceStepImpl(sp: MedpriceStream, inHigh: number, inLow: number): void {
  sp.cur_outReal = (inHigh + inLow) / 2.0;
}

function medpriceOpenImpl(
  sp: MedpriceStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let outIdx = 0;
  let i = 0;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* MEDPRICE = (High + Low ) / 2
   * This is the high and low of the same price bar.
   *
   * See MIDPRICE to use instead the highest high and lowest
   * low over multiple price bar.
   */
  outIdx = 0;
  for (i = startIdx; i <= endIdx; i += 1) {
    outReal[((outIdx++ * outStride) | 0)] = (inHigh[i] + inLow[i]) / 2.0;
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function medpriceOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): MedpriceStream {
  const sp = new MedpriceStream(core);
  const retCode = medpriceOpenImpl(sp, inHigh, inLow, startIdx, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MEDPRICE openAndFill', inHigh.length, startIdx, medpriceLookback(core));
  }
  throw streamFailure('MEDPRICE openAndFill', retCode);
}

/** @internal */
export function medpriceOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
): MedpriceStream {
  const sp = new MedpriceStream(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = medpriceOpenImpl(sp, inHigh, inLow, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MEDPRICE open', inHigh.length, startIdx, medpriceLookback(core));
  }
  throw streamFailure('MEDPRICE open', retCode);
}

/**
 * Opens a live MEDPRICE stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link medprice} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `medpriceLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an
 *   empty one, and `TA_BAD_PARAM` for histories of different lengths.
 *
 * @example
 * ```ts
 * import { medpriceOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = medpriceOpen(high, low);
 * const peeked = stream.peek(101.4, 99.1); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1); // the bar closes
 * ```
 */
export function medpriceOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): MedpriceStream {
  requireArgument('MEDPRICE open', 'inHigh', inHigh);
  requireHistory('MEDPRICE open', inHigh.length);
  requireArgument('MEDPRICE open', 'inLow', inLow);
  requireHistoryLength('MEDPRICE open', 'inLow', inLow.length, inHigh.length);
  return medpriceOpenInternal(core, float64(inHigh), float64(inLow), 0);
}

/**
 * {@link medpriceOpen} that also fills the outputs over the whole history in the
 * same pass, bit-identically to {@link medprice}. The range written is the
 * stream's `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param outReal - Midpoint of each bar's high and low. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `medpriceLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an
 *   empty one, and `TA_BAD_PARAM` for histories of different lengths or an output
 *   too short or starting where another output or a `Float64Array` input starts, or,
 *   on a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { medpriceOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(high.length);
 * const stream = medpriceOpenAndFill(high, low, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function medpriceOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): MedpriceStream {
  requireArgument('MEDPRICE openAndFill', 'inHigh', inHigh);
  requireHistory('MEDPRICE openAndFill', inHigh.length);
  requireArgument('MEDPRICE openAndFill', 'inLow', inLow);
  const guardOutLen = openFillCount('MEDPRICE openAndFill', inHigh.length, medpriceLookback(core));
  requireHistoryLength('MEDPRICE openAndFill', 'inLow', inLow.length, inHigh.length);
  requireLength('MEDPRICE openAndFill', 'outReal', outReal, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if ((outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) || (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset)) {
    throw streamFailure('MEDPRICE openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inHighF64) || overlaps(outReal, inLowF64))) {
    throw streamFailure('MEDPRICE openAndFill', RetCode.BAD_PARAM);
  }
  return medpriceOpenAndFillInternal(core, inHighF64, inLowF64, 0, new MInteger(), new MInteger(), outReal);
}
