/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  100826 KL,CC  Creation (#487).
 *  101126 MF,CC  The extension step is a select; the pivot index is derived (#546).
 */

import { badSink, clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireReal, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link zigzag} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInSensitivity - Minimum move away from the current extreme that
 *   reverses the leg, in percent (default 5; range 0 to 100; `Core.REAL_DEFAULT`
 *   selects the default).
 * @param optInMinTrendLength - Minimum number of bars between two pivots (default
 *   1; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function zigzagLookback(optInSensitivity: number, optInMinTrendLength: number, core: Core = Core.DEFAULT): number {
  void core;
  if (typeof optInSensitivity !== 'number') {
    return -1;
  }
  if ((optInMinTrendLength | 0) !== optInMinTrendLength) {
    return -1;
  }
  if (optInSensitivity === Core.REAL_DEFAULT) {
    optInSensitivity = 5.0;
  } else if (!(optInSensitivity >= 0e0 && optInSensitivity <= 1e2)) {
    return -1;
  }
  if (optInMinTrendLength === Core.INTEGER_DEFAULT) {
    optInMinTrendLength = 1;
  } else if (optInMinTrendLength < 1 || optInMinTrendLength > 100000) {
    return -1;
  }
  /* The first bar at which a reversal off the seed can fire. The gate counts
   * from the pivot's OWN bar and the seed is a pivot like any other, so the
   * earliest reversal is optInMinTrendLength bars after the seed, which sits
   * that many bars before the first output.
   *
   * Independent of the sensitivity: the threshold decides WHETHER a reversal
   * fires, never how early it may.
   */
  return optInMinTrendLength;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link zigzag}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInSensitivity - Minimum move away from the current extreme that
 *   reverses the leg, in percent (default 5; range 0 to 100; `Core.REAL_DEFAULT`
 *   selects the default).
 * @param optInMinTrendLength - Minimum number of bars between two pivots (default
 *   1; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function zigzagDisplayShift(optInSensitivity: number, optInMinTrendLength: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (zigzagLookback(optInSensitivity, optInMinTrendLength, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 3) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function zigzagImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  optInSensitivity: number,
  optInMinTrendLength: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outZigZag: Float64Array,
  outTrend: Int32Array,
  outPivotIdx: Int32Array,
): RetCode {
  let lookbackTotal = 0;
  let today = 0;
  let outIdx = 0;
  let trend = 0;
  let barsSincePivot = 0;
  let barIdx = 0;
  let isExt = 0;
  let s = 0;
  let up = 0;
  let dn = 0;
  let pivot = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInSensitivity === Core.REAL_DEFAULT) {
    optInSensitivity = 5.0;
  } else if (!(optInSensitivity >= 0e0 && optInSensitivity <= 1e2)) {
    return RetCode.BAD_PARAM;
  }
  if (optInMinTrendLength === Core.INTEGER_DEFAULT) {
    optInMinTrendLength = 1;
  } else if (optInMinTrendLength < 1 || optInMinTrendLength > 100000) {
    return RetCode.BAD_PARAM;
  }
  if ((outZigZag.buffer === outTrend.buffer && outZigZag.byteOffset === outTrend.byteOffset) || (outZigZag.buffer === outPivotIdx.buffer && outZigZag.byteOffset === outPivotIdx.byteOffset) || (outTrend.buffer === outPivotIdx.buffer && outTrend.byteOffset === outPivotIdx.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  lookbackTotal = zigzagLookback(optInSensitivity, optInMinTrendLength, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  /* Percent in, fraction inside. Each factor is rounded once, here, rather
   * than rebuilt per bar: the threshold is `pivot * up` in binary64, which is
   * the expression order the oracle uses.
   */
  s = optInSensitivity / 100.0;
  up = 1.0 + s;
  dn = 1.0 - s;
  /* The seed: a down leg whose extreme is the low of the bar
   * optInMinTrendLength before the first output. Because the gate counts the
   * seed bar like every other pivot, the first reversal can fire exactly at
   * the first output bar, not one later.
   */
  today = startIdx - lookbackTotal;
  trend = -1;
  pivot = inLow[today];
  barIdx = today;
  barsSincePivot = 0;
  today = today + 1;
  outIdx = 0;
  while (today <= endIdx) {
    /* The pivot's bar is barIdx - barsSincePivot. Both are carried: a stream
     * has no cursor to read a bar number from.
     */
    barsSincePivot = barsSincePivot + 1;
    barIdx = barIdx + 1;
    /* Reversal is tested BEFORE extension. On an outside bar that both makes
     * a new extreme and clears the threshold, the leg reverses and the old
     * pivot stays where it was; testing extension first would move the pivot
     * and lose the reversal, which is a whole leg of difference rather than a
     * rounding.
     */
    if (trend === -1) {
      if (inHigh[today] >= pivot * up && barsSincePivot >= optInMinTrendLength) {
        trend = 1;
        pivot = inHigh[today];
        barsSincePivot = 0;
      } else {
        /* Ties extend: at an equal price the pivot moves to the later bar,
         * which only outPivotIdx shows.
         *
         * A select, not a branch: on moving prices the extension is taken
         * on a share of the bars no predictor follows.
         */
        isExt = (inLow[today] <= pivot ? 1 : 0);
        pivot = (pivot < inLow[today]) ? pivot : inLow[today];
        barsSincePivot = barsSincePivot - ((isExt * barsSincePivot) | 0);
      }
    } else if (inLow[today] <= pivot * dn && barsSincePivot >= optInMinTrendLength) {
      trend = -1;
      pivot = inLow[today];
      barsSincePivot = 0;
    } else {
      isExt = (inHigh[today] >= pivot ? 1 : 0);
      pivot = (pivot > inHigh[today]) ? pivot : inHigh[today];
      barsSincePivot = barsSincePivot - ((isExt * barsSincePivot) | 0);
    }
    if (today >= startIdx) {
      outZigZag[outIdx] = pivot;
      outTrend[outIdx] = trend;
      outPivotIdx[outIdx] = barIdx - barsSincePivot;
      outIdx = outIdx + 1;
    }
    today = today + 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * The percent-filter swing line. A leg runs from the last confirmed pivot to the
 * most extreme price since it, and it reverses only when price moves at least
 * `optInSensitivity` percent away from that extreme and at least
 * `optInMinTrendLength` bars have passed since the pivot's own bar. Achelis
 * describes the chart form and warns that *"the last 'leg' displayed in a Zig Zag
 * chart can change"*. This function does not emit the chart line, which is drawn
 * with hindsight. It emits the causal state the line is drawn from, one row per
 * bar and final once emitted: the price of the swing extreme currently being
 * tracked, the absolute bar index of that extreme, and whether the current leg is
 * up or down. The chart is rebuilt exactly from the outputs and the seed. A pivot
 * is confirmed at bar `i > outBegIdx` whenever `outTrend[i]` differs from
 * `outTrend[i-1]`, and that pivot is `(outPivotIdx[i-1], outZigZag[i-1])`. When
 * `outTrend[outBegIdx]` is +1 a reversal fired on the first output bar, and the
 * pivot it confirmed is the seed,
 * `(outBegIdx - optInMinTrendLength, inLow[outBegIdx - optInMinTrendLength])`,
 * which no output row carries. The last, still-open pivot is
 * `(outPivotIdx[last], outZigZag[last])` — the leg Achelis warns about.
 *
 * Formula and more info at
 * [ta-lib.org/functions/zigzag](https://ta-lib.org/functions/zigzag).
 *
 * **Notes**
 * - **Reversal is tested before extension.** On an outside bar that both makes a
 *   new extreme and clears the threshold, the leg reverses and the old pivot stays
 *   where it was. The other order moves the pivot to the new extreme and defers
 *   the reversal to the next bar, against a threshold that has just moved further
 *   away: on the 252-bar regtest corpus at 5 percent that costs one bar (R TTR,
 *   which takes that order, places the swing low at 148 where this places it at
 *   147), but a range narrower than the sensitivity after the outside bar keeps it
 *   from ever firing, and the leg is then lost rather than delayed.
 * - **Ties extend.** The comparisons are `<=` and `>=`, so at an equal price the
 *   pivot moves to the later bar: `outZigZag` does not change and `outPivotIdx`
 *   does. That movement is the only thing the index output carries and the price
 *   output cannot.
 * - **The gate counts from the pivot's own bar, the seed included.** No reversal
 *   can fire before the first output bar; the first one can fire exactly there.
 * - **The seed is a low.** The first reversal is therefore always upward. A down
 *   reversal compares a bar's low with the tracked high, so a rising series stays
 *   on its up leg only while no low falls `optInSensitivity` percent under that
 *   high on a bar the gate allows.
 * - **The input must be positive.** The threshold is a percent of the pivot: at a
 *   pivot of 0 it sits on the pivot and below 0 on the wrong side of it, so the
 *   leg reverses on nearly every bar the gate allows. The output is undefined for
 *   such input and no error is returned. A series that crosses zero, such as an
 *   oscillator or a spread, needs an absolute reversal amount, which this function
 *   does not have.
 * - The lookback is `optInMinTrendLength` and does not depend on the sensitivity:
 *   the threshold decides whether a reversal fires, never how early it may.
 * - `optInSensitivity` is a percent, as Achelis, StockCharts, TTR, Skender and
 *   TradingView take it. 0 and 100 are degenerate but defined: at 0 a reversal
 *   fires on almost every bar the gate allows, and at 100 no downward reversal
 *   fires on positive prices.
 * - The value at a bar depends on where the caller started, through the seed; the
 *   function is `path_dependent` for that reason, as SAR and SUPERTREND are.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link zigzagLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param optInSensitivity - Minimum move away from the current extreme that
 *   reverses the leg, in percent (default 5; range 0 to 100; `Core.REAL_DEFAULT`
 *   selects the default).
 * @param optInMinTrendLength - Minimum number of bars between two pivots (default
 *   1; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outZigZag - Price of the swing extreme currently being tracked. Must hold
 *   the values produced and never be empty; the input length always suffices.
 * @param outTrend - +1 while the leg is up, -1 while it is down. Must hold the
 *   values produced and never be empty; the input length always suffices.
 * @param outPivotIdx - Absolute index, into the input, of the bar that extreme
 *   sits on. Must hold the values produced and never be empty; the input length
 *   always suffices.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The range written: `begIdx` is the first bar with a value, `count` how
 *   many were written.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for an argument that is not an int32, a
 *   real parameter that is not a number, a parameter outside its range, an array too
 *   short for the range, an empty output, two outputs sharing memory or an output
 *   sharing memory with a `Float64Array` input it is not. Checked before anything is
 *   written.
 *
 * @example
 * ```ts
 * import { zigzag } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const zigZag = new Float64Array(high.length);
 * const trend = new Int32Array(high.length);
 * const pivotIdx = new Int32Array(high.length);
 * const { begIdx, count } = zigzag(
 *   0, high.length - 1,
 *   high, low,
 *   5, 1,
 *   zigZag, trend, pivotIdx,
 * );
 * // zigZag[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`sar`](https://ta-lib.org/functions/sar)
 * @see [`supertrend`](https://ta-lib.org/functions/supertrend)
 * @see [`minmaxindex`](https://ta-lib.org/functions/minmaxindex)
 * @see [`midprice`](https://ta-lib.org/functions/midprice)
 */
export function zigzag(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInSensitivity: number,
  optInMinTrendLength: number,
  outZigZag: Float64Array,
  outTrend: Int32Array,
  outPivotIdx: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('ZIGZAG', 'startIdx', startIdx);
  endIdx = requireInt32('ZIGZAG', 'endIdx', endIdx);
  optInSensitivity = requireReal('ZIGZAG', 'optInSensitivity', optInSensitivity);
  optInMinTrendLength = requireInt32('ZIGZAG', 'optInMinTrendLength', optInMinTrendLength);
  requireIndexRange('ZIGZAG', startIdx, endIdx);
  const guardStart = clampedStart('ZIGZAG', startIdx, zigzagLookback(optInSensitivity, optInMinTrendLength, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('ZIGZAG', 'inHigh', inHigh, guardInLen);
  requireLength('ZIGZAG', 'inLow', inLow, guardInLen);
  requireLength('ZIGZAG', 'outZigZag', outZigZag, guardOutLen);
  requireLength('ZIGZAG', 'outTrend', outTrend, guardOutLen);
  requireLength('ZIGZAG', 'outPivotIdx', outPivotIdx, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlapsPartly(outZigZag, inHighF64) || overlapsPartly(outZigZag, inLowF64) || overlaps(outTrend, inHighF64) || overlaps(outTrend, inLowF64) || overlaps(outPivotIdx, inHighF64) || overlaps(outPivotIdx, inLowF64) || overlaps(outZigZag, outTrend) || overlaps(outZigZag, outPivotIdx) || overlaps(outTrend, outPivotIdx)) {
    throw failure('ZIGZAG', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = zigzagImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInSensitivity, optInMinTrendLength, outBegIdx, outNBElement, outZigZag, outTrend, outPivotIdx);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('ZIGZAG', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live ZIGZAG stream: one value per closed bar, bit-identical to {@link zigzag}
 * over the same series. Open it with {@link zigzagOpen} or
 * {@link zigzagOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/zigzag](https://ta-lib.org/functions/zigzag).
 */
export class ZigzagStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInSensitivity: number = 0;
  /** @internal */
  optInMinTrendLength: number = 0;
  /** @internal */
  trend: number = 0;
  /** @internal */
  barsSincePivot: number = 0;
  /** @internal */
  barIdx: number = 0;
  /** @internal */
  up: number = 0;
  /** @internal */
  dn: number = 0;
  /** @internal */
  pivot: number = 0;
  /** @internal */
  cur_outZigZag: number = 0;
  /** @internal */
  cur_outTrend: number = 0;
  /** @internal */
  cur_outPivotIdx: number = 0;
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
   * {@link zigzag} reports them over the same bars. Each accepted `update` and each
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
      throw failure('ZIGZAG advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar, writing the new values into `out`.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number, out: ZigzagOut): void {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('ZIGZAG update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (typeof out !== 'object' || out === null) {
      throw badSink('ZIGZAG update', 'ZigzagOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('ZIGZAG update', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    zigzagStepImpl(this, inHigh, inLow);
    this.outRangeCount++;
    out.zigZag = this.cur_outZigZag;
    out.trend = this.cur_outTrend;
    out.pivotIdx = this.cur_outPivotIdx;
  }

  /**
   * Writes into `out` what the next `update` with the same bar would write,
   * committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number, out: ZigzagOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('ZIGZAG peek', 'ZigzagOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('ZIGZAG peek', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    const sp = this;
    let isExt = 0;
    let barIdx = sp.barIdx;
    let barsSincePivot = sp.barsSincePivot;
    let cur_outPivotIdx = 0;
    let cur_outTrend = 0;
    let cur_outZigZag = 0;
    let pivot = sp.pivot;
    let trend = sp.trend;
    /* The pivot's bar is barIdx - barsSincePivot. Both are carried: a stream
     * has no cursor to read a bar number from.
     */
    barsSincePivot = barsSincePivot + 1;
    barIdx = barIdx + 1;
    /* Reversal is tested BEFORE extension. On an outside bar that both makes
     * a new extreme and clears the threshold, the leg reverses and the old
     * pivot stays where it was; testing extension first would move the pivot
     * and lose the reversal, which is a whole leg of difference rather than a
     * rounding.
     */
    if (trend === -1) {
      if (inHigh >= pivot * sp.up && barsSincePivot >= sp.optInMinTrendLength) {
        trend = 1;
        pivot = inHigh;
        barsSincePivot = 0;
      } else {
        /* Ties extend: at an equal price the pivot moves to the later bar,
         * which only outPivotIdx shows.
         *
         * A select, not a branch: on moving prices the extension is taken
         * on a share of the bars no predictor follows.
         */
        isExt = (inLow <= pivot ? 1 : 0);
        pivot = (pivot < inLow) ? pivot : inLow;
        barsSincePivot = barsSincePivot - ((isExt * barsSincePivot) | 0);
      }
    } else if (inLow <= pivot * sp.dn && barsSincePivot >= sp.optInMinTrendLength) {
      trend = -1;
      pivot = inLow;
      barsSincePivot = 0;
    } else {
      isExt = (inHigh >= pivot ? 1 : 0);
      pivot = (pivot > inHigh) ? pivot : inHigh;
      barsSincePivot = barsSincePivot - ((isExt * barsSincePivot) | 0);
    }
    cur_outZigZag = pivot;
    cur_outTrend = trend;
    cur_outPivotIdx = barIdx - barsSincePivot;
    out.zigZag = cur_outZigZag;
    out.trend = cur_outTrend;
    out.pivotIdx = cur_outPivotIdx;
  }

  /**
   * The value at the last bar this stream counted; `peek` does not change it.
   *
   * @param out - Receives the outputs.
   */
  value(out: ZigzagOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('ZIGZAG value', 'ZigzagOut');
    }
    out.zigZag = this.cur_outZigZag;
    out.trend = this.cur_outTrend;
    out.pivotIdx = this.cur_outPivotIdx;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): ZigzagStream {
    const c = new ZigzagStream(this.core);
    c.optInSensitivity = this.optInSensitivity;
    c.optInMinTrendLength = this.optInMinTrendLength;
    c.trend = this.trend;
    c.barsSincePivot = this.barsSincePivot;
    c.barIdx = this.barIdx;
    c.up = this.up;
    c.dn = this.dn;
    c.pivot = this.pivot;
    c.cur_outZigZag = this.cur_outZigZag;
    c.cur_outTrend = this.cur_outTrend;
    c.cur_outPivotIdx = this.cur_outPivotIdx;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one ZIGZAG bar, written into an object the caller owns and
 * reuses: valid until the next call that writes it.
 */
export class ZigzagOut {
  /** Price of the swing extreme currently being tracked. */
  zigZag: number = 0;
  /** +1 while the leg is up, -1 while it is down. */
  trend: number = 0;
  /** Absolute index, into the input, of the bar that extreme sits on. */
  pivotIdx: number = 0;
}

function zigzagStepImpl(sp: ZigzagStream, inHigh: number, inLow: number): void {
  let isExt = 0;
  /* The pivot's bar is barIdx - barsSincePivot. Both are carried: a stream
   * has no cursor to read a bar number from.
   */
  sp.barsSincePivot = sp.barsSincePivot + 1;
  sp.barIdx = sp.barIdx + 1;
  /* Reversal is tested BEFORE extension. On an outside bar that both makes
   * a new extreme and clears the threshold, the leg reverses and the old
   * pivot stays where it was; testing extension first would move the pivot
   * and lose the reversal, which is a whole leg of difference rather than a
   * rounding.
   */
  if (sp.trend === -1) {
    if (inHigh >= sp.pivot * sp.up && sp.barsSincePivot >= sp.optInMinTrendLength) {
      sp.trend = 1;
      sp.pivot = inHigh;
      sp.barsSincePivot = 0;
    } else {
      /* Ties extend: at an equal price the pivot moves to the later bar,
       * which only outPivotIdx shows.
       *
       * A select, not a branch: on moving prices the extension is taken
       * on a share of the bars no predictor follows.
       */
      isExt = (inLow <= sp.pivot ? 1 : 0);
      sp.pivot = (sp.pivot < inLow) ? sp.pivot : inLow;
      sp.barsSincePivot = sp.barsSincePivot - ((isExt * sp.barsSincePivot) | 0);
    }
  } else if (inLow <= sp.pivot * sp.dn && sp.barsSincePivot >= sp.optInMinTrendLength) {
    sp.trend = -1;
    sp.pivot = inLow;
    sp.barsSincePivot = 0;
  } else {
    isExt = (inHigh >= sp.pivot ? 1 : 0);
    sp.pivot = (sp.pivot > inHigh) ? sp.pivot : inHigh;
    sp.barsSincePivot = sp.barsSincePivot - ((isExt * sp.barsSincePivot) | 0);
  }
  sp.cur_outZigZag = sp.pivot;
  sp.cur_outTrend = sp.trend;
  sp.cur_outPivotIdx = sp.barIdx - sp.barsSincePivot;
}

function zigzagOpenImpl(
  sp: ZigzagStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInSensitivity: number,
  optInMinTrendLength: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outZigZag: Float64Array,
  outTrend: Int32Array,
  outPivotIdx: Int32Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let lookbackTotal = 0;
  let today = 0;
  let outIdx = 0;
  let trend = 0;
  let barsSincePivot = 0;
  let barIdx = 0;
  let isExt = 0;
  let s = 0;
  let up = 0;
  let dn = 0;
  let pivot = 0;
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
  if (optInSensitivity === Core.REAL_DEFAULT) {
    optInSensitivity = 5.0;
  } else if (!(optInSensitivity >= 0e0 && optInSensitivity <= 1e2)) {
    return RetCode.BAD_PARAM;
  }
  if (optInMinTrendLength === Core.INTEGER_DEFAULT) {
    optInMinTrendLength = 1;
  } else if (optInMinTrendLength < 1 || optInMinTrendLength > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  lookbackTotal = zigzagLookback(optInSensitivity, optInMinTrendLength, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Percent in, fraction inside. Each factor is rounded once, here, rather
   * than rebuilt per bar: the threshold is `pivot * up` in binary64, which is
   * the expression order the oracle uses.
   */
  s = optInSensitivity / 100.0;
  up = 1.0 + s;
  dn = 1.0 - s;
  /* The seed: a down leg whose extreme is the low of the bar
   * optInMinTrendLength before the first output. Because the gate counts the
   * seed bar like every other pivot, the first reversal can fire exactly at
   * the first output bar, not one later.
   */
  today = startIdx - lookbackTotal;
  trend = -1;
  pivot = inLow[today];
  barIdx = today;
  barsSincePivot = 0;
  today = today + 1;
  outIdx = 0;
  while (today <= endIdx) {
    /* The pivot's bar is barIdx - barsSincePivot. Both are carried: a stream
     * has no cursor to read a bar number from.
     */
    barsSincePivot = barsSincePivot + 1;
    barIdx = barIdx + 1;
    /* Reversal is tested BEFORE extension. On an outside bar that both makes
     * a new extreme and clears the threshold, the leg reverses and the old
     * pivot stays where it was; testing extension first would move the pivot
     * and lose the reversal, which is a whole leg of difference rather than a
     * rounding.
     */
    if (trend === -1) {
      if (inHigh[today] >= pivot * up && barsSincePivot >= optInMinTrendLength) {
        trend = 1;
        pivot = inHigh[today];
        barsSincePivot = 0;
      } else {
        /* Ties extend: at an equal price the pivot moves to the later bar,
         * which only outPivotIdx shows.
         *
         * A select, not a branch: on moving prices the extension is taken
         * on a share of the bars no predictor follows.
         */
        isExt = (inLow[today] <= pivot ? 1 : 0);
        pivot = (pivot < inLow[today]) ? pivot : inLow[today];
        barsSincePivot = barsSincePivot - ((isExt * barsSincePivot) | 0);
      }
    } else if (inLow[today] <= pivot * dn && barsSincePivot >= optInMinTrendLength) {
      trend = -1;
      pivot = inLow[today];
      barsSincePivot = 0;
    } else {
      isExt = (inHigh[today] >= pivot ? 1 : 0);
      pivot = (pivot > inHigh[today]) ? pivot : inHigh[today];
      barsSincePivot = barsSincePivot - ((isExt * barsSincePivot) | 0);
    }
    if (today >= startIdx) {
      outZigZag[((outIdx * outStride) | 0)] = pivot;
      outTrend[((outIdx * outStride) | 0)] = trend;
      outPivotIdx[((outIdx * outStride) | 0)] = barIdx - barsSincePivot;
      outIdx = outIdx + 1;
    }
    today = today + 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  sp.optInSensitivity = optInSensitivity;
  sp.optInMinTrendLength = optInMinTrendLength;
  sp.trend = trend;
  sp.barsSincePivot = barsSincePivot;
  sp.barIdx = barIdx;
  sp.up = up;
  sp.dn = dn;
  sp.pivot = pivot;
  sp.cur_outZigZag = outZigZag[(outNBElement.value - 1) * outStride];
  sp.cur_outTrend = outTrend[(outNBElement.value - 1) * outStride];
  sp.cur_outPivotIdx = outPivotIdx[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function zigzagOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInSensitivity: number,
  optInMinTrendLength: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outZigZag: Float64Array,
  outTrend: Int32Array,
  outPivotIdx: Int32Array,
): ZigzagStream {
  const sp = new ZigzagStream(core);
  const retCode = zigzagOpenImpl(sp, inHigh, inLow, startIdx, optInSensitivity, optInMinTrendLength, outBegIdx, outNBElement, outZigZag, outTrend, outPivotIdx, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ZIGZAG openAndFill', inHigh.length, startIdx, zigzagLookback(optInSensitivity, optInMinTrendLength, core));
  }
  throw streamFailure('ZIGZAG openAndFill', retCode);
}

/** @internal */
export function zigzagOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInSensitivity: number,
  optInMinTrendLength: number,
): ZigzagStream {
  const sp = new ZigzagStream(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outZigZag = new Float64Array(1);
  const sink_outTrend = new Int32Array(1);
  const sink_outPivotIdx = new Int32Array(1);
  const retCode = zigzagOpenImpl(sp, inHigh, inLow, startIdx, optInSensitivity, optInMinTrendLength, outBegIdx, outNBElement, sink_outZigZag, sink_outTrend, sink_outPivotIdx, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ZIGZAG open', inHigh.length, startIdx, zigzagLookback(optInSensitivity, optInMinTrendLength, core));
  }
  throw streamFailure('ZIGZAG open', retCode);
}

/**
 * Opens a live ZIGZAG stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link zigzag} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param optInSensitivity - Minimum move away from the current extreme that
 *   reverses the leg, in percent (default 5; range 0 to 100; `Core.REAL_DEFAULT`
 *   selects the default).
 * @param optInMinTrendLength - Minimum number of bars between two pivots (default
 *   1; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `zigzagLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a real
 *   parameter that is not a number, a parameter outside its range or histories of
 *   different lengths.
 *
 * @example
 * ```ts
 * import { ZigzagOut, zigzagOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = zigzagOpen(high, low, 5, 1);
 * const peeked = new ZigzagOut();
 * const updated = new ZigzagOut();
 * stream.peek(101.4, 99.1, peeked); // a forming bar: commits nothing
 * stream.update(101.4, 99.1, updated); // the bar closes
 * ```
 */
export function zigzagOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInSensitivity: number,
  optInMinTrendLength: number,
  core: Core = Core.DEFAULT,
): ZigzagStream {
  optInSensitivity = requireReal('ZIGZAG open', 'optInSensitivity', optInSensitivity);
  optInMinTrendLength = requireInt32('ZIGZAG open', 'optInMinTrendLength', optInMinTrendLength);
  requireArgument('ZIGZAG open', 'inHigh', inHigh);
  requireHistory('ZIGZAG open', inHigh.length);
  requireArgument('ZIGZAG open', 'inLow', inLow);
  requireHistoryLength('ZIGZAG open', 'inLow', inLow.length, inHigh.length);
  return zigzagOpenInternal(core, float64(inHigh), float64(inLow), 0, optInSensitivity, optInMinTrendLength);
}

/**
 * {@link zigzagOpen} that also fills the outputs over the whole history in the
 * same pass, bit-identically to {@link zigzag}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param optInSensitivity - Minimum move away from the current extreme that
 *   reverses the leg, in percent (default 5; range 0 to 100; `Core.REAL_DEFAULT`
 *   selects the default).
 * @param optInMinTrendLength - Minimum number of bars between two pivots (default
 *   1; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outZigZag - Price of the swing extreme currently being tracked. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param outTrend - +1 while the leg is up, -1 while it is down. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param outPivotIdx - Absolute index, into the input, of the bar that extreme
 *   sits on. Must hold `historyLen - lookback` values and share no memory with an
 *   input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `zigzagLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a real
 *   parameter that is not a number, a parameter outside its range, histories of
 *   different lengths or an output too short or starting where another output or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { zigzagOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const zigZag = new Float64Array(high.length);
 * const trend = new Int32Array(high.length);
 * const pivotIdx = new Int32Array(high.length);
 * const stream = zigzagOpenAndFill(high, low, 5, 1, zigZag, trend, pivotIdx);
 * const { begIdx, count } = stream.outRange();
 * // zigZag[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function zigzagOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInSensitivity: number,
  optInMinTrendLength: number,
  outZigZag: Float64Array,
  outTrend: Int32Array,
  outPivotIdx: Int32Array,
  core: Core = Core.DEFAULT,
): ZigzagStream {
  optInSensitivity = requireReal('ZIGZAG openAndFill', 'optInSensitivity', optInSensitivity);
  optInMinTrendLength = requireInt32('ZIGZAG openAndFill', 'optInMinTrendLength', optInMinTrendLength);
  requireArgument('ZIGZAG openAndFill', 'inHigh', inHigh);
  requireHistory('ZIGZAG openAndFill', inHigh.length);
  requireArgument('ZIGZAG openAndFill', 'inLow', inLow);
  const guardOutLen = openFillCount('ZIGZAG openAndFill', inHigh.length, zigzagLookback(optInSensitivity, optInMinTrendLength, core));
  requireHistoryLength('ZIGZAG openAndFill', 'inLow', inLow.length, inHigh.length);
  requireLength('ZIGZAG openAndFill', 'outZigZag', outZigZag, guardOutLen);
  requireLength('ZIGZAG openAndFill', 'outTrend', outTrend, guardOutLen);
  requireLength('ZIGZAG openAndFill', 'outPivotIdx', outPivotIdx, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if ((outZigZag.buffer === inHighF64.buffer && outZigZag.byteOffset === inHighF64.byteOffset) || (outZigZag.buffer === inLowF64.buffer && outZigZag.byteOffset === inLowF64.byteOffset) || (outTrend.buffer === inHighF64.buffer && outTrend.byteOffset === inHighF64.byteOffset) || (outTrend.buffer === inLowF64.buffer && outTrend.byteOffset === inLowF64.byteOffset) || (outPivotIdx.buffer === inHighF64.buffer && outPivotIdx.byteOffset === inHighF64.byteOffset) || (outPivotIdx.buffer === inLowF64.buffer && outPivotIdx.byteOffset === inLowF64.byteOffset) || (outZigZag.buffer === outTrend.buffer && outZigZag.byteOffset === outTrend.byteOffset) || (outZigZag.buffer === outPivotIdx.buffer && outZigZag.byteOffset === outPivotIdx.byteOffset) || (outTrend.buffer === outPivotIdx.buffer && outTrend.byteOffset === outPivotIdx.byteOffset)) {
    throw streamFailure('ZIGZAG openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outZigZag, inHighF64) || overlaps(outZigZag, inLowF64) || overlaps(outTrend, inHighF64) || overlaps(outTrend, inLowF64) || overlaps(outPivotIdx, inHighF64) || overlaps(outPivotIdx, inLowF64) || overlaps(outZigZag, outTrend) || overlaps(outZigZag, outPivotIdx) || overlaps(outTrend, outPivotIdx))) {
    throw streamFailure('ZIGZAG openAndFill', RetCode.BAD_PARAM);
  }
  return zigzagOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInSensitivity, optInMinTrendLength, new MInteger(), new MInteger(), outZigZag, outTrend, outPivotIdx);
}
