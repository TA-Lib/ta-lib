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
 *  103126 MF,CC  Write the two gaps as an else-if chain of pre-checks
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { CandleSettingType, Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link cdltasukigap} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function cdltasukigapLookback(core: Core = Core.DEFAULT): number {
  const Near_avgPeriod = core.candleSettings[CandleSettingType.NEAR].avgPeriod;
  return Near_avgPeriod + 2;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdltasukigap}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function cdltasukigapDisplayShift(outputIdx: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdltasukigapImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): RetCode {
  let NearPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let NearTrailingIdx = 0;
  let lookbackTotal = 0;
  const Near_rangeType: number = core.candleSettings[CandleSettingType.NEAR].rangeType;
  const Near_avgPeriod = core.candleSettings[CandleSettingType.NEAR].avgPeriod;
  const Near_factor = core.candleSettings[CandleSettingType.NEAR].factor;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdltasukigapLookback(core);
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
  NearPeriodTotal = 0;
  NearTrailingIdx = startIdx - Near_avgPeriod;
  i = NearTrailingIdx;
  while (i < startIdx) {
    NearPeriodTotal += ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - upside (downside) gap
   * - first candle after the window: white (black) candlestick
   * - second candle: black (white) candlestick that opens within the previous real body and closes under (above)
   *   the previous real body inside the gap
   * - the size of two real bodies should be near the same
   * The meaning of "near" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (((inOpen[i - 1] < inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1]) > (inOpen[i - 2] > inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2])) && /* upside gap */
        ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === 1 && /* 1st: white */
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === -1)          /* 2nd: black */
    {
      if (inOpen[i] < inClose[i - 1] &&
          inOpen[i] > inOpen[i - 1] &&  /* that opens within the white rb */
          inClose[i] < inOpen[i - 1] && /* and closes under the white rb */
          inClose[i] > (inClose[i - 2] > inOpen[i - 2] ? inClose[i - 2] : inOpen[i - 2]) && /* inside the gap */
          Math.abs(Math.abs(inClose[i - 1] - inOpen[i - 1]) - Math.abs(inClose[i] - inOpen[i])) < ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0))))) /* size of 2 rb near the same */
      {
        outInteger[outIdx++] = ((((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) * 100) | 0);
      } else {
        outInteger[outIdx++] = 0;
      }
    } else if (((inOpen[i - 1] > inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1]) < (inOpen[i - 2] < inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2])) && /* downside gap */
        ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === -1 && /* 1st: black */
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === 1)            /* 2nd: white */
    {
      if (inOpen[i] < inOpen[i - 1] &&
          inOpen[i] > inClose[i - 1] && /* that opens within the black rb */
          inClose[i] > inOpen[i - 1] && /* and closes above the black rb */
          inClose[i] < (inClose[i - 2] < inOpen[i - 2] ? inClose[i - 2] : inOpen[i - 2]) && /* inside the gap */
          Math.abs(Math.abs(inClose[i - 1] - inOpen[i - 1]) - Math.abs(inClose[i] - inOpen[i])) < ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0))))) /* size of 2 rb near the same */
      {
        outInteger[outIdx++] = ((((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) * 100) | 0);
      } else {
        outInteger[outIdx++] = 0;
      }
    } else {
      outInteger[outIdx++] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    NearPeriodTotal += ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((Near_rangeType === 0) ? (Math.abs(inClose[NearTrailingIdx - 1] - inOpen[NearTrailingIdx - 1])) : ((Near_rangeType === 1) ? (inHigh[NearTrailingIdx - 1] - inLow[NearTrailingIdx - 1]) : ((Near_rangeType === 2) ? ((inHigh[NearTrailingIdx - 1] - (((inClose[NearTrailingIdx - 1]) >= (inOpen[NearTrailingIdx - 1])) ? (inClose[NearTrailingIdx - 1]) : (inOpen[NearTrailingIdx - 1]))) + ((((inClose[NearTrailingIdx - 1]) >= (inOpen[NearTrailingIdx - 1])) ? (inOpen[NearTrailingIdx - 1]) : (inClose[NearTrailingIdx - 1])) - inLow[NearTrailingIdx - 1])) : 0.0)));
    i += 1;
    NearTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/**
 * A three-candle pattern: a real-body-gapping candle followed by an opposite-color
 * candle that opens inside its body and closes back into the gap without filling
 * it. An upside gap is a bullish continuation signal; a downside gap is a bearish
 * continuation signal.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdltasukigap](https://ta-lib.org/functions/cdltasukigap).
 *
 * **Notes**
 * - This continuation pattern does not verify the prior trend it classically
 *   assumes; the caller must confirm the trend.
 * - Bulkowski's testing found the downside Tasuki Gap actually acts as a bullish
 *   REVERSAL 54% of the time — opposite its textbook bearish-continuation label —
 *   while the upside variant does continue as labeled, but only 57% of the time
 *   ("near random").
 *   ([thepatternsite.com](https://thepatternsite.com/DownsideTasukiGap.html))
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link cdltasukigapLookback} is a success
 * with `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inOpen - Open price of each bar.
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param outInteger - +100 on a bullish (upside-gap) tasuki gap, -100 on a bearish
 *   (downside-gap) tasuki gap, 0 otherwise. Sign equals the color of the gap candle
 *   i-1 (candlecolor(i-1)*100) Must hold the values produced and never be empty; the
 *   input length always suffices.
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
 * import { cdltasukigap } from 'ta-lib';
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
 * const { begIdx, count } = cdltasukigap(
 *   0, open.length - 1,
 *   open, high, low, close,
 *   out,
 * );
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`cdlgapsidesidewhite`](https://ta-lib.org/functions/cdlgapsidesidewhite)
 * @see [`cdlxsidegap3methods`](https://ta-lib.org/functions/cdlxsidegap3methods)
 */
export function cdltasukigap(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDLTASUKIGAP', 'startIdx', startIdx);
  endIdx = requireInt32('CDLTASUKIGAP', 'endIdx', endIdx);
  requireIndexRange('CDLTASUKIGAP', startIdx, endIdx);
  const guardStart = clampedStart('CDLTASUKIGAP', startIdx, cdltasukigapLookback(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLTASUKIGAP', 'inOpen', inOpen, guardInLen);
  requireLength('CDLTASUKIGAP', 'inHigh', inHigh, guardInLen);
  requireLength('CDLTASUKIGAP', 'inLow', inLow, guardInLen);
  requireLength('CDLTASUKIGAP', 'inClose', inClose, guardInLen);
  requireLength('CDLTASUKIGAP', 'outInteger', outInteger, guardOutLen);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64) || overlaps(outInteger, inHighF64) || overlaps(outInteger, inLowF64) || overlaps(outInteger, inCloseF64)) {
    throw failure('CDLTASUKIGAP', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdltasukigapImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLTASUKIGAP', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live CDLTASUKIGAP stream: one value per closed bar, bit-identical to
 * {@link cdltasukigap} over the same series. Open it with {@link cdltasukigapOpen}
 * or {@link cdltasukigapOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdltasukigap](https://ta-lib.org/functions/cdltasukigap).
 */
export class CdltasukigapStream {
  /** @internal */
  core: Core;
  /** @internal */
  NearPeriodTotal: number = 0;
  /** @internal */
  lag1_inOpen: number = 0;
  /** @internal */
  lag2_inOpen: number = 0;
  /** @internal */
  lag1_inHigh: number = 0;
  /** @internal */
  lag1_inLow: number = 0;
  /** @internal */
  lag1_inClose: number = 0;
  /** @internal */
  lag2_inClose: number = 0;
  /** @internal */
  ringPos_NearTrailingIdx: number = 0;
  /** @internal */
  ringCap_NearTrailingIdx: number = 2;
  /** @internal */
  ringLag_NearTrailingIdx: number = 0;
  /** @internal */
  ring_NearTrailingIdx_derived: Float64Array = new Float64Array(2);
  /** @internal */
  cs_Near_rangeType: number = 0;
  /** @internal */
  cs_Near_avgPeriod: number = 0;
  /** @internal */
  cs_Near_factor: number = 0;
  /** @internal */
  cur_outInteger: number = 0;
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
   * {@link cdltasukigap} reports them over the same bars. Each accepted `update` and
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
      throw failure('CDLTASUKIGAP advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw failure('CDLTASUKIGAP update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inOpen) || !Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('CDLTASUKIGAP update', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    cdltasukigapStepImpl(this, inOpen, inHigh, inLow, inClose);
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
      throw nonFiniteBar('CDLTASUKIGAP peek', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let cur_outInteger = 0;
    const Near_rangeType = sp.cs_Near_rangeType;
    const Near_avgPeriod = sp.cs_Near_avgPeriod;
    const Near_factor = sp.cs_Near_factor;
    if (((sp.lag1_inOpen < sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose) > (sp.lag2_inOpen > sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose)) && /* upside gap */
        ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === 1 && /* 1st: white */
        ((inClose >= inOpen) ? 1 : -1) === -1)                  /* 2nd: black */
    {
      if (inOpen < sp.lag1_inClose &&
          inOpen > sp.lag1_inOpen &&  /* that opens within the white rb */
          inClose < sp.lag1_inOpen && /* and closes under the white rb */
          inClose > (sp.lag2_inClose > sp.lag2_inOpen ? sp.lag2_inClose : sp.lag2_inOpen) && /* inside the gap */
          Math.abs(Math.abs(sp.lag1_inClose - sp.lag1_inOpen) - Math.abs(inClose - inOpen)) < ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0))))) /* size of 2 rb near the same */
      {
        cur_outInteger = ((((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) * 100) | 0);
      } else {
        cur_outInteger = 0;
      }
    } else if (((sp.lag1_inOpen > sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose) < (sp.lag2_inOpen < sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose)) && /* downside gap */
        ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === -1 && /* 1st: black */
        ((inClose >= inOpen) ? 1 : -1) === 1)                    /* 2nd: white */
    {
      if (inOpen < sp.lag1_inOpen &&
          inOpen > sp.lag1_inClose && /* that opens within the black rb */
          inClose > sp.lag1_inOpen && /* and closes above the black rb */
          inClose < (sp.lag2_inClose < sp.lag2_inOpen ? sp.lag2_inClose : sp.lag2_inOpen) && /* inside the gap */
          Math.abs(Math.abs(sp.lag1_inClose - sp.lag1_inOpen) - Math.abs(inClose - inOpen)) < ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0))))) /* size of 2 rb near the same */
      {
        cur_outInteger = ((((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) * 100) | 0);
      } else {
        cur_outInteger = 0;
      }
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
  clone(): CdltasukigapStream {
    const c = new CdltasukigapStream(this.core);
    c.NearPeriodTotal = this.NearPeriodTotal;
    c.lag1_inOpen = this.lag1_inOpen;
    c.lag2_inOpen = this.lag2_inOpen;
    c.lag1_inHigh = this.lag1_inHigh;
    c.lag1_inLow = this.lag1_inLow;
    c.lag1_inClose = this.lag1_inClose;
    c.lag2_inClose = this.lag2_inClose;
    c.ringPos_NearTrailingIdx = this.ringPos_NearTrailingIdx;
    c.ringCap_NearTrailingIdx = this.ringCap_NearTrailingIdx;
    c.ringLag_NearTrailingIdx = this.ringLag_NearTrailingIdx;
    c.ring_NearTrailingIdx_derived = this.ring_NearTrailingIdx_derived.slice();
    c.cs_Near_rangeType = this.cs_Near_rangeType;
    c.cs_Near_avgPeriod = this.cs_Near_avgPeriod;
    c.cs_Near_factor = this.cs_Near_factor;
    c.cur_outInteger = this.cur_outInteger;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cdltasukigapStepImpl(sp: CdltasukigapStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  const Near_rangeType = sp.cs_Near_rangeType;
  const Near_avgPeriod = sp.cs_Near_avgPeriod;
  const Near_factor = sp.cs_Near_factor;
  sp.ring_NearTrailingIdx_derived[sp.ringPos_NearTrailingIdx] = ((Near_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((Near_rangeType === 1) ? (inHigh - inLow) : ((Near_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  if (((sp.lag1_inOpen < sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose) > (sp.lag2_inOpen > sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose)) && /* upside gap */
      ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === 1 && /* 1st: white */
      ((inClose >= inOpen) ? 1 : -1) === -1)                  /* 2nd: black */
  {
    if (inOpen < sp.lag1_inClose &&
        inOpen > sp.lag1_inOpen &&  /* that opens within the white rb */
        inClose < sp.lag1_inOpen && /* and closes under the white rb */
        inClose > (sp.lag2_inClose > sp.lag2_inOpen ? sp.lag2_inClose : sp.lag2_inOpen) && /* inside the gap */
        Math.abs(Math.abs(sp.lag1_inClose - sp.lag1_inOpen) - Math.abs(inClose - inOpen)) < ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0))))) /* size of 2 rb near the same */
    {
      sp.cur_outInteger = ((((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) * 100) | 0);
    } else {
      sp.cur_outInteger = 0;
    }
  } else if (((sp.lag1_inOpen > sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose) < (sp.lag2_inOpen < sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose)) && /* downside gap */
      ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === -1 && /* 1st: black */
      ((inClose >= inOpen) ? 1 : -1) === 1)                    /* 2nd: white */
  {
    if (inOpen < sp.lag1_inOpen &&
        inOpen > sp.lag1_inClose && /* that opens within the black rb */
        inClose > sp.lag1_inOpen && /* and closes above the black rb */
        inClose < (sp.lag2_inClose < sp.lag2_inOpen ? sp.lag2_inClose : sp.lag2_inOpen) && /* inside the gap */
        Math.abs(Math.abs(sp.lag1_inClose - sp.lag1_inOpen) - Math.abs(inClose - inOpen)) < ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0))))) /* size of 2 rb near the same */
    {
      sp.cur_outInteger = ((((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) * 100) | 0);
    } else {
      sp.cur_outInteger = 0;
    }
  } else {
    sp.cur_outInteger = 0;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  sp.NearPeriodTotal += ((Near_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0))) - sp.ring_NearTrailingIdx_derived[(((sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - 1) % sp.ringCap_NearTrailingIdx) | 0)];
  sp.lag2_inOpen = sp.lag1_inOpen;
  sp.lag1_inOpen = inOpen;
  sp.lag1_inHigh = inHigh;
  sp.lag1_inLow = inLow;
  sp.lag2_inClose = sp.lag1_inClose;
  sp.lag1_inClose = inClose;
  sp.ringPos_NearTrailingIdx = sp.ringPos_NearTrailingIdx + 1;
  if (sp.ringPos_NearTrailingIdx >= sp.ringCap_NearTrailingIdx) {
    sp.ringPos_NearTrailingIdx = 0;
  }
}

function cdltasukigapOpenImpl(
  sp: CdltasukigapStream,
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
  let NearPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let NearTrailingIdx = 0;
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
  const Near_rangeType: number = core.candleSettings[CandleSettingType.NEAR].rangeType;
  const Near_avgPeriod = core.candleSettings[CandleSettingType.NEAR].avgPeriod;
  const Near_factor = core.candleSettings[CandleSettingType.NEAR].factor;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdltasukigapLookback(core);
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
  NearPeriodTotal = 0;
  NearTrailingIdx = startIdx - Near_avgPeriod;
  i = NearTrailingIdx;
  while (i < startIdx) {
    NearPeriodTotal += ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - upside (downside) gap
   * - first candle after the window: white (black) candlestick
   * - second candle: black (white) candlestick that opens within the previous real body and closes under (above)
   *   the previous real body inside the gap
   * - the size of two real bodies should be near the same
   * The meaning of "near" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (((inOpen[i - 1] < inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1]) > (inOpen[i - 2] > inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2])) && /* upside gap */
        ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === 1 && /* 1st: white */
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === -1)          /* 2nd: black */
    {
      if (inOpen[i] < inClose[i - 1] &&
          inOpen[i] > inOpen[i - 1] &&  /* that opens within the white rb */
          inClose[i] < inOpen[i - 1] && /* and closes under the white rb */
          inClose[i] > (inClose[i - 2] > inOpen[i - 2] ? inClose[i - 2] : inOpen[i - 2]) && /* inside the gap */
          Math.abs(Math.abs(inClose[i - 1] - inOpen[i - 1]) - Math.abs(inClose[i] - inOpen[i])) < ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0))))) /* size of 2 rb near the same */
      {
        outInteger[((outIdx++ * outStride) | 0)] = ((((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) * 100) | 0);
      } else {
        outInteger[((outIdx++ * outStride) | 0)] = 0;
      }
    } else if (((inOpen[i - 1] > inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1]) < (inOpen[i - 2] < inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2])) && /* downside gap */
        ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === -1 && /* 1st: black */
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === 1)            /* 2nd: white */
    {
      if (inOpen[i] < inOpen[i - 1] &&
          inOpen[i] > inClose[i - 1] && /* that opens within the black rb */
          inClose[i] > inOpen[i - 1] && /* and closes above the black rb */
          inClose[i] < (inClose[i - 2] < inOpen[i - 2] ? inClose[i - 2] : inOpen[i - 2]) && /* inside the gap */
          Math.abs(Math.abs(inClose[i - 1] - inOpen[i - 1]) - Math.abs(inClose[i] - inOpen[i])) < ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0))))) /* size of 2 rb near the same */
      {
        outInteger[((outIdx++ * outStride) | 0)] = ((((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) * 100) | 0);
      } else {
        outInteger[((outIdx++ * outStride) | 0)] = 0;
      }
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    NearPeriodTotal += ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((Near_rangeType === 0) ? (Math.abs(inClose[NearTrailingIdx - 1] - inOpen[NearTrailingIdx - 1])) : ((Near_rangeType === 1) ? (inHigh[NearTrailingIdx - 1] - inLow[NearTrailingIdx - 1]) : ((Near_rangeType === 2) ? ((inHigh[NearTrailingIdx - 1] - (((inClose[NearTrailingIdx - 1]) >= (inOpen[NearTrailingIdx - 1])) ? (inClose[NearTrailingIdx - 1]) : (inOpen[NearTrailingIdx - 1]))) + ((((inClose[NearTrailingIdx - 1]) >= (inOpen[NearTrailingIdx - 1])) ? (inOpen[NearTrailingIdx - 1]) : (inClose[NearTrailingIdx - 1])) - inLow[NearTrailingIdx - 1])) : 0.0)));
    i += 1;
    NearTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const capLag_NearTrailingIdx = i - NearTrailingIdx;
  const cap_NearTrailingIdx = capLag_NearTrailingIdx + 2;
  if (capLag_NearTrailingIdx < 0 || cap_NearTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_NearTrailingIdx = cap_NearTrailingIdx > 0 ? cap_NearTrailingIdx : 1;
  const capRing_NearTrailingIdx_derived = new Float64Array(allocN_NearTrailingIdx);
  for (let fillJ = historyLen - cap_NearTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_NearTrailingIdx_derived[fillJ % cap_NearTrailingIdx] = ((Near_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((Near_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((Near_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  sp.NearPeriodTotal = NearPeriodTotal;
  sp.lag1_inOpen = inOpen[historyLen - 1];
  sp.lag2_inOpen = inOpen[historyLen - 2];
  sp.lag1_inHigh = inHigh[historyLen - 1];
  sp.lag1_inLow = inLow[historyLen - 1];
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.lag2_inClose = inClose[historyLen - 2];
  sp.ringPos_NearTrailingIdx = historyLen % cap_NearTrailingIdx;
  sp.ringCap_NearTrailingIdx = cap_NearTrailingIdx;
  sp.ringLag_NearTrailingIdx = capLag_NearTrailingIdx;
  sp.ring_NearTrailingIdx_derived = capRing_NearTrailingIdx_derived;
  sp.cs_Near_rangeType = Near_rangeType;
  sp.cs_Near_avgPeriod = Near_avgPeriod;
  sp.cs_Near_factor = Near_factor;
  sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cdltasukigapOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): CdltasukigapStream {
  const sp = new CdltasukigapStream(core);
  const retCode = cdltasukigapOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLTASUKIGAP openAndFill', inOpen.length, startIdx, cdltasukigapLookback(core));
  }
  throw streamFailure('CDLTASUKIGAP openAndFill', retCode);
}

/** @internal */
export function cdltasukigapOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): CdltasukigapStream {
  const sp = new CdltasukigapStream(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdltasukigapOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLTASUKIGAP open', inOpen.length, startIdx, cdltasukigapLookback(core));
  }
  throw streamFailure('CDLTASUKIGAP open', retCode);
}

/**
 * Opens a live CDLTASUKIGAP stream over the warm-up history: `value()` starts at
 * the last history bar, bit-identical to {@link cdltasukigap} there.
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
 *   than `cdltasukigapLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an
 *   empty one, and `TA_BAD_PARAM` for histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdltasukigapOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdltasukigapOpen(open, high, low, close);
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdltasukigapOpen(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): CdltasukigapStream {
  requireArgument('CDLTASUKIGAP open', 'inOpen', inOpen);
  requireHistory('CDLTASUKIGAP open', inOpen.length);
  requireArgument('CDLTASUKIGAP open', 'inHigh', inHigh);
  requireArgument('CDLTASUKIGAP open', 'inLow', inLow);
  requireArgument('CDLTASUKIGAP open', 'inClose', inClose);
  requireHistoryLength('CDLTASUKIGAP open', 'inHigh', inHigh.length, inOpen.length);
  requireHistoryLength('CDLTASUKIGAP open', 'inLow', inLow.length, inOpen.length);
  requireHistoryLength('CDLTASUKIGAP open', 'inClose', inClose.length, inOpen.length);
  return cdltasukigapOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/**
 * {@link cdltasukigapOpen} that also fills the outputs over the whole history in
 * the same pass, bit-identically to {@link cdltasukigap}. The range written is the
 * stream's `outRange()`.
 *
 * @param inOpen - Warm-up history. Open price of each bar. Every history has the
 *   same length.
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param outInteger - +100 on a bullish (upside-gap) tasuki gap, -100 on a bearish
 *   (downside-gap) tasuki gap, 0 otherwise. Sign equals the color of the gap candle
 *   i-1 (candlecolor(i-1)*100) Must hold `historyLen - lookback` values and share no
 *   memory with an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdltasukigapLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an
 *   empty one, and `TA_BAD_PARAM` for histories of different lengths or an output
 *   too short or starting where another output or a `Float64Array` input starts, or,
 *   on a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdltasukigapOpenAndFill } from 'ta-lib';
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
 * const stream = cdltasukigapOpenAndFill(open, high, low, close, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function cdltasukigapOpenAndFill(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): CdltasukigapStream {
  requireArgument('CDLTASUKIGAP openAndFill', 'inOpen', inOpen);
  requireHistory('CDLTASUKIGAP openAndFill', inOpen.length);
  requireArgument('CDLTASUKIGAP openAndFill', 'inHigh', inHigh);
  requireArgument('CDLTASUKIGAP openAndFill', 'inLow', inLow);
  requireArgument('CDLTASUKIGAP openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('CDLTASUKIGAP openAndFill', inOpen.length, cdltasukigapLookback(core));
  requireHistoryLength('CDLTASUKIGAP openAndFill', 'inHigh', inHigh.length, inOpen.length);
  requireHistoryLength('CDLTASUKIGAP openAndFill', 'inLow', inLow.length, inOpen.length);
  requireHistoryLength('CDLTASUKIGAP openAndFill', 'inClose', inClose.length, inOpen.length);
  requireLength('CDLTASUKIGAP openAndFill', 'outInteger', outInteger, guardOutLen);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) || (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) || (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) || (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset)) {
    throw streamFailure('CDLTASUKIGAP openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outInteger, inOpenF64) || overlaps(outInteger, inHighF64) || overlaps(outInteger, inLowF64) || overlaps(outInteger, inCloseF64))) {
    throw streamFailure('CDLTASUKIGAP openAndFill', RetCode.BAD_PARAM);
  }
  return cdltasukigapOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}
