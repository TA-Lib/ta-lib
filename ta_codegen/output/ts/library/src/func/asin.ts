/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090807 MF     Initial Version
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function asinLookbackPositional(_core: Core = Core.DEFAULT): number {
  return 0;
}

/** @internal */
export function asinDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function asinImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
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
  for (i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1) {
    outReal[outIdx] = Math.asin(inReal[i]);
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function asinPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('ASIN', 'startIdx', startIdx);
  endIdx = requireInt32('ASIN', 'endIdx', endIdx);
  requireIndexRange('ASIN', startIdx, endIdx);
  const guardStart = clampedStart('ASIN', startIdx, asinLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('ASIN', 'real', inReal, guardInLen);
  requireOutput('ASIN', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('ASIN', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = asinImpl(core, startIdx, endIdx, inRealF64, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('ASIN', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link asin}, {@link asinLookback}, {@link asinDisplayShift},
 * {@link asinOpen} and {@link asinOpenAndFill}. Every key is optional: an omitted
 * or `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface AsinOptions {
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

/** What {@link asin} returns, and {@link asinOpenAndFill} with its stream. */
export interface AsinResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Arcsine of each input, in radians. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link asin} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AsinOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function asinLookback(options?: AsinOptions): number {
  const o = optionsOf('ASIN lookback', options);
  return asinLookbackPositional(coreOf('ASIN lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link asin}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AsinOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function asinDisplayShift(outputIdx: number, options?: AsinOptions): number {
  const o = optionsOf('ASIN displayShift', options);
  return asinDisplayShiftPositional(outputIdx, coreOf('ASIN displayShift', o.core));
}

/**
 * Element-wise arcsine of the input series.
 *
 * Formula and more info at
 * [ta-lib.org/functions/asin](https://ta-lib.org/functions/asin).
 *
 * **Notes**
 * - Outside \[-1, 1] there is no angle whose sine is that value, so those elements
 *   come out NaN.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link asinLookback} is a success with empty outputs.
 *
 * @param real - Input values (domain \[-1,1] for a real result)
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link AsinOptions}.
 * @returns `begIdx` and the values; see {@link AsinResult}. Each output is a view
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
 * import { asin } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) => Math.sin(0.1 * i));
 * const { begIdx, values } = asin(data);
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`acos`](https://ta-lib.org/functions/acos)
 * @see [`atan`](https://ta-lib.org/functions/atan)
 * @see [`sin`](https://ta-lib.org/functions/sin)
 * @see [`cos`](https://ta-lib.org/functions/cos)
 */
export function asin(real: ArrayLike<number>, options?: AsinOptions): AsinResult {
  const o = optionsOf('ASIN', options);
  if (o.endIdx === undefined) {
    requireSeries('ASIN', 'real', real);
  }
  const core = coreOf('ASIN', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, asinLookbackPositional(core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = asinPositional(startIdx, endIdx, real, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live ASIN stream: one value per closed bar, bit-identical to {@link asin} over
 * the same series. Open it with {@link asinOpen} or {@link asinOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/asin](https://ta-lib.org/functions/asin).
 */
export class AsinStream {
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
  static of(core: Core): AsinStream {
    return new AsinStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link asin} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('ASIN advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: Input values (domain \[-1,1] for a real
   *   result)
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('ASIN update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('ASIN update', 'real');
    }
    asinStepImpl(this, real);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: Input values (domain \[-1,1] for a real
   *   result)
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number): number {
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('ASIN peek', 'real');
    }
    const inReal = real;
    let cur_outReal = 0;
    cur_outReal = Math.asin(inReal);
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
  clone(): AsinStream {
    const c = new AsinStream(this.core);
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function asinStepImpl(sp: AsinStream, inReal: number): void {
  sp.cur_outReal = Math.asin(inReal);
}

function asinOpenImpl(
  sp: AsinStream,
  inReal: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let outIdx = 0;
  let i = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  for (i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1) {
    outReal[((outIdx * outStride) | 0)] = Math.asin(inReal[i]);
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function asinOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): AsinStream {
  const sp = AsinStream.of(core);
  const retCode = asinOpenImpl(sp, inReal, startIdx, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ASIN openAndFill', inReal.length, startIdx, asinLookbackPositional(core));
  }
  throw streamFailure('ASIN openAndFill', retCode);
}

/** @internal */
export function asinOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
): AsinStream {
  const sp = AsinStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = asinOpenImpl(sp, inReal, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ASIN open', inReal.length, startIdx, asinLookbackPositional(core));
  }
  throw streamFailure('ASIN open', retCode);
}

/** @internal */
export function asinOpenPositional(
  inReal: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): AsinStream {
  requireArgument('ASIN open', 'real', inReal);
  requireHistory('ASIN open', 'real', inReal.length);
  return asinOpenInternal(core, float64(inReal), 0);
}

/** @internal */
export function asinOpenAndFillPositional(
  inReal: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): AsinStream {
  requireArgument('ASIN openAndFill', 'real', inReal);
  requireHistory('ASIN openAndFill', 'real', inReal.length);
  const guardOutLen = openFillCount('ASIN openAndFill', inReal.length, asinLookbackPositional(core));
  requireOutput('ASIN openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('ASIN openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('ASIN openAndFill', 'outReal', 'real');
  }
  return asinOpenAndFillInternal(core, inRealF64, 0, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live ASIN stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link asin} there.
 *
 * @param real - Warm-up history. Input values (domain \[-1,1] for a real result)
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AsinOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `asinLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core` or an absent
 *   input.
 *
 * @example
 * ```ts
 * import { asinOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) => Math.sin(0.1 * i));
 * const stream = asinOpen(data);
 * const peeked = stream.peek(0.42); // a forming bar: commits nothing
 * const updated = stream.update(0.42); // the bar closes
 * ```
 */
export function asinOpen(real: ArrayLike<number>, options?: AsinOptions): AsinStream {
  const o = optionsOf('ASIN open', options);
  const core = coreOf('ASIN open', o.core);
  return asinOpenPositional(real, core);
}

/**
 * {@link asinOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link asin}.
 *
 * @param real - Warm-up history. Input values (domain \[-1,1] for a real result)
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link AsinOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link asin} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `asinLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input
 *   or a buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { asinOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) => Math.sin(0.1 * i));
 * const { stream, begIdx, values } = asinOpenAndFill(data);
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function asinOpenAndFill(real: ArrayLike<number>, options?: AsinOptions): AsinResult & { stream: AsinStream } {
  const o = optionsOf('ASIN openAndFill', options);
  const core = coreOf('ASIN openAndFill', o.core);
  const capacity = fillCapacity(asinLookbackPositional(core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = asinOpenAndFillPositional(real, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
