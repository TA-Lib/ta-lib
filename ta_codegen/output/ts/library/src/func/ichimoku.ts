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
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  100726 KL,CC  Creation (#490).
 *  101126 MF,CC  Two scratch buffers; Senkou B is written in place (#490).
 */

import { badSink, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { MedpriceStream, medpriceOpenAndFillInternal, medpricePositional } from './medprice.ts';
import { MidpriceStream, midpriceOpenAndFillInternal, midpricePositional } from './midprice.ts';

/** @internal */
export function ichimokuLookbackPositional(optInTenkanPeriod: number, optInKijunPeriod: number, optInSenkouBPeriod: number, _core: Core = Core.DEFAULT): number {
  if ((optInTenkanPeriod | 0) !== optInTenkanPeriod) {
    return -1;
  }
  if ((optInKijunPeriod | 0) !== optInKijunPeriod) {
    return -1;
  }
  if ((optInSenkouBPeriod | 0) !== optInSenkouBPeriod) {
    return -1;
  }
  if (optInTenkanPeriod === Core.INTEGER_DEFAULT) {
    optInTenkanPeriod = 9;
  } else if (optInTenkanPeriod < 2 || optInTenkanPeriod > 100000) {
    return -1;
  }
  if (optInKijunPeriod === Core.INTEGER_DEFAULT) {
    optInKijunPeriod = 26;
  } else if (optInKijunPeriod < 2 || optInKijunPeriod > 100000) {
    return -1;
  }
  if (optInSenkouBPeriod === Core.INTEGER_DEFAULT) {
    optInSenkouBPeriod = 52;
  } else if (optInSenkouBPeriod < 2 || optInSenkouBPeriod > 100000) {
    return -1;
  }
  let longest = 0;
  /* Nothing orders the three periods: any of them can be the longest. */
  longest = optInTenkanPeriod;
  if (optInKijunPeriod > longest) {
    longest = optInKijunPeriod;
  }
  if (optInSenkouBPeriod > longest) {
    longest = optInSenkouBPeriod;
  }
  return longest - 1;
}

/** @internal */
export function ichimokuDisplayShiftPositional(optInTenkanPeriod: number, optInKijunPeriod: number, optInSenkouBPeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (ichimokuLookbackPositional(optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, core) < 0) {
    return -2147483648;
  }
  if (optInTenkanPeriod === Core.INTEGER_DEFAULT) {
    optInTenkanPeriod = 9;
  } else if (optInTenkanPeriod < 2 || optInTenkanPeriod > 100000) {
    return -2147483648;
  }
  if (optInKijunPeriod === Core.INTEGER_DEFAULT) {
    optInKijunPeriod = 26;
  } else if (optInKijunPeriod < 2 || optInKijunPeriod > 100000) {
    return -2147483648;
  }
  if (optInSenkouBPeriod === Core.INTEGER_DEFAULT) {
    optInSenkouBPeriod = 52;
  } else if (optInSenkouBPeriod < 2 || optInSenkouBPeriod > 100000) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 4) {
    return -2147483648;
  }
  if (outputIdx === 0 || outputIdx === 1) {
    return 0;
  }
  /* Positive: the two spans are drawn ahead of the bar that computed them. */
  if (outputIdx === 2 || outputIdx === 3) {
    return optInKijunPeriod;
  }
  return 0;
}

/** @internal */
export function ichimokuImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  optInTenkanPeriod: number,
  optInKijunPeriod: number,
  optInSenkouBPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outTenkanSen: Float64Array,
  outKijunSen: Float64Array,
  outSenkouSpanA: Float64Array,
  outSenkouSpanB: Float64Array,
): RetCode {
  let lookbackTotal = 0;
  let n = 0;
  let i = 0;
  const tempBegIdx = new MInteger();
  const tempNbElement = new MInteger();
  let tempT!: Float64Array;
  let tempK!: Float64Array;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTenkanPeriod === Core.INTEGER_DEFAULT) {
    optInTenkanPeriod = 9;
  } else if (optInTenkanPeriod < 2 || optInTenkanPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInKijunPeriod === Core.INTEGER_DEFAULT) {
    optInKijunPeriod = 26;
  } else if (optInKijunPeriod < 2 || optInKijunPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSenkouBPeriod === Core.INTEGER_DEFAULT) {
    optInSenkouBPeriod = 52;
  } else if (optInSenkouBPeriod < 2 || optInSenkouBPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if ((outTenkanSen.buffer === outKijunSen.buffer && outTenkanSen.byteOffset === outKijunSen.byteOffset) || (outTenkanSen.buffer === outSenkouSpanA.buffer && outTenkanSen.byteOffset === outSenkouSpanA.byteOffset) || (outTenkanSen.buffer === outSenkouSpanB.buffer && outTenkanSen.byteOffset === outSenkouSpanB.byteOffset) || (outKijunSen.buffer === outSenkouSpanA.buffer && outKijunSen.byteOffset === outSenkouSpanA.byteOffset) || (outKijunSen.buffer === outSenkouSpanB.buffer && outKijunSen.byteOffset === outSenkouSpanB.byteOffset) || (outSenkouSpanA.buffer === outSenkouSpanB.buffer && outSenkouSpanA.byteOffset === outSenkouSpanB.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = ichimokuLookbackPositional(optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  n = endIdx - startIdx + 1;
  tempT = new Float64Array(n);
  tempK = new Float64Array(n);
  /* An output may alias inHigh or inLow: no output is written before the
   * last midprice call, which is the last read of either input.
   */
  const _xr0 = midpricePositional(startIdx, endIdx, inHigh, inLow, optInTenkanPeriod, tempT, core);
  tempBegIdx.value = _xr0.begIdx;
  tempNbElement.value = _xr0.count;
  const _xr1 = midpricePositional(startIdx, endIdx, inHigh, inLow, optInKijunPeriod, tempK, core);
  tempBegIdx.value = _xr1.begIdx;
  tempNbElement.value = _xr1.count;
  const _xr2 = midpricePositional(startIdx, endIdx, inHigh, inLow, optInSenkouBPeriod, outSenkouSpanB, core);
  tempBegIdx.value = _xr2.begIdx;
  tempNbElement.value = _xr2.count;
  /* Span A halves the two rounded midpoints. (hh+ll+hh+ll)/4 is a different
   * double in the last bit.
   */
  const _xr3 = medpricePositional(0, n - 1, tempT, tempK, outSenkouSpanA, core);
  tempBegIdx.value = _xr3.begIdx;
  tempNbElement.value = _xr3.count;
  for (i = 0; i < n; i += 1) {
    outTenkanSen[i] = tempT[i];
    outKijunSen[i] = tempK[i];
  }
  outBegIdx.value = startIdx;
  outNBElement.value = n;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['tenkanPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['kijunPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['senkouBPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000']] as const;

/** @internal */
export function ichimokuPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTenkanPeriod: number,
  optInKijunPeriod: number,
  optInSenkouBPeriod: number,
  outTenkanSen: Float64Array,
  outKijunSen: Float64Array,
  outSenkouSpanA: Float64Array,
  outSenkouSpanB: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('ICHIMOKU', 'startIdx', startIdx);
  endIdx = requireInt32('ICHIMOKU', 'endIdx', endIdx);
  optInTenkanPeriod = requireInt32('ICHIMOKU', 'tenkanPeriod', optInTenkanPeriod);
  optInKijunPeriod = requireInt32('ICHIMOKU', 'kijunPeriod', optInKijunPeriod);
  optInSenkouBPeriod = requireInt32('ICHIMOKU', 'senkouBPeriod', optInSenkouBPeriod);
  requireIndexRange('ICHIMOKU', startIdx, endIdx);
  const guardLookback = ichimokuLookbackPositional(optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('ICHIMOKU', PARAM_RANGES, optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod);
  }
  const guardStart = clampedStart('ICHIMOKU', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('ICHIMOKU', 'prices.high', inHigh, guardInLen);
  requireLength('ICHIMOKU', 'prices.low', inLow, guardInLen);
  requireOutput('ICHIMOKU', 'outTenkanSen', outTenkanSen, guardOutLen, Float64Array);
  requireOutput('ICHIMOKU', 'outKijunSen', outKijunSen, guardOutLen, Float64Array);
  requireOutput('ICHIMOKU', 'outSenkouSpanA', outSenkouSpanA, guardOutLen, Float64Array);
  requireOutput('ICHIMOKU', 'outSenkouSpanB', outSenkouSpanB, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlapsPartly(outTenkanSen, inHighF64)) {
    throw sharedMemory('ICHIMOKU', 'outTenkanSen', 'prices.high');
  }
  if (overlapsPartly(outTenkanSen, inLowF64)) {
    throw sharedMemory('ICHIMOKU', 'outTenkanSen', 'prices.low');
  }
  if (overlapsPartly(outKijunSen, inHighF64)) {
    throw sharedMemory('ICHIMOKU', 'outKijunSen', 'prices.high');
  }
  if (overlapsPartly(outKijunSen, inLowF64)) {
    throw sharedMemory('ICHIMOKU', 'outKijunSen', 'prices.low');
  }
  if (overlapsPartly(outSenkouSpanA, inHighF64)) {
    throw sharedMemory('ICHIMOKU', 'outSenkouSpanA', 'prices.high');
  }
  if (overlapsPartly(outSenkouSpanA, inLowF64)) {
    throw sharedMemory('ICHIMOKU', 'outSenkouSpanA', 'prices.low');
  }
  if (overlapsPartly(outSenkouSpanB, inHighF64)) {
    throw sharedMemory('ICHIMOKU', 'outSenkouSpanB', 'prices.high');
  }
  if (overlapsPartly(outSenkouSpanB, inLowF64)) {
    throw sharedMemory('ICHIMOKU', 'outSenkouSpanB', 'prices.low');
  }
  if (overlaps(outTenkanSen, outKijunSen)) {
    throw sharedMemory('ICHIMOKU', 'outTenkanSen', 'outKijunSen');
  }
  if (overlaps(outTenkanSen, outSenkouSpanA)) {
    throw sharedMemory('ICHIMOKU', 'outTenkanSen', 'outSenkouSpanA');
  }
  if (overlaps(outTenkanSen, outSenkouSpanB)) {
    throw sharedMemory('ICHIMOKU', 'outTenkanSen', 'outSenkouSpanB');
  }
  if (overlaps(outKijunSen, outSenkouSpanA)) {
    throw sharedMemory('ICHIMOKU', 'outKijunSen', 'outSenkouSpanA');
  }
  if (overlaps(outKijunSen, outSenkouSpanB)) {
    throw sharedMemory('ICHIMOKU', 'outKijunSen', 'outSenkouSpanB');
  }
  if (overlaps(outSenkouSpanA, outSenkouSpanB)) {
    throw sharedMemory('ICHIMOKU', 'outSenkouSpanA', 'outSenkouSpanB');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = ichimokuImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, outBegIdx, outNBElement, outTenkanSen, outKijunSen, outSenkouSpanA, outSenkouSpanB);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('ICHIMOKU', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link ichimoku}, {@link ichimokuLookback},
 * {@link ichimokuDisplayShift}, {@link ichimokuOpen} and
 * {@link ichimokuOpenAndFill}. Every key is optional: an omitted or `undefined`
 * key takes its default, and a call ignores the keys it has no use for.
 */
export interface IchimokuOptions {
  /**
   * Period of the conversion line (default 9; range 2 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  tenkanPeriod?: number | undefined;
  /**
   * Period of the base line, and the forward shift of the two spans (default 26;
   * range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  kijunPeriod?: number | undefined;
  /**
   * Period of the second leading span (default 52; range 2 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  senkouBPeriod?: number | undefined;
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
   * Buffer for `outTenkanSen`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outTenkanSen?: Float64Array<ArrayBuffer> | undefined;
  /**
   * Buffer for `outKijunSen`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outKijunSen?: Float64Array<ArrayBuffer> | undefined;
  /**
   * Buffer for `outSenkouSpanA`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outSenkouSpanA?: Float64Array<ArrayBuffer> | undefined;
  /**
   * Buffer for `outSenkouSpanB`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outSenkouSpanB?: Float64Array<ArrayBuffer> | undefined;
}

/** What {@link ichimoku} returns, and {@link ichimokuOpenAndFill} with its stream. */
export interface IchimokuResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /** Conversion line. `outTenkanSen[k]` is the value at input bar `begIdx + k`. */
  outTenkanSen: Float64Array<ArrayBuffer>;
  /** Base line. `outKijunSen[k]` is the value at input bar `begIdx + k`. */
  outKijunSen: Float64Array<ArrayBuffer>;
  /**
   * First leading span, drawn ahead by the base period. `outSenkouSpanA[k]` is the
   * value at input bar `begIdx + k`.
   */
  outSenkouSpanA: Float64Array<ArrayBuffer>;
  /**
   * Second leading span, drawn ahead by the base period. `outSenkouSpanB[k]` is the
   * value at input bar `begIdx + k`.
   */
  outSenkouSpanB: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link ichimoku} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link IchimokuOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function ichimokuLookback(options?: IchimokuOptions): number {
  const o = optionsOf('ICHIMOKU lookback', options);
  return ichimokuLookbackPositional(orDefault(o.tenkanPeriod, Core.INTEGER_DEFAULT), orDefault(o.kijunPeriod, Core.INTEGER_DEFAULT), orDefault(o.senkouBPeriod, Core.INTEGER_DEFAULT), coreOf('ICHIMOKU lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link ichimoku}.
 *
 * The values are never shifted: this describes the drawing only.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link IchimokuOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function ichimokuDisplayShift(outputIdx: number, options?: IchimokuOptions): number {
  const o = optionsOf('ICHIMOKU displayShift', options);
  return ichimokuDisplayShiftPositional(orDefault(o.tenkanPeriod, Core.INTEGER_DEFAULT), orDefault(o.kijunPeriod, Core.INTEGER_DEFAULT), orDefault(o.senkouBPeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('ICHIMOKU displayShift', o.core));
}

/**
 * Ichimoku Kinko Hyo, "one glance equilibrium chart": four lines built from highs
 * and lows alone. Goichi Hosoda's reading is that a market's balance is visible
 * without any smoothing — each line is the midpoint of a window, the mean of its
 * highest high and its lowest low, so it marks the level at which that stretch of
 * trading was evenly divided. The conversion line turns fastest, the base line is
 * the reference, and the two leading spans are drawn ahead of price, where the
 * band between them is read as support or resistance before it is reached.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ichimoku](https://ta-lib.org/functions/ichimoku).
 *
 * **Notes**
 * - Each line is `midprice` over its own period, and Span A is `medprice` of the
 *   Tenkan and Kijun lines: it halves two midpoints that are already rounded.
 *   Averaging the four extremes in one step is the same number in real arithmetic
 *   and can differ in the last bit.
 * - The two spans are drawn `kijunPeriod` bars ahead of the bar that computed
 *   them. That is a display shift: `ichimokuDisplayShift` reports it, and it
 *   changes nothing about the values, the lookback or the returned range. Every
 *   output is written at the bar that computed it, so the last `kijunPeriod` span
 *   values are the cloud a chart draws past the last bar.
 * - Hosoda's 26 counts the current bar, which is a chart offset of 25.
 *   QuantConnect LEAN, pandas-ta and MetaTrader offset by 26, and that is what the
 *   display shift reports at the default periods; a caller who wants Hosoda's
 *   count draws the spans one bar earlier.
 * - The lookback is the longest of the three periods less one. Nothing orders the
 *   three, so any of them can be the longest.
 * - The Chikou span is the close drawn `kijunPeriod` bars back. It carries no
 *   computation and is not an output. Its usual signal, the close above the close
 *   `n` bars earlier, is `mom(close, n) > 0`.
 *
 * `outTenkanSen[k]` is the value at input bar `begIdx + k`; every output is
 * exactly as long as the number of values written, and nothing is padded. A range
 * that ends before {@link ichimokuLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `high`: High price
 *   series. `low`: Low price series.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link IchimokuOptions}.
 * @returns `begIdx` and the values; see {@link IchimokuResult}. Each output is a
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
 * import { ichimoku } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const {
 *   begIdx,
 *   outTenkanSen,
 *   outKijunSen,
 *   outSenkouSpanA,
 *   outSenkouSpanB,
 * } = ichimoku({ high, low }, {
 *   tenkanPeriod: 9,
 *   kijunPeriod: 26,
 *   senkouBPeriod: 52,
 * });
 * // outTenkanSen[k] is the value at bar begIdx + k
 * ```
 * @see [`midprice`](https://ta-lib.org/functions/midprice)
 * @see [`medprice`](https://ta-lib.org/functions/medprice)
 * @see [`sar`](https://ta-lib.org/functions/sar)
 * @see [`bbands`](https://ta-lib.org/functions/bbands)
 */
export function ichimoku(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: IchimokuOptions): IchimokuResult {
  requireArgument('ICHIMOKU', 'prices', prices);
  const o = optionsOf('ICHIMOKU', options);
  if (o.endIdx === undefined) {
    requireSeries('ICHIMOKU', 'prices.high', prices.high);
    requireSeries('ICHIMOKU', 'prices.low', prices.low);
  }
  const optInTenkanPeriod = orDefault(o.tenkanPeriod, Core.INTEGER_DEFAULT);
  const optInKijunPeriod = orDefault(o.kijunPeriod, Core.INTEGER_DEFAULT);
  const optInSenkouBPeriod = orDefault(o.senkouBPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('ICHIMOKU', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, ichimokuLookbackPositional(optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, core), prices.high, prices.low);
  const outTenkanSen = o.outTenkanSen === undefined ? new Float64Array(capacity) : o.outTenkanSen;
  const outKijunSen = o.outKijunSen === undefined ? new Float64Array(capacity) : o.outKijunSen;
  const outSenkouSpanA = o.outSenkouSpanA === undefined ? new Float64Array(capacity) : o.outSenkouSpanA;
  const outSenkouSpanB = o.outSenkouSpanB === undefined ? new Float64Array(capacity) : o.outSenkouSpanB;
  const range = ichimokuPositional(startIdx, endIdx, prices.high, prices.low, optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, outTenkanSen, outKijunSen, outSenkouSpanA, outSenkouSpanB, core);
  return { begIdx: range.begIdx, outTenkanSen: outTenkanSen.subarray(0, range.count), outKijunSen: outKijunSen.subarray(0, range.count), outSenkouSpanA: outSenkouSpanA.subarray(0, range.count), outSenkouSpanB: outSenkouSpanB.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live ICHIMOKU stream: one value per closed bar, bit-identical to
 * {@link ichimoku} over the same series. Open it with {@link ichimokuOpen} or
 * {@link ichimokuOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ichimoku](https://ta-lib.org/functions/ichimoku).
 */
export class IchimokuStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTenkanPeriod: number = 0;
  /** @internal */
  optInKijunPeriod: number = 0;
  /** @internal */
  optInSenkouBPeriod: number = 0;
  /** @internal */
  cur_outTenkanSen: number = 0;
  /** @internal */
  cur_outKijunSen: number = 0;
  /** @internal */
  cur_outSenkouSpanA: number = 0;
  /** @internal */
  cur_outSenkouSpanB: number = 0;
  /** @internal */
  sub0!: MidpriceStream;
  /** @internal */
  sub1!: MidpriceStream;
  /** @internal */
  sub2!: MidpriceStream;
  /** @internal */
  sub3!: MedpriceStream;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): IchimokuStream {
    return new IchimokuStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link ichimoku} reports them over the same bars. Each accepted `update` and
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
      throw streamFailure('ICHIMOKU advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns its outputs.
   *
   * @param high - The bar's `high` value: High price series.
   * @param low - The bar's `low` value: Low price series.
   * @param out - Receives the outputs and is returned; a new {@link IchimokuOut}
   *   when omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included, before anything moves.
   *   `TALibError` with code `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached
   *   bar `Core.INDEX_MAX`.
   */
  update(high: number, low: number, out?: IchimokuOut): IchimokuOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('ICHIMOKU update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('ICHIMOKU update', 'IchimokuOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low)) {
      throw nonFiniteBar('ICHIMOKU update', !Number.isFinite(high) ? 'high' : 'low');
    }
    ichimokuStepImpl(this, high, low);
    this.outRangeCount++;
    if (out === undefined) {
      out = new IchimokuOut();
    }
    out.outTenkanSen = this.cur_outTenkanSen;
    out.outKijunSen = this.cur_outKijunSen;
    out.outSenkouSpanA = this.cur_outSenkouSpanA;
    out.outSenkouSpanB = this.cur_outSenkouSpanB;
    return out;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param high - The bar's `high` value: High price series.
   * @param low - The bar's `low` value: Low price series.
   * @param out - Receives the outputs and is returned; a new {@link IchimokuOut}
   *   when omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included.
   */
  peek(high: number, low: number, out?: IchimokuOut): IchimokuOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('ICHIMOKU peek', 'IchimokuOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low)) {
      throw nonFiniteBar('ICHIMOKU peek', !Number.isFinite(high) ? 'high' : 'low');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
    let cur_tempT = 0;
    let cur_tempK = 0;
    let cur_outSenkouSpanB = 0;
    let cur_outSenkouSpanA = 0;
    let cur_outTenkanSen = 0;
    let cur_outKijunSen = 0;
    cur_tempT = sp.sub0.peek(inHigh, inLow);
    cur_tempK = sp.sub1.peek(inHigh, inLow);
    cur_outSenkouSpanB = sp.sub2.peek(inHigh, inLow);
    cur_outSenkouSpanA = sp.sub3.peek(cur_tempT, cur_tempK);
    cur_outTenkanSen = cur_tempT;
    cur_outKijunSen = cur_tempK;
    if (out === undefined) {
      out = new IchimokuOut();
    }
    out.outTenkanSen = cur_outTenkanSen;
    out.outKijunSen = cur_outKijunSen;
    out.outSenkouSpanA = cur_outSenkouSpanA;
    out.outSenkouSpanB = cur_outSenkouSpanB;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link IchimokuOut}
   *   when omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a sink that is not an object,
   *   `null` included.
   */
  value(out?: IchimokuOut): IchimokuOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('ICHIMOKU value', 'IchimokuOut');
    }
    if (out === undefined) {
      out = new IchimokuOut();
    }
    out.outTenkanSen = this.cur_outTenkanSen;
    out.outKijunSen = this.cur_outKijunSen;
    out.outSenkouSpanA = this.cur_outSenkouSpanA;
    out.outSenkouSpanB = this.cur_outSenkouSpanB;
    return out;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): IchimokuStream {
    const c = new IchimokuStream(this.core);
    c.optInTenkanPeriod = this.optInTenkanPeriod;
    c.optInKijunPeriod = this.optInKijunPeriod;
    c.optInSenkouBPeriod = this.optInSenkouBPeriod;
    c.cur_outTenkanSen = this.cur_outTenkanSen;
    c.cur_outKijunSen = this.cur_outKijunSen;
    c.cur_outSenkouSpanA = this.cur_outSenkouSpanA;
    c.cur_outSenkouSpanB = this.cur_outSenkouSpanB;
    c.sub0 = this.sub0.clone();
    c.sub1 = this.sub1.clone();
    c.sub2 = this.sub2.clone();
    c.sub3 = this.sub3.clone();
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one ICHIMOKU bar, as `update`, `peek` and `value` return them.
 * Passed back to one of them, it is filled and returned again, with no allocation;
 * it holds its values until the next call that writes it. A new one holds `NaN`.
 */
export class IchimokuOut {
  /** Conversion line. */
  outTenkanSen: number = NaN;
  /** Base line. */
  outKijunSen: number = NaN;
  /** First leading span, drawn ahead by the base period. */
  outSenkouSpanA: number = NaN;
  /** Second leading span, drawn ahead by the base period. */
  outSenkouSpanB: number = NaN;
}

function ichimokuStepImpl(sp: IchimokuStream, inHigh: number, inLow: number): void {
  let cur_tempT = 0;
  let cur_tempK = 0;
  let cur_outSenkouSpanB = 0;
  let cur_outSenkouSpanA = 0;
  let cur_outTenkanSen = 0;
  let cur_outKijunSen = 0;
  cur_tempT = sp.sub0.update(inHigh, inLow);
  cur_tempK = sp.sub1.update(inHigh, inLow);
  cur_outSenkouSpanB = sp.sub2.update(inHigh, inLow);
  cur_outSenkouSpanA = sp.sub3.update(cur_tempT, cur_tempK);
  cur_outTenkanSen = cur_tempT;
  cur_outKijunSen = cur_tempK;
  sp.cur_outTenkanSen = cur_outTenkanSen;
  sp.cur_outKijunSen = cur_outKijunSen;
  sp.cur_outSenkouSpanA = cur_outSenkouSpanA;
  sp.cur_outSenkouSpanB = cur_outSenkouSpanB;
}

function ichimokuOpenImpl(
  sp: IchimokuStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTenkanPeriod: number,
  optInKijunPeriod: number,
  optInSenkouBPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outTenkanSen: Float64Array,
  outKijunSen: Float64Array,
  outSenkouSpanA: Float64Array,
  outSenkouSpanB: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let lookbackTotal = 0;
  let n = 0;
  let i = 0;
  const tempBegIdx = new MInteger();
  const tempNbElement = new MInteger();
  let tempT!: Float64Array;
  let tempK!: Float64Array;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInTenkanPeriod === Core.INTEGER_DEFAULT) {
    optInTenkanPeriod = 9;
  } else if (optInTenkanPeriod < 2 || optInTenkanPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInKijunPeriod === Core.INTEGER_DEFAULT) {
    optInKijunPeriod = 26;
  } else if (optInKijunPeriod < 2 || optInKijunPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSenkouBPeriod === Core.INTEGER_DEFAULT) {
    optInSenkouBPeriod = 52;
  } else if (optInSenkouBPeriod < 2 || optInSenkouBPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen < ichimokuLookbackPositional(optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const sc_outTenkanSen = outStride === 1 ? outTenkanSen : new Float64Array(historyLen);
  const sc_outKijunSen = outStride === 1 ? outKijunSen : new Float64Array(historyLen);
  const sc_outSenkouSpanA = outStride === 1 ? outSenkouSpanA : new Float64Array(historyLen);
  const sc_outSenkouSpanB = outStride === 1 ? outSenkouSpanB : new Float64Array(historyLen);
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = ichimokuLookbackPositional(optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  n = endIdx - startIdx + 1;
  tempT = new Float64Array(n);
  tempK = new Float64Array(n);
  /* An output may alias inHigh or inLow: no output is written before the
   * last midprice call, which is the last read of either input.
   */
  const sub0 = midpriceOpenAndFillInternal(core, inHigh, inLow, startIdx, optInTenkanPeriod, tempBegIdx, tempNbElement, tempT);
  const sub1 = midpriceOpenAndFillInternal(core, inHigh, inLow, startIdx, optInKijunPeriod, tempBegIdx, tempNbElement, tempK);
  const sub2 = midpriceOpenAndFillInternal(core, inHigh, inLow, startIdx, optInSenkouBPeriod, tempBegIdx, tempNbElement, sc_outSenkouSpanB);
  /* Span A halves the two rounded midpoints. (hh+ll+hh+ll)/4 is a different
   * double in the last bit.
   */
  const sub3 = medpriceOpenAndFillInternal(core, tempT.slice(0, (n - 1) + 1), tempK.slice(0, (n - 1) + 1), 0, tempBegIdx, tempNbElement, sc_outSenkouSpanA);
  for (i = 0; i < n; i += 1) {
    sc_outTenkanSen[i] = tempT[i];
    sc_outKijunSen[i] = tempK[i];
  }
  outBegIdx.value = startIdx;
  outNBElement.value = n;
  if (outNBElement.value < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  sp.optInTenkanPeriod = optInTenkanPeriod;
  sp.optInKijunPeriod = optInKijunPeriod;
  sp.optInSenkouBPeriod = optInSenkouBPeriod;
  sp.sub0 = sub0;
  sp.sub1 = sub1;
  sp.sub2 = sub2;
  sp.sub3 = sub3;
  sp.cur_outTenkanSen = sc_outTenkanSen[outNBElement.value - 1];
  sp.cur_outKijunSen = sc_outKijunSen[outNBElement.value - 1];
  sp.cur_outSenkouSpanA = sc_outSenkouSpanA[outNBElement.value - 1];
  sp.cur_outSenkouSpanB = sc_outSenkouSpanB[outNBElement.value - 1];
  return RetCode.SUCCESS;
}

/** @internal */
export function ichimokuOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTenkanPeriod: number,
  optInKijunPeriod: number,
  optInSenkouBPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outTenkanSen: Float64Array,
  outKijunSen: Float64Array,
  outSenkouSpanA: Float64Array,
  outSenkouSpanB: Float64Array,
): IchimokuStream {
  const sp = IchimokuStream.of(core);
  const retCode = ichimokuOpenImpl(sp, inHigh, inLow, startIdx, optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, outBegIdx, outNBElement, outTenkanSen, outKijunSen, outSenkouSpanA, outSenkouSpanB, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ICHIMOKU openAndFill', inHigh.length, startIdx, ichimokuLookbackPositional(optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && ichimokuLookbackPositional(optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, core) < 0) {
    throw rejectedParams('ICHIMOKU openAndFill', PARAM_RANGES, optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod);
  }
  throw streamFailure('ICHIMOKU openAndFill', retCode);
}

/** @internal */
export function ichimokuOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTenkanPeriod: number,
  optInKijunPeriod: number,
  optInSenkouBPeriod: number,
): IchimokuStream {
  const sp = IchimokuStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outTenkanSen = new Float64Array(1);
  const sink_outKijunSen = new Float64Array(1);
  const sink_outSenkouSpanA = new Float64Array(1);
  const sink_outSenkouSpanB = new Float64Array(1);
  const retCode = ichimokuOpenImpl(sp, inHigh, inLow, startIdx, optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, outBegIdx, outNBElement, sink_outTenkanSen, sink_outKijunSen, sink_outSenkouSpanA, sink_outSenkouSpanB, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ICHIMOKU open', inHigh.length, startIdx, ichimokuLookbackPositional(optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && ichimokuLookbackPositional(optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, core) < 0) {
    throw rejectedParams('ICHIMOKU open', PARAM_RANGES, optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod);
  }
  throw streamFailure('ICHIMOKU open', retCode);
}

/** @internal */
export function ichimokuOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTenkanPeriod: number,
  optInKijunPeriod: number,
  optInSenkouBPeriod: number,
  core: Core = Core.DEFAULT,
): IchimokuStream {
  optInTenkanPeriod = requireInt32('ICHIMOKU open', 'tenkanPeriod', optInTenkanPeriod);
  optInKijunPeriod = requireInt32('ICHIMOKU open', 'kijunPeriod', optInKijunPeriod);
  optInSenkouBPeriod = requireInt32('ICHIMOKU open', 'senkouBPeriod', optInSenkouBPeriod);
  requireArgument('ICHIMOKU open', 'prices.high', inHigh);
  requireHistory('ICHIMOKU open', 'prices.high', inHigh.length);
  requireArgument('ICHIMOKU open', 'prices.low', inLow);
  requireHistoryLength('ICHIMOKU open', 'prices.low', inLow.length, inHigh.length);
  return ichimokuOpenInternal(core, float64(inHigh), float64(inLow), 0, optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod);
}

/** @internal */
export function ichimokuOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTenkanPeriod: number,
  optInKijunPeriod: number,
  optInSenkouBPeriod: number,
  outTenkanSen: Float64Array,
  outKijunSen: Float64Array,
  outSenkouSpanA: Float64Array,
  outSenkouSpanB: Float64Array,
  core: Core = Core.DEFAULT,
): IchimokuStream {
  optInTenkanPeriod = requireInt32('ICHIMOKU openAndFill', 'tenkanPeriod', optInTenkanPeriod);
  optInKijunPeriod = requireInt32('ICHIMOKU openAndFill', 'kijunPeriod', optInKijunPeriod);
  optInSenkouBPeriod = requireInt32('ICHIMOKU openAndFill', 'senkouBPeriod', optInSenkouBPeriod);
  requireArgument('ICHIMOKU openAndFill', 'prices.high', inHigh);
  requireHistory('ICHIMOKU openAndFill', 'prices.high', inHigh.length);
  requireArgument('ICHIMOKU openAndFill', 'prices.low', inLow);
  const guardLookback = ichimokuLookbackPositional(optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('ICHIMOKU openAndFill', PARAM_RANGES, optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod);
  }
  const guardOutLen = openFillCount('ICHIMOKU openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('ICHIMOKU openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireOutput('ICHIMOKU openAndFill', 'outTenkanSen', outTenkanSen, guardOutLen, Float64Array);
  requireOutput('ICHIMOKU openAndFill', 'outKijunSen', outKijunSen, guardOutLen, Float64Array);
  requireOutput('ICHIMOKU openAndFill', 'outSenkouSpanA', outSenkouSpanA, guardOutLen, Float64Array);
  requireOutput('ICHIMOKU openAndFill', 'outSenkouSpanB', outSenkouSpanB, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (outTenkanSen.buffer === inHighF64.buffer && outTenkanSen.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('ICHIMOKU openAndFill', 'outTenkanSen', 'prices.high');
  }
  if (outTenkanSen.buffer === inLowF64.buffer && outTenkanSen.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('ICHIMOKU openAndFill', 'outTenkanSen', 'prices.low');
  }
  if (outKijunSen.buffer === inHighF64.buffer && outKijunSen.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('ICHIMOKU openAndFill', 'outKijunSen', 'prices.high');
  }
  if (outKijunSen.buffer === inLowF64.buffer && outKijunSen.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('ICHIMOKU openAndFill', 'outKijunSen', 'prices.low');
  }
  if (outSenkouSpanA.buffer === inHighF64.buffer && outSenkouSpanA.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('ICHIMOKU openAndFill', 'outSenkouSpanA', 'prices.high');
  }
  if (outSenkouSpanA.buffer === inLowF64.buffer && outSenkouSpanA.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('ICHIMOKU openAndFill', 'outSenkouSpanA', 'prices.low');
  }
  if (outSenkouSpanB.buffer === inHighF64.buffer && outSenkouSpanB.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('ICHIMOKU openAndFill', 'outSenkouSpanB', 'prices.high');
  }
  if (outSenkouSpanB.buffer === inLowF64.buffer && outSenkouSpanB.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('ICHIMOKU openAndFill', 'outSenkouSpanB', 'prices.low');
  }
  if (outTenkanSen.buffer === outKijunSen.buffer && outTenkanSen.byteOffset === outKijunSen.byteOffset) {
    throw sharedMemory('ICHIMOKU openAndFill', 'outTenkanSen', 'outKijunSen');
  }
  if (outTenkanSen.buffer === outSenkouSpanA.buffer && outTenkanSen.byteOffset === outSenkouSpanA.byteOffset) {
    throw sharedMemory('ICHIMOKU openAndFill', 'outTenkanSen', 'outSenkouSpanA');
  }
  if (outTenkanSen.buffer === outSenkouSpanB.buffer && outTenkanSen.byteOffset === outSenkouSpanB.byteOffset) {
    throw sharedMemory('ICHIMOKU openAndFill', 'outTenkanSen', 'outSenkouSpanB');
  }
  if (outKijunSen.buffer === outSenkouSpanA.buffer && outKijunSen.byteOffset === outSenkouSpanA.byteOffset) {
    throw sharedMemory('ICHIMOKU openAndFill', 'outKijunSen', 'outSenkouSpanA');
  }
  if (outKijunSen.buffer === outSenkouSpanB.buffer && outKijunSen.byteOffset === outSenkouSpanB.byteOffset) {
    throw sharedMemory('ICHIMOKU openAndFill', 'outKijunSen', 'outSenkouSpanB');
  }
  if (outSenkouSpanA.buffer === outSenkouSpanB.buffer && outSenkouSpanA.byteOffset === outSenkouSpanB.byteOffset) {
    throw sharedMemory('ICHIMOKU openAndFill', 'outSenkouSpanA', 'outSenkouSpanB');
  }
  if (guardOutLen > 0 && overlaps(outTenkanSen, inHighF64)) {
    throw sharedMemory('ICHIMOKU openAndFill', 'outTenkanSen', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outTenkanSen, inLowF64)) {
    throw sharedMemory('ICHIMOKU openAndFill', 'outTenkanSen', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outKijunSen, inHighF64)) {
    throw sharedMemory('ICHIMOKU openAndFill', 'outKijunSen', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outKijunSen, inLowF64)) {
    throw sharedMemory('ICHIMOKU openAndFill', 'outKijunSen', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outSenkouSpanA, inHighF64)) {
    throw sharedMemory('ICHIMOKU openAndFill', 'outSenkouSpanA', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outSenkouSpanA, inLowF64)) {
    throw sharedMemory('ICHIMOKU openAndFill', 'outSenkouSpanA', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outSenkouSpanB, inHighF64)) {
    throw sharedMemory('ICHIMOKU openAndFill', 'outSenkouSpanB', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outSenkouSpanB, inLowF64)) {
    throw sharedMemory('ICHIMOKU openAndFill', 'outSenkouSpanB', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outTenkanSen, outKijunSen)) {
    throw sharedMemory('ICHIMOKU openAndFill', 'outTenkanSen', 'outKijunSen');
  }
  if (guardOutLen > 0 && overlaps(outTenkanSen, outSenkouSpanA)) {
    throw sharedMemory('ICHIMOKU openAndFill', 'outTenkanSen', 'outSenkouSpanA');
  }
  if (guardOutLen > 0 && overlaps(outTenkanSen, outSenkouSpanB)) {
    throw sharedMemory('ICHIMOKU openAndFill', 'outTenkanSen', 'outSenkouSpanB');
  }
  if (guardOutLen > 0 && overlaps(outKijunSen, outSenkouSpanA)) {
    throw sharedMemory('ICHIMOKU openAndFill', 'outKijunSen', 'outSenkouSpanA');
  }
  if (guardOutLen > 0 && overlaps(outKijunSen, outSenkouSpanB)) {
    throw sharedMemory('ICHIMOKU openAndFill', 'outKijunSen', 'outSenkouSpanB');
  }
  if (guardOutLen > 0 && overlaps(outSenkouSpanA, outSenkouSpanB)) {
    throw sharedMemory('ICHIMOKU openAndFill', 'outSenkouSpanA', 'outSenkouSpanB');
  }
  return ichimokuOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, new MInteger(), new MInteger(), outTenkanSen, outKijunSen, outSenkouSpanA, outSenkouSpanB);
}

/**
 * Opens a live ICHIMOKU stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link ichimoku} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price series. `low`: Low price series.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link IchimokuOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `ichimokuLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range
 *   or histories of different lengths.
 *
 * @example
 * ```ts
 * import { ichimokuOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = ichimokuOpen({ high, low }, {
 *   tenkanPeriod: 9,
 *   kijunPeriod: 26,
 *   senkouBPeriod: 52,
 * });
 * const peeked = stream.peek(101.4, 99.1); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1); // the bar closes
 * ```
 */
export function ichimokuOpen(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: IchimokuOptions): IchimokuStream {
  requireArgument('ICHIMOKU open', 'prices', prices);
  const o = optionsOf('ICHIMOKU open', options);
  const optInTenkanPeriod = orDefault(o.tenkanPeriod, Core.INTEGER_DEFAULT);
  const optInKijunPeriod = orDefault(o.kijunPeriod, Core.INTEGER_DEFAULT);
  const optInSenkouBPeriod = orDefault(o.senkouBPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('ICHIMOKU open', o.core);
  return ichimokuOpenPositional(prices.high, prices.low, optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, core);
}

/**
 * {@link ichimokuOpen} that also computes the outputs over the whole history in
 * the same pass, bit-identically to {@link ichimoku}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price series. `low`: Low price series.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link IchimokuOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link ichimoku} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `ichimokuLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { ichimokuOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const {
 *   stream,
 *   begIdx,
 *   outTenkanSen,
 *   outKijunSen,
 *   outSenkouSpanA,
 *   outSenkouSpanB,
 * } = ichimokuOpenAndFill({ high, low }, {
 *   tenkanPeriod: 9,
 *   kijunPeriod: 26,
 *   senkouBPeriod: 52,
 * });
 * // outTenkanSen[k] is the value at bar begIdx + k
 * ```
 */
export function ichimokuOpenAndFill(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: IchimokuOptions): IchimokuResult & { stream: IchimokuStream } {
  requireArgument('ICHIMOKU openAndFill', 'prices', prices);
  const o = optionsOf('ICHIMOKU openAndFill', options);
  const optInTenkanPeriod = orDefault(o.tenkanPeriod, Core.INTEGER_DEFAULT);
  const optInKijunPeriod = orDefault(o.kijunPeriod, Core.INTEGER_DEFAULT);
  const optInSenkouBPeriod = orDefault(o.senkouBPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('ICHIMOKU openAndFill', o.core);
  const capacity = fillCapacity(ichimokuLookbackPositional(optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, core), prices.high, prices.low);
  const outTenkanSen = o.outTenkanSen === undefined ? new Float64Array(capacity) : o.outTenkanSen;
  const outKijunSen = o.outKijunSen === undefined ? new Float64Array(capacity) : o.outKijunSen;
  const outSenkouSpanA = o.outSenkouSpanA === undefined ? new Float64Array(capacity) : o.outSenkouSpanA;
  const outSenkouSpanB = o.outSenkouSpanB === undefined ? new Float64Array(capacity) : o.outSenkouSpanB;
  const stream = ichimokuOpenAndFillPositional(prices.high, prices.low, optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, outTenkanSen, outKijunSen, outSenkouSpanA, outSenkouSpanB, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outTenkanSen: outTenkanSen.subarray(0, range.count), outKijunSen: outKijunSen.subarray(0, range.count), outSenkouSpanA: outSenkouSpanA.subarray(0, range.count), outSenkouSpanB: outSenkouSpanB.subarray(0, range.count) };
}
