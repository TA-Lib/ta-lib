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
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { CandleSettingType, Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function cdlinvertedhammerLookbackPositional(core: Core = Core.DEFAULT): number {
  const BodyShort_avgPeriod = core.candleSettings[CandleSettingType.BODY_SHORT].avgPeriod;
  const ShadowLong_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_LONG].avgPeriod;
  const ShadowVeryShort_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].avgPeriod;
  return ((BodyShort_avgPeriod > ShadowLong_avgPeriod ? BodyShort_avgPeriod : ShadowLong_avgPeriod) > ShadowVeryShort_avgPeriod ? (BodyShort_avgPeriod > ShadowLong_avgPeriod ? BodyShort_avgPeriod : ShadowLong_avgPeriod) : ShadowVeryShort_avgPeriod) + 1;
}

/** @internal */
export function cdlinvertedhammerDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdlinvertedhammerImpl(
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
  let BodyPeriodTotal = 0;
  let ShadowLongPeriodTotal = 0;
  let ShadowVeryShortPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let BodyTrailingIdx = 0;
  let ShadowLongTrailingIdx = 0;
  let ShadowVeryShortTrailingIdx = 0;
  let lookbackTotal = 0;
  const BodyShort_rangeType: number = core.candleSettings[CandleSettingType.BODY_SHORT].rangeType;
  const BodyShort_avgPeriod = core.candleSettings[CandleSettingType.BODY_SHORT].avgPeriod;
  const BodyShort_factor = core.candleSettings[CandleSettingType.BODY_SHORT].factor;
  const ShadowLong_rangeType: number = core.candleSettings[CandleSettingType.SHADOW_LONG].rangeType;
  const ShadowLong_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_LONG].avgPeriod;
  const ShadowLong_factor = core.candleSettings[CandleSettingType.SHADOW_LONG].factor;
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
  lookbackTotal = cdlinvertedhammerLookbackPositional(core);
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
  BodyPeriodTotal = 0;
  BodyTrailingIdx = startIdx - BodyShort_avgPeriod;
  ShadowLongPeriodTotal = 0;
  ShadowLongTrailingIdx = startIdx - ShadowLong_avgPeriod;
  ShadowVeryShortPeriodTotal = 0;
  ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
  i = BodyTrailingIdx;
  while (i < startIdx) {
    BodyPeriodTotal += ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = ShadowLongTrailingIdx;
  while (i < startIdx) {
    ShadowLongPeriodTotal += ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = ShadowVeryShortTrailingIdx;
  while (i < startIdx) {
    ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - small real body
   * - long upper shadow
   * - no, or very short, lower shadow
   * - gap down
   * The meaning of "short", "very short" and "long" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (((inOpen[i] > inClose[i] ? inOpen[i] : inClose[i]) < (inOpen[i - 1] < inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1])) && /* gap down */
        Math.abs(inClose[i] - inOpen[i]) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (BodyPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) && /* small rb */
        (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowLong_factor * (((ShadowLong_avgPeriod !== 0) ? (ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowLong_rangeType === 2) ? 2.0 : 1.0)))) && /* long upper shadow */
        (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0))))) /* very short lower shadow */
    {
      outInteger[outIdx++] = 100;
    } else {
      outInteger[outIdx++] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyPeriodTotal += ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyShort_rangeType === 0) ? (Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : ((BodyShort_rangeType === 1) ? (inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) : ((BodyShort_rangeType === 2) ? ((inHigh[BodyTrailingIdx] - (((inClose[BodyTrailingIdx]) >= (inOpen[BodyTrailingIdx])) ? (inClose[BodyTrailingIdx]) : (inOpen[BodyTrailingIdx]))) + ((((inClose[BodyTrailingIdx]) >= (inOpen[BodyTrailingIdx])) ? (inOpen[BodyTrailingIdx]) : (inClose[BodyTrailingIdx])) - inLow[BodyTrailingIdx])) : 0.0)));
    ShadowLongPeriodTotal += ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[ShadowLongTrailingIdx] - inOpen[ShadowLongTrailingIdx])) : ((ShadowLong_rangeType === 1) ? (inHigh[ShadowLongTrailingIdx] - inLow[ShadowLongTrailingIdx]) : ((ShadowLong_rangeType === 2) ? ((inHigh[ShadowLongTrailingIdx] - (((inClose[ShadowLongTrailingIdx]) >= (inOpen[ShadowLongTrailingIdx])) ? (inClose[ShadowLongTrailingIdx]) : (inOpen[ShadowLongTrailingIdx]))) + ((((inClose[ShadowLongTrailingIdx]) >= (inOpen[ShadowLongTrailingIdx])) ? (inOpen[ShadowLongTrailingIdx]) : (inClose[ShadowLongTrailingIdx])) - inLow[ShadowLongTrailingIdx])) : 0.0)));
    ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx] - inOpen[ShadowVeryShortTrailingIdx])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[ShadowVeryShortTrailingIdx] - inLow[ShadowVeryShortTrailingIdx]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[ShadowVeryShortTrailingIdx] - (((inClose[ShadowVeryShortTrailingIdx]) >= (inOpen[ShadowVeryShortTrailingIdx])) ? (inClose[ShadowVeryShortTrailingIdx]) : (inOpen[ShadowVeryShortTrailingIdx]))) + ((((inClose[ShadowVeryShortTrailingIdx]) >= (inOpen[ShadowVeryShortTrailingIdx])) ? (inOpen[ShadowVeryShortTrailingIdx]) : (inClose[ShadowVeryShortTrailingIdx])) - inLow[ShadowVeryShortTrailingIdx])) : 0.0)));
    i += 1;
    BodyTrailingIdx += 1;
    ShadowLongTrailingIdx += 1;
    ShadowVeryShortTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlinvertedhammerPositional(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDLINVERTEDHAMMER', 'startIdx', startIdx);
  endIdx = requireInt32('CDLINVERTEDHAMMER', 'endIdx', endIdx);
  requireIndexRange('CDLINVERTEDHAMMER', startIdx, endIdx);
  const guardStart = clampedStart('CDLINVERTEDHAMMER', startIdx, cdlinvertedhammerLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLINVERTEDHAMMER', 'prices.open', inOpen, guardInLen);
  requireLength('CDLINVERTEDHAMMER', 'prices.high', inHigh, guardInLen);
  requireLength('CDLINVERTEDHAMMER', 'prices.low', inLow, guardInLen);
  requireLength('CDLINVERTEDHAMMER', 'prices.close', inClose, guardInLen);
  requireOutput('CDLINVERTEDHAMMER', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLINVERTEDHAMMER', 'outInteger', 'prices.open');
  }
  if (overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLINVERTEDHAMMER', 'outInteger', 'prices.high');
  }
  if (overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLINVERTEDHAMMER', 'outInteger', 'prices.low');
  }
  if (overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLINVERTEDHAMMER', 'outInteger', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdlinvertedhammerImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLINVERTEDHAMMER', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cdlinvertedhammer}, {@link cdlinvertedhammerLookback},
 * {@link cdlinvertedhammerDisplayShift}, {@link cdlinvertedhammerOpen} and
 * {@link cdlinvertedhammerOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface CdlinvertedhammerOptions {
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
 * What {@link cdlinvertedhammer} returns, and {@link cdlinvertedhammerOpenAndFill}
 * with its stream.
 */
export interface CdlinvertedhammerResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * +100 when the inverted hammer is detected, 0 otherwise. Never emits -100; the
   * pattern is always bullish. `values[k]` is the value at input bar `begIdx + k`.
   */
  values: Int32Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link cdlinvertedhammer} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlinvertedhammerOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlinvertedhammerLookback(options?: CdlinvertedhammerOptions): number {
  const o = optionsOf('CDLINVERTEDHAMMER lookback', options);
  return cdlinvertedhammerLookbackPositional(coreOf('CDLINVERTEDHAMMER lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdlinvertedhammer}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlinvertedhammerOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlinvertedhammerDisplayShift(outputIdx: number, options?: CdlinvertedhammerOptions): number {
  const o = optionsOf('CDLINVERTEDHAMMER displayShift', options);
  return cdlinvertedhammerDisplayShiftPositional(outputIdx, coreOf('CDLINVERTEDHAMMER displayShift', o.core));
}

/**
 * Single-candle pattern: a small real body with a long upper shadow and
 * little-to-no lower shadow that gaps down from the prior candle. A hit flags a
 * potential bullish reversal.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlinvertedhammer](https://ta-lib.org/functions/cdlinvertedhammer).
 *
 * **Notes**
 * - Does not verify the preceding downtrend that the pattern classically assumes;
 *   it only checks the gap down from the immediately preceding candle.
 * - Despite the bullish-reversal label, Bulkowski's testing found this actually
 *   behaves as a bearish continuation 65% of the time — yet its overall
 *   post-breakout performance rank (6th of 103) is among the best of all
 *   candlestick patterns he studied.
 *   ([thepatternsite.com](https://thepatternsite.com/HammerInv.html))
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cdlinvertedhammerLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `open`: Open price of
 *   each bar. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link CdlinvertedhammerOptions}.
 * @returns `begIdx` and the values; see {@link CdlinvertedhammerResult}. Each
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
 * import { cdlinvertedhammer } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { begIdx, values } = cdlinvertedhammer({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`cdlhammer`](https://ta-lib.org/functions/cdlhammer)
 * @see [`cdlshootingstar`](https://ta-lib.org/functions/cdlshootingstar)
 * @see [`cdlhangingman`](https://ta-lib.org/functions/cdlhangingman)
 */
export function cdlinvertedhammer(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlinvertedhammerOptions): CdlinvertedhammerResult {
  requireArgument('CDLINVERTEDHAMMER', 'prices', prices);
  const o = optionsOf('CDLINVERTEDHAMMER', options);
  if (o.endIdx === undefined) {
    requireSeries('CDLINVERTEDHAMMER', 'prices.open', prices.open);
    requireSeries('CDLINVERTEDHAMMER', 'prices.high', prices.high);
    requireSeries('CDLINVERTEDHAMMER', 'prices.low', prices.low);
    requireSeries('CDLINVERTEDHAMMER', 'prices.close', prices.close);
  }
  const core = coreOf('CDLINVERTEDHAMMER', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cdlinvertedhammerLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const range = cdlinvertedhammerPositional(startIdx, endIdx, prices.open, prices.high, prices.low, prices.close, outInteger, core);
  return { begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CDLINVERTEDHAMMER stream: one value per closed bar, bit-identical to
 * {@link cdlinvertedhammer} over the same series. Open it with
 * {@link cdlinvertedhammerOpen} or {@link cdlinvertedhammerOpenAndFill}; there is
 * nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlinvertedhammer](https://ta-lib.org/functions/cdlinvertedhammer).
 */
export class CdlinvertedhammerStream {
  /** @internal */
  core: Core;
  /** @internal */
  BodyPeriodTotal: number = 0;
  /** @internal */
  ShadowLongPeriodTotal: number = 0;
  /** @internal */
  ShadowVeryShortPeriodTotal: number = 0;
  /** @internal */
  lag1_inOpen: number = 0;
  /** @internal */
  lag1_inClose: number = 0;
  /** @internal */
  ringPos_BodyTrailingIdx: number = 0;
  /** @internal */
  ringCap_BodyTrailingIdx: number = 0;
  /** @internal */
  ring_BodyTrailingIdx_derived: Float64Array = new Float64Array(1);
  /** @internal */
  ringPos_ShadowLongTrailingIdx: number = 0;
  /** @internal */
  ringCap_ShadowLongTrailingIdx: number = 0;
  /** @internal */
  ring_ShadowLongTrailingIdx_derived: Float64Array = new Float64Array(1);
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
  cs_ShadowLong_rangeType: number = 0;
  /** @internal */
  cs_ShadowLong_avgPeriod: number = 0;
  /** @internal */
  cs_ShadowLong_factor: number = 0;
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
  static of(core: Core): CdlinvertedhammerStream {
    return new CdlinvertedhammerStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cdlinvertedhammer} reports them over the same bars. Each accepted
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
      throw streamFailure('CDLINVERTEDHAMMER advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw streamFailure('CDLINVERTEDHAMMER update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(open) || !Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('CDLINVERTEDHAMMER update', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    cdlinvertedhammerStepImpl(this, open, high, low, close);
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
      throw nonFiniteBar('CDLINVERTEDHAMMER peek', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
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
    const ShadowLong_rangeType = sp.cs_ShadowLong_rangeType;
    const ShadowLong_avgPeriod = sp.cs_ShadowLong_avgPeriod;
    const ShadowLong_factor = sp.cs_ShadowLong_factor;
    const ShadowVeryShort_rangeType = sp.cs_ShadowVeryShort_rangeType;
    const ShadowVeryShort_avgPeriod = sp.cs_ShadowVeryShort_avgPeriod;
    const ShadowVeryShort_factor = sp.cs_ShadowVeryShort_factor;
    if (((inOpen > inClose ? inOpen : inClose) < (sp.lag1_inOpen < sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose)) && /* gap down */
        Math.abs(inClose - inOpen) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (sp.BodyPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) && /* small rb */
        (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) > ((ShadowLong_factor * (((ShadowLong_avgPeriod !== 0) ? (sp.ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowLong_rangeType === 2) ? 2.0 : 1.0)))) && /* long upper shadow */
        (((inClose >= inOpen) ? inOpen : inClose) - inLow) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (sp.ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0))))) /* very short lower shadow */
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
  clone(): CdlinvertedhammerStream {
    const c = new CdlinvertedhammerStream(this.core);
    c.BodyPeriodTotal = this.BodyPeriodTotal;
    c.ShadowLongPeriodTotal = this.ShadowLongPeriodTotal;
    c.ShadowVeryShortPeriodTotal = this.ShadowVeryShortPeriodTotal;
    c.lag1_inOpen = this.lag1_inOpen;
    c.lag1_inClose = this.lag1_inClose;
    c.ringPos_BodyTrailingIdx = this.ringPos_BodyTrailingIdx;
    c.ringCap_BodyTrailingIdx = this.ringCap_BodyTrailingIdx;
    c.ring_BodyTrailingIdx_derived = this.ring_BodyTrailingIdx_derived.slice();
    c.ringPos_ShadowLongTrailingIdx = this.ringPos_ShadowLongTrailingIdx;
    c.ringCap_ShadowLongTrailingIdx = this.ringCap_ShadowLongTrailingIdx;
    c.ring_ShadowLongTrailingIdx_derived = this.ring_ShadowLongTrailingIdx_derived.slice();
    c.ringPos_ShadowVeryShortTrailingIdx = this.ringPos_ShadowVeryShortTrailingIdx;
    c.ringCap_ShadowVeryShortTrailingIdx = this.ringCap_ShadowVeryShortTrailingIdx;
    c.ring_ShadowVeryShortTrailingIdx_derived = this.ring_ShadowVeryShortTrailingIdx_derived.slice();
    c.cs_BodyShort_rangeType = this.cs_BodyShort_rangeType;
    c.cs_BodyShort_avgPeriod = this.cs_BodyShort_avgPeriod;
    c.cs_BodyShort_factor = this.cs_BodyShort_factor;
    c.cs_ShadowLong_rangeType = this.cs_ShadowLong_rangeType;
    c.cs_ShadowLong_avgPeriod = this.cs_ShadowLong_avgPeriod;
    c.cs_ShadowLong_factor = this.cs_ShadowLong_factor;
    c.cs_ShadowVeryShort_rangeType = this.cs_ShadowVeryShort_rangeType;
    c.cs_ShadowVeryShort_avgPeriod = this.cs_ShadowVeryShort_avgPeriod;
    c.cs_ShadowVeryShort_factor = this.cs_ShadowVeryShort_factor;
    c.cur_outInteger = this.cur_outInteger;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cdlinvertedhammerStepImpl(sp: CdlinvertedhammerStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  const BodyShort_rangeType = sp.cs_BodyShort_rangeType;
  const BodyShort_avgPeriod = sp.cs_BodyShort_avgPeriod;
  const BodyShort_factor = sp.cs_BodyShort_factor;
  const ShadowLong_rangeType = sp.cs_ShadowLong_rangeType;
  const ShadowLong_avgPeriod = sp.cs_ShadowLong_avgPeriod;
  const ShadowLong_factor = sp.cs_ShadowLong_factor;
  const ShadowVeryShort_rangeType = sp.cs_ShadowVeryShort_rangeType;
  const ShadowVeryShort_avgPeriod = sp.cs_ShadowVeryShort_avgPeriod;
  const ShadowVeryShort_factor = sp.cs_ShadowVeryShort_factor;
  if (sp.ringCap_BodyTrailingIdx === 0) {
    sp.ring_BodyTrailingIdx_derived[0] = ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
  if (sp.ringCap_ShadowLongTrailingIdx === 0) {
    sp.ring_ShadowLongTrailingIdx_derived[0] = ((ShadowLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
  if (sp.ringCap_ShadowVeryShortTrailingIdx === 0) {
    sp.ring_ShadowVeryShortTrailingIdx_derived[0] = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
  if (((inOpen > inClose ? inOpen : inClose) < (sp.lag1_inOpen < sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose)) && /* gap down */
      Math.abs(inClose - inOpen) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (sp.BodyPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) && /* small rb */
      (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) > ((ShadowLong_factor * (((ShadowLong_avgPeriod !== 0) ? (sp.ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowLong_rangeType === 2) ? 2.0 : 1.0)))) && /* long upper shadow */
      (((inClose >= inOpen) ? inOpen : inClose) - inLow) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (sp.ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0))))) /* very short lower shadow */
  {
    sp.cur_outInteger = 100;
  } else {
    sp.cur_outInteger = 0;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  sp.BodyPeriodTotal += ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0))) - sp.ring_BodyTrailingIdx_derived[sp.ringPos_BodyTrailingIdx];
  sp.ShadowLongPeriodTotal += ((ShadowLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0))) - sp.ring_ShadowLongTrailingIdx_derived[sp.ringPos_ShadowLongTrailingIdx];
  sp.ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0))) - sp.ring_ShadowVeryShortTrailingIdx_derived[sp.ringPos_ShadowVeryShortTrailingIdx];
  sp.lag1_inOpen = inOpen;
  sp.lag1_inClose = inClose;
  sp.ring_BodyTrailingIdx_derived[sp.ringPos_BodyTrailingIdx] = ((BodyShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType === 1) ? (inHigh - inLow) : ((BodyShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  sp.ringPos_BodyTrailingIdx = sp.ringPos_BodyTrailingIdx + 1;
  if (sp.ringPos_BodyTrailingIdx >= sp.ringCap_BodyTrailingIdx) {
    sp.ringPos_BodyTrailingIdx = 0;
  }
  sp.ring_ShadowLongTrailingIdx_derived[sp.ringPos_ShadowLongTrailingIdx] = ((ShadowLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  sp.ringPos_ShadowLongTrailingIdx = sp.ringPos_ShadowLongTrailingIdx + 1;
  if (sp.ringPos_ShadowLongTrailingIdx >= sp.ringCap_ShadowLongTrailingIdx) {
    sp.ringPos_ShadowLongTrailingIdx = 0;
  }
  sp.ring_ShadowVeryShortTrailingIdx_derived[sp.ringPos_ShadowVeryShortTrailingIdx] = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  sp.ringPos_ShadowVeryShortTrailingIdx = sp.ringPos_ShadowVeryShortTrailingIdx + 1;
  if (sp.ringPos_ShadowVeryShortTrailingIdx >= sp.ringCap_ShadowVeryShortTrailingIdx) {
    sp.ringPos_ShadowVeryShortTrailingIdx = 0;
  }
}

function cdlinvertedhammerOpenImpl(
  sp: CdlinvertedhammerStream,
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
  let BodyPeriodTotal = 0;
  let ShadowLongPeriodTotal = 0;
  let ShadowVeryShortPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let BodyTrailingIdx = 0;
  let ShadowLongTrailingIdx = 0;
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
  const BodyShort_rangeType: number = core.candleSettings[CandleSettingType.BODY_SHORT].rangeType;
  const BodyShort_avgPeriod = core.candleSettings[CandleSettingType.BODY_SHORT].avgPeriod;
  const BodyShort_factor = core.candleSettings[CandleSettingType.BODY_SHORT].factor;
  const ShadowLong_rangeType: number = core.candleSettings[CandleSettingType.SHADOW_LONG].rangeType;
  const ShadowLong_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_LONG].avgPeriod;
  const ShadowLong_factor = core.candleSettings[CandleSettingType.SHADOW_LONG].factor;
  const ShadowVeryShort_rangeType: number = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].rangeType;
  const ShadowVeryShort_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].avgPeriod;
  const ShadowVeryShort_factor = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].factor;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlinvertedhammerLookbackPositional(core);
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
  BodyPeriodTotal = 0;
  BodyTrailingIdx = startIdx - BodyShort_avgPeriod;
  ShadowLongPeriodTotal = 0;
  ShadowLongTrailingIdx = startIdx - ShadowLong_avgPeriod;
  ShadowVeryShortPeriodTotal = 0;
  ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
  i = BodyTrailingIdx;
  while (i < startIdx) {
    BodyPeriodTotal += ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = ShadowLongTrailingIdx;
  while (i < startIdx) {
    ShadowLongPeriodTotal += ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = ShadowVeryShortTrailingIdx;
  while (i < startIdx) {
    ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - small real body
   * - long upper shadow
   * - no, or very short, lower shadow
   * - gap down
   * The meaning of "short", "very short" and "long" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (((inOpen[i] > inClose[i] ? inOpen[i] : inClose[i]) < (inOpen[i - 1] < inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1])) && /* gap down */
        Math.abs(inClose[i] - inOpen[i]) < ((BodyShort_factor * (((BodyShort_avgPeriod !== 0) ? (BodyPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyShort_rangeType === 2) ? 2.0 : 1.0)))) && /* small rb */
        (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowLong_factor * (((ShadowLong_avgPeriod !== 0) ? (ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowLong_rangeType === 2) ? 2.0 : 1.0)))) && /* long upper shadow */
        (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0))))) /* very short lower shadow */
    {
      outInteger[((outIdx++ * outStride) | 0)] = 100;
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyPeriodTotal += ((BodyShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyShort_rangeType === 0) ? (Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : ((BodyShort_rangeType === 1) ? (inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) : ((BodyShort_rangeType === 2) ? ((inHigh[BodyTrailingIdx] - (((inClose[BodyTrailingIdx]) >= (inOpen[BodyTrailingIdx])) ? (inClose[BodyTrailingIdx]) : (inOpen[BodyTrailingIdx]))) + ((((inClose[BodyTrailingIdx]) >= (inOpen[BodyTrailingIdx])) ? (inOpen[BodyTrailingIdx]) : (inClose[BodyTrailingIdx])) - inLow[BodyTrailingIdx])) : 0.0)));
    ShadowLongPeriodTotal += ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((ShadowLong_rangeType === 0) ? (Math.abs(inClose[ShadowLongTrailingIdx] - inOpen[ShadowLongTrailingIdx])) : ((ShadowLong_rangeType === 1) ? (inHigh[ShadowLongTrailingIdx] - inLow[ShadowLongTrailingIdx]) : ((ShadowLong_rangeType === 2) ? ((inHigh[ShadowLongTrailingIdx] - (((inClose[ShadowLongTrailingIdx]) >= (inOpen[ShadowLongTrailingIdx])) ? (inClose[ShadowLongTrailingIdx]) : (inOpen[ShadowLongTrailingIdx]))) + ((((inClose[ShadowLongTrailingIdx]) >= (inOpen[ShadowLongTrailingIdx])) ? (inOpen[ShadowLongTrailingIdx]) : (inClose[ShadowLongTrailingIdx])) - inLow[ShadowLongTrailingIdx])) : 0.0)));
    ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx] - inOpen[ShadowVeryShortTrailingIdx])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[ShadowVeryShortTrailingIdx] - inLow[ShadowVeryShortTrailingIdx]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[ShadowVeryShortTrailingIdx] - (((inClose[ShadowVeryShortTrailingIdx]) >= (inOpen[ShadowVeryShortTrailingIdx])) ? (inClose[ShadowVeryShortTrailingIdx]) : (inOpen[ShadowVeryShortTrailingIdx]))) + ((((inClose[ShadowVeryShortTrailingIdx]) >= (inOpen[ShadowVeryShortTrailingIdx])) ? (inOpen[ShadowVeryShortTrailingIdx]) : (inClose[ShadowVeryShortTrailingIdx])) - inLow[ShadowVeryShortTrailingIdx])) : 0.0)));
    i += 1;
    BodyTrailingIdx += 1;
    ShadowLongTrailingIdx += 1;
    ShadowVeryShortTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const cap_BodyTrailingIdx = i - BodyTrailingIdx;
  if (cap_BodyTrailingIdx < 0 || cap_BodyTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_BodyTrailingIdx = cap_BodyTrailingIdx > 0 ? cap_BodyTrailingIdx : 1;
  const capRing_BodyTrailingIdx_derived = new Float64Array(allocN_BodyTrailingIdx);
  for (let fillJ = historyLen - cap_BodyTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_BodyTrailingIdx_derived[fillJ - (historyLen - cap_BodyTrailingIdx)] = ((BodyShort_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyShort_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyShort_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
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
  const cap_ShadowVeryShortTrailingIdx = i - ShadowVeryShortTrailingIdx;
  if (cap_ShadowVeryShortTrailingIdx < 0 || cap_ShadowVeryShortTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_ShadowVeryShortTrailingIdx = cap_ShadowVeryShortTrailingIdx > 0 ? cap_ShadowVeryShortTrailingIdx : 1;
  const capRing_ShadowVeryShortTrailingIdx_derived = new Float64Array(allocN_ShadowVeryShortTrailingIdx);
  for (let fillJ = historyLen - cap_ShadowVeryShortTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_ShadowVeryShortTrailingIdx_derived[fillJ - (historyLen - cap_ShadowVeryShortTrailingIdx)] = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  sp.BodyPeriodTotal = BodyPeriodTotal;
  sp.ShadowLongPeriodTotal = ShadowLongPeriodTotal;
  sp.ShadowVeryShortPeriodTotal = ShadowVeryShortPeriodTotal;
  sp.lag1_inOpen = inOpen[historyLen - 1];
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.ringPos_BodyTrailingIdx = 0;
  sp.ringCap_BodyTrailingIdx = cap_BodyTrailingIdx;
  sp.ring_BodyTrailingIdx_derived = capRing_BodyTrailingIdx_derived;
  sp.ringPos_ShadowLongTrailingIdx = 0;
  sp.ringCap_ShadowLongTrailingIdx = cap_ShadowLongTrailingIdx;
  sp.ring_ShadowLongTrailingIdx_derived = capRing_ShadowLongTrailingIdx_derived;
  sp.ringPos_ShadowVeryShortTrailingIdx = 0;
  sp.ringCap_ShadowVeryShortTrailingIdx = cap_ShadowVeryShortTrailingIdx;
  sp.ring_ShadowVeryShortTrailingIdx_derived = capRing_ShadowVeryShortTrailingIdx_derived;
  sp.cs_BodyShort_rangeType = BodyShort_rangeType;
  sp.cs_BodyShort_avgPeriod = BodyShort_avgPeriod;
  sp.cs_BodyShort_factor = BodyShort_factor;
  sp.cs_ShadowLong_rangeType = ShadowLong_rangeType;
  sp.cs_ShadowLong_avgPeriod = ShadowLong_avgPeriod;
  sp.cs_ShadowLong_factor = ShadowLong_factor;
  sp.cs_ShadowVeryShort_rangeType = ShadowVeryShort_rangeType;
  sp.cs_ShadowVeryShort_avgPeriod = ShadowVeryShort_avgPeriod;
  sp.cs_ShadowVeryShort_factor = ShadowVeryShort_factor;
  sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlinvertedhammerOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): CdlinvertedhammerStream {
  const sp = CdlinvertedhammerStream.of(core);
  const retCode = cdlinvertedhammerOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLINVERTEDHAMMER openAndFill', inOpen.length, startIdx, cdlinvertedhammerLookbackPositional(core));
  }
  throw streamFailure('CDLINVERTEDHAMMER openAndFill', retCode);
}

/** @internal */
export function cdlinvertedhammerOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): CdlinvertedhammerStream {
  const sp = CdlinvertedhammerStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdlinvertedhammerOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLINVERTEDHAMMER open', inOpen.length, startIdx, cdlinvertedhammerLookbackPositional(core));
  }
  throw streamFailure('CDLINVERTEDHAMMER open', retCode);
}

/** @internal */
export function cdlinvertedhammerOpenPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): CdlinvertedhammerStream {
  requireArgument('CDLINVERTEDHAMMER open', 'prices.open', inOpen);
  requireHistory('CDLINVERTEDHAMMER open', 'prices.open', inOpen.length);
  requireArgument('CDLINVERTEDHAMMER open', 'prices.high', inHigh);
  requireArgument('CDLINVERTEDHAMMER open', 'prices.low', inLow);
  requireArgument('CDLINVERTEDHAMMER open', 'prices.close', inClose);
  requireHistoryLength('CDLINVERTEDHAMMER open', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLINVERTEDHAMMER open', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLINVERTEDHAMMER open', 'prices.close', inClose.length, inOpen.length);
  return cdlinvertedhammerOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/** @internal */
export function cdlinvertedhammerOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): CdlinvertedhammerStream {
  requireArgument('CDLINVERTEDHAMMER openAndFill', 'prices.open', inOpen);
  requireHistory('CDLINVERTEDHAMMER openAndFill', 'prices.open', inOpen.length);
  requireArgument('CDLINVERTEDHAMMER openAndFill', 'prices.high', inHigh);
  requireArgument('CDLINVERTEDHAMMER openAndFill', 'prices.low', inLow);
  requireArgument('CDLINVERTEDHAMMER openAndFill', 'prices.close', inClose);
  const guardOutLen = openFillCount('CDLINVERTEDHAMMER openAndFill', inOpen.length, cdlinvertedhammerLookbackPositional(core));
  requireHistoryLength('CDLINVERTEDHAMMER openAndFill', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLINVERTEDHAMMER openAndFill', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLINVERTEDHAMMER openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('CDLINVERTEDHAMMER openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('CDLINVERTEDHAMMER openAndFill', 'outInteger', 'prices.open');
  }
  if (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CDLINVERTEDHAMMER openAndFill', 'outInteger', 'prices.high');
  }
  if (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CDLINVERTEDHAMMER openAndFill', 'outInteger', 'prices.low');
  }
  if (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('CDLINVERTEDHAMMER openAndFill', 'outInteger', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLINVERTEDHAMMER openAndFill', 'outInteger', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLINVERTEDHAMMER openAndFill', 'outInteger', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLINVERTEDHAMMER openAndFill', 'outInteger', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLINVERTEDHAMMER openAndFill', 'outInteger', 'prices.close');
  }
  return cdlinvertedhammerOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}

/**
 * Opens a live CDLINVERTEDHAMMER stream over the warm-up history: `value()` starts
 * at the last history bar, bit-identical to {@link cdlinvertedhammer} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlinvertedhammerOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlinvertedhammerLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input or histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdlinvertedhammerOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdlinvertedhammerOpen({ open, high, low, close });
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdlinvertedhammerOpen(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlinvertedhammerOptions): CdlinvertedhammerStream {
  requireArgument('CDLINVERTEDHAMMER open', 'prices', prices);
  const o = optionsOf('CDLINVERTEDHAMMER open', options);
  const core = coreOf('CDLINVERTEDHAMMER open', o.core);
  return cdlinvertedhammerOpenPositional(prices.open, prices.high, prices.low, prices.close, core);
}

/**
 * {@link cdlinvertedhammerOpen} that also computes the outputs over the whole
 * history in the same pass, bit-identically to {@link cdlinvertedhammer}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CdlinvertedhammerOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cdlinvertedhammer} returns them; the range is
 *   the stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlinvertedhammerLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, histories of different lengths or a buffer too short,
 *   of the wrong type or starting where another buffer or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdlinvertedhammerOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = cdlinvertedhammerOpenAndFill({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cdlinvertedhammerOpenAndFill(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlinvertedhammerOptions): CdlinvertedhammerResult & { stream: CdlinvertedhammerStream } {
  requireArgument('CDLINVERTEDHAMMER openAndFill', 'prices', prices);
  const o = optionsOf('CDLINVERTEDHAMMER openAndFill', options);
  const core = coreOf('CDLINVERTEDHAMMER openAndFill', o.core);
  const capacity = fillCapacity(cdlinvertedhammerLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const stream = cdlinvertedhammerOpenAndFillPositional(prices.open, prices.high, prices.low, prices.close, outInteger, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}
