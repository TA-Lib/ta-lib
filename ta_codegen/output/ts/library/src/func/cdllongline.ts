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
 *  071704 AC     Creation
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { CandleSettingType, Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link cdllongline} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function cdllonglineLookback(core: Core = Core.DEFAULT): number {
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  const ShadowShort_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_SHORT].avgPeriod;
  return (BodyLong_avgPeriod > ShadowShort_avgPeriod ? BodyLong_avgPeriod : ShadowShort_avgPeriod);
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdllongline}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function cdllonglineDisplayShift(outputIdx: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdllonglineImpl(
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
  let ShadowPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let BodyTrailingIdx = 0;
  let ShadowTrailingIdx = 0;
  let lookbackTotal = 0;
  const BodyLong_rangeType: number = core.candleSettings[CandleSettingType.BODY_LONG].rangeType;
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  const BodyLong_factor = core.candleSettings[CandleSettingType.BODY_LONG].factor;
  const ShadowShort_rangeType: number = core.candleSettings[CandleSettingType.SHADOW_SHORT].rangeType;
  const ShadowShort_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_SHORT].avgPeriod;
  const ShadowShort_factor = core.candleSettings[CandleSettingType.SHADOW_SHORT].factor;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdllonglineLookback(core);
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
  BodyTrailingIdx = startIdx - BodyLong_avgPeriod;
  ShadowPeriodTotal = 0;
  ShadowTrailingIdx = startIdx - ShadowShort_avgPeriod;
  i = BodyTrailingIdx;
  while (i < startIdx) {
    BodyPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = ShadowTrailingIdx;
  while (i < startIdx) {
    ShadowPeriodTotal += ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - long real body
   * - short upper and lower shadow
   * The meaning of "long" and "short" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (Math.abs(inClose[i] - inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) < ((ShadowShort_factor * (((ShadowShort_avgPeriod !== 0) ? (ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowShort_rangeType === 2) ? 2.0 : 1.0)))) && (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) < ((ShadowShort_factor * (((ShadowShort_avgPeriod !== 0) ? (ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowShort_rangeType === 2) ? 2.0 : 1.0))))) {
      outInteger[outIdx++] = ((((inClose[i] >= inOpen[i]) ? 1 : -1) * 100) | 0);
    } else {
      outInteger[outIdx++] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyTrailingIdx] - (((inClose[BodyTrailingIdx]) >= (inOpen[BodyTrailingIdx])) ? (inClose[BodyTrailingIdx]) : (inOpen[BodyTrailingIdx]))) + ((((inClose[BodyTrailingIdx]) >= (inOpen[BodyTrailingIdx])) ? (inOpen[BodyTrailingIdx]) : (inClose[BodyTrailingIdx])) - inLow[BodyTrailingIdx])) : 0.0)));
    ShadowPeriodTotal += ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[ShadowTrailingIdx] - inOpen[ShadowTrailingIdx])) : ((ShadowShort_rangeType === 1) ? (inHigh[ShadowTrailingIdx] - inLow[ShadowTrailingIdx]) : ((ShadowShort_rangeType === 2) ? ((inHigh[ShadowTrailingIdx] - (((inClose[ShadowTrailingIdx]) >= (inOpen[ShadowTrailingIdx])) ? (inClose[ShadowTrailingIdx]) : (inOpen[ShadowTrailingIdx]))) + ((((inClose[ShadowTrailingIdx]) >= (inOpen[ShadowTrailingIdx])) ? (inOpen[ShadowTrailingIdx]) : (inClose[ShadowTrailingIdx])) - inLow[ShadowTrailingIdx])) : 0.0)));
    i += 1;
    BodyTrailingIdx += 1;
    ShadowTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/**
 * A single-candle pattern: a long real body with short upper and short lower
 * shadow. The signal direction follows the candle color (bullish if white, bearish
 * if black). Signals strong directional conviction on the bar. Not intrinsically a
 * reversal or continuation signal.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdllongline](https://ta-lib.org/functions/cdllongline).
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link cdllonglineLookback} is a success
 * with `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inOpen - Open price of each bar.
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param outInteger - +100 on a white (close>=open) long line, -100 on a black
 *   long line, 0 when no pattern. Must hold the values produced and never be empty;
 *   the input length always suffices.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The range written: `begIdx` is the first bar with a value, `count` how
 *   many were written.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for an argument that is not an int32, an
 *   array too short for the range, an empty output or an output sharing memory with
 *   a `Float64Array` input it is not. Checked before anything is written.
 *
 * @example
 * ```ts
 * import { cdllongline } from 'ta-lib';
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
 * const { begIdx, count } = cdllongline(
 *   0, open.length - 1,
 *   open, high, low, close,
 *   out,
 * );
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`cdlshortline`](https://ta-lib.org/functions/cdlshortline)
 * @see [`cdlclosingmarubozu`](https://ta-lib.org/functions/cdlclosingmarubozu)
 * @see [`cdlmarubozu`](https://ta-lib.org/functions/cdlmarubozu)
 * @see [`cdllongleggeddoji`](https://ta-lib.org/functions/cdllongleggeddoji)
 */
