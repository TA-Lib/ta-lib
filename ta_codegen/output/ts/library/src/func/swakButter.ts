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
import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma, fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link swakButter} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param optInTimePeriod - Cutoff period; shorter keeps more of the fast motion,
 *   longer smooths harder and lags more (default 20; range 2 to 10000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function swakButterLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
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
   * no callee whose lookback could be inherited, so the function's own
   * unstable period is the whole of it.
   */
  return core.unstableCount(FuncUnstId.SWAK_BUTTER, (((((((10 + 3) * (optInTimePeriod + 2)) | 0) + 8) / 9) | 0)), (((((((19 + 3) * (optInTimePeriod + 2)) | 0) + 8) / 9) | 0)));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link swakButter}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Cutoff period; shorter keeps more of the fast motion,
 *   longer smooths harder and lags more (default 20; range 2 to 10000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function swakButterDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (swakButterLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function swakButterImpl(
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
  lookbackTotal = swakButterLookback(optInTimePeriod, core);
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
  /* The Butterworth row: the Gaussian's double real pole with two zeros added
   * at Nyquist, which is what the (1, 2, 1) numerator is. The quarter in c0
   * keeps the DC gain at 1 against that numerator's weight of 4.
   */
  om = 1.0 - a2p;
  c0 = a2p * a2p / 4.0;
  a1 = 2.0 * om;
  a2 = -(om * om);
  today = startIdx - lookbackTotal;
  /* Start in the steady state of a constant input equal to the first bar.
   * Unlike the Gaussian row this one has non-zero b1 and b2, so the two input
   * slots are carried in locals and never re-read from inReal: outReal may
   * alias inReal, and an aliased write would already have overwritten the
   * earlier bars this recurrence needs.
   */
  x0 = inReal[today];
  x1 = x0;
  x2 = x0;
  y1 = x0;
  y2 = x0;
  /* Skip the unstable period: run the recurrence but publish nothing. */
  i = lookbackTotal;
  while (i !== 0) {
    x0 = inReal[today];
    let _fma0: number;
    {
      const _a: number = 2.0, _b: number = x1, _c: number = x0;
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
    let _fma1: number;
    {
      const _a: number = a2, _b: number = y2, _c: number = c0 * (_fma0 + x2);
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
    {
      const _a: number = a1, _b: number = y1, _c: number = _fma1;
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
    let _fma3: number;
    {
      const _a: number = 2.0, _b: number = x1, _c: number = x0;
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
    let _fma4: number;
    {
      const _a: number = a2, _b: number = y2, _c: number = c0 * (_fma3 + x2);
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
    {
      const _a: number = a1, _b: number = y1, _c: number = _fma4;
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

/**
 * The Butterworth row of John Ehlers' Swiss Army Knife filter: the Gaussian row's
 * double real pole with two zeros added at Nyquist, giving a two-pole low-pass
 * that cuts the shortest cycles harder than the Gaussian does while keeping the
 * same pole placement. Read it as a smoothed price line. Its DC gain is 1, so a
 * flat market returns the price itself and the line sits on the chart with price.
 * The two Nyquist zeros are what separate it from `TA_SWAK_GAUSS`: a bar-to-bar
 * alternation, the fastest motion a sampled series can carry, is removed outright
 * rather than merely attenuated, which is why the numerator weights three
 * consecutive bars 1, 2, 1. The name keeps the `SWAK` prefix on purpose. This is
 * the Swiss Army Knife's Butterworth row, not the Butterworth filters of Ehlers'
 * separate article, and not the SuperSmoother, which is a third two-pole low-pass
 * with complex poles.
 *
 * Formula and more info at
 * [ta-lib.org/functions/swak_butter](https://ta-lib.org/functions/swak_butter).
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link swakButterLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - The series to filter; Ehlers' default is the bar midpoint
 *   `(H+L)/2`, which the caller passes as `TA_MEDPRICE` output.
 * @param optInTimePeriod - Cutoff period; shorter keeps more of the fast motion,
 *   longer smooths harder and lags more (default 20; range 2 to 10000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - The filtered line, on the same scale as the input. Must hold
 *   the values produced and never be empty; the input length always suffices.
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
 * import { swakButter } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const { begIdx, count } = swakButter(0, data.length - 1, data, 20, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`swakGauss`](https://ta-lib.org/functions/swak_gauss)
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`medprice`](https://ta-lib.org/functions/medprice)
 */
export function swakButter(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('SWAK_BUTTER', 'startIdx', startIdx);
  endIdx = requireInt32('SWAK_BUTTER', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('SWAK_BUTTER', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('SWAK_BUTTER', startIdx, endIdx);
  const guardStart = clampedStart('SWAK_BUTTER', startIdx, swakButterLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('SWAK_BUTTER', 'inReal', inReal, guardInLen);
  requireOutput('SWAK_BUTTER', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw failure('SWAK_BUTTER', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = swakButterImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('SWAK_BUTTER', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live SWAK_BUTTER stream: one value per closed bar, bit-identical to
 * {@link swakButter} over the same series. Open it with {@link swakButterOpen} or
 * {@link swakButterOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/swak_butter](https://ta-lib.org/functions/swak_butter).
 */
export class SwakButterStream {
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
  static of(core: Core): SwakButterStream {
    return new SwakButterStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link swakButter} reports them over the same bars. Each accepted `update` and
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
      throw failure('SWAK_BUTTER advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw failure('SWAK_BUTTER update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('SWAK_BUTTER update', 'inReal');
    }
    swakButterStepImpl(this, inReal);
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
      throw nonFiniteBar('SWAK_BUTTER peek', 'inReal');
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
    let _fma0: number;
    {
      const _a: number = 2.0, _b: number = x1, _c: number = x0;
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
    let _fma1: number;
    {
      const _a: number = sp.a2, _b: number = y2, _c: number = sp.c0 * (_fma0 + x2);
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
    {
      const _a: number = sp.a1, _b: number = y1, _c: number = _fma1;
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
  clone(): SwakButterStream {
    const c = new SwakButterStream(this.core);
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

function swakButterStepImpl(sp: SwakButterStream, inReal: number): void {
  let x0 = 0;
  let y = 0;
  /* a1*y1 stays the outermost term. y1 is the newest output, so the bar
   * rate is the latency of whatever y1 crosses to become y: outermost, that
   * is one fused step. Nested inside, it is three, and every backend's last
   * bit moves with it.
   */
  x0 = inReal;
  let _fma3: number;
  {
    const _a: number = 2.0, _b: number = sp.x1, _c: number = x0;
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
  let _fma4: number;
  {
    const _a: number = sp.a2, _b: number = sp.y2, _c: number = sp.c0 * (_fma3 + sp.x2);
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
  {
    const _a: number = sp.a1, _b: number = sp.y1, _c: number = _fma4;
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
  sp.x2 = sp.x1;
  sp.x1 = x0;
  sp.y2 = sp.y1;
  sp.y1 = y;
  sp.cur_outReal = y;
}

function swakButterOpenImpl(
  sp: SwakButterStream,
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
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 10000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = swakButterLookback(optInTimePeriod, core);
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
  /* The Butterworth row: the Gaussian's double real pole with two zeros added
   * at Nyquist, which is what the (1, 2, 1) numerator is. The quarter in c0
   * keeps the DC gain at 1 against that numerator's weight of 4.
   */
  om = 1.0 - a2p;
  c0 = a2p * a2p / 4.0;
  a1 = 2.0 * om;
  a2 = -(om * om);
  today = startIdx - lookbackTotal;
  /* Start in the steady state of a constant input equal to the first bar.
   * Unlike the Gaussian row this one has non-zero b1 and b2, so the two input
   * slots are carried in locals and never re-read from inReal: outReal may
   * alias inReal, and an aliased write would already have overwritten the
   * earlier bars this recurrence needs.
   */
  x0 = inReal[today];
  x1 = x0;
  x2 = x0;
  y1 = x0;
  y2 = x0;
  /* Skip the unstable period: run the recurrence but publish nothing. */
  i = lookbackTotal;
  while (i !== 0) {
    x0 = inReal[today];
    let _fma6: number;
    {
      const _a: number = 2.0, _b: number = x1, _c: number = x0;
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
    let _fma7: number;
    {
      const _a: number = a2, _b: number = y2, _c: number = c0 * (_fma6 + x2);
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
    {
      const _a: number = a1, _b: number = y1, _c: number = _fma7;
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
    let _fma9: number;
    {
      const _a: number = 2.0, _b: number = x1, _c: number = x0;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      _fma9 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    let _fma10: number;
    {
      const _a: number = a2, _b: number = y2, _c: number = c0 * (_fma9 + x2);
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      _fma10 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = a1, _b: number = y1, _c: number = _fma10;
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
export function swakButterOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): SwakButterStream {
  const sp = SwakButterStream.of(core);
  const retCode = swakButterOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SWAK_BUTTER openAndFill', inReal.length, startIdx, swakButterLookback(optInTimePeriod, core));
  }
  throw streamFailure('SWAK_BUTTER openAndFill', retCode);
}

/** @internal */
export function swakButterOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): SwakButterStream {
  const sp = SwakButterStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = swakButterOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SWAK_BUTTER open', inReal.length, startIdx, swakButterLookback(optInTimePeriod, core));
  }
  throw streamFailure('SWAK_BUTTER open', retCode);
}

/**
 * Opens a live SWAK_BUTTER stream over the warm-up history: `value()` starts at
 * the last history bar, bit-identical to {@link swakButter} there.
 *
 * @param inReal - Warm-up history. The series to filter; Ehlers' default is the
 *   bar midpoint `(H+L)/2`, which the caller passes as `TA_MEDPRICE` output.
 * @param optInTimePeriod - Cutoff period; shorter keeps more of the fast motion,
 *   longer smooths harder and lags more (default 20; range 2 to 10000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `swakButterLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an
 *   empty one, and `TA_BAD_PARAM` for an integer parameter that is not an int32 or a
 *   parameter outside its range.
 *
 * @example
 * ```ts
 * import { swakButterOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = swakButterOpen(data, 20);
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function swakButterOpen(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): SwakButterStream {
  optInTimePeriod = requireInt32('SWAK_BUTTER open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('SWAK_BUTTER open', 'inReal', inReal);
  requireHistory('SWAK_BUTTER open', inReal.length);
  return swakButterOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/**
 * {@link swakButterOpen} that also fills the outputs over the whole history in the
 * same pass, bit-identically to {@link swakButter}. The range written is the
 * stream's `outRange()`.
 *
 * @param inReal - Warm-up history. The series to filter; Ehlers' default is the
 *   bar midpoint `(H+L)/2`, which the caller passes as `TA_MEDPRICE` output.
 * @param optInTimePeriod - Cutoff period; shorter keeps more of the fast motion,
 *   longer smooths harder and lags more (default 20; range 2 to 10000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - The filtered line, on the same scale as the input. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `swakButterLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an
 *   empty one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or an output too short or starting where another
 *   output or a `Float64Array` input starts, or, on a history long enough to open,
 *   one sharing memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { swakButterOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const stream = swakButterOpenAndFill(data, 20, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function swakButterOpenAndFill(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): SwakButterStream {
  optInTimePeriod = requireInt32('SWAK_BUTTER openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('SWAK_BUTTER openAndFill', 'inReal', inReal);
  requireHistory('SWAK_BUTTER openAndFill', inReal.length);
  const guardOutLen = openFillCount('SWAK_BUTTER openAndFill', inReal.length, swakButterLookback(optInTimePeriod, core));
  requireOutput('SWAK_BUTTER openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if ((outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset)) {
    throw streamFailure('SWAK_BUTTER openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inRealF64))) {
    throw streamFailure('SWAK_BUTTER openAndFill', RetCode.BAD_PARAM);
  }
  return swakButterOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}
