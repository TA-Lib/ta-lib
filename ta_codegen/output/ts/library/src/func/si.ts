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
 *  092726 MF,CC  Initial version (#451).
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireReal, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function siLookbackPositional(optInLimitMove: number, _core: Core = Core.DEFAULT): number {
  if (typeof optInLimitMove !== 'number') {
    return -1;
  }
  if (optInLimitMove === Core.REAL_DEFAULT) {
    optInLimitMove = 3.0;
  } else if (!(optInLimitMove >= 1e-8 && optInLimitMove <= Core.REAL_MAX)) {
    return -1;
  }
  return 1;
}

/** @internal */
export function siDisplayShiftPositional(optInLimitMove: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (siLookbackPositional(optInLimitMove, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function siImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  optInLimitMove: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let i = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let prevClose = 0;
  let prevBody = 0;
  let tempOpen = 0;
  let tempHigh = 0;
  let tempLow = 0;
  let tempClose = 0;
  let body = 0;
  let n = 0;
  let up = 0;
  let dn = 0;
  let rg = 0;
  let k = 0;
  let r = 0;
  let swing = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInLimitMove === Core.REAL_DEFAULT) {
    optInLimitMove = 3.0;
  } else if (!(optInLimitMove >= 1e-8 && optInLimitMove <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  lookbackTotal = siLookbackPositional(optInLimitMove, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  prevClose = inClose[startIdx - 1];
  prevBody = prevClose - inOpen[startIdx - 1];
  outIdx = 0;
  for (i = startIdx; i <= endIdx; i += 1) {
    tempOpen = inOpen[i];
    tempHigh = inHigh[i];
    tempLow = inLow[i];
    tempClose = inClose[i];
    body = tempClose - tempOpen;
    n = tempClose - prevClose;
    n += 0.5 * body;
    n += 0.25 * prevBody;
    up = Math.abs(tempHigh - prevClose);
    dn = Math.abs(tempLow - prevClose);
    rg = Math.abs(tempHigh - tempLow);
    k = (up > dn ? up : dn);
    /* Wilder's three cases of R are this one max, since rg is up + dn or
     * |up - dn|. Keep it branch-free: the case chain mispredicts on most
     * bars of real data. The four backends' max builtins agree only while
     * no operand is -0.0 or NaN, which holds while up and dn are finite.
     */
    r = ((up - 0.5 * dn) > (dn - 0.5 * up) ? (up - 0.5 * dn) : (dn - 0.5 * up));
    r = (r > rg ? r : rg);
    r += 0.25 * Math.abs(prevBody);
    if (r === 0.0) {
      swing = 0.0;
    } else {
      swing = 50.0 * (n / r) * (k / optInLimitMove);
    }
    outReal[outIdx] = swing;
    outIdx += 1;
    prevClose = tempClose;
    prevBody = body;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['limitMove', Core.REAL_DEFAULT, 1e-8, Core.REAL_MAX, 'in 0.00000001..3e37']] as const;

/** @internal */
export function siPositional(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInLimitMove: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('SI', 'startIdx', startIdx);
  endIdx = requireInt32('SI', 'endIdx', endIdx);
  optInLimitMove = requireReal('SI', 'limitMove', optInLimitMove);
  requireIndexRange('SI', startIdx, endIdx);
  const guardLookback = siLookbackPositional(optInLimitMove, core);
  if (guardLookback < 0) {
    throw rejectedParams('SI', PARAM_RANGES, optInLimitMove);
  }
  const guardStart = clampedStart('SI', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('SI', 'prices.open', inOpen, guardInLen);
  requireLength('SI', 'prices.high', inHigh, guardInLen);
  requireLength('SI', 'prices.low', inLow, guardInLen);
  requireLength('SI', 'prices.close', inClose, guardInLen);
  requireOutput('SI', 'outReal', outReal, guardOutLen, Float64Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outReal, inOpenF64)) {
    throw sharedMemory('SI', 'outReal', 'prices.open');
  }
  if (overlapsPartly(outReal, inHighF64)) {
    throw sharedMemory('SI', 'outReal', 'prices.high');
  }
  if (overlapsPartly(outReal, inLowF64)) {
    throw sharedMemory('SI', 'outReal', 'prices.low');
  }
  if (overlapsPartly(outReal, inCloseF64)) {
    throw sharedMemory('SI', 'outReal', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = siImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, optInLimitMove, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('SI', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link si}, {@link siLookback}, {@link siDisplayShift},
 * {@link siOpen} and {@link siOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface SiOptions {
  /**
   * Limit move, the largest one-bar price move the index is scaled against, in price
   * units (default 3; minimum 0.00000001; `Core.REAL_DEFAULT` selects the default).
   */
  limitMove?: number | undefined;
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

/** What {@link si} returns, and {@link siOpenAndFill} with its stream. */
export interface SiResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Swing index of the bar against the previous bar. `values[k]` is the value at
   * input bar `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link si} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SiOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function siLookback(options?: SiOptions): number {
  const o = optionsOf('SI lookback', options);
  return siLookbackPositional(orDefault(o.limitMove, Core.REAL_DEFAULT), coreOf('SI lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link si}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SiOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function siDisplayShift(outputIdx: number, options?: SiOptions): number {
  const o = optionsOf('SI displayShift', options);
  return siDisplayShiftPositional(orDefault(o.limitMove, Core.REAL_DEFAULT), outputIdx, coreOf('SI displayShift', o.core));
}

/**
 * Wilder's Swing Index: one number rating each bar against the bar before it, from
 * both bars' open and close and the current bar's high and low, signed by the
 * direction of the swing. A strong close above the prior close on a wide range
 * reads high and positive, a strong down swing reads high and negative. It is
 * scaled against the limit move, the largest one-bar move the market allows, and
 * stays between -100 and +100 while every move is within that limit. J. Welles
 * Wilder Jr. introduced it in 1978 and trades its running total, ASI.
 *
 * Formula and more info at
 * [ta-lib.org/functions/si](https://ta-lib.org/functions/si).
 *
 * **Notes**
 * - Wilder rounds each value to a whole number when working by hand; the output is
 *   not rounded.
 * - A bar that moves more than the limit move can read beyond -100 or +100; the
 *   output is not clamped.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link siLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `open`: Open price of
 *   each bar. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link SiOptions}.
 * @returns `begIdx` and the values; see {@link SiResult}. Each output is a view of
 *   its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, a real parameter that is not a number, a parameter outside its range, an
 *   input too short for the range, a buffer that is empty, too short or of the wrong
 *   type or a buffer sharing memory with a `Float64Array` input it is not. Checked
 *   before anything is written.
 *
 * @example
 * ```ts
 * import { si } from 'ta-lib';
 *
 * const open = new Float64Array(252); // your data, oldest first
 * const high = new Float64Array(252); // your data, oldest first
 * const low = new Float64Array(252); // your data, oldest first
 * const close = new Float64Array(252); // your data, oldest first
 *
 * const { begIdx, values } = si({ open, high, low, close }, { limitMove: 3 });
 * console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
 * ```
 * @see [`asi`](https://ta-lib.org/functions/asi)
 * @see [`wad`](https://ta-lib.org/functions/wad)
 * @see [`trange`](https://ta-lib.org/functions/trange)
 * @see [`bop`](https://ta-lib.org/functions/bop)
 */
export function si(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: SiOptions): SiResult {
  requireArgument('SI', 'prices', prices);
  const o = optionsOf('SI', options);
  if (o.endIdx === undefined) {
    requireSeries('SI', 'prices.open', prices.open);
    requireSeries('SI', 'prices.high', prices.high);
    requireSeries('SI', 'prices.low', prices.low);
    requireSeries('SI', 'prices.close', prices.close);
  }
  const optInLimitMove = orDefault(o.limitMove, Core.REAL_DEFAULT);
  const core = coreOf('SI', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, siLookbackPositional(optInLimitMove, core), prices.open, prices.high, prices.low, prices.close);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = siPositional(startIdx, endIdx, prices.open, prices.high, prices.low, prices.close, optInLimitMove, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live SI stream: one value per closed bar, bit-identical to {@link si} over the
 * same series. Open it with {@link siOpen} or {@link siOpenAndFill}; there is
 * nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/si](https://ta-lib.org/functions/si).
 */
export class SiStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInLimitMove: number = 0;
  /** @internal */
  prevClose: number = 0;
  /** @internal */
  prevBody: number = 0;
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
  static of(core: Core): SiStream {
    return new SiStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link si} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('SI advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param open - The bar's `open` value: Open price of each bar.
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(open: number, high: number, low: number, close: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('SI update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(open) || !Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('SI update', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    siStepImpl(this, open, high, low, close);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param open - The bar's `open` value: Open price of each bar.
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(open: number, high: number, low: number, close: number): number {
    if (!Number.isFinite(open) || !Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('SI peek', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inOpen = open;
    const inHigh = high;
    const inLow = low;
    const inClose = close;
    let tempOpen = 0;
    let tempHigh = 0;
    let tempLow = 0;
    let tempClose = 0;
    let body = 0;
    let n = 0;
    let up = 0;
    let dn = 0;
    let rg = 0;
    let k = 0;
    let r = 0;
    let swing = 0;
    let cur_outReal = 0;
    tempOpen = inOpen;
    tempHigh = inHigh;
    tempLow = inLow;
    tempClose = inClose;
    body = tempClose - tempOpen;
    n = tempClose - sp.prevClose;
    n += 0.5 * body;
    n += 0.25 * sp.prevBody;
    up = Math.abs(tempHigh - sp.prevClose);
    dn = Math.abs(tempLow - sp.prevClose);
    rg = Math.abs(tempHigh - tempLow);
    k = (up > dn ? up : dn);
    /* Wilder's three cases of R are this one max, since rg is up + dn or
     * |up - dn|. Keep it branch-free: the case chain mispredicts on most
     * bars of real data. The four backends' max builtins agree only while
     * no operand is -0.0 or NaN, which holds while up and dn are finite.
     */
    r = ((up - 0.5 * dn) > (dn - 0.5 * up) ? (up - 0.5 * dn) : (dn - 0.5 * up));
    r = (r > rg ? r : rg);
    r += 0.25 * Math.abs(sp.prevBody);
    if (r === 0.0) {
      swing = 0.0;
    } else {
      swing = 50.0 * (n / r) * (k / sp.optInLimitMove);
    }
    cur_outReal = swing;
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
  clone(): SiStream {
    const c = new SiStream(this.core);
    c.optInLimitMove = this.optInLimitMove;
    c.prevClose = this.prevClose;
    c.prevBody = this.prevBody;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function siStepImpl(sp: SiStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  let tempOpen = 0;
  let tempHigh = 0;
  let tempLow = 0;
  let tempClose = 0;
  let body = 0;
  let n = 0;
  let up = 0;
  let dn = 0;
  let rg = 0;
  let k = 0;
  let r = 0;
  let swing = 0;
  tempOpen = inOpen;
  tempHigh = inHigh;
  tempLow = inLow;
  tempClose = inClose;
  body = tempClose - tempOpen;
  n = tempClose - sp.prevClose;
  n += 0.5 * body;
  n += 0.25 * sp.prevBody;
  up = Math.abs(tempHigh - sp.prevClose);
  dn = Math.abs(tempLow - sp.prevClose);
  rg = Math.abs(tempHigh - tempLow);
  k = (up > dn ? up : dn);
  /* Wilder's three cases of R are this one max, since rg is up + dn or
   * |up - dn|. Keep it branch-free: the case chain mispredicts on most
   * bars of real data. The four backends' max builtins agree only while
   * no operand is -0.0 or NaN, which holds while up and dn are finite.
   */
  r = ((up - 0.5 * dn) > (dn - 0.5 * up) ? (up - 0.5 * dn) : (dn - 0.5 * up));
  r = (r > rg ? r : rg);
  r += 0.25 * Math.abs(sp.prevBody);
  if (r === 0.0) {
    swing = 0.0;
  } else {
    swing = 50.0 * (n / r) * (k / sp.optInLimitMove);
  }
  sp.cur_outReal = swing;
  sp.prevClose = tempClose;
  sp.prevBody = body;
}

function siOpenImpl(
  sp: SiStream,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInLimitMove: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let i = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let prevClose = 0;
  let prevBody = 0;
  let tempOpen = 0;
  let tempHigh = 0;
  let tempLow = 0;
  let tempClose = 0;
  let body = 0;
  let n = 0;
  let up = 0;
  let dn = 0;
  let rg = 0;
  let k = 0;
  let r = 0;
  let swing = 0;
  const historyLen = inOpen.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inHigh.length !== inOpen.length || inLow.length !== inOpen.length || inClose.length !== inOpen.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInLimitMove === Core.REAL_DEFAULT) {
    optInLimitMove = 3.0;
  } else if (!(optInLimitMove >= 1e-8 && optInLimitMove <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  lookbackTotal = siLookbackPositional(optInLimitMove, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  prevClose = inClose[startIdx - 1];
  prevBody = prevClose - inOpen[startIdx - 1];
  outIdx = 0;
  for (i = startIdx; i <= endIdx; i += 1) {
    tempOpen = inOpen[i];
    tempHigh = inHigh[i];
    tempLow = inLow[i];
    tempClose = inClose[i];
    body = tempClose - tempOpen;
    n = tempClose - prevClose;
    n += 0.5 * body;
    n += 0.25 * prevBody;
    up = Math.abs(tempHigh - prevClose);
    dn = Math.abs(tempLow - prevClose);
    rg = Math.abs(tempHigh - tempLow);
    k = (up > dn ? up : dn);
    /* Wilder's three cases of R are this one max, since rg is up + dn or
     * |up - dn|. Keep it branch-free: the case chain mispredicts on most
     * bars of real data. The four backends' max builtins agree only while
     * no operand is -0.0 or NaN, which holds while up and dn are finite.
     */
    r = ((up - 0.5 * dn) > (dn - 0.5 * up) ? (up - 0.5 * dn) : (dn - 0.5 * up));
    r = (r > rg ? r : rg);
    r += 0.25 * Math.abs(prevBody);
    if (r === 0.0) {
      swing = 0.0;
    } else {
      swing = 50.0 * (n / r) * (k / optInLimitMove);
    }
    outReal[((outIdx * outStride) | 0)] = swing;
    outIdx += 1;
    prevClose = tempClose;
    prevBody = body;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  sp.optInLimitMove = optInLimitMove;
  sp.prevClose = prevClose;
  sp.prevBody = prevBody;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function siOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInLimitMove: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): SiStream {
  const sp = SiStream.of(core);
  const retCode = siOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, optInLimitMove, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SI openAndFill', inOpen.length, startIdx, siLookbackPositional(optInLimitMove, core));
  }
  if (retCode === RetCode.BAD_PARAM && siLookbackPositional(optInLimitMove, core) < 0) {
    throw rejectedParams('SI openAndFill', PARAM_RANGES, optInLimitMove);
  }
  throw streamFailure('SI openAndFill', retCode);
}

/** @internal */
export function siOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInLimitMove: number,
): SiStream {
  const sp = SiStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = siOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, optInLimitMove, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SI open', inOpen.length, startIdx, siLookbackPositional(optInLimitMove, core));
  }
  if (retCode === RetCode.BAD_PARAM && siLookbackPositional(optInLimitMove, core) < 0) {
    throw rejectedParams('SI open', PARAM_RANGES, optInLimitMove);
  }
  throw streamFailure('SI open', retCode);
}

/** @internal */
export function siOpenPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInLimitMove: number,
  core: Core = Core.DEFAULT,
): SiStream {
  optInLimitMove = requireReal('SI open', 'limitMove', optInLimitMove);
  requireArgument('SI open', 'prices.open', inOpen);
  requireHistory('SI open', 'prices.open', inOpen.length);
  requireArgument('SI open', 'prices.high', inHigh);
  requireArgument('SI open', 'prices.low', inLow);
  requireArgument('SI open', 'prices.close', inClose);
  requireHistoryLength('SI open', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('SI open', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('SI open', 'prices.close', inClose.length, inOpen.length);
  return siOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0, optInLimitMove);
}

/** @internal */
export function siOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInLimitMove: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): SiStream {
  optInLimitMove = requireReal('SI openAndFill', 'limitMove', optInLimitMove);
  requireArgument('SI openAndFill', 'prices.open', inOpen);
  requireHistory('SI openAndFill', 'prices.open', inOpen.length);
  requireArgument('SI openAndFill', 'prices.high', inHigh);
  requireArgument('SI openAndFill', 'prices.low', inLow);
  requireArgument('SI openAndFill', 'prices.close', inClose);
  const guardLookback = siLookbackPositional(optInLimitMove, core);
  if (guardLookback < 0) {
    throw rejectedParams('SI openAndFill', PARAM_RANGES, optInLimitMove);
  }
  const guardOutLen = openFillCount('SI openAndFill', inOpen.length, guardLookback);
  requireHistoryLength('SI openAndFill', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('SI openAndFill', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('SI openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('SI openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outReal.buffer === inOpenF64.buffer && outReal.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('SI openAndFill', 'outReal', 'prices.open');
  }
  if (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('SI openAndFill', 'outReal', 'prices.high');
  }
  if (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('SI openAndFill', 'outReal', 'prices.low');
  }
  if (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('SI openAndFill', 'outReal', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outReal, inOpenF64)) {
    throw sharedMemory('SI openAndFill', 'outReal', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outReal, inHighF64)) {
    throw sharedMemory('SI openAndFill', 'outReal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outReal, inLowF64)) {
    throw sharedMemory('SI openAndFill', 'outReal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outReal, inCloseF64)) {
    throw sharedMemory('SI openAndFill', 'outReal', 'prices.close');
  }
  return siOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, optInLimitMove, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live SI stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link si} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SiOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `siLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   a real parameter that is not a number, a parameter outside its range or
 *   histories of different lengths.
 *
 * @example
 * ```ts
 * import { siOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = siOpen({ open, high, low, close }, { limitMove: 3 });
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function siOpen(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: SiOptions): SiStream {
  requireArgument('SI open', 'prices', prices);
  const o = optionsOf('SI open', options);
  const optInLimitMove = orDefault(o.limitMove, Core.REAL_DEFAULT);
  const core = coreOf('SI open', o.core);
  return siOpenPositional(prices.open, prices.high, prices.low, prices.close, optInLimitMove, core);
}

/**
 * {@link siOpen} that also computes the outputs over the whole history in the same
 * pass, bit-identically to {@link si}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link SiOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link si} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `siLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   a real parameter that is not a number, a parameter outside its range, histories
 *   of different lengths or a buffer too short, of the wrong type or starting where
 *   another buffer or a `Float64Array` input starts, or, on a history long enough to
 *   open, one sharing memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { siOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = siOpenAndFill({ open, high, low, close }, {
 *   limitMove: 3,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function siOpenAndFill(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: SiOptions): SiResult & { stream: SiStream } {
  requireArgument('SI openAndFill', 'prices', prices);
  const o = optionsOf('SI openAndFill', options);
  const optInLimitMove = orDefault(o.limitMove, Core.REAL_DEFAULT);
  const core = coreOf('SI openAndFill', o.core);
  const capacity = fillCapacity(siLookbackPositional(optInLimitMove, core), prices.open, prices.high, prices.low, prices.close);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = siOpenAndFillPositional(prices.open, prices.high, prices.low, prices.close, optInLimitMove, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
