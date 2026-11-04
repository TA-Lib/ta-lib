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

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { CandleSettingType, Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link cdlbreakaway} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function cdlbreakawayLookback(core: Core = Core.DEFAULT): number {
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  return BodyLong_avgPeriod + 4;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdlbreakaway}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function cdlbreakawayDisplayShift(outputIdx: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdlbreakawayImpl(
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
  let BodyLongPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let BodyLongTrailingIdx = 0;
  let lookbackTotal = 0;
  const BodyLong_rangeType: number = core.candleSettings[CandleSettingType.BODY_LONG].rangeType;
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  const BodyLong_factor = core.candleSettings[CandleSettingType.BODY_LONG].factor;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlbreakawayLookback(core);
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
  BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
  i = BodyLongTrailingIdx;
  while (i < startIdx) {
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first candle: long black (white)
   * - second candle: black (white) day whose body gaps down (up)
   * - third candle: black or white day with lower (higher) high and lower (higher) low than prior candle's
   * - fourth candle: black (white) day with lower (higher) high and lower (higher) low than prior candle's
   * - fifth candle: white (black) day that closes inside the gap, erasing the prior 3 days
   * The meaning of "long" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) === ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : -1) && /* 1st, 2nd, 4th same color, 5th opposite */
        ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : -1) === ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) &&
        ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === (-((inClose[i] >= inOpen[i]) ? 1 : -1) | 0) &&
        Math.abs(inClose[i - 4] - inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st long */
        (((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) === -1 && /* when 1st is black: */
          ((inOpen[i - 3] > inClose[i - 3] ? inOpen[i - 3] : inClose[i - 3]) < (inOpen[i - 4] < inClose[i - 4] ? inOpen[i - 4] : inClose[i - 4])) && /* 2nd gaps down */
          inHigh[i - 2] < inHigh[i - 3] &&
          inLow[i - 2] < inLow[i - 3] &&                        /* 3rd has lower high and low than 2nd */
          inHigh[i - 1] < inHigh[i - 2] &&
          inLow[i - 1] < inLow[i - 2] &&                        /* 4th has lower high and low than 3rd */
          inClose[i] > inOpen[i - 3] &&
          inClose[i] < inClose[i - 4] ||                        /* 5th closes inside the gap */
         ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) === 1 &&  /* when 1st is white: */
          ((inOpen[i - 3] < inClose[i - 3] ? inOpen[i - 3] : inClose[i - 3]) > (inOpen[i - 4] > inClose[i - 4] ? inOpen[i - 4] : inClose[i - 4])) && /* 2nd gaps up */
          inHigh[i - 2] > inHigh[i - 3] &&
          inLow[i - 2] > inLow[i - 3] &&                        /* 3rd has higher high and low than 2nd */
          inHigh[i - 1] > inHigh[i - 2] &&
          inLow[i - 1] > inLow[i - 2] &&                        /* 4th has higher high and low than 3rd */
          inClose[i] < inOpen[i - 3] &&
          inClose[i] > inClose[i - 4]))                         /* 5th closes inside the gap */
    {
      outInteger[outIdx++] = ((((inClose[i] >= inOpen[i]) ? 1 : -1) * 100) | 0);
    } else {
      outInteger[outIdx++] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0))) - ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 4] - inOpen[BodyLongTrailingIdx - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyLongTrailingIdx - 4] - inLow[BodyLongTrailingIdx - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyLongTrailingIdx - 4] - (((inClose[BodyLongTrailingIdx - 4]) >= (inOpen[BodyLongTrailingIdx - 4])) ? (inClose[BodyLongTrailingIdx - 4]) : (inOpen[BodyLongTrailingIdx - 4]))) + ((((inClose[BodyLongTrailingIdx - 4]) >= (inOpen[BodyLongTrailingIdx - 4])) ? (inOpen[BodyLongTrailingIdx - 4]) : (inClose[BodyLongTrailingIdx - 4])) - inLow[BodyLongTrailingIdx - 4])) : 0.0)));
    i += 1;
    BodyLongTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/**
 * A five-candle reversal pattern: a long first candle, a same-colored second
 * candle that gaps away from it by its real body, two more candles extending the
 * move, and an opposite-colored fifth candle that closes back inside the gap.
 * Emits a bullish signal (bottom reversal) or bearish signal (top reversal).
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlbreakaway](https://ta-lib.org/functions/cdlbreakaway).
 *
 * **Notes**
 * - Does not verify the prior trend the pattern classically assumes (a breakaway
 *   matters most against a preceding move).
 * - Bulkowski's data shows a directional asymmetry TA-Lib's symmetric output
 *   doesn't capture: bullish Breakaway reverses only 59% of the time ("near
 *   random"), while bearish Breakaway reverses 63% of the time overall.
 *   ([thepatternsite.com](https://thepatternsite.com/BullBreakaway.html))
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link cdlbreakawayLookback} is a success
 * with `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inOpen - Open price of each bar.
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param outInteger - +100 when the fifth candle is white (bullish breakaway),
 *   -100 when it is black (bearish breakaway), 0 otherwise. Must hold the values
 *   produced and never be empty; the input length always suffices.
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
 * import { cdlbreakaway } from 'ta-lib';
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
 * const { begIdx, count } = cdlbreakaway(
 *   0, open.length - 1,
 *   open, high, low, close,
 *   out,
 * );
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`cdlgapsidesidewhite`](https://ta-lib.org/functions/cdlgapsidesidewhite)
 * @see [`cdlrisefall3methods`](https://ta-lib.org/functions/cdlrisefall3methods)
 * @see [`cdl3linestrike`](https://ta-lib.org/functions/cdl3linestrike)
 */
