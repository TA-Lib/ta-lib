/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 *
 * Change history:
 *
 *  MMDDYY BY    Description
 *  -------------------------------------------------------------------
 *  010802 MF    Template creation.
 *  052603 MF    Adapt code to compile with .NET Managed C++
 *  090626 MF,CC Fix #395. Divide by the range, scale after, then clamp: the
 *               hoisted `(highest-lowest)/-100.0` underflowed to 0.0 on a
 *               denormal range that the guard still called "not flat", and
 *               the pre-scaled divisor left the documented [-100,0] bound.
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function willrLookbackPositional(optInTimePeriod: number, _core: Core = Core.DEFAULT): number {
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
export function willrDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (willrLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function willrImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
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
  let tmp = 0;
  let tempReal = 0;
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
   * into a per-bar automaton, so the streaming tier runs willr_ALT1
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
      tmp = inHigh[i];
      if (tmp > highest) {
        highest = tmp;
      }
      tmp = inLow[i];
      if (tmp < lowest) {
        lowest = tmp;
      }
      sufHighest[i - blockStart] = highest;
      sufLowest[i - blockStart] = lowest;
    }
    highest = sufHighest[0];
    lowest = sufLowest[0];
    /* Divide by the range itself and scale after: the guard has to test the
     * very expression the division uses, or a scaling step can carry a
     * guarded-non-zero into a zero divisor. It is also what puts a close on
     * the period low at exactly -100.
     *
     * The band is the range against ITS OWN two extremes, not a fixed
     * constant: the range carries the quote unit, so a constant answers
     * "flat" for every window of an instrument quoted below it (issue #253).
     * It absorbs the machine-flat window an exact test would divide into
     * [-100,0] noise (issue #107 / STOCH).
     *
     * The clamp is unreachable while lowest <= close <= highest -- the
     * quotient is <= 1 under any rounding mode. Its domain is the close
     * outside its own bar, which nothing here validates.
     */
    if (!(Math.abs(highest - lowest) <= 0.00000000000001 * (Math.abs(highest) + Math.abs(lowest)))) {
      tempReal = (highest - inClose[today]) / (highest - lowest) * -100.0;
      if (tempReal > 0.0) {
        tempReal = 0.0;
      } else if (tempReal < -100.0) {
        tempReal = -100.0;
      }
      outReal[outIdx++] = tempReal;
    } else {
      outReal[outIdx++] = 0.0;
    }
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
        tmp = inHigh[blockNext + i];
        if (tmp > highest) {
          highest = tmp;
        }
        tmp = inLow[blockNext + i];
        if (tmp < lowest) {
          lowest = tmp;
        }
        preHighest[i] = highest;
        preLowest[i] = lowest;
        i += 1;
      }
      /* Combine and emit. The suffix half is the older one, so
       * preferring it on a tie keeps the earliest-wins rule. The
       * bar being emitted for offset m is today+m-1: 'today' was
       * advanced once above and is not touched inside this loop.
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
        if (!(Math.abs(highest - lowest) <= 0.00000000000001 * (Math.abs(highest) + Math.abs(lowest)))) {
          tempReal = (highest - inClose[today + m - 1]) / (highest - lowest) * -100.0;
          if (tempReal > 0.0) {
            tempReal = 0.0;
          } else if (tempReal < -100.0) {
            tempReal = -100.0;
          }
          outReal[outIdx++] = tempReal;
        } else {
          outReal[outIdx++] = 0.0;
        }
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
export function willrPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('WILLR', 'startIdx', startIdx);
  endIdx = requireInt32('WILLR', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('WILLR', 'timePeriod', optInTimePeriod);
  requireIndexRange('WILLR', startIdx, endIdx);
  const guardLookback = willrLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('WILLR', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('WILLR', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('WILLR', 'prices.high', inHigh, guardInLen);
  requireLength('WILLR', 'prices.low', inLow, guardInLen);
  requireLength('WILLR', 'prices.close', inClose, guardInLen);
  requireOutput('WILLR', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outReal, inHighF64)) {
    throw sharedMemory('WILLR', 'outReal', 'prices.high');
  }
  if (overlapsPartly(outReal, inLowF64)) {
    throw sharedMemory('WILLR', 'outReal', 'prices.low');
  }
  if (overlapsPartly(outReal, inCloseF64)) {
    throw sharedMemory('WILLR', 'outReal', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = willrImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('WILLR', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link willr}, {@link willrLookback}, {@link willrDisplayShift},
 * {@link willrOpen} and {@link willrOpenAndFill}. Every key is optional: an
 * omitted or `undefined` key takes its default, and a call ignores the keys it has
 * no use for.
 */
