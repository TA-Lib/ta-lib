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

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function marketfiLookbackPositional(_core: Core = Core.DEFAULT): number {
  /* Each output depends only on its own bar, so nothing is consumed
   * before the first one can be produced.
   */
  return 0;
}

/** @internal */
export function marketfiDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
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

/** @internal */
export function marketfiPositional(
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
  const guardStart = clampedStart('MARKETFI', startIdx, marketfiLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('MARKETFI', 'prices.high', inHigh, guardInLen);
  requireLength('MARKETFI', 'prices.low', inLow, guardInLen);
  requireLength('MARKETFI', 'prices.volume', inVolume, guardInLen);
  requireOutput('MARKETFI', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inVolumeF64 = float64(inVolume);
  if (overlapsPartly(outReal, inHighF64)) {
    throw sharedMemory('MARKETFI', 'outReal', 'prices.high');
  }
  if (overlapsPartly(outReal, inLowF64)) {
    throw sharedMemory('MARKETFI', 'outReal', 'prices.low');
  }
  if (overlapsPartly(outReal, inVolumeF64)) {
    throw sharedMemory('MARKETFI', 'outReal', 'prices.volume');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = marketfiImpl(core, startIdx, endIdx, inHighF64, inLowF64, inVolumeF64, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('MARKETFI', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link marketfi}, {@link marketfiLookback},
 * {@link marketfiDisplayShift}, {@link marketfiOpen} and
 * {@link marketfiOpenAndFill}. Every key is optional: an omitted or `undefined`
 * key takes its default, and a call ignores the keys it has no use for.
 */
export interface MarketfiOptions {
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

/** What {@link marketfi} returns, and {@link marketfiOpenAndFill} with its stream. */
export interface MarketfiResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Range travelled per unit of volume, per bar. `values[k]` is the value at input
   * bar `begIdx + k`.
   */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link marketfi} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MarketfiOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function marketfiLookback(options?: MarketfiOptions): number {
  const o = optionsOf('MARKETFI lookback', options);
  return marketfiLookbackPositional(coreOf('MARKETFI lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link marketfi}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MarketfiOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function marketfiDisplayShift(outputIdx: number, options?: MarketfiOptions): number {
  const o = optionsOf('MARKETFI displayShift', options);
  return marketfiDisplayShiftPositional(outputIdx, coreOf('MARKETFI displayShift', o.core));
}

/**
 * Bill Williams' Market Facilitation Index (*Trading Chaos*, 1995): the price
 * range a bar travelled per unit of volume traded — how much movement the market
 * "facilitated" per tick. A rising index on rising volume is read as a move the
 * market is absorbing; a rising index on falling volume as one it is not. Retail
 * material commonly abbreviates this "MFI" or "BW MFI". TA-Lib already ships `mfi`
 * for the Money Flow Index, so this carries the `MARKETFI` name used by Tulip and
 * pandas-ta-classic. Charting packages often overlay a four-state colour code
 * (green / fade / fake / squat) derived from the signs of the bar-to-bar change in
 * this index and in volume. That is an interpretive layer on top of the series,
 * not part of it; `outReal` is the scalar only.
 *
 * Formula and more info at
 * [ta-lib.org/functions/marketfi](https://ta-lib.org/functions/marketfi).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link marketfiLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `high`: High
 *   price of each bar. `low`: Low price of each bar. `volume`: Volume of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link MarketfiOptions}.
 * @returns `begIdx` and the values; see {@link MarketfiResult}. Each output is a
 *   view of its buffer, the caller's or a new one.
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
 * import { marketfi } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const { begIdx, values } = marketfi({ high, low, volume });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`ad`](https://ta-lib.org/functions/ad)
 * @see [`adosc`](https://ta-lib.org/functions/adosc)
 * @see [`nvi`](https://ta-lib.org/functions/nvi)
 * @see [`obv`](https://ta-lib.org/functions/obv)
 * @see [`pvi`](https://ta-lib.org/functions/pvi)
 */
export function marketfi(prices: { high: ArrayLike<number>; low: ArrayLike<number>; volume: ArrayLike<number> }, options?: MarketfiOptions): MarketfiResult {
  requireArgument('MARKETFI', 'prices', prices);
  const o = optionsOf('MARKETFI', options);
  if (o.endIdx === undefined) {
    requireSeries('MARKETFI', 'prices.high', prices.high);
    requireSeries('MARKETFI', 'prices.low', prices.low);
    requireSeries('MARKETFI', 'prices.volume', prices.volume);
  }
  const core = coreOf('MARKETFI', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, marketfiLookbackPositional(core), prices.high, prices.low, prices.volume);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = marketfiPositional(startIdx, endIdx, prices.high, prices.low, prices.volume, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
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
      throw streamFailure('MARKETFI advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param volume - The bar's `volume` value: Volume of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(high: number, low: number, volume: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('MARKETFI update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(volume)) {
      throw nonFiniteBar('MARKETFI update', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'volume');
    }
    marketfiStepImpl(this, high, low, volume);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param volume - The bar's `volume` value: Volume of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(high: number, low: number, volume: number): number {
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(volume)) {
      throw nonFiniteBar('MARKETFI peek', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'volume');
    }
    const inHigh = high;
    const inLow = low;
    const inVolume = volume;
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
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length || inVolume.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
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
    throw insufficientHistory('MARKETFI openAndFill', inHigh.length, startIdx, marketfiLookbackPositional(core));
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
    throw insufficientHistory('MARKETFI open', inHigh.length, startIdx, marketfiLookbackPositional(core));
  }
  throw streamFailure('MARKETFI open', retCode);
}

/** @internal */
export function marketfiOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): MarketfiStream {
  requireArgument('MARKETFI open', 'prices.high', inHigh);
  requireHistory('MARKETFI open', 'prices.high', inHigh.length);
  requireArgument('MARKETFI open', 'prices.low', inLow);
  requireArgument('MARKETFI open', 'prices.volume', inVolume);
  requireHistoryLength('MARKETFI open', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('MARKETFI open', 'prices.volume', inVolume.length, inHigh.length);
  return marketfiOpenInternal(core, float64(inHigh), float64(inLow), float64(inVolume), 0);
}

/** @internal */
export function marketfiOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): MarketfiStream {
  requireArgument('MARKETFI openAndFill', 'prices.high', inHigh);
  requireHistory('MARKETFI openAndFill', 'prices.high', inHigh.length);
  requireArgument('MARKETFI openAndFill', 'prices.low', inLow);
  requireArgument('MARKETFI openAndFill', 'prices.volume', inVolume);
  const guardOutLen = openFillCount('MARKETFI openAndFill', inHigh.length, marketfiLookbackPositional(core));
  requireHistoryLength('MARKETFI openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('MARKETFI openAndFill', 'prices.volume', inVolume.length, inHigh.length);
  requireOutput('MARKETFI openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inVolumeF64 = float64(inVolume);
  if (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('MARKETFI openAndFill', 'outReal', 'prices.high');
  }
  if (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('MARKETFI openAndFill', 'outReal', 'prices.low');
  }
  if (outReal.buffer === inVolumeF64.buffer && outReal.byteOffset === inVolumeF64.byteOffset) {
    throw sharedMemory('MARKETFI openAndFill', 'outReal', 'prices.volume');
  }
  if (guardOutLen > 0 && overlaps(outReal, inHighF64)) {
    throw sharedMemory('MARKETFI openAndFill', 'outReal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outReal, inLowF64)) {
    throw sharedMemory('MARKETFI openAndFill', 'outReal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outReal, inVolumeF64)) {
    throw sharedMemory('MARKETFI openAndFill', 'outReal', 'prices.volume');
  }
  return marketfiOpenAndFillInternal(core, inHighF64, inLowF64, inVolumeF64, 0, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live MARKETFI stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link marketfi} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar. `volume`:
 *   Volume of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MarketfiOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `marketfiLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input or histories of different lengths.
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
 * const stream = marketfiOpen({ high, low, volume });
 * const peeked = stream.peek(101.4, 99.1, 12345); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 12345); // the bar closes
 * ```
 */
export function marketfiOpen(prices: { high: ArrayLike<number>; low: ArrayLike<number>; volume: ArrayLike<number> }, options?: MarketfiOptions): MarketfiStream {
  requireArgument('MARKETFI open', 'prices', prices);
  const o = optionsOf('MARKETFI open', options);
  const core = coreOf('MARKETFI open', o.core);
  return marketfiOpenPositional(prices.high, prices.low, prices.volume, core);
}

/**
 * {@link marketfiOpen} that also computes the outputs over the whole history in
 * the same pass, bit-identically to {@link marketfi}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar. `volume`:
 *   Volume of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link MarketfiOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link marketfi} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `marketfiLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, histories of different lengths or a buffer too short, of the wrong type
 *   or starting where another buffer or a `Float64Array` input starts, or, on a
 *   history long enough to open, one sharing memory with another buffer or a
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
 * const { stream, begIdx, values } = marketfiOpenAndFill({ high, low, volume });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function marketfiOpenAndFill(prices: { high: ArrayLike<number>; low: ArrayLike<number>; volume: ArrayLike<number> }, options?: MarketfiOptions): MarketfiResult & { stream: MarketfiStream } {
  requireArgument('MARKETFI openAndFill', 'prices', prices);
  const o = optionsOf('MARKETFI openAndFill', options);
  const core = coreOf('MARKETFI openAndFill', o.core);
  const capacity = fillCapacity(marketfiLookbackPositional(core), prices.high, prices.low, prices.volume);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = marketfiOpenAndFillPositional(prices.high, prices.low, prices.volume, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
