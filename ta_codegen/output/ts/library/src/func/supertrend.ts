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

import { badSink, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireReal, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { atrLookbackPositional } from './atr.ts';

/** @internal */
export function supertrendLookbackPositional(optInTimePeriod: number, optInMultiplier: number, core: Core = Core.DEFAULT): number {
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
  return atrLookbackPositional(optInTimePeriod, core);
}

/** @internal */
export function supertrendDisplayShiftPositional(optInTimePeriod: number, optInMultiplier: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (supertrendLookbackPositional(optInTimePeriod, optInMultiplier, core) < 0) {
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
  lookbackTotal = supertrendLookbackPositional(optInTimePeriod, optInMultiplier, core);
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

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['multiplier', Core.REAL_DEFAULT, 0e0, Core.REAL_MAX, 'in 0..3e37']] as const;

/** @internal */
export function supertrendPositional(
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
  optInTimePeriod = requireInt32('SUPERTREND', 'timePeriod', optInTimePeriod);
  optInMultiplier = requireReal('SUPERTREND', 'multiplier', optInMultiplier);
  requireIndexRange('SUPERTREND', startIdx, endIdx);
  const guardLookback = supertrendLookbackPositional(optInTimePeriod, optInMultiplier, core);
  if (guardLookback < 0) {
    throw rejectedParams('SUPERTREND', PARAM_RANGES, optInTimePeriod, optInMultiplier);
  }
  const guardStart = clampedStart('SUPERTREND', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('SUPERTREND', 'prices.high', inHigh, guardInLen);
  requireLength('SUPERTREND', 'prices.low', inLow, guardInLen);
  requireLength('SUPERTREND', 'prices.close', inClose, guardInLen);
  requireOutput('SUPERTREND', 'outSupertrend', outSupertrend, guardOutLen, Float64Array);
  requireOutput('SUPERTREND', 'outTrend', outTrend, guardOutLen, Int32Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outSupertrend, inHighF64)) {
    throw sharedMemory('SUPERTREND', 'outSupertrend', 'prices.high');
  }
  if (overlapsPartly(outSupertrend, inLowF64)) {
    throw sharedMemory('SUPERTREND', 'outSupertrend', 'prices.low');
  }
  if (overlapsPartly(outSupertrend, inCloseF64)) {
    throw sharedMemory('SUPERTREND', 'outSupertrend', 'prices.close');
  }
  if (overlaps(outTrend, inHighF64)) {
    throw sharedMemory('SUPERTREND', 'outTrend', 'prices.high');
  }
  if (overlaps(outTrend, inLowF64)) {
    throw sharedMemory('SUPERTREND', 'outTrend', 'prices.low');
  }
  if (overlaps(outTrend, inCloseF64)) {
    throw sharedMemory('SUPERTREND', 'outTrend', 'prices.close');
  }
  if (overlaps(outSupertrend, outTrend)) {
    throw sharedMemory('SUPERTREND', 'outSupertrend', 'outTrend');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = supertrendImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInTimePeriod, optInMultiplier, outBegIdx, outNBElement, outSupertrend, outTrend);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('SUPERTREND', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link supertrend}, {@link supertrendLookback},
 * {@link supertrendDisplayShift}, {@link supertrendOpen} and
 * {@link supertrendOpenAndFill}. Every key is optional: an omitted or `undefined`
 * key takes its default, and a call ignores the keys it has no use for.
 */
export interface SupertrendOptions {
  /**
   * Smoothing period of the Average True Range (default 10; range 2 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  timePeriod?: number | undefined;
  /**
   * Multiplier applied to the Average True Range to set the band width (default 3;
   * minimum 0; `Core.REAL_DEFAULT` selects the default).
   */
  multiplier?: number | undefined;
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
   * Buffer for `outSupertrend`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outSupertrend?: Float64Array | undefined;
  /**
   * Buffer for `outTrend`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input.
   */
  outTrend?: Int32Array | undefined;
}

/**
 * What {@link supertrend} returns, and {@link supertrendOpenAndFill} with its
 * stream.
 */
export interface SupertrendResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * The SuperTrend line: the band the trend is currently riding. `outSupertrend[k]`
   * is the value at input bar `begIdx + k`.
   */
  outSupertrend: Float64Array;
  /**
   * Trend direction: +1 while the trend rides the lower band, -1 while it rides the
   * upper one. `outTrend[k]` is the value at input bar `begIdx + k`.
   */
  outTrend: Int32Array;
}

/**
 * Number of leading input bars {@link supertrend} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SupertrendOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function supertrendLookback(options?: SupertrendOptions): number {
  const o = optionsOf('SUPERTREND lookback', options);
  return supertrendLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.multiplier, Core.REAL_DEFAULT), coreOf('SUPERTREND lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link supertrend}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SupertrendOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function supertrendDisplayShift(outputIdx: number, options?: SupertrendOptions): number {
  const o = optionsOf('SUPERTREND displayShift', options);
  return supertrendDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.multiplier, Core.REAL_DEFAULT), outputIdx, coreOf('SUPERTREND displayShift', o.core));
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
 * `outSupertrend[k]` is the value at input bar `begIdx + k`; every output is
 * exactly as long as the number of values written, and nothing is padded. A range
 * that ends before {@link supertrendLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `high`: High
 *   price of each bar. `low`: Low price of each bar. `close`: Close price of each
 *   bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link SupertrendOptions}.
 * @returns `begIdx` and the values; see {@link SupertrendResult}. Each output is a
 *   view of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, an integer parameter that is not an int32, a real parameter that is not a
 *   number, a parameter outside its range, an input too short for the range, a
 *   buffer that is empty, too short or of the wrong type, two buffers sharing memory
 *   or a buffer sharing memory with a `Float64Array` input it is not. Checked before
 *   anything is written.
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
 * const { begIdx, outSupertrend, outTrend } = supertrend({ high, low, close }, {
 *   timePeriod: 10,
 *   multiplier: 3,
 * });
 * // outSupertrend[k] is the value at bar begIdx + k
 * ```
 * @see [`atr`](https://ta-lib.org/functions/atr)
 * @see [`medprice`](https://ta-lib.org/functions/medprice)
 * @see [`sar`](https://ta-lib.org/functions/sar)
 * @see [`sarext`](https://ta-lib.org/functions/sarext)
 * @see [`kc`](https://ta-lib.org/functions/kc)
 */
export function supertrend(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: SupertrendOptions): SupertrendResult {
  requireArgument('SUPERTREND', 'prices', prices);
  const o = optionsOf('SUPERTREND', options);
  if (o.endIdx === undefined) {
    requireSeries('SUPERTREND', 'prices.high', prices.high);
    requireSeries('SUPERTREND', 'prices.low', prices.low);
    requireSeries('SUPERTREND', 'prices.close', prices.close);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInMultiplier = orDefault(o.multiplier, Core.REAL_DEFAULT);
  const core = coreOf('SUPERTREND', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, supertrendLookbackPositional(optInTimePeriod, optInMultiplier, core), prices.high, prices.low, prices.close);
  const outSupertrend = o.outSupertrend === undefined ? new Float64Array(capacity) : o.outSupertrend;
  const outTrend = o.outTrend === undefined ? new Int32Array(capacity) : o.outTrend;
  const range = supertrendPositional(startIdx, endIdx, prices.high, prices.low, prices.close, optInTimePeriod, optInMultiplier, outSupertrend, outTrend, core);
  return { begIdx: range.begIdx, outSupertrend: outSupertrend.subarray(0, range.count), outTrend: outTrend.subarray(0, range.count) };
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
   * Commits one closed bar and returns its outputs.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param out - Receives the outputs and is returned; a new {@link SupertrendOut}
   *   when omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number, inClose: number, out?: SupertrendOut): SupertrendOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('SUPERTREND update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('SUPERTREND update', 'SupertrendOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('SUPERTREND update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    supertrendStepImpl(this, inHigh, inLow, inClose);
    this.outRangeCount++;
    if (out === undefined) {
      out = new SupertrendOut();
    }
    out.outSupertrend = this.cur_outSupertrend;
    out.outTrend = this.cur_outTrend;
    return out;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param out - Receives the outputs and is returned; a new {@link SupertrendOut}
   *   when omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number, inClose: number, out?: SupertrendOut): SupertrendOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
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
    if (out === undefined) {
      out = new SupertrendOut();
    }
    out.outSupertrend = cur_outSupertrend;
    out.outTrend = cur_outTrend;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link SupertrendOut}
   *   when omitted.
   * @returns `out`, or the new sink.
   */
  value(out?: SupertrendOut): SupertrendOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('SUPERTREND value', 'SupertrendOut');
    }
    if (out === undefined) {
      out = new SupertrendOut();
    }
    out.outSupertrend = this.cur_outSupertrend;
    out.outTrend = this.cur_outTrend;
    return out;
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
 * The outputs of one SUPERTREND bar, as `update`, `peek` and `value` return them.
 * Passed back to one of them, it is filled and returned again, with no allocation;
 * it holds its values until the next call that writes it. A new one holds `NaN`.
 */
export class SupertrendOut {
  /** The SuperTrend line: the band the trend is currently riding. */
  outSupertrend: number = NaN;
  /**
   * Trend direction: +1 while the trend rides the lower band, -1 while it rides the
   * upper one.
   */
  outTrend: number = NaN;
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
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = supertrendLookbackPositional(optInTimePeriod, optInMultiplier, core);
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
    throw insufficientHistory('SUPERTREND openAndFill', inHigh.length, startIdx, supertrendLookbackPositional(optInTimePeriod, optInMultiplier, core));
  }
  if (retCode === RetCode.BAD_PARAM && supertrendLookbackPositional(optInTimePeriod, optInMultiplier, core) < 0) {
    throw rejectedParams('SUPERTREND openAndFill', PARAM_RANGES, optInTimePeriod, optInMultiplier);
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
    throw insufficientHistory('SUPERTREND open', inHigh.length, startIdx, supertrendLookbackPositional(optInTimePeriod, optInMultiplier, core));
  }
  if (retCode === RetCode.BAD_PARAM && supertrendLookbackPositional(optInTimePeriod, optInMultiplier, core) < 0) {
    throw rejectedParams('SUPERTREND open', PARAM_RANGES, optInTimePeriod, optInMultiplier);
  }
  throw streamFailure('SUPERTREND open', retCode);
}

/** @internal */
export function supertrendOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  optInMultiplier: number,
  core: Core = Core.DEFAULT,
): SupertrendStream {
  optInTimePeriod = requireInt32('SUPERTREND open', 'timePeriod', optInTimePeriod);
  optInMultiplier = requireReal('SUPERTREND open', 'multiplier', optInMultiplier);
  requireArgument('SUPERTREND open', 'prices.high', inHigh);
  requireHistory('SUPERTREND open', inHigh.length);
  requireArgument('SUPERTREND open', 'prices.low', inLow);
  requireArgument('SUPERTREND open', 'prices.close', inClose);
  requireHistoryLength('SUPERTREND open', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('SUPERTREND open', 'prices.close', inClose.length, inHigh.length);
  return supertrendOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInTimePeriod, optInMultiplier);
}

