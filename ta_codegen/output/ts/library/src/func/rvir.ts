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
 *  KL       Kevin Lin
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  091126 KL     First version (proposal-drafts issue #68).
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { RviStream, rviLookbackPositional, rviOpenAndFillInternal, rviPositional } from './rvi.ts';

/** @internal */
export function rvirLookbackPositional(optInTimePeriod: number, optInStdDevPeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if ((optInStdDevPeriod | 0) !== optInStdDevPeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  if (optInStdDevPeriod === Core.INTEGER_DEFAULT) {
    optInStdDevPeriod = 10;
  } else if (optInStdDevPeriod < 2 || optInStdDevPeriod > 100000) {
    return -1;
  }
  /* Both legs are the shipped TA_RVI at the same parameters, so they warm on
   * the same bar and there is no max() to take. Stated as the callee's
   * lookback rather than restating the arithmetic, which is what makes this
   * function inherit TA_FUNC_UNST_RVI the way KC inherits its two (kc.c).
   */
  return rviLookbackPositional(optInTimePeriod, optInStdDevPeriod, core);
}

/** @internal */
export function rvirDisplayShiftPositional(optInTimePeriod: number, optInStdDevPeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (rvirLookbackPositional(optInTimePeriod, optInStdDevPeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function rvirImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  optInTimePeriod: number,
  optInStdDevPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let tempHigh!: Float64Array;
  let i = 0;
  let lookbackTotal = 0;
  const tempBegIdx = new MInteger();
  const tempNbElement = new MInteger();
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInStdDevPeriod === Core.INTEGER_DEFAULT) {
    optInStdDevPeriod = 10;
  } else if (optInStdDevPeriod < 2 || optInStdDevPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = rvirLookbackPositional(optInTimePeriod, optInStdDevPeriod, core);
  /* Nothing is allocated and no input is read when the range cannot produce a
   * value, so a caller-supplied input that stops short of endIdx is never
   * read past its end (kc.c).
   */
  if (lookbackTotal > endIdx) {
    return RetCode.SUCCESS;
  }
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  /* Each leg is the shipped TA_RVI over this range, entered at the same
   * startIdx: same function, same parameters, same lookback, so the two
   * outputs are aligned bar for bar with no anchor to reconcile.
   *
   * Two calls rather than one fused loop. The fused body is expressible --
   * two copies of TA_RVI's state driven by one bar cursor -- but it carries
   * two window-start cursors, and the streaming analyzer accepts exactly one
   * ("extrema automaton: expected exactly one window-start variable"), so
   * fusing would cost this function the streaming tier. Composing keeps it,
   * for the reason KC's legs do: each leg streams as itself.
   */
  tempHigh = new Float64Array(endIdx - startIdx + 1);
  /* Either input may be aliased onto outReal. Both legs are safe against that
   * for TA_RVI's own reason -- its write index trails its read index by the
   * lookback and never overtakes it -- so the order of the two calls is an
   * implementation detail, not what makes the aliasing safe. Swapping them
   * leaves every value identical, which is why the aliasing test earns its
   * keep against the scratch buffer's extent rather than against this order.
   */
  const _xr0 = rviPositional(startIdx, endIdx, inHigh, optInTimePeriod, optInStdDevPeriod, tempHigh, core);
  tempBegIdx.value = _xr0.begIdx;
  tempNbElement.value = _xr0.count;
  const _xr1 = rviPositional(startIdx, endIdx, inLow, optInTimePeriod, optInStdDevPeriod, outReal, core);
  outBegIdx.value = _xr1.begIdx;
  outNBElement.value = _xr1.count;
  /* Each leg has already resolved its own zero-total case to TA_RVI's neutral
   * 50 before the average is taken, so a tie in one series never drags the
   * other. Scaling by one half is exact, so the spelling of the average is
   * not a variant: 0.5*(a+b) is one rounding of a+b, as are (a+b)/2 and
   * 0.5*a + 0.5*b.
   */
  for (i = 0; i < outNBElement.value; i += 1) {
    outReal[i] = 0.5 * (tempHigh[i] + outReal[i]);
  }
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['stdDevPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000']] as const;

/** @internal */
export function rvirPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  optInStdDevPeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('RVIR', 'startIdx', startIdx);
  endIdx = requireInt32('RVIR', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('RVIR', 'timePeriod', optInTimePeriod);
  optInStdDevPeriod = requireInt32('RVIR', 'stdDevPeriod', optInStdDevPeriod);
  requireIndexRange('RVIR', startIdx, endIdx);
  const guardLookback = rvirLookbackPositional(optInTimePeriod, optInStdDevPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('RVIR', PARAM_RANGES, optInTimePeriod, optInStdDevPeriod);
  }
  const guardStart = clampedStart('RVIR', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('RVIR', 'prices.high', inHigh, guardInLen);
  requireLength('RVIR', 'prices.low', inLow, guardInLen);
  requireOutput('RVIR', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlapsPartly(outReal, inHighF64)) {
    throw sharedMemory('RVIR', 'outReal', 'prices.high');
  }
  if (overlapsPartly(outReal, inLowF64)) {
    throw sharedMemory('RVIR', 'outReal', 'prices.low');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = rvirImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInTimePeriod, optInStdDevPeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('RVIR', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link rvir}, {@link rvirLookback}, {@link rvirDisplayShift},
 * {@link rvirOpen} and {@link rvirOpenAndFill}. Every key is optional: an omitted
 * or `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface RvirOptions {
  /**
   * Wilder smoothing period applied to both legs of both indices (default 14; range
   * 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  timePeriod?: number | undefined;
  /**
   * Number of trailing values each standard deviation spans (default 10; range 2 to
   * 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  stdDevPeriod?: number | undefined;
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

/** What {@link rvir} returns, and {@link rvirOpenAndFill} with its stream. */
export interface RvirResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * The averaged index, in 0..100. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link rvir} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link RvirOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function rvirLookback(options?: RvirOptions): number {
  const o = optionsOf('RVIR lookback', options);
  return rvirLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.stdDevPeriod, Core.INTEGER_DEFAULT), coreOf('RVIR lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link rvir}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link RvirOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function rvirDisplayShift(outputIdx: number, options?: RvirOptions): number {
  const o = optionsOf('RVIR displayShift', options);
  return rvirDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.stdDevPeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('RVIR displayShift', o.core));
}

/**
 * Relative Volatility Index, refined form: Donald Dorsey's 1995 revision of his
 * own indicator, which runs the 1993 RVI over the daily highs and again over the
 * daily lows and averages the two indices. Output is bounded \[0..100] and is
 * interpreted like RVI: above 50 the highs and lows have been more volatile while
 * rising than while falling, below 50 the reverse. Dorsey's stated reason for the
 * revision is that a high and a low carry the day's range, so the pair answers the
 * question the close alone can only approximate.
 *
 * Formula and more info at
 * [ta-lib.org/functions/rvir](https://ta-lib.org/functions/rvir).
 *
 * **Notes**
 * - RVIR is the 1995 revision; [`RVI`](https://ta-lib.org/functions/rvi) is the
 *   1993 version.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link rvirLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `high`: High price of
 *   each bar. `low`: Low price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link RvirOptions}.
 * @returns `begIdx` and the values; see {@link RvirResult}. Each output is a view
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
 * import { rvir } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = rvir({ high, low }, {
 *   timePeriod: 14,
 *   stdDevPeriod: 10,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`rvi`](https://ta-lib.org/functions/rvi)
 * @see [`stddev`](https://ta-lib.org/functions/stddev)
 * @see [`atr`](https://ta-lib.org/functions/atr)
 */
export function rvir(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: RvirOptions): RvirResult {
  requireArgument('RVIR', 'prices', prices);
  const o = optionsOf('RVIR', options);
  if (o.endIdx === undefined) {
    requireSeries('RVIR', 'prices.high', prices.high);
    requireSeries('RVIR', 'prices.low', prices.low);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInStdDevPeriod = orDefault(o.stdDevPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('RVIR', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, rvirLookbackPositional(optInTimePeriod, optInStdDevPeriod, core), prices.high, prices.low);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = rvirPositional(startIdx, endIdx, prices.high, prices.low, optInTimePeriod, optInStdDevPeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live RVIR stream: one value per closed bar, bit-identical to {@link rvir} over
 * the same series. Open it with {@link rvirOpen} or {@link rvirOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/rvir](https://ta-lib.org/functions/rvir).
 */
export class RvirStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  optInStdDevPeriod: number = 0;
  /** @internal */
  cur_outReal: number = 0;
  /** @internal */
  sub0!: RviStream;
  /** @internal */
  sub1!: RviStream;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): RvirStream {
    return new RvirStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link rvir} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('RVIR advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(high: number, low: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('RVIR update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(high) || !Number.isFinite(low)) {
      throw nonFiniteBar('RVIR update', !Number.isFinite(high) ? 'high' : 'low');
    }
    rvirStepImpl(this, high, low);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(high: number, low: number): number {
    if (!Number.isFinite(high) || !Number.isFinite(low)) {
      throw nonFiniteBar('RVIR peek', !Number.isFinite(high) ? 'high' : 'low');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
    let cur_tempHigh = 0;
    let cur_outReal = 0;
    cur_tempHigh = sp.sub0.peek(inHigh);
    cur_outReal = sp.sub1.peek(inLow);
    cur_outReal = 0.5 * (cur_tempHigh + cur_outReal);
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
  clone(): RvirStream {
    const c = new RvirStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.optInStdDevPeriod = this.optInStdDevPeriod;
    c.cur_outReal = this.cur_outReal;
    c.sub0 = this.sub0.clone();
    c.sub1 = this.sub1.clone();
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function rvirStepImpl(sp: RvirStream, inHigh: number, inLow: number): void {
  let cur_tempHigh = 0;
  let cur_outReal = 0;
  cur_tempHigh = sp.sub0.update(inHigh);
  cur_outReal = sp.sub1.update(inLow);
  cur_outReal = 0.5 * (cur_tempHigh + cur_outReal);
  sp.cur_outReal = cur_outReal;
}

function rvirOpenImpl(
  sp: RvirStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInStdDevPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let tempHigh!: Float64Array;
  let i = 0;
  let lookbackTotal = 0;
  const tempBegIdx = new MInteger();
  const tempNbElement = new MInteger();
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInStdDevPeriod === Core.INTEGER_DEFAULT) {
    optInStdDevPeriod = 10;
  } else if (optInStdDevPeriod < 2 || optInStdDevPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen < rvirLookbackPositional(optInTimePeriod, optInStdDevPeriod, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const sc_outReal = outStride === 1 ? outReal : new Float64Array(historyLen);
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = rvirLookbackPositional(optInTimePeriod, optInStdDevPeriod, core);
  /* Nothing is allocated and no input is read when the range cannot produce a
   * value, so a caller-supplied input that stops short of endIdx is never
   * read past its end (kc.c).
   */
  if (lookbackTotal > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Each leg is the shipped TA_RVI over this range, entered at the same
   * startIdx: same function, same parameters, same lookback, so the two
   * outputs are aligned bar for bar with no anchor to reconcile.
   *
   * Two calls rather than one fused loop. The fused body is expressible --
   * two copies of TA_RVI's state driven by one bar cursor -- but it carries
   * two window-start cursors, and the streaming analyzer accepts exactly one
   * ("extrema automaton: expected exactly one window-start variable"), so
   * fusing would cost this function the streaming tier. Composing keeps it,
   * for the reason KC's legs do: each leg streams as itself.
   */
  tempHigh = new Float64Array(endIdx - startIdx + 1);
  /* Either input may be aliased onto outReal. Both legs are safe against that
   * for TA_RVI's own reason -- its write index trails its read index by the
   * lookback and never overtakes it -- so the order of the two calls is an
   * implementation detail, not what makes the aliasing safe. Swapping them
   * leaves every value identical, which is why the aliasing test earns its
   * keep against the scratch buffer's extent rather than against this order.
   */
  const sub0 = rviOpenAndFillInternal(core, inHigh, startIdx, optInTimePeriod, optInStdDevPeriod, tempBegIdx, tempNbElement, tempHigh);
  const sub1 = rviOpenAndFillInternal(core, inLow, startIdx, optInTimePeriod, optInStdDevPeriod, outBegIdx, outNBElement, sc_outReal);
  /* Each leg has already resolved its own zero-total case to TA_RVI's neutral
   * 50 before the average is taken, so a tie in one series never drags the
   * other. Scaling by one half is exact, so the spelling of the average is
   * not a variant: 0.5*(a+b) is one rounding of a+b, as are (a+b)/2 and
   * 0.5*a + 0.5*b.
   */
  for (i = 0; i < outNBElement.value; i += 1) {
    sc_outReal[i] = 0.5 * (tempHigh[i] + sc_outReal[i]);
  }
  if (outNBElement.value < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.optInStdDevPeriod = optInStdDevPeriod;
  sp.sub0 = sub0;
  sp.sub1 = sub1;
  sp.cur_outReal = sc_outReal[outNBElement.value - 1];
  return RetCode.SUCCESS;
}

/** @internal */
export function rvirOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInStdDevPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RvirStream {
  const sp = RvirStream.of(core);
  const retCode = rvirOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, optInStdDevPeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('RVIR openAndFill', inHigh.length, startIdx, rvirLookbackPositional(optInTimePeriod, optInStdDevPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && rvirLookbackPositional(optInTimePeriod, optInStdDevPeriod, core) < 0) {
    throw rejectedParams('RVIR openAndFill', PARAM_RANGES, optInTimePeriod, optInStdDevPeriod);
  }
  throw streamFailure('RVIR openAndFill', retCode);
}

/** @internal */
export function rvirOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInStdDevPeriod: number,
): RvirStream {
  const sp = RvirStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = rvirOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, optInStdDevPeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('RVIR open', inHigh.length, startIdx, rvirLookbackPositional(optInTimePeriod, optInStdDevPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && rvirLookbackPositional(optInTimePeriod, optInStdDevPeriod, core) < 0) {
    throw rejectedParams('RVIR open', PARAM_RANGES, optInTimePeriod, optInStdDevPeriod);
  }
  throw streamFailure('RVIR open', retCode);
}

/** @internal */
export function rvirOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  optInStdDevPeriod: number,
  core: Core = Core.DEFAULT,
): RvirStream {
  optInTimePeriod = requireInt32('RVIR open', 'timePeriod', optInTimePeriod);
  optInStdDevPeriod = requireInt32('RVIR open', 'stdDevPeriod', optInStdDevPeriod);
  requireArgument('RVIR open', 'prices.high', inHigh);
  requireHistory('RVIR open', 'prices.high', inHigh.length);
  requireArgument('RVIR open', 'prices.low', inLow);
  requireHistoryLength('RVIR open', 'prices.low', inLow.length, inHigh.length);
  return rvirOpenInternal(core, float64(inHigh), float64(inLow), 0, optInTimePeriod, optInStdDevPeriod);
}

/** @internal */
export function rvirOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  optInStdDevPeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): RvirStream {
  optInTimePeriod = requireInt32('RVIR openAndFill', 'timePeriod', optInTimePeriod);
  optInStdDevPeriod = requireInt32('RVIR openAndFill', 'stdDevPeriod', optInStdDevPeriod);
  requireArgument('RVIR openAndFill', 'prices.high', inHigh);
  requireHistory('RVIR openAndFill', 'prices.high', inHigh.length);
  requireArgument('RVIR openAndFill', 'prices.low', inLow);
  const guardLookback = rvirLookbackPositional(optInTimePeriod, optInStdDevPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('RVIR openAndFill', PARAM_RANGES, optInTimePeriod, optInStdDevPeriod);
  }
  const guardOutLen = openFillCount('RVIR openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('RVIR openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireOutput('RVIR openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('RVIR openAndFill', 'outReal', 'prices.high');
  }
  if (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('RVIR openAndFill', 'outReal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outReal, inHighF64)) {
    throw sharedMemory('RVIR openAndFill', 'outReal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outReal, inLowF64)) {
    throw sharedMemory('RVIR openAndFill', 'outReal', 'prices.low');
  }
  return rvirOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInTimePeriod, optInStdDevPeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live RVIR stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link rvir} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link RvirOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `rvirLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or
 *   histories of different lengths.
 *
 * @example
 * ```ts
 * import { rvirOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = rvirOpen({ high, low }, { timePeriod: 14, stdDevPeriod: 10 });
 * const peeked = stream.peek(101.4, 99.1); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1); // the bar closes
 * ```
 */
export function rvirOpen(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: RvirOptions): RvirStream {
  requireArgument('RVIR open', 'prices', prices);
  const o = optionsOf('RVIR open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInStdDevPeriod = orDefault(o.stdDevPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('RVIR open', o.core);
  return rvirOpenPositional(prices.high, prices.low, optInTimePeriod, optInStdDevPeriod, core);
}

/**
 * {@link rvirOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link rvir}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link RvirOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link rvir} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `rvirLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { rvirOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = rvirOpenAndFill({ high, low }, {
 *   timePeriod: 14,
 *   stdDevPeriod: 10,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function rvirOpenAndFill(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: RvirOptions): RvirResult & { stream: RvirStream } {
  requireArgument('RVIR openAndFill', 'prices', prices);
  const o = optionsOf('RVIR openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInStdDevPeriod = orDefault(o.stdDevPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('RVIR openAndFill', o.core);
  const capacity = fillCapacity(rvirLookbackPositional(optInTimePeriod, optInStdDevPeriod, core), prices.high, prices.low);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = rvirOpenAndFillPositional(prices.high, prices.low, optInTimePeriod, optInStdDevPeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
