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
 *  110726 MF,CC  First version (issue #583).
 */

import { badSink, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { MedpriceStream, medpriceOpenAndFillInternal, medpricePositional } from './medprice.ts';
import { RmaStream, rmaLookbackPositional, rmaOpenAndFillInternal, rmaPositional } from './rma.ts';

/** @internal */
export function gatorLookbackPositional(optInJawPeriod: number, optInJawShift: number, optInTeethPeriod: number, optInTeethShift: number, optInLipsPeriod: number, optInLipsShift: number, core: Core = Core.DEFAULT): number {
  if ((optInJawPeriod | 0) !== optInJawPeriod) {
    return -1;
  }
  if ((optInJawShift | 0) !== optInJawShift) {
    return -1;
  }
  if ((optInTeethPeriod | 0) !== optInTeethPeriod) {
    return -1;
  }
  if ((optInTeethShift | 0) !== optInTeethShift) {
    return -1;
  }
  if ((optInLipsPeriod | 0) !== optInLipsPeriod) {
    return -1;
  }
  if ((optInLipsShift | 0) !== optInLipsShift) {
    return -1;
  }
  if (optInJawPeriod === Core.INTEGER_DEFAULT) {
    optInJawPeriod = 13;
  } else if (optInJawPeriod < 1 || optInJawPeriod > 100000) {
    return -1;
  }
  if (optInJawShift === Core.INTEGER_DEFAULT) {
    optInJawShift = 8;
  } else if (optInJawShift < 0 || optInJawShift > 100000) {
    return -1;
  }
  if (optInTeethPeriod === Core.INTEGER_DEFAULT) {
    optInTeethPeriod = 8;
  } else if (optInTeethPeriod < 1 || optInTeethPeriod > 100000) {
    return -1;
  }
  if (optInTeethShift === Core.INTEGER_DEFAULT) {
    optInTeethShift = 5;
  } else if (optInTeethShift < 0 || optInTeethShift > 100000) {
    return -1;
  }
  if (optInLipsPeriod === Core.INTEGER_DEFAULT) {
    optInLipsPeriod = 5;
  } else if (optInLipsPeriod < 1 || optInLipsPeriod > 100000) {
    return -1;
  }
  if (optInLipsShift === Core.INTEGER_DEFAULT) {
    optInLipsShift = 3;
  } else if (optInLipsShift < 0 || optInLipsShift > 100000) {
    return -1;
  }
  let backJaw = 0;
  let backTeethUpper = 0;
  let backTeethLower = 0;
  let backLips = 0;
  let lookback = 0;
  let legLookback = 0;
  /* Each histogram pairs its two lines as a chart draws them: the line drawn
   * further ahead is read back by the difference of the two shifts. Each term
   * is that leg's rma_lookback plus its read-back, so every leg inherits
   * TA_FUNC_UNST_RMA.
   */
  backJaw = 0;
  backTeethUpper = 0;
  if (optInJawShift > optInTeethShift) {
    backJaw = optInJawShift - optInTeethShift;
  } else {
    backTeethUpper = optInTeethShift - optInJawShift;
  }
  backTeethLower = 0;
  backLips = 0;
  if (optInTeethShift > optInLipsShift) {
    backTeethLower = optInTeethShift - optInLipsShift;
  } else {
    backLips = optInLipsShift - optInTeethShift;
  }
  lookback = rmaLookbackPositional(optInJawPeriod, core) + backJaw;
  legLookback = rmaLookbackPositional(optInTeethPeriod, core) + backTeethUpper;
  if (legLookback > lookback) {
    lookback = legLookback;
  }
  legLookback = rmaLookbackPositional(optInTeethPeriod, core) + backTeethLower;
  if (legLookback > lookback) {
    lookback = legLookback;
  }
  legLookback = rmaLookbackPositional(optInLipsPeriod, core) + backLips;
  if (legLookback > lookback) {
    lookback = legLookback;
  }
  return lookback;
}

/** @internal */
export function gatorDisplayShiftPositional(optInJawPeriod: number, optInJawShift: number, optInTeethPeriod: number, optInTeethShift: number, optInLipsPeriod: number, optInLipsShift: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (gatorLookbackPositional(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, core) < 0) {
    return -2147483648;
  }
  if (optInJawPeriod === Core.INTEGER_DEFAULT) {
    optInJawPeriod = 13;
  } else if (optInJawPeriod < 1 || optInJawPeriod > 100000) {
    return -2147483648;
  }
  if (optInJawShift === Core.INTEGER_DEFAULT) {
    optInJawShift = 8;
  } else if (optInJawShift < 0 || optInJawShift > 100000) {
    return -2147483648;
  }
  if (optInTeethPeriod === Core.INTEGER_DEFAULT) {
    optInTeethPeriod = 8;
  } else if (optInTeethPeriod < 1 || optInTeethPeriod > 100000) {
    return -2147483648;
  }
  if (optInTeethShift === Core.INTEGER_DEFAULT) {
    optInTeethShift = 5;
  } else if (optInTeethShift < 0 || optInTeethShift > 100000) {
    return -2147483648;
  }
  if (optInLipsPeriod === Core.INTEGER_DEFAULT) {
    optInLipsPeriod = 5;
  } else if (optInLipsPeriod < 1 || optInLipsPeriod > 100000) {
    return -2147483648;
  }
  if (optInLipsShift === Core.INTEGER_DEFAULT) {
    optInLipsShift = 3;
  } else if (optInLipsShift < 0 || optInLipsShift > 100000) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 2) {
    return -2147483648;
  }
  if (outputIdx === 0) {
    return (optInJawShift < optInTeethShift) ? optInJawShift : optInTeethShift;
  }
  return (optInTeethShift < optInLipsShift) ? optInTeethShift : optInLipsShift;
}

/** @internal */
export function gatorImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  optInJawPeriod: number,
  optInJawShift: number,
  optInTeethPeriod: number,
  optInTeethShift: number,
  optInLipsPeriod: number,
  optInLipsShift: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outUpper: Float64Array,
  outLower: Float64Array,
): RetCode {
  let i = 0;
  let nbElement = 0;
  let maxBack = 0;
  let lookbackTotal = 0;
  let mpStartIdx = 0;
  let localStart = 0;
  let localEnd = 0;
  let backJaw = 0;
  let backTeethUpper = 0;
  let backTeethLower = 0;
  let backLips = 0;
  const tempBegIdx = new MInteger();
  const tempNbElement = new MInteger();
  let tempMP!: Float64Array;
  let tempA!: Float64Array;
  let tempB!: Float64Array;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInJawPeriod === Core.INTEGER_DEFAULT) {
    optInJawPeriod = 13;
  } else if (optInJawPeriod < 1 || optInJawPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInJawShift === Core.INTEGER_DEFAULT) {
    optInJawShift = 8;
  } else if (optInJawShift < 0 || optInJawShift > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInTeethPeriod === Core.INTEGER_DEFAULT) {
    optInTeethPeriod = 8;
  } else if (optInTeethPeriod < 1 || optInTeethPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInTeethShift === Core.INTEGER_DEFAULT) {
    optInTeethShift = 5;
  } else if (optInTeethShift < 0 || optInTeethShift > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInLipsPeriod === Core.INTEGER_DEFAULT) {
    optInLipsPeriod = 5;
  } else if (optInLipsPeriod < 1 || optInLipsPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInLipsShift === Core.INTEGER_DEFAULT) {
    optInLipsShift = 3;
  } else if (optInLipsShift < 0 || optInLipsShift > 100000) {
    return RetCode.BAD_PARAM;
  }
  if ((outUpper.buffer === outLower.buffer && outUpper.byteOffset === outLower.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  lookbackTotal = gatorLookbackPositional(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, core);
  /* Return before touching anything, so that an input which stops short of
   * endIdx is never read past its end.
   */
  if (lookbackTotal > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  backJaw = 0;
  backTeethUpper = 0;
  if (optInJawShift > optInTeethShift) {
    backJaw = optInJawShift - optInTeethShift;
  } else {
    backTeethUpper = optInTeethShift - optInJawShift;
  }
  backTeethLower = 0;
  backLips = 0;
  if (optInTeethShift > optInLipsShift) {
    backTeethLower = optInTeethShift - optInLipsShift;
  } else {
    backLips = optInLipsShift - optInTeethShift;
  }
  maxBack = backJaw + backTeethUpper;
  if (backTeethLower + backLips > maxBack) {
    maxBack = backTeethLower + backLips;
  }
  mpStartIdx = startIdx - lookbackTotal;
  localStart = lookbackTotal;
  localEnd = endIdx - mpStartIdx;
  nbElement = endIdx - startIdx + 1;
  tempMP = new Float64Array(endIdx - mpStartIdx + 1);
  tempA = new Float64Array(nbElement + maxBack);
  tempB = new Float64Array(nbElement + maxBack);
  /* Every price is consumed here, before any output is written: an output
   * may be aliased onto inHigh or inLow.
   */
  const _xr0 = medpricePositional(mpStartIdx, endIdx, inHigh, inLow, tempMP, core);
  tempBegIdx.value = _xr0.begIdx;
  tempNbElement.value = _xr0.count;
  /* Each leg is its own rma call starting at its read-back bar and seeded at
   * its own lookback before that, so neither histogram moves when the line it
   * does not read changes; the teeth is therefore computed once per
   * histogram. A leg read back by `back` bars starts that many bars early, so
   * element i of its buffer is the bar the output's element i pairs it with.
   */
  const _xr1 = rmaPositional(localStart - backJaw, localEnd, tempMP, optInJawPeriod, tempA, core);
  tempBegIdx.value = _xr1.begIdx;
  tempNbElement.value = _xr1.count;
  const _xr2 = rmaPositional(localStart - backTeethUpper, localEnd, tempMP, optInTeethPeriod, tempB, core);
  tempBegIdx.value = _xr2.begIdx;
  tempNbElement.value = _xr2.count;
  for (i = 0; i < nbElement; i += 1) {
    outUpper[i] = Math.abs(tempA[i] - tempB[i]);
  }
  const _xr3 = rmaPositional(localStart - backTeethLower, localEnd, tempMP, optInTeethPeriod, tempA, core);
  tempBegIdx.value = _xr3.begIdx;
  tempNbElement.value = _xr3.count;
  const _xr4 = rmaPositional(localStart - backLips, localEnd, tempMP, optInLipsPeriod, tempB, core);
  tempBegIdx.value = _xr4.begIdx;
  tempNbElement.value = _xr4.count;
  /* A zero spread must come out +0.0. Keep the select: gcc folds the literal
   * 0.0 - fabs(x) into -fabs(x), which is -0.0 there.
   */
  for (i = 0; i < nbElement; i += 1) {
    outLower[i] = (tempA[i] < tempB[i]) ? tempA[i] - tempB[i] : tempB[i] - tempA[i];
  }
  outBegIdx.value = startIdx;
  outNBElement.value = nbElement;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['jawPeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['jawShift', Core.INTEGER_DEFAULT, 0, 100000, 'in 0..100000'], ['teethPeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['teethShift', Core.INTEGER_DEFAULT, 0, 100000, 'in 0..100000'], ['lipsPeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['lipsShift', Core.INTEGER_DEFAULT, 0, 100000, 'in 0..100000']] as const;

/** @internal */
export function gatorPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInJawPeriod: number,
  optInJawShift: number,
  optInTeethPeriod: number,
  optInTeethShift: number,
  optInLipsPeriod: number,
  optInLipsShift: number,
  outUpper: Float64Array,
  outLower: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('GATOR', 'startIdx', startIdx);
  endIdx = requireInt32('GATOR', 'endIdx', endIdx);
  optInJawPeriod = requireInt32('GATOR', 'jawPeriod', optInJawPeriod);
  optInJawShift = requireInt32('GATOR', 'jawShift', optInJawShift);
  optInTeethPeriod = requireInt32('GATOR', 'teethPeriod', optInTeethPeriod);
  optInTeethShift = requireInt32('GATOR', 'teethShift', optInTeethShift);
  optInLipsPeriod = requireInt32('GATOR', 'lipsPeriod', optInLipsPeriod);
  optInLipsShift = requireInt32('GATOR', 'lipsShift', optInLipsShift);
  requireIndexRange('GATOR', startIdx, endIdx);
  const guardLookback = gatorLookbackPositional(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, core);
  if (guardLookback < 0) {
    throw rejectedParams('GATOR', PARAM_RANGES, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift);
  }
  const guardStart = clampedStart('GATOR', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('GATOR', 'prices.high', inHigh, guardInLen);
  requireLength('GATOR', 'prices.low', inLow, guardInLen);
  requireOutput('GATOR', 'outUpper', outUpper, guardOutLen, Float64Array);
  requireOutput('GATOR', 'outLower', outLower, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlapsPartly(outUpper, inHighF64)) {
    throw sharedMemory('GATOR', 'outUpper', 'prices.high');
  }
  if (overlapsPartly(outUpper, inLowF64)) {
    throw sharedMemory('GATOR', 'outUpper', 'prices.low');
  }
  if (overlapsPartly(outLower, inHighF64)) {
    throw sharedMemory('GATOR', 'outLower', 'prices.high');
  }
  if (overlapsPartly(outLower, inLowF64)) {
    throw sharedMemory('GATOR', 'outLower', 'prices.low');
  }
  if (overlaps(outUpper, outLower)) {
    throw sharedMemory('GATOR', 'outUpper', 'outLower');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = gatorImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, outBegIdx, outNBElement, outUpper, outLower);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('GATOR', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link gator}, {@link gatorLookback}, {@link gatorDisplayShift},
 * {@link gatorOpen} and {@link gatorOpenAndFill}. Every key is optional: an
 * omitted or `undefined` key takes its default, and a call ignores the keys it has
 * no use for.
 */
export interface GatorOptions {
  /**
   * Number of bars in the jaw's smoothed moving average (default 13; range 1 to
   * 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  jawPeriod?: number | undefined;
  /**
   * Bars ahead of the computing bar at which a chart draws the jaw (default 8; range
   * 0 to 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  jawShift?: number | undefined;
  /**
   * Number of bars in the teeth's smoothed moving average (default 8; range 1 to
   * 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  teethPeriod?: number | undefined;
  /**
   * Bars ahead of the computing bar at which a chart draws the teeth (default 5;
   * range 0 to 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  teethShift?: number | undefined;
  /**
   * Number of bars in the lips' smoothed moving average (default 5; range 1 to
   * 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  lipsPeriod?: number | undefined;
  /**
   * Bars ahead of the computing bar at which a chart draws the lips (default 3;
   * range 0 to 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  lipsShift?: number | undefined;
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
   * Buffer for `outUpper`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outUpper?: Float64Array<ArrayBuffer> | undefined;
  /**
   * Buffer for `outLower`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outLower?: Float64Array<ArrayBuffer> | undefined;
}

/** What {@link gator} returns, and {@link gatorOpenAndFill} with its stream. */
export interface GatorResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Gap between the Jaw and the Teeth, zero or above. `outUpper[k]` is the value at
   * input bar `begIdx + k`.
   */
  outUpper: Float64Array<ArrayBuffer>;
  /**
   * Gap between the Teeth and the Lips, zero or below. `outLower[k]` is the value at
   * input bar `begIdx + k`.
   */
  outLower: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link gator} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link GatorOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function gatorLookback(options?: GatorOptions): number {
  const o = optionsOf('GATOR lookback', options);
  return gatorLookbackPositional(orDefault(o.jawPeriod, Core.INTEGER_DEFAULT), orDefault(o.jawShift, Core.INTEGER_DEFAULT), orDefault(o.teethPeriod, Core.INTEGER_DEFAULT), orDefault(o.teethShift, Core.INTEGER_DEFAULT), orDefault(o.lipsPeriod, Core.INTEGER_DEFAULT), orDefault(o.lipsShift, Core.INTEGER_DEFAULT), coreOf('GATOR lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link gator}.
 *
 * The values are never shifted: this describes the drawing only.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link GatorOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function gatorDisplayShift(outputIdx: number, options?: GatorOptions): number {
  const o = optionsOf('GATOR displayShift', options);
  return gatorDisplayShiftPositional(orDefault(o.jawPeriod, Core.INTEGER_DEFAULT), orDefault(o.jawShift, Core.INTEGER_DEFAULT), orDefault(o.teethPeriod, Core.INTEGER_DEFAULT), orDefault(o.teethShift, Core.INTEGER_DEFAULT), orDefault(o.lipsPeriod, Core.INTEGER_DEFAULT), orDefault(o.lipsShift, Core.INTEGER_DEFAULT), outputIdx, coreOf('GATOR displayShift', o.core));
}

/**
 * Bill Williams' Gator Oscillator: how far apart the three Alligator lines are, as
 * two histograms. The upper bars are the gap between Jaw and Teeth, the lower bars
 * the gap between Teeth and Lips drawn below zero. Both growing is the alligator
 * eating, a trend under way; both shrinking toward zero is it falling asleep, the
 * lines converging in a range. One side growing while the other shrinks is it
 * waking or sated, the turn from one state into the other. It is read under the
 * Alligator ([`ALLIGATOR`](https://ta-lib.org/functions/alligator)), the
 * Profitunity leg it measures.
 *
 * Formula and more info at
 * [ta-lib.org/functions/gator](https://ta-lib.org/functions/gator).
 *
 * **Notes**
 * - Each line is drawn ahead by its own shift, so the line drawn further ahead is
 *   read back by the difference of the two shifts: at the default shifts Upper_t = |
 *   Jaw_t−3 − Teeth_t | and Lower_t = −| Teeth_t−2 − Lips_t |. `gatorDisplayShift`
 *   reports where a chart draws each histogram, the smaller of its two lines'
 *   shifts; it changes nothing about the values or the returned range.
 * - Each histogram computes its own lines, each seeded over the bars its own
 *   lookback reaches back, so the Teeth an output reads can differ from the
 *   `alligator` Teeth by the seed until it has decayed; setting the `RMA` unstable
 *   period closes the gap.
 * - A zero Lower is `+0.0`.
 *
 * `outUpper[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link gatorLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `high`: High price of
 *   each bar. `low`: Low price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link GatorOptions}.
 * @returns `begIdx` and the values; see {@link GatorResult}. Each output is a view
 *   of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, an integer parameter that is not an int32, a parameter outside its range,
 *   an input too short for the range, a buffer that is empty, too short or of the
 *   wrong type, two buffers sharing memory or a buffer sharing memory with a
 *   `Float64Array` input it is not. Checked before anything is written.
 *
 * @example
 * ```ts
 * import { gator } from 'ta-lib';
 *
 * const high = new Float64Array(252); // your data, oldest first
 * const low = new Float64Array(252); // your data, oldest first
 *
 * const { begIdx, outUpper, outLower } = gator({ high, low }, {
 *   jawPeriod: 13,
 *   jawShift: 8,
 *   teethPeriod: 8,
 *   teethShift: 5,
 *   lipsPeriod: 5,
 *   lipsShift: 3,
 * });
 * console.log(outUpper.length + ' values from bar ' + begIdx + '; last outUpper = ' + outUpper[outUpper.length - 1]);
 * ```
 * @see [`alligator`](https://ta-lib.org/functions/alligator)
 * @see [`ao`](https://ta-lib.org/functions/ao)
 * @see [`ac`](https://ta-lib.org/functions/ac)
 * @see [`rma`](https://ta-lib.org/functions/rma)
 */
export function gator(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: GatorOptions): GatorResult {
  requireArgument('GATOR', 'prices', prices);
  const o = optionsOf('GATOR', options);
  if (o.endIdx === undefined) {
    requireSeries('GATOR', 'prices.high', prices.high);
    requireSeries('GATOR', 'prices.low', prices.low);
  }
  const optInJawPeriod = orDefault(o.jawPeriod, Core.INTEGER_DEFAULT);
  const optInJawShift = orDefault(o.jawShift, Core.INTEGER_DEFAULT);
  const optInTeethPeriod = orDefault(o.teethPeriod, Core.INTEGER_DEFAULT);
  const optInTeethShift = orDefault(o.teethShift, Core.INTEGER_DEFAULT);
  const optInLipsPeriod = orDefault(o.lipsPeriod, Core.INTEGER_DEFAULT);
  const optInLipsShift = orDefault(o.lipsShift, Core.INTEGER_DEFAULT);
  const core = coreOf('GATOR', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, gatorLookbackPositional(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, core), prices.high, prices.low);
  const outUpper = o.outUpper === undefined ? new Float64Array(capacity) : o.outUpper;
  const outLower = o.outLower === undefined ? new Float64Array(capacity) : o.outLower;
  const range = gatorPositional(startIdx, endIdx, prices.high, prices.low, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, outUpper, outLower, core);
  return { begIdx: range.begIdx, outUpper: outUpper.subarray(0, range.count), outLower: outLower.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live GATOR stream: one value per closed bar, bit-identical to {@link gator}
 * over the same series. Open it with {@link gatorOpen} or
 * {@link gatorOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/gator](https://ta-lib.org/functions/gator).
 */
export class GatorStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInJawPeriod: number = 0;
  /** @internal */
  optInJawShift: number = 0;
  /** @internal */
  optInTeethPeriod: number = 0;
  /** @internal */
  optInTeethShift: number = 0;
  /** @internal */
  optInLipsPeriod: number = 0;
  /** @internal */
  optInLipsShift: number = 0;
  /** @internal */
  cur_outUpper: number = 0;
  /** @internal */
  cur_outLower: number = 0;
  /** @internal */
  lagPos0: number = 0;
  /** @internal */
  lagCap0: number = 0;
  /** @internal */
  lagBuf0: Float64Array = new Float64Array(1);
  /** @internal */
  lagPos1: number = 0;
  /** @internal */
  lagCap1: number = 0;
  /** @internal */
  lagBuf1: Float64Array = new Float64Array(1);
  /** @internal */
  lagPos2: number = 0;
  /** @internal */
  lagCap2: number = 0;
  /** @internal */
  lagBuf2: Float64Array = new Float64Array(1);
  /** @internal */
  lagPos3: number = 0;
  /** @internal */
  lagCap3: number = 0;
  /** @internal */
  lagBuf3: Float64Array = new Float64Array(1);
  /** @internal */
  sub0!: MedpriceStream;
  /** @internal */
  sub1!: RmaStream;
  /** @internal */
  sub2!: RmaStream;
  /** @internal */
  sub3!: RmaStream;
  /** @internal */
  sub4!: RmaStream;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): GatorStream {
    return new GatorStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link gator} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('GATOR advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns its outputs.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param out - Receives the outputs and is returned; a new {@link GatorOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included, before anything moves.
   *   `TALibError` with code `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached
   *   bar `Core.INDEX_MAX`.
   */
  update(high: number, low: number, out?: GatorOut): GatorOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('GATOR update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('GATOR update', 'GatorOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low)) {
      throw nonFiniteBar('GATOR update', !Number.isFinite(high) ? 'high' : 'low');
    }
    gatorStepImpl(this, high, low);
    this.outRangeCount++;
    if (out === undefined) {
      out = new GatorOut();
    }
    out.outUpper = this.cur_outUpper;
    out.outLower = this.cur_outLower;
    return out;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param out - Receives the outputs and is returned; a new {@link GatorOut} when
   *   omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included.
   */
  peek(high: number, low: number, out?: GatorOut): GatorOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('GATOR peek', 'GatorOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low)) {
      throw nonFiniteBar('GATOR peek', !Number.isFinite(high) ? 'high' : 'low');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
    let cur_tempMP = 0;
    let cur_tempA = 0;
    let cur_tempB = 0;
    let cur_outUpper = 0;
    let cur_outLower = 0;
    let lagIn0 = 0;
    let lagTap0_0 = 0;
    let lagIn1 = 0;
    let lagTap1_0 = 0;
    let lagIn2 = 0;
    let lagTap2_0 = 0;
    let lagIn3 = 0;
    let lagTap3_0 = 0;
    cur_tempMP = sp.sub0.peek(inHigh, inLow);
    cur_tempA = sp.sub1.peek(cur_tempMP);
    lagIn0 = cur_tempA;
    cur_tempB = sp.sub2.peek(cur_tempMP);
    lagIn1 = cur_tempB;
    lagTap0_0 = sp.lagBuf0[sp.lagPos0];
    lagTap0_0 = (sp.lagCap0 === 0) ? lagIn0 : lagTap0_0;
    lagTap1_0 = sp.lagBuf1[sp.lagPos1];
    lagTap1_0 = (sp.lagCap1 === 0) ? lagIn1 : lagTap1_0;
    cur_outUpper = Math.abs(lagTap0_0 - lagTap1_0);
    cur_tempA = sp.sub3.peek(cur_tempMP);
    lagIn2 = cur_tempA;
    cur_tempB = sp.sub4.peek(cur_tempMP);
    lagIn3 = cur_tempB;
    lagTap2_0 = sp.lagBuf2[sp.lagPos2];
    lagTap2_0 = (sp.lagCap2 === 0) ? lagIn2 : lagTap2_0;
    lagTap3_0 = sp.lagBuf3[sp.lagPos3];
    lagTap3_0 = (sp.lagCap3 === 0) ? lagIn3 : lagTap3_0;
    cur_outLower = (lagTap2_0 < lagTap3_0) ? lagTap2_0 - lagTap3_0 : lagTap3_0 - lagTap2_0;
    if (out === undefined) {
      out = new GatorOut();
    }
    out.outUpper = cur_outUpper;
    out.outLower = cur_outLower;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link GatorOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a sink that is not an object,
   *   `null` included.
   */
  value(out?: GatorOut): GatorOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('GATOR value', 'GatorOut');
    }
    if (out === undefined) {
      out = new GatorOut();
    }
    out.outUpper = this.cur_outUpper;
    out.outLower = this.cur_outLower;
    return out;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): GatorStream {
    const c = new GatorStream(this.core);
    c.optInJawPeriod = this.optInJawPeriod;
    c.optInJawShift = this.optInJawShift;
    c.optInTeethPeriod = this.optInTeethPeriod;
    c.optInTeethShift = this.optInTeethShift;
    c.optInLipsPeriod = this.optInLipsPeriod;
    c.optInLipsShift = this.optInLipsShift;
    c.cur_outUpper = this.cur_outUpper;
    c.cur_outLower = this.cur_outLower;
    c.lagPos0 = this.lagPos0;
    c.lagCap0 = this.lagCap0;
    c.lagBuf0 = this.lagBuf0.slice();
    c.lagPos1 = this.lagPos1;
    c.lagCap1 = this.lagCap1;
    c.lagBuf1 = this.lagBuf1.slice();
    c.lagPos2 = this.lagPos2;
    c.lagCap2 = this.lagCap2;
    c.lagBuf2 = this.lagBuf2.slice();
    c.lagPos3 = this.lagPos3;
    c.lagCap3 = this.lagCap3;
    c.lagBuf3 = this.lagBuf3.slice();
    c.sub0 = this.sub0.clone();
    c.sub1 = this.sub1.clone();
    c.sub2 = this.sub2.clone();
    c.sub3 = this.sub3.clone();
    c.sub4 = this.sub4.clone();
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one GATOR bar, as `update`, `peek` and `value` return them.
 * Passed back to one of them, it is filled and returned again, with no allocation;
 * it holds its values until the next call that writes it. A new one holds `NaN`.
 */
export class GatorOut {
  /** Gap between the Jaw and the Teeth, zero or above. */
  outUpper: number = NaN;
  /** Gap between the Teeth and the Lips, zero or below. */
  outLower: number = NaN;
}

function gatorStepImpl(sp: GatorStream, inHigh: number, inLow: number): void {
  let cur_tempMP = 0;
  let cur_tempA = 0;
  let cur_tempB = 0;
  let cur_outUpper = 0;
  let cur_outLower = 0;
  let lagIn0 = 0;
  let lagTap0_0 = 0;
  let lagIn1 = 0;
  let lagTap1_0 = 0;
  let lagIn2 = 0;
  let lagTap2_0 = 0;
  let lagIn3 = 0;
  let lagTap3_0 = 0;
  let lagCapL0 = 0;
  let lagPosL0 = 0;
  let lagCapL1 = 0;
  let lagPosL1 = 0;
  let lagCapL2 = 0;
  let lagPosL2 = 0;
  let lagCapL3 = 0;
  let lagPosL3 = 0;
  cur_tempMP = sp.sub0.update(inHigh, inLow);
  cur_tempA = sp.sub1.update(cur_tempMP);
  lagIn0 = cur_tempA;
  cur_tempB = sp.sub2.update(cur_tempMP);
  lagIn1 = cur_tempB;
  lagTap0_0 = sp.lagBuf0[sp.lagPos0];
  lagTap0_0 = (sp.lagCap0 === 0) ? lagIn0 : lagTap0_0;
  lagTap1_0 = sp.lagBuf1[sp.lagPos1];
  lagTap1_0 = (sp.lagCap1 === 0) ? lagIn1 : lagTap1_0;
  cur_outUpper = Math.abs(lagTap0_0 - lagTap1_0);
  cur_tempA = sp.sub3.update(cur_tempMP);
  lagIn2 = cur_tempA;
  cur_tempB = sp.sub4.update(cur_tempMP);
  lagIn3 = cur_tempB;
  lagTap2_0 = sp.lagBuf2[sp.lagPos2];
  lagTap2_0 = (sp.lagCap2 === 0) ? lagIn2 : lagTap2_0;
  lagTap3_0 = sp.lagBuf3[sp.lagPos3];
  lagTap3_0 = (sp.lagCap3 === 0) ? lagIn3 : lagTap3_0;
  cur_outLower = (lagTap2_0 < lagTap3_0) ? lagTap2_0 - lagTap3_0 : lagTap3_0 - lagTap2_0;
  lagCapL0 = sp.lagCap0;
  lagPosL0 = sp.lagPos0;
  sp.lagBuf0[lagPosL0] = lagIn0;
  lagPosL0 = lagPosL0 + 1;
  lagPosL0 = lagPosL0 & (lagPosL0 - lagCapL0 >> 31);
  sp.lagPos0 = lagPosL0;
  lagCapL1 = sp.lagCap1;
  lagPosL1 = sp.lagPos1;
  sp.lagBuf1[lagPosL1] = lagIn1;
  lagPosL1 = lagPosL1 + 1;
  lagPosL1 = lagPosL1 & (lagPosL1 - lagCapL1 >> 31);
  sp.lagPos1 = lagPosL1;
  lagCapL2 = sp.lagCap2;
  lagPosL2 = sp.lagPos2;
  sp.lagBuf2[lagPosL2] = lagIn2;
  lagPosL2 = lagPosL2 + 1;
  lagPosL2 = lagPosL2 & (lagPosL2 - lagCapL2 >> 31);
  sp.lagPos2 = lagPosL2;
  lagCapL3 = sp.lagCap3;
  lagPosL3 = sp.lagPos3;
  sp.lagBuf3[lagPosL3] = lagIn3;
  lagPosL3 = lagPosL3 + 1;
  lagPosL3 = lagPosL3 & (lagPosL3 - lagCapL3 >> 31);
  sp.lagPos3 = lagPosL3;
  sp.cur_outUpper = cur_outUpper;
  sp.cur_outLower = cur_outLower;
}

function gatorOpenImpl(
  sp: GatorStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInJawPeriod: number,
  optInJawShift: number,
  optInTeethPeriod: number,
  optInTeethShift: number,
  optInLipsPeriod: number,
  optInLipsShift: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outUpper: Float64Array,
  outLower: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let i = 0;
  let nbElement = 0;
  let maxBack = 0;
  let lookbackTotal = 0;
  let mpStartIdx = 0;
  let localStart = 0;
  let localEnd = 0;
  let backJaw = 0;
  let backTeethUpper = 0;
  let backTeethLower = 0;
  let backLips = 0;
  const tempBegIdx = new MInteger();
  const tempNbElement = new MInteger();
  let tempMP!: Float64Array;
  let tempA!: Float64Array;
  let tempB!: Float64Array;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInJawPeriod === Core.INTEGER_DEFAULT) {
    optInJawPeriod = 13;
  } else if (optInJawPeriod < 1 || optInJawPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInJawShift === Core.INTEGER_DEFAULT) {
    optInJawShift = 8;
  } else if (optInJawShift < 0 || optInJawShift > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInTeethPeriod === Core.INTEGER_DEFAULT) {
    optInTeethPeriod = 8;
  } else if (optInTeethPeriod < 1 || optInTeethPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInTeethShift === Core.INTEGER_DEFAULT) {
    optInTeethShift = 5;
  } else if (optInTeethShift < 0 || optInTeethShift > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInLipsPeriod === Core.INTEGER_DEFAULT) {
    optInLipsPeriod = 5;
  } else if (optInLipsPeriod < 1 || optInLipsPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInLipsShift === Core.INTEGER_DEFAULT) {
    optInLipsShift = 3;
  } else if (optInLipsShift < 0 || optInLipsShift > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen < gatorLookbackPositional(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const sc_outUpper = outStride === 1 ? outUpper : new Float64Array(historyLen);
  const sc_outLower = outStride === 1 ? outLower : new Float64Array(historyLen);
  let lagPos0 = 0;
  let lagCap0 = 0;
  let lagBuf0!: Float64Array;
  let lagPos1 = 0;
  let lagCap1 = 0;
  let lagBuf1!: Float64Array;
  let lagPos2 = 0;
  let lagCap2 = 0;
  let lagBuf2!: Float64Array;
  let lagPos3 = 0;
  let lagCap3 = 0;
  let lagBuf3!: Float64Array;
  let lagD0_0 = 0;
  let lagD1_0 = 0;
  let lagD2_0 = 0;
  let lagD3_0 = 0;
  let lagNb0 = 0;
  let lagNb1 = 0;
  let lagNb2 = 0;
  let lagNb3 = 0;
  lookbackTotal = gatorLookbackPositional(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, core);
  /* Return before touching anything, so that an input which stops short of
   * endIdx is never read past its end.
   */
  if (lookbackTotal > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  backJaw = 0;
  backTeethUpper = 0;
  if (optInJawShift > optInTeethShift) {
    backJaw = optInJawShift - optInTeethShift;
  } else {
    backTeethUpper = optInTeethShift - optInJawShift;
  }
  backTeethLower = 0;
  backLips = 0;
  if (optInTeethShift > optInLipsShift) {
    backTeethLower = optInTeethShift - optInLipsShift;
  } else {
    backLips = optInLipsShift - optInTeethShift;
  }
  maxBack = backJaw + backTeethUpper;
  if (backTeethLower + backLips > maxBack) {
    maxBack = backTeethLower + backLips;
  }
  mpStartIdx = startIdx - lookbackTotal;
  localStart = lookbackTotal;
  localEnd = endIdx - mpStartIdx;
  nbElement = endIdx - startIdx + 1;
  tempMP = new Float64Array(endIdx - mpStartIdx + 1);
  tempA = new Float64Array(nbElement + maxBack);
  tempB = new Float64Array(nbElement + maxBack);
  /* Every price is consumed here, before any output is written: an output
   * may be aliased onto inHigh or inLow.
   */
  const sub0 = medpriceOpenAndFillInternal(core, inHigh, inLow, mpStartIdx, tempBegIdx, tempNbElement, tempMP);
  /* Each leg is its own rma call starting at its read-back bar and seeded at
   * its own lookback before that, so neither histogram moves when the line it
   * does not read changes; the teeth is therefore computed once per
   * histogram. A leg read back by `back` bars starts that many bars early, so
   * element i of its buffer is the bar the output's element i pairs it with.
   */
  const sub1 = rmaOpenAndFillInternal(core, tempMP.slice(0, (localEnd) + 1), localStart - backJaw, optInJawPeriod, tempBegIdx, tempNbElement, tempA);
  lagNb0 = tempNbElement.value;
  const sub2 = rmaOpenAndFillInternal(core, tempMP.slice(0, (localEnd) + 1), localStart - backTeethUpper, optInTeethPeriod, tempBegIdx, tempNbElement, tempB);
  lagNb1 = tempNbElement.value;
  if (nbElement < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  lagD0_0 = lagNb0 - nbElement;
  if (lagD0_0 < 0 || lagD0_0 > lagNb0 - 1) {
    return RetCode.INTERNAL_ERROR;
  }
  lagD1_0 = lagNb1 - nbElement;
  if (lagD1_0 < 0 || lagD1_0 > lagNb1 - 1) {
    return RetCode.INTERNAL_ERROR;
  }
  lagCap0 = lagD0_0;
  lagBuf0 = new Float64Array(lagCap0 > 0 ? lagCap0 : 1);
  lagBuf0.set(tempA.subarray(lagNb0 - lagCap0, lagNb0), 0);
  lagPos0 = 0;
  lagCap1 = lagD1_0;
  lagBuf1 = new Float64Array(lagCap1 > 0 ? lagCap1 : 1);
  lagBuf1.set(tempB.subarray(lagNb1 - lagCap1, lagNb1), 0);
  lagPos1 = 0;
  for (i = 0; i < nbElement; i += 1) {
    sc_outUpper[i] = Math.abs(tempA[i] - tempB[i]);
  }
  const sub3 = rmaOpenAndFillInternal(core, tempMP.slice(0, (localEnd) + 1), localStart - backTeethLower, optInTeethPeriod, tempBegIdx, tempNbElement, tempA);
  lagNb2 = tempNbElement.value;
  const sub4 = rmaOpenAndFillInternal(core, tempMP.slice(0, (localEnd) + 1), localStart - backLips, optInLipsPeriod, tempBegIdx, tempNbElement, tempB);
  lagNb3 = tempNbElement.value;
  /* A zero spread must come out +0.0. Keep the select: gcc folds the literal
   * 0.0 - fabs(x) into -fabs(x), which is -0.0 there.
   */
  if (nbElement < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  lagD2_0 = lagNb2 - nbElement;
  if (lagD2_0 < 0 || lagD2_0 > lagNb2 - 1) {
    return RetCode.INTERNAL_ERROR;
  }
  lagD3_0 = lagNb3 - nbElement;
  if (lagD3_0 < 0 || lagD3_0 > lagNb3 - 1) {
    return RetCode.INTERNAL_ERROR;
  }
  lagCap2 = lagD2_0;
  lagBuf2 = new Float64Array(lagCap2 > 0 ? lagCap2 : 1);
  lagBuf2.set(tempA.subarray(lagNb2 - lagCap2, lagNb2), 0);
  lagPos2 = 0;
  lagCap3 = lagD3_0;
  lagBuf3 = new Float64Array(lagCap3 > 0 ? lagCap3 : 1);
  lagBuf3.set(tempB.subarray(lagNb3 - lagCap3, lagNb3), 0);
  lagPos3 = 0;
  for (i = 0; i < nbElement; i += 1) {
    sc_outLower[i] = (tempA[i] < tempB[i]) ? tempA[i] - tempB[i] : tempB[i] - tempA[i];
  }
  outBegIdx.value = startIdx;
  outNBElement.value = nbElement;
  if (outNBElement.value < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  sp.optInJawPeriod = optInJawPeriod;
  sp.optInJawShift = optInJawShift;
  sp.optInTeethPeriod = optInTeethPeriod;
  sp.optInTeethShift = optInTeethShift;
  sp.optInLipsPeriod = optInLipsPeriod;
  sp.optInLipsShift = optInLipsShift;
  sp.sub0 = sub0;
  sp.sub1 = sub1;
  sp.sub2 = sub2;
  sp.sub3 = sub3;
  sp.sub4 = sub4;
  sp.lagPos0 = lagPos0;
  sp.lagCap0 = lagCap0;
  sp.lagBuf0 = lagBuf0;
  sp.lagPos1 = lagPos1;
  sp.lagCap1 = lagCap1;
  sp.lagBuf1 = lagBuf1;
  sp.lagPos2 = lagPos2;
  sp.lagCap2 = lagCap2;
  sp.lagBuf2 = lagBuf2;
  sp.lagPos3 = lagPos3;
  sp.lagCap3 = lagCap3;
  sp.lagBuf3 = lagBuf3;
  sp.cur_outUpper = sc_outUpper[outNBElement.value - 1];
  sp.cur_outLower = sc_outLower[outNBElement.value - 1];
  return RetCode.SUCCESS;
}

/** @internal */
export function gatorOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInJawPeriod: number,
  optInJawShift: number,
  optInTeethPeriod: number,
  optInTeethShift: number,
  optInLipsPeriod: number,
  optInLipsShift: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outUpper: Float64Array,
  outLower: Float64Array,
): GatorStream {
  const sp = GatorStream.of(core);
  const retCode = gatorOpenImpl(sp, inHigh, inLow, startIdx, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, outBegIdx, outNBElement, outUpper, outLower, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('GATOR openAndFill', inHigh.length, startIdx, gatorLookbackPositional(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, core));
  }
  if (retCode === RetCode.BAD_PARAM && gatorLookbackPositional(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, core) < 0) {
    throw rejectedParams('GATOR openAndFill', PARAM_RANGES, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift);
  }
  throw streamFailure('GATOR openAndFill', retCode);
}

/** @internal */
export function gatorOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInJawPeriod: number,
  optInJawShift: number,
  optInTeethPeriod: number,
  optInTeethShift: number,
  optInLipsPeriod: number,
  optInLipsShift: number,
): GatorStream {
  const sp = GatorStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outUpper = new Float64Array(1);
  const sink_outLower = new Float64Array(1);
  const retCode = gatorOpenImpl(sp, inHigh, inLow, startIdx, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, outBegIdx, outNBElement, sink_outUpper, sink_outLower, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('GATOR open', inHigh.length, startIdx, gatorLookbackPositional(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, core));
  }
  if (retCode === RetCode.BAD_PARAM && gatorLookbackPositional(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, core) < 0) {
    throw rejectedParams('GATOR open', PARAM_RANGES, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift);
  }
  throw streamFailure('GATOR open', retCode);
}

/** @internal */
export function gatorOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInJawPeriod: number,
  optInJawShift: number,
  optInTeethPeriod: number,
  optInTeethShift: number,
  optInLipsPeriod: number,
  optInLipsShift: number,
  core: Core = Core.DEFAULT,
): GatorStream {
  optInJawPeriod = requireInt32('GATOR open', 'jawPeriod', optInJawPeriod);
  optInJawShift = requireInt32('GATOR open', 'jawShift', optInJawShift);
  optInTeethPeriod = requireInt32('GATOR open', 'teethPeriod', optInTeethPeriod);
  optInTeethShift = requireInt32('GATOR open', 'teethShift', optInTeethShift);
  optInLipsPeriod = requireInt32('GATOR open', 'lipsPeriod', optInLipsPeriod);
  optInLipsShift = requireInt32('GATOR open', 'lipsShift', optInLipsShift);
  requireArgument('GATOR open', 'prices.high', inHigh);
  requireHistory('GATOR open', 'prices.high', inHigh.length);
  requireArgument('GATOR open', 'prices.low', inLow);
  requireHistoryLength('GATOR open', 'prices.low', inLow.length, inHigh.length);
  return gatorOpenInternal(core, float64(inHigh), float64(inLow), 0, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift);
}

/** @internal */
export function gatorOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInJawPeriod: number,
  optInJawShift: number,
  optInTeethPeriod: number,
  optInTeethShift: number,
  optInLipsPeriod: number,
  optInLipsShift: number,
  outUpper: Float64Array,
  outLower: Float64Array,
  core: Core = Core.DEFAULT,
): GatorStream {
  optInJawPeriod = requireInt32('GATOR openAndFill', 'jawPeriod', optInJawPeriod);
  optInJawShift = requireInt32('GATOR openAndFill', 'jawShift', optInJawShift);
  optInTeethPeriod = requireInt32('GATOR openAndFill', 'teethPeriod', optInTeethPeriod);
  optInTeethShift = requireInt32('GATOR openAndFill', 'teethShift', optInTeethShift);
  optInLipsPeriod = requireInt32('GATOR openAndFill', 'lipsPeriod', optInLipsPeriod);
  optInLipsShift = requireInt32('GATOR openAndFill', 'lipsShift', optInLipsShift);
  requireArgument('GATOR openAndFill', 'prices.high', inHigh);
  requireHistory('GATOR openAndFill', 'prices.high', inHigh.length);
  requireArgument('GATOR openAndFill', 'prices.low', inLow);
  const guardLookback = gatorLookbackPositional(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, core);
  if (guardLookback < 0) {
    throw rejectedParams('GATOR openAndFill', PARAM_RANGES, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift);
  }
  const guardOutLen = openFillCount('GATOR openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('GATOR openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireOutput('GATOR openAndFill', 'outUpper', outUpper, guardOutLen, Float64Array);
  requireOutput('GATOR openAndFill', 'outLower', outLower, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (outUpper.buffer === inHighF64.buffer && outUpper.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('GATOR openAndFill', 'outUpper', 'prices.high');
  }
  if (outUpper.buffer === inLowF64.buffer && outUpper.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('GATOR openAndFill', 'outUpper', 'prices.low');
  }
  if (outLower.buffer === inHighF64.buffer && outLower.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('GATOR openAndFill', 'outLower', 'prices.high');
  }
  if (outLower.buffer === inLowF64.buffer && outLower.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('GATOR openAndFill', 'outLower', 'prices.low');
  }
  if (outUpper.buffer === outLower.buffer && outUpper.byteOffset === outLower.byteOffset) {
    throw sharedMemory('GATOR openAndFill', 'outUpper', 'outLower');
  }
  if (guardOutLen > 0 && overlaps(outUpper, inHighF64)) {
    throw sharedMemory('GATOR openAndFill', 'outUpper', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outUpper, inLowF64)) {
    throw sharedMemory('GATOR openAndFill', 'outUpper', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outLower, inHighF64)) {
    throw sharedMemory('GATOR openAndFill', 'outLower', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outLower, inLowF64)) {
    throw sharedMemory('GATOR openAndFill', 'outLower', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outUpper, outLower)) {
    throw sharedMemory('GATOR openAndFill', 'outUpper', 'outLower');
  }
  return gatorOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, new MInteger(), new MInteger(), outUpper, outLower);
}

/**
 * Opens a live GATOR stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link gator} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link GatorOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `gatorLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range
 *   or histories of different lengths.
 *
 * @example
 * ```ts
 * import { gatorOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = gatorOpen({ high, low }, {
 *   jawPeriod: 13,
 *   jawShift: 8,
 *   teethPeriod: 8,
 *   teethShift: 5,
 *   lipsPeriod: 5,
 *   lipsShift: 3,
 * });
 * const peeked = stream.peek(101.4, 99.1); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1); // the bar closes
 * ```
 */
export function gatorOpen(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: GatorOptions): GatorStream {
  requireArgument('GATOR open', 'prices', prices);
  const o = optionsOf('GATOR open', options);
  const optInJawPeriod = orDefault(o.jawPeriod, Core.INTEGER_DEFAULT);
  const optInJawShift = orDefault(o.jawShift, Core.INTEGER_DEFAULT);
  const optInTeethPeriod = orDefault(o.teethPeriod, Core.INTEGER_DEFAULT);
  const optInTeethShift = orDefault(o.teethShift, Core.INTEGER_DEFAULT);
  const optInLipsPeriod = orDefault(o.lipsPeriod, Core.INTEGER_DEFAULT);
  const optInLipsShift = orDefault(o.lipsShift, Core.INTEGER_DEFAULT);
  const core = coreOf('GATOR open', o.core);
  return gatorOpenPositional(prices.high, prices.low, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, core);
}

/**
 * {@link gatorOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link gator}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link GatorOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link gator} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `gatorLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { gatorOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, outUpper, outLower } = gatorOpenAndFill({ high, low }, {
 *   jawPeriod: 13,
 *   jawShift: 8,
 *   teethPeriod: 8,
 *   teethShift: 5,
 *   lipsPeriod: 5,
 *   lipsShift: 3,
 * });
 * // outUpper[k] is the value at bar begIdx + k
 * ```
 */
export function gatorOpenAndFill(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close?: ArrayLike<number> | undefined; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: GatorOptions): GatorResult & { stream: GatorStream } {
  requireArgument('GATOR openAndFill', 'prices', prices);
  const o = optionsOf('GATOR openAndFill', options);
  const optInJawPeriod = orDefault(o.jawPeriod, Core.INTEGER_DEFAULT);
  const optInJawShift = orDefault(o.jawShift, Core.INTEGER_DEFAULT);
  const optInTeethPeriod = orDefault(o.teethPeriod, Core.INTEGER_DEFAULT);
  const optInTeethShift = orDefault(o.teethShift, Core.INTEGER_DEFAULT);
  const optInLipsPeriod = orDefault(o.lipsPeriod, Core.INTEGER_DEFAULT);
  const optInLipsShift = orDefault(o.lipsShift, Core.INTEGER_DEFAULT);
  const core = coreOf('GATOR openAndFill', o.core);
  const capacity = fillCapacity(gatorLookbackPositional(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, core), prices.high, prices.low);
  const outUpper = o.outUpper === undefined ? new Float64Array(capacity) : o.outUpper;
  const outLower = o.outLower === undefined ? new Float64Array(capacity) : o.outLower;
  const stream = gatorOpenAndFillPositional(prices.high, prices.low, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, outUpper, outLower, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outUpper: outUpper.subarray(0, range.count), outLower: outLower.subarray(0, range.count) };
}
