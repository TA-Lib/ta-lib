/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090526 KL     First version (issue #372).
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function cumsumLookbackPositional(_core: Core = Core.DEFAULT): number {
  return 0;
}

/** @internal */
export function cumsumDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cumsumImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let total = 0;
  let i = 0;
  let outIdx = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  /* Running total from the ANCHOR bar forward: the accumulator re-seeds at
   * startIdx, exactly as every shipped path-dependent accumulator does
   * (ad.c anchors at startIdx with ad = 0.0, obv.c seeds at
   * inVolume[startIdx]) -- TA_CUMSUM(3, 7, x)[0] is x[3], NOT sum(x[0..3]).
   * "From bar 0" could only mean "from the start of whatever buffer the
   * caller passed", under which a composed call handed inReal + off silently
   * changes answer; the path_dependent flag exists to declare this class
   * (issue #372). Left-to-right, one double, no compensation -- ad.c's own
   * plain += convention, and what both external oracles compute bit-exactly.
   *
   * Two statements, not outReal[outIdx] = (total += ...): a value-producing
   * compound assignment appears in zero input files, and Rust has no
   * assignment expression, so the one-liner cannot lower to a required
   * backend.
   */
  total = 0.0;
  for (i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1) {
    total += inReal[i];
    outReal[outIdx] = total;
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function cumsumPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CUMSUM', 'startIdx', startIdx);
  endIdx = requireInt32('CUMSUM', 'endIdx', endIdx);
  requireIndexRange('CUMSUM', startIdx, endIdx);
  const guardStart = clampedStart('CUMSUM', startIdx, cumsumLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CUMSUM', 'real', inReal, guardInLen);
  requireOutput('CUMSUM', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('CUMSUM', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cumsumImpl(core, startIdx, endIdx, inRealF64, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CUMSUM', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cumsum}, {@link cumsumLookback}, {@link cumsumDisplayShift},
 * {@link cumsumOpen} and {@link cumsumOpenAndFill}. Every key is optional: an
 * omitted or `undefined` key takes its default, and a call ignores the keys it has
 * no use for.
 */
export interface CumsumOptions {
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

/** What {@link cumsum} returns, and {@link cumsumOpenAndFill} with its stream. */
export interface CumsumResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Running total since the anchor bar. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link cumsum} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CumsumOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cumsumLookback(options?: CumsumOptions): number {
  const o = optionsOf('CUMSUM lookback', options);
  return cumsumLookbackPositional(coreOf('CUMSUM lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cumsum}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CumsumOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cumsumDisplayShift(outputIdx: number, options?: CumsumOptions): number {
  const o = optionsOf('CUMSUM displayShift', options);
  return cumsumDisplayShiftPositional(outputIdx, coreOf('CUMSUM displayShift', o.core));
}

/**
 * Cumulative Sum: the running total of a series from the anchor bar forward. A
 * math primitive rather than an indicator — it is the one operation between the
 * shipped corpus and three named classical breadth indicators: the A/D Line is
 * `CUMSUM(SUB(advances, declines))`, the A/D Volume Line is
 * `CUMSUM(SUB(advancingVolume, decliningVolume))`, and the McClellan Summation
 * Index is `CUMSUM` of the McClellan Oscillator.
 * [`SUM`](https://ta-lib.org/functions/sum) is a *rolling window* over
 * `optInTimePeriod` bars; `CUMSUM` has no window — every bar since the anchor
 * contributes.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cumsum](https://ta-lib.org/functions/cumsum).
 *
 * **Notes**
 * - Lookback 0: `outBegIdx = startIdx`, one output per input bar. Streaming state
 *   is a single accumulator, so a peek commits nothing by construction.
 * - The sum is uncompensated. A Kahan or Neumaier variant would diverge from
 *   `AD`'s own convention, which this follows.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cumsumLookback} is a success with empty outputs.
 *
 * @param real - Source series (canonically a per-bar net figure, e.g. advances −
 *   declines)
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link CumsumOptions}.
 * @returns `begIdx` and the values; see {@link CumsumResult}. Each output is a
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
 * import { cumsum } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = cumsum(data);
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cumsum(real: ArrayLike<number>, options?: CumsumOptions): CumsumResult {
  const o = optionsOf('CUMSUM', options);
  if (o.endIdx === undefined) {
    requireSeries('CUMSUM', 'real', real);
  }
  const core = coreOf('CUMSUM', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cumsumLookbackPositional(core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = cumsumPositional(startIdx, endIdx, real, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CUMSUM stream: one value per closed bar, bit-identical to {@link cumsum}
 * over the same series. Open it with {@link cumsumOpen} or
 * {@link cumsumOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cumsum](https://ta-lib.org/functions/cumsum).
 */
export class CumsumStream {
  /** @internal */
  core: Core;
  /** @internal */
  total: number = 0;
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
  static of(core: Core): CumsumStream {
    return new CumsumStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cumsum} reports them over the same bars. Each accepted `update` and each
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
      throw failure('CUMSUM advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: Source series (canonically a per-bar
   *   net figure, e.g. advances − declines)
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('CUMSUM update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('CUMSUM update', 'inReal');
    }
    cumsumStepImpl(this, inReal);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Source series (canonically a per-bar
   *   net figure, e.g. advances − declines)
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number): number {
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('CUMSUM peek', 'inReal');
    }
    const sp = this;
    let cur_outReal = 0;
    let total = sp.total;
    total += inReal;
    cur_outReal = total;
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
  clone(): CumsumStream {
    const c = new CumsumStream(this.core);
    c.total = this.total;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cumsumStepImpl(sp: CumsumStream, inReal: number): void {
  sp.total += inReal;
  sp.cur_outReal = sp.total;
}

function cumsumOpenImpl(
  sp: CumsumStream,
  inReal: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let total = 0;
  let i = 0;
  let outIdx = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Running total from the ANCHOR bar forward: the accumulator re-seeds at
   * startIdx, exactly as every shipped path-dependent accumulator does
   * (ad.c anchors at startIdx with ad = 0.0, obv.c seeds at
   * inVolume[startIdx]) -- TA_CUMSUM(3, 7, x)[0] is x[3], NOT sum(x[0..3]).
   * "From bar 0" could only mean "from the start of whatever buffer the
   * caller passed", under which a composed call handed inReal + off silently
   * changes answer; the path_dependent flag exists to declare this class
   * (issue #372). Left-to-right, one double, no compensation -- ad.c's own
   * plain += convention, and what both external oracles compute bit-exactly.
   *
   * Two statements, not outReal[outIdx] = (total += ...): a value-producing
   * compound assignment appears in zero input files, and Rust has no
   * assignment expression, so the one-liner cannot lower to a required
   * backend.
   */
  total = 0.0;
  for (i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1) {
    total += inReal[i];
    outReal[((outIdx * outStride) | 0)] = total;
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  sp.total = total;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cumsumOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): CumsumStream {
  const sp = CumsumStream.of(core);
  const retCode = cumsumOpenImpl(sp, inReal, startIdx, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CUMSUM openAndFill', inReal.length, startIdx, cumsumLookbackPositional(core));
  }
  throw streamFailure('CUMSUM openAndFill', retCode);
}

/** @internal */
export function cumsumOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
): CumsumStream {
  const sp = CumsumStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = cumsumOpenImpl(sp, inReal, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CUMSUM open', inReal.length, startIdx, cumsumLookbackPositional(core));
  }
  throw streamFailure('CUMSUM open', retCode);
}

/** @internal */
export function cumsumOpenPositional(
  inReal: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): CumsumStream {
  requireArgument('CUMSUM open', 'real', inReal);
  requireHistory('CUMSUM open', inReal.length);
  return cumsumOpenInternal(core, float64(inReal), 0);
}

/** @internal */
export function cumsumOpenAndFillPositional(
  inReal: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): CumsumStream {
  requireArgument('CUMSUM openAndFill', 'real', inReal);
  requireHistory('CUMSUM openAndFill', inReal.length);
  const guardOutLen = openFillCount('CUMSUM openAndFill', inReal.length, cumsumLookbackPositional(core));
  requireOutput('CUMSUM openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('CUMSUM openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('CUMSUM openAndFill', 'outReal', 'real');
  }
  return cumsumOpenAndFillInternal(core, inRealF64, 0, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live CUMSUM stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link cumsum} there.
 *
 * @param real - Warm-up history. Source series (canonically a per-bar net figure,
 *   e.g. advances − declines)
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CumsumOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cumsumLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core` or an absent
 *   input.
 *
 * @example
 * ```ts
 * import { cumsumOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = cumsumOpen(data);
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function cumsumOpen(real: ArrayLike<number>, options?: CumsumOptions): CumsumStream {
  const o = optionsOf('CUMSUM open', options);
  const core = coreOf('CUMSUM open', o.core);
  return cumsumOpenPositional(real, core);
}

/**
 * {@link cumsumOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link cumsum}.
 *
 * @param real - Warm-up history. Source series (canonically a per-bar net figure,
 *   e.g. advances − declines)
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CumsumOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cumsum} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cumsumLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input or a buffer too short, of the wrong type or starting where another buffer
 *   or a `Float64Array` input starts, or, on a history long enough to open, one
 *   sharing memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cumsumOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = cumsumOpenAndFill(data);
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cumsumOpenAndFill(real: ArrayLike<number>, options?: CumsumOptions): CumsumResult & { stream: CumsumStream } {
  const o = optionsOf('CUMSUM openAndFill', options);
  const core = coreOf('CUMSUM openAndFill', o.core);
  const capacity = fillCapacity(cumsumLookbackPositional(core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = cumsumOpenAndFillPositional(real, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
