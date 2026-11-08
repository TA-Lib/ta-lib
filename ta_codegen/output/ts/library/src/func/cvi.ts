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
 *  090426 MF,CC  First version (issue #358).
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { emaLookbackPositional } from './ema.ts';
import { rocpLookbackPositional } from './rocp.ts';

/** @internal */
export function cviLookbackPositional(optInTimePeriod: number, optInROCPeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if ((optInROCPeriod | 0) !== optInROCPeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 10;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  if (optInROCPeriod === Core.INTEGER_DEFAULT) {
    optInROCPeriod = 10;
  } else if (optInROCPeriod < 1 || optInROCPeriod > 100000) {
    return -1;
  }
  return emaLookbackPositional(optInTimePeriod, core) + rocpLookbackPositional(optInROCPeriod, core);
}

/** @internal */
export function cviDisplayShiftPositional(optInTimePeriod: number, optInROCPeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (cviLookbackPositional(optInTimePeriod, optInROCPeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cviImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  optInTimePeriod: number,
  optInROCPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let prevEMA = 0;
  let laggedEMA = 0;
  let tempReal = 0;
  let optInK_1 = 0;
  let emaBeta = 0;
  let i = 0;
  let today = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let emaRing!: Float64Array;
  let emaRing_Idx = 0;
  let maxIdx_emaRing = (32) - 1;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 10;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInROCPeriod === Core.INTEGER_DEFAULT) {
    optInROCPeriod = 10;
  } else if (optInROCPeriod < 1 || optInROCPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  /* CVI[t] = 100 * (E[t] - E[t-optInROCPeriod]) / E[t-optInROCPeriod], with E
   * an EMA of the high-low spread. The spread is never materialised and the
   * EMA is anchored optInROCPeriod bars behind startIdx.
   *
   * The arithmetic below is TA_EMA's and TA_ROCP's verbatim -- seed sum
   * accumulated from 0.0 in ascending bar order, k*x + beta*prev, and
   * 100*((a-b)/b) under an exact zero test. That is what makes this fused pass
   * bit-identical to composing TA_SUB, TA_EMA and TA_ROCP, which test_cvi.c
   * holds it to memcmp-exact; reshaping any of it breaks that silently. The
   * guard stays an exact `!= 0.0` and never TA_IS_ZERO -- an epsilon band
   * carries the quote unit and would zero the indicator for anything priced
   * under it (issue #253).
   */
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = emaLookbackPositional(optInTimePeriod, core) + rocpLookbackPositional(optInROCPeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  if (optInROCPeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  emaRing = new Float64Array(optInROCPeriod);
  maxIdx_emaRing = optInROCPeriod - 1;
  emaRing_Idx = 0;
  emaBeta = (optInTimePeriod - 1) / (optInTimePeriod + 1);
  optInK_1 = 1.0 - emaBeta;
  if (emaBeta < 0.5) {
    emaBeta = 1.0 - optInK_1;
  }
  today = startIdx - lookbackTotal;
  i = optInTimePeriod;
  tempReal = 0.0;
  while (i-- > 0) {
    tempReal += inHigh[today] - inLow[today];
    today += 1;
  }
  prevEMA = tempReal / optInTimePeriod;
  /* The ring keeps only the newest optInROCPeriod values, so pushing every EMA
   * value from the seed bar on leaves exactly the lagged terms the output
   * loop reads.
   */
  emaRing[emaRing_Idx] = prevEMA;
  emaRing_Idx++;
  if (emaRing_Idx > maxIdx_emaRing) {
    emaRing_Idx = 0;
  }
  while (today < startIdx) {
    tempReal = inHigh[today] - inLow[today];
    prevEMA = fma(emaBeta, prevEMA, optInK_1 * tempReal);
    today += 1;
    emaRing[emaRing_Idx] = prevEMA;
    emaRing_Idx++;
    if (emaRing_Idx > maxIdx_emaRing) {
      emaRing_Idx = 0;
    }
  }
  /* Read the expiring slot before overwriting it: that is what makes the lag
   * exactly optInROCPeriod rather than one less.
   */
  outIdx = 0;
  while (today <= endIdx) {
    tempReal = inHigh[today] - inLow[today];
    prevEMA = fma(emaBeta, prevEMA, optInK_1 * tempReal);
    today += 1;
    laggedEMA = emaRing[emaRing_Idx];
    emaRing[emaRing_Idx] = prevEMA;
    emaRing_Idx++;
    if (emaRing_Idx > maxIdx_emaRing) {
      emaRing_Idx = 0;
    }
    if (laggedEMA !== 0.0) {
      outReal[outIdx++] = 100.0 * ((prevEMA - laggedEMA) / laggedEMA);
    } else {
      outReal[outIdx++] = 0.0;
    }
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['rocPeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000']] as const;

/** @internal */
export function cviPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  optInROCPeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CVI', 'startIdx', startIdx);
  endIdx = requireInt32('CVI', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('CVI', 'timePeriod', optInTimePeriod);
  optInROCPeriod = requireInt32('CVI', 'rocPeriod', optInROCPeriod);
  requireIndexRange('CVI', startIdx, endIdx);
  const guardLookback = cviLookbackPositional(optInTimePeriod, optInROCPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('CVI', PARAM_RANGES, optInTimePeriod, optInROCPeriod);
  }
  const guardStart = clampedStart('CVI', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CVI', 'prices.high', inHigh, guardInLen);
  requireLength('CVI', 'prices.low', inLow, guardInLen);
  requireOutput('CVI', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlapsPartly(outReal, inHighF64)) {
    throw sharedMemory('CVI', 'outReal', 'prices.high');
  }
  if (overlapsPartly(outReal, inLowF64)) {
    throw sharedMemory('CVI', 'outReal', 'prices.low');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cviImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInTimePeriod, optInROCPeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CVI', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cvi}, {@link cviLookback}, {@link cviDisplayShift},
 * {@link cviOpen} and {@link cviOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface CviOptions {
  /**
   * Number of bars in the exponential average of the high-low spread (default 10;
   * range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  timePeriod?: number | undefined;
  /**
   * How many bars back the percent change reaches (default 10; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  rocPeriod?: number | undefined;
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

/** What {@link cvi} returns, and {@link cviOpenAndFill} with its stream. */
export interface CviResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Percent change of the smoothed high-low spread. `values[k]` is the value at
   * input bar `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link cvi} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CviOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cviLookback(options?: CviOptions): number {
  const o = optionsOf('CVI lookback', options);
  return cviLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.rocPeriod, Core.INTEGER_DEFAULT), coreOf('CVI lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cvi}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CviOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cviDisplayShift(outputIdx: number, options?: CviOptions): number {
  const o = optionsOf('CVI displayShift', options);
  return cviDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.rocPeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('CVI displayShift', o.core));
}

/**
 * Chaikin's Volatility: Marc Chaikin's reading of how fast a market's daily
 * trading range is widening or narrowing. The high-low spread is smoothed by an
 * exponential moving average, and the indicator reports the percent that average
 * has changed over a lookback of its own. Read it as a rate of expansion. Positive
 * means the smoothed range is wider than it was; negative means it has contracted.
 * Chaikin's own interpretation is contrarian on the fast side: a range that widens
 * sharply over a short span is typical of the panic near a market bottom, while a
 * range that narrows steadily over a long span is typical of a market topping out.
 * It measures range, not direction, so it says nothing about which way price is
 * heading.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cvi](https://ta-lib.org/functions/cvi).
 *
 * **Notes**
 * - The averaging length and the rate-of-change length are independent, as in
 *   Achelis's relay of the author ("an exponential moving average of the
 *   difference between the daily high and low prices ... then the percent that
 *   this moving average has changed over a specified time period") and in the
 *   MathWorks `chaikvolat` signature. Implementations that expose a single length
 *   are the special case where both are set to the same value.
 * - Some vendors default the rate-of-change length to 12 rather than to Achelis's
 *   recommendation, which is the same for both lengths.
 * - A window whose bars are all exactly flat, high equal to low, leaves the lagged
 *   average at zero. CVI reports 0 there. Tulip Indicators and pandas-ta-classic
 *   leave the division unguarded and emit NaN; trading-signals returns 0, as here.
 * - Implementations disagree on how the inner EMA is seeded. TA-Lib uses its own
 *   EMA convention, the simple average of the first `timePeriod` spreads, where
 *   Tulip Indicators and trading-signals seed from a single raw spread and
 *   converge to these values only after many bars.
 * - CVI inherits EMA's unstable period rather than owning one:
 *   `CoreBuilder.unstablePeriod(FuncUnstId.EMA, ...)` moves CVI's first output
 *   too.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cviLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `high`: High price.
 *   `low`: Low price.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link CviOptions}.
 * @returns `begIdx` and the values; see {@link CviResult}. Each output is a view
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
 * import { cvi } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = cvi({ high, low }, {
 *   timePeriod: 10,
 *   rocPeriod: 10,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`atr`](https://ta-lib.org/functions/atr)
 * @see [`natr`](https://ta-lib.org/functions/natr)
 * @see [`trange`](https://ta-lib.org/functions/trange)
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`rocp`](https://ta-lib.org/functions/rocp)
 */
export function cvi(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CviOptions): CviResult {
  requireArgument('CVI', 'prices', prices);
  const o = optionsOf('CVI', options);
  if (o.endIdx === undefined) {
    requireSeries('CVI', 'prices.high', prices.high);
    requireSeries('CVI', 'prices.low', prices.low);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInROCPeriod = orDefault(o.rocPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('CVI', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cviLookbackPositional(optInTimePeriod, optInROCPeriod, core), prices.high, prices.low);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = cviPositional(startIdx, endIdx, prices.high, prices.low, optInTimePeriod, optInROCPeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CVI stream: one value per closed bar, bit-identical to {@link cvi} over
 * the same series. Open it with {@link cviOpen} or {@link cviOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cvi](https://ta-lib.org/functions/cvi).
 */
export class CviStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  optInROCPeriod: number = 0;
  /** @internal */
  prevEMA: number = 0;
  /** @internal */
  optInK_1: number = 0;
  /** @internal */
  emaBeta: number = 0;
  /** @internal */
  emaRing_Idx: number = 0;
  /** @internal */
  maxIdx_emaRing: number = 0;
  /** @internal */
  cbSize_emaRing: number = 0;
  /** @internal */
  cb_emaRing: Float64Array = new Float64Array(1);
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
  static of(core: Core): CviStream {
    return new CviStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cvi} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('CVI advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param high - The bar's `high` value: High price.
   * @param low - The bar's `low` value: Low price.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(high: number, low: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('CVI update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(high) || !Number.isFinite(low)) {
      throw nonFiniteBar('CVI update', !Number.isFinite(high) ? 'high' : 'low');
    }
    cviStepImpl(this, high, low);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param high - The bar's `high` value: High price.
   * @param low - The bar's `low` value: Low price.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(high: number, low: number): number {
    if (!Number.isFinite(high) || !Number.isFinite(low)) {
      throw nonFiniteBar('CVI peek', !Number.isFinite(high) ? 'high' : 'low');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
    let laggedEMA = 0;
    let tempReal = 0;
    let cur_outReal = 0;
    let emaRing_Idx = sp.emaRing_Idx;
    let prevEMA = sp.prevEMA;
    tempReal = inHigh - inLow;
    prevEMA = fma(sp.emaBeta, prevEMA, sp.optInK_1 * tempReal);
    laggedEMA = sp.cb_emaRing[emaRing_Idx];
    emaRing_Idx = emaRing_Idx + 1;
    if (emaRing_Idx > sp.maxIdx_emaRing) {
      emaRing_Idx = 0;
    }
    if (laggedEMA !== 0.0) {
      cur_outReal = 100.0 * ((prevEMA - laggedEMA) / laggedEMA);
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
  clone(): CviStream {
    const c = new CviStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.optInROCPeriod = this.optInROCPeriod;
    c.prevEMA = this.prevEMA;
    c.optInK_1 = this.optInK_1;
    c.emaBeta = this.emaBeta;
    c.emaRing_Idx = this.emaRing_Idx;
    c.maxIdx_emaRing = this.maxIdx_emaRing;
    c.cbSize_emaRing = this.cbSize_emaRing;
    c.cb_emaRing = this.cb_emaRing.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cviStepImpl(sp: CviStream, inHigh: number, inLow: number): void {
  let laggedEMA = 0;
  let tempReal = 0;
  tempReal = inHigh - inLow;
  sp.prevEMA = fma(sp.emaBeta, sp.prevEMA, sp.optInK_1 * tempReal);
  laggedEMA = sp.cb_emaRing[sp.emaRing_Idx];
  sp.cb_emaRing[sp.emaRing_Idx] = sp.prevEMA;
  sp.emaRing_Idx = sp.emaRing_Idx + 1;
  if (sp.emaRing_Idx > sp.maxIdx_emaRing) {
    sp.emaRing_Idx = 0;
  }
  if (laggedEMA !== 0.0) {
    sp.cur_outReal = 100.0 * ((sp.prevEMA - laggedEMA) / laggedEMA);
  } else {
    sp.cur_outReal = 0.0;
  }
}

function cviOpenImpl(
  sp: CviStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInROCPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let prevEMA = 0;
  let laggedEMA = 0;
  let tempReal = 0;
  let optInK_1 = 0;
  let emaBeta = 0;
  let i = 0;
  let today = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let emaRing!: Float64Array;
  let emaRing_Idx = 0;
  let maxIdx_emaRing = (32) - 1;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 10;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInROCPeriod === Core.INTEGER_DEFAULT) {
    optInROCPeriod = 10;
  } else if (optInROCPeriod < 1 || optInROCPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* CVI[t] = 100 * (E[t] - E[t-optInROCPeriod]) / E[t-optInROCPeriod], with E
   * an EMA of the high-low spread. The spread is never materialised and the
   * EMA is anchored optInROCPeriod bars behind startIdx.
   *
   * The arithmetic below is TA_EMA's and TA_ROCP's verbatim -- seed sum
   * accumulated from 0.0 in ascending bar order, k*x + beta*prev, and
   * 100*((a-b)/b) under an exact zero test. That is what makes this fused pass
   * bit-identical to composing TA_SUB, TA_EMA and TA_ROCP, which test_cvi.c
   * holds it to memcmp-exact; reshaping any of it breaks that silently. The
   * guard stays an exact `!= 0.0` and never TA_IS_ZERO -- an epsilon band
   * carries the quote unit and would zero the indicator for anything priced
   * under it (issue #253).
   */
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = emaLookbackPositional(optInTimePeriod, core) + rocpLookbackPositional(optInROCPeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (optInROCPeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  emaRing = new Float64Array(optInROCPeriod);
  maxIdx_emaRing = optInROCPeriod - 1;
  emaRing_Idx = 0;
  emaBeta = (optInTimePeriod - 1) / (optInTimePeriod + 1);
  optInK_1 = 1.0 - emaBeta;
  if (emaBeta < 0.5) {
    emaBeta = 1.0 - optInK_1;
  }
  today = startIdx - lookbackTotal;
  i = optInTimePeriod;
  tempReal = 0.0;
  while (i-- > 0) {
    tempReal += inHigh[today] - inLow[today];
    today += 1;
  }
  prevEMA = tempReal / optInTimePeriod;
  /* The ring keeps only the newest optInROCPeriod values, so pushing every EMA
   * value from the seed bar on leaves exactly the lagged terms the output
   * loop reads.
   */
  emaRing[emaRing_Idx] = prevEMA;
  emaRing_Idx++;
  if (emaRing_Idx > maxIdx_emaRing) {
    emaRing_Idx = 0;
  }
  while (today < startIdx) {
    tempReal = inHigh[today] - inLow[today];
    prevEMA = fma(emaBeta, prevEMA, optInK_1 * tempReal);
    today += 1;
    emaRing[emaRing_Idx] = prevEMA;
    emaRing_Idx++;
    if (emaRing_Idx > maxIdx_emaRing) {
      emaRing_Idx = 0;
    }
  }
  /* Read the expiring slot before overwriting it: that is what makes the lag
   * exactly optInROCPeriod rather than one less.
   */
  outIdx = 0;
  while (today <= endIdx) {
    tempReal = inHigh[today] - inLow[today];
    prevEMA = fma(emaBeta, prevEMA, optInK_1 * tempReal);
    today += 1;
    laggedEMA = emaRing[emaRing_Idx];
    emaRing[emaRing_Idx] = prevEMA;
    emaRing_Idx++;
    if (emaRing_Idx > maxIdx_emaRing) {
      emaRing_Idx = 0;
    }
    if (laggedEMA !== 0.0) {
      outReal[((outIdx++ * outStride) | 0)] = 100.0 * ((prevEMA - laggedEMA) / laggedEMA);
    } else {
      outReal[((outIdx++ * outStride) | 0)] = 0.0;
    }
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  const capCb_emaRing = maxIdx_emaRing + 1;
  if (capCb_emaRing > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.optInROCPeriod = optInROCPeriod;
  sp.prevEMA = prevEMA;
  sp.optInK_1 = optInK_1;
  sp.emaBeta = emaBeta;
  sp.emaRing_Idx = emaRing_Idx;
  sp.maxIdx_emaRing = maxIdx_emaRing;
  sp.cbSize_emaRing = capCb_emaRing;
  sp.cb_emaRing = emaRing;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cviOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInROCPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): CviStream {
  const sp = CviStream.of(core);
  const retCode = cviOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, optInROCPeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CVI openAndFill', inHigh.length, startIdx, cviLookbackPositional(optInTimePeriod, optInROCPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && cviLookbackPositional(optInTimePeriod, optInROCPeriod, core) < 0) {
    throw rejectedParams('CVI openAndFill', PARAM_RANGES, optInTimePeriod, optInROCPeriod);
  }
  throw streamFailure('CVI openAndFill', retCode);
}

/** @internal */
export function cviOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInROCPeriod: number,
): CviStream {
  const sp = CviStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = cviOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, optInROCPeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CVI open', inHigh.length, startIdx, cviLookbackPositional(optInTimePeriod, optInROCPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && cviLookbackPositional(optInTimePeriod, optInROCPeriod, core) < 0) {
    throw rejectedParams('CVI open', PARAM_RANGES, optInTimePeriod, optInROCPeriod);
  }
  throw streamFailure('CVI open', retCode);
}

/** @internal */
export function cviOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  optInROCPeriod: number,
  core: Core = Core.DEFAULT,
): CviStream {
  optInTimePeriod = requireInt32('CVI open', 'timePeriod', optInTimePeriod);
  optInROCPeriod = requireInt32('CVI open', 'rocPeriod', optInROCPeriod);
  requireArgument('CVI open', 'prices.high', inHigh);
  requireHistory('CVI open', 'prices.high', inHigh.length);
  requireArgument('CVI open', 'prices.low', inLow);
  requireHistoryLength('CVI open', 'prices.low', inLow.length, inHigh.length);
  return cviOpenInternal(core, float64(inHigh), float64(inLow), 0, optInTimePeriod, optInROCPeriod);
}

/** @internal */
export function cviOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  optInROCPeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): CviStream {
  optInTimePeriod = requireInt32('CVI openAndFill', 'timePeriod', optInTimePeriod);
  optInROCPeriod = requireInt32('CVI openAndFill', 'rocPeriod', optInROCPeriod);
  requireArgument('CVI openAndFill', 'prices.high', inHigh);
  requireHistory('CVI openAndFill', 'prices.high', inHigh.length);
  requireArgument('CVI openAndFill', 'prices.low', inLow);
  const guardLookback = cviLookbackPositional(optInTimePeriod, optInROCPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('CVI openAndFill', PARAM_RANGES, optInTimePeriod, optInROCPeriod);
  }
  const guardOutLen = openFillCount('CVI openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('CVI openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireOutput('CVI openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CVI openAndFill', 'outReal', 'prices.high');
  }
  if (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CVI openAndFill', 'outReal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outReal, inHighF64)) {
    throw sharedMemory('CVI openAndFill', 'outReal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outReal, inLowF64)) {
    throw sharedMemory('CVI openAndFill', 'outReal', 'prices.low');
  }
  return cviOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInTimePeriod, optInROCPeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live CVI stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link cvi} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price. `low`: Low price.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CviOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cviLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or
 *   histories of different lengths.
 *
 * @example
 * ```ts
 * import { cviOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = cviOpen({ high, low }, { timePeriod: 10, rocPeriod: 10 });
 * const peeked = stream.peek(101.4, 99.1); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1); // the bar closes
 * ```
 */
export function cviOpen(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CviOptions): CviStream {
  requireArgument('CVI open', 'prices', prices);
  const o = optionsOf('CVI open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInROCPeriod = orDefault(o.rocPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('CVI open', o.core);
  return cviOpenPositional(prices.high, prices.low, optInTimePeriod, optInROCPeriod, core);
}

/**
 * {@link cviOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link cvi}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price. `low`: Low price.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CviOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cvi} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cviLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { cviOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = cviOpenAndFill({ high, low }, {
 *   timePeriod: 10,
 *   rocPeriod: 10,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cviOpenAndFill(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CviOptions): CviResult & { stream: CviStream } {
  requireArgument('CVI openAndFill', 'prices', prices);
  const o = optionsOf('CVI openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInROCPeriod = orDefault(o.rocPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('CVI openAndFill', o.core);
  const capacity = fillCapacity(cviLookbackPositional(optInTimePeriod, optInROCPeriod, core), prices.high, prices.low);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = cviOpenAndFillPositional(prices.high, prices.low, optInTimePeriod, optInROCPeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
