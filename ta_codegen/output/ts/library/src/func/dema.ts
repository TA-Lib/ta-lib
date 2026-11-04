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
 *  010102 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  070526 MF,CC  Speed optimization: compute both EMA in a single
 *                lockstep pass (bit-exact, no temporary buffers).
 *  080926 MF,CC  Explicit no-smoothing copy at a period of 1.
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma, fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { emaLookback } from './ema.ts';

/**
 * Number of leading input bars {@link dema} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Smoothing period for both EMA passes (default 30; range
 *   1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function demaLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  /* Get lookback for one EMA.
   * Multiply by two (because double smoothing).
   */
  return ((emaLookback(optInTimePeriod, core) * 2) | 0);
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link dema}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Smoothing period for both EMA passes (default 30; range
 *   1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function demaDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (demaLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function demaImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let prevEMA1 = 0;
  let prevEMA2 = 0;
  let tempReal = 0;
  let optInK_1 = 0;
  let emaBeta = 0;
  let i = 0;
  let today = 0;
  let outIdx = 0;
  let lookbackEMA = 0;
  let lookbackTotal = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  /* For an explanation of this function, please read
   *
   * Stocks & Commodities V. 12:1 (11-19):
   *   Smoothing Data With Faster Moving Averages
   * Stocks & Commodities V. 12:2 (72-80):
   *   Smoothing Data With Less Lag
   *
   * Both magazine articles written by Patrick G. Mulloy
   *
   * Essentially, a DEMA of time serie 't' is:
   *   EMA2 = EMA(EMA(t,period),period)
   *   DEMA = 2*EMA(t,period)- EMA2
   *
   * DEMA offers a moving average with less lags then the
   * traditional EMA.
   *
   * Do not confuse a DEMA with the EMA2. Both are called
   * "Double EMA" in the litterature, but EMA2 is a simple
   * EMA of an EMA, while DEMA is a compostie of a single
   * EMA with EMA2.
   *
   * TEMA is very similar (and from the same author).
   */
  /* Will change only on success. */
  outNBElement.value = 0;
  outBegIdx.value = 0;
  /* Adjust startIdx to account for the lookback period. */
  lookbackEMA = emaLookback(optInTimePeriod, core);
  lookbackTotal = ((lookbackEMA * 2) | 0);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  /* No smoothing at period of 1: the output is a copy of the input
   * (same convention as TA_MA for every MAType). Explicit and separate
   * from TA_EMA's own copy because the two EMA below are inlined here,
   * not delegated -- at period 1 they run at a k of 1.0 and a beta of
   * 0.0, which does not keep the sign of a -0.0 input. The unstable
   * period still delays the first output, and at twice EMA's rate: TA_MA
   * reports lookback 0 at period 1, so the two disagree on alignment when
   * it is non-zero.
   */
  if (optInTimePeriod === 1) {
    outBegIdx.value = startIdx;
    outIdx = 0;
    today = startIdx;
    while (today <= endIdx) {
      outReal[outIdx++] = inReal[today++];
    }
    outNBElement.value = outIdx;
    return RetCode.SUCCESS;
  }
  /* Both EMA are computed in a single lockstep pass: each new
   * EMA1 value is immediately fed into EMA2. No temporary
   * buffers are needed.
   *
   * The arithmetic order below is the bit-exactness contract
   * (do not reorder or fuse operations):
   *  - EMA recursion: k*x + beta*prev, with ema.c's k and beta.
   *  - Each EMA is seeded with the sum of its first 'period'
   *    inputs, accumulated from 0.0 in input order (0.0+x is
   *    not x for x=-0.0), divided by the period.
   *
   * In-place (inReal == outReal) is supported: outReal[outIdx]
   * is written only after inReal[startIdx+outIdx] was read.
   */
  emaBeta = (optInTimePeriod - 1) / (optInTimePeriod + 1);
  optInK_1 = 1.0 - emaBeta;
  if (emaBeta < 0.5) {
    emaBeta = 1.0 - optInK_1;
  }
  /* Seed EMA1 with a simple average of the first
   * 'period' price bars.
   */
  today = startIdx - lookbackTotal;
  i = optInTimePeriod;
  tempReal = 0.0;
  while (i-- > 0) {
    tempReal += inReal[today++];
  }
  prevEMA1 = tempReal / optInTimePeriod;
  /* Advance EMA1 alone through its unstable period, up to
   * the bar where EMA2 seeding begins.
   */
  while (today <= startIdx - lookbackEMA) {
    {
      const _a: number = emaBeta, _b: number = prevEMA1, _c: number = optInK_1 * inReal[today++];
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevEMA1 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
  }
  /* Seed EMA2 with a simple average of the first 'period'
   * EMA1 values, accumulated as EMA1 produces them.
   */
  tempReal = 0.0;
  tempReal += prevEMA1;
  i = optInTimePeriod - 1;
  while (i-- > 0) {
    {
      const _a: number = emaBeta, _b: number = prevEMA1, _c: number = optInK_1 * inReal[today++];
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevEMA1 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    tempReal += prevEMA1;
  }
  prevEMA2 = tempReal / optInTimePeriod;
  /* Advance both EMA in lockstep through the unstable period
   * of EMA2, up to the first output bar.
   */
  while (today <= startIdx) {
    {
      const _a: number = emaBeta, _b: number = prevEMA1, _c: number = optInK_1 * inReal[today++];
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevEMA1 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = emaBeta, _b: number = prevEMA2, _c: number = optInK_1 * prevEMA1;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevEMA2 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
  }
  /* Stable zone: keep advancing both EMA in lockstep and
   * write the DEMA into the output.
   */
  outReal[0] = 2.0 * prevEMA1 - prevEMA2;
  outIdx = 1;
  while (today <= endIdx) {
    {
      const _a: number = emaBeta, _b: number = prevEMA1, _c: number = optInK_1 * inReal[today++];
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevEMA1 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = emaBeta, _b: number = prevEMA2, _c: number = optInK_1 * prevEMA1;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevEMA2 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    outReal[outIdx++] = 2.0 * prevEMA1 - prevEMA2;
  }
  /* Succeed. Indicate where the output starts relative to
   * the caller input.
   */
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * Double Exponential Moving Average: an EMA combined with an EMA-of-EMA to reduce
 * lag versus a plain EMA. Overlap Studies overlay on price.
 *
 * Formula and more info at
 * [ta-lib.org/functions/dema](https://ta-lib.org/functions/dema).
 *
 * **Notes**
 * - A period of 1 performs no smoothing: the output is a copy of the input.
 *   Allowed since 0.6.5 (issues #48/#59).
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link demaLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - Source series (typically price)
 * @param optInTimePeriod - Smoothing period for both EMA passes (default 30; range
 *   1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - DEMA line. Must hold the values produced and never be empty;
 *   the input length always suffices.
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
 * import { dema } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const { begIdx, count } = dema(0, data.length - 1, data, 30, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`tema`](https://ta-lib.org/functions/tema)
 * @see [`ma`](https://ta-lib.org/functions/ma)
 */
export function dema(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('DEMA', 'startIdx', startIdx);
  endIdx = requireInt32('DEMA', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('DEMA', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('DEMA', startIdx, endIdx);
  const guardStart = clampedStart('DEMA', startIdx, demaLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('DEMA', 'inReal', inReal, guardInLen);
  requireOutput('DEMA', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw failure('DEMA', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = demaImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('DEMA', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live DEMA stream: one value per closed bar, bit-identical to {@link dema} over
 * the same series. Open it with {@link demaOpen} or {@link demaOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/dema](https://ta-lib.org/functions/dema).
 */
export class DemaStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  prevEMA1: number = 0;
  /** @internal */
  prevEMA2: number = 0;
  /** @internal */
  optInK_1: number = 0;
  /** @internal */
  emaBeta: number = 0;
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
  static of(core: Core): DemaStream {
    return new DemaStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link dema} reports them over the same bars. Each accepted `update` and each
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
      throw failure('DEMA advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: Source series (typically price)
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('DEMA update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('DEMA update', 'inReal');
    }
    demaStepImpl(this, inReal);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Source series (typically price)
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number): number {
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('DEMA peek', 'inReal');
    }
    const sp = this;
    let cur_outReal = 0;
    let prevEMA1 = sp.prevEMA1;
    let prevEMA2 = sp.prevEMA2;
    if (sp.optInTimePeriod === 1) {
      cur_outReal = inReal;
      return cur_outReal;
    }
    prevEMA1 = fma(sp.emaBeta, prevEMA1, sp.optInK_1 * inReal);
    prevEMA2 = fma(sp.emaBeta, prevEMA2, sp.optInK_1 * prevEMA1);
    cur_outReal = 2.0 * prevEMA1 - prevEMA2;
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
  clone(): DemaStream {
    const c = new DemaStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.prevEMA1 = this.prevEMA1;
    c.prevEMA2 = this.prevEMA2;
    c.optInK_1 = this.optInK_1;
    c.emaBeta = this.emaBeta;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function demaStepImpl(sp: DemaStream, inReal: number): void {
  if (sp.optInTimePeriod === 1) {
    sp.cur_outReal = inReal;
    return;
  }
  sp.prevEMA1 = fma(sp.emaBeta, sp.prevEMA1, sp.optInK_1 * inReal);
  sp.prevEMA2 = fma(sp.emaBeta, sp.prevEMA2, sp.optInK_1 * sp.prevEMA1);
  sp.cur_outReal = 2.0 * sp.prevEMA1 - sp.prevEMA2;
}

function demaOpenImpl(
  sp: DemaStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let prevEMA1 = 0;
  let prevEMA2 = 0;
  let tempReal = 0;
  let optInK_1 = 0;
  let emaBeta = 0;
  let i = 0;
  let today = 0;
  let outIdx = 0;
  let lookbackEMA = 0;
  let lookbackTotal = 0;
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
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (optInTimePeriod === 1) {
    let fillLb = demaLookback(optInTimePeriod, core);
    if (startIdx > fillLb) {
      fillLb = startIdx;
    }
    if (historyLen < fillLb + 1) {
      return RetCode.INSUFFICIENT_HISTORY;
    }
    sp.optInTimePeriod = optInTimePeriod;
    sp.prevEMA1 = 0;
    sp.prevEMA2 = 0;
    sp.optInK_1 = 0;
    sp.emaBeta = 0;
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
  /* For an explanation of this function, please read
   *
   * Stocks & Commodities V. 12:1 (11-19):
   *   Smoothing Data With Faster Moving Averages
   * Stocks & Commodities V. 12:2 (72-80):
   *   Smoothing Data With Less Lag
   *
   * Both magazine articles written by Patrick G. Mulloy
   *
   * Essentially, a DEMA of time serie 't' is:
   *   EMA2 = EMA(EMA(t,period),period)
   *   DEMA = 2*EMA(t,period)- EMA2
   *
   * DEMA offers a moving average with less lags then the
   * traditional EMA.
   *
   * Do not confuse a DEMA with the EMA2. Both are called
   * "Double EMA" in the litterature, but EMA2 is a simple
   * EMA of an EMA, while DEMA is a compostie of a single
   * EMA with EMA2.
   *
   * TEMA is very similar (and from the same author).
   */
  /* Will change only on success. */
  outNBElement.value = 0;
  outBegIdx.value = 0;
  /* Adjust startIdx to account for the lookback period. */
  lookbackEMA = emaLookback(optInTimePeriod, core);
  lookbackTotal = ((lookbackEMA * 2) | 0);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Both EMA are computed in a single lockstep pass: each new
   * EMA1 value is immediately fed into EMA2. No temporary
   * buffers are needed.
   *
   * The arithmetic order below is the bit-exactness contract
   * (do not reorder or fuse operations):
   *  - EMA recursion: k*x + beta*prev, with ema.c's k and beta.
   *  - Each EMA is seeded with the sum of its first 'period'
   *    inputs, accumulated from 0.0 in input order (0.0+x is
   *    not x for x=-0.0), divided by the period.
   *
   * In-place (inReal == outReal) is supported: outReal[outIdx]
   * is written only after inReal[startIdx+outIdx] was read.
   */
  emaBeta = (optInTimePeriod - 1) / (optInTimePeriod + 1);
  optInK_1 = 1.0 - emaBeta;
  if (emaBeta < 0.5) {
    emaBeta = 1.0 - optInK_1;
  }
  /* Seed EMA1 with a simple average of the first
   * 'period' price bars.
   */
  today = startIdx - lookbackTotal;
  i = optInTimePeriod;
  tempReal = 0.0;
  while (i-- > 0) {
    tempReal += inReal[today++];
  }
  prevEMA1 = tempReal / optInTimePeriod;
  /* Advance EMA1 alone through its unstable period, up to
   * the bar where EMA2 seeding begins.
   */
  while (today <= startIdx - lookbackEMA) {
    {
      const _a: number = emaBeta, _b: number = prevEMA1, _c: number = optInK_1 * inReal[today++];
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevEMA1 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
  }
  /* Seed EMA2 with a simple average of the first 'period'
   * EMA1 values, accumulated as EMA1 produces them.
   */
  tempReal = 0.0;
  tempReal += prevEMA1;
  i = optInTimePeriod - 1;
  while (i-- > 0) {
    {
      const _a: number = emaBeta, _b: number = prevEMA1, _c: number = optInK_1 * inReal[today++];
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevEMA1 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    tempReal += prevEMA1;
  }
  prevEMA2 = tempReal / optInTimePeriod;
  /* Advance both EMA in lockstep through the unstable period
   * of EMA2, up to the first output bar.
   */
  while (today <= startIdx) {
    {
      const _a: number = emaBeta, _b: number = prevEMA1, _c: number = optInK_1 * inReal[today++];
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevEMA1 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = emaBeta, _b: number = prevEMA2, _c: number = optInK_1 * prevEMA1;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevEMA2 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
  }
  /* Stable zone: keep advancing both EMA in lockstep and
   * write the DEMA into the output.
   */
  outReal[((0 * outStride) | 0)] = 2.0 * prevEMA1 - prevEMA2;
  outIdx = 1;
  while (today <= endIdx) {
    {
      const _a: number = emaBeta, _b: number = prevEMA1, _c: number = optInK_1 * inReal[today++];
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevEMA1 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = emaBeta, _b: number = prevEMA2, _c: number = optInK_1 * prevEMA1;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevEMA2 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    outReal[((outIdx++ * outStride) | 0)] = 2.0 * prevEMA1 - prevEMA2;
  }
  /* Succeed. Indicate where the output starts relative to
   * the caller input.
   */
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  sp.optInTimePeriod = optInTimePeriod;
  sp.prevEMA1 = prevEMA1;
  sp.prevEMA2 = prevEMA2;
  sp.optInK_1 = optInK_1;
  sp.emaBeta = emaBeta;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function demaOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): DemaStream {
  const sp = DemaStream.of(core);
  const retCode = demaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('DEMA openAndFill', inReal.length, startIdx, demaLookback(optInTimePeriod, core));
  }
  throw streamFailure('DEMA openAndFill', retCode);
}

/** @internal */
export function demaOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): DemaStream {
  const sp = DemaStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = demaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('DEMA open', inReal.length, startIdx, demaLookback(optInTimePeriod, core));
  }
  throw streamFailure('DEMA open', retCode);
}

/**
 * Opens a live DEMA stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link dema} there.
 *
 * @param inReal - Warm-up history. Source series (typically price)
 * @param optInTimePeriod - Smoothing period for both EMA passes (default 30; range
 *   1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `demaLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { demaOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = demaOpen(data, 30);
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function demaOpen(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): DemaStream {
  optInTimePeriod = requireInt32('DEMA open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('DEMA open', 'inReal', inReal);
  requireHistory('DEMA open', inReal.length);
  return demaOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/**
 * {@link demaOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link dema}. The range written is the stream's
 * `outRange()`.
 *
 * @param inReal - Warm-up history. Source series (typically price)
 * @param optInTimePeriod - Smoothing period for both EMA passes (default 30; range
 *   1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - DEMA line. Must hold `historyLen - lookback` values and share
 *   no memory with an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `demaLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   an integer parameter that is not an int32, a parameter outside its range or an
 *   output too short or starting where another output or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { demaOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const stream = demaOpenAndFill(data, 30, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function demaOpenAndFill(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): DemaStream {
  optInTimePeriod = requireInt32('DEMA openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('DEMA openAndFill', 'inReal', inReal);
  requireHistory('DEMA openAndFill', inReal.length);
  const guardOutLen = openFillCount('DEMA openAndFill', inReal.length, demaLookback(optInTimePeriod, core));
  requireOutput('DEMA openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if ((outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset)) {
    throw streamFailure('DEMA openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inRealF64))) {
    throw streamFailure('DEMA openAndFill', RetCode.BAD_PARAM);
  }
  return demaOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/** @internal */
export function demaStepTape(sp: DemaStream, _tape: Float64Array, _tapeBase: number, _tapeMask: number, inReal: number): number {
  demaStepImpl(sp, inReal);
  sp.outRangeCount++;
  return sp.cur_outReal;
}

/** @internal */
export function demaPeekTape(sp: DemaStream, _tape: Float64Array, _tapeBase: number, _tapeMask: number, inReal: number): number {
  return sp.peek(inReal);
}

/** @internal */
export function demaTapeDetach(_sp: DemaStream): number {
  return 0;
}
