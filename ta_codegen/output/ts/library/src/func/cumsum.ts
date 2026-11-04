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

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link cumsum} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function cumsumLookback(core: Core = Core.DEFAULT): number {
  void core;
  return 0;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cumsum}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function cumsumDisplayShift(outputIdx: number, core: Core = Core.DEFAULT): number {
  void core;
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
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link cumsumLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - Source series (canonically a per-bar net figure, e.g. advances −
 *   declines)
 * @param outReal - Running total since the anchor bar. Must hold the values
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
 * import { cumsum } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const { begIdx, count } = cumsum(0, data.length - 1, data, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function cumsum(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CUMSUM', 'startIdx', startIdx);
  endIdx = requireInt32('CUMSUM', 'endIdx', endIdx);
  requireIndexRange('CUMSUM', startIdx, endIdx);
  const guardStart = clampedStart('CUMSUM', startIdx, cumsumLookback(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CUMSUM', 'inReal', inReal, guardInLen);
  requireOutput('CUMSUM', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw failure('CUMSUM', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cumsumImpl(core, startIdx, endIdx, inRealF64, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CUMSUM', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
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
    return RetCode.OUT_OF_RANGE_START_INDEX;
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
    throw insufficientHistory('CUMSUM openAndFill', inReal.length, startIdx, cumsumLookback(core));
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
    throw insufficientHistory('CUMSUM open', inReal.length, startIdx, cumsumLookback(core));
  }
  throw streamFailure('CUMSUM open', retCode);
}

/**
 * Opens a live CUMSUM stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link cumsum} there.
 *
 * @param inReal - Warm-up history. Source series (canonically a per-bar net
 *   figure, e.g. advances − declines)
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cumsumLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one.
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
export function cumsumOpen(
  inReal: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): CumsumStream {
  requireArgument('CUMSUM open', 'inReal', inReal);
  requireHistory('CUMSUM open', inReal.length);
  return cumsumOpenInternal(core, float64(inReal), 0);
}

/**
 * {@link cumsumOpen} that also fills the outputs over the whole history in the
 * same pass, bit-identically to {@link cumsum}. The range written is the stream's
 * `outRange()`.
 *
 * @param inReal - Warm-up history. Source series (canonically a per-bar net
 *   figure, e.g. advances − declines)
 * @param outReal - Running total since the anchor bar. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cumsumLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an output too short or starting where another output
 *   or a `Float64Array` input starts, or, on a history long enough to open, one
 *   sharing memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cumsumOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const stream = cumsumOpenAndFill(data, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function cumsumOpenAndFill(
  inReal: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): CumsumStream {
  requireArgument('CUMSUM openAndFill', 'inReal', inReal);
  requireHistory('CUMSUM openAndFill', inReal.length);
  const guardOutLen = openFillCount('CUMSUM openAndFill', inReal.length, cumsumLookback(core));
  requireOutput('CUMSUM openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if ((outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset)) {
    throw streamFailure('CUMSUM openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inRealF64))) {
    throw streamFailure('CUMSUM openAndFill', RetCode.BAD_PARAM);
  }
  return cumsumOpenAndFillInternal(core, inRealF64, 0, new MInteger(), new MInteger(), outReal);
}
