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
 * Number of leading input bars {@link si} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInLimitMove - Limit move, the largest one-bar price move the index is
 *   scaled against, in price units (default 3; minimum 0.00000001;
 *   `Core.REAL_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function siLookback(optInLimitMove: number, core: Core = Core.DEFAULT): number {
  void core;
  if (typeof optInLimitMove !== 'number') {
    return -1;
  }
  if (optInLimitMove === Core.REAL_DEFAULT) {
    optInLimitMove = 3.0;
  } else if (!(optInLimitMove >= 1e-8 && optInLimitMove <= Core.REAL_MAX)) {
    return -1;
  }
  return 1;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link si}.
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
export function siDisplayShift(optInLimitMove: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (siLookback(optInLimitMove, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function siImpl(
  core: Core,
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
  let lookbackTotal = 0;
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
  lookbackTotal = siLookback(optInLimitMove, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  prevClose = inClose[startIdx - 1];
  prevBody = prevClose - inOpen[startIdx - 1];
  outIdx = 0;
  for (i = startIdx; i <= endIdx; i += 1) {
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
    /* Wilder's three cases of R are this one max, since rg is up + dn or
     * |up - dn|. Keep it branch-free: the case chain mispredicts on most
     * bars of real data. The four backends' max builtins agree only while
     * no operand is -0.0 or NaN, which holds while up and dn are finite.
     */
    r = ((up - 0.5 * dn) > (dn - 0.5 * up) ? (up - 0.5 * dn) : (dn - 0.5 * up));
    r = (r > rg ? r : rg);
    r += 0.25 * Math.abs(prevBody);
    if (r === 0.0) {
      swing = 0.0;
    } else {
      swing = 50.0 * (n / r) * (k / optInLimitMove);
    }
    outReal[outIdx] = swing;
    outIdx += 1;
    prevClose = tempClose;
    prevBody = body;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * Wilder's Swing Index: one number rating each bar against the bar before it, from
 * both bars' open and close and the current bar's high and low, signed by the
 * direction of the swing. A strong close above the prior close on a wide range
 * reads high and positive, a strong down swing reads high and negative. It is
 * scaled against the limit move, the largest one-bar move the market allows, and
 * stays between -100 and +100 while every move is within that limit. J. Welles
 * Wilder Jr. introduced it in 1978 and trades its running total, ASI.
 *
 * Formula and more info at
 * [ta-lib.org/functions/si](https://ta-lib.org/functions/si).
 *
 * **Notes**
 * - Wilder rounds each value to a whole number when working by hand; the output is
 *   not rounded.
 * - A bar that moves more than the limit move can read beyond -100 or +100; the
 *   output is not clamped.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link siLookback} is a success with `count` 0.
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
 * @param outReal - Swing index of the bar against the previous bar. Must hold the
 *   values produced and never be empty; the input length always suffices.
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
 * import { si } from 'ta-lib';
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
 * const { begIdx, count } = si(
 *   0, open.length - 1,
 *   open, high, low, close,
 *   3,
 *   out,
 * );
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`asi`](https://ta-lib.org/functions/asi)
 * @see [`wad`](https://ta-lib.org/functions/wad)
 * @see [`trange`](https://ta-lib.org/functions/trange)
 * @see [`bop`](https://ta-lib.org/functions/bop)
 */
export function si(
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
  startIdx = requireInt32('SI', 'startIdx', startIdx);
  endIdx = requireInt32('SI', 'endIdx', endIdx);
  optInLimitMove = requireReal('SI', 'optInLimitMove', optInLimitMove);
  requireIndexRange('SI', startIdx, endIdx);
  const guardStart = clampedStart('SI', startIdx, siLookback(optInLimitMove, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('SI', 'inOpen', inOpen, guardInLen);
  requireLength('SI', 'inHigh', inHigh, guardInLen);
  requireLength('SI', 'inLow', inLow, guardInLen);
  requireLength('SI', 'inClose', inClose, guardInLen);
  requireLength('SI', 'outReal', outReal, guardOutLen);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outReal, inOpenF64) || overlapsPartly(outReal, inHighF64) || overlapsPartly(outReal, inLowF64) || overlapsPartly(outReal, inCloseF64)) {
    throw failure('SI', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = siImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, optInLimitMove, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('SI', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live SI stream: one value per closed bar, bit-identical to {@link si} over the
 * same series. Open it with {@link siOpen} or {@link siOpenAndFill}; there is
 * nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/si](https://ta-lib.org/functions/si).
 */
export class SiStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInLimitMove: number = 0;
  /** @internal */
  prevClose: number = 0;
  /** @internal */
  prevBody: number = 0;
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
  static of(core: Core): SiStream {
    return new SiStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link si} reports them over the same bars. Each accepted `update` and each
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
      throw failure('SI advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw failure('SI update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inOpen) || !Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('SI update', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    siStepImpl(this, inOpen, inHigh, inLow, inClose);
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
      throw nonFiniteBar('SI peek', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
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
    /* Wilder's three cases of R are this one max, since rg is up + dn or
     * |up - dn|. Keep it branch-free: the case chain mispredicts on most
     * bars of real data. The four backends' max builtins agree only while
     * no operand is -0.0 or NaN, which holds while up and dn are finite.
     */
    r = ((up - 0.5 * dn) > (dn - 0.5 * up) ? (up - 0.5 * dn) : (dn - 0.5 * up));
    r = (r > rg ? r : rg);
    r += 0.25 * Math.abs(sp.prevBody);
    if (r === 0.0) {
      swing = 0.0;
    } else {
      swing = 50.0 * (n / r) * (k / sp.optInLimitMove);
    }
    cur_outReal = swing;
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
  clone(): SiStream {
    const c = new SiStream(this.core);
    c.optInLimitMove = this.optInLimitMove;
    c.prevClose = this.prevClose;
    c.prevBody = this.prevBody;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function siStepImpl(sp: SiStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
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
  /* Wilder's three cases of R are this one max, since rg is up + dn or
   * |up - dn|. Keep it branch-free: the case chain mispredicts on most
   * bars of real data. The four backends' max builtins agree only while
   * no operand is -0.0 or NaN, which holds while up and dn are finite.
   */
  r = ((up - 0.5 * dn) > (dn - 0.5 * up) ? (up - 0.5 * dn) : (dn - 0.5 * up));
  r = (r > rg ? r : rg);
  r += 0.25 * Math.abs(sp.prevBody);
  if (r === 0.0) {
    swing = 0.0;
  } else {
    swing = 50.0 * (n / r) * (k / sp.optInLimitMove);
  }
  sp.cur_outReal = swing;
  sp.prevClose = tempClose;
  sp.prevBody = body;
}

function siOpenImpl(
  sp: SiStream,
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
  const core = sp.core;
  let i = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
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
  lookbackTotal = siLookback(optInLimitMove, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  prevClose = inClose[startIdx - 1];
  prevBody = prevClose - inOpen[startIdx - 1];
  outIdx = 0;
  for (i = startIdx; i <= endIdx; i += 1) {
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
    /* Wilder's three cases of R are this one max, since rg is up + dn or
     * |up - dn|. Keep it branch-free: the case chain mispredicts on most
     * bars of real data. The four backends' max builtins agree only while
     * no operand is -0.0 or NaN, which holds while up and dn are finite.
     */
    r = ((up - 0.5 * dn) > (dn - 0.5 * up) ? (up - 0.5 * dn) : (dn - 0.5 * up));
    r = (r > rg ? r : rg);
    r += 0.25 * Math.abs(prevBody);
    if (r === 0.0) {
      swing = 0.0;
    } else {
      swing = 50.0 * (n / r) * (k / optInLimitMove);
    }
    outReal[((outIdx * outStride) | 0)] = swing;
    outIdx += 1;
    prevClose = tempClose;
    prevBody = body;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  sp.optInLimitMove = optInLimitMove;
  sp.prevClose = prevClose;
  sp.prevBody = prevBody;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function siOpenAndFillInternal(
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
): SiStream {
  const sp = SiStream.of(core);
  const retCode = siOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, optInLimitMove, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SI openAndFill', inOpen.length, startIdx, siLookback(optInLimitMove, core));
  }
  throw streamFailure('SI openAndFill', retCode);
}

/** @internal */
export function siOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInLimitMove: number,
): SiStream {
  const sp = SiStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = siOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, optInLimitMove, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SI open', inOpen.length, startIdx, siLookback(optInLimitMove, core));
  }
  throw streamFailure('SI open', retCode);
}

/**
 * Opens a live SI stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link si} there.
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
 *   than `siLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty one,
 *   and `TA_BAD_PARAM` for a real parameter that is not a number, a parameter
 *   outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { siOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = siOpen(open, high, low, close, 3);
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function siOpen(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInLimitMove: number,
  core: Core = Core.DEFAULT,
): SiStream {
  optInLimitMove = requireReal('SI open', 'optInLimitMove', optInLimitMove);
  requireArgument('SI open', 'inOpen', inOpen);
  requireHistory('SI open', inOpen.length);
  requireArgument('SI open', 'inHigh', inHigh);
  requireArgument('SI open', 'inLow', inLow);
  requireArgument('SI open', 'inClose', inClose);
  requireHistoryLength('SI open', 'inHigh', inHigh.length, inOpen.length);
  requireHistoryLength('SI open', 'inLow', inLow.length, inOpen.length);
  requireHistoryLength('SI open', 'inClose', inClose.length, inOpen.length);
  return siOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0, optInLimitMove);
}

/**
 * {@link siOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link si}. The range written is the stream's
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
 * @param outReal - Swing index of the bar against the previous bar. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `siLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty one,
 *   and `TA_BAD_PARAM` for a real parameter that is not a number, a parameter
 *   outside its range, histories of different lengths or an output too short or
 *   starting where another output or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another output or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { siOpenAndFill } from 'ta-lib';
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
 * const stream = siOpenAndFill(open, high, low, close, 3, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function siOpenAndFill(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInLimitMove: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): SiStream {
  optInLimitMove = requireReal('SI openAndFill', 'optInLimitMove', optInLimitMove);
  requireArgument('SI openAndFill', 'inOpen', inOpen);
  requireHistory('SI openAndFill', inOpen.length);
  requireArgument('SI openAndFill', 'inHigh', inHigh);
  requireArgument('SI openAndFill', 'inLow', inLow);
  requireArgument('SI openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('SI openAndFill', inOpen.length, siLookback(optInLimitMove, core));
  requireHistoryLength('SI openAndFill', 'inHigh', inHigh.length, inOpen.length);
  requireHistoryLength('SI openAndFill', 'inLow', inLow.length, inOpen.length);
  requireHistoryLength('SI openAndFill', 'inClose', inClose.length, inOpen.length);
  requireLength('SI openAndFill', 'outReal', outReal, guardOutLen);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outReal.buffer === inOpenF64.buffer && outReal.byteOffset === inOpenF64.byteOffset) || (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) || (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) || (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset)) {
    throw streamFailure('SI openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inOpenF64) || overlaps(outReal, inHighF64) || overlaps(outReal, inLowF64) || overlaps(outReal, inCloseF64))) {
    throw streamFailure('SI openAndFill', RetCode.BAD_PARAM);
  }
  return siOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, optInLimitMove, new MInteger(), new MInteger(), outReal);
}
