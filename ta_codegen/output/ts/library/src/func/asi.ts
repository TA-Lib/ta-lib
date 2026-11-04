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
 *  092726 MF,CC  Initial version (#451).
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireReal, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link asi} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInLimitMove - Limit move, the largest one-bar price move the index is
 *   scaled against, in price units (default 3; minimum 0.00000001;
 *   `Core.REAL_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function asiLookback(optInLimitMove: number, core: Core = Core.DEFAULT): number {
  void core;
  if (typeof optInLimitMove !== 'number') {
    return -1;
  }
  if (optInLimitMove === Core.REAL_DEFAULT) {
    optInLimitMove = 3.0;
  } else if (!(optInLimitMove >= 1e-8 && optInLimitMove <= Core.REAL_MAX)) {
    return -1;
  }
  return 0;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link asi}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInLimitMove - Limit move, the largest one-bar price move the index is
 *   scaled against, in price units (default 3; minimum 0.00000001;
 *   `Core.REAL_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function asiDisplayShift(optInLimitMove: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (asiLookback(optInLimitMove, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function asiImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  optInLimitMove: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let i = 0;
  let outIdx = 0;
  let prevClose = 0;
  let prevBody = 0;
  let tempOpen = 0;
  let tempHigh = 0;
  let tempLow = 0;
  let tempClose = 0;
  let body = 0;
  let n = 0;
  let up = 0;
  let dn = 0;
  let rg = 0;
  let k = 0;
  let r = 0;
  let swing = 0;
  let sum = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInLimitMove === Core.REAL_DEFAULT) {
    optInLimitMove = 3.0;
  } else if (!(optInLimitMove >= 1e-8 && optInLimitMove <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  /* SI's per-bar statements, kept identical: the regression test holds this
   * function bit for bit to CUMSUM over SI. Bar startIdx is read before the
   * anchor store, which may overwrite it when outReal aliases an input.
   */
  prevClose = inClose[startIdx];
  prevBody = prevClose - inOpen[startIdx];
  sum = 0.0;
  outReal[0] = sum;
  outIdx = 1;
  for (i = startIdx + 1; i <= endIdx; i += 1) {
    tempOpen = inOpen[i];
    tempHigh = inHigh[i];
    tempLow = inLow[i];
    tempClose = inClose[i];
    body = tempClose - tempOpen;
    n = tempClose - prevClose;
    n += 0.5 * body;
    n += 0.25 * prevBody;
    up = Math.abs(tempHigh - prevClose);
    dn = Math.abs(tempLow - prevClose);
    rg = Math.abs(tempHigh - tempLow);
    k = (up > dn ? up : dn);
    r = ((up - 0.5 * dn) > (dn - 0.5 * up) ? (up - 0.5 * dn) : (dn - 0.5 * up));
    r = (r > rg ? r : rg);
    r += 0.25 * Math.abs(prevBody);
    if (r === 0.0) {
      swing = 0.0;
    } else {
      swing = 50.0 * (n / r) * (k / optInLimitMove);
    }
    sum += swing;
    outReal[outIdx] = sum;
    outIdx += 1;
    prevClose = tempClose;
    prevBody = body;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * Wilder's Accumulative Swing Index: the running total of the Swing Index (SI), a
 * line meant to trace the market's real swings through the noise of the daily
 * closes. Wilder draws trendlines and breakout points on it as on a price chart: a
 * breakout of the ASI that the price has not confirmed yet is his signal. J.
 * Welles Wilder Jr. introduced both in 1978. The line starts at 0 on the first bar
 * of the requested range, so only its shape carries meaning, not its level.
 *
 * Formula and more info at
 * [ta-lib.org/functions/asi](https://ta-lib.org/functions/asi).
 *
 * **Notes**
 * - Wilder accumulates Swing Index values rounded to whole numbers; the running
 *   total here is of the unrounded values.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link asiLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inOpen - Open price of each bar.
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param optInLimitMove - Limit move, the largest one-bar price move the index is
 *   scaled against, in price units (default 3; minimum 0.00000001;
 *   `Core.REAL_DEFAULT` selects the default).
 * @param outReal - Running total of the swing index from the first bar of the
 *   range. Must hold the values produced and never be empty; the input length always
 *   suffices.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The range written: `begIdx` is the first bar with a value, `count` how
 *   many were written.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for an argument that is not an int32, a
 *   real parameter that is not a number, a parameter outside its range, an array too
 *   short for the range, an empty output or an output sharing memory with a
 *   `Float64Array` input it is not. Checked before anything is written.
 *
 * @example
 * ```ts
 * import { asi } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const out = new Float64Array(open.length);
 * const { begIdx, count } = asi(
 *   0, open.length - 1,
 *   open, high, low, close,
 *   3,
 *   out,
 * );
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`si`](https://ta-lib.org/functions/si)
 * @see [`wad`](https://ta-lib.org/functions/wad)
 * @see [`cumsum`](https://ta-lib.org/functions/cumsum)
 */
export function asi(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInLimitMove: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('ASI', 'startIdx', startIdx);
  endIdx = requireInt32('ASI', 'endIdx', endIdx);
  optInLimitMove = requireReal('ASI', 'optInLimitMove', optInLimitMove);
  requireIndexRange('ASI', startIdx, endIdx);
  const guardStart = clampedStart('ASI', startIdx, asiLookback(optInLimitMove, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('ASI', 'inOpen', inOpen, guardInLen);
  requireLength('ASI', 'inHigh', inHigh, guardInLen);
  requireLength('ASI', 'inLow', inLow, guardInLen);
  requireLength('ASI', 'inClose', inClose, guardInLen);
  requireLength('ASI', 'outReal', outReal, guardOutLen);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outReal, inOpenF64) || overlapsPartly(outReal, inHighF64) || overlapsPartly(outReal, inLowF64) || overlapsPartly(outReal, inCloseF64)) {
    throw failure('ASI', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = asiImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, optInLimitMove, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('ASI', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live ASI stream: one value per closed bar, bit-identical to {@link asi} over
 * the same series. Open it with {@link asiOpen} or {@link asiOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/asi](https://ta-lib.org/functions/asi).
 */
export class AsiStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInLimitMove: number = 0;
  /** @internal */
  prevClose: number = 0;
  /** @internal */
  prevBody: number = 0;
  /** @internal */
  sum: number = 0;
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
  static of(core: Core): AsiStream {
    return new AsiStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link asi} reports them over the same bars. Each accepted `update` and each
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
      throw failure('ASI advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inOpen - The bar's `inOpen` value: Open price of each bar.
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inOpen: number, inHigh: number, inLow: number, inClose: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('ASI update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inOpen) || !Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('ASI update', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    asiStepImpl(this, inOpen, inHigh, inLow, inClose);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inOpen - The bar's `inOpen` value: Open price of each bar.
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inOpen: number, inHigh: number, inLow: number, inClose: number): number {
    if (!Number.isFinite(inOpen) || !Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('ASI peek', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let tempOpen = 0;
    let tempHigh = 0;
    let tempLow = 0;
    let tempClose = 0;
    let body = 0;
    let n = 0;
    let up = 0;
    let dn = 0;
    let rg = 0;
    let k = 0;
    let r = 0;
    let swing = 0;
    let cur_outReal = 0;
    let sum = sp.sum;
    tempOpen = inOpen;
    tempHigh = inHigh;
    tempLow = inLow;
    tempClose = inClose;
    body = tempClose - tempOpen;
    n = tempClose - sp.prevClose;
    n += 0.5 * body;
    n += 0.25 * sp.prevBody;
    up = Math.abs(tempHigh - sp.prevClose);
    dn = Math.abs(tempLow - sp.prevClose);
    rg = Math.abs(tempHigh - tempLow);
    k = (up > dn ? up : dn);
    r = ((up - 0.5 * dn) > (dn - 0.5 * up) ? (up - 0.5 * dn) : (dn - 0.5 * up));
    r = (r > rg ? r : rg);
    r += 0.25 * Math.abs(sp.prevBody);
    if (r === 0.0) {
      swing = 0.0;
    } else {
      swing = 50.0 * (n / r) * (k / sp.optInLimitMove);
    }
    sum += swing;
    cur_outReal = sum;
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
  clone(): AsiStream {
    const c = new AsiStream(this.core);
    c.optInLimitMove = this.optInLimitMove;
    c.prevClose = this.prevClose;
    c.prevBody = this.prevBody;
    c.sum = this.sum;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function asiStepImpl(sp: AsiStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  let tempOpen = 0;
  let tempHigh = 0;
  let tempLow = 0;
  let tempClose = 0;
  let body = 0;
  let n = 0;
  let up = 0;
  let dn = 0;
  let rg = 0;
  let k = 0;
  let r = 0;
  let swing = 0;
  tempOpen = inOpen;
  tempHigh = inHigh;
  tempLow = inLow;
  tempClose = inClose;
  body = tempClose - tempOpen;
  n = tempClose - sp.prevClose;
  n += 0.5 * body;
  n += 0.25 * sp.prevBody;
  up = Math.abs(tempHigh - sp.prevClose);
  dn = Math.abs(tempLow - sp.prevClose);
  rg = Math.abs(tempHigh - tempLow);
  k = (up > dn ? up : dn);
  r = ((up - 0.5 * dn) > (dn - 0.5 * up) ? (up - 0.5 * dn) : (dn - 0.5 * up));
  r = (r > rg ? r : rg);
  r += 0.25 * Math.abs(sp.prevBody);
  if (r === 0.0) {
    swing = 0.0;
  } else {
    swing = 50.0 * (n / r) * (k / sp.optInLimitMove);
  }
  sp.sum += swing;
  sp.cur_outReal = sp.sum;
  sp.prevClose = tempClose;
  sp.prevBody = body;
}

function asiOpenImpl(
  sp: AsiStream,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInLimitMove: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let i = 0;
  let outIdx = 0;
  let prevClose = 0;
  let prevBody = 0;
  let tempOpen = 0;
  let tempHigh = 0;
  let tempLow = 0;
  let tempClose = 0;
  let body = 0;
  let n = 0;
  let up = 0;
  let dn = 0;
  let rg = 0;
  let k = 0;
  let r = 0;
  let swing = 0;
  let sum = 0;
  const historyLen = inOpen.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inHigh.length !== inOpen.length || inLow.length !== inOpen.length || inClose.length !== inOpen.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInLimitMove === Core.REAL_DEFAULT) {
    optInLimitMove = 3.0;
  } else if (!(optInLimitMove >= 1e-8 && optInLimitMove <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* SI's per-bar statements, kept identical: the regression test holds this
   * function bit for bit to CUMSUM over SI. Bar startIdx is read before the
   * anchor store, which may overwrite it when outReal aliases an input.
   */
  prevClose = inClose[startIdx];
  prevBody = prevClose - inOpen[startIdx];
  sum = 0.0;
  outReal[((0 * outStride) | 0)] = sum;
  outIdx = 1;
  for (i = startIdx + 1; i <= endIdx; i += 1) {
    tempOpen = inOpen[i];
    tempHigh = inHigh[i];
    tempLow = inLow[i];
    tempClose = inClose[i];
    body = tempClose - tempOpen;
    n = tempClose - prevClose;
    n += 0.5 * body;
    n += 0.25 * prevBody;
    up = Math.abs(tempHigh - prevClose);
    dn = Math.abs(tempLow - prevClose);
    rg = Math.abs(tempHigh - tempLow);
    k = (up > dn ? up : dn);
    r = ((up - 0.5 * dn) > (dn - 0.5 * up) ? (up - 0.5 * dn) : (dn - 0.5 * up));
    r = (r > rg ? r : rg);
    r += 0.25 * Math.abs(prevBody);
    if (r === 0.0) {
      swing = 0.0;
    } else {
      swing = 50.0 * (n / r) * (k / optInLimitMove);
    }
    sum += swing;
    outReal[((outIdx * outStride) | 0)] = sum;
    outIdx += 1;
    prevClose = tempClose;
    prevBody = body;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  sp.optInLimitMove = optInLimitMove;
  sp.prevClose = prevClose;
  sp.prevBody = prevBody;
  sp.sum = sum;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function asiOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInLimitMove: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): AsiStream {
  const sp = AsiStream.of(core);
  const retCode = asiOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, optInLimitMove, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ASI openAndFill', inOpen.length, startIdx, asiLookback(optInLimitMove, core));
  }
  throw streamFailure('ASI openAndFill', retCode);
}

/** @internal */
export function asiOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInLimitMove: number,
): AsiStream {
  const sp = AsiStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = asiOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, optInLimitMove, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ASI open', inOpen.length, startIdx, asiLookback(optInLimitMove, core));
  }
  throw streamFailure('ASI open', retCode);
}

/**
 * Opens a live ASI stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link asi} there.
 *
 * @param inOpen - Warm-up history. Open price of each bar. Every history has the
 *   same length.
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInLimitMove - Limit move, the largest one-bar price move the index is
 *   scaled against, in price units (default 3; minimum 0.00000001;
 *   `Core.REAL_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `asiLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for a real parameter that is not a number, a parameter
 *   outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { asiOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = asiOpen(open, high, low, close, 3);
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function asiOpen(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInLimitMove: number,
  core: Core = Core.DEFAULT,
): AsiStream {
  optInLimitMove = requireReal('ASI open', 'optInLimitMove', optInLimitMove);
  requireArgument('ASI open', 'inOpen', inOpen);
  requireHistory('ASI open', inOpen.length);
  requireArgument('ASI open', 'inHigh', inHigh);
  requireArgument('ASI open', 'inLow', inLow);
  requireArgument('ASI open', 'inClose', inClose);
  requireHistoryLength('ASI open', 'inHigh', inHigh.length, inOpen.length);
  requireHistoryLength('ASI open', 'inLow', inLow.length, inOpen.length);
  requireHistoryLength('ASI open', 'inClose', inClose.length, inOpen.length);
  return asiOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0, optInLimitMove);
}

/**
 * {@link asiOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link asi}. The range written is the stream's
 * `outRange()`.
 *
 * @param inOpen - Warm-up history. Open price of each bar. Every history has the
 *   same length.
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInLimitMove - Limit move, the largest one-bar price move the index is
 *   scaled against, in price units (default 3; minimum 0.00000001;
 *   `Core.REAL_DEFAULT` selects the default).
 * @param outReal - Running total of the swing index from the first bar of the
 *   range. Must hold `historyLen - lookback` values and share no memory with an
 *   input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `asiLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for a real parameter that is not a number, a parameter
 *   outside its range, histories of different lengths or an output too short or
 *   starting where another output or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another output or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { asiOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const out = new Float64Array(open.length);
 * const stream = asiOpenAndFill(open, high, low, close, 3, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function asiOpenAndFill(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInLimitMove: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): AsiStream {
  optInLimitMove = requireReal('ASI openAndFill', 'optInLimitMove', optInLimitMove);
  requireArgument('ASI openAndFill', 'inOpen', inOpen);
  requireHistory('ASI openAndFill', inOpen.length);
  requireArgument('ASI openAndFill', 'inHigh', inHigh);
  requireArgument('ASI openAndFill', 'inLow', inLow);
  requireArgument('ASI openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('ASI openAndFill', inOpen.length, asiLookback(optInLimitMove, core));
  requireHistoryLength('ASI openAndFill', 'inHigh', inHigh.length, inOpen.length);
  requireHistoryLength('ASI openAndFill', 'inLow', inLow.length, inOpen.length);
  requireHistoryLength('ASI openAndFill', 'inClose', inClose.length, inOpen.length);
  requireLength('ASI openAndFill', 'outReal', outReal, guardOutLen);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outReal.buffer === inOpenF64.buffer && outReal.byteOffset === inOpenF64.byteOffset) || (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) || (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) || (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset)) {
    throw streamFailure('ASI openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inOpenF64) || overlaps(outReal, inHighF64) || overlaps(outReal, inLowF64) || overlaps(outReal, inCloseF64))) {
    throw streamFailure('ASI openAndFill', RetCode.BAD_PARAM);
  }
  return asiOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, optInLimitMove, new MInteger(), new MInteger(), outReal);
}
