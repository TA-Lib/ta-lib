/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  092926 KL,CC  Creation (#468).
 *  092926 MF,CC  Store the quotient, then overwrite it, so the loop vectorizes.
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link ibs} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function ibsLookback(core: Core = Core.DEFAULT): number {
  void core;
  /* One bar in, one bar out: the value reads only its own bar, so there is
   * nothing to warm up. bop.c:19-22's lookback, and stochf_lookback(1,1,SMA)
   * agrees, since ma_lookback returns 0 for a period of 1 (ma.c:28-29).
   */
  return 0;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link ibs}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function ibsDisplayShift(outputIdx: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function ibsImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let outIdx = 0;
  let i = 0;
  let range = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  /* IBS = (Close - Low)/(High - Low): where the close sits inside its own
   * bar, 0 at the low and 1 at the high.
   */
  outIdx = 0;
  for (i = startIdx; i <= endIdx; i += 1) {
    /* An exact test: the range of one bar carries the quote unit, so any
     * band would flip every bar of a small enough instrument to 0.5. Spelled
     * <= so a NaN input keeps the quotient's NaN.
     *
     * Store, then overwrite: gcc keeps a guarded or selected quotient
     * scalar under -ftrapping-math.
     */
    range = inHigh[i] - inLow[i];
    outReal[outIdx] = (inClose[i] - inLow[i]) / range;
    if (range <= 0.0) {
      outReal[outIdx] = 0.5;
    }
    outIdx += 1;
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/**
 * Internal Bar Strength: where the close sits inside its own bar's range. It is 0
 * when the bar closes at its low, 1 at its high and 0.5 at the midpoint, and it
 * reads one bar only, with no window and no state. It is the closing half of a
 * stochastic oscillator taken over a single bar, which is how the paper that named
 * the effect describes it. Low readings say the session ended on weakness, high
 * readings on strength, and the published use is mean reversion on the next bar
 * rather than trend confirmation.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ibs](https://ta-lib.org/functions/ibs).
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link ibsLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param outReal - Position of the close within the bar's range, 0 to 1 on a bar
 *   with range. Must hold the values produced and never be empty; the input length
 *   always suffices.
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
 * import { ibs } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const out = new Float64Array(high.length);
 * const { begIdx, count } = ibs(0, high.length - 1, high, low, close, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function ibs(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('IBS', 'startIdx', startIdx);
  endIdx = requireInt32('IBS', 'endIdx', endIdx);
  requireIndexRange('IBS', startIdx, endIdx);
  const guardStart = clampedStart('IBS', startIdx, ibsLookback(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('IBS', 'inHigh', inHigh, guardInLen);
  requireLength('IBS', 'inLow', inLow, guardInLen);
  requireLength('IBS', 'inClose', inClose, guardInLen);
  requireLength('IBS', 'outReal', outReal, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outReal, inHighF64) || overlapsPartly(outReal, inLowF64) || overlapsPartly(outReal, inCloseF64)) {
    throw failure('IBS', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = ibsImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('IBS', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live IBS stream: one value per closed bar, bit-identical to {@link ibs} over
 * the same series. Open it with {@link ibsOpen} or {@link ibsOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ibs](https://ta-lib.org/functions/ibs).
 */
export class IbsStream {
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
   * {@link ibs} reports them over the same bars. Each accepted `update` and each
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
      throw failure('IBS advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number, inClose: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('IBS update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('IBS update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    ibsStepImpl(this, inHigh, inLow, inClose);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number, inClose: number): number {
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('IBS peek', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    let range = 0;
    let cur_outReal = 0;
    /* An exact test: the range of one bar carries the quote unit, so any
     * band would flip every bar of a small enough instrument to 0.5. Spelled
     * <= so a NaN input keeps the quotient's NaN.
     *
     * Store, then overwrite: gcc keeps a guarded or selected quotient
     * scalar under -ftrapping-math.
     */
    range = inHigh - inLow;
    cur_outReal = (inClose - inLow) / range;
    if (range <= 0.0) {
      cur_outReal = 0.5;
    }
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
  clone(): IbsStream {
    const c = new IbsStream(this.core);
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function ibsStepImpl(sp: IbsStream, inHigh: number, inLow: number, inClose: number): void {
  let range = 0;
  /* An exact test: the range of one bar carries the quote unit, so any
   * band would flip every bar of a small enough instrument to 0.5. Spelled
   * <= so a NaN input keeps the quotient's NaN.
   *
   * Store, then overwrite: gcc keeps a guarded or selected quotient
   * scalar under -ftrapping-math.
   */
  range = inHigh - inLow;
  sp.cur_outReal = (inClose - inLow) / range;
  if (range <= 0.0) {
    sp.cur_outReal = 0.5;
  }
}

function ibsOpenImpl(
  sp: IbsStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let outIdx = 0;
  let i = 0;
  let range = 0;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length || inClose.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* IBS = (Close - Low)/(High - Low): where the close sits inside its own
   * bar, 0 at the low and 1 at the high.
   */
  outIdx = 0;
  for (i = startIdx; i <= endIdx; i += 1) {
    /* An exact test: the range of one bar carries the quote unit, so any
     * band would flip every bar of a small enough instrument to 0.5. Spelled
     * <= so a NaN input keeps the quotient's NaN.
     *
     * Store, then overwrite: gcc keeps a guarded or selected quotient
     * scalar under -ftrapping-math.
     */
    range = inHigh[i] - inLow[i];
    outReal[((outIdx * outStride) | 0)] = (inClose[i] - inLow[i]) / range;
    if (range <= 0.0) {
      outReal[((outIdx * outStride) | 0)] = 0.5;
    }
    outIdx += 1;
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function ibsOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): IbsStream {
  const sp = new IbsStream(core);
  const retCode = ibsOpenImpl(sp, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('IBS openAndFill', inHigh.length, startIdx, ibsLookback(core));
  }
  throw streamFailure('IBS openAndFill', retCode);
}

/** @internal */
export function ibsOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): IbsStream {
  const sp = new IbsStream(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = ibsOpenImpl(sp, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('IBS open', inHigh.length, startIdx, ibsLookback(core));
  }
  throw streamFailure('IBS open', retCode);
}

/**
 * Opens a live IBS stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link ibs} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `ibsLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for histories of different lengths.
 *
 * @example
 * ```ts
 * import { ibsOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = ibsOpen(high, low, close);
 * const peeked = stream.peek(101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function ibsOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): IbsStream {
  requireArgument('IBS open', 'inHigh', inHigh);
  requireHistory('IBS open', inHigh.length);
  requireArgument('IBS open', 'inLow', inLow);
  requireArgument('IBS open', 'inClose', inClose);
  requireHistoryLength('IBS open', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('IBS open', 'inClose', inClose.length, inHigh.length);
  return ibsOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0);
}

/**
 * {@link ibsOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link ibs}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param outReal - Position of the close within the bar's range, 0 to 1 on a bar
 *   with range. Must hold `historyLen - lookback` values and share no memory with an
 *   input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `ibsLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for histories of different lengths or an output too
 *   short or starting where another output or a `Float64Array` input starts, or, on
 *   a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { ibsOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const out = new Float64Array(high.length);
 * const stream = ibsOpenAndFill(high, low, close, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function ibsOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): IbsStream {
  requireArgument('IBS openAndFill', 'inHigh', inHigh);
  requireHistory('IBS openAndFill', inHigh.length);
  requireArgument('IBS openAndFill', 'inLow', inLow);
  requireArgument('IBS openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('IBS openAndFill', inHigh.length, ibsLookback(core));
  requireHistoryLength('IBS openAndFill', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('IBS openAndFill', 'inClose', inClose.length, inHigh.length);
  requireLength('IBS openAndFill', 'outReal', outReal, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) || (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) || (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset)) {
    throw streamFailure('IBS openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inHighF64) || overlaps(outReal, inLowF64) || overlaps(outReal, inCloseF64))) {
    throw streamFailure('IBS openAndFill', RetCode.BAD_PARAM);
  }
  return ibsOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outReal);
}
