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
 *  100304 AC     Creation
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 *  103126 MF,CC  Nest the cheap, selective pre-check conjuncts in an outer if
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireReal, streamFailure } from '../runtime/checks.ts';
import { CandleSettingType, Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link cdleveningstar} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param optInPenetration - Fraction of the 1st candle's real body the 3rd close
 *   must penetrate below the 1st close; larger requires deeper penetration (default
 *   0.3; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function cdleveningstarLookback(optInPenetration: number, core: Core = Core.DEFAULT): number {
  if (typeof optInPenetration !== 'number') {
    return -1;
  }
  if (optInPenetration === Core.REAL_DEFAULT) {
    optInPenetration = 0.3;
  } else if (!(optInPenetration >= 0e0 && optInPenetration <= Core.REAL_MAX)) {
    return -1;
  }
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  const BodyShort_avgPeriod = core.candleSettings[CandleSettingType.BODY_SHORT].avgPeriod;
  return (BodyShort_avgPeriod > BodyLong_avgPeriod ? BodyShort_avgPeriod : BodyLong_avgPeriod) + 2;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdleveningstar}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInPenetration - Fraction of the 1st candle's real body the 3rd close
 *   must penetrate below the 1st close; larger requires deeper penetration (default
 *   0.3; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function cdleveningstarDisplayShift(optInPenetration: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (cdleveningstarLookback(optInPenetration, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdleveningstarImpl(
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
  let BodyShortPeriodTotal = 0;
  let BodyLongPeriodTotal = 0;
  let BodyShortPeriodTotal2 = 0;
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
    optInPenetration = 0.3;
  } else if (!(optInPenetration >= 0e0 && optInPenetration <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdleveningstarLookback(optInPenetration, core);
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
  BodyShortPeriodTotal = 0;
  BodyShortPeriodTotal2 = 0;
  BodyLongTrailingIdx = startIdx - 2 - BodyLong_avgPeriod;
  BodyShortTrailingIdx = startIdx - 1 - BodyShort_avgPeriod;
  i = BodyLongTrailingIdx;
  while (i < startIdx - 2) {
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = BodyShortTrailingIdx;
  while (i < startIdx - 1) {
    BodyShortPeriodTotal += ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    BodyShortPeriodTotal2 += ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i + 1] - inOpen[i + 1])) : ((BodyShort_rangeType === 1) ? (inHigh[i + 1] - inLow[i + 1]) : ((BodyShort_rangeType === 2) ? ((inHigh[i + 1] - (((inClose[i + 1]) >= (inOpen[i + 1])) ? (inClose[i + 1]) : (inOpen[i + 1]))) + ((((inClose[i + 1]) >= (inOpen[i + 1])) ? (inOpen[i + 1]) : (inClose[i + 1])) - inLow[i + 1])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first candle: long white real body
   * - second candle: star (short real body gapping up)
   * - third candle: black real body that moves well within the first candle's real body
   * The meaning of "short" and "long" is specified with TA_SetCandleSettings
   * The meaning of "moves well within" is specified with optInPenetration and "moves" should mean the real body should
   * not be short ("short" is specified with TA_SetCandleSettings) - Greg Morris wants it to be long, someone else want
   * it to be relatively long
   */
  outIdx = 0;
  do {
    if (((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === 1 && /* white */
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === -1 &&        /* black real body */
        ((inOpen[i - 1] < inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1]) > (inOpen[i - 2] > inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2])) && /* gapping up */
        inClose[i] < inClose[i - 2] - Math.abs(inClose[i - 2] - inOpen[i - 2]) * optInPenetration && /* closing well within 1st rb */
        Math.abs(inClose[i - 2] - inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st: long */
        Math.abs(inClose[i - 1] - inOpen[i - 1]) <= ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd: short */
        Math.abs(inClose[i] - inOpen[i]) > ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (BodyShortPeriodTotal2 / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0))))) /* 3rd: longer than short */
    {
      outInteger[outIdx++] = -100;
    } else {
      outInteger[outIdx++] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0))) - ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyLongTrailingIdx] - (((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inClose[BodyLongTrailingIdx]) : (inOpen[BodyLongTrailingIdx]))) + ((((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inOpen[BodyLongTrailingIdx]) : (inClose[BodyLongTrailingIdx])) - inLow[BodyLongTrailingIdx])) : 0.0)));
    BodyShortPeriodTotal += ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((BodyShort_rangeType === 0) ? (Math.abs(inClose[BodyShortTrailingIdx] - inOpen[BodyShortTrailingIdx])) : ((BodyShort_rangeType === 1) ? (inHigh[BodyShortTrailingIdx] - inLow[BodyShortTrailingIdx]) : ((BodyShort_rangeType === 2) ? ((inHigh[BodyShortTrailingIdx] - (((inClose[BodyShortTrailingIdx]) >= (inOpen[BodyShortTrailingIdx])) ? (inClose[BodyShortTrailingIdx]) : (inOpen[BodyShortTrailingIdx]))) + ((((inClose[BodyShortTrailingIdx]) >= (inOpen[BodyShortTrailingIdx])) ? (inOpen[BodyShortTrailingIdx]) : (inClose[BodyShortTrailingIdx])) - inLow[BodyShortTrailingIdx])) : 0.0)));
    BodyShortPeriodTotal2 += ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyShort_rangeType === 0) ? (Math.abs(inClose[BodyShortTrailingIdx + 1] - inOpen[BodyShortTrailingIdx + 1])) : ((BodyShort_rangeType === 1) ? (inHigh[BodyShortTrailingIdx + 1] - inLow[BodyShortTrailingIdx + 1]) : ((BodyShort_rangeType === 2) ? ((inHigh[BodyShortTrailingIdx + 1] - (((inClose[BodyShortTrailingIdx + 1]) >= (inOpen[BodyShortTrailingIdx + 1])) ? (inClose[BodyShortTrailingIdx + 1]) : (inOpen[BodyShortTrailingIdx + 1]))) + ((((inClose[BodyShortTrailingIdx + 1]) >= (inOpen[BodyShortTrailingIdx + 1])) ? (inOpen[BodyShortTrailingIdx + 1]) : (inClose[BodyShortTrailingIdx + 1])) - inLow[BodyShortTrailingIdx + 1])) : 0.0)));
    i += 1;
    BodyLongTrailingIdx += 1;
    BodyShortTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/**
 * A three-candle bearish reversal pattern: a long white candle, a short-bodied
 * star gapping up, then a black candle closing well down into the first candle's
 * body. A hit signals a bearish reversal (most significant in an uptrend).
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdleveningstar](https://ta-lib.org/functions/cdleveningstar).
 *
 * **Notes**
 * - Does not verify the preceding uptrend the bearish reversal classically
 *   assumes.
 * - The third candle only needs a body longer than short, not the full long body
 *   some definitions require.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link cdleveningstarLookback} is a success
 * with `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inOpen - Open price of each bar.
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param optInPenetration - Fraction of the 1st candle's real body the 3rd close
 *   must penetrate below the 1st close; larger requires deeper penetration (default
 *   0.3; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param outInteger - -100 when detected (always bearish), 0 otherwise. Never
 *   emits +100. Must hold the values produced and never be empty; the input length
 *   always suffices.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The range written: `begIdx` is the first bar with a value, `count` how
 *   many were written.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for an argument that is not an int32, a
 *   real parameter that is not a number, a parameter outside its range, an array too
 *   short for the range, an empty output or an output sharing memory with a
 *   `Float64Array` input it is not. Checked before anything is written.
 *
 * @example
 * ```ts
 * import { cdleveningstar } from 'ta-lib';
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
 * const { begIdx, count } = cdleveningstar(
 *   0, open.length - 1,
 *   open, high, low, close,
 *   0.3,
 *   out,
 * );
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`cdleveningdojistar`](https://ta-lib.org/functions/cdleveningdojistar)
 * @see [`cdlmorningstar`](https://ta-lib.org/functions/cdlmorningstar)
 * @see [`cdlmorningdojistar`](https://ta-lib.org/functions/cdlmorningdojistar)
 */
