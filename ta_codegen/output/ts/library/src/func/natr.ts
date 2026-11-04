/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  060306 MF     Initial Version
 *  070526 MF,CC  Fix #98: partial-range calls normalized with a close
 *                from the wrong bar (TR-buffer-relative index).
 *  070626 MF,CC  Speed optimization: True Range computed inline in a
 *                single pass (bit-exact, no temporary buffer).
 *  082326 MF,CC  Fix #253. Test the close exactly instead of against the fixed
 *                TA_IS_ZERO band, which zeroed the output for any instrument
 *                quoted small enough to fall under it.
 *  090326 MF,CC  #338 Two-coefficient Wilder step; no divide in the
 *                loop-carried chain.
 */

import { FuncUnstId } from '../enums.ts';
import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link natr} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param optInTimePeriod - Smoothing period for the true range average (default
 *   14; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function natrLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  /* The ATR lookback is the sum of:
   *    1 + (optInTimePeriod - 1)
   *
   * Where 1 is for the True Range, and
   * (optInTimePeriod-1) is for the simple
   * moving average.
   */
  return optInTimePeriod + core.unstableCount(FuncUnstId.NATR, ((optInTimePeriod > 1) ? ((10 * optInTimePeriod) | 0) : 0), ((optInTimePeriod > 1) ? ((19 * optInTimePeriod) | 0) : 0));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link natr}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Smoothing period for the true range average (default
 *   14; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function natrDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (natrLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function natrImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let i = 0;
  let outIdx = 0;
  let today = 0;
  let lookbackTotal = 0;
  let nbATR = 0;
  let prevATR = 0;
  let periodTotal = 0;
  let tempValue = 0;
  let wAlpha = 0;
  let wBeta = 0;
  let val2 = 0;
  let val3 = 0;
  let greatest = 0;
  let tempCY = 0;
  let tempLT = 0;
  let tempHT = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  /* This function is very similar as ATR, except
   * it is being normalized as follow:
   *
   *    NATR = (ATR(period) / Close) * 100
   *
   *
   * Normalization make the ATR function more relevant
   * in the folllowing scenario:
   *    - Long term analysis where the price changes drastically.
   *    - Cross-market or cross-security ATR comparison.
   *
   * More Info:
   *      Technical Analysis of Stock & Commodities (TASC)
   *      May 2006 by John Forman
   */
  /* Average True Range is the greatest of the following:
   *
   *  val1 = distance from today's high to today's low.
   *  val2 = distance from yesterday's close to today's high.
   *  val3 = distance from yesterday's close to today's low.
   *
   * These value are averaged for the specified period using
   * Wilder method. This method have an unstable period comparable
   * to an Exponential Moving Average (EMA).
   */
  outBegIdx.value = 0;
  outNBElement.value = 0;
  /* Adjust startIdx to account for the lookback period. */
  lookbackTotal = natrLookback(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  /* wAlpha is derived FROM wBeta, never the reverse: only that order makes
   * wAlpha + wBeta exactly 1 (Sterbenz -- wBeta lands in [0.5, 1)), and it
   * measures closer to the exact recursion than the 1/period-first spelling
   * at nearly every period. The order is a gated contract, not a preference:
   * swapping it reddens the frozen-release comparison.
   */
  wBeta = (optInTimePeriod - 1) / optInTimePeriod;
  wAlpha = 1.0 - wBeta;
  /* The True Range of each bar is computed inline in a single
   * pass. No temporary buffer is needed.
   *
   * The arithmetic order below is the bit-exactness contract
   * (do not reorder):
   *  - True Range: start from high-low, then compare/replace
   *    with the two previous-close distances, in that order.
   *  - Seed: the first 'period' True Range values are summed,
   *    accumulated from 0.0 in input order, then divided by
   *    the period.
   *  - Wilder smoothing: ONE statement. Splitting it back
   *    unfuses the multiply-add and puts a second latency on
   *    the recurrence's dependency chain.
   *
   * Each output is normalized by the close of its own bar; a
   * close of zero yields 0.0.
   *
   * In-place (outReal being one of the input arrays) is
   * supported: each output is written only after every input
   * read at or before its bar, and the output index is always
   * smaller than the bar index of any remaining read.
   */
  /* The first True Range needs the two price bars at
   * startIdx-lookbackTotal+1 (a previous close is consumed).
   */
  today = startIdx - lookbackTotal + 1;
  /* Seed the ATR with a simple average of the True Range
   * for the first 'period' bars.
   */
  periodTotal = 0.0;
  i = optInTimePeriod;
  while (i-- > 0) {
    /* Find the greatest of the 3 values. */
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    periodTotal += greatest;
    today += 1;
  }
  prevATR = periodTotal / optInTimePeriod;
  /* Skip the unstable period. */
  i = lookbackTotal - optInTimePeriod;
  while (i !== 0) {
    /* Find the greatest of the 3 values. */
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    prevATR = fma(wBeta, prevATR, wAlpha * greatest);
    today += 1;
    i -= 1;
  }
  /* Now start to write the final NATR in the caller
   * provided outReal.
   */
  outIdx = 1;
  if (optInTimePeriod <= 1) {
    /* Period 1 is the raw True Range and is deliberately NOT normalized,
     * which is the TRANGE delegation this path replaced.
     */
    outReal[0] = prevATR;
  } else {
    /* NATR is the ATR as a percentage of the close, so it is scale-free and
     * the divisor only has to be non-zero. An exact test, not the fixed
     * TA_IS_ZERO band it used to be: a close carries the quote unit, and that
     * band zeroed the whole output for any instrument quoted below it (#253).
     */
    tempValue = inClose[startIdx];
    if (tempValue !== 0.0) {
      outReal[0] = prevATR / tempValue * 100.0;
    } else {
      outReal[0] = 0.0;
    }
  }
  /* Now do the number of requested NATR. */
  nbATR = endIdx - startIdx + 1;
  while (--nbATR !== 0) {
    /* Find the greatest of the 3 values. */
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    prevATR = fma(wBeta, prevATR, wAlpha * greatest);
    if (optInTimePeriod <= 1) {
      outReal[outIdx] = prevATR;
    } else {
      tempValue = inClose[today];
      if (tempValue !== 0.0) {
        outReal[outIdx] = prevATR / tempValue * 100.0;
      } else {
        outReal[outIdx] = 0.0;
      }
    }
    outIdx += 1;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * Average True Range expressed as a percentage of the current close, making
 * volatility comparable across price levels and securities. Same computation as
 * ATR, then normalized by close. Higher values mean greater relative volatility;
 * unit is percent of price.
 *
 * Formula and more info at
 * [ta-lib.org/functions/natr](https://ta-lib.org/functions/natr).
 *
 * **Notes**
 * - **The input is assumed positive.** The close is the divisor. Where it is
 *   exactly 0 the output is 0. Near zero the divisor is small and the output very
 *   large; no error is returned. A series that reaches or crosses zero, such as an
 *   oscillator or a spread, has no meaningful result here.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link natrLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param optInTimePeriod - Smoothing period for the true range average (default
 *   14; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - ATR as a percentage of the close. Must hold the values produced
 *   and never be empty; the input length always suffices.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The range written: `begIdx` is the first bar with a value, `count` how
 *   many were written.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for an argument that is not an int32, a
 *   parameter outside its range, an array too short for the range, an empty output
 *   or an output sharing memory with a `Float64Array` input it is not. Checked
 *   before anything is written.
 *
 * @example
 * ```ts
 * import { natr } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const out = new Float64Array(high.length);
 * const { begIdx, count } = natr(0, high.length - 1, high, low, close, 14, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`atr`](https://ta-lib.org/functions/atr)
 * @see [`trange`](https://ta-lib.org/functions/trange)
 * @see [`sma`](https://ta-lib.org/functions/sma)
 */
export function natr(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('NATR', 'startIdx', startIdx);
  endIdx = requireInt32('NATR', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('NATR', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('NATR', startIdx, endIdx);
  const guardStart = clampedStart('NATR', startIdx, natrLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('NATR', 'inHigh', inHigh, guardInLen);
  requireLength('NATR', 'inLow', inLow, guardInLen);
  requireLength('NATR', 'inClose', inClose, guardInLen);
  requireLength('NATR', 'outReal', outReal, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outReal, inHighF64) || overlapsPartly(outReal, inLowF64) || overlapsPartly(outReal, inCloseF64)) {
    throw failure('NATR', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = natrImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('NATR', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live NATR stream: one value per closed bar, bit-identical to {@link natr} over
 * the same series. Open it with {@link natrOpen} or {@link natrOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/natr](https://ta-lib.org/functions/natr).
 */
export class NatrStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  prevATR: number = 0;
  /** @internal */
  wAlpha: number = 0;
  /** @internal */
  wBeta: number = 0;
  /** @internal */
  lag1_inClose: number = 0;
  /** @internal */
  cur_outReal: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): NatrStream {
    return new NatrStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link natr} reports them over the same bars. Each accepted `update` and each
   * `advance` adds one to `count`.
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
      throw failure('NATR advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number, inClose: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('NATR update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('NATR update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    natrStepImpl(this, inHigh, inLow, inClose);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number, inClose: number): number {
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('NATR peek', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let tempValue = 0;
    let val2 = 0;
    let val3 = 0;
    let greatest = 0;
    let tempCY = 0;
    let tempLT = 0;
    let tempHT = 0;
    let cur_outReal = 0;
    let prevATR = sp.prevATR;
    /* Find the greatest of the 3 values. */
    tempLT = inLow;
    tempHT = inHigh;
    tempCY = sp.lag1_inClose;
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    prevATR = fma(sp.wBeta, prevATR, sp.wAlpha * greatest);
    if (sp.optInTimePeriod <= 1) {
      cur_outReal = prevATR;
    } else {
      tempValue = inClose;
      if (tempValue !== 0.0) {
        cur_outReal = prevATR / tempValue * 100.0;
      } else {
        cur_outReal = 0.0;
      }
    }
    return cur_outReal;
  }

  /** The value at the last bar this stream counted; `peek` does not change it. */
  value(): number {
    return this.cur_outReal;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): NatrStream {
    const c = new NatrStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.prevATR = this.prevATR;
    c.wAlpha = this.wAlpha;
    c.wBeta = this.wBeta;
    c.lag1_inClose = this.lag1_inClose;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function natrStepImpl(sp: NatrStream, inHigh: number, inLow: number, inClose: number): void {
  let tempValue = 0;
  let val2 = 0;
  let val3 = 0;
  let greatest = 0;
  let tempCY = 0;
  let tempLT = 0;
  let tempHT = 0;
  /* Find the greatest of the 3 values. */
  tempLT = inLow;
  tempHT = inHigh;
  tempCY = sp.lag1_inClose;
  greatest = tempHT - tempLT;
  /* val1 */
  val2 = Math.abs(tempCY - tempHT);
  if (val2 > greatest) {
    greatest = val2;
  }
  val3 = Math.abs(tempCY - tempLT);
  if (val3 > greatest) {
    greatest = val3;
  }
  sp.prevATR = fma(sp.wBeta, sp.prevATR, sp.wAlpha * greatest);
  if (sp.optInTimePeriod <= 1) {
    sp.cur_outReal = sp.prevATR;
  } else {
    tempValue = inClose;
    if (tempValue !== 0.0) {
      sp.cur_outReal = sp.prevATR / tempValue * 100.0;
    } else {
      sp.cur_outReal = 0.0;
    }
  }
  sp.lag1_inClose = inClose;
}

function natrOpenImpl(
  sp: NatrStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let i = 0;
  let outIdx = 0;
  let today = 0;
  let lookbackTotal = 0;
  let nbATR = 0;
  let prevATR = 0;
  let periodTotal = 0;
  let tempValue = 0;
  let wAlpha = 0;
  let wBeta = 0;
  let val2 = 0;
  let val3 = 0;
  let greatest = 0;
  let tempCY = 0;
  let tempLT = 0;
  let tempHT = 0;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length || inClose.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* This function is very similar as ATR, except
   * it is being normalized as follow:
   *
   *    NATR = (ATR(period) / Close) * 100
   *
   *
   * Normalization make the ATR function more relevant
   * in the folllowing scenario:
   *    - Long term analysis where the price changes drastically.
   *    - Cross-market or cross-security ATR comparison.
   *
   * More Info:
   *      Technical Analysis of Stock & Commodities (TASC)
   *      May 2006 by John Forman
   */
  /* Average True Range is the greatest of the following:
   *
   *  val1 = distance from today's high to today's low.
   *  val2 = distance from yesterday's close to today's high.
   *  val3 = distance from yesterday's close to today's low.
   *
   * These value are averaged for the specified period using
   * Wilder method. This method have an unstable period comparable
   * to an Exponential Moving Average (EMA).
   */
  outBegIdx.value = 0;
  outNBElement.value = 0;
  /* Adjust startIdx to account for the lookback period. */
  lookbackTotal = natrLookback(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* wAlpha is derived FROM wBeta, never the reverse: only that order makes
   * wAlpha + wBeta exactly 1 (Sterbenz -- wBeta lands in [0.5, 1)), and it
   * measures closer to the exact recursion than the 1/period-first spelling
   * at nearly every period. The order is a gated contract, not a preference:
   * swapping it reddens the frozen-release comparison.
   */
  wBeta = (optInTimePeriod - 1) / optInTimePeriod;
  wAlpha = 1.0 - wBeta;
  /* The True Range of each bar is computed inline in a single
   * pass. No temporary buffer is needed.
   *
   * The arithmetic order below is the bit-exactness contract
   * (do not reorder):
   *  - True Range: start from high-low, then compare/replace
   *    with the two previous-close distances, in that order.
   *  - Seed: the first 'period' True Range values are summed,
   *    accumulated from 0.0 in input order, then divided by
   *    the period.
   *  - Wilder smoothing: ONE statement. Splitting it back
   *    unfuses the multiply-add and puts a second latency on
   *    the recurrence's dependency chain.
   *
   * Each output is normalized by the close of its own bar; a
   * close of zero yields 0.0.
   *
   * In-place (outReal being one of the input arrays) is
   * supported: each output is written only after every input
   * read at or before its bar, and the output index is always
   * smaller than the bar index of any remaining read.
   */
  /* The first True Range needs the two price bars at
   * startIdx-lookbackTotal+1 (a previous close is consumed).
   */
  today = startIdx - lookbackTotal + 1;
  /* Seed the ATR with a simple average of the True Range
   * for the first 'period' bars.
   */
  periodTotal = 0.0;
  i = optInTimePeriod;
  while (i-- > 0) {
    /* Find the greatest of the 3 values. */
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    periodTotal += greatest;
    today += 1;
  }
  prevATR = periodTotal / optInTimePeriod;
  /* Skip the unstable period. */
  i = lookbackTotal - optInTimePeriod;
  while (i !== 0) {
    /* Find the greatest of the 3 values. */
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    prevATR = fma(wBeta, prevATR, wAlpha * greatest);
    today += 1;
    i -= 1;
  }
  /* Now start to write the final NATR in the caller
   * provided outReal.
   */
  outIdx = 1;
  if (optInTimePeriod <= 1) {
    /* Period 1 is the raw True Range and is deliberately NOT normalized,
     * which is the TRANGE delegation this path replaced.
     */
    outReal[((0 * outStride) | 0)] = prevATR;
  } else {
    /* NATR is the ATR as a percentage of the close, so it is scale-free and
     * the divisor only has to be non-zero. An exact test, not the fixed
     * TA_IS_ZERO band it used to be: a close carries the quote unit, and that
     * band zeroed the whole output for any instrument quoted below it (#253).
     */
    tempValue = inClose[startIdx];
    if (tempValue !== 0.0) {
      outReal[((0 * outStride) | 0)] = prevATR / tempValue * 100.0;
    } else {
      outReal[((0 * outStride) | 0)] = 0.0;
    }
  }
  /* Now do the number of requested NATR. */
  nbATR = endIdx - startIdx + 1;
  while (--nbATR !== 0) {
    /* Find the greatest of the 3 values. */
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    prevATR = fma(wBeta, prevATR, wAlpha * greatest);
    if (optInTimePeriod <= 1) {
      outReal[((outIdx * outStride) | 0)] = prevATR;
    } else {
      tempValue = inClose[today];
      if (tempValue !== 0.0) {
        outReal[((outIdx * outStride) | 0)] = prevATR / tempValue * 100.0;
      } else {
        outReal[((outIdx * outStride) | 0)] = 0.0;
      }
    }
    outIdx += 1;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  sp.optInTimePeriod = optInTimePeriod;
  sp.prevATR = prevATR;
  sp.wAlpha = wAlpha;
  sp.wBeta = wBeta;
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function natrOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): NatrStream {
  const sp = NatrStream.of(core);
  const retCode = natrOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('NATR openAndFill', inHigh.length, startIdx, natrLookback(optInTimePeriod, core));
  }
  throw streamFailure('NATR openAndFill', retCode);
}

/** @internal */
export function natrOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): NatrStream {
  const sp = NatrStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = natrOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('NATR open', inHigh.length, startIdx, natrLookback(optInTimePeriod, core));
  }
  throw streamFailure('NATR open', retCode);
}

/**
 * Opens a live NATR stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link natr} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Smoothing period for the true range average (default
 *   14; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `natrLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { natrOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = natrOpen(high, low, close, 14);
 * const peeked = stream.peek(101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function natrOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): NatrStream {
  optInTimePeriod = requireInt32('NATR open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('NATR open', 'inHigh', inHigh);
  requireHistory('NATR open', inHigh.length);
  requireArgument('NATR open', 'inLow', inLow);
  requireArgument('NATR open', 'inClose', inClose);
  requireHistoryLength('NATR open', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('NATR open', 'inClose', inClose.length, inHigh.length);
  return natrOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInTimePeriod);
}

/**
 * {@link natrOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link natr}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Smoothing period for the true range average (default
 *   14; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - ATR as a percentage of the close. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `natrLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range, histories of different lengths or an output too
 *   short or starting where another output or a `Float64Array` input starts, or, on
 *   a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { natrOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const out = new Float64Array(high.length);
 * const stream = natrOpenAndFill(high, low, close, 14, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function natrOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): NatrStream {
  optInTimePeriod = requireInt32('NATR openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('NATR openAndFill', 'inHigh', inHigh);
  requireHistory('NATR openAndFill', inHigh.length);
  requireArgument('NATR openAndFill', 'inLow', inLow);
  requireArgument('NATR openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('NATR openAndFill', inHigh.length, natrLookback(optInTimePeriod, core));
  requireHistoryLength('NATR openAndFill', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('NATR openAndFill', 'inClose', inClose.length, inHigh.length);
  requireLength('NATR openAndFill', 'outReal', outReal, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) || (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) || (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset)) {
    throw streamFailure('NATR openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inHighF64) || overlaps(outReal, inLowF64) || overlaps(outReal, inCloseF64))) {
    throw streamFailure('NATR openAndFill', RetCode.BAD_PARAM);
  }
  return natrOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}
