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
 *  103104 AC     Creation
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
export function cdlidentical3crowsLookbackPositional(core: Core = Core.DEFAULT): number {
  const Equal_avgPeriod = core.candleSettings[CandleSettingType.EQUAL].avgPeriod;
  const ShadowVeryShort_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].avgPeriod;
  return (ShadowVeryShort_avgPeriod > Equal_avgPeriod ? ShadowVeryShort_avgPeriod : Equal_avgPeriod) + 2;
}

/** @internal */
export function cdlidentical3crowsDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
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
  let ShadowVeryShortPeriodTotalRange0 = 0;
  let ShadowVeryShortPeriodTotalTrailRange0 = 0;
  let ShadowVeryShortPeriodTotalRange1 = 0;
  let ShadowVeryShortPeriodTotalTrailRange1 = 0;
  let ShadowVeryShortPeriodTotalRange2 = 0;
  let ShadowVeryShortPeriodTotalTrailRange2 = 0;
  let EqualPeriodTotalRange1 = 0;
  let EqualPeriodTotalTrailRange1 = 0;
  let EqualPeriodTotalRange2 = 0;
  let EqualPeriodTotalTrailRange2 = 0;
  let i = 0;
  let outIdx = 0;
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
  lookbackTotal = cdlidentical3crowsLookbackPositional(core);
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
  ShadowVeryShortPeriodTotalRange1 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
  ShadowVeryShortPeriodTotalTrailRange1 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx - 1] - inOpen[ShadowVeryShortTrailingIdx - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[ShadowVeryShortTrailingIdx - 1] - inLow[ShadowVeryShortTrailingIdx - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[ShadowVeryShortTrailingIdx - 1] - (((inClose[ShadowVeryShortTrailingIdx - 1]) >= (inOpen[ShadowVeryShortTrailingIdx - 1])) ? (inClose[ShadowVeryShortTrailingIdx - 1]) : (inOpen[ShadowVeryShortTrailingIdx - 1]))) + ((((inClose[ShadowVeryShortTrailingIdx - 1]) >= (inOpen[ShadowVeryShortTrailingIdx - 1])) ? (inOpen[ShadowVeryShortTrailingIdx - 1]) : (inClose[ShadowVeryShortTrailingIdx - 1])) - inLow[ShadowVeryShortTrailingIdx - 1])) : 0.0)));
  ShadowVeryShortPeriodTotalRange2 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
  ShadowVeryShortPeriodTotalTrailRange2 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx - 2] - inOpen[ShadowVeryShortTrailingIdx - 2])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[ShadowVeryShortTrailingIdx - 2] - inLow[ShadowVeryShortTrailingIdx - 2]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[ShadowVeryShortTrailingIdx - 2] - (((inClose[ShadowVeryShortTrailingIdx - 2]) >= (inOpen[ShadowVeryShortTrailingIdx - 2])) ? (inClose[ShadowVeryShortTrailingIdx - 2]) : (inOpen[ShadowVeryShortTrailingIdx - 2]))) + ((((inClose[ShadowVeryShortTrailingIdx - 2]) >= (inOpen[ShadowVeryShortTrailingIdx - 2])) ? (inOpen[ShadowVeryShortTrailingIdx - 2]) : (inClose[ShadowVeryShortTrailingIdx - 2])) - inLow[ShadowVeryShortTrailingIdx - 2])) : 0.0)));
  EqualPeriodTotalRange2 = ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
  EqualPeriodTotalTrailRange2 = ((Equal_rangeType === 0) ? (Math.abs(inClose[EqualTrailingIdx - 2] - inOpen[EqualTrailingIdx - 2])) : ((Equal_rangeType === 1) ? (inHigh[EqualTrailingIdx - 2] - inLow[EqualTrailingIdx - 2]) : ((Equal_rangeType === 2) ? ((inHigh[EqualTrailingIdx - 2] - (((inClose[EqualTrailingIdx - 2]) >= (inOpen[EqualTrailingIdx - 2])) ? (inClose[EqualTrailingIdx - 2]) : (inOpen[EqualTrailingIdx - 2]))) + ((((inClose[EqualTrailingIdx - 2]) >= (inOpen[EqualTrailingIdx - 2])) ? (inOpen[EqualTrailingIdx - 2]) : (inClose[EqualTrailingIdx - 2])) - inLow[EqualTrailingIdx - 2])) : 0.0)));
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
    ShadowVeryShortPeriodTotalRange0 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    ShadowVeryShortPeriodTotalTrailRange0 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx] - inOpen[ShadowVeryShortTrailingIdx])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[ShadowVeryShortTrailingIdx] - inLow[ShadowVeryShortTrailingIdx]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[ShadowVeryShortTrailingIdx] - (((inClose[ShadowVeryShortTrailingIdx]) >= (inOpen[ShadowVeryShortTrailingIdx])) ? (inClose[ShadowVeryShortTrailingIdx]) : (inOpen[ShadowVeryShortTrailingIdx]))) + ((((inClose[ShadowVeryShortTrailingIdx]) >= (inOpen[ShadowVeryShortTrailingIdx])) ? (inOpen[ShadowVeryShortTrailingIdx]) : (inClose[ShadowVeryShortTrailingIdx])) - inLow[ShadowVeryShortTrailingIdx])) : 0.0)));
    ShadowVeryShortPeriodTotal[2] = ShadowVeryShortPeriodTotal[2] + (ShadowVeryShortPeriodTotalRange2 - ShadowVeryShortPeriodTotalTrailRange2);
    ShadowVeryShortPeriodTotal[1] = ShadowVeryShortPeriodTotal[1] + (ShadowVeryShortPeriodTotalRange1 - ShadowVeryShortPeriodTotalTrailRange1);
    ShadowVeryShortPeriodTotal[0] = ShadowVeryShortPeriodTotal[0] + (ShadowVeryShortPeriodTotalRange0 - ShadowVeryShortPeriodTotalTrailRange0);
    ShadowVeryShortPeriodTotalRange2 = ShadowVeryShortPeriodTotalRange1;
    ShadowVeryShortPeriodTotalTrailRange2 = ShadowVeryShortPeriodTotalTrailRange1;
    ShadowVeryShortPeriodTotalRange1 = ShadowVeryShortPeriodTotalRange0;
    ShadowVeryShortPeriodTotalTrailRange1 = ShadowVeryShortPeriodTotalTrailRange0;
    EqualPeriodTotalRange1 = ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    EqualPeriodTotalTrailRange1 = ((Equal_rangeType === 0) ? (Math.abs(inClose[EqualTrailingIdx - 1] - inOpen[EqualTrailingIdx - 1])) : ((Equal_rangeType === 1) ? (inHigh[EqualTrailingIdx - 1] - inLow[EqualTrailingIdx - 1]) : ((Equal_rangeType === 2) ? ((inHigh[EqualTrailingIdx - 1] - (((inClose[EqualTrailingIdx - 1]) >= (inOpen[EqualTrailingIdx - 1])) ? (inClose[EqualTrailingIdx - 1]) : (inOpen[EqualTrailingIdx - 1]))) + ((((inClose[EqualTrailingIdx - 1]) >= (inOpen[EqualTrailingIdx - 1])) ? (inOpen[EqualTrailingIdx - 1]) : (inClose[EqualTrailingIdx - 1])) - inLow[EqualTrailingIdx - 1])) : 0.0)));
    EqualPeriodTotal[2] = EqualPeriodTotal[2] + (EqualPeriodTotalRange2 - EqualPeriodTotalTrailRange2);
    EqualPeriodTotal[1] = EqualPeriodTotal[1] + (EqualPeriodTotalRange1 - EqualPeriodTotalTrailRange1);
    EqualPeriodTotalRange2 = EqualPeriodTotalRange1;
    EqualPeriodTotalTrailRange2 = EqualPeriodTotalTrailRange1;
    i += 1;
    ShadowVeryShortTrailingIdx += 1;
    EqualTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlidentical3crowsPositional(
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
  const guardStart = clampedStart('CDLIDENTICAL3CROWS', startIdx, cdlidentical3crowsLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLIDENTICAL3CROWS', 'prices.open', inOpen, guardInLen);
  requireLength('CDLIDENTICAL3CROWS', 'prices.high', inHigh, guardInLen);
  requireLength('CDLIDENTICAL3CROWS', 'prices.low', inLow, guardInLen);
  requireLength('CDLIDENTICAL3CROWS', 'prices.close', inClose, guardInLen);
  requireOutput('CDLIDENTICAL3CROWS', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLIDENTICAL3CROWS', 'outInteger', 'prices.open');
  }
  if (overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLIDENTICAL3CROWS', 'outInteger', 'prices.high');
  }
  if (overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLIDENTICAL3CROWS', 'outInteger', 'prices.low');
  }
  if (overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLIDENTICAL3CROWS', 'outInteger', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdlidentical3crowsImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLIDENTICAL3CROWS', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cdlidentical3crows}, {@link cdlidentical3crowsLookback},
 * {@link cdlidentical3crowsDisplayShift}, {@link cdlidentical3crowsOpen} and
 * {@link cdlidentical3crowsOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface Cdlidentical3crowsOptions {
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
 * What {@link cdlidentical3crows} returns, and
 * {@link cdlidentical3crowsOpenAndFill} with its stream.
 */
export interface Cdlidentical3crowsResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * -100 when the pattern is detected (always bearish), 0 otherwise. Never emits
   * +100. `values[k]` is the value at input bar `begIdx + k`.
   */
  values: Int32Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link cdlidentical3crows} consumes before its
 * first value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link Cdlidentical3crowsOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlidentical3crowsLookback(options?: Cdlidentical3crowsOptions): number {
  const o = optionsOf('CDLIDENTICAL3CROWS lookback', options);
  return cdlidentical3crowsLookbackPositional(coreOf('CDLIDENTICAL3CROWS lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdlidentical3crows}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link Cdlidentical3crowsOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlidentical3crowsDisplayShift(outputIdx: number, options?: Cdlidentical3crowsOptions): number {
  const o = optionsOf('CDLIDENTICAL3CROWS displayShift', options);
  return cdlidentical3crowsDisplayShiftPositional(outputIdx, coreOf('CDLIDENTICAL3CROWS displayShift', o.core));
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
 * - Rare in Bulkowski's tests: he ranks it 83rd of 103 patterns by how often it
 *   occurs ([Identical Three
 *   Crows](https://thepatternsite.com/Identical3Crows.html)). TA-Lib finds it more
 *   often in crypto: 180 times in [about 16,000 daily
 *   bars](https://ta-lib.org/api/candlestick-outputs/#how-often-patterns-fire).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cdlidentical3crowsLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `open`: Open price of
 *   each bar. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link Cdlidentical3crowsOptions}.
 * @returns `begIdx` and the values; see {@link Cdlidentical3crowsResult}. Each
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
 * import { cdlidentical3crows } from 'ta-lib';
 *
 * const open = new Float64Array(252); // your data, oldest first
 * const high = new Float64Array(252); // your data, oldest first
 * const low = new Float64Array(252); // your data, oldest first
 * const close = new Float64Array(252); // your data, oldest first
 *
 * const { begIdx, values } = cdlidentical3crows({ open, high, low, close });
 * console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
 * ```
 * @see [`cdl3blackcrows`](https://ta-lib.org/functions/cdl3blackcrows)
 * @see [`cdl2crows`](https://ta-lib.org/functions/cdl2crows)
 */
export function cdlidentical3crows(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: Cdlidentical3crowsOptions): Cdlidentical3crowsResult {
  requireArgument('CDLIDENTICAL3CROWS', 'prices', prices);
  const o = optionsOf('CDLIDENTICAL3CROWS', options);
  if (o.endIdx === undefined) {
    requireSeries('CDLIDENTICAL3CROWS', 'prices.open', prices.open);
    requireSeries('CDLIDENTICAL3CROWS', 'prices.high', prices.high);
    requireSeries('CDLIDENTICAL3CROWS', 'prices.low', prices.low);
    requireSeries('CDLIDENTICAL3CROWS', 'prices.close', prices.close);
  }
  const core = coreOf('CDLIDENTICAL3CROWS', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cdlidentical3crowsLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const range = cdlidentical3crowsPositional(startIdx, endIdx, prices.open, prices.high, prices.low, prices.close, outInteger, core);
  return { begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
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
  ShadowVeryShortPeriodTotalRange1: number = 0;
  /** @internal */
  ShadowVeryShortPeriodTotalTrailRange1: number = 0;
  /** @internal */
  ShadowVeryShortPeriodTotalRange2: number = 0;
  /** @internal */
  ShadowVeryShortPeriodTotalTrailRange2: number = 0;
  /** @internal */
  EqualPeriodTotalRange2: number = 0;
  /** @internal */
  EqualPeriodTotalTrailRange2: number = 0;
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
  ringCap_EqualTrailingIdx: number = 2;
  /** @internal */
  ringLag_EqualTrailingIdx: number = 0;
  /** @internal */
  ring_EqualTrailingIdx_derived: Float64Array = new Float64Array(2);
  /** @internal */
  ringPos_ShadowVeryShortTrailingIdx: number = 0;
  /** @internal */
  ringCap_ShadowVeryShortTrailingIdx: number = 0;
  /** @internal */
  ring_ShadowVeryShortTrailingIdx_derived: Float64Array = new Float64Array(1);
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
      throw streamFailure('CDLIDENTICAL3CROWS advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw streamFailure('CDLIDENTICAL3CROWS update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(open) || !Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('CDLIDENTICAL3CROWS update', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    cdlidentical3crowsStepImpl(this, open, high, low, close);
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
      throw nonFiniteBar('CDLIDENTICAL3CROWS peek', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inOpen = open;
    const inHigh = high;
    const inLow = low;
    const inClose = close;
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
    c.ShadowVeryShortPeriodTotalRange1 = this.ShadowVeryShortPeriodTotalRange1;
    c.ShadowVeryShortPeriodTotalTrailRange1 = this.ShadowVeryShortPeriodTotalTrailRange1;
    c.ShadowVeryShortPeriodTotalRange2 = this.ShadowVeryShortPeriodTotalRange2;
    c.ShadowVeryShortPeriodTotalTrailRange2 = this.ShadowVeryShortPeriodTotalTrailRange2;
    c.EqualPeriodTotalRange2 = this.EqualPeriodTotalRange2;
    c.EqualPeriodTotalTrailRange2 = this.EqualPeriodTotalTrailRange2;
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
  let ShadowVeryShortPeriodTotalRange0 = 0;
  let ShadowVeryShortPeriodTotalTrailRange0 = 0;
  let EqualPeriodTotalRange1 = 0;
  let EqualPeriodTotalTrailRange1 = 0;
  let ringPosL_EqualTrailingIdx = 0;
  let ringPosL_ShadowVeryShortTrailingIdx = 0;
  const Equal_rangeType = sp.cs_Equal_rangeType;
  const Equal_avgPeriod = sp.cs_Equal_avgPeriod;
  const Equal_factor = sp.cs_Equal_factor;
  const ShadowVeryShort_rangeType = sp.cs_ShadowVeryShort_rangeType;
  const ShadowVeryShort_avgPeriod = sp.cs_ShadowVeryShort_avgPeriod;
  const ShadowVeryShort_factor = sp.cs_ShadowVeryShort_factor;
  sp.ring_EqualTrailingIdx_derived[sp.ringPos_EqualTrailingIdx] = ((Equal_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((Equal_rangeType === 1) ? (inHigh - inLow) : ((Equal_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  if (sp.ringCap_ShadowVeryShortTrailingIdx === 0) {
    sp.ring_ShadowVeryShortTrailingIdx_derived[0] = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
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
  ShadowVeryShortPeriodTotalRange0 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  ShadowVeryShortPeriodTotalTrailRange0 = sp.ring_ShadowVeryShortTrailingIdx_derived[sp.ringPos_ShadowVeryShortTrailingIdx];
  sp.ShadowVeryShortPeriodTotal[2] = sp.ShadowVeryShortPeriodTotal[2] + (sp.ShadowVeryShortPeriodTotalRange2 - sp.ShadowVeryShortPeriodTotalTrailRange2);
  sp.ShadowVeryShortPeriodTotal[1] = sp.ShadowVeryShortPeriodTotal[1] + (sp.ShadowVeryShortPeriodTotalRange1 - sp.ShadowVeryShortPeriodTotalTrailRange1);
  sp.ShadowVeryShortPeriodTotal[0] = sp.ShadowVeryShortPeriodTotal[0] + (ShadowVeryShortPeriodTotalRange0 - ShadowVeryShortPeriodTotalTrailRange0);
  sp.ShadowVeryShortPeriodTotalRange2 = sp.ShadowVeryShortPeriodTotalRange1;
  sp.ShadowVeryShortPeriodTotalTrailRange2 = sp.ShadowVeryShortPeriodTotalTrailRange1;
  sp.ShadowVeryShortPeriodTotalRange1 = ShadowVeryShortPeriodTotalRange0;
  sp.ShadowVeryShortPeriodTotalTrailRange1 = ShadowVeryShortPeriodTotalTrailRange0;
  EqualPeriodTotalRange1 = ((Equal_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)));
  EqualPeriodTotalTrailRange1 = sp.ring_EqualTrailingIdx_derived[(((sp.ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - sp.ringLag_EqualTrailingIdx - 1) % sp.ringCap_EqualTrailingIdx) | 0)];
  sp.EqualPeriodTotal[2] = sp.EqualPeriodTotal[2] + (sp.EqualPeriodTotalRange2 - sp.EqualPeriodTotalTrailRange2);
  sp.EqualPeriodTotal[1] = sp.EqualPeriodTotal[1] + (EqualPeriodTotalRange1 - EqualPeriodTotalTrailRange1);
  sp.EqualPeriodTotalRange2 = EqualPeriodTotalRange1;
  sp.EqualPeriodTotalTrailRange2 = EqualPeriodTotalTrailRange1;
  sp.lag2_inOpen = sp.lag1_inOpen;
  sp.lag1_inOpen = inOpen;
  sp.lag2_inHigh = sp.lag1_inHigh;
  sp.lag1_inHigh = inHigh;
  sp.lag2_inLow = sp.lag1_inLow;
  sp.lag1_inLow = inLow;
  sp.lag2_inClose = sp.lag1_inClose;
  sp.lag1_inClose = inClose;
  ringPosL_EqualTrailingIdx = sp.ringPos_EqualTrailingIdx;
  ringPosL_EqualTrailingIdx = ringPosL_EqualTrailingIdx + 1;
  ringPosL_EqualTrailingIdx = ringPosL_EqualTrailingIdx & (ringPosL_EqualTrailingIdx - sp.ringCap_EqualTrailingIdx >> 31);
  sp.ringPos_EqualTrailingIdx = ringPosL_EqualTrailingIdx;
  ringPosL_ShadowVeryShortTrailingIdx = sp.ringPos_ShadowVeryShortTrailingIdx;
  sp.ring_ShadowVeryShortTrailingIdx_derived[ringPosL_ShadowVeryShortTrailingIdx] = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  ringPosL_ShadowVeryShortTrailingIdx = ringPosL_ShadowVeryShortTrailingIdx + 1;
  ringPosL_ShadowVeryShortTrailingIdx = ringPosL_ShadowVeryShortTrailingIdx & (ringPosL_ShadowVeryShortTrailingIdx - sp.ringCap_ShadowVeryShortTrailingIdx >> 31);
  sp.ringPos_ShadowVeryShortTrailingIdx = ringPosL_ShadowVeryShortTrailingIdx;
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
  let ShadowVeryShortPeriodTotalRange0 = 0;
  let ShadowVeryShortPeriodTotalTrailRange0 = 0;
  let ShadowVeryShortPeriodTotalRange1 = 0;
  let ShadowVeryShortPeriodTotalTrailRange1 = 0;
  let ShadowVeryShortPeriodTotalRange2 = 0;
  let ShadowVeryShortPeriodTotalTrailRange2 = 0;
  let EqualPeriodTotalRange1 = 0;
  let EqualPeriodTotalTrailRange1 = 0;
  let EqualPeriodTotalRange2 = 0;
  let EqualPeriodTotalTrailRange2 = 0;
  let i = 0;
  let outIdx = 0;
  let ShadowVeryShortTrailingIdx = 0;
  let EqualTrailingIdx = 0;
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
  const Equal_rangeType: number = core.candleSettings[CandleSettingType.EQUAL].rangeType;
  const Equal_avgPeriod = core.candleSettings[CandleSettingType.EQUAL].avgPeriod;
  const Equal_factor = core.candleSettings[CandleSettingType.EQUAL].factor;
  const ShadowVeryShort_rangeType: number = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].rangeType;
  const ShadowVeryShort_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].avgPeriod;
  const ShadowVeryShort_factor = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].factor;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlidentical3crowsLookbackPositional(core);
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
  ShadowVeryShortPeriodTotalRange1 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
  ShadowVeryShortPeriodTotalTrailRange1 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx - 1] - inOpen[ShadowVeryShortTrailingIdx - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[ShadowVeryShortTrailingIdx - 1] - inLow[ShadowVeryShortTrailingIdx - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[ShadowVeryShortTrailingIdx - 1] - (((inClose[ShadowVeryShortTrailingIdx - 1]) >= (inOpen[ShadowVeryShortTrailingIdx - 1])) ? (inClose[ShadowVeryShortTrailingIdx - 1]) : (inOpen[ShadowVeryShortTrailingIdx - 1]))) + ((((inClose[ShadowVeryShortTrailingIdx - 1]) >= (inOpen[ShadowVeryShortTrailingIdx - 1])) ? (inOpen[ShadowVeryShortTrailingIdx - 1]) : (inClose[ShadowVeryShortTrailingIdx - 1])) - inLow[ShadowVeryShortTrailingIdx - 1])) : 0.0)));
  ShadowVeryShortPeriodTotalRange2 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
  ShadowVeryShortPeriodTotalTrailRange2 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx - 2] - inOpen[ShadowVeryShortTrailingIdx - 2])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[ShadowVeryShortTrailingIdx - 2] - inLow[ShadowVeryShortTrailingIdx - 2]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[ShadowVeryShortTrailingIdx - 2] - (((inClose[ShadowVeryShortTrailingIdx - 2]) >= (inOpen[ShadowVeryShortTrailingIdx - 2])) ? (inClose[ShadowVeryShortTrailingIdx - 2]) : (inOpen[ShadowVeryShortTrailingIdx - 2]))) + ((((inClose[ShadowVeryShortTrailingIdx - 2]) >= (inOpen[ShadowVeryShortTrailingIdx - 2])) ? (inOpen[ShadowVeryShortTrailingIdx - 2]) : (inClose[ShadowVeryShortTrailingIdx - 2])) - inLow[ShadowVeryShortTrailingIdx - 2])) : 0.0)));
  EqualPeriodTotalRange2 = ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
  EqualPeriodTotalTrailRange2 = ((Equal_rangeType === 0) ? (Math.abs(inClose[EqualTrailingIdx - 2] - inOpen[EqualTrailingIdx - 2])) : ((Equal_rangeType === 1) ? (inHigh[EqualTrailingIdx - 2] - inLow[EqualTrailingIdx - 2]) : ((Equal_rangeType === 2) ? ((inHigh[EqualTrailingIdx - 2] - (((inClose[EqualTrailingIdx - 2]) >= (inOpen[EqualTrailingIdx - 2])) ? (inClose[EqualTrailingIdx - 2]) : (inOpen[EqualTrailingIdx - 2]))) + ((((inClose[EqualTrailingIdx - 2]) >= (inOpen[EqualTrailingIdx - 2])) ? (inOpen[EqualTrailingIdx - 2]) : (inClose[EqualTrailingIdx - 2])) - inLow[EqualTrailingIdx - 2])) : 0.0)));
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
    ShadowVeryShortPeriodTotalRange0 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    ShadowVeryShortPeriodTotalTrailRange0 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx] - inOpen[ShadowVeryShortTrailingIdx])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[ShadowVeryShortTrailingIdx] - inLow[ShadowVeryShortTrailingIdx]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[ShadowVeryShortTrailingIdx] - (((inClose[ShadowVeryShortTrailingIdx]) >= (inOpen[ShadowVeryShortTrailingIdx])) ? (inClose[ShadowVeryShortTrailingIdx]) : (inOpen[ShadowVeryShortTrailingIdx]))) + ((((inClose[ShadowVeryShortTrailingIdx]) >= (inOpen[ShadowVeryShortTrailingIdx])) ? (inOpen[ShadowVeryShortTrailingIdx]) : (inClose[ShadowVeryShortTrailingIdx])) - inLow[ShadowVeryShortTrailingIdx])) : 0.0)));
    ShadowVeryShortPeriodTotal[2] = ShadowVeryShortPeriodTotal[2] + (ShadowVeryShortPeriodTotalRange2 - ShadowVeryShortPeriodTotalTrailRange2);
    ShadowVeryShortPeriodTotal[1] = ShadowVeryShortPeriodTotal[1] + (ShadowVeryShortPeriodTotalRange1 - ShadowVeryShortPeriodTotalTrailRange1);
    ShadowVeryShortPeriodTotal[0] = ShadowVeryShortPeriodTotal[0] + (ShadowVeryShortPeriodTotalRange0 - ShadowVeryShortPeriodTotalTrailRange0);
    ShadowVeryShortPeriodTotalRange2 = ShadowVeryShortPeriodTotalRange1;
    ShadowVeryShortPeriodTotalTrailRange2 = ShadowVeryShortPeriodTotalTrailRange1;
    ShadowVeryShortPeriodTotalRange1 = ShadowVeryShortPeriodTotalRange0;
    ShadowVeryShortPeriodTotalTrailRange1 = ShadowVeryShortPeriodTotalTrailRange0;
    EqualPeriodTotalRange1 = ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    EqualPeriodTotalTrailRange1 = ((Equal_rangeType === 0) ? (Math.abs(inClose[EqualTrailingIdx - 1] - inOpen[EqualTrailingIdx - 1])) : ((Equal_rangeType === 1) ? (inHigh[EqualTrailingIdx - 1] - inLow[EqualTrailingIdx - 1]) : ((Equal_rangeType === 2) ? ((inHigh[EqualTrailingIdx - 1] - (((inClose[EqualTrailingIdx - 1]) >= (inOpen[EqualTrailingIdx - 1])) ? (inClose[EqualTrailingIdx - 1]) : (inOpen[EqualTrailingIdx - 1]))) + ((((inClose[EqualTrailingIdx - 1]) >= (inOpen[EqualTrailingIdx - 1])) ? (inOpen[EqualTrailingIdx - 1]) : (inClose[EqualTrailingIdx - 1])) - inLow[EqualTrailingIdx - 1])) : 0.0)));
    EqualPeriodTotal[2] = EqualPeriodTotal[2] + (EqualPeriodTotalRange2 - EqualPeriodTotalTrailRange2);
    EqualPeriodTotal[1] = EqualPeriodTotal[1] + (EqualPeriodTotalRange1 - EqualPeriodTotalTrailRange1);
    EqualPeriodTotalRange2 = EqualPeriodTotalRange1;
    EqualPeriodTotalTrailRange2 = EqualPeriodTotalTrailRange1;
    i += 1;
    ShadowVeryShortTrailingIdx += 1;
    EqualTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
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
  sp.EqualPeriodTotal = EqualPeriodTotal;
  sp.ShadowVeryShortPeriodTotalRange1 = ShadowVeryShortPeriodTotalRange1;
  sp.ShadowVeryShortPeriodTotalTrailRange1 = ShadowVeryShortPeriodTotalTrailRange1;
  sp.ShadowVeryShortPeriodTotalRange2 = ShadowVeryShortPeriodTotalRange2;
  sp.ShadowVeryShortPeriodTotalTrailRange2 = ShadowVeryShortPeriodTotalTrailRange2;
  sp.EqualPeriodTotalRange2 = EqualPeriodTotalRange2;
  sp.EqualPeriodTotalTrailRange2 = EqualPeriodTotalTrailRange2;
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
  sp.ringPos_ShadowVeryShortTrailingIdx = 0;
  sp.ringCap_ShadowVeryShortTrailingIdx = cap_ShadowVeryShortTrailingIdx;
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
    throw insufficientHistory('CDLIDENTICAL3CROWS openAndFill', inOpen.length, startIdx, cdlidentical3crowsLookbackPositional(core));
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
    throw insufficientHistory('CDLIDENTICAL3CROWS open', inOpen.length, startIdx, cdlidentical3crowsLookbackPositional(core));
  }
  throw streamFailure('CDLIDENTICAL3CROWS open', retCode);
}

/** @internal */
export function cdlidentical3crowsOpenPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): Cdlidentical3crowsStream {
  requireArgument('CDLIDENTICAL3CROWS open', 'prices.open', inOpen);
  requireHistory('CDLIDENTICAL3CROWS open', 'prices.open', inOpen.length);
  requireArgument('CDLIDENTICAL3CROWS open', 'prices.high', inHigh);
  requireArgument('CDLIDENTICAL3CROWS open', 'prices.low', inLow);
  requireArgument('CDLIDENTICAL3CROWS open', 'prices.close', inClose);
  requireHistoryLength('CDLIDENTICAL3CROWS open', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLIDENTICAL3CROWS open', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLIDENTICAL3CROWS open', 'prices.close', inClose.length, inOpen.length);
  return cdlidentical3crowsOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/** @internal */
export function cdlidentical3crowsOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): Cdlidentical3crowsStream {
  requireArgument('CDLIDENTICAL3CROWS openAndFill', 'prices.open', inOpen);
  requireHistory('CDLIDENTICAL3CROWS openAndFill', 'prices.open', inOpen.length);
  requireArgument('CDLIDENTICAL3CROWS openAndFill', 'prices.high', inHigh);
  requireArgument('CDLIDENTICAL3CROWS openAndFill', 'prices.low', inLow);
  requireArgument('CDLIDENTICAL3CROWS openAndFill', 'prices.close', inClose);
  const guardOutLen = openFillCount('CDLIDENTICAL3CROWS openAndFill', inOpen.length, cdlidentical3crowsLookbackPositional(core));
  requireHistoryLength('CDLIDENTICAL3CROWS openAndFill', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLIDENTICAL3CROWS openAndFill', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLIDENTICAL3CROWS openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('CDLIDENTICAL3CROWS openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('CDLIDENTICAL3CROWS openAndFill', 'outInteger', 'prices.open');
  }
  if (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CDLIDENTICAL3CROWS openAndFill', 'outInteger', 'prices.high');
  }
  if (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CDLIDENTICAL3CROWS openAndFill', 'outInteger', 'prices.low');
  }
  if (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('CDLIDENTICAL3CROWS openAndFill', 'outInteger', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLIDENTICAL3CROWS openAndFill', 'outInteger', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLIDENTICAL3CROWS openAndFill', 'outInteger', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLIDENTICAL3CROWS openAndFill', 'outInteger', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLIDENTICAL3CROWS openAndFill', 'outInteger', 'prices.close');
  }
  return cdlidentical3crowsOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}

/**
 * Opens a live CDLIDENTICAL3CROWS stream over the warm-up history: `value()`
 * starts at the last history bar, bit-identical to {@link cdlidentical3crows}
 * there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link Cdlidentical3crowsOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlidentical3crowsLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input or histories of different lengths.
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
 * const stream = cdlidentical3crowsOpen({ open, high, low, close });
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdlidentical3crowsOpen(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: Cdlidentical3crowsOptions): Cdlidentical3crowsStream {
  requireArgument('CDLIDENTICAL3CROWS open', 'prices', prices);
  const o = optionsOf('CDLIDENTICAL3CROWS open', options);
  const core = coreOf('CDLIDENTICAL3CROWS open', o.core);
  return cdlidentical3crowsOpenPositional(prices.open, prices.high, prices.low, prices.close, core);
}

/**
 * {@link cdlidentical3crowsOpen} that also computes the outputs over the whole
 * history in the same pass, bit-identically to {@link cdlidentical3crows}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link Cdlidentical3crowsOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cdlidentical3crows} returns them; the range is
 *   the stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlidentical3crowsLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, histories of different lengths or a buffer too short,
 *   of the wrong type or starting where another buffer or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   buffer or a `Float64Array` input.
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
 * const { stream, begIdx, values } = cdlidentical3crowsOpenAndFill({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cdlidentical3crowsOpenAndFill(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: Cdlidentical3crowsOptions): Cdlidentical3crowsResult & { stream: Cdlidentical3crowsStream } {
  requireArgument('CDLIDENTICAL3CROWS openAndFill', 'prices', prices);
  const o = optionsOf('CDLIDENTICAL3CROWS openAndFill', options);
  const core = coreOf('CDLIDENTICAL3CROWS openAndFill', o.core);
  const capacity = fillCapacity(cdlidentical3crowsLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const stream = cdlidentical3crowsOpenAndFillPositional(prices.open, prices.high, prices.low, prices.close, outInteger, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}
