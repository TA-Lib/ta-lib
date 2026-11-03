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

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireReal, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { VarStream, varLookback, varOpenAndFillInternal, var_ } from './var.ts';

/**
 * Number of leading input bars {@link stddev} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Window length (default 5; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param optInNbDev - Multiplier applied to the standard deviation (default 1;
 *   `Core.REAL_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function stddevLookback(optInTimePeriod: number, optInNbDev: number, core: Core = Core.DEFAULT): number {
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
  return varLookback(optInTimePeriod, optInNbDev, core);
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link stddev}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Window length (default 5; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param optInNbDev - Multiplier applied to the standard deviation (default 1;
 *   `Core.REAL_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function stddevDisplayShift(optInTimePeriod: number, optInNbDev: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (stddevLookback(optInTimePeriod, optInNbDev, core) < 0) {
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
  if (stddevLookback(optInTimePeriod, optInNbDev, core) > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  /* Calculate the variance. */
  const _xr0 = var_(startIdx, endIdx, inReal, optInTimePeriod, 1.0, outReal, core);
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
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link stddevLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - Series to measure dispersion of.
 * @param optInTimePeriod - Window length (default 5; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param optInNbDev - Multiplier applied to the standard deviation (default 1;
 *   `Core.REAL_DEFAULT` selects the default).
 * @param outReal - Standard deviation at each bar, scaled by optInNbDev. Must hold
 *   the values produced and never be empty; the input length always suffices.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The range written: `begIdx` is the first bar with a value, `count` how
 *   many were written.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for an argument that is not an int32, a
 *   real parameter that is not a number, a parameter outside its range, an array too
 *   short for the range, an empty output or an output sharing memory with a
 *   `Float64Array` input it is not. Checked before anything is written.
 *
 * @example
 * ```ts
 * import { stddev } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const { begIdx, count } = stddev(0, data.length - 1, data, 5, 1, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`var_`](https://ta-lib.org/functions/var)
 * @see [`bbands`](https://ta-lib.org/functions/bbands)
 * @see [`sma`](https://ta-lib.org/functions/sma)
 */
export function stddev(
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
  optInTimePeriod = requireInt32('STDDEV', 'optInTimePeriod', optInTimePeriod);
  optInNbDev = requireReal('STDDEV', 'optInNbDev', optInNbDev);
  requireIndexRange('STDDEV', startIdx, endIdx);
  const guardStart = clampedStart('STDDEV', startIdx, stddevLookback(optInTimePeriod, optInNbDev, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('STDDEV', 'inReal', inReal, guardInLen);
  requireLength('STDDEV', 'outReal', outReal, guardOutLen);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw failure('STDDEV', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = stddevImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('STDDEV', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
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

  /** @internal */
  constructor(core: Core) {
    this.core = core;
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
    return RetCode.OUT_OF_RANGE_START_INDEX;
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
  if (historyLen < stddevLookback(optInTimePeriod, optInNbDev, core) + 1) {
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
  if (stddevLookback(optInTimePeriod, optInNbDev, core) > endIdx) {
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
  const sp = new StddevStream(core);
  const retCode = stddevOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('STDDEV openAndFill', inReal.length, startIdx, stddevLookback(optInTimePeriod, optInNbDev, core));
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
  const sp = new StddevStream(core);
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
    throw insufficientHistory('STDDEV open', inReal.length, startIdx, stddevLookback(optInTimePeriod, optInNbDev, core));
  }
  throw streamFailure('STDDEV open', retCode);
}

/**
 * Opens a live STDDEV stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link stddev} there.
 *
 * @param inReal - Warm-up history. Series to measure dispersion of.
 * @param optInTimePeriod - Window length (default 5; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param optInNbDev - Multiplier applied to the standard deviation (default 1;
 *   `Core.REAL_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `stddevLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a real
 *   parameter that is not a number or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { stddevOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = stddevOpen(data, 5, 1);
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function stddevOpen(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInNbDev: number,
  core: Core = Core.DEFAULT,
): StddevStream {
  optInTimePeriod = requireInt32('STDDEV open', 'optInTimePeriod', optInTimePeriod);
  optInNbDev = requireReal('STDDEV open', 'optInNbDev', optInNbDev);
  requireArgument('STDDEV open', 'inReal', inReal);
  requireHistory('STDDEV open', inReal.length);
  return stddevOpenInternal(core, float64(inReal), 0, optInTimePeriod, optInNbDev);
}

/**
 * {@link stddevOpen} that also fills the outputs over the whole history in the
 * same pass, bit-identically to {@link stddev}. The range written is the stream's
 * `outRange()`.
 *
 * @param inReal - Warm-up history. Series to measure dispersion of.
 * @param optInTimePeriod - Window length (default 5; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param optInNbDev - Multiplier applied to the standard deviation (default 1;
 *   `Core.REAL_DEFAULT` selects the default).
 * @param outReal - Standard deviation at each bar, scaled by optInNbDev. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `stddevLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a real
 *   parameter that is not a number, a parameter outside its range or an output too
 *   short or starting where another output or a `Float64Array` input starts, or, on
 *   a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { stddevOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const stream = stddevOpenAndFill(data, 5, 1, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function stddevOpenAndFill(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInNbDev: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): StddevStream {
  optInTimePeriod = requireInt32('STDDEV openAndFill', 'optInTimePeriod', optInTimePeriod);
  optInNbDev = requireReal('STDDEV openAndFill', 'optInNbDev', optInNbDev);
  requireArgument('STDDEV openAndFill', 'inReal', inReal);
  requireHistory('STDDEV openAndFill', inReal.length);
  const guardOutLen = openFillCount('STDDEV openAndFill', inReal.length, stddevLookback(optInTimePeriod, optInNbDev, core));
  requireLength('STDDEV openAndFill', 'outReal', outReal, guardOutLen);
  const inRealF64 = float64(inReal);
  if ((outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset)) {
    throw streamFailure('STDDEV openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inRealF64))) {
    throw streamFailure('STDDEV openAndFill', RetCode.BAD_PARAM);
  }
  return stddevOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, optInNbDev, new MInteger(), new MInteger(), outReal);
}
