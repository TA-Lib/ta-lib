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
export function swak2phpLookbackPositional(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 10000) {
    return -1;
  }
  /* No structural lookback: the two input slots and the two output slots are
   * seeded from the first bar rather than read from before it, and there is
   * no callee whose lookback could be inherited.
   */
  return core.unstableCount(FuncUnstId.SWAK_2PHP, (((((((10 + 3) * (optInTimePeriod + 2)) | 0) + 8) / 9) | 0)), (((((((19 + 3) * (optInTimePeriod + 2)) | 0) + 8) / 9) | 0)));
}

/** @internal */
export function swak2phpDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (swak2phpLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function swak2phpImpl(
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
  let x0 = 0;
  let x1 = 0;
  let x2 = 0;
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
  lookbackTotal = swak2phpLookbackPositional(optInTimePeriod, core);
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
  /* The two-pole high-pass row: the same double real pole as the Gaussian
   * and Butterworth rows, with a (1, -2, 1) numerator instead. That
   * numerator is zero on a constant, so the DC gain is 0 and the line is
   * centred on zero; at Nyquist it weighs 4, and c0 is exactly what divides
   * that back to unity -- ((2-a2p)/2)^2 times 4/(2-a2p)^2 is 1. Rolling off
   * twice as steeply as the one-pole row is the whole reason to pay for the
   * second pole.
   */
  om = 1.0 - a2p;
  c0 = (1.0 - a2p / 2.0) * (1.0 - a2p / 2.0);
  a1 = 2.0 * om;
  a2 = -(om * om);
  today = startIdx - lookbackTotal;
  /* Start in the steady state of a constant input equal to the first bar: a
   * DC-gain-0 row answers 0 for a constant, so both output slots start there
   * while both input slots hold the bar. The input slots are carried in
   * locals and never re-read from inReal, because outReal may alias it.
   */
  x1 = inReal[today];
  x2 = x1;
  y1 = 0.0;
  y2 = 0.0;
  /* Skip the unstable period: run the recurrence but publish nothing. */
  i = lookbackTotal;
  while (i !== 0) {
    x0 = inReal[today];
    let _fma0: number;
    {
      const _a: number = a2, _b: number = y2, _c: number = c0 * (x0 - 2.0 * x1 + x2);
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
    x2 = x1;
    x1 = x0;
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
    x0 = inReal[today];
    let _fma2: number;
    {
      const _a: number = a2, _b: number = y2, _c: number = c0 * (x0 - 2.0 * x1 + x2);
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
    x2 = x1;
    x1 = x0;
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
export function swak2phpPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('SWAK_2PHP', 'startIdx', startIdx);
  endIdx = requireInt32('SWAK_2PHP', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('SWAK_2PHP', 'timePeriod', optInTimePeriod);
  requireIndexRange('SWAK_2PHP', startIdx, endIdx);
  const guardLookback = swak2phpLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('SWAK_2PHP', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('SWAK_2PHP', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('SWAK_2PHP', 'real', inReal, guardInLen);
  requireOutput('SWAK_2PHP', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('SWAK_2PHP', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = swak2phpImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('SWAK_2PHP', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link swak2php}, {@link swak2phpLookback},
 * {@link swak2phpDisplayShift}, {@link swak2phpOpen} and
 * {@link swak2phpOpenAndFill}. Every key is optional: an omitted or `undefined`
 * key takes its default, and a call ignores the keys it has no use for.
 */
export interface Swak2phpOptions {
  /**
   * Cutoff period; cycles longer than it are removed, cycles shorter pass (default
   * 20; range 2 to 10000; `Core.INTEGER_DEFAULT` selects the default).
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

/** What {@link swak2php} returns, and {@link swak2phpOpenAndFill} with its stream. */
export interface Swak2phpResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * The detrended line, centred on zero. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link swak2php} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link Swak2phpOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function swak2phpLookback(options?: Swak2phpOptions): number {
  const o = optionsOf('SWAK_2PHP lookback', options);
  return swak2phpLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('SWAK_2PHP lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link swak2php}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link Swak2phpOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function swak2phpDisplayShift(outputIdx: number, options?: Swak2phpOptions): number {
  const o = optionsOf('SWAK_2PHP displayShift', options);
  return swak2phpDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('SWAK_2PHP displayShift', o.core));
}

/**
 * The two-pole high-pass row of John Ehlers' Swiss Army Knife filter: the same
 * double real pole the Gaussian and Butterworth rows use, with a detrending
 * numerator instead of a smoothing one. It removes what is slower than the cutoff
 * period and rolls off twice as steeply as the one-pole row. Read it as an
 * oscillator, not as price. Its DC gain is 0, so a flat market returns zero and a
 * trend returns its departure from itself. Against `swakHp` the difference is the
 * slope of the transition: the second pole buys a sharper separation between what
 * is kept and what is removed.
 *
 * Formula and more info at
 * [ta-lib.org/functions/swak_2php](https://ta-lib.org/functions/swak_2php).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link swak2phpLookback} is a success with empty outputs.
 *
 * @param real - The series to detrend; Ehlers' default is the bar midpoint
 *   `(H+L)/2`, which the caller passes as `medprice` output.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link Swak2phpOptions}.
 * @returns `begIdx` and the values; see {@link Swak2phpResult}. Each output is a
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
 * import { swak2php } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = swak2php(data, { timePeriod: 20 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`swakHp`](https://ta-lib.org/functions/swak_hp)
 * @see [`swakBp`](https://ta-lib.org/functions/swak_bp)
 * @see [`medprice`](https://ta-lib.org/functions/medprice)
 */
export function swak2php(real: ArrayLike<number>, options?: Swak2phpOptions): Swak2phpResult {
  const o = optionsOf('SWAK_2PHP', options);
  if (o.endIdx === undefined) {
    requireSeries('SWAK_2PHP', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('SWAK_2PHP', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, swak2phpLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = swak2phpPositional(startIdx, endIdx, real, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live SWAK_2PHP stream: one value per closed bar, bit-identical to
 * {@link swak2php} over the same series. Open it with {@link swak2phpOpen} or
 * {@link swak2phpOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/swak_2php](https://ta-lib.org/functions/swak_2php).
 */
export class Swak2phpStream {
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
  x1: number = 0;
  /** @internal */
  x2: number = 0;
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
  static of(core: Core): Swak2phpStream {
    return new Swak2phpStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link swak2php} reports them over the same bars. Each accepted `update` and
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
      throw streamFailure('SWAK_2PHP advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: The series to detrend; Ehlers' default is
   *   the bar midpoint `(H+L)/2`, which the caller passes as `medprice` output.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('SWAK_2PHP update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('SWAK_2PHP update', 'real');
    }
    swak2phpStepImpl(this, real);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: The series to detrend; Ehlers' default is
   *   the bar midpoint `(H+L)/2`, which the caller passes as `medprice` output.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number): number {
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('SWAK_2PHP peek', 'real');
    }
    const sp = this;
    const inReal = real;
    let x0 = 0;
    let y = 0;
    let cur_outReal = 0;
    let x1 = sp.x1;
    let x2 = sp.x2;
    let y1 = sp.y1;
    let y2 = sp.y2;
    /* a1*y1 stays the outermost term. y1 is the newest output, so the bar
     * rate is the latency of whatever y1 crosses to become y: outermost, that
     * is one fused step. Nested inside, it is three, and every backend's last
     * bit moves with it.
     */
    x0 = inReal;
    y = fma(sp.a1, y1, fma(sp.a2, y2, sp.c0 * (x0 - 2.0 * x1 + x2)));
    x2 = x1;
    x1 = x0;
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
  clone(): Swak2phpStream {
    const c = new Swak2phpStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.c0 = this.c0;
    c.a1 = this.a1;
    c.a2 = this.a2;
    c.x1 = this.x1;
    c.x2 = this.x2;
    c.y1 = this.y1;
    c.y2 = this.y2;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function swak2phpStepImpl(sp: Swak2phpStream, inReal: number): void {
  let x0 = 0;
  let y = 0;
  /* a1*y1 stays the outermost term. y1 is the newest output, so the bar
   * rate is the latency of whatever y1 crosses to become y: outermost, that
   * is one fused step. Nested inside, it is three, and every backend's last
   * bit moves with it.
   */
  x0 = inReal;
  y = fma(sp.a1, sp.y1, fma(sp.a2, sp.y2, sp.c0 * (x0 - 2.0 * sp.x1 + sp.x2)));
  sp.x2 = sp.x1;
  sp.x1 = x0;
  sp.y2 = sp.y1;
  sp.y1 = y;
  sp.cur_outReal = y;
}

function swak2phpOpenImpl(
  sp: Swak2phpStream,
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
  let x0 = 0;
  let x1 = 0;
  let x2 = 0;
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
  lookbackTotal = swak2phpLookbackPositional(optInTimePeriod, core);
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
  /* The two-pole high-pass row: the same double real pole as the Gaussian
   * and Butterworth rows, with a (1, -2, 1) numerator instead. That
   * numerator is zero on a constant, so the DC gain is 0 and the line is
   * centred on zero; at Nyquist it weighs 4, and c0 is exactly what divides
   * that back to unity -- ((2-a2p)/2)^2 times 4/(2-a2p)^2 is 1. Rolling off
   * twice as steeply as the one-pole row is the whole reason to pay for the
   * second pole.
   */
  om = 1.0 - a2p;
  c0 = (1.0 - a2p / 2.0) * (1.0 - a2p / 2.0);
  a1 = 2.0 * om;
  a2 = -(om * om);
  today = startIdx - lookbackTotal;
  /* Start in the steady state of a constant input equal to the first bar: a
   * DC-gain-0 row answers 0 for a constant, so both output slots start there
   * while both input slots hold the bar. The input slots are carried in
   * locals and never re-read from inReal, because outReal may alias it.
   */
  x1 = inReal[today];
  x2 = x1;
  y1 = 0.0;
  y2 = 0.0;
  /* Skip the unstable period: run the recurrence but publish nothing. */
  i = lookbackTotal;
  while (i !== 0) {
    x0 = inReal[today];
    let _fma0: number;
    {
      const _a: number = a2, _b: number = y2, _c: number = c0 * (x0 - 2.0 * x1 + x2);
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
    x2 = x1;
    x1 = x0;
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
    x0 = inReal[today];
    let _fma2: number;
    {
      const _a: number = a2, _b: number = y2, _c: number = c0 * (x0 - 2.0 * x1 + x2);
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
    x2 = x1;
    x1 = x0;
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
  sp.x1 = x1;
  sp.x2 = x2;
  sp.y1 = y1;
  sp.y2 = y2;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function swak2phpOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): Swak2phpStream {
  const sp = Swak2phpStream.of(core);
  const retCode = swak2phpOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SWAK_2PHP openAndFill', inReal.length, startIdx, swak2phpLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && swak2phpLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('SWAK_2PHP openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('SWAK_2PHP openAndFill', retCode);
}

/** @internal */
export function swak2phpOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): Swak2phpStream {
  const sp = Swak2phpStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = swak2phpOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SWAK_2PHP open', inReal.length, startIdx, swak2phpLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && swak2phpLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('SWAK_2PHP open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('SWAK_2PHP open', retCode);
}

/** @internal */
export function swak2phpOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): Swak2phpStream {
  optInTimePeriod = requireInt32('SWAK_2PHP open', 'timePeriod', optInTimePeriod);
  requireArgument('SWAK_2PHP open', 'real', inReal);
  requireHistory('SWAK_2PHP open', 'real', inReal.length);
  return swak2phpOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/** @internal */
export function swak2phpOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): Swak2phpStream {
  optInTimePeriod = requireInt32('SWAK_2PHP openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('SWAK_2PHP openAndFill', 'real', inReal);
  requireHistory('SWAK_2PHP openAndFill', 'real', inReal.length);
  const guardLookback = swak2phpLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('SWAK_2PHP openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('SWAK_2PHP openAndFill', inReal.length, guardLookback);
  requireOutput('SWAK_2PHP openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('SWAK_2PHP openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('SWAK_2PHP openAndFill', 'outReal', 'real');
  }
  return swak2phpOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live SWAK_2PHP stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link swak2php} there.
 *
 * @param real - Warm-up history. The series to detrend; Ehlers' default is the bar
 *   midpoint `(H+L)/2`, which the caller passes as `medprice` output.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link Swak2phpOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `swak2phpLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32 or a parameter outside its
 *   range.
 *
 * @example
 * ```ts
 * import { swak2phpOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = swak2phpOpen(data, { timePeriod: 20 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function swak2phpOpen(real: ArrayLike<number>, options?: Swak2phpOptions): Swak2phpStream {
  const o = optionsOf('SWAK_2PHP open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('SWAK_2PHP open', o.core);
  return swak2phpOpenPositional(real, optInTimePeriod, core);
}

/**
 * {@link swak2phpOpen} that also computes the outputs over the whole history in
 * the same pass, bit-identically to {@link swak2php}.
 *
 * @param real - Warm-up history. The series to detrend; Ehlers' default is the bar
 *   midpoint `(H+L)/2`, which the caller passes as `medprice` output.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link Swak2phpOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link swak2php} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `swak2phpLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range
 *   or a buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { swak2phpOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = swak2phpOpenAndFill(data, {
 *   timePeriod: 20,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function swak2phpOpenAndFill(real: ArrayLike<number>, options?: Swak2phpOptions): Swak2phpResult & { stream: Swak2phpStream } {
  const o = optionsOf('SWAK_2PHP openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('SWAK_2PHP openAndFill', o.core);
  const capacity = fillCapacity(swak2phpLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = swak2phpOpenAndFillPositional(real, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
