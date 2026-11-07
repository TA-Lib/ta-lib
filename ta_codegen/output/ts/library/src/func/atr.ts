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
 *  112400 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  070626 MF,CC  Speed optimization: True Range computed inline in a
 *                single pass (bit-exact, no temporary buffer).
 *  090326 MF,CC  #338 Two-coefficient Wilder step; no divide in the
 *                loop-carried chain.
 */

import { FuncUnstId } from '../enums.ts';
import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function atrLookbackPositional(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  /* The ATR lookback is the sum of:
   *    1 + (optInTimePeriod - 1)
   *
   * Where 1 is for the True Range, and
   * (optInTimePeriod-1) is for the simple
   * moving average.
   */
  return optInTimePeriod + core.unstableCount(FuncUnstId.ATR, ((optInTimePeriod > 1) ? (((((10 * (((2 * optInTimePeriod) | 0) - 1)) | 0) + 1) / 2) | 0) : 0), ((optInTimePeriod > 1) ? (((((19 * (((2 * optInTimePeriod) | 0) - 1)) | 0) + 1) / 2) | 0) : 0));
}

/** @internal */
export function atrDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (atrLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function atrImpl(
  core: Core,
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
  let i = 0;
  let outIdx = 0;
  let today = 0;
  let lookbackTotal = 0;
  let nbATR = 0;
  let prevATR = 0;
  let periodTotal = 0;
  let wAlpha = 0;
  let wBeta = 0;
  let val2 = 0;
  let val3 = 0;
  let greatest = 0;
  let tempCY = 0;
  let tempLT = 0;
  let tempHT = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  /* Average True Range is the greatest of the following:
   *
   *  val1 = distance from today's high to today's low.
   *  val2 = distance from yesterday's close to today's high.
   *  val3 = distance from yesterday's close to today's low.
   *
   * These value are averaged for the specified period using
   * Wilder method. This method have an unstable period comparable
   * to and Exponential Moving Average (EMA).
   */
  outBegIdx.value = 0;
  outNBElement.value = 0;
  /* Adjust startIdx to account for the lookback period. */
  lookbackTotal = atrLookbackPositional(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  /* wAlpha is derived FROM wBeta, never the reverse: only that order makes
   * wAlpha + wBeta exactly 1 (Sterbenz -- wBeta lands in [0.5, 1)), and it
   * measures closer to the exact recursion than the 1/period-first spelling
   * at nearly every period. The order is a gated contract, not a preference:
   * swapping it reddens the frozen-release comparison, and breaks the
   * bit-for-bit identity TA_RMA(TA_TRANGE(h,l,c),n) == TA_ATR(n).
   * The pair is exactly (1, 0) at period 1 -- hence no period-1 arm.
   */
  wBeta = (optInTimePeriod - 1) / optInTimePeriod;
  wAlpha = 1.0 - wBeta;
  /* The True Range of each bar is computed inline in a single
   * pass. No temporary buffer is needed.
   *
   * The arithmetic order below is the bit-exactness contract
   * (do not reorder):
   *  - True Range: start from high-low, then compare/replace
   *    with the two previous-close distances, in that order.
   *  - Seed: the first 'period' True Range values are summed,
   *    accumulated from 0.0 in input order, then divided by
   *    the period.
   *  - Wilder smoothing: ONE statement. Splitting it back
   *    unfuses the multiply-add and puts a second latency on
   *    the recurrence's dependency chain.
   *
   * In-place (outReal being one of the input arrays) is
   * supported: each output is written only after every input
   * read at or before its bar, and the output index is always
   * smaller than the bar index of any remaining read.
   */
  /* The first True Range needs the two price bars at
   * startIdx-lookbackTotal+1 (a previous close is consumed).
   */
  today = startIdx - lookbackTotal + 1;
  /* Seed the ATR with a simple average of the True Range
   * for the first 'period' bars.
   */
  periodTotal = 0.0;
  i = optInTimePeriod;
  while (i-- > 0) {
    /* Find the greatest of the 3 values. */
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    periodTotal += greatest;
    today += 1;
  }
  prevATR = periodTotal / optInTimePeriod;
  /* Skip the unstable period. */
  i = lookbackTotal - optInTimePeriod;
  while (i !== 0) {
    /* Find the greatest of the 3 values. */
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    prevATR = fma(wBeta, prevATR, wAlpha * greatest);
    today += 1;
    i -= 1;
  }
  /* Now start to write the final ATR in the caller
   * provided outReal.
   */
  outIdx = 1;
  outReal[0] = prevATR;
  /* Now do the number of requested ATR. */
  nbATR = endIdx - startIdx + 1;
  while (--nbATR !== 0) {
    /* Find the greatest of the 3 values. */
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    prevATR = fma(wBeta, prevATR, wAlpha * greatest);
    outReal[outIdx++] = prevATR;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000']] as const;

/** @internal */
export function atrPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('ATR', 'startIdx', startIdx);
  endIdx = requireInt32('ATR', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('ATR', 'timePeriod', optInTimePeriod);
  requireIndexRange('ATR', startIdx, endIdx);
  const guardLookback = atrLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('ATR', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('ATR', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('ATR', 'prices.high', inHigh, guardInLen);
  requireLength('ATR', 'prices.low', inLow, guardInLen);
  requireLength('ATR', 'prices.close', inClose, guardInLen);
  requireOutput('ATR', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outReal, inHighF64)) {
    throw sharedMemory('ATR', 'outReal', 'prices.high');
  }
  if (overlapsPartly(outReal, inLowF64)) {
    throw sharedMemory('ATR', 'outReal', 'prices.low');
  }
  if (overlapsPartly(outReal, inCloseF64)) {
    throw sharedMemory('ATR', 'outReal', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = atrImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('ATR', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link atr}, {@link atrLookback}, {@link atrDisplayShift},
 * {@link atrOpen} and {@link atrOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface AtrOptions {
  /**
   * Smoothing period (default 14; range 1 to 100000; `Core.INTEGER_DEFAULT` selects
   * the default).
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

/** What {@link atr} returns, and {@link atrOpenAndFill} with its stream. */
export interface AtrResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /** Average True Range value. `values[k]` is the value at input bar `begIdx + k`. */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link atr} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AtrOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function atrLookback(options?: AtrOptions): number {
  const o = optionsOf('ATR lookback', options);
  return atrLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('ATR lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link atr}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AtrOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function atrDisplayShift(outputIdx: number, options?: AtrOptions): number {
  const o = optionsOf('ATR displayShift', options);
  return atrDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('ATR displayShift', o.core));
}

/**
 * Wilder-smoothed average of the True Range over a period, measuring price
 * volatility regardless of direction. Higher ATR means greater volatility; no
 * directional bias.
 *
 * Formula and more info at
 * [ta-lib.org/functions/atr](https://ta-lib.org/functions/atr).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link atrLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `high`: High
 *   price of each bar. `low`: Low price of each bar. `close`: Close price of each
 *   bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link AtrOptions}.
 * @returns `begIdx` and the values; see {@link AtrResult}. Each output is a view
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
 * import { atr } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { begIdx, values } = atr({ high, low, close }, { timePeriod: 14 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`trange`](https://ta-lib.org/functions/trange)
 * @see [`natr`](https://ta-lib.org/functions/natr)
 * @see [`sma`](https://ta-lib.org/functions/sma)
 * @see [`ema`](https://ta-lib.org/functions/ema)
 */
export function atr(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: AtrOptions): AtrResult {
  requireArgument('ATR', 'prices', prices);
  const o = optionsOf('ATR', options);
  if (o.endIdx === undefined) {
    requireSeries('ATR', 'prices.high', prices.high);
    requireSeries('ATR', 'prices.low', prices.low);
    requireSeries('ATR', 'prices.close', prices.close);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('ATR', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, atrLookbackPositional(optInTimePeriod, core), prices.high, prices.low, prices.close);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = atrPositional(startIdx, endIdx, prices.high, prices.low, prices.close, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live ATR stream: one value per closed bar, bit-identical to {@link atr} over
 * the same series. Open it with {@link atrOpen} or {@link atrOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/atr](https://ta-lib.org/functions/atr).
 */
export class AtrStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  prevATR: number = 0;
  /** @internal */
  wAlpha: number = 0;
  /** @internal */
  wBeta: number = 0;
  /** @internal */
  lag1_inClose: number = 0;
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
  static of(core: Core): AtrStream {
    return new AtrStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link atr} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('ATR advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(high: number, low: number, close: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('ATR update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('ATR update', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    atrStepImpl(this, high, low, close);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(high: number, low: number, close: number): number {
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('ATR peek', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
    let val2 = 0;
    let val3 = 0;
    let greatest = 0;
    let tempCY = 0;
    let tempLT = 0;
    let tempHT = 0;
    let cur_outReal = 0;
    let prevATR = sp.prevATR;
    /* Find the greatest of the 3 values. */
    tempLT = inLow;
    tempHT = inHigh;
    tempCY = sp.lag1_inClose;
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    prevATR = fma(sp.wBeta, prevATR, sp.wAlpha * greatest);
    cur_outReal = prevATR;
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
  clone(): AtrStream {
    const c = new AtrStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.prevATR = this.prevATR;
    c.wAlpha = this.wAlpha;
    c.wBeta = this.wBeta;
    c.lag1_inClose = this.lag1_inClose;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function atrStepImpl(sp: AtrStream, inHigh: number, inLow: number, inClose: number): void {
  let val2 = 0;
  let val3 = 0;
  let greatest = 0;
  let tempCY = 0;
  let tempLT = 0;
  let tempHT = 0;
  /* Find the greatest of the 3 values. */
  tempLT = inLow;
  tempHT = inHigh;
  tempCY = sp.lag1_inClose;
  greatest = tempHT - tempLT;
  /* val1 */
  val2 = Math.abs(tempCY - tempHT);
  if (val2 > greatest) {
    greatest = val2;
  }
  val3 = Math.abs(tempCY - tempLT);
  if (val3 > greatest) {
    greatest = val3;
  }
  sp.prevATR = fma(sp.wBeta, sp.prevATR, sp.wAlpha * greatest);
  sp.cur_outReal = sp.prevATR;
  sp.lag1_inClose = inClose;
}

function atrOpenImpl(
  sp: AtrStream,
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
  const core = sp.core;
  let i = 0;
  let outIdx = 0;
  let today = 0;
  let lookbackTotal = 0;
  let nbATR = 0;
  let prevATR = 0;
  let periodTotal = 0;
  let wAlpha = 0;
  let wBeta = 0;
  let val2 = 0;
  let val3 = 0;
  let greatest = 0;
  let tempCY = 0;
  let tempLT = 0;
  let tempHT = 0;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length || inClose.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Average True Range is the greatest of the following:
   *
   *  val1 = distance from today's high to today's low.
   *  val2 = distance from yesterday's close to today's high.
   *  val3 = distance from yesterday's close to today's low.
   *
   * These value are averaged for the specified period using
   * Wilder method. This method have an unstable period comparable
   * to and Exponential Moving Average (EMA).
   */
  outBegIdx.value = 0;
  outNBElement.value = 0;
  /* Adjust startIdx to account for the lookback period. */
  lookbackTotal = atrLookbackPositional(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* wAlpha is derived FROM wBeta, never the reverse: only that order makes
   * wAlpha + wBeta exactly 1 (Sterbenz -- wBeta lands in [0.5, 1)), and it
   * measures closer to the exact recursion than the 1/period-first spelling
   * at nearly every period. The order is a gated contract, not a preference:
   * swapping it reddens the frozen-release comparison, and breaks the
   * bit-for-bit identity TA_RMA(TA_TRANGE(h,l,c),n) == TA_ATR(n).
   * The pair is exactly (1, 0) at period 1 -- hence no period-1 arm.
   */
  wBeta = (optInTimePeriod - 1) / optInTimePeriod;
  wAlpha = 1.0 - wBeta;
  /* The True Range of each bar is computed inline in a single
   * pass. No temporary buffer is needed.
   *
   * The arithmetic order below is the bit-exactness contract
   * (do not reorder):
   *  - True Range: start from high-low, then compare/replace
   *    with the two previous-close distances, in that order.
   *  - Seed: the first 'period' True Range values are summed,
   *    accumulated from 0.0 in input order, then divided by
   *    the period.
   *  - Wilder smoothing: ONE statement. Splitting it back
   *    unfuses the multiply-add and puts a second latency on
   *    the recurrence's dependency chain.
   *
   * In-place (outReal being one of the input arrays) is
   * supported: each output is written only after every input
   * read at or before its bar, and the output index is always
   * smaller than the bar index of any remaining read.
   */
  /* The first True Range needs the two price bars at
   * startIdx-lookbackTotal+1 (a previous close is consumed).
   */
  today = startIdx - lookbackTotal + 1;
  /* Seed the ATR with a simple average of the True Range
   * for the first 'period' bars.
   */
  periodTotal = 0.0;
  i = optInTimePeriod;
  while (i-- > 0) {
    /* Find the greatest of the 3 values. */
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    periodTotal += greatest;
    today += 1;
  }
  prevATR = periodTotal / optInTimePeriod;
  /* Skip the unstable period. */
  i = lookbackTotal - optInTimePeriod;
  while (i !== 0) {
    /* Find the greatest of the 3 values. */
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    prevATR = fma(wBeta, prevATR, wAlpha * greatest);
    today += 1;
    i -= 1;
  }
  /* Now start to write the final ATR in the caller
   * provided outReal.
   */
  outIdx = 1;
  outReal[((0 * outStride) | 0)] = prevATR;
  /* Now do the number of requested ATR. */
  nbATR = endIdx - startIdx + 1;
  while (--nbATR !== 0) {
    /* Find the greatest of the 3 values. */
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    prevATR = fma(wBeta, prevATR, wAlpha * greatest);
    outReal[((outIdx++ * outStride) | 0)] = prevATR;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  sp.optInTimePeriod = optInTimePeriod;
  sp.prevATR = prevATR;
  sp.wAlpha = wAlpha;
  sp.wBeta = wBeta;
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function atrOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): AtrStream {
  const sp = AtrStream.of(core);
  const retCode = atrOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ATR openAndFill', inHigh.length, startIdx, atrLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && atrLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('ATR openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('ATR openAndFill', retCode);
}

/** @internal */
export function atrOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): AtrStream {
  const sp = AtrStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = atrOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ATR open', inHigh.length, startIdx, atrLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && atrLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('ATR open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('ATR open', retCode);
}

/** @internal */
export function atrOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): AtrStream {
  optInTimePeriod = requireInt32('ATR open', 'timePeriod', optInTimePeriod);
  requireArgument('ATR open', 'prices.high', inHigh);
  requireHistory('ATR open', 'prices.high', inHigh.length);
  requireArgument('ATR open', 'prices.low', inLow);
  requireArgument('ATR open', 'prices.close', inClose);
  requireHistoryLength('ATR open', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('ATR open', 'prices.close', inClose.length, inHigh.length);
  return atrOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInTimePeriod);
}

/** @internal */
export function atrOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): AtrStream {
  optInTimePeriod = requireInt32('ATR openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('ATR openAndFill', 'prices.high', inHigh);
  requireHistory('ATR openAndFill', 'prices.high', inHigh.length);
  requireArgument('ATR openAndFill', 'prices.low', inLow);
  requireArgument('ATR openAndFill', 'prices.close', inClose);
  const guardLookback = atrLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('ATR openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('ATR openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('ATR openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('ATR openAndFill', 'prices.close', inClose.length, inHigh.length);
  requireOutput('ATR openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('ATR openAndFill', 'outReal', 'prices.high');
  }
  if (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('ATR openAndFill', 'outReal', 'prices.low');
  }
  if (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('ATR openAndFill', 'outReal', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outReal, inHighF64)) {
    throw sharedMemory('ATR openAndFill', 'outReal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outReal, inLowF64)) {
    throw sharedMemory('ATR openAndFill', 'outReal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outReal, inCloseF64)) {
    throw sharedMemory('ATR openAndFill', 'outReal', 'prices.close');
  }
  return atrOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live ATR stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link atr} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AtrOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `atrLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or
 *   histories of different lengths.
 *
 * @example
 * ```ts
 * import { atrOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = atrOpen({ high, low, close }, { timePeriod: 14 });
 * const peeked = stream.peek(101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function atrOpen(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: AtrOptions): AtrStream {
  requireArgument('ATR open', 'prices', prices);
  const o = optionsOf('ATR open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('ATR open', o.core);
  return atrOpenPositional(prices.high, prices.low, prices.close, optInTimePeriod, core);
}

/**
 * {@link atrOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link atr}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link AtrOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link atr} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `atrLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { atrOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = atrOpenAndFill({ high, low, close }, {
 *   timePeriod: 14,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function atrOpenAndFill(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: AtrOptions): AtrResult & { stream: AtrStream } {
  requireArgument('ATR openAndFill', 'prices', prices);
  const o = optionsOf('ATR openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('ATR openAndFill', o.core);
  const capacity = fillCapacity(atrLookbackPositional(optInTimePeriod, core), prices.high, prices.low, prices.close);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = atrOpenAndFillPositional(prices.high, prices.low, prices.close, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
