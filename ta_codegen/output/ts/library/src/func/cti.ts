/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  091526 KL     First version (proposal-drafts issue #74).
 *  092126 MF,CC  Rebuild against the peak sum of squares (issue #430).
 *  092226 MF,CC  #434 branch-free peak update.
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function ctiLookbackPositional(optInTimePeriod: number, _core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  return optInTimePeriod - 1;
}

/** @internal */
export function ctiDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (ctiLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function ctiImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let sumX = 0;
  let sumX2 = 0;
  let sumXY = 0;
  let x = 0;
  let trailingX = 0;
  let peakX2 = 0;
  let shift = 0;
  let ssX = 0;
  let spXY = 0;
  let tempReal = 0;
  let invPeriod = 0;
  let dPeriod = 0;
  let sumY = 0;
  let ssY = 0;
  let lookbackTotal = 0;
  let outIdx = 0;
  let today = 0;
  let trailingIdx = 0;
  let windowStart = 0;
  let j = 0;
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
  lookbackTotal = optInTimePeriod - 1;
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  outBegIdx.value = startIdx;
  trailingIdx = startIdx - lookbackTotal;
  dPeriod = optInTimePeriod;
  invPeriod = 1.0 / dPeriod;
  /* The ramp side is data-independent and collapses to two constants. With y
   * running 0..optInTimePeriod-1 as BARS AGO,
   *
   *    sumY = n(n-1)/2            ssY = sumY2 - sumY*sumY/n = n(n*n-1)/12
   *
   * Both are computed in double because their integer forms overflow: at the
   * top of the parameter range n*n*n is 1e15, far past what an int holds, and
   * the product would wrap silently rather than fail. ssY is strictly positive
   * for every n >= 2, which is why only the price side can make a window
   * degenerate.
   */
  sumY = dPeriod * (dPeriod - 1.0) * 0.5;
  ssY = dPeriod * (dPeriod * dPeriod - 1.0) / 12.0;
  /* Measure the price side against a shift near the window, as correl.c does
   * (#242): n*Sxx - Sx*Sx on raw price levels, as the author's listing writes
   * it, cancels catastrophically once the spread is small against the level.
   *
   * Anchor on the first window value here; a rebuild anchors on the window
   * mean instead (or on inReal[today] when the mean leaves the window
   * flat), which is better centred but costs a pass this one cannot afford
   * before the sums exist.
   */
  shift = inReal[trailingIdx];
  /* The initial window, less its last bar. This bar's y is startIdx-j, since
   * the first output is computed with `today` at startIdx.
   */
  sumXY = 0.0;
  sumX2 = sumXY;
  sumX = sumX2;
  for (j = trailingIdx; j < startIdx; j += 1) {
    x = inReal[j] - shift;
    sumX += x;
    sumX2 += x * x;
    sumXY += x * (startIdx - j);
  }
  today = startIdx;
  outIdx = 0;
  barsSinceReseed = ((32 * optInTimePeriod) | 0);
  peakX2 = sumX2;
  do {
    /* The incoming bar is zero bars ago, so it moves sumX and sumX2 and
     * leaves sumXY alone.
     */
    x = inReal[today] - shift;
    sumX += x;
    sumX2 += x * x;
    peakX2 = (sumX2 > peakX2) ? sumX2 : peakX2;
    ssX = sumX2 - sumX * sumX * invPeriod;
    spXY = sumXY - sumX * sumY * invPeriod;
    /* Re-anchor and rebuild when the shift has gone stale: the price sum of
     * squares has shrunk below 1e-6 of the LARGEST sumX2 held since the last
     * rebuild, OR at least every 32 windows. Measure against that peak, not
     * the current sumX2: the rounding the running sums carry scales with the
     * peak, and a series that decays back onto the shift leaves a current
     * sumX2 made of nothing but that rounding. Only the price side is
     * watched -- the ramp side is exact constants and cannot drift.
     *
     * A vanishing spXY is NOT a trigger. It is a legitimate answer, a window
     * with no linear trend, and reseeding on it would rebuild on every bar of
     * ordinary sideways data.
     */
    barsSinceReseed -= 1;
    if (ssX < 1e-6 * peakX2 || barsSinceReseed <= 0) {
      barsSinceReseed = ((32 * optInTimePeriod) | 0);
      windowStart = today - lookbackTotal;
      tempReal = 0.0;
      for (j = windowStart; j <= today; j += 1) {
        tempReal += inReal[j];
      }
      shift = tempReal * invPeriod;
      sumXY = 0.0;
      sumX2 = sumXY;
      sumX = sumX2;
      for (j = windowStart; j <= today; j += 1) {
        x = inReal[j] - shift;
        sumX += x;
        sumX2 += x * x;
        sumXY += x * (today - j);
      }
      /* A window flat to within the rounding of its own mean leaves ssX at
       * that rounding, which would fire the trigger again on every bar.
       * Anchored on one of its own values instead, ssX is at least half the
       * squared range and sumX2 at most n times it, so it cannot, short of
       * squares that underflow.
       */
      if (sumX2 - sumX * sumX * invPeriod < 1e-6 * sumX2) {
        shift = inReal[today];
        sumXY = 0.0;
        sumX2 = sumXY;
        sumX = sumX2;
        for (j = windowStart; j <= today; j += 1) {
          x = inReal[j] - shift;
          sumX += x;
          sumX2 += x * x;
          sumXY += x * (today - j);
        }
      }
      peakX2 = sumX2;
      ssX = sumX2 - sumX * sumX * invPeriod;
      spXY = sumXY - sumX * sumY * invPeriod;
    }
    /* Save the trailing value before writing the output, since the input and
     * output might be the same array.
     */
    trailingX = inReal[trailingIdx] - shift;
    trailingIdx += 1;
    /* THE SIGN. y here is BARS AGO, so it runs backward in time and a rising
     * series correlates NEGATIVELY with it. Ehlers' listing counts the same
     * way and takes Y = -count, the positively-sloped line the indicator is
     * defined against; negating the coefficient once is the same thing, and
     * keeps the slide below in its bars-ago form. Without it every value is
     * reversed and every magnitude kept.
     *
     * ssY is a positive constant, so only ssX can make the window degenerate.
     * It is tested against its own scale because an absolute band would
     * reject a well-defined correlation on small-valued data. The product is
     * tested too, since at n = 2 the smallest subnormal ssX rounds it to
     * zero.
     *
     * An all-flat window therefore emits exactly 0.0 rather than NaN, which
     * is correl.c's precedent and what #112 requires of a successful call.
     */
    if (ssX > 1e-14 * sumX2 && ssX * ssY > 0.0) {
      tempReal = -spXY / Math.sqrt(ssX * ssY);
      /* A correlation coefficient cannot leave [-1,1]; rounding in the
       * three sums can still put it slightly outside.
       */
      if (tempReal > 1.0) {
        tempReal = 1.0;
      } else if (tempReal < -1.0) {
        tempReal = -1.0;
      }
      outReal[outIdx++] = tempReal;
    } else {
      outReal[outIdx++] = 0.0;
    }
    /* Slide the window one bar in O(1). Advancing it ages every retained
     * value by one bar, which raises each of their weights by 1 and so adds
     * the whole deviation sum; the departing value leaves at the full weight
     * optInTimePeriod-1 and also loses its place in that sum, which is the
     * remaining -1. The two together are exactly -optInTimePeriod*trailingX,
     * and sumX must still be the COMPLETE window's sum when it is read here
     * -- it is decremented on the line below, not above.
     */
    sumXY = sumXY + sumX - dPeriod * trailingX;
    sumX -= trailingX;
    sumX2 -= trailingX * trailingX;
    today += 1;
  } while (today <= endIdx);
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000']] as const;

/** @internal */
export function ctiPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CTI', 'startIdx', startIdx);
  endIdx = requireInt32('CTI', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('CTI', 'timePeriod', optInTimePeriod);
  requireIndexRange('CTI', startIdx, endIdx);
  const guardLookback = ctiLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('CTI', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('CTI', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CTI', 'real', inReal, guardInLen);
  requireOutput('CTI', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('CTI', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = ctiImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CTI', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cti}, {@link ctiLookback}, {@link ctiDisplayShift},
 * {@link ctiOpen} and {@link ctiOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface CtiOptions {
  /**
   * Number of trailing values correlated against the ramp (default 20; range 2 to
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
  outReal?: Float64Array | undefined;
}

/** What {@link cti} returns, and {@link ctiOpenAndFill} with its stream. */
export interface CtiResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Correlation against the ramp, in -1..+1. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link cti} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CtiOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function ctiLookback(options?: CtiOptions): number {
  const o = optionsOf('CTI lookback', options);
  return ctiLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('CTI lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cti}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CtiOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function ctiDisplayShift(outputIdx: number, options?: CtiOptions): number {
  const o = optionsOf('CTI displayShift', options);
  return ctiDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('CTI displayShift', o.core));
}

/**
 * John F. Ehlers' Correlation Trend Indicator: the Pearson correlation of the last
 * `timePeriod` closes against a straight line of positive slope. Bounded in -1..+1
 * by construction — `+1` is a perfectly linear uptrend, `-1` a perfectly linear
 * downtrend, `0` no linear trend. Trend strength and direction in one bounded
 * number, with no smoothing, no recursion and no filter coefficients.
 * Arithmetically it is [`CORREL`](https://ta-lib.org/functions/correl) of the
 * series against a ramp, with the ramp side collapsed to closed-form constants.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cti](https://ta-lib.org/functions/cti).
 *
 * **Notes**
 * - A flat window returns exactly `0.0` rather than holding the previous value as
 *   the author's listing does; holding would make the function path-dependent.
 * - The result is clamped into -1..+1, as `CORREL` is: rounding in three sums can
 *   put a coefficient slightly outside its own range.
 * - `timePeriod` starts at 2, not 1: at `n = 1` the closed form `n²(n²−1)/12` is
 *   identically zero and every window is degenerate.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link ctiLookback} is a success with empty outputs.
 *
 * @param real - The series to measure the trend of.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link CtiOptions}.
 * @returns `begIdx` and the values; see {@link CtiResult}. Each output is a view
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
 * import { cti } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = cti(data, { timePeriod: 20 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`correl`](https://ta-lib.org/functions/correl)
 * @see [`linearregSlope`](https://ta-lib.org/functions/linearreg_slope)
 * @see [`vhf`](https://ta-lib.org/functions/vhf)
 */
export function cti(real: ArrayLike<number>, options?: CtiOptions): CtiResult {
  const o = optionsOf('CTI', options);
  if (o.endIdx === undefined) {
    requireSeries('CTI', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('CTI', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, ctiLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = ctiPositional(startIdx, endIdx, real, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CTI stream: one value per closed bar, bit-identical to {@link cti} over
 * the same series. Open it with {@link ctiOpen} or {@link ctiOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cti](https://ta-lib.org/functions/cti).
 */
export class CtiStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  sumX: number = 0;
  /** @internal */
  sumX2: number = 0;
  /** @internal */
  sumXY: number = 0;
  /** @internal */
  peakX2: number = 0;
  /** @internal */
  shift: number = 0;
  /** @internal */
  invPeriod: number = 0;
  /** @internal */
  dPeriod: number = 0;
  /** @internal */
  sumY: number = 0;
  /** @internal */
  ssY: number = 0;
  /** @internal */
  lookbackTotal: number = 0;
  /** @internal */
  trailingIdx: number = 0;
  /** @internal */
  barsSinceReseed: number = 0;
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
  static of(core: Core): CtiStream {
    return new CtiStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cti} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('CTI advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: The series to measure the trend of.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('CTI update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('CTI update', 'real');
    }
    ctiStepImpl(this, real);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: The series to measure the trend of.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number): number {
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('CTI peek', 'real');
    }
    const sp = this;
    const inReal = real;
    let x = 0;
    let ssX = 0;
    let spXY = 0;
    let tempReal = 0;
    let windowStart = 0;
    let barsSinceReseed = sp.barsSinceReseed;
    let cur_outReal = 0;
    let j = sp.j;
    let peakX2 = sp.peakX2;
    let shift = sp.shift;
    let sumX = sp.sumX;
    let sumX2 = sp.sumX2;
    let sumXY = sp.sumXY;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    pkSlot0 = sp.today & sp.xMask;
    pkVal0 = inReal;
    /* The incoming bar is zero bars ago, so it moves sumX and sumX2 and
     * leaves sumXY alone.
     */
    x = (((sp.today & sp.xMask) !== pkSlot0) ? sp.x_inReal[sp.today & sp.xMask] : pkVal0) - shift;
    sumX += x;
    sumX2 += x * x;
    peakX2 = (sumX2 > peakX2) ? sumX2 : peakX2;
    ssX = sumX2 - sumX * sumX * sp.invPeriod;
    spXY = sumXY - sumX * sp.sumY * sp.invPeriod;
    /* Re-anchor and rebuild when the shift has gone stale: the price sum of
     * squares has shrunk below 1e-6 of the LARGEST sumX2 held since the last
     * rebuild, OR at least every 32 windows. Measure against that peak, not
     * the current sumX2: the rounding the running sums carry scales with the
     * peak, and a series that decays back onto the shift leaves a current
     * sumX2 made of nothing but that rounding. Only the price side is
     * watched -- the ramp side is exact constants and cannot drift.
     *
     * A vanishing spXY is NOT a trigger. It is a legitimate answer, a window
     * with no linear trend, and reseeding on it would rebuild on every bar of
     * ordinary sideways data.
     */
    barsSinceReseed -= 1;
    if (ssX < 1e-6 * peakX2 || barsSinceReseed <= 0) {
      barsSinceReseed = ((32 * sp.optInTimePeriod) | 0);
      windowStart = sp.today - sp.lookbackTotal;
      tempReal = 0.0;
      for (j = windowStart; j <= sp.today; j += 1) {
        tempReal += ((j & sp.xMask) !== pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0;
      }
      shift = tempReal * sp.invPeriod;
      sumXY = 0.0;
      sumX2 = sumXY;
      sumX = sumX2;
      for (j = windowStart; j <= sp.today; j += 1) {
        x = (((j & sp.xMask) !== pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0) - shift;
        sumX += x;
        sumX2 += x * x;
        sumXY += x * (sp.today - j);
      }
      /* A window flat to within the rounding of its own mean leaves ssX at
       * that rounding, which would fire the trigger again on every bar.
       * Anchored on one of its own values instead, ssX is at least half the
       * squared range and sumX2 at most n times it, so it cannot, short of
       * squares that underflow.
       */
      if (sumX2 - sumX * sumX * sp.invPeriod < 1e-6 * sumX2) {
        shift = ((sp.today & sp.xMask) !== pkSlot0) ? sp.x_inReal[sp.today & sp.xMask] : pkVal0;
        sumXY = 0.0;
        sumX2 = sumXY;
        sumX = sumX2;
        for (j = windowStart; j <= sp.today; j += 1) {
          x = (((j & sp.xMask) !== pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0) - shift;
          sumX += x;
          sumX2 += x * x;
          sumXY += x * (sp.today - j);
        }
      }
      peakX2 = sumX2;
      ssX = sumX2 - sumX * sumX * sp.invPeriod;
      spXY = sumXY - sumX * sp.sumY * sp.invPeriod;
    }
    /* THE SIGN. y here is BARS AGO, so it runs backward in time and a rising
     * series correlates NEGATIVELY with it. Ehlers' listing counts the same
     * way and takes Y = -count, the positively-sloped line the indicator is
     * defined against; negating the coefficient once is the same thing, and
     * keeps the slide below in its bars-ago form. Without it every value is
     * reversed and every magnitude kept.
     *
     * ssY is a positive constant, so only ssX can make the window degenerate.
     * It is tested against its own scale because an absolute band would
     * reject a well-defined correlation on small-valued data. The product is
     * tested too, since at n = 2 the smallest subnormal ssX rounds it to
     * zero.
     *
     * An all-flat window therefore emits exactly 0.0 rather than NaN, which
     * is correl.c's precedent and what #112 requires of a successful call.
     */
    if (ssX > 1e-14 * sumX2 && ssX * sp.ssY > 0.0) {
      tempReal = -spXY / Math.sqrt(ssX * sp.ssY);
      /* A correlation coefficient cannot leave [-1,1]; rounding in the
       * three sums can still put it slightly outside.
       */
      if (tempReal > 1.0) {
        tempReal = 1.0;
      } else if (tempReal < -1.0) {
        tempReal = -1.0;
      }
      cur_outReal = tempReal;
    } else {
      cur_outReal = 0.0;
    }
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
  clone(): CtiStream {
    const c = new CtiStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.sumX = this.sumX;
    c.sumX2 = this.sumX2;
    c.sumXY = this.sumXY;
    c.peakX2 = this.peakX2;
    c.shift = this.shift;
    c.invPeriod = this.invPeriod;
    c.dPeriod = this.dPeriod;
    c.sumY = this.sumY;
    c.ssY = this.ssY;
    c.lookbackTotal = this.lookbackTotal;
    c.trailingIdx = this.trailingIdx;
    c.barsSinceReseed = this.barsSinceReseed;
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

function ctiStepImpl(sp: CtiStream, inReal: number): void {
  let x = 0;
  let trailingX = 0;
  let ssX = 0;
  let spXY = 0;
  let tempReal = 0;
  let windowStart = 0;
  sp.x_inReal[sp.today & sp.xMask] = inReal;
  /* The incoming bar is zero bars ago, so it moves sumX and sumX2 and
   * leaves sumXY alone.
   */
  x = sp.x_inReal[sp.today & sp.xMask] - sp.shift;
  sp.sumX += x;
  sp.sumX2 += x * x;
  sp.peakX2 = (sp.sumX2 > sp.peakX2) ? sp.sumX2 : sp.peakX2;
  ssX = sp.sumX2 - sp.sumX * sp.sumX * sp.invPeriod;
  spXY = sp.sumXY - sp.sumX * sp.sumY * sp.invPeriod;
  /* Re-anchor and rebuild when the shift has gone stale: the price sum of
   * squares has shrunk below 1e-6 of the LARGEST sumX2 held since the last
   * rebuild, OR at least every 32 windows. Measure against that peak, not
   * the current sumX2: the rounding the running sums carry scales with the
   * peak, and a series that decays back onto the shift leaves a current
   * sumX2 made of nothing but that rounding. Only the price side is
   * watched -- the ramp side is exact constants and cannot drift.
   *
   * A vanishing spXY is NOT a trigger. It is a legitimate answer, a window
   * with no linear trend, and reseeding on it would rebuild on every bar of
   * ordinary sideways data.
   */
  sp.barsSinceReseed -= 1;
  if (ssX < 1e-6 * sp.peakX2 || sp.barsSinceReseed <= 0) {
    sp.barsSinceReseed = ((32 * sp.optInTimePeriod) | 0);
    windowStart = sp.today - sp.lookbackTotal;
    tempReal = 0.0;
    for (sp.j = windowStart; sp.j <= sp.today; sp.j += 1) {
      tempReal += sp.x_inReal[sp.j & sp.xMask];
    }
    sp.shift = tempReal * sp.invPeriod;
    sp.sumXY = 0.0;
    sp.sumX2 = sp.sumXY;
    sp.sumX = sp.sumX2;
    for (sp.j = windowStart; sp.j <= sp.today; sp.j += 1) {
      x = sp.x_inReal[sp.j & sp.xMask] - sp.shift;
      sp.sumX += x;
      sp.sumX2 += x * x;
      sp.sumXY += x * (sp.today - sp.j);
    }
    /* A window flat to within the rounding of its own mean leaves ssX at
     * that rounding, which would fire the trigger again on every bar.
     * Anchored on one of its own values instead, ssX is at least half the
     * squared range and sumX2 at most n times it, so it cannot, short of
     * squares that underflow.
     */
    if (sp.sumX2 - sp.sumX * sp.sumX * sp.invPeriod < 1e-6 * sp.sumX2) {
      sp.shift = sp.x_inReal[sp.today & sp.xMask];
      sp.sumXY = 0.0;
      sp.sumX2 = sp.sumXY;
      sp.sumX = sp.sumX2;
      for (sp.j = windowStart; sp.j <= sp.today; sp.j += 1) {
        x = sp.x_inReal[sp.j & sp.xMask] - sp.shift;
        sp.sumX += x;
        sp.sumX2 += x * x;
        sp.sumXY += x * (sp.today - sp.j);
      }
    }
    sp.peakX2 = sp.sumX2;
    ssX = sp.sumX2 - sp.sumX * sp.sumX * sp.invPeriod;
    spXY = sp.sumXY - sp.sumX * sp.sumY * sp.invPeriod;
  }
  /* Save the trailing value before writing the output, since the input and
   * output might be the same array.
   */
  trailingX = sp.x_inReal[sp.trailingIdx & sp.xMask] - sp.shift;
  sp.trailingIdx += 1;
  /* THE SIGN. y here is BARS AGO, so it runs backward in time and a rising
   * series correlates NEGATIVELY with it. Ehlers' listing counts the same
   * way and takes Y = -count, the positively-sloped line the indicator is
   * defined against; negating the coefficient once is the same thing, and
   * keeps the slide below in its bars-ago form. Without it every value is
   * reversed and every magnitude kept.
   *
   * ssY is a positive constant, so only ssX can make the window degenerate.
   * It is tested against its own scale because an absolute band would
   * reject a well-defined correlation on small-valued data. The product is
   * tested too, since at n = 2 the smallest subnormal ssX rounds it to
   * zero.
   *
   * An all-flat window therefore emits exactly 0.0 rather than NaN, which
   * is correl.c's precedent and what #112 requires of a successful call.
   */
  if (ssX > 1e-14 * sp.sumX2 && ssX * sp.ssY > 0.0) {
    tempReal = -spXY / Math.sqrt(ssX * sp.ssY);
    /* A correlation coefficient cannot leave [-1,1]; rounding in the
     * three sums can still put it slightly outside.
     */
    if (tempReal > 1.0) {
      tempReal = 1.0;
    } else if (tempReal < -1.0) {
      tempReal = -1.0;
    }
    sp.cur_outReal = tempReal;
  } else {
    sp.cur_outReal = 0.0;
  }
  /* Slide the window one bar in O(1). Advancing it ages every retained
   * value by one bar, which raises each of their weights by 1 and so adds
   * the whole deviation sum; the departing value leaves at the full weight
   * optInTimePeriod-1 and also loses its place in that sum, which is the
   * remaining -1. The two together are exactly -optInTimePeriod*trailingX,
   * and sumX must still be the COMPLETE window's sum when it is read here
   * -- it is decremented on the line below, not above.
   */
  sp.sumXY = sp.sumXY + sp.sumX - sp.dPeriod * trailingX;
  sp.sumX -= trailingX;
  sp.sumX2 -= trailingX * trailingX;
  sp.today += 1;
}

function ctiOpenImpl(
  sp: CtiStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let sumX = 0;
  let sumX2 = 0;
  let sumXY = 0;
  let x = 0;
  let trailingX = 0;
  let peakX2 = 0;
  let shift = 0;
  let ssX = 0;
  let spXY = 0;
  let tempReal = 0;
  let invPeriod = 0;
  let dPeriod = 0;
  let sumY = 0;
  let ssY = 0;
  let lookbackTotal = 0;
  let outIdx = 0;
  let today = 0;
  let trailingIdx = 0;
  let windowStart = 0;
  let j = 0;
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
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  lookbackTotal = optInTimePeriod - 1;
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outBegIdx.value = startIdx;
  trailingIdx = startIdx - lookbackTotal;
  dPeriod = optInTimePeriod;
  invPeriod = 1.0 / dPeriod;
  /* The ramp side is data-independent and collapses to two constants. With y
   * running 0..optInTimePeriod-1 as BARS AGO,
   *
   *    sumY = n(n-1)/2            ssY = sumY2 - sumY*sumY/n = n(n*n-1)/12
   *
   * Both are computed in double because their integer forms overflow: at the
   * top of the parameter range n*n*n is 1e15, far past what an int holds, and
   * the product would wrap silently rather than fail. ssY is strictly positive
   * for every n >= 2, which is why only the price side can make a window
   * degenerate.
   */
  sumY = dPeriod * (dPeriod - 1.0) * 0.5;
  ssY = dPeriod * (dPeriod * dPeriod - 1.0) / 12.0;
  /* Measure the price side against a shift near the window, as correl.c does
   * (#242): n*Sxx - Sx*Sx on raw price levels, as the author's listing writes
   * it, cancels catastrophically once the spread is small against the level.
   *
   * Anchor on the first window value here; a rebuild anchors on the window
   * mean instead (or on inReal[today] when the mean leaves the window
   * flat), which is better centred but costs a pass this one cannot afford
   * before the sums exist.
   */
  shift = inReal[trailingIdx];
  /* The initial window, less its last bar. This bar's y is startIdx-j, since
   * the first output is computed with `today` at startIdx.
   */
  sumXY = 0.0;
  sumX2 = sumXY;
  sumX = sumX2;
  for (j = trailingIdx; j < startIdx; j += 1) {
    x = inReal[j] - shift;
    sumX += x;
    sumX2 += x * x;
    sumXY += x * (startIdx - j);
  }
  today = startIdx;
  outIdx = 0;
  barsSinceReseed = ((32 * optInTimePeriod) | 0);
  peakX2 = sumX2;
  do {
    /* The incoming bar is zero bars ago, so it moves sumX and sumX2 and
     * leaves sumXY alone.
     */
    x = inReal[today] - shift;
    sumX += x;
    sumX2 += x * x;
    peakX2 = (sumX2 > peakX2) ? sumX2 : peakX2;
    ssX = sumX2 - sumX * sumX * invPeriod;
    spXY = sumXY - sumX * sumY * invPeriod;
    /* Re-anchor and rebuild when the shift has gone stale: the price sum of
     * squares has shrunk below 1e-6 of the LARGEST sumX2 held since the last
     * rebuild, OR at least every 32 windows. Measure against that peak, not
     * the current sumX2: the rounding the running sums carry scales with the
     * peak, and a series that decays back onto the shift leaves a current
     * sumX2 made of nothing but that rounding. Only the price side is
     * watched -- the ramp side is exact constants and cannot drift.
     *
     * A vanishing spXY is NOT a trigger. It is a legitimate answer, a window
     * with no linear trend, and reseeding on it would rebuild on every bar of
     * ordinary sideways data.
     */
    barsSinceReseed -= 1;
    if (ssX < 1e-6 * peakX2 || barsSinceReseed <= 0) {
      barsSinceReseed = ((32 * optInTimePeriod) | 0);
      windowStart = today - lookbackTotal;
      tempReal = 0.0;
      for (j = windowStart; j <= today; j += 1) {
        tempReal += inReal[j];
      }
      shift = tempReal * invPeriod;
      sumXY = 0.0;
      sumX2 = sumXY;
      sumX = sumX2;
      for (j = windowStart; j <= today; j += 1) {
        x = inReal[j] - shift;
        sumX += x;
        sumX2 += x * x;
        sumXY += x * (today - j);
      }
      /* A window flat to within the rounding of its own mean leaves ssX at
       * that rounding, which would fire the trigger again on every bar.
       * Anchored on one of its own values instead, ssX is at least half the
       * squared range and sumX2 at most n times it, so it cannot, short of
       * squares that underflow.
       */
      if (sumX2 - sumX * sumX * invPeriod < 1e-6 * sumX2) {
        shift = inReal[today];
        sumXY = 0.0;
        sumX2 = sumXY;
        sumX = sumX2;
        for (j = windowStart; j <= today; j += 1) {
          x = inReal[j] - shift;
          sumX += x;
          sumX2 += x * x;
          sumXY += x * (today - j);
        }
      }
      peakX2 = sumX2;
      ssX = sumX2 - sumX * sumX * invPeriod;
      spXY = sumXY - sumX * sumY * invPeriod;
    }
    /* Save the trailing value before writing the output, since the input and
     * output might be the same array.
     */
    trailingX = inReal[trailingIdx] - shift;
    trailingIdx += 1;
    /* THE SIGN. y here is BARS AGO, so it runs backward in time and a rising
     * series correlates NEGATIVELY with it. Ehlers' listing counts the same
     * way and takes Y = -count, the positively-sloped line the indicator is
     * defined against; negating the coefficient once is the same thing, and
     * keeps the slide below in its bars-ago form. Without it every value is
     * reversed and every magnitude kept.
     *
     * ssY is a positive constant, so only ssX can make the window degenerate.
     * It is tested against its own scale because an absolute band would
     * reject a well-defined correlation on small-valued data. The product is
     * tested too, since at n = 2 the smallest subnormal ssX rounds it to
     * zero.
     *
     * An all-flat window therefore emits exactly 0.0 rather than NaN, which
     * is correl.c's precedent and what #112 requires of a successful call.
     */
    if (ssX > 1e-14 * sumX2 && ssX * ssY > 0.0) {
      tempReal = -spXY / Math.sqrt(ssX * ssY);
      /* A correlation coefficient cannot leave [-1,1]; rounding in the
       * three sums can still put it slightly outside.
       */
      if (tempReal > 1.0) {
        tempReal = 1.0;
      } else if (tempReal < -1.0) {
        tempReal = -1.0;
      }
      outReal[((outIdx++ * outStride) | 0)] = tempReal;
    } else {
      outReal[((outIdx++ * outStride) | 0)] = 0.0;
    }
    /* Slide the window one bar in O(1). Advancing it ages every retained
     * value by one bar, which raises each of their weights by 1 and so adds
     * the whole deviation sum; the departing value leaves at the full weight
     * optInTimePeriod-1 and also loses its place in that sum, which is the
     * remaining -1. The two together are exactly -optInTimePeriod*trailingX,
     * and sumX must still be the COMPLETE window's sum when it is read here
     * -- it is decremented on the line below, not above.
     */
    sumXY = sumXY + sumX - dPeriod * trailingX;
    sumX -= trailingX;
    sumX2 -= trailingX * trailingX;
    today += 1;
  } while (today <= endIdx);
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
  sp.sumX = sumX;
  sp.sumX2 = sumX2;
  sp.sumXY = sumXY;
  sp.peakX2 = peakX2;
  sp.shift = shift;
  sp.invPeriod = invPeriod;
  sp.dPeriod = dPeriod;
  sp.sumY = sumY;
  sp.ssY = ssY;
  sp.lookbackTotal = lookbackTotal;
  sp.trailingIdx = trailingIdx;
  sp.barsSinceReseed = barsSinceReseed;
  sp.j = j;
  sp.today = today;
  sp.xMask = physX - 1;
  sp.x_inReal = capX_inReal;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function ctiOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): CtiStream {
  const sp = CtiStream.of(core);
  const retCode = ctiOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CTI openAndFill', inReal.length, startIdx, ctiLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && ctiLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('CTI openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('CTI openAndFill', retCode);
}

/** @internal */
export function ctiOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): CtiStream {
  const sp = CtiStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = ctiOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CTI open', inReal.length, startIdx, ctiLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && ctiLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('CTI open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('CTI open', retCode);
}

/** @internal */
export function ctiOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): CtiStream {
  optInTimePeriod = requireInt32('CTI open', 'timePeriod', optInTimePeriod);
  requireArgument('CTI open', 'real', inReal);
  requireHistory('CTI open', 'real', inReal.length);
  return ctiOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/** @internal */
export function ctiOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): CtiStream {
  optInTimePeriod = requireInt32('CTI openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('CTI openAndFill', 'real', inReal);
  requireHistory('CTI openAndFill', 'real', inReal.length);
  const guardLookback = ctiLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('CTI openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('CTI openAndFill', inReal.length, guardLookback);
  requireOutput('CTI openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('CTI openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('CTI openAndFill', 'outReal', 'real');
  }
  return ctiOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live CTI stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link cti} there.
 *
 * @param real - Warm-up history. The series to measure the trend of.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CtiOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `ctiLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { ctiOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = ctiOpen(data, { timePeriod: 20 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function ctiOpen(real: ArrayLike<number>, options?: CtiOptions): CtiStream {
  const o = optionsOf('CTI open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('CTI open', o.core);
  return ctiOpenPositional(real, optInTimePeriod, core);
}

/**
 * {@link ctiOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link cti}.
 *
 * @param real - Warm-up history. The series to measure the trend of.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CtiOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cti} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `ctiLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or a
 *   buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { ctiOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = ctiOpenAndFill(data, { timePeriod: 20 });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function ctiOpenAndFill(real: ArrayLike<number>, options?: CtiOptions): CtiResult & { stream: CtiStream } {
  const o = optionsOf('CTI openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('CTI openAndFill', o.core);
  const capacity = fillCapacity(ctiLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = ctiOpenAndFillPositional(real, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
