/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  JV       Jesus Viver <324122@cienz.unizar.es>
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   Template creation.
 *  100502 JV   Speed optimization of the algorithm
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  090404 MF   Fix #978056. Trap sqrt with negative zero values.
 *  082326 MF,CC #243 the sqrt trap moves to var's scale-relative floor.
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireReal, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { VarStream, varLookbackPositional, varOpenAndFillInternal, varPositional } from './var.ts';

/** @internal */
export function stddevLookbackPositional(optInTimePeriod: number, optInNbDev: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (typeof optInNbDev !== 'number') {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 5;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  if (optInNbDev === Core.REAL_DEFAULT) {
    optInNbDev = 1.0;
  } else if (!(optInNbDev >= Core.REAL_MIN && optInNbDev <= Core.REAL_MAX)) {
    return -1;
  }
  /* Lookback is driven by the variance. */
  return varLookbackPositional(optInTimePeriod, optInNbDev, core);
}

/** @internal */
export function stddevDisplayShiftPositional(optInTimePeriod: number, optInNbDev: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (stddevLookbackPositional(optInTimePeriod, optInNbDev, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function stddevImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  optInNbDev: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let i = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 5;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInNbDev === Core.REAL_DEFAULT) {
    optInNbDev = 1.0;
  } else if (!(optInNbDev >= Core.REAL_MIN && optInNbDev <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  /* Nothing to produce: the range ends before the lookback. Return before
   * touching anything.
   *
   * Same shape as the guard in apo and bbands: the variance below runs on the
   * same range and its lookback IS stddev's, so it declines and yields 0,0
   * without reading. Observably identical, but it makes "a range that ends before
   * the lookback reads nothing" true of stddev itself rather than only of var.
   * Pinned by the zero-length no-I/O probe over every guarded core.
   */
  if (stddevLookbackPositional(optInTimePeriod, optInNbDev, core) > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  /* Calculate the variance. */
  const _xr0 = varPositional(startIdx, endIdx, inReal, optInTimePeriod, 1.0, outReal, core);
  outBegIdx.value = _xr0.begIdx;
  outNBElement.value = _xr0.count;
  /* Calculate the square root of each variance, this
   * is the standard deviation.
   *
   * Multiply also by the ratio specified.
   *
   * Unconditional. var owns the sign: it returns a non-negative variance,
   * exactly 0 on a window of identical values (var.c). What used to
   * stand here instead - zero the output wherever the variance fell under
   * TA_EPSILON - compared a SQUARED quantity to a fixed 1e-14, which is a cliff
   * at a price level rather than a noise floor: a $100.00 instrument quoted in
   * 1e-8 ticks has a variance around 1e-16 and came back as exactly 0 on every
   * bar, with TA_SUCCESS and nothing to say it had been suppressed (#243).
   * Dropping it also leaves a pure map, which the branch had kept sqrt out of.
   */
  if (optInNbDev !== 1.0) {
    for (i = 0; i < outNBElement.value; i += 1) {
      outReal[i] = Math.sqrt(outReal[i]) * optInNbDev;
    }
  } else {
    for (i = 0; i < outNBElement.value; i += 1) {
      outReal[i] = Math.sqrt(outReal[i]);
    }
  }
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['nbDev', Core.REAL_DEFAULT, Core.REAL_MIN, Core.REAL_MAX, 'in -3e37..3e37']] as const;

/** @internal */
export function stddevPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInNbDev: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('STDDEV', 'startIdx', startIdx);
  endIdx = requireInt32('STDDEV', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('STDDEV', 'timePeriod', optInTimePeriod);
  optInNbDev = requireReal('STDDEV', 'nbDev', optInNbDev);
  requireIndexRange('STDDEV', startIdx, endIdx);
  const guardLookback = stddevLookbackPositional(optInTimePeriod, optInNbDev, core);
  if (guardLookback < 0) {
    throw rejectedParams('STDDEV', PARAM_RANGES, optInTimePeriod, optInNbDev);
  }
  const guardStart = clampedStart('STDDEV', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('STDDEV', 'real', inReal, guardInLen);
  requireOutput('STDDEV', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('STDDEV', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = stddevImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('STDDEV', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link stddev}, {@link stddevLookback}, {@link stddevDisplayShift},
 * {@link stddevOpen} and {@link stddevOpenAndFill}. Every key is optional: an
 * omitted or `undefined` key takes its default, and a call ignores the keys it has
 * no use for.
 */
export interface StddevOptions {
  /**
   * Window length (default 5; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
   * default).
   */
  timePeriod?: number | undefined;
  /**
   * Multiplier applied to the standard deviation (default 1; `Core.REAL_DEFAULT`
   * selects the default).
   */
  nbDev?: number | undefined;
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

/** What {@link stddev} returns, and {@link stddevOpenAndFill} with its stream. */
export interface StddevResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Standard deviation at each bar, scaled by optInNbDev. `values[k]` is the value
   * at input bar `begIdx + k`.
   */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link stddev} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link StddevOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function stddevLookback(options?: StddevOptions): number {
  const o = optionsOf('STDDEV lookback', options);
  return stddevLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.nbDev, Core.REAL_DEFAULT), coreOf('STDDEV lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link stddev}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link StddevOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function stddevDisplayShift(outputIdx: number, options?: StddevOptions): number {
  const o = optionsOf('STDDEV displayShift', options);
  return stddevDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.nbDev, Core.REAL_DEFAULT), outputIdx, coreOf('STDDEV displayShift', o.core));
}

/**
 * Rolling standard deviation of a series over a window, scaled by a deviations
 * multiplier. Delegates to VAR, then takes the square root.
 *
 * Formula and more info at
 * [ta-lib.org/functions/stddev](https://ta-lib.org/functions/stddev).
 *
 * **Notes**
 * - Uses population variance (divides by the period, not period minus one), so
 *   results differ slightly from the sample standard deviation used by some tools.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link stddevLookback} is a success with empty outputs.
 *
 * @param real - Series to measure dispersion of.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link StddevOptions}.
 * @returns `begIdx` and the values; see {@link StddevResult}. Each output is a
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
 * import { stddev } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = stddev(data, { timePeriod: 5, nbDev: 1 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`var_`](https://ta-lib.org/functions/var)
 * @see [`bbands`](https://ta-lib.org/functions/bbands)
 * @see [`sma`](https://ta-lib.org/functions/sma)
 */
export function stddev(real: ArrayLike<number>, options?: StddevOptions): StddevResult {
  const o = optionsOf('STDDEV', options);
  if (o.endIdx === undefined) {
    requireSeries('STDDEV', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInNbDev = orDefault(o.nbDev, Core.REAL_DEFAULT);
  const core = coreOf('STDDEV', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, stddevLookbackPositional(optInTimePeriod, optInNbDev, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = stddevPositional(startIdx, endIdx, real, optInTimePeriod, optInNbDev, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live STDDEV stream: one value per closed bar, bit-identical to {@link stddev}
 * over the same series. Open it with {@link stddevOpen} or
 * {@link stddevOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/stddev](https://ta-lib.org/functions/stddev).
 */
export class StddevStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  optInNbDev: number = 0;
  /** @internal */
  cur_outReal: number = 0;
  /** @internal */
  sub0!: VarStream;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): StddevStream {
    return new StddevStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link stddev} reports them over the same bars. Each accepted `update` and each
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
      throw failure('STDDEV advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: Series to measure dispersion of.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('STDDEV update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('STDDEV update', 'inReal');
    }
    stddevStepImpl(this, inReal);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Series to measure dispersion of.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number): number {
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('STDDEV peek', 'inReal');
    }
    const sp = this;
    let cur_outReal = 0;
    cur_outReal = sp.sub0.peek(inReal);
    if (sp.optInNbDev !== 1.0) {
      cur_outReal = Math.sqrt(cur_outReal) * sp.optInNbDev;
    } else {
      cur_outReal = Math.sqrt(cur_outReal);
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
  clone(): StddevStream {
    const c = new StddevStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.optInNbDev = this.optInNbDev;
    c.cur_outReal = this.cur_outReal;
    c.sub0 = this.sub0.clone();
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function stddevStepImpl(sp: StddevStream, inReal: number): void {
  let cur_outReal = 0;
  cur_outReal = sp.sub0.update(inReal);
  if (sp.optInNbDev !== 1.0) {
    cur_outReal = Math.sqrt(cur_outReal) * sp.optInNbDev;
  } else {
    cur_outReal = Math.sqrt(cur_outReal);
  }
  sp.cur_outReal = cur_outReal;
}

function stddevOpenImpl(
  sp: StddevStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInNbDev: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let i = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 5;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInNbDev === Core.REAL_DEFAULT) {
    optInNbDev = 1.0;
  } else if (!(optInNbDev >= Core.REAL_MIN && optInNbDev <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen < stddevLookbackPositional(optInTimePeriod, optInNbDev, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const sc_outReal = outStride === 1 ? outReal : new Float64Array(historyLen);
  /* Nothing to produce: the range ends before the lookback. Return before
   * touching anything.
   *
   * Same shape as the guard in apo and bbands: the variance below runs on the
   * same range and its lookback IS stddev's, so it declines and yields 0,0
   * without reading. Observably identical, but it makes "a range that ends before
   * the lookback reads nothing" true of stddev itself rather than only of var.
   * Pinned by the zero-length no-I/O probe over every guarded core.
   */
  if (stddevLookbackPositional(optInTimePeriod, optInNbDev, core) > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Calculate the variance. */
  const sub0 = varOpenAndFillInternal(core, inReal, startIdx, optInTimePeriod, 1.0, outBegIdx, outNBElement, sc_outReal);
  /* Calculate the square root of each variance, this
   * is the standard deviation.
   *
   * Multiply also by the ratio specified.
   *
   * Unconditional. var owns the sign: it returns a non-negative variance,
   * exactly 0 on a window of identical values (var.c). What used to
   * stand here instead - zero the output wherever the variance fell under
   * TA_EPSILON - compared a SQUARED quantity to a fixed 1e-14, which is a cliff
   * at a price level rather than a noise floor: a $100.00 instrument quoted in
   * 1e-8 ticks has a variance around 1e-16 and came back as exactly 0 on every
   * bar, with TA_SUCCESS and nothing to say it had been suppressed (#243).
   * Dropping it also leaves a pure map, which the branch had kept sqrt out of.
   */
  if (optInNbDev !== 1.0) {
    for (i = 0; i < outNBElement.value; i += 1) {
      sc_outReal[i] = Math.sqrt(sc_outReal[i]) * optInNbDev;
    }
  } else {
    for (i = 0; i < outNBElement.value; i += 1) {
      sc_outReal[i] = Math.sqrt(sc_outReal[i]);
    }
  }
  if (outNBElement.value < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.optInNbDev = optInNbDev;
  sp.sub0 = sub0;
  sp.cur_outReal = sc_outReal[outNBElement.value - 1];
  return RetCode.SUCCESS;
}

/** @internal */
export function stddevOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInNbDev: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): StddevStream {
  const sp = StddevStream.of(core);
  const retCode = stddevOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('STDDEV openAndFill', inReal.length, startIdx, stddevLookbackPositional(optInTimePeriod, optInNbDev, core));
  }
  if (retCode === RetCode.BAD_PARAM && stddevLookbackPositional(optInTimePeriod, optInNbDev, core) < 0) {
    throw rejectedParams('STDDEV openAndFill', PARAM_RANGES, optInTimePeriod, optInNbDev);
  }
  throw streamFailure('STDDEV openAndFill', retCode);
}

/** @internal */
export function stddevOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInNbDev: number,
): StddevStream {
  const sp = StddevStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = stddevOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('STDDEV open', inReal.length, startIdx, stddevLookbackPositional(optInTimePeriod, optInNbDev, core));
  }
  if (retCode === RetCode.BAD_PARAM && stddevLookbackPositional(optInTimePeriod, optInNbDev, core) < 0) {
    throw rejectedParams('STDDEV open', PARAM_RANGES, optInTimePeriod, optInNbDev);
  }
  throw streamFailure('STDDEV open', retCode);
}

/** @internal */
export function stddevOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInNbDev: number,
  core: Core = Core.DEFAULT,
): StddevStream {
  optInTimePeriod = requireInt32('STDDEV open', 'timePeriod', optInTimePeriod);
  optInNbDev = requireReal('STDDEV open', 'nbDev', optInNbDev);
  requireArgument('STDDEV open', 'real', inReal);
  requireHistory('STDDEV open', inReal.length);
  return stddevOpenInternal(core, float64(inReal), 0, optInTimePeriod, optInNbDev);
}

/** @internal */
export function stddevOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInNbDev: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): StddevStream {
  optInTimePeriod = requireInt32('STDDEV openAndFill', 'timePeriod', optInTimePeriod);
  optInNbDev = requireReal('STDDEV openAndFill', 'nbDev', optInNbDev);
  requireArgument('STDDEV openAndFill', 'real', inReal);
  requireHistory('STDDEV openAndFill', inReal.length);
  const guardLookback = stddevLookbackPositional(optInTimePeriod, optInNbDev, core);
  if (guardLookback < 0) {
    throw rejectedParams('STDDEV openAndFill', PARAM_RANGES, optInTimePeriod, optInNbDev);
  }
  const guardOutLen = openFillCount('STDDEV openAndFill', inReal.length, guardLookback);
  requireOutput('STDDEV openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('STDDEV openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('STDDEV openAndFill', 'outReal', 'real');
  }
  return stddevOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, optInNbDev, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live STDDEV stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link stddev} there.
 *
 * @param real - Warm-up history. Series to measure dispersion of.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link StddevOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `stddevLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a real parameter that is not a
 *   number or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { stddevOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = stddevOpen(data, { timePeriod: 5, nbDev: 1 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function stddevOpen(real: ArrayLike<number>, options?: StddevOptions): StddevStream {
  const o = optionsOf('STDDEV open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInNbDev = orDefault(o.nbDev, Core.REAL_DEFAULT);
  const core = coreOf('STDDEV open', o.core);
  return stddevOpenPositional(real, optInTimePeriod, optInNbDev, core);
}

/**
 * {@link stddevOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link stddev}.
 *
 * @param real - Warm-up history. Series to measure dispersion of.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link StddevOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link stddev} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `stddevLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a real parameter that is not a
 *   number, a parameter outside its range or a buffer too short, of the wrong type
 *   or starting where another buffer or a `Float64Array` input starts, or, on a
 *   history long enough to open, one sharing memory with another buffer or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { stddevOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = stddevOpenAndFill(data, {
 *   timePeriod: 5,
 *   nbDev: 1,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function stddevOpenAndFill(real: ArrayLike<number>, options?: StddevOptions): StddevResult & { stream: StddevStream } {
  const o = optionsOf('STDDEV openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInNbDev = orDefault(o.nbDev, Core.REAL_DEFAULT);
  const core = coreOf('STDDEV openAndFill', o.core);
  const capacity = fillCapacity(stddevLookbackPositional(optInTimePeriod, optInNbDev, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = stddevOpenAndFillPositional(real, optInTimePeriod, optInNbDev, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
