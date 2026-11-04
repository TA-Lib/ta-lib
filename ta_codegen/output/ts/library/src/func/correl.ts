/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 *
 * Change history:
 *
 *  MMDDYY BY    Description
 *  -------------------------------------------------------------------
 *  120802 MF    Template creation.
 *  101003 MF    Initial Coding
 *  062804 MF    Resolve div by zero bug on limit case.
 *  082326 MF    Fix #242. Cancellation-free sums (shifted data + reseed, as
 *               TA_VAR does since #118), per-factor degeneracy test and a
 *               range clamp.
 *  090626 MF,CC Fix #395. Test the product too: it underflows to 0.0 while ssX
 *               and ssY are still ordinary normals, and the divide then
 *               returned NaN under TA_SUCCESS -- which the range clamp cannot
 *               catch -- or a perfect correlation from a degenerate window.
 *  092226 MF,CC #434 rebuild against each side's peak sum of squares.
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link correl} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Rolling window length (default 30; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function correlLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  return optInTimePeriod - 1;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link correl}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Rolling window length (default 30; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function correlDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (correlLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function correlImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inReal0: Float64Array,
  inReal1: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let sumXY = 0;
  let sumX = 0;
  let sumY = 0;
  let sumX2 = 0;
  let sumY2 = 0;
  let x = 0;
  let y = 0;
  let trailingX = 0;
  let trailingY = 0;
  let shiftX = 0;
  let shiftY = 0;
  let ssX = 0;
  let ssY = 0;
  let spXY = 0;
  let peakX2 = 0;
  let peakY2 = 0;
  let tempReal = 0;
  let invPeriod = 0;
  let lookbackTotal = 0;
  let today = 0;
  let trailingIdx = 0;
  let outIdx = 0;
  let j = 0;
  let windowStart = 0;
  let barsSinceReseed = 0;
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
  /* Move up the start index if there is not
   * enough initial data.
   */
  /* One reciprocal instead of three divisions per bar, as TA_VAR does. The
   * extra rounding it costs is invisible next to what the shift recovers, and
   * it is what keeps this form cheaper than the one it replaces.
   */
  invPeriod = 1.0 / optInTimePeriod;
  lookbackTotal = optInTimePeriod - 1;
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  outBegIdx.value = startIdx;
  trailingIdx = startIdx - lookbackTotal;
  /* Measure both series against a shift near the window, exactly as TA_VAR
   * does (#118). The running sums then hold deviations rather than raw levels,
   * so ssX = sumX2-(sumX*sumX)*invPeriod is no longer a difference of two
   * ~period*mean^2 quantities. Without this the extracted sum of squares keeps
   * only the digits that survive that subtraction: at a $100 price level with a
   * 1e-5 spread that is three of them, and the correlation of two perfectly
   * correlated series came back as 0, as -1, or as -1.73 (#242).
   *
   * Anchor on the first window value here; a rebuild anchors on the window
   * mean instead (or on a window value when the mean leaves the window
   * flat), which is better centred but costs a pass this one cannot afford
   * before the sums exist.
   */
  shiftX = inReal0[trailingIdx];
  shiftY = inReal1[trailingIdx];
  /* Calculate the initial values (the window less its last bar). */
  sumY2 = 0.0;
  sumX2 = sumY2;
  sumY = sumX2;
  sumX = sumY;
  sumXY = sumX;
  for (j = trailingIdx; j < startIdx; j += 1) {
    x = inReal0[j] - shiftX;
    sumX += x;
    sumX2 += x * x;
    y = inReal1[j] - shiftY;
    sumXY += x * y;
    sumY += y;
    sumY2 += y * y;
  }
  today = startIdx;
  outIdx = 0;
  barsSinceReseed = ((32 * optInTimePeriod) | 0);
  peakX2 = sumX2;
  peakY2 = sumY2;
  do {
    /* Add the incoming value, measured against the shift. */
    x = inReal0[today] - shiftX;
    sumX += x;
    sumX2 += x * x;
    y = inReal1[today] - shiftY;
    sumXY += x * y;
    sumY += y;
    sumY2 += y * y;
    peakX2 = (sumX2 > peakX2) ? sumX2 : peakX2;
    peakY2 = (sumY2 > peakY2) ? sumY2 : peakY2;
    ssX = sumX2 - sumX * sumX * invPeriod;
    ssY = sumY2 - sumY * sumY * invPeriod;
    spXY = sumXY - sumX * sumY * invPeriod;
    /* Rebuild with a fresh two-pass when either sum of squares has shrunk
     * below 1e-6 of the LARGEST one held since the last rebuild, or at least
     * every 32 windows. Measure against that peak, not the current sum: the
     * rounding the running sums carry scales with the peak, so once a series
     * settles back near its shift, or an outlier leaves the window, the
     * current sum holds nothing but that rounding. Each side keeps its own
     * peak: one peak shared by two series of different scale fires on every
     * bar. The collapse is seen on the first bar whose sums carry it, and the
     * rebuild recomputes that bar's output before it is written.
     *
     * The triggers watch ssX and ssY only, never spXY. A vanishing spXY is a
     * legitimate answer - two uncorrelated series - not a loss of digits, and
     * reseeding on it would rebuild the window on every bar of ordinary data.
     * This is where the analogy with TA_VAR stops: variance has one extracted
     * quantity and all of it is signal.
     *
     * Reading the window here is safe when outReal aliases an input: the
     * outputs written so far occupy [0, outIdx-1] while windowStart is
     * startIdx-lookbackTotal+outIdx, which is >= outIdx.
     */
    barsSinceReseed -= 1;
    if (ssX < 1e-6 * peakX2 || ssY < 1e-6 * peakY2 || barsSinceReseed <= 0) {
      barsSinceReseed = ((32 * optInTimePeriod) | 0);
      windowStart = today - lookbackTotal;
      /* Both means in one pass over the window: the rebuild below is the
       * only O(period) work on this function's hot path, so it is walked
       * twice, not three times.
       */
      tempReal = 0.0;
      shiftY = 0.0;
      for (j = windowStart; j <= today; j += 1) {
        tempReal += inReal0[j];
        shiftY += inReal1[j];
      }
      shiftX = tempReal * invPeriod;
      shiftY = shiftY * invPeriod;
      sumY2 = 0.0;
      sumX2 = sumY2;
      sumY = sumX2;
      sumX = sumY;
      sumXY = sumX;
      for (j = windowStart; j <= today; j += 1) {
        x = inReal0[j] - shiftX;
        sumX += x;
        sumX2 += x * x;
        y = inReal1[j] - shiftY;
        sumXY += x * y;
        sumY += y;
        sumY2 += y * y;
      }
      ssX = sumX2 - sumX * sumX * invPeriod;
      ssY = sumY2 - sumY * sumY * invPeriod;
      spXY = sumXY - sumX * sumY * invPeriod;
      /* A side flat to within the rounding of its own mean leaves its sum of
       * squares at that rounding, which would fire the trigger again on every
       * bar. Anchored on one of its own values it cannot, short of squares
       * that underflow. sumXY depends on both shifts, so all five sums are
       * redone.
       */
      if (ssX < 1e-6 * sumX2 || ssY < 1e-6 * sumY2) {
        if (ssX < 1e-6 * sumX2) {
          shiftX = inReal0[today];
        }
        if (ssY < 1e-6 * sumY2) {
          shiftY = inReal1[today];
        }
        sumY2 = 0.0;
        sumX2 = sumY2;
        sumY = sumX2;
        sumX = sumY;
        sumXY = sumX;
        for (j = windowStart; j <= today; j += 1) {
          x = inReal0[j] - shiftX;
          sumX += x;
          sumX2 += x * x;
          y = inReal1[j] - shiftY;
          sumXY += x * y;
          sumY += y;
          sumY2 += y * y;
        }
        ssX = sumX2 - sumX * sumX * invPeriod;
        ssY = sumY2 - sumY * sumY * invPeriod;
        spXY = sumXY - sumX * sumY * invPeriod;
      }
      peakX2 = sumX2;
      peakY2 = sumY2;
      /* A sum of squares is non-negative by definition, but this one is
       * extracted as a difference, so its SIGN is not guaranteed on a window
       * sitting inside a flat stretch. Enforce the invariant HERE and not at
       * the divide: a negative ssX always reseeds on the same bar, because
       * the peak it is compared with is never negative, so the divide below
       * can rely on both being >= 0 and needs no sign test of its own.
       * CHANGING THE TRIGGERS MEANS RE-CHECKING THIS.
       */
      if (ssX < 0.0) {
        ssX = 0.0;
      }
      if (ssY < 0.0) {
        ssY = 0.0;
      }
    }
    /* Save the trailing values before writing the output, since the input
     * and output might be the same array.
     */
    trailingX = inReal0[trailingIdx] - shiftX;
    trailingY = inReal1[trailingIdx] - shiftY;
    trailingIdx += 1;
    /* Output the new coefficient.
     *
     * Each sum of squares is tested against its OWN scale, not the pair
     * against a fixed band. The product ssX*ssY carries the fourth power of
     * the window's spread, so an absolute threshold on it rejects a perfectly
     * well-defined correlation as soon as the data is small - and, worse,
     * lets a pair of NEGATIVE sums through, their signs cancelling into a
     * plausible-looking result of the wrong sign. Testing each factor
     * separately is what forecloses both.
     *
     * The literal is TA_EPSILON. This is deliberately NOT TA_IS_ZERO_SCALED,
     * whose fabs() would admit a LARGE NEGATIVE ssX -- exactly the operand
     * that must never reach the square root. A plain `>` rejects it, and it
     * is also the cheaper test: the two fabs() cost ~7% of this function's
     * runtime, and buy a wrong answer.
     *
     * sqrt(ssX*ssY) rather than sqrt(ssX)*sqrt(ssY): the guard has already
     * established both are positive, so the product needs no protection from
     * a negative operand, and the second square root is worth ~14% of this
     * function's runtime (measured, #395).
     *
     * The product is then tested on its own, because neither factor's test
     * implies it: at that fourth power it underflows to exactly 0.0 while ssX
     * and ssY are still ordinary normals (#395). A zero divisor there gives
     * NaN, which the clamp below does NOT catch -- NaN fails both comparisons
     * -- or an infinity the clamp rewrites into a perfect correlation. Exact,
     * not a band: an absolute band on the product is the #253 defect at a new
     * address. At the other end the product overflows to +Inf and the quotient
     * is 0.0, the degenerate answer anyway.
     */
    if (ssX > 1e-14 * sumX2 && ssY > 1e-14 * sumY2 && ssX * ssY > 0.0) {
      tempReal = spXY / Math.sqrt(ssX * ssY);
      /* A correlation coefficient cannot leave [-1,1]; rounding in the
       * three sums can still put it a few ulp outside.
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
    /* Remove the trailing values (prepares the next window). */
    sumX -= trailingX;
    sumX2 -= trailingX * trailingX;
    sumXY -= trailingX * trailingY;
    sumY -= trailingY;
    sumY2 -= trailingY * trailingY;
    today += 1;
  } while (today <= endIdx);
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * Pearson's correlation coefficient (r) between two input series over a rolling
 * window of optInTimePeriod bars. Measures how linearly the two series move
 * together. r near +1: strong positive co-movement; near -1: strong inverse; near
 * 0: no linear relationship.
 *
 * Formula and more info at
 * [ta-lib.org/functions/correl](https://ta-lib.org/functions/correl).
 *
 * **Notes**
 * - When the correlation is undefined for a window (for example a constant
 *   series), the output is 0 rather than an error or NaN.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link correlLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal0 - First data series (X)
 * @param inReal1 - Second data series (Y)
 * @param optInTimePeriod - Rolling window length (default 30; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Correlation coefficient r in \[-1, 1]. Must hold the values
 *   produced and never be empty; the input length always suffices.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The range written: `begIdx` is the first bar with a value, `count` how
 *   many were written.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for an argument that is not an int32, a
 *   parameter outside its range, an array too short for the range, an empty output
 *   or an output sharing memory with a `Float64Array` input it is not. Checked
 *   before anything is written.
 *
 * @example
 * ```ts
 * import { correl } from 'ta-lib';
 *
 * const data0 = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const data1 = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i + 0.7));
 * const out = new Float64Array(data0.length);
 * const { begIdx, count } = correl(0, data0.length - 1, data0, data1, 30, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`beta`](https://ta-lib.org/functions/beta)
 * @see [`stddev`](https://ta-lib.org/functions/stddev)
 * @see [`var_`](https://ta-lib.org/functions/var)
 */
export function correl(
  startIdx: number,
  endIdx: number,
  inReal0: ArrayLike<number>,
  inReal1: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CORREL', 'startIdx', startIdx);
  endIdx = requireInt32('CORREL', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('CORREL', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('CORREL', startIdx, endIdx);
  const guardStart = clampedStart('CORREL', startIdx, correlLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CORREL', 'inReal0', inReal0, guardInLen);
  requireLength('CORREL', 'inReal1', inReal1, guardInLen);
  requireOutput('CORREL', 'outReal', outReal, guardOutLen, Float64Array);
  const inReal0F64 = float64(inReal0);
  const inReal1F64 = float64(inReal1);
  if (overlapsPartly(outReal, inReal0F64) || overlapsPartly(outReal, inReal1F64)) {
    throw failure('CORREL', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = correlImpl(core, startIdx, endIdx, inReal0F64, inReal1F64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CORREL', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live CORREL stream: one value per closed bar, bit-identical to {@link correl}
 * over the same series. Open it with {@link correlOpen} or
 * {@link correlOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/correl](https://ta-lib.org/functions/correl).
 */
export class CorrelStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  sumXY: number = 0;
  /** @internal */
  sumX: number = 0;
  /** @internal */
  sumY: number = 0;
  /** @internal */
  sumX2: number = 0;
  /** @internal */
  sumY2: number = 0;
  /** @internal */
  shiftX: number = 0;
  /** @internal */
  shiftY: number = 0;
  /** @internal */
  peakX2: number = 0;
  /** @internal */
  peakY2: number = 0;
  /** @internal */
  invPeriod: number = 0;
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
  x_inReal0: Float64Array = new Float64Array(1);
  /** @internal */
  x_inReal1: Float64Array = new Float64Array(1);
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
  static of(core: Core): CorrelStream {
    return new CorrelStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link correl} reports them over the same bars. Each accepted `update` and each
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
      throw failure('CORREL advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal0 - The bar's `inReal0` value: First data series (X)
   * @param inReal1 - The bar's `inReal1` value: Second data series (Y)
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal0: number, inReal1: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('CORREL update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal0) || !Number.isFinite(inReal1)) {
      throw nonFiniteBar('CORREL update', !Number.isFinite(inReal0) ? 'inReal0' : 'inReal1');
    }
    correlStepImpl(this, inReal0, inReal1);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal0 - The bar's `inReal0` value: First data series (X)
   * @param inReal1 - The bar's `inReal1` value: Second data series (Y)
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal0: number, inReal1: number): number {
    if (!Number.isFinite(inReal0) || !Number.isFinite(inReal1)) {
      throw nonFiniteBar('CORREL peek', !Number.isFinite(inReal0) ? 'inReal0' : 'inReal1');
    }
    const sp = this;
    let x = 0;
    let y = 0;
    let ssX = 0;
    let ssY = 0;
    let spXY = 0;
    let tempReal = 0;
    let windowStart = 0;
    let barsSinceReseed = sp.barsSinceReseed;
    let cur_outReal = 0;
    let j = sp.j;
    let peakX2 = sp.peakX2;
    let peakY2 = sp.peakY2;
    let shiftX = sp.shiftX;
    let shiftY = sp.shiftY;
    let sumX = sp.sumX;
    let sumX2 = sp.sumX2;
    let sumXY = sp.sumXY;
    let sumY = sp.sumY;
    let sumY2 = sp.sumY2;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    let pkSlot1 = -1;
    let pkVal1 = 0;
    pkSlot0 = sp.today & sp.xMask;
    pkVal0 = inReal0;
    pkSlot1 = sp.today & sp.xMask;
    pkVal1 = inReal1;
    /* Add the incoming value, measured against the shift. */
    x = (((sp.today & sp.xMask) !== pkSlot0) ? sp.x_inReal0[sp.today & sp.xMask] : pkVal0) - shiftX;
    sumX += x;
    sumX2 += x * x;
    y = (((sp.today & sp.xMask) !== pkSlot1) ? sp.x_inReal1[sp.today & sp.xMask] : pkVal1) - shiftY;
    sumXY += x * y;
    sumY += y;
    sumY2 += y * y;
    peakX2 = (sumX2 > peakX2) ? sumX2 : peakX2;
    peakY2 = (sumY2 > peakY2) ? sumY2 : peakY2;
    ssX = sumX2 - sumX * sumX * sp.invPeriod;
    ssY = sumY2 - sumY * sumY * sp.invPeriod;
    spXY = sumXY - sumX * sumY * sp.invPeriod;
    /* Rebuild with a fresh two-pass when either sum of squares has shrunk
     * below 1e-6 of the LARGEST one held since the last rebuild, or at least
     * every 32 windows. Measure against that peak, not the current sum: the
     * rounding the running sums carry scales with the peak, so once a series
     * settles back near its shift, or an outlier leaves the window, the
     * current sum holds nothing but that rounding. Each side keeps its own
     * peak: one peak shared by two series of different scale fires on every
     * bar. The collapse is seen on the first bar whose sums carry it, and the
     * rebuild recomputes that bar's output before it is written.
     *
     * The triggers watch ssX and ssY only, never spXY. A vanishing spXY is a
     * legitimate answer - two uncorrelated series - not a loss of digits, and
     * reseeding on it would rebuild the window on every bar of ordinary data.
     * This is where the analogy with TA_VAR stops: variance has one extracted
     * quantity and all of it is signal.
     *
     * Reading the window here is safe when outReal aliases an input: the
     * outputs written so far occupy [0, outIdx-1] while windowStart is
     * startIdx-lookbackTotal+outIdx, which is >= outIdx.
     */
    barsSinceReseed -= 1;
    if (ssX < 1e-6 * peakX2 || ssY < 1e-6 * peakY2 || barsSinceReseed <= 0) {
      barsSinceReseed = ((32 * sp.optInTimePeriod) | 0);
      windowStart = sp.today - sp.lookbackTotal;
      /* Both means in one pass over the window: the rebuild below is the
       * only O(period) work on this function's hot path, so it is walked
       * twice, not three times.
       */
      tempReal = 0.0;
      shiftY = 0.0;
      for (j = windowStart; j <= sp.today; j += 1) {
        tempReal += ((j & sp.xMask) !== pkSlot0) ? sp.x_inReal0[j & sp.xMask] : pkVal0;
        shiftY += ((j & sp.xMask) !== pkSlot1) ? sp.x_inReal1[j & sp.xMask] : pkVal1;
      }
      shiftX = tempReal * sp.invPeriod;
      shiftY = shiftY * sp.invPeriod;
      sumY2 = 0.0;
      sumX2 = sumY2;
      sumY = sumX2;
      sumX = sumY;
      sumXY = sumX;
      for (j = windowStart; j <= sp.today; j += 1) {
        x = (((j & sp.xMask) !== pkSlot0) ? sp.x_inReal0[j & sp.xMask] : pkVal0) - shiftX;
        sumX += x;
        sumX2 += x * x;
        y = (((j & sp.xMask) !== pkSlot1) ? sp.x_inReal1[j & sp.xMask] : pkVal1) - shiftY;
        sumXY += x * y;
        sumY += y;
        sumY2 += y * y;
      }
      ssX = sumX2 - sumX * sumX * sp.invPeriod;
      ssY = sumY2 - sumY * sumY * sp.invPeriod;
      spXY = sumXY - sumX * sumY * sp.invPeriod;
      /* A side flat to within the rounding of its own mean leaves its sum of
       * squares at that rounding, which would fire the trigger again on every
       * bar. Anchored on one of its own values it cannot, short of squares
       * that underflow. sumXY depends on both shifts, so all five sums are
       * redone.
       */
      if (ssX < 1e-6 * sumX2 || ssY < 1e-6 * sumY2) {
        if (ssX < 1e-6 * sumX2) {
          shiftX = ((sp.today & sp.xMask) !== pkSlot0) ? sp.x_inReal0[sp.today & sp.xMask] : pkVal0;
        }
        if (ssY < 1e-6 * sumY2) {
          shiftY = ((sp.today & sp.xMask) !== pkSlot1) ? sp.x_inReal1[sp.today & sp.xMask] : pkVal1;
        }
        sumY2 = 0.0;
        sumX2 = sumY2;
        sumY = sumX2;
        sumX = sumY;
        sumXY = sumX;
        for (j = windowStart; j <= sp.today; j += 1) {
          x = (((j & sp.xMask) !== pkSlot0) ? sp.x_inReal0[j & sp.xMask] : pkVal0) - shiftX;
          sumX += x;
          sumX2 += x * x;
          y = (((j & sp.xMask) !== pkSlot1) ? sp.x_inReal1[j & sp.xMask] : pkVal1) - shiftY;
          sumXY += x * y;
          sumY += y;
          sumY2 += y * y;
        }
        ssX = sumX2 - sumX * sumX * sp.invPeriod;
        ssY = sumY2 - sumY * sumY * sp.invPeriod;
        spXY = sumXY - sumX * sumY * sp.invPeriod;
      }
      peakX2 = sumX2;
      peakY2 = sumY2;
      /* A sum of squares is non-negative by definition, but this one is
       * extracted as a difference, so its SIGN is not guaranteed on a window
       * sitting inside a flat stretch. Enforce the invariant HERE and not at
       * the divide: a negative ssX always reseeds on the same bar, because
       * the peak it is compared with is never negative, so the divide below
       * can rely on both being >= 0 and needs no sign test of its own.
       * CHANGING THE TRIGGERS MEANS RE-CHECKING THIS.
       */
      if (ssX < 0.0) {
        ssX = 0.0;
      }
      if (ssY < 0.0) {
        ssY = 0.0;
      }
    }
    /* Output the new coefficient.
     *
     * Each sum of squares is tested against its OWN scale, not the pair
     * against a fixed band. The product ssX*ssY carries the fourth power of
     * the window's spread, so an absolute threshold on it rejects a perfectly
     * well-defined correlation as soon as the data is small - and, worse,
     * lets a pair of NEGATIVE sums through, their signs cancelling into a
     * plausible-looking result of the wrong sign. Testing each factor
     * separately is what forecloses both.
     *
     * The literal is TA_EPSILON. This is deliberately NOT TA_IS_ZERO_SCALED,
     * whose fabs() would admit a LARGE NEGATIVE ssX -- exactly the operand
     * that must never reach the square root. A plain `>` rejects it, and it
     * is also the cheaper test: the two fabs() cost ~7% of this function's
     * runtime, and buy a wrong answer.
     *
     * sqrt(ssX*ssY) rather than sqrt(ssX)*sqrt(ssY): the guard has already
     * established both are positive, so the product needs no protection from
     * a negative operand, and the second square root is worth ~14% of this
     * function's runtime (measured, #395).
     *
     * The product is then tested on its own, because neither factor's test
     * implies it: at that fourth power it underflows to exactly 0.0 while ssX
     * and ssY are still ordinary normals (#395). A zero divisor there gives
     * NaN, which the clamp below does NOT catch -- NaN fails both comparisons
     * -- or an infinity the clamp rewrites into a perfect correlation. Exact,
     * not a band: an absolute band on the product is the #253 defect at a new
     * address. At the other end the product overflows to +Inf and the quotient
     * is 0.0, the degenerate answer anyway.
     */
    if (ssX > 1e-14 * sumX2 && ssY > 1e-14 * sumY2 && ssX * ssY > 0.0) {
      tempReal = spXY / Math.sqrt(ssX * ssY);
      /* A correlation coefficient cannot leave [-1,1]; rounding in the
       * three sums can still put it a few ulp outside.
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
  clone(): CorrelStream {
    const c = new CorrelStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.sumXY = this.sumXY;
    c.sumX = this.sumX;
    c.sumY = this.sumY;
    c.sumX2 = this.sumX2;
    c.sumY2 = this.sumY2;
    c.shiftX = this.shiftX;
    c.shiftY = this.shiftY;
    c.peakX2 = this.peakX2;
    c.peakY2 = this.peakY2;
    c.invPeriod = this.invPeriod;
    c.lookbackTotal = this.lookbackTotal;
    c.trailingIdx = this.trailingIdx;
    c.barsSinceReseed = this.barsSinceReseed;
    c.j = this.j;
    c.today = this.today;
    c.xMask = this.xMask;
    c.x_inReal0 = this.x_inReal0.slice();
    c.x_inReal1 = this.x_inReal1.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function correlStepImpl(sp: CorrelStream, inReal0: number, inReal1: number): void {
  let x = 0;
  let y = 0;
  let trailingX = 0;
  let trailingY = 0;
  let ssX = 0;
  let ssY = 0;
  let spXY = 0;
  let tempReal = 0;
  let windowStart = 0;
  sp.x_inReal0[sp.today & sp.xMask] = inReal0;
  sp.x_inReal1[sp.today & sp.xMask] = inReal1;
  /* Add the incoming value, measured against the shift. */
  x = sp.x_inReal0[sp.today & sp.xMask] - sp.shiftX;
  sp.sumX += x;
  sp.sumX2 += x * x;
  y = sp.x_inReal1[sp.today & sp.xMask] - sp.shiftY;
  sp.sumXY += x * y;
  sp.sumY += y;
  sp.sumY2 += y * y;
  sp.peakX2 = (sp.sumX2 > sp.peakX2) ? sp.sumX2 : sp.peakX2;
  sp.peakY2 = (sp.sumY2 > sp.peakY2) ? sp.sumY2 : sp.peakY2;
  ssX = sp.sumX2 - sp.sumX * sp.sumX * sp.invPeriod;
  ssY = sp.sumY2 - sp.sumY * sp.sumY * sp.invPeriod;
  spXY = sp.sumXY - sp.sumX * sp.sumY * sp.invPeriod;
  /* Rebuild with a fresh two-pass when either sum of squares has shrunk
   * below 1e-6 of the LARGEST one held since the last rebuild, or at least
   * every 32 windows. Measure against that peak, not the current sum: the
   * rounding the running sums carry scales with the peak, so once a series
   * settles back near its shift, or an outlier leaves the window, the
   * current sum holds nothing but that rounding. Each side keeps its own
   * peak: one peak shared by two series of different scale fires on every
   * bar. The collapse is seen on the first bar whose sums carry it, and the
   * rebuild recomputes that bar's output before it is written.
   *
   * The triggers watch ssX and ssY only, never spXY. A vanishing spXY is a
   * legitimate answer - two uncorrelated series - not a loss of digits, and
   * reseeding on it would rebuild the window on every bar of ordinary data.
   * This is where the analogy with TA_VAR stops: variance has one extracted
   * quantity and all of it is signal.
   *
   * Reading the window here is safe when outReal aliases an input: the
   * outputs written so far occupy [0, outIdx-1] while windowStart is
   * startIdx-lookbackTotal+outIdx, which is >= outIdx.
   */
  sp.barsSinceReseed -= 1;
  if (ssX < 1e-6 * sp.peakX2 || ssY < 1e-6 * sp.peakY2 || sp.barsSinceReseed <= 0) {
    sp.barsSinceReseed = ((32 * sp.optInTimePeriod) | 0);
    windowStart = sp.today - sp.lookbackTotal;
    /* Both means in one pass over the window: the rebuild below is the
     * only O(period) work on this function's hot path, so it is walked
     * twice, not three times.
     */
    tempReal = 0.0;
    sp.shiftY = 0.0;
    for (sp.j = windowStart; sp.j <= sp.today; sp.j += 1) {
      tempReal += sp.x_inReal0[sp.j & sp.xMask];
      sp.shiftY += sp.x_inReal1[sp.j & sp.xMask];
    }
    sp.shiftX = tempReal * sp.invPeriod;
    sp.shiftY = sp.shiftY * sp.invPeriod;
    sp.sumY2 = 0.0;
    sp.sumX2 = sp.sumY2;
    sp.sumY = sp.sumX2;
    sp.sumX = sp.sumY;
    sp.sumXY = sp.sumX;
    for (sp.j = windowStart; sp.j <= sp.today; sp.j += 1) {
      x = sp.x_inReal0[sp.j & sp.xMask] - sp.shiftX;
      sp.sumX += x;
      sp.sumX2 += x * x;
      y = sp.x_inReal1[sp.j & sp.xMask] - sp.shiftY;
      sp.sumXY += x * y;
      sp.sumY += y;
      sp.sumY2 += y * y;
    }
    ssX = sp.sumX2 - sp.sumX * sp.sumX * sp.invPeriod;
    ssY = sp.sumY2 - sp.sumY * sp.sumY * sp.invPeriod;
    spXY = sp.sumXY - sp.sumX * sp.sumY * sp.invPeriod;
    /* A side flat to within the rounding of its own mean leaves its sum of
     * squares at that rounding, which would fire the trigger again on every
     * bar. Anchored on one of its own values it cannot, short of squares
     * that underflow. sumXY depends on both shifts, so all five sums are
     * redone.
     */
    if (ssX < 1e-6 * sp.sumX2 || ssY < 1e-6 * sp.sumY2) {
      if (ssX < 1e-6 * sp.sumX2) {
        sp.shiftX = sp.x_inReal0[sp.today & sp.xMask];
      }
      if (ssY < 1e-6 * sp.sumY2) {
        sp.shiftY = sp.x_inReal1[sp.today & sp.xMask];
      }
      sp.sumY2 = 0.0;
      sp.sumX2 = sp.sumY2;
      sp.sumY = sp.sumX2;
      sp.sumX = sp.sumY;
      sp.sumXY = sp.sumX;
      for (sp.j = windowStart; sp.j <= sp.today; sp.j += 1) {
        x = sp.x_inReal0[sp.j & sp.xMask] - sp.shiftX;
        sp.sumX += x;
        sp.sumX2 += x * x;
        y = sp.x_inReal1[sp.j & sp.xMask] - sp.shiftY;
        sp.sumXY += x * y;
        sp.sumY += y;
        sp.sumY2 += y * y;
      }
      ssX = sp.sumX2 - sp.sumX * sp.sumX * sp.invPeriod;
      ssY = sp.sumY2 - sp.sumY * sp.sumY * sp.invPeriod;
      spXY = sp.sumXY - sp.sumX * sp.sumY * sp.invPeriod;
    }
    sp.peakX2 = sp.sumX2;
    sp.peakY2 = sp.sumY2;
    /* A sum of squares is non-negative by definition, but this one is
     * extracted as a difference, so its SIGN is not guaranteed on a window
     * sitting inside a flat stretch. Enforce the invariant HERE and not at
     * the divide: a negative ssX always reseeds on the same bar, because
     * the peak it is compared with is never negative, so the divide below
     * can rely on both being >= 0 and needs no sign test of its own.
     * CHANGING THE TRIGGERS MEANS RE-CHECKING THIS.
     */
    if (ssX < 0.0) {
      ssX = 0.0;
    }
    if (ssY < 0.0) {
      ssY = 0.0;
    }
  }
  /* Save the trailing values before writing the output, since the input
   * and output might be the same array.
   */
  trailingX = sp.x_inReal0[sp.trailingIdx & sp.xMask] - sp.shiftX;
  trailingY = sp.x_inReal1[sp.trailingIdx & sp.xMask] - sp.shiftY;
  sp.trailingIdx += 1;
  /* Output the new coefficient.
   *
   * Each sum of squares is tested against its OWN scale, not the pair
   * against a fixed band. The product ssX*ssY carries the fourth power of
   * the window's spread, so an absolute threshold on it rejects a perfectly
   * well-defined correlation as soon as the data is small - and, worse,
   * lets a pair of NEGATIVE sums through, their signs cancelling into a
   * plausible-looking result of the wrong sign. Testing each factor
   * separately is what forecloses both.
   *
   * The literal is TA_EPSILON. This is deliberately NOT TA_IS_ZERO_SCALED,
   * whose fabs() would admit a LARGE NEGATIVE ssX -- exactly the operand
   * that must never reach the square root. A plain `>` rejects it, and it
   * is also the cheaper test: the two fabs() cost ~7% of this function's
   * runtime, and buy a wrong answer.
   *
   * sqrt(ssX*ssY) rather than sqrt(ssX)*sqrt(ssY): the guard has already
   * established both are positive, so the product needs no protection from
   * a negative operand, and the second square root is worth ~14% of this
   * function's runtime (measured, #395).
   *
   * The product is then tested on its own, because neither factor's test
   * implies it: at that fourth power it underflows to exactly 0.0 while ssX
   * and ssY are still ordinary normals (#395). A zero divisor there gives
   * NaN, which the clamp below does NOT catch -- NaN fails both comparisons
   * -- or an infinity the clamp rewrites into a perfect correlation. Exact,
   * not a band: an absolute band on the product is the #253 defect at a new
   * address. At the other end the product overflows to +Inf and the quotient
   * is 0.0, the degenerate answer anyway.
   */
  if (ssX > 1e-14 * sp.sumX2 && ssY > 1e-14 * sp.sumY2 && ssX * ssY > 0.0) {
    tempReal = spXY / Math.sqrt(ssX * ssY);
    /* A correlation coefficient cannot leave [-1,1]; rounding in the
     * three sums can still put it a few ulp outside.
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
  /* Remove the trailing values (prepares the next window). */
  sp.sumX -= trailingX;
  sp.sumX2 -= trailingX * trailingX;
  sp.sumXY -= trailingX * trailingY;
  sp.sumY -= trailingY;
  sp.sumY2 -= trailingY * trailingY;
  sp.today += 1;
}

function correlOpenImpl(
  sp: CorrelStream,
  inReal0: Float64Array,
  inReal1: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let sumXY = 0;
  let sumX = 0;
  let sumY = 0;
  let sumX2 = 0;
  let sumY2 = 0;
  let x = 0;
  let y = 0;
  let trailingX = 0;
  let trailingY = 0;
  let shiftX = 0;
  let shiftY = 0;
  let ssX = 0;
  let ssY = 0;
  let spXY = 0;
  let peakX2 = 0;
  let peakY2 = 0;
  let tempReal = 0;
  let invPeriod = 0;
  let lookbackTotal = 0;
  let today = 0;
  let trailingIdx = 0;
  let outIdx = 0;
  let j = 0;
  let windowStart = 0;
  let barsSinceReseed = 0;
  const historyLen = inReal0.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inReal1.length !== inReal0.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Move up the start index if there is not
   * enough initial data.
   */
  /* One reciprocal instead of three divisions per bar, as TA_VAR does. The
   * extra rounding it costs is invisible next to what the shift recovers, and
   * it is what keeps this form cheaper than the one it replaces.
   */
  invPeriod = 1.0 / optInTimePeriod;
  lookbackTotal = optInTimePeriod - 1;
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outBegIdx.value = startIdx;
  trailingIdx = startIdx - lookbackTotal;
  /* Measure both series against a shift near the window, exactly as TA_VAR
   * does (#118). The running sums then hold deviations rather than raw levels,
   * so ssX = sumX2-(sumX*sumX)*invPeriod is no longer a difference of two
   * ~period*mean^2 quantities. Without this the extracted sum of squares keeps
   * only the digits that survive that subtraction: at a $100 price level with a
   * 1e-5 spread that is three of them, and the correlation of two perfectly
   * correlated series came back as 0, as -1, or as -1.73 (#242).
   *
   * Anchor on the first window value here; a rebuild anchors on the window
   * mean instead (or on a window value when the mean leaves the window
   * flat), which is better centred but costs a pass this one cannot afford
   * before the sums exist.
   */
  shiftX = inReal0[trailingIdx];
  shiftY = inReal1[trailingIdx];
  /* Calculate the initial values (the window less its last bar). */
  sumY2 = 0.0;
  sumX2 = sumY2;
  sumY = sumX2;
  sumX = sumY;
  sumXY = sumX;
  for (j = trailingIdx; j < startIdx; j += 1) {
    x = inReal0[j] - shiftX;
    sumX += x;
    sumX2 += x * x;
    y = inReal1[j] - shiftY;
    sumXY += x * y;
    sumY += y;
    sumY2 += y * y;
  }
  today = startIdx;
  outIdx = 0;
  barsSinceReseed = ((32 * optInTimePeriod) | 0);
  peakX2 = sumX2;
  peakY2 = sumY2;
  do {
    /* Add the incoming value, measured against the shift. */
    x = inReal0[today] - shiftX;
    sumX += x;
    sumX2 += x * x;
    y = inReal1[today] - shiftY;
    sumXY += x * y;
    sumY += y;
    sumY2 += y * y;
    peakX2 = (sumX2 > peakX2) ? sumX2 : peakX2;
    peakY2 = (sumY2 > peakY2) ? sumY2 : peakY2;
    ssX = sumX2 - sumX * sumX * invPeriod;
    ssY = sumY2 - sumY * sumY * invPeriod;
    spXY = sumXY - sumX * sumY * invPeriod;
    /* Rebuild with a fresh two-pass when either sum of squares has shrunk
     * below 1e-6 of the LARGEST one held since the last rebuild, or at least
     * every 32 windows. Measure against that peak, not the current sum: the
     * rounding the running sums carry scales with the peak, so once a series
     * settles back near its shift, or an outlier leaves the window, the
     * current sum holds nothing but that rounding. Each side keeps its own
     * peak: one peak shared by two series of different scale fires on every
     * bar. The collapse is seen on the first bar whose sums carry it, and the
     * rebuild recomputes that bar's output before it is written.
     *
     * The triggers watch ssX and ssY only, never spXY. A vanishing spXY is a
     * legitimate answer - two uncorrelated series - not a loss of digits, and
     * reseeding on it would rebuild the window on every bar of ordinary data.
     * This is where the analogy with TA_VAR stops: variance has one extracted
     * quantity and all of it is signal.
     *
     * Reading the window here is safe when outReal aliases an input: the
     * outputs written so far occupy [0, outIdx-1] while windowStart is
     * startIdx-lookbackTotal+outIdx, which is >= outIdx.
     */
    barsSinceReseed -= 1;
    if (ssX < 1e-6 * peakX2 || ssY < 1e-6 * peakY2 || barsSinceReseed <= 0) {
      barsSinceReseed = ((32 * optInTimePeriod) | 0);
      windowStart = today - lookbackTotal;
      /* Both means in one pass over the window: the rebuild below is the
       * only O(period) work on this function's hot path, so it is walked
       * twice, not three times.
       */
      tempReal = 0.0;
      shiftY = 0.0;
      for (j = windowStart; j <= today; j += 1) {
        tempReal += inReal0[j];
        shiftY += inReal1[j];
      }
      shiftX = tempReal * invPeriod;
      shiftY = shiftY * invPeriod;
      sumY2 = 0.0;
      sumX2 = sumY2;
      sumY = sumX2;
      sumX = sumY;
      sumXY = sumX;
      for (j = windowStart; j <= today; j += 1) {
        x = inReal0[j] - shiftX;
        sumX += x;
        sumX2 += x * x;
        y = inReal1[j] - shiftY;
        sumXY += x * y;
        sumY += y;
        sumY2 += y * y;
      }
      ssX = sumX2 - sumX * sumX * invPeriod;
      ssY = sumY2 - sumY * sumY * invPeriod;
      spXY = sumXY - sumX * sumY * invPeriod;
      /* A side flat to within the rounding of its own mean leaves its sum of
       * squares at that rounding, which would fire the trigger again on every
       * bar. Anchored on one of its own values it cannot, short of squares
       * that underflow. sumXY depends on both shifts, so all five sums are
       * redone.
       */
      if (ssX < 1e-6 * sumX2 || ssY < 1e-6 * sumY2) {
        if (ssX < 1e-6 * sumX2) {
          shiftX = inReal0[today];
        }
        if (ssY < 1e-6 * sumY2) {
          shiftY = inReal1[today];
        }
        sumY2 = 0.0;
        sumX2 = sumY2;
        sumY = sumX2;
        sumX = sumY;
        sumXY = sumX;
        for (j = windowStart; j <= today; j += 1) {
          x = inReal0[j] - shiftX;
          sumX += x;
          sumX2 += x * x;
          y = inReal1[j] - shiftY;
          sumXY += x * y;
          sumY += y;
          sumY2 += y * y;
        }
        ssX = sumX2 - sumX * sumX * invPeriod;
        ssY = sumY2 - sumY * sumY * invPeriod;
        spXY = sumXY - sumX * sumY * invPeriod;
      }
      peakX2 = sumX2;
      peakY2 = sumY2;
      /* A sum of squares is non-negative by definition, but this one is
       * extracted as a difference, so its SIGN is not guaranteed on a window
       * sitting inside a flat stretch. Enforce the invariant HERE and not at
       * the divide: a negative ssX always reseeds on the same bar, because
       * the peak it is compared with is never negative, so the divide below
       * can rely on both being >= 0 and needs no sign test of its own.
       * CHANGING THE TRIGGERS MEANS RE-CHECKING THIS.
       */
      if (ssX < 0.0) {
        ssX = 0.0;
      }
      if (ssY < 0.0) {
        ssY = 0.0;
      }
    }
    /* Save the trailing values before writing the output, since the input
     * and output might be the same array.
     */
    trailingX = inReal0[trailingIdx] - shiftX;
    trailingY = inReal1[trailingIdx] - shiftY;
    trailingIdx += 1;
    /* Output the new coefficient.
     *
     * Each sum of squares is tested against its OWN scale, not the pair
     * against a fixed band. The product ssX*ssY carries the fourth power of
     * the window's spread, so an absolute threshold on it rejects a perfectly
     * well-defined correlation as soon as the data is small - and, worse,
     * lets a pair of NEGATIVE sums through, their signs cancelling into a
     * plausible-looking result of the wrong sign. Testing each factor
     * separately is what forecloses both.
     *
     * The literal is TA_EPSILON. This is deliberately NOT TA_IS_ZERO_SCALED,
     * whose fabs() would admit a LARGE NEGATIVE ssX -- exactly the operand
     * that must never reach the square root. A plain `>` rejects it, and it
     * is also the cheaper test: the two fabs() cost ~7% of this function's
     * runtime, and buy a wrong answer.
     *
     * sqrt(ssX*ssY) rather than sqrt(ssX)*sqrt(ssY): the guard has already
     * established both are positive, so the product needs no protection from
     * a negative operand, and the second square root is worth ~14% of this
     * function's runtime (measured, #395).
     *
     * The product is then tested on its own, because neither factor's test
     * implies it: at that fourth power it underflows to exactly 0.0 while ssX
     * and ssY are still ordinary normals (#395). A zero divisor there gives
     * NaN, which the clamp below does NOT catch -- NaN fails both comparisons
     * -- or an infinity the clamp rewrites into a perfect correlation. Exact,
     * not a band: an absolute band on the product is the #253 defect at a new
     * address. At the other end the product overflows to +Inf and the quotient
     * is 0.0, the degenerate answer anyway.
     */
    if (ssX > 1e-14 * sumX2 && ssY > 1e-14 * sumY2 && ssX * ssY > 0.0) {
      tempReal = spXY / Math.sqrt(ssX * ssY);
      /* A correlation coefficient cannot leave [-1,1]; rounding in the
       * three sums can still put it a few ulp outside.
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
    /* Remove the trailing values (prepares the next window). */
    sumX -= trailingX;
    sumX2 -= trailingX * trailingX;
    sumXY -= trailingX * trailingY;
    sumY -= trailingY;
    sumY2 -= trailingY * trailingY;
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
  const capX_inReal0 = new Float64Array(physX);
  const capX_inReal1 = new Float64Array(physX);
  for (let fillJ = historyLen - capX; fillJ < historyLen; fillJ++) {
    capX_inReal0[fillJ & (physX - 1)] = inReal0[fillJ];
    capX_inReal1[fillJ & (physX - 1)] = inReal1[fillJ];
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.sumXY = sumXY;
  sp.sumX = sumX;
  sp.sumY = sumY;
  sp.sumX2 = sumX2;
  sp.sumY2 = sumY2;
  sp.shiftX = shiftX;
  sp.shiftY = shiftY;
  sp.peakX2 = peakX2;
  sp.peakY2 = peakY2;
  sp.invPeriod = invPeriod;
  sp.lookbackTotal = lookbackTotal;
  sp.trailingIdx = trailingIdx;
  sp.barsSinceReseed = barsSinceReseed;
  sp.j = j;
  sp.today = today;
  sp.xMask = physX - 1;
  sp.x_inReal0 = capX_inReal0;
  sp.x_inReal1 = capX_inReal1;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function correlOpenAndFillInternal(
  core: Core,
  inReal0: Float64Array,
  inReal1: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): CorrelStream {
  const sp = CorrelStream.of(core);
  const retCode = correlOpenImpl(sp, inReal0, inReal1, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CORREL openAndFill', inReal0.length, startIdx, correlLookback(optInTimePeriod, core));
  }
  throw streamFailure('CORREL openAndFill', retCode);
}

/** @internal */
export function correlOpenInternal(
  core: Core,
  inReal0: Float64Array,
  inReal1: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): CorrelStream {
  const sp = CorrelStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = correlOpenImpl(sp, inReal0, inReal1, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CORREL open', inReal0.length, startIdx, correlLookback(optInTimePeriod, core));
  }
  throw streamFailure('CORREL open', retCode);
}

/**
 * Opens a live CORREL stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link correl} there.
 *
 * @param inReal0 - Warm-up history. First data series (X) Every history has the
 *   same length.
 * @param inReal1 - Warm-up history. Second data series (Y) Every history has the
 *   same length.
 * @param optInTimePeriod - Rolling window length (default 30; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `correlLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { correlOpen } from 'ta-lib';
 *
 * const data0 = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const data1 = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i + 0.7));
 * const stream = correlOpen(data0, data1, 30);
 * const peeked = stream.peek(100.9, 101.3); // a forming bar: commits nothing
 * const updated = stream.update(100.9, 101.3); // the bar closes
 * ```
 */
export function correlOpen(
  inReal0: ArrayLike<number>,
  inReal1: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): CorrelStream {
  optInTimePeriod = requireInt32('CORREL open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('CORREL open', 'inReal0', inReal0);
  requireHistory('CORREL open', inReal0.length);
  requireArgument('CORREL open', 'inReal1', inReal1);
  requireHistoryLength('CORREL open', 'inReal1', inReal1.length, inReal0.length);
  return correlOpenInternal(core, float64(inReal0), float64(inReal1), 0, optInTimePeriod);
}

/**
 * {@link correlOpen} that also fills the outputs over the whole history in the
 * same pass, bit-identically to {@link correl}. The range written is the stream's
 * `outRange()`.
 *
 * @param inReal0 - Warm-up history. First data series (X) Every history has the
 *   same length.
 * @param inReal1 - Warm-up history. Second data series (Y) Every history has the
 *   same length.
 * @param optInTimePeriod - Rolling window length (default 30; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Correlation coefficient r in \[-1, 1]. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `correlLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range, histories of different lengths or an output too
 *   short or starting where another output or a `Float64Array` input starts, or, on
 *   a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { correlOpenAndFill } from 'ta-lib';
 *
 * const data0 = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const data1 = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i + 0.7));
 * const out = new Float64Array(data0.length);
 * const stream = correlOpenAndFill(data0, data1, 30, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function correlOpenAndFill(
  inReal0: ArrayLike<number>,
  inReal1: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): CorrelStream {
  optInTimePeriod = requireInt32('CORREL openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('CORREL openAndFill', 'inReal0', inReal0);
  requireHistory('CORREL openAndFill', inReal0.length);
  requireArgument('CORREL openAndFill', 'inReal1', inReal1);
  const guardOutLen = openFillCount('CORREL openAndFill', inReal0.length, correlLookback(optInTimePeriod, core));
  requireHistoryLength('CORREL openAndFill', 'inReal1', inReal1.length, inReal0.length);
  requireOutput('CORREL openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inReal0F64 = float64(inReal0);
  const inReal1F64 = float64(inReal1);
  if ((outReal.buffer === inReal0F64.buffer && outReal.byteOffset === inReal0F64.byteOffset) || (outReal.buffer === inReal1F64.buffer && outReal.byteOffset === inReal1F64.byteOffset)) {
    throw streamFailure('CORREL openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inReal0F64) || overlaps(outReal, inReal1F64))) {
    throw streamFailure('CORREL openAndFill', RetCode.BAD_PARAM);
  }
  return correlOpenAndFillInternal(core, inReal0F64, inReal1F64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}
