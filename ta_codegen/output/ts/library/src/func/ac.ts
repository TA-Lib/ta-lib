/*! TA-LIB Copyright (c) 1999-2026, Mario Fortier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither name of author nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


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

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { aoLookbackPositional } from './ao.ts';
import { smaLookbackPositional } from './sma.ts';

/** @internal */
export function acLookbackPositional(optInFastPeriod: number, optInSlowPeriod: number, optInSignalPeriod: number, core: Core = Core.DEFAULT): number {
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
  return aoLookbackPositional(optInFastPeriod, optInSlowPeriod, core) + smaLookbackPositional(optInSignalPeriod, core);
}

/** @internal */
export function acDisplayShiftPositional(optInFastPeriod: number, optInSlowPeriod: number, optInSignalPeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (acLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core) < 0) {
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
  lookbackTotal = acLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core);
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

const PARAM_RANGES = [['fastPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['slowPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['signalPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000']] as const;

/** @internal */
export function acPositional(
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
  optInFastPeriod = requireInt32('AC', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('AC', 'slowPeriod', optInSlowPeriod);
  optInSignalPeriod = requireInt32('AC', 'signalPeriod', optInSignalPeriod);
  requireIndexRange('AC', startIdx, endIdx);
  const guardLookback = acLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('AC', PARAM_RANGES, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
  }
  const guardStart = clampedStart('AC', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('AC', 'prices.high', inHigh, guardInLen);
  requireLength('AC', 'prices.low', inLow, guardInLen);
  requireOutput('AC', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlapsPartly(outReal, inHighF64)) {
    throw sharedMemory('AC', 'outReal', 'prices.high');
  }
  if (overlapsPartly(outReal, inLowF64)) {
    throw sharedMemory('AC', 'outReal', 'prices.low');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = acImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('AC', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link ac}, {@link acLookback}, {@link acDisplayShift},
 * {@link acOpen} and {@link acOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface AcOptions {
  /**
   * Number of bars in the short moving average of the median price (default 5; range
   * 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  fastPeriod?: number | undefined;
  /**
   * Number of bars in the long moving average of the median price (default 34; range
   * 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  slowPeriod?: number | undefined;
  /**
   * Number of bars in the moving average taken over the oscillator (default 5; range
   * 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  signalPeriod?: number | undefined;
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

/** What {@link ac} returns, and {@link acOpenAndFill} with its stream. */
export interface AcResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Distance of the Awesome Oscillator ([`AO`](https://ta-lib.org/functions/ao))
   * from its own moving average, centred on zero. `values[k]` is the value at input
   * bar `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link ac} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AcOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function acLookback(options?: AcOptions): number {
  const o = optionsOf('AC lookback', options);
  return acLookbackPositional(orDefault(o.fastPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowPeriod, Core.INTEGER_DEFAULT), orDefault(o.signalPeriod, Core.INTEGER_DEFAULT), coreOf('AC lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link ac}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AcOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function acDisplayShift(outputIdx: number, options?: AcOptions): number {
  const o = optionsOf('AC displayShift', options);
  return acDisplayShiftPositional(orDefault(o.fastPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowPeriod, Core.INTEGER_DEFAULT), orDefault(o.signalPeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('AC displayShift', o.core));
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
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link acLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `high`: High price of
 *   each bar. `low`: Low price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link AcOptions}.
 * @returns `begIdx` and the values; see {@link AcResult}. Each output is a view of
 *   its buffer, the caller's or a new one.
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
 * import { ac } from 'ta-lib';
 *
 * const high = new Float64Array(252); // your data, oldest first
 * const low = new Float64Array(252); // your data, oldest first
 *
 * const { begIdx, values } = ac({ high, low }, {
 *   fastPeriod: 5,
 *   slowPeriod: 34,
 *   signalPeriod: 5,
 * });
 * console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
 * ```
 * @see [`ao`](https://ta-lib.org/functions/ao)
 * @see [`macd`](https://ta-lib.org/functions/macd)
 * @see [`medprice`](https://ta-lib.org/functions/medprice)
 * @see [`ppo`](https://ta-lib.org/functions/ppo)
 * @see [`sma`](https://ta-lib.org/functions/sma)
 */
export function ac(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: AcOptions): AcResult {
  requireArgument('AC', 'prices', prices);
  const o = optionsOf('AC', options);
  if (o.endIdx === undefined) {
    requireSeries('AC', 'prices.high', prices.high);
    requireSeries('AC', 'prices.low', prices.low);
  }
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('AC', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, acLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core), prices.high, prices.low);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = acPositional(startIdx, endIdx, prices.high, prices.low, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
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

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): AcStream {
    return new AcStream(core);
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
      throw streamFailure('AC advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(high: number, low: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('AC update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(high) || !Number.isFinite(low)) {
      throw nonFiniteBar('AC update', !Number.isFinite(high) ? 'high' : 'low');
    }
    acStepImpl(this, high, low);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(high: number, low: number): number {
    if (!Number.isFinite(high) || !Number.isFinite(low)) {
      throw nonFiniteBar('AC peek', !Number.isFinite(high) ? 'high' : 'low');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
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
  let ringPosL_trailingFastIdx = 0;
  let ringPosL_trailingSlowIdx = 0;
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
  ringPosL_trailingFastIdx = sp.ringPos_trailingFastIdx;
  sp.ring_trailingFastIdx_derived[ringPosL_trailingFastIdx] = (inHigh + inLow) / 2.0;
  ringPosL_trailingFastIdx = ringPosL_trailingFastIdx + 1;
  ringPosL_trailingFastIdx = ringPosL_trailingFastIdx & (ringPosL_trailingFastIdx - ringCapL_trailingFastIdx >> 31);
  sp.ringPos_trailingFastIdx = ringPosL_trailingFastIdx;
  ringCapL_trailingSlowIdx = sp.ringCap_trailingSlowIdx;
  ringPosL_trailingSlowIdx = sp.ringPos_trailingSlowIdx;
  sp.ring_trailingSlowIdx_derived[ringPosL_trailingSlowIdx] = (inHigh + inLow) / 2.0;
  ringPosL_trailingSlowIdx = ringPosL_trailingSlowIdx + 1;
  ringPosL_trailingSlowIdx = ringPosL_trailingSlowIdx & (ringPosL_trailingSlowIdx - ringCapL_trailingSlowIdx >> 31);
  sp.ringPos_trailingSlowIdx = ringPosL_trailingSlowIdx;
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
  if (startIdx >= historyLen) {
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
  lookbackTotal = acLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core);
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
  const sp = AcStream.of(core);
  const retCode = acOpenImpl(sp, inHigh, inLow, startIdx, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('AC openAndFill', inHigh.length, startIdx, acLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && acLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core) < 0) {
    throw rejectedParams('AC openAndFill', PARAM_RANGES, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
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
  const sp = AcStream.of(core);
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
    throw insufficientHistory('AC open', inHigh.length, startIdx, acLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && acLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core) < 0) {
    throw rejectedParams('AC open', PARAM_RANGES, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
  }
  throw streamFailure('AC open', retCode);
}

/** @internal */
export function acOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  core: Core = Core.DEFAULT,
): AcStream {
  optInFastPeriod = requireInt32('AC open', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('AC open', 'slowPeriod', optInSlowPeriod);
  optInSignalPeriod = requireInt32('AC open', 'signalPeriod', optInSignalPeriod);
  requireArgument('AC open', 'prices.high', inHigh);
  requireHistory('AC open', 'prices.high', inHigh.length);
  requireArgument('AC open', 'prices.low', inLow);
  requireHistoryLength('AC open', 'prices.low', inLow.length, inHigh.length);
  return acOpenInternal(core, float64(inHigh), float64(inLow), 0, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
}

/** @internal */
export function acOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): AcStream {
  optInFastPeriod = requireInt32('AC openAndFill', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('AC openAndFill', 'slowPeriod', optInSlowPeriod);
  optInSignalPeriod = requireInt32('AC openAndFill', 'signalPeriod', optInSignalPeriod);
  requireArgument('AC openAndFill', 'prices.high', inHigh);
  requireHistory('AC openAndFill', 'prices.high', inHigh.length);
  requireArgument('AC openAndFill', 'prices.low', inLow);
  const guardLookback = acLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('AC openAndFill', PARAM_RANGES, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
  }
  const guardOutLen = openFillCount('AC openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('AC openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireOutput('AC openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('AC openAndFill', 'outReal', 'prices.high');
  }
  if (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('AC openAndFill', 'outReal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outReal, inHighF64)) {
    throw sharedMemory('AC openAndFill', 'outReal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outReal, inLowF64)) {
    throw sharedMemory('AC openAndFill', 'outReal', 'prices.low');
  }
  return acOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live AC stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link ac} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AcOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `acLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or
 *   histories of different lengths.
 *
 * @example
 * ```ts
 * import { acOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = acOpen({ high, low }, {
 *   fastPeriod: 5,
 *   slowPeriod: 34,
 *   signalPeriod: 5,
 * });
 * const peeked = stream.peek(101.4, 99.1); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1); // the bar closes
 * ```
 */
export function acOpen(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: AcOptions): AcStream {
  requireArgument('AC open', 'prices', prices);
  const o = optionsOf('AC open', options);
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('AC open', o.core);
  return acOpenPositional(prices.high, prices.low, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core);
}

/**
 * {@link acOpen} that also computes the outputs over the whole history in the same
 * pass, bit-identically to {@link ac}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link AcOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link ac} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `acLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
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
 * const { stream, begIdx, values } = acOpenAndFill({ high, low }, {
 *   fastPeriod: 5,
 *   slowPeriod: 34,
 *   signalPeriod: 5,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function acOpenAndFill(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: AcOptions): AcResult & { stream: AcStream } {
  requireArgument('AC openAndFill', 'prices', prices);
  const o = optionsOf('AC openAndFill', options);
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('AC openAndFill', o.core);
  const capacity = fillCapacity(acLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core), prices.high, prices.low);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = acOpenAndFillPositional(prices.high, prices.low, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
