/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  YV       Yvictor (@Yvictor)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  021807 MF     Initial Version
 *  072026 MF,CC  Fix #130. Stage results locally so in-place (outReal==inReal)
 *                calls no longer corrupt the input the ma() passes re-read.
 *  072626 MF,CC  #143. Group outputs by clamped period (counting sort) and
 *                bound each ma() pass at its period's last use.
 *  072726 MF,CC  #145. Index the bucket table relative to the smallest period
 *                used, and bound it so an off-contract period cannot overflow.
 *  080326 MF,CC  Split the size temp from the cast-fed period temp (#160).
 *  092526 MF,CC  #442. Allocate the multi-period buffers on that path only.
 *  100226 MF,CC  #497. An inverted window is refused with the range untouched.
 *  103026 YV,CC  Equal bounds skip the period clamp scan (PR #563).
 */

import { MAType, MATypes } from '../enums.ts';
import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { MaStream, ma, maLookback, maOpenInternal, maPeekTape, maStepTape, maTapeDetach } from './ma.ts';

/**
 * Number of leading input bars {@link mavp} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInMinPeriod - Lower clamp for the per-bar period (default 2; range 1
 *   to 10000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInMaxPeriod - Upper clamp for the per-bar period (default 30; range 1
 *   to 10000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInMAType - Moving-average type applied (default `MAType.SMA`; values:
 *   0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA,
 *   10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
 *   `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function mavpLookback(optInMinPeriod: number, optInMaxPeriod: number, optInMAType: MAType | typeof Core.INTEGER_DEFAULT, core: Core = Core.DEFAULT): number {
  if ((optInMinPeriod | 0) !== optInMinPeriod) {
    return -1;
  }
  if ((optInMaxPeriod | 0) !== optInMaxPeriod) {
    return -1;
  }
  if ((optInMAType | 0) !== optInMAType) {
    return -1;
  }
  if (optInMinPeriod === Core.INTEGER_DEFAULT) {
    optInMinPeriod = 2;
  } else if (optInMinPeriod < 1 || optInMinPeriod > 10000) {
    return -1;
  }
  if (optInMaxPeriod === Core.INTEGER_DEFAULT) {
    optInMaxPeriod = 30;
  } else if (optInMaxPeriod < 1 || optInMaxPeriod > 10000) {
    return -1;
  }
  if (optInMAType === Core.INTEGER_DEFAULT || optInMAType === MAType.DEFAULT) {
    optInMAType = MAType.SMA;
  } else if (optInMAType < MATypes.Min || optInMAType > MATypes.Max) {
    return -1;
  }
  /* The same cross-parameter constraint mavp() rejects on. Each period is in
   * range on its own, so no prologue check can catch it, and without this the
   * lookback answers a usable number for a call that cannot run.
   */
  if (optInMinPeriod > optInMaxPeriod) {
    return -1;
  }
  return maLookback(optInMaxPeriod, (optInMAType as MAType), core);
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link mavp}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInMinPeriod - Lower clamp for the per-bar period (default 2; range 1
 *   to 10000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInMaxPeriod - Upper clamp for the per-bar period (default 30; range 1
 *   to 10000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInMAType - Moving-average type applied (default `MAType.SMA`; values:
 *   0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA,
 *   10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
 *   `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function mavpDisplayShift(optInMinPeriod: number, optInMaxPeriod: number, optInMAType: MAType | typeof Core.INTEGER_DEFAULT, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (mavpLookback(optInMinPeriod, optInMaxPeriod, optInMAType, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function mavpImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  inPeriods: Float64Array,
  optInMinPeriod: number,
  optInMaxPeriod: number,
  optInMAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let i = 0;
  let lookbackTotal = 0;
  let outputSize = 0;
  let firstOut = 0;
  let tempInt = 0;
  let curPeriod = 0;
  let tempPeriod = 0;
  let minPeriodReal = 0;
  let maxPeriodReal = 0;
  let firstOccurrence = 0;
  let lastOccurrence = 0;
  let bucketStart = 0;
  let bucketEnd = 0;
  let minUsed = 0;
  let maxUsed = 0;
  let localPeriodArray!: Int32Array;
  let sortedIdx!: Int32Array;
  let bucketOfs!: Int32Array;
  let localOutputArray!: Float64Array;
  let localFinalArray!: Float64Array;
  const localBegIdx = new MInteger();
  const localNbElement = new MInteger();
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInMinPeriod === Core.INTEGER_DEFAULT) {
    optInMinPeriod = 2;
  } else if (optInMinPeriod < 1 || optInMinPeriod > 10000) {
    return RetCode.BAD_PARAM;
  }
  if (optInMaxPeriod === Core.INTEGER_DEFAULT) {
    optInMaxPeriod = 30;
  } else if (optInMaxPeriod < 1 || optInMaxPeriod > 10000) {
    return RetCode.BAD_PARAM;
  }
  if (optInMAType === Core.INTEGER_DEFAULT || optInMAType === MAType.DEFAULT) {
    optInMAType = MAType.SMA;
  } else if (optInMAType < MATypes.Min || optInMAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  /* An inverted period window (min above max) is an invalid parameter
   * combination: the per-bar clamp below would push a period above
   * optInMaxPeriod, exceeding the lookback and reading uninitialized
   * results. Reject it cleanly instead of returning garbage.
   */
  if (optInMinPeriod > optInMaxPeriod) {
    return RetCode.BAD_PARAM;
  }
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = maLookback(optInMaxPeriod, (optInMAType as MAType), core);
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
  /* Calculate exact output size. A dedicated temp: tempInt is the cast-fed
   * period below, which the Rust backend types SIGNED (#160) — reusing it
   * here would drag this index arithmetic into i32.
   */
  if (lookbackTotal > startIdx) {
    firstOut = lookbackTotal;
  } else {
    firstOut = startIdx;
  }
  if (firstOut > endIdx) {
    /* No output */
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  outputSize = endIdx - firstOut + 1;
  localPeriodArray = new Int32Array(outputSize);
  /* In-place defence (issue #130): each ma() pass below re-reads inReal over
   * the full range, so with outReal==inReal the results are staged in a
   * scratch buffer and copied once at the end. A regular call writes
   * straight to outReal and skips both the allocation and the copy.
   */
  if ((outReal.buffer === inReal.buffer && outReal.byteOffset === inReal.byteOffset)) {
    localFinalArray = new Float64Array(outputSize);
  } else {
    localFinalArray = outReal;
  }
  /* Read the caller array of period, truncate to min/max, and track the
   * range of periods actually used so all later work is sized by the data,
   * not by optInMaxPeriod. The floor at 1 (and on minUsed's start value)
   * keeps a period below 1 from indexing the occurrence tables out of range.
   * mavp.yaml's period range starts at 1, so it is inert through the API;
   * it is kept because this file is the source of truth for four
   * backends and it makes the shared source safe by construction rather than
   * by trusting each backend's prologue to be identical.
   */
  minUsed = optInMaxPeriod;
  if (minUsed < 1) {
    minUsed = 1;
  }
  maxUsed = 1;
  /* Both bounds widened once, outside the loop. In the C backend, left to the
   * compiler, only the first of the two is hoisted.
   */
  minPeriodReal = optInMinPeriod;
  maxPeriodReal = optInMaxPeriod;
  /* Equal bounds clamp every period to the same value, so the period array
   * is left unwritten: only the multi-period path may read it.
   */
  if (optInMinPeriod === optInMaxPeriod) {
    maxUsed = minUsed;
  } else {
    for (i = 0; i < outputSize; i += 1) {
      /* Clamp in the real domain, then narrow -- the order matters in the C
       * backend, and only there. C leaves an out-of-range narrowing undefined
       * and x86 lands EVERY such value on INT_MIN, so clamping afterwards pulls
       * a huge POSITIVE period down to the minimum. Java, C# and Rust saturate
       * to their maximum instead, so they were already right and this form
       * simply keeps them so.
       * `!(x >= min)` rather than `x < min`: both plain comparisons are false
       * for NaN, so only the inverted spelling catches it.
       */
      tempPeriod = inPeriods[startIdx + i];
      if (!(tempPeriod >= minPeriodReal)) {
        tempInt = optInMinPeriod;
      } else if (tempPeriod > maxPeriodReal) {
        tempInt = optInMaxPeriod;
      } else {
        tempInt = (tempPeriod | 0);
      }
      if (tempInt < 1) {
        tempInt = 1;
      }
      localPeriodArray[i] = tempInt;
      if (tempInt < minUsed) {
        minUsed = tempInt;
      }
      if (tempInt > maxUsed) {
        maxUsed = tempInt;
      }
    }
  }
  /* Bound the bucket table before sizing it.
   *
   * Unreachable through the API: mavp.yaml's period range keeps the spread
   * below this bound. It is kept because it protects a
   * memory-safety property and this file is the source of truth for four
   * backends — without it the size expression below can overflow (signed
   * overflow in C, a wrapped negative in Java, a usize underflow panic in
   * Rust), and the four bodies would rest on each backend's prologue being
   * byte-for-byte equivalent, which nothing here states or checks. One
   * integer comparison per call is a cheap way not to depend on that.
   *
   * Written as a plain integer comparison on purpose — that is the only
   * construct that means the same thing in every backend. A (size_t) cast
   * would NOT help: this dialect's size_t parses to the generic index type
   * and renders back as int in both the C and the Java output (it is a
   * Rust-only annotation).
   *
   * If you delete this, delete the clamps and the comments together.
   */
  if (maxUsed < minUsed || maxUsed - minUsed > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (minUsed === maxUsed) {
    /* Single distinct period: one MA pass, written straight into the
     * destination buffer. Nothing to group or copy.
     */
    const _xr0 = ma(startIdx, endIdx, inReal, minUsed, (optInMAType as MAType), localFinalArray, core);
    localBegIdx.value = _xr0.begIdx;
    localNbElement.value = _xr0.count;
  } else {
    localOutputArray = new Float64Array(outputSize);
    sortedIdx = new Int32Array(outputSize);
    bucketOfs = new Int32Array(maxUsed - minUsed + 2);
    /* Counting sort: sortedIdx ends up holding the output indices ordered
     * by period, one contiguous ascending slice per distinct period, with
     * bucketOfs[p] the end of period p's slice.
     */
    for (curPeriod = minUsed; curPeriod <= maxUsed + 1; curPeriod += 1) {
      bucketOfs[curPeriod - minUsed] = 0;
    }
    for (i = 0; i < outputSize; i += 1) {
      /* Staged through tempInt, not indexed inline: the Rust backend only
       * coerces an int-array read to the index type when it is a DIRECT
       * operand, so bucketOfs[localPeriodArray[i]+1-minUsed] would mix
       * i32 with usize and fail to compile.
       */
      tempInt = localPeriodArray[i];
      bucketOfs[tempInt + 1 - minUsed] = bucketOfs[tempInt + 1 - minUsed] + 1;
    }
    for (curPeriod = minUsed; curPeriod <= maxUsed; curPeriod += 1) {
      bucketOfs[curPeriod + 1 - minUsed] = bucketOfs[curPeriod + 1 - minUsed] + bucketOfs[curPeriod - minUsed];
    }
    for (i = 0; i < outputSize; i += 1) {
      tempInt = localPeriodArray[i];
      sortedIdx[bucketOfs[tempInt - minUsed]] = i;
      bucketOfs[tempInt - minUsed] = bucketOfs[tempInt - minUsed] + 1;
    }
    /* One MA pass per period actually requested, ending at the last output
     * that uses it: outputs before that point cannot depend on later input
     * (every MA here fills forward), so the shorter range is bit-identical.
     * The pass must still START at startIdx: the window MAs (SMA, WMA,
     * TRIMA, HMA) slide a running accumulator seeded at startIdx-lookback,
     * so starting at the period's first use would change the rounding path
     * and break bit-identity, as would moving any recursive MA's warm-up.
     * The direct indexing of localOutputArray also relies on ma_lookback
     * being non-decreasing in the period, so the inner call never moves
     * its own start up. Both properties are pinned by the MAVP/GROUPING
     * regression test.
     */
    bucketStart = 0;
    for (curPeriod = minUsed; curPeriod <= maxUsed; curPeriod += 1) {
      bucketEnd = bucketOfs[curPeriod - minUsed];
      if (bucketEnd > bucketStart) {
        firstOccurrence = sortedIdx[bucketStart];
        lastOccurrence = sortedIdx[bucketEnd - 1];
        /* Calculation of the MA required. */
        const _xr1 = ma(startIdx, startIdx + lastOccurrence, inReal, curPeriod, (optInMAType as MAType), localOutputArray, core);
        localBegIdx.value = _xr1.begIdx;
        localNbElement.value = _xr1.count;
        if (lastOccurrence - firstOccurrence === bucketEnd - 1 - bucketStart) {
          /* The period's outputs form one contiguous run: block copy. */
          localFinalArray.set(localOutputArray.subarray(firstOccurrence, firstOccurrence + (bucketEnd - bucketStart)), firstOccurrence);
        } else {
          for (i = bucketStart; i < bucketEnd; i += 1) {
            tempInt = sortedIdx[i];
            localFinalArray[tempInt] = localOutputArray[tempInt];
          }
        }
      }
      bucketStart = bucketEnd;
    }
  }
  if (!(localFinalArray.buffer === outReal.buffer && localFinalArray.byteOffset === outReal.byteOffset)) {
    outReal.set(localFinalArray.subarray(0, outputSize), 0);
  }
  /* Done. Inform the caller of the success. */
  outBegIdx.value = startIdx;
  outNBElement.value = outputSize;
  return RetCode.SUCCESS;
}

/**
 * Moving average whose period varies per bar, driven by a companion period series.
 * For each bar it computes an MA of the selected type over the (clamped) period
 * given by inPeriods.
 *
 * Formula and more info at
 * [ta-lib.org/functions/mavp](https://ta-lib.org/functions/mavp).
 *
 * **Notes**
 * - Fractional per-bar periods are truncated to whole numbers before being clamped
 *   to the minimum and maximum period.
 * - Period values of 1 perform no smoothing (the bar's output equals its input);
 *   the minimum allowed period is 1 since 0.6.5.
 * - `optInMinPeriod` must not exceed `optInMaxPeriod`; a larger minimum is
 *   rejected.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link mavpLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - series to be averaged.
 * @param inPeriods - per-bar desired MA period.
 * @param optInMinPeriod - Lower clamp for the per-bar period (default 2; range 1
 *   to 10000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInMaxPeriod - Upper clamp for the per-bar period (default 30; range 1
 *   to 10000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInMAType - Moving-average type applied (default `MAType.SMA`; values:
 *   0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA,
 *   10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
 *   `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param outReal - variable-period moving average. Must hold the values produced
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
 * import { MAType, mavp } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const periods = Float64Array.from({ length: 252 }, (_, i) => 5 + (i % 10));
 * const out = new Float64Array(data.length);
 * const { begIdx, count } = mavp(
 *   0, data.length - 1,
 *   data, periods,
 *   2, 30, MAType.SMA,
 *   out,
 * );
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`ma`](https://ta-lib.org/functions/ma)
 * @see [`sma`](https://ta-lib.org/functions/sma)
 * @see [`mama`](https://ta-lib.org/functions/mama)
 * @see [`t3`](https://ta-lib.org/functions/t3)
 */
export function mavp(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  inPeriods: ArrayLike<number>,
  optInMinPeriod: number,
  optInMaxPeriod: number,
  optInMAType: MAType | typeof Core.INTEGER_DEFAULT,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('MAVP', 'startIdx', startIdx);
  endIdx = requireInt32('MAVP', 'endIdx', endIdx);
  optInMinPeriod = requireInt32('MAVP', 'optInMinPeriod', optInMinPeriod);
  optInMaxPeriod = requireInt32('MAVP', 'optInMaxPeriod', optInMaxPeriod);
  optInMAType = requireInt32('MAVP', 'optInMAType', optInMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireIndexRange('MAVP', startIdx, endIdx);
  const guardStart = clampedStart('MAVP', startIdx, mavpLookback(optInMinPeriod, optInMaxPeriod, optInMAType, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('MAVP', 'inReal', inReal, guardInLen);
  requireLength('MAVP', 'inPeriods', inPeriods, guardInLen);
  requireLength('MAVP', 'outReal', outReal, guardOutLen);
  const inRealF64 = float64(inReal);
  const inPeriodsF64 = float64(inPeriods);
  if (overlapsPartly(outReal, inRealF64) || overlapsPartly(outReal, inPeriodsF64)) {
    throw failure('MAVP', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = mavpImpl(core, startIdx, endIdx, inRealF64, inPeriodsF64, optInMinPeriod, optInMaxPeriod, optInMAType, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('MAVP', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live MAVP stream: one value per closed bar, bit-identical to {@link mavp} over
 * the same series. Open it with {@link mavpOpen} or {@link mavpOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/mavp](https://ta-lib.org/functions/mavp).
 */
export class MavpStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInMinPeriod: number = 0;
  /** @internal */
  optInMaxPeriod: number = 0;
  /** @internal */
  optInMAType: number = 0;
  /** @internal */
  cur_outReal: number = 0;
  /** @internal */
  tapeMask: number = 0;
  /** @internal */
  tapePos: number = 0;
  /** @internal */
  tape: Float64Array = new Float64Array(0);
  /** @internal */
  bank: MaStream[] = [];
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): MavpStream {
    return new MavpStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link mavp} reports them over the same bars. Each accepted `update` and each
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
      throw failure('MAVP advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: series to be averaged.
   * @param inPeriods - The bar's `inPeriods` value: per-bar desired MA period.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number, inPeriods: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('MAVP update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal) || !Number.isFinite(inPeriods)) {
      throw nonFiniteBar('MAVP update', !Number.isFinite(inReal) ? 'inReal' : 'inPeriods');
    }
    mavpStepImpl(this, inReal, inPeriods);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: series to be averaged.
   * @param inPeriods - The bar's `inPeriods` value: per-bar desired MA period.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number, inPeriods: number): number {
    if (!Number.isFinite(inReal) || !Number.isFinite(inPeriods)) {
      throw nonFiniteBar('MAVP peek', !Number.isFinite(inReal) ? 'inReal' : 'inPeriods');
    }
    const sp = this;
    let cp = sp.optInMinPeriod;
    if (inPeriods > sp.optInMaxPeriod) {
      cp = sp.optInMaxPeriod;
    } else if (inPeriods >= sp.optInMinPeriod) {
      cp = inPeriods | 0;
    }
    const slot = cp - sp.optInMinPeriod;
    const cur_outReal = sp.bank.length === 0 ? mavpEvalWindow(sp, inReal, cp) : maPeekTape(sp.bank[slot], sp.tape, ((sp.tapePos + 1) & sp.tapeMask) + sp.tapeMask + 1, sp.tapeMask, inReal);
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
  clone(): MavpStream {
    const c = new MavpStream(this.core);
    c.optInMinPeriod = this.optInMinPeriod;
    c.optInMaxPeriod = this.optInMaxPeriod;
    c.optInMAType = this.optInMAType;
    c.cur_outReal = this.cur_outReal;
    c.tapeMask = this.tapeMask;
    c.tapePos = this.tapePos;
    c.tape = this.tape.slice();
    c.bank = [];
    for (let bankIdx = 0; bankIdx < this.bank.length; bankIdx++) {
      c.bank.push(this.bank[bankIdx].clone());
    }
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function mavpStepImpl(sp: MavpStream, inReal: number, inPeriods: number): void {
  if (sp.bank.length === 0) {
    let cp = sp.optInMinPeriod;
    if (inPeriods > sp.optInMaxPeriod) {
      cp = sp.optInMaxPeriod;
    } else if (inPeriods >= sp.optInMinPeriod) {
      cp = inPeriods | 0;
    }
    const v = mavpEvalWindow(sp, inReal, cp);
    sp.tapePos = (sp.tapePos + 1) & sp.tapeMask;
    sp.tape[sp.tapePos] = inReal;
    sp.cur_outReal = v;
    return;
  }
  let cp = sp.optInMinPeriod;
  if (inPeriods > sp.optInMaxPeriod) {
    cp = sp.optInMaxPeriod;
  } else if (inPeriods >= sp.optInMinPeriod) {
    cp = inPeriods | 0;
  }
  const slot = cp - sp.optInMinPeriod;
  sp.tapePos = (sp.tapePos + 1) & sp.tapeMask;
  sp.tape[sp.tapePos] = inReal;
  const tapeBase = sp.tapePos + sp.tapeMask + 1;
  for (let bankIdx = 0; bankIdx < sp.bank.length; bankIdx++) {
    const subValue = maStepTape(sp.bank[bankIdx], sp.tape, tapeBase, sp.tapeMask, inReal);
    if (bankIdx === slot) {
      sp.cur_outReal = subValue;
    }
  }
}

function mavpTapeOpen(sp: MavpStream, inReal: Float64Array, historyLen: number, reach: number): void {
  let size = 1;
  while (size <= reach) {
    size <<= 1;
  }
  sp.tape = new Float64Array(size);
  sp.tapeMask = size - 1;
  for (let b = historyLen > size ? historyLen - size : 0; b < historyLen; b++) {
    sp.tape[b & sp.tapeMask] = inReal[b];
  }
  sp.tapePos = (historyLen - 1) & sp.tapeMask;
}

function mavpWindowMode(t: number): boolean {
  switch (t) {
    case MAType.ALMA:
    case MAType.DISABLED:
      return true;
    default:
      return false;
  }
}

let mavpWindow = new Float64Array(0);
const mavpValue = new Float64Array(1);

function mavpEvalWindow(sp: MavpStream, inReal: number, cp: number): number {
  const lb = maLookback(cp, sp.optInMAType as MAType, sp.core);
  if (mavpWindow.length < lb + 1) {
    mavpWindow = new Float64Array(lb + 1);
  }
  const win = mavpWindow;
  for (let i = 0; i < lb; i++) {
    win[i] = sp.tape[(sp.tapePos + sp.tapeMask + 2 - lb + i) & sp.tapeMask];
  }
  win[lb] = inReal;
  ma(lb, lb, win, cp, sp.optInMAType as MAType, mavpValue, sp.core);
  return mavpValue[0];
}

function mavpOpenImpl(
  sp: MavpStream,
  inReal: Float64Array,
  inPeriods: Float64Array,
  startIdx: number,
  optInMinPeriod: number,
  optInMaxPeriod: number,
  optInMAType: number,
): RetCode {
  const core = sp.core;
  const historyLen = inReal.length;
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inPeriods.length !== inReal.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInMinPeriod === Core.INTEGER_DEFAULT) {
    optInMinPeriod = 2;
  } else if (optInMinPeriod < 1 || optInMinPeriod > 10000) {
    return RetCode.BAD_PARAM;
  }
  if (optInMaxPeriod === Core.INTEGER_DEFAULT) {
    optInMaxPeriod = 30;
  } else if (optInMaxPeriod < 1 || optInMaxPeriod > 10000) {
    return RetCode.BAD_PARAM;
  }
  if (optInMAType === Core.INTEGER_DEFAULT || optInMAType === MAType.DEFAULT) {
    optInMAType = MAType.SMA;
  } else if (optInMAType < MATypes.Min || optInMAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if (optInMinPeriod > optInMaxPeriod) {
    return RetCode.BAD_PARAM;
  }
  if (historyLen < mavpLookback(optInMinPeriod, optInMaxPeriod, optInMAType as MAType, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const lookbackTotal = maLookback(optInMaxPeriod, optInMAType as MAType, core);
  const subStart = startIdx < lookbackTotal ? lookbackTotal : startIdx;
  if (historyLen < subStart + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const nBank = optInMaxPeriod - optInMinPeriod + 1;
  let window = mavpWindowMode(optInMAType);
  if (optInMaxPeriod - optInMinPeriod + 1 < 12) {
    window = false;
  }
  if (window) {
    for (let bankIdx = 0; bankIdx < nBank; bankIdx++) {
      if (maLookback(optInMinPeriod + bankIdx, optInMAType as MAType, core) > lookbackTotal) {
        window = false;
        break;
      }
    }
  }
  if (window) {
    let cp = optInMinPeriod;
    if (inPeriods[historyLen - 1] > optInMaxPeriod) {
      cp = optInMaxPeriod;
    } else if (inPeriods[historyLen - 1] >= optInMinPeriod) {
      cp = inPeriods[historyLen - 1] | 0;
    }
    sp.optInMinPeriod = optInMinPeriod;
    sp.optInMaxPeriod = optInMaxPeriod;
    sp.optInMAType = optInMAType;
    sp.bank = [];
    mavpTapeOpen(sp, inReal, historyLen, lookbackTotal);
    const out1 = new Float64Array(1);
    ma(historyLen - 1, historyLen - 1, inReal, cp, optInMAType as MAType, out1, core);
    sp.cur_outReal = out1[0];
    sp.outRangeBegIdx = subStart;
    sp.outRangeCount = historyLen - subStart;
    return RetCode.SUCCESS;
  }
  const bank: MaStream[] = [];
  let reach = 0;
  for (let bankIdx = 0; bankIdx < nBank; bankIdx++) {
    const slotSub = maOpenInternal(core, inReal, subStart, optInMinPeriod + bankIdx, optInMAType as MAType);
    bank.push(slotSub);
    const slotReach = maTapeDetach(slotSub);
    if (slotReach > reach) {
      reach = slotReach;
    }
  }
  let cp = optInMinPeriod;
  if (inPeriods[historyLen - 1] > optInMaxPeriod) {
    cp = optInMaxPeriod;
  } else if (inPeriods[historyLen - 1] >= optInMinPeriod) {
    cp = inPeriods[historyLen - 1] | 0;
  }
  sp.optInMinPeriod = optInMinPeriod;
  sp.optInMaxPeriod = optInMaxPeriod;
  sp.optInMAType = optInMAType;
  sp.bank = bank;
  mavpTapeOpen(sp, inReal, historyLen, reach);
  sp.cur_outReal = bank[cp - optInMinPeriod].cur_outReal;
  sp.outRangeBegIdx = subStart;
  sp.outRangeCount = historyLen - subStart;
  return RetCode.SUCCESS;
}

function mavpOpenAndFillImpl(
  sp: MavpStream,
  inReal: Float64Array,
  inPeriods: Float64Array,
  optInMinPeriod: number,
  optInMaxPeriod: number,
  optInMAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  const core = sp.core;
  const historyLen = inReal.length;
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inPeriods.length !== inReal.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInMinPeriod === Core.INTEGER_DEFAULT) {
    optInMinPeriod = 2;
  } else if (optInMinPeriod < 1 || optInMinPeriod > 10000) {
    return RetCode.BAD_PARAM;
  }
  if (optInMaxPeriod === Core.INTEGER_DEFAULT) {
    optInMaxPeriod = 30;
  } else if (optInMaxPeriod < 1 || optInMaxPeriod > 10000) {
    return RetCode.BAD_PARAM;
  }
  if (optInMAType === Core.INTEGER_DEFAULT || optInMAType === MAType.DEFAULT) {
    optInMAType = MAType.SMA;
  } else if (optInMAType < MATypes.Min || optInMAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if ((outReal.buffer === inReal.buffer && outReal.byteOffset === inReal.byteOffset) || (outReal.buffer === inPeriods.buffer && outReal.byteOffset === inPeriods.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  if (historyLen > mavpLookback(optInMinPeriod, optInMaxPeriod, optInMAType as MAType, core) && (overlaps(outReal, inReal) || overlaps(outReal, inPeriods))) {
    return RetCode.BAD_PARAM;
  }
  if (optInMinPeriod > optInMaxPeriod) {
    return RetCode.BAD_PARAM;
  }
  const lookbackTotal = maLookback(optInMaxPeriod, optInMAType as MAType, core);
  if (historyLen < lookbackTotal + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const nBank = optInMaxPeriod - optInMinPeriod + 1;
  let window = mavpWindowMode(optInMAType);
  if (optInMaxPeriod - optInMinPeriod + 1 < 12) {
    window = false;
  }
  if (window) {
    for (let bankIdx = 0; bankIdx < nBank; bankIdx++) {
      if (maLookback(optInMinPeriod + bankIdx, optInMAType as MAType, core) > lookbackTotal) {
        window = false;
        break;
      }
    }
  }
  if (window) {
    const run = new Float64Array(historyLen - lookbackTotal);
    let t1 = 0;
    for (let t = lookbackTotal; t < historyLen; t = t1 + 1) {
      let cp = optInMinPeriod;
      if (inPeriods[t] > optInMaxPeriod) {
        cp = optInMaxPeriod;
      } else if (inPeriods[t] >= optInMinPeriod) {
        cp = inPeriods[t] | 0;
      }
      for (t1 = t; t1 + 1 < historyLen; t1++) {
        let cp2 = optInMinPeriod;
        if (inPeriods[t1 + 1] > optInMaxPeriod) {
          cp2 = optInMaxPeriod;
        } else if (inPeriods[t1 + 1] >= optInMinPeriod) {
          cp2 = inPeriods[t1 + 1] | 0;
        }
        if (cp2 !== cp) {
          break;
        }
      }
      ma(t, t1, inReal, cp, optInMAType as MAType, run, core);
      outReal.set(run.subarray(0, t1 - t + 1), t - lookbackTotal);
    }
    sp.optInMinPeriod = optInMinPeriod;
    sp.optInMaxPeriod = optInMaxPeriod;
    sp.optInMAType = optInMAType;
    sp.bank = [];
    mavpTapeOpen(sp, inReal, historyLen, lookbackTotal);
    outBegIdx.value = lookbackTotal;
    outNBElement.value = historyLen - lookbackTotal;
    sp.cur_outReal = outReal[outNBElement.value - 1];
    return RetCode.SUCCESS;
  }
  const seedPrefix = inReal.slice(0, lookbackTotal + 1);
  const bank: MaStream[] = [];
  let reach = 0;
  for (let bankIdx = 0; bankIdx < nBank; bankIdx++) {
    const slotSub = maOpenInternal(core, seedPrefix, lookbackTotal, optInMinPeriod + bankIdx, optInMAType as MAType);
    bank.push(slotSub);
    const slotReach = maTapeDetach(slotSub);
    if (slotReach > reach) {
      reach = slotReach;
    }
  }
  let cp = optInMinPeriod;
  if (inPeriods[lookbackTotal] > optInMaxPeriod) {
    cp = optInMaxPeriod;
  } else if (inPeriods[lookbackTotal] >= optInMinPeriod) {
    cp = inPeriods[lookbackTotal] | 0;
  }
  sp.optInMinPeriod = optInMinPeriod;
  sp.optInMaxPeriod = optInMaxPeriod;
  sp.optInMAType = optInMAType;
  sp.bank = bank;
  mavpTapeOpen(sp, seedPrefix, lookbackTotal + 1, reach);
  outReal[0] = bank[cp - optInMinPeriod].cur_outReal;
  for (let t = lookbackTotal + 1; t < historyLen; t++) {
    mavpStepImpl(sp, inReal[t], inPeriods[t]);
    outReal[t - lookbackTotal] = sp.cur_outReal;
  }
  outBegIdx.value = lookbackTotal;
  outNBElement.value = historyLen - lookbackTotal;
  sp.cur_outReal = outReal[outNBElement.value - 1];
  return RetCode.SUCCESS;
}

/** @internal */
export function mavpOpenInternal(
  core: Core,
  inReal: Float64Array,
  inPeriods: Float64Array,
  startIdx: number,
  optInMinPeriod: number,
  optInMaxPeriod: number,
  optInMAType: number,
): MavpStream {
  const sp = MavpStream.of(core);
  const retCode = mavpOpenImpl(sp, inReal, inPeriods, startIdx, optInMinPeriod, optInMaxPeriod, optInMAType);
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MAVP open', inReal.length, startIdx, mavpLookback(optInMinPeriod, optInMaxPeriod, optInMAType as MAType, core));
  }
  throw streamFailure('MAVP open', retCode);
}

/**
 * Opens a live MAVP stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link mavp} there.
 *
 * @param inReal - Warm-up history. series to be averaged. Every history has the
 *   same length.
 * @param inPeriods - Warm-up history. per-bar desired MA period. Every history has
 *   the same length.
 * @param optInMinPeriod - Lower clamp for the per-bar period (default 2; range 1
 *   to 10000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInMaxPeriod - Upper clamp for the per-bar period (default 30; range 1
 *   to 10000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInMAType - Moving-average type applied (default `MAType.SMA`; values:
 *   0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA,
 *   10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
 *   `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `mavpLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { MAType, mavpOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const periods = Float64Array.from({ length: 252 }, (_, i) => 5 + (i % 10));
 * const stream = mavpOpen(data, periods, 2, 30, MAType.SMA);
 * const peeked = stream.peek(100.9, 14); // a forming bar: commits nothing
 * const updated = stream.update(100.9, 14); // the bar closes
 * ```
 */
export function mavpOpen(
  inReal: ArrayLike<number>,
  inPeriods: ArrayLike<number>,
  optInMinPeriod: number,
  optInMaxPeriod: number,
  optInMAType: MAType | typeof Core.INTEGER_DEFAULT,
  core: Core = Core.DEFAULT,
): MavpStream {
  optInMinPeriod = requireInt32('MAVP open', 'optInMinPeriod', optInMinPeriod);
  optInMaxPeriod = requireInt32('MAVP open', 'optInMaxPeriod', optInMaxPeriod);
  optInMAType = requireInt32('MAVP open', 'optInMAType', optInMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('MAVP open', 'inReal', inReal);
  requireHistory('MAVP open', inReal.length);
  requireArgument('MAVP open', 'inPeriods', inPeriods);
  requireHistoryLength('MAVP open', 'inPeriods', inPeriods.length, inReal.length);
  return mavpOpenInternal(core, float64(inReal), float64(inPeriods), 0, optInMinPeriod, optInMaxPeriod, optInMAType);
}

/**
 * {@link mavpOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link mavp}. The range written is the stream's
 * `outRange()`.
 *
 * @param inReal - Warm-up history. series to be averaged. Every history has the
 *   same length.
 * @param inPeriods - Warm-up history. per-bar desired MA period. Every history has
 *   the same length.
 * @param optInMinPeriod - Lower clamp for the per-bar period (default 2; range 1
 *   to 10000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInMaxPeriod - Upper clamp for the per-bar period (default 30; range 1
 *   to 10000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInMAType - Moving-average type applied (default `MAType.SMA`; values:
 *   0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA,
 *   10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
 *   `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param outReal - variable-period moving average. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `mavpLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range, histories of different lengths or an output too
 *   short or starting where another output or a `Float64Array` input starts, or, on
 *   a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { MAType, mavpOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const periods = Float64Array.from({ length: 252 }, (_, i) => 5 + (i % 10));
 * const out = new Float64Array(data.length);
 * const stream = mavpOpenAndFill(data, periods, 2, 30, MAType.SMA, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function mavpOpenAndFill(
  inReal: ArrayLike<number>,
  inPeriods: ArrayLike<number>,
  optInMinPeriod: number,
  optInMaxPeriod: number,
  optInMAType: MAType | typeof Core.INTEGER_DEFAULT,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): MavpStream {
  optInMinPeriod = requireInt32('MAVP openAndFill', 'optInMinPeriod', optInMinPeriod);
  optInMaxPeriod = requireInt32('MAVP openAndFill', 'optInMaxPeriod', optInMaxPeriod);
  optInMAType = requireInt32('MAVP openAndFill', 'optInMAType', optInMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('MAVP openAndFill', 'inReal', inReal);
  requireHistory('MAVP openAndFill', inReal.length);
  requireArgument('MAVP openAndFill', 'inPeriods', inPeriods);
  const guardOutLen = openFillCount('MAVP openAndFill', inReal.length, mavpLookback(optInMinPeriod, optInMaxPeriod, optInMAType as MAType, core));
  requireHistoryLength('MAVP openAndFill', 'inPeriods', inPeriods.length, inReal.length);
  requireLength('MAVP openAndFill', 'outReal', outReal, guardOutLen);
  const inRealF64 = float64(inReal);
  const inPeriodsF64 = float64(inPeriods);
  const sp = MavpStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = mavpOpenAndFillImpl(sp, inRealF64, inPeriodsF64, optInMinPeriod, optInMaxPeriod, optInMAType, outBegIdx, outNBElement, outReal);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MAVP openAndFill', inReal.length, 0, mavpLookback(optInMinPeriod, optInMaxPeriod, optInMAType as MAType, core));
  }
  throw streamFailure('MAVP openAndFill', retCode);
}
