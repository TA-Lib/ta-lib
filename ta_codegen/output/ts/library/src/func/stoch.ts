/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY    Description
 *  -------------------------------------------------------------------
 *  112400 MF    Template creation.
 *  052603 MF    Adapt code to compile with .NET Managed C++
 *  071026 MF,CC Fix #107. Guard the Fast-K division with TA_IS_ZERO, not an
 *               exact `diff != 0.0`, so a machine-flat window yields 0 instead
 *               of dividing a sub-epsilon residue into [0,100] noise (STOCHRSI).
 *  072026 MF,CC Fix #130. Never elect outSlowD as the K scratch buffer: %D's
 *               in-place ma() destroyed the smoothed K before the final copy.
 *  082326 MF,CC Fix #253. Scale that guard to the window's own extremes: the
 *               fixed band zeroed the whole output for any instrument quoted
 *               small enough to fall under it.
 *  082726 MF,CC Fix #269. Answer a rejected %D ma() before the copy, not after:
 *               the stale *outNBElement overran outSlowK by lookbackDSlow.
 *  090626 MF,CC Fix #390. Divide by the range, scale after: the hoisted
 *               `(highest-lowest)/100.0` underflowed to 0.0 on a denormal
 *               range that the guard still called "not flat".
 */

import { MAType, MATypes } from '../enums.ts';
import { badSink, clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { MaStream, ma, maLookback, maOpenAndFillInternal, maOpenInternal } from './ma.ts';

/**
 * Number of leading input bars {@link stoch} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInFastK_Period - Lookback window for the raw %K high-low range
 *   (default 5; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowK_Period - Smoothing period turning FastK into SlowK (default 3;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowK_MAType - MA type used to smooth into SlowK (default
 *   `MAType.SMA`; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
 *   7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA,
 *   15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param optInSlowD_Period - Smoothing period for the SlowD signal line (default
 *   3; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowD_MAType - MA type used for the SlowD line (default
 *   `MAType.SMA`; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
 *   7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA,
 *   15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function stochLookback(optInFastK_Period: number, optInSlowK_Period: number, optInSlowK_MAType: MAType | typeof Core.INTEGER_DEFAULT, optInSlowD_Period: number, optInSlowD_MAType: MAType | typeof Core.INTEGER_DEFAULT, core: Core = Core.DEFAULT): number {
  if ((optInFastK_Period | 0) !== optInFastK_Period) {
    return -1;
  }
  if ((optInSlowK_Period | 0) !== optInSlowK_Period) {
    return -1;
  }
  if ((optInSlowK_MAType | 0) !== optInSlowK_MAType) {
    return -1;
  }
  if ((optInSlowD_Period | 0) !== optInSlowD_Period) {
    return -1;
  }
  if ((optInSlowD_MAType | 0) !== optInSlowD_MAType) {
    return -1;
  }
  if (optInFastK_Period === Core.INTEGER_DEFAULT) {
    optInFastK_Period = 5;
  } else if (optInFastK_Period < 1 || optInFastK_Period > 100000) {
    return -1;
  }
  if (optInSlowK_Period === Core.INTEGER_DEFAULT) {
    optInSlowK_Period = 3;
  } else if (optInSlowK_Period < 1 || optInSlowK_Period > 100000) {
    return -1;
  }
  if (optInSlowK_MAType === Core.INTEGER_DEFAULT || optInSlowK_MAType === MAType.DEFAULT) {
    optInSlowK_MAType = MAType.SMA;
  } else if (optInSlowK_MAType < MATypes.Min || optInSlowK_MAType > MATypes.Max) {
    return -1;
  }
  if (optInSlowD_Period === Core.INTEGER_DEFAULT) {
    optInSlowD_Period = 3;
  } else if (optInSlowD_Period < 1 || optInSlowD_Period > 100000) {
    return -1;
  }
  if (optInSlowD_MAType === Core.INTEGER_DEFAULT || optInSlowD_MAType === MAType.DEFAULT) {
    optInSlowD_MAType = MAType.SMA;
  } else if (optInSlowD_MAType < MATypes.Min || optInSlowD_MAType > MATypes.Max) {
    return -1;
  }
  let retValue = 0;
  /* Account for the initial data needed for Fast-K. */
  retValue = optInFastK_Period - 1;
  /* Add the smoothing being done for %K slow */
  retValue += maLookback(optInSlowK_Period, (optInSlowK_MAType as MAType), core);
  /* Add the smoothing being done for %D slow. */
  retValue += maLookback(optInSlowD_Period, (optInSlowD_MAType as MAType), core);
  return retValue;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link stoch}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInFastK_Period - Lookback window for the raw %K high-low range
 *   (default 5; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowK_Period - Smoothing period turning FastK into SlowK (default 3;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowK_MAType - MA type used to smooth into SlowK (default
 *   `MAType.SMA`; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
 *   7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA,
 *   15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param optInSlowD_Period - Smoothing period for the SlowD signal line (default
 *   3; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowD_MAType - MA type used for the SlowD line (default
 *   `MAType.SMA`; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
 *   7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA,
 *   15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function stochDisplayShift(optInFastK_Period: number, optInSlowK_Period: number, optInSlowK_MAType: MAType | typeof Core.INTEGER_DEFAULT, optInSlowD_Period: number, optInSlowD_MAType: MAType | typeof Core.INTEGER_DEFAULT, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (stochLookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 2) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function stochImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  optInFastK_Period: number,
  optInSlowK_Period: number,
  optInSlowK_MAType: number,
  optInSlowD_Period: number,
  optInSlowD_MAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outSlowK: Float64Array,
  outSlowD: Float64Array,
): RetCode {
  let retCode: RetCode = RetCode.SUCCESS;
  let lowest = 0;
  let highest = 0;
  let tmp = 0;
  let tempBuffer!: Float64Array;
  let outIdx = 0;
  let lowestIdx = 0;
  let highestIdx = 0;
  let lookbackTotal = 0;
  let lookbackK = 0;
  let lookbackKSlow = 0;
  let lookbackDSlow = 0;
  let trailingIdx = 0;
  let today = 0;
  let i = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInFastK_Period === Core.INTEGER_DEFAULT) {
    optInFastK_Period = 5;
  } else if (optInFastK_Period < 1 || optInFastK_Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowK_Period === Core.INTEGER_DEFAULT) {
    optInSlowK_Period = 3;
  } else if (optInSlowK_Period < 1 || optInSlowK_Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowK_MAType === Core.INTEGER_DEFAULT || optInSlowK_MAType === MAType.DEFAULT) {
    optInSlowK_MAType = MAType.SMA;
  } else if (optInSlowK_MAType < MATypes.Min || optInSlowK_MAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowD_Period === Core.INTEGER_DEFAULT) {
    optInSlowD_Period = 3;
  } else if (optInSlowD_Period < 1 || optInSlowD_Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowD_MAType === Core.INTEGER_DEFAULT || optInSlowD_MAType === MAType.DEFAULT) {
    optInSlowD_MAType = MAType.SMA;
  } else if (optInSlowD_MAType < MATypes.Min || optInSlowD_MAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if ((outSlowK.buffer === outSlowD.buffer && outSlowK.byteOffset === outSlowD.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  i = 0;
  /* With stochastic, there is a total of 4 different lines that
   * are defined: FASTK, FASTD, SLOWK and SLOWD.
   *
   * The D is the signal line usually drawn over its
   * corresponding K function.
   *
   *                    (Today's Close - LowestLow)
   *  FASTK(Kperiod) =  --------------------------- * 100
   *                     (HighestHigh - LowestLow)
   *
   *  FASTD(FastDperiod, MA type) = MA Smoothed FASTK over FastDperiod
   *
   *  SLOWK(SlowKperiod, MA type) = MA Smoothed FASTK over SlowKperiod
   *
   *  SLOWD(SlowDperiod, MA Type) = MA Smoothed SLOWK over SlowDperiod
   *
   * The HighestHigh and LowestLow are the extreme values among the
   * last 'Kperiod'.
   *
   * SLOWK and FASTD are equivalent when using the same period.
   *
   * The following shows how these four lines are made available in TA-LIB:
   *
   *  TA_STOCH  : Returns the SLOWK and SLOWD
   *  TA_STOCHF : Returns the FASTK and FASTD
   *
   * The TA_STOCH function correspond to the more widely implemented version
   * found in many software/charting package. The TA_STOCHF is more rarely
   * used because its higher volatility cause often whipsaws.
   */
  /* Identify the lookback needed. */
  lookbackK = optInFastK_Period - 1;
  lookbackKSlow = maLookback(optInSlowK_Period, (optInSlowK_MAType as MAType), core);
  lookbackDSlow = maLookback(optInSlowD_Period, (optInSlowD_MAType as MAType), core);
  lookbackTotal = lookbackK + lookbackDSlow + lookbackKSlow;
  /* Move up the start index if there is not
   * enough initial data.
   */
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    /* Succeed... but no data in the output. */
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  /* Do the K calculation:
   *
   *    Kt = 100 x ((Ct-Lt)/(Ht-Lt))
   *
   * Kt is today stochastic
   * Ct is today closing price.
   * Lt is the lowest price of the last K Period (including today)
   * Ht is the highest price of the last K Period (including today)
   */
  /* Proceed with the calculation for the requested range.
   * Note that this algorithm allows the input and
   * output to be the same buffer.
   */
  outIdx = 0;
  /* Calculate just enough K for ending up with the caller
   * requested range. (The range of k must consider all
   * the lookback involve with the smoothing).
   */
  trailingIdx = startIdx - lookbackTotal;
  today = trailingIdx + lookbackK;
  highestIdx = -1;
  lowestIdx = highestIdx;
  lowest = 0.0;
  highest = lowest;
  /* Allocate a temporary buffer large enough to
   * store the K.
   *
   * When outSlowK aliases a price input the caller buffer doubles as the
   * scratch, saving one allocation: the K writes trail the min/max window
   * reads, and the final memmove is overlap-safe. outSlowD must NOT be
   * elected: the %D ma() below would then run in place over the smoothed K
   * that the memmove into outSlowK still needs (issue #130).
   */
  if ((outSlowK.buffer === inHigh.buffer && outSlowK.byteOffset === inHigh.byteOffset) || (outSlowK.buffer === inLow.buffer && outSlowK.byteOffset === inLow.byteOffset) || (outSlowK.buffer === inClose.buffer && outSlowK.byteOffset === inClose.byteOffset)) {
    tempBuffer = outSlowK;
  } else {
    tempBuffer = new Float64Array(endIdx - today + 1);
  }
  /* Do the K calculation */
  while (today <= endIdx) {
    /* Set the lowest low */
    tmp = inLow[today];
    if (lowestIdx < trailingIdx) {
      lowestIdx = trailingIdx;
      lowest = inLow[lowestIdx];
      i = lowestIdx;
      while (++i <= today) {
        tmp = inLow[i];
        if (tmp < lowest) {
          lowestIdx = i;
          lowest = tmp;
        }
      }
    } else if (tmp <= lowest) {
      lowestIdx = today;
      lowest = tmp;
    }
    /* Set the highest high */
    tmp = inHigh[today];
    if (highestIdx < trailingIdx) {
      highestIdx = trailingIdx;
      highest = inHigh[highestIdx];
      i = highestIdx;
      while (++i <= today) {
        tmp = inHigh[i];
        if (tmp > highest) {
          highestIdx = i;
          highest = tmp;
        }
      }
    } else if (tmp >= highest) {
      highestIdx = today;
      highest = tmp;
    }
    /* Divide by the range itself and scale after: the guard has to test the
     * very expression the division uses, or a scaling step can carry a
     * guarded-non-zero into a zero divisor.
     *
     * The band is the range against ITS OWN two extremes, not a fixed
     * constant: the range carries the quote unit, so a constant answers
     * "flat" for every window of an instrument quoted below it (issue #253).
     * It absorbs the machine-flat window an exact test would divide into
     * [0,100] noise (issue #107 / STOCHRSI).
     */
    if (!(Math.abs(highest - lowest) <= 0.00000000000001 * (Math.abs(highest) + Math.abs(lowest)))) {
      tempBuffer[outIdx++] = (inClose[today] - lowest) / (highest - lowest) * 100.0;
    } else {
      tempBuffer[outIdx++] = 0.0;
    }
    trailingIdx += 1;
    today += 1;
  }
  /* Un-smoothed K calculation completed. This K calculation is not returned
   * to the caller. It is always smoothed and then return.
   * Some documentation will refer to the smoothed version as being
   * "K-Slow", but often this end up to be shorten to "K".
   */
  const _xr0 = ma(0, outIdx - 1, tempBuffer, optInSlowK_Period, (optInSlowK_MAType as MAType), tempBuffer, core);
  outBegIdx.value = _xr0.begIdx;
  outNBElement.value = _xr0.count;
  retCode = RetCode.SUCCESS;
  if (outNBElement.value === 0) {
    /* Something wrong happen? No further data? */
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return retCode;
  }
  /* Calculate the %D which is simply a moving average of
   * the already smoothed %K.
   */
  const _xr1 = ma(0, outNBElement.value - 1, tempBuffer, optInSlowD_Period, (optInSlowD_MAType as MAType), outSlowD, core);
  outBegIdx.value = _xr1.begIdx;
  outNBElement.value = _xr1.count;
  retCode = RetCode.SUCCESS;
  /* Copy tempBuffer into the caller buffer.
   * (Calculation could not be done directly in the
   *  caller buffer because more input data then the
   *  requested range was needed for doing %D).
   */
  /* memmove, not memcpy: tempBuffer aliases outSlowK when the caller buffer is
   * reused as scratch, so source and destination overlap (issue #94).
   */
  outSlowK.set(tempBuffer.subarray(lookbackDSlow, lookbackDSlow + outNBElement.value), 0);
  /* Note: Keep the outBegIdx relative to the
   *       caller input before returning.
   */
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/**
 * Slow Stochastic oscillator: locates the close within the high-low range over a
 * lookback period, then double-smooths it. Returns the Slow-%K and Slow-%D lines.
 * SlowK/SlowD > 80 overbought, \< 20 oversold; %K crossing %D signals momentum
 * shifts.
 *
 * Formula and more info at
 * [ta-lib.org/functions/stoch](https://ta-lib.org/functions/stoch).
 *
 * **Notes**
 * - When the high-low range over the window is zero, the raw stochastic is set to
 *   0 instead of being undefined.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link stochLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param optInFastK_Period - Lookback window for the raw %K high-low range
 *   (default 5; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowK_Period - Smoothing period turning FastK into SlowK (default 3;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowK_MAType - MA type used to smooth into SlowK (default
 *   `MAType.SMA`; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
 *   7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA,
 *   15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param optInSlowD_Period - Smoothing period for the SlowD signal line (default
 *   3; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowD_MAType - MA type used for the SlowD line (default
 *   `MAType.SMA`; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
 *   7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA,
 *   15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param outSlowK - Raw FastK smoothed by SlowK_Period MA. Must hold the values
 *   produced and never be empty; the input length always suffices.
 * @param outSlowD - Signal line: SlowK smoothed by SlowD_Period MA. Must hold the
 *   values produced and never be empty; the input length always suffices.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The range written: `begIdx` is the first bar with a value, `count` how
 *   many were written.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for an argument that is not an int32, a
 *   parameter outside its range, an array too short for the range, an empty output,
 *   two outputs sharing memory or an output sharing memory with a `Float64Array`
 *   input it is not. Checked before anything is written.
 *
 * @example
 * ```ts
 * import { MAType, stoch } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const slowK = new Float64Array(high.length);
 * const slowD = new Float64Array(high.length);
 * const { begIdx, count } = stoch(
 *   0, high.length - 1,
 *   high, low, close,
 *   5, 3, MAType.SMA, 3, MAType.SMA,
 *   slowK, slowD,
 * );
 * // slowK[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`stochf`](https://ta-lib.org/functions/stochf)
 * @see [`stochrsi`](https://ta-lib.org/functions/stochrsi)
 * @see [`ma`](https://ta-lib.org/functions/ma)
 */
export function stoch(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInFastK_Period: number,
  optInSlowK_Period: number,
  optInSlowK_MAType: MAType | typeof Core.INTEGER_DEFAULT,
  optInSlowD_Period: number,
  optInSlowD_MAType: MAType | typeof Core.INTEGER_DEFAULT,
  outSlowK: Float64Array,
  outSlowD: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('STOCH', 'startIdx', startIdx);
  endIdx = requireInt32('STOCH', 'endIdx', endIdx);
  optInFastK_Period = requireInt32('STOCH', 'optInFastK_Period', optInFastK_Period);
  optInSlowK_Period = requireInt32('STOCH', 'optInSlowK_Period', optInSlowK_Period);
  optInSlowK_MAType = requireInt32('STOCH', 'optInSlowK_MAType', optInSlowK_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  optInSlowD_Period = requireInt32('STOCH', 'optInSlowD_Period', optInSlowD_Period);
  optInSlowD_MAType = requireInt32('STOCH', 'optInSlowD_MAType', optInSlowD_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireIndexRange('STOCH', startIdx, endIdx);
  const guardStart = clampedStart('STOCH', startIdx, stochLookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('STOCH', 'inHigh', inHigh, guardInLen);
  requireLength('STOCH', 'inLow', inLow, guardInLen);
  requireLength('STOCH', 'inClose', inClose, guardInLen);
  requireLength('STOCH', 'outSlowK', outSlowK, guardOutLen);
  requireLength('STOCH', 'outSlowD', outSlowD, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outSlowK, inHighF64) || overlapsPartly(outSlowK, inLowF64) || overlapsPartly(outSlowK, inCloseF64) || overlapsPartly(outSlowD, inHighF64) || overlapsPartly(outSlowD, inLowF64) || overlapsPartly(outSlowD, inCloseF64) || overlaps(outSlowK, outSlowD)) {
    throw failure('STOCH', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = stochImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, outSlowK, outSlowD);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('STOCH', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live STOCH stream: one value per closed bar, bit-identical to {@link stoch}
 * over the same series. Open it with {@link stochOpen} or
 * {@link stochOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/stoch](https://ta-lib.org/functions/stoch).
 */
export class StochStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInFastK_Period: number = 0;
  /** @internal */
  optInSlowK_Period: number = 0;
  /** @internal */
  optInSlowK_MAType: number = 0;
  /** @internal */
  optInSlowD_Period: number = 0;
  /** @internal */
  optInSlowD_MAType: number = 0;
  /** @internal */
  lowest: number = 0;
  /** @internal */
  highest: number = 0;
  /** @internal */
  lowestIdx: number = 0;
  /** @internal */
  highestIdx: number = 0;
  /** @internal */
  trailingIdx: number = 0;
  /** @internal */
  i: number = 0;
  /** @internal */
  today: number = 0;
  /** @internal */
  xMask: number = 0;
  /** @internal */
  x_inHigh: Float64Array = new Float64Array(1);
  /** @internal */
  x_inLow: Float64Array = new Float64Array(1);
  /** @internal */
  x_inClose: Float64Array = new Float64Array(1);
  /** @internal */
  cur_outSlowK: number = 0;
  /** @internal */
  cur_outSlowD: number = 0;
  /** @internal */
  sub0!: MaStream;
  /** @internal */
  sub1!: MaStream;
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
   * {@link stoch} reports them over the same bars. Each accepted `update` and each
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
      throw failure('STOCH advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar, writing the new values into `out`.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number, inClose: number, out: StochOut): void {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('STOCH update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (typeof out !== 'object' || out === null) {
      throw badSink('STOCH update', 'StochOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('STOCH update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    stochStepImpl(this, inHigh, inLow, inClose);
    this.outRangeCount++;
    out.slowK = this.cur_outSlowK;
    out.slowD = this.cur_outSlowD;
  }

  /**
   * Writes into `out` what the next `update` with the same bar would write,
   * committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number, inClose: number, out: StochOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('STOCH peek', 'StochOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('STOCH peek', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let cur_tempBuffer = 0;
    let cur_outSlowD = 0;
    let cur_outSlowK = 0;
    let tmp = 0;
    let highest = sp.highest;
    let highestIdx = sp.highestIdx;
    let i = sp.i;
    let lowest = sp.lowest;
    let lowestIdx = sp.lowestIdx;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    let pkSlot1 = -1;
    let pkVal1 = 0;
    let pkSlot2 = -1;
    let pkVal2 = 0;
    pkSlot0 = sp.today & sp.xMask;
    pkVal0 = inHigh;
    pkSlot1 = sp.today & sp.xMask;
    pkVal1 = inLow;
    pkSlot2 = sp.today & sp.xMask;
    pkVal2 = inClose;
    /* Set the lowest low */
    tmp = ((sp.today & sp.xMask) !== pkSlot1) ? sp.x_inLow[sp.today & sp.xMask] : pkVal1;
    if (lowestIdx < sp.trailingIdx) {
      lowestIdx = sp.trailingIdx;
      lowest = ((lowestIdx & sp.xMask) !== pkSlot1) ? sp.x_inLow[lowestIdx & sp.xMask] : pkVal1;
      i = lowestIdx;
      while (++i <= sp.today) {
        tmp = ((i & sp.xMask) !== pkSlot1) ? sp.x_inLow[i & sp.xMask] : pkVal1;
        if (tmp < lowest) {
          lowestIdx = i;
          lowest = tmp;
        }
      }
    } else if (tmp <= lowest) {
      lowestIdx = sp.today;
      lowest = tmp;
    }
    /* Set the highest high */
    tmp = ((sp.today & sp.xMask) !== pkSlot0) ? sp.x_inHigh[sp.today & sp.xMask] : pkVal0;
    if (highestIdx < sp.trailingIdx) {
      highestIdx = sp.trailingIdx;
      highest = ((highestIdx & sp.xMask) !== pkSlot0) ? sp.x_inHigh[highestIdx & sp.xMask] : pkVal0;
      i = highestIdx;
      while (++i <= sp.today) {
        tmp = ((i & sp.xMask) !== pkSlot0) ? sp.x_inHigh[i & sp.xMask] : pkVal0;
        if (tmp > highest) {
          highestIdx = i;
          highest = tmp;
        }
      }
    } else if (tmp >= highest) {
      highestIdx = sp.today;
      highest = tmp;
    }
    /* Divide by the range itself and scale after: the guard has to test the
     * very expression the division uses, or a scaling step can carry a
     * guarded-non-zero into a zero divisor.
     *
     * The band is the range against ITS OWN two extremes, not a fixed
     * constant: the range carries the quote unit, so a constant answers
     * "flat" for every window of an instrument quoted below it (issue #253).
     * It absorbs the machine-flat window an exact test would divide into
     * [0,100] noise (issue #107 / STOCHRSI).
     */
    if (!(Math.abs(highest - lowest) <= 0.00000000000001 * (Math.abs(highest) + Math.abs(lowest)))) {
      cur_tempBuffer = ((((sp.today & sp.xMask) !== pkSlot2) ? sp.x_inClose[sp.today & sp.xMask] : pkVal2) - lowest) / (highest - lowest) * 100.0;
    } else {
      cur_tempBuffer = 0.0;
    }
    cur_tempBuffer = sp.sub0.peek(cur_tempBuffer);
    cur_outSlowD = sp.sub1.peek(cur_tempBuffer);
    cur_outSlowK = cur_tempBuffer;
    out.slowK = cur_outSlowK;
    out.slowD = cur_outSlowD;
  }

  /**
   * The value at the last bar this stream counted; `peek` does not change it.
   *
   * @param out - Receives the outputs.
   */
  value(out: StochOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('STOCH value', 'StochOut');
    }
    out.slowK = this.cur_outSlowK;
    out.slowD = this.cur_outSlowD;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): StochStream {
    const c = new StochStream(this.core);
    c.optInFastK_Period = this.optInFastK_Period;
    c.optInSlowK_Period = this.optInSlowK_Period;
    c.optInSlowK_MAType = this.optInSlowK_MAType;
    c.optInSlowD_Period = this.optInSlowD_Period;
    c.optInSlowD_MAType = this.optInSlowD_MAType;
    c.lowest = this.lowest;
    c.highest = this.highest;
    c.lowestIdx = this.lowestIdx;
    c.highestIdx = this.highestIdx;
    c.trailingIdx = this.trailingIdx;
    c.i = this.i;
    c.today = this.today;
    c.xMask = this.xMask;
    c.x_inHigh = this.x_inHigh.slice();
    c.x_inLow = this.x_inLow.slice();
    c.x_inClose = this.x_inClose.slice();
    c.cur_outSlowK = this.cur_outSlowK;
    c.cur_outSlowD = this.cur_outSlowD;
    c.sub0 = this.sub0.clone();
    c.sub1 = this.sub1.clone();
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one STOCH bar, written into an object the caller owns and reuses:
 * valid until the next call that writes it.
 */
export class StochOut {
  /** Raw FastK smoothed by SlowK_Period MA. */
  slowK: number = 0;
  /** Signal line: SlowK smoothed by SlowD_Period MA. */
  slowD: number = 0;
}

function stochStepImpl(sp: StochStream, inHigh: number, inLow: number, inClose: number): void {
  let tmp = 0;
  let cur_tempBuffer = 0;
  let cur_outSlowD = 0;
  sp.x_inHigh[sp.today & sp.xMask] = inHigh;
  sp.x_inLow[sp.today & sp.xMask] = inLow;
  sp.x_inClose[sp.today & sp.xMask] = inClose;
  /* Set the lowest low */
  tmp = sp.x_inLow[sp.today & sp.xMask];
  if (sp.lowestIdx < sp.trailingIdx) {
    sp.lowestIdx = sp.trailingIdx;
    sp.lowest = sp.x_inLow[sp.lowestIdx & sp.xMask];
    sp.i = sp.lowestIdx;
    while (++sp.i <= sp.today) {
      tmp = sp.x_inLow[sp.i & sp.xMask];
      if (tmp < sp.lowest) {
        sp.lowestIdx = sp.i;
        sp.lowest = tmp;
      }
    }
  } else if (tmp <= sp.lowest) {
    sp.lowestIdx = sp.today;
    sp.lowest = tmp;
  }
  /* Set the highest high */
  tmp = sp.x_inHigh[sp.today & sp.xMask];
  if (sp.highestIdx < sp.trailingIdx) {
    sp.highestIdx = sp.trailingIdx;
    sp.highest = sp.x_inHigh[sp.highestIdx & sp.xMask];
    sp.i = sp.highestIdx;
    while (++sp.i <= sp.today) {
      tmp = sp.x_inHigh[sp.i & sp.xMask];
      if (tmp > sp.highest) {
        sp.highestIdx = sp.i;
        sp.highest = tmp;
      }
    }
  } else if (tmp >= sp.highest) {
    sp.highestIdx = sp.today;
    sp.highest = tmp;
  }
  /* Divide by the range itself and scale after: the guard has to test the
   * very expression the division uses, or a scaling step can carry a
   * guarded-non-zero into a zero divisor.
   *
   * The band is the range against ITS OWN two extremes, not a fixed
   * constant: the range carries the quote unit, so a constant answers
   * "flat" for every window of an instrument quoted below it (issue #253).
   * It absorbs the machine-flat window an exact test would divide into
   * [0,100] noise (issue #107 / STOCHRSI).
   */
  if (!(Math.abs(sp.highest - sp.lowest) <= 0.00000000000001 * (Math.abs(sp.highest) + Math.abs(sp.lowest)))) {
    cur_tempBuffer = (sp.x_inClose[sp.today & sp.xMask] - sp.lowest) / (sp.highest - sp.lowest) * 100.0;
  } else {
    cur_tempBuffer = 0.0;
  }
  sp.trailingIdx += 1;
  sp.today += 1;
  cur_tempBuffer = sp.sub0.update(cur_tempBuffer);
  cur_outSlowD = sp.sub1.update(cur_tempBuffer);
  sp.cur_outSlowK = cur_tempBuffer;
  sp.cur_outSlowD = cur_outSlowD;
}

function stochOpenImpl(
  sp: StochStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInFastK_Period: number,
  optInSlowK_Period: number,
  optInSlowK_MAType: number,
  optInSlowD_Period: number,
  optInSlowD_MAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outSlowK: Float64Array,
  outSlowD: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let lowest = 0;
  let highest = 0;
  let tmp = 0;
  let tempBuffer!: Float64Array;
  let outIdx = 0;
  let lowestIdx = 0;
  let highestIdx = 0;
  let lookbackTotal = 0;
  let lookbackK = 0;
  let lookbackKSlow = 0;
  let lookbackDSlow = 0;
  let trailingIdx = 0;
  let today = 0;
  let i = 0;
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
  if (optInFastK_Period === Core.INTEGER_DEFAULT) {
    optInFastK_Period = 5;
  } else if (optInFastK_Period < 1 || optInFastK_Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowK_Period === Core.INTEGER_DEFAULT) {
    optInSlowK_Period = 3;
  } else if (optInSlowK_Period < 1 || optInSlowK_Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowK_MAType === Core.INTEGER_DEFAULT || optInSlowK_MAType === MAType.DEFAULT) {
    optInSlowK_MAType = MAType.SMA;
  } else if (optInSlowK_MAType < MATypes.Min || optInSlowK_MAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowD_Period === Core.INTEGER_DEFAULT) {
    optInSlowD_Period = 3;
  } else if (optInSlowD_Period < 1 || optInSlowD_Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowD_MAType === Core.INTEGER_DEFAULT || optInSlowD_MAType === MAType.DEFAULT) {
    optInSlowD_MAType = MAType.SMA;
  } else if (optInSlowD_MAType < MATypes.Min || optInSlowD_MAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen < stochLookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType as MAType, optInSlowD_Period, optInSlowD_MAType as MAType, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const sc_outSlowK = outStride === 1 ? outSlowK : new Float64Array(historyLen);
  const sc_outSlowD = outStride === 1 ? outSlowD : new Float64Array(historyLen);
  i = 0;
  /* With stochastic, there is a total of 4 different lines that
   * are defined: FASTK, FASTD, SLOWK and SLOWD.
   *
   * The D is the signal line usually drawn over its
   * corresponding K function.
   *
   *                    (Today's Close - LowestLow)
   *  FASTK(Kperiod) =  --------------------------- * 100
   *                     (HighestHigh - LowestLow)
   *
   *  FASTD(FastDperiod, MA type) = MA Smoothed FASTK over FastDperiod
   *
   *  SLOWK(SlowKperiod, MA type) = MA Smoothed FASTK over SlowKperiod
   *
   *  SLOWD(SlowDperiod, MA Type) = MA Smoothed SLOWK over SlowDperiod
   *
   * The HighestHigh and LowestLow are the extreme values among the
   * last 'Kperiod'.
   *
   * SLOWK and FASTD are equivalent when using the same period.
   *
   * The following shows how these four lines are made available in TA-LIB:
   *
   *  TA_STOCH  : Returns the SLOWK and SLOWD
   *  TA_STOCHF : Returns the FASTK and FASTD
   *
   * The TA_STOCH function correspond to the more widely implemented version
   * found in many software/charting package. The TA_STOCHF is more rarely
   * used because its higher volatility cause often whipsaws.
   */
  /* Identify the lookback needed. */
  lookbackK = optInFastK_Period - 1;
  lookbackKSlow = maLookback(optInSlowK_Period, (optInSlowK_MAType as MAType), core);
  lookbackDSlow = maLookback(optInSlowD_Period, (optInSlowD_MAType as MAType), core);
  lookbackTotal = lookbackK + lookbackDSlow + lookbackKSlow;
  /* Move up the start index if there is not
   * enough initial data.
   */
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    /* Succeed... but no data in the output. */
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Do the K calculation:
   *
   *    Kt = 100 x ((Ct-Lt)/(Ht-Lt))
   *
   * Kt is today stochastic
   * Ct is today closing price.
   * Lt is the lowest price of the last K Period (including today)
   * Ht is the highest price of the last K Period (including today)
   */
  /* Proceed with the calculation for the requested range.
   * Note that this algorithm allows the input and
   * output to be the same buffer.
   */
  outIdx = 0;
  /* Calculate just enough K for ending up with the caller
   * requested range. (The range of k must consider all
   * the lookback involve with the smoothing).
   */
  trailingIdx = startIdx - lookbackTotal;
  today = trailingIdx + lookbackK;
  highestIdx = -1;
  lowestIdx = highestIdx;
  lowest = 0.0;
  highest = lowest;
  /* Allocate a temporary buffer large enough to
   * store the K.
   *
   * When outSlowK aliases a price input the caller buffer doubles as the
   * scratch, saving one allocation: the K writes trail the min/max window
   * reads, and the final memmove is overlap-safe. outSlowD must NOT be
   * elected: the %D ma() below would then run in place over the smoothed K
   * that the memmove into outSlowK still needs (issue #130).
   */
  if ((sc_outSlowK.buffer === inHigh.buffer && sc_outSlowK.byteOffset === inHigh.byteOffset) || (sc_outSlowK.buffer === inLow.buffer && sc_outSlowK.byteOffset === inLow.byteOffset) || (sc_outSlowK.buffer === inClose.buffer && sc_outSlowK.byteOffset === inClose.byteOffset)) {
    tempBuffer = sc_outSlowK;
  } else {
    tempBuffer = new Float64Array(endIdx - today + 1);
  }
  /* Do the K calculation */
  while (today <= endIdx) {
    /* Set the lowest low */
    tmp = inLow[today];
    if (lowestIdx < trailingIdx) {
      lowestIdx = trailingIdx;
      lowest = inLow[lowestIdx];
      i = lowestIdx;
      while (++i <= today) {
        tmp = inLow[i];
        if (tmp < lowest) {
          lowestIdx = i;
          lowest = tmp;
        }
      }
    } else if (tmp <= lowest) {
      lowestIdx = today;
      lowest = tmp;
    }
    /* Set the highest high */
    tmp = inHigh[today];
    if (highestIdx < trailingIdx) {
      highestIdx = trailingIdx;
      highest = inHigh[highestIdx];
      i = highestIdx;
      while (++i <= today) {
        tmp = inHigh[i];
        if (tmp > highest) {
          highestIdx = i;
          highest = tmp;
        }
      }
    } else if (tmp >= highest) {
      highestIdx = today;
      highest = tmp;
    }
    /* Divide by the range itself and scale after: the guard has to test the
     * very expression the division uses, or a scaling step can carry a
     * guarded-non-zero into a zero divisor.
     *
     * The band is the range against ITS OWN two extremes, not a fixed
     * constant: the range carries the quote unit, so a constant answers
     * "flat" for every window of an instrument quoted below it (issue #253).
     * It absorbs the machine-flat window an exact test would divide into
     * [0,100] noise (issue #107 / STOCHRSI).
     */
    if (!(Math.abs(highest - lowest) <= 0.00000000000001 * (Math.abs(highest) + Math.abs(lowest)))) {
      tempBuffer[outIdx++] = (inClose[today] - lowest) / (highest - lowest) * 100.0;
    } else {
      tempBuffer[outIdx++] = 0.0;
    }
    trailingIdx += 1;
    today += 1;
  }
  /* Un-smoothed K calculation completed. This K calculation is not returned
   * to the caller. It is always smoothed and then return.
   * Some documentation will refer to the smoothed version as being
   * "K-Slow", but often this end up to be shorten to "K".
   */
  const sub0 = maOpenInternal(core, tempBuffer.slice(0, (outIdx - 1) + 1), 0, optInSlowK_Period, (optInSlowK_MAType as MAType));
  const _xr0 = ma(0, outIdx - 1, tempBuffer, optInSlowK_Period, (optInSlowK_MAType as MAType), tempBuffer, core);
  outBegIdx.value = _xr0.begIdx;
  outNBElement.value = _xr0.count;
  if (outNBElement.value === 0) {
    /* Something wrong happen? No further data? */
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Calculate the %D which is simply a moving average of
   * the already smoothed %K.
   */
  const sub1 = maOpenAndFillInternal(core, tempBuffer.slice(0, (outNBElement.value - 1) + 1), 0, optInSlowD_Period, (optInSlowD_MAType as MAType), outBegIdx, outNBElement, sc_outSlowD);
  /* Copy tempBuffer into the caller buffer.
   * (Calculation could not be done directly in the
   *  caller buffer because more input data then the
   *  requested range was needed for doing %D).
   */
  /* memmove, not memcpy: tempBuffer aliases outSlowK when the caller buffer is
   * reused as scratch, so source and destination overlap (issue #94).
   */
  sc_outSlowK.set(tempBuffer.subarray(lookbackDSlow, lookbackDSlow + outNBElement.value), 0);
  /* Note: Keep the outBegIdx relative to the
   *       caller input before returning.
   */
  outBegIdx.value = startIdx;
  if (outNBElement.value < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const capX = today - trailingIdx + 1;
  if (capX < 1 || capX > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  let physX = 1;
  while (physX < capX) {
    physX <<= 1;
  }
  const capX_inHigh = new Float64Array(physX);
  const capX_inLow = new Float64Array(physX);
  const capX_inClose = new Float64Array(physX);
  for (let fillJ = historyLen - capX; fillJ < historyLen; fillJ++) {
    capX_inHigh[fillJ & (physX - 1)] = inHigh[fillJ];
    capX_inLow[fillJ & (physX - 1)] = inLow[fillJ];
    capX_inClose[fillJ & (physX - 1)] = inClose[fillJ];
  }
  sp.optInFastK_Period = optInFastK_Period;
  sp.optInSlowK_Period = optInSlowK_Period;
  sp.optInSlowK_MAType = optInSlowK_MAType;
  sp.optInSlowD_Period = optInSlowD_Period;
  sp.optInSlowD_MAType = optInSlowD_MAType;
  sp.lowest = lowest;
  sp.highest = highest;
  sp.lowestIdx = lowestIdx;
  sp.highestIdx = highestIdx;
  sp.trailingIdx = trailingIdx;
  sp.i = i;
  sp.today = today;
  sp.xMask = physX - 1;
  sp.x_inHigh = capX_inHigh;
  sp.x_inLow = capX_inLow;
  sp.x_inClose = capX_inClose;
  sp.sub0 = sub0;
  sp.sub1 = sub1;
  sp.cur_outSlowK = sc_outSlowK[outNBElement.value - 1];
  sp.cur_outSlowD = sc_outSlowD[outNBElement.value - 1];
  return RetCode.SUCCESS;
}

/** @internal */
export function stochOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInFastK_Period: number,
  optInSlowK_Period: number,
  optInSlowK_MAType: number,
  optInSlowD_Period: number,
  optInSlowD_MAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outSlowK: Float64Array,
  outSlowD: Float64Array,
): StochStream {
  const sp = new StochStream(core);
  const retCode = stochOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, outSlowK, outSlowD, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('STOCH openAndFill', inHigh.length, startIdx, stochLookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType as MAType, optInSlowD_Period, optInSlowD_MAType as MAType, core));
  }
  throw streamFailure('STOCH openAndFill', retCode);
}

/** @internal */
export function stochOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInFastK_Period: number,
  optInSlowK_Period: number,
  optInSlowK_MAType: number,
  optInSlowD_Period: number,
  optInSlowD_MAType: number,
): StochStream {
  const sp = new StochStream(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outSlowK = new Float64Array(1);
  const sink_outSlowD = new Float64Array(1);
  const retCode = stochOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, sink_outSlowK, sink_outSlowD, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('STOCH open', inHigh.length, startIdx, stochLookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType as MAType, optInSlowD_Period, optInSlowD_MAType as MAType, core));
  }
  throw streamFailure('STOCH open', retCode);
}

/**
 * Opens a live STOCH stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link stoch} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInFastK_Period - Lookback window for the raw %K high-low range
 *   (default 5; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowK_Period - Smoothing period turning FastK into SlowK (default 3;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowK_MAType - MA type used to smooth into SlowK (default
 *   `MAType.SMA`; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
 *   7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA,
 *   15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param optInSlowD_Period - Smoothing period for the SlowD signal line (default
 *   3; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowD_MAType - MA type used for the SlowD line (default
 *   `MAType.SMA`; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
 *   7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA,
 *   15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `stochLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { MAType, StochOut, stochOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = stochOpen(high, low, close, 5, 3, MAType.SMA, 3, MAType.SMA);
 * const peeked = new StochOut();
 * const updated = new StochOut();
 * stream.peek(101.4, 99.1, 100.9, peeked); // a forming bar: commits nothing
 * stream.update(101.4, 99.1, 100.9, updated); // the bar closes
 * ```
 */
export function stochOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInFastK_Period: number,
  optInSlowK_Period: number,
  optInSlowK_MAType: MAType | typeof Core.INTEGER_DEFAULT,
  optInSlowD_Period: number,
  optInSlowD_MAType: MAType | typeof Core.INTEGER_DEFAULT,
  core: Core = Core.DEFAULT,
): StochStream {
  optInFastK_Period = requireInt32('STOCH open', 'optInFastK_Period', optInFastK_Period);
  optInSlowK_Period = requireInt32('STOCH open', 'optInSlowK_Period', optInSlowK_Period);
  optInSlowK_MAType = requireInt32('STOCH open', 'optInSlowK_MAType', optInSlowK_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  optInSlowD_Period = requireInt32('STOCH open', 'optInSlowD_Period', optInSlowD_Period);
  optInSlowD_MAType = requireInt32('STOCH open', 'optInSlowD_MAType', optInSlowD_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('STOCH open', 'inHigh', inHigh);
  requireHistory('STOCH open', inHigh.length);
  requireArgument('STOCH open', 'inLow', inLow);
  requireArgument('STOCH open', 'inClose', inClose);
  requireHistoryLength('STOCH open', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('STOCH open', 'inClose', inClose.length, inHigh.length);
  return stochOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType);
}

/**
 * {@link stochOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link stoch}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInFastK_Period - Lookback window for the raw %K high-low range
 *   (default 5; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowK_Period - Smoothing period turning FastK into SlowK (default 3;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowK_MAType - MA type used to smooth into SlowK (default
 *   `MAType.SMA`; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
 *   7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA,
 *   15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param optInSlowD_Period - Smoothing period for the SlowD signal line (default
 *   3; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowD_MAType - MA type used for the SlowD line (default
 *   `MAType.SMA`; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
 *   7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA,
 *   15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param outSlowK - Raw FastK smoothed by SlowK_Period MA. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param outSlowD - Signal line: SlowK smoothed by SlowD_Period MA. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `stochLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range, histories of different lengths or an output too
 *   short or starting where another output or a `Float64Array` input starts, or, on
 *   a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { MAType, stochOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const slowK = new Float64Array(high.length);
 * const slowD = new Float64Array(high.length);
 * const stream = stochOpenAndFill(
 *   high, low, close,
 *   5, 3, MAType.SMA, 3, MAType.SMA,
 *   slowK, slowD,
 * );
 * const { begIdx, count } = stream.outRange();
 * // slowK[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function stochOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInFastK_Period: number,
  optInSlowK_Period: number,
  optInSlowK_MAType: MAType | typeof Core.INTEGER_DEFAULT,
  optInSlowD_Period: number,
  optInSlowD_MAType: MAType | typeof Core.INTEGER_DEFAULT,
  outSlowK: Float64Array,
  outSlowD: Float64Array,
  core: Core = Core.DEFAULT,
): StochStream {
  optInFastK_Period = requireInt32('STOCH openAndFill', 'optInFastK_Period', optInFastK_Period);
  optInSlowK_Period = requireInt32('STOCH openAndFill', 'optInSlowK_Period', optInSlowK_Period);
  optInSlowK_MAType = requireInt32('STOCH openAndFill', 'optInSlowK_MAType', optInSlowK_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  optInSlowD_Period = requireInt32('STOCH openAndFill', 'optInSlowD_Period', optInSlowD_Period);
  optInSlowD_MAType = requireInt32('STOCH openAndFill', 'optInSlowD_MAType', optInSlowD_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('STOCH openAndFill', 'inHigh', inHigh);
  requireHistory('STOCH openAndFill', inHigh.length);
  requireArgument('STOCH openAndFill', 'inLow', inLow);
  requireArgument('STOCH openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('STOCH openAndFill', inHigh.length, stochLookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType as MAType, optInSlowD_Period, optInSlowD_MAType as MAType, core));
  requireHistoryLength('STOCH openAndFill', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('STOCH openAndFill', 'inClose', inClose.length, inHigh.length);
  requireLength('STOCH openAndFill', 'outSlowK', outSlowK, guardOutLen);
  requireLength('STOCH openAndFill', 'outSlowD', outSlowD, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outSlowK.buffer === inHighF64.buffer && outSlowK.byteOffset === inHighF64.byteOffset) || (outSlowK.buffer === inLowF64.buffer && outSlowK.byteOffset === inLowF64.byteOffset) || (outSlowK.buffer === inCloseF64.buffer && outSlowK.byteOffset === inCloseF64.byteOffset) || (outSlowD.buffer === inHighF64.buffer && outSlowD.byteOffset === inHighF64.byteOffset) || (outSlowD.buffer === inLowF64.buffer && outSlowD.byteOffset === inLowF64.byteOffset) || (outSlowD.buffer === inCloseF64.buffer && outSlowD.byteOffset === inCloseF64.byteOffset) || (outSlowK.buffer === outSlowD.buffer && outSlowK.byteOffset === outSlowD.byteOffset)) {
    throw streamFailure('STOCH openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outSlowK, inHighF64) || overlaps(outSlowK, inLowF64) || overlaps(outSlowK, inCloseF64) || overlaps(outSlowD, inHighF64) || overlaps(outSlowD, inLowF64) || overlaps(outSlowD, inCloseF64) || overlaps(outSlowK, outSlowD))) {
    throw streamFailure('STOCH openAndFill', RetCode.BAD_PARAM);
  }
  return stochOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, new MInteger(), new MInteger(), outSlowK, outSlowD);
}
