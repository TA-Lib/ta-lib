/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AC       Angelo Ciceri
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  011605 AC     Creation
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link cdlxsidegap3methods} consumes before its
 * first value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function cdlxsidegap3methodsLookback(core: Core = Core.DEFAULT): number {
  void core;
  return 2;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdlxsidegap3methods}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function cdlxsidegap3methodsDisplayShift(outputIdx: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdlxsidegap3methodsImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inOpen: Float64Array,
  _inHigh: Float64Array,
  _inLow: Float64Array,
  inClose: Float64Array,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): RetCode {
  let i = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlxsidegap3methodsLookback(core);
  /* Move up the start index if there is not
   * enough initial data.
   */
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  /* Do the calculation using tight loops. */
  /* Add-up the initial period, except for the last value. */
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first candle: white (black) candle
   * - second candle: white (black) candle
   * - upside (downside) gap between the first and the second real bodies
   * - third candle: black (white) candle that opens within the second real body and closes within the first real body
   */
  outIdx = 0;
  do {
    if (((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) && /* 1st and 2nd of same color */
        ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === (-((inClose[i] >= inOpen[i]) ? 1 : -1) | 0) && /* 3rd opposite color */
        inOpen[i] < (inClose[i - 1] > inOpen[i - 1] ? inClose[i - 1] : inOpen[i - 1]) && /* 3rd opens within 2nd rb */
        inOpen[i] > (inClose[i - 1] < inOpen[i - 1] ? inClose[i - 1] : inOpen[i - 1]) &&
        inClose[i] < (inClose[i - 2] > inOpen[i - 2] ? inClose[i - 2] : inOpen[i - 2]) && /* 3rd closes within 1st rb */
        inClose[i] > (inClose[i - 2] < inOpen[i - 2] ? inClose[i - 2] : inOpen[i - 2]) &&
        (((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === 1 &&  /* when 1st is white */
          ((inOpen[i - 1] < inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1]) > (inOpen[i - 2] > inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2])) || /* upside gap */
         ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === -1 && /* when 1st is black */
          ((inOpen[i - 1] > inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1]) < (inOpen[i - 2] < inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2])))) /* downside gap */
    {
      outInteger[outIdx++] = ((((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) * 100) | 0);
    } else {
      outInteger[outIdx++] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    i += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/**
 * A three-candle continuation pattern: two same-color candles separated by a
 * real-body gap, followed by an opposite-color candle that fills into the gap.
 * Bullish (upside) when the first two candles are white, bearish (downside) when
 * they are black.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlxsidegap3methods](https://ta-lib.org/functions/cdlxsidegap3methods).
 *
 * **Notes**
 * - This continuation pattern does not verify the prior trend it classically
 *   assumes; the caller must confirm the trend.
 * - Bulkowski's testing found BOTH directions of this pattern actually act as
 *   reversals more often than not, opposite the classic continuation label: the
 *   upside variant reverses bearish 59% of the time, the downside variant reverses
 *   bullish 62% of the time.
 *   ([thepatternsite.com](https://thepatternsite.com/UpGap3Methods.html))
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link cdlxsidegap3methodsLookback} is a
 * success with `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inOpen - Open price of each bar.
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param outInteger - +100 when the two same-color candles are white
 *   (bullish/upside continuation), -100 when black (bearish/downside continuation),
 *   0 otherwise. Equals candlecolor(1st candle) * 100. Must hold the values produced
 *   and never be empty; the input length always suffices.
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
 * import { cdlxsidegap3methods } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const out = new Int32Array(open.length);
 * const { begIdx, count } = cdlxsidegap3methods(
 *   0, open.length - 1,
 *   open, high, low, close,
 *   out,
 * );
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`cdlgapsidesidewhite`](https://ta-lib.org/functions/cdlgapsidesidewhite)
 * @see [`cdltasukigap`](https://ta-lib.org/functions/cdltasukigap)
 * @see [`cdlrisefall3methods`](https://ta-lib.org/functions/cdlrisefall3methods)
 */
export function cdlxsidegap3methods(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDLXSIDEGAP3METHODS', 'startIdx', startIdx);
  endIdx = requireInt32('CDLXSIDEGAP3METHODS', 'endIdx', endIdx);
  requireIndexRange('CDLXSIDEGAP3METHODS', startIdx, endIdx);
  const guardStart = clampedStart('CDLXSIDEGAP3METHODS', startIdx, cdlxsidegap3methodsLookback(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLXSIDEGAP3METHODS', 'inOpen', inOpen, guardInLen);
  requireLength('CDLXSIDEGAP3METHODS', 'inHigh', inHigh, guardInLen);
  requireLength('CDLXSIDEGAP3METHODS', 'inLow', inLow, guardInLen);
  requireLength('CDLXSIDEGAP3METHODS', 'inClose', inClose, guardInLen);
  requireOutput('CDLXSIDEGAP3METHODS', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64) || overlaps(outInteger, inHighF64) || overlaps(outInteger, inLowF64) || overlaps(outInteger, inCloseF64)) {
    throw failure('CDLXSIDEGAP3METHODS', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdlxsidegap3methodsImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLXSIDEGAP3METHODS', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live CDLXSIDEGAP3METHODS stream: one value per closed bar, bit-identical to
 * {@link cdlxsidegap3methods} over the same series. Open it with
 * {@link cdlxsidegap3methodsOpen} or {@link cdlxsidegap3methodsOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlxsidegap3methods](https://ta-lib.org/functions/cdlxsidegap3methods).
 */
export class Cdlxsidegap3methodsStream {
  /** @internal */
  core: Core;
  /** @internal */
  lag1_inOpen: number = 0;
  /** @internal */
  lag2_inOpen: number = 0;
  /** @internal */
  lag1_inClose: number = 0;
  /** @internal */
  lag2_inClose: number = 0;
  /** @internal */
  cur_outInteger: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): Cdlxsidegap3methodsStream {
    return new Cdlxsidegap3methodsStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cdlxsidegap3methods} reports them over the same bars. Each accepted
   * `update` and each `advance` adds one to `count`.
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
      throw failure('CDLXSIDEGAP3METHODS advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw failure('CDLXSIDEGAP3METHODS update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inOpen) || !Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('CDLXSIDEGAP3METHODS update', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    cdlxsidegap3methodsStepImpl(this, inOpen, inHigh, inLow, inClose);
    this.outRangeCount++;
    return this.cur_outInteger;
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
      throw nonFiniteBar('CDLXSIDEGAP3METHODS peek', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let cur_outInteger = 0;
    if (((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) && /* 1st and 2nd of same color */
        ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === (-((inClose >= inOpen) ? 1 : -1) | 0) && /* 3rd opposite color */
        inOpen < (sp.lag1_inClose > sp.lag1_inOpen ? sp.lag1_inClose : sp.lag1_inOpen) && /* 3rd opens within 2nd rb */
        inOpen > (sp.lag1_inClose < sp.lag1_inOpen ? sp.lag1_inClose : sp.lag1_inOpen) &&
        inClose < (sp.lag2_inClose > sp.lag2_inOpen ? sp.lag2_inClose : sp.lag2_inOpen) && /* 3rd closes within 1st rb */
        inClose > (sp.lag2_inClose < sp.lag2_inOpen ? sp.lag2_inClose : sp.lag2_inOpen) &&
        (((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === 1 &&  /* when 1st is white */
          ((sp.lag1_inOpen < sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose) > (sp.lag2_inOpen > sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose)) || /* upside gap */
         ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === -1 && /* when 1st is black */
          ((sp.lag1_inOpen > sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose) < (sp.lag2_inOpen < sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose)))) /* downside gap */
    {
      cur_outInteger = ((((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) * 100) | 0);
    } else {
      cur_outInteger = 0;
    }
    return cur_outInteger;
  }

  /** The value at the last bar this stream counted; `peek` does not change it. */
  value(): number {
    return this.cur_outInteger;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): Cdlxsidegap3methodsStream {
    const c = new Cdlxsidegap3methodsStream(this.core);
    c.lag1_inOpen = this.lag1_inOpen;
    c.lag2_inOpen = this.lag2_inOpen;
    c.lag1_inClose = this.lag1_inClose;
    c.lag2_inClose = this.lag2_inClose;
    c.cur_outInteger = this.cur_outInteger;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cdlxsidegap3methodsStepImpl(sp: Cdlxsidegap3methodsStream, inOpen: number, _inHigh: number, _inLow: number, inClose: number): void {
  if (((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) && /* 1st and 2nd of same color */
      ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === (-((inClose >= inOpen) ? 1 : -1) | 0) && /* 3rd opposite color */
      inOpen < (sp.lag1_inClose > sp.lag1_inOpen ? sp.lag1_inClose : sp.lag1_inOpen) && /* 3rd opens within 2nd rb */
      inOpen > (sp.lag1_inClose < sp.lag1_inOpen ? sp.lag1_inClose : sp.lag1_inOpen) &&
      inClose < (sp.lag2_inClose > sp.lag2_inOpen ? sp.lag2_inClose : sp.lag2_inOpen) && /* 3rd closes within 1st rb */
      inClose > (sp.lag2_inClose < sp.lag2_inOpen ? sp.lag2_inClose : sp.lag2_inOpen) &&
      (((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === 1 &&  /* when 1st is white */
        ((sp.lag1_inOpen < sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose) > (sp.lag2_inOpen > sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose)) || /* upside gap */
       ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === -1 && /* when 1st is black */
        ((sp.lag1_inOpen > sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose) < (sp.lag2_inOpen < sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose)))) /* downside gap */
  {
    sp.cur_outInteger = ((((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) * 100) | 0);
  } else {
    sp.cur_outInteger = 0;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  sp.lag2_inOpen = sp.lag1_inOpen;
  sp.lag1_inOpen = inOpen;
  sp.lag2_inClose = sp.lag1_inClose;
  sp.lag1_inClose = inClose;
}

function cdlxsidegap3methodsOpenImpl(
  sp: Cdlxsidegap3methodsStream,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let i = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
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
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlxsidegap3methodsLookback(core);
  /* Move up the start index if there is not
   * enough initial data.
   */
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Do the calculation using tight loops. */
  /* Add-up the initial period, except for the last value. */
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first candle: white (black) candle
   * - second candle: white (black) candle
   * - upside (downside) gap between the first and the second real bodies
   * - third candle: black (white) candle that opens within the second real body and closes within the first real body
   */
  outIdx = 0;
  do {
    if (((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) && /* 1st and 2nd of same color */
        ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === (-((inClose[i] >= inOpen[i]) ? 1 : -1) | 0) && /* 3rd opposite color */
        inOpen[i] < (inClose[i - 1] > inOpen[i - 1] ? inClose[i - 1] : inOpen[i - 1]) && /* 3rd opens within 2nd rb */
        inOpen[i] > (inClose[i - 1] < inOpen[i - 1] ? inClose[i - 1] : inOpen[i - 1]) &&
        inClose[i] < (inClose[i - 2] > inOpen[i - 2] ? inClose[i - 2] : inOpen[i - 2]) && /* 3rd closes within 1st rb */
        inClose[i] > (inClose[i - 2] < inOpen[i - 2] ? inClose[i - 2] : inOpen[i - 2]) &&
        (((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === 1 &&  /* when 1st is white */
          ((inOpen[i - 1] < inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1]) > (inOpen[i - 2] > inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2])) || /* upside gap */
         ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === -1 && /* when 1st is black */
          ((inOpen[i - 1] > inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1]) < (inOpen[i - 2] < inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2])))) /* downside gap */
    {
      outInteger[((outIdx++ * outStride) | 0)] = ((((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) * 100) | 0);
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    i += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  sp.lag1_inOpen = inOpen[historyLen - 1];
  sp.lag2_inOpen = inOpen[historyLen - 2];
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.lag2_inClose = inClose[historyLen - 2];
  sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlxsidegap3methodsOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): Cdlxsidegap3methodsStream {
  const sp = Cdlxsidegap3methodsStream.of(core);
  const retCode = cdlxsidegap3methodsOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLXSIDEGAP3METHODS openAndFill', inOpen.length, startIdx, cdlxsidegap3methodsLookback(core));
  }
  throw streamFailure('CDLXSIDEGAP3METHODS openAndFill', retCode);
}

/** @internal */
export function cdlxsidegap3methodsOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): Cdlxsidegap3methodsStream {
  const sp = Cdlxsidegap3methodsStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdlxsidegap3methodsOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLXSIDEGAP3METHODS open', inOpen.length, startIdx, cdlxsidegap3methodsLookback(core));
  }
  throw streamFailure('CDLXSIDEGAP3METHODS open', retCode);
}

/**
 * Opens a live CDLXSIDEGAP3METHODS stream over the warm-up history: `value()`
 * starts at the last history bar, bit-identical to {@link cdlxsidegap3methods}
 * there.
 *
 * @param inOpen - Warm-up history. Open price of each bar. Every history has the
 *   same length.
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
 *   than `cdlxsidegap3methodsLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX`
 *   for an empty one, and `TA_BAD_PARAM` for histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdlxsidegap3methodsOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdlxsidegap3methodsOpen(open, high, low, close);
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdlxsidegap3methodsOpen(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): Cdlxsidegap3methodsStream {
  requireArgument('CDLXSIDEGAP3METHODS open', 'inOpen', inOpen);
  requireHistory('CDLXSIDEGAP3METHODS open', inOpen.length);
  requireArgument('CDLXSIDEGAP3METHODS open', 'inHigh', inHigh);
  requireArgument('CDLXSIDEGAP3METHODS open', 'inLow', inLow);
  requireArgument('CDLXSIDEGAP3METHODS open', 'inClose', inClose);
  requireHistoryLength('CDLXSIDEGAP3METHODS open', 'inHigh', inHigh.length, inOpen.length);
  requireHistoryLength('CDLXSIDEGAP3METHODS open', 'inLow', inLow.length, inOpen.length);
  requireHistoryLength('CDLXSIDEGAP3METHODS open', 'inClose', inClose.length, inOpen.length);
  return cdlxsidegap3methodsOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/**
 * {@link cdlxsidegap3methodsOpen} that also fills the outputs over the whole
 * history in the same pass, bit-identically to {@link cdlxsidegap3methods}. The
 * range written is the stream's `outRange()`.
 *
 * @param inOpen - Warm-up history. Open price of each bar. Every history has the
 *   same length.
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param outInteger - +100 when the two same-color candles are white
 *   (bullish/upside continuation), -100 when black (bearish/downside continuation),
 *   0 otherwise. Equals candlecolor(1st candle) * 100. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlxsidegap3methodsLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX`
 *   for an empty one, and `TA_BAD_PARAM` for histories of different lengths or an
 *   output too short or starting where another output or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdlxsidegap3methodsOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const out = new Int32Array(open.length);
 * const stream = cdlxsidegap3methodsOpenAndFill(open, high, low, close, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function cdlxsidegap3methodsOpenAndFill(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): Cdlxsidegap3methodsStream {
  requireArgument('CDLXSIDEGAP3METHODS openAndFill', 'inOpen', inOpen);
  requireHistory('CDLXSIDEGAP3METHODS openAndFill', inOpen.length);
  requireArgument('CDLXSIDEGAP3METHODS openAndFill', 'inHigh', inHigh);
  requireArgument('CDLXSIDEGAP3METHODS openAndFill', 'inLow', inLow);
  requireArgument('CDLXSIDEGAP3METHODS openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('CDLXSIDEGAP3METHODS openAndFill', inOpen.length, cdlxsidegap3methodsLookback(core));
  requireHistoryLength('CDLXSIDEGAP3METHODS openAndFill', 'inHigh', inHigh.length, inOpen.length);
  requireHistoryLength('CDLXSIDEGAP3METHODS openAndFill', 'inLow', inLow.length, inOpen.length);
  requireHistoryLength('CDLXSIDEGAP3METHODS openAndFill', 'inClose', inClose.length, inOpen.length);
  requireOutput('CDLXSIDEGAP3METHODS openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) || (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) || (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) || (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset)) {
    throw streamFailure('CDLXSIDEGAP3METHODS openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outInteger, inOpenF64) || overlaps(outInteger, inHighF64) || overlaps(outInteger, inLowF64) || overlaps(outInteger, inCloseF64))) {
    throw streamFailure('CDLXSIDEGAP3METHODS openAndFill', RetCode.BAD_PARAM);
  }
  return cdlxsidegap3methodsOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}
