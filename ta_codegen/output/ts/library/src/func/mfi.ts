/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  BT       BobTrader (TADoc.org forum user).
 *  MW       github @mw66
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120802 MF    Template creation.
 *  052603 MF    Adapt code to compile with .NET Managed C++
 *  062704 MF    Prevent divide by zero.
 *  121705 MF    Java port related changes.
 *  060907 MF,BT Fix #1727704. MFI logic bug when no price movement
 *  070726 MW,CC Fix #4. MFI has no unstable period; drop the unstable-period
 *               term (and the now-dead unstable-skip loop) so
 *               TA_SetUnstablePeriod is a no-op for it.
 *  071026 MF,CC Fix #107. Classify money-flow direction with a magnitude-scaled
 *               dead-zone (TA_IS_ZERO_SCALED), not an exact sign test, so an
 *               epsilon-flat typical price is "no movement", not a spurious move.
 *  082326 MF,CC Fix #244. Detect an empty window by counting bars, not by
 *               testing the money-flow sum against a literal 1.0; classify
 *               branchlessly; clamp the emitted ratio into [0,100].
 *  092526 MF,CC #442. Allocate the money-flow ring only when there is output.
 *  102326 MF,CC #554 rare reset.
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function mfiLookbackPositional(optInTimePeriod: number, _core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  return optInTimePeriod;
}

