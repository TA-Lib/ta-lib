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
 *  DM       Drew McCormack (http://www.trade-strategist.com)
 *  MF       Mario Fortier
 *  DX       Dex Hunter (https://github.com/dexhunter)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY    Description
 *  -------------------------------------------------------------------
 *  281206 DM    Initial Implementation
 *  010606 MF    Abstract local arrays. Detect divide by zero.
 *  073126 DX    Evaluate each bar's terms once via a CIRCBUF ring (PR #154).
 *  082326 MF,CC Fix #253. Recognize an empty window by counting bars, so the
 *               divides are guarded exactly instead of against the fixed
 *               TA_IS_ZERO band -- which zeroed the oscillator for any
 *               instrument quoted small enough to fall under it.
 *  102326 MF,CC #554 branch-free zero run, rare reset.
 *  102426 MF,CC #554 zero-run test on the true range itself.
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { smaLookbackPositional } from './sma.ts';

/** @internal */
export function ultoscLookbackPositional(optInTimePeriod1: number, optInTimePeriod2: number, optInTimePeriod3: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod1 | 0) !== optInTimePeriod1) {
    return -1;
  }
  if ((optInTimePeriod2 | 0) !== optInTimePeriod2) {
    return -1;
  }
  if ((optInTimePeriod3 | 0) !== optInTimePeriod3) {
    return -1;
  }
  if (optInTimePeriod1 === Core.INTEGER_DEFAULT) {
    optInTimePeriod1 = 7;
  } else if (optInTimePeriod1 < 1 || optInTimePeriod1 > 100000) {
    return -1;
  }
  if (optInTimePeriod2 === Core.INTEGER_DEFAULT) {
    optInTimePeriod2 = 14;
  } else if (optInTimePeriod2 < 1 || optInTimePeriod2 > 100000) {
    return -1;
  }
  if (optInTimePeriod3 === Core.INTEGER_DEFAULT) {
    optInTimePeriod3 = 28;
  } else if (optInTimePeriod3 < 1 || optInTimePeriod3 > 100000) {
    return -1;
  }
  let maxPeriod = 0;
  /* Lookback for the Ultimate Oscillator is the lookback of the SMA with the longest
   * time period, plus 1 for the True Range.
   */
  maxPeriod = ((optInTimePeriod1 > optInTimePeriod2 ? optInTimePeriod1 : optInTimePeriod2) > optInTimePeriod3 ? (optInTimePeriod1 > optInTimePeriod2 ? optInTimePeriod1 : optInTimePeriod2) : optInTimePeriod3);
  return smaLookbackPositional(maxPeriod, core) + 1;
}

