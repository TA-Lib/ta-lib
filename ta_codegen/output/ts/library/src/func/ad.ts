/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  JD       jdoyle
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  120802 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  111705 MF,JD  Fix#1359452 for handling properly start/end range.
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function adLookbackPositional(_core: Core = Core.DEFAULT): number {
  /* This function have no lookback needed. */
  return 0;
}

/** @internal */
export function adDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function adImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  inVolume: Float64Array,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let nbBar = 0;
  let currentBar = 0;
  let outIdx = 0;
  let high = 0;
  let low = 0;
  let close = 0;
  let tmp = 0;
  let ad = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  /* Note: Results from this function might vary slightly
   *       from Metastock outputs. The reason being that
   *       Metastock use float instead of double and this
   *       cause a different floating-point precision to
   *       be used.
   *
   *       For most function, this is not an apparent difference
   *       but for function using large cummulative values (like
   *       this AD function), minor imprecision adds up and becomes
   *       significative.
   *
   *       For better precision, TA-Lib use double in all its
   *       its calculations.
   */
  /* Default return values */
  nbBar = endIdx - startIdx + 1;
  outNBElement.value = nbBar;
  outBegIdx.value = startIdx;
  currentBar = startIdx;
  outIdx = 0;
  ad = 0.0;
  while (nbBar !== 0) {
    high = inHigh[currentBar];
    low = inLow[currentBar];
    tmp = high - low;
    close = inClose[currentBar];
    if (tmp > 0.0) {
      ad += (close - low - (high - close)) / tmp * inVolume[currentBar];
    }
    outReal[outIdx++] = ad;
    currentBar += 1;
    nbBar -= 1;
  }
  return RetCode.SUCCESS;
}

