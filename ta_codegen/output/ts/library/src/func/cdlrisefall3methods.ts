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
 *  020605 AC     Creation
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 *  103126 MF,CC  Nest the cheap, selective pre-check conjuncts in an outer if
 *  110526 MF,CC  Carry the ranges of the previous bars instead of recomputing them
 *  110626 MF,CC  Opt out of the range-type copy, which slows this body
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { CandleSettingType, Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function cdlrisefall3methodsLookbackPositional(core: Core = Core.DEFAULT): number {
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  const BodyShort_avgPeriod = core.candleSettings[CandleSettingType.BODY_SHORT].avgPeriod;
  return (BodyShort_avgPeriod > BodyLong_avgPeriod ? BodyShort_avgPeriod : BodyLong_avgPeriod) + 4;
}

/** @internal */
export function cdlrisefall3methodsDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdlrisefall3methodsImpl(
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
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlrisefall3methodsLookbackPositional(core);
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
    BodyPeriodTotal[0] = BodyPeriodTotal[0] + ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first candle: long white (black) candlestick
   * - then: group of falling (rising) small real body candlesticks (commonly black (white)) that hold within
   *   the prior long candle's range: ideally they should be three but two or more than three are ok too
   * - final candle: long white (black) candle that opens above (below) the previous small candle's close
   *   and closes above (below) the first long candle's close
   * The meaning of "short" and "long" is specified with TA_SetCandleSettings; here only patterns with 3 small candles
   * are considered
   */
  outIdx = 0;
  BodyPeriodTotalRange2 = ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
  BodyPeriodTotalTrailRange2 = ((BodyShort_rangeType === 0) ? (Math.abs(inClose[BodyShortTrailingIdx - 2] - inOpen[BodyShortTrailingIdx - 2])) : ((BodyShort_rangeType === 1) ? (inHigh[BodyShortTrailingIdx - 2] - inLow[BodyShortTrailingIdx - 2]) : ((BodyShort_rangeType === 2) ? ((inHigh[BodyShortTrailingIdx - 2] - (((inClose[BodyShortTrailingIdx - 2]) >= (inOpen[BodyShortTrailingIdx - 2])) ? (inClose[BodyShortTrailingIdx - 2]) : (inOpen[BodyShortTrailingIdx - 2]))) + ((((inClose[BodyShortTrailingIdx - 2]) >= (inOpen[BodyShortTrailingIdx - 2])) ? (inOpen[BodyShortTrailingIdx - 2]) : (inClose[BodyShortTrailingIdx - 2])) - inLow[BodyShortTrailingIdx - 2])) : 0.0)));
  BodyPeriodTotalRange3 = ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)));
  BodyPeriodTotalTrailRange3 = ((BodyShort_rangeType === 0) ? (Math.abs(inClose[BodyShortTrailingIdx - 3] - inOpen[BodyShortTrailingIdx - 3])) : ((BodyShort_rangeType === 1) ? (inHigh[BodyShortTrailingIdx - 3] - inLow[BodyShortTrailingIdx - 3]) : ((BodyShort_rangeType === 2) ? ((inHigh[BodyShortTrailingIdx - 3] - (((inClose[BodyShortTrailingIdx - 3]) >= (inOpen[BodyShortTrailingIdx - 3])) ? (inClose[BodyShortTrailingIdx - 3]) : (inOpen[BodyShortTrailingIdx - 3]))) + ((((inClose[BodyShortTrailingIdx - 3]) >= (inOpen[BodyShortTrailingIdx - 3])) ? (inOpen[BodyShortTrailingIdx - 3]) : (inClose[BodyShortTrailingIdx - 3])) - inLow[BodyShortTrailingIdx - 3])) : 0.0)));
  do {
    if (((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) === (-((inClose[i - 3] >= inOpen[i - 3]) ? 1 : -1) | 0) && /* white, 3 black, white  ||  black, 3 white, black */
        ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : -1) === ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) &&
        ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) &&
        ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === (-((inClose[i] >= inOpen[i]) ? 1 : -1) | 0) &&
        (inOpen[i - 3] < inClose[i - 3] ? inOpen[i - 3] : inClose[i - 3]) < inHigh[i - 4] && /* 2nd to 4th hold within 1st: a part of the real body must be within 1st range */
        (inOpen[i - 3] > inClose[i - 3] ? inOpen[i - 3] : inClose[i - 3]) > inLow[i - 4] &&
        (inOpen[i - 2] < inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2]) < inHigh[i - 4] &&
        (inOpen[i - 2] > inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2]) > inLow[i - 4] &&
        (inOpen[i - 1] < inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1]) < inHigh[i - 4] &&
        (inOpen[i - 1] > inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1]) > inLow[i - 4] &&
        inClose[i - 2] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) < inClose[i - 3] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) && /* 2nd to 4th are falling (rising) */
        inClose[i - 1] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) < inClose[i - 2] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) &&
        inOpen[i] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) > inClose[i - 1] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) && /* 5th opens above (below) the prior close */
        inClose[i] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) > inClose[i - 4] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) && /* 5th closes above (below) the 1st close */
        Math.abs(inClose[i - 4] - inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyPeriodTotal[4] / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st long, then 3 small, 5th long */
        Math.abs(inClose[i - 3] - inOpen[i - 3]) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (BodyPeriodTotal[3] / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) &&
        Math.abs(inClose[i - 2] - inOpen[i - 2]) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (BodyPeriodTotal[2] / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) &&
        Math.abs(inClose[i - 1] - inOpen[i - 1]) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (BodyPeriodTotal[1] / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) &&
        Math.abs(inClose[i] - inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyPeriodTotal[0] / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))))
    {
      outInteger[outIdx++] = ((100 * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1)) | 0);
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
    BodyPeriodTotal[0] = BodyPeriodTotal[0] + (((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyLongTrailingIdx] - (((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inClose[BodyLongTrailingIdx]) : (inOpen[BodyLongTrailingIdx]))) + ((((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inOpen[BodyLongTrailingIdx]) : (inClose[BodyLongTrailingIdx])) - inLow[BodyLongTrailingIdx])) : 0.0))));
    i += 1;
    BodyShortTrailingIdx += 1;
    BodyLongTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlrisefall3methodsPositional(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDLRISEFALL3METHODS', 'startIdx', startIdx);
  endIdx = requireInt32('CDLRISEFALL3METHODS', 'endIdx', endIdx);
  requireIndexRange('CDLRISEFALL3METHODS', startIdx, endIdx);
  const guardStart = clampedStart('CDLRISEFALL3METHODS', startIdx, cdlrisefall3methodsLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLRISEFALL3METHODS', 'prices.open', inOpen, guardInLen);
  requireLength('CDLRISEFALL3METHODS', 'prices.high', inHigh, guardInLen);
  requireLength('CDLRISEFALL3METHODS', 'prices.low', inLow, guardInLen);
  requireLength('CDLRISEFALL3METHODS', 'prices.close', inClose, guardInLen);
  requireOutput('CDLRISEFALL3METHODS', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLRISEFALL3METHODS', 'outInteger', 'prices.open');
  }
  if (overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLRISEFALL3METHODS', 'outInteger', 'prices.high');
  }
  if (overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLRISEFALL3METHODS', 'outInteger', 'prices.low');
  }
  if (overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLRISEFALL3METHODS', 'outInteger', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdlrisefall3methodsImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLRISEFALL3METHODS', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cdlrisefall3methods}, {@link cdlrisefall3methodsLookback},
 * {@link cdlrisefall3methodsDisplayShift}, {@link cdlrisefall3methodsOpen} and
 * {@link cdlrisefall3methodsOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface Cdlrisefall3methodsOptions {
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
 * What {@link cdlrisefall3methods} returns, and
 * {@link cdlrisefall3methodsOpenAndFill} with its stream.
 */
export interface Cdlrisefall3methodsResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * +100 when candle 1 is white (rising/bullish continuation), -100 when candle 1 is
   * black (falling/bearish continuation), 0 otherwise. Sign = 100 * color of candle 1.
   * `values[k]` is the value at input bar `begIdx + k`.
   */
  values: Int32Array;
}

/**
 * Number of leading input bars {@link cdlrisefall3methods} consumes before its
 * first value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link Cdlrisefall3methodsOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlrisefall3methodsLookback(options?: Cdlrisefall3methodsOptions): number {
  const o = optionsOf('CDLRISEFALL3METHODS lookback', options);
  return cdlrisefall3methodsLookbackPositional(coreOf('CDLRISEFALL3METHODS lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdlrisefall3methods}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link Cdlrisefall3methodsOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlrisefall3methodsDisplayShift(outputIdx: number, options?: Cdlrisefall3methodsOptions): number {
  const o = optionsOf('CDLRISEFALL3METHODS displayShift', options);
  return cdlrisefall3methodsDisplayShiftPositional(outputIdx, coreOf('CDLRISEFALL3METHODS displayShift', o.core));
}

/**
 * A five-candle continuation pattern: a long candle, three small counter-color
 * candles that stay partly within the first candle's high-low range, then a long
 * same-color candle that resumes the trend. Bullish (rising) or bearish (falling)
 * continuation signal.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlrisefall3methods](https://ta-lib.org/functions/cdlrisefall3methods).
 *
 * **Notes**
 * - Only the three-small-candle variant is detected; the classic pattern allowing
 *   two or more small candles is not supported.
 * - The middle candles need only partially overlap the first candle's range, not
 *   be fully contained within it.
 * - The prior trend the continuation reading assumes is not verified.
 * - Bulkowski's testing found Rising Three Methods continues 74% of the time (102
 *   examples out of 4.7M candle lines) and Falling Three Methods continues 71% of
 *   the time (just 64 examples) — both act as classically labeled, but Bulkowski
 *   flags the samples as too thin to trust: Falling Three Methods is so rare he
 *   omitted its statistics from his book entirely.
 *   ([thepatternsite.com](https://thepatternsite.com/Rising3Methods.html))
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cdlrisefall3methodsLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `open`: Open
 *   price of each bar. `high`: High price of each bar. `low`: Low price of each bar.
 *   `close`: Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see
 *   {@link Cdlrisefall3methodsOptions}.
 * @returns `begIdx` and the values; see {@link Cdlrisefall3methodsResult}. Each
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
 * import { cdlrisefall3methods } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { begIdx, values } = cdlrisefall3methods({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`cdlxsidegap3methods`](https://ta-lib.org/functions/cdlxsidegap3methods)
 * @see [`cdl3inside`](https://ta-lib.org/functions/cdl3inside)
 * @see [`cdl3outside`](https://ta-lib.org/functions/cdl3outside)
 */
export function cdlrisefall3methods(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: Cdlrisefall3methodsOptions): Cdlrisefall3methodsResult {
  requireArgument('CDLRISEFALL3METHODS', 'prices', prices);
  const o = optionsOf('CDLRISEFALL3METHODS', options);
  if (o.endIdx === undefined) {
    requireSeries('CDLRISEFALL3METHODS', 'prices.open', prices.open);
    requireSeries('CDLRISEFALL3METHODS', 'prices.high', prices.high);
    requireSeries('CDLRISEFALL3METHODS', 'prices.low', prices.low);
    requireSeries('CDLRISEFALL3METHODS', 'prices.close', prices.close);
  }
  const core = coreOf('CDLRISEFALL3METHODS', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cdlrisefall3methodsLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const range = cdlrisefall3methodsPositional(startIdx, endIdx, prices.open, prices.high, prices.low, prices.close, outInteger, core);
  return { begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CDLRISEFALL3METHODS stream: one value per closed bar, bit-identical to
 * {@link cdlrisefall3methods} over the same series. Open it with
 * {@link cdlrisefall3methodsOpen} or {@link cdlrisefall3methodsOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlrisefall3methods](https://ta-lib.org/functions/cdlrisefall3methods).
 */
export class Cdlrisefall3methodsStream {
  /** @internal */
  core: Core;
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
  static of(core: Core): Cdlrisefall3methodsStream {
    return new Cdlrisefall3methodsStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cdlrisefall3methods} reports them over the same bars. Each accepted
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
      throw streamFailure('CDLRISEFALL3METHODS advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw streamFailure('CDLRISEFALL3METHODS update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(open) || !Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('CDLRISEFALL3METHODS update', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    cdlrisefall3methodsStepImpl(this, open, high, low, close);
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
      throw nonFiniteBar('CDLRISEFALL3METHODS peek', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inOpen = open;
    const inHigh = high;
    const inLow = low;
    const inClose = close;
    let cur_outInteger = 0;
    const BodyLong_rangeType = sp.cs_BodyLong_rangeType;
    const BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
    const BodyLong_factor = sp.cs_BodyLong_factor;
    const BodyShort_rangeType = sp.cs_BodyShort_rangeType;
    const BodyShort_avgPeriod = sp.cs_BodyShort_avgPeriod;
    const BodyShort_factor = sp.cs_BodyShort_factor;
    if (((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) === (-((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : -1) | 0) && /* white, 3 black, white  ||  black, 3 white, black */
        ((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : -1) === ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) &&
        ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) &&
        ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === (-((inClose >= inOpen) ? 1 : -1) | 0) &&
        (sp.lag3_inOpen < sp.lag3_inClose ? sp.lag3_inOpen : sp.lag3_inClose) < sp.lag4_inHigh && /* 2nd to 4th hold within 1st: a part of the real body must be within 1st range */
        (sp.lag3_inOpen > sp.lag3_inClose ? sp.lag3_inOpen : sp.lag3_inClose) > sp.lag4_inLow &&
        (sp.lag2_inOpen < sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose) < sp.lag4_inHigh &&
        (sp.lag2_inOpen > sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose) > sp.lag4_inLow &&
        (sp.lag1_inOpen < sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose) < sp.lag4_inHigh &&
        (sp.lag1_inOpen > sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose) > sp.lag4_inLow &&
        sp.lag2_inClose * ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) < sp.lag3_inClose * ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) && /* 2nd to 4th are falling (rising) */
        sp.lag1_inClose * ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) < sp.lag2_inClose * ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) &&
        inOpen * ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) > sp.lag1_inClose * ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) && /* 5th opens above (below) the prior close */
        inClose * ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) > sp.lag4_inClose * ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) && /* 5th closes above (below) the 1st close */
        Math.abs(sp.lag4_inClose - sp.lag4_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyPeriodTotal[4] / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag4_inClose - sp.lag4_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag4_inHigh - sp.lag4_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag4_inHigh - (((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inClose) : (sp.lag4_inOpen))) + ((((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inOpen) : (sp.lag4_inClose)) - sp.lag4_inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st long, then 3 small, 5th long */
        Math.abs(sp.lag3_inClose - sp.lag3_inOpen) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (sp.BodyPeriodTotal[3] / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(sp.lag3_inClose - sp.lag3_inOpen)) : ((BodyShort_rangeType === 1) ? (sp.lag3_inHigh - sp.lag3_inLow) : ((BodyShort_rangeType === 2) ? ((sp.lag3_inHigh - (((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inClose) : (sp.lag3_inOpen))) + ((((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inOpen) : (sp.lag3_inClose)) - sp.lag3_inLow)) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) &&
        Math.abs(sp.lag2_inClose - sp.lag2_inOpen) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (sp.BodyPeriodTotal[2] / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyShort_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyShort_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) &&
        Math.abs(sp.lag1_inClose - sp.lag1_inOpen) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (sp.BodyPeriodTotal[1] / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyShort_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyShort_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) &&
        Math.abs(inClose - inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyPeriodTotal[0] / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType === 1) ? (inHigh - inLow) : ((BodyLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))))
    {
      cur_outInteger = ((100 * ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1)) | 0);
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
  clone(): Cdlrisefall3methodsStream {
    const c = new Cdlrisefall3methodsStream(this.core);
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

function cdlrisefall3methodsStepImpl(sp: Cdlrisefall3methodsStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
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
  if (((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) === (-((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : -1) | 0) && /* white, 3 black, white  ||  black, 3 white, black */
      ((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : -1) === ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) &&
      ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) &&
      ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === (-((inClose >= inOpen) ? 1 : -1) | 0) &&
      (sp.lag3_inOpen < sp.lag3_inClose ? sp.lag3_inOpen : sp.lag3_inClose) < sp.lag4_inHigh && /* 2nd to 4th hold within 1st: a part of the real body must be within 1st range */
      (sp.lag3_inOpen > sp.lag3_inClose ? sp.lag3_inOpen : sp.lag3_inClose) > sp.lag4_inLow &&
      (sp.lag2_inOpen < sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose) < sp.lag4_inHigh &&
      (sp.lag2_inOpen > sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose) > sp.lag4_inLow &&
      (sp.lag1_inOpen < sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose) < sp.lag4_inHigh &&
      (sp.lag1_inOpen > sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose) > sp.lag4_inLow &&
      sp.lag2_inClose * ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) < sp.lag3_inClose * ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) && /* 2nd to 4th are falling (rising) */
      sp.lag1_inClose * ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) < sp.lag2_inClose * ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) &&
      inOpen * ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) > sp.lag1_inClose * ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) && /* 5th opens above (below) the prior close */
      inClose * ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) > sp.lag4_inClose * ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) && /* 5th closes above (below) the 1st close */
      Math.abs(sp.lag4_inClose - sp.lag4_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyPeriodTotal[4] / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag4_inClose - sp.lag4_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag4_inHigh - sp.lag4_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag4_inHigh - (((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inClose) : (sp.lag4_inOpen))) + ((((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inOpen) : (sp.lag4_inClose)) - sp.lag4_inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st long, then 3 small, 5th long */
      Math.abs(sp.lag3_inClose - sp.lag3_inOpen) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (sp.BodyPeriodTotal[3] / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(sp.lag3_inClose - sp.lag3_inOpen)) : ((BodyShort_rangeType === 1) ? (sp.lag3_inHigh - sp.lag3_inLow) : ((BodyShort_rangeType === 2) ? ((sp.lag3_inHigh - (((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inClose) : (sp.lag3_inOpen))) + ((((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inOpen) : (sp.lag3_inClose)) - sp.lag3_inLow)) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) &&
      Math.abs(sp.lag2_inClose - sp.lag2_inOpen) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (sp.BodyPeriodTotal[2] / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyShort_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyShort_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) &&
      Math.abs(sp.lag1_inClose - sp.lag1_inOpen) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (sp.BodyPeriodTotal[1] / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyShort_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyShort_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) &&
      Math.abs(inClose - inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyPeriodTotal[0] / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType === 1) ? (inHigh - inLow) : ((BodyLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))))
  {
    sp.cur_outInteger = ((100 * ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1)) | 0);
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
  sp.BodyPeriodTotal[0] = sp.BodyPeriodTotal[0] + (((BodyLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType === 1) ? (inHigh - inLow) : ((BodyLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0))) - sp.ring_BodyLongTrailingIdx_derived[(((sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx) % sp.ringCap_BodyLongTrailingIdx) | 0)]);
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

function cdlrisefall3methodsOpenImpl(
  sp: Cdlrisefall3methodsStream,
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
  lookbackTotal = cdlrisefall3methodsLookbackPositional(core);
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
    BodyPeriodTotal[0] = BodyPeriodTotal[0] + ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first candle: long white (black) candlestick
   * - then: group of falling (rising) small real body candlesticks (commonly black (white)) that hold within
   *   the prior long candle's range: ideally they should be three but two or more than three are ok too
   * - final candle: long white (black) candle that opens above (below) the previous small candle's close
   *   and closes above (below) the first long candle's close
   * The meaning of "short" and "long" is specified with TA_SetCandleSettings; here only patterns with 3 small candles
   * are considered
   */
  outIdx = 0;
  BodyPeriodTotalRange2 = ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
  BodyPeriodTotalTrailRange2 = ((BodyShort_rangeType === 0) ? (Math.abs(inClose[BodyShortTrailingIdx - 2] - inOpen[BodyShortTrailingIdx - 2])) : ((BodyShort_rangeType === 1) ? (inHigh[BodyShortTrailingIdx - 2] - inLow[BodyShortTrailingIdx - 2]) : ((BodyShort_rangeType === 2) ? ((inHigh[BodyShortTrailingIdx - 2] - (((inClose[BodyShortTrailingIdx - 2]) >= (inOpen[BodyShortTrailingIdx - 2])) ? (inClose[BodyShortTrailingIdx - 2]) : (inOpen[BodyShortTrailingIdx - 2]))) + ((((inClose[BodyShortTrailingIdx - 2]) >= (inOpen[BodyShortTrailingIdx - 2])) ? (inOpen[BodyShortTrailingIdx - 2]) : (inClose[BodyShortTrailingIdx - 2])) - inLow[BodyShortTrailingIdx - 2])) : 0.0)));
  BodyPeriodTotalRange3 = ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)));
  BodyPeriodTotalTrailRange3 = ((BodyShort_rangeType === 0) ? (Math.abs(inClose[BodyShortTrailingIdx - 3] - inOpen[BodyShortTrailingIdx - 3])) : ((BodyShort_rangeType === 1) ? (inHigh[BodyShortTrailingIdx - 3] - inLow[BodyShortTrailingIdx - 3]) : ((BodyShort_rangeType === 2) ? ((inHigh[BodyShortTrailingIdx - 3] - (((inClose[BodyShortTrailingIdx - 3]) >= (inOpen[BodyShortTrailingIdx - 3])) ? (inClose[BodyShortTrailingIdx - 3]) : (inOpen[BodyShortTrailingIdx - 3]))) + ((((inClose[BodyShortTrailingIdx - 3]) >= (inOpen[BodyShortTrailingIdx - 3])) ? (inOpen[BodyShortTrailingIdx - 3]) : (inClose[BodyShortTrailingIdx - 3])) - inLow[BodyShortTrailingIdx - 3])) : 0.0)));
  do {
    if (((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) === (-((inClose[i - 3] >= inOpen[i - 3]) ? 1 : -1) | 0) && /* white, 3 black, white  ||  black, 3 white, black */
        ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : -1) === ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) &&
        ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) &&
        ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === (-((inClose[i] >= inOpen[i]) ? 1 : -1) | 0) &&
        (inOpen[i - 3] < inClose[i - 3] ? inOpen[i - 3] : inClose[i - 3]) < inHigh[i - 4] && /* 2nd to 4th hold within 1st: a part of the real body must be within 1st range */
        (inOpen[i - 3] > inClose[i - 3] ? inOpen[i - 3] : inClose[i - 3]) > inLow[i - 4] &&
        (inOpen[i - 2] < inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2]) < inHigh[i - 4] &&
        (inOpen[i - 2] > inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2]) > inLow[i - 4] &&
        (inOpen[i - 1] < inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1]) < inHigh[i - 4] &&
        (inOpen[i - 1] > inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1]) > inLow[i - 4] &&
        inClose[i - 2] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) < inClose[i - 3] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) && /* 2nd to 4th are falling (rising) */
        inClose[i - 1] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) < inClose[i - 2] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) &&
        inOpen[i] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) > inClose[i - 1] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) && /* 5th opens above (below) the prior close */
        inClose[i] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) > inClose[i - 4] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) && /* 5th closes above (below) the 1st close */
        Math.abs(inClose[i - 4] - inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyPeriodTotal[4] / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st long, then 3 small, 5th long */
        Math.abs(inClose[i - 3] - inOpen[i - 3]) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (BodyPeriodTotal[3] / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) &&
        Math.abs(inClose[i - 2] - inOpen[i - 2]) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (BodyPeriodTotal[2] / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) &&
        Math.abs(inClose[i - 1] - inOpen[i - 1]) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (BodyPeriodTotal[1] / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) &&
        Math.abs(inClose[i] - inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyPeriodTotal[0] / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))))
    {
      outInteger[((outIdx++ * outStride) | 0)] = ((100 * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1)) | 0);
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
    BodyPeriodTotal[0] = BodyPeriodTotal[0] + (((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyLongTrailingIdx] - (((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inClose[BodyLongTrailingIdx]) : (inOpen[BodyLongTrailingIdx]))) + ((((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inOpen[BodyLongTrailingIdx]) : (inClose[BodyLongTrailingIdx])) - inLow[BodyLongTrailingIdx])) : 0.0))));
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
export function cdlrisefall3methodsOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): Cdlrisefall3methodsStream {
  const sp = Cdlrisefall3methodsStream.of(core);
  const retCode = cdlrisefall3methodsOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLRISEFALL3METHODS openAndFill', inOpen.length, startIdx, cdlrisefall3methodsLookbackPositional(core));
  }
  throw streamFailure('CDLRISEFALL3METHODS openAndFill', retCode);
}

