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

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma, fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { emaLookbackPositional } from './ema.ts';

/** @internal */
export function demaLookbackPositional(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
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
  return ((emaLookbackPositional(optInTimePeriod, core) * 2) | 0);
}

/** @internal */
export function demaDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (demaLookbackPositional(optInTimePeriod, core) < 0) {
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
  lookbackEMA = emaLookbackPositional(optInTimePeriod, core);
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

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000']] as const;

/** @internal */
export function demaPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('DEMA', 'startIdx', startIdx);
  endIdx = requireInt32('DEMA', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('DEMA', 'timePeriod', optInTimePeriod);
  requireIndexRange('DEMA', startIdx, endIdx);
  const guardLookback = demaLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('DEMA', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('DEMA', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('DEMA', 'real', inReal, guardInLen);
  requireOutput('DEMA', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('DEMA', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = demaImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('DEMA', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link dema}, {@link demaLookback}, {@link demaDisplayShift},
 * {@link demaOpen} and {@link demaOpenAndFill}. Every key is optional: an omitted
 * or `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface DemaOptions {
  /**
   * Smoothing period for both EMA passes (default 30; range 1 to 100000;
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

/** What {@link dema} returns, and {@link demaOpenAndFill} with its stream. */
export interface DemaResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /** DEMA line. `values[k]` is the value at input bar `begIdx + k`. */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link dema} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link DemaOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function demaLookback(options?: DemaOptions): number {
  const o = optionsOf('DEMA lookback', options);
  return demaLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('DEMA lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link dema}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link DemaOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function demaDisplayShift(outputIdx: number, options?: DemaOptions): number {
  const o = optionsOf('DEMA displayShift', options);
  return demaDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('DEMA displayShift', o.core));
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
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link demaLookback} is a success with empty outputs.
 *
 * @param real - Source series (typically price)
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link DemaOptions}.
 * @returns `begIdx` and the values; see {@link DemaResult}. Each output is a view
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
 * import { dema } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = dema(data, { timePeriod: 30 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`tema`](https://ta-lib.org/functions/tema)
 * @see [`ma`](https://ta-lib.org/functions/ma)
 */
export function dema(real: ArrayLike<number>, options?: DemaOptions): DemaResult {
  const o = optionsOf('DEMA', options);
  if (o.endIdx === undefined) {
    requireSeries('DEMA', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('DEMA', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, demaLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = demaPositional(startIdx, endIdx, real, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
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
      throw streamFailure('DEMA advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: Source series (typically price)
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('DEMA update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('DEMA update', 'real');
    }
    demaStepImpl(this, real);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: Source series (typically price)
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number): number {
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('DEMA peek', 'real');
    }
    const sp = this;
    const inReal = real;
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
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (optInTimePeriod === 1) {
    let fillLb = demaLookbackPositional(optInTimePeriod, core);
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
  lookbackEMA = emaLookbackPositional(optInTimePeriod, core);
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
    throw insufficientHistory('DEMA openAndFill', inReal.length, startIdx, demaLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && demaLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('DEMA openAndFill', PARAM_RANGES, optInTimePeriod);
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
    throw insufficientHistory('DEMA open', inReal.length, startIdx, demaLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && demaLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('DEMA open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('DEMA open', retCode);
}

/** @internal */
export function demaOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): DemaStream {
  optInTimePeriod = requireInt32('DEMA open', 'timePeriod', optInTimePeriod);
  requireArgument('DEMA open', 'real', inReal);
  requireHistory('DEMA open', 'real', inReal.length);
  return demaOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/** @internal */
export function demaOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): DemaStream {
  optInTimePeriod = requireInt32('DEMA openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('DEMA openAndFill', 'real', inReal);
  requireHistory('DEMA openAndFill', 'real', inReal.length);
  const guardLookback = demaLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('DEMA openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('DEMA openAndFill', inReal.length, guardLookback);
  requireOutput('DEMA openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('DEMA openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('DEMA openAndFill', 'outReal', 'real');
  }
  return demaOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live DEMA stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link dema} there.
 *
 * @param real - Warm-up history. Source series (typically price)
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link DemaOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `demaLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { demaOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = demaOpen(data, { timePeriod: 30 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function demaOpen(real: ArrayLike<number>, options?: DemaOptions): DemaStream {
  const o = optionsOf('DEMA open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('DEMA open', o.core);
  return demaOpenPositional(real, optInTimePeriod, core);
}

/**
 * {@link demaOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link dema}.
 *
 * @param real - Warm-up history. Source series (typically price)
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link DemaOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link dema} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `demaLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or a
 *   buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { demaOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = demaOpenAndFill(data, { timePeriod: 30 });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function demaOpenAndFill(real: ArrayLike<number>, options?: DemaOptions): DemaResult & { stream: DemaStream } {
  const o = optionsOf('DEMA openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('DEMA openAndFill', o.core);
  const capacity = fillCapacity(demaLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = demaOpenAndFillPositional(real, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
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