export function cdllongline(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDLLONGLINE', 'startIdx', startIdx);
  endIdx = requireInt32('CDLLONGLINE', 'endIdx', endIdx);
  requireIndexRange('CDLLONGLINE', startIdx, endIdx);
  const guardStart = clampedStart('CDLLONGLINE', startIdx, cdllonglineLookback(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLLONGLINE', 'inOpen', inOpen, guardInLen);
  requireLength('CDLLONGLINE', 'inHigh', inHigh, guardInLen);
  requireLength('CDLLONGLINE', 'inLow', inLow, guardInLen);
  requireLength('CDLLONGLINE', 'inClose', inClose, guardInLen);
  requireOutput('CDLLONGLINE', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64) || overlaps(outInteger, inHighF64) || overlaps(outInteger, inLowF64) || overlaps(outInteger, inCloseF64)) {
    throw failure('CDLLONGLINE', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdllonglineImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLLONGLINE', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live CDLLONGLINE stream: one value per closed bar, bit-identical to
 * {@link cdllongline} over the same series. Open it with {@link cdllonglineOpen}
 * or {@link cdllonglineOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdllongline](https://ta-lib.org/functions/cdllongline).
 */
export class CdllonglineStream {
  /** @internal */
  core: Core;
  /** @internal */
  BodyPeriodTotal: number = 0;
  /** @internal */
  ShadowPeriodTotal: number = 0;
  /** @internal */
  ringPos_BodyTrailingIdx: number = 0;
  /** @internal */
  ringCap_BodyTrailingIdx: number = 0;
  /** @internal */
  ring_BodyTrailingIdx_derived: Float64Array = new Float64Array(1);
  /** @internal */
  ringPos_ShadowTrailingIdx: number = 0;
  /** @internal */
  ringCap_ShadowTrailingIdx: number = 0;
  /** @internal */
  ring_ShadowTrailingIdx_derived: Float64Array = new Float64Array(1);
  /** @internal */
  cs_BodyLong_rangeType: number = 0;
  /** @internal */
  cs_BodyLong_avgPeriod: number = 0;
  /** @internal */
  cs_BodyLong_factor: number = 0;
  /** @internal */
  cs_ShadowShort_rangeType: number = 0;
  /** @internal */
  cs_ShadowShort_avgPeriod: number = 0;
  /** @internal */
  cs_ShadowShort_factor: number = 0;
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
  static of(core: Core): CdllonglineStream {
    return new CdllonglineStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cdllongline} reports them over the same bars. Each accepted `update` and
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
      throw failure('CDLLONGLINE advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw failure('CDLLONGLINE update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inOpen) || !Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('CDLLONGLINE update', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    cdllonglineStepImpl(this, inOpen, inHigh, inLow, inClose);
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
      throw nonFiniteBar('CDLLONGLINE peek', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let cur_outInteger = 0;
    const BodyLong_rangeType = sp.cs_BodyLong_rangeType;
    const BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
    const BodyLong_factor = sp.cs_BodyLong_factor;
    const ShadowShort_rangeType = sp.cs_ShadowShort_rangeType;
    const ShadowShort_avgPeriod = sp.cs_ShadowShort_avgPeriod;
    const ShadowShort_factor = sp.cs_ShadowShort_factor;
    if (Math.abs(inClose - inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType === 1) ? (inHigh - inLow) : ((BodyLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) < ((ShadowShort_factor * (((ShadowShort_avgPeriod !== 0) ? (sp.ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowShort_rangeType === 2) ? 2.0 : 1.0)))) && (((inClose >= inOpen) ? inOpen : inClose) - inLow) < ((ShadowShort_factor * (((ShadowShort_avgPeriod !== 0) ? (sp.ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowShort_rangeType === 2) ? 2.0 : 1.0))))) {
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
  clone(): CdllonglineStream {
    const c = new CdllonglineStream(this.core);
    c.BodyPeriodTotal = this.BodyPeriodTotal;
    c.ShadowPeriodTotal = this.ShadowPeriodTotal;
    c.ringPos_BodyTrailingIdx = this.ringPos_BodyTrailingIdx;
    c.ringCap_BodyTrailingIdx = this.ringCap_BodyTrailingIdx;
    c.ring_BodyTrailingIdx_derived = this.ring_BodyTrailingIdx_derived.slice();
    c.ringPos_ShadowTrailingIdx = this.ringPos_ShadowTrailingIdx;
    c.ringCap_ShadowTrailingIdx = this.ringCap_ShadowTrailingIdx;
    c.ring_ShadowTrailingIdx_derived = this.ring_ShadowTrailingIdx_derived.slice();
    c.cs_BodyLong_rangeType = this.cs_BodyLong_rangeType;
    c.cs_BodyLong_avgPeriod = this.cs_BodyLong_avgPeriod;
    c.cs_BodyLong_factor = this.cs_BodyLong_factor;
    c.cs_ShadowShort_rangeType = this.cs_ShadowShort_rangeType;
    c.cs_ShadowShort_avgPeriod = this.cs_ShadowShort_avgPeriod;
    c.cs_ShadowShort_factor = this.cs_ShadowShort_factor;
    c.cur_outInteger = this.cur_outInteger;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cdllonglineStepImpl(sp: CdllonglineStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  const BodyLong_rangeType = sp.cs_BodyLong_rangeType;
  const BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
  const BodyLong_factor = sp.cs_BodyLong_factor;
  const ShadowShort_rangeType = sp.cs_ShadowShort_rangeType;
  const ShadowShort_avgPeriod = sp.cs_ShadowShort_avgPeriod;
  const ShadowShort_factor = sp.cs_ShadowShort_factor;
  if (sp.ringCap_BodyTrailingIdx === 0) {
    sp.ring_BodyTrailingIdx_derived[0] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType === 1) ? (inHigh - inLow) : ((BodyLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
  if (sp.ringCap_ShadowTrailingIdx === 0) {
    sp.ring_ShadowTrailingIdx_derived[0] = ((ShadowShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
  if (Math.abs(inClose - inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType === 1) ? (inHigh - inLow) : ((BodyLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) < ((ShadowShort_factor * (((ShadowShort_avgPeriod !== 0) ? (sp.ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowShort_rangeType === 2) ? 2.0 : 1.0)))) && (((inClose >= inOpen) ? inOpen : inClose) - inLow) < ((ShadowShort_factor * (((ShadowShort_avgPeriod !== 0) ? (sp.ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowShort_rangeType === 2) ? 2.0 : 1.0))))) {
    sp.cur_outInteger = ((((inClose >= inOpen) ? 1 : -1) * 100) | 0);
  } else {
    sp.cur_outInteger = 0;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  sp.BodyPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType === 1) ? (inHigh - inLow) : ((BodyLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0))) - sp.ring_BodyTrailingIdx_derived[sp.ringPos_BodyTrailingIdx];
  sp.ShadowPeriodTotal += ((ShadowShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0))) - sp.ring_ShadowTrailingIdx_derived[sp.ringPos_ShadowTrailingIdx];
  sp.ring_BodyTrailingIdx_derived[sp.ringPos_BodyTrailingIdx] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType === 1) ? (inHigh - inLow) : ((BodyLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  sp.ringPos_BodyTrailingIdx = sp.ringPos_BodyTrailingIdx + 1;
  if (sp.ringPos_BodyTrailingIdx >= sp.ringCap_BodyTrailingIdx) {
    sp.ringPos_BodyTrailingIdx = 0;
  }
  sp.ring_ShadowTrailingIdx_derived[sp.ringPos_ShadowTrailingIdx] = ((ShadowShort_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType === 1) ? (inHigh - inLow) : ((ShadowShort_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  sp.ringPos_ShadowTrailingIdx = sp.ringPos_ShadowTrailingIdx + 1;
  if (sp.ringPos_ShadowTrailingIdx >= sp.ringCap_ShadowTrailingIdx) {
    sp.ringPos_ShadowTrailingIdx = 0;
  }
}

function cdllonglineOpenImpl(
  sp: CdllonglineStream,
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
  let ShadowPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let BodyTrailingIdx = 0;
  let ShadowTrailingIdx = 0;
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
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const BodyLong_rangeType: number = core.candleSettings[CandleSettingType.BODY_LONG].rangeType;
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  const BodyLong_factor = core.candleSettings[CandleSettingType.BODY_LONG].factor;
  const ShadowShort_rangeType: number = core.candleSettings[CandleSettingType.SHADOW_SHORT].rangeType;
  const ShadowShort_avgPeriod = core.candleSettings[CandleSettingType.SHADOW_SHORT].avgPeriod;
  const ShadowShort_factor = core.candleSettings[CandleSettingType.SHADOW_SHORT].factor;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdllonglineLookback(core);
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
  BodyTrailingIdx = startIdx - BodyLong_avgPeriod;
  ShadowPeriodTotal = 0;
  ShadowTrailingIdx = startIdx - ShadowShort_avgPeriod;
  i = BodyTrailingIdx;
  while (i < startIdx) {
    BodyPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = ShadowTrailingIdx;
  while (i < startIdx) {
    ShadowPeriodTotal += ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - long real body
   * - short upper and lower shadow
   * The meaning of "long" and "short" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (Math.abs(inClose[i] - inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) < ((ShadowShort_factor * (((ShadowShort_avgPeriod !== 0) ? (ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowShort_rangeType === 2) ? 2.0 : 1.0)))) && (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) < ((ShadowShort_factor * (((ShadowShort_avgPeriod !== 0) ? (ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowShort_rangeType === 2) ? 2.0 : 1.0))))) {
      outInteger[((outIdx++ * outStride) | 0)] = ((((inClose[i] >= inOpen[i]) ? 1 : -1) * 100) | 0);
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyTrailingIdx] - (((inClose[BodyTrailingIdx]) >= (inOpen[BodyTrailingIdx])) ? (inClose[BodyTrailingIdx]) : (inOpen[BodyTrailingIdx]))) + ((((inClose[BodyTrailingIdx]) >= (inOpen[BodyTrailingIdx])) ? (inOpen[BodyTrailingIdx]) : (inClose[BodyTrailingIdx])) - inLow[BodyTrailingIdx])) : 0.0)));
    ShadowPeriodTotal += ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[ShadowTrailingIdx] - inOpen[ShadowTrailingIdx])) : ((ShadowShort_rangeType === 1) ? (inHigh[ShadowTrailingIdx] - inLow[ShadowTrailingIdx]) : ((ShadowShort_rangeType === 2) ? ((inHigh[ShadowTrailingIdx] - (((inClose[ShadowTrailingIdx]) >= (inOpen[ShadowTrailingIdx])) ? (inClose[ShadowTrailingIdx]) : (inOpen[ShadowTrailingIdx]))) + ((((inClose[ShadowTrailingIdx]) >= (inOpen[ShadowTrailingIdx])) ? (inOpen[ShadowTrailingIdx]) : (inClose[ShadowTrailingIdx])) - inLow[ShadowTrailingIdx])) : 0.0)));
    i += 1;
    BodyTrailingIdx += 1;
    ShadowTrailingIdx += 1;
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
    capRing_BodyTrailingIdx_derived[fillJ - (historyLen - cap_BodyTrailingIdx)] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyLong_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyLong_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  const cap_ShadowTrailingIdx = i - ShadowTrailingIdx;
  if (cap_ShadowTrailingIdx < 0 || cap_ShadowTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_ShadowTrailingIdx = cap_ShadowTrailingIdx > 0 ? cap_ShadowTrailingIdx : 1;
  const capRing_ShadowTrailingIdx_derived = new Float64Array(allocN_ShadowTrailingIdx);
  for (let fillJ = historyLen - cap_ShadowTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_ShadowTrailingIdx_derived[fillJ - (historyLen - cap_ShadowTrailingIdx)] = ((ShadowShort_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((ShadowShort_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((ShadowShort_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  sp.BodyPeriodTotal = BodyPeriodTotal;
  sp.ShadowPeriodTotal = ShadowPeriodTotal;
  sp.ringPos_BodyTrailingIdx = 0;
  sp.ringCap_BodyTrailingIdx = cap_BodyTrailingIdx;
  sp.ring_BodyTrailingIdx_derived = capRing_BodyTrailingIdx_derived;
  sp.ringPos_ShadowTrailingIdx = 0;
  sp.ringCap_ShadowTrailingIdx = cap_ShadowTrailingIdx;
  sp.ring_ShadowTrailingIdx_derived = capRing_ShadowTrailingIdx_derived;
  sp.cs_BodyLong_rangeType = BodyLong_rangeType;
  sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
  sp.cs_BodyLong_factor = BodyLong_factor;
  sp.cs_ShadowShort_rangeType = ShadowShort_rangeType;
  sp.cs_ShadowShort_avgPeriod = ShadowShort_avgPeriod;
  sp.cs_ShadowShort_factor = ShadowShort_factor;
  sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cdllonglineOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): CdllonglineStream {
  const sp = CdllonglineStream.of(core);
  const retCode = cdllonglineOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLLONGLINE openAndFill', inOpen.length, startIdx, cdllonglineLookback(core));
  }
  throw streamFailure('CDLLONGLINE openAndFill', retCode);
}

/** @internal */
export function cdllonglineOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): CdllonglineStream {
  const sp = CdllonglineStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdllonglineOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLLONGLINE open', inOpen.length, startIdx, cdllonglineLookback(core));
  }
  throw streamFailure('CDLLONGLINE open', retCode);
}

/**
 * Opens a live CDLLONGLINE stream over the warm-up history: `value()` starts at
 * the last history bar, bit-identical to {@link cdllongline} there.
 *
 * @param inOpen - Warm-up history. Open price of each bar. Every history has the
 *   same length.
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdllonglineLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an
 *   empty one, and `TA_BAD_PARAM` for histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdllonglineOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdllonglineOpen(open, high, low, close);
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdllonglineOpen(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): CdllonglineStream {
  requireArgument('CDLLONGLINE open', 'inOpen', inOpen);
  requireHistory('CDLLONGLINE open', inOpen.length);
  requireArgument('CDLLONGLINE open', 'inHigh', inHigh);
  requireArgument('CDLLONGLINE open', 'inLow', inLow);
  requireArgument('CDLLONGLINE open', 'inClose', inClose);
  requireHistoryLength('CDLLONGLINE open', 'inHigh', inHigh.length, inOpen.length);
  requireHistoryLength('CDLLONGLINE open', 'inLow', inLow.length, inOpen.length);
  requireHistoryLength('CDLLONGLINE open', 'inClose', inClose.length, inOpen.length);
  return cdllonglineOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/**
 * {@link cdllonglineOpen} that also fills the outputs over the whole history in
 * the same pass, bit-identically to {@link cdllongline}. The range written is the
 * stream's `outRange()`.
 *
 * @param inOpen - Warm-up history. Open price of each bar. Every history has the
 *   same length.
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param outInteger - +100 on a white (close>=open) long line, -100 on a black
 *   long line, 0 when no pattern. Must hold `historyLen - lookback` values and share
 *   no memory with an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdllonglineLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an
 *   empty one, and `TA_BAD_PARAM` for histories of different lengths or an output
 *   too short or starting where another output or a `Float64Array` input starts, or,
 *   on a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdllonglineOpenAndFill } from 'ta-lib';
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
 * const stream = cdllonglineOpenAndFill(open, high, low, close, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function cdllonglineOpenAndFill(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): CdllonglineStream {
  requireArgument('CDLLONGLINE openAndFill', 'inOpen', inOpen);
  requireHistory('CDLLONGLINE openAndFill', inOpen.length);
  requireArgument('CDLLONGLINE openAndFill', 'inHigh', inHigh);
  requireArgument('CDLLONGLINE openAndFill', 'inLow', inLow);
  requireArgument('CDLLONGLINE openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('CDLLONGLINE openAndFill', inOpen.length, cdllonglineLookback(core));
  requireHistoryLength('CDLLONGLINE openAndFill', 'inHigh', inHigh.length, inOpen.length);
  requireHistoryLength('CDLLONGLINE openAndFill', 'inLow', inLow.length, inOpen.length);
  requireHistoryLength('CDLLONGLINE openAndFill', 'inClose', inClose.length, inOpen.length);
  requireOutput('CDLLONGLINE openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) || (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) || (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) || (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset)) {
    throw streamFailure('CDLLONGLINE openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outInteger, inOpenF64) || overlaps(outInteger, inHighF64) || overlaps(outInteger, inLowF64) || overlaps(outInteger, inCloseF64))) {
    throw streamFailure('CDLLONGLINE openAndFill', RetCode.BAD_PARAM);
  }
  return cdllonglineOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}
