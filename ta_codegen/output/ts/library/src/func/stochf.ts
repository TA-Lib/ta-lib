/*! TA-LIB Copyright (c) 1999-2026, Mario Fortier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither name of author nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


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
import { badSink, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { MaStream, maLookbackPositional, maOpenAndFillInternal, maPositional } from './ma.ts';

/** @internal */
export function stochfLookbackPositional(optInFastK_Period: number, optInFastD_Period: number, optInFastD_MAType: MAType | typeof Core.INTEGER_DEFAULT, core: Core = Core.DEFAULT): number {
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
  retValue += maLookbackPositional(optInFastD_Period, (optInFastD_MAType as MAType), core);
  return retValue;
}

/** @internal */
export function stochfDisplayShiftPositional(optInFastK_Period: number, optInFastD_Period: number, optInFastD_MAType: MAType | typeof Core.INTEGER_DEFAULT, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (stochfLookbackPositional(optInFastK_Period, optInFastD_Period, optInFastD_MAType, core) < 0) {
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
  lookbackFastD = maLookbackPositional(optInFastD_Period, (optInFastD_MAType as MAType), core);
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
  const _xr0 = maPositional(0, outIdx - 1, tempBuffer, optInFastD_Period, (optInFastD_MAType as MAType), outFastD, core);
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

const PARAM_RANGES = [['fastKPeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['fastDPeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['fastDMaType', Core.INTEGER_DEFAULT, MATypes.Min, MATypes.Max, 'a MAType']] as const;

/** @internal */
export function stochfPositional(
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
  optInFastK_Period = requireInt32('STOCHF', 'fastKPeriod', optInFastK_Period);
  optInFastD_Period = requireInt32('STOCHF', 'fastDPeriod', optInFastD_Period);
  optInFastD_MAType = requireInt32('STOCHF', 'fastDMaType', optInFastD_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireIndexRange('STOCHF', startIdx, endIdx);
  const guardLookback = stochfLookbackPositional(optInFastK_Period, optInFastD_Period, optInFastD_MAType, core);
  if (guardLookback < 0) {
    throw rejectedParams('STOCHF', PARAM_RANGES, optInFastK_Period, optInFastD_Period, optInFastD_MAType);
  }
  const guardStart = clampedStart('STOCHF', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('STOCHF', 'prices.high', inHigh, guardInLen);
  requireLength('STOCHF', 'prices.low', inLow, guardInLen);
  requireLength('STOCHF', 'prices.close', inClose, guardInLen);
  requireOutput('STOCHF', 'outFastK', outFastK, guardOutLen, Float64Array);
  requireOutput('STOCHF', 'outFastD', outFastD, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outFastK, inHighF64)) {
    throw sharedMemory('STOCHF', 'outFastK', 'prices.high');
  }
  if (overlapsPartly(outFastK, inLowF64)) {
    throw sharedMemory('STOCHF', 'outFastK', 'prices.low');
  }
  if (overlapsPartly(outFastK, inCloseF64)) {
    throw sharedMemory('STOCHF', 'outFastK', 'prices.close');
  }
  if (overlapsPartly(outFastD, inHighF64)) {
    throw sharedMemory('STOCHF', 'outFastD', 'prices.high');
  }
  if (overlapsPartly(outFastD, inLowF64)) {
    throw sharedMemory('STOCHF', 'outFastD', 'prices.low');
  }
  if (overlapsPartly(outFastD, inCloseF64)) {
    throw sharedMemory('STOCHF', 'outFastD', 'prices.close');
  }
  if (overlaps(outFastK, outFastD)) {
    throw sharedMemory('STOCHF', 'outFastK', 'outFastD');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = stochfImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx, outNBElement, outFastK, outFastD);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('STOCHF', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link stochf}, {@link stochfLookback}, {@link stochfDisplayShift},
 * {@link stochfOpen} and {@link stochfOpenAndFill}. Every key is optional: an
 * omitted or `undefined` key takes its default, and a call ignores the keys it has
 * no use for.
 */
export interface StochfOptions {
  /**
   * Lookback window for the highest-high/lowest-low of Fast-K (default 5; range 1 to
   * 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  fastKPeriod?: number | undefined;
  /**
   * Smoothing period for the Fast-D line (default 3; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  fastDPeriod?: number | undefined;
  /**
   * Moving-average type used to smooth Fast-D (default `MAType.SMA`; values: 0=SMA,
   * 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA, 10=DISABLED,
   * 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA; `Core.INTEGER_DEFAULT` or
   * `MAType.DEFAULT` selects the default).
   */
  fastDMaType?: MAType | typeof Core.INTEGER_DEFAULT | undefined;
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
   * Buffer for `outFastK`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outFastK?: Float64Array<ArrayBuffer> | undefined;
  /**
   * Buffer for `outFastD`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outFastD?: Float64Array<ArrayBuffer> | undefined;
}

/** What {@link stochf} returns, and {@link stochfOpenAndFill} with its stream. */
export interface StochfResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /** Raw %K stochastic line. `outFastK[k]` is the value at input bar `begIdx + k`. */
  outFastK: Float64Array<ArrayBuffer>;
  /**
   * MA-smoothed %K (signal line). `outFastD[k]` is the value at input bar
   * `begIdx + k`.
   */
  outFastD: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link stochf} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link StochfOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function stochfLookback(options?: StochfOptions): number {
  const o = optionsOf('STOCHF lookback', options);
  return stochfLookbackPositional(orDefault(o.fastKPeriod, Core.INTEGER_DEFAULT), orDefault(o.fastDPeriod, Core.INTEGER_DEFAULT), orDefault(o.fastDMaType, Core.INTEGER_DEFAULT), coreOf('STOCHF lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link stochf}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link StochfOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function stochfDisplayShift(outputIdx: number, options?: StochfOptions): number {
  const o = optionsOf('STOCHF displayShift', options);
  return stochfDisplayShiftPositional(orDefault(o.fastKPeriod, Core.INTEGER_DEFAULT), orDefault(o.fastDPeriod, Core.INTEGER_DEFAULT), orDefault(o.fastDMaType, Core.INTEGER_DEFAULT), outputIdx, coreOf('STOCHF displayShift', o.core));
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
 * `outFastK[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link stochfLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `high`: High price of
 *   each bar. `low`: Low price of each bar. `close`: Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link StochfOptions}.
 * @returns `begIdx` and the values; see {@link StochfResult}. Each output is a
 *   view of its buffer, the caller's or a new one.
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
 * import { MAType, stochf } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { begIdx, outFastK, outFastD } = stochf({ high, low, close }, {
 *   fastKPeriod: 5,
 *   fastDPeriod: 3,
 *   fastDMaType: MAType.SMA,
 * });
 * // outFastK[k] is the value at bar begIdx + k
 * ```
 * @see [`stoch`](https://ta-lib.org/functions/stoch)
 * @see [`stochrsi`](https://ta-lib.org/functions/stochrsi)
 * @see [`ma`](https://ta-lib.org/functions/ma)
 */
export function stochf(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: StochfOptions): StochfResult {
  requireArgument('STOCHF', 'prices', prices);
  const o = optionsOf('STOCHF', options);
  if (o.endIdx === undefined) {
    requireSeries('STOCHF', 'prices.high', prices.high);
    requireSeries('STOCHF', 'prices.low', prices.low);
    requireSeries('STOCHF', 'prices.close', prices.close);
  }
  const optInFastK_Period = orDefault(o.fastKPeriod, Core.INTEGER_DEFAULT);
  const optInFastD_Period = orDefault(o.fastDPeriod, Core.INTEGER_DEFAULT);
  const optInFastD_MAType = orDefault(o.fastDMaType, Core.INTEGER_DEFAULT);
  const core = coreOf('STOCHF', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, stochfLookbackPositional(optInFastK_Period, optInFastD_Period, optInFastD_MAType, core), prices.high, prices.low, prices.close);
  const outFastK = o.outFastK === undefined ? new Float64Array(capacity) : o.outFastK;
  const outFastD = o.outFastD === undefined ? new Float64Array(capacity) : o.outFastD;
  const range = stochfPositional(startIdx, endIdx, prices.high, prices.low, prices.close, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outFastK, outFastD, core);
  return { begIdx: range.begIdx, outFastK: outFastK.subarray(0, range.count), outFastD: outFastD.subarray(0, range.count) };
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
      throw streamFailure('STOCHF advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns its outputs.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @param out - Receives the outputs and is returned; a new {@link StochfOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included, before anything moves.
   *   `TALibError` with code `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached
   *   bar `Core.INDEX_MAX`.
   */
  update(high: number, low: number, close: number, out?: StochfOut): StochfOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('STOCHF update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('STOCHF update', 'StochfOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('STOCHF update', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    stochfStepImpl(this, high, low, close);
    this.outRangeCount++;
    if (out === undefined) {
      out = new StochfOut();
    }
    out.outFastK = this.cur_outFastK;
    out.outFastD = this.cur_outFastD;
    return out;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @param out - Receives the outputs and is returned; a new {@link StochfOut} when
   *   omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included.
   */
  peek(high: number, low: number, close: number, out?: StochfOut): StochfOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('STOCHF peek', 'StochfOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('STOCHF peek', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
    const inClose = close;
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
    if (out === undefined) {
      out = new StochfOut();
    }
    out.outFastK = cur_outFastK;
    out.outFastD = cur_outFastD;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link StochfOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a sink that is not an object,
   *   `null` included.
   */
  value(out?: StochfOut): StochfOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('STOCHF value', 'StochfOut');
    }
    if (out === undefined) {
      out = new StochfOut();
    }
    out.outFastK = this.cur_outFastK;
    out.outFastD = this.cur_outFastD;
    return out;
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
 * The outputs of one STOCHF bar, as `update`, `peek` and `value` return them.
 * Passed back to one of them, it is filled and returned again, with no allocation;
 * it holds its values until the next call that writes it. A new one holds `NaN`.
 */
export class StochfOut {
  /** Raw %K stochastic line. */
  outFastK: number = NaN;
  /** MA-smoothed %K (signal line) */
  outFastD: number = NaN;
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
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen < stochfLookbackPositional(optInFastK_Period, optInFastD_Period, optInFastD_MAType as MAType, core) + 1) {
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
  lookbackFastD = maLookbackPositional(optInFastD_Period, (optInFastD_MAType as MAType), core);
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
    throw insufficientHistory('STOCHF openAndFill', inHigh.length, startIdx, stochfLookbackPositional(optInFastK_Period, optInFastD_Period, optInFastD_MAType as MAType, core));
  }
  if (retCode === RetCode.BAD_PARAM && stochfLookbackPositional(optInFastK_Period, optInFastD_Period, optInFastD_MAType as MAType, core) < 0) {
    throw rejectedParams('STOCHF openAndFill', PARAM_RANGES, optInFastK_Period, optInFastD_Period, optInFastD_MAType);
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
    throw insufficientHistory('STOCHF open', inHigh.length, startIdx, stochfLookbackPositional(optInFastK_Period, optInFastD_Period, optInFastD_MAType as MAType, core));
  }
  if (retCode === RetCode.BAD_PARAM && stochfLookbackPositional(optInFastK_Period, optInFastD_Period, optInFastD_MAType as MAType, core) < 0) {
    throw rejectedParams('STOCHF open', PARAM_RANGES, optInFastK_Period, optInFastD_Period, optInFastD_MAType);
  }
  throw streamFailure('STOCHF open', retCode);
}

/** @internal */
export function stochfOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInFastK_Period: number,
  optInFastD_Period: number,
  optInFastD_MAType: MAType | typeof Core.INTEGER_DEFAULT,
  core: Core = Core.DEFAULT,
): StochfStream {
  optInFastK_Period = requireInt32('STOCHF open', 'fastKPeriod', optInFastK_Period);
  optInFastD_Period = requireInt32('STOCHF open', 'fastDPeriod', optInFastD_Period);
  optInFastD_MAType = requireInt32('STOCHF open', 'fastDMaType', optInFastD_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('STOCHF open', 'prices.high', inHigh);
  requireHistory('STOCHF open', 'prices.high', inHigh.length);
  requireArgument('STOCHF open', 'prices.low', inLow);
  requireArgument('STOCHF open', 'prices.close', inClose);
  requireHistoryLength('STOCHF open', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('STOCHF open', 'prices.close', inClose.length, inHigh.length);
  return stochfOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInFastK_Period, optInFastD_Period, optInFastD_MAType);
}

/** @internal */
export function stochfOpenAndFillPositional(
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
  optInFastK_Period = requireInt32('STOCHF openAndFill', 'fastKPeriod', optInFastK_Period);
  optInFastD_Period = requireInt32('STOCHF openAndFill', 'fastDPeriod', optInFastD_Period);
  optInFastD_MAType = requireInt32('STOCHF openAndFill', 'fastDMaType', optInFastD_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('STOCHF openAndFill', 'prices.high', inHigh);
  requireHistory('STOCHF openAndFill', 'prices.high', inHigh.length);
  requireArgument('STOCHF openAndFill', 'prices.low', inLow);
  requireArgument('STOCHF openAndFill', 'prices.close', inClose);
  const guardLookback = stochfLookbackPositional(optInFastK_Period, optInFastD_Period, optInFastD_MAType as MAType, core);
  if (guardLookback < 0) {
    throw rejectedParams('STOCHF openAndFill', PARAM_RANGES, optInFastK_Period, optInFastD_Period, optInFastD_MAType);
  }
  const guardOutLen = openFillCount('STOCHF openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('STOCHF openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('STOCHF openAndFill', 'prices.close', inClose.length, inHigh.length);
  requireOutput('STOCHF openAndFill', 'outFastK', outFastK, guardOutLen, Float64Array);
  requireOutput('STOCHF openAndFill', 'outFastD', outFastD, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outFastK.buffer === inHighF64.buffer && outFastK.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('STOCHF openAndFill', 'outFastK', 'prices.high');
  }
  if (outFastK.buffer === inLowF64.buffer && outFastK.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('STOCHF openAndFill', 'outFastK', 'prices.low');
  }
  if (outFastK.buffer === inCloseF64.buffer && outFastK.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('STOCHF openAndFill', 'outFastK', 'prices.close');
  }
  if (outFastD.buffer === inHighF64.buffer && outFastD.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('STOCHF openAndFill', 'outFastD', 'prices.high');
  }
  if (outFastD.buffer === inLowF64.buffer && outFastD.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('STOCHF openAndFill', 'outFastD', 'prices.low');
  }
  if (outFastD.buffer === inCloseF64.buffer && outFastD.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('STOCHF openAndFill', 'outFastD', 'prices.close');
  }
  if (outFastK.buffer === outFastD.buffer && outFastK.byteOffset === outFastD.byteOffset) {
    throw sharedMemory('STOCHF openAndFill', 'outFastK', 'outFastD');
  }
  if (guardOutLen > 0 && overlaps(outFastK, inHighF64)) {
    throw sharedMemory('STOCHF openAndFill', 'outFastK', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outFastK, inLowF64)) {
    throw sharedMemory('STOCHF openAndFill', 'outFastK', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outFastK, inCloseF64)) {
    throw sharedMemory('STOCHF openAndFill', 'outFastK', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outFastD, inHighF64)) {
    throw sharedMemory('STOCHF openAndFill', 'outFastD', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outFastD, inLowF64)) {
    throw sharedMemory('STOCHF openAndFill', 'outFastD', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outFastD, inCloseF64)) {
    throw sharedMemory('STOCHF openAndFill', 'outFastD', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outFastK, outFastD)) {
    throw sharedMemory('STOCHF openAndFill', 'outFastK', 'outFastD');
  }
  return stochfOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInFastK_Period, optInFastD_Period, optInFastD_MAType, new MInteger(), new MInteger(), outFastK, outFastD);
}

/**
 * Opens a live STOCHF stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link stochf} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar. `close`: Close
 *   price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link StochfOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `stochfLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range
 *   or histories of different lengths.
 *
 * @example
 * ```ts
 * import { MAType, stochfOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = stochfOpen({ high, low, close }, {
 *   fastKPeriod: 5,
 *   fastDPeriod: 3,
 *   fastDMaType: MAType.SMA,
 * });
 * const peeked = stream.peek(101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function stochfOpen(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: StochfOptions): StochfStream {
  requireArgument('STOCHF open', 'prices', prices);
  const o = optionsOf('STOCHF open', options);
  const optInFastK_Period = orDefault(o.fastKPeriod, Core.INTEGER_DEFAULT);
  const optInFastD_Period = orDefault(o.fastDPeriod, Core.INTEGER_DEFAULT);
  const optInFastD_MAType = orDefault(o.fastDMaType, Core.INTEGER_DEFAULT);
  const core = coreOf('STOCHF open', o.core);
  return stochfOpenPositional(prices.high, prices.low, prices.close, optInFastK_Period, optInFastD_Period, optInFastD_MAType, core);
}

/**
 * {@link stochfOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link stochf}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar. `close`: Close
 *   price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link StochfOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link stochf} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `stochfLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
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
 * const { stream, begIdx, outFastK, outFastD } = stochfOpenAndFill({ high, low, close }, {
 *   fastKPeriod: 5,
 *   fastDPeriod: 3,
 *   fastDMaType: MAType.SMA,
 * });
 * // outFastK[k] is the value at bar begIdx + k
 * ```
 */
export function stochfOpenAndFill(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: StochfOptions): StochfResult & { stream: StochfStream } {
  requireArgument('STOCHF openAndFill', 'prices', prices);
  const o = optionsOf('STOCHF openAndFill', options);
  const optInFastK_Period = orDefault(o.fastKPeriod, Core.INTEGER_DEFAULT);
  const optInFastD_Period = orDefault(o.fastDPeriod, Core.INTEGER_DEFAULT);
  const optInFastD_MAType = orDefault(o.fastDMaType, Core.INTEGER_DEFAULT);
  const core = coreOf('STOCHF openAndFill', o.core);
  const capacity = fillCapacity(stochfLookbackPositional(optInFastK_Period, optInFastD_Period, optInFastD_MAType, core), prices.high, prices.low, prices.close);
  const outFastK = o.outFastK === undefined ? new Float64Array(capacity) : o.outFastK;
  const outFastD = o.outFastD === undefined ? new Float64Array(capacity) : o.outFastD;
  const stream = stochfOpenAndFillPositional(prices.high, prices.low, prices.close, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outFastK, outFastD, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outFastK: outFastK.subarray(0, range.count), outFastD: outFastD.subarray(0, range.count) };
}
