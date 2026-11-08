/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AC       Angelo Ciceri
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  010802 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  110206 AC   Change volume and open interest to double
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function obvLookbackPositional(_core: Core = Core.DEFAULT): number {
  /* This function have no lookback needed. */
  return 0;
}

/** @internal */
export function obvDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function obvImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  inVolume: Float64Array,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let i = 0;
  let outIdx = 0;
  let prevReal = 0;
  let tempReal = 0;
  let prevOBV = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  prevOBV = inVolume[startIdx];
  prevReal = inReal[startIdx];
  outIdx = 0;
  for (i = startIdx; i <= endIdx; i += 1) {
    tempReal = inReal[i];
    if (tempReal > prevReal) {
      prevOBV += inVolume[i];
    } else if (tempReal < prevReal) {
      prevOBV -= inVolume[i];
    }
    outReal[outIdx++] = prevOBV;
    prevReal = tempReal;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function obvPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('OBV', 'startIdx', startIdx);
  endIdx = requireInt32('OBV', 'endIdx', endIdx);
  requireIndexRange('OBV', startIdx, endIdx);
  const guardStart = clampedStart('OBV', startIdx, obvLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('OBV', 'real', inReal, guardInLen);
  requireLength('OBV', 'prices.volume', inVolume, guardInLen);
  requireOutput('OBV', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  const inVolumeF64 = float64(inVolume);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('OBV', 'outReal', 'real');
  }
  if (overlapsPartly(outReal, inVolumeF64)) {
    throw sharedMemory('OBV', 'outReal', 'prices.volume');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = obvImpl(core, startIdx, endIdx, inRealF64, inVolumeF64, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('OBV', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link obv}, {@link obvLookback}, {@link obvDisplayShift},
 * {@link obvOpen} and {@link obvOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface ObvOptions {
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

/** What {@link obv} returns, and {@link obvOpenAndFill} with its stream. */
export interface ObvResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Cumulative on-balance volume. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link obv} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link ObvOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function obvLookback(options?: ObvOptions): number {
  const o = optionsOf('OBV lookback', options);
  return obvLookbackPositional(coreOf('OBV lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link obv}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link ObvOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function obvDisplayShift(outputIdx: number, options?: ObvOptions): number {
  const o = optionsOf('OBV displayShift', options);
  return obvDisplayShiftPositional(outputIdx, coreOf('OBV displayShift', o.core));
}

/**
 * On Balance Volume: a running cumulative total of volume, added on up-price bars
 * and subtracted on down-price bars. Relates volume flow to price direction.
 *
 * Formula and more info at
 * [ta-lib.org/functions/obv](https://ta-lib.org/functions/obv).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link obvLookback} is a success with empty outputs.
 *
 * @param real - Price series, typically close.
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `volume`: Volume of
 *   each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link ObvOptions}.
 * @returns `begIdx` and the values; see {@link ObvResult}. Each output is a view
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
 * import { obv } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const { begIdx, values } = obv(data, { volume });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function obv(real: ArrayLike<number>, prices: { open?: ArrayLike<number> | undefined; high?: ArrayLike<number> | undefined; low?: ArrayLike<number> | undefined; close?: ArrayLike<number> | undefined; volume: ArrayLike<number>; openInterest?: ArrayLike<number> | undefined }, options?: ObvOptions): ObvResult {
  requireArgument('OBV', 'prices', prices);
  const o = optionsOf('OBV', options);
  if (o.endIdx === undefined) {
    requireSeries('OBV', 'real', real);
    requireSeries('OBV', 'prices.volume', prices.volume);
  }
  const core = coreOf('OBV', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, obvLookbackPositional(core), real, prices.volume);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = obvPositional(startIdx, endIdx, real, prices.volume, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live OBV stream: one value per closed bar, bit-identical to {@link obv} over
 * the same series. Open it with {@link obvOpen} or {@link obvOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/obv](https://ta-lib.org/functions/obv).
 */
export class ObvStream {
  /** @internal */
  core: Core;
  /** @internal */
  prevReal: number = 0;
  /** @internal */
  prevOBV: number = 0;
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
  static of(core: Core): ObvStream {
    return new ObvStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link obv} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('OBV advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: Price series, typically close.
   * @param volume - The bar's `volume` value: Volume of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number, volume: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('OBV update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real) || !Number.isFinite(volume)) {
      throw nonFiniteBar('OBV update', !Number.isFinite(real) ? 'real' : 'volume');
    }
    obvStepImpl(this, real, volume);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: Price series, typically close.
   * @param volume - The bar's `volume` value: Volume of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number, volume: number): number {
    if (!Number.isFinite(real) || !Number.isFinite(volume)) {
      throw nonFiniteBar('OBV peek', !Number.isFinite(real) ? 'real' : 'volume');
    }
    const sp = this;
    const inReal = real;
    const inVolume = volume;
    let tempReal = 0;
    let cur_outReal = 0;
    let prevOBV = sp.prevOBV;
    tempReal = inReal;
    if (tempReal > sp.prevReal) {
      prevOBV += inVolume;
    } else if (tempReal < sp.prevReal) {
      prevOBV -= inVolume;
    }
    cur_outReal = prevOBV;
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
  clone(): ObvStream {
    const c = new ObvStream(this.core);
    c.prevReal = this.prevReal;
    c.prevOBV = this.prevOBV;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function obvStepImpl(sp: ObvStream, inReal: number, inVolume: number): void {
  let tempReal = 0;
  tempReal = inReal;
  if (tempReal > sp.prevReal) {
    sp.prevOBV += inVolume;
  } else if (tempReal < sp.prevReal) {
    sp.prevOBV -= inVolume;
  }
  sp.cur_outReal = sp.prevOBV;
  sp.prevReal = tempReal;
}

function obvOpenImpl(
  sp: ObvStream,
  inReal: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let i = 0;
  let outIdx = 0;
  let prevReal = 0;
  let tempReal = 0;
  let prevOBV = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inVolume.length !== inReal.length) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  prevOBV = inVolume[startIdx];
  prevReal = inReal[startIdx];
  outIdx = 0;
  for (i = startIdx; i <= endIdx; i += 1) {
    tempReal = inReal[i];
    if (tempReal > prevReal) {
      prevOBV += inVolume[i];
    } else if (tempReal < prevReal) {
      prevOBV -= inVolume[i];
    }
    outReal[((outIdx++ * outStride) | 0)] = prevOBV;
    prevReal = tempReal;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  sp.prevReal = prevReal;
  sp.prevOBV = prevOBV;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function obvOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): ObvStream {
  const sp = ObvStream.of(core);
  const retCode = obvOpenImpl(sp, inReal, inVolume, startIdx, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('OBV openAndFill', inReal.length, startIdx, obvLookbackPositional(core));
  }
  throw streamFailure('OBV openAndFill', retCode);
}

/** @internal */
export function obvOpenInternal(
  core: Core,
  inReal: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
): ObvStream {
  const sp = ObvStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = obvOpenImpl(sp, inReal, inVolume, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('OBV open', inReal.length, startIdx, obvLookbackPositional(core));
  }
  throw streamFailure('OBV open', retCode);
}

/** @internal */
export function obvOpenPositional(
  inReal: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): ObvStream {
  requireArgument('OBV open', 'real', inReal);
  requireHistory('OBV open', 'real', inReal.length);
  requireArgument('OBV open', 'prices.volume', inVolume);
  requireHistoryLength('OBV open', 'prices.volume', inVolume.length, inReal.length);
  return obvOpenInternal(core, float64(inReal), float64(inVolume), 0);
}

/** @internal */
export function obvOpenAndFillPositional(
  inReal: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): ObvStream {
  requireArgument('OBV openAndFill', 'real', inReal);
  requireHistory('OBV openAndFill', 'real', inReal.length);
  requireArgument('OBV openAndFill', 'prices.volume', inVolume);
  const guardOutLen = openFillCount('OBV openAndFill', inReal.length, obvLookbackPositional(core));
  requireHistoryLength('OBV openAndFill', 'prices.volume', inVolume.length, inReal.length);
  requireOutput('OBV openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  const inVolumeF64 = float64(inVolume);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('OBV openAndFill', 'outReal', 'real');
  }
  if (outReal.buffer === inVolumeF64.buffer && outReal.byteOffset === inVolumeF64.byteOffset) {
    throw sharedMemory('OBV openAndFill', 'outReal', 'prices.volume');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('OBV openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inVolumeF64)) {
    throw sharedMemory('OBV openAndFill', 'outReal', 'prices.volume');
  }
  return obvOpenAndFillInternal(core, inRealF64, inVolumeF64, 0, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live OBV stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link obv} there.
 *
 * @param real - Warm-up history. Price series, typically close.
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `volume`: Volume of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link ObvOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `obvLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input
 *   or histories of different lengths.
 *
 * @example
 * ```ts
 * import { obvOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const stream = obvOpen(data, { volume });
 * const peeked = stream.peek(100.9, 12345); // a forming bar: commits nothing
 * const updated = stream.update(100.9, 12345); // the bar closes
 * ```
 */
export function obvOpen(real: ArrayLike<number>, prices: { open?: ArrayLike<number> | undefined; high?: ArrayLike<number> | undefined; low?: ArrayLike<number> | undefined; close?: ArrayLike<number> | undefined; volume: ArrayLike<number>; openInterest?: ArrayLike<number> | undefined }, options?: ObvOptions): ObvStream {
  requireArgument('OBV open', 'prices', prices);
  const o = optionsOf('OBV open', options);
  const core = coreOf('OBV open', o.core);
  return obvOpenPositional(real, prices.volume, core);
}

/**
 * {@link obvOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link obv}.
 *
 * @param real - Warm-up history. Price series, typically close.
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `volume`: Volume of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link ObvOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link obv} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `obvLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { obvOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const { stream, begIdx, values } = obvOpenAndFill(data, { volume });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function obvOpenAndFill(real: ArrayLike<number>, prices: { open?: ArrayLike<number> | undefined; high?: ArrayLike<number> | undefined; low?: ArrayLike<number> | undefined; close?: ArrayLike<number> | undefined; volume: ArrayLike<number>; openInterest?: ArrayLike<number> | undefined }, options?: ObvOptions): ObvResult & { stream: ObvStream } {
  requireArgument('OBV openAndFill', 'prices', prices);
  const o = optionsOf('OBV openAndFill', options);
  const core = coreOf('OBV openAndFill', o.core);
  const capacity = fillCapacity(obvLookbackPositional(core), real, prices.volume);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = obvOpenAndFillPositional(real, prices.volume, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
