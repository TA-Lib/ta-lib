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
 *  081226 KL   Initial version.
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link marketfi} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function marketfiLookback(core: Core = Core.DEFAULT): number {
  void core;
  /* Each output depends only on its own bar, so nothing is consumed
   * before the first one can be produced.
   */
  return 0;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link marketfi}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function marketfiDisplayShift(outputIdx: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function marketfiImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inVolume: Float64Array,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let outIdx = 0;
  let i = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  /* Bill Williams' Market Facilitation Index: the price range a bar
   * travelled per unit of volume traded, i.e. how much movement the
   * market "facilitated" per tick.
   *
   *      MARKETFI = ( High - Low ) / Volume
   *
   * Stateless and per-bar: no seeding, no smoothing, no accumulator and
   * no unstable period, so the output for a bar never depends on where
   * the caller started the range.
   *
   * Retail material often abbreviates this "MFI" or "BW MFI". TA_MFI is
   * already the Money Flow Index, so this carries the name Tulip and
   * pandas-ta-classic use.
   */
  outIdx = 0;
  for (i = startIdx; i <= endIdx; i += 1) {
    /* A zero-volume bar would divide by zero. Neither reference guards
     * it -- they emit +/-Inf, or NaN when the range is zero too -- but
     * issue #112 settled that a successful call never emits NaN or Inf,
     * so an untraded bar facilitated no movement and reports 0.
     *
     * The comparison is an exact != 0.0 rather than TA_IS_ZERO, whose
     * 1e-14 band is an absolute threshold and meaningless against an
     * unbounded volume scale. Same reasoning as the prevClose guard in
     * ta_codegen/input/nvi/nvi.c.
     */
    if (inVolume[i] !== 0.0) {
      outReal[outIdx++] = (inHigh[i] - inLow[i]) / inVolume[i];
    } else {
      outReal[outIdx++] = 0.0;
    }
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * Bill Williams' Market Facilitation Index (*Trading Chaos*, 1995): the price
 * range a bar travelled per unit of volume traded — how much movement the market
 * "facilitated" per tick. A rising index on rising volume is read as a move the
 * market is absorbing; a rising index on falling volume as one it is not. Retail
 * material commonly abbreviates this "MFI" or "BW MFI". TA-Lib already ships
 * `TA_MFI` for the Money Flow Index, so this carries the `MARKETFI` name used by
 * Tulip and pandas-ta-classic. Charting packages often overlay a four-state colour
 * code (green / fade / fake / squat) derived from the signs of the bar-to-bar
 * change in this index and in volume. That is an interpretive layer on top of the
 * series, not part of it; `outReal` is the scalar only.
 *
 * Formula and more info at
 * [ta-lib.org/functions/marketfi](https://ta-lib.org/functions/marketfi).
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link marketfiLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inVolume - Volume of each bar.
 * @param outReal - Range travelled per unit of volume, per bar. Must hold the
 *   values produced and never be empty; the input length always suffices.
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
 * import { marketfi } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const out = new Float64Array(high.length);
 * const { begIdx, count } = marketfi(0, high.length - 1, high, low, volume, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`ad`](https://ta-lib.org/functions/ad)
 * @see [`adosc`](https://ta-lib.org/functions/adosc)
 * @see [`nvi`](https://ta-lib.org/functions/nvi)
 * @see [`obv`](https://ta-lib.org/functions/obv)
 * @see [`pvi`](https://ta-lib.org/functions/pvi)
 */
export function marketfi(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('MARKETFI', 'startIdx', startIdx);
  endIdx = requireInt32('MARKETFI', 'endIdx', endIdx);
  requireIndexRange('MARKETFI', startIdx, endIdx);
  const guardStart = clampedStart('MARKETFI', startIdx, marketfiLookback(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('MARKETFI', 'inHigh', inHigh, guardInLen);
  requireLength('MARKETFI', 'inLow', inLow, guardInLen);
  requireLength('MARKETFI', 'inVolume', inVolume, guardInLen);
  requireOutput('MARKETFI', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inVolumeF64 = float64(inVolume);
  if (overlapsPartly(outReal, inHighF64) || overlapsPartly(outReal, inLowF64) || overlapsPartly(outReal, inVolumeF64)) {
    throw failure('MARKETFI', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = marketfiImpl(core, startIdx, endIdx, inHighF64, inLowF64, inVolumeF64, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('MARKETFI', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live MARKETFI stream: one value per closed bar, bit-identical to
 * {@link marketfi} over the same series. Open it with {@link marketfiOpen} or
 * {@link marketfiOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/marketfi](https://ta-lib.org/functions/marketfi).
 */
export class MarketfiStream {
  /** @internal */
  core: Core;
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
  static of(core: Core): MarketfiStream {
    return new MarketfiStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link marketfi} reports them over the same bars. Each accepted `update` and
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
      throw failure('MARKETFI advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inVolume - The bar's `inVolume` value: Volume of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number, inVolume: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('MARKETFI update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inVolume)) {
      throw nonFiniteBar('MARKETFI update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inVolume');
    }
    marketfiStepImpl(this, inHigh, inLow, inVolume);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inVolume - The bar's `inVolume` value: Volume of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number, inVolume: number): number {
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inVolume)) {
      throw nonFiniteBar('MARKETFI peek', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inVolume');
    }
    let cur_outReal = 0;
    /* A zero-volume bar would divide by zero. Neither reference guards
     * it -- they emit +/-Inf, or NaN when the range is zero too -- but
     * issue #112 settled that a successful call never emits NaN or Inf,
     * so an untraded bar facilitated no movement and reports 0.
     *
     * The comparison is an exact != 0.0 rather than TA_IS_ZERO, whose
     * 1e-14 band is an absolute threshold and meaningless against an
     * unbounded volume scale. Same reasoning as the prevClose guard in
     * ta_codegen/input/nvi/nvi.c.
     */
    if (inVolume !== 0.0) {
      cur_outReal = (inHigh - inLow) / inVolume;
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
  clone(): MarketfiStream {
    const c = new MarketfiStream(this.core);
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function marketfiStepImpl(sp: MarketfiStream, inHigh: number, inLow: number, inVolume: number): void {
  /* A zero-volume bar would divide by zero. Neither reference guards
   * it -- they emit +/-Inf, or NaN when the range is zero too -- but
   * issue #112 settled that a successful call never emits NaN or Inf,
   * so an untraded bar facilitated no movement and reports 0.
   *
   * The comparison is an exact != 0.0 rather than TA_IS_ZERO, whose
   * 1e-14 band is an absolute threshold and meaningless against an
   * unbounded volume scale. Same reasoning as the prevClose guard in
   * ta_codegen/input/nvi/nvi.c.
   */
  if (inVolume !== 0.0) {
    sp.cur_outReal = (inHigh - inLow) / inVolume;
  } else {
    sp.cur_outReal = 0.0;
  }
}

function marketfiOpenImpl(
  sp: MarketfiStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let outIdx = 0;
  let i = 0;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length || inVolume.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Bill Williams' Market Facilitation Index: the price range a bar
   * travelled per unit of volume traded, i.e. how much movement the
   * market "facilitated" per tick.
   *
   *      MARKETFI = ( High - Low ) / Volume
   *
   * Stateless and per-bar: no seeding, no smoothing, no accumulator and
   * no unstable period, so the output for a bar never depends on where
   * the caller started the range.
   *
   * Retail material often abbreviates this "MFI" or "BW MFI". TA_MFI is
   * already the Money Flow Index, so this carries the name Tulip and
   * pandas-ta-classic use.
   */
  outIdx = 0;
  for (i = startIdx; i <= endIdx; i += 1) {
    /* A zero-volume bar would divide by zero. Neither reference guards
     * it -- they emit +/-Inf, or NaN when the range is zero too -- but
     * issue #112 settled that a successful call never emits NaN or Inf,
     * so an untraded bar facilitated no movement and reports 0.
     *
     * The comparison is an exact != 0.0 rather than TA_IS_ZERO, whose
     * 1e-14 band is an absolute threshold and meaningless against an
     * unbounded volume scale. Same reasoning as the prevClose guard in
     * ta_codegen/input/nvi/nvi.c.
     */
    if (inVolume[i] !== 0.0) {
      outReal[((outIdx++ * outStride) | 0)] = (inHigh[i] - inLow[i]) / inVolume[i];
    } else {
      outReal[((outIdx++ * outStride) | 0)] = 0.0;
    }
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function marketfiOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): MarketfiStream {
  const sp = MarketfiStream.of(core);
  const retCode = marketfiOpenImpl(sp, inHigh, inLow, inVolume, startIdx, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MARKETFI openAndFill', inHigh.length, startIdx, marketfiLookback(core));
  }
  throw streamFailure('MARKETFI openAndFill', retCode);
}

/** @internal */
export function marketfiOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
): MarketfiStream {
  const sp = MarketfiStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = marketfiOpenImpl(sp, inHigh, inLow, inVolume, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MARKETFI open', inHigh.length, startIdx, marketfiLookback(core));
  }
  throw streamFailure('MARKETFI open', retCode);
}

/**
 * Opens a live MARKETFI stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link marketfi} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inVolume - Warm-up history. Volume of each bar. Every history has the
 *   same length.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `marketfiLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an
 *   empty one, and `TA_BAD_PARAM` for histories of different lengths.
 *
 * @example
 * ```ts
 * import { marketfiOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const stream = marketfiOpen(high, low, volume);
 * const peeked = stream.peek(101.4, 99.1, 12345); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 12345); // the bar closes
 * ```
 */
export function marketfiOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): MarketfiStream {
  requireArgument('MARKETFI open', 'inHigh', inHigh);
  requireHistory('MARKETFI open', inHigh.length);
  requireArgument('MARKETFI open', 'inLow', inLow);
  requireArgument('MARKETFI open', 'inVolume', inVolume);
  requireHistoryLength('MARKETFI open', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('MARKETFI open', 'inVolume', inVolume.length, inHigh.length);
  return marketfiOpenInternal(core, float64(inHigh), float64(inLow), float64(inVolume), 0);
}

/**
 * {@link marketfiOpen} that also fills the outputs over the whole history in the
 * same pass, bit-identically to {@link marketfi}. The range written is the
 * stream's `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inVolume - Warm-up history. Volume of each bar. Every history has the
 *   same length.
 * @param outReal - Range travelled per unit of volume, per bar. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `marketfiLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an
 *   empty one, and `TA_BAD_PARAM` for histories of different lengths or an output
 *   too short or starting where another output or a `Float64Array` input starts, or,
 *   on a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { marketfiOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const out = new Float64Array(high.length);
 * const stream = marketfiOpenAndFill(high, low, volume, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function marketfiOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): MarketfiStream {
  requireArgument('MARKETFI openAndFill', 'inHigh', inHigh);
  requireHistory('MARKETFI openAndFill', inHigh.length);
  requireArgument('MARKETFI openAndFill', 'inLow', inLow);
  requireArgument('MARKETFI openAndFill', 'inVolume', inVolume);
  const guardOutLen = openFillCount('MARKETFI openAndFill', inHigh.length, marketfiLookback(core));
  requireHistoryLength('MARKETFI openAndFill', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('MARKETFI openAndFill', 'inVolume', inVolume.length, inHigh.length);
  requireOutput('MARKETFI openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inVolumeF64 = float64(inVolume);
  if ((outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) || (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) || (outReal.buffer === inVolumeF64.buffer && outReal.byteOffset === inVolumeF64.byteOffset)) {
    throw streamFailure('MARKETFI openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inHighF64) || overlaps(outReal, inLowF64) || overlaps(outReal, inVolumeF64))) {
    throw streamFailure('MARKETFI openAndFill', RetCode.BAD_PARAM);
  }
  return marketfiOpenAndFillInternal(core, inHighF64, inLowF64, inVolumeF64, 0, new MInteger(), new MInteger(), outReal);
}
