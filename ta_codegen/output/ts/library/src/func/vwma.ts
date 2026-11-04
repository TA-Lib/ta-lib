/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  072026 MF,CC  First version (#131).
 *  080926 MF,CC  Allow period of 1. Just copy input into output.
 *  092526 MF,CC  #446 exact zero sums on a dead volume window.
 *  092626 MF,CC  #446 branch-free zero count.
 *  102326 MF,CC  #554 rare reset.
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link vwma} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Number of bars in the weighting window (default 30;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function vwmaLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  return optInTimePeriod - 1;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link vwma}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Number of bars in the weighting window (default 30;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function vwmaDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (vwmaLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function vwmaImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  inVolume: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let sumPV = 0;
  let sumV = 0;
  let tempPV = 0;
  let tempV = 0;
  let tempReal = 0;
  let trailingVolume = 0;
  let i = 0;
  let outIdx = 0;
  let trailingIdx = 0;
  let lookbackTotal = 0;
  let zeroCount = 0;
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
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = (optInTimePeriod - 1);
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
  /* No smoothing at period of 1: the output is a copy of the input
   * (same convention as TA_MA for every MAType). Explicit because
   * (P*V)/V round-trips only ~97% of the time in IEEE double, and
   * because a lone zero volume must give the price, not NaN.
   */
  if (optInTimePeriod === 1) {
    outBegIdx.value = startIdx;
    outIdx = 0;
    i = startIdx;
    while (i <= endIdx) {
      outReal[outIdx++] = inReal[i++];
    }
    outNBElement.value = outIdx;
    return RetCode.SUCCESS;
  }
  /* Add-up the initial period, except for the last value.
   *
   * The price*volume product is kept in its own statement so no compiler may
   * contract it into an FMA: that would make this function disagree with the
   * Rust/Java backends under the cross-language bitwise gate, and with the
   * two-TA_SMA composite reference.
   */
  sumPV = 0.0;
  sumV = 0.0;
  trailingIdx = startIdx - lookbackTotal;
  /* Zero-volume bars in the window. Once they fill it both sums are exactly
   * zero, where add-then-subtract would leave the rounding residue of the bars
   * that departed, of either sign. The test is fabs(v) <= 0.0 rather than
   * == 0.0: the same result, NaN included, from one flag instead of two.
   */
  zeroCount = 0;
  i = trailingIdx;
  if (optInTimePeriod > 1) {
    while (i < startIdx) {
      tempReal = inReal[i] * inVolume[i];
      sumPV += tempReal;
      sumV += inVolume[i];
      zeroCount += (Math.abs(inVolume[i]) <= 0.0 ? 1 : 0);
      i = i + 1;
    }
  }
  /* Proceed with the calculation for the requested range.
   * Note that this algorithm allows the inReal and
   * outReal to be the same buffer.
   */
  outIdx = 0;
  while (i <= endIdx) {
    tempReal = inReal[i] * inVolume[i];
    sumPV += tempReal;
    sumV += inVolume[i];
    zeroCount += (Math.abs(inVolume[i]) <= 0.0 ? 1 : 0);
    i = i + 1;
    /* Read the trailing values before writing the output, since the caller
     * may pass the same buffer for an input and the output.
     */
    trailingVolume = inVolume[trailingIdx];
    tempReal = inReal[trailingIdx] * trailingVolume;
    /* Each branch writes its own output: a branch that only zeroes the sums
     * is if-converted into a mask on their dependency chain, hint or not.
     */
    if (zeroCount >= optInTimePeriod) {
      /* Zero, then subtract the departing bar: a non-finite price times its
       * zero volume is NaN, not zero.
       */
      tempPV = 0.0;
      tempV = 0.0;
      sumPV = 0.0 - tempReal;
      sumV = 0.0 - trailingVolume;
      outReal[outIdx] = tempPV / optInTimePeriod / (tempV / optInTimePeriod);
    } else {
      /* Snapshot both sums before removing the trailing bar, mirroring the
       * add-new / snapshot / subtract-old order of TA_SMA. Up to the first
       * dead window, that order is what makes this bit-identical to
       * SMA(inReal*inVolume)/SMA(inVolume).
       */
      tempPV = sumPV;
      tempV = sumV;
      sumPV -= tempReal;
      sumV -= trailingVolume;
      outReal[outIdx] = tempPV / optInTimePeriod / (tempV / optInTimePeriod);
    }
    zeroCount -= (Math.abs(trailingVolume) <= 0.0 ? 1 : 0);
    trailingIdx = trailingIdx + 1;
    outIdx = outIdx + 1;
  }
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/**
 * Volume Weighted Moving Average: the mean price over a trailing window of
 * `optInTimePeriod` bars, each bar weighted by its own volume. Heavily traded bars
 * pull the average toward their price; quiet bars barely move it. Read like any
 * moving average — price above is strength, below is weakness. Against a plain
 * [`SMA`](https://ta-lib.org/functions/sma) of the same window it leads on
 * high-volume moves and lags on low-volume drift, so the gap between the two lines
 * measures how volume-confirmed a move is. It has no attributable inventor —
 * charting-package folklore — and every published definition agrees, so there is
 * no competing variant.
 *
 * Formula and more info at
 * [ta-lib.org/functions/vwma](https://ta-lib.org/functions/vwma).
 *
 * **Notes**
 * - A period of 1 performs no smoothing: the output is a copy of the input,
 *   whatever the volume.
 * - Volume is expected to be non-negative. Individual zero-volume bars are fine: a
 *   bar that did not trade simply carries no weight, and the average stays well
 *   defined as long as some bar in the window has volume. At a period of 2 or
 *   more, a window in which *every* volume is zero has no weights at all; the
 *   weighted mean is then undefined and that element is NaN, as it is in every
 *   other implementation. Series carrying no volume on any bar, such as cash-index
 *   feeds, are outside what a volume-weighted average can describe — use SMA or
 *   WMA there.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link vwmaLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - Source price series, close by convention.
 * @param inVolume - Volume of each bar.
 * @param optInTimePeriod - Number of bars in the weighting window (default 30;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Volume weighted moving average of the input. Must hold the
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
 * import { vwma } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const out = new Float64Array(data.length);
 * const { begIdx, count } = vwma(0, data.length - 1, data, volume, 30, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`sma`](https://ta-lib.org/functions/sma)
 * @see [`wma`](https://ta-lib.org/functions/wma)
 * @see [`ma`](https://ta-lib.org/functions/ma)
 * @see [`obv`](https://ta-lib.org/functions/obv)
 */
export function vwma(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('VWMA', 'startIdx', startIdx);
  endIdx = requireInt32('VWMA', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('VWMA', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('VWMA', startIdx, endIdx);
  const guardStart = clampedStart('VWMA', startIdx, vwmaLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('VWMA', 'inReal', inReal, guardInLen);
  requireLength('VWMA', 'inVolume', inVolume, guardInLen);
  requireOutput('VWMA', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  const inVolumeF64 = float64(inVolume);
  if (overlapsPartly(outReal, inRealF64) || overlapsPartly(outReal, inVolumeF64)) {
    throw failure('VWMA', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = vwmaImpl(core, startIdx, endIdx, inRealF64, inVolumeF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('VWMA', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live VWMA stream: one value per closed bar, bit-identical to {@link vwma} over
 * the same series. Open it with {@link vwmaOpen} or {@link vwmaOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/vwma](https://ta-lib.org/functions/vwma).
 */
export class VwmaStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  sumPV: number = 0;
  /** @internal */
  sumV: number = 0;
  /** @internal */
  zeroCount: number = 0;
  /** @internal */
  ringPos_trailingIdx: number = 0;
  /** @internal */
  ringCap_trailingIdx: number = 0;
  /** @internal */
  ring_trailingIdx_inReal: Float64Array = new Float64Array(1);
  /** @internal */
  ring_trailingIdx_inVolume: Float64Array = new Float64Array(1);
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
  static of(core: Core): VwmaStream {
    return new VwmaStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link vwma} reports them over the same bars. Each accepted `update` and each
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
      throw failure('VWMA advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: Source price series, close by
   *   convention.
   * @param inVolume - The bar's `inVolume` value: Volume of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number, inVolume: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('VWMA update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal) || !Number.isFinite(inVolume)) {
      throw nonFiniteBar('VWMA update', !Number.isFinite(inReal) ? 'inReal' : 'inVolume');
    }
    vwmaStepImpl(this, inReal, inVolume);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Source price series, close by
   *   convention.
   * @param inVolume - The bar's `inVolume` value: Volume of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number, inVolume: number): number {
    if (!Number.isFinite(inReal) || !Number.isFinite(inVolume)) {
      throw nonFiniteBar('VWMA peek', !Number.isFinite(inReal) ? 'inReal' : 'inVolume');
    }
    const sp = this;
    let tempPV = 0;
    let tempV = 0;
    let tempReal = 0;
    let trailingVolume = 0;
    let cur_outReal = 0;
    let sumPV = sp.sumPV;
    let sumV = sp.sumV;
    let zeroCount = sp.zeroCount;
    if (sp.optInTimePeriod === 1) {
      cur_outReal = inReal;
      return cur_outReal;
    }
    tempReal = inReal * inVolume;
    sumPV += tempReal;
    sumV += inVolume;
    zeroCount += (Math.abs(inVolume) <= 0.0 ? 1 : 0);
    /* Read the trailing values before writing the output, since the caller
     * may pass the same buffer for an input and the output.
     */
    trailingVolume = sp.ring_trailingIdx_inVolume[sp.ringPos_trailingIdx];
    tempReal = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] * trailingVolume;
    /* Each branch writes its own output: a branch that only zeroes the sums
     * is if-converted into a mask on their dependency chain, hint or not.
     */
    if (zeroCount >= sp.optInTimePeriod) {
      /* Zero, then subtract the departing bar: a non-finite price times its
       * zero volume is NaN, not zero.
       */
      tempPV = 0.0;
      tempV = 0.0;
      sumPV = 0.0 - tempReal;
      sumV = 0.0 - trailingVolume;
      cur_outReal = tempPV / sp.optInTimePeriod / (tempV / sp.optInTimePeriod);
    } else {
      /* Snapshot both sums before removing the trailing bar, mirroring the
       * add-new / snapshot / subtract-old order of TA_SMA. Up to the first
       * dead window, that order is what makes this bit-identical to
       * SMA(inReal*inVolume)/SMA(inVolume).
       */
      tempPV = sumPV;
      tempV = sumV;
      sumPV -= tempReal;
      sumV -= trailingVolume;
      cur_outReal = tempPV / sp.optInTimePeriod / (tempV / sp.optInTimePeriod);
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
  clone(): VwmaStream {
    const c = new VwmaStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.sumPV = this.sumPV;
    c.sumV = this.sumV;
    c.zeroCount = this.zeroCount;
    c.ringPos_trailingIdx = this.ringPos_trailingIdx;
    c.ringCap_trailingIdx = this.ringCap_trailingIdx;
    c.ring_trailingIdx_inReal = this.ring_trailingIdx_inReal.slice();
    c.ring_trailingIdx_inVolume = this.ring_trailingIdx_inVolume.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function vwmaStepImpl(sp: VwmaStream, inReal: number, inVolume: number): void {
  let tempPV = 0;
  let tempV = 0;
  let tempReal = 0;
  let trailingVolume = 0;
  let ringCapL_trailingIdx = 0;
  if (sp.optInTimePeriod === 1) {
    sp.cur_outReal = inReal;
    return;
  }
  tempReal = inReal * inVolume;
  sp.sumPV += tempReal;
  sp.sumV += inVolume;
  sp.zeroCount += (Math.abs(inVolume) <= 0.0 ? 1 : 0);
  /* Read the trailing values before writing the output, since the caller
   * may pass the same buffer for an input and the output.
   */
  trailingVolume = sp.ring_trailingIdx_inVolume[sp.ringPos_trailingIdx];
  tempReal = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] * trailingVolume;
  /* Each branch writes its own output: a branch that only zeroes the sums
   * is if-converted into a mask on their dependency chain, hint or not.
   */
  if (sp.zeroCount >= sp.optInTimePeriod) {
    /* Zero, then subtract the departing bar: a non-finite price times its
     * zero volume is NaN, not zero.
     */
    tempPV = 0.0;
    tempV = 0.0;
    sp.sumPV = 0.0 - tempReal;
    sp.sumV = 0.0 - trailingVolume;
    sp.cur_outReal = tempPV / sp.optInTimePeriod / (tempV / sp.optInTimePeriod);
  } else {
    /* Snapshot both sums before removing the trailing bar, mirroring the
     * add-new / snapshot / subtract-old order of TA_SMA. Up to the first
     * dead window, that order is what makes this bit-identical to
     * SMA(inReal*inVolume)/SMA(inVolume).
     */
    tempPV = sp.sumPV;
    tempV = sp.sumV;
    sp.sumPV -= tempReal;
    sp.sumV -= trailingVolume;
    sp.cur_outReal = tempPV / sp.optInTimePeriod / (tempV / sp.optInTimePeriod);
  }
  sp.zeroCount -= (Math.abs(trailingVolume) <= 0.0 ? 1 : 0);
  ringCapL_trailingIdx = sp.ringCap_trailingIdx;
  sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
  sp.ring_trailingIdx_inVolume[sp.ringPos_trailingIdx] = inVolume;
  sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
  if (sp.ringPos_trailingIdx >= ringCapL_trailingIdx) {
    sp.ringPos_trailingIdx = 0;
  }
}

function vwmaOpenImpl(
  sp: VwmaStream,
  inReal: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let sumPV = 0;
  let sumV = 0;
  let tempPV = 0;
  let tempV = 0;
  let tempReal = 0;
  let trailingVolume = 0;
  let i = 0;
  let outIdx = 0;
  let trailingIdx = 0;
  let lookbackTotal = 0;
  let zeroCount = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inVolume.length !== inReal.length) {
    return RetCode.BAD_PARAM;
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
  if (optInTimePeriod === 1) {
    let fillLb = vwmaLookback(optInTimePeriod, core);
    if (startIdx > fillLb) {
      fillLb = startIdx;
    }
    if (historyLen < fillLb + 1) {
      return RetCode.INSUFFICIENT_HISTORY;
    }
    sp.optInTimePeriod = optInTimePeriod;
    sp.sumPV = 0;
    sp.sumV = 0;
    sp.zeroCount = 0;
    sp.ringPos_trailingIdx = 0;
    sp.ringCap_trailingIdx = 0;
    sp.ring_trailingIdx_inReal = new Float64Array(1);
    sp.ring_trailingIdx_inVolume = new Float64Array(1);
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
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = (optInTimePeriod - 1);
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
  /* Add-up the initial period, except for the last value.
   *
   * The price*volume product is kept in its own statement so no compiler may
   * contract it into an FMA: that would make this function disagree with the
   * Rust/Java backends under the cross-language bitwise gate, and with the
   * two-TA_SMA composite reference.
   */
  sumPV = 0.0;
  sumV = 0.0;
  trailingIdx = startIdx - lookbackTotal;
  /* Zero-volume bars in the window. Once they fill it both sums are exactly
   * zero, where add-then-subtract would leave the rounding residue of the bars
   * that departed, of either sign. The test is fabs(v) <= 0.0 rather than
   * == 0.0: the same result, NaN included, from one flag instead of two.
   */
  zeroCount = 0;
  i = trailingIdx;
  if (optInTimePeriod > 1) {
    while (i < startIdx) {
      tempReal = inReal[i] * inVolume[i];
      sumPV += tempReal;
      sumV += inVolume[i];
      zeroCount += (Math.abs(inVolume[i]) <= 0.0 ? 1 : 0);
      i = i + 1;
    }
  }
  /* Proceed with the calculation for the requested range.
   * Note that this algorithm allows the inReal and
   * outReal to be the same buffer.
   */
  outIdx = 0;
  while (i <= endIdx) {
    tempReal = inReal[i] * inVolume[i];
    sumPV += tempReal;
    sumV += inVolume[i];
    zeroCount += (Math.abs(inVolume[i]) <= 0.0 ? 1 : 0);
    i = i + 1;
    /* Read the trailing values before writing the output, since the caller
     * may pass the same buffer for an input and the output.
     */
    trailingVolume = inVolume[trailingIdx];
    tempReal = inReal[trailingIdx] * trailingVolume;
    /* Each branch writes its own output: a branch that only zeroes the sums
     * is if-converted into a mask on their dependency chain, hint or not.
     */
    if (zeroCount >= optInTimePeriod) {
      /* Zero, then subtract the departing bar: a non-finite price times its
       * zero volume is NaN, not zero.
       */
      tempPV = 0.0;
      tempV = 0.0;
      sumPV = 0.0 - tempReal;
      sumV = 0.0 - trailingVolume;
      outReal[((outIdx * outStride) | 0)] = tempPV / optInTimePeriod / (tempV / optInTimePeriod);
    } else {
      /* Snapshot both sums before removing the trailing bar, mirroring the
       * add-new / snapshot / subtract-old order of TA_SMA. Up to the first
       * dead window, that order is what makes this bit-identical to
       * SMA(inReal*inVolume)/SMA(inVolume).
       */
      tempPV = sumPV;
      tempV = sumV;
      sumPV -= tempReal;
      sumV -= trailingVolume;
      outReal[((outIdx * outStride) | 0)] = tempPV / optInTimePeriod / (tempV / optInTimePeriod);
    }
    zeroCount -= (Math.abs(trailingVolume) <= 0.0 ? 1 : 0);
    trailingIdx = trailingIdx + 1;
    outIdx = outIdx + 1;
  }
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const cap_trailingIdx = i - trailingIdx;
  if (cap_trailingIdx < 1 || cap_trailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_trailingIdx = cap_trailingIdx > 0 ? cap_trailingIdx : 1;
  const capRing_trailingIdx_inReal = new Float64Array(allocN_trailingIdx);
  capRing_trailingIdx_inReal.set(inReal.subarray(historyLen - cap_trailingIdx, historyLen), 0);
  const capRing_trailingIdx_inVolume = new Float64Array(allocN_trailingIdx);
  capRing_trailingIdx_inVolume.set(inVolume.subarray(historyLen - cap_trailingIdx, historyLen), 0);
  sp.optInTimePeriod = optInTimePeriod;
  sp.sumPV = sumPV;
  sp.sumV = sumV;
  sp.zeroCount = zeroCount;
  sp.ringPos_trailingIdx = 0;
  sp.ringCap_trailingIdx = cap_trailingIdx;
  sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
  sp.ring_trailingIdx_inVolume = capRing_trailingIdx_inVolume;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function vwmaOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): VwmaStream {
  const sp = VwmaStream.of(core);
  const retCode = vwmaOpenImpl(sp, inReal, inVolume, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('VWMA openAndFill', inReal.length, startIdx, vwmaLookback(optInTimePeriod, core));
  }
  throw streamFailure('VWMA openAndFill', retCode);
}

/** @internal */
export function vwmaOpenInternal(
  core: Core,
  inReal: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): VwmaStream {
  const sp = VwmaStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = vwmaOpenImpl(sp, inReal, inVolume, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('VWMA open', inReal.length, startIdx, vwmaLookback(optInTimePeriod, core));
  }
  throw streamFailure('VWMA open', retCode);
}

/**
 * Opens a live VWMA stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link vwma} there.
 *
 * @param inReal - Warm-up history. Source price series, close by convention. Every
 *   history has the same length.
 * @param inVolume - Warm-up history. Volume of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Number of bars in the weighting window (default 30;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `vwmaLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { vwmaOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const stream = vwmaOpen(data, volume, 30);
 * const peeked = stream.peek(100.9, 12345); // a forming bar: commits nothing
 * const updated = stream.update(100.9, 12345); // the bar closes
 * ```
 */
export function vwmaOpen(
  inReal: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): VwmaStream {
  optInTimePeriod = requireInt32('VWMA open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('VWMA open', 'inReal', inReal);
  requireHistory('VWMA open', inReal.length);
  requireArgument('VWMA open', 'inVolume', inVolume);
  requireHistoryLength('VWMA open', 'inVolume', inVolume.length, inReal.length);
  return vwmaOpenInternal(core, float64(inReal), float64(inVolume), 0, optInTimePeriod);
}

/**
 * {@link vwmaOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link vwma}. The range written is the stream's
 * `outRange()`.
 *
 * @param inReal - Warm-up history. Source price series, close by convention. Every
 *   history has the same length.
 * @param inVolume - Warm-up history. Volume of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Number of bars in the weighting window (default 30;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Volume weighted moving average of the input. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `vwmaLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range, histories of different lengths or an output too
 *   short or starting where another output or a `Float64Array` input starts, or, on
 *   a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { vwmaOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const out = new Float64Array(data.length);
 * const stream = vwmaOpenAndFill(data, volume, 30, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function vwmaOpenAndFill(
  inReal: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): VwmaStream {
  optInTimePeriod = requireInt32('VWMA openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('VWMA openAndFill', 'inReal', inReal);
  requireHistory('VWMA openAndFill', inReal.length);
  requireArgument('VWMA openAndFill', 'inVolume', inVolume);
  const guardOutLen = openFillCount('VWMA openAndFill', inReal.length, vwmaLookback(optInTimePeriod, core));
  requireHistoryLength('VWMA openAndFill', 'inVolume', inVolume.length, inReal.length);
  requireOutput('VWMA openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  const inVolumeF64 = float64(inVolume);
  if ((outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) || (outReal.buffer === inVolumeF64.buffer && outReal.byteOffset === inVolumeF64.byteOffset)) {
    throw streamFailure('VWMA openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inRealF64) || overlaps(outReal, inVolumeF64))) {
    throw streamFailure('VWMA openAndFill', RetCode.BAD_PARAM);
  }
  return vwmaOpenAndFillInternal(core, inRealF64, inVolumeF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}
