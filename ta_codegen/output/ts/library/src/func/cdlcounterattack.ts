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
 *  010605 AC     Creation
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 *  110526 MF,CC  Carry the ranges of the previous bars instead of recomputing them
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { CandleSettingType, Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function cdlcounterattackLookbackPositional(core: Core = Core.DEFAULT): number {
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  const Equal_avgPeriod = core.candleSettings[CandleSettingType.EQUAL].avgPeriod;
  return (Equal_avgPeriod > BodyLong_avgPeriod ? Equal_avgPeriod : BodyLong_avgPeriod) + 1;
}

/** @internal */
export function cdlcounterattackDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdlcounterattackImpl(
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
  const BodyLongPeriodTotal = new Float64Array(2);
  let BodyLongPeriodTotalRange0 = 0;
  let BodyLongPeriodTotalTrailRange0 = 0;
  let BodyLongPeriodTotalRange1 = 0;
  let BodyLongPeriodTotalTrailRange1 = 0;
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
  lookbackTotal = cdlcounterattackLookbackPositional(core);
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
  BodyLongPeriodTotal[1] = 0;
  BodyLongPeriodTotal[0] = 0;
  BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
  i = EqualTrailingIdx;
  while (i < startIdx) {
    EqualPeriodTotal += ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    i += 1;
  }
  i = BodyLongTrailingIdx;
  while (i < startIdx) {
    BodyLongPeriodTotal[1] = BodyLongPeriodTotal[1] + ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    BodyLongPeriodTotal[0] = BodyLongPeriodTotal[0] + ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first candle: long black (white)
   * - second candle: long white (black) with close equal to the prior close
   * The meaning of "equal" and "long" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  BodyLongPeriodTotalRange1 = ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
  BodyLongPeriodTotalTrailRange1 = ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 1] - inOpen[BodyLongTrailingIdx - 1])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyLongTrailingIdx - 1] - inLow[BodyLongTrailingIdx - 1]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyLongTrailingIdx - 1] - (((inClose[BodyLongTrailingIdx - 1]) >= (inOpen[BodyLongTrailingIdx - 1])) ? (inClose[BodyLongTrailingIdx - 1]) : (inOpen[BodyLongTrailingIdx - 1]))) + ((((inClose[BodyLongTrailingIdx - 1]) >= (inOpen[BodyLongTrailingIdx - 1])) ? (inOpen[BodyLongTrailingIdx - 1]) : (inClose[BodyLongTrailingIdx - 1])) - inLow[BodyLongTrailingIdx - 1])) : 0.0)));
  do {
    if (((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === (-((inClose[i] >= inOpen[i]) ? 1 : -1) | 0) && /* opposite candles */
        Math.abs(inClose[i - 1] - inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyLongPeriodTotal[1] / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st long */
        Math.abs(inClose[i] - inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyLongPeriodTotal[0] / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd long */
        inClose[i] <= inClose[i - 1] + ((Equal_factor * (((Equal_avgPeriod !== 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))) && /* equal closes */
        inClose[i] >= inClose[i - 1] - ((Equal_factor * (((Equal_avgPeriod !== 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))))
    {
      outInteger[outIdx++] = ((((inClose[i] >= inOpen[i]) ? 1 : -1) * 100) | 0);
    } else {
      outInteger[outIdx++] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    EqualPeriodTotal += ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((Equal_rangeType === 0) ? (Math.abs(inClose[EqualTrailingIdx - 1] - inOpen[EqualTrailingIdx - 1])) : ((Equal_rangeType === 1) ? (inHigh[EqualTrailingIdx - 1] - inLow[EqualTrailingIdx - 1]) : ((Equal_rangeType === 2) ? ((inHigh[EqualTrailingIdx - 1] - (((inClose[EqualTrailingIdx - 1]) >= (inOpen[EqualTrailingIdx - 1])) ? (inClose[EqualTrailingIdx - 1]) : (inOpen[EqualTrailingIdx - 1]))) + ((((inClose[EqualTrailingIdx - 1]) >= (inOpen[EqualTrailingIdx - 1])) ? (inOpen[EqualTrailingIdx - 1]) : (inClose[EqualTrailingIdx - 1])) - inLow[EqualTrailingIdx - 1])) : 0.0)));
    BodyLongPeriodTotalRange0 = ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    BodyLongPeriodTotalTrailRange0 = ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyLongTrailingIdx] - (((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inClose[BodyLongTrailingIdx]) : (inOpen[BodyLongTrailingIdx]))) + ((((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inOpen[BodyLongTrailingIdx]) : (inClose[BodyLongTrailingIdx])) - inLow[BodyLongTrailingIdx])) : 0.0)));
    BodyLongPeriodTotal[1] = BodyLongPeriodTotal[1] + (BodyLongPeriodTotalRange1 - BodyLongPeriodTotalTrailRange1);
    BodyLongPeriodTotal[0] = BodyLongPeriodTotal[0] + (BodyLongPeriodTotalRange0 - BodyLongPeriodTotalTrailRange0);
    BodyLongPeriodTotalRange1 = BodyLongPeriodTotalRange0;
    BodyLongPeriodTotalTrailRange1 = BodyLongPeriodTotalTrailRange0;
    i += 1;
    EqualTrailingIdx += 1;
    BodyLongTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlcounterattackPositional(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDLCOUNTERATTACK', 'startIdx', startIdx);
  endIdx = requireInt32('CDLCOUNTERATTACK', 'endIdx', endIdx);
  requireIndexRange('CDLCOUNTERATTACK', startIdx, endIdx);
  const guardStart = clampedStart('CDLCOUNTERATTACK', startIdx, cdlcounterattackLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLCOUNTERATTACK', 'prices.open', inOpen, guardInLen);
  requireLength('CDLCOUNTERATTACK', 'prices.high', inHigh, guardInLen);
  requireLength('CDLCOUNTERATTACK', 'prices.low', inLow, guardInLen);
  requireLength('CDLCOUNTERATTACK', 'prices.close', inClose, guardInLen);
  requireOutput('CDLCOUNTERATTACK', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLCOUNTERATTACK', 'outInteger', 'prices.open');
  }
  if (overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLCOUNTERATTACK', 'outInteger', 'prices.high');
  }
  if (overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLCOUNTERATTACK', 'outInteger', 'prices.low');
  }
  if (overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLCOUNTERATTACK', 'outInteger', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdlcounterattackImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLCOUNTERATTACK', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cdlcounterattack}, {@link cdlcounterattackLookback},
 * {@link cdlcounterattackDisplayShift}, {@link cdlcounterattackOpen} and
 * {@link cdlcounterattackOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface CdlcounterattackOptions {
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
 * What {@link cdlcounterattack} returns, and {@link cdlcounterattackOpenAndFill}
 * with its stream.
 */
export interface CdlcounterattackResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * +100 when the second candle is white (bullish), -100 when it is black (bearish),
   * 0 when no pattern. `values[k]` is the value at input bar `begIdx + k`.
   */
  values: Int32Array;
}

/**
 * Number of leading input bars {@link cdlcounterattack} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlcounterattackOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlcounterattackLookback(options?: CdlcounterattackOptions): number {
  const o = optionsOf('CDLCOUNTERATTACK lookback', options);
  return cdlcounterattackLookbackPositional(coreOf('CDLCOUNTERATTACK lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdlcounterattack}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlcounterattackOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlcounterattackDisplayShift(outputIdx: number, options?: CdlcounterattackOptions): number {
  const o = optionsOf('CDLCOUNTERATTACK displayShift', options);
  return cdlcounterattackDisplayShiftPositional(outputIdx, coreOf('CDLCOUNTERATTACK displayShift', o.core));
}

/**
 * A two-candle pattern of two long, opposite-colored real bodies whose closing
 * prices are (nearly) equal. Emits a bullish signal when the second candle is
 * white and a bearish signal when it is black (a reversal signal, though its trend
 * context is not checked).
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlcounterattack](https://ta-lib.org/functions/cdlcounterattack).
 *
 * **Notes**
 * - Does not verify the prior trend the reversal signal classically assumes.
 * - Bulkowski's testing found the bearish Counterattack/Meeting Lines does not
 *   reliably reverse at all — it acts as a bullish CONTINUATION 51% of the time —
 *   and the bullish version reverses only 56% of the time, both "near random" by
 *   his classification.
 *   ([thepatternsite.com](https://thepatternsite.com/MeetingLinesBear.html))
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cdlcounterattackLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `open`: Open
 *   price of each bar. `high`: High price of each bar. `low`: Low price of each bar.
 *   `close`: Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link CdlcounterattackOptions}.
 * @returns `begIdx` and the values; see {@link CdlcounterattackResult}. Each
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
 * import { cdlcounterattack } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { begIdx, values } = cdlcounterattack({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`cdlpiercing`](https://ta-lib.org/functions/cdlpiercing)
 * @see [`cdldarkcloudcover`](https://ta-lib.org/functions/cdldarkcloudcover)
 * @see [`cdlgapsidesidewhite`](https://ta-lib.org/functions/cdlgapsidesidewhite)
 */
export function cdlcounterattack(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: CdlcounterattackOptions): CdlcounterattackResult {
  requireArgument('CDLCOUNTERATTACK', 'prices', prices);
  const o = optionsOf('CDLCOUNTERATTACK', options);
  if (o.endIdx === undefined) {
    requireSeries('CDLCOUNTERATTACK', 'prices.open', prices.open);
    requireSeries('CDLCOUNTERATTACK', 'prices.high', prices.high);
    requireSeries('CDLCOUNTERATTACK', 'prices.low', prices.low);
    requireSeries('CDLCOUNTERATTACK', 'prices.close', prices.close);
  }
  const core = coreOf('CDLCOUNTERATTACK', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cdlcounterattackLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const range = cdlcounterattackPositional(startIdx, endIdx, prices.open, prices.high, prices.low, prices.close, outInteger, core);
  return { begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CDLCOUNTERATTACK stream: one value per closed bar, bit-identical to
 * {@link cdlcounterattack} over the same series. Open it with
 * {@link cdlcounterattackOpen} or {@link cdlcounterattackOpenAndFill}; there is
 * nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlcounterattack](https://ta-lib.org/functions/cdlcounterattack).
 */
export class CdlcounterattackStream {
  /** @internal */
  core: Core;
  /** @internal */
  EqualPeriodTotal: number = 0;
  /** @internal */
  BodyLongPeriodTotal: Float64Array = new Float64Array(2);
  /** @internal */
  BodyLongPeriodTotalRange1: number = 0;
  /** @internal */
  BodyLongPeriodTotalTrailRange1: number = 0;
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
  ringCap_BodyLongTrailingIdx: number = 0;
  /** @internal */
  ring_BodyLongTrailingIdx_derived: Float64Array = new Float64Array(1);
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

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): CdlcounterattackStream {
    return new CdlcounterattackStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cdlcounterattack} reports them over the same bars. Each accepted `update`
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
      throw streamFailure('CDLCOUNTERATTACK advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw streamFailure('CDLCOUNTERATTACK update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(open) || !Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('CDLCOUNTERATTACK update', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    cdlcounterattackStepImpl(this, open, high, low, close);
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
      throw nonFiniteBar('CDLCOUNTERATTACK peek', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
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
    const Equal_rangeType = sp.cs_Equal_rangeType;
    const Equal_avgPeriod = sp.cs_Equal_avgPeriod;
    const Equal_factor = sp.cs_Equal_factor;
    if (((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === (-((inClose >= inOpen) ? 1 : -1) | 0) && /* opposite candles */
        Math.abs(sp.lag1_inClose - sp.lag1_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyLongPeriodTotal[1] / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st long */
        Math.abs(inClose - inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyLongPeriodTotal[0] / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType === 1) ? (inHigh - inLow) : ((BodyLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd long */
        inClose <= sp.lag1_inClose + ((Equal_factor * (((Equal_avgPeriod !== 0) ? (sp.EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))) && /* equal closes */
        inClose >= sp.lag1_inClose - ((Equal_factor * (((Equal_avgPeriod !== 0) ? (sp.EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))))
    {
      cur_outInteger = ((((inClose >= inOpen) ? 1 : -1) * 100) | 0);
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
  clone(): CdlcounterattackStream {
    const c = new CdlcounterattackStream(this.core);
    c.EqualPeriodTotal = this.EqualPeriodTotal;
    c.BodyLongPeriodTotal = this.BodyLongPeriodTotal.slice();
    c.BodyLongPeriodTotalRange1 = this.BodyLongPeriodTotalRange1;
    c.BodyLongPeriodTotalTrailRange1 = this.BodyLongPeriodTotalTrailRange1;
    c.lag1_inOpen = this.lag1_inOpen;
    c.lag1_inHigh = this.lag1_inHigh;
    c.lag1_inLow = this.lag1_inLow;
    c.lag1_inClose = this.lag1_inClose;
    c.ringPos_BodyLongTrailingIdx = this.ringPos_BodyLongTrailingIdx;
    c.ringCap_BodyLongTrailingIdx = this.ringCap_BodyLongTrailingIdx;
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

function cdlcounterattackStepImpl(sp: CdlcounterattackStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  let BodyLongPeriodTotalRange0 = 0;
  let BodyLongPeriodTotalTrailRange0 = 0;
  const BodyLong_rangeType = sp.cs_BodyLong_rangeType;
  const BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
  const BodyLong_factor = sp.cs_BodyLong_factor;
  const Equal_rangeType = sp.cs_Equal_rangeType;
  const Equal_avgPeriod = sp.cs_Equal_avgPeriod;
  const Equal_factor = sp.cs_Equal_factor;
  if (sp.ringCap_BodyLongTrailingIdx === 0) {
    sp.ring_BodyLongTrailingIdx_derived[0] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType === 1) ? (inHigh - inLow) : ((BodyLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
  sp.ring_EqualTrailingIdx_derived[sp.ringPos_EqualTrailingIdx] = ((Equal_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((Equal_rangeType === 1) ? (inHigh - inLow) : ((Equal_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  if (((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === (-((inClose >= inOpen) ? 1 : -1) | 0) && /* opposite candles */
      Math.abs(sp.lag1_inClose - sp.lag1_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyLongPeriodTotal[1] / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st long */
      Math.abs(inClose - inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyLongPeriodTotal[0] / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType === 1) ? (inHigh - inLow) : ((BodyLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd long */
      inClose <= sp.lag1_inClose + ((Equal_factor * (((Equal_avgPeriod !== 0) ? (sp.EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))) && /* equal closes */
      inClose >= sp.lag1_inClose - ((Equal_factor * (((Equal_avgPeriod !== 0) ? (sp.EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))))
  {
    sp.cur_outInteger = ((((inClose >= inOpen) ? 1 : -1) * 100) | 0);
  } else {
    sp.cur_outInteger = 0;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  sp.EqualPeriodTotal += ((Equal_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0))) - sp.ring_EqualTrailingIdx_derived[(((sp.ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - sp.ringLag_EqualTrailingIdx - 1) % sp.ringCap_EqualTrailingIdx) | 0)];
  BodyLongPeriodTotalRange0 = ((BodyLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType === 1) ? (inHigh - inLow) : ((BodyLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  BodyLongPeriodTotalTrailRange0 = sp.ring_BodyLongTrailingIdx_derived[sp.ringPos_BodyLongTrailingIdx];
  sp.BodyLongPeriodTotal[1] = sp.BodyLongPeriodTotal[1] + (sp.BodyLongPeriodTotalRange1 - sp.BodyLongPeriodTotalTrailRange1);
  sp.BodyLongPeriodTotal[0] = sp.BodyLongPeriodTotal[0] + (BodyLongPeriodTotalRange0 - BodyLongPeriodTotalTrailRange0);
  sp.BodyLongPeriodTotalRange1 = BodyLongPeriodTotalRange0;
  sp.BodyLongPeriodTotalTrailRange1 = BodyLongPeriodTotalTrailRange0;
  sp.lag1_inOpen = inOpen;
  sp.lag1_inHigh = inHigh;
  sp.lag1_inLow = inLow;
  sp.lag1_inClose = inClose;
  sp.ring_BodyLongTrailingIdx_derived[sp.ringPos_BodyLongTrailingIdx] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType === 1) ? (inHigh - inLow) : ((BodyLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  sp.ringPos_BodyLongTrailingIdx = sp.ringPos_BodyLongTrailingIdx + 1;
  if (sp.ringPos_BodyLongTrailingIdx >= sp.ringCap_BodyLongTrailingIdx) {
    sp.ringPos_BodyLongTrailingIdx = 0;
  }
  sp.ringPos_EqualTrailingIdx = sp.ringPos_EqualTrailingIdx + 1;
  if (sp.ringPos_EqualTrailingIdx >= sp.ringCap_EqualTrailingIdx) {
    sp.ringPos_EqualTrailingIdx = 0;
  }
}

function cdlcounterattackOpenImpl(
  sp: CdlcounterattackStream,
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
  const BodyLongPeriodTotal = new Float64Array(2);
  let BodyLongPeriodTotalRange0 = 0;
  let BodyLongPeriodTotalTrailRange0 = 0;
  let BodyLongPeriodTotalRange1 = 0;
  let BodyLongPeriodTotalTrailRange1 = 0;
  let i = 0;
  let outIdx = 0;
  let EqualTrailingIdx = 0;
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
  const Equal_rangeType: number = core.candleSettings[CandleSettingType.EQUAL].rangeType;
  const Equal_avgPeriod = core.candleSettings[CandleSettingType.EQUAL].avgPeriod;
  const Equal_factor = core.candleSettings[CandleSettingType.EQUAL].factor;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlcounterattackLookbackPositional(core);
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
  BodyLongPeriodTotal[1] = 0;
  BodyLongPeriodTotal[0] = 0;
  BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
  i = EqualTrailingIdx;
  while (i < startIdx) {
    EqualPeriodTotal += ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    i += 1;
  }
  i = BodyLongTrailingIdx;
  while (i < startIdx) {
    BodyLongPeriodTotal[1] = BodyLongPeriodTotal[1] + ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    BodyLongPeriodTotal[0] = BodyLongPeriodTotal[0] + ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first candle: long black (white)
   * - second candle: long white (black) with close equal to the prior close
   * The meaning of "equal" and "long" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  BodyLongPeriodTotalRange1 = ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
  BodyLongPeriodTotalTrailRange1 = ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 1] - inOpen[BodyLongTrailingIdx - 1])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyLongTrailingIdx - 1] - inLow[BodyLongTrailingIdx - 1]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyLongTrailingIdx - 1] - (((inClose[BodyLongTrailingIdx - 1]) >= (inOpen[BodyLongTrailingIdx - 1])) ? (inClose[BodyLongTrailingIdx - 1]) : (inOpen[BodyLongTrailingIdx - 1]))) + ((((inClose[BodyLongTrailingIdx - 1]) >= (inOpen[BodyLongTrailingIdx - 1])) ? (inOpen[BodyLongTrailingIdx - 1]) : (inClose[BodyLongTrailingIdx - 1])) - inLow[BodyLongTrailingIdx - 1])) : 0.0)));
  do {
    if (((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === (-((inClose[i] >= inOpen[i]) ? 1 : -1) | 0) && /* opposite candles */
        Math.abs(inClose[i - 1] - inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyLongPeriodTotal[1] / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st long */
        Math.abs(inClose[i] - inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyLongPeriodTotal[0] / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd long */
        inClose[i] <= inClose[i - 1] + ((Equal_factor * (((Equal_avgPeriod !== 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))) && /* equal closes */
        inClose[i] >= inClose[i - 1] - ((Equal_factor * (((Equal_avgPeriod !== 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Equal_rangeType === 2) ? 2.0 : 1.0)))))
    {
      outInteger[((outIdx++ * outStride) | 0)] = ((((inClose[i] >= inOpen[i]) ? 1 : -1) * 100) | 0);
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    EqualPeriodTotal += ((Equal_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((Equal_rangeType === 0) ? (Math.abs(inClose[EqualTrailingIdx - 1] - inOpen[EqualTrailingIdx - 1])) : ((Equal_rangeType === 1) ? (inHigh[EqualTrailingIdx - 1] - inLow[EqualTrailingIdx - 1]) : ((Equal_rangeType === 2) ? ((inHigh[EqualTrailingIdx - 1] - (((inClose[EqualTrailingIdx - 1]) >= (inOpen[EqualTrailingIdx - 1])) ? (inClose[EqualTrailingIdx - 1]) : (inOpen[EqualTrailingIdx - 1]))) + ((((inClose[EqualTrailingIdx - 1]) >= (inOpen[EqualTrailingIdx - 1])) ? (inOpen[EqualTrailingIdx - 1]) : (inClose[EqualTrailingIdx - 1])) - inLow[EqualTrailingIdx - 1])) : 0.0)));
    BodyLongPeriodTotalRange0 = ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    BodyLongPeriodTotalTrailRange0 = ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyLongTrailingIdx] - (((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inClose[BodyLongTrailingIdx]) : (inOpen[BodyLongTrailingIdx]))) + ((((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inOpen[BodyLongTrailingIdx]) : (inClose[BodyLongTrailingIdx])) - inLow[BodyLongTrailingIdx])) : 0.0)));
    BodyLongPeriodTotal[1] = BodyLongPeriodTotal[1] + (BodyLongPeriodTotalRange1 - BodyLongPeriodTotalTrailRange1);
    BodyLongPeriodTotal[0] = BodyLongPeriodTotal[0] + (BodyLongPeriodTotalRange0 - BodyLongPeriodTotalTrailRange0);
    BodyLongPeriodTotalRange1 = BodyLongPeriodTotalRange0;
    BodyLongPeriodTotalTrailRange1 = BodyLongPeriodTotalTrailRange0;
    i += 1;
    EqualTrailingIdx += 1;
    BodyLongTrailingIdx += 1;
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
  sp.BodyLongPeriodTotalRange1 = BodyLongPeriodTotalRange1;
  sp.BodyLongPeriodTotalTrailRange1 = BodyLongPeriodTotalTrailRange1;
  sp.lag1_inOpen = inOpen[historyLen - 1];
  sp.lag1_inHigh = inHigh[historyLen - 1];
  sp.lag1_inLow = inLow[historyLen - 1];
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.ringPos_BodyLongTrailingIdx = 0;
  sp.ringCap_BodyLongTrailingIdx = cap_BodyLongTrailingIdx;
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
export function cdlcounterattackOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): CdlcounterattackStream {
  const sp = CdlcounterattackStream.of(core);
  const retCode = cdlcounterattackOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLCOUNTERATTACK openAndFill', inOpen.length, startIdx, cdlcounterattackLookbackPositional(core));
  }
  throw streamFailure('CDLCOUNTERATTACK openAndFill', retCode);
}

/** @internal */
export function cdlcounterattackOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): CdlcounterattackStream {
  const sp = CdlcounterattackStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdlcounterattackOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLCOUNTERATTACK open', inOpen.length, startIdx, cdlcounterattackLookbackPositional(core));
  }
  throw streamFailure('CDLCOUNTERATTACK open', retCode);
}

/** @internal */
export function cdlcounterattackOpenPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): CdlcounterattackStream {
  requireArgument('CDLCOUNTERATTACK open', 'prices.open', inOpen);
  requireHistory('CDLCOUNTERATTACK open', 'prices.open', inOpen.length);
  requireArgument('CDLCOUNTERATTACK open', 'prices.high', inHigh);
  requireArgument('CDLCOUNTERATTACK open', 'prices.low', inLow);
  requireArgument('CDLCOUNTERATTACK open', 'prices.close', inClose);
  requireHistoryLength('CDLCOUNTERATTACK open', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLCOUNTERATTACK open', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLCOUNTERATTACK open', 'prices.close', inClose.length, inOpen.length);
  return cdlcounterattackOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/** @internal */
export function cdlcounterattackOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): CdlcounterattackStream {
  requireArgument('CDLCOUNTERATTACK openAndFill', 'prices.open', inOpen);
  requireHistory('CDLCOUNTERATTACK openAndFill', 'prices.open', inOpen.length);
  requireArgument('CDLCOUNTERATTACK openAndFill', 'prices.high', inHigh);
  requireArgument('CDLCOUNTERATTACK openAndFill', 'prices.low', inLow);
  requireArgument('CDLCOUNTERATTACK openAndFill', 'prices.close', inClose);
  const guardOutLen = openFillCount('CDLCOUNTERATTACK openAndFill', inOpen.length, cdlcounterattackLookbackPositional(core));
  requireHistoryLength('CDLCOUNTERATTACK openAndFill', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLCOUNTERATTACK openAndFill', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLCOUNTERATTACK openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('CDLCOUNTERATTACK openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('CDLCOUNTERATTACK openAndFill', 'outInteger', 'prices.open');
  }
  if (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CDLCOUNTERATTACK openAndFill', 'outInteger', 'prices.high');
  }
  if (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CDLCOUNTERATTACK openAndFill', 'outInteger', 'prices.low');
  }
  if (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('CDLCOUNTERATTACK openAndFill', 'outInteger', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLCOUNTERATTACK openAndFill', 'outInteger', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLCOUNTERATTACK openAndFill', 'outInteger', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLCOUNTERATTACK openAndFill', 'outInteger', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLCOUNTERATTACK openAndFill', 'outInteger', 'prices.close');
  }
  return cdlcounterattackOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}

/**
 * Opens a live CDLCOUNTERATTACK stream over the warm-up history: `value()` starts
 * at the last history bar, bit-identical to {@link cdlcounterattack} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `open`: Open price of each bar. `high`: High price of each bar. `low`:
 *   Low price of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlcounterattackOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlcounterattackLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input or histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdlcounterattackOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdlcounterattackOpen({ open, high, low, close });
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdlcounterattackOpen(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: CdlcounterattackOptions): CdlcounterattackStream {
  requireArgument('CDLCOUNTERATTACK open', 'prices', prices);
  const o = optionsOf('CDLCOUNTERATTACK open', options);
  const core = coreOf('CDLCOUNTERATTACK open', o.core);
  return cdlcounterattackOpenPositional(prices.open, prices.high, prices.low, prices.close, core);
}

/**
 * {@link cdlcounterattackOpen} that also computes the outputs over the whole
 * history in the same pass, bit-identically to {@link cdlcounterattack}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `open`: Open price of each bar. `high`: High price of each bar. `low`:
 *   Low price of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CdlcounterattackOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cdlcounterattack} returns them; the range is
 *   the stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlcounterattackLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, histories of different lengths or a buffer too short,
 *   of the wrong type or starting where another buffer or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdlcounterattackOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = cdlcounterattackOpenAndFill({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cdlcounterattackOpenAndFill(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: CdlcounterattackOptions): CdlcounterattackResult & { stream: CdlcounterattackStream } {
  requireArgument('CDLCOUNTERATTACK openAndFill', 'prices', prices);
  const o = optionsOf('CDLCOUNTERATTACK openAndFill', options);
  const core = coreOf('CDLCOUNTERATTACK openAndFill', o.core);
  const capacity = fillCapacity(cdlcounterattackLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const stream = cdlcounterattackOpenAndFillPositional(prices.open, prices.high, prices.low, prices.close, outInteger, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}
