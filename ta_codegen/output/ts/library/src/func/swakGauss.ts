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
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  100126 KL,CC  Creation (#486).
 *  100326 MF,CC  The newest output on one fused step (#486).
 *  100526 MF,CC  b2p without its cancellation at long periods (#486).
 */

import { FuncUnstId } from '../enums.ts';
import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma, fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function swakGaussLookbackPositional(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 10000) {
    return -1;
  }
  /* No structural lookback. Every term of the recurrence exists at the first
   * bar -- the two history slots are seeded from that bar rather than read
   * from before it -- and there is no callee whose lookback could be
   * inherited, so the function's own unstable period is the whole of it.
   */
  return core.unstableCount(FuncUnstId.SWAK_GAUSS, (((((((10 + 3) * (optInTimePeriod + 2)) | 0) + 8) / 9) | 0)), (((((((19 + 3) * (optInTimePeriod + 2)) | 0) + 8) / 9) | 0)));
}

/** @internal */
export function swakGaussDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (swakGaussLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function swakGaussImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let i = 0;
  let outIdx = 0;
  let today = 0;
  let lookbackTotal = 0;
  let s = 0;
  let b2p = 0;
  let a2p = 0;
  let om = 0;
  let c0 = 0;
  let a1 = 0;
  let a2 = 0;
  let y = 0;
  let y1 = 0;
  let y2 = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 10000) {
    return RetCode.BAD_PARAM;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = swakGaussLookbackPositional(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  /* The two-pole alpha of Ehlers' "Swiss Army Knife Indicator", Figure 5. The
   * paper's 360/P is a full turn, so 2*pi/P.
   *
   * Keep 1 - cos(w) as 2*sin(w/2)^2: the subtraction cancels more of b2p's
   * digits the longer the period, and the cutoff drifts with them.
   */
  s = Math.sin(3.141592653589793 / optInTimePeriod);
  b2p = 2.415 * (2.0 * s * s);
  a2p = -b2p + Math.sqrt(fma(b2p, b2p, 2.0 * b2p));
  /* The Gaussian row of Figure 5: numerator a2p^2 on the bar alone, no
   * x[i-1] or x[i-2] term. Its DC gain is 1, so the line sits on price.
   */
  om = 1.0 - a2p;
  c0 = a2p * a2p;
  a1 = 2.0 * om;
  a2 = -(om * om);
  today = startIdx - lookbackTotal;
  /* Start in the steady state of a constant input equal to the first bar: a
   * DC-gain-1 row answers that constant, so both output slots hold it. The
   * input slots would hold it too, but this row's b1 and b2 are zero and
   * never reads them -- which is also why outReal may alias inReal here.
   */
  y1 = inReal[today];
  y2 = y1;
  /* Skip the unstable period: run the recurrence but publish nothing. */
  i = lookbackTotal;
  while (i !== 0) {
    let _fma0: number;
    {
      const _a: number = a2, _b: number = y2, _c: number = c0 * inReal[today];
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
    {
      const _a: number = a1, _b: number = y1, _c: number = _fma0;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      y = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    y2 = y1;
    y1 = y;
    today += 1;
    i -= 1;
  }
  outIdx = 0;
  while (today <= endIdx) {
    /* a1*y1 stays the outermost term. y1 is the newest output, so the bar
     * rate is the latency of whatever y1 crosses to become y: outermost, that
     * is one fused step. Nested inside, it is three, and every backend's last
     * bit moves with it.
     */
    let _fma2: number;
    {
      const _a: number = a2, _b: number = y2, _c: number = c0 * inReal[today];
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
    {
      const _a: number = a1, _b: number = y1, _c: number = _fma2;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      y = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    y2 = y1;
    y1 = y;
    outReal[outIdx] = y;
    outIdx = outIdx + 1;
    today += 1;
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 10000, 'in 2..10000']] as const;

/** @internal */
export function swakGaussPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('SWAK_GAUSS', 'startIdx', startIdx);
  endIdx = requireInt32('SWAK_GAUSS', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('SWAK_GAUSS', 'timePeriod', optInTimePeriod);
  requireIndexRange('SWAK_GAUSS', startIdx, endIdx);
  const guardLookback = swakGaussLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('SWAK_GAUSS', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('SWAK_GAUSS', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('SWAK_GAUSS', 'real', inReal, guardInLen);
  requireOutput('SWAK_GAUSS', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('SWAK_GAUSS', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = swakGaussImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('SWAK_GAUSS', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link swakGauss}, {@link swakGaussLookback},
 * {@link swakGaussDisplayShift}, {@link swakGaussOpen} and
 * {@link swakGaussOpenAndFill}. Every key is optional: an omitted or `undefined`
 * key takes its default, and a call ignores the keys it has no use for.
 */
export interface SwakGaussOptions {
  /**
   * Cutoff period; shorter keeps more of the fast motion, longer smooths harder and
   * lags more (default 20; range 2 to 10000; `Core.INTEGER_DEFAULT` selects the
   * default).
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
 * What {@link swakGauss} returns, and {@link swakGaussOpenAndFill} with its
 * stream.
 */
export interface SwakGaussResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * The filtered line, on the same scale as the input. `values[k]` is the value at
   * input bar `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link swakGauss} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SwakGaussOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function swakGaussLookback(options?: SwakGaussOptions): number {
  const o = optionsOf('SWAK_GAUSS lookback', options);
  return swakGaussLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('SWAK_GAUSS lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link swakGauss}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SwakGaussOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function swakGaussDisplayShift(outputIdx: number, options?: SwakGaussOptions): number {
  const o = optionsOf('SWAK_GAUSS displayShift', options);
  return swakGaussDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('SWAK_GAUSS displayShift', o.core));
}

/**
 * The Gaussian row of John Ehlers' Swiss Army Knife filter: a two-pole low-pass
 * smoother whose impulse response approximates a Gaussian, so it lags price
 * without the overshoot a sharper filter leaves behind a turn. Read it as a
 * smoothed price line. Its DC gain is 1, so a flat market returns the price itself
 * and the line sits on the chart with price rather than oscillating around zero.
 * The cutoff period sets how much is removed: cycles far shorter than it are
 * attenuated smoothly, cycles far longer pass essentially untouched, and there is
 * no band in between where the filter rings. The alpha is the one the Gaussian
 * construction asks for, not the exponential-moving-average alpha. Ehlers notes
 * that the same shape can be had by taking an EMA of an EMA, but that doing so
 * "leaves the computation of the correct alpha to be a little nebulous", and gives
 * these coefficients instead. `ema` cannot stand in: its alpha is `2/(n+1)` for
 * whole `n`, and at a cutoff period of 20 this filter's alpha is 0.38217, which
 * would ask for `n = 4.233`.
 *
 * Formula and more info at
 * [ta-lib.org/functions/swak_gauss](https://ta-lib.org/functions/swak_gauss).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link swakGaussLookback} is a success with empty outputs.
 *
 * @param real - The series to filter; Ehlers' default is the bar midpoint
 *   `(H+L)/2`, which the caller passes as `medprice` output.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link SwakGaussOptions}.
 * @returns `begIdx` and the values; see {@link SwakGaussResult}. Each output is a
 *   view of its buffer, the caller's or a new one.
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
 * import { swakGauss } from 'ta-lib';
 *
 * const series = new Float64Array(252); // your data, oldest first
 *
 * const { begIdx, values } = swakGauss(series, { timePeriod: 20 });
 * console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
 * ```
 * @see [`swakButter`](https://ta-lib.org/functions/swak_butter)
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`medprice`](https://ta-lib.org/functions/medprice)
 */
export function swakGauss(real: ArrayLike<number>, options?: SwakGaussOptions): SwakGaussResult {
  const o = optionsOf('SWAK_GAUSS', options);
  if (o.endIdx === undefined) {
    requireSeries('SWAK_GAUSS', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('SWAK_GAUSS', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, swakGaussLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = swakGaussPositional(startIdx, endIdx, real, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live SWAK_GAUSS stream: one value per closed bar, bit-identical to
 * {@link swakGauss} over the same series. Open it with {@link swakGaussOpen} or
 * {@link swakGaussOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/swak_gauss](https://ta-lib.org/functions/swak_gauss).
 */
export class SwakGaussStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  c0: number = 0;
  /** @internal */
  a1: number = 0;
  /** @internal */
  a2: number = 0;
  /** @internal */
  y1: number = 0;
  /** @internal */
  y2: number = 0;
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
  static of(core: Core): SwakGaussStream {
    return new SwakGaussStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link swakGauss} reports them over the same bars. Each accepted `update` and
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
      throw streamFailure('SWAK_GAUSS advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: The series to filter; Ehlers' default is
   *   the bar midpoint `(H+L)/2`, which the caller passes as `medprice` output.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('SWAK_GAUSS update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('SWAK_GAUSS update', 'real');
    }
    swakGaussStepImpl(this, real);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: The series to filter; Ehlers' default is
   *   the bar midpoint `(H+L)/2`, which the caller passes as `medprice` output.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number): number {
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('SWAK_GAUSS peek', 'real');
    }
    const sp = this;
    const inReal = real;
    let y = 0;
    let cur_outReal = 0;
    let y1 = sp.y1;
    let y2 = sp.y2;
    /* a1*y1 stays the outermost term. y1 is the newest output, so the bar
     * rate is the latency of whatever y1 crosses to become y: outermost, that
     * is one fused step. Nested inside, it is three, and every backend's last
     * bit moves with it.
     */
    y = fma(sp.a1, y1, fma(sp.a2, y2, sp.c0 * inReal));
    y2 = y1;
    y1 = y;
    cur_outReal = y;
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
  clone(): SwakGaussStream {
    const c = new SwakGaussStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.c0 = this.c0;
    c.a1 = this.a1;
    c.a2 = this.a2;
    c.y1 = this.y1;
    c.y2 = this.y2;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function swakGaussStepImpl(sp: SwakGaussStream, inReal: number): void {
  let y = 0;
  /* a1*y1 stays the outermost term. y1 is the newest output, so the bar
   * rate is the latency of whatever y1 crosses to become y: outermost, that
   * is one fused step. Nested inside, it is three, and every backend's last
   * bit moves with it.
   */
  y = fma(sp.a1, sp.y1, fma(sp.a2, sp.y2, sp.c0 * inReal));
  sp.y2 = sp.y1;
  sp.y1 = y;
  sp.cur_outReal = y;
}

function swakGaussOpenImpl(
  sp: SwakGaussStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let i = 0;
  let outIdx = 0;
  let today = 0;
  let lookbackTotal = 0;
  let s = 0;
  let b2p = 0;
  let a2p = 0;
  let om = 0;
  let c0 = 0;
  let a1 = 0;
  let a2 = 0;
  let y = 0;
  let y1 = 0;
  let y2 = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 10000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = swakGaussLookbackPositional(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* The two-pole alpha of Ehlers' "Swiss Army Knife Indicator", Figure 5. The
   * paper's 360/P is a full turn, so 2*pi/P.
   *
   * Keep 1 - cos(w) as 2*sin(w/2)^2: the subtraction cancels more of b2p's
   * digits the longer the period, and the cutoff drifts with them.
   */
  s = Math.sin(3.141592653589793 / optInTimePeriod);
  b2p = 2.415 * (2.0 * s * s);
  a2p = -b2p + Math.sqrt(fma(b2p, b2p, 2.0 * b2p));
  /* The Gaussian row of Figure 5: numerator a2p^2 on the bar alone, no
   * x[i-1] or x[i-2] term. Its DC gain is 1, so the line sits on price.
   */
  om = 1.0 - a2p;
  c0 = a2p * a2p;
  a1 = 2.0 * om;
  a2 = -(om * om);
  today = startIdx - lookbackTotal;
  /* Start in the steady state of a constant input equal to the first bar: a
   * DC-gain-1 row answers that constant, so both output slots hold it. The
   * input slots would hold it too, but this row's b1 and b2 are zero and
   * never reads them -- which is also why outReal may alias inReal here.
   */
  y1 = inReal[today];
  y2 = y1;
  /* Skip the unstable period: run the recurrence but publish nothing. */
  i = lookbackTotal;
  while (i !== 0) {
    let _fma0: number;
    {
      const _a: number = a2, _b: number = y2, _c: number = c0 * inReal[today];
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
    {
      const _a: number = a1, _b: number = y1, _c: number = _fma0;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      y = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    y2 = y1;
    y1 = y;
    today += 1;
    i -= 1;
  }
  outIdx = 0;
  while (today <= endIdx) {
    /* a1*y1 stays the outermost term. y1 is the newest output, so the bar
     * rate is the latency of whatever y1 crosses to become y: outermost, that
     * is one fused step. Nested inside, it is three, and every backend's last
     * bit moves with it.
     */
    let _fma2: number;
    {
      const _a: number = a2, _b: number = y2, _c: number = c0 * inReal[today];
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
    {
      const _a: number = a1, _b: number = y1, _c: number = _fma2;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      y = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    y2 = y1;
    y1 = y;
    outReal[((outIdx * outStride) | 0)] = y;
    outIdx = outIdx + 1;
    today += 1;
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  sp.optInTimePeriod = optInTimePeriod;
  sp.c0 = c0;
  sp.a1 = a1;
  sp.a2 = a2;
  sp.y1 = y1;
  sp.y2 = y2;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function swakGaussOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): SwakGaussStream {
  const sp = SwakGaussStream.of(core);
  const retCode = swakGaussOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SWAK_GAUSS openAndFill', inReal.length, startIdx, swakGaussLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && swakGaussLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('SWAK_GAUSS openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('SWAK_GAUSS openAndFill', retCode);
}

/** @internal */
export function swakGaussOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): SwakGaussStream {
  const sp = SwakGaussStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = swakGaussOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SWAK_GAUSS open', inReal.length, startIdx, swakGaussLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && swakGaussLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('SWAK_GAUSS open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('SWAK_GAUSS open', retCode);
}

/** @internal */
export function swakGaussOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): SwakGaussStream {
  optInTimePeriod = requireInt32('SWAK_GAUSS open', 'timePeriod', optInTimePeriod);
  requireArgument('SWAK_GAUSS open', 'real', inReal);
  requireHistory('SWAK_GAUSS open', 'real', inReal.length);
  return swakGaussOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/** @internal */
export function swakGaussOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): SwakGaussStream {
  optInTimePeriod = requireInt32('SWAK_GAUSS openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('SWAK_GAUSS openAndFill', 'real', inReal);
  requireHistory('SWAK_GAUSS openAndFill', 'real', inReal.length);
  const guardLookback = swakGaussLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('SWAK_GAUSS openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('SWAK_GAUSS openAndFill', inReal.length, guardLookback);
  requireOutput('SWAK_GAUSS openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('SWAK_GAUSS openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('SWAK_GAUSS openAndFill', 'outReal', 'real');
  }
  return swakGaussOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live SWAK_GAUSS stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link swakGauss} there.
 *
 * @param real - Warm-up history. The series to filter; Ehlers' default is the bar
 *   midpoint `(H+L)/2`, which the caller passes as `medprice` output.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SwakGaussOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `swakGaussLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, an integer parameter that is not an int32 or a
 *   parameter outside its range.
 *
 * @example
 * ```ts
 * import { swakGaussOpen } from 'ta-lib';
 *
 * const series = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = swakGaussOpen(series, { timePeriod: 20 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function swakGaussOpen(real: ArrayLike<number>, options?: SwakGaussOptions): SwakGaussStream {
  const o = optionsOf('SWAK_GAUSS open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('SWAK_GAUSS open', o.core);
  return swakGaussOpenPositional(real, optInTimePeriod, core);
}

/**
 * {@link swakGaussOpen} that also computes the outputs over the whole history in
 * the same pass, bit-identically to {@link swakGauss}.
 *
 * @param real - Warm-up history. The series to filter; Ehlers' default is the bar
 *   midpoint `(H+L)/2`, which the caller passes as `medprice` output.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link SwakGaussOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link swakGauss} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `swakGaussLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, an integer parameter that is not an int32, a parameter
 *   outside its range or a buffer too short, of the wrong type or starting where
 *   another buffer or a `Float64Array` input starts, or, on a history long enough to
 *   open, one sharing memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { swakGaussOpenAndFill } from 'ta-lib';
 *
 * const series = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = swakGaussOpenAndFill(series, {
 *   timePeriod: 20,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function swakGaussOpenAndFill(real: ArrayLike<number>, options?: SwakGaussOptions): SwakGaussResult & { stream: SwakGaussStream } {
  const o = optionsOf('SWAK_GAUSS openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('SWAK_GAUSS openAndFill', o.core);
  const capacity = fillCapacity(swakGaussLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = swakGaussOpenAndFillPositional(real, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
