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
 *  090526 KL     First version (issue #362).
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link coppock} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param optInWMAPeriod - Smoothing period for the ROC sum (default 10; range 1 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInROC1Period - Short rate-of-change period (default 11; range 1 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInROC2Period - Long rate-of-change period (default 14; range 1 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function coppockLookback(optInWMAPeriod: number, optInROC1Period: number, optInROC2Period: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((optInWMAPeriod | 0) !== optInWMAPeriod) {
    return -1;
  }
  if ((optInROC1Period | 0) !== optInROC1Period) {
    return -1;
  }
  if ((optInROC2Period | 0) !== optInROC2Period) {
    return -1;
  }
  if (optInWMAPeriod === Core.INTEGER_DEFAULT) {
    optInWMAPeriod = 10;
  } else if (optInWMAPeriod < 1 || optInWMAPeriod > 100000) {
    return -1;
  }
  if (optInROC1Period === Core.INTEGER_DEFAULT) {
    optInROC1Period = 11;
  } else if (optInROC1Period < 1 || optInROC1Period > 100000) {
    return -1;
  }
  if (optInROC2Period === Core.INTEGER_DEFAULT) {
    optInROC2Period = 14;
  } else if (optInROC2Period < 1 || optInROC2Period > 100000) {
    return -1;
  }
  /* The ROC sum needs max(p1,p2) prior bars before it exists at all, and the
   * WMA needs optInWMAPeriod values of that sum -> the first output sits at
   * max(p1,p2) + optInWMAPeriod - 1. The lookback keys off the MAX, which is
   * why optInROC1Period > optInROC2Period is accepted rather than rejected:
   * the formula is symmetric in the two ROCs (issue #362, decision 2).
   */
  if (optInROC1Period > optInROC2Period) {
    return optInROC1Period + optInWMAPeriod - 1;
  }
  return optInROC2Period + optInWMAPeriod - 1;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link coppock}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInWMAPeriod - Smoothing period for the ROC sum (default 10; range 1 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInROC1Period - Short rate-of-change period (default 11; range 1 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInROC2Period - Long rate-of-change period (default 14; range 1 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function coppockDisplayShift(optInWMAPeriod: number, optInROC1Period: number, optInROC2Period: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (coppockLookback(optInWMAPeriod, optInROC1Period, optInROC2Period, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function coppockImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInWMAPeriod: number,
  optInROC1Period: number,
  optInROC2Period: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let outIdx = 0;
  let inIdx = 0;
  let lookbackTotal = 0;
  let i = 0;
  let q = 0;
  let rw = 0;
  let ringWalk = 0;
  let ringSize = 0;
  let barsSinceReseed = 0;
  let roc1Idx = 0;
  let roc2Idx = 0;
  let periodSum = 0;
  let periodSub = 0;
  let tempReal = 0;
  let tempReal2 = 0;
  let trailingValue = 0;
  let divider = 0;
  let base1 = 0;
  let base2 = 0;
  let roc1 = 0;
  let roc2 = 0;
  let sRing!: Float64Array;
  let sRing_Idx = 0;
  let maxIdx_sRing = (50) - 1;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInWMAPeriod === Core.INTEGER_DEFAULT) {
    optInWMAPeriod = 10;
  } else if (optInWMAPeriod < 1 || optInWMAPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInROC1Period === Core.INTEGER_DEFAULT) {
    optInROC1Period = 11;
  } else if (optInROC1Period < 1 || optInROC1Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInROC2Period === Core.INTEGER_DEFAULT) {
    optInROC2Period = 14;
  } else if (optInROC2Period < 1 || optInROC2Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  /* Coppock Curve: a WMA(optInWMAPeriod) of the SUM of two rates of change,
   * ROC(optInROC1Period) + ROC(optInROC2Period). The sum, not the mean:
   * every published definition sums them; Tulip's beta/copp.c averages and
   * therefore reads at exactly half this amplitude. A clean 2.000000x ratio
   * against Tulip is Tulip's variant, not a defect here (issue #362).
   *
   * The smoothed series is the inline expression
   *    S(j) = R(j,p1) + R(j,p2)
   *    R(j,p) = (inReal[j-p] != 0.0) ? ((inReal[j]/inReal[j-p])-1.0)*100.0
   *                                  : 0.0
   * -- TA_ROC's own zero guard included, so a zero price yields 0.0 exactly
   * where TA_ROC yields 0.0, never an inf that pollutes the window. Each
   * lagged denominator goes through its own trailing cursor advanced in
   * lock-step (TA_ROC's own shape) -- a parameter-sized lag subscript is
   * outside the stream classifier's index grammar.
   *
   * The WMA stage reproduces TA_WMA's recurrence verbatim -- the triangle
   * divider computed in double (#142), the periodSum/periodSub carry and the
   * 8*w re-anchor (#254) -- because anything short of verbatim breaks the
   * bit-exact composite differential against TA_ROC + TA_ROC + TA_WMA. S is
   * a DERIVED series that is never materialised, so, exactly as in TA_HMA's
   * outer stage, each S value is computed once and carried in a
   * (optInWMAPeriod-1)-slot ring: the trailing subtraction reads the
   * expiring slot and the re-anchor walks the ring, oldest first, weight
   * counting up from 1, then adds the current bar at weight w.
   */
  lookbackTotal = coppockLookback(optInWMAPeriod, optInROC1Period, optInROC2Period, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  /* Triangle divider in double: the int product w*(w+1) overflows int32 at
   * w >= 46341 (#142), exactly as in TA_WMA.
   */
  divider = optInWMAPeriod * (optInWMAPeriod + 1) / 2.0;
  outIdx = 0;
  /* The S value computed at bar t expires optInWMAPeriod-1 bars later, so a
   * single-cursor ring of optInWMAPeriod-1 slots is enough: read the
   * expiring value, overwrite the slot, advance. At w == 1 the recurrence's
   * state is never consumed (see the identity output below), but the ring
   * still needs one slot for the reads to stay in bounds.
   */
  ringSize = optInWMAPeriod - 1;
  if (ringSize < 1) {
    ringSize = 1;
  }
  if (ringSize < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sRing = new Float64Array(ringSize);
  maxIdx_sRing = ringSize - 1;
  sRing_Idx = 0;
  /* At w == 1 the priming loop below never runs, so the first trailing read
   * would see an undefined slot; at w > 1 priming overwrites every slot.
   */
  sRing[0] = 0.0;
  /* One trailing cursor per ROC denominator, advanced in lock-step from the
   * priming scan onward.
   */
  inIdx = startIdx - (optInWMAPeriod - 1);
  roc1Idx = inIdx - optInROC1Period;
  roc2Idx = inIdx - optInROC2Period;
  /* Priming: the w-1 S values before the first output, oldest first with
   * the weight counting up from 1 -- TA_WMA's own priming order, which the
   * re-anchor below must reproduce. They also fill the ring.
   */
  periodSub = 0.0;
  periodSum = periodSub;
  i = 1;
  while (inIdx < startIdx) {
    base1 = inReal[roc1Idx];
    roc1Idx += 1;
    base2 = inReal[roc2Idx];
    roc2Idx += 1;
    roc1 = (base1 !== 0.0) ? (inReal[inIdx] / base1 - 1.0) * 100.0 : 0.0;
    roc2 = (base2 !== 0.0) ? (inReal[inIdx] / base2 - 1.0) * 100.0 : 0.0;
    tempReal = roc1 + roc2;
    periodSub += tempReal;
    periodSum += tempReal * i;
    i += 1;
    sRing[sRing_Idx] = tempReal;
    sRing_Idx++;
    if (sRing_Idx > maxIdx_sRing) {
      sRing_Idx = 0;
    }
    inIdx += 1;
  }
  barsSinceReseed = ((8 * optInWMAPeriod) | 0);
  trailingValue = 0.0;
  /* Tight loop for the requested range. */
  while (inIdx <= endIdx) {
    base1 = inReal[roc1Idx];
    roc1Idx += 1;
    base2 = inReal[roc2Idx];
    roc2Idx += 1;
    roc1 = (base1 !== 0.0) ? (inReal[inIdx] / base1 - 1.0) * 100.0 : 0.0;
    roc2 = (base2 !== 0.0) ? (inReal[inIdx] / base2 - 1.0) * 100.0 : 0.0;
    tempReal = roc1 + roc2;
    periodSub += tempReal;
    periodSub -= trailingValue;
    periodSum += tempReal * optInWMAPeriod;
    /* Re-anchor every 8*w bars: rebuild both totals from the window
     * itself -- TA_WMA's #254 fix, same single trigger, same interval.
     * The window lives in the ring (w-1 prior S values, sRing_Idx the
     * oldest) plus the current bar's tempReal at weight w.
     */
    barsSinceReseed -= 1;
    if (barsSinceReseed <= 0) {
      barsSinceReseed = ((8 * optInWMAPeriod) | 0);
      periodSub = 0.0;
      periodSum = 0.0;
      rw = 1;
      ringWalk = sRing_Idx;
      for (q = 0; q < ringSize; q += 1) {
        tempReal2 = sRing[ringWalk];
        periodSub += tempReal2;
        periodSum += tempReal2 * rw;
        rw += 1;
        ringWalk += 1;
        if (ringWalk >= ringSize) {
          ringWalk = 0;
        }
      }
      periodSub += tempReal;
      periodSum += tempReal * optInWMAPeriod;
    }
    /* Read the expiring S value BEFORE overwriting its slot with the
     * current one -- the ring is the aliasing-safe stand-in for TA_WMA's
     * "save the trailing value before the store" rule.
     */
    trailingValue = sRing[sRing_Idx];
    sRing[sRing_Idx] = tempReal;
    sRing_Idx++;
    if (sRing_Idx > maxIdx_sRing) {
      sRing_Idx = 0;
    }
    /* Load-bearing, not a rounding nicety: keep it. WMA(1) is the identity
     * and TA_WMA ships an exact copy fast path, but the recurrence here is
     * off by a whole term at w == 1 -- ringSize clamps to 1, so the
     * read-before-write ring hands back the wrong trailing value. Deleting
     * this arm moves TA_SREF bar 16 at (1,11,14) from -11.311839169954585
     * to -6.4591709868291103.
     */
    if (optInWMAPeriod === 1) {
      outReal[outIdx] = tempReal;
    } else {
      outReal[outIdx] = periodSum / divider;
    }
    outIdx += 1;
    periodSum -= periodSub;
    inIdx += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * Coppock Curve: Edwin S. "Sedge" Coppock's long-term momentum oscillator
 * (*Barron's*, originally published as the "Trendex Model"), computed as a
 * weighted moving average of the **sum** of two rates of change. Unbounded;
 * positive turns from below zero are the signal the indicator was designed for
 * (long-term buying opportunities on monthly index data).
 *
 * Formula and more info at
 * [ta-lib.org/functions/coppock](https://ta-lib.org/functions/coppock).
 *
 * **Notes**
 * - **The input is assumed positive.** The price each rate of change looks back to
 *   is its divisor. Near zero the divisor is small and the output very large; no
 *   error is returned. A series that reaches or crosses zero, such as an
 *   oscillator or a spread, has no meaningful result here.
 * - The single fused pass is bit-identical to running `ROC + ROC` into
 *   [`WMA`](https://ta-lib.org/functions/wma).
 * - First output at `max(optInROC1Period, optInROC2Period) + optInWMAPeriod - 1`.
 *   Not start-dependent: each output depends only on its finite trailing window.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link coppockLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - Source price/value series (canonically a monthly close)
 * @param optInWMAPeriod - Smoothing period for the ROC sum (default 10; range 1 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInROC1Period - Short rate-of-change period (default 11; range 1 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInROC2Period - Long rate-of-change period (default 14; range 1 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Coppock Curve value. Must hold the values produced and never be
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
 * import { coppock } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const { begIdx, count } = coppock(0, data.length - 1, data, 10, 11, 14, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function coppock(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInWMAPeriod: number,
  optInROC1Period: number,
  optInROC2Period: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('COPPOCK', 'startIdx', startIdx);
  endIdx = requireInt32('COPPOCK', 'endIdx', endIdx);
  optInWMAPeriod = requireInt32('COPPOCK', 'optInWMAPeriod', optInWMAPeriod);
  optInROC1Period = requireInt32('COPPOCK', 'optInROC1Period', optInROC1Period);
  optInROC2Period = requireInt32('COPPOCK', 'optInROC2Period', optInROC2Period);
  requireIndexRange('COPPOCK', startIdx, endIdx);
  const guardStart = clampedStart('COPPOCK', startIdx, coppockLookback(optInWMAPeriod, optInROC1Period, optInROC2Period, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('COPPOCK', 'inReal', inReal, guardInLen);
  requireOutput('COPPOCK', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw failure('COPPOCK', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = coppockImpl(core, startIdx, endIdx, inRealF64, optInWMAPeriod, optInROC1Period, optInROC2Period, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('COPPOCK', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live COPPOCK stream: one value per closed bar, bit-identical to
 * {@link coppock} over the same series. Open it with {@link coppockOpen} or
 * {@link coppockOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/coppock](https://ta-lib.org/functions/coppock).
 */
export class CoppockStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInWMAPeriod: number = 0;
  /** @internal */
  optInROC1Period: number = 0;
  /** @internal */
  optInROC2Period: number = 0;
  /** @internal */
  ringSize: number = 0;
  /** @internal */
  barsSinceReseed: number = 0;
  /** @internal */
  periodSum: number = 0;
  /** @internal */
  periodSub: number = 0;
  /** @internal */
  trailingValue: number = 0;
  /** @internal */
  divider: number = 0;
  /** @internal */
  sRing_Idx: number = 0;
  /** @internal */
  maxIdx_sRing: number = 0;
  /** @internal */
  ringPos_roc1Idx: number = 0;
  /** @internal */
  ringCap_roc1Idx: number = 0;
  /** @internal */
  ring_roc1Idx_inReal: Float64Array = new Float64Array(1);
  /** @internal */
  ringPos_roc2Idx: number = 0;
  /** @internal */
  ringCap_roc2Idx: number = 0;
  /** @internal */
  ring_roc2Idx_inReal: Float64Array = new Float64Array(1);
  /** @internal */
  cbSize_sRing: number = 0;
  /** @internal */
  cb_sRing: Float64Array = new Float64Array(1);
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
  static of(core: Core): CoppockStream {
    return new CoppockStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link coppock} reports them over the same bars. Each accepted `update` and each
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
      throw failure('COPPOCK advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: Source price/value series (canonically
   *   a monthly close)
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('COPPOCK update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('COPPOCK update', 'inReal');
    }
    coppockStepImpl(this, inReal);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Source price/value series (canonically
   *   a monthly close)
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number): number {
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('COPPOCK peek', 'inReal');
    }
    const sp = this;
    let q = 0;
    let rw = 0;
    let ringWalk = 0;
    let tempReal = 0;
    let tempReal2 = 0;
    let base1 = 0;
    let base2 = 0;
    let roc1 = 0;
    let roc2 = 0;
    let barsSinceReseed = sp.barsSinceReseed;
    let cur_outReal = 0;
    let periodSum = sp.periodSum;
    let sRing_Idx = sp.sRing_Idx;
    base1 = sp.ring_roc1Idx_inReal[sp.ringPos_roc1Idx];
    base2 = sp.ring_roc2Idx_inReal[sp.ringPos_roc2Idx];
    roc1 = (base1 !== 0.0) ? (inReal / base1 - 1.0) * 100.0 : 0.0;
    roc2 = (base2 !== 0.0) ? (inReal / base2 - 1.0) * 100.0 : 0.0;
    tempReal = roc1 + roc2;
    periodSum += tempReal * sp.optInWMAPeriod;
    /* Re-anchor every 8*w bars: rebuild both totals from the window
     * itself -- TA_WMA's #254 fix, same single trigger, same interval.
     * The window lives in the ring (w-1 prior S values, sRing_Idx the
     * oldest) plus the current bar's tempReal at weight w.
     */
    barsSinceReseed -= 1;
    if (barsSinceReseed <= 0) {
      barsSinceReseed = ((8 * sp.optInWMAPeriod) | 0);
      periodSum = 0.0;
      rw = 1;
      ringWalk = sRing_Idx;
      for (q = 0; q < sp.ringSize; q += 1) {
        tempReal2 = sp.cb_sRing[ringWalk];
        periodSum += tempReal2 * rw;
        rw += 1;
        ringWalk += 1;
        if (ringWalk >= sp.ringSize) {
          ringWalk = 0;
        }
      }
      periodSum += tempReal * sp.optInWMAPeriod;
    }
    /* Read the expiring S value BEFORE overwriting its slot with the
     * current one -- the ring is the aliasing-safe stand-in for TA_WMA's
     * "save the trailing value before the store" rule.
     */
    sRing_Idx = sRing_Idx + 1;
    if (sRing_Idx > sp.maxIdx_sRing) {
      sRing_Idx = 0;
    }
    /* Load-bearing, not a rounding nicety: keep it. WMA(1) is the identity
     * and TA_WMA ships an exact copy fast path, but the recurrence here is
     * off by a whole term at w == 1 -- ringSize clamps to 1, so the
     * read-before-write ring hands back the wrong trailing value. Deleting
     * this arm moves TA_SREF bar 16 at (1,11,14) from -11.311839169954585
     * to -6.4591709868291103.
     */
    if (sp.optInWMAPeriod === 1) {
      cur_outReal = tempReal;
    } else {
      cur_outReal = periodSum / sp.divider;
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
  clone(): CoppockStream {
    const c = new CoppockStream(this.core);
    c.optInWMAPeriod = this.optInWMAPeriod;
    c.optInROC1Period = this.optInROC1Period;
    c.optInROC2Period = this.optInROC2Period;
    c.ringSize = this.ringSize;
    c.barsSinceReseed = this.barsSinceReseed;
    c.periodSum = this.periodSum;
    c.periodSub = this.periodSub;
    c.trailingValue = this.trailingValue;
    c.divider = this.divider;
    c.sRing_Idx = this.sRing_Idx;
    c.maxIdx_sRing = this.maxIdx_sRing;
    c.ringPos_roc1Idx = this.ringPos_roc1Idx;
    c.ringCap_roc1Idx = this.ringCap_roc1Idx;
    c.ring_roc1Idx_inReal = this.ring_roc1Idx_inReal.slice();
    c.ringPos_roc2Idx = this.ringPos_roc2Idx;
    c.ringCap_roc2Idx = this.ringCap_roc2Idx;
    c.ring_roc2Idx_inReal = this.ring_roc2Idx_inReal.slice();
    c.cbSize_sRing = this.cbSize_sRing;
    c.cb_sRing = this.cb_sRing.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function coppockStepImpl(sp: CoppockStream, inReal: number): void {
  let q = 0;
  let rw = 0;
  let ringWalk = 0;
  let tempReal = 0;
  let tempReal2 = 0;
  let base1 = 0;
  let base2 = 0;
  let roc1 = 0;
  let roc2 = 0;
  let ringCapL_roc1Idx = 0;
  let ringCapL_roc2Idx = 0;
  base1 = sp.ring_roc1Idx_inReal[sp.ringPos_roc1Idx];
  base2 = sp.ring_roc2Idx_inReal[sp.ringPos_roc2Idx];
  roc1 = (base1 !== 0.0) ? (inReal / base1 - 1.0) * 100.0 : 0.0;
  roc2 = (base2 !== 0.0) ? (inReal / base2 - 1.0) * 100.0 : 0.0;
  tempReal = roc1 + roc2;
  sp.periodSub += tempReal;
  sp.periodSub -= sp.trailingValue;
  sp.periodSum += tempReal * sp.optInWMAPeriod;
  /* Re-anchor every 8*w bars: rebuild both totals from the window
   * itself -- TA_WMA's #254 fix, same single trigger, same interval.
   * The window lives in the ring (w-1 prior S values, sRing_Idx the
   * oldest) plus the current bar's tempReal at weight w.
   */
  sp.barsSinceReseed -= 1;
  if (sp.barsSinceReseed <= 0) {
    sp.barsSinceReseed = ((8 * sp.optInWMAPeriod) | 0);
    sp.periodSub = 0.0;
    sp.periodSum = 0.0;
    rw = 1;
    ringWalk = sp.sRing_Idx;
    for (q = 0; q < sp.ringSize; q += 1) {
      tempReal2 = sp.cb_sRing[ringWalk];
      sp.periodSub += tempReal2;
      sp.periodSum += tempReal2 * rw;
      rw += 1;
      ringWalk += 1;
      if (ringWalk >= sp.ringSize) {
        ringWalk = 0;
      }
    }
    sp.periodSub += tempReal;
    sp.periodSum += tempReal * sp.optInWMAPeriod;
  }
  /* Read the expiring S value BEFORE overwriting its slot with the
   * current one -- the ring is the aliasing-safe stand-in for TA_WMA's
   * "save the trailing value before the store" rule.
   */
  sp.trailingValue = sp.cb_sRing[sp.sRing_Idx];
  sp.cb_sRing[sp.sRing_Idx] = tempReal;
  sp.sRing_Idx = sp.sRing_Idx + 1;
  if (sp.sRing_Idx > sp.maxIdx_sRing) {
    sp.sRing_Idx = 0;
  }
  /* Load-bearing, not a rounding nicety: keep it. WMA(1) is the identity
   * and TA_WMA ships an exact copy fast path, but the recurrence here is
   * off by a whole term at w == 1 -- ringSize clamps to 1, so the
   * read-before-write ring hands back the wrong trailing value. Deleting
   * this arm moves TA_SREF bar 16 at (1,11,14) from -11.311839169954585
   * to -6.4591709868291103.
   */
  if (sp.optInWMAPeriod === 1) {
    sp.cur_outReal = tempReal;
  } else {
    sp.cur_outReal = sp.periodSum / sp.divider;
  }
  sp.periodSum -= sp.periodSub;
  ringCapL_roc1Idx = sp.ringCap_roc1Idx;
  sp.ring_roc1Idx_inReal[sp.ringPos_roc1Idx] = inReal;
  sp.ringPos_roc1Idx = sp.ringPos_roc1Idx + 1;
  if (sp.ringPos_roc1Idx >= ringCapL_roc1Idx) {
    sp.ringPos_roc1Idx = 0;
  }
  ringCapL_roc2Idx = sp.ringCap_roc2Idx;
  sp.ring_roc2Idx_inReal[sp.ringPos_roc2Idx] = inReal;
  sp.ringPos_roc2Idx = sp.ringPos_roc2Idx + 1;
  if (sp.ringPos_roc2Idx >= ringCapL_roc2Idx) {
    sp.ringPos_roc2Idx = 0;
  }
}

function coppockOpenImpl(
  sp: CoppockStream,
  inReal: Float64Array,
  startIdx: number,
  optInWMAPeriod: number,
  optInROC1Period: number,
  optInROC2Period: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let outIdx = 0;
  let inIdx = 0;
  let lookbackTotal = 0;
  let i = 0;
  let q = 0;
  let rw = 0;
  let ringWalk = 0;
  let ringSize = 0;
  let barsSinceReseed = 0;
  let roc1Idx = 0;
  let roc2Idx = 0;
  let periodSum = 0;
  let periodSub = 0;
  let tempReal = 0;
  let tempReal2 = 0;
  let trailingValue = 0;
  let divider = 0;
  let base1 = 0;
  let base2 = 0;
  let roc1 = 0;
  let roc2 = 0;
  let sRing!: Float64Array;
  let sRing_Idx = 0;
  let maxIdx_sRing = (50) - 1;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInWMAPeriod === Core.INTEGER_DEFAULT) {
    optInWMAPeriod = 10;
  } else if (optInWMAPeriod < 1 || optInWMAPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInROC1Period === Core.INTEGER_DEFAULT) {
    optInROC1Period = 11;
  } else if (optInROC1Period < 1 || optInROC1Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInROC2Period === Core.INTEGER_DEFAULT) {
    optInROC2Period = 14;
  } else if (optInROC2Period < 1 || optInROC2Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Coppock Curve: a WMA(optInWMAPeriod) of the SUM of two rates of change,
   * ROC(optInROC1Period) + ROC(optInROC2Period). The sum, not the mean:
   * every published definition sums them; Tulip's beta/copp.c averages and
   * therefore reads at exactly half this amplitude. A clean 2.000000x ratio
   * against Tulip is Tulip's variant, not a defect here (issue #362).
   *
   * The smoothed series is the inline expression
   *    S(j) = R(j,p1) + R(j,p2)
   *    R(j,p) = (inReal[j-p] != 0.0) ? ((inReal[j]/inReal[j-p])-1.0)*100.0
   *                                  : 0.0
   * -- TA_ROC's own zero guard included, so a zero price yields 0.0 exactly
   * where TA_ROC yields 0.0, never an inf that pollutes the window. Each
   * lagged denominator goes through its own trailing cursor advanced in
   * lock-step (TA_ROC's own shape) -- a parameter-sized lag subscript is
   * outside the stream classifier's index grammar.
   *
   * The WMA stage reproduces TA_WMA's recurrence verbatim -- the triangle
   * divider computed in double (#142), the periodSum/periodSub carry and the
   * 8*w re-anchor (#254) -- because anything short of verbatim breaks the
   * bit-exact composite differential against TA_ROC + TA_ROC + TA_WMA. S is
   * a DERIVED series that is never materialised, so, exactly as in TA_HMA's
   * outer stage, each S value is computed once and carried in a
   * (optInWMAPeriod-1)-slot ring: the trailing subtraction reads the
   * expiring slot and the re-anchor walks the ring, oldest first, weight
   * counting up from 1, then adds the current bar at weight w.
   */
  lookbackTotal = coppockLookback(optInWMAPeriod, optInROC1Period, optInROC2Period, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Triangle divider in double: the int product w*(w+1) overflows int32 at
   * w >= 46341 (#142), exactly as in TA_WMA.
   */
  divider = optInWMAPeriod * (optInWMAPeriod + 1) / 2.0;
  outIdx = 0;
  /* The S value computed at bar t expires optInWMAPeriod-1 bars later, so a
   * single-cursor ring of optInWMAPeriod-1 slots is enough: read the
   * expiring value, overwrite the slot, advance. At w == 1 the recurrence's
   * state is never consumed (see the identity output below), but the ring
   * still needs one slot for the reads to stay in bounds.
   */
  ringSize = optInWMAPeriod - 1;
  if (ringSize < 1) {
    ringSize = 1;
  }
  if (ringSize < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sRing = new Float64Array(ringSize);
  maxIdx_sRing = ringSize - 1;
  sRing_Idx = 0;
  /* At w == 1 the priming loop below never runs, so the first trailing read
   * would see an undefined slot; at w > 1 priming overwrites every slot.
   */
  sRing[0] = 0.0;
  /* One trailing cursor per ROC denominator, advanced in lock-step from the
   * priming scan onward.
   */
  inIdx = startIdx - (optInWMAPeriod - 1);
  roc1Idx = inIdx - optInROC1Period;
  roc2Idx = inIdx - optInROC2Period;
  /* Priming: the w-1 S values before the first output, oldest first with
   * the weight counting up from 1 -- TA_WMA's own priming order, which the
   * re-anchor below must reproduce. They also fill the ring.
   */
  periodSub = 0.0;
  periodSum = periodSub;
  i = 1;
  while (inIdx < startIdx) {
    base1 = inReal[roc1Idx];
    roc1Idx += 1;
    base2 = inReal[roc2Idx];
    roc2Idx += 1;
    roc1 = (base1 !== 0.0) ? (inReal[inIdx] / base1 - 1.0) * 100.0 : 0.0;
    roc2 = (base2 !== 0.0) ? (inReal[inIdx] / base2 - 1.0) * 100.0 : 0.0;
    tempReal = roc1 + roc2;
    periodSub += tempReal;
    periodSum += tempReal * i;
    i += 1;
    sRing[sRing_Idx] = tempReal;
    sRing_Idx++;
    if (sRing_Idx > maxIdx_sRing) {
      sRing_Idx = 0;
    }
    inIdx += 1;
  }
  barsSinceReseed = ((8 * optInWMAPeriod) | 0);
  trailingValue = 0.0;
  /* Tight loop for the requested range. */
  while (inIdx <= endIdx) {
    base1 = inReal[roc1Idx];
    roc1Idx += 1;
    base2 = inReal[roc2Idx];
    roc2Idx += 1;
    roc1 = (base1 !== 0.0) ? (inReal[inIdx] / base1 - 1.0) * 100.0 : 0.0;
    roc2 = (base2 !== 0.0) ? (inReal[inIdx] / base2 - 1.0) * 100.0 : 0.0;
    tempReal = roc1 + roc2;
    periodSub += tempReal;
    periodSub -= trailingValue;
    periodSum += tempReal * optInWMAPeriod;
    /* Re-anchor every 8*w bars: rebuild both totals from the window
     * itself -- TA_WMA's #254 fix, same single trigger, same interval.
     * The window lives in the ring (w-1 prior S values, sRing_Idx the
     * oldest) plus the current bar's tempReal at weight w.
     */
    barsSinceReseed -= 1;
    if (barsSinceReseed <= 0) {
      barsSinceReseed = ((8 * optInWMAPeriod) | 0);
      periodSub = 0.0;
      periodSum = 0.0;
      rw = 1;
      ringWalk = sRing_Idx;
      for (q = 0; q < ringSize; q += 1) {
        tempReal2 = sRing[ringWalk];
        periodSub += tempReal2;
        periodSum += tempReal2 * rw;
        rw += 1;
        ringWalk += 1;
        if (ringWalk >= ringSize) {
          ringWalk = 0;
        }
      }
      periodSub += tempReal;
      periodSum += tempReal * optInWMAPeriod;
    }
    /* Read the expiring S value BEFORE overwriting its slot with the
     * current one -- the ring is the aliasing-safe stand-in for TA_WMA's
     * "save the trailing value before the store" rule.
     */
    trailingValue = sRing[sRing_Idx];
    sRing[sRing_Idx] = tempReal;
    sRing_Idx++;
    if (sRing_Idx > maxIdx_sRing) {
      sRing_Idx = 0;
    }
    /* Load-bearing, not a rounding nicety: keep it. WMA(1) is the identity
     * and TA_WMA ships an exact copy fast path, but the recurrence here is
     * off by a whole term at w == 1 -- ringSize clamps to 1, so the
     * read-before-write ring hands back the wrong trailing value. Deleting
     * this arm moves TA_SREF bar 16 at (1,11,14) from -11.311839169954585
     * to -6.4591709868291103.
     */
    if (optInWMAPeriod === 1) {
      outReal[((outIdx * outStride) | 0)] = tempReal;
    } else {
      outReal[((outIdx * outStride) | 0)] = periodSum / divider;
    }
    outIdx += 1;
    periodSum -= periodSub;
    inIdx += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  const cap_roc1Idx = inIdx - roc1Idx;
  if (cap_roc1Idx < 1 || cap_roc1Idx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_roc1Idx = cap_roc1Idx > 0 ? cap_roc1Idx : 1;
  const capRing_roc1Idx_inReal = new Float64Array(allocN_roc1Idx);
  capRing_roc1Idx_inReal.set(inReal.subarray(historyLen - cap_roc1Idx, historyLen), 0);
  const cap_roc2Idx = inIdx - roc2Idx;
  if (cap_roc2Idx < 1 || cap_roc2Idx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_roc2Idx = cap_roc2Idx > 0 ? cap_roc2Idx : 1;
  const capRing_roc2Idx_inReal = new Float64Array(allocN_roc2Idx);
  capRing_roc2Idx_inReal.set(inReal.subarray(historyLen - cap_roc2Idx, historyLen), 0);
  const capCb_sRing = maxIdx_sRing + 1;
  if (capCb_sRing > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sp.optInWMAPeriod = optInWMAPeriod;
  sp.optInROC1Period = optInROC1Period;
  sp.optInROC2Period = optInROC2Period;
  sp.ringSize = ringSize;
  sp.barsSinceReseed = barsSinceReseed;
  sp.periodSum = periodSum;
  sp.periodSub = periodSub;
  sp.trailingValue = trailingValue;
  sp.divider = divider;
  sp.sRing_Idx = sRing_Idx;
  sp.maxIdx_sRing = maxIdx_sRing;
  sp.ringPos_roc1Idx = 0;
  sp.ringCap_roc1Idx = cap_roc1Idx;
  sp.ring_roc1Idx_inReal = capRing_roc1Idx_inReal;
  sp.ringPos_roc2Idx = 0;
  sp.ringCap_roc2Idx = cap_roc2Idx;
  sp.ring_roc2Idx_inReal = capRing_roc2Idx_inReal;
  sp.cbSize_sRing = capCb_sRing;
  sp.cb_sRing = sRing;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function coppockOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInWMAPeriod: number,
  optInROC1Period: number,
  optInROC2Period: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): CoppockStream {
  const sp = CoppockStream.of(core);
  const retCode = coppockOpenImpl(sp, inReal, startIdx, optInWMAPeriod, optInROC1Period, optInROC2Period, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('COPPOCK openAndFill', inReal.length, startIdx, coppockLookback(optInWMAPeriod, optInROC1Period, optInROC2Period, core));
  }
  throw streamFailure('COPPOCK openAndFill', retCode);
}

/** @internal */
export function coppockOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInWMAPeriod: number,
  optInROC1Period: number,
  optInROC2Period: number,
): CoppockStream {
  const sp = CoppockStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = coppockOpenImpl(sp, inReal, startIdx, optInWMAPeriod, optInROC1Period, optInROC2Period, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('COPPOCK open', inReal.length, startIdx, coppockLookback(optInWMAPeriod, optInROC1Period, optInROC2Period, core));
  }
  throw streamFailure('COPPOCK open', retCode);
}

/**
 * Opens a live COPPOCK stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link coppock} there.
 *
 * @param inReal - Warm-up history. Source price/value series (canonically a
 *   monthly close)
 * @param optInWMAPeriod - Smoothing period for the ROC sum (default 10; range 1 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInROC1Period - Short rate-of-change period (default 11; range 1 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInROC2Period - Long rate-of-change period (default 14; range 1 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `coppockLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { coppockOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = coppockOpen(data, 10, 11, 14);
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function coppockOpen(
  inReal: ArrayLike<number>,
  optInWMAPeriod: number,
  optInROC1Period: number,
  optInROC2Period: number,
  core: Core = Core.DEFAULT,
): CoppockStream {
  optInWMAPeriod = requireInt32('COPPOCK open', 'optInWMAPeriod', optInWMAPeriod);
  optInROC1Period = requireInt32('COPPOCK open', 'optInROC1Period', optInROC1Period);
  optInROC2Period = requireInt32('COPPOCK open', 'optInROC2Period', optInROC2Period);
  requireArgument('COPPOCK open', 'inReal', inReal);
  requireHistory('COPPOCK open', inReal.length);
  return coppockOpenInternal(core, float64(inReal), 0, optInWMAPeriod, optInROC1Period, optInROC2Period);
}

/**
 * {@link coppockOpen} that also fills the outputs over the whole history in the
 * same pass, bit-identically to {@link coppock}. The range written is the stream's
 * `outRange()`.
 *
 * @param inReal - Warm-up history. Source price/value series (canonically a
 *   monthly close)
 * @param optInWMAPeriod - Smoothing period for the ROC sum (default 10; range 1 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInROC1Period - Short rate-of-change period (default 11; range 1 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInROC2Period - Long rate-of-change period (default 14; range 1 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Coppock Curve value. Must hold `historyLen - lookback` values
 *   and share no memory with an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `coppockLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for an integer parameter that is not an int32, a parameter outside its range or
 *   an output too short or starting where another output or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { coppockOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const stream = coppockOpenAndFill(data, 10, 11, 14, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function coppockOpenAndFill(
  inReal: ArrayLike<number>,
  optInWMAPeriod: number,
  optInROC1Period: number,
  optInROC2Period: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): CoppockStream {
  optInWMAPeriod = requireInt32('COPPOCK openAndFill', 'optInWMAPeriod', optInWMAPeriod);
  optInROC1Period = requireInt32('COPPOCK openAndFill', 'optInROC1Period', optInROC1Period);
  optInROC2Period = requireInt32('COPPOCK openAndFill', 'optInROC2Period', optInROC2Period);
  requireArgument('COPPOCK openAndFill', 'inReal', inReal);
  requireHistory('COPPOCK openAndFill', inReal.length);
  const guardOutLen = openFillCount('COPPOCK openAndFill', inReal.length, coppockLookback(optInWMAPeriod, optInROC1Period, optInROC2Period, core));
  requireOutput('COPPOCK openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if ((outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset)) {
    throw streamFailure('COPPOCK openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inRealF64))) {
    throw streamFailure('COPPOCK openAndFill', RetCode.BAD_PARAM);
  }
  return coppockOpenAndFillInternal(core, inRealF64, 0, optInWMAPeriod, optInROC1Period, optInROC2Period, new MInteger(), new MInteger(), outReal);
}
