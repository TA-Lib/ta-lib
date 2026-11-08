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
 *  110726 MF,CC  Initial version (#572).
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireReal, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function sortinoLookbackPositional(optInTimePeriod: number, optInMAR: number, optInAnnualization: number, _core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (typeof optInMAR !== 'number') {
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
  if (optInMAR === Core.REAL_DEFAULT) {
    optInMAR = 0.0;
  } else if (!(optInMAR >= Core.REAL_MIN && optInMAR <= Core.REAL_MAX)) {
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
export function sortinoDisplayShiftPositional(optInTimePeriod: number, optInMAR: number, optInAnnualization: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (sortinoLookbackPositional(optInTimePeriod, optInMAR, optInAnnualization, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function sortinoImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  optInMAR: number,
  optInAnnualization: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let periodTotal = 0;
  let downTotal = 0;
  let absTotal = 0;
  let peakDown = 0;
  let peakAbs = 0;
  let windowTotal = 0;
  let windowDown = 0;
  let windowAbs = 0;
  let price = 0;
  let prevPrice = 0;
  let trailingPrevPrice = 0;
  let windowPrevPrice = 0;
  let ret = 0;
  let dev = 0;
  let invPeriod = 0;
  let sqrtAnnual = 0;
  let excess = 0;
  let ratio = 0;
  let i = 0;
  let j = 0;
  let outIdx = 0;
  let trailingIdx = 0;
  let windowStart = 0;
  let barsSinceReseed = 0;
  let nbBelow = 0;
  let nbAbove = 0;
  let windowBelow = 0;
  let windowAbove = 0;
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
  if (optInMAR === Core.REAL_DEFAULT) {
    optInMAR = 0.0;
  } else if (!(optInMAR >= Core.REAL_MIN && optInMAR <= Core.REAL_MAX)) {
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
  invPeriod = 1.0 / optInTimePeriod;
  sqrtAnnual = Math.sqrt(optInAnnualization);
  /* The window holds the returns of the bars after trailingIdx up to i; a
   * bar's return reads the price before it. A return needs a non-zero base
   * price and nothing more: a zero base gives a zero return.
   *
   * The counts of returns below and above the MAR are exact, so they, not
   * the running sums, decide when the window has no downside.
   */
  trailingIdx = startIdx - optInTimePeriod;
  prevPrice = inReal[trailingIdx];
  periodTotal = 0.0;
  downTotal = 0.0;
  absTotal = 0.0;
  nbBelow = 0;
  nbAbove = 0;
  for (j = trailingIdx + 1; j < startIdx; j += 1) {
    price = inReal[j];
    ret = 0.0;
    if (prevPrice !== 0.0) {
      ret = (price - prevPrice) / prevPrice;
    }
    prevPrice = price;
    periodTotal += ret;
    absTotal += Math.abs(ret);
    dev = ret - optInMAR;
    if (dev < 0.0) {
      downTotal += dev * dev;
      nbBelow += 1;
    } else {
      nbAbove += +(dev > 0.0);
    }
  }
  i = startIdx;
  outIdx = 0;
  barsSinceReseed = ((32 * optInTimePeriod) | 0);
  peakDown = downTotal;
  peakAbs = absTotal;
  do {
    price = inReal[i];
    ret = 0.0;
    if (prevPrice !== 0.0) {
      ret = (price - prevPrice) / prevPrice;
    }
    prevPrice = price;
    periodTotal += ret;
    absTotal += Math.abs(ret);
    dev = ret - optInMAR;
    if (dev < 0.0) {
      downTotal += dev * dev;
      nbBelow += 1;
    } else {
      nbAbove += +(dev > 0.0);
    }
    peakDown = (downTotal > peakDown) ? downTotal : peakDown;
    peakAbs = (absTotal > peakAbs) ? absTotal : peakAbs;
    windowTotal = periodTotal;
    windowDown = downTotal;
    windowAbs = absTotal;
    windowBelow = nbBelow;
    windowAbove = nbAbove;
    /* inReal and outReal may be the same buffer, and the trailing prices
     * sit in slots the output reaches: every read of the window happens
     * before this bar's write.
     */
    trailingPrevPrice = inReal[trailingIdx];
    trailingIdx += 1;
    price = inReal[trailingIdx];
    ret = 0.0;
    if (trailingPrevPrice !== 0.0) {
      ret = (price - trailingPrevPrice) / trailingPrevPrice;
    }
    periodTotal -= ret;
    absTotal -= Math.abs(ret);
    dev = ret - optInMAR;
    if (dev < 0.0) {
      downTotal -= dev * dev;
      nbBelow -= 1;
    } else if (dev > 0.0) {
      nbAbove -= 1;
    }
    /* Rebuild when the downside sum, or the sum of absolute returns, has
     * shrunk below 1% of the largest one held since the last rebuild, or at
     * least every 32 windows. The rounding a running sum carries scales
     * with that peak, so once a large return leaves, the mean or the
     * divisor can be all rounding.
     */
    barsSinceReseed -= 1;
    if (windowDown < 0.01 * peakDown || windowAbs < 0.01 * peakAbs || barsSinceReseed <= 0) {
      barsSinceReseed = ((32 * optInTimePeriod) | 0);
      windowStart = i - optInTimePeriod + 1;
      windowPrevPrice = inReal[windowStart - 1];
      windowTotal = 0.0;
      windowDown = 0.0;
      windowAbs = 0.0;
      for (j = windowStart; j <= i; j += 1) {
        price = inReal[j];
        ret = 0.0;
        if (windowPrevPrice !== 0.0) {
          ret = (price - windowPrevPrice) / windowPrevPrice;
        }
        windowPrevPrice = price;
        windowTotal += ret;
        windowAbs += Math.abs(ret);
        dev = ret - optInMAR;
        if (dev < 0.0) {
          windowDown += dev * dev;
        }
      }
      peakDown = windowDown;
      peakAbs = windowAbs;
      /* Re-remove the trailing return so the carried state matches the
       * path without a rebuild; the counts never drift.
       */
      price = inReal[windowStart];
      ret = 0.0;
      if (inReal[windowStart - 1] !== 0.0) {
        ret = (price - inReal[windowStart - 1]) / inReal[windowStart - 1];
      }
      periodTotal = windowTotal - ret;
      absTotal = windowAbs - Math.abs(ret);
      downTotal = windowDown;
      dev = ret - optInMAR;
      if (dev < 0.0) {
        downTotal -= dev * dev;
      }
    }
    /* No downside is the plain quotient (ruled on #572): the mean excess
     * is then positive exactly when some return beats the MAR, giving
     * +Inf, and zero when every return equals it, giving NaN. The count
     * stands in for the mean so rounding cannot flip either case.
     * Annualize the per-bar ratio last, so an annualization of 1 is that
     * ratio bit for bit.
     */
    excess = windowTotal * invPeriod - optInMAR;
    if (windowBelow === 0) {
      windowDown = 0.0;
      excess = windowAbove;
    }
    ratio = excess / Math.sqrt(windowDown * invPeriod);
    outReal[outIdx++] = sqrtAnnual * ratio;
    i += 1;
  } while (i <= endIdx);
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['mar', Core.REAL_DEFAULT, Core.REAL_MIN, Core.REAL_MAX, 'in -3e37..3e37'], ['annualization', Core.REAL_DEFAULT, 0e0, Core.REAL_MAX, 'in 0..3e37']] as const;

/** @internal */
export function sortinoPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInMAR: number,
  optInAnnualization: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('SORTINO', 'startIdx', startIdx);
  endIdx = requireInt32('SORTINO', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('SORTINO', 'timePeriod', optInTimePeriod);
  optInMAR = requireReal('SORTINO', 'mar', optInMAR);
  optInAnnualization = requireReal('SORTINO', 'annualization', optInAnnualization);
  requireIndexRange('SORTINO', startIdx, endIdx);
  const guardLookback = sortinoLookbackPositional(optInTimePeriod, optInMAR, optInAnnualization, core);
  if (guardLookback < 0) {
    throw rejectedParams('SORTINO', PARAM_RANGES, optInTimePeriod, optInMAR, optInAnnualization);
  }
  const guardStart = clampedStart('SORTINO', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('SORTINO', 'real', inReal, guardInLen);
  requireOutput('SORTINO', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('SORTINO', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = sortinoImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, optInMAR, optInAnnualization, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('SORTINO', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link sortino}, {@link sortinoLookback},
 * {@link sortinoDisplayShift}, {@link sortinoOpen} and {@link sortinoOpenAndFill}.
 * Every key is optional: an omitted or `undefined` key takes its default, and a
 * call ignores the keys it has no use for.
 */
export interface SortinoOptions {
  /**
   * Number of returns in the window; one price more is read (default 20; range 2 to
   * 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  timePeriod?: number | undefined;
  /**
   * Minimum acceptable return per bar; returns below it are the downside (default 0;
   * `Core.REAL_DEFAULT` selects the default).
   */
  mar?: number | undefined;
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

/** What {@link sortino} returns, and {@link sortinoOpenAndFill} with its stream. */
export interface SortinoResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Annualized Sortino ratio of the trailing window, or +Inf or NaN where the window
   * has no return below the MAR. `values[k]` is the value at input bar `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link sortino} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SortinoOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function sortinoLookback(options?: SortinoOptions): number {
  const o = optionsOf('SORTINO lookback', options);
  return sortinoLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.mar, Core.REAL_DEFAULT), orDefault(o.annualization, Core.REAL_DEFAULT), coreOf('SORTINO lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link sortino}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SortinoOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function sortinoDisplayShift(outputIdx: number, options?: SortinoOptions): number {
  const o = optionsOf('SORTINO displayShift', options);
  return sortinoDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.mar, Core.REAL_DEFAULT), orDefault(o.annualization, Core.REAL_DEFAULT), outputIdx, coreOf('SORTINO displayShift', o.core));
}

/**
 * The rolling Sortino ratio: the mean return in excess of a minimum acceptable
 * return (MAR), divided by the downside deviation, over a trailing window, scaled
 * to a year. It is the Sharpe ratio with upside volatility taken out of the
 * denominator: only returns below the MAR count as risk. Above 0 the window's
 * returns beat the MAR on average; the larger the value, the more return per unit
 * of downside.
 *
 * Formula and more info at
 * [ta-lib.org/functions/sortino](https://ta-lib.org/functions/sortino).
 *
 * **Notes**
 * - The downside deviation divides by the full window `n`: a return above the MAR
 *   counts in `n` and adds zero to the sum. This is the definition of Sortino and
 *   Price (1994) and the divisor of Bacon's worked example; dividing by `n−1`, or
 *   by the number of returns below the MAR, gives a different value.
 * - The mean is arithmetic, not compounded.
 * - A window with no return below the MAR has no downside: the result is +Inf, and
 *   NaN when every return equals the MAR. In a rising market this is common. With
 *   an annualization of 0 it is NaN there and 0 elsewhere.
 * - The MAR is per bar, in the same unit as the returns. For an annual rate `R`,
 *   pass `(1+R)^(1/A) − 1`, where `A` is the annualization; it is not
 *   de-annualized for you.
 * - An annualization of 1 gives the raw per-bar ratio.
 * - A zero price leaves no return to measure from it: the return of the bar after
 *   it counts as 0.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link sortinoLookback} is a success with empty outputs.
 *
 * @param real - The price series; its returns are computed internally.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link SortinoOptions}.
 * @returns `begIdx` and the values; see {@link SortinoResult}. Each output is a
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
 * import { sortino } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = sortino(data, {
 *   timePeriod: 20,
 *   mar: 0,
 *   annualization: 252,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`sharpe`](https://ta-lib.org/functions/sharpe)
 * @see [`stddev`](https://ta-lib.org/functions/stddev)
 * @see [`var_`](https://ta-lib.org/functions/var)
 * @see [`rocp`](https://ta-lib.org/functions/rocp)
 */
export function sortino(real: ArrayLike<number>, options?: SortinoOptions): SortinoResult {
  const o = optionsOf('SORTINO', options);
  if (o.endIdx === undefined) {
    requireSeries('SORTINO', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInMAR = orDefault(o.mar, Core.REAL_DEFAULT);
  const optInAnnualization = orDefault(o.annualization, Core.REAL_DEFAULT);
  const core = coreOf('SORTINO', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, sortinoLookbackPositional(optInTimePeriod, optInMAR, optInAnnualization, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = sortinoPositional(startIdx, endIdx, real, optInTimePeriod, optInMAR, optInAnnualization, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live SORTINO stream: one value per closed bar, bit-identical to
 * {@link sortino} over the same series. Open it with {@link sortinoOpen} or
 * {@link sortinoOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/sortino](https://ta-lib.org/functions/sortino).
 */
export class SortinoStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  optInMAR: number = 0;
  /** @internal */
  optInAnnualization: number = 0;
  /** @internal */
  periodTotal: number = 0;
  /** @internal */
  downTotal: number = 0;
  /** @internal */
  absTotal: number = 0;
  /** @internal */
  peakDown: number = 0;
  /** @internal */
  peakAbs: number = 0;
  /** @internal */
  prevPrice: number = 0;
  /** @internal */
  invPeriod: number = 0;
  /** @internal */
  sqrtAnnual: number = 0;
  /** @internal */
  trailingIdx: number = 0;
  /** @internal */
  barsSinceReseed: number = 0;
  /** @internal */
  nbBelow: number = 0;
  /** @internal */
  nbAbove: number = 0;
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
  static of(core: Core): SortinoStream {
    return new SortinoStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link sortino} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('SORTINO advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw streamFailure('SORTINO update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('SORTINO update', 'real');
    }
    sortinoStepImpl(this, real);
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
      throw nonFiniteBar('SORTINO peek', 'real');
    }
    const sp = this;
    const inReal = real;
    let windowTotal = 0;
    let windowDown = 0;
    let windowAbs = 0;
    let price = 0;
    let trailingPrevPrice = 0;
    let windowPrevPrice = 0;
    let ret = 0;
    let dev = 0;
    let excess = 0;
    let ratio = 0;
    let windowBelow = 0;
    let windowAbove = 0;
    let absTotal = sp.absTotal;
    let barsSinceReseed = sp.barsSinceReseed;
    let cur_outReal = 0;
    let downTotal = sp.downTotal;
    let j = sp.j;
    let nbAbove = sp.nbAbove;
    let nbBelow = sp.nbBelow;
    let peakAbs = sp.peakAbs;
    let peakDown = sp.peakDown;
    let periodTotal = sp.periodTotal;
    let prevPrice = sp.prevPrice;
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
    periodTotal += ret;
    absTotal += Math.abs(ret);
    dev = ret - sp.optInMAR;
    if (dev < 0.0) {
      downTotal += dev * dev;
      nbBelow += 1;
    } else {
      nbAbove += +(dev > 0.0);
    }
    peakDown = (downTotal > peakDown) ? downTotal : peakDown;
    peakAbs = (absTotal > peakAbs) ? absTotal : peakAbs;
    windowTotal = periodTotal;
    windowDown = downTotal;
    windowAbs = absTotal;
    windowBelow = nbBelow;
    windowAbove = nbAbove;
    /* inReal and outReal may be the same buffer, and the trailing prices
     * sit in slots the output reaches: every read of the window happens
     * before this bar's write.
     */
    trailingPrevPrice = ((trailingIdx & sp.xMask) !== pkSlot0) ? sp.x_inReal[trailingIdx & sp.xMask] : pkVal0;
    trailingIdx += 1;
    price = ((trailingIdx & sp.xMask) !== pkSlot0) ? sp.x_inReal[trailingIdx & sp.xMask] : pkVal0;
    ret = 0.0;
    if (trailingPrevPrice !== 0.0) {
      ret = (price - trailingPrevPrice) / trailingPrevPrice;
    }
    periodTotal -= ret;
    absTotal -= Math.abs(ret);
    dev = ret - sp.optInMAR;
    if (dev < 0.0) {
      downTotal -= dev * dev;
      nbBelow -= 1;
    } else if (dev > 0.0) {
      nbAbove -= 1;
    }
    /* Rebuild when the downside sum, or the sum of absolute returns, has
     * shrunk below 1% of the largest one held since the last rebuild, or at
     * least every 32 windows. The rounding a running sum carries scales
     * with that peak, so once a large return leaves, the mean or the
     * divisor can be all rounding.
     */
    barsSinceReseed -= 1;
    if (windowDown < 0.01 * peakDown || windowAbs < 0.01 * peakAbs || barsSinceReseed <= 0) {
      barsSinceReseed = ((32 * sp.optInTimePeriod) | 0);
      windowStart = sp.i - sp.optInTimePeriod + 1;
      windowPrevPrice = (((windowStart - 1) & sp.xMask) !== pkSlot0) ? sp.x_inReal[(windowStart - 1) & sp.xMask] : pkVal0;
      windowTotal = 0.0;
      windowDown = 0.0;
      windowAbs = 0.0;
      for (j = windowStart; j <= sp.i; j += 1) {
        price = ((j & sp.xMask) !== pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0;
        ret = 0.0;
        if (windowPrevPrice !== 0.0) {
          ret = (price - windowPrevPrice) / windowPrevPrice;
        }
        windowPrevPrice = price;
        windowTotal += ret;
        windowAbs += Math.abs(ret);
        dev = ret - sp.optInMAR;
        if (dev < 0.0) {
          windowDown += dev * dev;
        }
      }
      peakDown = windowDown;
      peakAbs = windowAbs;
      /* Re-remove the trailing return so the carried state matches the
       * path without a rebuild; the counts never drift.
       */
      price = ((windowStart & sp.xMask) !== pkSlot0) ? sp.x_inReal[windowStart & sp.xMask] : pkVal0;
      ret = 0.0;
      if (((((windowStart - 1) & sp.xMask) !== pkSlot0) ? sp.x_inReal[(windowStart - 1) & sp.xMask] : pkVal0) !== 0.0) {
        ret = (price - ((((windowStart - 1) & sp.xMask) !== pkSlot0) ? sp.x_inReal[(windowStart - 1) & sp.xMask] : pkVal0)) / ((((windowStart - 1) & sp.xMask) !== pkSlot0) ? sp.x_inReal[(windowStart - 1) & sp.xMask] : pkVal0);
      }
      periodTotal = windowTotal - ret;
      absTotal = windowAbs - Math.abs(ret);
      downTotal = windowDown;
      dev = ret - sp.optInMAR;
      if (dev < 0.0) {
        downTotal -= dev * dev;
      }
    }
    /* No downside is the plain quotient (ruled on #572): the mean excess
     * is then positive exactly when some return beats the MAR, giving
     * +Inf, and zero when every return equals it, giving NaN. The count
     * stands in for the mean so rounding cannot flip either case.
     * Annualize the per-bar ratio last, so an annualization of 1 is that
     * ratio bit for bit.
     */
    excess = windowTotal * sp.invPeriod - sp.optInMAR;
    if (windowBelow === 0) {
      windowDown = 0.0;
      excess = windowAbove;
    }
    ratio = excess / Math.sqrt(windowDown * sp.invPeriod);
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
  clone(): SortinoStream {
    const c = new SortinoStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.optInMAR = this.optInMAR;
    c.optInAnnualization = this.optInAnnualization;
    c.periodTotal = this.periodTotal;
    c.downTotal = this.downTotal;
    c.absTotal = this.absTotal;
    c.peakDown = this.peakDown;
    c.peakAbs = this.peakAbs;
    c.prevPrice = this.prevPrice;
    c.invPeriod = this.invPeriod;
    c.sqrtAnnual = this.sqrtAnnual;
    c.trailingIdx = this.trailingIdx;
    c.barsSinceReseed = this.barsSinceReseed;
    c.nbBelow = this.nbBelow;
    c.nbAbove = this.nbAbove;
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

function sortinoStepImpl(sp: SortinoStream, inReal: number): void {
  let windowTotal = 0;
  let windowDown = 0;
  let windowAbs = 0;
  let price = 0;
  let trailingPrevPrice = 0;
  let windowPrevPrice = 0;
  let ret = 0;
  let dev = 0;
  let excess = 0;
  let ratio = 0;
  let windowBelow = 0;
  let windowAbove = 0;
  sp.x_inReal[sp.i & sp.xMask] = inReal;
  price = sp.x_inReal[sp.i & sp.xMask];
  ret = 0.0;
  if (sp.prevPrice !== 0.0) {
    ret = (price - sp.prevPrice) / sp.prevPrice;
  }
  sp.prevPrice = price;
  sp.periodTotal += ret;
  sp.absTotal += Math.abs(ret);
  dev = ret - sp.optInMAR;
  if (dev < 0.0) {
    sp.downTotal += dev * dev;
    sp.nbBelow += 1;
  } else {
    sp.nbAbove += +(dev > 0.0);
  }
  sp.peakDown = (sp.downTotal > sp.peakDown) ? sp.downTotal : sp.peakDown;
  sp.peakAbs = (sp.absTotal > sp.peakAbs) ? sp.absTotal : sp.peakAbs;
  windowTotal = sp.periodTotal;
  windowDown = sp.downTotal;
  windowAbs = sp.absTotal;
  windowBelow = sp.nbBelow;
  windowAbove = sp.nbAbove;
  /* inReal and outReal may be the same buffer, and the trailing prices
   * sit in slots the output reaches: every read of the window happens
   * before this bar's write.
   */
  trailingPrevPrice = sp.x_inReal[sp.trailingIdx & sp.xMask];
  sp.trailingIdx += 1;
  price = sp.x_inReal[sp.trailingIdx & sp.xMask];
  ret = 0.0;
  if (trailingPrevPrice !== 0.0) {
    ret = (price - trailingPrevPrice) / trailingPrevPrice;
  }
  sp.periodTotal -= ret;
  sp.absTotal -= Math.abs(ret);
  dev = ret - sp.optInMAR;
  if (dev < 0.0) {
    sp.downTotal -= dev * dev;
    sp.nbBelow -= 1;
  } else if (dev > 0.0) {
    sp.nbAbove -= 1;
  }
  /* Rebuild when the downside sum, or the sum of absolute returns, has
   * shrunk below 1% of the largest one held since the last rebuild, or at
   * least every 32 windows. The rounding a running sum carries scales
   * with that peak, so once a large return leaves, the mean or the
   * divisor can be all rounding.
   */
  sp.barsSinceReseed -= 1;
  if (windowDown < 0.01 * sp.peakDown || windowAbs < 0.01 * sp.peakAbs || sp.barsSinceReseed <= 0) {
    sp.barsSinceReseed = ((32 * sp.optInTimePeriod) | 0);
    sp.windowStart = sp.i - sp.optInTimePeriod + 1;
    windowPrevPrice = sp.x_inReal[(sp.windowStart - 1) & sp.xMask];
    windowTotal = 0.0;
    windowDown = 0.0;
    windowAbs = 0.0;
    for (sp.j = sp.windowStart; sp.j <= sp.i; sp.j += 1) {
      price = sp.x_inReal[sp.j & sp.xMask];
      ret = 0.0;
      if (windowPrevPrice !== 0.0) {
        ret = (price - windowPrevPrice) / windowPrevPrice;
      }
      windowPrevPrice = price;
      windowTotal += ret;
      windowAbs += Math.abs(ret);
      dev = ret - sp.optInMAR;
      if (dev < 0.0) {
        windowDown += dev * dev;
      }
    }
    sp.peakDown = windowDown;
    sp.peakAbs = windowAbs;
    /* Re-remove the trailing return so the carried state matches the
     * path without a rebuild; the counts never drift.
     */
    price = sp.x_inReal[sp.windowStart & sp.xMask];
    ret = 0.0;
    if (sp.x_inReal[(sp.windowStart - 1) & sp.xMask] !== 0.0) {
      ret = (price - sp.x_inReal[(sp.windowStart - 1) & sp.xMask]) / sp.x_inReal[(sp.windowStart - 1) & sp.xMask];
    }
    sp.periodTotal = windowTotal - ret;
    sp.absTotal = windowAbs - Math.abs(ret);
    sp.downTotal = windowDown;
    dev = ret - sp.optInMAR;
    if (dev < 0.0) {
      sp.downTotal -= dev * dev;
    }
  }
  /* No downside is the plain quotient (ruled on #572): the mean excess
   * is then positive exactly when some return beats the MAR, giving
   * +Inf, and zero when every return equals it, giving NaN. The count
   * stands in for the mean so rounding cannot flip either case.
   * Annualize the per-bar ratio last, so an annualization of 1 is that
   * ratio bit for bit.
   */
  excess = windowTotal * sp.invPeriod - sp.optInMAR;
  if (windowBelow === 0) {
    windowDown = 0.0;
    excess = windowAbove;
  }
  ratio = excess / Math.sqrt(windowDown * sp.invPeriod);
  sp.cur_outReal = sp.sqrtAnnual * ratio;
  sp.i += 1;
}

function sortinoOpenImpl(
  sp: SortinoStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInMAR: number,
  optInAnnualization: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let periodTotal = 0;
  let downTotal = 0;
  let absTotal = 0;
  let peakDown = 0;
  let peakAbs = 0;
  let windowTotal = 0;
  let windowDown = 0;
  let windowAbs = 0;
  let price = 0;
  let prevPrice = 0;
  let trailingPrevPrice = 0;
  let windowPrevPrice = 0;
  let ret = 0;
  let dev = 0;
  let invPeriod = 0;
  let sqrtAnnual = 0;
  let excess = 0;
  let ratio = 0;
  let i = 0;
  let j = 0;
  let outIdx = 0;
  let trailingIdx = 0;
  let windowStart = 0;
  let barsSinceReseed = 0;
  let nbBelow = 0;
  let nbAbove = 0;
  let windowBelow = 0;
  let windowAbove = 0;
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
  if (optInMAR === Core.REAL_DEFAULT) {
    optInMAR = 0.0;
  } else if (!(optInMAR >= Core.REAL_MIN && optInMAR <= Core.REAL_MAX)) {
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
  invPeriod = 1.0 / optInTimePeriod;
  sqrtAnnual = Math.sqrt(optInAnnualization);
  /* The window holds the returns of the bars after trailingIdx up to i; a
   * bar's return reads the price before it. A return needs a non-zero base
   * price and nothing more: a zero base gives a zero return.
   *
   * The counts of returns below and above the MAR are exact, so they, not
   * the running sums, decide when the window has no downside.
   */
  trailingIdx = startIdx - optInTimePeriod;
  prevPrice = inReal[trailingIdx];
  periodTotal = 0.0;
  downTotal = 0.0;
  absTotal = 0.0;
  nbBelow = 0;
  nbAbove = 0;
  for (j = trailingIdx + 1; j < startIdx; j += 1) {
    price = inReal[j];
    ret = 0.0;
    if (prevPrice !== 0.0) {
      ret = (price - prevPrice) / prevPrice;
    }
    prevPrice = price;
    periodTotal += ret;
    absTotal += Math.abs(ret);
    dev = ret - optInMAR;
    if (dev < 0.0) {
      downTotal += dev * dev;
      nbBelow += 1;
    } else {
      nbAbove += +(dev > 0.0);
    }
  }
  i = startIdx;
  outIdx = 0;
  barsSinceReseed = ((32 * optInTimePeriod) | 0);
  peakDown = downTotal;
  peakAbs = absTotal;
  do {
    price = inReal[i];
    ret = 0.0;
    if (prevPrice !== 0.0) {
      ret = (price - prevPrice) / prevPrice;
    }
    prevPrice = price;
    periodTotal += ret;
    absTotal += Math.abs(ret);
    dev = ret - optInMAR;
    if (dev < 0.0) {
      downTotal += dev * dev;
      nbBelow += 1;
    } else {
      nbAbove += +(dev > 0.0);
    }
    peakDown = (downTotal > peakDown) ? downTotal : peakDown;
    peakAbs = (absTotal > peakAbs) ? absTotal : peakAbs;
    windowTotal = periodTotal;
    windowDown = downTotal;
    windowAbs = absTotal;
    windowBelow = nbBelow;
    windowAbove = nbAbove;
    /* inReal and outReal may be the same buffer, and the trailing prices
     * sit in slots the output reaches: every read of the window happens
     * before this bar's write.
     */
    trailingPrevPrice = inReal[trailingIdx];
    trailingIdx += 1;
    price = inReal[trailingIdx];
    ret = 0.0;
    if (trailingPrevPrice !== 0.0) {
      ret = (price - trailingPrevPrice) / trailingPrevPrice;
    }
    periodTotal -= ret;
    absTotal -= Math.abs(ret);
    dev = ret - optInMAR;
    if (dev < 0.0) {
      downTotal -= dev * dev;
      nbBelow -= 1;
    } else if (dev > 0.0) {
      nbAbove -= 1;
    }
    /* Rebuild when the downside sum, or the sum of absolute returns, has
     * shrunk below 1% of the largest one held since the last rebuild, or at
     * least every 32 windows. The rounding a running sum carries scales
     * with that peak, so once a large return leaves, the mean or the
     * divisor can be all rounding.
     */
    barsSinceReseed -= 1;
    if (windowDown < 0.01 * peakDown || windowAbs < 0.01 * peakAbs || barsSinceReseed <= 0) {
      barsSinceReseed = ((32 * optInTimePeriod) | 0);
      windowStart = i - optInTimePeriod + 1;
      windowPrevPrice = inReal[windowStart - 1];
      windowTotal = 0.0;
      windowDown = 0.0;
      windowAbs = 0.0;
      for (j = windowStart; j <= i; j += 1) {
        price = inReal[j];
        ret = 0.0;
        if (windowPrevPrice !== 0.0) {
          ret = (price - windowPrevPrice) / windowPrevPrice;
        }
        windowPrevPrice = price;
        windowTotal += ret;
        windowAbs += Math.abs(ret);
        dev = ret - optInMAR;
        if (dev < 0.0) {
          windowDown += dev * dev;
        }
      }
      peakDown = windowDown;
      peakAbs = windowAbs;
      /* Re-remove the trailing return so the carried state matches the
       * path without a rebuild; the counts never drift.
       */
      price = inReal[windowStart];
      ret = 0.0;
      if (inReal[windowStart - 1] !== 0.0) {
        ret = (price - inReal[windowStart - 1]) / inReal[windowStart - 1];
      }
      periodTotal = windowTotal - ret;
      absTotal = windowAbs - Math.abs(ret);
      downTotal = windowDown;
      dev = ret - optInMAR;
      if (dev < 0.0) {
        downTotal -= dev * dev;
      }
    }
    /* No downside is the plain quotient (ruled on #572): the mean excess
     * is then positive exactly when some return beats the MAR, giving
     * +Inf, and zero when every return equals it, giving NaN. The count
     * stands in for the mean so rounding cannot flip either case.
     * Annualize the per-bar ratio last, so an annualization of 1 is that
     * ratio bit for bit.
     */
    excess = windowTotal * invPeriod - optInMAR;
    if (windowBelow === 0) {
      windowDown = 0.0;
      excess = windowAbove;
    }
    ratio = excess / Math.sqrt(windowDown * invPeriod);
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
  sp.optInMAR = optInMAR;
  sp.optInAnnualization = optInAnnualization;
  sp.periodTotal = periodTotal;
  sp.downTotal = downTotal;
  sp.absTotal = absTotal;
  sp.peakDown = peakDown;
  sp.peakAbs = peakAbs;
  sp.prevPrice = prevPrice;
  sp.invPeriod = invPeriod;
  sp.sqrtAnnual = sqrtAnnual;
  sp.trailingIdx = trailingIdx;
  sp.barsSinceReseed = barsSinceReseed;
  sp.nbBelow = nbBelow;
  sp.nbAbove = nbAbove;
  sp.j = j;
  sp.windowStart = windowStart;
  sp.i = i;
  sp.xMask = physX - 1;
  sp.x_inReal = capX_inReal;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function sortinoOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInMAR: number,
  optInAnnualization: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): SortinoStream {
  const sp = SortinoStream.of(core);
  const retCode = sortinoOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInMAR, optInAnnualization, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SORTINO openAndFill', inReal.length, startIdx, sortinoLookbackPositional(optInTimePeriod, optInMAR, optInAnnualization, core));
  }
  if (retCode === RetCode.BAD_PARAM && sortinoLookbackPositional(optInTimePeriod, optInMAR, optInAnnualization, core) < 0) {
    throw rejectedParams('SORTINO openAndFill', PARAM_RANGES, optInTimePeriod, optInMAR, optInAnnualization);
  }
  throw streamFailure('SORTINO openAndFill', retCode);
}

/** @internal */
export function sortinoOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInMAR: number,
  optInAnnualization: number,
): SortinoStream {
  const sp = SortinoStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = sortinoOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInMAR, optInAnnualization, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SORTINO open', inReal.length, startIdx, sortinoLookbackPositional(optInTimePeriod, optInMAR, optInAnnualization, core));
  }
  if (retCode === RetCode.BAD_PARAM && sortinoLookbackPositional(optInTimePeriod, optInMAR, optInAnnualization, core) < 0) {
    throw rejectedParams('SORTINO open', PARAM_RANGES, optInTimePeriod, optInMAR, optInAnnualization);
  }
  throw streamFailure('SORTINO open', retCode);
}

/** @internal */
export function sortinoOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInMAR: number,
  optInAnnualization: number,
  core: Core = Core.DEFAULT,
): SortinoStream {
  optInTimePeriod = requireInt32('SORTINO open', 'timePeriod', optInTimePeriod);
  optInMAR = requireReal('SORTINO open', 'mar', optInMAR);
  optInAnnualization = requireReal('SORTINO open', 'annualization', optInAnnualization);
  requireArgument('SORTINO open', 'real', inReal);
  requireHistory('SORTINO open', 'real', inReal.length);
  return sortinoOpenInternal(core, float64(inReal), 0, optInTimePeriod, optInMAR, optInAnnualization);
}

/** @internal */
export function sortinoOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInMAR: number,
  optInAnnualization: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): SortinoStream {
  optInTimePeriod = requireInt32('SORTINO openAndFill', 'timePeriod', optInTimePeriod);
  optInMAR = requireReal('SORTINO openAndFill', 'mar', optInMAR);
  optInAnnualization = requireReal('SORTINO openAndFill', 'annualization', optInAnnualization);
  requireArgument('SORTINO openAndFill', 'real', inReal);
  requireHistory('SORTINO openAndFill', 'real', inReal.length);
  const guardLookback = sortinoLookbackPositional(optInTimePeriod, optInMAR, optInAnnualization, core);
  if (guardLookback < 0) {
    throw rejectedParams('SORTINO openAndFill', PARAM_RANGES, optInTimePeriod, optInMAR, optInAnnualization);
  }
  const guardOutLen = openFillCount('SORTINO openAndFill', inReal.length, guardLookback);
  requireOutput('SORTINO openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('SORTINO openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('SORTINO openAndFill', 'outReal', 'real');
  }
  return sortinoOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, optInMAR, optInAnnualization, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live SORTINO stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link sortino} there.
 *
 * @param real - Warm-up history. The price series; its returns are computed
 *   internally.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SortinoOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `sortinoLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a real parameter that is not a
 *   number or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { sortinoOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = sortinoOpen(data, {
 *   timePeriod: 20,
 *   mar: 0,
 *   annualization: 252,
 * });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function sortinoOpen(real: ArrayLike<number>, options?: SortinoOptions): SortinoStream {
  const o = optionsOf('SORTINO open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInMAR = orDefault(o.mar, Core.REAL_DEFAULT);
  const optInAnnualization = orDefault(o.annualization, Core.REAL_DEFAULT);
  const core = coreOf('SORTINO open', o.core);
  return sortinoOpenPositional(real, optInTimePeriod, optInMAR, optInAnnualization, core);
}

/**
 * {@link sortinoOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link sortino}.
 *
 * @param real - Warm-up history. The price series; its returns are computed
 *   internally.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link SortinoOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link sortino} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `sortinoLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a real parameter that is not a
 *   number, a parameter outside its range or a buffer too short, of the wrong type
 *   or starting where another buffer or a `Float64Array` input starts, or, on a
 *   history long enough to open, one sharing memory with another buffer or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { sortinoOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = sortinoOpenAndFill(data, {
 *   timePeriod: 20,
 *   mar: 0,
 *   annualization: 252,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function sortinoOpenAndFill(real: ArrayLike<number>, options?: SortinoOptions): SortinoResult & { stream: SortinoStream } {
  const o = optionsOf('SORTINO openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInMAR = orDefault(o.mar, Core.REAL_DEFAULT);
  const optInAnnualization = orDefault(o.annualization, Core.REAL_DEFAULT);
  const core = coreOf('SORTINO openAndFill', o.core);
  const capacity = fillCapacity(sortinoLookbackPositional(optInTimePeriod, optInMAR, optInAnnualization, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = sortinoOpenAndFillPositional(real, optInTimePeriod, optInMAR, optInAnnualization, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
