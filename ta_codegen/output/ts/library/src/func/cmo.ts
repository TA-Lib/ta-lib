/*! TA-LIB Copyright (c) 1999-2026, Mario Fortier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither name of author nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


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
import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function cmoLookbackPositional(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
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

/** @internal */
export function cmoDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (cmoLookbackPositional(optInTimePeriod, core) < 0) {
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
  lookbackTotal = cmoLookbackPositional(optInTimePeriod, core);
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

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000']] as const;

/** @internal */
export function cmoPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CMO', 'startIdx', startIdx);
  endIdx = requireInt32('CMO', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('CMO', 'timePeriod', optInTimePeriod);
  requireIndexRange('CMO', startIdx, endIdx);
  const guardLookback = cmoLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('CMO', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('CMO', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CMO', 'real', inReal, guardInLen);
  requireOutput('CMO', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('CMO', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cmoImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CMO', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cmo}, {@link cmoLookback}, {@link cmoDisplayShift},
 * {@link cmoOpen} and {@link cmoOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface CmoOptions {
  /**
   * Bars over which gains/losses are smoothed (default 14; range 2 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
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

/** What {@link cmo} returns, and {@link cmoOpenAndFill} with its stream. */
export interface CmoResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /** CMO oscillator value. `values[k]` is the value at input bar `begIdx + k`. */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link cmo} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CmoOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cmoLookback(options?: CmoOptions): number {
  const o = optionsOf('CMO lookback', options);
  return cmoLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('CMO lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cmo}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CmoOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cmoDisplayShift(outputIdx: number, options?: CmoOptions): number {
  const o = optionsOf('CMO displayShift', options);
  return cmoDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('CMO displayShift', o.core));
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
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cmoLookback} is a success with empty outputs.
 *
 * @param real - Source price/value series.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link CmoOptions}.
 * @returns `begIdx` and the values; see {@link CmoResult}. Each output is a view
 *   of its buffer, the caller's or a new one.
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
 * import { cmo } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = cmo(data, { timePeriod: 14 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`rsi`](https://ta-lib.org/functions/rsi)
 */
export function cmo(real: ArrayLike<number>, options?: CmoOptions): CmoResult {
  const o = optionsOf('CMO', options);
  if (o.endIdx === undefined) {
    requireSeries('CMO', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('CMO', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cmoLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = cmoPositional(startIdx, endIdx, real, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
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

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): CmoStream {
    return new CmoStream(core);
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
      throw streamFailure('CMO advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw streamFailure('CMO update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('CMO update', 'real');
    }
    cmoStepImpl(this, real);
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
      throw nonFiniteBar('CMO peek', 'real');
    }
    const sp = this;
    const inReal = real;
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
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  /* Adjust startIdx to account for the lookback period. */
  lookbackTotal = cmoLookbackPositional(optInTimePeriod, core);
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
  const sp = CmoStream.of(core);
  const retCode = cmoOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CMO openAndFill', inReal.length, startIdx, cmoLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && cmoLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('CMO openAndFill', PARAM_RANGES, optInTimePeriod);
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
  const sp = CmoStream.of(core);
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
    throw insufficientHistory('CMO open', inReal.length, startIdx, cmoLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && cmoLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('CMO open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('CMO open', retCode);
}

/** @internal */
export function cmoOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): CmoStream {
  optInTimePeriod = requireInt32('CMO open', 'timePeriod', optInTimePeriod);
  requireArgument('CMO open', 'real', inReal);
  requireHistory('CMO open', 'real', inReal.length);
  return cmoOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/** @internal */
export function cmoOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): CmoStream {
  optInTimePeriod = requireInt32('CMO openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('CMO openAndFill', 'real', inReal);
  requireHistory('CMO openAndFill', 'real', inReal.length);
  const guardLookback = cmoLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('CMO openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('CMO openAndFill', inReal.length, guardLookback);
  requireOutput('CMO openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('CMO openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('CMO openAndFill', 'outReal', 'real');
  }
  return cmoOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live CMO stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link cmo} there.
 *
 * @param real - Warm-up history. Source price/value series.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CmoOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cmoLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { cmoOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = cmoOpen(data, { timePeriod: 14 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function cmoOpen(real: ArrayLike<number>, options?: CmoOptions): CmoStream {
  const o = optionsOf('CMO open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('CMO open', o.core);
  return cmoOpenPositional(real, optInTimePeriod, core);
}

/**
 * {@link cmoOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link cmo}.
 *
 * @param real - Warm-up history. Source price/value series.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CmoOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cmo} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cmoLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or a
 *   buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cmoOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = cmoOpenAndFill(data, { timePeriod: 14 });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cmoOpenAndFill(real: ArrayLike<number>, options?: CmoOptions): CmoResult & { stream: CmoStream } {
  const o = optionsOf('CMO openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('CMO openAndFill', o.core);
  const capacity = fillCapacity(cmoLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = cmoOpenAndFillPositional(real, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
