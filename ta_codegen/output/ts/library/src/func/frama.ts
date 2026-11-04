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
import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link frama} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param optInTimePeriod - Number of bars in the window, split into two equal
 *   halves (default 16; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function framaLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
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

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link frama}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Number of bars in the window, split into two equal
 *   halves (default 16; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function framaDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (framaLookback(optInTimePeriod, core) < 0) {
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
  lookbackTotal = framaLookback(optInTimePeriod, core);
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
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link framaLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param optInTimePeriod - Number of bars in the window, split into two equal
 *   halves (default 16; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param outReal - Adaptive moving average line. Must hold the values produced and
 *   never be empty; the input length always suffices.
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
 * import { frama } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(high.length);
 * const { begIdx, count } = frama(0, high.length - 1, high, low, 16, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`kama`](https://ta-lib.org/functions/kama)
 * @see [`mama`](https://ta-lib.org/functions/mama)
 * @see [`medprice`](https://ta-lib.org/functions/medprice)
 */
export function frama(
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
  optInTimePeriod = requireInt32('FRAMA', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('FRAMA', startIdx, endIdx);
  const guardStart = clampedStart('FRAMA', startIdx, framaLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('FRAMA', 'inHigh', inHigh, guardInLen);
  requireLength('FRAMA', 'inLow', inLow, guardInLen);
  requireOutput('FRAMA', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlapsPartly(outReal, inHighF64) || overlapsPartly(outReal, inLowF64)) {
    throw failure('FRAMA', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = framaImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('FRAMA', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
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
    optInTimePeriod = 16;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
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
  lookbackTotal = framaLookback(optInTimePeriod, core);
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
    throw insufficientHistory('FRAMA openAndFill', inHigh.length, startIdx, framaLookback(optInTimePeriod, core));
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
    throw insufficientHistory('FRAMA open', inHigh.length, startIdx, framaLookback(optInTimePeriod, core));
  }
  throw streamFailure('FRAMA open', retCode);
}

/**
 * Opens a live FRAMA stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link frama} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Number of bars in the window, split into two equal
 *   halves (default 16; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `framaLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { framaOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = framaOpen(high, low, 16);
 * const peeked = stream.peek(101.4, 99.1); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1); // the bar closes
 * ```
 */
export function framaOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): FramaStream {
  optInTimePeriod = requireInt32('FRAMA open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('FRAMA open', 'inHigh', inHigh);
  requireHistory('FRAMA open', inHigh.length);
  requireArgument('FRAMA open', 'inLow', inLow);
  requireHistoryLength('FRAMA open', 'inLow', inLow.length, inHigh.length);
  return framaOpenInternal(core, float64(inHigh), float64(inLow), 0, optInTimePeriod);
}

/**
 * {@link framaOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link frama}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Number of bars in the window, split into two equal
 *   halves (default 16; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param outReal - Adaptive moving average line. Must hold `historyLen - lookback`
 *   values and share no memory with an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `framaLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range, histories of different lengths or an output too
 *   short or starting where another output or a `Float64Array` input starts, or, on
 *   a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { framaOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(high.length);
 * const stream = framaOpenAndFill(high, low, 16, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function framaOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): FramaStream {
  optInTimePeriod = requireInt32('FRAMA openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('FRAMA openAndFill', 'inHigh', inHigh);
  requireHistory('FRAMA openAndFill', inHigh.length);
  requireArgument('FRAMA openAndFill', 'inLow', inLow);
  const guardOutLen = openFillCount('FRAMA openAndFill', inHigh.length, framaLookback(optInTimePeriod, core));
  requireHistoryLength('FRAMA openAndFill', 'inLow', inLow.length, inHigh.length);
  requireOutput('FRAMA openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if ((outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) || (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset)) {
    throw streamFailure('FRAMA openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inHighF64) || overlaps(outReal, inLowF64))) {
    throw streamFailure('FRAMA openAndFill', RetCode.BAD_PARAM);
  }
  return framaOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}
