/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090626 KL     First version (issue #350).
 *  102326 MF,CC  #554 branch-free zero run, rare reset.
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link er} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Number of one-bar changes in the path sum (`KAMA`'s
 *   `optInTimePeriod` is the same window, under its own default) (default 10; range
 *   2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function erLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 10;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  /* P one-bar changes need P+1 prices: first output at index P. */
  return optInTimePeriod;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link er}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Number of one-bar changes in the path sum (`KAMA`'s
 *   `optInTimePeriod` is the same window, under its own default) (default 10; range
 *   2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function erDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (erLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function erImpl(
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
  let sumROC1 = 0;
  let periodROC = 0;
  let tempReal = 0;
  let tempReal2 = 0;
  let trailingValue = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 10;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  /* Kaufman Efficiency Ratio (Perry J. Kaufman, Smarter Trading, 1995):
   * net directional movement over the period divided by the total path
   * travelled,
   *
   *   ER[t] = |c[t] - c[t-P]| / SUM(k = t-P+1 .. t) |c[k] - c[k-1]|
   *
   * The ratio is in [0,1] in exact arithmetic, but sumROC1 drifts, so the
   * clamp to 1.0 is what makes the declared range a bound rather than a
   * hope -- and in kama.c what keeps its recurrence a convex combination.
   *
   * This is a lift of TA_KAMA's inner efficiency ratio (kama.c) so the
   * two stay bit-identical -- the KAMA-reconstruction differential in
   * test_composite2.c exists to keep it that way. Three more guards are
   * load-bearing and shared with kama.c:
   *
   *   - `sumROC1 <= periodROC` pins the ratio to exactly 1.0 where FP
   *     would give 1.0000000000000002, on up-moves. It compares against
   *     the SIGNED numerator, so it is false for every down move: it
   *     bounds nothing on its own, which is what the clamp is for.
   *   - a genuinely flat window is recognized by COUNTING exactly-zero
   *     one-bar changes (nullRun >= P forces sumROC1 to 0.0, purging the
   *     running sum's rounding residue, and the ratio to 1.0) -- never
   *     NaN (#112). This is kama.c's #253 form; the
   *     absolute TA_IS_ZERO band it replaced fails the QUOTE-UNIT/SCALE
   *     gate (ER is homogeneous of degree 0, and a fixed 1e-14 met a
   *     price-carrying sum).
   *
   * The third is the denominator test (#385): a subtract-then-add sum can
   * reach 0.0, or below it, on a window that is not flat. The clamp
   * subsumes it numerically, so it is held by the structural sweep over
   * divisors, not by any value test.
   *
   * The subtract-then-add update order matches TA_SUM's recurrence,
   * which is what makes the composite differential bit-exact. The
   * trailing value is cached one iteration ahead, which is what keeps
   * outReal == inReal aliasing safe.
   */
  lookbackTotal = erLookback(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  /* Prime the path sum over the optInTimePeriod one-bar changes ending
   * at the first output bar's predecessor.
   */
  sumROC1 = 0.0;
  nullRun = 0;
  today = startIdx - lookbackTotal;
  trailingIdx = today;
  i = optInTimePeriod;
  while (i-- > 0) {
    tempReal = inReal[today++];
    tempReal -= inReal[today];
    sumROC1 += Math.abs(tempReal);
    live = (!(Math.abs(tempReal) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
  }
  /* First output: today == startIdx. */
  tempReal = inReal[today];
  tempReal2 = inReal[trailingIdx++];
  periodROC = tempReal - tempReal2;
  trailingValue = tempReal2;
  /* A fully flat priming window sums to an exact 0.0 (no residue yet), so
   * `0 <= 0` already answers 1.0 here without the nullRun purge. The
   * denominator test below is unreachable at this site -- a priming sum only
   * ever has non-negative terms added to it -- and is written anyway so both
   * sites read as one rule.
   */
  if (sumROC1 <= 0.0 || sumROC1 <= periodROC) {
    outReal[0] = 1.0;
  } else {
    tempReal = Math.abs(periodROC / sumROC1);
    if (tempReal > 1.0) {
      tempReal = 1.0;
    }
    outReal[0] = tempReal;
  }
  outIdx = 1;
  today += 1;
  while (today <= endIdx) {
    tempReal = inReal[today];
    tempReal2 = inReal[trailingIdx++];
    periodROC = tempReal - tempReal2;
    /* Subtract-then-add, TA_SUM's own order. */
    sumROC1 -= Math.abs(trailingValue - tempReal2);
    sumROC1 += Math.abs(tempReal - inReal[today - 1]);
    /* Once a whole window of one-bar changes is exactly zero, the sum's
     * only content is rounding residue from the subtract/add carry --
     * purge it so the flat window is recognized exactly (kama.c #253).
     */
    live = (!(Math.abs(tempReal - inReal[today - 1]) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
    if (nullRun >= optInTimePeriod) {
      nullRun = optInTimePeriod;
      sumROC1 = 0.0;
      outReal[outIdx++] = 1.0;
    } else if (sumROC1 <= 0.0 || sumROC1 <= periodROC) {
      outReal[outIdx++] = 1.0;
    } else {
      tempReal = Math.abs(periodROC / sumROC1);
      if (tempReal > 1.0) {
        tempReal = 1.0;
      }
      outReal[outIdx++] = tempReal;
    }
    /* Save the trailing value: outReal may alias inReal, and the next
     * iteration's subtraction needs the ORIGINAL bar, not the slot the
     * write above may have clobbered.
     */
    trailingValue = tempReal2;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * Kaufman Efficiency Ratio (also searched as "KER"): Perry Kaufman's noise measure
 * from *Smarter Trading* (1995) — the net directional movement over the period
 * divided by the total path travelled to get there. 1.0 is a perfectly efficient
 * (straight-line) move; values near 0 are churn. This is exactly the efficiency
 * ratio [`KAMA`](https://ta-lib.org/functions/kama) computes internally to set its
 * adaptive smoothing constant, exposed standalone and kept bit-identical to it.
 *
 * Formula and more info at
 * [ta-lib.org/functions/er](https://ta-lib.org/functions/er).
 *
 * **Notes**
 * - First output at index `P` (`P` one-bar changes need `P+1` prices). No unstable
 *   period, not start-dependent.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link erLookback} is a success with `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - Source price/value series (canonically close)
 * @param optInTimePeriod - Number of one-bar changes in the path sum (`KAMA`'s
 *   `optInTimePeriod` is the same window, under its own default) (default 10; range
 *   2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Efficiency ratio. Must hold the values produced and never be
 *   empty; the input length always suffices.
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
 * import { er } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const { begIdx, count } = er(0, data.length - 1, data, 10, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`kama`](https://ta-lib.org/functions/kama)
 * @see [`mama`](https://ta-lib.org/functions/mama)
 * @see [`stddev`](https://ta-lib.org/functions/stddev)
 * @see [`vhf`](https://ta-lib.org/functions/vhf)
 */
export function er(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('ER', 'startIdx', startIdx);
  endIdx = requireInt32('ER', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('ER', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('ER', startIdx, endIdx);
  const guardStart = clampedStart('ER', startIdx, erLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('ER', 'inReal', inReal, guardInLen);
  requireOutput('ER', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw failure('ER', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = erImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('ER', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live ER stream: one value per closed bar, bit-identical to {@link er} over the
 * same series. Open it with {@link erOpen} or {@link erOpenAndFill}; there is
 * nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/er](https://ta-lib.org/functions/er).
 */
export class ErStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  nullRun: number = 0;
  /** @internal */
  sumROC1: number = 0;
  /** @internal */
  trailingValue: number = 0;
  /** @internal */
  lag1_inReal: number = 0;
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
  static of(core: Core): ErStream {
    return new ErStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link er} reports them over the same bars. Each accepted `update` and each
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
      throw failure('ER advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: Source price/value series (canonically
   *   close)
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('ER update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('ER update', 'inReal');
    }
    erStepImpl(this, inReal);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Source price/value series (canonically
   *   close)
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number): number {
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('ER peek', 'inReal');
    }
    const sp = this;
    let live = 0;
    let periodROC = 0;
    let tempReal = 0;
    let tempReal2 = 0;
    let cur_outReal = 0;
    let nullRun = sp.nullRun;
    let sumROC1 = sp.sumROC1;
    tempReal = inReal;
    tempReal2 = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
    periodROC = tempReal - tempReal2;
    /* Subtract-then-add, TA_SUM's own order. */
    sumROC1 -= Math.abs(sp.trailingValue - tempReal2);
    sumROC1 += Math.abs(tempReal - sp.lag1_inReal);
    /* Once a whole window of one-bar changes is exactly zero, the sum's
     * only content is rounding residue from the subtract/add carry --
     * purge it so the flat window is recognized exactly (kama.c #253).
     */
    live = (!(Math.abs(tempReal - sp.lag1_inReal) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
    if (nullRun >= sp.optInTimePeriod) {
      nullRun = sp.optInTimePeriod;
      sumROC1 = 0.0;
      cur_outReal = 1.0;
    } else if (sumROC1 <= 0.0 || sumROC1 <= periodROC) {
      cur_outReal = 1.0;
    } else {
      tempReal = Math.abs(periodROC / sumROC1);
      if (tempReal > 1.0) {
        tempReal = 1.0;
      }
      cur_outReal = tempReal;
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
  clone(): ErStream {
    const c = new ErStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.nullRun = this.nullRun;
    c.sumROC1 = this.sumROC1;
    c.trailingValue = this.trailingValue;
    c.lag1_inReal = this.lag1_inReal;
    c.ringPos_trailingIdx = this.ringPos_trailingIdx;
    c.ringCap_trailingIdx = this.ringCap_trailingIdx;
    c.ring_trailingIdx_inReal = this.ring_trailingIdx_inReal.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function erStepImpl(sp: ErStream, inReal: number): void {
  let live = 0;
  let periodROC = 0;
  let tempReal = 0;
  let tempReal2 = 0;
  let ringCapL_trailingIdx = 0;
  tempReal = inReal;
  tempReal2 = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
  periodROC = tempReal - tempReal2;
  /* Subtract-then-add, TA_SUM's own order. */
  sp.sumROC1 -= Math.abs(sp.trailingValue - tempReal2);
  sp.sumROC1 += Math.abs(tempReal - sp.lag1_inReal);
  /* Once a whole window of one-bar changes is exactly zero, the sum's
   * only content is rounding residue from the subtract/add carry --
   * purge it so the flat window is recognized exactly (kama.c #253).
   */
  live = (!(Math.abs(tempReal - sp.lag1_inReal) <= 0.0) ? 1 : 0);
  sp.nullRun = (sp.nullRun + 1) & (live - 1);
  if (sp.nullRun >= sp.optInTimePeriod) {
    sp.nullRun = sp.optInTimePeriod;
    sp.sumROC1 = 0.0;
    sp.cur_outReal = 1.0;
  } else if (sp.sumROC1 <= 0.0 || sp.sumROC1 <= periodROC) {
    sp.cur_outReal = 1.0;
  } else {
    tempReal = Math.abs(periodROC / sp.sumROC1);
    if (tempReal > 1.0) {
      tempReal = 1.0;
    }
    sp.cur_outReal = tempReal;
  }
  /* Save the trailing value: outReal may alias inReal, and the next
   * iteration's subtraction needs the ORIGINAL bar, not the slot the
   * write above may have clobbered.
   */
  sp.trailingValue = tempReal2;
  sp.lag1_inReal = inReal;
  ringCapL_trailingIdx = sp.ringCap_trailingIdx;
  sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
  sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
  if (sp.ringPos_trailingIdx >= ringCapL_trailingIdx) {
    sp.ringPos_trailingIdx = 0;
  }
}

function erOpenImpl(
  sp: ErStream,
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
  let sumROC1 = 0;
  let periodROC = 0;
  let tempReal = 0;
  let tempReal2 = 0;
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
    optInTimePeriod = 10;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Kaufman Efficiency Ratio (Perry J. Kaufman, Smarter Trading, 1995):
   * net directional movement over the period divided by the total path
   * travelled,
   *
   *   ER[t] = |c[t] - c[t-P]| / SUM(k = t-P+1 .. t) |c[k] - c[k-1]|
   *
   * The ratio is in [0,1] in exact arithmetic, but sumROC1 drifts, so the
   * clamp to 1.0 is what makes the declared range a bound rather than a
   * hope -- and in kama.c what keeps its recurrence a convex combination.
   *
   * This is a lift of TA_KAMA's inner efficiency ratio (kama.c) so the
   * two stay bit-identical -- the KAMA-reconstruction differential in
   * test_composite2.c exists to keep it that way. Three more guards are
   * load-bearing and shared with kama.c:
   *
   *   - `sumROC1 <= periodROC` pins the ratio to exactly 1.0 where FP
   *     would give 1.0000000000000002, on up-moves. It compares against
   *     the SIGNED numerator, so it is false for every down move: it
   *     bounds nothing on its own, which is what the clamp is for.
   *   - a genuinely flat window is recognized by COUNTING exactly-zero
   *     one-bar changes (nullRun >= P forces sumROC1 to 0.0, purging the
   *     running sum's rounding residue, and the ratio to 1.0) -- never
   *     NaN (#112). This is kama.c's #253 form; the
   *     absolute TA_IS_ZERO band it replaced fails the QUOTE-UNIT/SCALE
   *     gate (ER is homogeneous of degree 0, and a fixed 1e-14 met a
   *     price-carrying sum).
   *
   * The third is the denominator test (#385): a subtract-then-add sum can
   * reach 0.0, or below it, on a window that is not flat. The clamp
   * subsumes it numerically, so it is held by the structural sweep over
   * divisors, not by any value test.
   *
   * The subtract-then-add update order matches TA_SUM's recurrence,
   * which is what makes the composite differential bit-exact. The
   * trailing value is cached one iteration ahead, which is what keeps
   * outReal == inReal aliasing safe.
   */
  lookbackTotal = erLookback(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Prime the path sum over the optInTimePeriod one-bar changes ending
   * at the first output bar's predecessor.
   */
  sumROC1 = 0.0;
  nullRun = 0;
  today = startIdx - lookbackTotal;
  trailingIdx = today;
  i = optInTimePeriod;
  while (i-- > 0) {
    tempReal = inReal[today++];
    tempReal -= inReal[today];
    sumROC1 += Math.abs(tempReal);
    live = (!(Math.abs(tempReal) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
  }
  /* First output: today == startIdx. */
  tempReal = inReal[today];
  tempReal2 = inReal[trailingIdx++];
  periodROC = tempReal - tempReal2;
  trailingValue = tempReal2;
  /* A fully flat priming window sums to an exact 0.0 (no residue yet), so
   * `0 <= 0` already answers 1.0 here without the nullRun purge. The
   * denominator test below is unreachable at this site -- a priming sum only
   * ever has non-negative terms added to it -- and is written anyway so both
   * sites read as one rule.
   */
  if (sumROC1 <= 0.0 || sumROC1 <= periodROC) {
    outReal[((0 * outStride) | 0)] = 1.0;
  } else {
    tempReal = Math.abs(periodROC / sumROC1);
    if (tempReal > 1.0) {
      tempReal = 1.0;
    }
    outReal[((0 * outStride) | 0)] = tempReal;
  }
  outIdx = 1;
  today += 1;
  while (today <= endIdx) {
    tempReal = inReal[today];
    tempReal2 = inReal[trailingIdx++];
    periodROC = tempReal - tempReal2;
    /* Subtract-then-add, TA_SUM's own order. */
    sumROC1 -= Math.abs(trailingValue - tempReal2);
    sumROC1 += Math.abs(tempReal - inReal[today - 1]);
    /* Once a whole window of one-bar changes is exactly zero, the sum's
     * only content is rounding residue from the subtract/add carry --
     * purge it so the flat window is recognized exactly (kama.c #253).
     */
    live = (!(Math.abs(tempReal - inReal[today - 1]) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
    if (nullRun >= optInTimePeriod) {
      nullRun = optInTimePeriod;
      sumROC1 = 0.0;
      outReal[((outIdx++ * outStride) | 0)] = 1.0;
    } else if (sumROC1 <= 0.0 || sumROC1 <= periodROC) {
      outReal[((outIdx++ * outStride) | 0)] = 1.0;
    } else {
      tempReal = Math.abs(periodROC / sumROC1);
      if (tempReal > 1.0) {
        tempReal = 1.0;
      }
      outReal[((outIdx++ * outStride) | 0)] = tempReal;
    }
    /* Save the trailing value: outReal may alias inReal, and the next
     * iteration's subtraction needs the ORIGINAL bar, not the slot the
     * write above may have clobbered.
     */
    trailingValue = tempReal2;
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
  sp.sumROC1 = sumROC1;
  sp.trailingValue = trailingValue;
  sp.lag1_inReal = inReal[historyLen - 1];
  sp.ringPos_trailingIdx = 0;
  sp.ringCap_trailingIdx = cap_trailingIdx;
  sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function erOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): ErStream {
  const sp = ErStream.of(core);
  const retCode = erOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ER openAndFill', inReal.length, startIdx, erLookback(optInTimePeriod, core));
  }
  throw streamFailure('ER openAndFill', retCode);
}

/** @internal */
export function erOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): ErStream {
  const sp = ErStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = erOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ER open', inReal.length, startIdx, erLookback(optInTimePeriod, core));
  }
  throw streamFailure('ER open', retCode);
}

/**
 * Opens a live ER stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link er} there.
 *
 * @param inReal - Warm-up history. Source price/value series (canonically close)
 * @param optInTimePeriod - Number of one-bar changes in the path sum (`KAMA`'s
 *   `optInTimePeriod` is the same window, under its own default) (default 10; range
 *   2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `erLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty one,
 *   and `TA_BAD_PARAM` for an integer parameter that is not an int32 or a parameter
 *   outside its range.
 *
 * @example
 * ```ts
 * import { erOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = erOpen(data, 10);
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function erOpen(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): ErStream {
  optInTimePeriod = requireInt32('ER open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('ER open', 'inReal', inReal);
  requireHistory('ER open', inReal.length);
  return erOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/**
 * {@link erOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link er}. The range written is the stream's
 * `outRange()`.
 *
 * @param inReal - Warm-up history. Source price/value series (canonically close)
 * @param optInTimePeriod - Number of one-bar changes in the path sum (`KAMA`'s
 *   `optInTimePeriod` is the same window, under its own default) (default 10; range
 *   2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Efficiency ratio. Must hold `historyLen - lookback` values and
 *   share no memory with an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `erLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty one,
 *   and `TA_BAD_PARAM` for an integer parameter that is not an int32, a parameter
 *   outside its range or an output too short or starting where another output or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { erOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const stream = erOpenAndFill(data, 10, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function erOpenAndFill(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): ErStream {
  optInTimePeriod = requireInt32('ER openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('ER openAndFill', 'inReal', inReal);
  requireHistory('ER openAndFill', inReal.length);
  const guardOutLen = openFillCount('ER openAndFill', inReal.length, erLookback(optInTimePeriod, core));
  requireOutput('ER openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if ((outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset)) {
    throw streamFailure('ER openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inRealF64))) {
    throw streamFailure('ER openAndFill', RetCode.BAD_PARAM);
  }
  return erOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}