export interface WillrOptions {
  /**
   * Lookback bars for the high/low range (default 14; range 2 to 100000;
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
  outReal?: Float64Array | undefined;
}

/** What {@link willr} returns, and {@link willrOpenAndFill} with its stream. */
export interface WillrResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Williams' %R value in \[-100, 0]. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link willr} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link WillrOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function willrLookback(options?: WillrOptions): number {
  const o = optionsOf('WILLR lookback', options);
  return willrLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('WILLR lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link willr}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link WillrOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function willrDisplayShift(outputIdx: number, options?: WillrOptions): number {
  const o = optionsOf('WILLR displayShift', options);
  return willrDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('WILLR displayShift', o.core));
}

/**
 * Williams' %R momentum oscillator over a rolling period, bounded in \[-100, 0].
 * Measures where the current close sits relative to the high-low range of the last
 * N bars. Near 0 = close at period high (overbought); near -100 = close at period
 * low (oversold).
 *
 * Formula and more info at
 * [ta-lib.org/functions/willr](https://ta-lib.org/functions/willr).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link willrLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `high`: High
 *   price of each bar. `low`: Low price of each bar. `close`: Close price of each
 *   bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link WillrOptions}.
 * @returns `begIdx` and the values; see {@link WillrResult}. Each output is a view
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
 * import { willr } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { begIdx, values } = willr({ high, low, close }, { timePeriod: 14 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`stoch`](https://ta-lib.org/functions/stoch)
 * @see [`stochf`](https://ta-lib.org/functions/stochf)
 * @see [`minmax`](https://ta-lib.org/functions/minmax)
 */
