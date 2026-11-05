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
 *  011505 AC     Creation
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 *  103126 MF,CC  Nest the cheap, selective pre-check conjuncts in an outer if
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { CandleSettingType, Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function cdltakuriLookbackPositional(core: Core = Core.DEFAULT): number {
  const BodyDoji_avgPeriod = core.candleSettings[CandleSettingType.BODY_DOJI].avgPeriod;
  const ShadowVeryLong_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_VERY_LONG].avgPeriod;
  const ShadowVeryShort_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].avgPeriod;
  return ((BodyDoji_avgPeriod > ShadowVeryShort_avgPeriod ? BodyDoji_avgPeriod : ShadowVeryShort_avgPeriod) > ShadowVeryLong_avgPeriod ? (BodyDoji_avgPeriod > ShadowVeryShort_avgPeriod ? BodyDoji_avgPeriod : ShadowVeryShort_avgPeriod) : ShadowVeryLong_avgPeriod);
}

/** @internal */
export function cdltakuriDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdltakuriImpl(
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
  let BodyDojiPeriodTotal = 0;
  let ShadowVeryShortPeriodTotal = 0;
  let ShadowVeryLongPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let BodyDojiTrailingIdx = 0;
  let ShadowVeryShortTrailingIdx = 0;
  let ShadowVeryLongTrailingIdx = 0;
  let lookbackTotal = 0;
  const BodyDoji_rangeType: number = core.candleSettings[CandleSettingType.BODY_DOJI].rangeType;
  const BodyDoji_avgPeriod = core.candleSettings[CandleSettingType.BODY_DOJI].avgPeriod;
  const BodyDoji_factor = core.candleSettings[CandleSettingType.BODY_DOJI].factor;
  const ShadowVeryLong_rangeType: number = core.candleSettings[CandleSettingType.SHADOW_VERY_LONG].rangeType;
  const ShadowVeryLong_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_VERY_LONG].avgPeriod;
  const ShadowVeryLong_factor = core.candleSettings[CandleSettingType.SHADOW_VERY_LONG].factor;
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
  lookbackTotal = cdltakuriLookbackPositional(core);
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
  BodyDojiPeriodTotal = 0;
  BodyDojiTrailingIdx = startIdx - BodyDoji_avgPeriod;
  ShadowVeryShortPeriodTotal = 0;
  ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
  ShadowVeryLongPeriodTotal = 0;
  ShadowVeryLongTrailingIdx = startIdx - ShadowVeryLong_avgPeriod;
  i = BodyDojiTrailingIdx;
  while (i < startIdx) {
    BodyDojiPeriodTotal += ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = ShadowVeryShortTrailingIdx;
  while (i < startIdx) {
    ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = ShadowVeryLongTrailingIdx;
  while (i < startIdx) {
    ShadowVeryLongPeriodTotal += ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  /* Proceed with the calculation for the requested range.
   *
   * Must have:
   * - doji body
   * - open and close at the high of the day = no or very short upper shadow
   * - very long lower shadow
   * The meaning of "doji", "very short" and "very long" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (Math.abs(inClose[i] - inOpen[i]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod !== 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyDoji_rangeType === 2) ? 2.0 : 1.0)))) && (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod !== 0) ? (ShadowVeryLongPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowVeryLong_rangeType === 2) ? 2.0 : 1.0))))) {
      outInteger[outIdx++] = 100;
    } else {
      outInteger[outIdx++] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyDojiPeriodTotal += ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : ((BodyDoji_rangeType === 1) ? (inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) : ((BodyDoji_rangeType === 2) ? ((inHigh[BodyDojiTrailingIdx] - (((inClose[BodyDojiTrailingIdx]) >= (inOpen[BodyDojiTrailingIdx])) ? (inClose[BodyDojiTrailingIdx]) : (inOpen[BodyDojiTrailingIdx]))) + ((((inClose[BodyDojiTrailingIdx]) >= (inOpen[BodyDojiTrailingIdx])) ? (inOpen[BodyDojiTrailingIdx]) : (inClose[BodyDojiTrailingIdx])) - inLow[BodyDojiTrailingIdx])) : 0.0)));
    ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx] - inOpen[ShadowVeryShortTrailingIdx])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[ShadowVeryShortTrailingIdx] - inLow[ShadowVeryShortTrailingIdx]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[ShadowVeryShortTrailingIdx] - (((inClose[ShadowVeryShortTrailingIdx]) >= (inOpen[ShadowVeryShortTrailingIdx])) ? (inClose[ShadowVeryShortTrailingIdx]) : (inOpen[ShadowVeryShortTrailingIdx]))) + ((((inClose[ShadowVeryShortTrailingIdx]) >= (inOpen[ShadowVeryShortTrailingIdx])) ? (inOpen[ShadowVeryShortTrailingIdx]) : (inClose[ShadowVeryShortTrailingIdx])) - inLow[ShadowVeryShortTrailingIdx])) : 0.0)));
    ShadowVeryLongPeriodTotal += ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose[ShadowVeryLongTrailingIdx] - inOpen[ShadowVeryLongTrailingIdx])) : ((ShadowVeryLong_rangeType === 1) ? (inHigh[ShadowVeryLongTrailingIdx] - inLow[ShadowVeryLongTrailingIdx]) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh[ShadowVeryLongTrailingIdx] - (((inClose[ShadowVeryLongTrailingIdx]) >= (inOpen[ShadowVeryLongTrailingIdx])) ? (inClose[ShadowVeryLongTrailingIdx]) : (inOpen[ShadowVeryLongTrailingIdx]))) + ((((inClose[ShadowVeryLongTrailingIdx]) >= (inOpen[ShadowVeryLongTrailingIdx])) ? (inOpen[ShadowVeryLongTrailingIdx]) : (inClose[ShadowVeryLongTrailingIdx])) - inLow[ShadowVeryLongTrailingIdx])) : 0.0)));
    i += 1;
    BodyDojiTrailingIdx += 1;
    ShadowVeryShortTrailingIdx += 1;
    ShadowVeryLongTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function cdltakuriPositional(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDLTAKURI', 'startIdx', startIdx);
  endIdx = requireInt32('CDLTAKURI', 'endIdx', endIdx);
  requireIndexRange('CDLTAKURI', startIdx, endIdx);
  const guardStart = clampedStart('CDLTAKURI', startIdx, cdltakuriLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLTAKURI', 'prices.open', inOpen, guardInLen);
  requireLength('CDLTAKURI', 'prices.high', inHigh, guardInLen);
  requireLength('CDLTAKURI', 'prices.low', inLow, guardInLen);
  requireLength('CDLTAKURI', 'prices.close', inClose, guardInLen);
  requireOutput('CDLTAKURI', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLTAKURI', 'outInteger', 'prices.open');
  }
  if (overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLTAKURI', 'outInteger', 'prices.high');
  }
  if (overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLTAKURI', 'outInteger', 'prices.low');
  }
  if (overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLTAKURI', 'outInteger', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdltakuriImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLTAKURI', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cdltakuri}, {@link cdltakuriLookback},
 * {@link cdltakuriDisplayShift}, {@link cdltakuriOpen} and
 * {@link cdltakuriOpenAndFill}. Every key is optional: an omitted or `undefined`
 * key takes its default, and a call ignores the keys it has no use for.
 */
export interface CdltakuriOptions {
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
 * What {@link cdltakuri} returns, and {@link cdltakuriOpenAndFill} with its
 * stream.
 */
export interface CdltakuriResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * +100 when the takuri pattern is detected, 0 otherwise. Never negative; the
   * positive sign is a convention and does not by itself imply bullishness.
   * `values[k]` is the value at input bar `begIdx + k`.
   */
  values: Int32Array;
}

/**
 * Number of leading input bars {@link cdltakuri} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdltakuriOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdltakuriLookback(options?: CdltakuriOptions): number {
  const o = optionsOf('CDLTAKURI lookback', options);
  return cdltakuriLookbackPositional(coreOf('CDLTAKURI lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdltakuri}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdltakuriOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdltakuriDisplayShift(outputIdx: number, options?: CdltakuriOptions): number {
  const o = optionsOf('CDLTAKURI displayShift', options);
  return cdltakuriDisplayShiftPositional(outputIdx, coreOf('CDLTAKURI displayShift', o.core));
}

/**
 * Single-candle pattern: a doji whose open and close sit at the high (no/very
 * short upper shadow) with a very long lower shadow, i.e. a dragonfly doji with an
 * exceptionally long lower shadow. Emitted as a positive signal, but its
 * directional meaning depends on the prevailing trend, which the code does not
 * check. A hit marks a takuri (dragonfly-doji) line; a potential reversal only
 * when read against the trend (typically a bottom/bullish reversal after a
 * downtrend), which the code itself does not verify.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdltakuri](https://ta-lib.org/functions/cdltakuri).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cdltakuriLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `open`: Open
 *   price of each bar. `high`: High price of each bar. `low`: Low price of each bar.
 *   `close`: Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link CdltakuriOptions}.
 * @returns `begIdx` and the values; see {@link CdltakuriResult}. Each output is a
 *   view of its buffer, the caller's or a new one.
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
 * import { cdltakuri } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { begIdx, values } = cdltakuri({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`cdldragonflydoji`](https://ta-lib.org/functions/cdldragonflydoji)
 * @see [`cdldoji`](https://ta-lib.org/functions/cdldoji)
 * @see [`cdlhammer`](https://ta-lib.org/functions/cdlhammer)
 * @see [`cdlgravestonedoji`](https://ta-lib.org/functions/cdlgravestonedoji)
 */
export function cdltakuri(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: CdltakuriOptions): CdltakuriResult {
  requireArgument('CDLTAKURI', 'prices', prices);
  const o = optionsOf('CDLTAKURI', options);
  if (o.endIdx === undefined) {
    requireSeries('CDLTAKURI', 'prices.open', prices.open);
    requireSeries('CDLTAKURI', 'prices.high', prices.high);
    requireSeries('CDLTAKURI', 'prices.low', prices.low);
    requireSeries('CDLTAKURI', 'prices.close', prices.close);
  }
  const core = coreOf('CDLTAKURI', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cdltakuriLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const range = cdltakuriPositional(startIdx, endIdx, prices.open, prices.high, prices.low, prices.close, outInteger, core);
  return { begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CDLTAKURI stream: one value per closed bar, bit-identical to
 * {@link cdltakuri} over the same series. Open it with {@link cdltakuriOpen} or
 * {@link cdltakuriOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdltakuri](https://ta-lib.org/functions/cdltakuri).
 */
export class CdltakuriStream {
  /** @internal */
  core: Core;
  /** @internal */
  BodyDojiPeriodTotal: number = 0;
  /** @internal */
  ShadowVeryShortPeriodTotal: number = 0;
  /** @internal */
  ShadowVeryLongPeriodTotal: number = 0;
  /** @internal */
  ringPos_BodyDojiTrailingIdx: number = 0;
  /** @internal */
  ringCap_BodyDojiTrailingIdx: number = 0;
  /** @internal */
  ring_BodyDojiTrailingIdx_derived: Float64Array = new Float64Array(1);
  /** @internal */
  ringPos_ShadowVeryLongTrailingIdx: number = 0;
  /** @internal */
  ringCap_ShadowVeryLongTrailingIdx: number = 0;
  /** @internal */
  ring_ShadowVeryLongTrailingIdx_derived: Float64Array = new Float64Array(1);
  /** @internal */
  ringPos_ShadowVeryShortTrailingIdx: number = 0;
  /** @internal */
  ringCap_ShadowVeryShortTrailingIdx: number = 0;
  /** @internal */
  ring_ShadowVeryShortTrailingIdx_derived: Float64Array = new Float64Array(1);
  /** @internal */
  cs_BodyDoji_rangeType: number = 0;
  /** @internal */
  cs_BodyDoji_avgPeriod: number = 0;
  /** @internal */
  cs_BodyDoji_factor: number = 0;
  /** @internal */
  cs_ShadowVeryLong_rangeType: number = 0;
  /** @internal */
  cs_ShadowVeryLong_avgPeriod: number = 0;
  /** @internal */
  cs_ShadowVeryLong_factor: number = 0;
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
  static of(core: Core): CdltakuriStream {
    return new CdltakuriStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cdltakuri} reports them over the same bars. Each accepted `update` and
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
      throw failure('CDLTAKURI advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw failure('CDLTAKURI update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inOpen) || !Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('CDLTAKURI update', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    cdltakuriStepImpl(this, inOpen, inHigh, inLow, inClose);
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
      throw nonFiniteBar('CDLTAKURI peek', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let cur_outInteger = 0;
    const BodyDoji_rangeType = sp.cs_BodyDoji_rangeType;
    const BodyDoji_avgPeriod = sp.cs_BodyDoji_avgPeriod;
    const BodyDoji_factor = sp.cs_BodyDoji_factor;
    const ShadowVeryLong_rangeType = sp.cs_ShadowVeryLong_rangeType;
    const ShadowVeryLong_avgPeriod = sp.cs_ShadowVeryLong_avgPeriod;
    const ShadowVeryLong_factor = sp.cs_ShadowVeryLong_factor;
    const ShadowVeryShort_rangeType = sp.cs_ShadowVeryShort_rangeType;
    const ShadowVeryShort_avgPeriod = sp.cs_ShadowVeryShort_avgPeriod;
    const ShadowVeryShort_factor = sp.cs_ShadowVeryShort_factor;
    if (Math.abs(inClose - inOpen) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod !== 0) ? (sp.BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyDoji_rangeType === 1) ? (inHigh - inLow) : ((BodyDoji_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyDoji_rangeType === 2) ? 2.0 : 1.0)))) && (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (sp.ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && (((inClose >= inOpen) ? inOpen : inClose) - inLow) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod !== 0) ? (sp.ShadowVeryLongPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowVeryLong_rangeType === 2) ? 2.0 : 1.0))))) {
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
  clone(): CdltakuriStream {
    const c = new CdltakuriStream(this.core);
    c.BodyDojiPeriodTotal = this.BodyDojiPeriodTotal;
    c.ShadowVeryShortPeriodTotal = this.ShadowVeryShortPeriodTotal;
    c.ShadowVeryLongPeriodTotal = this.ShadowVeryLongPeriodTotal;
    c.ringPos_BodyDojiTrailingIdx = this.ringPos_BodyDojiTrailingIdx;
    c.ringCap_BodyDojiTrailingIdx = this.ringCap_BodyDojiTrailingIdx;
    c.ring_BodyDojiTrailingIdx_derived = this.ring_BodyDojiTrailingIdx_derived.slice();
    c.ringPos_ShadowVeryLongTrailingIdx = this.ringPos_ShadowVeryLongTrailingIdx;
    c.ringCap_ShadowVeryLongTrailingIdx = this.ringCap_ShadowVeryLongTrailingIdx;
    c.ring_ShadowVeryLongTrailingIdx_derived = this.ring_ShadowVeryLongTrailingIdx_derived.slice();
    c.ringPos_ShadowVeryShortTrailingIdx = this.ringPos_ShadowVeryShortTrailingIdx;
    c.ringCap_ShadowVeryShortTrailingIdx = this.ringCap_ShadowVeryShortTrailingIdx;
    c.ring_ShadowVeryShortTrailingIdx_derived = this.ring_ShadowVeryShortTrailingIdx_derived.slice();
    c.cs_BodyDoji_rangeType = this.cs_BodyDoji_rangeType;
    c.cs_BodyDoji_avgPeriod = this.cs_BodyDoji_avgPeriod;
    c.cs_BodyDoji_factor = this.cs_BodyDoji_factor;
    c.cs_ShadowVeryLong_rangeType = this.cs_ShadowVeryLong_rangeType;
    c.cs_ShadowVeryLong_avgPeriod = this.cs_ShadowVeryLong_avgPeriod;
    c.cs_ShadowVeryLong_factor = this.cs_ShadowVeryLong_factor;
    c.cs_ShadowVeryShort_rangeType = this.cs_ShadowVeryShort_rangeType;
    c.cs_ShadowVeryShort_avgPeriod = this.cs_ShadowVeryShort_avgPeriod;
    c.cs_ShadowVeryShort_factor = this.cs_ShadowVeryShort_factor;
    c.cur_outInteger = this.cur_outInteger;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cdltakuriStepImpl(sp: CdltakuriStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  const BodyDoji_rangeType = sp.cs_BodyDoji_rangeType;
  const BodyDoji_avgPeriod = sp.cs_BodyDoji_avgPeriod;
  const BodyDoji_factor = sp.cs_BodyDoji_factor;
  const ShadowVeryLong_rangeType = sp.cs_ShadowVeryLong_rangeType;
  const ShadowVeryLong_avgPeriod = sp.cs_ShadowVeryLong_avgPeriod;
  const ShadowVeryLong_factor = sp.cs_ShadowVeryLong_factor;
  const ShadowVeryShort_rangeType = sp.cs_ShadowVeryShort_rangeType;
  const ShadowVeryShort_avgPeriod = sp.cs_ShadowVeryShort_avgPeriod;
  const ShadowVeryShort_factor = sp.cs_ShadowVeryShort_factor;
  if (sp.ringCap_BodyDojiTrailingIdx === 0) {
    sp.ring_BodyDojiTrailingIdx_derived[0] = ((BodyDoji_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyDoji_rangeType === 1) ? (inHigh - inLow) : ((BodyDoji_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
  if (sp.ringCap_ShadowVeryLongTrailingIdx === 0) {
    sp.ring_ShadowVeryLongTrailingIdx_derived[0] = ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
  if (sp.ringCap_ShadowVeryShortTrailingIdx === 0) {
    sp.ring_ShadowVeryShortTrailingIdx_derived[0] = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
  if (Math.abs(inClose - inOpen) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod !== 0) ? (sp.BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyDoji_rangeType === 1) ? (inHigh - inLow) : ((BodyDoji_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyDoji_rangeType === 2) ? 2.0 : 1.0)))) && (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (sp.ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && (((inClose >= inOpen) ? inOpen : inClose) - inLow) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod !== 0) ? (sp.ShadowVeryLongPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowVeryLong_rangeType === 2) ? 2.0 : 1.0))))) {
    sp.cur_outInteger = 100;
  } else {
    sp.cur_outInteger = 0;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  sp.BodyDojiPeriodTotal += ((BodyDoji_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyDoji_rangeType === 1) ? (inHigh - inLow) : ((BodyDoji_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0))) - sp.ring_BodyDojiTrailingIdx_derived[sp.ringPos_BodyDojiTrailingIdx];
  sp.ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0))) - sp.ring_ShadowVeryShortTrailingIdx_derived[sp.ringPos_ShadowVeryShortTrailingIdx];
  sp.ShadowVeryLongPeriodTotal += ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0))) - sp.ring_ShadowVeryLongTrailingIdx_derived[sp.ringPos_ShadowVeryLongTrailingIdx];
  sp.ring_BodyDojiTrailingIdx_derived[sp.ringPos_BodyDojiTrailingIdx] = ((BodyDoji_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyDoji_rangeType === 1) ? (inHigh - inLow) : ((BodyDoji_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  sp.ringPos_BodyDojiTrailingIdx = sp.ringPos_BodyDojiTrailingIdx + 1;
  if (sp.ringPos_BodyDojiTrailingIdx >= sp.ringCap_BodyDojiTrailingIdx) {
    sp.ringPos_BodyDojiTrailingIdx = 0;
  }
  sp.ring_ShadowVeryLongTrailingIdx_derived[sp.ringPos_ShadowVeryLongTrailingIdx] = ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryLong_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  sp.ringPos_ShadowVeryLongTrailingIdx = sp.ringPos_ShadowVeryLongTrailingIdx + 1;
  if (sp.ringPos_ShadowVeryLongTrailingIdx >= sp.ringCap_ShadowVeryLongTrailingIdx) {
    sp.ringPos_ShadowVeryLongTrailingIdx = 0;
  }
  sp.ring_ShadowVeryShortTrailingIdx_derived[sp.ringPos_ShadowVeryShortTrailingIdx] = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  sp.ringPos_ShadowVeryShortTrailingIdx = sp.ringPos_ShadowVeryShortTrailingIdx + 1;
  if (sp.ringPos_ShadowVeryShortTrailingIdx >= sp.ringCap_ShadowVeryShortTrailingIdx) {
    sp.ringPos_ShadowVeryShortTrailingIdx = 0;
  }
}

function cdltakuriOpenImpl(
  sp: CdltakuriStream,
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
  let BodyDojiPeriodTotal = 0;
  let ShadowVeryShortPeriodTotal = 0;
  let ShadowVeryLongPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let BodyDojiTrailingIdx = 0;
  let ShadowVeryShortTrailingIdx = 0;
  let ShadowVeryLongTrailingIdx = 0;
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
  const BodyDoji_rangeType: number = core.candleSettings[CandleSettingType.BODY_DOJI].rangeType;
  const BodyDoji_avgPeriod = core.candleSettings[CandleSettingType.BODY_DOJI].avgPeriod;
  const BodyDoji_factor = core.candleSettings[CandleSettingType.BODY_DOJI].factor;
  const ShadowVeryLong_rangeType: number = core.candleSettings[CandleSettingType.SHADOW_VERY_LONG].rangeType;
  const ShadowVeryLong_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_VERY_LONG].avgPeriod;
  const ShadowVeryLong_factor = core.candleSettings[CandleSettingType.SHADOW_VERY_LONG].factor;
  const ShadowVeryShort_rangeType: number = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].rangeType;
  const ShadowVeryShort_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].avgPeriod;
  const ShadowVeryShort_factor = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].factor;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdltakuriLookbackPositional(core);
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
  BodyDojiPeriodTotal = 0;
  BodyDojiTrailingIdx = startIdx - BodyDoji_avgPeriod;
  ShadowVeryShortPeriodTotal = 0;
  ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
  ShadowVeryLongPeriodTotal = 0;
  ShadowVeryLongTrailingIdx = startIdx - ShadowVeryLong_avgPeriod;
  i = BodyDojiTrailingIdx;
  while (i < startIdx) {
    BodyDojiPeriodTotal += ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = ShadowVeryShortTrailingIdx;
  while (i < startIdx) {
    ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = ShadowVeryLongTrailingIdx;
  while (i < startIdx) {
    ShadowVeryLongPeriodTotal += ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  /* Proceed with the calculation for the requested range.
   *
   * Must have:
   * - doji body
   * - open and close at the high of the day = no or very short upper shadow
   * - very long lower shadow
   * The meaning of "doji", "very short" and "very long" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (Math.abs(inClose[i] - inOpen[i]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod !== 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyDoji_rangeType === 2) ? 2.0 : 1.0)))) && (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) && (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod !== 0) ? (ShadowVeryLongPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowVeryLong_rangeType === 2) ? 2.0 : 1.0))))) {
      outInteger[((outIdx++ * outStride) | 0)] = 100;
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyDojiPeriodTotal += ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : ((BodyDoji_rangeType === 1) ? (inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) : ((BodyDoji_rangeType === 2) ? ((inHigh[BodyDojiTrailingIdx] - (((inClose[BodyDojiTrailingIdx]) >= (inOpen[BodyDojiTrailingIdx])) ? (inClose[BodyDojiTrailingIdx]) : (inOpen[BodyDojiTrailingIdx]))) + ((((inClose[BodyDojiTrailingIdx]) >= (inOpen[BodyDojiTrailingIdx])) ? (inOpen[BodyDojiTrailingIdx]) : (inClose[BodyDojiTrailingIdx])) - inLow[BodyDojiTrailingIdx])) : 0.0)));
    ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx] - inOpen[ShadowVeryShortTrailingIdx])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[ShadowVeryShortTrailingIdx] - inLow[ShadowVeryShortTrailingIdx]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[ShadowVeryShortTrailingIdx] - (((inClose[ShadowVeryShortTrailingIdx]) >= (inOpen[ShadowVeryShortTrailingIdx])) ? (inClose[ShadowVeryShortTrailingIdx]) : (inOpen[ShadowVeryShortTrailingIdx]))) + ((((inClose[ShadowVeryShortTrailingIdx]) >= (inOpen[ShadowVeryShortTrailingIdx])) ? (inOpen[ShadowVeryShortTrailingIdx]) : (inClose[ShadowVeryShortTrailingIdx])) - inLow[ShadowVeryShortTrailingIdx])) : 0.0)));
    ShadowVeryLongPeriodTotal += ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose[ShadowVeryLongTrailingIdx] - inOpen[ShadowVeryLongTrailingIdx])) : ((ShadowVeryLong_rangeType === 1) ? (inHigh[ShadowVeryLongTrailingIdx] - inLow[ShadowVeryLongTrailingIdx]) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh[ShadowVeryLongTrailingIdx] - (((inClose[ShadowVeryLongTrailingIdx]) >= (inOpen[ShadowVeryLongTrailingIdx])) ? (inClose[ShadowVeryLongTrailingIdx]) : (inOpen[ShadowVeryLongTrailingIdx]))) + ((((inClose[ShadowVeryLongTrailingIdx]) >= (inOpen[ShadowVeryLongTrailingIdx])) ? (inOpen[ShadowVeryLongTrailingIdx]) : (inClose[ShadowVeryLongTrailingIdx])) - inLow[ShadowVeryLongTrailingIdx])) : 0.0)));
    i += 1;
    BodyDojiTrailingIdx += 1;
    ShadowVeryShortTrailingIdx += 1;
    ShadowVeryLongTrailingIdx += 1;
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
  const cap_ShadowVeryLongTrailingIdx = i - ShadowVeryLongTrailingIdx;
  if (cap_ShadowVeryLongTrailingIdx < 0 || cap_ShadowVeryLongTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_ShadowVeryLongTrailingIdx = cap_ShadowVeryLongTrailingIdx > 0 ? cap_ShadowVeryLongTrailingIdx : 1;
  const capRing_ShadowVeryLongTrailingIdx_derived = new Float64Array(allocN_ShadowVeryLongTrailingIdx);
  for (let fillJ = historyLen - cap_ShadowVeryLongTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_ShadowVeryLongTrailingIdx_derived[fillJ - (historyLen - cap_ShadowVeryLongTrailingIdx)] = ((ShadowVeryLong_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((ShadowVeryLong_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((ShadowVeryLong_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
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
  sp.BodyDojiPeriodTotal = BodyDojiPeriodTotal;
  sp.ShadowVeryShortPeriodTotal = ShadowVeryShortPeriodTotal;
  sp.ShadowVeryLongPeriodTotal = ShadowVeryLongPeriodTotal;
  sp.ringPos_BodyDojiTrailingIdx = 0;
  sp.ringCap_BodyDojiTrailingIdx = cap_BodyDojiTrailingIdx;
  sp.ring_BodyDojiTrailingIdx_derived = capRing_BodyDojiTrailingIdx_derived;
  sp.ringPos_ShadowVeryLongTrailingIdx = 0;
  sp.ringCap_ShadowVeryLongTrailingIdx = cap_ShadowVeryLongTrailingIdx;
  sp.ring_ShadowVeryLongTrailingIdx_derived = capRing_ShadowVeryLongTrailingIdx_derived;
  sp.ringPos_ShadowVeryShortTrailingIdx = 0;
  sp.ringCap_ShadowVeryShortTrailingIdx = cap_ShadowVeryShortTrailingIdx;
  sp.ring_ShadowVeryShortTrailingIdx_derived = capRing_ShadowVeryShortTrailingIdx_derived;
  sp.cs_BodyDoji_rangeType = BodyDoji_rangeType;
  sp.cs_BodyDoji_avgPeriod = BodyDoji_avgPeriod;
  sp.cs_BodyDoji_factor = BodyDoji_factor;
  sp.cs_ShadowVeryLong_rangeType = ShadowVeryLong_rangeType;
  sp.cs_ShadowVeryLong_avgPeriod = ShadowVeryLong_avgPeriod;
  sp.cs_ShadowVeryLong_factor = ShadowVeryLong_factor;
  sp.cs_ShadowVeryShort_rangeType = ShadowVeryShort_rangeType;
  sp.cs_ShadowVeryShort_avgPeriod = ShadowVeryShort_avgPeriod;
  sp.cs_ShadowVeryShort_factor = ShadowVeryShort_factor;
  sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cdltakuriOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): CdltakuriStream {
  const sp = CdltakuriStream.of(core);
  const retCode = cdltakuriOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLTAKURI openAndFill', inOpen.length, startIdx, cdltakuriLookbackPositional(core));
  }
  throw streamFailure('CDLTAKURI openAndFill', retCode);
}

/** @internal */
export function cdltakuriOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): CdltakuriStream {
  const sp = CdltakuriStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdltakuriOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLTAKURI open', inOpen.length, startIdx, cdltakuriLookbackPositional(core));
  }
  throw streamFailure('CDLTAKURI open', retCode);
}

/** @internal */
export function cdltakuriOpenPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): CdltakuriStream {
  requireArgument('CDLTAKURI open', 'prices.open', inOpen);
  requireHistory('CDLTAKURI open', inOpen.length);
  requireArgument('CDLTAKURI open', 'prices.high', inHigh);
  requireArgument('CDLTAKURI open', 'prices.low', inLow);
  requireArgument('CDLTAKURI open', 'prices.close', inClose);
  requireHistoryLength('CDLTAKURI open', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLTAKURI open', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLTAKURI open', 'prices.close', inClose.length, inOpen.length);
  return cdltakuriOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/** @internal */
export function cdltakuriOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): CdltakuriStream {
  requireArgument('CDLTAKURI openAndFill', 'prices.open', inOpen);
  requireHistory('CDLTAKURI openAndFill', inOpen.length);
  requireArgument('CDLTAKURI openAndFill', 'prices.high', inHigh);
  requireArgument('CDLTAKURI openAndFill', 'prices.low', inLow);
  requireArgument('CDLTAKURI openAndFill', 'prices.close', inClose);
  const guardOutLen = openFillCount('CDLTAKURI openAndFill', inOpen.length, cdltakuriLookbackPositional(core));
  requireHistoryLength('CDLTAKURI openAndFill', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLTAKURI openAndFill', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLTAKURI openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('CDLTAKURI openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('CDLTAKURI openAndFill', 'outInteger', 'prices.open');
  }
  if (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CDLTAKURI openAndFill', 'outInteger', 'prices.high');
  }
  if (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CDLTAKURI openAndFill', 'outInteger', 'prices.low');
  }
  if (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('CDLTAKURI openAndFill', 'outInteger', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLTAKURI openAndFill', 'outInteger', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLTAKURI openAndFill', 'outInteger', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLTAKURI openAndFill', 'outInteger', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLTAKURI openAndFill', 'outInteger', 'prices.close');
  }
  return cdltakuriOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}

