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
 *  022005 AC     Creation
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 *  103126 MF,CC  Nest the cheap, selective pre-check conjuncts in an outer if
 *  110526 MF,CC  Carry the ranges of the previous bars instead of recomputing them
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireReal, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { CandleSettingType, Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function cdlmatholdLookbackPositional(optInPenetration: number, core: Core = Core.DEFAULT): number {
  if (typeof optInPenetration !== 'number') {
    return -1;
  }
  if (optInPenetration === Core.REAL_DEFAULT) {
    optInPenetration = 0.5;
  } else if (!(optInPenetration >= 0e0 && optInPenetration <= Core.REAL_MAX)) {
    return -1;
  }
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  const BodyShort_avgPeriod = core.candleSettings[CandleSettingType.BODY_SHORT].avgPeriod;
  return (BodyShort_avgPeriod > BodyLong_avgPeriod ? BodyShort_avgPeriod : BodyLong_avgPeriod) + 4;
}

/** @internal */
export function cdlmatholdDisplayShiftPositional(optInPenetration: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (cdlmatholdLookbackPositional(optInPenetration, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdlmatholdImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  optInPenetration: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): RetCode {
  const BodyPeriodTotal = new Float64Array(5);
  let BodyPeriodTotalRange1 = 0;
  let BodyPeriodTotalTrailRange1 = 0;
  let BodyPeriodTotalRange2 = 0;
  let BodyPeriodTotalTrailRange2 = 0;
  let BodyPeriodTotalRange3 = 0;
  let BodyPeriodTotalTrailRange3 = 0;
  let i = 0;
  let outIdx = 0;
  let BodyShortTrailingIdx = 0;
  let BodyLongTrailingIdx = 0;
  let lookbackTotal = 0;
  const BodyLong_rangeType: number = core.candleSettings[CandleSettingType.BODY_LONG].rangeType;
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  const BodyLong_factor = core.candleSettings[CandleSettingType.BODY_LONG].factor;
  const BodyShort_rangeType: number = core.candleSettings[CandleSettingType.BODY_SHORT].rangeType;
  const BodyShort_avgPeriod = core.candleSettings[CandleSettingType.BODY_SHORT].avgPeriod;
  const BodyShort_factor = core.candleSettings[CandleSettingType.BODY_SHORT].factor;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInPenetration === Core.REAL_DEFAULT) {
    optInPenetration = 0.5;
  } else if (!(optInPenetration >= 0e0 && optInPenetration <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlmatholdLookbackPositional(optInPenetration, core);
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
  BodyPeriodTotal[4] = 0;
  BodyPeriodTotal[3] = 0;
  BodyPeriodTotal[2] = 0;
  BodyPeriodTotal[1] = 0;
  BodyPeriodTotal[0] = 0;
  BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
  BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
  i = BodyShortTrailingIdx;
  while (i < startIdx) {
    BodyPeriodTotal[3] = BodyPeriodTotal[3] + ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)));
    BodyPeriodTotal[2] = BodyPeriodTotal[2] + ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
    BodyPeriodTotal[1] = BodyPeriodTotal[1] + ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    i += 1;
  }
  i = BodyLongTrailingIdx;
  while (i < startIdx) {
    BodyPeriodTotal[4] = BodyPeriodTotal[4] + ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first candle: long white candle
   * - upside gap between the first and the second bodies
   * - second candle: small black candle
   * - third and fourth candles: falling small real body candlesticks (commonly black) that hold within the long
   *   white candle's body and are higher than the reaction days of the rising three methods
   * - fifth candle: white candle that opens above the previous small candle's close and closes higher than the
   *   high of the highest reaction day
   * The meaning of "short" and "long" is specified with TA_SetCandleSettings;
   * "hold within" means "a part of the real body must be within";
   * optInPenetration is the maximum percentage of the first white body the reaction days can penetrate (it is
   * to specify how much the reaction days should be "higher than the reaction days of the rising three methods")
   */
  outIdx = 0;
  BodyPeriodTotalRange2 = ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
  BodyPeriodTotalTrailRange2 = ((BodyShort_rangeType === 0) ? (Math.abs(inClose[BodyShortTrailingIdx - 2] - inOpen[BodyShortTrailingIdx - 2])) : ((BodyShort_rangeType === 1) ? (inHigh[BodyShortTrailingIdx - 2] - inLow[BodyShortTrailingIdx - 2]) : ((BodyShort_rangeType === 2) ? ((inHigh[BodyShortTrailingIdx - 2] - (((inClose[BodyShortTrailingIdx - 2]) >= (inOpen[BodyShortTrailingIdx - 2])) ? (inClose[BodyShortTrailingIdx - 2]) : (inOpen[BodyShortTrailingIdx - 2]))) + ((((inClose[BodyShortTrailingIdx - 2]) >= (inOpen[BodyShortTrailingIdx - 2])) ? (inOpen[BodyShortTrailingIdx - 2]) : (inClose[BodyShortTrailingIdx - 2])) - inLow[BodyShortTrailingIdx - 2])) : 0.0)));
  BodyPeriodTotalRange3 = ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)));
  BodyPeriodTotalTrailRange3 = ((BodyShort_rangeType === 0) ? (Math.abs(inClose[BodyShortTrailingIdx - 3] - inOpen[BodyShortTrailingIdx - 3])) : ((BodyShort_rangeType === 1) ? (inHigh[BodyShortTrailingIdx - 3] - inLow[BodyShortTrailingIdx - 3]) : ((BodyShort_rangeType === 2) ? ((inHigh[BodyShortTrailingIdx - 3] - (((inClose[BodyShortTrailingIdx - 3]) >= (inOpen[BodyShortTrailingIdx - 3])) ? (inClose[BodyShortTrailingIdx - 3]) : (inOpen[BodyShortTrailingIdx - 3]))) + ((((inClose[BodyShortTrailingIdx - 3]) >= (inOpen[BodyShortTrailingIdx - 3])) ? (inOpen[BodyShortTrailingIdx - 3]) : (inClose[BodyShortTrailingIdx - 3])) - inLow[BodyShortTrailingIdx - 3])) : 0.0)));
  do {
    if (((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) === 1 &&  /* white, black, 2 black or white, white */
        ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : -1) === -1 &&
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === 1 &&
        ((inOpen[i - 3] < inClose[i - 3] ? inOpen[i - 3] : inClose[i - 3]) > (inOpen[i - 4] > inClose[i - 4] ? inOpen[i - 4] : inClose[i - 4])) && /* upside gap 1st to 2nd */
        (inOpen[i - 2] < inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2]) < inClose[i - 4] && /* 3rd to 4th hold within 1st: a part of the real body must be within 1st real body */
        (inOpen[i - 1] < inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1]) < inClose[i - 4] &&
        (inOpen[i - 2] < inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2]) > inClose[i - 4] - Math.abs(inClose[i - 4] - inOpen[i - 4]) * optInPenetration && /* reaction days penetrate first body less than optInPenetration percent */
        (inOpen[i - 1] < inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1]) > inClose[i - 4] - Math.abs(inClose[i - 4] - inOpen[i - 4]) * optInPenetration &&
        (inClose[i - 2] > inOpen[i - 2] ? inClose[i - 2] : inOpen[i - 2]) < inOpen[i - 3] && /* 2nd to 4th are falling */
        (inClose[i - 1] > inOpen[i - 1] ? inClose[i - 1] : inOpen[i - 1]) < (inClose[i - 2] > inOpen[i - 2] ? inClose[i - 2] : inOpen[i - 2]) &&
        inOpen[i] > inClose[i - 1] &&                          /* 5th opens above the prior close */
        inClose[i] > ((inHigh[i - 3] > inHigh[i - 2] ? inHigh[i - 3] : inHigh[i - 2]) > inHigh[i - 1] ? (inHigh[i - 3] > inHigh[i - 2] ? inHigh[i - 3] : inHigh[i - 2]) : inHigh[i - 1]) && /* 5th closes above the highest high of the reaction days */
        Math.abs(inClose[i - 4] - inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyPeriodTotal[4] / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st long, then 3 small */
        Math.abs(inClose[i - 3] - inOpen[i - 3]) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (BodyPeriodTotal[3] / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) &&
        Math.abs(inClose[i - 2] - inOpen[i - 2]) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (BodyPeriodTotal[2] / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) &&
        Math.abs(inClose[i - 1] - inOpen[i - 1]) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (BodyPeriodTotal[1] / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))))
    {
      outInteger[outIdx++] = 100;
    } else {
      outInteger[outIdx++] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyPeriodTotal[4] = BodyPeriodTotal[4] + (((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0))) - ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 4] - inOpen[BodyLongTrailingIdx - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyLongTrailingIdx - 4] - inLow[BodyLongTrailingIdx - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyLongTrailingIdx - 4] - (((inClose[BodyLongTrailingIdx - 4]) >= (inOpen[BodyLongTrailingIdx - 4])) ? (inClose[BodyLongTrailingIdx - 4]) : (inOpen[BodyLongTrailingIdx - 4]))) + ((((inClose[BodyLongTrailingIdx - 4]) >= (inOpen[BodyLongTrailingIdx - 4])) ? (inOpen[BodyLongTrailingIdx - 4]) : (inClose[BodyLongTrailingIdx - 4])) - inLow[BodyLongTrailingIdx - 4])) : 0.0))));
    BodyPeriodTotalRange1 = ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    BodyPeriodTotalTrailRange1 = ((BodyShort_rangeType === 0) ? (Math.abs(inClose[BodyShortTrailingIdx - 1] - inOpen[BodyShortTrailingIdx - 1])) : ((BodyShort_rangeType === 1) ? (inHigh[BodyShortTrailingIdx - 1] - inLow[BodyShortTrailingIdx - 1]) : ((BodyShort_rangeType === 2) ? ((inHigh[BodyShortTrailingIdx - 1] - (((inClose[BodyShortTrailingIdx - 1]) >= (inOpen[BodyShortTrailingIdx - 1])) ? (inClose[BodyShortTrailingIdx - 1]) : (inOpen[BodyShortTrailingIdx - 1]))) + ((((inClose[BodyShortTrailingIdx - 1]) >= (inOpen[BodyShortTrailingIdx - 1])) ? (inOpen[BodyShortTrailingIdx - 1]) : (inClose[BodyShortTrailingIdx - 1])) - inLow[BodyShortTrailingIdx - 1])) : 0.0)));
    BodyPeriodTotal[3] = BodyPeriodTotal[3] + (BodyPeriodTotalRange3 - BodyPeriodTotalTrailRange3);
    BodyPeriodTotal[2] = BodyPeriodTotal[2] + (BodyPeriodTotalRange2 - BodyPeriodTotalTrailRange2);
    BodyPeriodTotal[1] = BodyPeriodTotal[1] + (BodyPeriodTotalRange1 - BodyPeriodTotalTrailRange1);
    BodyPeriodTotalRange3 = BodyPeriodTotalRange2;
    BodyPeriodTotalTrailRange3 = BodyPeriodTotalTrailRange2;
    BodyPeriodTotalRange2 = BodyPeriodTotalRange1;
    BodyPeriodTotalTrailRange2 = BodyPeriodTotalTrailRange1;
    i += 1;
    BodyShortTrailingIdx += 1;
    BodyLongTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['penetration', Core.REAL_DEFAULT, 0e0, Core.REAL_MAX, 'in 0..3e37']] as const;

