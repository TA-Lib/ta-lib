/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  100626 KL,CC  Creation (#485).
 *  100626 MF,CC  Batch tier: block scan of the channel (#485).
 *  100626 MF,CC  Auto rule sized on the slope under the limit (#485).
 */

import { FuncUnstId } from '../enums.ts';
import { badSink, clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link fisher} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param optInTimePeriod - Number of bars in the channel the midpoint is located
 *   in (default 10; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function fisherLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 10;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  /* The smoothed values of two starts differ by 0.67 times less each bar on
   * any input, 2.5 bars per e-fold, while the limiter treats both alike.
   * The 6 e-folds added to K are the gain from that difference to the
   * outputs, against the largest difference the seed causes: the
   * transform's slope at the limit, 1/(1 - 0.99^2), about 50, times
   * 0.67/(0.67 - 0.5) for its own 0.5 pole, plus a bar for the trigger.
   * Not a bound when the limiter moves one start alone: 0.999 is fed back,
   * and a channel position that stays above 0.986 keeps it there.
   */
  return optInTimePeriod - 1 + core.unstableCount(FuncUnstId.FISHER, (((((5 * (10 + 6)) | 0) + 1) / 2) | 0), (((((5 * (19 + 6)) | 0) + 1) / 2) | 0));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link fisher}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Number of bars in the channel the midpoint is located
 *   in (default 10; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function fisherDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (fisherLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 2) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function fisherImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outFisher: Float64Array,
  outTrigger: Float64Array,
): RetCode {
  let sufHighest!: Float64Array;
  let preHighest!: Float64Array;
  let sufLowest!: Float64Array;
  let preLowest!: Float64Array;
  let price = 0;
  let highest = 0;
  let lowest = 0;
  let ratio = 0;
  let smoothed = 0;
  let fish = 0;
  let prevFish = 0;
  let tempReal = 0;
  let tempHigh = 0;
  let tempLow = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let unstablePeriod = 0;
  let nbInitialElementNeeded = 0;
  let today = 0;
  let i = 0;
  let m = 0;
  let blockStart = 0;
  let blockNext = 0;
  let nAvail = 0;
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
  if ((outFisher.buffer === outTrigger.buffer && outFisher.byteOffset === outTrigger.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  nbInitialElementNeeded = fisherLookback(optInTimePeriod, core);
  if (startIdx < nbInitialElementNeeded) {
    startIdx = nbInitialElementNeeded;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  lookbackTotal = optInTimePeriod - 1;
  unstablePeriod = nbInitialElementNeeded - lookbackTotal;
  /* Same values as fisher_ALT1 below, which carries the formula. Only the
   * channel differs: a Van Herk / Gil-Werman block scan (WILLR's, issue
   * #147) over the midpoints, so the cost per bar does not depend on the
   * period or on the shape of the input, where the cached extremum of
   * fisher_ALT1 rescans its whole window on every bar of a flat or
   * trending stretch. Every scratch array holds copies, so an output may
   * alias an input.
   */
  smoothed = 0.0;
  prevFish = 0.0;
  today = startIdx - unstablePeriod;
  blockStart = today - lookbackTotal;
  outIdx = 0;
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sufHighest = new Float64Array(optInTimePeriod);
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  preHighest = new Float64Array(optInTimePeriod);
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sufLowest = new Float64Array(optInTimePeriod);
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  preLowest = new Float64Array(optInTimePeriod);
  while (today <= endIdx) {
    /* Suffix extrema of the block [blockStart, today]. */
    i = today;
    tempHigh = inHigh[i];
    tempLow = inLow[i];
    highest = (tempHigh + tempLow) / 2.0;
    lowest = highest;
    sufHighest[optInTimePeriod - 1] = highest;
    sufLowest[optInTimePeriod - 1] = lowest;
    while (i > blockStart) {
      i -= 1;
      tempHigh = inHigh[i];
      tempLow = inLow[i];
      tempReal = (tempHigh + tempLow) / 2.0;
      if (tempReal > highest) {
        highest = tempReal;
      }
      if (tempReal < lowest) {
        lowest = tempReal;
      }
      sufHighest[i - blockStart] = highest;
      sufLowest[i - blockStart] = lowest;
    }
    /* Prefix extrema of the next block, clamped to what remains, stored
     * one slot up: slot 0 repeats the suffix so that bar 'today', whose
     * window is the block itself, runs the same combine as the others.
     */
    blockNext = blockStart + optInTimePeriod;
    nAvail = endIdx + 1 - blockNext;
    if (nAvail > optInTimePeriod - 1) {
      nAvail = optInTimePeriod - 1;
    }
    preHighest[0] = sufHighest[0];
    preLowest[0] = sufLowest[0];
    if (nAvail > 0) {
      tempHigh = inHigh[blockNext];
      tempLow = inLow[blockNext];
      highest = (tempHigh + tempLow) / 2.0;
      lowest = highest;
      preHighest[1] = highest;
      preLowest[1] = lowest;
      i = 1;
      while (i < nAvail) {
        tempHigh = inHigh[blockNext + i];
        tempLow = inLow[blockNext + i];
        tempReal = (tempHigh + tempLow) / 2.0;
        if (tempReal > highest) {
          highest = tempReal;
        }
        if (tempReal < lowest) {
          lowest = tempReal;
        }
        preHighest[i + 1] = highest;
        preLowest[i + 1] = lowest;
        i += 1;
      }
    }
    m = 0;
    while (m <= nAvail) {
      highest = sufHighest[m];
      if (preHighest[m] > highest) {
        highest = preHighest[m];
      }
      lowest = sufLowest[m];
      if (preLowest[m] < lowest) {
        lowest = preLowest[m];
      }
      tempHigh = inHigh[today + m];
      tempLow = inLow[today + m];
      price = (tempHigh + tempLow) / 2.0;
      tempReal = highest - lowest;
      if (!(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(highest) + Math.abs(lowest)))) {
        ratio = (price - lowest) / tempReal;
      } else {
        ratio = 0.5;
      }
      smoothed = fma(0.67, smoothed, 0.33 * 2.0 * (ratio - 0.5));
      if (smoothed > 0.99) {
        smoothed = 0.999;
      }
      if (smoothed < -0.99) {
        smoothed = -0.999;
      }
      fish = fma(0.5, Math.log((1.0 + smoothed) / (1.0 - smoothed)), 0.5 * prevFish);
      if (today + m >= startIdx) {
        outFisher[outIdx] = fish;
        outTrigger[outIdx] = prevFish;
        outIdx = outIdx + 1;
      }
      prevFish = fish;
      m += 1;
    }
    today = today + nAvail + 1;
    blockStart = blockNext;
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/**
 * Ehlers' Fisher Transform: an oscillator that reshapes the midpoint's position in
 * its rolling channel into a near-normal distribution. Extreme values become rare
 * and turning points sharp, where a plain channel position spends much of its time
 * pinned near the edges. The output is unbounded and centred on zero. A peak or
 * trough marks a likely turn, and the Fisher line crossing its Trigger, the same
 * line one bar later, is the author's entry signal.
 *
 * Formula and more info at
 * [ta-lib.org/functions/fisher](https://ta-lib.org/functions/fisher).
 *
 * **Notes**
 * - A window whose midpoints are all equal takes the neutral position, so a market
 *   that does not move decays toward 0. The original divides by the zero range
 *   there.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link fisherLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param optInTimePeriod - Number of bars in the channel the midpoint is located
 *   in (default 10; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outFisher - Fisher Transform value. Must hold the values produced and
 *   never be empty; the input length always suffices.
 * @param outTrigger - Fisher value of the previous bar. Must hold the values
 *   produced and never be empty; the input length always suffices.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The range written: `begIdx` is the first bar with a value, `count` how
 *   many were written.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for an argument that is not an int32, a
 *   parameter outside its range, an array too short for the range, an empty output,
 *   two outputs sharing memory or an output sharing memory with a `Float64Array`
 *   input it is not. Checked before anything is written.
 *
 * @example
 * ```ts
 * import { fisher } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const fisherOut = new Float64Array(high.length);
 * const trigger = new Float64Array(high.length);
 * const { begIdx, count } = fisher(
 *   0, high.length - 1,
 *   high, low,
 *   10,
 *   fisherOut, trigger,
 * );
 * // fisherOut[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`stochf`](https://ta-lib.org/functions/stochf)
 * @see [`willr`](https://ta-lib.org/functions/willr)
 * @see [`midprice`](https://ta-lib.org/functions/midprice)
 * @see [`ibs`](https://ta-lib.org/functions/ibs)
 */
export function fisher(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  outFisher: Float64Array,
  outTrigger: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('FISHER', 'startIdx', startIdx);
  endIdx = requireInt32('FISHER', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('FISHER', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('FISHER', startIdx, endIdx);
  const guardStart = clampedStart('FISHER', startIdx, fisherLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('FISHER', 'inHigh', inHigh, guardInLen);
  requireLength('FISHER', 'inLow', inLow, guardInLen);
  requireOutput('FISHER', 'outFisher', outFisher, guardOutLen, Float64Array);
  requireOutput('FISHER', 'outTrigger', outTrigger, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlapsPartly(outFisher, inHighF64) || overlapsPartly(outFisher, inLowF64) || overlapsPartly(outTrigger, inHighF64) || overlapsPartly(outTrigger, inLowF64) || overlaps(outFisher, outTrigger)) {
    throw failure('FISHER', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = fisherImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInTimePeriod, outBegIdx, outNBElement, outFisher, outTrigger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('FISHER', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/* Using fisher_ALT1 for TA_ALT={STREAM,ALL_LANGUAGES} */

/**
 * A live FISHER stream: one value per closed bar, bit-identical to {@link fisher}
 * over the same series. Open it with {@link fisherOpen} or
 * {@link fisherOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/fisher](https://ta-lib.org/functions/fisher).
 */
export class FisherStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  highest: number = 0;
  /** @internal */
  lowest: number = 0;
  /** @internal */
  smoothed: number = 0;
  /** @internal */
  prevFish: number = 0;
  /** @internal */
  trailingIdx: number = 0;
  /** @internal */
  highestIdx: number = 0;
  /** @internal */
  lowestIdx: number = 0;
  /** @internal */
  i: number = 0;
  /** @internal */
  today: number = 0;
  /** @internal */
  xMask: number = 0;
  /** @internal */
  x_inHigh: Float64Array = new Float64Array(1);
  /** @internal */
  x_inLow: Float64Array = new Float64Array(1);
  /** @internal */
  cur_outFisher: number = 0;
  /** @internal */
  cur_outTrigger: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): FisherStream {
    return new FisherStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link fisher} reports them over the same bars. Each accepted `update` and each
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
      throw failure('FISHER advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar, writing the new values into `out`.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number, out: FisherOut): void {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('FISHER update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (typeof out !== 'object' || out === null) {
      throw badSink('FISHER update', 'FisherOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('FISHER update', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    fisherStepImpl(this, inHigh, inLow);
    this.outRangeCount++;
    out.fisher = this.cur_outFisher;
    out.trigger = this.cur_outTrigger;
  }

  /**
   * Writes into `out` what the next `update` with the same bar would write,
   * committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number, out: FisherOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('FISHER peek', 'FisherOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('FISHER peek', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    const sp = this;
    let price = 0;
    let ratio = 0;
    let fish = 0;
    let tempReal = 0;
    let tempHigh = 0;
    let tempLow = 0;
    let cur_outFisher = 0;
    let cur_outTrigger = 0;
    let highest = sp.highest;
    let highestIdx = sp.highestIdx;
    let i = sp.i;
    let lowest = sp.lowest;
    let lowestIdx = sp.lowestIdx;
    let smoothed = sp.smoothed;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    let pkSlot1 = -1;
    let pkVal1 = 0;
    pkSlot0 = sp.today & sp.xMask;
    pkVal0 = inHigh;
    pkSlot1 = sp.today & sp.xMask;
    pkVal1 = inLow;
    /* The channel, over the midpoints rather than over the highs and the
     * lows separately: this indicator reads one series, which happens to be
     * (H+L)/2, so both extremes come from that same series. STOCH's shape,
     * with the midpoint recomputed on the rare rescan rather than held in a
     * buffer the caller would have to own.
     */
    price = ((((sp.today & sp.xMask) !== pkSlot0) ? sp.x_inHigh[sp.today & sp.xMask] : pkVal0) + (((sp.today & sp.xMask) !== pkSlot1) ? sp.x_inLow[sp.today & sp.xMask] : pkVal1)) / 2.0;
    if (highestIdx < sp.trailingIdx) {
      highestIdx = sp.trailingIdx;
      tempHigh = ((highestIdx & sp.xMask) !== pkSlot0) ? sp.x_inHigh[highestIdx & sp.xMask] : pkVal0;
      tempLow = ((highestIdx & sp.xMask) !== pkSlot1) ? sp.x_inLow[highestIdx & sp.xMask] : pkVal1;
      highest = (tempHigh + tempLow) / 2.0;
      i = highestIdx;
      while (++i <= sp.today) {
        tempHigh = ((i & sp.xMask) !== pkSlot0) ? sp.x_inHigh[i & sp.xMask] : pkVal0;
        tempLow = ((i & sp.xMask) !== pkSlot1) ? sp.x_inLow[i & sp.xMask] : pkVal1;
        tempReal = (tempHigh + tempLow) / 2.0;
        if (tempReal > highest) {
          highestIdx = i;
          highest = tempReal;
        }
      }
    } else if (price >= highest) {
      highestIdx = sp.today;
      highest = price;
    }
    if (lowestIdx < sp.trailingIdx) {
      lowestIdx = sp.trailingIdx;
      tempHigh = ((lowestIdx & sp.xMask) !== pkSlot0) ? sp.x_inHigh[lowestIdx & sp.xMask] : pkVal0;
      tempLow = ((lowestIdx & sp.xMask) !== pkSlot1) ? sp.x_inLow[lowestIdx & sp.xMask] : pkVal1;
      lowest = (tempHigh + tempLow) / 2.0;
      i = lowestIdx;
      while (++i <= sp.today) {
        tempHigh = ((i & sp.xMask) !== pkSlot0) ? sp.x_inHigh[i & sp.xMask] : pkVal0;
        tempLow = ((i & sp.xMask) !== pkSlot1) ? sp.x_inLow[i & sp.xMask] : pkVal1;
        tempReal = (tempHigh + tempLow) / 2.0;
        if (tempReal < lowest) {
          lowestIdx = i;
          lowest = tempReal;
        }
      }
    } else if (price <= lowest) {
      lowestIdx = sp.today;
      lowest = price;
    }
    /* A flat window answers the neutral position rather than dividing by
     * its own zero range. The band is the range against its own two
     * extremes, STOCH's test: a fixed constant answers "flat" for every
     * window of an instrument quoted below it (#253), and an exact test
     * divides a machine-flat window into noise (#107). At 0.5 the bar
     * contributes nothing and both recursions decay on their own
     * coefficients.
     */
    tempReal = highest - lowest;
    if (!(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(highest) + Math.abs(lowest)))) {
      ratio = (price - lowest) / tempReal;
    } else {
      ratio = 0.5;
    }
    /* The listing's .33*2*(r-.5) + .67*Value1[1]. */
    smoothed = fma(0.67, smoothed, 0.33 * 2.0 * (ratio - 0.5));
    /* The clamp is what keeps atanh finite, and the CLAMPED smoothed is what
     * the next bar's smoothing reads -- the listing assigns it back to
     * Value1 rather than holding it for the transform alone.
     */
    if (smoothed > 0.99) {
      smoothed = 0.999;
    }
    if (smoothed < -0.99) {
      smoothed = -0.999;
    }
    fish = fma(0.5, Math.log((1.0 + smoothed) / (1.0 - smoothed)), 0.5 * sp.prevFish);
    cur_outFisher = fish;
    /* The author's second plot is Fish[1]: the previous bar's smoothed,
     * which at the first output bar is the zero seed when no unstable
     * period was discarded, and the computed smoothed of the bar before it
     * when one was.
     */
    cur_outTrigger = sp.prevFish;
    out.fisher = cur_outFisher;
    out.trigger = cur_outTrigger;
  }

  /**
   * The value at the last bar this stream counted; `peek` does not change it.
   *
   * @param out - Receives the outputs.
   */
  value(out: FisherOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('FISHER value', 'FisherOut');
    }
    out.fisher = this.cur_outFisher;
    out.trigger = this.cur_outTrigger;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): FisherStream {
    const c = new FisherStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.highest = this.highest;
    c.lowest = this.lowest;
    c.smoothed = this.smoothed;
    c.prevFish = this.prevFish;
    c.trailingIdx = this.trailingIdx;
    c.highestIdx = this.highestIdx;
    c.lowestIdx = this.lowestIdx;
    c.i = this.i;
    c.today = this.today;
    c.xMask = this.xMask;
    c.x_inHigh = this.x_inHigh.slice();
    c.x_inLow = this.x_inLow.slice();
    c.cur_outFisher = this.cur_outFisher;
    c.cur_outTrigger = this.cur_outTrigger;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one FISHER bar, written into an object the caller owns and
 * reuses: valid until the next call that writes it.
 */
export class FisherOut {
  /** Fisher Transform value. */
  fisher: number = 0;
  /** Fisher value of the previous bar. */
  trigger: number = 0;
}

function fisherStepImpl(sp: FisherStream, inHigh: number, inLow: number): void {
  let price = 0;
  let ratio = 0;
  let fish = 0;
  let tempReal = 0;
  let tempHigh = 0;
  let tempLow = 0;
  sp.x_inHigh[sp.today & sp.xMask] = inHigh;
  sp.x_inLow[sp.today & sp.xMask] = inLow;
  /* The channel, over the midpoints rather than over the highs and the
   * lows separately: this indicator reads one series, which happens to be
   * (H+L)/2, so both extremes come from that same series. STOCH's shape,
   * with the midpoint recomputed on the rare rescan rather than held in a
   * buffer the caller would have to own.
   */
  price = (sp.x_inHigh[sp.today & sp.xMask] + sp.x_inLow[sp.today & sp.xMask]) / 2.0;
  if (sp.highestIdx < sp.trailingIdx) {
    sp.highestIdx = sp.trailingIdx;
    tempHigh = sp.x_inHigh[sp.highestIdx & sp.xMask];
    tempLow = sp.x_inLow[sp.highestIdx & sp.xMask];
    sp.highest = (tempHigh + tempLow) / 2.0;
    sp.i = sp.highestIdx;
    while (++sp.i <= sp.today) {
      tempHigh = sp.x_inHigh[sp.i & sp.xMask];
      tempLow = sp.x_inLow[sp.i & sp.xMask];
      tempReal = (tempHigh + tempLow) / 2.0;
      if (tempReal > sp.highest) {
        sp.highestIdx = sp.i;
        sp.highest = tempReal;
      }
    }
  } else if (price >= sp.highest) {
    sp.highestIdx = sp.today;
    sp.highest = price;
  }
  if (sp.lowestIdx < sp.trailingIdx) {
    sp.lowestIdx = sp.trailingIdx;
    tempHigh = sp.x_inHigh[sp.lowestIdx & sp.xMask];
    tempLow = sp.x_inLow[sp.lowestIdx & sp.xMask];
    sp.lowest = (tempHigh + tempLow) / 2.0;
    sp.i = sp.lowestIdx;
    while (++sp.i <= sp.today) {
      tempHigh = sp.x_inHigh[sp.i & sp.xMask];
      tempLow = sp.x_inLow[sp.i & sp.xMask];
      tempReal = (tempHigh + tempLow) / 2.0;
      if (tempReal < sp.lowest) {
        sp.lowestIdx = sp.i;
        sp.lowest = tempReal;
      }
    }
  } else if (price <= sp.lowest) {
    sp.lowestIdx = sp.today;
    sp.lowest = price;
  }
  /* A flat window answers the neutral position rather than dividing by
   * its own zero range. The band is the range against its own two
   * extremes, STOCH's test: a fixed constant answers "flat" for every
   * window of an instrument quoted below it (#253), and an exact test
   * divides a machine-flat window into noise (#107). At 0.5 the bar
   * contributes nothing and both recursions decay on their own
   * coefficients.
   */
  tempReal = sp.highest - sp.lowest;
  if (!(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(sp.highest) + Math.abs(sp.lowest)))) {
    ratio = (price - sp.lowest) / tempReal;
  } else {
    ratio = 0.5;
  }
  /* The listing's .33*2*(r-.5) + .67*Value1[1]. */
  sp.smoothed = fma(0.67, sp.smoothed, 0.33 * 2.0 * (ratio - 0.5));
  /* The clamp is what keeps atanh finite, and the CLAMPED smoothed is what
   * the next bar's smoothing reads -- the listing assigns it back to
   * Value1 rather than holding it for the transform alone.
   */
  if (sp.smoothed > 0.99) {
    sp.smoothed = 0.999;
  }
  if (sp.smoothed < -0.99) {
    sp.smoothed = -0.999;
  }
  fish = fma(0.5, Math.log((1.0 + sp.smoothed) / (1.0 - sp.smoothed)), 0.5 * sp.prevFish);
  sp.cur_outFisher = fish;
  /* The author's second plot is Fish[1]: the previous bar's smoothed,
   * which at the first output bar is the zero seed when no unstable
   * period was discarded, and the computed smoothed of the bar before it
   * when one was.
   */
  sp.cur_outTrigger = sp.prevFish;
  sp.prevFish = fish;
  sp.trailingIdx += 1;
  sp.today += 1;
}

function fisherOpenImpl(
  sp: FisherStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outFisher: Float64Array,
  outTrigger: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let price = 0;
  let highest = 0;
  let lowest = 0;
  let ratio = 0;
  let smoothed = 0;
  let fish = 0;
  let prevFish = 0;
  let tempReal = 0;
  let tempHigh = 0;
  let tempLow = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let unstablePeriod = 0;
  let nbInitialElementNeeded = 0;
  let today = 0;
  let trailingIdx = 0;
  let highestIdx = 0;
  let lowestIdx = 0;
  let i = 0;
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
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  nbInitialElementNeeded = fisherLookback(optInTimePeriod, core);
  if (startIdx < nbInitialElementNeeded) {
    startIdx = nbInitialElementNeeded;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  lookbackTotal = optInTimePeriod - 1;
  unstablePeriod = nbInitialElementNeeded - lookbackTotal;
  /* John F. Ehlers, "Using The Fisher Transform", Stocks & Commodities
   * V.20:11 (November 2002), pp.40-42, the EasyLanguage listing in Figure 4.
   *
   * The bar's midpoint is located in its rolling n-bar channel, rescaled to
   * (-1, +1), smoothed, clamped, and passed through atanh. What the
   * transform buys is the tail: a channel position is close to uniformly
   * distributed, and the Fisher transform of a uniform variable is close to
   * normal, so an extreme reading is rare rather than routine and a turn is
   * a sharp corner rather than a drift.
   *
   * Both recursions start from the author's zero seed and decay at their own
   * coefficient -- 0.67 for the smoothing, 0.5 for the transform -- so the
   * first bars carry the seed rather than the series. That is what the
   * unstable period discards.
   */
  smoothed = 0.0;
  prevFish = 0.0;
  today = startIdx - unstablePeriod;
  trailingIdx = today - lookbackTotal;
  highestIdx = -1;
  highest = 0.0;
  lowestIdx = -1;
  lowest = 0.0;
  outIdx = 0;
  while (today <= endIdx) {
    /* The channel, over the midpoints rather than over the highs and the
     * lows separately: this indicator reads one series, which happens to be
     * (H+L)/2, so both extremes come from that same series. STOCH's shape,
     * with the midpoint recomputed on the rare rescan rather than held in a
     * buffer the caller would have to own.
     */
    price = (inHigh[today] + inLow[today]) / 2.0;
    if (highestIdx < trailingIdx) {
      highestIdx = trailingIdx;
      tempHigh = inHigh[highestIdx];
      tempLow = inLow[highestIdx];
      highest = (tempHigh + tempLow) / 2.0;
      i = highestIdx;
      while (++i <= today) {
        tempHigh = inHigh[i];
        tempLow = inLow[i];
        tempReal = (tempHigh + tempLow) / 2.0;
        if (tempReal > highest) {
          highestIdx = i;
          highest = tempReal;
        }
      }
    } else if (price >= highest) {
      highestIdx = today;
      highest = price;
    }
    if (lowestIdx < trailingIdx) {
      lowestIdx = trailingIdx;
      tempHigh = inHigh[lowestIdx];
      tempLow = inLow[lowestIdx];
      lowest = (tempHigh + tempLow) / 2.0;
      i = lowestIdx;
      while (++i <= today) {
        tempHigh = inHigh[i];
        tempLow = inLow[i];
        tempReal = (tempHigh + tempLow) / 2.0;
        if (tempReal < lowest) {
          lowestIdx = i;
          lowest = tempReal;
        }
      }
    } else if (price <= lowest) {
      lowestIdx = today;
      lowest = price;
    }
    /* A flat window answers the neutral position rather than dividing by
     * its own zero range. The band is the range against its own two
     * extremes, STOCH's test: a fixed constant answers "flat" for every
     * window of an instrument quoted below it (#253), and an exact test
     * divides a machine-flat window into noise (#107). At 0.5 the bar
     * contributes nothing and both recursions decay on their own
     * coefficients.
     */
    tempReal = highest - lowest;
    if (!(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(highest) + Math.abs(lowest)))) {
      ratio = (price - lowest) / tempReal;
    } else {
      ratio = 0.5;
    }
    /* The listing's .33*2*(r-.5) + .67*Value1[1]. */
    smoothed = fma(0.67, smoothed, 0.33 * 2.0 * (ratio - 0.5));
    /* The clamp is what keeps atanh finite, and the CLAMPED smoothed is what
     * the next bar's smoothing reads -- the listing assigns it back to
     * Value1 rather than holding it for the transform alone.
     */
    if (smoothed > 0.99) {
      smoothed = 0.999;
    }
    if (smoothed < -0.99) {
      smoothed = -0.999;
    }
    fish = fma(0.5, Math.log((1.0 + smoothed) / (1.0 - smoothed)), 0.5 * prevFish);
    if (today >= startIdx) {
      outFisher[((outIdx * outStride) | 0)] = fish;
      /* The author's second plot is Fish[1]: the previous bar's smoothed,
       * which at the first output bar is the zero seed when no unstable
       * period was discarded, and the computed smoothed of the bar before it
       * when one was.
       */
      outTrigger[((outIdx * outStride) | 0)] = prevFish;
      outIdx = outIdx + 1;
    }
    prevFish = fish;
    trailingIdx += 1;
    today += 1;
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const capX = today - trailingIdx + 1;
  if (capX < 1 || capX > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  let physX = 1;
  while (physX < capX) {
    physX <<= 1;
  }
  const capX_inHigh = new Float64Array(physX);
  const capX_inLow = new Float64Array(physX);
  for (let fillJ = historyLen - capX; fillJ < historyLen; fillJ++) {
    capX_inHigh[fillJ & (physX - 1)] = inHigh[fillJ];
    capX_inLow[fillJ & (physX - 1)] = inLow[fillJ];
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.highest = highest;
  sp.lowest = lowest;
  sp.smoothed = smoothed;
  sp.prevFish = prevFish;
  sp.trailingIdx = trailingIdx;
  sp.highestIdx = highestIdx;
  sp.lowestIdx = lowestIdx;
  sp.i = i;
  sp.today = today;
  sp.xMask = physX - 1;
  sp.x_inHigh = capX_inHigh;
  sp.x_inLow = capX_inLow;
  sp.cur_outFisher = outFisher[(outNBElement.value - 1) * outStride];
  sp.cur_outTrigger = outTrigger[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function fisherOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outFisher: Float64Array,
  outTrigger: Float64Array,
): FisherStream {
  const sp = FisherStream.of(core);
  const retCode = fisherOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, outFisher, outTrigger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('FISHER openAndFill', inHigh.length, startIdx, fisherLookback(optInTimePeriod, core));
  }
  throw streamFailure('FISHER openAndFill', retCode);
}

/** @internal */
export function fisherOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): FisherStream {
  const sp = FisherStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outFisher = new Float64Array(1);
  const sink_outTrigger = new Float64Array(1);
  const retCode = fisherOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outFisher, sink_outTrigger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('FISHER open', inHigh.length, startIdx, fisherLookback(optInTimePeriod, core));
  }
  throw streamFailure('FISHER open', retCode);
}

/**
 * Opens a live FISHER stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link fisher} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Number of bars in the channel the midpoint is located
 *   in (default 10; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `fisherLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { FisherOut, fisherOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = fisherOpen(high, low, 10);
 * const peeked = new FisherOut();
 * const updated = new FisherOut();
 * stream.peek(101.4, 99.1, peeked); // a forming bar: commits nothing
 * stream.update(101.4, 99.1, updated); // the bar closes
 * ```
 */
export function fisherOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): FisherStream {
  optInTimePeriod = requireInt32('FISHER open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('FISHER open', 'inHigh', inHigh);
  requireHistory('FISHER open', inHigh.length);
  requireArgument('FISHER open', 'inLow', inLow);
  requireHistoryLength('FISHER open', 'inLow', inLow.length, inHigh.length);
  return fisherOpenInternal(core, float64(inHigh), float64(inLow), 0, optInTimePeriod);
}

/**
 * {@link fisherOpen} that also fills the outputs over the whole history in the
 * same pass, bit-identically to {@link fisher}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Number of bars in the channel the midpoint is located
 *   in (default 10; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outFisher - Fisher Transform value. Must hold `historyLen - lookback`
 *   values and share no memory with an input or another output.
 * @param outTrigger - Fisher value of the previous bar. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `fisherLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range, histories of different lengths or an output too
 *   short or starting where another output or a `Float64Array` input starts, or, on
 *   a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { fisherOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const fisherOut = new Float64Array(high.length);
 * const trigger = new Float64Array(high.length);
 * const stream = fisherOpenAndFill(high, low, 10, fisherOut, trigger);
 * const { begIdx, count } = stream.outRange();
 * // fisherOut[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function fisherOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  outFisher: Float64Array,
  outTrigger: Float64Array,
  core: Core = Core.DEFAULT,
): FisherStream {
  optInTimePeriod = requireInt32('FISHER openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('FISHER openAndFill', 'inHigh', inHigh);
  requireHistory('FISHER openAndFill', inHigh.length);
  requireArgument('FISHER openAndFill', 'inLow', inLow);
  const guardOutLen = openFillCount('FISHER openAndFill', inHigh.length, fisherLookback(optInTimePeriod, core));
  requireHistoryLength('FISHER openAndFill', 'inLow', inLow.length, inHigh.length);
  requireOutput('FISHER openAndFill', 'outFisher', outFisher, guardOutLen, Float64Array);
  requireOutput('FISHER openAndFill', 'outTrigger', outTrigger, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if ((outFisher.buffer === inHighF64.buffer && outFisher.byteOffset === inHighF64.byteOffset) || (outFisher.buffer === inLowF64.buffer && outFisher.byteOffset === inLowF64.byteOffset) || (outTrigger.buffer === inHighF64.buffer && outTrigger.byteOffset === inHighF64.byteOffset) || (outTrigger.buffer === inLowF64.buffer && outTrigger.byteOffset === inLowF64.byteOffset) || (outFisher.buffer === outTrigger.buffer && outFisher.byteOffset === outTrigger.byteOffset)) {
    throw streamFailure('FISHER openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outFisher, inHighF64) || overlaps(outFisher, inLowF64) || overlaps(outTrigger, inHighF64) || overlaps(outTrigger, inLowF64) || overlaps(outFisher, outTrigger))) {
    throw streamFailure('FISHER openAndFill', RetCode.BAD_PARAM);
  }
  return fisherOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outFisher, outTrigger);
}
