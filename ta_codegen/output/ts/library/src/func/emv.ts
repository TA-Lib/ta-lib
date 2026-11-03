/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin (@kevinlincg)
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  092826 KL,CC  First version (#465).
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireReal, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { smaLookback } from './sma.ts';

/**
 * Number of leading input bars {@link emv} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Number of one-bar values in the simple moving average
 *   (default 14; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInVolumeDivisor - Volume is divided by this before it forms the box
 *   ratio (default 10000; minimum 1; `Core.REAL_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function emvLookback(optInTimePeriod: number, optInVolumeDivisor: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (typeof optInVolumeDivisor !== 'number') {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  if (optInVolumeDivisor === Core.REAL_DEFAULT) {
    optInVolumeDivisor = 10000.0;
  } else if (!(optInVolumeDivisor >= 1e0 && optInVolumeDivisor <= Core.REAL_MAX)) {
    return -1;
  }
  return 1 + smaLookback(optInTimePeriod, core);
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link emv}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Number of one-bar values in the simple moving average
 *   (default 14; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInVolumeDivisor - Volume is divided by this before it forms the box
 *   ratio (default 10000; minimum 1; `Core.REAL_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function emvDisplayShift(optInTimePeriod: number, optInVolumeDivisor: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (emvLookback(optInTimePeriod, optInVolumeDivisor, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function emvImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inVolume: Float64Array,
  optInTimePeriod: number,
  optInVolumeDivisor: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let periodTotal = 0;
  let prevMid = 0;
  let mid = 0;
  let range = 0;
  let boxRatio = 0;
  let raw = 0;
  let tempReal = 0;
  let lookbackTotal = 0;
  let outIdx = 0;
  let i = 0;
  let rawBuffer!: Float64Array;
  let rawBuffer_Idx = 0;
  let maxIdx_rawBuffer = (50) - 1;
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
  if (optInVolumeDivisor === Core.REAL_DEFAULT) {
    optInVolumeDivisor = 10000.0;
  } else if (!(optInVolumeDivisor >= 1e0 && optInVolumeDivisor <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  /* The one-bar values are kept in a ring rather than recomputed at the
   * trailing index: outReal may alias an input, and those bars may already
   * hold outputs.
   */
  lookbackTotal = emvLookback(optInTimePeriod, optInVolumeDivisor, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  rawBuffer = new Float64Array(optInTimePeriod);
  maxIdx_rawBuffer = optInTimePeriod - 1;
  rawBuffer_Idx = 0;
  /* Keep the operand order and sma.c's add / snapshot / subtract order: the
   * composite gate rebuilds this from MEDPRICE, MOM, SUB, DIV and SMA and
   * compares bitwise.
   *
   * The zero guards are exact != 0.0 tests, never TA_IS_ZERO: the output
   * scale follows the volume and the divisor, so no absolute band fits. A
   * nonzero range with a box ratio that underflows to 0 takes the guard too.
   */
  i = startIdx - lookbackTotal;
  prevMid = (inHigh[i] + inLow[i]) / 2.0;
  i = i + 1;
  periodTotal = 0.0;
  while (i < startIdx) {
    mid = (inHigh[i] + inLow[i]) / 2.0;
    range = inHigh[i] - inLow[i];
    raw = 0.0;
    if (range !== 0.0) {
      boxRatio = inVolume[i] / optInVolumeDivisor / range;
      if (boxRatio !== 0.0) {
        raw = (mid - prevMid) / boxRatio;
      }
    }
    prevMid = mid;
    i = i + 1;
    rawBuffer[rawBuffer_Idx] = raw;
    periodTotal += raw;
    rawBuffer_Idx++;
    if (rawBuffer_Idx > maxIdx_rawBuffer) {
      rawBuffer_Idx = 0;
    }
  }
  outIdx = 0;
  while (i <= endIdx) {
    mid = (inHigh[i] + inLow[i]) / 2.0;
    range = inHigh[i] - inLow[i];
    raw = 0.0;
    if (range !== 0.0) {
      boxRatio = inVolume[i] / optInVolumeDivisor / range;
      if (boxRatio !== 0.0) {
        raw = (mid - prevMid) / boxRatio;
      }
    }
    prevMid = mid;
    i = i + 1;
    rawBuffer[rawBuffer_Idx] = raw;
    periodTotal += raw;
    tempReal = periodTotal;
    rawBuffer_Idx++;
    if (rawBuffer_Idx > maxIdx_rawBuffer) {
      rawBuffer_Idx = 0;
    }
    periodTotal -= rawBuffer[rawBuffer_Idx];
    outReal[outIdx] = tempReal / optInTimePeriod;
    outIdx = outIdx + 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * Ease of Movement: the bar-to-bar move of the high-low midpoint divided by a box
 * ratio of volume to range, averaged over a trailing window. It is the numeric
 * form of Richard W. Arms, Jr.'s Equivolume box. The box ratio is positive
 * whenever the bar traded and has a range, so the sign follows the midpoint move.
 * A large positive value means price rose easily, on light volume relative to its
 * range; a large negative value means it fell easily. Values near zero mean volume
 * was heavy for the distance travelled, or price hardly moved. The output scales
 * with the instrument's volume and with the volume divisor, so its level is
 * comparable only within one instrument at one divisor. Traders mostly watch its
 * sign and its zero crossings.
 *
 * Formula and more info at
 * [ta-lib.org/functions/emv](https://ta-lib.org/functions/emv).
 *
 * **Notes**
 * - The range is in price points. Achelis's text gives it in eighths of a point,
 *   the pre-decimal US quote unit; that reading is reached by multiplying the
 *   divisor by 8.
 * - The divisor is a pure output scale: doubling it doubles every value. Pick one
 *   that suits the instrument's volume.
 * - A period of 1 returns the unsmoothed one-bar values. Smoothing is a simple
 *   moving average; for an exponential one, apply `EMA` to this function's output
 *   at a period of 1.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link emvLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inVolume - Volume of each bar.
 * @param optInTimePeriod - Number of one-bar values in the simple moving average
 *   (default 14; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInVolumeDivisor - Volume is divided by this before it forms the box
 *   ratio (default 10000; minimum 1; `Core.REAL_DEFAULT` selects the default).
 * @param outReal - Ease of Movement, averaged over the window. Must hold the
 *   values produced and never be empty; the input length always suffices.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The range written: `begIdx` is the first bar with a value, `count` how
 *   many were written.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for an argument that is not an int32, a
 *   real parameter that is not a number, a parameter outside its range, an array too
 *   short for the range, an empty output or an output sharing memory with a
 *   `Float64Array` input it is not. Checked before anything is written.
 *
 * @example
 * ```ts
 * import { emv } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const out = new Float64Array(high.length);
 * const { begIdx, count } = emv(
 *   0, high.length - 1,
 *   high, low, volume,
 *   14, 10000,
 *   out,
 * );
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`medprice`](https://ta-lib.org/functions/medprice)
 * @see [`marketfi`](https://ta-lib.org/functions/marketfi)
 * @see [`efi`](https://ta-lib.org/functions/efi)
 * @see [`sma`](https://ta-lib.org/functions/sma)
 * @see [`ema`](https://ta-lib.org/functions/ema)
 */
export function emv(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  optInTimePeriod: number,
  optInVolumeDivisor: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('EMV', 'startIdx', startIdx);
  endIdx = requireInt32('EMV', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('EMV', 'optInTimePeriod', optInTimePeriod);
  optInVolumeDivisor = requireReal('EMV', 'optInVolumeDivisor', optInVolumeDivisor);
  requireIndexRange('EMV', startIdx, endIdx);
  const guardStart = clampedStart('EMV', startIdx, emvLookback(optInTimePeriod, optInVolumeDivisor, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('EMV', 'inHigh', inHigh, guardInLen);
  requireLength('EMV', 'inLow', inLow, guardInLen);
  requireLength('EMV', 'inVolume', inVolume, guardInLen);
  requireLength('EMV', 'outReal', outReal, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inVolumeF64 = float64(inVolume);
  if (overlapsPartly(outReal, inHighF64) || overlapsPartly(outReal, inLowF64) || overlapsPartly(outReal, inVolumeF64)) {
    throw failure('EMV', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = emvImpl(core, startIdx, endIdx, inHighF64, inLowF64, inVolumeF64, optInTimePeriod, optInVolumeDivisor, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('EMV', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live EMV stream: one value per closed bar, bit-identical to {@link emv} over
 * the same series. Open it with {@link emvOpen} or {@link emvOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/emv](https://ta-lib.org/functions/emv).
 */
export class EmvStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  optInVolumeDivisor: number = 0;
  /** @internal */
  periodTotal: number = 0;
  /** @internal */
  prevMid: number = 0;
  /** @internal */
  rawBuffer_Idx: number = 0;
  /** @internal */
  maxIdx_rawBuffer: number = 0;
  /** @internal */
  cbSize_rawBuffer: number = 0;
  /** @internal */
  cb_rawBuffer: Float64Array = new Float64Array(1);
  /** @internal */
  cur_outReal: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  /** @internal */
  constructor(core: Core) {
    this.core = core;
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link emv} reports them over the same bars. Each accepted `update` and each
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
      throw failure('EMV advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inVolume - The bar's `inVolume` value: Volume of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number, inVolume: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('EMV update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inVolume)) {
      throw nonFiniteBar('EMV update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inVolume');
    }
    emvStepImpl(this, inHigh, inLow, inVolume);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inVolume - The bar's `inVolume` value: Volume of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number, inVolume: number): number {
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inVolume)) {
      throw nonFiniteBar('EMV peek', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inVolume');
    }
    const sp = this;
    let mid = 0;
    let range = 0;
    let boxRatio = 0;
    let raw = 0;
    let tempReal = 0;
    let cur_outReal = 0;
    let periodTotal = sp.periodTotal;
    let prevMid = sp.prevMid;
    let rawBuffer_Idx = sp.rawBuffer_Idx;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    mid = (inHigh + inLow) / 2.0;
    range = inHigh - inLow;
    raw = 0.0;
    if (range !== 0.0) {
      boxRatio = inVolume / sp.optInVolumeDivisor / range;
      if (boxRatio !== 0.0) {
        raw = (mid - prevMid) / boxRatio;
      }
    }
    prevMid = mid;
    pkSlot0 = rawBuffer_Idx;
    pkVal0 = raw;
    periodTotal += raw;
    tempReal = periodTotal;
    rawBuffer_Idx = rawBuffer_Idx + 1;
    if (rawBuffer_Idx > sp.maxIdx_rawBuffer) {
      rawBuffer_Idx = 0;
    }
    periodTotal -= (rawBuffer_Idx !== pkSlot0) ? sp.cb_rawBuffer[rawBuffer_Idx] : pkVal0;
    cur_outReal = tempReal / sp.optInTimePeriod;
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
  clone(): EmvStream {
    const c = new EmvStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.optInVolumeDivisor = this.optInVolumeDivisor;
    c.periodTotal = this.periodTotal;
    c.prevMid = this.prevMid;
    c.rawBuffer_Idx = this.rawBuffer_Idx;
    c.maxIdx_rawBuffer = this.maxIdx_rawBuffer;
    c.cbSize_rawBuffer = this.cbSize_rawBuffer;
    c.cb_rawBuffer = this.cb_rawBuffer.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function emvStepImpl(sp: EmvStream, inHigh: number, inLow: number, inVolume: number): void {
  let mid = 0;
  let range = 0;
  let boxRatio = 0;
  let raw = 0;
  let tempReal = 0;
  mid = (inHigh + inLow) / 2.0;
  range = inHigh - inLow;
  raw = 0.0;
  if (range !== 0.0) {
    boxRatio = inVolume / sp.optInVolumeDivisor / range;
    if (boxRatio !== 0.0) {
      raw = (mid - sp.prevMid) / boxRatio;
    }
  }
  sp.prevMid = mid;
  sp.cb_rawBuffer[sp.rawBuffer_Idx] = raw;
  sp.periodTotal += raw;
  tempReal = sp.periodTotal;
  sp.rawBuffer_Idx = sp.rawBuffer_Idx + 1;
  if (sp.rawBuffer_Idx > sp.maxIdx_rawBuffer) {
    sp.rawBuffer_Idx = 0;
  }
  sp.periodTotal -= sp.cb_rawBuffer[sp.rawBuffer_Idx];
  sp.cur_outReal = tempReal / sp.optInTimePeriod;
}

function emvOpenImpl(
  sp: EmvStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInVolumeDivisor: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let periodTotal = 0;
  let prevMid = 0;
  let mid = 0;
  let range = 0;
  let boxRatio = 0;
  let raw = 0;
  let tempReal = 0;
  let lookbackTotal = 0;
  let outIdx = 0;
  let i = 0;
  let rawBuffer!: Float64Array;
  let rawBuffer_Idx = 0;
  let maxIdx_rawBuffer = (50) - 1;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length || inVolume.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInVolumeDivisor === Core.REAL_DEFAULT) {
    optInVolumeDivisor = 10000.0;
  } else if (!(optInVolumeDivisor >= 1e0 && optInVolumeDivisor <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* The one-bar values are kept in a ring rather than recomputed at the
   * trailing index: outReal may alias an input, and those bars may already
   * hold outputs.
   */
  lookbackTotal = emvLookback(optInTimePeriod, optInVolumeDivisor, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  rawBuffer = new Float64Array(optInTimePeriod);
  maxIdx_rawBuffer = optInTimePeriod - 1;
  rawBuffer_Idx = 0;
  /* Keep the operand order and sma.c's add / snapshot / subtract order: the
   * composite gate rebuilds this from MEDPRICE, MOM, SUB, DIV and SMA and
   * compares bitwise.
   *
   * The zero guards are exact != 0.0 tests, never TA_IS_ZERO: the output
   * scale follows the volume and the divisor, so no absolute band fits. A
   * nonzero range with a box ratio that underflows to 0 takes the guard too.
   */
  i = startIdx - lookbackTotal;
  prevMid = (inHigh[i] + inLow[i]) / 2.0;
  i = i + 1;
  periodTotal = 0.0;
  while (i < startIdx) {
    mid = (inHigh[i] + inLow[i]) / 2.0;
    range = inHigh[i] - inLow[i];
    raw = 0.0;
    if (range !== 0.0) {
      boxRatio = inVolume[i] / optInVolumeDivisor / range;
      if (boxRatio !== 0.0) {
        raw = (mid - prevMid) / boxRatio;
      }
    }
    prevMid = mid;
    i = i + 1;
    rawBuffer[rawBuffer_Idx] = raw;
    periodTotal += raw;
    rawBuffer_Idx++;
    if (rawBuffer_Idx > maxIdx_rawBuffer) {
      rawBuffer_Idx = 0;
    }
  }
  outIdx = 0;
  while (i <= endIdx) {
    mid = (inHigh[i] + inLow[i]) / 2.0;
    range = inHigh[i] - inLow[i];
    raw = 0.0;
    if (range !== 0.0) {
      boxRatio = inVolume[i] / optInVolumeDivisor / range;
      if (boxRatio !== 0.0) {
        raw = (mid - prevMid) / boxRatio;
      }
    }
    prevMid = mid;
    i = i + 1;
    rawBuffer[rawBuffer_Idx] = raw;
    periodTotal += raw;
    tempReal = periodTotal;
    rawBuffer_Idx++;
    if (rawBuffer_Idx > maxIdx_rawBuffer) {
      rawBuffer_Idx = 0;
    }
    periodTotal -= rawBuffer[rawBuffer_Idx];
    outReal[((outIdx * outStride) | 0)] = tempReal / optInTimePeriod;
    outIdx = outIdx + 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  const capCb_rawBuffer = maxIdx_rawBuffer + 1;
  if (capCb_rawBuffer > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.optInVolumeDivisor = optInVolumeDivisor;
  sp.periodTotal = periodTotal;
  sp.prevMid = prevMid;
  sp.rawBuffer_Idx = rawBuffer_Idx;
  sp.maxIdx_rawBuffer = maxIdx_rawBuffer;
  sp.cbSize_rawBuffer = capCb_rawBuffer;
  sp.cb_rawBuffer = rawBuffer;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function emvOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInVolumeDivisor: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): EmvStream {
  const sp = new EmvStream(core);
  const retCode = emvOpenImpl(sp, inHigh, inLow, inVolume, startIdx, optInTimePeriod, optInVolumeDivisor, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('EMV openAndFill', inHigh.length, startIdx, emvLookback(optInTimePeriod, optInVolumeDivisor, core));
  }
  throw streamFailure('EMV openAndFill', retCode);
}

/** @internal */
export function emvOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInVolumeDivisor: number,
): EmvStream {
  const sp = new EmvStream(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = emvOpenImpl(sp, inHigh, inLow, inVolume, startIdx, optInTimePeriod, optInVolumeDivisor, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('EMV open', inHigh.length, startIdx, emvLookback(optInTimePeriod, optInVolumeDivisor, core));
  }
  throw streamFailure('EMV open', retCode);
}

/**
 * Opens a live EMV stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link emv} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inVolume - Warm-up history. Volume of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Number of one-bar values in the simple moving average
 *   (default 14; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInVolumeDivisor - Volume is divided by this before it forms the box
 *   ratio (default 10000; minimum 1; `Core.REAL_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `emvLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a real
 *   parameter that is not a number, a parameter outside its range or histories of
 *   different lengths.
 *
 * @example
 * ```ts
 * import { emvOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const stream = emvOpen(high, low, volume, 14, 10000);
 * const peeked = stream.peek(101.4, 99.1, 12345); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 12345); // the bar closes
 * ```
 */
export function emvOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  optInTimePeriod: number,
  optInVolumeDivisor: number,
  core: Core = Core.DEFAULT,
): EmvStream {
  optInTimePeriod = requireInt32('EMV open', 'optInTimePeriod', optInTimePeriod);
  optInVolumeDivisor = requireReal('EMV open', 'optInVolumeDivisor', optInVolumeDivisor);
  requireArgument('EMV open', 'inHigh', inHigh);
  requireHistory('EMV open', inHigh.length);
  requireArgument('EMV open', 'inLow', inLow);
  requireArgument('EMV open', 'inVolume', inVolume);
  requireHistoryLength('EMV open', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('EMV open', 'inVolume', inVolume.length, inHigh.length);
  return emvOpenInternal(core, float64(inHigh), float64(inLow), float64(inVolume), 0, optInTimePeriod, optInVolumeDivisor);
}

/**
 * {@link emvOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link emv}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inVolume - Warm-up history. Volume of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Number of one-bar values in the simple moving average
 *   (default 14; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInVolumeDivisor - Volume is divided by this before it forms the box
 *   ratio (default 10000; minimum 1; `Core.REAL_DEFAULT` selects the default).
 * @param outReal - Ease of Movement, averaged over the window. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `emvLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a real
 *   parameter that is not a number, a parameter outside its range, histories of
 *   different lengths or an output too short or starting where another output or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { emvOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const out = new Float64Array(high.length);
 * const stream = emvOpenAndFill(high, low, volume, 14, 10000, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function emvOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  optInTimePeriod: number,
  optInVolumeDivisor: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): EmvStream {
  optInTimePeriod = requireInt32('EMV openAndFill', 'optInTimePeriod', optInTimePeriod);
  optInVolumeDivisor = requireReal('EMV openAndFill', 'optInVolumeDivisor', optInVolumeDivisor);
  requireArgument('EMV openAndFill', 'inHigh', inHigh);
  requireHistory('EMV openAndFill', inHigh.length);
  requireArgument('EMV openAndFill', 'inLow', inLow);
  requireArgument('EMV openAndFill', 'inVolume', inVolume);
  const guardOutLen = openFillCount('EMV openAndFill', inHigh.length, emvLookback(optInTimePeriod, optInVolumeDivisor, core));
  requireHistoryLength('EMV openAndFill', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('EMV openAndFill', 'inVolume', inVolume.length, inHigh.length);
  requireLength('EMV openAndFill', 'outReal', outReal, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inVolumeF64 = float64(inVolume);
  if ((outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) || (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) || (outReal.buffer === inVolumeF64.buffer && outReal.byteOffset === inVolumeF64.byteOffset)) {
    throw streamFailure('EMV openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inHighF64) || overlaps(outReal, inLowF64) || overlaps(outReal, inVolumeF64))) {
    throw streamFailure('EMV openAndFill', RetCode.BAD_PARAM);
  }
  return emvOpenAndFillInternal(core, inHighF64, inLowF64, inVolumeF64, 0, optInTimePeriod, optInVolumeDivisor, new MInteger(), new MInteger(), outReal);
}
