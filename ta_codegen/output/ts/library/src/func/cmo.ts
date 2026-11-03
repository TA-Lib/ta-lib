/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  BT       Barry Tsung
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY      Description
 *  -------------------------------------------------------------------
 *  112605 MF      Initial version.
 *  021806 MF,BT   Fix #1434450 reported by BT.
 *  082326 MF,CC   Fix #253. Test the gain+loss total exactly instead of against
 *                 the fixed TA_IS_ZERO band, which zeroed the oscillator for any
 *                 instrument quoted small enough to fall under it.
 *  091326 MF,CC   #411 Wilder step without a divide or a branch.
 *  092826 MF,CC   #466 Drop the period-1 copy-through; the range starts at 2.
 */

import { FuncUnstId } from '../enums.ts';
import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link cmo} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param optInTimePeriod - Bars over which gains/losses are smoothed (default 14;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function cmoLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  return optInTimePeriod + core.unstableCount(FuncUnstId.CMO, ((optInTimePeriod > 1) ? ((10 * optInTimePeriod) | 0) : 0), ((optInTimePeriod > 1) ? ((19 * optInTimePeriod) | 0) : 0));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cmo}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Bars over which gains/losses are smoothed (default 14;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function cmoDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (cmoLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cmoImpl(
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
  let lookbackTotal = 0;
  let i = 0;
  let gainDelta = 0;
  let prevGain = 0;
  let prevLoss = 0;
  let invPeriod = 0;
  let prevValue = 0;
  let tempValue1 = 0;
  let tempValue2 = 0;
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
  outBegIdx.value = 0;
  outNBElement.value = 0;
  /* Adjust startIdx to account for the lookback period. */
  lookbackTotal = cmoLookback(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  outIdx = 0;
  /* Index into the output. */
  /* The declaration order above sets invPeriod's place in the stream state,
   * and that place is load-bearing: a layout that lets Update load it paired
   * with a field the previous bar stored stalls every call. Re-measure Update
   * in C and Rust before reordering those declarations.
   */
  invPeriod = 1.0 / optInTimePeriod;
  /* Accumulate Wilder's "Average Gain" and "Average Loss"
   * among the initial period.
   */
  today = startIdx - lookbackTotal;
  prevValue = inReal[today];
  prevGain = 0.0;
  prevLoss = 0.0;
  today += 1;
  for (i = optInTimePeriod; i > 0; i -= 1) {
    tempValue1 = inReal[today++];
    tempValue2 = tempValue1 - prevValue;
    prevValue = tempValue1;
    gainDelta = (tempValue2 > 0.0) ? tempValue2 : 0.0;
    prevGain += gainDelta;
    prevLoss += gainDelta - tempValue2;
  }
  /* Subsequent prevLoss and prevGain are smoothed
   * using the previous values (Wilder's approach):
   *    prev += (today - prev) / 'period'
   * gainDelta - tempValue2 is the exact loss delta for every finite
   * tempValue2, so both accumulators step without a branch. Keep the step a
   * difference of two products: prev - (prev - today)*k lets the zero arm fold
   * to prev, which gcc compiles back into a branch, and prev + (today - prev)*k
   * becomes a fused multiply-add.
   */
  prevLoss /= optInTimePeriod;
  prevGain /= optInTimePeriod;
  /* Often documentation present the RSI calculation as follow:
   *    RSI = 100 - (100 / 1 + (prevGain/prevLoss))
   *
   * The following is equivalent:
   *    RSI = 100 * (prevGain/(prevGain+prevLoss))
   *
   * The second equation is used here for speed optimization.
   *
   * prevGain+prevLoss is a sum of non-negative magnitudes, so it is zero only
   * when every change since the seed was exactly zero -- test it exactly, never
   * against a fixed band. A gain carries the quote unit, so a constant put
   * against it zeroes a healthy oscillator for an instrument quoted below it
   * (issue #253).
   */
  if (today > startIdx) {
    tempValue1 = prevGain + prevLoss;
    if (tempValue1 > 0.0) {
      outReal[outIdx++] = 100.0 * ((prevGain - prevLoss) / tempValue1);
    } else {
      outReal[outIdx++] = 0.0;
    }
  } else {
    /* Skip the unstable period. Do the processing
     * but do not write it in the output.
     */
    while (today < startIdx) {
      tempValue1 = inReal[today];
      tempValue2 = tempValue1 - prevValue;
      prevValue = tempValue1;
      gainDelta = (tempValue2 > 0.0) ? tempValue2 : 0.0;
      prevGain += gainDelta * invPeriod - prevGain * invPeriod;
      prevLoss += (gainDelta - tempValue2) * invPeriod - prevLoss * invPeriod;
      today += 1;
    }
  }
  /* Unstable period skipped... now continue
   * processing if needed.
   */
  while (today <= endIdx) {
    tempValue1 = inReal[today++];
    tempValue2 = tempValue1 - prevValue;
    prevValue = tempValue1;
    gainDelta = (tempValue2 > 0.0) ? tempValue2 : 0.0;
    prevGain += gainDelta * invPeriod - prevGain * invPeriod;
    prevLoss += (gainDelta - tempValue2) * invPeriod - prevLoss * invPeriod;
    tempValue1 = prevGain + prevLoss;
    if (tempValue1 > 0.0) {
      outReal[outIdx++] = 100.0 * ((prevGain - prevLoss) / tempValue1);
    } else {
      outReal[outIdx++] = 0.0;
    }
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * Chande Momentum Oscillator: bounded momentum measure from Wilder-smoothed
 * average up-moves and down-moves. Identical to RSI except the numerator uses
 * (gain-loss) instead of gain. Bounded in \[-100,+100]; positive = net upward
 * momentum, negative = net downward.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cmo](https://ta-lib.org/functions/cmo).
 *
 * **Notes**
 * - Gains and losses are smoothed with Wilder's method (as in RSI) rather than the
 *   simple period sums of Chande's original definition.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link cmoLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - Source price/value series.
 * @param optInTimePeriod - Bars over which gains/losses are smoothed (default 14;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - CMO oscillator value. Must hold the values produced and never
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
 * import { cmo } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const { begIdx, count } = cmo(0, data.length - 1, data, 14, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`rsi`](https://ta-lib.org/functions/rsi)
 */
export function cmo(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CMO', 'startIdx', startIdx);
  endIdx = requireInt32('CMO', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('CMO', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('CMO', startIdx, endIdx);
  const guardStart = clampedStart('CMO', startIdx, cmoLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CMO', 'inReal', inReal, guardInLen);
  requireLength('CMO', 'outReal', outReal, guardOutLen);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw failure('CMO', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cmoImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CMO', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live CMO stream: one value per closed bar, bit-identical to {@link cmo} over
 * the same series. Open it with {@link cmoOpen} or {@link cmoOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cmo](https://ta-lib.org/functions/cmo).
 */
export class CmoStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  prevGain: number = 0;
  /** @internal */
  prevLoss: number = 0;
  /** @internal */
  invPeriod: number = 0;
  /** @internal */
  prevValue: number = 0;
  /** @internal */
  cur_outReal: number = 0;
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
   * {@link cmo} reports them over the same bars. Each accepted `update` and each
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
      throw failure('CMO advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw failure('CMO update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('CMO update', 'inReal');
    }
    cmoStepImpl(this, inReal);
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
      throw nonFiniteBar('CMO peek', 'inReal');
    }
    const sp = this;
    let gainDelta = 0;
    let tempValue1 = 0;
    let tempValue2 = 0;
    let cur_outReal = 0;
    let prevGain = sp.prevGain;
    let prevLoss = sp.prevLoss;
    let prevValue = sp.prevValue;
    tempValue1 = inReal;
    tempValue2 = tempValue1 - prevValue;
    prevValue = tempValue1;
    gainDelta = (tempValue2 > 0.0) ? tempValue2 : 0.0;
    prevGain += gainDelta * sp.invPeriod - prevGain * sp.invPeriod;
    prevLoss += (gainDelta - tempValue2) * sp.invPeriod - prevLoss * sp.invPeriod;
    tempValue1 = prevGain + prevLoss;
    if (tempValue1 > 0.0) {
      cur_outReal = 100.0 * ((prevGain - prevLoss) / tempValue1);
    } else {
      cur_outReal = 0.0;
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
  clone(): CmoStream {
    const c = new CmoStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.prevGain = this.prevGain;
    c.prevLoss = this.prevLoss;
    c.invPeriod = this.invPeriod;
    c.prevValue = this.prevValue;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cmoStepImpl(sp: CmoStream, inReal: number): void {
  let gainDelta = 0;
  let tempValue1 = 0;
  let tempValue2 = 0;
  tempValue1 = inReal;
  tempValue2 = tempValue1 - sp.prevValue;
  sp.prevValue = tempValue1;
  gainDelta = (tempValue2 > 0.0) ? tempValue2 : 0.0;
  sp.prevGain += gainDelta * sp.invPeriod - sp.prevGain * sp.invPeriod;
  sp.prevLoss += (gainDelta - tempValue2) * sp.invPeriod - sp.prevLoss * sp.invPeriod;
  tempValue1 = sp.prevGain + sp.prevLoss;
  if (tempValue1 > 0.0) {
    sp.cur_outReal = 100.0 * ((sp.prevGain - sp.prevLoss) / tempValue1);
  } else {
    sp.cur_outReal = 0.0;
  }
}

function cmoOpenImpl(
  sp: CmoStream,
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
  let lookbackTotal = 0;
  let i = 0;
  let gainDelta = 0;
  let prevGain = 0;
  let prevLoss = 0;
  let invPeriod = 0;
  let prevValue = 0;
  let tempValue1 = 0;
  let tempValue2 = 0;
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
  outBegIdx.value = 0;
  outNBElement.value = 0;
  /* Adjust startIdx to account for the lookback period. */
  lookbackTotal = cmoLookback(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outIdx = 0;
  /* Index into the output. */
  /* The declaration order above sets invPeriod's place in the stream state,
   * and that place is load-bearing: a layout that lets Update load it paired
   * with a field the previous bar stored stalls every call. Re-measure Update
   * in C and Rust before reordering those declarations.
   */
  invPeriod = 1.0 / optInTimePeriod;
  /* Accumulate Wilder's "Average Gain" and "Average Loss"
   * among the initial period.
   */
  today = startIdx - lookbackTotal;
  prevValue = inReal[today];
  prevGain = 0.0;
  prevLoss = 0.0;
  today += 1;
  for (i = optInTimePeriod; i > 0; i -= 1) {
    tempValue1 = inReal[today++];
    tempValue2 = tempValue1 - prevValue;
    prevValue = tempValue1;
    gainDelta = (tempValue2 > 0.0) ? tempValue2 : 0.0;
    prevGain += gainDelta;
    prevLoss += gainDelta - tempValue2;
  }
  /* Subsequent prevLoss and prevGain are smoothed
   * using the previous values (Wilder's approach):
   *    prev += (today - prev) / 'period'
   * gainDelta - tempValue2 is the exact loss delta for every finite
   * tempValue2, so both accumulators step without a branch. Keep the step a
   * difference of two products: prev - (prev - today)*k lets the zero arm fold
   * to prev, which gcc compiles back into a branch, and prev + (today - prev)*k
   * becomes a fused multiply-add.
   */
  prevLoss /= optInTimePeriod;
  prevGain /= optInTimePeriod;
  /* Often documentation present the RSI calculation as follow:
   *    RSI = 100 - (100 / 1 + (prevGain/prevLoss))
   *
   * The following is equivalent:
   *    RSI = 100 * (prevGain/(prevGain+prevLoss))
   *
   * The second equation is used here for speed optimization.
   *
   * prevGain+prevLoss is a sum of non-negative magnitudes, so it is zero only
   * when every change since the seed was exactly zero -- test it exactly, never
   * against a fixed band. A gain carries the quote unit, so a constant put
   * against it zeroes a healthy oscillator for an instrument quoted below it
   * (issue #253).
   */
  if (today > startIdx) {
    tempValue1 = prevGain + prevLoss;
    if (tempValue1 > 0.0) {
      outReal[((outIdx++ * outStride) | 0)] = 100.0 * ((prevGain - prevLoss) / tempValue1);
    } else {
      outReal[((outIdx++ * outStride) | 0)] = 0.0;
    }
  } else {
    /* Skip the unstable period. Do the processing
     * but do not write it in the output.
     */
    while (today < startIdx) {
      tempValue1 = inReal[today];
      tempValue2 = tempValue1 - prevValue;
      prevValue = tempValue1;
      gainDelta = (tempValue2 > 0.0) ? tempValue2 : 0.0;
      prevGain += gainDelta * invPeriod - prevGain * invPeriod;
      prevLoss += (gainDelta - tempValue2) * invPeriod - prevLoss * invPeriod;
      today += 1;
    }
  }
  /* Unstable period skipped... now continue
   * processing if needed.
   */
  while (today <= endIdx) {
    tempValue1 = inReal[today++];
    tempValue2 = tempValue1 - prevValue;
    prevValue = tempValue1;
    gainDelta = (tempValue2 > 0.0) ? tempValue2 : 0.0;
    prevGain += gainDelta * invPeriod - prevGain * invPeriod;
    prevLoss += (gainDelta - tempValue2) * invPeriod - prevLoss * invPeriod;
    tempValue1 = prevGain + prevLoss;
    if (tempValue1 > 0.0) {
      outReal[((outIdx++ * outStride) | 0)] = 100.0 * ((prevGain - prevLoss) / tempValue1);
    } else {
      outReal[((outIdx++ * outStride) | 0)] = 0.0;
    }
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  sp.optInTimePeriod = optInTimePeriod;
  sp.prevGain = prevGain;
  sp.prevLoss = prevLoss;
  sp.invPeriod = invPeriod;
  sp.prevValue = prevValue;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cmoOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): CmoStream {
  const sp = new CmoStream(core);
  const retCode = cmoOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CMO openAndFill', inReal.length, startIdx, cmoLookback(optInTimePeriod, core));
  }
  throw streamFailure('CMO openAndFill', retCode);
}

/** @internal */
export function cmoOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): CmoStream {
  const sp = new CmoStream(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = cmoOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CMO open', inReal.length, startIdx, cmoLookback(optInTimePeriod, core));
  }
  throw streamFailure('CMO open', retCode);
}

/**
 * Opens a live CMO stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link cmo} there.
 *
 * @param inReal - Warm-up history. Source price/value series.
 * @param optInTimePeriod - Bars over which gains/losses are smoothed (default 14;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cmoLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32 or a
 *   parameter outside its range.
 *
 * @example
 * ```ts
 * import { cmoOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = cmoOpen(data, 14);
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function cmoOpen(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): CmoStream {
  optInTimePeriod = requireInt32('CMO open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('CMO open', 'inReal', inReal);
  requireHistory('CMO open', inReal.length);
  return cmoOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/**
 * {@link cmoOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link cmo}. The range written is the stream's
 * `outRange()`.
 *
 * @param inReal - Warm-up history. Source price/value series.
 * @param optInTimePeriod - Bars over which gains/losses are smoothed (default 14;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - CMO oscillator value. Must hold `historyLen - lookback` values
 *   and share no memory with an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cmoLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or an output too short or starting where another
 *   output or a `Float64Array` input starts, or, on a history long enough to open,
 *   one sharing memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cmoOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const stream = cmoOpenAndFill(data, 14, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function cmoOpenAndFill(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): CmoStream {
  optInTimePeriod = requireInt32('CMO openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('CMO openAndFill', 'inReal', inReal);
  requireHistory('CMO openAndFill', inReal.length);
  const guardOutLen = openFillCount('CMO openAndFill', inReal.length, cmoLookback(optInTimePeriod, core));
  requireLength('CMO openAndFill', 'outReal', outReal, guardOutLen);
  const inRealF64 = float64(inReal);
  if ((outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset)) {
    throw streamFailure('CMO openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inRealF64))) {
    throw streamFailure('CMO openAndFill', RetCode.BAD_PARAM);
  }
  return cmoOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}
