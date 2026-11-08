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
 *  JP       John Price <jp_talib@gcfl.net>
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  070203 JP     Initial.
 *  071326 MF,CC  O(period) per-bar rescan -> O(1) sliding-sum recurrence
 *                (numerics-changing). See issue #103.
 *  072026 MF,CC  Read the departing value before the output write so in-place
 *                (outReal==inReal) calls stay correct. See issue #130.
 *  082426 MF,CC  Fix #254. Re-anchor the running sums: every 32*period bars,
 *                and on the bar a large value leaves the window.
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function linearregSlopeLookbackPositional(optInTimePeriod: number, _core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  return optInTimePeriod - 1;
}

/** @internal */
export function linearregSlopeDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (linearregSlopeLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function linearregSlopeImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let outIdx = 0;
  let today = 0;
  let lookbackTotal = 0;
  let trailingIdx = 0;
  let SumX = 0;
  let SumXY = 0;
  let SumY = 0;
  let SumXSqr = 0;
  let Divisor = 0;
  let i = 0;
  let j = 0;
  let windowStart = 0;
  let barsSinceReseed = 0;
  let tempValue1 = 0;
  let tempValue2 = 0;
  let trailingValue = 0;
  let weightedTrailing = 0;
  let sumAbs = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  /* Linear Regression is a concept also known as the
   * "least squares method" or "best fit." Linear
   * Regression attempts to fit a straight line between
   * several data points in such a way that distance
   * between each data point and the line is minimized.
   *
   * For each point, a straight line over the specified
   * previous bar period is determined in terms
   * of y = b + m*x:
   *
   * TA_LINEARREG          : Returns b+m*(period-1)
   * TA_LINEARREG_SLOPE    : Returns 'm'
   * TA_LINEARREG_ANGLE    : Returns 'm' in degree.
   * TA_LINEARREG_INTERCEPT: Returns 'b'
   * TA_TSF                : Returns b+m*(period)
   */
  /* Adjust startIdx to account for the lookback period. */
  lookbackTotal = linearregSlopeLookbackPositional(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  outIdx = 0;
  /* Index into the output. */
  today = startIdx;
  trailingIdx = startIdx - lookbackTotal;
  SumX = optInTimePeriod * (optInTimePeriod - 1) * 0.5;
  SumXSqr = optInTimePeriod * (optInTimePeriod - 1) * (((2 * optInTimePeriod) | 0) - 1) / 6.0;
  Divisor = SumX * SumX - optInTimePeriod * SumXSqr;
  /* Prime the two data-dependent window sums for the first output with a
   * one-time full-window scan. SumX/SumXSqr/Divisor are period-only constants;
   * SumY = sum of the window, SumXY = sum of i*value (i the reversed
   * 0..period-1 position).
   */
  SumXY = 0;
  SumY = 0;
  sumAbs = 0;
  for (i = optInTimePeriod; i-- !== 0; ) {
    tempValue1 = inReal[today - i];
    SumY += tempValue1;
    SumXY += i * tempValue1;
    sumAbs += Math.abs(tempValue1);
  }
  barsSinceReseed = ((32 * optInTimePeriod) | 0);
  trailingValue = inReal[trailingIdx];
  trailingIdx += 1;
  outReal[outIdx++] = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
  today += 1;
  /* Slide the window one bar at a time, keeping both sums in O(1): advancing
   * the window raises every retained value's weight by 1 (adds SumY) and drops
   * the departing value at full weight (subtracts period*trailingValue). Same
   * incremental identity as WMA/CORREL; the output arithmetic is unchanged.
   * (perf #103 -- numerics-changing: running total vs per-bar fresh sum.)
   * Each departing value is read before the output write of the same bar:
   * with outReal==inReal (in-place, #130) that write lands on the cell the
   * next iteration departs from.
   */
  while (today <= endIdx) {
    weightedTrailing = optInTimePeriod * trailingValue;
    SumXY = SumXY + SumY - weightedTrailing;
    SumY = SumY - trailingValue + inReal[today];
    sumAbs = sumAbs - Math.abs(trailingValue) + Math.abs(inReal[today]);
    /* Re-anchor: rebuild both sums from the window itself. #103 left them as
     * running totals that are never rebuilt, so each bar's rounding joins a
     * residue no later bar can subtract -- unbounded in the length of the
     * call, and scaled by the largest value the sums have EVER held rather
     * than by what the window holds now. Two triggers, and they cover
     * different failures (issue #254):
     *
     *   - every 32*period bars, so a slow drift stays bounded however long
     *     the series runs. Same interval as TA_VAR / TA_CORREL / TA_BETA.
     *
     *   - when the value the window just dropped carries more weight than
     *     everything left in it. That is the one the interval cannot cover:
     *     one large print inflates the residue for up to 32*period bars
     *     after it is gone (measured 31x at period 5), and this rebuilds on
     *     the bar it leaves instead.
     *
     * The threshold compares two DEGREE-1 quantities, so it is 100 rather
     * than the 1e6 a degree-2 ratio would take. On ordinary prices it is ~1 and this
     * never fires; it is a compare, not work. The constant is 100 rather than
     * 10 because at 10 a zero-mean oscillator rebuilds on 8.8% of bars for no
     * measured accuracy gain.
     *
     * THE DENOMINATOR IS sumAbs, NOT SumY, AND THAT IS THE WHOLE POINT.
     * SumY is a CANCELLING sum: on a zero-mean window it collapses toward 0
     * while the departing value does not, so |weightedTrailing|/|SumY| is
     * unbounded and the rebuild fires on EVERY bar -- an alternating +/-1
     * series measured 10.9x slower at period 30, which is precisely the
     * O(n*period) cost #103 removed. Same shape of error as #242's absolute
     * guard on a quartic quantity: a ratio test is ill-posed when its
     * denominator can cancel. sumAbs is a sum of magnitudes, so it is 0 only
     * when every value in the window is 0 -- and then the numerator is 0 too
     * and the test is false. There is no window it can misjudge.
     *
     * It is also the RIGHT quantity on the merits, not just the safe one: a
     * fresh rebuild's own error is ~eps*sum|y|, so comparing the departing
     * term against sum|y| asks exactly "would rebuilding beat what we are
     * carrying?".
     *
     * Carrying it is free in practice. Measured on the shipped libta-lib.a it
     * costs nothing against the |SumY| form on a price series (1.541 vs 1.605
     * ns/bar at period 14) because the update is INDEPENDENT of the serial
     * SumXY -> SumY dependency chain and fills slots that were idle. The
     * rejected alternative -- keeping |SumY| and rate-limiting the trigger to
     * once per `period` bars -- bounded the cliff at 1.2x rather than removing
     * it, and silently dropped any print departing within `period` bars of a
     * rebuild (~3% of them).
     *
     * The scan walks the window oldest-first with the weight counting DOWN,
     * which is the priming scan's order and weighting -- so a reseeded bar is
     * bit-identical to the same bar computed by a call that started there.
     * That identity is the whole point: it is what the range-stability
     * contract measures.
     *
     * Reading the window is safe when outReal aliases inReal (#130): the
     * outputs written so far occupy [0, outIdx-1], and windowStart is
     * today-lookbackTotal, which is >= outIdx because startIdx was clamped
     * to at least lookbackTotal.
     */
    barsSinceReseed -= 1;
    if (barsSinceReseed <= 0 || Math.abs(weightedTrailing) > 100.0 * sumAbs) {
      barsSinceReseed = ((32 * optInTimePeriod) | 0);
      windowStart = today - lookbackTotal;
      SumY = 0;
      SumXY = 0;
      sumAbs = 0;
      tempValue2 = lookbackTotal;
      for (j = windowStart; j <= today; j += 1) {
        tempValue1 = inReal[j];
        SumY += tempValue1;
        SumXY += tempValue2 * tempValue1;
        sumAbs += Math.abs(tempValue1);
        tempValue2 -= 1.0;
      }
    }
    trailingValue = inReal[trailingIdx];
    trailingIdx += 1;
    outReal[outIdx++] = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000']] as const;

/** @internal */
export function linearregSlopePositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('LINEARREG_SLOPE', 'startIdx', startIdx);
  endIdx = requireInt32('LINEARREG_SLOPE', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('LINEARREG_SLOPE', 'timePeriod', optInTimePeriod);
  requireIndexRange('LINEARREG_SLOPE', startIdx, endIdx);
  const guardLookback = linearregSlopeLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('LINEARREG_SLOPE', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('LINEARREG_SLOPE', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('LINEARREG_SLOPE', 'real', inReal, guardInLen);
  requireOutput('LINEARREG_SLOPE', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('LINEARREG_SLOPE', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = linearregSlopeImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('LINEARREG_SLOPE', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link linearregSlope}, {@link linearregSlopeLookback},
 * {@link linearregSlopeDisplayShift}, {@link linearregSlopeOpen} and
 * {@link linearregSlopeOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface LinearregSlopeOptions {
  /**
   * Number of bars in the regression window (default 14; range 2 to 100000;
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
   * Buffer for `values`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outReal?: Float64Array<ArrayBuffer> | undefined;
}

/**
 * What {@link linearregSlope} returns, and {@link linearregSlopeOpenAndFill} with
 * its stream.
 */
export interface LinearregSlopeResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /** Slope m of the fitted line. `values[k]` is the value at input bar `begIdx + k`. */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link linearregSlope} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link LinearregSlopeOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function linearregSlopeLookback(options?: LinearregSlopeOptions): number {
  const o = optionsOf('LINEARREG_SLOPE lookback', options);
  return linearregSlopeLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('LINEARREG_SLOPE lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link linearregSlope}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link LinearregSlopeOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function linearregSlopeDisplayShift(outputIdx: number, options?: LinearregSlopeOptions): number {
  const o = optionsOf('LINEARREG_SLOPE displayShift', options);
  return linearregSlopeDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('LINEARREG_SLOPE displayShift', o.core));
}

/**
 * Slope 'm' of the least-squares best-fit line (y = b + m*x) over the last
 * timePeriod bars. Reports the per-bar rate of change of the fitted trend line.
 * Positive slope = rising trend, negative = falling; magnitude is price change per
 * bar.
 *
 * Formula and more info at
 * [ta-lib.org/functions/linearreg_slope](https://ta-lib.org/functions/linearreg_slope).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link linearregSlopeLookback} is a success with empty outputs.
 *
 * @param real - Data series to fit.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link LinearregSlopeOptions}.
 * @returns `begIdx` and the values; see {@link LinearregSlopeResult}. Each output
 *   is a view of its buffer, the caller's or a new one.
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
 * import { linearregSlope } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = linearregSlope(data, { timePeriod: 14 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`linearreg`](https://ta-lib.org/functions/linearreg)
 * @see [`linearregIntercept`](https://ta-lib.org/functions/linearreg_intercept)
 * @see [`linearregAngle`](https://ta-lib.org/functions/linearreg_angle)
 * @see [`tsf`](https://ta-lib.org/functions/tsf)
 */
export function linearregSlope(real: ArrayLike<number>, options?: LinearregSlopeOptions): LinearregSlopeResult {
  const o = optionsOf('LINEARREG_SLOPE', options);
  if (o.endIdx === undefined) {
    requireSeries('LINEARREG_SLOPE', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('LINEARREG_SLOPE', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, linearregSlopeLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = linearregSlopePositional(startIdx, endIdx, real, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live LINEARREG_SLOPE stream: one value per closed bar, bit-identical to
 * {@link linearregSlope} over the same series. Open it with
 * {@link linearregSlopeOpen} or {@link linearregSlopeOpenAndFill}; there is
 * nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/linearreg_slope](https://ta-lib.org/functions/linearreg_slope).
 */
export class LinearregSlopeStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  lookbackTotal: number = 0;
  /** @internal */
  trailingIdx: number = 0;
  /** @internal */
  SumX: number = 0;
  /** @internal */
  SumXY: number = 0;
  /** @internal */
  SumY: number = 0;
  /** @internal */
  Divisor: number = 0;
  /** @internal */
  barsSinceReseed: number = 0;
  /** @internal */
  trailingValue: number = 0;
  /** @internal */
  sumAbs: number = 0;
  /** @internal */
  j: number = 0;
  /** @internal */
  today: number = 0;
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
  static of(core: Core): LinearregSlopeStream {
    return new LinearregSlopeStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link linearregSlope} reports them over the same bars. Each accepted `update`
   * and each `advance` adds one to `count`.
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
      throw streamFailure('LINEARREG_SLOPE advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: Data series to fit.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('LINEARREG_SLOPE update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('LINEARREG_SLOPE update', 'real');
    }
    linearregSlopeStepImpl(this, real);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: Data series to fit.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number): number {
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('LINEARREG_SLOPE peek', 'real');
    }
    const sp = this;
    const inReal = real;
    let windowStart = 0;
    let tempValue1 = 0;
    let tempValue2 = 0;
    let weightedTrailing = 0;
    let SumXY = sp.SumXY;
    let SumY = sp.SumY;
    let barsSinceReseed = sp.barsSinceReseed;
    let cur_outReal = 0;
    let j = sp.j;
    let sumAbs = sp.sumAbs;
    let trailingIdx = sp.trailingIdx;
    let trailingValue = sp.trailingValue;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    pkSlot0 = sp.today & sp.xMask;
    pkVal0 = inReal;
    weightedTrailing = sp.optInTimePeriod * trailingValue;
    SumXY = SumXY + SumY - weightedTrailing;
    SumY = SumY - trailingValue + (((sp.today & sp.xMask) !== pkSlot0) ? sp.x_inReal[sp.today & sp.xMask] : pkVal0);
    sumAbs = sumAbs - Math.abs(trailingValue) + Math.abs(((sp.today & sp.xMask) !== pkSlot0) ? sp.x_inReal[sp.today & sp.xMask] : pkVal0);
    /* Re-anchor: rebuild both sums from the window itself. #103 left them as
     * running totals that are never rebuilt, so each bar's rounding joins a
     * residue no later bar can subtract -- unbounded in the length of the
     * call, and scaled by the largest value the sums have EVER held rather
     * than by what the window holds now. Two triggers, and they cover
     * different failures (issue #254):
     *
     *   - every 32*period bars, so a slow drift stays bounded however long
     *     the series runs. Same interval as TA_VAR / TA_CORREL / TA_BETA.
     *
     *   - when the value the window just dropped carries more weight than
     *     everything left in it. That is the one the interval cannot cover:
     *     one large print inflates the residue for up to 32*period bars
     *     after it is gone (measured 31x at period 5), and this rebuilds on
     *     the bar it leaves instead.
     *
     * The threshold compares two DEGREE-1 quantities, so it is 100 rather
     * than the 1e6 a degree-2 ratio would take. On ordinary prices it is ~1 and this
     * never fires; it is a compare, not work. The constant is 100 rather than
     * 10 because at 10 a zero-mean oscillator rebuilds on 8.8% of bars for no
     * measured accuracy gain.
     *
     * THE DENOMINATOR IS sumAbs, NOT SumY, AND THAT IS THE WHOLE POINT.
     * SumY is a CANCELLING sum: on a zero-mean window it collapses toward 0
     * while the departing value does not, so |weightedTrailing|/|SumY| is
     * unbounded and the rebuild fires on EVERY bar -- an alternating +/-1
     * series measured 10.9x slower at period 30, which is precisely the
     * O(n*period) cost #103 removed. Same shape of error as #242's absolute
     * guard on a quartic quantity: a ratio test is ill-posed when its
     * denominator can cancel. sumAbs is a sum of magnitudes, so it is 0 only
     * when every value in the window is 0 -- and then the numerator is 0 too
     * and the test is false. There is no window it can misjudge.
     *
     * It is also the RIGHT quantity on the merits, not just the safe one: a
     * fresh rebuild's own error is ~eps*sum|y|, so comparing the departing
     * term against sum|y| asks exactly "would rebuilding beat what we are
     * carrying?".
     *
     * Carrying it is free in practice. Measured on the shipped libta-lib.a it
     * costs nothing against the |SumY| form on a price series (1.541 vs 1.605
     * ns/bar at period 14) because the update is INDEPENDENT of the serial
     * SumXY -> SumY dependency chain and fills slots that were idle. The
     * rejected alternative -- keeping |SumY| and rate-limiting the trigger to
     * once per `period` bars -- bounded the cliff at 1.2x rather than removing
     * it, and silently dropped any print departing within `period` bars of a
     * rebuild (~3% of them).
     *
     * The scan walks the window oldest-first with the weight counting DOWN,
     * which is the priming scan's order and weighting -- so a reseeded bar is
     * bit-identical to the same bar computed by a call that started there.
     * That identity is the whole point: it is what the range-stability
     * contract measures.
     *
     * Reading the window is safe when outReal aliases inReal (#130): the
     * outputs written so far occupy [0, outIdx-1], and windowStart is
     * today-lookbackTotal, which is >= outIdx because startIdx was clamped
     * to at least lookbackTotal.
     */
    barsSinceReseed -= 1;
    if (barsSinceReseed <= 0 || Math.abs(weightedTrailing) > 100.0 * sumAbs) {
      barsSinceReseed = ((32 * sp.optInTimePeriod) | 0);
      windowStart = sp.today - sp.lookbackTotal;
      SumY = 0;
      SumXY = 0;
      sumAbs = 0;
      tempValue2 = sp.lookbackTotal;
      for (j = windowStart; j <= sp.today; j += 1) {
        tempValue1 = ((j & sp.xMask) !== pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0;
        SumY += tempValue1;
        SumXY += tempValue2 * tempValue1;
        sumAbs += Math.abs(tempValue1);
        tempValue2 -= 1.0;
      }
    }
    trailingValue = ((trailingIdx & sp.xMask) !== pkSlot0) ? sp.x_inReal[trailingIdx & sp.xMask] : pkVal0;
    trailingIdx += 1;
    cur_outReal = (sp.optInTimePeriod * SumXY - sp.SumX * SumY) / sp.Divisor;
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
  clone(): LinearregSlopeStream {
    const c = new LinearregSlopeStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.lookbackTotal = this.lookbackTotal;
    c.trailingIdx = this.trailingIdx;
    c.SumX = this.SumX;
    c.SumXY = this.SumXY;
    c.SumY = this.SumY;
    c.Divisor = this.Divisor;
    c.barsSinceReseed = this.barsSinceReseed;
    c.trailingValue = this.trailingValue;
    c.sumAbs = this.sumAbs;
    c.j = this.j;
    c.today = this.today;
    c.xMask = this.xMask;
    c.x_inReal = this.x_inReal.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function linearregSlopeStepImpl(sp: LinearregSlopeStream, inReal: number): void {
  let windowStart = 0;
  let tempValue1 = 0;
  let tempValue2 = 0;
  let weightedTrailing = 0;
  sp.x_inReal[sp.today & sp.xMask] = inReal;
  weightedTrailing = sp.optInTimePeriod * sp.trailingValue;
  sp.SumXY = sp.SumXY + sp.SumY - weightedTrailing;
  sp.SumY = sp.SumY - sp.trailingValue + sp.x_inReal[sp.today & sp.xMask];
  sp.sumAbs = sp.sumAbs - Math.abs(sp.trailingValue) + Math.abs(sp.x_inReal[sp.today & sp.xMask]);
  /* Re-anchor: rebuild both sums from the window itself. #103 left them as
   * running totals that are never rebuilt, so each bar's rounding joins a
   * residue no later bar can subtract -- unbounded in the length of the
   * call, and scaled by the largest value the sums have EVER held rather
   * than by what the window holds now. Two triggers, and they cover
   * different failures (issue #254):
   *
   *   - every 32*period bars, so a slow drift stays bounded however long
   *     the series runs. Same interval as TA_VAR / TA_CORREL / TA_BETA.
   *
   *   - when the value the window just dropped carries more weight than
   *     everything left in it. That is the one the interval cannot cover:
   *     one large print inflates the residue for up to 32*period bars
   *     after it is gone (measured 31x at period 5), and this rebuilds on
   *     the bar it leaves instead.
   *
   * The threshold compares two DEGREE-1 quantities, so it is 100 rather
   * than the 1e6 a degree-2 ratio would take. On ordinary prices it is ~1 and this
   * never fires; it is a compare, not work. The constant is 100 rather than
   * 10 because at 10 a zero-mean oscillator rebuilds on 8.8% of bars for no
   * measured accuracy gain.
   *
   * THE DENOMINATOR IS sumAbs, NOT SumY, AND THAT IS THE WHOLE POINT.
   * SumY is a CANCELLING sum: on a zero-mean window it collapses toward 0
   * while the departing value does not, so |weightedTrailing|/|SumY| is
   * unbounded and the rebuild fires on EVERY bar -- an alternating +/-1
   * series measured 10.9x slower at period 30, which is precisely the
   * O(n*period) cost #103 removed. Same shape of error as #242's absolute
   * guard on a quartic quantity: a ratio test is ill-posed when its
   * denominator can cancel. sumAbs is a sum of magnitudes, so it is 0 only
   * when every value in the window is 0 -- and then the numerator is 0 too
   * and the test is false. There is no window it can misjudge.
   *
   * It is also the RIGHT quantity on the merits, not just the safe one: a
   * fresh rebuild's own error is ~eps*sum|y|, so comparing the departing
   * term against sum|y| asks exactly "would rebuilding beat what we are
   * carrying?".
   *
   * Carrying it is free in practice. Measured on the shipped libta-lib.a it
   * costs nothing against the |SumY| form on a price series (1.541 vs 1.605
   * ns/bar at period 14) because the update is INDEPENDENT of the serial
   * SumXY -> SumY dependency chain and fills slots that were idle. The
   * rejected alternative -- keeping |SumY| and rate-limiting the trigger to
   * once per `period` bars -- bounded the cliff at 1.2x rather than removing
   * it, and silently dropped any print departing within `period` bars of a
   * rebuild (~3% of them).
   *
   * The scan walks the window oldest-first with the weight counting DOWN,
   * which is the priming scan's order and weighting -- so a reseeded bar is
   * bit-identical to the same bar computed by a call that started there.
   * That identity is the whole point: it is what the range-stability
   * contract measures.
   *
   * Reading the window is safe when outReal aliases inReal (#130): the
   * outputs written so far occupy [0, outIdx-1], and windowStart is
   * today-lookbackTotal, which is >= outIdx because startIdx was clamped
   * to at least lookbackTotal.
   */
  sp.barsSinceReseed -= 1;
  if (sp.barsSinceReseed <= 0 || Math.abs(weightedTrailing) > 100.0 * sp.sumAbs) {
    sp.barsSinceReseed = ((32 * sp.optInTimePeriod) | 0);
    windowStart = sp.today - sp.lookbackTotal;
    sp.SumY = 0;
    sp.SumXY = 0;
    sp.sumAbs = 0;
    tempValue2 = sp.lookbackTotal;
    for (sp.j = windowStart; sp.j <= sp.today; sp.j += 1) {
      tempValue1 = sp.x_inReal[sp.j & sp.xMask];
      sp.SumY += tempValue1;
      sp.SumXY += tempValue2 * tempValue1;
      sp.sumAbs += Math.abs(tempValue1);
      tempValue2 -= 1.0;
    }
  }
  sp.trailingValue = sp.x_inReal[sp.trailingIdx & sp.xMask];
  sp.trailingIdx += 1;
  sp.cur_outReal = (sp.optInTimePeriod * sp.SumXY - sp.SumX * sp.SumY) / sp.Divisor;
  sp.today += 1;
}

function linearregSlopeOpenImpl(
  sp: LinearregSlopeStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let outIdx = 0;
  let today = 0;
  let lookbackTotal = 0;
  let trailingIdx = 0;
  let SumX = 0;
  let SumXY = 0;
  let SumY = 0;
  let SumXSqr = 0;
  let Divisor = 0;
  let i = 0;
  let j = 0;
  let windowStart = 0;
  let barsSinceReseed = 0;
  let tempValue1 = 0;
  let tempValue2 = 0;
  let trailingValue = 0;
  let weightedTrailing = 0;
  let sumAbs = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Linear Regression is a concept also known as the
   * "least squares method" or "best fit." Linear
   * Regression attempts to fit a straight line between
   * several data points in such a way that distance
   * between each data point and the line is minimized.
   *
   * For each point, a straight line over the specified
   * previous bar period is determined in terms
   * of y = b + m*x:
   *
   * TA_LINEARREG          : Returns b+m*(period-1)
   * TA_LINEARREG_SLOPE    : Returns 'm'
   * TA_LINEARREG_ANGLE    : Returns 'm' in degree.
   * TA_LINEARREG_INTERCEPT: Returns 'b'
   * TA_TSF                : Returns b+m*(period)
   */
  /* Adjust startIdx to account for the lookback period. */
  lookbackTotal = linearregSlopeLookbackPositional(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outIdx = 0;
  /* Index into the output. */
  today = startIdx;
  trailingIdx = startIdx - lookbackTotal;
  SumX = optInTimePeriod * (optInTimePeriod - 1) * 0.5;
  SumXSqr = optInTimePeriod * (optInTimePeriod - 1) * (((2 * optInTimePeriod) | 0) - 1) / 6.0;
  Divisor = SumX * SumX - optInTimePeriod * SumXSqr;
  /* Prime the two data-dependent window sums for the first output with a
   * one-time full-window scan. SumX/SumXSqr/Divisor are period-only constants;
   * SumY = sum of the window, SumXY = sum of i*value (i the reversed
   * 0..period-1 position).
   */
  SumXY = 0;
  SumY = 0;
  sumAbs = 0;
  for (i = optInTimePeriod; i-- !== 0; ) {
    tempValue1 = inReal[today - i];
    SumY += tempValue1;
    SumXY += i * tempValue1;
    sumAbs += Math.abs(tempValue1);
  }
  barsSinceReseed = ((32 * optInTimePeriod) | 0);
  trailingValue = inReal[trailingIdx];
  trailingIdx += 1;
  outReal[((outIdx++ * outStride) | 0)] = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
  today += 1;
  /* Slide the window one bar at a time, keeping both sums in O(1): advancing
   * the window raises every retained value's weight by 1 (adds SumY) and drops
   * the departing value at full weight (subtracts period*trailingValue). Same
   * incremental identity as WMA/CORREL; the output arithmetic is unchanged.
   * (perf #103 -- numerics-changing: running total vs per-bar fresh sum.)
   * Each departing value is read before the output write of the same bar:
   * with outReal==inReal (in-place, #130) that write lands on the cell the
   * next iteration departs from.
   */
  while (today <= endIdx) {
    weightedTrailing = optInTimePeriod * trailingValue;
    SumXY = SumXY + SumY - weightedTrailing;
    SumY = SumY - trailingValue + inReal[today];
    sumAbs = sumAbs - Math.abs(trailingValue) + Math.abs(inReal[today]);
    /* Re-anchor: rebuild both sums from the window itself. #103 left them as
     * running totals that are never rebuilt, so each bar's rounding joins a
     * residue no later bar can subtract -- unbounded in the length of the
     * call, and scaled by the largest value the sums have EVER held rather
     * than by what the window holds now. Two triggers, and they cover
     * different failures (issue #254):
     *
     *   - every 32*period bars, so a slow drift stays bounded however long
     *     the series runs. Same interval as TA_VAR / TA_CORREL / TA_BETA.
     *
     *   - when the value the window just dropped carries more weight than
     *     everything left in it. That is the one the interval cannot cover:
     *     one large print inflates the residue for up to 32*period bars
     *     after it is gone (measured 31x at period 5), and this rebuilds on
     *     the bar it leaves instead.
     *
     * The threshold compares two DEGREE-1 quantities, so it is 100 rather
     * than the 1e6 a degree-2 ratio would take. On ordinary prices it is ~1 and this
     * never fires; it is a compare, not work. The constant is 100 rather than
     * 10 because at 10 a zero-mean oscillator rebuilds on 8.8% of bars for no
     * measured accuracy gain.
     *
     * THE DENOMINATOR IS sumAbs, NOT SumY, AND THAT IS THE WHOLE POINT.
     * SumY is a CANCELLING sum: on a zero-mean window it collapses toward 0
     * while the departing value does not, so |weightedTrailing|/|SumY| is
     * unbounded and the rebuild fires on EVERY bar -- an alternating +/-1
     * series measured 10.9x slower at period 30, which is precisely the
     * O(n*period) cost #103 removed. Same shape of error as #242's absolute
     * guard on a quartic quantity: a ratio test is ill-posed when its
     * denominator can cancel. sumAbs is a sum of magnitudes, so it is 0 only
     * when every value in the window is 0 -- and then the numerator is 0 too
     * and the test is false. There is no window it can misjudge.
     *
     * It is also the RIGHT quantity on the merits, not just the safe one: a
     * fresh rebuild's own error is ~eps*sum|y|, so comparing the departing
     * term against sum|y| asks exactly "would rebuilding beat what we are
     * carrying?".
     *
     * Carrying it is free in practice. Measured on the shipped libta-lib.a it
     * costs nothing against the |SumY| form on a price series (1.541 vs 1.605
     * ns/bar at period 14) because the update is INDEPENDENT of the serial
     * SumXY -> SumY dependency chain and fills slots that were idle. The
     * rejected alternative -- keeping |SumY| and rate-limiting the trigger to
     * once per `period` bars -- bounded the cliff at 1.2x rather than removing
     * it, and silently dropped any print departing within `period` bars of a
     * rebuild (~3% of them).
     *
     * The scan walks the window oldest-first with the weight counting DOWN,
     * which is the priming scan's order and weighting -- so a reseeded bar is
     * bit-identical to the same bar computed by a call that started there.
     * That identity is the whole point: it is what the range-stability
     * contract measures.
     *
     * Reading the window is safe when outReal aliases inReal (#130): the
     * outputs written so far occupy [0, outIdx-1], and windowStart is
     * today-lookbackTotal, which is >= outIdx because startIdx was clamped
     * to at least lookbackTotal.
     */
    barsSinceReseed -= 1;
    if (barsSinceReseed <= 0 || Math.abs(weightedTrailing) > 100.0 * sumAbs) {
      barsSinceReseed = ((32 * optInTimePeriod) | 0);
      windowStart = today - lookbackTotal;
      SumY = 0;
      SumXY = 0;
      sumAbs = 0;
      tempValue2 = lookbackTotal;
      for (j = windowStart; j <= today; j += 1) {
        tempValue1 = inReal[j];
        SumY += tempValue1;
        SumXY += tempValue2 * tempValue1;
        sumAbs += Math.abs(tempValue1);
        tempValue2 -= 1.0;
      }
    }
    trailingValue = inReal[trailingIdx];
    trailingIdx += 1;
    outReal[((outIdx++ * outStride) | 0)] = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  const capX = today - trailingIdx + 1;
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
  sp.lookbackTotal = lookbackTotal;
  sp.trailingIdx = trailingIdx;
  sp.SumX = SumX;
  sp.SumXY = SumXY;
  sp.SumY = SumY;
  sp.Divisor = Divisor;
  sp.barsSinceReseed = barsSinceReseed;
  sp.trailingValue = trailingValue;
  sp.sumAbs = sumAbs;
  sp.j = j;
  sp.today = today;
  sp.xMask = physX - 1;
  sp.x_inReal = capX_inReal;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function linearregSlopeOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): LinearregSlopeStream {
  const sp = LinearregSlopeStream.of(core);
  const retCode = linearregSlopeOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('LINEARREG_SLOPE openAndFill', inReal.length, startIdx, linearregSlopeLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && linearregSlopeLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('LINEARREG_SLOPE openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('LINEARREG_SLOPE openAndFill', retCode);
}

/** @internal */
export function linearregSlopeOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): LinearregSlopeStream {
  const sp = LinearregSlopeStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = linearregSlopeOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('LINEARREG_SLOPE open', inReal.length, startIdx, linearregSlopeLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && linearregSlopeLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('LINEARREG_SLOPE open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('LINEARREG_SLOPE open', retCode);
}

/** @internal */
export function linearregSlopeOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): LinearregSlopeStream {
  optInTimePeriod = requireInt32('LINEARREG_SLOPE open', 'timePeriod', optInTimePeriod);
  requireArgument('LINEARREG_SLOPE open', 'real', inReal);
  requireHistory('LINEARREG_SLOPE open', 'real', inReal.length);
  return linearregSlopeOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/** @internal */
export function linearregSlopeOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): LinearregSlopeStream {
  optInTimePeriod = requireInt32('LINEARREG_SLOPE openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('LINEARREG_SLOPE openAndFill', 'real', inReal);
  requireHistory('LINEARREG_SLOPE openAndFill', 'real', inReal.length);
  const guardLookback = linearregSlopeLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('LINEARREG_SLOPE openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('LINEARREG_SLOPE openAndFill', inReal.length, guardLookback);
  requireOutput('LINEARREG_SLOPE openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('LINEARREG_SLOPE openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('LINEARREG_SLOPE openAndFill', 'outReal', 'real');
  }
  return linearregSlopeOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live LINEARREG_SLOPE stream over the warm-up history: `value()` starts
 * at the last history bar, bit-identical to {@link linearregSlope} there.
 *
 * @param real - Warm-up history. Data series to fit.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link LinearregSlopeOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `linearregSlopeLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, an integer parameter that is not an int32 or a
 *   parameter outside its range.
 *
 * @example
 * ```ts
 * import { linearregSlopeOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = linearregSlopeOpen(data, { timePeriod: 14 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function linearregSlopeOpen(real: ArrayLike<number>, options?: LinearregSlopeOptions): LinearregSlopeStream {
  const o = optionsOf('LINEARREG_SLOPE open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('LINEARREG_SLOPE open', o.core);
  return linearregSlopeOpenPositional(real, optInTimePeriod, core);
}

/**
 * {@link linearregSlopeOpen} that also computes the outputs over the whole history
 * in the same pass, bit-identically to {@link linearregSlope}.
 *
 * @param real - Warm-up history. Data series to fit.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link LinearregSlopeOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link linearregSlope} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `linearregSlopeLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, an integer parameter that is not an int32, a parameter
 *   outside its range or a buffer too short, of the wrong type or starting where
 *   another buffer or a `Float64Array` input starts, or, on a history long enough to
 *   open, one sharing memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { linearregSlopeOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = linearregSlopeOpenAndFill(data, {
 *   timePeriod: 14,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function linearregSlopeOpenAndFill(real: ArrayLike<number>, options?: LinearregSlopeOptions): LinearregSlopeResult & { stream: LinearregSlopeStream } {
  const o = optionsOf('LINEARREG_SLOPE openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('LINEARREG_SLOPE openAndFill', o.core);
  const capacity = fillCapacity(linearregSlopeLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = linearregSlopeOpenAndFillPositional(real, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
