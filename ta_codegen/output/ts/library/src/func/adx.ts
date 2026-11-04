/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AM       Adrian Michel
 *  MIF      Mirek Fontan (mira@fontan.cz)
 *  GC       guycom@users.sourceforge.net
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
 *  082206 MF    Fix #1544555. Div by zero bug reported by GC.
 *  082326 MF,CC Fix #253. Test the true-range sum exactly instead of against
 *               the fixed TA_IS_ZERO band, which zeroed the index for any
 *               instrument quoted small enough to fall under it.
 *  091326 MF,CC #411 Wilder steps without a divide or a branch.
 */

import { FuncUnstId } from '../enums.ts';
import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link adx} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param optInTimePeriod - Smoothing/averaging period for DM, TR, and ADX (default
 *   14; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function adxLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  return ((2 * optInTimePeriod) | 0) + core.unstableCount(FuncUnstId.ADX, (((10 + 6) * optInTimePeriod) | 0), (((19 + 6) * optInTimePeriod) | 0)) - 1;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link adx}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Smoothing/averaging period for DM, TR, and ADX (default
 *   14; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function adxDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (adxLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function adxImpl(
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
  let invPeriod = 0;
  let prevClose = 0;
  let prevMinusDM = 0;
  let prevPlusDM = 0;
  let prevTR = 0;
  let tempReal = 0;
  let diffP = 0;
  let diffM = 0;
  let minusSel = 0;
  let plusSel = 0;
  let minusDM1 = 0;
  let plusDM1 = 0;
  let minusDI = 0;
  let plusDI = 0;
  let sumDX = 0;
  let prevADX = 0;
  let i = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
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
   *  |  | +DM=0
   * B|  | -DM=0
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
   *                                    Previous -DM14
   *  Today's -DM14 = Previous -DM14 -  -------------- + Today's -DM1
   *                                         14
   *
   * (Same thing for +DM14)
   *
   * Calculation of a -DI14 is as follow:
   *
   *               -DM14
   *     -DI14 =  --------
   *                TR14
   *
   * (Same thing for +DI14)
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
   * Calculation of the DX14 is:
   *
   *    diffDI = ABS( (-DI14) - (+DI14) )
   *    sumDI  = (-DI14) + (+DI14)
   *
   *    DX14 = 100 * (diffDI / sumDI)
   *
   * Calculation of the first ADX:
   *
   *    ADX14 = SUM of the first 14 DX
   *
   * Calculation of subsequent ADX:
   *
   *            ((Previous ADX14)*(14-1))+ Today's DX
   *    ADX14 = -------------------------------------
   *                             14
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
  lookbackTotal = adxLookback(optInTimePeriod, core);
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
  /* Process the initial DM and TR */
  today = startIdx;
  outBegIdx.value = today;
  prevMinusDM = 0.0;
  prevPlusDM = 0.0;
  prevTR = 0.0;
  /* The declaration order above sets invPeriod's place in the stream state,
   * and that place is load-bearing: a layout that lets Update load it paired
   * with a field the previous bar stored stalls every call. Re-measure Update
   * in C and Rust before reordering those declarations.
   */
  invPeriod = 1.0 / optInTimePeriod;
  today = startIdx - lookbackTotal;
  prevHigh = inHigh[today];
  prevLow = inLow[today];
  prevClose = inClose[today];
  i = optInTimePeriod - 1;
  while (i-- > 0) {
    /* Calculate the prevMinusDM and prevPlusDM */
    today += 1;
    tempReal = inHigh[today];
    diffP = tempReal - prevHigh;
    /* Plus Delta */
    prevHigh = tempReal;
    tempReal = inLow[today];
    diffM = prevLow - tempReal;
    /* Minus Delta */
    prevLow = tempReal;
    /* Gate on the leg subtraction first, then on the sign: comparing diffM > diffP
     * directly, or testing the sign first, lets gcc thread the selects back into
     * branches. a-b > 0.0 is exactly a > b for any in-domain difference.
     */
    minusSel = (diffM - diffP > 0.0) ? diffM : 0.0;
    plusSel = (diffP - diffM > 0.0) ? diffP : 0.0;
    minusDM1 = (diffM > 0.0) ? minusSel : 0.0;
    plusDM1 = (diffP > 0.0) ? plusSel : 0.0;
    prevMinusDM += minusDM1;
    prevPlusDM += plusDM1;
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
    prevTR += tempReal;
    prevClose = inClose[today];
  }
  /* Add up all the initial DX. */
  sumDX = 0.0;
  i = optInTimePeriod;
  while (i-- > 0) {
    /* Calculate the prevMinusDM and prevPlusDM */
    today += 1;
    tempReal = inHigh[today];
    diffP = tempReal - prevHigh;
    /* Plus Delta */
    prevHigh = tempReal;
    tempReal = inLow[today];
    diffM = prevLow - tempReal;
    /* Minus Delta */
    prevLow = tempReal;
    prevMinusDM -= prevMinusDM * invPeriod;
    prevPlusDM -= prevPlusDM * invPeriod;
    minusSel = (diffM - diffP > 0.0) ? diffM : 0.0;
    plusSel = (diffP - diffM > 0.0) ? diffP : 0.0;
    minusDM1 = (diffM > 0.0) ? minusSel : 0.0;
    plusDM1 = (diffP > 0.0) ? plusSel : 0.0;
    prevMinusDM += minusDM1;
    prevPlusDM += plusDM1;
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
    prevTR = prevTR - prevTR * invPeriod + tempReal;
    prevClose = inClose[today];
    /* Calculate the DX. The value is rounded (see Wilder book).
     *
     * prevTR is a running sum of true ranges: non-negative by construction
     * and built only by adding, so it carries no cancellation residue and
     * reaches zero only for a window whose every range is exactly zero. Test
     * it exactly. A true range carries the quote unit, so the fixed
     * TA_IS_ZERO band it used to be compared against was a constant in some
     * arbitrary unit, and zeroed the index for any instrument quoted below
     * it (issue #253). The DI legs it feeds are ratios -- dimensionless --
     * so the fixed band on THEIR sum is scale-invariant and stays.
     */
    if (prevTR > 0.0) {
      minusDI = (100.0 * (prevMinusDM / prevTR));
      plusDI = (100.0 * (prevPlusDM / prevTR));
      /* This loop is just to accumulate the initial DX */
      tempReal = minusDI + plusDI;
      if (!((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001))) {
        sumDX += (100.0 * (Math.abs(minusDI - plusDI) / tempReal));
      }
    }
  }
  /* Calculate the first ADX */
  prevADX = (sumDX / optInTimePeriod);
  /* Skip the unstable period */
  i = lookbackTotal - (((2 * optInTimePeriod) | 0) - 1);
  while (i-- > 0) {
    /* Calculate the prevMinusDM and prevPlusDM */
    today += 1;
    tempReal = inHigh[today];
    diffP = tempReal - prevHigh;
    /* Plus Delta */
    prevHigh = tempReal;
    tempReal = inLow[today];
    diffM = prevLow - tempReal;
    /* Minus Delta */
    prevLow = tempReal;
    prevMinusDM -= prevMinusDM * invPeriod;
    prevPlusDM -= prevPlusDM * invPeriod;
    minusSel = (diffM - diffP > 0.0) ? diffM : 0.0;
    plusSel = (diffP - diffM > 0.0) ? diffP : 0.0;
    minusDM1 = (diffM > 0.0) ? minusSel : 0.0;
    plusDM1 = (diffP > 0.0) ? plusSel : 0.0;
    prevMinusDM += minusDM1;
    prevPlusDM += plusDM1;
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
    if (prevTR > 0.0) {
      /* Calculate the DX. The value is rounded (see Wilder book). */
      minusDI = (100.0 * (prevMinusDM / prevTR));
      plusDI = (100.0 * (prevPlusDM / prevTR));
      tempReal = minusDI + plusDI;
      if (!((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001))) {
        tempReal = (100.0 * (Math.abs(minusDI - plusDI) / tempReal));
        /* Calculate the ADX */
        prevADX = (prevADX - (prevADX - tempReal) * invPeriod);
      }
    }
  }
  /* Output the first ADX */
  outReal[0] = prevADX;
  outIdx = 1;
  /* Calculate and output subsequent ADX */
  while (today < endIdx) {
    /* Calculate the prevMinusDM and prevPlusDM */
    today += 1;
    tempReal = inHigh[today];
    diffP = tempReal - prevHigh;
    /* Plus Delta */
    prevHigh = tempReal;
    tempReal = inLow[today];
    diffM = prevLow - tempReal;
    /* Minus Delta */
    prevLow = tempReal;
    prevMinusDM -= prevMinusDM * invPeriod;
    prevPlusDM -= prevPlusDM * invPeriod;
    minusSel = (diffM - diffP > 0.0) ? diffM : 0.0;
    plusSel = (diffP - diffM > 0.0) ? diffP : 0.0;
    minusDM1 = (diffM > 0.0) ? minusSel : 0.0;
    plusDM1 = (diffP > 0.0) ? plusSel : 0.0;
    prevMinusDM += minusDM1;
    prevPlusDM += plusDM1;
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
    if (prevTR > 0.0) {
      /* Calculate the DX. The value is rounded (see Wilder book). */
      minusDI = (100.0 * (prevMinusDM / prevTR));
      plusDI = (100.0 * (prevPlusDM / prevTR));
      tempReal = minusDI + plusDI;
      if (!((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001))) {
        tempReal = (100.0 * (Math.abs(minusDI - plusDI) / tempReal));
        /* Calculate the ADX */
        prevADX = (prevADX - (prevADX - tempReal) * invPeriod);
      }
    }
    /* Output the ADX */
    outReal[outIdx++] = prevADX;
  }
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * Wilder's Average Directional Movement Index, a smoothed measure of trend
 * strength derived from the directional indicators (+DI/-DI). Quantifies how
 * strongly a market is trending, regardless of direction. Higher values indicate a
 * stronger trend (a common convention treats >25 as trending); says nothing about
 * direction.
 *
 * Formula and more info at
 * [ta-lib.org/functions/adx](https://ta-lib.org/functions/adx).
 *
 * **Notes**
 * - Wilder's original integer rounding is not applied.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link adxLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param optInTimePeriod - Smoothing/averaging period for DM, TR, and ADX (default
 *   14; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Smoothed directional trend-strength index (0-100) Must hold the
 *   values produced and never be empty; the input length always suffices.
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
 * import { adx } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const out = new Float64Array(high.length);
 * const { begIdx, count } = adx(0, high.length - 1, high, low, close, 14, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`adxr`](https://ta-lib.org/functions/adxr)
 * @see [`dx`](https://ta-lib.org/functions/dx)
 * @see [`plusDi`](https://ta-lib.org/functions/plus_di)
 * @see [`minusDi`](https://ta-lib.org/functions/minus_di)
 * @see [`plusDm`](https://ta-lib.org/functions/plus_dm)
 * @see [`minusDm`](https://ta-lib.org/functions/minus_dm)
 * @see [`trange`](https://ta-lib.org/functions/trange)
 */
export function adx(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('ADX', 'startIdx', startIdx);
  endIdx = requireInt32('ADX', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('ADX', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('ADX', startIdx, endIdx);
  const guardStart = clampedStart('ADX', startIdx, adxLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('ADX', 'inHigh', inHigh, guardInLen);
  requireLength('ADX', 'inLow', inLow, guardInLen);
  requireLength('ADX', 'inClose', inClose, guardInLen);
  requireOutput('ADX', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outReal, inHighF64) || overlapsPartly(outReal, inLowF64) || overlapsPartly(outReal, inCloseF64)) {
    throw failure('ADX', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = adxImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('ADX', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live ADX stream: one value per closed bar, bit-identical to {@link adx} over
 * the same series. Open it with {@link adxOpen} or {@link adxOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/adx](https://ta-lib.org/functions/adx).
 */
export class AdxStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  prevHigh: number = 0;
  /** @internal */
  prevLow: number = 0;
  /** @internal */
  invPeriod: number = 0;
  /** @internal */
  prevClose: number = 0;
  /** @internal */
  prevMinusDM: number = 0;
  /** @internal */
  prevPlusDM: number = 0;
  /** @internal */
  prevTR: number = 0;
  /** @internal */
  prevADX: number = 0;
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
  static of(core: Core): AdxStream {
    return new AdxStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link adx} reports them over the same bars. Each accepted `update` and each
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
      throw failure('ADX advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw failure('ADX update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('ADX update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    adxStepImpl(this, inHigh, inLow, inClose);
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
      throw nonFiniteBar('ADX peek', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let tempReal = 0;
    let diffP = 0;
    let diffM = 0;
    let minusSel = 0;
    let plusSel = 0;
    let minusDM1 = 0;
    let plusDM1 = 0;
    let minusDI = 0;
    let plusDI = 0;
    let cur_outReal = 0;
    let prevADX = sp.prevADX;
    let prevClose = sp.prevClose;
    let prevHigh = sp.prevHigh;
    let prevLow = sp.prevLow;
    let prevMinusDM = sp.prevMinusDM;
    let prevPlusDM = sp.prevPlusDM;
    let prevTR = sp.prevTR;
    /* Calculate the prevMinusDM and prevPlusDM */
    tempReal = inHigh;
    diffP = tempReal - prevHigh;
    /* Plus Delta */
    prevHigh = tempReal;
    tempReal = inLow;
    diffM = prevLow - tempReal;
    /* Minus Delta */
    prevLow = tempReal;
    prevMinusDM -= prevMinusDM * sp.invPeriod;
    prevPlusDM -= prevPlusDM * sp.invPeriod;
    minusSel = (diffM - diffP > 0.0) ? diffM : 0.0;
    plusSel = (diffP - diffM > 0.0) ? diffP : 0.0;
    minusDM1 = (diffM > 0.0) ? minusSel : 0.0;
    plusDM1 = (diffP > 0.0) ? plusSel : 0.0;
    prevMinusDM += minusDM1;
    prevPlusDM += plusDM1;
    /* Calculate the prevTR */
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
    prevTR = prevTR - prevTR * sp.invPeriod + tempReal;
    prevClose = inClose;
    if (prevTR > 0.0) {
      /* Calculate the DX. The value is rounded (see Wilder book). */
      minusDI = (100.0 * (prevMinusDM / prevTR));
      plusDI = (100.0 * (prevPlusDM / prevTR));
      tempReal = minusDI + plusDI;
      if (!((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001))) {
        tempReal = (100.0 * (Math.abs(minusDI - plusDI) / tempReal));
        /* Calculate the ADX */
        prevADX = (prevADX - (prevADX - tempReal) * sp.invPeriod);
      }
    }
    /* Output the ADX */
    cur_outReal = prevADX;
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
  clone(): AdxStream {
    const c = new AdxStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.prevHigh = this.prevHigh;
    c.prevLow = this.prevLow;
    c.invPeriod = this.invPeriod;
    c.prevClose = this.prevClose;
    c.prevMinusDM = this.prevMinusDM;
    c.prevPlusDM = this.prevPlusDM;
    c.prevTR = this.prevTR;
    c.prevADX = this.prevADX;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function adxStepImpl(sp: AdxStream, inHigh: number, inLow: number, inClose: number): void {
  let tempReal = 0;
  let diffP = 0;
  let diffM = 0;
  let minusSel = 0;
  let plusSel = 0;
  let minusDM1 = 0;
  let plusDM1 = 0;
  let minusDI = 0;
  let plusDI = 0;
  /* Calculate the prevMinusDM and prevPlusDM */
  tempReal = inHigh;
  diffP = tempReal - sp.prevHigh;
  /* Plus Delta */
  sp.prevHigh = tempReal;
  tempReal = inLow;
  diffM = sp.prevLow - tempReal;
  /* Minus Delta */
  sp.prevLow = tempReal;
  sp.prevMinusDM -= sp.prevMinusDM * sp.invPeriod;
  sp.prevPlusDM -= sp.prevPlusDM * sp.invPeriod;
  minusSel = (diffM - diffP > 0.0) ? diffM : 0.0;
  plusSel = (diffP - diffM > 0.0) ? diffP : 0.0;
  minusDM1 = (diffM > 0.0) ? minusSel : 0.0;
  plusDM1 = (diffP > 0.0) ? plusSel : 0.0;
  sp.prevMinusDM += minusDM1;
  sp.prevPlusDM += plusDM1;
  /* Calculate the prevTR */
  let _true_range_1 = 0;
  let range_1 = sp.prevHigh - sp.prevLow;
  let tmp_1 = Math.abs(sp.prevHigh - sp.prevClose);
  if (tmp_1 > range_1) {
    range_1 = tmp_1;
  }
  tmp_1 = Math.abs(sp.prevLow - sp.prevClose);
  if (tmp_1 > range_1) {
    range_1 = tmp_1;
  }
  _true_range_1 = range_1;
  tempReal = _true_range_1;
  sp.prevTR = sp.prevTR - sp.prevTR * sp.invPeriod + tempReal;
  sp.prevClose = inClose;
  if (sp.prevTR > 0.0) {
    /* Calculate the DX. The value is rounded (see Wilder book). */
    minusDI = (100.0 * (sp.prevMinusDM / sp.prevTR));
    plusDI = (100.0 * (sp.prevPlusDM / sp.prevTR));
    tempReal = minusDI + plusDI;
    if (!((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001))) {
      tempReal = (100.0 * (Math.abs(minusDI - plusDI) / tempReal));
      /* Calculate the ADX */
      sp.prevADX = (sp.prevADX - (sp.prevADX - tempReal) * sp.invPeriod);
    }
  }
  /* Output the ADX */
  sp.cur_outReal = sp.prevADX;
}

function adxOpenImpl(
  sp: AdxStream,
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
  let today = 0;
  let lookbackTotal = 0;
  let outIdx = 0;
  let prevHigh = 0;
  let prevLow = 0;
  let invPeriod = 0;
  let prevClose = 0;
  let prevMinusDM = 0;
  let prevPlusDM = 0;
  let prevTR = 0;
  let tempReal = 0;
  let diffP = 0;
  let diffM = 0;
  let minusSel = 0;
  let plusSel = 0;
  let minusDM1 = 0;
  let plusDM1 = 0;
  let minusDI = 0;
  let plusDI = 0;
  let sumDX = 0;
  let prevADX = 0;
  let i = 0;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length || inClose.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
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
   *  |  | +DM=0
   * B|  | -DM=0
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
   *                                    Previous -DM14
   *  Today's -DM14 = Previous -DM14 -  -------------- + Today's -DM1
   *                                         14
   *
   * (Same thing for +DM14)
   *
   * Calculation of a -DI14 is as follow:
   *
   *               -DM14
   *     -DI14 =  --------
   *                TR14
   *
   * (Same thing for +DI14)
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
   * Calculation of the DX14 is:
   *
   *    diffDI = ABS( (-DI14) - (+DI14) )
   *    sumDI  = (-DI14) + (+DI14)
   *
   *    DX14 = 100 * (diffDI / sumDI)
   *
   * Calculation of the first ADX:
   *
   *    ADX14 = SUM of the first 14 DX
   *
   * Calculation of subsequent ADX:
   *
   *            ((Previous ADX14)*(14-1))+ Today's DX
   *    ADX14 = -------------------------------------
   *                             14
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
  lookbackTotal = adxLookback(optInTimePeriod, core);
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
  /* Process the initial DM and TR */
  today = startIdx;
  outBegIdx.value = today;
  prevMinusDM = 0.0;
  prevPlusDM = 0.0;
  prevTR = 0.0;
  /* The declaration order above sets invPeriod's place in the stream state,
   * and that place is load-bearing: a layout that lets Update load it paired
   * with a field the previous bar stored stalls every call. Re-measure Update
   * in C and Rust before reordering those declarations.
   */
  invPeriod = 1.0 / optInTimePeriod;
  today = startIdx - lookbackTotal;
  prevHigh = inHigh[today];
  prevLow = inLow[today];
  prevClose = inClose[today];
  i = optInTimePeriod - 1;
  while (i-- > 0) {
    /* Calculate the prevMinusDM and prevPlusDM */
    today += 1;
    tempReal = inHigh[today];
    diffP = tempReal - prevHigh;
    /* Plus Delta */
    prevHigh = tempReal;
    tempReal = inLow[today];
    diffM = prevLow - tempReal;
    /* Minus Delta */
    prevLow = tempReal;
    /* Gate on the leg subtraction first, then on the sign: comparing diffM > diffP
     * directly, or testing the sign first, lets gcc thread the selects back into
     * branches. a-b > 0.0 is exactly a > b for any in-domain difference.
     */
    minusSel = (diffM - diffP > 0.0) ? diffM : 0.0;
    plusSel = (diffP - diffM > 0.0) ? diffP : 0.0;
    minusDM1 = (diffM > 0.0) ? minusSel : 0.0;
    plusDM1 = (diffP > 0.0) ? plusSel : 0.0;
    prevMinusDM += minusDM1;
    prevPlusDM += plusDM1;
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
    prevTR += tempReal;
    prevClose = inClose[today];
  }
  /* Add up all the initial DX. */
  sumDX = 0.0;
  i = optInTimePeriod;
  while (i-- > 0) {
    /* Calculate the prevMinusDM and prevPlusDM */
    today += 1;
    tempReal = inHigh[today];
    diffP = tempReal - prevHigh;
    /* Plus Delta */
    prevHigh = tempReal;
    tempReal = inLow[today];
    diffM = prevLow - tempReal;
    /* Minus Delta */
    prevLow = tempReal;
    prevMinusDM -= prevMinusDM * invPeriod;
    prevPlusDM -= prevPlusDM * invPeriod;
    minusSel = (diffM - diffP > 0.0) ? diffM : 0.0;
    plusSel = (diffP - diffM > 0.0) ? diffP : 0.0;
    minusDM1 = (diffM > 0.0) ? minusSel : 0.0;
    plusDM1 = (diffP > 0.0) ? plusSel : 0.0;
    prevMinusDM += minusDM1;
    prevPlusDM += plusDM1;
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
    /* Calculate the DX. The value is rounded (see Wilder book).
     *
     * prevTR is a running sum of true ranges: non-negative by construction
     * and built only by adding, so it carries no cancellation residue and
     * reaches zero only for a window whose every range is exactly zero. Test
     * it exactly. A true range carries the quote unit, so the fixed
     * TA_IS_ZERO band it used to be compared against was a constant in some
     * arbitrary unit, and zeroed the index for any instrument quoted below
     * it (issue #253). The DI legs it feeds are ratios -- dimensionless --
     * so the fixed band on THEIR sum is scale-invariant and stays.
     */
    if (prevTR > 0.0) {
      minusDI = (100.0 * (prevMinusDM / prevTR));
      plusDI = (100.0 * (prevPlusDM / prevTR));
      /* This loop is just to accumulate the initial DX */
      tempReal = minusDI + plusDI;
      if (!((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001))) {
        sumDX += (100.0 * (Math.abs(minusDI - plusDI) / tempReal));
      }
    }
  }
  /* Calculate the first ADX */
  prevADX = (sumDX / optInTimePeriod);
  /* Skip the unstable period */
  i = lookbackTotal - (((2 * optInTimePeriod) | 0) - 1);
  while (i-- > 0) {
    /* Calculate the prevMinusDM and prevPlusDM */
    today += 1;
    tempReal = inHigh[today];
    diffP = tempReal - prevHigh;
    /* Plus Delta */
    prevHigh = tempReal;
    tempReal = inLow[today];
    diffM = prevLow - tempReal;
    /* Minus Delta */
    prevLow = tempReal;
    prevMinusDM -= prevMinusDM * invPeriod;
    prevPlusDM -= prevPlusDM * invPeriod;
    minusSel = (diffM - diffP > 0.0) ? diffM : 0.0;
    plusSel = (diffP - diffM > 0.0) ? diffP : 0.0;
    minusDM1 = (diffM > 0.0) ? minusSel : 0.0;
    plusDM1 = (diffP > 0.0) ? plusSel : 0.0;
    prevMinusDM += minusDM1;
    prevPlusDM += plusDM1;
    /* Calculate the prevTR */
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
    prevTR = prevTR - prevTR * invPeriod + tempReal;
    prevClose = inClose[today];
    if (prevTR > 0.0) {
      /* Calculate the DX. The value is rounded (see Wilder book). */
      minusDI = (100.0 * (prevMinusDM / prevTR));
      plusDI = (100.0 * (prevPlusDM / prevTR));
      tempReal = minusDI + plusDI;
      if (!((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001))) {
        tempReal = (100.0 * (Math.abs(minusDI - plusDI) / tempReal));
        /* Calculate the ADX */
        prevADX = (prevADX - (prevADX - tempReal) * invPeriod);
      }
    }
  }
  /* Output the first ADX */
  outReal[((0 * outStride) | 0)] = prevADX;
  outIdx = 1;
  /* Calculate and output subsequent ADX */
  while (today < endIdx) {
    /* Calculate the prevMinusDM and prevPlusDM */
    today += 1;
    tempReal = inHigh[today];
    diffP = tempReal - prevHigh;
    /* Plus Delta */
    prevHigh = tempReal;
    tempReal = inLow[today];
    diffM = prevLow - tempReal;
    /* Minus Delta */
    prevLow = tempReal;
    prevMinusDM -= prevMinusDM * invPeriod;
    prevPlusDM -= prevPlusDM * invPeriod;
    minusSel = (diffM - diffP > 0.0) ? diffM : 0.0;
    plusSel = (diffP - diffM > 0.0) ? diffP : 0.0;
    minusDM1 = (diffM > 0.0) ? minusSel : 0.0;
    plusDM1 = (diffP > 0.0) ? plusSel : 0.0;
    prevMinusDM += minusDM1;
    prevPlusDM += plusDM1;
    /* Calculate the prevTR */
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
    prevTR = prevTR - prevTR * invPeriod + tempReal;
    prevClose = inClose[today];
    if (prevTR > 0.0) {
      /* Calculate the DX. The value is rounded (see Wilder book). */
      minusDI = (100.0 * (prevMinusDM / prevTR));
      plusDI = (100.0 * (prevPlusDM / prevTR));
      tempReal = minusDI + plusDI;
      if (!((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001))) {
        tempReal = (100.0 * (Math.abs(minusDI - plusDI) / tempReal));
        /* Calculate the ADX */
        prevADX = (prevADX - (prevADX - tempReal) * invPeriod);
      }
    }
    /* Output the ADX */
    outReal[((outIdx++ * outStride) | 0)] = prevADX;
  }
  outNBElement.value = outIdx;
  sp.optInTimePeriod = optInTimePeriod;
  sp.prevHigh = prevHigh;
  sp.prevLow = prevLow;
  sp.invPeriod = invPeriod;
  sp.prevClose = prevClose;
  sp.prevMinusDM = prevMinusDM;
  sp.prevPlusDM = prevPlusDM;
  sp.prevTR = prevTR;
  sp.prevADX = prevADX;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function adxOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): AdxStream {
  const sp = AdxStream.of(core);
  const retCode = adxOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ADX openAndFill', inHigh.length, startIdx, adxLookback(optInTimePeriod, core));
  }
  throw streamFailure('ADX openAndFill', retCode);
}

/** @internal */
export function adxOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): AdxStream {
  const sp = AdxStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = adxOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ADX open', inHigh.length, startIdx, adxLookback(optInTimePeriod, core));
  }
  throw streamFailure('ADX open', retCode);
}

/**
 * Opens a live ADX stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link adx} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Smoothing/averaging period for DM, TR, and ADX (default
 *   14; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `adxLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   an integer parameter that is not an int32, a parameter outside its range or
 *   histories of different lengths.
 *
 * @example
 * ```ts
 * import { adxOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = adxOpen(high, low, close, 14);
 * const peeked = stream.peek(101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function adxOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): AdxStream {
  optInTimePeriod = requireInt32('ADX open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('ADX open', 'inHigh', inHigh);
  requireHistory('ADX open', inHigh.length);
  requireArgument('ADX open', 'inLow', inLow);
  requireArgument('ADX open', 'inClose', inClose);
  requireHistoryLength('ADX open', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('ADX open', 'inClose', inClose.length, inHigh.length);
  return adxOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInTimePeriod);
}

/**
 * {@link adxOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link adx}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Smoothing/averaging period for DM, TR, and ADX (default
 *   14; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Smoothed directional trend-strength index (0-100) Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `adxLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or an output too short or starting where another
 *   output or a `Float64Array` input starts, or, on a history long enough to open,
 *   one sharing memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { adxOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const out = new Float64Array(high.length);
 * const stream = adxOpenAndFill(high, low, close, 14, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function adxOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): AdxStream {
  optInTimePeriod = requireInt32('ADX openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('ADX openAndFill', 'inHigh', inHigh);
  requireHistory('ADX openAndFill', inHigh.length);
  requireArgument('ADX openAndFill', 'inLow', inLow);
  requireArgument('ADX openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('ADX openAndFill', inHigh.length, adxLookback(optInTimePeriod, core));
  requireHistoryLength('ADX openAndFill', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('ADX openAndFill', 'inClose', inClose.length, inHigh.length);
  requireOutput('ADX openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) || (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) || (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset)) {
    throw streamFailure('ADX openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inHighF64) || overlaps(outReal, inLowF64) || overlaps(outReal, inCloseF64))) {
    throw streamFailure('ADX openAndFill', RetCode.BAD_PARAM);
  }
  return adxOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}
