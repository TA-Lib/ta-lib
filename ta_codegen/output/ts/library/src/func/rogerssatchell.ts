/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  100526 KL,CC  Creation (#483).
 *  100526 MF,CC  Carry the window's terms in a ring (#483).
 *  100526 MF,CC  Rebuild trigger compares magnitudes (#483).
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireReal, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link rogerssatchell} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param optInTimePeriod - Number of bars in the window. `n = 1` is the paper's
 *   own single-bar estimator (default 10; range 1 to 100000; `Core.INTEGER_DEFAULT`
 *   selects the default).
 * @param optInAnnualization - Periods per year. Pass 1 for the per-bar figure
 *   (default 252; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function rogerssatchellLookback(optInTimePeriod: number, optInAnnualization: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (typeof optInAnnualization !== 'number') {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 10;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  if (optInAnnualization === Core.REAL_DEFAULT) {
    optInAnnualization = 252.0;
  } else if (!(optInAnnualization >= 0e0 && optInAnnualization <= Core.REAL_MAX)) {
    return -1;
  }
  return optInTimePeriod - 1;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link rogerssatchell}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Number of bars in the window. `n = 1` is the paper's
 *   own single-bar estimator (default 10; range 1 to 100000; `Core.INTEGER_DEFAULT`
 *   selects the default).
 * @param optInAnnualization - Periods per year. Pass 1 for the per-bar figure
 *   (default 252; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function rogerssatchellDisplayShift(optInTimePeriod: number, optInAnnualization: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (rogerssatchellLookback(optInTimePeriod, optInAnnualization, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function rogerssatchellImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  optInTimePeriod: number,
  optInAnnualization: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let o = 0;
  let h = 0;
  let l = 0;
  let c = 0;
  let p1 = 0;
  let p2 = 0;
  let term = 0;
  let periodTotal = 0;
  let windowTotal = 0;
  let windowMagnitude = 0;
  let peakTotal = 0;
  let sqrtA = 0;
  let i = 0;
  let j = 0;
  let outIdx = 0;
  let nbInitialElementNeeded = 0;
  let barsSinceRebuild = 0;
  let termRing!: Float64Array;
  let termRing_Idx = 0;
  let maxIdx_termRing = (32) - 1;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 10;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInAnnualization === Core.REAL_DEFAULT) {
    optInAnnualization = 252.0;
  } else if (!(optInAnnualization >= 0e0 && optInAnnualization <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  /* Each bar's term costs four logarithms, so it is computed once and kept
   * until it leaves the window. That also makes outReal safe to alias any
   * input: a bar is never read again once it has been consumed.
   */
  nbInitialElementNeeded = optInTimePeriod - 1;
  if (startIdx < nbInitialElementNeeded) {
    startIdx = nbInitialElementNeeded;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  termRing = new Float64Array(optInTimePeriod);
  maxIdx_termRing = optInTimePeriod - 1;
  termRing_Idx = 0;
  /* Rogers and Satchell, The Annals of Applied Probability 1(4):504-512 (1991),
   * eq. (2): with the log price measured from the bar's open, S1 = ln(H/O),
   * I1 = ln(L/O) and X1 = ln(C/O), one bar's estimate of its variance is
   * S1(S1 - X1) + I1(I1 - X1) = ln(H/C)ln(H/O) + ln(L/C)ln(L/O), unbiased
   * whatever the drift. The window mean, the root and the annual scale are
   * convention, not the paper's.
   *
   * Keep sqrt(A) a separate factor applied last: A = 1.0 is then an exact
   * identity and the annualised output is exactly sqrt(A) times the per-bar
   * one. Folding A under the root moves both by an ulp.
   */
  sqrtA = Math.sqrt(optInAnnualization);
  periodTotal = 0.0;
  for (j = startIdx - nbInitialElementNeeded; j < startIdx; j += 1) {
    /* Keep the two products separate statements. As one expression the
     * first product is fused into the add, which changes the values and
     * breaks bit equality with the same estimator composed from LN, DIV,
     * MULT, ADD and SUM.
     *
     * A bar with any price at or below zero contributes a 0.0 term and
     * still counts toward the window. The test is exact, not a band, so a
     * small-unit quote is not zeroed.
     */
    o = inOpen[j];
    h = inHigh[j];
    l = inLow[j];
    c = inClose[j];
    if (o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0) {
      p1 = Math.log(h / c) * Math.log(h / o);
      p2 = Math.log(l / c) * Math.log(l / o);
      term = p1 + p2;
    } else {
      term = 0.0;
    }
    termRing[termRing_Idx] = term;
    periodTotal += term;
    termRing_Idx++;
    if (termRing_Idx > maxIdx_termRing) {
      termRing_Idx = 0;
    }
  }
  i = startIdx;
  outIdx = 0;
  barsSinceRebuild = ((32 * optInTimePeriod) | 0);
  peakTotal = Math.abs(periodTotal);
  do {
    o = inOpen[i];
    h = inHigh[i];
    l = inLow[i];
    c = inClose[i];
    if (o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0) {
      p1 = Math.log(h / c) * Math.log(h / o);
      p2 = Math.log(l / c) * Math.log(l / o);
      term = p1 + p2;
    } else {
      term = 0.0;
    }
    /* Add, publish, subtract: the slot written here was taken out of the
     * sum at the end of the previous bar, and the advance lands on the
     * oldest term, the one that leaves next.
     */
    termRing[termRing_Idx] = term;
    periodTotal += term;
    windowTotal = periodTotal;
    windowMagnitude = Math.abs(windowTotal);
    peakTotal = (windowMagnitude > peakTotal) ? windowMagnitude : peakTotal;
    termRing_Idx++;
    if (termRing_Idx > maxIdx_termRing) {
      termRing_Idx = 0;
    }
    periodTotal -= termRing[termRing_Idx];
    /* A running sum carries rounding at the scale of the largest window it
     * has held, so it is rebuilt as a fresh sum once it falls below 1e-6 of
     * that peak, and at least every 32 windows. Compare against the peak,
     * not the current sum: after a quiet stretch arrives the current sum
     * can be nothing but that rounding, of either sign, where a fresh sum
     * of an all-flat window is exactly 0.0.
     *
     * Compare magnitudes. A window holding a bar whose high or low sits
     * inside its open and close can sum below zero, and a signed test
     * would then rebuild on every bar for as long as it does.
     *
     * Sum oldest first, so the rebuilt value is the one a fresh pass over
     * the bars gives.
     */
    barsSinceRebuild -= 1;
    if (windowMagnitude < 1e-6 * peakTotal || barsSinceRebuild <= 0) {
      barsSinceRebuild = ((32 * optInTimePeriod) | 0);
      windowTotal = 0.0;
      for (j = termRing_Idx; j < optInTimePeriod; j += 1) {
        windowTotal += termRing[j];
      }
      for (j = 0; j < termRing_Idx; j += 1) {
        windowTotal += termRing[j];
      }
      peakTotal = Math.abs(windowTotal);
      periodTotal = windowTotal;
      periodTotal -= termRing[termRing_Idx];
    }
    /* Divide, then root, then scale. A sum at or below zero answers 0.0
     * instead of reaching the root: only a bar whose high or low sits
     * strictly inside its open and close can make a fresh sum negative,
     * and the estimator is not defined on such a bar.
     */
    if (windowTotal > 0.0) {
      outReal[outIdx] = sqrtA * Math.sqrt(windowTotal / optInTimePeriod);
    } else {
      outReal[outIdx] = 0.0;
    }
    outIdx = outIdx + 1;
    i += 1;
  } while (i <= endIdx);
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/**
 * Rogers-Satchell volatility: a range-based estimator that reads one bar's open,
 * high, low and close as a single unbiased estimate of that bar's variance, then
 * reports the root of the mean over the last `n` bars, scaled to periods per year.
 * What separates it from the other range estimators is that it is unbiased
 * **whatever the drift**. A bar that opens at its low and closes at its high has
 * travelled in one direction and dispersed nothing around that path, and this
 * estimator reads it as exactly zero, where Parkinson and Garman-Klass read a wide
 * range as volatility. The price of that is a blind spot of its own: the estimator
 * has no close-to-open term, so overnight gaps are invisible to it. Read the
 * output as a fraction in log-return units, not price units and not percent. At an
 * `optInAnnualization` of 252 it is an annualised figure for daily bars; pass 1 to
 * leave the per-bar figure, 52 for weekly bars, 12 for monthly.
 *
 * Formula and more info at
 * [ta-lib.org/functions/rogerssatchell](https://ta-lib.org/functions/rogerssatchell).
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link rogerssatchellLookback} is a success
 * with `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inOpen - Open price of each bar.
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param optInTimePeriod - Number of bars in the window. `n = 1` is the paper's
 *   own single-bar estimator (default 10; range 1 to 100000; `Core.INTEGER_DEFAULT`
 *   selects the default).
 * @param optInAnnualization - Periods per year. Pass 1 for the per-bar figure
 *   (default 252; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param outReal - Estimated volatility, in log-return units. Must hold the values
 *   produced and never be empty; the input length always suffices.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The range written: `begIdx` is the first bar with a value, `count` how
 *   many were written.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for an argument that is not an int32, a
 *   real parameter that is not a number, a parameter outside its range, an array too
 *   short for the range, an empty output or an output sharing memory with a
 *   `Float64Array` input it is not. Checked before anything is written.
 *
 * @example
 * ```ts
 * import { rogerssatchell } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const out = new Float64Array(open.length);
 * const { begIdx, count } = rogerssatchell(
 *   0, open.length - 1,
 *   open, high, low, close,
 *   10, 252,
 *   out,
 * );
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`atr`](https://ta-lib.org/functions/atr)
 * @see [`natr`](https://ta-lib.org/functions/natr)
 * @see [`var_`](https://ta-lib.org/functions/var)
 */
export function rogerssatchell(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  optInAnnualization: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('ROGERSSATCHELL', 'startIdx', startIdx);
  endIdx = requireInt32('ROGERSSATCHELL', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('ROGERSSATCHELL', 'optInTimePeriod', optInTimePeriod);
  optInAnnualization = requireReal('ROGERSSATCHELL', 'optInAnnualization', optInAnnualization);
  requireIndexRange('ROGERSSATCHELL', startIdx, endIdx);
  const guardStart = clampedStart('ROGERSSATCHELL', startIdx, rogerssatchellLookback(optInTimePeriod, optInAnnualization, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('ROGERSSATCHELL', 'inOpen', inOpen, guardInLen);
  requireLength('ROGERSSATCHELL', 'inHigh', inHigh, guardInLen);
  requireLength('ROGERSSATCHELL', 'inLow', inLow, guardInLen);
  requireLength('ROGERSSATCHELL', 'inClose', inClose, guardInLen);
  requireOutput('ROGERSSATCHELL', 'outReal', outReal, guardOutLen, Float64Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outReal, inOpenF64) || overlapsPartly(outReal, inHighF64) || overlapsPartly(outReal, inLowF64) || overlapsPartly(outReal, inCloseF64)) {
    throw failure('ROGERSSATCHELL', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = rogerssatchellImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, optInTimePeriod, optInAnnualization, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('ROGERSSATCHELL', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live ROGERSSATCHELL stream: one value per closed bar, bit-identical to
 * {@link rogerssatchell} over the same series. Open it with
 * {@link rogerssatchellOpen} or {@link rogerssatchellOpenAndFill}; there is
 * nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/rogerssatchell](https://ta-lib.org/functions/rogerssatchell).
 */
export class RogerssatchellStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  optInAnnualization: number = 0;
  /** @internal */
  periodTotal: number = 0;
  /** @internal */
  peakTotal: number = 0;
  /** @internal */
  sqrtA: number = 0;
  /** @internal */
  barsSinceRebuild: number = 0;
  /** @internal */
  termRing_Idx: number = 0;
  /** @internal */
  maxIdx_termRing: number = 0;
  /** @internal */
  cbSize_termRing: number = 0;
  /** @internal */
  cb_termRing: Float64Array = new Float64Array(1);
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
  static of(core: Core): RogerssatchellStream {
    return new RogerssatchellStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link rogerssatchell} reports them over the same bars. Each accepted `update`
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
      throw failure('ROGERSSATCHELL advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inOpen - The bar's `inOpen` value: Open price of each bar.
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inOpen: number, inHigh: number, inLow: number, inClose: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('ROGERSSATCHELL update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inOpen) || !Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('ROGERSSATCHELL update', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    rogerssatchellStepImpl(this, inOpen, inHigh, inLow, inClose);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inOpen - The bar's `inOpen` value: Open price of each bar.
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inOpen: number, inHigh: number, inLow: number, inClose: number): number {
    if (!Number.isFinite(inOpen) || !Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('ROGERSSATCHELL peek', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let o = 0;
    let h = 0;
    let l = 0;
    let c = 0;
    let p1 = 0;
    let p2 = 0;
    let term = 0;
    let windowTotal = 0;
    let windowMagnitude = 0;
    let j = 0;
    let barsSinceRebuild = sp.barsSinceRebuild;
    let cur_outReal = 0;
    let peakTotal = sp.peakTotal;
    let periodTotal = sp.periodTotal;
    let termRing_Idx = sp.termRing_Idx;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    o = inOpen;
    h = inHigh;
    l = inLow;
    c = inClose;
    if (o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0) {
      p1 = Math.log(h / c) * Math.log(h / o);
      p2 = Math.log(l / c) * Math.log(l / o);
      term = p1 + p2;
    } else {
      term = 0.0;
    }
    /* Add, publish, subtract: the slot written here was taken out of the
     * sum at the end of the previous bar, and the advance lands on the
     * oldest term, the one that leaves next.
     */
    pkSlot0 = termRing_Idx;
    pkVal0 = term;
    periodTotal += term;
    windowTotal = periodTotal;
    windowMagnitude = Math.abs(windowTotal);
    peakTotal = (windowMagnitude > peakTotal) ? windowMagnitude : peakTotal;
    termRing_Idx = termRing_Idx + 1;
    if (termRing_Idx > sp.maxIdx_termRing) {
      termRing_Idx = 0;
    }
    periodTotal -= (termRing_Idx !== pkSlot0) ? sp.cb_termRing[termRing_Idx] : pkVal0;
    /* A running sum carries rounding at the scale of the largest window it
     * has held, so it is rebuilt as a fresh sum once it falls below 1e-6 of
     * that peak, and at least every 32 windows. Compare against the peak,
     * not the current sum: after a quiet stretch arrives the current sum
     * can be nothing but that rounding, of either sign, where a fresh sum
     * of an all-flat window is exactly 0.0.
     *
     * Compare magnitudes. A window holding a bar whose high or low sits
     * inside its open and close can sum below zero, and a signed test
     * would then rebuild on every bar for as long as it does.
     *
     * Sum oldest first, so the rebuilt value is the one a fresh pass over
     * the bars gives.
     */
    barsSinceRebuild -= 1;
    if (windowMagnitude < 1e-6 * peakTotal || barsSinceRebuild <= 0) {
      barsSinceRebuild = ((32 * sp.optInTimePeriod) | 0);
      windowTotal = 0.0;
      for (j = termRing_Idx; j < sp.optInTimePeriod; j += 1) {
        windowTotal += (j !== pkSlot0) ? sp.cb_termRing[j] : pkVal0;
      }
      for (j = 0; j < termRing_Idx; j += 1) {
        windowTotal += (j !== pkSlot0) ? sp.cb_termRing[j] : pkVal0;
      }
      peakTotal = Math.abs(windowTotal);
      periodTotal = windowTotal;
      periodTotal -= (termRing_Idx !== pkSlot0) ? sp.cb_termRing[termRing_Idx] : pkVal0;
    }
    /* Divide, then root, then scale. A sum at or below zero answers 0.0
     * instead of reaching the root: only a bar whose high or low sits
     * strictly inside its open and close can make a fresh sum negative,
     * and the estimator is not defined on such a bar.
     */
    if (windowTotal > 0.0) {
      cur_outReal = sp.sqrtA * Math.sqrt(windowTotal / sp.optInTimePeriod);
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
  clone(): RogerssatchellStream {
    const c = new RogerssatchellStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.optInAnnualization = this.optInAnnualization;
    c.periodTotal = this.periodTotal;
    c.peakTotal = this.peakTotal;
    c.sqrtA = this.sqrtA;
    c.barsSinceRebuild = this.barsSinceRebuild;
    c.termRing_Idx = this.termRing_Idx;
    c.maxIdx_termRing = this.maxIdx_termRing;
    c.cbSize_termRing = this.cbSize_termRing;
    c.cb_termRing = this.cb_termRing.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function rogerssatchellStepImpl(sp: RogerssatchellStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  let o = 0;
  let h = 0;
  let l = 0;
  let c = 0;
  let p1 = 0;
  let p2 = 0;
  let term = 0;
  let windowTotal = 0;
  let windowMagnitude = 0;
  let j = 0;
  o = inOpen;
  h = inHigh;
  l = inLow;
  c = inClose;
  if (o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0) {
    p1 = Math.log(h / c) * Math.log(h / o);
    p2 = Math.log(l / c) * Math.log(l / o);
    term = p1 + p2;
  } else {
    term = 0.0;
  }
  /* Add, publish, subtract: the slot written here was taken out of the
   * sum at the end of the previous bar, and the advance lands on the
   * oldest term, the one that leaves next.
   */
  sp.cb_termRing[sp.termRing_Idx] = term;
  sp.periodTotal += term;
  windowTotal = sp.periodTotal;
  windowMagnitude = Math.abs(windowTotal);
  sp.peakTotal = (windowMagnitude > sp.peakTotal) ? windowMagnitude : sp.peakTotal;
  sp.termRing_Idx = sp.termRing_Idx + 1;
  if (sp.termRing_Idx > sp.maxIdx_termRing) {
    sp.termRing_Idx = 0;
  }
  sp.periodTotal -= sp.cb_termRing[sp.termRing_Idx];
  /* A running sum carries rounding at the scale of the largest window it
   * has held, so it is rebuilt as a fresh sum once it falls below 1e-6 of
   * that peak, and at least every 32 windows. Compare against the peak,
   * not the current sum: after a quiet stretch arrives the current sum
   * can be nothing but that rounding, of either sign, where a fresh sum
   * of an all-flat window is exactly 0.0.
   *
   * Compare magnitudes. A window holding a bar whose high or low sits
   * inside its open and close can sum below zero, and a signed test
   * would then rebuild on every bar for as long as it does.
   *
   * Sum oldest first, so the rebuilt value is the one a fresh pass over
   * the bars gives.
   */
  sp.barsSinceRebuild -= 1;
  if (windowMagnitude < 1e-6 * sp.peakTotal || sp.barsSinceRebuild <= 0) {
    sp.barsSinceRebuild = ((32 * sp.optInTimePeriod) | 0);
    windowTotal = 0.0;
    for (j = sp.termRing_Idx; j < sp.optInTimePeriod; j += 1) {
      windowTotal += sp.cb_termRing[j];
    }
    for (j = 0; j < sp.termRing_Idx; j += 1) {
      windowTotal += sp.cb_termRing[j];
    }
    sp.peakTotal = Math.abs(windowTotal);
    sp.periodTotal = windowTotal;
    sp.periodTotal -= sp.cb_termRing[sp.termRing_Idx];
  }
  /* Divide, then root, then scale. A sum at or below zero answers 0.0
   * instead of reaching the root: only a bar whose high or low sits
   * strictly inside its open and close can make a fresh sum negative,
   * and the estimator is not defined on such a bar.
   */
  if (windowTotal > 0.0) {
    sp.cur_outReal = sp.sqrtA * Math.sqrt(windowTotal / sp.optInTimePeriod);
  } else {
    sp.cur_outReal = 0.0;
  }
}

function rogerssatchellOpenImpl(
  sp: RogerssatchellStream,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInAnnualization: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let o = 0;
  let h = 0;
  let l = 0;
  let c = 0;
  let p1 = 0;
  let p2 = 0;
  let term = 0;
  let periodTotal = 0;
  let windowTotal = 0;
  let windowMagnitude = 0;
  let peakTotal = 0;
  let sqrtA = 0;
  let i = 0;
  let j = 0;
  let outIdx = 0;
  let nbInitialElementNeeded = 0;
  let barsSinceRebuild = 0;
  let termRing!: Float64Array;
  let termRing_Idx = 0;
  let maxIdx_termRing = (32) - 1;
  const historyLen = inOpen.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inHigh.length !== inOpen.length || inLow.length !== inOpen.length || inClose.length !== inOpen.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 10;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInAnnualization === Core.REAL_DEFAULT) {
    optInAnnualization = 252.0;
  } else if (!(optInAnnualization >= 0e0 && optInAnnualization <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Each bar's term costs four logarithms, so it is computed once and kept
   * until it leaves the window. That also makes outReal safe to alias any
   * input: a bar is never read again once it has been consumed.
   */
  nbInitialElementNeeded = optInTimePeriod - 1;
  if (startIdx < nbInitialElementNeeded) {
    startIdx = nbInitialElementNeeded;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  termRing = new Float64Array(optInTimePeriod);
  maxIdx_termRing = optInTimePeriod - 1;
  termRing_Idx = 0;
  /* Rogers and Satchell, The Annals of Applied Probability 1(4):504-512 (1991),
   * eq. (2): with the log price measured from the bar's open, S1 = ln(H/O),
   * I1 = ln(L/O) and X1 = ln(C/O), one bar's estimate of its variance is
   * S1(S1 - X1) + I1(I1 - X1) = ln(H/C)ln(H/O) + ln(L/C)ln(L/O), unbiased
   * whatever the drift. The window mean, the root and the annual scale are
   * convention, not the paper's.
   *
   * Keep sqrt(A) a separate factor applied last: A = 1.0 is then an exact
   * identity and the annualised output is exactly sqrt(A) times the per-bar
   * one. Folding A under the root moves both by an ulp.
   */
  sqrtA = Math.sqrt(optInAnnualization);
  periodTotal = 0.0;
  for (j = startIdx - nbInitialElementNeeded; j < startIdx; j += 1) {
    /* Keep the two products separate statements. As one expression the
     * first product is fused into the add, which changes the values and
     * breaks bit equality with the same estimator composed from LN, DIV,
     * MULT, ADD and SUM.
     *
     * A bar with any price at or below zero contributes a 0.0 term and
     * still counts toward the window. The test is exact, not a band, so a
     * small-unit quote is not zeroed.
     */
    o = inOpen[j];
    h = inHigh[j];
    l = inLow[j];
    c = inClose[j];
    if (o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0) {
      p1 = Math.log(h / c) * Math.log(h / o);
      p2 = Math.log(l / c) * Math.log(l / o);
      term = p1 + p2;
    } else {
      term = 0.0;
    }
    termRing[termRing_Idx] = term;
    periodTotal += term;
    termRing_Idx++;
    if (termRing_Idx > maxIdx_termRing) {
      termRing_Idx = 0;
    }
  }
  i = startIdx;
  outIdx = 0;
  barsSinceRebuild = ((32 * optInTimePeriod) | 0);
  peakTotal = Math.abs(periodTotal);
  do {
    o = inOpen[i];
    h = inHigh[i];
    l = inLow[i];
    c = inClose[i];
    if (o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0) {
      p1 = Math.log(h / c) * Math.log(h / o);
      p2 = Math.log(l / c) * Math.log(l / o);
      term = p1 + p2;
    } else {
      term = 0.0;
    }
    /* Add, publish, subtract: the slot written here was taken out of the
     * sum at the end of the previous bar, and the advance lands on the
     * oldest term, the one that leaves next.
     */
    termRing[termRing_Idx] = term;
    periodTotal += term;
    windowTotal = periodTotal;
    windowMagnitude = Math.abs(windowTotal);
    peakTotal = (windowMagnitude > peakTotal) ? windowMagnitude : peakTotal;
    termRing_Idx++;
    if (termRing_Idx > maxIdx_termRing) {
      termRing_Idx = 0;
    }
    periodTotal -= termRing[termRing_Idx];
    /* A running sum carries rounding at the scale of the largest window it
     * has held, so it is rebuilt as a fresh sum once it falls below 1e-6 of
     * that peak, and at least every 32 windows. Compare against the peak,
     * not the current sum: after a quiet stretch arrives the current sum
     * can be nothing but that rounding, of either sign, where a fresh sum
     * of an all-flat window is exactly 0.0.
     *
     * Compare magnitudes. A window holding a bar whose high or low sits
     * inside its open and close can sum below zero, and a signed test
     * would then rebuild on every bar for as long as it does.
     *
     * Sum oldest first, so the rebuilt value is the one a fresh pass over
     * the bars gives.
     */
    barsSinceRebuild -= 1;
    if (windowMagnitude < 1e-6 * peakTotal || barsSinceRebuild <= 0) {
      barsSinceRebuild = ((32 * optInTimePeriod) | 0);
      windowTotal = 0.0;
      for (j = termRing_Idx; j < optInTimePeriod; j += 1) {
        windowTotal += termRing[j];
      }
      for (j = 0; j < termRing_Idx; j += 1) {
        windowTotal += termRing[j];
      }
      peakTotal = Math.abs(windowTotal);
      periodTotal = windowTotal;
      periodTotal -= termRing[termRing_Idx];
    }
    /* Divide, then root, then scale. A sum at or below zero answers 0.0
     * instead of reaching the root: only a bar whose high or low sits
     * strictly inside its open and close can make a fresh sum negative,
     * and the estimator is not defined on such a bar.
     */
    if (windowTotal > 0.0) {
      outReal[((outIdx * outStride) | 0)] = sqrtA * Math.sqrt(windowTotal / optInTimePeriod);
    } else {
      outReal[((outIdx * outStride) | 0)] = 0.0;
    }
    outIdx = outIdx + 1;
    i += 1;
  } while (i <= endIdx);
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const capCb_termRing = maxIdx_termRing + 1;
  if (capCb_termRing > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.optInAnnualization = optInAnnualization;
  sp.periodTotal = periodTotal;
  sp.peakTotal = peakTotal;
  sp.sqrtA = sqrtA;
  sp.barsSinceRebuild = barsSinceRebuild;
  sp.termRing_Idx = termRing_Idx;
  sp.maxIdx_termRing = maxIdx_termRing;
  sp.cbSize_termRing = capCb_termRing;
  sp.cb_termRing = termRing;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function rogerssatchellOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInAnnualization: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RogerssatchellStream {
  const sp = RogerssatchellStream.of(core);
  const retCode = rogerssatchellOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, optInTimePeriod, optInAnnualization, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ROGERSSATCHELL openAndFill', inOpen.length, startIdx, rogerssatchellLookback(optInTimePeriod, optInAnnualization, core));
  }
  throw streamFailure('ROGERSSATCHELL openAndFill', retCode);
}

/** @internal */
export function rogerssatchellOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInAnnualization: number,
): RogerssatchellStream {
  const sp = RogerssatchellStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = rogerssatchellOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, optInTimePeriod, optInAnnualization, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ROGERSSATCHELL open', inOpen.length, startIdx, rogerssatchellLookback(optInTimePeriod, optInAnnualization, core));
  }
  throw streamFailure('ROGERSSATCHELL open', retCode);
}

/**
 * Opens a live ROGERSSATCHELL stream over the warm-up history: `value()` starts at
 * the last history bar, bit-identical to {@link rogerssatchell} there.
 *
 * @param inOpen - Warm-up history. Open price of each bar. Every history has the
 *   same length.
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Number of bars in the window. `n = 1` is the paper's
 *   own single-bar estimator (default 10; range 1 to 100000; `Core.INTEGER_DEFAULT`
 *   selects the default).
 * @param optInAnnualization - Periods per year. Pass 1 for the per-bar figure
 *   (default 252; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `rogerssatchellLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for
 *   an empty one, and `TA_BAD_PARAM` for an integer parameter that is not an int32,
 *   a real parameter that is not a number, a parameter outside its range or
 *   histories of different lengths.
 *
 * @example
 * ```ts
 * import { rogerssatchellOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = rogerssatchellOpen(open, high, low, close, 10, 252);
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function rogerssatchellOpen(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  optInAnnualization: number,
  core: Core = Core.DEFAULT,
): RogerssatchellStream {
  optInTimePeriod = requireInt32('ROGERSSATCHELL open', 'optInTimePeriod', optInTimePeriod);
  optInAnnualization = requireReal('ROGERSSATCHELL open', 'optInAnnualization', optInAnnualization);
  requireArgument('ROGERSSATCHELL open', 'inOpen', inOpen);
  requireHistory('ROGERSSATCHELL open', inOpen.length);
  requireArgument('ROGERSSATCHELL open', 'inHigh', inHigh);
  requireArgument('ROGERSSATCHELL open', 'inLow', inLow);
  requireArgument('ROGERSSATCHELL open', 'inClose', inClose);
  requireHistoryLength('ROGERSSATCHELL open', 'inHigh', inHigh.length, inOpen.length);
  requireHistoryLength('ROGERSSATCHELL open', 'inLow', inLow.length, inOpen.length);
  requireHistoryLength('ROGERSSATCHELL open', 'inClose', inClose.length, inOpen.length);
  return rogerssatchellOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0, optInTimePeriod, optInAnnualization);
}

/**
 * {@link rogerssatchellOpen} that also fills the outputs over the whole history in
 * the same pass, bit-identically to {@link rogerssatchell}. The range written is
 * the stream's `outRange()`.
 *
 * @param inOpen - Warm-up history. Open price of each bar. Every history has the
 *   same length.
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Number of bars in the window. `n = 1` is the paper's
 *   own single-bar estimator (default 10; range 1 to 100000; `Core.INTEGER_DEFAULT`
 *   selects the default).
 * @param optInAnnualization - Periods per year. Pass 1 for the per-bar figure
 *   (default 252; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param outReal - Estimated volatility, in log-return units. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `rogerssatchellLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for
 *   an empty one, and `TA_BAD_PARAM` for an integer parameter that is not an int32,
 *   a real parameter that is not a number, a parameter outside its range, histories
 *   of different lengths or an output too short or starting where another output or
 *   a `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { rogerssatchellOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const out = new Float64Array(open.length);
 * const stream = rogerssatchellOpenAndFill(open, high, low, close, 10, 252, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function rogerssatchellOpenAndFill(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  optInAnnualization: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): RogerssatchellStream {
  optInTimePeriod = requireInt32('ROGERSSATCHELL openAndFill', 'optInTimePeriod', optInTimePeriod);
  optInAnnualization = requireReal('ROGERSSATCHELL openAndFill', 'optInAnnualization', optInAnnualization);
  requireArgument('ROGERSSATCHELL openAndFill', 'inOpen', inOpen);
  requireHistory('ROGERSSATCHELL openAndFill', inOpen.length);
  requireArgument('ROGERSSATCHELL openAndFill', 'inHigh', inHigh);
  requireArgument('ROGERSSATCHELL openAndFill', 'inLow', inLow);
  requireArgument('ROGERSSATCHELL openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('ROGERSSATCHELL openAndFill', inOpen.length, rogerssatchellLookback(optInTimePeriod, optInAnnualization, core));
  requireHistoryLength('ROGERSSATCHELL openAndFill', 'inHigh', inHigh.length, inOpen.length);
  requireHistoryLength('ROGERSSATCHELL openAndFill', 'inLow', inLow.length, inOpen.length);
  requireHistoryLength('ROGERSSATCHELL openAndFill', 'inClose', inClose.length, inOpen.length);
  requireOutput('ROGERSSATCHELL openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outReal.buffer === inOpenF64.buffer && outReal.byteOffset === inOpenF64.byteOffset) || (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) || (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) || (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset)) {
    throw streamFailure('ROGERSSATCHELL openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inOpenF64) || overlaps(outReal, inHighF64) || overlaps(outReal, inLowF64) || overlaps(outReal, inCloseF64))) {
    throw streamFailure('ROGERSSATCHELL openAndFill', RetCode.BAD_PARAM);
  }
  return rogerssatchellOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, optInTimePeriod, optInAnnualization, new MInteger(), new MInteger(), outReal);
}
