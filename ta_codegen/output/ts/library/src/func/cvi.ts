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

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { emaLookback } from './ema.ts';
import { rocpLookback } from './rocp.ts';

/**
 * Number of leading input bars {@link cvi} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Number of bars in the exponential average of the
 *   high-low spread (default 10; range 2 to 100000; `Core.INTEGER_DEFAULT` selects
 *   the default).
 * @param optInROCPeriod - How many bars back the percent change reaches (default
 *   10; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function cviLookback(optInTimePeriod: number, optInROCPeriod: number, core: Core = Core.DEFAULT): number {
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
  return emaLookback(optInTimePeriod, core) + rocpLookback(optInROCPeriod, core);
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cvi}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Number of bars in the exponential average of the
 *   high-low spread (default 10; range 2 to 100000; `Core.INTEGER_DEFAULT` selects
 *   the default).
 * @param optInROCPeriod - How many bars back the percent change reaches (default
 *   10; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function cviDisplayShift(optInTimePeriod: number, optInROCPeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (cviLookback(optInTimePeriod, optInROCPeriod, core) < 0) {
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
  lookbackTotal = emaLookback(optInTimePeriod, core) + rocpLookback(optInROCPeriod, core);
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
 *   EMA convention, the simple average of the first `optInTimePeriod` spreads,
 *   where Tulip Indicators and trading-signals seed from a single raw spread and
 *   converge to these values only after many bars.
 * - CVI inherits EMA's unstable period rather than owning one:
 *   `TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)` moves CVI's first output too.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link cviLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price.
 * @param inLow - Low price.
 * @param optInTimePeriod - Number of bars in the exponential average of the
 *   high-low spread (default 10; range 2 to 100000; `Core.INTEGER_DEFAULT` selects
 *   the default).
 * @param optInROCPeriod - How many bars back the percent change reaches (default
 *   10; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Percent change of the smoothed high-low spread. Must hold the
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
 * import { cvi } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(high.length);
 * const { begIdx, count } = cvi(0, high.length - 1, high, low, 10, 10, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`atr`](https://ta-lib.org/functions/atr)
 * @see [`natr`](https://ta-lib.org/functions/natr)
 * @see [`trange`](https://ta-lib.org/functions/trange)
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`rocp`](https://ta-lib.org/functions/rocp)
 */
export function cvi(
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
  optInTimePeriod = requireInt32('CVI', 'optInTimePeriod', optInTimePeriod);
  optInROCPeriod = requireInt32('CVI', 'optInROCPeriod', optInROCPeriod);
  requireIndexRange('CVI', startIdx, endIdx);
  const guardStart = clampedStart('CVI', startIdx, cviLookback(optInTimePeriod, optInROCPeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CVI', 'inHigh', inHigh, guardInLen);
  requireLength('CVI', 'inLow', inLow, guardInLen);
  requireLength('CVI', 'outReal', outReal, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlapsPartly(outReal, inHighF64) || overlapsPartly(outReal, inLowF64)) {
    throw failure('CVI', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cviImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInTimePeriod, optInROCPeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CVI', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
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
      throw failure('CVI advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inHigh - The bar's `inHigh` value: High price.
   * @param inLow - The bar's `inLow` value: Low price.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('CVI update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('CVI update', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    cviStepImpl(this, inHigh, inLow);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price.
   * @param inLow - The bar's `inLow` value: Low price.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number): number {
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('CVI peek', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    const sp = this;
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
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
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
  if (startIdx > endIdx) {
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
  lookbackTotal = emaLookback(optInTimePeriod, core) + rocpLookback(optInROCPeriod, core);
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
    throw insufficientHistory('CVI openAndFill', inHigh.length, startIdx, cviLookback(optInTimePeriod, optInROCPeriod, core));
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
    throw insufficientHistory('CVI open', inHigh.length, startIdx, cviLookback(optInTimePeriod, optInROCPeriod, core));
  }
  throw streamFailure('CVI open', retCode);
}

/**
 * Opens a live CVI stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link cvi} there.
 *
 * @param inHigh - Warm-up history. High price. Every history has the same length.
 * @param inLow - Warm-up history. Low price. Every history has the same length.
 * @param optInTimePeriod - Number of bars in the exponential average of the
 *   high-low spread (default 10; range 2 to 100000; `Core.INTEGER_DEFAULT` selects
 *   the default).
 * @param optInROCPeriod - How many bars back the percent change reaches (default
 *   10; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cviLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { cviOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = cviOpen(high, low, 10, 10);
 * const peeked = stream.peek(101.4, 99.1); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1); // the bar closes
 * ```
 */
export function cviOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  optInROCPeriod: number,
  core: Core = Core.DEFAULT,
): CviStream {
  optInTimePeriod = requireInt32('CVI open', 'optInTimePeriod', optInTimePeriod);
  optInROCPeriod = requireInt32('CVI open', 'optInROCPeriod', optInROCPeriod);
  requireArgument('CVI open', 'inHigh', inHigh);
  requireHistory('CVI open', inHigh.length);
  requireArgument('CVI open', 'inLow', inLow);
  requireHistoryLength('CVI open', 'inLow', inLow.length, inHigh.length);
  return cviOpenInternal(core, float64(inHigh), float64(inLow), 0, optInTimePeriod, optInROCPeriod);
}

/**
 * {@link cviOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link cvi}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price. Every history has the same length.
 * @param inLow - Warm-up history. Low price. Every history has the same length.
 * @param optInTimePeriod - Number of bars in the exponential average of the
 *   high-low spread (default 10; range 2 to 100000; `Core.INTEGER_DEFAULT` selects
 *   the default).
 * @param optInROCPeriod - How many bars back the percent change reaches (default
 *   10; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Percent change of the smoothed high-low spread. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cviLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range, histories of different lengths or an output too
 *   short or starting where another output or a `Float64Array` input starts, or, on
 *   a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cviOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(high.length);
 * const stream = cviOpenAndFill(high, low, 10, 10, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function cviOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  optInROCPeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): CviStream {
  optInTimePeriod = requireInt32('CVI openAndFill', 'optInTimePeriod', optInTimePeriod);
  optInROCPeriod = requireInt32('CVI openAndFill', 'optInROCPeriod', optInROCPeriod);
  requireArgument('CVI openAndFill', 'inHigh', inHigh);
  requireHistory('CVI openAndFill', inHigh.length);
  requireArgument('CVI openAndFill', 'inLow', inLow);
  const guardOutLen = openFillCount('CVI openAndFill', inHigh.length, cviLookback(optInTimePeriod, optInROCPeriod, core));
  requireHistoryLength('CVI openAndFill', 'inLow', inLow.length, inHigh.length);
  requireLength('CVI openAndFill', 'outReal', outReal, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if ((outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) || (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset)) {
    throw streamFailure('CVI openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inHighF64) || overlaps(outReal, inLowF64))) {
    throw streamFailure('CVI openAndFill', RetCode.BAD_PARAM);
  }
  return cviOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInTimePeriod, optInROCPeriod, new MInteger(), new MInteger(), outReal);
}
