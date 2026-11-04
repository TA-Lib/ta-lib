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
import { badSink, clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link ha} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function haLookback(core: Core = Core.DEFAULT): number {
  /* 13/9 is 1/ln(2) rounded up: the open is the only state, and it shows
   * the whole first difference at its first bar.
   */
  return core.unstableCount(FuncUnstId.HA, (((((13 * 10) | 0) + 8) / 9) | 0), (((((13 * 19) | 0) + 8) / 9) | 0));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link ha}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function haDisplayShift(outputIdx: number, core: Core = Core.DEFAULT): number {
  void core;
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
  lookbackTotal = haLookback(core);
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
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link haLookback} is a success with `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inOpen - Open price of each bar.
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param outHAOpen - Heikin-Ashi open. Must hold the values produced and never be
 *   empty; the input length always suffices.
 * @param outHAHigh - Heikin-Ashi high. Must hold the values produced and never be
 *   empty; the input length always suffices.
 * @param outHALow - Heikin-Ashi low. Must hold the values produced and never be
 *   empty; the input length always suffices.
 * @param outHAClose - Heikin-Ashi close. Must hold the values produced and never
 *   be empty; the input length always suffices.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The range written: `begIdx` is the first bar with a value, `count` how
 *   many were written.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for an argument that is not an int32, an
 *   array too short for the range, an empty output, two outputs sharing memory or an
 *   output sharing memory with a `Float64Array` input it is not. Checked before
 *   anything is written.
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
 * const haOpen = new Float64Array(open.length);
 * const haHigh = new Float64Array(open.length);
 * const haLow = new Float64Array(open.length);
 * const haClose = new Float64Array(open.length);
 * const { begIdx, count } = ha(
 *   0, open.length - 1,
 *   open, high, low, close,
 *   haOpen, haHigh, haLow, haClose,
 * );
 * // haOpen[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`avgprice`](https://ta-lib.org/functions/avgprice)
 * @see [`medprice`](https://ta-lib.org/functions/medprice)
 * @see [`typprice`](https://ta-lib.org/functions/typprice)
 * @see [`wclprice`](https://ta-lib.org/functions/wclprice)
 */
export function ha(
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
  const guardStart = clampedStart('HA', startIdx, haLookback(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('HA', 'inOpen', inOpen, guardInLen);
  requireLength('HA', 'inHigh', inHigh, guardInLen);
  requireLength('HA', 'inLow', inLow, guardInLen);
  requireLength('HA', 'inClose', inClose, guardInLen);
  requireOutput('HA', 'outHAOpen', outHAOpen, guardOutLen, Float64Array);
  requireOutput('HA', 'outHAHigh', outHAHigh, guardOutLen, Float64Array);
  requireOutput('HA', 'outHALow', outHALow, guardOutLen, Float64Array);
  requireOutput('HA', 'outHAClose', outHAClose, guardOutLen, Float64Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outHAOpen, inOpenF64) || overlapsPartly(outHAOpen, inHighF64) || overlapsPartly(outHAOpen, inLowF64) || overlapsPartly(outHAOpen, inCloseF64) || overlapsPartly(outHAHigh, inOpenF64) || overlapsPartly(outHAHigh, inHighF64) || overlapsPartly(outHAHigh, inLowF64) || overlapsPartly(outHAHigh, inCloseF64) || overlapsPartly(outHALow, inOpenF64) || overlapsPartly(outHALow, inHighF64) || overlapsPartly(outHALow, inLowF64) || overlapsPartly(outHALow, inCloseF64) || overlapsPartly(outHAClose, inOpenF64) || overlapsPartly(outHAClose, inHighF64) || overlapsPartly(outHAClose, inLowF64) || overlapsPartly(outHAClose, inCloseF64) || overlaps(outHAOpen, outHAHigh) || overlaps(outHAOpen, outHALow) || overlaps(outHAOpen, outHAClose) || overlaps(outHAHigh, outHALow) || overlaps(outHAHigh, outHAClose) || overlaps(outHALow, outHAClose)) {
    throw failure('HA', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = haImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outHAOpen, outHAHigh, outHALow, outHAClose);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('HA', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
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
   * Commits one closed bar, writing the new values into `out`.
   *
   * @param inOpen - The bar's `inOpen` value: Open price of each bar.
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inOpen: number, inHigh: number, inLow: number, inClose: number, out: HaOut): void {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('HA update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (typeof out !== 'object' || out === null) {
      throw badSink('HA update', 'HaOut');
    }
    if (!Number.isFinite(inOpen) || !Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('HA update', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    haStepImpl(this, inOpen, inHigh, inLow, inClose);
    this.outRangeCount++;
    out.haOpen = this.cur_outHAOpen;
    out.haHigh = this.cur_outHAHigh;
    out.haLow = this.cur_outHALow;
    out.haClose = this.cur_outHAClose;
  }

  /**
   * Writes into `out` what the next `update` with the same bar would write,
   * committing nothing.
   *
   * @param inOpen - The bar's `inOpen` value: Open price of each bar.
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inOpen: number, inHigh: number, inLow: number, inClose: number, out: HaOut): void {
    if (typeof out !== 'object' || out === null) {
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
    out.haOpen = cur_outHAOpen;
    out.haHigh = cur_outHAHigh;
    out.haLow = cur_outHALow;
    out.haClose = cur_outHAClose;
  }

  /**
   * The value at the last bar this stream counted; `peek` does not change it.
   *
   * @param out - Receives the outputs.
   */
  value(out: HaOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('HA value', 'HaOut');
    }
    out.haOpen = this.cur_outHAOpen;
    out.haHigh = this.cur_outHAHigh;
    out.haLow = this.cur_outHALow;
    out.haClose = this.cur_outHAClose;
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
 * The outputs of one HA bar, written into an object the caller owns and reuses:
 * valid until the next call that writes it.
 */
export class HaOut {
  /** Heikin-Ashi open. */
  haOpen: number = 0;
  /** Heikin-Ashi high. */
  haHigh: number = 0;
  /** Heikin-Ashi low. */
  haLow: number = 0;
  /** Heikin-Ashi close. */
  haClose: number = 0;
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
    return RetCode.OUT_OF_RANGE_START_INDEX;
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
  lookbackTotal = haLookback(core);
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
    throw insufficientHistory('HA openAndFill', inOpen.length, startIdx, haLookback(core));
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
    throw insufficientHistory('HA open', inOpen.length, startIdx, haLookback(core));
  }
  throw streamFailure('HA open', retCode);
}

/**
 * Opens a live HA stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link ha} there.
 *
 * @param inOpen - Warm-up history. Open price of each bar. Every history has the
 *   same length.
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `haLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty one,
 *   and `TA_BAD_PARAM` for histories of different lengths.
 *
 * @example
 * ```ts
 * import { HaOut, haOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = haOpen(open, high, low, close);
 * const peeked = new HaOut();
 * const updated = new HaOut();
 * stream.peek(100.2, 101.4, 99.1, 100.9, peeked); // a forming bar: commits nothing
 * stream.update(100.2, 101.4, 99.1, 100.9, updated); // the bar closes
 * ```
 */
