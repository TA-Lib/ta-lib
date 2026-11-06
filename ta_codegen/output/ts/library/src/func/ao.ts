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
 *  081626 MF,CC  Initial version (#227).
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { smaLookbackPositional } from './sma.ts';

/** @internal */
export function aoLookbackPositional(optInFastPeriod: number, optInSlowPeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInFastPeriod | 0) !== optInFastPeriod) {
    return -1;
  }
  if ((optInSlowPeriod | 0) !== optInSlowPeriod) {
    return -1;
  }
  if (optInFastPeriod === Core.INTEGER_DEFAULT) {
    optInFastPeriod = 5;
  } else if (optInFastPeriod < 2 || optInFastPeriod > 100000) {
    return -1;
  }
  if (optInSlowPeriod === Core.INTEGER_DEFAULT) {
    optInSlowPeriod = 34;
  } else if (optInSlowPeriod < 2 || optInSlowPeriod > 100000) {
    return -1;
  }
  /* The longer of the two windows drives the lookback, and it is the lookback
   * of that window's SMA. There is no swap of an inverted pair, so the max is
   * taken over the periods exactly as the caller gave them.
   */
  return smaLookbackPositional((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod), core);
}

/** @internal */
export function aoDisplayShiftPositional(optInFastPeriod: number, optInSlowPeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (aoLookbackPositional(optInFastPeriod, optInSlowPeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function aoImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let sumFast = 0;
  let sumSlow = 0;
  let medianPrice = 0;
  let tempReal = 0;
  let i = 0;
  let outIdx = 0;
  let trailingFastIdx = 0;
  let trailingSlowIdx = 0;
  let lookbackTotal = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInFastPeriod === Core.INTEGER_DEFAULT) {
    optInFastPeriod = 5;
  } else if (optInFastPeriod < 2 || optInFastPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowPeriod === Core.INTEGER_DEFAULT) {
    optInSlowPeriod = 34;
  } else if (optInSlowPeriod < 2 || optInSlowPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  /* Bill Williams' Awesome Oscillator (New Trading Dimensions, 1998): the
   * spread between a short and a long simple moving average of the median
   * price, drawn as a zero-centred histogram.
   *
   *    median_t = (high_t + low_t) / 2
   *    AO_t     = SMA(median, fast)_t - SMA(median, slow)_t
   *
   * Both legs are plain SMAs, so there is no seeding convention to get wrong
   * and no cross-library divergence of the kind an EMA brings.
   *
   * This is two copies of ta_codegen/input/sma/sma.c walking one derived
   * series, sharing a single pass: the same running-sum order, the same
   * snapshot of each total before the trailing bar leaves it, and the same
   * divide by the period. Keeping the arithmetic identical is what makes the
   * composed reference in test_composite.c -- TA_MEDPRICE, two TA_SMA calls
   * and a TA_SUB -- bit-exact rather than merely close, so any future drift
   * in either path is a hard failure instead of a tolerance argument.
   *
   * In particular each total is DIVIDED by its period; it is not multiplied
   * by a precomputed 1/period. Tulip's ao.c multiplies by per5/per34, which
   * costs it up to one ULP against TA_SMA. Dividing buys the memcmp
   * differential, which is the stronger of the two gates (#117, #118).
   *
   * An inverted pair (fast > slow) is NOT swapped, unlike MACD and APO. The
   * lookback is well defined either way and the result is simply -AO, so the
   * swap would buy nothing and would have to be duplicated in ao_lookback.
   */
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = aoLookbackPositional(optInFastPeriod, optInSlowPeriod, core);
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
  sumFast = 0.0;
  sumSlow = 0.0;
  trailingFastIdx = startIdx - (optInFastPeriod - 1);
  trailingSlowIdx = startIdx - (optInSlowPeriod - 1);
  /* Add-up both initial periods, except for the last value.
   *
   * One pass over the longer warm-up window replaces two overlapping passes.
   * A bar inside the shorter window is added to that total as it is reached,
   * so each total still accumulates exactly the same bars in exactly the same
   * ascending order two separate loops would have given it -- which is what
   * keeps each leg bit-identical to the TA_SMA called with this same startIdx.
   */
  i = startIdx - lookbackTotal;
  while (i < startIdx) {
    medianPrice = (inHigh[i] + inLow[i]) / 2.0;
    if (i >= trailingFastIdx) {
      sumFast += medianPrice;
    }
    if (i >= trailingSlowIdx) {
      sumSlow += medianPrice;
    }
    i = i + 1;
  }
  /* Proceed with the calculation for the requested range.
   * Note that this algorithm allows outReal to be the same
   * buffer as either input.
   */
  outIdx = 0;
  while (i <= endIdx) {
    medianPrice = (inHigh[i] + inLow[i]) / 2.0;
    sumFast += medianPrice;
    sumSlow += medianPrice;
    i = i + 1;
    /* Snapshot the oscillator before either total drops its trailing bar,
     * mirroring the add-new / snapshot / subtract-old order of TA_SMA.
     */
    tempReal = sumFast / optInFastPeriod - sumSlow / optInSlowPeriod;
    /* Read both trailing bars before writing the output. When startIdx is
     * clamped to the lookback the longer window's trailing index equals
     * outIdx exactly, so a store hoisted above this would read back the
     * value it had just overwritten whenever the caller aliases outReal
     * over inHigh or inLow.
     */
    sumFast -= (inHigh[trailingFastIdx] + inLow[trailingFastIdx]) / 2.0;
    sumSlow -= (inHigh[trailingSlowIdx] + inLow[trailingSlowIdx]) / 2.0;
    trailingFastIdx = trailingFastIdx + 1;
    trailingSlowIdx = trailingSlowIdx + 1;
    outReal[outIdx] = tempReal;
    outIdx = outIdx + 1;
  }
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['fastPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['slowPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000']] as const;

/** @internal */
export function aoPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('AO', 'startIdx', startIdx);
  endIdx = requireInt32('AO', 'endIdx', endIdx);
  optInFastPeriod = requireInt32('AO', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('AO', 'slowPeriod', optInSlowPeriod);
  requireIndexRange('AO', startIdx, endIdx);
  const guardLookback = aoLookbackPositional(optInFastPeriod, optInSlowPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('AO', PARAM_RANGES, optInFastPeriod, optInSlowPeriod);
  }
  const guardStart = clampedStart('AO', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('AO', 'prices.high', inHigh, guardInLen);
  requireLength('AO', 'prices.low', inLow, guardInLen);
  requireOutput('AO', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlapsPartly(outReal, inHighF64)) {
    throw sharedMemory('AO', 'outReal', 'prices.high');
  }
  if (overlapsPartly(outReal, inLowF64)) {
    throw sharedMemory('AO', 'outReal', 'prices.low');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = aoImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInFastPeriod, optInSlowPeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('AO', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link ao}, {@link aoLookback}, {@link aoDisplayShift},
 * {@link aoOpen} and {@link aoOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface AoOptions {
  /**
   * Number of bars in the short moving average (default 5; range 2 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  fastPeriod?: number | undefined;
  /**
   * Number of bars in the long moving average (default 34; range 2 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  slowPeriod?: number | undefined;
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

/** What {@link ao} returns, and {@link aoOpenAndFill} with its stream. */
export interface AoResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Spread between the two moving averages, centred on zero. `values[k]` is the
   * value at input bar `begIdx + k`.
   */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link ao} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AoOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function aoLookback(options?: AoOptions): number {
  const o = optionsOf('AO lookback', options);
  return aoLookbackPositional(orDefault(o.fastPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowPeriod, Core.INTEGER_DEFAULT), coreOf('AO lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link ao}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AoOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function aoDisplayShift(outputIdx: number, options?: AoOptions): number {
  const o = optionsOf('AO displayShift', options);
  return aoDisplayShiftPositional(orDefault(o.fastPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowPeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('AO displayShift', o.core));
}

/**
 * Bill Williams' Awesome Oscillator (*New Trading Dimensions*, 1998): market
 * momentum read as the spread between a short and a long simple moving average of
 * the median price. It contrasts what the recent bars have done against a longer
 * stretch of the same market, using the bar midpoint rather than the close so that
 * intrabar range, not the settle, drives the reading. Above zero the short window
 * sits higher than the long one and momentum is with the bulls; below zero it is
 * with the bears. It is drawn as a zero-centred histogram, and the readings that
 * get traded are the zero-line crossings, the twin-peaks divergence, and the run
 * of consecutive same-side bars — which is why the sign and the bar-to-bar change
 * matter more than the level. The oscillator is the first leg of Williams'
 * Profitunity system, alongside the Alligator and the Accelerator/Decelerator
 * ([`AC`](https://ta-lib.org/functions/ac)).
 *
 * Formula and more info at
 * [ta-lib.org/functions/ao](https://ta-lib.org/functions/ao).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link aoLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `high`: High
 *   price of each bar. `low`: Low price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link AoOptions}.
 * @returns `begIdx` and the values; see {@link AoResult}. Each output is a view of
 *   its buffer, the caller's or a new one.
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
 * import { ao } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = ao({ high, low }, { fastPeriod: 5, slowPeriod: 34 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`apo`](https://ta-lib.org/functions/apo)
 * @see [`macd`](https://ta-lib.org/functions/macd)
 * @see [`medprice`](https://ta-lib.org/functions/medprice)
 * @see [`ppo`](https://ta-lib.org/functions/ppo)
 * @see [`ultosc`](https://ta-lib.org/functions/ultosc)
 */
export function ao(prices: { high: ArrayLike<number>; low: ArrayLike<number> }, options?: AoOptions): AoResult {
  requireArgument('AO', 'prices', prices);
  const o = optionsOf('AO', options);
  if (o.endIdx === undefined) {
    requireSeries('AO', 'prices.high', prices.high);
    requireSeries('AO', 'prices.low', prices.low);
  }
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('AO', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, aoLookbackPositional(optInFastPeriod, optInSlowPeriod, core), prices.high, prices.low);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = aoPositional(startIdx, endIdx, prices.high, prices.low, optInFastPeriod, optInSlowPeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live AO stream: one value per closed bar, bit-identical to {@link ao} over the
 * same series. Open it with {@link aoOpen} or {@link aoOpenAndFill}; there is
 * nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ao](https://ta-lib.org/functions/ao).
 */
export class AoStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInFastPeriod: number = 0;
  /** @internal */
  optInSlowPeriod: number = 0;
  /** @internal */
  sumFast: number = 0;
  /** @internal */
  sumSlow: number = 0;
  /** @internal */
  ringPos_trailingFastIdx: number = 0;
  /** @internal */
  ringCap_trailingFastIdx: number = 0;
  /** @internal */
  ring_trailingFastIdx_derived: Float64Array = new Float64Array(1);
  /** @internal */
  ringPos_trailingSlowIdx: number = 0;
  /** @internal */
  ringCap_trailingSlowIdx: number = 0;
  /** @internal */
  ring_trailingSlowIdx_derived: Float64Array = new Float64Array(1);
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
  static of(core: Core): AoStream {
    return new AoStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link ao} reports them over the same bars. Each accepted `update` and each
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
      throw failure('AO advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('AO update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('AO update', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    aoStepImpl(this, inHigh, inLow);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number): number {
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('AO peek', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    const sp = this;
    let medianPrice = 0;
    let tempReal = 0;
    let cur_outReal = 0;
    let sumFast = sp.sumFast;
    let sumSlow = sp.sumSlow;
    medianPrice = (inHigh + inLow) / 2.0;
    sumFast += medianPrice;
    sumSlow += medianPrice;
    /* Snapshot the oscillator before either total drops its trailing bar,
     * mirroring the add-new / snapshot / subtract-old order of TA_SMA.
     */
    tempReal = sumFast / sp.optInFastPeriod - sumSlow / sp.optInSlowPeriod;
    /* Read both trailing bars before writing the output. When startIdx is
     * clamped to the lookback the longer window's trailing index equals
     * outIdx exactly, so a store hoisted above this would read back the
     * value it had just overwritten whenever the caller aliases outReal
     * over inHigh or inLow.
     */
    sumFast -= sp.ring_trailingFastIdx_derived[sp.ringPos_trailingFastIdx];
    sumSlow -= sp.ring_trailingSlowIdx_derived[sp.ringPos_trailingSlowIdx];
    cur_outReal = tempReal;
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
  clone(): AoStream {
    const c = new AoStream(this.core);
    c.optInFastPeriod = this.optInFastPeriod;
    c.optInSlowPeriod = this.optInSlowPeriod;
    c.sumFast = this.sumFast;
    c.sumSlow = this.sumSlow;
    c.ringPos_trailingFastIdx = this.ringPos_trailingFastIdx;
    c.ringCap_trailingFastIdx = this.ringCap_trailingFastIdx;
    c.ring_trailingFastIdx_derived = this.ring_trailingFastIdx_derived.slice();
    c.ringPos_trailingSlowIdx = this.ringPos_trailingSlowIdx;
    c.ringCap_trailingSlowIdx = this.ringCap_trailingSlowIdx;
    c.ring_trailingSlowIdx_derived = this.ring_trailingSlowIdx_derived.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function aoStepImpl(sp: AoStream, inHigh: number, inLow: number): void {
  let medianPrice = 0;
  let tempReal = 0;
  let ringCapL_trailingFastIdx = 0;
  let ringCapL_trailingSlowIdx = 0;
  medianPrice = (inHigh + inLow) / 2.0;
  sp.sumFast += medianPrice;
  sp.sumSlow += medianPrice;
  /* Snapshot the oscillator before either total drops its trailing bar,
   * mirroring the add-new / snapshot / subtract-old order of TA_SMA.
   */
  tempReal = sp.sumFast / sp.optInFastPeriod - sp.sumSlow / sp.optInSlowPeriod;
  /* Read both trailing bars before writing the output. When startIdx is
   * clamped to the lookback the longer window's trailing index equals
   * outIdx exactly, so a store hoisted above this would read back the
   * value it had just overwritten whenever the caller aliases outReal
   * over inHigh or inLow.
   */
  sp.sumFast -= sp.ring_trailingFastIdx_derived[sp.ringPos_trailingFastIdx];
  sp.sumSlow -= sp.ring_trailingSlowIdx_derived[sp.ringPos_trailingSlowIdx];
  sp.cur_outReal = tempReal;
  ringCapL_trailingFastIdx = sp.ringCap_trailingFastIdx;
  sp.ring_trailingFastIdx_derived[sp.ringPos_trailingFastIdx] = (inHigh + inLow) / 2.0;
  sp.ringPos_trailingFastIdx = sp.ringPos_trailingFastIdx + 1;
  if (sp.ringPos_trailingFastIdx >= ringCapL_trailingFastIdx) {
    sp.ringPos_trailingFastIdx = 0;
  }
  ringCapL_trailingSlowIdx = sp.ringCap_trailingSlowIdx;
  sp.ring_trailingSlowIdx_derived[sp.ringPos_trailingSlowIdx] = (inHigh + inLow) / 2.0;
  sp.ringPos_trailingSlowIdx = sp.ringPos_trailingSlowIdx + 1;
  if (sp.ringPos_trailingSlowIdx >= ringCapL_trailingSlowIdx) {
    sp.ringPos_trailingSlowIdx = 0;
  }
}

function aoOpenImpl(
  sp: AoStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let sumFast = 0;
  let sumSlow = 0;
  let medianPrice = 0;
  let tempReal = 0;
  let i = 0;
  let outIdx = 0;
  let trailingFastIdx = 0;
  let trailingSlowIdx = 0;
  let lookbackTotal = 0;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInFastPeriod === Core.INTEGER_DEFAULT) {
    optInFastPeriod = 5;
  } else if (optInFastPeriod < 2 || optInFastPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowPeriod === Core.INTEGER_DEFAULT) {
    optInSlowPeriod = 34;
  } else if (optInSlowPeriod < 2 || optInSlowPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Bill Williams' Awesome Oscillator (New Trading Dimensions, 1998): the
   * spread between a short and a long simple moving average of the median
   * price, drawn as a zero-centred histogram.
   *
   *    median_t = (high_t + low_t) / 2
   *    AO_t     = SMA(median, fast)_t - SMA(median, slow)_t
   *
   * Both legs are plain SMAs, so there is no seeding convention to get wrong
   * and no cross-library divergence of the kind an EMA brings.
   *
   * This is two copies of ta_codegen/input/sma/sma.c walking one derived
   * series, sharing a single pass: the same running-sum order, the same
   * snapshot of each total before the trailing bar leaves it, and the same
   * divide by the period. Keeping the arithmetic identical is what makes the
   * composed reference in test_composite.c -- TA_MEDPRICE, two TA_SMA calls
   * and a TA_SUB -- bit-exact rather than merely close, so any future drift
   * in either path is a hard failure instead of a tolerance argument.
   *
   * In particular each total is DIVIDED by its period; it is not multiplied
   * by a precomputed 1/period. Tulip's ao.c multiplies by per5/per34, which
   * costs it up to one ULP against TA_SMA. Dividing buys the memcmp
   * differential, which is the stronger of the two gates (#117, #118).
   *
   * An inverted pair (fast > slow) is NOT swapped, unlike MACD and APO. The
   * lookback is well defined either way and the result is simply -AO, so the
   * swap would buy nothing and would have to be duplicated in ao_lookback.
   */
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = aoLookbackPositional(optInFastPeriod, optInSlowPeriod, core);
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
  sumFast = 0.0;
  sumSlow = 0.0;
  trailingFastIdx = startIdx - (optInFastPeriod - 1);
  trailingSlowIdx = startIdx - (optInSlowPeriod - 1);
  /* Add-up both initial periods, except for the last value.
   *
   * One pass over the longer warm-up window replaces two overlapping passes.
   * A bar inside the shorter window is added to that total as it is reached,
   * so each total still accumulates exactly the same bars in exactly the same
   * ascending order two separate loops would have given it -- which is what
   * keeps each leg bit-identical to the TA_SMA called with this same startIdx.
   */
  i = startIdx - lookbackTotal;
  while (i < startIdx) {
    medianPrice = (inHigh[i] + inLow[i]) / 2.0;
    if (i >= trailingFastIdx) {
      sumFast += medianPrice;
    }
    if (i >= trailingSlowIdx) {
      sumSlow += medianPrice;
    }
    i = i + 1;
  }
  /* Proceed with the calculation for the requested range.
   * Note that this algorithm allows outReal to be the same
   * buffer as either input.
   */
  outIdx = 0;
  while (i <= endIdx) {
    medianPrice = (inHigh[i] + inLow[i]) / 2.0;
    sumFast += medianPrice;
    sumSlow += medianPrice;
    i = i + 1;
    /* Snapshot the oscillator before either total drops its trailing bar,
     * mirroring the add-new / snapshot / subtract-old order of TA_SMA.
     */
    tempReal = sumFast / optInFastPeriod - sumSlow / optInSlowPeriod;
    /* Read both trailing bars before writing the output. When startIdx is
     * clamped to the lookback the longer window's trailing index equals
     * outIdx exactly, so a store hoisted above this would read back the
     * value it had just overwritten whenever the caller aliases outReal
     * over inHigh or inLow.
     */
    sumFast -= (inHigh[trailingFastIdx] + inLow[trailingFastIdx]) / 2.0;
    sumSlow -= (inHigh[trailingSlowIdx] + inLow[trailingSlowIdx]) / 2.0;
    trailingFastIdx = trailingFastIdx + 1;
    trailingSlowIdx = trailingSlowIdx + 1;
    outReal[((outIdx * outStride) | 0)] = tempReal;
    outIdx = outIdx + 1;
  }
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const cap_trailingFastIdx = i - trailingFastIdx;
  if (cap_trailingFastIdx < 1 || cap_trailingFastIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_trailingFastIdx = cap_trailingFastIdx > 0 ? cap_trailingFastIdx : 1;
  const capRing_trailingFastIdx_derived = new Float64Array(allocN_trailingFastIdx);
  for (let fillJ = historyLen - cap_trailingFastIdx; fillJ < historyLen; fillJ++) {
    capRing_trailingFastIdx_derived[fillJ - (historyLen - cap_trailingFastIdx)] = (inHigh[fillJ] + inLow[fillJ]) / 2.0;
  }
  const cap_trailingSlowIdx = i - trailingSlowIdx;
  if (cap_trailingSlowIdx < 1 || cap_trailingSlowIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_trailingSlowIdx = cap_trailingSlowIdx > 0 ? cap_trailingSlowIdx : 1;
  const capRing_trailingSlowIdx_derived = new Float64Array(allocN_trailingSlowIdx);
  for (let fillJ = historyLen - cap_trailingSlowIdx; fillJ < historyLen; fillJ++) {
    capRing_trailingSlowIdx_derived[fillJ - (historyLen - cap_trailingSlowIdx)] = (inHigh[fillJ] + inLow[fillJ]) / 2.0;
  }
  sp.optInFastPeriod = optInFastPeriod;
  sp.optInSlowPeriod = optInSlowPeriod;
  sp.sumFast = sumFast;
  sp.sumSlow = sumSlow;
  sp.ringPos_trailingFastIdx = 0;
  sp.ringCap_trailingFastIdx = cap_trailingFastIdx;
  sp.ring_trailingFastIdx_derived = capRing_trailingFastIdx_derived;
  sp.ringPos_trailingSlowIdx = 0;
  sp.ringCap_trailingSlowIdx = cap_trailingSlowIdx;
  sp.ring_trailingSlowIdx_derived = capRing_trailingSlowIdx_derived;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function aoOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): AoStream {
  const sp = AoStream.of(core);
  const retCode = aoOpenImpl(sp, inHigh, inLow, startIdx, optInFastPeriod, optInSlowPeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('AO openAndFill', inHigh.length, startIdx, aoLookbackPositional(optInFastPeriod, optInSlowPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && aoLookbackPositional(optInFastPeriod, optInSlowPeriod, core) < 0) {
    throw rejectedParams('AO openAndFill', PARAM_RANGES, optInFastPeriod, optInSlowPeriod);
  }
  throw streamFailure('AO openAndFill', retCode);
}

/** @internal */
export function aoOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
): AoStream {
  const sp = AoStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = aoOpenImpl(sp, inHigh, inLow, startIdx, optInFastPeriod, optInSlowPeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('AO open', inHigh.length, startIdx, aoLookbackPositional(optInFastPeriod, optInSlowPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && aoLookbackPositional(optInFastPeriod, optInSlowPeriod, core) < 0) {
    throw rejectedParams('AO open', PARAM_RANGES, optInFastPeriod, optInSlowPeriod);
  }
  throw streamFailure('AO open', retCode);
}

/** @internal */
export function aoOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  core: Core = Core.DEFAULT,
): AoStream {
  optInFastPeriod = requireInt32('AO open', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('AO open', 'slowPeriod', optInSlowPeriod);
  requireArgument('AO open', 'prices.high', inHigh);
  requireHistory('AO open', inHigh.length);
  requireArgument('AO open', 'prices.low', inLow);
  requireHistoryLength('AO open', 'prices.low', inLow.length, inHigh.length);
  return aoOpenInternal(core, float64(inHigh), float64(inLow), 0, optInFastPeriod, optInSlowPeriod);
}

/** @internal */
export function aoOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): AoStream {
  optInFastPeriod = requireInt32('AO openAndFill', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('AO openAndFill', 'slowPeriod', optInSlowPeriod);
  requireArgument('AO openAndFill', 'prices.high', inHigh);
  requireHistory('AO openAndFill', inHigh.length);
  requireArgument('AO openAndFill', 'prices.low', inLow);
  const guardLookback = aoLookbackPositional(optInFastPeriod, optInSlowPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('AO openAndFill', PARAM_RANGES, optInFastPeriod, optInSlowPeriod);
  }
  const guardOutLen = openFillCount('AO openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('AO openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireOutput('AO openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('AO openAndFill', 'outReal', 'prices.high');
  }
  if (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('AO openAndFill', 'outReal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outReal, inHighF64)) {
    throw sharedMemory('AO openAndFill', 'outReal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outReal, inLowF64)) {
    throw sharedMemory('AO openAndFill', 'outReal', 'prices.low');
  }
  return aoOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInFastPeriod, optInSlowPeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live AO stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link ao} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AoOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `aoLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or
 *   histories of different lengths.
 *
 * @example
 * ```ts
 * import { aoOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = aoOpen({ high, low }, { fastPeriod: 5, slowPeriod: 34 });
 * const peeked = stream.peek(101.4, 99.1); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1); // the bar closes
 * ```
 */
export function aoOpen(prices: { high: ArrayLike<number>; low: ArrayLike<number> }, options?: AoOptions): AoStream {
  requireArgument('AO open', 'prices', prices);
  const o = optionsOf('AO open', options);
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('AO open', o.core);
  return aoOpenPositional(prices.high, prices.low, optInFastPeriod, optInSlowPeriod, core);
}

/**
 * {@link aoOpen} that also computes the outputs over the whole history in the same
 * pass, bit-identically to {@link ao}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link AoOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link ao} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `aoLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { aoOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = aoOpenAndFill({ high, low }, {
 *   fastPeriod: 5,
 *   slowPeriod: 34,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function aoOpenAndFill(prices: { high: ArrayLike<number>; low: ArrayLike<number> }, options?: AoOptions): AoResult & { stream: AoStream } {
  requireArgument('AO openAndFill', 'prices', prices);
  const o = optionsOf('AO openAndFill', options);
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('AO openAndFill', o.core);
  const capacity = fillCapacity(aoLookbackPositional(optInFastPeriod, optInSlowPeriod, core), prices.high, prices.low);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = aoOpenAndFillPositional(prices.high, prices.low, optInFastPeriod, optInSlowPeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
