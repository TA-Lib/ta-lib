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
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function smaLookbackPositional(optInTimePeriod: number, _core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  return optInTimePeriod - 1;
}

/** @internal */
export function smaDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (smaLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function smaImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
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
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
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
      periodTotal += inReal[i];
      i = i + 1;
    }
  }
  /* Proceed with the calculation for the requested range.
   * Note that this algorithm allows the inReal and
   * outReal to be the same buffer.
   */
  outIdx = 0;
  while (i <= endIdx) {
    periodTotal += inReal[i];
    i = i + 1;
    tempReal = periodTotal;
    periodTotal -= inReal[trailingIdx];
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
export function smaPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('SMA', 'startIdx', startIdx);
  endIdx = requireInt32('SMA', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('SMA', 'timePeriod', optInTimePeriod);
  requireIndexRange('SMA', startIdx, endIdx);
  const guardLookback = smaLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('SMA', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('SMA', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('SMA', 'real', inReal, guardInLen);
  requireOutput('SMA', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('SMA', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = smaImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('SMA', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link sma}, {@link smaLookback}, {@link smaDisplayShift},
 * {@link smaOpen} and {@link smaOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface SmaOptions {
  /**
   * Number of bars in the averaging window (default 30; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
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

/** What {@link sma} returns, and {@link smaOpenAndFill} with its stream. */
export interface SmaResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Simple moving average of the input. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link sma} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SmaOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function smaLookback(options?: SmaOptions): number {
  const o = optionsOf('SMA lookback', options);
  return smaLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('SMA lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link sma}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SmaOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function smaDisplayShift(outputIdx: number, options?: SmaOptions): number {
  const o = optionsOf('SMA displayShift', options);
  return smaDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('SMA displayShift', o.core));
}

/**
 * Simple Moving Average: the unweighted arithmetic mean of the last N input
 * values. Used to smooth a series.
 *
 * Formula and more info at
 * [ta-lib.org/functions/sma](https://ta-lib.org/functions/sma).
 *
 * **Notes**
 * - A period of 1 performs no smoothing: the output is a copy of the input.
 *   Allowed since 0.6.5 (issues #48/#59).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link smaLookback} is a success with empty outputs.
 *
 * @param real - Source series to average.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link SmaOptions}.
 * @returns `begIdx` and the values; see {@link SmaResult}. Each output is a view
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
 * import { sma } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = sma(data, { timePeriod: 30 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`wma`](https://ta-lib.org/functions/wma)
 * @see [`ma`](https://ta-lib.org/functions/ma)
 * @see [`dema`](https://ta-lib.org/functions/dema)
 * @see [`tema`](https://ta-lib.org/functions/tema)
 */
export function sma(real: ArrayLike<number>, options?: SmaOptions): SmaResult {
  const o = optionsOf('SMA', options);
  if (o.endIdx === undefined) {
    requireSeries('SMA', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('SMA', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, smaLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = smaPositional(startIdx, endIdx, real, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live SMA stream: one value per closed bar, bit-identical to {@link sma} over
 * the same series. Open it with {@link smaOpen} or {@link smaOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/sma](https://ta-lib.org/functions/sma).
 */
export class SmaStream {
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
  ring_trailingIdx_inReal: Float64Array = new Float64Array(1);
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
  static of(core: Core): SmaStream {
    return new SmaStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link sma} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('SMA advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: Source series to average.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('SMA update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('SMA update', 'real');
    }
    smaStepImpl(this, real);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: Source series to average.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number): number {
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('SMA peek', 'real');
    }
    const sp = this;
    const inReal = real;
    let tempReal = 0;
    let cur_outReal = 0;
    let periodTotal = sp.periodTotal;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    if (sp.ringCap_trailingIdx === 0) {
      pkSlot0 = 0;
      pkVal0 = inReal;
    }
    periodTotal += inReal;
    tempReal = periodTotal;
    periodTotal -= ((sp.ringPos_trailingIdx !== pkSlot0) ? sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] : pkVal0);
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
  clone(): SmaStream {
    const c = new SmaStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.periodTotal = this.periodTotal;
    c.ringPos_trailingIdx = this.ringPos_trailingIdx;
    c.ringCap_trailingIdx = this.ringCap_trailingIdx;
    c.ring_trailingIdx_inReal = this.ring_trailingIdx_inReal.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function smaStepImpl(sp: SmaStream, inReal: number): void {
  let tempReal = 0;
  if (sp.ringCap_trailingIdx === 0) {
    sp.ring_trailingIdx_inReal[0] = inReal;
  }
  sp.periodTotal += inReal;
  tempReal = sp.periodTotal;
  sp.periodTotal -= sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
  sp.cur_outReal = tempReal / sp.optInTimePeriod;
  sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
  sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
  if (sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx) {
    sp.ringPos_trailingIdx = 0;
  }
}

function smaOpenImpl(
  sp: SmaStream,
  inReal: Float64Array,
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
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
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
      periodTotal += inReal[i];
      i = i + 1;
    }
  }
  /* Proceed with the calculation for the requested range.
   * Note that this algorithm allows the inReal and
   * outReal to be the same buffer.
   */
  outIdx = 0;
  while (i <= endIdx) {
    periodTotal += inReal[i];
    i = i + 1;
    tempReal = periodTotal;
    periodTotal -= inReal[trailingIdx];
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
  const capRing_trailingIdx_inReal = new Float64Array(allocN_trailingIdx);
  capRing_trailingIdx_inReal.set(inReal.subarray(historyLen - cap_trailingIdx, historyLen), 0);
  sp.optInTimePeriod = optInTimePeriod;
  sp.periodTotal = periodTotal;
  sp.ringPos_trailingIdx = 0;
  sp.ringCap_trailingIdx = cap_trailingIdx;
  sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function smaOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): SmaStream {
  const sp = SmaStream.of(core);
  const retCode = smaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SMA openAndFill', inReal.length, startIdx, smaLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && smaLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('SMA openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('SMA openAndFill', retCode);
}

/** @internal */
export function smaOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): SmaStream {
  const sp = SmaStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = smaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SMA open', inReal.length, startIdx, smaLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && smaLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('SMA open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('SMA open', retCode);
}

/** @internal */
export function smaOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): SmaStream {
  optInTimePeriod = requireInt32('SMA open', 'timePeriod', optInTimePeriod);
  requireArgument('SMA open', 'real', inReal);
  requireHistory('SMA open', 'real', inReal.length);
  return smaOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/** @internal */
export function smaOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): SmaStream {
  optInTimePeriod = requireInt32('SMA openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('SMA openAndFill', 'real', inReal);
  requireHistory('SMA openAndFill', 'real', inReal.length);
  const guardLookback = smaLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('SMA openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('SMA openAndFill', inReal.length, guardLookback);
  requireOutput('SMA openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('SMA openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('SMA openAndFill', 'outReal', 'real');
  }
  return smaOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live SMA stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link sma} there.
 *
 * @param real - Warm-up history. Source series to average.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SmaOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `smaLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { smaOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = smaOpen(data, { timePeriod: 30 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function smaOpen(real: ArrayLike<number>, options?: SmaOptions): SmaStream {
  const o = optionsOf('SMA open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('SMA open', o.core);
  return smaOpenPositional(real, optInTimePeriod, core);
}

/**
 * {@link smaOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link sma}.
 *
 * @param real - Warm-up history. Source series to average.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link SmaOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link sma} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `smaLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or a
 *   buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { smaOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = smaOpenAndFill(data, { timePeriod: 30 });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function smaOpenAndFill(real: ArrayLike<number>, options?: SmaOptions): SmaResult & { stream: SmaStream } {
  const o = optionsOf('SMA openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('SMA openAndFill', o.core);
  const capacity = fillCapacity(smaLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = smaOpenAndFillPositional(real, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/** @internal */
export function smaStepTape(sp: SmaStream, tape: Float64Array, tapeBase: number, tapeMask: number, inReal: number): number {
  let tempReal = 0;
  sp.periodTotal += inReal;
  tempReal = sp.periodTotal;
  sp.periodTotal -= tape[(tapeBase - sp.ringCap_trailingIdx) & tapeMask];
  sp.cur_outReal = tempReal / sp.optInTimePeriod;
  sp.outRangeCount++;
  return sp.cur_outReal;
}

/** @internal */
export function smaPeekTape(sp: SmaStream, tape: Float64Array, tapeBase: number, tapeMask: number, inReal: number): number {
  let tempReal = 0;
  let cur_outReal = 0;
  let periodTotal = sp.periodTotal;
  let pkSlot0 = -1;
  let pkVal0 = 0;
  pkSlot0 = tapeBase & tapeMask;
  pkVal0 = inReal;
  periodTotal += inReal;
  tempReal = periodTotal;
  periodTotal -= ((((tapeBase - sp.ringCap_trailingIdx) & tapeMask) !== pkSlot0) ? tape[(tapeBase - sp.ringCap_trailingIdx) & tapeMask] : pkVal0);
  cur_outReal = tempReal / sp.optInTimePeriod;
  return cur_outReal;
}

/** @internal */
export function smaTapeDetach(sp: SmaStream): number {
  let reach = 0;
  sp.ring_trailingIdx_inReal = new Float64Array(0);
  if (sp.ringCap_trailingIdx > reach) {
    reach = sp.ringCap_trailingIdx;
  }
  return reach;
}
