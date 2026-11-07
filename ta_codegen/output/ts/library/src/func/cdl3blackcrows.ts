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
 *  103004 AC     Creation
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 *  103126 MF,CC  Nest the cheap, selective pre-check conjuncts in an outer if
 *  110526 MF,CC  Carry the ranges of the two previous bars instead of recomputing them
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { CandleSettingType, Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function cdl3blackcrowsLookbackPositional(core: Core = Core.DEFAULT): number {
  const ShadowVeryShort_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].avgPeriod;
  return ShadowVeryShort_avgPeriod + 3;
}

/** @internal */
export function cdl3blackcrowsDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdl3blackcrowsImpl(
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
  let range1 = 0;
  let range2 = 0;
  let trailRange1 = 0;
  let trailRange2 = 0;
  let range0 = 0;
  let trailRange0 = 0;
  let i = 0;
  let outIdx = 0;
  let ShadowVeryShortTrailingIdx = 0;
  let lookbackTotal = 0;
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
  lookbackTotal = cdl3blackcrowsLookbackPositional(core);
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
  i = ShadowVeryShortTrailingIdx;
  while (i < startIdx) {
    ShadowVeryShortPeriodTotal[2] = ShadowVeryShortPeriodTotal[2] + ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
    ShadowVeryShortPeriodTotal[1] = ShadowVeryShortPeriodTotal[1] + ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    ShadowVeryShortPeriodTotal[0] = ShadowVeryShortPeriodTotal[0] + ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  range2 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
  range1 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
  trailRange2 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx - 2] - inOpen[ShadowVeryShortTrailingIdx - 2])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[ShadowVeryShortTrailingIdx - 2] - inLow[ShadowVeryShortTrailingIdx - 2]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[ShadowVeryShortTrailingIdx - 2] - (((inClose[ShadowVeryShortTrailingIdx - 2]) >= (inOpen[ShadowVeryShortTrailingIdx - 2])) ? (inClose[ShadowVeryShortTrailingIdx - 2]) : (inOpen[ShadowVeryShortTrailingIdx - 2]))) + ((((inClose[ShadowVeryShortTrailingIdx - 2]) >= (inOpen[ShadowVeryShortTrailingIdx - 2])) ? (inOpen[ShadowVeryShortTrailingIdx - 2]) : (inClose[ShadowVeryShortTrailingIdx - 2])) - inLow[ShadowVeryShortTrailingIdx - 2])) : 0.0)));
  trailRange1 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx - 1] - inOpen[ShadowVeryShortTrailingIdx - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[ShadowVeryShortTrailingIdx - 1] - inLow[ShadowVeryShortTrailingIdx - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[ShadowVeryShortTrailingIdx - 1] - (((inClose[ShadowVeryShortTrailingIdx - 1]) >= (inOpen[ShadowVeryShortTrailingIdx - 1])) ? (inClose[ShadowVeryShortTrailingIdx - 1]) : (inOpen[ShadowVeryShortTrailingIdx - 1]))) + ((((inClose[ShadowVeryShortTrailingIdx - 1]) >= (inOpen[ShadowVeryShortTrailingIdx - 1])) ? (inOpen[ShadowVeryShortTrailingIdx - 1]) : (inClose[ShadowVeryShortTrailingIdx - 1])) - inLow[ShadowVeryShortTrailingIdx - 1])) : 0.0)));
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - three consecutive and declining black candlesticks
   * - each candle must have no or very short lower shadow
   * - each candle after the first must open within the prior candle's real body
   * - the first candle's close should be under the prior white candle's high
   * The meaning of "very short" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (((inClose[i - 3] >= inOpen[i - 3]) ? 1 : -1) === 1 &&  /* white */
        ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === -1 && /* 1st black */
        ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === -1 && /* 2nd black */
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === -1 &&         /* 3rd black */
        inOpen[i - 1] < inOpen[i - 2] &&
        inOpen[i - 1] > inClose[i - 2] &&                      /* 2nd black opens within 1st black's rb */
        inOpen[i] < inOpen[i - 1] &&
        inOpen[i] > inClose[i - 1] &&                          /* 3rd black opens within 2nd black's rb */
        inHigh[i - 3] > inClose[i - 2] &&                      /* 1st black closes under prior candle's high */
        inClose[i - 2] > inClose[i - 1] &&                     /* three declining */
        inClose[i - 1] > inClose[i] &&                         /* three declining */
        (((inClose[i - 2] >= inOpen[i - 2]) ? inOpen[i - 2] : inClose[i - 2]) - inLow[i - 2]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
        (((inClose[i - 1] >= inOpen[i - 1]) ? inOpen[i - 1] : inClose[i - 1]) - inLow[i - 1]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (ShadowVeryShortPeriodTotal[1] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
        (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (ShadowVeryShortPeriodTotal[0] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0))))) /* very short lower shadow */
    {
      outInteger[outIdx++] = -100;
    } else {
      outInteger[outIdx++] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    /* A bar's ranges for i-1 and i-2 are the ones computed one and two bars ago. */
    range0 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    trailRange0 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx] - inOpen[ShadowVeryShortTrailingIdx])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[ShadowVeryShortTrailingIdx] - inLow[ShadowVeryShortTrailingIdx]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[ShadowVeryShortTrailingIdx] - (((inClose[ShadowVeryShortTrailingIdx]) >= (inOpen[ShadowVeryShortTrailingIdx])) ? (inClose[ShadowVeryShortTrailingIdx]) : (inOpen[ShadowVeryShortTrailingIdx]))) + ((((inClose[ShadowVeryShortTrailingIdx]) >= (inOpen[ShadowVeryShortTrailingIdx])) ? (inOpen[ShadowVeryShortTrailingIdx]) : (inClose[ShadowVeryShortTrailingIdx])) - inLow[ShadowVeryShortTrailingIdx])) : 0.0)));
    ShadowVeryShortPeriodTotal[2] = ShadowVeryShortPeriodTotal[2] + (range2 - trailRange2);
    ShadowVeryShortPeriodTotal[1] = ShadowVeryShortPeriodTotal[1] + (range1 - trailRange1);
    ShadowVeryShortPeriodTotal[0] = ShadowVeryShortPeriodTotal[0] + (range0 - trailRange0);
    range2 = range1;
    range1 = range0;
    trailRange2 = trailRange1;
    trailRange1 = trailRange0;
    i += 1;
    ShadowVeryShortTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function cdl3blackcrowsPositional(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDL3BLACKCROWS', 'startIdx', startIdx);
  endIdx = requireInt32('CDL3BLACKCROWS', 'endIdx', endIdx);
  requireIndexRange('CDL3BLACKCROWS', startIdx, endIdx);
  const guardStart = clampedStart('CDL3BLACKCROWS', startIdx, cdl3blackcrowsLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDL3BLACKCROWS', 'prices.open', inOpen, guardInLen);
  requireLength('CDL3BLACKCROWS', 'prices.high', inHigh, guardInLen);
  requireLength('CDL3BLACKCROWS', 'prices.low', inLow, guardInLen);
  requireLength('CDL3BLACKCROWS', 'prices.close', inClose, guardInLen);
  requireOutput('CDL3BLACKCROWS', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDL3BLACKCROWS', 'outInteger', 'prices.open');
  }
  if (overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDL3BLACKCROWS', 'outInteger', 'prices.high');
  }
  if (overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDL3BLACKCROWS', 'outInteger', 'prices.low');
  }
  if (overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDL3BLACKCROWS', 'outInteger', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdl3blackcrowsImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDL3BLACKCROWS', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cdl3blackcrows}, {@link cdl3blackcrowsLookback},
 * {@link cdl3blackcrowsDisplayShift}, {@link cdl3blackcrowsOpen} and
 * {@link cdl3blackcrowsOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface Cdl3blackcrowsOptions {
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
 * What {@link cdl3blackcrows} returns, and {@link cdl3blackcrowsOpenAndFill} with
 * its stream.
 */
export interface Cdl3blackcrowsResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * -100 when the bearish pattern is detected, 0 otherwise. Never emits +100.
   * `values[k]` is the value at input bar `begIdx + k`.
   */
  values: Int32Array;
}

/**
 * Number of leading input bars {@link cdl3blackcrows} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link Cdl3blackcrowsOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdl3blackcrowsLookback(options?: Cdl3blackcrowsOptions): number {
  const o = optionsOf('CDL3BLACKCROWS lookback', options);
  return cdl3blackcrowsLookbackPositional(coreOf('CDL3BLACKCROWS lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdl3blackcrows}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link Cdl3blackcrowsOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdl3blackcrowsDisplayShift(outputIdx: number, options?: Cdl3blackcrowsOptions): number {
  const o = optionsOf('CDL3BLACKCROWS displayShift', options);
  return cdl3blackcrowsDisplayShiftPositional(outputIdx, coreOf('CDL3BLACKCROWS displayShift', o.core));
}

/**
 * A four-bar pattern: a white candle followed by three consecutive black (down)
 * candles with successively lower closes, each opening inside the prior black's
 * real body. It is a bearish reversal signal. A hit (-100) signals a bearish
 * reversal.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdl3blackcrows](https://ta-lib.org/functions/cdl3blackcrows).
 *
 * **Notes**
 * - Does not verify the prior mature uptrend the pattern classically assumes for
 *   significance.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cdl3blackcrowsLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `open`: Open
 *   price of each bar. `high`: High price of each bar. `low`: Low price of each bar.
 *   `close`: Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link Cdl3blackcrowsOptions}.
 * @returns `begIdx` and the values; see {@link Cdl3blackcrowsResult}. Each output
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
 * import { cdl3blackcrows } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { begIdx, values } = cdl3blackcrows({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`cdl3whitesoldiers`](https://ta-lib.org/functions/cdl3whitesoldiers)
 * @see [`cdlidentical3crows`](https://ta-lib.org/functions/cdlidentical3crows)
 * @see [`cdladvanceblock`](https://ta-lib.org/functions/cdladvanceblock)
 */
export function cdl3blackcrows(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: Cdl3blackcrowsOptions): Cdl3blackcrowsResult {
  requireArgument('CDL3BLACKCROWS', 'prices', prices);
  const o = optionsOf('CDL3BLACKCROWS', options);
  if (o.endIdx === undefined) {
    requireSeries('CDL3BLACKCROWS', 'prices.open', prices.open);
    requireSeries('CDL3BLACKCROWS', 'prices.high', prices.high);
    requireSeries('CDL3BLACKCROWS', 'prices.low', prices.low);
    requireSeries('CDL3BLACKCROWS', 'prices.close', prices.close);
  }
  const core = coreOf('CDL3BLACKCROWS', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cdl3blackcrowsLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const range = cdl3blackcrowsPositional(startIdx, endIdx, prices.open, prices.high, prices.low, prices.close, outInteger, core);
  return { begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CDL3BLACKCROWS stream: one value per closed bar, bit-identical to
 * {@link cdl3blackcrows} over the same series. Open it with
 * {@link cdl3blackcrowsOpen} or {@link cdl3blackcrowsOpenAndFill}; there is
 * nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdl3blackcrows](https://ta-lib.org/functions/cdl3blackcrows).
 */
export class Cdl3blackcrowsStream {
  /** @internal */
  core: Core;
  /** @internal */
  ShadowVeryShortPeriodTotal: Float64Array = new Float64Array(3);
  /** @internal */
  range1: number = 0;
  /** @internal */
  range2: number = 0;
  /** @internal */
  trailRange1: number = 0;
  /** @internal */
  trailRange2: number = 0;
  /** @internal */
  lag1_inOpen: number = 0;
  /** @internal */
  lag2_inOpen: number = 0;
  /** @internal */
  lag3_inOpen: number = 0;
  /** @internal */
  lag1_inHigh: number = 0;
  /** @internal */
  lag2_inHigh: number = 0;
  /** @internal */
  lag3_inHigh: number = 0;
  /** @internal */
  lag1_inLow: number = 0;
  /** @internal */
  lag2_inLow: number = 0;
  /** @internal */
  lag1_inClose: number = 0;
  /** @internal */
  lag2_inClose: number = 0;
  /** @internal */
  lag3_inClose: number = 0;
  /** @internal */
  ringPos_ShadowVeryShortTrailingIdx: number = 0;
  /** @internal */
  ringCap_ShadowVeryShortTrailingIdx: number = 0;
  /** @internal */
  ring_ShadowVeryShortTrailingIdx_derived: Float64Array = new Float64Array(1);
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
  static of(core: Core): Cdl3blackcrowsStream {
    return new Cdl3blackcrowsStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cdl3blackcrows} reports them over the same bars. Each accepted `update`
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
      throw streamFailure('CDL3BLACKCROWS advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw streamFailure('CDL3BLACKCROWS update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(open) || !Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('CDL3BLACKCROWS update', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    cdl3blackcrowsStepImpl(this, open, high, low, close);
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
      throw nonFiniteBar('CDL3BLACKCROWS peek', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inOpen = open;
    const inHigh = high;
    const inLow = low;
    const inClose = close;
    let cur_outInteger = 0;
    const ShadowVeryShort_rangeType = sp.cs_ShadowVeryShort_rangeType;
    const ShadowVeryShort_avgPeriod = sp.cs_ShadowVeryShort_avgPeriod;
    const ShadowVeryShort_factor = sp.cs_ShadowVeryShort_factor;
    if (((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : -1) === 1 &&  /* white */
        ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === -1 && /* 1st black */
        ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === -1 && /* 2nd black */
        ((inClose >= inOpen) ? 1 : -1) === -1 &&                 /* 3rd black */
        sp.lag1_inOpen < sp.lag2_inOpen &&
        sp.lag1_inOpen > sp.lag2_inClose &&                      /* 2nd black opens within 1st black's rb */
        inOpen < sp.lag1_inOpen &&
        inOpen > sp.lag1_inClose &&                              /* 3rd black opens within 2nd black's rb */
        sp.lag3_inHigh > sp.lag2_inClose &&                      /* 1st black closes under prior candle's high */
        sp.lag2_inClose > sp.lag1_inClose &&                     /* three declining */
        sp.lag1_inClose > inClose &&                             /* three declining */
        (((sp.lag2_inClose >= sp.lag2_inOpen) ? sp.lag2_inOpen : sp.lag2_inClose) - sp.lag2_inLow) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (sp.ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((ShadowVeryShort_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
        (((sp.lag1_inClose >= sp.lag1_inOpen) ? sp.lag1_inOpen : sp.lag1_inClose) - sp.lag1_inLow) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (sp.ShadowVeryShortPeriodTotal[1] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((ShadowVeryShort_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
        (((inClose >= inOpen) ? inOpen : inClose) - inLow) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (sp.ShadowVeryShortPeriodTotal[0] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0))))) /* very short lower shadow */
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
  clone(): Cdl3blackcrowsStream {
    const c = new Cdl3blackcrowsStream(this.core);
    c.ShadowVeryShortPeriodTotal = this.ShadowVeryShortPeriodTotal.slice();
    c.range1 = this.range1;
    c.range2 = this.range2;
    c.trailRange1 = this.trailRange1;
    c.trailRange2 = this.trailRange2;
    c.lag1_inOpen = this.lag1_inOpen;
    c.lag2_inOpen = this.lag2_inOpen;
    c.lag3_inOpen = this.lag3_inOpen;
    c.lag1_inHigh = this.lag1_inHigh;
    c.lag2_inHigh = this.lag2_inHigh;
    c.lag3_inHigh = this.lag3_inHigh;
    c.lag1_inLow = this.lag1_inLow;
    c.lag2_inLow = this.lag2_inLow;
    c.lag1_inClose = this.lag1_inClose;
    c.lag2_inClose = this.lag2_inClose;
    c.lag3_inClose = this.lag3_inClose;
    c.ringPos_ShadowVeryShortTrailingIdx = this.ringPos_ShadowVeryShortTrailingIdx;
    c.ringCap_ShadowVeryShortTrailingIdx = this.ringCap_ShadowVeryShortTrailingIdx;
    c.ring_ShadowVeryShortTrailingIdx_derived = this.ring_ShadowVeryShortTrailingIdx_derived.slice();
    c.cs_ShadowVeryShort_rangeType = this.cs_ShadowVeryShort_rangeType;
    c.cs_ShadowVeryShort_avgPeriod = this.cs_ShadowVeryShort_avgPeriod;
    c.cs_ShadowVeryShort_factor = this.cs_ShadowVeryShort_factor;
    c.cur_outInteger = this.cur_outInteger;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cdl3blackcrowsStepImpl(sp: Cdl3blackcrowsStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  let range0 = 0;
  let trailRange0 = 0;
  const ShadowVeryShort_rangeType = sp.cs_ShadowVeryShort_rangeType;
  const ShadowVeryShort_avgPeriod = sp.cs_ShadowVeryShort_avgPeriod;
  const ShadowVeryShort_factor = sp.cs_ShadowVeryShort_factor;
  if (sp.ringCap_ShadowVeryShortTrailingIdx === 0) {
    sp.ring_ShadowVeryShortTrailingIdx_derived[0] = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
  if (((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : -1) === 1 &&  /* white */
      ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === -1 && /* 1st black */
      ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === -1 && /* 2nd black */
      ((inClose >= inOpen) ? 1 : -1) === -1 &&                 /* 3rd black */
      sp.lag1_inOpen < sp.lag2_inOpen &&
      sp.lag1_inOpen > sp.lag2_inClose &&                      /* 2nd black opens within 1st black's rb */
      inOpen < sp.lag1_inOpen &&
      inOpen > sp.lag1_inClose &&                              /* 3rd black opens within 2nd black's rb */
      sp.lag3_inHigh > sp.lag2_inClose &&                      /* 1st black closes under prior candle's high */
      sp.lag2_inClose > sp.lag1_inClose &&                     /* three declining */
      sp.lag1_inClose > inClose &&                             /* three declining */
      (((sp.lag2_inClose >= sp.lag2_inOpen) ? sp.lag2_inOpen : sp.lag2_inClose) - sp.lag2_inLow) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (sp.ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((ShadowVeryShort_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
      (((sp.lag1_inClose >= sp.lag1_inOpen) ? sp.lag1_inOpen : sp.lag1_inClose) - sp.lag1_inLow) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (sp.ShadowVeryShortPeriodTotal[1] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((ShadowVeryShort_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
      (((inClose >= inOpen) ? inOpen : inClose) - inLow) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (sp.ShadowVeryShortPeriodTotal[0] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0))))) /* very short lower shadow */
  {
    sp.cur_outInteger = -100;
  } else {
    sp.cur_outInteger = 0;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  /* A bar's ranges for i-1 and i-2 are the ones computed one and two bars ago. */
  range0 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  trailRange0 = sp.ring_ShadowVeryShortTrailingIdx_derived[sp.ringPos_ShadowVeryShortTrailingIdx];
  sp.ShadowVeryShortPeriodTotal[2] = sp.ShadowVeryShortPeriodTotal[2] + (sp.range2 - sp.trailRange2);
  sp.ShadowVeryShortPeriodTotal[1] = sp.ShadowVeryShortPeriodTotal[1] + (sp.range1 - sp.trailRange1);
  sp.ShadowVeryShortPeriodTotal[0] = sp.ShadowVeryShortPeriodTotal[0] + (range0 - trailRange0);
  sp.range2 = sp.range1;
  sp.range1 = range0;
  sp.trailRange2 = sp.trailRange1;
  sp.trailRange1 = trailRange0;
  sp.lag3_inOpen = sp.lag2_inOpen;
  sp.lag2_inOpen = sp.lag1_inOpen;
  sp.lag1_inOpen = inOpen;
  sp.lag3_inHigh = sp.lag2_inHigh;
  sp.lag2_inHigh = sp.lag1_inHigh;
  sp.lag1_inHigh = inHigh;
  sp.lag2_inLow = sp.lag1_inLow;
  sp.lag1_inLow = inLow;
  sp.lag3_inClose = sp.lag2_inClose;
  sp.lag2_inClose = sp.lag1_inClose;
  sp.lag1_inClose = inClose;
  sp.ring_ShadowVeryShortTrailingIdx_derived[sp.ringPos_ShadowVeryShortTrailingIdx] = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  sp.ringPos_ShadowVeryShortTrailingIdx = sp.ringPos_ShadowVeryShortTrailingIdx + 1;
  if (sp.ringPos_ShadowVeryShortTrailingIdx >= sp.ringCap_ShadowVeryShortTrailingIdx) {
    sp.ringPos_ShadowVeryShortTrailingIdx = 0;
  }
}

function cdl3blackcrowsOpenImpl(
  sp: Cdl3blackcrowsStream,
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
  let range1 = 0;
  let range2 = 0;
  let trailRange1 = 0;
  let trailRange2 = 0;
  let range0 = 0;
  let trailRange0 = 0;
  let i = 0;
  let outIdx = 0;
  let ShadowVeryShortTrailingIdx = 0;
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
  const ShadowVeryShort_rangeType: number = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].rangeType;
  const ShadowVeryShort_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].avgPeriod;
  const ShadowVeryShort_factor = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].factor;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdl3blackcrowsLookbackPositional(core);
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
  i = ShadowVeryShortTrailingIdx;
  while (i < startIdx) {
    ShadowVeryShortPeriodTotal[2] = ShadowVeryShortPeriodTotal[2] + ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
    ShadowVeryShortPeriodTotal[1] = ShadowVeryShortPeriodTotal[1] + ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    ShadowVeryShortPeriodTotal[0] = ShadowVeryShortPeriodTotal[0] + ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  range2 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
  range1 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
  trailRange2 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx - 2] - inOpen[ShadowVeryShortTrailingIdx - 2])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[ShadowVeryShortTrailingIdx - 2] - inLow[ShadowVeryShortTrailingIdx - 2]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[ShadowVeryShortTrailingIdx - 2] - (((inClose[ShadowVeryShortTrailingIdx - 2]) >= (inOpen[ShadowVeryShortTrailingIdx - 2])) ? (inClose[ShadowVeryShortTrailingIdx - 2]) : (inOpen[ShadowVeryShortTrailingIdx - 2]))) + ((((inClose[ShadowVeryShortTrailingIdx - 2]) >= (inOpen[ShadowVeryShortTrailingIdx - 2])) ? (inOpen[ShadowVeryShortTrailingIdx - 2]) : (inClose[ShadowVeryShortTrailingIdx - 2])) - inLow[ShadowVeryShortTrailingIdx - 2])) : 0.0)));
  trailRange1 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx - 1] - inOpen[ShadowVeryShortTrailingIdx - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[ShadowVeryShortTrailingIdx - 1] - inLow[ShadowVeryShortTrailingIdx - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[ShadowVeryShortTrailingIdx - 1] - (((inClose[ShadowVeryShortTrailingIdx - 1]) >= (inOpen[ShadowVeryShortTrailingIdx - 1])) ? (inClose[ShadowVeryShortTrailingIdx - 1]) : (inOpen[ShadowVeryShortTrailingIdx - 1]))) + ((((inClose[ShadowVeryShortTrailingIdx - 1]) >= (inOpen[ShadowVeryShortTrailingIdx - 1])) ? (inOpen[ShadowVeryShortTrailingIdx - 1]) : (inClose[ShadowVeryShortTrailingIdx - 1])) - inLow[ShadowVeryShortTrailingIdx - 1])) : 0.0)));
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - three consecutive and declining black candlesticks
   * - each candle must have no or very short lower shadow
   * - each candle after the first must open within the prior candle's real body
   * - the first candle's close should be under the prior white candle's high
   * The meaning of "very short" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (((inClose[i - 3] >= inOpen[i - 3]) ? 1 : -1) === 1 &&  /* white */
        ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === -1 && /* 1st black */
        ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === -1 && /* 2nd black */
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === -1 &&         /* 3rd black */
        inOpen[i - 1] < inOpen[i - 2] &&
        inOpen[i - 1] > inClose[i - 2] &&                      /* 2nd black opens within 1st black's rb */
        inOpen[i] < inOpen[i - 1] &&
        inOpen[i] > inClose[i - 1] &&                          /* 3rd black opens within 2nd black's rb */
        inHigh[i - 3] > inClose[i - 2] &&                      /* 1st black closes under prior candle's high */
        inClose[i - 2] > inClose[i - 1] &&                     /* three declining */
        inClose[i - 1] > inClose[i] &&                         /* three declining */
        (((inClose[i - 2] >= inOpen[i - 2]) ? inOpen[i - 2] : inClose[i - 2]) - inLow[i - 2]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
        (((inClose[i - 1] >= inOpen[i - 1]) ? inOpen[i - 1] : inClose[i - 1]) - inLow[i - 1]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (ShadowVeryShortPeriodTotal[1] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
        (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (ShadowVeryShortPeriodTotal[0] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0))))) /* very short lower shadow */
    {
      outInteger[((outIdx++ * outStride) | 0)] = -100;
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    /* A bar's ranges for i-1 and i-2 are the ones computed one and two bars ago. */
    range0 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    trailRange0 = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx] - inOpen[ShadowVeryShortTrailingIdx])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[ShadowVeryShortTrailingIdx] - inLow[ShadowVeryShortTrailingIdx]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[ShadowVeryShortTrailingIdx] - (((inClose[ShadowVeryShortTrailingIdx]) >= (inOpen[ShadowVeryShortTrailingIdx])) ? (inClose[ShadowVeryShortTrailingIdx]) : (inOpen[ShadowVeryShortTrailingIdx]))) + ((((inClose[ShadowVeryShortTrailingIdx]) >= (inOpen[ShadowVeryShortTrailingIdx])) ? (inOpen[ShadowVeryShortTrailingIdx]) : (inClose[ShadowVeryShortTrailingIdx])) - inLow[ShadowVeryShortTrailingIdx])) : 0.0)));
    ShadowVeryShortPeriodTotal[2] = ShadowVeryShortPeriodTotal[2] + (range2 - trailRange2);
    ShadowVeryShortPeriodTotal[1] = ShadowVeryShortPeriodTotal[1] + (range1 - trailRange1);
    ShadowVeryShortPeriodTotal[0] = ShadowVeryShortPeriodTotal[0] + (range0 - trailRange0);
    range2 = range1;
    range1 = range0;
    trailRange2 = trailRange1;
    trailRange1 = trailRange0;
    i += 1;
    ShadowVeryShortTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
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
  sp.range1 = range1;
  sp.range2 = range2;
  sp.trailRange1 = trailRange1;
  sp.trailRange2 = trailRange2;
  sp.lag1_inOpen = inOpen[historyLen - 1];
  sp.lag2_inOpen = inOpen[historyLen - 2];
  sp.lag3_inOpen = inOpen[historyLen - 3];
  sp.lag1_inHigh = inHigh[historyLen - 1];
  sp.lag2_inHigh = inHigh[historyLen - 2];
  sp.lag3_inHigh = inHigh[historyLen - 3];
  sp.lag1_inLow = inLow[historyLen - 1];
  sp.lag2_inLow = inLow[historyLen - 2];
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.lag2_inClose = inClose[historyLen - 2];
  sp.lag3_inClose = inClose[historyLen - 3];
  sp.ringPos_ShadowVeryShortTrailingIdx = 0;
  sp.ringCap_ShadowVeryShortTrailingIdx = cap_ShadowVeryShortTrailingIdx;
  sp.ring_ShadowVeryShortTrailingIdx_derived = capRing_ShadowVeryShortTrailingIdx_derived;
  sp.cs_ShadowVeryShort_rangeType = ShadowVeryShort_rangeType;
  sp.cs_ShadowVeryShort_avgPeriod = ShadowVeryShort_avgPeriod;
  sp.cs_ShadowVeryShort_factor = ShadowVeryShort_factor;
  sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cdl3blackcrowsOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): Cdl3blackcrowsStream {
  const sp = Cdl3blackcrowsStream.of(core);
  const retCode = cdl3blackcrowsOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDL3BLACKCROWS openAndFill', inOpen.length, startIdx, cdl3blackcrowsLookbackPositional(core));
  }
  throw streamFailure('CDL3BLACKCROWS openAndFill', retCode);
}

/** @internal */
export function cdl3blackcrowsOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): Cdl3blackcrowsStream {
  const sp = Cdl3blackcrowsStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdl3blackcrowsOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDL3BLACKCROWS open', inOpen.length, startIdx, cdl3blackcrowsLookbackPositional(core));
  }
  throw streamFailure('CDL3BLACKCROWS open', retCode);
}

