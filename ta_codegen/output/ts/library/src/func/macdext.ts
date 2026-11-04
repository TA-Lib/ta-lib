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
 *  010802 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  070526 MF,CC  Speed optimization: delegate to the single-pass MACD
 *                when all three MA types are EMA (bit-exact).
 *  100226 MF,CC  #497. The consistency exit leaves the range untouched.
 */

import { MAType, MATypes } from '../enums.ts';
import { badSink, clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { MaStream, ma, maLookback, maOpenAndFillInternal } from './ma.ts';
import { macd } from './macd.ts';

/**
 * Number of leading input bars {@link macdext} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param optInFastPeriod - Period of the fast MA (default 12; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param optInFastMAType - MA type for the fast MA (default `MAType.SMA`; values:
 *   0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA,
 *   10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
 *   `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param optInSlowPeriod - Period of the slow MA (default 26; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowMAType - MA type for the slow MA (default `MAType.SMA`; values:
 *   0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA,
 *   10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
 *   `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param optInSignalPeriod - Period of the signal-line MA (default 9; range 1 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSignalMAType - MA type for the signal line (default `MAType.SMA`;
 *   values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3,
 *   9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
 *   `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function macdextLookback(optInFastPeriod: number, optInFastMAType: MAType | typeof Core.INTEGER_DEFAULT, optInSlowPeriod: number, optInSlowMAType: MAType | typeof Core.INTEGER_DEFAULT, optInSignalPeriod: number, optInSignalMAType: MAType | typeof Core.INTEGER_DEFAULT, core: Core = Core.DEFAULT): number {
  if ((optInFastPeriod | 0) !== optInFastPeriod) {
    return -1;
  }
  if ((optInFastMAType | 0) !== optInFastMAType) {
    return -1;
  }
  if ((optInSlowPeriod | 0) !== optInSlowPeriod) {
    return -1;
  }
  if ((optInSlowMAType | 0) !== optInSlowMAType) {
    return -1;
  }
  if ((optInSignalPeriod | 0) !== optInSignalPeriod) {
    return -1;
  }
  if ((optInSignalMAType | 0) !== optInSignalMAType) {
    return -1;
  }
  if (optInFastPeriod === Core.INTEGER_DEFAULT) {
    optInFastPeriod = 12;
  } else if (optInFastPeriod < 2 || optInFastPeriod > 100000) {
    return -1;
  }
  if (optInFastMAType === Core.INTEGER_DEFAULT || optInFastMAType === MAType.DEFAULT) {
    optInFastMAType = MAType.SMA;
  } else if (optInFastMAType < MATypes.Min || optInFastMAType > MATypes.Max) {
    return -1;
  }
  if (optInSlowPeriod === Core.INTEGER_DEFAULT) {
    optInSlowPeriod = 26;
  } else if (optInSlowPeriod < 2 || optInSlowPeriod > 100000) {
    return -1;
  }
  if (optInSlowMAType === Core.INTEGER_DEFAULT || optInSlowMAType === MAType.DEFAULT) {
    optInSlowMAType = MAType.SMA;
  } else if (optInSlowMAType < MATypes.Min || optInSlowMAType > MATypes.Max) {
    return -1;
  }
  if (optInSignalPeriod === Core.INTEGER_DEFAULT) {
    optInSignalPeriod = 9;
  } else if (optInSignalPeriod < 1 || optInSignalPeriod > 100000) {
    return -1;
  }
  if (optInSignalMAType === Core.INTEGER_DEFAULT || optInSignalMAType === MAType.DEFAULT) {
    optInSignalMAType = MAType.SMA;
  } else if (optInSignalMAType < MATypes.Min || optInSignalMAType > MATypes.Max) {
    return -1;
  }
  let tempInteger = 0;
  let lookbackLargest = 0;
  /* Find the MA with the largest lookback */
  lookbackLargest = maLookback(optInFastPeriod, (optInFastMAType as MAType), core);
  tempInteger = maLookback(optInSlowPeriod, (optInSlowMAType as MAType), core);
  if (tempInteger > lookbackLargest) {
    lookbackLargest = tempInteger;
  }
  /* Add to the largest MA lookback the signal line lookback */
  return lookbackLargest + maLookback(optInSignalPeriod, (optInSignalMAType as MAType), core);
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link macdext}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInFastPeriod - Period of the fast MA (default 12; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param optInFastMAType - MA type for the fast MA (default `MAType.SMA`; values:
 *   0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA,
 *   10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
 *   `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param optInSlowPeriod - Period of the slow MA (default 26; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowMAType - MA type for the slow MA (default `MAType.SMA`; values:
 *   0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA,
 *   10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
 *   `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param optInSignalPeriod - Period of the signal-line MA (default 9; range 1 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSignalMAType - MA type for the signal line (default `MAType.SMA`;
 *   values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3,
 *   9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
 *   `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function macdextDisplayShift(optInFastPeriod: number, optInFastMAType: MAType | typeof Core.INTEGER_DEFAULT, optInSlowPeriod: number, optInSlowMAType: MAType | typeof Core.INTEGER_DEFAULT, optInSignalPeriod: number, optInSignalMAType: MAType | typeof Core.INTEGER_DEFAULT, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (macdextLookback(optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 3) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function macdextImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInFastPeriod: number,
  optInFastMAType: number,
  optInSlowPeriod: number,
  optInSlowMAType: number,
  optInSignalPeriod: number,
  optInSignalMAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outMACD: Float64Array,
  outMACDSignal: Float64Array,
  outMACDHist: Float64Array,
): RetCode {
  let slowMABuffer!: Float64Array;
  let fastMABuffer!: Float64Array;
  let tempInteger = 0;
  const outBegIdx1 = new MInteger();
  const outNbElement1 = new MInteger();
  const outBegIdx2 = new MInteger();
  const outNbElement2 = new MInteger();
  let lookbackTotal = 0;
  let lookbackSignal = 0;
  let lookbackLargest = 0;
  let i = 0;
  let tempMAType = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInFastPeriod === Core.INTEGER_DEFAULT) {
    optInFastPeriod = 12;
  } else if (optInFastPeriod < 2 || optInFastPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInFastMAType === Core.INTEGER_DEFAULT || optInFastMAType === MAType.DEFAULT) {
    optInFastMAType = MAType.SMA;
  } else if (optInFastMAType < MATypes.Min || optInFastMAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowPeriod === Core.INTEGER_DEFAULT) {
    optInSlowPeriod = 26;
  } else if (optInSlowPeriod < 2 || optInSlowPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowMAType === Core.INTEGER_DEFAULT || optInSlowMAType === MAType.DEFAULT) {
    optInSlowMAType = MAType.SMA;
  } else if (optInSlowMAType < MATypes.Min || optInSlowMAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if (optInSignalPeriod === Core.INTEGER_DEFAULT) {
    optInSignalPeriod = 9;
  } else if (optInSignalPeriod < 1 || optInSignalPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSignalMAType === Core.INTEGER_DEFAULT || optInSignalMAType === MAType.DEFAULT) {
    optInSignalMAType = MAType.SMA;
  } else if (optInSignalMAType < MATypes.Min || optInSignalMAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if ((outMACD.buffer === outMACDSignal.buffer && outMACD.byteOffset === outMACDSignal.byteOffset) || (outMACD.buffer === outMACDHist.buffer && outMACD.byteOffset === outMACDHist.byteOffset) || (outMACDSignal.buffer === outMACDHist.buffer && outMACDSignal.byteOffset === outMACDHist.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  if (optInFastMAType === MAType.EMA && optInSlowMAType === MAType.EMA && optInSignalMAType === MAType.EMA && optInFastPeriod >= 2 && optInSlowPeriod >= 2 && optInSignalPeriod >= 2) {
    /* An all-EMA MACDEXT computes exactly what MACD computes. Delegate
     * to its single-pass implementation. Period 1 stays on the generic
     * path: ma() copies the input for it instead of running an EMA
     * recursion.
     *
     * This block is a batch-only specialization: the generator strips it
     * from the streaming tier, which composes the general three-MA path for
     * every parameter value. The two agreeing bit for bit is not assumed --
     * stream_verify's multi-enum diagonal selects all-EMA and holds this
     * block to the composed path (issue #181). Keep the comment INSIDE the
     * block: above it, the stream inherits it and reads as if it delegated.
     */
    const _xr0 = macd(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outMACD, outMACDSignal, outMACDHist, core);
    outBegIdx.value = _xr0.begIdx;
    outNBElement.value = _xr0.count;
    return RetCode.SUCCESS;
  }
  /* Make sure slow is really slower than
   * the fast period! if not, swap...
   */
  if (optInSlowPeriod < optInFastPeriod) {
    /* swap period */
    tempInteger = optInSlowPeriod;
    optInSlowPeriod = optInFastPeriod;
    optInFastPeriod = tempInteger;
    /* swap type */
    tempMAType = optInSlowMAType;
    optInSlowMAType = optInFastMAType;
    optInFastMAType = tempMAType;
  }
  /* Find the MA with the largest lookback */
  lookbackLargest = maLookback(optInFastPeriod, (optInFastMAType as MAType), core);
  tempInteger = maLookback(optInSlowPeriod, (optInSlowMAType as MAType), core);
  if (tempInteger > lookbackLargest) {
    lookbackLargest = tempInteger;
  }
  /* Add the lookback needed for the signal line */
  lookbackSignal = maLookback(optInSignalPeriod, (optInSignalMAType as MAType), core);
  lookbackTotal = lookbackSignal + lookbackLargest;
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
  /* Allocate intermediate buffer for fast/slow MA. */
  tempInteger = endIdx - startIdx + 1 + lookbackSignal;
  fastMABuffer = new Float64Array(tempInteger);
  slowMABuffer = new Float64Array(tempInteger);
  /* Calculate the slow MA.
   *
   * Move back the startIdx to get enough data
   * for the signal period. That way, once the
   * signal calculation is done, all the output
   * will start at the requested 'startIdx'.
   */
  tempInteger = startIdx - lookbackSignal;
  const _xr1 = ma(tempInteger, endIdx, inReal, optInSlowPeriod, (optInSlowMAType as MAType), slowMABuffer, core);
  outBegIdx1.value = _xr1.begIdx;
  outNbElement1.value = _xr1.count;
  /* Calculate the fast MA. */
  const _xr2 = ma(tempInteger, endIdx, inReal, optInFastPeriod, (optInFastMAType as MAType), fastMABuffer, core);
  outBegIdx2.value = _xr2.begIdx;
  outNbElement2.value = _xr2.count;
  /* Parano tests. Will be removed eventually. */
  if (outBegIdx1.value !== tempInteger || outBegIdx2.value !== tempInteger || outNbElement1.value !== outNbElement2.value || outNbElement1.value !== endIdx - startIdx + 1 + lookbackSignal) {
    return RetCode.BAD_PARAM;
  }
  /* Calculate (fast MA) - (slow MA). */
  for (i = 0; i < outNbElement1.value; i += 1) {
    fastMABuffer[i] = fastMABuffer[i] - slowMABuffer[i];
  }
  /* Copy the result into the output for the caller. */
  /* memmove, not memcpy: fastMABuffer aliases outMACD when the caller buffer is
   * reused as scratch, so source and destination overlap (issue #94).
   */
  outMACD.set(fastMABuffer.subarray(lookbackSignal, lookbackSignal + (endIdx - startIdx + 1)), 0);
  /* Calculate the signal/trigger line. */
  const _xr3 = ma(0, outNbElement1.value - 1, fastMABuffer, optInSignalPeriod, (optInSignalMAType as MAType), outMACDSignal, core);
  outBegIdx2.value = _xr3.begIdx;
  outNbElement2.value = _xr3.count;
  /* Calculate the histogram. */
  for (i = 0; i < outNbElement2.value; i += 1) {
    outMACDHist[i] = outMACD[i] - outMACDSignal[i];
  }
  /* All done! Indicate the output limits and return success. */
  outBegIdx.value = startIdx;
  outNBElement.value = outNbElement2.value;
  return RetCode.SUCCESS;
}

/**
 * MACD variant where the fast, slow, and signal moving averages each use a
 * user-selectable MA type. Outputs the MACD line, its signal line, and their
 * difference (histogram). Hist sign change (MACD crossing its signal line) flags
 * momentum shifts.
 *
 * Formula and more info at
 * [ta-lib.org/functions/macdext](https://ta-lib.org/functions/macdext).
 *
 * **Notes**
 * - If the slow period is set smaller than the fast period, the fast and slow
 *   periods and their MA types are swapped so the slow moving average is always
 *   the longer one.
 * - A signal period of 1 disables signal-line smoothing for every signal MAType:
 *   the signal equals the MACD line and the histogram is zero.
 * - `TA_MAType_MAMA` ignores its period argument, so it always produces the same
 *   series regardless of the period requested. If both `optInFastMAType` and
 *   `optInSlowMAType` are set to MAMA, the fast and slow lines are therefore
 *   identical and MACD, Signal, and Hist are all zero at every bar. Select MAMA
 *   for only one side to get a meaningful spread.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link macdextLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - Source series.
 * @param optInFastPeriod - Period of the fast MA (default 12; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param optInFastMAType - MA type for the fast MA (default `MAType.SMA`; values:
 *   0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA,
 *   10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
 *   `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param optInSlowPeriod - Period of the slow MA (default 26; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowMAType - MA type for the slow MA (default `MAType.SMA`; values:
 *   0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA,
 *   10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
 *   `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param optInSignalPeriod - Period of the signal-line MA (default 9; range 1 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSignalMAType - MA type for the signal line (default `MAType.SMA`;
 *   values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3,
 *   9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
 *   `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param outMACD - MACD line: fast MA minus slow MA. Must hold the values produced
 *   and never be empty; the input length always suffices.
 * @param outMACDSignal - Signal line: MA of the MACD line. Must hold the values
 *   produced and never be empty; the input length always suffices.
 * @param outMACDHist - Histogram: MACD minus signal. Must hold the values produced
 *   and never be empty; the input length always suffices.
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
 * import { MAType, macdext } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const macd = new Float64Array(data.length);
 * const macdSignal = new Float64Array(data.length);
 * const macdHist = new Float64Array(data.length);
 * const { begIdx, count } = macdext(
 *   0, data.length - 1,
 *   data,
 *   12, MAType.SMA, 26, MAType.SMA, 9, MAType.SMA,
 *   macd, macdSignal, macdHist,
 * );
 * // macd[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`macd`](https://ta-lib.org/functions/macd)
 * @see [`macdfix`](https://ta-lib.org/functions/macdfix)
 * @see [`ma`](https://ta-lib.org/functions/ma)
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`apo`](https://ta-lib.org/functions/apo)
 * @see [`ppo`](https://ta-lib.org/functions/ppo)
 */
export function macdext(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInFastPeriod: number,
  optInFastMAType: MAType | typeof Core.INTEGER_DEFAULT,
  optInSlowPeriod: number,
  optInSlowMAType: MAType | typeof Core.INTEGER_DEFAULT,
  optInSignalPeriod: number,
  optInSignalMAType: MAType | typeof Core.INTEGER_DEFAULT,
  outMACD: Float64Array,
  outMACDSignal: Float64Array,
  outMACDHist: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('MACDEXT', 'startIdx', startIdx);
  endIdx = requireInt32('MACDEXT', 'endIdx', endIdx);
  optInFastPeriod = requireInt32('MACDEXT', 'optInFastPeriod', optInFastPeriod);
  optInFastMAType = requireInt32('MACDEXT', 'optInFastMAType', optInFastMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  optInSlowPeriod = requireInt32('MACDEXT', 'optInSlowPeriod', optInSlowPeriod);
  optInSlowMAType = requireInt32('MACDEXT', 'optInSlowMAType', optInSlowMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  optInSignalPeriod = requireInt32('MACDEXT', 'optInSignalPeriod', optInSignalPeriod);
  optInSignalMAType = requireInt32('MACDEXT', 'optInSignalMAType', optInSignalMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireIndexRange('MACDEXT', startIdx, endIdx);
  const guardStart = clampedStart('MACDEXT', startIdx, macdextLookback(optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('MACDEXT', 'inReal', inReal, guardInLen);
  requireOutput('MACDEXT', 'outMACD', outMACD, guardOutLen, Float64Array);
  requireOutput('MACDEXT', 'outMACDSignal', outMACDSignal, guardOutLen, Float64Array);
  requireOutput('MACDEXT', 'outMACDHist', outMACDHist, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outMACD, inRealF64) || overlapsPartly(outMACDSignal, inRealF64) || overlapsPartly(outMACDHist, inRealF64) || overlaps(outMACD, outMACDSignal) || overlaps(outMACD, outMACDHist) || overlaps(outMACDSignal, outMACDHist)) {
    throw failure('MACDEXT', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = macdextImpl(core, startIdx, endIdx, inRealF64, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('MACDEXT', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live MACDEXT stream: one value per closed bar, bit-identical to
 * {@link macdext} over the same series. Open it with {@link macdextOpen} or
 * {@link macdextOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/macdext](https://ta-lib.org/functions/macdext).
 */
export class MacdextStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInFastPeriod: number = 0;
  /** @internal */
  optInFastMAType: number = 0;
  /** @internal */
  optInSlowPeriod: number = 0;
  /** @internal */
  optInSlowMAType: number = 0;
  /** @internal */
  optInSignalPeriod: number = 0;
  /** @internal */
  optInSignalMAType: number = 0;
  /** @internal */
  cur_outMACD: number = 0;
  /** @internal */
  cur_outMACDSignal: number = 0;
  /** @internal */
  cur_outMACDHist: number = 0;
  /** @internal */
  sub0!: MaStream;
  /** @internal */
  sub1!: MaStream;
  /** @internal */
  sub2!: MaStream;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): MacdextStream {
    return new MacdextStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link macdext} reports them over the same bars. Each accepted `update` and each
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
      throw failure('MACDEXT advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar, writing the new values into `out`.
   *
   * @param inReal - The bar's `inReal` value: Source series.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number, out: MacdextOut): void {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('MACDEXT update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (typeof out !== 'object' || out === null) {
      throw badSink('MACDEXT update', 'MacdextOut');
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('MACDEXT update', 'inReal');
    }
    macdextStepImpl(this, inReal);
    this.outRangeCount++;
    out.macd = this.cur_outMACD;
    out.macdSignal = this.cur_outMACDSignal;
    out.macdHist = this.cur_outMACDHist;
  }

  /**
   * Writes into `out` what the next `update` with the same bar would write,
   * committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Source series.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number, out: MacdextOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('MACDEXT peek', 'MacdextOut');
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('MACDEXT peek', 'inReal');
    }
    const sp = this;
    let cur_slowMABuffer = 0;
    let cur_fastMABuffer = 0;
    let cur_outMACDSignal = 0;
    let cur_outMACDHist = 0;
    let cur_outMACD = 0;
    cur_slowMABuffer = sp.sub0.peek(inReal);
    cur_fastMABuffer = sp.sub1.peek(inReal);
    cur_fastMABuffer = cur_fastMABuffer - cur_slowMABuffer;
    cur_outMACDSignal = sp.sub2.peek(cur_fastMABuffer);
    cur_outMACDHist = cur_fastMABuffer - cur_outMACDSignal;
    cur_outMACD = cur_fastMABuffer;
    out.macd = cur_outMACD;
    out.macdSignal = cur_outMACDSignal;
    out.macdHist = cur_outMACDHist;
  }

  /**
   * The value at the last bar this stream counted; `peek` does not change it.
   *
   * @param out - Receives the outputs.
   */
  value(out: MacdextOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('MACDEXT value', 'MacdextOut');
    }
    out.macd = this.cur_outMACD;
    out.macdSignal = this.cur_outMACDSignal;
    out.macdHist = this.cur_outMACDHist;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): MacdextStream {
    const c = new MacdextStream(this.core);
    c.optInFastPeriod = this.optInFastPeriod;
    c.optInFastMAType = this.optInFastMAType;
    c.optInSlowPeriod = this.optInSlowPeriod;
    c.optInSlowMAType = this.optInSlowMAType;
    c.optInSignalPeriod = this.optInSignalPeriod;
    c.optInSignalMAType = this.optInSignalMAType;
    c.cur_outMACD = this.cur_outMACD;
    c.cur_outMACDSignal = this.cur_outMACDSignal;
    c.cur_outMACDHist = this.cur_outMACDHist;
    c.sub0 = this.sub0.clone();
    c.sub1 = this.sub1.clone();
    c.sub2 = this.sub2.clone();
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one MACDEXT bar, written into an object the caller owns and
 * reuses: valid until the next call that writes it.
 */
export class MacdextOut {
  /** MACD line: fast MA minus slow MA. */
  macd: number = 0;
  /** Signal line: MA of the MACD line. */
  macdSignal: number = 0;
  /** Histogram: MACD minus signal. */
  macdHist: number = 0;
}

function macdextStepImpl(sp: MacdextStream, inReal: number): void {
  let cur_slowMABuffer = 0;
  let cur_fastMABuffer = 0;
  let cur_outMACDSignal = 0;
  let cur_outMACDHist = 0;
  cur_slowMABuffer = sp.sub0.update(inReal);
  cur_fastMABuffer = sp.sub1.update(inReal);
  cur_fastMABuffer = cur_fastMABuffer - cur_slowMABuffer;
  cur_outMACDSignal = sp.sub2.update(cur_fastMABuffer);
  cur_outMACDHist = cur_fastMABuffer - cur_outMACDSignal;
  sp.cur_outMACD = cur_fastMABuffer;
  sp.cur_outMACDSignal = cur_outMACDSignal;
  sp.cur_outMACDHist = cur_outMACDHist;
}

function macdextOpenImpl(
  sp: MacdextStream,
  inReal: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInFastMAType: number,
  optInSlowPeriod: number,
  optInSlowMAType: number,
  optInSignalPeriod: number,
  optInSignalMAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outMACD: Float64Array,
  outMACDSignal: Float64Array,
  outMACDHist: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let slowMABuffer!: Float64Array;
  let fastMABuffer!: Float64Array;
  let tempInteger = 0;
  const outBegIdx1 = new MInteger();
  const outNbElement1 = new MInteger();
  const outBegIdx2 = new MInteger();
  const outNbElement2 = new MInteger();
  let lookbackTotal = 0;
  let lookbackSignal = 0;
  let lookbackLargest = 0;
  let i = 0;
  let tempMAType = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInFastPeriod === Core.INTEGER_DEFAULT) {
    optInFastPeriod = 12;
  } else if (optInFastPeriod < 2 || optInFastPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInFastMAType === Core.INTEGER_DEFAULT || optInFastMAType === MAType.DEFAULT) {
    optInFastMAType = MAType.SMA;
  } else if (optInFastMAType < MATypes.Min || optInFastMAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowPeriod === Core.INTEGER_DEFAULT) {
    optInSlowPeriod = 26;
  } else if (optInSlowPeriod < 2 || optInSlowPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowMAType === Core.INTEGER_DEFAULT || optInSlowMAType === MAType.DEFAULT) {
    optInSlowMAType = MAType.SMA;
  } else if (optInSlowMAType < MATypes.Min || optInSlowMAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if (optInSignalPeriod === Core.INTEGER_DEFAULT) {
    optInSignalPeriod = 9;
  } else if (optInSignalPeriod < 1 || optInSignalPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSignalMAType === Core.INTEGER_DEFAULT || optInSignalMAType === MAType.DEFAULT) {
    optInSignalMAType = MAType.SMA;
  } else if (optInSignalMAType < MATypes.Min || optInSignalMAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen < macdextLookback(optInFastPeriod, optInFastMAType as MAType, optInSlowPeriod, optInSlowMAType as MAType, optInSignalPeriod, optInSignalMAType as MAType, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const sc_outMACD = outStride === 1 ? outMACD : new Float64Array(historyLen);
  const sc_outMACDSignal = outStride === 1 ? outMACDSignal : new Float64Array(historyLen);
  const sc_outMACDHist = outStride === 1 ? outMACDHist : new Float64Array(historyLen);
  /* Make sure slow is really slower than
   * the fast period! if not, swap...
   */
  if (optInSlowPeriod < optInFastPeriod) {
    /* swap period */
    tempInteger = optInSlowPeriod;
    optInSlowPeriod = optInFastPeriod;
    optInFastPeriod = tempInteger;
    /* swap type */
    tempMAType = optInSlowMAType;
    optInSlowMAType = optInFastMAType;
    optInFastMAType = tempMAType;
  }
  /* Find the MA with the largest lookback */
  lookbackLargest = maLookback(optInFastPeriod, (optInFastMAType as MAType), core);
  tempInteger = maLookback(optInSlowPeriod, (optInSlowMAType as MAType), core);
  if (tempInteger > lookbackLargest) {
    lookbackLargest = tempInteger;
  }
  /* Add the lookback needed for the signal line */
  lookbackSignal = maLookback(optInSignalPeriod, (optInSignalMAType as MAType), core);
  lookbackTotal = lookbackSignal + lookbackLargest;
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
  /* Allocate intermediate buffer for fast/slow MA. */
  tempInteger = endIdx - startIdx + 1 + lookbackSignal;
  fastMABuffer = new Float64Array(tempInteger);
  slowMABuffer = new Float64Array(tempInteger);
  /* Calculate the slow MA.
   *
   * Move back the startIdx to get enough data
   * for the signal period. That way, once the
   * signal calculation is done, all the output
   * will start at the requested 'startIdx'.
   */
  tempInteger = startIdx - lookbackSignal;
  const sub0 = maOpenAndFillInternal(core, inReal, tempInteger, optInSlowPeriod, (optInSlowMAType as MAType), outBegIdx1, outNbElement1, slowMABuffer);
  /* Calculate the fast MA. */
  const sub1 = maOpenAndFillInternal(core, inReal, tempInteger, optInFastPeriod, (optInFastMAType as MAType), outBegIdx2, outNbElement2, fastMABuffer);
  /* Parano tests. Will be removed eventually. */
  if (outBegIdx1.value !== tempInteger || outBegIdx2.value !== tempInteger || outNbElement1.value !== outNbElement2.value || outNbElement1.value !== endIdx - startIdx + 1 + lookbackSignal) {
    return RetCode.BAD_PARAM;
  }
  /* Calculate (fast MA) - (slow MA). */
  for (i = 0; i < outNbElement1.value; i += 1) {
    fastMABuffer[i] = fastMABuffer[i] - slowMABuffer[i];
  }
  /* Copy the result into the output for the caller. */
  /* memmove, not memcpy: fastMABuffer aliases outMACD when the caller buffer is
   * reused as scratch, so source and destination overlap (issue #94).
   */
  sc_outMACD.set(fastMABuffer.subarray(lookbackSignal, lookbackSignal + (endIdx - startIdx + 1)), 0);
  /* Calculate the signal/trigger line. */
  const sub2 = maOpenAndFillInternal(core, fastMABuffer.slice(0, (outNbElement1.value - 1) + 1), 0, optInSignalPeriod, (optInSignalMAType as MAType), outBegIdx2, outNbElement2, sc_outMACDSignal);
  /* Calculate the histogram. */
  for (i = 0; i < outNbElement2.value; i += 1) {
    sc_outMACDHist[i] = sc_outMACD[i] - sc_outMACDSignal[i];
  }
  /* All done! Indicate the output limits and return success. */
  outBegIdx.value = startIdx;
  outNBElement.value = outNbElement2.value;
  if (outNBElement.value < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  sp.optInFastPeriod = optInFastPeriod;
  sp.optInFastMAType = optInFastMAType;
  sp.optInSlowPeriod = optInSlowPeriod;
  sp.optInSlowMAType = optInSlowMAType;
  sp.optInSignalPeriod = optInSignalPeriod;
  sp.optInSignalMAType = optInSignalMAType;
  sp.sub0 = sub0;
  sp.sub1 = sub1;
  sp.sub2 = sub2;
  sp.cur_outMACD = sc_outMACD[outNBElement.value - 1];
  sp.cur_outMACDSignal = sc_outMACDSignal[outNBElement.value - 1];
  sp.cur_outMACDHist = sc_outMACDHist[outNBElement.value - 1];
  return RetCode.SUCCESS;
}

/** @internal */
export function macdextOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInFastMAType: number,
  optInSlowPeriod: number,
  optInSlowMAType: number,
  optInSignalPeriod: number,
  optInSignalMAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outMACD: Float64Array,
  outMACDSignal: Float64Array,
  outMACDHist: Float64Array,
): MacdextStream {
  const sp = MacdextStream.of(core);
  const retCode = macdextOpenImpl(sp, inReal, startIdx, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MACDEXT openAndFill', inReal.length, startIdx, macdextLookback(optInFastPeriod, optInFastMAType as MAType, optInSlowPeriod, optInSlowMAType as MAType, optInSignalPeriod, optInSignalMAType as MAType, core));
  }
  throw streamFailure('MACDEXT openAndFill', retCode);
}

/** @internal */
export function macdextOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInFastMAType: number,
  optInSlowPeriod: number,
  optInSlowMAType: number,
  optInSignalPeriod: number,
  optInSignalMAType: number,
): MacdextStream {
  const sp = MacdextStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outMACD = new Float64Array(1);
  const sink_outMACDSignal = new Float64Array(1);
  const sink_outMACDHist = new Float64Array(1);
  const retCode = macdextOpenImpl(sp, inReal, startIdx, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType, outBegIdx, outNBElement, sink_outMACD, sink_outMACDSignal, sink_outMACDHist, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MACDEXT open', inReal.length, startIdx, macdextLookback(optInFastPeriod, optInFastMAType as MAType, optInSlowPeriod, optInSlowMAType as MAType, optInSignalPeriod, optInSignalMAType as MAType, core));
  }
  throw streamFailure('MACDEXT open', retCode);
}

/**
 * Opens a live MACDEXT stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link macdext} there.
 *
 * @param inReal - Warm-up history. Source series.
 * @param optInFastPeriod - Period of the fast MA (default 12; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param optInFastMAType - MA type for the fast MA (default `MAType.SMA`; values:
 *   0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA,
 *   10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
 *   `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param optInSlowPeriod - Period of the slow MA (default 26; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowMAType - MA type for the slow MA (default `MAType.SMA`; values:
 *   0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA,
 *   10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
 *   `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param optInSignalPeriod - Period of the signal-line MA (default 9; range 1 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSignalMAType - MA type for the signal line (default `MAType.SMA`;
 *   values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3,
 *   9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
 *   `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `macdextLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { MAType, MacdextOut, macdextOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = macdextOpen(data, 12, MAType.SMA, 26, MAType.SMA, 9, MAType.SMA);
 * const peeked = new MacdextOut();
 * const updated = new MacdextOut();
 * stream.peek(100.9, peeked); // a forming bar: commits nothing
 * stream.update(100.9, updated); // the bar closes
 * ```
 */
export function macdextOpen(
  inReal: ArrayLike<number>,
  optInFastPeriod: number,
  optInFastMAType: MAType | typeof Core.INTEGER_DEFAULT,
  optInSlowPeriod: number,
  optInSlowMAType: MAType | typeof Core.INTEGER_DEFAULT,
  optInSignalPeriod: number,
  optInSignalMAType: MAType | typeof Core.INTEGER_DEFAULT,
  core: Core = Core.DEFAULT,
): MacdextStream {
  optInFastPeriod = requireInt32('MACDEXT open', 'optInFastPeriod', optInFastPeriod);
  optInFastMAType = requireInt32('MACDEXT open', 'optInFastMAType', optInFastMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  optInSlowPeriod = requireInt32('MACDEXT open', 'optInSlowPeriod', optInSlowPeriod);
  optInSlowMAType = requireInt32('MACDEXT open', 'optInSlowMAType', optInSlowMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  optInSignalPeriod = requireInt32('MACDEXT open', 'optInSignalPeriod', optInSignalPeriod);
  optInSignalMAType = requireInt32('MACDEXT open', 'optInSignalMAType', optInSignalMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('MACDEXT open', 'inReal', inReal);
  requireHistory('MACDEXT open', inReal.length);
  return macdextOpenInternal(core, float64(inReal), 0, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType);
}

/**
 * {@link macdextOpen} that also fills the outputs over the whole history in the
 * same pass, bit-identically to {@link macdext}. The range written is the stream's
 * `outRange()`.
 *
 * @param inReal - Warm-up history. Source series.
 * @param optInFastPeriod - Period of the fast MA (default 12; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param optInFastMAType - MA type for the fast MA (default `MAType.SMA`; values:
 *   0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA,
 *   10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
 *   `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param optInSlowPeriod - Period of the slow MA (default 26; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowMAType - MA type for the slow MA (default `MAType.SMA`; values:
 *   0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA,
 *   10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
 *   `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param optInSignalPeriod - Period of the signal-line MA (default 9; range 1 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSignalMAType - MA type for the signal line (default `MAType.SMA`;
 *   values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3,
 *   9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
 *   `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param outMACD - MACD line: fast MA minus slow MA. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param outMACDSignal - Signal line: MA of the MACD line. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param outMACDHist - Histogram: MACD minus signal. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `macdextLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for an integer parameter that is not an int32, a parameter outside its range or
 *   an output too short or starting where another output or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { MAType, macdextOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const macd = new Float64Array(data.length);
 * const macdSignal = new Float64Array(data.length);
 * const macdHist = new Float64Array(data.length);
 * const stream = macdextOpenAndFill(
 *   data,
 *   12, MAType.SMA, 26, MAType.SMA, 9, MAType.SMA,
 *   macd, macdSignal, macdHist,
 * );
 * const { begIdx, count } = stream.outRange();
 * // macd[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function macdextOpenAndFill(
  inReal: ArrayLike<number>,
  optInFastPeriod: number,
  optInFastMAType: MAType | typeof Core.INTEGER_DEFAULT,
  optInSlowPeriod: number,
  optInSlowMAType: MAType | typeof Core.INTEGER_DEFAULT,
  optInSignalPeriod: number,
  optInSignalMAType: MAType | typeof Core.INTEGER_DEFAULT,
  outMACD: Float64Array,
  outMACDSignal: Float64Array,
  outMACDHist: Float64Array,
  core: Core = Core.DEFAULT,
): MacdextStream {
  optInFastPeriod = requireInt32('MACDEXT openAndFill', 'optInFastPeriod', optInFastPeriod);
  optInFastMAType = requireInt32('MACDEXT openAndFill', 'optInFastMAType', optInFastMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  optInSlowPeriod = requireInt32('MACDEXT openAndFill', 'optInSlowPeriod', optInSlowPeriod);
  optInSlowMAType = requireInt32('MACDEXT openAndFill', 'optInSlowMAType', optInSlowMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  optInSignalPeriod = requireInt32('MACDEXT openAndFill', 'optInSignalPeriod', optInSignalPeriod);
  optInSignalMAType = requireInt32('MACDEXT openAndFill', 'optInSignalMAType', optInSignalMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('MACDEXT openAndFill', 'inReal', inReal);
  requireHistory('MACDEXT openAndFill', inReal.length);
  const guardOutLen = openFillCount('MACDEXT openAndFill', inReal.length, macdextLookback(optInFastPeriod, optInFastMAType as MAType, optInSlowPeriod, optInSlowMAType as MAType, optInSignalPeriod, optInSignalMAType as MAType, core));
  requireOutput('MACDEXT openAndFill', 'outMACD', outMACD, guardOutLen, Float64Array);
  requireOutput('MACDEXT openAndFill', 'outMACDSignal', outMACDSignal, guardOutLen, Float64Array);
  requireOutput('MACDEXT openAndFill', 'outMACDHist', outMACDHist, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if ((outMACD.buffer === inRealF64.buffer && outMACD.byteOffset === inRealF64.byteOffset) || (outMACDSignal.buffer === inRealF64.buffer && outMACDSignal.byteOffset === inRealF64.byteOffset) || (outMACDHist.buffer === inRealF64.buffer && outMACDHist.byteOffset === inRealF64.byteOffset) || (outMACD.buffer === outMACDSignal.buffer && outMACD.byteOffset === outMACDSignal.byteOffset) || (outMACD.buffer === outMACDHist.buffer && outMACD.byteOffset === outMACDHist.byteOffset) || (outMACDSignal.buffer === outMACDHist.buffer && outMACDSignal.byteOffset === outMACDHist.byteOffset)) {
    throw streamFailure('MACDEXT openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outMACD, inRealF64) || overlaps(outMACDSignal, inRealF64) || overlaps(outMACDHist, inRealF64) || overlaps(outMACD, outMACDSignal) || overlaps(outMACD, outMACDHist) || overlaps(outMACDSignal, outMACDHist))) {
    throw streamFailure('MACDEXT openAndFill', RetCode.BAD_PARAM);
  }
  return macdextOpenAndFillInternal(core, inRealF64, 0, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType, new MInteger(), new MInteger(), outMACD, outMACDSignal, outMACDHist);
}
