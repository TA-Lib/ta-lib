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
 *  110126 MF,CC  First version (issue #569).
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function skewLookbackPositional(optInTimePeriod: number, _core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 3 || optInTimePeriod > 100000) {
    return -1;
  }
  return optInTimePeriod - 1;
}

/** @internal */
export function skewDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (skewLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function skewImpl(
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
  let dev = 0;
  let dev2 = 0;
  let residue = 0;
  let residueSq = 0;
  let moment2 = 0;
  let moment3 = 0;
  let dPeriod = 0;
  let coef = 0;
  let invPeriod = 0;
  let skewness = 0;
  let peak2 = 0;
  let denominator = 0;
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
  } else if (optInTimePeriod < 3 || optInTimePeriod > 100000) {
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
  /* G1 = n*sqrt(n-1)/(n-2) * M3 / M2^(3/2), over the central sums M2 and
   * M3. The n-2 is why the range starts at 3, and the n-1 lookback hands the
   * estimator a full window at the first emitted bar.
   */
  dPeriod = optInTimePeriod;
  coef = dPeriod * Math.sqrt(dPeriod - 1.0) / (dPeriod - 2.0);
  invPeriod = 1.0 / dPeriod;
  /* Deviations are measured against a shift near the window, and the
   * central sums are recovered from them below. The periodic rebuild bounds
   * the rounding the slide accumulates, and is the only way out of a NaN or
   * Inf input, on which every trigger comparison is false.
   */
  reseedPeriod = ((32 * optInTimePeriod) | 0);
  trailingIdx = startIdx - nbInitialElementNeeded;
  shift = inReal[trailingIdx];
  total1 = 0.0;
  total2 = 0.0;
  total3 = 0.0;
  for (j = trailingIdx; j < startIdx; j += 1) {
    dev = inReal[j] - shift;
    dev2 = dev * dev;
    total1 += dev;
    total2 += dev2;
    total3 += dev2 * dev;
  }
  i = startIdx;
  outIdx = 0;
  barsSinceReseed = reseedPeriod;
  peak2 = total2;
  do {
    dev = inReal[i] - shift;
    dev2 = dev * dev;
    total1 += dev;
    total2 += dev2;
    total3 += dev2 * dev;
    if (total2 > peak2) {
      peak2 = total2;
    }
    residue = total1 * invPeriod;
    residueSq = residue * residue;
    moment2 = total2 - dPeriod * residueSq;
    {
      const _a: number = 2.0 * dPeriod * residueSq, _b: number = residue, _c: number = total3 - 3.0 * residue * total2;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      moment3 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    /* Each bar rounds the cubic sum by at most eps times the peak second
     * sum to the power 3/2, so G1's error grows as (peak/M2)^(3/2), at most
     * 1000x before the rebuild at 1% of the peak.
     */
    barsSinceReseed -= 1;
    if (moment2 < 0.01 * peak2 || barsSinceReseed <= 0) {
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
      for (j = windowStart; j <= i; j += 1) {
        dev = inReal[j] - shift;
        dev2 = dev * dev;
        total1 += dev;
        total2 += dev2;
        total3 += dev2 * dev;
      }
      residue = total1 * invPeriod;
      residueSq = residue * residue;
      moment2 = total2 - dPeriod * residueSq;
      {
        const _a: number = 2.0 * dPeriod * residueSq, _b: number = residue, _c: number = total3 - 3.0 * residue * total2;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        moment3 = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      /* The left-to-right sum can miss the mean by more than the window's
       * spread, and the trigger would then fire again on every bar. Adding
       * the measured miss back lands within about an ulp of the mean, where
       * a flat window's deviations are exactly 0. Keep this threshold the
       * trigger's.
       */
      if (moment2 < 0.01 * total2) {
        shift += residue;
        total1 = 0.0;
        total2 = 0.0;
        total3 = 0.0;
        for (j = windowStart; j <= i; j += 1) {
          dev = inReal[j] - shift;
          dev2 = dev * dev;
          total1 += dev;
          total2 += dev2;
          total3 += dev2 * dev;
        }
        residue = total1 * invPeriod;
        residueSq = residue * residue;
        moment2 = total2 - dPeriod * residueSq;
        {
          const _a: number = 2.0 * dPeriod * residueSq, _b: number = residue, _c: number = total3 - 3.0 * residue * total2;
          const _p = _a * _b;
          let _t = 134217729 * _a;
          const _ah = _t - (_t - _a), _al = _a - _ah;
          _t = 134217729 * _b;
          const _bh = _t - (_t - _b), _bl = _b - _bh;
          const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
          const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
          const _v = _tl + _pl, _v2 = _v + _v;
          const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
          moment3 = _w && (_pl === 0 || _v === 0) ? _th
            : _w && _v2 !== (_th + _v2) - _th ? _th + _v
            : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
        }
      }
      peak2 = total2;
    }
    /* A window of equal values is symmetric, and reaches here with every
     * deviation exactly 0. The bound is the smallest normal double, not a
     * scale: a denominator under it comes with cubed deviations that have
     * lost their precision in the subnormals.
     */
    denominator = moment2 * Math.sqrt(moment2);
    if (denominator < 2.2250738585072014e-308) {
      skewness = 0.0;
    } else {
      skewness = coef * moment3 / denominator;
    }
    /* inReal and outReal may be the same buffer: read the trailing value
     * before the output write.
     */
    dev = inReal[trailingIdx] - shift;
    dev2 = dev * dev;
    total1 -= dev;
    total2 -= dev2;
    total3 -= dev2 * dev;
    trailingIdx += 1;
    outReal[outIdx++] = skewness;
    i += 1;
  } while (i <= endIdx);
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 3, 100000, 'in 3..100000']] as const;

/** @internal */
export function skewPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('SKEW', 'startIdx', startIdx);
  endIdx = requireInt32('SKEW', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('SKEW', 'timePeriod', optInTimePeriod);
  requireIndexRange('SKEW', startIdx, endIdx);
  const guardLookback = skewLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('SKEW', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('SKEW', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('SKEW', 'real', inReal, guardInLen);
  requireOutput('SKEW', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('SKEW', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = skewImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('SKEW', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link skew}, {@link skewLookback}, {@link skewDisplayShift},
 * {@link skewOpen} and {@link skewOpenAndFill}. Every key is optional: an omitted
 * or `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface SkewOptions {
  /**
   * Number of trailing values in the window (default 30; range 3 to 100000;
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

/** What {@link skew} returns, and {@link skewOpenAndFill} with its stream. */
export interface SkewResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Skewness of the trailing window, or 0 where the window has no spread.
   * `values[k]` is the value at input bar `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link skew} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SkewOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function skewLookback(options?: SkewOptions): number {
  const o = optionsOf('SKEW lookback', options);
  return skewLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('SKEW lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link skew}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SkewOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function skewDisplayShift(outputIdx: number, options?: SkewOptions): number {
  const o = optionsOf('SKEW displayShift', options);
  return skewDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('SKEW displayShift', o.core));
}

/**
 * An estimate, from the trailing window, of the skewness of the distribution its
 * values come from: the third standardised moment. An asymmetry measure: above 0
 * the distribution has a longer tail above its mean, below 0 a longer tail below
 * it, and a symmetric window reads 0. The companion to
 * [`KURTOSIS`](https://ta-lib.org/functions/kurtosis),
 * [`VAR`](https://ta-lib.org/functions/var) and
 * [`STDDEV`](https://ta-lib.org/functions/stddev) in the same group.
 *
 * Formula and more info at
 * [ta-lib.org/functions/skew](https://ta-lib.org/functions/skew).
 *
 * **Notes**
 * - This is `G1`, the form Excel `SKEW`, `scipy.stats.skew(bias=False)` and R
 *   `e1071::skewness(type = 2)` compute. Two other estimators are in circulation,
 *   each a constant multiple of `g1` rather than a rounding convention: `g1`
 *   itself (`scipy.stats.skew(bias=True)`) and `b1 = g1 · ((n−1)/n)^(3/2)`
 *   (`type = 3`).
 * - One reading apart from `n−1` equal ones gives exactly `√n`, above the rest
 *   when the odd one is above and `−√n` when it is below. That is the largest
 *   magnitude a window can reach, so a window dominated by one outlier is
 *   legitimately far from 0.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link skewLookback} is a success with empty outputs.
 *
 * @param real - The series to measure.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link SkewOptions}.
 * @returns `begIdx` and the values; see {@link SkewResult}. Each output is a view
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
 * import { skew } from 'ta-lib';
 *
 * const series = new Float64Array(252); // your data, oldest first
 *
 * const { begIdx, values } = skew(series, { timePeriod: 30 });
 * console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
 * ```
 * @see [`kurtosis`](https://ta-lib.org/functions/kurtosis)
 * @see [`var_`](https://ta-lib.org/functions/var)
 * @see [`stddev`](https://ta-lib.org/functions/stddev)
 */
export function skew(real: ArrayLike<number>, options?: SkewOptions): SkewResult {
  const o = optionsOf('SKEW', options);
  if (o.endIdx === undefined) {
    requireSeries('SKEW', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('SKEW', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, skewLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = skewPositional(startIdx, endIdx, real, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live SKEW stream: one value per closed bar, bit-identical to {@link skew} over
 * the same series. Open it with {@link skewOpen} or {@link skewOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/skew](https://ta-lib.org/functions/skew).
 */
export class SkewStream {
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
  dPeriod: number = 0;
  /** @internal */
  coef: number = 0;
  /** @internal */
  invPeriod: number = 0;
  /** @internal */
  peak2: number = 0;
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
  static of(core: Core): SkewStream {
    return new SkewStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link skew} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('SKEW advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: The series to measure.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('SKEW update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('SKEW update', 'real');
    }
    skewStepImpl(this, real);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: The series to measure.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number): number {
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('SKEW peek', 'real');
    }
    const sp = this;
    const inReal = real;
    let tempReal = 0;
    let dev = 0;
    let dev2 = 0;
    let residue = 0;
    let residueSq = 0;
    let moment2 = 0;
    let moment3 = 0;
    let skewness = 0;
    let denominator = 0;
    let windowStart = 0;
    let barsSinceReseed = sp.barsSinceReseed;
    let cur_outReal = 0;
    let j = sp.j;
    let peak2 = sp.peak2;
    let shift = sp.shift;
    let total1 = sp.total1;
    let total2 = sp.total2;
    let total3 = sp.total3;
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
    if (total2 > peak2) {
      peak2 = total2;
    }
    residue = total1 * sp.invPeriod;
    residueSq = residue * residue;
    moment2 = total2 - sp.dPeriod * residueSq;
    {
      const _a: number = 2.0 * sp.dPeriod * residueSq, _b: number = residue, _c: number = total3 - 3.0 * residue * total2;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      moment3 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    /* Each bar rounds the cubic sum by at most eps times the peak second
     * sum to the power 3/2, so G1's error grows as (peak/M2)^(3/2), at most
     * 1000x before the rebuild at 1% of the peak.
     */
    barsSinceReseed -= 1;
    if (moment2 < 0.01 * peak2 || barsSinceReseed <= 0) {
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
      for (j = windowStart; j <= sp.i; j += 1) {
        dev = (((j & sp.xMask) !== pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0) - shift;
        dev2 = dev * dev;
        total1 += dev;
        total2 += dev2;
        total3 += dev2 * dev;
      }
      residue = total1 * sp.invPeriod;
      residueSq = residue * residue;
      moment2 = total2 - sp.dPeriod * residueSq;
      {
        const _a: number = 2.0 * sp.dPeriod * residueSq, _b: number = residue, _c: number = total3 - 3.0 * residue * total2;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        moment3 = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      /* The left-to-right sum can miss the mean by more than the window's
       * spread, and the trigger would then fire again on every bar. Adding
       * the measured miss back lands within about an ulp of the mean, where
       * a flat window's deviations are exactly 0. Keep this threshold the
       * trigger's.
       */
      if (moment2 < 0.01 * total2) {
        shift += residue;
        total1 = 0.0;
        total2 = 0.0;
        total3 = 0.0;
        for (j = windowStart; j <= sp.i; j += 1) {
          dev = (((j & sp.xMask) !== pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0) - shift;
          dev2 = dev * dev;
          total1 += dev;
          total2 += dev2;
          total3 += dev2 * dev;
        }
        residue = total1 * sp.invPeriod;
        residueSq = residue * residue;
        moment2 = total2 - sp.dPeriod * residueSq;
        {
          const _a: number = 2.0 * sp.dPeriod * residueSq, _b: number = residue, _c: number = total3 - 3.0 * residue * total2;
          const _p = _a * _b;
          let _t = 134217729 * _a;
          const _ah = _t - (_t - _a), _al = _a - _ah;
          _t = 134217729 * _b;
          const _bh = _t - (_t - _b), _bl = _b - _bh;
          const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
          const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
          const _v = _tl + _pl, _v2 = _v + _v;
          const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
          moment3 = _w && (_pl === 0 || _v === 0) ? _th
            : _w && _v2 !== (_th + _v2) - _th ? _th + _v
            : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
        }
      }
      peak2 = total2;
    }
    /* A window of equal values is symmetric, and reaches here with every
     * deviation exactly 0. The bound is the smallest normal double, not a
     * scale: a denominator under it comes with cubed deviations that have
     * lost their precision in the subnormals.
     */
    denominator = moment2 * Math.sqrt(moment2);
    if (denominator < 2.2250738585072014e-308) {
      skewness = 0.0;
    } else {
      skewness = sp.coef * moment3 / denominator;
    }
    /* inReal and outReal may be the same buffer: read the trailing value
     * before the output write.
     */
    dev = (((trailingIdx & sp.xMask) !== pkSlot0) ? sp.x_inReal[trailingIdx & sp.xMask] : pkVal0) - shift;
    dev2 = dev * dev;
    total1 -= dev;
    total2 -= dev2;
    total3 -= dev2 * dev;
    trailingIdx += 1;
    cur_outReal = skewness;
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
  clone(): SkewStream {
    const c = new SkewStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.shift = this.shift;
    c.total1 = this.total1;
    c.total2 = this.total2;
    c.total3 = this.total3;
    c.dPeriod = this.dPeriod;
    c.coef = this.coef;
    c.invPeriod = this.invPeriod;
    c.peak2 = this.peak2;
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

function skewStepImpl(sp: SkewStream, inReal: number): void {
  let tempReal = 0;
  let dev = 0;
  let dev2 = 0;
  let residue = 0;
  let residueSq = 0;
  let moment2 = 0;
  let moment3 = 0;
  let skewness = 0;
  let denominator = 0;
  let windowStart = 0;
  sp.x_inReal[sp.i & sp.xMask] = inReal;
  dev = sp.x_inReal[sp.i & sp.xMask] - sp.shift;
  dev2 = dev * dev;
  sp.total1 += dev;
  sp.total2 += dev2;
  sp.total3 += dev2 * dev;
  if (sp.total2 > sp.peak2) {
    sp.peak2 = sp.total2;
  }
  residue = sp.total1 * sp.invPeriod;
  residueSq = residue * residue;
  moment2 = sp.total2 - sp.dPeriod * residueSq;
  {
    const _a: number = 2.0 * sp.dPeriod * residueSq, _b: number = residue, _c: number = sp.total3 - 3.0 * residue * sp.total2;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    moment3 = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  /* Each bar rounds the cubic sum by at most eps times the peak second
   * sum to the power 3/2, so G1's error grows as (peak/M2)^(3/2), at most
   * 1000x before the rebuild at 1% of the peak.
   */
  sp.barsSinceReseed -= 1;
  if (moment2 < 0.01 * sp.peak2 || sp.barsSinceReseed <= 0) {
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
    for (sp.j = windowStart; sp.j <= sp.i; sp.j += 1) {
      dev = sp.x_inReal[sp.j & sp.xMask] - sp.shift;
      dev2 = dev * dev;
      sp.total1 += dev;
      sp.total2 += dev2;
      sp.total3 += dev2 * dev;
    }
    residue = sp.total1 * sp.invPeriod;
    residueSq = residue * residue;
    moment2 = sp.total2 - sp.dPeriod * residueSq;
    {
      const _a: number = 2.0 * sp.dPeriod * residueSq, _b: number = residue, _c: number = sp.total3 - 3.0 * residue * sp.total2;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      moment3 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    /* The left-to-right sum can miss the mean by more than the window's
     * spread, and the trigger would then fire again on every bar. Adding
     * the measured miss back lands within about an ulp of the mean, where
     * a flat window's deviations are exactly 0. Keep this threshold the
     * trigger's.
     */
    if (moment2 < 0.01 * sp.total2) {
      sp.shift += residue;
      sp.total1 = 0.0;
      sp.total2 = 0.0;
      sp.total3 = 0.0;
      for (sp.j = windowStart; sp.j <= sp.i; sp.j += 1) {
        dev = sp.x_inReal[sp.j & sp.xMask] - sp.shift;
        dev2 = dev * dev;
        sp.total1 += dev;
        sp.total2 += dev2;
        sp.total3 += dev2 * dev;
      }
      residue = sp.total1 * sp.invPeriod;
      residueSq = residue * residue;
      moment2 = sp.total2 - sp.dPeriod * residueSq;
      {
        const _a: number = 2.0 * sp.dPeriod * residueSq, _b: number = residue, _c: number = sp.total3 - 3.0 * residue * sp.total2;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        moment3 = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
    }
    sp.peak2 = sp.total2;
  }
  /* A window of equal values is symmetric, and reaches here with every
   * deviation exactly 0. The bound is the smallest normal double, not a
   * scale: a denominator under it comes with cubed deviations that have
   * lost their precision in the subnormals.
   */
  denominator = moment2 * Math.sqrt(moment2);
  if (denominator < 2.2250738585072014e-308) {
    skewness = 0.0;
  } else {
    skewness = sp.coef * moment3 / denominator;
  }
  /* inReal and outReal may be the same buffer: read the trailing value
   * before the output write.
   */
  dev = sp.x_inReal[sp.trailingIdx & sp.xMask] - sp.shift;
  dev2 = dev * dev;
  sp.total1 -= dev;
  sp.total2 -= dev2;
  sp.total3 -= dev2 * dev;
  sp.trailingIdx += 1;
  sp.cur_outReal = skewness;
  sp.i += 1;
}

function skewOpenImpl(
  sp: SkewStream,
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
  let dev = 0;
  let dev2 = 0;
  let residue = 0;
  let residueSq = 0;
  let moment2 = 0;
  let moment3 = 0;
  let dPeriod = 0;
  let coef = 0;
  let invPeriod = 0;
  let skewness = 0;
  let peak2 = 0;
  let denominator = 0;
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
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 3 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
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
  /* G1 = n*sqrt(n-1)/(n-2) * M3 / M2^(3/2), over the central sums M2 and
   * M3. The n-2 is why the range starts at 3, and the n-1 lookback hands the
   * estimator a full window at the first emitted bar.
   */
  dPeriod = optInTimePeriod;
  coef = dPeriod * Math.sqrt(dPeriod - 1.0) / (dPeriod - 2.0);
  invPeriod = 1.0 / dPeriod;
  /* Deviations are measured against a shift near the window, and the
   * central sums are recovered from them below. The periodic rebuild bounds
   * the rounding the slide accumulates, and is the only way out of a NaN or
   * Inf input, on which every trigger comparison is false.
   */
  reseedPeriod = ((32 * optInTimePeriod) | 0);
  trailingIdx = startIdx - nbInitialElementNeeded;
  shift = inReal[trailingIdx];
  total1 = 0.0;
  total2 = 0.0;
  total3 = 0.0;
  for (j = trailingIdx; j < startIdx; j += 1) {
    dev = inReal[j] - shift;
    dev2 = dev * dev;
    total1 += dev;
    total2 += dev2;
    total3 += dev2 * dev;
  }
  i = startIdx;
  outIdx = 0;
  barsSinceReseed = reseedPeriod;
  peak2 = total2;
  do {
    dev = inReal[i] - shift;
    dev2 = dev * dev;
    total1 += dev;
    total2 += dev2;
    total3 += dev2 * dev;
    if (total2 > peak2) {
      peak2 = total2;
    }
    residue = total1 * invPeriod;
    residueSq = residue * residue;
    moment2 = total2 - dPeriod * residueSq;
    {
      const _a: number = 2.0 * dPeriod * residueSq, _b: number = residue, _c: number = total3 - 3.0 * residue * total2;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      moment3 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    /* Each bar rounds the cubic sum by at most eps times the peak second
     * sum to the power 3/2, so G1's error grows as (peak/M2)^(3/2), at most
     * 1000x before the rebuild at 1% of the peak.
     */
    barsSinceReseed -= 1;
    if (moment2 < 0.01 * peak2 || barsSinceReseed <= 0) {
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
      for (j = windowStart; j <= i; j += 1) {
        dev = inReal[j] - shift;
        dev2 = dev * dev;
        total1 += dev;
        total2 += dev2;
        total3 += dev2 * dev;
      }
      residue = total1 * invPeriod;
      residueSq = residue * residue;
      moment2 = total2 - dPeriod * residueSq;
      {
        const _a: number = 2.0 * dPeriod * residueSq, _b: number = residue, _c: number = total3 - 3.0 * residue * total2;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        moment3 = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      /* The left-to-right sum can miss the mean by more than the window's
       * spread, and the trigger would then fire again on every bar. Adding
       * the measured miss back lands within about an ulp of the mean, where
       * a flat window's deviations are exactly 0. Keep this threshold the
       * trigger's.
       */
      if (moment2 < 0.01 * total2) {
        shift += residue;
        total1 = 0.0;
        total2 = 0.0;
        total3 = 0.0;
        for (j = windowStart; j <= i; j += 1) {
          dev = inReal[j] - shift;
          dev2 = dev * dev;
          total1 += dev;
          total2 += dev2;
          total3 += dev2 * dev;
        }
        residue = total1 * invPeriod;
        residueSq = residue * residue;
        moment2 = total2 - dPeriod * residueSq;
        {
          const _a: number = 2.0 * dPeriod * residueSq, _b: number = residue, _c: number = total3 - 3.0 * residue * total2;
          const _p = _a * _b;
          let _t = 134217729 * _a;
          const _ah = _t - (_t - _a), _al = _a - _ah;
          _t = 134217729 * _b;
          const _bh = _t - (_t - _b), _bl = _b - _bh;
          const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
          const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
          const _v = _tl + _pl, _v2 = _v + _v;
          const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
          moment3 = _w && (_pl === 0 || _v === 0) ? _th
            : _w && _v2 !== (_th + _v2) - _th ? _th + _v
            : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
        }
      }
      peak2 = total2;
    }
    /* A window of equal values is symmetric, and reaches here with every
     * deviation exactly 0. The bound is the smallest normal double, not a
     * scale: a denominator under it comes with cubed deviations that have
     * lost their precision in the subnormals.
     */
    denominator = moment2 * Math.sqrt(moment2);
    if (denominator < 2.2250738585072014e-308) {
      skewness = 0.0;
    } else {
      skewness = coef * moment3 / denominator;
    }
    /* inReal and outReal may be the same buffer: read the trailing value
     * before the output write.
     */
    dev = inReal[trailingIdx] - shift;
    dev2 = dev * dev;
    total1 -= dev;
    total2 -= dev2;
    total3 -= dev2 * dev;
    trailingIdx += 1;
    outReal[((outIdx++ * outStride) | 0)] = skewness;
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
  sp.dPeriod = dPeriod;
  sp.coef = coef;
  sp.invPeriod = invPeriod;
  sp.peak2 = peak2;
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
export function skewOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): SkewStream {
  const sp = SkewStream.of(core);
  const retCode = skewOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SKEW openAndFill', inReal.length, startIdx, skewLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && skewLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('SKEW openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('SKEW openAndFill', retCode);
}

/** @internal */
export function skewOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): SkewStream {
  const sp = SkewStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = skewOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SKEW open', inReal.length, startIdx, skewLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && skewLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('SKEW open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('SKEW open', retCode);
}

/** @internal */
export function skewOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): SkewStream {
  optInTimePeriod = requireInt32('SKEW open', 'timePeriod', optInTimePeriod);
  requireArgument('SKEW open', 'real', inReal);
  requireHistory('SKEW open', 'real', inReal.length);
  return skewOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/** @internal */
export function skewOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): SkewStream {
  optInTimePeriod = requireInt32('SKEW openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('SKEW openAndFill', 'real', inReal);
  requireHistory('SKEW openAndFill', 'real', inReal.length);
  const guardLookback = skewLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('SKEW openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('SKEW openAndFill', inReal.length, guardLookback);
  requireOutput('SKEW openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('SKEW openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('SKEW openAndFill', 'outReal', 'real');
  }
  return skewOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live SKEW stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link skew} there.
 *
 * @param real - Warm-up history. The series to measure.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SkewOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `skewLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { skewOpen } from 'ta-lib';
 *
 * const series = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = skewOpen(series, { timePeriod: 30 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function skewOpen(real: ArrayLike<number>, options?: SkewOptions): SkewStream {
  const o = optionsOf('SKEW open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('SKEW open', o.core);
  return skewOpenPositional(real, optInTimePeriod, core);
}

/**
 * {@link skewOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link skew}.
 *
 * @param real - Warm-up history. The series to measure.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link SkewOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link skew} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `skewLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or a
 *   buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { skewOpenAndFill } from 'ta-lib';
 *
 * const series = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = skewOpenAndFill(series, { timePeriod: 30 });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function skewOpenAndFill(real: ArrayLike<number>, options?: SkewOptions): SkewResult & { stream: SkewStream } {
  const o = optionsOf('SKEW openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('SKEW openAndFill', o.core);
  const capacity = fillCapacity(skewLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = skewOpenAndFillPositional(real, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