export function willr(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: WillrOptions): WillrResult {
  requireArgument('WILLR', 'prices', prices);
  const o = optionsOf('WILLR', options);
  if (o.endIdx === undefined) {
    requireSeries('WILLR', 'prices.high', prices.high);
    requireSeries('WILLR', 'prices.low', prices.low);
    requireSeries('WILLR', 'prices.close', prices.close);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('WILLR', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, willrLookbackPositional(optInTimePeriod, core), prices.high, prices.low, prices.close);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = willrPositional(startIdx, endIdx, prices.high, prices.low, prices.close, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/* Using willr_ALT1 for TA_ALT={STREAM,ALL_LANGUAGES} */

/**
 * A live WILLR stream: one value per closed bar, bit-identical to {@link willr}
 * over the same series. Open it with {@link willrOpen} or
 * {@link willrOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/willr](https://ta-lib.org/functions/willr).
 */
export class WillrStream {
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
  x_inClose: Float64Array = new Float64Array(1);
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
  static of(core: Core): WillrStream {
    return new WillrStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link willr} reports them over the same bars. Each accepted `update` and each
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
      throw failure('WILLR advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number, inClose: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('WILLR update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('WILLR update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    willrStepImpl(this, inHigh, inLow, inClose);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number, inClose: number): number {
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('WILLR peek', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let tmp = 0;
    let tempReal = 0;
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
    let pkSlot2 = -1;
    let pkVal2 = 0;
    pkSlot0 = sp.today & sp.xMask;
    pkVal0 = inHigh;
    pkSlot1 = sp.today & sp.xMask;
    pkVal1 = inLow;
    pkSlot2 = sp.today & sp.xMask;
    pkVal2 = inClose;
    /* Set the lowest low */
    tmp = ((sp.today & sp.xMask) !== pkSlot1) ? sp.x_inLow[sp.today & sp.xMask] : pkVal1;
    if (lowestIdx < sp.trailingIdx) {
      lowestIdx = sp.trailingIdx;
      lowest = ((lowestIdx & sp.xMask) !== pkSlot1) ? sp.x_inLow[lowestIdx & sp.xMask] : pkVal1;
      i = lowestIdx;
      while (++i <= sp.today) {
        tmp = ((i & sp.xMask) !== pkSlot1) ? sp.x_inLow[i & sp.xMask] : pkVal1;
        if (tmp < lowest) {
          lowestIdx = i;
          lowest = tmp;
        }
      }
    } else if (tmp <= lowest) {
      lowestIdx = sp.today;
      lowest = tmp;
    }
    /* Set the highest high */
    tmp = ((sp.today & sp.xMask) !== pkSlot0) ? sp.x_inHigh[sp.today & sp.xMask] : pkVal0;
    if (highestIdx < sp.trailingIdx) {
      highestIdx = sp.trailingIdx;
      highest = ((highestIdx & sp.xMask) !== pkSlot0) ? sp.x_inHigh[highestIdx & sp.xMask] : pkVal0;
      i = highestIdx;
      while (++i <= sp.today) {
        tmp = ((i & sp.xMask) !== pkSlot0) ? sp.x_inHigh[i & sp.xMask] : pkVal0;
        if (tmp > highest) {
          highestIdx = i;
          highest = tmp;
        }
      }
    } else if (tmp >= highest) {
      highestIdx = sp.today;
      highest = tmp;
    }
    /* Same rule, band and clamp as the block scan above. */
    if (!(Math.abs(highest - lowest) <= 0.00000000000001 * (Math.abs(highest) + Math.abs(lowest)))) {
      tempReal = (highest - (((sp.today & sp.xMask) !== pkSlot2) ? sp.x_inClose[sp.today & sp.xMask] : pkVal2)) / (highest - lowest) * -100.0;
      if (tempReal > 0.0) {
        tempReal = 0.0;
      } else if (tempReal < -100.0) {
        tempReal = -100.0;
      }
      cur_outReal = tempReal;
    } else {
      cur_outReal = 0.0;
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
  clone(): WillrStream {
    const c = new WillrStream(this.core);
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
    c.x_inClose = this.x_inClose.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function willrStepImpl(sp: WillrStream, inHigh: number, inLow: number, inClose: number): void {
  let tmp = 0;
  let tempReal = 0;
  sp.x_inHigh[sp.today & sp.xMask] = inHigh;
  sp.x_inLow[sp.today & sp.xMask] = inLow;
  sp.x_inClose[sp.today & sp.xMask] = inClose;
  /* Set the lowest low */
  tmp = sp.x_inLow[sp.today & sp.xMask];
  if (sp.lowestIdx < sp.trailingIdx) {
    sp.lowestIdx = sp.trailingIdx;
    sp.lowest = sp.x_inLow[sp.lowestIdx & sp.xMask];
    sp.i = sp.lowestIdx;
    while (++sp.i <= sp.today) {
      tmp = sp.x_inLow[sp.i & sp.xMask];
      if (tmp < sp.lowest) {
        sp.lowestIdx = sp.i;
        sp.lowest = tmp;
      }
    }
  } else if (tmp <= sp.lowest) {
    sp.lowestIdx = sp.today;
    sp.lowest = tmp;
  }
  /* Set the highest high */
  tmp = sp.x_inHigh[sp.today & sp.xMask];
  if (sp.highestIdx < sp.trailingIdx) {
    sp.highestIdx = sp.trailingIdx;
    sp.highest = sp.x_inHigh[sp.highestIdx & sp.xMask];
    sp.i = sp.highestIdx;
    while (++sp.i <= sp.today) {
      tmp = sp.x_inHigh[sp.i & sp.xMask];
      if (tmp > sp.highest) {
        sp.highestIdx = sp.i;
        sp.highest = tmp;
      }
    }
  } else if (tmp >= sp.highest) {
    sp.highestIdx = sp.today;
    sp.highest = tmp;
  }
  /* Same rule, band and clamp as the block scan above. */
  if (!(Math.abs(sp.highest - sp.lowest) <= 0.00000000000001 * (Math.abs(sp.highest) + Math.abs(sp.lowest)))) {
    tempReal = (sp.highest - sp.x_inClose[sp.today & sp.xMask]) / (sp.highest - sp.lowest) * -100.0;
    if (tempReal > 0.0) {
      tempReal = 0.0;
    } else if (tempReal < -100.0) {
      tempReal = -100.0;
    }
    sp.cur_outReal = tempReal;
  } else {
    sp.cur_outReal = 0.0;
  }
  sp.trailingIdx += 1;
  sp.today += 1;
}

function willrOpenImpl(
  sp: WillrStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let lowest = 0;
  let highest = 0;
  let tmp = 0;
  let tempReal = 0;
  let outIdx = 0;
  let nbInitialElementNeeded = 0;
  let trailingIdx = 0;
  let lowestIdx = 0;
  let highestIdx = 0;
  let today = 0;
  let i = 0;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length || inClose.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
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
  lowestIdx = highestIdx;
  lowest = 0.0;
  highest = lowest;
  while (today <= endIdx) {
    /* Set the lowest low */
    tmp = inLow[today];
    if (lowestIdx < trailingIdx) {
      lowestIdx = trailingIdx;
      lowest = inLow[lowestIdx];
      i = lowestIdx;
      while (++i <= today) {
        tmp = inLow[i];
        if (tmp < lowest) {
          lowestIdx = i;
          lowest = tmp;
        }
      }
    } else if (tmp <= lowest) {
      lowestIdx = today;
      lowest = tmp;
    }
    /* Set the highest high */
    tmp = inHigh[today];
    if (highestIdx < trailingIdx) {
      highestIdx = trailingIdx;
      highest = inHigh[highestIdx];
      i = highestIdx;
      while (++i <= today) {
        tmp = inHigh[i];
        if (tmp > highest) {
          highestIdx = i;
          highest = tmp;
        }
      }
    } else if (tmp >= highest) {
      highestIdx = today;
      highest = tmp;
    }
    /* Same rule, band and clamp as the block scan above. */
    if (!(Math.abs(highest - lowest) <= 0.00000000000001 * (Math.abs(highest) + Math.abs(lowest)))) {
      tempReal = (highest - inClose[today]) / (highest - lowest) * -100.0;
      if (tempReal > 0.0) {
        tempReal = 0.0;
      } else if (tempReal < -100.0) {
        tempReal = -100.0;
      }
      outReal[((outIdx++ * outStride) | 0)] = tempReal;
    } else {
      outReal[((outIdx++ * outStride) | 0)] = 0.0;
    }
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
  const capX_inClose = new Float64Array(physX);
  for (let fillJ = historyLen - capX; fillJ < historyLen; fillJ++) {
    capX_inHigh[fillJ & (physX - 1)] = inHigh[fillJ];
    capX_inLow[fillJ & (physX - 1)] = inLow[fillJ];
    capX_inClose[fillJ & (physX - 1)] = inClose[fillJ];
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
  sp.x_inClose = capX_inClose;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function willrOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): WillrStream {
  const sp = WillrStream.of(core);
  const retCode = willrOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('WILLR openAndFill', inHigh.length, startIdx, willrLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && willrLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('WILLR openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('WILLR openAndFill', retCode);
}

/** @internal */
export function willrOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): WillrStream {
  const sp = WillrStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = willrOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('WILLR open', inHigh.length, startIdx, willrLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && willrLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('WILLR open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('WILLR open', retCode);
}

/** @internal */
export function willrOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): WillrStream {
  optInTimePeriod = requireInt32('WILLR open', 'timePeriod', optInTimePeriod);
  requireArgument('WILLR open', 'prices.high', inHigh);
  requireHistory('WILLR open', inHigh.length);
  requireArgument('WILLR open', 'prices.low', inLow);
  requireArgument('WILLR open', 'prices.close', inClose);
  requireHistoryLength('WILLR open', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('WILLR open', 'prices.close', inClose.length, inHigh.length);
  return willrOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInTimePeriod);
}

/** @internal */
export function willrOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): WillrStream {
  optInTimePeriod = requireInt32('WILLR openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('WILLR openAndFill', 'prices.high', inHigh);
  requireHistory('WILLR openAndFill', inHigh.length);
  requireArgument('WILLR openAndFill', 'prices.low', inLow);
  requireArgument('WILLR openAndFill', 'prices.close', inClose);
  const guardLookback = willrLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('WILLR openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('WILLR openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('WILLR openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('WILLR openAndFill', 'prices.close', inClose.length, inHigh.length);
  requireOutput('WILLR openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('WILLR openAndFill', 'outReal', 'prices.high');
  }
  if (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('WILLR openAndFill', 'outReal', 'prices.low');
  }
  if (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('WILLR openAndFill', 'outReal', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outReal, inHighF64)) {
    throw sharedMemory('WILLR openAndFill', 'outReal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outReal, inLowF64)) {
    throw sharedMemory('WILLR openAndFill', 'outReal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outReal, inCloseF64)) {
    throw sharedMemory('WILLR openAndFill', 'outReal', 'prices.close');
  }
  return willrOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live WILLR stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link willr} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link WillrOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `willrLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range
 *   or histories of different lengths.
 *
 * @example
 * ```ts
 * import { willrOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = willrOpen({ high, low, close }, { timePeriod: 14 });
 * const peeked = stream.peek(101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function willrOpen(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: WillrOptions): WillrStream {
  requireArgument('WILLR open', 'prices', prices);
  const o = optionsOf('WILLR open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('WILLR open', o.core);
  return willrOpenPositional(prices.high, prices.low, prices.close, optInTimePeriod, core);
}

/**
 * {@link willrOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link willr}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link WillrOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link willr} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `willrLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { willrOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = willrOpenAndFill({ high, low, close }, {
 *   timePeriod: 14,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function willrOpenAndFill(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: WillrOptions): WillrResult & { stream: WillrStream } {
  requireArgument('WILLR openAndFill', 'prices', prices);
  const o = optionsOf('WILLR openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('WILLR openAndFill', o.core);
  const capacity = fillCapacity(willrLookbackPositional(optInTimePeriod, core), prices.high, prices.low, prices.close);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = willrOpenAndFillPositional(prices.high, prices.low, prices.close, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
