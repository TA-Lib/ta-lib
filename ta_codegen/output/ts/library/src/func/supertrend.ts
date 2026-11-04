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
 *  090126 MF,CC  First version (issue #272).
 *  090326 MF,CC  #338 Two-coefficient Wilder step, in lockstep with TA_ATR.
 */

import { badSink, clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireReal, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { atrLookback } from './atr.ts';

/**
 * Number of leading input bars {@link supertrend} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Smoothing period of the Average True Range (default 10;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInMultiplier - Multiplier applied to the Average True Range to set the
 *   band width (default 3; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function supertrendLookback(optInTimePeriod: number, optInMultiplier: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (typeof optInMultiplier !== 'number') {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 10;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  if (optInMultiplier === Core.REAL_DEFAULT) {
    optInMultiplier = 3.0;
  } else if (!(optInMultiplier >= 0e0 && optInMultiplier <= Core.REAL_MAX)) {
    return -1;
  }
  /* Every output bar needs the Average True Range at the same bar, and nothing
   * else reaches further back, so the lookback is exactly the callee's. Never
   * restated here, which is what makes SUPERTREND inherit TA_FUNC_UNST_ATR.
   */
  return atrLookback(optInTimePeriod, core);
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link supertrend}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Smoothing period of the Average True Range (default 10;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInMultiplier - Multiplier applied to the Average True Range to set the
 *   band width (default 3; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function supertrendDisplayShift(optInTimePeriod: number, optInMultiplier: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (supertrendLookback(optInTimePeriod, optInMultiplier, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 2) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function supertrendImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  optInTimePeriod: number,
  optInMultiplier: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outSupertrend: Float64Array,
  outTrend: Int32Array,
): RetCode {
  let i = 0;
  let today = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let isUptrend = 0;
  let prevATR = 0;
  let periodTotal = 0;
  let wAlpha = 0;
  let wBeta = 0;
  let val2 = 0;
  let val3 = 0;
  let greatest = 0;
  let tempCY = 0;
  let tempLT = 0;
  let tempHT = 0;
  let medianPrice = 0;
  let band = 0;
  let basicUpper = 0;
  let basicLower = 0;
  let finalUpper = 0;
  let finalLower = 0;
  let closeToday = 0;
  let prevClose = 0;
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
  if (optInMultiplier === Core.REAL_DEFAULT) {
    optInMultiplier = 3.0;
  } else if (!(optInMultiplier >= 0e0 && optInMultiplier <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if ((outSupertrend.buffer === outTrend.buffer && outSupertrend.byteOffset === outTrend.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = supertrendLookback(optInTimePeriod, optInMultiplier, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  /* The Average True Range is carried inline rather than taken from a call,
   * because the band and the ratchet advance together one bar at a time and a
   * whole-range buffer between them would not stream.
   *
   * The arithmetic order below is the bit-exactness contract with TA_ATR (do
   * not reorder): True Range from high-low, then the two previous-close
   * distances in that order; the seed summed from 0.0 over the first 'period'
   * True Ranges and divided once; the same two Wilder coefficients, wBeta
   * rounded and wAlpha derived from it, in one fused statement.
   */
  wBeta = (optInTimePeriod - 1) / optInTimePeriod;
  wAlpha = 1.0 - wBeta;
  today = startIdx - lookbackTotal + 1;
  periodTotal = 0.0;
  i = optInTimePeriod;
  while (i-- > 0) {
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    periodTotal += greatest;
    today += 1;
  }
  prevATR = periodTotal / optInTimePeriod;
  /* Skip the Average True Range's unstable period. Taking the count from the
   * lookback rather than naming the setting keeps the two from disagreeing.
   */
  i = lookbackTotal - optInTimePeriod;
  while (i !== 0) {
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    prevATR = fma(wBeta, prevATR, wAlpha * greatest);
    today += 1;
    i -= 1;
  }
  /* The first bar has no band to ratchet against and no trend to carry, so
   * both bands take their unclamped value and the trend is seeded long, as
   * ta4j's SuperTrendIndicator does. The formula does not settle this and the
   * published implementations are split on it, so the choice is a convention:
   * it stays visible for as long as the first trend lasts, and on a series
   * whose close never leaves the band it never washes out at all.
   */
  medianPrice = (inHigh[startIdx] + inLow[startIdx]) / 2.0;
  band = optInMultiplier * prevATR;
  finalUpper = medianPrice + band;
  finalLower = medianPrice - band;
  isUptrend = 1;
  prevClose = inClose[startIdx];
  outSupertrend[0] = finalLower;
  outTrend[0] = 1;
  outIdx = 1;
  today = startIdx + 1;
  while (today <= endIdx) {
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    prevATR = fma(wBeta, prevATR, wAlpha * greatest);
    medianPrice = (tempHT + tempLT) / 2.0;
    band = optInMultiplier * prevATR;
    basicUpper = medianPrice + band;
    basicLower = medianPrice - band;
    /* Each band ratchets toward price and is released only by a close on its
     * far side. Nothing keeps the two ordered -- each is released by its own
     * condition, so the lower one can end up above the upper one -- and where
     * that happens a flip puts the emitted band on the far side of the close.
     * The line is therefore NOT monotone within a trend, and the flag does not
     * always say which side of the line price is on; both read as invariants
     * and neither is one.
     */
    if (basicUpper < finalUpper || prevClose > finalUpper) {
      finalUpper = basicUpper;
    }
    if (basicLower > finalLower || prevClose < finalLower) {
      finalLower = basicLower;
    }
    closeToday = inClose[today];
    /* The trend is carried in its own variable rather than recovered by
     * comparing the previous output against the previous bands. The two
     * carried bands do coincide on some bars -- always on a bar with no true
     * range, and on some of them at a multiplier of zero -- and there that
     * comparison cannot tell them apart, so it would silently lose the
     * hysteresis on exactly the flat input a corpus of real prices lacks.
     */
    if (isUptrend !== 0) {
      if (closeToday < finalLower) {
        isUptrend = 0;
      }
    } else if (closeToday > finalUpper) {
      isUptrend = 1;
    }
    if (isUptrend !== 0) {
      outSupertrend[outIdx] = finalLower;
      outTrend[outIdx] = 1;
    } else {
      outSupertrend[outIdx] = finalUpper;
      outTrend[outIdx] = -1;
    }
    prevClose = closeToday;
    outIdx += 1;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * An ATR-scaled trailing band that follows price on one side at a time and flips
 * to the other side when the close breaks through it. The trend rides the lower
 * band while it is up and the upper band while it is down, so the line is usually
 * below price in an uptrend and above it in a downtrend, and the flip is the
 * signal. Attributed to Olivier Seban.
 *
 * Formula and more info at
 * [ta-lib.org/functions/supertrend](https://ta-lib.org/functions/supertrend).
 *
 * **Notes**
 * - Both bands are carried forward on every bar, and the trend is decided against
 *   the current bar's band. This is the form Investopedia, TradingView and ta4j
 *   all describe. A second published form, from the AmiBroker script attributed to
 *   Seban, carries only the band the trend is riding and lets the other float
 *   free; the two agree on almost every bar and part company at a flip, where this
 *   form hands back a band it has been carrying all along and that one hands back
 *   a fresh value.
 * - The recurrence has no value before the first bar it can be computed on, so the
 *   trend is seeded up there and both bands take their unclamped value. Published
 *   implementations are split on that seed; this is ta4j's. The choice stays
 *   visible for as long as the first trend lasts, it never washes out on a series
 *   whose close never leaves the band, and it is why the same bar computed from a
 *   later start index can differ.
 * - The direction is reported as +1 for an uptrend and -1 for a downtrend, the
 *   sign every other signed output in this library uses for bullish. TradingView's
 *   built-in `ta.supertrend` returns the opposite signs for the same two states,
 *   and seeds the other way; a strategy ported from Pine has to swap them.
 * - The two carried bands are released by different conditions, so nothing keeps
 *   the lower one below the upper one. Where they cross, a flip leaves the line on
 *   the far side of the close, and within a single trend the line can widen away
 *   from price instead of tightening toward it. Both are ordinary consequences of
 *   the published formula, not a variation on it; they are simply not the
 *   guarantees the indicator is usually described as giving.
 * - A multiplier of zero is degenerate but defined: the two basic bands collapse
 *   onto the median price exactly. The carried bands do not collapse with them —
 *   each is still released by its own condition — so they coincide on some bars
 *   and differ on others, and the trend flips on most bars rather than on every
 *   one.
 * - The band inherits the Average True Range's warm-up, so a caller who wants it
 *   converged sets `TA_FUNC_UNST_ATR`, exactly as when calling that function
 *   directly.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link supertrendLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param optInTimePeriod - Smoothing period of the Average True Range (default 10;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInMultiplier - Multiplier applied to the Average True Range to set the
 *   band width (default 3; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param outSupertrend - The SuperTrend line: the band the trend is currently
 *   riding. Must hold the values produced and never be empty; the input length
 *   always suffices.
 * @param outTrend - Trend direction: +1 while the trend rides the lower band, -1
 *   while it rides the upper one. Must hold the values produced and never be empty;
 *   the input length always suffices.
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
 * import { supertrend } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const supertrendOut = new Float64Array(high.length);
 * const trend = new Int32Array(high.length);
 * const { begIdx, count } = supertrend(
 *   0, high.length - 1,
 *   high, low, close,
 *   10, 3,
 *   supertrendOut, trend,
 * );
 * // supertrendOut[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`atr`](https://ta-lib.org/functions/atr)
 * @see [`medprice`](https://ta-lib.org/functions/medprice)
 * @see [`sar`](https://ta-lib.org/functions/sar)
 * @see [`sarext`](https://ta-lib.org/functions/sarext)
 * @see [`kc`](https://ta-lib.org/functions/kc)
 */
export function supertrend(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  optInMultiplier: number,
  outSupertrend: Float64Array,
  outTrend: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('SUPERTREND', 'startIdx', startIdx);
  endIdx = requireInt32('SUPERTREND', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('SUPERTREND', 'optInTimePeriod', optInTimePeriod);
  optInMultiplier = requireReal('SUPERTREND', 'optInMultiplier', optInMultiplier);
  requireIndexRange('SUPERTREND', startIdx, endIdx);
  const guardStart = clampedStart('SUPERTREND', startIdx, supertrendLookback(optInTimePeriod, optInMultiplier, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('SUPERTREND', 'inHigh', inHigh, guardInLen);
  requireLength('SUPERTREND', 'inLow', inLow, guardInLen);
  requireLength('SUPERTREND', 'inClose', inClose, guardInLen);
  requireLength('SUPERTREND', 'outSupertrend', outSupertrend, guardOutLen);
  requireLength('SUPERTREND', 'outTrend', outTrend, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outSupertrend, inHighF64) || overlapsPartly(outSupertrend, inLowF64) || overlapsPartly(outSupertrend, inCloseF64) || overlaps(outTrend, inHighF64) || overlaps(outTrend, inLowF64) || overlaps(outTrend, inCloseF64) || overlaps(outSupertrend, outTrend)) {
    throw failure('SUPERTREND', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = supertrendImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInTimePeriod, optInMultiplier, outBegIdx, outNBElement, outSupertrend, outTrend);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('SUPERTREND', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live SUPERTREND stream: one value per closed bar, bit-identical to
 * {@link supertrend} over the same series. Open it with {@link supertrendOpen} or
 * {@link supertrendOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/supertrend](https://ta-lib.org/functions/supertrend).
 */
export class SupertrendStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  optInMultiplier: number = 0;
  /** @internal */
  isUptrend: number = 0;
  /** @internal */
  prevATR: number = 0;
  /** @internal */
  wAlpha: number = 0;
  /** @internal */
  wBeta: number = 0;
  /** @internal */
  finalUpper: number = 0;
  /** @internal */
  finalLower: number = 0;
  /** @internal */
  prevClose: number = 0;
  /** @internal */
  lag1_inClose: number = 0;
  /** @internal */
  cur_outSupertrend: number = 0;
  /** @internal */
  cur_outTrend: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): SupertrendStream {
    return new SupertrendStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link supertrend} reports them over the same bars. Each accepted `update` and
   * each `advance` adds one to `count`.
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
      throw failure('SUPERTREND advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar, writing the new values into `out`.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number, inClose: number, out: SupertrendOut): void {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('SUPERTREND update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (typeof out !== 'object' || out === null) {
      throw badSink('SUPERTREND update', 'SupertrendOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('SUPERTREND update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    supertrendStepImpl(this, inHigh, inLow, inClose);
    this.outRangeCount++;
    out.supertrend = this.cur_outSupertrend;
    out.trend = this.cur_outTrend;
  }

  /**
   * Writes into `out` what the next `update` with the same bar would write,
   * committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number, inClose: number, out: SupertrendOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('SUPERTREND peek', 'SupertrendOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('SUPERTREND peek', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let val2 = 0;
    let val3 = 0;
    let greatest = 0;
    let tempCY = 0;
    let tempLT = 0;
    let tempHT = 0;
    let medianPrice = 0;
    let band = 0;
    let basicUpper = 0;
    let basicLower = 0;
    let closeToday = 0;
    let cur_outSupertrend = 0;
    let cur_outTrend = 0;
    let finalLower = sp.finalLower;
    let finalUpper = sp.finalUpper;
    let isUptrend = sp.isUptrend;
    let prevATR = sp.prevATR;
    tempLT = inLow;
    tempHT = inHigh;
    tempCY = sp.lag1_inClose;
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    prevATR = fma(sp.wBeta, prevATR, sp.wAlpha * greatest);
    medianPrice = (tempHT + tempLT) / 2.0;
    band = sp.optInMultiplier * prevATR;
    basicUpper = medianPrice + band;
    basicLower = medianPrice - band;
    /* Each band ratchets toward price and is released only by a close on its
     * far side. Nothing keeps the two ordered -- each is released by its own
     * condition, so the lower one can end up above the upper one -- and where
     * that happens a flip puts the emitted band on the far side of the close.
     * The line is therefore NOT monotone within a trend, and the flag does not
     * always say which side of the line price is on; both read as invariants
     * and neither is one.
     */
    if (basicUpper < finalUpper || sp.prevClose > finalUpper) {
      finalUpper = basicUpper;
    }
    if (basicLower > finalLower || sp.prevClose < finalLower) {
      finalLower = basicLower;
    }
    closeToday = inClose;
    /* The trend is carried in its own variable rather than recovered by
     * comparing the previous output against the previous bands. The two
     * carried bands do coincide on some bars -- always on a bar with no true
     * range, and on some of them at a multiplier of zero -- and there that
     * comparison cannot tell them apart, so it would silently lose the
     * hysteresis on exactly the flat input a corpus of real prices lacks.
     */
    if (isUptrend !== 0) {
      if (closeToday < finalLower) {
        isUptrend = 0;
      }
    } else if (closeToday > finalUpper) {
      isUptrend = 1;
    }
    if (isUptrend !== 0) {
      cur_outSupertrend = finalLower;
      cur_outTrend = 1;
    } else {
      cur_outSupertrend = finalUpper;
      cur_outTrend = -1;
    }
    out.supertrend = cur_outSupertrend;
    out.trend = cur_outTrend;
  }

  /**
   * The value at the last bar this stream counted; `peek` does not change it.
   *
   * @param out - Receives the outputs.
   */
  value(out: SupertrendOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('SUPERTREND value', 'SupertrendOut');
    }
    out.supertrend = this.cur_outSupertrend;
    out.trend = this.cur_outTrend;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): SupertrendStream {
    const c = new SupertrendStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.optInMultiplier = this.optInMultiplier;
    c.isUptrend = this.isUptrend;
    c.prevATR = this.prevATR;
    c.wAlpha = this.wAlpha;
    c.wBeta = this.wBeta;
    c.finalUpper = this.finalUpper;
    c.finalLower = this.finalLower;
    c.prevClose = this.prevClose;
    c.lag1_inClose = this.lag1_inClose;
    c.cur_outSupertrend = this.cur_outSupertrend;
    c.cur_outTrend = this.cur_outTrend;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one SUPERTREND bar, written into an object the caller owns and
 * reuses: valid until the next call that writes it.
 */
export class SupertrendOut {
  /** The SuperTrend line: the band the trend is currently riding. */
  supertrend: number = 0;
  /**
   * Trend direction: +1 while the trend rides the lower band, -1 while it rides the
   * upper one.
   */
  trend: number = 0;
}

function supertrendStepImpl(sp: SupertrendStream, inHigh: number, inLow: number, inClose: number): void {
  let val2 = 0;
  let val3 = 0;
  let greatest = 0;
  let tempCY = 0;
  let tempLT = 0;
  let tempHT = 0;
  let medianPrice = 0;
  let band = 0;
  let basicUpper = 0;
  let basicLower = 0;
  let closeToday = 0;
  tempLT = inLow;
  tempHT = inHigh;
  tempCY = sp.lag1_inClose;
  greatest = tempHT - tempLT;
  /* val1 */
  val2 = Math.abs(tempCY - tempHT);
  if (val2 > greatest) {
    greatest = val2;
  }
  val3 = Math.abs(tempCY - tempLT);
  if (val3 > greatest) {
    greatest = val3;
  }
  sp.prevATR = fma(sp.wBeta, sp.prevATR, sp.wAlpha * greatest);
  medianPrice = (tempHT + tempLT) / 2.0;
  band = sp.optInMultiplier * sp.prevATR;
  basicUpper = medianPrice + band;
  basicLower = medianPrice - band;
  /* Each band ratchets toward price and is released only by a close on its
   * far side. Nothing keeps the two ordered -- each is released by its own
   * condition, so the lower one can end up above the upper one -- and where
   * that happens a flip puts the emitted band on the far side of the close.
   * The line is therefore NOT monotone within a trend, and the flag does not
   * always say which side of the line price is on; both read as invariants
   * and neither is one.
   */
  if (basicUpper < sp.finalUpper || sp.prevClose > sp.finalUpper) {
    sp.finalUpper = basicUpper;
  }
  if (basicLower > sp.finalLower || sp.prevClose < sp.finalLower) {
    sp.finalLower = basicLower;
  }
  closeToday = inClose;
  /* The trend is carried in its own variable rather than recovered by
   * comparing the previous output against the previous bands. The two
   * carried bands do coincide on some bars -- always on a bar with no true
   * range, and on some of them at a multiplier of zero -- and there that
   * comparison cannot tell them apart, so it would silently lose the
   * hysteresis on exactly the flat input a corpus of real prices lacks.
   */
  if (sp.isUptrend !== 0) {
    if (closeToday < sp.finalLower) {
      sp.isUptrend = 0;
    }
  } else if (closeToday > sp.finalUpper) {
    sp.isUptrend = 1;
  }
  if (sp.isUptrend !== 0) {
    sp.cur_outSupertrend = sp.finalLower;
    sp.cur_outTrend = 1;
  } else {
    sp.cur_outSupertrend = sp.finalUpper;
    sp.cur_outTrend = -1;
  }
  sp.prevClose = closeToday;
  sp.lag1_inClose = inClose;
}

function supertrendOpenImpl(
  sp: SupertrendStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInMultiplier: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outSupertrend: Float64Array,
  outTrend: Int32Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let i = 0;
  let today = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let isUptrend = 0;
  let prevATR = 0;
  let periodTotal = 0;
  let wAlpha = 0;
  let wBeta = 0;
  let val2 = 0;
  let val3 = 0;
  let greatest = 0;
  let tempCY = 0;
  let tempLT = 0;
  let tempHT = 0;
  let medianPrice = 0;
  let band = 0;
  let basicUpper = 0;
  let basicLower = 0;
  let finalUpper = 0;
  let finalLower = 0;
  let closeToday = 0;
  let prevClose = 0;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length || inClose.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 10;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInMultiplier === Core.REAL_DEFAULT) {
    optInMultiplier = 3.0;
  } else if (!(optInMultiplier >= 0e0 && optInMultiplier <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = supertrendLookback(optInTimePeriod, optInMultiplier, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* The Average True Range is carried inline rather than taken from a call,
   * because the band and the ratchet advance together one bar at a time and a
   * whole-range buffer between them would not stream.
   *
   * The arithmetic order below is the bit-exactness contract with TA_ATR (do
   * not reorder): True Range from high-low, then the two previous-close
   * distances in that order; the seed summed from 0.0 over the first 'period'
   * True Ranges and divided once; the same two Wilder coefficients, wBeta
   * rounded and wAlpha derived from it, in one fused statement.
   */
  wBeta = (optInTimePeriod - 1) / optInTimePeriod;
  wAlpha = 1.0 - wBeta;
  today = startIdx - lookbackTotal + 1;
  periodTotal = 0.0;
  i = optInTimePeriod;
  while (i-- > 0) {
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    periodTotal += greatest;
    today += 1;
  }
  prevATR = periodTotal / optInTimePeriod;
  /* Skip the Average True Range's unstable period. Taking the count from the
   * lookback rather than naming the setting keeps the two from disagreeing.
   */
  i = lookbackTotal - optInTimePeriod;
  while (i !== 0) {
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    prevATR = fma(wBeta, prevATR, wAlpha * greatest);
    today += 1;
    i -= 1;
  }
  /* The first bar has no band to ratchet against and no trend to carry, so
   * both bands take their unclamped value and the trend is seeded long, as
   * ta4j's SuperTrendIndicator does. The formula does not settle this and the
   * published implementations are split on it, so the choice is a convention:
   * it stays visible for as long as the first trend lasts, and on a series
   * whose close never leaves the band it never washes out at all.
   */
  medianPrice = (inHigh[startIdx] + inLow[startIdx]) / 2.0;
  band = optInMultiplier * prevATR;
  finalUpper = medianPrice + band;
  finalLower = medianPrice - band;
  isUptrend = 1;
  prevClose = inClose[startIdx];
  outSupertrend[((0 * outStride) | 0)] = finalLower;
  outTrend[((0 * outStride) | 0)] = 1;
  outIdx = 1;
  today = startIdx + 1;
  while (today <= endIdx) {
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    prevATR = fma(wBeta, prevATR, wAlpha * greatest);
    medianPrice = (tempHT + tempLT) / 2.0;
    band = optInMultiplier * prevATR;
    basicUpper = medianPrice + band;
    basicLower = medianPrice - band;
    /* Each band ratchets toward price and is released only by a close on its
     * far side. Nothing keeps the two ordered -- each is released by its own
     * condition, so the lower one can end up above the upper one -- and where
     * that happens a flip puts the emitted band on the far side of the close.
     * The line is therefore NOT monotone within a trend, and the flag does not
     * always say which side of the line price is on; both read as invariants
     * and neither is one.
     */
    if (basicUpper < finalUpper || prevClose > finalUpper) {
      finalUpper = basicUpper;
    }
    if (basicLower > finalLower || prevClose < finalLower) {
      finalLower = basicLower;
    }
    closeToday = inClose[today];
    /* The trend is carried in its own variable rather than recovered by
     * comparing the previous output against the previous bands. The two
     * carried bands do coincide on some bars -- always on a bar with no true
     * range, and on some of them at a multiplier of zero -- and there that
     * comparison cannot tell them apart, so it would silently lose the
     * hysteresis on exactly the flat input a corpus of real prices lacks.
     */
    if (isUptrend !== 0) {
      if (closeToday < finalLower) {
        isUptrend = 0;
      }
    } else if (closeToday > finalUpper) {
      isUptrend = 1;
    }
    if (isUptrend !== 0) {
      outSupertrend[((outIdx * outStride) | 0)] = finalLower;
      outTrend[((outIdx * outStride) | 0)] = 1;
    } else {
      outSupertrend[((outIdx * outStride) | 0)] = finalUpper;
      outTrend[((outIdx * outStride) | 0)] = -1;
    }
    prevClose = closeToday;
    outIdx += 1;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  sp.optInTimePeriod = optInTimePeriod;
  sp.optInMultiplier = optInMultiplier;
  sp.isUptrend = isUptrend;
  sp.prevATR = prevATR;
  sp.wAlpha = wAlpha;
  sp.wBeta = wBeta;
  sp.finalUpper = finalUpper;
  sp.finalLower = finalLower;
  sp.prevClose = prevClose;
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.cur_outSupertrend = outSupertrend[(outNBElement.value - 1) * outStride];
  sp.cur_outTrend = outTrend[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function supertrendOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInMultiplier: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outSupertrend: Float64Array,
  outTrend: Int32Array,
): SupertrendStream {
  const sp = SupertrendStream.of(core);
  const retCode = supertrendOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, optInMultiplier, outBegIdx, outNBElement, outSupertrend, outTrend, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SUPERTREND openAndFill', inHigh.length, startIdx, supertrendLookback(optInTimePeriod, optInMultiplier, core));
  }
  throw streamFailure('SUPERTREND openAndFill', retCode);
}

/** @internal */
export function supertrendOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInMultiplier: number,
): SupertrendStream {
  const sp = SupertrendStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outSupertrend = new Float64Array(1);
  const sink_outTrend = new Int32Array(1);
  const retCode = supertrendOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, optInMultiplier, outBegIdx, outNBElement, sink_outSupertrend, sink_outTrend, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SUPERTREND open', inHigh.length, startIdx, supertrendLookback(optInTimePeriod, optInMultiplier, core));
  }
  throw streamFailure('SUPERTREND open', retCode);
}

/**
 * Opens a live SUPERTREND stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link supertrend} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Smoothing period of the Average True Range (default 10;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInMultiplier - Multiplier applied to the Average True Range to set the
 *   band width (default 3; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `supertrendLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an
 *   empty one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   real parameter that is not a number, a parameter outside its range or histories
 *   of different lengths.
 *
 * @example
 * ```ts
 * import { SupertrendOut, supertrendOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = supertrendOpen(high, low, close, 10, 3);
 * const peeked = new SupertrendOut();
 * const updated = new SupertrendOut();
 * stream.peek(101.4, 99.1, 100.9, peeked); // a forming bar: commits nothing
 * stream.update(101.4, 99.1, 100.9, updated); // the bar closes
 * ```
 */
export function supertrendOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  optInMultiplier: number,
  core: Core = Core.DEFAULT,
): SupertrendStream {
  optInTimePeriod = requireInt32('SUPERTREND open', 'optInTimePeriod', optInTimePeriod);
  optInMultiplier = requireReal('SUPERTREND open', 'optInMultiplier', optInMultiplier);
  requireArgument('SUPERTREND open', 'inHigh', inHigh);
  requireHistory('SUPERTREND open', inHigh.length);
  requireArgument('SUPERTREND open', 'inLow', inLow);
  requireArgument('SUPERTREND open', 'inClose', inClose);
  requireHistoryLength('SUPERTREND open', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('SUPERTREND open', 'inClose', inClose.length, inHigh.length);
  return supertrendOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInTimePeriod, optInMultiplier);
}

/**
 * {@link supertrendOpen} that also fills the outputs over the whole history in the
 * same pass, bit-identically to {@link supertrend}. The range written is the
 * stream's `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Smoothing period of the Average True Range (default 10;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInMultiplier - Multiplier applied to the Average True Range to set the
 *   band width (default 3; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param outSupertrend - The SuperTrend line: the band the trend is currently
 *   riding. Must hold `historyLen - lookback` values and share no memory with an
 *   input or another output.
 * @param outTrend - Trend direction: +1 while the trend rides the lower band, -1
 *   while it rides the upper one. Must hold `historyLen - lookback` values and share
 *   no memory with an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `supertrendLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an
 *   empty one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   real parameter that is not a number, a parameter outside its range, histories of
 *   different lengths or an output too short or starting where another output or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { supertrendOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const supertrendOut = new Float64Array(high.length);
 * const trend = new Int32Array(high.length);
 * const stream = supertrendOpenAndFill(
 *   high, low, close,
 *   10, 3,
 *   supertrendOut, trend,
 * );
 * const { begIdx, count } = stream.outRange();
 * // supertrendOut[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function supertrendOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  optInMultiplier: number,
  outSupertrend: Float64Array,
  outTrend: Int32Array,
  core: Core = Core.DEFAULT,
): SupertrendStream {
  optInTimePeriod = requireInt32('SUPERTREND openAndFill', 'optInTimePeriod', optInTimePeriod);
  optInMultiplier = requireReal('SUPERTREND openAndFill', 'optInMultiplier', optInMultiplier);
  requireArgument('SUPERTREND openAndFill', 'inHigh', inHigh);
  requireHistory('SUPERTREND openAndFill', inHigh.length);
  requireArgument('SUPERTREND openAndFill', 'inLow', inLow);
  requireArgument('SUPERTREND openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('SUPERTREND openAndFill', inHigh.length, supertrendLookback(optInTimePeriod, optInMultiplier, core));
  requireHistoryLength('SUPERTREND openAndFill', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('SUPERTREND openAndFill', 'inClose', inClose.length, inHigh.length);
  requireLength('SUPERTREND openAndFill', 'outSupertrend', outSupertrend, guardOutLen);
  requireLength('SUPERTREND openAndFill', 'outTrend', outTrend, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outSupertrend.buffer === inHighF64.buffer && outSupertrend.byteOffset === inHighF64.byteOffset) || (outSupertrend.buffer === inLowF64.buffer && outSupertrend.byteOffset === inLowF64.byteOffset) || (outSupertrend.buffer === inCloseF64.buffer && outSupertrend.byteOffset === inCloseF64.byteOffset) || (outTrend.buffer === inHighF64.buffer && outTrend.byteOffset === inHighF64.byteOffset) || (outTrend.buffer === inLowF64.buffer && outTrend.byteOffset === inLowF64.byteOffset) || (outTrend.buffer === inCloseF64.buffer && outTrend.byteOffset === inCloseF64.byteOffset) || (outSupertrend.buffer === outTrend.buffer && outSupertrend.byteOffset === outTrend.byteOffset)) {
    throw streamFailure('SUPERTREND openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outSupertrend, inHighF64) || overlaps(outSupertrend, inLowF64) || overlaps(outSupertrend, inCloseF64) || overlaps(outTrend, inHighF64) || overlaps(outTrend, inLowF64) || overlaps(outTrend, inCloseF64) || overlaps(outSupertrend, outTrend))) {
    throw streamFailure('SUPERTREND openAndFill', RetCode.BAD_PARAM);
  }
  return supertrendOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInTimePeriod, optInMultiplier, new MInteger(), new MInteger(), outSupertrend, outTrend);
}
