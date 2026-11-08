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
 *  011605 AC     Creation
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 *  103126 MF,CC  Write the two gaps as an else-if chain of pre-checks
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { CandleSettingType, Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function cdltasukigapLookbackPositional(core: Core = Core.DEFAULT): number {
  const Near_avgPeriod = core.candleSettings[CandleSettingType.NEAR].avgPeriod;
  return Near_avgPeriod + 2;
}

/** @internal */
export function cdltasukigapDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdltasukigapImpl(
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
  const Near_rangeType: number = core.candleSettings[CandleSettingType.NEAR].rangeType;
  const Near_avgPeriod = core.candleSettings[CandleSettingType.NEAR].avgPeriod;
  const Near_factor = core.candleSettings[CandleSettingType.NEAR].factor;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdltasukigapLookbackPositional(core);
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
  NearTrailingIdx = startIdx - Near_avgPeriod;
  i = NearTrailingIdx;
  while (i < startIdx) {
    NearPeriodTotal += ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - upside (downside) gap
   * - first candle after the window: white (black) candlestick
   * - second candle: black (white) candlestick that opens within the previous real body and closes under (above)
   *   the previous real body inside the gap
   * - the size of two real bodies should be near the same
   * The meaning of "near" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (((inOpen[i - 1] < inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1]) > (inOpen[i - 2] > inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2])) && /* upside gap */
        ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === 1 && /* 1st: white */
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === -1)          /* 2nd: black */
    {
      if (inOpen[i] < inClose[i - 1] &&
          inOpen[i] > inOpen[i - 1] &&  /* that opens within the white rb */
          inClose[i] < inOpen[i - 1] && /* and closes under the white rb */
          inClose[i] > (inClose[i - 2] > inOpen[i - 2] ? inClose[i - 2] : inOpen[i - 2]) && /* inside the gap */
          Math.abs(Math.abs(inClose[i - 1] - inOpen[i - 1]) - Math.abs(inClose[i] - inOpen[i])) < ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0))))) /* size of 2 rb near the same */
      {
        outInteger[outIdx++] = ((((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) * 100) | 0);
      } else {
        outInteger[outIdx++] = 0;
      }
    } else if (((inOpen[i - 1] > inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1]) < (inOpen[i - 2] < inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2])) && /* downside gap */
        ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === -1 && /* 1st: black */
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === 1)            /* 2nd: white */
    {
      if (inOpen[i] < inOpen[i - 1] &&
          inOpen[i] > inClose[i - 1] && /* that opens within the black rb */
          inClose[i] > inOpen[i - 1] && /* and closes above the black rb */
          inClose[i] < (inClose[i - 2] < inOpen[i - 2] ? inClose[i - 2] : inOpen[i - 2]) && /* inside the gap */
          Math.abs(Math.abs(inClose[i - 1] - inOpen[i - 1]) - Math.abs(inClose[i] - inOpen[i])) < ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0))))) /* size of 2 rb near the same */
      {
        outInteger[outIdx++] = ((((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) * 100) | 0);
      } else {
        outInteger[outIdx++] = 0;
      }
    } else {
      outInteger[outIdx++] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    NearPeriodTotal += ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((Near_rangeType === 0) ? (Math.abs(inClose[NearTrailingIdx - 1] - inOpen[NearTrailingIdx - 1])) : ((Near_rangeType === 1) ? (inHigh[NearTrailingIdx - 1] - inLow[NearTrailingIdx - 1]) : ((Near_rangeType === 2) ? ((inHigh[NearTrailingIdx - 1] - (((inClose[NearTrailingIdx - 1]) >= (inOpen[NearTrailingIdx - 1])) ? (inClose[NearTrailingIdx - 1]) : (inOpen[NearTrailingIdx - 1]))) + ((((inClose[NearTrailingIdx - 1]) >= (inOpen[NearTrailingIdx - 1])) ? (inOpen[NearTrailingIdx - 1]) : (inClose[NearTrailingIdx - 1])) - inLow[NearTrailingIdx - 1])) : 0.0)));
    i += 1;
    NearTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function cdltasukigapPositional(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDLTASUKIGAP', 'startIdx', startIdx);
  endIdx = requireInt32('CDLTASUKIGAP', 'endIdx', endIdx);
  requireIndexRange('CDLTASUKIGAP', startIdx, endIdx);
  const guardStart = clampedStart('CDLTASUKIGAP', startIdx, cdltasukigapLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLTASUKIGAP', 'prices.open', inOpen, guardInLen);
  requireLength('CDLTASUKIGAP', 'prices.high', inHigh, guardInLen);
  requireLength('CDLTASUKIGAP', 'prices.low', inLow, guardInLen);
  requireLength('CDLTASUKIGAP', 'prices.close', inClose, guardInLen);
  requireOutput('CDLTASUKIGAP', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLTASUKIGAP', 'outInteger', 'prices.open');
  }
  if (overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLTASUKIGAP', 'outInteger', 'prices.high');
  }
  if (overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLTASUKIGAP', 'outInteger', 'prices.low');
  }
  if (overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLTASUKIGAP', 'outInteger', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdltasukigapImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLTASUKIGAP', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cdltasukigap}, {@link cdltasukigapLookback},
 * {@link cdltasukigapDisplayShift}, {@link cdltasukigapOpen} and
 * {@link cdltasukigapOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface CdltasukigapOptions {
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
 * What {@link cdltasukigap} returns, and {@link cdltasukigapOpenAndFill} with its
 * stream.
 */
export interface CdltasukigapResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * +100 on a bullish (upside-gap) tasuki gap, -100 on a bearish (downside-gap)
   * tasuki gap, 0 otherwise. Sign equals the color of the gap candle i-1
   * (candlecolor(i-1)*100). `values[k]` is the value at input bar `begIdx + k`.
   */
  values: Int32Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link cdltasukigap} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdltasukigapOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdltasukigapLookback(options?: CdltasukigapOptions): number {
  const o = optionsOf('CDLTASUKIGAP lookback', options);
  return cdltasukigapLookbackPositional(coreOf('CDLTASUKIGAP lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdltasukigap}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdltasukigapOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdltasukigapDisplayShift(outputIdx: number, options?: CdltasukigapOptions): number {
  const o = optionsOf('CDLTASUKIGAP displayShift', options);
  return cdltasukigapDisplayShiftPositional(outputIdx, coreOf('CDLTASUKIGAP displayShift', o.core));
}

/**
 * A three-candle pattern: a real-body-gapping candle followed by an opposite-color
 * candle that opens inside its body and closes back into the gap without filling
 * it. An upside gap is a bullish continuation signal; a downside gap is a bearish
 * continuation signal.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdltasukigap](https://ta-lib.org/functions/cdltasukigap).
 *
 * **Notes**
 * - This continuation pattern does not verify the prior trend it classically
 *   assumes; the caller must confirm the trend.
 * - Bulkowski's testing found the downside Tasuki Gap actually acts as a bullish
 *   REVERSAL 54% of the time — opposite its textbook bearish-continuation label —
 *   while the upside variant does continue as labeled, but only 57% of the time
 *   ("near random").
 *   ([thepatternsite.com](https://thepatternsite.com/DownsideTasukiGap.html))
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cdltasukigapLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `open`: Open price of
 *   each bar. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link CdltasukigapOptions}.
 * @returns `begIdx` and the values; see {@link CdltasukigapResult}. Each output is
 *   a view of its buffer, the caller's or a new one.
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
 * import { cdltasukigap } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { begIdx, values } = cdltasukigap({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`cdlgapsidesidewhite`](https://ta-lib.org/functions/cdlgapsidesidewhite)
 * @see [`cdlxsidegap3methods`](https://ta-lib.org/functions/cdlxsidegap3methods)
 */
export function cdltasukigap(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdltasukigapOptions): CdltasukigapResult {
  requireArgument('CDLTASUKIGAP', 'prices', prices);
  const o = optionsOf('CDLTASUKIGAP', options);
  if (o.endIdx === undefined) {
    requireSeries('CDLTASUKIGAP', 'prices.open', prices.open);
    requireSeries('CDLTASUKIGAP', 'prices.high', prices.high);
    requireSeries('CDLTASUKIGAP', 'prices.low', prices.low);
    requireSeries('CDLTASUKIGAP', 'prices.close', prices.close);
  }
  const core = coreOf('CDLTASUKIGAP', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cdltasukigapLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const range = cdltasukigapPositional(startIdx, endIdx, prices.open, prices.high, prices.low, prices.close, outInteger, core);
  return { begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CDLTASUKIGAP stream: one value per closed bar, bit-identical to
 * {@link cdltasukigap} over the same series. Open it with {@link cdltasukigapOpen}
 * or {@link cdltasukigapOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdltasukigap](https://ta-lib.org/functions/cdltasukigap).
 */
export class CdltasukigapStream {
  /** @internal */
  core: Core;
  /** @internal */
  NearPeriodTotal: number = 0;
  /** @internal */
  lag1_inOpen: number = 0;
  /** @internal */
  lag2_inOpen: number = 0;
  /** @internal */
  lag1_inHigh: number = 0;
  /** @internal */
  lag1_inLow: number = 0;
  /** @internal */
  lag1_inClose: number = 0;
  /** @internal */
  lag2_inClose: number = 0;
  /** @internal */
  ringPos_NearTrailingIdx: number = 0;
  /** @internal */
  ringCap_NearTrailingIdx: number = 2;
  /** @internal */
  ringLag_NearTrailingIdx: number = 0;
  /** @internal */
  ring_NearTrailingIdx_derived: Float64Array = new Float64Array(2);
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
  static of(core: Core): CdltasukigapStream {
    return new CdltasukigapStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cdltasukigap} reports them over the same bars. Each accepted `update` and
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
      throw streamFailure('CDLTASUKIGAP advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw streamFailure('CDLTASUKIGAP update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(open) || !Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('CDLTASUKIGAP update', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    cdltasukigapStepImpl(this, open, high, low, close);
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
      throw nonFiniteBar('CDLTASUKIGAP peek', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inOpen = open;
    const inClose = close;
    let cur_outInteger = 0;
    const Near_rangeType = sp.cs_Near_rangeType;
    const Near_avgPeriod = sp.cs_Near_avgPeriod;
    const Near_factor = sp.cs_Near_factor;
    if (((sp.lag1_inOpen < sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose) > (sp.lag2_inOpen > sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose)) && /* upside gap */
        ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === 1 && /* 1st: white */
        ((inClose >= inOpen) ? 1 : -1) === -1)                  /* 2nd: black */
    {
      if (inOpen < sp.lag1_inClose &&
          inOpen > sp.lag1_inOpen &&  /* that opens within the white rb */
          inClose < sp.lag1_inOpen && /* and closes under the white rb */
          inClose > (sp.lag2_inClose > sp.lag2_inOpen ? sp.lag2_inClose : sp.lag2_inOpen) && /* inside the gap */
          Math.abs(Math.abs(sp.lag1_inClose - sp.lag1_inOpen) - Math.abs(inClose - inOpen)) < ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0))))) /* size of 2 rb near the same */
      {
        cur_outInteger = ((((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) * 100) | 0);
      } else {
        cur_outInteger = 0;
      }
    } else if (((sp.lag1_inOpen > sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose) < (sp.lag2_inOpen < sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose)) && /* downside gap */
        ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === -1 && /* 1st: black */
        ((inClose >= inOpen) ? 1 : -1) === 1)                    /* 2nd: white */
    {
      if (inOpen < sp.lag1_inOpen &&
          inOpen > sp.lag1_inClose && /* that opens within the black rb */
          inClose > sp.lag1_inOpen && /* and closes above the black rb */
          inClose < (sp.lag2_inClose < sp.lag2_inOpen ? sp.lag2_inClose : sp.lag2_inOpen) && /* inside the gap */
          Math.abs(Math.abs(sp.lag1_inClose - sp.lag1_inOpen) - Math.abs(inClose - inOpen)) < ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0))))) /* size of 2 rb near the same */
      {
        cur_outInteger = ((((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) * 100) | 0);
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
  clone(): CdltasukigapStream {
    const c = new CdltasukigapStream(this.core);
    c.NearPeriodTotal = this.NearPeriodTotal;
    c.lag1_inOpen = this.lag1_inOpen;
    c.lag2_inOpen = this.lag2_inOpen;
    c.lag1_inHigh = this.lag1_inHigh;
    c.lag1_inLow = this.lag1_inLow;
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

function cdltasukigapStepImpl(sp: CdltasukigapStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  const Near_rangeType = sp.cs_Near_rangeType;
  const Near_avgPeriod = sp.cs_Near_avgPeriod;
  const Near_factor = sp.cs_Near_factor;
  sp.ring_NearTrailingIdx_derived[sp.ringPos_NearTrailingIdx] = ((Near_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((Near_rangeType === 1) ? (inHigh - inLow) : ((Near_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  if (((sp.lag1_inOpen < sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose) > (sp.lag2_inOpen > sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose)) && /* upside gap */
      ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === 1 && /* 1st: white */
      ((inClose >= inOpen) ? 1 : -1) === -1)                  /* 2nd: black */
  {
    if (inOpen < sp.lag1_inClose &&
        inOpen > sp.lag1_inOpen &&  /* that opens within the white rb */
        inClose < sp.lag1_inOpen && /* and closes under the white rb */
        inClose > (sp.lag2_inClose > sp.lag2_inOpen ? sp.lag2_inClose : sp.lag2_inOpen) && /* inside the gap */
        Math.abs(Math.abs(sp.lag1_inClose - sp.lag1_inOpen) - Math.abs(inClose - inOpen)) < ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0))))) /* size of 2 rb near the same */
    {
      sp.cur_outInteger = ((((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) * 100) | 0);
    } else {
      sp.cur_outInteger = 0;
    }
  } else if (((sp.lag1_inOpen > sp.lag1_inClose ? sp.lag1_inOpen : sp.lag1_inClose) < (sp.lag2_inOpen < sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose)) && /* downside gap */
      ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === -1 && /* 1st: black */
      ((inClose >= inOpen) ? 1 : -1) === 1)                    /* 2nd: white */
  {
    if (inOpen < sp.lag1_inOpen &&
        inOpen > sp.lag1_inClose && /* that opens within the black rb */
        inClose > sp.lag1_inOpen && /* and closes above the black rb */
        inClose < (sp.lag2_inClose < sp.lag2_inOpen ? sp.lag2_inClose : sp.lag2_inOpen) && /* inside the gap */
        Math.abs(Math.abs(sp.lag1_inClose - sp.lag1_inOpen) - Math.abs(inClose - inOpen)) < ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0))))) /* size of 2 rb near the same */
    {
      sp.cur_outInteger = ((((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) * 100) | 0);
    } else {
      sp.cur_outInteger = 0;
    }
  } else {
    sp.cur_outInteger = 0;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  sp.NearPeriodTotal += ((Near_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0))) - sp.ring_NearTrailingIdx_derived[(((sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - 1) % sp.ringCap_NearTrailingIdx) | 0)];
  sp.lag2_inOpen = sp.lag1_inOpen;
  sp.lag1_inOpen = inOpen;
  sp.lag1_inHigh = inHigh;
  sp.lag1_inLow = inLow;
  sp.lag2_inClose = sp.lag1_inClose;
  sp.lag1_inClose = inClose;
  sp.ringPos_NearTrailingIdx = sp.ringPos_NearTrailingIdx + 1;
  if (sp.ringPos_NearTrailingIdx >= sp.ringCap_NearTrailingIdx) {
    sp.ringPos_NearTrailingIdx = 0;
  }
}

function cdltasukigapOpenImpl(
  sp: CdltasukigapStream,
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
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdltasukigapLookbackPositional(core);
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
  NearTrailingIdx = startIdx - Near_avgPeriod;
  i = NearTrailingIdx;
  while (i < startIdx) {
    NearPeriodTotal += ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - upside (downside) gap
   * - first candle after the window: white (black) candlestick
   * - second candle: black (white) candlestick that opens within the previous real body and closes under (above)
   *   the previous real body inside the gap
   * - the size of two real bodies should be near the same
   * The meaning of "near" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (((inOpen[i - 1] < inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1]) > (inOpen[i - 2] > inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2])) && /* upside gap */
        ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === 1 && /* 1st: white */
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === -1)          /* 2nd: black */
    {
      if (inOpen[i] < inClose[i - 1] &&
          inOpen[i] > inOpen[i - 1] &&  /* that opens within the white rb */
          inClose[i] < inOpen[i - 1] && /* and closes under the white rb */
          inClose[i] > (inClose[i - 2] > inOpen[i - 2] ? inClose[i - 2] : inOpen[i - 2]) && /* inside the gap */
          Math.abs(Math.abs(inClose[i - 1] - inOpen[i - 1]) - Math.abs(inClose[i] - inOpen[i])) < ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0))))) /* size of 2 rb near the same */
      {
        outInteger[((outIdx++ * outStride) | 0)] = ((((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) * 100) | 0);
      } else {
        outInteger[((outIdx++ * outStride) | 0)] = 0;
      }
    } else if (((inOpen[i - 1] > inClose[i - 1] ? inOpen[i - 1] : inClose[i - 1]) < (inOpen[i - 2] < inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2])) && /* downside gap */
        ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === -1 && /* 1st: black */
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === 1)            /* 2nd: white */
    {
      if (inOpen[i] < inOpen[i - 1] &&
          inOpen[i] > inClose[i - 1] && /* that opens within the black rb */
          inClose[i] > inOpen[i - 1] && /* and closes above the black rb */
          inClose[i] < (inClose[i - 2] < inOpen[i - 2] ? inClose[i - 2] : inOpen[i - 2]) && /* inside the gap */
          Math.abs(Math.abs(inClose[i - 1] - inOpen[i - 1]) - Math.abs(inClose[i] - inOpen[i])) < ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0))))) /* size of 2 rb near the same */
      {
        outInteger[((outIdx++ * outStride) | 0)] = ((((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) * 100) | 0);
      } else {
        outInteger[((outIdx++ * outStride) | 0)] = 0;
      }
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    NearPeriodTotal += ((Near_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((Near_rangeType === 0) ? (Math.abs(inClose[NearTrailingIdx - 1] - inOpen[NearTrailingIdx - 1])) : ((Near_rangeType === 1) ? (inHigh[NearTrailingIdx - 1] - inLow[NearTrailingIdx - 1]) : ((Near_rangeType === 2) ? ((inHigh[NearTrailingIdx - 1] - (((inClose[NearTrailingIdx - 1]) >= (inOpen[NearTrailingIdx - 1])) ? (inClose[NearTrailingIdx - 1]) : (inOpen[NearTrailingIdx - 1]))) + ((((inClose[NearTrailingIdx - 1]) >= (inOpen[NearTrailingIdx - 1])) ? (inOpen[NearTrailingIdx - 1]) : (inClose[NearTrailingIdx - 1])) - inLow[NearTrailingIdx - 1])) : 0.0)));
    i += 1;
    NearTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const capLag_NearTrailingIdx = i - NearTrailingIdx;
  const cap_NearTrailingIdx = capLag_NearTrailingIdx + 2;
  if (capLag_NearTrailingIdx < 0 || cap_NearTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_NearTrailingIdx = cap_NearTrailingIdx > 0 ? cap_NearTrailingIdx : 1;
  const capRing_NearTrailingIdx_derived = new Float64Array(allocN_NearTrailingIdx);
  for (let fillJ = historyLen - cap_NearTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_NearTrailingIdx_derived[fillJ % cap_NearTrailingIdx] = ((Near_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((Near_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((Near_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  sp.NearPeriodTotal = NearPeriodTotal;
  sp.lag1_inOpen = inOpen[historyLen - 1];
  sp.lag2_inOpen = inOpen[historyLen - 2];
  sp.lag1_inHigh = inHigh[historyLen - 1];
  sp.lag1_inLow = inLow[historyLen - 1];
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
export function cdltasukigapOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): CdltasukigapStream {
  const sp = CdltasukigapStream.of(core);
  const retCode = cdltasukigapOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLTASUKIGAP openAndFill', inOpen.length, startIdx, cdltasukigapLookbackPositional(core));
  }
  throw streamFailure('CDLTASUKIGAP openAndFill', retCode);
}

/** @internal */
export function cdltasukigapOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): CdltasukigapStream {
  const sp = CdltasukigapStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdltasukigapOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLTASUKIGAP open', inOpen.length, startIdx, cdltasukigapLookbackPositional(core));
  }
  throw streamFailure('CDLTASUKIGAP open', retCode);
}

/** @internal */
export function cdltasukigapOpenPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): CdltasukigapStream {
  requireArgument('CDLTASUKIGAP open', 'prices.open', inOpen);
  requireHistory('CDLTASUKIGAP open', 'prices.open', inOpen.length);
  requireArgument('CDLTASUKIGAP open', 'prices.high', inHigh);
  requireArgument('CDLTASUKIGAP open', 'prices.low', inLow);
  requireArgument('CDLTASUKIGAP open', 'prices.close', inClose);
  requireHistoryLength('CDLTASUKIGAP open', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLTASUKIGAP open', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLTASUKIGAP open', 'prices.close', inClose.length, inOpen.length);
  return cdltasukigapOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/** @internal */
export function cdltasukigapOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): CdltasukigapStream {
  requireArgument('CDLTASUKIGAP openAndFill', 'prices.open', inOpen);
  requireHistory('CDLTASUKIGAP openAndFill', 'prices.open', inOpen.length);
  requireArgument('CDLTASUKIGAP openAndFill', 'prices.high', inHigh);
  requireArgument('CDLTASUKIGAP openAndFill', 'prices.low', inLow);
  requireArgument('CDLTASUKIGAP openAndFill', 'prices.close', inClose);
  const guardOutLen = openFillCount('CDLTASUKIGAP openAndFill', inOpen.length, cdltasukigapLookbackPositional(core));
  requireHistoryLength('CDLTASUKIGAP openAndFill', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLTASUKIGAP openAndFill', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLTASUKIGAP openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('CDLTASUKIGAP openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('CDLTASUKIGAP openAndFill', 'outInteger', 'prices.open');
  }
  if (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CDLTASUKIGAP openAndFill', 'outInteger', 'prices.high');
  }
  if (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CDLTASUKIGAP openAndFill', 'outInteger', 'prices.low');
  }
  if (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('CDLTASUKIGAP openAndFill', 'outInteger', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLTASUKIGAP openAndFill', 'outInteger', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLTASUKIGAP openAndFill', 'outInteger', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLTASUKIGAP openAndFill', 'outInteger', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLTASUKIGAP openAndFill', 'outInteger', 'prices.close');
  }
  return cdltasukigapOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}

/**
 * Opens a live CDLTASUKIGAP stream over the warm-up history: `value()` starts at
 * the last history bar, bit-identical to {@link cdltasukigap} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdltasukigapOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdltasukigapLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input or histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdltasukigapOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdltasukigapOpen({ open, high, low, close });
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdltasukigapOpen(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdltasukigapOptions): CdltasukigapStream {
  requireArgument('CDLTASUKIGAP open', 'prices', prices);
  const o = optionsOf('CDLTASUKIGAP open', options);
  const core = coreOf('CDLTASUKIGAP open', o.core);
  return cdltasukigapOpenPositional(prices.open, prices.high, prices.low, prices.close, core);
}

/**
 * {@link cdltasukigapOpen} that also computes the outputs over the whole history
 * in the same pass, bit-identically to {@link cdltasukigap}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CdltasukigapOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cdltasukigap} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdltasukigapLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, histories of different lengths or a buffer too short,
 *   of the wrong type or starting where another buffer or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdltasukigapOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = cdltasukigapOpenAndFill({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cdltasukigapOpenAndFill(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdltasukigapOptions): CdltasukigapResult & { stream: CdltasukigapStream } {
  requireArgument('CDLTASUKIGAP openAndFill', 'prices', prices);
  const o = optionsOf('CDLTASUKIGAP openAndFill', options);
  const core = coreOf('CDLTASUKIGAP openAndFill', o.core);
  const capacity = fillCapacity(cdltasukigapLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const stream = cdltasukigapOpenAndFillPositional(prices.open, prices.high, prices.low, prices.close, outInteger, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}