/** @internal */
export function cdlrisefall3methodsOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): Cdlrisefall3methodsStream {
  const sp = Cdlrisefall3methodsStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdlrisefall3methodsOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLRISEFALL3METHODS open', inOpen.length, startIdx, cdlrisefall3methodsLookbackPositional(core));
  }
  throw streamFailure('CDLRISEFALL3METHODS open', retCode);
}

/** @internal */
export function cdlrisefall3methodsOpenPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): Cdlrisefall3methodsStream {
  requireArgument('CDLRISEFALL3METHODS open', 'prices.open', inOpen);
  requireHistory('CDLRISEFALL3METHODS open', 'prices.open', inOpen.length);
  requireArgument('CDLRISEFALL3METHODS open', 'prices.high', inHigh);
  requireArgument('CDLRISEFALL3METHODS open', 'prices.low', inLow);
  requireArgument('CDLRISEFALL3METHODS open', 'prices.close', inClose);
  requireHistoryLength('CDLRISEFALL3METHODS open', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLRISEFALL3METHODS open', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLRISEFALL3METHODS open', 'prices.close', inClose.length, inOpen.length);
  return cdlrisefall3methodsOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/** @internal */
export function cdlrisefall3methodsOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): Cdlrisefall3methodsStream {
  requireArgument('CDLRISEFALL3METHODS openAndFill', 'prices.open', inOpen);
  requireHistory('CDLRISEFALL3METHODS openAndFill', 'prices.open', inOpen.length);
  requireArgument('CDLRISEFALL3METHODS openAndFill', 'prices.high', inHigh);
  requireArgument('CDLRISEFALL3METHODS openAndFill', 'prices.low', inLow);
  requireArgument('CDLRISEFALL3METHODS openAndFill', 'prices.close', inClose);
  const guardOutLen = openFillCount('CDLRISEFALL3METHODS openAndFill', inOpen.length, cdlrisefall3methodsLookbackPositional(core));
  requireHistoryLength('CDLRISEFALL3METHODS openAndFill', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLRISEFALL3METHODS openAndFill', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLRISEFALL3METHODS openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('CDLRISEFALL3METHODS openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('CDLRISEFALL3METHODS openAndFill', 'outInteger', 'prices.open');
  }
  if (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CDLRISEFALL3METHODS openAndFill', 'outInteger', 'prices.high');
  }
  if (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CDLRISEFALL3METHODS openAndFill', 'outInteger', 'prices.low');
  }
  if (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('CDLRISEFALL3METHODS openAndFill', 'outInteger', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLRISEFALL3METHODS openAndFill', 'outInteger', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLRISEFALL3METHODS openAndFill', 'outInteger', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLRISEFALL3METHODS openAndFill', 'outInteger', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLRISEFALL3METHODS openAndFill', 'outInteger', 'prices.close');
  }
  return cdlrisefall3methodsOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}

