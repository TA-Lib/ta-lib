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
 *  090426 MF,CC  Initial version (#369).
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link percentrank} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param optInTimePeriod - Number of preceding values the current value is ranked
 *   against (default 100; range 2 to 10000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function percentrankLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 100;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 10000) {
    return -1;
  }
  return optInTimePeriod;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link percentrank}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Number of preceding values the current value is ranked
 *   against (default 100; range 2 to 10000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function percentrankDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (percentrankLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function percentrankImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let today = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let i = 0;
  let count = 0;
  let current = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 100;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 10000) {
    return RetCode.BAD_PARAM;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = percentrankLookback(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  outIdx = 0;
  today = startIdx;
  while (today <= endIdx) {
    current = inReal[today];
    count = 0;
    for (i = optInTimePeriod; i >= 1; i -= 1) {
      count += +(inReal[today - i] < current);
    }
    /* Divide, then scale. (count/N)*100 and 100*count/N are different
     * doubles and the reference implementations round the first way.
     */
    outReal[outIdx] = count / optInTimePeriod * 100.0;
    outIdx += 1;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * Percent Rank: where the current value sits inside the distribution of the values
 * that came before it, as a percentage. Each bar is compared against the
 * `optInTimePeriod` values immediately preceding it — the current bar is not part
 * of its own comparison set — and the output is the share of them it exceeds. 0
 * means the current value is at or below every one of them, 100 means it is above
 * all of them, 50 means it is above half. It is a distribution-relative reading
 * rather than a price-relative one: unlike an oscillator built from ranges or
 * averages, it says nothing about how far the value moved, only how many of its
 * recent predecessors it overtook. That makes it flat-scaled across instruments
 * and directly comparable between them. It is best known as the third leg of
 * Connors' ConnorsRSI, applied there to one-day returns rather than to price.
 *
 * Formula and more info at
 * [ta-lib.org/functions/percentrank](https://ta-lib.org/functions/percentrank).
 *
 * **Notes**
 * - Ties are strict: a predecessor equal to the current value is not counted. A
 *   constant series therefore reports 0 on every bar. TradingView's
 *   `ta.percentrank` counts ties as well (less-than-or-equal), which reports 100
 *   on that same series; Pine parity is a different function, not a variant of
 *   this one, and the two agree only on windows with no repeated values.
 * - +0.0 and -0.0 compare equal, so a sign-only difference never contributes to
 *   the count.
 * - Finite input is a precondition. A NaN in the window fails every comparison, so
 *   it silently lowers the rank instead of propagating: the output stays finite
 *   and no error is reported.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link percentrankLookback} is a success
 * with `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - Source price/value series.
 * @param optInTimePeriod - Number of preceding values the current value is ranked
 *   against (default 100; range 2 to 10000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param outReal - Percentage of the preceding window strictly below the current
 *   value, 0 to 100. Must hold the values produced and never be empty; the input
 *   length always suffices.
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
 * import { percentrank } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const { begIdx, count } = percentrank(0, data.length - 1, data, 100, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`rsi`](https://ta-lib.org/functions/rsi)
 * @see [`willr`](https://ta-lib.org/functions/willr)
 * @see [`stddev`](https://ta-lib.org/functions/stddev)
 */
export function percentrank(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('PERCENTRANK', 'startIdx', startIdx);
  endIdx = requireInt32('PERCENTRANK', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('PERCENTRANK', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('PERCENTRANK', startIdx, endIdx);
  const guardStart = clampedStart('PERCENTRANK', startIdx, percentrankLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('PERCENTRANK', 'inReal', inReal, guardInLen);
  requireOutput('PERCENTRANK', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw failure('PERCENTRANK', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = percentrankImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('PERCENTRANK', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live PERCENTRANK stream: one value per closed bar, bit-identical to
 * {@link percentrank} over the same series. Open it with {@link percentrankOpen}
 * or {@link percentrankOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/percentrank](https://ta-lib.org/functions/percentrank).
 */
export class PercentrankStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  winPos_i: number = 0;
  /** @internal */
  winCap_i: number = 1;
  /** @internal */
  win_i_inReal: Float64Array = new Float64Array(1);
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
  static of(core: Core): PercentrankStream {
    return new PercentrankStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link percentrank} reports them over the same bars. Each accepted `update` and
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
      throw failure('PERCENTRANK advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: Source price/value series.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('PERCENTRANK update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('PERCENTRANK update', 'inReal');
    }
    percentrankStepImpl(this, inReal);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Source price/value series.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number): number {
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('PERCENTRANK peek', 'inReal');
    }
    const sp = this;
    let i = 0;
    let count = 0;
    let current = 0;
    let cur_outReal = 0;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    pkSlot0 = sp.winPos_i;
    pkVal0 = inReal;
    current = inReal;
    count = 0;
    for (i = sp.optInTimePeriod; i >= 1; i -= 1) {
      count += +(((((sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i) !== pkSlot0) ? sp.win_i_inReal[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i] : pkVal0) < current);
    }
    /* Divide, then scale. (count/N)*100 and 100*count/N are different
     * doubles and the reference implementations round the first way.
     */
    cur_outReal = count / sp.optInTimePeriod * 100.0;
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
  clone(): PercentrankStream {
    const c = new PercentrankStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.winPos_i = this.winPos_i;
    c.winCap_i = this.winCap_i;
    c.win_i_inReal = this.win_i_inReal.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function percentrankStepImpl(sp: PercentrankStream, inReal: number): void {
  let i = 0;
  let count = 0;
  let current = 0;
  sp.win_i_inReal[sp.winPos_i] = inReal;
  current = inReal;
  count = 0;
  for (i = sp.optInTimePeriod; i >= 1; i -= 1) {
    count += +(sp.win_i_inReal[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i] < current);
  }
  /* Divide, then scale. (count/N)*100 and 100*count/N are different
   * doubles and the reference implementations round the first way.
   */
  sp.cur_outReal = count / sp.optInTimePeriod * 100.0;
  sp.winPos_i = sp.winPos_i + 1;
  if (sp.winPos_i >= sp.winCap_i) {
    sp.winPos_i = 0;
  }
}

function percentrankOpenImpl(
  sp: PercentrankStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let today = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let i = 0;
  let count = 0;
  let current = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 100;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 10000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = percentrankLookback(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outIdx = 0;
  today = startIdx;
  while (today <= endIdx) {
    current = inReal[today];
    count = 0;
    for (i = optInTimePeriod; i >= 1; i -= 1) {
      count += +(inReal[today - i] < current);
    }
    /* Divide, then scale. (count/N)*100 and 100*count/N are different
     * doubles and the reference implementations round the first way.
     */
    outReal[((outIdx * outStride) | 0)] = count / optInTimePeriod * 100.0;
    outIdx += 1;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  const cap_i = optInTimePeriod + 1;
  if (cap_i < 1 || cap_i > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const capWin_i_inReal = inReal.slice(historyLen - cap_i, historyLen);
  sp.optInTimePeriod = optInTimePeriod;
  sp.winPos_i = 0;
  sp.winCap_i = cap_i;
  sp.win_i_inReal = capWin_i_inReal;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function percentrankOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): PercentrankStream {
  const sp = PercentrankStream.of(core);
  const retCode = percentrankOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('PERCENTRANK openAndFill', inReal.length, startIdx, percentrankLookback(optInTimePeriod, core));
  }
  throw streamFailure('PERCENTRANK openAndFill', retCode);
}

/** @internal */
export function percentrankOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): PercentrankStream {
  const sp = PercentrankStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = percentrankOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('PERCENTRANK open', inReal.length, startIdx, percentrankLookback(optInTimePeriod, core));
  }
  throw streamFailure('PERCENTRANK open', retCode);
}

/**
 * Opens a live PERCENTRANK stream over the warm-up history: `value()` starts at
 * the last history bar, bit-identical to {@link percentrank} there.
 *
 * @param inReal - Warm-up history. Source price/value series.
 * @param optInTimePeriod - Number of preceding values the current value is ranked
 *   against (default 100; range 2 to 10000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `percentrankLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an
 *   empty one, and `TA_BAD_PARAM` for an integer parameter that is not an int32 or a
 *   parameter outside its range.
 *
 * @example
 * ```ts
 * import { percentrankOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = percentrankOpen(data, 100);
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function percentrankOpen(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): PercentrankStream {
  optInTimePeriod = requireInt32('PERCENTRANK open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('PERCENTRANK open', 'inReal', inReal);
  requireHistory('PERCENTRANK open', inReal.length);
  return percentrankOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/**
 * {@link percentrankOpen} that also fills the outputs over the whole history in
 * the same pass, bit-identically to {@link percentrank}. The range written is the
 * stream's `outRange()`.
 *
 * @param inReal - Warm-up history. Source price/value series.
 * @param optInTimePeriod - Number of preceding values the current value is ranked
 *   against (default 100; range 2 to 10000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param outReal - Percentage of the preceding window strictly below the current
 *   value, 0 to 100. Must hold `historyLen - lookback` values and share no memory
 *   with an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `percentrankLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an
 *   empty one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or an output too short or starting where another
 *   output or a `Float64Array` input starts, or, on a history long enough to open,
 *   one sharing memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { percentrankOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const stream = percentrankOpenAndFill(data, 100, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function percentrankOpenAndFill(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): PercentrankStream {
  optInTimePeriod = requireInt32('PERCENTRANK openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('PERCENTRANK openAndFill', 'inReal', inReal);
  requireHistory('PERCENTRANK openAndFill', inReal.length);
  const guardOutLen = openFillCount('PERCENTRANK openAndFill', inReal.length, percentrankLookback(optInTimePeriod, core));
  requireOutput('PERCENTRANK openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if ((outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset)) {
    throw streamFailure('PERCENTRANK openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inRealF64))) {
    throw streamFailure('PERCENTRANK openAndFill', RetCode.BAD_PARAM);
  }
  return percentrankOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}
