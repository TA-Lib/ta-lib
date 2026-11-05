/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AB       Anatoliy Belsky
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090812 AB     Initial Version
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function avgdevLookbackPositional(optInTimePeriod: number, _core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  return optInTimePeriod - 1;
}

/** @internal */
export function avgdevDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (avgdevLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function avgdevImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let today = 0;
  let outIdx = 0;
  let lookback = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  lookback = optInTimePeriod - 1;
  if (startIdx < lookback) {
    startIdx = lookback;
  }
  today = startIdx;
  /* Make sure there is still something to evaluate. */
  if (today > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  /* Process the initial DM and TR */
  outBegIdx.value = today;
  outIdx = 0;
  while (today <= endIdx) {
    let todaySum = 0;
    let todayDev = 0;
    let i = 0;
    todaySum = 0.0;
    for (i = 0; i < optInTimePeriod; i += 1) {
      todaySum += inReal[today - i];
    }
    todayDev = 0.0;
    for (i = 0; i < optInTimePeriod; i += 1) {
      todayDev += Math.abs(inReal[today - i] - todaySum / optInTimePeriod);
    }
    outReal[outIdx] = todayDev / optInTimePeriod;
    outIdx += 1;
    today += 1;
  }
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000']] as const;

/** @internal */
export function avgdevPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('AVGDEV', 'startIdx', startIdx);
  endIdx = requireInt32('AVGDEV', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('AVGDEV', 'timePeriod', optInTimePeriod);
  requireIndexRange('AVGDEV', startIdx, endIdx);
  const guardLookback = avgdevLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('AVGDEV', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('AVGDEV', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('AVGDEV', 'real', inReal, guardInLen);
  requireOutput('AVGDEV', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('AVGDEV', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = avgdevImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('AVGDEV', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link avgdev}, {@link avgdevLookback}, {@link avgdevDisplayShift},
 * {@link avgdevOpen} and {@link avgdevOpenAndFill}. Every key is optional: an
 * omitted or `undefined` key takes its default, and a call ignores the keys it has
 * no use for.
 */
export interface AvgdevOptions {
  /**
   * Window length (default 14; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
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
  outReal?: Float64Array | undefined;
}

/** What {@link avgdev} returns, and {@link avgdevOpenAndFill} with its stream. */
export interface AvgdevResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * mean absolute deviation over the window. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link avgdev} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AvgdevOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function avgdevLookback(options?: AvgdevOptions): number {
  const o = optionsOf('AVGDEV lookback', options);
  return avgdevLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('AVGDEV lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link avgdev}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AvgdevOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function avgdevDisplayShift(outputIdx: number, options?: AvgdevOptions): number {
  const o = optionsOf('AVGDEV displayShift', options);
  return avgdevDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('AVGDEV displayShift', o.core));
}

/**
 * Rolling average absolute deviation of a series from its own simple moving
 * average over the last N periods. Measures dispersion around the window mean.
 * Higher values indicate greater spread; zero when all values in the window are
 * equal.
 *
 * Formula and more info at
 * [ta-lib.org/functions/avgdev](https://ta-lib.org/functions/avgdev).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link avgdevLookback} is a success with empty outputs.
 *
 * @param real - source series.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link AvgdevOptions}.
 * @returns `begIdx` and the values; see {@link AvgdevResult}. Each output is a
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
 * import { avgdev } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = avgdev(data, { timePeriod: 14 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`stddev`](https://ta-lib.org/functions/stddev)
 * @see [`var_`](https://ta-lib.org/functions/var)
 * @see [`sma`](https://ta-lib.org/functions/sma)
 */
export function avgdev(real: ArrayLike<number>, options?: AvgdevOptions): AvgdevResult {
  const o = optionsOf('AVGDEV', options);
  if (o.endIdx === undefined) {
    requireSeries('AVGDEV', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('AVGDEV', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, avgdevLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = avgdevPositional(startIdx, endIdx, real, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live AVGDEV stream: one value per closed bar, bit-identical to {@link avgdev}
 * over the same series. Open it with {@link avgdevOpen} or
 * {@link avgdevOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/avgdev](https://ta-lib.org/functions/avgdev).
 */
export class AvgdevStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  winPos_i: number = 0;
  /** @internal */
  winCap_i: number = 1;
  /** @internal */
  win_i_inReal: Float64Array = new Float64Array(1);
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
  static of(core: Core): AvgdevStream {
    return new AvgdevStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link avgdev} reports them over the same bars. Each accepted `update` and each
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
      throw failure('AVGDEV advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: source series.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('AVGDEV update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('AVGDEV update', 'inReal');
    }
    avgdevStepImpl(this, inReal);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: source series.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number): number {
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('AVGDEV peek', 'inReal');
    }
    const sp = this;
    let todaySum = 0;
    let todayDev = 0;
    let i = 0;
    let cur_outReal = 0;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    pkSlot0 = sp.winPos_i;
    pkVal0 = inReal;
    todaySum = 0.0;
    for (i = 0; i < sp.optInTimePeriod; i += 1) {
      todaySum += (((sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i) !== pkSlot0) ? sp.win_i_inReal[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i] : pkVal0;
    }
    todayDev = 0.0;
    for (i = 0; i < sp.optInTimePeriod; i += 1) {
      todayDev += Math.abs(((((sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i) !== pkSlot0) ? sp.win_i_inReal[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i] : pkVal0) - todaySum / sp.optInTimePeriod);
    }
    cur_outReal = todayDev / sp.optInTimePeriod;
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
  clone(): AvgdevStream {
    const c = new AvgdevStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.winPos_i = this.winPos_i;
    c.winCap_i = this.winCap_i;
    c.win_i_inReal = this.win_i_inReal.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function avgdevStepImpl(sp: AvgdevStream, inReal: number): void {
  let todaySum = 0;
  let todayDev = 0;
  let i = 0;
  sp.win_i_inReal[sp.winPos_i] = inReal;
  todaySum = 0.0;
  for (i = 0; i < sp.optInTimePeriod; i += 1) {
    todaySum += sp.win_i_inReal[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i];
  }
  todayDev = 0.0;
  for (i = 0; i < sp.optInTimePeriod; i += 1) {
    todayDev += Math.abs(sp.win_i_inReal[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i] - todaySum / sp.optInTimePeriod);
  }
  sp.cur_outReal = todayDev / sp.optInTimePeriod;
  sp.winPos_i = sp.winPos_i + 1;
  if (sp.winPos_i >= sp.winCap_i) {
    sp.winPos_i = 0;
  }
}

function avgdevOpenImpl(
  sp: AvgdevStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let today = 0;
  let outIdx = 0;
  let lookback = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  lookback = optInTimePeriod - 1;
  if (startIdx < lookback) {
    startIdx = lookback;
  }
  today = startIdx;
  /* Make sure there is still something to evaluate. */
  if (today > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Process the initial DM and TR */
  outBegIdx.value = today;
  outIdx = 0;
  while (today <= endIdx) {
    let todaySum = 0;
    let todayDev = 0;
    let i = 0;
    todaySum = 0.0;
    for (i = 0; i < optInTimePeriod; i += 1) {
      todaySum += inReal[today - i];
    }
    todayDev = 0.0;
    for (i = 0; i < optInTimePeriod; i += 1) {
      todayDev += Math.abs(inReal[today - i] - todaySum / optInTimePeriod);
    }
    outReal[((outIdx * outStride) | 0)] = todayDev / optInTimePeriod;
    outIdx += 1;
    today += 1;
  }
  outNBElement.value = outIdx;
  const cap_i = optInTimePeriod;
  if (cap_i < 1 || cap_i > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const capWin_i_inReal = inReal.slice(historyLen - cap_i, historyLen);
  sp.optInTimePeriod = optInTimePeriod;
  sp.winPos_i = 0;
  sp.winCap_i = cap_i;
  sp.win_i_inReal = capWin_i_inReal;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function avgdevOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): AvgdevStream {
  const sp = AvgdevStream.of(core);
  const retCode = avgdevOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('AVGDEV openAndFill', inReal.length, startIdx, avgdevLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && avgdevLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('AVGDEV openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('AVGDEV openAndFill', retCode);
}

/** @internal */
export function avgdevOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): AvgdevStream {
  const sp = AvgdevStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = avgdevOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('AVGDEV open', inReal.length, startIdx, avgdevLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && avgdevLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('AVGDEV open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('AVGDEV open', retCode);
}

/** @internal */
export function avgdevOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): AvgdevStream {
  optInTimePeriod = requireInt32('AVGDEV open', 'timePeriod', optInTimePeriod);
  requireArgument('AVGDEV open', 'real', inReal);
  requireHistory('AVGDEV open', inReal.length);
  return avgdevOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/** @internal */
export function avgdevOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): AvgdevStream {
  optInTimePeriod = requireInt32('AVGDEV openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('AVGDEV openAndFill', 'real', inReal);
  requireHistory('AVGDEV openAndFill', inReal.length);
  const guardLookback = avgdevLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('AVGDEV openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('AVGDEV openAndFill', inReal.length, guardLookback);
  requireOutput('AVGDEV openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('AVGDEV openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('AVGDEV openAndFill', 'outReal', 'real');
  }
  return avgdevOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live AVGDEV stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link avgdev} there.
 *
 * @param real - Warm-up history. source series.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AvgdevOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `avgdevLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32 or a parameter outside its
 *   range.
 *
 * @example
 * ```ts
 * import { avgdevOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = avgdevOpen(data, { timePeriod: 14 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function avgdevOpen(real: ArrayLike<number>, options?: AvgdevOptions): AvgdevStream {
  const o = optionsOf('AVGDEV open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('AVGDEV open', o.core);
  return avgdevOpenPositional(real, optInTimePeriod, core);
}

/**
 * {@link avgdevOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link avgdev}.
 *
 * @param real - Warm-up history. source series.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link AvgdevOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link avgdev} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `avgdevLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range
 *   or a buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { avgdevOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = avgdevOpenAndFill(data, { timePeriod: 14 });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function avgdevOpenAndFill(real: ArrayLike<number>, options?: AvgdevOptions): AvgdevResult & { stream: AvgdevStream } {
  const o = optionsOf('AVGDEV openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('AVGDEV openAndFill', o.core);
  const capacity = fillCapacity(avgdevLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = avgdevOpenAndFillPositional(real, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