/**
 * Opens a live CDLRISEFALL3METHODS stream over the warm-up history: `value()`
 * starts at the last history bar, bit-identical to {@link cdlrisefall3methods}
 * there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `open`: Open price of each bar. `high`: High price of each bar. `low`:
 *   Low price of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link Cdlrisefall3methodsOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlrisefall3methodsLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input or histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdlrisefall3methodsOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdlrisefall3methodsOpen({ open, high, low, close });
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdlrisefall3methodsOpen(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: Cdlrisefall3methodsOptions): Cdlrisefall3methodsStream {
  requireArgument('CDLRISEFALL3METHODS open', 'prices', prices);
  const o = optionsOf('CDLRISEFALL3METHODS open', options);
  const core = coreOf('CDLRISEFALL3METHODS open', o.core);
  return cdlrisefall3methodsOpenPositional(prices.open, prices.high, prices.low, prices.close, core);
}

/**
 * {@link cdlrisefall3methodsOpen} that also computes the outputs over the whole
 * history in the same pass, bit-identically to {@link cdlrisefall3methods}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `open`: Open price of each bar. `high`: High price of each bar. `low`:
 *   Low price of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link Cdlrisefall3methodsOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cdlrisefall3methods} returns them; the range
 *   is the stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlrisefall3methodsLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, histories of different lengths or a buffer too short,
 *   of the wrong type or starting where another buffer or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdlrisefall3methodsOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = cdlrisefall3methodsOpenAndFill({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cdlrisefall3methodsOpenAndFill(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: Cdlrisefall3methodsOptions): Cdlrisefall3methodsResult & { stream: Cdlrisefall3methodsStream } {
  requireArgument('CDLRISEFALL3METHODS openAndFill', 'prices', prices);
  const o = optionsOf('CDLRISEFALL3METHODS openAndFill', options);
  const core = coreOf('CDLRISEFALL3METHODS openAndFill', o.core);
  const capacity = fillCapacity(cdlrisefall3methodsLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const stream = cdlrisefall3methodsOpenAndFillPositional(prices.open, prices.high, prices.low, prices.close, outInteger, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}