/** @internal */
export function mfiDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (mfiLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function mfiImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  inVolume: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let posSumMF = 0;
  let negSumMF = 0;
  let prevValue = 0;
  let tempValue1 = 0;
  let tempValue2 = 0;
  let tempValue3 = 0;
  let moneyFlow = 0;
  let posFlow = 0;
  let negFlow = 0;
  let posClamped = 0;
  let lookbackTotal = 0;
  let outIdx = 0;
  let i = 0;
  let today = 0;
  let nullRun = 0;
  let mflow_positive!: Float64Array;
  let mflow_negative!: Float64Array;
  let mflow_Idx = 0;
  let maxIdx_mflow = (50) - 1;
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
  /* Id, Type, Static Size */
  outBegIdx.value = 0;
  outNBElement.value = 0;
  /* Adjust startIdx to account for the lookback period. */
  lookbackTotal = optInTimePeriod;
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  mflow_positive = new Float64Array(optInTimePeriod);
  mflow_negative = new Float64Array(optInTimePeriod);
  maxIdx_mflow = optInTimePeriod - 1;
  mflow_Idx = 0;
  outIdx = 0;
  /* Index into the output. */
  /* Accumulate the positive and negative money flow
   * among the initial period.
   */
  today = startIdx - lookbackTotal;
  prevValue = (inHigh[today] + inLow[today] + inClose[today]) / 3.0;
  posSumMF = 0.0;
  negSumMF = 0.0;
  /* Consecutive bars that put nothing into the window, counted so that an
   * empty window can be recognized exactly (issue #244).  The running sums
   * cannot answer that question themselves: they are maintained by
   * add-then-subtract, so when the window empties they hold rounding
   * residue of arbitrary sign, not zero.
   */
  nullRun = 0;
  today += 1;
  for (i = optInTimePeriod; i > 0; i -= 1) {
    tempValue1 = (inHigh[today] + inLow[today] + inClose[today]) / 3.0;
    tempValue2 = tempValue1 - prevValue;
    /* Dead-zone scaled to the two typical prices being compared (issue #107).
     * Captured before prevValue/tempValue1 are repurposed below.
     */
    tempValue3 = Math.abs(tempValue1) + Math.abs(prevValue);
    prevValue = tempValue1;
    tempValue1 *= inVolume[today++];
    /* This bar's money flow, and its split into the positive and negative
     * sums.  Selects rather than a three-arm branch: the direction of a
     * price move is a coin flip, so that branch mispredicted on roughly
     * every other bar and dominated the cost of the function.  Adding the
     * unused side's 0.0 to a sum is an exact no-op, so this reproduces the
     * branching form bit for bit.
     */
    moneyFlow = (Math.abs(tempValue2) <= 0.00000000000001 * (tempValue3)) ? 0.0 : tempValue1;
    posFlow = (tempValue2 < 0.0) ? 0.0 : moneyFlow;
    negFlow = (tempValue2 < 0.0) ? moneyFlow : 0.0;
    mflow_positive[mflow_Idx] = posFlow;
    mflow_negative[mflow_Idx] = negFlow;
    posSumMF += posFlow;
    negSumMF += negFlow;
    /* A bar contributes nothing when the typical price did not move, or
     * when it moved but carried no volume.  Once a whole period of those
     * has gone by, every slot of the ring is 0.0, so the sums are known to
     * be exactly zero and the residue can be dropped.
     */
    nullRun = (moneyFlow === 0.0) ? nullRun + 1 : 0;
    if (nullRun >= optInTimePeriod) {
      nullRun = optInTimePeriod;
      posSumMF = 0.0;
      negSumMF = 0.0;
    }
    mflow_Idx++;
    if (mflow_Idx > maxIdx_mflow) {
      mflow_Idx = 0;
    }
  }
  /* The following two equations are equivalent:
   *    MFI = 100 - (100 / 1 + (posSumMF/negSumMF))
   *    MFI = 100 * (posSumMF/(posSumMF+negSumMF))
   * The second equation is used here for speed optimization.
   *
   * Both sums are non-negative, so the total is zero only for a window that
   * received no money flow at all -- 0/0, reported as 0.0.  The test is on
   * the total itself, not on a fixed threshold: money flow is a price times
   * a volume, so any constant compared against it is a constant in some
   * arbitrary unit, and would zero a healthy index for any instrument
   * quoted small enough to fall under it (issue #244).
   *
   * Clamping the numerator into [0,total] keeps the result inside the
   * documented 0-100 range: the sums drift by a few ulp as the window
   * slides, and a sum whose true value is near zero can drift negative.
   */
  /* The first full window is complete: emit its output for startIdx here,
   * then slide the window over the remaining bars below.
   */
  tempValue1 = posSumMF + negSumMF;
  posClamped = (posSumMF < 0.0) ? 0.0 : ((posSumMF > tempValue1) ? tempValue1 : posSumMF);
  if (tempValue1 <= 0.0) {
    outReal[outIdx++] = 0.0;
  } else {
    outReal[outIdx++] = 100.0 * (posClamped / tempValue1);
  }
  /* Now continue processing the remaining bars. */
  while (today <= endIdx) {
    posSumMF -= mflow_positive[mflow_Idx];
    negSumMF -= mflow_negative[mflow_Idx];
    tempValue1 = (inHigh[today] + inLow[today] + inClose[today]) / 3.0;
    tempValue2 = tempValue1 - prevValue;
    /* Dead-zone scaled to the two typical prices being compared (issue #107).
     * Captured before prevValue/tempValue1 are repurposed below.
     */
    tempValue3 = Math.abs(tempValue1) + Math.abs(prevValue);
    prevValue = tempValue1;
    tempValue1 *= inVolume[today++];
    moneyFlow = (Math.abs(tempValue2) <= 0.00000000000001 * (tempValue3)) ? 0.0 : tempValue1;
    posFlow = (tempValue2 < 0.0) ? 0.0 : moneyFlow;
    negFlow = (tempValue2 < 0.0) ? moneyFlow : 0.0;
    mflow_positive[mflow_Idx] = posFlow;
    mflow_negative[mflow_Idx] = negFlow;
    posSumMF += posFlow;
    negSumMF += negFlow;
    nullRun = (moneyFlow === 0.0) ? nullRun + 1 : 0;
    if (nullRun >= optInTimePeriod) {
      nullRun = optInTimePeriod;
      posSumMF = 0.0;
      negSumMF = 0.0;
    }
    tempValue1 = posSumMF + negSumMF;
    posClamped = (posSumMF < 0.0) ? 0.0 : ((posSumMF > tempValue1) ? tempValue1 : posSumMF);
    if (tempValue1 <= 0.0) {
      outReal[outIdx++] = 0.0;
    } else {
      outReal[outIdx++] = 100.0 * (posClamped / tempValue1);
    }
    mflow_Idx++;
    if (mflow_Idx > maxIdx_mflow) {
      mflow_Idx = 0;
    }
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000']] as const;

/** @internal */
export function mfiPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('MFI', 'startIdx', startIdx);
  endIdx = requireInt32('MFI', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('MFI', 'timePeriod', optInTimePeriod);
  requireIndexRange('MFI', startIdx, endIdx);
  const guardLookback = mfiLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('MFI', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('MFI', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('MFI', 'prices.high', inHigh, guardInLen);
  requireLength('MFI', 'prices.low', inLow, guardInLen);
  requireLength('MFI', 'prices.close', inClose, guardInLen);
  requireLength('MFI', 'prices.volume', inVolume, guardInLen);
  requireOutput('MFI', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  const inVolumeF64 = float64(inVolume);
  if (overlapsPartly(outReal, inHighF64)) {
    throw sharedMemory('MFI', 'outReal', 'prices.high');
  }
  if (overlapsPartly(outReal, inLowF64)) {
    throw sharedMemory('MFI', 'outReal', 'prices.low');
  }
  if (overlapsPartly(outReal, inCloseF64)) {
    throw sharedMemory('MFI', 'outReal', 'prices.close');
  }
  if (overlapsPartly(outReal, inVolumeF64)) {
    throw sharedMemory('MFI', 'outReal', 'prices.volume');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = mfiImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, inVolumeF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('MFI', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link mfi}, {@link mfiLookback}, {@link mfiDisplayShift},
 * {@link mfiOpen} and {@link mfiOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface MfiOptions {
  /**
   * Lookback window for summing money flow (default 14; range 2 to 100000;
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
  outReal?: Float64Array | undefined;
}

/** What {@link mfi} returns, and {@link mfiOpenAndFill} with its stream. */
export interface MfiResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /** Money Flow Index. `values[k]` is the value at input bar `begIdx + k`. */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link mfi} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MfiOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function mfiLookback(options?: MfiOptions): number {
  const o = optionsOf('MFI lookback', options);
  return mfiLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('MFI lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link mfi}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MfiOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function mfiDisplayShift(outputIdx: number, options?: MfiOptions): number {
  const o = optionsOf('MFI displayShift', options);
  return mfiDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('MFI displayShift', o.core));
}

/**
 * Money Flow Index: a volume-weighted momentum oscillator (0-100) comparing
 * positive vs negative money flow over a period. A volume-based analog of RSI. >80
 * overbought, \<20 oversold.
 *
 * Formula and more info at
 * [ta-lib.org/functions/mfi](https://ta-lib.org/functions/mfi).
 *
 * **Notes**
 * - When the typical price is unchanged from the prior bar, that bar's money flow
 *   is counted as neither positive nor negative.
 * - A window in which no bar contributed any money flow — every typical price
 *   unchanged, or no volume traded — leaves the index undefined (0/0); 0 is
 *   returned. The result does not otherwise depend on the size of the money flow:
 *   scaling every volume, or quoting the instrument in a different unit, leaves
 *   the index unchanged.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link mfiLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `high`: High
 *   price of each bar. `low`: Low price of each bar. `close`: Close price of each
 *   bar. `volume`: Volume of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link MfiOptions}.
 * @returns `begIdx` and the values; see {@link MfiResult}. Each output is a view
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
 * import { mfi } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const { begIdx, values } = mfi({ high, low, close, volume }, {
 *   timePeriod: 14,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`rsi`](https://ta-lib.org/functions/rsi)
 * @see [`ad`](https://ta-lib.org/functions/ad)
 * @see [`adosc`](https://ta-lib.org/functions/adosc)
 */
export function mfi(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume: ArrayLike<number> }, options?: MfiOptions): MfiResult {
  requireArgument('MFI', 'prices', prices);
  const o = optionsOf('MFI', options);
  if (o.endIdx === undefined) {
    requireSeries('MFI', 'prices.high', prices.high);
    requireSeries('MFI', 'prices.low', prices.low);
    requireSeries('MFI', 'prices.close', prices.close);
    requireSeries('MFI', 'prices.volume', prices.volume);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MFI', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, mfiLookbackPositional(optInTimePeriod, core), prices.high, prices.low, prices.close, prices.volume);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = mfiPositional(startIdx, endIdx, prices.high, prices.low, prices.close, prices.volume, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live MFI stream: one value per closed bar, bit-identical to {@link mfi} over
 * the same series. Open it with {@link mfiOpen} or {@link mfiOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/mfi](https://ta-lib.org/functions/mfi).
 */
export class MfiStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  posSumMF: number = 0;
  /** @internal */
  negSumMF: number = 0;
  /** @internal */
  prevValue: number = 0;
  /** @internal */
  nullRun: number = 0;
  /** @internal */
  mflow_Idx: number = 0;
  /** @internal */
  maxIdx_mflow: number = 0;
  /** @internal */
  cbSize_mflow: number = 0;
  /** @internal */
  cb_mflow_positive: Float64Array = new Float64Array(1);
  /** @internal */
  cb_mflow_negative: Float64Array = new Float64Array(1);
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
  static of(core: Core): MfiStream {
    return new MfiStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link mfi} reports them over the same bars. Each accepted `update` and each
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
      throw failure('MFI advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param inVolume - The bar's `inVolume` value: Volume of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number, inClose: number, inVolume: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('MFI update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose) || !Number.isFinite(inVolume)) {
      throw nonFiniteBar('MFI update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : !Number.isFinite(inClose) ? 'inClose' : 'inVolume');
    }
    mfiStepImpl(this, inHigh, inLow, inClose, inVolume);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param inVolume - The bar's `inVolume` value: Volume of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number, inClose: number, inVolume: number): number {
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose) || !Number.isFinite(inVolume)) {
      throw nonFiniteBar('MFI peek', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : !Number.isFinite(inClose) ? 'inClose' : 'inVolume');
    }
    const sp = this;
    let tempValue1 = 0;
    let tempValue2 = 0;
    let tempValue3 = 0;
    let moneyFlow = 0;
    let posFlow = 0;
    let negFlow = 0;
    let posClamped = 0;
    let cur_outReal = 0;
    let negSumMF = sp.negSumMF;
    let nullRun = sp.nullRun;
    let posSumMF = sp.posSumMF;
    let prevValue = sp.prevValue;
    posSumMF -= sp.cb_mflow_positive[sp.mflow_Idx];
    negSumMF -= sp.cb_mflow_negative[sp.mflow_Idx];
    tempValue1 = (inHigh + inLow + inClose) / 3.0;
    tempValue2 = tempValue1 - prevValue;
    /* Dead-zone scaled to the two typical prices being compared (issue #107).
     * Captured before prevValue/tempValue1 are repurposed below.
     */
    tempValue3 = Math.abs(tempValue1) + Math.abs(prevValue);
    prevValue = tempValue1;
    tempValue1 *= inVolume;
    moneyFlow = (Math.abs(tempValue2) <= 0.00000000000001 * (tempValue3)) ? 0.0 : tempValue1;
    posFlow = (tempValue2 < 0.0) ? 0.0 : moneyFlow;
    negFlow = (tempValue2 < 0.0) ? moneyFlow : 0.0;
    posSumMF += posFlow;
    negSumMF += negFlow;
    nullRun = (moneyFlow === 0.0) ? nullRun + 1 : 0;
    if (nullRun >= sp.optInTimePeriod) {
      nullRun = sp.optInTimePeriod;
      posSumMF = 0.0;
      negSumMF = 0.0;
    }
    tempValue1 = posSumMF + negSumMF;
    posClamped = (posSumMF < 0.0) ? 0.0 : ((posSumMF > tempValue1) ? tempValue1 : posSumMF);
    if (tempValue1 <= 0.0) {
      cur_outReal = 0.0;
    } else {
      cur_outReal = 100.0 * (posClamped / tempValue1);
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
  clone(): MfiStream {
    const c = new MfiStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.posSumMF = this.posSumMF;
    c.negSumMF = this.negSumMF;
    c.prevValue = this.prevValue;
    c.nullRun = this.nullRun;
    c.mflow_Idx = this.mflow_Idx;
    c.maxIdx_mflow = this.maxIdx_mflow;
    c.cbSize_mflow = this.cbSize_mflow;
    c.cb_mflow_positive = this.cb_mflow_positive.slice();
    c.cb_mflow_negative = this.cb_mflow_negative.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function mfiStepImpl(sp: MfiStream, inHigh: number, inLow: number, inClose: number, inVolume: number): void {
  let tempValue1 = 0;
  let tempValue2 = 0;
  let tempValue3 = 0;
  let moneyFlow = 0;
  let posFlow = 0;
  let negFlow = 0;
  let posClamped = 0;
  sp.posSumMF -= sp.cb_mflow_positive[sp.mflow_Idx];
  sp.negSumMF -= sp.cb_mflow_negative[sp.mflow_Idx];
  tempValue1 = (inHigh + inLow + inClose) / 3.0;
  tempValue2 = tempValue1 - sp.prevValue;
  /* Dead-zone scaled to the two typical prices being compared (issue #107).
   * Captured before prevValue/tempValue1 are repurposed below.
   */
  tempValue3 = Math.abs(tempValue1) + Math.abs(sp.prevValue);
  sp.prevValue = tempValue1;
  tempValue1 *= inVolume;
  moneyFlow = (Math.abs(tempValue2) <= 0.00000000000001 * (tempValue3)) ? 0.0 : tempValue1;
  posFlow = (tempValue2 < 0.0) ? 0.0 : moneyFlow;
  negFlow = (tempValue2 < 0.0) ? moneyFlow : 0.0;
  sp.cb_mflow_positive[sp.mflow_Idx] = posFlow;
  sp.cb_mflow_negative[sp.mflow_Idx] = negFlow;
  sp.posSumMF += posFlow;
  sp.negSumMF += negFlow;
  sp.nullRun = (moneyFlow === 0.0) ? sp.nullRun + 1 : 0;
  if (sp.nullRun >= sp.optInTimePeriod) {
    sp.nullRun = sp.optInTimePeriod;
    sp.posSumMF = 0.0;
    sp.negSumMF = 0.0;
  }
  tempValue1 = sp.posSumMF + sp.negSumMF;
  posClamped = (sp.posSumMF < 0.0) ? 0.0 : ((sp.posSumMF > tempValue1) ? tempValue1 : sp.posSumMF);
  if (tempValue1 <= 0.0) {
    sp.cur_outReal = 0.0;
  } else {
    sp.cur_outReal = 100.0 * (posClamped / tempValue1);
  }
  sp.mflow_Idx = sp.mflow_Idx + 1;
  if (sp.mflow_Idx > sp.maxIdx_mflow) {
    sp.mflow_Idx = 0;
  }
}

function mfiOpenImpl(
  sp: MfiStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let posSumMF = 0;
  let negSumMF = 0;
  let prevValue = 0;
  let tempValue1 = 0;
  let tempValue2 = 0;
  let tempValue3 = 0;
  let moneyFlow = 0;
  let posFlow = 0;
  let negFlow = 0;
  let posClamped = 0;
  let lookbackTotal = 0;
  let outIdx = 0;
  let i = 0;
  let today = 0;
  let nullRun = 0;
  let mflow_positive!: Float64Array;
  let mflow_negative!: Float64Array;
  let mflow_Idx = 0;
  let maxIdx_mflow = (50) - 1;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length || inClose.length !== inHigh.length || inVolume.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
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
  /* Id, Type, Static Size */
  outBegIdx.value = 0;
  outNBElement.value = 0;
  /* Adjust startIdx to account for the lookback period. */
  lookbackTotal = optInTimePeriod;
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  mflow_positive = new Float64Array(optInTimePeriod);
  mflow_negative = new Float64Array(optInTimePeriod);
  maxIdx_mflow = optInTimePeriod - 1;
  mflow_Idx = 0;
  outIdx = 0;
  /* Index into the output. */
  /* Accumulate the positive and negative money flow
   * among the initial period.
   */
  today = startIdx - lookbackTotal;
  prevValue = (inHigh[today] + inLow[today] + inClose[today]) / 3.0;
  posSumMF = 0.0;
  negSumMF = 0.0;
  /* Consecutive bars that put nothing into the window, counted so that an
   * empty window can be recognized exactly (issue #244).  The running sums
   * cannot answer that question themselves: they are maintained by
   * add-then-subtract, so when the window empties they hold rounding
   * residue of arbitrary sign, not zero.
   */
  nullRun = 0;
  today += 1;
  for (i = optInTimePeriod; i > 0; i -= 1) {
    tempValue1 = (inHigh[today] + inLow[today] + inClose[today]) / 3.0;
    tempValue2 = tempValue1 - prevValue;
    /* Dead-zone scaled to the two typical prices being compared (issue #107).
     * Captured before prevValue/tempValue1 are repurposed below.
     */
    tempValue3 = Math.abs(tempValue1) + Math.abs(prevValue);
    prevValue = tempValue1;
    tempValue1 *= inVolume[today++];
    /* This bar's money flow, and its split into the positive and negative
     * sums.  Selects rather than a three-arm branch: the direction of a
     * price move is a coin flip, so that branch mispredicted on roughly
     * every other bar and dominated the cost of the function.  Adding the
     * unused side's 0.0 to a sum is an exact no-op, so this reproduces the
     * branching form bit for bit.
     */
    moneyFlow = (Math.abs(tempValue2) <= 0.00000000000001 * (tempValue3)) ? 0.0 : tempValue1;
    posFlow = (tempValue2 < 0.0) ? 0.0 : moneyFlow;
    negFlow = (tempValue2 < 0.0) ? moneyFlow : 0.0;
    mflow_positive[mflow_Idx] = posFlow;
    mflow_negative[mflow_Idx] = negFlow;
    posSumMF += posFlow;
    negSumMF += negFlow;
    /* A bar contributes nothing when the typical price did not move, or
     * when it moved but carried no volume.  Once a whole period of those
     * has gone by, every slot of the ring is 0.0, so the sums are known to
     * be exactly zero and the residue can be dropped.
     */
    nullRun = (moneyFlow === 0.0) ? nullRun + 1 : 0;
    if (nullRun >= optInTimePeriod) {
      nullRun = optInTimePeriod;
      posSumMF = 0.0;
      negSumMF = 0.0;
    }
    mflow_Idx++;
    if (mflow_Idx > maxIdx_mflow) {
      mflow_Idx = 0;
    }
  }
  /* The following two equations are equivalent:
   *    MFI = 100 - (100 / 1 + (posSumMF/negSumMF))
   *    MFI = 100 * (posSumMF/(posSumMF+negSumMF))
   * The second equation is used here for speed optimization.
   *
   * Both sums are non-negative, so the total is zero only for a window that
   * received no money flow at all -- 0/0, reported as 0.0.  The test is on
   * the total itself, not on a fixed threshold: money flow is a price times
   * a volume, so any constant compared against it is a constant in some
   * arbitrary unit, and would zero a healthy index for any instrument
   * quoted small enough to fall under it (issue #244).
   *
   * Clamping the numerator into [0,total] keeps the result inside the
   * documented 0-100 range: the sums drift by a few ulp as the window
   * slides, and a sum whose true value is near zero can drift negative.
   */
  /* The first full window is complete: emit its output for startIdx here,
   * then slide the window over the remaining bars below.
   */
  tempValue1 = posSumMF + negSumMF;
  posClamped = (posSumMF < 0.0) ? 0.0 : ((posSumMF > tempValue1) ? tempValue1 : posSumMF);
  if (tempValue1 <= 0.0) {
    outReal[((outIdx++ * outStride) | 0)] = 0.0;
  } else {
    outReal[((outIdx++ * outStride) | 0)] = 100.0 * (posClamped / tempValue1);
  }
  /* Now continue processing the remaining bars. */
  while (today <= endIdx) {
    posSumMF -= mflow_positive[mflow_Idx];
    negSumMF -= mflow_negative[mflow_Idx];
    tempValue1 = (inHigh[today] + inLow[today] + inClose[today]) / 3.0;
    tempValue2 = tempValue1 - prevValue;
    /* Dead-zone scaled to the two typical prices being compared (issue #107).
     * Captured before prevValue/tempValue1 are repurposed below.
     */
    tempValue3 = Math.abs(tempValue1) + Math.abs(prevValue);
    prevValue = tempValue1;
    tempValue1 *= inVolume[today++];
    moneyFlow = (Math.abs(tempValue2) <= 0.00000000000001 * (tempValue3)) ? 0.0 : tempValue1;
    posFlow = (tempValue2 < 0.0) ? 0.0 : moneyFlow;
    negFlow = (tempValue2 < 0.0) ? moneyFlow : 0.0;
    mflow_positive[mflow_Idx] = posFlow;
    mflow_negative[mflow_Idx] = negFlow;
    posSumMF += posFlow;
    negSumMF += negFlow;
    nullRun = (moneyFlow === 0.0) ? nullRun + 1 : 0;
    if (nullRun >= optInTimePeriod) {
      nullRun = optInTimePeriod;
      posSumMF = 0.0;
      negSumMF = 0.0;
    }
    tempValue1 = posSumMF + negSumMF;
    posClamped = (posSumMF < 0.0) ? 0.0 : ((posSumMF > tempValue1) ? tempValue1 : posSumMF);
    if (tempValue1 <= 0.0) {
      outReal[((outIdx++ * outStride) | 0)] = 0.0;
    } else {
      outReal[((outIdx++ * outStride) | 0)] = 100.0 * (posClamped / tempValue1);
    }
    mflow_Idx++;
    if (mflow_Idx > maxIdx_mflow) {
      mflow_Idx = 0;
    }
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  const capCb_mflow = maxIdx_mflow + 1;
  if (capCb_mflow > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.posSumMF = posSumMF;
  sp.negSumMF = negSumMF;
  sp.prevValue = prevValue;
  sp.nullRun = nullRun;
  sp.mflow_Idx = mflow_Idx;
  sp.maxIdx_mflow = maxIdx_mflow;
  sp.cbSize_mflow = capCb_mflow;
  sp.cb_mflow_positive = mflow_positive;
  sp.cb_mflow_negative = mflow_negative;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function mfiOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): MfiStream {
  const sp = MfiStream.of(core);
  const retCode = mfiOpenImpl(sp, inHigh, inLow, inClose, inVolume, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MFI openAndFill', inHigh.length, startIdx, mfiLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && mfiLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('MFI openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('MFI openAndFill', retCode);
}

/** @internal */
export function mfiOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): MfiStream {
  const sp = MfiStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = mfiOpenImpl(sp, inHigh, inLow, inClose, inVolume, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MFI open', inHigh.length, startIdx, mfiLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && mfiLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('MFI open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('MFI open', retCode);
}

/** @internal */
export function mfiOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): MfiStream {
  optInTimePeriod = requireInt32('MFI open', 'timePeriod', optInTimePeriod);
  requireArgument('MFI open', 'prices.high', inHigh);
  requireHistory('MFI open', inHigh.length);
  requireArgument('MFI open', 'prices.low', inLow);
  requireArgument('MFI open', 'prices.close', inClose);
  requireArgument('MFI open', 'prices.volume', inVolume);
  requireHistoryLength('MFI open', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('MFI open', 'prices.close', inClose.length, inHigh.length);
  requireHistoryLength('MFI open', 'prices.volume', inVolume.length, inHigh.length);
  return mfiOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), float64(inVolume), 0, optInTimePeriod);
}

/** @internal */
export function mfiOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): MfiStream {
  optInTimePeriod = requireInt32('MFI openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('MFI openAndFill', 'prices.high', inHigh);
  requireHistory('MFI openAndFill', inHigh.length);
  requireArgument('MFI openAndFill', 'prices.low', inLow);
  requireArgument('MFI openAndFill', 'prices.close', inClose);
  requireArgument('MFI openAndFill', 'prices.volume', inVolume);
  const guardLookback = mfiLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('MFI openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('MFI openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('MFI openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('MFI openAndFill', 'prices.close', inClose.length, inHigh.length);
  requireHistoryLength('MFI openAndFill', 'prices.volume', inVolume.length, inHigh.length);
  requireOutput('MFI openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  const inVolumeF64 = float64(inVolume);
  if (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('MFI openAndFill', 'outReal', 'prices.high');
  }
  if (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('MFI openAndFill', 'outReal', 'prices.low');
  }
  if (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('MFI openAndFill', 'outReal', 'prices.close');
  }
  if (outReal.buffer === inVolumeF64.buffer && outReal.byteOffset === inVolumeF64.byteOffset) {
    throw sharedMemory('MFI openAndFill', 'outReal', 'prices.volume');
  }
  if (guardOutLen > 0 && overlaps(outReal, inHighF64)) {
    throw sharedMemory('MFI openAndFill', 'outReal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outReal, inLowF64)) {
    throw sharedMemory('MFI openAndFill', 'outReal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outReal, inCloseF64)) {
    throw sharedMemory('MFI openAndFill', 'outReal', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outReal, inVolumeF64)) {
    throw sharedMemory('MFI openAndFill', 'outReal', 'prices.volume');
  }
  return mfiOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, inVolumeF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live MFI stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link mfi} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar. `volume`: Volume of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MfiOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `mfiLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or
 *   histories of different lengths.
 *
 * @example
 * ```ts
 * import { mfiOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const stream = mfiOpen({ high, low, close, volume }, { timePeriod: 14 });
 * const peeked = stream.peek(101.4, 99.1, 100.9, 12345); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9, 12345); // the bar closes
 * ```
 */
export function mfiOpen(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume: ArrayLike<number> }, options?: MfiOptions): MfiStream {
  requireArgument('MFI open', 'prices', prices);
  const o = optionsOf('MFI open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MFI open', o.core);
  return mfiOpenPositional(prices.high, prices.low, prices.close, prices.volume, optInTimePeriod, core);
}

/**
 * {@link mfiOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link mfi}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar. `volume`: Volume of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link MfiOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link mfi} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `mfiLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { mfiOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const { stream, begIdx, values } = mfiOpenAndFill({ high, low, close, volume }, {
 *   timePeriod: 14,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function mfiOpenAndFill(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume: ArrayLike<number> }, options?: MfiOptions): MfiResult & { stream: MfiStream } {
  requireArgument('MFI openAndFill', 'prices', prices);
  const o = optionsOf('MFI openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MFI openAndFill', o.core);
  const capacity = fillCapacity(mfiLookbackPositional(optInTimePeriod, core), prices.high, prices.low, prices.close, prices.volume);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = mfiOpenAndFillPositional(prices.high, prices.low, prices.close, prices.volume, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