/** @internal */
export function ultoscDisplayShiftPositional(optInTimePeriod1: number, optInTimePeriod2: number, optInTimePeriod3: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (ultoscLookbackPositional(optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function ultoscImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  optInTimePeriod1: number,
  optInTimePeriod2: number,
  optInTimePeriod3: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let a1Total = 0;
  let a2Total = 0;
  let a3Total = 0;
  let b1Total = 0;
  let b2Total = 0;
  let b3Total = 0;
  let trueLow = 0;
  let trueRange = 0;
  let closeMinusTrueLow = 0;
  let tempDouble = 0;
  let output = 0;
  let tempHT = 0;
  let tempLT = 0;
  let tempCY = 0;
  let lookbackTotal = 0;
  let longestPeriod = 0;
  let longestIndex = 0;
  let i = 0;
  let j = 0;
  let today = 0;
  let outIdx = 0;
  let trailingPos1 = 0;
  let trailingPos2 = 0;
  let nullRun = 0;
  let live = 0;
  const usedFlag = new Int32Array(3);
  const periods = new Int32Array(3);
  const sortedPeriods = new Int32Array(3);
  let term_closeMinusTrueLow!: Float64Array;
  let term_trueRange!: Float64Array;
  let term_Idx = 0;
  let maxIdx_term = (32) - 1;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod1 === Core.INTEGER_DEFAULT) {
    optInTimePeriod1 = 7;
  } else if (optInTimePeriod1 < 1 || optInTimePeriod1 > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod2 === Core.INTEGER_DEFAULT) {
    optInTimePeriod2 = 14;
  } else if (optInTimePeriod2 < 1 || optInTimePeriod2 > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod3 === Core.INTEGER_DEFAULT) {
    optInTimePeriod3 = 28;
  } else if (optInTimePeriod3 < 1 || optInTimePeriod3 > 100000) {
    return RetCode.BAD_PARAM;
  }
  /* The two per-bar terms the three moving sums are built from. Both are a
   * pure function of the bar, so each bar is evaluated once on entry and read
   * back when it leaves each of the three windows.
   */
  /* One entry per bar of the longest window. Stays on the stack for every
   * period up to 32, which covers the 7/14/28 default.
   */
  /* Id, Type, Static Size */
  outBegIdx.value = 0;
  outNBElement.value = 0;
  /* Ensure that the time periods are ordered from shortest to longest.
   * Sort.
   */
  periods[0] = optInTimePeriod1;
  periods[1] = optInTimePeriod2;
  periods[2] = optInTimePeriod3;
  usedFlag[0] = 0;
  usedFlag[1] = 0;
  usedFlag[2] = 0;
  for (i = 0; i < 3; i += 1) {
    longestPeriod = 0;
    longestIndex = 0;
    for (j = 0; j < 3; j += 1) {
      if (usedFlag[j] === 0 && periods[j] > longestPeriod) {
        longestPeriod = periods[j];
        longestIndex = j;
      }
    }
    usedFlag[longestIndex] = 1;
    sortedPeriods[i] = longestPeriod;
  }
  optInTimePeriod1 = sortedPeriods[2];
  optInTimePeriod2 = sortedPeriods[1];
  optInTimePeriod3 = sortedPeriods[0];
  /* Adjust startIdx for lookback period. */
  lookbackTotal = ultoscLookbackPositional(optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  if (optInTimePeriod3 < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  term_closeMinusTrueLow = new Float64Array(optInTimePeriod3);
  term_trueRange = new Float64Array(optInTimePeriod3);
  maxIdx_term = optInTimePeriod3 - 1;
  term_Idx = 0;
  /* Prime running totals used in moving averages.
   *
   * One pass over the longest warm-up window replaces three overlapping
   * passes. A bar inside the shorter windows is added to those totals as it
   * is reached, so every total still accumulates exactly the same bars in
   * exactly the same ascending order as three separate loops did.
   */
  a1Total = 0;
  b1Total = 0;
  a2Total = 0;
  b2Total = 0;
  a3Total = 0;
  b3Total = 0;
  /* Consecutive bars that put nothing into the windows, counted so that an
   * empty window can be recognized exactly (the shape #244 needed for MFI).
   * The running totals cannot answer that question themselves: they are
   * maintained by add-then-subtract, so once a window empties they hold
   * rounding residue of arbitrary sign rather than zero, and v0.6.4 divides
   * one residue by another there -- it returns -92.9 for an oscillator
   * documented to run 0..100. Both of a bar's terms have to be zero for it to
   * count, which for valid bars is one condition (a zero true range means
   * H == L == the previous close, which leaves the close on the true low).
   * Reseeding on the count is what lets the divides below be guarded exactly
   * rather than against a fixed band -- a true range carries the quote unit,
   * so the band they used to carry zeroed the oscillator for any instrument
   * quoted below it (issue #253).
   */
  nullRun = 0;
  for (i = startIdx - optInTimePeriod3 + 1; i < startIdx; i += 1) {
    tempLT = inLow[i];
    tempHT = inHigh[i];
    tempCY = inClose[i - 1];
    trueLow = (tempLT < tempCY ? tempLT : tempCY);
    closeMinusTrueLow = inClose[i] - trueLow;
    trueRange = tempHT - tempLT;
    tempDouble = Math.abs(tempCY - tempHT);
    if (tempDouble > trueRange) {
      trueRange = tempDouble;
    }
    tempDouble = Math.abs(tempCY - tempLT);
    if (tempDouble > trueRange) {
      trueRange = tempDouble;
    }
    term_closeMinusTrueLow[term_Idx] = closeMinusTrueLow;
    term_trueRange[term_Idx] = trueRange;
    term_Idx++;
    if (term_Idx > maxIdx_term) {
      term_Idx = 0;
    }
    live = (!(trueRange + Math.abs(closeMinusTrueLow) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
    if (i >= startIdx - optInTimePeriod1 + 1) {
      a1Total += closeMinusTrueLow;
      b1Total += trueRange;
    }
    if (i >= startIdx - optInTimePeriod2 + 1) {
      a2Total += closeMinusTrueLow;
      b2Total += trueRange;
    }
    a3Total += closeMinusTrueLow;
    b3Total += trueRange;
  }
  /* Calculate oscillator */
  today = startIdx;
  outIdx = 0;
  /* The warm-up wrote optInTimePeriod3-1 bars, so term_Idx is the slot for
   * `today` and, once advanced past it, the slot of the bar leaving the
   * longest window. The two shorter windows trail it by a fixed offset.
   */
  trailingPos1 = term_Idx + optInTimePeriod3 - optInTimePeriod1 + 1;
  if (trailingPos1 >= optInTimePeriod3) {
    trailingPos1 -= optInTimePeriod3;
  }
  trailingPos2 = term_Idx + optInTimePeriod3 - optInTimePeriod2 + 1;
  if (trailingPos2 >= optInTimePeriod3) {
    trailingPos2 -= optInTimePeriod3;
  }
  while (today <= endIdx) {
    /* Add on today's terms */
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    trueLow = (tempLT < tempCY ? tempLT : tempCY);
    closeMinusTrueLow = inClose[today] - trueLow;
    trueRange = tempHT - tempLT;
    tempDouble = Math.abs(tempCY - tempHT);
    if (tempDouble > trueRange) {
      trueRange = tempDouble;
    }
    tempDouble = Math.abs(tempCY - tempLT);
    if (tempDouble > trueRange) {
      trueRange = tempDouble;
    }
    term_closeMinusTrueLow[term_Idx] = closeMinusTrueLow;
    term_trueRange[term_Idx] = trueRange;
    a1Total += closeMinusTrueLow;
    a2Total += closeMinusTrueLow;
    a3Total += closeMinusTrueLow;
    b1Total += trueRange;
    b2Total += trueRange;
    b3Total += trueRange;
    /* Once a whole window of no-contribution bars has gone by, every slot it
     * spans is 0.0, so its totals are known to be exactly zero and the
     * residue can be dropped. The periods are sorted shortest-first, so a
     * run long enough for a longer window is long enough for every shorter
     * one.
     */
    live = (!(trueRange + Math.abs(closeMinusTrueLow) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
    if (nullRun >= optInTimePeriod1) {
      a1Total = 0.0;
      b1Total = 0.0;
      if (nullRun >= optInTimePeriod2) {
        a2Total = 0.0;
        b2Total = 0.0;
        if (nullRun >= optInTimePeriod3) {
          nullRun = optInTimePeriod3;
          a3Total = 0.0;
          b3Total = 0.0;
        }
      }
    }
    /* Calculate the oscillator value for today. Each window contributes only
     * when it holds a true range; the totals are sums of non-negative terms
     * and the reseed above removes their residue, so the test is exact.
     */
    output = 0.0;
    if (b1Total > 0.0) {
      output += 4.0 * (a1Total / b1Total);
    }
    if (b2Total > 0.0) {
      output += 2.0 * (a2Total / b2Total);
    }
    if (b3Total > 0.0) {
      output += a3Total / b3Total;
    }
    /* Remove the trailing terms to prepare for next day. Each was evaluated
     * once, when its bar entered the ring.
     */
    a1Total -= term_closeMinusTrueLow[trailingPos1];
    b1Total -= term_trueRange[trailingPos1];
    trailingPos1 += 1;
    if (trailingPos1 >= optInTimePeriod3) {
      trailingPos1 = 0;
    }
    a2Total -= term_closeMinusTrueLow[trailingPos2];
    b2Total -= term_trueRange[trailingPos2];
    trailingPos2 += 1;
    if (trailingPos2 >= optInTimePeriod3) {
      trailingPos2 = 0;
    }
    term_Idx++;
    if (term_Idx > maxIdx_term) {
      term_Idx = 0;
    }
    a3Total -= term_closeMinusTrueLow[term_Idx];
    b3Total -= term_trueRange[term_Idx];
    /* Last operation is to write the output. Must
     * be done after the trailing index have all been
     * taken care of because the caller is allowed
     * to have the input array to be also the output
     * array.
     */
    outReal[outIdx] = 100.0 * (output / 7.0);
    /* Increment indexes */
    outIdx += 1;
    today += 1;
  }
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod1', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['timePeriod2', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['timePeriod3', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000']] as const;

/** @internal */
export function ultoscPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod1: number,
  optInTimePeriod2: number,
  optInTimePeriod3: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('ULTOSC', 'startIdx', startIdx);
  endIdx = requireInt32('ULTOSC', 'endIdx', endIdx);
  optInTimePeriod1 = requireInt32('ULTOSC', 'timePeriod1', optInTimePeriod1);
  optInTimePeriod2 = requireInt32('ULTOSC', 'timePeriod2', optInTimePeriod2);
  optInTimePeriod3 = requireInt32('ULTOSC', 'timePeriod3', optInTimePeriod3);
  requireIndexRange('ULTOSC', startIdx, endIdx);
  const guardLookback = ultoscLookbackPositional(optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, core);
  if (guardLookback < 0) {
    throw rejectedParams('ULTOSC', PARAM_RANGES, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3);
  }
  const guardStart = clampedStart('ULTOSC', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('ULTOSC', 'prices.high', inHigh, guardInLen);
  requireLength('ULTOSC', 'prices.low', inLow, guardInLen);
  requireLength('ULTOSC', 'prices.close', inClose, guardInLen);
  requireOutput('ULTOSC', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outReal, inHighF64)) {
    throw sharedMemory('ULTOSC', 'outReal', 'prices.high');
  }
  if (overlapsPartly(outReal, inLowF64)) {
    throw sharedMemory('ULTOSC', 'outReal', 'prices.low');
  }
  if (overlapsPartly(outReal, inCloseF64)) {
    throw sharedMemory('ULTOSC', 'outReal', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = ultoscImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('ULTOSC', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link ultosc}, {@link ultoscLookback}, {@link ultoscDisplayShift},
 * {@link ultoscOpen} and {@link ultoscOpenAndFill}. Every key is optional: an
 * omitted or `undefined` key takes its default, and a call ignores the keys it has
 * no use for.
 */
export interface UltoscOptions {
  /**
   * Bars for one averaging window (default 7; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  timePeriod1?: number | undefined;
  /**
   * Bars for another averaging window (default 14; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  timePeriod2?: number | undefined;
  /**
   * Bars for another averaging window (default 28; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  timePeriod3?: number | undefined;
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

/** What {@link ultosc} returns, and {@link ultoscOpenAndFill} with its stream. */
export interface UltoscResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /** Ultimate Oscillator value. `values[k]` is the value at input bar `begIdx + k`. */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link ultosc} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link UltoscOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function ultoscLookback(options?: UltoscOptions): number {
  const o = optionsOf('ULTOSC lookback', options);
  return ultoscLookbackPositional(orDefault(o.timePeriod1, Core.INTEGER_DEFAULT), orDefault(o.timePeriod2, Core.INTEGER_DEFAULT), orDefault(o.timePeriod3, Core.INTEGER_DEFAULT), coreOf('ULTOSC lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link ultosc}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link UltoscOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function ultoscDisplayShift(outputIdx: number, options?: UltoscOptions): number {
  const o = optionsOf('ULTOSC displayShift', options);
  return ultoscDisplayShiftPositional(orDefault(o.timePeriod1, Core.INTEGER_DEFAULT), orDefault(o.timePeriod2, Core.INTEGER_DEFAULT), orDefault(o.timePeriod3, Core.INTEGER_DEFAULT), outputIdx, coreOf('ULTOSC displayShift', o.core));
}

/**
 * Ultimate Oscillator: momentum indicator combining buying-pressure/true-range
 * ratios over three time periods into one 0-100 weighted average. Blends short-,
 * medium-, and long-term momentum to damp single-period noise. Ranges 0-100;
 * conventionally >70 overbought, \<30 oversold.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ultosc](https://ta-lib.org/functions/ultosc).
 *
 * **Notes**
 * - The three periods are sorted internally, so the 4/2/1 weighting always applies
 *   to the shortest, middle, and longest period regardless of the order in which
 *   you pass them.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link ultoscLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `high`: High price of
 *   each bar. `low`: Low price of each bar. `close`: Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link UltoscOptions}.
 * @returns `begIdx` and the values; see {@link UltoscResult}. Each output is a
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
 * import { ultosc } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { begIdx, values } = ultosc({ high, low, close }, {
 *   timePeriod1: 7,
 *   timePeriod2: 14,
 *   timePeriod3: 28,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`atr`](https://ta-lib.org/functions/atr)
 * @see [`trange`](https://ta-lib.org/functions/trange)
 * @see [`rsi`](https://ta-lib.org/functions/rsi)
 */
export function ultosc(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: UltoscOptions): UltoscResult {
  requireArgument('ULTOSC', 'prices', prices);
  const o = optionsOf('ULTOSC', options);
  if (o.endIdx === undefined) {
    requireSeries('ULTOSC', 'prices.high', prices.high);
    requireSeries('ULTOSC', 'prices.low', prices.low);
    requireSeries('ULTOSC', 'prices.close', prices.close);
  }
  const optInTimePeriod1 = orDefault(o.timePeriod1, Core.INTEGER_DEFAULT);
  const optInTimePeriod2 = orDefault(o.timePeriod2, Core.INTEGER_DEFAULT);
  const optInTimePeriod3 = orDefault(o.timePeriod3, Core.INTEGER_DEFAULT);
  const core = coreOf('ULTOSC', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, ultoscLookbackPositional(optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, core), prices.high, prices.low, prices.close);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = ultoscPositional(startIdx, endIdx, prices.high, prices.low, prices.close, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live ULTOSC stream: one value per closed bar, bit-identical to {@link ultosc}
 * over the same series. Open it with {@link ultoscOpen} or
 * {@link ultoscOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ultosc](https://ta-lib.org/functions/ultosc).
 */
export class UltoscStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod1: number = 0;
  /** @internal */
  optInTimePeriod2: number = 0;
  /** @internal */
  optInTimePeriod3: number = 0;
  /** @internal */
  a1Total: number = 0;
  /** @internal */
  a2Total: number = 0;
  /** @internal */
  a3Total: number = 0;
  /** @internal */
  b1Total: number = 0;
  /** @internal */
  b2Total: number = 0;
  /** @internal */
  b3Total: number = 0;
  /** @internal */
  trailingPos1: number = 0;
  /** @internal */
  trailingPos2: number = 0;
  /** @internal */
  nullRun: number = 0;
  /** @internal */
  term_Idx: number = 0;
  /** @internal */
  maxIdx_term: number = 0;
  /** @internal */
  lag1_inClose: number = 0;
  /** @internal */
  cbSize_term: number = 0;
  /** @internal */
  cb_term_closeMinusTrueLow: Float64Array = new Float64Array(1);
  /** @internal */
  cb_term_trueRange: Float64Array = new Float64Array(1);
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
  static of(core: Core): UltoscStream {
    return new UltoscStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link ultosc} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('ULTOSC advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(high: number, low: number, close: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('ULTOSC update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('ULTOSC update', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    ultoscStepImpl(this, high, low, close);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(high: number, low: number, close: number): number {
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('ULTOSC peek', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
    const inClose = close;
    let trueLow = 0;
    let trueRange = 0;
    let closeMinusTrueLow = 0;
    let tempDouble = 0;
    let output = 0;
    let tempHT = 0;
    let tempLT = 0;
    let tempCY = 0;
    let live = 0;
    let a1Total = sp.a1Total;
    let a2Total = sp.a2Total;
    let a3Total = sp.a3Total;
    let b1Total = sp.b1Total;
    let b2Total = sp.b2Total;
    let b3Total = sp.b3Total;
    let cur_outReal = 0;
    let nullRun = sp.nullRun;
    let term_Idx = sp.term_Idx;
    let trailingPos1 = sp.trailingPos1;
    let trailingPos2 = sp.trailingPos2;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    let pkSlot1 = -1;
    let pkVal1 = 0;
    /* Add on today's terms */
    tempLT = inLow;
    tempHT = inHigh;
    tempCY = sp.lag1_inClose;
    trueLow = (tempLT < tempCY ? tempLT : tempCY);
    closeMinusTrueLow = inClose - trueLow;
    trueRange = tempHT - tempLT;
    tempDouble = Math.abs(tempCY - tempHT);
    if (tempDouble > trueRange) {
      trueRange = tempDouble;
    }
    tempDouble = Math.abs(tempCY - tempLT);
    if (tempDouble > trueRange) {
      trueRange = tempDouble;
    }
    pkSlot0 = term_Idx;
    pkVal0 = closeMinusTrueLow;
    pkSlot1 = term_Idx;
    pkVal1 = trueRange;
    a1Total += closeMinusTrueLow;
    a2Total += closeMinusTrueLow;
    a3Total += closeMinusTrueLow;
    b1Total += trueRange;
    b2Total += trueRange;
    b3Total += trueRange;
    /* Once a whole window of no-contribution bars has gone by, every slot it
     * spans is 0.0, so its totals are known to be exactly zero and the
     * residue can be dropped. The periods are sorted shortest-first, so a
     * run long enough for a longer window is long enough for every shorter
     * one.
     */
    live = (!(trueRange + Math.abs(closeMinusTrueLow) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
    if (nullRun >= sp.optInTimePeriod1) {
      a1Total = 0.0;
      b1Total = 0.0;
      if (nullRun >= sp.optInTimePeriod2) {
        a2Total = 0.0;
        b2Total = 0.0;
        if (nullRun >= sp.optInTimePeriod3) {
          nullRun = sp.optInTimePeriod3;
          a3Total = 0.0;
          b3Total = 0.0;
        }
      }
    }
    /* Calculate the oscillator value for today. Each window contributes only
     * when it holds a true range; the totals are sums of non-negative terms
     * and the reseed above removes their residue, so the test is exact.
     */
    output = 0.0;
    if (b1Total > 0.0) {
      output += 4.0 * (a1Total / b1Total);
    }
    if (b2Total > 0.0) {
      output += 2.0 * (a2Total / b2Total);
    }
    if (b3Total > 0.0) {
      output += a3Total / b3Total;
    }
    /* Remove the trailing terms to prepare for next day. Each was evaluated
     * once, when its bar entered the ring.
     */
    a1Total -= (trailingPos1 !== pkSlot0) ? sp.cb_term_closeMinusTrueLow[trailingPos1] : pkVal0;
    b1Total -= (trailingPos1 !== pkSlot1) ? sp.cb_term_trueRange[trailingPos1] : pkVal1;
    trailingPos1 += 1;
    if (trailingPos1 >= sp.optInTimePeriod3) {
      trailingPos1 = 0;
    }
    a2Total -= (trailingPos2 !== pkSlot0) ? sp.cb_term_closeMinusTrueLow[trailingPos2] : pkVal0;
    b2Total -= (trailingPos2 !== pkSlot1) ? sp.cb_term_trueRange[trailingPos2] : pkVal1;
    trailingPos2 += 1;
    if (trailingPos2 >= sp.optInTimePeriod3) {
      trailingPos2 = 0;
    }
    term_Idx = term_Idx + 1;
    if (term_Idx > sp.maxIdx_term) {
      term_Idx = 0;
    }
    a3Total -= (term_Idx !== pkSlot0) ? sp.cb_term_closeMinusTrueLow[term_Idx] : pkVal0;
    b3Total -= (term_Idx !== pkSlot1) ? sp.cb_term_trueRange[term_Idx] : pkVal1;
    /* Last operation is to write the output. Must
     * be done after the trailing index have all been
     * taken care of because the caller is allowed
     * to have the input array to be also the output
     * array.
     */
    cur_outReal = 100.0 * (output / 7.0);
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
  clone(): UltoscStream {
    const c = new UltoscStream(this.core);
    c.optInTimePeriod1 = this.optInTimePeriod1;
    c.optInTimePeriod2 = this.optInTimePeriod2;
    c.optInTimePeriod3 = this.optInTimePeriod3;
    c.a1Total = this.a1Total;
    c.a2Total = this.a2Total;
    c.a3Total = this.a3Total;
    c.b1Total = this.b1Total;
    c.b2Total = this.b2Total;
    c.b3Total = this.b3Total;
    c.trailingPos1 = this.trailingPos1;
    c.trailingPos2 = this.trailingPos2;
    c.nullRun = this.nullRun;
    c.term_Idx = this.term_Idx;
    c.maxIdx_term = this.maxIdx_term;
    c.lag1_inClose = this.lag1_inClose;
    c.cbSize_term = this.cbSize_term;
    c.cb_term_closeMinusTrueLow = this.cb_term_closeMinusTrueLow.slice();
    c.cb_term_trueRange = this.cb_term_trueRange.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function ultoscStepImpl(sp: UltoscStream, inHigh: number, inLow: number, inClose: number): void {
  let trueLow = 0;
  let trueRange = 0;
  let closeMinusTrueLow = 0;
  let tempDouble = 0;
  let output = 0;
  let tempHT = 0;
  let tempLT = 0;
  let tempCY = 0;
  let live = 0;
  /* Add on today's terms */
  tempLT = inLow;
  tempHT = inHigh;
  tempCY = sp.lag1_inClose;
  trueLow = (tempLT < tempCY ? tempLT : tempCY);
  closeMinusTrueLow = inClose - trueLow;
  trueRange = tempHT - tempLT;
  tempDouble = Math.abs(tempCY - tempHT);
  if (tempDouble > trueRange) {
    trueRange = tempDouble;
  }
  tempDouble = Math.abs(tempCY - tempLT);
  if (tempDouble > trueRange) {
    trueRange = tempDouble;
  }
  sp.cb_term_closeMinusTrueLow[sp.term_Idx] = closeMinusTrueLow;
  sp.cb_term_trueRange[sp.term_Idx] = trueRange;
  sp.a1Total += closeMinusTrueLow;
  sp.a2Total += closeMinusTrueLow;
  sp.a3Total += closeMinusTrueLow;
  sp.b1Total += trueRange;
  sp.b2Total += trueRange;
  sp.b3Total += trueRange;
  /* Once a whole window of no-contribution bars has gone by, every slot it
   * spans is 0.0, so its totals are known to be exactly zero and the
   * residue can be dropped. The periods are sorted shortest-first, so a
   * run long enough for a longer window is long enough for every shorter
   * one.
   */
  live = (!(trueRange + Math.abs(closeMinusTrueLow) <= 0.0) ? 1 : 0);
  sp.nullRun = (sp.nullRun + 1) & (live - 1);
  if (sp.nullRun >= sp.optInTimePeriod1) {
    sp.a1Total = 0.0;
    sp.b1Total = 0.0;
    if (sp.nullRun >= sp.optInTimePeriod2) {
      sp.a2Total = 0.0;
      sp.b2Total = 0.0;
      if (sp.nullRun >= sp.optInTimePeriod3) {
        sp.nullRun = sp.optInTimePeriod3;
        sp.a3Total = 0.0;
        sp.b3Total = 0.0;
      }
    }
  }
  /* Calculate the oscillator value for today. Each window contributes only
   * when it holds a true range; the totals are sums of non-negative terms
   * and the reseed above removes their residue, so the test is exact.
   */
  output = 0.0;
  if (sp.b1Total > 0.0) {
    output += 4.0 * (sp.a1Total / sp.b1Total);
  }
  if (sp.b2Total > 0.0) {
    output += 2.0 * (sp.a2Total / sp.b2Total);
  }
  if (sp.b3Total > 0.0) {
    output += sp.a3Total / sp.b3Total;
  }
  /* Remove the trailing terms to prepare for next day. Each was evaluated
   * once, when its bar entered the ring.
   */
  sp.a1Total -= sp.cb_term_closeMinusTrueLow[sp.trailingPos1];
  sp.b1Total -= sp.cb_term_trueRange[sp.trailingPos1];
  sp.trailingPos1 += 1;
  if (sp.trailingPos1 >= sp.optInTimePeriod3) {
    sp.trailingPos1 = 0;
  }
  sp.a2Total -= sp.cb_term_closeMinusTrueLow[sp.trailingPos2];
  sp.b2Total -= sp.cb_term_trueRange[sp.trailingPos2];
  sp.trailingPos2 += 1;
  if (sp.trailingPos2 >= sp.optInTimePeriod3) {
    sp.trailingPos2 = 0;
  }
  sp.term_Idx = sp.term_Idx + 1;
  if (sp.term_Idx > sp.maxIdx_term) {
    sp.term_Idx = 0;
  }
  sp.a3Total -= sp.cb_term_closeMinusTrueLow[sp.term_Idx];
  sp.b3Total -= sp.cb_term_trueRange[sp.term_Idx];
  /* Last operation is to write the output. Must
   * be done after the trailing index have all been
   * taken care of because the caller is allowed
   * to have the input array to be also the output
   * array.
   */
  sp.cur_outReal = 100.0 * (output / 7.0);
  /* Increment indexes */
  sp.lag1_inClose = inClose;
}

function ultoscOpenImpl(
  sp: UltoscStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod1: number,
  optInTimePeriod2: number,
  optInTimePeriod3: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let a1Total = 0;
  let a2Total = 0;
  let a3Total = 0;
  let b1Total = 0;
  let b2Total = 0;
  let b3Total = 0;
  let trueLow = 0;
  let trueRange = 0;
  let closeMinusTrueLow = 0;
  let tempDouble = 0;
  let output = 0;
  let tempHT = 0;
  let tempLT = 0;
  let tempCY = 0;
  let lookbackTotal = 0;
  let longestPeriod = 0;
  let longestIndex = 0;
  let i = 0;
  let j = 0;
  let today = 0;
  let outIdx = 0;
  let trailingPos1 = 0;
  let trailingPos2 = 0;
  let nullRun = 0;
  let live = 0;
  const usedFlag = new Int32Array(3);
  const periods = new Int32Array(3);
  const sortedPeriods = new Int32Array(3);
  let term_closeMinusTrueLow!: Float64Array;
  let term_trueRange!: Float64Array;
  let term_Idx = 0;
  let maxIdx_term = (32) - 1;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length || inClose.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod1 === Core.INTEGER_DEFAULT) {
    optInTimePeriod1 = 7;
  } else if (optInTimePeriod1 < 1 || optInTimePeriod1 > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod2 === Core.INTEGER_DEFAULT) {
    optInTimePeriod2 = 14;
  } else if (optInTimePeriod2 < 1 || optInTimePeriod2 > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod3 === Core.INTEGER_DEFAULT) {
    optInTimePeriod3 = 28;
  } else if (optInTimePeriod3 < 1 || optInTimePeriod3 > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* The two per-bar terms the three moving sums are built from. Both are a
   * pure function of the bar, so each bar is evaluated once on entry and read
   * back when it leaves each of the three windows.
   */
  /* One entry per bar of the longest window. Stays on the stack for every
   * period up to 32, which covers the 7/14/28 default.
   */
  /* Id, Type, Static Size */
  outBegIdx.value = 0;
  outNBElement.value = 0;
  /* Ensure that the time periods are ordered from shortest to longest.
   * Sort.
   */
  periods[0] = optInTimePeriod1;
  periods[1] = optInTimePeriod2;
  periods[2] = optInTimePeriod3;
  usedFlag[0] = 0;
  usedFlag[1] = 0;
  usedFlag[2] = 0;
  for (i = 0; i < 3; i += 1) {
    longestPeriod = 0;
    longestIndex = 0;
    for (j = 0; j < 3; j += 1) {
      if (usedFlag[j] === 0 && periods[j] > longestPeriod) {
        longestPeriod = periods[j];
        longestIndex = j;
      }
    }
    usedFlag[longestIndex] = 1;
    sortedPeriods[i] = longestPeriod;
  }
  optInTimePeriod1 = sortedPeriods[2];
  optInTimePeriod2 = sortedPeriods[1];
  optInTimePeriod3 = sortedPeriods[0];
  /* Adjust startIdx for lookback period. */
  lookbackTotal = ultoscLookbackPositional(optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (optInTimePeriod3 < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  term_closeMinusTrueLow = new Float64Array(optInTimePeriod3);
  term_trueRange = new Float64Array(optInTimePeriod3);
  maxIdx_term = optInTimePeriod3 - 1;
  term_Idx = 0;
  /* Prime running totals used in moving averages.
   *
   * One pass over the longest warm-up window replaces three overlapping
   * passes. A bar inside the shorter windows is added to those totals as it
   * is reached, so every total still accumulates exactly the same bars in
   * exactly the same ascending order as three separate loops did.
   */
  a1Total = 0;
  b1Total = 0;
  a2Total = 0;
  b2Total = 0;
  a3Total = 0;
  b3Total = 0;
  /* Consecutive bars that put nothing into the windows, counted so that an
   * empty window can be recognized exactly (the shape #244 needed for MFI).
   * The running totals cannot answer that question themselves: they are
   * maintained by add-then-subtract, so once a window empties they hold
   * rounding residue of arbitrary sign rather than zero, and v0.6.4 divides
   * one residue by another there -- it returns -92.9 for an oscillator
   * documented to run 0..100. Both of a bar's terms have to be zero for it to
   * count, which for valid bars is one condition (a zero true range means
   * H == L == the previous close, which leaves the close on the true low).
   * Reseeding on the count is what lets the divides below be guarded exactly
   * rather than against a fixed band -- a true range carries the quote unit,
   * so the band they used to carry zeroed the oscillator for any instrument
   * quoted below it (issue #253).
   */
  nullRun = 0;
  for (i = startIdx - optInTimePeriod3 + 1; i < startIdx; i += 1) {
    tempLT = inLow[i];
    tempHT = inHigh[i];
    tempCY = inClose[i - 1];
    trueLow = (tempLT < tempCY ? tempLT : tempCY);
    closeMinusTrueLow = inClose[i] - trueLow;
    trueRange = tempHT - tempLT;
    tempDouble = Math.abs(tempCY - tempHT);
    if (tempDouble > trueRange) {
      trueRange = tempDouble;
    }
    tempDouble = Math.abs(tempCY - tempLT);
    if (tempDouble > trueRange) {
      trueRange = tempDouble;
    }
    term_closeMinusTrueLow[term_Idx] = closeMinusTrueLow;
    term_trueRange[term_Idx] = trueRange;
    term_Idx++;
    if (term_Idx > maxIdx_term) {
      term_Idx = 0;
    }
    live = (!(trueRange + Math.abs(closeMinusTrueLow) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
    if (i >= startIdx - optInTimePeriod1 + 1) {
      a1Total += closeMinusTrueLow;
      b1Total += trueRange;
    }
    if (i >= startIdx - optInTimePeriod2 + 1) {
      a2Total += closeMinusTrueLow;
      b2Total += trueRange;
    }
    a3Total += closeMinusTrueLow;
    b3Total += trueRange;
  }
  /* Calculate oscillator */
  today = startIdx;
  outIdx = 0;
  /* The warm-up wrote optInTimePeriod3-1 bars, so term_Idx is the slot for
   * `today` and, once advanced past it, the slot of the bar leaving the
   * longest window. The two shorter windows trail it by a fixed offset.
   */
  trailingPos1 = term_Idx + optInTimePeriod3 - optInTimePeriod1 + 1;
  if (trailingPos1 >= optInTimePeriod3) {
    trailingPos1 -= optInTimePeriod3;
  }
  trailingPos2 = term_Idx + optInTimePeriod3 - optInTimePeriod2 + 1;
  if (trailingPos2 >= optInTimePeriod3) {
    trailingPos2 -= optInTimePeriod3;
  }
  while (today <= endIdx) {
    /* Add on today's terms */
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    trueLow = (tempLT < tempCY ? tempLT : tempCY);
    closeMinusTrueLow = inClose[today] - trueLow;
    trueRange = tempHT - tempLT;
    tempDouble = Math.abs(tempCY - tempHT);
    if (tempDouble > trueRange) {
      trueRange = tempDouble;
    }
    tempDouble = Math.abs(tempCY - tempLT);
    if (tempDouble > trueRange) {
      trueRange = tempDouble;
    }
    term_closeMinusTrueLow[term_Idx] = closeMinusTrueLow;
    term_trueRange[term_Idx] = trueRange;
    a1Total += closeMinusTrueLow;
    a2Total += closeMinusTrueLow;
    a3Total += closeMinusTrueLow;
    b1Total += trueRange;
    b2Total += trueRange;
    b3Total += trueRange;
    /* Once a whole window of no-contribution bars has gone by, every slot it
     * spans is 0.0, so its totals are known to be exactly zero and the
     * residue can be dropped. The periods are sorted shortest-first, so a
     * run long enough for a longer window is long enough for every shorter
     * one.
     */
    live = (!(trueRange + Math.abs(closeMinusTrueLow) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
    if (nullRun >= optInTimePeriod1) {
      a1Total = 0.0;
      b1Total = 0.0;
      if (nullRun >= optInTimePeriod2) {
        a2Total = 0.0;
        b2Total = 0.0;
        if (nullRun >= optInTimePeriod3) {
          nullRun = optInTimePeriod3;
          a3Total = 0.0;
          b3Total = 0.0;
        }
      }
    }
    /* Calculate the oscillator value for today. Each window contributes only
     * when it holds a true range; the totals are sums of non-negative terms
     * and the reseed above removes their residue, so the test is exact.
     */
    output = 0.0;
    if (b1Total > 0.0) {
      output += 4.0 * (a1Total / b1Total);
    }
    if (b2Total > 0.0) {
      output += 2.0 * (a2Total / b2Total);
    }
    if (b3Total > 0.0) {
      output += a3Total / b3Total;
    }
    /* Remove the trailing terms to prepare for next day. Each was evaluated
     * once, when its bar entered the ring.
     */
    a1Total -= term_closeMinusTrueLow[trailingPos1];
    b1Total -= term_trueRange[trailingPos1];
    trailingPos1 += 1;
    if (trailingPos1 >= optInTimePeriod3) {
      trailingPos1 = 0;
    }
    a2Total -= term_closeMinusTrueLow[trailingPos2];
    b2Total -= term_trueRange[trailingPos2];
    trailingPos2 += 1;
    if (trailingPos2 >= optInTimePeriod3) {
      trailingPos2 = 0;
    }
    term_Idx++;
    if (term_Idx > maxIdx_term) {
      term_Idx = 0;
    }
    a3Total -= term_closeMinusTrueLow[term_Idx];
    b3Total -= term_trueRange[term_Idx];
    /* Last operation is to write the output. Must
     * be done after the trailing index have all been
     * taken care of because the caller is allowed
     * to have the input array to be also the output
     * array.
     */
    outReal[((outIdx * outStride) | 0)] = 100.0 * (output / 7.0);
    /* Increment indexes */
    outIdx += 1;
    today += 1;
  }
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const capCb_term = maxIdx_term + 1;
  if (capCb_term > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sp.optInTimePeriod1 = optInTimePeriod1;
  sp.optInTimePeriod2 = optInTimePeriod2;
  sp.optInTimePeriod3 = optInTimePeriod3;
  sp.a1Total = a1Total;
  sp.a2Total = a2Total;
  sp.a3Total = a3Total;
  sp.b1Total = b1Total;
  sp.b2Total = b2Total;
  sp.b3Total = b3Total;
  sp.trailingPos1 = trailingPos1;
  sp.trailingPos2 = trailingPos2;
  sp.nullRun = nullRun;
  sp.term_Idx = term_Idx;
  sp.maxIdx_term = maxIdx_term;
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.cbSize_term = capCb_term;
  sp.cb_term_closeMinusTrueLow = term_closeMinusTrueLow;
  sp.cb_term_trueRange = term_trueRange;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function ultoscOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod1: number,
  optInTimePeriod2: number,
  optInTimePeriod3: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): UltoscStream {
  const sp = UltoscStream.of(core);
  const retCode = ultoscOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ULTOSC openAndFill', inHigh.length, startIdx, ultoscLookbackPositional(optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, core));
  }
  if (retCode === RetCode.BAD_PARAM && ultoscLookbackPositional(optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, core) < 0) {
    throw rejectedParams('ULTOSC openAndFill', PARAM_RANGES, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3);
  }
  throw streamFailure('ULTOSC openAndFill', retCode);
}

/** @internal */
export function ultoscOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod1: number,
  optInTimePeriod2: number,
  optInTimePeriod3: number,
): UltoscStream {
  const sp = UltoscStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = ultoscOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ULTOSC open', inHigh.length, startIdx, ultoscLookbackPositional(optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, core));
  }
  if (retCode === RetCode.BAD_PARAM && ultoscLookbackPositional(optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, core) < 0) {
    throw rejectedParams('ULTOSC open', PARAM_RANGES, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3);
  }
  throw streamFailure('ULTOSC open', retCode);
}

/** @internal */
export function ultoscOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod1: number,
  optInTimePeriod2: number,
  optInTimePeriod3: number,
  core: Core = Core.DEFAULT,
): UltoscStream {
  optInTimePeriod1 = requireInt32('ULTOSC open', 'timePeriod1', optInTimePeriod1);
  optInTimePeriod2 = requireInt32('ULTOSC open', 'timePeriod2', optInTimePeriod2);
  optInTimePeriod3 = requireInt32('ULTOSC open', 'timePeriod3', optInTimePeriod3);
  requireArgument('ULTOSC open', 'prices.high', inHigh);
  requireHistory('ULTOSC open', 'prices.high', inHigh.length);
  requireArgument('ULTOSC open', 'prices.low', inLow);
  requireArgument('ULTOSC open', 'prices.close', inClose);
  requireHistoryLength('ULTOSC open', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('ULTOSC open', 'prices.close', inClose.length, inHigh.length);
  return ultoscOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3);
}

/** @internal */
export function ultoscOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod1: number,
  optInTimePeriod2: number,
  optInTimePeriod3: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): UltoscStream {
  optInTimePeriod1 = requireInt32('ULTOSC openAndFill', 'timePeriod1', optInTimePeriod1);
  optInTimePeriod2 = requireInt32('ULTOSC openAndFill', 'timePeriod2', optInTimePeriod2);
  optInTimePeriod3 = requireInt32('ULTOSC openAndFill', 'timePeriod3', optInTimePeriod3);
  requireArgument('ULTOSC openAndFill', 'prices.high', inHigh);
  requireHistory('ULTOSC openAndFill', 'prices.high', inHigh.length);
  requireArgument('ULTOSC openAndFill', 'prices.low', inLow);
  requireArgument('ULTOSC openAndFill', 'prices.close', inClose);
  const guardLookback = ultoscLookbackPositional(optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, core);
  if (guardLookback < 0) {
    throw rejectedParams('ULTOSC openAndFill', PARAM_RANGES, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3);
  }
  const guardOutLen = openFillCount('ULTOSC openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('ULTOSC openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('ULTOSC openAndFill', 'prices.close', inClose.length, inHigh.length);
  requireOutput('ULTOSC openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('ULTOSC openAndFill', 'outReal', 'prices.high');
  }
  if (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('ULTOSC openAndFill', 'outReal', 'prices.low');
  }
  if (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('ULTOSC openAndFill', 'outReal', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outReal, inHighF64)) {
    throw sharedMemory('ULTOSC openAndFill', 'outReal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outReal, inLowF64)) {
    throw sharedMemory('ULTOSC openAndFill', 'outReal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outReal, inCloseF64)) {
    throw sharedMemory('ULTOSC openAndFill', 'outReal', 'prices.close');
  }
  return ultoscOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live ULTOSC stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link ultosc} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar. `close`: Close
 *   price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link UltoscOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `ultoscLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range
 *   or histories of different lengths.
 *
 * @example
 * ```ts
 * import { ultoscOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = ultoscOpen({ high, low, close }, {
 *   timePeriod1: 7,
 *   timePeriod2: 14,
 *   timePeriod3: 28,
 * });
 * const peeked = stream.peek(101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function ultoscOpen(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: UltoscOptions): UltoscStream {
  requireArgument('ULTOSC open', 'prices', prices);
  const o = optionsOf('ULTOSC open', options);
  const optInTimePeriod1 = orDefault(o.timePeriod1, Core.INTEGER_DEFAULT);
  const optInTimePeriod2 = orDefault(o.timePeriod2, Core.INTEGER_DEFAULT);
  const optInTimePeriod3 = orDefault(o.timePeriod3, Core.INTEGER_DEFAULT);
  const core = coreOf('ULTOSC open', o.core);
  return ultoscOpenPositional(prices.high, prices.low, prices.close, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, core);
}

/**
 * {@link ultoscOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link ultosc}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar. `close`: Close
 *   price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link UltoscOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link ultosc} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `ultoscLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { ultoscOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = ultoscOpenAndFill({ high, low, close }, {
 *   timePeriod1: 7,
 *   timePeriod2: 14,
 *   timePeriod3: 28,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function ultoscOpenAndFill(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: UltoscOptions): UltoscResult & { stream: UltoscStream } {
  requireArgument('ULTOSC openAndFill', 'prices', prices);
  const o = optionsOf('ULTOSC openAndFill', options);
  const optInTimePeriod1 = orDefault(o.timePeriod1, Core.INTEGER_DEFAULT);
  const optInTimePeriod2 = orDefault(o.timePeriod2, Core.INTEGER_DEFAULT);
  const optInTimePeriod3 = orDefault(o.timePeriod3, Core.INTEGER_DEFAULT);
  const core = coreOf('ULTOSC openAndFill', o.core);
  const capacity = fillCapacity(ultoscLookbackPositional(optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, core), prices.high, prices.low, prices.close);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = ultoscOpenAndFillPositional(prices.high, prices.low, prices.close, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
