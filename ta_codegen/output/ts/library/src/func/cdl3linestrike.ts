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
 *  121104 AC     Creation
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 *  103126 MF,CC  Nest the cheap, selective pre-check conjuncts in an outer if
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { CandleSettingType, Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link cdl3linestrike} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function cdl3linestrikeLookback(core: Core = Core.DEFAULT): number {
  const Near_avgPeriod = core.candleSettings[CandleSettingType.NEAR].avgPeriod;
  return Near_avgPeriod + 3;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdl3linestrike}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function cdl3linestrikeDisplayShift(outputIdx: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdl3linestrikeImpl(
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
  const NearPeriodTotal = new Float64Array(4);
  let i = 0;
  let outIdx = 0;
  let totIdx = 0;
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
  lookbackTotal = cdl3linestrikeLookback(core);
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
  NearPeriodTotal[3] = 0;
  NearPeriodTotal[2] = 0;
  NearTrailingIdx = startIdx - Near_avgPeriod;
  i = NearTrailingIdx;
  while (i < startIdx) {
    NearPeriodTotal[3] = NearPeriodTotal[3] + ((Near_rangeType === 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType === 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType === 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)));
    NearPeriodTotal[2] = NearPeriodTotal[2] + ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - three white soldiers (three black crows): three white (black) candlesticks with consecutively higher (lower) closes,
   * each opening within or near the previous real body
   * - fourth candle: black (white) candle that opens above (below) prior candle's close and closes below (above)
   * the first candle's open
   * The meaning of "near" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (((inClose[i - 3] >= inOpen[i - 3]) ? 1 : -1) === ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) && /* three with same color */
        ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) &&
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === (-((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) | 0) && /* 4th opposite color */
        inOpen[i - 2] >= (inOpen[i - 3] < inClose[i - 3] ? inOpen[i - 3] : inClose[i - 3]) - ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType === 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType === 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd opens within/near 1st rb */
        inOpen[i - 2] <= (inOpen[i - 3] > inClose[i - 3] ? inOpen[i - 3] : inClose[i - 3]) + ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType === 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType === 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
        inOpen[i - 1] >= (inOpen[i - 2] < inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2]) - ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) && /* 3rd opens within/near 2nd rb */
        inOpen[i - 1] <= (inOpen[i - 2] > inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2]) + ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
        (((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === 1 &&  /* if three white */
          inClose[i - 1] > inClose[i - 2] &&
          inClose[i - 2] > inClose[i - 3] &&                    /* consecutive higher closes */
          inOpen[i] > inClose[i - 1] &&                         /* 4th opens above prior close */
          inClose[i] < inOpen[i - 3] ||                         /* 4th closes below 1st open */
         ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === -1 && /* if three black */
          inClose[i - 1] < inClose[i - 2] &&
          inClose[i - 2] < inClose[i - 3] &&                    /* consecutive lower closes */
          inOpen[i] < inClose[i - 1] &&                         /* 4th opens below prior close */
          inClose[i] > inOpen[i - 3]))                          /* 4th closes above 1st open */
    {
      outInteger[outIdx++] = ((((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) * 100) | 0);
    } else {
      outInteger[outIdx++] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    for (totIdx = 3; totIdx >= 2; totIdx -= 1) {
      NearPeriodTotal[totIdx] = NearPeriodTotal[totIdx] + (((Near_rangeType === 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((Near_rangeType === 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((Near_rangeType === 2) ? ((inHigh[i - totIdx] - (((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inClose[i - totIdx]) : (inOpen[i - totIdx]))) + ((((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inOpen[i - totIdx]) : (inClose[i - totIdx])) - inLow[i - totIdx])) : 0.0))) - ((Near_rangeType === 0) ? (Math.abs(inClose[NearTrailingIdx - totIdx] - inOpen[NearTrailingIdx - totIdx])) : ((Near_rangeType === 1) ? (inHigh[NearTrailingIdx - totIdx] - inLow[NearTrailingIdx - totIdx]) : ((Near_rangeType === 2) ? ((inHigh[NearTrailingIdx - totIdx] - (((inClose[NearTrailingIdx - totIdx]) >= (inOpen[NearTrailingIdx - totIdx])) ? (inClose[NearTrailingIdx - totIdx]) : (inOpen[NearTrailingIdx - totIdx]))) + ((((inClose[NearTrailingIdx - totIdx]) >= (inOpen[NearTrailingIdx - totIdx])) ? (inOpen[NearTrailingIdx - totIdx]) : (inClose[NearTrailingIdx - totIdx])) - inLow[NearTrailingIdx - totIdx])) : 0.0))));
    }
    i += 1;
    NearTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/**
 * A four-candle pattern: three same-color candles with consecutively higher (or
 * lower) closes, each opening within or near the prior real body, then a fourth
 * opposite-color candle that opens beyond the third close and closes past the
 * first candle's open. TA-Lib emits a signed continuation-style signal keyed to
 * the color of the first three candles, traditionally read as significant only
 * inside a trend matching those three candles.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdl3linestrike](https://ta-lib.org/functions/cdl3linestrike).
 *
 * **Notes**
 * - Does not verify the surrounding trend the pattern classically assumes for
 *   significance.
 * - TA-Lib's sign follows the classic continuation reading. Thomas Bulkowski's
 *   statistical study of the pattern (*Encyclopedia of Candlestick Charts*) found
 *   the opposite in practice — it acted as a reversal far more often than a
 *   continuation — so traders who follow his research read this pattern's signal
 *   in the opposite direction from what its sign here suggests.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link cdl3linestrikeLookback} is a success
 * with `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inOpen - Open price of each bar.
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param outInteger - +100 for a white (rising) three-line strike, -100 for a
 *   black (falling) three-line strike, 0 otherwise. Sign is the color of the first
 *   three candles: candlecolor(i-1)*100. Must hold the values produced and never be
 *   empty; the input length always suffices.
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
 * import { cdl3linestrike } from 'ta-lib';
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
 * const { begIdx, count } = cdl3linestrike(
 *   0, open.length - 1,
 *   open, high, low, close,
 *   out,
 * );
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`cdl3whitesoldiers`](https://ta-lib.org/functions/cdl3whitesoldiers)
 * @see [`cdl3blackcrows`](https://ta-lib.org/functions/cdl3blackcrows)
 */
export function cdl3linestrike(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDL3LINESTRIKE', 'startIdx', startIdx);
  endIdx = requireInt32('CDL3LINESTRIKE', 'endIdx', endIdx);
  requireIndexRange('CDL3LINESTRIKE', startIdx, endIdx);
  const guardStart = clampedStart('CDL3LINESTRIKE', startIdx, cdl3linestrikeLookback(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDL3LINESTRIKE', 'inOpen', inOpen, guardInLen);
  requireLength('CDL3LINESTRIKE', 'inHigh', inHigh, guardInLen);
  requireLength('CDL3LINESTRIKE', 'inLow', inLow, guardInLen);
  requireLength('CDL3LINESTRIKE', 'inClose', inClose, guardInLen);
  requireOutput('CDL3LINESTRIKE', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64) || overlaps(outInteger, inHighF64) || overlaps(outInteger, inLowF64) || overlaps(outInteger, inCloseF64)) {
    throw failure('CDL3LINESTRIKE', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdl3linestrikeImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDL3LINESTRIKE', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live CDL3LINESTRIKE stream: one value per closed bar, bit-identical to
 * {@link cdl3linestrike} over the same series. Open it with
 * {@link cdl3linestrikeOpen} or {@link cdl3linestrikeOpenAndFill}; there is
 * nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdl3linestrike](https://ta-lib.org/functions/cdl3linestrike).
 */
export class Cdl3linestrikeStream {
  /** @internal */
  core: Core;
  /** @internal */
  NearPeriodTotal: Float64Array = new Float64Array(4);
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
  lag3_inLow: number = 0;
  /** @internal */
  lag1_inClose: number = 0;
  /** @internal */
  lag2_inClose: number = 0;
  /** @internal */
  lag3_inClose: number = 0;
  /** @internal */
  ringPos_NearTrailingIdx: number = 0;
  /** @internal */
  ringCap_NearTrailingIdx: number = 4;
  /** @internal */
  ringLag_NearTrailingIdx: number = 0;
  /** @internal */
  ring_NearTrailingIdx_derived: Float64Array = new Float64Array(4);
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
  static of(core: Core): Cdl3linestrikeStream {
    return new Cdl3linestrikeStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cdl3linestrike} reports them over the same bars. Each accepted `update`
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
      throw failure('CDL3LINESTRIKE advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw failure('CDL3LINESTRIKE update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inOpen) || !Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('CDL3LINESTRIKE update', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    cdl3linestrikeStepImpl(this, inOpen, inHigh, inLow, inClose);
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
      throw nonFiniteBar('CDL3LINESTRIKE peek', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let cur_outInteger = 0;
    const Near_rangeType = sp.cs_Near_rangeType;
    const Near_avgPeriod = sp.cs_Near_avgPeriod;
    const Near_factor = sp.cs_Near_factor;
    if (((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : -1) === ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) && /* three with same color */
        ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) &&
        ((inClose >= inOpen) ? 1 : -1) === (-((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) | 0) && /* 4th opposite color */
        sp.lag2_inOpen >= (sp.lag3_inOpen < sp.lag3_inClose ? sp.lag3_inOpen : sp.lag3_inClose) - ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag3_inClose - sp.lag3_inOpen)) : ((Near_rangeType === 1) ? (sp.lag3_inHigh - sp.lag3_inLow) : ((Near_rangeType === 2) ? ((sp.lag3_inHigh - (((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inClose) : (sp.lag3_inOpen))) + ((((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inOpen) : (sp.lag3_inClose)) - sp.lag3_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd opens within/near 1st rb */
        sp.lag2_inOpen <= (sp.lag3_inOpen > sp.lag3_inClose ? sp.lag3_inOpen : sp.lag3_inClose) + ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag3_inClose - sp.lag3_inOpen)) : ((Near_rangeType === 1) ? (sp.lag3_inHigh - sp.lag3_inLow) : ((Near_rangeType === 2) ? ((sp.lag3_inHigh - (((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inClose) : (sp.lag3_inOpen))) + ((((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inOpen) : (sp.lag3_inClose)) - sp.lag3_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
        sp.lag1_inOpen >= (sp.lag2_inOpen < sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose) - ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) && /* 3rd opens within/near 2nd rb */
        sp.lag1_inOpen <= (sp.lag2_inOpen > sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose) + ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
        (((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === 1 &&  /* if three white */
          sp.lag1_inClose > sp.lag2_inClose &&
          sp.lag2_inClose > sp.lag3_inClose &&                    /* consecutive higher closes */
          inOpen > sp.lag1_inClose &&                             /* 4th opens above prior close */
          inClose < sp.lag3_inOpen ||                             /* 4th closes below 1st open */
         ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === -1 && /* if three black */
          sp.lag1_inClose < sp.lag2_inClose &&
          sp.lag2_inClose < sp.lag3_inClose &&                    /* consecutive lower closes */
          inOpen < sp.lag1_inClose &&                             /* 4th opens below prior close */
          inClose > sp.lag3_inOpen))                              /* 4th closes above 1st open */
    {
      cur_outInteger = ((((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) * 100) | 0);
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
  clone(): Cdl3linestrikeStream {
    const c = new Cdl3linestrikeStream(this.core);
    c.NearPeriodTotal = this.NearPeriodTotal.slice();
    c.lag1_inOpen = this.lag1_inOpen;
    c.lag2_inOpen = this.lag2_inOpen;
    c.lag3_inOpen = this.lag3_inOpen;
    c.lag1_inHigh = this.lag1_inHigh;
    c.lag2_inHigh = this.lag2_inHigh;
    c.lag3_inHigh = this.lag3_inHigh;
    c.lag1_inLow = this.lag1_inLow;
    c.lag2_inLow = this.lag2_inLow;
    c.lag3_inLow = this.lag3_inLow;
    c.lag1_inClose = this.lag1_inClose;
    c.lag2_inClose = this.lag2_inClose;
    c.lag3_inClose = this.lag3_inClose;
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

function cdl3linestrikeStepImpl(sp: Cdl3linestrikeStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  let totIdx = 0;
  const Near_rangeType = sp.cs_Near_rangeType;
  const Near_avgPeriod = sp.cs_Near_avgPeriod;
  const Near_factor = sp.cs_Near_factor;
  sp.ring_NearTrailingIdx_derived[sp.ringPos_NearTrailingIdx] = ((Near_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((Near_rangeType === 1) ? (inHigh - inLow) : ((Near_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  if (((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : -1) === ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) && /* three with same color */
      ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) &&
      ((inClose >= inOpen) ? 1 : -1) === (-((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) | 0) && /* 4th opposite color */
      sp.lag2_inOpen >= (sp.lag3_inOpen < sp.lag3_inClose ? sp.lag3_inOpen : sp.lag3_inClose) - ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag3_inClose - sp.lag3_inOpen)) : ((Near_rangeType === 1) ? (sp.lag3_inHigh - sp.lag3_inLow) : ((Near_rangeType === 2) ? ((sp.lag3_inHigh - (((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inClose) : (sp.lag3_inOpen))) + ((((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inOpen) : (sp.lag3_inClose)) - sp.lag3_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd opens within/near 1st rb */
      sp.lag2_inOpen <= (sp.lag3_inOpen > sp.lag3_inClose ? sp.lag3_inOpen : sp.lag3_inClose) + ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag3_inClose - sp.lag3_inOpen)) : ((Near_rangeType === 1) ? (sp.lag3_inHigh - sp.lag3_inLow) : ((Near_rangeType === 2) ? ((sp.lag3_inHigh - (((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inClose) : (sp.lag3_inOpen))) + ((((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inOpen) : (sp.lag3_inClose)) - sp.lag3_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
      sp.lag1_inOpen >= (sp.lag2_inOpen < sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose) - ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) && /* 3rd opens within/near 2nd rb */
      sp.lag1_inOpen <= (sp.lag2_inOpen > sp.lag2_inClose ? sp.lag2_inOpen : sp.lag2_inClose) + ((Near_factor * (((Near_avgPeriod !== 0) ? (sp.NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType === 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType === 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
      (((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === 1 &&  /* if three white */
        sp.lag1_inClose > sp.lag2_inClose &&
        sp.lag2_inClose > sp.lag3_inClose &&                    /* consecutive higher closes */
        inOpen > sp.lag1_inClose &&                             /* 4th opens above prior close */
        inClose < sp.lag3_inOpen ||                             /* 4th closes below 1st open */
       ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === -1 && /* if three black */
        sp.lag1_inClose < sp.lag2_inClose &&
        sp.lag2_inClose < sp.lag3_inClose &&                    /* consecutive lower closes */
        inOpen < sp.lag1_inClose &&                             /* 4th opens below prior close */
        inClose > sp.lag3_inOpen))                              /* 4th closes above 1st open */
  {
    sp.cur_outInteger = ((((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) * 100) | 0);
  } else {
    sp.cur_outInteger = 0;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  for (totIdx = 3; totIdx >= 2; totIdx -= 1) {
    sp.NearPeriodTotal[totIdx] = sp.NearPeriodTotal[totIdx] + (sp.ring_NearTrailingIdx_derived[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - totIdx >= sp.ringCap_NearTrailingIdx) ? sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - totIdx - sp.ringCap_NearTrailingIdx : sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - totIdx] - sp.ring_NearTrailingIdx_derived[(((sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - totIdx) % sp.ringCap_NearTrailingIdx) | 0)]);
  }
  sp.lag3_inOpen = sp.lag2_inOpen;
  sp.lag2_inOpen = sp.lag1_inOpen;
  sp.lag1_inOpen = inOpen;
  sp.lag3_inHigh = sp.lag2_inHigh;
  sp.lag2_inHigh = sp.lag1_inHigh;
  sp.lag1_inHigh = inHigh;
  sp.lag3_inLow = sp.lag2_inLow;
  sp.lag2_inLow = sp.lag1_inLow;
  sp.lag1_inLow = inLow;
  sp.lag3_inClose = sp.lag2_inClose;
  sp.lag2_inClose = sp.lag1_inClose;
  sp.lag1_inClose = inClose;
  sp.ringPos_NearTrailingIdx = sp.ringPos_NearTrailingIdx + 1;
  if (sp.ringPos_NearTrailingIdx >= sp.ringCap_NearTrailingIdx) {
    sp.ringPos_NearTrailingIdx = 0;
  }
}

function cdl3linestrikeOpenImpl(
  sp: Cdl3linestrikeStream,
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
  const NearPeriodTotal = new Float64Array(4);
  let i = 0;
  let outIdx = 0;
  let totIdx = 0;
  let NearTrailingIdx = 0;
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
  const Near_rangeType: number = core.candleSettings[CandleSettingType.NEAR].rangeType;
  const Near_avgPeriod = core.candleSettings[CandleSettingType.NEAR].avgPeriod;
  const Near_factor = core.candleSettings[CandleSettingType.NEAR].factor;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdl3linestrikeLookback(core);
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
  NearPeriodTotal[3] = 0;
  NearPeriodTotal[2] = 0;
  NearTrailingIdx = startIdx - Near_avgPeriod;
  i = NearTrailingIdx;
  while (i < startIdx) {
    NearPeriodTotal[3] = NearPeriodTotal[3] + ((Near_rangeType === 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType === 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType === 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)));
    NearPeriodTotal[2] = NearPeriodTotal[2] + ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - three white soldiers (three black crows): three white (black) candlesticks with consecutively higher (lower) closes,
   * each opening within or near the previous real body
   * - fourth candle: black (white) candle that opens above (below) prior candle's close and closes below (above)
   * the first candle's open
   * The meaning of "near" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (((inClose[i - 3] >= inOpen[i - 3]) ? 1 : -1) === ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) && /* three with same color */
        ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) &&
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === (-((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) | 0) && /* 4th opposite color */
        inOpen[i - 2] >= (inOpen[i - 3] < inClose[i - 3] ? inOpen[i - 3] : inClose[i - 3]) - ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType === 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType === 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) && /* 2nd opens within/near 1st rb */
        inOpen[i - 2] <= (inOpen[i - 3] > inClose[i - 3] ? inOpen[i - 3] : inClose[i - 3]) + ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType === 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType === 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
        inOpen[i - 1] >= (inOpen[i - 2] < inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2]) - ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) && /* 3rd opens within/near 2nd rb */
        inOpen[i - 1] <= (inOpen[i - 2] > inClose[i - 2] ? inOpen[i - 2] : inClose[i - 2]) + ((Near_factor * (((Near_avgPeriod !== 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType === 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType === 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType === 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType === 2) ? 2.0 : 1.0)))) &&
        (((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === 1 &&  /* if three white */
          inClose[i - 1] > inClose[i - 2] &&
          inClose[i - 2] > inClose[i - 3] &&                    /* consecutive higher closes */
          inOpen[i] > inClose[i - 1] &&                         /* 4th opens above prior close */
          inClose[i] < inOpen[i - 3] ||                         /* 4th closes below 1st open */
         ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === -1 && /* if three black */
          inClose[i - 1] < inClose[i - 2] &&
          inClose[i - 2] < inClose[i - 3] &&                    /* consecutive lower closes */
          inOpen[i] < inClose[i - 1] &&                         /* 4th opens below prior close */
          inClose[i] > inOpen[i - 3]))                          /* 4th closes above 1st open */
    {
      outInteger[((outIdx++ * outStride) | 0)] = ((((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) * 100) | 0);
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    for (totIdx = 3; totIdx >= 2; totIdx -= 1) {
      NearPeriodTotal[totIdx] = NearPeriodTotal[totIdx] + (((Near_rangeType === 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((Near_rangeType === 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((Near_rangeType === 2) ? ((inHigh[i - totIdx] - (((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inClose[i - totIdx]) : (inOpen[i - totIdx]))) + ((((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inOpen[i - totIdx]) : (inClose[i - totIdx])) - inLow[i - totIdx])) : 0.0))) - ((Near_rangeType === 0) ? (Math.abs(inClose[NearTrailingIdx - totIdx] - inOpen[NearTrailingIdx - totIdx])) : ((Near_rangeType === 1) ? (inHigh[NearTrailingIdx - totIdx] - inLow[NearTrailingIdx - totIdx]) : ((Near_rangeType === 2) ? ((inHigh[NearTrailingIdx - totIdx] - (((inClose[NearTrailingIdx - totIdx]) >= (inOpen[NearTrailingIdx - totIdx])) ? (inClose[NearTrailingIdx - totIdx]) : (inOpen[NearTrailingIdx - totIdx]))) + ((((inClose[NearTrailingIdx - totIdx]) >= (inOpen[NearTrailingIdx - totIdx])) ? (inOpen[NearTrailingIdx - totIdx]) : (inClose[NearTrailingIdx - totIdx])) - inLow[NearTrailingIdx - totIdx])) : 0.0))));
    }
    i += 1;
    NearTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const capLag_NearTrailingIdx = i - NearTrailingIdx;
  const cap_NearTrailingIdx = capLag_NearTrailingIdx + 4;
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
  sp.lag3_inOpen = inOpen[historyLen - 3];
  sp.lag1_inHigh = inHigh[historyLen - 1];
  sp.lag2_inHigh = inHigh[historyLen - 2];
  sp.lag3_inHigh = inHigh[historyLen - 3];
  sp.lag1_inLow = inLow[historyLen - 1];
  sp.lag2_inLow = inLow[historyLen - 2];
  sp.lag3_inLow = inLow[historyLen - 3];
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.lag2_inClose = inClose[historyLen - 2];
  sp.lag3_inClose = inClose[historyLen - 3];
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
export function cdl3linestrikeOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): Cdl3linestrikeStream {
  const sp = Cdl3linestrikeStream.of(core);
  const retCode = cdl3linestrikeOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDL3LINESTRIKE openAndFill', inOpen.length, startIdx, cdl3linestrikeLookback(core));
  }
  throw streamFailure('CDL3LINESTRIKE openAndFill', retCode);
}

/** @internal */
export function cdl3linestrikeOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): Cdl3linestrikeStream {
  const sp = Cdl3linestrikeStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdl3linestrikeOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDL3LINESTRIKE open', inOpen.length, startIdx, cdl3linestrikeLookback(core));
  }
  throw streamFailure('CDL3LINESTRIKE open', retCode);
}

/**
 * Opens a live CDL3LINESTRIKE stream over the warm-up history: `value()` starts at
 * the last history bar, bit-identical to {@link cdl3linestrike} there.
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
 *   than `cdl3linestrikeLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for
 *   an empty one, and `TA_BAD_PARAM` for histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdl3linestrikeOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdl3linestrikeOpen(open, high, low, close);
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdl3linestrikeOpen(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): Cdl3linestrikeStream {
  requireArgument('CDL3LINESTRIKE open', 'inOpen', inOpen);
  requireHistory('CDL3LINESTRIKE open', inOpen.length);
  requireArgument('CDL3LINESTRIKE open', 'inHigh', inHigh);
  requireArgument('CDL3LINESTRIKE open', 'inLow', inLow);
  requireArgument('CDL3LINESTRIKE open', 'inClose', inClose);
  requireHistoryLength('CDL3LINESTRIKE open', 'inHigh', inHigh.length, inOpen.length);
  requireHistoryLength('CDL3LINESTRIKE open', 'inLow', inLow.length, inOpen.length);
  requireHistoryLength('CDL3LINESTRIKE open', 'inClose', inClose.length, inOpen.length);
  return cdl3linestrikeOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/**
 * {@link cdl3linestrikeOpen} that also fills the outputs over the whole history in
 * the same pass, bit-identically to {@link cdl3linestrike}. The range written is
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
 * @param outInteger - +100 for a white (rising) three-line strike, -100 for a
 *   black (falling) three-line strike, 0 otherwise. Sign is the color of the first
 *   three candles: candlecolor(i-1)*100. Must hold `historyLen - lookback` values
 *   and share no memory with an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdl3linestrikeLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for
 *   an empty one, and `TA_BAD_PARAM` for histories of different lengths or an output
 *   too short or starting where another output or a `Float64Array` input starts, or,
 *   on a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdl3linestrikeOpenAndFill } from 'ta-lib';
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
 * const stream = cdl3linestrikeOpenAndFill(open, high, low, close, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function cdl3linestrikeOpenAndFill(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): Cdl3linestrikeStream {
  requireArgument('CDL3LINESTRIKE openAndFill', 'inOpen', inOpen);
  requireHistory('CDL3LINESTRIKE openAndFill', inOpen.length);
  requireArgument('CDL3LINESTRIKE openAndFill', 'inHigh', inHigh);
  requireArgument('CDL3LINESTRIKE openAndFill', 'inLow', inLow);
  requireArgument('CDL3LINESTRIKE openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('CDL3LINESTRIKE openAndFill', inOpen.length, cdl3linestrikeLookback(core));
  requireHistoryLength('CDL3LINESTRIKE openAndFill', 'inHigh', inHigh.length, inOpen.length);
  requireHistoryLength('CDL3LINESTRIKE openAndFill', 'inLow', inLow.length, inOpen.length);
  requireHistoryLength('CDL3LINESTRIKE openAndFill', 'inClose', inClose.length, inOpen.length);
  requireOutput('CDL3LINESTRIKE openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) || (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) || (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) || (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset)) {
    throw streamFailure('CDL3LINESTRIKE openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outInteger, inOpenF64) || overlaps(outInteger, inHighF64) || overlaps(outInteger, inLowF64) || overlaps(outInteger, inCloseF64))) {
    throw streamFailure('CDL3LINESTRIKE openAndFill', RetCode.BAD_PARAM);
  }
  return cdl3linestrikeOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}