export function cdlbreakaway(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDLBREAKAWAY', 'startIdx', startIdx);
  endIdx = requireInt32('CDLBREAKAWAY', 'endIdx', endIdx);
  requireIndexRange('CDLBREAKAWAY', startIdx, endIdx);
  const guardStart = clampedStart('CDLBREAKAWAY', startIdx, cdlbreakawayLookback(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLBREAKAWAY', 'inOpen', inOpen, guardInLen);
  requireLength('CDLBREAKAWAY', 'inHigh', inHigh, guardInLen);
  requireLength('CDLBREAKAWAY', 'inLow', inLow, guardInLen);
  requireLength('CDLBREAKAWAY', 'inClose', inClose, guardInLen);
  requireOutput('CDLBREAKAWAY', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64) || overlaps(outInteger, inHighF64) || overlaps(outInteger, inLowF64) || overlaps(outInteger, inCloseF64)) {
    throw failure('CDLBREAKAWAY', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdlbreakawayImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLBREAKAWAY', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live CDLBREAKAWAY stream: one value per closed bar, bit-identical to
 * {@link cdlbreakaway} over the same series. Open it with {@link cdlbreakawayOpen}
 * or {@link cdlbreakawayOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlbreakaway](https://ta-lib.org/functions/cdlbreakaway).
 */
export class CdlbreakawayStream {
  /** @internal */
  core: Core;
  /** @internal */
  BodyLongPeriodTotal: number = 0;
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
  lag2_inHigh: number = 0;
  /** @internal */
  lag3_inHigh: number = 0;
  /** @internal */
  lag4_inHigh: number = 0;
  /** @internal */
  lag1_inLow: number = 0;
  /** @internal */
  lag2_inLow: number = 0;
  /** @internal */
  lag3_inLow: number = 0;
  /** @internal */
  lag4_inLow: number = 0;
  /** @internal */
  lag1_inClose: number = 0;
  /** @internal */
  lag2_inClose: number = 0;
  /** @internal */
  lag3_inClose: number = 0;
  /** @internal */
  lag4_inClose: number = 0;
  /** @internal */
  ringPos_BodyLongTrailingIdx: number = 0;
  /** @internal */
  ringCap_BodyLongTrailingIdx: number = 5;
  /** @internal */
  ringLag_BodyLongTrailingIdx: number = 0;
  /** @internal */
  ring_BodyLongTrailingIdx_derived: Float64Array = new Float64Array(5);
  /** @internal */
  cs_BodyLong_rangeType: number = 0;
  /** @internal */
  cs_BodyLong_avgPeriod: number = 0;
  /** @internal */
  cs_BodyLong_factor: number = 0;
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
  static of(core: Core): CdlbreakawayStream {
    return new CdlbreakawayStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cdlbreakaway} reports them over the same bars. Each accepted `update` and
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
      throw failure('CDLBREAKAWAY advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw failure('CDLBREAKAWAY update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inOpen) || !Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('CDLBREAKAWAY update', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    cdlbreakawayStepImpl(this, inOpen, inHigh, inLow, inClose);
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
      throw nonFiniteBar('CDLBREAKAWAY peek', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let cur_outInteger = 0;
    const BodyLong_rangeType = sp.cs_BodyLong_rangeType;
    const BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
    const BodyLong_factor = sp.cs_BodyLong_factor;
    if (((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) === ((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : -1) && /* 1st, 2nd, 4th same color, 5th opposite */
        ((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : -1) === ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) &&
        ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === (-((inClose >= inOpen) ? 1 : -1) | 0) &&
        Math.abs(sp.lag4_inClose - sp.lag4_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag4_inClose - sp.lag4_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag4_inHigh - sp.lag4_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag4_inHigh - (((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inClose) : (sp.lag4_inOpen))) + ((((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inOpen) : (sp.lag4_inClose)) - sp.lag4_inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st long */
        (((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) === -1 && /* when 1st is black: */
          ((sp.lag3_inOpen > sp.lag3_inClose ? sp.lag3_inOpen : sp.lag3_inClose) < (sp.lag4_inOpen < sp.lag4_inClose ? sp.lag4_inOpen : sp.lag4_inClose)) && /* 2nd gaps down */
          sp.lag2_inHigh < sp.lag3_inHigh &&
          sp.lag2_inLow < sp.lag3_inLow &&                        /* 3rd has lower high and low than 2nd */
          sp.lag1_inHigh < sp.lag2_inHigh &&
          sp.lag1_inLow < sp.lag2_inLow &&                        /* 4th has lower high and low than 3rd */
          inClose > sp.lag3_inOpen &&
          inClose < sp.lag4_inClose ||                            /* 5th closes inside the gap */
         ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) === 1 &&  /* when 1st is white: */
          ((sp.lag3_inOpen < sp.lag3_inClose ? sp.lag3_inOpen : sp.lag3_inClose) > (sp.lag4_inOpen > sp.lag4_inClose ? sp.lag4_inOpen : sp.lag4_inClose)) && /* 2nd gaps up */
          sp.lag2_inHigh > sp.lag3_inHigh &&
          sp.lag2_inLow > sp.lag3_inLow &&                        /* 3rd has higher high and low than 2nd */
          sp.lag1_inHigh > sp.lag2_inHigh &&
          sp.lag1_inLow > sp.lag2_inLow &&                        /* 4th has higher high and low than 3rd */
          inClose < sp.lag3_inOpen &&
          inClose > sp.lag4_inClose))                             /* 5th closes inside the gap */
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
  clone(): CdlbreakawayStream {
    const c = new CdlbreakawayStream(this.core);
    c.BodyLongPeriodTotal = this.BodyLongPeriodTotal;
    c.lag1_inOpen = this.lag1_inOpen;
    c.lag2_inOpen = this.lag2_inOpen;
    c.lag3_inOpen = this.lag3_inOpen;
    c.lag4_inOpen = this.lag4_inOpen;
    c.lag1_inHigh = this.lag1_inHigh;
    c.lag2_inHigh = this.lag2_inHigh;
    c.lag3_inHigh = this.lag3_inHigh;
    c.lag4_inHigh = this.lag4_inHigh;
    c.lag1_inLow = this.lag1_inLow;
    c.lag2_inLow = this.lag2_inLow;
    c.lag3_inLow = this.lag3_inLow;
    c.lag4_inLow = this.lag4_inLow;
    c.lag1_inClose = this.lag1_inClose;
    c.lag2_inClose = this.lag2_inClose;
    c.lag3_inClose = this.lag3_inClose;
    c.lag4_inClose = this.lag4_inClose;
    c.ringPos_BodyLongTrailingIdx = this.ringPos_BodyLongTrailingIdx;
    c.ringCap_BodyLongTrailingIdx = this.ringCap_BodyLongTrailingIdx;
    c.ringLag_BodyLongTrailingIdx = this.ringLag_BodyLongTrailingIdx;
    c.ring_BodyLongTrailingIdx_derived = this.ring_BodyLongTrailingIdx_derived.slice();
    c.cs_BodyLong_rangeType = this.cs_BodyLong_rangeType;
    c.cs_BodyLong_avgPeriod = this.cs_BodyLong_avgPeriod;
    c.cs_BodyLong_factor = this.cs_BodyLong_factor;
    c.cur_outInteger = this.cur_outInteger;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cdlbreakawayStepImpl(sp: CdlbreakawayStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  const BodyLong_rangeType = sp.cs_BodyLong_rangeType;
  const BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
  const BodyLong_factor = sp.cs_BodyLong_factor;
  sp.ring_BodyLongTrailingIdx_derived[sp.ringPos_BodyLongTrailingIdx] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType === 1) ? (inHigh - inLow) : ((BodyLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  if (((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) === ((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : -1) && /* 1st, 2nd, 4th same color, 5th opposite */
      ((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : -1) === ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) &&
      ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === (-((inClose >= inOpen) ? 1 : -1) | 0) &&
      Math.abs(sp.lag4_inClose - sp.lag4_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag4_inClose - sp.lag4_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag4_inHigh - sp.lag4_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag4_inHigh - (((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inClose) : (sp.lag4_inOpen))) + ((((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inOpen) : (sp.lag4_inClose)) - sp.lag4_inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st long */
      (((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) === -1 && /* when 1st is black: */
        ((sp.lag3_inOpen > sp.lag3_inClose ? sp.lag3_inOpen : sp.lag3_inClose) < (sp.lag4_inOpen < sp.lag4_inClose ? sp.lag4_inOpen : sp.lag4_inClose)) && /* 2nd gaps down */
        sp.lag2_inHigh < sp.lag3_inHigh &&
        sp.lag2_inLow < sp.lag3_inLow &&                        /* 3rd has lower high and low than 2nd */
        sp.lag1_inHigh < sp.lag2_inHigh &&
        sp.lag1_inLow < sp.lag2_inLow &&                        /* 4th has lower high and low than 3rd */
        inClose > sp.lag3_inOpen &&
        inClose < sp.lag4_inClose ||                            /* 5th closes inside the gap */
       ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : -1) === 1 &&  /* when 1st is white: */
        ((sp.lag3_inOpen < sp.lag3_inClose ? sp.lag3_inOpen : sp.lag3_inClose) > (sp.lag4_inOpen > sp.lag4_inClose ? sp.lag4_inOpen : sp.lag4_inClose)) && /* 2nd gaps up */
        sp.lag2_inHigh > sp.lag3_inHigh &&
        sp.lag2_inLow > sp.lag3_inLow &&                        /* 3rd has higher high and low than 2nd */
        sp.lag1_inHigh > sp.lag2_inHigh &&
        sp.lag1_inLow > sp.lag2_inLow &&                        /* 4th has higher high and low than 3rd */
        inClose < sp.lag3_inOpen &&
        inClose > sp.lag4_inClose))                             /* 5th closes inside the gap */
  {
    sp.cur_outInteger = ((((inClose >= inOpen) ? 1 : -1) * 100) | 0);
  } else {
    sp.cur_outInteger = 0;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  sp.BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag4_inClose - sp.lag4_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag4_inHigh - sp.lag4_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag4_inHigh - (((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inClose) : (sp.lag4_inOpen))) + ((((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inOpen) : (sp.lag4_inClose)) - sp.lag4_inLow)) : 0.0))) - sp.ring_BodyLongTrailingIdx_derived[(((sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 4) % sp.ringCap_BodyLongTrailingIdx) | 0)];
  sp.lag4_inOpen = sp.lag3_inOpen;
  sp.lag3_inOpen = sp.lag2_inOpen;
  sp.lag2_inOpen = sp.lag1_inOpen;
  sp.lag1_inOpen = inOpen;
  sp.lag4_inHigh = sp.lag3_inHigh;
  sp.lag3_inHigh = sp.lag2_inHigh;
  sp.lag2_inHigh = sp.lag1_inHigh;
  sp.lag1_inHigh = inHigh;
  sp.lag4_inLow = sp.lag3_inLow;
  sp.lag3_inLow = sp.lag2_inLow;
  sp.lag2_inLow = sp.lag1_inLow;
  sp.lag1_inLow = inLow;
  sp.lag4_inClose = sp.lag3_inClose;
  sp.lag3_inClose = sp.lag2_inClose;
  sp.lag2_inClose = sp.lag1_inClose;
  sp.lag1_inClose = inClose;
  sp.ringPos_BodyLongTrailingIdx = sp.ringPos_BodyLongTrailingIdx + 1;
  if (sp.ringPos_BodyLongTrailingIdx >= sp.ringCap_BodyLongTrailingIdx) {
    sp.ringPos_BodyLongTrailingIdx = 0;
  }
}

function cdlbreakawayOpenImpl(
  sp: CdlbreakawayStream,
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
  let BodyLongPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let BodyLongTrailingIdx = 0;
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
  const BodyLong_rangeType: number = core.candleSettings[CandleSettingType.BODY_LONG].rangeType;
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  const BodyLong_factor = core.candleSettings[CandleSettingType.BODY_LONG].factor;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlbreakawayLookback(core);
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
  BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
  i = BodyLongTrailingIdx;
  while (i < startIdx) {
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first candle: long black (white)
   * - second candle: black (white) day whose body gaps down (up)
   * - third candle: black or white day with lower (higher) high and lower (higher) low than prior candle's
   * - fourth candle: black (white) day with lower (higher) high and lower (higher) low than prior candle's
   * - fifth candle: white (black) day that closes inside the gap, erasing the prior 3 days
   * The meaning of "long" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) === ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : -1) && /* 1st, 2nd, 4th same color, 5th opposite */
        ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : -1) === ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) &&
        ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === (-((inClose[i] >= inOpen[i]) ? 1 : -1) | 0) &&
        Math.abs(inClose[i - 4] - inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0)))) && /* 1st long */
        (((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) === -1 && /* when 1st is black: */
          ((inOpen[i - 3] > inClose[i - 3] ? inOpen[i - 3] : inClose[i - 3]) < (inOpen[i - 4] < inClose[i - 4] ? inOpen[i - 4] : inClose[i - 4])) && /* 2nd gaps down */
          inHigh[i - 2] < inHigh[i - 3] &&
          inLow[i - 2] < inLow[i - 3] &&                        /* 3rd has lower high and low than 2nd */
          inHigh[i - 1] < inHigh[i - 2] &&
          inLow[i - 1] < inLow[i - 2] &&                        /* 4th has lower high and low than 3rd */
          inClose[i] > inOpen[i - 3] &&
          inClose[i] < inClose[i - 4] ||                        /* 5th closes inside the gap */
         ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : -1) === 1 &&  /* when 1st is white: */
          ((inOpen[i - 3] < inClose[i - 3] ? inOpen[i - 3] : inClose[i - 3]) > (inOpen[i - 4] > inClose[i - 4] ? inOpen[i - 4] : inClose[i - 4])) && /* 2nd gaps up */
          inHigh[i - 2] > inHigh[i - 3] &&
          inLow[i - 2] > inLow[i - 3] &&                        /* 3rd has higher high and low than 2nd */
          inHigh[i - 1] > inHigh[i - 2] &&
          inLow[i - 1] > inLow[i - 2] &&                        /* 4th has higher high and low than 3rd */
          inClose[i] < inOpen[i - 3] &&
          inClose[i] > inClose[i - 4]))                         /* 5th closes inside the gap */
    {
      outInteger[((outIdx++ * outStride) | 0)] = ((((inClose[i] >= inOpen[i]) ? 1 : -1) * 100) | 0);
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0))) - ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 4] - inOpen[BodyLongTrailingIdx - 4])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyLongTrailingIdx - 4] - inLow[BodyLongTrailingIdx - 4]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyLongTrailingIdx - 4] - (((inClose[BodyLongTrailingIdx - 4]) >= (inOpen[BodyLongTrailingIdx - 4])) ? (inClose[BodyLongTrailingIdx - 4]) : (inOpen[BodyLongTrailingIdx - 4]))) + ((((inClose[BodyLongTrailingIdx - 4]) >= (inOpen[BodyLongTrailingIdx - 4])) ? (inOpen[BodyLongTrailingIdx - 4]) : (inClose[BodyLongTrailingIdx - 4])) - inLow[BodyLongTrailingIdx - 4])) : 0.0)));
    i += 1;
    BodyLongTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const capLag_BodyLongTrailingIdx = i - BodyLongTrailingIdx;
  const cap_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx + 5;
  if (capLag_BodyLongTrailingIdx < 0 || cap_BodyLongTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_BodyLongTrailingIdx = cap_BodyLongTrailingIdx > 0 ? cap_BodyLongTrailingIdx : 1;
  const capRing_BodyLongTrailingIdx_derived = new Float64Array(allocN_BodyLongTrailingIdx);
  for (let fillJ = historyLen - cap_BodyLongTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_BodyLongTrailingIdx_derived[fillJ % cap_BodyLongTrailingIdx] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyLong_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyLong_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  sp.BodyLongPeriodTotal = BodyLongPeriodTotal;
  sp.lag1_inOpen = inOpen[historyLen - 1];
  sp.lag2_inOpen = inOpen[historyLen - 2];
  sp.lag3_inOpen = inOpen[historyLen - 3];
  sp.lag4_inOpen = inOpen[historyLen - 4];
  sp.lag1_inHigh = inHigh[historyLen - 1];
  sp.lag2_inHigh = inHigh[historyLen - 2];
  sp.lag3_inHigh = inHigh[historyLen - 3];
  sp.lag4_inHigh = inHigh[historyLen - 4];
  sp.lag1_inLow = inLow[historyLen - 1];
  sp.lag2_inLow = inLow[historyLen - 2];
  sp.lag3_inLow = inLow[historyLen - 3];
  sp.lag4_inLow = inLow[historyLen - 4];
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.lag2_inClose = inClose[historyLen - 2];
  sp.lag3_inClose = inClose[historyLen - 3];
  sp.lag4_inClose = inClose[historyLen - 4];
  sp.ringPos_BodyLongTrailingIdx = historyLen % cap_BodyLongTrailingIdx;
  sp.ringCap_BodyLongTrailingIdx = cap_BodyLongTrailingIdx;
  sp.ringLag_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx;
  sp.ring_BodyLongTrailingIdx_derived = capRing_BodyLongTrailingIdx_derived;
  sp.cs_BodyLong_rangeType = BodyLong_rangeType;
  sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
  sp.cs_BodyLong_factor = BodyLong_factor;
  sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlbreakawayOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): CdlbreakawayStream {
  const sp = CdlbreakawayStream.of(core);
  const retCode = cdlbreakawayOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLBREAKAWAY openAndFill', inOpen.length, startIdx, cdlbreakawayLookback(core));
  }
  throw streamFailure('CDLBREAKAWAY openAndFill', retCode);
}

/** @internal */
export function cdlbreakawayOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): CdlbreakawayStream {
  const sp = CdlbreakawayStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdlbreakawayOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLBREAKAWAY open', inOpen.length, startIdx, cdlbreakawayLookback(core));
  }
  throw streamFailure('CDLBREAKAWAY open', retCode);
}

