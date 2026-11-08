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
 *  090426 MF,CC  First version (issue #348).
 */

import { FuncUnstId } from '../enums.ts';
import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function rmaLookbackPositional(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  return optInTimePeriod - 1 + core.unstableCount(FuncUnstId.RMA, ((optInTimePeriod > 1) ? (((((10 * (((2 * optInTimePeriod) | 0) - 1)) | 0) + 1) / 2) | 0) : 0), ((optInTimePeriod > 1) ? (((((19 * (((2 * optInTimePeriod) | 0) - 1)) | 0) + 1) / 2) | 0) : 0));
}

/** @internal */
export function rmaDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (rmaLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function rmaImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let i = 0;
  let outIdx = 0;
  let today = 0;
  let lookbackTotal = 0;
  let nbRMA = 0;
  let prevRMA = 0;
  let periodTotal = 0;
  let wAlpha = 0;
  let wBeta = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  /* Adjust startIdx to account for the lookback period. */
  lookbackTotal = rmaLookbackPositional(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  /* wAlpha is derived FROM wBeta, never the reverse: only that order makes
   * wAlpha + wBeta exactly 1 (Sterbenz -- wBeta lands in [0.5, 1)), and it
   * measures closer to the exact recursion than the 1/period-first spelling
   * at nearly every period. The order is a gated contract, not a preference:
   * TA_RMA(TA_TRANGE(h,l,c),n) must equal TA_ATR(n) bit for bit.
   * The pair is exactly (1, 0) at period 1 -- hence no period-1 arm.
   */
  wBeta = (optInTimePeriod - 1) / optInTimePeriod;
  wAlpha = 1.0 - wBeta;
  /* The arithmetic order is the rest of that contract: the seed accumulates
   * from 0.0 in input order, and the smoothing step stays ONE statement --
   * splitting it unfuses the multiply-add and puts a second latency on the
   * recurrence's dependency chain.
   *
   * In-place (outReal being inReal) is supported: the output index never
   * passes the bar index of any remaining read.
   */
  today = startIdx - lookbackTotal;
  /* Seed with a simple average of the first 'period' values. */
  periodTotal = 0.0;
  i = optInTimePeriod;
  while (i-- > 0) {
    periodTotal += inReal[today];
    today += 1;
  }
  prevRMA = periodTotal / optInTimePeriod;
  /* Skip the unstable period. */
  i = lookbackTotal - (optInTimePeriod - 1);
  while (i !== 0) {
    prevRMA = fma(wBeta, prevRMA, wAlpha * inReal[today]);
    today += 1;
    i -= 1;
  }
  /* Now start to write the final RMA in the caller
   * provided outReal.
   */
  outIdx = 1;
  outReal[0] = prevRMA;
  /* Now do the number of requested RMA. */
  nbRMA = endIdx - startIdx + 1;
  while (--nbRMA !== 0) {
    prevRMA = fma(wBeta, prevRMA, wAlpha * inReal[today]);
    outReal[outIdx++] = prevRMA;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000']] as const;

/** @internal */
export function rmaPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('RMA', 'startIdx', startIdx);
  endIdx = requireInt32('RMA', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('RMA', 'timePeriod', optInTimePeriod);
  requireIndexRange('RMA', startIdx, endIdx);
  const guardLookback = rmaLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('RMA', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('RMA', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('RMA', 'real', inReal, guardInLen);
  requireOutput('RMA', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('RMA', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = rmaImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('RMA', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link rma}, {@link rmaLookback}, {@link rmaDisplayShift},
 * {@link rmaOpen} and {@link rmaOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface RmaOptions {
  /**
   * Number of bars in the seed window, and the reciprocal of the smoothing factor
   * (default 30; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
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

/** What {@link rma} returns, and {@link rmaOpenAndFill} with its stream. */
export interface RmaResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Wilder's smoothed moving average of the input. `values[k]` is the value at input
   * bar `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link rma} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link RmaOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function rmaLookback(options?: RmaOptions): number {
  const o = optionsOf('RMA lookback', options);
  return rmaLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('RMA lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link rma}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link RmaOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function rmaDisplayShift(outputIdx: number, options?: RmaOptions): number {
  const o = optionsOf('RMA displayShift', options);
  return rmaDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('RMA displayShift', o.core));
}

/**
 * Wilder's smoothed moving average: an exponential average whose smoothing factor
 * is the reciprocal of the period rather than the `2/(n+1)` of a classic EMA,
 * seeded with a simple average of the first window. J. Welles Wilder Jr.
 * introduced it in 1978 as the smoothing inside RSI, ATR and the
 * directional-movement family; this exposes it as a moving average in its own
 * right. Because the smoothing factor is smaller than an EMA's at the same period,
 * RMA reacts more slowly and gives noticeably more weight to old data: it takes
 * about twice the period to shed the influence of a bar. Read it as a slow trend
 * line — direction and slope matter, individual crossings much less than on a
 * faster average. RMA is also selectable as a moving-average type (`MAType.RMA`)
 * wherever an `maType` parameter is accepted
 * ([`MA`](https://ta-lib.org/functions/ma),
 * [`BBANDS`](https://ta-lib.org/functions/bbands),
 * [`STOCH`](https://ta-lib.org/functions/stoch),
 * [`MACDEXT`](https://ta-lib.org/functions/macdext), ...). It travels under five
 * names for one object: RMA (TradingView, pandas-ta), SMMA (MetaTrader), Wilder's
 * Smoothing or Wilder's Average (thinkorswim), `wilders` (Tulip), WilderMA
 * (Wealth-Lab).
 *
 * Formula and more info at
 * [ta-lib.org/functions/rma](https://ta-lib.org/functions/rma).
 *
 * **Notes**
 * - Wilder's own writing uses a period of 14, and pandas-ta defaults to 10. The
 *   default here is the one the rest of the moving-average family carries, so a
 *   call that swaps one MA for another keeps its period.
 * - The smoothing factor being `1/N` is sometimes quoted as "an RMA of N is an EMA
 *   of 2N-1". The factors really are identical, since `2/((2N-1)+1)` is `1/N`, but
 *   the two seed over different windows: the series differ through the warm-up and
 *   only converge as the seed's influence decays.
 * - `rma` over `trange` is `atr`, bit for bit. The recurrence is spelled here
 *   exactly as ATR spells it.
 * - The recurrence is the `alpha * x + (1 - alpha) * prev` form, which is
 *   TradingView Pine's `ta.rma` and pandas' `ewm(adjust=False)` kernel.
 *   Implementations that spell it `prev + (x - prev) * alpha` or
 *   `(prev * (N-1) + x) / N` are algebraically the same average and differ from
 *   this one only at the last bits.
 * - Being recursive, an output depends on how much history precedes it: the same
 *   bar computed from an earlier start differs while the seed still carries
 *   weight, and that difference decays by a factor of `1 - 1/N` per bar. The
 *   unstable period is how much of the warm-up to discard.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link rmaLookback} is a success with empty outputs.
 *
 * @param real - Data on which to compute the average.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link RmaOptions}.
 * @returns `begIdx` and the values; see {@link RmaResult}. Each output is a view
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
 * import { rma } from 'ta-lib';
 *
 * const series = new Float64Array(252); // your data, oldest first
 *
 * const { begIdx, values } = rma(series, { timePeriod: 30 });
 * console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
 * ```
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`sma`](https://ta-lib.org/functions/sma)
 * @see [`atr`](https://ta-lib.org/functions/atr)
 * @see [`rsi`](https://ta-lib.org/functions/rsi)
 * @see [`dema`](https://ta-lib.org/functions/dema)
 */
export function rma(real: ArrayLike<number>, options?: RmaOptions): RmaResult {
  const o = optionsOf('RMA', options);
  if (o.endIdx === undefined) {
    requireSeries('RMA', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('RMA', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, rmaLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = rmaPositional(startIdx, endIdx, real, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live RMA stream: one value per closed bar, bit-identical to {@link rma} over
 * the same series. Open it with {@link rmaOpen} or {@link rmaOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/rma](https://ta-lib.org/functions/rma).
 */
export class RmaStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  prevRMA: number = 0;
  /** @internal */
  wAlpha: number = 0;
  /** @internal */
  wBeta: number = 0;
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
  static of(core: Core): RmaStream {
    return new RmaStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link rma} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('RMA advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: Data on which to compute the average.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('RMA update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('RMA update', 'real');
    }
    rmaStepImpl(this, real);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: Data on which to compute the average.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number): number {
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('RMA peek', 'real');
    }
    const sp = this;
    const inReal = real;
    let cur_outReal = 0;
    let prevRMA = sp.prevRMA;
    prevRMA = fma(sp.wBeta, prevRMA, sp.wAlpha * inReal);
    cur_outReal = prevRMA;
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
  clone(): RmaStream {
    const c = new RmaStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.prevRMA = this.prevRMA;
    c.wAlpha = this.wAlpha;
    c.wBeta = this.wBeta;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function rmaStepImpl(sp: RmaStream, inReal: number): void {
  sp.prevRMA = fma(sp.wBeta, sp.prevRMA, sp.wAlpha * inReal);
  sp.cur_outReal = sp.prevRMA;
}

function rmaOpenImpl(
  sp: RmaStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let i = 0;
  let outIdx = 0;
  let today = 0;
  let lookbackTotal = 0;
  let nbRMA = 0;
  let prevRMA = 0;
  let periodTotal = 0;
  let wAlpha = 0;
  let wBeta = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  /* Adjust startIdx to account for the lookback period. */
  lookbackTotal = rmaLookbackPositional(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* wAlpha is derived FROM wBeta, never the reverse: only that order makes
   * wAlpha + wBeta exactly 1 (Sterbenz -- wBeta lands in [0.5, 1)), and it
   * measures closer to the exact recursion than the 1/period-first spelling
   * at nearly every period. The order is a gated contract, not a preference:
   * TA_RMA(TA_TRANGE(h,l,c),n) must equal TA_ATR(n) bit for bit.
   * The pair is exactly (1, 0) at period 1 -- hence no period-1 arm.
   */
  wBeta = (optInTimePeriod - 1) / optInTimePeriod;
  wAlpha = 1.0 - wBeta;
  /* The arithmetic order is the rest of that contract: the seed accumulates
   * from 0.0 in input order, and the smoothing step stays ONE statement --
   * splitting it unfuses the multiply-add and puts a second latency on the
   * recurrence's dependency chain.
   *
   * In-place (outReal being inReal) is supported: the output index never
   * passes the bar index of any remaining read.
   */
  today = startIdx - lookbackTotal;
  /* Seed with a simple average of the first 'period' values. */
  periodTotal = 0.0;
  i = optInTimePeriod;
  while (i-- > 0) {
    periodTotal += inReal[today];
    today += 1;
  }
  prevRMA = periodTotal / optInTimePeriod;
  /* Skip the unstable period. */
  i = lookbackTotal - (optInTimePeriod - 1);
  while (i !== 0) {
    prevRMA = fma(wBeta, prevRMA, wAlpha * inReal[today]);
    today += 1;
    i -= 1;
  }
  /* Now start to write the final RMA in the caller
   * provided outReal.
   */
  outIdx = 1;
  outReal[((0 * outStride) | 0)] = prevRMA;
  /* Now do the number of requested RMA. */
  nbRMA = endIdx - startIdx + 1;
  while (--nbRMA !== 0) {
    prevRMA = fma(wBeta, prevRMA, wAlpha * inReal[today]);
    outReal[((outIdx++ * outStride) | 0)] = prevRMA;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  sp.optInTimePeriod = optInTimePeriod;
  sp.prevRMA = prevRMA;
  sp.wAlpha = wAlpha;
  sp.wBeta = wBeta;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function rmaOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RmaStream {
  const sp = RmaStream.of(core);
  const retCode = rmaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('RMA openAndFill', inReal.length, startIdx, rmaLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && rmaLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('RMA openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('RMA openAndFill', retCode);
}

/** @internal */
export function rmaOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): RmaStream {
  const sp = RmaStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = rmaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('RMA open', inReal.length, startIdx, rmaLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && rmaLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('RMA open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('RMA open', retCode);
}

/** @internal */
export function rmaOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): RmaStream {
  optInTimePeriod = requireInt32('RMA open', 'timePeriod', optInTimePeriod);
  requireArgument('RMA open', 'real', inReal);
  requireHistory('RMA open', 'real', inReal.length);
  return rmaOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/** @internal */
export function rmaOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): RmaStream {
  optInTimePeriod = requireInt32('RMA openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('RMA openAndFill', 'real', inReal);
  requireHistory('RMA openAndFill', 'real', inReal.length);
  const guardLookback = rmaLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('RMA openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('RMA openAndFill', inReal.length, guardLookback);
  requireOutput('RMA openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('RMA openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('RMA openAndFill', 'outReal', 'real');
  }
  return rmaOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live RMA stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link rma} there.
 *
 * @param real - Warm-up history. Data on which to compute the average.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link RmaOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `rmaLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { rmaOpen } from 'ta-lib';
 *
 * const series = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = rmaOpen(series, { timePeriod: 30 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function rmaOpen(real: ArrayLike<number>, options?: RmaOptions): RmaStream {
  const o = optionsOf('RMA open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('RMA open', o.core);
  return rmaOpenPositional(real, optInTimePeriod, core);
}

/**
 * {@link rmaOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link rma}.
 *
 * @param real - Warm-up history. Data on which to compute the average.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link RmaOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link rma} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `rmaLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or a
 *   buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { rmaOpenAndFill } from 'ta-lib';
 *
 * const series = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = rmaOpenAndFill(series, { timePeriod: 30 });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function rmaOpenAndFill(real: ArrayLike<number>, options?: RmaOptions): RmaResult & { stream: RmaStream } {
  const o = optionsOf('RMA openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('RMA openAndFill', o.core);
  const capacity = fillCapacity(rmaLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = rmaOpenAndFillPositional(real, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/** @internal */
export function rmaStepTape(sp: RmaStream, _tape: Float64Array, _tapeBase: number, _tapeMask: number, inReal: number): number {
  rmaStepImpl(sp, inReal);
  sp.outRangeCount++;
  return sp.cur_outReal;
}

/** @internal */
export function rmaPeekTape(sp: RmaStream, _tape: Float64Array, _tapeBase: number, _tapeMask: number, inReal: number): number {
  return sp.peek(inReal);
}

/** @internal */
export function rmaTapeDetach(_sp: RmaStream): number {
  return 0;
}
