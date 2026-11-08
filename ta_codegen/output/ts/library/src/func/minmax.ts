/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AC       Angelo Ciceri
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120906 AC   Creation
 */

import { badSink, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function minmaxLookbackPositional(optInTimePeriod: number, _core: Core = Core.DEFAULT): number {
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
export function minmaxDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (minmaxLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 2) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function minmaxImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outMin: Float64Array,
  outMax: Float64Array,
): RetCode {
  let sufHighest!: Float64Array;
  let preHighest!: Float64Array;
  let sufLowest!: Float64Array;
  let preLowest!: Float64Array;
  let highest = 0;
  let lowest = 0;
  let tmpHigh = 0;
  let outIdx = 0;
  let nbInitialElementNeeded = 0;
  let trailingIdx = 0;
  let today = 0;
  let i = 0;
  let blockStart = 0;
  let nAvail = 0;
  let m = 0;
  let blockNext = 0;
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
  if ((outMin.buffer === outMax.buffer && outMin.byteOffset === outMax.byteOffset)) {
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
   * Note that this algorithm allows the input and
   * output to be the same buffer.
   *
   * Van Herk / Gil-Werman block scan, block-batched form. The p outputs
   * belonging to one block boundary are produced together: one backward
   * pass builds the older block's suffix extrema, one forward pass builds
   * the newer block's prefix extrema, and a third pass combines them.
   * Both extrema travel in the same passes.
   * All the loops are straight-line with no data-dependent branching,
   * which is what lets a compiler vectorize them, and the work per bar is
   * a fixed number of comparisons regardless of period. Every scratch
   * array holds COPIES, so input and output may alias.
   *
   * Producing a whole block at a time is also why this cannot be turned
   * into a per-bar automaton, so the streaming tier runs minmax_ALT1
   * below. See issue #147.
   */
  outIdx = 0;
  today = startIdx;
  trailingIdx = startIdx - nbInitialElementNeeded;
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sufHighest = new Float64Array(optInTimePeriod);
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  preHighest = new Float64Array(optInTimePeriod);
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sufLowest = new Float64Array(optInTimePeriod);
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  preLowest = new Float64Array(optInTimePeriod);
  blockStart = trailingIdx;
  while (today <= endIdx) {
    /* Suffix extrema of the block [blockStart, blockStart+p-1], which
     * is fully available here: today == blockStart+p-1 <= endIdx.
     * Scanning backward while keeping the incumbent on a tie
     * leaves the later element holding a tie, which is what lets this
     * compile to a single min/max instruction.
     */
    i = blockStart + optInTimePeriod - 1;
    highest = inReal[i];
    lowest = highest;
    sufHighest[optInTimePeriod - 1] = highest;
    sufLowest[optInTimePeriod - 1] = lowest;
    while (i > blockStart) {
      i -= 1;
      tmpHigh = inReal[i];
      if (tmpHigh > highest) {
        highest = tmpHigh;
      }
      if (tmpHigh < lowest) {
        lowest = tmpHigh;
      }
      sufHighest[i - blockStart] = highest;
      sufLowest[i - blockStart] = lowest;
    }
    outMax[outIdx] = sufHighest[0];
    outMin[outIdx] = sufLowest[0];
    outIdx += 1;
    trailingIdx += 1;
    today += 1;
    if (today > endIdx) {
      blockStart = blockStart + optInTimePeriod;
    } else {
      /* Prefix extrema of the next block, clamped to what remains.
       * Forward, keeping the incumbent on a tie: earliest wins again.
       */
      blockNext = blockStart + optInTimePeriod;
      nAvail = endIdx - blockNext + 1;
      if (nAvail > optInTimePeriod - 1) {
        nAvail = optInTimePeriod - 1;
      }
      highest = inReal[blockNext];
      lowest = highest;
      preHighest[0] = highest;
      preLowest[0] = lowest;
      i = 1;
      while (i < nAvail) {
        tmpHigh = inReal[blockNext + i];
        if (tmpHigh > highest) {
          highest = tmpHigh;
        }
        if (tmpHigh < lowest) {
          lowest = tmpHigh;
        }
        preHighest[i] = highest;
        preLowest[i] = lowest;
        i += 1;
      }
      /* Combine. The suffix half is the older one, so preferring it
       * on a tie keeps the earliest-wins rule.
       */
      m = 1;
      while (m <= nAvail) {
        highest = sufHighest[m];
        if (preHighest[m - 1] > highest) {
          highest = preHighest[m - 1];
        }
        lowest = sufLowest[m];
        if (preLowest[m - 1] < lowest) {
          lowest = preLowest[m - 1];
        }
        outMax[outIdx] = highest;
        outMin[outIdx] = lowest;
        outIdx += 1;
        m += 1;
      }
      trailingIdx = trailingIdx + nAvail;
      today = today + nAvail;
      blockStart = blockStart + optInTimePeriod;
    }
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
export function minmaxPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outMin: Float64Array,
  outMax: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('MINMAX', 'startIdx', startIdx);
  endIdx = requireInt32('MINMAX', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('MINMAX', 'timePeriod', optInTimePeriod);
  requireIndexRange('MINMAX', startIdx, endIdx);
  const guardLookback = minmaxLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('MINMAX', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('MINMAX', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('MINMAX', 'real', inReal, guardInLen);
  requireOutput('MINMAX', 'outMin', outMin, guardOutLen, Float64Array);
  requireOutput('MINMAX', 'outMax', outMax, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outMin, inRealF64)) {
    throw sharedMemory('MINMAX', 'outMin', 'real');
  }
  if (overlapsPartly(outMax, inRealF64)) {
    throw sharedMemory('MINMAX', 'outMax', 'real');
  }
  if (overlaps(outMin, outMax)) {
    throw sharedMemory('MINMAX', 'outMin', 'outMax');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = minmaxImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outMin, outMax);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('MINMAX', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link minmax}, {@link minmaxLookback}, {@link minmaxDisplayShift},
 * {@link minmaxOpen} and {@link minmaxOpenAndFill}. Every key is optional: an
 * omitted or `undefined` key takes its default, and a call ignores the keys it has
 * no use for.
 */
export interface MinmaxOptions {
  /**
   * Rolling window length (default 30; range 2 to 100000; `Core.INTEGER_DEFAULT`
   * selects the default).
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
   * Buffer for `outMin`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outMin?: Float64Array<ArrayBuffer> | undefined;
  /**
   * Buffer for `outMax`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outMax?: Float64Array<ArrayBuffer> | undefined;
}

/** What {@link minmax} returns, and {@link minmaxOpenAndFill} with its stream. */
export interface MinmaxResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Lowest value in each rolling window. `outMin[k]` is the value at input bar
   * `begIdx + k`.
   */
  outMin: Float64Array<ArrayBuffer>;
  /**
   * Highest value in each rolling window. `outMax[k]` is the value at input bar
   * `begIdx + k`.
   */
  outMax: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link minmax} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MinmaxOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function minmaxLookback(options?: MinmaxOptions): number {
  const o = optionsOf('MINMAX lookback', options);
  return minmaxLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('MINMAX lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link minmax}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MinmaxOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function minmaxDisplayShift(outputIdx: number, options?: MinmaxOptions): number {
  const o = optionsOf('MINMAX displayShift', options);
  return minmaxDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('MINMAX displayShift', o.core));
}

/**
 * Returns both the lowest and highest values of the input over a rolling window of
 * the last timePeriod bars. An overlap-study companion to MIN and MAX that
 * computes both extrema in one pass.
 *
 * Formula and more info at
 * [ta-lib.org/functions/minmax](https://ta-lib.org/functions/minmax).
 *
 * `outMin[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link minmaxLookback} is a success with empty outputs.
 *
 * @param real - Values scanned for the window min and max.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link MinmaxOptions}.
 * @returns `begIdx` and the values; see {@link MinmaxResult}. Each output is a
 *   view of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, an integer parameter that is not an int32, a parameter outside its range,
 *   an input too short for the range, a buffer that is empty, too short or of the
 *   wrong type, two buffers sharing memory or a buffer sharing memory with a
 *   `Float64Array` input it is not. Checked before anything is written.
 *
 * @example
 * ```ts
 * import { minmax } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, outMin, outMax } = minmax(data, { timePeriod: 30 });
 * // outMin[k] is the value at bar begIdx + k
 * ```
 * @see [`min`](https://ta-lib.org/functions/min)
 * @see [`max`](https://ta-lib.org/functions/max)
 * @see [`minmaxindex`](https://ta-lib.org/functions/minmaxindex)
 * @see [`minindex`](https://ta-lib.org/functions/minindex)
 * @see [`maxindex`](https://ta-lib.org/functions/maxindex)
 */
export function minmax(real: ArrayLike<number>, options?: MinmaxOptions): MinmaxResult {
  const o = optionsOf('MINMAX', options);
  if (o.endIdx === undefined) {
    requireSeries('MINMAX', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MINMAX', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, minmaxLookbackPositional(optInTimePeriod, core), real);
  const outMin = o.outMin === undefined ? new Float64Array(capacity) : o.outMin;
  const outMax = o.outMax === undefined ? new Float64Array(capacity) : o.outMax;
  const range = minmaxPositional(startIdx, endIdx, real, optInTimePeriod, outMin, outMax, core);
  return { begIdx: range.begIdx, outMin: outMin.subarray(0, range.count), outMax: outMax.subarray(0, range.count) };
}

/**** Streaming API *****/

/* Using minmax_ALT1 for TA_ALT={STREAM,ALL_LANGUAGES} */

/**
 * A live MINMAX stream: one value per closed bar, bit-identical to {@link minmax}
 * over the same series. Open it with {@link minmaxOpen} or
 * {@link minmaxOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/minmax](https://ta-lib.org/functions/minmax).
 */
export class MinmaxStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  highest: number = 0;
  /** @internal */
  lowest: number = 0;
  /** @internal */
  trailingIdx: number = 0;
  /** @internal */
  highestIdx: number = 0;
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
  cur_outMin: number = 0;
  /** @internal */
  cur_outMax: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): MinmaxStream {
    return new MinmaxStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link minmax} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('MINMAX advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns its outputs.
   *
   * @param real - The bar's `real` value: Values scanned for the window min and max.
   * @param out - Receives the outputs and is returned; a new {@link MinmaxOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included, before anything moves.
   *   `TALibError` with code `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached
   *   bar `Core.INDEX_MAX`.
   */
  update(real: number, out?: MinmaxOut): MinmaxOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('MINMAX update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('MINMAX update', 'MinmaxOut');
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('MINMAX update', 'real');
    }
    minmaxStepImpl(this, real);
    this.outRangeCount++;
    if (out === undefined) {
      out = new MinmaxOut();
    }
    out.outMin = this.cur_outMin;
    out.outMax = this.cur_outMax;
    return out;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: Values scanned for the window min and max.
   * @param out - Receives the outputs and is returned; a new {@link MinmaxOut} when
   *   omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included.
   */
  peek(real: number, out?: MinmaxOut): MinmaxOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('MINMAX peek', 'MinmaxOut');
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('MINMAX peek', 'real');
    }
    const sp = this;
    const inReal = real;
    let tmpHigh = 0;
    let tmpLow = 0;
    let cur_outMax = 0;
    let cur_outMin = 0;
    let highest = sp.highest;
    let highestIdx = sp.highestIdx;
    let i = sp.i;
    let lowest = sp.lowest;
    let lowestIdx = sp.lowestIdx;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    pkSlot0 = sp.today & sp.xMask;
    pkVal0 = inReal;
    tmpHigh = ((sp.today & sp.xMask) !== pkSlot0) ? sp.x_inReal[sp.today & sp.xMask] : pkVal0;
    tmpLow = tmpHigh;
    if (highestIdx < sp.trailingIdx) {
      highestIdx = sp.trailingIdx;
      highest = ((highestIdx & sp.xMask) !== pkSlot0) ? sp.x_inReal[highestIdx & sp.xMask] : pkVal0;
      i = highestIdx;
      while (++i <= sp.today) {
        tmpHigh = ((i & sp.xMask) !== pkSlot0) ? sp.x_inReal[i & sp.xMask] : pkVal0;
        if (tmpHigh > highest) {
          highestIdx = i;
          highest = tmpHigh;
        }
      }
    } else if (tmpHigh >= highest) {
      highestIdx = sp.today;
      highest = tmpHigh;
    }
    if (lowestIdx < sp.trailingIdx) {
      lowestIdx = sp.trailingIdx;
      lowest = ((lowestIdx & sp.xMask) !== pkSlot0) ? sp.x_inReal[lowestIdx & sp.xMask] : pkVal0;
      i = lowestIdx;
      while (++i <= sp.today) {
        tmpLow = ((i & sp.xMask) !== pkSlot0) ? sp.x_inReal[i & sp.xMask] : pkVal0;
        if (tmpLow < lowest) {
          lowestIdx = i;
          lowest = tmpLow;
        }
      }
    } else if (tmpLow <= lowest) {
      lowestIdx = sp.today;
      lowest = tmpLow;
    }
    cur_outMax = highest;
    cur_outMin = lowest;
    if (out === undefined) {
      out = new MinmaxOut();
    }
    out.outMin = cur_outMin;
    out.outMax = cur_outMax;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link MinmaxOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a sink that is not an object,
   *   `null` included.
   */
  value(out?: MinmaxOut): MinmaxOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('MINMAX value', 'MinmaxOut');
    }
    if (out === undefined) {
      out = new MinmaxOut();
    }
    out.outMin = this.cur_outMin;
    out.outMax = this.cur_outMax;
    return out;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): MinmaxStream {
    const c = new MinmaxStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.highest = this.highest;
    c.lowest = this.lowest;
    c.trailingIdx = this.trailingIdx;
    c.highestIdx = this.highestIdx;
    c.lowestIdx = this.lowestIdx;
    c.i = this.i;
    c.today = this.today;
    c.xMask = this.xMask;
    c.x_inReal = this.x_inReal.slice();
    c.cur_outMin = this.cur_outMin;
    c.cur_outMax = this.cur_outMax;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one MINMAX bar, as `update`, `peek` and `value` return them.
 * Passed back to one of them, it is filled and returned again, with no allocation;
 * it holds its values until the next call that writes it. A new one holds `NaN`.
 */
export class MinmaxOut {
  /** Lowest value in each rolling window. */
  outMin: number = NaN;
  /** Highest value in each rolling window. */
  outMax: number = NaN;
}

function minmaxStepImpl(sp: MinmaxStream, inReal: number): void {
  let tmpHigh = 0;
  let tmpLow = 0;
  sp.x_inReal[sp.today & sp.xMask] = inReal;
  tmpHigh = sp.x_inReal[sp.today & sp.xMask];
  tmpLow = tmpHigh;
  if (sp.highestIdx < sp.trailingIdx) {
    sp.highestIdx = sp.trailingIdx;
    sp.highest = sp.x_inReal[sp.highestIdx & sp.xMask];
    sp.i = sp.highestIdx;
    while (++sp.i <= sp.today) {
      tmpHigh = sp.x_inReal[sp.i & sp.xMask];
      if (tmpHigh > sp.highest) {
        sp.highestIdx = sp.i;
        sp.highest = tmpHigh;
      }
    }
  } else if (tmpHigh >= sp.highest) {
    sp.highestIdx = sp.today;
    sp.highest = tmpHigh;
  }
  if (sp.lowestIdx < sp.trailingIdx) {
    sp.lowestIdx = sp.trailingIdx;
    sp.lowest = sp.x_inReal[sp.lowestIdx & sp.xMask];
    sp.i = sp.lowestIdx;
    while (++sp.i <= sp.today) {
      tmpLow = sp.x_inReal[sp.i & sp.xMask];
      if (tmpLow < sp.lowest) {
        sp.lowestIdx = sp.i;
        sp.lowest = tmpLow;
      }
    }
  } else if (tmpLow <= sp.lowest) {
    sp.lowestIdx = sp.today;
    sp.lowest = tmpLow;
  }
  sp.cur_outMax = sp.highest;
  sp.cur_outMin = sp.lowest;
  sp.trailingIdx += 1;
  sp.today += 1;
}

function minmaxOpenImpl(
  sp: MinmaxStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outMin: Float64Array,
  outMax: Float64Array,
  outStride: number,
): RetCode {
  let highest = 0;
  let lowest = 0;
  let tmpHigh = 0;
  let tmpLow = 0;
  let outIdx = 0;
  let nbInitialElementNeeded = 0;
  let trailingIdx = 0;
  let today = 0;
  let i = 0;
  let highestIdx = 0;
  let lowestIdx = 0;
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
   * Note that this algorithm allows the input and
   * output to be the same buffer.
   *
   * The highest and lowest values of the window are cached with their
   * indices; the window is rescanned only when a cached extremum drops out
   * of it. That is O(1)
   * per bar while the extremum sits away from the trailing edge, but it is
   * not amortized O(1): an extremum on the oldest in-window bar drops out
   * on the very next bar, so the rescan repeats and the cost stays
   * O(period) per bar for as long as that persists.
   *
   * Tracking both extrema keeps that state going through a trend: while
   * the high is refreshed by each new bar, the low stays pinned at the
   * oldest bar for the whole leg (and the reverse on the way down). A flat
   * stretch pins both. Random-walk input is the favourable case, where
   * rescans are rare.
   *
   * Slower than the block scan the batch tier runs; it is here because one
   * bar at a time is exactly what the streaming tier needs. See issue #147.
   */
  outIdx = 0;
  today = startIdx;
  trailingIdx = startIdx - nbInitialElementNeeded;
  highestIdx = -1;
  highest = 0.0;
  lowestIdx = -1;
  lowest = 0.0;
  while (today <= endIdx) {
    tmpHigh = inReal[today];
    tmpLow = tmpHigh;
    if (highestIdx < trailingIdx) {
      highestIdx = trailingIdx;
      highest = inReal[highestIdx];
      i = highestIdx;
      while (++i <= today) {
        tmpHigh = inReal[i];
        if (tmpHigh > highest) {
          highestIdx = i;
          highest = tmpHigh;
        }
      }
    } else if (tmpHigh >= highest) {
      highestIdx = today;
      highest = tmpHigh;
    }
    if (lowestIdx < trailingIdx) {
      lowestIdx = trailingIdx;
      lowest = inReal[lowestIdx];
      i = lowestIdx;
      while (++i <= today) {
        tmpLow = inReal[i];
        if (tmpLow < lowest) {
          lowestIdx = i;
          lowest = tmpLow;
        }
      }
    } else if (tmpLow <= lowest) {
      lowestIdx = today;
      lowest = tmpLow;
    }
    outMax[((outIdx * outStride) | 0)] = highest;
    outMin[((outIdx * outStride) | 0)] = lowest;
    outIdx += 1;
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
  sp.highest = highest;
  sp.lowest = lowest;
  sp.trailingIdx = trailingIdx;
  sp.highestIdx = highestIdx;
  sp.lowestIdx = lowestIdx;
  sp.i = i;
  sp.today = today;
  sp.xMask = physX - 1;
  sp.x_inReal = capX_inReal;
  sp.cur_outMin = outMin[(outNBElement.value - 1) * outStride];
  sp.cur_outMax = outMax[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function minmaxOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outMin: Float64Array,
  outMax: Float64Array,
): MinmaxStream {
  const sp = MinmaxStream.of(core);
  const retCode = minmaxOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outMin, outMax, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MINMAX openAndFill', inReal.length, startIdx, minmaxLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && minmaxLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('MINMAX openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('MINMAX openAndFill', retCode);
}

/** @internal */
export function minmaxOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): MinmaxStream {
  const sp = MinmaxStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outMin = new Float64Array(1);
  const sink_outMax = new Float64Array(1);
  const retCode = minmaxOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outMin, sink_outMax, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MINMAX open', inReal.length, startIdx, minmaxLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && minmaxLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('MINMAX open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('MINMAX open', retCode);
}

/** @internal */
export function minmaxOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): MinmaxStream {
  optInTimePeriod = requireInt32('MINMAX open', 'timePeriod', optInTimePeriod);
  requireArgument('MINMAX open', 'real', inReal);
  requireHistory('MINMAX open', 'real', inReal.length);
  return minmaxOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/** @internal */
export function minmaxOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outMin: Float64Array,
  outMax: Float64Array,
  core: Core = Core.DEFAULT,
): MinmaxStream {
  optInTimePeriod = requireInt32('MINMAX openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('MINMAX openAndFill', 'real', inReal);
  requireHistory('MINMAX openAndFill', 'real', inReal.length);
  const guardLookback = minmaxLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('MINMAX openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('MINMAX openAndFill', inReal.length, guardLookback);
  requireOutput('MINMAX openAndFill', 'outMin', outMin, guardOutLen, Float64Array);
  requireOutput('MINMAX openAndFill', 'outMax', outMax, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outMin.buffer === inRealF64.buffer && outMin.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('MINMAX openAndFill', 'outMin', 'real');
  }
  if (outMax.buffer === inRealF64.buffer && outMax.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('MINMAX openAndFill', 'outMax', 'real');
  }
  if (outMin.buffer === outMax.buffer && outMin.byteOffset === outMax.byteOffset) {
    throw sharedMemory('MINMAX openAndFill', 'outMin', 'outMax');
  }
  if (guardOutLen > 0 && overlaps(outMin, inRealF64)) {
    throw sharedMemory('MINMAX openAndFill', 'outMin', 'real');
  }
  if (guardOutLen > 0 && overlaps(outMax, inRealF64)) {
    throw sharedMemory('MINMAX openAndFill', 'outMax', 'real');
  }
  if (guardOutLen > 0 && overlaps(outMin, outMax)) {
    throw sharedMemory('MINMAX openAndFill', 'outMin', 'outMax');
  }
  return minmaxOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outMin, outMax);
}

/**
 * Opens a live MINMAX stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link minmax} there.
 *
 * @param real - Warm-up history. Values scanned for the window min and max.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MinmaxOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `minmaxLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32 or a parameter outside its
 *   range.
 *
 * @example
 * ```ts
 * import { minmaxOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = minmaxOpen(data, { timePeriod: 30 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function minmaxOpen(real: ArrayLike<number>, options?: MinmaxOptions): MinmaxStream {
  const o = optionsOf('MINMAX open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MINMAX open', o.core);
  return minmaxOpenPositional(real, optInTimePeriod, core);
}

/**
 * {@link minmaxOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link minmax}.
 *
 * @param real - Warm-up history. Values scanned for the window min and max.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link MinmaxOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link minmax} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `minmaxLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range
 *   or a buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { minmaxOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, outMin, outMax } = minmaxOpenAndFill(data, {
 *   timePeriod: 30,
 * });
 * // outMin[k] is the value at bar begIdx + k
 * ```
 */
export function minmaxOpenAndFill(real: ArrayLike<number>, options?: MinmaxOptions): MinmaxResult & { stream: MinmaxStream } {
  const o = optionsOf('MINMAX openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MINMAX openAndFill', o.core);
  const capacity = fillCapacity(minmaxLookbackPositional(optInTimePeriod, core), real);
  const outMin = o.outMin === undefined ? new Float64Array(capacity) : o.outMin;
  const outMax = o.outMax === undefined ? new Float64Array(capacity) : o.outMax;
  const stream = minmaxOpenAndFillPositional(real, optInTimePeriod, outMin, outMax, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outMin: outMin.subarray(0, range.count), outMax: outMax.subarray(0, range.count) };
}
