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
 *  112126 MF,CC  First version (issue #610).
 */

import { MAType, MATypes } from '../enums.ts';
import { NO_OPTIONS, badSink, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, isPlainOptions, nonFiniteBar, notPlainOptions, openFillCount, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireReal, requireSeries, sharedMemory, streamFailure, unknownOption } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { MaStream, maLookbackPositional, maOpenAndFillInternal, maPositional } from './ma.ts';

/** @internal */
export function maenvLookbackPositional(optInTimePeriod: number, optInPercentUp: number, optInPercentDn: number, optInMAType: MAType | typeof Core.INTEGER_DEFAULT, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (typeof optInPercentUp !== 'number') {
    return -1;
  }
  if (typeof optInPercentDn !== 'number') {
    return -1;
  }
  if ((optInMAType | 0) !== optInMAType) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  if (optInPercentUp === Core.REAL_DEFAULT) {
    optInPercentUp = 10.0;
  } else if (!(optInPercentUp >= Core.REAL_MIN && optInPercentUp <= Core.REAL_MAX)) {
    return -1;
  }
  if (optInPercentDn === Core.REAL_DEFAULT) {
    optInPercentDn = 10.0;
  } else if (!(optInPercentDn >= Core.REAL_MIN && optInPercentDn <= Core.REAL_MAX)) {
    return -1;
  }
  if (optInMAType === Core.INTEGER_DEFAULT || optInMAType === MAType.DEFAULT) {
    optInMAType = MAType.SMA;
  } else if (optInMAType < MATypes.Min || optInMAType > MATypes.Max) {
    return -1;
  }
  return maLookbackPositional(optInTimePeriod, (optInMAType as MAType), core);
}

/** @internal */
export function maenvDisplayShiftPositional(optInTimePeriod: number, optInPercentUp: number, optInPercentDn: number, optInMAType: MAType | typeof Core.INTEGER_DEFAULT, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (maenvLookbackPositional(optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 3) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function maenvImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  optInPercentUp: number,
  optInPercentDn: number,
  optInMAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
): RetCode {
  let i = 0;
  let upFactor = 0;
  let dnFactor = 0;
  let middle = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInPercentUp === Core.REAL_DEFAULT) {
    optInPercentUp = 10.0;
  } else if (!(optInPercentUp >= Core.REAL_MIN && optInPercentUp <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInPercentDn === Core.REAL_DEFAULT) {
    optInPercentDn = 10.0;
  } else if (!(optInPercentDn >= Core.REAL_MIN && optInPercentDn <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInMAType === Core.INTEGER_DEFAULT || optInMAType === MAType.DEFAULT) {
    optInMAType = MAType.SMA;
  } else if (optInMAType < MATypes.Min || optInMAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if ((outRealUpperBand.buffer === outRealMiddleBand.buffer && outRealUpperBand.byteOffset === outRealMiddleBand.byteOffset) || (outRealUpperBand.buffer === outRealLowerBand.buffer && outRealUpperBand.byteOffset === outRealLowerBand.byteOffset) || (outRealMiddleBand.buffer === outRealLowerBand.buffer && outRealMiddleBand.byteOffset === outRealLowerBand.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  if (maLookbackPositional(optInTimePeriod, (optInMAType as MAType), core) > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  const _xr0 = maPositional(startIdx, endIdx, inReal, optInTimePeriod, (optInMAType as MAType), outRealMiddleBand, core);
  outBegIdx.value = _xr0.begIdx;
  outNBElement.value = _xr0.count;
  upFactor = 1.0 + optInPercentUp / 100.0;
  dnFactor = 1.0 - optInPercentDn / 100.0;
  for (i = 0; i < outNBElement.value; i += 1) {
    middle = outRealMiddleBand[i];
    outRealUpperBand[i] = middle * upFactor;
    outRealLowerBand[i] = middle * dnFactor;
  }
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['percentUp', Core.REAL_DEFAULT, Core.REAL_MIN, Core.REAL_MAX, 'in -3e37..3e37'], ['percentDn', Core.REAL_DEFAULT, Core.REAL_MIN, Core.REAL_MAX, 'in -3e37..3e37'], ['maType', Core.INTEGER_DEFAULT, MATypes.Min, MATypes.Max, 'a MAType']] as const;

/** @internal */
export function maenvPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInPercentUp: number,
  optInPercentDn: number,
  optInMAType: MAType | typeof Core.INTEGER_DEFAULT,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('MAENV', 'startIdx', startIdx);
  endIdx = requireInt32('MAENV', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('MAENV', 'timePeriod', optInTimePeriod);
  optInPercentUp = requireReal('MAENV', 'percentUp', optInPercentUp);
  optInPercentDn = requireReal('MAENV', 'percentDn', optInPercentDn);
  optInMAType = requireInt32('MAENV', 'maType', optInMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireIndexRange('MAENV', startIdx, endIdx);
  const guardLookback = maenvLookbackPositional(optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType, core);
  if (guardLookback < 0) {
    throw rejectedParams('MAENV', PARAM_RANGES, optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType);
  }
  const guardStart = clampedStart('MAENV', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('MAENV', 'real', inReal, guardInLen);
  requireOutput('MAENV', 'outUpperBand', outRealUpperBand, guardOutLen, Float64Array);
  requireOutput('MAENV', 'outMiddleBand', outRealMiddleBand, guardOutLen, Float64Array);
  requireOutput('MAENV', 'outLowerBand', outRealLowerBand, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outRealUpperBand, inRealF64)) {
    throw sharedMemory('MAENV', 'outUpperBand', 'real');
  }
  if (overlapsPartly(outRealMiddleBand, inRealF64)) {
    throw sharedMemory('MAENV', 'outMiddleBand', 'real');
  }
  if (overlapsPartly(outRealLowerBand, inRealF64)) {
    throw sharedMemory('MAENV', 'outLowerBand', 'real');
  }
  if (overlaps(outRealUpperBand, outRealMiddleBand)) {
    throw sharedMemory('MAENV', 'outUpperBand', 'outMiddleBand');
  }
  if (overlaps(outRealUpperBand, outRealLowerBand)) {
    throw sharedMemory('MAENV', 'outUpperBand', 'outLowerBand');
  }
  if (overlaps(outRealMiddleBand, outRealLowerBand)) {
    throw sharedMemory('MAENV', 'outMiddleBand', 'outLowerBand');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = maenvImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('MAENV', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link maenv}, {@link maenvLookback}, {@link maenvDisplayShift},
 * {@link maenvOpen} and {@link maenvOpenAndFill}. Every key is optional: an
 * omitted or `undefined` key takes its default, and a call ignores the keys it has
 * no use for. A key that is not one of these is `TA_BAD_PARAM`.
 */
export interface MaenvOptions {
  /**
   * Periods of the moving average (default 20; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  timePeriod?: number | undefined;
  /**
   * Percent of the moving average added to form the upper band (default 10;
   * `Core.REAL_DEFAULT` selects the default).
   */
  percentUp?: number | undefined;
  /**
   * Percent of the moving average subtracted to form the lower band (default 10;
   * `Core.REAL_DEFAULT` selects the default).
   */
  percentDn?: number | undefined;
  /**
   * Moving-average type for the middle band (default `MAType.SMA`; values: 0=SMA,
   * 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA, 10=DISABLED,
   * 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA, 16=SSF; `Core.INTEGER_DEFAULT`
   * or `MAType.DEFAULT` selects the default).
   */
  maType?: MAType | typeof Core.INTEGER_DEFAULT | undefined;
  /**
   * First bar of the requested range (default 0). The lookback's bars before it are
   * still read. Batch calls only.
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
   * Buffer for `outUpperBand`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outUpperBand?: Float64Array<ArrayBuffer> | undefined;
  /**
   * Buffer for `outMiddleBand`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outMiddleBand?: Float64Array<ArrayBuffer> | undefined;
  /**
   * Buffer for `outLowerBand`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outLowerBand?: Float64Array<ArrayBuffer> | undefined;
}

/** @internal */
export function maenvOptionsOf(funcName: string, options: MaenvOptions | undefined): MaenvOptions {
  if (options === undefined) {
    return NO_OPTIONS;
  }
  if (!isPlainOptions(options)) {
    throw notPlainOptions(funcName, options);
  }
  for (const k in options) {
    if (k !== 'timePeriod' && k !== 'percentUp' && k !== 'percentDn' && k !== 'maType' && k !== 'startIdx' && k !== 'endIdx' && k !== 'core' && k !== 'outUpperBand' && k !== 'outMiddleBand' && k !== 'outLowerBand') {
      unknownOption(funcName, options, k, 'timePeriod percentUp percentDn maType startIdx endIdx core outUpperBand outMiddleBand outLowerBand');
    }
  }
  return options;
}

/** What {@link maenv} returns, and {@link maenvOpenAndFill} with its stream. */
export interface MaenvResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * The moving average plus the upper percentage of it. `outUpperBand[k]` is the
   * value at input bar `begIdx + k`.
   */
  outUpperBand: Float64Array<ArrayBuffer>;
  /** The moving average. `outMiddleBand[k]` is the value at input bar `begIdx + k`. */
  outMiddleBand: Float64Array<ArrayBuffer>;
  /**
   * The moving average minus the lower percentage of it. `outLowerBand[k]` is the
   * value at input bar `begIdx + k`.
   */
  outLowerBand: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link maenv} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; the function's other option keys are
 *   ignored. See {@link MaenvOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` for options that are not a plain
 *   object, an option key the function does not have or a `core` that is not a
 *   `Core`.
 */
export function maenvLookback(options?: MaenvOptions): number {
  const o = maenvOptionsOf('MAENV lookback', options);
  return maenvLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.percentUp, Core.REAL_DEFAULT), orDefault(o.percentDn, Core.REAL_DEFAULT), orDefault(o.maType, Core.INTEGER_DEFAULT), coreOf('MAENV lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link maenv}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; the function's other option keys are
 *   ignored. See {@link MaenvOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` for options that are not a plain
 *   object, an option key the function does not have or a `core` that is not a
 *   `Core`.
 */
export function maenvDisplayShift(outputIdx: number, options?: MaenvOptions): number {
  const o = maenvOptionsOf('MAENV displayShift', options);
  return maenvDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.percentUp, Core.REAL_DEFAULT), orDefault(o.percentDn, Core.REAL_DEFAULT), orDefault(o.maType, Core.INTEGER_DEFAULT), outputIdx, coreOf('MAENV displayShift', o.core));
}

/**
 * Moving Average Envelope: a moving average with one band a fixed percentage above
 * it and one a fixed percentage below. The bands keep a constant proportional
 * distance from the average, so price reaching a band reads as stretched relative
 * to its recent mean, and a move beyond one as unusual strength or weakness.
 *
 * Formula and more info at
 * [ta-lib.org/functions/maenv](https://ta-lib.org/functions/maenv).
 *
 * **Notes**
 * - The percentages are in percent: 10, not 0.1, places a band 10% from the moving
 *   average.
 * - Each band has its own percentage; the usual single-percentage envelope is the
 *   case where both are equal. A negative percentage puts its band on the other
 *   side of the average.
 * - **The input is assumed positive.** Each band sits a percent of the moving
 *   average away from it. Where the average is 0 both bands equal it, and where it
 *   is negative the two bands trade places; no error is returned. A series that
 *   reaches or crosses zero, such as an oscillator or a spread, has no meaningful
 *   result here.
 * - A displaced envelope is a shift of the plot by a number of bars. The outputs
 *   are not displaced.
 *
 * `outUpperBand[k]` is the value at input bar `begIdx + k`; every output is
 * exactly as long as the number of values written, and nothing is padded. A range
 * that ends before {@link maenvLookback} is a success with empty outputs.
 *
 * @param real - Input data series.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link MaenvOptions}.
 * @returns `begIdx` and the values; see {@link MaenvResult}. Each output is a view
 *   of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not a plain object, an
 *   option key the function does not have, a `core` that is not a `Core`, an absent
 *   or empty input, a range bound that is not an int32, an integer parameter that is
 *   not an int32, a real parameter that is not a number, a parameter outside its
 *   range, an input too short for the range, a buffer that is empty, too short or of
 *   the wrong type, two buffers sharing memory or a buffer sharing memory with a
 *   `Float64Array` input it is not. Checked before anything is written.
 *
 * @example
 * ```ts
 * import { MAType, maenv } from 'ta-lib';
 *
 * const series = new Float64Array(252); // your data, oldest first
 *
 * const {
 *   begIdx,
 *   outUpperBand,
 *   outMiddleBand,
 *   outLowerBand,
 * } = maenv(series, {
 *   timePeriod: 20,
 *   percentUp: 10,
 *   percentDn: 10,
 *   maType: MAType.SMA,
 * });
 * console.log(outUpperBand.length + ' values from bar ' + begIdx + ':');
 * console.log('outUpperBand = [' + outUpperBand[0].toFixed(4) + ', ..., ' + outUpperBand[outUpperBand.length - 1].toFixed(4) + ']');
 * console.log('outMiddleBand = [' + outMiddleBand[0].toFixed(4) + ', ..., ' + outMiddleBand[outMiddleBand.length - 1].toFixed(4) + ']');
 * console.log('outLowerBand = [' + outLowerBand[0].toFixed(4) + ', ..., ' + outLowerBand[outLowerBand.length - 1].toFixed(4) + ']');
 * ```
 * @see [`ma`](https://ta-lib.org/functions/ma)
 * @see [`bbands`](https://ta-lib.org/functions/bbands)
 * @see [`kc`](https://ta-lib.org/functions/kc)
 * @see [`accbands`](https://ta-lib.org/functions/accbands)
 */
export function maenv(real: ArrayLike<number>, options?: MaenvOptions): MaenvResult {
  const o = maenvOptionsOf('MAENV', options);
  if (o.endIdx === undefined) {
    requireSeries('MAENV', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInPercentUp = orDefault(o.percentUp, Core.REAL_DEFAULT);
  const optInPercentDn = orDefault(o.percentDn, Core.REAL_DEFAULT);
  const optInMAType = orDefault(o.maType, Core.INTEGER_DEFAULT);
  const core = coreOf('MAENV', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, maenvLookbackPositional(optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType, core), real);
  const outRealUpperBand = o.outUpperBand === undefined ? new Float64Array(capacity) : o.outUpperBand;
  const outRealMiddleBand = o.outMiddleBand === undefined ? new Float64Array(capacity) : o.outMiddleBand;
  const outRealLowerBand = o.outLowerBand === undefined ? new Float64Array(capacity) : o.outLowerBand;
  const range = maenvPositional(startIdx, endIdx, real, optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType, outRealUpperBand, outRealMiddleBand, outRealLowerBand, core);
  return { begIdx: range.begIdx, outUpperBand: outRealUpperBand.subarray(0, range.count), outMiddleBand: outRealMiddleBand.subarray(0, range.count), outLowerBand: outRealLowerBand.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live MAENV stream: one value per closed bar, bit-identical to {@link maenv}
 * over the same series. Open it with {@link maenvOpen} or
 * {@link maenvOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/maenv](https://ta-lib.org/functions/maenv).
 */
export class MaenvStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  optInPercentUp: number = 0;
  /** @internal */
  optInPercentDn: number = 0;
  /** @internal */
  optInMAType: number = 0;
  /** @internal */
  cur_outRealUpperBand: number = 0;
  /** @internal */
  cur_outRealMiddleBand: number = 0;
  /** @internal */
  cur_outRealLowerBand: number = 0;
  /** @internal */
  dnFactor: number = 0;
  /** @internal */
  upFactor: number = 0;
  /** @internal */
  sub0!: MaStream;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): MaenvStream {
    return new MaenvStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link maenv} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('MAENV advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns its outputs.
   *
   * @param real - The bar's `real` value: Input data series.
   * @param out - Receives the outputs and is returned; a new {@link MaenvOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included, before anything moves.
   *   `TALibError` with code `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached
   *   bar `Core.INDEX_MAX`.
   */
  update(real: number, out?: MaenvOut): MaenvOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('MAENV update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('MAENV update', 'MaenvOut');
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('MAENV update', 'real');
    }
    maenvStepImpl(this, real);
    this.outRangeCount++;
    if (out === undefined) {
      out = new MaenvOut();
    }
    out.outUpperBand = this.cur_outRealUpperBand;
    out.outMiddleBand = this.cur_outRealMiddleBand;
    out.outLowerBand = this.cur_outRealLowerBand;
    return out;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: Input data series.
   * @param out - Receives the outputs and is returned; a new {@link MaenvOut} when
   *   omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included.
   */
  peek(real: number, out?: MaenvOut): MaenvOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('MAENV peek', 'MaenvOut');
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('MAENV peek', 'real');
    }
    const sp = this;
    const inReal = real;
    let middle = 0;
    let cur_outRealMiddleBand = 0;
    let cur_outRealUpperBand = 0;
    let cur_outRealLowerBand = 0;
    cur_outRealMiddleBand = sp.sub0.peek(inReal);
    let dnFactor = sp.dnFactor;
    let upFactor = sp.upFactor;
    middle = cur_outRealMiddleBand;
    cur_outRealUpperBand = middle * upFactor;
    cur_outRealLowerBand = middle * dnFactor;
    if (out === undefined) {
      out = new MaenvOut();
    }
    out.outUpperBand = cur_outRealUpperBand;
    out.outMiddleBand = cur_outRealMiddleBand;
    out.outLowerBand = cur_outRealLowerBand;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link MaenvOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a sink that is not an object,
   *   `null` included.
   */
  value(out?: MaenvOut): MaenvOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('MAENV value', 'MaenvOut');
    }
    if (out === undefined) {
      out = new MaenvOut();
    }
    out.outUpperBand = this.cur_outRealUpperBand;
    out.outMiddleBand = this.cur_outRealMiddleBand;
    out.outLowerBand = this.cur_outRealLowerBand;
    return out;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): MaenvStream {
    const c = new MaenvStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.optInPercentUp = this.optInPercentUp;
    c.optInPercentDn = this.optInPercentDn;
    c.optInMAType = this.optInMAType;
    c.cur_outRealUpperBand = this.cur_outRealUpperBand;
    c.cur_outRealMiddleBand = this.cur_outRealMiddleBand;
    c.cur_outRealLowerBand = this.cur_outRealLowerBand;
    c.dnFactor = this.dnFactor;
    c.upFactor = this.upFactor;
    c.sub0 = this.sub0.clone();
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one MAENV bar, as `update`, `peek` and `value` return them.
 * Passed back to one of them, it is filled and returned again, so no result object
 * is created; it holds its values until the next call that writes it. A new one
 * holds `NaN`.
 */
export class MaenvOut {
  /** The moving average plus the upper percentage of it. */
  outUpperBand: number = NaN;
  /** The moving average. */
  outMiddleBand: number = NaN;
  /** The moving average minus the lower percentage of it. */
  outLowerBand: number = NaN;
}

function maenvStepImpl(sp: MaenvStream, inReal: number): void {
  let middle = 0;
  let cur_outRealMiddleBand = 0;
  let cur_outRealUpperBand = 0;
  let cur_outRealLowerBand = 0;
  cur_outRealMiddleBand = sp.sub0.update(inReal);
  let dnFactor = sp.dnFactor;
  let upFactor = sp.upFactor;
  middle = cur_outRealMiddleBand;
  cur_outRealUpperBand = middle * upFactor;
  cur_outRealLowerBand = middle * dnFactor;
  sp.cur_outRealUpperBand = cur_outRealUpperBand;
  sp.cur_outRealMiddleBand = cur_outRealMiddleBand;
  sp.cur_outRealLowerBand = cur_outRealLowerBand;
}

function maenvOpenImpl(
  sp: MaenvStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInPercentUp: number,
  optInPercentDn: number,
  optInMAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let i = 0;
  let upFactor = 0;
  let dnFactor = 0;
  let middle = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInPercentUp === Core.REAL_DEFAULT) {
    optInPercentUp = 10.0;
  } else if (!(optInPercentUp >= Core.REAL_MIN && optInPercentUp <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInPercentDn === Core.REAL_DEFAULT) {
    optInPercentDn = 10.0;
  } else if (!(optInPercentDn >= Core.REAL_MIN && optInPercentDn <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInMAType === Core.INTEGER_DEFAULT || optInMAType === MAType.DEFAULT) {
    optInMAType = MAType.SMA;
  } else if (optInMAType < MATypes.Min || optInMAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen < maenvLookbackPositional(optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType as MAType, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const sc_outRealUpperBand = outStride === 1 ? outRealUpperBand : new Float64Array(historyLen);
  const sc_outRealMiddleBand = outStride === 1 ? outRealMiddleBand : new Float64Array(historyLen);
  const sc_outRealLowerBand = outStride === 1 ? outRealLowerBand : new Float64Array(historyLen);
  if (maLookbackPositional(optInTimePeriod, (optInMAType as MAType), core) > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const sub0 = maOpenAndFillInternal(core, inReal, startIdx, optInTimePeriod, (optInMAType as MAType), outBegIdx, outNBElement, sc_outRealMiddleBand);
  upFactor = 1.0 + optInPercentUp / 100.0;
  dnFactor = 1.0 - optInPercentDn / 100.0;
  for (i = 0; i < outNBElement.value; i += 1) {
    middle = sc_outRealMiddleBand[i];
    sc_outRealUpperBand[i] = middle * upFactor;
    sc_outRealLowerBand[i] = middle * dnFactor;
  }
  if (outNBElement.value < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.optInPercentUp = optInPercentUp;
  sp.optInPercentDn = optInPercentDn;
  sp.optInMAType = optInMAType;
  sp.sub0 = sub0;
  sp.dnFactor = dnFactor;
  sp.upFactor = upFactor;
  sp.cur_outRealUpperBand = sc_outRealUpperBand[outNBElement.value - 1];
  sp.cur_outRealMiddleBand = sc_outRealMiddleBand[outNBElement.value - 1];
  sp.cur_outRealLowerBand = sc_outRealLowerBand[outNBElement.value - 1];
  return RetCode.SUCCESS;
}

/** @internal */
export function maenvOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInPercentUp: number,
  optInPercentDn: number,
  optInMAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
): MaenvStream {
  const sp = MaenvStream.of(core);
  const retCode = maenvOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MAENV openAndFill', inReal.length, startIdx, maenvLookbackPositional(optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType as MAType, core));
  }
  if (retCode === RetCode.BAD_PARAM && maenvLookbackPositional(optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType as MAType, core) < 0) {
    throw rejectedParams('MAENV openAndFill', PARAM_RANGES, optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType);
  }
  throw streamFailure('MAENV openAndFill', retCode);
}

/** @internal */
export function maenvOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInPercentUp: number,
  optInPercentDn: number,
  optInMAType: number,
): MaenvStream {
  const sp = MaenvStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outRealUpperBand = new Float64Array(1);
  const sink_outRealMiddleBand = new Float64Array(1);
  const sink_outRealLowerBand = new Float64Array(1);
  const retCode = maenvOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType, outBegIdx, outNBElement, sink_outRealUpperBand, sink_outRealMiddleBand, sink_outRealLowerBand, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MAENV open', inReal.length, startIdx, maenvLookbackPositional(optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType as MAType, core));
  }
  if (retCode === RetCode.BAD_PARAM && maenvLookbackPositional(optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType as MAType, core) < 0) {
    throw rejectedParams('MAENV open', PARAM_RANGES, optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType);
  }
  throw streamFailure('MAENV open', retCode);
}

/** @internal */
export function maenvOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInPercentUp: number,
  optInPercentDn: number,
  optInMAType: MAType | typeof Core.INTEGER_DEFAULT,
  core: Core = Core.DEFAULT,
): MaenvStream {
  optInTimePeriod = requireInt32('MAENV open', 'timePeriod', optInTimePeriod);
  optInPercentUp = requireReal('MAENV open', 'percentUp', optInPercentUp);
  optInPercentDn = requireReal('MAENV open', 'percentDn', optInPercentDn);
  optInMAType = requireInt32('MAENV open', 'maType', optInMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('MAENV open', 'real', inReal);
  requireHistory('MAENV open', 'real', inReal.length);
  return maenvOpenInternal(core, float64(inReal), 0, optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType);
}

/** @internal */
export function maenvOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInPercentUp: number,
  optInPercentDn: number,
  optInMAType: MAType | typeof Core.INTEGER_DEFAULT,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
  core: Core = Core.DEFAULT,
): MaenvStream {
  optInTimePeriod = requireInt32('MAENV openAndFill', 'timePeriod', optInTimePeriod);
  optInPercentUp = requireReal('MAENV openAndFill', 'percentUp', optInPercentUp);
  optInPercentDn = requireReal('MAENV openAndFill', 'percentDn', optInPercentDn);
  optInMAType = requireInt32('MAENV openAndFill', 'maType', optInMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('MAENV openAndFill', 'real', inReal);
  requireHistory('MAENV openAndFill', 'real', inReal.length);
  const guardLookback = maenvLookbackPositional(optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType as MAType, core);
  if (guardLookback < 0) {
    throw rejectedParams('MAENV openAndFill', PARAM_RANGES, optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType);
  }
  const guardOutLen = openFillCount('MAENV openAndFill', inReal.length, guardLookback);
  requireOutput('MAENV openAndFill', 'outUpperBand', outRealUpperBand, guardOutLen, Float64Array);
  requireOutput('MAENV openAndFill', 'outMiddleBand', outRealMiddleBand, guardOutLen, Float64Array);
  requireOutput('MAENV openAndFill', 'outLowerBand', outRealLowerBand, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outRealUpperBand.buffer === inRealF64.buffer && outRealUpperBand.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('MAENV openAndFill', 'outUpperBand', 'real');
  }
  if (outRealMiddleBand.buffer === inRealF64.buffer && outRealMiddleBand.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('MAENV openAndFill', 'outMiddleBand', 'real');
  }
  if (outRealLowerBand.buffer === inRealF64.buffer && outRealLowerBand.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('MAENV openAndFill', 'outLowerBand', 'real');
  }
  if (outRealUpperBand.buffer === outRealMiddleBand.buffer && outRealUpperBand.byteOffset === outRealMiddleBand.byteOffset) {
    throw sharedMemory('MAENV openAndFill', 'outUpperBand', 'outMiddleBand');
  }
  if (outRealUpperBand.buffer === outRealLowerBand.buffer && outRealUpperBand.byteOffset === outRealLowerBand.byteOffset) {
    throw sharedMemory('MAENV openAndFill', 'outUpperBand', 'outLowerBand');
  }
  if (outRealMiddleBand.buffer === outRealLowerBand.buffer && outRealMiddleBand.byteOffset === outRealLowerBand.byteOffset) {
    throw sharedMemory('MAENV openAndFill', 'outMiddleBand', 'outLowerBand');
  }
  if (guardOutLen > 0 && overlaps(outRealUpperBand, inRealF64)) {
    throw sharedMemory('MAENV openAndFill', 'outUpperBand', 'real');
  }
  if (guardOutLen > 0 && overlaps(outRealMiddleBand, inRealF64)) {
    throw sharedMemory('MAENV openAndFill', 'outMiddleBand', 'real');
  }
  if (guardOutLen > 0 && overlaps(outRealLowerBand, inRealF64)) {
    throw sharedMemory('MAENV openAndFill', 'outLowerBand', 'real');
  }
  if (guardOutLen > 0 && overlaps(outRealUpperBand, outRealMiddleBand)) {
    throw sharedMemory('MAENV openAndFill', 'outUpperBand', 'outMiddleBand');
  }
  if (guardOutLen > 0 && overlaps(outRealUpperBand, outRealLowerBand)) {
    throw sharedMemory('MAENV openAndFill', 'outUpperBand', 'outLowerBand');
  }
  if (guardOutLen > 0 && overlaps(outRealMiddleBand, outRealLowerBand)) {
    throw sharedMemory('MAENV openAndFill', 'outMiddleBand', 'outLowerBand');
  }
  return maenvOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType, new MInteger(), new MInteger(), outRealUpperBand, outRealMiddleBand, outRealLowerBand);
}

/**
 * Opens a live MAENV stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link maenv} there.
 *
 * @param real - Warm-up history. Input data series.
 * @param options - The parameters and `core`; the function's other option keys are
 *   ignored. See {@link MaenvOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `maenvLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not a plain object, an option key the function does not
 *   have, a `core` that is not a `Core`, an absent input, an integer parameter that
 *   is not an int32, a real parameter that is not a number or a parameter outside
 *   its range.
 *
 * @example
 * ```ts
 * import { MAType, maenvOpen } from 'ta-lib';
 *
 * const series = Float64Array.from({ length: 252 }, (_, i) =>
 *   90 + (i % 40 - 20) * (i % 40 - 20) / 20.0);
 * const stream = maenvOpen(series, {
 *   timePeriod: 20,
 *   percentUp: 10,
 *   percentDn: 10,
 *   maType: MAType.SMA,
 * });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function maenvOpen(real: ArrayLike<number>, options?: MaenvOptions): MaenvStream {
  const o = maenvOptionsOf('MAENV open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInPercentUp = orDefault(o.percentUp, Core.REAL_DEFAULT);
  const optInPercentDn = orDefault(o.percentDn, Core.REAL_DEFAULT);
  const optInMAType = orDefault(o.maType, Core.INTEGER_DEFAULT);
  const core = coreOf('MAENV open', o.core);
  return maenvOpenPositional(real, optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType, core);
}

/**
 * {@link maenvOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link maenv}.
 *
 * @param real - Warm-up history. Input data series.
 * @param options - The parameters, `core` and output buffers; the function's other
 *   option keys are ignored. See {@link MaenvOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link maenv} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `maenvLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not a plain object, an option key the function does not
 *   have, a `core` that is not a `Core`, an absent input, an integer parameter that
 *   is not an int32, a real parameter that is not a number, a parameter outside its
 *   range or a buffer too short, of the wrong type or starting where another buffer
 *   or a `Float64Array` input starts, or, on a history long enough to open, one
 *   sharing memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { MAType, maenvOpenAndFill } from 'ta-lib';
 *
 * const series = Float64Array.from({ length: 252 }, (_, i) =>
 *   90 + (i % 40 - 20) * (i % 40 - 20) / 20.0);
 * const {
 *   stream,
 *   begIdx,
 *   outUpperBand,
 *   outMiddleBand,
 *   outLowerBand,
 * } = maenvOpenAndFill(series, {
 *   timePeriod: 20,
 *   percentUp: 10,
 *   percentDn: 10,
 *   maType: MAType.SMA,
 * });
 * // outUpperBand[k] is the value at bar begIdx + k
 * ```
 */
export function maenvOpenAndFill(real: ArrayLike<number>, options?: MaenvOptions): MaenvResult & { stream: MaenvStream } {
  const o = maenvOptionsOf('MAENV openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInPercentUp = orDefault(o.percentUp, Core.REAL_DEFAULT);
  const optInPercentDn = orDefault(o.percentDn, Core.REAL_DEFAULT);
  const optInMAType = orDefault(o.maType, Core.INTEGER_DEFAULT);
  const core = coreOf('MAENV openAndFill', o.core);
  const capacity = fillCapacity(maenvLookbackPositional(optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType, core), real);
  const outRealUpperBand = o.outUpperBand === undefined ? new Float64Array(capacity) : o.outUpperBand;
  const outRealMiddleBand = o.outMiddleBand === undefined ? new Float64Array(capacity) : o.outMiddleBand;
  const outRealLowerBand = o.outLowerBand === undefined ? new Float64Array(capacity) : o.outLowerBand;
  const stream = maenvOpenAndFillPositional(real, optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType, outRealUpperBand, outRealMiddleBand, outRealLowerBand, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outUpperBand: outRealUpperBand.subarray(0, range.count), outMiddleBand: outRealMiddleBand.subarray(0, range.count), outLowerBand: outRealLowerBand.subarray(0, range.count) };
}
