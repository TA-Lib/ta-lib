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
 *  110526 MF,CC  Carry the ranges of the previous bars instead of recomputing them
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { CandleSettingType, Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function cdladvanceblockLookbackPositional(core: Core = Core.DEFAULT): number {
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  const Far_avgPeriod = core.candleSettings[CandleSettingType.FAR].avgPeriod;
  const Near_avgPeriod = core.candleSettings[CandleSettingType.NEAR].avgPeriod;
  const ShadowLong_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_LONG].avgPeriod;
  const ShadowShort_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_SHORT].avgPeriod;
  return (((ShadowLong_avgPeriod > ShadowShort_avgPeriod ? ShadowLong_avgPeriod : ShadowShort_avgPeriod) > (Far_avgPeriod > Near_avgPeriod ? Far_avgPeriod : Near_avgPeriod) ? (ShadowLong_avgPeriod > ShadowShort_avgPeriod ? ShadowLong_avgPeriod : ShadowShort_avgPeriod) : (Far_avgPeriod > Near_avgPeriod ? Far_avgPeriod : Near_avgPeriod)) > BodyLong_avgPeriod ? ((ShadowLong_avgPeriod > ShadowShort_avgPeriod ? ShadowLong_avgPeriod : ShadowShort_avgPeriod) > (Far_avgPeriod > Near_avgPeriod ? Far_avgPeriod : Near_avgPeriod) ? (ShadowLong_avgPeriod > ShadowShort_avgPeriod ? ShadowLong_avgPeriod : ShadowShort_avgPeriod) : (Far_avgPeriod > Near_avgPeriod ? Far_avgPeriod : Near_avgPeriod)) : BodyLong_avgPeriod) + 2;
}