/** @internal */
export function cdl3blackcrowsOpenPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): Cdl3blackcrowsStream {
  requireArgument('CDL3BLACKCROWS open', 'prices.open', inOpen);
  requireHistory('CDL3BLACKCROWS open', 'prices.open', inOpen.length);
  requireArgument('CDL3BLACKCROWS open', 'prices.high', inHigh);
  requireArgument('CDL3BLACKCROWS open', 'prices.low', inLow);
  requireArgument('CDL3BLACKCROWS open', 'prices.close', inClose);
  requireHistoryLength('CDL3BLACKCROWS open', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDL3BLACKCROWS open', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDL3BLACKCROWS open', 'prices.close', inClose.length, inOpen.length);
  return cdl3blackcrowsOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/** @internal */
export function cdl3blackcrowsOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): Cdl3blackcrowsStream {
  requireArgument('CDL3BLACKCROWS openAndFill', 'prices.open', inOpen);
  requireHistory('CDL3BLACKCROWS openAndFill', 'prices.open', inOpen.length);
  requireArgument('CDL3BLACKCROWS openAndFill', 'prices.high', inHigh);
  requireArgument('CDL3BLACKCROWS openAndFill', 'prices.low', inLow);
  requireArgument('CDL3BLACKCROWS openAndFill', 'prices.close', inClose);
  const guardOutLen = openFillCount('CDL3BLACKCROWS openAndFill', inOpen.length, cdl3blackcrowsLookbackPositional(core));
  requireHistoryLength('CDL3BLACKCROWS openAndFill', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDL3BLACKCROWS openAndFill', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDL3BLACKCROWS openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('CDL3BLACKCROWS openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('CDL3BLACKCROWS openAndFill', 'outInteger', 'prices.open');
  }
  if (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CDL3BLACKCROWS openAndFill', 'outInteger', 'prices.high');
  }
  if (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CDL3BLACKCROWS openAndFill', 'outInteger', 'prices.low');
  }
  if (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('CDL3BLACKCROWS openAndFill', 'outInteger', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDL3BLACKCROWS openAndFill', 'outInteger', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDL3BLACKCROWS openAndFill', 'outInteger', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDL3BLACKCROWS openAndFill', 'outInteger', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDL3BLACKCROWS openAndFill', 'outInteger', 'prices.close');
  }
  return cdl3blackcrowsOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}

