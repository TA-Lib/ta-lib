/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin (@kevinlincg)
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  081226 KL   Initial version (#200).
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link wad} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function wadLookback(core: Core = Core.DEFAULT): number {
  void core;
  /* The first bar has no previous close, so it accumulates nothing and the
   * line starts at 0.0 -- the same convention as the other cumulative
   * lines in the tree: OBV, AD, NVI, PVI and PVT all return 0 here and emit
   * a seed value at startIdx. Tulip's ti_wad_start() returns 1 instead, so its
   * series is this one without the leading zero.
   */
  return 0;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link wad}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function wadDisplayShift(outputIdx: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function wadImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let sum = 0;
  let prevClose = 0;
  let close = 0;
  let trueExtreme = 0;
  let i = 0;
  let outIdx = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  /* Williams' Accumulation/Distribution, in the form Steven Achelis
   * published (Technical Analysis from A to Z, 2nd ed., p.368) and the form
   * every modern vendor ships: each bar's close is measured against the TRUE
   * range extreme -- the previous close when it lies outside today's bar --
   * and the results accumulate.
   *
   *    TRH = max( prevClose, high )      TRL = min( prevClose, low )
   *    AD  = close - TRL   if close > prevClose
   *        = close - TRH   if close < prevClose
   *        = 0             if close == prevClose
   *    WAD = running sum of AD
   *
   * NO VOLUME IS CONSUMED, despite the name. Larry Williams' original
   * multiplies the move by volume; Achelis' modification drops it, the
   * industry attached Williams' name to the modification anyway, and Tulip,
   * pandas-ta-classic, cTrader, TC2000, WealthCharts and MultiCharts all ship
   * the no-volume form. Shipping the volume form under this name would
   * surprise every user, so this is the one place the usual "the original
   * author wins" rule is set aside. The volume-weighted series is a different
   * indicator. What is left once the multiplier is gone is a signed
   * close-to-close move clipped by the true-range extreme and accumulated --
   * momentum measured on the true range -- which is why this is grouped with
   * the other no-volume directional lines (PLUS_DM, MINUS_DM, BOP, WILLR)
   * rather than with AD/OBV.
   *
   * The three-way branch is written with plain > and < rather than any
   * epsilon: the flat arm must fire on exactly-equal consecutive closes and
   * on nothing else, which also keeps -0.0 and NaN behaviour identical
   * across the C, Rust, Java and .NET backends.
   *
   * prevClose is carried in a scalar, so outReal may alias any input: every
   * read of bar i happens before the store at outIdx <= i, and no earlier
   * bar is ever re-read.
   */
  sum = 0.0;
  outIdx = 0;
  /* The first bar of the requested range is measured against itself, i.e. it
   * contributes exactly 0.0. The accumulator therefore restarts wherever the
   * caller starts, which is why this function is flagged path_dependent.
   */
  prevClose = inClose[startIdx];
  for (i = startIdx; i <= endIdx; i += 1) {
    close = inClose[i];
    if (close > prevClose) {
      trueExtreme = inLow[i];
      if (prevClose < trueExtreme) {
        trueExtreme = prevClose;
      }
      sum += close - trueExtreme;
    } else if (close < prevClose) {
      trueExtreme = inHigh[i];
      if (prevClose > trueExtreme) {
        trueExtreme = prevClose;
      }
      sum += close - trueExtreme;
    }
    outReal[outIdx] = sum;
    outIdx = outIdx + 1;
    prevClose = close;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * Williams' Accumulation/Distribution: a cumulative line meant to expose whether a
 * security is quietly under accumulation (informed buying) or distribution
 * (informed selling) beneath the surface of price. Larry Williams built it to
 * catch that shift before price confirms it — traders watch for the line to
 * diverge from price, since a line that keeps rising while price stalls or falls
 * points to accumulation, and one that stalls while price pushes to a new high
 * points to distribution. **It consumes no volume.** Larry Williams' original
 * multiplies each move by that bar's volume; Steven Achelis published the
 * modification that drops the multiplier (*Technical Analysis from A to Z*, 2nd
 * ed., p.368), and the industry kept Williams' name on that no-volume form. That
 * industry-wide decision is enough for TA-Lib to ship the same form under the same
 * name. What remains once the multiplier is dropped is a signed close-to-close
 * move measured on the true range, so it is grouped as a momentum indicator, not a
 * volume one.
 *
 * Formula and more info at
 * [ta-lib.org/functions/wad](https://ta-lib.org/functions/wad).
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link wadLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param outReal - Cumulative accumulation/distribution. Must hold the values
 *   produced and never be empty; the input length always suffices.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The range written: `begIdx` is the first bar with a value, `count` how
 *   many were written.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for an argument that is not an int32, an
 *   array too short for the range, an empty output or an output sharing memory with
 *   a `Float64Array` input it is not. Checked before anything is written.
 *
 * @example
 * ```ts
 * import { wad } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const out = new Float64Array(high.length);
 * const { begIdx, count } = wad(0, high.length - 1, high, low, close, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`ad`](https://ta-lib.org/functions/ad)
 * @see [`adosc`](https://ta-lib.org/functions/adosc)
 * @see [`nvi`](https://ta-lib.org/functions/nvi)
 * @see [`obv`](https://ta-lib.org/functions/obv)
 * @see [`pvi`](https://ta-lib.org/functions/pvi)
 */
export function wad(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('WAD', 'startIdx', startIdx);
  endIdx = requireInt32('WAD', 'endIdx', endIdx);
  requireIndexRange('WAD', startIdx, endIdx);
  const guardStart = clampedStart('WAD', startIdx, wadLookback(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('WAD', 'inHigh', inHigh, guardInLen);
  requireLength('WAD', 'inLow', inLow, guardInLen);
  requireLength('WAD', 'inClose', inClose, guardInLen);
  requireLength('WAD', 'outReal', outReal, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outReal, inHighF64) || overlapsPartly(outReal, inLowF64) || overlapsPartly(outReal, inCloseF64)) {
    throw failure('WAD', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = wadImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('WAD', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live WAD stream: one value per closed bar, bit-identical to {@link wad} over
 * the same series. Open it with {@link wadOpen} or {@link wadOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/wad](https://ta-lib.org/functions/wad).
 */
export class WadStream {
  /** @internal */
  core: Core;
  /** @internal */
  sum: number = 0;
  /** @internal */
  prevClose: number = 0;
  /** @internal */
  cur_outReal: number = 0;
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
   * {@link wad} reports them over the same bars. Each accepted `update` and each
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
      throw failure('WAD advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw failure('WAD update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('WAD update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    wadStepImpl(this, inHigh, inLow, inClose);
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
      throw nonFiniteBar('WAD peek', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let close = 0;
    let trueExtreme = 0;
    let cur_outReal = 0;
    let sum = sp.sum;
    close = inClose;
    if (close > sp.prevClose) {
      trueExtreme = inLow;
      if (sp.prevClose < trueExtreme) {
        trueExtreme = sp.prevClose;
      }
      sum += close - trueExtreme;
    } else if (close < sp.prevClose) {
      trueExtreme = inHigh;
      if (sp.prevClose > trueExtreme) {
        trueExtreme = sp.prevClose;
      }
      sum += close - trueExtreme;
    }
    cur_outReal = sum;
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
  clone(): WadStream {
    const c = new WadStream(this.core);
    c.sum = this.sum;
    c.prevClose = this.prevClose;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function wadStepImpl(sp: WadStream, inHigh: number, inLow: number, inClose: number): void {
  let close = 0;
  let trueExtreme = 0;
  close = inClose;
  if (close > sp.prevClose) {
    trueExtreme = inLow;
    if (sp.prevClose < trueExtreme) {
      trueExtreme = sp.prevClose;
    }
    sp.sum += close - trueExtreme;
  } else if (close < sp.prevClose) {
    trueExtreme = inHigh;
    if (sp.prevClose > trueExtreme) {
      trueExtreme = sp.prevClose;
    }
    sp.sum += close - trueExtreme;
  }
  sp.cur_outReal = sp.sum;
  sp.prevClose = close;
}

function wadOpenImpl(
  sp: WadStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let sum = 0;
  let prevClose = 0;
  let close = 0;
  let trueExtreme = 0;
  let i = 0;
  let outIdx = 0;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length || inClose.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Williams' Accumulation/Distribution, in the form Steven Achelis
   * published (Technical Analysis from A to Z, 2nd ed., p.368) and the form
   * every modern vendor ships: each bar's close is measured against the TRUE
   * range extreme -- the previous close when it lies outside today's bar --
   * and the results accumulate.
   *
   *    TRH = max( prevClose, high )      TRL = min( prevClose, low )
   *    AD  = close - TRL   if close > prevClose
   *        = close - TRH   if close < prevClose
   *        = 0             if close == prevClose
   *    WAD = running sum of AD
   *
   * NO VOLUME IS CONSUMED, despite the name. Larry Williams' original
   * multiplies the move by volume; Achelis' modification drops it, the
   * industry attached Williams' name to the modification anyway, and Tulip,
   * pandas-ta-classic, cTrader, TC2000, WealthCharts and MultiCharts all ship
   * the no-volume form. Shipping the volume form under this name would
   * surprise every user, so this is the one place the usual "the original
   * author wins" rule is set aside. The volume-weighted series is a different
   * indicator. What is left once the multiplier is gone is a signed
   * close-to-close move clipped by the true-range extreme and accumulated --
   * momentum measured on the true range -- which is why this is grouped with
   * the other no-volume directional lines (PLUS_DM, MINUS_DM, BOP, WILLR)
   * rather than with AD/OBV.
   *
   * The three-way branch is written with plain > and < rather than any
   * epsilon: the flat arm must fire on exactly-equal consecutive closes and
   * on nothing else, which also keeps -0.0 and NaN behaviour identical
   * across the C, Rust, Java and .NET backends.
   *
   * prevClose is carried in a scalar, so outReal may alias any input: every
   * read of bar i happens before the store at outIdx <= i, and no earlier
   * bar is ever re-read.
   */
  sum = 0.0;
  outIdx = 0;
  /* The first bar of the requested range is measured against itself, i.e. it
   * contributes exactly 0.0. The accumulator therefore restarts wherever the
   * caller starts, which is why this function is flagged path_dependent.
   */
  prevClose = inClose[startIdx];
  for (i = startIdx; i <= endIdx; i += 1) {
    close = inClose[i];
    if (close > prevClose) {
      trueExtreme = inLow[i];
      if (prevClose < trueExtreme) {
        trueExtreme = prevClose;
      }
      sum += close - trueExtreme;
    } else if (close < prevClose) {
      trueExtreme = inHigh[i];
      if (prevClose > trueExtreme) {
        trueExtreme = prevClose;
      }
      sum += close - trueExtreme;
    }
    outReal[((outIdx * outStride) | 0)] = sum;
    outIdx = outIdx + 1;
    prevClose = close;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  sp.sum = sum;
  sp.prevClose = prevClose;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function wadOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): WadStream {
  const sp = new WadStream(core);
  const retCode = wadOpenImpl(sp, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('WAD openAndFill', inHigh.length, startIdx, wadLookback(core));
  }
  throw streamFailure('WAD openAndFill', retCode);
}

/** @internal */
export function wadOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): WadStream {
  const sp = new WadStream(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = wadOpenImpl(sp, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('WAD open', inHigh.length, startIdx, wadLookback(core));
  }
  throw streamFailure('WAD open', retCode);
}

/**
 * Opens a live WAD stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link wad} there.
 *
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
 *   than `wadLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for histories of different lengths.
 *
 * @example
 * ```ts
 * import { wadOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = wadOpen(high, low, close);
 * const peeked = stream.peek(101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function wadOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): WadStream {
  requireArgument('WAD open', 'inHigh', inHigh);
  requireHistory('WAD open', inHigh.length);
  requireArgument('WAD open', 'inLow', inLow);
  requireArgument('WAD open', 'inClose', inClose);
  requireHistoryLength('WAD open', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('WAD open', 'inClose', inClose.length, inHigh.length);
  return wadOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0);
}

/**
 * {@link wadOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link wad}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param outReal - Cumulative accumulation/distribution. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `wadLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for histories of different lengths or an output too
 *   short or starting where another output or a `Float64Array` input starts, or, on
 *   a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { wadOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const out = new Float64Array(high.length);
 * const stream = wadOpenAndFill(high, low, close, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function wadOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): WadStream {
  requireArgument('WAD openAndFill', 'inHigh', inHigh);
  requireHistory('WAD openAndFill', inHigh.length);
  requireArgument('WAD openAndFill', 'inLow', inLow);
  requireArgument('WAD openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('WAD openAndFill', inHigh.length, wadLookback(core));
  requireHistoryLength('WAD openAndFill', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('WAD openAndFill', 'inClose', inClose.length, inHigh.length);
  requireLength('WAD openAndFill', 'outReal', outReal, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) || (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) || (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset)) {
    throw streamFailure('WAD openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inHighF64) || overlaps(outReal, inLowF64) || overlaps(outReal, inCloseF64))) {
    throw streamFailure('WAD openAndFill', RetCode.BAD_PARAM);
  }
  return wadOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outReal);
}
