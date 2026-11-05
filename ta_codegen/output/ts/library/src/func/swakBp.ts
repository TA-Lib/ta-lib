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
 */

import { FuncUnstId } from '../enums.ts';
import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireReal, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma, fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function swakBpLookbackPositional(optInTimePeriod: number, optInDelta: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (typeof optInDelta !== 'number') {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 5 || optInTimePeriod > 2000) {
    return -1;
  }
  if (optInDelta === Core.REAL_DEFAULT) {
    optInDelta = 0.1;
  } else if (!(optInDelta >= 5e-2 && optInDelta <= 5e-1)) {
    return -1;
  }
  let count4 = 0;
  let count8 = 0;
  /* ceil( (K+1)*P / (6*delta) ) per level, in this order of operations: the
   * count is defined as this double expression, not as the real quotient.
   */
  count4 = (Math.ceil((((11 * optInTimePeriod) | 0)) / (6.0 * optInDelta)) | 0);
  count8 = (Math.ceil((((20 * optInTimePeriod) | 0)) / (6.0 * optInDelta)) | 0);
  /* No structural lookback: the two input slots and the two output slots are
   * seeded from the first bar rather than read from before it, and there is
   * no callee whose lookback could be inherited.
   */
  return core.unstableCount(FuncUnstId.SWAK_BP, count4, count8);
}

