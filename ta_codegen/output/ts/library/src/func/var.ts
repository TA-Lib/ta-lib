/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  JV       Jesus Viver <324122@cienz.unizar.es>
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  112400 MF     Template creation.
 *  100502 JV     Speed optimization of the algorithm
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  071726 MF,CC  #118 cancellation-free variance (shifted sums + reseed); fixes bug 90.
 *  082326 MF,CC  #243 reseed floor is scale-relative, not `variance < 0`.
 *  092226 MF,CC  #434 rebuild against the peak sum of squares; re-anchor a flat window.
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireReal, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function varLookbackPositional(optInTimePeriod: number, optInNbDev: number, _core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (typeof optInNbDev !== 'number') {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 5;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  if (optInNbDev === Core.REAL_DEFAULT) {
    optInNbDev = 1.0;
  } else if (!(optInNbDev >= Core.REAL_MIN && optInNbDev <= Core.REAL_MAX)) {
    return -1;
  }
  return optInTimePeriod - 1;
}

/** @internal */
export function varDisplayShiftPositional(optInTimePeriod: number, optInNbDev: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (varLookbackPositional(optInTimePeriod, optInNbDev, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function varImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  optInNbDev: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let tempReal = 0;
  let shift = 0;
  let periodTotal1 = 0;
  let periodTotal2 = 0;
  let meanValue1 = 0;
  let variance = 0;
  let invPeriod = 0;
  let peakTotal2 = 0;
  let i = 0;
  let j = 0;
  let outIdx = 0;
  let trailingIdx = 0;
  let windowStart = 0;
  let nbInitialElementNeeded = 0;
  let barsSinceReseed = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 5;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInNbDev === Core.REAL_DEFAULT) {
    optInNbDev = 1.0;
  } else if (!(optInNbDev >= Core.REAL_MIN && optInNbDev <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  /* Identify the minimum number of price bar needed to calculate
   * at least one output.
   */
  nbInitialElementNeeded = optInTimePeriod - 1;
  /* Move up the start index if there is not enough initial data. */
  if (startIdx < nbInitialElementNeeded) {
    startIdx = nbInitialElementNeeded;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  invPeriod = 1.0 / optInTimePeriod;
  /* Measure deviations against a shift near the window: the running sums
   * periodTotal1 = sum(inReal-shift) and periodTotal2 = sum((inReal-shift)^2)
   * stay at variance scale, so variance = periodTotal2/period - mean^2 no longer
   * subtracts two ~mean^2 quantities. Anchor the shift to the first window value
   * (also gives an exact 0 for period 1, with no division by period-1).
   */
  trailingIdx = startIdx - nbInitialElementNeeded;
  shift = inReal[trailingIdx];
  periodTotal1 = 0.0;
  periodTotal2 = 0.0;
  for (j = trailingIdx; j < startIdx; j += 1) {
    tempReal = inReal[j] - shift;
    periodTotal1 += tempReal;
    tempReal *= tempReal;
    periodTotal2 += tempReal;
  }
  /* inReal and outReal may be the same buffer: each trailing value is consumed
   * before its slot is overwritten by the output.
   */
  i = startIdx;
  outIdx = 0;
  barsSinceReseed = ((32 * optInTimePeriod) | 0);
  peakTotal2 = periodTotal2;
  do {
    /* Add the incoming value, measured against the shift. */
    tempReal = inReal[i] - shift;
    periodTotal1 += tempReal;
    tempReal *= tempReal;
    periodTotal2 += tempReal;
    peakTotal2 = (periodTotal2 > peakTotal2) ? periodTotal2 : peakTotal2;
    meanValue1 = periodTotal1 * invPeriod;
    variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
    /* Remove the trailing value (prepares the next window). */
    tempReal = inReal[trailingIdx] - shift;
    periodTotal1 -= tempReal;
    tempReal *= tempReal;
    periodTotal2 -= tempReal;
    trailingIdx += 1;
    /* Rebuild with a fresh two-pass when the variance has shrunk below 1e-6
     * of the LARGEST mean squared deviation held since the last rebuild, or at
     * least every 32 windows. Measure against that peak, not the current sum:
     * the rounding the running sums carry scales with the peak, so once a
     * series settles back near the shift, or an outlier leaves the window,
     * the current sum holds nothing but that rounding. The collapse is seen
     * on the first bar whose sums carry it, and the rebuild recomputes that
     * bar.
     */
    barsSinceReseed -= 1;
    if (variance < 1e-6 * (peakTotal2 * invPeriod) || barsSinceReseed <= 0) {
      barsSinceReseed = ((32 * optInTimePeriod) | 0);
      windowStart = i - nbInitialElementNeeded;
      tempReal = 0.0;
      for (j = windowStart; j <= i; j += 1) {
        tempReal += inReal[j];
      }
      shift = tempReal * invPeriod;
      periodTotal1 = 0.0;
      periodTotal2 = 0.0;
      for (j = windowStart; j <= i; j += 1) {
        tempReal = inReal[j] - shift;
        periodTotal1 += tempReal;
        tempReal *= tempReal;
        periodTotal2 += tempReal;
      }
      meanValue1 = periodTotal1 * invPeriod;
      variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
      /* A window flat to within the rounding of its own mean leaves the
       * variance at that rounding, which would fire the trigger again on
       * every bar. Anchored on one of its own values it cannot: the variance
       * is then at least 1/(2n) of the mean square it is extracted from.
       */
      if (variance < 1e-6 * (periodTotal2 * invPeriod)) {
        shift = inReal[i];
        periodTotal1 = 0.0;
        periodTotal2 = 0.0;
        for (j = windowStart; j <= i; j += 1) {
          tempReal = inReal[j] - shift;
          periodTotal1 += tempReal;
          tempReal *= tempReal;
          periodTotal2 += tempReal;
        }
        meanValue1 = periodTotal1 * invPeriod;
        variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
      }
      /* Before the re-remove below: the peak must hold the whole window. */
      peakTotal2 = periodTotal2;
      /* After the re-anchor a window with any spread sits orders above this
       * floor, so it catches only a variance that rounding left at or below
       * 0. That keeps the output non-negative, which lets STDDEV and BBANDS
       * square-root it unconditionally (#243). A negative variance always
       * gets here: the peak is never negative, so the trigger fires on it.
       */
      if (variance < 1e-12 * (periodTotal2 * invPeriod)) {
        variance = 0.0;
      }
      /* Re-remove the trailing value under the new shift so the carried state
       * matches the non-reseed path.
       */
      tempReal = inReal[windowStart] - shift;
      periodTotal1 -= tempReal;
      tempReal *= tempReal;
      periodTotal2 -= tempReal;
    }
    outReal[outIdx++] = variance;
    i += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['nbDev', Core.REAL_DEFAULT, Core.REAL_MIN, Core.REAL_MAX, 'in -3e37..3e37']] as const;

/** @internal */
export function varPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInNbDev: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('VAR', 'startIdx', startIdx);
  endIdx = requireInt32('VAR', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('VAR', 'timePeriod', optInTimePeriod);
  optInNbDev = requireReal('VAR', 'nbDev', optInNbDev);
  requireIndexRange('VAR', startIdx, endIdx);
  const guardLookback = varLookbackPositional(optInTimePeriod, optInNbDev, core);
  if (guardLookback < 0) {
    throw rejectedParams('VAR', PARAM_RANGES, optInTimePeriod, optInNbDev);
  }
  const guardStart = clampedStart('VAR', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('VAR', 'real', inReal, guardInLen);
  requireOutput('VAR', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('VAR', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = varImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('VAR', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link var_}, {@link varLookback}, {@link varDisplayShift},
 * {@link varOpen} and {@link varOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface VarOptions {
  /**
   * Window length for the variance (default 5; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  timePeriod?: number | undefined;
  /**
   * Deviation count accepted by the API but never used in the computation (default
   * 1; `Core.REAL_DEFAULT` selects the default).
   */
  nbDev?: number | undefined;
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

/** What {@link var_} returns, and {@link varOpenAndFill} with its stream. */
export interface VarResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /** Rolling population variance. `values[k]` is the value at input bar `begIdx + k`. */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link var_} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link VarOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function varLookback(options?: VarOptions): number {
  const o = optionsOf('VAR lookback', options);
  return varLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.nbDev, Core.REAL_DEFAULT), coreOf('VAR lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link var_}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link VarOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function varDisplayShift(outputIdx: number, options?: VarOptions): number {
  const o = optionsOf('VAR displayShift', options);
  return varDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.nbDev, Core.REAL_DEFAULT), outputIdx, coreOf('VAR displayShift', o.core));
}

/**
 * Rolling population variance of a real series over a given period. Measures
 * dispersion of values around their mean. Higher values indicate greater
 * dispersion; 0 means constant input.
 *
 * Formula and more info at
 * [ta-lib.org/functions/var](https://ta-lib.org/functions/var).
 *
 * **Notes**
 * - Computes population variance (divides by the period), not the sample variance
 *   (n-1) used by some definitions.
 * - The deviation-count parameter is accepted but has no effect on the result.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link varLookback} is a success with empty outputs.
 *
 * @param real - Source series.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link VarOptions}.
 * @returns `begIdx` and the values; see {@link VarResult}. Each output is a view
 *   of its buffer, the caller's or a new one.
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
 * import { var_ } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = var_(data, { timePeriod: 5, nbDev: 1 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`stddev`](https://ta-lib.org/functions/stddev)
 */
export function var_(real: ArrayLike<number>, options?: VarOptions): VarResult {
  const o = optionsOf('VAR', options);
  if (o.endIdx === undefined) {
    requireSeries('VAR', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInNbDev = orDefault(o.nbDev, Core.REAL_DEFAULT);
  const core = coreOf('VAR', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, varLookbackPositional(optInTimePeriod, optInNbDev, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = varPositional(startIdx, endIdx, real, optInTimePeriod, optInNbDev, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live VAR stream: one value per closed bar, bit-identical to {@link var_} over
 * the same series. Open it with {@link varOpen} or {@link varOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/var](https://ta-lib.org/functions/var).
 */
export class VarStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  optInNbDev: number = 0;
  /** @internal */
  shift: number = 0;
  /** @internal */
  periodTotal1: number = 0;
  /** @internal */
  periodTotal2: number = 0;
  /** @internal */
  invPeriod: number = 0;
  /** @internal */
  peakTotal2: number = 0;
  /** @internal */
  trailingIdx: number = 0;
  /** @internal */
  nbInitialElementNeeded: number = 0;
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
  static of(core: Core): VarStream {
    return new VarStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link var_} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('VAR advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: Source series.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('VAR update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('VAR update', 'real');
    }
    varStepImpl(this, real);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: Source series.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number): number {
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('VAR peek', 'real');
    }
    const sp = this;
    const inReal = real;
    let tempReal = 0;
    let meanValue1 = 0;
    let variance = 0;
    let barsSinceReseed = sp.barsSinceReseed;
    let cur_outReal = 0;
    let j = sp.j;
    let peakTotal2 = sp.peakTotal2;
    let periodTotal1 = sp.periodTotal1;
    let periodTotal2 = sp.periodTotal2;
    let shift = sp.shift;
    let trailingIdx = sp.trailingIdx;
    let windowStart = sp.windowStart;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    pkSlot0 = sp.i & sp.xMask;
    pkVal0 = inReal;
    /* Add the incoming value, measured against the shift. */
    tempReal = (((sp.i & sp.xMask) !== pkSlot0) ? sp.x_inReal[sp.i & sp.xMask] : pkVal0) - shift;
    periodTotal1 += tempReal;
    tempReal *= tempReal;
    periodTotal2 += tempReal;
    peakTotal2 = (periodTotal2 > peakTotal2) ? periodTotal2 : peakTotal2;
    meanValue1 = periodTotal1 * sp.invPeriod;
    variance = periodTotal2 * sp.invPeriod - meanValue1 * meanValue1;
    /* Remove the trailing value (prepares the next window). */
    tempReal = (((trailingIdx & sp.xMask) !== pkSlot0) ? sp.x_inReal[trailingIdx & sp.xMask] : pkVal0) - shift;
    periodTotal1 -= tempReal;
    tempReal *= tempReal;
    periodTotal2 -= tempReal;
    trailingIdx += 1;
    /* Rebuild with a fresh two-pass when the variance has shrunk below 1e-6
     * of the LARGEST mean squared deviation held since the last rebuild, or at
     * least every 32 windows. Measure against that peak, not the current sum:
     * the rounding the running sums carry scales with the peak, so once a
     * series settles back near the shift, or an outlier leaves the window,
     * the current sum holds nothing but that rounding. The collapse is seen
     * on the first bar whose sums carry it, and the rebuild recomputes that
     * bar.
     */
    barsSinceReseed -= 1;
    if (variance < 1e-6 * (peakTotal2 * sp.invPeriod) || barsSinceReseed <= 0) {
      barsSinceReseed = ((32 * sp.optInTimePeriod) | 0);
      windowStart = sp.i - sp.nbInitialElementNeeded;
      tempReal = 0.0;
      for (j = windowStart; j <= sp.i; j += 1) {
        tempReal += ((j & sp.xMask) !== pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0;
      }
      shift = tempReal * sp.invPeriod;
      periodTotal1 = 0.0;
      periodTotal2 = 0.0;
      for (j = windowStart; j <= sp.i; j += 1) {
        tempReal = (((j & sp.xMask) !== pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0) - shift;
        periodTotal1 += tempReal;
        tempReal *= tempReal;
        periodTotal2 += tempReal;
      }
      meanValue1 = periodTotal1 * sp.invPeriod;
      variance = periodTotal2 * sp.invPeriod - meanValue1 * meanValue1;
      /* A window flat to within the rounding of its own mean leaves the
       * variance at that rounding, which would fire the trigger again on
       * every bar. Anchored on one of its own values it cannot: the variance
       * is then at least 1/(2n) of the mean square it is extracted from.
       */
      if (variance < 1e-6 * (periodTotal2 * sp.invPeriod)) {
        shift = ((sp.i & sp.xMask) !== pkSlot0) ? sp.x_inReal[sp.i & sp.xMask] : pkVal0;
        periodTotal1 = 0.0;
        periodTotal2 = 0.0;
        for (j = windowStart; j <= sp.i; j += 1) {
          tempReal = (((j & sp.xMask) !== pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0) - shift;
          periodTotal1 += tempReal;
          tempReal *= tempReal;
          periodTotal2 += tempReal;
        }
        meanValue1 = periodTotal1 * sp.invPeriod;
        variance = periodTotal2 * sp.invPeriod - meanValue1 * meanValue1;
      }
      /* Before the re-remove below: the peak must hold the whole window. */
      peakTotal2 = periodTotal2;
      /* After the re-anchor a window with any spread sits orders above this
       * floor, so it catches only a variance that rounding left at or below
       * 0. That keeps the output non-negative, which lets STDDEV and BBANDS
       * square-root it unconditionally (#243). A negative variance always
       * gets here: the peak is never negative, so the trigger fires on it.
       */
      if (variance < 1e-12 * (periodTotal2 * sp.invPeriod)) {
        variance = 0.0;
      }
      /* Re-remove the trailing value under the new shift so the carried state
       * matches the non-reseed path.
       */
      tempReal = (((windowStart & sp.xMask) !== pkSlot0) ? sp.x_inReal[windowStart & sp.xMask] : pkVal0) - shift;
      periodTotal1 -= tempReal;
      tempReal *= tempReal;
      periodTotal2 -= tempReal;
    }
    cur_outReal = variance;
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
  clone(): VarStream {
    const c = new VarStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.optInNbDev = this.optInNbDev;
    c.shift = this.shift;
    c.periodTotal1 = this.periodTotal1;
    c.periodTotal2 = this.periodTotal2;
    c.invPeriod = this.invPeriod;
    c.peakTotal2 = this.peakTotal2;
    c.trailingIdx = this.trailingIdx;
    c.nbInitialElementNeeded = this.nbInitialElementNeeded;
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

function varStepImpl(sp: VarStream, inReal: number): void {
  let tempReal = 0;
  let meanValue1 = 0;
  let variance = 0;
  sp.x_inReal[sp.i & sp.xMask] = inReal;
  /* Add the incoming value, measured against the shift. */
  tempReal = sp.x_inReal[sp.i & sp.xMask] - sp.shift;
  sp.periodTotal1 += tempReal;
  tempReal *= tempReal;
  sp.periodTotal2 += tempReal;
  sp.peakTotal2 = (sp.periodTotal2 > sp.peakTotal2) ? sp.periodTotal2 : sp.peakTotal2;
  meanValue1 = sp.periodTotal1 * sp.invPeriod;
  variance = sp.periodTotal2 * sp.invPeriod - meanValue1 * meanValue1;
  /* Remove the trailing value (prepares the next window). */
  tempReal = sp.x_inReal[sp.trailingIdx & sp.xMask] - sp.shift;
  sp.periodTotal1 -= tempReal;
  tempReal *= tempReal;
  sp.periodTotal2 -= tempReal;
  sp.trailingIdx += 1;
  /* Rebuild with a fresh two-pass when the variance has shrunk below 1e-6
   * of the LARGEST mean squared deviation held since the last rebuild, or at
   * least every 32 windows. Measure against that peak, not the current sum:
   * the rounding the running sums carry scales with the peak, so once a
   * series settles back near the shift, or an outlier leaves the window,
   * the current sum holds nothing but that rounding. The collapse is seen
   * on the first bar whose sums carry it, and the rebuild recomputes that
   * bar.
   */
  sp.barsSinceReseed -= 1;
  if (variance < 1e-6 * (sp.peakTotal2 * sp.invPeriod) || sp.barsSinceReseed <= 0) {
    sp.barsSinceReseed = ((32 * sp.optInTimePeriod) | 0);
    sp.windowStart = sp.i - sp.nbInitialElementNeeded;
    tempReal = 0.0;
    for (sp.j = sp.windowStart; sp.j <= sp.i; sp.j += 1) {
      tempReal += sp.x_inReal[sp.j & sp.xMask];
    }
    sp.shift = tempReal * sp.invPeriod;
    sp.periodTotal1 = 0.0;
    sp.periodTotal2 = 0.0;
    for (sp.j = sp.windowStart; sp.j <= sp.i; sp.j += 1) {
      tempReal = sp.x_inReal[sp.j & sp.xMask] - sp.shift;
      sp.periodTotal1 += tempReal;
      tempReal *= tempReal;
      sp.periodTotal2 += tempReal;
    }
    meanValue1 = sp.periodTotal1 * sp.invPeriod;
    variance = sp.periodTotal2 * sp.invPeriod - meanValue1 * meanValue1;
    /* A window flat to within the rounding of its own mean leaves the
     * variance at that rounding, which would fire the trigger again on
     * every bar. Anchored on one of its own values it cannot: the variance
     * is then at least 1/(2n) of the mean square it is extracted from.
     */
    if (variance < 1e-6 * (sp.periodTotal2 * sp.invPeriod)) {
      sp.shift = sp.x_inReal[sp.i & sp.xMask];
      sp.periodTotal1 = 0.0;
      sp.periodTotal2 = 0.0;
      for (sp.j = sp.windowStart; sp.j <= sp.i; sp.j += 1) {
        tempReal = sp.x_inReal[sp.j & sp.xMask] - sp.shift;
        sp.periodTotal1 += tempReal;
        tempReal *= tempReal;
        sp.periodTotal2 += tempReal;
      }
      meanValue1 = sp.periodTotal1 * sp.invPeriod;
      variance = sp.periodTotal2 * sp.invPeriod - meanValue1 * meanValue1;
    }
    /* Before the re-remove below: the peak must hold the whole window. */
    sp.peakTotal2 = sp.periodTotal2;
    /* After the re-anchor a window with any spread sits orders above this
     * floor, so it catches only a variance that rounding left at or below
     * 0. That keeps the output non-negative, which lets STDDEV and BBANDS
     * square-root it unconditionally (#243). A negative variance always
     * gets here: the peak is never negative, so the trigger fires on it.
     */
    if (variance < 1e-12 * (sp.periodTotal2 * sp.invPeriod)) {
      variance = 0.0;
    }
    /* Re-remove the trailing value under the new shift so the carried state
     * matches the non-reseed path.
     */
    tempReal = sp.x_inReal[sp.windowStart & sp.xMask] - sp.shift;
    sp.periodTotal1 -= tempReal;
    tempReal *= tempReal;
    sp.periodTotal2 -= tempReal;
  }
  sp.cur_outReal = variance;
  sp.i += 1;
}

function varOpenImpl(
  sp: VarStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInNbDev: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let tempReal = 0;
  let shift = 0;
  let periodTotal1 = 0;
  let periodTotal2 = 0;
  let meanValue1 = 0;
  let variance = 0;
  let invPeriod = 0;
  let peakTotal2 = 0;
  let i = 0;
  let j = 0;
  let outIdx = 0;
  let trailingIdx = 0;
  let windowStart = 0;
  let nbInitialElementNeeded = 0;
  let barsSinceReseed = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 5;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInNbDev === Core.REAL_DEFAULT) {
    optInNbDev = 1.0;
  } else if (!(optInNbDev >= Core.REAL_MIN && optInNbDev <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Identify the minimum number of price bar needed to calculate
   * at least one output.
   */
  nbInitialElementNeeded = optInTimePeriod - 1;
  /* Move up the start index if there is not enough initial data. */
  if (startIdx < nbInitialElementNeeded) {
    startIdx = nbInitialElementNeeded;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  invPeriod = 1.0 / optInTimePeriod;
  /* Measure deviations against a shift near the window: the running sums
   * periodTotal1 = sum(inReal-shift) and periodTotal2 = sum((inReal-shift)^2)
   * stay at variance scale, so variance = periodTotal2/period - mean^2 no longer
   * subtracts two ~mean^2 quantities. Anchor the shift to the first window value
   * (also gives an exact 0 for period 1, with no division by period-1).
   */
  trailingIdx = startIdx - nbInitialElementNeeded;
  shift = inReal[trailingIdx];
  periodTotal1 = 0.0;
  periodTotal2 = 0.0;
  for (j = trailingIdx; j < startIdx; j += 1) {
    tempReal = inReal[j] - shift;
    periodTotal1 += tempReal;
    tempReal *= tempReal;
    periodTotal2 += tempReal;
  }
  /* inReal and outReal may be the same buffer: each trailing value is consumed
   * before its slot is overwritten by the output.
   */
  i = startIdx;
  outIdx = 0;
  barsSinceReseed = ((32 * optInTimePeriod) | 0);
  peakTotal2 = periodTotal2;
  do {
    /* Add the incoming value, measured against the shift. */
    tempReal = inReal[i] - shift;
    periodTotal1 += tempReal;
    tempReal *= tempReal;
    periodTotal2 += tempReal;
    peakTotal2 = (periodTotal2 > peakTotal2) ? periodTotal2 : peakTotal2;
    meanValue1 = periodTotal1 * invPeriod;
    variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
    /* Remove the trailing value (prepares the next window). */
    tempReal = inReal[trailingIdx] - shift;
    periodTotal1 -= tempReal;
    tempReal *= tempReal;
    periodTotal2 -= tempReal;
    trailingIdx += 1;
    /* Rebuild with a fresh two-pass when the variance has shrunk below 1e-6
     * of the LARGEST mean squared deviation held since the last rebuild, or at
     * least every 32 windows. Measure against that peak, not the current sum:
     * the rounding the running sums carry scales with the peak, so once a
     * series settles back near the shift, or an outlier leaves the window,
     * the current sum holds nothing but that rounding. The collapse is seen
     * on the first bar whose sums carry it, and the rebuild recomputes that
     * bar.
     */
    barsSinceReseed -= 1;
    if (variance < 1e-6 * (peakTotal2 * invPeriod) || barsSinceReseed <= 0) {
      barsSinceReseed = ((32 * optInTimePeriod) | 0);
      windowStart = i - nbInitialElementNeeded;
      tempReal = 0.0;
      for (j = windowStart; j <= i; j += 1) {
        tempReal += inReal[j];
      }
      shift = tempReal * invPeriod;
      periodTotal1 = 0.0;
      periodTotal2 = 0.0;
      for (j = windowStart; j <= i; j += 1) {
        tempReal = inReal[j] - shift;
        periodTotal1 += tempReal;
        tempReal *= tempReal;
        periodTotal2 += tempReal;
      }
      meanValue1 = periodTotal1 * invPeriod;
      variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
      /* A window flat to within the rounding of its own mean leaves the
       * variance at that rounding, which would fire the trigger again on
       * every bar. Anchored on one of its own values it cannot: the variance
       * is then at least 1/(2n) of the mean square it is extracted from.
       */
      if (variance < 1e-6 * (periodTotal2 * invPeriod)) {
        shift = inReal[i];
        periodTotal1 = 0.0;
        periodTotal2 = 0.0;
        for (j = windowStart; j <= i; j += 1) {
          tempReal = inReal[j] - shift;
          periodTotal1 += tempReal;
          tempReal *= tempReal;
          periodTotal2 += tempReal;
        }
        meanValue1 = periodTotal1 * invPeriod;
        variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
      }
      /* Before the re-remove below: the peak must hold the whole window. */
      peakTotal2 = periodTotal2;
      /* After the re-anchor a window with any spread sits orders above this
       * floor, so it catches only a variance that rounding left at or below
       * 0. That keeps the output non-negative, which lets STDDEV and BBANDS
       * square-root it unconditionally (#243). A negative variance always
       * gets here: the peak is never negative, so the trigger fires on it.
       */
      if (variance < 1e-12 * (periodTotal2 * invPeriod)) {
        variance = 0.0;
      }
      /* Re-remove the trailing value under the new shift so the carried state
       * matches the non-reseed path.
       */
      tempReal = inReal[windowStart] - shift;
      periodTotal1 -= tempReal;
      tempReal *= tempReal;
      periodTotal2 -= tempReal;
    }
    outReal[((outIdx++ * outStride) | 0)] = variance;
    i += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
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
  sp.optInNbDev = optInNbDev;
  sp.shift = shift;
  sp.periodTotal1 = periodTotal1;
  sp.periodTotal2 = periodTotal2;
  sp.invPeriod = invPeriod;
  sp.peakTotal2 = peakTotal2;
  sp.trailingIdx = trailingIdx;
  sp.nbInitialElementNeeded = nbInitialElementNeeded;
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
export function varOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInNbDev: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): VarStream {
  const sp = VarStream.of(core);
  const retCode = varOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('VAR openAndFill', inReal.length, startIdx, varLookbackPositional(optInTimePeriod, optInNbDev, core));
  }
  if (retCode === RetCode.BAD_PARAM && varLookbackPositional(optInTimePeriod, optInNbDev, core) < 0) {
    throw rejectedParams('VAR openAndFill', PARAM_RANGES, optInTimePeriod, optInNbDev);
  }
  throw streamFailure('VAR openAndFill', retCode);
}

/** @internal */
export function varOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInNbDev: number,
): VarStream {
  const sp = VarStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = varOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('VAR open', inReal.length, startIdx, varLookbackPositional(optInTimePeriod, optInNbDev, core));
  }
  if (retCode === RetCode.BAD_PARAM && varLookbackPositional(optInTimePeriod, optInNbDev, core) < 0) {
    throw rejectedParams('VAR open', PARAM_RANGES, optInTimePeriod, optInNbDev);
  }
  throw streamFailure('VAR open', retCode);
}

/** @internal */
export function varOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInNbDev: number,
  core: Core = Core.DEFAULT,
): VarStream {
  optInTimePeriod = requireInt32('VAR open', 'timePeriod', optInTimePeriod);
  optInNbDev = requireReal('VAR open', 'nbDev', optInNbDev);
  requireArgument('VAR open', 'real', inReal);
  requireHistory('VAR open', 'real', inReal.length);
  return varOpenInternal(core, float64(inReal), 0, optInTimePeriod, optInNbDev);
}

/** @internal */
export function varOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInNbDev: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): VarStream {
  optInTimePeriod = requireInt32('VAR openAndFill', 'timePeriod', optInTimePeriod);
  optInNbDev = requireReal('VAR openAndFill', 'nbDev', optInNbDev);
  requireArgument('VAR openAndFill', 'real', inReal);
  requireHistory('VAR openAndFill', 'real', inReal.length);
  const guardLookback = varLookbackPositional(optInTimePeriod, optInNbDev, core);
  if (guardLookback < 0) {
    throw rejectedParams('VAR openAndFill', PARAM_RANGES, optInTimePeriod, optInNbDev);
  }
  const guardOutLen = openFillCount('VAR openAndFill', inReal.length, guardLookback);
  requireOutput('VAR openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('VAR openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('VAR openAndFill', 'outReal', 'real');
  }
  return varOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, optInNbDev, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live VAR stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link var_} there.
 *
 * @param real - Warm-up history. Source series.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link VarOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `varLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a real parameter that is not a number
 *   or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { varOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = varOpen(data, { timePeriod: 5, nbDev: 1 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function varOpen(real: ArrayLike<number>, options?: VarOptions): VarStream {
  const o = optionsOf('VAR open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInNbDev = orDefault(o.nbDev, Core.REAL_DEFAULT);
  const core = coreOf('VAR open', o.core);
  return varOpenPositional(real, optInTimePeriod, optInNbDev, core);
}

/**
 * {@link varOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link var_}.
 *
 * @param real - Warm-up history. Source series.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link VarOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link var_} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `varLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a real parameter that is not a
 *   number, a parameter outside its range or a buffer too short, of the wrong type
 *   or starting where another buffer or a `Float64Array` input starts, or, on a
 *   history long enough to open, one sharing memory with another buffer or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { varOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = varOpenAndFill(data, {
 *   timePeriod: 5,
 *   nbDev: 1,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function varOpenAndFill(real: ArrayLike<number>, options?: VarOptions): VarResult & { stream: VarStream } {
  const o = optionsOf('VAR openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInNbDev = orDefault(o.nbDev, Core.REAL_DEFAULT);
  const core = coreOf('VAR openAndFill', o.core);
  const capacity = fillCapacity(varLookbackPositional(optInTimePeriod, optInNbDev, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = varOpenAndFillPositional(real, optInTimePeriod, optInNbDev, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
