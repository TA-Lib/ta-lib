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
 *  092826 MF,CC  First version (issue #464).
 *  100226 MF,CC  #497. An odd period is refused before the range is written.
 *  100726 MF,CC  #492. The Auto rule grows with the period and stops at the
 *                slowest alpha's bound.
 */

import { FuncUnstId } from '../enums.ts';
import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function framaLookbackPositional(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 16;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  let root = 0;
  root = (Math.sqrt(optInTimePeriod) | 0);
  /* The range check cannot demand an even period; without this the lookback
   * answers a usable number for a call that cannot run.
   */
  if (((optInTimePeriod % 2) | 0) !== 0) {
    return -1;
  }
  return optInTimePeriod + core.unstableCount(FuncUnstId.FRAMA, ((((((((9 * (4 + 4)) | 0) * (root + 2)) | 0) / 2) | 0) < ((99 * 10) | 0)) ? ((((((9 * (4 + 4)) | 0) * (root + 2)) | 0) / 2) | 0) : ((99 * 10) | 0)), ((((((((9 * (8 + 4)) | 0) * (root + 2)) | 0) / 2) | 0) < ((99 * 19) | 0)) ? ((((((9 * (8 + 4)) | 0) * (root + 2)) | 0) / 2) | 0) : ((99 * 19) | 0)));
}

/** @internal */
export function framaDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (framaLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function framaImpl(
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
  let expScale = 0;
  let slot_sufHigh!: Float64Array;
  let slot_sufLow!: Float64Array;
  let slot_oldHigh!: Float64Array;
  let slot_oldLow!: Float64Array;
  let slot_Idx = 0;
  let maxIdx_slot = (32) - 1;
  let tmpHigh = 0;
  let tmpLow = 0;
  let preHigh = 0;
  let preLow = 0;
  let hi1 = 0;
  let lo1 = 0;
  let hi2 = 0;
  let lo2 = 0;
  let r1 = 0;
  let r2 = 0;
  let r = 0;
  let price = 0;
  let alpha = 0;
  let prevFRAMA = 0;
  let i = 0;
  let today = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let half = 0;
  let lastSlot = 0;
  let seedIdx = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 16;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  expScale = -4.6 / 0.6931471805599453;
  /* -4.6/ln(2): alpha = exp(-4.6*(D-1)) with D-1 = log2((R1+R2)/R). */
  /* Id, Type, Static Size */
  if (((optInTimePeriod % 2) | 0) !== 0) {
    return RetCode.BAD_PARAM;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = framaLookbackPositional(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  half = ((optInTimePeriod / 2) | 0);
  lastSlot = half - 1;
  /* van Herk / Gil-Werman over blocks of `half` bars. Slot j holds the current
   * block's bar j (or, past the running position, the previous block's suffix
   * extremes), and in oldHigh/oldLow the newer half's extremes from `half` bars
   * ago, which are the older half's extremes now: R2[t] = R1[t-half].
   *
   * Every slot store must stay below the output store: that is what keeps the
   * stream's Peek read-only and O(1). The slots hold copies, so outReal may
   * alias an input.
   */
  if (half < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  slot_sufHigh = new Float64Array(half);
  slot_sufLow = new Float64Array(half);
  slot_oldHigh = new Float64Array(half);
  slot_oldLow = new Float64Array(half);
  maxIdx_slot = half - 1;
  slot_Idx = 0;
  today = startIdx - lookbackTotal + 1;
  seedIdx = startIdx - (lookbackTotal - optInTimePeriod) - 1;
  /* The first block's suffix reads must see a bar inside the window. */
  i = 0;
  while (i < half) {
    slot_sufHigh[i] = inHigh[today];
    slot_sufLow[i] = inLow[today];
    slot_oldHigh[i] = inHigh[today];
    slot_oldLow[i] = inLow[today];
    i += 1;
  }
  preHigh = 0.0;
  preLow = 0.0;
  /* Through seedIdx only the blocks are built: the seed discards every value
   * before it, so no alpha is computed there.
   */
  while (today <= seedIdx) {
    tmpHigh = inHigh[today];
    tmpLow = inLow[today];
    if (slot_Idx === 0) {
      preHigh = tmpHigh;
      preLow = tmpLow;
    } else {
      if (tmpHigh > preHigh) {
        preHigh = tmpHigh;
      }
      if (tmpLow < preLow) {
        preLow = tmpLow;
      }
    }
    if (slot_Idx === lastSlot) {
      hi1 = preHigh;
      lo1 = preLow;
    } else {
      hi1 = slot_sufHigh[slot_Idx + 1];
      if (preHigh > hi1) {
        hi1 = preHigh;
      }
      lo1 = slot_sufLow[slot_Idx + 1];
      if (preLow < lo1) {
        lo1 = preLow;
      }
    }
    slot_oldHigh[slot_Idx] = hi1;
    slot_oldLow[slot_Idx] = lo1;
    slot_sufHigh[slot_Idx] = tmpHigh;
    slot_sufLow[slot_Idx] = tmpLow;
    if (slot_Idx === lastSlot) {
      i = lastSlot;
      while (i > 0) {
        i -= 1;
        if (slot_sufHigh[i + 1] > slot_sufHigh[i]) {
          slot_sufHigh[i] = slot_sufHigh[i + 1];
        }
        if (slot_sufLow[i + 1] < slot_sufLow[i]) {
          slot_sufLow[i] = slot_sufLow[i + 1];
        }
      }
    }
    slot_Idx++;
    if (slot_Idx > maxIdx_slot) {
      slot_Idx = 0;
    }
    today += 1;
  }
  prevFRAMA = (inHigh[seedIdx] + inLow[seedIdx]) / 2.0;
  outIdx = 0;
  while (today <= endIdx) {
    tmpHigh = inHigh[today];
    tmpLow = inLow[today];
    price = (tmpHigh + tmpLow) / 2.0;
    if (slot_Idx === 0) {
      preHigh = tmpHigh;
      preLow = tmpLow;
    } else {
      if (tmpHigh > preHigh) {
        preHigh = tmpHigh;
      }
      if (tmpLow < preLow) {
        preLow = tmpLow;
      }
    }
    if (slot_Idx === lastSlot) {
      hi1 = preHigh;
      lo1 = preLow;
    } else {
      hi1 = slot_sufHigh[slot_Idx + 1];
      if (preHigh > hi1) {
        hi1 = preHigh;
      }
      lo1 = slot_sufLow[slot_Idx + 1];
      if (preLow < lo1) {
        lo1 = preLow;
      }
    }
    hi2 = slot_oldHigh[slot_Idx];
    lo2 = slot_oldLow[slot_Idx];
    r1 = hi1 - lo1;
    r2 = hi2 - lo2;
    r = ((hi1 > hi2) ? hi1 : hi2) - ((lo1 < lo2) ? lo1 : lo2);
    /* R >= max(R1,R2) makes R1+R2 <= R exactly the alpha >= 1 clamp, so a
     * clamped or flat bar returns the price with no transcendental. Keep the
     * recursion as alpha*P + (1-alpha)*prev: a computed alpha of 1.0 then
     * returns the price exactly too.
     */
    if (r1 > 0.0 && r2 > 0.0 && r1 + r2 > r) {
      alpha = Math.exp(expScale * Math.log((r1 + r2) / r));
      prevFRAMA = fma(1.0 - alpha, prevFRAMA, alpha * price);
    } else {
      prevFRAMA = price;
    }
    if (today >= startIdx) {
      outReal[outIdx++] = prevFRAMA;
    }
    slot_oldHigh[slot_Idx] = hi1;
    slot_oldLow[slot_Idx] = lo1;
    slot_sufHigh[slot_Idx] = tmpHigh;
    slot_sufLow[slot_Idx] = tmpLow;
    if (slot_Idx === lastSlot) {
      i = lastSlot;
      while (i > 0) {
        i -= 1;
        if (slot_sufHigh[i + 1] > slot_sufHigh[i]) {
          slot_sufHigh[i] = slot_sufHigh[i + 1];
        }
        if (slot_sufLow[i + 1] < slot_sufLow[i]) {
          slot_sufLow[i] = slot_sufLow[i + 1];
        }
      }
    }
    slot_Idx++;
    if (slot_Idx > maxIdx_slot) {
      slot_Idx = 0;
    }
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000']] as const;

/** @internal */
export function framaPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('FRAMA', 'startIdx', startIdx);
  endIdx = requireInt32('FRAMA', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('FRAMA', 'timePeriod', optInTimePeriod);
  requireIndexRange('FRAMA', startIdx, endIdx);
  const guardLookback = framaLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('FRAMA', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('FRAMA', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('FRAMA', 'prices.high', inHigh, guardInLen);
  requireLength('FRAMA', 'prices.low', inLow, guardInLen);
  requireOutput('FRAMA', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlapsPartly(outReal, inHighF64)) {
    throw sharedMemory('FRAMA', 'outReal', 'prices.high');
  }
  if (overlapsPartly(outReal, inLowF64)) {
    throw sharedMemory('FRAMA', 'outReal', 'prices.low');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = framaImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('FRAMA', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link frama}, {@link framaLookback}, {@link framaDisplayShift},
 * {@link framaOpen} and {@link framaOpenAndFill}. Every key is optional: an
 * omitted or `undefined` key takes its default, and a call ignores the keys it has
 * no use for.
 */
export interface FramaOptions {
  /**
   * Number of bars in the window, split into two equal halves (default 16; range 2
   * to 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  timePeriod?: number | undefined;
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
  outReal?: Float64Array | undefined;
}

/** What {@link frama} returns, and {@link framaOpenAndFill} with its stream. */
export interface FramaResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Adaptive moving average line. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link frama} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link FramaOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function framaLookback(options?: FramaOptions): number {
  const o = optionsOf('FRAMA lookback', options);
  return framaLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('FRAMA lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link frama}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link FramaOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function framaDisplayShift(outputIdx: number, options?: FramaOptions): number {
  const o = optionsOf('FRAMA displayShift', options);
  return framaDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('FRAMA displayShift', o.core));
}

/**
 * Fractal Adaptive Moving Average (John Ehlers): an EMA whose smoothing factor
 * adapts each bar to the fractal dimension of the window, estimated from the
 * high-low ranges of the window and of its two halves. A straight run gives
 * dimension 1 and the output follows the price; dense congestion gives dimension 2
 * and very slow smoothing.
 *
 * Formula and more info at
 * [ta-lib.org/functions/frama](https://ta-lib.org/functions/frama).
 *
 * **Notes**
 * - Where either half of the window is flat, alpha is 1 and the output is the
 *   price; Ehlers' listing keeps the previous bar's dimension there instead.
 * - The period must be even; an odd period is rejected.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link framaLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `high`: High
 *   price of each bar. `low`: Low price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link FramaOptions}.
 * @returns `begIdx` and the values; see {@link FramaResult}. Each output is a view
 *   of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, an integer parameter that is not an int32, a parameter outside its range,
 *   an input too short for the range, a buffer that is empty, too short or of the
 *   wrong type or a buffer sharing memory with a `Float64Array` input it is not.
 *   Checked before anything is written.
 *
 * @example
 * ```ts
 * import { frama } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = frama({ high, low }, { timePeriod: 16 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`kama`](https://ta-lib.org/functions/kama)
 * @see [`mama`](https://ta-lib.org/functions/mama)
 * @see [`medprice`](https://ta-lib.org/functions/medprice)
 */
export function frama(prices: { high: ArrayLike<number>; low: ArrayLike<number> }, options?: FramaOptions): FramaResult {
  requireArgument('FRAMA', 'prices', prices);
  const o = optionsOf('FRAMA', options);
  if (o.endIdx === undefined) {
    requireSeries('FRAMA', 'prices.high', prices.high);
    requireSeries('FRAMA', 'prices.low', prices.low);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('FRAMA', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, framaLookbackPositional(optInTimePeriod, core), prices.high, prices.low);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = framaPositional(startIdx, endIdx, prices.high, prices.low, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live FRAMA stream: one value per closed bar, bit-identical to {@link frama}
 * over the same series. Open it with {@link framaOpen} or
 * {@link framaOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/frama](https://ta-lib.org/functions/frama).
 */
export class FramaStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  expScale: number = 0;
  /** @internal */
  preHigh: number = 0;
  /** @internal */
  preLow: number = 0;
  /** @internal */
  prevFRAMA: number = 0;
  /** @internal */
  lastSlot: number = 0;
  /** @internal */
  slot_Idx: number = 0;
  /** @internal */
  maxIdx_slot: number = 0;
  /** @internal */
  cbSize_slot: number = 0;
  /** @internal */
  cb_slot_sufHigh: Float64Array = new Float64Array(1);
  /** @internal */
  cb_slot_sufLow: Float64Array = new Float64Array(1);
  /** @internal */
  cb_slot_oldHigh: Float64Array = new Float64Array(1);
  /** @internal */
  cb_slot_oldLow: Float64Array = new Float64Array(1);
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
  static of(core: Core): FramaStream {
    return new FramaStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link frama} reports them over the same bars. Each accepted `update` and each
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
      throw failure('FRAMA advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw failure('FRAMA update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('FRAMA update', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    framaStepImpl(this, inHigh, inLow);
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
      throw nonFiniteBar('FRAMA peek', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    const sp = this;
    let tmpHigh = 0;
    let tmpLow = 0;
    let hi1 = 0;
    let lo1 = 0;
    let hi2 = 0;
    let lo2 = 0;
    let r1 = 0;
    let r2 = 0;
    let r = 0;
    let price = 0;
    let alpha = 0;
    let cur_outReal = 0;
    let preHigh = sp.preHigh;
    let preLow = sp.preLow;
    let prevFRAMA = sp.prevFRAMA;
    tmpHigh = inHigh;
    tmpLow = inLow;
    price = (tmpHigh + tmpLow) / 2.0;
    if (sp.slot_Idx === 0) {
      preHigh = tmpHigh;
      preLow = tmpLow;
    } else {
      if (tmpHigh > preHigh) {
        preHigh = tmpHigh;
      }
      if (tmpLow < preLow) {
        preLow = tmpLow;
      }
    }
    if (sp.slot_Idx === sp.lastSlot) {
      hi1 = preHigh;
      lo1 = preLow;
    } else {
      hi1 = sp.cb_slot_sufHigh[sp.slot_Idx + 1];
      if (preHigh > hi1) {
        hi1 = preHigh;
      }
      lo1 = sp.cb_slot_sufLow[sp.slot_Idx + 1];
      if (preLow < lo1) {
        lo1 = preLow;
      }
    }
    hi2 = sp.cb_slot_oldHigh[sp.slot_Idx];
    lo2 = sp.cb_slot_oldLow[sp.slot_Idx];
    r1 = hi1 - lo1;
    r2 = hi2 - lo2;
    r = ((hi1 > hi2) ? hi1 : hi2) - ((lo1 < lo2) ? lo1 : lo2);
    /* R >= max(R1,R2) makes R1+R2 <= R exactly the alpha >= 1 clamp, so a
     * clamped or flat bar returns the price with no transcendental. Keep the
     * recursion as alpha*P + (1-alpha)*prev: a computed alpha of 1.0 then
     * returns the price exactly too.
     */
    if (r1 > 0.0 && r2 > 0.0 && r1 + r2 > r) {
      alpha = Math.exp(sp.expScale * Math.log((r1 + r2) / r));
      prevFRAMA = fma(1.0 - alpha, prevFRAMA, alpha * price);
    } else {
      prevFRAMA = price;
    }
    cur_outReal = prevFRAMA;
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
  clone(): FramaStream {
    const c = new FramaStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.expScale = this.expScale;
    c.preHigh = this.preHigh;
    c.preLow = this.preLow;
    c.prevFRAMA = this.prevFRAMA;
    c.lastSlot = this.lastSlot;
    c.slot_Idx = this.slot_Idx;
    c.maxIdx_slot = this.maxIdx_slot;
    c.cbSize_slot = this.cbSize_slot;
    c.cb_slot_sufHigh = this.cb_slot_sufHigh.slice();
    c.cb_slot_sufLow = this.cb_slot_sufLow.slice();
    c.cb_slot_oldHigh = this.cb_slot_oldHigh.slice();
    c.cb_slot_oldLow = this.cb_slot_oldLow.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function framaStepImpl(sp: FramaStream, inHigh: number, inLow: number): void {
  let tmpHigh = 0;
  let tmpLow = 0;
  let hi1 = 0;
  let lo1 = 0;
  let hi2 = 0;
  let lo2 = 0;
  let r1 = 0;
  let r2 = 0;
  let r = 0;
  let price = 0;
  let alpha = 0;
  let i = 0;
  tmpHigh = inHigh;
  tmpLow = inLow;
  price = (tmpHigh + tmpLow) / 2.0;
  if (sp.slot_Idx === 0) {
    sp.preHigh = tmpHigh;
    sp.preLow = tmpLow;
  } else {
    if (tmpHigh > sp.preHigh) {
      sp.preHigh = tmpHigh;
    }
    if (tmpLow < sp.preLow) {
      sp.preLow = tmpLow;
    }
  }
  if (sp.slot_Idx === sp.lastSlot) {
    hi1 = sp.preHigh;
    lo1 = sp.preLow;
  } else {
    hi1 = sp.cb_slot_sufHigh[sp.slot_Idx + 1];
    if (sp.preHigh > hi1) {
      hi1 = sp.preHigh;
    }
    lo1 = sp.cb_slot_sufLow[sp.slot_Idx + 1];
    if (sp.preLow < lo1) {
      lo1 = sp.preLow;
    }
  }
  hi2 = sp.cb_slot_oldHigh[sp.slot_Idx];
  lo2 = sp.cb_slot_oldLow[sp.slot_Idx];
  r1 = hi1 - lo1;
  r2 = hi2 - lo2;
  r = ((hi1 > hi2) ? hi1 : hi2) - ((lo1 < lo2) ? lo1 : lo2);
  /* R >= max(R1,R2) makes R1+R2 <= R exactly the alpha >= 1 clamp, so a
   * clamped or flat bar returns the price with no transcendental. Keep the
   * recursion as alpha*P + (1-alpha)*prev: a computed alpha of 1.0 then
   * returns the price exactly too.
   */
  if (r1 > 0.0 && r2 > 0.0 && r1 + r2 > r) {
    alpha = Math.exp(sp.expScale * Math.log((r1 + r2) / r));
    sp.prevFRAMA = fma(1.0 - alpha, sp.prevFRAMA, alpha * price);
  } else {
    sp.prevFRAMA = price;
  }
  sp.cur_outReal = sp.prevFRAMA;
  sp.cb_slot_oldHigh[sp.slot_Idx] = hi1;
  sp.cb_slot_oldLow[sp.slot_Idx] = lo1;
  sp.cb_slot_sufHigh[sp.slot_Idx] = tmpHigh;
  sp.cb_slot_sufLow[sp.slot_Idx] = tmpLow;
  if (sp.slot_Idx === sp.lastSlot) {
    i = sp.lastSlot;
    while (i > 0) {
      i -= 1;
      if (sp.cb_slot_sufHigh[i + 1] > sp.cb_slot_sufHigh[i]) {
        sp.cb_slot_sufHigh[i] = sp.cb_slot_sufHigh[i + 1];
      }
      if (sp.cb_slot_sufLow[i + 1] < sp.cb_slot_sufLow[i]) {
        sp.cb_slot_sufLow[i] = sp.cb_slot_sufLow[i + 1];
      }
    }
  }
  sp.slot_Idx = sp.slot_Idx + 1;
  if (sp.slot_Idx > sp.maxIdx_slot) {
    sp.slot_Idx = 0;
  }
}

function framaOpenImpl(
  sp: FramaStream,
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
  let expScale = 0;
  let slot_sufHigh!: Float64Array;
  let slot_sufLow!: Float64Array;
  let slot_oldHigh!: Float64Array;
  let slot_oldLow!: Float64Array;
  let slot_Idx = 0;
  let maxIdx_slot = (32) - 1;
  let tmpHigh = 0;
  let tmpLow = 0;
  let preHigh = 0;
  let preLow = 0;
  let hi1 = 0;
  let lo1 = 0;
  let hi2 = 0;
  let lo2 = 0;
  let r1 = 0;
  let r2 = 0;
  let r = 0;
  let price = 0;
  let alpha = 0;
  let prevFRAMA = 0;
  let i = 0;
  let today = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let half = 0;
  let lastSlot = 0;
  let seedIdx = 0;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 16;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  expScale = -4.6 / 0.6931471805599453;
  /* -4.6/ln(2): alpha = exp(-4.6*(D-1)) with D-1 = log2((R1+R2)/R). */
  /* Id, Type, Static Size */
  if (((optInTimePeriod % 2) | 0) !== 0) {
    return RetCode.BAD_PARAM;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = framaLookbackPositional(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  half = ((optInTimePeriod / 2) | 0);
  lastSlot = half - 1;
  /* van Herk / Gil-Werman over blocks of `half` bars. Slot j holds the current
   * block's bar j (or, past the running position, the previous block's suffix
   * extremes), and in oldHigh/oldLow the newer half's extremes from `half` bars
   * ago, which are the older half's extremes now: R2[t] = R1[t-half].
   *
   * Every slot store must stay below the output store: that is what keeps the
   * stream's Peek read-only and O(1). The slots hold copies, so outReal may
   * alias an input.
   */
  if (half < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  slot_sufHigh = new Float64Array(half);
  slot_sufLow = new Float64Array(half);
  slot_oldHigh = new Float64Array(half);
  slot_oldLow = new Float64Array(half);
  maxIdx_slot = half - 1;
  slot_Idx = 0;
  today = startIdx - lookbackTotal + 1;
  seedIdx = startIdx - (lookbackTotal - optInTimePeriod) - 1;
  /* The first block's suffix reads must see a bar inside the window. */
  i = 0;
  while (i < half) {
    slot_sufHigh[i] = inHigh[today];
    slot_sufLow[i] = inLow[today];
    slot_oldHigh[i] = inHigh[today];
    slot_oldLow[i] = inLow[today];
    i += 1;
  }
  preHigh = 0.0;
  preLow = 0.0;
  /* Through seedIdx only the blocks are built: the seed discards every value
   * before it, so no alpha is computed there.
   */
  while (today <= seedIdx) {
    tmpHigh = inHigh[today];
    tmpLow = inLow[today];
    if (slot_Idx === 0) {
      preHigh = tmpHigh;
      preLow = tmpLow;
    } else {
      if (tmpHigh > preHigh) {
        preHigh = tmpHigh;
      }
      if (tmpLow < preLow) {
        preLow = tmpLow;
      }
    }
    if (slot_Idx === lastSlot) {
      hi1 = preHigh;
      lo1 = preLow;
    } else {
      hi1 = slot_sufHigh[slot_Idx + 1];
      if (preHigh > hi1) {
        hi1 = preHigh;
      }
      lo1 = slot_sufLow[slot_Idx + 1];
      if (preLow < lo1) {
        lo1 = preLow;
      }
    }
    slot_oldHigh[slot_Idx] = hi1;
    slot_oldLow[slot_Idx] = lo1;
    slot_sufHigh[slot_Idx] = tmpHigh;
    slot_sufLow[slot_Idx] = tmpLow;
    if (slot_Idx === lastSlot) {
      i = lastSlot;
      while (i > 0) {
        i -= 1;
        if (slot_sufHigh[i + 1] > slot_sufHigh[i]) {
          slot_sufHigh[i] = slot_sufHigh[i + 1];
        }
        if (slot_sufLow[i + 1] < slot_sufLow[i]) {
          slot_sufLow[i] = slot_sufLow[i + 1];
        }
      }
    }
    slot_Idx++;
    if (slot_Idx > maxIdx_slot) {
      slot_Idx = 0;
    }
    today += 1;
  }
  prevFRAMA = (inHigh[seedIdx] + inLow[seedIdx]) / 2.0;
  outIdx = 0;
  while (today <= endIdx) {
    tmpHigh = inHigh[today];
    tmpLow = inLow[today];
    price = (tmpHigh + tmpLow) / 2.0;
    if (slot_Idx === 0) {
      preHigh = tmpHigh;
      preLow = tmpLow;
    } else {
      if (tmpHigh > preHigh) {
        preHigh = tmpHigh;
      }
      if (tmpLow < preLow) {
        preLow = tmpLow;
      }
    }
    if (slot_Idx === lastSlot) {
      hi1 = preHigh;
      lo1 = preLow;
    } else {
      hi1 = slot_sufHigh[slot_Idx + 1];
      if (preHigh > hi1) {
        hi1 = preHigh;
      }
      lo1 = slot_sufLow[slot_Idx + 1];
      if (preLow < lo1) {
        lo1 = preLow;
      }
    }
    hi2 = slot_oldHigh[slot_Idx];
    lo2 = slot_oldLow[slot_Idx];
    r1 = hi1 - lo1;
    r2 = hi2 - lo2;
    r = ((hi1 > hi2) ? hi1 : hi2) - ((lo1 < lo2) ? lo1 : lo2);
    /* R >= max(R1,R2) makes R1+R2 <= R exactly the alpha >= 1 clamp, so a
     * clamped or flat bar returns the price with no transcendental. Keep the
     * recursion as alpha*P + (1-alpha)*prev: a computed alpha of 1.0 then
     * returns the price exactly too.
     */
    if (r1 > 0.0 && r2 > 0.0 && r1 + r2 > r) {
      alpha = Math.exp(expScale * Math.log((r1 + r2) / r));
      prevFRAMA = fma(1.0 - alpha, prevFRAMA, alpha * price);
    } else {
      prevFRAMA = price;
    }
    if (today >= startIdx) {
      outReal[((outIdx++ * outStride) | 0)] = prevFRAMA;
    }
    slot_oldHigh[slot_Idx] = hi1;
    slot_oldLow[slot_Idx] = lo1;
    slot_sufHigh[slot_Idx] = tmpHigh;
    slot_sufLow[slot_Idx] = tmpLow;
    if (slot_Idx === lastSlot) {
      i = lastSlot;
      while (i > 0) {
        i -= 1;
        if (slot_sufHigh[i + 1] > slot_sufHigh[i]) {
          slot_sufHigh[i] = slot_sufHigh[i + 1];
        }
        if (slot_sufLow[i + 1] < slot_sufLow[i]) {
          slot_sufLow[i] = slot_sufLow[i + 1];
        }
      }
    }
    slot_Idx++;
    if (slot_Idx > maxIdx_slot) {
      slot_Idx = 0;
    }
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  const capCb_slot = maxIdx_slot + 1;
  if (capCb_slot > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.expScale = expScale;
  sp.preHigh = preHigh;
  sp.preLow = preLow;
  sp.prevFRAMA = prevFRAMA;
  sp.lastSlot = lastSlot;
  sp.slot_Idx = slot_Idx;
  sp.maxIdx_slot = maxIdx_slot;
  sp.cbSize_slot = capCb_slot;
  sp.cb_slot_sufHigh = slot_sufHigh;
  sp.cb_slot_sufLow = slot_sufLow;
  sp.cb_slot_oldHigh = slot_oldHigh;
  sp.cb_slot_oldLow = slot_oldLow;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function framaOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): FramaStream {
  const sp = FramaStream.of(core);
  const retCode = framaOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('FRAMA openAndFill', inHigh.length, startIdx, framaLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && framaLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('FRAMA openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('FRAMA openAndFill', retCode);
}

/** @internal */
export function framaOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): FramaStream {
  const sp = FramaStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = framaOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('FRAMA open', inHigh.length, startIdx, framaLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && framaLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('FRAMA open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('FRAMA open', retCode);
}

/** @internal */
export function framaOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): FramaStream {
  optInTimePeriod = requireInt32('FRAMA open', 'timePeriod', optInTimePeriod);
  requireArgument('FRAMA open', 'prices.high', inHigh);
  requireHistory('FRAMA open', inHigh.length);
  requireArgument('FRAMA open', 'prices.low', inLow);
  requireHistoryLength('FRAMA open', 'prices.low', inLow.length, inHigh.length);
  return framaOpenInternal(core, float64(inHigh), float64(inLow), 0, optInTimePeriod);
}

/** @internal */
export function framaOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): FramaStream {
  optInTimePeriod = requireInt32('FRAMA openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('FRAMA openAndFill', 'prices.high', inHigh);
  requireHistory('FRAMA openAndFill', inHigh.length);
  requireArgument('FRAMA openAndFill', 'prices.low', inLow);
  const guardLookback = framaLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('FRAMA openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('FRAMA openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('FRAMA openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireOutput('FRAMA openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('FRAMA openAndFill', 'outReal', 'prices.high');
  }
  if (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('FRAMA openAndFill', 'outReal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outReal, inHighF64)) {
    throw sharedMemory('FRAMA openAndFill', 'outReal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outReal, inLowF64)) {
    throw sharedMemory('FRAMA openAndFill', 'outReal', 'prices.low');
  }
  return framaOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live FRAMA stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link frama} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link FramaOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `framaLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range
 *   or histories of different lengths.
 *
 * @example
 * ```ts
 * import { framaOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = framaOpen({ high, low }, { timePeriod: 16 });
 * const peeked = stream.peek(101.4, 99.1); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1); // the bar closes
 * ```
 */
export function framaOpen(prices: { high: ArrayLike<number>; low: ArrayLike<number> }, options?: FramaOptions): FramaStream {
  requireArgument('FRAMA open', 'prices', prices);
  const o = optionsOf('FRAMA open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('FRAMA open', o.core);
  return framaOpenPositional(prices.high, prices.low, optInTimePeriod, core);
}

/**
 * {@link framaOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link frama}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link FramaOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link frama} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `framaLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { framaOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = framaOpenAndFill({ high, low }, {
 *   timePeriod: 16,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function framaOpenAndFill(prices: { high: ArrayLike<number>; low: ArrayLike<number> }, options?: FramaOptions): FramaResult & { stream: FramaStream } {
  requireArgument('FRAMA openAndFill', 'prices', prices);
  const o = optionsOf('FRAMA openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('FRAMA openAndFill', o.core);
  const capacity = fillCapacity(framaLookbackPositional(optInTimePeriod, core), prices.high, prices.low);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = framaOpenAndFillPositional(prices.high, prices.low, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
