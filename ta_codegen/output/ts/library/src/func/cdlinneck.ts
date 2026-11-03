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
 *  121104 AC     Creation
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 *  103126 MF,CC  Pre-check the cheap tests in an outer if, the candle averages inside
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { CandleSettingType, Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link cdlinneck} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function cdlinneckLookback(core: Core = Core.DEFAULT): number {
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  const Equal_avgPeriod = core.candleSettings[CandleSettingType.EQUAL].avgPeriod;
  return (Equal_avgPeriod > BodyLong_avgPeriod ? Equal_avgPeriod : BodyLong_avgPeriod) + 1;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdlinneck}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function cdlinneckDisplayShift(outputIdx: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdlinneckImpl(
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
  let EqualPeriodTotal = 0;
  let BodyLongPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let EqualTrailingIdx = 0;
  let BodyLongTrailingIdx = 0;
  let lookbackTotal = 0;
  const BodyLong_rangeType: number = core.candleSettings[CandleSettingType.BODY_LONG].rangeType;
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  const BodyLong_factor = core.candleSettings[CandleSettingType.BODY_LONG].factor;
  const Equal_rangeType: number = core.candleSettings[CandleSettingType.EQUAL].rangeType;
  const Equal_avgPeriod = core.candleSettings[CandleSettingType.EQUAL].avgPeriod;
  const Equal_factor = core.candleSettings[CandleSettingType.EQUAL].factor;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlinneckLookback(core);
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
  EqualPeriodTotal = 0;
  EqualTrailingIdx = startIdx - Equal_avgPeriod;
  BodyLongPeriodTotal = 0;
  BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
  i = EqualTrailingIdx;
  while (i < startIdx) {
    EqualPeriodTotal += ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    i += 1;
  }
  i = BodyLongTrailingIdx;
  while (i < startIdx) {
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first candle: long black candle
   * - second candle: white candle with open below previous day low and close slightly into previous day body
   * The meaning of "equal" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === -1 && /* 1st: black */
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === 1 &&          /* 2nd: white */
        inOpen[i] < inLow[i - 1] &&                            /* open below prior low */
        Math.abs(inClose[i - 1] - inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* long */
        inClose[i] <= inClose[i - 1] + ((Equal_factor * (((Equal_avgPeriod !== 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))) && /* close slightly into prior body */
        inClose[i] >= inClose[i - 1])
    {
      outInteger[outIdx++] = -100;
    } else {
      outInteger[outIdx++] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    EqualPeriodTotal += ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((Equal_rangeType === 0) ? (Math.abs(inClose[EqualTrailingIdx - 1] - inOpen[EqualTrailingIdx - 1])) : ((Equal_rangeType === 1) ? (inHigh[EqualTrailingIdx - 1] - inLow[EqualTrailingIdx - 1]) : ((Equal_rangeType === 2) ? ((inHigh[EqualTrailingIdx - 1] - (((inClose[EqualTrailingIdx - 1]) >= (inOpen[EqualTrailingIdx - 1])) ? (inClose[EqualTrailingIdx - 1]) : (inOpen[EqualTrailingIdx - 1]))) + ((((inClose[EqualTrailingIdx - 1]) >= (inOpen[EqualTrailingIdx - 1])) ? (inOpen[EqualTrailingIdx - 1]) : (inClose[EqualTrailingIdx - 1])) - inLow[EqualTrailingIdx - 1])) : 0.0)));
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 1] - inOpen[BodyLongTrailingIdx - 1])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyLongTrailingIdx - 1] - inLow[BodyLongTrailingIdx - 1]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyLongTrailingIdx - 1] - (((inClose[BodyLongTrailingIdx - 1]) >= (inOpen[BodyLongTrailingIdx - 1])) ? (inClose[BodyLongTrailingIdx - 1]) : (inOpen[BodyLongTrailingIdx - 1]))) + ((((inClose[BodyLongTrailingIdx - 1]) >= (inOpen[BodyLongTrailingIdx - 1])) ? (inOpen[BodyLongTrailingIdx - 1]) : (inClose[BodyLongTrailingIdx - 1])) - inLow[BodyLongTrailingIdx - 1])) : 0.0)));
    i += 1;
    EqualTrailingIdx += 1;
    BodyLongTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/**
 * A two-candle in-neck pattern: a long black candle followed by a white candle
 * that opens below the prior low and closes just barely into the prior body (near
 * the prior close). It is a bearish continuation signal. A hit signals bearish
 * continuation (the down move is expected to resume).
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlinneck](https://ta-lib.org/functions/cdlinneck).
 *
 * **Notes**
 * - Does not verify the preceding downtrend that this bearish continuation pattern
 *   assumes.
 * - Bulkowski's testing found the bearish continuation holds only 53% of the time
 *   — "near random" — though its overall post-breakout performance still ranks a
 *   strong 17th of 103.
 *   ([thepatternsite.com](https://www.thepatternsite.com/InNeck.html))
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link cdlinneckLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inOpen - Open price of each bar.
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param outInteger - -100 when the in-neck pattern is detected, 0 otherwise. This
 *   pattern only ever emits the negative (bearish) signal; it never emits +100. Must
 *   hold the values produced and never be empty; the input length always suffices.
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
 * import { cdlinneck } from 'ta-lib';
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
 * const { begIdx, count } = cdlinneck(
 *   0, open.length - 1,
 *   open, high, low, close,
 *   out,
 * );
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`cdlonneck`](https://ta-lib.org/functions/cdlonneck)
 * @see [`cdlthrusting`](https://ta-lib.org/functions/cdlthrusting)
 * @see [`cdlmatchinglow`](https://ta-lib.org/functions/cdlmatchinglow)
 */
