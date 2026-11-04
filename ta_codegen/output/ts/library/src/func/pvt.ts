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

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link pvt} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function pvtLookback(core: Core = Core.DEFAULT): number {
  void core;
  /* This function have no lookback needed. */
  return 0;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link pvt}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function pvtDisplayShift(outputIdx: number, core: Core = Core.DEFAULT): number {
  void core;
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
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link pvtLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inClose - Close price of each bar.
 * @param inVolume - Volume of each bar.
 * @param outReal - Cumulative price volume trend, seeded at zero. Must hold the
 *   values produced and never be empty; the input length always suffices.
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
 * import { pvt } from 'ta-lib';
 *
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const out = new Float64Array(close.length);
 * const { begIdx, count } = pvt(0, close.length - 1, close, volume, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`obv`](https://ta-lib.org/functions/obv)
 * @see [`nvi`](https://ta-lib.org/functions/nvi)
 * @see [`pvi`](https://ta-lib.org/functions/pvi)
 * @see [`pvo`](https://ta-lib.org/functions/pvo)
 * @see [`ad`](https://ta-lib.org/functions/ad)
 */
export function pvt(
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
  const guardStart = clampedStart('PVT', startIdx, pvtLookback(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('PVT', 'inClose', inClose, guardInLen);
  requireLength('PVT', 'inVolume', inVolume, guardInLen);
  requireOutput('PVT', 'outReal', outReal, guardOutLen, Float64Array);
  const inCloseF64 = float64(inClose);
  const inVolumeF64 = float64(inVolume);
  if (overlapsPartly(outReal, inCloseF64) || overlapsPartly(outReal, inVolumeF64)) {
    throw failure('PVT', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = pvtImpl(core, startIdx, endIdx, inCloseF64, inVolumeF64, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('PVT', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
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
      throw failure('PVT advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param inVolume - The bar's `inVolume` value: Volume of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inClose: number, inVolume: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('PVT update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inClose) || !Number.isFinite(inVolume)) {
      throw nonFiniteBar('PVT update', !Number.isFinite(inClose) ? 'inClose' : 'inVolume');
    }
    pvtStepImpl(this, inClose, inVolume);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param inVolume - The bar's `inVolume` value: Volume of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inClose: number, inVolume: number): number {
    if (!Number.isFinite(inClose) || !Number.isFinite(inVolume)) {
      throw nonFiniteBar('PVT peek', !Number.isFinite(inClose) ? 'inClose' : 'inVolume');
    }
    const sp = this;
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
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inVolume.length !== inClose.length) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
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
    throw insufficientHistory('PVT openAndFill', inClose.length, startIdx, pvtLookback(core));
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
    throw insufficientHistory('PVT open', inClose.length, startIdx, pvtLookback(core));
  }
  throw streamFailure('PVT open', retCode);
}

/**
 * Opens a live PVT stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link pvt} there.
 *
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param inVolume - Warm-up history. Volume of each bar. Every history has the
 *   same length.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `pvtLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for histories of different lengths.
 *
 * @example
 * ```ts
 * import { pvtOpen } from 'ta-lib';
 *
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const stream = pvtOpen(close, volume);
 * const peeked = stream.peek(100.9, 12345); // a forming bar: commits nothing
 * const updated = stream.update(100.9, 12345); // the bar closes
 * ```
 */
export function pvtOpen(
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): PvtStream {
  requireArgument('PVT open', 'inClose', inClose);
  requireHistory('PVT open', inClose.length);
  requireArgument('PVT open', 'inVolume', inVolume);
  requireHistoryLength('PVT open', 'inVolume', inVolume.length, inClose.length);
  return pvtOpenInternal(core, float64(inClose), float64(inVolume), 0);
}

/**
 * {@link pvtOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link pvt}. The range written is the stream's
 * `outRange()`.
 *
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param inVolume - Warm-up history. Volume of each bar. Every history has the
 *   same length.
 * @param outReal - Cumulative price volume trend, seeded at zero. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `pvtLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for histories of different lengths or an output too
 *   short or starting where another output or a `Float64Array` input starts, or, on
 *   a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { pvtOpenAndFill } from 'ta-lib';
 *
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const out = new Float64Array(close.length);
 * const stream = pvtOpenAndFill(close, volume, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function pvtOpenAndFill(
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): PvtStream {
  requireArgument('PVT openAndFill', 'inClose', inClose);
  requireHistory('PVT openAndFill', inClose.length);
  requireArgument('PVT openAndFill', 'inVolume', inVolume);
  const guardOutLen = openFillCount('PVT openAndFill', inClose.length, pvtLookback(core));
  requireHistoryLength('PVT openAndFill', 'inVolume', inVolume.length, inClose.length);
  requireOutput('PVT openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inCloseF64 = float64(inClose);
  const inVolumeF64 = float64(inVolume);
  if ((outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset) || (outReal.buffer === inVolumeF64.buffer && outReal.byteOffset === inVolumeF64.byteOffset)) {
    throw streamFailure('PVT openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inCloseF64) || overlaps(outReal, inVolumeF64))) {
    throw streamFailure('PVT openAndFill', RetCode.BAD_PARAM);
  }
  return pvtOpenAndFillInternal(core, inCloseF64, inVolumeF64, 0, new MInteger(), new MInteger(), outReal);
}
