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
 *  120404 AC     Creation
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 *  103126 MF,CC  Nest the cheap, selective pre-check conjuncts in an outer if
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { CandleSettingType, Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link cdladvanceblock} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function cdladvanceblockLookback(core: Core = Core.DEFAULT): number {
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  const Far_avgPeriod = core.candleSettings[CandleSettingType.FAR].avgPeriod;
  const Near_avgPeriod = core.candleSettings[CandleSettingType.NEAR].avgPeriod;
  const ShadowLong_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_LONG].avgPeriod;
  const ShadowShort_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_SHORT].avgPeriod;
  return (((ShadowLong_avgPeriod > ShadowShort_avgPeriod ? ShadowLong_avgPeriod : ShadowShort_avgPeriod) > (Far_avgPeriod > Near_avgPeriod ? Far_avgPeriod : Near_avgPeriod) ? (ShadowLong_avgPeriod > ShadowShort_avgPeriod ? ShadowLong_avgPeriod : ShadowShort_avgPeriod) : (Far_avgPeriod > Near_avgPeriod ? Far_avgPeriod : Near_avgPeriod)) > BodyLong_avgPeriod ? ((ShadowLong_avgPeriod > ShadowShort_avgPeriod ? ShadowLong_avgPeriod : ShadowShort_avgPeriod) > (Far_avgPeriod > Near_avgPeriod ? Far_avgPeriod : Near_avgPeriod) ? (ShadowLong_avgPeriod > ShadowShort_avgPeriod ? ShadowLong_avgPeriod : ShadowShort_avgPeriod) : (Far_avgPeriod > Near_avgPeriod ? Far_avgPeriod : Near_avgPeriod)) : BodyLong_avgPeriod) + 2;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdladvanceblock}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function cdladvanceblockDisplayShift(outputIdx: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdladvanceblockImpl(
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
  const ShadowShortPeriodTotal = new Float64Array(3);
  const ShadowLongPeriodTotal = new Float64Array(2);
  const NearPeriodTotal = new Float64Array(3);
  const FarPeriodTotal = new Float64Array(3);
  let BodyLongPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let totIdx = 0;
  let BodyLongTrailingIdx = 0;
  let ShadowShortTrailingIdx = 0;
  let ShadowLongTrailingIdx = 0;
  let NearTrailingIdx = 0;
  let FarTrailingIdx = 0;
  let lookbackTotal = 0;
  const BodyLong_rangeType: number = core.candleSettings[CandleSettingType.BODY_LONG].rangeType;
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  const BodyLong_factor = core.candleSettings[CandleSettingType.BODY_LONG].factor;
  const Far_rangeType: number = core.candleSettings[CandleSettingType.FAR].rangeType;
  const Far_avgPeriod = core.candleSettings[CandleSettingType.FAR].avgPeriod;
  const Far_factor = core.candleSettings[CandleSettingType.FAR].factor;
  const Near_rangeType: number = core.candleSettings[CandleSettingType.NEAR].rangeType;
  const Near_avgPeriod = core.candleSettings[CandleSettingType.NEAR].avgPeriod;
  const Near_factor = core.candleSettings[CandleSettingType.NEAR].factor;
  const ShadowLong_rangeType: number = core.candleSettings[CandleSettingType.SHADOW_LONG].rangeType;
  const ShadowLong_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_LONG].avgPeriod;
  const ShadowLong_factor = core.candleSettings[CandleSettingType.SHADOW_LONG].factor;
  const ShadowShort_rangeType: number = core.candleSettings[CandleSettingType.SHADOW_SHORT].rangeType;
  const ShadowShort_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_SHORT].avgPeriod;
  const ShadowShort_factor = core.candleSettings[CandleSettingType.SHADOW_SHORT].factor;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdladvanceblockLookback(core);
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
  ShadowShortPeriodTotal[2] = 0;
  ShadowShortPeriodTotal[1] = 0;
  ShadowShortPeriodTotal[0] = 0;
  ShadowShortTrailingIdx = startIdx - ShadowShort_avgPeriod;
  ShadowLongPeriodTotal[1] = 0;
  ShadowLongPeriodTotal[0] = 0;
  ShadowLongTrailingIdx = startIdx - ShadowLong_avgPeriod;
  NearPeriodTotal[2] = 0;
  NearPeriodTotal[1] = 0;
  NearPeriodTotal[0] = 0;
  NearTrailingIdx = startIdx - Near_avgPeriod;
  FarPeriodTotal[2] = 0;
  FarPeriodTotal[1] = 0;
  FarPeriodTotal[0] = 0;
  FarTrailingIdx = startIdx - Far_avgPeriod;
  BodyLongPeriodTotal = 0;
  BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
  i = ShadowShortTrailingIdx;
  while (i < startIdx) {
    ShadowShortPeriodTotal[2] = ShadowShortPeriodTotal[2] + ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
    ShadowShortPeriodTotal[1] = ShadowShortPeriodTotal[1] + ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    ShadowShortPeriodTotal[0] = ShadowShortPeriodTotal[0] + ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = ShadowLongTrailingIdx;
  while (i < startIdx) {
    ShadowLongPeriodTotal[1] = ShadowLongPeriodTotal[1] + ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowLong_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    ShadowLongPeriodTotal[0] = ShadowLongPeriodTotal[0] + ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = NearTrailingIdx;
  while (i < startIdx) {
    NearPeriodTotal[2] = NearPeriodTotal[2] + ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
    NearPeriodTotal[1] = NearPeriodTotal[1] + ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    i += 1;
  }
  i = FarTrailingIdx;
  while (i < startIdx) {
    FarPeriodTotal[2] = FarPeriodTotal[2] + ((Far_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Far_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Far_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
    FarPeriodTotal[1] = FarPeriodTotal[1] + ((Far_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Far_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Far_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    i += 1;
  }
  i = BodyLongTrailingIdx;
  while (i < startIdx) {
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - three white candlesticks with consecutively higher closes
   * - each candle opens within or near the previous white real body
   * - first candle: long white with no or very short upper shadow (a short shadow is accepted too for more flexibility)
   * - second and third candles, or only third candle, show signs of weakening: progressively smaller white real bodies
   * and/or relatively long upper shadows; see below for specific conditions
   * The meanings of "long body", "short shadow", "far" and "near" are specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === 1 && /* 1st white */
        ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === 1 && /* 2nd white */
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === 1 &&         /* 3rd white */
        inClose[i] > inClose[i - 1] &&
        inClose[i - 1] > inClose[i - 2] &&                    /* consecutive higher closes */
        inOpen[i - 1] > inOpen[i - 2] &&                      /* 2nd opens within/near 1st real body */
        inOpen[i - 1] <= inClose[i - 2] + ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
        inOpen[i] > inOpen[i - 1] &&                          /* 3rd opens within/near 2nd real body */
        inOpen[i] <= inClose[i - 1] + ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
        Math.abs(inClose[i - 2] - inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st: long real body */
        (inHigh[i - 2] - ((inClose[i - 2] >= inOpen[i - 2]) ? inClose[i - 2] : inOpen[i - 2])) < ((ShadowShort_factor * (((ShadowShort_avgPeriod !== 0) ? (ShadowShortPeriodTotal[2] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((ShadowShort_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st: short upper shadow */
        (Math.abs(inClose[i - 1] - inOpen[i - 1]) < Math.abs(inClose[i - 2] - inOpen[i - 2]) - ((Far_factor * (((Far_avgPeriod !== 0) ? (FarPeriodTotal[2] / Far_avgPeriod) : ((Far_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Far_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Far_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Far_rangeType === 2) ? 2.0 : 1.0)))) && /* ( 2 far smaller than 1 && 3 not longer than 2 ) advance blocked with the 2nd, 3rd must not carry on the advance */
          Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) + ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) ||
         Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) - ((Far_factor * (((Far_avgPeriod !== 0) ? (FarPeriodTotal[1] / Far_avgPeriod) : ((Far_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Far_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Far_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Far_rangeType === 2) ? 2.0 : 1.0)))) || /* 3 far smaller than 2 advance blocked with the 3rd */
         Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) && /* ( 3 smaller than 2 && 2 smaller than 1 && (3 or 2 not short upper shadow) ) advance blocked with progressively smaller real bodies and some upper shadows */
          Math.abs(inClose[i - 1] - inOpen[i - 1]) < Math.abs(inClose[i - 2] - inOpen[i - 2]) &&
          ((inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowShort_factor * (((ShadowShort_avgPeriod !== 0) ? (ShadowShortPeriodTotal[0] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowShort_rangeType === 2) ? 2.0 : 1.0)))) ||
           (inHigh[i - 1] - ((inClose[i - 1] >= inOpen[i - 1]) ? inClose[i - 1] : inOpen[i - 1])) > ((ShadowShort_factor * (((ShadowShort_avgPeriod !== 0) ? (ShadowShortPeriodTotal[1] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((ShadowShort_rangeType === 2) ? 2.0 : 1.0))))) ||
         Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) && /* ( 3 smaller than 2 && 3 long upper shadow ) advance blocked with 3rd candle's long upper shadow and smaller body */
          (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowLong_factor * (((ShadowLong_avgPeriod !== 0) ? (ShadowLongPeriodTotal[0] / ShadowLong_avgPeriod) : ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowLong_rangeType === 2) ? 2.0 : 1.0))))))
    {
      outInteger[outIdx++] = -100;
    } else {
      outInteger[outIdx++] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    for (totIdx = 2; totIdx >= 0; totIdx -= 1) {
      ShadowShortPeriodTotal[totIdx] = ShadowShortPeriodTotal[totIdx] + (((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((ShadowShort_rangeType === 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i - totIdx] - (((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inClose[i - totIdx]) : (inOpen[i - totIdx]))) + ((((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inOpen[i - totIdx]) : (inClose[i - totIdx])) - inLow[i - totIdx])) : 0.0))) - ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[ShadowShortTrailingIdx - totIdx] - inOpen[ShadowShortTrailingIdx - totIdx])) : ((ShadowShort_rangeType === 1) ? (inHigh[ShadowShortTrailingIdx - totIdx] - inLow[ShadowShortTrailingIdx - totIdx]) : ((ShadowShort_rangeType === 2) ? ((inHigh[ShadowShortTrailingIdx - totIdx] - (((inClose[ShadowShortTrailingIdx - totIdx]) >= (inOpen[ShadowShortTrailingIdx - totIdx])) ? (inClose[ShadowShortTrailingIdx - totIdx]) : (inOpen[ShadowShortTrailingIdx - totIdx]))) + ((((inClose[ShadowShortTrailingIdx - totIdx]) >= (inOpen[ShadowShortTrailingIdx - totIdx])) ? (inOpen[ShadowShortTrailingIdx - totIdx]) : (inClose[ShadowShortTrailingIdx - totIdx])) - inLow[ShadowShortTrailingIdx - totIdx])) : 0.0))));
    }
    for (totIdx = 1; totIdx >= 0; totIdx -= 1) {
      ShadowLongPeriodTotal[totIdx] = ShadowLongPeriodTotal[totIdx] + (((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((ShadowLong_rangeType === 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i - totIdx] - (((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inClose[i - totIdx]) : (inOpen[i - totIdx]))) + ((((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inOpen[i - totIdx]) : (inClose[i - totIdx])) - inLow[i - totIdx])) : 0.0))) - ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[ShadowLongTrailingIdx - totIdx] - inOpen[ShadowLongTrailingIdx - totIdx])) : ((ShadowLong_rangeType === 1) ? (inHigh[ShadowLongTrailingIdx - totIdx] - inLow[ShadowLongTrailingIdx - totIdx]) : ((ShadowLong_rangeType === 2) ? ((inHigh[ShadowLongTrailingIdx - totIdx] - (((inClose[ShadowLongTrailingIdx - totIdx]) >= (inOpen[ShadowLongTrailingIdx - totIdx])) ? (inClose[ShadowLongTrailingIdx - totIdx]) : (inOpen[ShadowLongTrailingIdx - totIdx]))) + ((((inClose[ShadowLongTrailingIdx - totIdx]) >= (inOpen[ShadowLongTrailingIdx - totIdx])) ? (inOpen[ShadowLongTrailingIdx - totIdx]) : (inClose[ShadowLongTrailingIdx - totIdx])) - inLow[ShadowLongTrailingIdx - totIdx])) : 0.0))));
    }
    for (totIdx = 2; totIdx >= 1; totIdx -= 1) {
      FarPeriodTotal[totIdx] = FarPeriodTotal[totIdx] + (((Far_rangeType === 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((Far_rangeType === 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((Far_rangeType === 2) ? ((inHigh[i - totIdx] - (((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inClose[i - totIdx]) : (inOpen[i - totIdx]))) + ((((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inOpen[i - totIdx]) : (inClose[i - totIdx])) - inLow[i - totIdx])) : 0.0))) - ((Far_rangeType === 0) ? (Math.abs(inClose[FarTrailingIdx - totIdx] - inOpen[FarTrailingIdx - totIdx])) : ((Far_rangeType === 1) ? (inHigh[FarTrailingIdx - totIdx] - inLow[FarTrailingIdx - totIdx]) : ((Far_rangeType === 2) ? ((inHigh[FarTrailingIdx - totIdx] - (((inClose[FarTrailingIdx - totIdx]) >= (inOpen[FarTrailingIdx - totIdx])) ? (inClose[FarTrailingIdx - totIdx]) : (inOpen[FarTrailingIdx - totIdx]))) + ((((inClose[FarTrailingIdx - totIdx]) >= (inOpen[FarTrailingIdx - totIdx])) ? (inOpen[FarTrailingIdx - totIdx]) : (inClose[FarTrailingIdx - totIdx])) - inLow[FarTrailingIdx - totIdx])) : 0.0))));
      NearPeriodTotal[totIdx] = NearPeriodTotal[totIdx] + (((Near_rangeType === 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((Near_rangeType === 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((Near_rangeType === 2) ? ((inHigh[i - totIdx] - (((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inClose[i - totIdx]) : (inOpen[i - totIdx]))) + ((((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inOpen[i - totIdx]) : (inClose[i - totIdx])) - inLow[i - totIdx])) : 0.0))) - ((Near_rangeType === 0) ? (Math.abs(inClose[NearTrailingIdx - totIdx] - inOpen[NearTrailingIdx - totIdx])) : ((Near_rangeType === 1) ? (inHigh[NearTrailingIdx - totIdx] - inLow[NearTrailingIdx - totIdx]) : ((Near_rangeType === 2) ? ((inHigh[NearTrailingIdx - totIdx] - (((inClose[NearTrailingIdx - totIdx]) >= (inOpen[NearTrailingIdx - totIdx])) ? (inClose[NearTrailingIdx - totIdx]) : (inOpen[NearTrailingIdx - totIdx]))) + ((((inClose[NearTrailingIdx - totIdx]) >= (inOpen[NearTrailingIdx - totIdx])) ? (inOpen[NearTrailingIdx - totIdx]) : (inClose[NearTrailingIdx - totIdx])) - inLow[NearTrailingIdx - totIdx])) : 0.0))));
    }
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0))) - ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 2] - inOpen[BodyLongTrailingIdx - 2])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyLongTrailingIdx - 2] - inLow[BodyLongTrailingIdx - 2]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyLongTrailingIdx - 2] - (((inClose[BodyLongTrailingIdx - 2]) >= (inOpen[BodyLongTrailingIdx - 2])) ? (inClose[BodyLongTrailingIdx - 2]) : (inOpen[BodyLongTrailingIdx - 2]))) + ((((inClose[BodyLongTrailingIdx - 2]) >= (inOpen[BodyLongTrailingIdx - 2])) ? (inOpen[BodyLongTrailingIdx - 2]) : (inClose[BodyLongTrailingIdx - 2])) - inLow[BodyLongTrailingIdx - 2])) : 0.0)));
    i += 1;
    ShadowShortTrailingIdx += 1;
    ShadowLongTrailingIdx += 1;
    NearTrailingIdx += 1;
    FarTrailingIdx += 1;
    BodyLongTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/**
 * Three-candle bearish reversal pattern: three white candles with consecutively
 * higher closes whose advance weakens (progressively smaller bodies and/or
 * lengthening upper shadows). Signals that an uptrend's advance is being blocked.
 * A hit (-100) is bearish: the advance is stalling/blocked; meaningful mainly
 * within an existing uptrend.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdladvanceblock](https://ta-lib.org/functions/cdladvanceblock).
 *
 * **Notes**
 * - Does not verify the prior uptrend the pattern classically assumes for
 *   significance.
 * - Although classically read as a bearish reversal, Bulkowski's testing found the
 *   Advance Block actually acts as a bullish continuation 64% of the time.
 *   ([thepatternsite.com](https://thepatternsite.com/AdvanceBlock.html))
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link cdladvanceblockLookback} is a success
 * with `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inOpen - Open price of each bar.
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param outInteger - -100 on a detected pattern (always bearish), 0 otherwise;
 *   never emits +100. Must hold the values produced and never be empty; the input
 *   length always suffices.
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
 * import { cdladvanceblock } from 'ta-lib';
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
 * const { begIdx, count } = cdladvanceblock(
 *   0, open.length - 1,
 *   open, high, low, close,
 *   out,
 * );
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`cdl3whitesoldiers`](https://ta-lib.org/functions/cdl3whitesoldiers)
 * @see [`cdlstalledpattern`](https://ta-lib.org/functions/cdlstalledpattern)
 */
export function cdladvanceblock(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDLADVANCEBLOCK', 'startIdx', startIdx);
  endIdx = requireInt32('CDLADVANCEBLOCK', 'endIdx', endIdx);
  requireIndexRange('CDLADVANCEBLOCK', startIdx, endIdx);
  const guardStart = clampedStart('CDLADVANCEBLOCK', startIdx, cdladvanceblockLookback(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLADVANCEBLOCK', 'inOpen', inOpen, guardInLen);
  requireLength('CDLADVANCEBLOCK', 'inHigh', inHigh, guardInLen);
  requireLength('CDLADVANCEBLOCK', 'inLow', inLow, guardInLen);
  requireLength('CDLADVANCEBLOCK', 'inClose', inClose, guardInLen);
  requireLength('CDLADVANCEBLOCK', 'outInteger', outInteger, guardOutLen);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64) || overlaps(outInteger, inHighF64) || overlaps(outInteger, inLowF64) || overlaps(outInteger, inCloseF64)) {
    throw failure('CDLADVANCEBLOCK', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdladvanceblockImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLADVANCEBLOCK', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live CDLADVANCEBLOCK stream: one value per closed bar, bit-identical to
 * {@link cdladvanceblock} over the same series. Open it with
 * {@link cdladvanceblockOpen} or {@link cdladvanceblockOpenAndFill}; there is
 * nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdladvanceblock](https://ta-lib.org/functions/cdladvanceblock).
 */
export class CdladvanceblockStream {
  /** @internal */
  core: Core;
  /** @internal */
  ShadowShortPeriodTotal: Float64Array = new Float64Array(3);
  /** @internal */
  ShadowLongPeriodTotal: Float64Array = new Float64Array(2);
  /** @internal */
  NearPeriodTotal: Float64Array = new Float64Array(3);
  /** @internal */
  FarPeriodTotal: Float64Array = new Float64Array(3);
  /** @internal */
  BodyLongPeriodTotal: number = 0;
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
  ringPos_BodyLongTrailingIdx: number = 0;
  /** @internal */
  ringCap_BodyLongTrailingIdx: number = 3;
  /** @internal */
  ringLag_BodyLongTrailingIdx: number = 0;
  /** @internal */
  ring_BodyLongTrailingIdx_derived: Float64Array = new Float64Array(3);
  /** @internal */
  ringPos_FarTrailingIdx: number = 0;
  /** @internal */
  ringCap_FarTrailingIdx: number = 3;
  /** @internal */
  ringLag_FarTrailingIdx: number = 0;
  /** @internal */
  ring_FarTrailingIdx_derived: Float64Array = new Float64Array(3);
  /** @internal */
  ringPos_NearTrailingIdx: number = 0;
  /** @internal */
  ringCap_NearTrailingIdx: number = 3;
  /** @internal */
  ringLag_NearTrailingIdx: number = 0;
  /** @internal */
  ring_NearTrailingIdx_derived: Float64Array = new Float64Array(3);
  /** @internal */
  ringPos_ShadowLongTrailingIdx: number = 0;
  /** @internal */
  ringCap_ShadowLongTrailingIdx: number = 3;
  /** @internal */
  ringLag_ShadowLongTrailingIdx: number = 0;
  /** @internal */
  ring_ShadowLongTrailingIdx_derived: Float64Array = new Float64Array(3);
  /** @internal */
  ringPos_ShadowShortTrailingIdx: number = 0;
  /** @internal */
  ringCap_ShadowShortTrailingIdx: number = 3;
  /** @internal */
  ringLag_ShadowShortTrailingIdx: number = 0;
  /** @internal */
  ring_ShadowShortTrailingIdx_derived: Float64Array = new Float64Array(3);
  /** @internal */
  cs_BodyLong_rangeType: number = 0;
  /** @internal */
  cs_BodyLong_avgPeriod: number = 0;
  /** @internal */
  cs_BodyLong_factor: number = 0;
  /** @internal */
  cs_Far_rangeType: number = 0;
  /** @internal */
  cs_Far_avgPeriod: number = 0;
  /** @internal */
  cs_Far_factor: number = 0;
  /** @internal */
  cs_Near_rangeType: number = 0;
  /** @internal */
  cs_Near_avgPeriod: number = 0;
  /** @internal */
  cs_Near_factor: number = 0;
  /** @internal */
  cs_ShadowLong_rangeType: number = 0;
  /** @internal */
  cs_ShadowLong_avgPeriod: number = 0;
  /** @internal */
  cs_ShadowLong_factor: number = 0;
  /** @internal */
  cs_ShadowShort_rangeType: number = 0;
  /** @internal */
  cs_ShadowShort_avgPeriod: number = 0;
  /** @internal */
  cs_ShadowShort_factor: number = 0;
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
   * {@link cdladvanceblock} reports them over the same bars. Each accepted `update`
   * and each `advance` adds one to `count`.
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
      throw failure('CDLADVANCEBLOCK advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw failure('CDLADVANCEBLOCK update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inOpen) || !Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('CDLADVANCEBLOCK update', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    cdladvanceblockStepImpl(this, inOpen, inHigh, inLow, inClose);
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
      throw nonFiniteBar('CDLADVANCEBLOCK peek', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let cur_outInteger = 0;
    const BodyLong_rangeType = sp.cs_BodyLong_rangeType;
    const BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
    const BodyLong_factor = sp.cs_BodyLong_factor;
    const Far_rangeType = sp.cs_Far_rangeType;
    const Far_avgPeriod = sp.cs_Far_avgPeriod;
    const Far_factor = sp.cs_Far_factor;
    const Near_rangeType = sp.cs_Near_rangeType;
    const Near_avgPeriod = sp.cs_Near_avgPeriod;
    const Near_factor = sp.cs_Near_factor;
    const ShadowLong_rangeType = sp.cs_ShadowLong_rangeType;
    const ShadowLong_avgPeriod = sp.cs_ShadowLong_avgPeriod;
    const ShadowLong_factor = sp.cs_ShadowLong_factor;
    const ShadowShort_rangeType = sp.cs_ShadowShort_rangeType;
    const ShadowShort_avgPeriod = sp.cs_ShadowShort_avgPeriod;
    const ShadowShort_factor = sp.cs_ShadowShort_factor;
    if (((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === 1 && /* 1st white */
        ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === 1 && /* 2nd white */
        ((inClose >= inOpen) ? 1 : -1) === 1 &&                 /* 3rd white */
        inClose > sp.lag1_inClose &&
        sp.lag1_inClose > sp.lag2_inClose &&                    /* consecutive higher closes */
        sp.lag1_inOpen > sp.lag2_inOpen &&                      /* 2nd opens within/near 1st real body */
        sp.lag1_inOpen <= sp.lag2_inClose + ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
        inOpen > sp.lag1_inOpen &&                              /* 3rd opens within/near 2nd real body */
        inOpen <= sp.lag1_inClose + ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
        Math.abs(sp.lag2_inClose - sp.lag2_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st: long real body */
        (sp.lag2_inHigh - ((sp.lag2_inClose >= sp.lag2_inOpen) ? sp.lag2_inClose : sp.lag2_inOpen)) < ((ShadowShort_factor * (((ShadowShort_avgPeriod !== 0) ? (sp.ShadowShortPeriodTotal[2] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((ShadowShort_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((ShadowShort_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((ShadowShort_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st: short upper shadow */
        (Math.abs(sp.lag1_inClose - sp.lag1_inOpen) < Math.abs(sp.lag2_inClose - sp.lag2_inOpen) - ((Far_factor * (((Far_avgPeriod !== 0) ? (sp.FarPeriodTotal[2] / Far_avgPeriod) : ((Far_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Far_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Far_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Far_rangeType === 2) ? 2.0 : 1.0)))) && /* ( 2 far smaller than 1 && 3 not longer than 2 ) advance blocked with the 2nd, 3rd must not carry on the advance */
          Math.abs(inClose - inOpen) < Math.abs(sp.lag1_inClose - sp.lag1_inOpen) + ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) ||
         Math.abs(inClose - inOpen) < Math.abs(sp.lag1_inClose - sp.lag1_inOpen) - ((Far_factor * (((Far_avgPeriod !== 0) ? (sp.FarPeriodTotal[1] / Far_avgPeriod) : ((Far_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Far_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Far_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Far_rangeType === 2) ? 2.0 : 1.0)))) || /* 3 far smaller than 2 advance blocked with the 3rd */
         Math.abs(inClose - inOpen) < Math.abs(sp.lag1_inClose - sp.lag1_inOpen) && /* ( 3 smaller than 2 && 2 smaller than 1 && (3 or 2 not short upper shadow) ) advance blocked with progressively smaller real bodies and some upper shadows */
          Math.abs(sp.lag1_inClose - sp.lag1_inOpen) < Math.abs(sp.lag2_inClose - sp.lag2_inOpen) &&
          ((inHigh - ((inClose >= inOpen) ? inClose : inOpen)) > ((ShadowShort_factor * (((ShadowShort_avgPeriod !== 0) ? (sp.ShadowShortPeriodTotal[0] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowShort_rangeType === 2) ? 2.0 : 1.0)))) ||
           (sp.lag1_inHigh - ((sp.lag1_inClose >= sp.lag1_inOpen) ? sp.lag1_inClose : sp.lag1_inOpen)) > ((ShadowShort_factor * (((ShadowShort_avgPeriod !== 0) ? (sp.ShadowShortPeriodTotal[1] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((ShadowShort_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((ShadowShort_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((ShadowShort_rangeType === 2) ? 2.0 : 1.0))))) ||
         Math.abs(inClose - inOpen) < Math.abs(sp.lag1_inClose - sp.lag1_inOpen) && /* ( 3 smaller than 2 && 3 long upper shadow ) advance blocked with 3rd candle's long upper shadow and smaller body */
          (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) > ((ShadowLong_factor * (((ShadowLong_avgPeriod !== 0) ? (sp.ShadowLongPeriodTotal[0] / ShadowLong_avgPeriod) : ((ShadowLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowLong_rangeType === 2) ? 2.0 : 1.0))))))
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
  clone(): CdladvanceblockStream {
    const c = new CdladvanceblockStream(this.core);
    c.ShadowShortPeriodTotal = this.ShadowShortPeriodTotal.slice();
    c.ShadowLongPeriodTotal = this.ShadowLongPeriodTotal.slice();
    c.NearPeriodTotal = this.NearPeriodTotal.slice();
    c.FarPeriodTotal = this.FarPeriodTotal.slice();
    c.BodyLongPeriodTotal = this.BodyLongPeriodTotal;
    c.lag1_inOpen = this.lag1_inOpen;
    c.lag2_inOpen = this.lag2_inOpen;
    c.lag1_inHigh = this.lag1_inHigh;
    c.lag2_inHigh = this.lag2_inHigh;
    c.lag1_inLow = this.lag1_inLow;
    c.lag2_inLow = this.lag2_inLow;
    c.lag1_inClose = this.lag1_inClose;
    c.lag2_inClose = this.lag2_inClose;
    c.ringPos_BodyLongTrailingIdx = this.ringPos_BodyLongTrailingIdx;
    c.ringCap_BodyLongTrailingIdx = this.ringCap_BodyLongTrailingIdx;
    c.ringLag_BodyLongTrailingIdx = this.ringLag_BodyLongTrailingIdx;
    c.ring_BodyLongTrailingIdx_derived = this.ring_BodyLongTrailingIdx_derived.slice();
    c.ringPos_FarTrailingIdx = this.ringPos_FarTrailingIdx;
    c.ringCap_FarTrailingIdx = this.ringCap_FarTrailingIdx;
    c.ringLag_FarTrailingIdx = this.ringLag_FarTrailingIdx;
    c.ring_FarTrailingIdx_derived = this.ring_FarTrailingIdx_derived.slice();
    c.ringPos_NearTrailingIdx = this.ringPos_NearTrailingIdx;
    c.ringCap_NearTrailingIdx = this.ringCap_NearTrailingIdx;
    c.ringLag_NearTrailingIdx = this.ringLag_NearTrailingIdx;
    c.ring_NearTrailingIdx_derived = this.ring_NearTrailingIdx_derived.slice();
    c.ringPos_ShadowLongTrailingIdx = this.ringPos_ShadowLongTrailingIdx;
    c.ringCap_ShadowLongTrailingIdx = this.ringCap_ShadowLongTrailingIdx;
    c.ringLag_ShadowLongTrailingIdx = this.ringLag_ShadowLongTrailingIdx;
    c.ring_ShadowLongTrailingIdx_derived = this.ring_ShadowLongTrailingIdx_derived.slice();
    c.ringPos_ShadowShortTrailingIdx = this.ringPos_ShadowShortTrailingIdx;
    c.ringCap_ShadowShortTrailingIdx = this.ringCap_ShadowShortTrailingIdx;
    c.ringLag_ShadowShortTrailingIdx = this.ringLag_ShadowShortTrailingIdx;
    c.ring_ShadowShortTrailingIdx_derived = this.ring_ShadowShortTrailingIdx_derived.slice();
    c.cs_BodyLong_rangeType = this.cs_BodyLong_rangeType;
    c.cs_BodyLong_avgPeriod = this.cs_BodyLong_avgPeriod;
    c.cs_BodyLong_factor = this.cs_BodyLong_factor;
    c.cs_Far_rangeType = this.cs_Far_rangeType;
    c.cs_Far_avgPeriod = this.cs_Far_avgPeriod;
    c.cs_Far_factor = this.cs_Far_factor;
    c.cs_Near_rangeType = this.cs_Near_rangeType;
    c.cs_Near_avgPeriod = this.cs_Near_avgPeriod;
    c.cs_Near_factor = this.cs_Near_factor;
    c.cs_ShadowLong_rangeType = this.cs_ShadowLong_rangeType;
    c.cs_ShadowLong_avgPeriod = this.cs_ShadowLong_avgPeriod;
    c.cs_ShadowLong_factor = this.cs_ShadowLong_factor;
    c.cs_ShadowShort_rangeType = this.cs_ShadowShort_rangeType;
    c.cs_ShadowShort_avgPeriod = this.cs_ShadowShort_avgPeriod;
    c.cs_ShadowShort_factor = this.cs_ShadowShort_factor;
    c.cur_outInteger = this.cur_outInteger;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cdladvanceblockStepImpl(sp: CdladvanceblockStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  let totIdx = 0;
  const BodyLong_rangeType = sp.cs_BodyLong_rangeType;
  const BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
  const BodyLong_factor = sp.cs_BodyLong_factor;
  const Far_rangeType = sp.cs_Far_rangeType;
  const Far_avgPeriod = sp.cs_Far_avgPeriod;
  const Far_factor = sp.cs_Far_factor;
  const Near_rangeType = sp.cs_Near_rangeType;
  const Near_avgPeriod = sp.cs_Near_avgPeriod;
  const Near_factor = sp.cs_Near_factor;
  const ShadowLong_rangeType = sp.cs_ShadowLong_rangeType;
  const ShadowLong_avgPeriod = sp.cs_ShadowLong_avgPeriod;
  const ShadowLong_factor = sp.cs_ShadowLong_factor;
  const ShadowShort_rangeType = sp.cs_ShadowShort_rangeType;
  const ShadowShort_avgPeriod = sp.cs_ShadowShort_avgPeriod;
  const ShadowShort_factor = sp.cs_ShadowShort_factor;
  sp.ring_BodyLongTrailingIdx_derived[sp.ringPos_BodyLongTrailingIdx] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType === 1) ? (inHigh - inLow) : ((BodyLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  sp.ring_FarTrailingIdx_derived[sp.ringPos_FarTrailingIdx] = ((Far_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((Far_rangeType === 1) ? (inHigh - inLow) : ((Far_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  sp.ring_NearTrailingIdx_derived[sp.ringPos_NearTrailingIdx] = ((Near_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((Near_rangeType === 1) ? (inHigh - inLow) : ((Near_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  sp.ring_ShadowLongTrailingIdx_derived[sp.ringPos_ShadowLongTrailingIdx] = ((ShadowLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  sp.ring_ShadowShortTrailingIdx_derived[sp.ringPos_ShadowShortTrailingIdx] = ((ShadowShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  if (((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === 1 && /* 1st white */
      ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === 1 && /* 2nd white */
      ((inClose >= inOpen) ? 1 : -1) === 1 &&                 /* 3rd white */
      inClose > sp.lag1_inClose &&
      sp.lag1_inClose > sp.lag2_inClose &&                    /* consecutive higher closes */
      sp.lag1_inOpen > sp.lag2_inOpen &&                      /* 2nd opens within/near 1st real body */
      sp.lag1_inOpen <= sp.lag2_inClose + ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
      inOpen > sp.lag1_inOpen &&                              /* 3rd opens within/near 2nd real body */
      inOpen <= sp.lag1_inClose + ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
      Math.abs(sp.lag2_inClose - sp.lag2_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st: long real body */
      (sp.lag2_inHigh - ((sp.lag2_inClose >= sp.lag2_inOpen) ? sp.lag2_inClose : sp.lag2_inOpen)) < ((ShadowShort_factor * (((ShadowShort_avgPeriod !== 0) ? (sp.ShadowShortPeriodTotal[2] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((ShadowShort_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((ShadowShort_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((ShadowShort_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st: short upper shadow */
      (Math.abs(sp.lag1_inClose - sp.lag1_inOpen) < Math.abs(sp.lag2_inClose - sp.lag2_inOpen) - ((Far_factor * (((Far_avgPeriod !== 0) ? (sp.FarPeriodTotal[2] / Far_avgPeriod) : ((Far_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Far_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Far_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Far_rangeType === 2) ? 2.0 : 1.0)))) && /* ( 2 far smaller than 1 && 3 not longer than 2 ) advance blocked with the 2nd, 3rd must not carry on the advance */
        Math.abs(inClose - inOpen) < Math.abs(sp.lag1_inClose - sp.lag1_inOpen) + ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) ||
       Math.abs(inClose - inOpen) < Math.abs(sp.lag1_inClose - sp.lag1_inOpen) - ((Far_factor * (((Far_avgPeriod !== 0) ? (sp.FarPeriodTotal[1] / Far_avgPeriod) : ((Far_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Far_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Far_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Far_rangeType === 2) ? 2.0 : 1.0)))) || /* 3 far smaller than 2 advance blocked with the 3rd */
       Math.abs(inClose - inOpen) < Math.abs(sp.lag1_inClose - sp.lag1_inOpen) && /* ( 3 smaller than 2 && 2 smaller than 1 && (3 or 2 not short upper shadow) ) advance blocked with progressively smaller real bodies and some upper shadows */
        Math.abs(sp.lag1_inClose - sp.lag1_inOpen) < Math.abs(sp.lag2_inClose - sp.lag2_inOpen) &&
        ((inHigh - ((inClose >= inOpen) ? inClose : inOpen)) > ((ShadowShort_factor * (((ShadowShort_avgPeriod !== 0) ? (sp.ShadowShortPeriodTotal[0] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowShort_rangeType === 2) ? 2.0 : 1.0)))) ||
         (sp.lag1_inHigh - ((sp.lag1_inClose >= sp.lag1_inOpen) ? sp.lag1_inClose : sp.lag1_inOpen)) > ((ShadowShort_factor * (((ShadowShort_avgPeriod !== 0) ? (sp.ShadowShortPeriodTotal[1] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((ShadowShort_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((ShadowShort_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((ShadowShort_rangeType === 2) ? 2.0 : 1.0))))) ||
       Math.abs(inClose - inOpen) < Math.abs(sp.lag1_inClose - sp.lag1_inOpen) && /* ( 3 smaller than 2 && 3 long upper shadow ) advance blocked with 3rd candle's long upper shadow and smaller body */
        (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) > ((ShadowLong_factor * (((ShadowLong_avgPeriod !== 0) ? (sp.ShadowLongPeriodTotal[0] / ShadowLong_avgPeriod) : ((ShadowLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowLong_rangeType === 2) ? 2.0 : 1.0))))))
  {
    sp.cur_outInteger = -100;
  } else {
    sp.cur_outInteger = 0;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  for (totIdx = 2; totIdx >= 0; totIdx -= 1) {
    sp.ShadowShortPeriodTotal[totIdx] = sp.ShadowShortPeriodTotal[totIdx] + (sp.ring_ShadowShortTrailingIdx_derived[(sp.ringPos_ShadowShortTrailingIdx + sp.ringCap_ShadowShortTrailingIdx - totIdx >= sp.ringCap_ShadowShortTrailingIdx) ? sp.ringPos_ShadowShortTrailingIdx + sp.ringCap_ShadowShortTrailingIdx - totIdx - sp.ringCap_ShadowShortTrailingIdx : sp.ringPos_ShadowShortTrailingIdx + sp.ringCap_ShadowShortTrailingIdx - totIdx] - sp.ring_ShadowShortTrailingIdx_derived[(((sp.ringPos_ShadowShortTrailingIdx + sp.ringCap_ShadowShortTrailingIdx - sp.ringLag_ShadowShortTrailingIdx - totIdx) % sp.ringCap_ShadowShortTrailingIdx) | 0)]);
  }
  for (totIdx = 1; totIdx >= 0; totIdx -= 1) {
    sp.ShadowLongPeriodTotal[totIdx] = sp.ShadowLongPeriodTotal[totIdx] + (sp.ring_ShadowLongTrailingIdx_derived[(sp.ringPos_ShadowLongTrailingIdx + sp.ringCap_ShadowLongTrailingIdx - totIdx >= sp.ringCap_ShadowLongTrailingIdx) ? sp.ringPos_ShadowLongTrailingIdx + sp.ringCap_ShadowLongTrailingIdx - totIdx - sp.ringCap_ShadowLongTrailingIdx : sp.ringPos_ShadowLongTrailingIdx + sp.ringCap_ShadowLongTrailingIdx - totIdx] - sp.ring_ShadowLongTrailingIdx_derived[(((sp.ringPos_ShadowLongTrailingIdx + sp.ringCap_ShadowLongTrailingIdx - sp.ringLag_ShadowLongTrailingIdx - totIdx) % sp.ringCap_ShadowLongTrailingIdx) | 0)]);
  }
  for (totIdx = 2; totIdx >= 1; totIdx -= 1) {
    sp.FarPeriodTotal[totIdx] = sp.FarPeriodTotal[totIdx] + (sp.ring_FarTrailingIdx_derived[(sp.ringPos_FarTrailingIdx + sp.ringCap_FarTrailingIdx - totIdx >= sp.ringCap_FarTrailingIdx) ? sp.ringPos_FarTrailingIdx + sp.ringCap_FarTrailingIdx - totIdx - sp.ringCap_FarTrailingIdx : sp.ringPos_FarTrailingIdx + sp.ringCap_FarTrailingIdx - totIdx] - sp.ring_FarTrailingIdx_derived[(((sp.ringPos_FarTrailingIdx + sp.ringCap_FarTrailingIdx - sp.ringLag_FarTrailingIdx - totIdx) % sp.ringCap_FarTrailingIdx) | 0)]);
    sp.NearPeriodTotal[totIdx] = sp.NearPeriodTotal[totIdx] + (sp.ring_NearTrailingIdx_derived[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - totIdx >= sp.ringCap_NearTrailingIdx) ? sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - totIdx - sp.ringCap_NearTrailingIdx : sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - totIdx] - sp.ring_NearTrailingIdx_derived[(((sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - totIdx) % sp.ringCap_NearTrailingIdx) | 0)]);
  }
  sp.BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0))) - sp.ring_BodyLongTrailingIdx_derived[(((sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 2) % sp.ringCap_BodyLongTrailingIdx) | 0)];
  sp.lag2_inOpen = sp.lag1_inOpen;
  sp.lag1_inOpen = inOpen;
  sp.lag2_inHigh = sp.lag1_inHigh;
  sp.lag1_inHigh = inHigh;
  sp.lag2_inLow = sp.lag1_inLow;
  sp.lag1_inLow = inLow;
  sp.lag2_inClose = sp.lag1_inClose;
  sp.lag1_inClose = inClose;
  sp.ringPos_BodyLongTrailingIdx = sp.ringPos_BodyLongTrailingIdx + 1;
  if (sp.ringPos_BodyLongTrailingIdx >= sp.ringCap_BodyLongTrailingIdx) {
    sp.ringPos_BodyLongTrailingIdx = 0;
  }
  sp.ringPos_FarTrailingIdx = sp.ringPos_FarTrailingIdx + 1;
  if (sp.ringPos_FarTrailingIdx >= sp.ringCap_FarTrailingIdx) {
    sp.ringPos_FarTrailingIdx = 0;
  }
  sp.ringPos_NearTrailingIdx = sp.ringPos_NearTrailingIdx + 1;
  if (sp.ringPos_NearTrailingIdx >= sp.ringCap_NearTrailingIdx) {
    sp.ringPos_NearTrailingIdx = 0;
  }
  sp.ringPos_ShadowLongTrailingIdx = sp.ringPos_ShadowLongTrailingIdx + 1;
  if (sp.ringPos_ShadowLongTrailingIdx >= sp.ringCap_ShadowLongTrailingIdx) {
    sp.ringPos_ShadowLongTrailingIdx = 0;
  }
  sp.ringPos_ShadowShortTrailingIdx = sp.ringPos_ShadowShortTrailingIdx + 1;
  if (sp.ringPos_ShadowShortTrailingIdx >= sp.ringCap_ShadowShortTrailingIdx) {
    sp.ringPos_ShadowShortTrailingIdx = 0;
  }
}

function cdladvanceblockOpenImpl(
  sp: CdladvanceblockStream,
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
  const ShadowShortPeriodTotal = new Float64Array(3);
  const ShadowLongPeriodTotal = new Float64Array(2);
  const NearPeriodTotal = new Float64Array(3);
  const FarPeriodTotal = new Float64Array(3);
  let BodyLongPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let totIdx = 0;
  let BodyLongTrailingIdx = 0;
  let ShadowShortTrailingIdx = 0;
  let ShadowLongTrailingIdx = 0;
  let NearTrailingIdx = 0;
  let FarTrailingIdx = 0;
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
  const Far_rangeType: number = core.candleSettings[CandleSettingType.FAR].rangeType;
  const Far_avgPeriod = core.candleSettings[CandleSettingType.FAR].avgPeriod;
  const Far_factor = core.candleSettings[CandleSettingType.FAR].factor;
  const Near_rangeType: number = core.candleSettings[CandleSettingType.NEAR].rangeType;
  const Near_avgPeriod = core.candleSettings[CandleSettingType.NEAR].avgPeriod;
  const Near_factor = core.candleSettings[CandleSettingType.NEAR].factor;
  const ShadowLong_rangeType: number = core.candleSettings[CandleSettingType.SHADOW_LONG].rangeType;
  const ShadowLong_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_LONG].avgPeriod;
  const ShadowLong_factor = core.candleSettings[CandleSettingType.SHADOW_LONG].factor;
  const ShadowShort_rangeType: number = core.candleSettings[CandleSettingType.SHADOW_SHORT].rangeType;
  const ShadowShort_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_SHORT].avgPeriod;
  const ShadowShort_factor = core.candleSettings[CandleSettingType.SHADOW_SHORT].factor;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdladvanceblockLookback(core);
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
  ShadowShortPeriodTotal[2] = 0;
  ShadowShortPeriodTotal[1] = 0;
  ShadowShortPeriodTotal[0] = 0;
  ShadowShortTrailingIdx = startIdx - ShadowShort_avgPeriod;
  ShadowLongPeriodTotal[1] = 0;
  ShadowLongPeriodTotal[0] = 0;
  ShadowLongTrailingIdx = startIdx - ShadowLong_avgPeriod;
  NearPeriodTotal[2] = 0;
  NearPeriodTotal[1] = 0;
  NearPeriodTotal[0] = 0;
  NearTrailingIdx = startIdx - Near_avgPeriod;
  FarPeriodTotal[2] = 0;
  FarPeriodTotal[1] = 0;
  FarPeriodTotal[0] = 0;
  FarTrailingIdx = startIdx - Far_avgPeriod;
  BodyLongPeriodTotal = 0;
  BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
  i = ShadowShortTrailingIdx;
  while (i < startIdx) {
    ShadowShortPeriodTotal[2] = ShadowShortPeriodTotal[2] + ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
    ShadowShortPeriodTotal[1] = ShadowShortPeriodTotal[1] + ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    ShadowShortPeriodTotal[0] = ShadowShortPeriodTotal[0] + ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = ShadowLongTrailingIdx;
  while (i < startIdx) {
    ShadowLongPeriodTotal[1] = ShadowLongPeriodTotal[1] + ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowLong_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    ShadowLongPeriodTotal[0] = ShadowLongPeriodTotal[0] + ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = NearTrailingIdx;
  while (i < startIdx) {
    NearPeriodTotal[2] = NearPeriodTotal[2] + ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
    NearPeriodTotal[1] = NearPeriodTotal[1] + ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    i += 1;
  }
  i = FarTrailingIdx;
  while (i < startIdx) {
    FarPeriodTotal[2] = FarPeriodTotal[2] + ((Far_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Far_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Far_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
    FarPeriodTotal[1] = FarPeriodTotal[1] + ((Far_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Far_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Far_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    i += 1;
  }
  i = BodyLongTrailingIdx;
  while (i < startIdx) {
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - three white candlesticks with consecutively higher closes
   * - each candle opens within or near the previous white real body
   * - first candle: long white with no or very short upper shadow (a short shadow is accepted too for more flexibility)
   * - second and third candles, or only third candle, show signs of weakening: progressively smaller white real bodies
   * and/or relatively long upper shadows; see below for specific conditions
   * The meanings of "long body", "short shadow", "far" and "near" are specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === 1 && /* 1st white */
        ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === 1 && /* 2nd white */
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === 1 &&         /* 3rd white */
        inClose[i] > inClose[i - 1] &&
        inClose[i - 1] > inClose[i - 2] &&                    /* consecutive higher closes */
        inOpen[i - 1] > inOpen[i - 2] &&                      /* 2nd opens within/near 1st real body */
        inOpen[i - 1] <= inClose[i - 2] + ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
        inOpen[i] > inOpen[i - 1] &&                          /* 3rd opens within/near 2nd real body */
        inOpen[i] <= inClose[i - 1] + ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
        Math.abs(inClose[i - 2] - inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st: long real body */
        (inHigh[i - 2] - ((inClose[i - 2] >= inOpen[i - 2]) ? inClose[i - 2] : inOpen[i - 2])) < ((ShadowShort_factor * (((ShadowShort_avgPeriod !== 0) ? (ShadowShortPeriodTotal[2] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((ShadowShort_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st: short upper shadow */
        (Math.abs(inClose[i - 1] - inOpen[i - 1]) < Math.abs(inClose[i - 2] - inOpen[i - 2]) - ((Far_factor * (((Far_avgPeriod !== 0) ? (FarPeriodTotal[2] / Far_avgPeriod) : ((Far_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Far_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Far_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Far_rangeType === 2) ? 2.0 : 1.0)))) && /* ( 2 far smaller than 1 && 3 not longer than 2 ) advance blocked with the 2nd, 3rd must not carry on the advance */
          Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) + ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) ||
         Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) - ((Far_factor * (((Far_avgPeriod !== 0) ? (FarPeriodTotal[1] / Far_avgPeriod) : ((Far_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Far_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Far_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Far_rangeType === 2) ? 2.0 : 1.0)))) || /* 3 far smaller than 2 advance blocked with the 3rd */
         Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) && /* ( 3 smaller than 2 && 2 smaller than 1 && (3 or 2 not short upper shadow) ) advance blocked with progressively smaller real bodies and some upper shadows */
          Math.abs(inClose[i - 1] - inOpen[i - 1]) < Math.abs(inClose[i - 2] - inOpen[i - 2]) &&
          ((inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowShort_factor * (((ShadowShort_avgPeriod !== 0) ? (ShadowShortPeriodTotal[0] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowShort_rangeType === 2) ? 2.0 : 1.0)))) ||
           (inHigh[i - 1] - ((inClose[i - 1] >= inOpen[i - 1]) ? inClose[i - 1] : inOpen[i - 1])) > ((ShadowShort_factor * (((ShadowShort_avgPeriod !== 0) ? (ShadowShortPeriodTotal[1] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((ShadowShort_rangeType === 2) ? 2.0 : 1.0))))) ||
         Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) && /* ( 3 smaller than 2 && 3 long upper shadow ) advance blocked with 3rd candle's long upper shadow and smaller body */
          (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowLong_factor * (((ShadowLong_avgPeriod !== 0) ? (ShadowLongPeriodTotal[0] / ShadowLong_avgPeriod) : ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowLong_rangeType === 2) ? 2.0 : 1.0))))))
    {
      outInteger[((outIdx++ * outStride) | 0)] = -100;
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    for (totIdx = 2; totIdx >= 0; totIdx -= 1) {
      ShadowShortPeriodTotal[totIdx] = ShadowShortPeriodTotal[totIdx] + (((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((ShadowShort_rangeType === 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i - totIdx] - (((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inClose[i - totIdx]) : (inOpen[i - totIdx]))) + ((((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inOpen[i - totIdx]) : (inClose[i - totIdx])) - inLow[i - totIdx])) : 0.0))) - ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[ShadowShortTrailingIdx - totIdx] - inOpen[ShadowShortTrailingIdx - totIdx])) : ((ShadowShort_rangeType === 1) ? (inHigh[ShadowShortTrailingIdx - totIdx] - inLow[ShadowShortTrailingIdx - totIdx]) : ((ShadowShort_rangeType === 2) ? ((inHigh[ShadowShortTrailingIdx - totIdx] - (((inClose[ShadowShortTrailingIdx - totIdx]) >= (inOpen[ShadowShortTrailingIdx - totIdx])) ? (inClose[ShadowShortTrailingIdx - totIdx]) : (inOpen[ShadowShortTrailingIdx - totIdx]))) + ((((inClose[ShadowShortTrailingIdx - totIdx]) >= (inOpen[ShadowShortTrailingIdx - totIdx])) ? (inOpen[ShadowShortTrailingIdx - totIdx]) : (inClose[ShadowShortTrailingIdx - totIdx])) - inLow[ShadowShortTrailingIdx - totIdx])) : 0.0))));
    }
    for (totIdx = 1; totIdx >= 0; totIdx -= 1) {
      ShadowLongPeriodTotal[totIdx] = ShadowLongPeriodTotal[totIdx] + (((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((ShadowLong_rangeType === 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i - totIdx] - (((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inClose[i - totIdx]) : (inOpen[i - totIdx]))) + ((((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inOpen[i - totIdx]) : (inClose[i - totIdx])) - inLow[i - totIdx])) : 0.0))) - ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[ShadowLongTrailingIdx - totIdx] - inOpen[ShadowLongTrailingIdx - totIdx])) : ((ShadowLong_rangeType === 1) ? (inHigh[ShadowLongTrailingIdx - totIdx] - inLow[ShadowLongTrailingIdx - totIdx]) : ((ShadowLong_rangeType === 2) ? ((inHigh[ShadowLongTrailingIdx - totIdx] - (((inClose[ShadowLongTrailingIdx - totIdx]) >= (inOpen[ShadowLongTrailingIdx - totIdx])) ? (inClose[ShadowLongTrailingIdx - totIdx]) : (inOpen[ShadowLongTrailingIdx - totIdx]))) + ((((inClose[ShadowLongTrailingIdx - totIdx]) >= (inOpen[ShadowLongTrailingIdx - totIdx])) ? (inOpen[ShadowLongTrailingIdx - totIdx]) : (inClose[ShadowLongTrailingIdx - totIdx])) - inLow[ShadowLongTrailingIdx - totIdx])) : 0.0))));
    }
    for (totIdx = 2; totIdx >= 1; totIdx -= 1) {
      FarPeriodTotal[totIdx] = FarPeriodTotal[totIdx] + (((Far_rangeType === 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((Far_rangeType === 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((Far_rangeType === 2) ? ((inHigh[i - totIdx] - (((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inClose[i - totIdx]) : (inOpen[i - totIdx]))) + ((((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inOpen[i - totIdx]) : (inClose[i - totIdx])) - inLow[i - totIdx])) : 0.0))) - ((Far_rangeType === 0) ? (Math.abs(inClose[FarTrailingIdx - totIdx] - inOpen[FarTrailingIdx - totIdx])) : ((Far_rangeType === 1) ? (inHigh[FarTrailingIdx - totIdx] - inLow[FarTrailingIdx - totIdx]) : ((Far_rangeType === 2) ? ((inHigh[FarTrailingIdx - totIdx] - (((inClose[FarTrailingIdx - totIdx]) >= (inOpen[FarTrailingIdx - totIdx])) ? (inClose[FarTrailingIdx - totIdx]) : (inOpen[FarTrailingIdx - totIdx]))) + ((((inClose[FarTrailingIdx - totIdx]) >= (inOpen[FarTrailingIdx - totIdx])) ? (inOpen[FarTrailingIdx - totIdx]) : (inClose[FarTrailingIdx - totIdx])) - inLow[FarTrailingIdx - totIdx])) : 0.0))));
      NearPeriodTotal[totIdx] = NearPeriodTotal[totIdx] + (((Near_rangeType === 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((Near_rangeType === 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((Near_rangeType === 2) ? ((inHigh[i - totIdx] - (((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inClose[i - totIdx]) : (inOpen[i - totIdx]))) + ((((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inOpen[i - totIdx]) : (inClose[i - totIdx])) - inLow[i - totIdx])) : 0.0))) - ((Near_rangeType === 0) ? (Math.abs(inClose[NearTrailingIdx - totIdx] - inOpen[NearTrailingIdx - totIdx])) : ((Near_rangeType === 1) ? (inHigh[NearTrailingIdx - totIdx] - inLow[NearTrailingIdx - totIdx]) : ((Near_rangeType === 2) ? ((inHigh[NearTrailingIdx - totIdx] - (((inClose[NearTrailingIdx - totIdx]) >= (inOpen[NearTrailingIdx - totIdx])) ? (inClose[NearTrailingIdx - totIdx]) : (inOpen[NearTrailingIdx - totIdx]))) + ((((inClose[NearTrailingIdx - totIdx]) >= (inOpen[NearTrailingIdx - totIdx])) ? (inOpen[NearTrailingIdx - totIdx]) : (inClose[NearTrailingIdx - totIdx])) - inLow[NearTrailingIdx - totIdx])) : 0.0))));
    }
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0))) - ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 2] - inOpen[BodyLongTrailingIdx - 2])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyLongTrailingIdx - 2] - inLow[BodyLongTrailingIdx - 2]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyLongTrailingIdx - 2] - (((inClose[BodyLongTrailingIdx - 2]) >= (inOpen[BodyLongTrailingIdx - 2])) ? (inClose[BodyLongTrailingIdx - 2]) : (inOpen[BodyLongTrailingIdx - 2]))) + ((((inClose[BodyLongTrailingIdx - 2]) >= (inOpen[BodyLongTrailingIdx - 2])) ? (inOpen[BodyLongTrailingIdx - 2]) : (inClose[BodyLongTrailingIdx - 2])) - inLow[BodyLongTrailingIdx - 2])) : 0.0)));
    i += 1;
    ShadowShortTrailingIdx += 1;
    ShadowLongTrailingIdx += 1;
    NearTrailingIdx += 1;
    FarTrailingIdx += 1;
    BodyLongTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const capLag_BodyLongTrailingIdx = i - BodyLongTrailingIdx;
  const cap_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx + 3;
  if (capLag_BodyLongTrailingIdx < 0 || cap_BodyLongTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_BodyLongTrailingIdx = cap_BodyLongTrailingIdx > 0 ? cap_BodyLongTrailingIdx : 1;
  const capRing_BodyLongTrailingIdx_derived = new Float64Array(allocN_BodyLongTrailingIdx);
  for (let fillJ = historyLen - cap_BodyLongTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_BodyLongTrailingIdx_derived[fillJ % cap_BodyLongTrailingIdx] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyLong_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyLong_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  const capLag_FarTrailingIdx = i - FarTrailingIdx;
  const cap_FarTrailingIdx = capLag_FarTrailingIdx + 3;
  if (capLag_FarTrailingIdx < 0 || cap_FarTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_FarTrailingIdx = cap_FarTrailingIdx > 0 ? cap_FarTrailingIdx : 1;
  const capRing_FarTrailingIdx_derived = new Float64Array(allocN_FarTrailingIdx);
  for (let fillJ = historyLen - cap_FarTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_FarTrailingIdx_derived[fillJ % cap_FarTrailingIdx] = ((Far_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((Far_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((Far_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  const capLag_NearTrailingIdx = i - NearTrailingIdx;
  const cap_NearTrailingIdx = capLag_NearTrailingIdx + 3;
  if (capLag_NearTrailingIdx < 0 || cap_NearTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_NearTrailingIdx = cap_NearTrailingIdx > 0 ? cap_NearTrailingIdx : 1;
  const capRing_NearTrailingIdx_derived = new Float64Array(allocN_NearTrailingIdx);
  for (let fillJ = historyLen - cap_NearTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_NearTrailingIdx_derived[fillJ % cap_NearTrailingIdx] = ((Near_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((Near_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((Near_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  const capLag_ShadowLongTrailingIdx = i - ShadowLongTrailingIdx;
  const cap_ShadowLongTrailingIdx = capLag_ShadowLongTrailingIdx + 3;
  if (capLag_ShadowLongTrailingIdx < 0 || cap_ShadowLongTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_ShadowLongTrailingIdx = cap_ShadowLongTrailingIdx > 0 ? cap_ShadowLongTrailingIdx : 1;
  const capRing_ShadowLongTrailingIdx_derived = new Float64Array(allocN_ShadowLongTrailingIdx);
  for (let fillJ = historyLen - cap_ShadowLongTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_ShadowLongTrailingIdx_derived[fillJ % cap_ShadowLongTrailingIdx] = ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((ShadowLong_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((ShadowLong_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  const capLag_ShadowShortTrailingIdx = i - ShadowShortTrailingIdx;
  const cap_ShadowShortTrailingIdx = capLag_ShadowShortTrailingIdx + 3;
  if (capLag_ShadowShortTrailingIdx < 0 || cap_ShadowShortTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_ShadowShortTrailingIdx = cap_ShadowShortTrailingIdx > 0 ? cap_ShadowShortTrailingIdx : 1;
  const capRing_ShadowShortTrailingIdx_derived = new Float64Array(allocN_ShadowShortTrailingIdx);
  for (let fillJ = historyLen - cap_ShadowShortTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_ShadowShortTrailingIdx_derived[fillJ % cap_ShadowShortTrailingIdx] = ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((ShadowShort_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((ShadowShort_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  sp.ShadowShortPeriodTotal = ShadowShortPeriodTotal;
  sp.ShadowLongPeriodTotal = ShadowLongPeriodTotal;
  sp.NearPeriodTotal = NearPeriodTotal;
  sp.FarPeriodTotal = FarPeriodTotal;
  sp.BodyLongPeriodTotal = BodyLongPeriodTotal;
  sp.lag1_inOpen = inOpen[historyLen - 1];
  sp.lag2_inOpen = inOpen[historyLen - 2];
  sp.lag1_inHigh = inHigh[historyLen - 1];
  sp.lag2_inHigh = inHigh[historyLen - 2];
  sp.lag1_inLow = inLow[historyLen - 1];
  sp.lag2_inLow = inLow[historyLen - 2];
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.lag2_inClose = inClose[historyLen - 2];
  sp.ringPos_BodyLongTrailingIdx = historyLen % cap_BodyLongTrailingIdx;
  sp.ringCap_BodyLongTrailingIdx = cap_BodyLongTrailingIdx;
  sp.ringLag_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx;
  sp.ring_BodyLongTrailingIdx_derived = capRing_BodyLongTrailingIdx_derived;
  sp.ringPos_FarTrailingIdx = historyLen % cap_FarTrailingIdx;
  sp.ringCap_FarTrailingIdx = cap_FarTrailingIdx;
  sp.ringLag_FarTrailingIdx = capLag_FarTrailingIdx;
  sp.ring_FarTrailingIdx_derived = capRing_FarTrailingIdx_derived;
  sp.ringPos_NearTrailingIdx = historyLen % cap_NearTrailingIdx;
  sp.ringCap_NearTrailingIdx = cap_NearTrailingIdx;
  sp.ringLag_NearTrailingIdx = capLag_NearTrailingIdx;
  sp.ring_NearTrailingIdx_derived = capRing_NearTrailingIdx_derived;
  sp.ringPos_ShadowLongTrailingIdx = historyLen % cap_ShadowLongTrailingIdx;
  sp.ringCap_ShadowLongTrailingIdx = cap_ShadowLongTrailingIdx;
  sp.ringLag_ShadowLongTrailingIdx = capLag_ShadowLongTrailingIdx;
  sp.ring_ShadowLongTrailingIdx_derived = capRing_ShadowLongTrailingIdx_derived;
  sp.ringPos_ShadowShortTrailingIdx = historyLen % cap_ShadowShortTrailingIdx;
  sp.ringCap_ShadowShortTrailingIdx = cap_ShadowShortTrailingIdx;
  sp.ringLag_ShadowShortTrailingIdx = capLag_ShadowShortTrailingIdx;
  sp.ring_ShadowShortTrailingIdx_derived = capRing_ShadowShortTrailingIdx_derived;
  sp.cs_BodyLong_rangeType = BodyLong_rangeType;
  sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
  sp.cs_BodyLong_factor = BodyLong_factor;
  sp.cs_Far_rangeType = Far_rangeType;
  sp.cs_Far_avgPeriod = Far_avgPeriod;
  sp.cs_Far_factor = Far_factor;
  sp.cs_Near_rangeType = Near_rangeType;
  sp.cs_Near_avgPeriod = Near_avgPeriod;
  sp.cs_Near_factor = Near_factor;
  sp.cs_ShadowLong_rangeType = ShadowLong_rangeType;
  sp.cs_ShadowLong_avgPeriod = ShadowLong_avgPeriod;
  sp.cs_ShadowLong_factor = ShadowLong_factor;
  sp.cs_ShadowShort_rangeType = ShadowShort_rangeType;
  sp.cs_ShadowShort_avgPeriod = ShadowShort_avgPeriod;
  sp.cs_ShadowShort_factor = ShadowShort_factor;
  sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cdladvanceblockOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): CdladvanceblockStream {
  const sp = new CdladvanceblockStream(core);
  const retCode = cdladvanceblockOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLADVANCEBLOCK openAndFill', inOpen.length, startIdx, cdladvanceblockLookback(core));
  }
  throw streamFailure('CDLADVANCEBLOCK openAndFill', retCode);
}

/** @internal */
export function cdladvanceblockOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): CdladvanceblockStream {
  const sp = new CdladvanceblockStream(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdladvanceblockOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLADVANCEBLOCK open', inOpen.length, startIdx, cdladvanceblockLookback(core));
  }
  throw streamFailure('CDLADVANCEBLOCK open', retCode);
}

/**
 * Opens a live CDLADVANCEBLOCK stream over the warm-up history: `value()` starts
 * at the last history bar, bit-identical to {@link cdladvanceblock} there.
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
 *   than `cdladvanceblockLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for
 *   an empty one, and `TA_BAD_PARAM` for histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdladvanceblockOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdladvanceblockOpen(open, high, low, close);
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdladvanceblockOpen(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): CdladvanceblockStream {
  requireArgument('CDLADVANCEBLOCK open', 'inOpen', inOpen);
  requireHistory('CDLADVANCEBLOCK open', inOpen.length);
  requireArgument('CDLADVANCEBLOCK open', 'inHigh', inHigh);
  requireArgument('CDLADVANCEBLOCK open', 'inLow', inLow);
  requireArgument('CDLADVANCEBLOCK open', 'inClose', inClose);
  requireHistoryLength('CDLADVANCEBLOCK open', 'inHigh', inHigh.length, inOpen.length);
  requireHistoryLength('CDLADVANCEBLOCK open', 'inLow', inLow.length, inOpen.length);
  requireHistoryLength('CDLADVANCEBLOCK open', 'inClose', inClose.length, inOpen.length);
  return cdladvanceblockOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/**
 * {@link cdladvanceblockOpen} that also fills the outputs over the whole history
 * in the same pass, bit-identically to {@link cdladvanceblock}. The range written
 * is the stream's `outRange()`.
 *
 * @param inOpen - Warm-up history. Open price of each bar. Every history has the
 *   same length.
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param outInteger - -100 on a detected pattern (always bearish), 0 otherwise;
 *   never emits +100. Must hold `historyLen - lookback` values and share no memory
 *   with an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdladvanceblockLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for
 *   an empty one, and `TA_BAD_PARAM` for histories of different lengths or an output
 *   too short or starting where another output or a `Float64Array` input starts, or,
 *   on a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdladvanceblockOpenAndFill } from 'ta-lib';
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
 * const stream = cdladvanceblockOpenAndFill(open, high, low, close, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function cdladvanceblockOpenAndFill(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): CdladvanceblockStream {
  requireArgument('CDLADVANCEBLOCK openAndFill', 'inOpen', inOpen);
  requireHistory('CDLADVANCEBLOCK openAndFill', inOpen.length);
  requireArgument('CDLADVANCEBLOCK openAndFill', 'inHigh', inHigh);
  requireArgument('CDLADVANCEBLOCK openAndFill', 'inLow', inLow);
  requireArgument('CDLADVANCEBLOCK openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('CDLADVANCEBLOCK openAndFill', inOpen.length, cdladvanceblockLookback(core));
  requireHistoryLength('CDLADVANCEBLOCK openAndFill', 'inHigh', inHigh.length, inOpen.length);
  requireHistoryLength('CDLADVANCEBLOCK openAndFill', 'inLow', inLow.length, inOpen.length);
  requireHistoryLength('CDLADVANCEBLOCK openAndFill', 'inClose', inClose.length, inOpen.length);
  requireLength('CDLADVANCEBLOCK openAndFill', 'outInteger', outInteger, guardOutLen);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) || (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) || (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) || (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset)) {
    throw streamFailure('CDLADVANCEBLOCK openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outInteger, inOpenF64) || overlaps(outInteger, inHighF64) || overlaps(outInteger, inLowF64) || overlaps(outInteger, inCloseF64))) {
    throw streamFailure('CDLADVANCEBLOCK openAndFill', RetCode.BAD_PARAM);
  }
  return cdladvanceblockOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}
