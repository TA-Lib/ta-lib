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
 *  112326 MF,CC  First version (issue #609).
 */

import { FuncUnstId } from '../enums.ts';
import { NO_OPTIONS, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, isPlainOptions, nonFiniteBar, notPlainOptions, openFillCount, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure, unknownOption } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma, fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function ssfLookbackPositional(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 10;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 10000) {
    return -1;
  }
  let structural = 0;
  if (optInTimePeriod === 1) {
    return core.unstableCount(FuncUnstId.SSF, 0, 0);
  }
  /* The seed occupies three bars, so the first output is a recursion step
   * even at periods 2 and 3; from 4 up the lookback is EMA's P-1.
   *
   * Auto: the poles are a complex pair at radius exp(-1.414*3.14159/P), so an
   * e-fold costs P/4.44221 bars, taken as 7P/31. Two output slots carry the
   * seed, so the difference of two starts is R*r^k*cos(k*w + phi) with R and
   * phi free; against the largest difference it ever shows, the worst phase
   * leaves ln(R/S) at most 2.51 (P = 2), 1.39 at a long period. The fixed
   * part 2 covers it at every period with the 7/31 rounding (rules_check.py).
   */
  structural = optInTimePeriod - 1;
  if (structural < 3) {
    structural = 3;
  }
  return structural + core.unstableCount(FuncUnstId.SSF, (((((((7 * (10 + 2)) | 0) * optInTimePeriod) | 0) + 30) / 31) | 0), (((((((7 * (19 + 2)) | 0) * optInTimePeriod) | 0) + 30) / 31) | 0));
}

