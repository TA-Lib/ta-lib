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
 *  090526 MF,CC  First version (issue #373).
 *  100626 MF,CC  Auto rule sized on the pole at 1/2: ceil(13*K/9) (#492).
 */

import { FuncUnstId } from '../enums.ts';
import { badSink, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function haLookbackPositional(core: Core = Core.DEFAULT): number {
  /* 13/9 is 1/ln(2) rounded up: the open is the only state, and it shows
   * the whole first difference at its first bar.
   */
  return core.unstableCount(FuncUnstId.HA, (((((13 * 10) | 0) + 8) / 9) | 0), (((((13 * 19) | 0) + 8) / 9) | 0));
}

/** @internal */
export function haDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 4) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function haImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outHAOpen: Float64Array,
  outHAHigh: Float64Array,
  outHALow: Float64Array,
  outHAClose: Float64Array,
): RetCode {
  let i = 0;
  let outIdx = 0;
  let today = 0;
  let lookbackTotal = 0;
  let haOpen = 0;
  let haClose = 0;
  let haHigh = 0;
  let haLow = 0;
  let tempOpen = 0;
  let tempHigh = 0;
  let tempLow = 0;
  let tempClose = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if ((outHAOpen.buffer === outHAHigh.buffer && outHAOpen.byteOffset === outHAHigh.byteOffset) || (outHAOpen.buffer === outHALow.buffer && outHAOpen.byteOffset === outHALow.byteOffset) || (outHAOpen.buffer === outHAClose.buffer && outHAOpen.byteOffset === outHAClose.byteOffset) || (outHAHigh.buffer === outHALow.buffer && outHAHigh.byteOffset === outHALow.byteOffset) || (outHAHigh.buffer === outHAClose.buffer && outHAHigh.byteOffset === outHAClose.byteOffset) || (outHALow.buffer === outHAClose.buffer && outHALow.byteOffset === outHAClose.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = haLookbackPositional(core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  /* The summation order ((O+H)+L)+C is the bit-exactness contract with every
   * external implementation of this indicator. TA_AVGPRICE is documented as
   * the same quantity but sums ((H+L)+C)+O, which is a different double on
   * some bars; the two are deliberately not unified.
   */
  today = startIdx - lookbackTotal;
  haOpen = (inOpen[today] + inClose[today]) / 2.0;
  haClose = (inOpen[today] + inHigh[today] + inLow[today] + inClose[today]) / 4.0;
  today += 1;
  /* Skip the unstable period. */
  i = lookbackTotal;
  while (i !== 0) {
    /* haOpen consumes the PREVIOUS candle on both sides of the midpoint, so
     * it must advance before haClose is overwritten. Swapping these two
     * still yields a plausible smoothed series.
     */
    haOpen = (haOpen + haClose) / 2.0;
    haClose = (inOpen[today] + inHigh[today] + inLow[today] + inClose[today]) / 4.0;
    today += 1;
    i -= 1;
  }
  tempHigh = inHigh[startIdx];
  tempLow = inLow[startIdx];
  /* The three-way extremum is spelled with plain comparisons, never the
   * max/min builtins: C's macros return their SECOND operand on a tie and
   * Rust's and Java's builtins do not, so a bar of signed zeros
   * emits different bytes per backend and the cross-language gate compares
   * bytes. Measured on (O,H,L,C) = (-0.0, +0.0, -0.0, -0.0).
   */
  haHigh = tempHigh;
  if (haOpen > haHigh) {
    haHigh = haOpen;
  }
  if (haClose > haHigh) {
    haHigh = haClose;
  }
  haLow = tempLow;
  if (haOpen < haLow) {
    haLow = haOpen;
  }
  if (haClose < haLow) {
    haLow = haClose;
  }
  outHAOpen[0] = haOpen;
  outHAHigh[0] = haHigh;
  outHALow[0] = haLow;
  outHAClose[0] = haClose;
  outIdx = 1;
  while (today <= endIdx) {
    /* Every price of the bar is read into a local before any output is
     * written, which is what lets an output alias any input: at startIdx 0
     * the store lands on the very slot the high and low were just read from.
     */
    tempOpen = inOpen[today];
    tempHigh = inHigh[today];
    tempLow = inLow[today];
    tempClose = inClose[today];
    haOpen = (haOpen + haClose) / 2.0;
    haClose = (tempOpen + tempHigh + tempLow + tempClose) / 4.0;
    haHigh = tempHigh;
    if (haOpen > haHigh) {
      haHigh = haOpen;
    }
    if (haClose > haHigh) {
      haHigh = haClose;
    }
    haLow = tempLow;
    if (haOpen < haLow) {
      haLow = haOpen;
    }
    if (haClose < haLow) {
      haLow = haClose;
    }
    outHAOpen[outIdx] = haOpen;
    outHAHigh[outIdx] = haHigh;
    outHALow[outIdx] = haLow;
    outHAClose[outIdx] = haClose;
    outIdx += 1;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function haPositional(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outHAOpen: Float64Array,
  outHAHigh: Float64Array,
  outHALow: Float64Array,
  outHAClose: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('HA', 'startIdx', startIdx);
  endIdx = requireInt32('HA', 'endIdx', endIdx);
  requireIndexRange('HA', startIdx, endIdx);
  const guardStart = clampedStart('HA', startIdx, haLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('HA', 'prices.open', inOpen, guardInLen);
  requireLength('HA', 'prices.high', inHigh, guardInLen);
  requireLength('HA', 'prices.low', inLow, guardInLen);
  requireLength('HA', 'prices.close', inClose, guardInLen);
  requireOutput('HA', 'outHaOpen', outHAOpen, guardOutLen, Float64Array);
  requireOutput('HA', 'outHaHigh', outHAHigh, guardOutLen, Float64Array);
  requireOutput('HA', 'outHaLow', outHALow, guardOutLen, Float64Array);
  requireOutput('HA', 'outHaClose', outHAClose, guardOutLen, Float64Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outHAOpen, inOpenF64)) {
    throw sharedMemory('HA', 'outHaOpen', 'prices.open');
  }
  if (overlapsPartly(outHAOpen, inHighF64)) {
    throw sharedMemory('HA', 'outHaOpen', 'prices.high');
  }
  if (overlapsPartly(outHAOpen, inLowF64)) {
    throw sharedMemory('HA', 'outHaOpen', 'prices.low');
  }
  if (overlapsPartly(outHAOpen, inCloseF64)) {
    throw sharedMemory('HA', 'outHaOpen', 'prices.close');
  }
  if (overlapsPartly(outHAHigh, inOpenF64)) {
    throw sharedMemory('HA', 'outHaHigh', 'prices.open');
  }
  if (overlapsPartly(outHAHigh, inHighF64)) {
    throw sharedMemory('HA', 'outHaHigh', 'prices.high');
  }
  if (overlapsPartly(outHAHigh, inLowF64)) {
    throw sharedMemory('HA', 'outHaHigh', 'prices.low');
  }
  if (overlapsPartly(outHAHigh, inCloseF64)) {
    throw sharedMemory('HA', 'outHaHigh', 'prices.close');
  }
  if (overlapsPartly(outHALow, inOpenF64)) {
    throw sharedMemory('HA', 'outHaLow', 'prices.open');
  }
  if (overlapsPartly(outHALow, inHighF64)) {
    throw sharedMemory('HA', 'outHaLow', 'prices.high');
  }
  if (overlapsPartly(outHALow, inLowF64)) {
    throw sharedMemory('HA', 'outHaLow', 'prices.low');
  }
  if (overlapsPartly(outHALow, inCloseF64)) {
    throw sharedMemory('HA', 'outHaLow', 'prices.close');
  }
  if (overlapsPartly(outHAClose, inOpenF64)) {
    throw sharedMemory('HA', 'outHaClose', 'prices.open');
  }
  if (overlapsPartly(outHAClose, inHighF64)) {
    throw sharedMemory('HA', 'outHaClose', 'prices.high');
  }
  if (overlapsPartly(outHAClose, inLowF64)) {
    throw sharedMemory('HA', 'outHaClose', 'prices.low');
  }
  if (overlapsPartly(outHAClose, inCloseF64)) {
    throw sharedMemory('HA', 'outHaClose', 'prices.close');
  }
  if (overlaps(outHAOpen, outHAHigh)) {
    throw sharedMemory('HA', 'outHaOpen', 'outHaHigh');
  }
  if (overlaps(outHAOpen, outHALow)) {
    throw sharedMemory('HA', 'outHaOpen', 'outHaLow');
  }
  if (overlaps(outHAOpen, outHAClose)) {
    throw sharedMemory('HA', 'outHaOpen', 'outHaClose');
  }
  if (overlaps(outHAHigh, outHALow)) {
    throw sharedMemory('HA', 'outHaHigh', 'outHaLow');
  }
  if (overlaps(outHAHigh, outHAClose)) {
    throw sharedMemory('HA', 'outHaHigh', 'outHaClose');
  }
  if (overlaps(outHALow, outHAClose)) {
    throw sharedMemory('HA', 'outHaLow', 'outHaClose');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = haImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outHAOpen, outHAHigh, outHALow, outHAClose);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('HA', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link ha}, {@link haLookback}, {@link haDisplayShift},
 * {@link haOpen} and {@link haOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface HaOptions {
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
   * Buffer for `outHaOpen`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outHaOpen?: Float64Array | undefined;
  /**
   * Buffer for `outHaHigh`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outHaHigh?: Float64Array | undefined;
  /**
   * Buffer for `outHaLow`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outHaLow?: Float64Array | undefined;
  /**
   * Buffer for `outHaClose`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outHaClose?: Float64Array | undefined;
}

/** What {@link ha} returns, and {@link haOpenAndFill} with its stream. */
export interface HaResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /** Heikin-Ashi open. `outHaOpen[k]` is the value at input bar `begIdx + k`. */
  outHaOpen: Float64Array;
  /** Heikin-Ashi high. `outHaHigh[k]` is the value at input bar `begIdx + k`. */
  outHaHigh: Float64Array;
  /** Heikin-Ashi low. `outHaLow[k]` is the value at input bar `begIdx + k`. */
  outHaLow: Float64Array;
  /** Heikin-Ashi close. `outHaClose[k]` is the value at input bar `begIdx + k`. */
  outHaClose: Float64Array;
}

/**
 * Number of leading input bars {@link ha} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link HaOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function haLookback(options?: HaOptions): number {
  const o = optionsOf('HA lookback', options);
  return haLookbackPositional(coreOf('HA lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link ha}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link HaOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function haDisplayShift(outputIdx: number, options?: HaOptions): number {
  const o = optionsOf('HA displayShift', options);
  return haDisplayShiftPositional(outputIdx, coreOf('HA displayShift', o.core));
}

/**
 * Heikin-Ashi candles: an OHLC-to-OHLC transform that replaces each bar with a
 * smoothed synthetic candle. The synthetic close is the bar's four-price average,
 * the synthetic open is the midpoint of the previous synthetic candle's own open
 * and close, and the synthetic high and low extend the raw extremes far enough to
 * contain that pair. Read the result as a trend filter rather than as price:
 * consecutive candles of one colour run longer than on the raw chart, small
 * counter-trend bars are absorbed, and a candle with no shadow on the trend side
 * is the usual continuation cue. The cost is that the synthetic open and close are
 * not tradeable prices and gaps disappear entirely, so orders must still be placed
 * against the raw series. `HA` is recursive: every candle carries the previous
 * one, so the first candle of a request is seeded from its own bar and its
 * influence halves on each bar that follows.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ha](https://ta-lib.org/functions/ha).
 *
 * **Notes**
 * - The first candle has no predecessor, so its open is seeded with the midpoint
 *   of the raw open and close. Other conventions exist — ta4j emits the raw bar
 *   unchanged as its first candle — and they differ only while the seed still
 *   carries weight.
 * - Both divisors are exact powers of two, so implementations that scale by `0.5`
 *   and `0.25` produce the same doubles as those that divide by 2 and 4.
 * - The unstable period discards candles of warm-up before the first output,
 *   trading history for a smaller residual difference between two requests that
 *   start at different bars.
 * - Averaging four prices of one bar is also what
 *   [`AVGPRICE`](https://ta-lib.org/functions/avgprice) computes, but it sums them
 *   in a different order, so the two can differ in the last bits.
 *
 * `outHaOpen[k]` is the value at input bar `begIdx + k`; every output is exactly
 * as long as the number of values written, and nothing is padded. A range that
 * ends before {@link haLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `open`: Open
 *   price of each bar. `high`: High price of each bar. `low`: Low price of each bar.
 *   `close`: Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link HaOptions}.
 * @returns `begIdx` and the values; see {@link HaResult}. Each output is a view of
 *   its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, an input too short for the range, a buffer that is empty, too short or of
 *   the wrong type, two buffers sharing memory or a buffer sharing memory with a
 *   `Float64Array` input it is not. Checked before anything is written.
 *
 * @example
 * ```ts
 * import { ha } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const {
 *   begIdx,
 *   outHaOpen,
 *   outHaHigh,
 *   outHaLow,
 *   outHaClose,
 * } = ha({ open, high, low, close });
 * // outHaOpen[k] is the value at bar begIdx + k
 * ```
 * @see [`avgprice`](https://ta-lib.org/functions/avgprice)
 * @see [`medprice`](https://ta-lib.org/functions/medprice)
 * @see [`typprice`](https://ta-lib.org/functions/typprice)
 * @see [`wclprice`](https://ta-lib.org/functions/wclprice)
 */
export function ha(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: HaOptions): HaResult {
  requireArgument('HA', 'prices', prices);
  const o = optionsOf('HA', options);
  if (o.endIdx === undefined) {
    requireSeries('HA', 'prices.open', prices.open);
    requireSeries('HA', 'prices.high', prices.high);
    requireSeries('HA', 'prices.low', prices.low);
    requireSeries('HA', 'prices.close', prices.close);
  }
  const core = coreOf('HA', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, haLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outHAOpen = o.outHaOpen === undefined ? new Float64Array(capacity) : o.outHaOpen;
  const outHAHigh = o.outHaHigh === undefined ? new Float64Array(capacity) : o.outHaHigh;
  const outHALow = o.outHaLow === undefined ? new Float64Array(capacity) : o.outHaLow;
  const outHAClose = o.outHaClose === undefined ? new Float64Array(capacity) : o.outHaClose;
  const range = haPositional(startIdx, endIdx, prices.open, prices.high, prices.low, prices.close, outHAOpen, outHAHigh, outHALow, outHAClose, core);
  return { begIdx: range.begIdx, outHaOpen: outHAOpen.subarray(0, range.count), outHaHigh: outHAHigh.subarray(0, range.count), outHaLow: outHALow.subarray(0, range.count), outHaClose: outHAClose.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live HA stream: one value per closed bar, bit-identical to {@link ha} over the
 * same series. Open it with {@link haOpen} or {@link haOpenAndFill}; there is
 * nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ha](https://ta-lib.org/functions/ha).
 */
export class HaStream {
  /** @internal */
  core: Core;
  /** @internal */
  haOpen: number = 0;
  /** @internal */
  haClose: number = 0;
  /** @internal */
  cur_outHAOpen: number = 0;
  /** @internal */
  cur_outHAHigh: number = 0;
  /** @internal */
  cur_outHALow: number = 0;
  /** @internal */
  cur_outHAClose: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): HaStream {
    return new HaStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link ha} reports them over the same bars. Each accepted `update` and each
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
      throw failure('HA advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns its outputs.
   *
   * @param inOpen - The bar's `inOpen` value: Open price of each bar.
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param out - Receives the outputs and is returned; a new {@link HaOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inOpen: number, inHigh: number, inLow: number, inClose: number, out?: HaOut): HaOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('HA update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('HA update', 'HaOut');
    }
    if (!Number.isFinite(inOpen) || !Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('HA update', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    haStepImpl(this, inOpen, inHigh, inLow, inClose);
    this.outRangeCount++;
    if (out === undefined) {
      out = new HaOut();
    }
    out.outHaOpen = this.cur_outHAOpen;
    out.outHaHigh = this.cur_outHAHigh;
    out.outHaLow = this.cur_outHALow;
    out.outHaClose = this.cur_outHAClose;
    return out;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inOpen - The bar's `inOpen` value: Open price of each bar.
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param out - Receives the outputs and is returned; a new {@link HaOut} when
   *   omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inOpen: number, inHigh: number, inLow: number, inClose: number, out?: HaOut): HaOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('HA peek', 'HaOut');
    }
    if (!Number.isFinite(inOpen) || !Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('HA peek', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let haHigh = 0;
    let haLow = 0;
    let tempOpen = 0;
    let tempHigh = 0;
    let tempLow = 0;
    let tempClose = 0;
    let cur_outHAClose = 0;
    let cur_outHAHigh = 0;
    let cur_outHALow = 0;
    let cur_outHAOpen = 0;
    let haClose = sp.haClose;
    let haOpen = sp.haOpen;
    /* Every price of the bar is read into a local before any output is
     * written, which is what lets an output alias any input: at startIdx 0
     * the store lands on the very slot the high and low were just read from.
     */
    tempOpen = inOpen;
    tempHigh = inHigh;
    tempLow = inLow;
    tempClose = inClose;
    haOpen = (haOpen + haClose) / 2.0;
    haClose = (tempOpen + tempHigh + tempLow + tempClose) / 4.0;
    haHigh = tempHigh;
    if (haOpen > haHigh) {
      haHigh = haOpen;
    }
    if (haClose > haHigh) {
      haHigh = haClose;
    }
    haLow = tempLow;
    if (haOpen < haLow) {
      haLow = haOpen;
    }
    if (haClose < haLow) {
      haLow = haClose;
    }
    cur_outHAOpen = haOpen;
    cur_outHAHigh = haHigh;
    cur_outHALow = haLow;
    cur_outHAClose = haClose;
    if (out === undefined) {
      out = new HaOut();
    }
    out.outHaOpen = cur_outHAOpen;
    out.outHaHigh = cur_outHAHigh;
    out.outHaLow = cur_outHALow;
    out.outHaClose = cur_outHAClose;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link HaOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   */
  value(out?: HaOut): HaOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('HA value', 'HaOut');
    }
    if (out === undefined) {
      out = new HaOut();
    }
    out.outHaOpen = this.cur_outHAOpen;
    out.outHaHigh = this.cur_outHAHigh;
    out.outHaLow = this.cur_outHALow;
    out.outHaClose = this.cur_outHAClose;
    return out;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): HaStream {
    const c = new HaStream(this.core);
    c.haOpen = this.haOpen;
    c.haClose = this.haClose;
    c.cur_outHAOpen = this.cur_outHAOpen;
    c.cur_outHAHigh = this.cur_outHAHigh;
    c.cur_outHALow = this.cur_outHALow;
    c.cur_outHAClose = this.cur_outHAClose;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one HA bar, as `update`, `peek` and `value` return them. Passed
 * back to one of them, it is filled and returned again, with no allocation; it
 * holds its values until the next call that writes it. A new one holds `NaN`.
 */
export class HaOut {
  /** Heikin-Ashi open. */
  outHaOpen: number = NaN;
  /** Heikin-Ashi high. */
  outHaHigh: number = NaN;
  /** Heikin-Ashi low. */
  outHaLow: number = NaN;
  /** Heikin-Ashi close. */
  outHaClose: number = NaN;
}

function haStepImpl(sp: HaStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  let haHigh = 0;
  let haLow = 0;
  let tempOpen = 0;
  let tempHigh = 0;
  let tempLow = 0;
  let tempClose = 0;
  /* Every price of the bar is read into a local before any output is
   * written, which is what lets an output alias any input: at startIdx 0
   * the store lands on the very slot the high and low were just read from.
   */
  tempOpen = inOpen;
  tempHigh = inHigh;
  tempLow = inLow;
  tempClose = inClose;
  sp.haOpen = (sp.haOpen + sp.haClose) / 2.0;
  sp.haClose = (tempOpen + tempHigh + tempLow + tempClose) / 4.0;
  haHigh = tempHigh;
  if (sp.haOpen > haHigh) {
    haHigh = sp.haOpen;
  }
  if (sp.haClose > haHigh) {
    haHigh = sp.haClose;
  }
  haLow = tempLow;
  if (sp.haOpen < haLow) {
    haLow = sp.haOpen;
  }
  if (sp.haClose < haLow) {
    haLow = sp.haClose;
  }
  sp.cur_outHAOpen = sp.haOpen;
  sp.cur_outHAHigh = haHigh;
  sp.cur_outHALow = haLow;
  sp.cur_outHAClose = sp.haClose;
}

function haOpenImpl(
  sp: HaStream,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outHAOpen: Float64Array,
  outHAHigh: Float64Array,
  outHALow: Float64Array,
  outHAClose: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let i = 0;
  let outIdx = 0;
  let today = 0;
  let lookbackTotal = 0;
  let haOpen = 0;
  let haClose = 0;
  let haHigh = 0;
  let haLow = 0;
  let tempOpen = 0;
  let tempHigh = 0;
  let tempLow = 0;
  let tempClose = 0;
  const historyLen = inOpen.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inHigh.length !== inOpen.length || inLow.length !== inOpen.length || inClose.length !== inOpen.length) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = haLookbackPositional(core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* The summation order ((O+H)+L)+C is the bit-exactness contract with every
   * external implementation of this indicator. TA_AVGPRICE is documented as
   * the same quantity but sums ((H+L)+C)+O, which is a different double on
   * some bars; the two are deliberately not unified.
   */
  today = startIdx - lookbackTotal;
  haOpen = (inOpen[today] + inClose[today]) / 2.0;
  haClose = (inOpen[today] + inHigh[today] + inLow[today] + inClose[today]) / 4.0;
  today += 1;
  /* Skip the unstable period. */
  i = lookbackTotal;
  while (i !== 0) {
    /* haOpen consumes the PREVIOUS candle on both sides of the midpoint, so
     * it must advance before haClose is overwritten. Swapping these two
     * still yields a plausible smoothed series.
     */
    haOpen = (haOpen + haClose) / 2.0;
    haClose = (inOpen[today] + inHigh[today] + inLow[today] + inClose[today]) / 4.0;
    today += 1;
    i -= 1;
  }
  tempHigh = inHigh[startIdx];
  tempLow = inLow[startIdx];
  /* The three-way extremum is spelled with plain comparisons, never the
   * max/min builtins: C's macros return their SECOND operand on a tie and
   * Rust's and Java's builtins do not, so a bar of signed zeros
   * emits different bytes per backend and the cross-language gate compares
   * bytes. Measured on (O,H,L,C) = (-0.0, +0.0, -0.0, -0.0).
   */
  haHigh = tempHigh;
  if (haOpen > haHigh) {
    haHigh = haOpen;
  }
  if (haClose > haHigh) {
    haHigh = haClose;
  }
  haLow = tempLow;
  if (haOpen < haLow) {
    haLow = haOpen;
  }
  if (haClose < haLow) {
    haLow = haClose;
  }
  outHAOpen[((0 * outStride) | 0)] = haOpen;
  outHAHigh[((0 * outStride) | 0)] = haHigh;
  outHALow[((0 * outStride) | 0)] = haLow;
  outHAClose[((0 * outStride) | 0)] = haClose;
  outIdx = 1;
  while (today <= endIdx) {
    /* Every price of the bar is read into a local before any output is
     * written, which is what lets an output alias any input: at startIdx 0
     * the store lands on the very slot the high and low were just read from.
     */
    tempOpen = inOpen[today];
    tempHigh = inHigh[today];
    tempLow = inLow[today];
    tempClose = inClose[today];
    haOpen = (haOpen + haClose) / 2.0;
    haClose = (tempOpen + tempHigh + tempLow + tempClose) / 4.0;
    haHigh = tempHigh;
    if (haOpen > haHigh) {
      haHigh = haOpen;
    }
    if (haClose > haHigh) {
      haHigh = haClose;
    }
    haLow = tempLow;
    if (haOpen < haLow) {
      haLow = haOpen;
    }
    if (haClose < haLow) {
      haLow = haClose;
    }
    outHAOpen[((outIdx * outStride) | 0)] = haOpen;
    outHAHigh[((outIdx * outStride) | 0)] = haHigh;
    outHALow[((outIdx * outStride) | 0)] = haLow;
    outHAClose[((outIdx * outStride) | 0)] = haClose;
    outIdx += 1;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  sp.haOpen = haOpen;
  sp.haClose = haClose;
  sp.cur_outHAOpen = outHAOpen[(outNBElement.value - 1) * outStride];
  sp.cur_outHAHigh = outHAHigh[(outNBElement.value - 1) * outStride];
  sp.cur_outHALow = outHALow[(outNBElement.value - 1) * outStride];
  sp.cur_outHAClose = outHAClose[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function haOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outHAOpen: Float64Array,
  outHAHigh: Float64Array,
  outHALow: Float64Array,
  outHAClose: Float64Array,
): HaStream {
  const sp = HaStream.of(core);
  const retCode = haOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outHAOpen, outHAHigh, outHALow, outHAClose, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('HA openAndFill', inOpen.length, startIdx, haLookbackPositional(core));
  }
  throw streamFailure('HA openAndFill', retCode);
}

/** @internal */
export function haOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): HaStream {
  const sp = HaStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outHAOpen = new Float64Array(1);
  const sink_outHAHigh = new Float64Array(1);
  const sink_outHALow = new Float64Array(1);
  const sink_outHAClose = new Float64Array(1);
  const retCode = haOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outHAOpen, sink_outHAHigh, sink_outHALow, sink_outHAClose, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('HA open', inOpen.length, startIdx, haLookbackPositional(core));
  }
  throw streamFailure('HA open', retCode);
}

/** @internal */
export function haOpenPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): HaStream {
  requireArgument('HA open', 'prices.open', inOpen);
  requireHistory('HA open', inOpen.length);
  requireArgument('HA open', 'prices.high', inHigh);
  requireArgument('HA open', 'prices.low', inLow);
  requireArgument('HA open', 'prices.close', inClose);
  requireHistoryLength('HA open', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('HA open', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('HA open', 'prices.close', inClose.length, inOpen.length);
  return haOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/** @internal */
export function haOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outHAOpen: Float64Array,
  outHAHigh: Float64Array,
  outHALow: Float64Array,
  outHAClose: Float64Array,
  core: Core = Core.DEFAULT,
): HaStream {
  requireArgument('HA openAndFill', 'prices.open', inOpen);
  requireHistory('HA openAndFill', inOpen.length);
  requireArgument('HA openAndFill', 'prices.high', inHigh);
  requireArgument('HA openAndFill', 'prices.low', inLow);
  requireArgument('HA openAndFill', 'prices.close', inClose);
  const guardOutLen = openFillCount('HA openAndFill', inOpen.length, haLookbackPositional(core));
  requireHistoryLength('HA openAndFill', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('HA openAndFill', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('HA openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('HA openAndFill', 'outHaOpen', outHAOpen, guardOutLen, Float64Array);
  requireOutput('HA openAndFill', 'outHaHigh', outHAHigh, guardOutLen, Float64Array);
  requireOutput('HA openAndFill', 'outHaLow', outHALow, guardOutLen, Float64Array);
  requireOutput('HA openAndFill', 'outHaClose', outHAClose, guardOutLen, Float64Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outHAOpen.buffer === inOpenF64.buffer && outHAOpen.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('HA openAndFill', 'outHaOpen', 'prices.open');
  }
  if (outHAOpen.buffer === inHighF64.buffer && outHAOpen.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('HA openAndFill', 'outHaOpen', 'prices.high');
  }
  if (outHAOpen.buffer === inLowF64.buffer && outHAOpen.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('HA openAndFill', 'outHaOpen', 'prices.low');
  }
  if (outHAOpen.buffer === inCloseF64.buffer && outHAOpen.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('HA openAndFill', 'outHaOpen', 'prices.close');
  }
  if (outHAHigh.buffer === inOpenF64.buffer && outHAHigh.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('HA openAndFill', 'outHaHigh', 'prices.open');
  }
  if (outHAHigh.buffer === inHighF64.buffer && outHAHigh.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('HA openAndFill', 'outHaHigh', 'prices.high');
  }
  if (outHAHigh.buffer === inLowF64.buffer && outHAHigh.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('HA openAndFill', 'outHaHigh', 'prices.low');
  }
  if (outHAHigh.buffer === inCloseF64.buffer && outHAHigh.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('HA openAndFill', 'outHaHigh', 'prices.close');
  }
  if (outHALow.buffer === inOpenF64.buffer && outHALow.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('HA openAndFill', 'outHaLow', 'prices.open');
  }
  if (outHALow.buffer === inHighF64.buffer && outHALow.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('HA openAndFill', 'outHaLow', 'prices.high');
  }
  if (outHALow.buffer === inLowF64.buffer && outHALow.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('HA openAndFill', 'outHaLow', 'prices.low');
  }
  if (outHALow.buffer === inCloseF64.buffer && outHALow.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('HA openAndFill', 'outHaLow', 'prices.close');
  }
  if (outHAClose.buffer === inOpenF64.buffer && outHAClose.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('HA openAndFill', 'outHaClose', 'prices.open');
  }
  if (outHAClose.buffer === inHighF64.buffer && outHAClose.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('HA openAndFill', 'outHaClose', 'prices.high');
  }
  if (outHAClose.buffer === inLowF64.buffer && outHAClose.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('HA openAndFill', 'outHaClose', 'prices.low');
  }
  if (outHAClose.buffer === inCloseF64.buffer && outHAClose.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('HA openAndFill', 'outHaClose', 'prices.close');
  }
  if (outHAOpen.buffer === outHAHigh.buffer && outHAOpen.byteOffset === outHAHigh.byteOffset) {
    throw sharedMemory('HA openAndFill', 'outHaOpen', 'outHaHigh');
  }
  if (outHAOpen.buffer === outHALow.buffer && outHAOpen.byteOffset === outHALow.byteOffset) {
    throw sharedMemory('HA openAndFill', 'outHaOpen', 'outHaLow');
  }
  if (outHAOpen.buffer === outHAClose.buffer && outHAOpen.byteOffset === outHAClose.byteOffset) {
    throw sharedMemory('HA openAndFill', 'outHaOpen', 'outHaClose');
  }
  if (outHAHigh.buffer === outHALow.buffer && outHAHigh.byteOffset === outHALow.byteOffset) {
    throw sharedMemory('HA openAndFill', 'outHaHigh', 'outHaLow');
  }
  if (outHAHigh.buffer === outHAClose.buffer && outHAHigh.byteOffset === outHAClose.byteOffset) {
    throw sharedMemory('HA openAndFill', 'outHaHigh', 'outHaClose');
  }
  if (outHALow.buffer === outHAClose.buffer && outHALow.byteOffset === outHAClose.byteOffset) {
    throw sharedMemory('HA openAndFill', 'outHaLow', 'outHaClose');
  }
  if (guardOutLen > 0 && overlaps(outHAOpen, inOpenF64)) {
    throw sharedMemory('HA openAndFill', 'outHaOpen', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outHAOpen, inHighF64)) {
    throw sharedMemory('HA openAndFill', 'outHaOpen', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outHAOpen, inLowF64)) {
    throw sharedMemory('HA openAndFill', 'outHaOpen', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outHAOpen, inCloseF64)) {
    throw sharedMemory('HA openAndFill', 'outHaOpen', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outHAHigh, inOpenF64)) {
    throw sharedMemory('HA openAndFill', 'outHaHigh', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outHAHigh, inHighF64)) {
    throw sharedMemory('HA openAndFill', 'outHaHigh', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outHAHigh, inLowF64)) {
    throw sharedMemory('HA openAndFill', 'outHaHigh', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outHAHigh, inCloseF64)) {
    throw sharedMemory('HA openAndFill', 'outHaHigh', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outHALow, inOpenF64)) {
    throw sharedMemory('HA openAndFill', 'outHaLow', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outHALow, inHighF64)) {
    throw sharedMemory('HA openAndFill', 'outHaLow', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outHALow, inLowF64)) {
    throw sharedMemory('HA openAndFill', 'outHaLow', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outHALow, inCloseF64)) {
    throw sharedMemory('HA openAndFill', 'outHaLow', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outHAClose, inOpenF64)) {
    throw sharedMemory('HA openAndFill', 'outHaClose', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outHAClose, inHighF64)) {
    throw sharedMemory('HA openAndFill', 'outHaClose', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outHAClose, inLowF64)) {
    throw sharedMemory('HA openAndFill', 'outHaClose', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outHAClose, inCloseF64)) {
    throw sharedMemory('HA openAndFill', 'outHaClose', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outHAOpen, outHAHigh)) {
    throw sharedMemory('HA openAndFill', 'outHaOpen', 'outHaHigh');
  }
  if (guardOutLen > 0 && overlaps(outHAOpen, outHALow)) {
    throw sharedMemory('HA openAndFill', 'outHaOpen', 'outHaLow');
  }
  if (guardOutLen > 0 && overlaps(outHAOpen, outHAClose)) {
    throw sharedMemory('HA openAndFill', 'outHaOpen', 'outHaClose');
  }
  if (guardOutLen > 0 && overlaps(outHAHigh, outHALow)) {
    throw sharedMemory('HA openAndFill', 'outHaHigh', 'outHaLow');
  }
  if (guardOutLen > 0 && overlaps(outHAHigh, outHAClose)) {
    throw sharedMemory('HA openAndFill', 'outHaHigh', 'outHaClose');
  }
  if (guardOutLen > 0 && overlaps(outHALow, outHAClose)) {
    throw sharedMemory('HA openAndFill', 'outHaLow', 'outHaClose');
  }
  return haOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outHAOpen, outHAHigh, outHALow, outHAClose);
}

/**
 * Opens a live HA stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link ha} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `open`: Open price of each bar. `high`: High price of each bar. `low`:
 *   Low price of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link HaOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `haLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input
 *   or histories of different lengths.
 *
 * @example
 * ```ts
 * import { haOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = haOpen({ open, high, low, close });
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function haOpen(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: HaOptions): HaStream {
  requireArgument('HA open', 'prices', prices);
  const o = optionsOf('HA open', options);
  const core = coreOf('HA open', o.core);
  return haOpenPositional(prices.open, prices.high, prices.low, prices.close, core);
}

/**
 * {@link haOpen} that also computes the outputs over the whole history in the same
 * pass, bit-identically to {@link ha}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `open`: Open price of each bar. `high`: High price of each bar. `low`:
 *   Low price of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link HaOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link ha} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `haLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { haOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const {
 *   stream,
 *   begIdx,
 *   outHaOpen,
 *   outHaHigh,
 *   outHaLow,
 *   outHaClose,
 * } = haOpenAndFill({ open, high, low, close });
 * // outHaOpen[k] is the value at bar begIdx + k
 * ```
 */
export function haOpenAndFill(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: HaOptions): HaResult & { stream: HaStream } {
  requireArgument('HA openAndFill', 'prices', prices);
  const o = optionsOf('HA openAndFill', options);
  const core = coreOf('HA openAndFill', o.core);
  const capacity = fillCapacity(haLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outHAOpen = o.outHaOpen === undefined ? new Float64Array(capacity) : o.outHaOpen;
  const outHAHigh = o.outHaHigh === undefined ? new Float64Array(capacity) : o.outHaHigh;
  const outHALow = o.outHaLow === undefined ? new Float64Array(capacity) : o.outHaLow;
  const outHAClose = o.outHaClose === undefined ? new Float64Array(capacity) : o.outHaClose;
  const stream = haOpenAndFillPositional(prices.open, prices.high, prices.low, prices.close, outHAOpen, outHAHigh, outHALow, outHAClose, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outHaOpen: outHAOpen.subarray(0, range.count), outHaHigh: outHAHigh.subarray(0, range.count), outHaLow: outHALow.subarray(0, range.count), outHaClose: outHAClose.subarray(0, range.count) };
}
