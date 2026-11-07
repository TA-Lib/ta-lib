/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  JP       John Price <jp_talib@gcfl.net>
 *  MF       Mario Fortier
 *  AM       Adrian Michel <http://amichel.com>
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY      Description
 *  -------------------------------------------------------------------
 *  070203 JP      Initial.
 *  072106 MF,AM   Fix #1526632. Add missing atan().
 *  071326 MF,CC   O(period) per-bar rescan -> O(1) sliding-sum recurrence
 *                 (numerics-changing). See issue #103.
 *  072026 MF,CC   Read the departing value before the output write so in-place
 *                 (outReal==inReal) calls stay correct. See issue #130.
 *  082426 MF,CC  Fix #254. Re-anchor the running sums: every 32*period bars,
 *                and on the bar a large value leaves the window.
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function linearregAngleLookbackPositional(optInTimePeriod: number, _core: Core = Core.DEFAULT): number {
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
export function linearregAngleDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (linearregAngleLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function linearregAngleImpl(
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
  let m = 0;
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
  lookbackTotal = linearregAngleLookbackPositional(optInTimePeriod, core);
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
  m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
  barsSinceReseed = ((32 * optInTimePeriod) | 0);
  trailingValue = inReal[trailingIdx];
  trailingIdx += 1;
  outReal[outIdx++] = Math.atan(m) * (180.0 / 3.141592653589793);
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
    m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
    trailingValue = inReal[trailingIdx];
    trailingIdx += 1;
    outReal[outIdx++] = Math.atan(m) * (180.0 / 3.141592653589793);
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000']] as const;

/** @internal */
export function linearregAnglePositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('LINEARREG_ANGLE', 'startIdx', startIdx);
  endIdx = requireInt32('LINEARREG_ANGLE', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('LINEARREG_ANGLE', 'timePeriod', optInTimePeriod);
  requireIndexRange('LINEARREG_ANGLE', startIdx, endIdx);
  const guardLookback = linearregAngleLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('LINEARREG_ANGLE', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('LINEARREG_ANGLE', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('LINEARREG_ANGLE', 'real', inReal, guardInLen);
  requireOutput('LINEARREG_ANGLE', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('LINEARREG_ANGLE', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = linearregAngleImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('LINEARREG_ANGLE', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link linearregAngle}, {@link linearregAngleLookback},
 * {@link linearregAngleDisplayShift}, {@link linearregAngleOpen} and
 * {@link linearregAngleOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface LinearregAngleOptions {
  /**
   * Number of points in the regression window (default 14; range 2 to 100000;
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
  outReal?: Float64Array | undefined;
}

/**
 * What {@link linearregAngle} returns, and {@link linearregAngleOpenAndFill} with
 * its stream.
 */
export interface LinearregAngleResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Regression line slope expressed as an angle in degrees. `values[k]` is the value
   * at input bar `begIdx + k`.
   */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link linearregAngle} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link LinearregAngleOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function linearregAngleLookback(options?: LinearregAngleOptions): number {
  const o = optionsOf('LINEARREG_ANGLE lookback', options);
  return linearregAngleLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('LINEARREG_ANGLE lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link linearregAngle}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link LinearregAngleOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function linearregAngleDisplayShift(outputIdx: number, options?: LinearregAngleOptions): number {
  const o = optionsOf('LINEARREG_ANGLE displayShift', options);
  return linearregAngleDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('LINEARREG_ANGLE displayShift', o.core));
}

/**
 * The angle, in degrees, of the least-squares best-fit line over the last N
 * points. It is the LINEARREG_SLOPE value passed through atan and converted to
 * degrees. Positive angle = rising fit line, negative = falling; magnitude
 * reflects steepness.
 *
 * Formula and more info at
 * [ta-lib.org/functions/linearreg_angle](https://ta-lib.org/functions/linearreg_angle).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link linearregAngleLookback} is a success with empty outputs.
 *
 * @param real - Input series to regress.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link LinearregAngleOptions}.
 * @returns `begIdx` and the values; see {@link LinearregAngleResult}. Each output
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
 * import { linearregAngle } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = linearregAngle(data, { timePeriod: 14 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`linearreg`](https://ta-lib.org/functions/linearreg)
 * @see [`linearregSlope`](https://ta-lib.org/functions/linearreg_slope)
 * @see [`linearregIntercept`](https://ta-lib.org/functions/linearreg_intercept)
 * @see [`tsf`](https://ta-lib.org/functions/tsf)
 */
export function linearregAngle(real: ArrayLike<number>, options?: LinearregAngleOptions): LinearregAngleResult {
  const o = optionsOf('LINEARREG_ANGLE', options);
  if (o.endIdx === undefined) {
    requireSeries('LINEARREG_ANGLE', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('LINEARREG_ANGLE', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, linearregAngleLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = linearregAnglePositional(startIdx, endIdx, real, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live LINEARREG_ANGLE stream: one value per closed bar, bit-identical to
 * {@link linearregAngle} over the same series. Open it with
 * {@link linearregAngleOpen} or {@link linearregAngleOpenAndFill}; there is
 * nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/linearreg_angle](https://ta-lib.org/functions/linearreg_angle).
 */
export class LinearregAngleStream {
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
  static of(core: Core): LinearregAngleStream {
    return new LinearregAngleStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link linearregAngle} reports them over the same bars. Each accepted `update`
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
      throw streamFailure('LINEARREG_ANGLE advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: Input series to regress.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('LINEARREG_ANGLE update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('LINEARREG_ANGLE update', 'real');
    }
    linearregAngleStepImpl(this, real);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: Input series to regress.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number): number {
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('LINEARREG_ANGLE peek', 'real');
    }
    const sp = this;
    const inReal = real;
    let m = 0;
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
    m = (sp.optInTimePeriod * SumXY - sp.SumX * SumY) / sp.Divisor;
    trailingValue = ((trailingIdx & sp.xMask) !== pkSlot0) ? sp.x_inReal[trailingIdx & sp.xMask] : pkVal0;
    trailingIdx += 1;
    cur_outReal = Math.atan(m) * (180.0 / 3.141592653589793);
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
  clone(): LinearregAngleStream {
    const c = new LinearregAngleStream(this.core);
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

function linearregAngleStepImpl(sp: LinearregAngleStream, inReal: number): void {
  let m = 0;
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
  m = (sp.optInTimePeriod * sp.SumXY - sp.SumX * sp.SumY) / sp.Divisor;
  sp.trailingValue = sp.x_inReal[sp.trailingIdx & sp.xMask];
  sp.trailingIdx += 1;
  sp.cur_outReal = Math.atan(m) * (180.0 / 3.141592653589793);
  sp.today += 1;
}

function linearregAngleOpenImpl(
  sp: LinearregAngleStream,
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
  let m = 0;
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
  lookbackTotal = linearregAngleLookbackPositional(optInTimePeriod, core);
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
  m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
  barsSinceReseed = ((32 * optInTimePeriod) | 0);
  trailingValue = inReal[trailingIdx];
  trailingIdx += 1;
  outReal[((outIdx++ * outStride) | 0)] = Math.atan(m) * (180.0 / 3.141592653589793);
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
    m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
    trailingValue = inReal[trailingIdx];
    trailingIdx += 1;
    outReal[((outIdx++ * outStride) | 0)] = Math.atan(m) * (180.0 / 3.141592653589793);
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
export function linearregAngleOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): LinearregAngleStream {
  const sp = LinearregAngleStream.of(core);
  const retCode = linearregAngleOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('LINEARREG_ANGLE openAndFill', inReal.length, startIdx, linearregAngleLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && linearregAngleLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('LINEARREG_ANGLE openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('LINEARREG_ANGLE openAndFill', retCode);
}

/** @internal */
export function linearregAngleOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): LinearregAngleStream {
  const sp = LinearregAngleStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = linearregAngleOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('LINEARREG_ANGLE open', inReal.length, startIdx, linearregAngleLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && linearregAngleLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('LINEARREG_ANGLE open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('LINEARREG_ANGLE open', retCode);
}

/** @internal */
export function linearregAngleOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): LinearregAngleStream {
  optInTimePeriod = requireInt32('LINEARREG_ANGLE open', 'timePeriod', optInTimePeriod);
  requireArgument('LINEARREG_ANGLE open', 'real', inReal);
  requireHistory('LINEARREG_ANGLE open', 'real', inReal.length);
  return linearregAngleOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/** @internal */
export function linearregAngleOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): LinearregAngleStream {
  optInTimePeriod = requireInt32('LINEARREG_ANGLE openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('LINEARREG_ANGLE openAndFill', 'real', inReal);
  requireHistory('LINEARREG_ANGLE openAndFill', 'real', inReal.length);
  const guardLookback = linearregAngleLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('LINEARREG_ANGLE openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('LINEARREG_ANGLE openAndFill', inReal.length, guardLookback);
  requireOutput('LINEARREG_ANGLE openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('LINEARREG_ANGLE openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('LINEARREG_ANGLE openAndFill', 'outReal', 'real');
  }
  return linearregAngleOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live LINEARREG_ANGLE stream over the warm-up history: `value()` starts
 * at the last history bar, bit-identical to {@link linearregAngle} there.
 *
 * @param real - Warm-up history. Input series to regress.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link LinearregAngleOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `linearregAngleLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, an integer parameter that is not an int32 or a
 *   parameter outside its range.
 *
 * @example
 * ```ts
 * import { linearregAngleOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = linearregAngleOpen(data, { timePeriod: 14 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function linearregAngleOpen(real: ArrayLike<number>, options?: LinearregAngleOptions): LinearregAngleStream {
  const o = optionsOf('LINEARREG_ANGLE open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('LINEARREG_ANGLE open', o.core);
  return linearregAngleOpenPositional(real, optInTimePeriod, core);
}

/**
 * {@link linearregAngleOpen} that also computes the outputs over the whole history
 * in the same pass, bit-identically to {@link linearregAngle}.
 *
 * @param real - Warm-up history. Input series to regress.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link LinearregAngleOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link linearregAngle} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `linearregAngleLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, an integer parameter that is not an int32, a parameter
 *   outside its range or a buffer too short, of the wrong type or starting where
 *   another buffer or a `Float64Array` input starts, or, on a history long enough to
 *   open, one sharing memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { linearregAngleOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = linearregAngleOpenAndFill(data, {
 *   timePeriod: 14,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function linearregAngleOpenAndFill(real: ArrayLike<number>, options?: LinearregAngleOptions): LinearregAngleResult & { stream: LinearregAngleStream } {
  const o = optionsOf('LINEARREG_ANGLE openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('LINEARREG_ANGLE openAndFill', o.core);
  const capacity = fillCapacity(linearregAngleLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = linearregAngleOpenAndFillPositional(real, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
