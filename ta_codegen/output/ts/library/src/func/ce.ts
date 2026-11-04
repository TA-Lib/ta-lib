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
 *  103126 MF,CC  Creation (#568).
 */

import { badSink, clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireReal, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { atrLookback } from './atr.ts';

/**
 * Number of leading input bars {@link ce} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Window of the highest high and lowest low, and
 *   smoothing period of the Average True Range (default 22; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param optInMultiplier - Multiplier applied to the Average True Range to offset
 *   the lines (default 3; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function ceLookback(optInTimePeriod: number, optInMultiplier: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (typeof optInMultiplier !== 'number') {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 22;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  if (optInMultiplier === Core.REAL_DEFAULT) {
    optInMultiplier = 3.0;
  } else if (!(optInMultiplier >= 0e0 && optInMultiplier <= Core.REAL_MAX)) {
    return -1;
  }
  /* The ATR term is never restated here, which is what makes CE inherit
   * TA_FUNC_UNST_ATR. It is at least the extreme window's own lookback.
   */
  return atrLookback(optInTimePeriod, core);
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link ce}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Window of the highest high and lowest low, and
 *   smoothing period of the Average True Range (default 22; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param optInMultiplier - Multiplier applied to the Average True Range to offset
 *   the lines (default 3; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function ceDisplayShift(optInTimePeriod: number, optInMultiplier: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (ceLookback(optInTimePeriod, optInMultiplier, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 2) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function ceImpl(
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
  outRealLong: Float64Array,
  outRealShort: Float64Array,
): RetCode {
  let hhBuf!: Float64Array;
  let hhBuf_Idx = 0;
  let maxIdx_hhBuf = (30) - 1;
  let llBuf!: Float64Array;
  let i = 0;
  let ip = 0;
  let today = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let lastP = 0;
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
  let tempReal = 0;
  let hhPre = 0;
  let llPre = 0;
  let sufHi = 0;
  let sufLo = 0;
  let highest = 0;
  let lowest = 0;
  let band = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 22;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInMultiplier === Core.REAL_DEFAULT) {
    optInMultiplier = 3.0;
  } else if (!(optInMultiplier >= 0e0 && optInMultiplier <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if ((outRealLong.buffer === outRealShort.buffer && outRealLong.byteOffset === outRealShort.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = ceLookback(optInTimePeriod, optInMultiplier, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  /* Two rolling extrema, van Herk / Gil-Werman, one buffer each: the window
   * ending in slot j is the current block's prefix extremum joined with the
   * previous block's suffix extremum from slot j+1. A slot holds the raw value
   * while its block is filling and is turned into the suffix extremum, in
   * place, when the block completes; slot j+1 is always still the previous
   * block's when slot j is filled. The high and low sides share an index.
   *
   * The extrema are exact, so both outputs must stay bit-identical to
   * TA_MAX(high) - x*TA_ATR and its mirror, whatever the block phase; only
   * the sign of a zero tied between +0.0 and -0.0 is free.
   */
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  hhBuf = new Float64Array(optInTimePeriod);
  maxIdx_hhBuf = optInTimePeriod - 1;
  hhBuf_Idx = 0;
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  llBuf = new Float64Array(optInTimePeriod);
  lastP = optInTimePeriod - 1;
  /* The Average True Range is carried inline rather than taken from a call,
   * so that the extreme window and the average advance together one bar at
   * a time and the body streams.
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
  /* Skip the bars the lookback adds for the unstable period. Taking the count
   * from the lookback rather than naming the setting keeps the two from
   * disagreeing.
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
  /* prevATR is now the Average True Range of bar today-1, the first output
   * bar. Its extreme window is one whole block, taken straight from the
   * input.
   */
  highest = inHigh[today - 1];
  lowest = inLow[today - 1];
  hhBuf[lastP] = highest;
  llBuf[lastP] = lowest;
  ip = lastP;
  while (ip > 0) {
    ip -= 1;
    tempHT = inHigh[today - 1 - lastP + ip];
    tempLT = inLow[today - 1 - lastP + ip];
    if (tempHT > highest) {
      highest = tempHT;
    }
    if (tempLT < lowest) {
      lowest = tempLT;
    }
    hhBuf[ip] = highest;
    llBuf[ip] = lowest;
  }
  hhPre = highest;
  llPre = lowest;
  /* The multiple of the ATR is formed on its own, never fused into the
   * offset, so that each line is TA_MAX - x*TA_ATR (or its mirror) as a
   * caller composing the three functions would compute it.
   */
  band = optInMultiplier * prevATR;
  outRealLong[0] = highest - band;
  outRealShort[0] = lowest + band;
  outIdx = 1;
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
    /* Keep this shape: the prefix extreme selected in a local, the
     * block-start override after it, one store. A store made only when the
     * bar sets a new extreme, or the override ahead of the select, compiles
     * to a data-dependent branch in the stream step, where the prefix lives
     * in the handle.
     */
    highest = hhPre;
    lowest = llPre;
    if (tempHT > highest) {
      highest = tempHT;
    }
    if (tempLT < lowest) {
      lowest = tempLT;
    }
    if (hhBuf_Idx === 0) {
      highest = tempHT;
      lowest = tempLT;
    }
    hhPre = highest;
    llPre = lowest;
    if (hhBuf_Idx < lastP) {
      tempReal = hhBuf[hhBuf_Idx + 1];
      if (tempReal > highest) {
        highest = tempReal;
      }
      tempReal = llBuf[hhBuf_Idx + 1];
      if (tempReal < lowest) {
        lowest = tempReal;
      }
    }
    hhBuf[hhBuf_Idx] = tempHT;
    llBuf[hhBuf_Idx] = tempLT;
    hhBuf_Idx++;
    if (hhBuf_Idx > maxIdx_hhBuf) {
      hhBuf_Idx = 0;
    }
    band = optInMultiplier * prevATR;
    outRealLong[outIdx] = highest - band;
    outRealShort[outIdx] = lowest + band;
    /* A completed block becomes its suffix extrema. Nothing this bar reads
     * them, so the pass stays below the output stores, where the peek
     * frame never runs it.
     */
    if (hhBuf_Idx === 0) {
      sufHi = hhBuf[lastP];
      sufLo = llBuf[lastP];
      ip = lastP;
      while (ip > 1) {
        ip -= 1;
        tempReal = hhBuf[ip];
        if (tempReal > sufHi) {
          sufHi = tempReal;
        }
        hhBuf[ip] = sufHi;
        tempReal = llBuf[ip];
        if (tempReal < sufLo) {
          sufLo = tempReal;
        }
        llBuf[ip] = sufLo;
      }
    }
    outIdx += 1;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * Le Beau's Chandelier Exit, a two-line volatility stop. The highest high of the
 * window is offset down, and the lowest low up, by a multiple of the Average True
 * Range, so each line hangs from the recent extreme at a distance that widens as
 * the true range grows. The long line is the exit for a long position and hangs
 * below the recent highs; the short line is the mirror for a short position, above
 * the recent lows.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ce](https://ta-lib.org/functions/ce).
 *
 * **Notes**
 * - The extremes are taken over the last TimePeriod bars. Le Beau anchors the
 *   highest high at the entry of the trade, which a function over a price series
 *   cannot know; the rolling window is what charting platforms plot under this
 *   name.
 * - The Average True Range is this library's, whose first value is the average of
 *   the first full period of true ranges that have a previous close. Platforms
 *   that start the true range on the very first bar give slightly different values
 *   on early bars; the difference decays as the average warms up.
 * - Both lines are emitted on every bar, each from the highs or the lows. A
 *   popular variant takes the extremes of the closes and keeps a single stop that
 *   only moves toward price until the trend flips; that variant is not this
 *   function.
 * - Neither line is always on its side of the other: with a small multiplier the
 *   long line sits above the short one.
 * - Both lines inherit the Average True Range's warm-up, so a caller who wants
 *   them converged sets `TA_FUNC_UNST_ATR`, exactly as when calling that function
 *   directly.
 * - The two lines are the Chande Kroll Stop with a stop period of 1, `outRealLong`
 *   being CKSP's `outHighStop`.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link ceLookback} is a success with `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param optInTimePeriod - Window of the highest high and lowest low, and
 *   smoothing period of the Average True Range (default 22; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param optInMultiplier - Multiplier applied to the Average True Range to offset
 *   the lines (default 3; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param outRealLong - Highest high less the multiple of the Average True Range;
 *   the stop for a long position. Must hold the values produced and never be empty;
 *   the input length always suffices.
 * @param outRealShort - Lowest low plus the multiple of the Average True Range;
 *   the stop for a short position. Must hold the values produced and never be empty;
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
 * import { ce } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const long = new Float64Array(high.length);
 * const short = new Float64Array(high.length);
 * const { begIdx, count } = ce(
 *   0, high.length - 1,
 *   high, low, close,
 *   22, 3,
 *   long, short,
 * );
 * // long[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`atr`](https://ta-lib.org/functions/atr)
 * @see [`max`](https://ta-lib.org/functions/max)
 * @see [`min`](https://ta-lib.org/functions/min)
 * @see [`cksp`](https://ta-lib.org/functions/cksp)
 * @see [`supertrend`](https://ta-lib.org/functions/supertrend)
 * @see [`kc`](https://ta-lib.org/functions/kc)
 * @see [`sar`](https://ta-lib.org/functions/sar)
 */
export function ce(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  optInMultiplier: number,
  outRealLong: Float64Array,
  outRealShort: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CE', 'startIdx', startIdx);
  endIdx = requireInt32('CE', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('CE', 'optInTimePeriod', optInTimePeriod);
  optInMultiplier = requireReal('CE', 'optInMultiplier', optInMultiplier);
  requireIndexRange('CE', startIdx, endIdx);
  const guardStart = clampedStart('CE', startIdx, ceLookback(optInTimePeriod, optInMultiplier, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CE', 'inHigh', inHigh, guardInLen);
  requireLength('CE', 'inLow', inLow, guardInLen);
  requireLength('CE', 'inClose', inClose, guardInLen);
  requireOutput('CE', 'outRealLong', outRealLong, guardOutLen, Float64Array);
  requireOutput('CE', 'outRealShort', outRealShort, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outRealLong, inHighF64) || overlapsPartly(outRealLong, inLowF64) || overlapsPartly(outRealLong, inCloseF64) || overlapsPartly(outRealShort, inHighF64) || overlapsPartly(outRealShort, inLowF64) || overlapsPartly(outRealShort, inCloseF64) || overlaps(outRealLong, outRealShort)) {
    throw failure('CE', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = ceImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInTimePeriod, optInMultiplier, outBegIdx, outNBElement, outRealLong, outRealShort);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CE', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live CE stream: one value per closed bar, bit-identical to {@link ce} over the
 * same series. Open it with {@link ceOpen} or {@link ceOpenAndFill}; there is
 * nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ce](https://ta-lib.org/functions/ce).
 */
export class CeStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  optInMultiplier: number = 0;
  /** @internal */
  lastP: number = 0;
  /** @internal */
  prevATR: number = 0;
  /** @internal */
  wAlpha: number = 0;
  /** @internal */
  wBeta: number = 0;
  /** @internal */
  hhPre: number = 0;
  /** @internal */
  llPre: number = 0;
  /** @internal */
  hhBuf_Idx: number = 0;
  /** @internal */
  maxIdx_hhBuf: number = 0;
  /** @internal */
  llBuf_Idx: number = 0;
  /** @internal */
  maxIdx_llBuf: number = 0;
  /** @internal */
  lag1_inClose: number = 0;
  /** @internal */
  cbSize_hhBuf: number = 0;
  /** @internal */
  cb_hhBuf: Float64Array = new Float64Array(1);
  /** @internal */
  cbSize_llBuf: number = 0;
  /** @internal */
  cb_llBuf: Float64Array = new Float64Array(1);
  /** @internal */
  cur_outRealLong: number = 0;
  /** @internal */
  cur_outRealShort: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): CeStream {
    return new CeStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link ce} reports them over the same bars. Each accepted `update` and each
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
      throw failure('CE advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
  update(inHigh: number, inLow: number, inClose: number, out: CeOut): void {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('CE update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (typeof out !== 'object' || out === null) {
      throw badSink('CE update', 'CeOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('CE update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    ceStepImpl(this, inHigh, inLow, inClose);
    this.outRangeCount++;
    out.realLong = this.cur_outRealLong;
    out.realShort = this.cur_outRealShort;
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
  peek(inHigh: number, inLow: number, inClose: number, out: CeOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('CE peek', 'CeOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('CE peek', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let val2 = 0;
    let val3 = 0;
    let greatest = 0;
    let tempCY = 0;
    let tempLT = 0;
    let tempHT = 0;
    let tempReal = 0;
    let highest = 0;
    let lowest = 0;
    let band = 0;
    let cur_outRealLong = 0;
    let cur_outRealShort = 0;
    let hhBuf_Idx = sp.hhBuf_Idx;
    let hhPre = sp.hhPre;
    let llPre = sp.llPre;
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
    /* Keep this shape: the prefix extreme selected in a local, the
     * block-start override after it, one store. A store made only when the
     * bar sets a new extreme, or the override ahead of the select, compiles
     * to a data-dependent branch in the stream step, where the prefix lives
     * in the handle.
     */
    highest = hhPre;
    lowest = llPre;
    if (tempHT > highest) {
      highest = tempHT;
    }
    if (tempLT < lowest) {
      lowest = tempLT;
    }
    if (hhBuf_Idx === 0) {
      highest = tempHT;
      lowest = tempLT;
    }
    hhPre = highest;
    llPre = lowest;
    if (hhBuf_Idx < sp.lastP) {
      tempReal = sp.cb_hhBuf[hhBuf_Idx + 1];
      if (tempReal > highest) {
        highest = tempReal;
      }
      tempReal = sp.cb_llBuf[hhBuf_Idx + 1];
      if (tempReal < lowest) {
        lowest = tempReal;
      }
    }
    hhBuf_Idx = hhBuf_Idx + 1;
    if (hhBuf_Idx > sp.maxIdx_hhBuf) {
      hhBuf_Idx = 0;
    }
    band = sp.optInMultiplier * prevATR;
    cur_outRealLong = highest - band;
    cur_outRealShort = lowest + band;
    out.realLong = cur_outRealLong;
    out.realShort = cur_outRealShort;
  }

  /**
   * The value at the last bar this stream counted; `peek` does not change it.
   *
   * @param out - Receives the outputs.
   */
  value(out: CeOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('CE value', 'CeOut');
    }
    out.realLong = this.cur_outRealLong;
    out.realShort = this.cur_outRealShort;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): CeStream {
    const c = new CeStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.optInMultiplier = this.optInMultiplier;
    c.lastP = this.lastP;
    c.prevATR = this.prevATR;
    c.wAlpha = this.wAlpha;
    c.wBeta = this.wBeta;
    c.hhPre = this.hhPre;
    c.llPre = this.llPre;
    c.hhBuf_Idx = this.hhBuf_Idx;
    c.maxIdx_hhBuf = this.maxIdx_hhBuf;
    c.llBuf_Idx = this.llBuf_Idx;
    c.maxIdx_llBuf = this.maxIdx_llBuf;
    c.lag1_inClose = this.lag1_inClose;
    c.cbSize_hhBuf = this.cbSize_hhBuf;
    c.cb_hhBuf = this.cb_hhBuf.slice();
    c.cbSize_llBuf = this.cbSize_llBuf;
    c.cb_llBuf = this.cb_llBuf.slice();
    c.cur_outRealLong = this.cur_outRealLong;
    c.cur_outRealShort = this.cur_outRealShort;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one CE bar, written into an object the caller owns and reuses:
 * valid until the next call that writes it.
 */
export class CeOut {
  /**
   * Highest high less the multiple of the Average True Range; the stop for a long
   * position.
   */
  realLong: number = 0;
  /**
   * Lowest low plus the multiple of the Average True Range; the stop for a short
   * position.
   */
  realShort: number = 0;
}

function ceStepImpl(sp: CeStream, inHigh: number, inLow: number, inClose: number): void {
  let ip = 0;
  let val2 = 0;
  let val3 = 0;
  let greatest = 0;
  let tempCY = 0;
  let tempLT = 0;
  let tempHT = 0;
  let tempReal = 0;
  let sufHi = 0;
  let sufLo = 0;
  let highest = 0;
  let lowest = 0;
  let band = 0;
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
  /* Keep this shape: the prefix extreme selected in a local, the
   * block-start override after it, one store. A store made only when the
   * bar sets a new extreme, or the override ahead of the select, compiles
   * to a data-dependent branch in the stream step, where the prefix lives
   * in the handle.
   */
  highest = sp.hhPre;
  lowest = sp.llPre;
  if (tempHT > highest) {
    highest = tempHT;
  }
  if (tempLT < lowest) {
    lowest = tempLT;
  }
  if (sp.hhBuf_Idx === 0) {
    highest = tempHT;
    lowest = tempLT;
  }
  sp.hhPre = highest;
  sp.llPre = lowest;
  if (sp.hhBuf_Idx < sp.lastP) {
    tempReal = sp.cb_hhBuf[sp.hhBuf_Idx + 1];
    if (tempReal > highest) {
      highest = tempReal;
    }
    tempReal = sp.cb_llBuf[sp.hhBuf_Idx + 1];
    if (tempReal < lowest) {
      lowest = tempReal;
    }
  }
  sp.cb_hhBuf[sp.hhBuf_Idx] = tempHT;
  sp.cb_llBuf[sp.hhBuf_Idx] = tempLT;
  sp.hhBuf_Idx = sp.hhBuf_Idx + 1;
  if (sp.hhBuf_Idx > sp.maxIdx_hhBuf) {
    sp.hhBuf_Idx = 0;
  }
  band = sp.optInMultiplier * sp.prevATR;
  sp.cur_outRealLong = highest - band;
  sp.cur_outRealShort = lowest + band;
  /* A completed block becomes its suffix extrema. Nothing this bar reads
   * them, so the pass stays below the output stores, where the peek
   * frame never runs it.
   */
  if (sp.hhBuf_Idx === 0) {
    sufHi = sp.cb_hhBuf[sp.lastP];
    sufLo = sp.cb_llBuf[sp.lastP];
    ip = sp.lastP;
    while (ip > 1) {
      ip -= 1;
      tempReal = sp.cb_hhBuf[ip];
      if (tempReal > sufHi) {
        sufHi = tempReal;
      }
      sp.cb_hhBuf[ip] = sufHi;
      tempReal = sp.cb_llBuf[ip];
      if (tempReal < sufLo) {
        sufLo = tempReal;
      }
      sp.cb_llBuf[ip] = sufLo;
    }
  }
  sp.lag1_inClose = inClose;
}

function ceOpenImpl(
  sp: CeStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInMultiplier: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outRealLong: Float64Array,
  outRealShort: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let hhBuf!: Float64Array;
  let hhBuf_Idx = 0;
  let maxIdx_hhBuf = (30) - 1;
  let llBuf!: Float64Array;
  let llBuf_Idx = 0;
  let maxIdx_llBuf = (30) - 1;
  let i = 0;
  let ip = 0;
  let today = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let lastP = 0;
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
  let tempReal = 0;
  let hhPre = 0;
  let llPre = 0;
  let sufHi = 0;
  let sufLo = 0;
  let highest = 0;
  let lowest = 0;
  let band = 0;
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
    optInTimePeriod = 22;
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
  lookbackTotal = ceLookback(optInTimePeriod, optInMultiplier, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Two rolling extrema, van Herk / Gil-Werman, one buffer each: the window
   * ending in slot j is the current block's prefix extremum joined with the
   * previous block's suffix extremum from slot j+1. A slot holds the raw value
   * while its block is filling and is turned into the suffix extremum, in
   * place, when the block completes; slot j+1 is always still the previous
   * block's when slot j is filled. The high and low sides share an index.
   *
   * The extrema are exact, so both outputs must stay bit-identical to
   * TA_MAX(high) - x*TA_ATR and its mirror, whatever the block phase; only
   * the sign of a zero tied between +0.0 and -0.0 is free.
   */
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  hhBuf = new Float64Array(optInTimePeriod);
  maxIdx_hhBuf = optInTimePeriod - 1;
  hhBuf_Idx = 0;
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  llBuf = new Float64Array(optInTimePeriod);
  maxIdx_llBuf = optInTimePeriod - 1;
  llBuf_Idx = 0;
  lastP = optInTimePeriod - 1;
  /* The Average True Range is carried inline rather than taken from a call,
   * so that the extreme window and the average advance together one bar at
   * a time and the body streams.
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
  /* Skip the bars the lookback adds for the unstable period. Taking the count
   * from the lookback rather than naming the setting keeps the two from
   * disagreeing.
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
  /* prevATR is now the Average True Range of bar today-1, the first output
   * bar. Its extreme window is one whole block, taken straight from the
   * input.
   */
  highest = inHigh[today - 1];
  lowest = inLow[today - 1];
  hhBuf[lastP] = highest;
  llBuf[lastP] = lowest;
  ip = lastP;
  while (ip > 0) {
    ip -= 1;
    tempHT = inHigh[today - 1 - lastP + ip];
    tempLT = inLow[today - 1 - lastP + ip];
    if (tempHT > highest) {
      highest = tempHT;
    }
    if (tempLT < lowest) {
      lowest = tempLT;
    }
    hhBuf[ip] = highest;
    llBuf[ip] = lowest;
  }
  hhPre = highest;
  llPre = lowest;
  /* The multiple of the ATR is formed on its own, never fused into the
   * offset, so that each line is TA_MAX - x*TA_ATR (or its mirror) as a
   * caller composing the three functions would compute it.
   */
  band = optInMultiplier * prevATR;
  outRealLong[((0 * outStride) | 0)] = highest - band;
  outRealShort[((0 * outStride) | 0)] = lowest + band;
  outIdx = 1;
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
    /* Keep this shape: the prefix extreme selected in a local, the
     * block-start override after it, one store. A store made only when the
     * bar sets a new extreme, or the override ahead of the select, compiles
     * to a data-dependent branch in the stream step, where the prefix lives
     * in the handle.
     */
    highest = hhPre;
    lowest = llPre;
    if (tempHT > highest) {
      highest = tempHT;
    }
    if (tempLT < lowest) {
      lowest = tempLT;
    }
    if (hhBuf_Idx === 0) {
      highest = tempHT;
      lowest = tempLT;
    }
    hhPre = highest;
    llPre = lowest;
    if (hhBuf_Idx < lastP) {
      tempReal = hhBuf[hhBuf_Idx + 1];
      if (tempReal > highest) {
        highest = tempReal;
      }
      tempReal = llBuf[hhBuf_Idx + 1];
      if (tempReal < lowest) {
        lowest = tempReal;
      }
    }
    hhBuf[hhBuf_Idx] = tempHT;
    llBuf[hhBuf_Idx] = tempLT;
    hhBuf_Idx++;
    if (hhBuf_Idx > maxIdx_hhBuf) {
      hhBuf_Idx = 0;
    }
    band = optInMultiplier * prevATR;
    outRealLong[((outIdx * outStride) | 0)] = highest - band;
    outRealShort[((outIdx * outStride) | 0)] = lowest + band;
    /* A completed block becomes its suffix extrema. Nothing this bar reads
     * them, so the pass stays below the output stores, where the peek
     * frame never runs it.
     */
    if (hhBuf_Idx === 0) {
      sufHi = hhBuf[lastP];
      sufLo = llBuf[lastP];
      ip = lastP;
      while (ip > 1) {
        ip -= 1;
        tempReal = hhBuf[ip];
        if (tempReal > sufHi) {
          sufHi = tempReal;
        }
        hhBuf[ip] = sufHi;
        tempReal = llBuf[ip];
        if (tempReal < sufLo) {
          sufLo = tempReal;
        }
        llBuf[ip] = sufLo;
      }
    }
    outIdx += 1;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  const capCb_hhBuf = maxIdx_hhBuf + 1;
  if (capCb_hhBuf > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  const capCb_llBuf = maxIdx_llBuf + 1;
  if (capCb_llBuf > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.optInMultiplier = optInMultiplier;
  sp.lastP = lastP;
  sp.prevATR = prevATR;
  sp.wAlpha = wAlpha;
  sp.wBeta = wBeta;
  sp.hhPre = hhPre;
  sp.llPre = llPre;
  sp.hhBuf_Idx = hhBuf_Idx;
  sp.maxIdx_hhBuf = maxIdx_hhBuf;
  sp.llBuf_Idx = llBuf_Idx;
  sp.maxIdx_llBuf = maxIdx_llBuf;
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.cbSize_hhBuf = capCb_hhBuf;
  sp.cb_hhBuf = hhBuf;
  sp.cbSize_llBuf = capCb_llBuf;
  sp.cb_llBuf = llBuf;
  sp.cur_outRealLong = outRealLong[(outNBElement.value - 1) * outStride];
  sp.cur_outRealShort = outRealShort[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function ceOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInMultiplier: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outRealLong: Float64Array,
  outRealShort: Float64Array,
): CeStream {
  const sp = CeStream.of(core);
  const retCode = ceOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, optInMultiplier, outBegIdx, outNBElement, outRealLong, outRealShort, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CE openAndFill', inHigh.length, startIdx, ceLookback(optInTimePeriod, optInMultiplier, core));
  }
  throw streamFailure('CE openAndFill', retCode);
}

/** @internal */
export function ceOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInMultiplier: number,
): CeStream {
  const sp = CeStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outRealLong = new Float64Array(1);
  const sink_outRealShort = new Float64Array(1);
  const retCode = ceOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, optInMultiplier, outBegIdx, outNBElement, sink_outRealLong, sink_outRealShort, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CE open', inHigh.length, startIdx, ceLookback(optInTimePeriod, optInMultiplier, core));
  }
  throw streamFailure('CE open', retCode);
}

/**
 * Opens a live CE stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link ce} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Window of the highest high and lowest low, and
 *   smoothing period of the Average True Range (default 22; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param optInMultiplier - Multiplier applied to the Average True Range to offset
 *   the lines (default 3; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `ceLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty one,
 *   and `TA_BAD_PARAM` for an integer parameter that is not an int32, a real
 *   parameter that is not a number, a parameter outside its range or histories of
 *   different lengths.
 *
 * @example
 * ```ts
 * import { CeOut, ceOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = ceOpen(high, low, close, 22, 3);
 * const peeked = new CeOut();
 * const updated = new CeOut();
 * stream.peek(101.4, 99.1, 100.9, peeked); // a forming bar: commits nothing
 * stream.update(101.4, 99.1, 100.9, updated); // the bar closes
 * ```
 */
export function ceOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  optInMultiplier: number,
  core: Core = Core.DEFAULT,
): CeStream {
  optInTimePeriod = requireInt32('CE open', 'optInTimePeriod', optInTimePeriod);
  optInMultiplier = requireReal('CE open', 'optInMultiplier', optInMultiplier);
  requireArgument('CE open', 'inHigh', inHigh);
  requireHistory('CE open', inHigh.length);
  requireArgument('CE open', 'inLow', inLow);
  requireArgument('CE open', 'inClose', inClose);
  requireHistoryLength('CE open', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('CE open', 'inClose', inClose.length, inHigh.length);
  return ceOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInTimePeriod, optInMultiplier);
}

/**
 * {@link ceOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link ce}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Window of the highest high and lowest low, and
 *   smoothing period of the Average True Range (default 22; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param optInMultiplier - Multiplier applied to the Average True Range to offset
 *   the lines (default 3; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param outRealLong - Highest high less the multiple of the Average True Range;
 *   the stop for a long position. Must hold `historyLen - lookback` values and share
 *   no memory with an input or another output.
 * @param outRealShort - Lowest low plus the multiple of the Average True Range;
 *   the stop for a short position. Must hold `historyLen - lookback` values and
 *   share no memory with an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `ceLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty one,
 *   and `TA_BAD_PARAM` for an integer parameter that is not an int32, a real
 *   parameter that is not a number, a parameter outside its range, histories of
 *   different lengths or an output too short or starting where another output or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { ceOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const long = new Float64Array(high.length);
 * const short = new Float64Array(high.length);
 * const stream = ceOpenAndFill(high, low, close, 22, 3, long, short);
 * const { begIdx, count } = stream.outRange();
 * // long[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function ceOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  optInMultiplier: number,
  outRealLong: Float64Array,
  outRealShort: Float64Array,
  core: Core = Core.DEFAULT,
): CeStream {
  optInTimePeriod = requireInt32('CE openAndFill', 'optInTimePeriod', optInTimePeriod);
  optInMultiplier = requireReal('CE openAndFill', 'optInMultiplier', optInMultiplier);
  requireArgument('CE openAndFill', 'inHigh', inHigh);
  requireHistory('CE openAndFill', inHigh.length);
  requireArgument('CE openAndFill', 'inLow', inLow);
  requireArgument('CE openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('CE openAndFill', inHigh.length, ceLookback(optInTimePeriod, optInMultiplier, core));
  requireHistoryLength('CE openAndFill', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('CE openAndFill', 'inClose', inClose.length, inHigh.length);
  requireOutput('CE openAndFill', 'outRealLong', outRealLong, guardOutLen, Float64Array);
  requireOutput('CE openAndFill', 'outRealShort', outRealShort, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outRealLong.buffer === inHighF64.buffer && outRealLong.byteOffset === inHighF64.byteOffset) || (outRealLong.buffer === inLowF64.buffer && outRealLong.byteOffset === inLowF64.byteOffset) || (outRealLong.buffer === inCloseF64.buffer && outRealLong.byteOffset === inCloseF64.byteOffset) || (outRealShort.buffer === inHighF64.buffer && outRealShort.byteOffset === inHighF64.byteOffset) || (outRealShort.buffer === inLowF64.buffer && outRealShort.byteOffset === inLowF64.byteOffset) || (outRealShort.buffer === inCloseF64.buffer && outRealShort.byteOffset === inCloseF64.byteOffset) || (outRealLong.buffer === outRealShort.buffer && outRealLong.byteOffset === outRealShort.byteOffset)) {
    throw streamFailure('CE openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outRealLong, inHighF64) || overlaps(outRealLong, inLowF64) || overlaps(outRealLong, inCloseF64) || overlaps(outRealShort, inHighF64) || overlaps(outRealShort, inLowF64) || overlaps(outRealShort, inCloseF64) || overlaps(outRealLong, outRealShort))) {
    throw streamFailure('CE openAndFill', RetCode.BAD_PARAM);
  }
  return ceOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInTimePeriod, optInMultiplier, new MInteger(), new MInteger(), outRealLong, outRealShort);
}
