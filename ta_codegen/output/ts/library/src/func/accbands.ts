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
 *  RM       Robert Meier
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  120307 RM     Initial Version
 *  120907 MF     Handling of a few limit cases
 *  071226 MF,CC  Fused single-loop rewrite: maintain the three band running
 *                sums (close for the middle band; the pointwise High/Low map for
 *                the upper/lower bands) over one shared trailing window, instead
 *                of two scratch buffers + three sma() calls. Enables streaming
 *                and is bit-identical to the prior three-SMA form (verified vs
 *                v0.6.4).
 *  082326 MF,CC  Fix #253. Scale the High+Low cancellation test to its own
 *                operands instead of the fixed TA_IS_ZERO band, which widened
 *                the bands of any instrument quoted small enough to fall
 *                under it.
 */

import { badSink, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { smaLookbackPositional } from './sma.ts';

/** @internal */
export function accbandsLookbackPositional(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  return smaLookbackPositional(optInTimePeriod, core);
}

/** @internal */
export function accbandsDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (accbandsLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 3) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function accbandsImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
): RetCode {
  let periodTotalUpper = 0;
  let periodTotalMiddle = 0;
  let periodTotalLower = 0;
  let tempUpper = 0;
  let tempMiddle = 0;
  let tempLower = 0;
  let tempReal = 0;
  let i = 0;
  let outIdx = 0;
  let trailingIdx = 0;
  let lookbackTotal = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if ((outRealUpperBand.buffer === outRealMiddleBand.buffer && outRealUpperBand.byteOffset === outRealMiddleBand.byteOffset) || (outRealUpperBand.buffer === outRealLowerBand.buffer && outRealUpperBand.byteOffset === outRealLowerBand.byteOffset) || (outRealMiddleBand.buffer === outRealLowerBand.buffer && outRealMiddleBand.byteOffset === outRealLowerBand.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = smaLookbackPositional(optInTimePeriod, core);
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
  /* Each band is a simple moving average maintained as a running sum over a
   * shared trailing window (all three share optInTimePeriod, so one trailing
   * index walks all three windows in lockstep):
   *    middle = SMA( close )
   *    upper  = SMA( high * (1 + 4*(high-low)/(high+low)) )
   *    lower  = SMA( low  * (1 - 4*(high-low)/(high+low)) )
   * When high+low is zero the upper/lower map degenerates to high/low.
   * Fusing the three moving averages into one loop is bit-identical to the
   * former "two scratch buffers + three sma() calls": each accumulator's
   * add/record/subtract order is unchanged, and the High/Low map is a pure
   * function recomputed from the raw trailing bar.
   */
  periodTotalUpper = 0.0;
  periodTotalMiddle = 0.0;
  periodTotalLower = 0.0;
  trailingIdx = startIdx - lookbackTotal;
  /* Warm up the running sums with the initial period,
   * except for the last value.
   */
  i = trailingIdx;
  while (i < startIdx) {
    /* The band factor 4*(H-L)/(H+L) is a ratio of two prices, so it is
     * scale-free -- but H+L is a sum that CANCELS when the two prices have
     * opposite signs, and the factor then blows up on what is left of the
     * operands' last bits. Test the sum against ITS OWN operands, not against
     * a fixed band: an absolute threshold answers "cancelled" for every bar
     * of an instrument quoted small enough to fall under it, and widened
     * every band it touched (issue #253). Same test on all three sites, so
     * the bar that enters a running sum is the one that later leaves it.
     */
    tempReal = inHigh[i] + inLow[i];
    if (!(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(inHigh[i]) + Math.abs(inLow[i])))) {
      tempReal = 4 * (inHigh[i] - inLow[i]) / tempReal;
      periodTotalUpper += inHigh[i] * (1 + tempReal);
      periodTotalLower += inLow[i] * (1 - tempReal);
    } else {
      periodTotalUpper += inHigh[i];
      periodTotalLower += inLow[i];
    }
    periodTotalMiddle += inClose[i];
    i = i + 1;
  }
  /* Proceed with the calculation for the requested range.
   * Note that this algorithm allows the input and output to be the
   * same buffer: every trailing bar is read before any output is written.
   */
  outIdx = 0;
  while (i <= endIdx) {
    /* Add the incoming bar to each running sum. */
    tempReal = inHigh[i] + inLow[i];
    if (!(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(inHigh[i]) + Math.abs(inLow[i])))) {
      tempReal = 4 * (inHigh[i] - inLow[i]) / tempReal;
      periodTotalUpper += inHigh[i] * (1 + tempReal);
      periodTotalLower += inLow[i] * (1 - tempReal);
    } else {
      periodTotalUpper += inHigh[i];
      periodTotalLower += inLow[i];
    }
    periodTotalMiddle += inClose[i];
    i = i + 1;
    /* Record the current window sums. */
    tempUpper = periodTotalUpper;
    tempMiddle = periodTotalMiddle;
    tempLower = periodTotalLower;
    /* Remove the trailing bar from each running sum. */
    tempReal = inHigh[trailingIdx] + inLow[trailingIdx];
    if (!(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(inHigh[trailingIdx]) + Math.abs(inLow[trailingIdx])))) {
      tempReal = 4 * (inHigh[trailingIdx] - inLow[trailingIdx]) / tempReal;
      periodTotalUpper -= inHigh[trailingIdx] * (1 + tempReal);
      periodTotalLower -= inLow[trailingIdx] * (1 - tempReal);
    } else {
      periodTotalUpper -= inHigh[trailingIdx];
      periodTotalLower -= inLow[trailingIdx];
    }
    periodTotalMiddle -= inClose[trailingIdx];
    trailingIdx = trailingIdx + 1;
    /* Write the three bands. */
    outRealUpperBand[outIdx] = tempUpper / optInTimePeriod;
    outRealMiddleBand[outIdx] = tempMiddle / optInTimePeriod;
    outRealLowerBand[outIdx] = tempLower / optInTimePeriod;
    outIdx = outIdx + 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000']] as const;

/** @internal */
export function accbandsPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('ACCBANDS', 'startIdx', startIdx);
  endIdx = requireInt32('ACCBANDS', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('ACCBANDS', 'timePeriod', optInTimePeriod);
  requireIndexRange('ACCBANDS', startIdx, endIdx);
  const guardLookback = accbandsLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('ACCBANDS', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('ACCBANDS', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('ACCBANDS', 'prices.high', inHigh, guardInLen);
  requireLength('ACCBANDS', 'prices.low', inLow, guardInLen);
  requireLength('ACCBANDS', 'prices.close', inClose, guardInLen);
  requireOutput('ACCBANDS', 'outUpperBand', outRealUpperBand, guardOutLen, Float64Array);
  requireOutput('ACCBANDS', 'outMiddleBand', outRealMiddleBand, guardOutLen, Float64Array);
  requireOutput('ACCBANDS', 'outLowerBand', outRealLowerBand, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outRealUpperBand, inHighF64)) {
    throw sharedMemory('ACCBANDS', 'outUpperBand', 'prices.high');
  }
  if (overlapsPartly(outRealUpperBand, inLowF64)) {
    throw sharedMemory('ACCBANDS', 'outUpperBand', 'prices.low');
  }
  if (overlapsPartly(outRealUpperBand, inCloseF64)) {
    throw sharedMemory('ACCBANDS', 'outUpperBand', 'prices.close');
  }
  if (overlapsPartly(outRealMiddleBand, inHighF64)) {
    throw sharedMemory('ACCBANDS', 'outMiddleBand', 'prices.high');
  }
  if (overlapsPartly(outRealMiddleBand, inLowF64)) {
    throw sharedMemory('ACCBANDS', 'outMiddleBand', 'prices.low');
  }
  if (overlapsPartly(outRealMiddleBand, inCloseF64)) {
    throw sharedMemory('ACCBANDS', 'outMiddleBand', 'prices.close');
  }
  if (overlapsPartly(outRealLowerBand, inHighF64)) {
    throw sharedMemory('ACCBANDS', 'outLowerBand', 'prices.high');
  }
  if (overlapsPartly(outRealLowerBand, inLowF64)) {
    throw sharedMemory('ACCBANDS', 'outLowerBand', 'prices.low');
  }
  if (overlapsPartly(outRealLowerBand, inCloseF64)) {
    throw sharedMemory('ACCBANDS', 'outLowerBand', 'prices.close');
  }
  if (overlaps(outRealUpperBand, outRealMiddleBand)) {
    throw sharedMemory('ACCBANDS', 'outUpperBand', 'outMiddleBand');
  }
  if (overlaps(outRealUpperBand, outRealLowerBand)) {
    throw sharedMemory('ACCBANDS', 'outUpperBand', 'outLowerBand');
  }
  if (overlaps(outRealMiddleBand, outRealLowerBand)) {
    throw sharedMemory('ACCBANDS', 'outMiddleBand', 'outLowerBand');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = accbandsImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInTimePeriod, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('ACCBANDS', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link accbands}, {@link accbandsLookback},
 * {@link accbandsDisplayShift}, {@link accbandsOpen} and
 * {@link accbandsOpenAndFill}. Every key is optional: an omitted or `undefined`
 * key takes its default, and a call ignores the keys it has no use for.
 */
export interface AccbandsOptions {
  /**
   * SMA smoothing period for all three bands (default 20; range 2 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  timePeriod?: number | undefined;
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
   * Buffer for `outUpperBand`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outUpperBand?: Float64Array<ArrayBuffer> | undefined;
  /**
   * Buffer for `outMiddleBand`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outMiddleBand?: Float64Array<ArrayBuffer> | undefined;
  /**
   * Buffer for `outLowerBand`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outLowerBand?: Float64Array<ArrayBuffer> | undefined;
}

/** What {@link accbands} returns, and {@link accbandsOpenAndFill} with its stream. */
export interface AccbandsResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * SMA of the range-scaled high band. `outUpperBand[k]` is the value at input bar
   * `begIdx + k`.
   */
  outUpperBand: Float64Array<ArrayBuffer>;
  /** SMA of the close. `outMiddleBand[k]` is the value at input bar `begIdx + k`. */
  outMiddleBand: Float64Array<ArrayBuffer>;
  /**
   * SMA of the range-scaled low band. `outLowerBand[k]` is the value at input bar
   * `begIdx + k`.
   */
  outLowerBand: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link accbands} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AccbandsOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function accbandsLookback(options?: AccbandsOptions): number {
  const o = optionsOf('ACCBANDS lookback', options);
  return accbandsLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('ACCBANDS lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link accbands}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AccbandsOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function accbandsDisplayShift(outputIdx: number, options?: AccbandsOptions): number {
  const o = optionsOf('ACCBANDS displayShift', options);
  return accbandsDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('ACCBANDS displayShift', o.core));
}

/**
 * Acceleration Bands: three overlap lines around price. The middle band is an SMA
 * of the close; the upper/lower bands are SMAs of the high/low scaled by an
 * intraday-range factor.
 *
 * Formula and more info at
 * [ta-lib.org/functions/accbands](https://ta-lib.org/functions/accbands).
 *
 * `outUpperBand[k]` is the value at input bar `begIdx + k`; every output is
 * exactly as long as the number of values written, and nothing is padded. A range
 * that ends before {@link accbandsLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `high`: High price of
 *   each bar. `low`: Low price of each bar. `close`: Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link AccbandsOptions}.
 * @returns `begIdx` and the values; see {@link AccbandsResult}. Each output is a
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
 * import { accbands } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const {
 *   begIdx,
 *   outUpperBand,
 *   outMiddleBand,
 *   outLowerBand,
 * } = accbands({ high, low, close }, { timePeriod: 20 });
 * // outUpperBand[k] is the value at bar begIdx + k
 * ```
 * @see [`sma`](https://ta-lib.org/functions/sma)
 * @see [`bbands`](https://ta-lib.org/functions/bbands)
 */
export function accbands(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: AccbandsOptions): AccbandsResult {
  requireArgument('ACCBANDS', 'prices', prices);
  const o = optionsOf('ACCBANDS', options);
  if (o.endIdx === undefined) {
    requireSeries('ACCBANDS', 'prices.high', prices.high);
    requireSeries('ACCBANDS', 'prices.low', prices.low);
    requireSeries('ACCBANDS', 'prices.close', prices.close);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('ACCBANDS', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, accbandsLookbackPositional(optInTimePeriod, core), prices.high, prices.low, prices.close);
  const outRealUpperBand = o.outUpperBand === undefined ? new Float64Array(capacity) : o.outUpperBand;
  const outRealMiddleBand = o.outMiddleBand === undefined ? new Float64Array(capacity) : o.outMiddleBand;
  const outRealLowerBand = o.outLowerBand === undefined ? new Float64Array(capacity) : o.outLowerBand;
  const range = accbandsPositional(startIdx, endIdx, prices.high, prices.low, prices.close, optInTimePeriod, outRealUpperBand, outRealMiddleBand, outRealLowerBand, core);
  return { begIdx: range.begIdx, outUpperBand: outRealUpperBand.subarray(0, range.count), outMiddleBand: outRealMiddleBand.subarray(0, range.count), outLowerBand: outRealLowerBand.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live ACCBANDS stream: one value per closed bar, bit-identical to
 * {@link accbands} over the same series. Open it with {@link accbandsOpen} or
 * {@link accbandsOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/accbands](https://ta-lib.org/functions/accbands).
 */
export class AccbandsStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  periodTotalUpper: number = 0;
  /** @internal */
  periodTotalMiddle: number = 0;
  /** @internal */
  periodTotalLower: number = 0;
  /** @internal */
  ringPos_trailingIdx: number = 0;
  /** @internal */
  ringCap_trailingIdx: number = 0;
  /** @internal */
  ring_trailingIdx_inHigh: Float64Array = new Float64Array(1);
  /** @internal */
  ring_trailingIdx_inLow: Float64Array = new Float64Array(1);
  /** @internal */
  ring_trailingIdx_inClose: Float64Array = new Float64Array(1);
  /** @internal */
  cur_outRealUpperBand: number = 0;
  /** @internal */
  cur_outRealMiddleBand: number = 0;
  /** @internal */
  cur_outRealLowerBand: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): AccbandsStream {
    return new AccbandsStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link accbands} reports them over the same bars. Each accepted `update` and
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
      throw streamFailure('ACCBANDS advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns its outputs.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @param out - Receives the outputs and is returned; a new {@link AccbandsOut}
   *   when omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included, before anything moves.
   *   `TALibError` with code `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached
   *   bar `Core.INDEX_MAX`.
   */
  update(high: number, low: number, close: number, out?: AccbandsOut): AccbandsOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('ACCBANDS update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('ACCBANDS update', 'AccbandsOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('ACCBANDS update', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    accbandsStepImpl(this, high, low, close);
    this.outRangeCount++;
    if (out === undefined) {
      out = new AccbandsOut();
    }
    out.outUpperBand = this.cur_outRealUpperBand;
    out.outMiddleBand = this.cur_outRealMiddleBand;
    out.outLowerBand = this.cur_outRealLowerBand;
    return out;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @param out - Receives the outputs and is returned; a new {@link AccbandsOut}
   *   when omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included.
   */
  peek(high: number, low: number, close: number, out?: AccbandsOut): AccbandsOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('ACCBANDS peek', 'AccbandsOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('ACCBANDS peek', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
    const inClose = close;
    let tempUpper = 0;
    let tempMiddle = 0;
    let tempLower = 0;
    let tempReal = 0;
    let cur_outRealLowerBand = 0;
    let cur_outRealMiddleBand = 0;
    let cur_outRealUpperBand = 0;
    let periodTotalLower = sp.periodTotalLower;
    let periodTotalMiddle = sp.periodTotalMiddle;
    let periodTotalUpper = sp.periodTotalUpper;
    /* Add the incoming bar to each running sum. */
    tempReal = inHigh + inLow;
    if (!(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(inHigh) + Math.abs(inLow)))) {
      tempReal = 4 * (inHigh - inLow) / tempReal;
      periodTotalUpper += inHigh * (1 + tempReal);
      periodTotalLower += inLow * (1 - tempReal);
    } else {
      periodTotalUpper += inHigh;
      periodTotalLower += inLow;
    }
    periodTotalMiddle += inClose;
    /* Record the current window sums. */
    tempUpper = periodTotalUpper;
    tempMiddle = periodTotalMiddle;
    tempLower = periodTotalLower;
    /* Remove the trailing bar from each running sum. */
    tempReal = sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx] + sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx];
    if (!(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx]) + Math.abs(sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx])))) {
      tempReal = 4 * (sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx] - sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx]) / tempReal;
      periodTotalUpper -= sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx] * (1 + tempReal);
      periodTotalLower -= sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx] * (1 - tempReal);
    } else {
      periodTotalUpper -= sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx];
      periodTotalLower -= sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx];
    }
    periodTotalMiddle -= sp.ring_trailingIdx_inClose[sp.ringPos_trailingIdx];
    /* Write the three bands. */
    cur_outRealUpperBand = tempUpper / sp.optInTimePeriod;
    cur_outRealMiddleBand = tempMiddle / sp.optInTimePeriod;
    cur_outRealLowerBand = tempLower / sp.optInTimePeriod;
    if (out === undefined) {
      out = new AccbandsOut();
    }
    out.outUpperBand = cur_outRealUpperBand;
    out.outMiddleBand = cur_outRealMiddleBand;
    out.outLowerBand = cur_outRealLowerBand;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link AccbandsOut}
   *   when omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a sink that is not an object,
   *   `null` included.
   */
  value(out?: AccbandsOut): AccbandsOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('ACCBANDS value', 'AccbandsOut');
    }
    if (out === undefined) {
      out = new AccbandsOut();
    }
    out.outUpperBand = this.cur_outRealUpperBand;
    out.outMiddleBand = this.cur_outRealMiddleBand;
    out.outLowerBand = this.cur_outRealLowerBand;
    return out;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): AccbandsStream {
    const c = new AccbandsStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.periodTotalUpper = this.periodTotalUpper;
    c.periodTotalMiddle = this.periodTotalMiddle;
    c.periodTotalLower = this.periodTotalLower;
    c.ringPos_trailingIdx = this.ringPos_trailingIdx;
    c.ringCap_trailingIdx = this.ringCap_trailingIdx;
    c.ring_trailingIdx_inHigh = this.ring_trailingIdx_inHigh.slice();
    c.ring_trailingIdx_inLow = this.ring_trailingIdx_inLow.slice();
    c.ring_trailingIdx_inClose = this.ring_trailingIdx_inClose.slice();
    c.cur_outRealUpperBand = this.cur_outRealUpperBand;
    c.cur_outRealMiddleBand = this.cur_outRealMiddleBand;
    c.cur_outRealLowerBand = this.cur_outRealLowerBand;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one ACCBANDS bar, as `update`, `peek` and `value` return them.
 * Passed back to one of them, it is filled and returned again, with no allocation;
 * it holds its values until the next call that writes it. A new one holds `NaN`.
 */
export class AccbandsOut {
  /** SMA of the range-scaled high band. */
  outUpperBand: number = NaN;
  /** SMA of the close. */
  outMiddleBand: number = NaN;
  /** SMA of the range-scaled low band. */
  outLowerBand: number = NaN;
}

function accbandsStepImpl(sp: AccbandsStream, inHigh: number, inLow: number, inClose: number): void {
  let tempUpper = 0;
  let tempMiddle = 0;
  let tempLower = 0;
  let tempReal = 0;
  let ringCapL_trailingIdx = 0;
  /* Add the incoming bar to each running sum. */
  tempReal = inHigh + inLow;
  if (!(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(inHigh) + Math.abs(inLow)))) {
    tempReal = 4 * (inHigh - inLow) / tempReal;
    sp.periodTotalUpper += inHigh * (1 + tempReal);
    sp.periodTotalLower += inLow * (1 - tempReal);
  } else {
    sp.periodTotalUpper += inHigh;
    sp.periodTotalLower += inLow;
  }
  sp.periodTotalMiddle += inClose;
  /* Record the current window sums. */
  tempUpper = sp.periodTotalUpper;
  tempMiddle = sp.periodTotalMiddle;
  tempLower = sp.periodTotalLower;
  /* Remove the trailing bar from each running sum. */
  tempReal = sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx] + sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx];
  if (!(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx]) + Math.abs(sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx])))) {
    tempReal = 4 * (sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx] - sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx]) / tempReal;
    sp.periodTotalUpper -= sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx] * (1 + tempReal);
    sp.periodTotalLower -= sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx] * (1 - tempReal);
  } else {
    sp.periodTotalUpper -= sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx];
    sp.periodTotalLower -= sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx];
  }
  sp.periodTotalMiddle -= sp.ring_trailingIdx_inClose[sp.ringPos_trailingIdx];
  /* Write the three bands. */
  sp.cur_outRealUpperBand = tempUpper / sp.optInTimePeriod;
  sp.cur_outRealMiddleBand = tempMiddle / sp.optInTimePeriod;
  sp.cur_outRealLowerBand = tempLower / sp.optInTimePeriod;
  ringCapL_trailingIdx = sp.ringCap_trailingIdx;
  sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx] = inHigh;
  sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx] = inLow;
  sp.ring_trailingIdx_inClose[sp.ringPos_trailingIdx] = inClose;
  sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
  if (sp.ringPos_trailingIdx >= ringCapL_trailingIdx) {
    sp.ringPos_trailingIdx = 0;
  }
}

function accbandsOpenImpl(
  sp: AccbandsStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let periodTotalUpper = 0;
  let periodTotalMiddle = 0;
  let periodTotalLower = 0;
  let tempUpper = 0;
  let tempMiddle = 0;
  let tempLower = 0;
  let tempReal = 0;
  let i = 0;
  let outIdx = 0;
  let trailingIdx = 0;
  let lookbackTotal = 0;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length || inClose.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = smaLookbackPositional(optInTimePeriod, core);
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
  /* Each band is a simple moving average maintained as a running sum over a
   * shared trailing window (all three share optInTimePeriod, so one trailing
   * index walks all three windows in lockstep):
   *    middle = SMA( close )
   *    upper  = SMA( high * (1 + 4*(high-low)/(high+low)) )
   *    lower  = SMA( low  * (1 - 4*(high-low)/(high+low)) )
   * When high+low is zero the upper/lower map degenerates to high/low.
   * Fusing the three moving averages into one loop is bit-identical to the
   * former "two scratch buffers + three sma() calls": each accumulator's
   * add/record/subtract order is unchanged, and the High/Low map is a pure
   * function recomputed from the raw trailing bar.
   */
  periodTotalUpper = 0.0;
  periodTotalMiddle = 0.0;
  periodTotalLower = 0.0;
  trailingIdx = startIdx - lookbackTotal;
  /* Warm up the running sums with the initial period,
   * except for the last value.
   */
  i = trailingIdx;
  while (i < startIdx) {
    /* The band factor 4*(H-L)/(H+L) is a ratio of two prices, so it is
     * scale-free -- but H+L is a sum that CANCELS when the two prices have
     * opposite signs, and the factor then blows up on what is left of the
     * operands' last bits. Test the sum against ITS OWN operands, not against
     * a fixed band: an absolute threshold answers "cancelled" for every bar
     * of an instrument quoted small enough to fall under it, and widened
     * every band it touched (issue #253). Same test on all three sites, so
     * the bar that enters a running sum is the one that later leaves it.
     */
    tempReal = inHigh[i] + inLow[i];
    if (!(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(inHigh[i]) + Math.abs(inLow[i])))) {
      tempReal = 4 * (inHigh[i] - inLow[i]) / tempReal;
      periodTotalUpper += inHigh[i] * (1 + tempReal);
      periodTotalLower += inLow[i] * (1 - tempReal);
    } else {
      periodTotalUpper += inHigh[i];
      periodTotalLower += inLow[i];
    }
    periodTotalMiddle += inClose[i];
    i = i + 1;
  }
  /* Proceed with the calculation for the requested range.
   * Note that this algorithm allows the input and output to be the
   * same buffer: every trailing bar is read before any output is written.
   */
  outIdx = 0;
  while (i <= endIdx) {
    /* Add the incoming bar to each running sum. */
    tempReal = inHigh[i] + inLow[i];
    if (!(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(inHigh[i]) + Math.abs(inLow[i])))) {
      tempReal = 4 * (inHigh[i] - inLow[i]) / tempReal;
      periodTotalUpper += inHigh[i] * (1 + tempReal);
      periodTotalLower += inLow[i] * (1 - tempReal);
    } else {
      periodTotalUpper += inHigh[i];
      periodTotalLower += inLow[i];
    }
    periodTotalMiddle += inClose[i];
    i = i + 1;
    /* Record the current window sums. */
    tempUpper = periodTotalUpper;
    tempMiddle = periodTotalMiddle;
    tempLower = periodTotalLower;
    /* Remove the trailing bar from each running sum. */
    tempReal = inHigh[trailingIdx] + inLow[trailingIdx];
    if (!(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(inHigh[trailingIdx]) + Math.abs(inLow[trailingIdx])))) {
      tempReal = 4 * (inHigh[trailingIdx] - inLow[trailingIdx]) / tempReal;
      periodTotalUpper -= inHigh[trailingIdx] * (1 + tempReal);
      periodTotalLower -= inLow[trailingIdx] * (1 - tempReal);
    } else {
      periodTotalUpper -= inHigh[trailingIdx];
      periodTotalLower -= inLow[trailingIdx];
    }
    periodTotalMiddle -= inClose[trailingIdx];
    trailingIdx = trailingIdx + 1;
    /* Write the three bands. */
    outRealUpperBand[((outIdx * outStride) | 0)] = tempUpper / optInTimePeriod;
    outRealMiddleBand[((outIdx * outStride) | 0)] = tempMiddle / optInTimePeriod;
    outRealLowerBand[((outIdx * outStride) | 0)] = tempLower / optInTimePeriod;
    outIdx = outIdx + 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  const cap_trailingIdx = i - trailingIdx;
  if (cap_trailingIdx < 1 || cap_trailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_trailingIdx = cap_trailingIdx > 0 ? cap_trailingIdx : 1;
  const capRing_trailingIdx_inHigh = new Float64Array(allocN_trailingIdx);
  capRing_trailingIdx_inHigh.set(inHigh.subarray(historyLen - cap_trailingIdx, historyLen), 0);
  const capRing_trailingIdx_inLow = new Float64Array(allocN_trailingIdx);
  capRing_trailingIdx_inLow.set(inLow.subarray(historyLen - cap_trailingIdx, historyLen), 0);
  const capRing_trailingIdx_inClose = new Float64Array(allocN_trailingIdx);
  capRing_trailingIdx_inClose.set(inClose.subarray(historyLen - cap_trailingIdx, historyLen), 0);
  sp.optInTimePeriod = optInTimePeriod;
  sp.periodTotalUpper = periodTotalUpper;
  sp.periodTotalMiddle = periodTotalMiddle;
  sp.periodTotalLower = periodTotalLower;
  sp.ringPos_trailingIdx = 0;
  sp.ringCap_trailingIdx = cap_trailingIdx;
  sp.ring_trailingIdx_inHigh = capRing_trailingIdx_inHigh;
  sp.ring_trailingIdx_inLow = capRing_trailingIdx_inLow;
  sp.ring_trailingIdx_inClose = capRing_trailingIdx_inClose;
  sp.cur_outRealUpperBand = outRealUpperBand[(outNBElement.value - 1) * outStride];
  sp.cur_outRealMiddleBand = outRealMiddleBand[(outNBElement.value - 1) * outStride];
  sp.cur_outRealLowerBand = outRealLowerBand[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function accbandsOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
): AccbandsStream {
  const sp = AccbandsStream.of(core);
  const retCode = accbandsOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ACCBANDS openAndFill', inHigh.length, startIdx, accbandsLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && accbandsLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('ACCBANDS openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('ACCBANDS openAndFill', retCode);
}

/** @internal */
export function accbandsOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): AccbandsStream {
  const sp = AccbandsStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outRealUpperBand = new Float64Array(1);
  const sink_outRealMiddleBand = new Float64Array(1);
  const sink_outRealLowerBand = new Float64Array(1);
  const retCode = accbandsOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outRealUpperBand, sink_outRealMiddleBand, sink_outRealLowerBand, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ACCBANDS open', inHigh.length, startIdx, accbandsLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && accbandsLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('ACCBANDS open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('ACCBANDS open', retCode);
}

/** @internal */
export function accbandsOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): AccbandsStream {
  optInTimePeriod = requireInt32('ACCBANDS open', 'timePeriod', optInTimePeriod);
  requireArgument('ACCBANDS open', 'prices.high', inHigh);
  requireHistory('ACCBANDS open', 'prices.high', inHigh.length);
  requireArgument('ACCBANDS open', 'prices.low', inLow);
  requireArgument('ACCBANDS open', 'prices.close', inClose);
  requireHistoryLength('ACCBANDS open', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('ACCBANDS open', 'prices.close', inClose.length, inHigh.length);
  return accbandsOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInTimePeriod);
}

/** @internal */
export function accbandsOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
  core: Core = Core.DEFAULT,
): AccbandsStream {
  optInTimePeriod = requireInt32('ACCBANDS openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('ACCBANDS openAndFill', 'prices.high', inHigh);
  requireHistory('ACCBANDS openAndFill', 'prices.high', inHigh.length);
  requireArgument('ACCBANDS openAndFill', 'prices.low', inLow);
  requireArgument('ACCBANDS openAndFill', 'prices.close', inClose);
  const guardLookback = accbandsLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('ACCBANDS openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('ACCBANDS openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('ACCBANDS openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('ACCBANDS openAndFill', 'prices.close', inClose.length, inHigh.length);
  requireOutput('ACCBANDS openAndFill', 'outUpperBand', outRealUpperBand, guardOutLen, Float64Array);
  requireOutput('ACCBANDS openAndFill', 'outMiddleBand', outRealMiddleBand, guardOutLen, Float64Array);
  requireOutput('ACCBANDS openAndFill', 'outLowerBand', outRealLowerBand, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outRealUpperBand.buffer === inHighF64.buffer && outRealUpperBand.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('ACCBANDS openAndFill', 'outUpperBand', 'prices.high');
  }
  if (outRealUpperBand.buffer === inLowF64.buffer && outRealUpperBand.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('ACCBANDS openAndFill', 'outUpperBand', 'prices.low');
  }
  if (outRealUpperBand.buffer === inCloseF64.buffer && outRealUpperBand.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('ACCBANDS openAndFill', 'outUpperBand', 'prices.close');
  }
  if (outRealMiddleBand.buffer === inHighF64.buffer && outRealMiddleBand.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('ACCBANDS openAndFill', 'outMiddleBand', 'prices.high');
  }
  if (outRealMiddleBand.buffer === inLowF64.buffer && outRealMiddleBand.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('ACCBANDS openAndFill', 'outMiddleBand', 'prices.low');
  }
  if (outRealMiddleBand.buffer === inCloseF64.buffer && outRealMiddleBand.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('ACCBANDS openAndFill', 'outMiddleBand', 'prices.close');
  }
  if (outRealLowerBand.buffer === inHighF64.buffer && outRealLowerBand.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('ACCBANDS openAndFill', 'outLowerBand', 'prices.high');
  }
  if (outRealLowerBand.buffer === inLowF64.buffer && outRealLowerBand.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('ACCBANDS openAndFill', 'outLowerBand', 'prices.low');
  }
  if (outRealLowerBand.buffer === inCloseF64.buffer && outRealLowerBand.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('ACCBANDS openAndFill', 'outLowerBand', 'prices.close');
  }
  if (outRealUpperBand.buffer === outRealMiddleBand.buffer && outRealUpperBand.byteOffset === outRealMiddleBand.byteOffset) {
    throw sharedMemory('ACCBANDS openAndFill', 'outUpperBand', 'outMiddleBand');
  }
  if (outRealUpperBand.buffer === outRealLowerBand.buffer && outRealUpperBand.byteOffset === outRealLowerBand.byteOffset) {
    throw sharedMemory('ACCBANDS openAndFill', 'outUpperBand', 'outLowerBand');
  }
  if (outRealMiddleBand.buffer === outRealLowerBand.buffer && outRealMiddleBand.byteOffset === outRealLowerBand.byteOffset) {
    throw sharedMemory('ACCBANDS openAndFill', 'outMiddleBand', 'outLowerBand');
  }
  if (guardOutLen > 0 && overlaps(outRealUpperBand, inHighF64)) {
    throw sharedMemory('ACCBANDS openAndFill', 'outUpperBand', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outRealUpperBand, inLowF64)) {
    throw sharedMemory('ACCBANDS openAndFill', 'outUpperBand', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outRealUpperBand, inCloseF64)) {
    throw sharedMemory('ACCBANDS openAndFill', 'outUpperBand', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outRealMiddleBand, inHighF64)) {
    throw sharedMemory('ACCBANDS openAndFill', 'outMiddleBand', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outRealMiddleBand, inLowF64)) {
    throw sharedMemory('ACCBANDS openAndFill', 'outMiddleBand', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outRealMiddleBand, inCloseF64)) {
    throw sharedMemory('ACCBANDS openAndFill', 'outMiddleBand', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outRealLowerBand, inHighF64)) {
    throw sharedMemory('ACCBANDS openAndFill', 'outLowerBand', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outRealLowerBand, inLowF64)) {
    throw sharedMemory('ACCBANDS openAndFill', 'outLowerBand', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outRealLowerBand, inCloseF64)) {
    throw sharedMemory('ACCBANDS openAndFill', 'outLowerBand', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outRealUpperBand, outRealMiddleBand)) {
    throw sharedMemory('ACCBANDS openAndFill', 'outUpperBand', 'outMiddleBand');
  }
  if (guardOutLen > 0 && overlaps(outRealUpperBand, outRealLowerBand)) {
    throw sharedMemory('ACCBANDS openAndFill', 'outUpperBand', 'outLowerBand');
  }
  if (guardOutLen > 0 && overlaps(outRealMiddleBand, outRealLowerBand)) {
    throw sharedMemory('ACCBANDS openAndFill', 'outMiddleBand', 'outLowerBand');
  }
  return accbandsOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outRealUpperBand, outRealMiddleBand, outRealLowerBand);
}

/**
 * Opens a live ACCBANDS stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link accbands} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar. `close`: Close
 *   price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AccbandsOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `accbandsLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range
 *   or histories of different lengths.
 *
 * @example
 * ```ts
 * import { accbandsOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = accbandsOpen({ high, low, close }, { timePeriod: 20 });
 * const peeked = stream.peek(101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function accbandsOpen(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: AccbandsOptions): AccbandsStream {
  requireArgument('ACCBANDS open', 'prices', prices);
  const o = optionsOf('ACCBANDS open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('ACCBANDS open', o.core);
  return accbandsOpenPositional(prices.high, prices.low, prices.close, optInTimePeriod, core);
}

/**
 * {@link accbandsOpen} that also computes the outputs over the whole history in
 * the same pass, bit-identically to {@link accbands}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar. `close`: Close
 *   price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link AccbandsOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link accbands} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `accbandsLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { accbandsOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const {
 *   stream,
 *   begIdx,
 *   outUpperBand,
 *   outMiddleBand,
 *   outLowerBand,
 * } = accbandsOpenAndFill({ high, low, close }, { timePeriod: 20 });
 * // outUpperBand[k] is the value at bar begIdx + k
 * ```
 */
export function accbandsOpenAndFill(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: AccbandsOptions): AccbandsResult & { stream: AccbandsStream } {
  requireArgument('ACCBANDS openAndFill', 'prices', prices);
  const o = optionsOf('ACCBANDS openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('ACCBANDS openAndFill', o.core);
  const capacity = fillCapacity(accbandsLookbackPositional(optInTimePeriod, core), prices.high, prices.low, prices.close);
  const outRealUpperBand = o.outUpperBand === undefined ? new Float64Array(capacity) : o.outUpperBand;
  const outRealMiddleBand = o.outMiddleBand === undefined ? new Float64Array(capacity) : o.outMiddleBand;
  const outRealLowerBand = o.outLowerBand === undefined ? new Float64Array(capacity) : o.outLowerBand;
  const stream = accbandsOpenAndFillPositional(prices.high, prices.low, prices.close, optInTimePeriod, outRealUpperBand, outRealMiddleBand, outRealLowerBand, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outUpperBand: outRealUpperBand.subarray(0, range.count), outMiddleBand: outRealMiddleBand.subarray(0, range.count), outLowerBand: outRealLowerBand.subarray(0, range.count) };
}
