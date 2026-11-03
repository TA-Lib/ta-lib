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
 *  091126 KL     First version (proposal-drafts issue #68).
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { RviStream, rvi, rviLookback, rviOpenAndFillInternal } from './rvi.ts';

/**
 * Number of leading input bars {@link rvir} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Wilder smoothing period applied to both legs of both
 *   indices (default 14; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param optInStdDevPeriod - Number of trailing values each standard deviation
 *   spans (default 10; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function rvirLookback(optInTimePeriod: number, optInStdDevPeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if ((optInStdDevPeriod | 0) !== optInStdDevPeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  if (optInStdDevPeriod === Core.INTEGER_DEFAULT) {
    optInStdDevPeriod = 10;
  } else if (optInStdDevPeriod < 2 || optInStdDevPeriod > 100000) {
    return -1;
  }
  /* Both legs are the shipped TA_RVI at the same parameters, so they warm on
   * the same bar and there is no max() to take. Stated as the callee's
   * lookback rather than restating the arithmetic, which is what makes this
   * function inherit TA_FUNC_UNST_RVI the way KC inherits its two (kc.c).
   */
  return rviLookback(optInTimePeriod, optInStdDevPeriod, core);
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link rvir}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Wilder smoothing period applied to both legs of both
 *   indices (default 14; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param optInStdDevPeriod - Number of trailing values each standard deviation
 *   spans (default 10; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function rvirDisplayShift(optInTimePeriod: number, optInStdDevPeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (rvirLookback(optInTimePeriod, optInStdDevPeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function rvirImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  optInTimePeriod: number,
  optInStdDevPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let tempHigh!: Float64Array;
  let i = 0;
  let lookbackTotal = 0;
  const tempBegIdx = new MInteger();
  const tempNbElement = new MInteger();
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInStdDevPeriod === Core.INTEGER_DEFAULT) {
    optInStdDevPeriod = 10;
  } else if (optInStdDevPeriod < 2 || optInStdDevPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = rvirLookback(optInTimePeriod, optInStdDevPeriod, core);
  /* Nothing is allocated and no input is read when the range cannot produce a
   * value, so a caller-supplied input that stops short of endIdx is never
   * read past its end (kc.c).
   */
  if (lookbackTotal > endIdx) {
    return RetCode.SUCCESS;
  }
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  /* Each leg is the shipped TA_RVI over this range, entered at the same
   * startIdx: same function, same parameters, same lookback, so the two
   * outputs are aligned bar for bar with no anchor to reconcile.
   *
   * Two calls rather than one fused loop. The fused body is expressible --
   * two copies of TA_RVI's state driven by one bar cursor -- but it carries
   * two window-start cursors, and the streaming analyzer accepts exactly one
   * ("extrema automaton: expected exactly one window-start variable"), so
   * fusing would cost this function the streaming tier. Composing keeps it,
   * for the reason KC's legs do: each leg streams as itself.
   */
  tempHigh = new Float64Array(endIdx - startIdx + 1);
  /* Either input may be aliased onto outReal. Both legs are safe against that
   * for TA_RVI's own reason -- its write index trails its read index by the
   * lookback and never overtakes it -- so the order of the two calls is an
   * implementation detail, not what makes the aliasing safe. Swapping them
   * leaves every value identical, which is why the aliasing test earns its
   * keep against the scratch buffer's extent rather than against this order.
   */
  const _xr0 = rvi(startIdx, endIdx, inHigh, optInTimePeriod, optInStdDevPeriod, tempHigh, core);
  tempBegIdx.value = _xr0.begIdx;
  tempNbElement.value = _xr0.count;
  const _xr1 = rvi(startIdx, endIdx, inLow, optInTimePeriod, optInStdDevPeriod, outReal, core);
  outBegIdx.value = _xr1.begIdx;
  outNBElement.value = _xr1.count;
  /* Each leg has already resolved its own zero-total case to TA_RVI's neutral
   * 50 before the average is taken, so a tie in one series never drags the
   * other. Scaling by one half is exact, so the spelling of the average is
   * not a variant: 0.5*(a+b) is one rounding of a+b, as are (a+b)/2 and
   * 0.5*a + 0.5*b.
   */
  for (i = 0; i < outNBElement.value; i += 1) {
    outReal[i] = 0.5 * (tempHigh[i] + outReal[i]);
  }
  return RetCode.SUCCESS;
}

/**
 * Relative Volatility Index, refined form: Donald Dorsey's 1995 revision of his
 * own indicator, which runs the 1993 RVI over the daily highs and again over the
 * daily lows and averages the two indices. Output is bounded \[0..100] and is
 * interpreted like RVI: above 50 the highs and lows have been more volatile while
 * rising than while falling, below 50 the reverse. Dorsey's stated reason for the
 * revision is that a high and a low carry the day's range, so the pair answers the
 * question the close alone can only approximate.
 *
 * Formula and more info at
 * [ta-lib.org/functions/rvir](https://ta-lib.org/functions/rvir).
 *
 * **Notes**
 * - RVIR is the 1995 revision; [`RVI`](https://ta-lib.org/functions/rvi) is the
 *   1993 version.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link rvirLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param optInTimePeriod - Wilder smoothing period applied to both legs of both
 *   indices (default 14; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param optInStdDevPeriod - Number of trailing values each standard deviation
 *   spans (default 10; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param outReal - The averaged index, in 0..100. Must hold the values produced
 *   and never be empty; the input length always suffices.
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
 * import { rvir } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(high.length);
 * const { begIdx, count } = rvir(0, high.length - 1, high, low, 14, 10, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`rvi`](https://ta-lib.org/functions/rvi)
 * @see [`stddev`](https://ta-lib.org/functions/stddev)
 * @see [`atr`](https://ta-lib.org/functions/atr)
 */
export function rvir(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  optInStdDevPeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('RVIR', 'startIdx', startIdx);
  endIdx = requireInt32('RVIR', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('RVIR', 'optInTimePeriod', optInTimePeriod);
  optInStdDevPeriod = requireInt32('RVIR', 'optInStdDevPeriod', optInStdDevPeriod);
  requireIndexRange('RVIR', startIdx, endIdx);
  const guardStart = clampedStart('RVIR', startIdx, rvirLookback(optInTimePeriod, optInStdDevPeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('RVIR', 'inHigh', inHigh, guardInLen);
  requireLength('RVIR', 'inLow', inLow, guardInLen);
  requireLength('RVIR', 'outReal', outReal, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlapsPartly(outReal, inHighF64) || overlapsPartly(outReal, inLowF64)) {
    throw failure('RVIR', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = rvirImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInTimePeriod, optInStdDevPeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('RVIR', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live RVIR stream: one value per closed bar, bit-identical to {@link rvir} over
 * the same series. Open it with {@link rvirOpen} or {@link rvirOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/rvir](https://ta-lib.org/functions/rvir).
 */
export class RvirStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  optInStdDevPeriod: number = 0;
  /** @internal */
  cur_outReal: number = 0;
  /** @internal */
  sub0!: RviStream;
  /** @internal */
  sub1!: RviStream;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  /** @internal */
  constructor(core: Core) {
    this.core = core;
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link rvir} reports them over the same bars. Each accepted `update` and each
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
      throw failure('RVIR advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('RVIR update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('RVIR update', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    rvirStepImpl(this, inHigh, inLow);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number): number {
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('RVIR peek', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    const sp = this;
    let cur_tempHigh = 0;
    let cur_outReal = 0;
    cur_tempHigh = sp.sub0.peek(inHigh);
    cur_outReal = sp.sub1.peek(inLow);
    cur_outReal = 0.5 * (cur_tempHigh + cur_outReal);
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
  clone(): RvirStream {
    const c = new RvirStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.optInStdDevPeriod = this.optInStdDevPeriod;
    c.cur_outReal = this.cur_outReal;
    c.sub0 = this.sub0.clone();
    c.sub1 = this.sub1.clone();
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function rvirStepImpl(sp: RvirStream, inHigh: number, inLow: number): void {
  let cur_tempHigh = 0;
  let cur_outReal = 0;
  cur_tempHigh = sp.sub0.update(inHigh);
  cur_outReal = sp.sub1.update(inLow);
  cur_outReal = 0.5 * (cur_tempHigh + cur_outReal);
  sp.cur_outReal = cur_outReal;
}

function rvirOpenImpl(
  sp: RvirStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInStdDevPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let tempHigh!: Float64Array;
  let i = 0;
  let lookbackTotal = 0;
  const tempBegIdx = new MInteger();
  const tempNbElement = new MInteger();
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInStdDevPeriod === Core.INTEGER_DEFAULT) {
    optInStdDevPeriod = 10;
  } else if (optInStdDevPeriod < 2 || optInStdDevPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen < rvirLookback(optInTimePeriod, optInStdDevPeriod, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const sc_outReal = outStride === 1 ? outReal : new Float64Array(historyLen);
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = rvirLookback(optInTimePeriod, optInStdDevPeriod, core);
  /* Nothing is allocated and no input is read when the range cannot produce a
   * value, so a caller-supplied input that stops short of endIdx is never
   * read past its end (kc.c).
   */
  if (lookbackTotal > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Each leg is the shipped TA_RVI over this range, entered at the same
   * startIdx: same function, same parameters, same lookback, so the two
   * outputs are aligned bar for bar with no anchor to reconcile.
   *
   * Two calls rather than one fused loop. The fused body is expressible --
   * two copies of TA_RVI's state driven by one bar cursor -- but it carries
   * two window-start cursors, and the streaming analyzer accepts exactly one
   * ("extrema automaton: expected exactly one window-start variable"), so
   * fusing would cost this function the streaming tier. Composing keeps it,
   * for the reason KC's legs do: each leg streams as itself.
   */
  tempHigh = new Float64Array(endIdx - startIdx + 1);
  /* Either input may be aliased onto outReal. Both legs are safe against that
   * for TA_RVI's own reason -- its write index trails its read index by the
   * lookback and never overtakes it -- so the order of the two calls is an
   * implementation detail, not what makes the aliasing safe. Swapping them
   * leaves every value identical, which is why the aliasing test earns its
   * keep against the scratch buffer's extent rather than against this order.
   */
  const sub0 = rviOpenAndFillInternal(core, inHigh, startIdx, optInTimePeriod, optInStdDevPeriod, tempBegIdx, tempNbElement, tempHigh);
  const sub1 = rviOpenAndFillInternal(core, inLow, startIdx, optInTimePeriod, optInStdDevPeriod, outBegIdx, outNBElement, sc_outReal);
  /* Each leg has already resolved its own zero-total case to TA_RVI's neutral
   * 50 before the average is taken, so a tie in one series never drags the
   * other. Scaling by one half is exact, so the spelling of the average is
   * not a variant: 0.5*(a+b) is one rounding of a+b, as are (a+b)/2 and
   * 0.5*a + 0.5*b.
   */
  for (i = 0; i < outNBElement.value; i += 1) {
    sc_outReal[i] = 0.5 * (tempHigh[i] + sc_outReal[i]);
  }
  if (outNBElement.value < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.optInStdDevPeriod = optInStdDevPeriod;
  sp.sub0 = sub0;
  sp.sub1 = sub1;
  sp.cur_outReal = sc_outReal[outNBElement.value - 1];
  return RetCode.SUCCESS;
}

/** @internal */
export function rvirOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInStdDevPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RvirStream {
  const sp = new RvirStream(core);
  const retCode = rvirOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, optInStdDevPeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('RVIR openAndFill', inHigh.length, startIdx, rvirLookback(optInTimePeriod, optInStdDevPeriod, core));
  }
  throw streamFailure('RVIR openAndFill', retCode);
}

/** @internal */
export function rvirOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInStdDevPeriod: number,
): RvirStream {
  const sp = new RvirStream(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = rvirOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, optInStdDevPeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('RVIR open', inHigh.length, startIdx, rvirLookback(optInTimePeriod, optInStdDevPeriod, core));
  }
  throw streamFailure('RVIR open', retCode);
}

/**
 * Opens a live RVIR stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link rvir} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Wilder smoothing period applied to both legs of both
 *   indices (default 14; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param optInStdDevPeriod - Number of trailing values each standard deviation
 *   spans (default 10; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `rvirLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { rvirOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = rvirOpen(high, low, 14, 10);
 * const peeked = stream.peek(101.4, 99.1); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1); // the bar closes
 * ```
 */
export function rvirOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  optInStdDevPeriod: number,
  core: Core = Core.DEFAULT,
): RvirStream {
  optInTimePeriod = requireInt32('RVIR open', 'optInTimePeriod', optInTimePeriod);
  optInStdDevPeriod = requireInt32('RVIR open', 'optInStdDevPeriod', optInStdDevPeriod);
  requireArgument('RVIR open', 'inHigh', inHigh);
  requireHistory('RVIR open', inHigh.length);
  requireArgument('RVIR open', 'inLow', inLow);
  requireHistoryLength('RVIR open', 'inLow', inLow.length, inHigh.length);
  return rvirOpenInternal(core, float64(inHigh), float64(inLow), 0, optInTimePeriod, optInStdDevPeriod);
}

/**
 * {@link rvirOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link rvir}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Wilder smoothing period applied to both legs of both
 *   indices (default 14; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param optInStdDevPeriod - Number of trailing values each standard deviation
 *   spans (default 10; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param outReal - The averaged index, in 0..100. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `rvirLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range, histories of different lengths or an output too
 *   short or starting where another output or a `Float64Array` input starts, or, on
 *   a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { rvirOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(high.length);
 * const stream = rvirOpenAndFill(high, low, 14, 10, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function rvirOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  optInStdDevPeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): RvirStream {
  optInTimePeriod = requireInt32('RVIR openAndFill', 'optInTimePeriod', optInTimePeriod);
  optInStdDevPeriod = requireInt32('RVIR openAndFill', 'optInStdDevPeriod', optInStdDevPeriod);
  requireArgument('RVIR openAndFill', 'inHigh', inHigh);
  requireHistory('RVIR openAndFill', inHigh.length);
  requireArgument('RVIR openAndFill', 'inLow', inLow);
  const guardOutLen = openFillCount('RVIR openAndFill', inHigh.length, rvirLookback(optInTimePeriod, optInStdDevPeriod, core));
  requireHistoryLength('RVIR openAndFill', 'inLow', inLow.length, inHigh.length);
  requireLength('RVIR openAndFill', 'outReal', outReal, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if ((outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) || (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset)) {
    throw streamFailure('RVIR openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inHighF64) || overlaps(outReal, inLowF64))) {
    throw streamFailure('RVIR openAndFill', RetCode.BAD_PARAM);
  }
  return rvirOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInTimePeriod, optInStdDevPeriod, new MInteger(), new MInteger(), outReal);
}