export function cdleveningstar(
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
  startIdx = requireInt32('CDLEVENINGSTAR', 'startIdx', startIdx);
  endIdx = requireInt32('CDLEVENINGSTAR', 'endIdx', endIdx);
  optInPenetration = requireReal('CDLEVENINGSTAR', 'optInPenetration', optInPenetration);
  requireIndexRange('CDLEVENINGSTAR', startIdx, endIdx);
  const guardStart = clampedStart('CDLEVENINGSTAR', startIdx, cdleveningstarLookback(optInPenetration, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLEVENINGSTAR', 'inOpen', inOpen, guardInLen);
  requireLength('CDLEVENINGSTAR', 'inHigh', inHigh, guardInLen);
  requireLength('CDLEVENINGSTAR', 'inLow', inLow, guardInLen);
  requireLength('CDLEVENINGSTAR', 'inClose', inClose, guardInLen);
  requireLength('CDLEVENINGSTAR', 'outInteger', outInteger, guardOutLen);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64) || overlaps(outInteger, inHighF64) || overlaps(outInteger, inLowF64) || overlaps(outInteger, inCloseF64)) {
    throw failure('CDLEVENINGSTAR', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdleveningstarImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, optInPenetration, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLEVENINGSTAR', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live CDLEVENINGSTAR stream: one value per closed bar, bit-identical to
 * {@link cdleveningstar} over the same series. Open it with
 * {@link cdleveningstarOpen} or {@link cdleveningstarOpenAndFill}; there is
 * nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdleveningstar](https://ta-lib.org/functions/cdleveningstar).
 */
export class CdleveningstarStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInPenetration: number = 0;
  /** @internal */
  BodyShortPeriodTotal: number = 0;
  /** @internal */
  BodyLongPeriodTotal: number = 0;
  /** @internal */
  BodyShortPeriodTotal2: number = 0;
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
  ringCap_BodyLongTrailingIdx: number = 0;
  /** @internal */
  ring_BodyLongTrailingIdx_derived: Float64Array = new Float64Array(1);
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

  /** @internal */
  constructor(core: Core) {
    this.core = core;
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cdleveningstar} reports them over the same bars. Each accepted `update`
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
      throw failure('CDLEVENINGSTAR advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw failure('CDLEVENINGSTAR update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inOpen) || !Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('CDLEVENINGSTAR update', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    cdleveningstarStepImpl(this, inOpen, inHigh, inLow, inClose);
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
      throw nonFiniteBar('CDLEVENINGSTAR peek', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let cur_outInteger = 0;
    const BodyLong_rangeType = sp.cs_BodyLong_rangeType;
    const BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
    const BodyLong_factor = sp.cs_BodyLong_factor;
    const BodyShort_rangeType = sp.cs_BodyShort_rangeType;
    const BodyShort_avgPeriod = sp.cs_BodyShort_avgPeriod;
    const BodyShort_factor = sp.cs_BodyShort_factor;
    if (((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === 1 && /* white */
        ((inClose >= inOpen) ? 1 : -1) === -1 &&                /* black real body */
        ((sp.lag1_inOpen < sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose) > (sp.lag2_inOpen > sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose)) && /* gapping up */
        inClose < sp.lag2_inClose - Math.abs(sp.lag2_inClose - sp.lag2_inOpen) * sp.optInPenetration && /* closing well within 1st rb */
        Math.abs(sp.lag2_inClose - sp.lag2_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st: long */
        Math.abs(sp.lag1_inClose - sp.lag1_inOpen) <= ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (sp.BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyShort_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyShort_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd: short */
        Math.abs(inClose - inOpen) > ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (sp.BodyShortPeriodTotal2 / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0))))) /* 3rd: longer than short */
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
  clone(): CdleveningstarStream {
    const c = new CdleveningstarStream(this.core);
    c.optInPenetration = this.optInPenetration;
    c.BodyShortPeriodTotal = this.BodyShortPeriodTotal;
    c.BodyLongPeriodTotal = this.BodyLongPeriodTotal;
    c.BodyShortPeriodTotal2 = this.BodyShortPeriodTotal2;
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

function cdleveningstarStepImpl(sp: CdleveningstarStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  const BodyLong_rangeType = sp.cs_BodyLong_rangeType;
  const BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
  const BodyLong_factor = sp.cs_BodyLong_factor;
  const BodyShort_rangeType = sp.cs_BodyShort_rangeType;
  const BodyShort_avgPeriod = sp.cs_BodyShort_avgPeriod;
  const BodyShort_factor = sp.cs_BodyShort_factor;
  if (sp.ringCap_BodyLongTrailingIdx === 0) {
    sp.ring_BodyLongTrailingIdx_derived[0] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType === 1) ? (inHigh - inLow) : ((BodyLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
  sp.ring_BodyShortTrailingIdx_derived[sp.ringPos_BodyShortTrailingIdx] = ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  if (((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === 1 && /* white */
      ((inClose >= inOpen) ? 1 : -1) === -1 &&                /* black real body */
      ((sp.lag1_inOpen < sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose) > (sp.lag2_inOpen > sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose)) && /* gapping up */
      inClose < sp.lag2_inClose - Math.abs(sp.lag2_inClose - sp.lag2_inOpen) * sp.optInPenetration && /* closing well within 1st rb */
      Math.abs(sp.lag2_inClose - sp.lag2_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st: long */
      Math.abs(sp.lag1_inClose - sp.lag1_inOpen) <= ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (sp.BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyShort_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyShort_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd: short */
      Math.abs(inClose - inOpen) > ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (sp.BodyShortPeriodTotal2 / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0))))) /* 3rd: longer than short */
  {
    sp.cur_outInteger = -100;
  } else {
    sp.cur_outInteger = 0;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  sp.BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0))) - sp.ring_BodyLongTrailingIdx_derived[sp.ringPos_BodyLongTrailingIdx];
  sp.BodyShortPeriodTotal += ((BodyShort_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyShort_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyShort_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0))) - sp.ring_BodyShortTrailingIdx_derived[(((sp.ringPos_BodyShortTrailingIdx + sp.ringCap_BodyShortTrailingIdx - sp.ringLag_BodyShortTrailingIdx) % sp.ringCap_BodyShortTrailingIdx) | 0)];
  sp.BodyShortPeriodTotal2 += ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0))) - sp.ring_BodyShortTrailingIdx_derived[(((sp.ringPos_BodyShortTrailingIdx + sp.ringCap_BodyShortTrailingIdx - sp.ringLag_BodyShortTrailingIdx + 1) % sp.ringCap_BodyShortTrailingIdx) | 0)];
  sp.lag2_inOpen = sp.lag1_inOpen;
  sp.lag1_inOpen = inOpen;
  sp.lag2_inHigh = sp.lag1_inHigh;
  sp.lag1_inHigh = inHigh;
  sp.lag2_inLow = sp.lag1_inLow;
  sp.lag1_inLow = inLow;
  sp.lag2_inClose = sp.lag1_inClose;
  sp.lag1_inClose = inClose;
  sp.ring_BodyLongTrailingIdx_derived[sp.ringPos_BodyLongTrailingIdx] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType === 1) ? (inHigh - inLow) : ((BodyLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  sp.ringPos_BodyLongTrailingIdx = sp.ringPos_BodyLongTrailingIdx + 1;
  if (sp.ringPos_BodyLongTrailingIdx >= sp.ringCap_BodyLongTrailingIdx) {
    sp.ringPos_BodyLongTrailingIdx = 0;
  }
  sp.ringPos_BodyShortTrailingIdx = sp.ringPos_BodyShortTrailingIdx + 1;
  if (sp.ringPos_BodyShortTrailingIdx >= sp.ringCap_BodyShortTrailingIdx) {
    sp.ringPos_BodyShortTrailingIdx = 0;
  }
}

function cdleveningstarOpenImpl(
  sp: CdleveningstarStream,
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
  let BodyShortPeriodTotal = 0;
  let BodyLongPeriodTotal = 0;
  let BodyShortPeriodTotal2 = 0;
  let i = 0;
  let outIdx = 0;
  let BodyShortTrailingIdx = 0;
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
  if (optInPenetration === Core.REAL_DEFAULT) {
    optInPenetration = 0.3;
  } else if (!(optInPenetration >= 0e0 && optInPenetration <= Core.REAL_MAX)) {
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
  const BodyShort_rangeType: number = core.candleSettings[CandleSettingType.BODY_SHORT].rangeType;
  const BodyShort_avgPeriod = core.candleSettings[CandleSettingType.BODY_SHORT].avgPeriod;
  const BodyShort_factor = core.candleSettings[CandleSettingType.BODY_SHORT].factor;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdleveningstarLookback(optInPenetration, core);
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
  BodyShortPeriodTotal = 0;
  BodyShortPeriodTotal2 = 0;
  BodyLongTrailingIdx = startIdx - 2 - BodyLong_avgPeriod;
  BodyShortTrailingIdx = startIdx - 1 - BodyShort_avgPeriod;
  i = BodyLongTrailingIdx;
  while (i < startIdx - 2) {
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = BodyShortTrailingIdx;
  while (i < startIdx - 1) {
    BodyShortPeriodTotal += ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    BodyShortPeriodTotal2 += ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i + 1] - inOpen[i + 1])) : ((BodyShort_rangeType === 1) ? (inHigh[i + 1] - inLow[i + 1]) : ((BodyShort_rangeType === 2) ? ((inHigh[i + 1] - (((inClose[i + 1]) >= (inOpen[i + 1])) ? (inClose[i + 1]) : (inOpen[i + 1]))) + ((((inClose[i + 1]) >= (inOpen[i + 1])) ? (inOpen[i + 1]) : (inClose[i + 1])) - inLow[i + 1])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first candle: long white real body
   * - second candle: star (short real body gapping up)
   * - third candle: black real body that moves well within the first candle's real body
   * The meaning of "short" and "long" is specified with TA_SetCandleSettings
   * The meaning of "moves well within" is specified with optInPenetration and "moves" should mean the real body should
   * not be short ("short" is specified with TA_SetCandleSettings) - Greg Morris wants it to be long, someone else want
   * it to be relatively long
   */
  outIdx = 0;
  do {
    if (((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === 1 && /* white */
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === -1 &&        /* black real body */
        ((inOpen[i - 1] < inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1]) > (inOpen[i - 2] > inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2])) && /* gapping up */
        inClose[i] < inClose[i - 2] - Math.abs(inClose[i - 2] - inOpen[i - 2]) * optInPenetration && /* closing well within 1st rb */
        Math.abs(inClose[i - 2] - inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st: long */
        Math.abs(inClose[i - 1] - inOpen[i - 1]) <= ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd: short */
        Math.abs(inClose[i] - inOpen[i]) > ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (BodyShortPeriodTotal2 / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0))))) /* 3rd: longer than short */
    {
      outInteger[((outIdx++ * outStride) | 0)] = -100;
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0))) - ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyLongTrailingIdx] - (((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inClose[BodyLongTrailingIdx]) : (inOpen[BodyLongTrailingIdx]))) + ((((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inOpen[BodyLongTrailingIdx]) : (inClose[BodyLongTrailingIdx])) - inLow[BodyLongTrailingIdx])) : 0.0)));
    BodyShortPeriodTotal += ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((BodyShort_rangeType === 0) ? (Math.abs(inClose[BodyShortTrailingIdx] - inOpen[BodyShortTrailingIdx])) : ((BodyShort_rangeType === 1) ? (inHigh[BodyShortTrailingIdx] - inLow[BodyShortTrailingIdx]) : ((BodyShort_rangeType === 2) ? ((inHigh[BodyShortTrailingIdx] - (((inClose[BodyShortTrailingIdx]) >= (inOpen[BodyShortTrailingIdx])) ? (inClose[BodyShortTrailingIdx]) : (inOpen[BodyShortTrailingIdx]))) + ((((inClose[BodyShortTrailingIdx]) >= (inOpen[BodyShortTrailingIdx])) ? (inOpen[BodyShortTrailingIdx]) : (inClose[BodyShortTrailingIdx])) - inLow[BodyShortTrailingIdx])) : 0.0)));
    BodyShortPeriodTotal2 += ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyShort_rangeType === 0) ? (Math.abs(inClose[BodyShortTrailingIdx + 1] - inOpen[BodyShortTrailingIdx + 1])) : ((BodyShort_rangeType === 1) ? (inHigh[BodyShortTrailingIdx + 1] - inLow[BodyShortTrailingIdx + 1]) : ((BodyShort_rangeType === 2) ? ((inHigh[BodyShortTrailingIdx + 1] - (((inClose[BodyShortTrailingIdx + 1]) >= (inOpen[BodyShortTrailingIdx + 1])) ? (inClose[BodyShortTrailingIdx + 1]) : (inOpen[BodyShortTrailingIdx + 1]))) + ((((inClose[BodyShortTrailingIdx + 1]) >= (inOpen[BodyShortTrailingIdx + 1])) ? (inOpen[BodyShortTrailingIdx + 1]) : (inClose[BodyShortTrailingIdx + 1])) - inLow[BodyShortTrailingIdx + 1])) : 0.0)));
    i += 1;
    BodyLongTrailingIdx += 1;
    BodyShortTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const cap_BodyLongTrailingIdx = i - BodyLongTrailingIdx;
  if (cap_BodyLongTrailingIdx < 0 || cap_BodyLongTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_BodyLongTrailingIdx = cap_BodyLongTrailingIdx > 0 ? cap_BodyLongTrailingIdx : 1;
  const capRing_BodyLongTrailingIdx_derived = new Float64Array(allocN_BodyLongTrailingIdx);
  for (let fillJ = historyLen - cap_BodyLongTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_BodyLongTrailingIdx_derived[fillJ - (historyLen - cap_BodyLongTrailingIdx)] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyLong_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyLong_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  const capLag_BodyShortTrailingIdx = i - BodyShortTrailingIdx;
  const cap_BodyShortTrailingIdx = capLag_BodyShortTrailingIdx + 2;
  if (capLag_BodyShortTrailingIdx < 1 || cap_BodyShortTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_BodyShortTrailingIdx = cap_BodyShortTrailingIdx > 0 ? cap_BodyShortTrailingIdx : 1;
  const capRing_BodyShortTrailingIdx_derived = new Float64Array(allocN_BodyShortTrailingIdx);
  for (let fillJ = historyLen - cap_BodyShortTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_BodyShortTrailingIdx_derived[fillJ % cap_BodyShortTrailingIdx] = ((BodyShort_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyShort_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyShort_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  sp.optInPenetration = optInPenetration;
  sp.BodyShortPeriodTotal = BodyShortPeriodTotal;
  sp.BodyLongPeriodTotal = BodyLongPeriodTotal;
  sp.BodyShortPeriodTotal2 = BodyShortPeriodTotal2;
  sp.lag1_inOpen = inOpen[historyLen - 1];
  sp.lag2_inOpen = inOpen[historyLen - 2];
  sp.lag1_inHigh = inHigh[historyLen - 1];
  sp.lag2_inHigh = inHigh[historyLen - 2];
  sp.lag1_inLow = inLow[historyLen - 1];
  sp.lag2_inLow = inLow[historyLen - 2];
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.lag2_inClose = inClose[historyLen - 2];
  sp.ringPos_BodyLongTrailingIdx = 0;
  sp.ringCap_BodyLongTrailingIdx = cap_BodyLongTrailingIdx;
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
export function cdleveningstarOpenAndFillInternal(
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
): CdleveningstarStream {
  const sp = new CdleveningstarStream(core);
  const retCode = cdleveningstarOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, optInPenetration, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLEVENINGSTAR openAndFill', inOpen.length, startIdx, cdleveningstarLookback(optInPenetration, core));
  }
  throw streamFailure('CDLEVENINGSTAR openAndFill', retCode);
}

/** @internal */
export function cdleveningstarOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInPenetration: number,
): CdleveningstarStream {
  const sp = new CdleveningstarStream(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdleveningstarOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, optInPenetration, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLEVENINGSTAR open', inOpen.length, startIdx, cdleveningstarLookback(optInPenetration, core));
  }
  throw streamFailure('CDLEVENINGSTAR open', retCode);
}

/**
 * Opens a live CDLEVENINGSTAR stream over the warm-up history: `value()` starts at
 * the last history bar, bit-identical to {@link cdleveningstar} there.
 *
 * @param inOpen - Warm-up history. Open price of each bar. Every history has the
 *   same length.
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInPenetration - Fraction of the 1st candle's real body the 3rd close
 *   must penetrate below the 1st close; larger requires deeper penetration (default
 *   0.3; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdleveningstarLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for
 *   an empty one, and `TA_BAD_PARAM` for a real parameter that is not a number, a
 *   parameter outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdleveningstarOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdleveningstarOpen(open, high, low, close, 0.3);
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdleveningstarOpen(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInPenetration: number,
  core: Core = Core.DEFAULT,
): CdleveningstarStream {
  optInPenetration = requireReal('CDLEVENINGSTAR open', 'optInPenetration', optInPenetration);
  requireArgument('CDLEVENINGSTAR open', 'inOpen', inOpen);
  requireHistory('CDLEVENINGSTAR open', inOpen.length);
  requireArgument('CDLEVENINGSTAR open', 'inHigh', inHigh);
  requireArgument('CDLEVENINGSTAR open', 'inLow', inLow);
  requireArgument('CDLEVENINGSTAR open', 'inClose', inClose);
  requireHistoryLength('CDLEVENINGSTAR open', 'inHigh', inHigh.length, inOpen.length);
  requireHistoryLength('CDLEVENINGSTAR open', 'inLow', inLow.length, inOpen.length);
  requireHistoryLength('CDLEVENINGSTAR open', 'inClose', inClose.length, inOpen.length);
  return cdleveningstarOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0, optInPenetration);
}

/**
 * {@link cdleveningstarOpen} that also fills the outputs over the whole history in
 * the same pass, bit-identically to {@link cdleveningstar}. The range written is
 * the stream's `outRange()`.
 *
 * @param inOpen - Warm-up history. Open price of each bar. Every history has the
 *   same length.
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInPenetration - Fraction of the 1st candle's real body the 3rd close
 *   must penetrate below the 1st close; larger requires deeper penetration (default
 *   0.3; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param outInteger - -100 when detected (always bearish), 0 otherwise. Never
 *   emits +100. Must hold `historyLen - lookback` values and share no memory with an
 *   input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdleveningstarLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for
 *   an empty one, and `TA_BAD_PARAM` for a real parameter that is not a number, a
 *   parameter outside its range, histories of different lengths or an output too
 *   short or starting where another output or a `Float64Array` input starts, or, on
 *   a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdleveningstarOpenAndFill } from 'ta-lib';
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
 * const stream = cdleveningstarOpenAndFill(open, high, low, close, 0.3, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function cdleveningstarOpenAndFill(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInPenetration: number,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): CdleveningstarStream {
  optInPenetration = requireReal('CDLEVENINGSTAR openAndFill', 'optInPenetration', optInPenetration);
  requireArgument('CDLEVENINGSTAR openAndFill', 'inOpen', inOpen);
  requireHistory('CDLEVENINGSTAR openAndFill', inOpen.length);
  requireArgument('CDLEVENINGSTAR openAndFill', 'inHigh', inHigh);
  requireArgument('CDLEVENINGSTAR openAndFill', 'inLow', inLow);
  requireArgument('CDLEVENINGSTAR openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('CDLEVENINGSTAR openAndFill', inOpen.length, cdleveningstarLookback(optInPenetration, core));
  requireHistoryLength('CDLEVENINGSTAR openAndFill', 'inHigh', inHigh.length, inOpen.length);
  requireHistoryLength('CDLEVENINGSTAR openAndFill', 'inLow', inLow.length, inOpen.length);
  requireHistoryLength('CDLEVENINGSTAR openAndFill', 'inClose', inClose.length, inOpen.length);
  requireLength('CDLEVENINGSTAR openAndFill', 'outInteger', outInteger, guardOutLen);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) || (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) || (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) || (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset)) {
    throw streamFailure('CDLEVENINGSTAR openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outInteger, inOpenF64) || overlaps(outInteger, inHighF64) || overlaps(outInteger, inLowF64) || overlaps(outInteger, inCloseF64))) {
    throw streamFailure('CDLEVENINGSTAR openAndFill', RetCode.BAD_PARAM);
  }
  return cdleveningstarOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, optInPenetration, new MInteger(), new MInteger(), outInteger);
}