/** @internal */
export function ssfDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (ssfLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function ssfImpl(
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
  let a1 = 0;
  let s = 0;
  let c1h = 0;
  let c2 = 0;
  let c3 = 0;
  let x0 = 0;
  let x1 = 0;
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
    optInTimePeriod = 10;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 10000) {
    return RetCode.BAD_PARAM;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = ssfLookbackPositional(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  /* No smoothing at period 1: the output is a copy of the input, as MA gives
   * for every MAType. The unstable period still delays the first output.
   */
  if (optInTimePeriod === 1) {
    outBegIdx.value = startIdx;
    outIdx = 0;
    today = startIdx;
    if (today <= endIdx) {
      const _wn = endIdx - today + 1;
      const _w1_0 = outIdx;
      const _w0_0 = today;
      for (let _wk = 0; _wk < (_wn & ~3); _wk = (_wk + 4) | 0) {
        outReal[(_w1_0 + _wk) | 0] = inReal[(_w0_0 + _wk) | 0];
        outReal[(_w1_0 + _wk + 1) | 0] = inReal[(_w0_0 + _wk + 1) | 0];
        outReal[(_w1_0 + _wk + 2) | 0] = inReal[(_w0_0 + _wk + 2) | 0];
        outReal[(_w1_0 + _wk + 3) | 0] = inReal[(_w0_0 + _wk + 3) | 0];
      }
      for (let _wk = _wn & ~3; _wk < _wn; _wk = (_wk + 1) | 0) {
        outReal[(_w1_0 + _wk) | 0] = inReal[(_w0_0 + _wk) | 0];
      }
      outIdx += _wn;
      today += _wn;
    }
    outNBElement.value = outIdx;
    return RetCode.SUCCESS;
  }
  /* Ehlers' constants as he prints them: 1.414 and 3.14159 in the radius,
   * and Cosine(1.414*180/P) in degrees, so a full-precision pi in the angle.
   *
   * Keep the cosine as 1 - 2*sin(w/2)^2: cos(w) is close to 1 at a long
   * period, and a one-ULP libm difference in it moves c1 by about
   * 5.6e-18*P^2 relative; the half angle does not.
   */
  a1 = Math.exp(-1.414 * 3.14159 / optInTimePeriod);
  s = Math.sin(1.414 * 3.141592653589793 / (2.0 * optInTimePeriod));
  c2 = 2.0 * a1 * (1.0 - 2.0 * s * s);
  c3 = -(a1 * a1);
  /* c1 = 1 - c2 - c3 makes the DC gain 1 whatever the constants. Halving it
   * here is exact, so c1h*(x + x1) is c1*(x + x1)/2.
   */
  c1h = 0.5 * (1.0 - c2 - c3);
  /* Ehlers' guard: the first three bars of the history are the price. The
   * input slot is carried in a local and never re-read, because outReal may
   * alias inReal.
   */
  today = startIdx - lookbackTotal;
  y2 = inReal[today + 1];
  y1 = inReal[today + 2];
  x1 = y1;
  today += 3;
  /* Skip the rest of the lookback: run the recurrence, publish nothing. */
  i = lookbackTotal - 3;
  while (i !== 0) {
    x0 = inReal[today];
    let _fma0: number;
    {
      const _a: number = c3, _b: number = y2, _c: number = c1h * (x0 + x1);
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
      const _a: number = c2, _b: number = y1, _c: number = _fma0;
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
    x1 = x0;
    y2 = y1;
    y1 = y;
    today += 1;
    i -= 1;
  }
  outIdx = 0;
  while (today <= endIdx) {
    /* c2*y1 stays the outermost term: y1 is the newest output, so that keeps
     * the bar rate at one fused step.
     */
    x0 = inReal[today];
    let _fma2: number;
    {
      const _a: number = c3, _b: number = y2, _c: number = c1h * (x0 + x1);
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
      const _a: number = c2, _b: number = y1, _c: number = _fma2;
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

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 1, 10000, 'in 1..10000']] as const;

/** @internal */
export function ssfPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('SSF', 'startIdx', startIdx);
  endIdx = requireInt32('SSF', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('SSF', 'timePeriod', optInTimePeriod);
  requireIndexRange('SSF', startIdx, endIdx);
  const guardLookback = ssfLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('SSF', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('SSF', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('SSF', 'real', inReal, guardInLen);
  requireOutput('SSF', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('SSF', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = ssfImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('SSF', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link ssf}, {@link ssfLookback}, {@link ssfDisplayShift},
 * {@link ssfOpen} and {@link ssfOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for. A key that is not one of these is `TA_BAD_PARAM`.
 */
export interface SsfOptions {
  /**
   * Critical period: the wavelength that divides the pass band and the reject band
   * (default 10; range 1 to 10000; `Core.INTEGER_DEFAULT` selects the default).
   */
  timePeriod?: number | undefined;
  /**
   * First bar of the requested range (default 0). The lookback's bars before it are
   * still read. Batch calls only.
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

/** @internal */
export function ssfOptionsOf(funcName: string, options: SsfOptions | undefined): SsfOptions {
  if (options === undefined) {
    return NO_OPTIONS;
  }
  if (!isPlainOptions(options)) {
    throw notPlainOptions(funcName, options);
  }
  for (const k in options) {
    if (k !== 'timePeriod' && k !== 'startIdx' && k !== 'endIdx' && k !== 'core' && k !== 'outReal') {
      unknownOption(funcName, options, k, 'timePeriod startIdx endIdx core outReal');
    }
  }
  return options;
}

/** What {@link ssf} returns, and {@link ssfOpenAndFill} with its stream. */
export interface SsfResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * The smoothed line, on the same scale as the input. `values[k]` is the value at
   * input bar `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link ssf} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param options - The parameters and `core`; the function's other option keys are
 *   ignored. See {@link SsfOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` for options that are not a plain
 *   object, an option key the function does not have or a `core` that is not a
 *   `Core`.
 */
export function ssfLookback(options?: SsfOptions): number {
  const o = ssfOptionsOf('SSF lookback', options);
  return ssfLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('SSF lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link ssf}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; the function's other option keys are
 *   ignored. See {@link SsfOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` for options that are not a plain
 *   object, an option key the function does not have or a `core` that is not a
 *   `Core`.
 */
export function ssfDisplayShift(outputIdx: number, options?: SsfOptions): number {
  const o = ssfOptionsOf('SSF displayShift', options);
  return ssfDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('SSF displayShift', o.core));
}

/**
 * John Ehlers' SuperSmoother: a two-pole low-pass smoother with a zero at Nyquist,
 * which he recommends for use instead of an EMA and builds most of his later
 * indicators on. Read it as a smoothed price line with less lag than an EMA of
 * comparable smoothness. Its DC gain is 1, so a flat market returns the price
 * itself and the line sits on the chart with price. The critical period is the
 * wavelength that divides the pass band from the reject band: cycles longer than
 * it pass, shorter ones fall off at 12 dB per octave, and a two-bar alternation is
 * removed entirely. The poles are a complex pair, which no EMA of an EMA and none
 * of the `SWAK_*` rows can place.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ssf](https://ta-lib.org/functions/ssf).
 *
 * **Notes**
 * - A period of 1 performs no smoothing: the output is a copy of the input,
 *   consistent with `MA(period=1)` for every MAType.
 * - Ehlers' 2004 form averages no bars in the numerator (`c1 * x[i]`, no zero at
 *   Nyquist); he replaced it by 2013, and implementations still computing it
 *   differ from this function by up to a few percent.
 * - Implementations with an exact `sqrt(2)` and `pi` in place of Ehlers' constants
 *   differ from this function in about the fifth significant digit.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link ssfLookback} is a success with empty outputs.
 *
 * @param real - The series to smooth; Ehlers' examples use the close.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link SsfOptions}.
 * @returns `begIdx` and the values; see {@link SsfResult}. Each output is a view
 *   of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not a plain object, an
 *   option key the function does not have, a `core` that is not a `Core`, an absent
 *   or empty input, a range bound that is not an int32, an integer parameter that is
 *   not an int32, a parameter outside its range, an input too short for the range, a
 *   buffer that is empty, too short or of the wrong type or a buffer sharing memory
 *   with a `Float64Array` input it is not. Checked before anything is written.
 *
 * @example
 * ```ts
 * import { ssf } from 'ta-lib';
 *
 * const series = new Float64Array(252); // your data, oldest first
 *
 * const { begIdx, values } = ssf(series, { timePeriod: 10 });
 * console.log(values.length + ' values from bar ' + begIdx + ': [' + values[0].toFixed(4) + ', ..., ' + values[values.length - 1].toFixed(4) + ']');
 * ```
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`swakButter`](https://ta-lib.org/functions/swak_butter)
 * @see [`swakGauss`](https://ta-lib.org/functions/swak_gauss)
 * @see [`ma`](https://ta-lib.org/functions/ma)
 */
export function ssf(real: ArrayLike<number>, options?: SsfOptions): SsfResult {
  const o = ssfOptionsOf('SSF', options);
  if (o.endIdx === undefined) {
    requireSeries('SSF', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('SSF', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, ssfLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = ssfPositional(startIdx, endIdx, real, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live SSF stream: one value per closed bar, bit-identical to {@link ssf} over
 * the same series. Open it with {@link ssfOpen} or {@link ssfOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ssf](https://ta-lib.org/functions/ssf).
 */
export class SsfStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  c1h: number = 0;
  /** @internal */
  c2: number = 0;
  /** @internal */
  c3: number = 0;
  /** @internal */
  x1: number = 0;
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
  static of(core: Core): SsfStream {
    return new SsfStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link ssf} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('SSF advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: The series to smooth; Ehlers' examples use
   *   the close.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('SSF update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('SSF update', 'real');
    }
    ssfStepImpl(this, real);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: The series to smooth; Ehlers' examples use
   *   the close.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number): number {
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('SSF peek', 'real');
    }
    const sp = this;
    const inReal = real;
    let x0 = 0;
    let y = 0;
    let cur_outReal = 0;
    let x1 = sp.x1;
    let y1 = sp.y1;
    let y2 = sp.y2;
    if (sp.optInTimePeriod === 1) {
      cur_outReal = inReal;
      return cur_outReal;
    }
    /* c2*y1 stays the outermost term: y1 is the newest output, so that keeps
     * the bar rate at one fused step.
     */
    x0 = inReal;
    y = fma(sp.c2, y1, fma(sp.c3, y2, sp.c1h * (x0 + x1)));
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
  clone(): SsfStream {
    const c = new SsfStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.c1h = this.c1h;
    c.c2 = this.c2;
    c.c3 = this.c3;
    c.x1 = this.x1;
    c.y1 = this.y1;
    c.y2 = this.y2;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function ssfStepImpl(sp: SsfStream, inReal: number): void {
  let x0 = 0;
  let y = 0;
  if (sp.optInTimePeriod === 1) {
    sp.cur_outReal = inReal;
    return;
  }
  /* c2*y1 stays the outermost term: y1 is the newest output, so that keeps
   * the bar rate at one fused step.
   */
  x0 = inReal;
  y = fma(sp.c2, sp.y1, fma(sp.c3, sp.y2, sp.c1h * (x0 + sp.x1)));
  sp.x1 = x0;
  sp.y2 = sp.y1;
  sp.y1 = y;
  sp.cur_outReal = y;
}

function ssfOpenImpl(
  sp: SsfStream,
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
  let a1 = 0;
  let s = 0;
  let c1h = 0;
  let c2 = 0;
  let c3 = 0;
  let x0 = 0;
  let x1 = 0;
  let y = 0;
  let y1 = 0;
  let y2 = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 10;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 10000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (optInTimePeriod === 1) {
    let fillLb = ssfLookbackPositional(optInTimePeriod, core);
    if (startIdx > fillLb) {
      fillLb = startIdx;
    }
    if (historyLen < fillLb + 1) {
      return RetCode.INSUFFICIENT_HISTORY;
    }
    sp.optInTimePeriod = optInTimePeriod;
    sp.c1h = 0;
    sp.c2 = 0;
    sp.c3 = 0;
    sp.x1 = 0;
    sp.y1 = 0;
    sp.y2 = 0;
    outBegIdx.value = fillLb;
    outNBElement.value = historyLen - fillLb;
    if (outStride === 0) {
      outReal[0] = inReal[historyLen - 1];
    } else {
      for (let fillIdx = 0; fillIdx < historyLen - fillLb; fillIdx++) {
        outReal[fillIdx] = inReal[fillLb + fillIdx];
      }
    }
    sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
    return RetCode.SUCCESS;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = ssfLookbackPositional(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Ehlers' constants as he prints them: 1.414 and 3.14159 in the radius,
   * and Cosine(1.414*180/P) in degrees, so a full-precision pi in the angle.
   *
   * Keep the cosine as 1 - 2*sin(w/2)^2: cos(w) is close to 1 at a long
   * period, and a one-ULP libm difference in it moves c1 by about
   * 5.6e-18*P^2 relative; the half angle does not.
   */
  a1 = Math.exp(-1.414 * 3.14159 / optInTimePeriod);
  s = Math.sin(1.414 * 3.141592653589793 / (2.0 * optInTimePeriod));
  c2 = 2.0 * a1 * (1.0 - 2.0 * s * s);
  c3 = -(a1 * a1);
  /* c1 = 1 - c2 - c3 makes the DC gain 1 whatever the constants. Halving it
   * here is exact, so c1h*(x + x1) is c1*(x + x1)/2.
   */
  c1h = 0.5 * (1.0 - c2 - c3);
  /* Ehlers' guard: the first three bars of the history are the price. The
   * input slot is carried in a local and never re-read, because outReal may
   * alias inReal.
   */
  today = startIdx - lookbackTotal;
  y2 = inReal[today + 1];
  y1 = inReal[today + 2];
  x1 = y1;
  today += 3;
  /* Skip the rest of the lookback: run the recurrence, publish nothing. */
  i = lookbackTotal - 3;
  while (i !== 0) {
    x0 = inReal[today];
    let _fma0: number;
    {
      const _a: number = c3, _b: number = y2, _c: number = c1h * (x0 + x1);
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
      const _a: number = c2, _b: number = y1, _c: number = _fma0;
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
    x1 = x0;
    y2 = y1;
    y1 = y;
    today += 1;
    i -= 1;
  }
  outIdx = 0;
  while (today <= endIdx) {
    /* c2*y1 stays the outermost term: y1 is the newest output, so that keeps
     * the bar rate at one fused step.
     */
    x0 = inReal[today];
    let _fma2: number;
    {
      const _a: number = c3, _b: number = y2, _c: number = c1h * (x0 + x1);
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
      const _a: number = c2, _b: number = y1, _c: number = _fma2;
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
  sp.c1h = c1h;
  sp.c2 = c2;
  sp.c3 = c3;
  sp.x1 = x1;
  sp.y1 = y1;
  sp.y2 = y2;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function ssfOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): SsfStream {
  const sp = SsfStream.of(core);
  const retCode = ssfOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SSF openAndFill', inReal.length, startIdx, ssfLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && ssfLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('SSF openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('SSF openAndFill', retCode);
}

/** @internal */
export function ssfOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): SsfStream {
  const sp = SsfStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = ssfOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SSF open', inReal.length, startIdx, ssfLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && ssfLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('SSF open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('SSF open', retCode);
}

/** @internal */
export function ssfOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): SsfStream {
  optInTimePeriod = requireInt32('SSF open', 'timePeriod', optInTimePeriod);
  requireArgument('SSF open', 'real', inReal);
  requireHistory('SSF open', 'real', inReal.length);
  return ssfOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/** @internal */
export function ssfOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): SsfStream {
  optInTimePeriod = requireInt32('SSF openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('SSF openAndFill', 'real', inReal);
  requireHistory('SSF openAndFill', 'real', inReal.length);
  const guardLookback = ssfLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('SSF openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('SSF openAndFill', inReal.length, guardLookback);
  requireOutput('SSF openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('SSF openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('SSF openAndFill', 'outReal', 'real');
  }
  return ssfOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live SSF stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link ssf} there.
 *
 * @param real - Warm-up history. The series to smooth; Ehlers' examples use the
 *   close.
 * @param options - The parameters and `core`; the function's other option keys are
 *   ignored. See {@link SsfOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `ssfLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not a plain object, an option key the function does not have, a
 *   `core` that is not a `Core`, an absent input, an integer parameter that is not
 *   an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { ssfOpen } from 'ta-lib';
 *
 * const series = Float64Array.from({ length: 252 }, (_, i) =>
 *   90 + (i % 40 - 20) * (i % 40 - 20) / 20.0);
 * const stream = ssfOpen(series, { timePeriod: 10 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function ssfOpen(real: ArrayLike<number>, options?: SsfOptions): SsfStream {
  const o = ssfOptionsOf('SSF open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('SSF open', o.core);
  return ssfOpenPositional(real, optInTimePeriod, core);
}

/**
 * {@link ssfOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link ssf}.
 *
 * @param real - Warm-up history. The series to smooth; Ehlers' examples use the
 *   close.
 * @param options - The parameters, `core` and output buffers; the function's other
 *   option keys are ignored. See {@link SsfOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link ssf} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `ssfLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not a plain object, an option key the function does not have, a
 *   `core` that is not a `Core`, an absent input, an integer parameter that is not
 *   an int32, a parameter outside its range or a buffer too short, of the wrong type
 *   or starting where another buffer or a `Float64Array` input starts, or, on a
 *   history long enough to open, one sharing memory with another buffer or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { ssfOpenAndFill } from 'ta-lib';
 *
 * const series = Float64Array.from({ length: 252 }, (_, i) =>
 *   90 + (i % 40 - 20) * (i % 40 - 20) / 20.0);
 * const { stream, begIdx, values } = ssfOpenAndFill(series, { timePeriod: 10 });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function ssfOpenAndFill(real: ArrayLike<number>, options?: SsfOptions): SsfResult & { stream: SsfStream } {
  const o = ssfOptionsOf('SSF openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('SSF openAndFill', o.core);
  const capacity = fillCapacity(ssfLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = ssfOpenAndFillPositional(real, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/** @internal */
export function ssfStepTape(sp: SsfStream, _tape: Float64Array, _tapeBase: number, _tapeMask: number, inReal: number): number {
  ssfStepImpl(sp, inReal);
  sp.outRangeCount++;
  return sp.cur_outReal;
}

/** @internal */
export function ssfPeekTape(sp: SsfStream, _tape: Float64Array, _tapeBase: number, _tapeMask: number, inReal: number): number {
  return sp.peek(inReal);
}

/** @internal */
export function ssfTapeDetach(_sp: SsfStream): number {
  return 0;
}
