/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AM       Adrian Michel
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  010802 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  082303 MF     Fix #792298. Remove rounding. Bug reported by AM.
 *  071126 MF,CC  Rewrite the ADX combine as a single cursor: outReal[k] =
 *                (adx[k+(period-1)] + adx[k])/2 (current ADX + ADX lagged by
 *                period-1). Bit-identical to the two-cursor form, and the
 *                streamable-source form (a sub-output lag ring).
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { AdxStream, adx as adx_, adxLookback, adxOpenAndFillInternal } from './adx.ts';
import { adxrLookback as adxrLookback_ } from './adxr.ts';

/**
 * Number of leading input bars {@link adxr} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Smoothing period, also the bar gap between the two
 *   averaged ADX values (default 14; range 2 to 100000; `Core.INTEGER_DEFAULT`
 *   selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function adxrLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  if (optInTimePeriod > 1) {
    return optInTimePeriod + adxLookback(optInTimePeriod, core) - 1;
  } else {
    return 3;
  }
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link adxr}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Smoothing period, also the bar gap between the two
 *   averaged ADX values (default 14; range 2 to 100000; `Core.INTEGER_DEFAULT`
 *   selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function adxrDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (adxrLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function adxrImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let adx!: Float64Array;
  let adxrLookback = 0;
  let outIdx = 0;
  let nbElement = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  /* Original implementation from Wilder's book was doing some integer
   * rounding in its calculations.
   *
   * This was understandable in the context that at the time the book
   * was written, most user were doing the calculation by hand.
   *
   * For a computer, rounding is unnecessary (and even problematic when inputs
   * are close to 1).
   *
   * TA-Lib does not do the rounding. Still, if you want to reproduce Wilder's examples,
   * you can comment out the following #undef/#define and rebuild the library.
   */
  /* Move up the start index if there is not
   * enough initial data.
   * Always one price bar gets consumed.
   */
  adxrLookback = adxrLookback_(optInTimePeriod, core);
  if (startIdx < adxrLookback) {
    startIdx = adxrLookback;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  adx = new Float64Array(endIdx - startIdx + optInTimePeriod);
  /* Compute ADX over a range that starts (period-1) bars earlier, so each
   * ADXR bar can pair the current ADX with the ADX from (period-1) bars ago.
   */
  const _xr0 = adx_(startIdx - (optInTimePeriod - 1), endIdx, inHigh, inLow, inClose, optInTimePeriod, adx, core);
  outBegIdx.value = _xr0.begIdx;
  outNBElement.value = _xr0.count;
  /* ADXR[k] = (ADX[k] + ADX[k-(period-1)]) / 2. Walking a single cursor over
   * the ADXR output, the current ADX is adx[k+(period-1)] and the lagged one
   * is adx[k]; the ADX range holds (period-1) more elements than the output.
   */
  nbElement = outNBElement.value - (optInTimePeriod - 1);
  for (outIdx = 0; outIdx < nbElement; outIdx += 1) {
    outReal[outIdx] = ((adx[outIdx + (optInTimePeriod - 1)] + adx[outIdx]) / 2.0);
  }
  outBegIdx.value = startIdx;
  outNBElement.value = nbElement;
  return RetCode.SUCCESS;
}

/**
 * Smoothed variant of ADX: the average of the current ADX value and the ADX value
 * from (period-1) bars earlier. Further damps ADX to gauge trend strength. Higher
 * values mean a stronger trend; smoother and more lagging than ADX.
 *
 * Formula and more info at
 * [ta-lib.org/functions/adxr](https://ta-lib.org/functions/adxr).
 *
 * **Notes**
 * - Wilder's original integer rounding is not applied (unreliable when values are
 *   near 1).
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link adxrLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param optInTimePeriod - Smoothing period, also the bar gap between the two
 *   averaged ADX values (default 14; range 2 to 100000; `Core.INTEGER_DEFAULT`
 *   selects the default).
 * @param outReal - ADXR line (averaged ADX) Must hold the values produced and
 *   never be empty; the input length always suffices.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The range written: `begIdx` is the first bar with a value, `count` how
 *   many were written.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for an argument that is not an int32, a
 *   parameter outside its range, an array too short for the range, an empty output
 *   or an output sharing memory with a `Float64Array` input it is not. Checked
 *   before anything is written.
 *
 * @example
 * ```ts
 * import { adxr } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const out = new Float64Array(high.length);
 * const { begIdx, count } = adxr(0, high.length - 1, high, low, close, 14, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`adx`](https://ta-lib.org/functions/adx)
 * @see [`dx`](https://ta-lib.org/functions/dx)
 * @see [`plusDi`](https://ta-lib.org/functions/plus_di)
 * @see [`minusDi`](https://ta-lib.org/functions/minus_di)
 */
