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
 *  090426 MF,CC  Initial version (#370).
 *  092526 MF,CC  #446 exact zero total on a dead volume window.
 *  092626 MF,CC  #446 branch-free zero count.
 *  102326 MF,CC  #554 rare reset, zero run as a mask, output first.
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function rvolLookbackPositional(optInTimePeriod: number, _core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  return optInTimePeriod;
}

/** @internal */
export function rvolDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (rvolLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function rvolImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inVolume: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let periodTotal = 0;
  let baseline = 0;
  let todayVolume = 0;
  let trailingVolume = 0;
  let i = 0;
  let outIdx = 0;
  let trailingIdx = 0;
  let lookbackTotal = 0;
  let nullRun = 0;
  let liveIn = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  /* One bar more than a moving average of the same period: today is excluded
   * from its own baseline.
   */
  lookbackTotal = optInTimePeriod;
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  periodTotal = 0.0;
  trailingIdx = startIdx - lookbackTotal;
  /* Consecutive zero-volume bars, ending at the last one added. Once they
   * fill a window the total is exactly zero, where add-then-subtract would
   * leave the rounding residue of the volumes that departed, of either sign.
   * The run advances through a mask: as a select or a product gcc compiles
   * it to a branch that mispredicts on scattered zero volumes. The test is
   * fabs(v) <= 0.0 rather than == 0.0: the same result, NaN included, from
   * one flag instead of two.
   */
  nullRun = 0;
  i = trailingIdx;
  while (i < startIdx) {
    periodTotal += inVolume[i];
    liveIn = (!(Math.abs(inVolume[i]) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (liveIn - 1);
    i = i + 1;
  }
  outIdx = 0;
  while (i <= endIdx) {
    /* Drop the trailing bar BEFORE adding today's. Up to the first dead
     * window, that order makes each baseline bit-identical to the moving
     * average of the same period at the previous bar; the reverse order
     * differs only in the last ulp, so no tolerance can tell the two apart.
     */
    baseline = periodTotal / optInTimePeriod;
    trailingVolume = inVolume[trailingIdx];
    trailingIdx = trailingIdx + 1;
    todayVolume = inVolume[i];
    i = i + 1;
    /* After both reads (input and output may be one buffer) and before the
     * total moves, or the divide keeps the old total live across the reset.
     */
    outReal[outIdx] = todayVolume / baseline;
    outIdx = outIdx + 1;
    periodTotal -= trailingVolume;
    periodTotal += todayVolume;
    liveIn = (!(Math.abs(todayVolume) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (liveIn - 1);
    if (nullRun >= optInTimePeriod) {
      periodTotal = 0.0;
    }
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000']] as const;

/** @internal */
export function rvolPositional(
  startIdx: number,
  endIdx: number,
  inVolume: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('RVOL', 'startIdx', startIdx);
  endIdx = requireInt32('RVOL', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('RVOL', 'timePeriod', optInTimePeriod);
  requireIndexRange('RVOL', startIdx, endIdx);
  const guardLookback = rvolLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('RVOL', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('RVOL', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('RVOL', 'prices.volume', inVolume, guardInLen);
  requireOutput('RVOL', 'outReal', outReal, guardOutLen, Float64Array);
  const inVolumeF64 = float64(inVolume);
  if (overlapsPartly(outReal, inVolumeF64)) {
    throw sharedMemory('RVOL', 'outReal', 'prices.volume');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = rvolImpl(core, startIdx, endIdx, inVolumeF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('RVOL', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link rvol}, {@link rvolLookback}, {@link rvolDisplayShift},
 * {@link rvolOpen} and {@link rvolOpenAndFill}. Every key is optional: an omitted
 * or `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface RvolOptions {
  /**
   * Number of preceding bars averaged to form the baseline (default 20; range 1 to
   * 100000; `Core.INTEGER_DEFAULT` selects the default).
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
   * Buffer for `values`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outReal?: Float64Array<ArrayBuffer> | undefined;
}

/** What {@link rvol} returns, and {@link rvolOpenAndFill} with its stream. */
export interface RvolResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Ratio of the current bar's volume to the average of the preceding window.
   * `values[k]` is the value at input bar `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link rvol} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link RvolOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function rvolLookback(options?: RvolOptions): number {
  const o = optionsOf('RVOL lookback', options);
  return rvolLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('RVOL lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link rvol}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link RvolOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function rvolDisplayShift(outputIdx: number, options?: RvolOptions): number {
  const o = optionsOf('RVOL displayShift', options);
  return rvolDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('RVOL displayShift', o.core));
}

/**
 * Relative Volume: today's volume as a ratio to the average volume of the bars
 * that came before it. A value of 1 means the bar traded exactly its recent
 * average, above 1 means unusual participation, below 1 means the move is thin.
 * Because the current bar is excluded from its own baseline, a volume spike shows
 * up at full size instead of being diluted by the average it is compared against.
 * Read it as a confirmation filter rather than a direction signal: it says how
 * much conviction is behind a price move, not which way. Breakouts on a high ratio
 * are the ones that tend to follow through; the same breakout near 1 is the one to
 * distrust.
 *
 * Formula and more info at
 * [ta-lib.org/functions/rvol](https://ta-lib.org/functions/rvol).
 *
 * **Notes**
 * - The baseline is the mean of the N bars *preceding* the current one, so RVOL
 *   needs one bar more than a moving average of the same period before it emits a
 *   value.
 * - A window in which every bar traded nothing has a baseline of zero and no
 *   defined ratio: that element is ±Inf, or NaN when the current bar is also zero.
 *   Real volume is non-negative, so this only happens on a dead window — an
 *   instrument that did not trade at all, or a series carrying no volume, such as
 *   a cash-index feed.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link rvolLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `volume`: Volume of
 *   each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link RvolOptions}.
 * @returns `begIdx` and the values; see {@link RvolResult}. Each output is a view
 *   of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, an integer parameter that is not an int32, a parameter outside its range,
 *   an input too short for the range, a buffer that is empty, too short or of the
 *   wrong type or a buffer sharing memory with a `Float64Array` input it is not.
 *   Checked before anything is written.
 *
 * @example
 * ```ts
 * import { rvol } from 'ta-lib';
 *
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const { begIdx, values } = rvol({ volume }, { timePeriod: 20 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`obv`](https://ta-lib.org/functions/obv)
 * @see [`pvo`](https://ta-lib.org/functions/pvo)
 * @see [`vwma`](https://ta-lib.org/functions/vwma)
 * @see [`sma`](https://ta-lib.org/functions/sma)
 */
export function rvol(prices: { open?: ArrayLike<number> | undefined; high?: ArrayLike<number> | undefined; low?: ArrayLike<number> | undefined; close?: ArrayLike<number> | undefined; volume: ArrayLike<number>; openInterest?: ArrayLike<number> | undefined }, options?: RvolOptions): RvolResult {
  requireArgument('RVOL', 'prices', prices);
  const o = optionsOf('RVOL', options);
  if (o.endIdx === undefined) {
    requireSeries('RVOL', 'prices.volume', prices.volume);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('RVOL', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.volume.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, rvolLookbackPositional(optInTimePeriod, core), prices.volume);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = rvolPositional(startIdx, endIdx, prices.volume, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live RVOL stream: one value per closed bar, bit-identical to {@link rvol} over
 * the same series. Open it with {@link rvolOpen} or {@link rvolOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/rvol](https://ta-lib.org/functions/rvol).
 */
export class RvolStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  periodTotal: number = 0;
  /** @internal */
  nullRun: number = 0;
  /** @internal */
  ringPos_trailingIdx: number = 0;
  /** @internal */
  ringCap_trailingIdx: number = 0;
  /** @internal */
  ring_trailingIdx_inVolume: Float64Array = new Float64Array(1);
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
  static of(core: Core): RvolStream {
    return new RvolStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link rvol} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('RVOL advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param volume - The bar's `volume` value: Volume of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(volume: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('RVOL update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(volume)) {
      throw nonFiniteBar('RVOL update', 'volume');
    }
    rvolStepImpl(this, volume);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param volume - The bar's `volume` value: Volume of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(volume: number): number {
    if (!Number.isFinite(volume)) {
      throw nonFiniteBar('RVOL peek', 'volume');
    }
    const sp = this;
    const inVolume = volume;
    let baseline = 0;
    let todayVolume = 0;
    let cur_outReal = 0;
    /* Drop the trailing bar BEFORE adding today's. Up to the first dead
     * window, that order makes each baseline bit-identical to the moving
     * average of the same period at the previous bar; the reverse order
     * differs only in the last ulp, so no tolerance can tell the two apart.
     */
    baseline = sp.periodTotal / sp.optInTimePeriod;
    todayVolume = inVolume;
    /* After both reads (input and output may be one buffer) and before the
     * total moves, or the divide keeps the old total live across the reset.
     */
    cur_outReal = todayVolume / baseline;
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
  clone(): RvolStream {
    const c = new RvolStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.periodTotal = this.periodTotal;
    c.nullRun = this.nullRun;
    c.ringPos_trailingIdx = this.ringPos_trailingIdx;
    c.ringCap_trailingIdx = this.ringCap_trailingIdx;
    c.ring_trailingIdx_inVolume = this.ring_trailingIdx_inVolume.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function rvolStepImpl(sp: RvolStream, inVolume: number): void {
  let baseline = 0;
  let todayVolume = 0;
  let trailingVolume = 0;
  let liveIn = 0;
  let ringCapL_trailingIdx = 0;
  /* Drop the trailing bar BEFORE adding today's. Up to the first dead
   * window, that order makes each baseline bit-identical to the moving
   * average of the same period at the previous bar; the reverse order
   * differs only in the last ulp, so no tolerance can tell the two apart.
   */
  baseline = sp.periodTotal / sp.optInTimePeriod;
  trailingVolume = sp.ring_trailingIdx_inVolume[sp.ringPos_trailingIdx];
  todayVolume = inVolume;
  /* After both reads (input and output may be one buffer) and before the
   * total moves, or the divide keeps the old total live across the reset.
   */
  sp.cur_outReal = todayVolume / baseline;
  sp.periodTotal -= trailingVolume;
  sp.periodTotal += todayVolume;
  liveIn = (!(Math.abs(todayVolume) <= 0.0) ? 1 : 0);
  sp.nullRun = (sp.nullRun + 1) & (liveIn - 1);
  if (sp.nullRun >= sp.optInTimePeriod) {
    sp.periodTotal = 0.0;
  }
  ringCapL_trailingIdx = sp.ringCap_trailingIdx;
  sp.ring_trailingIdx_inVolume[sp.ringPos_trailingIdx] = inVolume;
  sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
  if (sp.ringPos_trailingIdx >= ringCapL_trailingIdx) {
    sp.ringPos_trailingIdx = 0;
  }
}

function rvolOpenImpl(
  sp: RvolStream,
  inVolume: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let periodTotal = 0;
  let baseline = 0;
  let todayVolume = 0;
  let trailingVolume = 0;
  let i = 0;
  let outIdx = 0;
  let trailingIdx = 0;
  let lookbackTotal = 0;
  let nullRun = 0;
  let liveIn = 0;
  const historyLen = inVolume.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* One bar more than a moving average of the same period: today is excluded
   * from its own baseline.
   */
  lookbackTotal = optInTimePeriod;
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  periodTotal = 0.0;
  trailingIdx = startIdx - lookbackTotal;
  /* Consecutive zero-volume bars, ending at the last one added. Once they
   * fill a window the total is exactly zero, where add-then-subtract would
   * leave the rounding residue of the volumes that departed, of either sign.
   * The run advances through a mask: as a select or a product gcc compiles
   * it to a branch that mispredicts on scattered zero volumes. The test is
   * fabs(v) <= 0.0 rather than == 0.0: the same result, NaN included, from
   * one flag instead of two.
   */
  nullRun = 0;
  i = trailingIdx;
  while (i < startIdx) {
    periodTotal += inVolume[i];
    liveIn = (!(Math.abs(inVolume[i]) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (liveIn - 1);
    i = i + 1;
  }
  outIdx = 0;
  while (i <= endIdx) {
    /* Drop the trailing bar BEFORE adding today's. Up to the first dead
     * window, that order makes each baseline bit-identical to the moving
     * average of the same period at the previous bar; the reverse order
     * differs only in the last ulp, so no tolerance can tell the two apart.
     */
    baseline = periodTotal / optInTimePeriod;
    trailingVolume = inVolume[trailingIdx];
    trailingIdx = trailingIdx + 1;
    todayVolume = inVolume[i];
    i = i + 1;
    /* After both reads (input and output may be one buffer) and before the
     * total moves, or the divide keeps the old total live across the reset.
     */
    outReal[((outIdx * outStride) | 0)] = todayVolume / baseline;
    outIdx = outIdx + 1;
    periodTotal -= trailingVolume;
    periodTotal += todayVolume;
    liveIn = (!(Math.abs(todayVolume) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (liveIn - 1);
    if (nullRun >= optInTimePeriod) {
      periodTotal = 0.0;
    }
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const cap_trailingIdx = i - trailingIdx;
  if (cap_trailingIdx < 1 || cap_trailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_trailingIdx = cap_trailingIdx > 0 ? cap_trailingIdx : 1;
  const capRing_trailingIdx_inVolume = new Float64Array(allocN_trailingIdx);
  capRing_trailingIdx_inVolume.set(inVolume.subarray(historyLen - cap_trailingIdx, historyLen), 0);
  sp.optInTimePeriod = optInTimePeriod;
  sp.periodTotal = periodTotal;
  sp.nullRun = nullRun;
  sp.ringPos_trailingIdx = 0;
  sp.ringCap_trailingIdx = cap_trailingIdx;
  sp.ring_trailingIdx_inVolume = capRing_trailingIdx_inVolume;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function rvolOpenAndFillInternal(
  core: Core,
  inVolume: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RvolStream {
  const sp = RvolStream.of(core);
  const retCode = rvolOpenImpl(sp, inVolume, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('RVOL openAndFill', inVolume.length, startIdx, rvolLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && rvolLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('RVOL openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('RVOL openAndFill', retCode);
}

/** @internal */
export function rvolOpenInternal(
  core: Core,
  inVolume: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): RvolStream {
  const sp = RvolStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = rvolOpenImpl(sp, inVolume, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('RVOL open', inVolume.length, startIdx, rvolLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && rvolLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('RVOL open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('RVOL open', retCode);
}

/** @internal */
export function rvolOpenPositional(
  inVolume: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): RvolStream {
  optInTimePeriod = requireInt32('RVOL open', 'timePeriod', optInTimePeriod);
  requireArgument('RVOL open', 'prices.volume', inVolume);
  requireHistory('RVOL open', 'prices.volume', inVolume.length);
  return rvolOpenInternal(core, float64(inVolume), 0, optInTimePeriod);
}

/** @internal */
export function rvolOpenAndFillPositional(
  inVolume: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): RvolStream {
  optInTimePeriod = requireInt32('RVOL openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('RVOL openAndFill', 'prices.volume', inVolume);
  requireHistory('RVOL openAndFill', 'prices.volume', inVolume.length);
  const guardLookback = rvolLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('RVOL openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('RVOL openAndFill', inVolume.length, guardLookback);
  requireOutput('RVOL openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inVolumeF64 = float64(inVolume);
  if (outReal.buffer === inVolumeF64.buffer && outReal.byteOffset === inVolumeF64.byteOffset) {
    throw sharedMemory('RVOL openAndFill', 'outReal', 'prices.volume');
  }
  if (guardOutLen > 0 && overlaps(outReal, inVolumeF64)) {
    throw sharedMemory('RVOL openAndFill', 'outReal', 'prices.volume');
  }
  return rvolOpenAndFillInternal(core, inVolumeF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live RVOL stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link rvol} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `volume`: Volume of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link RvolOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `rvolLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { rvolOpen } from 'ta-lib';
 *
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const stream = rvolOpen({ volume }, { timePeriod: 20 });
 * const peeked = stream.peek(12345); // a forming bar: commits nothing
 * const updated = stream.update(12345); // the bar closes
 * ```
 */
export function rvolOpen(prices: { open?: ArrayLike<number> | undefined; high?: ArrayLike<number> | undefined; low?: ArrayLike<number> | undefined; close?: ArrayLike<number> | undefined; volume: ArrayLike<number>; openInterest?: ArrayLike<number> | undefined }, options?: RvolOptions): RvolStream {
  requireArgument('RVOL open', 'prices', prices);
  const o = optionsOf('RVOL open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('RVOL open', o.core);
  return rvolOpenPositional(prices.volume, optInTimePeriod, core);
}

/**
 * {@link rvolOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link rvol}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `volume`: Volume of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link RvolOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link rvol} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `rvolLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or a
 *   buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { rvolOpenAndFill } from 'ta-lib';
 *
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const { stream, begIdx, values } = rvolOpenAndFill({ volume }, {
 *   timePeriod: 20,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function rvolOpenAndFill(prices: { open?: ArrayLike<number> | undefined; high?: ArrayLike<number> | undefined; low?: ArrayLike<number> | undefined; close?: ArrayLike<number> | undefined; volume: ArrayLike<number>; openInterest?: ArrayLike<number> | undefined }, options?: RvolOptions): RvolResult & { stream: RvolStream } {
  requireArgument('RVOL openAndFill', 'prices', prices);
  const o = optionsOf('RVOL openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('RVOL openAndFill', o.core);
  const capacity = fillCapacity(rvolLookbackPositional(optInTimePeriod, core), prices.volume);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = rvolOpenAndFillPositional(prices.volume, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
