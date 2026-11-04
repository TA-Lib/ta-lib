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
 *  091526 KL     First version (proposal-drafts issue #72).
 *  092126 MF,CC  Rebuild against the peak moments (issue #433).
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link kurtosis} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Number of trailing values in the window (default 30;
 *   range 4 to 10000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function kurtosisLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 4 || optInTimePeriod > 10000) {
    return -1;
  }
  return optInTimePeriod - 1;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link kurtosis}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Number of trailing values in the window (default 30;
 *   range 4 to 10000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function kurtosisDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (kurtosisLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function kurtosisImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let tempReal = 0;
  let shift = 0;
  let total1 = 0;
  let total2 = 0;
  let total3 = 0;
  let total4 = 0;
  let dev = 0;
  let dev2 = 0;
  let residue = 0;
  let residueSq = 0;
  let moment2 = 0;
  let moment4 = 0;
  let sampleVar = 0;
  let varSquared = 0;
  let dPeriod = 0;
  let coefA = 0;
  let coefB = 0;
  let invPeriod = 0;
  let kurt = 0;
  let peak2 = 0;
  let peak4 = 0;
  let i = 0;
  let j = 0;
  let outIdx = 0;
  let trailingIdx = 0;
  let windowStart = 0;
  let nbInitialElementNeeded = 0;
  let barsSinceReseed = 0;
  let reseedPeriod = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 4 || optInTimePeriod > 10000) {
    return RetCode.BAD_PARAM;
  }
  nbInitialElementNeeded = optInTimePeriod - 1;
  if (startIdx < nbInitialElementNeeded) {
    startIdx = nbInitialElementNeeded;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  /* G2, the sample-adjusted Fisher excess kurtosis. The coefficients are
   * computed in double: at the top of the parameter range coefA's denominator
   * (n-1)(n-2)(n-3) is ~1e12, far past what an int holds.
   *
   * The (n-2)(n-3) denominators are why the range starts at 4 rather than 1.
   * The argument contract rejects anything below it, and the n-1 lookback
   * guarantees a full window at the first emitted bar, so the estimator is
   * never handed fewer than four points and needs no runtime branch for it.
   */
  dPeriod = optInTimePeriod;
  coefA = dPeriod * (dPeriod + 1.0) / ((dPeriod - 1.0) * (dPeriod - 2.0) * (dPeriod - 3.0));
  coefB = 3.0 * (dPeriod - 1.0) * (dPeriod - 1.0) / ((dPeriod - 2.0) * (dPeriod - 3.0));
  invPeriod = 1.0 / dPeriod;
  /* Deviations are measured against a shift near the window, so the running
   * sums stay at deviation scale instead of at price scale, and the central
   * moments are recovered from them by the binomial expansion below. The
   * period bounds the rounding the slide accumulates and is the only way out
   * of a NaN or Inf input, on which every trigger comparison is false; a stale
   * shift is caught by the triggers in the loop, not by this.
   */
  reseedPeriod = ((32 * optInTimePeriod) | 0);
  trailingIdx = startIdx - nbInitialElementNeeded;
  shift = inReal[trailingIdx];
  total1 = 0.0;
  total2 = 0.0;
  total3 = 0.0;
  total4 = 0.0;
  for (j = trailingIdx; j < startIdx; j += 1) {
    dev = inReal[j] - shift;
    dev2 = dev * dev;
    total1 += dev;
    total2 += dev2;
    total3 += dev2 * dev;
    total4 += dev2 * dev2;
  }
  i = startIdx;
  outIdx = 0;
  barsSinceReseed = reseedPeriod;
  peak2 = total2;
  peak4 = total4;
  do {
    dev = inReal[i] - shift;
    dev2 = dev * dev;
    total1 += dev;
    total2 += dev2;
    total3 += dev2 * dev;
    total4 += dev2 * dev2;
    if (total2 > peak2) {
      peak2 = total2;
    }
    if (total4 > peak4) {
      peak4 = total4;
    }
    residue = total1 * invPeriod;
    residueSq = residue * residue;
    moment2 = total2 - dPeriod * residueSq;
    let _fma0: number;
    {
      const _a: number = 6.0 * residueSq, _b: number = total2, _c: number = total4 - 4.0 * residue * total3;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      _fma0 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    moment4 = _fma0 - 3.0 * dPeriod * residueSq * residueSq;
    /* Rebuild once either central moment falls below 1% of the largest
     * shifted sum of its power held since the last rebuild. That catches a
     * shift several sigma stale, where the expansion above cancels as
     * (u/sigma)^4, and a series decaying back onto the shift or an outlier
     * leaving the window, which leave current sums made mostly of rounding
     * that scales with the peaks.
     */
    barsSinceReseed -= 1;
    if (moment2 < 0.01 * peak2 || moment4 < 0.01 * peak4 || barsSinceReseed <= 0) {
      barsSinceReseed = reseedPeriod;
      windowStart = i - nbInitialElementNeeded;
      tempReal = 0.0;
      for (j = windowStart; j <= i; j += 1) {
        tempReal += inReal[j];
      }
      shift = tempReal * invPeriod;
      total1 = 0.0;
      total2 = 0.0;
      total3 = 0.0;
      total4 = 0.0;
      for (j = windowStart; j <= i; j += 1) {
        dev = inReal[j] - shift;
        dev2 = dev * dev;
        total1 += dev;
        total2 += dev2;
        total3 += dev2 * dev;
        total4 += dev2 * dev2;
      }
      residue = total1 * invPeriod;
      residueSq = residue * residue;
      moment2 = total2 - dPeriod * residueSq;
      let _fma1: number;
      {
        const _a: number = 6.0 * residueSq, _b: number = total2, _c: number = total4 - 4.0 * residue * total3;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        _fma1 = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      moment4 = _fma1 - 3.0 * dPeriod * residueSq * residueSq;
      /* The left-to-right sum misses the mean by many ulps at a long
       * period, and a window whose spread is under that miss would fire a
       * trigger again on every bar. The residue just measured is the
       * miss: adding it back lands within about an ulp of the mean, where
       * a flat window's deviations are exactly 0.
       */
      if (moment2 < 0.01 * total2 || moment4 < 0.01 * total4) {
        shift += residue;
        total1 = 0.0;
        total2 = 0.0;
        total3 = 0.0;
        total4 = 0.0;
        for (j = windowStart; j <= i; j += 1) {
          dev = inReal[j] - shift;
          dev2 = dev * dev;
          total1 += dev;
          total2 += dev2;
          total3 += dev2 * dev;
          total4 += dev2 * dev2;
        }
        residue = total1 * invPeriod;
        residueSq = residue * residue;
        moment2 = total2 - dPeriod * residueSq;
        let _fma2: number;
        {
          const _a: number = 6.0 * residueSq, _b: number = total2, _c: number = total4 - 4.0 * residue * total3;
          const _p = _a * _b;
          let _t = 134217729 * _a;
          const _ah = _t - (_t - _a), _al = _a - _ah;
          _t = 134217729 * _b;
          const _bh = _t - (_t - _b), _bl = _b - _bh;
          const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
          const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
          const _v = _tl + _pl, _v2 = _v + _v;
          const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
          _fma2 = _w && (_pl === 0 || _v === 0) ? _th
            : _w && _v2 !== (_th + _v2) - _th ? _th + _v
            : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
        }
        moment4 = _fma2 - 3.0 * dPeriod * residueSq * residueSq;
      }
      peak2 = total2;
      peak4 = total4;
    }
    /* No spread, no kurtosis: 0 would assert normality and -1.2 uniformity.
     * A flat window reaches here with every deviation exactly 0, so this is
     * 0/0 and IEEE answers NaN without a branch. A guard with an absolute
     * epsilon would be a cliff at a price level, not a noise floor (#243).
     */
    sampleVar = moment2 / (dPeriod - 1.0);
    varSquared = sampleVar * sampleVar;
    kurt = coefA * (moment4 / varSquared) - coefB;
    /* inReal and outReal may be the same buffer: read the trailing value
     * before the output write.
     */
    dev = inReal[trailingIdx] - shift;
    dev2 = dev * dev;
    total1 -= dev;
    total2 -= dev2;
    total3 -= dev2 * dev;
    total4 -= dev2 * dev2;
    trailingIdx += 1;
    outReal[outIdx++] = kurt;
    i += 1;
  } while (i <= endIdx);
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/**
 * An estimate, from the trailing window, of the excess kurtosis of the
 * distribution its values come from: the fourth standardised moment minus 3, so a
 * normal distribution reads 0. A tail-weight measure: above 0 the distribution has
 * heavier tails than a normal one with the same variance, below 0 lighter ones.
 * The companion to the shipped [`VAR`](https://ta-lib.org/functions/var) and
 * [`STDDEV`](https://ta-lib.org/functions/stddev) in the same group.
 *
 * Formula and more info at
 * [ta-lib.org/functions/kurtosis](https://ta-lib.org/functions/kurtosis).
 *
 * **Notes**
 * - This is `G2`, the form Excel `KURT` and `scipy.stats.kurtosis(bias=False)`
 *   compute. The other form in circulation is the biased `g2 = m₄/m₂² − 3` over
 *   population moments, a different estimator rather than a rounding convention:
 *   `G2 = (n−1)((n+1)·g2 + 6) / ((n−2)(n−3))`.
 * - A point mass has no defensible excess kurtosis, so the degenerate window is
 *   NaN rather than a number: `0` would assert normality and `−1.2` uniformity.
 *   This is deliberately unlike `VAR`, which floors to zero — a variance of zero
 *   says something true about the window.
 * - The result is at most `n`, the value of one reading apart from `n−1` equal
 *   ones, so a window dominated by one outlier is legitimately far above 0.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link kurtosisLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - The series to measure.
 * @param optInTimePeriod - Number of trailing values in the window (default 30;
 *   range 4 to 10000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Excess kurtosis of the trailing window, or NaN where the window
 *   has no spread. Must hold the values produced and never be empty; the input
 *   length always suffices.
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
 * import { kurtosis } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const { begIdx, count } = kurtosis(0, data.length - 1, data, 30, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`var_`](https://ta-lib.org/functions/var)
 * @see [`stddev`](https://ta-lib.org/functions/stddev)
 * @see [`bbands`](https://ta-lib.org/functions/bbands)
 */
export function kurtosis(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('KURTOSIS', 'startIdx', startIdx);
  endIdx = requireInt32('KURTOSIS', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('KURTOSIS', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('KURTOSIS', startIdx, endIdx);
  const guardStart = clampedStart('KURTOSIS', startIdx, kurtosisLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('KURTOSIS', 'inReal', inReal, guardInLen);
  requireOutput('KURTOSIS', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw failure('KURTOSIS', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = kurtosisImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('KURTOSIS', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live KURTOSIS stream: one value per closed bar, bit-identical to
 * {@link kurtosis} over the same series. Open it with {@link kurtosisOpen} or
 * {@link kurtosisOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/kurtosis](https://ta-lib.org/functions/kurtosis).
 */
export class KurtosisStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  shift: number = 0;
  /** @internal */
  total1: number = 0;
  /** @internal */
  total2: number = 0;
  /** @internal */
  total3: number = 0;
  /** @internal */
  total4: number = 0;
  /** @internal */
  dPeriod: number = 0;
  /** @internal */
  coefA: number = 0;
  /** @internal */
  coefB: number = 0;
  /** @internal */
  invPeriod: number = 0;
  /** @internal */
  peak2: number = 0;
  /** @internal */
  peak4: number = 0;
  /** @internal */
  trailingIdx: number = 0;
  /** @internal */
  nbInitialElementNeeded: number = 0;
  /** @internal */
  barsSinceReseed: number = 0;
  /** @internal */
  reseedPeriod: number = 0;
  /** @internal */
  j: number = 0;
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
  static of(core: Core): KurtosisStream {
    return new KurtosisStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link kurtosis} reports them over the same bars. Each accepted `update` and
   * each `advance` adds one to `count`.
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
      throw failure('KURTOSIS advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: The series to measure.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('KURTOSIS update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('KURTOSIS update', 'inReal');
    }
    kurtosisStepImpl(this, inReal);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: The series to measure.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number): number {
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('KURTOSIS peek', 'inReal');
    }
    const sp = this;
    let tempReal = 0;
    let dev = 0;
    let dev2 = 0;
    let residue = 0;
    let residueSq = 0;
    let moment2 = 0;
    let moment4 = 0;
    let sampleVar = 0;
    let varSquared = 0;
    let kurt = 0;
    let windowStart = 0;
    let barsSinceReseed = sp.barsSinceReseed;
    let cur_outReal = 0;
    let j = sp.j;
    let peak2 = sp.peak2;
    let peak4 = sp.peak4;
    let shift = sp.shift;
    let total1 = sp.total1;
    let total2 = sp.total2;
    let total3 = sp.total3;
    let total4 = sp.total4;
    let trailingIdx = sp.trailingIdx;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    pkSlot0 = sp.i & sp.xMask;
    pkVal0 = inReal;
    dev = (((sp.i & sp.xMask) !== pkSlot0) ? sp.x_inReal[sp.i & sp.xMask] : pkVal0) - shift;
    dev2 = dev * dev;
    total1 += dev;
    total2 += dev2;
    total3 += dev2 * dev;
    total4 += dev2 * dev2;
    if (total2 > peak2) {
      peak2 = total2;
    }
    if (total4 > peak4) {
      peak4 = total4;
    }
    residue = total1 * sp.invPeriod;
    residueSq = residue * residue;
    moment2 = total2 - sp.dPeriod * residueSq;
    let _fma0: number;
    {
      const _a: number = 6.0 * residueSq, _b: number = total2, _c: number = total4 - 4.0 * residue * total3;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      _fma0 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    moment4 = _fma0 - 3.0 * sp.dPeriod * residueSq * residueSq;
    /* Rebuild once either central moment falls below 1% of the largest
     * shifted sum of its power held since the last rebuild. That catches a
     * shift several sigma stale, where the expansion above cancels as
     * (u/sigma)^4, and a series decaying back onto the shift or an outlier
     * leaving the window, which leave current sums made mostly of rounding
     * that scales with the peaks.
     */
    barsSinceReseed -= 1;
    if (moment2 < 0.01 * peak2 || moment4 < 0.01 * peak4 || barsSinceReseed <= 0) {
      barsSinceReseed = sp.reseedPeriod;
      windowStart = sp.i - sp.nbInitialElementNeeded;
      tempReal = 0.0;
      for (j = windowStart; j <= sp.i; j += 1) {
        tempReal += ((j & sp.xMask) !== pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0;
      }
      shift = tempReal * sp.invPeriod;
      total1 = 0.0;
      total2 = 0.0;
      total3 = 0.0;
      total4 = 0.0;
      for (j = windowStart; j <= sp.i; j += 1) {
        dev = (((j & sp.xMask) !== pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0) - shift;
        dev2 = dev * dev;
        total1 += dev;
        total2 += dev2;
        total3 += dev2 * dev;
        total4 += dev2 * dev2;
      }
      residue = total1 * sp.invPeriod;
      residueSq = residue * residue;
      moment2 = total2 - sp.dPeriod * residueSq;
      let _fma1: number;
      {
        const _a: number = 6.0 * residueSq, _b: number = total2, _c: number = total4 - 4.0 * residue * total3;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        _fma1 = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      moment4 = _fma1 - 3.0 * sp.dPeriod * residueSq * residueSq;
      /* The left-to-right sum misses the mean by many ulps at a long
       * period, and a window whose spread is under that miss would fire a
       * trigger again on every bar. The residue just measured is the
       * miss: adding it back lands within about an ulp of the mean, where
       * a flat window's deviations are exactly 0.
       */
      if (moment2 < 0.01 * total2 || moment4 < 0.01 * total4) {
        shift += residue;
        total1 = 0.0;
        total2 = 0.0;
        total3 = 0.0;
        total4 = 0.0;
        for (j = windowStart; j <= sp.i; j += 1) {
          dev = (((j & sp.xMask) !== pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0) - shift;
          dev2 = dev * dev;
          total1 += dev;
          total2 += dev2;
          total3 += dev2 * dev;
          total4 += dev2 * dev2;
        }
        residue = total1 * sp.invPeriod;
        residueSq = residue * residue;
        moment2 = total2 - sp.dPeriod * residueSq;
        let _fma2: number;
        {
          const _a: number = 6.0 * residueSq, _b: number = total2, _c: number = total4 - 4.0 * residue * total3;
          const _p = _a * _b;
          let _t = 134217729 * _a;
          const _ah = _t - (_t - _a), _al = _a - _ah;
          _t = 134217729 * _b;
          const _bh = _t - (_t - _b), _bl = _b - _bh;
          const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
          const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
          const _v = _tl + _pl, _v2 = _v + _v;
          const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
          _fma2 = _w && (_pl === 0 || _v === 0) ? _th
            : _w && _v2 !== (_th + _v2) - _th ? _th + _v
            : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
        }
        moment4 = _fma2 - 3.0 * sp.dPeriod * residueSq * residueSq;
      }
      peak2 = total2;
      peak4 = total4;
    }
    /* No spread, no kurtosis: 0 would assert normality and -1.2 uniformity.
     * A flat window reaches here with every deviation exactly 0, so this is
     * 0/0 and IEEE answers NaN without a branch. A guard with an absolute
     * epsilon would be a cliff at a price level, not a noise floor (#243).
     */
    sampleVar = moment2 / (sp.dPeriod - 1.0);
    varSquared = sampleVar * sampleVar;
    kurt = sp.coefA * (moment4 / varSquared) - sp.coefB;
    /* inReal and outReal may be the same buffer: read the trailing value
     * before the output write.
     */
    dev = (((trailingIdx & sp.xMask) !== pkSlot0) ? sp.x_inReal[trailingIdx & sp.xMask] : pkVal0) - shift;
    dev2 = dev * dev;
    total1 -= dev;
    total2 -= dev2;
    total3 -= dev2 * dev;
    total4 -= dev2 * dev2;
    trailingIdx += 1;
    cur_outReal = kurt;
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
  clone(): KurtosisStream {
    const c = new KurtosisStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.shift = this.shift;
    c.total1 = this.total1;
    c.total2 = this.total2;
    c.total3 = this.total3;
    c.total4 = this.total4;
    c.dPeriod = this.dPeriod;
    c.coefA = this.coefA;
    c.coefB = this.coefB;
    c.invPeriod = this.invPeriod;
    c.peak2 = this.peak2;
    c.peak4 = this.peak4;
    c.trailingIdx = this.trailingIdx;
    c.nbInitialElementNeeded = this.nbInitialElementNeeded;
    c.barsSinceReseed = this.barsSinceReseed;
    c.reseedPeriod = this.reseedPeriod;
    c.j = this.j;
    c.i = this.i;
    c.xMask = this.xMask;
    c.x_inReal = this.x_inReal.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function kurtosisStepImpl(sp: KurtosisStream, inReal: number): void {
  let tempReal = 0;
  let dev = 0;
  let dev2 = 0;
  let residue = 0;
  let residueSq = 0;
  let moment2 = 0;
  let moment4 = 0;
  let sampleVar = 0;
  let varSquared = 0;
  let kurt = 0;
  let windowStart = 0;
  sp.x_inReal[sp.i & sp.xMask] = inReal;
  dev = sp.x_inReal[sp.i & sp.xMask] - sp.shift;
  dev2 = dev * dev;
  sp.total1 += dev;
  sp.total2 += dev2;
  sp.total3 += dev2 * dev;
  sp.total4 += dev2 * dev2;
  if (sp.total2 > sp.peak2) {
    sp.peak2 = sp.total2;
  }
  if (sp.total4 > sp.peak4) {
    sp.peak4 = sp.total4;
  }
  residue = sp.total1 * sp.invPeriod;
  residueSq = residue * residue;
  moment2 = sp.total2 - sp.dPeriod * residueSq;
  let _fma3: number;
  {
    const _a: number = 6.0 * residueSq, _b: number = sp.total2, _c: number = sp.total4 - 4.0 * residue * sp.total3;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    _fma3 = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  moment4 = _fma3 - 3.0 * sp.dPeriod * residueSq * residueSq;
  /* Rebuild once either central moment falls below 1% of the largest
   * shifted sum of its power held since the last rebuild. That catches a
   * shift several sigma stale, where the expansion above cancels as
   * (u/sigma)^4, and a series decaying back onto the shift or an outlier
   * leaving the window, which leave current sums made mostly of rounding
   * that scales with the peaks.
   */
  sp.barsSinceReseed -= 1;
  if (moment2 < 0.01 * sp.peak2 || moment4 < 0.01 * sp.peak4 || sp.barsSinceReseed <= 0) {
    sp.barsSinceReseed = sp.reseedPeriod;
    windowStart = sp.i - sp.nbInitialElementNeeded;
    tempReal = 0.0;
    for (sp.j = windowStart; sp.j <= sp.i; sp.j += 1) {
      tempReal += sp.x_inReal[sp.j & sp.xMask];
    }
    sp.shift = tempReal * sp.invPeriod;
    sp.total1 = 0.0;
    sp.total2 = 0.0;
    sp.total3 = 0.0;
    sp.total4 = 0.0;
    for (sp.j = windowStart; sp.j <= sp.i; sp.j += 1) {
      dev = sp.x_inReal[sp.j & sp.xMask] - sp.shift;
      dev2 = dev * dev;
      sp.total1 += dev;
      sp.total2 += dev2;
      sp.total3 += dev2 * dev;
      sp.total4 += dev2 * dev2;
    }
    residue = sp.total1 * sp.invPeriod;
    residueSq = residue * residue;
    moment2 = sp.total2 - sp.dPeriod * residueSq;
    let _fma4: number;
    {
      const _a: number = 6.0 * residueSq, _b: number = sp.total2, _c: number = sp.total4 - 4.0 * residue * sp.total3;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      _fma4 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    moment4 = _fma4 - 3.0 * sp.dPeriod * residueSq * residueSq;
    /* The left-to-right sum misses the mean by many ulps at a long
     * period, and a window whose spread is under that miss would fire a
     * trigger again on every bar. The residue just measured is the
     * miss: adding it back lands within about an ulp of the mean, where
     * a flat window's deviations are exactly 0.
     */
    if (moment2 < 0.01 * sp.total2 || moment4 < 0.01 * sp.total4) {
      sp.shift += residue;
      sp.total1 = 0.0;
      sp.total2 = 0.0;
      sp.total3 = 0.0;
      sp.total4 = 0.0;
      for (sp.j = windowStart; sp.j <= sp.i; sp.j += 1) {
        dev = sp.x_inReal[sp.j & sp.xMask] - sp.shift;
        dev2 = dev * dev;
        sp.total1 += dev;
        sp.total2 += dev2;
        sp.total3 += dev2 * dev;
        sp.total4 += dev2 * dev2;
      }
      residue = sp.total1 * sp.invPeriod;
      residueSq = residue * residue;
      moment2 = sp.total2 - sp.dPeriod * residueSq;
      let _fma5: number;
      {
        const _a: number = 6.0 * residueSq, _b: number = sp.total2, _c: number = sp.total4 - 4.0 * residue * sp.total3;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        _fma5 = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      moment4 = _fma5 - 3.0 * sp.dPeriod * residueSq * residueSq;
    }
    sp.peak2 = sp.total2;
    sp.peak4 = sp.total4;
  }
  /* No spread, no kurtosis: 0 would assert normality and -1.2 uniformity.
   * A flat window reaches here with every deviation exactly 0, so this is
   * 0/0 and IEEE answers NaN without a branch. A guard with an absolute
   * epsilon would be a cliff at a price level, not a noise floor (#243).
   */
  sampleVar = moment2 / (sp.dPeriod - 1.0);
  varSquared = sampleVar * sampleVar;
  kurt = sp.coefA * (moment4 / varSquared) - sp.coefB;
  /* inReal and outReal may be the same buffer: read the trailing value
   * before the output write.
   */
  dev = sp.x_inReal[sp.trailingIdx & sp.xMask] - sp.shift;
  dev2 = dev * dev;
  sp.total1 -= dev;
  sp.total2 -= dev2;
  sp.total3 -= dev2 * dev;
  sp.total4 -= dev2 * dev2;
  sp.trailingIdx += 1;
  sp.cur_outReal = kurt;
  sp.i += 1;
}

function kurtosisOpenImpl(
  sp: KurtosisStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let tempReal = 0;
  let shift = 0;
  let total1 = 0;
  let total2 = 0;
  let total3 = 0;
  let total4 = 0;
  let dev = 0;
  let dev2 = 0;
  let residue = 0;
  let residueSq = 0;
  let moment2 = 0;
  let moment4 = 0;
  let sampleVar = 0;
  let varSquared = 0;
  let dPeriod = 0;
  let coefA = 0;
  let coefB = 0;
  let invPeriod = 0;
  let kurt = 0;
  let peak2 = 0;
  let peak4 = 0;
  let i = 0;
  let j = 0;
  let outIdx = 0;
  let trailingIdx = 0;
  let windowStart = 0;
  let nbInitialElementNeeded = 0;
  let barsSinceReseed = 0;
  let reseedPeriod = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 4 || optInTimePeriod > 10000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  nbInitialElementNeeded = optInTimePeriod - 1;
  if (startIdx < nbInitialElementNeeded) {
    startIdx = nbInitialElementNeeded;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* G2, the sample-adjusted Fisher excess kurtosis. The coefficients are
   * computed in double: at the top of the parameter range coefA's denominator
   * (n-1)(n-2)(n-3) is ~1e12, far past what an int holds.
   *
   * The (n-2)(n-3) denominators are why the range starts at 4 rather than 1.
   * The argument contract rejects anything below it, and the n-1 lookback
   * guarantees a full window at the first emitted bar, so the estimator is
   * never handed fewer than four points and needs no runtime branch for it.
   */
  dPeriod = optInTimePeriod;
  coefA = dPeriod * (dPeriod + 1.0) / ((dPeriod - 1.0) * (dPeriod - 2.0) * (dPeriod - 3.0));
  coefB = 3.0 * (dPeriod - 1.0) * (dPeriod - 1.0) / ((dPeriod - 2.0) * (dPeriod - 3.0));
  invPeriod = 1.0 / dPeriod;
  /* Deviations are measured against a shift near the window, so the running
   * sums stay at deviation scale instead of at price scale, and the central
   * moments are recovered from them by the binomial expansion below. The
   * period bounds the rounding the slide accumulates and is the only way out
   * of a NaN or Inf input, on which every trigger comparison is false; a stale
   * shift is caught by the triggers in the loop, not by this.
   */
  reseedPeriod = ((32 * optInTimePeriod) | 0);
  trailingIdx = startIdx - nbInitialElementNeeded;
  shift = inReal[trailingIdx];
  total1 = 0.0;
  total2 = 0.0;
  total3 = 0.0;
  total4 = 0.0;
  for (j = trailingIdx; j < startIdx; j += 1) {
    dev = inReal[j] - shift;
    dev2 = dev * dev;
    total1 += dev;
    total2 += dev2;
    total3 += dev2 * dev;
    total4 += dev2 * dev2;
  }
  i = startIdx;
  outIdx = 0;
  barsSinceReseed = reseedPeriod;
  peak2 = total2;
  peak4 = total4;
  do {
    dev = inReal[i] - shift;
    dev2 = dev * dev;
    total1 += dev;
    total2 += dev2;
    total3 += dev2 * dev;
    total4 += dev2 * dev2;
    if (total2 > peak2) {
      peak2 = total2;
    }
    if (total4 > peak4) {
      peak4 = total4;
    }
    residue = total1 * invPeriod;
    residueSq = residue * residue;
    moment2 = total2 - dPeriod * residueSq;
    let _fma6: number;
    {
      const _a: number = 6.0 * residueSq, _b: number = total2, _c: number = total4 - 4.0 * residue * total3;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      _fma6 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    moment4 = _fma6 - 3.0 * dPeriod * residueSq * residueSq;
    /* Rebuild once either central moment falls below 1% of the largest
     * shifted sum of its power held since the last rebuild. That catches a
     * shift several sigma stale, where the expansion above cancels as
     * (u/sigma)^4, and a series decaying back onto the shift or an outlier
     * leaving the window, which leave current sums made mostly of rounding
     * that scales with the peaks.
     */
    barsSinceReseed -= 1;
    if (moment2 < 0.01 * peak2 || moment4 < 0.01 * peak4 || barsSinceReseed <= 0) {
      barsSinceReseed = reseedPeriod;
      windowStart = i - nbInitialElementNeeded;
      tempReal = 0.0;
      for (j = windowStart; j <= i; j += 1) {
        tempReal += inReal[j];
      }
      shift = tempReal * invPeriod;
      total1 = 0.0;
      total2 = 0.0;
      total3 = 0.0;
      total4 = 0.0;
      for (j = windowStart; j <= i; j += 1) {
        dev = inReal[j] - shift;
        dev2 = dev * dev;
        total1 += dev;
        total2 += dev2;
        total3 += dev2 * dev;
        total4 += dev2 * dev2;
      }
      residue = total1 * invPeriod;
      residueSq = residue * residue;
      moment2 = total2 - dPeriod * residueSq;
      let _fma7: number;
      {
        const _a: number = 6.0 * residueSq, _b: number = total2, _c: number = total4 - 4.0 * residue * total3;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        _fma7 = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      moment4 = _fma7 - 3.0 * dPeriod * residueSq * residueSq;
      /* The left-to-right sum misses the mean by many ulps at a long
       * period, and a window whose spread is under that miss would fire a
       * trigger again on every bar. The residue just measured is the
       * miss: adding it back lands within about an ulp of the mean, where
       * a flat window's deviations are exactly 0.
       */
      if (moment2 < 0.01 * total2 || moment4 < 0.01 * total4) {
        shift += residue;
        total1 = 0.0;
        total2 = 0.0;
        total3 = 0.0;
        total4 = 0.0;
        for (j = windowStart; j <= i; j += 1) {
          dev = inReal[j] - shift;
          dev2 = dev * dev;
          total1 += dev;
          total2 += dev2;
          total3 += dev2 * dev;
          total4 += dev2 * dev2;
        }
        residue = total1 * invPeriod;
        residueSq = residue * residue;
        moment2 = total2 - dPeriod * residueSq;
        let _fma8: number;
        {
          const _a: number = 6.0 * residueSq, _b: number = total2, _c: number = total4 - 4.0 * residue * total3;
          const _p = _a * _b;
          let _t = 134217729 * _a;
          const _ah = _t - (_t - _a), _al = _a - _ah;
          _t = 134217729 * _b;
          const _bh = _t - (_t - _b), _bl = _b - _bh;
          const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
          const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
          const _v = _tl + _pl, _v2 = _v + _v;
          const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
          _fma8 = _w && (_pl === 0 || _v === 0) ? _th
            : _w && _v2 !== (_th + _v2) - _th ? _th + _v
            : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
        }
        moment4 = _fma8 - 3.0 * dPeriod * residueSq * residueSq;
      }
      peak2 = total2;
      peak4 = total4;
    }
    /* No spread, no kurtosis: 0 would assert normality and -1.2 uniformity.
     * A flat window reaches here with every deviation exactly 0, so this is
     * 0/0 and IEEE answers NaN without a branch. A guard with an absolute
     * epsilon would be a cliff at a price level, not a noise floor (#243).
     */
    sampleVar = moment2 / (dPeriod - 1.0);
    varSquared = sampleVar * sampleVar;
    kurt = coefA * (moment4 / varSquared) - coefB;
    /* inReal and outReal may be the same buffer: read the trailing value
     * before the output write.
     */
    dev = inReal[trailingIdx] - shift;
    dev2 = dev * dev;
    total1 -= dev;
    total2 -= dev2;
    total3 -= dev2 * dev;
    total4 -= dev2 * dev2;
    trailingIdx += 1;
    outReal[((outIdx++ * outStride) | 0)] = kurt;
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
  sp.shift = shift;
  sp.total1 = total1;
  sp.total2 = total2;
  sp.total3 = total3;
  sp.total4 = total4;
  sp.dPeriod = dPeriod;
  sp.coefA = coefA;
  sp.coefB = coefB;
  sp.invPeriod = invPeriod;
  sp.peak2 = peak2;
  sp.peak4 = peak4;
  sp.trailingIdx = trailingIdx;
  sp.nbInitialElementNeeded = nbInitialElementNeeded;
  sp.barsSinceReseed = barsSinceReseed;
  sp.reseedPeriod = reseedPeriod;
  sp.j = j;
  sp.i = i;
  sp.xMask = physX - 1;
  sp.x_inReal = capX_inReal;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function kurtosisOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): KurtosisStream {
  const sp = KurtosisStream.of(core);
  const retCode = kurtosisOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('KURTOSIS openAndFill', inReal.length, startIdx, kurtosisLookback(optInTimePeriod, core));
  }
  throw streamFailure('KURTOSIS openAndFill', retCode);
}

/** @internal */
export function kurtosisOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): KurtosisStream {
  const sp = KurtosisStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = kurtosisOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('KURTOSIS open', inReal.length, startIdx, kurtosisLookback(optInTimePeriod, core));
  }
  throw streamFailure('KURTOSIS open', retCode);
}

/**
 * Opens a live KURTOSIS stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link kurtosis} there.
 *
 * @param inReal - Warm-up history. The series to measure.
 * @param optInTimePeriod - Number of trailing values in the window (default 30;
 *   range 4 to 10000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `kurtosisLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { kurtosisOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = kurtosisOpen(data, 30);
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function kurtosisOpen(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): KurtosisStream {
  optInTimePeriod = requireInt32('KURTOSIS open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('KURTOSIS open', 'inReal', inReal);
  requireHistory('KURTOSIS open', inReal.length);
  return kurtosisOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/**
 * {@link kurtosisOpen} that also fills the outputs over the whole history in the
 * same pass, bit-identically to {@link kurtosis}. The range written is the
 * stream's `outRange()`.
 *
 * @param inReal - Warm-up history. The series to measure.
 * @param optInTimePeriod - Number of trailing values in the window (default 30;
 *   range 4 to 10000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Excess kurtosis of the trailing window, or NaN where the window
 *   has no spread. Must hold `historyLen - lookback` values and share no memory with
 *   an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `kurtosisLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for an integer parameter that is not an int32, a parameter outside its range or
 *   an output too short or starting where another output or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { kurtosisOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const stream = kurtosisOpenAndFill(data, 30, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function kurtosisOpenAndFill(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): KurtosisStream {
  optInTimePeriod = requireInt32('KURTOSIS openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('KURTOSIS openAndFill', 'inReal', inReal);
  requireHistory('KURTOSIS openAndFill', inReal.length);
  const guardOutLen = openFillCount('KURTOSIS openAndFill', inReal.length, kurtosisLookback(optInTimePeriod, core));
  requireOutput('KURTOSIS openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if ((outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset)) {
    throw streamFailure('KURTOSIS openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inRealF64))) {
    throw streamFailure('KURTOSIS openAndFill', RetCode.BAD_PARAM);
  }
  return kurtosisOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}
