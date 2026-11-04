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
 *  103104 AC     Creation
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 *  103126 MF,CC  Pre-check the cheap tests in an outer if, the candle averages inside
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { CandleSettingType, Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link cdlidentical3crows} consumes before its
 * first value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function cdlidentical3crowsLookback(core: Core = Core.DEFAULT): number {
  const Equal_avgPeriod = core.candleSettings[CandleSettingType.EQUAL].avgPeriod;
  const ShadowVeryShort_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].avgPeriod;
  return (ShadowVeryShort_avgPeriod > Equal_avgPeriod ? ShadowVeryShort_avgPeriod : Equal_avgPeriod) + 2;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdlidentical3crows}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function cdlidentical3crowsDisplayShift(outputIdx: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdlidentical3crowsImpl(
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
  const ShadowVeryShortPeriodTotal = new Float64Array(3);
  const EqualPeriodTotal = new Float64Array(3);
  let i = 0;
  let outIdx = 0;
  let totIdx = 0;
  let ShadowVeryShortTrailingIdx = 0;
  let EqualTrailingIdx = 0;
  let lookbackTotal = 0;
  const Equal_rangeType: number = core.candleSettings[CandleSettingType.EQUAL].rangeType;
  const Equal_avgPeriod = core.candleSettings[CandleSettingType.EQUAL].avgPeriod;
  const Equal_factor = core.candleSettings[CandleSettingType.EQUAL].factor;
  const ShadowVeryShort_rangeType: number = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].rangeType;
  const ShadowVeryShort_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].avgPeriod;
  const ShadowVeryShort_factor = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].factor;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlidentical3crowsLookback(core);
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
  ShadowVeryShortPeriodTotal[2] = 0;
  ShadowVeryShortPeriodTotal[1] = 0;
  ShadowVeryShortPeriodTotal[0] = 0;
  ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
  EqualPeriodTotal[2] = 0;
  EqualPeriodTotal[1] = 0;
  EqualPeriodTotal[0] = 0;
  EqualTrailingIdx = startIdx - Equal_avgPeriod;
  i = ShadowVeryShortTrailingIdx;
  while (i < startIdx) {
    ShadowVeryShortPeriodTotal[2] = ShadowVeryShortPeriodTotal[2] + ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
    ShadowVeryShortPeriodTotal[1] = ShadowVeryShortPeriodTotal[1] + ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    ShadowVeryShortPeriodTotal[0] = ShadowVeryShortPeriodTotal[0] + ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = EqualTrailingIdx;
  while (i < startIdx) {
    EqualPeriodTotal[2] = EqualPeriodTotal[2] + ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
    EqualPeriodTotal[1] = EqualPeriodTotal[1] + ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - three consecutive and declining black candlesticks
   * - each candle must have no or very short lower shadow
   * - each candle after the first must open at or very close to the prior candle's close
   * The meaning of "very short" is specified with TA_SetCandleSettings;
   * the meaning of "very close" is specified with TA_SetCandleSettings (Equal)
   */
  outIdx = 0;
  do {
    if (((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === -1 && /* 1st black */
        ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === -1 && /* 2nd black */
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === -1 &&         /* 3rd black */
        (((inClose[i - 2] >= inOpen[i - 2]) ? inOpen[i - 2] : inClose[i - 2]) - inLow[i - 2]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
        (((inClose[i - 1] >= inOpen[i - 1]) ? inOpen[i - 1] : inClose[i - 1]) - inLow[i - 1]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (ShadowVeryShortPeriodTotal[1] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
        (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (ShadowVeryShortPeriodTotal[0] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
        inClose[i - 2] > inClose[i - 1] &&                     /* three declining */
        inClose[i - 1] > inClose[i] &&
        inOpen[i - 1] <= inClose[i - 2] + ((Equal_factor * (((Equal_avgPeriod !== 0) ? (EqualPeriodTotal[2] / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd black opens very close to 1st close */
        inOpen[i - 1] >= inClose[i - 2] - ((Equal_factor * (((Equal_avgPeriod !== 0) ? (EqualPeriodTotal[2] / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))) &&
        inOpen[i] <= inClose[i - 1] + ((Equal_factor * (((Equal_avgPeriod !== 0) ? (EqualPeriodTotal[1] / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))) && /* 3rd black opens very close to 2nd close */
        inOpen[i] >= inClose[i - 1] - ((Equal_factor * (((Equal_avgPeriod !== 0) ? (EqualPeriodTotal[1] / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))))
    {
      outInteger[outIdx++] = -100;
    } else {
      outInteger[outIdx++] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    for (totIdx = 2; totIdx >= 0; totIdx -= 1) {
      ShadowVeryShortPeriodTotal[totIdx] = ShadowVeryShortPeriodTotal[totIdx] + (((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - totIdx] - (((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inClose[i - totIdx]) : (inOpen[i - totIdx]))) + ((((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inOpen[i - totIdx]) : (inClose[i - totIdx])) - inLow[i - totIdx])) : 0.0))) - ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx - totIdx] - inOpen[ShadowVeryShortTrailingIdx - totIdx])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[ShadowVeryShortTrailingIdx - totIdx] - inLow[ShadowVeryShortTrailingIdx - totIdx]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[ShadowVeryShortTrailingIdx - totIdx] - (((inClose[ShadowVeryShortTrailingIdx - totIdx]) >= (inOpen[ShadowVeryShortTrailingIdx - totIdx])) ? (inClose[ShadowVeryShortTrailingIdx - totIdx]) : (inOpen[ShadowVeryShortTrailingIdx - totIdx]))) + ((((inClose[ShadowVeryShortTrailingIdx - totIdx]) >= (inOpen[ShadowVeryShortTrailingIdx - totIdx])) ? (inOpen[ShadowVeryShortTrailingIdx - totIdx]) : (inClose[ShadowVeryShortTrailingIdx - totIdx])) - inLow[ShadowVeryShortTrailingIdx - totIdx])) : 0.0))));
    }
    for (totIdx = 2; totIdx >= 1; totIdx -= 1) {
      EqualPeriodTotal[totIdx] = EqualPeriodTotal[totIdx] + (((Equal_rangeType === 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((Equal_rangeType === 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((Equal_rangeType === 2) ? ((inHigh[i - totIdx] - (((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inClose[i - totIdx]) : (inOpen[i - totIdx]))) + ((((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inOpen[i - totIdx]) : (inClose[i - totIdx])) - inLow[i - totIdx])) : 0.0))) - ((Equal_rangeType === 0) ? (Math.abs(inClose[EqualTrailingIdx - totIdx] - inOpen[EqualTrailingIdx - totIdx])) : ((Equal_rangeType === 1) ? (inHigh[EqualTrailingIdx - totIdx] - inLow[EqualTrailingIdx - totIdx]) : ((Equal_rangeType === 2) ? ((inHigh[EqualTrailingIdx - totIdx] - (((inClose[EqualTrailingIdx - totIdx]) >= (inOpen[EqualTrailingIdx - totIdx])) ? (inClose[EqualTrailingIdx - totIdx]) : (inOpen[EqualTrailingIdx - totIdx]))) + ((((inClose[EqualTrailingIdx - totIdx]) >= (inOpen[EqualTrailingIdx - totIdx])) ? (inOpen[EqualTrailingIdx - totIdx]) : (inClose[EqualTrailingIdx - totIdx])) - inLow[EqualTrailingIdx - totIdx])) : 0.0))));
    }
    i += 1;
    ShadowVeryShortTrailingIdx += 1;
    EqualTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/**
 * A three-candle bearish reversal pattern: three consecutive declining black
 * candles, each with a very short (or no) lower shadow, where each candle after
 * the first opens at or very near the prior candle's close. A hit signals a
 * bearish reversal (pattern is always bearish).
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlidentical3crows](https://ta-lib.org/functions/cdlidentical3crows).
 *
 * **Notes**
 * - Does not verify the preceding uptrend that the bearish reversal classically
 *   assumes.
 * - Does not require the three bodies to be equal in size; 'identical' refers only
 *   to each candle opening at or near the previous candle's close.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link cdlidentical3crowsLookback} is a
 * success with `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inOpen - Open price of each bar.
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param outInteger - -100 when the pattern is detected (always bearish), 0
 *   otherwise. Never emits +100. Must hold the values produced and never be empty;
 *   the input length always suffices.
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
 * import { cdlidentical3crows } from 'ta-lib';
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
 * const { begIdx, count } = cdlidentical3crows(
 *   0, open.length - 1,
 *   open, high, low, close,
 *   out,
 * );
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`cdl3blackcrows`](https://ta-lib.org/functions/cdl3blackcrows)
 * @see [`cdl2crows`](https://ta-lib.org/functions/cdl2crows)
 */
export function cdlidentical3crows(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDLIDENTICAL3CROWS', 'startIdx', startIdx);
  endIdx = requireInt32('CDLIDENTICAL3CROWS', 'endIdx', endIdx);
  requireIndexRange('CDLIDENTICAL3CROWS', startIdx, endIdx);
  const guardStart = clampedStart('CDLIDENTICAL3CROWS', startIdx, cdlidentical3crowsLookback(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLIDENTICAL3CROWS', 'inOpen', inOpen, guardInLen);
  requireLength('CDLIDENTICAL3CROWS', 'inHigh', inHigh, guardInLen);
  requireLength('CDLIDENTICAL3CROWS', 'inLow', inLow, guardInLen);
  requireLength('CDLIDENTICAL3CROWS', 'inClose', inClose, guardInLen);
  requireOutput('CDLIDENTICAL3CROWS', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64) || overlaps(outInteger, inHighF64) || overlaps(outInteger, inLowF64) || overlaps(outInteger, inCloseF64)) {
    throw failure('CDLIDENTICAL3CROWS', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdlidentical3crowsImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLIDENTICAL3CROWS', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live CDLIDENTICAL3CROWS stream: one value per closed bar, bit-identical to
 * {@link cdlidentical3crows} over the same series. Open it with
 * {@link cdlidentical3crowsOpen} or {@link cdlidentical3crowsOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlidentical3crows](https://ta-lib.org/functions/cdlidentical3crows).
 */
export class Cdlidentical3crowsStream {
  /** @internal */
  core: Core;
  /** @internal */
  ShadowVeryShortPeriodTotal: Float64Array = new Float64Array(3);
  /** @internal */
  EqualPeriodTotal: Float64Array = new Float64Array(3);
  /** @internal */
  lag1_inOpen: number = 0;
  /** @internal */
  lag2_inOpen: number = 0;
  /** @internal */
  lag1_inHigh: number = 0;
  /** @internal */
  lag2_inHigh: number = 0;
  /** @internal */
  lag1_inLow: number = 0;
  /** @internal */
  lag2_inLow: number = 0;
  /** @internal */
  lag1_inClose: number = 0;
  /** @internal */
  lag2_inClose: number = 0;
  /** @internal */
  ringPos_EqualTrailingIdx: number = 0;
  /** @internal */
  ringCap_EqualTrailingIdx: number = 3;
  /** @internal */
  ringLag_EqualTrailingIdx: number = 0;
  /** @internal */
  ring_EqualTrailingIdx_derived: Float64Array = new Float64Array(3);
  /** @internal */
  ringPos_ShadowVeryShortTrailingIdx: number = 0;
  /** @internal */
  ringCap_ShadowVeryShortTrailingIdx: number = 3;
  /** @internal */
  ringLag_ShadowVeryShortTrailingIdx: number = 0;
  /** @internal */
  ring_ShadowVeryShortTrailingIdx_derived: Float64Array = new Float64Array(3);
  /** @internal */
  cs_Equal_rangeType: number = 0;
  /** @internal */
  cs_Equal_avgPeriod: number = 0;
  /** @internal */
  cs_Equal_factor: number = 0;
  /** @internal */
  cs_ShadowVeryShort_rangeType: number = 0;
  /** @internal */
  cs_ShadowVeryShort_avgPeriod: number = 0;
  /** @internal */
  cs_ShadowVeryShort_factor: number = 0;
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
  static of(core: Core): Cdlidentical3crowsStream {
    return new Cdlidentical3crowsStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cdlidentical3crows} reports them over the same bars. Each accepted
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
      throw failure('CDLIDENTICAL3CROWS advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw failure('CDLIDENTICAL3CROWS update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inOpen) || !Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('CDLIDENTICAL3CROWS update', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    cdlidentical3crowsStepImpl(this, inOpen, inHigh, inLow, inClose);
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
      throw nonFiniteBar('CDLIDENTICAL3CROWS peek', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let cur_outInteger = 0;
    const Equal_rangeType = sp.cs_Equal_rangeType;
    const Equal_avgPeriod = sp.cs_Equal_avgPeriod;
    const Equal_factor = sp.cs_Equal_factor;
    const ShadowVeryShort_rangeType = sp.cs_ShadowVeryShort_rangeType;
    const ShadowVeryShort_avgPeriod = sp.cs_ShadowVeryShort_avgPeriod;
    const ShadowVeryShort_factor = sp.cs_ShadowVeryShort_factor;
    if (((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === -1 && /* 1st black */
        ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === -1 && /* 2nd black */
        ((inClose >= inOpen) ? 1 : -1) === -1 &&                 /* 3rd black */
        (((sp.lag2_inClose >= sp.lag2_inOpen) ? sp.lag2_inOpen : sp.lag2_inClose) - sp.lag2_inLow) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (sp.ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((ShadowVeryShort_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
        (((sp.lag1_inClose >= sp.lag1_inOpen) ? sp.lag1_inOpen : sp.lag1_inClose) - sp.lag1_inLow) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (sp.ShadowVeryShortPeriodTotal[1] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((ShadowVeryShort_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
        (((inClose >= inOpen) ? inOpen : inClose) - inLow) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (sp.ShadowVeryShortPeriodTotal[0] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
        sp.lag2_inClose > sp.lag1_inClose &&                     /* three declining */
        sp.lag1_inClose > inClose &&
        sp.lag1_inOpen <= sp.lag2_inClose + ((Equal_factor * (((Equal_avgPeriod !== 0) ? (sp.EqualPeriodTotal[2] / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Equal_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Equal_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd black opens very close to 1st close */
        sp.lag1_inOpen >= sp.lag2_inClose - ((Equal_factor * (((Equal_avgPeriod !== 0) ? (sp.EqualPeriodTotal[2] / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Equal_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Equal_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))) &&
        inOpen <= sp.lag1_inClose + ((Equal_factor * (((Equal_avgPeriod !== 0) ? (sp.EqualPeriodTotal[1] / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))) && /* 3rd black opens very close to 2nd close */
        inOpen >= sp.lag1_inClose - ((Equal_factor * (((Equal_avgPeriod !== 0) ? (sp.EqualPeriodTotal[1] / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))))
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
  clone(): Cdlidentical3crowsStream {
    const c = new Cdlidentical3crowsStream(this.core);
    c.ShadowVeryShortPeriodTotal = this.ShadowVeryShortPeriodTotal.slice();
    c.EqualPeriodTotal = this.EqualPeriodTotal.slice();
    c.lag1_inOpen = this.lag1_inOpen;
    c.lag2_inOpen = this.lag2_inOpen;
    c.lag1_inHigh = this.lag1_inHigh;
    c.lag2_inHigh = this.lag2_inHigh;
    c.lag1_inLow = this.lag1_inLow;
    c.lag2_inLow = this.lag2_inLow;
    c.lag1_inClose = this.lag1_inClose;
    c.lag2_inClose = this.lag2_inClose;
    c.ringPos_EqualTrailingIdx = this.ringPos_EqualTrailingIdx;
    c.ringCap_EqualTrailingIdx = this.ringCap_EqualTrailingIdx;
    c.ringLag_EqualTrailingIdx = this.ringLag_EqualTrailingIdx;
    c.ring_EqualTrailingIdx_derived = this.ring_EqualTrailingIdx_derived.slice();
    c.ringPos_ShadowVeryShortTrailingIdx = this.ringPos_ShadowVeryShortTrailingIdx;
    c.ringCap_ShadowVeryShortTrailingIdx = this.ringCap_ShadowVeryShortTrailingIdx;
    c.ringLag_ShadowVeryShortTrailingIdx = this.ringLag_ShadowVeryShortTrailingIdx;
    c.ring_ShadowVeryShortTrailingIdx_derived = this.ring_ShadowVeryShortTrailingIdx_derived.slice();
    c.cs_Equal_rangeType = this.cs_Equal_rangeType;
    c.cs_Equal_avgPeriod = this.cs_Equal_avgPeriod;
    c.cs_Equal_factor = this.cs_Equal_factor;
    c.cs_ShadowVeryShort_rangeType = this.cs_ShadowVeryShort_rangeType;
    c.cs_ShadowVeryShort_avgPeriod = this.cs_ShadowVeryShort_avgPeriod;
    c.cs_ShadowVeryShort_factor = this.cs_ShadowVeryShort_factor;
    c.cur_outInteger = this.cur_outInteger;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cdlidentical3crowsStepImpl(sp: Cdlidentical3crowsStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  let totIdx = 0;
  const Equal_rangeType = sp.cs_Equal_rangeType;
  const Equal_avgPeriod = sp.cs_Equal_avgPeriod;
  const Equal_factor = sp.cs_Equal_factor;
  const ShadowVeryShort_rangeType = sp.cs_ShadowVeryShort_rangeType;
  const ShadowVeryShort_avgPeriod = sp.cs_ShadowVeryShort_avgPeriod;
  const ShadowVeryShort_factor = sp.cs_ShadowVeryShort_factor;
  sp.ring_EqualTrailingIdx_derived[sp.ringPos_EqualTrailingIdx] = ((Equal_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((Equal_rangeType === 1) ? (inHigh - inLow) : ((Equal_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  sp.ring_ShadowVeryShortTrailingIdx_derived[sp.ringPos_ShadowVeryShortTrailingIdx] = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  if (((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === -1 && /* 1st black */
      ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === -1 && /* 2nd black */
      ((inClose >= inOpen) ? 1 : -1) === -1 &&                 /* 3rd black */
      (((sp.lag2_inClose >= sp.lag2_inOpen) ? sp.lag2_inOpen : sp.lag2_inClose) - sp.lag2_inLow) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (sp.ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((ShadowVeryShort_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
      (((sp.lag1_inClose >= sp.lag1_inOpen) ? sp.lag1_inOpen : sp.lag1_inClose) - sp.lag1_inLow) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (sp.ShadowVeryShortPeriodTotal[1] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((ShadowVeryShort_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
      (((inClose >= inOpen) ? inOpen : inClose) - inLow) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (sp.ShadowVeryShortPeriodTotal[0] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
      sp.lag2_inClose > sp.lag1_inClose &&                     /* three declining */
      sp.lag1_inClose > inClose &&
      sp.lag1_inOpen <= sp.lag2_inClose + ((Equal_factor * (((Equal_avgPeriod !== 0) ? (sp.EqualPeriodTotal[2] / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Equal_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Equal_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd black opens very close to 1st close */
      sp.lag1_inOpen >= sp.lag2_inClose - ((Equal_factor * (((Equal_avgPeriod !== 0) ? (sp.EqualPeriodTotal[2] / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Equal_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Equal_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))) &&
      inOpen <= sp.lag1_inClose + ((Equal_factor * (((Equal_avgPeriod !== 0) ? (sp.EqualPeriodTotal[1] / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))) && /* 3rd black opens very close to 2nd close */
      inOpen >= sp.lag1_inClose - ((Equal_factor * (((Equal_avgPeriod !== 0) ? (sp.EqualPeriodTotal[1] / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))))
  {
    sp.cur_outInteger = -100;
  } else {
    sp.cur_outInteger = 0;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  for (totIdx = 2; totIdx >= 0; totIdx -= 1) {
    sp.ShadowVeryShortPeriodTotal[totIdx] = sp.ShadowVeryShortPeriodTotal[totIdx] + (sp.ring_ShadowVeryShortTrailingIdx_derived[(sp.ringPos_ShadowVeryShortTrailingIdx + sp.ringCap_ShadowVeryShortTrailingIdx - totIdx >= sp.ringCap_ShadowVeryShortTrailingIdx) ? sp.ringPos_ShadowVeryShortTrailingIdx + sp.ringCap_ShadowVeryShortTrailingIdx - totIdx - sp.ringCap_ShadowVeryShortTrailingIdx : sp.ringPos_ShadowVeryShortTrailingIdx + sp.ringCap_ShadowVeryShortTrailingIdx - totIdx] - sp.ring_ShadowVeryShortTrailingIdx_derived[(((sp.ringPos_ShadowVeryShortTrailingIdx + sp.ringCap_ShadowVeryShortTrailingIdx - sp.ringLag_ShadowVeryShortTrailingIdx - totIdx) % sp.ringCap_ShadowVeryShortTrailingIdx) | 0)]);
  }
  for (totIdx = 2; totIdx >= 1; totIdx -= 1) {
    sp.EqualPeriodTotal[totIdx] = sp.EqualPeriodTotal[totIdx] + (sp.ring_EqualTrailingIdx_derived[(sp.ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - totIdx >= sp.ringCap_EqualTrailingIdx) ? sp.ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - totIdx - sp.ringCap_EqualTrailingIdx : sp.ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - totIdx] - sp.ring_EqualTrailingIdx_derived[(((sp.ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - sp.ringLag_EqualTrailingIdx - totIdx) % sp.ringCap_EqualTrailingIdx) | 0)]);
  }
  sp.lag2_inOpen = sp.lag1_inOpen;
  sp.lag1_inOpen = inOpen;
  sp.lag2_inHigh = sp.lag1_inHigh;
  sp.lag1_inHigh = inHigh;
  sp.lag2_inLow = sp.lag1_inLow;
  sp.lag1_inLow = inLow;
  sp.lag2_inClose = sp.lag1_inClose;
  sp.lag1_inClose = inClose;
  sp.ringPos_EqualTrailingIdx = sp.ringPos_EqualTrailingIdx + 1;
  if (sp.ringPos_EqualTrailingIdx >= sp.ringCap_EqualTrailingIdx) {
    sp.ringPos_EqualTrailingIdx = 0;
  }
  sp.ringPos_ShadowVeryShortTrailingIdx = sp.ringPos_ShadowVeryShortTrailingIdx + 1;
  if (sp.ringPos_ShadowVeryShortTrailingIdx >= sp.ringCap_ShadowVeryShortTrailingIdx) {
    sp.ringPos_ShadowVeryShortTrailingIdx = 0;
  }
}

function cdlidentical3crowsOpenImpl(
  sp: Cdlidentical3crowsStream,
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
  const ShadowVeryShortPeriodTotal = new Float64Array(3);
  const EqualPeriodTotal = new Float64Array(3);
  let i = 0;
  let outIdx = 0;
  let totIdx = 0;
  let ShadowVeryShortTrailingIdx = 0;
  let EqualTrailingIdx = 0;
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
  const Equal_rangeType: number = core.candleSettings[CandleSettingType.EQUAL].rangeType;
  const Equal_avgPeriod = core.candleSettings[CandleSettingType.EQUAL].avgPeriod;
  const Equal_factor = core.candleSettings[CandleSettingType.EQUAL].factor;
  const ShadowVeryShort_rangeType: number = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].rangeType;
  const ShadowVeryShort_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].avgPeriod;
  const ShadowVeryShort_factor = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].factor;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlidentical3crowsLookback(core);
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
  ShadowVeryShortPeriodTotal[2] = 0;
  ShadowVeryShortPeriodTotal[1] = 0;
  ShadowVeryShortPeriodTotal[0] = 0;
  ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
  EqualPeriodTotal[2] = 0;
  EqualPeriodTotal[1] = 0;
  EqualPeriodTotal[0] = 0;
  EqualTrailingIdx = startIdx - Equal_avgPeriod;
  i = ShadowVeryShortTrailingIdx;
  while (i < startIdx) {
    ShadowVeryShortPeriodTotal[2] = ShadowVeryShortPeriodTotal[2] + ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
    ShadowVeryShortPeriodTotal[1] = ShadowVeryShortPeriodTotal[1] + ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    ShadowVeryShortPeriodTotal[0] = ShadowVeryShortPeriodTotal[0] + ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = EqualTrailingIdx;
  while (i < startIdx) {
    EqualPeriodTotal[2] = EqualPeriodTotal[2] + ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
    EqualPeriodTotal[1] = EqualPeriodTotal[1] + ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - three consecutive and declining black candlesticks
   * - each candle must have no or very short lower shadow
   * - each candle after the first must open at or very close to the prior candle's close
   * The meaning of "very short" is specified with TA_SetCandleSettings;
   * the meaning of "very close" is specified with TA_SetCandleSettings (Equal)
   */
  outIdx = 0;
  do {
    if (((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === -1 && /* 1st black */
        ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === -1 && /* 2nd black */
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === -1 &&         /* 3rd black */
        (((inClose[i - 2] >= inOpen[i - 2]) ? inOpen[i - 2] : inClose[i - 2]) - inLow[i - 2]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
        (((inClose[i - 1] >= inOpen[i - 1]) ? inOpen[i - 1] : inClose[i - 1]) - inLow[i - 1]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (ShadowVeryShortPeriodTotal[1] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
        (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (ShadowVeryShortPeriodTotal[0] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
        inClose[i - 2] > inClose[i - 1] &&                     /* three declining */
        inClose[i - 1] > inClose[i] &&
        inOpen[i - 1] <= inClose[i - 2] + ((Equal_factor * (((Equal_avgPeriod !== 0) ? (EqualPeriodTotal[2] / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd black opens very close to 1st close */
        inOpen[i - 1] >= inClose[i - 2] - ((Equal_factor * (((Equal_avgPeriod !== 0) ? (EqualPeriodTotal[2] / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))) &&
        inOpen[i] <= inClose[i - 1] + ((Equal_factor * (((Equal_avgPeriod !== 0) ? (EqualPeriodTotal[1] / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))) && /* 3rd black opens very close to 2nd close */
        inOpen[i] >= inClose[i - 1] - ((Equal_factor * (((Equal_avgPeriod !== 0) ? (EqualPeriodTotal[1] / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))))
    {
      outInteger[((outIdx++ * outStride) | 0)] = -100;
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    for (totIdx = 2; totIdx >= 0; totIdx -= 1) {
      ShadowVeryShortPeriodTotal[totIdx] = ShadowVeryShortPeriodTotal[totIdx] + (((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - totIdx] - (((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inClose[i - totIdx]) : (inOpen[i - totIdx]))) + ((((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inOpen[i - totIdx]) : (inClose[i - totIdx])) - inLow[i - totIdx])) : 0.0))) - ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx - totIdx] - inOpen[ShadowVeryShortTrailingIdx - totIdx])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[ShadowVeryShortTrailingIdx - totIdx] - inLow[ShadowVeryShortTrailingIdx - totIdx]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[ShadowVeryShortTrailingIdx - totIdx] - (((inClose[ShadowVeryShortTrailingIdx - totIdx]) >= (inOpen[ShadowVeryShortTrailingIdx - totIdx])) ? (inClose[ShadowVeryShortTrailingIdx - totIdx]) : (inOpen[ShadowVeryShortTrailingIdx - totIdx]))) + ((((inClose[ShadowVeryShortTrailingIdx - totIdx]) >= (inOpen[ShadowVeryShortTrailingIdx - totIdx])) ? (inOpen[ShadowVeryShortTrailingIdx - totIdx]) : (inClose[ShadowVeryShortTrailingIdx - totIdx])) - inLow[ShadowVeryShortTrailingIdx - totIdx])) : 0.0))));
    }
    for (totIdx = 2; totIdx >= 1; totIdx -= 1) {
      EqualPeriodTotal[totIdx] = EqualPeriodTotal[totIdx] + (((Equal_rangeType === 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((Equal_rangeType === 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((Equal_rangeType === 2) ? ((inHigh[i - totIdx] - (((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inClose[i - totIdx]) : (inOpen[i - totIdx]))) + ((((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inOpen[i - totIdx]) : (inClose[i - totIdx])) - inLow[i - totIdx])) : 0.0))) - ((Equal_rangeType === 0) ? (Math.abs(inClose[EqualTrailingIdx - totIdx] - inOpen[EqualTrailingIdx - totIdx])) : ((Equal_rangeType === 1) ? (inHigh[EqualTrailingIdx - totIdx] - inLow[EqualTrailingIdx - totIdx]) : ((Equal_rangeType === 2) ? ((inHigh[EqualTrailingIdx - totIdx] - (((inClose[EqualTrailingIdx - totIdx]) >= (inOpen[EqualTrailingIdx - totIdx])) ? (inClose[EqualTrailingIdx - totIdx]) : (inOpen[EqualTrailingIdx - totIdx]))) + ((((inClose[EqualTrailingIdx - totIdx]) >= (inOpen[EqualTrailingIdx - totIdx])) ? (inOpen[EqualTrailingIdx - totIdx]) : (inClose[EqualTrailingIdx - totIdx])) - inLow[EqualTrailingIdx - totIdx])) : 0.0))));
    }
    i += 1;
    ShadowVeryShortTrailingIdx += 1;
    EqualTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const capLag_EqualTrailingIdx = i - EqualTrailingIdx;
  const cap_EqualTrailingIdx = capLag_EqualTrailingIdx + 3;
  if (capLag_EqualTrailingIdx < 0 || cap_EqualTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_EqualTrailingIdx = cap_EqualTrailingIdx > 0 ? cap_EqualTrailingIdx : 1;
  const capRing_EqualTrailingIdx_derived = new Float64Array(allocN_EqualTrailingIdx);
  for (let fillJ = historyLen - cap_EqualTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_EqualTrailingIdx_derived[fillJ % cap_EqualTrailingIdx] = ((Equal_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((Equal_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((Equal_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  const capLag_ShadowVeryShortTrailingIdx = i - ShadowVeryShortTrailingIdx;
  const cap_ShadowVeryShortTrailingIdx = capLag_ShadowVeryShortTrailingIdx + 3;
  if (capLag_ShadowVeryShortTrailingIdx < 0 || cap_ShadowVeryShortTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_ShadowVeryShortTrailingIdx = cap_ShadowVeryShortTrailingIdx > 0 ? cap_ShadowVeryShortTrailingIdx : 1;
  const capRing_ShadowVeryShortTrailingIdx_derived = new Float64Array(allocN_ShadowVeryShortTrailingIdx);
  for (let fillJ = historyLen - cap_ShadowVeryShortTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_ShadowVeryShortTrailingIdx_derived[fillJ % cap_ShadowVeryShortTrailingIdx] = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  sp.ShadowVeryShortPeriodTotal = ShadowVeryShortPeriodTotal;
  sp.EqualPeriodTotal = EqualPeriodTotal;
  sp.lag1_inOpen = inOpen[historyLen - 1];
  sp.lag2_inOpen = inOpen[historyLen - 2];
  sp.lag1_inHigh = inHigh[historyLen - 1];
  sp.lag2_inHigh = inHigh[historyLen - 2];
  sp.lag1_inLow = inLow[historyLen - 1];
  sp.lag2_inLow = inLow[historyLen - 2];
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.lag2_inClose = inClose[historyLen - 2];
  sp.ringPos_EqualTrailingIdx = historyLen % cap_EqualTrailingIdx;
  sp.ringCap_EqualTrailingIdx = cap_EqualTrailingIdx;
  sp.ringLag_EqualTrailingIdx = capLag_EqualTrailingIdx;
  sp.ring_EqualTrailingIdx_derived = capRing_EqualTrailingIdx_derived;
  sp.ringPos_ShadowVeryShortTrailingIdx = historyLen % cap_ShadowVeryShortTrailingIdx;
  sp.ringCap_ShadowVeryShortTrailingIdx = cap_ShadowVeryShortTrailingIdx;
  sp.ringLag_ShadowVeryShortTrailingIdx = capLag_ShadowVeryShortTrailingIdx;
  sp.ring_ShadowVeryShortTrailingIdx_derived = capRing_ShadowVeryShortTrailingIdx_derived;
  sp.cs_Equal_rangeType = Equal_rangeType;
  sp.cs_Equal_avgPeriod = Equal_avgPeriod;
  sp.cs_Equal_factor = Equal_factor;
  sp.cs_ShadowVeryShort_rangeType = ShadowVeryShort_rangeType;
  sp.cs_ShadowVeryShort_avgPeriod = ShadowVeryShort_avgPeriod;
  sp.cs_ShadowVeryShort_factor = ShadowVeryShort_factor;
  sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlidentical3crowsOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): Cdlidentical3crowsStream {
  const sp = Cdlidentical3crowsStream.of(core);
  const retCode = cdlidentical3crowsOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLIDENTICAL3CROWS openAndFill', inOpen.length, startIdx, cdlidentical3crowsLookback(core));
  }
  throw streamFailure('CDLIDENTICAL3CROWS openAndFill', retCode);
}

/** @internal */
export function cdlidentical3crowsOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): Cdlidentical3crowsStream {
  const sp = Cdlidentical3crowsStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdlidentical3crowsOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLIDENTICAL3CROWS open', inOpen.length, startIdx, cdlidentical3crowsLookback(core));
  }
  throw streamFailure('CDLIDENTICAL3CROWS open', retCode);
}

/**
 * Opens a live CDLIDENTICAL3CROWS stream over the warm-up history: `value()`
 * starts at the last history bar, bit-identical to {@link cdlidentical3crows}
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
 *   than `cdlidentical3crowsLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX`
 *   for an empty one, and `TA_BAD_PARAM` for histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdlidentical3crowsOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdlidentical3crowsOpen(open, high, low, close);
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdlidentical3crowsOpen(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): Cdlidentical3crowsStream {
  requireArgument('CDLIDENTICAL3CROWS open', 'inOpen', inOpen);
  requireHistory('CDLIDENTICAL3CROWS open', inOpen.length);
  requireArgument('CDLIDENTICAL3CROWS open', 'inHigh', inHigh);
  requireArgument('CDLIDENTICAL3CROWS open', 'inLow', inLow);
  requireArgument('CDLIDENTICAL3CROWS open', 'inClose', inClose);
  requireHistoryLength('CDLIDENTICAL3CROWS open', 'inHigh', inHigh.length, inOpen.length);
  requireHistoryLength('CDLIDENTICAL3CROWS open', 'inLow', inLow.length, inOpen.length);
  requireHistoryLength('CDLIDENTICAL3CROWS open', 'inClose', inClose.length, inOpen.length);
  return cdlidentical3crowsOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/**
 * {@link cdlidentical3crowsOpen} that also fills the outputs over the whole
 * history in the same pass, bit-identically to {@link cdlidentical3crows}. The
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
 * @param outInteger - -100 when the pattern is detected (always bearish), 0
 *   otherwise. Never emits +100. Must hold `historyLen - lookback` values and share
 *   no memory with an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlidentical3crowsLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX`
 *   for an empty one, and `TA_BAD_PARAM` for histories of different lengths or an
 *   output too short or starting where another output or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdlidentical3crowsOpenAndFill } from 'ta-lib';
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
 * const stream = cdlidentical3crowsOpenAndFill(open, high, low, close, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function cdlidentical3crowsOpenAndFill(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): Cdlidentical3crowsStream {
  requireArgument('CDLIDENTICAL3CROWS openAndFill', 'inOpen', inOpen);
  requireHistory('CDLIDENTICAL3CROWS openAndFill', inOpen.length);
  requireArgument('CDLIDENTICAL3CROWS openAndFill', 'inHigh', inHigh);
  requireArgument('CDLIDENTICAL3CROWS openAndFill', 'inLow', inLow);
  requireArgument('CDLIDENTICAL3CROWS openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('CDLIDENTICAL3CROWS openAndFill', inOpen.length, cdlidentical3crowsLookback(core));
  requireHistoryLength('CDLIDENTICAL3CROWS openAndFill', 'inHigh', inHigh.length, inOpen.length);
  requireHistoryLength('CDLIDENTICAL3CROWS openAndFill', 'inLow', inLow.length, inOpen.length);
  requireHistoryLength('CDLIDENTICAL3CROWS openAndFill', 'inClose', inClose.length, inOpen.length);
  requireOutput('CDLIDENTICAL3CROWS openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) || (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) || (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) || (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset)) {
    throw streamFailure('CDLIDENTICAL3CROWS openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outInteger, inOpenF64) || overlaps(outInteger, inHighF64) || overlaps(outInteger, inLowF64) || overlaps(outInteger, inCloseF64))) {
    throw streamFailure('CDLIDENTICAL3CROWS openAndFill', RetCode.BAD_PARAM);
  }
  return cdlidentical3crowsOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}
