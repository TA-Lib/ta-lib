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
 *  010802 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  070226 MF,CC  Speed optimization: cache the highest/lowest index
 *                instead of rescanning the window on every bar (same
 *                approach as MIN/MAX/MINMAX).
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function midpointLookbackPositional(optInTimePeriod: number, _core: Core = Core.DEFAULT): number {
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
export function midpointDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (midpointLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function midpointImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let sufHighest!: Float64Array;
  let preHighest!: Float64Array;
  let sufLowest!: Float64Array;
  let preLowest!: Float64Array;
  let lowest = 0;
  let highest = 0;
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
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  /* Find the highest and lowest value of a timeserie
   * over the period.
   *      MIDPOINT = (Highest Value + Lowest Value)/2
   *
   * See MIDPRICE if the input is a price bar with a
   * high and low timeserie.
   */
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
   * into a per-bar automaton, so the streaming tier runs midpoint_ALT1
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
    outReal[outIdx++] = (sufHighest[0] + sufLowest[0]) / 2.0;
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
        outReal[outIdx++] = (highest + lowest) / 2.0;
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
export function midpointPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('MIDPOINT', 'startIdx', startIdx);
  endIdx = requireInt32('MIDPOINT', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('MIDPOINT', 'timePeriod', optInTimePeriod);
  requireIndexRange('MIDPOINT', startIdx, endIdx);
  const guardLookback = midpointLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('MIDPOINT', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('MIDPOINT', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('MIDPOINT', 'real', inReal, guardInLen);
  requireOutput('MIDPOINT', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('MIDPOINT', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = midpointImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('MIDPOINT', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link midpoint}, {@link midpointLookback},
 * {@link midpointDisplayShift}, {@link midpointOpen} and
 * {@link midpointOpenAndFill}. Every key is optional: an omitted or `undefined`
 * key takes its default, and a call ignores the keys it has no use for.
 */
export interface MidpointOptions {
  /**
   * Lookback window length (default 14; range 2 to 100000; `Core.INTEGER_DEFAULT`
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
   * Buffer for `values`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outReal?: Float64Array | undefined;
}

/** What {@link midpoint} returns, and {@link midpointOpenAndFill} with its stream. */
export interface MidpointResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Midpoint of the period's high/low range. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link midpoint} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MidpointOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function midpointLookback(options?: MidpointOptions): number {
  const o = optionsOf('MIDPOINT lookback', options);
  return midpointLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('MIDPOINT lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link midpoint}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MidpointOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function midpointDisplayShift(outputIdx: number, options?: MidpointOptions): number {
  const o = optionsOf('MIDPOINT displayShift', options);
  return midpointDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('MIDPOINT displayShift', o.core));
}

/**
 * Midpoint over a period: the average of the highest and lowest input values
 * within the lookback window. A single-series overlap smoother (use MIDPRICE for
 * separate high/low price bars).
 *
 * Formula and more info at
 * [ta-lib.org/functions/midpoint](https://ta-lib.org/functions/midpoint).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link midpointLookback} is a success with empty outputs.
 *
 * @param real - Series to compute the midpoint over.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link MidpointOptions}.
 * @returns `begIdx` and the values; see {@link MidpointResult}. Each output is a
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
 * import { midpoint } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = midpoint(data, { timePeriod: 14 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`midprice`](https://ta-lib.org/functions/midprice)
 * @see [`max`](https://ta-lib.org/functions/max)
 * @see [`min`](https://ta-lib.org/functions/min)
 */
export function midpoint(real: ArrayLike<number>, options?: MidpointOptions): MidpointResult {
  const o = optionsOf('MIDPOINT', options);
  if (o.endIdx === undefined) {
    requireSeries('MIDPOINT', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MIDPOINT', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, midpointLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = midpointPositional(startIdx, endIdx, real, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/* Using midpoint_ALT1 for TA_ALT={STREAM,ALL_LANGUAGES} */

/**
 * A live MIDPOINT stream: one value per closed bar, bit-identical to
 * {@link midpoint} over the same series. Open it with {@link midpointOpen} or
 * {@link midpointOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/midpoint](https://ta-lib.org/functions/midpoint).
 */
export class MidpointStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  lowest: number = 0;
  /** @internal */
  highest: number = 0;
  /** @internal */
  trailingIdx: number = 0;
  /** @internal */
  lowestIdx: number = 0;
  /** @internal */
  highestIdx: number = 0;
  /** @internal */
  i: number = 0;
  /** @internal */
  today: number = 0;
  /** @internal */
  xMask: number = 0;
  /** @internal */
  x_inReal: Float64Array = new Float64Array(1);
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
  static of(core: Core): MidpointStream {
    return new MidpointStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link midpoint} reports them over the same bars. Each accepted `update` and
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
      throw streamFailure('MIDPOINT advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: Series to compute the midpoint over.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('MIDPOINT update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('MIDPOINT update', 'real');
    }
    midpointStepImpl(this, real);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: Series to compute the midpoint over.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number): number {
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('MIDPOINT peek', 'real');
    }
    const sp = this;
    const inReal = real;
    let tmpLow = 0;
    let tmpHigh = 0;
    let cur_outReal = 0;
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
    cur_outReal = (highest + lowest) / 2.0;
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
  clone(): MidpointStream {
    const c = new MidpointStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.lowest = this.lowest;
    c.highest = this.highest;
    c.trailingIdx = this.trailingIdx;
    c.lowestIdx = this.lowestIdx;
    c.highestIdx = this.highestIdx;
    c.i = this.i;
    c.today = this.today;
    c.xMask = this.xMask;
    c.x_inReal = this.x_inReal.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function midpointStepImpl(sp: MidpointStream, inReal: number): void {
  let tmpLow = 0;
  let tmpHigh = 0;
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
  sp.cur_outReal = (sp.highest + sp.lowest) / 2.0;
  sp.trailingIdx += 1;
  sp.today += 1;
}

function midpointOpenImpl(
  sp: MidpointStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let lowest = 0;
  let highest = 0;
  let tmpLow = 0;
  let tmpHigh = 0;
  let outIdx = 0;
  let nbInitialElementNeeded = 0;
  let trailingIdx = 0;
  let lowestIdx = 0;
  let highestIdx = 0;
  let today = 0;
  let i = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Find the highest and lowest value of a timeserie
   * over the period.
   *      MIDPOINT = (Highest Value + Lowest Value)/2
   *
   * See MIDPRICE if the input is a price bar with a
   * high and low timeserie.
   */
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
   * The highest and lowest of the window are cached with their indices;
   * the window is rescanned only when a cached extremum drops out of it.
   * That is O(1)
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
    outReal[((outIdx++ * outStride) | 0)] = (highest + lowest) / 2.0;
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
  sp.highest = highest;
  sp.trailingIdx = trailingIdx;
  sp.lowestIdx = lowestIdx;
  sp.highestIdx = highestIdx;
  sp.i = i;
  sp.today = today;
  sp.xMask = physX - 1;
  sp.x_inReal = capX_inReal;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function midpointOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): MidpointStream {
  const sp = MidpointStream.of(core);
  const retCode = midpointOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MIDPOINT openAndFill', inReal.length, startIdx, midpointLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && midpointLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('MIDPOINT openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('MIDPOINT openAndFill', retCode);
}

/** @internal */
export function midpointOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): MidpointStream {
  const sp = MidpointStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = midpointOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MIDPOINT open', inReal.length, startIdx, midpointLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && midpointLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('MIDPOINT open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('MIDPOINT open', retCode);
}

/** @internal */
export function midpointOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): MidpointStream {
  optInTimePeriod = requireInt32('MIDPOINT open', 'timePeriod', optInTimePeriod);
  requireArgument('MIDPOINT open', 'real', inReal);
  requireHistory('MIDPOINT open', 'real', inReal.length);
  return midpointOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/** @internal */
export function midpointOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): MidpointStream {
  optInTimePeriod = requireInt32('MIDPOINT openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('MIDPOINT openAndFill', 'real', inReal);
  requireHistory('MIDPOINT openAndFill', 'real', inReal.length);
  const guardLookback = midpointLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('MIDPOINT openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('MIDPOINT openAndFill', inReal.length, guardLookback);
  requireOutput('MIDPOINT openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('MIDPOINT openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('MIDPOINT openAndFill', 'outReal', 'real');
  }
  return midpointOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live MIDPOINT stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link midpoint} there.
 *
 * @param real - Warm-up history. Series to compute the midpoint over.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MidpointOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `midpointLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32 or a parameter outside its
 *   range.
 *
 * @example
 * ```ts
 * import { midpointOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = midpointOpen(data, { timePeriod: 14 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function midpointOpen(real: ArrayLike<number>, options?: MidpointOptions): MidpointStream {
  const o = optionsOf('MIDPOINT open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MIDPOINT open', o.core);
  return midpointOpenPositional(real, optInTimePeriod, core);
}

/**
 * {@link midpointOpen} that also computes the outputs over the whole history in
 * the same pass, bit-identically to {@link midpoint}.
 *
 * @param real - Warm-up history. Series to compute the midpoint over.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link MidpointOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link midpoint} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `midpointLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range
 *   or a buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { midpointOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = midpointOpenAndFill(data, {
 *   timePeriod: 14,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function midpointOpenAndFill(real: ArrayLike<number>, options?: MidpointOptions): MidpointResult & { stream: MidpointStream } {
  const o = optionsOf('MIDPOINT openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MIDPOINT openAndFill', o.core);
  const capacity = fillCapacity(midpointLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = midpointOpenAndFillPositional(real, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
