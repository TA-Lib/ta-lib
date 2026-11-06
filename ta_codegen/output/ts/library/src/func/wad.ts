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

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function wadLookbackPositional(_core: Core = Core.DEFAULT): number {
  /* The first bar has no previous close, so it accumulates nothing and the
   * line starts at 0.0 -- the same convention as the other cumulative
   * lines in the tree: OBV, AD, NVI, PVI and PVT all return 0 here and emit
   * a seed value at startIdx. Tulip's ti_wad_start() returns 1 instead, so its
   * series is this one without the leading zero.
   */
  return 0;
}

/** @internal */
export function wadDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
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

/** @internal */
export function wadPositional(
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
  const guardStart = clampedStart('WAD', startIdx, wadLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('WAD', 'prices.high', inHigh, guardInLen);
  requireLength('WAD', 'prices.low', inLow, guardInLen);
  requireLength('WAD', 'prices.close', inClose, guardInLen);
  requireOutput('WAD', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outReal, inHighF64)) {
    throw sharedMemory('WAD', 'outReal', 'prices.high');
  }
  if (overlapsPartly(outReal, inLowF64)) {
    throw sharedMemory('WAD', 'outReal', 'prices.low');
  }
  if (overlapsPartly(outReal, inCloseF64)) {
    throw sharedMemory('WAD', 'outReal', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = wadImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('WAD', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link wad}, {@link wadLookback}, {@link wadDisplayShift},
 * {@link wadOpen} and {@link wadOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface WadOptions {
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

/** What {@link wad} returns, and {@link wadOpenAndFill} with its stream. */
export interface WadResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Cumulative accumulation/distribution. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link wad} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link WadOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function wadLookback(options?: WadOptions): number {
  const o = optionsOf('WAD lookback', options);
  return wadLookbackPositional(coreOf('WAD lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link wad}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link WadOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function wadDisplayShift(outputIdx: number, options?: WadOptions): number {
  const o = optionsOf('WAD displayShift', options);
  return wadDisplayShiftPositional(outputIdx, coreOf('WAD displayShift', o.core));
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
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link wadLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `high`: High
 *   price of each bar. `low`: Low price of each bar. `close`: Close price of each
 *   bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link WadOptions}.
 * @returns `begIdx` and the values; see {@link WadResult}. Each output is a view
 *   of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, an input too short for the range, a buffer that is empty, too short or of
 *   the wrong type or a buffer sharing memory with a `Float64Array` input it is not.
 *   Checked before anything is written.
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
 * const { begIdx, values } = wad({ high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`ad`](https://ta-lib.org/functions/ad)
 * @see [`adosc`](https://ta-lib.org/functions/adosc)
 * @see [`nvi`](https://ta-lib.org/functions/nvi)
 * @see [`obv`](https://ta-lib.org/functions/obv)
 * @see [`pvi`](https://ta-lib.org/functions/pvi)
 */
export function wad(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: WadOptions): WadResult {
  requireArgument('WAD', 'prices', prices);
  const o = optionsOf('WAD', options);
  if (o.endIdx === undefined) {
    requireSeries('WAD', 'prices.high', prices.high);
    requireSeries('WAD', 'prices.low', prices.low);
    requireSeries('WAD', 'prices.close', prices.close);
  }
  const core = coreOf('WAD', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, wadLookbackPositional(core), prices.high, prices.low, prices.close);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = wadPositional(startIdx, endIdx, prices.high, prices.low, prices.close, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
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

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): WadStream {
    return new WadStream(core);
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
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length || inClose.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
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
  const sp = WadStream.of(core);
  const retCode = wadOpenImpl(sp, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('WAD openAndFill', inHigh.length, startIdx, wadLookbackPositional(core));
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
  const sp = WadStream.of(core);
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
    throw insufficientHistory('WAD open', inHigh.length, startIdx, wadLookbackPositional(core));
  }
  throw streamFailure('WAD open', retCode);
}

/** @internal */
export function wadOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): WadStream {
  requireArgument('WAD open', 'prices.high', inHigh);
  requireHistory('WAD open', inHigh.length);
  requireArgument('WAD open', 'prices.low', inLow);
  requireArgument('WAD open', 'prices.close', inClose);
  requireHistoryLength('WAD open', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('WAD open', 'prices.close', inClose.length, inHigh.length);
  return wadOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0);
}

/** @internal */
export function wadOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): WadStream {
  requireArgument('WAD openAndFill', 'prices.high', inHigh);
  requireHistory('WAD openAndFill', inHigh.length);
  requireArgument('WAD openAndFill', 'prices.low', inLow);
  requireArgument('WAD openAndFill', 'prices.close', inClose);
  const guardOutLen = openFillCount('WAD openAndFill', inHigh.length, wadLookbackPositional(core));
  requireHistoryLength('WAD openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('WAD openAndFill', 'prices.close', inClose.length, inHigh.length);
  requireOutput('WAD openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('WAD openAndFill', 'outReal', 'prices.high');
  }
  if (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('WAD openAndFill', 'outReal', 'prices.low');
  }
  if (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('WAD openAndFill', 'outReal', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outReal, inHighF64)) {
    throw sharedMemory('WAD openAndFill', 'outReal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outReal, inLowF64)) {
    throw sharedMemory('WAD openAndFill', 'outReal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outReal, inCloseF64)) {
    throw sharedMemory('WAD openAndFill', 'outReal', 'prices.close');
  }
  return wadOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live WAD stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link wad} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link WadOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `wadLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input
 *   or histories of different lengths.
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
 * const stream = wadOpen({ high, low, close });
 * const peeked = stream.peek(101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function wadOpen(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: WadOptions): WadStream {
  requireArgument('WAD open', 'prices', prices);
  const o = optionsOf('WAD open', options);
  const core = coreOf('WAD open', o.core);
  return wadOpenPositional(prices.high, prices.low, prices.close, core);
}

/**
 * {@link wadOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link wad}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link WadOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link wad} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `wadLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
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
 * const { stream, begIdx, values } = wadOpenAndFill({ high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function wadOpenAndFill(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: WadOptions): WadResult & { stream: WadStream } {
  requireArgument('WAD openAndFill', 'prices', prices);
  const o = optionsOf('WAD openAndFill', options);
  const core = coreOf('WAD openAndFill', o.core);
  const capacity = fillCapacity(wadLookbackPositional(core), prices.high, prices.low, prices.close);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = wadOpenAndFillPositional(prices.high, prices.low, prices.close, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
