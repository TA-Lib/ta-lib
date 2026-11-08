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
import { badSink, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { MaStream, maLookbackPositional, maOpenAndFillInternal, maPositional } from './ma.ts';
import { macdPositional } from './macd.ts';

/** @internal */
export function macdextLookbackPositional(optInFastPeriod: number, optInFastMAType: MAType | typeof Core.INTEGER_DEFAULT, optInSlowPeriod: number, optInSlowMAType: MAType | typeof Core.INTEGER_DEFAULT, optInSignalPeriod: number, optInSignalMAType: MAType | typeof Core.INTEGER_DEFAULT, core: Core = Core.DEFAULT): number {
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
  lookbackLargest = maLookbackPositional(optInFastPeriod, (optInFastMAType as MAType), core);
  tempInteger = maLookbackPositional(optInSlowPeriod, (optInSlowMAType as MAType), core);
  if (tempInteger > lookbackLargest) {
    lookbackLargest = tempInteger;
  }
  /* Add to the largest MA lookback the signal line lookback */
  return lookbackLargest + maLookbackPositional(optInSignalPeriod, (optInSignalMAType as MAType), core);
}

/** @internal */
export function macdextDisplayShiftPositional(optInFastPeriod: number, optInFastMAType: MAType | typeof Core.INTEGER_DEFAULT, optInSlowPeriod: number, optInSlowMAType: MAType | typeof Core.INTEGER_DEFAULT, optInSignalPeriod: number, optInSignalMAType: MAType | typeof Core.INTEGER_DEFAULT, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (macdextLookbackPositional(optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType, core) < 0) {
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
    const _xr0 = macdPositional(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outMACD, outMACDSignal, outMACDHist, core);
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
  lookbackLargest = maLookbackPositional(optInFastPeriod, (optInFastMAType as MAType), core);
  tempInteger = maLookbackPositional(optInSlowPeriod, (optInSlowMAType as MAType), core);
  if (tempInteger > lookbackLargest) {
    lookbackLargest = tempInteger;
  }
  /* Add the lookback needed for the signal line */
  lookbackSignal = maLookbackPositional(optInSignalPeriod, (optInSignalMAType as MAType), core);
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
  const _xr1 = maPositional(tempInteger, endIdx, inReal, optInSlowPeriod, (optInSlowMAType as MAType), slowMABuffer, core);
  outBegIdx1.value = _xr1.begIdx;
  outNbElement1.value = _xr1.count;
  /* Calculate the fast MA. */
  const _xr2 = maPositional(tempInteger, endIdx, inReal, optInFastPeriod, (optInFastMAType as MAType), fastMABuffer, core);
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
  const _xr3 = maPositional(0, outNbElement1.value - 1, fastMABuffer, optInSignalPeriod, (optInSignalMAType as MAType), outMACDSignal, core);
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

const PARAM_RANGES = [['fastPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['fastMaType', Core.INTEGER_DEFAULT, MATypes.Min, MATypes.Max, 'a MAType'], ['slowPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['slowMaType', Core.INTEGER_DEFAULT, MATypes.Min, MATypes.Max, 'a MAType'], ['signalPeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['signalMaType', Core.INTEGER_DEFAULT, MATypes.Min, MATypes.Max, 'a MAType']] as const;

/** @internal */
export function macdextPositional(
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
  optInFastPeriod = requireInt32('MACDEXT', 'fastPeriod', optInFastPeriod);
  optInFastMAType = requireInt32('MACDEXT', 'fastMaType', optInFastMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  optInSlowPeriod = requireInt32('MACDEXT', 'slowPeriod', optInSlowPeriod);
  optInSlowMAType = requireInt32('MACDEXT', 'slowMaType', optInSlowMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  optInSignalPeriod = requireInt32('MACDEXT', 'signalPeriod', optInSignalPeriod);
  optInSignalMAType = requireInt32('MACDEXT', 'signalMaType', optInSignalMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireIndexRange('MACDEXT', startIdx, endIdx);
  const guardLookback = macdextLookbackPositional(optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType, core);
  if (guardLookback < 0) {
    throw rejectedParams('MACDEXT', PARAM_RANGES, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType);
  }
  const guardStart = clampedStart('MACDEXT', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('MACDEXT', 'real', inReal, guardInLen);
  requireOutput('MACDEXT', 'outMacd', outMACD, guardOutLen, Float64Array);
  requireOutput('MACDEXT', 'outMacdSignal', outMACDSignal, guardOutLen, Float64Array);
  requireOutput('MACDEXT', 'outMacdHist', outMACDHist, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outMACD, inRealF64)) {
    throw sharedMemory('MACDEXT', 'outMacd', 'real');
  }
  if (overlapsPartly(outMACDSignal, inRealF64)) {
    throw sharedMemory('MACDEXT', 'outMacdSignal', 'real');
  }
  if (overlapsPartly(outMACDHist, inRealF64)) {
    throw sharedMemory('MACDEXT', 'outMacdHist', 'real');
  }
  if (overlaps(outMACD, outMACDSignal)) {
    throw sharedMemory('MACDEXT', 'outMacd', 'outMacdSignal');
  }
  if (overlaps(outMACD, outMACDHist)) {
    throw sharedMemory('MACDEXT', 'outMacd', 'outMacdHist');
  }
  if (overlaps(outMACDSignal, outMACDHist)) {
    throw sharedMemory('MACDEXT', 'outMacdSignal', 'outMacdHist');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = macdextImpl(core, startIdx, endIdx, inRealF64, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('MACDEXT', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link macdext}, {@link macdextLookback},
 * {@link macdextDisplayShift}, {@link macdextOpen} and {@link macdextOpenAndFill}.
 * Every key is optional: an omitted or `undefined` key takes its default, and a
 * call ignores the keys it has no use for.
 */
export interface MacdextOptions {
  /**
   * Period of the fast MA (default 12; range 2 to 100000; `Core.INTEGER_DEFAULT`
   * selects the default).
   */
  fastPeriod?: number | undefined;
  /**
   * MA type for the fast MA (default `MAType.SMA`; values: 0=SMA, 1=EMA, 2=WMA,
   * 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT,
   * 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT`
   * selects the default).
   */
  fastMaType?: MAType | typeof Core.INTEGER_DEFAULT | undefined;
  /**
   * Period of the slow MA (default 26; range 2 to 100000; `Core.INTEGER_DEFAULT`
   * selects the default).
   */
  slowPeriod?: number | undefined;
  /**
   * MA type for the slow MA (default `MAType.SMA`; values: 0=SMA, 1=EMA, 2=WMA,
   * 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT,
   * 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT`
   * selects the default).
   */
  slowMaType?: MAType | typeof Core.INTEGER_DEFAULT | undefined;
  /**
   * Period of the signal-line MA (default 9; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  signalPeriod?: number | undefined;
  /**
   * MA type for the signal line (default `MAType.SMA`; values: 0=SMA, 1=EMA, 2=WMA,
   * 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT,
   * 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT`
   * selects the default).
   */
  signalMaType?: MAType | typeof Core.INTEGER_DEFAULT | undefined;
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
   * Buffer for `outMacd`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outMacd?: Float64Array<ArrayBuffer> | undefined;
  /**
   * Buffer for `outMacdSignal`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outMacdSignal?: Float64Array<ArrayBuffer> | undefined;
  /**
   * Buffer for `outMacdHist`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outMacdHist?: Float64Array<ArrayBuffer> | undefined;
}

/** What {@link macdext} returns, and {@link macdextOpenAndFill} with its stream. */
export interface MacdextResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * MACD line: fast MA minus slow MA. `outMacd[k]` is the value at input bar
   * `begIdx + k`.
   */
  outMacd: Float64Array<ArrayBuffer>;
  /**
   * Signal line: MA of the MACD line. `outMacdSignal[k]` is the value at input bar
   * `begIdx + k`.
   */
  outMacdSignal: Float64Array<ArrayBuffer>;
  /**
   * Histogram: MACD minus signal. `outMacdHist[k]` is the value at input bar
   * `begIdx + k`.
   */
  outMacdHist: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link macdext} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MacdextOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function macdextLookback(options?: MacdextOptions): number {
  const o = optionsOf('MACDEXT lookback', options);
  return macdextLookbackPositional(orDefault(o.fastPeriod, Core.INTEGER_DEFAULT), orDefault(o.fastMaType, Core.INTEGER_DEFAULT), orDefault(o.slowPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowMaType, Core.INTEGER_DEFAULT), orDefault(o.signalPeriod, Core.INTEGER_DEFAULT), orDefault(o.signalMaType, Core.INTEGER_DEFAULT), coreOf('MACDEXT lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link macdext}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MacdextOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function macdextDisplayShift(outputIdx: number, options?: MacdextOptions): number {
  const o = optionsOf('MACDEXT displayShift', options);
  return macdextDisplayShiftPositional(orDefault(o.fastPeriod, Core.INTEGER_DEFAULT), orDefault(o.fastMaType, Core.INTEGER_DEFAULT), orDefault(o.slowPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowMaType, Core.INTEGER_DEFAULT), orDefault(o.signalPeriod, Core.INTEGER_DEFAULT), orDefault(o.signalMaType, Core.INTEGER_DEFAULT), outputIdx, coreOf('MACDEXT displayShift', o.core));
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
 * - `MAType.MAMA` ignores its period argument, so it always produces the same
 *   series regardless of the period requested. If both `fastMaType` and
 *   `slowMaType` are set to MAMA, the fast and slow lines are therefore identical
 *   and MACD, Signal, and Hist are all zero at every bar. Select MAMA for only one
 *   side to get a meaningful spread.
 *
 * `outMacd[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link macdextLookback} is a success with empty outputs.
 *
 * @param real - Source series.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link MacdextOptions}.
 * @returns `begIdx` and the values; see {@link MacdextResult}. Each output is a
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
 * import { MAType, macdext } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const {
 *   begIdx,
 *   outMacd,
 *   outMacdSignal,
 *   outMacdHist,
 * } = macdext(data, {
 *   fastPeriod: 12,
 *   fastMaType: MAType.SMA,
 *   slowPeriod: 26,
 *   slowMaType: MAType.SMA,
 *   signalPeriod: 9,
 *   signalMaType: MAType.SMA,
 * });
 * // outMacd[k] is the value at bar begIdx + k
 * ```
 * @see [`macd`](https://ta-lib.org/functions/macd)
 * @see [`macdfix`](https://ta-lib.org/functions/macdfix)
 * @see [`ma`](https://ta-lib.org/functions/ma)
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`apo`](https://ta-lib.org/functions/apo)
 * @see [`ppo`](https://ta-lib.org/functions/ppo)
 */
export function macdext(real: ArrayLike<number>, options?: MacdextOptions): MacdextResult {
  const o = optionsOf('MACDEXT', options);
  if (o.endIdx === undefined) {
    requireSeries('MACDEXT', 'real', real);
  }
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInFastMAType = orDefault(o.fastMaType, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const optInSlowMAType = orDefault(o.slowMaType, Core.INTEGER_DEFAULT);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const optInSignalMAType = orDefault(o.signalMaType, Core.INTEGER_DEFAULT);
  const core = coreOf('MACDEXT', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, macdextLookbackPositional(optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType, core), real);
  const outMACD = o.outMacd === undefined ? new Float64Array(capacity) : o.outMacd;
  const outMACDSignal = o.outMacdSignal === undefined ? new Float64Array(capacity) : o.outMacdSignal;
  const outMACDHist = o.outMacdHist === undefined ? new Float64Array(capacity) : o.outMacdHist;
  const range = macdextPositional(startIdx, endIdx, real, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType, outMACD, outMACDSignal, outMACDHist, core);
  return { begIdx: range.begIdx, outMacd: outMACD.subarray(0, range.count), outMacdSignal: outMACDSignal.subarray(0, range.count), outMacdHist: outMACDHist.subarray(0, range.count) };
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
      throw streamFailure('MACDEXT advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns its outputs.
   *
   * @param real - The bar's `real` value: Source series.
   * @param out - Receives the outputs and is returned; a new {@link MacdextOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included, before anything moves.
   *   `TALibError` with code `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached
   *   bar `Core.INDEX_MAX`.
   */
  update(real: number, out?: MacdextOut): MacdextOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('MACDEXT update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('MACDEXT update', 'MacdextOut');
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('MACDEXT update', 'real');
    }
    macdextStepImpl(this, real);
    this.outRangeCount++;
    if (out === undefined) {
      out = new MacdextOut();
    }
    out.outMacd = this.cur_outMACD;
    out.outMacdSignal = this.cur_outMACDSignal;
    out.outMacdHist = this.cur_outMACDHist;
    return out;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: Source series.
   * @param out - Receives the outputs and is returned; a new {@link MacdextOut} when
   *   omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included.
   */
  peek(real: number, out?: MacdextOut): MacdextOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('MACDEXT peek', 'MacdextOut');
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('MACDEXT peek', 'real');
    }
    const sp = this;
    const inReal = real;
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
    if (out === undefined) {
      out = new MacdextOut();
    }
    out.outMacd = cur_outMACD;
    out.outMacdSignal = cur_outMACDSignal;
    out.outMacdHist = cur_outMACDHist;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link MacdextOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a sink that is not an object,
   *   `null` included.
   */
  value(out?: MacdextOut): MacdextOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('MACDEXT value', 'MacdextOut');
    }
    if (out === undefined) {
      out = new MacdextOut();
    }
    out.outMacd = this.cur_outMACD;
    out.outMacdSignal = this.cur_outMACDSignal;
    out.outMacdHist = this.cur_outMACDHist;
    return out;
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
 * The outputs of one MACDEXT bar, as `update`, `peek` and `value` return them.
 * Passed back to one of them, it is filled and returned again, with no allocation;
 * it holds its values until the next call that writes it. A new one holds `NaN`.
 */
export class MacdextOut {
  /** MACD line: fast MA minus slow MA. */
  outMacd: number = NaN;
  /** Signal line: MA of the MACD line. */
  outMacdSignal: number = NaN;
  /** Histogram: MACD minus signal. */
  outMacdHist: number = NaN;
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
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen < macdextLookbackPositional(optInFastPeriod, optInFastMAType as MAType, optInSlowPeriod, optInSlowMAType as MAType, optInSignalPeriod, optInSignalMAType as MAType, core) + 1) {
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
  lookbackLargest = maLookbackPositional(optInFastPeriod, (optInFastMAType as MAType), core);
  tempInteger = maLookbackPositional(optInSlowPeriod, (optInSlowMAType as MAType), core);
  if (tempInteger > lookbackLargest) {
    lookbackLargest = tempInteger;
  }
  /* Add the lookback needed for the signal line */
  lookbackSignal = maLookbackPositional(optInSignalPeriod, (optInSignalMAType as MAType), core);
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
    throw insufficientHistory('MACDEXT openAndFill', inReal.length, startIdx, macdextLookbackPositional(optInFastPeriod, optInFastMAType as MAType, optInSlowPeriod, optInSlowMAType as MAType, optInSignalPeriod, optInSignalMAType as MAType, core));
  }
  if (retCode === RetCode.BAD_PARAM && macdextLookbackPositional(optInFastPeriod, optInFastMAType as MAType, optInSlowPeriod, optInSlowMAType as MAType, optInSignalPeriod, optInSignalMAType as MAType, core) < 0) {
    throw rejectedParams('MACDEXT openAndFill', PARAM_RANGES, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType);
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
    throw insufficientHistory('MACDEXT open', inReal.length, startIdx, macdextLookbackPositional(optInFastPeriod, optInFastMAType as MAType, optInSlowPeriod, optInSlowMAType as MAType, optInSignalPeriod, optInSignalMAType as MAType, core));
  }
  if (retCode === RetCode.BAD_PARAM && macdextLookbackPositional(optInFastPeriod, optInFastMAType as MAType, optInSlowPeriod, optInSlowMAType as MAType, optInSignalPeriod, optInSignalMAType as MAType, core) < 0) {
    throw rejectedParams('MACDEXT open', PARAM_RANGES, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType);
  }
  throw streamFailure('MACDEXT open', retCode);
}

/** @internal */
export function macdextOpenPositional(
  inReal: ArrayLike<number>,
  optInFastPeriod: number,
  optInFastMAType: MAType | typeof Core.INTEGER_DEFAULT,
  optInSlowPeriod: number,
  optInSlowMAType: MAType | typeof Core.INTEGER_DEFAULT,
  optInSignalPeriod: number,
  optInSignalMAType: MAType | typeof Core.INTEGER_DEFAULT,
  core: Core = Core.DEFAULT,
): MacdextStream {
  optInFastPeriod = requireInt32('MACDEXT open', 'fastPeriod', optInFastPeriod);
  optInFastMAType = requireInt32('MACDEXT open', 'fastMaType', optInFastMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  optInSlowPeriod = requireInt32('MACDEXT open', 'slowPeriod', optInSlowPeriod);
  optInSlowMAType = requireInt32('MACDEXT open', 'slowMaType', optInSlowMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  optInSignalPeriod = requireInt32('MACDEXT open', 'signalPeriod', optInSignalPeriod);
  optInSignalMAType = requireInt32('MACDEXT open', 'signalMaType', optInSignalMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('MACDEXT open', 'real', inReal);
  requireHistory('MACDEXT open', 'real', inReal.length);
  return macdextOpenInternal(core, float64(inReal), 0, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType);
}

/** @internal */
export function macdextOpenAndFillPositional(
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
  optInFastPeriod = requireInt32('MACDEXT openAndFill', 'fastPeriod', optInFastPeriod);
  optInFastMAType = requireInt32('MACDEXT openAndFill', 'fastMaType', optInFastMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  optInSlowPeriod = requireInt32('MACDEXT openAndFill', 'slowPeriod', optInSlowPeriod);
  optInSlowMAType = requireInt32('MACDEXT openAndFill', 'slowMaType', optInSlowMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  optInSignalPeriod = requireInt32('MACDEXT openAndFill', 'signalPeriod', optInSignalPeriod);
  optInSignalMAType = requireInt32('MACDEXT openAndFill', 'signalMaType', optInSignalMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('MACDEXT openAndFill', 'real', inReal);
  requireHistory('MACDEXT openAndFill', 'real', inReal.length);
  const guardLookback = macdextLookbackPositional(optInFastPeriod, optInFastMAType as MAType, optInSlowPeriod, optInSlowMAType as MAType, optInSignalPeriod, optInSignalMAType as MAType, core);
  if (guardLookback < 0) {
    throw rejectedParams('MACDEXT openAndFill', PARAM_RANGES, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType);
  }
  const guardOutLen = openFillCount('MACDEXT openAndFill', inReal.length, guardLookback);
  requireOutput('MACDEXT openAndFill', 'outMacd', outMACD, guardOutLen, Float64Array);
  requireOutput('MACDEXT openAndFill', 'outMacdSignal', outMACDSignal, guardOutLen, Float64Array);
  requireOutput('MACDEXT openAndFill', 'outMacdHist', outMACDHist, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outMACD.buffer === inRealF64.buffer && outMACD.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('MACDEXT openAndFill', 'outMacd', 'real');
  }
  if (outMACDSignal.buffer === inRealF64.buffer && outMACDSignal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('MACDEXT openAndFill', 'outMacdSignal', 'real');
  }
  if (outMACDHist.buffer === inRealF64.buffer && outMACDHist.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('MACDEXT openAndFill', 'outMacdHist', 'real');
  }
  if (outMACD.buffer === outMACDSignal.buffer && outMACD.byteOffset === outMACDSignal.byteOffset) {
    throw sharedMemory('MACDEXT openAndFill', 'outMacd', 'outMacdSignal');
  }
  if (outMACD.buffer === outMACDHist.buffer && outMACD.byteOffset === outMACDHist.byteOffset) {
    throw sharedMemory('MACDEXT openAndFill', 'outMacd', 'outMacdHist');
  }
  if (outMACDSignal.buffer === outMACDHist.buffer && outMACDSignal.byteOffset === outMACDHist.byteOffset) {
    throw sharedMemory('MACDEXT openAndFill', 'outMacdSignal', 'outMacdHist');
  }
  if (guardOutLen > 0 && overlaps(outMACD, inRealF64)) {
    throw sharedMemory('MACDEXT openAndFill', 'outMacd', 'real');
  }
  if (guardOutLen > 0 && overlaps(outMACDSignal, inRealF64)) {
    throw sharedMemory('MACDEXT openAndFill', 'outMacdSignal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outMACDHist, inRealF64)) {
    throw sharedMemory('MACDEXT openAndFill', 'outMacdHist', 'real');
  }
  if (guardOutLen > 0 && overlaps(outMACD, outMACDSignal)) {
    throw sharedMemory('MACDEXT openAndFill', 'outMacd', 'outMacdSignal');
  }
  if (guardOutLen > 0 && overlaps(outMACD, outMACDHist)) {
    throw sharedMemory('MACDEXT openAndFill', 'outMacd', 'outMacdHist');
  }
  if (guardOutLen > 0 && overlaps(outMACDSignal, outMACDHist)) {
    throw sharedMemory('MACDEXT openAndFill', 'outMacdSignal', 'outMacdHist');
  }
  return macdextOpenAndFillInternal(core, inRealF64, 0, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType, new MInteger(), new MInteger(), outMACD, outMACDSignal, outMACDHist);
}

/**
 * Opens a live MACDEXT stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link macdext} there.
 *
 * @param real - Warm-up history. Source series.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MacdextOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `macdextLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32 or a parameter outside its
 *   range.
 *
 * @example
 * ```ts
 * import { MAType, macdextOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = macdextOpen(data, {
 *   fastPeriod: 12,
 *   fastMaType: MAType.SMA,
 *   slowPeriod: 26,
 *   slowMaType: MAType.SMA,
 *   signalPeriod: 9,
 *   signalMaType: MAType.SMA,
 * });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function macdextOpen(real: ArrayLike<number>, options?: MacdextOptions): MacdextStream {
  const o = optionsOf('MACDEXT open', options);
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInFastMAType = orDefault(o.fastMaType, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const optInSlowMAType = orDefault(o.slowMaType, Core.INTEGER_DEFAULT);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const optInSignalMAType = orDefault(o.signalMaType, Core.INTEGER_DEFAULT);
  const core = coreOf('MACDEXT open', o.core);
  return macdextOpenPositional(real, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType, core);
}

/**
 * {@link macdextOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link macdext}.
 *
 * @param real - Warm-up history. Source series.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link MacdextOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link macdext} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `macdextLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range
 *   or a buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { MAType, macdextOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const {
 *   stream,
 *   begIdx,
 *   outMacd,
 *   outMacdSignal,
 *   outMacdHist,
 * } = macdextOpenAndFill(data, {
 *   fastPeriod: 12,
 *   fastMaType: MAType.SMA,
 *   slowPeriod: 26,
 *   slowMaType: MAType.SMA,
 *   signalPeriod: 9,
 *   signalMaType: MAType.SMA,
 * });
 * // outMacd[k] is the value at bar begIdx + k
 * ```
 */
export function macdextOpenAndFill(real: ArrayLike<number>, options?: MacdextOptions): MacdextResult & { stream: MacdextStream } {
  const o = optionsOf('MACDEXT openAndFill', options);
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInFastMAType = orDefault(o.fastMaType, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const optInSlowMAType = orDefault(o.slowMaType, Core.INTEGER_DEFAULT);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const optInSignalMAType = orDefault(o.signalMaType, Core.INTEGER_DEFAULT);
  const core = coreOf('MACDEXT openAndFill', o.core);
  const capacity = fillCapacity(macdextLookbackPositional(optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType, core), real);
  const outMACD = o.outMacd === undefined ? new Float64Array(capacity) : o.outMacd;
  const outMACDSignal = o.outMacdSignal === undefined ? new Float64Array(capacity) : o.outMacdSignal;
  const outMACDHist = o.outMacdHist === undefined ? new Float64Array(capacity) : o.outMacdHist;
  const stream = macdextOpenAndFillPositional(real, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType, outMACD, outMACDSignal, outMACDHist, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outMacd: outMACD.subarray(0, range.count), outMacdSignal: outMACDSignal.subarray(0, range.count), outMacdHist: outMACDHist.subarray(0, range.count) };
}