/** @internal */
export function supertrendOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  optInMultiplier: number,
  outSupertrend: Float64Array,
  outTrend: Int32Array,
  core: Core = Core.DEFAULT,
): SupertrendStream {
  optInTimePeriod = requireInt32('SUPERTREND openAndFill', 'timePeriod', optInTimePeriod);
  optInMultiplier = requireReal('SUPERTREND openAndFill', 'multiplier', optInMultiplier);
  requireArgument('SUPERTREND openAndFill', 'prices.high', inHigh);
  requireHistory('SUPERTREND openAndFill', inHigh.length);
  requireArgument('SUPERTREND openAndFill', 'prices.low', inLow);
  requireArgument('SUPERTREND openAndFill', 'prices.close', inClose);
  const guardLookback = supertrendLookbackPositional(optInTimePeriod, optInMultiplier, core);
  if (guardLookback < 0) {
    throw rejectedParams('SUPERTREND openAndFill', PARAM_RANGES, optInTimePeriod, optInMultiplier);
  }
  const guardOutLen = openFillCount('SUPERTREND openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('SUPERTREND openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('SUPERTREND openAndFill', 'prices.close', inClose.length, inHigh.length);
  requireOutput('SUPERTREND openAndFill', 'outSupertrend', outSupertrend, guardOutLen, Float64Array);
  requireOutput('SUPERTREND openAndFill', 'outTrend', outTrend, guardOutLen, Int32Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outSupertrend.buffer === inHighF64.buffer && outSupertrend.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('SUPERTREND openAndFill', 'outSupertrend', 'prices.high');
  }
  if (outSupertrend.buffer === inLowF64.buffer && outSupertrend.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('SUPERTREND openAndFill', 'outSupertrend', 'prices.low');
  }
  if (outSupertrend.buffer === inCloseF64.buffer && outSupertrend.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('SUPERTREND openAndFill', 'outSupertrend', 'prices.close');
  }
  if (outTrend.buffer === inHighF64.buffer && outTrend.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('SUPERTREND openAndFill', 'outTrend', 'prices.high');
  }
  if (outTrend.buffer === inLowF64.buffer && outTrend.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('SUPERTREND openAndFill', 'outTrend', 'prices.low');
  }
  if (outTrend.buffer === inCloseF64.buffer && outTrend.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('SUPERTREND openAndFill', 'outTrend', 'prices.close');
  }
  if (outSupertrend.buffer === outTrend.buffer && outSupertrend.byteOffset === outTrend.byteOffset) {
    throw sharedMemory('SUPERTREND openAndFill', 'outSupertrend', 'outTrend');
  }
  if (guardOutLen > 0 && overlaps(outSupertrend, inHighF64)) {
    throw sharedMemory('SUPERTREND openAndFill', 'outSupertrend', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outSupertrend, inLowF64)) {
    throw sharedMemory('SUPERTREND openAndFill', 'outSupertrend', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outSupertrend, inCloseF64)) {
    throw sharedMemory('SUPERTREND openAndFill', 'outSupertrend', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outTrend, inHighF64)) {
    throw sharedMemory('SUPERTREND openAndFill', 'outTrend', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outTrend, inLowF64)) {
    throw sharedMemory('SUPERTREND openAndFill', 'outTrend', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outTrend, inCloseF64)) {
    throw sharedMemory('SUPERTREND openAndFill', 'outTrend', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outSupertrend, outTrend)) {
    throw sharedMemory('SUPERTREND openAndFill', 'outSupertrend', 'outTrend');
  }
  return supertrendOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInTimePeriod, optInMultiplier, new MInteger(), new MInteger(), outSupertrend, outTrend);
}