export function haOpen(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): HaStream {
  requireArgument('HA open', 'inOpen', inOpen);
  requireHistory('HA open', inOpen.length);
  requireArgument('HA open', 'inHigh', inHigh);
  requireArgument('HA open', 'inLow', inLow);
  requireArgument('HA open', 'inClose', inClose);
  requireHistoryLength('HA open', 'inHigh', inHigh.length, inOpen.length);
  requireHistoryLength('HA open', 'inLow', inLow.length, inOpen.length);
  requireHistoryLength('HA open', 'inClose', inClose.length, inOpen.length);
  return haOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/**
 * {@link haOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link ha}. The range written is the stream's
 * `outRange()`.
 *
 * @param inOpen - Warm-up history. Open price of each bar. Every history has the
 *   same length.
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param outHAOpen - Heikin-Ashi open. Must hold `historyLen - lookback` values
 *   and share no memory with an input or another output.
 * @param outHAHigh - Heikin-Ashi high. Must hold `historyLen - lookback` values
 *   and share no memory with an input or another output.
 * @param outHALow - Heikin-Ashi low. Must hold `historyLen - lookback` values and
 *   share no memory with an input or another output.
 * @param outHAClose - Heikin-Ashi close. Must hold `historyLen - lookback` values
 *   and share no memory with an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `haLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty one,
 *   and `TA_BAD_PARAM` for histories of different lengths or an output too short or
 *   starting where another output or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another output or a `Float64Array`
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
 * const haOpen = new Float64Array(open.length);
 * const haHigh = new Float64Array(open.length);
 * const haLow = new Float64Array(open.length);
 * const haClose = new Float64Array(open.length);
 * const stream = haOpenAndFill(
 *   open, high, low, close,
 *   haOpen, haHigh, haLow, haClose,
 * );
 * const { begIdx, count } = stream.outRange();
 * // haOpen[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function haOpenAndFill(
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
  requireArgument('HA openAndFill', 'inOpen', inOpen);
  requireHistory('HA openAndFill', inOpen.length);
  requireArgument('HA openAndFill', 'inHigh', inHigh);
  requireArgument('HA openAndFill', 'inLow', inLow);
  requireArgument('HA openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('HA openAndFill', inOpen.length, haLookback(core));
  requireHistoryLength('HA openAndFill', 'inHigh', inHigh.length, inOpen.length);
  requireHistoryLength('HA openAndFill', 'inLow', inLow.length, inOpen.length);
  requireHistoryLength('HA openAndFill', 'inClose', inClose.length, inOpen.length);
  requireOutput('HA openAndFill', 'outHAOpen', outHAOpen, guardOutLen, Float64Array);
  requireOutput('HA openAndFill', 'outHAHigh', outHAHigh, guardOutLen, Float64Array);
  requireOutput('HA openAndFill', 'outHALow', outHALow, guardOutLen, Float64Array);
  requireOutput('HA openAndFill', 'outHAClose', outHAClose, guardOutLen, Float64Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outHAOpen.buffer === inOpenF64.buffer && outHAOpen.byteOffset === inOpenF64.byteOffset) || (outHAOpen.buffer === inHighF64.buffer && outHAOpen.byteOffset === inHighF64.byteOffset) || (outHAOpen.buffer === inLowF64.buffer && outHAOpen.byteOffset === inLowF64.byteOffset) || (outHAOpen.buffer === inCloseF64.buffer && outHAOpen.byteOffset === inCloseF64.byteOffset) || (outHAHigh.buffer === inOpenF64.buffer && outHAHigh.byteOffset === inOpenF64.byteOffset) || (outHAHigh.buffer === inHighF64.buffer && outHAHigh.byteOffset === inHighF64.byteOffset) || (outHAHigh.buffer === inLowF64.buffer && outHAHigh.byteOffset === inLowF64.byteOffset) || (outHAHigh.buffer === inCloseF64.buffer && outHAHigh.byteOffset === inCloseF64.byteOffset) || (outHALow.buffer === inOpenF64.buffer && outHALow.byteOffset === inOpenF64.byteOffset) || (outHALow.buffer === inHighF64.buffer && outHALow.byteOffset === inHighF64.byteOffset) || (outHALow.buffer === inLowF64.buffer && outHALow.byteOffset === inLowF64.byteOffset) || (outHALow.buffer === inCloseF64.buffer && outHALow.byteOffset === inCloseF64.byteOffset) || (outHAClose.buffer === inOpenF64.buffer && outHAClose.byteOffset === inOpenF64.byteOffset) || (outHAClose.buffer === inHighF64.buffer && outHAClose.byteOffset === inHighF64.byteOffset) || (outHAClose.buffer === inLowF64.buffer && outHAClose.byteOffset === inLowF64.byteOffset) || (outHAClose.buffer === inCloseF64.buffer && outHAClose.byteOffset === inCloseF64.byteOffset) || (outHAOpen.buffer === outHAHigh.buffer && outHAOpen.byteOffset === outHAHigh.byteOffset) || (outHAOpen.buffer === outHALow.buffer && outHAOpen.byteOffset === outHALow.byteOffset) || (outHAOpen.buffer === outHAClose.buffer && outHAOpen.byteOffset === outHAClose.byteOffset) || (outHAHigh.buffer === outHALow.buffer && outHAHigh.byteOffset === outHALow.byteOffset) || (outHAHigh.buffer === outHAClose.buffer && outHAHigh.byteOffset === outHAClose.byteOffset) || (outHALow.buffer === outHAClose.buffer && outHALow.byteOffset === outHAClose.byteOffset)) {
    throw streamFailure('HA openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outHAOpen, inOpenF64) || overlaps(outHAOpen, inHighF64) || overlaps(outHAOpen, inLowF64) || overlaps(outHAOpen, inCloseF64) || overlaps(outHAHigh, inOpenF64) || overlaps(outHAHigh, inHighF64) || overlaps(outHAHigh, inLowF64) || overlaps(outHAHigh, inCloseF64) || overlaps(outHALow, inOpenF64) || overlaps(outHALow, inHighF64) || overlaps(outHALow, inLowF64) || overlaps(outHALow, inCloseF64) || overlaps(outHAClose, inOpenF64) || overlaps(outHAClose, inHighF64) || overlaps(outHAClose, inLowF64) || overlaps(outHAClose, inCloseF64) || overlaps(outHAOpen, outHAHigh) || overlaps(outHAOpen, outHALow) || overlaps(outHAOpen, outHAClose) || overlaps(outHAHigh, outHALow) || overlaps(outHAHigh, outHAClose) || overlaps(outHALow, outHAClose))) {
    throw streamFailure('HA openAndFill', RetCode.BAD_PARAM);
  }
  return haOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outHAOpen, outHAHigh, outHALow, outHAClose);
}