/**
 * Opens a live CDLBREAKAWAY stream over the warm-up history: `value()` starts at
 * the last history bar, bit-identical to {@link cdlbreakaway} there.
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
 *   than `cdlbreakawayLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdlbreakawayOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdlbreakawayOpen(open, high, low, close);
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdlbreakawayOpen(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): CdlbreakawayStream {
  requireArgument('CDLBREAKAWAY open', 'inOpen', inOpen);
  requireHistory('CDLBREAKAWAY open', inOpen.length);
  requireArgument('CDLBREAKAWAY open', 'inHigh', inHigh);
  requireArgument('CDLBREAKAWAY open', 'inLow', inLow);
  requireArgument('CDLBREAKAWAY open', 'inClose', inClose);
  requireHistoryLength('CDLBREAKAWAY open', 'inHigh', inHigh.length, inOpen.length);
  requireHistoryLength('CDLBREAKAWAY open', 'inLow', inLow.length, inOpen.length);
  requireHistoryLength('CDLBREAKAWAY open', 'inClose', inClose.length, inOpen.length);
  return cdlbreakawayOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/**
 * {@link cdlbreakawayOpen} that also fills the outputs over the whole history in
 * the same pass, bit-identically to {@link cdlbreakaway}. The range written is the
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
 * @param outInteger - +100 when the fifth candle is white (bullish breakaway),
 *   -100 when it is black (bearish breakaway), 0 otherwise. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlbreakawayLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for histories of different lengths or an output too short or
 *   starting where another output or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another output or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { cdlbreakawayOpenAndFill } from 'ta-lib';
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
 * const stream = cdlbreakawayOpenAndFill(open, high, low, close, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function cdlbreakawayOpenAndFill(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): CdlbreakawayStream {
  requireArgument('CDLBREAKAWAY openAndFill', 'inOpen', inOpen);
  requireHistory('CDLBREAKAWAY openAndFill', inOpen.length);
  requireArgument('CDLBREAKAWAY openAndFill', 'inHigh', inHigh);
  requireArgument('CDLBREAKAWAY openAndFill', 'inLow', inLow);
  requireArgument('CDLBREAKAWAY openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('CDLBREAKAWAY openAndFill', inOpen.length, cdlbreakawayLookback(core));
  requireHistoryLength('CDLBREAKAWAY openAndFill', 'inHigh', inHigh.length, inOpen.length);
  requireHistoryLength('CDLBREAKAWAY openAndFill', 'inLow', inLow.length, inOpen.length);
  requireHistoryLength('CDLBREAKAWAY openAndFill', 'inClose', inClose.length, inOpen.length);
  requireOutput('CDLBREAKAWAY openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) || (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) || (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) || (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset)) {
    throw streamFailure('CDLBREAKAWAY openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outInteger, inOpenF64) || overlaps(outInteger, inHighF64) || overlaps(outInteger, inLowF64) || overlaps(outInteger, inCloseF64))) {
    throw streamFailure('CDLBREAKAWAY openAndFill', RetCode.BAD_PARAM);
  }
  return cdlbreakawayOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}
