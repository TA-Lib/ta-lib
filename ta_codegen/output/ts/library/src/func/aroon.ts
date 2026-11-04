/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AM       Adrian Michel <michel@pacbell.net>
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120802 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  050703 MF   Fix algorithm base on Adrian Michel bug report #748163
 */

import { badSink, clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link aroon} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Lookback window length (default 14; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function aroonLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  void core;
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

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link aroon}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Lookback window length (default 14; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function aroonDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (aroonLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 2) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function aroonImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outAroonDown: Float64Array,
  outAroonUp: Float64Array,
): RetCode {
  let lowest = 0;
  let highest = 0;
  let tmp = 0;
  let factor = 0;
  let outIdx = 0;
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
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if ((outAroonDown.buffer === outAroonUp.buffer && outAroonDown.byteOffset === outAroonUp.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  /* This function is using a speed optimized algorithm
   * for the min/max logic.
   *
   * You might want to first look at how TA_MIN/TA_MAX works
   * and this function will become easier to understand.
   */
  /* Move up the start index if there is not
   * enough initial data.
   */
  if (startIdx < optInTimePeriod) {
    startIdx = optInTimePeriod;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  /* Proceed with the calculation for the requested range.
   * Note that this algorithm allows the input and
   * output to be the same buffer.
   */
  outIdx = 0;
  today = startIdx;
  trailingIdx = startIdx - optInTimePeriod;
  lowestIdx = -1;
  highestIdx = -1;
  lowest = 0.0;
  highest = 0.0;
  factor = 100.0 / optInTimePeriod;
  while (today <= endIdx) {
    /* Keep track of the lowestIdx */
    tmp = inLow[today];
    if (lowestIdx < trailingIdx) {
      lowestIdx = trailingIdx;
      lowest = inLow[lowestIdx];
      i = lowestIdx;
      while (++i <= today) {
        tmp = inLow[i];
        if (tmp <= lowest) {
          lowestIdx = i;
          lowest = tmp;
        }
      }
    } else if (tmp <= lowest) {
      lowestIdx = today;
      lowest = tmp;
    }
    /* Keep track of the highestIdx */
    tmp = inHigh[today];
    if (highestIdx < trailingIdx) {
      highestIdx = trailingIdx;
      highest = inHigh[highestIdx];
      i = highestIdx;
      while (++i <= today) {
        tmp = inHigh[i];
        if (tmp >= highest) {
          highestIdx = i;
          highest = tmp;
        }
      }
    } else if (tmp >= highest) {
      highestIdx = today;
      highest = tmp;
    }
    /* Note: Do not forget that input and output buffer can be the same,
     *       so writing to the output is the last thing being done here.
     */
    outAroonUp[outIdx] = factor * (optInTimePeriod - (today - highestIdx));
    outAroonDown[outIdx] = factor * (optInTimePeriod - (today - lowestIdx));
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
 * Aroon reports how recently the highest high and lowest low occurred within a
 * rolling window of length optInTimePeriod, as two 0-100 oscillators. Indicates
 * trend strength and direction. Up near 100 = a very recent new high (strong
 * uptrend); Down near 100 = a very recent new low. Up/Down crossovers signal trend
 * shifts.
 *
 * Formula and more info at
 * [ta-lib.org/functions/aroon](https://ta-lib.org/functions/aroon).
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link aroonLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param optInTimePeriod - Lookback window length (default 14; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param outAroonDown - Recency of the lowest low (100 = it is the current bar,
 *   decaying as it ages) Must hold the values produced and never be empty; the input
 *   length always suffices.
 * @param outAroonUp - Recency of the highest high (100 = it is the current bar,
 *   decaying as it ages) Must hold the values produced and never be empty; the input
 *   length always suffices.
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
 * import { aroon } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const aroonDown = new Float64Array(high.length);
 * const aroonUp = new Float64Array(high.length);
 * const { begIdx, count } = aroon(
 *   0, high.length - 1,
 *   high, low,
 *   14,
 *   aroonDown, aroonUp,
 * );
 * // aroonDown[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`aroonosc`](https://ta-lib.org/functions/aroonosc)
 * @see [`minmaxindex`](https://ta-lib.org/functions/minmaxindex)
 * @see [`min`](https://ta-lib.org/functions/min)
 * @see [`max`](https://ta-lib.org/functions/max)
 */
export function aroon(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  outAroonDown: Float64Array,
  outAroonUp: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('AROON', 'startIdx', startIdx);
  endIdx = requireInt32('AROON', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('AROON', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('AROON', startIdx, endIdx);
  const guardStart = clampedStart('AROON', startIdx, aroonLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('AROON', 'inHigh', inHigh, guardInLen);
  requireLength('AROON', 'inLow', inLow, guardInLen);
  requireLength('AROON', 'outAroonDown', outAroonDown, guardOutLen);
  requireLength('AROON', 'outAroonUp', outAroonUp, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlapsPartly(outAroonDown, inHighF64) || overlapsPartly(outAroonDown, inLowF64) || overlapsPartly(outAroonUp, inHighF64) || overlapsPartly(outAroonUp, inLowF64) || overlaps(outAroonDown, outAroonUp)) {
    throw failure('AROON', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = aroonImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInTimePeriod, outBegIdx, outNBElement, outAroonDown, outAroonUp);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('AROON', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live AROON stream: one value per closed bar, bit-identical to {@link aroon}
 * over the same series. Open it with {@link aroonOpen} or
 * {@link aroonOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/aroon](https://ta-lib.org/functions/aroon).
 */
export class AroonStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  lowest: number = 0;
  /** @internal */
  highest: number = 0;
  /** @internal */
  factor: number = 0;
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
  cur_outAroonDown: number = 0;
  /** @internal */
  cur_outAroonUp: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): AroonStream {
    return new AroonStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link aroon} reports them over the same bars. Each accepted `update` and each
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
      throw failure('AROON advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
  update(inHigh: number, inLow: number, out: AroonOut): void {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('AROON update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (typeof out !== 'object' || out === null) {
      throw badSink('AROON update', 'AroonOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('AROON update', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    aroonStepImpl(this, inHigh, inLow);
    this.outRangeCount++;
    out.aroonDown = this.cur_outAroonDown;
    out.aroonUp = this.cur_outAroonUp;
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
  peek(inHigh: number, inLow: number, out: AroonOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('AROON peek', 'AroonOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('AROON peek', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    const sp = this;
    let tmp = 0;
    let cur_outAroonDown = 0;
    let cur_outAroonUp = 0;
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
    /* Keep track of the lowestIdx */
    tmp = ((sp.today & sp.xMask) !== pkSlot1) ? sp.x_inLow[sp.today & sp.xMask] : pkVal1;
    if (lowestIdx < sp.trailingIdx) {
      lowestIdx = sp.trailingIdx;
      lowest = ((lowestIdx & sp.xMask) !== pkSlot1) ? sp.x_inLow[lowestIdx & sp.xMask] : pkVal1;
      i = lowestIdx;
      while (++i <= sp.today) {
        tmp = ((i & sp.xMask) !== pkSlot1) ? sp.x_inLow[i & sp.xMask] : pkVal1;
        if (tmp <= lowest) {
          lowestIdx = i;
          lowest = tmp;
        }
      }
    } else if (tmp <= lowest) {
      lowestIdx = sp.today;
      lowest = tmp;
    }
    /* Keep track of the highestIdx */
    tmp = ((sp.today & sp.xMask) !== pkSlot0) ? sp.x_inHigh[sp.today & sp.xMask] : pkVal0;
    if (highestIdx < sp.trailingIdx) {
      highestIdx = sp.trailingIdx;
      highest = ((highestIdx & sp.xMask) !== pkSlot0) ? sp.x_inHigh[highestIdx & sp.xMask] : pkVal0;
      i = highestIdx;
      while (++i <= sp.today) {
        tmp = ((i & sp.xMask) !== pkSlot0) ? sp.x_inHigh[i & sp.xMask] : pkVal0;
        if (tmp >= highest) {
          highestIdx = i;
          highest = tmp;
        }
      }
    } else if (tmp >= highest) {
      highestIdx = sp.today;
      highest = tmp;
    }
    /* Note: Do not forget that input and output buffer can be the same,
     *       so writing to the output is the last thing being done here.
     */
    cur_outAroonUp = sp.factor * (sp.optInTimePeriod - (sp.today - highestIdx));
    cur_outAroonDown = sp.factor * (sp.optInTimePeriod - (sp.today - lowestIdx));
    out.aroonDown = cur_outAroonDown;
    out.aroonUp = cur_outAroonUp;
  }

  /**
   * The value at the last bar this stream counted; `peek` does not change it.
   *
   * @param out - Receives the outputs.
   */
  value(out: AroonOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('AROON value', 'AroonOut');
    }
    out.aroonDown = this.cur_outAroonDown;
    out.aroonUp = this.cur_outAroonUp;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): AroonStream {
    const c = new AroonStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.lowest = this.lowest;
    c.highest = this.highest;
    c.factor = this.factor;
    c.trailingIdx = this.trailingIdx;
    c.lowestIdx = this.lowestIdx;
    c.highestIdx = this.highestIdx;
    c.i = this.i;
    c.today = this.today;
    c.xMask = this.xMask;
    c.x_inHigh = this.x_inHigh.slice();
    c.x_inLow = this.x_inLow.slice();
    c.cur_outAroonDown = this.cur_outAroonDown;
    c.cur_outAroonUp = this.cur_outAroonUp;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one AROON bar, written into an object the caller owns and reuses:
 * valid until the next call that writes it.
 */
export class AroonOut {
  /** Recency of the lowest low (100 = it is the current bar, decaying as it ages) */
  aroonDown: number = 0;
  /** Recency of the highest high (100 = it is the current bar, decaying as it ages) */
  aroonUp: number = 0;
}

function aroonStepImpl(sp: AroonStream, inHigh: number, inLow: number): void {
  let tmp = 0;
  sp.x_inHigh[sp.today & sp.xMask] = inHigh;
  sp.x_inLow[sp.today & sp.xMask] = inLow;
  /* Keep track of the lowestIdx */
  tmp = sp.x_inLow[sp.today & sp.xMask];
  if (sp.lowestIdx < sp.trailingIdx) {
    sp.lowestIdx = sp.trailingIdx;
    sp.lowest = sp.x_inLow[sp.lowestIdx & sp.xMask];
    sp.i = sp.lowestIdx;
    while (++sp.i <= sp.today) {
      tmp = sp.x_inLow[sp.i & sp.xMask];
      if (tmp <= sp.lowest) {
        sp.lowestIdx = sp.i;
        sp.lowest = tmp;
      }
    }
  } else if (tmp <= sp.lowest) {
    sp.lowestIdx = sp.today;
    sp.lowest = tmp;
  }
  /* Keep track of the highestIdx */
  tmp = sp.x_inHigh[sp.today & sp.xMask];
  if (sp.highestIdx < sp.trailingIdx) {
    sp.highestIdx = sp.trailingIdx;
    sp.highest = sp.x_inHigh[sp.highestIdx & sp.xMask];
    sp.i = sp.highestIdx;
    while (++sp.i <= sp.today) {
      tmp = sp.x_inHigh[sp.i & sp.xMask];
      if (tmp >= sp.highest) {
        sp.highestIdx = sp.i;
        sp.highest = tmp;
      }
    }
  } else if (tmp >= sp.highest) {
    sp.highestIdx = sp.today;
    sp.highest = tmp;
  }
  /* Note: Do not forget that input and output buffer can be the same,
   *       so writing to the output is the last thing being done here.
   */
  sp.cur_outAroonUp = sp.factor * (sp.optInTimePeriod - (sp.today - sp.highestIdx));
  sp.cur_outAroonDown = sp.factor * (sp.optInTimePeriod - (sp.today - sp.lowestIdx));
  sp.trailingIdx += 1;
  sp.today += 1;
}

function aroonOpenImpl(
  sp: AroonStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outAroonDown: Float64Array,
  outAroonUp: Float64Array,
  outStride: number,
): RetCode {
  let lowest = 0;
  let highest = 0;
  let tmp = 0;
  let factor = 0;
  let outIdx = 0;
  let trailingIdx = 0;
  let lowestIdx = 0;
  let highestIdx = 0;
  let today = 0;
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
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* This function is using a speed optimized algorithm
   * for the min/max logic.
   *
   * You might want to first look at how TA_MIN/TA_MAX works
   * and this function will become easier to understand.
   */
  /* Move up the start index if there is not
   * enough initial data.
   */
  if (startIdx < optInTimePeriod) {
    startIdx = optInTimePeriod;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Proceed with the calculation for the requested range.
   * Note that this algorithm allows the input and
   * output to be the same buffer.
   */
  outIdx = 0;
  today = startIdx;
  trailingIdx = startIdx - optInTimePeriod;
  lowestIdx = -1;
  highestIdx = -1;
  lowest = 0.0;
  highest = 0.0;
  factor = 100.0 / optInTimePeriod;
  while (today <= endIdx) {
    /* Keep track of the lowestIdx */
    tmp = inLow[today];
    if (lowestIdx < trailingIdx) {
      lowestIdx = trailingIdx;
      lowest = inLow[lowestIdx];
      i = lowestIdx;
      while (++i <= today) {
        tmp = inLow[i];
        if (tmp <= lowest) {
          lowestIdx = i;
          lowest = tmp;
        }
      }
    } else if (tmp <= lowest) {
      lowestIdx = today;
      lowest = tmp;
    }
    /* Keep track of the highestIdx */
    tmp = inHigh[today];
    if (highestIdx < trailingIdx) {
      highestIdx = trailingIdx;
      highest = inHigh[highestIdx];
      i = highestIdx;
      while (++i <= today) {
        tmp = inHigh[i];
        if (tmp >= highest) {
          highestIdx = i;
          highest = tmp;
        }
      }
    } else if (tmp >= highest) {
      highestIdx = today;
      highest = tmp;
    }
    /* Note: Do not forget that input and output buffer can be the same,
     *       so writing to the output is the last thing being done here.
     */
    outAroonUp[((outIdx * outStride) | 0)] = factor * (optInTimePeriod - (today - highestIdx));
    outAroonDown[((outIdx * outStride) | 0)] = factor * (optInTimePeriod - (today - lowestIdx));
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
  sp.factor = factor;
  sp.trailingIdx = trailingIdx;
  sp.lowestIdx = lowestIdx;
  sp.highestIdx = highestIdx;
  sp.i = i;
  sp.today = today;
  sp.xMask = physX - 1;
  sp.x_inHigh = capX_inHigh;
  sp.x_inLow = capX_inLow;
  sp.cur_outAroonDown = outAroonDown[(outNBElement.value - 1) * outStride];
  sp.cur_outAroonUp = outAroonUp[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function aroonOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outAroonDown: Float64Array,
  outAroonUp: Float64Array,
): AroonStream {
  const sp = AroonStream.of(core);
  const retCode = aroonOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, outAroonDown, outAroonUp, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('AROON openAndFill', inHigh.length, startIdx, aroonLookback(optInTimePeriod, core));
  }
  throw streamFailure('AROON openAndFill', retCode);
}

/** @internal */
export function aroonOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): AroonStream {
  const sp = AroonStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outAroonDown = new Float64Array(1);
  const sink_outAroonUp = new Float64Array(1);
  const retCode = aroonOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outAroonDown, sink_outAroonUp, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('AROON open', inHigh.length, startIdx, aroonLookback(optInTimePeriod, core));
  }
  throw streamFailure('AROON open', retCode);
}

/**
 * Opens a live AROON stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link aroon} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Lookback window length (default 14; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `aroonLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { AroonOut, aroonOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = aroonOpen(high, low, 14);
 * const peeked = new AroonOut();
 * const updated = new AroonOut();
 * stream.peek(101.4, 99.1, peeked); // a forming bar: commits nothing
 * stream.update(101.4, 99.1, updated); // the bar closes
 * ```
 */
export function aroonOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): AroonStream {
  optInTimePeriod = requireInt32('AROON open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('AROON open', 'inHigh', inHigh);
  requireHistory('AROON open', inHigh.length);
  requireArgument('AROON open', 'inLow', inLow);
  requireHistoryLength('AROON open', 'inLow', inLow.length, inHigh.length);
  return aroonOpenInternal(core, float64(inHigh), float64(inLow), 0, optInTimePeriod);
}

/**
 * {@link aroonOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link aroon}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Lookback window length (default 14; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param outAroonDown - Recency of the lowest low (100 = it is the current bar,
 *   decaying as it ages) Must hold `historyLen - lookback` values and share no
 *   memory with an input or another output.
 * @param outAroonUp - Recency of the highest high (100 = it is the current bar,
 *   decaying as it ages) Must hold `historyLen - lookback` values and share no
 *   memory with an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `aroonLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range, histories of different lengths or an output too
 *   short or starting where another output or a `Float64Array` input starts, or, on
 *   a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { aroonOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const aroonDown = new Float64Array(high.length);
 * const aroonUp = new Float64Array(high.length);
 * const stream = aroonOpenAndFill(high, low, 14, aroonDown, aroonUp);
 * const { begIdx, count } = stream.outRange();
 * // aroonDown[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function aroonOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  outAroonDown: Float64Array,
  outAroonUp: Float64Array,
  core: Core = Core.DEFAULT,
): AroonStream {
  optInTimePeriod = requireInt32('AROON openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('AROON openAndFill', 'inHigh', inHigh);
  requireHistory('AROON openAndFill', inHigh.length);
  requireArgument('AROON openAndFill', 'inLow', inLow);
  const guardOutLen = openFillCount('AROON openAndFill', inHigh.length, aroonLookback(optInTimePeriod, core));
  requireHistoryLength('AROON openAndFill', 'inLow', inLow.length, inHigh.length);
  requireLength('AROON openAndFill', 'outAroonDown', outAroonDown, guardOutLen);
  requireLength('AROON openAndFill', 'outAroonUp', outAroonUp, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if ((outAroonDown.buffer === inHighF64.buffer && outAroonDown.byteOffset === inHighF64.byteOffset) || (outAroonDown.buffer === inLowF64.buffer && outAroonDown.byteOffset === inLowF64.byteOffset) || (outAroonUp.buffer === inHighF64.buffer && outAroonUp.byteOffset === inHighF64.byteOffset) || (outAroonUp.buffer === inLowF64.buffer && outAroonUp.byteOffset === inLowF64.byteOffset) || (outAroonDown.buffer === outAroonUp.buffer && outAroonDown.byteOffset === outAroonUp.byteOffset)) {
    throw streamFailure('AROON openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outAroonDown, inHighF64) || overlaps(outAroonDown, inLowF64) || overlaps(outAroonUp, inHighF64) || overlaps(outAroonUp, inLowF64) || overlaps(outAroonDown, outAroonUp))) {
    throw streamFailure('AROON openAndFill', RetCode.BAD_PARAM);
  }
  return aroonOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outAroonDown, outAroonUp);
}