/** @internal */
export function swakBpDisplayShiftPositional(optInTimePeriod: number, optInDelta: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (swakBpLookbackPositional(optInTimePeriod, optInDelta, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function swakBpImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  optInDelta: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let i = 0;
  let outIdx = 0;
  let today = 0;
  let lookbackTotal = 0;
  let w = 0;
  let beta = 0;
  let t = 0;
  let abp = 0;
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
  } else if (optInTimePeriod < 5 || optInTimePeriod > 2000) {
    return RetCode.BAD_PARAM;
  }
  if (optInDelta === Core.REAL_DEFAULT) {
    optInDelta = 0.1;
  } else if (!(optInDelta >= 5e-2 && optInDelta <= 5e-1)) {
    return RetCode.BAD_PARAM;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = swakBpLookbackPositional(optInTimePeriod, optInDelta, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  /* The band-pass row (Ehlers, "Swiss Army Knife Indicator", Figure 5).
   * The paper's 360/P is a full turn, so 2*pi/P; beta is the cosine of the
   * centre frequency and t is the half-bandwidth angle.
   */
  w = 2.0 * 3.141592653589793 / optInTimePeriod;
  beta = Math.cos(w);
  t = 4.0 * 3.141592653589793 * optInDelta / optInTimePeriod;
  /* abp is written as (1 - sin t)/cos t rather than the paper's
   * gamma - sqrt(gamma^2 - 1) with gamma = 1/cos t. The two are equal for
   * 0 < t < pi/2, which this function's ranges guarantee (t <= 0.4*pi), but
   * the published form cancels as t -> 0 because gamma^2 - 1 goes as t^2.
   * Against a 60-digit reference the published form is already 4.5e-13 off
   * at the period cap; this one is not, and that is what lets the cap be
   * 2000 rather than 1000.
   */
  abp = (1.0 - Math.sin(t)) / Math.cos(t);
  /* The numerator (1, 0, -1) is zero both on a constant and on a Nyquist
   * alternation, so this row answers 0 at DC and at Nyquist, and exactly 1
   * with zero phase at the centre period itself.
   */
  c0 = (1.0 - abp) / 2.0;
  a1 = beta * (1.0 + abp);
  a2 = -abp;
  today = startIdx - lookbackTotal;
  /* Start in the steady state of a constant input equal to the first bar: a
   * band-pass of a constant is 0, so both output slots start there while both
   * input slots hold the bar. The input slots are carried in locals and never
   * re-read from inReal, because outReal may alias it.
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
      const _a: number = a2, _b: number = y2, _c: number = c0 * (x0 - x2);
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
      const _a: number = a2, _b: number = y2, _c: number = c0 * (x0 - x2);
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

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 5, 2000, 'in 5..2000'], ['delta', Core.REAL_DEFAULT, 5e-2, 5e-1, 'in 0.05..0.5']] as const;

/** @internal */
export function swakBpPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInDelta: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('SWAK_BP', 'startIdx', startIdx);
  endIdx = requireInt32('SWAK_BP', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('SWAK_BP', 'timePeriod', optInTimePeriod);
  optInDelta = requireReal('SWAK_BP', 'delta', optInDelta);
  requireIndexRange('SWAK_BP', startIdx, endIdx);
  const guardLookback = swakBpLookbackPositional(optInTimePeriod, optInDelta, core);
  if (guardLookback < 0) {
    throw rejectedParams('SWAK_BP', PARAM_RANGES, optInTimePeriod, optInDelta);
  }
  const guardStart = clampedStart('SWAK_BP', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('SWAK_BP', 'real', inReal, guardInLen);
  requireOutput('SWAK_BP', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('SWAK_BP', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = swakBpImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, optInDelta, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('SWAK_BP', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link swakBp}, {@link swakBpLookback}, {@link swakBpDisplayShift},
 * {@link swakBpOpen} and {@link swakBpOpenAndFill}. Every key is optional: an
 * omitted or `undefined` key takes its default, and a call ignores the keys it has
 * no use for.
 */
export interface SwakBpOptions {
  /**
   * Centre period of the band; the filter passes this one untouched (default 20;
   * range 5 to 2000; `Core.INTEGER_DEFAULT` selects the default).
   */
  timePeriod?: number | undefined;
  /**
   * Half-bandwidth as a fraction of the centre period; smaller is a narrower band
   * and a longer settling transient (default 0.1; range 0.05 to 0.5;
   * `Core.REAL_DEFAULT` selects the default).
   */
  delta?: number | undefined;
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

/** What {@link swakBp} returns, and {@link swakBpOpenAndFill} with its stream. */
export interface SwakBpResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * The extracted cycle, centred on zero. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link swakBp} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SwakBpOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function swakBpLookback(options?: SwakBpOptions): number {
  const o = optionsOf('SWAK_BP lookback', options);
  return swakBpLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.delta, Core.REAL_DEFAULT), coreOf('SWAK_BP lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link swakBp}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SwakBpOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function swakBpDisplayShift(outputIdx: number, options?: SwakBpOptions): number {
  const o = optionsOf('SWAK_BP displayShift', options);
  return swakBpDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.delta, Core.REAL_DEFAULT), outputIdx, coreOf('SWAK_BP displayShift', o.core));
}

/**
 * The band-pass row of John Ehlers' Swiss Army Knife filter: a cycle extractor
 * that keeps a band of periods around a chosen centre and removes everything on
 * both sides of it. Read it as an oscillator, not as price. It answers zero on a
 * constant, zero on a bar-to-bar alternation, and exactly the input, with the same
 * amplitude and no phase shift, on a sine wave at the centre period. Between those
 * it tapers, with the half-power points near `P(1 ± delta)`: at a centre of 20
 * bars and a delta of 0.1 the band is roughly 20 ± 2 bars. What separates it from
 * the high-pass rows is that it rejects the fast end too. A detrender keeps
 * everything above its cutoff, including the bar-to-bar noise; this keeps only the
 * band asked for.
 *
 * Formula and more info at
 * [ta-lib.org/functions/swak_bp](https://ta-lib.org/functions/swak_bp).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link swakBpLookback} is a success with empty outputs.
 *
 * @param real - The series to filter; Ehlers' default is the bar midpoint
 *   `(H+L)/2`, which the caller passes as `TA_MEDPRICE` output.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link SwakBpOptions}.
 * @returns `begIdx` and the values; see {@link SwakBpResult}. Each output is a
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
 * import { swakBp } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = swakBp(data, { timePeriod: 20, delta: 0.1 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`swakHp`](https://ta-lib.org/functions/swak_hp)
 * @see [`swak2php`](https://ta-lib.org/functions/swak_2php)
 * @see [`medprice`](https://ta-lib.org/functions/medprice)
 */
export function swakBp(real: ArrayLike<number>, options?: SwakBpOptions): SwakBpResult {
  const o = optionsOf('SWAK_BP', options);
  if (o.endIdx === undefined) {
    requireSeries('SWAK_BP', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInDelta = orDefault(o.delta, Core.REAL_DEFAULT);
  const core = coreOf('SWAK_BP', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, swakBpLookbackPositional(optInTimePeriod, optInDelta, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = swakBpPositional(startIdx, endIdx, real, optInTimePeriod, optInDelta, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live SWAK_BP stream: one value per closed bar, bit-identical to {@link swakBp}
 * over the same series. Open it with {@link swakBpOpen} or
 * {@link swakBpOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/swak_bp](https://ta-lib.org/functions/swak_bp).
 */
export class SwakBpStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  optInDelta: number = 0;
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
  static of(core: Core): SwakBpStream {
    return new SwakBpStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link swakBp} reports them over the same bars. Each accepted `update` and each
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
      throw failure('SWAK_BP advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: The series to filter; Ehlers' default
   *   is the bar midpoint `(H+L)/2`, which the caller passes as `TA_MEDPRICE` output.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('SWAK_BP update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('SWAK_BP update', 'inReal');
    }
    swakBpStepImpl(this, inReal);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: The series to filter; Ehlers' default
   *   is the bar midpoint `(H+L)/2`, which the caller passes as `TA_MEDPRICE` output.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number): number {
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('SWAK_BP peek', 'inReal');
    }
    const sp = this;
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
    y = fma(sp.a1, y1, fma(sp.a2, y2, sp.c0 * (x0 - x2)));
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
  clone(): SwakBpStream {
    const c = new SwakBpStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.optInDelta = this.optInDelta;
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

function swakBpStepImpl(sp: SwakBpStream, inReal: number): void {
  let x0 = 0;
  let y = 0;
  /* a1*y1 stays the outermost term. y1 is the newest output, so the bar
   * rate is the latency of whatever y1 crosses to become y: outermost, that
   * is one fused step. Nested inside, it is three, and every backend's last
   * bit moves with it.
   */
  x0 = inReal;
  y = fma(sp.a1, sp.y1, fma(sp.a2, sp.y2, sp.c0 * (x0 - sp.x2)));
  sp.x2 = sp.x1;
  sp.x1 = x0;
  sp.y2 = sp.y1;
  sp.y1 = y;
  sp.cur_outReal = y;
}

function swakBpOpenImpl(
  sp: SwakBpStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInDelta: number,
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
  let w = 0;
  let beta = 0;
  let t = 0;
  let abp = 0;
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
  if (historyLen < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 5 || optInTimePeriod > 2000) {
    return RetCode.BAD_PARAM;
  }
  if (optInDelta === Core.REAL_DEFAULT) {
    optInDelta = 0.1;
  } else if (!(optInDelta >= 5e-2 && optInDelta <= 5e-1)) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = swakBpLookbackPositional(optInTimePeriod, optInDelta, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* The band-pass row (Ehlers, "Swiss Army Knife Indicator", Figure 5).
   * The paper's 360/P is a full turn, so 2*pi/P; beta is the cosine of the
   * centre frequency and t is the half-bandwidth angle.
   */
  w = 2.0 * 3.141592653589793 / optInTimePeriod;
  beta = Math.cos(w);
  t = 4.0 * 3.141592653589793 * optInDelta / optInTimePeriod;
  /* abp is written as (1 - sin t)/cos t rather than the paper's
   * gamma - sqrt(gamma^2 - 1) with gamma = 1/cos t. The two are equal for
   * 0 < t < pi/2, which this function's ranges guarantee (t <= 0.4*pi), but
   * the published form cancels as t -> 0 because gamma^2 - 1 goes as t^2.
   * Against a 60-digit reference the published form is already 4.5e-13 off
   * at the period cap; this one is not, and that is what lets the cap be
   * 2000 rather than 1000.
   */
  abp = (1.0 - Math.sin(t)) / Math.cos(t);
  /* The numerator (1, 0, -1) is zero both on a constant and on a Nyquist
   * alternation, so this row answers 0 at DC and at Nyquist, and exactly 1
   * with zero phase at the centre period itself.
   */
  c0 = (1.0 - abp) / 2.0;
  a1 = beta * (1.0 + abp);
  a2 = -abp;
  today = startIdx - lookbackTotal;
  /* Start in the steady state of a constant input equal to the first bar: a
   * band-pass of a constant is 0, so both output slots start there while both
   * input slots hold the bar. The input slots are carried in locals and never
   * re-read from inReal, because outReal may alias it.
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
      const _a: number = a2, _b: number = y2, _c: number = c0 * (x0 - x2);
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
      const _a: number = a2, _b: number = y2, _c: number = c0 * (x0 - x2);
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
  sp.optInDelta = optInDelta;
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
export function swakBpOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInDelta: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): SwakBpStream {
  const sp = SwakBpStream.of(core);
  const retCode = swakBpOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInDelta, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SWAK_BP openAndFill', inReal.length, startIdx, swakBpLookbackPositional(optInTimePeriod, optInDelta, core));
  }
  if (retCode === RetCode.BAD_PARAM && swakBpLookbackPositional(optInTimePeriod, optInDelta, core) < 0) {
    throw rejectedParams('SWAK_BP openAndFill', PARAM_RANGES, optInTimePeriod, optInDelta);
  }
  throw streamFailure('SWAK_BP openAndFill', retCode);
}

/** @internal */
export function swakBpOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInDelta: number,
): SwakBpStream {
  const sp = SwakBpStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = swakBpOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInDelta, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SWAK_BP open', inReal.length, startIdx, swakBpLookbackPositional(optInTimePeriod, optInDelta, core));
  }
  if (retCode === RetCode.BAD_PARAM && swakBpLookbackPositional(optInTimePeriod, optInDelta, core) < 0) {
    throw rejectedParams('SWAK_BP open', PARAM_RANGES, optInTimePeriod, optInDelta);
  }
  throw streamFailure('SWAK_BP open', retCode);
}

/** @internal */
export function swakBpOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInDelta: number,
  core: Core = Core.DEFAULT,
): SwakBpStream {
  optInTimePeriod = requireInt32('SWAK_BP open', 'timePeriod', optInTimePeriod);
  optInDelta = requireReal('SWAK_BP open', 'delta', optInDelta);
  requireArgument('SWAK_BP open', 'real', inReal);
  requireHistory('SWAK_BP open', inReal.length);
  return swakBpOpenInternal(core, float64(inReal), 0, optInTimePeriod, optInDelta);
}

/** @internal */
export function swakBpOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInDelta: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): SwakBpStream {
  optInTimePeriod = requireInt32('SWAK_BP openAndFill', 'timePeriod', optInTimePeriod);
  optInDelta = requireReal('SWAK_BP openAndFill', 'delta', optInDelta);
  requireArgument('SWAK_BP openAndFill', 'real', inReal);
  requireHistory('SWAK_BP openAndFill', inReal.length);
  const guardLookback = swakBpLookbackPositional(optInTimePeriod, optInDelta, core);
  if (guardLookback < 0) {
    throw rejectedParams('SWAK_BP openAndFill', PARAM_RANGES, optInTimePeriod, optInDelta);
  }
  const guardOutLen = openFillCount('SWAK_BP openAndFill', inReal.length, guardLookback);
  requireOutput('SWAK_BP openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('SWAK_BP openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('SWAK_BP openAndFill', 'outReal', 'real');
  }
  return swakBpOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, optInDelta, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live SWAK_BP stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link swakBp} there.
 *
 * @param real - Warm-up history. The series to filter; Ehlers' default is the bar
 *   midpoint `(H+L)/2`, which the caller passes as `TA_MEDPRICE` output.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SwakBpOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `swakBpLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a real parameter that is not a
 *   number or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { swakBpOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = swakBpOpen(data, { timePeriod: 20, delta: 0.1 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function swakBpOpen(real: ArrayLike<number>, options?: SwakBpOptions): SwakBpStream {
  const o = optionsOf('SWAK_BP open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInDelta = orDefault(o.delta, Core.REAL_DEFAULT);
  const core = coreOf('SWAK_BP open', o.core);
  return swakBpOpenPositional(real, optInTimePeriod, optInDelta, core);
}

/**
 * {@link swakBpOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link swakBp}.
 *
 * @param real - Warm-up history. The series to filter; Ehlers' default is the bar
 *   midpoint `(H+L)/2`, which the caller passes as `TA_MEDPRICE` output.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link SwakBpOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link swakBp} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `swakBpLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a real parameter that is not a
 *   number, a parameter outside its range or a buffer too short, of the wrong type
 *   or starting where another buffer or a `Float64Array` input starts, or, on a
 *   history long enough to open, one sharing memory with another buffer or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { swakBpOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = swakBpOpenAndFill(data, {
 *   timePeriod: 20,
 *   delta: 0.1,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function swakBpOpenAndFill(real: ArrayLike<number>, options?: SwakBpOptions): SwakBpResult & { stream: SwakBpStream } {
  const o = optionsOf('SWAK_BP openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInDelta = orDefault(o.delta, Core.REAL_DEFAULT);
  const core = coreOf('SWAK_BP openAndFill', o.core);
  const capacity = fillCapacity(swakBpLookbackPositional(optInTimePeriod, optInDelta, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = swakBpOpenAndFillPositional(real, optInTimePeriod, optInDelta, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