/** @internal */
export function cdladvanceblockDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
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
  let ShadowShortPeriodTotalRange0 = 0;
  let ShadowShortPeriodTotalTrailRange0 = 0;
  let ShadowShortPeriodTotalRange1 = 0;
  let ShadowShortPeriodTotalTrailRange1 = 0;
  let ShadowShortPeriodTotalRange2 = 0;
  let ShadowShortPeriodTotalTrailRange2 = 0;
  let ShadowLongPeriodTotalRange0 = 0;
  let ShadowLongPeriodTotalTrailRange0 = 0;
  let ShadowLongPeriodTotalRange1 = 0;
  let ShadowLongPeriodTotalTrailRange1 = 0;
  let FarPeriodTotalRange1 = 0;
  let FarPeriodTotalTrailRange1 = 0;
  let FarPeriodTotalRange2 = 0;
  let FarPeriodTotalTrailRange2 = 0;
  let NearPeriodTotalRange1 = 0;
  let NearPeriodTotalTrailRange1 = 0;
  let NearPeriodTotalRange2 = 0;
  let NearPeriodTotalTrailRange2 = 0;
  let BodyLongPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
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
  lookbackTotal = cdladvanceblockLookbackPositional(core);
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
  ShadowShortPeriodTotalRange1 = ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
  ShadowShortPeriodTotalTrailRange1 = ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[ShadowShortTrailingIdx - 1] - inOpen[ShadowShortTrailingIdx - 1])) : ((ShadowShort_rangeType === 1) ? (inHigh[ShadowShortTrailingIdx - 1] - inLow[ShadowShortTrailingIdx - 1]) : ((ShadowShort_rangeType === 2) ? ((inHigh[ShadowShortTrailingIdx - 1] - (((inClose[ShadowShortTrailingIdx - 1]) >= (inOpen[ShadowShortTrailingIdx - 1])) ? (inClose[ShadowShortTrailingIdx - 1]) : (inOpen[ShadowShortTrailingIdx - 1]))) + ((((inClose[ShadowShortTrailingIdx - 1]) >= (inOpen[ShadowShortTrailingIdx - 1])) ? (inOpen[ShadowShortTrailingIdx - 1]) : (inClose[ShadowShortTrailingIdx - 1])) - inLow[ShadowShortTrailingIdx - 1])) : 0.0)));
  ShadowShortPeriodTotalRange2 = ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
  ShadowShortPeriodTotalTrailRange2 = ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[ShadowShortTrailingIdx - 2] - inOpen[ShadowShortTrailingIdx - 2])) : ((ShadowShort_rangeType === 1) ? (inHigh[ShadowShortTrailingIdx - 2] - inLow[ShadowShortTrailingIdx - 2]) : ((ShadowShort_rangeType === 2) ? ((inHigh[ShadowShortTrailingIdx - 2] - (((inClose[ShadowShortTrailingIdx - 2]) >= (inOpen[ShadowShortTrailingIdx - 2])) ? (inClose[ShadowShortTrailingIdx - 2]) : (inOpen[ShadowShortTrailingIdx - 2]))) + ((((inClose[ShadowShortTrailingIdx - 2]) >= (inOpen[ShadowShortTrailingIdx - 2])) ? (inOpen[ShadowShortTrailingIdx - 2]) : (inClose[ShadowShortTrailingIdx - 2])) - inLow[ShadowShortTrailingIdx - 2])) : 0.0)));
  ShadowLongPeriodTotalRange1 = ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowLong_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
  ShadowLongPeriodTotalTrailRange1 = ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[ShadowLongTrailingIdx - 1] - inOpen[ShadowLongTrailingIdx - 1])) : ((ShadowLong_rangeType === 1) ? (inHigh[ShadowLongTrailingIdx - 1] - inLow[ShadowLongTrailingIdx - 1]) : ((ShadowLong_rangeType === 2) ? ((inHigh[ShadowLongTrailingIdx - 1] - (((inClose[ShadowLongTrailingIdx - 1]) >= (inOpen[ShadowLongTrailingIdx - 1])) ? (inClose[ShadowLongTrailingIdx - 1]) : (inOpen[ShadowLongTrailingIdx - 1]))) + ((((inClose[ShadowLongTrailingIdx - 1]) >= (inOpen[ShadowLongTrailingIdx - 1])) ? (inOpen[ShadowLongTrailingIdx - 1]) : (inClose[ShadowLongTrailingIdx - 1])) - inLow[ShadowLongTrailingIdx - 1])) : 0.0)));
  FarPeriodTotalRange2 = ((Far_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Far_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Far_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
  FarPeriodTotalTrailRange2 = ((Far_rangeType === 0) ? (Math.abs(inClose[FarTrailingIdx - 2] - inOpen[FarTrailingIdx - 2])) : ((Far_rangeType === 1) ? (inHigh[FarTrailingIdx - 2] - inLow[FarTrailingIdx - 2]) : ((Far_rangeType === 2) ? ((inHigh[FarTrailingIdx - 2] - (((inClose[FarTrailingIdx - 2]) >= (inOpen[FarTrailingIdx - 2])) ? (inClose[FarTrailingIdx - 2]) : (inOpen[FarTrailingIdx - 2]))) + ((((inClose[FarTrailingIdx - 2]) >= (inOpen[FarTrailingIdx - 2])) ? (inOpen[FarTrailingIdx - 2]) : (inClose[FarTrailingIdx - 2])) - inLow[FarTrailingIdx - 2])) : 0.0)));
  NearPeriodTotalRange2 = ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
  NearPeriodTotalTrailRange2 = ((Near_rangeType === 0) ? (Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : ((Near_rangeType === 1) ? (inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) : ((Near_rangeType === 2) ? ((inHigh[NearTrailingIdx - 2] - (((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inClose[NearTrailingIdx - 2]) : (inOpen[NearTrailingIdx - 2]))) + ((((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inOpen[NearTrailingIdx - 2]) : (inClose[NearTrailingIdx - 2])) - inLow[NearTrailingIdx - 2])) : 0.0)));
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
    ShadowShortPeriodTotalRange0 = ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    ShadowShortPeriodTotalTrailRange0 = ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[ShadowShortTrailingIdx] - inOpen[ShadowShortTrailingIdx])) : ((ShadowShort_rangeType === 1) ? (inHigh[ShadowShortTrailingIdx] - inLow[ShadowShortTrailingIdx]) : ((ShadowShort_rangeType === 2) ? ((inHigh[ShadowShortTrailingIdx] - (((inClose[ShadowShortTrailingIdx]) >= (inOpen[ShadowShortTrailingIdx])) ? (inClose[ShadowShortTrailingIdx]) : (inOpen[ShadowShortTrailingIdx]))) + ((((inClose[ShadowShortTrailingIdx]) >= (inOpen[ShadowShortTrailingIdx])) ? (inOpen[ShadowShortTrailingIdx]) : (inClose[ShadowShortTrailingIdx])) - inLow[ShadowShortTrailingIdx])) : 0.0)));
    ShadowShortPeriodTotal[2] = ShadowShortPeriodTotal[2] + (ShadowShortPeriodTotalRange2 - ShadowShortPeriodTotalTrailRange2);
    ShadowShortPeriodTotal[1] = ShadowShortPeriodTotal[1] + (ShadowShortPeriodTotalRange1 - ShadowShortPeriodTotalTrailRange1);
    ShadowShortPeriodTotal[0] = ShadowShortPeriodTotal[0] + (ShadowShortPeriodTotalRange0 - ShadowShortPeriodTotalTrailRange0);
    ShadowShortPeriodTotalRange2 = ShadowShortPeriodTotalRange1;
    ShadowShortPeriodTotalTrailRange2 = ShadowShortPeriodTotalTrailRange1;
    ShadowShortPeriodTotalRange1 = ShadowShortPeriodTotalRange0;
    ShadowShortPeriodTotalTrailRange1 = ShadowShortPeriodTotalTrailRange0;
    ShadowLongPeriodTotalRange0 = ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    ShadowLongPeriodTotalTrailRange0 = ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[ShadowLongTrailingIdx] - inOpen[ShadowLongTrailingIdx])) : ((ShadowLong_rangeType === 1) ? (inHigh[ShadowLongTrailingIdx] - inLow[ShadowLongTrailingIdx]) : ((ShadowLong_rangeType === 2) ? ((inHigh[ShadowLongTrailingIdx] - (((inClose[ShadowLongTrailingIdx]) >= (inOpen[ShadowLongTrailingIdx])) ? (inClose[ShadowLongTrailingIdx]) : (inOpen[ShadowLongTrailingIdx]))) + ((((inClose[ShadowLongTrailingIdx]) >= (inOpen[ShadowLongTrailingIdx])) ? (inOpen[ShadowLongTrailingIdx]) : (inClose[ShadowLongTrailingIdx])) - inLow[ShadowLongTrailingIdx])) : 0.0)));
    ShadowLongPeriodTotal[1] = ShadowLongPeriodTotal[1] + (ShadowLongPeriodTotalRange1 - ShadowLongPeriodTotalTrailRange1);
    ShadowLongPeriodTotal[0] = ShadowLongPeriodTotal[0] + (ShadowLongPeriodTotalRange0 - ShadowLongPeriodTotalTrailRange0);
    ShadowLongPeriodTotalRange1 = ShadowLongPeriodTotalRange0;
    ShadowLongPeriodTotalTrailRange1 = ShadowLongPeriodTotalTrailRange0;
    FarPeriodTotalRange1 = ((Far_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Far_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Far_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    FarPeriodTotalTrailRange1 = ((Far_rangeType === 0) ? (Math.abs(inClose[FarTrailingIdx - 1] - inOpen[FarTrailingIdx - 1])) : ((Far_rangeType === 1) ? (inHigh[FarTrailingIdx - 1] - inLow[FarTrailingIdx - 1]) : ((Far_rangeType === 2) ? ((inHigh[FarTrailingIdx - 1] - (((inClose[FarTrailingIdx - 1]) >= (inOpen[FarTrailingIdx - 1])) ? (inClose[FarTrailingIdx - 1]) : (inOpen[FarTrailingIdx - 1]))) + ((((inClose[FarTrailingIdx - 1]) >= (inOpen[FarTrailingIdx - 1])) ? (inOpen[FarTrailingIdx - 1]) : (inClose[FarTrailingIdx - 1])) - inLow[FarTrailingIdx - 1])) : 0.0)));
    FarPeriodTotal[2] = FarPeriodTotal[2] + (FarPeriodTotalRange2 - FarPeriodTotalTrailRange2);
    FarPeriodTotal[1] = FarPeriodTotal[1] + (FarPeriodTotalRange1 - FarPeriodTotalTrailRange1);
    FarPeriodTotalRange2 = FarPeriodTotalRange1;
    FarPeriodTotalTrailRange2 = FarPeriodTotalTrailRange1;
    NearPeriodTotalRange1 = ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    NearPeriodTotalTrailRange1 = ((Near_rangeType === 0) ? (Math.abs(inClose[NearTrailingIdx - 1] - inOpen[NearTrailingIdx - 1])) : ((Near_rangeType === 1) ? (inHigh[NearTrailingIdx - 1] - inLow[NearTrailingIdx - 1]) : ((Near_rangeType === 2) ? ((inHigh[NearTrailingIdx - 1] - (((inClose[NearTrailingIdx - 1]) >= (inOpen[NearTrailingIdx - 1])) ? (inClose[NearTrailingIdx - 1]) : (inOpen[NearTrailingIdx - 1]))) + ((((inClose[NearTrailingIdx - 1]) >= (inOpen[NearTrailingIdx - 1])) ? (inOpen[NearTrailingIdx - 1]) : (inClose[NearTrailingIdx - 1])) - inLow[NearTrailingIdx - 1])) : 0.0)));
    NearPeriodTotal[2] = NearPeriodTotal[2] + (NearPeriodTotalRange2 - NearPeriodTotalTrailRange2);
    NearPeriodTotal[1] = NearPeriodTotal[1] + (NearPeriodTotalRange1 - NearPeriodTotalTrailRange1);
    NearPeriodTotalRange2 = NearPeriodTotalRange1;
    NearPeriodTotalTrailRange2 = NearPeriodTotalTrailRange1;
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

/** @internal */
export function cdladvanceblockPositional(
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
  const guardStart = clampedStart('CDLADVANCEBLOCK', startIdx, cdladvanceblockLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLADVANCEBLOCK', 'prices.open', inOpen, guardInLen);
  requireLength('CDLADVANCEBLOCK', 'prices.high', inHigh, guardInLen);
  requireLength('CDLADVANCEBLOCK', 'prices.low', inLow, guardInLen);
  requireLength('CDLADVANCEBLOCK', 'prices.close', inClose, guardInLen);
  requireOutput('CDLADVANCEBLOCK', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLADVANCEBLOCK', 'outInteger', 'prices.open');
  }
  if (overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLADVANCEBLOCK', 'outInteger', 'prices.high');
  }
  if (overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLADVANCEBLOCK', 'outInteger', 'prices.low');
  }
  if (overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLADVANCEBLOCK', 'outInteger', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdladvanceblockImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLADVANCEBLOCK', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cdladvanceblock}, {@link cdladvanceblockLookback},
 * {@link cdladvanceblockDisplayShift}, {@link cdladvanceblockOpen} and
 * {@link cdladvanceblockOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface CdladvanceblockOptions {
  /**
   * First bar of the requested range (default 0). Bars before it are still read as
   * warm-up. Batch calls only.
   */
  startIdx?: number | undefined;
  /**
   * Last bar of the requested range, inclusive (default the last bar of the first
   * input). Batch calls only.
   */
  endIdx?: number | undefined;
  /**
   * Unstable periods and candle settings to compute with; `Core.DEFAULT` when
   * omitted.
   */
  core?: Core | undefined;
  /**
   * Buffer for `values`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input.
   */
  outInteger?: Int32Array | undefined;
}

/**
 * What {@link cdladvanceblock} returns, and {@link cdladvanceblockOpenAndFill}
 * with its stream.
 */
export interface CdladvanceblockResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * -100 on a detected pattern (always bearish), 0 otherwise; never emits +100.
   * `values[k]` is the value at input bar `begIdx + k`.
   */
  values: Int32Array;
}

/**
 * Number of leading input bars {@link cdladvanceblock} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdladvanceblockOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdladvanceblockLookback(options?: CdladvanceblockOptions): number {
  const o = optionsOf('CDLADVANCEBLOCK lookback', options);
  return cdladvanceblockLookbackPositional(coreOf('CDLADVANCEBLOCK lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdladvanceblock}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdladvanceblockOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdladvanceblockDisplayShift(outputIdx: number, options?: CdladvanceblockOptions): number {
  const o = optionsOf('CDLADVANCEBLOCK displayShift', options);
  return cdladvanceblockDisplayShiftPositional(outputIdx, coreOf('CDLADVANCEBLOCK displayShift', o.core));
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
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cdladvanceblockLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `open`: Open
 *   price of each bar. `high`: High price of each bar. `low`: Low price of each bar.
 *   `close`: Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link CdladvanceblockOptions}.
 * @returns `begIdx` and the values; see {@link CdladvanceblockResult}. Each output
 *   is a view of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, an input too short for the range, a buffer that is empty, too short or of
 *   the wrong type or a buffer sharing memory with a `Float64Array` input it is not.
 *   Checked before anything is written.
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
 * const { begIdx, values } = cdladvanceblock({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`cdl3whitesoldiers`](https://ta-lib.org/functions/cdl3whitesoldiers)
 * @see [`cdlstalledpattern`](https://ta-lib.org/functions/cdlstalledpattern)
 */
export function cdladvanceblock(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: CdladvanceblockOptions): CdladvanceblockResult {
  requireArgument('CDLADVANCEBLOCK', 'prices', prices);
  const o = optionsOf('CDLADVANCEBLOCK', options);
  if (o.endIdx === undefined) {
    requireSeries('CDLADVANCEBLOCK', 'prices.open', prices.open);
    requireSeries('CDLADVANCEBLOCK', 'prices.high', prices.high);
    requireSeries('CDLADVANCEBLOCK', 'prices.low', prices.low);
    requireSeries('CDLADVANCEBLOCK', 'prices.close', prices.close);
  }
  const core = coreOf('CDLADVANCEBLOCK', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cdladvanceblockLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const range = cdladvanceblockPositional(startIdx, endIdx, prices.open, prices.high, prices.low, prices.close, outInteger, core);
  return { begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
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
  ShadowShortPeriodTotalRange1: number = 0;
  /** @internal */
  ShadowShortPeriodTotalTrailRange1: number = 0;
  /** @internal */
  ShadowShortPeriodTotalRange2: number = 0;
  /** @internal */
  ShadowShortPeriodTotalTrailRange2: number = 0;
  /** @internal */
  ShadowLongPeriodTotalRange1: number = 0;
  /** @internal */
  ShadowLongPeriodTotalTrailRange1: number = 0;
  /** @internal */
  FarPeriodTotalRange2: number = 0;
  /** @internal */
  FarPeriodTotalTrailRange2: number = 0;
  /** @internal */
  NearPeriodTotalRange2: number = 0;
  /** @internal */
  NearPeriodTotalTrailRange2: number = 0;
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
  ringCap_FarTrailingIdx: number = 2;
  /** @internal */
  ringLag_FarTrailingIdx: number = 0;
  /** @internal */
  ring_FarTrailingIdx_derived: Float64Array = new Float64Array(2);
  /** @internal */
  ringPos_NearTrailingIdx: number = 0;
  /** @internal */
  ringCap_NearTrailingIdx: number = 2;
  /** @internal */
  ringLag_NearTrailingIdx: number = 0;
  /** @internal */
  ring_NearTrailingIdx_derived: Float64Array = new Float64Array(2);
  /** @internal */
  ringPos_ShadowLongTrailingIdx: number = 0;
  /** @internal */
  ringCap_ShadowLongTrailingIdx: number = 0;
  /** @internal */
  ring_ShadowLongTrailingIdx_derived: Float64Array = new Float64Array(1);
  /** @internal */
  ringPos_ShadowShortTrailingIdx: number = 0;
  /** @internal */
  ringCap_ShadowShortTrailingIdx: number = 0;
  /** @internal */
  ring_ShadowShortTrailingIdx_derived: Float64Array = new Float64Array(1);
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

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): CdladvanceblockStream {
    return new CdladvanceblockStream(core);
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
    c.ShadowShortPeriodTotalRange1 = this.ShadowShortPeriodTotalRange1;
    c.ShadowShortPeriodTotalTrailRange1 = this.ShadowShortPeriodTotalTrailRange1;
    c.ShadowShortPeriodTotalRange2 = this.ShadowShortPeriodTotalRange2;
    c.ShadowShortPeriodTotalTrailRange2 = this.ShadowShortPeriodTotalTrailRange2;
    c.ShadowLongPeriodTotalRange1 = this.ShadowLongPeriodTotalRange1;
    c.ShadowLongPeriodTotalTrailRange1 = this.ShadowLongPeriodTotalTrailRange1;
    c.FarPeriodTotalRange2 = this.FarPeriodTotalRange2;
    c.FarPeriodTotalTrailRange2 = this.FarPeriodTotalTrailRange2;
    c.NearPeriodTotalRange2 = this.NearPeriodTotalRange2;
    c.NearPeriodTotalTrailRange2 = this.NearPeriodTotalTrailRange2;
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
    c.ring_ShadowLongTrailingIdx_derived = this.ring_ShadowLongTrailingIdx_derived.slice();
    c.ringPos_ShadowShortTrailingIdx = this.ringPos_ShadowShortTrailingIdx;
    c.ringCap_ShadowShortTrailingIdx = this.ringCap_ShadowShortTrailingIdx;
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
  let ShadowShortPeriodTotalRange0 = 0;
  let ShadowShortPeriodTotalTrailRange0 = 0;
  let ShadowLongPeriodTotalRange0 = 0;
  let ShadowLongPeriodTotalTrailRange0 = 0;
  let FarPeriodTotalRange1 = 0;
  let FarPeriodTotalTrailRange1 = 0;
  let NearPeriodTotalRange1 = 0;
  let NearPeriodTotalTrailRange1 = 0;
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
  if (sp.ringCap_ShadowLongTrailingIdx === 0) {
    sp.ring_ShadowLongTrailingIdx_derived[0] = ((ShadowLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
  if (sp.ringCap_ShadowShortTrailingIdx === 0) {
    sp.ring_ShadowShortTrailingIdx_derived[0] = ((ShadowShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
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
  ShadowShortPeriodTotalRange0 = ((ShadowShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  ShadowShortPeriodTotalTrailRange0 = sp.ring_ShadowShortTrailingIdx_derived[sp.ringPos_ShadowShortTrailingIdx];
  sp.ShadowShortPeriodTotal[2] = sp.ShadowShortPeriodTotal[2] + (sp.ShadowShortPeriodTotalRange2 - sp.ShadowShortPeriodTotalTrailRange2);
  sp.ShadowShortPeriodTotal[1] = sp.ShadowShortPeriodTotal[1] + (sp.ShadowShortPeriodTotalRange1 - sp.ShadowShortPeriodTotalTrailRange1);
  sp.ShadowShortPeriodTotal[0] = sp.ShadowShortPeriodTotal[0] + (ShadowShortPeriodTotalRange0 - ShadowShortPeriodTotalTrailRange0);
  sp.ShadowShortPeriodTotalRange2 = sp.ShadowShortPeriodTotalRange1;
  sp.ShadowShortPeriodTotalTrailRange2 = sp.ShadowShortPeriodTotalTrailRange1;
  sp.ShadowShortPeriodTotalRange1 = ShadowShortPeriodTotalRange0;
  sp.ShadowShortPeriodTotalTrailRange1 = ShadowShortPeriodTotalTrailRange0;
  ShadowLongPeriodTotalRange0 = ((ShadowLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  ShadowLongPeriodTotalTrailRange0 = sp.ring_ShadowLongTrailingIdx_derived[sp.ringPos_ShadowLongTrailingIdx];
  sp.ShadowLongPeriodTotal[1] = sp.ShadowLongPeriodTotal[1] + (sp.ShadowLongPeriodTotalRange1 - sp.ShadowLongPeriodTotalTrailRange1);
  sp.ShadowLongPeriodTotal[0] = sp.ShadowLongPeriodTotal[0] + (ShadowLongPeriodTotalRange0 - ShadowLongPeriodTotalTrailRange0);
  sp.ShadowLongPeriodTotalRange1 = ShadowLongPeriodTotalRange0;
  sp.ShadowLongPeriodTotalTrailRange1 = ShadowLongPeriodTotalTrailRange0;
  FarPeriodTotalRange1 = ((Far_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Far_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Far_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)));
  FarPeriodTotalTrailRange1 = sp.ring_FarTrailingIdx_derived[(((sp.ringPos_FarTrailingIdx + sp.ringCap_FarTrailingIdx - sp.ringLag_FarTrailingIdx - 1) % sp.ringCap_FarTrailingIdx) | 0)];
  sp.FarPeriodTotal[2] = sp.FarPeriodTotal[2] + (sp.FarPeriodTotalRange2 - sp.FarPeriodTotalTrailRange2);
  sp.FarPeriodTotal[1] = sp.FarPeriodTotal[1] + (FarPeriodTotalRange1 - FarPeriodTotalTrailRange1);
  sp.FarPeriodTotalRange2 = FarPeriodTotalRange1;
  sp.FarPeriodTotalTrailRange2 = FarPeriodTotalTrailRange1;
  NearPeriodTotalRange1 = ((Near_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)));
  NearPeriodTotalTrailRange1 = sp.ring_NearTrailingIdx_derived[(((sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - 1) % sp.ringCap_NearTrailingIdx) | 0)];
  sp.NearPeriodTotal[2] = sp.NearPeriodTotal[2] + (sp.NearPeriodTotalRange2 - sp.NearPeriodTotalTrailRange2);
  sp.NearPeriodTotal[1] = sp.NearPeriodTotal[1] + (NearPeriodTotalRange1 - NearPeriodTotalTrailRange1);
  sp.NearPeriodTotalRange2 = NearPeriodTotalRange1;
  sp.NearPeriodTotalTrailRange2 = NearPeriodTotalTrailRange1;
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
  sp.ring_ShadowLongTrailingIdx_derived[sp.ringPos_ShadowLongTrailingIdx] = ((ShadowLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  sp.ringPos_ShadowLongTrailingIdx = sp.ringPos_ShadowLongTrailingIdx + 1;
  if (sp.ringPos_ShadowLongTrailingIdx >= sp.ringCap_ShadowLongTrailingIdx) {
    sp.ringPos_ShadowLongTrailingIdx = 0;
  }
  sp.ring_ShadowShortTrailingIdx_derived[sp.ringPos_ShadowShortTrailingIdx] = ((ShadowShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
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
  let ShadowShortPeriodTotalRange0 = 0;
  let ShadowShortPeriodTotalTrailRange0 = 0;
  let ShadowShortPeriodTotalRange1 = 0;
  let ShadowShortPeriodTotalTrailRange1 = 0;
  let ShadowShortPeriodTotalRange2 = 0;
  let ShadowShortPeriodTotalTrailRange2 = 0;
  let ShadowLongPeriodTotalRange0 = 0;
  let ShadowLongPeriodTotalTrailRange0 = 0;
  let ShadowLongPeriodTotalRange1 = 0;
  let ShadowLongPeriodTotalTrailRange1 = 0;
  let FarPeriodTotalRange1 = 0;
  let FarPeriodTotalTrailRange1 = 0;
  let FarPeriodTotalRange2 = 0;
  let FarPeriodTotalTrailRange2 = 0;
  let NearPeriodTotalRange1 = 0;
  let NearPeriodTotalTrailRange1 = 0;
  let NearPeriodTotalRange2 = 0;
  let NearPeriodTotalTrailRange2 = 0;
  let BodyLongPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let BodyLongTrailingIdx = 0;
  let ShadowShortTrailingIdx = 0;
  let ShadowLongTrailingIdx = 0;
  let NearTrailingIdx = 0;
  let FarTrailingIdx = 0;
  let lookbackTotal = 0;
  const historyLen = inOpen.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
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
  lookbackTotal = cdladvanceblockLookbackPositional(core);
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
  ShadowShortPeriodTotalRange1 = ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
  ShadowShortPeriodTotalTrailRange1 = ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[ShadowShortTrailingIdx - 1] - inOpen[ShadowShortTrailingIdx - 1])) : ((ShadowShort_rangeType === 1) ? (inHigh[ShadowShortTrailingIdx - 1] - inLow[ShadowShortTrailingIdx - 1]) : ((ShadowShort_rangeType === 2) ? ((inHigh[ShadowShortTrailingIdx - 1] - (((inClose[ShadowShortTrailingIdx - 1]) >= (inOpen[ShadowShortTrailingIdx - 1])) ? (inClose[ShadowShortTrailingIdx - 1]) : (inOpen[ShadowShortTrailingIdx - 1]))) + ((((inClose[ShadowShortTrailingIdx - 1]) >= (inOpen[ShadowShortTrailingIdx - 1])) ? (inOpen[ShadowShortTrailingIdx - 1]) : (inClose[ShadowShortTrailingIdx - 1])) - inLow[ShadowShortTrailingIdx - 1])) : 0.0)));
  ShadowShortPeriodTotalRange2 = ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
  ShadowShortPeriodTotalTrailRange2 = ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[ShadowShortTrailingIdx - 2] - inOpen[ShadowShortTrailingIdx - 2])) : ((ShadowShort_rangeType === 1) ? (inHigh[ShadowShortTrailingIdx - 2] - inLow[ShadowShortTrailingIdx - 2]) : ((ShadowShort_rangeType === 2) ? ((inHigh[ShadowShortTrailingIdx - 2] - (((inClose[ShadowShortTrailingIdx - 2]) >= (inOpen[ShadowShortTrailingIdx - 2])) ? (inClose[ShadowShortTrailingIdx - 2]) : (inOpen[ShadowShortTrailingIdx - 2]))) + ((((inClose[ShadowShortTrailingIdx - 2]) >= (inOpen[ShadowShortTrailingIdx - 2])) ? (inOpen[ShadowShortTrailingIdx - 2]) : (inClose[ShadowShortTrailingIdx - 2])) - inLow[ShadowShortTrailingIdx - 2])) : 0.0)));
  ShadowLongPeriodTotalRange1 = ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowLong_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
  ShadowLongPeriodTotalTrailRange1 = ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[ShadowLongTrailingIdx - 1] - inOpen[ShadowLongTrailingIdx - 1])) : ((ShadowLong_rangeType === 1) ? (inHigh[ShadowLongTrailingIdx - 1] - inLow[ShadowLongTrailingIdx - 1]) : ((ShadowLong_rangeType === 2) ? ((inHigh[ShadowLongTrailingIdx - 1] - (((inClose[ShadowLongTrailingIdx - 1]) >= (inOpen[ShadowLongTrailingIdx - 1])) ? (inClose[ShadowLongTrailingIdx - 1]) : (inOpen[ShadowLongTrailingIdx - 1]))) + ((((inClose[ShadowLongTrailingIdx - 1]) >= (inOpen[ShadowLongTrailingIdx - 1])) ? (inOpen[ShadowLongTrailingIdx - 1]) : (inClose[ShadowLongTrailingIdx - 1])) - inLow[ShadowLongTrailingIdx - 1])) : 0.0)));
  FarPeriodTotalRange2 = ((Far_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Far_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Far_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
  FarPeriodTotalTrailRange2 = ((Far_rangeType === 0) ? (Math.abs(inClose[FarTrailingIdx - 2] - inOpen[FarTrailingIdx - 2])) : ((Far_rangeType === 1) ? (inHigh[FarTrailingIdx - 2] - inLow[FarTrailingIdx - 2]) : ((Far_rangeType === 2) ? ((inHigh[FarTrailingIdx - 2] - (((inClose[FarTrailingIdx - 2]) >= (inOpen[FarTrailingIdx - 2])) ? (inClose[FarTrailingIdx - 2]) : (inOpen[FarTrailingIdx - 2]))) + ((((inClose[FarTrailingIdx - 2]) >= (inOpen[FarTrailingIdx - 2])) ? (inOpen[FarTrailingIdx - 2]) : (inClose[FarTrailingIdx - 2])) - inLow[FarTrailingIdx - 2])) : 0.0)));
  NearPeriodTotalRange2 = ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
  NearPeriodTotalTrailRange2 = ((Near_rangeType === 0) ? (Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : ((Near_rangeType === 1) ? (inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) : ((Near_rangeType === 2) ? ((inHigh[NearTrailingIdx - 2] - (((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inClose[NearTrailingIdx - 2]) : (inOpen[NearTrailingIdx - 2]))) + ((((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inOpen[NearTrailingIdx - 2]) : (inClose[NearTrailingIdx - 2])) - inLow[NearTrailingIdx - 2])) : 0.0)));
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
    ShadowShortPeriodTotalRange0 = ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    ShadowShortPeriodTotalTrailRange0 = ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[ShadowShortTrailingIdx] - inOpen[ShadowShortTrailingIdx])) : ((ShadowShort_rangeType === 1) ? (inHigh[ShadowShortTrailingIdx] - inLow[ShadowShortTrailingIdx]) : ((ShadowShort_rangeType === 2) ? ((inHigh[ShadowShortTrailingIdx] - (((inClose[ShadowShortTrailingIdx]) >= (inOpen[ShadowShortTrailingIdx])) ? (inClose[ShadowShortTrailingIdx]) : (inOpen[ShadowShortTrailingIdx]))) + ((((inClose[ShadowShortTrailingIdx]) >= (inOpen[ShadowShortTrailingIdx])) ? (inOpen[ShadowShortTrailingIdx]) : (inClose[ShadowShortTrailingIdx])) - inLow[ShadowShortTrailingIdx])) : 0.0)));
    ShadowShortPeriodTotal[2] = ShadowShortPeriodTotal[2] + (ShadowShortPeriodTotalRange2 - ShadowShortPeriodTotalTrailRange2);
    ShadowShortPeriodTotal[1] = ShadowShortPeriodTotal[1] + (ShadowShortPeriodTotalRange1 - ShadowShortPeriodTotalTrailRange1);
    ShadowShortPeriodTotal[0] = ShadowShortPeriodTotal[0] + (ShadowShortPeriodTotalRange0 - ShadowShortPeriodTotalTrailRange0);
    ShadowShortPeriodTotalRange2 = ShadowShortPeriodTotalRange1;
    ShadowShortPeriodTotalTrailRange2 = ShadowShortPeriodTotalTrailRange1;
    ShadowShortPeriodTotalRange1 = ShadowShortPeriodTotalRange0;
    ShadowShortPeriodTotalTrailRange1 = ShadowShortPeriodTotalTrailRange0;
    ShadowLongPeriodTotalRange0 = ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    ShadowLongPeriodTotalTrailRange0 = ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[ShadowLongTrailingIdx] - inOpen[ShadowLongTrailingIdx])) : ((ShadowLong_rangeType === 1) ? (inHigh[ShadowLongTrailingIdx] - inLow[ShadowLongTrailingIdx]) : ((ShadowLong_rangeType === 2) ? ((inHigh[ShadowLongTrailingIdx] - (((inClose[ShadowLongTrailingIdx]) >= (inOpen[ShadowLongTrailingIdx])) ? (inClose[ShadowLongTrailingIdx]) : (inOpen[ShadowLongTrailingIdx]))) + ((((inClose[ShadowLongTrailingIdx]) >= (inOpen[ShadowLongTrailingIdx])) ? (inOpen[ShadowLongTrailingIdx]) : (inClose[ShadowLongTrailingIdx])) - inLow[ShadowLongTrailingIdx])) : 0.0)));
    ShadowLongPeriodTotal[1] = ShadowLongPeriodTotal[1] + (ShadowLongPeriodTotalRange1 - ShadowLongPeriodTotalTrailRange1);
    ShadowLongPeriodTotal[0] = ShadowLongPeriodTotal[0] + (ShadowLongPeriodTotalRange0 - ShadowLongPeriodTotalTrailRange0);
    ShadowLongPeriodTotalRange1 = ShadowLongPeriodTotalRange0;
    ShadowLongPeriodTotalTrailRange1 = ShadowLongPeriodTotalTrailRange0;
    FarPeriodTotalRange1 = ((Far_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Far_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Far_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    FarPeriodTotalTrailRange1 = ((Far_rangeType === 0) ? (Math.abs(inClose[FarTrailingIdx - 1] - inOpen[FarTrailingIdx - 1])) : ((Far_rangeType === 1) ? (inHigh[FarTrailingIdx - 1] - inLow[FarTrailingIdx - 1]) : ((Far_rangeType === 2) ? ((inHigh[FarTrailingIdx - 1] - (((inClose[FarTrailingIdx - 1]) >= (inOpen[FarTrailingIdx - 1])) ? (inClose[FarTrailingIdx - 1]) : (inOpen[FarTrailingIdx - 1]))) + ((((inClose[FarTrailingIdx - 1]) >= (inOpen[FarTrailingIdx - 1])) ? (inOpen[FarTrailingIdx - 1]) : (inClose[FarTrailingIdx - 1])) - inLow[FarTrailingIdx - 1])) : 0.0)));
    FarPeriodTotal[2] = FarPeriodTotal[2] + (FarPeriodTotalRange2 - FarPeriodTotalTrailRange2);
    FarPeriodTotal[1] = FarPeriodTotal[1] + (FarPeriodTotalRange1 - FarPeriodTotalTrailRange1);
    FarPeriodTotalRange2 = FarPeriodTotalRange1;
    FarPeriodTotalTrailRange2 = FarPeriodTotalTrailRange1;
    NearPeriodTotalRange1 = ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    NearPeriodTotalTrailRange1 = ((Near_rangeType === 0) ? (Math.abs(inClose[NearTrailingIdx - 1] - inOpen[NearTrailingIdx - 1])) : ((Near_rangeType === 1) ? (inHigh[NearTrailingIdx - 1] - inLow[NearTrailingIdx - 1]) : ((Near_rangeType === 2) ? ((inHigh[NearTrailingIdx - 1] - (((inClose[NearTrailingIdx - 1]) >= (inOpen[NearTrailingIdx - 1])) ? (inClose[NearTrailingIdx - 1]) : (inOpen[NearTrailingIdx - 1]))) + ((((inClose[NearTrailingIdx - 1]) >= (inOpen[NearTrailingIdx - 1])) ? (inOpen[NearTrailingIdx - 1]) : (inClose[NearTrailingIdx - 1])) - inLow[NearTrailingIdx - 1])) : 0.0)));
    NearPeriodTotal[2] = NearPeriodTotal[2] + (NearPeriodTotalRange2 - NearPeriodTotalTrailRange2);
    NearPeriodTotal[1] = NearPeriodTotal[1] + (NearPeriodTotalRange1 - NearPeriodTotalTrailRange1);
    NearPeriodTotalRange2 = NearPeriodTotalRange1;
    NearPeriodTotalTrailRange2 = NearPeriodTotalTrailRange1;
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
  const cap_FarTrailingIdx = capLag_FarTrailingIdx + 2;
  if (capLag_FarTrailingIdx < 0 || cap_FarTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_FarTrailingIdx = cap_FarTrailingIdx > 0 ? cap_FarTrailingIdx : 1;
  const capRing_FarTrailingIdx_derived = new Float64Array(allocN_FarTrailingIdx);
  for (let fillJ = historyLen - cap_FarTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_FarTrailingIdx_derived[fillJ % cap_FarTrailingIdx] = ((Far_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((Far_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((Far_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
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
  const cap_ShadowLongTrailingIdx = i - ShadowLongTrailingIdx;
  if (cap_ShadowLongTrailingIdx < 0 || cap_ShadowLongTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_ShadowLongTrailingIdx = cap_ShadowLongTrailingIdx > 0 ? cap_ShadowLongTrailingIdx : 1;
  const capRing_ShadowLongTrailingIdx_derived = new Float64Array(allocN_ShadowLongTrailingIdx);
  for (let fillJ = historyLen - cap_ShadowLongTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_ShadowLongTrailingIdx_derived[fillJ - (historyLen - cap_ShadowLongTrailingIdx)] = ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((ShadowLong_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((ShadowLong_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  const cap_ShadowShortTrailingIdx = i - ShadowShortTrailingIdx;
  if (cap_ShadowShortTrailingIdx < 0 || cap_ShadowShortTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_ShadowShortTrailingIdx = cap_ShadowShortTrailingIdx > 0 ? cap_ShadowShortTrailingIdx : 1;
  const capRing_ShadowShortTrailingIdx_derived = new Float64Array(allocN_ShadowShortTrailingIdx);
  for (let fillJ = historyLen - cap_ShadowShortTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_ShadowShortTrailingIdx_derived[fillJ - (historyLen - cap_ShadowShortTrailingIdx)] = ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((ShadowShort_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((ShadowShort_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  sp.ShadowShortPeriodTotal = ShadowShortPeriodTotal;
  sp.ShadowLongPeriodTotal = ShadowLongPeriodTotal;
  sp.NearPeriodTotal = NearPeriodTotal;
  sp.FarPeriodTotal = FarPeriodTotal;
  sp.ShadowShortPeriodTotalRange1 = ShadowShortPeriodTotalRange1;
  sp.ShadowShortPeriodTotalTrailRange1 = ShadowShortPeriodTotalTrailRange1;
  sp.ShadowShortPeriodTotalRange2 = ShadowShortPeriodTotalRange2;
  sp.ShadowShortPeriodTotalTrailRange2 = ShadowShortPeriodTotalTrailRange2;
  sp.ShadowLongPeriodTotalRange1 = ShadowLongPeriodTotalRange1;
  sp.ShadowLongPeriodTotalTrailRange1 = ShadowLongPeriodTotalTrailRange1;
  sp.FarPeriodTotalRange2 = FarPeriodTotalRange2;
  sp.FarPeriodTotalTrailRange2 = FarPeriodTotalTrailRange2;
  sp.NearPeriodTotalRange2 = NearPeriodTotalRange2;
  sp.NearPeriodTotalTrailRange2 = NearPeriodTotalTrailRange2;
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
  sp.ringPos_ShadowLongTrailingIdx = 0;
  sp.ringCap_ShadowLongTrailingIdx = cap_ShadowLongTrailingIdx;
  sp.ring_ShadowLongTrailingIdx_derived = capRing_ShadowLongTrailingIdx_derived;
  sp.ringPos_ShadowShortTrailingIdx = 0;
  sp.ringCap_ShadowShortTrailingIdx = cap_ShadowShortTrailingIdx;
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
  const sp = CdladvanceblockStream.of(core);
  const retCode = cdladvanceblockOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLADVANCEBLOCK openAndFill', inOpen.length, startIdx, cdladvanceblockLookbackPositional(core));
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
  const sp = CdladvanceblockStream.of(core);
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
    throw insufficientHistory('CDLADVANCEBLOCK open', inOpen.length, startIdx, cdladvanceblockLookbackPositional(core));
  }
  throw streamFailure('CDLADVANCEBLOCK open', retCode);
}

/** @internal */
export function cdladvanceblockOpenPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): CdladvanceblockStream {
  requireArgument('CDLADVANCEBLOCK open', 'prices.open', inOpen);
  requireHistory('CDLADVANCEBLOCK open', inOpen.length);
  requireArgument('CDLADVANCEBLOCK open', 'prices.high', inHigh);
  requireArgument('CDLADVANCEBLOCK open', 'prices.low', inLow);
  requireArgument('CDLADVANCEBLOCK open', 'prices.close', inClose);
  requireHistoryLength('CDLADVANCEBLOCK open', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLADVANCEBLOCK open', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLADVANCEBLOCK open', 'prices.close', inClose.length, inOpen.length);
  return cdladvanceblockOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/** @internal */
export function cdladvanceblockOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): CdladvanceblockStream {
  requireArgument('CDLADVANCEBLOCK openAndFill', 'prices.open', inOpen);
  requireHistory('CDLADVANCEBLOCK openAndFill', inOpen.length);
  requireArgument('CDLADVANCEBLOCK openAndFill', 'prices.high', inHigh);
  requireArgument('CDLADVANCEBLOCK openAndFill', 'prices.low', inLow);
  requireArgument('CDLADVANCEBLOCK openAndFill', 'prices.close', inClose);
  const guardOutLen = openFillCount('CDLADVANCEBLOCK openAndFill', inOpen.length, cdladvanceblockLookbackPositional(core));
  requireHistoryLength('CDLADVANCEBLOCK openAndFill', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLADVANCEBLOCK openAndFill', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLADVANCEBLOCK openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('CDLADVANCEBLOCK openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('CDLADVANCEBLOCK openAndFill', 'outInteger', 'prices.open');
  }
  if (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CDLADVANCEBLOCK openAndFill', 'outInteger', 'prices.high');
  }
  if (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CDLADVANCEBLOCK openAndFill', 'outInteger', 'prices.low');
  }
  if (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('CDLADVANCEBLOCK openAndFill', 'outInteger', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLADVANCEBLOCK openAndFill', 'outInteger', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLADVANCEBLOCK openAndFill', 'outInteger', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLADVANCEBLOCK openAndFill', 'outInteger', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLADVANCEBLOCK openAndFill', 'outInteger', 'prices.close');
  }
  return cdladvanceblockOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}

/**
 * Opens a live CDLADVANCEBLOCK stream over the warm-up history: `value()` starts
 * at the last history bar, bit-identical to {@link cdladvanceblock} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `open`: Open price of each bar. `high`: High price of each bar. `low`:
 *   Low price of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdladvanceblockOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdladvanceblockLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input or histories of different lengths.
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
 * const stream = cdladvanceblockOpen({ open, high, low, close });
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdladvanceblockOpen(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: CdladvanceblockOptions): CdladvanceblockStream {
  requireArgument('CDLADVANCEBLOCK open', 'prices', prices);
  const o = optionsOf('CDLADVANCEBLOCK open', options);
  const core = coreOf('CDLADVANCEBLOCK open', o.core);
  return cdladvanceblockOpenPositional(prices.open, prices.high, prices.low, prices.close, core);
}

/**
 * {@link cdladvanceblockOpen} that also computes the outputs over the whole
 * history in the same pass, bit-identically to {@link cdladvanceblock}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `open`: Open price of each bar. `high`: High price of each bar. `low`:
 *   Low price of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CdladvanceblockOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cdladvanceblock} returns them; the range is
 *   the stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdladvanceblockLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, histories of different lengths or a buffer too short,
 *   of the wrong type or starting where another buffer or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   buffer or a `Float64Array` input.
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
 * const { stream, begIdx, values } = cdladvanceblockOpenAndFill({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cdladvanceblockOpenAndFill(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: CdladvanceblockOptions): CdladvanceblockResult & { stream: CdladvanceblockStream } {
  requireArgument('CDLADVANCEBLOCK openAndFill', 'prices', prices);
  const o = optionsOf('CDLADVANCEBLOCK openAndFill', options);
  const core = coreOf('CDLADVANCEBLOCK openAndFill', o.core);
  const capacity = fillCapacity(cdladvanceblockLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const stream = cdladvanceblockOpenAndFillPositional(prices.open, prices.high, prices.low, prices.close, outInteger, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}