/**
 * Opens a live SUPERTREND stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link supertrend} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SupertrendOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `supertrendLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, an integer parameter that is not an int32, a real
 *   parameter that is not a number, a parameter outside its range or histories of
 *   different lengths.
 *
 * @example
 * ```ts
 * import { supertrendOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = supertrendOpen({ high, low, close }, {
 *   timePeriod: 10,
 *   multiplier: 3,
 * });
 * const peeked = stream.peek(101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function supertrendOpen(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: SupertrendOptions): SupertrendStream {
  requireArgument('SUPERTREND open', 'prices', prices);
  const o = optionsOf('SUPERTREND open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInMultiplier = orDefault(o.multiplier, Core.REAL_DEFAULT);
  const core = coreOf('SUPERTREND open', o.core);
  return supertrendOpenPositional(prices.high, prices.low, prices.close, optInTimePeriod, optInMultiplier, core);
}

/**
 * {@link supertrendOpen} that also computes the outputs over the whole history in
 * the same pass, bit-identically to {@link supertrend}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link SupertrendOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link supertrend} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `supertrendLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, an integer parameter that is not an int32, a real
 *   parameter that is not a number, a parameter outside its range, histories of
 *   different lengths or a buffer too short, of the wrong type or starting where
 *   another buffer or a `Float64Array` input starts, or, on a history long enough to
 *   open, one sharing memory with another buffer or a `Float64Array` input.
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
 * const {
 *   stream,
 *   begIdx,
 *   outSupertrend,
 *   outTrend,
 * } = supertrendOpenAndFill({ high, low, close }, {
 *   timePeriod: 10,
 *   multiplier: 3,
 * });
 * // outSupertrend[k] is the value at bar begIdx + k
 * ```
 */
export function supertrendOpenAndFill(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: SupertrendOptions): SupertrendResult & { stream: SupertrendStream } {
  requireArgument('SUPERTREND openAndFill', 'prices', prices);
  const o = optionsOf('SUPERTREND openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInMultiplier = orDefault(o.multiplier, Core.REAL_DEFAULT);
  const core = coreOf('SUPERTREND openAndFill', o.core);
  const capacity = fillCapacity(supertrendLookbackPositional(optInTimePeriod, optInMultiplier, core), prices.high, prices.low, prices.close);
  const outSupertrend = o.outSupertrend === undefined ? new Float64Array(capacity) : o.outSupertrend;
  const outTrend = o.outTrend === undefined ? new Int32Array(capacity) : o.outTrend;
  const stream = supertrendOpenAndFillPositional(prices.high, prices.low, prices.close, optInTimePeriod, optInMultiplier, outSupertrend, outTrend, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outSupertrend: outSupertrend.subarray(0, range.count), outTrend: outTrend.subarray(0, range.count) };
}
