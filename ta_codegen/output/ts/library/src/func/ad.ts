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

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link ad} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function adLookback(core: Core = Core.DEFAULT): number {
  void core;
  /* This function have no lookback needed. */
  return 0;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link ad}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function adDisplayShift(outputIdx: number, core: Core = Core.DEFAULT): number {
  void core;
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

/**
 * Chaikin Accumulation/Distribution Line, a cumulative volume-flow indicator. Sums
 * a volume-weighted money-flow multiplier per bar to gauge buying vs. selling
 * pressure. Rising line = accumulation (buying pressure); falling = distribution.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ad](https://ta-lib.org/functions/ad).
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link adLookback} is a success with `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param inVolume - Volume of each bar.
 * @param outReal - Cumulative A/D line value per bar. Must hold the values
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
 * const out = new Float64Array(high.length);
 * const { begIdx, count } = ad(0, high.length - 1, high, low, close, volume, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`adosc`](https://ta-lib.org/functions/adosc)
 * @see [`obv`](https://ta-lib.org/functions/obv)
 */
export function ad(
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
  const guardStart = clampedStart('AD', startIdx, adLookback(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('AD', 'inHigh', inHigh, guardInLen);
  requireLength('AD', 'inLow', inLow, guardInLen);
  requireLength('AD', 'inClose', inClose, guardInLen);
  requireLength('AD', 'inVolume', inVolume, guardInLen);
  requireOutput('AD', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  const inVolumeF64 = float64(inVolume);
  if (overlapsPartly(outReal, inHighF64) || overlapsPartly(outReal, inLowF64) || overlapsPartly(outReal, inCloseF64) || overlapsPartly(outReal, inVolumeF64)) {
    throw failure('AD', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = adImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, inVolumeF64, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('AD', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
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
      throw failure('AD advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param inVolume - The bar's `inVolume` value: Volume of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number, inClose: number, inVolume: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('AD update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose) || !Number.isFinite(inVolume)) {
      throw nonFiniteBar('AD update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : !Number.isFinite(inClose) ? 'inClose' : 'inVolume');
    }
    adStepImpl(this, inHigh, inLow, inClose, inVolume);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param inVolume - The bar's `inVolume` value: Volume of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number, inClose: number, inVolume: number): number {
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose) || !Number.isFinite(inVolume)) {
      throw nonFiniteBar('AD peek', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : !Number.isFinite(inClose) ? 'inClose' : 'inVolume');
    }
    const sp = this;
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
  if (historyLen < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length || inClose.length !== inHigh.length || inVolume.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
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
    throw insufficientHistory('AD openAndFill', inHigh.length, startIdx, adLookback(core));
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
    throw insufficientHistory('AD open', inHigh.length, startIdx, adLookback(core));
  }
  throw streamFailure('AD open', retCode);
}

/**
 * Opens a live AD stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link ad} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param inVolume - Warm-up history. Volume of each bar. Every history has the
 *   same length.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `adLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   histories of different lengths.
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
 * const stream = adOpen(high, low, close, volume);
 * const peeked = stream.peek(101.4, 99.1, 100.9, 12345); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9, 12345); // the bar closes
 * ```
 */
export function adOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): AdStream {
  requireArgument('AD open', 'inHigh', inHigh);
  requireHistory('AD open', inHigh.length);
  requireArgument('AD open', 'inLow', inLow);
  requireArgument('AD open', 'inClose', inClose);
  requireArgument('AD open', 'inVolume', inVolume);
  requireHistoryLength('AD open', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('AD open', 'inClose', inClose.length, inHigh.length);
  requireHistoryLength('AD open', 'inVolume', inVolume.length, inHigh.length);
  return adOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), float64(inVolume), 0);
}

/**
 * {@link adOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link ad}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param inVolume - Warm-up history. Volume of each bar. Every history has the
 *   same length.
 * @param outReal - Cumulative A/D line value per bar. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `adLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   histories of different lengths or an output too short or starting where another
 *   output or a `Float64Array` input starts, or, on a history long enough to open,
 *   one sharing memory with another output or a `Float64Array` input.
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
 * const out = new Float64Array(high.length);
 * const stream = adOpenAndFill(high, low, close, volume, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function adOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): AdStream {
  requireArgument('AD openAndFill', 'inHigh', inHigh);
  requireHistory('AD openAndFill', inHigh.length);
  requireArgument('AD openAndFill', 'inLow', inLow);
  requireArgument('AD openAndFill', 'inClose', inClose);
  requireArgument('AD openAndFill', 'inVolume', inVolume);
  const guardOutLen = openFillCount('AD openAndFill', inHigh.length, adLookback(core));
  requireHistoryLength('AD openAndFill', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('AD openAndFill', 'inClose', inClose.length, inHigh.length);
  requireHistoryLength('AD openAndFill', 'inVolume', inVolume.length, inHigh.length);
  requireOutput('AD openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  const inVolumeF64 = float64(inVolume);
  if ((outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) || (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) || (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset) || (outReal.buffer === inVolumeF64.buffer && outReal.byteOffset === inVolumeF64.byteOffset)) {
    throw streamFailure('AD openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inHighF64) || overlaps(outReal, inLowF64) || overlaps(outReal, inCloseF64) || overlaps(outReal, inVolumeF64))) {
    throw streamFailure('AD openAndFill', RetCode.BAD_PARAM);
  }
  return adOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, inVolumeF64, 0, new MInteger(), new MInteger(), outReal);
}
