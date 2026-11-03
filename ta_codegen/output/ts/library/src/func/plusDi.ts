/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AM       Adrian Michel
 *  MIF      Mirek Fontan (mira@fontan.cz)
 *  CF       Christo Fogelberg
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY    Description
 *  -------------------------------------------------------------------
 *  010802 MF    Template creation.
 *  052603 MF    Adapt code to compile with .NET Managed C++
 *  082303 MF    Fix #792298. Remove rounding. Bug reported by AM.
 *  062704 MF    Fix #965557. Div by zero bug reported by MIF.
 *  122204 MF,CF Fix #1090231. Issues when period is 1.
 *  082326 MF,CC Fix #253. Test the true range exactly instead of against the
 *               fixed TA_IS_ZERO band, which zeroed the index for any
 *               instrument quoted small enough to fall under it.
 *  091326 MF,CC #411 Wilder steps without a divide or a branch.
 */

import { FuncUnstId } from '../enums.ts';
import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link plusDi} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param optInTimePeriod - Wilder smoothing period (default 14; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function plusDiLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  if (optInTimePeriod > 1) {
    return optInTimePeriod + core.unstableCount(FuncUnstId.PLUS_DI, ((optInTimePeriod > 1) ? ((10 * optInTimePeriod) | 0) : 0), ((optInTimePeriod > 1) ? ((19 * optInTimePeriod) | 0) : 0));
  } else {
    return 1;
  }
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link plusDi}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Wilder smoothing period (default 14; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function plusDiDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (plusDiLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function plusDiImpl(
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
  let today = 0;
  let lookbackTotal = 0;
  let outIdx = 0;
  let prevHigh = 0;
  let prevLow = 0;
  let prevClose = 0;
  let invPeriod = 0;
  let prevPlusDM = 0;
  let prevTR = 0;
  let tempReal = 0;
  let diffP = 0;
  let diffM = 0;
  let plusDM1 = 0;
  let i = 0;
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
  /*
   * The DM1 (one period) is base on the largest part of
   * today's range that is outside of yesterdays range.
   *
   * The following 7 cases explain how the +DM and -DM are
   * calculated on one period:
   *
   * Case 1:                       Case 2:
   *    C|                        A|
   *     |                         | C|
   *     | +DM1 = (C-A)           B|  | +DM1 = 0
   *     | -DM1 = 0                   | -DM1 = (B-D)
   * A|  |                           D|
   *  | D|
   * B|
   *
   * Case 3:                       Case 4:
   *    C|                           C|
   *     |                        A|  |
   *     | +DM1 = (C-A)            |  | +DM1 = 0
   *     | -DM1 = 0               B|  | -DM1 = (B-D)
   * A|  |                            |
   *  |  |                           D|
   * B|  |
   *    D|
   *
   * Case 5:                      Case 6:
   * A|                           A| C|
   *  | C| +DM1 = 0                |  |  +DM1 = 0
   *  |  | -DM1 = 0                |  |  -DM1 = 0
   *  | D|                         |  |
   * B|                           B| D|
   *
   *
   * Case 7:
   *
   *    C|
   * A|  |
   *  |  | +DM1=0
   * B|  | -DM1=0
   *    D|
   *
   * In case 3 and 4, the rule is that the smallest delta between
   * (C-A) and (B-D) determine which of +DM or -DM is zero.
   *
   * In case 7, (C-A) and (B-D) are equal, so both +DM and -DM are
   * zero.
   *
   * The rules remain the same when A=B and C=D (when the highs
   * equal the lows).
   *
   * When calculating the DM over a period > 1, the one-period DM
   * for the desired period are initialy sum. In other word,
   * for a -DM14, sum the -DM1 for the first 14 days (that's
   * 13 values because there is no DM for the first day!)
   * Subsequent DM are calculated using the Wilder's
   * smoothing approach:
   *
   *                                    Previous +DM14
   *  Today's +DM14 = Previous +DM14 -  -------------- + Today's +DM1
   *                                         14
   *
   * Calculation of a +DI14 is as follow:
   *
   *               +DM14
   *     +DI14 =  --------
   *                TR14
   *
   * Calculation of the TR14 is:
   *
   *                                   Previous TR14
   *    Today's TR14 = Previous TR14 - -------------- + Today's TR1
   *                                         14
   *
   *    The first TR14 is the summation of the first 14 TR1. See the
   *    TA_TRANGE function on how to calculate the true range.
   *
   * Reference:
   *    New Concepts In Technical Trading Systems, J. Welles Wilder Jr
   */
  /* Original implementation from Wilder's book was doing some integer
   * rounding in its calculations.
   *
   * This was understandable in the context that at the time the book
   * was written, most user were doing the calculation by hand.
   *
   * For a computer, rounding is unnecessary (and even problematic when inputs
   * are close to 1).
   *
   * TA-Lib does not do the rounding. Still, if you want to reproduce Wilder's examples,
   * you can comment out the following #undef/#define and rebuild the library.
   */
  lookbackTotal = plusDiLookback(optInTimePeriod, core);
  /* Adjust startIdx to account for the lookback period. */
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  /* Indicate where the next output should be put
   * in the outReal.
   */
  outIdx = 0;
  /* Trap the case where no smoothing is needed. */
  if (optInTimePeriod <= 1) {
    /* No smoothing needed. Just do the following:
     * for each price bar.
     *          +DM1
     *   +DI1 = ----
     *           TR1
     */
    outBegIdx.value = startIdx;
    today = startIdx - 1;
    prevHigh = inHigh[today];
    prevLow = inLow[today];
    prevClose = inClose[today];
    while (today < endIdx) {
      today += 1;
      tempReal = inHigh[today];
      diffP = tempReal - prevHigh;
      /* Plus Delta */
      prevHigh = tempReal;
      tempReal = inLow[today];
      diffM = prevLow - tempReal;
      /* Minus Delta */
      prevLow = tempReal;
      if (diffP > 0 && diffP > diffM) {
        /* Case 1 and 3: +DM=diffP,-DM=0 */
        let _true_range_0 = 0;
        let range_0 = prevHigh - prevLow;
        let tmp_0 = Math.abs(prevHigh - prevClose);
        if (tmp_0 > range_0) {
          range_0 = tmp_0;
        }
        tmp_0 = Math.abs(prevLow - prevClose);
        if (tmp_0 > range_0) {
          range_0 = tmp_0;
        }
        _true_range_0 = range_0;
        tempReal = _true_range_0;
        if (tempReal <= 0.0) {
          outReal[outIdx++] = 0.0;
        } else {
          outReal[outIdx++] = diffP / tempReal;
        }
      } else {
        outReal[outIdx++] = 0.0;
      }
      prevClose = inClose[today];
    }
    outNBElement.value = outIdx;
    return RetCode.SUCCESS;
  }
  /* The declaration order above sets invPeriod's place in the stream state,
   * and that place is load-bearing: a layout that lets Update load it paired
   * with a field the previous bar stored stalls every call. Re-measure Update
   * in C and Rust before reordering those declarations.
   */
  invPeriod = 1.0 / optInTimePeriod;
  /* Process the initial DM and TR */
  today = startIdx;
  outBegIdx.value = today;
  prevPlusDM = 0.0;
  prevTR = 0.0;
  today = startIdx - lookbackTotal;
  prevHigh = inHigh[today];
  prevLow = inLow[today];
  prevClose = inClose[today];
  i = optInTimePeriod - 1;
  while (i-- > 0) {
    today += 1;
    tempReal = inHigh[today];
    diffP = tempReal - prevHigh;
    /* Plus Delta */
    prevHigh = tempReal;
    tempReal = inLow[today];
    diffM = prevLow - tempReal;
    /* Minus Delta */
    prevLow = tempReal;
    /* +DM1 = diffP when diffP > diffM and diffP > 0: the select takes the
     * first test and the max the second, as a non-positive delta cannot raise
     * the sum. gcc keeps a branch if a select, not the max, ends the step.
     */
    tempReal = diffP - diffM;
    plusDM1 = (tempReal > 0.0) ? diffP : 0.0;
    tempReal = prevPlusDM + plusDM1;
    prevPlusDM = (prevPlusDM > tempReal) ? prevPlusDM : tempReal;
    let _true_range_1 = 0;
    let range_1 = prevHigh - prevLow;
    let tmp_1 = Math.abs(prevHigh - prevClose);
    if (tmp_1 > range_1) {
      range_1 = tmp_1;
    }
    tmp_1 = Math.abs(prevLow - prevClose);
    if (tmp_1 > range_1) {
      range_1 = tmp_1;
    }
    _true_range_1 = range_1;
    tempReal = _true_range_1;
    prevTR += tempReal;
    prevClose = inClose[today];
  }
  /* Process subsequent DI */
  /* Skip the unstable period. Note that this loop must be executed
   * at least ONCE to calculate the first DI.
   */
  i = lookbackTotal - (optInTimePeriod - 1);
  while (i-- !== 0) {
    /* Calculate the prevPlusDM */
    today += 1;
    tempReal = inHigh[today];
    diffP = tempReal - prevHigh;
    /* Plus Delta */
    prevHigh = tempReal;
    tempReal = inLow[today];
    diffM = prevLow - tempReal;
    /* Minus Delta */
    prevLow = tempReal;
    tempReal = diffP - diffM;
    plusDM1 = (tempReal > 0.0) ? diffP : 0.0;
    tempReal = prevPlusDM - prevPlusDM * invPeriod;
    prevPlusDM = tempReal + plusDM1;
    prevPlusDM = (tempReal > prevPlusDM) ? tempReal : prevPlusDM;
    /* Calculate the prevTR */
    let _true_range_2 = 0;
    let range_2 = prevHigh - prevLow;
    let tmp_2 = Math.abs(prevHigh - prevClose);
    if (tmp_2 > range_2) {
      range_2 = tmp_2;
    }
    tmp_2 = Math.abs(prevLow - prevClose);
    if (tmp_2 > range_2) {
      range_2 = tmp_2;
    }
    _true_range_2 = range_2;
    tempReal = _true_range_2;
    prevTR = prevTR - prevTR * invPeriod + tempReal;
    prevClose = inClose[today];
  }
  /* Now start to write the output in
   * the caller provided outReal.
   */
  /* prevTR is a running sum of true ranges: non-negative by construction and
   * built only by adding, so it carries no cancellation residue and reaches
   * zero only for a window whose every range is exactly zero. Test it exactly.
   * A true range carries the quote unit, so the fixed TA_IS_ZERO band it used
   * to be compared against was a constant in some arbitrary unit, and zeroed
   * the index for any instrument quoted below it (issue #253).
   */
  if (prevTR > 0.0) {
    outReal[0] = (100.0 * (prevPlusDM / prevTR));
  } else {
    outReal[0] = 0.0;
  }
  outIdx = 1;
  while (today < endIdx) {
    /* Calculate the prevPlusDM */
    today += 1;
    tempReal = inHigh[today];
    diffP = tempReal - prevHigh;
    /* Plus Delta */
    prevHigh = tempReal;
    tempReal = inLow[today];
    diffM = prevLow - tempReal;
    /* Minus Delta */
    prevLow = tempReal;
    tempReal = diffP - diffM;
    plusDM1 = (tempReal > 0.0) ? diffP : 0.0;
    tempReal = prevPlusDM - prevPlusDM * invPeriod;
    prevPlusDM = tempReal + plusDM1;
    prevPlusDM = (tempReal > prevPlusDM) ? tempReal : prevPlusDM;
    /* Calculate the prevTR */
    let _true_range_3 = 0;
    let range_3 = prevHigh - prevLow;
    let tmp_3 = Math.abs(prevHigh - prevClose);
    if (tmp_3 > range_3) {
      range_3 = tmp_3;
    }
    tmp_3 = Math.abs(prevLow - prevClose);
    if (tmp_3 > range_3) {
      range_3 = tmp_3;
    }
    _true_range_3 = range_3;
    tempReal = _true_range_3;
    prevTR = prevTR - prevTR * invPeriod + tempReal;
    prevClose = inClose[today];
    /* Calculate the DI. The value is rounded (see Wilder book). */
    if (prevTR > 0.0) {
      outReal[outIdx++] = (100.0 * (prevPlusDM / prevTR));
    } else {
      outReal[outIdx++] = 0.0;
    }
  }
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * Plus Directional Indicator: the Wilder-smoothed positive directional movement
 * expressed as a percentage of the true range. Measures the strength of upward
 * price movement. Rising +DI signals strengthening upward direction; compared
 * against MINUS_DI to judge trend direction.
 *
 * Formula and more info at
 * [ta-lib.org/functions/plus_di](https://ta-lib.org/functions/plus_di).
 *
 * **Notes**
 * - Wilder's original integer rounding of intermediate values is not applied (it
 *   was unreliable when values are near 1).
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link plusDiLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param optInTimePeriod - Wilder smoothing period (default 14; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Plus Directional Indicator. Must hold the values produced and
 *   never be empty; the input length always suffices.
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
 * import { plusDi } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const out = new Float64Array(high.length);
 * const { begIdx, count } = plusDi(0, high.length - 1, high, low, close, 14, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`minusDi`](https://ta-lib.org/functions/minus_di)
 * @see [`dx`](https://ta-lib.org/functions/dx)
 * @see [`adx`](https://ta-lib.org/functions/adx)
 * @see [`adxr`](https://ta-lib.org/functions/adxr)
 * @see [`plusDm`](https://ta-lib.org/functions/plus_dm)
 * @see [`trange`](https://ta-lib.org/functions/trange)
 */
export function plusDi(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('PLUS_DI', 'startIdx', startIdx);
  endIdx = requireInt32('PLUS_DI', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('PLUS_DI', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('PLUS_DI', startIdx, endIdx);
  const guardStart = clampedStart('PLUS_DI', startIdx, plusDiLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('PLUS_DI', 'inHigh', inHigh, guardInLen);
  requireLength('PLUS_DI', 'inLow', inLow, guardInLen);
  requireLength('PLUS_DI', 'inClose', inClose, guardInLen);
  requireLength('PLUS_DI', 'outReal', outReal, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outReal, inHighF64) || overlapsPartly(outReal, inLowF64) || overlapsPartly(outReal, inCloseF64)) {
    throw failure('PLUS_DI', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = plusDiImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('PLUS_DI', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live PLUS_DI stream: one value per closed bar, bit-identical to {@link plusDi}
 * over the same series. Open it with {@link plusDiOpen} or
 * {@link plusDiOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/plus_di](https://ta-lib.org/functions/plus_di).
 */
export class PlusDiStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  prevHigh: number = 0;
  /** @internal */
  prevLow: number = 0;
  /** @internal */
  prevClose: number = 0;
  /** @internal */
  invPeriod: number = 0;
  /** @internal */
  prevPlusDM: number = 0;
  /** @internal */
  prevTR: number = 0;
  /** @internal */
  cur_outReal: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  /** @internal */
  constructor(core: Core) {
    this.core = core;
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link plusDi} reports them over the same bars. Each accepted `update` and each
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
      throw failure('PLUS_DI advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw failure('PLUS_DI update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('PLUS_DI update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    plusDiStepImpl(this, inHigh, inLow, inClose);
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
      throw nonFiniteBar('PLUS_DI peek', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let cur_outReal = 0;
    if (sp.optInTimePeriod <= 1) {
      let tempReal = 0;
      let diffP = 0;
      let diffM = 0;
      let prevHigh = sp.prevHigh;
      let prevLow = sp.prevLow;
      tempReal = inHigh;
      diffP = tempReal - prevHigh;
      /* Plus Delta */
      prevHigh = tempReal;
      tempReal = inLow;
      diffM = prevLow - tempReal;
      /* Minus Delta */
      prevLow = tempReal;
      if (diffP > 0 && diffP > diffM) {
        /* Case 1 and 3: +DM=diffP,-DM=0 */
        let _true_range_0 = 0;
        let range_0 = prevHigh - prevLow;
        let tmp_0 = Math.abs(prevHigh - sp.prevClose);
        if (tmp_0 > range_0) {
          range_0 = tmp_0;
        }
        tmp_0 = Math.abs(prevLow - sp.prevClose);
        if (tmp_0 > range_0) {
          range_0 = tmp_0;
        }
        _true_range_0 = range_0;
        tempReal = _true_range_0;
        if (tempReal <= 0.0) {
          cur_outReal = 0.0;
        } else {
          cur_outReal = diffP / tempReal;
        }
      } else {
        cur_outReal = 0.0;
      }
    } else {
      let tempReal = 0;
      let diffP = 0;
      let diffM = 0;
      let plusDM1 = 0;
      let prevClose = sp.prevClose;
      let prevHigh = sp.prevHigh;
      let prevLow = sp.prevLow;
      let prevPlusDM = sp.prevPlusDM;
      let prevTR = sp.prevTR;
      /* Calculate the prevPlusDM */
      tempReal = inHigh;
      diffP = tempReal - prevHigh;
      /* Plus Delta */
      prevHigh = tempReal;
      tempReal = inLow;
      diffM = prevLow - tempReal;
      /* Minus Delta */
      prevLow = tempReal;
      tempReal = diffP - diffM;
      plusDM1 = (tempReal > 0.0) ? diffP : 0.0;
      tempReal = prevPlusDM - prevPlusDM * sp.invPeriod;
      prevPlusDM = tempReal + plusDM1;
      prevPlusDM = (tempReal > prevPlusDM) ? tempReal : prevPlusDM;
      /* Calculate the prevTR */
      let _true_range_1 = 0;
      let range_1 = prevHigh - prevLow;
      let tmp_1 = Math.abs(prevHigh - prevClose);
      if (tmp_1 > range_1) {
        range_1 = tmp_1;
      }
      tmp_1 = Math.abs(prevLow - prevClose);
      if (tmp_1 > range_1) {
        range_1 = tmp_1;
      }
      _true_range_1 = range_1;
      tempReal = _true_range_1;
      prevTR = prevTR - prevTR * sp.invPeriod + tempReal;
      prevClose = inClose;
      /* Calculate the DI. The value is rounded (see Wilder book). */
      if (prevTR > 0.0) {
        cur_outReal = (100.0 * (prevPlusDM / prevTR));
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
  clone(): PlusDiStream {
    const c = new PlusDiStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.prevHigh = this.prevHigh;
    c.prevLow = this.prevLow;
    c.prevClose = this.prevClose;
    c.invPeriod = this.invPeriod;
    c.prevPlusDM = this.prevPlusDM;
    c.prevTR = this.prevTR;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function plusDiStepImpl(sp: PlusDiStream, inHigh: number, inLow: number, inClose: number): void {
  if (sp.optInTimePeriod <= 1) {
    let tempReal = 0;
    let diffP = 0;
    let diffM = 0;
    tempReal = inHigh;
    diffP = tempReal - sp.prevHigh;
    /* Plus Delta */
    sp.prevHigh = tempReal;
    tempReal = inLow;
    diffM = sp.prevLow - tempReal;
    /* Minus Delta */
    sp.prevLow = tempReal;
    if (diffP > 0 && diffP > diffM) {
      /* Case 1 and 3: +DM=diffP,-DM=0 */
      let _true_range_2 = 0;
      let range_2 = sp.prevHigh - sp.prevLow;
      let tmp_2 = Math.abs(sp.prevHigh - sp.prevClose);
      if (tmp_2 > range_2) {
        range_2 = tmp_2;
      }
      tmp_2 = Math.abs(sp.prevLow - sp.prevClose);
      if (tmp_2 > range_2) {
        range_2 = tmp_2;
      }
      _true_range_2 = range_2;
      tempReal = _true_range_2;
      if (tempReal <= 0.0) {
        sp.cur_outReal = 0.0;
      } else {
        sp.cur_outReal = diffP / tempReal;
      }
    } else {
      sp.cur_outReal = 0.0;
    }
    sp.prevClose = inClose;
  } else {
    let tempReal = 0;
    let diffP = 0;
    let diffM = 0;
    let plusDM1 = 0;
    /* Calculate the prevPlusDM */
    tempReal = inHigh;
    diffP = tempReal - sp.prevHigh;
    /* Plus Delta */
    sp.prevHigh = tempReal;
    tempReal = inLow;
    diffM = sp.prevLow - tempReal;
    /* Minus Delta */
    sp.prevLow = tempReal;
    tempReal = diffP - diffM;
    plusDM1 = (tempReal > 0.0) ? diffP : 0.0;
    tempReal = sp.prevPlusDM - sp.prevPlusDM * sp.invPeriod;
    sp.prevPlusDM = tempReal + plusDM1;
    sp.prevPlusDM = (tempReal > sp.prevPlusDM) ? tempReal : sp.prevPlusDM;
    /* Calculate the prevTR */
    let _true_range_3 = 0;
    let range_3 = sp.prevHigh - sp.prevLow;
    let tmp_3 = Math.abs(sp.prevHigh - sp.prevClose);
    if (tmp_3 > range_3) {
      range_3 = tmp_3;
    }
    tmp_3 = Math.abs(sp.prevLow - sp.prevClose);
    if (tmp_3 > range_3) {
      range_3 = tmp_3;
    }
    _true_range_3 = range_3;
    tempReal = _true_range_3;
    sp.prevTR = sp.prevTR - sp.prevTR * sp.invPeriod + tempReal;
    sp.prevClose = inClose;
    /* Calculate the DI. The value is rounded (see Wilder book). */
    if (sp.prevTR > 0.0) {
      sp.cur_outReal = (100.0 * (sp.prevPlusDM / sp.prevTR));
    } else {
      sp.cur_outReal = 0.0;
    }
  }
}

function plusDiOpenImpl(
  sp: PlusDiStream,
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
  if (optInTimePeriod <= 1) {
    let today = 0;
    let lookbackTotal = 0;
    let outIdx = 0;
    let prevHigh = 0;
    let prevLow = 0;
    let prevClose = 0;
    let invPeriod = 0;
    let prevPlusDM = 0;
    let prevTR = 0;
    let tempReal = 0;
    let diffP = 0;
    let diffM = 0;
    /*
     * The DM1 (one period) is base on the largest part of
     * today's range that is outside of yesterdays range.
     *
     * The following 7 cases explain how the +DM and -DM are
     * calculated on one period:
     *
     * Case 1:                       Case 2:
     *    C|                        A|
     *     |                         | C|
     *     | +DM1 = (C-A)           B|  | +DM1 = 0
     *     | -DM1 = 0                   | -DM1 = (B-D)
     * A|  |                           D|
     *  | D|
     * B|
     *
     * Case 3:                       Case 4:
     *    C|                           C|
     *     |                        A|  |
     *     | +DM1 = (C-A)            |  | +DM1 = 0
     *     | -DM1 = 0               B|  | -DM1 = (B-D)
     * A|  |                            |
     *  |  |                           D|
     * B|  |
     *    D|
     *
     * Case 5:                      Case 6:
     * A|                           A| C|
     *  | C| +DM1 = 0                |  |  +DM1 = 0
     *  |  | -DM1 = 0                |  |  -DM1 = 0
     *  | D|                         |  |
     * B|                           B| D|
     *
     *
     * Case 7:
     *
     *    C|
     * A|  |
     *  |  | +DM1=0
     * B|  | -DM1=0
     *    D|
     *
     * In case 3 and 4, the rule is that the smallest delta between
     * (C-A) and (B-D) determine which of +DM or -DM is zero.
     *
     * In case 7, (C-A) and (B-D) are equal, so both +DM and -DM are
     * zero.
     *
     * The rules remain the same when A=B and C=D (when the highs
     * equal the lows).
     *
     * When calculating the DM over a period > 1, the one-period DM
     * for the desired period are initialy sum. In other word,
     * for a -DM14, sum the -DM1 for the first 14 days (that's
     * 13 values because there is no DM for the first day!)
     * Subsequent DM are calculated using the Wilder's
     * smoothing approach:
     *
     *                                    Previous +DM14
     *  Today's +DM14 = Previous +DM14 -  -------------- + Today's +DM1
     *                                         14
     *
     * Calculation of a +DI14 is as follow:
     *
     *               +DM14
     *     +DI14 =  --------
     *                TR14
     *
     * Calculation of the TR14 is:
     *
     *                                   Previous TR14
     *    Today's TR14 = Previous TR14 - -------------- + Today's TR1
     *                                         14
     *
     *    The first TR14 is the summation of the first 14 TR1. See the
     *    TA_TRANGE function on how to calculate the true range.
     *
     * Reference:
     *    New Concepts In Technical Trading Systems, J. Welles Wilder Jr
     */
    /* Original implementation from Wilder's book was doing some integer
     * rounding in its calculations.
     *
     * This was understandable in the context that at the time the book
     * was written, most user were doing the calculation by hand.
     *
     * For a computer, rounding is unnecessary (and even problematic when inputs
     * are close to 1).
     *
     * TA-Lib does not do the rounding. Still, if you want to reproduce Wilder's examples,
     * you can comment out the following #undef/#define and rebuild the library.
     */
    lookbackTotal = plusDiLookback(optInTimePeriod, core);
    /* Adjust startIdx to account for the lookback period. */
    if (startIdx < lookbackTotal) {
      startIdx = lookbackTotal;
    }
    /* Make sure there is still something to evaluate. */
    if (startIdx > endIdx) {
      outBegIdx.value = 0;
      outNBElement.value = 0;
      return RetCode.INSUFFICIENT_HISTORY;
    }
    /* Indicate where the next output should be put
     * in the outReal.
     */
    outIdx = 0;
    /* Trap the case where no smoothing is needed. */
    /* No smoothing needed. Just do the following:
     * for each price bar.
     *          +DM1
     *   +DI1 = ----
     *           TR1
     */
    outBegIdx.value = startIdx;
    today = startIdx - 1;
    prevHigh = inHigh[today];
    prevLow = inLow[today];
    prevClose = inClose[today];
    while (today < endIdx) {
      today += 1;
      tempReal = inHigh[today];
      diffP = tempReal - prevHigh;
      /* Plus Delta */
      prevHigh = tempReal;
      tempReal = inLow[today];
      diffM = prevLow - tempReal;
      /* Minus Delta */
      prevLow = tempReal;
      if (diffP > 0 && diffP > diffM) {
        /* Case 1 and 3: +DM=diffP,-DM=0 */
        let _true_range_4 = 0;
        let range_4 = prevHigh - prevLow;
        let tmp_4 = Math.abs(prevHigh - prevClose);
        if (tmp_4 > range_4) {
          range_4 = tmp_4;
        }
        tmp_4 = Math.abs(prevLow - prevClose);
        if (tmp_4 > range_4) {
          range_4 = tmp_4;
        }
        _true_range_4 = range_4;
        tempReal = _true_range_4;
        if (tempReal <= 0.0) {
          outReal[((outIdx++ * outStride) | 0)] = 0.0;
        } else {
          outReal[((outIdx++ * outStride) | 0)] = diffP / tempReal;
        }
      } else {
        outReal[((outIdx++ * outStride) | 0)] = 0.0;
      }
      prevClose = inClose[today];
    }
    outNBElement.value = outIdx;
    sp.optInTimePeriod = optInTimePeriod;
    sp.prevHigh = prevHigh;
    sp.prevLow = prevLow;
    sp.prevClose = prevClose;
    sp.invPeriod = invPeriod;
    sp.prevPlusDM = prevPlusDM;
    sp.prevTR = prevTR;
    sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
    return RetCode.SUCCESS;
  } else {
    let today = 0;
    let lookbackTotal = 0;
    let outIdx = 0;
    let prevHigh = 0;
    let prevLow = 0;
    let prevClose = 0;
    let invPeriod = 0;
    let prevPlusDM = 0;
    let prevTR = 0;
    let tempReal = 0;
    let diffP = 0;
    let diffM = 0;
    let plusDM1 = 0;
    let i = 0;
    /*
     * The DM1 (one period) is base on the largest part of
     * today's range that is outside of yesterdays range.
     *
     * The following 7 cases explain how the +DM and -DM are
     * calculated on one period:
     *
     * Case 1:                       Case 2:
     *    C|                        A|
     *     |                         | C|
     *     | +DM1 = (C-A)           B|  | +DM1 = 0
     *     | -DM1 = 0                   | -DM1 = (B-D)
     * A|  |                           D|
     *  | D|
     * B|
     *
     * Case 3:                       Case 4:
     *    C|                           C|
     *     |                        A|  |
     *     | +DM1 = (C-A)            |  | +DM1 = 0
     *     | -DM1 = 0               B|  | -DM1 = (B-D)
     * A|  |                            |
     *  |  |                           D|
     * B|  |
     *    D|
     *
     * Case 5:                      Case 6:
     * A|                           A| C|
     *  | C| +DM1 = 0                |  |  +DM1 = 0
     *  |  | -DM1 = 0                |  |  -DM1 = 0
     *  | D|                         |  |
     * B|                           B| D|
     *
     *
     * Case 7:
     *
     *    C|
     * A|  |
     *  |  | +DM1=0
     * B|  | -DM1=0
     *    D|
     *
     * In case 3 and 4, the rule is that the smallest delta between
     * (C-A) and (B-D) determine which of +DM or -DM is zero.
     *
     * In case 7, (C-A) and (B-D) are equal, so both +DM and -DM are
     * zero.
     *
     * The rules remain the same when A=B and C=D (when the highs
     * equal the lows).
     *
     * When calculating the DM over a period > 1, the one-period DM
     * for the desired period are initialy sum. In other word,
     * for a -DM14, sum the -DM1 for the first 14 days (that's
     * 13 values because there is no DM for the first day!)
     * Subsequent DM are calculated using the Wilder's
     * smoothing approach:
     *
     *                                    Previous +DM14
     *  Today's +DM14 = Previous +DM14 -  -------------- + Today's +DM1
     *                                         14
     *
     * Calculation of a +DI14 is as follow:
     *
     *               +DM14
     *     +DI14 =  --------
     *                TR14
     *
     * Calculation of the TR14 is:
     *
     *                                   Previous TR14
     *    Today's TR14 = Previous TR14 - -------------- + Today's TR1
     *                                         14
     *
     *    The first TR14 is the summation of the first 14 TR1. See the
     *    TA_TRANGE function on how to calculate the true range.
     *
     * Reference:
     *    New Concepts In Technical Trading Systems, J. Welles Wilder Jr
     */
    /* Original implementation from Wilder's book was doing some integer
     * rounding in its calculations.
     *
     * This was understandable in the context that at the time the book
     * was written, most user were doing the calculation by hand.
     *
     * For a computer, rounding is unnecessary (and even problematic when inputs
     * are close to 1).
     *
     * TA-Lib does not do the rounding. Still, if you want to reproduce Wilder's examples,
     * you can comment out the following #undef/#define and rebuild the library.
     */
    lookbackTotal = plusDiLookback(optInTimePeriod, core);
    /* Adjust startIdx to account for the lookback period. */
    if (startIdx < lookbackTotal) {
      startIdx = lookbackTotal;
    }
    /* Make sure there is still something to evaluate. */
    if (startIdx > endIdx) {
      outBegIdx.value = 0;
      outNBElement.value = 0;
      return RetCode.INSUFFICIENT_HISTORY;
    }
    /* Indicate where the next output should be put
     * in the outReal.
     */
    outIdx = 0;
    /* Trap the case where no smoothing is needed. */
    /* The declaration order above sets invPeriod's place in the stream state,
     * and that place is load-bearing: a layout that lets Update load it paired
     * with a field the previous bar stored stalls every call. Re-measure Update
     * in C and Rust before reordering those declarations.
     */
    invPeriod = 1.0 / optInTimePeriod;
    /* Process the initial DM and TR */
    today = startIdx;
    outBegIdx.value = today;
    prevPlusDM = 0.0;
    prevTR = 0.0;
    today = startIdx - lookbackTotal;
    prevHigh = inHigh[today];
    prevLow = inLow[today];
    prevClose = inClose[today];
    i = optInTimePeriod - 1;
    while (i-- > 0) {
      today += 1;
      tempReal = inHigh[today];
      diffP = tempReal - prevHigh;
      /* Plus Delta */
      prevHigh = tempReal;
      tempReal = inLow[today];
      diffM = prevLow - tempReal;
      /* Minus Delta */
      prevLow = tempReal;
      /* +DM1 = diffP when diffP > diffM and diffP > 0: the select takes the
       * first test and the max the second, as a non-positive delta cannot raise
       * the sum. gcc keeps a branch if a select, not the max, ends the step.
       */
      tempReal = diffP - diffM;
      plusDM1 = (tempReal > 0.0) ? diffP : 0.0;
      tempReal = prevPlusDM + plusDM1;
      prevPlusDM = (prevPlusDM > tempReal) ? prevPlusDM : tempReal;
      let _true_range_5 = 0;
      let range_5 = prevHigh - prevLow;
      let tmp_5 = Math.abs(prevHigh - prevClose);
      if (tmp_5 > range_5) {
        range_5 = tmp_5;
      }
      tmp_5 = Math.abs(prevLow - prevClose);
      if (tmp_5 > range_5) {
        range_5 = tmp_5;
      }
      _true_range_5 = range_5;
      tempReal = _true_range_5;
      prevTR += tempReal;
      prevClose = inClose[today];
    }
    /* Process subsequent DI */
    /* Skip the unstable period. Note that this loop must be executed
     * at least ONCE to calculate the first DI.
     */
    i = lookbackTotal - (optInTimePeriod - 1);
    while (i-- !== 0) {
      /* Calculate the prevPlusDM */
      today += 1;
      tempReal = inHigh[today];
      diffP = tempReal - prevHigh;
      /* Plus Delta */
      prevHigh = tempReal;
      tempReal = inLow[today];
      diffM = prevLow - tempReal;
      /* Minus Delta */
      prevLow = tempReal;
      tempReal = diffP - diffM;
      plusDM1 = (tempReal > 0.0) ? diffP : 0.0;
      tempReal = prevPlusDM - prevPlusDM * invPeriod;
      prevPlusDM = tempReal + plusDM1;
      prevPlusDM = (tempReal > prevPlusDM) ? tempReal : prevPlusDM;
      /* Calculate the prevTR */
      let _true_range_6 = 0;
      let range_6 = prevHigh - prevLow;
      let tmp_6 = Math.abs(prevHigh - prevClose);
      if (tmp_6 > range_6) {
        range_6 = tmp_6;
      }
      tmp_6 = Math.abs(prevLow - prevClose);
      if (tmp_6 > range_6) {
        range_6 = tmp_6;
      }
      _true_range_6 = range_6;
      tempReal = _true_range_6;
      prevTR = prevTR - prevTR * invPeriod + tempReal;
      prevClose = inClose[today];
    }
    /* Now start to write the output in
     * the caller provided outReal.
     */
    /* prevTR is a running sum of true ranges: non-negative by construction and
     * built only by adding, so it carries no cancellation residue and reaches
     * zero only for a window whose every range is exactly zero. Test it exactly.
     * A true range carries the quote unit, so the fixed TA_IS_ZERO band it used
     * to be compared against was a constant in some arbitrary unit, and zeroed
     * the index for any instrument quoted below it (issue #253).
     */
    if (prevTR > 0.0) {
      outReal[((0 * outStride) | 0)] = (100.0 * (prevPlusDM / prevTR));
    } else {
      outReal[((0 * outStride) | 0)] = 0.0;
    }
    outIdx = 1;
    while (today < endIdx) {
      /* Calculate the prevPlusDM */
      today += 1;
      tempReal = inHigh[today];
      diffP = tempReal - prevHigh;
      /* Plus Delta */
      prevHigh = tempReal;
      tempReal = inLow[today];
      diffM = prevLow - tempReal;
      /* Minus Delta */
      prevLow = tempReal;
      tempReal = diffP - diffM;
      plusDM1 = (tempReal > 0.0) ? diffP : 0.0;
      tempReal = prevPlusDM - prevPlusDM * invPeriod;
      prevPlusDM = tempReal + plusDM1;
      prevPlusDM = (tempReal > prevPlusDM) ? tempReal : prevPlusDM;
      /* Calculate the prevTR */
      let _true_range_7 = 0;
      let range_7 = prevHigh - prevLow;
      let tmp_7 = Math.abs(prevHigh - prevClose);
      if (tmp_7 > range_7) {
        range_7 = tmp_7;
      }
      tmp_7 = Math.abs(prevLow - prevClose);
      if (tmp_7 > range_7) {
        range_7 = tmp_7;
      }
      _true_range_7 = range_7;
      tempReal = _true_range_7;
      prevTR = prevTR - prevTR * invPeriod + tempReal;
      prevClose = inClose[today];
      /* Calculate the DI. The value is rounded (see Wilder book). */
      if (prevTR > 0.0) {
        outReal[((outIdx++ * outStride) | 0)] = (100.0 * (prevPlusDM / prevTR));
      } else {
        outReal[((outIdx++ * outStride) | 0)] = 0.0;
      }
    }
    outNBElement.value = outIdx;
    sp.optInTimePeriod = optInTimePeriod;
    sp.prevHigh = prevHigh;
    sp.prevLow = prevLow;
    sp.prevClose = prevClose;
    sp.invPeriod = invPeriod;
    sp.prevPlusDM = prevPlusDM;
    sp.prevTR = prevTR;
    sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
    return RetCode.SUCCESS;
  }
}

/** @internal */
export function plusDiOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): PlusDiStream {
  const sp = new PlusDiStream(core);
  const retCode = plusDiOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('PLUS_DI openAndFill', inHigh.length, startIdx, plusDiLookback(optInTimePeriod, core));
  }
  throw streamFailure('PLUS_DI openAndFill', retCode);
}

/** @internal */
export function plusDiOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): PlusDiStream {
  const sp = new PlusDiStream(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = plusDiOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('PLUS_DI open', inHigh.length, startIdx, plusDiLookback(optInTimePeriod, core));
  }
  throw streamFailure('PLUS_DI open', retCode);
}

/**
 * Opens a live PLUS_DI stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link plusDi} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Wilder smoothing period (default 14; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `plusDiLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { plusDiOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = plusDiOpen(high, low, close, 14);
 * const peeked = stream.peek(101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function plusDiOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): PlusDiStream {
  optInTimePeriod = requireInt32('PLUS_DI open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('PLUS_DI open', 'inHigh', inHigh);
  requireHistory('PLUS_DI open', inHigh.length);
  requireArgument('PLUS_DI open', 'inLow', inLow);
  requireArgument('PLUS_DI open', 'inClose', inClose);
  requireHistoryLength('PLUS_DI open', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('PLUS_DI open', 'inClose', inClose.length, inHigh.length);
  return plusDiOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInTimePeriod);
}

/**
 * {@link plusDiOpen} that also fills the outputs over the whole history in the
 * same pass, bit-identically to {@link plusDi}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Wilder smoothing period (default 14; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Plus Directional Indicator. Must hold `historyLen - lookback`
 *   values and share no memory with an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `plusDiLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range, histories of different lengths or an output too
 *   short or starting where another output or a `Float64Array` input starts, or, on
 *   a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { plusDiOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const out = new Float64Array(high.length);
 * const stream = plusDiOpenAndFill(high, low, close, 14, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function plusDiOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): PlusDiStream {
  optInTimePeriod = requireInt32('PLUS_DI openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('PLUS_DI openAndFill', 'inHigh', inHigh);
  requireHistory('PLUS_DI openAndFill', inHigh.length);
  requireArgument('PLUS_DI openAndFill', 'inLow', inLow);
  requireArgument('PLUS_DI openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('PLUS_DI openAndFill', inHigh.length, plusDiLookback(optInTimePeriod, core));
  requireHistoryLength('PLUS_DI openAndFill', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('PLUS_DI openAndFill', 'inClose', inClose.length, inHigh.length);
  requireLength('PLUS_DI openAndFill', 'outReal', outReal, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) || (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) || (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset)) {
    throw streamFailure('PLUS_DI openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inHighF64) || overlaps(outReal, inLowF64) || overlaps(outReal, inCloseF64))) {
    throw streamFailure('PLUS_DI openAndFill', RetCode.BAD_PARAM);
  }
  return plusDiOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}
