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

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireReal, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { smaLookbackPositional } from './sma.ts';

/** @internal */
export function emvLookbackPositional(optInTimePeriod: number, optInVolumeDivisor: number, core: Core = Core.DEFAULT): number {
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
  return 1 + smaLookbackPositional(optInTimePeriod, core);
}

/** @internal */
export function emvDisplayShiftPositional(optInTimePeriod: number, optInVolumeDivisor: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (emvLookbackPositional(optInTimePeriod, optInVolumeDivisor, core) < 0) {
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
  lookbackTotal = emvLookbackPositional(optInTimePeriod, optInVolumeDivisor, core);
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

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['volumeDivisor', Core.REAL_DEFAULT, 1e0, Core.REAL_MAX, 'in 1..3e37']] as const;

/** @internal */
export function emvPositional(
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
  optInTimePeriod = requireInt32('EMV', 'timePeriod', optInTimePeriod);
  optInVolumeDivisor = requireReal('EMV', 'volumeDivisor', optInVolumeDivisor);
  requireIndexRange('EMV', startIdx, endIdx);
  const guardLookback = emvLookbackPositional(optInTimePeriod, optInVolumeDivisor, core);
  if (guardLookback < 0) {
    throw rejectedParams('EMV', PARAM_RANGES, optInTimePeriod, optInVolumeDivisor);
  }
  const guardStart = clampedStart('EMV', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('EMV', 'prices.high', inHigh, guardInLen);
  requireLength('EMV', 'prices.low', inLow, guardInLen);
  requireLength('EMV', 'prices.volume', inVolume, guardInLen);
  requireOutput('EMV', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inVolumeF64 = float64(inVolume);
  if (overlapsPartly(outReal, inHighF64)) {
    throw sharedMemory('EMV', 'outReal', 'prices.high');
  }
  if (overlapsPartly(outReal, inLowF64)) {
    throw sharedMemory('EMV', 'outReal', 'prices.low');
  }
  if (overlapsPartly(outReal, inVolumeF64)) {
    throw sharedMemory('EMV', 'outReal', 'prices.volume');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = emvImpl(core, startIdx, endIdx, inHighF64, inLowF64, inVolumeF64, optInTimePeriod, optInVolumeDivisor, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('EMV', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link emv}, {@link emvLookback}, {@link emvDisplayShift},
 * {@link emvOpen} and {@link emvOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface EmvOptions {
  /**
   * Number of one-bar values in the simple moving average (default 14; range 1 to
   * 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  timePeriod?: number | undefined;
  /**
   * Volume is divided by this before it forms the box ratio (default 10000; minimum
   * 1; `Core.REAL_DEFAULT` selects the default).
   */
  volumeDivisor?: number | undefined;
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
   * Buffer for `values`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outReal?: Float64Array<ArrayBuffer> | undefined;
}

/** What {@link emv} returns, and {@link emvOpenAndFill} with its stream. */
export interface EmvResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Ease of Movement, averaged over the window. `values[k]` is the value at input
   * bar `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link emv} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link EmvOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function emvLookback(options?: EmvOptions): number {
  const o = optionsOf('EMV lookback', options);
  return emvLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.volumeDivisor, Core.REAL_DEFAULT), coreOf('EMV lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link emv}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link EmvOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function emvDisplayShift(outputIdx: number, options?: EmvOptions): number {
  const o = optionsOf('EMV displayShift', options);
  return emvDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.volumeDivisor, Core.REAL_DEFAULT), outputIdx, coreOf('EMV displayShift', o.core));
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
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link emvLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `high`: High price of
 *   each bar. `low`: Low price of each bar. `volume`: Volume of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link EmvOptions}.
 * @returns `begIdx` and the values; see {@link EmvResult}. Each output is a view
 *   of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, an integer parameter that is not an int32, a real parameter that is not a
 *   number, a parameter outside its range, an input too short for the range, a
 *   buffer that is empty, too short or of the wrong type or a buffer sharing memory
 *   with a `Float64Array` input it is not. Checked before anything is written.
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
 * const { begIdx, values } = emv({ high, low, volume }, {
 *   timePeriod: 14,
 *   volumeDivisor: 10000,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`medprice`](https://ta-lib.org/functions/medprice)
 * @see [`marketfi`](https://ta-lib.org/functions/marketfi)
 * @see [`efi`](https://ta-lib.org/functions/efi)
 * @see [`sma`](https://ta-lib.org/functions/sma)
 * @see [`ema`](https://ta-lib.org/functions/ema)
 */
export function emv(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume: ArrayLike<number>; openInterest?: ArrayLike<number> | undefined }, options?: EmvOptions): EmvResult {
  requireArgument('EMV', 'prices', prices);
  const o = optionsOf('EMV', options);
  if (o.endIdx === undefined) {
    requireSeries('EMV', 'prices.high', prices.high);
    requireSeries('EMV', 'prices.low', prices.low);
    requireSeries('EMV', 'prices.volume', prices.volume);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInVolumeDivisor = orDefault(o.volumeDivisor, Core.REAL_DEFAULT);
  const core = coreOf('EMV', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, emvLookbackPositional(optInTimePeriod, optInVolumeDivisor, core), prices.high, prices.low, prices.volume);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = emvPositional(startIdx, endIdx, prices.high, prices.low, prices.volume, optInTimePeriod, optInVolumeDivisor, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
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

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): EmvStream {
    return new EmvStream(core);
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
      throw streamFailure('EMV advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param volume - The bar's `volume` value: Volume of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(high: number, low: number, volume: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('EMV update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(volume)) {
      throw nonFiniteBar('EMV update', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'volume');
    }
    emvStepImpl(this, high, low, volume);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param volume - The bar's `volume` value: Volume of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(high: number, low: number, volume: number): number {
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(volume)) {
      throw nonFiniteBar('EMV peek', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'volume');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
    const inVolume = volume;
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
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* The one-bar values are kept in a ring rather than recomputed at the
   * trailing index: outReal may alias an input, and those bars may already
   * hold outputs.
   */
  lookbackTotal = emvLookbackPositional(optInTimePeriod, optInVolumeDivisor, core);
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
  const sp = EmvStream.of(core);
  const retCode = emvOpenImpl(sp, inHigh, inLow, inVolume, startIdx, optInTimePeriod, optInVolumeDivisor, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('EMV openAndFill', inHigh.length, startIdx, emvLookbackPositional(optInTimePeriod, optInVolumeDivisor, core));
  }
  if (retCode === RetCode.BAD_PARAM && emvLookbackPositional(optInTimePeriod, optInVolumeDivisor, core) < 0) {
    throw rejectedParams('EMV openAndFill', PARAM_RANGES, optInTimePeriod, optInVolumeDivisor);
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
  const sp = EmvStream.of(core);
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
    throw insufficientHistory('EMV open', inHigh.length, startIdx, emvLookbackPositional(optInTimePeriod, optInVolumeDivisor, core));
  }
  if (retCode === RetCode.BAD_PARAM && emvLookbackPositional(optInTimePeriod, optInVolumeDivisor, core) < 0) {
    throw rejectedParams('EMV open', PARAM_RANGES, optInTimePeriod, optInVolumeDivisor);
  }
  throw streamFailure('EMV open', retCode);
}

/** @internal */
export function emvOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  optInTimePeriod: number,
  optInVolumeDivisor: number,
  core: Core = Core.DEFAULT,
): EmvStream {
  optInTimePeriod = requireInt32('EMV open', 'timePeriod', optInTimePeriod);
  optInVolumeDivisor = requireReal('EMV open', 'volumeDivisor', optInVolumeDivisor);
  requireArgument('EMV open', 'prices.high', inHigh);
  requireHistory('EMV open', 'prices.high', inHigh.length);
  requireArgument('EMV open', 'prices.low', inLow);
  requireArgument('EMV open', 'prices.volume', inVolume);
  requireHistoryLength('EMV open', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('EMV open', 'prices.volume', inVolume.length, inHigh.length);
  return emvOpenInternal(core, float64(inHigh), float64(inLow), float64(inVolume), 0, optInTimePeriod, optInVolumeDivisor);
}

/** @internal */
export function emvOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  optInTimePeriod: number,
  optInVolumeDivisor: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): EmvStream {
  optInTimePeriod = requireInt32('EMV openAndFill', 'timePeriod', optInTimePeriod);
  optInVolumeDivisor = requireReal('EMV openAndFill', 'volumeDivisor', optInVolumeDivisor);
  requireArgument('EMV openAndFill', 'prices.high', inHigh);
  requireHistory('EMV openAndFill', 'prices.high', inHigh.length);
  requireArgument('EMV openAndFill', 'prices.low', inLow);
  requireArgument('EMV openAndFill', 'prices.volume', inVolume);
  const guardLookback = emvLookbackPositional(optInTimePeriod, optInVolumeDivisor, core);
  if (guardLookback < 0) {
    throw rejectedParams('EMV openAndFill', PARAM_RANGES, optInTimePeriod, optInVolumeDivisor);
  }
  const guardOutLen = openFillCount('EMV openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('EMV openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('EMV openAndFill', 'prices.volume', inVolume.length, inHigh.length);
  requireOutput('EMV openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inVolumeF64 = float64(inVolume);
  if (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('EMV openAndFill', 'outReal', 'prices.high');
  }
  if (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('EMV openAndFill', 'outReal', 'prices.low');
  }
  if (outReal.buffer === inVolumeF64.buffer && outReal.byteOffset === inVolumeF64.byteOffset) {
    throw sharedMemory('EMV openAndFill', 'outReal', 'prices.volume');
  }
  if (guardOutLen > 0 && overlaps(outReal, inHighF64)) {
    throw sharedMemory('EMV openAndFill', 'outReal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outReal, inLowF64)) {
    throw sharedMemory('EMV openAndFill', 'outReal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outReal, inVolumeF64)) {
    throw sharedMemory('EMV openAndFill', 'outReal', 'prices.volume');
  }
  return emvOpenAndFillInternal(core, inHighF64, inLowF64, inVolumeF64, 0, optInTimePeriod, optInVolumeDivisor, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live EMV stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link emv} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar. `volume`: Volume
 *   of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link EmvOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `emvLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a real parameter that is not a
 *   number, a parameter outside its range or histories of different lengths.
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
 * const stream = emvOpen({ high, low, volume }, {
 *   timePeriod: 14,
 *   volumeDivisor: 10000,
 * });
 * const peeked = stream.peek(101.4, 99.1, 12345); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 12345); // the bar closes
 * ```
 */
export function emvOpen(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume: ArrayLike<number>; openInterest?: ArrayLike<number> | undefined }, options?: EmvOptions): EmvStream {
  requireArgument('EMV open', 'prices', prices);
  const o = optionsOf('EMV open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInVolumeDivisor = orDefault(o.volumeDivisor, Core.REAL_DEFAULT);
  const core = coreOf('EMV open', o.core);
  return emvOpenPositional(prices.high, prices.low, prices.volume, optInTimePeriod, optInVolumeDivisor, core);
}

/**
 * {@link emvOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link emv}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar. `volume`: Volume
 *   of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link EmvOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link emv} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `emvLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a real parameter that is not a
 *   number, a parameter outside its range, histories of different lengths or a
 *   buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
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
 * const { stream, begIdx, values } = emvOpenAndFill({ high, low, volume }, {
 *   timePeriod: 14,
 *   volumeDivisor: 10000,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function emvOpenAndFill(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume: ArrayLike<number>; openInterest?: ArrayLike<number> | undefined }, options?: EmvOptions): EmvResult & { stream: EmvStream } {
  requireArgument('EMV openAndFill', 'prices', prices);
  const o = optionsOf('EMV openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInVolumeDivisor = orDefault(o.volumeDivisor, Core.REAL_DEFAULT);
  const core = coreOf('EMV openAndFill', o.core);
  const capacity = fillCapacity(emvLookbackPositional(optInTimePeriod, optInVolumeDivisor, core), prices.high, prices.low, prices.volume);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = emvOpenAndFillPositional(prices.high, prices.low, prices.volume, optInTimePeriod, optInVolumeDivisor, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
