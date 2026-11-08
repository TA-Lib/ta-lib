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
 *  AC       Angelo Ciceri
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  120305 AC     Creation
 *  071226 MF,CC  Streaming-friendly rewrite: carry the confirmation state
 *                (countdown + cached 2nd-candle high/low) instead of the absolute
 *                bar index, so the per-bar logic reads no cursor. Bit-identical
 *                batch results (verified vs v0.6.4).
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function cdlhikkakeLookbackPositional(_core: Core = Core.DEFAULT): number {
  return 5;
}

/** @internal */
export function cdlhikkakeDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdlhikkakeImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  _inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): RetCode {
  let i = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let patternResult = 0;
  let cd = 0;
  let savedHigh = 0;
  let savedLow = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  savedHigh = 0.0;
  savedLow = 0.0;
  /* Confirmation-window countdown + cached 2nd-candle high/low: the pattern
   * state carried without an absolute bar index.
   */
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlhikkakeLookbackPositional(core);
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
  /* Do the calculation using tight loops. */
  /* Add-up the initial period, except for the last value. */
  cd = 0;
  patternResult = 0;
  i = startIdx - 3;
  while (i < startIdx) {
    /* copy here the pattern recognition code below */
    if (inHigh[i - 1] < inHigh[i - 2] &&
        inLow[i - 1] > inLow[i - 2] &&   /* 1st + 2nd: lower high and higher low */
        (inHigh[i] < inHigh[i - 1] &&
          inLow[i] < inLow[i - 1] ||     /* (bull) 3rd: lower high and lower low */
         inHigh[i] > inHigh[i - 1] &&
          inLow[i] > inLow[i - 1]))      /* (bear) 3rd: higher high and higher low */
    {
      patternResult = ((100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : -1)) | 0);
      savedHigh = inHigh[i - 1];
      savedLow = inLow[i - 1];
      cd = 4;
    } else if (cd > 0 &&
        (patternResult > 0 &&       /* search for confirmation if hikkake was no more than 3 bars ago */
          inClose[i] > savedHigh || /* close higher than the high of 2nd */
         patternResult < 0 &&
          inClose[i] < savedLow))   /* close lower than the low of 2nd */
    {
      cd = 0;
    }
    if (cd > 0) {
      cd -= 1;
    }
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first and second candle: inside bar (2nd has lower high and higher low than 1st)
   * - third candle: lower high and lower low than 2nd (higher high and higher low than 2nd)
   * Confirmation could come in the next 3 days with:
   * - a day that closes higher than the high (lower than the low) of the 2nd candle
   */
  outIdx = 0;
  do {
    if (inHigh[i - 1] < inHigh[i - 2] &&
        inLow[i - 1] > inLow[i - 2] &&   /* 1st + 2nd: lower high and higher low */
        (inHigh[i] < inHigh[i - 1] &&
          inLow[i] < inLow[i - 1] ||     /* (bull) 3rd: lower high and lower low */
         inHigh[i] > inHigh[i - 1] &&
          inLow[i] > inLow[i - 1]))      /* (bear) 3rd: higher high and higher low */
    {
      patternResult = ((100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : -1)) | 0);
      savedHigh = inHigh[i - 1];
      savedLow = inLow[i - 1];
      cd = 4;
      outInteger[outIdx++] = patternResult;
    } else if (cd > 0 &&
        (patternResult > 0 &&       /* search for confirmation if hikkake was no more than 3 bars ago */
          inClose[i] > savedHigh || /* close higher than the high of 2nd */
         patternResult < 0 &&
          inClose[i] < savedLow))   /* close lower than the low of 2nd */
    {
      outInteger[outIdx++] = patternResult + ((100 * ((patternResult > 0) ? 1 : -1)) | 0);
      cd = 0;
    } else {
      outInteger[outIdx++] = 0;
    }
    if (cd > 0) {
      cd -= 1;
    }
    i += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlhikkakePositional(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDLHIKKAKE', 'startIdx', startIdx);
  endIdx = requireInt32('CDLHIKKAKE', 'endIdx', endIdx);
  requireIndexRange('CDLHIKKAKE', startIdx, endIdx);
  const guardStart = clampedStart('CDLHIKKAKE', startIdx, cdlhikkakeLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLHIKKAKE', 'prices.open', inOpen, guardInLen);
  requireLength('CDLHIKKAKE', 'prices.high', inHigh, guardInLen);
  requireLength('CDLHIKKAKE', 'prices.low', inLow, guardInLen);
  requireLength('CDLHIKKAKE', 'prices.close', inClose, guardInLen);
  requireOutput('CDLHIKKAKE', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLHIKKAKE', 'outInteger', 'prices.open');
  }
  if (overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLHIKKAKE', 'outInteger', 'prices.high');
  }
  if (overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLHIKKAKE', 'outInteger', 'prices.low');
  }
  if (overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLHIKKAKE', 'outInteger', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdlhikkakeImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLHIKKAKE', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cdlhikkake}, {@link cdlhikkakeLookback},
 * {@link cdlhikkakeDisplayShift}, {@link cdlhikkakeOpen} and
 * {@link cdlhikkakeOpenAndFill}. Every key is optional: an omitted or `undefined`
 * key takes its default, and a call ignores the keys it has no use for.
 */
export interface CdlhikkakeOptions {
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
   * buffer or with an input.
   */
  outInteger?: Int32Array<ArrayBuffer> | undefined;
}

/**
 * What {@link cdlhikkake} returns, and {@link cdlhikkakeOpenAndFill} with its
 * stream.
 */
export interface CdlhikkakeResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * +100/-100 at the hikkake (breakout) bar for bull/bear; +200/-200 at a later
   * confirmation bar; 0 otherwise. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Int32Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link cdlhikkake} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlhikkakeOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlhikkakeLookback(options?: CdlhikkakeOptions): number {
  const o = optionsOf('CDLHIKKAKE lookback', options);
  return cdlhikkakeLookbackPositional(coreOf('CDLHIKKAKE lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdlhikkake}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlhikkakeOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlhikkakeDisplayShift(outputIdx: number, options?: CdlhikkakeOptions): number {
  const o = optionsOf('CDLHIKKAKE displayShift', options);
  return cdlhikkakeDisplayShiftPositional(outputIdx, coreOf('CDLHIKKAKE displayShift', o.core));
}

/**
 * A 3-bar pattern: an inside bar followed by a false breakout, optionally later
 * confirmed by a follow-through bar. Signals a bullish or bearish
 * reversal/continuation depending on the breakout direction. A false-breakout
 * setup: positive = bullish, negative = bearish; magnitude 200 flags the
 * confirming bar.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlhikkake](https://ta-lib.org/functions/cdlhikkake).
 *
 * **Notes**
 * - The name comes from the Japanese word for a deceptive move or "trap" —
 *   fitting, since the pattern exists to catch traders acting on a false breakout.
 *   Bulkowski's testing of the confirmed pattern found the trap itself barely
 *   beats a coin flip: the bullish variant continues as expected only 52% of the
 *   time and the bearish variant exactly 50% ("random"), both ranking in the
 *   bottom fifth (83rd-84th of 105) for post-breakout performance.
 *   ([thepatternsite.com](https://thepatternsite.com/HikkakeBull.html))
 * - A bar that is both a new setup and the confirmation of an earlier one reports
 *   the new setup; the earlier one ends unconfirmed.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cdlhikkakeLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `open`: Open price of
 *   each bar. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link CdlhikkakeOptions}.
 * @returns `begIdx` and the values; see {@link CdlhikkakeResult}. Each output is a
 *   view of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, an input too short for the range, a buffer that is empty, too short or of
 *   the wrong type or a buffer sharing memory with a `Float64Array` input it is not.
 *   Checked before anything is written.
 *
 * @example
 * ```ts
 * import { cdlhikkake } from 'ta-lib';
 *
 * const open = new Float64Array(252); // your data, oldest first
 * const high = new Float64Array(252); // your data, oldest first
 * const low = new Float64Array(252); // your data, oldest first
 * const close = new Float64Array(252); // your data, oldest first
 *
 * const { begIdx, values } = cdlhikkake({ open, high, low, close });
 * console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
 * ```
 * @see [`cdlhikkakemod`](https://ta-lib.org/functions/cdlhikkakemod)
 * @see [`cdlharami`](https://ta-lib.org/functions/cdlharami)
 */
export function cdlhikkake(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlhikkakeOptions): CdlhikkakeResult {
  requireArgument('CDLHIKKAKE', 'prices', prices);
  const o = optionsOf('CDLHIKKAKE', options);
  if (o.endIdx === undefined) {
    requireSeries('CDLHIKKAKE', 'prices.open', prices.open);
    requireSeries('CDLHIKKAKE', 'prices.high', prices.high);
    requireSeries('CDLHIKKAKE', 'prices.low', prices.low);
    requireSeries('CDLHIKKAKE', 'prices.close', prices.close);
  }
  const core = coreOf('CDLHIKKAKE', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cdlhikkakeLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const range = cdlhikkakePositional(startIdx, endIdx, prices.open, prices.high, prices.low, prices.close, outInteger, core);
  return { begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CDLHIKKAKE stream: one value per closed bar, bit-identical to
 * {@link cdlhikkake} over the same series. Open it with {@link cdlhikkakeOpen} or
 * {@link cdlhikkakeOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlhikkake](https://ta-lib.org/functions/cdlhikkake).
 */
export class CdlhikkakeStream {
  /** @internal */
  core: Core;
  /** @internal */
  patternResult: number = 0;
  /** @internal */
  cd: number = 0;
  /** @internal */
  savedHigh: number = 0;
  /** @internal */
  savedLow: number = 0;
  /** @internal */
  lag1_inHigh: number = 0;
  /** @internal */
  lag2_inHigh: number = 0;
  /** @internal */
  lag1_inLow: number = 0;
  /** @internal */
  lag2_inLow: number = 0;
  /** @internal */
  cur_outInteger: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): CdlhikkakeStream {
    return new CdlhikkakeStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cdlhikkake} reports them over the same bars. Each accepted `update` and
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
      throw streamFailure('CDLHIKKAKE advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw streamFailure('CDLHIKKAKE update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(open) || !Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('CDLHIKKAKE update', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    cdlhikkakeStepImpl(this, open, high, low, close);
    this.outRangeCount++;
    return this.cur_outInteger;
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
      throw nonFiniteBar('CDLHIKKAKE peek', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
    const inClose = close;
    let cd = sp.cd;
    let cur_outInteger = 0;
    let patternResult = sp.patternResult;
    let savedHigh = sp.savedHigh;
    let savedLow = sp.savedLow;
    if (sp.lag1_inHigh < sp.lag2_inHigh &&
        sp.lag1_inLow > sp.lag2_inLow &&   /* 1st + 2nd: lower high and higher low */
        (inHigh < sp.lag1_inHigh &&
          inLow < sp.lag1_inLow ||         /* (bull) 3rd: lower high and lower low */
         inHigh > sp.lag1_inHigh &&
          inLow > sp.lag1_inLow))          /* (bear) 3rd: higher high and higher low */
    {
      patternResult = ((100 * ((inHigh < sp.lag1_inHigh) ? 1 : -1)) | 0);
      savedHigh = sp.lag1_inHigh;
      savedLow = sp.lag1_inLow;
      cd = 4;
      cur_outInteger = patternResult;
    } else if (cd > 0 &&
        (patternResult > 0 &&    /* search for confirmation if hikkake was no more than 3 bars ago */
          inClose > savedHigh || /* close higher than the high of 2nd */
         patternResult < 0 &&
          inClose < savedLow))   /* close lower than the low of 2nd */
    {
      cur_outInteger = patternResult + ((100 * ((patternResult > 0) ? 1 : -1)) | 0);
      cd = 0;
    } else {
      cur_outInteger = 0;
    }
    return cur_outInteger;
  }

  /** The value at the last bar this stream counted; `peek` does not change it. */
  value(): number {
    return this.cur_outInteger;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): CdlhikkakeStream {
    const c = new CdlhikkakeStream(this.core);
    c.patternResult = this.patternResult;
    c.cd = this.cd;
    c.savedHigh = this.savedHigh;
    c.savedLow = this.savedLow;
    c.lag1_inHigh = this.lag1_inHigh;
    c.lag2_inHigh = this.lag2_inHigh;
    c.lag1_inLow = this.lag1_inLow;
    c.lag2_inLow = this.lag2_inLow;
    c.cur_outInteger = this.cur_outInteger;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cdlhikkakeStepImpl(sp: CdlhikkakeStream, _inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  if (sp.lag1_inHigh < sp.lag2_inHigh &&
      sp.lag1_inLow > sp.lag2_inLow &&   /* 1st + 2nd: lower high and higher low */
      (inHigh < sp.lag1_inHigh &&
        inLow < sp.lag1_inLow ||         /* (bull) 3rd: lower high and lower low */
       inHigh > sp.lag1_inHigh &&
        inLow > sp.lag1_inLow))          /* (bear) 3rd: higher high and higher low */
  {
    sp.patternResult = ((100 * ((inHigh < sp.lag1_inHigh) ? 1 : -1)) | 0);
    sp.savedHigh = sp.lag1_inHigh;
    sp.savedLow = sp.lag1_inLow;
    sp.cd = 4;
    sp.cur_outInteger = sp.patternResult;
  } else if (sp.cd > 0 &&
      (sp.patternResult > 0 &&    /* search for confirmation if hikkake was no more than 3 bars ago */
        inClose > sp.savedHigh || /* close higher than the high of 2nd */
       sp.patternResult < 0 &&
        inClose < sp.savedLow))   /* close lower than the low of 2nd */
  {
    sp.cur_outInteger = sp.patternResult + ((100 * ((sp.patternResult > 0) ? 1 : -1)) | 0);
    sp.cd = 0;
  } else {
    sp.cur_outInteger = 0;
  }
  if (sp.cd > 0) {
    sp.cd -= 1;
  }
  sp.lag2_inHigh = sp.lag1_inHigh;
  sp.lag1_inHigh = inHigh;
  sp.lag2_inLow = sp.lag1_inLow;
  sp.lag1_inLow = inLow;
}

function cdlhikkakeOpenImpl(
  sp: CdlhikkakeStream,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let i = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let patternResult = 0;
  let cd = 0;
  let savedHigh = 0;
  let savedLow = 0;
  const historyLen = inOpen.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inHigh.length !== inOpen.length || inLow.length !== inOpen.length || inClose.length !== inOpen.length) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  savedHigh = 0.0;
  savedLow = 0.0;
  /* Confirmation-window countdown + cached 2nd-candle high/low: the pattern
   * state carried without an absolute bar index.
   */
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlhikkakeLookbackPositional(core);
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
  /* Do the calculation using tight loops. */
  /* Add-up the initial period, except for the last value. */
  cd = 0;
  patternResult = 0;
  i = startIdx - 3;
  while (i < startIdx) {
    /* copy here the pattern recognition code below */
    if (inHigh[i - 1] < inHigh[i - 2] &&
        inLow[i - 1] > inLow[i - 2] &&   /* 1st + 2nd: lower high and higher low */
        (inHigh[i] < inHigh[i - 1] &&
          inLow[i] < inLow[i - 1] ||     /* (bull) 3rd: lower high and lower low */
         inHigh[i] > inHigh[i - 1] &&
          inLow[i] > inLow[i - 1]))      /* (bear) 3rd: higher high and higher low */
    {
      patternResult = ((100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : -1)) | 0);
      savedHigh = inHigh[i - 1];
      savedLow = inLow[i - 1];
      cd = 4;
    } else if (cd > 0 &&
        (patternResult > 0 &&       /* search for confirmation if hikkake was no more than 3 bars ago */
          inClose[i] > savedHigh || /* close higher than the high of 2nd */
         patternResult < 0 &&
          inClose[i] < savedLow))   /* close lower than the low of 2nd */
    {
      cd = 0;
    }
    if (cd > 0) {
      cd -= 1;
    }
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first and second candle: inside bar (2nd has lower high and higher low than 1st)
   * - third candle: lower high and lower low than 2nd (higher high and higher low than 2nd)
   * Confirmation could come in the next 3 days with:
   * - a day that closes higher than the high (lower than the low) of the 2nd candle
   */
  outIdx = 0;
  do {
    if (inHigh[i - 1] < inHigh[i - 2] &&
        inLow[i - 1] > inLow[i - 2] &&   /* 1st + 2nd: lower high and higher low */
        (inHigh[i] < inHigh[i - 1] &&
          inLow[i] < inLow[i - 1] ||     /* (bull) 3rd: lower high and lower low */
         inHigh[i] > inHigh[i - 1] &&
          inLow[i] > inLow[i - 1]))      /* (bear) 3rd: higher high and higher low */
    {
      patternResult = ((100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : -1)) | 0);
      savedHigh = inHigh[i - 1];
      savedLow = inLow[i - 1];
      cd = 4;
      outInteger[((outIdx++ * outStride) | 0)] = patternResult;
    } else if (cd > 0 &&
        (patternResult > 0 &&       /* search for confirmation if hikkake was no more than 3 bars ago */
          inClose[i] > savedHigh || /* close higher than the high of 2nd */
         patternResult < 0 &&
          inClose[i] < savedLow))   /* close lower than the low of 2nd */
    {
      outInteger[((outIdx++ * outStride) | 0)] = patternResult + ((100 * ((patternResult > 0) ? 1 : -1)) | 0);
      cd = 0;
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    if (cd > 0) {
      cd -= 1;
    }
    i += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  sp.patternResult = patternResult;
  sp.cd = cd;
  sp.savedHigh = savedHigh;
  sp.savedLow = savedLow;
  sp.lag1_inHigh = inHigh[historyLen - 1];
  sp.lag2_inHigh = inHigh[historyLen - 2];
  sp.lag1_inLow = inLow[historyLen - 1];
  sp.lag2_inLow = inLow[historyLen - 2];
  sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlhikkakeOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): CdlhikkakeStream {
  const sp = CdlhikkakeStream.of(core);
  const retCode = cdlhikkakeOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLHIKKAKE openAndFill', inOpen.length, startIdx, cdlhikkakeLookbackPositional(core));
  }
  throw streamFailure('CDLHIKKAKE openAndFill', retCode);
}

/** @internal */
export function cdlhikkakeOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): CdlhikkakeStream {
  const sp = CdlhikkakeStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdlhikkakeOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLHIKKAKE open', inOpen.length, startIdx, cdlhikkakeLookbackPositional(core));
  }
  throw streamFailure('CDLHIKKAKE open', retCode);
}

/** @internal */
export function cdlhikkakeOpenPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): CdlhikkakeStream {
  requireArgument('CDLHIKKAKE open', 'prices.open', inOpen);
  requireHistory('CDLHIKKAKE open', 'prices.open', inOpen.length);
  requireArgument('CDLHIKKAKE open', 'prices.high', inHigh);
  requireArgument('CDLHIKKAKE open', 'prices.low', inLow);
  requireArgument('CDLHIKKAKE open', 'prices.close', inClose);
  requireHistoryLength('CDLHIKKAKE open', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLHIKKAKE open', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLHIKKAKE open', 'prices.close', inClose.length, inOpen.length);
  return cdlhikkakeOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/** @internal */
export function cdlhikkakeOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): CdlhikkakeStream {
  requireArgument('CDLHIKKAKE openAndFill', 'prices.open', inOpen);
  requireHistory('CDLHIKKAKE openAndFill', 'prices.open', inOpen.length);
  requireArgument('CDLHIKKAKE openAndFill', 'prices.high', inHigh);
  requireArgument('CDLHIKKAKE openAndFill', 'prices.low', inLow);
  requireArgument('CDLHIKKAKE openAndFill', 'prices.close', inClose);
  const guardOutLen = openFillCount('CDLHIKKAKE openAndFill', inOpen.length, cdlhikkakeLookbackPositional(core));
  requireHistoryLength('CDLHIKKAKE openAndFill', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLHIKKAKE openAndFill', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLHIKKAKE openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('CDLHIKKAKE openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('CDLHIKKAKE openAndFill', 'outInteger', 'prices.open');
  }
  if (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CDLHIKKAKE openAndFill', 'outInteger', 'prices.high');
  }
  if (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CDLHIKKAKE openAndFill', 'outInteger', 'prices.low');
  }
  if (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('CDLHIKKAKE openAndFill', 'outInteger', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLHIKKAKE openAndFill', 'outInteger', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLHIKKAKE openAndFill', 'outInteger', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLHIKKAKE openAndFill', 'outInteger', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLHIKKAKE openAndFill', 'outInteger', 'prices.close');
  }
  return cdlhikkakeOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}

/**
 * Opens a live CDLHIKKAKE stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link cdlhikkake} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlhikkakeOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlhikkakeLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input or histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdlhikkakeOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdlhikkakeOpen({ open, high, low, close });
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdlhikkakeOpen(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlhikkakeOptions): CdlhikkakeStream {
  requireArgument('CDLHIKKAKE open', 'prices', prices);
  const o = optionsOf('CDLHIKKAKE open', options);
  const core = coreOf('CDLHIKKAKE open', o.core);
  return cdlhikkakeOpenPositional(prices.open, prices.high, prices.low, prices.close, core);
}

/**
 * {@link cdlhikkakeOpen} that also computes the outputs over the whole history in
 * the same pass, bit-identically to {@link cdlhikkake}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CdlhikkakeOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cdlhikkake} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlhikkakeLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, histories of different lengths or a buffer too short,
 *   of the wrong type or starting where another buffer or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdlhikkakeOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = cdlhikkakeOpenAndFill({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cdlhikkakeOpenAndFill(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlhikkakeOptions): CdlhikkakeResult & { stream: CdlhikkakeStream } {
  requireArgument('CDLHIKKAKE openAndFill', 'prices', prices);
  const o = optionsOf('CDLHIKKAKE openAndFill', options);
  const core = coreOf('CDLHIKKAKE openAndFill', o.core);
  const capacity = fillCapacity(cdlhikkakeLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const stream = cdlhikkakeOpenAndFillPositional(prices.open, prices.high, prices.low, prices.close, outInteger, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}
