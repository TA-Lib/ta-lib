/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  YV       Yvictor (@Yvictor)
 *  CC       Claude Code (AI assistant)
 *
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  072226 MF,CC  First version (issue #139).
 *  072326 MF,CC  Fused single-pass rewrite: rolling sums + sqrt(n)-sized
 *                CIRCBUF, no whole-range temporaries (issue #139).
 *  080926 MF,CC  Allow period of 1. Just copy input into output.
 *  102926 YV,CC  Weights in double (PR #565).
 *  102926 MF,CC  Steady loop: fewer live values, sqrt(n)-slot ring,
 *                unlikely re-anchors (PR #565).
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { wmaLookback } from './wma.ts';

/**
 * Number of leading input bars {@link hma} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Number of bars in the full-period WMA; the half and
 *   square-root periods derive from it (default 20; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function hmaLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  let sqrtPeriod = 0;
  sqrtPeriod = (Math.sqrt(optInTimePeriod) | 0);
  return wmaLookback(optInTimePeriod, core) + wmaLookback(sqrtPeriod, core);
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link hma}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Number of bars in the full-period WMA; the half and
 *   square-root periods derive from it (default 20; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function hmaDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (hmaLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function hmaImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let lookbackTotal = 0;
  let lookbackSqrt = 0;
  let halfPeriod = 0;
  let sqrtPeriod = 0;
  let ringSize = 0;
  let wmaStartIdx = 0;
  let today = 0;
  let outIdx = 0;
  let i = 0;
  let dividerFull = 0;
  let dividerHalf = 0;
  let dividerSqrt = 0;
  let trailingIdxFull = 0;
  let trailingIdxHalf = 0;
  let periodSubFull = 0;
  let periodSumFull = 0;
  let trailingFull = 0;
  let periodSubHalf = 0;
  let periodSumHalf = 0;
  let trailingHalf = 0;
  let subHalf = 0;
  let periodSubSqrt = 0;
  let periodSumSqrt = 0;
  let tempReal = 0;
  let fullOut = 0;
  let halfOut = 0;
  let diffReal = 0;
  let jFull = 0;
  let jHalf = 0;
  let q = 0;
  let ringWalk = 0;
  let lookbackFull = 0;
  let lookbackHalf = 0;
  let barsSinceReseedFull = 0;
  let barsSinceReseedHalf = 0;
  let barsSinceReseedSqrt = 0;
  let tempReal2 = 0;
  let w = 0;
  let rw = 0;
  let dRing!: Float64Array;
  let dRing_Idx = 0;
  let maxIdx_dRing = (50) - 1;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  /* Keep the weights double: they stay exact (never above the period), and
   * an integer weight slows the Rust loop.
   */
  /* The de-lagged series needs only its last sqrt(n) values, so the whole
   * computation runs in one pass over a single window into the input:
   * three interleaved WMA rolling sums plus this small ring. The ring has
   * sqrt(n) slots: on the stack while that fits the 50-slot prolog
   * (optInTimePeriod <= 2600), TA_Malloc from optInTimePeriod = 2601 up.
   */
  /* Hull Moving Average (Alan Hull, 2005):
   *
   *    HMA(n) = WMA( 2*WMA(price, Integer(n/2)) - WMA(price, n), Integer(SquareRoot(n)) )
   *
   * Both derived periods use the author's Integer() truncation; some other
   * published sources round to nearest instead, which is a visibly different
   * line. See hma.md and issue #139.
   *
   * Each of the three WMAs keeps TA_WMA's exact accumulation order
   * (periodSub/periodSum, trailing subtract), so this fused pass is
   * BIT-IDENTICAL to composing three TA_WMA calls -- the composite
   * differential in test_composite1.c holds it to that, memcmp-exact.
   */
  /* No smoothing at period of 1: the output is a copy of the input
   * (same convention as TA_MA for every MAType). Explicit because the
   * formula has no value there -- Integer(1/2) is 0 -- and the degenerate
   * arm below would leave a cancellation residual instead of a copy.
   */
  if (optInTimePeriod === 1) {
    outBegIdx.value = startIdx;
    outIdx = 0;
    today = startIdx;
    while (today <= endIdx) {
      outReal[outIdx++] = inReal[today++];
    }
    outNBElement.value = outIdx;
    return RetCode.SUCCESS;
  }
  halfPeriod = ((optInTimePeriod / 2) | 0);
  sqrtPeriod = (Math.sqrt(optInTimePeriod) | 0);
  lookbackSqrt = wmaLookback(sqrtPeriod, core);
  lookbackTotal = wmaLookback(optInTimePeriod, core) + lookbackSqrt;
  /* Move up the start index if there is not
   * enough initial data.
   */
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  /* The two price WMAs are anchored where the first de-lagged value is
   * needed: lookbackSqrt bars before the first requested output.
   * wmaStartIdx >= optInTimePeriod-1 is implied by the clamp above.
   */
  wmaStartIdx = startIdx - lookbackSqrt;
  dividerFull = optInTimePeriod * (optInTimePeriod + 1) / 2.0;
  /* Prime the full-period WMA over the optInTimePeriod-1 bars before
   * wmaStartIdx, exactly as TA_WMA does (weights 1..period-1).
   */
  lookbackFull = optInTimePeriod - 1;
  periodSubFull = 0.0;
  periodSumFull = 0.0;
  trailingIdxFull = wmaStartIdx - lookbackFull;
  i = trailingIdxFull;
  w = 1.0;
  while (i < wmaStartIdx) {
    tempReal = inReal[i];
    i += 1;
    periodSubFull += tempReal;
    periodSumFull += tempReal * w;
    w += 1;
  }
  barsSinceReseedFull = ((8 * optInTimePeriod) | 0);
  trailingFull = 0.0;
  outIdx = 0;
  /* sqrtPeriod == 1 exactly when optInTimePeriod is 2 or 3; stated on the
   * param so the stream analyzer sees a param-pure dual-mode split.
   */
  if (optInTimePeriod === 2 || optInTimePeriod === 3) {
    /* Degenerate regime, optInTimePeriod 2 or 3 only: halfPeriod and
     * sqrtPeriod are both 1, and a period-1 WMA is the identity (TA_WMA's
     * own short-circuit). The whole formula collapses to
     *    HMA[t] = 2*price[t] - WMA(price, n)[t]
     * with no de-lag ring at all. In-place note: the output store lands on
     * the SAME slot the trailing read just consumed (zero margin), so the
     * read stays ordered before the store.
     */
    for (today = startIdx; today <= endIdx; today += 1) {
      tempReal = inReal[today];
      periodSubFull += tempReal;
      periodSubFull -= trailingFull;
      periodSumFull += tempReal * optInTimePeriod;
      barsSinceReseedFull -= 1;
      if (barsSinceReseedFull <= 0) {
        barsSinceReseedFull = ((8 * optInTimePeriod) | 0);
        periodSubFull = 0.0;
        periodSumFull = 0.0;
        rw = 1.0;
        for (jFull = today - lookbackFull; jFull <= today; jFull += 1) {
          tempReal2 = inReal[jFull];
          periodSubFull += tempReal2;
          periodSumFull += tempReal2 * rw;
          rw += 1;
        }
      }
      trailingFull = inReal[trailingIdxFull];
      trailingIdxFull += 1;
      fullOut = periodSumFull / dividerFull;
      periodSumFull -= periodSubFull;
      outReal[outIdx++] = 2.0 * tempReal - fullOut;
    }
  } else {
    /* General regime: optInTimePeriod >= 4, so halfPeriod >= 2 and
     * sqrtPeriod >= 2 -- no period-1 special cases below this point.
     */
    dividerHalf = halfPeriod * (halfPeriod + 1) / 2.0;
    dividerSqrt = sqrtPeriod * (sqrtPeriod + 1) / 2.0;
    /* Prime the half-period WMA the same way. */
    lookbackHalf = halfPeriod - 1;
    periodSubHalf = 0.0;
    periodSumHalf = 0.0;
    trailingIdxHalf = wmaStartIdx - lookbackHalf;
    i = trailingIdxHalf;
    w = 1.0;
    while (i < wmaStartIdx) {
      tempReal = inReal[i];
      i += 1;
      periodSubHalf += tempReal;
      periodSumHalf += tempReal * w;
      w += 1;
    }
    barsSinceReseedHalf = ((8 * halfPeriod) | 0);
    trailingHalf = 0.0;
    /* The de-lagged value computed at bar t leaves the outer WMA's window
     * sqrtPeriod bars later, so a single-cursor ring of sqrtPeriod slots
     * still holds it when it leaves: subtract the slot, overwrite it with
     * the current value, advance. The slot the warm-up does not fill is the
     * first bar's departing value, which is zero.
     */
    ringSize = sqrtPeriod;
    if (ringSize < 1) {
      return RetCode.INTERNAL_ERROR;
    }
    dRing = new Float64Array(ringSize);
    maxIdx_dRing = ringSize - 1;
    dRing_Idx = 0;
    dRing[ringSize - 1] = 0.0;
    /* Warm-up: the sqrtPeriod-1 de-lagged values before the first output
     * prime the outer WMA (weights 1..sqrtPeriod-1) and fill the ring.
     */
    periodSubSqrt = 0.0;
    periodSumSqrt = 0.0;
    w = 1.0;
    for (today = wmaStartIdx; today < startIdx; today += 1) {
      tempReal = inReal[today];
      periodSubFull += tempReal;
      periodSubFull -= trailingFull;
      periodSumFull += tempReal * optInTimePeriod;
      barsSinceReseedFull -= 1;
      if (barsSinceReseedFull <= 0) {
        barsSinceReseedFull = ((8 * optInTimePeriod) | 0);
        periodSubFull = 0.0;
        periodSumFull = 0.0;
        rw = 1.0;
        for (jFull = today - lookbackFull; jFull <= today; jFull += 1) {
          tempReal2 = inReal[jFull];
          periodSubFull += tempReal2;
          periodSumFull += tempReal2 * rw;
          rw += 1;
        }
      }
      trailingFull = inReal[trailingIdxFull];
      trailingIdxFull += 1;
      fullOut = periodSumFull / dividerFull;
      periodSumFull -= periodSubFull;
      periodSubHalf += tempReal;
      periodSubHalf -= trailingHalf;
      periodSumHalf += tempReal * halfPeriod;
      barsSinceReseedHalf -= 1;
      if (barsSinceReseedHalf <= 0) {
        barsSinceReseedHalf = ((8 * halfPeriod) | 0);
        periodSubHalf = 0.0;
        periodSumHalf = 0.0;
        rw = 1.0;
        for (jHalf = today - lookbackHalf; jHalf <= today; jHalf += 1) {
          tempReal2 = inReal[jHalf];
          periodSubHalf += tempReal2;
          periodSumHalf += tempReal2 * rw;
          rw += 1;
        }
      }
      trailingHalf = inReal[trailingIdxHalf];
      trailingIdxHalf += 1;
      halfOut = periodSumHalf / dividerHalf;
      periodSumHalf -= periodSubHalf;
      diffReal = 2.0 * halfOut - fullOut;
      periodSubSqrt += diffReal;
      periodSumSqrt += diffReal * w;
      w += 1;
      dRing[dRing_Idx] = diffReal;
      dRing_Idx++;
      if (dRing_Idx > maxIdx_dRing) {
        dRing_Idx = 0;
      }
    }
    barsSinceReseedSqrt = ((8 * sqrtPeriod) | 0);
    /* Read each departing bar when it is subtracted: a value carried across
     * bars costs a register, and Java at 16 XMM registers then spills a
     * running sum every bar. Keep the steady loop's half-period sum apart
     * from the warm-up's: shared, arm64 C# copies it between two registers
     * every bar.
     */
    trailingIdxFull -= 1;
    trailingIdxHalf -= 1;
    subHalf = periodSubHalf;
    /* Steady state: one pass, three rolling WMAs. Each bar's trailing read
     * sits sqrtPeriod-2 or more slots ahead of its store, zero at periods
     * 4..8, so keep that read before the store for outReal == inReal.
     */
    for (today = startIdx; today <= endIdx; today += 1) {
      tempReal = inReal[today];
      tempReal2 = inReal[trailingIdxFull];
      trailingIdxFull += 1;
      periodSubFull += tempReal;
      periodSubFull -= tempReal2;
      periodSumFull += tempReal * optInTimePeriod;
      barsSinceReseedFull -= 1;
      if (barsSinceReseedFull <= 0) {
        barsSinceReseedFull = ((8 * optInTimePeriod) | 0);
        periodSubFull = 0.0;
        periodSumFull = 0.0;
        rw = 1.0;
        for (jFull = today - lookbackFull; jFull <= today; jFull += 1) {
          tempReal2 = inReal[jFull];
          periodSubFull += tempReal2;
          periodSumFull += tempReal2 * rw;
          rw += 1;
        }
      }
      fullOut = periodSumFull / dividerFull;
      periodSumFull -= periodSubFull;
      tempReal2 = inReal[trailingIdxHalf];
      trailingIdxHalf += 1;
      subHalf += tempReal;
      subHalf -= tempReal2;
      periodSumHalf += tempReal * halfPeriod;
      barsSinceReseedHalf -= 1;
      if (barsSinceReseedHalf <= 0) {
        barsSinceReseedHalf = ((8 * halfPeriod) | 0);
        subHalf = 0.0;
        periodSumHalf = 0.0;
        rw = 1.0;
        for (jHalf = today - lookbackHalf; jHalf <= today; jHalf += 1) {
          tempReal2 = inReal[jHalf];
          subHalf += tempReal2;
          periodSumHalf += tempReal2 * rw;
          rw += 1;
        }
      }
      halfOut = periodSumHalf / dividerHalf;
      periodSumHalf -= subHalf;
      diffReal = 2.0 * halfOut - fullOut;
      periodSubSqrt += diffReal;
      periodSubSqrt -= dRing[dRing_Idx];
      periodSumSqrt += diffReal * sqrtPeriod;
      /* The outer WMA consumes a DERIVED series that is never
       * materialised, so its rescan walks the de-lag ring: the slots after
       * dRing_Idx (which holds the departing value) and diffReal are the
       * whole window. Oldest first, weight counting up from 1 -- the
       * priming order above.
       */
      barsSinceReseedSqrt -= 1;
      if (barsSinceReseedSqrt <= 0) {
        barsSinceReseedSqrt = ((8 * sqrtPeriod) | 0);
        periodSubSqrt = 0.0;
        periodSumSqrt = 0.0;
        rw = 1.0;
        ringWalk = dRing_Idx;
        for (q = 1; q < ringSize; q += 1) {
          ringWalk += 1;
          if (ringWalk >= ringSize) {
            ringWalk = 0;
          }
          tempReal2 = dRing[ringWalk];
          periodSubSqrt += tempReal2;
          periodSumSqrt += tempReal2 * rw;
          rw += 1;
        }
        periodSubSqrt += diffReal;
        periodSumSqrt += diffReal * sqrtPeriod;
      }
      dRing[dRing_Idx] = diffReal;
      dRing_Idx++;
      if (dRing_Idx > maxIdx_dRing) {
        dRing_Idx = 0;
      }
      outReal[outIdx++] = periodSumSqrt / dividerSqrt;
      periodSumSqrt -= periodSubSqrt;
    }
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * Hull Moving Average, published by Alan Hull in 2005: a moving average built to
 * track price with far less lag than an [`SMA`](https://ta-lib.org/functions/sma),
 * [`WMA`](https://ta-lib.org/functions/wma) or
 * [`EMA`](https://ta-lib.org/functions/ema) of the same length while staying
 * smooth. It first removes lag by doubling a half-period
 * [`WMA`](https://ta-lib.org/functions/wma) and subtracting the full-period one —
 * extrapolating the average toward current price — then smooths that de-lagged
 * series with a final WMA over the square root of the period. HMA is also
 * selectable as a moving-average type (`TA_MAType_HMA`) wherever an `optInMAType`
 * parameter is accepted ([`MA`](https://ta-lib.org/functions/ma),
 * [`BBANDS`](https://ta-lib.org/functions/bbands),
 * [`STOCH`](https://ta-lib.org/functions/stoch),
 * [`MACDEXT`](https://ta-lib.org/functions/macdext), ...).
 *
 * Formula and more info at
 * [ta-lib.org/functions/hma](https://ta-lib.org/functions/hma).
 *
 * **Notes**
 * - The two derived periods `n/2` and `sqrt(n)` are **truncated** to integers,
 *   exactly as in Alan Hull's own statement of the formula (`Integer()`); Tulip
 *   Indicators and pandas-ta do the same. Some other published descriptions round
 *   to nearest instead, which changes both the values and, for the square root,
 *   the lookback — a visibly different line, not a tolerance-level difference.
 *   TA-Lib follows the author.
 * - The default period of 20 is Alan Hull's own default. It is also a period on
 *   which the truncate and round-to-nearest conventions coincide (20/2 is exact;
 *   sqrt(20) = 4.47 truncates and rounds to 4), so at the default a charting
 *   platform using the other convention still lands on TA-Lib's values.
 * - A period of 1 performs no smoothing: the output is a copy of the input.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link hmaLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - Source price series, close by convention.
 * @param optInTimePeriod - Number of bars in the full-period WMA; the half and
 *   square-root periods derive from it (default 20; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Hull moving average of the input. Must hold the values produced
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
 * import { hma } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const { begIdx, count } = hma(0, data.length - 1, data, 20, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`wma`](https://ta-lib.org/functions/wma)
 * @see [`ma`](https://ta-lib.org/functions/ma)
 * @see [`sma`](https://ta-lib.org/functions/sma)
 * @see [`ema`](https://ta-lib.org/functions/ema)
 */
export function hma(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('HMA', 'startIdx', startIdx);
  endIdx = requireInt32('HMA', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('HMA', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('HMA', startIdx, endIdx);
  const guardStart = clampedStart('HMA', startIdx, hmaLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('HMA', 'inReal', inReal, guardInLen);
  requireLength('HMA', 'outReal', outReal, guardOutLen);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw failure('HMA', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = hmaImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('HMA', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live HMA stream: one value per closed bar, bit-identical to {@link hma} over
 * the same series. Open it with {@link hmaOpen} or {@link hmaOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/hma](https://ta-lib.org/functions/hma).
 */
export class HmaStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  dividerFull: number = 0;
  /** @internal */
  periodSubFull: number = 0;
  /** @internal */
  periodSumFull: number = 0;
  /** @internal */
  trailingFull: number = 0;
  /** @internal */
  lookbackFull: number = 0;
  /** @internal */
  barsSinceReseedFull: number = 0;
  /** @internal */
  halfPeriod: number = 0;
  /** @internal */
  sqrtPeriod: number = 0;
  /** @internal */
  ringSize: number = 0;
  /** @internal */
  dividerHalf: number = 0;
  /** @internal */
  dividerSqrt: number = 0;
  /** @internal */
  periodSumHalf: number = 0;
  /** @internal */
  subHalf: number = 0;
  /** @internal */
  periodSubSqrt: number = 0;
  /** @internal */
  periodSumSqrt: number = 0;
  /** @internal */
  lookbackHalf: number = 0;
  /** @internal */
  barsSinceReseedHalf: number = 0;
  /** @internal */
  barsSinceReseedSqrt: number = 0;
  /** @internal */
  dRing_Idx: number = 0;
  /** @internal */
  maxIdx_dRing: number = 0;
  /** @internal */
  ringPos_trailingIdxFull: number = 0;
  /** @internal */
  ringCap_trailingIdxFull: number = 0;
  /** @internal */
  ring_trailingIdxFull_inReal: Float64Array = new Float64Array(1);
  /** @internal */
  winPos_jFull: number = 0;
  /** @internal */
  winCap_jFull: number = 1;
  /** @internal */
  win_jFull_inReal: Float64Array = new Float64Array(1);
  /** @internal */
  cur_outReal: number = 0;
  /** @internal */
  ringPos_trailingIdxHalf: number = 0;
  /** @internal */
  ringCap_trailingIdxHalf: number = 0;
  /** @internal */
  ring_trailingIdxHalf_inReal: Float64Array = new Float64Array(1);
  /** @internal */
  winPos_jHalf: number = 0;
  /** @internal */
  winCap_jHalf: number = 1;
  /** @internal */
  win_jHalf_inReal: Float64Array = new Float64Array(1);
  /** @internal */
  cbSize_dRing: number = 0;
  /** @internal */
  cb_dRing: Float64Array = new Float64Array(1);
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
   * {@link hma} reports them over the same bars. Each accepted `update` and each
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
      throw failure('HMA advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: Source price series, close by
   *   convention.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('HMA update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('HMA update', 'inReal');
    }
    hmaStepImpl(this, inReal);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Source price series, close by
   *   convention.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number): number {
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('HMA peek', 'inReal');
    }
    const sp = this;
    let cur_outReal = 0;
    if (sp.optInTimePeriod === 1) {
      cur_outReal = inReal;
      return cur_outReal;
    }
    if (sp.optInTimePeriod === 2 || sp.optInTimePeriod === 3) {
      let tempReal = 0;
      let fullOut = 0;
      let jFull = 0;
      let tempReal2 = 0;
      let rw = 0;
      let barsSinceReseedFull = sp.barsSinceReseedFull;
      let periodSubFull = sp.periodSubFull;
      let periodSumFull = sp.periodSumFull;
      let trailingFull = sp.trailingFull;
      let pkSlot0 = -1;
      let pkVal0 = 0;
      pkSlot0 = sp.winPos_jFull;
      pkVal0 = inReal;
      tempReal = inReal;
      periodSubFull += tempReal;
      periodSubFull -= trailingFull;
      periodSumFull += tempReal * sp.optInTimePeriod;
      barsSinceReseedFull -= 1;
      if (barsSinceReseedFull <= 0) {
        barsSinceReseedFull = ((8 * sp.optInTimePeriod) | 0);
        periodSubFull = 0.0;
        periodSumFull = 0.0;
        rw = 1.0;
        for (jFull = sp.lookbackFull; jFull >= 0; jFull -= 1) {
          tempReal2 = (((sp.winPos_jFull + sp.winCap_jFull - jFull >= sp.winCap_jFull) ? sp.winPos_jFull + sp.winCap_jFull - jFull - sp.winCap_jFull : sp.winPos_jFull + sp.winCap_jFull - jFull) !== pkSlot0) ? sp.win_jFull_inReal[(sp.winPos_jFull + sp.winCap_jFull - jFull >= sp.winCap_jFull) ? sp.winPos_jFull + sp.winCap_jFull - jFull - sp.winCap_jFull : sp.winPos_jFull + sp.winCap_jFull - jFull] : pkVal0;
          periodSubFull += tempReal2;
          periodSumFull += tempReal2 * rw;
          rw += 1;
        }
      }
      trailingFull = sp.ring_trailingIdxFull_inReal[sp.ringPos_trailingIdxFull];
      fullOut = periodSumFull / sp.dividerFull;
      periodSumFull -= periodSubFull;
      cur_outReal = 2.0 * tempReal - fullOut;
    } else {
      let tempReal = 0;
      let fullOut = 0;
      let halfOut = 0;
      let diffReal = 0;
      let jFull = 0;
      let jHalf = 0;
      let q = 0;
      let ringWalk = 0;
      let tempReal2 = 0;
      let rw = 0;
      let barsSinceReseedFull = sp.barsSinceReseedFull;
      let barsSinceReseedHalf = sp.barsSinceReseedHalf;
      let barsSinceReseedSqrt = sp.barsSinceReseedSqrt;
      let dRing_Idx = sp.dRing_Idx;
      let periodSubFull = sp.periodSubFull;
      let periodSumFull = sp.periodSumFull;
      let periodSumHalf = sp.periodSumHalf;
      let periodSumSqrt = sp.periodSumSqrt;
      let subHalf = sp.subHalf;
      let pkSlot0 = -1;
      let pkVal0 = 0;
      let pkSlot1 = -1;
      let pkVal1 = 0;
      pkSlot0 = sp.winPos_jFull;
      pkVal0 = inReal;
      pkSlot1 = sp.winPos_jHalf;
      pkVal1 = inReal;
      tempReal = inReal;
      tempReal2 = sp.ring_trailingIdxFull_inReal[sp.ringPos_trailingIdxFull];
      periodSubFull += tempReal;
      periodSubFull -= tempReal2;
      periodSumFull += tempReal * sp.optInTimePeriod;
      barsSinceReseedFull -= 1;
      if (barsSinceReseedFull <= 0) {
        barsSinceReseedFull = ((8 * sp.optInTimePeriod) | 0);
        periodSubFull = 0.0;
        periodSumFull = 0.0;
        rw = 1.0;
        for (jFull = sp.lookbackFull; jFull >= 0; jFull -= 1) {
          tempReal2 = (((sp.winPos_jFull + sp.winCap_jFull - jFull >= sp.winCap_jFull) ? sp.winPos_jFull + sp.winCap_jFull - jFull - sp.winCap_jFull : sp.winPos_jFull + sp.winCap_jFull - jFull) !== pkSlot0) ? sp.win_jFull_inReal[(sp.winPos_jFull + sp.winCap_jFull - jFull >= sp.winCap_jFull) ? sp.winPos_jFull + sp.winCap_jFull - jFull - sp.winCap_jFull : sp.winPos_jFull + sp.winCap_jFull - jFull] : pkVal0;
          periodSubFull += tempReal2;
          periodSumFull += tempReal2 * rw;
          rw += 1;
        }
      }
      fullOut = periodSumFull / sp.dividerFull;
      periodSumFull -= periodSubFull;
      tempReal2 = sp.ring_trailingIdxHalf_inReal[sp.ringPos_trailingIdxHalf];
      subHalf += tempReal;
      subHalf -= tempReal2;
      periodSumHalf += tempReal * sp.halfPeriod;
      barsSinceReseedHalf -= 1;
      if (barsSinceReseedHalf <= 0) {
        barsSinceReseedHalf = ((8 * sp.halfPeriod) | 0);
        subHalf = 0.0;
        periodSumHalf = 0.0;
        rw = 1.0;
        for (jHalf = sp.lookbackHalf; jHalf >= 0; jHalf -= 1) {
          tempReal2 = (((sp.winPos_jHalf + sp.winCap_jHalf - jHalf >= sp.winCap_jHalf) ? sp.winPos_jHalf + sp.winCap_jHalf - jHalf - sp.winCap_jHalf : sp.winPos_jHalf + sp.winCap_jHalf - jHalf) !== pkSlot1) ? sp.win_jHalf_inReal[(sp.winPos_jHalf + sp.winCap_jHalf - jHalf >= sp.winCap_jHalf) ? sp.winPos_jHalf + sp.winCap_jHalf - jHalf - sp.winCap_jHalf : sp.winPos_jHalf + sp.winCap_jHalf - jHalf] : pkVal1;
          subHalf += tempReal2;
          periodSumHalf += tempReal2 * rw;
          rw += 1;
        }
      }
      halfOut = periodSumHalf / sp.dividerHalf;
      periodSumHalf -= subHalf;
      diffReal = 2.0 * halfOut - fullOut;
      periodSumSqrt += diffReal * sp.sqrtPeriod;
      /* The outer WMA consumes a DERIVED series that is never
       * materialised, so its rescan walks the de-lag ring: the slots after
       * dRing_Idx (which holds the departing value) and diffReal are the
       * whole window. Oldest first, weight counting up from 1 -- the
       * priming order above.
       */
      barsSinceReseedSqrt -= 1;
      if (barsSinceReseedSqrt <= 0) {
        barsSinceReseedSqrt = ((8 * sp.sqrtPeriod) | 0);
        periodSumSqrt = 0.0;
        rw = 1.0;
        ringWalk = dRing_Idx;
        for (q = 1; q < sp.ringSize; q += 1) {
          ringWalk += 1;
          if (ringWalk >= sp.ringSize) {
            ringWalk = 0;
          }
          tempReal2 = sp.cb_dRing[ringWalk];
          periodSumSqrt += tempReal2 * rw;
          rw += 1;
        }
        periodSumSqrt += diffReal * sp.sqrtPeriod;
      }
      dRing_Idx = dRing_Idx + 1;
      if (dRing_Idx > sp.maxIdx_dRing) {
        dRing_Idx = 0;
      }
      cur_outReal = periodSumSqrt / sp.dividerSqrt;
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
  clone(): HmaStream {
    const c = new HmaStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.dividerFull = this.dividerFull;
    c.periodSubFull = this.periodSubFull;
    c.periodSumFull = this.periodSumFull;
    c.trailingFull = this.trailingFull;
    c.lookbackFull = this.lookbackFull;
    c.barsSinceReseedFull = this.barsSinceReseedFull;
    c.halfPeriod = this.halfPeriod;
    c.sqrtPeriod = this.sqrtPeriod;
    c.ringSize = this.ringSize;
    c.dividerHalf = this.dividerHalf;
    c.dividerSqrt = this.dividerSqrt;
    c.periodSumHalf = this.periodSumHalf;
    c.subHalf = this.subHalf;
    c.periodSubSqrt = this.periodSubSqrt;
    c.periodSumSqrt = this.periodSumSqrt;
    c.lookbackHalf = this.lookbackHalf;
    c.barsSinceReseedHalf = this.barsSinceReseedHalf;
    c.barsSinceReseedSqrt = this.barsSinceReseedSqrt;
    c.dRing_Idx = this.dRing_Idx;
    c.maxIdx_dRing = this.maxIdx_dRing;
    c.ringPos_trailingIdxFull = this.ringPos_trailingIdxFull;
    c.ringCap_trailingIdxFull = this.ringCap_trailingIdxFull;
    c.ring_trailingIdxFull_inReal = this.ring_trailingIdxFull_inReal.slice();
    c.winPos_jFull = this.winPos_jFull;
    c.winCap_jFull = this.winCap_jFull;
    c.win_jFull_inReal = this.win_jFull_inReal.slice();
    c.cur_outReal = this.cur_outReal;
    c.ringPos_trailingIdxHalf = this.ringPos_trailingIdxHalf;
    c.ringCap_trailingIdxHalf = this.ringCap_trailingIdxHalf;
    c.ring_trailingIdxHalf_inReal = this.ring_trailingIdxHalf_inReal.slice();
    c.winPos_jHalf = this.winPos_jHalf;
    c.winCap_jHalf = this.winCap_jHalf;
    c.win_jHalf_inReal = this.win_jHalf_inReal.slice();
    c.cbSize_dRing = this.cbSize_dRing;
    c.cb_dRing = this.cb_dRing.slice();
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function hmaStepImpl(sp: HmaStream, inReal: number): void {
  if (sp.optInTimePeriod === 1) {
    sp.cur_outReal = inReal;
    return;
  }
  if (sp.optInTimePeriod === 2 || sp.optInTimePeriod === 3) {
    let tempReal = 0;
    let fullOut = 0;
    let jFull = 0;
    let tempReal2 = 0;
    let rw = 0;
    let ringCapL_trailingIdxFull = 0;
    sp.win_jFull_inReal[sp.winPos_jFull] = inReal;
    tempReal = inReal;
    sp.periodSubFull += tempReal;
    sp.periodSubFull -= sp.trailingFull;
    sp.periodSumFull += tempReal * sp.optInTimePeriod;
    sp.barsSinceReseedFull -= 1;
    if (sp.barsSinceReseedFull <= 0) {
      sp.barsSinceReseedFull = ((8 * sp.optInTimePeriod) | 0);
      sp.periodSubFull = 0.0;
      sp.periodSumFull = 0.0;
      rw = 1.0;
      for (jFull = sp.lookbackFull; jFull >= 0; jFull -= 1) {
        tempReal2 = sp.win_jFull_inReal[(sp.winPos_jFull + sp.winCap_jFull - jFull >= sp.winCap_jFull) ? sp.winPos_jFull + sp.winCap_jFull - jFull - sp.winCap_jFull : sp.winPos_jFull + sp.winCap_jFull - jFull];
        sp.periodSubFull += tempReal2;
        sp.periodSumFull += tempReal2 * rw;
        rw += 1;
      }
    }
    sp.trailingFull = sp.ring_trailingIdxFull_inReal[sp.ringPos_trailingIdxFull];
    fullOut = sp.periodSumFull / sp.dividerFull;
    sp.periodSumFull -= sp.periodSubFull;
    sp.cur_outReal = 2.0 * tempReal - fullOut;
    ringCapL_trailingIdxFull = sp.ringCap_trailingIdxFull;
    sp.ring_trailingIdxFull_inReal[sp.ringPos_trailingIdxFull] = inReal;
    sp.ringPos_trailingIdxFull = sp.ringPos_trailingIdxFull + 1;
    if (sp.ringPos_trailingIdxFull >= ringCapL_trailingIdxFull) {
      sp.ringPos_trailingIdxFull = 0;
    }
    sp.winPos_jFull = sp.winPos_jFull + 1;
    if (sp.winPos_jFull >= sp.winCap_jFull) {
      sp.winPos_jFull = 0;
    }
  } else {
    let tempReal = 0;
    let fullOut = 0;
    let halfOut = 0;
    let diffReal = 0;
    let jFull = 0;
    let jHalf = 0;
    let q = 0;
    let ringWalk = 0;
    let tempReal2 = 0;
    let rw = 0;
    let ringCapL_trailingIdxFull = 0;
    let ringCapL_trailingIdxHalf = 0;
    sp.win_jFull_inReal[sp.winPos_jFull] = inReal;
    sp.win_jHalf_inReal[sp.winPos_jHalf] = inReal;
    tempReal = inReal;
    tempReal2 = sp.ring_trailingIdxFull_inReal[sp.ringPos_trailingIdxFull];
    sp.periodSubFull += tempReal;
    sp.periodSubFull -= tempReal2;
    sp.periodSumFull += tempReal * sp.optInTimePeriod;
    sp.barsSinceReseedFull -= 1;
    if (sp.barsSinceReseedFull <= 0) {
      sp.barsSinceReseedFull = ((8 * sp.optInTimePeriod) | 0);
      sp.periodSubFull = 0.0;
      sp.periodSumFull = 0.0;
      rw = 1.0;
      for (jFull = sp.lookbackFull; jFull >= 0; jFull -= 1) {
        tempReal2 = sp.win_jFull_inReal[(sp.winPos_jFull + sp.winCap_jFull - jFull >= sp.winCap_jFull) ? sp.winPos_jFull + sp.winCap_jFull - jFull - sp.winCap_jFull : sp.winPos_jFull + sp.winCap_jFull - jFull];
        sp.periodSubFull += tempReal2;
        sp.periodSumFull += tempReal2 * rw;
        rw += 1;
      }
    }
    fullOut = sp.periodSumFull / sp.dividerFull;
    sp.periodSumFull -= sp.periodSubFull;
    tempReal2 = sp.ring_trailingIdxHalf_inReal[sp.ringPos_trailingIdxHalf];
    sp.subHalf += tempReal;
    sp.subHalf -= tempReal2;
    sp.periodSumHalf += tempReal * sp.halfPeriod;
    sp.barsSinceReseedHalf -= 1;
    if (sp.barsSinceReseedHalf <= 0) {
      sp.barsSinceReseedHalf = ((8 * sp.halfPeriod) | 0);
      sp.subHalf = 0.0;
      sp.periodSumHalf = 0.0;
      rw = 1.0;
      for (jHalf = sp.lookbackHalf; jHalf >= 0; jHalf -= 1) {
        tempReal2 = sp.win_jHalf_inReal[(sp.winPos_jHalf + sp.winCap_jHalf - jHalf >= sp.winCap_jHalf) ? sp.winPos_jHalf + sp.winCap_jHalf - jHalf - sp.winCap_jHalf : sp.winPos_jHalf + sp.winCap_jHalf - jHalf];
        sp.subHalf += tempReal2;
        sp.periodSumHalf += tempReal2 * rw;
        rw += 1;
      }
    }
    halfOut = sp.periodSumHalf / sp.dividerHalf;
    sp.periodSumHalf -= sp.subHalf;
    diffReal = 2.0 * halfOut - fullOut;
    sp.periodSubSqrt += diffReal;
    sp.periodSubSqrt -= sp.cb_dRing[sp.dRing_Idx];
    sp.periodSumSqrt += diffReal * sp.sqrtPeriod;
    /* The outer WMA consumes a DERIVED series that is never
     * materialised, so its rescan walks the de-lag ring: the slots after
     * dRing_Idx (which holds the departing value) and diffReal are the
     * whole window. Oldest first, weight counting up from 1 -- the
     * priming order above.
     */
    sp.barsSinceReseedSqrt -= 1;
    if (sp.barsSinceReseedSqrt <= 0) {
      sp.barsSinceReseedSqrt = ((8 * sp.sqrtPeriod) | 0);
      sp.periodSubSqrt = 0.0;
      sp.periodSumSqrt = 0.0;
      rw = 1.0;
      ringWalk = sp.dRing_Idx;
      for (q = 1; q < sp.ringSize; q += 1) {
        ringWalk += 1;
        if (ringWalk >= sp.ringSize) {
          ringWalk = 0;
        }
        tempReal2 = sp.cb_dRing[ringWalk];
        sp.periodSubSqrt += tempReal2;
        sp.periodSumSqrt += tempReal2 * rw;
        rw += 1;
      }
      sp.periodSubSqrt += diffReal;
      sp.periodSumSqrt += diffReal * sp.sqrtPeriod;
    }
    sp.cb_dRing[sp.dRing_Idx] = diffReal;
    sp.dRing_Idx = sp.dRing_Idx + 1;
    if (sp.dRing_Idx > sp.maxIdx_dRing) {
      sp.dRing_Idx = 0;
    }
    sp.cur_outReal = sp.periodSumSqrt / sp.dividerSqrt;
    sp.periodSumSqrt -= sp.periodSubSqrt;
    ringCapL_trailingIdxFull = sp.ringCap_trailingIdxFull;
    sp.ring_trailingIdxFull_inReal[sp.ringPos_trailingIdxFull] = inReal;
    sp.ringPos_trailingIdxFull = sp.ringPos_trailingIdxFull + 1;
    if (sp.ringPos_trailingIdxFull >= ringCapL_trailingIdxFull) {
      sp.ringPos_trailingIdxFull = 0;
    }
    ringCapL_trailingIdxHalf = sp.ringCap_trailingIdxHalf;
    sp.ring_trailingIdxHalf_inReal[sp.ringPos_trailingIdxHalf] = inReal;
    sp.ringPos_trailingIdxHalf = sp.ringPos_trailingIdxHalf + 1;
    if (sp.ringPos_trailingIdxHalf >= ringCapL_trailingIdxHalf) {
      sp.ringPos_trailingIdxHalf = 0;
    }
    sp.winPos_jFull = sp.winPos_jFull + 1;
    if (sp.winPos_jFull >= sp.winCap_jFull) {
      sp.winPos_jFull = 0;
    }
    sp.winPos_jHalf = sp.winPos_jHalf + 1;
    if (sp.winPos_jHalf >= sp.winCap_jHalf) {
      sp.winPos_jHalf = 0;
    }
  }
}

function hmaOpenImpl(
  sp: HmaStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod === 1) {
    let fillLb = hmaLookback(optInTimePeriod, core);
    if (startIdx > fillLb) {
      fillLb = startIdx;
    }
    if (historyLen < fillLb + 1) {
      return RetCode.INSUFFICIENT_HISTORY;
    }
    sp.optInTimePeriod = optInTimePeriod;
    sp.dividerFull = 0;
    sp.periodSubFull = 0;
    sp.periodSumFull = 0;
    sp.trailingFull = 0;
    sp.lookbackFull = 0;
    sp.barsSinceReseedFull = 0;
    sp.halfPeriod = 0;
    sp.sqrtPeriod = 0;
    sp.ringSize = 0;
    sp.dividerHalf = 0;
    sp.dividerSqrt = 0;
    sp.periodSumHalf = 0;
    sp.subHalf = 0;
    sp.periodSubSqrt = 0;
    sp.periodSumSqrt = 0;
    sp.lookbackHalf = 0;
    sp.barsSinceReseedHalf = 0;
    sp.barsSinceReseedSqrt = 0;
    sp.dRing_Idx = 0;
    sp.maxIdx_dRing = 0;
    sp.ringPos_trailingIdxFull = 0;
    sp.ringCap_trailingIdxFull = 0;
    sp.ring_trailingIdxFull_inReal = new Float64Array(1);
    sp.winPos_jFull = 0;
    sp.winCap_jFull = 1;
    sp.win_jFull_inReal = new Float64Array(1);
    sp.ringPos_trailingIdxHalf = 0;
    sp.ringCap_trailingIdxHalf = 0;
    sp.ring_trailingIdxHalf_inReal = new Float64Array(1);
    sp.winPos_jHalf = 0;
    sp.winCap_jHalf = 1;
    sp.win_jHalf_inReal = new Float64Array(1);
    sp.cbSize_dRing = 0;
    sp.cb_dRing = new Float64Array(1);
    outBegIdx.value = fillLb;
    outNBElement.value = historyLen - fillLb;
    if (outStride === 0) {
      outReal[0] = inReal[historyLen - 1];
    } else {
      for (let fillIdx = 0; fillIdx < historyLen - fillLb; fillIdx++) {
        outReal[fillIdx] = inReal[fillLb + fillIdx];
      }
    }
    sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
    return RetCode.SUCCESS;
  }
  if (optInTimePeriod === 2 || optInTimePeriod === 3) {
    let lookbackTotal = 0;
    let lookbackSqrt = 0;
    let halfPeriod = 0;
    let sqrtPeriod = 0;
    let ringSize = 0;
    let wmaStartIdx = 0;
    let today = 0;
    let outIdx = 0;
    let i = 0;
    let dividerFull = 0;
    let dividerHalf = 0;
    let dividerSqrt = 0;
    let trailingIdxFull = 0;
    let periodSubFull = 0;
    let periodSumFull = 0;
    let trailingFull = 0;
    let periodSumHalf = 0;
    let subHalf = 0;
    let periodSubSqrt = 0;
    let periodSumSqrt = 0;
    let tempReal = 0;
    let fullOut = 0;
    let jFull = 0;
    let lookbackFull = 0;
    let lookbackHalf = 0;
    let barsSinceReseedFull = 0;
    let barsSinceReseedHalf = 0;
    let barsSinceReseedSqrt = 0;
    let tempReal2 = 0;
    let w = 0;
    let rw = 0;
    let dRing_Idx = 0;
    let maxIdx_dRing = (50) - 1;
    /* Keep the weights double: they stay exact (never above the period), and
     * an integer weight slows the Rust loop.
     */
    /* The de-lagged series needs only its last sqrt(n) values, so the whole
     * computation runs in one pass over a single window into the input:
     * three interleaved WMA rolling sums plus this small ring. The ring has
     * sqrt(n) slots: on the stack while that fits the 50-slot prolog
     * (optInTimePeriod <= 2600), TA_Malloc from optInTimePeriod = 2601 up.
     */
    /* Hull Moving Average (Alan Hull, 2005):
     *
     *    HMA(n) = WMA( 2*WMA(price, Integer(n/2)) - WMA(price, n), Integer(SquareRoot(n)) )
     *
     * Both derived periods use the author's Integer() truncation; some other
     * published sources round to nearest instead, which is a visibly different
     * line. See hma.md and issue #139.
     *
     * Each of the three WMAs keeps TA_WMA's exact accumulation order
     * (periodSub/periodSum, trailing subtract), so this fused pass is
     * BIT-IDENTICAL to composing three TA_WMA calls -- the composite
     * differential in test_composite1.c holds it to that, memcmp-exact.
     */
    halfPeriod = ((optInTimePeriod / 2) | 0);
    sqrtPeriod = (Math.sqrt(optInTimePeriod) | 0);
    lookbackSqrt = wmaLookback(sqrtPeriod, core);
    lookbackTotal = wmaLookback(optInTimePeriod, core) + lookbackSqrt;
    /* Move up the start index if there is not
     * enough initial data.
     */
    if (startIdx < lookbackTotal) {
      startIdx = lookbackTotal;
    }
    /* Make sure there is still something to evaluate. */
    if (startIdx > endIdx) {
      outBegIdx.value = 0;
      outNBElement.value = 0;
      return RetCode.INSUFFICIENT_HISTORY;
    }
    /* The two price WMAs are anchored where the first de-lagged value is
     * needed: lookbackSqrt bars before the first requested output.
     * wmaStartIdx >= optInTimePeriod-1 is implied by the clamp above.
     */
    wmaStartIdx = startIdx - lookbackSqrt;
    dividerFull = optInTimePeriod * (optInTimePeriod + 1) / 2.0;
    /* Prime the full-period WMA over the optInTimePeriod-1 bars before
     * wmaStartIdx, exactly as TA_WMA does (weights 1..period-1).
     */
    lookbackFull = optInTimePeriod - 1;
    periodSubFull = 0.0;
    periodSumFull = 0.0;
    trailingIdxFull = wmaStartIdx - lookbackFull;
    i = trailingIdxFull;
    w = 1.0;
    while (i < wmaStartIdx) {
      tempReal = inReal[i];
      i += 1;
      periodSubFull += tempReal;
      periodSumFull += tempReal * w;
      w += 1;
    }
    barsSinceReseedFull = ((8 * optInTimePeriod) | 0);
    trailingFull = 0.0;
    outIdx = 0;
    /* sqrtPeriod == 1 exactly when optInTimePeriod is 2 or 3; stated on the
     * param so the stream analyzer sees a param-pure dual-mode split.
     */
    /* Degenerate regime, optInTimePeriod 2 or 3 only: halfPeriod and
     * sqrtPeriod are both 1, and a period-1 WMA is the identity (TA_WMA's
     * own short-circuit). The whole formula collapses to
     *    HMA[t] = 2*price[t] - WMA(price, n)[t]
     * with no de-lag ring at all. In-place note: the output store lands on
     * the SAME slot the trailing read just consumed (zero margin), so the
     * read stays ordered before the store.
     */
    for (today = startIdx; today <= endIdx; today += 1) {
      tempReal = inReal[today];
      periodSubFull += tempReal;
      periodSubFull -= trailingFull;
      periodSumFull += tempReal * optInTimePeriod;
      barsSinceReseedFull -= 1;
      if (barsSinceReseedFull <= 0) {
        barsSinceReseedFull = ((8 * optInTimePeriod) | 0);
        periodSubFull = 0.0;
        periodSumFull = 0.0;
        rw = 1.0;
        for (jFull = today - lookbackFull; jFull <= today; jFull += 1) {
          tempReal2 = inReal[jFull];
          periodSubFull += tempReal2;
          periodSumFull += tempReal2 * rw;
          rw += 1;
        }
      }
      trailingFull = inReal[trailingIdxFull];
      trailingIdxFull += 1;
      fullOut = periodSumFull / dividerFull;
      periodSumFull -= periodSubFull;
      outReal[((outIdx++ * outStride) | 0)] = 2.0 * tempReal - fullOut;
    }
    outBegIdx.value = startIdx;
    outNBElement.value = outIdx;
    const cap_trailingIdxFull = today - trailingIdxFull;
    if (cap_trailingIdxFull < 1 || cap_trailingIdxFull > historyLen) {
      return RetCode.INTERNAL_ERROR;
    }
    const allocN_trailingIdxFull = cap_trailingIdxFull > 0 ? cap_trailingIdxFull : 1;
    const capRing_trailingIdxFull_inReal = new Float64Array(allocN_trailingIdxFull);
    capRing_trailingIdxFull_inReal.set(inReal.subarray(historyLen - cap_trailingIdxFull, historyLen), 0);
    const cap_jFull = lookbackFull + 1;
    if (cap_jFull < 1 || cap_jFull > historyLen) {
      return RetCode.INTERNAL_ERROR;
    }
    const capWin_jFull_inReal = inReal.slice(historyLen - cap_jFull, historyLen);
    sp.optInTimePeriod = optInTimePeriod;
    sp.dividerFull = dividerFull;
    sp.periodSubFull = periodSubFull;
    sp.periodSumFull = periodSumFull;
    sp.trailingFull = trailingFull;
    sp.lookbackFull = lookbackFull;
    sp.barsSinceReseedFull = barsSinceReseedFull;
    sp.halfPeriod = halfPeriod;
    sp.sqrtPeriod = sqrtPeriod;
    sp.ringSize = ringSize;
    sp.dividerHalf = dividerHalf;
    sp.dividerSqrt = dividerSqrt;
    sp.periodSumHalf = periodSumHalf;
    sp.subHalf = subHalf;
    sp.periodSubSqrt = periodSubSqrt;
    sp.periodSumSqrt = periodSumSqrt;
    sp.lookbackHalf = lookbackHalf;
    sp.barsSinceReseedHalf = barsSinceReseedHalf;
    sp.barsSinceReseedSqrt = barsSinceReseedSqrt;
    sp.dRing_Idx = dRing_Idx;
    sp.maxIdx_dRing = maxIdx_dRing;
    sp.ringPos_trailingIdxFull = 0;
    sp.ringCap_trailingIdxFull = cap_trailingIdxFull;
    sp.ring_trailingIdxFull_inReal = capRing_trailingIdxFull_inReal;
    sp.winPos_jFull = 0;
    sp.winCap_jFull = cap_jFull;
    sp.win_jFull_inReal = capWin_jFull_inReal;
    sp.ring_trailingIdxHalf_inReal = new Float64Array(1);
    sp.win_jHalf_inReal = new Float64Array(1);
    sp.cb_dRing = new Float64Array(1);
    sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
    return RetCode.SUCCESS;
  } else {
    let lookbackTotal = 0;
    let lookbackSqrt = 0;
    let halfPeriod = 0;
    let sqrtPeriod = 0;
    let ringSize = 0;
    let wmaStartIdx = 0;
    let today = 0;
    let outIdx = 0;
    let i = 0;
    let dividerFull = 0;
    let dividerHalf = 0;
    let dividerSqrt = 0;
    let trailingIdxFull = 0;
    let trailingIdxHalf = 0;
    let periodSubFull = 0;
    let periodSumFull = 0;
    let trailingFull = 0;
    let periodSubHalf = 0;
    let periodSumHalf = 0;
    let trailingHalf = 0;
    let subHalf = 0;
    let periodSubSqrt = 0;
    let periodSumSqrt = 0;
    let tempReal = 0;
    let fullOut = 0;
    let halfOut = 0;
    let diffReal = 0;
    let jFull = 0;
    let jHalf = 0;
    let q = 0;
    let ringWalk = 0;
    let lookbackFull = 0;
    let lookbackHalf = 0;
    let barsSinceReseedFull = 0;
    let barsSinceReseedHalf = 0;
    let barsSinceReseedSqrt = 0;
    let tempReal2 = 0;
    let w = 0;
    let rw = 0;
    let dRing!: Float64Array;
    let dRing_Idx = 0;
    let maxIdx_dRing = (50) - 1;
    /* Keep the weights double: they stay exact (never above the period), and
     * an integer weight slows the Rust loop.
     */
    /* The de-lagged series needs only its last sqrt(n) values, so the whole
     * computation runs in one pass over a single window into the input:
     * three interleaved WMA rolling sums plus this small ring. The ring has
     * sqrt(n) slots: on the stack while that fits the 50-slot prolog
     * (optInTimePeriod <= 2600), TA_Malloc from optInTimePeriod = 2601 up.
     */
    /* Hull Moving Average (Alan Hull, 2005):
     *
     *    HMA(n) = WMA( 2*WMA(price, Integer(n/2)) - WMA(price, n), Integer(SquareRoot(n)) )
     *
     * Both derived periods use the author's Integer() truncation; some other
     * published sources round to nearest instead, which is a visibly different
     * line. See hma.md and issue #139.
     *
     * Each of the three WMAs keeps TA_WMA's exact accumulation order
     * (periodSub/periodSum, trailing subtract), so this fused pass is
     * BIT-IDENTICAL to composing three TA_WMA calls -- the composite
     * differential in test_composite1.c holds it to that, memcmp-exact.
     */
    halfPeriod = ((optInTimePeriod / 2) | 0);
    sqrtPeriod = (Math.sqrt(optInTimePeriod) | 0);
    lookbackSqrt = wmaLookback(sqrtPeriod, core);
    lookbackTotal = wmaLookback(optInTimePeriod, core) + lookbackSqrt;
    /* Move up the start index if there is not
     * enough initial data.
     */
    if (startIdx < lookbackTotal) {
      startIdx = lookbackTotal;
    }
    /* Make sure there is still something to evaluate. */
    if (startIdx > endIdx) {
      outBegIdx.value = 0;
      outNBElement.value = 0;
      return RetCode.INSUFFICIENT_HISTORY;
    }
    /* The two price WMAs are anchored where the first de-lagged value is
     * needed: lookbackSqrt bars before the first requested output.
     * wmaStartIdx >= optInTimePeriod-1 is implied by the clamp above.
     */
    wmaStartIdx = startIdx - lookbackSqrt;
    dividerFull = optInTimePeriod * (optInTimePeriod + 1) / 2.0;
    /* Prime the full-period WMA over the optInTimePeriod-1 bars before
     * wmaStartIdx, exactly as TA_WMA does (weights 1..period-1).
     */
    lookbackFull = optInTimePeriod - 1;
    periodSubFull = 0.0;
    periodSumFull = 0.0;
    trailingIdxFull = wmaStartIdx - lookbackFull;
    i = trailingIdxFull;
    w = 1.0;
    while (i < wmaStartIdx) {
      tempReal = inReal[i];
      i += 1;
      periodSubFull += tempReal;
      periodSumFull += tempReal * w;
      w += 1;
    }
    barsSinceReseedFull = ((8 * optInTimePeriod) | 0);
    trailingFull = 0.0;
    outIdx = 0;
    /* sqrtPeriod == 1 exactly when optInTimePeriod is 2 or 3; stated on the
     * param so the stream analyzer sees a param-pure dual-mode split.
     */
    /* General regime: optInTimePeriod >= 4, so halfPeriod >= 2 and
     * sqrtPeriod >= 2 -- no period-1 special cases below this point.
     */
    dividerHalf = halfPeriod * (halfPeriod + 1) / 2.0;
    dividerSqrt = sqrtPeriod * (sqrtPeriod + 1) / 2.0;
    /* Prime the half-period WMA the same way. */
    lookbackHalf = halfPeriod - 1;
    periodSubHalf = 0.0;
    periodSumHalf = 0.0;
    trailingIdxHalf = wmaStartIdx - lookbackHalf;
    i = trailingIdxHalf;
    w = 1.0;
    while (i < wmaStartIdx) {
      tempReal = inReal[i];
      i += 1;
      periodSubHalf += tempReal;
      periodSumHalf += tempReal * w;
      w += 1;
    }
    barsSinceReseedHalf = ((8 * halfPeriod) | 0);
    trailingHalf = 0.0;
    /* The de-lagged value computed at bar t leaves the outer WMA's window
     * sqrtPeriod bars later, so a single-cursor ring of sqrtPeriod slots
     * still holds it when it leaves: subtract the slot, overwrite it with
     * the current value, advance. The slot the warm-up does not fill is the
     * first bar's departing value, which is zero.
     */
    ringSize = sqrtPeriod;
    if (ringSize < 1) {
      return RetCode.INTERNAL_ERROR;
    }
    dRing = new Float64Array(ringSize);
    maxIdx_dRing = ringSize - 1;
    dRing_Idx = 0;
    dRing[ringSize - 1] = 0.0;
    /* Warm-up: the sqrtPeriod-1 de-lagged values before the first output
     * prime the outer WMA (weights 1..sqrtPeriod-1) and fill the ring.
     */
    periodSubSqrt = 0.0;
    periodSumSqrt = 0.0;
    w = 1.0;
    for (today = wmaStartIdx; today < startIdx; today += 1) {
      tempReal = inReal[today];
      periodSubFull += tempReal;
      periodSubFull -= trailingFull;
      periodSumFull += tempReal * optInTimePeriod;
      barsSinceReseedFull -= 1;
      if (barsSinceReseedFull <= 0) {
        barsSinceReseedFull = ((8 * optInTimePeriod) | 0);
        periodSubFull = 0.0;
        periodSumFull = 0.0;
        rw = 1.0;
        for (jFull = today - lookbackFull; jFull <= today; jFull += 1) {
          tempReal2 = inReal[jFull];
          periodSubFull += tempReal2;
          periodSumFull += tempReal2 * rw;
          rw += 1;
        }
      }
      trailingFull = inReal[trailingIdxFull];
      trailingIdxFull += 1;
      fullOut = periodSumFull / dividerFull;
      periodSumFull -= periodSubFull;
      periodSubHalf += tempReal;
      periodSubHalf -= trailingHalf;
      periodSumHalf += tempReal * halfPeriod;
      barsSinceReseedHalf -= 1;
      if (barsSinceReseedHalf <= 0) {
        barsSinceReseedHalf = ((8 * halfPeriod) | 0);
        periodSubHalf = 0.0;
        periodSumHalf = 0.0;
        rw = 1.0;
        for (jHalf = today - lookbackHalf; jHalf <= today; jHalf += 1) {
          tempReal2 = inReal[jHalf];
          periodSubHalf += tempReal2;
          periodSumHalf += tempReal2 * rw;
          rw += 1;
        }
      }
      trailingHalf = inReal[trailingIdxHalf];
      trailingIdxHalf += 1;
      halfOut = periodSumHalf / dividerHalf;
      periodSumHalf -= periodSubHalf;
      diffReal = 2.0 * halfOut - fullOut;
      periodSubSqrt += diffReal;
      periodSumSqrt += diffReal * w;
      w += 1;
      dRing[dRing_Idx] = diffReal;
      dRing_Idx++;
      if (dRing_Idx > maxIdx_dRing) {
        dRing_Idx = 0;
      }
    }
    barsSinceReseedSqrt = ((8 * sqrtPeriod) | 0);
    /* Read each departing bar when it is subtracted: a value carried across
     * bars costs a register, and Java at 16 XMM registers then spills a
     * running sum every bar. Keep the steady loop's half-period sum apart
     * from the warm-up's: shared, arm64 C# copies it between two registers
     * every bar.
     */
    trailingIdxFull -= 1;
    trailingIdxHalf -= 1;
    subHalf = periodSubHalf;
    /* Steady state: one pass, three rolling WMAs. Each bar's trailing read
     * sits sqrtPeriod-2 or more slots ahead of its store, zero at periods
     * 4..8, so keep that read before the store for outReal == inReal.
     */
    for (today = startIdx; today <= endIdx; today += 1) {
      tempReal = inReal[today];
      tempReal2 = inReal[trailingIdxFull];
      trailingIdxFull += 1;
      periodSubFull += tempReal;
      periodSubFull -= tempReal2;
      periodSumFull += tempReal * optInTimePeriod;
      barsSinceReseedFull -= 1;
      if (barsSinceReseedFull <= 0) {
        barsSinceReseedFull = ((8 * optInTimePeriod) | 0);
        periodSubFull = 0.0;
        periodSumFull = 0.0;
        rw = 1.0;
        for (jFull = today - lookbackFull; jFull <= today; jFull += 1) {
          tempReal2 = inReal[jFull];
          periodSubFull += tempReal2;
          periodSumFull += tempReal2 * rw;
          rw += 1;
        }
      }
      fullOut = periodSumFull / dividerFull;
      periodSumFull -= periodSubFull;
      tempReal2 = inReal[trailingIdxHalf];
      trailingIdxHalf += 1;
      subHalf += tempReal;
      subHalf -= tempReal2;
      periodSumHalf += tempReal * halfPeriod;
      barsSinceReseedHalf -= 1;
      if (barsSinceReseedHalf <= 0) {
        barsSinceReseedHalf = ((8 * halfPeriod) | 0);
        subHalf = 0.0;
        periodSumHalf = 0.0;
        rw = 1.0;
        for (jHalf = today - lookbackHalf; jHalf <= today; jHalf += 1) {
          tempReal2 = inReal[jHalf];
          subHalf += tempReal2;
          periodSumHalf += tempReal2 * rw;
          rw += 1;
        }
      }
      halfOut = periodSumHalf / dividerHalf;
      periodSumHalf -= subHalf;
      diffReal = 2.0 * halfOut - fullOut;
      periodSubSqrt += diffReal;
      periodSubSqrt -= dRing[dRing_Idx];
      periodSumSqrt += diffReal * sqrtPeriod;
      /* The outer WMA consumes a DERIVED series that is never
       * materialised, so its rescan walks the de-lag ring: the slots after
       * dRing_Idx (which holds the departing value) and diffReal are the
       * whole window. Oldest first, weight counting up from 1 -- the
       * priming order above.
       */
      barsSinceReseedSqrt -= 1;
      if (barsSinceReseedSqrt <= 0) {
        barsSinceReseedSqrt = ((8 * sqrtPeriod) | 0);
        periodSubSqrt = 0.0;
        periodSumSqrt = 0.0;
        rw = 1.0;
        ringWalk = dRing_Idx;
        for (q = 1; q < ringSize; q += 1) {
          ringWalk += 1;
          if (ringWalk >= ringSize) {
            ringWalk = 0;
          }
          tempReal2 = dRing[ringWalk];
          periodSubSqrt += tempReal2;
          periodSumSqrt += tempReal2 * rw;
          rw += 1;
        }
        periodSubSqrt += diffReal;
        periodSumSqrt += diffReal * sqrtPeriod;
      }
      dRing[dRing_Idx] = diffReal;
      dRing_Idx++;
      if (dRing_Idx > maxIdx_dRing) {
        dRing_Idx = 0;
      }
      outReal[((outIdx++ * outStride) | 0)] = periodSumSqrt / dividerSqrt;
      periodSumSqrt -= periodSubSqrt;
    }
    outBegIdx.value = startIdx;
    outNBElement.value = outIdx;
    const cap_trailingIdxFull = today - trailingIdxFull;
    if (cap_trailingIdxFull < 1 || cap_trailingIdxFull > historyLen) {
      return RetCode.INTERNAL_ERROR;
    }
    const allocN_trailingIdxFull = cap_trailingIdxFull > 0 ? cap_trailingIdxFull : 1;
    const capRing_trailingIdxFull_inReal = new Float64Array(allocN_trailingIdxFull);
    capRing_trailingIdxFull_inReal.set(inReal.subarray(historyLen - cap_trailingIdxFull, historyLen), 0);
    const cap_trailingIdxHalf = today - trailingIdxHalf;
    if (cap_trailingIdxHalf < 1 || cap_trailingIdxHalf > historyLen) {
      return RetCode.INTERNAL_ERROR;
    }
    const allocN_trailingIdxHalf = cap_trailingIdxHalf > 0 ? cap_trailingIdxHalf : 1;
    const capRing_trailingIdxHalf_inReal = new Float64Array(allocN_trailingIdxHalf);
    capRing_trailingIdxHalf_inReal.set(inReal.subarray(historyLen - cap_trailingIdxHalf, historyLen), 0);
    const cap_jFull = lookbackFull + 1;
    if (cap_jFull < 1 || cap_jFull > historyLen) {
      return RetCode.INTERNAL_ERROR;
    }
    const capWin_jFull_inReal = inReal.slice(historyLen - cap_jFull, historyLen);
    const cap_jHalf = lookbackHalf + 1;
    if (cap_jHalf < 1 || cap_jHalf > historyLen) {
      return RetCode.INTERNAL_ERROR;
    }
    const capWin_jHalf_inReal = inReal.slice(historyLen - cap_jHalf, historyLen);
    const capCb_dRing = maxIdx_dRing + 1;
    if (capCb_dRing > historyLen + 1) {
      return RetCode.INTERNAL_ERROR;
    }
    sp.optInTimePeriod = optInTimePeriod;
    sp.dividerFull = dividerFull;
    sp.periodSubFull = periodSubFull;
    sp.periodSumFull = periodSumFull;
    sp.trailingFull = trailingFull;
    sp.lookbackFull = lookbackFull;
    sp.barsSinceReseedFull = barsSinceReseedFull;
    sp.halfPeriod = halfPeriod;
    sp.sqrtPeriod = sqrtPeriod;
    sp.ringSize = ringSize;
    sp.dividerHalf = dividerHalf;
    sp.dividerSqrt = dividerSqrt;
    sp.periodSumHalf = periodSumHalf;
    sp.subHalf = subHalf;
    sp.periodSubSqrt = periodSubSqrt;
    sp.periodSumSqrt = periodSumSqrt;
    sp.lookbackHalf = lookbackHalf;
    sp.barsSinceReseedHalf = barsSinceReseedHalf;
    sp.barsSinceReseedSqrt = barsSinceReseedSqrt;
    sp.dRing_Idx = dRing_Idx;
    sp.maxIdx_dRing = maxIdx_dRing;
    sp.ringPos_trailingIdxFull = 0;
    sp.ringCap_trailingIdxFull = cap_trailingIdxFull;
    sp.ring_trailingIdxFull_inReal = capRing_trailingIdxFull_inReal;
    sp.ringPos_trailingIdxHalf = 0;
    sp.ringCap_trailingIdxHalf = cap_trailingIdxHalf;
    sp.ring_trailingIdxHalf_inReal = capRing_trailingIdxHalf_inReal;
    sp.winPos_jFull = 0;
    sp.winCap_jFull = cap_jFull;
    sp.win_jFull_inReal = capWin_jFull_inReal;
    sp.winPos_jHalf = 0;
    sp.winCap_jHalf = cap_jHalf;
    sp.win_jHalf_inReal = capWin_jHalf_inReal;
    sp.cbSize_dRing = capCb_dRing;
    sp.cb_dRing = dRing;
    sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
    return RetCode.SUCCESS;
  }
}

/** @internal */
export function hmaOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): HmaStream {
  const sp = new HmaStream(core);
  const retCode = hmaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('HMA openAndFill', inReal.length, startIdx, hmaLookback(optInTimePeriod, core));
  }
  throw streamFailure('HMA openAndFill', retCode);
}

/** @internal */
export function hmaOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): HmaStream {
  const sp = new HmaStream(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = hmaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('HMA open', inReal.length, startIdx, hmaLookback(optInTimePeriod, core));
  }
  throw streamFailure('HMA open', retCode);
}

/**
 * Opens a live HMA stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link hma} there.
 *
 * @param inReal - Warm-up history. Source price series, close by convention.
 * @param optInTimePeriod - Number of bars in the full-period WMA; the half and
 *   square-root periods derive from it (default 20; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `hmaLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32 or a
 *   parameter outside its range.
 *
 * @example
 * ```ts
 * import { hmaOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = hmaOpen(data, 20);
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function hmaOpen(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): HmaStream {
  optInTimePeriod = requireInt32('HMA open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('HMA open', 'inReal', inReal);
  requireHistory('HMA open', inReal.length);
  return hmaOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/**
 * {@link hmaOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link hma}. The range written is the stream's
 * `outRange()`.
 *
 * @param inReal - Warm-up history. Source price series, close by convention.
 * @param optInTimePeriod - Number of bars in the full-period WMA; the half and
 *   square-root periods derive from it (default 20; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Hull moving average of the input. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `hmaLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or an output too short or starting where another
 *   output or a `Float64Array` input starts, or, on a history long enough to open,
 *   one sharing memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { hmaOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const stream = hmaOpenAndFill(data, 20, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function hmaOpenAndFill(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): HmaStream {
  optInTimePeriod = requireInt32('HMA openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('HMA openAndFill', 'inReal', inReal);
  requireHistory('HMA openAndFill', inReal.length);
  const guardOutLen = openFillCount('HMA openAndFill', inReal.length, hmaLookback(optInTimePeriod, core));
  requireLength('HMA openAndFill', 'outReal', outReal, guardOutLen);
  const inRealF64 = float64(inReal);
  if ((outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset)) {
    throw streamFailure('HMA openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inRealF64))) {
    throw streamFailure('HMA openAndFill', RetCode.BAD_PARAM);
  }
  return hmaOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/** @internal */
export function hmaStepTape(sp: HmaStream, tape: Float64Array, tapeBase: number, tapeMask: number, inReal: number): number {
  if (sp.optInTimePeriod === 2 || sp.optInTimePeriod === 3) {
    let tempReal = 0;
    let fullOut = 0;
    let jFull = 0;
    let tempReal2 = 0;
    let rw = 0;
    tempReal = inReal;
    sp.periodSubFull += tempReal;
    sp.periodSubFull -= sp.trailingFull;
    sp.periodSumFull += tempReal * sp.optInTimePeriod;
    sp.barsSinceReseedFull -= 1;
    if (sp.barsSinceReseedFull <= 0) {
      sp.barsSinceReseedFull = ((8 * sp.optInTimePeriod) | 0);
      sp.periodSubFull = 0.0;
      sp.periodSumFull = 0.0;
      rw = 1.0;
      for (jFull = sp.lookbackFull; jFull >= 0; jFull -= 1) {
        tempReal2 = tape[(tapeBase - jFull) & tapeMask];
        sp.periodSubFull += tempReal2;
        sp.periodSumFull += tempReal2 * rw;
        rw += 1;
      }
    }
    sp.trailingFull = tape[(tapeBase - sp.ringCap_trailingIdxFull) & tapeMask];
    fullOut = sp.periodSumFull / sp.dividerFull;
    sp.periodSumFull -= sp.periodSubFull;
    sp.cur_outReal = 2.0 * tempReal - fullOut;
  } else {
    let tempReal = 0;
    let fullOut = 0;
    let halfOut = 0;
    let diffReal = 0;
    let jFull = 0;
    let jHalf = 0;
    let q = 0;
    let ringWalk = 0;
    let tempReal2 = 0;
    let rw = 0;
    tempReal = inReal;
    tempReal2 = tape[(tapeBase - sp.ringCap_trailingIdxFull) & tapeMask];
    sp.periodSubFull += tempReal;
    sp.periodSubFull -= tempReal2;
    sp.periodSumFull += tempReal * sp.optInTimePeriod;
    sp.barsSinceReseedFull -= 1;
    if (sp.barsSinceReseedFull <= 0) {
      sp.barsSinceReseedFull = ((8 * sp.optInTimePeriod) | 0);
      sp.periodSubFull = 0.0;
      sp.periodSumFull = 0.0;
      rw = 1.0;
      for (jFull = sp.lookbackFull; jFull >= 0; jFull -= 1) {
        tempReal2 = tape[(tapeBase - jFull) & tapeMask];
        sp.periodSubFull += tempReal2;
        sp.periodSumFull += tempReal2 * rw;
        rw += 1;
      }
    }
    fullOut = sp.periodSumFull / sp.dividerFull;
    sp.periodSumFull -= sp.periodSubFull;
    tempReal2 = tape[(tapeBase - sp.ringCap_trailingIdxHalf) & tapeMask];
    sp.subHalf += tempReal;
    sp.subHalf -= tempReal2;
    sp.periodSumHalf += tempReal * sp.halfPeriod;
    sp.barsSinceReseedHalf -= 1;
    if (sp.barsSinceReseedHalf <= 0) {
      sp.barsSinceReseedHalf = ((8 * sp.halfPeriod) | 0);
      sp.subHalf = 0.0;
      sp.periodSumHalf = 0.0;
      rw = 1.0;
      for (jHalf = sp.lookbackHalf; jHalf >= 0; jHalf -= 1) {
        tempReal2 = tape[(tapeBase - jHalf) & tapeMask];
        sp.subHalf += tempReal2;
        sp.periodSumHalf += tempReal2 * rw;
        rw += 1;
      }
    }
    halfOut = sp.periodSumHalf / sp.dividerHalf;
    sp.periodSumHalf -= sp.subHalf;
    diffReal = 2.0 * halfOut - fullOut;
    sp.periodSubSqrt += diffReal;
    sp.periodSubSqrt -= sp.cb_dRing[sp.dRing_Idx];
    sp.periodSumSqrt += diffReal * sp.sqrtPeriod;
    /* The outer WMA consumes a DERIVED series that is never
     * materialised, so its rescan walks the de-lag ring: the slots after
     * dRing_Idx (which holds the departing value) and diffReal are the
     * whole window. Oldest first, weight counting up from 1 -- the
     * priming order above.
     */
    sp.barsSinceReseedSqrt -= 1;
    if (sp.barsSinceReseedSqrt <= 0) {
      sp.barsSinceReseedSqrt = ((8 * sp.sqrtPeriod) | 0);
      sp.periodSubSqrt = 0.0;
      sp.periodSumSqrt = 0.0;
      rw = 1.0;
      ringWalk = sp.dRing_Idx;
      for (q = 1; q < sp.ringSize; q += 1) {
        ringWalk += 1;
        if (ringWalk >= sp.ringSize) {
          ringWalk = 0;
        }
        tempReal2 = sp.cb_dRing[ringWalk];
        sp.periodSubSqrt += tempReal2;
        sp.periodSumSqrt += tempReal2 * rw;
        rw += 1;
      }
      sp.periodSubSqrt += diffReal;
      sp.periodSumSqrt += diffReal * sp.sqrtPeriod;
    }
    sp.cb_dRing[sp.dRing_Idx] = diffReal;
    sp.dRing_Idx = sp.dRing_Idx + 1;
    if (sp.dRing_Idx > sp.maxIdx_dRing) {
      sp.dRing_Idx = 0;
    }
    sp.cur_outReal = sp.periodSumSqrt / sp.dividerSqrt;
    sp.periodSumSqrt -= sp.periodSubSqrt;
  }
  sp.outRangeCount++;
  return sp.cur_outReal;
}

/** @internal */
export function hmaPeekTape(sp: HmaStream, tape: Float64Array, tapeBase: number, tapeMask: number, inReal: number): number {
  let cur_outReal = 0;
  if (sp.optInTimePeriod === 2 || sp.optInTimePeriod === 3) {
    let tempReal = 0;
    let fullOut = 0;
    let jFull = 0;
    let tempReal2 = 0;
    let rw = 0;
    let barsSinceReseedFull = sp.barsSinceReseedFull;
    let periodSubFull = sp.periodSubFull;
    let periodSumFull = sp.periodSumFull;
    let trailingFull = sp.trailingFull;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    pkSlot0 = tapeBase & tapeMask;
    pkVal0 = inReal;
    tempReal = inReal;
    periodSubFull += tempReal;
    periodSubFull -= trailingFull;
    periodSumFull += tempReal * sp.optInTimePeriod;
    barsSinceReseedFull -= 1;
    if (barsSinceReseedFull <= 0) {
      barsSinceReseedFull = ((8 * sp.optInTimePeriod) | 0);
      periodSubFull = 0.0;
      periodSumFull = 0.0;
      rw = 1.0;
      for (jFull = sp.lookbackFull; jFull >= 0; jFull -= 1) {
        tempReal2 = (((tapeBase - jFull) & tapeMask) !== pkSlot0) ? tape[(tapeBase - jFull) & tapeMask] : pkVal0;
        periodSubFull += tempReal2;
        periodSumFull += tempReal2 * rw;
        rw += 1;
      }
    }
    trailingFull = tape[(tapeBase - sp.ringCap_trailingIdxFull) & tapeMask];
    fullOut = periodSumFull / sp.dividerFull;
    periodSumFull -= periodSubFull;
    cur_outReal = 2.0 * tempReal - fullOut;
  } else {
    let tempReal = 0;
    let fullOut = 0;
    let halfOut = 0;
    let diffReal = 0;
    let jFull = 0;
    let jHalf = 0;
    let q = 0;
    let ringWalk = 0;
    let tempReal2 = 0;
    let rw = 0;
    let barsSinceReseedFull = sp.barsSinceReseedFull;
    let barsSinceReseedHalf = sp.barsSinceReseedHalf;
    let barsSinceReseedSqrt = sp.barsSinceReseedSqrt;
    let dRing_Idx = sp.dRing_Idx;
    let periodSubFull = sp.periodSubFull;
    let periodSumFull = sp.periodSumFull;
    let periodSumHalf = sp.periodSumHalf;
    let periodSumSqrt = sp.periodSumSqrt;
    let subHalf = sp.subHalf;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    pkSlot0 = tapeBase & tapeMask;
    pkVal0 = inReal;
    tempReal = inReal;
    tempReal2 = tape[(tapeBase - sp.ringCap_trailingIdxFull) & tapeMask];
    periodSubFull += tempReal;
    periodSubFull -= tempReal2;
    periodSumFull += tempReal * sp.optInTimePeriod;
    barsSinceReseedFull -= 1;
    if (barsSinceReseedFull <= 0) {
      barsSinceReseedFull = ((8 * sp.optInTimePeriod) | 0);
      periodSubFull = 0.0;
      periodSumFull = 0.0;
      rw = 1.0;
      for (jFull = sp.lookbackFull; jFull >= 0; jFull -= 1) {
        tempReal2 = (((tapeBase - jFull) & tapeMask) !== pkSlot0) ? tape[(tapeBase - jFull) & tapeMask] : pkVal0;
        periodSubFull += tempReal2;
        periodSumFull += tempReal2 * rw;
        rw += 1;
      }
    }
    fullOut = periodSumFull / sp.dividerFull;
    periodSumFull -= periodSubFull;
    tempReal2 = tape[(tapeBase - sp.ringCap_trailingIdxHalf) & tapeMask];
    subHalf += tempReal;
    subHalf -= tempReal2;
    periodSumHalf += tempReal * sp.halfPeriod;
    barsSinceReseedHalf -= 1;
    if (barsSinceReseedHalf <= 0) {
      barsSinceReseedHalf = ((8 * sp.halfPeriod) | 0);
      subHalf = 0.0;
      periodSumHalf = 0.0;
      rw = 1.0;
      for (jHalf = sp.lookbackHalf; jHalf >= 0; jHalf -= 1) {
        tempReal2 = (((tapeBase - jHalf) & tapeMask) !== pkSlot0) ? tape[(tapeBase - jHalf) & tapeMask] : pkVal0;
        subHalf += tempReal2;
        periodSumHalf += tempReal2 * rw;
        rw += 1;
      }
    }
    halfOut = periodSumHalf / sp.dividerHalf;
    periodSumHalf -= subHalf;
    diffReal = 2.0 * halfOut - fullOut;
    periodSumSqrt += diffReal * sp.sqrtPeriod;
    /* The outer WMA consumes a DERIVED series that is never
     * materialised, so its rescan walks the de-lag ring: the slots after
     * dRing_Idx (which holds the departing value) and diffReal are the
     * whole window. Oldest first, weight counting up from 1 -- the
     * priming order above.
     */
    barsSinceReseedSqrt -= 1;
    if (barsSinceReseedSqrt <= 0) {
      barsSinceReseedSqrt = ((8 * sp.sqrtPeriod) | 0);
      periodSumSqrt = 0.0;
      rw = 1.0;
      ringWalk = dRing_Idx;
      for (q = 1; q < sp.ringSize; q += 1) {
        ringWalk += 1;
        if (ringWalk >= sp.ringSize) {
          ringWalk = 0;
        }
        tempReal2 = sp.cb_dRing[ringWalk];
        periodSumSqrt += tempReal2 * rw;
        rw += 1;
      }
      periodSumSqrt += diffReal * sp.sqrtPeriod;
    }
    dRing_Idx = dRing_Idx + 1;
    if (dRing_Idx > sp.maxIdx_dRing) {
      dRing_Idx = 0;
    }
    cur_outReal = periodSumSqrt / sp.dividerSqrt;
  }
  return cur_outReal;
}

/** @internal */
export function hmaTapeDetach(sp: HmaStream): number {
  let reach = 0;
  sp.ring_trailingIdxFull_inReal = new Float64Array(0);
  if (sp.ringCap_trailingIdxFull > reach) {
    reach = sp.ringCap_trailingIdxFull;
  }
  sp.win_jFull_inReal = new Float64Array(0);
  if (sp.winCap_jFull - 1 > reach) {
    reach = sp.winCap_jFull - 1;
  }
  sp.ring_trailingIdxHalf_inReal = new Float64Array(0);
  if (sp.ringCap_trailingIdxHalf > reach) {
    reach = sp.ringCap_trailingIdxHalf;
  }
  sp.win_jHalf_inReal = new Float64Array(0);
  if (sp.winCap_jHalf - 1 > reach) {
    reach = sp.winCap_jHalf - 1;
  }
  return reach;
}
