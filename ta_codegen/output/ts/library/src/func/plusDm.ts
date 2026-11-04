/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CF       Christo Fogelberg
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  010802 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  122104 MF,CF  Fix#1089506 for when optInTimePeriod is 1.
 *  091326 MF,CC  #411 Wilder step without a divide or a branch.
 */

import { FuncUnstId } from '../enums.ts';
import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link plusDm} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param optInTimePeriod - Wilder smoothing period (default 14; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function plusDmLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  if (optInTimePeriod > 1) {
    return optInTimePeriod + core.unstableCount(FuncUnstId.PLUS_DM, ((optInTimePeriod > 1) ? (((((10 * (((2 * optInTimePeriod) | 0) - 1)) | 0) + 1) / 2) | 0) : 0), ((optInTimePeriod > 1) ? (((((19 * (((2 * optInTimePeriod) | 0) - 1)) | 0) + 1) / 2) | 0) : 0)) - 1;
  } else {
    return 1;
  }
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link plusDm}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Wilder smoothing period (default 14; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function plusDmDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (plusDmLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function plusDmImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let today = 0;
  let lookbackTotal = 0;
  let outIdx = 0;
  let prevHigh = 0;
  let prevLow = 0;
  let tempReal = 0;
  let invPeriod = 0;
  let prevPlusDM = 0;
  let diffP = 0;
  let diffM = 0;
  let plusDM1 = 0;
  let i = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  /*
   * The DM1 (one period) is base on the largest part of
   * today's range that is outside of yesterdays range.
   *
   * The following 7 cases explain how the +DM and -DM are
   * calculated on one period:
   *
   * Case 1:                       Case 2:
   *    C|                        A|
   *     |                         | C|
   *     | +DM1 = (C-A)           B|  | +DM1 = 0
   *     | -DM1 = 0                   | -DM1 = (B-D)
   * A|  |                           D|
   *  | D|
   * B|
   *
   * Case 3:                       Case 4:
   *    C|                           C|
   *     |                        A|  |
   *     | +DM1 = (C-A)            |  | +DM1 = 0
   *     | -DM1 = 0               B|  | -DM1 = (B-D)
   * A|  |                            |
   *  |  |                           D|
   * B|  |
   *    D|
   *
   * Case 5:                      Case 6:
   * A|                           A| C|
   *  | C| +DM1 = 0                |  |  +DM1 = 0
   *  |  | -DM1 = 0                |  |  -DM1 = 0
   *  | D|                         |  |
   * B|                           B| D|
   *
   *
   * Case 7:
   *
   *    C|
   * A|  |
   *  |  | +DM=0
   * B|  | -DM=0
   *    D|
   *
   * In case 3 and 4, the rule is that the smallest delta between
   * (C-A) and (B-D) determine which of +DM or -DM is zero.
   *
   * In case 7, (C-A) and (B-D) are equal, so both +DM and -DM are
   * zero.
   *
   * The rules remain the same when A=B and C=D (when the highs
   * equal the lows).
   *
   * When calculating the DM over a period > 1, the one-period DM
   * for the desired period are initialy sum. In other word,
   * for a +DM14, sum the +DM1 for the first 14 days (that's
   * 13 values because there is no DM for the first day!)
   * Subsequent DM are calculated using the Wilder's
   * smoothing approach:
   *
   *                                    Previous +DM14
   *  Today's +DM14 = Previous +DM14 -  -------------- + Today's +DM1
   *                                         14
   *
   * Reference:
   *    New Concepts In Technical Trading Systems, J. Welles Wilder Jr
   */
  lookbackTotal = plusDmLookback(optInTimePeriod, core);
  /* Adjust startIdx to account for the lookback period. */
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  /* Indicate where the next output should be put
   * in the outReal.
   */
  outIdx = 0;
  /* Trap the case where no smoothing is needed. */
  if (optInTimePeriod <= 1) {
    /* No smoothing needed. Just do a simple DM1
     * for each price bar.
     */
    outBegIdx.value = startIdx;
    today = startIdx - 1;
    prevHigh = inHigh[today];
    prevLow = inLow[today];
    while (today < endIdx) {
      today += 1;
      tempReal = inHigh[today];
      diffP = tempReal - prevHigh;
      /* Plus Delta */
      prevHigh = tempReal;
      tempReal = inLow[today];
      diffM = prevLow - tempReal;
      /* Minus Delta */
      prevLow = tempReal;
      plusDM1 = (diffP > 0.0) ? diffP : 0.0;
      plusDM1 = (diffP > diffM) ? plusDM1 : 0.0;
      outReal[outIdx++] = plusDM1;
    }
    outNBElement.value = outIdx;
    return RetCode.SUCCESS;
  }
  invPeriod = 1.0 / optInTimePeriod;
  /* Process the initial DM */
  outBegIdx.value = startIdx;
  prevPlusDM = 0.0;
  today = startIdx - lookbackTotal;
  prevHigh = inHigh[today];
  prevLow = inLow[today];
  i = optInTimePeriod - 1;
  while (i-- > 0) {
    today += 1;
    tempReal = inHigh[today];
    diffP = tempReal - prevHigh;
    /* Plus Delta */
    prevHigh = tempReal;
    tempReal = inLow[today];
    diffM = prevLow - tempReal;
    /* Minus Delta */
    prevLow = tempReal;
    /* +DM1 = diffP when diffP > diffM and diffP > 0: the select takes the
     * first test and the max the second, as a non-positive delta cannot raise
     * the sum. gcc keeps a branch if a select, not the max, ends the step.
     */
    tempReal = diffP - diffM;
    plusDM1 = (tempReal > 0.0) ? diffP : 0.0;
    tempReal = prevPlusDM + plusDM1;
    prevPlusDM = (prevPlusDM > tempReal) ? prevPlusDM : tempReal;
  }
  /* Process subsequent DM */
  /* Skip the unstable period. */
  i = lookbackTotal - (optInTimePeriod - 1);
  while (i-- !== 0) {
    today += 1;
    tempReal = inHigh[today];
    diffP = tempReal - prevHigh;
    /* Plus Delta */
    prevHigh = tempReal;
    tempReal = inLow[today];
    diffM = prevLow - tempReal;
    /* Minus Delta */
    prevLow = tempReal;
    tempReal = diffP - diffM;
    plusDM1 = (tempReal > 0.0) ? diffP : 0.0;
    tempReal = prevPlusDM - prevPlusDM * invPeriod;
    prevPlusDM = tempReal + plusDM1;
    prevPlusDM = (tempReal > prevPlusDM) ? tempReal : prevPlusDM;
  }
  /* Now start to write the output in
   * the caller provided outReal.
   */
  outReal[0] = prevPlusDM;
  outIdx = 1;
  while (today < endIdx) {
    today += 1;
    tempReal = inHigh[today];
    diffP = tempReal - prevHigh;
    /* Plus Delta */
    prevHigh = tempReal;
    tempReal = inLow[today];
    diffM = prevLow - tempReal;
    /* Minus Delta */
    prevLow = tempReal;
    tempReal = diffP - diffM;
    plusDM1 = (tempReal > 0.0) ? diffP : 0.0;
    tempReal = prevPlusDM - prevPlusDM * invPeriod;
    prevPlusDM = tempReal + plusDM1;
    prevPlusDM = (tempReal > prevPlusDM) ? tempReal : prevPlusDM;
    outReal[outIdx++] = prevPlusDM;
  }
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * Plus Directional Movement: the Wilder-smoothed accumulation of upward
 * directional movement (+DM1). A component of the Directional Movement System used
 * to build +DI/DX/ADX.
 *
 * Formula and more info at
 * [ta-lib.org/functions/plus_dm](https://ta-lib.org/functions/plus_dm).
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link plusDmLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param optInTimePeriod - Wilder smoothing period (default 14; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Smoothed plus directional movement. Must hold the values
 *   produced and never be empty; the input length always suffices.
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
 * import { plusDm } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(high.length);
 * const { begIdx, count } = plusDm(0, high.length - 1, high, low, 14, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`minusDm`](https://ta-lib.org/functions/minus_dm)
 * @see [`plusDi`](https://ta-lib.org/functions/plus_di)
 * @see [`minusDi`](https://ta-lib.org/functions/minus_di)
 * @see [`dx`](https://ta-lib.org/functions/dx)
 * @see [`adx`](https://ta-lib.org/functions/adx)
 * @see [`adxr`](https://ta-lib.org/functions/adxr)
 */
export function plusDm(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('PLUS_DM', 'startIdx', startIdx);
  endIdx = requireInt32('PLUS_DM', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('PLUS_DM', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('PLUS_DM', startIdx, endIdx);
  const guardStart = clampedStart('PLUS_DM', startIdx, plusDmLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('PLUS_DM', 'inHigh', inHigh, guardInLen);
  requireLength('PLUS_DM', 'inLow', inLow, guardInLen);
  requireOutput('PLUS_DM', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlapsPartly(outReal, inHighF64) || overlapsPartly(outReal, inLowF64)) {
    throw failure('PLUS_DM', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = plusDmImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('PLUS_DM', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live PLUS_DM stream: one value per closed bar, bit-identical to {@link plusDm}
 * over the same series. Open it with {@link plusDmOpen} or
 * {@link plusDmOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/plus_dm](https://ta-lib.org/functions/plus_dm).
 */
export class PlusDmStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  prevHigh: number = 0;
  /** @internal */
  prevLow: number = 0;
  /** @internal */
  invPeriod: number = 0;
  /** @internal */
  prevPlusDM: number = 0;
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
  static of(core: Core): PlusDmStream {
    return new PlusDmStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link plusDm} reports them over the same bars. Each accepted `update` and each
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
      throw failure('PLUS_DM advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw failure('PLUS_DM update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('PLUS_DM update', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    plusDmStepImpl(this, inHigh, inLow);
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
      throw nonFiniteBar('PLUS_DM peek', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    const sp = this;
    let cur_outReal = 0;
    if (sp.optInTimePeriod <= 1) {
      let tempReal = 0;
      let diffP = 0;
      let diffM = 0;
      let plusDM1 = 0;
      let prevHigh = sp.prevHigh;
      let prevLow = sp.prevLow;
      tempReal = inHigh;
      diffP = tempReal - prevHigh;
      /* Plus Delta */
      prevHigh = tempReal;
      tempReal = inLow;
      diffM = prevLow - tempReal;
      /* Minus Delta */
      prevLow = tempReal;
      plusDM1 = (diffP > 0.0) ? diffP : 0.0;
      plusDM1 = (diffP > diffM) ? plusDM1 : 0.0;
      cur_outReal = plusDM1;
    } else {
      let tempReal = 0;
      let diffP = 0;
      let diffM = 0;
      let plusDM1 = 0;
      let prevHigh = sp.prevHigh;
      let prevLow = sp.prevLow;
      let prevPlusDM = sp.prevPlusDM;
      tempReal = inHigh;
      diffP = tempReal - prevHigh;
      /* Plus Delta */
      prevHigh = tempReal;
      tempReal = inLow;
      diffM = prevLow - tempReal;
      /* Minus Delta */
      prevLow = tempReal;
      tempReal = diffP - diffM;
      plusDM1 = (tempReal > 0.0) ? diffP : 0.0;
      tempReal = prevPlusDM - prevPlusDM * sp.invPeriod;
      prevPlusDM = tempReal + plusDM1;
      prevPlusDM = (tempReal > prevPlusDM) ? tempReal : prevPlusDM;
      cur_outReal = prevPlusDM;
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
  clone(): PlusDmStream {
    const c = new PlusDmStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.prevHigh = this.prevHigh;
    c.prevLow = this.prevLow;
    c.invPeriod = this.invPeriod;
    c.prevPlusDM = this.prevPlusDM;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function plusDmStepImpl(sp: PlusDmStream, inHigh: number, inLow: number): void {
  if (sp.optInTimePeriod <= 1) {
    let tempReal = 0;
    let diffP = 0;
    let diffM = 0;
    let plusDM1 = 0;
    tempReal = inHigh;
    diffP = tempReal - sp.prevHigh;
    /* Plus Delta */
    sp.prevHigh = tempReal;
    tempReal = inLow;
    diffM = sp.prevLow - tempReal;
    /* Minus Delta */
    sp.prevLow = tempReal;
    plusDM1 = (diffP > 0.0) ? diffP : 0.0;
    plusDM1 = (diffP > diffM) ? plusDM1 : 0.0;
    sp.cur_outReal = plusDM1;
  } else {
    let tempReal = 0;
    let diffP = 0;
    let diffM = 0;
    let plusDM1 = 0;
    tempReal = inHigh;
    diffP = tempReal - sp.prevHigh;
    /* Plus Delta */
    sp.prevHigh = tempReal;
    tempReal = inLow;
    diffM = sp.prevLow - tempReal;
    /* Minus Delta */
    sp.prevLow = tempReal;
    tempReal = diffP - diffM;
    plusDM1 = (tempReal > 0.0) ? diffP : 0.0;
    tempReal = sp.prevPlusDM - sp.prevPlusDM * sp.invPeriod;
    sp.prevPlusDM = tempReal + plusDM1;
    sp.prevPlusDM = (tempReal > sp.prevPlusDM) ? tempReal : sp.prevPlusDM;
    sp.cur_outReal = sp.prevPlusDM;
  }
}

function plusDmOpenImpl(
  sp: PlusDmStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
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
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod <= 1) {
    let today = 0;
    let lookbackTotal = 0;
    let outIdx = 0;
    let prevHigh = 0;
    let prevLow = 0;
    let tempReal = 0;
    let invPeriod = 0;
    let prevPlusDM = 0;
    let diffP = 0;
    let diffM = 0;
    let plusDM1 = 0;
    /*
     * The DM1 (one period) is base on the largest part of
     * today's range that is outside of yesterdays range.
     *
     * The following 7 cases explain how the +DM and -DM are
     * calculated on one period:
     *
     * Case 1:                       Case 2:
     *    C|                        A|
     *     |                         | C|
     *     | +DM1 = (C-A)           B|  | +DM1 = 0
     *     | -DM1 = 0                   | -DM1 = (B-D)
     * A|  |                           D|
     *  | D|
     * B|
     *
     * Case 3:                       Case 4:
     *    C|                           C|
     *     |                        A|  |
     *     | +DM1 = (C-A)            |  | +DM1 = 0
     *     | -DM1 = 0               B|  | -DM1 = (B-D)
     * A|  |                            |
     *  |  |                           D|
     * B|  |
     *    D|
     *
     * Case 5:                      Case 6:
     * A|                           A| C|
     *  | C| +DM1 = 0                |  |  +DM1 = 0
     *  |  | -DM1 = 0                |  |  -DM1 = 0
     *  | D|                         |  |
     * B|                           B| D|
     *
     *
     * Case 7:
     *
     *    C|
     * A|  |
     *  |  | +DM=0
     * B|  | -DM=0
     *    D|
     *
     * In case 3 and 4, the rule is that the smallest delta between
     * (C-A) and (B-D) determine which of +DM or -DM is zero.
     *
     * In case 7, (C-A) and (B-D) are equal, so both +DM and -DM are
     * zero.
     *
     * The rules remain the same when A=B and C=D (when the highs
     * equal the lows).
     *
     * When calculating the DM over a period > 1, the one-period DM
     * for the desired period are initialy sum. In other word,
     * for a +DM14, sum the +DM1 for the first 14 days (that's
     * 13 values because there is no DM for the first day!)
     * Subsequent DM are calculated using the Wilder's
     * smoothing approach:
     *
     *                                    Previous +DM14
     *  Today's +DM14 = Previous +DM14 -  -------------- + Today's +DM1
     *                                         14
     *
     * Reference:
     *    New Concepts In Technical Trading Systems, J. Welles Wilder Jr
     */
    lookbackTotal = plusDmLookback(optInTimePeriod, core);
    /* Adjust startIdx to account for the lookback period. */
    if (startIdx < lookbackTotal) {
      startIdx = lookbackTotal;
    }
    /* Make sure there is still something to evaluate. */
    if (startIdx > endIdx) {
      outBegIdx.value = 0;
      outNBElement.value = 0;
      return RetCode.INSUFFICIENT_HISTORY;
    }
    /* Indicate where the next output should be put
     * in the outReal.
     */
    outIdx = 0;
    /* Trap the case where no smoothing is needed. */
    /* No smoothing needed. Just do a simple DM1
     * for each price bar.
     */
    outBegIdx.value = startIdx;
    today = startIdx - 1;
    prevHigh = inHigh[today];
    prevLow = inLow[today];
    while (today < endIdx) {
      today += 1;
      tempReal = inHigh[today];
      diffP = tempReal - prevHigh;
      /* Plus Delta */
      prevHigh = tempReal;
      tempReal = inLow[today];
      diffM = prevLow - tempReal;
      /* Minus Delta */
      prevLow = tempReal;
      plusDM1 = (diffP > 0.0) ? diffP : 0.0;
      plusDM1 = (diffP > diffM) ? plusDM1 : 0.0;
      outReal[((outIdx++ * outStride) | 0)] = plusDM1;
    }
    outNBElement.value = outIdx;
    sp.optInTimePeriod = optInTimePeriod;
    sp.prevHigh = prevHigh;
    sp.prevLow = prevLow;
    sp.invPeriod = invPeriod;
    sp.prevPlusDM = prevPlusDM;
    sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
    return RetCode.SUCCESS;
  } else {
    let today = 0;
    let lookbackTotal = 0;
    let outIdx = 0;
    let prevHigh = 0;
    let prevLow = 0;
    let tempReal = 0;
    let invPeriod = 0;
    let prevPlusDM = 0;
    let diffP = 0;
    let diffM = 0;
    let plusDM1 = 0;
    let i = 0;
    /*
     * The DM1 (one period) is base on the largest part of
     * today's range that is outside of yesterdays range.
     *
     * The following 7 cases explain how the +DM and -DM are
     * calculated on one period:
     *
     * Case 1:                       Case 2:
     *    C|                        A|
     *     |                         | C|
     *     | +DM1 = (C-A)           B|  | +DM1 = 0
     *     | -DM1 = 0                   | -DM1 = (B-D)
     * A|  |                           D|
     *  | D|
     * B|
     *
     * Case 3:                       Case 4:
     *    C|                           C|
     *     |                        A|  |
     *     | +DM1 = (C-A)            |  | +DM1 = 0
     *     | -DM1 = 0               B|  | -DM1 = (B-D)
     * A|  |                            |
     *  |  |                           D|
     * B|  |
     *    D|
     *
     * Case 5:                      Case 6:
     * A|                           A| C|
     *  | C| +DM1 = 0                |  |  +DM1 = 0
     *  |  | -DM1 = 0                |  |  -DM1 = 0
     *  | D|                         |  |
     * B|                           B| D|
     *
     *
     * Case 7:
     *
     *    C|
     * A|  |
     *  |  | +DM=0
     * B|  | -DM=0
     *    D|
     *
     * In case 3 and 4, the rule is that the smallest delta between
     * (C-A) and (B-D) determine which of +DM or -DM is zero.
     *
     * In case 7, (C-A) and (B-D) are equal, so both +DM and -DM are
     * zero.
     *
     * The rules remain the same when A=B and C=D (when the highs
     * equal the lows).
     *
     * When calculating the DM over a period > 1, the one-period DM
     * for the desired period are initialy sum. In other word,
     * for a +DM14, sum the +DM1 for the first 14 days (that's
     * 13 values because there is no DM for the first day!)
     * Subsequent DM are calculated using the Wilder's
     * smoothing approach:
     *
     *                                    Previous +DM14
     *  Today's +DM14 = Previous +DM14 -  -------------- + Today's +DM1
     *                                         14
     *
     * Reference:
     *    New Concepts In Technical Trading Systems, J. Welles Wilder Jr
     */
    lookbackTotal = plusDmLookback(optInTimePeriod, core);
    /* Adjust startIdx to account for the lookback period. */
    if (startIdx < lookbackTotal) {
      startIdx = lookbackTotal;
    }
    /* Make sure there is still something to evaluate. */
    if (startIdx > endIdx) {
      outBegIdx.value = 0;
      outNBElement.value = 0;
      return RetCode.INSUFFICIENT_HISTORY;
    }
    /* Indicate where the next output should be put
     * in the outReal.
     */
    outIdx = 0;
    /* Trap the case where no smoothing is needed. */
    invPeriod = 1.0 / optInTimePeriod;
    /* Process the initial DM */
    outBegIdx.value = startIdx;
    prevPlusDM = 0.0;
    today = startIdx - lookbackTotal;
    prevHigh = inHigh[today];
    prevLow = inLow[today];
    i = optInTimePeriod - 1;
    while (i-- > 0) {
      today += 1;
      tempReal = inHigh[today];
      diffP = tempReal - prevHigh;
      /* Plus Delta */
      prevHigh = tempReal;
      tempReal = inLow[today];
      diffM = prevLow - tempReal;
      /* Minus Delta */
      prevLow = tempReal;
      /* +DM1 = diffP when diffP > diffM and diffP > 0: the select takes the
       * first test and the max the second, as a non-positive delta cannot raise
       * the sum. gcc keeps a branch if a select, not the max, ends the step.
       */
      tempReal = diffP - diffM;
      plusDM1 = (tempReal > 0.0) ? diffP : 0.0;
      tempReal = prevPlusDM + plusDM1;
      prevPlusDM = (prevPlusDM > tempReal) ? prevPlusDM : tempReal;
    }
    /* Process subsequent DM */
    /* Skip the unstable period. */
    i = lookbackTotal - (optInTimePeriod - 1);
    while (i-- !== 0) {
      today += 1;
      tempReal = inHigh[today];
      diffP = tempReal - prevHigh;
      /* Plus Delta */
      prevHigh = tempReal;
      tempReal = inLow[today];
      diffM = prevLow - tempReal;
      /* Minus Delta */
      prevLow = tempReal;
      tempReal = diffP - diffM;
      plusDM1 = (tempReal > 0.0) ? diffP : 0.0;
      tempReal = prevPlusDM - prevPlusDM * invPeriod;
      prevPlusDM = tempReal + plusDM1;
      prevPlusDM = (tempReal > prevPlusDM) ? tempReal : prevPlusDM;
    }
    /* Now start to write the output in
     * the caller provided outReal.
     */
    outReal[((0 * outStride) | 0)] = prevPlusDM;
    outIdx = 1;
    while (today < endIdx) {
      today += 1;
      tempReal = inHigh[today];
      diffP = tempReal - prevHigh;
      /* Plus Delta */
      prevHigh = tempReal;
      tempReal = inLow[today];
      diffM = prevLow - tempReal;
      /* Minus Delta */
      prevLow = tempReal;
      tempReal = diffP - diffM;
      plusDM1 = (tempReal > 0.0) ? diffP : 0.0;
      tempReal = prevPlusDM - prevPlusDM * invPeriod;
      prevPlusDM = tempReal + plusDM1;
      prevPlusDM = (tempReal > prevPlusDM) ? tempReal : prevPlusDM;
      outReal[((outIdx++ * outStride) | 0)] = prevPlusDM;
    }
    outNBElement.value = outIdx;
    sp.optInTimePeriod = optInTimePeriod;
    sp.prevHigh = prevHigh;
    sp.prevLow = prevLow;
    sp.invPeriod = invPeriod;
    sp.prevPlusDM = prevPlusDM;
    sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
    return RetCode.SUCCESS;
  }
}

/** @internal */
export function plusDmOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): PlusDmStream {
  const sp = PlusDmStream.of(core);
  const retCode = plusDmOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('PLUS_DM openAndFill', inHigh.length, startIdx, plusDmLookback(optInTimePeriod, core));
  }
  throw streamFailure('PLUS_DM openAndFill', retCode);
}

/** @internal */
export function plusDmOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): PlusDmStream {
  const sp = PlusDmStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = plusDmOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('PLUS_DM open', inHigh.length, startIdx, plusDmLookback(optInTimePeriod, core));
  }
  throw streamFailure('PLUS_DM open', retCode);
}

/**
 * Opens a live PLUS_DM stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link plusDm} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Wilder smoothing period (default 14; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `plusDmLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for an integer parameter that is not an int32, a parameter outside its range or
 *   histories of different lengths.
 *
 * @example
 * ```ts
 * import { plusDmOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = plusDmOpen(high, low, 14);
 * const peeked = stream.peek(101.4, 99.1); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1); // the bar closes
 * ```
 */
export function plusDmOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): PlusDmStream {
  optInTimePeriod = requireInt32('PLUS_DM open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('PLUS_DM open', 'inHigh', inHigh);
  requireHistory('PLUS_DM open', inHigh.length);
  requireArgument('PLUS_DM open', 'inLow', inLow);
  requireHistoryLength('PLUS_DM open', 'inLow', inLow.length, inHigh.length);
  return plusDmOpenInternal(core, float64(inHigh), float64(inLow), 0, optInTimePeriod);
}

/**
 * {@link plusDmOpen} that also fills the outputs over the whole history in the
 * same pass, bit-identically to {@link plusDm}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Wilder smoothing period (default 14; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Smoothed plus directional movement. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `plusDmLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or an output too short or starting where another
 *   output or a `Float64Array` input starts, or, on a history long enough to open,
 *   one sharing memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { plusDmOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(high.length);
 * const stream = plusDmOpenAndFill(high, low, 14, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function plusDmOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): PlusDmStream {
  optInTimePeriod = requireInt32('PLUS_DM openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('PLUS_DM openAndFill', 'inHigh', inHigh);
  requireHistory('PLUS_DM openAndFill', inHigh.length);
  requireArgument('PLUS_DM openAndFill', 'inLow', inLow);
  const guardOutLen = openFillCount('PLUS_DM openAndFill', inHigh.length, plusDmLookback(optInTimePeriod, core));
  requireHistoryLength('PLUS_DM openAndFill', 'inLow', inLow.length, inHigh.length);
  requireOutput('PLUS_DM openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if ((outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) || (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset)) {
    throw streamFailure('PLUS_DM openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inHighF64) || overlaps(outReal, inLowF64))) {
    throw streamFailure('PLUS_DM openAndFill', RetCode.BAD_PARAM);
  }
  return plusDmOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}
