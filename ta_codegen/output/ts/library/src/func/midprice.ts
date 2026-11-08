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
 *  070226 MF,CC  Speed optimization: for periods above 20, cache the
 *                highest/lowest index instead of rescanning the window
 *                on every bar (same approach as MIN/MAX/WILLR). Smaller
 *                periods keep the simple scan, which auto-vectorizes
 *                and is faster there. Both paths produce identical
 *                output.
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function midpriceLookbackPositional(optInTimePeriod: number, _core: Core = Core.DEFAULT): number {
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
export function midpriceDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (midpriceLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function midpriceImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
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
  let tmpLow = 0;
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
  /* MIDPRICE = (Highest High + Lowest Low)/2
   *
   * This function is equivalent to MEDPRICE when the
   * period is 1.
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
   * into a per-bar automaton, so the streaming tier runs midprice_ALT1
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
    highest = inHigh[i];
    lowest = inLow[i];
    sufHighest[optInTimePeriod - 1] = highest;
    sufLowest[optInTimePeriod - 1] = lowest;
    while (i > blockStart) {
      i -= 1;
      tmpHigh = inHigh[i];
      if (tmpHigh > highest) {
        highest = tmpHigh;
      }
      tmpLow = inLow[i];
      if (tmpLow < lowest) {
        lowest = tmpLow;
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
      highest = inHigh[blockNext];
      lowest = inLow[blockNext];
      preHighest[0] = highest;
      preLowest[0] = lowest;
      i = 1;
      while (i < nAvail) {
        tmpHigh = inHigh[blockNext + i];
        if (tmpHigh > highest) {
          highest = tmpHigh;
        }
        tmpLow = inLow[blockNext + i];
        if (tmpLow < lowest) {
          lowest = tmpLow;
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
export function midpricePositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('MIDPRICE', 'startIdx', startIdx);
  endIdx = requireInt32('MIDPRICE', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('MIDPRICE', 'timePeriod', optInTimePeriod);
  requireIndexRange('MIDPRICE', startIdx, endIdx);
  const guardLookback = midpriceLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('MIDPRICE', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('MIDPRICE', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('MIDPRICE', 'prices.high', inHigh, guardInLen);
  requireLength('MIDPRICE', 'prices.low', inLow, guardInLen);
  requireOutput('MIDPRICE', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlapsPartly(outReal, inHighF64)) {
    throw sharedMemory('MIDPRICE', 'outReal', 'prices.high');
  }
  if (overlapsPartly(outReal, inLowF64)) {
    throw sharedMemory('MIDPRICE', 'outReal', 'prices.low');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = midpriceImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('MIDPRICE', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link midprice}, {@link midpriceLookback},
 * {@link midpriceDisplayShift}, {@link midpriceOpen} and
 * {@link midpriceOpenAndFill}. Every key is optional: an omitted or `undefined`
 * key takes its default, and a call ignores the keys it has no use for.
 */
export interface MidpriceOptions {
  /**
   * Window length over which the high/low extremes are taken (default 14; range 2 to
   * 100000; `Core.INTEGER_DEFAULT` selects the default).
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

/** What {@link midprice} returns, and {@link midpriceOpenAndFill} with its stream. */
export interface MidpriceResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Midpoint of the period's high/low extremes. `values[k]` is the value at input
   * bar `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link midprice} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MidpriceOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function midpriceLookback(options?: MidpriceOptions): number {
  const o = optionsOf('MIDPRICE lookback', options);
  return midpriceLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('MIDPRICE lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link midprice}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MidpriceOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function midpriceDisplayShift(outputIdx: number, options?: MidpriceOptions): number {
  const o = optionsOf('MIDPRICE displayShift', options);
  return midpriceDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('MIDPRICE displayShift', o.core));
}

/**
 * Midpoint of the price range over a rolling window: the average of the highest
 * high and lowest low across the last timePeriod bars. An overlap-study line
 * plotted on price.
 *
 * Formula and more info at
 * [ta-lib.org/functions/midprice](https://ta-lib.org/functions/midprice).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link midpriceLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `high`: High price of
 *   each bar. `low`: Low price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link MidpriceOptions}.
 * @returns `begIdx` and the values; see {@link MidpriceResult}. Each output is a
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
 * import { midprice } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = midprice({ high, low }, { timePeriod: 14 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`midpoint`](https://ta-lib.org/functions/midpoint)
 * @see [`medprice`](https://ta-lib.org/functions/medprice)
 */
export function midprice(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: MidpriceOptions): MidpriceResult {
  requireArgument('MIDPRICE', 'prices', prices);
  const o = optionsOf('MIDPRICE', options);
  if (o.endIdx === undefined) {
    requireSeries('MIDPRICE', 'prices.high', prices.high);
    requireSeries('MIDPRICE', 'prices.low', prices.low);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MIDPRICE', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, midpriceLookbackPositional(optInTimePeriod, core), prices.high, prices.low);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = midpricePositional(startIdx, endIdx, prices.high, prices.low, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/* Using midprice_ALT1 for TA_ALT={STREAM,ALL_LANGUAGES} */

/**
 * A live MIDPRICE stream: one value per closed bar, bit-identical to
 * {@link midprice} over the same series. Open it with {@link midpriceOpen} or
 * {@link midpriceOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/midprice](https://ta-lib.org/functions/midprice).
 */
export class MidpriceStream {
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
  x_inHigh: Float64Array = new Float64Array(1);
  /** @internal */
  x_inLow: Float64Array = new Float64Array(1);
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
  static of(core: Core): MidpriceStream {
    return new MidpriceStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link midprice} reports them over the same bars. Each accepted `update` and
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
      throw streamFailure('MIDPRICE advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(high: number, low: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('MIDPRICE update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(high) || !Number.isFinite(low)) {
      throw nonFiniteBar('MIDPRICE update', !Number.isFinite(high) ? 'high' : 'low');
    }
    midpriceStepImpl(this, high, low);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(high: number, low: number): number {
    if (!Number.isFinite(high) || !Number.isFinite(low)) {
      throw nonFiniteBar('MIDPRICE peek', !Number.isFinite(high) ? 'high' : 'low');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
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
    let pkSlot1 = -1;
    let pkVal1 = 0;
    pkSlot0 = sp.today & sp.xMask;
    pkVal0 = inHigh;
    pkSlot1 = sp.today & sp.xMask;
    pkVal1 = inLow;
    tmpHigh = ((sp.today & sp.xMask) !== pkSlot0) ? sp.x_inHigh[sp.today & sp.xMask] : pkVal0;
    tmpLow = ((sp.today & sp.xMask) !== pkSlot1) ? sp.x_inLow[sp.today & sp.xMask] : pkVal1;
    if (highestIdx < sp.trailingIdx) {
      highestIdx = sp.trailingIdx;
      highest = ((highestIdx & sp.xMask) !== pkSlot0) ? sp.x_inHigh[highestIdx & sp.xMask] : pkVal0;
      i = highestIdx;
      while (++i <= sp.today) {
        tmpHigh = ((i & sp.xMask) !== pkSlot0) ? sp.x_inHigh[i & sp.xMask] : pkVal0;
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
      lowest = ((lowestIdx & sp.xMask) !== pkSlot1) ? sp.x_inLow[lowestIdx & sp.xMask] : pkVal1;
      i = lowestIdx;
      while (++i <= sp.today) {
        tmpLow = ((i & sp.xMask) !== pkSlot1) ? sp.x_inLow[i & sp.xMask] : pkVal1;
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
  clone(): MidpriceStream {
    const c = new MidpriceStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.lowest = this.lowest;
    c.highest = this.highest;
    c.trailingIdx = this.trailingIdx;
    c.lowestIdx = this.lowestIdx;
    c.highestIdx = this.highestIdx;
    c.i = this.i;
    c.today = this.today;
    c.xMask = this.xMask;
    c.x_inHigh = this.x_inHigh.slice();
    c.x_inLow = this.x_inLow.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function midpriceStepImpl(sp: MidpriceStream, inHigh: number, inLow: number): void {
  let tmpLow = 0;
  let tmpHigh = 0;
  sp.x_inHigh[sp.today & sp.xMask] = inHigh;
  sp.x_inLow[sp.today & sp.xMask] = inLow;
  tmpHigh = sp.x_inHigh[sp.today & sp.xMask];
  tmpLow = sp.x_inLow[sp.today & sp.xMask];
  if (sp.highestIdx < sp.trailingIdx) {
    sp.highestIdx = sp.trailingIdx;
    sp.highest = sp.x_inHigh[sp.highestIdx & sp.xMask];
    sp.i = sp.highestIdx;
    while (++sp.i <= sp.today) {
      tmpHigh = sp.x_inHigh[sp.i & sp.xMask];
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
    sp.lowest = sp.x_inLow[sp.lowestIdx & sp.xMask];
    sp.i = sp.lowestIdx;
    while (++sp.i <= sp.today) {
      tmpLow = sp.x_inLow[sp.i & sp.xMask];
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

function midpriceOpenImpl(
  sp: MidpriceStream,
  inHigh: Float64Array,
  inLow: Float64Array,
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
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
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
  /* MIDPRICE = (Highest High + Lowest Low)/2
   *
   * This function is equivalent to MEDPRICE when the
   * period is 1.
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
   * The highest high and lowest low of the window are cached with their
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
    tmpHigh = inHigh[today];
    tmpLow = inLow[today];
    if (highestIdx < trailingIdx) {
      highestIdx = trailingIdx;
      highest = inHigh[highestIdx];
      i = highestIdx;
      while (++i <= today) {
        tmpHigh = inHigh[i];
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
      lowest = inLow[lowestIdx];
      i = lowestIdx;
      while (++i <= today) {
        tmpLow = inLow[i];
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
  const capX_inHigh = new Float64Array(physX);
  const capX_inLow = new Float64Array(physX);
  for (let fillJ = historyLen - capX; fillJ < historyLen; fillJ++) {
    capX_inHigh[fillJ & (physX - 1)] = inHigh[fillJ];
    capX_inLow[fillJ & (physX - 1)] = inLow[fillJ];
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
  sp.x_inHigh = capX_inHigh;
  sp.x_inLow = capX_inLow;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function midpriceOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): MidpriceStream {
  const sp = MidpriceStream.of(core);
  const retCode = midpriceOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MIDPRICE openAndFill', inHigh.length, startIdx, midpriceLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && midpriceLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('MIDPRICE openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('MIDPRICE openAndFill', retCode);
}

/** @internal */
export function midpriceOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): MidpriceStream {
  const sp = MidpriceStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = midpriceOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MIDPRICE open', inHigh.length, startIdx, midpriceLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && midpriceLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('MIDPRICE open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('MIDPRICE open', retCode);
}

/** @internal */
export function midpriceOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): MidpriceStream {
  optInTimePeriod = requireInt32('MIDPRICE open', 'timePeriod', optInTimePeriod);
  requireArgument('MIDPRICE open', 'prices.high', inHigh);
  requireHistory('MIDPRICE open', 'prices.high', inHigh.length);
  requireArgument('MIDPRICE open', 'prices.low', inLow);
  requireHistoryLength('MIDPRICE open', 'prices.low', inLow.length, inHigh.length);
  return midpriceOpenInternal(core, float64(inHigh), float64(inLow), 0, optInTimePeriod);
}

/** @internal */
export function midpriceOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): MidpriceStream {
  optInTimePeriod = requireInt32('MIDPRICE openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('MIDPRICE openAndFill', 'prices.high', inHigh);
  requireHistory('MIDPRICE openAndFill', 'prices.high', inHigh.length);
  requireArgument('MIDPRICE openAndFill', 'prices.low', inLow);
  const guardLookback = midpriceLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('MIDPRICE openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('MIDPRICE openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('MIDPRICE openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireOutput('MIDPRICE openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('MIDPRICE openAndFill', 'outReal', 'prices.high');
  }
  if (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('MIDPRICE openAndFill', 'outReal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outReal, inHighF64)) {
    throw sharedMemory('MIDPRICE openAndFill', 'outReal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outReal, inLowF64)) {
    throw sharedMemory('MIDPRICE openAndFill', 'outReal', 'prices.low');
  }
  return midpriceOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live MIDPRICE stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link midprice} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MidpriceOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `midpriceLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range
 *   or histories of different lengths.
 *
 * @example
 * ```ts
 * import { midpriceOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = midpriceOpen({ high, low }, { timePeriod: 14 });
 * const peeked = stream.peek(101.4, 99.1); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1); // the bar closes
 * ```
 */
export function midpriceOpen(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: MidpriceOptions): MidpriceStream {
  requireArgument('MIDPRICE open', 'prices', prices);
  const o = optionsOf('MIDPRICE open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MIDPRICE open', o.core);
  return midpriceOpenPositional(prices.high, prices.low, optInTimePeriod, core);
}

/**
 * {@link midpriceOpen} that also computes the outputs over the whole history in
 * the same pass, bit-identically to {@link midprice}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link MidpriceOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link midprice} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `midpriceLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { midpriceOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = midpriceOpenAndFill({ high, low }, {
 *   timePeriod: 14,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function midpriceOpenAndFill(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: MidpriceOptions): MidpriceResult & { stream: MidpriceStream } {
  requireArgument('MIDPRICE openAndFill', 'prices', prices);
  const o = optionsOf('MIDPRICE openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MIDPRICE openAndFill', o.core);
  const capacity = fillCapacity(midpriceLookbackPositional(optInTimePeriod, core), prices.high, prices.low);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = midpriceOpenAndFillPositional(prices.high, prices.low, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
