/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin
 *  MF       Mario Fortier
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090526 KL     First version (issue #342).
 *  090326 MF     Drop optInLag; the window ends at the current bar.
 */

import { badSink, clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link donchian} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Number of bars in the extrema window (default 20; range
 *   2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function donchianLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  void core;
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

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link donchian}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Number of bars in the extrema window (default 20; range
 *   2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function donchianDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (donchianLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 3) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function donchianImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
): RetCode {
  let lowest = 0;
  let highest = 0;
  let tmpLow = 0;
  let tmpHigh = 0;
  let outIdx = 0;
  let nbInitialElementNeeded = 0;
  let trailingIdx = 0;
  let lowestIdx = 0;
  let highestIdx = 0;
  let today = 0;
  let i = 0;
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
  if ((outRealUpperBand.buffer === outRealMiddleBand.buffer && outRealUpperBand.byteOffset === outRealMiddleBand.byteOffset) || (outRealUpperBand.buffer === outRealLowerBand.buffer && outRealUpperBand.byteOffset === outRealLowerBand.byteOffset) || (outRealMiddleBand.buffer === outRealLowerBand.buffer && outRealMiddleBand.byteOffset === outRealLowerBand.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  /* Donchian Channels over the optInTimePeriod bars ending at the current
   * bar:
   *
   *    Upper  = Highest High of the window
   *    Lower  = Lowest  Low  of the window
   *    Middle = (Upper + Lower) / 2
   *
   * The window includes the current bar, matching every other library and
   * charting platform. A breakout rule compares the current bar against the
   * PREVIOUS bar's band, which is where the one-bar offset belongs.
   *
   * Upper/Middle/Lower are bit-identical to MAX(high,N)/MIDPRICE(N)/MIN(low,N).
   */
  /* Identify the minimum number of price bar needed
   * to identify at least one output over the specified
   * period.
   */
  nbInitialElementNeeded = optInTimePeriod - 1;
  /* Move up the start index if there is not
   * enough initial data.
   */
  if (startIdx < nbInitialElementNeeded) {
    startIdx = nbInitialElementNeeded;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  /* Proceed with the calculation for the requested range.
   * Note that this algorithm allows the input and
   * output to be the same buffer: every position written (outIdx) sits
   * at or below trailingIdx, the oldest position any later bar reads.
   *
   * The highest high and lowest low of the window are cached with their
   * indices; the window is rescanned only when a cached extremum drops
   * out of it (same approach as MIN/MAX/WILLR and MIDPRICE).
   */
  outIdx = 0;
  today = startIdx;
  trailingIdx = startIdx - nbInitialElementNeeded;
  highestIdx = -1;
  highest = 0.0;
  lowestIdx = -1;
  lowest = 0.0;
  while (today <= endIdx) {
    tmpHigh = inHigh[today];
    tmpLow = inLow[today];
    if (highestIdx < trailingIdx) {
      highestIdx = trailingIdx;
      highest = inHigh[highestIdx];
      i = highestIdx;
      while (++i <= today) {
        tmpHigh = inHigh[i];
        if (tmpHigh > highest) {
          highestIdx = i;
          highest = tmpHigh;
        }
      }
    } else if (tmpHigh >= highest) {
      highestIdx = today;
      highest = tmpHigh;
    }
    if (lowestIdx < trailingIdx) {
      lowestIdx = trailingIdx;
      lowest = inLow[lowestIdx];
      i = lowestIdx;
      while (++i <= today) {
        tmpLow = inLow[i];
        if (tmpLow < lowest) {
          lowestIdx = i;
          lowest = tmpLow;
        }
      }
    } else if (tmpLow <= lowest) {
      lowestIdx = today;
      lowest = tmpLow;
    }
    outRealUpperBand[outIdx] = highest;
    outRealLowerBand[outIdx] = lowest;
    outRealMiddleBand[outIdx] = (highest + lowest) / 2.0;
    outIdx += 1;
    trailingIdx += 1;
    today += 1;
  }
  /* Keep the outBegIdx relative to the
   * caller input before returning.
   */
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * Donchian Channels: three overlap lines built from rolling price extrema. The
 * upper band is the highest high and the lower band the lowest low over the
 * period; the middle line is their midpoint. Richard Donchian's original four-week
 * rule — generally credited as the first published systematic trend-following
 * system — buys a break above the high of the preceding weeks and sells a break
 * below their low.
 *
 * Formula and more info at
 * [ta-lib.org/functions/donchian](https://ta-lib.org/functions/donchian).
 *
 * **Notes**
 * - The window includes the current bar, matching TradingView
 *   (`ta.highest`/`ta.lowest`), NinjaTrader, ta4j, pandas-ta and every other
 *   library that ships Donchian Channels.
 * - A breakout rule compares the current bar against the **previous** bar's band —
 *   `High[t] > Upper[t-1]` — which is where the one-bar offset belongs. Reading
 *   `Upper[t]` against `High[t]` can never signal, because `High[t]` is inside the
 *   window that produced it.
 * - Upper, Middle and Lower are bit-identical to `MAX(high, N)`, `MIDPRICE(N)` and
 *   `MIN(low, N)`. DONCHIAN computes all three in one pass under the name users
 *   look for.
 * - The middle line is the channel midpoint, not a moving average of price.
 * - No smoothing or recursion is involved, so there is no unstable period: outputs
 *   are exact from the first bar.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link donchianLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param optInTimePeriod - Number of bars in the extrema window (default 20; range
 *   2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outRealUpperBand - Highest high of the window. Must hold the values
 *   produced and never be empty; the input length always suffices.
 * @param outRealMiddleBand - Midpoint of the upper and lower bands. Must hold the
 *   values produced and never be empty; the input length always suffices.
 * @param outRealLowerBand - Lowest low of the window. Must hold the values
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
 * import { donchian } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const upperBand = new Float64Array(high.length);
 * const middleBand = new Float64Array(high.length);
 * const lowerBand = new Float64Array(high.length);
 * const { begIdx, count } = donchian(
 *   0, high.length - 1,
 *   high, low,
 *   20,
 *   upperBand, middleBand, lowerBand,
 * );
 * // upperBand[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function donchian(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('DONCHIAN', 'startIdx', startIdx);
  endIdx = requireInt32('DONCHIAN', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('DONCHIAN', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('DONCHIAN', startIdx, endIdx);
  const guardStart = clampedStart('DONCHIAN', startIdx, donchianLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('DONCHIAN', 'inHigh', inHigh, guardInLen);
  requireLength('DONCHIAN', 'inLow', inLow, guardInLen);
  requireOutput('DONCHIAN', 'outRealUpperBand', outRealUpperBand, guardOutLen, Float64Array);
  requireOutput('DONCHIAN', 'outRealMiddleBand', outRealMiddleBand, guardOutLen, Float64Array);
  requireOutput('DONCHIAN', 'outRealLowerBand', outRealLowerBand, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlapsPartly(outRealUpperBand, inHighF64) || overlapsPartly(outRealUpperBand, inLowF64) || overlapsPartly(outRealMiddleBand, inHighF64) || overlapsPartly(outRealMiddleBand, inLowF64) || overlapsPartly(outRealLowerBand, inHighF64) || overlapsPartly(outRealLowerBand, inLowF64) || overlaps(outRealUpperBand, outRealMiddleBand) || overlaps(outRealUpperBand, outRealLowerBand) || overlaps(outRealMiddleBand, outRealLowerBand)) {
    throw failure('DONCHIAN', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = donchianImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInTimePeriod, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('DONCHIAN', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live DONCHIAN stream: one value per closed bar, bit-identical to
 * {@link donchian} over the same series. Open it with {@link donchianOpen} or
 * {@link donchianOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/donchian](https://ta-lib.org/functions/donchian).
 */
export class DonchianStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  lowest: number = 0;
  /** @internal */
  highest: number = 0;
  /** @internal */
  trailingIdx: number = 0;
  /** @internal */
  lowestIdx: number = 0;
  /** @internal */
  highestIdx: number = 0;
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
  cur_outRealUpperBand: number = 0;
  /** @internal */
  cur_outRealMiddleBand: number = 0;
  /** @internal */
  cur_outRealLowerBand: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): DonchianStream {
    return new DonchianStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link donchian} reports them over the same bars. Each accepted `update` and
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
      throw failure('DONCHIAN advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
  update(inHigh: number, inLow: number, out: DonchianOut): void {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('DONCHIAN update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (typeof out !== 'object' || out === null) {
      throw badSink('DONCHIAN update', 'DonchianOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('DONCHIAN update', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    donchianStepImpl(this, inHigh, inLow);
    this.outRangeCount++;
    out.realUpperBand = this.cur_outRealUpperBand;
    out.realMiddleBand = this.cur_outRealMiddleBand;
    out.realLowerBand = this.cur_outRealLowerBand;
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
  peek(inHigh: number, inLow: number, out: DonchianOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('DONCHIAN peek', 'DonchianOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('DONCHIAN peek', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    const sp = this;
    let tmpLow = 0;
    let tmpHigh = 0;
    let cur_outRealLowerBand = 0;
    let cur_outRealMiddleBand = 0;
    let cur_outRealUpperBand = 0;
    let highest = sp.highest;
    let highestIdx = sp.highestIdx;
    let i = sp.i;
    let lowest = sp.lowest;
    let lowestIdx = sp.lowestIdx;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    let pkSlot1 = -1;
    let pkVal1 = 0;
    pkSlot0 = sp.today & sp.xMask;
    pkVal0 = inHigh;
    pkSlot1 = sp.today & sp.xMask;
    pkVal1 = inLow;
    tmpHigh = ((sp.today & sp.xMask) !== pkSlot0) ? sp.x_inHigh[sp.today & sp.xMask] : pkVal0;
    tmpLow = ((sp.today & sp.xMask) !== pkSlot1) ? sp.x_inLow[sp.today & sp.xMask] : pkVal1;
    if (highestIdx < sp.trailingIdx) {
      highestIdx = sp.trailingIdx;
      highest = ((highestIdx & sp.xMask) !== pkSlot0) ? sp.x_inHigh[highestIdx & sp.xMask] : pkVal0;
      i = highestIdx;
      while (++i <= sp.today) {
        tmpHigh = ((i & sp.xMask) !== pkSlot0) ? sp.x_inHigh[i & sp.xMask] : pkVal0;
        if (tmpHigh > highest) {
          highestIdx = i;
          highest = tmpHigh;
        }
      }
    } else if (tmpHigh >= highest) {
      highestIdx = sp.today;
      highest = tmpHigh;
    }
    if (lowestIdx < sp.trailingIdx) {
      lowestIdx = sp.trailingIdx;
      lowest = ((lowestIdx & sp.xMask) !== pkSlot1) ? sp.x_inLow[lowestIdx & sp.xMask] : pkVal1;
      i = lowestIdx;
      while (++i <= sp.today) {
        tmpLow = ((i & sp.xMask) !== pkSlot1) ? sp.x_inLow[i & sp.xMask] : pkVal1;
        if (tmpLow < lowest) {
          lowestIdx = i;
          lowest = tmpLow;
        }
      }
    } else if (tmpLow <= lowest) {
      lowestIdx = sp.today;
      lowest = tmpLow;
    }
    cur_outRealUpperBand = highest;
    cur_outRealLowerBand = lowest;
    cur_outRealMiddleBand = (highest + lowest) / 2.0;
    out.realUpperBand = cur_outRealUpperBand;
    out.realMiddleBand = cur_outRealMiddleBand;
    out.realLowerBand = cur_outRealLowerBand;
  }

  /**
   * The value at the last bar this stream counted; `peek` does not change it.
   *
   * @param out - Receives the outputs.
   */
  value(out: DonchianOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('DONCHIAN value', 'DonchianOut');
    }
    out.realUpperBand = this.cur_outRealUpperBand;
    out.realMiddleBand = this.cur_outRealMiddleBand;
    out.realLowerBand = this.cur_outRealLowerBand;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): DonchianStream {
    const c = new DonchianStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.lowest = this.lowest;
    c.highest = this.highest;
    c.trailingIdx = this.trailingIdx;
    c.lowestIdx = this.lowestIdx;
    c.highestIdx = this.highestIdx;
    c.i = this.i;
    c.today = this.today;
    c.xMask = this.xMask;
    c.x_inHigh = this.x_inHigh.slice();
    c.x_inLow = this.x_inLow.slice();
    c.cur_outRealUpperBand = this.cur_outRealUpperBand;
    c.cur_outRealMiddleBand = this.cur_outRealMiddleBand;
    c.cur_outRealLowerBand = this.cur_outRealLowerBand;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one DONCHIAN bar, written into an object the caller owns and
 * reuses: valid until the next call that writes it.
 */
export class DonchianOut {
  /** Highest high of the window. */
  realUpperBand: number = 0;
  /** Midpoint of the upper and lower bands. */
  realMiddleBand: number = 0;
  /** Lowest low of the window. */
  realLowerBand: number = 0;
}

function donchianStepImpl(sp: DonchianStream, inHigh: number, inLow: number): void {
  let tmpLow = 0;
  let tmpHigh = 0;
  sp.x_inHigh[sp.today & sp.xMask] = inHigh;
  sp.x_inLow[sp.today & sp.xMask] = inLow;
  tmpHigh = sp.x_inHigh[sp.today & sp.xMask];
  tmpLow = sp.x_inLow[sp.today & sp.xMask];
  if (sp.highestIdx < sp.trailingIdx) {
    sp.highestIdx = sp.trailingIdx;
    sp.highest = sp.x_inHigh[sp.highestIdx & sp.xMask];
    sp.i = sp.highestIdx;
    while (++sp.i <= sp.today) {
      tmpHigh = sp.x_inHigh[sp.i & sp.xMask];
      if (tmpHigh > sp.highest) {
        sp.highestIdx = sp.i;
        sp.highest = tmpHigh;
      }
    }
  } else if (tmpHigh >= sp.highest) {
    sp.highestIdx = sp.today;
    sp.highest = tmpHigh;
  }
  if (sp.lowestIdx < sp.trailingIdx) {
    sp.lowestIdx = sp.trailingIdx;
    sp.lowest = sp.x_inLow[sp.lowestIdx & sp.xMask];
    sp.i = sp.lowestIdx;
    while (++sp.i <= sp.today) {
      tmpLow = sp.x_inLow[sp.i & sp.xMask];
      if (tmpLow < sp.lowest) {
        sp.lowestIdx = sp.i;
        sp.lowest = tmpLow;
      }
    }
  } else if (tmpLow <= sp.lowest) {
    sp.lowestIdx = sp.today;
    sp.lowest = tmpLow;
  }
  sp.cur_outRealUpperBand = sp.highest;
  sp.cur_outRealLowerBand = sp.lowest;
  sp.cur_outRealMiddleBand = (sp.highest + sp.lowest) / 2.0;
  sp.trailingIdx += 1;
  sp.today += 1;
}

function donchianOpenImpl(
  sp: DonchianStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
  outStride: number,
): RetCode {
  let lowest = 0;
  let highest = 0;
  let tmpLow = 0;
  let tmpHigh = 0;
  let outIdx = 0;
  let nbInitialElementNeeded = 0;
  let trailingIdx = 0;
  let lowestIdx = 0;
  let highestIdx = 0;
  let today = 0;
  let i = 0;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Donchian Channels over the optInTimePeriod bars ending at the current
   * bar:
   *
   *    Upper  = Highest High of the window
   *    Lower  = Lowest  Low  of the window
   *    Middle = (Upper + Lower) / 2
   *
   * The window includes the current bar, matching every other library and
   * charting platform. A breakout rule compares the current bar against the
   * PREVIOUS bar's band, which is where the one-bar offset belongs.
   *
   * Upper/Middle/Lower are bit-identical to MAX(high,N)/MIDPRICE(N)/MIN(low,N).
   */
  /* Identify the minimum number of price bar needed
   * to identify at least one output over the specified
   * period.
   */
  nbInitialElementNeeded = optInTimePeriod - 1;
  /* Move up the start index if there is not
   * enough initial data.
   */
  if (startIdx < nbInitialElementNeeded) {
    startIdx = nbInitialElementNeeded;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Proceed with the calculation for the requested range.
   * Note that this algorithm allows the input and
   * output to be the same buffer: every position written (outIdx) sits
   * at or below trailingIdx, the oldest position any later bar reads.
   *
   * The highest high and lowest low of the window are cached with their
   * indices; the window is rescanned only when a cached extremum drops
   * out of it (same approach as MIN/MAX/WILLR and MIDPRICE).
   */
  outIdx = 0;
  today = startIdx;
  trailingIdx = startIdx - nbInitialElementNeeded;
  highestIdx = -1;
  highest = 0.0;
  lowestIdx = -1;
  lowest = 0.0;
  while (today <= endIdx) {
    tmpHigh = inHigh[today];
    tmpLow = inLow[today];
    if (highestIdx < trailingIdx) {
      highestIdx = trailingIdx;
      highest = inHigh[highestIdx];
      i = highestIdx;
      while (++i <= today) {
        tmpHigh = inHigh[i];
        if (tmpHigh > highest) {
          highestIdx = i;
          highest = tmpHigh;
        }
      }
    } else if (tmpHigh >= highest) {
      highestIdx = today;
      highest = tmpHigh;
    }
    if (lowestIdx < trailingIdx) {
      lowestIdx = trailingIdx;
      lowest = inLow[lowestIdx];
      i = lowestIdx;
      while (++i <= today) {
        tmpLow = inLow[i];
        if (tmpLow < lowest) {
          lowestIdx = i;
          lowest = tmpLow;
        }
      }
    } else if (tmpLow <= lowest) {
      lowestIdx = today;
      lowest = tmpLow;
    }
    outRealUpperBand[((outIdx * outStride) | 0)] = highest;
    outRealLowerBand[((outIdx * outStride) | 0)] = lowest;
    outRealMiddleBand[((outIdx * outStride) | 0)] = (highest + lowest) / 2.0;
    outIdx += 1;
    trailingIdx += 1;
    today += 1;
  }
  /* Keep the outBegIdx relative to the
   * caller input before returning.
   */
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
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
  sp.lowest = lowest;
  sp.highest = highest;
  sp.trailingIdx = trailingIdx;
  sp.lowestIdx = lowestIdx;
  sp.highestIdx = highestIdx;
  sp.i = i;
  sp.today = today;
  sp.xMask = physX - 1;
  sp.x_inHigh = capX_inHigh;
  sp.x_inLow = capX_inLow;
  sp.cur_outRealUpperBand = outRealUpperBand[(outNBElement.value - 1) * outStride];
  sp.cur_outRealMiddleBand = outRealMiddleBand[(outNBElement.value - 1) * outStride];
  sp.cur_outRealLowerBand = outRealLowerBand[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function donchianOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
): DonchianStream {
  const sp = DonchianStream.of(core);
  const retCode = donchianOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('DONCHIAN openAndFill', inHigh.length, startIdx, donchianLookback(optInTimePeriod, core));
  }
  throw streamFailure('DONCHIAN openAndFill', retCode);
}

/** @internal */
export function donchianOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): DonchianStream {
  const sp = DonchianStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outRealUpperBand = new Float64Array(1);
  const sink_outRealMiddleBand = new Float64Array(1);
  const sink_outRealLowerBand = new Float64Array(1);
  const retCode = donchianOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outRealUpperBand, sink_outRealMiddleBand, sink_outRealLowerBand, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('DONCHIAN open', inHigh.length, startIdx, donchianLookback(optInTimePeriod, core));
  }
  throw streamFailure('DONCHIAN open', retCode);
}

/**
 * Opens a live DONCHIAN stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link donchian} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Number of bars in the extrema window (default 20; range
 *   2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `donchianLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for an integer parameter that is not an int32, a parameter outside its range or
 *   histories of different lengths.
 *
 * @example
 * ```ts
 * import { DonchianOut, donchianOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = donchianOpen(high, low, 20);
 * const peeked = new DonchianOut();
 * const updated = new DonchianOut();
 * stream.peek(101.4, 99.1, peeked); // a forming bar: commits nothing
 * stream.update(101.4, 99.1, updated); // the bar closes
 * ```
 */
export function donchianOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): DonchianStream {
  optInTimePeriod = requireInt32('DONCHIAN open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('DONCHIAN open', 'inHigh', inHigh);
  requireHistory('DONCHIAN open', inHigh.length);
  requireArgument('DONCHIAN open', 'inLow', inLow);
  requireHistoryLength('DONCHIAN open', 'inLow', inLow.length, inHigh.length);
  return donchianOpenInternal(core, float64(inHigh), float64(inLow), 0, optInTimePeriod);
}

/**
 * {@link donchianOpen} that also fills the outputs over the whole history in the
 * same pass, bit-identically to {@link donchian}. The range written is the
 * stream's `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Number of bars in the extrema window (default 20; range
 *   2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outRealUpperBand - Highest high of the window. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param outRealMiddleBand - Midpoint of the upper and lower bands. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param outRealLowerBand - Lowest low of the window. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `donchianLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or an output too short or starting where another
 *   output or a `Float64Array` input starts, or, on a history long enough to open,
 *   one sharing memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { donchianOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const upperBand = new Float64Array(high.length);
 * const middleBand = new Float64Array(high.length);
 * const lowerBand = new Float64Array(high.length);
 * const stream = donchianOpenAndFill(
 *   high, low,
 *   20,
 *   upperBand, middleBand, lowerBand,
 * );
 * const { begIdx, count } = stream.outRange();
 * // upperBand[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function donchianOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
  core: Core = Core.DEFAULT,
): DonchianStream {
  optInTimePeriod = requireInt32('DONCHIAN openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('DONCHIAN openAndFill', 'inHigh', inHigh);
  requireHistory('DONCHIAN openAndFill', inHigh.length);
  requireArgument('DONCHIAN openAndFill', 'inLow', inLow);
  const guardOutLen = openFillCount('DONCHIAN openAndFill', inHigh.length, donchianLookback(optInTimePeriod, core));
  requireHistoryLength('DONCHIAN openAndFill', 'inLow', inLow.length, inHigh.length);
  requireOutput('DONCHIAN openAndFill', 'outRealUpperBand', outRealUpperBand, guardOutLen, Float64Array);
  requireOutput('DONCHIAN openAndFill', 'outRealMiddleBand', outRealMiddleBand, guardOutLen, Float64Array);
  requireOutput('DONCHIAN openAndFill', 'outRealLowerBand', outRealLowerBand, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if ((outRealUpperBand.buffer === inHighF64.buffer && outRealUpperBand.byteOffset === inHighF64.byteOffset) || (outRealUpperBand.buffer === inLowF64.buffer && outRealUpperBand.byteOffset === inLowF64.byteOffset) || (outRealMiddleBand.buffer === inHighF64.buffer && outRealMiddleBand.byteOffset === inHighF64.byteOffset) || (outRealMiddleBand.buffer === inLowF64.buffer && outRealMiddleBand.byteOffset === inLowF64.byteOffset) || (outRealLowerBand.buffer === inHighF64.buffer && outRealLowerBand.byteOffset === inHighF64.byteOffset) || (outRealLowerBand.buffer === inLowF64.buffer && outRealLowerBand.byteOffset === inLowF64.byteOffset) || (outRealUpperBand.buffer === outRealMiddleBand.buffer && outRealUpperBand.byteOffset === outRealMiddleBand.byteOffset) || (outRealUpperBand.buffer === outRealLowerBand.buffer && outRealUpperBand.byteOffset === outRealLowerBand.byteOffset) || (outRealMiddleBand.buffer === outRealLowerBand.buffer && outRealMiddleBand.byteOffset === outRealLowerBand.byteOffset)) {
    throw streamFailure('DONCHIAN openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outRealUpperBand, inHighF64) || overlaps(outRealUpperBand, inLowF64) || overlaps(outRealMiddleBand, inHighF64) || overlaps(outRealMiddleBand, inLowF64) || overlaps(outRealLowerBand, inHighF64) || overlaps(outRealLowerBand, inLowF64) || overlaps(outRealUpperBand, outRealMiddleBand) || overlaps(outRealUpperBand, outRealLowerBand) || overlaps(outRealMiddleBand, outRealLowerBand))) {
    throw streamFailure('DONCHIAN openAndFill', RetCode.BAD_PARAM);
  }
  return donchianOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outRealUpperBand, outRealMiddleBand, outRealLowerBand);
}