/** @internal */
export function adPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('AD', 'startIdx', startIdx);
  endIdx = requireInt32('AD', 'endIdx', endIdx);
  requireIndexRange('AD', startIdx, endIdx);
  const guardStart = clampedStart('AD', startIdx, adLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('AD', 'prices.high', inHigh, guardInLen);
  requireLength('AD', 'prices.low', inLow, guardInLen);
  requireLength('AD', 'prices.close', inClose, guardInLen);
  requireLength('AD', 'prices.volume', inVolume, guardInLen);
  requireOutput('AD', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  const inVolumeF64 = float64(inVolume);
  if (overlapsPartly(outReal, inHighF64)) {
    throw sharedMemory('AD', 'outReal', 'prices.high');
  }
  if (overlapsPartly(outReal, inLowF64)) {
    throw sharedMemory('AD', 'outReal', 'prices.low');
  }
  if (overlapsPartly(outReal, inCloseF64)) {
    throw sharedMemory('AD', 'outReal', 'prices.close');
  }
  if (overlapsPartly(outReal, inVolumeF64)) {
    throw sharedMemory('AD', 'outReal', 'prices.volume');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = adImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, inVolumeF64, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('AD', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link ad}, {@link adLookback}, {@link adDisplayShift},
 * {@link adOpen} and {@link adOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface AdOptions {
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

/** What {@link ad} returns, and {@link adOpenAndFill} with its stream. */
export interface AdResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Cumulative A/D line value per bar. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link ad} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AdOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function adLookback(options?: AdOptions): number {
  const o = optionsOf('AD lookback', options);
  return adLookbackPositional(coreOf('AD lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link ad}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AdOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function adDisplayShift(outputIdx: number, options?: AdOptions): number {
  const o = optionsOf('AD displayShift', options);
  return adDisplayShiftPositional(outputIdx, coreOf('AD displayShift', o.core));
}

/**
 * Chaikin Accumulation/Distribution Line, a cumulative volume-flow indicator. Sums
 * a volume-weighted money-flow multiplier per bar to gauge buying vs. selling
 * pressure. Rising line = accumulation (buying pressure); falling = distribution.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ad](https://ta-lib.org/functions/ad).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link adLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `high`: High price of
 *   each bar. `low`: Low price of each bar. `close`: Close price of each bar.
 *   `volume`: Volume of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link AdOptions}.
 * @returns `begIdx` and the values; see {@link AdResult}. Each output is a view of
 *   its buffer, the caller's or a new one.
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
 * import { ad } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const { begIdx, values } = ad({ high, low, close, volume });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`adosc`](https://ta-lib.org/functions/adosc)
 * @see [`obv`](https://ta-lib.org/functions/obv)
 */
export function ad(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume: ArrayLike<number>; openInterest?: ArrayLike<number> | undefined }, options?: AdOptions): AdResult {
  requireArgument('AD', 'prices', prices);
  const o = optionsOf('AD', options);
  if (o.endIdx === undefined) {
    requireSeries('AD', 'prices.high', prices.high);
    requireSeries('AD', 'prices.low', prices.low);
    requireSeries('AD', 'prices.close', prices.close);
    requireSeries('AD', 'prices.volume', prices.volume);
  }
  const core = coreOf('AD', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, adLookbackPositional(core), prices.high, prices.low, prices.close, prices.volume);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = adPositional(startIdx, endIdx, prices.high, prices.low, prices.close, prices.volume, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live AD stream: one value per closed bar, bit-identical to {@link ad} over the
 * same series. Open it with {@link adOpen} or {@link adOpenAndFill}; there is
 * nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ad](https://ta-lib.org/functions/ad).
 */
export class AdStream {
  /** @internal */
  core: Core;
  /** @internal */
  ad: number = 0;
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
  static of(core: Core): AdStream {
    return new AdStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link ad} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('AD advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @param volume - The bar's `volume` value: Volume of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(high: number, low: number, close: number, volume: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('AD update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close) || !Number.isFinite(volume)) {
      throw nonFiniteBar('AD update', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : !Number.isFinite(close) ? 'close' : 'volume');
    }
    adStepImpl(this, high, low, close, volume);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @param volume - The bar's `volume` value: Volume of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(high: number, low: number, close: number, volume: number): number {
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close) || !Number.isFinite(volume)) {
      throw nonFiniteBar('AD peek', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : !Number.isFinite(close) ? 'close' : 'volume');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
    const inClose = close;
    const inVolume = volume;
    {
      let high = 0;
      let low = 0;
      let close = 0;
      let tmp = 0;
      let ad = sp.ad;
      let cur_outReal = 0;
      high = inHigh;
      low = inLow;
      tmp = high - low;
      close = inClose;
      if (tmp > 0.0) {
        ad += (close - low - (high - close)) / tmp * inVolume;
      }
      cur_outReal = ad;
      return cur_outReal;
    }
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
  clone(): AdStream {
    const c = new AdStream(this.core);
    c.ad = this.ad;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function adStepImpl(sp: AdStream, inHigh: number, inLow: number, inClose: number, inVolume: number): void {
  let high = 0;
  let low = 0;
  let close = 0;
  let tmp = 0;
  high = inHigh;
  low = inLow;
  tmp = high - low;
  close = inClose;
  if (tmp > 0.0) {
    sp.ad += (close - low - (high - close)) / tmp * inVolume;
  }
  sp.cur_outReal = sp.ad;
}

function adOpenImpl(
  sp: AdStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let nbBar = 0;
  let currentBar = 0;
  let outIdx = 0;
  let high = 0;
  let low = 0;
  let close = 0;
  let tmp = 0;
  let ad = 0;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length || inClose.length !== inHigh.length || inVolume.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Note: Results from this function might vary slightly
   *       from Metastock outputs. The reason being that
   *       Metastock use float instead of double and this
   *       cause a different floating-point precision to
   *       be used.
   *
   *       For most function, this is not an apparent difference
   *       but for function using large cummulative values (like
   *       this AD function), minor imprecision adds up and becomes
   *       significative.
   *
   *       For better precision, TA-Lib use double in all its
   *       its calculations.
   */
  /* Default return values */
  nbBar = endIdx - startIdx + 1;
  outNBElement.value = nbBar;
  outBegIdx.value = startIdx;
  currentBar = startIdx;
  outIdx = 0;
  ad = 0.0;
  while (nbBar !== 0) {
    high = inHigh[currentBar];
    low = inLow[currentBar];
    tmp = high - low;
    close = inClose[currentBar];
    if (tmp > 0.0) {
      ad += (close - low - (high - close)) / tmp * inVolume[currentBar];
    }
    outReal[((outIdx++ * outStride) | 0)] = ad;
    currentBar += 1;
    nbBar -= 1;
  }
  sp.ad = ad;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function adOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): AdStream {
  const sp = AdStream.of(core);
  const retCode = adOpenImpl(sp, inHigh, inLow, inClose, inVolume, startIdx, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('AD openAndFill', inHigh.length, startIdx, adLookbackPositional(core));
  }
  throw streamFailure('AD openAndFill', retCode);
}

/** @internal */
export function adOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
): AdStream {
  const sp = AdStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = adOpenImpl(sp, inHigh, inLow, inClose, inVolume, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('AD open', inHigh.length, startIdx, adLookbackPositional(core));
  }
  throw streamFailure('AD open', retCode);
}

/** @internal */
export function adOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): AdStream {
  requireArgument('AD open', 'prices.high', inHigh);
  requireHistory('AD open', 'prices.high', inHigh.length);
  requireArgument('AD open', 'prices.low', inLow);
  requireArgument('AD open', 'prices.close', inClose);
  requireArgument('AD open', 'prices.volume', inVolume);
  requireHistoryLength('AD open', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('AD open', 'prices.close', inClose.length, inHigh.length);
  requireHistoryLength('AD open', 'prices.volume', inVolume.length, inHigh.length);
  return adOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), float64(inVolume), 0);
}

/** @internal */
export function adOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): AdStream {
  requireArgument('AD openAndFill', 'prices.high', inHigh);
  requireHistory('AD openAndFill', 'prices.high', inHigh.length);
  requireArgument('AD openAndFill', 'prices.low', inLow);
  requireArgument('AD openAndFill', 'prices.close', inClose);
  requireArgument('AD openAndFill', 'prices.volume', inVolume);
  const guardOutLen = openFillCount('AD openAndFill', inHigh.length, adLookbackPositional(core));
  requireHistoryLength('AD openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('AD openAndFill', 'prices.close', inClose.length, inHigh.length);
  requireHistoryLength('AD openAndFill', 'prices.volume', inVolume.length, inHigh.length);
  requireOutput('AD openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  const inVolumeF64 = float64(inVolume);
  if (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('AD openAndFill', 'outReal', 'prices.high');
  }
  if (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('AD openAndFill', 'outReal', 'prices.low');
  }
  if (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('AD openAndFill', 'outReal', 'prices.close');
  }
  if (outReal.buffer === inVolumeF64.buffer && outReal.byteOffset === inVolumeF64.byteOffset) {
    throw sharedMemory('AD openAndFill', 'outReal', 'prices.volume');
  }
  if (guardOutLen > 0 && overlaps(outReal, inHighF64)) {
    throw sharedMemory('AD openAndFill', 'outReal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outReal, inLowF64)) {
    throw sharedMemory('AD openAndFill', 'outReal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outReal, inCloseF64)) {
    throw sharedMemory('AD openAndFill', 'outReal', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outReal, inVolumeF64)) {
    throw sharedMemory('AD openAndFill', 'outReal', 'prices.volume');
  }
  return adOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, inVolumeF64, 0, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live AD stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link ad} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar. `close`: Close
 *   price of each bar. `volume`: Volume of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AdOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `adLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input
 *   or histories of different lengths.
 *
 * @example
 * ```ts
 * import { adOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const stream = adOpen({ high, low, close, volume });
 * const peeked = stream.peek(101.4, 99.1, 100.9, 12345); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9, 12345); // the bar closes
 * ```
 */
export function adOpen(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume: ArrayLike<number>; openInterest?: ArrayLike<number> | undefined }, options?: AdOptions): AdStream {
  requireArgument('AD open', 'prices', prices);
  const o = optionsOf('AD open', options);
  const core = coreOf('AD open', o.core);
  return adOpenPositional(prices.high, prices.low, prices.close, prices.volume, core);
}

/**
 * {@link adOpen} that also computes the outputs over the whole history in the same
 * pass, bit-identically to {@link ad}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar. `close`: Close
 *   price of each bar. `volume`: Volume of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link AdOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link ad} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `adLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { adOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const { stream, begIdx, values } = adOpenAndFill({ high, low, close, volume });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function adOpenAndFill(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume: ArrayLike<number>; openInterest?: ArrayLike<number> | undefined }, options?: AdOptions): AdResult & { stream: AdStream } {
  requireArgument('AD openAndFill', 'prices', prices);
  const o = optionsOf('AD openAndFill', options);
  const core = coreOf('AD openAndFill', o.core);
  const capacity = fillCapacity(adLookbackPositional(core), prices.high, prices.low, prices.close, prices.volume);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = adOpenAndFillPositional(prices.high, prices.low, prices.close, prices.volume, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
