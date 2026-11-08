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
import { badSink, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { MaStream, maLookbackPositional, maOpenAndFillInternal, maOpenInternal, maPositional } from './ma.ts';

/** @internal */
export function stochLookbackPositional(optInFastK_Period: number, optInSlowK_Period: number, optInSlowK_MAType: MAType | typeof Core.INTEGER_DEFAULT, optInSlowD_Period: number, optInSlowD_MAType: MAType | typeof Core.INTEGER_DEFAULT, core: Core = Core.DEFAULT): number {
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
  retValue += maLookbackPositional(optInSlowK_Period, (optInSlowK_MAType as MAType), core);
  /* Add the smoothing being done for %D slow. */
  retValue += maLookbackPositional(optInSlowD_Period, (optInSlowD_MAType as MAType), core);
  return retValue;
}

/** @internal */
export function stochDisplayShiftPositional(optInFastK_Period: number, optInSlowK_Period: number, optInSlowK_MAType: MAType | typeof Core.INTEGER_DEFAULT, optInSlowD_Period: number, optInSlowD_MAType: MAType | typeof Core.INTEGER_DEFAULT, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (stochLookbackPositional(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, core) < 0) {
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
  lookbackKSlow = maLookbackPositional(optInSlowK_Period, (optInSlowK_MAType as MAType), core);
  lookbackDSlow = maLookbackPositional(optInSlowD_Period, (optInSlowD_MAType as MAType), core);
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
  const _xr0 = maPositional(0, outIdx - 1, tempBuffer, optInSlowK_Period, (optInSlowK_MAType as MAType), tempBuffer, core);
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
  const _xr1 = maPositional(0, outNBElement.value - 1, tempBuffer, optInSlowD_Period, (optInSlowD_MAType as MAType), outSlowD, core);
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

const PARAM_RANGES = [['fastKPeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['slowKPeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['slowKMaType', Core.INTEGER_DEFAULT, MATypes.Min, MATypes.Max, 'a MAType'], ['slowDPeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['slowDMaType', Core.INTEGER_DEFAULT, MATypes.Min, MATypes.Max, 'a MAType']] as const;

/** @internal */
export function stochPositional(
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
  optInFastK_Period = requireInt32('STOCH', 'fastKPeriod', optInFastK_Period);
  optInSlowK_Period = requireInt32('STOCH', 'slowKPeriod', optInSlowK_Period);
  optInSlowK_MAType = requireInt32('STOCH', 'slowKMaType', optInSlowK_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  optInSlowD_Period = requireInt32('STOCH', 'slowDPeriod', optInSlowD_Period);
  optInSlowD_MAType = requireInt32('STOCH', 'slowDMaType', optInSlowD_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireIndexRange('STOCH', startIdx, endIdx);
  const guardLookback = stochLookbackPositional(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, core);
  if (guardLookback < 0) {
    throw rejectedParams('STOCH', PARAM_RANGES, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType);
  }
  const guardStart = clampedStart('STOCH', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('STOCH', 'prices.high', inHigh, guardInLen);
  requireLength('STOCH', 'prices.low', inLow, guardInLen);
  requireLength('STOCH', 'prices.close', inClose, guardInLen);
  requireOutput('STOCH', 'outSlowK', outSlowK, guardOutLen, Float64Array);
  requireOutput('STOCH', 'outSlowD', outSlowD, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outSlowK, inHighF64)) {
    throw sharedMemory('STOCH', 'outSlowK', 'prices.high');
  }
  if (overlapsPartly(outSlowK, inLowF64)) {
    throw sharedMemory('STOCH', 'outSlowK', 'prices.low');
  }
  if (overlapsPartly(outSlowK, inCloseF64)) {
    throw sharedMemory('STOCH', 'outSlowK', 'prices.close');
  }
  if (overlapsPartly(outSlowD, inHighF64)) {
    throw sharedMemory('STOCH', 'outSlowD', 'prices.high');
  }
  if (overlapsPartly(outSlowD, inLowF64)) {
    throw sharedMemory('STOCH', 'outSlowD', 'prices.low');
  }
  if (overlapsPartly(outSlowD, inCloseF64)) {
    throw sharedMemory('STOCH', 'outSlowD', 'prices.close');
  }
  if (overlaps(outSlowK, outSlowD)) {
    throw sharedMemory('STOCH', 'outSlowK', 'outSlowD');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = stochImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, outSlowK, outSlowD);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('STOCH', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link stoch}, {@link stochLookback}, {@link stochDisplayShift},
 * {@link stochOpen} and {@link stochOpenAndFill}. Every key is optional: an
 * omitted or `undefined` key takes its default, and a call ignores the keys it has
 * no use for.
 */
export interface StochOptions {
  /**
   * Lookback window for the raw %K high-low range (default 5; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  fastKPeriod?: number | undefined;
  /**
   * Smoothing period turning FastK into SlowK (default 3; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  slowKPeriod?: number | undefined;
  /**
   * MA type used to smooth into SlowK (default `MAType.SMA`; values: 0=SMA, 1=EMA,
   * 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA, 10=DISABLED,
   * 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA; `Core.INTEGER_DEFAULT` or
   * `MAType.DEFAULT` selects the default).
   */
  slowKMaType?: MAType | typeof Core.INTEGER_DEFAULT | undefined;
  /**
   * Smoothing period for the SlowD signal line (default 3; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  slowDPeriod?: number | undefined;
  /**
   * MA type used for the SlowD line (default `MAType.SMA`; values: 0=SMA, 1=EMA,
   * 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA, 10=DISABLED,
   * 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA; `Core.INTEGER_DEFAULT` or
   * `MAType.DEFAULT` selects the default).
   */
  slowDMaType?: MAType | typeof Core.INTEGER_DEFAULT | undefined;
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
   * Buffer for `outSlowK`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outSlowK?: Float64Array<ArrayBuffer> | undefined;
  /**
   * Buffer for `outSlowD`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outSlowD?: Float64Array<ArrayBuffer> | undefined;
}

/** What {@link stoch} returns, and {@link stochOpenAndFill} with its stream. */
export interface StochResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Raw FastK smoothed by SlowK_Period MA. `outSlowK[k]` is the value at input bar
   * `begIdx + k`.
   */
  outSlowK: Float64Array<ArrayBuffer>;
  /**
   * Signal line: SlowK smoothed by SlowD_Period MA. `outSlowD[k]` is the value at
   * input bar `begIdx + k`.
   */
  outSlowD: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link stoch} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link StochOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function stochLookback(options?: StochOptions): number {
  const o = optionsOf('STOCH lookback', options);
  return stochLookbackPositional(orDefault(o.fastKPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowKPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowKMaType, Core.INTEGER_DEFAULT), orDefault(o.slowDPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowDMaType, Core.INTEGER_DEFAULT), coreOf('STOCH lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link stoch}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link StochOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function stochDisplayShift(outputIdx: number, options?: StochOptions): number {
  const o = optionsOf('STOCH displayShift', options);
  return stochDisplayShiftPositional(orDefault(o.fastKPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowKPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowKMaType, Core.INTEGER_DEFAULT), orDefault(o.slowDPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowDMaType, Core.INTEGER_DEFAULT), outputIdx, coreOf('STOCH displayShift', o.core));
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
 * `outSlowK[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link stochLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `high`: High price of
 *   each bar. `low`: Low price of each bar. `close`: Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link StochOptions}.
 * @returns `begIdx` and the values; see {@link StochResult}. Each output is a view
 *   of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, an integer parameter that is not an int32, a parameter outside its range,
 *   an input too short for the range, a buffer that is empty, too short or of the
 *   wrong type, two buffers sharing memory or a buffer sharing memory with a
 *   `Float64Array` input it is not. Checked before anything is written.
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
 * const { begIdx, outSlowK, outSlowD } = stoch({ high, low, close }, {
 *   fastKPeriod: 5,
 *   slowKPeriod: 3,
 *   slowKMaType: MAType.SMA,
 *   slowDPeriod: 3,
 *   slowDMaType: MAType.SMA,
 * });
 * // outSlowK[k] is the value at bar begIdx + k
 * ```
 * @see [`stochf`](https://ta-lib.org/functions/stochf)
 * @see [`stochrsi`](https://ta-lib.org/functions/stochrsi)
 * @see [`ma`](https://ta-lib.org/functions/ma)
 */
export function stoch(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: StochOptions): StochResult {
  requireArgument('STOCH', 'prices', prices);
  const o = optionsOf('STOCH', options);
  if (o.endIdx === undefined) {
    requireSeries('STOCH', 'prices.high', prices.high);
    requireSeries('STOCH', 'prices.low', prices.low);
    requireSeries('STOCH', 'prices.close', prices.close);
  }
  const optInFastK_Period = orDefault(o.fastKPeriod, Core.INTEGER_DEFAULT);
  const optInSlowK_Period = orDefault(o.slowKPeriod, Core.INTEGER_DEFAULT);
  const optInSlowK_MAType = orDefault(o.slowKMaType, Core.INTEGER_DEFAULT);
  const optInSlowD_Period = orDefault(o.slowDPeriod, Core.INTEGER_DEFAULT);
  const optInSlowD_MAType = orDefault(o.slowDMaType, Core.INTEGER_DEFAULT);
  const core = coreOf('STOCH', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, stochLookbackPositional(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, core), prices.high, prices.low, prices.close);
  const outSlowK = o.outSlowK === undefined ? new Float64Array(capacity) : o.outSlowK;
  const outSlowD = o.outSlowD === undefined ? new Float64Array(capacity) : o.outSlowD;
  const range = stochPositional(startIdx, endIdx, prices.high, prices.low, prices.close, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, outSlowK, outSlowD, core);
  return { begIdx: range.begIdx, outSlowK: outSlowK.subarray(0, range.count), outSlowD: outSlowD.subarray(0, range.count) };
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

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): StochStream {
    return new StochStream(core);
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
      throw streamFailure('STOCH advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns its outputs.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @param out - Receives the outputs and is returned; a new {@link StochOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included, before anything moves.
   *   `TALibError` with code `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached
   *   bar `Core.INDEX_MAX`.
   */
  update(high: number, low: number, close: number, out?: StochOut): StochOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('STOCH update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('STOCH update', 'StochOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('STOCH update', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    stochStepImpl(this, high, low, close);
    this.outRangeCount++;
    if (out === undefined) {
      out = new StochOut();
    }
    out.outSlowK = this.cur_outSlowK;
    out.outSlowD = this.cur_outSlowD;
    return out;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @param out - Receives the outputs and is returned; a new {@link StochOut} when
   *   omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included.
   */
  peek(high: number, low: number, close: number, out?: StochOut): StochOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('STOCH peek', 'StochOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('STOCH peek', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
    const inClose = close;
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
    if (out === undefined) {
      out = new StochOut();
    }
    out.outSlowK = cur_outSlowK;
    out.outSlowD = cur_outSlowD;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link StochOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a sink that is not an object,
   *   `null` included.
   */
  value(out?: StochOut): StochOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('STOCH value', 'StochOut');
    }
    if (out === undefined) {
      out = new StochOut();
    }
    out.outSlowK = this.cur_outSlowK;
    out.outSlowD = this.cur_outSlowD;
    return out;
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
 * The outputs of one STOCH bar, as `update`, `peek` and `value` return them.
 * Passed back to one of them, it is filled and returned again, with no allocation;
 * it holds its values until the next call that writes it. A new one holds `NaN`.
 */
export class StochOut {
  /** Raw FastK smoothed by SlowK_Period MA. */
  outSlowK: number = NaN;
  /** Signal line: SlowK smoothed by SlowD_Period MA. */
  outSlowD: number = NaN;
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
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen < stochLookbackPositional(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType as MAType, optInSlowD_Period, optInSlowD_MAType as MAType, core) + 1) {
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
  lookbackKSlow = maLookbackPositional(optInSlowK_Period, (optInSlowK_MAType as MAType), core);
  lookbackDSlow = maLookbackPositional(optInSlowD_Period, (optInSlowD_MAType as MAType), core);
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
  const _xr0 = maPositional(0, outIdx - 1, tempBuffer, optInSlowK_Period, (optInSlowK_MAType as MAType), tempBuffer, core);
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
  const sp = StochStream.of(core);
  const retCode = stochOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, outSlowK, outSlowD, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('STOCH openAndFill', inHigh.length, startIdx, stochLookbackPositional(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType as MAType, optInSlowD_Period, optInSlowD_MAType as MAType, core));
  }
  if (retCode === RetCode.BAD_PARAM && stochLookbackPositional(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType as MAType, optInSlowD_Period, optInSlowD_MAType as MAType, core) < 0) {
    throw rejectedParams('STOCH openAndFill', PARAM_RANGES, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType);
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
  const sp = StochStream.of(core);
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
    throw insufficientHistory('STOCH open', inHigh.length, startIdx, stochLookbackPositional(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType as MAType, optInSlowD_Period, optInSlowD_MAType as MAType, core));
  }
  if (retCode === RetCode.BAD_PARAM && stochLookbackPositional(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType as MAType, optInSlowD_Period, optInSlowD_MAType as MAType, core) < 0) {
    throw rejectedParams('STOCH open', PARAM_RANGES, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType);
  }
  throw streamFailure('STOCH open', retCode);
}

/** @internal */
export function stochOpenPositional(
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
  optInFastK_Period = requireInt32('STOCH open', 'fastKPeriod', optInFastK_Period);
  optInSlowK_Period = requireInt32('STOCH open', 'slowKPeriod', optInSlowK_Period);
  optInSlowK_MAType = requireInt32('STOCH open', 'slowKMaType', optInSlowK_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  optInSlowD_Period = requireInt32('STOCH open', 'slowDPeriod', optInSlowD_Period);
  optInSlowD_MAType = requireInt32('STOCH open', 'slowDMaType', optInSlowD_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('STOCH open', 'prices.high', inHigh);
  requireHistory('STOCH open', 'prices.high', inHigh.length);
  requireArgument('STOCH open', 'prices.low', inLow);
  requireArgument('STOCH open', 'prices.close', inClose);
  requireHistoryLength('STOCH open', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('STOCH open', 'prices.close', inClose.length, inHigh.length);
  return stochOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType);
}

/** @internal */
export function stochOpenAndFillPositional(
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
  optInFastK_Period = requireInt32('STOCH openAndFill', 'fastKPeriod', optInFastK_Period);
  optInSlowK_Period = requireInt32('STOCH openAndFill', 'slowKPeriod', optInSlowK_Period);
  optInSlowK_MAType = requireInt32('STOCH openAndFill', 'slowKMaType', optInSlowK_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  optInSlowD_Period = requireInt32('STOCH openAndFill', 'slowDPeriod', optInSlowD_Period);
  optInSlowD_MAType = requireInt32('STOCH openAndFill', 'slowDMaType', optInSlowD_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('STOCH openAndFill', 'prices.high', inHigh);
  requireHistory('STOCH openAndFill', 'prices.high', inHigh.length);
  requireArgument('STOCH openAndFill', 'prices.low', inLow);
  requireArgument('STOCH openAndFill', 'prices.close', inClose);
  const guardLookback = stochLookbackPositional(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType as MAType, optInSlowD_Period, optInSlowD_MAType as MAType, core);
  if (guardLookback < 0) {
    throw rejectedParams('STOCH openAndFill', PARAM_RANGES, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType);
  }
  const guardOutLen = openFillCount('STOCH openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('STOCH openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('STOCH openAndFill', 'prices.close', inClose.length, inHigh.length);
  requireOutput('STOCH openAndFill', 'outSlowK', outSlowK, guardOutLen, Float64Array);
  requireOutput('STOCH openAndFill', 'outSlowD', outSlowD, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outSlowK.buffer === inHighF64.buffer && outSlowK.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('STOCH openAndFill', 'outSlowK', 'prices.high');
  }
  if (outSlowK.buffer === inLowF64.buffer && outSlowK.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('STOCH openAndFill', 'outSlowK', 'prices.low');
  }
  if (outSlowK.buffer === inCloseF64.buffer && outSlowK.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('STOCH openAndFill', 'outSlowK', 'prices.close');
  }
  if (outSlowD.buffer === inHighF64.buffer && outSlowD.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('STOCH openAndFill', 'outSlowD', 'prices.high');
  }
  if (outSlowD.buffer === inLowF64.buffer && outSlowD.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('STOCH openAndFill', 'outSlowD', 'prices.low');
  }
  if (outSlowD.buffer === inCloseF64.buffer && outSlowD.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('STOCH openAndFill', 'outSlowD', 'prices.close');
  }
  if (outSlowK.buffer === outSlowD.buffer && outSlowK.byteOffset === outSlowD.byteOffset) {
    throw sharedMemory('STOCH openAndFill', 'outSlowK', 'outSlowD');
  }
  if (guardOutLen > 0 && overlaps(outSlowK, inHighF64)) {
    throw sharedMemory('STOCH openAndFill', 'outSlowK', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outSlowK, inLowF64)) {
    throw sharedMemory('STOCH openAndFill', 'outSlowK', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outSlowK, inCloseF64)) {
    throw sharedMemory('STOCH openAndFill', 'outSlowK', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outSlowD, inHighF64)) {
    throw sharedMemory('STOCH openAndFill', 'outSlowD', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outSlowD, inLowF64)) {
    throw sharedMemory('STOCH openAndFill', 'outSlowD', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outSlowD, inCloseF64)) {
    throw sharedMemory('STOCH openAndFill', 'outSlowD', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outSlowK, outSlowD)) {
    throw sharedMemory('STOCH openAndFill', 'outSlowK', 'outSlowD');
  }
  return stochOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, new MInteger(), new MInteger(), outSlowK, outSlowD);
}

/**
 * Opens a live STOCH stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link stoch} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar. `close`: Close
 *   price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link StochOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `stochLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range
 *   or histories of different lengths.
 *
 * @example
 * ```ts
 * import { MAType, stochOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = stochOpen({ high, low, close }, {
 *   fastKPeriod: 5,
 *   slowKPeriod: 3,
 *   slowKMaType: MAType.SMA,
 *   slowDPeriod: 3,
 *   slowDMaType: MAType.SMA,
 * });
 * const peeked = stream.peek(101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function stochOpen(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: StochOptions): StochStream {
  requireArgument('STOCH open', 'prices', prices);
  const o = optionsOf('STOCH open', options);
  const optInFastK_Period = orDefault(o.fastKPeriod, Core.INTEGER_DEFAULT);
  const optInSlowK_Period = orDefault(o.slowKPeriod, Core.INTEGER_DEFAULT);
  const optInSlowK_MAType = orDefault(o.slowKMaType, Core.INTEGER_DEFAULT);
  const optInSlowD_Period = orDefault(o.slowDPeriod, Core.INTEGER_DEFAULT);
  const optInSlowD_MAType = orDefault(o.slowDMaType, Core.INTEGER_DEFAULT);
  const core = coreOf('STOCH open', o.core);
  return stochOpenPositional(prices.high, prices.low, prices.close, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, core);
}

/**
 * {@link stochOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link stoch}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar. `close`: Close
 *   price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link StochOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link stoch} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `stochLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
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
 * const { stream, begIdx, outSlowK, outSlowD } = stochOpenAndFill({ high, low, close }, {
 *   fastKPeriod: 5,
 *   slowKPeriod: 3,
 *   slowKMaType: MAType.SMA,
 *   slowDPeriod: 3,
 *   slowDMaType: MAType.SMA,
 * });
 * // outSlowK[k] is the value at bar begIdx + k
 * ```
 */
export function stochOpenAndFill(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: StochOptions): StochResult & { stream: StochStream } {
  requireArgument('STOCH openAndFill', 'prices', prices);
  const o = optionsOf('STOCH openAndFill', options);
  const optInFastK_Period = orDefault(o.fastKPeriod, Core.INTEGER_DEFAULT);
  const optInSlowK_Period = orDefault(o.slowKPeriod, Core.INTEGER_DEFAULT);
  const optInSlowK_MAType = orDefault(o.slowKMaType, Core.INTEGER_DEFAULT);
  const optInSlowD_Period = orDefault(o.slowDPeriod, Core.INTEGER_DEFAULT);
  const optInSlowD_MAType = orDefault(o.slowDMaType, Core.INTEGER_DEFAULT);
  const core = coreOf('STOCH openAndFill', o.core);
  const capacity = fillCapacity(stochLookbackPositional(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, core), prices.high, prices.low, prices.close);
  const outSlowK = o.outSlowK === undefined ? new Float64Array(capacity) : o.outSlowK;
  const outSlowD = o.outSlowD === undefined ? new Float64Array(capacity) : o.outSlowD;
  const stream = stochOpenAndFillPositional(prices.high, prices.low, prices.close, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, outSlowK, outSlowD, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outSlowK: outSlowK.subarray(0, range.count), outSlowD: outSlowD.subarray(0, range.count) };
}
