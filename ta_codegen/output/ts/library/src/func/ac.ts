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
 *  081726 MF,CC  Initial version (#228).
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { aoLookback } from './ao.ts';
import { smaLookback } from './sma.ts';

/**
 * Number of leading input bars {@link ac} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInFastPeriod - Number of bars in the short moving average of the
 *   median price (default 5; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param optInSlowPeriod - Number of bars in the long moving average of the median
 *   price (default 34; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param optInSignalPeriod - Number of bars in the moving average taken over the
 *   oscillator (default 5; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function acLookback(optInFastPeriod: number, optInSlowPeriod: number, optInSignalPeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInFastPeriod | 0) !== optInFastPeriod) {
    return -1;
  }
  if ((optInSlowPeriod | 0) !== optInSlowPeriod) {
    return -1;
  }
  if ((optInSignalPeriod | 0) !== optInSignalPeriod) {
    return -1;
  }
  if (optInFastPeriod === Core.INTEGER_DEFAULT) {
    optInFastPeriod = 5;
  } else if (optInFastPeriod < 2 || optInFastPeriod > 100000) {
    return -1;
  }
  if (optInSlowPeriod === Core.INTEGER_DEFAULT) {
    optInSlowPeriod = 34;
  } else if (optInSlowPeriod < 2 || optInSlowPeriod > 100000) {
    return -1;
  }
  if (optInSignalPeriod === Core.INTEGER_DEFAULT) {
    optInSignalPeriod = 5;
  } else if (optInSignalPeriod < 2 || optInSignalPeriod > 100000) {
    return -1;
  }
  /* The oscillator's own window, plus the simple moving average taken over
   * the oscillator itself. Both terms are exactly the lookback of the
   * function they come from, so neither is restated here.
   */
  return aoLookback(optInFastPeriod, optInSlowPeriod, core) + smaLookback(optInSignalPeriod, core);
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link ac}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInFastPeriod - Number of bars in the short moving average of the
 *   median price (default 5; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param optInSlowPeriod - Number of bars in the long moving average of the median
 *   price (default 34; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param optInSignalPeriod - Number of bars in the moving average taken over the
 *   oscillator (default 5; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function acDisplayShift(optInFastPeriod: number, optInSlowPeriod: number, optInSignalPeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (acLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function acImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let sumFast = 0;
  let sumSlow = 0;
  let sumSignal = 0;
  let medianPrice = 0;
  let osc = 0;
  let tempReal = 0;
  let i = 0;
  let outIdx = 0;
  let trailingFastIdx = 0;
  let trailingSlowIdx = 0;
  let oscStartIdx = 0;
  let lookbackTotal = 0;
  let oscBuffer!: Float64Array;
  let oscBuffer_Idx = 0;
  let maxIdx_oscBuffer = (32) - 1;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInFastPeriod === Core.INTEGER_DEFAULT) {
    optInFastPeriod = 5;
  } else if (optInFastPeriod < 2 || optInFastPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowPeriod === Core.INTEGER_DEFAULT) {
    optInSlowPeriod = 34;
  } else if (optInSlowPeriod < 2 || optInSlowPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSignalPeriod === Core.INTEGER_DEFAULT) {
    optInSignalPeriod = 5;
  } else if (optInSignalPeriod < 2 || optInSignalPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  /* Bill Williams' Accelerator/Decelerator Oscillator (New Trading
   * Dimensions, 1998): how fast the Awesome Oscillator is itself
   * accelerating, drawn as a zero-centred histogram.
   *
   *    AO_t = SMA(median, fast)_t - SMA(median, slow)_t
   *    AC_t = AO_t - SMA(AO, signal)_t
   *
   * Every leg is a plain SMA, so there is no seeding convention to get wrong
   * and no cross-library divergence of the kind an EMA brings.
   *
   * This is ta_codegen/input/ao/ao.c walking one derived series with a third
   * running sum layered on top: the oscillator is never materialised into an
   * array, it goes straight into a ring of the last `signal` values. The
   * order is the one ta_codegen/input/sma/sma.c uses -- add the new value,
   * snapshot the total, subtract the bar leaving the window, divide by the
   * period -- for all three sums, which is what makes the composed reference
   * in test_composite.c (TA_AO, TA_SMA and TA_SUB) bit-exact rather than
   * merely close. Any future drift in either path is then a hard failure
   * instead of a tolerance argument.
   *
   * Each total is DIVIDED by its period; it is not multiplied by a
   * precomputed 1/period, for the reason spelled out in ao.c and #117/#118.
   */
  /* This ptr will point on a circular buffer of at least
   * "optInSignalPeriod" element.
   */
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = acLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core);
  /* Move up the start index if there is not
   * enough initial data.
   */
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  /* Allocate a circular buffer equal to the requested signal period. */
  if (optInSignalPeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  oscBuffer = new Float64Array(optInSignalPeriod);
  maxIdx_oscBuffer = optInSignalPeriod - 1;
  oscBuffer_Idx = 0;
  /* The first bar the oscillator is evaluated on for this call. It trails
   * the first output by the signal window, because that many oscillator
   * values have to exist before their average does.
   */
  oscStartIdx = startIdx - (optInSignalPeriod - 1);
  sumFast = 0.0;
  sumSlow = 0.0;
  sumSignal = 0.0;
  trailingFastIdx = oscStartIdx - (optInFastPeriod - 1);
  trailingSlowIdx = oscStartIdx - (optInSlowPeriod - 1);
  /* Add-up both initial periods, except for the last value.
   *
   * One pass over the longer warm-up window replaces two overlapping passes.
   * A bar inside the shorter window is added to that total as it is reached,
   * so each total still accumulates exactly the same bars in exactly the same
   * ascending order two separate loops would have given it -- which is what
   * keeps each leg bit-identical to the TA_SMA that TA_AO would have called
   * had it been started at oscStartIdx.
   */
  i = startIdx - lookbackTotal;
  while (i < oscStartIdx) {
    medianPrice = (inHigh[i] + inLow[i]) / 2.0;
    if (i >= trailingFastIdx) {
      sumFast += medianPrice;
    }
    if (i >= trailingSlowIdx) {
      sumSlow += medianPrice;
    }
    i = i + 1;
  }
  /* Fill the signal ring with the oscillator values that precede the first
   * output. This is the same body as the main loop below minus the store --
   * the window is not full yet, so there is nothing to emit.
   */
  while (i < startIdx) {
    medianPrice = (inHigh[i] + inLow[i]) / 2.0;
    sumFast += medianPrice;
    sumSlow += medianPrice;
    osc = sumFast / optInFastPeriod - sumSlow / optInSlowPeriod;
    sumFast -= (inHigh[trailingFastIdx] + inLow[trailingFastIdx]) / 2.0;
    sumSlow -= (inHigh[trailingSlowIdx] + inLow[trailingSlowIdx]) / 2.0;
    trailingFastIdx = trailingFastIdx + 1;
    trailingSlowIdx = trailingSlowIdx + 1;
    i = i + 1;
    oscBuffer[oscBuffer_Idx] = osc;
    sumSignal += osc;
    oscBuffer_Idx++;
    if (oscBuffer_Idx > maxIdx_oscBuffer) {
      oscBuffer_Idx = 0;
    }
  }
  /* Proceed with the calculation for the requested range.
   * Note that this algorithm allows outReal to be the same
   * buffer as either input.
   */
  outIdx = 0;
  while (i <= endIdx) {
    medianPrice = (inHigh[i] + inLow[i]) / 2.0;
    sumFast += medianPrice;
    sumSlow += medianPrice;
    /* Snapshot the oscillator before either total drops its trailing bar,
     * mirroring the add-new / snapshot / subtract-old order of TA_SMA.
     */
    osc = sumFast / optInFastPeriod - sumSlow / optInSlowPeriod;
    sumFast -= (inHigh[trailingFastIdx] + inLow[trailingFastIdx]) / 2.0;
    sumSlow -= (inHigh[trailingSlowIdx] + inLow[trailingSlowIdx]) / 2.0;
    trailingFastIdx = trailingFastIdx + 1;
    trailingSlowIdx = trailingSlowIdx + 1;
    i = i + 1;
    /* Today's oscillator enters the signal window at its own slot, and the
     * bar leaving that window is read only after the ring has advanced onto
     * it -- writing first is what makes the slot the loop is about to
     * overwrite the newest value rather than the oldest one.
     */
    oscBuffer[oscBuffer_Idx] = osc;
    sumSignal += osc;
    tempReal = osc - sumSignal / optInSignalPeriod;
    oscBuffer_Idx++;
    if (oscBuffer_Idx > maxIdx_oscBuffer) {
      oscBuffer_Idx = 0;
    }
    sumSignal -= oscBuffer[oscBuffer_Idx];
    /* Every input read for this bar is done above, so the store is safe
     * when the caller aliases outReal over inHigh or inLow. Unlike ao.c
     * there is slack here -- the signal window puts both trailing indices
     * at least optInSignalPeriod-1 bars ahead of outIdx, so no reachable
     * parameter makes them collide -- but the order is kept anyway, so
     * that admitting a signal period of 1 would not silently reintroduce
     * the collision ao.c has to guard against.
     */
    outReal[outIdx] = tempReal;
    outIdx = outIdx + 1;
  }
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/**
 * Bill Williams' Accelerator/Decelerator Oscillator (*New Trading Dimensions*,
 * 1998): the rate at which market momentum is itself speeding up or slowing down.
 * Where the Awesome Oscillator ([`AO`](https://ta-lib.org/functions/ao)) measures
 * momentum, this measures the change in that momentum, by taking the oscillator's
 * distance above or below its own moving average. Because acceleration turns
 * before speed does, the reading changes sign ahead of the oscillator it is built
 * from — it is meant as the early half of a pair, not as a signal on its own.
 * Above zero acceleration is with the bulls, below zero with the bears, and it is
 * drawn as a zero-centred histogram whose colour convention is the bar-to-bar
 * change: rising bars accelerating, falling bars decelerating. Williams' rule of
 * thumb is that two same-coloured bars are what confirms the turn, which is why
 * the sign and the direction matter more than the level. The oscillator is one leg
 * of Williams' Profitunity system, alongside the Awesome Oscillator
 * ([`AO`](https://ta-lib.org/functions/ao)) and the Alligator.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ac](https://ta-lib.org/functions/ac).
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link acLookback} is a success with `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param optInFastPeriod - Number of bars in the short moving average of the
 *   median price (default 5; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param optInSlowPeriod - Number of bars in the long moving average of the median
 *   price (default 34; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param optInSignalPeriod - Number of bars in the moving average taken over the
 *   oscillator (default 5; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param outReal - Distance of the Awesome Oscillator
 *   ([`AO`](https://ta-lib.org/functions/ao)) from its own moving average, centred
 *   on zero. Must hold the values produced and never be empty; the input length
 *   always suffices.
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
 * import { ac } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(high.length);
 * const { begIdx, count } = ac(0, high.length - 1, high, low, 5, 34, 5, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`ao`](https://ta-lib.org/functions/ao)
 * @see [`macd`](https://ta-lib.org/functions/macd)
 * @see [`medprice`](https://ta-lib.org/functions/medprice)
 * @see [`ppo`](https://ta-lib.org/functions/ppo)
 * @see [`sma`](https://ta-lib.org/functions/sma)
 */
export function ac(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('AC', 'startIdx', startIdx);
  endIdx = requireInt32('AC', 'endIdx', endIdx);
  optInFastPeriod = requireInt32('AC', 'optInFastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('AC', 'optInSlowPeriod', optInSlowPeriod);
  optInSignalPeriod = requireInt32('AC', 'optInSignalPeriod', optInSignalPeriod);
  requireIndexRange('AC', startIdx, endIdx);
  const guardStart = clampedStart('AC', startIdx, acLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('AC', 'inHigh', inHigh, guardInLen);
  requireLength('AC', 'inLow', inLow, guardInLen);
  requireLength('AC', 'outReal', outReal, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlapsPartly(outReal, inHighF64) || overlapsPartly(outReal, inLowF64)) {
    throw failure('AC', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = acImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('AC', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live AC stream: one value per closed bar, bit-identical to {@link ac} over the
 * same series. Open it with {@link acOpen} or {@link acOpenAndFill}; there is
 * nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ac](https://ta-lib.org/functions/ac).
 */
export class AcStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInFastPeriod: number = 0;
  /** @internal */
  optInSlowPeriod: number = 0;
  /** @internal */
  optInSignalPeriod: number = 0;
  /** @internal */
  sumFast: number = 0;
  /** @internal */
  sumSlow: number = 0;
  /** @internal */
  sumSignal: number = 0;
  /** @internal */
  oscBuffer_Idx: number = 0;
  /** @internal */
  maxIdx_oscBuffer: number = 0;
  /** @internal */
  ringPos_trailingFastIdx: number = 0;
  /** @internal */
  ringCap_trailingFastIdx: number = 0;
  /** @internal */
  ring_trailingFastIdx_derived: Float64Array = new Float64Array(1);
  /** @internal */
  ringPos_trailingSlowIdx: number = 0;
  /** @internal */
  ringCap_trailingSlowIdx: number = 0;
  /** @internal */
  ring_trailingSlowIdx_derived: Float64Array = new Float64Array(1);
  /** @internal */
  cbSize_oscBuffer: number = 0;
  /** @internal */
  cb_oscBuffer: Float64Array = new Float64Array(1);
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
   * {@link ac} reports them over the same bars. Each accepted `update` and each
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
      throw failure('AC advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw failure('AC update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('AC update', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    acStepImpl(this, inHigh, inLow);
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
      throw nonFiniteBar('AC peek', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    const sp = this;
    let medianPrice = 0;
    let osc = 0;
    let tempReal = 0;
    let cur_outReal = 0;
    let oscBuffer_Idx = sp.oscBuffer_Idx;
    let sumFast = sp.sumFast;
    let sumSignal = sp.sumSignal;
    let sumSlow = sp.sumSlow;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    medianPrice = (inHigh + inLow) / 2.0;
    sumFast += medianPrice;
    sumSlow += medianPrice;
    /* Snapshot the oscillator before either total drops its trailing bar,
     * mirroring the add-new / snapshot / subtract-old order of TA_SMA.
     */
    osc = sumFast / sp.optInFastPeriod - sumSlow / sp.optInSlowPeriod;
    sumFast -= sp.ring_trailingFastIdx_derived[sp.ringPos_trailingFastIdx];
    sumSlow -= sp.ring_trailingSlowIdx_derived[sp.ringPos_trailingSlowIdx];
    /* Today's oscillator enters the signal window at its own slot, and the
     * bar leaving that window is read only after the ring has advanced onto
     * it -- writing first is what makes the slot the loop is about to
     * overwrite the newest value rather than the oldest one.
     */
    pkSlot0 = oscBuffer_Idx;
    pkVal0 = osc;
    sumSignal += osc;
    tempReal = osc - sumSignal / sp.optInSignalPeriod;
    oscBuffer_Idx = oscBuffer_Idx + 1;
    if (oscBuffer_Idx > sp.maxIdx_oscBuffer) {
      oscBuffer_Idx = 0;
    }
    sumSignal -= (oscBuffer_Idx !== pkSlot0) ? sp.cb_oscBuffer[oscBuffer_Idx] : pkVal0;
    /* Every input read for this bar is done above, so the store is safe
     * when the caller aliases outReal over inHigh or inLow. Unlike ao.c
     * there is slack here -- the signal window puts both trailing indices
     * at least optInSignalPeriod-1 bars ahead of outIdx, so no reachable
     * parameter makes them collide -- but the order is kept anyway, so
     * that admitting a signal period of 1 would not silently reintroduce
     * the collision ao.c has to guard against.
     */
    cur_outReal = tempReal;
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
  clone(): AcStream {
    const c = new AcStream(this.core);
    c.optInFastPeriod = this.optInFastPeriod;
    c.optInSlowPeriod = this.optInSlowPeriod;
    c.optInSignalPeriod = this.optInSignalPeriod;
    c.sumFast = this.sumFast;
    c.sumSlow = this.sumSlow;
    c.sumSignal = this.sumSignal;
    c.oscBuffer_Idx = this.oscBuffer_Idx;
    c.maxIdx_oscBuffer = this.maxIdx_oscBuffer;
    c.ringPos_trailingFastIdx = this.ringPos_trailingFastIdx;
    c.ringCap_trailingFastIdx = this.ringCap_trailingFastIdx;
    c.ring_trailingFastIdx_derived = this.ring_trailingFastIdx_derived.slice();
    c.ringPos_trailingSlowIdx = this.ringPos_trailingSlowIdx;
    c.ringCap_trailingSlowIdx = this.ringCap_trailingSlowIdx;
    c.ring_trailingSlowIdx_derived = this.ring_trailingSlowIdx_derived.slice();
    c.cbSize_oscBuffer = this.cbSize_oscBuffer;
    c.cb_oscBuffer = this.cb_oscBuffer.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function acStepImpl(sp: AcStream, inHigh: number, inLow: number): void {
  let medianPrice = 0;
  let osc = 0;
  let tempReal = 0;
  let ringCapL_trailingFastIdx = 0;
  let ringCapL_trailingSlowIdx = 0;
  medianPrice = (inHigh + inLow) / 2.0;
  sp.sumFast += medianPrice;
  sp.sumSlow += medianPrice;
  /* Snapshot the oscillator before either total drops its trailing bar,
   * mirroring the add-new / snapshot / subtract-old order of TA_SMA.
   */
  osc = sp.sumFast / sp.optInFastPeriod - sp.sumSlow / sp.optInSlowPeriod;
  sp.sumFast -= sp.ring_trailingFastIdx_derived[sp.ringPos_trailingFastIdx];
  sp.sumSlow -= sp.ring_trailingSlowIdx_derived[sp.ringPos_trailingSlowIdx];
  /* Today's oscillator enters the signal window at its own slot, and the
   * bar leaving that window is read only after the ring has advanced onto
   * it -- writing first is what makes the slot the loop is about to
   * overwrite the newest value rather than the oldest one.
   */
  sp.cb_oscBuffer[sp.oscBuffer_Idx] = osc;
  sp.sumSignal += osc;
  tempReal = osc - sp.sumSignal / sp.optInSignalPeriod;
  sp.oscBuffer_Idx = sp.oscBuffer_Idx + 1;
  if (sp.oscBuffer_Idx > sp.maxIdx_oscBuffer) {
    sp.oscBuffer_Idx = 0;
  }
  sp.sumSignal -= sp.cb_oscBuffer[sp.oscBuffer_Idx];
  /* Every input read for this bar is done above, so the store is safe
   * when the caller aliases outReal over inHigh or inLow. Unlike ao.c
   * there is slack here -- the signal window puts both trailing indices
   * at least optInSignalPeriod-1 bars ahead of outIdx, so no reachable
   * parameter makes them collide -- but the order is kept anyway, so
   * that admitting a signal period of 1 would not silently reintroduce
   * the collision ao.c has to guard against.
   */
  sp.cur_outReal = tempReal;
  ringCapL_trailingFastIdx = sp.ringCap_trailingFastIdx;
  sp.ring_trailingFastIdx_derived[sp.ringPos_trailingFastIdx] = (inHigh + inLow) / 2.0;
  sp.ringPos_trailingFastIdx = sp.ringPos_trailingFastIdx + 1;
  if (sp.ringPos_trailingFastIdx >= ringCapL_trailingFastIdx) {
    sp.ringPos_trailingFastIdx = 0;
  }
  ringCapL_trailingSlowIdx = sp.ringCap_trailingSlowIdx;
  sp.ring_trailingSlowIdx_derived[sp.ringPos_trailingSlowIdx] = (inHigh + inLow) / 2.0;
  sp.ringPos_trailingSlowIdx = sp.ringPos_trailingSlowIdx + 1;
  if (sp.ringPos_trailingSlowIdx >= ringCapL_trailingSlowIdx) {
    sp.ringPos_trailingSlowIdx = 0;
  }
}

function acOpenImpl(
  sp: AcStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let sumFast = 0;
  let sumSlow = 0;
  let sumSignal = 0;
  let medianPrice = 0;
  let osc = 0;
  let tempReal = 0;
  let i = 0;
  let outIdx = 0;
  let trailingFastIdx = 0;
  let trailingSlowIdx = 0;
  let oscStartIdx = 0;
  let lookbackTotal = 0;
  let oscBuffer!: Float64Array;
  let oscBuffer_Idx = 0;
  let maxIdx_oscBuffer = (32) - 1;
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
  if (optInFastPeriod === Core.INTEGER_DEFAULT) {
    optInFastPeriod = 5;
  } else if (optInFastPeriod < 2 || optInFastPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowPeriod === Core.INTEGER_DEFAULT) {
    optInSlowPeriod = 34;
  } else if (optInSlowPeriod < 2 || optInSlowPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSignalPeriod === Core.INTEGER_DEFAULT) {
    optInSignalPeriod = 5;
  } else if (optInSignalPeriod < 2 || optInSignalPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Bill Williams' Accelerator/Decelerator Oscillator (New Trading
   * Dimensions, 1998): how fast the Awesome Oscillator is itself
   * accelerating, drawn as a zero-centred histogram.
   *
   *    AO_t = SMA(median, fast)_t - SMA(median, slow)_t
   *    AC_t = AO_t - SMA(AO, signal)_t
   *
   * Every leg is a plain SMA, so there is no seeding convention to get wrong
   * and no cross-library divergence of the kind an EMA brings.
   *
   * This is ta_codegen/input/ao/ao.c walking one derived series with a third
   * running sum layered on top: the oscillator is never materialised into an
   * array, it goes straight into a ring of the last `signal` values. The
   * order is the one ta_codegen/input/sma/sma.c uses -- add the new value,
   * snapshot the total, subtract the bar leaving the window, divide by the
   * period -- for all three sums, which is what makes the composed reference
   * in test_composite.c (TA_AO, TA_SMA and TA_SUB) bit-exact rather than
   * merely close. Any future drift in either path is then a hard failure
   * instead of a tolerance argument.
   *
   * Each total is DIVIDED by its period; it is not multiplied by a
   * precomputed 1/period, for the reason spelled out in ao.c and #117/#118.
   */
  /* This ptr will point on a circular buffer of at least
   * "optInSignalPeriod" element.
   */
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = acLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core);
  /* Move up the start index if there is not
   * enough initial data.
   */
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Allocate a circular buffer equal to the requested signal period. */
  if (optInSignalPeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  oscBuffer = new Float64Array(optInSignalPeriod);
  maxIdx_oscBuffer = optInSignalPeriod - 1;
  oscBuffer_Idx = 0;
  /* The first bar the oscillator is evaluated on for this call. It trails
   * the first output by the signal window, because that many oscillator
   * values have to exist before their average does.
   */
  oscStartIdx = startIdx - (optInSignalPeriod - 1);
  sumFast = 0.0;
  sumSlow = 0.0;
  sumSignal = 0.0;
  trailingFastIdx = oscStartIdx - (optInFastPeriod - 1);
  trailingSlowIdx = oscStartIdx - (optInSlowPeriod - 1);
  /* Add-up both initial periods, except for the last value.
   *
   * One pass over the longer warm-up window replaces two overlapping passes.
   * A bar inside the shorter window is added to that total as it is reached,
   * so each total still accumulates exactly the same bars in exactly the same
   * ascending order two separate loops would have given it -- which is what
   * keeps each leg bit-identical to the TA_SMA that TA_AO would have called
   * had it been started at oscStartIdx.
   */
  i = startIdx - lookbackTotal;
  while (i < oscStartIdx) {
    medianPrice = (inHigh[i] + inLow[i]) / 2.0;
    if (i >= trailingFastIdx) {
      sumFast += medianPrice;
    }
    if (i >= trailingSlowIdx) {
      sumSlow += medianPrice;
    }
    i = i + 1;
  }
  /* Fill the signal ring with the oscillator values that precede the first
   * output. This is the same body as the main loop below minus the store --
   * the window is not full yet, so there is nothing to emit.
   */
  while (i < startIdx) {
    medianPrice = (inHigh[i] + inLow[i]) / 2.0;
    sumFast += medianPrice;
    sumSlow += medianPrice;
    osc = sumFast / optInFastPeriod - sumSlow / optInSlowPeriod;
    sumFast -= (inHigh[trailingFastIdx] + inLow[trailingFastIdx]) / 2.0;
    sumSlow -= (inHigh[trailingSlowIdx] + inLow[trailingSlowIdx]) / 2.0;
    trailingFastIdx = trailingFastIdx + 1;
    trailingSlowIdx = trailingSlowIdx + 1;
    i = i + 1;
    oscBuffer[oscBuffer_Idx] = osc;
    sumSignal += osc;
    oscBuffer_Idx++;
    if (oscBuffer_Idx > maxIdx_oscBuffer) {
      oscBuffer_Idx = 0;
    }
  }
  /* Proceed with the calculation for the requested range.
   * Note that this algorithm allows outReal to be the same
   * buffer as either input.
   */
  outIdx = 0;
  while (i <= endIdx) {
    medianPrice = (inHigh[i] + inLow[i]) / 2.0;
    sumFast += medianPrice;
    sumSlow += medianPrice;
    /* Snapshot the oscillator before either total drops its trailing bar,
     * mirroring the add-new / snapshot / subtract-old order of TA_SMA.
     */
    osc = sumFast / optInFastPeriod - sumSlow / optInSlowPeriod;
    sumFast -= (inHigh[trailingFastIdx] + inLow[trailingFastIdx]) / 2.0;
    sumSlow -= (inHigh[trailingSlowIdx] + inLow[trailingSlowIdx]) / 2.0;
    trailingFastIdx = trailingFastIdx + 1;
    trailingSlowIdx = trailingSlowIdx + 1;
    i = i + 1;
    /* Today's oscillator enters the signal window at its own slot, and the
     * bar leaving that window is read only after the ring has advanced onto
     * it -- writing first is what makes the slot the loop is about to
     * overwrite the newest value rather than the oldest one.
     */
    oscBuffer[oscBuffer_Idx] = osc;
    sumSignal += osc;
    tempReal = osc - sumSignal / optInSignalPeriod;
    oscBuffer_Idx++;
    if (oscBuffer_Idx > maxIdx_oscBuffer) {
      oscBuffer_Idx = 0;
    }
    sumSignal -= oscBuffer[oscBuffer_Idx];
    /* Every input read for this bar is done above, so the store is safe
     * when the caller aliases outReal over inHigh or inLow. Unlike ao.c
     * there is slack here -- the signal window puts both trailing indices
     * at least optInSignalPeriod-1 bars ahead of outIdx, so no reachable
     * parameter makes them collide -- but the order is kept anyway, so
     * that admitting a signal period of 1 would not silently reintroduce
     * the collision ao.c has to guard against.
     */
    outReal[((outIdx * outStride) | 0)] = tempReal;
    outIdx = outIdx + 1;
  }
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const cap_trailingFastIdx = i - trailingFastIdx;
  if (cap_trailingFastIdx < 1 || cap_trailingFastIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_trailingFastIdx = cap_trailingFastIdx > 0 ? cap_trailingFastIdx : 1;
  const capRing_trailingFastIdx_derived = new Float64Array(allocN_trailingFastIdx);
  for (let fillJ = historyLen - cap_trailingFastIdx; fillJ < historyLen; fillJ++) {
    capRing_trailingFastIdx_derived[fillJ - (historyLen - cap_trailingFastIdx)] = (inHigh[fillJ] + inLow[fillJ]) / 2.0;
  }
  const cap_trailingSlowIdx = i - trailingSlowIdx;
  if (cap_trailingSlowIdx < 1 || cap_trailingSlowIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_trailingSlowIdx = cap_trailingSlowIdx > 0 ? cap_trailingSlowIdx : 1;
  const capRing_trailingSlowIdx_derived = new Float64Array(allocN_trailingSlowIdx);
  for (let fillJ = historyLen - cap_trailingSlowIdx; fillJ < historyLen; fillJ++) {
    capRing_trailingSlowIdx_derived[fillJ - (historyLen - cap_trailingSlowIdx)] = (inHigh[fillJ] + inLow[fillJ]) / 2.0;
  }
  const capCb_oscBuffer = maxIdx_oscBuffer + 1;
  if (capCb_oscBuffer > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sp.optInFastPeriod = optInFastPeriod;
  sp.optInSlowPeriod = optInSlowPeriod;
  sp.optInSignalPeriod = optInSignalPeriod;
  sp.sumFast = sumFast;
  sp.sumSlow = sumSlow;
  sp.sumSignal = sumSignal;
  sp.oscBuffer_Idx = oscBuffer_Idx;
  sp.maxIdx_oscBuffer = maxIdx_oscBuffer;
  sp.ringPos_trailingFastIdx = 0;
  sp.ringCap_trailingFastIdx = cap_trailingFastIdx;
  sp.ring_trailingFastIdx_derived = capRing_trailingFastIdx_derived;
  sp.ringPos_trailingSlowIdx = 0;
  sp.ringCap_trailingSlowIdx = cap_trailingSlowIdx;
  sp.ring_trailingSlowIdx_derived = capRing_trailingSlowIdx_derived;
  sp.cbSize_oscBuffer = capCb_oscBuffer;
  sp.cb_oscBuffer = oscBuffer;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function acOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): AcStream {
  const sp = new AcStream(core);
  const retCode = acOpenImpl(sp, inHigh, inLow, startIdx, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('AC openAndFill', inHigh.length, startIdx, acLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core));
  }
  throw streamFailure('AC openAndFill', retCode);
}

/** @internal */
export function acOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
): AcStream {
  const sp = new AcStream(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = acOpenImpl(sp, inHigh, inLow, startIdx, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('AC open', inHigh.length, startIdx, acLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core));
  }
  throw streamFailure('AC open', retCode);
}

/**
 * Opens a live AC stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link ac} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param optInFastPeriod - Number of bars in the short moving average of the
 *   median price (default 5; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param optInSlowPeriod - Number of bars in the long moving average of the median
 *   price (default 34; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param optInSignalPeriod - Number of bars in the moving average taken over the
 *   oscillator (default 5; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `acLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty one,
 *   and `TA_BAD_PARAM` for an integer parameter that is not an int32, a parameter
 *   outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { acOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = acOpen(high, low, 5, 34, 5);
 * const peeked = stream.peek(101.4, 99.1); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1); // the bar closes
 * ```
 */
export function acOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  core: Core = Core.DEFAULT,
): AcStream {
  optInFastPeriod = requireInt32('AC open', 'optInFastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('AC open', 'optInSlowPeriod', optInSlowPeriod);
  optInSignalPeriod = requireInt32('AC open', 'optInSignalPeriod', optInSignalPeriod);
  requireArgument('AC open', 'inHigh', inHigh);
  requireHistory('AC open', inHigh.length);
  requireArgument('AC open', 'inLow', inLow);
  requireHistoryLength('AC open', 'inLow', inLow.length, inHigh.length);
  return acOpenInternal(core, float64(inHigh), float64(inLow), 0, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
}

/**
 * {@link acOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link ac}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param optInFastPeriod - Number of bars in the short moving average of the
 *   median price (default 5; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param optInSlowPeriod - Number of bars in the long moving average of the median
 *   price (default 34; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param optInSignalPeriod - Number of bars in the moving average taken over the
 *   oscillator (default 5; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param outReal - Distance of the Awesome Oscillator
 *   ([`AO`](https://ta-lib.org/functions/ao)) from its own moving average, centred
 *   on zero. Must hold `historyLen - lookback` values and share no memory with an
 *   input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `acLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty one,
 *   and `TA_BAD_PARAM` for an integer parameter that is not an int32, a parameter
 *   outside its range, histories of different lengths or an output too short or
 *   starting where another output or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another output or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { acOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(high.length);
 * const stream = acOpenAndFill(high, low, 5, 34, 5, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function acOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): AcStream {
  optInFastPeriod = requireInt32('AC openAndFill', 'optInFastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('AC openAndFill', 'optInSlowPeriod', optInSlowPeriod);
  optInSignalPeriod = requireInt32('AC openAndFill', 'optInSignalPeriod', optInSignalPeriod);
  requireArgument('AC openAndFill', 'inHigh', inHigh);
  requireHistory('AC openAndFill', inHigh.length);
  requireArgument('AC openAndFill', 'inLow', inLow);
  const guardOutLen = openFillCount('AC openAndFill', inHigh.length, acLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core));
  requireHistoryLength('AC openAndFill', 'inLow', inLow.length, inHigh.length);
  requireLength('AC openAndFill', 'outReal', outReal, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if ((outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) || (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset)) {
    throw streamFailure('AC openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inHighF64) || overlaps(outReal, inLowF64))) {
    throw streamFailure('AC openAndFill', RetCode.BAD_PARAM);
  }
  return acOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, new MInteger(), new MInteger(), outReal);
}