export function cdlinneck(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDLINNECK', 'startIdx', startIdx);
  endIdx = requireInt32('CDLINNECK', 'endIdx', endIdx);
  requireIndexRange('CDLINNECK', startIdx, endIdx);
  const guardStart = clampedStart('CDLINNECK', startIdx, cdlinneckLookback(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLINNECK', 'inOpen', inOpen, guardInLen);
  requireLength('CDLINNECK', 'inHigh', inHigh, guardInLen);
  requireLength('CDLINNECK', 'inLow', inLow, guardInLen);
  requireLength('CDLINNECK', 'inClose', inClose, guardInLen);
  requireLength('CDLINNECK', 'outInteger', outInteger, guardOutLen);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64) || overlaps(outInteger, inHighF64) || overlaps(outInteger, inLowF64) || overlaps(outInteger, inCloseF64)) {
    throw failure('CDLINNECK', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdlinneckImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLINNECK', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live CDLINNECK stream: one value per closed bar, bit-identical to
 * {@link cdlinneck} over the same series. Open it with {@link cdlinneckOpen} or
 * {@link cdlinneckOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlinneck](https://ta-lib.org/functions/cdlinneck).
 */
export class CdlinneckStream {
  /** @internal */
  core: Core;
  /** @internal */
  EqualPeriodTotal: number = 0;
  /** @internal */
  BodyLongPeriodTotal: number = 0;
  /** @internal */
  lag1_inOpen: number = 0;
  /** @internal */
  lag1_inHigh: number = 0;
  /** @internal */
  lag1_inLow: number = 0;
  /** @internal */
  lag1_inClose: number = 0;
  /** @internal */
  ringPos_BodyLongTrailingIdx: number = 0;
  /** @internal */
  ringCap_BodyLongTrailingIdx: number = 2;
  /** @internal */
  ringLag_BodyLongTrailingIdx: number = 0;
  /** @internal */
  ring_BodyLongTrailingIdx_derived: Float64Array = new Float64Array(2);
  /** @internal */
  ringPos_EqualTrailingIdx: number = 0;
  /** @internal */
  ringCap_EqualTrailingIdx: number = 2;
  /** @internal */
  ringLag_EqualTrailingIdx: number = 0;
  /** @internal */
  ring_EqualTrailingIdx_derived: Float64Array = new Float64Array(2);
  /** @internal */
  cs_BodyLong_rangeType: number = 0;
  /** @internal */
  cs_BodyLong_avgPeriod: number = 0;
  /** @internal */
  cs_BodyLong_factor: number = 0;
  /** @internal */
  cs_Equal_rangeType: number = 0;
  /** @internal */
  cs_Equal_avgPeriod: number = 0;
  /** @internal */
  cs_Equal_factor: number = 0;
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
   * {@link cdlinneck} reports them over the same bars. Each accepted `update` and
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
      throw failure('CDLINNECK advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw failure('CDLINNECK update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inOpen) || !Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('CDLINNECK update', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    cdlinneckStepImpl(this, inOpen, inHigh, inLow, inClose);
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
      throw nonFiniteBar('CDLINNECK peek', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let cur_outInteger = 0;
    const BodyLong_rangeType = sp.cs_BodyLong_rangeType;
    const BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
    const BodyLong_factor = sp.cs_BodyLong_factor;
    const Equal_rangeType = sp.cs_Equal_rangeType;
    const Equal_avgPeriod = sp.cs_Equal_avgPeriod;
    const Equal_factor = sp.cs_Equal_factor;
    if (((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === -1 && /* 1st: black */
        ((inClose >= inOpen) ? 1 : -1) === 1 &&                  /* 2nd: white */
        inOpen < sp.lag1_inLow &&                                /* open below prior low */
        Math.abs(sp.lag1_inClose - sp.lag1_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* long */
        inClose <= sp.lag1_inClose + ((Equal_factor * (((Equal_avgPeriod !== 0) ? (sp.EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))) && /* close slightly into prior body */
        inClose >= sp.lag1_inClose)
    {
      cur_outInteger = -100;
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
  clone(): CdlinneckStream {
    const c = new CdlinneckStream(this.core);
    c.EqualPeriodTotal = this.EqualPeriodTotal;
    c.BodyLongPeriodTotal = this.BodyLongPeriodTotal;
    c.lag1_inOpen = this.lag1_inOpen;
    c.lag1_inHigh = this.lag1_inHigh;
    c.lag1_inLow = this.lag1_inLow;
    c.lag1_inClose = this.lag1_inClose;
    c.ringPos_BodyLongTrailingIdx = this.ringPos_BodyLongTrailingIdx;
    c.ringCap_BodyLongTrailingIdx = this.ringCap_BodyLongTrailingIdx;
    c.ringLag_BodyLongTrailingIdx = this.ringLag_BodyLongTrailingIdx;
    c.ring_BodyLongTrailingIdx_derived = this.ring_BodyLongTrailingIdx_derived.slice();
    c.ringPos_EqualTrailingIdx = this.ringPos_EqualTrailingIdx;
    c.ringCap_EqualTrailingIdx = this.ringCap_EqualTrailingIdx;
    c.ringLag_EqualTrailingIdx = this.ringLag_EqualTrailingIdx;
    c.ring_EqualTrailingIdx_derived = this.ring_EqualTrailingIdx_derived.slice();
    c.cs_BodyLong_rangeType = this.cs_BodyLong_rangeType;
    c.cs_BodyLong_avgPeriod = this.cs_BodyLong_avgPeriod;
    c.cs_BodyLong_factor = this.cs_BodyLong_factor;
    c.cs_Equal_rangeType = this.cs_Equal_rangeType;
    c.cs_Equal_avgPeriod = this.cs_Equal_avgPeriod;
    c.cs_Equal_factor = this.cs_Equal_factor;
    c.cur_outInteger = this.cur_outInteger;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cdlinneckStepImpl(sp: CdlinneckStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  const BodyLong_rangeType = sp.cs_BodyLong_rangeType;
  const BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
  const BodyLong_factor = sp.cs_BodyLong_factor;
  const Equal_rangeType = sp.cs_Equal_rangeType;
  const Equal_avgPeriod = sp.cs_Equal_avgPeriod;
  const Equal_factor = sp.cs_Equal_factor;
  sp.ring_BodyLongTrailingIdx_derived[sp.ringPos_BodyLongTrailingIdx] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType === 1) ? (inHigh - inLow) : ((BodyLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  sp.ring_EqualTrailingIdx_derived[sp.ringPos_EqualTrailingIdx] = ((Equal_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((Equal_rangeType === 1) ? (inHigh - inLow) : ((Equal_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  if (((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === -1 && /* 1st: black */
      ((inClose >= inOpen) ? 1 : -1) === 1 &&                  /* 2nd: white */
      inOpen < sp.lag1_inLow &&                                /* open below prior low */
      Math.abs(sp.lag1_inClose - sp.lag1_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* long */
      inClose <= sp.lag1_inClose + ((Equal_factor * (((Equal_avgPeriod !== 0) ? (sp.EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))) && /* close slightly into prior body */
      inClose >= sp.lag1_inClose)
  {
    sp.cur_outInteger = -100;
  } else {
    sp.cur_outInteger = 0;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  sp.EqualPeriodTotal += ((Equal_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0))) - sp.ring_EqualTrailingIdx_derived[(((sp.ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - sp.ringLag_EqualTrailingIdx - 1) % sp.ringCap_EqualTrailingIdx) | 0)];
  sp.BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0))) - sp.ring_BodyLongTrailingIdx_derived[(((sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 1) % sp.ringCap_BodyLongTrailingIdx) | 0)];
  sp.lag1_inOpen = inOpen;
  sp.lag1_inHigh = inHigh;
  sp.lag1_inLow = inLow;
  sp.lag1_inClose = inClose;
  sp.ringPos_BodyLongTrailingIdx = sp.ringPos_BodyLongTrailingIdx + 1;
  if (sp.ringPos_BodyLongTrailingIdx >= sp.ringCap_BodyLongTrailingIdx) {
    sp.ringPos_BodyLongTrailingIdx = 0;
  }
  sp.ringPos_EqualTrailingIdx = sp.ringPos_EqualTrailingIdx + 1;
  if (sp.ringPos_EqualTrailingIdx >= sp.ringCap_EqualTrailingIdx) {
    sp.ringPos_EqualTrailingIdx = 0;
  }
}

function cdlinneckOpenImpl(
  sp: CdlinneckStream,
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
  let EqualPeriodTotal = 0;
  let BodyLongPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let EqualTrailingIdx = 0;
  let BodyLongTrailingIdx = 0;
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
  const BodyLong_rangeType: number = core.candleSettings[CandleSettingType.BODY_LONG].rangeType;
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  const BodyLong_factor = core.candleSettings[CandleSettingType.BODY_LONG].factor;
  const Equal_rangeType: number = core.candleSettings[CandleSettingType.EQUAL].rangeType;
  const Equal_avgPeriod = core.candleSettings[CandleSettingType.EQUAL].avgPeriod;
  const Equal_factor = core.candleSettings[CandleSettingType.EQUAL].factor;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlinneckLookback(core);
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
  EqualPeriodTotal = 0;
  EqualTrailingIdx = startIdx - Equal_avgPeriod;
  BodyLongPeriodTotal = 0;
  BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
  i = EqualTrailingIdx;
  while (i < startIdx) {
    EqualPeriodTotal += ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    i += 1;
  }
  i = BodyLongTrailingIdx;
  while (i < startIdx) {
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first candle: long black candle
   * - second candle: white candle with open below previous day low and close slightly into previous day body
   * The meaning of "equal" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === -1 && /* 1st: black */
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === 1 &&          /* 2nd: white */
        inOpen[i] < inLow[i - 1] &&                            /* open below prior low */
        Math.abs(inClose[i - 1] - inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* long */
        inClose[i] <= inClose[i - 1] + ((Equal_factor * (((Equal_avgPeriod !== 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))) && /* close slightly into prior body */
        inClose[i] >= inClose[i - 1])
    {
      outInteger[((outIdx++ * outStride) | 0)] = -100;
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    EqualPeriodTotal += ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((Equal_rangeType === 0) ? (Math.abs(inClose[EqualTrailingIdx - 1] - inOpen[EqualTrailingIdx - 1])) : ((Equal_rangeType === 1) ? (inHigh[EqualTrailingIdx - 1] - inLow[EqualTrailingIdx - 1]) : ((Equal_rangeType === 2) ? ((inHigh[EqualTrailingIdx - 1] - (((inClose[EqualTrailingIdx - 1]) >= (inOpen[EqualTrailingIdx - 1])) ? (inClose[EqualTrailingIdx - 1]) : (inOpen[EqualTrailingIdx - 1]))) + ((((inClose[EqualTrailingIdx - 1]) >= (inOpen[EqualTrailingIdx - 1])) ? (inOpen[EqualTrailingIdx - 1]) : (inClose[EqualTrailingIdx - 1])) - inLow[EqualTrailingIdx - 1])) : 0.0)));
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 1] - inOpen[BodyLongTrailingIdx - 1])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyLongTrailingIdx - 1] - inLow[BodyLongTrailingIdx - 1]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyLongTrailingIdx - 1] - (((inClose[BodyLongTrailingIdx - 1]) >= (inOpen[BodyLongTrailingIdx - 1])) ? (inClose[BodyLongTrailingIdx - 1]) : (inOpen[BodyLongTrailingIdx - 1]))) + ((((inClose[BodyLongTrailingIdx - 1]) >= (inOpen[BodyLongTrailingIdx - 1])) ? (inOpen[BodyLongTrailingIdx - 1]) : (inClose[BodyLongTrailingIdx - 1])) - inLow[BodyLongTrailingIdx - 1])) : 0.0)));
    i += 1;
    EqualTrailingIdx += 1;
    BodyLongTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const capLag_BodyLongTrailingIdx = i - BodyLongTrailingIdx;
  const cap_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx + 2;
  if (capLag_BodyLongTrailingIdx < 0 || cap_BodyLongTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_BodyLongTrailingIdx = cap_BodyLongTrailingIdx > 0 ? cap_BodyLongTrailingIdx : 1;
  const capRing_BodyLongTrailingIdx_derived = new Float64Array(allocN_BodyLongTrailingIdx);
  for (let fillJ = historyLen - cap_BodyLongTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_BodyLongTrailingIdx_derived[fillJ % cap_BodyLongTrailingIdx] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyLong_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyLong_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  const capLag_EqualTrailingIdx = i - EqualTrailingIdx;
  const cap_EqualTrailingIdx = capLag_EqualTrailingIdx + 2;
  if (capLag_EqualTrailingIdx < 0 || cap_EqualTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_EqualTrailingIdx = cap_EqualTrailingIdx > 0 ? cap_EqualTrailingIdx : 1;
  const capRing_EqualTrailingIdx_derived = new Float64Array(allocN_EqualTrailingIdx);
  for (let fillJ = historyLen - cap_EqualTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_EqualTrailingIdx_derived[fillJ % cap_EqualTrailingIdx] = ((Equal_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((Equal_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((Equal_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  sp.EqualPeriodTotal = EqualPeriodTotal;
  sp.BodyLongPeriodTotal = BodyLongPeriodTotal;
  sp.lag1_inOpen = inOpen[historyLen - 1];
  sp.lag1_inHigh = inHigh[historyLen - 1];
  sp.lag1_inLow = inLow[historyLen - 1];
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.ringPos_BodyLongTrailingIdx = historyLen % cap_BodyLongTrailingIdx;
  sp.ringCap_BodyLongTrailingIdx = cap_BodyLongTrailingIdx;
  sp.ringLag_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx;
  sp.ring_BodyLongTrailingIdx_derived = capRing_BodyLongTrailingIdx_derived;
  sp.ringPos_EqualTrailingIdx = historyLen % cap_EqualTrailingIdx;
  sp.ringCap_EqualTrailingIdx = cap_EqualTrailingIdx;
  sp.ringLag_EqualTrailingIdx = capLag_EqualTrailingIdx;
  sp.ring_EqualTrailingIdx_derived = capRing_EqualTrailingIdx_derived;
  sp.cs_BodyLong_rangeType = BodyLong_rangeType;
  sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
  sp.cs_BodyLong_factor = BodyLong_factor;
  sp.cs_Equal_rangeType = Equal_rangeType;
  sp.cs_Equal_avgPeriod = Equal_avgPeriod;
  sp.cs_Equal_factor = Equal_factor;
  sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlinneckOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): CdlinneckStream {
  const sp = new CdlinneckStream(core);
  const retCode = cdlinneckOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLINNECK openAndFill', inOpen.length, startIdx, cdlinneckLookback(core));
  }
  throw streamFailure('CDLINNECK openAndFill', retCode);
}

/** @internal */
export function cdlinneckOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): CdlinneckStream {
  const sp = new CdlinneckStream(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdlinneckOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLINNECK open', inOpen.length, startIdx, cdlinneckLookback(core));
  }
  throw streamFailure('CDLINNECK open', retCode);
}

/**
 * Opens a live CDLINNECK stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link cdlinneck} there.
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
 *   than `cdlinneckLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an
 *   empty one, and `TA_BAD_PARAM` for histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdlinneckOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdlinneckOpen(open, high, low, close);
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdlinneckOpen(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): CdlinneckStream {
  requireArgument('CDLINNECK open', 'inOpen', inOpen);
  requireHistory('CDLINNECK open', inOpen.length);
  requireArgument('CDLINNECK open', 'inHigh', inHigh);
  requireArgument('CDLINNECK open', 'inLow', inLow);
  requireArgument('CDLINNECK open', 'inClose', inClose);
  requireHistoryLength('CDLINNECK open', 'inHigh', inHigh.length, inOpen.length);
  requireHistoryLength('CDLINNECK open', 'inLow', inLow.length, inOpen.length);
  requireHistoryLength('CDLINNECK open', 'inClose', inClose.length, inOpen.length);
  return cdlinneckOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/**
 * {@link cdlinneckOpen} that also fills the outputs over the whole history in the
 * same pass, bit-identically to {@link cdlinneck}. The range written is the
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
 * @param outInteger - -100 when the in-neck pattern is detected, 0 otherwise. This
 *   pattern only ever emits the negative (bearish) signal; it never emits +100. Must
 *   hold `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlinneckLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an
 *   empty one, and `TA_BAD_PARAM` for histories of different lengths or an output
 *   too short or starting where another output or a `Float64Array` input starts, or,
 *   on a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdlinneckOpenAndFill } from 'ta-lib';
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
 * const stream = cdlinneckOpenAndFill(open, high, low, close, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function cdlinneckOpenAndFill(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): CdlinneckStream {
  requireArgument('CDLINNECK openAndFill', 'inOpen', inOpen);
  requireHistory('CDLINNECK openAndFill', inOpen.length);
  requireArgument('CDLINNECK openAndFill', 'inHigh', inHigh);
  requireArgument('CDLINNECK openAndFill', 'inLow', inLow);
  requireArgument('CDLINNECK openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('CDLINNECK openAndFill', inOpen.length, cdlinneckLookback(core));
  requireHistoryLength('CDLINNECK openAndFill', 'inHigh', inHigh.length, inOpen.length);
  requireHistoryLength('CDLINNECK openAndFill', 'inLow', inLow.length, inOpen.length);
  requireHistoryLength('CDLINNECK openAndFill', 'inClose', inClose.length, inOpen.length);
  requireLength('CDLINNECK openAndFill', 'outInteger', outInteger, guardOutLen);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) || (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) || (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) || (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset)) {
    throw streamFailure('CDLINNECK openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outInteger, inOpenF64) || overlaps(outInteger, inHighF64) || overlaps(outInteger, inLowF64) || overlaps(outInteger, inCloseF64))) {
    throw streamFailure('CDLINNECK openAndFill', RetCode.BAD_PARAM);
  }
  return cdlinneckOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}
