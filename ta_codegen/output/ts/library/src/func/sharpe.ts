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
 *  110626 MF,CC  Initial version (#571).
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireReal, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function sharpeLookbackPositional(optInTimePeriod: number, optInRiskFreeRate: number, optInAnnualization: number, _core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (typeof optInRiskFreeRate !== 'number') {
    return -1;
  }
  if (typeof optInAnnualization !== 'number') {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  if (optInRiskFreeRate === Core.REAL_DEFAULT) {
    optInRiskFreeRate = 0.0;
  } else if (!(optInRiskFreeRate >= Core.REAL_MIN && optInRiskFreeRate <= Core.REAL_MAX)) {
    return -1;
  }
  if (optInAnnualization === Core.REAL_DEFAULT) {
    optInAnnualization = 252.0;
  } else if (!(optInAnnualization >= 0e0 && optInAnnualization <= Core.REAL_MAX)) {
    return -1;
  }
  return optInTimePeriod;
}

/** @internal */
export function sharpeDisplayShiftPositional(optInTimePeriod: number, optInRiskFreeRate: number, optInAnnualization: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (sharpeLookbackPositional(optInTimePeriod, optInRiskFreeRate, optInAnnualization, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function sharpeImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  optInRiskFreeRate: number,
  optInAnnualization: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let shift = 0;
  let periodTotal1 = 0;
  let periodTotal2 = 0;
  let peakTotal2 = 0;
  let price = 0;
  let prevPrice = 0;
  let trailingPrevPrice = 0;
  let windowPrevPrice = 0;
  let ret = 0;
  let dev = 0;
  let dPeriod = 0;
  let invPeriod = 0;
  let meanDev = 0;
  let moment2 = 0;
  let excess = 0;
  let sqrtAnnual = 0;
  let ratio = 0;
  let i = 0;
  let j = 0;
  let outIdx = 0;
  let trailingIdx = 0;
  let windowStart = 0;
  let barsSinceReseed = 0;
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
  if (optInRiskFreeRate === Core.REAL_DEFAULT) {
    optInRiskFreeRate = 0.0;
  } else if (!(optInRiskFreeRate >= Core.REAL_MIN && optInRiskFreeRate <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInAnnualization === Core.REAL_DEFAULT) {
    optInAnnualization = 252.0;
  } else if (!(optInAnnualization >= 0e0 && optInAnnualization <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx < optInTimePeriod) {
    startIdx = optInTimePeriod;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  dPeriod = optInTimePeriod;
  invPeriod = 1.0 / dPeriod;
  sqrtAnnual = Math.sqrt(optInAnnualization);
  /* The window holds the returns of the bars after trailingIdx up to i; a
   * bar's return reads the price before it.
   *
   * The variance of the excess returns is the variance of the returns, so
   * the risk-free rate enters only the mean. That keeps the sums at return
   * scale whatever the rate, and lets a flat price window reach an exact
   * zero spread.
   */
  trailingIdx = startIdx - optInTimePeriod;
  prevPrice = inReal[trailingIdx];
  /* A return needs a non-zero base price and nothing more: the test is
   * exact, never a band, since a price carries the quote unit. A zero base
   * gives a zero return.
   */
  price = inReal[trailingIdx + 1];
  ret = 0.0;
  if (prevPrice !== 0.0) {
    ret = (price - prevPrice) / prevPrice;
  }
  shift = ret;
  periodTotal1 = 0.0;
  periodTotal2 = 0.0;
  for (j = trailingIdx + 1; j < startIdx; j += 1) {
    price = inReal[j];
    ret = 0.0;
    if (prevPrice !== 0.0) {
      ret = (price - prevPrice) / prevPrice;
    }
    prevPrice = price;
    dev = ret - shift;
    periodTotal1 += dev;
    periodTotal2 += dev * dev;
  }
  i = startIdx;
  outIdx = 0;
  barsSinceReseed = ((32 * optInTimePeriod) | 0);
  peakTotal2 = periodTotal2;
  do {
    price = inReal[i];
    ret = 0.0;
    if (prevPrice !== 0.0) {
      ret = (price - prevPrice) / prevPrice;
    }
    prevPrice = price;
    dev = ret - shift;
    periodTotal1 += dev;
    periodTotal2 += dev * dev;
    peakTotal2 = (periodTotal2 > peakTotal2) ? periodTotal2 : peakTotal2;
    meanDev = periodTotal1 * invPeriod;
    moment2 = periodTotal2 - meanDev * periodTotal1;
    /* inReal and outReal may be the same buffer, and the trailing prices
     * sit in slots the output reaches: every read of the window happens
     * before this bar's write. The oldest price read is the trailing
     * return's base, so the index stays on it.
     */
    trailingPrevPrice = inReal[trailingIdx];
    trailingIdx += 1;
    price = inReal[trailingIdx];
    ret = 0.0;
    if (trailingPrevPrice !== 0.0) {
      ret = (price - trailingPrevPrice) / trailingPrevPrice;
    }
    dev = ret - shift;
    periodTotal1 -= dev;
    periodTotal2 -= dev * dev;
    /* Rebuild against the window mean when the spread has shrunk below 1% of
     * the largest sum of squares held since the last rebuild, or at least
     * every 32 windows. The rounding the running sums carry scales with that
     * peak, so once a large return leaves, the current sum can be all
     * rounding; a flat run after a move must still come out exactly flat.
     * The spread is a divisor, so its relative error reaches the ratio.
     */
    barsSinceReseed -= 1;
    if (moment2 < 0.01 * peakTotal2 || barsSinceReseed <= 0) {
      barsSinceReseed = ((32 * optInTimePeriod) | 0);
      windowStart = i - optInTimePeriod + 1;
      windowPrevPrice = inReal[windowStart - 1];
      dev = 0.0;
      for (j = windowStart; j <= i; j += 1) {
        price = inReal[j];
        ret = 0.0;
        if (windowPrevPrice !== 0.0) {
          ret = (price - windowPrevPrice) / windowPrevPrice;
        }
        windowPrevPrice = price;
        dev += ret;
      }
      shift = dev * invPeriod;
      windowPrevPrice = inReal[windowStart - 1];
      periodTotal1 = 0.0;
      periodTotal2 = 0.0;
      for (j = windowStart; j <= i; j += 1) {
        price = inReal[j];
        ret = 0.0;
        if (windowPrevPrice !== 0.0) {
          ret = (price - windowPrevPrice) / windowPrevPrice;
        }
        windowPrevPrice = price;
        dev = ret - shift;
        periodTotal1 += dev;
        periodTotal2 += dev * dev;
      }
      meanDev = periodTotal1 * invPeriod;
      moment2 = periodTotal2 - meanDev * periodTotal1;
      /* A window of equal returns can sit off this mean by rounding, which
       * leaves a spread of rounding where the ratio must be the plain
       * quotient, and fires the trigger again on every bar. Anchored on
       * one of its own returns, its deviations are exactly zero.
       */
      if (moment2 < 1e-6 * periodTotal2) {
        shift = ret;
        windowPrevPrice = inReal[windowStart - 1];
        periodTotal1 = 0.0;
        periodTotal2 = 0.0;
        for (j = windowStart; j <= i; j += 1) {
          price = inReal[j];
          ret = 0.0;
          if (windowPrevPrice !== 0.0) {
            ret = (price - windowPrevPrice) / windowPrevPrice;
          }
          windowPrevPrice = price;
          dev = ret - shift;
          periodTotal1 += dev;
          periodTotal2 += dev * dev;
        }
        meanDev = periodTotal1 * invPeriod;
        moment2 = periodTotal2 - meanDev * periodTotal1;
      }
      peakTotal2 = periodTotal2;
      /* Re-remove the trailing return under the new shift so the carried
       * state matches the path without a rebuild.
       */
      price = inReal[windowStart];
      ret = 0.0;
      if (inReal[windowStart - 1] !== 0.0) {
        ret = (price - inReal[windowStart - 1]) / inReal[windowStart - 1];
      }
      dev = ret - shift;
      periodTotal1 -= dev;
      periodTotal2 -= dev * dev;
    }
    /* No spread is the plain quotient: +/-Inf by the sign of the excess
     * mean, NaN when it is exactly zero (ruled on #571). Annualize the
     * per-bar ratio last, so an annualization of 1 is that ratio bit for
     * bit.
     */
    excess = shift + meanDev - optInRiskFreeRate;
    ratio = excess / Math.sqrt(moment2 / (dPeriod - 1.0));
    outReal[outIdx++] = sqrtAnnual * ratio;
    i += 1;
  } while (i <= endIdx);
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['riskFreeRate', Core.REAL_DEFAULT, Core.REAL_MIN, Core.REAL_MAX, 'in -3e37..3e37'], ['annualization', Core.REAL_DEFAULT, 0e0, Core.REAL_MAX, 'in 0..3e37']] as const;

/** @internal */
export function sharpePositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInRiskFreeRate: number,
  optInAnnualization: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('SHARPE', 'startIdx', startIdx);
  endIdx = requireInt32('SHARPE', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('SHARPE', 'timePeriod', optInTimePeriod);
  optInRiskFreeRate = requireReal('SHARPE', 'riskFreeRate', optInRiskFreeRate);
  optInAnnualization = requireReal('SHARPE', 'annualization', optInAnnualization);
  requireIndexRange('SHARPE', startIdx, endIdx);
  const guardLookback = sharpeLookbackPositional(optInTimePeriod, optInRiskFreeRate, optInAnnualization, core);
  if (guardLookback < 0) {
    throw rejectedParams('SHARPE', PARAM_RANGES, optInTimePeriod, optInRiskFreeRate, optInAnnualization);
  }
  const guardStart = clampedStart('SHARPE', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('SHARPE', 'real', inReal, guardInLen);
  requireOutput('SHARPE', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('SHARPE', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = sharpeImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, optInRiskFreeRate, optInAnnualization, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('SHARPE', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link sharpe}, {@link sharpeLookback}, {@link sharpeDisplayShift},
 * {@link sharpeOpen} and {@link sharpeOpenAndFill}. Every key is optional: an
 * omitted or `undefined` key takes its default, and a call ignores the keys it has
 * no use for.
 */
export interface SharpeOptions {
  /**
   * Number of returns in the window; one price more is read (default 20; range 2 to
   * 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  timePeriod?: number | undefined;
  /**
   * Risk-free return per bar, subtracted from each return (default 0;
   * `Core.REAL_DEFAULT` selects the default).
   */
  riskFreeRate?: number | undefined;
  /**
   * Bars per year; the per-bar ratio is multiplied by its square root (default 252;
   * minimum 0; `Core.REAL_DEFAULT` selects the default).
   */
  annualization?: number | undefined;
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

/** What {@link sharpe} returns, and {@link sharpeOpenAndFill} with its stream. */
export interface SharpeResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Annualized Sharpe ratio of the trailing window, or ±Inf or NaN where the window
   * has no spread. `values[k]` is the value at input bar `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link sharpe} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SharpeOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function sharpeLookback(options?: SharpeOptions): number {
  const o = optionsOf('SHARPE lookback', options);
  return sharpeLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.riskFreeRate, Core.REAL_DEFAULT), orDefault(o.annualization, Core.REAL_DEFAULT), coreOf('SHARPE lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link sharpe}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SharpeOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function sharpeDisplayShift(outputIdx: number, options?: SharpeOptions): number {
  const o = optionsOf('SHARPE displayShift', options);
  return sharpeDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.riskFreeRate, Core.REAL_DEFAULT), orDefault(o.annualization, Core.REAL_DEFAULT), outputIdx, coreOf('SHARPE displayShift', o.core));
}

/**
 * The rolling Sharpe ratio: the mean return in excess of a risk-free rate, divided
 * by the standard deviation of that excess return, over a trailing window. A
 * drift-over-volatility measure of the series itself, scaled to a year. Above 0
 * the window's returns beat the risk-free rate on average; the larger the value,
 * the more return per unit of volatility.
 *
 * Formula and more info at
 * [ta-lib.org/functions/sharpe](https://ta-lib.org/functions/sharpe).
 *
 * **Notes**
 * - This is Sharpe's 1994 *ex post* ratio with a constant benchmark, over simple
 *   returns and the sample (`n−1`) standard deviation.
 * - A window whose returns are all equal has no spread: the result is +Inf or -Inf
 *   by the sign of the mean excess return, and NaN when that mean is exactly 0.
 *   With an annualization of 0 it is NaN there and 0 elsewhere.
 * - The risk-free rate is per bar, in the same unit as the returns. For an annual
 *   rate `R`, pass `(1+R)^(1/A) − 1`, where `A` is the annualization; it is not
 *   de-annualized for you.
 * - An annualization of 1 gives the raw per-bar ratio.
 * - A zero price leaves no return to measure from it: the return of the bar after
 *   it counts as 0.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link sharpeLookback} is a success with empty outputs.
 *
 * @param real - The price series; its returns are computed internally.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link SharpeOptions}.
 * @returns `begIdx` and the values; see {@link SharpeResult}. Each output is a
 *   view of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, an integer parameter that is not an int32, a real parameter that is not a
 *   number, a parameter outside its range, an input too short for the range, a
 *   buffer that is empty, too short or of the wrong type or a buffer sharing memory
 *   with a `Float64Array` input it is not. Checked before anything is written.
 *
 * @example
 * ```ts
 * import { sharpe } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = sharpe(data, {
 *   timePeriod: 20,
 *   riskFreeRate: 0,
 *   annualization: 252,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`stddev`](https://ta-lib.org/functions/stddev)
 * @see [`var_`](https://ta-lib.org/functions/var)
 * @see [`beta`](https://ta-lib.org/functions/beta)
 * @see [`rocp`](https://ta-lib.org/functions/rocp)
 */
export function sharpe(real: ArrayLike<number>, options?: SharpeOptions): SharpeResult {
  const o = optionsOf('SHARPE', options);
  if (o.endIdx === undefined) {
    requireSeries('SHARPE', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInRiskFreeRate = orDefault(o.riskFreeRate, Core.REAL_DEFAULT);
  const optInAnnualization = orDefault(o.annualization, Core.REAL_DEFAULT);
  const core = coreOf('SHARPE', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, sharpeLookbackPositional(optInTimePeriod, optInRiskFreeRate, optInAnnualization, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = sharpePositional(startIdx, endIdx, real, optInTimePeriod, optInRiskFreeRate, optInAnnualization, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live SHARPE stream: one value per closed bar, bit-identical to {@link sharpe}
 * over the same series. Open it with {@link sharpeOpen} or
 * {@link sharpeOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/sharpe](https://ta-lib.org/functions/sharpe).
 */
export class SharpeStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  optInRiskFreeRate: number = 0;
  /** @internal */
  optInAnnualization: number = 0;
  /** @internal */
  shift: number = 0;
  /** @internal */
  periodTotal1: number = 0;
  /** @internal */
  periodTotal2: number = 0;
  /** @internal */
  peakTotal2: number = 0;
  /** @internal */
  prevPrice: number = 0;
  /** @internal */
  dPeriod: number = 0;
  /** @internal */
  invPeriod: number = 0;
  /** @internal */
  sqrtAnnual: number = 0;
  /** @internal */
  trailingIdx: number = 0;
  /** @internal */
  barsSinceReseed: number = 0;
  /** @internal */
  j: number = 0;
  /** @internal */
  windowStart: number = 0;
  /** @internal */
  i: number = 0;
  /** @internal */
  xMask: number = 0;
  /** @internal */
  x_inReal: Float64Array = new Float64Array(1);
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
  static of(core: Core): SharpeStream {
    return new SharpeStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link sharpe} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('SHARPE advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: The price series; its returns are computed
   *   internally.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('SHARPE update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('SHARPE update', 'real');
    }
    sharpeStepImpl(this, real);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: The price series; its returns are computed
   *   internally.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number): number {
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('SHARPE peek', 'real');
    }
    const sp = this;
    const inReal = real;
    let price = 0;
    let trailingPrevPrice = 0;
    let windowPrevPrice = 0;
    let ret = 0;
    let dev = 0;
    let meanDev = 0;
    let moment2 = 0;
    let excess = 0;
    let ratio = 0;
    let barsSinceReseed = sp.barsSinceReseed;
    let cur_outReal = 0;
    let j = sp.j;
    let peakTotal2 = sp.peakTotal2;
    let periodTotal1 = sp.periodTotal1;
    let periodTotal2 = sp.periodTotal2;
    let prevPrice = sp.prevPrice;
    let shift = sp.shift;
    let trailingIdx = sp.trailingIdx;
    let windowStart = sp.windowStart;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    pkSlot0 = sp.i & sp.xMask;
    pkVal0 = inReal;
    price = ((sp.i & sp.xMask) !== pkSlot0) ? sp.x_inReal[sp.i & sp.xMask] : pkVal0;
    ret = 0.0;
    if (prevPrice !== 0.0) {
      ret = (price - prevPrice) / prevPrice;
    }
    prevPrice = price;
    dev = ret - shift;
    periodTotal1 += dev;
    periodTotal2 += dev * dev;
    peakTotal2 = (periodTotal2 > peakTotal2) ? periodTotal2 : peakTotal2;
    meanDev = periodTotal1 * sp.invPeriod;
    moment2 = periodTotal2 - meanDev * periodTotal1;
    /* inReal and outReal may be the same buffer, and the trailing prices
     * sit in slots the output reaches: every read of the window happens
     * before this bar's write. The oldest price read is the trailing
     * return's base, so the index stays on it.
     */
    trailingPrevPrice = ((trailingIdx & sp.xMask) !== pkSlot0) ? sp.x_inReal[trailingIdx & sp.xMask] : pkVal0;
    trailingIdx += 1;
    price = ((trailingIdx & sp.xMask) !== pkSlot0) ? sp.x_inReal[trailingIdx & sp.xMask] : pkVal0;
    ret = 0.0;
    if (trailingPrevPrice !== 0.0) {
      ret = (price - trailingPrevPrice) / trailingPrevPrice;
    }
    dev = ret - shift;
    periodTotal1 -= dev;
    periodTotal2 -= dev * dev;
    /* Rebuild against the window mean when the spread has shrunk below 1% of
     * the largest sum of squares held since the last rebuild, or at least
     * every 32 windows. The rounding the running sums carry scales with that
     * peak, so once a large return leaves, the current sum can be all
     * rounding; a flat run after a move must still come out exactly flat.
     * The spread is a divisor, so its relative error reaches the ratio.
     */
    barsSinceReseed -= 1;
    if (moment2 < 0.01 * peakTotal2 || barsSinceReseed <= 0) {
      barsSinceReseed = ((32 * sp.optInTimePeriod) | 0);
      windowStart = sp.i - sp.optInTimePeriod + 1;
      windowPrevPrice = (((windowStart - 1) & sp.xMask) !== pkSlot0) ? sp.x_inReal[(windowStart - 1) & sp.xMask] : pkVal0;
      dev = 0.0;
      for (j = windowStart; j <= sp.i; j += 1) {
        price = ((j & sp.xMask) !== pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0;
        ret = 0.0;
        if (windowPrevPrice !== 0.0) {
          ret = (price - windowPrevPrice) / windowPrevPrice;
        }
        windowPrevPrice = price;
        dev += ret;
      }
      shift = dev * sp.invPeriod;
      windowPrevPrice = (((windowStart - 1) & sp.xMask) !== pkSlot0) ? sp.x_inReal[(windowStart - 1) & sp.xMask] : pkVal0;
      periodTotal1 = 0.0;
      periodTotal2 = 0.0;
      for (j = windowStart; j <= sp.i; j += 1) {
        price = ((j & sp.xMask) !== pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0;
        ret = 0.0;
        if (windowPrevPrice !== 0.0) {
          ret = (price - windowPrevPrice) / windowPrevPrice;
        }
        windowPrevPrice = price;
        dev = ret - shift;
        periodTotal1 += dev;
        periodTotal2 += dev * dev;
      }
      meanDev = periodTotal1 * sp.invPeriod;
      moment2 = periodTotal2 - meanDev * periodTotal1;
      /* A window of equal returns can sit off this mean by rounding, which
       * leaves a spread of rounding where the ratio must be the plain
       * quotient, and fires the trigger again on every bar. Anchored on
       * one of its own returns, its deviations are exactly zero.
       */
      if (moment2 < 1e-6 * periodTotal2) {
        shift = ret;
        windowPrevPrice = (((windowStart - 1) & sp.xMask) !== pkSlot0) ? sp.x_inReal[(windowStart - 1) & sp.xMask] : pkVal0;
        periodTotal1 = 0.0;
        periodTotal2 = 0.0;
        for (j = windowStart; j <= sp.i; j += 1) {
          price = ((j & sp.xMask) !== pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0;
          ret = 0.0;
          if (windowPrevPrice !== 0.0) {
            ret = (price - windowPrevPrice) / windowPrevPrice;
          }
          windowPrevPrice = price;
          dev = ret - shift;
          periodTotal1 += dev;
          periodTotal2 += dev * dev;
        }
        meanDev = periodTotal1 * sp.invPeriod;
        moment2 = periodTotal2 - meanDev * periodTotal1;
      }
      peakTotal2 = periodTotal2;
      /* Re-remove the trailing return under the new shift so the carried
       * state matches the path without a rebuild.
       */
      price = ((windowStart & sp.xMask) !== pkSlot0) ? sp.x_inReal[windowStart & sp.xMask] : pkVal0;
      ret = 0.0;
      if (((((windowStart - 1) & sp.xMask) !== pkSlot0) ? sp.x_inReal[(windowStart - 1) & sp.xMask] : pkVal0) !== 0.0) {
        ret = (price - ((((windowStart - 1) & sp.xMask) !== pkSlot0) ? sp.x_inReal[(windowStart - 1) & sp.xMask] : pkVal0)) / ((((windowStart - 1) & sp.xMask) !== pkSlot0) ? sp.x_inReal[(windowStart - 1) & sp.xMask] : pkVal0);
      }
      dev = ret - shift;
      periodTotal1 -= dev;
      periodTotal2 -= dev * dev;
    }
    /* No spread is the plain quotient: +/-Inf by the sign of the excess
     * mean, NaN when it is exactly zero (ruled on #571). Annualize the
     * per-bar ratio last, so an annualization of 1 is that ratio bit for
     * bit.
     */
    excess = shift + meanDev - sp.optInRiskFreeRate;
    ratio = excess / Math.sqrt(moment2 / (sp.dPeriod - 1.0));
    cur_outReal = sp.sqrtAnnual * ratio;
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
  clone(): SharpeStream {
    const c = new SharpeStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.optInRiskFreeRate = this.optInRiskFreeRate;
    c.optInAnnualization = this.optInAnnualization;
    c.shift = this.shift;
    c.periodTotal1 = this.periodTotal1;
    c.periodTotal2 = this.periodTotal2;
    c.peakTotal2 = this.peakTotal2;
    c.prevPrice = this.prevPrice;
    c.dPeriod = this.dPeriod;
    c.invPeriod = this.invPeriod;
    c.sqrtAnnual = this.sqrtAnnual;
    c.trailingIdx = this.trailingIdx;
    c.barsSinceReseed = this.barsSinceReseed;
    c.j = this.j;
    c.windowStart = this.windowStart;
    c.i = this.i;
    c.xMask = this.xMask;
    c.x_inReal = this.x_inReal.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function sharpeStepImpl(sp: SharpeStream, inReal: number): void {
  let price = 0;
  let trailingPrevPrice = 0;
  let windowPrevPrice = 0;
  let ret = 0;
  let dev = 0;
  let meanDev = 0;
  let moment2 = 0;
  let excess = 0;
  let ratio = 0;
  sp.x_inReal[sp.i & sp.xMask] = inReal;
  price = sp.x_inReal[sp.i & sp.xMask];
  ret = 0.0;
  if (sp.prevPrice !== 0.0) {
    ret = (price - sp.prevPrice) / sp.prevPrice;
  }
  sp.prevPrice = price;
  dev = ret - sp.shift;
  sp.periodTotal1 += dev;
  sp.periodTotal2 += dev * dev;
  sp.peakTotal2 = (sp.periodTotal2 > sp.peakTotal2) ? sp.periodTotal2 : sp.peakTotal2;
  meanDev = sp.periodTotal1 * sp.invPeriod;
  moment2 = sp.periodTotal2 - meanDev * sp.periodTotal1;
  /* inReal and outReal may be the same buffer, and the trailing prices
   * sit in slots the output reaches: every read of the window happens
   * before this bar's write. The oldest price read is the trailing
   * return's base, so the index stays on it.
   */
  trailingPrevPrice = sp.x_inReal[sp.trailingIdx & sp.xMask];
  sp.trailingIdx += 1;
  price = sp.x_inReal[sp.trailingIdx & sp.xMask];
  ret = 0.0;
  if (trailingPrevPrice !== 0.0) {
    ret = (price - trailingPrevPrice) / trailingPrevPrice;
  }
  dev = ret - sp.shift;
  sp.periodTotal1 -= dev;
  sp.periodTotal2 -= dev * dev;
  /* Rebuild against the window mean when the spread has shrunk below 1% of
   * the largest sum of squares held since the last rebuild, or at least
   * every 32 windows. The rounding the running sums carry scales with that
   * peak, so once a large return leaves, the current sum can be all
   * rounding; a flat run after a move must still come out exactly flat.
   * The spread is a divisor, so its relative error reaches the ratio.
   */
  sp.barsSinceReseed -= 1;
  if (moment2 < 0.01 * sp.peakTotal2 || sp.barsSinceReseed <= 0) {
    sp.barsSinceReseed = ((32 * sp.optInTimePeriod) | 0);
    sp.windowStart = sp.i - sp.optInTimePeriod + 1;
    windowPrevPrice = sp.x_inReal[(sp.windowStart - 1) & sp.xMask];
    dev = 0.0;
    for (sp.j = sp.windowStart; sp.j <= sp.i; sp.j += 1) {
      price = sp.x_inReal[sp.j & sp.xMask];
      ret = 0.0;
      if (windowPrevPrice !== 0.0) {
        ret = (price - windowPrevPrice) / windowPrevPrice;
      }
      windowPrevPrice = price;
      dev += ret;
    }
    sp.shift = dev * sp.invPeriod;
    windowPrevPrice = sp.x_inReal[(sp.windowStart - 1) & sp.xMask];
    sp.periodTotal1 = 0.0;
    sp.periodTotal2 = 0.0;
    for (sp.j = sp.windowStart; sp.j <= sp.i; sp.j += 1) {
      price = sp.x_inReal[sp.j & sp.xMask];
      ret = 0.0;
      if (windowPrevPrice !== 0.0) {
        ret = (price - windowPrevPrice) / windowPrevPrice;
      }
      windowPrevPrice = price;
      dev = ret - sp.shift;
      sp.periodTotal1 += dev;
      sp.periodTotal2 += dev * dev;
    }
    meanDev = sp.periodTotal1 * sp.invPeriod;
    moment2 = sp.periodTotal2 - meanDev * sp.periodTotal1;
    /* A window of equal returns can sit off this mean by rounding, which
     * leaves a spread of rounding where the ratio must be the plain
     * quotient, and fires the trigger again on every bar. Anchored on
     * one of its own returns, its deviations are exactly zero.
     */
    if (moment2 < 1e-6 * sp.periodTotal2) {
      sp.shift = ret;
      windowPrevPrice = sp.x_inReal[(sp.windowStart - 1) & sp.xMask];
      sp.periodTotal1 = 0.0;
      sp.periodTotal2 = 0.0;
      for (sp.j = sp.windowStart; sp.j <= sp.i; sp.j += 1) {
        price = sp.x_inReal[sp.j & sp.xMask];
        ret = 0.0;
        if (windowPrevPrice !== 0.0) {
          ret = (price - windowPrevPrice) / windowPrevPrice;
        }
        windowPrevPrice = price;
        dev = ret - sp.shift;
        sp.periodTotal1 += dev;
        sp.periodTotal2 += dev * dev;
      }
      meanDev = sp.periodTotal1 * sp.invPeriod;
      moment2 = sp.periodTotal2 - meanDev * sp.periodTotal1;
    }
    sp.peakTotal2 = sp.periodTotal2;
    /* Re-remove the trailing return under the new shift so the carried
     * state matches the path without a rebuild.
     */
    price = sp.x_inReal[sp.windowStart & sp.xMask];
    ret = 0.0;
    if (sp.x_inReal[(sp.windowStart - 1) & sp.xMask] !== 0.0) {
      ret = (price - sp.x_inReal[(sp.windowStart - 1) & sp.xMask]) / sp.x_inReal[(sp.windowStart - 1) & sp.xMask];
    }
    dev = ret - sp.shift;
    sp.periodTotal1 -= dev;
    sp.periodTotal2 -= dev * dev;
  }
  /* No spread is the plain quotient: +/-Inf by the sign of the excess
   * mean, NaN when it is exactly zero (ruled on #571). Annualize the
   * per-bar ratio last, so an annualization of 1 is that ratio bit for
   * bit.
   */
  excess = sp.shift + meanDev - sp.optInRiskFreeRate;
  ratio = excess / Math.sqrt(moment2 / (sp.dPeriod - 1.0));
  sp.cur_outReal = sp.sqrtAnnual * ratio;
  sp.i += 1;
}

function sharpeOpenImpl(
  sp: SharpeStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInRiskFreeRate: number,
  optInAnnualization: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let shift = 0;
  let periodTotal1 = 0;
  let periodTotal2 = 0;
  let peakTotal2 = 0;
  let price = 0;
  let prevPrice = 0;
  let trailingPrevPrice = 0;
  let windowPrevPrice = 0;
  let ret = 0;
  let dev = 0;
  let dPeriod = 0;
  let invPeriod = 0;
  let meanDev = 0;
  let moment2 = 0;
  let excess = 0;
  let sqrtAnnual = 0;
  let ratio = 0;
  let i = 0;
  let j = 0;
  let outIdx = 0;
  let trailingIdx = 0;
  let windowStart = 0;
  let barsSinceReseed = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInRiskFreeRate === Core.REAL_DEFAULT) {
    optInRiskFreeRate = 0.0;
  } else if (!(optInRiskFreeRate >= Core.REAL_MIN && optInRiskFreeRate <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInAnnualization === Core.REAL_DEFAULT) {
    optInAnnualization = 252.0;
  } else if (!(optInAnnualization >= 0e0 && optInAnnualization <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (startIdx < optInTimePeriod) {
    startIdx = optInTimePeriod;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  dPeriod = optInTimePeriod;
  invPeriod = 1.0 / dPeriod;
  sqrtAnnual = Math.sqrt(optInAnnualization);
  /* The window holds the returns of the bars after trailingIdx up to i; a
   * bar's return reads the price before it.
   *
   * The variance of the excess returns is the variance of the returns, so
   * the risk-free rate enters only the mean. That keeps the sums at return
   * scale whatever the rate, and lets a flat price window reach an exact
   * zero spread.
   */
  trailingIdx = startIdx - optInTimePeriod;
  prevPrice = inReal[trailingIdx];
  /* A return needs a non-zero base price and nothing more: the test is
   * exact, never a band, since a price carries the quote unit. A zero base
   * gives a zero return.
   */
  price = inReal[trailingIdx + 1];
  ret = 0.0;
  if (prevPrice !== 0.0) {
    ret = (price - prevPrice) / prevPrice;
  }
  shift = ret;
  periodTotal1 = 0.0;
  periodTotal2 = 0.0;
  for (j = trailingIdx + 1; j < startIdx; j += 1) {
    price = inReal[j];
    ret = 0.0;
    if (prevPrice !== 0.0) {
      ret = (price - prevPrice) / prevPrice;
    }
    prevPrice = price;
    dev = ret - shift;
    periodTotal1 += dev;
    periodTotal2 += dev * dev;
  }
  i = startIdx;
  outIdx = 0;
  barsSinceReseed = ((32 * optInTimePeriod) | 0);
  peakTotal2 = periodTotal2;
  do {
    price = inReal[i];
    ret = 0.0;
    if (prevPrice !== 0.0) {
      ret = (price - prevPrice) / prevPrice;
    }
    prevPrice = price;
    dev = ret - shift;
    periodTotal1 += dev;
    periodTotal2 += dev * dev;
    peakTotal2 = (periodTotal2 > peakTotal2) ? periodTotal2 : peakTotal2;
    meanDev = periodTotal1 * invPeriod;
    moment2 = periodTotal2 - meanDev * periodTotal1;
    /* inReal and outReal may be the same buffer, and the trailing prices
     * sit in slots the output reaches: every read of the window happens
     * before this bar's write. The oldest price read is the trailing
     * return's base, so the index stays on it.
     */
    trailingPrevPrice = inReal[trailingIdx];
    trailingIdx += 1;
    price = inReal[trailingIdx];
    ret = 0.0;
    if (trailingPrevPrice !== 0.0) {
      ret = (price - trailingPrevPrice) / trailingPrevPrice;
    }
    dev = ret - shift;
    periodTotal1 -= dev;
    periodTotal2 -= dev * dev;
    /* Rebuild against the window mean when the spread has shrunk below 1% of
     * the largest sum of squares held since the last rebuild, or at least
     * every 32 windows. The rounding the running sums carry scales with that
     * peak, so once a large return leaves, the current sum can be all
     * rounding; a flat run after a move must still come out exactly flat.
     * The spread is a divisor, so its relative error reaches the ratio.
     */
    barsSinceReseed -= 1;
    if (moment2 < 0.01 * peakTotal2 || barsSinceReseed <= 0) {
      barsSinceReseed = ((32 * optInTimePeriod) | 0);
      windowStart = i - optInTimePeriod + 1;
      windowPrevPrice = inReal[windowStart - 1];
      dev = 0.0;
      for (j = windowStart; j <= i; j += 1) {
        price = inReal[j];
        ret = 0.0;
        if (windowPrevPrice !== 0.0) {
          ret = (price - windowPrevPrice) / windowPrevPrice;
        }
        windowPrevPrice = price;
        dev += ret;
      }
      shift = dev * invPeriod;
      windowPrevPrice = inReal[windowStart - 1];
      periodTotal1 = 0.0;
      periodTotal2 = 0.0;
      for (j = windowStart; j <= i; j += 1) {
        price = inReal[j];
        ret = 0.0;
        if (windowPrevPrice !== 0.0) {
          ret = (price - windowPrevPrice) / windowPrevPrice;
        }
        windowPrevPrice = price;
        dev = ret - shift;
        periodTotal1 += dev;
        periodTotal2 += dev * dev;
      }
      meanDev = periodTotal1 * invPeriod;
      moment2 = periodTotal2 - meanDev * periodTotal1;
      /* A window of equal returns can sit off this mean by rounding, which
       * leaves a spread of rounding where the ratio must be the plain
       * quotient, and fires the trigger again on every bar. Anchored on
       * one of its own returns, its deviations are exactly zero.
       */
      if (moment2 < 1e-6 * periodTotal2) {
        shift = ret;
        windowPrevPrice = inReal[windowStart - 1];
        periodTotal1 = 0.0;
        periodTotal2 = 0.0;
        for (j = windowStart; j <= i; j += 1) {
          price = inReal[j];
          ret = 0.0;
          if (windowPrevPrice !== 0.0) {
            ret = (price - windowPrevPrice) / windowPrevPrice;
          }
          windowPrevPrice = price;
          dev = ret - shift;
          periodTotal1 += dev;
          periodTotal2 += dev * dev;
        }
        meanDev = periodTotal1 * invPeriod;
        moment2 = periodTotal2 - meanDev * periodTotal1;
      }
      peakTotal2 = periodTotal2;
      /* Re-remove the trailing return under the new shift so the carried
       * state matches the path without a rebuild.
       */
      price = inReal[windowStart];
      ret = 0.0;
      if (inReal[windowStart - 1] !== 0.0) {
        ret = (price - inReal[windowStart - 1]) / inReal[windowStart - 1];
      }
      dev = ret - shift;
      periodTotal1 -= dev;
      periodTotal2 -= dev * dev;
    }
    /* No spread is the plain quotient: +/-Inf by the sign of the excess
     * mean, NaN when it is exactly zero (ruled on #571). Annualize the
     * per-bar ratio last, so an annualization of 1 is that ratio bit for
     * bit.
     */
    excess = shift + meanDev - optInRiskFreeRate;
    ratio = excess / Math.sqrt(moment2 / (dPeriod - 1.0));
    outReal[((outIdx++ * outStride) | 0)] = sqrtAnnual * ratio;
    i += 1;
  } while (i <= endIdx);
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const capX = i - trailingIdx + 1;
  if (capX < 1 || capX > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  let physX = 1;
  while (physX < capX) {
    physX <<= 1;
  }
  const capX_inReal = new Float64Array(physX);
  for (let fillJ = historyLen - capX; fillJ < historyLen; fillJ++) {
    capX_inReal[fillJ & (physX - 1)] = inReal[fillJ];
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.optInRiskFreeRate = optInRiskFreeRate;
  sp.optInAnnualization = optInAnnualization;
  sp.shift = shift;
  sp.periodTotal1 = periodTotal1;
  sp.periodTotal2 = periodTotal2;
  sp.peakTotal2 = peakTotal2;
  sp.prevPrice = prevPrice;
  sp.dPeriod = dPeriod;
  sp.invPeriod = invPeriod;
  sp.sqrtAnnual = sqrtAnnual;
  sp.trailingIdx = trailingIdx;
  sp.barsSinceReseed = barsSinceReseed;
  sp.j = j;
  sp.windowStart = windowStart;
  sp.i = i;
  sp.xMask = physX - 1;
  sp.x_inReal = capX_inReal;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function sharpeOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInRiskFreeRate: number,
  optInAnnualization: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): SharpeStream {
  const sp = SharpeStream.of(core);
  const retCode = sharpeOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInRiskFreeRate, optInAnnualization, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SHARPE openAndFill', inReal.length, startIdx, sharpeLookbackPositional(optInTimePeriod, optInRiskFreeRate, optInAnnualization, core));
  }
  if (retCode === RetCode.BAD_PARAM && sharpeLookbackPositional(optInTimePeriod, optInRiskFreeRate, optInAnnualization, core) < 0) {
    throw rejectedParams('SHARPE openAndFill', PARAM_RANGES, optInTimePeriod, optInRiskFreeRate, optInAnnualization);
  }
  throw streamFailure('SHARPE openAndFill', retCode);
}

/** @internal */
export function sharpeOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInRiskFreeRate: number,
  optInAnnualization: number,
): SharpeStream {
  const sp = SharpeStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = sharpeOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInRiskFreeRate, optInAnnualization, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SHARPE open', inReal.length, startIdx, sharpeLookbackPositional(optInTimePeriod, optInRiskFreeRate, optInAnnualization, core));
  }
  if (retCode === RetCode.BAD_PARAM && sharpeLookbackPositional(optInTimePeriod, optInRiskFreeRate, optInAnnualization, core) < 0) {
    throw rejectedParams('SHARPE open', PARAM_RANGES, optInTimePeriod, optInRiskFreeRate, optInAnnualization);
  }
  throw streamFailure('SHARPE open', retCode);
}

/** @internal */
export function sharpeOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInRiskFreeRate: number,
  optInAnnualization: number,
  core: Core = Core.DEFAULT,
): SharpeStream {
  optInTimePeriod = requireInt32('SHARPE open', 'timePeriod', optInTimePeriod);
  optInRiskFreeRate = requireReal('SHARPE open', 'riskFreeRate', optInRiskFreeRate);
  optInAnnualization = requireReal('SHARPE open', 'annualization', optInAnnualization);
  requireArgument('SHARPE open', 'real', inReal);
  requireHistory('SHARPE open', 'real', inReal.length);
  return sharpeOpenInternal(core, float64(inReal), 0, optInTimePeriod, optInRiskFreeRate, optInAnnualization);
}

/** @internal */
export function sharpeOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInRiskFreeRate: number,
  optInAnnualization: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): SharpeStream {
  optInTimePeriod = requireInt32('SHARPE openAndFill', 'timePeriod', optInTimePeriod);
  optInRiskFreeRate = requireReal('SHARPE openAndFill', 'riskFreeRate', optInRiskFreeRate);
  optInAnnualization = requireReal('SHARPE openAndFill', 'annualization', optInAnnualization);
  requireArgument('SHARPE openAndFill', 'real', inReal);
  requireHistory('SHARPE openAndFill', 'real', inReal.length);
  const guardLookback = sharpeLookbackPositional(optInTimePeriod, optInRiskFreeRate, optInAnnualization, core);
  if (guardLookback < 0) {
    throw rejectedParams('SHARPE openAndFill', PARAM_RANGES, optInTimePeriod, optInRiskFreeRate, optInAnnualization);
  }
  const guardOutLen = openFillCount('SHARPE openAndFill', inReal.length, guardLookback);
  requireOutput('SHARPE openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('SHARPE openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('SHARPE openAndFill', 'outReal', 'real');
  }
  return sharpeOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, optInRiskFreeRate, optInAnnualization, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live SHARPE stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link sharpe} there.
 *
 * @param real - Warm-up history. The price series; its returns are computed
 *   internally.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SharpeOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `sharpeLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a real parameter that is not a
 *   number or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { sharpeOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = sharpeOpen(data, {
 *   timePeriod: 20,
 *   riskFreeRate: 0,
 *   annualization: 252,
 * });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function sharpeOpen(real: ArrayLike<number>, options?: SharpeOptions): SharpeStream {
  const o = optionsOf('SHARPE open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInRiskFreeRate = orDefault(o.riskFreeRate, Core.REAL_DEFAULT);
  const optInAnnualization = orDefault(o.annualization, Core.REAL_DEFAULT);
  const core = coreOf('SHARPE open', o.core);
  return sharpeOpenPositional(real, optInTimePeriod, optInRiskFreeRate, optInAnnualization, core);
}

/**
 * {@link sharpeOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link sharpe}.
 *
 * @param real - Warm-up history. The price series; its returns are computed
 *   internally.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link SharpeOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link sharpe} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `sharpeLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a real parameter that is not a
 *   number, a parameter outside its range or a buffer too short, of the wrong type
 *   or starting where another buffer or a `Float64Array` input starts, or, on a
 *   history long enough to open, one sharing memory with another buffer or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { sharpeOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = sharpeOpenAndFill(data, {
 *   timePeriod: 20,
 *   riskFreeRate: 0,
 *   annualization: 252,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function sharpeOpenAndFill(real: ArrayLike<number>, options?: SharpeOptions): SharpeResult & { stream: SharpeStream } {
  const o = optionsOf('SHARPE openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInRiskFreeRate = orDefault(o.riskFreeRate, Core.REAL_DEFAULT);
  const optInAnnualization = orDefault(o.annualization, Core.REAL_DEFAULT);
  const core = coreOf('SHARPE openAndFill', o.core);
  const capacity = fillCapacity(sharpeLookbackPositional(optInTimePeriod, optInRiskFreeRate, optInAnnualization, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = sharpeOpenAndFillPositional(real, optInTimePeriod, optInRiskFreeRate, optInAnnualization, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
