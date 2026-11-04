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
 *  090426 MF,CC  First version (issue #348).
 */

import { FuncUnstId } from '../enums.ts';
import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link rma} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param optInTimePeriod - Number of bars in the seed window, and the reciprocal
 *   of the smoothing factor (default 30; range 1 to 100000; `Core.INTEGER_DEFAULT`
 *   selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function rmaLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  return optInTimePeriod - 1 + core.unstableCount(FuncUnstId.RMA, ((optInTimePeriod > 1) ? (((((10 * (((2 * optInTimePeriod) | 0) - 1)) | 0) + 1) / 2) | 0) : 0), ((optInTimePeriod > 1) ? (((((19 * (((2 * optInTimePeriod) | 0) - 1)) | 0) + 1) / 2) | 0) : 0));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link rma}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Number of bars in the seed window, and the reciprocal
 *   of the smoothing factor (default 30; range 1 to 100000; `Core.INTEGER_DEFAULT`
 *   selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function rmaDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (rmaLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function rmaImpl(
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
  let nbRMA = 0;
  let prevRMA = 0;
  let periodTotal = 0;
  let wAlpha = 0;
  let wBeta = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  /* Adjust startIdx to account for the lookback period. */
  lookbackTotal = rmaLookback(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  /* wAlpha is derived FROM wBeta, never the reverse: only that order makes
   * wAlpha + wBeta exactly 1 (Sterbenz -- wBeta lands in [0.5, 1)), and it
   * measures closer to the exact recursion than the 1/period-first spelling
   * at nearly every period. The order is a gated contract, not a preference:
   * TA_RMA(TA_TRANGE(h,l,c),n) must equal TA_ATR(n) bit for bit.
   * The pair is exactly (1, 0) at period 1 -- hence no period-1 arm.
   */
  wBeta = (optInTimePeriod - 1) / optInTimePeriod;
  wAlpha = 1.0 - wBeta;
  /* The arithmetic order is the rest of that contract: the seed accumulates
   * from 0.0 in input order, and the smoothing step stays ONE statement --
   * splitting it unfuses the multiply-add and puts a second latency on the
   * recurrence's dependency chain.
   *
   * In-place (outReal being inReal) is supported: the output index never
   * passes the bar index of any remaining read.
   */
  today = startIdx - lookbackTotal;
  /* Seed with a simple average of the first 'period' values. */
  periodTotal = 0.0;
  i = optInTimePeriod;
  while (i-- > 0) {
    periodTotal += inReal[today];
    today += 1;
  }
  prevRMA = periodTotal / optInTimePeriod;
  /* Skip the unstable period. */
  i = lookbackTotal - (optInTimePeriod - 1);
  while (i !== 0) {
    prevRMA = fma(wBeta, prevRMA, wAlpha * inReal[today]);
    today += 1;
    i -= 1;
  }
  /* Now start to write the final RMA in the caller
   * provided outReal.
   */
  outIdx = 1;
  outReal[0] = prevRMA;
  /* Now do the number of requested RMA. */
  nbRMA = endIdx - startIdx + 1;
  while (--nbRMA !== 0) {
    prevRMA = fma(wBeta, prevRMA, wAlpha * inReal[today]);
    outReal[outIdx++] = prevRMA;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * Wilder's smoothed moving average: an exponential average whose smoothing factor
 * is the reciprocal of the period rather than the `2/(n+1)` of a classic EMA,
 * seeded with a simple average of the first window. J. Welles Wilder Jr.
 * introduced it in 1978 as the smoothing inside RSI, ATR and the
 * directional-movement family; this exposes it as a moving average in its own
 * right. Because the smoothing factor is smaller than an EMA's at the same period,
 * RMA reacts more slowly and gives noticeably more weight to old data: it takes
 * about twice the period to shed the influence of a bar. Read it as a slow trend
 * line — direction and slope matter, individual crossings much less than on a
 * faster average. RMA is also selectable as a moving-average type
 * (`TA_MAType_RMA`) wherever an `optInMAType` parameter is accepted
 * ([`MA`](https://ta-lib.org/functions/ma),
 * [`BBANDS`](https://ta-lib.org/functions/bbands),
 * [`STOCH`](https://ta-lib.org/functions/stoch),
 * [`MACDEXT`](https://ta-lib.org/functions/macdext), ...). It travels under five
 * names for one object: RMA (TradingView, pandas-ta), SMMA (MetaTrader), Wilder's
 * Smoothing or Wilder's Average (thinkorswim), `wilders` (Tulip), WilderMA
 * (Wealth-Lab).
 *
 * Formula and more info at
 * [ta-lib.org/functions/rma](https://ta-lib.org/functions/rma).
 *
 * **Notes**
 * - Wilder's own writing uses a period of 14, and pandas-ta defaults to 10. The
 *   default here is the one the rest of the moving-average family carries, so a
 *   call that swaps one MA for another keeps its period.
 * - The smoothing factor being `1/N` is sometimes quoted as "an RMA of N is an EMA
 *   of 2N-1". The factors really are identical, since `2/((2N-1)+1)` is `1/N`, but
 *   the two seed over different windows: the series differ through the warm-up and
 *   only converge as the seed's influence decays.
 * - `TA_RMA` over `TA_TRANGE` is `TA_ATR`, bit for bit. The recurrence is spelled
 *   here exactly as ATR spells it.
 * - The recurrence is the `alpha * x + (1 - alpha) * prev` form, which is
 *   TradingView Pine's `ta.rma` and pandas' `ewm(adjust=False)` kernel.
 *   Implementations that spell it `prev + (x - prev) * alpha` or
 *   `(prev * (N-1) + x) / N` are algebraically the same average and differ from
 *   this one only at the last bits.
 * - Being recursive, an output depends on how much history precedes it: the same
 *   bar computed from an earlier start differs while the seed still carries
 *   weight, and that difference decays by a factor of `1 - 1/N` per bar. The
 *   unstable period is how much of the warm-up to discard.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link rmaLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - Data on which to compute the average.
 * @param optInTimePeriod - Number of bars in the seed window, and the reciprocal
 *   of the smoothing factor (default 30; range 1 to 100000; `Core.INTEGER_DEFAULT`
 *   selects the default).
 * @param outReal - Wilder's smoothed moving average of the input. Must hold the
 *   values produced and never be empty; the input length always suffices.
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
 * import { rma } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const { begIdx, count } = rma(0, data.length - 1, data, 30, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`sma`](https://ta-lib.org/functions/sma)
 * @see [`atr`](https://ta-lib.org/functions/atr)
 * @see [`rsi`](https://ta-lib.org/functions/rsi)
 * @see [`dema`](https://ta-lib.org/functions/dema)
 */
export function rma(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('RMA', 'startIdx', startIdx);
  endIdx = requireInt32('RMA', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('RMA', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('RMA', startIdx, endIdx);
  const guardStart = clampedStart('RMA', startIdx, rmaLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('RMA', 'inReal', inReal, guardInLen);
  requireOutput('RMA', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw failure('RMA', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = rmaImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('RMA', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live RMA stream: one value per closed bar, bit-identical to {@link rma} over
 * the same series. Open it with {@link rmaOpen} or {@link rmaOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/rma](https://ta-lib.org/functions/rma).
 */
export class RmaStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  prevRMA: number = 0;
  /** @internal */
  wAlpha: number = 0;
  /** @internal */
  wBeta: number = 0;
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
  static of(core: Core): RmaStream {
    return new RmaStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link rma} reports them over the same bars. Each accepted `update` and each
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
      throw failure('RMA advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw failure('RMA update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('RMA update', 'inReal');
    }
    rmaStepImpl(this, inReal);
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
      throw nonFiniteBar('RMA peek', 'inReal');
    }
    const sp = this;
    let cur_outReal = 0;
    let prevRMA = sp.prevRMA;
    prevRMA = fma(sp.wBeta, prevRMA, sp.wAlpha * inReal);
    cur_outReal = prevRMA;
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
  clone(): RmaStream {
    const c = new RmaStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.prevRMA = this.prevRMA;
    c.wAlpha = this.wAlpha;
    c.wBeta = this.wBeta;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function rmaStepImpl(sp: RmaStream, inReal: number): void {
  sp.prevRMA = fma(sp.wBeta, sp.prevRMA, sp.wAlpha * inReal);
  sp.cur_outReal = sp.prevRMA;
}

function rmaOpenImpl(
  sp: RmaStream,
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
  let nbRMA = 0;
  let prevRMA = 0;
  let periodTotal = 0;
  let wAlpha = 0;
  let wBeta = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  /* Adjust startIdx to account for the lookback period. */
  lookbackTotal = rmaLookback(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* wAlpha is derived FROM wBeta, never the reverse: only that order makes
   * wAlpha + wBeta exactly 1 (Sterbenz -- wBeta lands in [0.5, 1)), and it
   * measures closer to the exact recursion than the 1/period-first spelling
   * at nearly every period. The order is a gated contract, not a preference:
   * TA_RMA(TA_TRANGE(h,l,c),n) must equal TA_ATR(n) bit for bit.
   * The pair is exactly (1, 0) at period 1 -- hence no period-1 arm.
   */
  wBeta = (optInTimePeriod - 1) / optInTimePeriod;
  wAlpha = 1.0 - wBeta;
  /* The arithmetic order is the rest of that contract: the seed accumulates
   * from 0.0 in input order, and the smoothing step stays ONE statement --
   * splitting it unfuses the multiply-add and puts a second latency on the
   * recurrence's dependency chain.
   *
   * In-place (outReal being inReal) is supported: the output index never
   * passes the bar index of any remaining read.
   */
  today = startIdx - lookbackTotal;
  /* Seed with a simple average of the first 'period' values. */
  periodTotal = 0.0;
  i = optInTimePeriod;
  while (i-- > 0) {
    periodTotal += inReal[today];
    today += 1;
  }
  prevRMA = periodTotal / optInTimePeriod;
  /* Skip the unstable period. */
  i = lookbackTotal - (optInTimePeriod - 1);
  while (i !== 0) {
    prevRMA = fma(wBeta, prevRMA, wAlpha * inReal[today]);
    today += 1;
    i -= 1;
  }
  /* Now start to write the final RMA in the caller
   * provided outReal.
   */
  outIdx = 1;
  outReal[((0 * outStride) | 0)] = prevRMA;
  /* Now do the number of requested RMA. */
  nbRMA = endIdx - startIdx + 1;
  while (--nbRMA !== 0) {
    prevRMA = fma(wBeta, prevRMA, wAlpha * inReal[today]);
    outReal[((outIdx++ * outStride) | 0)] = prevRMA;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  sp.optInTimePeriod = optInTimePeriod;
  sp.prevRMA = prevRMA;
  sp.wAlpha = wAlpha;
  sp.wBeta = wBeta;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function rmaOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RmaStream {
  const sp = RmaStream.of(core);
  const retCode = rmaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('RMA openAndFill', inReal.length, startIdx, rmaLookback(optInTimePeriod, core));
  }
  throw streamFailure('RMA openAndFill', retCode);
}

/** @internal */
export function rmaOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): RmaStream {
  const sp = RmaStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = rmaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('RMA open', inReal.length, startIdx, rmaLookback(optInTimePeriod, core));
  }
  throw streamFailure('RMA open', retCode);
}

/**
 * Opens a live RMA stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link rma} there.
 *
 * @param inReal - Warm-up history. Data on which to compute the average.
 * @param optInTimePeriod - Number of bars in the seed window, and the reciprocal
 *   of the smoothing factor (default 30; range 1 to 100000; `Core.INTEGER_DEFAULT`
 *   selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `rmaLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32 or a
 *   parameter outside its range.
 *
 * @example
 * ```ts
 * import { rmaOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = rmaOpen(data, 30);
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function rmaOpen(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): RmaStream {
  optInTimePeriod = requireInt32('RMA open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('RMA open', 'inReal', inReal);
  requireHistory('RMA open', inReal.length);
  return rmaOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/**
 * {@link rmaOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link rma}. The range written is the stream's
 * `outRange()`.
 *
 * @param inReal - Warm-up history. Data on which to compute the average.
 * @param optInTimePeriod - Number of bars in the seed window, and the reciprocal
 *   of the smoothing factor (default 30; range 1 to 100000; `Core.INTEGER_DEFAULT`
 *   selects the default).
 * @param outReal - Wilder's smoothed moving average of the input. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `rmaLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or an output too short or starting where another
 *   output or a `Float64Array` input starts, or, on a history long enough to open,
 *   one sharing memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { rmaOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const stream = rmaOpenAndFill(data, 30, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function rmaOpenAndFill(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): RmaStream {
  optInTimePeriod = requireInt32('RMA openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('RMA openAndFill', 'inReal', inReal);
  requireHistory('RMA openAndFill', inReal.length);
  const guardOutLen = openFillCount('RMA openAndFill', inReal.length, rmaLookback(optInTimePeriod, core));
  requireOutput('RMA openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if ((outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset)) {
    throw streamFailure('RMA openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inRealF64))) {
    throw streamFailure('RMA openAndFill', RetCode.BAD_PARAM);
  }
  return rmaOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/** @internal */
export function rmaStepTape(sp: RmaStream, _tape: Float64Array, _tapeBase: number, _tapeMask: number, inReal: number): number {
  rmaStepImpl(sp, inReal);
  sp.outRangeCount++;
  return sp.cur_outReal;
}

/** @internal */
export function rmaPeekTape(sp: RmaStream, _tape: Float64Array, _tapeBase: number, _tapeMask: number, inReal: number): number {
  return sp.peek(inReal);
}

/** @internal */
export function rmaTapeDetach(_sp: RmaStream): number {
  return 0;
}
