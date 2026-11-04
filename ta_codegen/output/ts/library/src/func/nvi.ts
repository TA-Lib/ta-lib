/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  MF,CC    Mario Fortier, Claude Code
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120802 MF   Template creation.
 *  071726 MF,CC Implement Negative Volume Index (#126).
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link nvi} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function nviLookback(core: Core = Core.DEFAULT): number {
  void core;
  /* This function have no lookback needed. */
  return 0;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link nvi}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function nviDisplayShift(outputIdx: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function nviImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inClose: Float64Array,
  inVolume: Float64Array,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let i = 0;
  let outIdx = 0;
  let prevNVI = 0;
  let prevClose = 0;
  let prevVolume = 0;
  let tempClose = 0;
  let tempVolume = 0;
  let tempNVI = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  /* The index is a running cumulative value seeded at 1000, updated only on
   * bars whose volume decreased versus the prior bar (Negative Volume).
   */
  prevNVI = 1000.0;
  prevClose = inClose[startIdx];
  prevVolume = inVolume[startIdx];
  outIdx = 0;
  for (i = startIdx; i <= endIdx; i += 1) {
    tempClose = inClose[i];
    tempVolume = inVolume[i];
    /* prevClose != 0 guards the percentage-change division: a zero previous
     * close is a degenerate input that would otherwise emit NaN/Inf; carry
     * the index forward unchanged instead. Never triggers on real prices.
     */
    if (tempVolume < prevVolume && prevClose !== 0.0) {
      /* The index is a running product, so it has no upper bound: enough
       * compounding gains push it past the largest double. Keep the last
       * representable value instead of writing +/-Inf, which no caller can
       * chart and which poisons every arithmetic downstream of it. Real
       * price series never come close.
       *
       * Written as a compound assignment on the copy, exactly as the update
       * was before the guard: spelling it `a + r*a` would match the FMA
       * fusion detector and silently re-round every bar, not just the
       * overflowing one.
       */
      tempNVI = prevNVI;
      tempNVI += (tempClose - prevClose) / prevClose * tempNVI;
      if ((Number.isFinite(tempNVI))) {
        prevNVI = tempNVI;
      }
    }
    outReal[outIdx++] = prevNVI;
    prevClose = tempClose;
    prevVolume = tempVolume;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * Negative Volume Index: a running cumulative index that changes only on days when
 * volume falls versus the prior day, compounding that day's percentage price
 * change. The premise is that quiet, low-volume days reflect the actions of
 * well-informed "smart money", so NVI is read as a proxy for that cohort's
 * positioning.
 *
 * Formula and more info at
 * [ta-lib.org/functions/nvi](https://ta-lib.org/functions/nvi).
 *
 * **Notes**
 * - **The input is assumed positive.** The previous close is the divisor. Near
 *   zero the divisor is small and the output very large; no error is returned. A
 *   series that reaches or crosses zero, such as an oscillator or a spread, has no
 *   meaningful result here.
 * - The index compounds, so it has no upper bound. If a run of large rises ever
 *   pushes it past the largest representable number, the last representable value
 *   is carried forward instead of returning infinity. Real price series stay far
 *   away from that.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link nviLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inClose - Close price of each bar.
 * @param inVolume - Volume of each bar.
 * @param outReal - Cumulative negative volume index (seeded at 1000) Must hold the
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
 * import { nvi } from 'ta-lib';
 *
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const out = new Float64Array(close.length);
 * const { begIdx, count } = nvi(0, close.length - 1, close, volume, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function nvi(
  startIdx: number,
  endIdx: number,
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('NVI', 'startIdx', startIdx);
  endIdx = requireInt32('NVI', 'endIdx', endIdx);
  requireIndexRange('NVI', startIdx, endIdx);
  const guardStart = clampedStart('NVI', startIdx, nviLookback(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('NVI', 'inClose', inClose, guardInLen);
  requireLength('NVI', 'inVolume', inVolume, guardInLen);
  requireOutput('NVI', 'outReal', outReal, guardOutLen, Float64Array);
  const inCloseF64 = float64(inClose);
  const inVolumeF64 = float64(inVolume);
  if (overlapsPartly(outReal, inCloseF64) || overlapsPartly(outReal, inVolumeF64)) {
    throw failure('NVI', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = nviImpl(core, startIdx, endIdx, inCloseF64, inVolumeF64, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('NVI', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live NVI stream: one value per closed bar, bit-identical to {@link nvi} over
 * the same series. Open it with {@link nviOpen} or {@link nviOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/nvi](https://ta-lib.org/functions/nvi).
 */
export class NviStream {
  /** @internal */
  core: Core;
  /** @internal */
  prevNVI: number = 0;
  /** @internal */
  prevClose: number = 0;
  /** @internal */
  prevVolume: number = 0;
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
  static of(core: Core): NviStream {
    return new NviStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link nvi} reports them over the same bars. Each accepted `update` and each
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
      throw failure('NVI advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param inVolume - The bar's `inVolume` value: Volume of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inClose: number, inVolume: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('NVI update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inClose) || !Number.isFinite(inVolume)) {
      throw nonFiniteBar('NVI update', !Number.isFinite(inClose) ? 'inClose' : 'inVolume');
    }
    nviStepImpl(this, inClose, inVolume);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param inVolume - The bar's `inVolume` value: Volume of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inClose: number, inVolume: number): number {
    if (!Number.isFinite(inClose) || !Number.isFinite(inVolume)) {
      throw nonFiniteBar('NVI peek', !Number.isFinite(inClose) ? 'inClose' : 'inVolume');
    }
    const sp = this;
    let tempClose = 0;
    let tempVolume = 0;
    let tempNVI = 0;
    let cur_outReal = 0;
    let prevNVI = sp.prevNVI;
    tempClose = inClose;
    tempVolume = inVolume;
    /* prevClose != 0 guards the percentage-change division: a zero previous
     * close is a degenerate input that would otherwise emit NaN/Inf; carry
     * the index forward unchanged instead. Never triggers on real prices.
     */
    if (tempVolume < sp.prevVolume && sp.prevClose !== 0.0) {
      /* The index is a running product, so it has no upper bound: enough
       * compounding gains push it past the largest double. Keep the last
       * representable value instead of writing +/-Inf, which no caller can
       * chart and which poisons every arithmetic downstream of it. Real
       * price series never come close.
       *
       * Written as a compound assignment on the copy, exactly as the update
       * was before the guard: spelling it `a + r*a` would match the FMA
       * fusion detector and silently re-round every bar, not just the
       * overflowing one.
       */
      tempNVI = prevNVI;
      tempNVI += (tempClose - sp.prevClose) / sp.prevClose * tempNVI;
      if ((Number.isFinite(tempNVI))) {
        prevNVI = tempNVI;
      }
    }
    cur_outReal = prevNVI;
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
  clone(): NviStream {
    const c = new NviStream(this.core);
    c.prevNVI = this.prevNVI;
    c.prevClose = this.prevClose;
    c.prevVolume = this.prevVolume;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function nviStepImpl(sp: NviStream, inClose: number, inVolume: number): void {
  let tempClose = 0;
  let tempVolume = 0;
  let tempNVI = 0;
  tempClose = inClose;
  tempVolume = inVolume;
  /* prevClose != 0 guards the percentage-change division: a zero previous
   * close is a degenerate input that would otherwise emit NaN/Inf; carry
   * the index forward unchanged instead. Never triggers on real prices.
   */
  if (tempVolume < sp.prevVolume && sp.prevClose !== 0.0) {
    /* The index is a running product, so it has no upper bound: enough
     * compounding gains push it past the largest double. Keep the last
     * representable value instead of writing +/-Inf, which no caller can
     * chart and which poisons every arithmetic downstream of it. Real
     * price series never come close.
     *
     * Written as a compound assignment on the copy, exactly as the update
     * was before the guard: spelling it `a + r*a` would match the FMA
     * fusion detector and silently re-round every bar, not just the
     * overflowing one.
     */
    tempNVI = sp.prevNVI;
    tempNVI += (tempClose - sp.prevClose) / sp.prevClose * tempNVI;
    if ((Number.isFinite(tempNVI))) {
      sp.prevNVI = tempNVI;
    }
  }
  sp.cur_outReal = sp.prevNVI;
  sp.prevClose = tempClose;
  sp.prevVolume = tempVolume;
}

function nviOpenImpl(
  sp: NviStream,
  inClose: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let i = 0;
  let outIdx = 0;
  let prevNVI = 0;
  let prevClose = 0;
  let prevVolume = 0;
  let tempClose = 0;
  let tempVolume = 0;
  let tempNVI = 0;
  const historyLen = inClose.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inVolume.length !== inClose.length) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* The index is a running cumulative value seeded at 1000, updated only on
   * bars whose volume decreased versus the prior bar (Negative Volume).
   */
  prevNVI = 1000.0;
  prevClose = inClose[startIdx];
  prevVolume = inVolume[startIdx];
  outIdx = 0;
  for (i = startIdx; i <= endIdx; i += 1) {
    tempClose = inClose[i];
    tempVolume = inVolume[i];
    /* prevClose != 0 guards the percentage-change division: a zero previous
     * close is a degenerate input that would otherwise emit NaN/Inf; carry
     * the index forward unchanged instead. Never triggers on real prices.
     */
    if (tempVolume < prevVolume && prevClose !== 0.0) {
      /* The index is a running product, so it has no upper bound: enough
       * compounding gains push it past the largest double. Keep the last
       * representable value instead of writing +/-Inf, which no caller can
       * chart and which poisons every arithmetic downstream of it. Real
       * price series never come close.
       *
       * Written as a compound assignment on the copy, exactly as the update
       * was before the guard: spelling it `a + r*a` would match the FMA
       * fusion detector and silently re-round every bar, not just the
       * overflowing one.
       */
      tempNVI = prevNVI;
      tempNVI += (tempClose - prevClose) / prevClose * tempNVI;
      if ((Number.isFinite(tempNVI))) {
        prevNVI = tempNVI;
      }
    }
    outReal[((outIdx++ * outStride) | 0)] = prevNVI;
    prevClose = tempClose;
    prevVolume = tempVolume;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  sp.prevNVI = prevNVI;
  sp.prevClose = prevClose;
  sp.prevVolume = prevVolume;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function nviOpenAndFillInternal(
  core: Core,
  inClose: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): NviStream {
  const sp = NviStream.of(core);
  const retCode = nviOpenImpl(sp, inClose, inVolume, startIdx, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('NVI openAndFill', inClose.length, startIdx, nviLookback(core));
  }
  throw streamFailure('NVI openAndFill', retCode);
}

/** @internal */
export function nviOpenInternal(
  core: Core,
  inClose: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
): NviStream {
  const sp = NviStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = nviOpenImpl(sp, inClose, inVolume, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('NVI open', inClose.length, startIdx, nviLookback(core));
  }
  throw streamFailure('NVI open', retCode);
}

/**
 * Opens a live NVI stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link nvi} there.
 *
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param inVolume - Warm-up history. Volume of each bar. Every history has the
 *   same length.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `nviLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   histories of different lengths.
 *
 * @example
 * ```ts
 * import { nviOpen } from 'ta-lib';
 *
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const stream = nviOpen(close, volume);
 * const peeked = stream.peek(100.9, 12345); // a forming bar: commits nothing
 * const updated = stream.update(100.9, 12345); // the bar closes
 * ```
 */
export function nviOpen(
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): NviStream {
  requireArgument('NVI open', 'inClose', inClose);
  requireHistory('NVI open', inClose.length);
  requireArgument('NVI open', 'inVolume', inVolume);
  requireHistoryLength('NVI open', 'inVolume', inVolume.length, inClose.length);
  return nviOpenInternal(core, float64(inClose), float64(inVolume), 0);
}

/**
 * {@link nviOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link nvi}. The range written is the stream's
 * `outRange()`.
 *
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param inVolume - Warm-up history. Volume of each bar. Every history has the
 *   same length.
 * @param outReal - Cumulative negative volume index (seeded at 1000) Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `nviLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   histories of different lengths or an output too short or starting where another
 *   output or a `Float64Array` input starts, or, on a history long enough to open,
 *   one sharing memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { nviOpenAndFill } from 'ta-lib';
 *
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const out = new Float64Array(close.length);
 * const stream = nviOpenAndFill(close, volume, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function nviOpenAndFill(
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): NviStream {
  requireArgument('NVI openAndFill', 'inClose', inClose);
  requireHistory('NVI openAndFill', inClose.length);
  requireArgument('NVI openAndFill', 'inVolume', inVolume);
  const guardOutLen = openFillCount('NVI openAndFill', inClose.length, nviLookback(core));
  requireHistoryLength('NVI openAndFill', 'inVolume', inVolume.length, inClose.length);
  requireOutput('NVI openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inCloseF64 = float64(inClose);
  const inVolumeF64 = float64(inVolume);
  if ((outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset) || (outReal.buffer === inVolumeF64.buffer && outReal.byteOffset === inVolumeF64.byteOffset)) {
    throw streamFailure('NVI openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inCloseF64) || overlaps(outReal, inVolumeF64))) {
    throw streamFailure('NVI openAndFill', RetCode.BAD_PARAM);
  }
  return nviOpenAndFillInternal(core, inCloseF64, inVolumeF64, 0, new MInteger(), new MInteger(), outReal);
}
