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
 *  072126 MF,CC  First version (issue #134).
 *  092526 MF,CC  #446 exact zero sums on a dead volume window.
 *  092626 MF,CC  #446 branch-free zero run.
 *  102326 MF,CC  #554 rare reset, zero run as a mask.
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function cmfLookbackPositional(optInTimePeriod: number, _core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  return optInTimePeriod - 1;
}

/** @internal */
export function cmfDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (cmfLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cmfImpl(
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
  let sumMFV = 0;
  let sumVol = 0;
  let high = 0;
  let low = 0;
  let close = 0;
  let tmp = 0;
  let mfv = 0;
  let lookbackTotal = 0;
  let outIdx = 0;
  let i = 0;
  let today = 0;
  let nullRun = 0;
  let liveVol = 0;
  let mfv_flow!: Float64Array;
  let mfv_volume!: Float64Array;
  let mfv_Idx = 0;
  let maxIdx_mfv = (50) - 1;
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
  /* Both the per-bar money flow volume and the volume that produced it are
   * carried in the circular buffer. Keeping the volume here rather than
   * re-reading inVolume[] at the trailing index is what makes outReal safe to
   * alias any input: once a bar has been consumed it is never read again.
   */
  /* Id, Type, Static Size */
  outBegIdx.value = 0;
  outNBElement.value = 0;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = optInTimePeriod - 1;
  /* Move up the start index if there is not
   * enough initial data.
   */
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
  mfv_flow = new Float64Array(optInTimePeriod);
  mfv_volume = new Float64Array(optInTimePeriod);
  maxIdx_mfv = optInTimePeriod - 1;
  mfv_Idx = 0;
  outIdx = 0;
  /* Accumulate the money flow volume and the volume over the first
   * complete window, filling the circular buffer as we go.
   *
   * The per-bar multiplier is written exactly as in ta_AD.c so that the
   * Chaikin money flow volume has one definition in the library.
   */
  today = startIdx - lookbackTotal;
  sumMFV = 0.0;
  sumVol = 0.0;
  /* Consecutive zero-volume bars. Once they fill a window both sums are
   * exactly zero, where add-then-subtract would leave the rounding residue of
   * the bars that departed, of either sign. The run advances through a mask:
   * as a select or a product gcc compiles it to a branch that mispredicts on
   * scattered zero volumes. The test is fabs(v) <= 0.0 rather than == 0.0:
   * the same result, NaN included, from one flag instead of two.
   */
  nullRun = 0;
  for (i = optInTimePeriod; i > 0; i -= 1) {
    high = inHigh[today];
    low = inLow[today];
    close = inClose[today];
    tmp = high - low;
    if (tmp > 0.0) {
      mfv = (close - low - (high - close)) / tmp * inVolume[today];
    } else {
      mfv = 0.0;
    }
    mfv_flow[mfv_Idx] = mfv;
    mfv_volume[mfv_Idx] = inVolume[today];
    sumMFV += mfv;
    sumVol += inVolume[today];
    liveVol = (!(Math.abs(inVolume[today]) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (liveVol - 1);
    today += 1;
    mfv_Idx++;
    if (mfv_Idx > maxIdx_mfv) {
      mfv_Idx = 0;
    }
  }
  /* The first full window is complete: emit its output for startIdx here,
   * then slide the window over the remaining bars below.
   *
   * A window whose volume is entirely zero has no money flow to distribute;
   * report 0.0 rather than propagating a division by zero (issue #112).
   */
  if (sumVol > 0.0) {
    outReal[outIdx++] = sumMFV / sumVol;
  } else {
    outReal[outIdx++] = 0.0;
  }
  /* Now continue processing the remaining bars. */
  while (today <= endIdx) {
    sumMFV -= mfv_flow[mfv_Idx];
    sumVol -= mfv_volume[mfv_Idx];
    high = inHigh[today];
    low = inLow[today];
    close = inClose[today];
    tmp = high - low;
    if (tmp > 0.0) {
      mfv = (close - low - (high - close)) / tmp * inVolume[today];
    } else {
      mfv = 0.0;
    }
    mfv_flow[mfv_Idx] = mfv;
    mfv_volume[mfv_Idx] = inVolume[today];
    sumMFV += mfv;
    sumVol += inVolume[today];
    liveVol = (!(Math.abs(inVolume[today]) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (liveVol - 1);
    today += 1;
    if (nullRun >= optInTimePeriod) {
      nullRun = optInTimePeriod;
      sumMFV = 0.0;
      sumVol = 0.0;
      outReal[outIdx++] = 0.0;
    } else if (sumVol > 0.0) {
      outReal[outIdx++] = sumMFV / sumVol;
    } else {
      outReal[outIdx++] = 0.0;
    }
    mfv_Idx++;
    if (mfv_Idx > maxIdx_mfv) {
      mfv_Idx = 0;
    }
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000']] as const;

/** @internal */
export function cmfPositional(
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
  startIdx = requireInt32('CMF', 'startIdx', startIdx);
  endIdx = requireInt32('CMF', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('CMF', 'timePeriod', optInTimePeriod);
  requireIndexRange('CMF', startIdx, endIdx);
  const guardLookback = cmfLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('CMF', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('CMF', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CMF', 'prices.high', inHigh, guardInLen);
  requireLength('CMF', 'prices.low', inLow, guardInLen);
  requireLength('CMF', 'prices.close', inClose, guardInLen);
  requireLength('CMF', 'prices.volume', inVolume, guardInLen);
  requireOutput('CMF', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  const inVolumeF64 = float64(inVolume);
  if (overlapsPartly(outReal, inHighF64)) {
    throw sharedMemory('CMF', 'outReal', 'prices.high');
  }
  if (overlapsPartly(outReal, inLowF64)) {
    throw sharedMemory('CMF', 'outReal', 'prices.low');
  }
  if (overlapsPartly(outReal, inCloseF64)) {
    throw sharedMemory('CMF', 'outReal', 'prices.close');
  }
  if (overlapsPartly(outReal, inVolumeF64)) {
    throw sharedMemory('CMF', 'outReal', 'prices.volume');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cmfImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, inVolumeF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CMF', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cmf}, {@link cmfLookback}, {@link cmfDisplayShift},
 * {@link cmfOpen} and {@link cmfOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface CmfOptions {
  /**
   * Number of bars in the window (default 20; range 2 to 100000;
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

/** What {@link cmf} returns, and {@link cmfOpenAndFill} with its stream. */
export interface CmfResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Chaikin money flow, in the range -1 to +1. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link cmf} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CmfOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cmfLookback(options?: CmfOptions): number {
  const o = optionsOf('CMF lookback', options);
  return cmfLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('CMF lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cmf}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CmfOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cmfDisplayShift(outputIdx: number, options?: CmfOptions): number {
  const o = optionsOf('CMF displayShift', options);
  return cmfDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('CMF displayShift', o.core));
}

/**
 * Chaikin Money Flow: over a trailing window of `optInTimePeriod` bars, the sum of
 * each bar's money flow volume divided by the sum of its volume. The result is a
 * ratio in `[-1, +1]`. A bar's money flow volume is its volume scaled by where the
 * close sat inside the bar's range: a close at the high contributes the full
 * volume, a close at the low contributes minus the full volume, and a close at the
 * midpoint contributes nothing. Summing that over a window and dividing by the
 * window's volume answers "over these N bars, what share of the traded volume
 * closed near the top of its range?" Above zero is accumulation, below zero is
 * distribution, and the distance from zero measures conviction. Because the
 * divisor is the window's own volume, the output is comparable across instruments
 * and across time in a way a raw accumulation total is not. Created by Marc
 * Chaikin, who also authored the [`AD`](https://ta-lib.org/functions/ad) line this
 * shares its per-bar multiplier with. CMF is that same multiplier summed over a
 * fixed window and normalised, where AD accumulates it from the start of the
 * series without bound.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cmf](https://ta-lib.org/functions/cmf).
 *
 * **Notes**
 * - The output is the raw ratio in `[-1, +1]`, matching every published
 *   definition. Some retail platforms display it multiplied by 100; that is a
 *   presentation choice, not a different indicator.
 * - Each bar's close is expected to lie within its own `[low, high]`, and its
 *   volume to be finite and non-negative. A close outside its bar makes the
 *   multiplier exceed ±1 and is passed through unclamped, exactly as
 *   [`AD`](https://ta-lib.org/functions/ad) does.
 * - A bar whose high equals its low has no range for the close to sit inside, so
 *   it contributes exactly zero money flow volume rather than dividing by zero.
 *   Its volume still counts toward the divisor.
 * - A window whose volume is entirely zero has no money flow to distribute and
 *   reports 0.0. Published references are silent here and other implementations
 *   divide by zero; TA-Lib does not return NaN from a successful call.
 * - Bars where the low exceeds the high are malformed rather than degenerate, and
 *   also contribute zero.
 * - The default period of 20 follows the original write-up, which describes 20 or
 *   21 bars.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cmfLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `high`: High
 *   price of each bar. `low`: Low price of each bar. `close`: Close price of each
 *   bar. `volume`: Volume of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link CmfOptions}.
 * @returns `begIdx` and the values; see {@link CmfResult}. Each output is a view
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
 * import { cmf } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const { begIdx, values } = cmf({ high, low, close, volume }, {
 *   timePeriod: 20,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`ad`](https://ta-lib.org/functions/ad)
 * @see [`adosc`](https://ta-lib.org/functions/adosc)
 * @see [`mfi`](https://ta-lib.org/functions/mfi)
 * @see [`obv`](https://ta-lib.org/functions/obv)
 */
export function cmf(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume: ArrayLike<number> }, options?: CmfOptions): CmfResult {
  requireArgument('CMF', 'prices', prices);
  const o = optionsOf('CMF', options);
  if (o.endIdx === undefined) {
    requireSeries('CMF', 'prices.high', prices.high);
    requireSeries('CMF', 'prices.low', prices.low);
    requireSeries('CMF', 'prices.close', prices.close);
    requireSeries('CMF', 'prices.volume', prices.volume);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('CMF', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cmfLookbackPositional(optInTimePeriod, core), prices.high, prices.low, prices.close, prices.volume);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = cmfPositional(startIdx, endIdx, prices.high, prices.low, prices.close, prices.volume, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CMF stream: one value per closed bar, bit-identical to {@link cmf} over
 * the same series. Open it with {@link cmfOpen} or {@link cmfOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cmf](https://ta-lib.org/functions/cmf).
 */
export class CmfStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  sumMFV: number = 0;
  /** @internal */
  sumVol: number = 0;
  /** @internal */
  nullRun: number = 0;
  /** @internal */
  mfv_Idx: number = 0;
  /** @internal */
  maxIdx_mfv: number = 0;
  /** @internal */
  cbSize_mfv: number = 0;
  /** @internal */
  cb_mfv_flow: Float64Array = new Float64Array(1);
  /** @internal */
  cb_mfv_volume: Float64Array = new Float64Array(1);
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
  static of(core: Core): CmfStream {
    return new CmfStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cmf} reports them over the same bars. Each accepted `update` and each
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
      throw failure('CMF advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw failure('CMF update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose) || !Number.isFinite(inVolume)) {
      throw nonFiniteBar('CMF update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : !Number.isFinite(inClose) ? 'inClose' : 'inVolume');
    }
    cmfStepImpl(this, inHigh, inLow, inClose, inVolume);
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
      throw nonFiniteBar('CMF peek', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : !Number.isFinite(inClose) ? 'inClose' : 'inVolume');
    }
    const sp = this;
    let high = 0;
    let low = 0;
    let close = 0;
    let tmp = 0;
    let mfv = 0;
    let liveVol = 0;
    let cur_outReal = 0;
    let nullRun = sp.nullRun;
    let sumMFV = sp.sumMFV;
    let sumVol = sp.sumVol;
    sumMFV -= sp.cb_mfv_flow[sp.mfv_Idx];
    sumVol -= sp.cb_mfv_volume[sp.mfv_Idx];
    high = inHigh;
    low = inLow;
    close = inClose;
    tmp = high - low;
    if (tmp > 0.0) {
      mfv = (close - low - (high - close)) / tmp * inVolume;
    } else {
      mfv = 0.0;
    }
    sumMFV += mfv;
    sumVol += inVolume;
    liveVol = (!(Math.abs(inVolume) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (liveVol - 1);
    if (nullRun >= sp.optInTimePeriod) {
      nullRun = sp.optInTimePeriod;
      sumMFV = 0.0;
      sumVol = 0.0;
      cur_outReal = 0.0;
    } else if (sumVol > 0.0) {
      cur_outReal = sumMFV / sumVol;
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
  clone(): CmfStream {
    const c = new CmfStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.sumMFV = this.sumMFV;
    c.sumVol = this.sumVol;
    c.nullRun = this.nullRun;
    c.mfv_Idx = this.mfv_Idx;
    c.maxIdx_mfv = this.maxIdx_mfv;
    c.cbSize_mfv = this.cbSize_mfv;
    c.cb_mfv_flow = this.cb_mfv_flow.slice();
    c.cb_mfv_volume = this.cb_mfv_volume.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cmfStepImpl(sp: CmfStream, inHigh: number, inLow: number, inClose: number, inVolume: number): void {
  let high = 0;
  let low = 0;
  let close = 0;
  let tmp = 0;
  let mfv = 0;
  let liveVol = 0;
  sp.sumMFV -= sp.cb_mfv_flow[sp.mfv_Idx];
  sp.sumVol -= sp.cb_mfv_volume[sp.mfv_Idx];
  high = inHigh;
  low = inLow;
  close = inClose;
  tmp = high - low;
  if (tmp > 0.0) {
    mfv = (close - low - (high - close)) / tmp * inVolume;
  } else {
    mfv = 0.0;
  }
  sp.cb_mfv_flow[sp.mfv_Idx] = mfv;
  sp.cb_mfv_volume[sp.mfv_Idx] = inVolume;
  sp.sumMFV += mfv;
  sp.sumVol += inVolume;
  liveVol = (!(Math.abs(inVolume) <= 0.0) ? 1 : 0);
  sp.nullRun = (sp.nullRun + 1) & (liveVol - 1);
  if (sp.nullRun >= sp.optInTimePeriod) {
    sp.nullRun = sp.optInTimePeriod;
    sp.sumMFV = 0.0;
    sp.sumVol = 0.0;
    sp.cur_outReal = 0.0;
  } else if (sp.sumVol > 0.0) {
    sp.cur_outReal = sp.sumMFV / sp.sumVol;
  } else {
    sp.cur_outReal = 0.0;
  }
  sp.mfv_Idx = sp.mfv_Idx + 1;
  if (sp.mfv_Idx > sp.maxIdx_mfv) {
    sp.mfv_Idx = 0;
  }
}

function cmfOpenImpl(
  sp: CmfStream,
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
  let sumMFV = 0;
  let sumVol = 0;
  let high = 0;
  let low = 0;
  let close = 0;
  let tmp = 0;
  let mfv = 0;
  let lookbackTotal = 0;
  let outIdx = 0;
  let i = 0;
  let today = 0;
  let nullRun = 0;
  let liveVol = 0;
  let mfv_flow!: Float64Array;
  let mfv_volume!: Float64Array;
  let mfv_Idx = 0;
  let maxIdx_mfv = (50) - 1;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length || inClose.length !== inHigh.length || inVolume.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Both the per-bar money flow volume and the volume that produced it are
   * carried in the circular buffer. Keeping the volume here rather than
   * re-reading inVolume[] at the trailing index is what makes outReal safe to
   * alias any input: once a bar has been consumed it is never read again.
   */
  /* Id, Type, Static Size */
  outBegIdx.value = 0;
  outNBElement.value = 0;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = optInTimePeriod - 1;
  /* Move up the start index if there is not
   * enough initial data.
   */
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
  mfv_flow = new Float64Array(optInTimePeriod);
  mfv_volume = new Float64Array(optInTimePeriod);
  maxIdx_mfv = optInTimePeriod - 1;
  mfv_Idx = 0;
  outIdx = 0;
  /* Accumulate the money flow volume and the volume over the first
   * complete window, filling the circular buffer as we go.
   *
   * The per-bar multiplier is written exactly as in ta_AD.c so that the
   * Chaikin money flow volume has one definition in the library.
   */
  today = startIdx - lookbackTotal;
  sumMFV = 0.0;
  sumVol = 0.0;
  /* Consecutive zero-volume bars. Once they fill a window both sums are
   * exactly zero, where add-then-subtract would leave the rounding residue of
   * the bars that departed, of either sign. The run advances through a mask:
   * as a select or a product gcc compiles it to a branch that mispredicts on
   * scattered zero volumes. The test is fabs(v) <= 0.0 rather than == 0.0:
   * the same result, NaN included, from one flag instead of two.
   */
  nullRun = 0;
  for (i = optInTimePeriod; i > 0; i -= 1) {
    high = inHigh[today];
    low = inLow[today];
    close = inClose[today];
    tmp = high - low;
    if (tmp > 0.0) {
      mfv = (close - low - (high - close)) / tmp * inVolume[today];
    } else {
      mfv = 0.0;
    }
    mfv_flow[mfv_Idx] = mfv;
    mfv_volume[mfv_Idx] = inVolume[today];
    sumMFV += mfv;
    sumVol += inVolume[today];
    liveVol = (!(Math.abs(inVolume[today]) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (liveVol - 1);
    today += 1;
    mfv_Idx++;
    if (mfv_Idx > maxIdx_mfv) {
      mfv_Idx = 0;
    }
  }
  /* The first full window is complete: emit its output for startIdx here,
   * then slide the window over the remaining bars below.
   *
   * A window whose volume is entirely zero has no money flow to distribute;
   * report 0.0 rather than propagating a division by zero (issue #112).
   */
  if (sumVol > 0.0) {
    outReal[((outIdx++ * outStride) | 0)] = sumMFV / sumVol;
  } else {
    outReal[((outIdx++ * outStride) | 0)] = 0.0;
  }
  /* Now continue processing the remaining bars. */
  while (today <= endIdx) {
    sumMFV -= mfv_flow[mfv_Idx];
    sumVol -= mfv_volume[mfv_Idx];
    high = inHigh[today];
    low = inLow[today];
    close = inClose[today];
    tmp = high - low;
    if (tmp > 0.0) {
      mfv = (close - low - (high - close)) / tmp * inVolume[today];
    } else {
      mfv = 0.0;
    }
    mfv_flow[mfv_Idx] = mfv;
    mfv_volume[mfv_Idx] = inVolume[today];
    sumMFV += mfv;
    sumVol += inVolume[today];
    liveVol = (!(Math.abs(inVolume[today]) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (liveVol - 1);
    today += 1;
    if (nullRun >= optInTimePeriod) {
      nullRun = optInTimePeriod;
      sumMFV = 0.0;
      sumVol = 0.0;
      outReal[((outIdx++ * outStride) | 0)] = 0.0;
    } else if (sumVol > 0.0) {
      outReal[((outIdx++ * outStride) | 0)] = sumMFV / sumVol;
    } else {
      outReal[((outIdx++ * outStride) | 0)] = 0.0;
    }
    mfv_Idx++;
    if (mfv_Idx > maxIdx_mfv) {
      mfv_Idx = 0;
    }
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  const capCb_mfv = maxIdx_mfv + 1;
  if (capCb_mfv > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.sumMFV = sumMFV;
  sp.sumVol = sumVol;
  sp.nullRun = nullRun;
  sp.mfv_Idx = mfv_Idx;
  sp.maxIdx_mfv = maxIdx_mfv;
  sp.cbSize_mfv = capCb_mfv;
  sp.cb_mfv_flow = mfv_flow;
  sp.cb_mfv_volume = mfv_volume;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cmfOpenAndFillInternal(
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
): CmfStream {
  const sp = CmfStream.of(core);
  const retCode = cmfOpenImpl(sp, inHigh, inLow, inClose, inVolume, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CMF openAndFill', inHigh.length, startIdx, cmfLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && cmfLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('CMF openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('CMF openAndFill', retCode);
}

/** @internal */
export function cmfOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): CmfStream {
  const sp = CmfStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = cmfOpenImpl(sp, inHigh, inLow, inClose, inVolume, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CMF open', inHigh.length, startIdx, cmfLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && cmfLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('CMF open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('CMF open', retCode);
}

/** @internal */
export function cmfOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): CmfStream {
  optInTimePeriod = requireInt32('CMF open', 'timePeriod', optInTimePeriod);
  requireArgument('CMF open', 'prices.high', inHigh);
  requireHistory('CMF open', inHigh.length);
  requireArgument('CMF open', 'prices.low', inLow);
  requireArgument('CMF open', 'prices.close', inClose);
  requireArgument('CMF open', 'prices.volume', inVolume);
  requireHistoryLength('CMF open', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('CMF open', 'prices.close', inClose.length, inHigh.length);
  requireHistoryLength('CMF open', 'prices.volume', inVolume.length, inHigh.length);
  return cmfOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), float64(inVolume), 0, optInTimePeriod);
}

/** @internal */
export function cmfOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): CmfStream {
  optInTimePeriod = requireInt32('CMF openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('CMF openAndFill', 'prices.high', inHigh);
  requireHistory('CMF openAndFill', inHigh.length);
  requireArgument('CMF openAndFill', 'prices.low', inLow);
  requireArgument('CMF openAndFill', 'prices.close', inClose);
  requireArgument('CMF openAndFill', 'prices.volume', inVolume);
  const guardLookback = cmfLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('CMF openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('CMF openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('CMF openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('CMF openAndFill', 'prices.close', inClose.length, inHigh.length);
  requireHistoryLength('CMF openAndFill', 'prices.volume', inVolume.length, inHigh.length);
  requireOutput('CMF openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  const inVolumeF64 = float64(inVolume);
  if (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CMF openAndFill', 'outReal', 'prices.high');
  }
  if (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CMF openAndFill', 'outReal', 'prices.low');
  }
  if (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('CMF openAndFill', 'outReal', 'prices.close');
  }
  if (outReal.buffer === inVolumeF64.buffer && outReal.byteOffset === inVolumeF64.byteOffset) {
    throw sharedMemory('CMF openAndFill', 'outReal', 'prices.volume');
  }
  if (guardOutLen > 0 && overlaps(outReal, inHighF64)) {
    throw sharedMemory('CMF openAndFill', 'outReal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outReal, inLowF64)) {
    throw sharedMemory('CMF openAndFill', 'outReal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outReal, inCloseF64)) {
    throw sharedMemory('CMF openAndFill', 'outReal', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outReal, inVolumeF64)) {
    throw sharedMemory('CMF openAndFill', 'outReal', 'prices.volume');
  }
  return cmfOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, inVolumeF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live CMF stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link cmf} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar. `volume`: Volume of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CmfOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cmfLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or
 *   histories of different lengths.
 *
 * @example
 * ```ts
 * import { cmfOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const stream = cmfOpen({ high, low, close, volume }, { timePeriod: 20 });
 * const peeked = stream.peek(101.4, 99.1, 100.9, 12345); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9, 12345); // the bar closes
 * ```
 */
export function cmfOpen(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume: ArrayLike<number> }, options?: CmfOptions): CmfStream {
  requireArgument('CMF open', 'prices', prices);
  const o = optionsOf('CMF open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('CMF open', o.core);
  return cmfOpenPositional(prices.high, prices.low, prices.close, prices.volume, optInTimePeriod, core);
}

/**
 * {@link cmfOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link cmf}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar. `volume`: Volume of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CmfOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cmf} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cmfLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { cmfOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const { stream, begIdx, values } = cmfOpenAndFill({ high, low, close, volume }, {
 *   timePeriod: 20,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cmfOpenAndFill(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume: ArrayLike<number> }, options?: CmfOptions): CmfResult & { stream: CmfStream } {
  requireArgument('CMF openAndFill', 'prices', prices);
  const o = optionsOf('CMF openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('CMF openAndFill', o.core);
  const capacity = fillCapacity(cmfLookbackPositional(optInTimePeriod, core), prices.high, prices.low, prices.close, prices.volume);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = cmfOpenAndFillPositional(prices.high, prices.low, prices.close, prices.volume, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
