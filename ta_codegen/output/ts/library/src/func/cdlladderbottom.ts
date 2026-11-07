/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AC       Angelo Ciceri
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  032005 AC     Creation
 *  041305 MF     Minor modification for a compiler warning
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 *  103126 MF,CC  Nest the cheap, selective pre-check conjuncts in an outer if
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { CandleSettingType, Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function cdlladderbottomLookbackPositional(core: Core = Core.DEFAULT): number {
  const ShadowVeryShort_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_VERY_SHORT].avgPeriod;
  return ShadowVeryShort_avgPeriod + 4;
}

/** @internal */
export function cdlladderbottomDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdlladderbottomImpl(
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
  let ShadowVeryShortPeriodTotal = 0;
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
  lookbackTotal = cdlladderbottomLookbackPositional(core);
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
  ShadowVeryShortPeriodTotal = 0;
  ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
  i = ShadowVeryShortTrailingIdx;
  while (i < startIdx) {
    ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - three black candlesticks with consecutively lower opens and closes
   * - fourth candle: black candle with an upper shadow (it's supposed to be not very short)
   * - fifth candle: white candle that opens above prior candle's body and closes above prior candle's high
   * The meaning of "very short" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) === -1 &&
        ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : -1) === -1 &&
        ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === -1 && /* 3 black candlesticks */
        inOpen[i - 4] > inOpen[i - 3] &&
        inOpen[i - 3] > inOpen[i - 2] &&                       /* with consecutively lower opens */
        inClose[i - 4] > inClose[i - 3] &&
        inClose[i - 3] > inClose[i - 2] &&                     /* and closes */
        ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === -1 && /* 4th: black with an upper shadow */
        (inHigh[i - 1] - ((inClose[i - 1] >= inOpen[i - 1]) ? inClose[i - 1] : inOpen[i - 1])) > ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) &&
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === 1 &&          /* 5th: white */
        inOpen[i] > inOpen[i - 1] &&                           /* that opens above prior candle's body */
        inClose[i] > inHigh[i - 1])                            /* and closes above prior candle's high */
    {
      outInteger[outIdx++] = 100;
    } else {
      outInteger[outIdx++] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx - 1] - inOpen[ShadowVeryShortTrailingIdx - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[ShadowVeryShortTrailingIdx - 1] - inLow[ShadowVeryShortTrailingIdx - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[ShadowVeryShortTrailingIdx - 1] - (((inClose[ShadowVeryShortTrailingIdx - 1]) >= (inOpen[ShadowVeryShortTrailingIdx - 1])) ? (inClose[ShadowVeryShortTrailingIdx - 1]) : (inOpen[ShadowVeryShortTrailingIdx - 1]))) + ((((inClose[ShadowVeryShortTrailingIdx - 1]) >= (inOpen[ShadowVeryShortTrailingIdx - 1])) ? (inOpen[ShadowVeryShortTrailingIdx - 1]) : (inClose[ShadowVeryShortTrailingIdx - 1])) - inLow[ShadowVeryShortTrailingIdx - 1])) : 0.0)));
    i += 1;
    ShadowVeryShortTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlladderbottomPositional(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDLLADDERBOTTOM', 'startIdx', startIdx);
  endIdx = requireInt32('CDLLADDERBOTTOM', 'endIdx', endIdx);
  requireIndexRange('CDLLADDERBOTTOM', startIdx, endIdx);
  const guardStart = clampedStart('CDLLADDERBOTTOM', startIdx, cdlladderbottomLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLLADDERBOTTOM', 'prices.open', inOpen, guardInLen);
  requireLength('CDLLADDERBOTTOM', 'prices.high', inHigh, guardInLen);
  requireLength('CDLLADDERBOTTOM', 'prices.low', inLow, guardInLen);
  requireLength('CDLLADDERBOTTOM', 'prices.close', inClose, guardInLen);
  requireOutput('CDLLADDERBOTTOM', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLLADDERBOTTOM', 'outInteger', 'prices.open');
  }
  if (overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLLADDERBOTTOM', 'outInteger', 'prices.high');
  }
  if (overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLLADDERBOTTOM', 'outInteger', 'prices.low');
  }
  if (overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLLADDERBOTTOM', 'outInteger', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdlladderbottomImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLLADDERBOTTOM', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cdlladderbottom}, {@link cdlladderbottomLookback},
 * {@link cdlladderbottomDisplayShift}, {@link cdlladderbottomOpen} and
 * {@link cdlladderbottomOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface CdlladderbottomOptions {
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
 * What {@link cdlladderbottom} returns, and {@link cdlladderbottomOpenAndFill}
 * with its stream.
 */
export interface CdlladderbottomResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * +100 on a detected ladder bottom, 0 otherwise. Only ever emits +100 (never
   * -100); inherently bullish. `values[k]` is the value at input bar `begIdx + k`.
   */
  values: Int32Array;
}

/**
 * Number of leading input bars {@link cdlladderbottom} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlladderbottomOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlladderbottomLookback(options?: CdlladderbottomOptions): number {
  const o = optionsOf('CDLLADDERBOTTOM lookback', options);
  return cdlladderbottomLookbackPositional(coreOf('CDLLADDERBOTTOM lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdlladderbottom}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlladderbottomOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlladderbottomDisplayShift(outputIdx: number, options?: CdlladderbottomOptions): number {
  const o = optionsOf('CDLLADDERBOTTOM displayShift', options);
  return cdlladderbottomDisplayShiftPositional(outputIdx, coreOf('CDLLADDERBOTTOM displayShift', o.core));
}

/**
 * Five-candle bullish reversal pattern: three consecutively lower black candles, a
 * fourth black candle with a non-very-short upper shadow, then a white candle that
 * opens above the prior open and closes above the prior high. A hit is a bullish
 * reversal signal, most meaningful after a downtrend.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlladderbottom](https://ta-lib.org/functions/cdlladderbottom).
 *
 * **Notes**
 * - Does not verify the preceding downtrend that this bullish reversal classically
 *   assumes.
 * - Bulkowski's testing found this reverses a downtrend only 56% of the time —
 *   "near random" — and it is extremely rare (451 occurrences out of 4.7 million
 *   candle lines), ranking 41st of 103 patterns for overall performance.
 *   ([thepatternsite.com](https://thepatternsite.com/LadderBottom.html))
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cdlladderbottomLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `open`: Open
 *   price of each bar. `high`: High price of each bar. `low`: Low price of each bar.
 *   `close`: Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link CdlladderbottomOptions}.
 * @returns `begIdx` and the values; see {@link CdlladderbottomResult}. Each output
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
 * import { cdlladderbottom } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { begIdx, values } = cdlladderbottom({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`cdl3blackcrows`](https://ta-lib.org/functions/cdl3blackcrows)
 * @see [`cdlmatchinglow`](https://ta-lib.org/functions/cdlmatchinglow)
 * @see [`cdlbreakaway`](https://ta-lib.org/functions/cdlbreakaway)
 */
export function cdlladderbottom(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: CdlladderbottomOptions): CdlladderbottomResult {
  requireArgument('CDLLADDERBOTTOM', 'prices', prices);
  const o = optionsOf('CDLLADDERBOTTOM', options);
  if (o.endIdx === undefined) {
    requireSeries('CDLLADDERBOTTOM', 'prices.open', prices.open);
    requireSeries('CDLLADDERBOTTOM', 'prices.high', prices.high);
    requireSeries('CDLLADDERBOTTOM', 'prices.low', prices.low);
    requireSeries('CDLLADDERBOTTOM', 'prices.close', prices.close);
  }
  const core = coreOf('CDLLADDERBOTTOM', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cdlladderbottomLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const range = cdlladderbottomPositional(startIdx, endIdx, prices.open, prices.high, prices.low, prices.close, outInteger, core);
  return { begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CDLLADDERBOTTOM stream: one value per closed bar, bit-identical to
 * {@link cdlladderbottom} over the same series. Open it with
 * {@link cdlladderbottomOpen} or {@link cdlladderbottomOpenAndFill}; there is
 * nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlladderbottom](https://ta-lib.org/functions/cdlladderbottom).
 */
export class CdlladderbottomStream {
  /** @internal */
  core: Core;
  /** @internal */
  ShadowVeryShortPeriodTotal: number = 0;
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
  lag1_inLow: number = 0;
  /** @internal */
  lag1_inClose: number = 0;
  /** @internal */
  lag2_inClose: number = 0;
  /** @internal */
  lag3_inClose: number = 0;
  /** @internal */
  lag4_inClose: number = 0;
  /** @internal */
  ringPos_ShadowVeryShortTrailingIdx: number = 0;
  /** @internal */
  ringCap_ShadowVeryShortTrailingIdx: number = 2;
  /** @internal */
  ringLag_ShadowVeryShortTrailingIdx: number = 0;
  /** @internal */
  ring_ShadowVeryShortTrailingIdx_derived: Float64Array = new Float64Array(2);
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
  static of(core: Core): CdlladderbottomStream {
    return new CdlladderbottomStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cdlladderbottom} reports them over the same bars. Each accepted `update`
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
      throw streamFailure('CDLLADDERBOTTOM advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw streamFailure('CDLLADDERBOTTOM update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(open) || !Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('CDLLADDERBOTTOM update', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    cdlladderbottomStepImpl(this, open, high, low, close);
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
      throw nonFiniteBar('CDLLADDERBOTTOM peek', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inOpen = open;
    const inClose = close;
    let cur_outInteger = 0;
    const ShadowVeryShort_rangeType = sp.cs_ShadowVeryShort_rangeType;
    const ShadowVeryShort_avgPeriod = sp.cs_ShadowVeryShort_avgPeriod;
    const ShadowVeryShort_factor = sp.cs_ShadowVeryShort_factor;
    if (((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) === -1 &&
        ((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : -1) === -1 &&
        ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === -1 && /* 3 black candlesticks */
        sp.lag4_inOpen > sp.lag3_inOpen &&
        sp.lag3_inOpen > sp.lag2_inOpen &&                       /* with consecutively lower opens */
        sp.lag4_inClose > sp.lag3_inClose &&
        sp.lag3_inClose > sp.lag2_inClose &&                     /* and closes */
        ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === -1 && /* 4th: black with an upper shadow */
        (sp.lag1_inHigh - ((sp.lag1_inClose >= sp.lag1_inOpen) ? sp.lag1_inClose : sp.lag1_inOpen)) > ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (sp.ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((ShadowVeryShort_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) &&
        ((inClose >= inOpen) ? 1 : -1) === 1 &&                  /* 5th: white */
        inOpen > sp.lag1_inOpen &&                               /* that opens above prior candle's body */
        inClose > sp.lag1_inHigh)                                /* and closes above prior candle's high */
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
  clone(): CdlladderbottomStream {
    const c = new CdlladderbottomStream(this.core);
    c.ShadowVeryShortPeriodTotal = this.ShadowVeryShortPeriodTotal;
    c.lag1_inOpen = this.lag1_inOpen;
    c.lag2_inOpen = this.lag2_inOpen;
    c.lag3_inOpen = this.lag3_inOpen;
    c.lag4_inOpen = this.lag4_inOpen;
    c.lag1_inHigh = this.lag1_inHigh;
    c.lag1_inLow = this.lag1_inLow;
    c.lag1_inClose = this.lag1_inClose;
    c.lag2_inClose = this.lag2_inClose;
    c.lag3_inClose = this.lag3_inClose;
    c.lag4_inClose = this.lag4_inClose;
    c.ringPos_ShadowVeryShortTrailingIdx = this.ringPos_ShadowVeryShortTrailingIdx;
    c.ringCap_ShadowVeryShortTrailingIdx = this.ringCap_ShadowVeryShortTrailingIdx;
    c.ringLag_ShadowVeryShortTrailingIdx = this.ringLag_ShadowVeryShortTrailingIdx;
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

function cdlladderbottomStepImpl(sp: CdlladderbottomStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  const ShadowVeryShort_rangeType = sp.cs_ShadowVeryShort_rangeType;
  const ShadowVeryShort_avgPeriod = sp.cs_ShadowVeryShort_avgPeriod;
  const ShadowVeryShort_factor = sp.cs_ShadowVeryShort_factor;
  sp.ring_ShadowVeryShortTrailingIdx_derived[sp.ringPos_ShadowVeryShortTrailingIdx] = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  if (((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) === -1 &&
      ((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : -1) === -1 &&
      ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === -1 && /* 3 black candlesticks */
      sp.lag4_inOpen > sp.lag3_inOpen &&
      sp.lag3_inOpen > sp.lag2_inOpen &&                       /* with consecutively lower opens */
      sp.lag4_inClose > sp.lag3_inClose &&
      sp.lag3_inClose > sp.lag2_inClose &&                     /* and closes */
      ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === -1 && /* 4th: black with an upper shadow */
      (sp.lag1_inHigh - ((sp.lag1_inClose >= sp.lag1_inOpen) ? sp.lag1_inClose : sp.lag1_inOpen)) > ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (sp.ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((ShadowVeryShort_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) &&
      ((inClose >= inOpen) ? 1 : -1) === 1 &&                  /* 5th: white */
      inOpen > sp.lag1_inOpen &&                               /* that opens above prior candle's body */
      inClose > sp.lag1_inHigh)                                /* and closes above prior candle's high */
  {
    sp.cur_outInteger = 100;
  } else {
    sp.cur_outInteger = 0;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  sp.ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((ShadowVeryShort_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((ShadowVeryShort_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0))) - sp.ring_ShadowVeryShortTrailingIdx_derived[(((sp.ringPos_ShadowVeryShortTrailingIdx + sp.ringCap_ShadowVeryShortTrailingIdx - sp.ringLag_ShadowVeryShortTrailingIdx - 1) % sp.ringCap_ShadowVeryShortTrailingIdx) | 0)];
  sp.lag4_inOpen = sp.lag3_inOpen;
  sp.lag3_inOpen = sp.lag2_inOpen;
  sp.lag2_inOpen = sp.lag1_inOpen;
  sp.lag1_inOpen = inOpen;
  sp.lag1_inHigh = inHigh;
  sp.lag1_inLow = inLow;
  sp.lag4_inClose = sp.lag3_inClose;
  sp.lag3_inClose = sp.lag2_inClose;
  sp.lag2_inClose = sp.lag1_inClose;
  sp.lag1_inClose = inClose;
  sp.ringPos_ShadowVeryShortTrailingIdx = sp.ringPos_ShadowVeryShortTrailingIdx + 1;
  if (sp.ringPos_ShadowVeryShortTrailingIdx >= sp.ringCap_ShadowVeryShortTrailingIdx) {
    sp.ringPos_ShadowVeryShortTrailingIdx = 0;
  }
}

function cdlladderbottomOpenImpl(
  sp: CdlladderbottomStream,
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
  let ShadowVeryShortPeriodTotal = 0;
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
  lookbackTotal = cdlladderbottomLookbackPositional(core);
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
  ShadowVeryShortPeriodTotal = 0;
  ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
  i = ShadowVeryShortTrailingIdx;
  while (i < startIdx) {
    ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - three black candlesticks with consecutively lower opens and closes
   * - fourth candle: black candle with an upper shadow (it's supposed to be not very short)
   * - fifth candle: white candle that opens above prior candle's body and closes above prior candle's high
   * The meaning of "very short" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) === -1 &&
        ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : -1) === -1 &&
        ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === -1 && /* 3 black candlesticks */
        inOpen[i - 4] > inOpen[i - 3] &&
        inOpen[i - 3] > inOpen[i - 2] &&                       /* with consecutively lower opens */
        inClose[i - 4] > inClose[i - 3] &&
        inClose[i - 3] > inClose[i - 2] &&                     /* and closes */
        ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === -1 && /* 4th: black with an upper shadow */
        (inHigh[i - 1] - ((inClose[i - 1] >= inOpen[i - 1]) ? inClose[i - 1] : inOpen[i - 1])) > ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod !== 0) ? (ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((ShadowVeryShort_rangeType === 2) ? 2.0 : 1.0)))) &&
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === 1 &&          /* 5th: white */
        inOpen[i] > inOpen[i - 1] &&                           /* that opens above prior candle's body */
        inClose[i] > inHigh[i - 1])                            /* and closes above prior candle's high */
    {
      outInteger[((outIdx++ * outStride) | 0)] = 100;
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx - 1] - inOpen[ShadowVeryShortTrailingIdx - 1])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[ShadowVeryShortTrailingIdx - 1] - inLow[ShadowVeryShortTrailingIdx - 1]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[ShadowVeryShortTrailingIdx - 1] - (((inClose[ShadowVeryShortTrailingIdx - 1]) >= (inOpen[ShadowVeryShortTrailingIdx - 1])) ? (inClose[ShadowVeryShortTrailingIdx - 1]) : (inOpen[ShadowVeryShortTrailingIdx - 1]))) + ((((inClose[ShadowVeryShortTrailingIdx - 1]) >= (inOpen[ShadowVeryShortTrailingIdx - 1])) ? (inOpen[ShadowVeryShortTrailingIdx - 1]) : (inClose[ShadowVeryShortTrailingIdx - 1])) - inLow[ShadowVeryShortTrailingIdx - 1])) : 0.0)));
    i += 1;
    ShadowVeryShortTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const capLag_ShadowVeryShortTrailingIdx = i - ShadowVeryShortTrailingIdx;
  const cap_ShadowVeryShortTrailingIdx = capLag_ShadowVeryShortTrailingIdx + 2;
  if (capLag_ShadowVeryShortTrailingIdx < 0 || cap_ShadowVeryShortTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_ShadowVeryShortTrailingIdx = cap_ShadowVeryShortTrailingIdx > 0 ? cap_ShadowVeryShortTrailingIdx : 1;
  const capRing_ShadowVeryShortTrailingIdx_derived = new Float64Array(allocN_ShadowVeryShortTrailingIdx);
  for (let fillJ = historyLen - cap_ShadowVeryShortTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_ShadowVeryShortTrailingIdx_derived[fillJ % cap_ShadowVeryShortTrailingIdx] = ((ShadowVeryShort_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((ShadowVeryShort_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((ShadowVeryShort_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  sp.ShadowVeryShortPeriodTotal = ShadowVeryShortPeriodTotal;
  sp.lag1_inOpen = inOpen[historyLen - 1];
  sp.lag2_inOpen = inOpen[historyLen - 2];
  sp.lag3_inOpen = inOpen[historyLen - 3];
  sp.lag4_inOpen = inOpen[historyLen - 4];
  sp.lag1_inHigh = inHigh[historyLen - 1];
  sp.lag1_inLow = inLow[historyLen - 1];
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.lag2_inClose = inClose[historyLen - 2];
  sp.lag3_inClose = inClose[historyLen - 3];
  sp.lag4_inClose = inClose[historyLen - 4];
  sp.ringPos_ShadowVeryShortTrailingIdx = historyLen % cap_ShadowVeryShortTrailingIdx;
  sp.ringCap_ShadowVeryShortTrailingIdx = cap_ShadowVeryShortTrailingIdx;
  sp.ringLag_ShadowVeryShortTrailingIdx = capLag_ShadowVeryShortTrailingIdx;
  sp.ring_ShadowVeryShortTrailingIdx_derived = capRing_ShadowVeryShortTrailingIdx_derived;
  sp.cs_ShadowVeryShort_rangeType = ShadowVeryShort_rangeType;
  sp.cs_ShadowVeryShort_avgPeriod = ShadowVeryShort_avgPeriod;
  sp.cs_ShadowVeryShort_factor = ShadowVeryShort_factor;
  sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlladderbottomOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): CdlladderbottomStream {
  const sp = CdlladderbottomStream.of(core);
  const retCode = cdlladderbottomOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLLADDERBOTTOM openAndFill', inOpen.length, startIdx, cdlladderbottomLookbackPositional(core));
  }
  throw streamFailure('CDLLADDERBOTTOM openAndFill', retCode);
}

/** @internal */
export function cdlladderbottomOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): CdlladderbottomStream {
  const sp = CdlladderbottomStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdlladderbottomOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLLADDERBOTTOM open', inOpen.length, startIdx, cdlladderbottomLookbackPositional(core));
  }
  throw streamFailure('CDLLADDERBOTTOM open', retCode);
}

/** @internal */
export function cdlladderbottomOpenPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): CdlladderbottomStream {
  requireArgument('CDLLADDERBOTTOM open', 'prices.open', inOpen);
  requireHistory('CDLLADDERBOTTOM open', 'prices.open', inOpen.length);
  requireArgument('CDLLADDERBOTTOM open', 'prices.high', inHigh);
  requireArgument('CDLLADDERBOTTOM open', 'prices.low', inLow);
  requireArgument('CDLLADDERBOTTOM open', 'prices.close', inClose);
  requireHistoryLength('CDLLADDERBOTTOM open', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLLADDERBOTTOM open', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLLADDERBOTTOM open', 'prices.close', inClose.length, inOpen.length);
  return cdlladderbottomOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/** @internal */
export function cdlladderbottomOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): CdlladderbottomStream {
  requireArgument('CDLLADDERBOTTOM openAndFill', 'prices.open', inOpen);
  requireHistory('CDLLADDERBOTTOM openAndFill', 'prices.open', inOpen.length);
  requireArgument('CDLLADDERBOTTOM openAndFill', 'prices.high', inHigh);
  requireArgument('CDLLADDERBOTTOM openAndFill', 'prices.low', inLow);
  requireArgument('CDLLADDERBOTTOM openAndFill', 'prices.close', inClose);
  const guardOutLen = openFillCount('CDLLADDERBOTTOM openAndFill', inOpen.length, cdlladderbottomLookbackPositional(core));
  requireHistoryLength('CDLLADDERBOTTOM openAndFill', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLLADDERBOTTOM openAndFill', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLLADDERBOTTOM openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('CDLLADDERBOTTOM openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('CDLLADDERBOTTOM openAndFill', 'outInteger', 'prices.open');
  }
  if (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CDLLADDERBOTTOM openAndFill', 'outInteger', 'prices.high');
  }
  if (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CDLLADDERBOTTOM openAndFill', 'outInteger', 'prices.low');
  }
  if (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('CDLLADDERBOTTOM openAndFill', 'outInteger', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLLADDERBOTTOM openAndFill', 'outInteger', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLLADDERBOTTOM openAndFill', 'outInteger', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLLADDERBOTTOM openAndFill', 'outInteger', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLLADDERBOTTOM openAndFill', 'outInteger', 'prices.close');
  }
  return cdlladderbottomOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}

/**
 * Opens a live CDLLADDERBOTTOM stream over the warm-up history: `value()` starts
 * at the last history bar, bit-identical to {@link cdlladderbottom} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `open`: Open price of each bar. `high`: High price of each bar. `low`:
 *   Low price of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlladderbottomOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlladderbottomLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input or histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdlladderbottomOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdlladderbottomOpen({ open, high, low, close });
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdlladderbottomOpen(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: CdlladderbottomOptions): CdlladderbottomStream {
  requireArgument('CDLLADDERBOTTOM open', 'prices', prices);
  const o = optionsOf('CDLLADDERBOTTOM open', options);
  const core = coreOf('CDLLADDERBOTTOM open', o.core);
  return cdlladderbottomOpenPositional(prices.open, prices.high, prices.low, prices.close, core);
}

/**
 * {@link cdlladderbottomOpen} that also computes the outputs over the whole
 * history in the same pass, bit-identically to {@link cdlladderbottom}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `open`: Open price of each bar. `high`: High price of each bar. `low`:
 *   Low price of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CdlladderbottomOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cdlladderbottom} returns them; the range is
 *   the stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlladderbottomLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, histories of different lengths or a buffer too short,
 *   of the wrong type or starting where another buffer or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdlladderbottomOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = cdlladderbottomOpenAndFill({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cdlladderbottomOpenAndFill(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: CdlladderbottomOptions): CdlladderbottomResult & { stream: CdlladderbottomStream } {
  requireArgument('CDLLADDERBOTTOM openAndFill', 'prices', prices);
  const o = optionsOf('CDLLADDERBOTTOM openAndFill', options);
  const core = coreOf('CDLLADDERBOTTOM openAndFill', o.core);
  const capacity = fillCapacity(cdlladderbottomLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const stream = cdlladderbottomOpenAndFillPositional(prices.open, prices.high, prices.low, prices.close, outInteger, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}
