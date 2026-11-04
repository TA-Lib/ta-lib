/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AC       Angelo Ciceri
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120906 AC   Creation (equal to MINMAX but outputs index)
 *  100526 MF,CC A tie names the newest bar from any start (#503)
 */

import { badSink, clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link minmaxindex} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param optInTimePeriod - Window length in bars (default 30; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function minmaxindexLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  return optInTimePeriod - 1;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link minmaxindex}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Window length in bars (default 30; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function minmaxindexDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (minmaxindexLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 2) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function minmaxindexImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outMinIdx: Int32Array,
  outMaxIdx: Int32Array,
): RetCode {
  let highest = 0;
  let lowest = 0;
  let tmpHigh = 0;
  let tmpLow = 0;
  let outIdx = 0;
  let nbInitialElementNeeded = 0;
  let trailingIdx = 0;
  let today = 0;
  let i = 0;
  let highestIdx = 0;
  let lowestIdx = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if ((outMinIdx.buffer === outMaxIdx.buffer && outMinIdx.byteOffset === outMaxIdx.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
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
   * (The integer outputs can never share the real input's buffer —
   * different element type; issue #130.)
   */
  outIdx = 0;
  today = startIdx;
  trailingIdx = startIdx - nbInitialElementNeeded;
  highestIdx = -1;
  highest = 0.0;
  lowestIdx = -1;
  lowest = 0.0;
  while (today <= endIdx) {
    tmpHigh = inReal[today];
    tmpLow = tmpHigh;
    if (highestIdx < trailingIdx) {
      highestIdx = trailingIdx;
      highest = inReal[highestIdx];
      i = highestIdx;
      while (++i <= today) {
        tmpHigh = inReal[i];
        if (tmpHigh >= highest) {
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
      lowest = inReal[lowestIdx];
      i = lowestIdx;
      while (++i <= today) {
        tmpLow = inReal[i];
        if (tmpLow <= lowest) {
          lowestIdx = i;
          lowest = tmpLow;
        }
      }
    } else if (tmpLow <= lowest) {
      lowestIdx = today;
      lowest = tmpLow;
    }
    outMaxIdx[outIdx] = highestIdx;
    outMinIdx[outIdx] = lowestIdx;
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
 * Returns the absolute input indices of the lowest and highest values within each
 * rolling window of optInTimePeriod bars. Index variant of MINMAX.
 *
 * Formula and more info at
 * [ta-lib.org/functions/minmaxindex](https://ta-lib.org/functions/minmaxindex).
 *
 * **Notes**
 * - When several bars in a window share the extreme value, the index of the most
 *   recent of them is returned.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link minmaxindexLookback} is a success
 * with `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - Input series scanned for extremes.
 * @param optInTimePeriod - Window length in bars (default 30; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param outMinIdx - Absolute index (into inReal) of the window minimum. Must hold
 *   the values produced and never be empty; the input length always suffices.
 * @param outMaxIdx - Absolute index (into inReal) of the window maximum. Must hold
 *   the values produced and never be empty; the input length always suffices.
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
 * import { minmaxindex } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const minIdx = new Int32Array(data.length);
 * const maxIdx = new Int32Array(data.length);
 * const { begIdx, count } = minmaxindex(
 *   0, data.length - 1,
 *   data,
 *   30,
 *   minIdx, maxIdx,
 * );
 * // minIdx[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`minmax`](https://ta-lib.org/functions/minmax)
 * @see [`min`](https://ta-lib.org/functions/min)
 * @see [`max`](https://ta-lib.org/functions/max)
 * @see [`minindex`](https://ta-lib.org/functions/minindex)
 * @see [`maxindex`](https://ta-lib.org/functions/maxindex)
 */
export function minmaxindex(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outMinIdx: Int32Array,
  outMaxIdx: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('MINMAXINDEX', 'startIdx', startIdx);
  endIdx = requireInt32('MINMAXINDEX', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('MINMAXINDEX', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('MINMAXINDEX', startIdx, endIdx);
  const guardStart = clampedStart('MINMAXINDEX', startIdx, minmaxindexLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('MINMAXINDEX', 'inReal', inReal, guardInLen);
  requireOutput('MINMAXINDEX', 'outMinIdx', outMinIdx, guardOutLen, Int32Array);
  requireOutput('MINMAXINDEX', 'outMaxIdx', outMaxIdx, guardOutLen, Int32Array);
  const inRealF64 = float64(inReal);
  if (overlaps(outMinIdx, inRealF64) || overlaps(outMaxIdx, inRealF64) || overlaps(outMinIdx, outMaxIdx)) {
    throw failure('MINMAXINDEX', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = minmaxindexImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outMinIdx, outMaxIdx);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('MINMAXINDEX', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live MINMAXINDEX stream: one value per closed bar, bit-identical to
 * {@link minmaxindex} over the same series. Open it with {@link minmaxindexOpen}
 * or {@link minmaxindexOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/minmaxindex](https://ta-lib.org/functions/minmaxindex).
 */
export class MinmaxindexStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  highest: number = 0;
  /** @internal */
  lowest: number = 0;
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
  x_inReal: Float64Array = new Float64Array(1);
  /** @internal */
  cur_outMinIdx: number = 0;
  /** @internal */
  cur_outMaxIdx: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): MinmaxindexStream {
    return new MinmaxindexStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link minmaxindex} reports them over the same bars. Each accepted `update` and
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
      throw failure('MINMAXINDEX advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar, writing the new values into `out`.
   *
   * @param inReal - The bar's `inReal` value: Input series scanned for extremes.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number, out: MinmaxindexOut): void {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('MINMAXINDEX update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (typeof out !== 'object' || out === null) {
      throw badSink('MINMAXINDEX update', 'MinmaxindexOut');
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('MINMAXINDEX update', 'inReal');
    }
    minmaxindexStepImpl(this, inReal);
    this.outRangeCount++;
    out.minIdx = this.cur_outMinIdx;
    out.maxIdx = this.cur_outMaxIdx;
  }

  /**
   * Writes into `out` what the next `update` with the same bar would write,
   * committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Input series scanned for extremes.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number, out: MinmaxindexOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('MINMAXINDEX peek', 'MinmaxindexOut');
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('MINMAXINDEX peek', 'inReal');
    }
    const sp = this;
    let tmpHigh = 0;
    let tmpLow = 0;
    let cur_outMaxIdx = 0;
    let cur_outMinIdx = 0;
    let highest = sp.highest;
    let highestIdx = sp.highestIdx;
    let i = sp.i;
    let lowest = sp.lowest;
    let lowestIdx = sp.lowestIdx;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    pkSlot0 = sp.today & sp.xMask;
    pkVal0 = inReal;
    tmpHigh = ((sp.today & sp.xMask) !== pkSlot0) ? sp.x_inReal[sp.today & sp.xMask] : pkVal0;
    tmpLow = tmpHigh;
    if (highestIdx < sp.trailingIdx) {
      highestIdx = sp.trailingIdx;
      highest = ((highestIdx & sp.xMask) !== pkSlot0) ? sp.x_inReal[highestIdx & sp.xMask] : pkVal0;
      i = highestIdx;
      while (++i <= sp.today) {
        tmpHigh = ((i & sp.xMask) !== pkSlot0) ? sp.x_inReal[i & sp.xMask] : pkVal0;
        if (tmpHigh >= highest) {
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
      lowest = ((lowestIdx & sp.xMask) !== pkSlot0) ? sp.x_inReal[lowestIdx & sp.xMask] : pkVal0;
      i = lowestIdx;
      while (++i <= sp.today) {
        tmpLow = ((i & sp.xMask) !== pkSlot0) ? sp.x_inReal[i & sp.xMask] : pkVal0;
        if (tmpLow <= lowest) {
          lowestIdx = i;
          lowest = tmpLow;
        }
      }
    } else if (tmpLow <= lowest) {
      lowestIdx = sp.today;
      lowest = tmpLow;
    }
    cur_outMaxIdx = highestIdx;
    cur_outMinIdx = lowestIdx;
    out.minIdx = cur_outMinIdx;
    out.maxIdx = cur_outMaxIdx;
  }

  /**
   * The value at the last bar this stream counted; `peek` does not change it.
   *
   * @param out - Receives the outputs.
   */
  value(out: MinmaxindexOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('MINMAXINDEX value', 'MinmaxindexOut');
    }
    out.minIdx = this.cur_outMinIdx;
    out.maxIdx = this.cur_outMaxIdx;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): MinmaxindexStream {
    const c = new MinmaxindexStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.highest = this.highest;
    c.lowest = this.lowest;
    c.trailingIdx = this.trailingIdx;
    c.highestIdx = this.highestIdx;
    c.lowestIdx = this.lowestIdx;
    c.i = this.i;
    c.today = this.today;
    c.xMask = this.xMask;
    c.x_inReal = this.x_inReal.slice();
    c.cur_outMinIdx = this.cur_outMinIdx;
    c.cur_outMaxIdx = this.cur_outMaxIdx;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one MINMAXINDEX bar, written into an object the caller owns and
 * reuses: valid until the next call that writes it.
 */
export class MinmaxindexOut {
  /** Absolute index (into inReal) of the window minimum. */
  minIdx: number = 0;
  /** Absolute index (into inReal) of the window maximum. */
  maxIdx: number = 0;
}

function minmaxindexStepImpl(sp: MinmaxindexStream, inReal: number): void {
  let tmpHigh = 0;
  let tmpLow = 0;
  sp.x_inReal[sp.today & sp.xMask] = inReal;
  tmpHigh = sp.x_inReal[sp.today & sp.xMask];
  tmpLow = tmpHigh;
  if (sp.highestIdx < sp.trailingIdx) {
    sp.highestIdx = sp.trailingIdx;
    sp.highest = sp.x_inReal[sp.highestIdx & sp.xMask];
    sp.i = sp.highestIdx;
    while (++sp.i <= sp.today) {
      tmpHigh = sp.x_inReal[sp.i & sp.xMask];
      if (tmpHigh >= sp.highest) {
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
    sp.lowest = sp.x_inReal[sp.lowestIdx & sp.xMask];
    sp.i = sp.lowestIdx;
    while (++sp.i <= sp.today) {
      tmpLow = sp.x_inReal[sp.i & sp.xMask];
      if (tmpLow <= sp.lowest) {
        sp.lowestIdx = sp.i;
        sp.lowest = tmpLow;
      }
    }
  } else if (tmpLow <= sp.lowest) {
    sp.lowestIdx = sp.today;
    sp.lowest = tmpLow;
  }
  sp.cur_outMaxIdx = sp.highestIdx;
  sp.cur_outMinIdx = sp.lowestIdx;
  sp.trailingIdx += 1;
  sp.today += 1;
}

function minmaxindexOpenImpl(
  sp: MinmaxindexStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outMinIdx: Int32Array,
  outMaxIdx: Int32Array,
  outStride: number,
): RetCode {
  let highest = 0;
  let lowest = 0;
  let tmpHigh = 0;
  let tmpLow = 0;
  let outIdx = 0;
  let nbInitialElementNeeded = 0;
  let trailingIdx = 0;
  let today = 0;
  let i = 0;
  let highestIdx = 0;
  let lowestIdx = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
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
   * (The integer outputs can never share the real input's buffer —
   * different element type; issue #130.)
   */
  outIdx = 0;
  today = startIdx;
  trailingIdx = startIdx - nbInitialElementNeeded;
  highestIdx = -1;
  highest = 0.0;
  lowestIdx = -1;
  lowest = 0.0;
  while (today <= endIdx) {
    tmpHigh = inReal[today];
    tmpLow = tmpHigh;
    if (highestIdx < trailingIdx) {
      highestIdx = trailingIdx;
      highest = inReal[highestIdx];
      i = highestIdx;
      while (++i <= today) {
        tmpHigh = inReal[i];
        if (tmpHigh >= highest) {
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
      lowest = inReal[lowestIdx];
      i = lowestIdx;
      while (++i <= today) {
        tmpLow = inReal[i];
        if (tmpLow <= lowest) {
          lowestIdx = i;
          lowest = tmpLow;
        }
      }
    } else if (tmpLow <= lowest) {
      lowestIdx = today;
      lowest = tmpLow;
    }
    outMaxIdx[((outIdx * outStride) | 0)] = highestIdx;
    outMinIdx[((outIdx * outStride) | 0)] = lowestIdx;
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
  const capX_inReal = new Float64Array(physX);
  for (let fillJ = historyLen - capX; fillJ < historyLen; fillJ++) {
    capX_inReal[fillJ & (physX - 1)] = inReal[fillJ];
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.highest = highest;
  sp.lowest = lowest;
  sp.trailingIdx = trailingIdx;
  sp.highestIdx = highestIdx;
  sp.lowestIdx = lowestIdx;
  sp.i = i;
  sp.today = today;
  sp.xMask = physX - 1;
  sp.x_inReal = capX_inReal;
  sp.cur_outMinIdx = outMinIdx[(outNBElement.value - 1) * outStride];
  sp.cur_outMaxIdx = outMaxIdx[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function minmaxindexOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outMinIdx: Int32Array,
  outMaxIdx: Int32Array,
): MinmaxindexStream {
  const sp = MinmaxindexStream.of(core);
  const retCode = minmaxindexOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outMinIdx, outMaxIdx, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MINMAXINDEX openAndFill', inReal.length, startIdx, minmaxindexLookback(optInTimePeriod, core));
  }
  throw streamFailure('MINMAXINDEX openAndFill', retCode);
}

/** @internal */
export function minmaxindexOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): MinmaxindexStream {
  const sp = MinmaxindexStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outMinIdx = new Int32Array(1);
  const sink_outMaxIdx = new Int32Array(1);
  const retCode = minmaxindexOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outMinIdx, sink_outMaxIdx, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MINMAXINDEX open', inReal.length, startIdx, minmaxindexLookback(optInTimePeriod, core));
  }
  throw streamFailure('MINMAXINDEX open', retCode);
}

/**
 * Opens a live MINMAXINDEX stream over the warm-up history: `value()` starts at
 * the last history bar, bit-identical to {@link minmaxindex} there.
 *
 * @param inReal - Warm-up history. Input series scanned for extremes.
 * @param optInTimePeriod - Window length in bars (default 30; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `minmaxindexLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an
 *   empty one, and `TA_BAD_PARAM` for an integer parameter that is not an int32 or a
 *   parameter outside its range.
 *
 * @example
 * ```ts
 * import { MinmaxindexOut, minmaxindexOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = minmaxindexOpen(data, 30);
 * const peeked = new MinmaxindexOut();
 * const updated = new MinmaxindexOut();
 * stream.peek(100.9, peeked); // a forming bar: commits nothing
 * stream.update(100.9, updated); // the bar closes
 * ```
 */
export function minmaxindexOpen(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): MinmaxindexStream {
  optInTimePeriod = requireInt32('MINMAXINDEX open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('MINMAXINDEX open', 'inReal', inReal);
  requireHistory('MINMAXINDEX open', inReal.length);
  return minmaxindexOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/**
 * {@link minmaxindexOpen} that also fills the outputs over the whole history in
 * the same pass, bit-identically to {@link minmaxindex}. The range written is the
 * stream's `outRange()`.
 *
 * @param inReal - Warm-up history. Input series scanned for extremes.
 * @param optInTimePeriod - Window length in bars (default 30; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param outMinIdx - Absolute index (into inReal) of the window minimum. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param outMaxIdx - Absolute index (into inReal) of the window maximum. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `minmaxindexLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an
 *   empty one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or an output too short or starting where another
 *   output or a `Float64Array` input starts, or, on a history long enough to open,
 *   one sharing memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { minmaxindexOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const minIdx = new Int32Array(data.length);
 * const maxIdx = new Int32Array(data.length);
 * const stream = minmaxindexOpenAndFill(data, 30, minIdx, maxIdx);
 * const { begIdx, count } = stream.outRange();
 * // minIdx[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function minmaxindexOpenAndFill(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outMinIdx: Int32Array,
  outMaxIdx: Int32Array,
  core: Core = Core.DEFAULT,
): MinmaxindexStream {
  optInTimePeriod = requireInt32('MINMAXINDEX openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('MINMAXINDEX openAndFill', 'inReal', inReal);
  requireHistory('MINMAXINDEX openAndFill', inReal.length);
  const guardOutLen = openFillCount('MINMAXINDEX openAndFill', inReal.length, minmaxindexLookback(optInTimePeriod, core));
  requireOutput('MINMAXINDEX openAndFill', 'outMinIdx', outMinIdx, guardOutLen, Int32Array);
  requireOutput('MINMAXINDEX openAndFill', 'outMaxIdx', outMaxIdx, guardOutLen, Int32Array);
  const inRealF64 = float64(inReal);
  if ((outMinIdx.buffer === inRealF64.buffer && outMinIdx.byteOffset === inRealF64.byteOffset) || (outMaxIdx.buffer === inRealF64.buffer && outMaxIdx.byteOffset === inRealF64.byteOffset) || (outMinIdx.buffer === outMaxIdx.buffer && outMinIdx.byteOffset === outMaxIdx.byteOffset)) {
    throw streamFailure('MINMAXINDEX openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outMinIdx, inRealF64) || overlaps(outMaxIdx, inRealF64) || overlaps(outMinIdx, outMaxIdx))) {
    throw streamFailure('MINMAXINDEX openAndFill', RetCode.BAD_PARAM);
  }
  return minmaxindexOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outMinIdx, outMaxIdx);
}
