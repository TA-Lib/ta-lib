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
 *  122605 AC     Creation
 *  071226 MF,CC  Streaming-friendly rewrite: carry the confirmation state
 *                (countdown + cached 3rd-candle high/low) instead of the absolute
 *                bar index, so the per-bar logic reads no cursor. Bit-identical
 *                batch results (verified vs v0.6.4).
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { CandleSettingType, Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function cdlhikkakemodLookbackPositional(core: Core = Core.DEFAULT): number {
  const Near_avgPeriod = core.candleSettings[CandleSettingType.NEAR].avgPeriod;
  return (1 > Near_avgPeriod ? 1 : Near_avgPeriod) + 5;
}

/** @internal */
export function cdlhikkakemodDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdlhikkakemodImpl(
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
  let NearPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let NearTrailingIdx = 0;
  let lookbackTotal = 0;
  let patternResult = 0;
  let patternCount = 0;
  let patternHigh = 0;
  let patternLow = 0;
  const Near_rangeType: number = core.candleSettings[CandleSettingType.NEAR].rangeType;
  const Near_avgPeriod = core.candleSettings[CandleSettingType.NEAR].avgPeriod;
  const Near_factor = core.candleSettings[CandleSettingType.NEAR].factor;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  /* Confirmation window countdown (replaces the absolute patternIdx guard)
   * and a cache of the 3rd candle's high/low (replaces inHigh/inLow
   * [patternIdx-1]) so nothing in the per-bar logic references the cursor.
   */
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlhikkakemodLookbackPositional(core);
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
  NearPeriodTotal = 0;
  NearTrailingIdx = startIdx - 3 - Near_avgPeriod;
  i = NearTrailingIdx;
  while (i < startIdx - 3) {
    NearPeriodTotal += ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
    i += 1;
  }
  patternCount = 0;
  patternResult = 0;
  patternHigh = 0.0;
  patternLow = 0.0;
  i = startIdx - 3;
  while (i < startIdx) {
    /* copy here the pattern recognition code below */
    if (inHigh[i - 2] < inHigh[i - 3] &&
        inLow[i - 2] > inLow[i - 3] &&   /* 2nd: lower high and higher low than 1st */
        inHigh[i - 1] < inHigh[i - 2] &&
        inLow[i - 1] > inLow[i - 2] &&   /* 3rd: lower high and higher low than 2nd */
        (inHigh[i] < inHigh[i - 1] &&
          inLow[i] < inLow[i - 1] &&     /* (bull) 4th: lower high and lower low */
          inClose[i - 2] <= inLow[i - 2] + ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) || /* (bull) 2nd: close near the low */
         inHigh[i] > inHigh[i - 1] &&
          inLow[i] > inLow[i - 1] &&     /* (bear) 4th: higher high and higher low */
          inClose[i - 2] >= inHigh[i - 2] - ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))))) /* (bull) 2nd: close near the top */
    {
      patternResult = ((100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : -1)) | 0);
      patternHigh = inHigh[i - 1];
      patternLow = inLow[i - 1];
      patternCount = 4;
    } else if (patternCount > 0 &&
        (patternResult > 0 &&         /* search for confirmation if modified hikkake was no more than 3 bars ago */
          inClose[i] > patternHigh || /* close higher than the high of 3rd */
         patternResult < 0 &&
          inClose[i] < patternLow))   /* close lower than the low of 3rd */
    {
      patternCount = 0;
    }
    NearPeriodTotal += ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0))) - ((Near_rangeType === 0) ? (Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : ((Near_rangeType === 1) ? (inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) : ((Near_rangeType === 2) ? ((inHigh[NearTrailingIdx - 2] - (((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inClose[NearTrailingIdx - 2]) : (inOpen[NearTrailingIdx - 2]))) + ((((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inOpen[NearTrailingIdx - 2]) : (inClose[NearTrailingIdx - 2])) - inLow[NearTrailingIdx - 2])) : 0.0)));
    NearTrailingIdx += 1;
    if (patternCount > 0) {
      patternCount -= 1;
    }
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first candle
   * - second candle: candle with range less than first candle and close near the bottom (near the top)
   * - third candle: lower high and higher low than 2nd
   * - fourth candle: lower high and lower low (higher high and higher low) than 3rd
   * Confirmation could come in the next 3 days with:
   * - a day that closes higher than the high (lower than the low) of the 3rd candle
   */
  outIdx = 0;
  do {
    if (inHigh[i - 2] < inHigh[i - 3] &&
        inLow[i - 2] > inLow[i - 3] &&   /* 2nd: lower high and higher low than 1st */
        inHigh[i - 1] < inHigh[i - 2] &&
        inLow[i - 1] > inLow[i - 2] &&   /* 3rd: lower high and higher low than 2nd */
        (inHigh[i] < inHigh[i - 1] &&
          inLow[i] < inLow[i - 1] &&     /* (bull) 4th: lower high and lower low */
          inClose[i - 2] <= inLow[i - 2] + ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) || /* (bull) 2nd: close near the low */
         inHigh[i] > inHigh[i - 1] &&
          inLow[i] > inLow[i - 1] &&     /* (bear) 4th: higher high and higher low */
          inClose[i - 2] >= inHigh[i - 2] - ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))))) /* (bull) 2nd: close near the top */
    {
      patternResult = ((100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : -1)) | 0);
      patternHigh = inHigh[i - 1];
      patternLow = inLow[i - 1];
      patternCount = 4;
      outInteger[outIdx++] = patternResult;
    } else if (patternCount > 0 &&
        (patternResult > 0 &&         /* search for confirmation if modified hikkake was no more than 3 bars ago */
          inClose[i] > patternHigh || /* close higher than the high of 3rd */
         patternResult < 0 &&
          inClose[i] < patternLow))   /* close lower than the low of 3rd */
    {
      outInteger[outIdx++] = patternResult + ((100 * ((patternResult > 0) ? 1 : -1)) | 0);
      patternCount = 0;
    } else {
      outInteger[outIdx++] = 0;
    }
    NearPeriodTotal += ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0))) - ((Near_rangeType === 0) ? (Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : ((Near_rangeType === 1) ? (inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) : ((Near_rangeType === 2) ? ((inHigh[NearTrailingIdx - 2] - (((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inClose[NearTrailingIdx - 2]) : (inOpen[NearTrailingIdx - 2]))) + ((((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inOpen[NearTrailingIdx - 2]) : (inClose[NearTrailingIdx - 2])) - inLow[NearTrailingIdx - 2])) : 0.0)));
    NearTrailingIdx += 1;
    if (patternCount > 0) {
      patternCount -= 1;
    }
    i += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlhikkakemodPositional(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDLHIKKAKEMOD', 'startIdx', startIdx);
  endIdx = requireInt32('CDLHIKKAKEMOD', 'endIdx', endIdx);
  requireIndexRange('CDLHIKKAKEMOD', startIdx, endIdx);
  const guardStart = clampedStart('CDLHIKKAKEMOD', startIdx, cdlhikkakemodLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLHIKKAKEMOD', 'prices.open', inOpen, guardInLen);
  requireLength('CDLHIKKAKEMOD', 'prices.high', inHigh, guardInLen);
  requireLength('CDLHIKKAKEMOD', 'prices.low', inLow, guardInLen);
  requireLength('CDLHIKKAKEMOD', 'prices.close', inClose, guardInLen);
  requireOutput('CDLHIKKAKEMOD', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLHIKKAKEMOD', 'outInteger', 'prices.open');
  }
  if (overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLHIKKAKEMOD', 'outInteger', 'prices.high');
  }
  if (overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLHIKKAKEMOD', 'outInteger', 'prices.low');
  }
  if (overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLHIKKAKEMOD', 'outInteger', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdlhikkakemodImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLHIKKAKEMOD', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cdlhikkakemod}, {@link cdlhikkakemodLookback},
 * {@link cdlhikkakemodDisplayShift}, {@link cdlhikkakemodOpen} and
 * {@link cdlhikkakemodOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface CdlhikkakemodOptions {
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
 * What {@link cdlhikkakemod} returns, and {@link cdlhikkakemodOpenAndFill} with
 * its stream.
 */
export interface CdlhikkakemodResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * +100 bullish hikkake bar, -100 bearish; +200 confirmed bullish, -200 confirmed
   * bearish; 0 otherwise. `values[k]` is the value at input bar `begIdx + k`.
   */
  values: Int32Array;
}

/**
 * Number of leading input bars {@link cdlhikkakemod} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlhikkakemodOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlhikkakemodLookback(options?: CdlhikkakemodOptions): number {
  const o = optionsOf('CDLHIKKAKEMOD lookback', options);
  return cdlhikkakemodLookbackPositional(coreOf('CDLHIKKAKEMOD lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdlhikkakemod}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlhikkakemodOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlhikkakemodDisplayShift(outputIdx: number, options?: CdlhikkakemodOptions): number {
  const o = optionsOf('CDLHIKKAKEMOD displayShift', options);
  return cdlhikkakemodDisplayShiftPositional(outputIdx, coreOf('CDLHIKKAKEMOD displayShift', o.core));
}

/**
 * A four-candle pattern: two successively narrower inside bars, then a breakout
 * bar, with the second candle closing near one extreme of its range. Bullish or
 * bearish reversal signal. Bullish (+) or bearish (-) reversal; per the code's
 * note it is significant in a downtrend (bull) or uptrend (bear), context the code
 * does not verify.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlhikkakemod](https://ta-lib.org/functions/cdlhikkakemod).
 *
 * **Notes**
 * - Does not verify the prior trend (downtrend for bullish, uptrend for bearish)
 *   that this reversal pattern assumes.
 * - A bar that is both a new setup and the confirmation of an earlier one reports
 *   the new setup; the earlier one ends unconfirmed.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cdlhikkakemodLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `open`: Open
 *   price of each bar. `high`: High price of each bar. `low`: Low price of each bar.
 *   `close`: Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link CdlhikkakemodOptions}.
 * @returns `begIdx` and the values; see {@link CdlhikkakemodResult}. Each output
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
 * import { cdlhikkakemod } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { begIdx, values } = cdlhikkakemod({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`cdlhikkake`](https://ta-lib.org/functions/cdlhikkake)
 */
export function cdlhikkakemod(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: CdlhikkakemodOptions): CdlhikkakemodResult {
  requireArgument('CDLHIKKAKEMOD', 'prices', prices);
  const o = optionsOf('CDLHIKKAKEMOD', options);
  if (o.endIdx === undefined) {
    requireSeries('CDLHIKKAKEMOD', 'prices.open', prices.open);
    requireSeries('CDLHIKKAKEMOD', 'prices.high', prices.high);
    requireSeries('CDLHIKKAKEMOD', 'prices.low', prices.low);
    requireSeries('CDLHIKKAKEMOD', 'prices.close', prices.close);
  }
  const core = coreOf('CDLHIKKAKEMOD', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cdlhikkakemodLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const range = cdlhikkakemodPositional(startIdx, endIdx, prices.open, prices.high, prices.low, prices.close, outInteger, core);
  return { begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CDLHIKKAKEMOD stream: one value per closed bar, bit-identical to
 * {@link cdlhikkakemod} over the same series. Open it with
 * {@link cdlhikkakemodOpen} or {@link cdlhikkakemodOpenAndFill}; there is nothing
 * to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlhikkakemod](https://ta-lib.org/functions/cdlhikkakemod).
 */
export class CdlhikkakemodStream {
  /** @internal */
  core: Core;
  /** @internal */
  NearPeriodTotal: number = 0;
  /** @internal */
  patternResult: number = 0;
  /** @internal */
  patternCount: number = 0;
  /** @internal */
  patternHigh: number = 0;
  /** @internal */
  patternLow: number = 0;
  /** @internal */
  lag1_inOpen: number = 0;
  /** @internal */
  lag2_inOpen: number = 0;
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
  lag3_inLow: number = 0;
  /** @internal */
  lag1_inClose: number = 0;
  /** @internal */
  lag2_inClose: number = 0;
  /** @internal */
  ringPos_NearTrailingIdx: number = 0;
  /** @internal */
  ringCap_NearTrailingIdx: number = 3;
  /** @internal */
  ringLag_NearTrailingIdx: number = 0;
  /** @internal */
  ring_NearTrailingIdx_derived: Float64Array = new Float64Array(3);
  /** @internal */
  cs_Near_rangeType: number = 0;
  /** @internal */
  cs_Near_avgPeriod: number = 0;
  /** @internal */
  cs_Near_factor: number = 0;
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
  static of(core: Core): CdlhikkakemodStream {
    return new CdlhikkakemodStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cdlhikkakemod} reports them over the same bars. Each accepted `update`
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
      throw failure('CDLHIKKAKEMOD advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw failure('CDLHIKKAKEMOD update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inOpen) || !Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('CDLHIKKAKEMOD update', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    cdlhikkakemodStepImpl(this, inOpen, inHigh, inLow, inClose);
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
      throw nonFiniteBar('CDLHIKKAKEMOD peek', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let cur_outInteger = 0;
    let patternCount = sp.patternCount;
    let patternHigh = sp.patternHigh;
    let patternLow = sp.patternLow;
    let patternResult = sp.patternResult;
    const Near_rangeType = sp.cs_Near_rangeType;
    const Near_avgPeriod = sp.cs_Near_avgPeriod;
    const Near_factor = sp.cs_Near_factor;
    if (sp.lag2_inHigh < sp.lag3_inHigh &&
        sp.lag2_inLow > sp.lag3_inLow &&   /* 2nd: lower high and higher low than 1st */
        sp.lag1_inHigh < sp.lag2_inHigh &&
        sp.lag1_inLow > sp.lag2_inLow &&   /* 3rd: lower high and higher low than 2nd */
        (inHigh < sp.lag1_inHigh &&
          inLow < sp.lag1_inLow &&         /* (bull) 4th: lower high and lower low */
          sp.lag2_inClose <= sp.lag2_inLow + ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) || /* (bull) 2nd: close near the low */
         inHigh > sp.lag1_inHigh &&
          inLow > sp.lag1_inLow &&         /* (bear) 4th: higher high and higher low */
          sp.lag2_inClose >= sp.lag2_inHigh - ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))))) /* (bull) 2nd: close near the top */
    {
      patternResult = ((100 * ((inHigh < sp.lag1_inHigh) ? 1 : -1)) | 0);
      patternHigh = sp.lag1_inHigh;
      patternLow = sp.lag1_inLow;
      patternCount = 4;
      cur_outInteger = patternResult;
    } else if (patternCount > 0 &&
        (patternResult > 0 &&      /* search for confirmation if modified hikkake was no more than 3 bars ago */
          inClose > patternHigh || /* close higher than the high of 3rd */
         patternResult < 0 &&
          inClose < patternLow))   /* close lower than the low of 3rd */
    {
      cur_outInteger = patternResult + ((100 * ((patternResult > 0) ? 1 : -1)) | 0);
      patternCount = 0;
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
  clone(): CdlhikkakemodStream {
    const c = new CdlhikkakemodStream(this.core);
    c.NearPeriodTotal = this.NearPeriodTotal;
    c.patternResult = this.patternResult;
    c.patternCount = this.patternCount;
    c.patternHigh = this.patternHigh;
    c.patternLow = this.patternLow;
    c.lag1_inOpen = this.lag1_inOpen;
    c.lag2_inOpen = this.lag2_inOpen;
    c.lag1_inHigh = this.lag1_inHigh;
    c.lag2_inHigh = this.lag2_inHigh;
    c.lag3_inHigh = this.lag3_inHigh;
    c.lag1_inLow = this.lag1_inLow;
    c.lag2_inLow = this.lag2_inLow;
    c.lag3_inLow = this.lag3_inLow;
    c.lag1_inClose = this.lag1_inClose;
    c.lag2_inClose = this.lag2_inClose;
    c.ringPos_NearTrailingIdx = this.ringPos_NearTrailingIdx;
    c.ringCap_NearTrailingIdx = this.ringCap_NearTrailingIdx;
    c.ringLag_NearTrailingIdx = this.ringLag_NearTrailingIdx;
    c.ring_NearTrailingIdx_derived = this.ring_NearTrailingIdx_derived.slice();
    c.cs_Near_rangeType = this.cs_Near_rangeType;
    c.cs_Near_avgPeriod = this.cs_Near_avgPeriod;
    c.cs_Near_factor = this.cs_Near_factor;
    c.cur_outInteger = this.cur_outInteger;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cdlhikkakemodStepImpl(sp: CdlhikkakemodStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  const Near_rangeType = sp.cs_Near_rangeType;
  const Near_avgPeriod = sp.cs_Near_avgPeriod;
  const Near_factor = sp.cs_Near_factor;
  sp.ring_NearTrailingIdx_derived[sp.ringPos_NearTrailingIdx] = ((Near_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((Near_rangeType === 1) ? (inHigh - inLow) : ((Near_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  if (sp.lag2_inHigh < sp.lag3_inHigh &&
      sp.lag2_inLow > sp.lag3_inLow &&   /* 2nd: lower high and higher low than 1st */
      sp.lag1_inHigh < sp.lag2_inHigh &&
      sp.lag1_inLow > sp.lag2_inLow &&   /* 3rd: lower high and higher low than 2nd */
      (inHigh < sp.lag1_inHigh &&
        inLow < sp.lag1_inLow &&         /* (bull) 4th: lower high and lower low */
        sp.lag2_inClose <= sp.lag2_inLow + ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) || /* (bull) 2nd: close near the low */
       inHigh > sp.lag1_inHigh &&
        inLow > sp.lag1_inLow &&         /* (bear) 4th: higher high and higher low */
        sp.lag2_inClose >= sp.lag2_inHigh - ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))))) /* (bull) 2nd: close near the top */
  {
    sp.patternResult = ((100 * ((inHigh < sp.lag1_inHigh) ? 1 : -1)) | 0);
    sp.patternHigh = sp.lag1_inHigh;
    sp.patternLow = sp.lag1_inLow;
    sp.patternCount = 4;
    sp.cur_outInteger = sp.patternResult;
  } else if (sp.patternCount > 0 &&
      (sp.patternResult > 0 &&      /* search for confirmation if modified hikkake was no more than 3 bars ago */
        inClose > sp.patternHigh || /* close higher than the high of 3rd */
       sp.patternResult < 0 &&
        inClose < sp.patternLow))   /* close lower than the low of 3rd */
  {
    sp.cur_outInteger = sp.patternResult + ((100 * ((sp.patternResult > 0) ? 1 : -1)) | 0);
    sp.patternCount = 0;
  } else {
    sp.cur_outInteger = 0;
  }
  sp.NearPeriodTotal += ((Near_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0))) - sp.ring_NearTrailingIdx_derived[(((sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - 2) % sp.ringCap_NearTrailingIdx) | 0)];
  if (sp.patternCount > 0) {
    sp.patternCount -= 1;
  }
  sp.lag2_inOpen = sp.lag1_inOpen;
  sp.lag1_inOpen = inOpen;
  sp.lag3_inHigh = sp.lag2_inHigh;
  sp.lag2_inHigh = sp.lag1_inHigh;
  sp.lag1_inHigh = inHigh;
  sp.lag3_inLow = sp.lag2_inLow;
  sp.lag2_inLow = sp.lag1_inLow;
  sp.lag1_inLow = inLow;
  sp.lag2_inClose = sp.lag1_inClose;
  sp.lag1_inClose = inClose;
  sp.ringPos_NearTrailingIdx = sp.ringPos_NearTrailingIdx + 1;
  if (sp.ringPos_NearTrailingIdx >= sp.ringCap_NearTrailingIdx) {
    sp.ringPos_NearTrailingIdx = 0;
  }
}

function cdlhikkakemodOpenImpl(
  sp: CdlhikkakemodStream,
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
  let NearPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let NearTrailingIdx = 0;
  let lookbackTotal = 0;
  let patternResult = 0;
  let patternCount = 0;
  let patternHigh = 0;
  let patternLow = 0;
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
  const Near_rangeType: number = core.candleSettings[CandleSettingType.NEAR].rangeType;
  const Near_avgPeriod = core.candleSettings[CandleSettingType.NEAR].avgPeriod;
  const Near_factor = core.candleSettings[CandleSettingType.NEAR].factor;
  /* Confirmation window countdown (replaces the absolute patternIdx guard)
   * and a cache of the 3rd candle's high/low (replaces inHigh/inLow
   * [patternIdx-1]) so nothing in the per-bar logic references the cursor.
   */
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlhikkakemodLookbackPositional(core);
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
  NearPeriodTotal = 0;
  NearTrailingIdx = startIdx - 3 - Near_avgPeriod;
  i = NearTrailingIdx;
  while (i < startIdx - 3) {
    NearPeriodTotal += ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
    i += 1;
  }
  patternCount = 0;
  patternResult = 0;
  patternHigh = 0.0;
  patternLow = 0.0;
  i = startIdx - 3;
  while (i < startIdx) {
    /* copy here the pattern recognition code below */
    if (inHigh[i - 2] < inHigh[i - 3] &&
        inLow[i - 2] > inLow[i - 3] &&   /* 2nd: lower high and higher low than 1st */
        inHigh[i - 1] < inHigh[i - 2] &&
        inLow[i - 1] > inLow[i - 2] &&   /* 3rd: lower high and higher low than 2nd */
        (inHigh[i] < inHigh[i - 1] &&
          inLow[i] < inLow[i - 1] &&     /* (bull) 4th: lower high and lower low */
          inClose[i - 2] <= inLow[i - 2] + ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) || /* (bull) 2nd: close near the low */
         inHigh[i] > inHigh[i - 1] &&
          inLow[i] > inLow[i - 1] &&     /* (bear) 4th: higher high and higher low */
          inClose[i - 2] >= inHigh[i - 2] - ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))))) /* (bull) 2nd: close near the top */
    {
      patternResult = ((100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : -1)) | 0);
      patternHigh = inHigh[i - 1];
      patternLow = inLow[i - 1];
      patternCount = 4;
    } else if (patternCount > 0 &&
        (patternResult > 0 &&         /* search for confirmation if modified hikkake was no more than 3 bars ago */
          inClose[i] > patternHigh || /* close higher than the high of 3rd */
         patternResult < 0 &&
          inClose[i] < patternLow))   /* close lower than the low of 3rd */
    {
      patternCount = 0;
    }
    NearPeriodTotal += ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0))) - ((Near_rangeType === 0) ? (Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : ((Near_rangeType === 1) ? (inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) : ((Near_rangeType === 2) ? ((inHigh[NearTrailingIdx - 2] - (((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inClose[NearTrailingIdx - 2]) : (inOpen[NearTrailingIdx - 2]))) + ((((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inOpen[NearTrailingIdx - 2]) : (inClose[NearTrailingIdx - 2])) - inLow[NearTrailingIdx - 2])) : 0.0)));
    NearTrailingIdx += 1;
    if (patternCount > 0) {
      patternCount -= 1;
    }
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first candle
   * - second candle: candle with range less than first candle and close near the bottom (near the top)
   * - third candle: lower high and higher low than 2nd
   * - fourth candle: lower high and lower low (higher high and higher low) than 3rd
   * Confirmation could come in the next 3 days with:
   * - a day that closes higher than the high (lower than the low) of the 3rd candle
   */
  outIdx = 0;
  do {
    if (inHigh[i - 2] < inHigh[i - 3] &&
        inLow[i - 2] > inLow[i - 3] &&   /* 2nd: lower high and higher low than 1st */
        inHigh[i - 1] < inHigh[i - 2] &&
        inLow[i - 1] > inLow[i - 2] &&   /* 3rd: lower high and higher low than 2nd */
        (inHigh[i] < inHigh[i - 1] &&
          inLow[i] < inLow[i - 1] &&     /* (bull) 4th: lower high and lower low */
          inClose[i - 2] <= inLow[i - 2] + ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) || /* (bull) 2nd: close near the low */
         inHigh[i] > inHigh[i - 1] &&
          inLow[i] > inLow[i - 1] &&     /* (bear) 4th: higher high and higher low */
          inClose[i - 2] >= inHigh[i - 2] - ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))))) /* (bull) 2nd: close near the top */
    {
      patternResult = ((100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : -1)) | 0);
      patternHigh = inHigh[i - 1];
      patternLow = inLow[i - 1];
      patternCount = 4;
      outInteger[((outIdx++ * outStride) | 0)] = patternResult;
    } else if (patternCount > 0 &&
        (patternResult > 0 &&         /* search for confirmation if modified hikkake was no more than 3 bars ago */
          inClose[i] > patternHigh || /* close higher than the high of 3rd */
         patternResult < 0 &&
          inClose[i] < patternLow))   /* close lower than the low of 3rd */
    {
      outInteger[((outIdx++ * outStride) | 0)] = patternResult + ((100 * ((patternResult > 0) ? 1 : -1)) | 0);
      patternCount = 0;
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    NearPeriodTotal += ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0))) - ((Near_rangeType === 0) ? (Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : ((Near_rangeType === 1) ? (inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) : ((Near_rangeType === 2) ? ((inHigh[NearTrailingIdx - 2] - (((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inClose[NearTrailingIdx - 2]) : (inOpen[NearTrailingIdx - 2]))) + ((((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inOpen[NearTrailingIdx - 2]) : (inClose[NearTrailingIdx - 2])) - inLow[NearTrailingIdx - 2])) : 0.0)));
    NearTrailingIdx += 1;
    if (patternCount > 0) {
      patternCount -= 1;
    }
    i += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const capLag_NearTrailingIdx = i - NearTrailingIdx;
  const cap_NearTrailingIdx = capLag_NearTrailingIdx + 3;
  if (capLag_NearTrailingIdx < 0 || cap_NearTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_NearTrailingIdx = cap_NearTrailingIdx > 0 ? cap_NearTrailingIdx : 1;
  const capRing_NearTrailingIdx_derived = new Float64Array(allocN_NearTrailingIdx);
  for (let fillJ = historyLen - cap_NearTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_NearTrailingIdx_derived[fillJ % cap_NearTrailingIdx] = ((Near_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((Near_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((Near_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  sp.NearPeriodTotal = NearPeriodTotal;
  sp.patternResult = patternResult;
  sp.patternCount = patternCount;
  sp.patternHigh = patternHigh;
  sp.patternLow = patternLow;
  sp.lag1_inOpen = inOpen[historyLen - 1];
  sp.lag2_inOpen = inOpen[historyLen - 2];
  sp.lag1_inHigh = inHigh[historyLen - 1];
  sp.lag2_inHigh = inHigh[historyLen - 2];
  sp.lag3_inHigh = inHigh[historyLen - 3];
  sp.lag1_inLow = inLow[historyLen - 1];
  sp.lag2_inLow = inLow[historyLen - 2];
  sp.lag3_inLow = inLow[historyLen - 3];
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.lag2_inClose = inClose[historyLen - 2];
  sp.ringPos_NearTrailingIdx = historyLen % cap_NearTrailingIdx;
  sp.ringCap_NearTrailingIdx = cap_NearTrailingIdx;
  sp.ringLag_NearTrailingIdx = capLag_NearTrailingIdx;
  sp.ring_NearTrailingIdx_derived = capRing_NearTrailingIdx_derived;
  sp.cs_Near_rangeType = Near_rangeType;
  sp.cs_Near_avgPeriod = Near_avgPeriod;
  sp.cs_Near_factor = Near_factor;
  sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlhikkakemodOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): CdlhikkakemodStream {
  const sp = CdlhikkakemodStream.of(core);
  const retCode = cdlhikkakemodOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLHIKKAKEMOD openAndFill', inOpen.length, startIdx, cdlhikkakemodLookbackPositional(core));
  }
  throw streamFailure('CDLHIKKAKEMOD openAndFill', retCode);
}

/** @internal */
export function cdlhikkakemodOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): CdlhikkakemodStream {
  const sp = CdlhikkakemodStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdlhikkakemodOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLHIKKAKEMOD open', inOpen.length, startIdx, cdlhikkakemodLookbackPositional(core));
  }
  throw streamFailure('CDLHIKKAKEMOD open', retCode);
}

/** @internal */
export function cdlhikkakemodOpenPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): CdlhikkakemodStream {
  requireArgument('CDLHIKKAKEMOD open', 'prices.open', inOpen);
  requireHistory('CDLHIKKAKEMOD open', inOpen.length);
  requireArgument('CDLHIKKAKEMOD open', 'prices.high', inHigh);
  requireArgument('CDLHIKKAKEMOD open', 'prices.low', inLow);
  requireArgument('CDLHIKKAKEMOD open', 'prices.close', inClose);
  requireHistoryLength('CDLHIKKAKEMOD open', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLHIKKAKEMOD open', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLHIKKAKEMOD open', 'prices.close', inClose.length, inOpen.length);
  return cdlhikkakemodOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/** @internal */
export function cdlhikkakemodOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): CdlhikkakemodStream {
  requireArgument('CDLHIKKAKEMOD openAndFill', 'prices.open', inOpen);
  requireHistory('CDLHIKKAKEMOD openAndFill', inOpen.length);
  requireArgument('CDLHIKKAKEMOD openAndFill', 'prices.high', inHigh);
  requireArgument('CDLHIKKAKEMOD openAndFill', 'prices.low', inLow);
  requireArgument('CDLHIKKAKEMOD openAndFill', 'prices.close', inClose);
  const guardOutLen = openFillCount('CDLHIKKAKEMOD openAndFill', inOpen.length, cdlhikkakemodLookbackPositional(core));
  requireHistoryLength('CDLHIKKAKEMOD openAndFill', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLHIKKAKEMOD openAndFill', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLHIKKAKEMOD openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('CDLHIKKAKEMOD openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('CDLHIKKAKEMOD openAndFill', 'outInteger', 'prices.open');
  }
  if (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CDLHIKKAKEMOD openAndFill', 'outInteger', 'prices.high');
  }
  if (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CDLHIKKAKEMOD openAndFill', 'outInteger', 'prices.low');
  }
  if (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('CDLHIKKAKEMOD openAndFill', 'outInteger', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLHIKKAKEMOD openAndFill', 'outInteger', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLHIKKAKEMOD openAndFill', 'outInteger', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLHIKKAKEMOD openAndFill', 'outInteger', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLHIKKAKEMOD openAndFill', 'outInteger', 'prices.close');
  }
  return cdlhikkakemodOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}

/**
 * Opens a live CDLHIKKAKEMOD stream over the warm-up history: `value()` starts at
 * the last history bar, bit-identical to {@link cdlhikkakemod} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `open`: Open price of each bar. `high`: High price of each bar. `low`:
 *   Low price of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlhikkakemodOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlhikkakemodLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input or histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdlhikkakemodOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdlhikkakemodOpen({ open, high, low, close });
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdlhikkakemodOpen(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: CdlhikkakemodOptions): CdlhikkakemodStream {
  requireArgument('CDLHIKKAKEMOD open', 'prices', prices);
  const o = optionsOf('CDLHIKKAKEMOD open', options);
  const core = coreOf('CDLHIKKAKEMOD open', o.core);
  return cdlhikkakemodOpenPositional(prices.open, prices.high, prices.low, prices.close, core);
}

/**
 * {@link cdlhikkakemodOpen} that also computes the outputs over the whole history
 * in the same pass, bit-identically to {@link cdlhikkakemod}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `open`: Open price of each bar. `high`: High price of each bar. `low`:
 *   Low price of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CdlhikkakemodOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cdlhikkakemod} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlhikkakemodLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, histories of different lengths or a buffer too short,
 *   of the wrong type or starting where another buffer or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdlhikkakemodOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = cdlhikkakemodOpenAndFill({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cdlhikkakemodOpenAndFill(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: CdlhikkakemodOptions): CdlhikkakemodResult & { stream: CdlhikkakemodStream } {
  requireArgument('CDLHIKKAKEMOD openAndFill', 'prices', prices);
  const o = optionsOf('CDLHIKKAKEMOD openAndFill', options);
  const core = coreOf('CDLHIKKAKEMOD openAndFill', o.core);
  const capacity = fillCapacity(cdlhikkakemodLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const stream = cdlhikkakemodOpenAndFillPositional(prices.open, prices.high, prices.low, prices.close, outInteger, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}
