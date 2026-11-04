/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  EKO      echo999@ifrance.com
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY    Description
 *  -------------------------------------------------------------------
 *  010802 MF    Template creation.
 *  051103 EKO   Found bug and fix related to outFastD.
 *  052603 MF    Adapt code to compile with .NET Managed C++
 *  071026 MF,CC Fix #107. Guard the Fast-K division with TA_IS_ZERO, not an
 *               exact `diff != 0.0`, so a machine-flat window yields 0 instead
 *               of dividing a sub-epsilon residue into [0,100] noise (STOCHRSI).
 *  072026 MF,CC Fix #130. Never elect outFastD as the K scratch buffer: %D's
 *               in-place ma() destroyed the raw K before the final copy.
 *  082326 MF,CC Fix #253. Scale that guard to the window's own extremes: the
 *               fixed band zeroed the whole output for any instrument quoted
 *               small enough to fall under it.
 *  082726 MF,CC Drop the dead retCode block after the copy: the rejection is
 *               already answered above it, and the shape reads like #269.
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
import { MaStream, ma, maLookback, maOpenAndFillInternal } from './ma.ts';

/**
 * Number of leading input bars {@link stochf} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInFastK_Period - Lookback window for the highest-high/lowest-low of
 *   Fast-K (default 5; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param optInFastD_Period - Smoothing period for the Fast-D line (default 3;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInFastD_MAType - Moving-average type used to smooth Fast-D (default
 *   `MAType.SMA`; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
 *   7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA,
 *   15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function stochfLookback(optInFastK_Period: number, optInFastD_Period: number, optInFastD_MAType: MAType | typeof Core.INTEGER_DEFAULT, core: Core = Core.DEFAULT): number {
  if ((optInFastK_Period | 0) !== optInFastK_Period) {
    return -1;
  }
  if ((optInFastD_Period | 0) !== optInFastD_Period) {
    return -1;
  }
  if ((optInFastD_MAType | 0) !== optInFastD_MAType) {
    return -1;
  }
  if (optInFastK_Period === Core.INTEGER_DEFAULT) {
    optInFastK_Period = 5;
  } else if (optInFastK_Period < 1 || optInFastK_Period > 100000) {
    return -1;
  }
  if (optInFastD_Period === Core.INTEGER_DEFAULT) {
    optInFastD_Period = 3;
  } else if (optInFastD_Period < 1 || optInFastD_Period > 100000) {
    return -1;
  }
  if (optInFastD_MAType === Core.INTEGER_DEFAULT || optInFastD_MAType === MAType.DEFAULT) {
    optInFastD_MAType = MAType.SMA;
  } else if (optInFastD_MAType < MATypes.Min || optInFastD_MAType > MATypes.Max) {
    return -1;
  }
  let retValue = 0;
  /* Account for the initial data needed for Fast-K. */
  retValue = optInFastK_Period - 1;
  /* Add the smoothing being done for Fast-D */
  retValue += maLookback(optInFastD_Period, (optInFastD_MAType as MAType), core);
  return retValue;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link stochf}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInFastK_Period - Lookback window for the highest-high/lowest-low of
 *   Fast-K (default 5; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param optInFastD_Period - Smoothing period for the Fast-D line (default 3;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInFastD_MAType - Moving-average type used to smooth Fast-D (default
 *   `MAType.SMA`; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
 *   7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA,
 *   15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function stochfDisplayShift(optInFastK_Period: number, optInFastD_Period: number, optInFastD_MAType: MAType | typeof Core.INTEGER_DEFAULT, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (stochfLookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 2) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function stochfImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  optInFastK_Period: number,
  optInFastD_Period: number,
  optInFastD_MAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outFastK: Float64Array,
  outFastD: Float64Array,
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
  let lookbackFastD = 0;
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
  if (optInFastD_Period === Core.INTEGER_DEFAULT) {
    optInFastD_Period = 3;
  } else if (optInFastD_Period < 1 || optInFastD_Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInFastD_MAType === Core.INTEGER_DEFAULT || optInFastD_MAType === MAType.DEFAULT) {
    optInFastD_MAType = MAType.SMA;
  } else if (optInFastD_MAType < MATypes.Min || optInFastD_MAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if ((outFastK.buffer === outFastD.buffer && outFastK.byteOffset === outFastD.byteOffset)) {
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
  lookbackFastD = maLookback(optInFastD_Period, (optInFastD_MAType as MAType), core);
  lookbackTotal = lookbackK + lookbackFastD;
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
   * When outFastK aliases a price input the caller buffer doubles as the
   * scratch, saving one allocation: the K writes trail the min/max window
   * reads, and the final memmove is overlap-safe. outFastD must NOT be
   * elected: the %D ma() below would then run in place over the raw K
   * that the memmove into outFastK still needs (issue #130).
   */
  if ((outFastK.buffer === inHigh.buffer && outFastK.byteOffset === inHigh.byteOffset) || (outFastK.buffer === inLow.buffer && outFastK.byteOffset === inLow.byteOffset) || (outFastK.buffer === inClose.buffer && outFastK.byteOffset === inClose.byteOffset)) {
    tempBuffer = outFastK;
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
  /* Fast-K calculation completed. This K calculation is returned
   * to the caller. It is smoothed to become Fast-D.
   */
  const _xr0 = ma(0, outIdx - 1, tempBuffer, optInFastD_Period, (optInFastD_MAType as MAType), outFastD, core);
  outBegIdx.value = _xr0.begIdx;
  outNBElement.value = _xr0.count;
  retCode = RetCode.SUCCESS;
  if (outNBElement.value === 0) {
    /* Something wrong happen? No further data? */
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return retCode;
  }
  /* Copy tempBuffer into the caller buffer.
   * (Calculation could not be done directly in the
   *  caller buffer because more input data then the
   *  requested range was needed for doing %D).
   */
  /* memmove, not memcpy: tempBuffer aliases outFastK when the caller buffer is
   * reused as scratch, so source and destination overlap (issue #94).
   */
  outFastK.set(tempBuffer.subarray(lookbackFastD, lookbackFastD + outNBElement.value), 0);
  /* Note: Keep the outBegIdx relative to the
   *       caller input before returning.
   */
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/**
 * Fast Stochastic Oscillator: the raw %K line and its moving-average-smoothed %D
 * line. Unlike STOCH (which slows both lines), STOCHF returns the unsmoothed FastK
 * and FastD. Oscillates 0-100; >80 overbought, \<20 oversold.
 *
 * Formula and more info at
 * [ta-lib.org/functions/stochf](https://ta-lib.org/functions/stochf).
 *
 * **Notes**
 * - When the high-low range over the window is zero, %K is set to 0 instead of
 *   being undefined.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link stochfLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param optInFastK_Period - Lookback window for the highest-high/lowest-low of
 *   Fast-K (default 5; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param optInFastD_Period - Smoothing period for the Fast-D line (default 3;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInFastD_MAType - Moving-average type used to smooth Fast-D (default
 *   `MAType.SMA`; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
 *   7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA,
 *   15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param outFastK - Raw %K stochastic line. Must hold the values produced and
 *   never be empty; the input length always suffices.
 * @param outFastD - MA-smoothed %K (signal line) Must hold the values produced and
 *   never be empty; the input length always suffices.
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
 * import { MAType, stochf } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const fastK = new Float64Array(high.length);
 * const fastD = new Float64Array(high.length);
 * const { begIdx, count } = stochf(
 *   0, high.length - 1,
 *   high, low, close,
 *   5, 3, MAType.SMA,
 *   fastK, fastD,
 * );
 * // fastK[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`stoch`](https://ta-lib.org/functions/stoch)
 * @see [`stochrsi`](https://ta-lib.org/functions/stochrsi)
 * @see [`ma`](https://ta-lib.org/functions/ma)
 */
export function stochf(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInFastK_Period: number,
  optInFastD_Period: number,
  optInFastD_MAType: MAType | typeof Core.INTEGER_DEFAULT,
  outFastK: Float64Array,
  outFastD: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('STOCHF', 'startIdx', startIdx);
  endIdx = requireInt32('STOCHF', 'endIdx', endIdx);
  optInFastK_Period = requireInt32('STOCHF', 'optInFastK_Period', optInFastK_Period);
  optInFastD_Period = requireInt32('STOCHF', 'optInFastD_Period', optInFastD_Period);
  optInFastD_MAType = requireInt32('STOCHF', 'optInFastD_MAType', optInFastD_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireIndexRange('STOCHF', startIdx, endIdx);
  const guardStart = clampedStart('STOCHF', startIdx, stochfLookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('STOCHF', 'inHigh', inHigh, guardInLen);
  requireLength('STOCHF', 'inLow', inLow, guardInLen);
  requireLength('STOCHF', 'inClose', inClose, guardInLen);
  requireLength('STOCHF', 'outFastK', outFastK, guardOutLen);
  requireLength('STOCHF', 'outFastD', outFastD, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outFastK, inHighF64) || overlapsPartly(outFastK, inLowF64) || overlapsPartly(outFastK, inCloseF64) || overlapsPartly(outFastD, inHighF64) || overlapsPartly(outFastD, inLowF64) || overlapsPartly(outFastD, inCloseF64) || overlaps(outFastK, outFastD)) {
    throw failure('STOCHF', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = stochfImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx, outNBElement, outFastK, outFastD);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('STOCHF', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live STOCHF stream: one value per closed bar, bit-identical to {@link stochf}
 * over the same series. Open it with {@link stochfOpen} or
 * {@link stochfOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/stochf](https://ta-lib.org/functions/stochf).
 */
export class StochfStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInFastK_Period: number = 0;
  /** @internal */
  optInFastD_Period: number = 0;
  /** @internal */
  optInFastD_MAType: number = 0;
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
  cur_outFastK: number = 0;
  /** @internal */
  cur_outFastD: number = 0;
  /** @internal */
  sub0!: MaStream;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): StochfStream {
    return new StochfStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link stochf} reports them over the same bars. Each accepted `update` and each
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
      throw failure('STOCHF advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
  update(inHigh: number, inLow: number, inClose: number, out: StochfOut): void {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('STOCHF update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (typeof out !== 'object' || out === null) {
      throw badSink('STOCHF update', 'StochfOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('STOCHF update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    stochfStepImpl(this, inHigh, inLow, inClose);
    this.outRangeCount++;
    out.fastK = this.cur_outFastK;
    out.fastD = this.cur_outFastD;
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
  peek(inHigh: number, inLow: number, inClose: number, out: StochfOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('STOCHF peek', 'StochfOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('STOCHF peek', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let cur_tempBuffer = 0;
    let cur_outFastD = 0;
    let cur_outFastK = 0;
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
    cur_outFastD = sp.sub0.peek(cur_tempBuffer);
    cur_outFastK = cur_tempBuffer;
    out.fastK = cur_outFastK;
    out.fastD = cur_outFastD;
  }

  /**
   * The value at the last bar this stream counted; `peek` does not change it.
   *
   * @param out - Receives the outputs.
   */
  value(out: StochfOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('STOCHF value', 'StochfOut');
    }
    out.fastK = this.cur_outFastK;
    out.fastD = this.cur_outFastD;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): StochfStream {
    const c = new StochfStream(this.core);
    c.optInFastK_Period = this.optInFastK_Period;
    c.optInFastD_Period = this.optInFastD_Period;
    c.optInFastD_MAType = this.optInFastD_MAType;
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
    c.cur_outFastK = this.cur_outFastK;
    c.cur_outFastD = this.cur_outFastD;
    c.sub0 = this.sub0.clone();
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one STOCHF bar, written into an object the caller owns and
 * reuses: valid until the next call that writes it.
 */
export class StochfOut {
  /** Raw %K stochastic line. */
  fastK: number = 0;
  /** MA-smoothed %K (signal line) */
  fastD: number = 0;
}

function stochfStepImpl(sp: StochfStream, inHigh: number, inLow: number, inClose: number): void {
  let tmp = 0;
  let cur_tempBuffer = 0;
  let cur_outFastD = 0;
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
  cur_outFastD = sp.sub0.update(cur_tempBuffer);
  sp.cur_outFastK = cur_tempBuffer;
  sp.cur_outFastD = cur_outFastD;
}

function stochfOpenImpl(
  sp: StochfStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInFastK_Period: number,
  optInFastD_Period: number,
  optInFastD_MAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outFastK: Float64Array,
  outFastD: Float64Array,
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
  let lookbackFastD = 0;
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
  if (optInFastD_Period === Core.INTEGER_DEFAULT) {
    optInFastD_Period = 3;
  } else if (optInFastD_Period < 1 || optInFastD_Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInFastD_MAType === Core.INTEGER_DEFAULT || optInFastD_MAType === MAType.DEFAULT) {
    optInFastD_MAType = MAType.SMA;
  } else if (optInFastD_MAType < MATypes.Min || optInFastD_MAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen < stochfLookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType as MAType, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const sc_outFastK = outStride === 1 ? outFastK : new Float64Array(historyLen);
  const sc_outFastD = outStride === 1 ? outFastD : new Float64Array(historyLen);
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
  lookbackFastD = maLookback(optInFastD_Period, (optInFastD_MAType as MAType), core);
  lookbackTotal = lookbackK + lookbackFastD;
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
   * When outFastK aliases a price input the caller buffer doubles as the
   * scratch, saving one allocation: the K writes trail the min/max window
   * reads, and the final memmove is overlap-safe. outFastD must NOT be
   * elected: the %D ma() below would then run in place over the raw K
   * that the memmove into outFastK still needs (issue #130).
   */
  if ((sc_outFastK.buffer === inHigh.buffer && sc_outFastK.byteOffset === inHigh.byteOffset) || (sc_outFastK.buffer === inLow.buffer && sc_outFastK.byteOffset === inLow.byteOffset) || (sc_outFastK.buffer === inClose.buffer && sc_outFastK.byteOffset === inClose.byteOffset)) {
    tempBuffer = sc_outFastK;
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
  /* Fast-K calculation completed. This K calculation is returned
   * to the caller. It is smoothed to become Fast-D.
   */
  const sub0 = maOpenAndFillInternal(core, tempBuffer.slice(0, (outIdx - 1) + 1), 0, optInFastD_Period, (optInFastD_MAType as MAType), outBegIdx, outNBElement, sc_outFastD);
  if (outNBElement.value === 0) {
    /* Something wrong happen? No further data? */
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Copy tempBuffer into the caller buffer.
   * (Calculation could not be done directly in the
   *  caller buffer because more input data then the
   *  requested range was needed for doing %D).
   */
  /* memmove, not memcpy: tempBuffer aliases outFastK when the caller buffer is
   * reused as scratch, so source and destination overlap (issue #94).
   */
  sc_outFastK.set(tempBuffer.subarray(lookbackFastD, lookbackFastD + outNBElement.value), 0);
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
  sp.optInFastD_Period = optInFastD_Period;
  sp.optInFastD_MAType = optInFastD_MAType;
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
  sp.cur_outFastK = sc_outFastK[outNBElement.value - 1];
  sp.cur_outFastD = sc_outFastD[outNBElement.value - 1];
  return RetCode.SUCCESS;
}

/** @internal */
export function stochfOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInFastK_Period: number,
  optInFastD_Period: number,
  optInFastD_MAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outFastK: Float64Array,
  outFastD: Float64Array,
): StochfStream {
  const sp = StochfStream.of(core);
  const retCode = stochfOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx, outNBElement, outFastK, outFastD, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('STOCHF openAndFill', inHigh.length, startIdx, stochfLookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType as MAType, core));
  }
  throw streamFailure('STOCHF openAndFill', retCode);
}

/** @internal */
export function stochfOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInFastK_Period: number,
  optInFastD_Period: number,
  optInFastD_MAType: number,
): StochfStream {
  const sp = StochfStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outFastK = new Float64Array(1);
  const sink_outFastD = new Float64Array(1);
  const retCode = stochfOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx, outNBElement, sink_outFastK, sink_outFastD, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('STOCHF open', inHigh.length, startIdx, stochfLookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType as MAType, core));
  }
  throw streamFailure('STOCHF open', retCode);
}

/**
 * Opens a live STOCHF stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link stochf} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInFastK_Period - Lookback window for the highest-high/lowest-low of
 *   Fast-K (default 5; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param optInFastD_Period - Smoothing period for the Fast-D line (default 3;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInFastD_MAType - Moving-average type used to smooth Fast-D (default
 *   `MAType.SMA`; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
 *   7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA,
 *   15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `stochfLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { MAType, StochfOut, stochfOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = stochfOpen(high, low, close, 5, 3, MAType.SMA);
 * const peeked = new StochfOut();
 * const updated = new StochfOut();
 * stream.peek(101.4, 99.1, 100.9, peeked); // a forming bar: commits nothing
 * stream.update(101.4, 99.1, 100.9, updated); // the bar closes
 * ```
 */
export function stochfOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInFastK_Period: number,
  optInFastD_Period: number,
  optInFastD_MAType: MAType | typeof Core.INTEGER_DEFAULT,
  core: Core = Core.DEFAULT,
): StochfStream {
  optInFastK_Period = requireInt32('STOCHF open', 'optInFastK_Period', optInFastK_Period);
  optInFastD_Period = requireInt32('STOCHF open', 'optInFastD_Period', optInFastD_Period);
  optInFastD_MAType = requireInt32('STOCHF open', 'optInFastD_MAType', optInFastD_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('STOCHF open', 'inHigh', inHigh);
  requireHistory('STOCHF open', inHigh.length);
  requireArgument('STOCHF open', 'inLow', inLow);
  requireArgument('STOCHF open', 'inClose', inClose);
  requireHistoryLength('STOCHF open', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('STOCHF open', 'inClose', inClose.length, inHigh.length);
  return stochfOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInFastK_Period, optInFastD_Period, optInFastD_MAType);
}

/**
 * {@link stochfOpen} that also fills the outputs over the whole history in the
 * same pass, bit-identically to {@link stochf}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInFastK_Period - Lookback window for the highest-high/lowest-low of
 *   Fast-K (default 5; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param optInFastD_Period - Smoothing period for the Fast-D line (default 3;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInFastD_MAType - Moving-average type used to smooth Fast-D (default
 *   `MAType.SMA`; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
 *   7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA,
 *   15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param outFastK - Raw %K stochastic line. Must hold `historyLen - lookback`
 *   values and share no memory with an input or another output.
 * @param outFastD - MA-smoothed %K (signal line) Must hold `historyLen - lookback`
 *   values and share no memory with an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `stochfLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range, histories of different lengths or an output too
 *   short or starting where another output or a `Float64Array` input starts, or, on
 *   a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { MAType, stochfOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const fastK = new Float64Array(high.length);
 * const fastD = new Float64Array(high.length);
 * const stream = stochfOpenAndFill(
 *   high, low, close,
 *   5, 3, MAType.SMA,
 *   fastK, fastD,
 * );
 * const { begIdx, count } = stream.outRange();
 * // fastK[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function stochfOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInFastK_Period: number,
  optInFastD_Period: number,
  optInFastD_MAType: MAType | typeof Core.INTEGER_DEFAULT,
  outFastK: Float64Array,
  outFastD: Float64Array,
  core: Core = Core.DEFAULT,
): StochfStream {
  optInFastK_Period = requireInt32('STOCHF openAndFill', 'optInFastK_Period', optInFastK_Period);
  optInFastD_Period = requireInt32('STOCHF openAndFill', 'optInFastD_Period', optInFastD_Period);
  optInFastD_MAType = requireInt32('STOCHF openAndFill', 'optInFastD_MAType', optInFastD_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('STOCHF openAndFill', 'inHigh', inHigh);
  requireHistory('STOCHF openAndFill', inHigh.length);
  requireArgument('STOCHF openAndFill', 'inLow', inLow);
  requireArgument('STOCHF openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('STOCHF openAndFill', inHigh.length, stochfLookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType as MAType, core));
  requireHistoryLength('STOCHF openAndFill', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('STOCHF openAndFill', 'inClose', inClose.length, inHigh.length);
  requireLength('STOCHF openAndFill', 'outFastK', outFastK, guardOutLen);
  requireLength('STOCHF openAndFill', 'outFastD', outFastD, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outFastK.buffer === inHighF64.buffer && outFastK.byteOffset === inHighF64.byteOffset) || (outFastK.buffer === inLowF64.buffer && outFastK.byteOffset === inLowF64.byteOffset) || (outFastK.buffer === inCloseF64.buffer && outFastK.byteOffset === inCloseF64.byteOffset) || (outFastD.buffer === inHighF64.buffer && outFastD.byteOffset === inHighF64.byteOffset) || (outFastD.buffer === inLowF64.buffer && outFastD.byteOffset === inLowF64.byteOffset) || (outFastD.buffer === inCloseF64.buffer && outFastD.byteOffset === inCloseF64.byteOffset) || (outFastK.buffer === outFastD.buffer && outFastK.byteOffset === outFastD.byteOffset)) {
    throw streamFailure('STOCHF openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outFastK, inHighF64) || overlaps(outFastK, inLowF64) || overlaps(outFastK, inCloseF64) || overlaps(outFastD, inHighF64) || overlaps(outFastD, inLowF64) || overlaps(outFastD, inCloseF64) || overlaps(outFastK, outFastD))) {
    throw streamFailure('STOCHF openAndFill', RetCode.BAD_PARAM);
  }
  return stochfOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInFastK_Period, optInFastD_Period, optInFastD_MAType, new MInteger(), new MInteger(), outFastK, outFastD);
}