/** @internal */
export function cdlmatholdPositional(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInPenetration: number,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDLMATHOLD', 'startIdx', startIdx);
  endIdx = requireInt32('CDLMATHOLD', 'endIdx', endIdx);
  optInPenetration = requireReal('CDLMATHOLD', 'penetration', optInPenetration);
  requireIndexRange('CDLMATHOLD', startIdx, endIdx);
  const guardLookback = cdlmatholdLookbackPositional(optInPenetration, core);
  if (guardLookback < 0) {
    throw rejectedParams('CDLMATHOLD', PARAM_RANGES, optInPenetration);
  }
  const guardStart = clampedStart('CDLMATHOLD', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLMATHOLD', 'prices.open', inOpen, guardInLen);
  requireLength('CDLMATHOLD', 'prices.high', inHigh, guardInLen);
  requireLength('CDLMATHOLD', 'prices.low', inLow, guardInLen);
  requireLength('CDLMATHOLD', 'prices.close', inClose, guardInLen);
  requireOutput('CDLMATHOLD', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLMATHOLD', 'outInteger', 'prices.open');
  }
  if (overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLMATHOLD', 'outInteger', 'prices.high');
  }
  if (overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLMATHOLD', 'outInteger', 'prices.low');
  }
  if (overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLMATHOLD', 'outInteger', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdlmatholdImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, optInPenetration, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLMATHOLD', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cdlmathold}, {@link cdlmatholdLookback},
 * {@link cdlmatholdDisplayShift}, {@link cdlmatholdOpen} and
 * {@link cdlmatholdOpenAndFill}. Every key is optional: an omitted or `undefined`
 * key takes its default, and a call ignores the keys it has no use for.
 */
export interface CdlmatholdOptions {
  /**
   * Max fraction of the 1st white body the reaction days (3rd, 4th) may penetrate
   * (default 0.5; minimum 0; `Core.REAL_DEFAULT` selects the default).
   */
  penetration?: number | undefined;
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
 * What {@link cdlmathold} returns, and {@link cdlmatholdOpenAndFill} with its
 * stream.
 */
export interface CdlmatholdResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * +100 when the bullish Mat Hold is detected, 0 otherwise. Never emits -100.
   * `values[k]` is the value at input bar `begIdx + k`.
   */
  values: Int32Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link cdlmathold} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlmatholdOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlmatholdLookback(options?: CdlmatholdOptions): number {
  const o = optionsOf('CDLMATHOLD lookback', options);
  return cdlmatholdLookbackPositional(orDefault(o.penetration, Core.REAL_DEFAULT), coreOf('CDLMATHOLD lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdlmathold}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlmatholdOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlmatholdDisplayShift(outputIdx: number, options?: CdlmatholdOptions): number {
  const o = optionsOf('CDLMATHOLD displayShift', options);
  return cdlmatholdDisplayShiftPositional(orDefault(o.penetration, Core.REAL_DEFAULT), outputIdx, coreOf('CDLMATHOLD displayShift', o.core));
}

/**
 * A five-candle bullish continuation pattern: a long white candle, an upside
 * real-body-gapped small black candle, two more small falling candles that hold
 * within the first body, and a final white candle closing above the reaction days'
 * highs. Signals continuation of the prior uptrend. Hit = bullish continuation of
 * the existing uptrend.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlmathold](https://ta-lib.org/functions/cdlmathold).
 *
 * **Notes**
 * - The colors of the third and fourth (reaction) candles are not checked,
 *   although they are classically black.
 * - The continuation reading assumes a prior uptrend, which is not verified.
 * - Bulkowski's own dataset contains only 52 Mat Hold occurrences out of 4.7
 *   million candle lines; he explicitly warns the 78% continuation rate he
 *   measured "will likely be wrong or at least subject to large change as
 *   additional samples become available."
 *   ([thepatternsite.com](https://thepatternsite.com/MatHold.html))
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cdlmatholdLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `open`: Open price of
 *   each bar. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link CdlmatholdOptions}.
 * @returns `begIdx` and the values; see {@link CdlmatholdResult}. Each output is a
 *   view of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, a real parameter that is not a number, a parameter outside its range, an
 *   input too short for the range, a buffer that is empty, too short or of the wrong
 *   type or a buffer sharing memory with a `Float64Array` input it is not. Checked
 *   before anything is written.
 *
 * @example
 * ```ts
 * import { cdlmathold } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { begIdx, values } = cdlmathold({ open, high, low, close }, {
 *   penetration: 0.5,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`cdlrisefall3methods`](https://ta-lib.org/functions/cdlrisefall3methods)
 * @see [`cdlxsidegap3methods`](https://ta-lib.org/functions/cdlxsidegap3methods)
 */
export function cdlmathold(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlmatholdOptions): CdlmatholdResult {
  requireArgument('CDLMATHOLD', 'prices', prices);
  const o = optionsOf('CDLMATHOLD', options);
  if (o.endIdx === undefined) {
    requireSeries('CDLMATHOLD', 'prices.open', prices.open);
    requireSeries('CDLMATHOLD', 'prices.high', prices.high);
    requireSeries('CDLMATHOLD', 'prices.low', prices.low);
    requireSeries('CDLMATHOLD', 'prices.close', prices.close);
  }
  const optInPenetration = orDefault(o.penetration, Core.REAL_DEFAULT);
  const core = coreOf('CDLMATHOLD', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cdlmatholdLookbackPositional(optInPenetration, core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const range = cdlmatholdPositional(startIdx, endIdx, prices.open, prices.high, prices.low, prices.close, optInPenetration, outInteger, core);
  return { begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CDLMATHOLD stream: one value per closed bar, bit-identical to
 * {@link cdlmathold} over the same series. Open it with {@link cdlmatholdOpen} or
 * {@link cdlmatholdOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlmathold](https://ta-lib.org/functions/cdlmathold).
 */
export class CdlmatholdStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInPenetration: number = 0;
  /** @internal */
  BodyPeriodTotal: Float64Array = new Float64Array(5);
  /** @internal */
  BodyPeriodTotalRange2: number = 0;
  /** @internal */
  BodyPeriodTotalTrailRange2: number = 0;
  /** @internal */
  BodyPeriodTotalRange3: number = 0;
  /** @internal */
  BodyPeriodTotalTrailRange3: number = 0;
  /** @internal */
  lag1_inOpen: number = 0;
  /** @internal */
  lag2_inOpen: number = 0;
  /** @internal */
  lag3_inOpen: number = 0;
  /** @internal */
  lag4_inOpen: number = 0;
  /** @internal */
  lag1_inHigh: number = 0;
  /** @internal */
  lag2_inHigh: number = 0;
  /** @internal */
  lag3_inHigh: number = 0;
  /** @internal */
  lag4_inHigh: number = 0;
  /** @internal */
  lag1_inLow: number = 0;
  /** @internal */
  lag2_inLow: number = 0;
  /** @internal */
  lag3_inLow: number = 0;
  /** @internal */
  lag4_inLow: number = 0;
  /** @internal */
  lag1_inClose: number = 0;
  /** @internal */
  lag2_inClose: number = 0;
  /** @internal */
  lag3_inClose: number = 0;
  /** @internal */
  lag4_inClose: number = 0;
  /** @internal */
  ringPos_BodyLongTrailingIdx: number = 0;
  /** @internal */
  ringCap_BodyLongTrailingIdx: number = 5;
  /** @internal */
  ringLag_BodyLongTrailingIdx: number = 0;
  /** @internal */
  ring_BodyLongTrailingIdx_derived: Float64Array = new Float64Array(5);
  /** @internal */
  ringPos_BodyShortTrailingIdx: number = 0;
  /** @internal */
  ringCap_BodyShortTrailingIdx: number = 2;
  /** @internal */
  ringLag_BodyShortTrailingIdx: number = 0;
  /** @internal */
  ring_BodyShortTrailingIdx_derived: Float64Array = new Float64Array(2);
  /** @internal */
  cs_BodyLong_rangeType: number = 0;
  /** @internal */
  cs_BodyLong_avgPeriod: number = 0;
  /** @internal */
  cs_BodyLong_factor: number = 0;
  /** @internal */
  cs_BodyShort_rangeType: number = 0;
  /** @internal */
  cs_BodyShort_avgPeriod: number = 0;
  /** @internal */
  cs_BodyShort_factor: number = 0;
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
  static of(core: Core): CdlmatholdStream {
    return new CdlmatholdStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cdlmathold} reports them over the same bars. Each accepted `update` and
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
      throw streamFailure('CDLMATHOLD advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw streamFailure('CDLMATHOLD update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(open) || !Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('CDLMATHOLD update', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    cdlmatholdStepImpl(this, open, high, low, close);
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
      throw nonFiniteBar('CDLMATHOLD peek', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inOpen = open;
    const inClose = close;
    let cur_outInteger = 0;
    const BodyLong_rangeType = sp.cs_BodyLong_rangeType;
    const BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
    const BodyLong_factor = sp.cs_BodyLong_factor;
    const BodyShort_rangeType = sp.cs_BodyShort_rangeType;
    const BodyShort_avgPeriod = sp.cs_BodyShort_avgPeriod;
    const BodyShort_factor = sp.cs_BodyShort_factor;
    if (((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) === 1 &&  /* white, black, 2 black or white, white */
        ((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : -1) === -1 &&
        ((inClose >= inOpen) ? 1 : -1) === 1 &&
        ((sp.lag3_inOpen < sp.lag3_inClose ? sp.lag3_inOpen : sp.lag3_inClose) > (sp.lag4_inOpen > sp.lag4_inClose ? sp.lag4_inOpen : sp.lag4_inClose)) && /* upside gap 1st to 2nd */
        (sp.lag2_inOpen < sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose) < sp.lag4_inClose && /* 3rd to 4th hold within 1st: a part of the real body must be within 1st real body */
        (sp.lag1_inOpen < sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose) < sp.lag4_inClose &&
        (sp.lag2_inOpen < sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose) > sp.lag4_inClose - Math.abs(sp.lag4_inClose - sp.lag4_inOpen) * sp.optInPenetration && /* reaction days penetrate first body less than optInPenetration percent */
        (sp.lag1_inOpen < sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose) > sp.lag4_inClose - Math.abs(sp.lag4_inClose - sp.lag4_inOpen) * sp.optInPenetration &&
        (sp.lag2_inClose > sp.lag2_inOpen ? sp.lag2_inClose : sp.lag2_inOpen) < sp.lag3_inOpen && /* 2nd to 4th are falling */
        (sp.lag1_inClose > sp.lag1_inOpen ? sp.lag1_inClose : sp.lag1_inOpen) < (sp.lag2_inClose > sp.lag2_inOpen ? sp.lag2_inClose : sp.lag2_inOpen) &&
        inOpen > sp.lag1_inClose &&                              /* 5th opens above the prior close */
        inClose > ((sp.lag3_inHigh > sp.lag2_inHigh ? sp.lag3_inHigh : sp.lag2_inHigh) > sp.lag1_inHigh ? (sp.lag3_inHigh > sp.lag2_inHigh ? sp.lag3_inHigh : sp.lag2_inHigh) : sp.lag1_inHigh) && /* 5th closes above the highest high of the reaction days */
        Math.abs(sp.lag4_inClose - sp.lag4_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyPeriodTotal[4] / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag4_inClose - sp.lag4_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag4_inHigh - sp.lag4_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag4_inHigh - (((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inClose) : (sp.lag4_inOpen))) + ((((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inOpen) : (sp.lag4_inClose)) - sp.lag4_inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st long, then 3 small */
        Math.abs(sp.lag3_inClose - sp.lag3_inOpen) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (sp.BodyPeriodTotal[3] / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(sp.lag3_inClose - sp.lag3_inOpen)) : ((BodyShort_rangeType === 1) ? (sp.lag3_inHigh - sp.lag3_inLow) : ((BodyShort_rangeType === 2) ? ((sp.lag3_inHigh - (((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inClose) : (sp.lag3_inOpen))) + ((((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inOpen) : (sp.lag3_inClose)) - sp.lag3_inLow)) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) &&
        Math.abs(sp.lag2_inClose - sp.lag2_inOpen) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (sp.BodyPeriodTotal[2] / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyShort_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyShort_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) &&
        Math.abs(sp.lag1_inClose - sp.lag1_inOpen) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (sp.BodyPeriodTotal[1] / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyShort_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyShort_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))))
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
  clone(): CdlmatholdStream {
    const c = new CdlmatholdStream(this.core);
    c.optInPenetration = this.optInPenetration;
    c.BodyPeriodTotal = this.BodyPeriodTotal.slice();
    c.BodyPeriodTotalRange2 = this.BodyPeriodTotalRange2;
    c.BodyPeriodTotalTrailRange2 = this.BodyPeriodTotalTrailRange2;
    c.BodyPeriodTotalRange3 = this.BodyPeriodTotalRange3;
    c.BodyPeriodTotalTrailRange3 = this.BodyPeriodTotalTrailRange3;
    c.lag1_inOpen = this.lag1_inOpen;
    c.lag2_inOpen = this.lag2_inOpen;
    c.lag3_inOpen = this.lag3_inOpen;
    c.lag4_inOpen = this.lag4_inOpen;
    c.lag1_inHigh = this.lag1_inHigh;
    c.lag2_inHigh = this.lag2_inHigh;
    c.lag3_inHigh = this.lag3_inHigh;
    c.lag4_inHigh = this.lag4_inHigh;
    c.lag1_inLow = this.lag1_inLow;
    c.lag2_inLow = this.lag2_inLow;
    c.lag3_inLow = this.lag3_inLow;
    c.lag4_inLow = this.lag4_inLow;
    c.lag1_inClose = this.lag1_inClose;
    c.lag2_inClose = this.lag2_inClose;
    c.lag3_inClose = this.lag3_inClose;
    c.lag4_inClose = this.lag4_inClose;
    c.ringPos_BodyLongTrailingIdx = this.ringPos_BodyLongTrailingIdx;
    c.ringCap_BodyLongTrailingIdx = this.ringCap_BodyLongTrailingIdx;
    c.ringLag_BodyLongTrailingIdx = this.ringLag_BodyLongTrailingIdx;
    c.ring_BodyLongTrailingIdx_derived = this.ring_BodyLongTrailingIdx_derived.slice();
    c.ringPos_BodyShortTrailingIdx = this.ringPos_BodyShortTrailingIdx;
    c.ringCap_BodyShortTrailingIdx = this.ringCap_BodyShortTrailingIdx;
    c.ringLag_BodyShortTrailingIdx = this.ringLag_BodyShortTrailingIdx;
    c.ring_BodyShortTrailingIdx_derived = this.ring_BodyShortTrailingIdx_derived.slice();
    c.cs_BodyLong_rangeType = this.cs_BodyLong_rangeType;
    c.cs_BodyLong_avgPeriod = this.cs_BodyLong_avgPeriod;
    c.cs_BodyLong_factor = this.cs_BodyLong_factor;
    c.cs_BodyShort_rangeType = this.cs_BodyShort_rangeType;
    c.cs_BodyShort_avgPeriod = this.cs_BodyShort_avgPeriod;
    c.cs_BodyShort_factor = this.cs_BodyShort_factor;
    c.cur_outInteger = this.cur_outInteger;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cdlmatholdStepImpl(sp: CdlmatholdStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  let BodyPeriodTotalRange1 = 0;
  let BodyPeriodTotalTrailRange1 = 0;
  const BodyLong_rangeType = sp.cs_BodyLong_rangeType;
  const BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
  const BodyLong_factor = sp.cs_BodyLong_factor;
  const BodyShort_rangeType = sp.cs_BodyShort_rangeType;
  const BodyShort_avgPeriod = sp.cs_BodyShort_avgPeriod;
  const BodyShort_factor = sp.cs_BodyShort_factor;
  sp.ring_BodyLongTrailingIdx_derived[sp.ringPos_BodyLongTrailingIdx] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType === 1) ? (inHigh - inLow) : ((BodyLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  sp.ring_BodyShortTrailingIdx_derived[sp.ringPos_BodyShortTrailingIdx] = ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  if (((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) === 1 &&  /* white, black, 2 black or white, white */
      ((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : -1) === -1 &&
      ((inClose >= inOpen) ? 1 : -1) === 1 &&
      ((sp.lag3_inOpen < sp.lag3_inClose ? sp.lag3_inOpen : sp.lag3_inClose) > (sp.lag4_inOpen > sp.lag4_inClose ? sp.lag4_inOpen : sp.lag4_inClose)) && /* upside gap 1st to 2nd */
      (sp.lag2_inOpen < sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose) < sp.lag4_inClose && /* 3rd to 4th hold within 1st: a part of the real body must be within 1st real body */
      (sp.lag1_inOpen < sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose) < sp.lag4_inClose &&
      (sp.lag2_inOpen < sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose) > sp.lag4_inClose - Math.abs(sp.lag4_inClose - sp.lag4_inOpen) * sp.optInPenetration && /* reaction days penetrate first body less than optInPenetration percent */
      (sp.lag1_inOpen < sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose) > sp.lag4_inClose - Math.abs(sp.lag4_inClose - sp.lag4_inOpen) * sp.optInPenetration &&
      (sp.lag2_inClose > sp.lag2_inOpen ? sp.lag2_inClose : sp.lag2_inOpen) < sp.lag3_inOpen && /* 2nd to 4th are falling */
      (sp.lag1_inClose > sp.lag1_inOpen ? sp.lag1_inClose : sp.lag1_inOpen) < (sp.lag2_inClose > sp.lag2_inOpen ? sp.lag2_inClose : sp.lag2_inOpen) &&
      inOpen > sp.lag1_inClose &&                              /* 5th opens above the prior close */
      inClose > ((sp.lag3_inHigh > sp.lag2_inHigh ? sp.lag3_inHigh : sp.lag2_inHigh) > sp.lag1_inHigh ? (sp.lag3_inHigh > sp.lag2_inHigh ? sp.lag3_inHigh : sp.lag2_inHigh) : sp.lag1_inHigh) && /* 5th closes above the highest high of the reaction days */
      Math.abs(sp.lag4_inClose - sp.lag4_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyPeriodTotal[4] / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag4_inClose - sp.lag4_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag4_inHigh - sp.lag4_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag4_inHigh - (((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inClose) : (sp.lag4_inOpen))) + ((((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inOpen) : (sp.lag4_inClose)) - sp.lag4_inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st long, then 3 small */
      Math.abs(sp.lag3_inClose - sp.lag3_inOpen) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (sp.BodyPeriodTotal[3] / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(sp.lag3_inClose - sp.lag3_inOpen)) : ((BodyShort_rangeType === 1) ? (sp.lag3_inHigh - sp.lag3_inLow) : ((BodyShort_rangeType === 2) ? ((sp.lag3_inHigh - (((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inClose) : (sp.lag3_inOpen))) + ((((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inOpen) : (sp.lag3_inClose)) - sp.lag3_inLow)) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) &&
      Math.abs(sp.lag2_inClose - sp.lag2_inOpen) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (sp.BodyPeriodTotal[2] / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyShort_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyShort_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) &&
      Math.abs(sp.lag1_inClose - sp.lag1_inOpen) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (sp.BodyPeriodTotal[1] / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyShort_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyShort_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))))
  {
    sp.cur_outInteger = 100;
  } else {
    sp.cur_outInteger = 0;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  sp.BodyPeriodTotal[4] = sp.BodyPeriodTotal[4] + (((BodyLong_rangeType === 0) ? (Math.abs(sp.lag4_inClose - sp.lag4_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag4_inHigh - sp.lag4_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag4_inHigh - (((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inClose) : (sp.lag4_inOpen))) + ((((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inOpen) : (sp.lag4_inClose)) - sp.lag4_inLow)) : 0.0))) - sp.ring_BodyLongTrailingIdx_derived[(((sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 4) % sp.ringCap_BodyLongTrailingIdx) | 0)]);
  BodyPeriodTotalRange1 = ((BodyShort_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyShort_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyShort_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)));
  BodyPeriodTotalTrailRange1 = sp.ring_BodyShortTrailingIdx_derived[(((sp.ringPos_BodyShortTrailingIdx + sp.ringCap_BodyShortTrailingIdx - sp.ringLag_BodyShortTrailingIdx - 1) % sp.ringCap_BodyShortTrailingIdx) | 0)];
  sp.BodyPeriodTotal[3] = sp.BodyPeriodTotal[3] + (sp.BodyPeriodTotalRange3 - sp.BodyPeriodTotalTrailRange3);
  sp.BodyPeriodTotal[2] = sp.BodyPeriodTotal[2] + (sp.BodyPeriodTotalRange2 - sp.BodyPeriodTotalTrailRange2);
  sp.BodyPeriodTotal[1] = sp.BodyPeriodTotal[1] + (BodyPeriodTotalRange1 - BodyPeriodTotalTrailRange1);
  sp.BodyPeriodTotalRange3 = sp.BodyPeriodTotalRange2;
  sp.BodyPeriodTotalTrailRange3 = sp.BodyPeriodTotalTrailRange2;
  sp.BodyPeriodTotalRange2 = BodyPeriodTotalRange1;
  sp.BodyPeriodTotalTrailRange2 = BodyPeriodTotalTrailRange1;
  sp.lag4_inOpen = sp.lag3_inOpen;
  sp.lag3_inOpen = sp.lag2_inOpen;
  sp.lag2_inOpen = sp.lag1_inOpen;
  sp.lag1_inOpen = inOpen;
  sp.lag4_inHigh = sp.lag3_inHigh;
  sp.lag3_inHigh = sp.lag2_inHigh;
  sp.lag2_inHigh = sp.lag1_inHigh;
  sp.lag1_inHigh = inHigh;
  sp.lag4_inLow = sp.lag3_inLow;
  sp.lag3_inLow = sp.lag2_inLow;
  sp.lag2_inLow = sp.lag1_inLow;
  sp.lag1_inLow = inLow;
  sp.lag4_inClose = sp.lag3_inClose;
  sp.lag3_inClose = sp.lag2_inClose;
  sp.lag2_inClose = sp.lag1_inClose;
  sp.lag1_inClose = inClose;
  sp.ringPos_BodyLongTrailingIdx = sp.ringPos_BodyLongTrailingIdx + 1;
  if (sp.ringPos_BodyLongTrailingIdx >= sp.ringCap_BodyLongTrailingIdx) {
    sp.ringPos_BodyLongTrailingIdx = 0;
  }
  sp.ringPos_BodyShortTrailingIdx = sp.ringPos_BodyShortTrailingIdx + 1;
  if (sp.ringPos_BodyShortTrailingIdx >= sp.ringCap_BodyShortTrailingIdx) {
    sp.ringPos_BodyShortTrailingIdx = 0;
  }
}

function cdlmatholdOpenImpl(
  sp: CdlmatholdStream,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInPenetration: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  const BodyPeriodTotal = new Float64Array(5);
  let BodyPeriodTotalRange1 = 0;
  let BodyPeriodTotalTrailRange1 = 0;
  let BodyPeriodTotalRange2 = 0;
  let BodyPeriodTotalTrailRange2 = 0;
  let BodyPeriodTotalRange3 = 0;
  let BodyPeriodTotalTrailRange3 = 0;
  let i = 0;
  let outIdx = 0;
  let BodyShortTrailingIdx = 0;
  let BodyLongTrailingIdx = 0;
  let lookbackTotal = 0;
  const historyLen = inOpen.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inHigh.length !== inOpen.length || inLow.length !== inOpen.length || inClose.length !== inOpen.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInPenetration === Core.REAL_DEFAULT) {
    optInPenetration = 0.5;
  } else if (!(optInPenetration >= 0e0 && optInPenetration <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const BodyLong_rangeType: number = core.candleSettings[CandleSettingType.BODY_LONG].rangeType;
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  const BodyLong_factor = core.candleSettings[CandleSettingType.BODY_LONG].factor;
  const BodyShort_rangeType: number = core.candleSettings[CandleSettingType.BODY_SHORT].rangeType;
  const BodyShort_avgPeriod = core.candleSettings[CandleSettingType.BODY_SHORT].avgPeriod;
  const BodyShort_factor = core.candleSettings[CandleSettingType.BODY_SHORT].factor;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlmatholdLookbackPositional(optInPenetration, core);
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
  BodyPeriodTotal[4] = 0;
  BodyPeriodTotal[3] = 0;
  BodyPeriodTotal[2] = 0;
  BodyPeriodTotal[1] = 0;
  BodyPeriodTotal[0] = 0;
  BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
  BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
  i = BodyShortTrailingIdx;
  while (i < startIdx) {
    BodyPeriodTotal[3] = BodyPeriodTotal[3] + ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)));
    BodyPeriodTotal[2] = BodyPeriodTotal[2] + ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
    BodyPeriodTotal[1] = BodyPeriodTotal[1] + ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    i += 1;
  }
  i = BodyLongTrailingIdx;
  while (i < startIdx) {
    BodyPeriodTotal[4] = BodyPeriodTotal[4] + ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first candle: long white candle
   * - upside gap between the first and the second bodies
   * - second candle: small black candle
   * - third and fourth candles: falling small real body candlesticks (commonly black) that hold within the long
   *   white candle's body and are higher than the reaction days of the rising three methods
   * - fifth candle: white candle that opens above the previous small candle's close and closes higher than the
   *   high of the highest reaction day
   * The meaning of "short" and "long" is specified with TA_SetCandleSettings;
   * "hold within" means "a part of the real body must be within";
   * optInPenetration is the maximum percentage of the first white body the reaction days can penetrate (it is
   * to specify how much the reaction days should be "higher than the reaction days of the rising three methods")
   */
  outIdx = 0;
  BodyPeriodTotalRange2 = ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
  BodyPeriodTotalTrailRange2 = ((BodyShort_rangeType === 0) ? (Math.abs(inClose[BodyShortTrailingIdx - 2] - inOpen[BodyShortTrailingIdx - 2])) : ((BodyShort_rangeType === 1) ? (inHigh[BodyShortTrailingIdx - 2] - inLow[BodyShortTrailingIdx - 2]) : ((BodyShort_rangeType === 2) ? ((inHigh[BodyShortTrailingIdx - 2] - (((inClose[BodyShortTrailingIdx - 2]) >= (inOpen[BodyShortTrailingIdx - 2])) ? (inClose[BodyShortTrailingIdx - 2]) : (inOpen[BodyShortTrailingIdx - 2]))) + ((((inClose[BodyShortTrailingIdx - 2]) >= (inOpen[BodyShortTrailingIdx - 2])) ? (inOpen[BodyShortTrailingIdx - 2]) : (inClose[BodyShortTrailingIdx - 2])) - inLow[BodyShortTrailingIdx - 2])) : 0.0)));
  BodyPeriodTotalRange3 = ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)));
  BodyPeriodTotalTrailRange3 = ((BodyShort_rangeType === 0) ? (Math.abs(inClose[BodyShortTrailingIdx - 3] - inOpen[BodyShortTrailingIdx - 3])) : ((BodyShort_rangeType === 1) ? (inHigh[BodyShortTrailingIdx - 3] - inLow[BodyShortTrailingIdx - 3]) : ((BodyShort_rangeType === 2) ? ((inHigh[BodyShortTrailingIdx - 3] - (((inClose[BodyShortTrailingIdx - 3]) >= (inOpen[BodyShortTrailingIdx - 3])) ? (inClose[BodyShortTrailingIdx - 3]) : (inOpen[BodyShortTrailingIdx - 3]))) + ((((inClose[BodyShortTrailingIdx - 3]) >= (inOpen[BodyShortTrailingIdx - 3])) ? (inOpen[BodyShortTrailingIdx - 3]) : (inClose[BodyShortTrailingIdx - 3])) - inLow[BodyShortTrailingIdx - 3])) : 0.0)));
  do {
    if (((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) === 1 &&  /* white, black, 2 black or white, white */
        ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : -1) === -1 &&
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === 1 &&
        ((inOpen[i - 3] < inClose[i - 3] ? inOpen[i - 3] : inClose[i - 3]) > (inOpen[i - 4] > inClose[i - 4] ? inOpen[i - 4] : inClose[i - 4])) && /* upside gap 1st to 2nd */
        (inOpen[i - 2] < inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2]) < inClose[i - 4] && /* 3rd to 4th hold within 1st: a part of the real body must be within 1st real body */
        (inOpen[i - 1] < inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1]) < inClose[i - 4] &&
        (inOpen[i - 2] < inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2]) > inClose[i - 4] - Math.abs(inClose[i - 4] - inOpen[i - 4]) * optInPenetration && /* reaction days penetrate first body less than optInPenetration percent */
        (inOpen[i - 1] < inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1]) > inClose[i - 4] - Math.abs(inClose[i - 4] - inOpen[i - 4]) * optInPenetration &&
        (inClose[i - 2] > inOpen[i - 2] ? inClose[i - 2] : inOpen[i - 2]) < inOpen[i - 3] && /* 2nd to 4th are falling */
        (inClose[i - 1] > inOpen[i - 1] ? inClose[i - 1] : inOpen[i - 1]) < (inClose[i - 2] > inOpen[i - 2] ? inClose[i - 2] : inOpen[i - 2]) &&
        inOpen[i] > inClose[i - 1] &&                          /* 5th opens above the prior close */
        inClose[i] > ((inHigh[i - 3] > inHigh[i - 2] ? inHigh[i - 3] : inHigh[i - 2]) > inHigh[i - 1] ? (inHigh[i - 3] > inHigh[i - 2] ? inHigh[i - 3] : inHigh[i - 2]) : inHigh[i - 1]) && /* 5th closes above the highest high of the reaction days */
        Math.abs(inClose[i - 4] - inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyPeriodTotal[4] / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st long, then 3 small */
        Math.abs(inClose[i - 3] - inOpen[i - 3]) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (BodyPeriodTotal[3] / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) &&
        Math.abs(inClose[i - 2] - inOpen[i - 2]) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (BodyPeriodTotal[2] / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) &&
        Math.abs(inClose[i - 1] - inOpen[i - 1]) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (BodyPeriodTotal[1] / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))))
    {
      outInteger[((outIdx++ * outStride) | 0)] = 100;
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyPeriodTotal[4] = BodyPeriodTotal[4] + (((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0))) - ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 4] - inOpen[BodyLongTrailingIdx - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyLongTrailingIdx - 4] - inLow[BodyLongTrailingIdx - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyLongTrailingIdx - 4] - (((inClose[BodyLongTrailingIdx - 4]) >= (inOpen[BodyLongTrailingIdx - 4])) ? (inClose[BodyLongTrailingIdx - 4]) : (inOpen[BodyLongTrailingIdx - 4]))) + ((((inClose[BodyLongTrailingIdx - 4]) >= (inOpen[BodyLongTrailingIdx - 4])) ? (inOpen[BodyLongTrailingIdx - 4]) : (inClose[BodyLongTrailingIdx - 4])) - inLow[BodyLongTrailingIdx - 4])) : 0.0))));
    BodyPeriodTotalRange1 = ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    BodyPeriodTotalTrailRange1 = ((BodyShort_rangeType === 0) ? (Math.abs(inClose[BodyShortTrailingIdx - 1] - inOpen[BodyShortTrailingIdx - 1])) : ((BodyShort_rangeType === 1) ? (inHigh[BodyShortTrailingIdx - 1] - inLow[BodyShortTrailingIdx - 1]) : ((BodyShort_rangeType === 2) ? ((inHigh[BodyShortTrailingIdx - 1] - (((inClose[BodyShortTrailingIdx - 1]) >= (inOpen[BodyShortTrailingIdx - 1])) ? (inClose[BodyShortTrailingIdx - 1]) : (inOpen[BodyShortTrailingIdx - 1]))) + ((((inClose[BodyShortTrailingIdx - 1]) >= (inOpen[BodyShortTrailingIdx - 1])) ? (inOpen[BodyShortTrailingIdx - 1]) : (inClose[BodyShortTrailingIdx - 1])) - inLow[BodyShortTrailingIdx - 1])) : 0.0)));
    BodyPeriodTotal[3] = BodyPeriodTotal[3] + (BodyPeriodTotalRange3 - BodyPeriodTotalTrailRange3);
    BodyPeriodTotal[2] = BodyPeriodTotal[2] + (BodyPeriodTotalRange2 - BodyPeriodTotalTrailRange2);
    BodyPeriodTotal[1] = BodyPeriodTotal[1] + (BodyPeriodTotalRange1 - BodyPeriodTotalTrailRange1);
    BodyPeriodTotalRange3 = BodyPeriodTotalRange2;
    BodyPeriodTotalTrailRange3 = BodyPeriodTotalTrailRange2;
    BodyPeriodTotalRange2 = BodyPeriodTotalRange1;
    BodyPeriodTotalTrailRange2 = BodyPeriodTotalTrailRange1;
    i += 1;
    BodyShortTrailingIdx += 1;
    BodyLongTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const capLag_BodyLongTrailingIdx = i - BodyLongTrailingIdx;
  const cap_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx + 5;
  if (capLag_BodyLongTrailingIdx < 0 || cap_BodyLongTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_BodyLongTrailingIdx = cap_BodyLongTrailingIdx > 0 ? cap_BodyLongTrailingIdx : 1;
  const capRing_BodyLongTrailingIdx_derived = new Float64Array(allocN_BodyLongTrailingIdx);
  for (let fillJ = historyLen - cap_BodyLongTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_BodyLongTrailingIdx_derived[fillJ % cap_BodyLongTrailingIdx] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyLong_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyLong_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  const capLag_BodyShortTrailingIdx = i - BodyShortTrailingIdx;
  const cap_BodyShortTrailingIdx = capLag_BodyShortTrailingIdx + 2;
  if (capLag_BodyShortTrailingIdx < 0 || cap_BodyShortTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_BodyShortTrailingIdx = cap_BodyShortTrailingIdx > 0 ? cap_BodyShortTrailingIdx : 1;
  const capRing_BodyShortTrailingIdx_derived = new Float64Array(allocN_BodyShortTrailingIdx);
  for (let fillJ = historyLen - cap_BodyShortTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_BodyShortTrailingIdx_derived[fillJ % cap_BodyShortTrailingIdx] = ((BodyShort_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyShort_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyShort_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  sp.optInPenetration = optInPenetration;
  sp.BodyPeriodTotal = BodyPeriodTotal;
  sp.BodyPeriodTotalRange2 = BodyPeriodTotalRange2;
  sp.BodyPeriodTotalTrailRange2 = BodyPeriodTotalTrailRange2;
  sp.BodyPeriodTotalRange3 = BodyPeriodTotalRange3;
  sp.BodyPeriodTotalTrailRange3 = BodyPeriodTotalTrailRange3;
  sp.lag1_inOpen = inOpen[historyLen - 1];
  sp.lag2_inOpen = inOpen[historyLen - 2];
  sp.lag3_inOpen = inOpen[historyLen - 3];
  sp.lag4_inOpen = inOpen[historyLen - 4];
  sp.lag1_inHigh = inHigh[historyLen - 1];
  sp.lag2_inHigh = inHigh[historyLen - 2];
  sp.lag3_inHigh = inHigh[historyLen - 3];
  sp.lag4_inHigh = inHigh[historyLen - 4];
  sp.lag1_inLow = inLow[historyLen - 1];
  sp.lag2_inLow = inLow[historyLen - 2];
  sp.lag3_inLow = inLow[historyLen - 3];
  sp.lag4_inLow = inLow[historyLen - 4];
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.lag2_inClose = inClose[historyLen - 2];
  sp.lag3_inClose = inClose[historyLen - 3];
  sp.lag4_inClose = inClose[historyLen - 4];
  sp.ringPos_BodyLongTrailingIdx = historyLen % cap_BodyLongTrailingIdx;
  sp.ringCap_BodyLongTrailingIdx = cap_BodyLongTrailingIdx;
  sp.ringLag_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx;
  sp.ring_BodyLongTrailingIdx_derived = capRing_BodyLongTrailingIdx_derived;
  sp.ringPos_BodyShortTrailingIdx = historyLen % cap_BodyShortTrailingIdx;
  sp.ringCap_BodyShortTrailingIdx = cap_BodyShortTrailingIdx;
  sp.ringLag_BodyShortTrailingIdx = capLag_BodyShortTrailingIdx;
  sp.ring_BodyShortTrailingIdx_derived = capRing_BodyShortTrailingIdx_derived;
  sp.cs_BodyLong_rangeType = BodyLong_rangeType;
  sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
  sp.cs_BodyLong_factor = BodyLong_factor;
  sp.cs_BodyShort_rangeType = BodyShort_rangeType;
  sp.cs_BodyShort_avgPeriod = BodyShort_avgPeriod;
  sp.cs_BodyShort_factor = BodyShort_factor;
  sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlmatholdOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInPenetration: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): CdlmatholdStream {
  const sp = CdlmatholdStream.of(core);
  const retCode = cdlmatholdOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, optInPenetration, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLMATHOLD openAndFill', inOpen.length, startIdx, cdlmatholdLookbackPositional(optInPenetration, core));
  }
  if (retCode === RetCode.BAD_PARAM && cdlmatholdLookbackPositional(optInPenetration, core) < 0) {
    throw rejectedParams('CDLMATHOLD openAndFill', PARAM_RANGES, optInPenetration);
  }
  throw streamFailure('CDLMATHOLD openAndFill', retCode);
}

/** @internal */
export function cdlmatholdOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInPenetration: number,
): CdlmatholdStream {
  const sp = CdlmatholdStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdlmatholdOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, optInPenetration, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLMATHOLD open', inOpen.length, startIdx, cdlmatholdLookbackPositional(optInPenetration, core));
  }
  if (retCode === RetCode.BAD_PARAM && cdlmatholdLookbackPositional(optInPenetration, core) < 0) {
    throw rejectedParams('CDLMATHOLD open', PARAM_RANGES, optInPenetration);
  }
  throw streamFailure('CDLMATHOLD open', retCode);
}

/** @internal */
export function cdlmatholdOpenPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInPenetration: number,
  core: Core = Core.DEFAULT,
): CdlmatholdStream {
  optInPenetration = requireReal('CDLMATHOLD open', 'penetration', optInPenetration);
  requireArgument('CDLMATHOLD open', 'prices.open', inOpen);
  requireHistory('CDLMATHOLD open', 'prices.open', inOpen.length);
  requireArgument('CDLMATHOLD open', 'prices.high', inHigh);
  requireArgument('CDLMATHOLD open', 'prices.low', inLow);
  requireArgument('CDLMATHOLD open', 'prices.close', inClose);
  requireHistoryLength('CDLMATHOLD open', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLMATHOLD open', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLMATHOLD open', 'prices.close', inClose.length, inOpen.length);
  return cdlmatholdOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0, optInPenetration);
}

/** @internal */
export function cdlmatholdOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInPenetration: number,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): CdlmatholdStream {
  optInPenetration = requireReal('CDLMATHOLD openAndFill', 'penetration', optInPenetration);
  requireArgument('CDLMATHOLD openAndFill', 'prices.open', inOpen);
  requireHistory('CDLMATHOLD openAndFill', 'prices.open', inOpen.length);
  requireArgument('CDLMATHOLD openAndFill', 'prices.high', inHigh);
  requireArgument('CDLMATHOLD openAndFill', 'prices.low', inLow);
  requireArgument('CDLMATHOLD openAndFill', 'prices.close', inClose);
  const guardLookback = cdlmatholdLookbackPositional(optInPenetration, core);
  if (guardLookback < 0) {
    throw rejectedParams('CDLMATHOLD openAndFill', PARAM_RANGES, optInPenetration);
  }
  const guardOutLen = openFillCount('CDLMATHOLD openAndFill', inOpen.length, guardLookback);
  requireHistoryLength('CDLMATHOLD openAndFill', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLMATHOLD openAndFill', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLMATHOLD openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('CDLMATHOLD openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('CDLMATHOLD openAndFill', 'outInteger', 'prices.open');
  }
  if (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CDLMATHOLD openAndFill', 'outInteger', 'prices.high');
  }
  if (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CDLMATHOLD openAndFill', 'outInteger', 'prices.low');
  }
  if (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('CDLMATHOLD openAndFill', 'outInteger', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLMATHOLD openAndFill', 'outInteger', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLMATHOLD openAndFill', 'outInteger', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLMATHOLD openAndFill', 'outInteger', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLMATHOLD openAndFill', 'outInteger', 'prices.close');
  }
  return cdlmatholdOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, optInPenetration, new MInteger(), new MInteger(), outInteger);
}

/**
 * Opens a live CDLMATHOLD stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link cdlmathold} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlmatholdOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlmatholdLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, a real parameter that is not a number, a parameter
 *   outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdlmatholdOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdlmatholdOpen({ open, high, low, close }, { penetration: 0.5 });
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdlmatholdOpen(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlmatholdOptions): CdlmatholdStream {
  requireArgument('CDLMATHOLD open', 'prices', prices);
  const o = optionsOf('CDLMATHOLD open', options);
  const optInPenetration = orDefault(o.penetration, Core.REAL_DEFAULT);
  const core = coreOf('CDLMATHOLD open', o.core);
  return cdlmatholdOpenPositional(prices.open, prices.high, prices.low, prices.close, optInPenetration, core);
}

/**
 * {@link cdlmatholdOpen} that also computes the outputs over the whole history in
 * the same pass, bit-identically to {@link cdlmathold}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CdlmatholdOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cdlmathold} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlmatholdLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, a real parameter that is not a number, a parameter
 *   outside its range, histories of different lengths or a buffer too short, of the
 *   wrong type or starting where another buffer or a `Float64Array` input starts,
 *   or, on a history long enough to open, one sharing memory with another buffer or
 *   a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdlmatholdOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = cdlmatholdOpenAndFill({ open, high, low, close }, {
 *   penetration: 0.5,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cdlmatholdOpenAndFill(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlmatholdOptions): CdlmatholdResult & { stream: CdlmatholdStream } {
  requireArgument('CDLMATHOLD openAndFill', 'prices', prices);
  const o = optionsOf('CDLMATHOLD openAndFill', options);
  const optInPenetration = orDefault(o.penetration, Core.REAL_DEFAULT);
  const core = coreOf('CDLMATHOLD openAndFill', o.core);
  const capacity = fillCapacity(cdlmatholdLookbackPositional(optInPenetration, core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const stream = cdlmatholdOpenAndFillPositional(prices.open, prices.high, prices.low, prices.close, optInPenetration, outInteger, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}