/**
 * Opens a live CDL3BLACKCROWS stream over the warm-up history: `value()` starts at
 * the last history bar, bit-identical to {@link cdl3blackcrows} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `open`: Open price of each bar. `high`: High price of each bar. `low`:
 *   Low price of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link Cdl3blackcrowsOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdl3blackcrowsLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input or histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdl3blackcrowsOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdl3blackcrowsOpen({ open, high, low, close });
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdl3blackcrowsOpen(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: Cdl3blackcrowsOptions): Cdl3blackcrowsStream {
  requireArgument('CDL3BLACKCROWS open', 'prices', prices);
  const o = optionsOf('CDL3BLACKCROWS open', options);
  const core = coreOf('CDL3BLACKCROWS open', o.core);
  return cdl3blackcrowsOpenPositional(prices.open, prices.high, prices.low, prices.close, core);
}

/**
 * {@link cdl3blackcrowsOpen} that also computes the outputs over the whole history
 * in the same pass, bit-identically to {@link cdl3blackcrows}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `open`: Open price of each bar. `high`: High price of each bar. `low`:
 *   Low price of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link Cdl3blackcrowsOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cdl3blackcrows} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdl3blackcrowsLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, histories of different lengths or a buffer too short,
 *   of the wrong type or starting where another buffer or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdl3blackcrowsOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = cdl3blackcrowsOpenAndFill({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cdl3blackcrowsOpenAndFill(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: Cdl3blackcrowsOptions): Cdl3blackcrowsResult & { stream: Cdl3blackcrowsStream } {
  requireArgument('CDL3BLACKCROWS openAndFill', 'prices', prices);
  const o = optionsOf('CDL3BLACKCROWS openAndFill', options);
  const core = coreOf('CDL3BLACKCROWS openAndFill', o.core);
  const capacity = fillCapacity(cdl3blackcrowsLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const stream = cdl3blackcrowsOpenAndFillPositional(prices.open, prices.high, prices.low, prices.close, outInteger, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}
