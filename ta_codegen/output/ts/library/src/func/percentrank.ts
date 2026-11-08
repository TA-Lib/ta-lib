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

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function percentrankLookbackPositional(optInTimePeriod: number, _core: Core = Core.DEFAULT): number {
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

/** @internal */
export function percentrankDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (percentrankLookbackPositional(optInTimePeriod, core) < 0) {
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
  lookbackTotal = percentrankLookbackPositional(optInTimePeriod, core);
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

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 10000, 'in 2..10000']] as const;

/** @internal */
export function percentrankPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('PERCENTRANK', 'startIdx', startIdx);
  endIdx = requireInt32('PERCENTRANK', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('PERCENTRANK', 'timePeriod', optInTimePeriod);
  requireIndexRange('PERCENTRANK', startIdx, endIdx);
  const guardLookback = percentrankLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('PERCENTRANK', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('PERCENTRANK', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('PERCENTRANK', 'real', inReal, guardInLen);
  requireOutput('PERCENTRANK', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('PERCENTRANK', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = percentrankImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('PERCENTRANK', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link percentrank}, {@link percentrankLookback},
 * {@link percentrankDisplayShift}, {@link percentrankOpen} and
 * {@link percentrankOpenAndFill}. Every key is optional: an omitted or `undefined`
 * key takes its default, and a call ignores the keys it has no use for.
 */
export interface PercentrankOptions {
  /**
   * Number of preceding values the current value is ranked against (default 100;
   * range 2 to 10000; `Core.INTEGER_DEFAULT` selects the default).
   */
  timePeriod?: number | undefined;
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

/**
 * What {@link percentrank} returns, and {@link percentrankOpenAndFill} with its
 * stream.
 */
export interface PercentrankResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Percentage of the preceding window strictly below the current value, 0 to 100.
   * `values[k]` is the value at input bar `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link percentrank} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link PercentrankOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function percentrankLookback(options?: PercentrankOptions): number {
  const o = optionsOf('PERCENTRANK lookback', options);
  return percentrankLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('PERCENTRANK lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link percentrank}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link PercentrankOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function percentrankDisplayShift(outputIdx: number, options?: PercentrankOptions): number {
  const o = optionsOf('PERCENTRANK displayShift', options);
  return percentrankDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('PERCENTRANK displayShift', o.core));
}

/**
 * Percent Rank: where the current value sits inside the distribution of the values
 * that came before it, as a percentage. Each bar is compared against the
 * `timePeriod` values immediately preceding it — the current bar is not part of
 * its own comparison set — and the output is the share of them it exceeds. 0 means
 * the current value is at or below every one of them, 100 means it is above all of
 * them, 50 means it is above half. It is a distribution-relative reading rather
 * than a price-relative one: unlike an oscillator built from ranges or averages,
 * it says nothing about how far the value moved, only how many of its recent
 * predecessors it overtook. That makes it flat-scaled across instruments and
 * directly comparable between them. It is best known as the third leg of Connors'
 * ConnorsRSI, applied there to one-day returns rather than to price.
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
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link percentrankLookback} is a success with empty outputs.
 *
 * @param real - Source price/value series.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link PercentrankOptions}.
 * @returns `begIdx` and the values; see {@link PercentrankResult}. Each output is
 *   a view of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, an integer parameter that is not an int32, a parameter outside its range,
 *   an input too short for the range, a buffer that is empty, too short or of the
 *   wrong type or a buffer sharing memory with a `Float64Array` input it is not.
 *   Checked before anything is written.
 *
 * @example
 * ```ts
 * import { percentrank } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = percentrank(data, { timePeriod: 100 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`rsi`](https://ta-lib.org/functions/rsi)
 * @see [`willr`](https://ta-lib.org/functions/willr)
 * @see [`stddev`](https://ta-lib.org/functions/stddev)
 */
export function percentrank(real: ArrayLike<number>, options?: PercentrankOptions): PercentrankResult {
  const o = optionsOf('PERCENTRANK', options);
  if (o.endIdx === undefined) {
    requireSeries('PERCENTRANK', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('PERCENTRANK', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, percentrankLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = percentrankPositional(startIdx, endIdx, real, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
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
      throw streamFailure('PERCENTRANK advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: Source price/value series.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('PERCENTRANK update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('PERCENTRANK update', 'real');
    }
    percentrankStepImpl(this, real);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: Source price/value series.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number): number {
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('PERCENTRANK peek', 'real');
    }
    const sp = this;
    const inReal = real;
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
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 100;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 10000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = percentrankLookbackPositional(optInTimePeriod, core);
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
    throw insufficientHistory('PERCENTRANK openAndFill', inReal.length, startIdx, percentrankLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && percentrankLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('PERCENTRANK openAndFill', PARAM_RANGES, optInTimePeriod);
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
    throw insufficientHistory('PERCENTRANK open', inReal.length, startIdx, percentrankLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && percentrankLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('PERCENTRANK open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('PERCENTRANK open', retCode);
}

/** @internal */
export function percentrankOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): PercentrankStream {
  optInTimePeriod = requireInt32('PERCENTRANK open', 'timePeriod', optInTimePeriod);
  requireArgument('PERCENTRANK open', 'real', inReal);
  requireHistory('PERCENTRANK open', 'real', inReal.length);
  return percentrankOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/** @internal */
export function percentrankOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): PercentrankStream {
  optInTimePeriod = requireInt32('PERCENTRANK openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('PERCENTRANK openAndFill', 'real', inReal);
  requireHistory('PERCENTRANK openAndFill', 'real', inReal.length);
  const guardLookback = percentrankLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('PERCENTRANK openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('PERCENTRANK openAndFill', inReal.length, guardLookback);
  requireOutput('PERCENTRANK openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('PERCENTRANK openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('PERCENTRANK openAndFill', 'outReal', 'real');
  }
  return percentrankOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live PERCENTRANK stream over the warm-up history: `value()` starts at
 * the last history bar, bit-identical to {@link percentrank} there.
 *
 * @param real - Warm-up history. Source price/value series.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link PercentrankOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `percentrankLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, an integer parameter that is not an int32 or a
 *   parameter outside its range.
 *
 * @example
 * ```ts
 * import { percentrankOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = percentrankOpen(data, { timePeriod: 100 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function percentrankOpen(real: ArrayLike<number>, options?: PercentrankOptions): PercentrankStream {
  const o = optionsOf('PERCENTRANK open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('PERCENTRANK open', o.core);
  return percentrankOpenPositional(real, optInTimePeriod, core);
}

/**
 * {@link percentrankOpen} that also computes the outputs over the whole history in
 * the same pass, bit-identically to {@link percentrank}.
 *
 * @param real - Warm-up history. Source price/value series.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link PercentrankOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link percentrank} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `percentrankLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, an integer parameter that is not an int32, a parameter
 *   outside its range or a buffer too short, of the wrong type or starting where
 *   another buffer or a `Float64Array` input starts, or, on a history long enough to
 *   open, one sharing memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { percentrankOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = percentrankOpenAndFill(data, {
 *   timePeriod: 100,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function percentrankOpenAndFill(real: ArrayLike<number>, options?: PercentrankOptions): PercentrankResult & { stream: PercentrankStream } {
  const o = optionsOf('PERCENTRANK openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('PERCENTRANK openAndFill', o.core);
  const capacity = fillCapacity(percentrankLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = percentrankOpenAndFillPositional(real, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