export function adxr(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('ADXR', 'startIdx', startIdx);
  endIdx = requireInt32('ADXR', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('ADXR', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('ADXR', startIdx, endIdx);
  const guardStart = clampedStart('ADXR', startIdx, adxrLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('ADXR', 'inHigh', inHigh, guardInLen);
  requireLength('ADXR', 'inLow', inLow, guardInLen);
  requireLength('ADXR', 'inClose', inClose, guardInLen);
  requireLength('ADXR', 'outReal', outReal, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outReal, inHighF64) || overlapsPartly(outReal, inLowF64) || overlapsPartly(outReal, inCloseF64)) {
    throw failure('ADXR', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = adxrImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('ADXR', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live ADXR stream: one value per closed bar, bit-identical to {@link adxr} over
 * the same series. Open it with {@link adxrOpen} or {@link adxrOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/adxr](https://ta-lib.org/functions/adxr).
 */
export class AdxrStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  cur_outReal: number = 0;
  /** @internal */
  lagRingPos_adx: number = 0;
  /** @internal */
  lagRingCap_adx: number = 1;
  /** @internal */
  lagRing_adx: Float64Array = new Float64Array(1);
  /** @internal */
  sub0!: AdxStream;
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
   * {@link adxr} reports them over the same bars. Each accepted `update` and each
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
      throw failure('ADXR advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw failure('ADXR update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('ADXR update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    adxrStepImpl(this, inHigh, inLow, inClose);
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
      throw nonFiniteBar('ADXR peek', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let cur_adx = 0;
    let cur_outReal = 0;
    cur_adx = sp.sub0.peek(inHigh, inLow, inClose);
    cur_outReal = ((cur_adx + sp.lagRing_adx[sp.lagRingPos_adx]) / 2.0);
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
  clone(): AdxrStream {
    const c = new AdxrStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.cur_outReal = this.cur_outReal;
    c.lagRingPos_adx = this.lagRingPos_adx;
    c.lagRingCap_adx = this.lagRingCap_adx;
    c.lagRing_adx = this.lagRing_adx.slice();
    c.sub0 = this.sub0.clone();
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function adxrStepImpl(sp: AdxrStream, inHigh: number, inLow: number, inClose: number): void {
  let cur_adx = 0;
  let cur_outReal = 0;
  cur_adx = sp.sub0.update(inHigh, inLow, inClose);
  cur_outReal = ((cur_adx + sp.lagRing_adx[sp.lagRingPos_adx]) / 2.0);
  sp.lagRing_adx[sp.lagRingPos_adx] = cur_adx;
  sp.lagRingPos_adx = (sp.lagRingPos_adx + 1) % sp.lagRingCap_adx;
  sp.cur_outReal = cur_outReal;
}

function adxrOpenImpl(
  sp: AdxrStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let adx!: Float64Array;
  let adxrLookback = 0;
  let outIdx = 0;
  let nbElement = 0;
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
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen < adxrLookback_(optInTimePeriod, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const sc_outReal = outStride === 1 ? outReal : new Float64Array(historyLen);
  /* Original implementation from Wilder's book was doing some integer
   * rounding in its calculations.
   *
   * This was understandable in the context that at the time the book
   * was written, most user were doing the calculation by hand.
   *
   * For a computer, rounding is unnecessary (and even problematic when inputs
   * are close to 1).
   *
   * TA-Lib does not do the rounding. Still, if you want to reproduce Wilder's examples,
   * you can comment out the following #undef/#define and rebuild the library.
   */
  /* Move up the start index if there is not
   * enough initial data.
   * Always one price bar gets consumed.
   */
  adxrLookback = adxrLookback_(optInTimePeriod, core);
  if (startIdx < adxrLookback) {
    startIdx = adxrLookback;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  adx = new Float64Array(endIdx - startIdx + optInTimePeriod);
  /* Compute ADX over a range that starts (period-1) bars earlier, so each
   * ADXR bar can pair the current ADX with the ADX from (period-1) bars ago.
   */
  const sub0 = adxOpenAndFillInternal(core, inHigh, inLow, inClose, startIdx - (optInTimePeriod - 1), optInTimePeriod, outBegIdx, outNBElement, adx);
  /* ADXR[k] = (ADX[k] + ADX[k-(period-1)]) / 2. Walking a single cursor over
   * the ADXR output, the current ADX is adx[k+(period-1)] and the lagged one
   * is adx[k]; the ADX range holds (period-1) more elements than the output.
   */
  nbElement = outNBElement.value - (optInTimePeriod - 1);
  for (outIdx = 0; outIdx < nbElement; outIdx += 1) {
    sc_outReal[outIdx] = ((adx[outIdx + (optInTimePeriod - 1)] + adx[outIdx]) / 2.0);
  }
  outBegIdx.value = startIdx;
  outNBElement.value = nbElement;
  if (outNBElement.value < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const lagCap_adx = optInTimePeriod - 1;
  const lagRing_adx = new Float64Array(lagCap_adx);
  for (let lagI = 0; lagI < lagCap_adx; lagI++) {
    lagRing_adx[lagI] = adx[outNBElement.value + lagI];
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.sub0 = sub0;
  sp.lagRingPos_adx = 0;
  sp.lagRingCap_adx = lagCap_adx;
  sp.lagRing_adx = lagRing_adx;
  sp.cur_outReal = sc_outReal[outNBElement.value - 1];
  return RetCode.SUCCESS;
}

/** @internal */
export function adxrOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): AdxrStream {
  const sp = new AdxrStream(core);
  const retCode = adxrOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ADXR openAndFill', inHigh.length, startIdx, adxrLookback(optInTimePeriod, core));
  }
  throw streamFailure('ADXR openAndFill', retCode);
}

/** @internal */
export function adxrOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): AdxrStream {
  const sp = new AdxrStream(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = adxrOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ADXR open', inHigh.length, startIdx, adxrLookback(optInTimePeriod, core));
  }
  throw streamFailure('ADXR open', retCode);
}

/**
 * Opens a live ADXR stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link adxr} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Smoothing period, also the bar gap between the two
 *   averaged ADX values (default 14; range 2 to 100000; `Core.INTEGER_DEFAULT`
 *   selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `adxrLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { adxrOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = adxrOpen(high, low, close, 14);
 * const peeked = stream.peek(101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function adxrOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): AdxrStream {
  optInTimePeriod = requireInt32('ADXR open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('ADXR open', 'inHigh', inHigh);
  requireHistory('ADXR open', inHigh.length);
  requireArgument('ADXR open', 'inLow', inLow);
  requireArgument('ADXR open', 'inClose', inClose);
  requireHistoryLength('ADXR open', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('ADXR open', 'inClose', inClose.length, inHigh.length);
  return adxrOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInTimePeriod);
}

/**
 * {@link adxrOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link adxr}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Smoothing period, also the bar gap between the two
 *   averaged ADX values (default 14; range 2 to 100000; `Core.INTEGER_DEFAULT`
 *   selects the default).
 * @param outReal - ADXR line (averaged ADX) Must hold `historyLen - lookback`
 *   values and share no memory with an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `adxrLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range, histories of different lengths or an output too
 *   short or starting where another output or a `Float64Array` input starts, or, on
 *   a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { adxrOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const out = new Float64Array(high.length);
 * const stream = adxrOpenAndFill(high, low, close, 14, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function adxrOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): AdxrStream {
  optInTimePeriod = requireInt32('ADXR openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('ADXR openAndFill', 'inHigh', inHigh);
  requireHistory('ADXR openAndFill', inHigh.length);
  requireArgument('ADXR openAndFill', 'inLow', inLow);
  requireArgument('ADXR openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('ADXR openAndFill', inHigh.length, adxrLookback(optInTimePeriod, core));
  requireHistoryLength('ADXR openAndFill', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('ADXR openAndFill', 'inClose', inClose.length, inHigh.length);
  requireLength('ADXR openAndFill', 'outReal', outReal, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) || (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) || (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset)) {
    throw streamFailure('ADXR openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inHighF64) || overlaps(outReal, inLowF64) || overlaps(outReal, inCloseF64))) {
    throw streamFailure('ADXR openAndFill', RetCode.BAD_PARAM);
  }
  return adxrOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}
