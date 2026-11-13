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
 *  102304 AC     Creation
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 *  111126 MF,CC  Test the shadow gaps and colors first, as an else-if chain of pre-checks
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireReal, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { CandleSettingType, Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function cdlabandonedbabyLookbackPositional(optInPenetration: number, core: Core = Core.DEFAULT): number {
  if (typeof optInPenetration !== 'number') {
    return -1;
  }
  if (optInPenetration === Core.REAL_DEFAULT) {
    optInPenetration = 0.3;
  } else if (!(optInPenetration >= 0e0 && optInPenetration <= Core.REAL_MAX)) {
    return -1;
  }
  const BodyDoji_avgPeriod = core.candleSettings[CandleSettingType.BODY_DOJI].avgPeriod;
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  const BodyShort_avgPeriod = core.candleSettings[CandleSettingType.BODY_SHORT].avgPeriod;
  return ((BodyDoji_avgPeriod > BodyLong_avgPeriod ? BodyDoji_avgPeriod : BodyLong_avgPeriod) > BodyShort_avgPeriod ? (BodyDoji_avgPeriod > BodyLong_avgPeriod ? BodyDoji_avgPeriod : BodyLong_avgPeriod) : BodyShort_avgPeriod) + 2;
}

/** @internal */
export function cdlabandonedbabyDisplayShiftPositional(optInPenetration: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (cdlabandonedbabyLookbackPositional(optInPenetration, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdlabandonedbabyImpl(
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
  let BodyDojiPeriodTotal = 0;
  let BodyLongPeriodTotal = 0;
  let BodyShortPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let BodyDojiTrailingIdx = 0;
  let BodyLongTrailingIdx = 0;
  let BodyShortTrailingIdx = 0;
  let lookbackTotal = 0;
  const BodyDoji_rangeType: number = core.candleSettings[CandleSettingType.BODY_DOJI].rangeType;
  const BodyDoji_avgPeriod = core.candleSettings[CandleSettingType.BODY_DOJI].avgPeriod;
  const BodyDoji_factor = core.candleSettings[CandleSettingType.BODY_DOJI].factor;
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
    optInPenetration = 0.3;
  } else if (!(optInPenetration >= 0e0 && optInPenetration <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlabandonedbabyLookbackPositional(optInPenetration, core);
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
  BodyLongPeriodTotal = 0;
  BodyDojiPeriodTotal = 0;
  BodyShortPeriodTotal = 0;
  BodyLongTrailingIdx = startIdx - 2 - BodyLong_avgPeriod;
  BodyDojiTrailingIdx = startIdx - 1 - BodyDoji_avgPeriod;
  BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
  i = BodyLongTrailingIdx;
  while (i < startIdx - 2) {
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = BodyDojiTrailingIdx;
  while (i < startIdx - 1) {
    BodyDojiPeriodTotal += ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
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
   * - first candle: long white (black) real body
   * - second candle: doji
   * - third candle: black (white) real body that moves well within the first candle's real body
   * - upside (downside) gap between the first candle and the doji (the shadows of the two candles don't touch)
   * - downside (upside) gap between the doji and the third candle (the shadows of the two candles don't touch)
   * The meaning of "doji" and "long" is specified with TA_SetCandleSettings
   * The meaning of "moves well within" is specified with optInPenetration and "moves" should mean the real body should
   * not be short ("short" is specified with TA_SetCandleSettings) - Greg Morris wants it to be long, someone else want
   * it to be relatively long
   */
  outIdx = 0;
  do {
    if ((inLow[i - 1] > inHigh[i - 2]) &&                     /* upside gap between 1st and 2nd */
        (inHigh[i] < inLow[i - 1]) &&                         /* downside gap between 2nd and 3rd */
        ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === 1 && /* 1st white */
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === -1)          /* 3rd black */
    {
      if (Math.abs(inClose[i - 2] - inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st: long */
          Math.abs(inClose[i - 1] - inOpen[i - 1]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod !== 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyDoji_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((BodyDoji_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd: doji */
          Math.abs(inClose[i] - inOpen[i]) > ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) && /* 3rd: longer than short */
          inClose[i] < inClose[i - 2] - Math.abs(inClose[i - 2] - inOpen[i - 2]) * optInPenetration) /* 3rd closes well within 1st rb */
      {
        outInteger[outIdx++] = ((((inClose[i] >= inOpen[i]) ? 1 : -1) * 100) | 0);
      } else {
        outInteger[outIdx++] = 0;
      }
    } else if ((inHigh[i - 1] < inLow[i - 2]) &&                      /* downside gap between 1st and 2nd */
        (inLow[i] > inHigh[i - 1]) &&                          /* upside gap between 2nd and 3rd */
        ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === -1 && /* 1st black */
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === 1)            /* 3rd white */
    {
      if (Math.abs(inClose[i - 2] - inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st: long */
          Math.abs(inClose[i - 1] - inOpen[i - 1]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod !== 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyDoji_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((BodyDoji_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd: doji */
          Math.abs(inClose[i] - inOpen[i]) > ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) && /* 3rd: longer than short */
          inClose[i] > fma(Math.abs(inClose[i - 2] - inOpen[i - 2]), optInPenetration, inClose[i - 2])) /* 3rd closes well within 1st rb */
      {
        outInteger[outIdx++] = ((((inClose[i] >= inOpen[i]) ? 1 : -1) * 100) | 0);
      } else {
        outInteger[outIdx++] = 0;
      }
    } else {
      outInteger[outIdx++] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0))) - ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyLongTrailingIdx] - (((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inClose[BodyLongTrailingIdx]) : (inOpen[BodyLongTrailingIdx]))) + ((((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inOpen[BodyLongTrailingIdx]) : (inClose[BodyLongTrailingIdx])) - inLow[BodyLongTrailingIdx])) : 0.0)));
    BodyDojiPeriodTotal += ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyDoji_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : ((BodyDoji_rangeType === 1) ? (inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) : ((BodyDoji_rangeType === 2) ? ((inHigh[BodyDojiTrailingIdx] - (((inClose[BodyDojiTrailingIdx]) >= (inOpen[BodyDojiTrailingIdx])) ? (inClose[BodyDojiTrailingIdx]) : (inOpen[BodyDojiTrailingIdx]))) + ((((inClose[BodyDojiTrailingIdx]) >= (inOpen[BodyDojiTrailingIdx])) ? (inOpen[BodyDojiTrailingIdx]) : (inClose[BodyDojiTrailingIdx])) - inLow[BodyDojiTrailingIdx])) : 0.0)));
    BodyShortPeriodTotal += ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyShort_rangeType === 0) ? (Math.abs(inClose[BodyShortTrailingIdx] - inOpen[BodyShortTrailingIdx])) : ((BodyShort_rangeType === 1) ? (inHigh[BodyShortTrailingIdx] - inLow[BodyShortTrailingIdx]) : ((BodyShort_rangeType === 2) ? ((inHigh[BodyShortTrailingIdx] - (((inClose[BodyShortTrailingIdx]) >= (inOpen[BodyShortTrailingIdx])) ? (inClose[BodyShortTrailingIdx]) : (inOpen[BodyShortTrailingIdx]))) + ((((inClose[BodyShortTrailingIdx]) >= (inOpen[BodyShortTrailingIdx])) ? (inOpen[BodyShortTrailingIdx]) : (inClose[BodyShortTrailingIdx])) - inLow[BodyShortTrailingIdx])) : 0.0)));
    i += 1;
    BodyLongTrailingIdx += 1;
    BodyDojiTrailingIdx += 1;
    BodyShortTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['penetration', Core.REAL_DEFAULT, 0e0, Core.REAL_MAX, 'in 0..3e37']] as const;

/** @internal */
export function cdlabandonedbabyPositional(
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
  startIdx = requireInt32('CDLABANDONEDBABY', 'startIdx', startIdx);
  endIdx = requireInt32('CDLABANDONEDBABY', 'endIdx', endIdx);
  optInPenetration = requireReal('CDLABANDONEDBABY', 'penetration', optInPenetration);
  requireIndexRange('CDLABANDONEDBABY', startIdx, endIdx);
  const guardLookback = cdlabandonedbabyLookbackPositional(optInPenetration, core);
  if (guardLookback < 0) {
    throw rejectedParams('CDLABANDONEDBABY', PARAM_RANGES, optInPenetration);
  }
  const guardStart = clampedStart('CDLABANDONEDBABY', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLABANDONEDBABY', 'prices.open', inOpen, guardInLen);
  requireLength('CDLABANDONEDBABY', 'prices.high', inHigh, guardInLen);
  requireLength('CDLABANDONEDBABY', 'prices.low', inLow, guardInLen);
  requireLength('CDLABANDONEDBABY', 'prices.close', inClose, guardInLen);
  requireOutput('CDLABANDONEDBABY', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLABANDONEDBABY', 'outInteger', 'prices.open');
  }
  if (overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLABANDONEDBABY', 'outInteger', 'prices.high');
  }
  if (overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLABANDONEDBABY', 'outInteger', 'prices.low');
  }
  if (overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLABANDONEDBABY', 'outInteger', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdlabandonedbabyImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, optInPenetration, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLABANDONEDBABY', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cdlabandonedbaby}, {@link cdlabandonedbabyLookback},
 * {@link cdlabandonedbabyDisplayShift}, {@link cdlabandonedbabyOpen} and
 * {@link cdlabandonedbabyOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface CdlabandonedbabyOptions {
  /**
   * Fraction of the 1st candle's real body the 3rd close must penetrate (default
   * 0.3; minimum 0; `Core.REAL_DEFAULT` selects the default).
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
 * What {@link cdlabandonedbaby} returns, and {@link cdlabandonedbabyOpenAndFill}
 * with its stream.
 */
export interface CdlabandonedbabyResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * +100 at a bullish abandoned baby bottom (3rd candle white), -100 at a bearish
   * abandoned baby top (3rd candle black), 0 otherwise; sign = color of the 3rd
   * candle. `values[k]` is the value at input bar `begIdx + k`.
   */
  values: Int32Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link cdlabandonedbaby} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlabandonedbabyOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlabandonedbabyLookback(options?: CdlabandonedbabyOptions): number {
  const o = optionsOf('CDLABANDONEDBABY lookback', options);
  return cdlabandonedbabyLookbackPositional(orDefault(o.penetration, Core.REAL_DEFAULT), coreOf('CDLABANDONEDBABY lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdlabandonedbaby}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlabandonedbabyOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlabandonedbabyDisplayShift(outputIdx: number, options?: CdlabandonedbabyOptions): number {
  const o = optionsOf('CDLABANDONEDBABY displayShift', options);
  return cdlabandonedbabyDisplayShiftPositional(orDefault(o.penetration, Core.REAL_DEFAULT), outputIdx, coreOf('CDLABANDONEDBABY displayShift', o.core));
}

/**
 * A three-candle reversal pattern: a long body, then a gapped-away doji, then a
 * body of opposite color that gaps back the other way and closes deep into the
 * first body. Bullish (bottom) or bearish (top) reversal signal.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlabandonedbaby](https://ta-lib.org/functions/cdlabandonedbaby).
 *
 * **Notes**
 * - Does not verify the prior trend the pattern classically assumes for
 *   significance.
 * - Bulkowski found the Abandoned Baby unusually reliable when it does occur: 70%
 *   success as a reversal, overall performance rank 9 of 103.
 *   ([thepatternsite.com](https://thepatternsite.com/AbandonBabyBull.html))
 * - Rare: a few years of one symbol's daily bars may show none. Bulkowski ranks
 *   the bullish form 92nd and the bearish 96th of 103 patterns by how often they
 *   occur ([bullish](https://thepatternsite.com/AbandonBabyBull.html),
 *   [bearish](https://thepatternsite.com/AbandonBaby.html)).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cdlabandonedbabyLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `open`: Open price of
 *   each bar. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link CdlabandonedbabyOptions}.
 * @returns `begIdx` and the values; see {@link CdlabandonedbabyResult}. Each
 *   output is a view of its buffer, the caller's or a new one.
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
 * import { cdlabandonedbaby } from 'ta-lib';
 *
 * const open = new Float64Array(252); // your data, oldest first
 * const high = new Float64Array(252); // your data, oldest first
 * const low = new Float64Array(252); // your data, oldest first
 * const close = new Float64Array(252); // your data, oldest first
 *
 * const { begIdx, values } = cdlabandonedbaby({ open, high, low, close }, {
 *   penetration: 0.3,
 * });
 * console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
 * ```
 * @see [`cdleveningdojistar`](https://ta-lib.org/functions/cdleveningdojistar)
 * @see [`cdlmorningdojistar`](https://ta-lib.org/functions/cdlmorningdojistar)
 * @see [`cdleveningstar`](https://ta-lib.org/functions/cdleveningstar)
 * @see [`cdlmorningstar`](https://ta-lib.org/functions/cdlmorningstar)
 */
export function cdlabandonedbaby(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlabandonedbabyOptions): CdlabandonedbabyResult {
  requireArgument('CDLABANDONEDBABY', 'prices', prices);
  const o = optionsOf('CDLABANDONEDBABY', options);
  if (o.endIdx === undefined) {
    requireSeries('CDLABANDONEDBABY', 'prices.open', prices.open);
    requireSeries('CDLABANDONEDBABY', 'prices.high', prices.high);
    requireSeries('CDLABANDONEDBABY', 'prices.low', prices.low);
    requireSeries('CDLABANDONEDBABY', 'prices.close', prices.close);
  }
  const optInPenetration = orDefault(o.penetration, Core.REAL_DEFAULT);
  const core = coreOf('CDLABANDONEDBABY', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cdlabandonedbabyLookbackPositional(optInPenetration, core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const range = cdlabandonedbabyPositional(startIdx, endIdx, prices.open, prices.high, prices.low, prices.close, optInPenetration, outInteger, core);
  return { begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CDLABANDONEDBABY stream: one value per closed bar, bit-identical to
 * {@link cdlabandonedbaby} over the same series. Open it with
 * {@link cdlabandonedbabyOpen} or {@link cdlabandonedbabyOpenAndFill}; there is
 * nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlabandonedbaby](https://ta-lib.org/functions/cdlabandonedbaby).
 */
export class CdlabandonedbabyStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInPenetration: number = 0;
  /** @internal */
  BodyDojiPeriodTotal: number = 0;
  /** @internal */
  BodyLongPeriodTotal: number = 0;
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
  ringPos_BodyDojiTrailingIdx: number = 0;
  /** @internal */
  ringCap_BodyDojiTrailingIdx: number = 0;
  /** @internal */
  ring_BodyDojiTrailingIdx_derived: Float64Array = new Float64Array(1);
  /** @internal */
  ringPos_BodyLongTrailingIdx: number = 0;
  /** @internal */
  ringCap_BodyLongTrailingIdx: number = 0;
  /** @internal */
  ring_BodyLongTrailingIdx_derived: Float64Array = new Float64Array(1);
  /** @internal */
  ringPos_BodyShortTrailingIdx: number = 0;
  /** @internal */
  ringCap_BodyShortTrailingIdx: number = 0;
  /** @internal */
  ring_BodyShortTrailingIdx_derived: Float64Array = new Float64Array(1);
  /** @internal */
  cs_BodyDoji_rangeType: number = 0;
  /** @internal */
  cs_BodyDoji_avgPeriod: number = 0;
  /** @internal */
  cs_BodyDoji_factor: number = 0;
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
  static of(core: Core): CdlabandonedbabyStream {
    return new CdlabandonedbabyStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cdlabandonedbaby} reports them over the same bars. Each accepted `update`
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
      throw streamFailure('CDLABANDONEDBABY advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw streamFailure('CDLABANDONEDBABY update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(open) || !Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('CDLABANDONEDBABY update', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    cdlabandonedbabyStepImpl(this, open, high, low, close);
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
      throw nonFiniteBar('CDLABANDONEDBABY peek', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inOpen = open;
    const inHigh = high;
    const inLow = low;
    const inClose = close;
    let cur_outInteger = 0;
    const BodyDoji_rangeType = sp.cs_BodyDoji_rangeType;
    const BodyDoji_avgPeriod = sp.cs_BodyDoji_avgPeriod;
    const BodyDoji_factor = sp.cs_BodyDoji_factor;
    const BodyLong_rangeType = sp.cs_BodyLong_rangeType;
    const BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
    const BodyLong_factor = sp.cs_BodyLong_factor;
    const BodyShort_rangeType = sp.cs_BodyShort_rangeType;
    const BodyShort_avgPeriod = sp.cs_BodyShort_avgPeriod;
    const BodyShort_factor = sp.cs_BodyShort_factor;
    if ((sp.lag1_inLow > sp.lag2_inHigh) &&                     /* upside gap between 1st and 2nd */
        (inHigh < sp.lag1_inLow) &&                             /* downside gap between 2nd and 3rd */
        ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === 1 && /* 1st white */
        ((inClose >= inOpen) ? 1 : -1) === -1)                  /* 3rd black */
    {
      if (Math.abs(sp.lag2_inClose - sp.lag2_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st: long */
          Math.abs(sp.lag1_inClose - sp.lag1_inOpen) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod !== 0) ? (sp.BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyDoji_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyDoji_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((BodyDoji_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd: doji */
          Math.abs(inClose - inOpen) > ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (sp.BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) && /* 3rd: longer than short */
          inClose < sp.lag2_inClose - Math.abs(sp.lag2_inClose - sp.lag2_inOpen) * sp.optInPenetration) /* 3rd closes well within 1st rb */
      {
        cur_outInteger = ((((inClose >= inOpen) ? 1 : -1) * 100) | 0);
      } else {
        cur_outInteger = 0;
      }
    } else if ((sp.lag1_inHigh < sp.lag2_inLow) &&                      /* downside gap between 1st and 2nd */
        (inLow > sp.lag1_inHigh) &&                              /* upside gap between 2nd and 3rd */
        ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === -1 && /* 1st black */
        ((inClose >= inOpen) ? 1 : -1) === 1)                    /* 3rd white */
    {
      if (Math.abs(sp.lag2_inClose - sp.lag2_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st: long */
          Math.abs(sp.lag1_inClose - sp.lag1_inOpen) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod !== 0) ? (sp.BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyDoji_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyDoji_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((BodyDoji_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd: doji */
          Math.abs(inClose - inOpen) > ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (sp.BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) && /* 3rd: longer than short */
          inClose > fma(Math.abs(sp.lag2_inClose - sp.lag2_inOpen), sp.optInPenetration, sp.lag2_inClose)) /* 3rd closes well within 1st rb */
      {
        cur_outInteger = ((((inClose >= inOpen) ? 1 : -1) * 100) | 0);
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
  clone(): CdlabandonedbabyStream {
    const c = new CdlabandonedbabyStream(this.core);
    c.optInPenetration = this.optInPenetration;
    c.BodyDojiPeriodTotal = this.BodyDojiPeriodTotal;
    c.BodyLongPeriodTotal = this.BodyLongPeriodTotal;
    c.BodyShortPeriodTotal = this.BodyShortPeriodTotal;
    c.lag1_inOpen = this.lag1_inOpen;
    c.lag2_inOpen = this.lag2_inOpen;
    c.lag1_inHigh = this.lag1_inHigh;
    c.lag2_inHigh = this.lag2_inHigh;
    c.lag1_inLow = this.lag1_inLow;
    c.lag2_inLow = this.lag2_inLow;
    c.lag1_inClose = this.lag1_inClose;
    c.lag2_inClose = this.lag2_inClose;
    c.ringPos_BodyDojiTrailingIdx = this.ringPos_BodyDojiTrailingIdx;
    c.ringCap_BodyDojiTrailingIdx = this.ringCap_BodyDojiTrailingIdx;
    c.ring_BodyDojiTrailingIdx_derived = this.ring_BodyDojiTrailingIdx_derived.slice();
    c.ringPos_BodyLongTrailingIdx = this.ringPos_BodyLongTrailingIdx;
    c.ringCap_BodyLongTrailingIdx = this.ringCap_BodyLongTrailingIdx;
    c.ring_BodyLongTrailingIdx_derived = this.ring_BodyLongTrailingIdx_derived.slice();
    c.ringPos_BodyShortTrailingIdx = this.ringPos_BodyShortTrailingIdx;
    c.ringCap_BodyShortTrailingIdx = this.ringCap_BodyShortTrailingIdx;
    c.ring_BodyShortTrailingIdx_derived = this.ring_BodyShortTrailingIdx_derived.slice();
    c.cs_BodyDoji_rangeType = this.cs_BodyDoji_rangeType;
    c.cs_BodyDoji_avgPeriod = this.cs_BodyDoji_avgPeriod;
    c.cs_BodyDoji_factor = this.cs_BodyDoji_factor;
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

function cdlabandonedbabyStepImpl(sp: CdlabandonedbabyStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  let ringPosL_BodyDojiTrailingIdx = 0;
  let ringPosL_BodyLongTrailingIdx = 0;
  let ringPosL_BodyShortTrailingIdx = 0;
  const BodyDoji_rangeType = sp.cs_BodyDoji_rangeType;
  const BodyDoji_avgPeriod = sp.cs_BodyDoji_avgPeriod;
  const BodyDoji_factor = sp.cs_BodyDoji_factor;
  const BodyLong_rangeType = sp.cs_BodyLong_rangeType;
  const BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
  const BodyLong_factor = sp.cs_BodyLong_factor;
  const BodyShort_rangeType = sp.cs_BodyShort_rangeType;
  const BodyShort_avgPeriod = sp.cs_BodyShort_avgPeriod;
  const BodyShort_factor = sp.cs_BodyShort_factor;
  if (sp.ringCap_BodyDojiTrailingIdx === 0) {
    sp.ring_BodyDojiTrailingIdx_derived[0] = ((BodyDoji_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyDoji_rangeType === 1) ? (inHigh - inLow) : ((BodyDoji_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
  if (sp.ringCap_BodyLongTrailingIdx === 0) {
    sp.ring_BodyLongTrailingIdx_derived[0] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType === 1) ? (inHigh - inLow) : ((BodyLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
  if (sp.ringCap_BodyShortTrailingIdx === 0) {
    sp.ring_BodyShortTrailingIdx_derived[0] = ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
  if ((sp.lag1_inLow > sp.lag2_inHigh) &&                     /* upside gap between 1st and 2nd */
      (inHigh < sp.lag1_inLow) &&                             /* downside gap between 2nd and 3rd */
      ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === 1 && /* 1st white */
      ((inClose >= inOpen) ? 1 : -1) === -1)                  /* 3rd black */
  {
    if (Math.abs(sp.lag2_inClose - sp.lag2_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st: long */
        Math.abs(sp.lag1_inClose - sp.lag1_inOpen) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod !== 0) ? (sp.BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyDoji_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyDoji_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((BodyDoji_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd: doji */
        Math.abs(inClose - inOpen) > ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (sp.BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) && /* 3rd: longer than short */
        inClose < sp.lag2_inClose - Math.abs(sp.lag2_inClose - sp.lag2_inOpen) * sp.optInPenetration) /* 3rd closes well within 1st rb */
    {
      sp.cur_outInteger = ((((inClose >= inOpen) ? 1 : -1) * 100) | 0);
    } else {
      sp.cur_outInteger = 0;
    }
  } else if ((sp.lag1_inHigh < sp.lag2_inLow) &&                      /* downside gap between 1st and 2nd */
      (inLow > sp.lag1_inHigh) &&                              /* upside gap between 2nd and 3rd */
      ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === -1 && /* 1st black */
      ((inClose >= inOpen) ? 1 : -1) === 1)                    /* 3rd white */
  {
    if (Math.abs(sp.lag2_inClose - sp.lag2_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st: long */
        Math.abs(sp.lag1_inClose - sp.lag1_inOpen) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod !== 0) ? (sp.BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyDoji_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyDoji_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((BodyDoji_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd: doji */
        Math.abs(inClose - inOpen) > ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (sp.BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) && /* 3rd: longer than short */
        inClose > fma(Math.abs(sp.lag2_inClose - sp.lag2_inOpen), sp.optInPenetration, sp.lag2_inClose)) /* 3rd closes well within 1st rb */
    {
      sp.cur_outInteger = ((((inClose >= inOpen) ? 1 : -1) * 100) | 0);
    } else {
      sp.cur_outInteger = 0;
    }
  } else {
    sp.cur_outInteger = 0;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  sp.BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0))) - sp.ring_BodyLongTrailingIdx_derived[sp.ringPos_BodyLongTrailingIdx];
  sp.BodyDojiPeriodTotal += ((BodyDoji_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyDoji_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyDoji_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0))) - sp.ring_BodyDojiTrailingIdx_derived[sp.ringPos_BodyDojiTrailingIdx];
  sp.BodyShortPeriodTotal += ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0))) - sp.ring_BodyShortTrailingIdx_derived[sp.ringPos_BodyShortTrailingIdx];
  sp.lag2_inOpen = sp.lag1_inOpen;
  sp.lag1_inOpen = inOpen;
  sp.lag2_inHigh = sp.lag1_inHigh;
  sp.lag1_inHigh = inHigh;
  sp.lag2_inLow = sp.lag1_inLow;
  sp.lag1_inLow = inLow;
  sp.lag2_inClose = sp.lag1_inClose;
  sp.lag1_inClose = inClose;
  ringPosL_BodyDojiTrailingIdx = sp.ringPos_BodyDojiTrailingIdx;
  sp.ring_BodyDojiTrailingIdx_derived[ringPosL_BodyDojiTrailingIdx] = ((BodyDoji_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyDoji_rangeType === 1) ? (inHigh - inLow) : ((BodyDoji_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  ringPosL_BodyDojiTrailingIdx = ringPosL_BodyDojiTrailingIdx + 1;
  ringPosL_BodyDojiTrailingIdx = ringPosL_BodyDojiTrailingIdx & (ringPosL_BodyDojiTrailingIdx - sp.ringCap_BodyDojiTrailingIdx >> 31);
  sp.ringPos_BodyDojiTrailingIdx = ringPosL_BodyDojiTrailingIdx;
  ringPosL_BodyLongTrailingIdx = sp.ringPos_BodyLongTrailingIdx;
  sp.ring_BodyLongTrailingIdx_derived[ringPosL_BodyLongTrailingIdx] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType === 1) ? (inHigh - inLow) : ((BodyLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  ringPosL_BodyLongTrailingIdx = ringPosL_BodyLongTrailingIdx + 1;
  ringPosL_BodyLongTrailingIdx = ringPosL_BodyLongTrailingIdx & (ringPosL_BodyLongTrailingIdx - sp.ringCap_BodyLongTrailingIdx >> 31);
  sp.ringPos_BodyLongTrailingIdx = ringPosL_BodyLongTrailingIdx;
  ringPosL_BodyShortTrailingIdx = sp.ringPos_BodyShortTrailingIdx;
  sp.ring_BodyShortTrailingIdx_derived[ringPosL_BodyShortTrailingIdx] = ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  ringPosL_BodyShortTrailingIdx = ringPosL_BodyShortTrailingIdx + 1;
  ringPosL_BodyShortTrailingIdx = ringPosL_BodyShortTrailingIdx & (ringPosL_BodyShortTrailingIdx - sp.ringCap_BodyShortTrailingIdx >> 31);
  sp.ringPos_BodyShortTrailingIdx = ringPosL_BodyShortTrailingIdx;
}

function cdlabandonedbabyOpenImpl(
  sp: CdlabandonedbabyStream,
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
  let BodyDojiPeriodTotal = 0;
  let BodyLongPeriodTotal = 0;
  let BodyShortPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let BodyDojiTrailingIdx = 0;
  let BodyLongTrailingIdx = 0;
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
  if (optInPenetration === Core.REAL_DEFAULT) {
    optInPenetration = 0.3;
  } else if (!(optInPenetration >= 0e0 && optInPenetration <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const BodyDoji_rangeType: number = core.candleSettings[CandleSettingType.BODY_DOJI].rangeType;
  const BodyDoji_avgPeriod = core.candleSettings[CandleSettingType.BODY_DOJI].avgPeriod;
  const BodyDoji_factor = core.candleSettings[CandleSettingType.BODY_DOJI].factor;
  const BodyLong_rangeType: number = core.candleSettings[CandleSettingType.BODY_LONG].rangeType;
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  const BodyLong_factor = core.candleSettings[CandleSettingType.BODY_LONG].factor;
  const BodyShort_rangeType: number = core.candleSettings[CandleSettingType.BODY_SHORT].rangeType;
  const BodyShort_avgPeriod = core.candleSettings[CandleSettingType.BODY_SHORT].avgPeriod;
  const BodyShort_factor = core.candleSettings[CandleSettingType.BODY_SHORT].factor;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlabandonedbabyLookbackPositional(optInPenetration, core);
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
  BodyLongPeriodTotal = 0;
  BodyDojiPeriodTotal = 0;
  BodyShortPeriodTotal = 0;
  BodyLongTrailingIdx = startIdx - 2 - BodyLong_avgPeriod;
  BodyDojiTrailingIdx = startIdx - 1 - BodyDoji_avgPeriod;
  BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
  i = BodyLongTrailingIdx;
  while (i < startIdx - 2) {
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = BodyDojiTrailingIdx;
  while (i < startIdx - 1) {
    BodyDojiPeriodTotal += ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
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
   * - first candle: long white (black) real body
   * - second candle: doji
   * - third candle: black (white) real body that moves well within the first candle's real body
   * - upside (downside) gap between the first candle and the doji (the shadows of the two candles don't touch)
   * - downside (upside) gap between the doji and the third candle (the shadows of the two candles don't touch)
   * The meaning of "doji" and "long" is specified with TA_SetCandleSettings
   * The meaning of "moves well within" is specified with optInPenetration and "moves" should mean the real body should
   * not be short ("short" is specified with TA_SetCandleSettings) - Greg Morris wants it to be long, someone else want
   * it to be relatively long
   */
  outIdx = 0;
  do {
    if ((inLow[i - 1] > inHigh[i - 2]) &&                     /* upside gap between 1st and 2nd */
        (inHigh[i] < inLow[i - 1]) &&                         /* downside gap between 2nd and 3rd */
        ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === 1 && /* 1st white */
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === -1)          /* 3rd black */
    {
      if (Math.abs(inClose[i - 2] - inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st: long */
          Math.abs(inClose[i - 1] - inOpen[i - 1]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod !== 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyDoji_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((BodyDoji_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd: doji */
          Math.abs(inClose[i] - inOpen[i]) > ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) && /* 3rd: longer than short */
          inClose[i] < inClose[i - 2] - Math.abs(inClose[i - 2] - inOpen[i - 2]) * optInPenetration) /* 3rd closes well within 1st rb */
      {
        outInteger[((outIdx++ * outStride) | 0)] = ((((inClose[i] >= inOpen[i]) ? 1 : -1) * 100) | 0);
      } else {
        outInteger[((outIdx++ * outStride) | 0)] = 0;
      }
    } else if ((inHigh[i - 1] < inLow[i - 2]) &&                      /* downside gap between 1st and 2nd */
        (inLow[i] > inHigh[i - 1]) &&                          /* upside gap between 2nd and 3rd */
        ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === -1 && /* 1st black */
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === 1)            /* 3rd white */
    {
      if (Math.abs(inClose[i - 2] - inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st: long */
          Math.abs(inClose[i - 1] - inOpen[i - 1]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod !== 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyDoji_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((BodyDoji_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd: doji */
          Math.abs(inClose[i] - inOpen[i]) > ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) && /* 3rd: longer than short */
          inClose[i] > fma(Math.abs(inClose[i - 2] - inOpen[i - 2]), optInPenetration, inClose[i - 2])) /* 3rd closes well within 1st rb */
      {
        outInteger[((outIdx++ * outStride) | 0)] = ((((inClose[i] >= inOpen[i]) ? 1 : -1) * 100) | 0);
      } else {
        outInteger[((outIdx++ * outStride) | 0)] = 0;
      }
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0))) - ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyLongTrailingIdx] - (((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inClose[BodyLongTrailingIdx]) : (inOpen[BodyLongTrailingIdx]))) + ((((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inOpen[BodyLongTrailingIdx]) : (inClose[BodyLongTrailingIdx])) - inLow[BodyLongTrailingIdx])) : 0.0)));
    BodyDojiPeriodTotal += ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyDoji_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : ((BodyDoji_rangeType === 1) ? (inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) : ((BodyDoji_rangeType === 2) ? ((inHigh[BodyDojiTrailingIdx] - (((inClose[BodyDojiTrailingIdx]) >= (inOpen[BodyDojiTrailingIdx])) ? (inClose[BodyDojiTrailingIdx]) : (inOpen[BodyDojiTrailingIdx]))) + ((((inClose[BodyDojiTrailingIdx]) >= (inOpen[BodyDojiTrailingIdx])) ? (inOpen[BodyDojiTrailingIdx]) : (inClose[BodyDojiTrailingIdx])) - inLow[BodyDojiTrailingIdx])) : 0.0)));
    BodyShortPeriodTotal += ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyShort_rangeType === 0) ? (Math.abs(inClose[BodyShortTrailingIdx] - inOpen[BodyShortTrailingIdx])) : ((BodyShort_rangeType === 1) ? (inHigh[BodyShortTrailingIdx] - inLow[BodyShortTrailingIdx]) : ((BodyShort_rangeType === 2) ? ((inHigh[BodyShortTrailingIdx] - (((inClose[BodyShortTrailingIdx]) >= (inOpen[BodyShortTrailingIdx])) ? (inClose[BodyShortTrailingIdx]) : (inOpen[BodyShortTrailingIdx]))) + ((((inClose[BodyShortTrailingIdx]) >= (inOpen[BodyShortTrailingIdx])) ? (inOpen[BodyShortTrailingIdx]) : (inClose[BodyShortTrailingIdx])) - inLow[BodyShortTrailingIdx])) : 0.0)));
    i += 1;
    BodyLongTrailingIdx += 1;
    BodyDojiTrailingIdx += 1;
    BodyShortTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const cap_BodyDojiTrailingIdx = i - BodyDojiTrailingIdx;
  if (cap_BodyDojiTrailingIdx < 0 || cap_BodyDojiTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_BodyDojiTrailingIdx = cap_BodyDojiTrailingIdx > 0 ? cap_BodyDojiTrailingIdx : 1;
  const capRing_BodyDojiTrailingIdx_derived = new Float64Array(allocN_BodyDojiTrailingIdx);
  for (let fillJ = historyLen - cap_BodyDojiTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_BodyDojiTrailingIdx_derived[fillJ - (historyLen - cap_BodyDojiTrailingIdx)] = ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyDoji_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyDoji_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  const cap_BodyLongTrailingIdx = i - BodyLongTrailingIdx;
  if (cap_BodyLongTrailingIdx < 0 || cap_BodyLongTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_BodyLongTrailingIdx = cap_BodyLongTrailingIdx > 0 ? cap_BodyLongTrailingIdx : 1;
  const capRing_BodyLongTrailingIdx_derived = new Float64Array(allocN_BodyLongTrailingIdx);
  for (let fillJ = historyLen - cap_BodyLongTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_BodyLongTrailingIdx_derived[fillJ - (historyLen - cap_BodyLongTrailingIdx)] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyLong_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyLong_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  const cap_BodyShortTrailingIdx = i - BodyShortTrailingIdx;
  if (cap_BodyShortTrailingIdx < 0 || cap_BodyShortTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_BodyShortTrailingIdx = cap_BodyShortTrailingIdx > 0 ? cap_BodyShortTrailingIdx : 1;
  const capRing_BodyShortTrailingIdx_derived = new Float64Array(allocN_BodyShortTrailingIdx);
  for (let fillJ = historyLen - cap_BodyShortTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_BodyShortTrailingIdx_derived[fillJ - (historyLen - cap_BodyShortTrailingIdx)] = ((BodyShort_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyShort_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyShort_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  sp.optInPenetration = optInPenetration;
  sp.BodyDojiPeriodTotal = BodyDojiPeriodTotal;
  sp.BodyLongPeriodTotal = BodyLongPeriodTotal;
  sp.BodyShortPeriodTotal = BodyShortPeriodTotal;
  sp.lag1_inOpen = inOpen[historyLen - 1];
  sp.lag2_inOpen = inOpen[historyLen - 2];
  sp.lag1_inHigh = inHigh[historyLen - 1];
  sp.lag2_inHigh = inHigh[historyLen - 2];
  sp.lag1_inLow = inLow[historyLen - 1];
  sp.lag2_inLow = inLow[historyLen - 2];
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.lag2_inClose = inClose[historyLen - 2];
  sp.ringPos_BodyDojiTrailingIdx = 0;
  sp.ringCap_BodyDojiTrailingIdx = cap_BodyDojiTrailingIdx;
  sp.ring_BodyDojiTrailingIdx_derived = capRing_BodyDojiTrailingIdx_derived;
  sp.ringPos_BodyLongTrailingIdx = 0;
  sp.ringCap_BodyLongTrailingIdx = cap_BodyLongTrailingIdx;
  sp.ring_BodyLongTrailingIdx_derived = capRing_BodyLongTrailingIdx_derived;
  sp.ringPos_BodyShortTrailingIdx = 0;
  sp.ringCap_BodyShortTrailingIdx = cap_BodyShortTrailingIdx;
  sp.ring_BodyShortTrailingIdx_derived = capRing_BodyShortTrailingIdx_derived;
  sp.cs_BodyDoji_rangeType = BodyDoji_rangeType;
  sp.cs_BodyDoji_avgPeriod = BodyDoji_avgPeriod;
  sp.cs_BodyDoji_factor = BodyDoji_factor;
  sp.cs_BodyLong_rangeType = BodyLong_rangeType;
  sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
  sp.cs_BodyLong_factor = BodyLong_factor;
  sp.cs_BodyShort_rangeType = BodyShort_rangeType;
  sp.cs_BodyShort_avgPeriod = BodyShort_avgPeriod;
  sp.cs_BodyShort_factor = BodyShort_factor;
  sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

function cdlabandonedbabyOpenSinkFrame(
  sp: CdlabandonedbabyStream,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInPenetration: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): RetCode {
  const core = sp.core;
  let BodyDojiPeriodTotal = 0;
  let BodyLongPeriodTotal = 0;
  let BodyShortPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let BodyDojiTrailingIdx = 0;
  let BodyLongTrailingIdx = 0;
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
  if (optInPenetration === Core.REAL_DEFAULT) {
    optInPenetration = 0.3;
  } else if (!(optInPenetration >= 0e0 && optInPenetration <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const BodyDoji_rangeType: number = core.candleSettings[CandleSettingType.BODY_DOJI].rangeType;
  const BodyDoji_avgPeriod = core.candleSettings[CandleSettingType.BODY_DOJI].avgPeriod;
  const BodyDoji_factor = core.candleSettings[CandleSettingType.BODY_DOJI].factor;
  const BodyLong_rangeType: number = core.candleSettings[CandleSettingType.BODY_LONG].rangeType;
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  const BodyLong_factor = core.candleSettings[CandleSettingType.BODY_LONG].factor;
  const BodyShort_rangeType: number = core.candleSettings[CandleSettingType.BODY_SHORT].rangeType;
  const BodyShort_avgPeriod = core.candleSettings[CandleSettingType.BODY_SHORT].avgPeriod;
  const BodyShort_factor = core.candleSettings[CandleSettingType.BODY_SHORT].factor;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlabandonedbabyLookbackPositional(optInPenetration, core);
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
  BodyLongPeriodTotal = 0;
  BodyDojiPeriodTotal = 0;
  BodyShortPeriodTotal = 0;
  BodyLongTrailingIdx = startIdx - 2 - BodyLong_avgPeriod;
  BodyDojiTrailingIdx = startIdx - 1 - BodyDoji_avgPeriod;
  BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
  i = BodyLongTrailingIdx;
  while (i < startIdx - 2) {
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = BodyDojiTrailingIdx;
  while (i < startIdx - 1) {
    BodyDojiPeriodTotal += ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
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
   * - first candle: long white (black) real body
   * - second candle: doji
   * - third candle: black (white) real body that moves well within the first candle's real body
   * - upside (downside) gap between the first candle and the doji (the shadows of the two candles don't touch)
   * - downside (upside) gap between the doji and the third candle (the shadows of the two candles don't touch)
   * The meaning of "doji" and "long" is specified with TA_SetCandleSettings
   * The meaning of "moves well within" is specified with optInPenetration and "moves" should mean the real body should
   * not be short ("short" is specified with TA_SetCandleSettings) - Greg Morris wants it to be long, someone else want
   * it to be relatively long
   */
  outIdx = 0;
  while (i < endIdx) {
    outIdx += 1;
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0))) - ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyLongTrailingIdx] - (((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inClose[BodyLongTrailingIdx]) : (inOpen[BodyLongTrailingIdx]))) + ((((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inOpen[BodyLongTrailingIdx]) : (inClose[BodyLongTrailingIdx])) - inLow[BodyLongTrailingIdx])) : 0.0)));
    BodyDojiPeriodTotal += ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyDoji_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : ((BodyDoji_rangeType === 1) ? (inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) : ((BodyDoji_rangeType === 2) ? ((inHigh[BodyDojiTrailingIdx] - (((inClose[BodyDojiTrailingIdx]) >= (inOpen[BodyDojiTrailingIdx])) ? (inClose[BodyDojiTrailingIdx]) : (inOpen[BodyDojiTrailingIdx]))) + ((((inClose[BodyDojiTrailingIdx]) >= (inOpen[BodyDojiTrailingIdx])) ? (inOpen[BodyDojiTrailingIdx]) : (inClose[BodyDojiTrailingIdx])) - inLow[BodyDojiTrailingIdx])) : 0.0)));
    BodyShortPeriodTotal += ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyShort_rangeType === 0) ? (Math.abs(inClose[BodyShortTrailingIdx] - inOpen[BodyShortTrailingIdx])) : ((BodyShort_rangeType === 1) ? (inHigh[BodyShortTrailingIdx] - inLow[BodyShortTrailingIdx]) : ((BodyShort_rangeType === 2) ? ((inHigh[BodyShortTrailingIdx] - (((inClose[BodyShortTrailingIdx]) >= (inOpen[BodyShortTrailingIdx])) ? (inClose[BodyShortTrailingIdx]) : (inOpen[BodyShortTrailingIdx]))) + ((((inClose[BodyShortTrailingIdx]) >= (inOpen[BodyShortTrailingIdx])) ? (inOpen[BodyShortTrailingIdx]) : (inClose[BodyShortTrailingIdx])) - inLow[BodyShortTrailingIdx])) : 0.0)));
    i += 1;
    BodyLongTrailingIdx += 1;
    BodyDojiTrailingIdx += 1;
    BodyShortTrailingIdx += 1;
  }
  if ((inLow[i - 1] > inHigh[i - 2]) &&                     /* upside gap between 1st and 2nd */
      (inHigh[i] < inLow[i - 1]) &&                         /* downside gap between 2nd and 3rd */
      ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === 1 && /* 1st white */
      ((inClose[i] >= inOpen[i]) ? 1 : -1) === -1)          /* 3rd black */
  {
    if (Math.abs(inClose[i - 2] - inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st: long */
        Math.abs(inClose[i - 1] - inOpen[i - 1]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod !== 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyDoji_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((BodyDoji_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd: doji */
        Math.abs(inClose[i] - inOpen[i]) > ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) && /* 3rd: longer than short */
        inClose[i] < inClose[i - 2] - Math.abs(inClose[i - 2] - inOpen[i - 2]) * optInPenetration) /* 3rd closes well within 1st rb */
    {
      outInteger[0] = ((((inClose[i] >= inOpen[i]) ? 1 : -1) * 100) | 0);
      outIdx += 1;
    } else {
      outInteger[0] = 0;
      outIdx += 1;
    }
  } else if ((inHigh[i - 1] < inLow[i - 2]) &&                      /* downside gap between 1st and 2nd */
      (inLow[i] > inHigh[i - 1]) &&                          /* upside gap between 2nd and 3rd */
      ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === -1 && /* 1st black */
      ((inClose[i] >= inOpen[i]) ? 1 : -1) === 1)            /* 3rd white */
  {
    if (Math.abs(inClose[i - 2] - inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st: long */
        Math.abs(inClose[i - 1] - inOpen[i - 1]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod !== 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyDoji_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((BodyDoji_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd: doji */
        Math.abs(inClose[i] - inOpen[i]) > ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) && /* 3rd: longer than short */
        inClose[i] > fma(Math.abs(inClose[i - 2] - inOpen[i - 2]), optInPenetration, inClose[i - 2])) /* 3rd closes well within 1st rb */
    {
      outInteger[0] = ((((inClose[i] >= inOpen[i]) ? 1 : -1) * 100) | 0);
      outIdx += 1;
    } else {
      outInteger[0] = 0;
      outIdx += 1;
    }
  } else {
    outInteger[0] = 0;
    outIdx += 1;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0))) - ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyLongTrailingIdx] - (((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inClose[BodyLongTrailingIdx]) : (inOpen[BodyLongTrailingIdx]))) + ((((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inOpen[BodyLongTrailingIdx]) : (inClose[BodyLongTrailingIdx])) - inLow[BodyLongTrailingIdx])) : 0.0)));
  BodyDojiPeriodTotal += ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyDoji_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : ((BodyDoji_rangeType === 1) ? (inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) : ((BodyDoji_rangeType === 2) ? ((inHigh[BodyDojiTrailingIdx] - (((inClose[BodyDojiTrailingIdx]) >= (inOpen[BodyDojiTrailingIdx])) ? (inClose[BodyDojiTrailingIdx]) : (inOpen[BodyDojiTrailingIdx]))) + ((((inClose[BodyDojiTrailingIdx]) >= (inOpen[BodyDojiTrailingIdx])) ? (inOpen[BodyDojiTrailingIdx]) : (inClose[BodyDojiTrailingIdx])) - inLow[BodyDojiTrailingIdx])) : 0.0)));
  BodyShortPeriodTotal += ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyShort_rangeType === 0) ? (Math.abs(inClose[BodyShortTrailingIdx] - inOpen[BodyShortTrailingIdx])) : ((BodyShort_rangeType === 1) ? (inHigh[BodyShortTrailingIdx] - inLow[BodyShortTrailingIdx]) : ((BodyShort_rangeType === 2) ? ((inHigh[BodyShortTrailingIdx] - (((inClose[BodyShortTrailingIdx]) >= (inOpen[BodyShortTrailingIdx])) ? (inClose[BodyShortTrailingIdx]) : (inOpen[BodyShortTrailingIdx]))) + ((((inClose[BodyShortTrailingIdx]) >= (inOpen[BodyShortTrailingIdx])) ? (inOpen[BodyShortTrailingIdx]) : (inClose[BodyShortTrailingIdx])) - inLow[BodyShortTrailingIdx])) : 0.0)));
  i += 1;
  BodyLongTrailingIdx += 1;
  BodyDojiTrailingIdx += 1;
  BodyShortTrailingIdx += 1;
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const cap_BodyDojiTrailingIdx = i - BodyDojiTrailingIdx;
  if (cap_BodyDojiTrailingIdx < 0 || cap_BodyDojiTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_BodyDojiTrailingIdx = cap_BodyDojiTrailingIdx > 0 ? cap_BodyDojiTrailingIdx : 1;
  const capRing_BodyDojiTrailingIdx_derived = new Float64Array(allocN_BodyDojiTrailingIdx);
  for (let fillJ = historyLen - cap_BodyDojiTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_BodyDojiTrailingIdx_derived[fillJ - (historyLen - cap_BodyDojiTrailingIdx)] = ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyDoji_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyDoji_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  const cap_BodyLongTrailingIdx = i - BodyLongTrailingIdx;
  if (cap_BodyLongTrailingIdx < 0 || cap_BodyLongTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_BodyLongTrailingIdx = cap_BodyLongTrailingIdx > 0 ? cap_BodyLongTrailingIdx : 1;
  const capRing_BodyLongTrailingIdx_derived = new Float64Array(allocN_BodyLongTrailingIdx);
  for (let fillJ = historyLen - cap_BodyLongTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_BodyLongTrailingIdx_derived[fillJ - (historyLen - cap_BodyLongTrailingIdx)] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyLong_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyLong_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  const cap_BodyShortTrailingIdx = i - BodyShortTrailingIdx;
  if (cap_BodyShortTrailingIdx < 0 || cap_BodyShortTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_BodyShortTrailingIdx = cap_BodyShortTrailingIdx > 0 ? cap_BodyShortTrailingIdx : 1;
  const capRing_BodyShortTrailingIdx_derived = new Float64Array(allocN_BodyShortTrailingIdx);
  for (let fillJ = historyLen - cap_BodyShortTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_BodyShortTrailingIdx_derived[fillJ - (historyLen - cap_BodyShortTrailingIdx)] = ((BodyShort_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyShort_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyShort_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  sp.optInPenetration = optInPenetration;
  sp.BodyDojiPeriodTotal = BodyDojiPeriodTotal;
  sp.BodyLongPeriodTotal = BodyLongPeriodTotal;
  sp.BodyShortPeriodTotal = BodyShortPeriodTotal;
  sp.lag1_inOpen = inOpen[historyLen - 1];
  sp.lag2_inOpen = inOpen[historyLen - 2];
  sp.lag1_inHigh = inHigh[historyLen - 1];
  sp.lag2_inHigh = inHigh[historyLen - 2];
  sp.lag1_inLow = inLow[historyLen - 1];
  sp.lag2_inLow = inLow[historyLen - 2];
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.lag2_inClose = inClose[historyLen - 2];
  sp.ringPos_BodyDojiTrailingIdx = 0;
  sp.ringCap_BodyDojiTrailingIdx = cap_BodyDojiTrailingIdx;
  sp.ring_BodyDojiTrailingIdx_derived = capRing_BodyDojiTrailingIdx_derived;
  sp.ringPos_BodyLongTrailingIdx = 0;
  sp.ringCap_BodyLongTrailingIdx = cap_BodyLongTrailingIdx;
  sp.ring_BodyLongTrailingIdx_derived = capRing_BodyLongTrailingIdx_derived;
  sp.ringPos_BodyShortTrailingIdx = 0;
  sp.ringCap_BodyShortTrailingIdx = cap_BodyShortTrailingIdx;
  sp.ring_BodyShortTrailingIdx_derived = capRing_BodyShortTrailingIdx_derived;
  sp.cs_BodyDoji_rangeType = BodyDoji_rangeType;
  sp.cs_BodyDoji_avgPeriod = BodyDoji_avgPeriod;
  sp.cs_BodyDoji_factor = BodyDoji_factor;
  sp.cs_BodyLong_rangeType = BodyLong_rangeType;
  sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
  sp.cs_BodyLong_factor = BodyLong_factor;
  sp.cs_BodyShort_rangeType = BodyShort_rangeType;
  sp.cs_BodyShort_avgPeriod = BodyShort_avgPeriod;
  sp.cs_BodyShort_factor = BodyShort_factor;
  sp.cur_outInteger = outInteger[0];
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlabandonedbabyOpenAndFillInternal(
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
): CdlabandonedbabyStream {
  const sp = CdlabandonedbabyStream.of(core);
  const retCode = cdlabandonedbabyOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, optInPenetration, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLABANDONEDBABY openAndFill', inOpen.length, startIdx, cdlabandonedbabyLookbackPositional(optInPenetration, core));
  }
  if (retCode === RetCode.BAD_PARAM && cdlabandonedbabyLookbackPositional(optInPenetration, core) < 0) {
    throw rejectedParams('CDLABANDONEDBABY openAndFill', PARAM_RANGES, optInPenetration);
  }
  throw streamFailure('CDLABANDONEDBABY openAndFill', retCode);
}

/** @internal */
export function cdlabandonedbabyOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInPenetration: number,
): CdlabandonedbabyStream {
  const sp = CdlabandonedbabyStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdlabandonedbabyOpenSinkFrame(sp, inOpen, inHigh, inLow, inClose, startIdx, optInPenetration, outBegIdx, outNBElement, sink_outInteger);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLABANDONEDBABY open', inOpen.length, startIdx, cdlabandonedbabyLookbackPositional(optInPenetration, core));
  }
  if (retCode === RetCode.BAD_PARAM && cdlabandonedbabyLookbackPositional(optInPenetration, core) < 0) {
    throw rejectedParams('CDLABANDONEDBABY open', PARAM_RANGES, optInPenetration);
  }
  throw streamFailure('CDLABANDONEDBABY open', retCode);
}

/** @internal */
export function cdlabandonedbabyOpenPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInPenetration: number,
  core: Core = Core.DEFAULT,
): CdlabandonedbabyStream {
  optInPenetration = requireReal('CDLABANDONEDBABY open', 'penetration', optInPenetration);
  requireArgument('CDLABANDONEDBABY open', 'prices.open', inOpen);
  requireHistory('CDLABANDONEDBABY open', 'prices.open', inOpen.length);
  requireArgument('CDLABANDONEDBABY open', 'prices.high', inHigh);
  requireArgument('CDLABANDONEDBABY open', 'prices.low', inLow);
  requireArgument('CDLABANDONEDBABY open', 'prices.close', inClose);
  requireHistoryLength('CDLABANDONEDBABY open', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLABANDONEDBABY open', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLABANDONEDBABY open', 'prices.close', inClose.length, inOpen.length);
  return cdlabandonedbabyOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0, optInPenetration);
}

/** @internal */
export function cdlabandonedbabyOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInPenetration: number,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): CdlabandonedbabyStream {
  optInPenetration = requireReal('CDLABANDONEDBABY openAndFill', 'penetration', optInPenetration);
  requireArgument('CDLABANDONEDBABY openAndFill', 'prices.open', inOpen);
  requireHistory('CDLABANDONEDBABY openAndFill', 'prices.open', inOpen.length);
  requireArgument('CDLABANDONEDBABY openAndFill', 'prices.high', inHigh);
  requireArgument('CDLABANDONEDBABY openAndFill', 'prices.low', inLow);
  requireArgument('CDLABANDONEDBABY openAndFill', 'prices.close', inClose);
  const guardLookback = cdlabandonedbabyLookbackPositional(optInPenetration, core);
  if (guardLookback < 0) {
    throw rejectedParams('CDLABANDONEDBABY openAndFill', PARAM_RANGES, optInPenetration);
  }
  const guardOutLen = openFillCount('CDLABANDONEDBABY openAndFill', inOpen.length, guardLookback);
  requireHistoryLength('CDLABANDONEDBABY openAndFill', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLABANDONEDBABY openAndFill', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLABANDONEDBABY openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('CDLABANDONEDBABY openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('CDLABANDONEDBABY openAndFill', 'outInteger', 'prices.open');
  }
  if (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CDLABANDONEDBABY openAndFill', 'outInteger', 'prices.high');
  }
  if (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CDLABANDONEDBABY openAndFill', 'outInteger', 'prices.low');
  }
  if (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('CDLABANDONEDBABY openAndFill', 'outInteger', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLABANDONEDBABY openAndFill', 'outInteger', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLABANDONEDBABY openAndFill', 'outInteger', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLABANDONEDBABY openAndFill', 'outInteger', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLABANDONEDBABY openAndFill', 'outInteger', 'prices.close');
  }
  return cdlabandonedbabyOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, optInPenetration, new MInteger(), new MInteger(), outInteger);
}

/**
 * Opens a live CDLABANDONEDBABY stream over the warm-up history: `value()` starts
 * at the last history bar, bit-identical to {@link cdlabandonedbaby} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlabandonedbabyOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlabandonedbabyLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, a real parameter that is not a number, a parameter
 *   outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdlabandonedbabyOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdlabandonedbabyOpen({ open, high, low, close }, {
 *   penetration: 0.3,
 * });
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdlabandonedbabyOpen(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlabandonedbabyOptions): CdlabandonedbabyStream {
  requireArgument('CDLABANDONEDBABY open', 'prices', prices);
  const o = optionsOf('CDLABANDONEDBABY open', options);
  const optInPenetration = orDefault(o.penetration, Core.REAL_DEFAULT);
  const core = coreOf('CDLABANDONEDBABY open', o.core);
  return cdlabandonedbabyOpenPositional(prices.open, prices.high, prices.low, prices.close, optInPenetration, core);
}

/**
 * {@link cdlabandonedbabyOpen} that also computes the outputs over the whole
 * history in the same pass, bit-identically to {@link cdlabandonedbaby}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CdlabandonedbabyOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cdlabandonedbaby} returns them; the range is
 *   the stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlabandonedbabyLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, a real parameter that is not a number, a parameter
 *   outside its range, histories of different lengths or a buffer too short, of the
 *   wrong type or starting where another buffer or a `Float64Array` input starts,
 *   or, on a history long enough to open, one sharing memory with another buffer or
 *   a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdlabandonedbabyOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = cdlabandonedbabyOpenAndFill({ open, high, low, close }, {
 *   penetration: 0.3,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cdlabandonedbabyOpenAndFill(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlabandonedbabyOptions): CdlabandonedbabyResult & { stream: CdlabandonedbabyStream } {
  requireArgument('CDLABANDONEDBABY openAndFill', 'prices', prices);
  const o = optionsOf('CDLABANDONEDBABY openAndFill', options);
  const optInPenetration = orDefault(o.penetration, Core.REAL_DEFAULT);
  const core = coreOf('CDLABANDONEDBABY openAndFill', o.core);
  const capacity = fillCapacity(cdlabandonedbabyLookbackPositional(optInPenetration, core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const stream = cdlabandonedbabyOpenAndFillPositional(prices.open, prices.high, prices.low, prices.close, optInPenetration, outInteger, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}
