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
 *  090426 MF,CC  Initial version (#363).
 *  100126 MF,CC  Display shift (#489).
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link dpo} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Number of bars spanned by the moving average being
 *   removed; the displacement is derived from it (default 20; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function dpoLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  /* The max is load-bearing, not cosmetic. The displaced read reaches back
   * optInTimePeriod/2+1 bars, which exceeds the moving average's own
   * optInTimePeriod-1 at a period of 2, and a bare optInTimePeriod-1 would
   * then read inReal[-1].
   */
  return ((optInTimePeriod - 1) > (((optInTimePeriod / 2) | 0) + 1) ? (optInTimePeriod - 1) : (((optInTimePeriod / 2) | 0) + 1));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link dpo}.
 *
 * The values are never shifted: this describes the drawing only.
 *
 * @param optInTimePeriod - Number of bars spanned by the moving average being
 *   removed; the displacement is derived from it (default 20; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function dpoDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (dpoLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  /* The value computed at a bar detrends the price this many bars back,
   * which is where a chart draws it.
   */
  return (-(((optInTimePeriod / 2) | 0) + 1) | 0);
}

/** @internal */
export function dpoImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let periodTotal = 0;
  let tempReal = 0;
  let dispVal = 0;
  let i = 0;
  let outIdx = 0;
  let trailingIdx = 0;
  let dispIdx = 0;
  let lookbackTotal = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  /* The running sum is accumulated, snapshot and divided in exactly the order
   * ta_codegen/input/sma/sma.c uses, so this fused loop is bit-for-bit equal
   * to a TA_SMA anchored at the same startIdx. That equality is what makes
   * the composite differential in test_dpo.c a memcmp instead of a tolerance
   * argument, and it is lost by dividing once into a reciprocal or by
   * reordering the add/snapshot/subtract.
   */
  lookbackTotal = dpoLookback(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  periodTotal = 0.0;
  trailingIdx = startIdx - (optInTimePeriod - 1);
  dispIdx = startIdx - (((optInTimePeriod / 2) | 0) + 1);
  i = trailingIdx;
  while (i < startIdx) {
    periodTotal += inReal[i];
    i = i + 1;
  }
  outIdx = 0;
  while (i <= endIdx) {
    periodTotal += inReal[i];
    i = i + 1;
    tempReal = periodTotal;
    periodTotal -= inReal[trailingIdx];
    trailingIdx = trailingIdx + 1;
    /* Both reads precede the store. Either cursor can EQUAL outIdx -- the
     * displaced one whenever startIdx equals the displacement, the trailing
     * one whenever startIdx sits at the lookback -- so a store hoisted above
     * them would read back what it had just overwritten when the caller
     * aliases outReal over inReal.
     */
    dispVal = inReal[dispIdx];
    dispIdx = dispIdx + 1;
    outReal[outIdx] = dispVal - tempReal / optInTimePeriod;
    outIdx = outIdx + 1;
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/**
 * Detrended Price Oscillator: the price a half-cycle back, less the moving average
 * that spans the cycle. Removing the average removes the trend, leaving the
 * shorter oscillation that the trend was hiding. It crosses zero as price crosses
 * its own average, so peaks and troughs mark the cycle rather than the direction
 * of the market. The distance between successive peaks estimates the cycle length,
 * and the amplitude is in price units, so it is comparable across time only for
 * one instrument.
 *
 * Formula and more info at
 * [ta-lib.org/functions/dpo](https://ta-lib.org/functions/dpo).
 *
 * **Notes**
 * - The value is emitted at the bar whose moving average produced it. Charting
 *   packages usually draw it `t` bars to the left instead, which is a plotting
 *   convention rather than a different series; a caller wanting that view shifts
 *   `outReal` itself, by the display shift the function reports, `-t`.
 * - A causal variant, `P[i] - SMA(P, optInTimePeriod)[i - t]`, displaces the
 *   average instead of the price. It is a genuinely different series, not a
 *   re-indexing of this one, and is not implemented here.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link dpoLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - Source price/value series, canonically the close.
 * @param optInTimePeriod - Number of bars spanned by the moving average being
 *   removed; the displacement is derived from it (default 20; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Detrended Price Oscillator value, in the units of the input.
 *   Must hold the values produced and never be empty; the input length always
 *   suffices.
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
 * import { dpo } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const { begIdx, count } = dpo(0, data.length - 1, data, 20, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`sma`](https://ta-lib.org/functions/sma)
 * @see [`mom`](https://ta-lib.org/functions/mom)
 * @see [`apo`](https://ta-lib.org/functions/apo)
 */
export function dpo(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('DPO', 'startIdx', startIdx);
  endIdx = requireInt32('DPO', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('DPO', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('DPO', startIdx, endIdx);
  const guardStart = clampedStart('DPO', startIdx, dpoLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('DPO', 'inReal', inReal, guardInLen);
  requireOutput('DPO', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw failure('DPO', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = dpoImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('DPO', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live DPO stream: one value per closed bar, bit-identical to {@link dpo} over
 * the same series. Open it with {@link dpoOpen} or {@link dpoOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/dpo](https://ta-lib.org/functions/dpo).
 */
export class DpoStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  periodTotal: number = 0;
  /** @internal */
  ringPos_dispIdx: number = 0;
  /** @internal */
  ringCap_dispIdx: number = 0;
  /** @internal */
  ring_dispIdx_inReal: Float64Array = new Float64Array(1);
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
  static of(core: Core): DpoStream {
    return new DpoStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link dpo} reports them over the same bars. Each accepted `update` and each
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
      throw failure('DPO advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: Source price/value series, canonically
   *   the close.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('DPO update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('DPO update', 'inReal');
    }
    dpoStepImpl(this, inReal);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Source price/value series, canonically
   *   the close.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number): number {
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('DPO peek', 'inReal');
    }
    const sp = this;
    let tempReal = 0;
    let dispVal = 0;
    let cur_outReal = 0;
    let periodTotal = sp.periodTotal;
    periodTotal += inReal;
    tempReal = periodTotal;
    periodTotal -= sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
    /* Both reads precede the store. Either cursor can EQUAL outIdx -- the
     * displaced one whenever startIdx equals the displacement, the trailing
     * one whenever startIdx sits at the lookback -- so a store hoisted above
     * them would read back what it had just overwritten when the caller
     * aliases outReal over inReal.
     */
    dispVal = sp.ring_dispIdx_inReal[sp.ringPos_dispIdx];
    cur_outReal = dispVal - tempReal / sp.optInTimePeriod;
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
  clone(): DpoStream {
    const c = new DpoStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.periodTotal = this.periodTotal;
    c.ringPos_dispIdx = this.ringPos_dispIdx;
    c.ringCap_dispIdx = this.ringCap_dispIdx;
    c.ring_dispIdx_inReal = this.ring_dispIdx_inReal.slice();
    c.ringPos_trailingIdx = this.ringPos_trailingIdx;
    c.ringCap_trailingIdx = this.ringCap_trailingIdx;
    c.ring_trailingIdx_inReal = this.ring_trailingIdx_inReal.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function dpoStepImpl(sp: DpoStream, inReal: number): void {
  let tempReal = 0;
  let dispVal = 0;
  let ringCapL_dispIdx = 0;
  let ringCapL_trailingIdx = 0;
  sp.periodTotal += inReal;
  tempReal = sp.periodTotal;
  sp.periodTotal -= sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
  /* Both reads precede the store. Either cursor can EQUAL outIdx -- the
   * displaced one whenever startIdx equals the displacement, the trailing
   * one whenever startIdx sits at the lookback -- so a store hoisted above
   * them would read back what it had just overwritten when the caller
   * aliases outReal over inReal.
   */
  dispVal = sp.ring_dispIdx_inReal[sp.ringPos_dispIdx];
  sp.cur_outReal = dispVal - tempReal / sp.optInTimePeriod;
  ringCapL_dispIdx = sp.ringCap_dispIdx;
  sp.ring_dispIdx_inReal[sp.ringPos_dispIdx] = inReal;
  sp.ringPos_dispIdx = sp.ringPos_dispIdx + 1;
  if (sp.ringPos_dispIdx >= ringCapL_dispIdx) {
    sp.ringPos_dispIdx = 0;
  }
  ringCapL_trailingIdx = sp.ringCap_trailingIdx;
  sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
  sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
  if (sp.ringPos_trailingIdx >= ringCapL_trailingIdx) {
    sp.ringPos_trailingIdx = 0;
  }
}

function dpoOpenImpl(
  sp: DpoStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let periodTotal = 0;
  let tempReal = 0;
  let dispVal = 0;
  let i = 0;
  let outIdx = 0;
  let trailingIdx = 0;
  let dispIdx = 0;
  let lookbackTotal = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* The running sum is accumulated, snapshot and divided in exactly the order
   * ta_codegen/input/sma/sma.c uses, so this fused loop is bit-for-bit equal
   * to a TA_SMA anchored at the same startIdx. That equality is what makes
   * the composite differential in test_dpo.c a memcmp instead of a tolerance
   * argument, and it is lost by dividing once into a reciprocal or by
   * reordering the add/snapshot/subtract.
   */
  lookbackTotal = dpoLookback(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  periodTotal = 0.0;
  trailingIdx = startIdx - (optInTimePeriod - 1);
  dispIdx = startIdx - (((optInTimePeriod / 2) | 0) + 1);
  i = trailingIdx;
  while (i < startIdx) {
    periodTotal += inReal[i];
    i = i + 1;
  }
  outIdx = 0;
  while (i <= endIdx) {
    periodTotal += inReal[i];
    i = i + 1;
    tempReal = periodTotal;
    periodTotal -= inReal[trailingIdx];
    trailingIdx = trailingIdx + 1;
    /* Both reads precede the store. Either cursor can EQUAL outIdx -- the
     * displaced one whenever startIdx equals the displacement, the trailing
     * one whenever startIdx sits at the lookback -- so a store hoisted above
     * them would read back what it had just overwritten when the caller
     * aliases outReal over inReal.
     */
    dispVal = inReal[dispIdx];
    dispIdx = dispIdx + 1;
    outReal[((outIdx * outStride) | 0)] = dispVal - tempReal / optInTimePeriod;
    outIdx = outIdx + 1;
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const cap_dispIdx = i - dispIdx;
  if (cap_dispIdx < 1 || cap_dispIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_dispIdx = cap_dispIdx > 0 ? cap_dispIdx : 1;
  const capRing_dispIdx_inReal = new Float64Array(allocN_dispIdx);
  capRing_dispIdx_inReal.set(inReal.subarray(historyLen - cap_dispIdx, historyLen), 0);
  const cap_trailingIdx = i - trailingIdx;
  if (cap_trailingIdx < 1 || cap_trailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_trailingIdx = cap_trailingIdx > 0 ? cap_trailingIdx : 1;
  const capRing_trailingIdx_inReal = new Float64Array(allocN_trailingIdx);
  capRing_trailingIdx_inReal.set(inReal.subarray(historyLen - cap_trailingIdx, historyLen), 0);
  sp.optInTimePeriod = optInTimePeriod;
  sp.periodTotal = periodTotal;
  sp.ringPos_dispIdx = 0;
  sp.ringCap_dispIdx = cap_dispIdx;
  sp.ring_dispIdx_inReal = capRing_dispIdx_inReal;
  sp.ringPos_trailingIdx = 0;
  sp.ringCap_trailingIdx = cap_trailingIdx;
  sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function dpoOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): DpoStream {
  const sp = DpoStream.of(core);
  const retCode = dpoOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('DPO openAndFill', inReal.length, startIdx, dpoLookback(optInTimePeriod, core));
  }
  throw streamFailure('DPO openAndFill', retCode);
}

/** @internal */
export function dpoOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): DpoStream {
  const sp = DpoStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = dpoOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('DPO open', inReal.length, startIdx, dpoLookback(optInTimePeriod, core));
  }
  throw streamFailure('DPO open', retCode);
}

/**
 * Opens a live DPO stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link dpo} there.
 *
 * @param inReal - Warm-up history. Source price/value series, canonically the
 *   close.
 * @param optInTimePeriod - Number of bars spanned by the moving average being
 *   removed; the displacement is derived from it (default 20; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `dpoLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { dpoOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = dpoOpen(data, 20);
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function dpoOpen(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): DpoStream {
  optInTimePeriod = requireInt32('DPO open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('DPO open', 'inReal', inReal);
  requireHistory('DPO open', inReal.length);
  return dpoOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/**
 * {@link dpoOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link dpo}. The range written is the stream's
 * `outRange()`.
 *
 * @param inReal - Warm-up history. Source price/value series, canonically the
 *   close.
 * @param optInTimePeriod - Number of bars spanned by the moving average being
 *   removed; the displacement is derived from it (default 20; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Detrended Price Oscillator value, in the units of the input.
 *   Must hold `historyLen - lookback` values and share no memory with an input or
 *   another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `dpoLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   an integer parameter that is not an int32, a parameter outside its range or an
 *   output too short or starting where another output or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { dpoOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const stream = dpoOpenAndFill(data, 20, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function dpoOpenAndFill(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): DpoStream {
  optInTimePeriod = requireInt32('DPO openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('DPO openAndFill', 'inReal', inReal);
  requireHistory('DPO openAndFill', inReal.length);
  const guardOutLen = openFillCount('DPO openAndFill', inReal.length, dpoLookback(optInTimePeriod, core));
  requireOutput('DPO openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if ((outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset)) {
    throw streamFailure('DPO openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inRealF64))) {
    throw streamFailure('DPO openAndFill', RetCode.BAD_PARAM);
  }
  return dpoOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}
