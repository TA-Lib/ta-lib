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
 *  JV       Jesus Viver <324122@cienz.unizar.es>
 *  CC       Claude Code (AI assistant)
 *
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  112400 MF     Template creation.
 *  010503 MF     Fix to always use SMA for the STDDEV (Thanks to JV).
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  070626 MF,CC  Fix #99: realign the middle band when the standard
 *                deviation clamps to a later begIdx than the
 *                (period-independent) MAMA lookback, for
 *                optInTimePeriod >= 34.
 *  071126 MF,CC  Split into an SMA fast path (reuses the moving average as the
 *                mean) and a general MA + STDDEV path, so BBANDS streams as a
 *                composition of the TA_MA and TA_STDDEV streams. Bit-identical.
 *  071626 MF,CC  #117 speed optimization: fuse the SMA fast path's moving
 *                average and standard deviation into a single pass. Bit-identical.
 *  071726 MF,CC  #118 SMA-path deviation now uses the cancellation-free variance
 *                (var.c); two recurrences in one pass. Bit-identical.
 *  072426 MF,CC  Lookback is now max(MA, STDDEV) so it is honest for MA types
 *                whose lookback is below the deviation's (MAMA >= 34, and
 *                TA_MAType_DISABLED); required for streaming (issue #93).
 *  082326 MF,CC  #243 the SMA path's TA_EPSILON test on the variance is replaced
 *                by var.c's scale-relative reseed floor; the square root is
 *                unconditional. Bands no longer collapse on a fine tick.
 *  092226 MF,CC  #434 the SMA path's variance step follows var.c.
 */

import { MAType, MATypes } from '../enums.ts';
import { badSink, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireReal, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { MaStream, maLookbackPositional, maOpenAndFillInternal, maPositional } from './ma.ts';
import { StddevStream, stddevLookbackPositional, stddevOpenAndFillInternal, stddevPositional } from './stddev.ts';

/** @internal */
export function bbandsLookbackPositional(optInTimePeriod: number, optInNbDevUp: number, optInNbDevDn: number, optInMAType: MAType | typeof Core.INTEGER_DEFAULT, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (typeof optInNbDevUp !== 'number') {
    return -1;
  }
  if (typeof optInNbDevDn !== 'number') {
    return -1;
  }
  if ((optInMAType | 0) !== optInMAType) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  if (optInNbDevUp === Core.REAL_DEFAULT) {
    optInNbDevUp = 2.0;
  } else if (!(optInNbDevUp >= Core.REAL_MIN && optInNbDevUp <= Core.REAL_MAX)) {
    return -1;
  }
  if (optInNbDevDn === Core.REAL_DEFAULT) {
    optInNbDevDn = 2.0;
  } else if (!(optInNbDevDn >= Core.REAL_MIN && optInNbDevDn <= Core.REAL_MAX)) {
    return -1;
  }
  if (optInMAType === Core.INTEGER_DEFAULT || optInMAType === MAType.DEFAULT) {
    optInMAType = MAType.SMA;
  } else if (optInMAType < MATypes.Min || optInMAType > MATypes.Max) {
    return -1;
  }
  let maLookback = 0;
  let stddevLookback = 0;
  /* A band value needs BOTH the middle-band moving average and the standard
   * deviation of the outer bands, so the first valid output is the later of the
   * two lookbacks. For every MA type whose lookback is at least
   * optInTimePeriod-1 this equals the MA lookback (unchanged); it rises above it
   * only when the MA lookback is the smaller one - TA_MAType_MAMA at
   * optInTimePeriod >= 34 (constant MAMA lookback 32) and TA_MAType_DISABLED
   * (MA lookback 0, issue #93). Reporting the honest value keeps
   * outBegIdx == lookback: issue #99 kept it at the MA lookback, which
   * under-reported those cases (outBegIdx > lookback) and broke streaming, whose
   * Open is tied to lookback+1. The middle band still begins at the MA's earlier
   * begIdx internally and is realigned to this later bar in bbands() below.
   */
  maLookback = maLookbackPositional(optInTimePeriod, (optInMAType as MAType), core);
  stddevLookback = stddevLookbackPositional(optInTimePeriod, 1.0, core);
  return (maLookback > stddevLookback) ? maLookback : stddevLookback;
}

/** @internal */
export function bbandsDisplayShiftPositional(optInTimePeriod: number, optInNbDevUp: number, optInNbDevDn: number, optInMAType: MAType | typeof Core.INTEGER_DEFAULT, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (bbandsLookbackPositional(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 3) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function bbandsImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  optInNbDevUp: number,
  optInNbDevDn: number,
  optInMAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
): RetCode {
  let retCode: RetCode = RetCode.SUCCESS;
  let i = 0;
  let maBegIdx = 0;
  let shiftIdx = 0;
  let tempReal = 0;
  let tempReal2 = 0;
  let tempBuffer1!: Float64Array;
  let tempBuffer2!: Float64Array;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInNbDevUp === Core.REAL_DEFAULT) {
    optInNbDevUp = 2.0;
  } else if (!(optInNbDevUp >= Core.REAL_MIN && optInNbDevUp <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInNbDevDn === Core.REAL_DEFAULT) {
    optInNbDevDn = 2.0;
  } else if (!(optInNbDevDn >= Core.REAL_MIN && optInNbDevDn <= Core.REAL_MAX)) {
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
  if (optInMAType === MAType.SMA) {
    /* SMA fast path: the middle band (SMA) and the standard deviation share one
     * pass over the window below. Bit-identical to the general MA + STDDEV path
     * (which the stream composes for every MA type).
     *
     * Identify TWO temporary buffers among the outputs so the calculation
     * needs no memory allocation; whenever possible make tempBuffer1 be the
     * middle band output, saving one copy operation.
     */
    if ((inReal.buffer === outRealUpperBand.buffer && inReal.byteOffset === outRealUpperBand.byteOffset)) {
      tempBuffer1 = outRealMiddleBand;
      tempBuffer2 = outRealLowerBand;
    } else if ((inReal.buffer === outRealLowerBand.buffer && inReal.byteOffset === outRealLowerBand.byteOffset)) {
      tempBuffer1 = outRealMiddleBand;
      tempBuffer2 = outRealUpperBand;
    } else if ((inReal.buffer === outRealMiddleBand.buffer && inReal.byteOffset === outRealMiddleBand.byteOffset)) {
      tempBuffer1 = outRealLowerBand;
      tempBuffer2 = outRealUpperBand;
    } else {
      tempBuffer1 = outRealMiddleBand;
      tempBuffer2 = outRealUpperBand;
    }
    /* Check that the caller is not doing tricky things.
     * (like using the input buffer in two output!)
     */
    if ((tempBuffer1.buffer === inReal.buffer && tempBuffer1.byteOffset === inReal.byteOffset) || (tempBuffer2.buffer === inReal.buffer && tempBuffer2.byteOffset === inReal.byteOffset)) {
      return RetCode.BAD_PARAM;
    }
    /* One pass with two independent recurrences: the SMA running sum (maTotal,
     * mean -> tempBuffer1, bit-identical to TA_MA(SMA)) and the shifted-data
     * variance (-> tempBuffer2, bit-identical to TA_STDDEV/TA_VAR - see var.c).
     * The variance carries its own shift and reseed; the SMA sum is untouched by
     * it. tempBuffer1/2 never alias inReal (checked above).
     */
    let maTotal = 0;
    let shift = 0;
    let varTotal1 = 0;
    let varTotal2 = 0;
    let meanValue1 = 0;
    let variance = 0;
    let _invPeriod = 0;
    let _tempReal = 0;
    let _peakTotal2 = 0;
    let _i = 0;
    let _j = 0;
    let _outIdx = 0;
    let _trailingIdx = 0;
    let _windowStart = 0;
    let _lookbackTotal = 0;
    let _barsSinceReseed = 0;
    _lookbackTotal = optInTimePeriod - 1;
    if (startIdx < _lookbackTotal) {
      startIdx = _lookbackTotal;
    }
    if (startIdx > endIdx) {
      outBegIdx.value = 0;
      outNBElement.value = 0;
      return RetCode.SUCCESS;
    }
    _invPeriod = 1.0 / optInTimePeriod;
    _trailingIdx = startIdx - _lookbackTotal;
    shift = inReal[_trailingIdx];
    maTotal = 0.0;
    varTotal1 = 0.0;
    varTotal2 = 0.0;
    for (_j = _trailingIdx; _j < startIdx; _j += 1) {
      maTotal += inReal[_j];
      _tempReal = inReal[_j] - shift;
      varTotal1 += _tempReal;
      _tempReal *= _tempReal;
      varTotal2 += _tempReal;
    }
    _i = startIdx;
    _outIdx = 0;
    _barsSinceReseed = ((32 * optInTimePeriod) | 0);
    _peakTotal2 = varTotal2;
    do {
      maTotal += inReal[_i];
      _tempReal = inReal[_i] - shift;
      varTotal1 += _tempReal;
      _tempReal *= _tempReal;
      varTotal2 += _tempReal;
      _peakTotal2 = (varTotal2 > _peakTotal2) ? varTotal2 : _peakTotal2;
      meanValue1 = varTotal1 * _invPeriod;
      variance = varTotal2 * _invPeriod - meanValue1 * meanValue1;
      tempBuffer1[_outIdx] = maTotal / optInTimePeriod;
      maTotal -= inReal[_trailingIdx];
      _tempReal = inReal[_trailingIdx] - shift;
      varTotal1 -= _tempReal;
      _tempReal *= _tempReal;
      varTotal2 -= _tempReal;
      _trailingIdx += 1;
      _barsSinceReseed -= 1;
      if (variance < 1e-6 * (_peakTotal2 * _invPeriod) || _barsSinceReseed <= 0) {
        _barsSinceReseed = ((32 * optInTimePeriod) | 0);
        _windowStart = _i - _lookbackTotal;
        _tempReal = 0.0;
        for (_j = _windowStart; _j <= _i; _j += 1) {
          _tempReal += inReal[_j];
        }
        shift = _tempReal * _invPeriod;
        varTotal1 = 0.0;
        varTotal2 = 0.0;
        for (_j = _windowStart; _j <= _i; _j += 1) {
          _tempReal = inReal[_j] - shift;
          varTotal1 += _tempReal;
          _tempReal *= _tempReal;
          varTotal2 += _tempReal;
        }
        meanValue1 = varTotal1 * _invPeriod;
        variance = varTotal2 * _invPeriod - meanValue1 * meanValue1;
        if (variance < 1e-6 * (varTotal2 * _invPeriod)) {
          shift = inReal[_i];
          varTotal1 = 0.0;
          varTotal2 = 0.0;
          for (_j = _windowStart; _j <= _i; _j += 1) {
            _tempReal = inReal[_j] - shift;
            varTotal1 += _tempReal;
            _tempReal *= _tempReal;
            varTotal2 += _tempReal;
          }
          meanValue1 = varTotal1 * _invPeriod;
          variance = varTotal2 * _invPeriod - meanValue1 * meanValue1;
        }
        _peakTotal2 = varTotal2;
        /* The floor from var.c, verbatim: it owns the sign, so the
         * square root below can be unconditional.
         */
        if (variance < 1e-12 * (varTotal2 * _invPeriod)) {
          variance = 0.0;
        }
        _tempReal = inReal[_windowStart] - shift;
        varTotal1 -= _tempReal;
        _tempReal *= _tempReal;
        varTotal2 -= _tempReal;
      }
      /* The TA_EPSILON test that used to stand here compared a SQUARED
       * quantity to a fixed 1e-14 and flattened all three bands onto each
       * other for any finely quoted series (#243). What replaces it skips
       * the root ONLY where the answer is already known, because the
       * rebuild above has made it exactly 0 -- worth doing because
       * this root, unlike stddev.c's, sits in the fused loop with a
       * carried dependency and cannot vectorize, so running it on flat
       * input cost 1.59x.
       *
       * `!= 0.0` and not `> 0.0`: the two differ only on NaN, and `> 0.0`
       * sends it to the false arm, which would emit a zero-width band
       * where stddev.c, BBANDS' own non-SMA paths and BBANDS_Open all
       * return NaN. Reaching it needs a squared deviation to overflow
       * (|x| ~ 1.3e154, far outside TA_REAL_MAX), so nothing in the
       * declared domain can tell the two apart -- but there is no reason
       * to buy the disagreement, and `!= 0.0` is what this is for.
       */
      tempBuffer2[_outIdx] = (variance !== 0.0) ? Math.sqrt(variance) : 0.0;
      _outIdx += 1;
      _i += 1;
    } while (_i <= endIdx);
    outNBElement.value = _outIdx;
    outBegIdx.value = startIdx;
    /* Copy the MA calculation into the middle band ouput, unless
     * the calculation was done into it already!
     */
    if (!(tempBuffer1.buffer === outRealMiddleBand.buffer && tempBuffer1.byteOffset === outRealMiddleBand.byteOffset)) {
      outRealMiddleBand.set(tempBuffer1.subarray(0, outNBElement.value), 0);
    }
    /* Now do a tight loop to calculate the upper/lower band at the same time. */
    if (optInNbDevUp === optInNbDevDn) {
      for (i = 0; i < outNBElement.value; i += 1) {
        tempReal = tempBuffer2[i] * optInNbDevUp;
        tempReal2 = outRealMiddleBand[i];
        outRealUpperBand[i] = tempReal2 + tempReal;
        outRealLowerBand[i] = tempReal2 - tempReal;
      }
    } else {
      for (i = 0; i < outNBElement.value; i += 1) {
        tempReal = tempBuffer2[i];
        tempReal2 = outRealMiddleBand[i];
        outRealUpperBand[i] = fma(tempReal, optInNbDevUp, tempReal2);
        outRealLowerBand[i] = tempReal2 - tempReal * optInNbDevDn;
      }
    }
    return RetCode.SUCCESS;
  }
  /* General path (every MA type other than SMA): the middle band is the moving
   * average and the deviation is the standard deviation of the input, combined
   * at the same bar. Two intermediate buffers are allocated so the input may
   * safely alias an output (it is only read here).
   */
  /* Nothing to produce: the range ends before the lookback. Return before
   * touching anything.
   *
   * Without this the moving average below runs first, and for the MA types whose
   * lookback is BELOW the deviation's - TA_MAType_DISABLED (MA lookback 0) and
   * TA_MAType_MAMA at optInTimePeriod >= 34 - it reads the whole range and
   * computes a middle band the empty standard deviation then discards.
   * Observably identical (the empty deviation already yields 0,0 here), but it
   * is the difference between "a range that ends before the lookback reads nothing"
   * being true of this function and being false: with a caller-supplied inReal
   * that stops short of endIdx, that discarded work is an out-of-bounds read.
   * The SMA fast path above needs no such guard - its own lookback IS the
   * deviation's, so its clamp already covers it. Pinned by the zero-length
   * no-I/O probe over every guarded core, at every MA type.
   */
  if (bbandsLookbackPositional(optInTimePeriod, optInNbDevUp, optInNbDevDn, (optInMAType as MAType), core) > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  tempBuffer1 = new Float64Array(endIdx - startIdx + 1);
  tempBuffer2 = new Float64Array(endIdx - startIdx + 1);
  /* Calculate the middle band moving average. */
  const _xr0 = maPositional(startIdx, endIdx, inReal, optInTimePeriod, (optInMAType as MAType), tempBuffer1, core);
  outBegIdx.value = _xr0.begIdx;
  outNBElement.value = _xr0.count;
  retCode = RetCode.SUCCESS;
  if (outNBElement.value === 0) {
    outNBElement.value = 0;
    return retCode;
  }
  /* Remember where the moving average begins, to realign it below. */
  maBegIdx = outBegIdx.value;
  /* Calculate the Standard Deviation into tempBuffer2. */
  const _xr1 = stddevPositional(outBegIdx.value, endIdx, inReal, optInTimePeriod, 1.0, tempBuffer2, core);
  outBegIdx.value = _xr1.begIdx;
  outNBElement.value = _xr1.count;
  retCode = RetCode.SUCCESS;
  /* When the standard deviation (lookback optInTimePeriod-1) clamps to a later
   * begIdx than the moving average did - as with TA_MAType_MAMA (constant
   * lookback 32) and optInTimePeriod >= 34 - the MA in tempBuffer1 still starts
   * at the earlier maBegIdx. Copy it forward from that shift into the middle
   * band so each band value pairs the moving average and standard deviation of
   * the same bar. The guarded subtraction keeps shiftIdx non-negative even when
   * the standard deviation produced no output (an empty range leaves *outBegIdx
   * at 0), which the unconditional copy below then handles as a zero-length move.
   */
  if (outBegIdx.value > maBegIdx) {
    shiftIdx = outBegIdx.value - maBegIdx;
  } else {
    shiftIdx = 0;
  }
  outRealMiddleBand.set(tempBuffer1.subarray(shiftIdx, shiftIdx + outNBElement.value), 0);
  /* Now do a tight loop to calculate the upper/lower band at the same time. */
  if (optInNbDevUp === optInNbDevDn) {
    for (i = 0; i < outNBElement.value; i += 1) {
      tempReal = tempBuffer2[i] * optInNbDevUp;
      tempReal2 = outRealMiddleBand[i];
      outRealUpperBand[i] = tempReal2 + tempReal;
      outRealLowerBand[i] = tempReal2 - tempReal;
    }
  } else {
    for (i = 0; i < outNBElement.value; i += 1) {
      tempReal2 = outRealMiddleBand[i];
      outRealUpperBand[i] = fma(tempBuffer2[i], optInNbDevUp, tempReal2);
      outRealLowerBand[i] = tempReal2 - tempBuffer2[i] * optInNbDevDn;
    }
  }
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['nbDevUp', Core.REAL_DEFAULT, Core.REAL_MIN, Core.REAL_MAX, 'in -3e37..3e37'], ['nbDevDn', Core.REAL_DEFAULT, Core.REAL_MIN, Core.REAL_MAX, 'in -3e37..3e37'], ['maType', Core.INTEGER_DEFAULT, MATypes.Min, MATypes.Max, 'a MAType']] as const;

/** @internal */
export function bbandsPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInNbDevUp: number,
  optInNbDevDn: number,
  optInMAType: MAType | typeof Core.INTEGER_DEFAULT,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('BBANDS', 'startIdx', startIdx);
  endIdx = requireInt32('BBANDS', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('BBANDS', 'timePeriod', optInTimePeriod);
  optInNbDevUp = requireReal('BBANDS', 'nbDevUp', optInNbDevUp);
  optInNbDevDn = requireReal('BBANDS', 'nbDevDn', optInNbDevDn);
  optInMAType = requireInt32('BBANDS', 'maType', optInMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireIndexRange('BBANDS', startIdx, endIdx);
  const guardLookback = bbandsLookbackPositional(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, core);
  if (guardLookback < 0) {
    throw rejectedParams('BBANDS', PARAM_RANGES, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType);
  }
  const guardStart = clampedStart('BBANDS', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('BBANDS', 'real', inReal, guardInLen);
  requireOutput('BBANDS', 'outUpperBand', outRealUpperBand, guardOutLen, Float64Array);
  requireOutput('BBANDS', 'outMiddleBand', outRealMiddleBand, guardOutLen, Float64Array);
  requireOutput('BBANDS', 'outLowerBand', outRealLowerBand, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outRealUpperBand, inRealF64)) {
    throw sharedMemory('BBANDS', 'outUpperBand', 'real');
  }
  if (overlapsPartly(outRealMiddleBand, inRealF64)) {
    throw sharedMemory('BBANDS', 'outMiddleBand', 'real');
  }
  if (overlapsPartly(outRealLowerBand, inRealF64)) {
    throw sharedMemory('BBANDS', 'outLowerBand', 'real');
  }
  if (overlaps(outRealUpperBand, outRealMiddleBand)) {
    throw sharedMemory('BBANDS', 'outUpperBand', 'outMiddleBand');
  }
  if (overlaps(outRealUpperBand, outRealLowerBand)) {
    throw sharedMemory('BBANDS', 'outUpperBand', 'outLowerBand');
  }
  if (overlaps(outRealMiddleBand, outRealLowerBand)) {
    throw sharedMemory('BBANDS', 'outMiddleBand', 'outLowerBand');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = bbandsImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('BBANDS', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link bbands}, {@link bbandsLookback}, {@link bbandsDisplayShift},
 * {@link bbandsOpen} and {@link bbandsOpenAndFill}. Every key is optional: an
 * omitted or `undefined` key takes its default, and a call ignores the keys it has
 * no use for.
 */
export interface BbandsOptions {
  /**
   * Periods for the MA and standard deviation (default 20; range 2 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  timePeriod?: number | undefined;
  /**
   * Standard-deviation multiplier for the upper band (default 2; `Core.REAL_DEFAULT`
   * selects the default).
   */
  nbDevUp?: number | undefined;
  /**
   * Standard-deviation multiplier for the lower band (default 2; `Core.REAL_DEFAULT`
   * selects the default).
   */
  nbDevDn?: number | undefined;
  /**
   * Moving-average type for the middle band (default `MAType.SMA`; values: 0=SMA,
   * 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA, 10=DISABLED,
   * 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA; `Core.INTEGER_DEFAULT` or
   * `MAType.DEFAULT` selects the default).
   */
  maType?: MAType | typeof Core.INTEGER_DEFAULT | undefined;
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

/** What {@link bbands} returns, and {@link bbandsOpenAndFill} with its stream. */
export interface BbandsResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Middle band plus nbDevUp standard deviations. `outUpperBand[k]` is the value at
   * input bar `begIdx + k`.
   */
  outUpperBand: Float64Array<ArrayBuffer>;
  /** The moving average. `outMiddleBand[k]` is the value at input bar `begIdx + k`. */
  outMiddleBand: Float64Array<ArrayBuffer>;
  /**
   * Middle band minus nbDevDn standard deviations. `outLowerBand[k]` is the value at
   * input bar `begIdx + k`.
   */
  outLowerBand: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link bbands} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link BbandsOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function bbandsLookback(options?: BbandsOptions): number {
  const o = optionsOf('BBANDS lookback', options);
  return bbandsLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.nbDevUp, Core.REAL_DEFAULT), orDefault(o.nbDevDn, Core.REAL_DEFAULT), orDefault(o.maType, Core.INTEGER_DEFAULT), coreOf('BBANDS lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link bbands}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link BbandsOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function bbandsDisplayShift(outputIdx: number, options?: BbandsOptions): number {
  const o = optionsOf('BBANDS displayShift', options);
  return bbandsDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.nbDevUp, Core.REAL_DEFAULT), orDefault(o.nbDevDn, Core.REAL_DEFAULT), orDefault(o.maType, Core.INTEGER_DEFAULT), outputIdx, coreOf('BBANDS displayShift', o.core));
}

/**
 * Bollinger Bands: a moving-average middle band with upper and lower bands offset
 * by a multiple of the standard deviation. Used to gauge relative price
 * volatility.
 *
 * Formula and more info at
 * [ta-lib.org/functions/bbands](https://ta-lib.org/functions/bbands).
 *
 * `outUpperBand[k]` is the value at input bar `begIdx + k`; every output is
 * exactly as long as the number of values written, and nothing is padded. A range
 * that ends before {@link bbandsLookback} is a success with empty outputs.
 *
 * @param real - Input data series.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link BbandsOptions}.
 * @returns `begIdx` and the values; see {@link BbandsResult}. Each output is a
 *   view of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, an integer parameter that is not an int32, a real parameter that is not a
 *   number, a parameter outside its range, an input too short for the range, a
 *   buffer that is empty, too short or of the wrong type, two buffers sharing memory
 *   or a buffer sharing memory with a `Float64Array` input it is not. Checked before
 *   anything is written.
 *
 * @example
 * ```ts
 * import { MAType, bbands } from 'ta-lib';
 *
 * const series = new Float64Array(252); // your data, oldest first
 *
 * const {
 *   begIdx,
 *   outUpperBand,
 *   outMiddleBand,
 *   outLowerBand,
 * } = bbands(series, {
 *   timePeriod: 20,
 *   nbDevUp: 2,
 *   nbDevDn: 2,
 *   maType: MAType.SMA,
 * });
 * console.log(outUpperBand.length + ' values from bar ' + begIdx + '; last outUpperBand = ' + outUpperBand[outUpperBand.length - 1]);
 * ```
 * @see [`ma`](https://ta-lib.org/functions/ma)
 * @see [`stddev`](https://ta-lib.org/functions/stddev)
 * @see [`sma`](https://ta-lib.org/functions/sma)
 */
export function bbands(real: ArrayLike<number>, options?: BbandsOptions): BbandsResult {
  const o = optionsOf('BBANDS', options);
  if (o.endIdx === undefined) {
    requireSeries('BBANDS', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInNbDevUp = orDefault(o.nbDevUp, Core.REAL_DEFAULT);
  const optInNbDevDn = orDefault(o.nbDevDn, Core.REAL_DEFAULT);
  const optInMAType = orDefault(o.maType, Core.INTEGER_DEFAULT);
  const core = coreOf('BBANDS', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, bbandsLookbackPositional(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, core), real);
  const outRealUpperBand = o.outUpperBand === undefined ? new Float64Array(capacity) : o.outUpperBand;
  const outRealMiddleBand = o.outMiddleBand === undefined ? new Float64Array(capacity) : o.outMiddleBand;
  const outRealLowerBand = o.outLowerBand === undefined ? new Float64Array(capacity) : o.outLowerBand;
  const range = bbandsPositional(startIdx, endIdx, real, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, outRealUpperBand, outRealMiddleBand, outRealLowerBand, core);
  return { begIdx: range.begIdx, outUpperBand: outRealUpperBand.subarray(0, range.count), outMiddleBand: outRealMiddleBand.subarray(0, range.count), outLowerBand: outRealLowerBand.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live BBANDS stream: one value per closed bar, bit-identical to {@link bbands}
 * over the same series. Open it with {@link bbandsOpen} or
 * {@link bbandsOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/bbands](https://ta-lib.org/functions/bbands).
 */
export class BbandsStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  optInNbDevUp: number = 0;
  /** @internal */
  optInNbDevDn: number = 0;
  /** @internal */
  optInMAType: number = 0;
  /** @internal */
  cur_outRealUpperBand: number = 0;
  /** @internal */
  cur_outRealMiddleBand: number = 0;
  /** @internal */
  cur_outRealLowerBand: number = 0;
  /** @internal */
  sub0!: MaStream;
  /** @internal */
  sub1!: StddevStream;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): BbandsStream {
    return new BbandsStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link bbands} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('BBANDS advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns its outputs.
   *
   * @param real - The bar's `real` value: Input data series.
   * @param out - Receives the outputs and is returned; a new {@link BbandsOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included, before anything moves.
   *   `TALibError` with code `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached
   *   bar `Core.INDEX_MAX`.
   */
  update(real: number, out?: BbandsOut): BbandsOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('BBANDS update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('BBANDS update', 'BbandsOut');
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('BBANDS update', 'real');
    }
    bbandsStepImpl(this, real);
    this.outRangeCount++;
    if (out === undefined) {
      out = new BbandsOut();
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
   * @param out - Receives the outputs and is returned; a new {@link BbandsOut} when
   *   omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included.
   */
  peek(real: number, out?: BbandsOut): BbandsOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('BBANDS peek', 'BbandsOut');
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('BBANDS peek', 'real');
    }
    const sp = this;
    const inReal = real;
    let tempReal = 0;
    let tempReal2 = 0;
    let cur_tempBuffer1 = 0;
    let cur_tempBuffer2 = 0;
    let cur_outRealUpperBand = 0;
    let cur_outRealLowerBand = 0;
    let cur_outRealMiddleBand = 0;
    cur_tempBuffer1 = sp.sub0.peek(inReal);
    cur_tempBuffer2 = sp.sub1.peek(inReal);
    if (sp.optInNbDevUp === sp.optInNbDevDn) {
      tempReal = cur_tempBuffer2 * sp.optInNbDevUp;
      tempReal2 = cur_tempBuffer1;
      cur_outRealUpperBand = tempReal2 + tempReal;
      cur_outRealLowerBand = tempReal2 - tempReal;
    } else {
      tempReal2 = cur_tempBuffer1;
      cur_outRealUpperBand = fma(cur_tempBuffer2, sp.optInNbDevUp, tempReal2);
      cur_outRealLowerBand = tempReal2 - cur_tempBuffer2 * sp.optInNbDevDn;
    }
    cur_outRealMiddleBand = cur_tempBuffer1;
    if (out === undefined) {
      out = new BbandsOut();
    }
    out.outUpperBand = cur_outRealUpperBand;
    out.outMiddleBand = cur_outRealMiddleBand;
    out.outLowerBand = cur_outRealLowerBand;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link BbandsOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a sink that is not an object,
   *   `null` included.
   */
  value(out?: BbandsOut): BbandsOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('BBANDS value', 'BbandsOut');
    }
    if (out === undefined) {
      out = new BbandsOut();
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
  clone(): BbandsStream {
    const c = new BbandsStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.optInNbDevUp = this.optInNbDevUp;
    c.optInNbDevDn = this.optInNbDevDn;
    c.optInMAType = this.optInMAType;
    c.cur_outRealUpperBand = this.cur_outRealUpperBand;
    c.cur_outRealMiddleBand = this.cur_outRealMiddleBand;
    c.cur_outRealLowerBand = this.cur_outRealLowerBand;
    c.sub0 = this.sub0.clone();
    c.sub1 = this.sub1.clone();
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one BBANDS bar, as `update`, `peek` and `value` return them.
 * Passed back to one of them, it is filled and returned again, with no allocation;
 * it holds its values until the next call that writes it. A new one holds `NaN`.
 */
export class BbandsOut {
  /** Middle band plus nbDevUp standard deviations. */
  outUpperBand: number = NaN;
  /** The moving average. */
  outMiddleBand: number = NaN;
  /** Middle band minus nbDevDn standard deviations. */
  outLowerBand: number = NaN;
}

function bbandsStepImpl(sp: BbandsStream, inReal: number): void {
  let tempReal = 0;
  let tempReal2 = 0;
  let cur_tempBuffer1 = 0;
  let cur_tempBuffer2 = 0;
  let cur_outRealUpperBand = 0;
  let cur_outRealLowerBand = 0;
  cur_tempBuffer1 = sp.sub0.update(inReal);
  cur_tempBuffer2 = sp.sub1.update(inReal);
  if (sp.optInNbDevUp === sp.optInNbDevDn) {
    tempReal = cur_tempBuffer2 * sp.optInNbDevUp;
    tempReal2 = cur_tempBuffer1;
    cur_outRealUpperBand = tempReal2 + tempReal;
    cur_outRealLowerBand = tempReal2 - tempReal;
  } else {
    tempReal2 = cur_tempBuffer1;
    cur_outRealUpperBand = fma(cur_tempBuffer2, sp.optInNbDevUp, tempReal2);
    cur_outRealLowerBand = tempReal2 - cur_tempBuffer2 * sp.optInNbDevDn;
  }
  sp.cur_outRealUpperBand = cur_outRealUpperBand;
  sp.cur_outRealMiddleBand = cur_tempBuffer1;
  sp.cur_outRealLowerBand = cur_outRealLowerBand;
}

function bbandsOpenImpl(
  sp: BbandsStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInNbDevUp: number,
  optInNbDevDn: number,
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
  let maBegIdx = 0;
  let shiftIdx = 0;
  let tempReal = 0;
  let tempReal2 = 0;
  let tempBuffer1!: Float64Array;
  let tempBuffer2!: Float64Array;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInNbDevUp === Core.REAL_DEFAULT) {
    optInNbDevUp = 2.0;
  } else if (!(optInNbDevUp >= Core.REAL_MIN && optInNbDevUp <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInNbDevDn === Core.REAL_DEFAULT) {
    optInNbDevDn = 2.0;
  } else if (!(optInNbDevDn >= Core.REAL_MIN && optInNbDevDn <= Core.REAL_MAX)) {
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
  if (historyLen < bbandsLookbackPositional(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType as MAType, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const sc_outRealUpperBand = outStride === 1 ? outRealUpperBand : new Float64Array(historyLen);
  const sc_outRealMiddleBand = outStride === 1 ? outRealMiddleBand : new Float64Array(historyLen);
  const sc_outRealLowerBand = outStride === 1 ? outRealLowerBand : new Float64Array(historyLen);
  let lagNb0 = 0;
  /* General path (every MA type other than SMA): the middle band is the moving
   * average and the deviation is the standard deviation of the input, combined
   * at the same bar. Two intermediate buffers are allocated so the input may
   * safely alias an output (it is only read here).
   */
  /* Nothing to produce: the range ends before the lookback. Return before
   * touching anything.
   *
   * Without this the moving average below runs first, and for the MA types whose
   * lookback is BELOW the deviation's - TA_MAType_DISABLED (MA lookback 0) and
   * TA_MAType_MAMA at optInTimePeriod >= 34 - it reads the whole range and
   * computes a middle band the empty standard deviation then discards.
   * Observably identical (the empty deviation already yields 0,0 here), but it
   * is the difference between "a range that ends before the lookback reads nothing"
   * being true of this function and being false: with a caller-supplied inReal
   * that stops short of endIdx, that discarded work is an out-of-bounds read.
   * The SMA fast path above needs no such guard - its own lookback IS the
   * deviation's, so its clamp already covers it. Pinned by the zero-length
   * no-I/O probe over every guarded core, at every MA type.
   */
  if (bbandsLookbackPositional(optInTimePeriod, optInNbDevUp, optInNbDevDn, (optInMAType as MAType), core) > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  tempBuffer1 = new Float64Array(endIdx - startIdx + 1);
  tempBuffer2 = new Float64Array(endIdx - startIdx + 1);
  /* Calculate the middle band moving average. */
  const sub0 = maOpenAndFillInternal(core, inReal, startIdx, optInTimePeriod, (optInMAType as MAType), outBegIdx, outNBElement, tempBuffer1);
  if (outNBElement.value === 0) {
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  lagNb0 = outNBElement.value;
  /* Remember where the moving average begins, to realign it below. */
  maBegIdx = outBegIdx.value;
  /* Calculate the Standard Deviation into tempBuffer2. */
  const sub1 = stddevOpenAndFillInternal(core, inReal, outBegIdx.value, optInTimePeriod, 1.0, outBegIdx, outNBElement, tempBuffer2);
  /* When the standard deviation (lookback optInTimePeriod-1) clamps to a later
   * begIdx than the moving average did - as with TA_MAType_MAMA (constant
   * lookback 32) and optInTimePeriod >= 34 - the MA in tempBuffer1 still starts
   * at the earlier maBegIdx. Copy it forward from that shift into the middle
   * band so each band value pairs the moving average and standard deviation of
   * the same bar. The guarded subtraction keeps shiftIdx non-negative even when
   * the standard deviation produced no output (an empty range leaves *outBegIdx
   * at 0), which the unconditional copy below then handles as a zero-length move.
   */
  if (outBegIdx.value > maBegIdx) {
    shiftIdx = outBegIdx.value - maBegIdx;
  } else {
    shiftIdx = 0;
  }
  if (outNBElement.value < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (lagNb0 - shiftIdx - outNBElement.value !== 0) {
    return RetCode.INTERNAL_ERROR;
  }
  sc_outRealMiddleBand.set(tempBuffer1.subarray(shiftIdx, shiftIdx + outNBElement.value), 0);
  /* Now do a tight loop to calculate the upper/lower band at the same time. */
  if (optInNbDevUp === optInNbDevDn) {
    for (i = 0; i < outNBElement.value; i += 1) {
      tempReal = tempBuffer2[i] * optInNbDevUp;
      tempReal2 = sc_outRealMiddleBand[i];
      sc_outRealUpperBand[i] = tempReal2 + tempReal;
      sc_outRealLowerBand[i] = tempReal2 - tempReal;
    }
  } else {
    for (i = 0; i < outNBElement.value; i += 1) {
      tempReal2 = sc_outRealMiddleBand[i];
      sc_outRealUpperBand[i] = fma(tempBuffer2[i], optInNbDevUp, tempReal2);
      sc_outRealLowerBand[i] = tempReal2 - tempBuffer2[i] * optInNbDevDn;
    }
  }
  if (outNBElement.value < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.optInNbDevUp = optInNbDevUp;
  sp.optInNbDevDn = optInNbDevDn;
  sp.optInMAType = optInMAType;
  sp.sub0 = sub0;
  sp.sub1 = sub1;
  sp.cur_outRealUpperBand = sc_outRealUpperBand[outNBElement.value - 1];
  sp.cur_outRealMiddleBand = sc_outRealMiddleBand[outNBElement.value - 1];
  sp.cur_outRealLowerBand = sc_outRealLowerBand[outNBElement.value - 1];
  return RetCode.SUCCESS;
}

/** @internal */
export function bbandsOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInNbDevUp: number,
  optInNbDevDn: number,
  optInMAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
): BbandsStream {
  const sp = BbandsStream.of(core);
  const retCode = bbandsOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('BBANDS openAndFill', inReal.length, startIdx, bbandsLookbackPositional(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType as MAType, core));
  }
  if (retCode === RetCode.BAD_PARAM && bbandsLookbackPositional(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType as MAType, core) < 0) {
    throw rejectedParams('BBANDS openAndFill', PARAM_RANGES, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType);
  }
  throw streamFailure('BBANDS openAndFill', retCode);
}

/** @internal */
export function bbandsOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInNbDevUp: number,
  optInNbDevDn: number,
  optInMAType: number,
): BbandsStream {
  const sp = BbandsStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outRealUpperBand = new Float64Array(1);
  const sink_outRealMiddleBand = new Float64Array(1);
  const sink_outRealLowerBand = new Float64Array(1);
  const retCode = bbandsOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, outBegIdx, outNBElement, sink_outRealUpperBand, sink_outRealMiddleBand, sink_outRealLowerBand, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('BBANDS open', inReal.length, startIdx, bbandsLookbackPositional(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType as MAType, core));
  }
  if (retCode === RetCode.BAD_PARAM && bbandsLookbackPositional(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType as MAType, core) < 0) {
    throw rejectedParams('BBANDS open', PARAM_RANGES, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType);
  }
  throw streamFailure('BBANDS open', retCode);
}

/** @internal */
export function bbandsOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInNbDevUp: number,
  optInNbDevDn: number,
  optInMAType: MAType | typeof Core.INTEGER_DEFAULT,
  core: Core = Core.DEFAULT,
): BbandsStream {
  optInTimePeriod = requireInt32('BBANDS open', 'timePeriod', optInTimePeriod);
  optInNbDevUp = requireReal('BBANDS open', 'nbDevUp', optInNbDevUp);
  optInNbDevDn = requireReal('BBANDS open', 'nbDevDn', optInNbDevDn);
  optInMAType = requireInt32('BBANDS open', 'maType', optInMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('BBANDS open', 'real', inReal);
  requireHistory('BBANDS open', 'real', inReal.length);
  return bbandsOpenInternal(core, float64(inReal), 0, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType);
}

/** @internal */
export function bbandsOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInNbDevUp: number,
  optInNbDevDn: number,
  optInMAType: MAType | typeof Core.INTEGER_DEFAULT,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
  core: Core = Core.DEFAULT,
): BbandsStream {
  optInTimePeriod = requireInt32('BBANDS openAndFill', 'timePeriod', optInTimePeriod);
  optInNbDevUp = requireReal('BBANDS openAndFill', 'nbDevUp', optInNbDevUp);
  optInNbDevDn = requireReal('BBANDS openAndFill', 'nbDevDn', optInNbDevDn);
  optInMAType = requireInt32('BBANDS openAndFill', 'maType', optInMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('BBANDS openAndFill', 'real', inReal);
  requireHistory('BBANDS openAndFill', 'real', inReal.length);
  const guardLookback = bbandsLookbackPositional(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType as MAType, core);
  if (guardLookback < 0) {
    throw rejectedParams('BBANDS openAndFill', PARAM_RANGES, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType);
  }
  const guardOutLen = openFillCount('BBANDS openAndFill', inReal.length, guardLookback);
  requireOutput('BBANDS openAndFill', 'outUpperBand', outRealUpperBand, guardOutLen, Float64Array);
  requireOutput('BBANDS openAndFill', 'outMiddleBand', outRealMiddleBand, guardOutLen, Float64Array);
  requireOutput('BBANDS openAndFill', 'outLowerBand', outRealLowerBand, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outRealUpperBand.buffer === inRealF64.buffer && outRealUpperBand.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('BBANDS openAndFill', 'outUpperBand', 'real');
  }
  if (outRealMiddleBand.buffer === inRealF64.buffer && outRealMiddleBand.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('BBANDS openAndFill', 'outMiddleBand', 'real');
  }
  if (outRealLowerBand.buffer === inRealF64.buffer && outRealLowerBand.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('BBANDS openAndFill', 'outLowerBand', 'real');
  }
  if (outRealUpperBand.buffer === outRealMiddleBand.buffer && outRealUpperBand.byteOffset === outRealMiddleBand.byteOffset) {
    throw sharedMemory('BBANDS openAndFill', 'outUpperBand', 'outMiddleBand');
  }
  if (outRealUpperBand.buffer === outRealLowerBand.buffer && outRealUpperBand.byteOffset === outRealLowerBand.byteOffset) {
    throw sharedMemory('BBANDS openAndFill', 'outUpperBand', 'outLowerBand');
  }
  if (outRealMiddleBand.buffer === outRealLowerBand.buffer && outRealMiddleBand.byteOffset === outRealLowerBand.byteOffset) {
    throw sharedMemory('BBANDS openAndFill', 'outMiddleBand', 'outLowerBand');
  }
  if (guardOutLen > 0 && overlaps(outRealUpperBand, inRealF64)) {
    throw sharedMemory('BBANDS openAndFill', 'outUpperBand', 'real');
  }
  if (guardOutLen > 0 && overlaps(outRealMiddleBand, inRealF64)) {
    throw sharedMemory('BBANDS openAndFill', 'outMiddleBand', 'real');
  }
  if (guardOutLen > 0 && overlaps(outRealLowerBand, inRealF64)) {
    throw sharedMemory('BBANDS openAndFill', 'outLowerBand', 'real');
  }
  if (guardOutLen > 0 && overlaps(outRealUpperBand, outRealMiddleBand)) {
    throw sharedMemory('BBANDS openAndFill', 'outUpperBand', 'outMiddleBand');
  }
  if (guardOutLen > 0 && overlaps(outRealUpperBand, outRealLowerBand)) {
    throw sharedMemory('BBANDS openAndFill', 'outUpperBand', 'outLowerBand');
  }
  if (guardOutLen > 0 && overlaps(outRealMiddleBand, outRealLowerBand)) {
    throw sharedMemory('BBANDS openAndFill', 'outMiddleBand', 'outLowerBand');
  }
  return bbandsOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, new MInteger(), new MInteger(), outRealUpperBand, outRealMiddleBand, outRealLowerBand);
}

/**
 * Opens a live BBANDS stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link bbands} there.
 *
 * @param real - Warm-up history. Input data series.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link BbandsOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `bbandsLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a real parameter that is not a
 *   number or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { MAType, bbandsOpen } from 'ta-lib';
 *
 * const series = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = bbandsOpen(series, {
 *   timePeriod: 20,
 *   nbDevUp: 2,
 *   nbDevDn: 2,
 *   maType: MAType.SMA,
 * });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function bbandsOpen(real: ArrayLike<number>, options?: BbandsOptions): BbandsStream {
  const o = optionsOf('BBANDS open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInNbDevUp = orDefault(o.nbDevUp, Core.REAL_DEFAULT);
  const optInNbDevDn = orDefault(o.nbDevDn, Core.REAL_DEFAULT);
  const optInMAType = orDefault(o.maType, Core.INTEGER_DEFAULT);
  const core = coreOf('BBANDS open', o.core);
  return bbandsOpenPositional(real, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, core);
}

/**
 * {@link bbandsOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link bbands}.
 *
 * @param real - Warm-up history. Input data series.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link BbandsOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link bbands} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `bbandsLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a real parameter that is not a
 *   number, a parameter outside its range or a buffer too short, of the wrong type
 *   or starting where another buffer or a `Float64Array` input starts, or, on a
 *   history long enough to open, one sharing memory with another buffer or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { MAType, bbandsOpenAndFill } from 'ta-lib';
 *
 * const series = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const {
 *   stream,
 *   begIdx,
 *   outUpperBand,
 *   outMiddleBand,
 *   outLowerBand,
 * } = bbandsOpenAndFill(series, {
 *   timePeriod: 20,
 *   nbDevUp: 2,
 *   nbDevDn: 2,
 *   maType: MAType.SMA,
 * });
 * // outUpperBand[k] is the value at bar begIdx + k
 * ```
 */
export function bbandsOpenAndFill(real: ArrayLike<number>, options?: BbandsOptions): BbandsResult & { stream: BbandsStream } {
  const o = optionsOf('BBANDS openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInNbDevUp = orDefault(o.nbDevUp, Core.REAL_DEFAULT);
  const optInNbDevDn = orDefault(o.nbDevDn, Core.REAL_DEFAULT);
  const optInMAType = orDefault(o.maType, Core.INTEGER_DEFAULT);
  const core = coreOf('BBANDS openAndFill', o.core);
  const capacity = fillCapacity(bbandsLookbackPositional(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, core), real);
  const outRealUpperBand = o.outUpperBand === undefined ? new Float64Array(capacity) : o.outUpperBand;
  const outRealMiddleBand = o.outMiddleBand === undefined ? new Float64Array(capacity) : o.outMiddleBand;
  const outRealLowerBand = o.outLowerBand === undefined ? new Float64Array(capacity) : o.outLowerBand;
  const stream = bbandsOpenAndFillPositional(real, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, outRealUpperBand, outRealMiddleBand, outRealLowerBand, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outUpperBand: outRealUpperBand.subarray(0, range.count), outMiddleBand: outRealMiddleBand.subarray(0, range.count), outLowerBand: outRealLowerBand.subarray(0, range.count) };
}
