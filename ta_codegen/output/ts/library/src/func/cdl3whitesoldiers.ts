/*! TA-LIB Copyright (c) 1999-2026, Mario Fortier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither name of author nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


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
 *  103126 MF,CC  Pre-check the cheap tests in an outer if, the candle averages inside
 *  110526 MF,CC  Carry the ranges of the previous bars instead of recomputing them
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { CandleSettingType, Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function cdl3whitesoldiersLookbackPositional(core: Core = Core.DEFAULT): number {
  const BodyShort_avgPeriod = core.candleSettings[CandleSettingType.BODY_SHORT].avgPeriod;
  const Far_avgPeriod = core.candleSettings[CandleSettingType.FAR].avgPeriod;
  const Near_avgPeriod = core.candleSettings[CandleSettingType.NEAR].avgPeriod;
  const ShadowVeryShort_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].avgPeriod;
  return ((ShadowVeryShort_avgPeriod > BodyShort_avgPeriod ? ShadowVeryShort_avgPeriod : BodyShort_avgPeriod) > (Far_avgPeriod > Near_avgPeriod ? Far_avgPeriod : Near_avgPeriod) ? (ShadowVeryShort_avgPeriod > BodyShort_avgPeriod ? ShadowVeryShort_avgPeriod : BodyShort_avgPeriod) : (Far_avgPeriod > Near_avgPeriod ? Far_avgPeriod : Near_avgPeriod)) + 2;
}

/** @internal */
export function cdl3whitesoldiersDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdl3whitesoldiersImpl(
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
  const NearPeriodTotal = new Float64Array(3);
  const FarPeriodTotal = new Float64Array(3);
  let ShadowVeryShortPeriodTotalRange0 = 0;
  let ShadowVeryShortPeriodTotalTrailRange0 = 0;
  let ShadowVeryShortPeriodTotalRange1 = 0;
  let ShadowVeryShortPeriodTotalTrailRange1 = 0;
  let ShadowVeryShortPeriodTotalRange2 = 0;
  let ShadowVeryShortPeriodTotalTrailRange2 = 0;
  let FarPeriodTotalRange1 = 0;
  let FarPeriodTotalTrailRange1 = 0;
  let FarPeriodTotalRange2 = 0;
  let FarPeriodTotalTrailRange2 = 0;
  let NearPeriodTotalRange1 = 0;
  let NearPeriodTotalTrailRange1 = 0;
  let NearPeriodTotalRange2 = 0;
  let NearPeriodTotalTrailRange2 = 0;
  let BodyShortPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let ShadowVeryShortTrailingIdx = 0;
  let NearTrailingIdx = 0;
  let FarTrailingIdx = 0;
  let BodyShortTrailingIdx = 0;
  let lookbackTotal = 0;
  const BodyShort_rangeType: number = core.candleSettings[CandleSettingType.BODY_SHORT].rangeType;
  const BodyShort_avgPeriod = core.candleSettings[CandleSettingType.BODY_SHORT].avgPeriod;
  const BodyShort_factor = core.candleSettings[CandleSettingType.BODY_SHORT].factor;
  const Far_rangeType: number = core.candleSettings[CandleSettingType.FAR].rangeType;
  const Far_avgPeriod = core.candleSettings[CandleSettingType.FAR].avgPeriod;
  const Far_factor = core.candleSettings[CandleSettingType.FAR].factor;
  const Near_rangeType: number = core.candleSettings[CandleSettingType.NEAR].rangeType;
  const Near_avgPeriod = core.candleSettings[CandleSettingType.NEAR].avgPeriod;
  const Near_factor = core.candleSettings[CandleSettingType.NEAR].factor;
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
  lookbackTotal = cdl3whitesoldiersLookbackPositional(core);
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
  NearPeriodTotal[2] = 0;
  NearPeriodTotal[1] = 0;
  NearPeriodTotal[0] = 0;
  NearTrailingIdx = startIdx - Near_avgPeriod;
  FarPeriodTotal[2] = 0;
  FarPeriodTotal[1] = 0;
  FarPeriodTotal[0] = 0;
  FarTrailingIdx = startIdx - Far_avgPeriod;
  BodyShortPeriodTotal = 0;
  BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
  i = ShadowVeryShortTrailingIdx;
  while (i < startIdx) {
    ShadowVeryShortPeriodTotal[2] = ShadowVeryShortPeriodTotal[2] + ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
    ShadowVeryShortPeriodTotal[1] = ShadowVeryShortPeriodTotal[1] + ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    ShadowVeryShortPeriodTotal[0] = ShadowVeryShortPeriodTotal[0] + ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
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
  i = BodyShortTrailingIdx;
  while (i < startIdx) {
    BodyShortPeriodTotal += ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - three white candlesticks with consecutively higher closes
   * - Greg Morris wants them to be long, Steve Nison doesn't; anyway they should not be short
   * - each candle opens within or near the previous white real body
   * - each candle must have no or very short upper shadow
   * - to differentiate this pattern from advance block, each candle must not be far shorter than the prior candle
   * The meanings of "not short", "very short shadow", "far" and "near" are specified with TA_SetCandleSettings;
   * here the 3 candles must be not short, if you want them to be long use TA_SetCandleSettings on BodyShort
   */
  outIdx = 0;
  ShadowVeryShortPeriodTotalRange1 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
  ShadowVeryShortPeriodTotalTrailRange1 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx - 1] - inOpen[ShadowVeryShortTrailingIdx - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[ShadowVeryShortTrailingIdx - 1] - inLow[ShadowVeryShortTrailingIdx - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[ShadowVeryShortTrailingIdx - 1] - (((inClose[ShadowVeryShortTrailingIdx - 1]) >= (inOpen[ShadowVeryShortTrailingIdx - 1])) ? (inClose[ShadowVeryShortTrailingIdx - 1]) : (inOpen[ShadowVeryShortTrailingIdx - 1]))) + ((((inClose[ShadowVeryShortTrailingIdx - 1]) >= (inOpen[ShadowVeryShortTrailingIdx - 1])) ? (inOpen[ShadowVeryShortTrailingIdx - 1]) : (inClose[ShadowVeryShortTrailingIdx - 1])) - inLow[ShadowVeryShortTrailingIdx - 1])) : 0.0)));
  ShadowVeryShortPeriodTotalRange2 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
  ShadowVeryShortPeriodTotalTrailRange2 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx - 2] - inOpen[ShadowVeryShortTrailingIdx - 2])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[ShadowVeryShortTrailingIdx - 2] - inLow[ShadowVeryShortTrailingIdx - 2]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[ShadowVeryShortTrailingIdx - 2] - (((inClose[ShadowVeryShortTrailingIdx - 2]) >= (inOpen[ShadowVeryShortTrailingIdx - 2])) ? (inClose[ShadowVeryShortTrailingIdx - 2]) : (inOpen[ShadowVeryShortTrailingIdx - 2]))) + ((((inClose[ShadowVeryShortTrailingIdx - 2]) >= (inOpen[ShadowVeryShortTrailingIdx - 2])) ? (inOpen[ShadowVeryShortTrailingIdx - 2]) : (inClose[ShadowVeryShortTrailingIdx - 2])) - inLow[ShadowVeryShortTrailingIdx - 2])) : 0.0)));
  FarPeriodTotalRange2 = ((Far_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Far_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Far_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
  FarPeriodTotalTrailRange2 = ((Far_rangeType === 0) ? (Math.abs(inClose[FarTrailingIdx - 2] - inOpen[FarTrailingIdx - 2])) : ((Far_rangeType === 1) ? (inHigh[FarTrailingIdx - 2] - inLow[FarTrailingIdx - 2]) : ((Far_rangeType === 2) ? ((inHigh[FarTrailingIdx - 2] - (((inClose[FarTrailingIdx - 2]) >= (inOpen[FarTrailingIdx - 2])) ? (inClose[FarTrailingIdx - 2]) : (inOpen[FarTrailingIdx - 2]))) + ((((inClose[FarTrailingIdx - 2]) >= (inOpen[FarTrailingIdx - 2])) ? (inOpen[FarTrailingIdx - 2]) : (inClose[FarTrailingIdx - 2])) - inLow[FarTrailingIdx - 2])) : 0.0)));
  NearPeriodTotalRange2 = ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
  NearPeriodTotalTrailRange2 = ((Near_rangeType === 0) ? (Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : ((Near_rangeType === 1) ? (inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) : ((Near_rangeType === 2) ? ((inHigh[NearTrailingIdx - 2] - (((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inClose[NearTrailingIdx - 2]) : (inOpen[NearTrailingIdx - 2]))) + ((((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inOpen[NearTrailingIdx - 2]) : (inClose[NearTrailingIdx - 2])) - inLow[NearTrailingIdx - 2])) : 0.0)));
  do {
    if (((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === 1 && /* 1st white */
        ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === 1 && /* 2nd white */
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === 1 &&         /* 3rd white */
        (inHigh[i - 2] - ((inClose[i - 2] >= inOpen[i - 2]) ? inClose[i - 2] : inOpen[i - 2])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short upper shadow */
        (inHigh[i - 1] - ((inClose[i - 1] >= inOpen[i - 1]) ? inClose[i - 1] : inOpen[i - 1])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (ShadowVeryShortPeriodTotal[1] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short upper shadow */
        (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (ShadowVeryShortPeriodTotal[0] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short upper shadow */
        inClose[i] > inClose[i - 1] &&
        inClose[i - 1] > inClose[i - 2] &&                    /* consecutive higher closes */
        inOpen[i - 1] > inOpen[i - 2] &&                      /* 2nd opens within/near 1st real body */
        inOpen[i - 1] <= inClose[i - 2] + ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
        inOpen[i] > inOpen[i - 1] &&                          /* 3rd opens within/near 2nd real body */
        inOpen[i] <= inClose[i - 1] + ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
        Math.abs(inClose[i - 1] - inOpen[i - 1]) > Math.abs(inClose[i - 2] - inOpen[i - 2]) - ((Far_factor * (((Far_avgPeriod !== 0) ? (FarPeriodTotal[2] / Far_avgPeriod) : ((Far_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Far_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Far_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Far_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd not far shorter than 1st */
        Math.abs(inClose[i] - inOpen[i]) > Math.abs(inClose[i - 1] - inOpen[i - 1]) - ((Far_factor * (((Far_avgPeriod !== 0) ? (FarPeriodTotal[1] / Far_avgPeriod) : ((Far_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Far_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Far_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Far_rangeType === 2) ? 2.0 : 1.0)))) && /* 3rd not far shorter than 2nd */
        Math.abs(inClose[i] - inOpen[i]) > ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0))))) /* not short real body */
    {
      outInteger[outIdx++] = 100;
    } else {
      outInteger[outIdx++] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    ShadowVeryShortPeriodTotalRange0 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    ShadowVeryShortPeriodTotalTrailRange0 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx] - inOpen[ShadowVeryShortTrailingIdx])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[ShadowVeryShortTrailingIdx] - inLow[ShadowVeryShortTrailingIdx]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[ShadowVeryShortTrailingIdx] - (((inClose[ShadowVeryShortTrailingIdx]) >= (inOpen[ShadowVeryShortTrailingIdx])) ? (inClose[ShadowVeryShortTrailingIdx]) : (inOpen[ShadowVeryShortTrailingIdx]))) + ((((inClose[ShadowVeryShortTrailingIdx]) >= (inOpen[ShadowVeryShortTrailingIdx])) ? (inOpen[ShadowVeryShortTrailingIdx]) : (inClose[ShadowVeryShortTrailingIdx])) - inLow[ShadowVeryShortTrailingIdx])) : 0.0)));
    ShadowVeryShortPeriodTotal[2] = ShadowVeryShortPeriodTotal[2] + (ShadowVeryShortPeriodTotalRange2 - ShadowVeryShortPeriodTotalTrailRange2);
    ShadowVeryShortPeriodTotal[1] = ShadowVeryShortPeriodTotal[1] + (ShadowVeryShortPeriodTotalRange1 - ShadowVeryShortPeriodTotalTrailRange1);
    ShadowVeryShortPeriodTotal[0] = ShadowVeryShortPeriodTotal[0] + (ShadowVeryShortPeriodTotalRange0 - ShadowVeryShortPeriodTotalTrailRange0);
    ShadowVeryShortPeriodTotalRange2 = ShadowVeryShortPeriodTotalRange1;
    ShadowVeryShortPeriodTotalTrailRange2 = ShadowVeryShortPeriodTotalTrailRange1;
    ShadowVeryShortPeriodTotalRange1 = ShadowVeryShortPeriodTotalRange0;
    ShadowVeryShortPeriodTotalTrailRange1 = ShadowVeryShortPeriodTotalTrailRange0;
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
    BodyShortPeriodTotal += ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyShort_rangeType === 0) ? (Math.abs(inClose[BodyShortTrailingIdx] - inOpen[BodyShortTrailingIdx])) : ((BodyShort_rangeType === 1) ? (inHigh[BodyShortTrailingIdx] - inLow[BodyShortTrailingIdx]) : ((BodyShort_rangeType === 2) ? ((inHigh[BodyShortTrailingIdx] - (((inClose[BodyShortTrailingIdx]) >= (inOpen[BodyShortTrailingIdx])) ? (inClose[BodyShortTrailingIdx]) : (inOpen[BodyShortTrailingIdx]))) + ((((inClose[BodyShortTrailingIdx]) >= (inOpen[BodyShortTrailingIdx])) ? (inOpen[BodyShortTrailingIdx]) : (inClose[BodyShortTrailingIdx])) - inLow[BodyShortTrailingIdx])) : 0.0)));
    i += 1;
    ShadowVeryShortTrailingIdx += 1;
    NearTrailingIdx += 1;
    FarTrailingIdx += 1;
    BodyShortTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function cdl3whitesoldiersPositional(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDL3WHITESOLDIERS', 'startIdx', startIdx);
  endIdx = requireInt32('CDL3WHITESOLDIERS', 'endIdx', endIdx);
  requireIndexRange('CDL3WHITESOLDIERS', startIdx, endIdx);
  const guardStart = clampedStart('CDL3WHITESOLDIERS', startIdx, cdl3whitesoldiersLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDL3WHITESOLDIERS', 'prices.open', inOpen, guardInLen);
  requireLength('CDL3WHITESOLDIERS', 'prices.high', inHigh, guardInLen);
  requireLength('CDL3WHITESOLDIERS', 'prices.low', inLow, guardInLen);
  requireLength('CDL3WHITESOLDIERS', 'prices.close', inClose, guardInLen);
  requireOutput('CDL3WHITESOLDIERS', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDL3WHITESOLDIERS', 'outInteger', 'prices.open');
  }
  if (overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDL3WHITESOLDIERS', 'outInteger', 'prices.high');
  }
  if (overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDL3WHITESOLDIERS', 'outInteger', 'prices.low');
  }
  if (overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDL3WHITESOLDIERS', 'outInteger', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdl3whitesoldiersImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDL3WHITESOLDIERS', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cdl3whitesoldiers}, {@link cdl3whitesoldiersLookback},
 * {@link cdl3whitesoldiersDisplayShift}, {@link cdl3whitesoldiersOpen} and
 * {@link cdl3whitesoldiersOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface Cdl3whitesoldiersOptions {
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
  outInteger?: Int32Array<ArrayBuffer> | undefined;
}

/**
 * What {@link cdl3whitesoldiers} returns, and {@link cdl3whitesoldiersOpenAndFill}
 * with its stream.
 */
export interface Cdl3whitesoldiersResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * +100 when the pattern is detected, 0 otherwise; never negative (three white
   * soldiers is always bullish). `values[k]` is the value at input bar `begIdx + k`.
   */
  values: Int32Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link cdl3whitesoldiers} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link Cdl3whitesoldiersOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdl3whitesoldiersLookback(options?: Cdl3whitesoldiersOptions): number {
  const o = optionsOf('CDL3WHITESOLDIERS lookback', options);
  return cdl3whitesoldiersLookbackPositional(coreOf('CDL3WHITESOLDIERS lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdl3whitesoldiers}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link Cdl3whitesoldiersOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdl3whitesoldiersDisplayShift(outputIdx: number, options?: Cdl3whitesoldiersOptions): number {
  const o = optionsOf('CDL3WHITESOLDIERS displayShift', options);
  return cdl3whitesoldiersDisplayShiftPositional(outputIdx, coreOf('CDL3WHITESOLDIERS displayShift', o.core));
}

/**
 * A three-candle pattern of consecutive white candles with progressively higher
 * closes, each opening within/near the prior body and each with a very short upper
 * shadow. A hit is a bullish reversal signal, most meaningful in a downtrend,
 * which the code does not verify.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdl3whitesoldiers](https://ta-lib.org/functions/cdl3whitesoldiers).
 *
 * **Notes**
 * - Does not verify the prior downtrend the pattern classically assumes for
 *   significance.
 * - Bulkowski's testing found this reverses a downtrend 82% of the time, but
 *   cautions the high rate mostly reflects how rare downward breakouts are
 *   afterward — moves following an upward breakout perform poorly.
 *   ([thepatternsite.com](https://thepatternsite.com/ThreeWhiteSoldiers.html))
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cdl3whitesoldiersLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `open`: Open price of
 *   each bar. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link Cdl3whitesoldiersOptions}.
 * @returns `begIdx` and the values; see {@link Cdl3whitesoldiersResult}. Each
 *   output is a view of its buffer, the caller's or a new one.
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
 * import { cdl3whitesoldiers } from 'ta-lib';
 *
 * const open = new Float64Array(252); // your data, oldest first
 * const high = new Float64Array(252); // your data, oldest first
 * const low = new Float64Array(252); // your data, oldest first
 * const close = new Float64Array(252); // your data, oldest first
 *
 * const { begIdx, values } = cdl3whitesoldiers({ open, high, low, close });
 * console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
 * ```
 * @see [`cdl3blackcrows`](https://ta-lib.org/functions/cdl3blackcrows)
 * @see [`cdladvanceblock`](https://ta-lib.org/functions/cdladvanceblock)
 * @see [`cdlidentical3crows`](https://ta-lib.org/functions/cdlidentical3crows)
 */
export function cdl3whitesoldiers(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: Cdl3whitesoldiersOptions): Cdl3whitesoldiersResult {
  requireArgument('CDL3WHITESOLDIERS', 'prices', prices);
  const o = optionsOf('CDL3WHITESOLDIERS', options);
  if (o.endIdx === undefined) {
    requireSeries('CDL3WHITESOLDIERS', 'prices.open', prices.open);
    requireSeries('CDL3WHITESOLDIERS', 'prices.high', prices.high);
    requireSeries('CDL3WHITESOLDIERS', 'prices.low', prices.low);
    requireSeries('CDL3WHITESOLDIERS', 'prices.close', prices.close);
  }
  const core = coreOf('CDL3WHITESOLDIERS', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cdl3whitesoldiersLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const range = cdl3whitesoldiersPositional(startIdx, endIdx, prices.open, prices.high, prices.low, prices.close, outInteger, core);
  return { begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CDL3WHITESOLDIERS stream: one value per closed bar, bit-identical to
 * {@link cdl3whitesoldiers} over the same series. Open it with
 * {@link cdl3whitesoldiersOpen} or {@link cdl3whitesoldiersOpenAndFill}; there is
 * nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdl3whitesoldiers](https://ta-lib.org/functions/cdl3whitesoldiers).
 */
export class Cdl3whitesoldiersStream {
  /** @internal */
  core: Core;
  /** @internal */
  ShadowVeryShortPeriodTotal: Float64Array = new Float64Array(3);
  /** @internal */
  NearPeriodTotal: Float64Array = new Float64Array(3);
  /** @internal */
  FarPeriodTotal: Float64Array = new Float64Array(3);
  /** @internal */
  ShadowVeryShortPeriodTotalRange1: number = 0;
  /** @internal */
  ShadowVeryShortPeriodTotalTrailRange1: number = 0;
  /** @internal */
  ShadowVeryShortPeriodTotalRange2: number = 0;
  /** @internal */
  ShadowVeryShortPeriodTotalTrailRange2: number = 0;
  /** @internal */
  FarPeriodTotalRange2: number = 0;
  /** @internal */
  FarPeriodTotalTrailRange2: number = 0;
  /** @internal */
  NearPeriodTotalRange2: number = 0;
  /** @internal */
  NearPeriodTotalTrailRange2: number = 0;
  /** @internal */
  BodyShortPeriodTotal: number = 0;
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
  ringPos_BodyShortTrailingIdx: number = 0;
  /** @internal */
  ringCap_BodyShortTrailingIdx: number = 0;
  /** @internal */
  ring_BodyShortTrailingIdx_derived: Float64Array = new Float64Array(1);
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
  ringPos_ShadowVeryShortTrailingIdx: number = 0;
  /** @internal */
  ringCap_ShadowVeryShortTrailingIdx: number = 0;
  /** @internal */
  ring_ShadowVeryShortTrailingIdx_derived: Float64Array = new Float64Array(1);
  /** @internal */
  cs_BodyShort_rangeType: number = 0;
  /** @internal */
  cs_BodyShort_avgPeriod: number = 0;
  /** @internal */
  cs_BodyShort_factor: number = 0;
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
  static of(core: Core): Cdl3whitesoldiersStream {
    return new Cdl3whitesoldiersStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cdl3whitesoldiers} reports them over the same bars. Each accepted
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
      throw streamFailure('CDL3WHITESOLDIERS advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param open - The bar's `open` value: Open price of each bar.
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(open: number, high: number, low: number, close: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('CDL3WHITESOLDIERS update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(open) || !Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('CDL3WHITESOLDIERS update', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    cdl3whitesoldiersStepImpl(this, open, high, low, close);
    this.outRangeCount++;
    return this.cur_outInteger;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param open - The bar's `open` value: Open price of each bar.
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(open: number, high: number, low: number, close: number): number {
    if (!Number.isFinite(open) || !Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('CDL3WHITESOLDIERS peek', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inOpen = open;
    const inHigh = high;
    const inLow = low;
    const inClose = close;
    let cur_outInteger = 0;
    const BodyShort_rangeType = sp.cs_BodyShort_rangeType;
    const BodyShort_avgPeriod = sp.cs_BodyShort_avgPeriod;
    const BodyShort_factor = sp.cs_BodyShort_factor;
    const Far_rangeType = sp.cs_Far_rangeType;
    const Far_avgPeriod = sp.cs_Far_avgPeriod;
    const Far_factor = sp.cs_Far_factor;
    const Near_rangeType = sp.cs_Near_rangeType;
    const Near_avgPeriod = sp.cs_Near_avgPeriod;
    const Near_factor = sp.cs_Near_factor;
    const ShadowVeryShort_rangeType = sp.cs_ShadowVeryShort_rangeType;
    const ShadowVeryShort_avgPeriod = sp.cs_ShadowVeryShort_avgPeriod;
    const ShadowVeryShort_factor = sp.cs_ShadowVeryShort_factor;
    if (((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === 1 && /* 1st white */
        ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === 1 && /* 2nd white */
        ((inClose >= inOpen) ? 1 : -1) === 1 &&                 /* 3rd white */
        (sp.lag2_inHigh - ((sp.lag2_inClose >= sp.lag2_inOpen) ? sp.lag2_inClose : sp.lag2_inOpen)) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (sp.ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((ShadowVeryShort_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short upper shadow */
        (sp.lag1_inHigh - ((sp.lag1_inClose >= sp.lag1_inOpen) ? sp.lag1_inClose : sp.lag1_inOpen)) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (sp.ShadowVeryShortPeriodTotal[1] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((ShadowVeryShort_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short upper shadow */
        (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (sp.ShadowVeryShortPeriodTotal[0] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short upper shadow */
        inClose > sp.lag1_inClose &&
        sp.lag1_inClose > sp.lag2_inClose &&                    /* consecutive higher closes */
        sp.lag1_inOpen > sp.lag2_inOpen &&                      /* 2nd opens within/near 1st real body */
        sp.lag1_inOpen <= sp.lag2_inClose + ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
        inOpen > sp.lag1_inOpen &&                              /* 3rd opens within/near 2nd real body */
        inOpen <= sp.lag1_inClose + ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
        Math.abs(sp.lag1_inClose - sp.lag1_inOpen) > Math.abs(sp.lag2_inClose - sp.lag2_inOpen) - ((Far_factor * (((Far_avgPeriod !== 0) ? (sp.FarPeriodTotal[2] / Far_avgPeriod) : ((Far_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Far_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Far_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Far_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd not far shorter than 1st */
        Math.abs(inClose - inOpen) > Math.abs(sp.lag1_inClose - sp.lag1_inOpen) - ((Far_factor * (((Far_avgPeriod !== 0) ? (sp.FarPeriodTotal[1] / Far_avgPeriod) : ((Far_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Far_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Far_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Far_rangeType === 2) ? 2.0 : 1.0)))) && /* 3rd not far shorter than 2nd */
        Math.abs(inClose - inOpen) > ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (sp.BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0))))) /* not short real body */
    {
      cur_outInteger = 100;
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
  clone(): Cdl3whitesoldiersStream {
    const c = new Cdl3whitesoldiersStream(this.core);
    c.ShadowVeryShortPeriodTotal = this.ShadowVeryShortPeriodTotal.slice();
    c.NearPeriodTotal = this.NearPeriodTotal.slice();
    c.FarPeriodTotal = this.FarPeriodTotal.slice();
    c.ShadowVeryShortPeriodTotalRange1 = this.ShadowVeryShortPeriodTotalRange1;
    c.ShadowVeryShortPeriodTotalTrailRange1 = this.ShadowVeryShortPeriodTotalTrailRange1;
    c.ShadowVeryShortPeriodTotalRange2 = this.ShadowVeryShortPeriodTotalRange2;
    c.ShadowVeryShortPeriodTotalTrailRange2 = this.ShadowVeryShortPeriodTotalTrailRange2;
    c.FarPeriodTotalRange2 = this.FarPeriodTotalRange2;
    c.FarPeriodTotalTrailRange2 = this.FarPeriodTotalTrailRange2;
    c.NearPeriodTotalRange2 = this.NearPeriodTotalRange2;
    c.NearPeriodTotalTrailRange2 = this.NearPeriodTotalTrailRange2;
    c.BodyShortPeriodTotal = this.BodyShortPeriodTotal;
    c.lag1_inOpen = this.lag1_inOpen;
    c.lag2_inOpen = this.lag2_inOpen;
    c.lag1_inHigh = this.lag1_inHigh;
    c.lag2_inHigh = this.lag2_inHigh;
    c.lag1_inLow = this.lag1_inLow;
    c.lag2_inLow = this.lag2_inLow;
    c.lag1_inClose = this.lag1_inClose;
    c.lag2_inClose = this.lag2_inClose;
    c.ringPos_BodyShortTrailingIdx = this.ringPos_BodyShortTrailingIdx;
    c.ringCap_BodyShortTrailingIdx = this.ringCap_BodyShortTrailingIdx;
    c.ring_BodyShortTrailingIdx_derived = this.ring_BodyShortTrailingIdx_derived.slice();
    c.ringPos_FarTrailingIdx = this.ringPos_FarTrailingIdx;
    c.ringCap_FarTrailingIdx = this.ringCap_FarTrailingIdx;
    c.ringLag_FarTrailingIdx = this.ringLag_FarTrailingIdx;
    c.ring_FarTrailingIdx_derived = this.ring_FarTrailingIdx_derived.slice();
    c.ringPos_NearTrailingIdx = this.ringPos_NearTrailingIdx;
    c.ringCap_NearTrailingIdx = this.ringCap_NearTrailingIdx;
    c.ringLag_NearTrailingIdx = this.ringLag_NearTrailingIdx;
    c.ring_NearTrailingIdx_derived = this.ring_NearTrailingIdx_derived.slice();
    c.ringPos_ShadowVeryShortTrailingIdx = this.ringPos_ShadowVeryShortTrailingIdx;
    c.ringCap_ShadowVeryShortTrailingIdx = this.ringCap_ShadowVeryShortTrailingIdx;
    c.ring_ShadowVeryShortTrailingIdx_derived = this.ring_ShadowVeryShortTrailingIdx_derived.slice();
    c.cs_BodyShort_rangeType = this.cs_BodyShort_rangeType;
    c.cs_BodyShort_avgPeriod = this.cs_BodyShort_avgPeriod;
    c.cs_BodyShort_factor = this.cs_BodyShort_factor;
    c.cs_Far_rangeType = this.cs_Far_rangeType;
    c.cs_Far_avgPeriod = this.cs_Far_avgPeriod;
    c.cs_Far_factor = this.cs_Far_factor;
    c.cs_Near_rangeType = this.cs_Near_rangeType;
    c.cs_Near_avgPeriod = this.cs_Near_avgPeriod;
    c.cs_Near_factor = this.cs_Near_factor;
    c.cs_ShadowVeryShort_rangeType = this.cs_ShadowVeryShort_rangeType;
    c.cs_ShadowVeryShort_avgPeriod = this.cs_ShadowVeryShort_avgPeriod;
    c.cs_ShadowVeryShort_factor = this.cs_ShadowVeryShort_factor;
    c.cur_outInteger = this.cur_outInteger;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cdl3whitesoldiersStepImpl(sp: Cdl3whitesoldiersStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  let ShadowVeryShortPeriodTotalRange0 = 0;
  let ShadowVeryShortPeriodTotalTrailRange0 = 0;
  let FarPeriodTotalRange1 = 0;
  let FarPeriodTotalTrailRange1 = 0;
  let NearPeriodTotalRange1 = 0;
  let NearPeriodTotalTrailRange1 = 0;
  const BodyShort_rangeType = sp.cs_BodyShort_rangeType;
  const BodyShort_avgPeriod = sp.cs_BodyShort_avgPeriod;
  const BodyShort_factor = sp.cs_BodyShort_factor;
  const Far_rangeType = sp.cs_Far_rangeType;
  const Far_avgPeriod = sp.cs_Far_avgPeriod;
  const Far_factor = sp.cs_Far_factor;
  const Near_rangeType = sp.cs_Near_rangeType;
  const Near_avgPeriod = sp.cs_Near_avgPeriod;
  const Near_factor = sp.cs_Near_factor;
  const ShadowVeryShort_rangeType = sp.cs_ShadowVeryShort_rangeType;
  const ShadowVeryShort_avgPeriod = sp.cs_ShadowVeryShort_avgPeriod;
  const ShadowVeryShort_factor = sp.cs_ShadowVeryShort_factor;
  if (sp.ringCap_BodyShortTrailingIdx === 0) {
    sp.ring_BodyShortTrailingIdx_derived[0] = ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
  sp.ring_FarTrailingIdx_derived[sp.ringPos_FarTrailingIdx] = ((Far_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((Far_rangeType === 1) ? (inHigh - inLow) : ((Far_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  sp.ring_NearTrailingIdx_derived[sp.ringPos_NearTrailingIdx] = ((Near_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((Near_rangeType === 1) ? (inHigh - inLow) : ((Near_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  if (sp.ringCap_ShadowVeryShortTrailingIdx === 0) {
    sp.ring_ShadowVeryShortTrailingIdx_derived[0] = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
  if (((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === 1 && /* 1st white */
      ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === 1 && /* 2nd white */
      ((inClose >= inOpen) ? 1 : -1) === 1 &&                 /* 3rd white */
      (sp.lag2_inHigh - ((sp.lag2_inClose >= sp.lag2_inOpen) ? sp.lag2_inClose : sp.lag2_inOpen)) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (sp.ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((ShadowVeryShort_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short upper shadow */
      (sp.lag1_inHigh - ((sp.lag1_inClose >= sp.lag1_inOpen) ? sp.lag1_inClose : sp.lag1_inOpen)) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (sp.ShadowVeryShortPeriodTotal[1] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((ShadowVeryShort_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short upper shadow */
      (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (sp.ShadowVeryShortPeriodTotal[0] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short upper shadow */
      inClose > sp.lag1_inClose &&
      sp.lag1_inClose > sp.lag2_inClose &&                    /* consecutive higher closes */
      sp.lag1_inOpen > sp.lag2_inOpen &&                      /* 2nd opens within/near 1st real body */
      sp.lag1_inOpen <= sp.lag2_inClose + ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
      inOpen > sp.lag1_inOpen &&                              /* 3rd opens within/near 2nd real body */
      inOpen <= sp.lag1_inClose + ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
      Math.abs(sp.lag1_inClose - sp.lag1_inOpen) > Math.abs(sp.lag2_inClose - sp.lag2_inOpen) - ((Far_factor * (((Far_avgPeriod !== 0) ? (sp.FarPeriodTotal[2] / Far_avgPeriod) : ((Far_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Far_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Far_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Far_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd not far shorter than 1st */
      Math.abs(inClose - inOpen) > Math.abs(sp.lag1_inClose - sp.lag1_inOpen) - ((Far_factor * (((Far_avgPeriod !== 0) ? (sp.FarPeriodTotal[1] / Far_avgPeriod) : ((Far_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Far_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Far_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Far_rangeType === 2) ? 2.0 : 1.0)))) && /* 3rd not far shorter than 2nd */
      Math.abs(inClose - inOpen) > ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (sp.BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0))))) /* not short real body */
  {
    sp.cur_outInteger = 100;
  } else {
    sp.cur_outInteger = 0;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  ShadowVeryShortPeriodTotalRange0 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  ShadowVeryShortPeriodTotalTrailRange0 = sp.ring_ShadowVeryShortTrailingIdx_derived[sp.ringPos_ShadowVeryShortTrailingIdx];
  sp.ShadowVeryShortPeriodTotal[2] = sp.ShadowVeryShortPeriodTotal[2] + (sp.ShadowVeryShortPeriodTotalRange2 - sp.ShadowVeryShortPeriodTotalTrailRange2);
  sp.ShadowVeryShortPeriodTotal[1] = sp.ShadowVeryShortPeriodTotal[1] + (sp.ShadowVeryShortPeriodTotalRange1 - sp.ShadowVeryShortPeriodTotalTrailRange1);
  sp.ShadowVeryShortPeriodTotal[0] = sp.ShadowVeryShortPeriodTotal[0] + (ShadowVeryShortPeriodTotalRange0 - ShadowVeryShortPeriodTotalTrailRange0);
  sp.ShadowVeryShortPeriodTotalRange2 = sp.ShadowVeryShortPeriodTotalRange1;
  sp.ShadowVeryShortPeriodTotalTrailRange2 = sp.ShadowVeryShortPeriodTotalTrailRange1;
  sp.ShadowVeryShortPeriodTotalRange1 = ShadowVeryShortPeriodTotalRange0;
  sp.ShadowVeryShortPeriodTotalTrailRange1 = ShadowVeryShortPeriodTotalTrailRange0;
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
  sp.BodyShortPeriodTotal += ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0))) - sp.ring_BodyShortTrailingIdx_derived[sp.ringPos_BodyShortTrailingIdx];
  sp.lag2_inOpen = sp.lag1_inOpen;
  sp.lag1_inOpen = inOpen;
  sp.lag2_inHigh = sp.lag1_inHigh;
  sp.lag1_inHigh = inHigh;
  sp.lag2_inLow = sp.lag1_inLow;
  sp.lag1_inLow = inLow;
  sp.lag2_inClose = sp.lag1_inClose;
  sp.lag1_inClose = inClose;
  sp.ring_BodyShortTrailingIdx_derived[sp.ringPos_BodyShortTrailingIdx] = ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  sp.ringPos_BodyShortTrailingIdx = sp.ringPos_BodyShortTrailingIdx + 1;
  if (sp.ringPos_BodyShortTrailingIdx >= sp.ringCap_BodyShortTrailingIdx) {
    sp.ringPos_BodyShortTrailingIdx = 0;
  }
  sp.ringPos_FarTrailingIdx = sp.ringPos_FarTrailingIdx + 1;
  if (sp.ringPos_FarTrailingIdx >= sp.ringCap_FarTrailingIdx) {
    sp.ringPos_FarTrailingIdx = 0;
  }
  sp.ringPos_NearTrailingIdx = sp.ringPos_NearTrailingIdx + 1;
  if (sp.ringPos_NearTrailingIdx >= sp.ringCap_NearTrailingIdx) {
    sp.ringPos_NearTrailingIdx = 0;
  }
  sp.ring_ShadowVeryShortTrailingIdx_derived[sp.ringPos_ShadowVeryShortTrailingIdx] = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  sp.ringPos_ShadowVeryShortTrailingIdx = sp.ringPos_ShadowVeryShortTrailingIdx + 1;
  if (sp.ringPos_ShadowVeryShortTrailingIdx >= sp.ringCap_ShadowVeryShortTrailingIdx) {
    sp.ringPos_ShadowVeryShortTrailingIdx = 0;
  }
}

function cdl3whitesoldiersOpenImpl(
  sp: Cdl3whitesoldiersStream,
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
  const NearPeriodTotal = new Float64Array(3);
  const FarPeriodTotal = new Float64Array(3);
  let ShadowVeryShortPeriodTotalRange0 = 0;
  let ShadowVeryShortPeriodTotalTrailRange0 = 0;
  let ShadowVeryShortPeriodTotalRange1 = 0;
  let ShadowVeryShortPeriodTotalTrailRange1 = 0;
  let ShadowVeryShortPeriodTotalRange2 = 0;
  let ShadowVeryShortPeriodTotalTrailRange2 = 0;
  let FarPeriodTotalRange1 = 0;
  let FarPeriodTotalTrailRange1 = 0;
  let FarPeriodTotalRange2 = 0;
  let FarPeriodTotalTrailRange2 = 0;
  let NearPeriodTotalRange1 = 0;
  let NearPeriodTotalTrailRange1 = 0;
  let NearPeriodTotalRange2 = 0;
  let NearPeriodTotalTrailRange2 = 0;
  let BodyShortPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let ShadowVeryShortTrailingIdx = 0;
  let NearTrailingIdx = 0;
  let FarTrailingIdx = 0;
  let BodyShortTrailingIdx = 0;
  let lookbackTotal = 0;
  const historyLen = inOpen.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inHigh.length !== inOpen.length || inLow.length !== inOpen.length || inClose.length !== inOpen.length) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const BodyShort_rangeType: number = core.candleSettings[CandleSettingType.BODY_SHORT].rangeType;
  const BodyShort_avgPeriod = core.candleSettings[CandleSettingType.BODY_SHORT].avgPeriod;
  const BodyShort_factor = core.candleSettings[CandleSettingType.BODY_SHORT].factor;
  const Far_rangeType: number = core.candleSettings[CandleSettingType.FAR].rangeType;
  const Far_avgPeriod = core.candleSettings[CandleSettingType.FAR].avgPeriod;
  const Far_factor = core.candleSettings[CandleSettingType.FAR].factor;
  const Near_rangeType: number = core.candleSettings[CandleSettingType.NEAR].rangeType;
  const Near_avgPeriod = core.candleSettings[CandleSettingType.NEAR].avgPeriod;
  const Near_factor = core.candleSettings[CandleSettingType.NEAR].factor;
  const ShadowVeryShort_rangeType: number = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].rangeType;
  const ShadowVeryShort_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].avgPeriod;
  const ShadowVeryShort_factor = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].factor;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdl3whitesoldiersLookbackPositional(core);
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
  NearPeriodTotal[2] = 0;
  NearPeriodTotal[1] = 0;
  NearPeriodTotal[0] = 0;
  NearTrailingIdx = startIdx - Near_avgPeriod;
  FarPeriodTotal[2] = 0;
  FarPeriodTotal[1] = 0;
  FarPeriodTotal[0] = 0;
  FarTrailingIdx = startIdx - Far_avgPeriod;
  BodyShortPeriodTotal = 0;
  BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
  i = ShadowVeryShortTrailingIdx;
  while (i < startIdx) {
    ShadowVeryShortPeriodTotal[2] = ShadowVeryShortPeriodTotal[2] + ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
    ShadowVeryShortPeriodTotal[1] = ShadowVeryShortPeriodTotal[1] + ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    ShadowVeryShortPeriodTotal[0] = ShadowVeryShortPeriodTotal[0] + ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
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
  i = BodyShortTrailingIdx;
  while (i < startIdx) {
    BodyShortPeriodTotal += ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - three white candlesticks with consecutively higher closes
   * - Greg Morris wants them to be long, Steve Nison doesn't; anyway they should not be short
   * - each candle opens within or near the previous white real body
   * - each candle must have no or very short upper shadow
   * - to differentiate this pattern from advance block, each candle must not be far shorter than the prior candle
   * The meanings of "not short", "very short shadow", "far" and "near" are specified with TA_SetCandleSettings;
   * here the 3 candles must be not short, if you want them to be long use TA_SetCandleSettings on BodyShort
   */
  outIdx = 0;
  ShadowVeryShortPeriodTotalRange1 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
  ShadowVeryShortPeriodTotalTrailRange1 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx - 1] - inOpen[ShadowVeryShortTrailingIdx - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[ShadowVeryShortTrailingIdx - 1] - inLow[ShadowVeryShortTrailingIdx - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[ShadowVeryShortTrailingIdx - 1] - (((inClose[ShadowVeryShortTrailingIdx - 1]) >= (inOpen[ShadowVeryShortTrailingIdx - 1])) ? (inClose[ShadowVeryShortTrailingIdx - 1]) : (inOpen[ShadowVeryShortTrailingIdx - 1]))) + ((((inClose[ShadowVeryShortTrailingIdx - 1]) >= (inOpen[ShadowVeryShortTrailingIdx - 1])) ? (inOpen[ShadowVeryShortTrailingIdx - 1]) : (inClose[ShadowVeryShortTrailingIdx - 1])) - inLow[ShadowVeryShortTrailingIdx - 1])) : 0.0)));
  ShadowVeryShortPeriodTotalRange2 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
  ShadowVeryShortPeriodTotalTrailRange2 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx - 2] - inOpen[ShadowVeryShortTrailingIdx - 2])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[ShadowVeryShortTrailingIdx - 2] - inLow[ShadowVeryShortTrailingIdx - 2]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[ShadowVeryShortTrailingIdx - 2] - (((inClose[ShadowVeryShortTrailingIdx - 2]) >= (inOpen[ShadowVeryShortTrailingIdx - 2])) ? (inClose[ShadowVeryShortTrailingIdx - 2]) : (inOpen[ShadowVeryShortTrailingIdx - 2]))) + ((((inClose[ShadowVeryShortTrailingIdx - 2]) >= (inOpen[ShadowVeryShortTrailingIdx - 2])) ? (inOpen[ShadowVeryShortTrailingIdx - 2]) : (inClose[ShadowVeryShortTrailingIdx - 2])) - inLow[ShadowVeryShortTrailingIdx - 2])) : 0.0)));
  FarPeriodTotalRange2 = ((Far_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Far_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Far_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
  FarPeriodTotalTrailRange2 = ((Far_rangeType === 0) ? (Math.abs(inClose[FarTrailingIdx - 2] - inOpen[FarTrailingIdx - 2])) : ((Far_rangeType === 1) ? (inHigh[FarTrailingIdx - 2] - inLow[FarTrailingIdx - 2]) : ((Far_rangeType === 2) ? ((inHigh[FarTrailingIdx - 2] - (((inClose[FarTrailingIdx - 2]) >= (inOpen[FarTrailingIdx - 2])) ? (inClose[FarTrailingIdx - 2]) : (inOpen[FarTrailingIdx - 2]))) + ((((inClose[FarTrailingIdx - 2]) >= (inOpen[FarTrailingIdx - 2])) ? (inOpen[FarTrailingIdx - 2]) : (inClose[FarTrailingIdx - 2])) - inLow[FarTrailingIdx - 2])) : 0.0)));
  NearPeriodTotalRange2 = ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
  NearPeriodTotalTrailRange2 = ((Near_rangeType === 0) ? (Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : ((Near_rangeType === 1) ? (inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) : ((Near_rangeType === 2) ? ((inHigh[NearTrailingIdx - 2] - (((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inClose[NearTrailingIdx - 2]) : (inOpen[NearTrailingIdx - 2]))) + ((((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inOpen[NearTrailingIdx - 2]) : (inClose[NearTrailingIdx - 2])) - inLow[NearTrailingIdx - 2])) : 0.0)));
  do {
    if (((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === 1 && /* 1st white */
        ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === 1 && /* 2nd white */
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === 1 &&         /* 3rd white */
        (inHigh[i - 2] - ((inClose[i - 2] >= inOpen[i - 2]) ? inClose[i - 2] : inOpen[i - 2])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short upper shadow */
        (inHigh[i - 1] - ((inClose[i - 1] >= inOpen[i - 1]) ? inClose[i - 1] : inOpen[i - 1])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (ShadowVeryShortPeriodTotal[1] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short upper shadow */
        (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (ShadowVeryShortPeriodTotal[0] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short upper shadow */
        inClose[i] > inClose[i - 1] &&
        inClose[i - 1] > inClose[i - 2] &&                    /* consecutive higher closes */
        inOpen[i - 1] > inOpen[i - 2] &&                      /* 2nd opens within/near 1st real body */
        inOpen[i - 1] <= inClose[i - 2] + ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
        inOpen[i] > inOpen[i - 1] &&                          /* 3rd opens within/near 2nd real body */
        inOpen[i] <= inClose[i - 1] + ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
        Math.abs(inClose[i - 1] - inOpen[i - 1]) > Math.abs(inClose[i - 2] - inOpen[i - 2]) - ((Far_factor * (((Far_avgPeriod !== 0) ? (FarPeriodTotal[2] / Far_avgPeriod) : ((Far_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Far_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Far_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Far_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd not far shorter than 1st */
        Math.abs(inClose[i] - inOpen[i]) > Math.abs(inClose[i - 1] - inOpen[i - 1]) - ((Far_factor * (((Far_avgPeriod !== 0) ? (FarPeriodTotal[1] / Far_avgPeriod) : ((Far_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Far_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Far_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Far_rangeType === 2) ? 2.0 : 1.0)))) && /* 3rd not far shorter than 2nd */
        Math.abs(inClose[i] - inOpen[i]) > ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0))))) /* not short real body */
    {
      outInteger[((outIdx++ * outStride) | 0)] = 100;
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    ShadowVeryShortPeriodTotalRange0 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    ShadowVeryShortPeriodTotalTrailRange0 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx] - inOpen[ShadowVeryShortTrailingIdx])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[ShadowVeryShortTrailingIdx] - inLow[ShadowVeryShortTrailingIdx]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[ShadowVeryShortTrailingIdx] - (((inClose[ShadowVeryShortTrailingIdx]) >= (inOpen[ShadowVeryShortTrailingIdx])) ? (inClose[ShadowVeryShortTrailingIdx]) : (inOpen[ShadowVeryShortTrailingIdx]))) + ((((inClose[ShadowVeryShortTrailingIdx]) >= (inOpen[ShadowVeryShortTrailingIdx])) ? (inOpen[ShadowVeryShortTrailingIdx]) : (inClose[ShadowVeryShortTrailingIdx])) - inLow[ShadowVeryShortTrailingIdx])) : 0.0)));
    ShadowVeryShortPeriodTotal[2] = ShadowVeryShortPeriodTotal[2] + (ShadowVeryShortPeriodTotalRange2 - ShadowVeryShortPeriodTotalTrailRange2);
    ShadowVeryShortPeriodTotal[1] = ShadowVeryShortPeriodTotal[1] + (ShadowVeryShortPeriodTotalRange1 - ShadowVeryShortPeriodTotalTrailRange1);
    ShadowVeryShortPeriodTotal[0] = ShadowVeryShortPeriodTotal[0] + (ShadowVeryShortPeriodTotalRange0 - ShadowVeryShortPeriodTotalTrailRange0);
    ShadowVeryShortPeriodTotalRange2 = ShadowVeryShortPeriodTotalRange1;
    ShadowVeryShortPeriodTotalTrailRange2 = ShadowVeryShortPeriodTotalTrailRange1;
    ShadowVeryShortPeriodTotalRange1 = ShadowVeryShortPeriodTotalRange0;
    ShadowVeryShortPeriodTotalTrailRange1 = ShadowVeryShortPeriodTotalTrailRange0;
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
    BodyShortPeriodTotal += ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyShort_rangeType === 0) ? (Math.abs(inClose[BodyShortTrailingIdx] - inOpen[BodyShortTrailingIdx])) : ((BodyShort_rangeType === 1) ? (inHigh[BodyShortTrailingIdx] - inLow[BodyShortTrailingIdx]) : ((BodyShort_rangeType === 2) ? ((inHigh[BodyShortTrailingIdx] - (((inClose[BodyShortTrailingIdx]) >= (inOpen[BodyShortTrailingIdx])) ? (inClose[BodyShortTrailingIdx]) : (inOpen[BodyShortTrailingIdx]))) + ((((inClose[BodyShortTrailingIdx]) >= (inOpen[BodyShortTrailingIdx])) ? (inOpen[BodyShortTrailingIdx]) : (inClose[BodyShortTrailingIdx])) - inLow[BodyShortTrailingIdx])) : 0.0)));
    i += 1;
    ShadowVeryShortTrailingIdx += 1;
    NearTrailingIdx += 1;
    FarTrailingIdx += 1;
    BodyShortTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const cap_BodyShortTrailingIdx = i - BodyShortTrailingIdx;
  if (cap_BodyShortTrailingIdx < 0 || cap_BodyShortTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_BodyShortTrailingIdx = cap_BodyShortTrailingIdx > 0 ? cap_BodyShortTrailingIdx : 1;
  const capRing_BodyShortTrailingIdx_derived = new Float64Array(allocN_BodyShortTrailingIdx);
  for (let fillJ = historyLen - cap_BodyShortTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_BodyShortTrailingIdx_derived[fillJ - (historyLen - cap_BodyShortTrailingIdx)] = ((BodyShort_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyShort_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyShort_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
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
  const cap_ShadowVeryShortTrailingIdx = i - ShadowVeryShortTrailingIdx;
  if (cap_ShadowVeryShortTrailingIdx < 0 || cap_ShadowVeryShortTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_ShadowVeryShortTrailingIdx = cap_ShadowVeryShortTrailingIdx > 0 ? cap_ShadowVeryShortTrailingIdx : 1;
  const capRing_ShadowVeryShortTrailingIdx_derived = new Float64Array(allocN_ShadowVeryShortTrailingIdx);
  for (let fillJ = historyLen - cap_ShadowVeryShortTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_ShadowVeryShortTrailingIdx_derived[fillJ - (historyLen - cap_ShadowVeryShortTrailingIdx)] = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  sp.ShadowVeryShortPeriodTotal = ShadowVeryShortPeriodTotal;
  sp.NearPeriodTotal = NearPeriodTotal;
  sp.FarPeriodTotal = FarPeriodTotal;
  sp.ShadowVeryShortPeriodTotalRange1 = ShadowVeryShortPeriodTotalRange1;
  sp.ShadowVeryShortPeriodTotalTrailRange1 = ShadowVeryShortPeriodTotalTrailRange1;
  sp.ShadowVeryShortPeriodTotalRange2 = ShadowVeryShortPeriodTotalRange2;
  sp.ShadowVeryShortPeriodTotalTrailRange2 = ShadowVeryShortPeriodTotalTrailRange2;
  sp.FarPeriodTotalRange2 = FarPeriodTotalRange2;
  sp.FarPeriodTotalTrailRange2 = FarPeriodTotalTrailRange2;
  sp.NearPeriodTotalRange2 = NearPeriodTotalRange2;
  sp.NearPeriodTotalTrailRange2 = NearPeriodTotalTrailRange2;
  sp.BodyShortPeriodTotal = BodyShortPeriodTotal;
  sp.lag1_inOpen = inOpen[historyLen - 1];
  sp.lag2_inOpen = inOpen[historyLen - 2];
  sp.lag1_inHigh = inHigh[historyLen - 1];
  sp.lag2_inHigh = inHigh[historyLen - 2];
  sp.lag1_inLow = inLow[historyLen - 1];
  sp.lag2_inLow = inLow[historyLen - 2];
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.lag2_inClose = inClose[historyLen - 2];
  sp.ringPos_BodyShortTrailingIdx = 0;
  sp.ringCap_BodyShortTrailingIdx = cap_BodyShortTrailingIdx;
  sp.ring_BodyShortTrailingIdx_derived = capRing_BodyShortTrailingIdx_derived;
  sp.ringPos_FarTrailingIdx = historyLen % cap_FarTrailingIdx;
  sp.ringCap_FarTrailingIdx = cap_FarTrailingIdx;
  sp.ringLag_FarTrailingIdx = capLag_FarTrailingIdx;
  sp.ring_FarTrailingIdx_derived = capRing_FarTrailingIdx_derived;
  sp.ringPos_NearTrailingIdx = historyLen % cap_NearTrailingIdx;
  sp.ringCap_NearTrailingIdx = cap_NearTrailingIdx;
  sp.ringLag_NearTrailingIdx = capLag_NearTrailingIdx;
  sp.ring_NearTrailingIdx_derived = capRing_NearTrailingIdx_derived;
  sp.ringPos_ShadowVeryShortTrailingIdx = 0;
  sp.ringCap_ShadowVeryShortTrailingIdx = cap_ShadowVeryShortTrailingIdx;
  sp.ring_ShadowVeryShortTrailingIdx_derived = capRing_ShadowVeryShortTrailingIdx_derived;
  sp.cs_BodyShort_rangeType = BodyShort_rangeType;
  sp.cs_BodyShort_avgPeriod = BodyShort_avgPeriod;
  sp.cs_BodyShort_factor = BodyShort_factor;
  sp.cs_Far_rangeType = Far_rangeType;
  sp.cs_Far_avgPeriod = Far_avgPeriod;
  sp.cs_Far_factor = Far_factor;
  sp.cs_Near_rangeType = Near_rangeType;
  sp.cs_Near_avgPeriod = Near_avgPeriod;
  sp.cs_Near_factor = Near_factor;
  sp.cs_ShadowVeryShort_rangeType = ShadowVeryShort_rangeType;
  sp.cs_ShadowVeryShort_avgPeriod = ShadowVeryShort_avgPeriod;
  sp.cs_ShadowVeryShort_factor = ShadowVeryShort_factor;
  sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cdl3whitesoldiersOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): Cdl3whitesoldiersStream {
  const sp = Cdl3whitesoldiersStream.of(core);
  const retCode = cdl3whitesoldiersOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDL3WHITESOLDIERS openAndFill', inOpen.length, startIdx, cdl3whitesoldiersLookbackPositional(core));
  }
  throw streamFailure('CDL3WHITESOLDIERS openAndFill', retCode);
}

/** @internal */
export function cdl3whitesoldiersOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): Cdl3whitesoldiersStream {
  const sp = Cdl3whitesoldiersStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdl3whitesoldiersOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDL3WHITESOLDIERS open', inOpen.length, startIdx, cdl3whitesoldiersLookbackPositional(core));
  }
  throw streamFailure('CDL3WHITESOLDIERS open', retCode);
}

/** @internal */
export function cdl3whitesoldiersOpenPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): Cdl3whitesoldiersStream {
  requireArgument('CDL3WHITESOLDIERS open', 'prices.open', inOpen);
  requireHistory('CDL3WHITESOLDIERS open', 'prices.open', inOpen.length);
  requireArgument('CDL3WHITESOLDIERS open', 'prices.high', inHigh);
  requireArgument('CDL3WHITESOLDIERS open', 'prices.low', inLow);
  requireArgument('CDL3WHITESOLDIERS open', 'prices.close', inClose);
  requireHistoryLength('CDL3WHITESOLDIERS open', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDL3WHITESOLDIERS open', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDL3WHITESOLDIERS open', 'prices.close', inClose.length, inOpen.length);
  return cdl3whitesoldiersOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/** @internal */
export function cdl3whitesoldiersOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): Cdl3whitesoldiersStream {
  requireArgument('CDL3WHITESOLDIERS openAndFill', 'prices.open', inOpen);
  requireHistory('CDL3WHITESOLDIERS openAndFill', 'prices.open', inOpen.length);
  requireArgument('CDL3WHITESOLDIERS openAndFill', 'prices.high', inHigh);
  requireArgument('CDL3WHITESOLDIERS openAndFill', 'prices.low', inLow);
  requireArgument('CDL3WHITESOLDIERS openAndFill', 'prices.close', inClose);
  const guardOutLen = openFillCount('CDL3WHITESOLDIERS openAndFill', inOpen.length, cdl3whitesoldiersLookbackPositional(core));
  requireHistoryLength('CDL3WHITESOLDIERS openAndFill', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDL3WHITESOLDIERS openAndFill', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDL3WHITESOLDIERS openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('CDL3WHITESOLDIERS openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('CDL3WHITESOLDIERS openAndFill', 'outInteger', 'prices.open');
  }
  if (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CDL3WHITESOLDIERS openAndFill', 'outInteger', 'prices.high');
  }
  if (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CDL3WHITESOLDIERS openAndFill', 'outInteger', 'prices.low');
  }
  if (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('CDL3WHITESOLDIERS openAndFill', 'outInteger', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDL3WHITESOLDIERS openAndFill', 'outInteger', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDL3WHITESOLDIERS openAndFill', 'outInteger', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDL3WHITESOLDIERS openAndFill', 'outInteger', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDL3WHITESOLDIERS openAndFill', 'outInteger', 'prices.close');
  }
  return cdl3whitesoldiersOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}

/**
 * Opens a live CDL3WHITESOLDIERS stream over the warm-up history: `value()` starts
 * at the last history bar, bit-identical to {@link cdl3whitesoldiers} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link Cdl3whitesoldiersOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdl3whitesoldiersLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input or histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdl3whitesoldiersOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdl3whitesoldiersOpen({ open, high, low, close });
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdl3whitesoldiersOpen(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: Cdl3whitesoldiersOptions): Cdl3whitesoldiersStream {
  requireArgument('CDL3WHITESOLDIERS open', 'prices', prices);
  const o = optionsOf('CDL3WHITESOLDIERS open', options);
  const core = coreOf('CDL3WHITESOLDIERS open', o.core);
  return cdl3whitesoldiersOpenPositional(prices.open, prices.high, prices.low, prices.close, core);
}

/**
 * {@link cdl3whitesoldiersOpen} that also computes the outputs over the whole
 * history in the same pass, bit-identically to {@link cdl3whitesoldiers}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link Cdl3whitesoldiersOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cdl3whitesoldiers} returns them; the range is
 *   the stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdl3whitesoldiersLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, histories of different lengths or a buffer too short,
 *   of the wrong type or starting where another buffer or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdl3whitesoldiersOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = cdl3whitesoldiersOpenAndFill({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cdl3whitesoldiersOpenAndFill(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: Cdl3whitesoldiersOptions): Cdl3whitesoldiersResult & { stream: Cdl3whitesoldiersStream } {
  requireArgument('CDL3WHITESOLDIERS openAndFill', 'prices', prices);
  const o = optionsOf('CDL3WHITESOLDIERS openAndFill', options);
  const core = coreOf('CDL3WHITESOLDIERS openAndFill', o.core);
  const capacity = fillCapacity(cdl3whitesoldiersLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const stream = cdl3whitesoldiersOpenAndFillPositional(prices.open, prices.high, prices.low, prices.close, outInteger, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}
