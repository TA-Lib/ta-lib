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
 *  AM       Adrian Michel <michel@pacbell.net>
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120802 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  050703 MF   Fix algorithm base on Adrian Michel bug report #748163
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function aroonoscLookbackPositional(optInTimePeriod: number, _core: Core = Core.DEFAULT): number {
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

/** @internal */
export function aroonoscDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (aroonoscLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function aroonoscImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let lowest = 0;
  let highest = 0;
  let tmp = 0;
  let factor = 0;
  let aroon = 0;
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
  /* This code is almost identical to the TA_AROON function
   * except that instead of outputing ArroonUp and AroonDown
   * individually, an oscillator is build from both.
   *
   *  AroonOsc = AroonUp- AroonDown;
   */
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
    /* The oscillator is the following:
     *  AroonUp   = factor*(optInTimePeriod-(today-highestIdx));
     *  AroonDown = factor*(optInTimePeriod-(today-lowestIdx));
     *  AroonOsc  = AroonUp-AroonDown;
     *
     * An arithmetic simplification give us:
     *  Aroon = factor*(highestIdx-lowestIdx)
     */
    aroon = factor * (highestIdx - lowestIdx);
    /* Note: Do not forget that input and output buffer can be the same,
     *       so writing to the output is the last thing being done here.
     */
    outReal[outIdx] = aroon;
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

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000']] as const;

/** @internal */
export function aroonoscPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('AROONOSC', 'startIdx', startIdx);
  endIdx = requireInt32('AROONOSC', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('AROONOSC', 'timePeriod', optInTimePeriod);
  requireIndexRange('AROONOSC', startIdx, endIdx);
  const guardLookback = aroonoscLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('AROONOSC', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('AROONOSC', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('AROONOSC', 'prices.high', inHigh, guardInLen);
  requireLength('AROONOSC', 'prices.low', inLow, guardInLen);
  requireOutput('AROONOSC', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlapsPartly(outReal, inHighF64)) {
    throw sharedMemory('AROONOSC', 'outReal', 'prices.high');
  }
  if (overlapsPartly(outReal, inLowF64)) {
    throw sharedMemory('AROONOSC', 'outReal', 'prices.low');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = aroonoscImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('AROONOSC', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link aroonosc}, {@link aroonoscLookback},
 * {@link aroonoscDisplayShift}, {@link aroonoscOpen} and
 * {@link aroonoscOpenAndFill}. Every key is optional: an omitted or `undefined`
 * key takes its default, and a call ignores the keys it has no use for.
 */
export interface AroonoscOptions {
  /**
   * Lookback window for locating the highest high and lowest low (default 14; range
   * 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
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
  outReal?: Float64Array<ArrayBuffer> | undefined;
}

/** What {@link aroonosc} returns, and {@link aroonoscOpenAndFill} with its stream. */
export interface AroonoscResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Aroon oscillator value (AroonUp - AroonDown). `values[k]` is the value at input
   * bar `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link aroonosc} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AroonoscOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function aroonoscLookback(options?: AroonoscOptions): number {
  const o = optionsOf('AROONOSC lookback', options);
  return aroonoscLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('AROONOSC lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link aroonosc}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AroonoscOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function aroonoscDisplayShift(outputIdx: number, options?: AroonoscOptions): number {
  const o = optionsOf('AROONOSC displayShift', options);
  return aroonoscDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('AROONOSC displayShift', o.core));
}

/**
 * Aroon Oscillator: AroonUp minus AroonDown over a lookback window. Measures trend
 * direction and strength on a -100..+100 scale. Positive when the high is more
 * recent than the low (up-trend); negative when the low is more recent
 * (down-trend).
 *
 * Formula and more info at
 * [ta-lib.org/functions/aroonosc](https://ta-lib.org/functions/aroonosc).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link aroonoscLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `high`: High price of
 *   each bar. `low`: Low price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link AroonoscOptions}.
 * @returns `begIdx` and the values; see {@link AroonoscResult}. Each output is a
 *   view of its buffer, the caller's or a new one.
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
 * import { aroonosc } from 'ta-lib';
 *
 * const high = new Float64Array(252); // your data, oldest first
 * const low = new Float64Array(252); // your data, oldest first
 *
 * const { begIdx, values } = aroonosc({ high, low }, { timePeriod: 14 });
 * console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
 * ```
 * @see [`aroon`](https://ta-lib.org/functions/aroon)
 * @see [`minmax`](https://ta-lib.org/functions/minmax)
 */
export function aroonosc(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: AroonoscOptions): AroonoscResult {
  requireArgument('AROONOSC', 'prices', prices);
  const o = optionsOf('AROONOSC', options);
  if (o.endIdx === undefined) {
    requireSeries('AROONOSC', 'prices.high', prices.high);
    requireSeries('AROONOSC', 'prices.low', prices.low);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('AROONOSC', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, aroonoscLookbackPositional(optInTimePeriod, core), prices.high, prices.low);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = aroonoscPositional(startIdx, endIdx, prices.high, prices.low, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live AROONOSC stream: one value per closed bar, bit-identical to
 * {@link aroonosc} over the same series. Open it with {@link aroonoscOpen} or
 * {@link aroonoscOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/aroonosc](https://ta-lib.org/functions/aroonosc).
 */
export class AroonoscStream {
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
  cur_outReal: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): AroonoscStream {
    return new AroonoscStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link aroonosc} reports them over the same bars. Each accepted `update` and
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
      throw streamFailure('AROONOSC advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw streamFailure('AROONOSC update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(high) || !Number.isFinite(low)) {
      throw nonFiniteBar('AROONOSC update', !Number.isFinite(high) ? 'high' : 'low');
    }
    aroonoscStepImpl(this, high, low);
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
      throw nonFiniteBar('AROONOSC peek', !Number.isFinite(high) ? 'high' : 'low');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
    let tmp = 0;
    let aroon = 0;
    let cur_outReal = 0;
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
    /* The oscillator is the following:
     *  AroonUp   = factor*(optInTimePeriod-(today-highestIdx));
     *  AroonDown = factor*(optInTimePeriod-(today-lowestIdx));
     *  AroonOsc  = AroonUp-AroonDown;
     *
     * An arithmetic simplification give us:
     *  Aroon = factor*(highestIdx-lowestIdx)
     */
    aroon = sp.factor * (highestIdx - lowestIdx);
    /* Note: Do not forget that input and output buffer can be the same,
     *       so writing to the output is the last thing being done here.
     */
    cur_outReal = aroon;
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
  clone(): AroonoscStream {
    const c = new AroonoscStream(this.core);
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
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function aroonoscStepImpl(sp: AroonoscStream, inHigh: number, inLow: number): void {
  let tmp = 0;
  let aroon = 0;
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
  /* The oscillator is the following:
   *  AroonUp   = factor*(optInTimePeriod-(today-highestIdx));
   *  AroonDown = factor*(optInTimePeriod-(today-lowestIdx));
   *  AroonOsc  = AroonUp-AroonDown;
   *
   * An arithmetic simplification give us:
   *  Aroon = factor*(highestIdx-lowestIdx)
   */
  aroon = sp.factor * (sp.highestIdx - sp.lowestIdx);
  /* Note: Do not forget that input and output buffer can be the same,
   *       so writing to the output is the last thing being done here.
   */
  sp.cur_outReal = aroon;
  sp.trailingIdx += 1;
  sp.today += 1;
}

function aroonoscOpenImpl(
  sp: AroonoscStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let lowest = 0;
  let highest = 0;
  let tmp = 0;
  let factor = 0;
  let aroon = 0;
  let outIdx = 0;
  let trailingIdx = 0;
  let lowestIdx = 0;
  let highestIdx = 0;
  let today = 0;
  let i = 0;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
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
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* This code is almost identical to the TA_AROON function
   * except that instead of outputing ArroonUp and AroonDown
   * individually, an oscillator is build from both.
   *
   *  AroonOsc = AroonUp- AroonDown;
   */
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
    /* The oscillator is the following:
     *  AroonUp   = factor*(optInTimePeriod-(today-highestIdx));
     *  AroonDown = factor*(optInTimePeriod-(today-lowestIdx));
     *  AroonOsc  = AroonUp-AroonDown;
     *
     * An arithmetic simplification give us:
     *  Aroon = factor*(highestIdx-lowestIdx)
     */
    aroon = factor * (highestIdx - lowestIdx);
    /* Note: Do not forget that input and output buffer can be the same,
     *       so writing to the output is the last thing being done here.
     */
    outReal[((outIdx * outStride) | 0)] = aroon;
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
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function aroonoscOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): AroonoscStream {
  const sp = AroonoscStream.of(core);
  const retCode = aroonoscOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('AROONOSC openAndFill', inHigh.length, startIdx, aroonoscLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && aroonoscLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('AROONOSC openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('AROONOSC openAndFill', retCode);
}

/** @internal */
export function aroonoscOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): AroonoscStream {
  const sp = AroonoscStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = aroonoscOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('AROONOSC open', inHigh.length, startIdx, aroonoscLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && aroonoscLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('AROONOSC open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('AROONOSC open', retCode);
}

/** @internal */
export function aroonoscOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): AroonoscStream {
  optInTimePeriod = requireInt32('AROONOSC open', 'timePeriod', optInTimePeriod);
  requireArgument('AROONOSC open', 'prices.high', inHigh);
  requireHistory('AROONOSC open', 'prices.high', inHigh.length);
  requireArgument('AROONOSC open', 'prices.low', inLow);
  requireHistoryLength('AROONOSC open', 'prices.low', inLow.length, inHigh.length);
  return aroonoscOpenInternal(core, float64(inHigh), float64(inLow), 0, optInTimePeriod);
}

/** @internal */
export function aroonoscOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): AroonoscStream {
  optInTimePeriod = requireInt32('AROONOSC openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('AROONOSC openAndFill', 'prices.high', inHigh);
  requireHistory('AROONOSC openAndFill', 'prices.high', inHigh.length);
  requireArgument('AROONOSC openAndFill', 'prices.low', inLow);
  const guardLookback = aroonoscLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('AROONOSC openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('AROONOSC openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('AROONOSC openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireOutput('AROONOSC openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('AROONOSC openAndFill', 'outReal', 'prices.high');
  }
  if (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('AROONOSC openAndFill', 'outReal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outReal, inHighF64)) {
    throw sharedMemory('AROONOSC openAndFill', 'outReal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outReal, inLowF64)) {
    throw sharedMemory('AROONOSC openAndFill', 'outReal', 'prices.low');
  }
  return aroonoscOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live AROONOSC stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link aroonosc} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AroonoscOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `aroonoscLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range
 *   or histories of different lengths.
 *
 * @example
 * ```ts
 * import { aroonoscOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = aroonoscOpen({ high, low }, { timePeriod: 14 });
 * const peeked = stream.peek(101.4, 99.1); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1); // the bar closes
 * ```
 */
export function aroonoscOpen(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: AroonoscOptions): AroonoscStream {
  requireArgument('AROONOSC open', 'prices', prices);
  const o = optionsOf('AROONOSC open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('AROONOSC open', o.core);
  return aroonoscOpenPositional(prices.high, prices.low, optInTimePeriod, core);
}

/**
 * {@link aroonoscOpen} that also computes the outputs over the whole history in
 * the same pass, bit-identically to {@link aroonosc}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link AroonoscOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link aroonosc} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `aroonoscLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { aroonoscOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = aroonoscOpenAndFill({ high, low }, {
 *   timePeriod: 14,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function aroonoscOpenAndFill(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: AroonoscOptions): AroonoscResult & { stream: AroonoscStream } {
  requireArgument('AROONOSC openAndFill', 'prices', prices);
  const o = optionsOf('AROONOSC openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('AROONOSC openAndFill', o.core);
  const capacity = fillCapacity(aroonoscLookbackPositional(optInTimePeriod, core), prices.high, prices.low);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = aroonoscOpenAndFillPositional(prices.high, prices.low, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
