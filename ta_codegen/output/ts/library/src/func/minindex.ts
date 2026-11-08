/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AC       Angelo Ciceri
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120806 AC   Creation (equal to MIN but outputs index)
 *  100526 MF,CC A tie names the newest bar from any start (#503)
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function minindexLookbackPositional(optInTimePeriod: number, _core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  return optInTimePeriod - 1;
}

/** @internal */
export function minindexDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (minindexLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function minindexImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): RetCode {
  let lowest = 0;
  let tmp = 0;
  let outIdx = 0;
  let nbInitialElementNeeded = 0;
  let trailingIdx = 0;
  let lowestIdx = 0;
  let today = 0;
  let i = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  /* Identify the minimum number of price bar needed
   * to identify at least one output over the specified
   * period.
   */
  nbInitialElementNeeded = optInTimePeriod - 1;
  /* Move up the start index if there is not
   * enough initial data.
   */
  if (startIdx < nbInitialElementNeeded) {
    startIdx = nbInitialElementNeeded;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  /* Proceed with the calculation for the requested range.
   * (The integer output can never share the real input's buffer —
   * different element type; issue #130.)
   */
  outIdx = 0;
  today = startIdx;
  trailingIdx = startIdx - nbInitialElementNeeded;
  lowestIdx = -1;
  lowest = 0.0;
  while (today <= endIdx) {
    tmp = inReal[today];
    if (lowestIdx < trailingIdx) {
      lowestIdx = trailingIdx;
      lowest = inReal[lowestIdx];
      i = lowestIdx;
      while (++i <= today) {
        tmp = inReal[i];
        if (tmp <= lowest) {
          lowestIdx = i;
          lowest = tmp;
        }
      }
    } else if (tmp <= lowest) {
      lowestIdx = today;
      lowest = tmp;
    }
    outInteger[outIdx++] = lowestIdx;
    trailingIdx += 1;
    today += 1;
  }
  /* Keep the outBegIdx relative to the
   * caller input before returning.
   */
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000']] as const;

/** @internal */
export function minindexPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('MININDEX', 'startIdx', startIdx);
  endIdx = requireInt32('MININDEX', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('MININDEX', 'timePeriod', optInTimePeriod);
  requireIndexRange('MININDEX', startIdx, endIdx);
  const guardLookback = minindexLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('MININDEX', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('MININDEX', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('MININDEX', 'real', inReal, guardInLen);
  requireOutput('MININDEX', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inRealF64 = float64(inReal);
  if (overlaps(outInteger, inRealF64)) {
    throw sharedMemory('MININDEX', 'outInteger', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = minindexImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('MININDEX', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link minindex}, {@link minindexLookback},
 * {@link minindexDisplayShift}, {@link minindexOpen} and
 * {@link minindexOpenAndFill}. Every key is optional: an omitted or `undefined`
 * key takes its default, and a call ignores the keys it has no use for.
 */
export interface MinindexOptions {
  /**
   * Window length over which the minimum is located (default 30; range 2 to 100000;
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
   * buffer or with an input.
   */
  outInteger?: Int32Array<ArrayBuffer> | undefined;
}

/** What {@link minindex} returns, and {@link minindexOpenAndFill} with its stream. */
export interface MinindexResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Absolute index in real of the lowest value in each window. `values[k]` is the
   * value at input bar `begIdx + k`.
   */
  values: Int32Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link minindex} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MinindexOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function minindexLookback(options?: MinindexOptions): number {
  const o = optionsOf('MININDEX lookback', options);
  return minindexLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('MININDEX lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link minindex}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MinindexOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function minindexDisplayShift(outputIdx: number, options?: MinindexOptions): number {
  const o = optionsOf('MININDEX displayShift', options);
  return minindexDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('MININDEX displayShift', o.core));
}

/**
 * Returns the absolute index of the lowest value within a rolling window of the
 * given period. Same scan as MIN but outputs the position of the minimum rather
 * than its value.
 *
 * Formula and more info at
 * [ta-lib.org/functions/minindex](https://ta-lib.org/functions/minindex).
 *
 * **Notes**
 * - When several bars in a window share the lowest value, the index of the most
 *   recent of them is returned.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link minindexLookback} is a success with empty outputs.
 *
 * @param real - Series to scan for its minimum.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link MinindexOptions}.
 * @returns `begIdx` and the values; see {@link MinindexResult}. Each output is a
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
 * import { minindex } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = minindex(data, { timePeriod: 30 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`min`](https://ta-lib.org/functions/min)
 * @see [`maxindex`](https://ta-lib.org/functions/maxindex)
 * @see [`minmaxindex`](https://ta-lib.org/functions/minmaxindex)
 * @see [`minmax`](https://ta-lib.org/functions/minmax)
 */
export function minindex(real: ArrayLike<number>, options?: MinindexOptions): MinindexResult {
  const o = optionsOf('MININDEX', options);
  if (o.endIdx === undefined) {
    requireSeries('MININDEX', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MININDEX', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, minindexLookbackPositional(optInTimePeriod, core), real);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const range = minindexPositional(startIdx, endIdx, real, optInTimePeriod, outInteger, core);
  return { begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live MININDEX stream: one value per closed bar, bit-identical to
 * {@link minindex} over the same series. Open it with {@link minindexOpen} or
 * {@link minindexOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/minindex](https://ta-lib.org/functions/minindex).
 */
export class MinindexStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  lowest: number = 0;
  /** @internal */
  trailingIdx: number = 0;
  /** @internal */
  lowestIdx: number = 0;
  /** @internal */
  i: number = 0;
  /** @internal */
  today: number = 0;
  /** @internal */
  xMask: number = 0;
  /** @internal */
  x_inReal: Float64Array = new Float64Array(1);
  /** @internal */
  cur_outInteger: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): MinindexStream {
    return new MinindexStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link minindex} reports them over the same bars. Each accepted `update` and
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
      throw streamFailure('MININDEX advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: Series to scan for its minimum.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('MININDEX update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('MININDEX update', 'real');
    }
    minindexStepImpl(this, real);
    this.outRangeCount++;
    return this.cur_outInteger;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: Series to scan for its minimum.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number): number {
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('MININDEX peek', 'real');
    }
    const sp = this;
    const inReal = real;
    let tmp = 0;
    let cur_outInteger = 0;
    let i = sp.i;
    let lowest = sp.lowest;
    let lowestIdx = sp.lowestIdx;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    pkSlot0 = sp.today & sp.xMask;
    pkVal0 = inReal;
    tmp = ((sp.today & sp.xMask) !== pkSlot0) ? sp.x_inReal[sp.today & sp.xMask] : pkVal0;
    if (lowestIdx < sp.trailingIdx) {
      lowestIdx = sp.trailingIdx;
      lowest = ((lowestIdx & sp.xMask) !== pkSlot0) ? sp.x_inReal[lowestIdx & sp.xMask] : pkVal0;
      i = lowestIdx;
      while (++i <= sp.today) {
        tmp = ((i & sp.xMask) !== pkSlot0) ? sp.x_inReal[i & sp.xMask] : pkVal0;
        if (tmp <= lowest) {
          lowestIdx = i;
          lowest = tmp;
        }
      }
    } else if (tmp <= lowest) {
      lowestIdx = sp.today;
      lowest = tmp;
    }
    cur_outInteger = lowestIdx;
    return cur_outInteger;
  }

  /** The value at the last bar this stream counted; `peek` does not change it. */
  value(): number {
    return this.cur_outInteger;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): MinindexStream {
    const c = new MinindexStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.lowest = this.lowest;
    c.trailingIdx = this.trailingIdx;
    c.lowestIdx = this.lowestIdx;
    c.i = this.i;
    c.today = this.today;
    c.xMask = this.xMask;
    c.x_inReal = this.x_inReal.slice();
    c.cur_outInteger = this.cur_outInteger;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function minindexStepImpl(sp: MinindexStream, inReal: number): void {
  let tmp = 0;
  sp.x_inReal[sp.today & sp.xMask] = inReal;
  tmp = sp.x_inReal[sp.today & sp.xMask];
  if (sp.lowestIdx < sp.trailingIdx) {
    sp.lowestIdx = sp.trailingIdx;
    sp.lowest = sp.x_inReal[sp.lowestIdx & sp.xMask];
    sp.i = sp.lowestIdx;
    while (++sp.i <= sp.today) {
      tmp = sp.x_inReal[sp.i & sp.xMask];
      if (tmp <= sp.lowest) {
        sp.lowestIdx = sp.i;
        sp.lowest = tmp;
      }
    }
  } else if (tmp <= sp.lowest) {
    sp.lowestIdx = sp.today;
    sp.lowest = tmp;
  }
  sp.cur_outInteger = sp.lowestIdx;
  sp.trailingIdx += 1;
  sp.today += 1;
}

function minindexOpenImpl(
  sp: MinindexStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
  outStride: number,
): RetCode {
  let lowest = 0;
  let tmp = 0;
  let outIdx = 0;
  let nbInitialElementNeeded = 0;
  let trailingIdx = 0;
  let lowestIdx = 0;
  let today = 0;
  let i = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Identify the minimum number of price bar needed
   * to identify at least one output over the specified
   * period.
   */
  nbInitialElementNeeded = optInTimePeriod - 1;
  /* Move up the start index if there is not
   * enough initial data.
   */
  if (startIdx < nbInitialElementNeeded) {
    startIdx = nbInitialElementNeeded;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Proceed with the calculation for the requested range.
   * (The integer output can never share the real input's buffer —
   * different element type; issue #130.)
   */
  outIdx = 0;
  today = startIdx;
  trailingIdx = startIdx - nbInitialElementNeeded;
  lowestIdx = -1;
  lowest = 0.0;
  while (today <= endIdx) {
    tmp = inReal[today];
    if (lowestIdx < trailingIdx) {
      lowestIdx = trailingIdx;
      lowest = inReal[lowestIdx];
      i = lowestIdx;
      while (++i <= today) {
        tmp = inReal[i];
        if (tmp <= lowest) {
          lowestIdx = i;
          lowest = tmp;
        }
      }
    } else if (tmp <= lowest) {
      lowestIdx = today;
      lowest = tmp;
    }
    outInteger[((outIdx++ * outStride) | 0)] = lowestIdx;
    trailingIdx += 1;
    today += 1;
  }
  /* Keep the outBegIdx relative to the
   * caller input before returning.
   */
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  const capX = today - trailingIdx + 1;
  if (capX < 1 || capX > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  let physX = 1;
  while (physX < capX) {
    physX <<= 1;
  }
  const capX_inReal = new Float64Array(physX);
  for (let fillJ = historyLen - capX; fillJ < historyLen; fillJ++) {
    capX_inReal[fillJ & (physX - 1)] = inReal[fillJ];
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.lowest = lowest;
  sp.trailingIdx = trailingIdx;
  sp.lowestIdx = lowestIdx;
  sp.i = i;
  sp.today = today;
  sp.xMask = physX - 1;
  sp.x_inReal = capX_inReal;
  sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function minindexOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): MinindexStream {
  const sp = MinindexStream.of(core);
  const retCode = minindexOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MININDEX openAndFill', inReal.length, startIdx, minindexLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && minindexLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('MININDEX openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('MININDEX openAndFill', retCode);
}

/** @internal */
export function minindexOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): MinindexStream {
  const sp = MinindexStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = minindexOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MININDEX open', inReal.length, startIdx, minindexLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && minindexLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('MININDEX open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('MININDEX open', retCode);
}

/** @internal */
export function minindexOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): MinindexStream {
  optInTimePeriod = requireInt32('MININDEX open', 'timePeriod', optInTimePeriod);
  requireArgument('MININDEX open', 'real', inReal);
  requireHistory('MININDEX open', 'real', inReal.length);
  return minindexOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/** @internal */
export function minindexOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): MinindexStream {
  optInTimePeriod = requireInt32('MININDEX openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('MININDEX openAndFill', 'real', inReal);
  requireHistory('MININDEX openAndFill', 'real', inReal.length);
  const guardLookback = minindexLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('MININDEX openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('MININDEX openAndFill', inReal.length, guardLookback);
  requireOutput('MININDEX openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inRealF64 = float64(inReal);
  if (outInteger.buffer === inRealF64.buffer && outInteger.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('MININDEX openAndFill', 'outInteger', 'real');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inRealF64)) {
    throw sharedMemory('MININDEX openAndFill', 'outInteger', 'real');
  }
  return minindexOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outInteger);
}

/**
 * Opens a live MININDEX stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link minindex} there.
 *
 * @param real - Warm-up history. Series to scan for its minimum.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MinindexOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `minindexLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32 or a parameter outside its
 *   range.
 *
 * @example
 * ```ts
 * import { minindexOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = minindexOpen(data, { timePeriod: 30 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function minindexOpen(real: ArrayLike<number>, options?: MinindexOptions): MinindexStream {
  const o = optionsOf('MININDEX open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MININDEX open', o.core);
  return minindexOpenPositional(real, optInTimePeriod, core);
}

/**
 * {@link minindexOpen} that also computes the outputs over the whole history in
 * the same pass, bit-identically to {@link minindex}.
 *
 * @param real - Warm-up history. Series to scan for its minimum.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link MinindexOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link minindex} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `minindexLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range
 *   or a buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { minindexOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = minindexOpenAndFill(data, {
 *   timePeriod: 30,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function minindexOpenAndFill(real: ArrayLike<number>, options?: MinindexOptions): MinindexResult & { stream: MinindexStream } {
  const o = optionsOf('MININDEX openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MININDEX openAndFill', o.core);
  const capacity = fillCapacity(minindexLookbackPositional(optInTimePeriod, core), real);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const stream = minindexOpenAndFillPositional(real, optInTimePeriod, outInteger, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}
