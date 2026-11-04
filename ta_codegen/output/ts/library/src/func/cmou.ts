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
 *  071626 MF,CC  Initial version (#124).
 *  082326 MF,CC  Fix #253. Recognize a flat window by counting bars, so the
 *                0/0 is guarded exactly instead of against the fixed
 *                TA_IS_ZERO band -- which zeroed the oscillator for any
 *                instrument quoted small enough to fall under it.
 *  102326 MF,CC  #554 branch-free up/down split and zero run, rare reset.
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link cmou} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Number of trailing price changes summed (default 14;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function cmouLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  /* CMOU needs optInTimePeriod price changes -> optInTimePeriod+1 prices ->
   * the first output is at index optInTimePeriod.
   *
   * Unlike the shipped CMO, there is NO unstable period: CMOU is a plain
   * moving-window sum, so its lookback is exactly the period.
   */
  return optInTimePeriod;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cmou}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Number of trailing price changes summed (default 14;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function cmouDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (cmouLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cmouImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let outIdx = 0;
  let today = 0;
  let trailingIdx = 0;
  let lookbackTotal = 0;
  let i = 0;
  let nullRun = 0;
  let live = 0;
  let upSum = 0;
  let downSum = 0;
  let sum = 0;
  let diff = 0;
  let up = 0;
  let tempReal = 0;
  let prevValue = 0;
  let trailingValue = 0;
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
  /* CMOU -- unsmoothed Chande Momentum Oscillator (as in TradingView ta.cmo,
   * pandas-ta default). Over the trailing optInTimePeriod changes
   * d = inReal[i]-inReal[i-1]: Su = sum of up-moves (d>0), Sd = sum of
   * |down-moves| (d<0); CMOU = 100*(Su-Sd)/(Su+Sd), 0 for a flat window. A plain
   * moving-window sum (drop oldest change, add newest), NOT TA_CMO's Wilder
   * smoothing -- hence no unstable period.
   *
   * In-place safe (outReal == inReal): the trailing read inReal[trailingIdx]
   * is of a slot not yet written (trailingIdx >= outIdx), and the oldest
   * change's older endpoint comes from the `trailingValue` cache, not a re-read.
   */
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = cmouLookback(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  /* Accumulate the up/down sums over the first window: the optInTimePeriod
   * changes ending at startIdx (prices inReal[startIdx-optInTimePeriod ..
   * startIdx]). `trailingValue` caches the oldest price so the window's oldest
   * change can later be dropped by reading only the newer of its two prices.
   * `trailingIdx` points AT that newer price (one past the cached one).
   */
  today = startIdx - lookbackTotal;
  trailingIdx = today + 1;
  prevValue = inReal[today];
  trailingValue = prevValue;
  upSum = 0.0;
  downSum = 0.0;
  /* Consecutive changes of exactly zero, counted so that an empty window can
   * be recognized exactly (the shape #244 needed for MFI). The sums cannot
   * answer that question themselves once the window starts sliding: they are
   * maintained by add-then-subtract, so an emptied window leaves them holding
   * rounding residue of arbitrary sign rather than zero.
   *
   * Both the split and the run are branch-free, as an if/else on the sign or
   * on a zero mispredicts on random or flat-heavy prices. `up` is max(diff, 0)
   * spelled with fabs, which no backend turns back into a branch; adding its
   * +0.0 is exact because neither sum is ever -0.0.
   */
  nullRun = 0;
  for (i = 0; i < optInTimePeriod; i += 1) {
    today += 1;
    tempReal = inReal[today];
    diff = tempReal - prevValue;
    prevValue = tempReal;
    up = (diff + Math.abs(diff)) * 0.5;
    upSum += up;
    downSum += up - diff;
    live = (!(Math.abs(diff) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
  }
  /* Emit the first output (bar startIdx). Su+Sd is a sum of non-negative
   * magnitudes, so it is zero only for an exactly flat window; guard the 0/0
   * exactly and emit 0.0. Not against a fixed band: a price change carries the
   * quote unit, so a constant put against the total zeroed the oscillator for
   * any instrument quoted below it (issue #253).
   *
   * Scale-then-divide -- (100*(Su-Sd))/(Su+Sd), NOT the 100*((Su-Sd)/(Su+Sd))
   * order TA_CMO/RSI use -- so CMOU is BIT-IDENTICAL to the reference unsmoothed
   * CMO of Tulip Indicators (ti_cmo) and pandas-ta-classic (cmo, talib=False),
   * which both scale before dividing. The two orders differ by <=1 ULP.
   */
  outIdx = 0;
  sum = upSum + downSum;
  if (sum > 0.0) {
    outReal[outIdx++] = 100.0 * (upSum - downSum) / sum;
  } else {
    outReal[outIdx++] = 0.0;
  }
  /* Slide the window forward one bar at a time. Each bar drops the change
   * that leaves the window AFTER its output, ready for the next bar: the
   * sums see the same operations in the same order, and a peek, which keeps
   * nothing past its output, skips that half of the work. The oldest
   * change's older price comes from the cache.
   */
  tempReal = inReal[trailingIdx];
  diff = tempReal - trailingValue;
  trailingValue = tempReal;
  trailingIdx += 1;
  up = (diff + Math.abs(diff)) * 0.5;
  upSum -= up;
  downSum -= up - diff;
  today += 1;
  while (today <= endIdx) {
    /* Add the newest change: inReal[today] - inReal[today-1]. */
    tempReal = inReal[today];
    diff = tempReal - prevValue;
    prevValue = tempReal;
    up = (diff + Math.abs(diff)) * 0.5;
    upSum += up;
    downSum += up - diff;
    /* Once a whole period of flat bars has gone by, every change in the
     * window is exactly zero, so both sums are known to be exactly zero and
     * the residue can be dropped.
     */
    live = (!(Math.abs(diff) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
    if (nullRun >= optInTimePeriod) {
      nullRun = optInTimePeriod;
      upSum = 0.0;
      downSum = 0.0;
      outReal[outIdx++] = 0.0;
    } else {
      sum = upSum + downSum;
      if (sum > 0.0) {
        outReal[outIdx++] = 100.0 * (upSum - downSum) / sum;
      } else {
        outReal[outIdx++] = 0.0;
      }
    }
    tempReal = inReal[trailingIdx];
    diff = tempReal - trailingValue;
    trailingValue = tempReal;
    trailingIdx += 1;
    up = (diff + Math.abs(diff)) * 0.5;
    upSum -= up;
    downSum -= up - diff;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * Chande Momentum Oscillator: Tushar Chande's original momentum oscillator,
 * computed from **plain moving-window sums** of the up-moves and down-moves over
 * the period. Bounded in \[-100,+100]; positive = net upward momentum, negative =
 * net downward. CMOU is the version as defined by Chande in his book *The New
 * Technical Trader* (1994), and is the more common implementation used by
 * TradingView (`ta.cmo`) and pandas-ta's default. See
 * [`CMO`](https://ta-lib.org/functions/cmo) for a smoothed variant of CMOU.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cmou](https://ta-lib.org/functions/cmou).
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link cmouLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - Source price/value series.
 * @param optInTimePeriod - Number of trailing price changes summed (default 14;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - CMOU oscillator value. Must hold the values produced and never
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
 * import { cmou } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const { begIdx, count } = cmou(0, data.length - 1, data, 14, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`cmo`](https://ta-lib.org/functions/cmo)
 * @see [`rsi`](https://ta-lib.org/functions/rsi)
 */
export function cmou(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CMOU', 'startIdx', startIdx);
  endIdx = requireInt32('CMOU', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('CMOU', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('CMOU', startIdx, endIdx);
  const guardStart = clampedStart('CMOU', startIdx, cmouLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CMOU', 'inReal', inReal, guardInLen);
  requireOutput('CMOU', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw failure('CMOU', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cmouImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CMOU', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live CMOU stream: one value per closed bar, bit-identical to {@link cmou} over
 * the same series. Open it with {@link cmouOpen} or {@link cmouOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cmou](https://ta-lib.org/functions/cmou).
 */
export class CmouStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  nullRun: number = 0;
  /** @internal */
  upSum: number = 0;
  /** @internal */
  downSum: number = 0;
  /** @internal */
  prevValue: number = 0;
  /** @internal */
  trailingValue: number = 0;
  /** @internal */
  ringPos_trailingIdx: number = 0;
  /** @internal */
  ringCap_trailingIdx: number = 0;
  /** @internal */
  ring_trailingIdx_inReal: Float64Array = new Float64Array(1);
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
  static of(core: Core): CmouStream {
    return new CmouStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cmou} reports them over the same bars. Each accepted `update` and each
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
      throw failure('CMOU advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw failure('CMOU update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('CMOU update', 'inReal');
    }
    cmouStepImpl(this, inReal);
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
      throw nonFiniteBar('CMOU peek', 'inReal');
    }
    const sp = this;
    let live = 0;
    let sum = 0;
    let diff = 0;
    let up = 0;
    let tempReal = 0;
    let cur_outReal = 0;
    let downSum = sp.downSum;
    let nullRun = sp.nullRun;
    let prevValue = sp.prevValue;
    let upSum = sp.upSum;
    /* Add the newest change: inReal[today] - inReal[today-1]. */
    tempReal = inReal;
    diff = tempReal - prevValue;
    prevValue = tempReal;
    up = (diff + Math.abs(diff)) * 0.5;
    upSum += up;
    downSum += up - diff;
    /* Once a whole period of flat bars has gone by, every change in the
     * window is exactly zero, so both sums are known to be exactly zero and
     * the residue can be dropped.
     */
    live = (!(Math.abs(diff) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
    if (nullRun >= sp.optInTimePeriod) {
      nullRun = sp.optInTimePeriod;
      upSum = 0.0;
      downSum = 0.0;
      cur_outReal = 0.0;
    } else {
      sum = upSum + downSum;
      if (sum > 0.0) {
        cur_outReal = 100.0 * (upSum - downSum) / sum;
      } else {
        cur_outReal = 0.0;
      }
    }
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
  clone(): CmouStream {
    const c = new CmouStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.nullRun = this.nullRun;
    c.upSum = this.upSum;
    c.downSum = this.downSum;
    c.prevValue = this.prevValue;
    c.trailingValue = this.trailingValue;
    c.ringPos_trailingIdx = this.ringPos_trailingIdx;
    c.ringCap_trailingIdx = this.ringCap_trailingIdx;
    c.ring_trailingIdx_inReal = this.ring_trailingIdx_inReal.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cmouStepImpl(sp: CmouStream, inReal: number): void {
  let live = 0;
  let sum = 0;
  let diff = 0;
  let up = 0;
  let tempReal = 0;
  let ringCapL_trailingIdx = 0;
  /* Add the newest change: inReal[today] - inReal[today-1]. */
  tempReal = inReal;
  diff = tempReal - sp.prevValue;
  sp.prevValue = tempReal;
  up = (diff + Math.abs(diff)) * 0.5;
  sp.upSum += up;
  sp.downSum += up - diff;
  /* Once a whole period of flat bars has gone by, every change in the
   * window is exactly zero, so both sums are known to be exactly zero and
   * the residue can be dropped.
   */
  live = (!(Math.abs(diff) <= 0.0) ? 1 : 0);
  sp.nullRun = (sp.nullRun + 1) & (live - 1);
  if (sp.nullRun >= sp.optInTimePeriod) {
    sp.nullRun = sp.optInTimePeriod;
    sp.upSum = 0.0;
    sp.downSum = 0.0;
    sp.cur_outReal = 0.0;
  } else {
    sum = sp.upSum + sp.downSum;
    if (sum > 0.0) {
      sp.cur_outReal = 100.0 * (sp.upSum - sp.downSum) / sum;
    } else {
      sp.cur_outReal = 0.0;
    }
  }
  tempReal = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
  diff = tempReal - sp.trailingValue;
  sp.trailingValue = tempReal;
  up = (diff + Math.abs(diff)) * 0.5;
  sp.upSum -= up;
  sp.downSum -= up - diff;
  ringCapL_trailingIdx = sp.ringCap_trailingIdx;
  sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
  sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
  if (sp.ringPos_trailingIdx >= ringCapL_trailingIdx) {
    sp.ringPos_trailingIdx = 0;
  }
}

function cmouOpenImpl(
  sp: CmouStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let outIdx = 0;
  let today = 0;
  let trailingIdx = 0;
  let lookbackTotal = 0;
  let i = 0;
  let nullRun = 0;
  let live = 0;
  let upSum = 0;
  let downSum = 0;
  let sum = 0;
  let diff = 0;
  let up = 0;
  let tempReal = 0;
  let prevValue = 0;
  let trailingValue = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
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
  /* CMOU -- unsmoothed Chande Momentum Oscillator (as in TradingView ta.cmo,
   * pandas-ta default). Over the trailing optInTimePeriod changes
   * d = inReal[i]-inReal[i-1]: Su = sum of up-moves (d>0), Sd = sum of
   * |down-moves| (d<0); CMOU = 100*(Su-Sd)/(Su+Sd), 0 for a flat window. A plain
   * moving-window sum (drop oldest change, add newest), NOT TA_CMO's Wilder
   * smoothing -- hence no unstable period.
   *
   * In-place safe (outReal == inReal): the trailing read inReal[trailingIdx]
   * is of a slot not yet written (trailingIdx >= outIdx), and the oldest
   * change's older endpoint comes from the `trailingValue` cache, not a re-read.
   */
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = cmouLookback(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Accumulate the up/down sums over the first window: the optInTimePeriod
   * changes ending at startIdx (prices inReal[startIdx-optInTimePeriod ..
   * startIdx]). `trailingValue` caches the oldest price so the window's oldest
   * change can later be dropped by reading only the newer of its two prices.
   * `trailingIdx` points AT that newer price (one past the cached one).
   */
  today = startIdx - lookbackTotal;
  trailingIdx = today + 1;
  prevValue = inReal[today];
  trailingValue = prevValue;
  upSum = 0.0;
  downSum = 0.0;
  /* Consecutive changes of exactly zero, counted so that an empty window can
   * be recognized exactly (the shape #244 needed for MFI). The sums cannot
   * answer that question themselves once the window starts sliding: they are
   * maintained by add-then-subtract, so an emptied window leaves them holding
   * rounding residue of arbitrary sign rather than zero.
   *
   * Both the split and the run are branch-free, as an if/else on the sign or
   * on a zero mispredicts on random or flat-heavy prices. `up` is max(diff, 0)
   * spelled with fabs, which no backend turns back into a branch; adding its
   * +0.0 is exact because neither sum is ever -0.0.
   */
  nullRun = 0;
  for (i = 0; i < optInTimePeriod; i += 1) {
    today += 1;
    tempReal = inReal[today];
    diff = tempReal - prevValue;
    prevValue = tempReal;
    up = (diff + Math.abs(diff)) * 0.5;
    upSum += up;
    downSum += up - diff;
    live = (!(Math.abs(diff) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
  }
  /* Emit the first output (bar startIdx). Su+Sd is a sum of non-negative
   * magnitudes, so it is zero only for an exactly flat window; guard the 0/0
   * exactly and emit 0.0. Not against a fixed band: a price change carries the
   * quote unit, so a constant put against the total zeroed the oscillator for
   * any instrument quoted below it (issue #253).
   *
   * Scale-then-divide -- (100*(Su-Sd))/(Su+Sd), NOT the 100*((Su-Sd)/(Su+Sd))
   * order TA_CMO/RSI use -- so CMOU is BIT-IDENTICAL to the reference unsmoothed
   * CMO of Tulip Indicators (ti_cmo) and pandas-ta-classic (cmo, talib=False),
   * which both scale before dividing. The two orders differ by <=1 ULP.
   */
  outIdx = 0;
  sum = upSum + downSum;
  if (sum > 0.0) {
    outReal[((outIdx++ * outStride) | 0)] = 100.0 * (upSum - downSum) / sum;
  } else {
    outReal[((outIdx++ * outStride) | 0)] = 0.0;
  }
  /* Slide the window forward one bar at a time. Each bar drops the change
   * that leaves the window AFTER its output, ready for the next bar: the
   * sums see the same operations in the same order, and a peek, which keeps
   * nothing past its output, skips that half of the work. The oldest
   * change's older price comes from the cache.
   */
  tempReal = inReal[trailingIdx];
  diff = tempReal - trailingValue;
  trailingValue = tempReal;
  trailingIdx += 1;
  up = (diff + Math.abs(diff)) * 0.5;
  upSum -= up;
  downSum -= up - diff;
  today += 1;
  while (today <= endIdx) {
    /* Add the newest change: inReal[today] - inReal[today-1]. */
    tempReal = inReal[today];
    diff = tempReal - prevValue;
    prevValue = tempReal;
    up = (diff + Math.abs(diff)) * 0.5;
    upSum += up;
    downSum += up - diff;
    /* Once a whole period of flat bars has gone by, every change in the
     * window is exactly zero, so both sums are known to be exactly zero and
     * the residue can be dropped.
     */
    live = (!(Math.abs(diff) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
    if (nullRun >= optInTimePeriod) {
      nullRun = optInTimePeriod;
      upSum = 0.0;
      downSum = 0.0;
      outReal[((outIdx++ * outStride) | 0)] = 0.0;
    } else {
      sum = upSum + downSum;
      if (sum > 0.0) {
        outReal[((outIdx++ * outStride) | 0)] = 100.0 * (upSum - downSum) / sum;
      } else {
        outReal[((outIdx++ * outStride) | 0)] = 0.0;
      }
    }
    tempReal = inReal[trailingIdx];
    diff = tempReal - trailingValue;
    trailingValue = tempReal;
    trailingIdx += 1;
    up = (diff + Math.abs(diff)) * 0.5;
    upSum -= up;
    downSum -= up - diff;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  const cap_trailingIdx = today - trailingIdx;
  if (cap_trailingIdx < 1 || cap_trailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_trailingIdx = cap_trailingIdx > 0 ? cap_trailingIdx : 1;
  const capRing_trailingIdx_inReal = new Float64Array(allocN_trailingIdx);
  capRing_trailingIdx_inReal.set(inReal.subarray(historyLen - cap_trailingIdx, historyLen), 0);
  sp.optInTimePeriod = optInTimePeriod;
  sp.nullRun = nullRun;
  sp.upSum = upSum;
  sp.downSum = downSum;
  sp.prevValue = prevValue;
  sp.trailingValue = trailingValue;
  sp.ringPos_trailingIdx = 0;
  sp.ringCap_trailingIdx = cap_trailingIdx;
  sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cmouOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): CmouStream {
  const sp = CmouStream.of(core);
  const retCode = cmouOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CMOU openAndFill', inReal.length, startIdx, cmouLookback(optInTimePeriod, core));
  }
  throw streamFailure('CMOU openAndFill', retCode);
}

/** @internal */
export function cmouOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): CmouStream {
  const sp = CmouStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = cmouOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CMOU open', inReal.length, startIdx, cmouLookback(optInTimePeriod, core));
  }
  throw streamFailure('CMOU open', retCode);
}

/**
 * Opens a live CMOU stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link cmou} there.
 *
 * @param inReal - Warm-up history. Source price/value series.
 * @param optInTimePeriod - Number of trailing price changes summed (default 14;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cmouLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32 or a
 *   parameter outside its range.
 *
 * @example
 * ```ts
 * import { cmouOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = cmouOpen(data, 14);
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function cmouOpen(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): CmouStream {
  optInTimePeriod = requireInt32('CMOU open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('CMOU open', 'inReal', inReal);
  requireHistory('CMOU open', inReal.length);
  return cmouOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/**
 * {@link cmouOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link cmou}. The range written is the stream's
 * `outRange()`.
 *
 * @param inReal - Warm-up history. Source price/value series.
 * @param optInTimePeriod - Number of trailing price changes summed (default 14;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - CMOU oscillator value. Must hold `historyLen - lookback` values
 *   and share no memory with an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cmouLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or an output too short or starting where another
 *   output or a `Float64Array` input starts, or, on a history long enough to open,
 *   one sharing memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cmouOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const stream = cmouOpenAndFill(data, 14, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function cmouOpenAndFill(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): CmouStream {
  optInTimePeriod = requireInt32('CMOU openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('CMOU openAndFill', 'inReal', inReal);
  requireHistory('CMOU openAndFill', inReal.length);
  const guardOutLen = openFillCount('CMOU openAndFill', inReal.length, cmouLookback(optInTimePeriod, core));
  requireOutput('CMOU openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if ((outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset)) {
    throw streamFailure('CMOU openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inRealF64))) {
    throw streamFailure('CMOU openAndFill', RetCode.BAD_PARAM);
  }
  return cmouOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}
