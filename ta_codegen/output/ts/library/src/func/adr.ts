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
 *  090426 MF,CC  Initial version (#367).
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function adrLookbackPositional(optInTimePeriod: number, _core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  return optInTimePeriod - 1;
}

/** @internal */
export function adrDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (adrLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function adrImpl(
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
  let periodTotal = 0;
  let tempReal = 0;
  let i = 0;
  let outIdx = 0;
  let trailingIdx = 0;
  let lookbackTotal = 0;
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
  /* This is ta_codegen/input/sma/sma.c with inReal[x] replaced by
   * (inHigh[x] - inLow[x]), and deliberately nothing else -- the same
   * running-sum order, the same read-before-write of the trailing term, and
   * the same divide by the period. Keeping the arithmetic identical is what
   * makes the composed reference in test_adr.c -- TA_SUB followed by TA_SMA
   * -- bit-exact rather than merely close.
   *
   * In particular the last statement divides; it does NOT multiply by a
   * precomputed 1/period, which would cost up to one ULP against TA_SMA and
   * silently downgrade that memcmp into a tolerance argument.
   *
   * Averaging the ranges is not the same computation as differencing two
   * averages: SMA(high) - SMA(low) is algebraically equal but subtracts two
   * price-magnitude means to reach a range-magnitude answer, so it inherits
   * the larger scale's rounding.
   */
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = (optInTimePeriod - 1);
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
  /* Do the MA calculation using tight loops. */
  /* Add-up the initial period, except for the last value. */
  periodTotal = 0.0;
  trailingIdx = startIdx - lookbackTotal;
  i = trailingIdx;
  if (optInTimePeriod > 1) {
    while (i < startIdx) {
      periodTotal += (inHigh[i] - inLow[i]);
      i = i + 1;
    }
  }
  /* Proceed with the calculation for the requested range.
   * Note that this algorithm allows outReal to be the same
   * buffer as either input.
   */
  outIdx = 0;
  while (i <= endIdx) {
    periodTotal += (inHigh[i] - inLow[i]);
    i = i + 1;
    tempReal = periodTotal;
    periodTotal -= (inHigh[trailingIdx] - inLow[trailingIdx]);
    trailingIdx = trailingIdx + 1;
    outReal[outIdx] = tempReal / optInTimePeriod;
    outIdx = outIdx + 1;
  }
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000']] as const;

/** @internal */
export function adrPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('ADR', 'startIdx', startIdx);
  endIdx = requireInt32('ADR', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('ADR', 'timePeriod', optInTimePeriod);
  requireIndexRange('ADR', startIdx, endIdx);
  const guardLookback = adrLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('ADR', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('ADR', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('ADR', 'prices.high', inHigh, guardInLen);
  requireLength('ADR', 'prices.low', inLow, guardInLen);
  requireOutput('ADR', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlapsPartly(outReal, inHighF64)) {
    throw sharedMemory('ADR', 'outReal', 'prices.high');
  }
  if (overlapsPartly(outReal, inLowF64)) {
    throw sharedMemory('ADR', 'outReal', 'prices.low');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = adrImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('ADR', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link adr}, {@link adrLookback}, {@link adrDisplayShift},
 * {@link adrOpen} and {@link adrOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface AdrOptions {
  /**
   * Number of bar ranges averaged. Published conventions differ and none is
   * authoritative: TradingView's ADR indicator page works its example over 7 bars,
   * TC2000's over 10, and the Qullamaggie screener community reads "ADR" as 20. The
   * value shipped here is ATR's, so the two volatility measures are comparable out
   * of the box (default 14; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the
   * default).
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

/** What {@link adr} returns, and {@link adrOpenAndFill} with its stream. */
export interface AdrResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Average bar range over the window, in price units. `values[k]` is the value at
   * input bar `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link adr} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AdrOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function adrLookback(options?: AdrOptions): number {
  const o = optionsOf('ADR lookback', options);
  return adrLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('ADR lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link adr}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AdrOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function adrDisplayShift(outputIdx: number, options?: AdrOptions): number {
  const o = optionsOf('ADR displayShift', options);
  return adrDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('ADR displayShift', o.core));
}

/**
 * Average Day Range: the arithmetic mean of the last `timePeriod` bar ranges, high
 * minus low. It answers how far price travels *within* a bar, and is read as a
 * volatility budget — a stop or a target much smaller than ADR is inside the noise
 * the instrument produces on an ordinary bar, one much larger asks for a move that
 * rarely happens. Same family as ATR, and deliberately the narrower member: the
 * range excludes the overnight gap, so on a gapping instrument ADR is
 * systematically smaller than ATR. Having both is the point.
 *
 * Formula and more info at
 * [ta-lib.org/functions/adr](https://ta-lib.org/functions/adr).
 *
 * **Notes**
 * - The mean of the ranges, not the difference of the means.
 *   `SMA(high) - SMA(low)` is algebraically the same quantity and is what both
 *   TradingView pages spell, but it subtracts two price-magnitude averages to
 *   reach a range-magnitude answer and inherits the larger scale's rounding;
 *   TC2000's `AVG(H-L, x)` and kand's `SMA(High-Low, period)` spell the form
 *   implemented here.
 * - The "day" is not a calendar day or a trading session. No TA-Lib function takes
 *   a timestamp or a session boundary, so the bars the caller passes *are* the
 *   days — pass daily bars for a daily range, hourly bars for an hourly one. This
 *   is the convention VWAP already ships under.
 * - `high` below `low` is not rejected. The library validates ranges and
 *   parameters, not price sanity, so a bar entered upside down contributes a
 *   negative range and the average simply comes out lower, possibly negative, with
 *   no error.
 * - Not the width of a Donchian channel. `MAX(high, n) - MIN(low, n)` is how far
 *   the window's extremes lie apart; ADR is the mean of the per-bar ranges, which
 *   is smaller whenever the window trends. `DONCHIAN` ships the two extremes that
 *   width is built from, not the width itself.
 * - The request this function answers (`TA-Lib/ta-lib-python#575`) named "Average
 *   Day Range" but the freqtrade code behind it computes
 *   `MAX(close, 24) - MIN(close, 24)`, a channel width on the closes with no
 *   averaging and no high/low. That is a different series and already reachable,
 *   as `sub(max(close, 24), min(close, 24))`.
 * - No percentage form is emitted. The two published ones disagree by more than
 *   20% on ordinary data — Qullamaggie's `100 · (SMA(high/low, 20) - 1)` averages
 *   ratios, TradingView's `(SMA(high, 14) - SMA(low, 14)) / close · 100` takes a
 *   ratio of averages — so picking one silently would ship a second indicator
 *   under this one's name.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link adrLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `high`: High price of
 *   each bar. `low`: Low price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link AdrOptions}.
 * @returns `begIdx` and the values; see {@link AdrResult}. Each output is a view
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
 * import { adr } from 'ta-lib';
 *
 * const high = new Float64Array(252); // your data, oldest first
 * const low = new Float64Array(252); // your data, oldest first
 *
 * const { begIdx, values } = adr({ high, low }, { timePeriod: 14 });
 * console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
 * ```
 * @see [`atr`](https://ta-lib.org/functions/atr)
 * @see [`donchian`](https://ta-lib.org/functions/donchian)
 * @see [`natr`](https://ta-lib.org/functions/natr)
 * @see [`qstick`](https://ta-lib.org/functions/qstick)
 * @see [`trange`](https://ta-lib.org/functions/trange)
 */
export function adr(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: AdrOptions): AdrResult {
  requireArgument('ADR', 'prices', prices);
  const o = optionsOf('ADR', options);
  if (o.endIdx === undefined) {
    requireSeries('ADR', 'prices.high', prices.high);
    requireSeries('ADR', 'prices.low', prices.low);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('ADR', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, adrLookbackPositional(optInTimePeriod, core), prices.high, prices.low);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = adrPositional(startIdx, endIdx, prices.high, prices.low, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live ADR stream: one value per closed bar, bit-identical to {@link adr} over
 * the same series. Open it with {@link adrOpen} or {@link adrOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/adr](https://ta-lib.org/functions/adr).
 */
export class AdrStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  periodTotal: number = 0;
  /** @internal */
  ringPos_trailingIdx: number = 0;
  /** @internal */
  ringCap_trailingIdx: number = 0;
  /** @internal */
  ring_trailingIdx_derived: Float64Array = new Float64Array(1);
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
  static of(core: Core): AdrStream {
    return new AdrStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link adr} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('ADR advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw streamFailure('ADR update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(high) || !Number.isFinite(low)) {
      throw nonFiniteBar('ADR update', !Number.isFinite(high) ? 'high' : 'low');
    }
    adrStepImpl(this, high, low);
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
      throw nonFiniteBar('ADR peek', !Number.isFinite(high) ? 'high' : 'low');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
    let tempReal = 0;
    let cur_outReal = 0;
    let periodTotal = sp.periodTotal;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    if (sp.ringCap_trailingIdx === 0) {
      pkSlot0 = 0;
      pkVal0 = (inHigh - inLow);
    }
    periodTotal += (inHigh - inLow);
    tempReal = periodTotal;
    periodTotal -= (sp.ringPos_trailingIdx !== pkSlot0) ? sp.ring_trailingIdx_derived[sp.ringPos_trailingIdx] : pkVal0;
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
  clone(): AdrStream {
    const c = new AdrStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.periodTotal = this.periodTotal;
    c.ringPos_trailingIdx = this.ringPos_trailingIdx;
    c.ringCap_trailingIdx = this.ringCap_trailingIdx;
    c.ring_trailingIdx_derived = this.ring_trailingIdx_derived.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function adrStepImpl(sp: AdrStream, inHigh: number, inLow: number): void {
  let tempReal = 0;
  let ringPosL_trailingIdx = 0;
  if (sp.ringCap_trailingIdx === 0) {
    sp.ring_trailingIdx_derived[0] = (inHigh - inLow);
  }
  sp.periodTotal += (inHigh - inLow);
  tempReal = sp.periodTotal;
  sp.periodTotal -= sp.ring_trailingIdx_derived[sp.ringPos_trailingIdx];
  sp.cur_outReal = tempReal / sp.optInTimePeriod;
  ringPosL_trailingIdx = sp.ringPos_trailingIdx;
  sp.ring_trailingIdx_derived[ringPosL_trailingIdx] = (inHigh - inLow);
  ringPosL_trailingIdx = ringPosL_trailingIdx + 1;
  ringPosL_trailingIdx = ringPosL_trailingIdx & (ringPosL_trailingIdx - sp.ringCap_trailingIdx >> 31);
  sp.ringPos_trailingIdx = ringPosL_trailingIdx;
}

function adrOpenImpl(
  sp: AdrStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let periodTotal = 0;
  let tempReal = 0;
  let i = 0;
  let outIdx = 0;
  let trailingIdx = 0;
  let lookbackTotal = 0;
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
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* This is ta_codegen/input/sma/sma.c with inReal[x] replaced by
   * (inHigh[x] - inLow[x]), and deliberately nothing else -- the same
   * running-sum order, the same read-before-write of the trailing term, and
   * the same divide by the period. Keeping the arithmetic identical is what
   * makes the composed reference in test_adr.c -- TA_SUB followed by TA_SMA
   * -- bit-exact rather than merely close.
   *
   * In particular the last statement divides; it does NOT multiply by a
   * precomputed 1/period, which would cost up to one ULP against TA_SMA and
   * silently downgrade that memcmp into a tolerance argument.
   *
   * Averaging the ranges is not the same computation as differencing two
   * averages: SMA(high) - SMA(low) is algebraically equal but subtracts two
   * price-magnitude means to reach a range-magnitude answer, so it inherits
   * the larger scale's rounding.
   */
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = (optInTimePeriod - 1);
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
  /* Do the MA calculation using tight loops. */
  /* Add-up the initial period, except for the last value. */
  periodTotal = 0.0;
  trailingIdx = startIdx - lookbackTotal;
  i = trailingIdx;
  if (optInTimePeriod > 1) {
    while (i < startIdx) {
      periodTotal += (inHigh[i] - inLow[i]);
      i = i + 1;
    }
  }
  /* Proceed with the calculation for the requested range.
   * Note that this algorithm allows outReal to be the same
   * buffer as either input.
   */
  outIdx = 0;
  while (i <= endIdx) {
    periodTotal += (inHigh[i] - inLow[i]);
    i = i + 1;
    tempReal = periodTotal;
    periodTotal -= (inHigh[trailingIdx] - inLow[trailingIdx]);
    trailingIdx = trailingIdx + 1;
    outReal[((outIdx * outStride) | 0)] = tempReal / optInTimePeriod;
    outIdx = outIdx + 1;
  }
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const cap_trailingIdx = i - trailingIdx;
  if (cap_trailingIdx < 0 || cap_trailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_trailingIdx = cap_trailingIdx > 0 ? cap_trailingIdx : 1;
  const capRing_trailingIdx_derived = new Float64Array(allocN_trailingIdx);
  for (let fillJ = historyLen - cap_trailingIdx; fillJ < historyLen; fillJ++) {
    capRing_trailingIdx_derived[fillJ - (historyLen - cap_trailingIdx)] = (inHigh[fillJ] - inLow[fillJ]);
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.periodTotal = periodTotal;
  sp.ringPos_trailingIdx = 0;
  sp.ringCap_trailingIdx = cap_trailingIdx;
  sp.ring_trailingIdx_derived = capRing_trailingIdx_derived;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function adrOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): AdrStream {
  const sp = AdrStream.of(core);
  const retCode = adrOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ADR openAndFill', inHigh.length, startIdx, adrLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && adrLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('ADR openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('ADR openAndFill', retCode);
}

/** @internal */
export function adrOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): AdrStream {
  const sp = AdrStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = adrOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ADR open', inHigh.length, startIdx, adrLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && adrLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('ADR open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('ADR open', retCode);
}

/** @internal */
export function adrOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): AdrStream {
  optInTimePeriod = requireInt32('ADR open', 'timePeriod', optInTimePeriod);
  requireArgument('ADR open', 'prices.high', inHigh);
  requireHistory('ADR open', 'prices.high', inHigh.length);
  requireArgument('ADR open', 'prices.low', inLow);
  requireHistoryLength('ADR open', 'prices.low', inLow.length, inHigh.length);
  return adrOpenInternal(core, float64(inHigh), float64(inLow), 0, optInTimePeriod);
}

/** @internal */
export function adrOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): AdrStream {
  optInTimePeriod = requireInt32('ADR openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('ADR openAndFill', 'prices.high', inHigh);
  requireHistory('ADR openAndFill', 'prices.high', inHigh.length);
  requireArgument('ADR openAndFill', 'prices.low', inLow);
  const guardLookback = adrLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('ADR openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('ADR openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('ADR openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireOutput('ADR openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('ADR openAndFill', 'outReal', 'prices.high');
  }
  if (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('ADR openAndFill', 'outReal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outReal, inHighF64)) {
    throw sharedMemory('ADR openAndFill', 'outReal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outReal, inLowF64)) {
    throw sharedMemory('ADR openAndFill', 'outReal', 'prices.low');
  }
  return adrOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live ADR stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link adr} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AdrOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `adrLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or
 *   histories of different lengths.
 *
 * @example
 * ```ts
 * import { adrOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = adrOpen({ high, low }, { timePeriod: 14 });
 * const peeked = stream.peek(101.4, 99.1); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1); // the bar closes
 * ```
 */
export function adrOpen(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: AdrOptions): AdrStream {
  requireArgument('ADR open', 'prices', prices);
  const o = optionsOf('ADR open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('ADR open', o.core);
  return adrOpenPositional(prices.high, prices.low, optInTimePeriod, core);
}

/**
 * {@link adrOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link adr}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link AdrOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link adr} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `adrLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { adrOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = adrOpenAndFill({ high, low }, {
 *   timePeriod: 14,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function adrOpenAndFill(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: AdrOptions): AdrResult & { stream: AdrStream } {
  requireArgument('ADR openAndFill', 'prices', prices);
  const o = optionsOf('ADR openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('ADR openAndFill', o.core);
  const capacity = fillCapacity(adrLookbackPositional(optInTimePeriod, core), prices.high, prices.low);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = adrOpenAndFillPositional(prices.high, prices.low, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
