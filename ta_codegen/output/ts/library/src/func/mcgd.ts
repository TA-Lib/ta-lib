/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  092926 MF,CC  First version (issue #471).
 */

import { FuncUnstId } from '../enums.ts';
import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link mcgd} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param optInTimePeriod - The N of the step's denominator (default 14; range 2 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function mcgdLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  return optInTimePeriod - 1 + core.unstableCount(FuncUnstId.MCGD, ((((5 * 4) | 0) * optInTimePeriod) | 0), ((((5 * 8) | 0) * optInTimePeriod) | 0));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link mcgd}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - The N of the step's denominator (default 14; range 2 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function mcgdDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (mcgdLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function mcgdImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let i = 0;
  let outIdx = 0;
  let today = 0;
  let lookbackTotal = 0;
  let nbMCGD = 0;
  let prevMD = 0;
  let tempMD = 0;
  let tempReal = 0;
  let ratio = 0;
  let period = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = mcgdLookback(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  period = optInTimePeriod;
  /* Keep the denominator free of pow(): a transcendental would break the
   * four languages' bit-identity.
   *
   * A step whose value is not finite (a zero price, a zero line meeting a
   * zero price, or a price so small against the line that the step
   * overflows) holds the previous value, since every later bar reads it.
   */
  today = startIdx - lookbackTotal;
  prevMD = inReal[today];
  today += 1;
  i = lookbackTotal;
  while (i !== 0) {
    tempReal = inReal[today];
    ratio = tempReal / prevMD;
    tempMD = prevMD + (tempReal - prevMD) / (period * (ratio * ratio * (ratio * ratio)));
    if ((Number.isFinite(tempMD))) {
      prevMD = tempMD;
    }
    today += 1;
    i -= 1;
  }
  outIdx = 1;
  outReal[0] = prevMD;
  nbMCGD = endIdx - startIdx + 1;
  while (--nbMCGD !== 0) {
    tempReal = inReal[today];
    ratio = tempReal / prevMD;
    tempMD = prevMD + (tempReal - prevMD) / (period * (ratio * ratio * (ratio * ratio)));
    if ((Number.isFinite(tempMD))) {
      prevMD = tempMD;
    }
    outReal[outIdx++] = prevMD;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * McGinley Dynamic (John R. McGinley, Jr.): a moving average whose speed adjusts
 * to the market. Each bar closes a fraction `1 / (N * (x/MD)^4)` of the gap to the
 * price: less than `RMA`'s `1/N` while the price is above the line, more while it
 * is below, so the line tracks falling prices faster than rising ones. McGinley
 * suggests a period of about 60% of the simple moving average being emulated: a
 * Dynamic of 12 to follow a 20-bar average.
 *
 * Formula and more info at
 * [ta-lib.org/functions/mcgd](https://ta-lib.org/functions/mcgd).
 *
 * **Notes**
 * - Where the price is 0, or so small against the line that the step overflows,
 *   the step is undefined and the line keeps its previous value. A line at 0 stays
 *   at 0.
 * - Being recursive, an output depends on how much history precedes it. Close to
 *   the price the seed's influence decays by a factor of `1 - 1/N` per bar, as in
 *   `RMA`; the unstable period is how much of the warm-up to discard.
 * - The line is scale-equivariant but meant for positive prices: a series crossing
 *   zero sends it off to meaningless values, and a single bar far enough below the
 *   line can take the line to 0 or below it.
 * - Some implementations seed with the simple average of the first N bars. They
 *   agree with this one only once the seed's influence has decayed.
 * - Some implementations write the step's denominator as `0.6 * P * (x / MD)^4`.
 *   That is this function at a period of `0.6 * P`, when that is an integer.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link mcgdLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - Data on which to compute the average.
 * @param optInTimePeriod - The N of the step's denominator (default 14; range 2 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - McGinley Dynamic line. Must hold the values produced and never
 *   be empty; the input length always suffices.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The range written: `begIdx` is the first bar with a value, `count` how
 *   many were written.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for an argument that is not an int32, a
 *   parameter outside its range, an array too short for the range, an empty output
 *   or an output sharing memory with a `Float64Array` input it is not. Checked
 *   before anything is written.
 *
 * @example
 * ```ts
 * import { mcgd } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const { begIdx, count } = mcgd(0, data.length - 1, data, 14, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`rma`](https://ta-lib.org/functions/rma)
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`kama`](https://ta-lib.org/functions/kama)
 * @see [`sma`](https://ta-lib.org/functions/sma)
 */
export function mcgd(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('MCGD', 'startIdx', startIdx);
  endIdx = requireInt32('MCGD', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('MCGD', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('MCGD', startIdx, endIdx);
  const guardStart = clampedStart('MCGD', startIdx, mcgdLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('MCGD', 'inReal', inReal, guardInLen);
  requireOutput('MCGD', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw failure('MCGD', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = mcgdImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('MCGD', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live MCGD stream: one value per closed bar, bit-identical to {@link mcgd} over
 * the same series. Open it with {@link mcgdOpen} or {@link mcgdOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/mcgd](https://ta-lib.org/functions/mcgd).
 */
export class McgdStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  prevMD: number = 0;
  /** @internal */
  period: number = 0;
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
  static of(core: Core): McgdStream {
    return new McgdStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link mcgd} reports them over the same bars. Each accepted `update` and each
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
      throw failure('MCGD advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: Data on which to compute the average.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('MCGD update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('MCGD update', 'inReal');
    }
    mcgdStepImpl(this, inReal);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Data on which to compute the average.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number): number {
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('MCGD peek', 'inReal');
    }
    const sp = this;
    let tempMD = 0;
    let tempReal = 0;
    let ratio = 0;
    let cur_outReal = 0;
    let prevMD = sp.prevMD;
    tempReal = inReal;
    ratio = tempReal / prevMD;
    tempMD = prevMD + (tempReal - prevMD) / (sp.period * (ratio * ratio * (ratio * ratio)));
    if ((Number.isFinite(tempMD))) {
      prevMD = tempMD;
    }
    cur_outReal = prevMD;
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
  clone(): McgdStream {
    const c = new McgdStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.prevMD = this.prevMD;
    c.period = this.period;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function mcgdStepImpl(sp: McgdStream, inReal: number): void {
  let tempMD = 0;
  let tempReal = 0;
  let ratio = 0;
  tempReal = inReal;
  ratio = tempReal / sp.prevMD;
  tempMD = sp.prevMD + (tempReal - sp.prevMD) / (sp.period * (ratio * ratio * (ratio * ratio)));
  if ((Number.isFinite(tempMD))) {
    sp.prevMD = tempMD;
  }
  sp.cur_outReal = sp.prevMD;
}

function mcgdOpenImpl(
  sp: McgdStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let i = 0;
  let outIdx = 0;
  let today = 0;
  let lookbackTotal = 0;
  let nbMCGD = 0;
  let prevMD = 0;
  let tempMD = 0;
  let tempReal = 0;
  let ratio = 0;
  let period = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = mcgdLookback(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  period = optInTimePeriod;
  /* Keep the denominator free of pow(): a transcendental would break the
   * four languages' bit-identity.
   *
   * A step whose value is not finite (a zero price, a zero line meeting a
   * zero price, or a price so small against the line that the step
   * overflows) holds the previous value, since every later bar reads it.
   */
  today = startIdx - lookbackTotal;
  prevMD = inReal[today];
  today += 1;
  i = lookbackTotal;
  while (i !== 0) {
    tempReal = inReal[today];
    ratio = tempReal / prevMD;
    tempMD = prevMD + (tempReal - prevMD) / (period * (ratio * ratio * (ratio * ratio)));
    if ((Number.isFinite(tempMD))) {
      prevMD = tempMD;
    }
    today += 1;
    i -= 1;
  }
  outIdx = 1;
  outReal[((0 * outStride) | 0)] = prevMD;
  nbMCGD = endIdx - startIdx + 1;
  while (--nbMCGD !== 0) {
    tempReal = inReal[today];
    ratio = tempReal / prevMD;
    tempMD = prevMD + (tempReal - prevMD) / (period * (ratio * ratio * (ratio * ratio)));
    if ((Number.isFinite(tempMD))) {
      prevMD = tempMD;
    }
    outReal[((outIdx++ * outStride) | 0)] = prevMD;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  sp.optInTimePeriod = optInTimePeriod;
  sp.prevMD = prevMD;
  sp.period = period;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function mcgdOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): McgdStream {
  const sp = McgdStream.of(core);
  const retCode = mcgdOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MCGD openAndFill', inReal.length, startIdx, mcgdLookback(optInTimePeriod, core));
  }
  throw streamFailure('MCGD openAndFill', retCode);
}

/** @internal */
export function mcgdOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): McgdStream {
  const sp = McgdStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = mcgdOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MCGD open', inReal.length, startIdx, mcgdLookback(optInTimePeriod, core));
  }
  throw streamFailure('MCGD open', retCode);
}

/**
 * Opens a live MCGD stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link mcgd} there.
 *
 * @param inReal - Warm-up history. Data on which to compute the average.
 * @param optInTimePeriod - The N of the step's denominator (default 14; range 2 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `mcgdLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { mcgdOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = mcgdOpen(data, 14);
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function mcgdOpen(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): McgdStream {
  optInTimePeriod = requireInt32('MCGD open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('MCGD open', 'inReal', inReal);
  requireHistory('MCGD open', inReal.length);
  return mcgdOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/**
 * {@link mcgdOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link mcgd}. The range written is the stream's
 * `outRange()`.
 *
 * @param inReal - Warm-up history. Data on which to compute the average.
 * @param optInTimePeriod - The N of the step's denominator (default 14; range 2 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - McGinley Dynamic line. Must hold `historyLen - lookback` values
 *   and share no memory with an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `mcgdLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   an integer parameter that is not an int32, a parameter outside its range or an
 *   output too short or starting where another output or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { mcgdOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const stream = mcgdOpenAndFill(data, 14, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function mcgdOpenAndFill(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): McgdStream {
  optInTimePeriod = requireInt32('MCGD openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('MCGD openAndFill', 'inReal', inReal);
  requireHistory('MCGD openAndFill', inReal.length);
  const guardOutLen = openFillCount('MCGD openAndFill', inReal.length, mcgdLookback(optInTimePeriod, core));
  requireOutput('MCGD openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if ((outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset)) {
    throw streamFailure('MCGD openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inRealF64))) {
    throw streamFailure('MCGD openAndFill', RetCode.BAD_PARAM);
  }
  return mcgdOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}