/**
 * Opens a live CDLTAKURI stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link cdltakuri} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `open`: Open price of each bar. `high`: High price of each bar. `low`:
 *   Low price of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdltakuriOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdltakuriLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input or histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdltakuriOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdltakuriOpen({ open, high, low, close });
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdltakuriOpen(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: CdltakuriOptions): CdltakuriStream {
  requireArgument('CDLTAKURI open', 'prices', prices);
  const o = optionsOf('CDLTAKURI open', options);
  const core = coreOf('CDLTAKURI open', o.core);
  return cdltakuriOpenPositional(prices.open, prices.high, prices.low, prices.close, core);
}

/**
 * {@link cdltakuriOpen} that also computes the outputs over the whole history in
 * the same pass, bit-identically to {@link cdltakuri}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `open`: Open price of each bar. `high`: High price of each bar. `low`:
 *   Low price of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CdltakuriOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cdltakuri} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdltakuriLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, histories of different lengths or a buffer too short,
 *   of the wrong type or starting where another buffer or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdltakuriOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = cdltakuriOpenAndFill({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cdltakuriOpenAndFill(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: CdltakuriOptions): CdltakuriResult & { stream: CdltakuriStream } {
  requireArgument('CDLTAKURI openAndFill', 'prices', prices);
  const o = optionsOf('CDLTAKURI openAndFill', options);
  const core = coreOf('CDLTAKURI openAndFill', o.core);
  const capacity = fillCapacity(cdltakuriLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const stream = cdltakuriOpenAndFillPositional(prices.open, prices.high, prices.low, prices.close, outInteger, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}
