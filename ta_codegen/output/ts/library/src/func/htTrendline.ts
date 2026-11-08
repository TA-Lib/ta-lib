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
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  120802 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  070326 MF,CC  Remove the smoothPrice circular buffer: it was written
 *                on every bar but never read in this function (issue #88).
 *                The trendline averages RAW price over the dominant cycle
 *                period, exactly as published (Ehlers, "Rocket Science
 *                for Traders": ITrend sums Price, not SmoothPrice).
 *  071226 MF,CC  Stream: rewrite the raw-price backward sum
 *                for(i<DCPeriodInt) sum += inReal[idx--] (idx = today) as a
 *                constant-cap padded loop for(i<50) if(i<DCPeriodInt) sum +=
 *                inReal[today-i]. Bit-identical (same terms, same order); the
 *                literal cap lets the streaming rescan-window machinery bound it.
 *  100726 MF,CC  #492. The Auto rule sized on when the integer cycle period
 *                of two starts stops disagreeing.
 */

import { FuncUnstId } from '../enums.ts';
import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function htTrendlineLookbackPositional(core: Core = Core.DEFAULT): number {
  /* 31 input are skip
   * +32 output are skip to account for misc lookback
   * ---
   *  63 Total Lookback
   *
   * 31 is for being compatible with Tradestation.
   * See mama_lookback for an explanation of the "32".
   *
   * Two starts are equal once their integer cycle periods have agreed for
   * four bars, at either level: the Auto count buys a rarer late
   * disagreement, never a smaller difference.
   */
  return 63 + core.unstableCount(FuncUnstId.HT_TRENDLINE, 120 + ((20 * 4) | 0), 120 + ((20 * 8) | 0));
}

/** @internal */
export function htTrendlineDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function htTrendlineImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let outIdx = 0;
  let i = 0;
  let lookbackTotal = 0;
  let today = 0;
  let tempReal = 0;
  let tempReal2 = 0;
  let adjustedPrevPeriod = 0;
  let period = 0;
  let trailingWMAIdx = 0;
  let periodWMASum = 0;
  let periodWMASub = 0;
  let trailingWMAValue = 0;
  let smoothedValue = 0;
  let iTrend1 = 0;
  let iTrend2 = 0;
  let iTrend3 = 0;
  let a = 0;
  let b = 0;
  let hilbertTempReal = 0;
  let hilbertIdx = 0;
  const detrender_Odd = new Float64Array(3);
  const detrender_Even = new Float64Array(3);
  let detrender = 0;
  let prev_detrender_Odd = 0;
  let prev_detrender_Even = 0;
  let prev_detrender_input_Odd = 0;
  let prev_detrender_input_Even = 0;
  const Q1_Odd = new Float64Array(3);
  const Q1_Even = new Float64Array(3);
  let Q1 = 0;
  let prev_Q1_Odd = 0;
  let prev_Q1_Even = 0;
  let prev_Q1_input_Odd = 0;
  let prev_Q1_input_Even = 0;
  const jI_Odd = new Float64Array(3);
  const jI_Even = new Float64Array(3);
  let jI = 0;
  let prev_jI_Odd = 0;
  let prev_jI_Even = 0;
  let prev_jI_input_Odd = 0;
  let prev_jI_input_Even = 0;
  const jQ_Odd = new Float64Array(3);
  const jQ_Even = new Float64Array(3);
  let jQ = 0;
  let prev_jQ_Odd = 0;
  let prev_jQ_Even = 0;
  let prev_jQ_input_Odd = 0;
  let prev_jQ_input_Even = 0;
  let Q2 = 0;
  let I2 = 0;
  let prevQ2 = 0;
  let prevI2 = 0;
  let Re = 0;
  let Im = 0;
  let I1ForOddPrev2 = 0;
  let I1ForOddPrev3 = 0;
  let I1ForEvenPrev2 = 0;
  let I1ForEvenPrev3 = 0;
  let rad2Deg = 0;
  let todayValue = 0;
  let smoothPeriod = 0;
  let DCPeriodInt = 0;
  let DCPeriod = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  a = 0.0962;
  b = 0.5769;
  /* Variable used for the price smoother (a weighted moving average). */
  /* Variable to keep track of the last 3 ITrend */
  /* Variables used for the Hilbert Transormation */
  /* Variable used to calculate the dominant cycle phase */
  /* circular buffer already declared */
  iTrend3 = 0.0;
  iTrend2 = iTrend3;
  iTrend1 = iTrend2;
  /* Constant */
  tempReal = Math.atan(1);
  rad2Deg = 45.0 / tempReal;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = htTrendlineLookbackPositional(core);
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
  outBegIdx.value = startIdx;
  /* Initialize the price smoother, which is simply a weighted
   * moving average of the price.
   * To understand this algorithm, I strongly suggest to understand
   * first how TA_WMA is done.
   */
  trailingWMAIdx = startIdx - lookbackTotal;
  today = trailingWMAIdx;
  /* Initialization is same as WMA, except loop is unrolled
   * for speed optimization.
   */
  tempReal = inReal[today++];
  periodWMASub = tempReal;
  periodWMASum = tempReal;
  tempReal = inReal[today++];
  periodWMASub += tempReal;
  periodWMASum += tempReal * 2.0;
  tempReal = inReal[today++];
  periodWMASub += tempReal;
  periodWMASum += tempReal * 3.0;
  trailingWMAValue = 0.0;
  /* Subsequent WMA value are evaluated by using
   * the DO_PRICE_WMA macro.
   */
  i = 34;
  do {
    tempReal = inReal[today++];
    periodWMASub += tempReal;
    periodWMASub -= trailingWMAValue;
    periodWMASum += tempReal * 4.0;
    trailingWMAValue = inReal[trailingWMAIdx++];
    smoothedValue = periodWMASum * 0.1;
    periodWMASum -= periodWMASub;
  } while (--i !== 0);
  /* Initialize the circular buffers used by the hilbert
   * transform logic.
   * A buffer is used for odd day and another for even days.
   * This minimize the number of memory access and floating point
   * operations needed (note also that by using static circular buffer,
   * no large dynamic memory allocation is needed for storing
   * intermediate calculation!).
   */
  hilbertIdx = 0;
  detrender_Odd[0] = 0.0;
  detrender_Odd[1] = 0.0;
  detrender_Odd[2] = 0.0;
  detrender_Even[0] = 0.0;
  detrender_Even[1] = 0.0;
  detrender_Even[2] = 0.0;
  detrender = 0.0;
  prev_detrender_Odd = 0.0;
  prev_detrender_Even = 0.0;
  prev_detrender_input_Odd = 0.0;
  prev_detrender_input_Even = 0.0;
  Q1_Odd[0] = 0.0;
  Q1_Odd[1] = 0.0;
  Q1_Odd[2] = 0.0;
  Q1_Even[0] = 0.0;
  Q1_Even[1] = 0.0;
  Q1_Even[2] = 0.0;
  Q1 = 0.0;
  prev_Q1_Odd = 0.0;
  prev_Q1_Even = 0.0;
  prev_Q1_input_Odd = 0.0;
  prev_Q1_input_Even = 0.0;
  jI_Odd[0] = 0.0;
  jI_Odd[1] = 0.0;
  jI_Odd[2] = 0.0;
  jI_Even[0] = 0.0;
  jI_Even[1] = 0.0;
  jI_Even[2] = 0.0;
  jI = 0.0;
  prev_jI_Odd = 0.0;
  prev_jI_Even = 0.0;
  prev_jI_input_Odd = 0.0;
  prev_jI_input_Even = 0.0;
  jQ_Odd[0] = 0.0;
  jQ_Odd[1] = 0.0;
  jQ_Odd[2] = 0.0;
  jQ_Even[0] = 0.0;
  jQ_Even[1] = 0.0;
  jQ_Even[2] = 0.0;
  jQ = 0.0;
  prev_jQ_Odd = 0.0;
  prev_jQ_Even = 0.0;
  prev_jQ_input_Odd = 0.0;
  prev_jQ_input_Even = 0.0;
  period = 0.0;
  outIdx = 0;
  prevQ2 = 0.0;
  prevI2 = prevQ2;
  Im = 0.0;
  Re = Im;
  I1ForEvenPrev3 = 0.0;
  I1ForOddPrev3 = I1ForEvenPrev3;
  I1ForEvenPrev2 = 0.0;
  I1ForOddPrev2 = I1ForEvenPrev2;
  smoothPeriod = 0.0;
  /* The code is speed optimized and is most likely very
   * hard to follow if you do not already know well the
   * original algorithm.
   * To understadn better, it is strongly suggested to look
   * first at the Excel implementation in "test_MAMA.xls" included
   * in this package.
   */
  while (today <= endIdx) {
    {
      const _a: number = 0.075, _b: number = period, _c: number = 0.54;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      adjustedPrevPeriod = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    todayValue = inReal[today];
    periodWMASub += todayValue;
    periodWMASub -= trailingWMAValue;
    periodWMASum += todayValue * 4.0;
    trailingWMAValue = inReal[trailingWMAIdx++];
    smoothedValue = periodWMASum * 0.1;
    periodWMASum -= periodWMASub;
    if (((today % 2) | 0) === 0) {
      /* Do the Hilbert Transforms for even price bar */
      hilbertTempReal = a * smoothedValue;
      detrender = -detrender_Even[hilbertIdx];
      detrender_Even[hilbertIdx] = hilbertTempReal;
      detrender += hilbertTempReal;
      detrender -= prev_detrender_Even;
      prev_detrender_Even = b * prev_detrender_input_Even;
      detrender += prev_detrender_Even;
      prev_detrender_input_Even = smoothedValue;
      detrender *= adjustedPrevPeriod;
      hilbertTempReal = a * detrender;
      Q1 = -Q1_Even[hilbertIdx];
      Q1_Even[hilbertIdx] = hilbertTempReal;
      Q1 += hilbertTempReal;
      Q1 -= prev_Q1_Even;
      prev_Q1_Even = b * prev_Q1_input_Even;
      Q1 += prev_Q1_Even;
      prev_Q1_input_Even = detrender;
      Q1 *= adjustedPrevPeriod;
      hilbertTempReal = a * I1ForEvenPrev3;
      jI = -jI_Even[hilbertIdx];
      jI_Even[hilbertIdx] = hilbertTempReal;
      jI += hilbertTempReal;
      jI -= prev_jI_Even;
      prev_jI_Even = b * prev_jI_input_Even;
      jI += prev_jI_Even;
      prev_jI_input_Even = I1ForEvenPrev3;
      jI *= adjustedPrevPeriod;
      hilbertTempReal = a * Q1;
      jQ = -jQ_Even[hilbertIdx];
      jQ_Even[hilbertIdx] = hilbertTempReal;
      jQ += hilbertTempReal;
      jQ -= prev_jQ_Even;
      prev_jQ_Even = b * prev_jQ_input_Even;
      jQ += prev_jQ_Even;
      prev_jQ_input_Even = Q1;
      jQ *= adjustedPrevPeriod;
      if (++hilbertIdx === 3) {
        hilbertIdx = 0;
      }
      {
        const _a: number = 0.2, _b: number = Q1 + jI, _c: number = 0.8 * prevQ2;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        Q2 = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      {
        const _a: number = 0.2, _b: number = I1ForEvenPrev3 - jQ, _c: number = 0.8 * prevI2;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        I2 = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      /* The variable I1 is the detrender delayed for
       * 3 price bars.
       *
       * Save the current detrender value for being
       * used by the "odd" logic later.
       */
      I1ForOddPrev3 = I1ForOddPrev2;
      I1ForOddPrev2 = detrender;
    } else {
      /* Do the Hilbert Transforms for odd price bar */
      hilbertTempReal = a * smoothedValue;
      detrender = -detrender_Odd[hilbertIdx];
      detrender_Odd[hilbertIdx] = hilbertTempReal;
      detrender += hilbertTempReal;
      detrender -= prev_detrender_Odd;
      prev_detrender_Odd = b * prev_detrender_input_Odd;
      detrender += prev_detrender_Odd;
      prev_detrender_input_Odd = smoothedValue;
      detrender *= adjustedPrevPeriod;
      hilbertTempReal = a * detrender;
      Q1 = -Q1_Odd[hilbertIdx];
      Q1_Odd[hilbertIdx] = hilbertTempReal;
      Q1 += hilbertTempReal;
      Q1 -= prev_Q1_Odd;
      prev_Q1_Odd = b * prev_Q1_input_Odd;
      Q1 += prev_Q1_Odd;
      prev_Q1_input_Odd = detrender;
      Q1 *= adjustedPrevPeriod;
      hilbertTempReal = a * I1ForOddPrev3;
      jI = -jI_Odd[hilbertIdx];
      jI_Odd[hilbertIdx] = hilbertTempReal;
      jI += hilbertTempReal;
      jI -= prev_jI_Odd;
      prev_jI_Odd = b * prev_jI_input_Odd;
      jI += prev_jI_Odd;
      prev_jI_input_Odd = I1ForOddPrev3;
      jI *= adjustedPrevPeriod;
      hilbertTempReal = a * Q1;
      jQ = -jQ_Odd[hilbertIdx];
      jQ_Odd[hilbertIdx] = hilbertTempReal;
      jQ += hilbertTempReal;
      jQ -= prev_jQ_Odd;
      prev_jQ_Odd = b * prev_jQ_input_Odd;
      jQ += prev_jQ_Odd;
      prev_jQ_input_Odd = Q1;
      jQ *= adjustedPrevPeriod;
      {
        const _a: number = 0.2, _b: number = Q1 + jI, _c: number = 0.8 * prevQ2;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        Q2 = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      {
        const _a: number = 0.2, _b: number = I1ForOddPrev3 - jQ, _c: number = 0.8 * prevI2;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        I2 = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      /* The varaiable I1 is the detrender delayed for
       * 3 price bars.
       *
       * Save the current detrender value for being
       * used by the "even" logic later.
       */
      I1ForEvenPrev3 = I1ForEvenPrev2;
      I1ForEvenPrev2 = detrender;
    }
    /* Adjust the period for next price bar */
    let _fma5: number;
    {
      const _a: number = I2, _b: number = prevI2, _c: number = Q2 * prevQ2;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      _fma5 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = 0.8, _b: number = Re, _c: number = 0.2 * _fma5;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      Re = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = 0.8, _b: number = Im, _c: number = 0.2 * (I2 * prevQ2 - Q2 * prevI2);
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      Im = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    prevQ2 = Q2;
    prevI2 = I2;
    tempReal = period;
    if (Im !== 0.0 && Re !== 0.0) {
      period = 360.0 / (Math.atan(Im / Re) * rad2Deg);
    }
    tempReal2 = 1.5 * tempReal;
    if (period > tempReal2) {
      period = tempReal2;
    }
    tempReal2 = 0.67 * tempReal;
    if (period < tempReal2) {
      period = tempReal2;
    }
    if (period < 6) {
      period = 6;
    } else if (period > 50) {
      period = 50;
    }
    {
      const _a: number = 0.2, _b: number = period, _c: number = 0.8 * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      period = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = 0.67, _b: number = smoothPeriod, _c: number = 0.33 * period;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      smoothPeriod = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    /* Compute Trendline */
    DCPeriod = smoothPeriod + 0.5;
    /* A NaN period has no int value, and C leaves converting one undefined. */
    if (DCPeriod >= 0.0) {
      DCPeriodInt = (DCPeriod | 0);
    } else {
      DCPeriodInt = 0;
    }
    /* Average the RAW price over the dominant cycle period
     * (Ehlers, "Rocket Science for Traders": the Instantaneous
     * Trendline sums Price — not SmoothPrice, which only feeds
     * the Hilbert detrender above). See issue #88.
     */
    /* Sum the last DCPeriodInt (<= 50) raw prices. The fixed 50-iteration
     * loop with an inner guard is a streaming-friendly rewrite of the
     * data-dependent backward scan `for(i<DCPeriodInt) sum += inReal[idx--]`
     * (idx starting at today): identical terms in identical order, so
     * bit-for-bit unchanged, but the constant cap lets the rescan-window
     * machinery bound the window (DCPeriod is clamped to [6.5, 50.5]).
     */
    tempReal = 0.0;
    for (i = 0; i < 50; i += 1) {
      if (i < DCPeriodInt) {
        tempReal += inReal[today - i];
      }
    }
    if (DCPeriodInt > 0) {
      tempReal = tempReal / DCPeriodInt;
    }
    let _fma10: number;
    {
      const _a: number = 4.0, _b: number = tempReal, _c: number = 3.0 * iTrend1;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      _fma10 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    let _fma11: number;
    {
      const _a: number = 2.0, _b: number = iTrend2, _c: number = _fma10;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      _fma11 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    tempReal2 = (_fma11 + iTrend3) / 10.0;
    iTrend3 = iTrend2;
    iTrend2 = iTrend1;
    iTrend1 = tempReal;
    if (today >= startIdx) {
      outReal[outIdx++] = tempReal2;
    }
    /* Ooof... let's do the next price bar now! */
    today += 1;
  }
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function htTrendlinePositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('HT_TRENDLINE', 'startIdx', startIdx);
  endIdx = requireInt32('HT_TRENDLINE', 'endIdx', endIdx);
  requireIndexRange('HT_TRENDLINE', startIdx, endIdx);
  const guardStart = clampedStart('HT_TRENDLINE', startIdx, htTrendlineLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('HT_TRENDLINE', 'real', inReal, guardInLen);
  requireOutput('HT_TRENDLINE', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('HT_TRENDLINE', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = htTrendlineImpl(core, startIdx, endIdx, inRealF64, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('HT_TRENDLINE', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link htTrendline}, {@link htTrendlineLookback},
 * {@link htTrendlineDisplayShift}, {@link htTrendlineOpen} and
 * {@link htTrendlineOpenAndFill}. Every key is optional: an omitted or `undefined`
 * key takes its default, and a call ignores the keys it has no use for.
 */
export interface HtTrendlineOptions {
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

/**
 * What {@link htTrendline} returns, and {@link htTrendlineOpenAndFill} with its
 * stream.
 */
export interface HtTrendlineResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Instantaneous trendline value. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link htTrendline} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link HtTrendlineOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function htTrendlineLookback(options?: HtTrendlineOptions): number {
  const o = optionsOf('HT_TRENDLINE lookback', options);
  return htTrendlineLookbackPositional(coreOf('HT_TRENDLINE lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link htTrendline}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link HtTrendlineOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function htTrendlineDisplayShift(outputIdx: number, options?: HtTrendlineOptions): number {
  const o = optionsOf('HT_TRENDLINE displayShift', options);
  return htTrendlineDisplayShiftPositional(outputIdx, coreOf('HT_TRENDLINE displayShift', o.core));
}

/**
 * Ehlers' Hilbert Transform Instantaneous Trendline: a smoothed, low-lag overlay
 * whose averaging window adapts to the dominant cycle period measured via
 * Hilbert-transform quadrature (I/Q) analysis of price.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ht_trendline](https://ta-lib.org/functions/ht_trendline).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link htTrendlineLookback} is a success with empty outputs.
 *
 * @param real - Source price series.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link HtTrendlineOptions}.
 * @returns `begIdx` and the values; see {@link HtTrendlineResult}. Each output is
 *   a view of its buffer, the caller's or a new one.
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
 * import { htTrendline } from 'ta-lib';
 *
 * const series = new Float64Array(252); // your data, oldest first
 *
 * const { begIdx, values } = htTrendline(series);
 * console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
 * ```
 * @see [`htDcperiod`](https://ta-lib.org/functions/ht_dcperiod)
 * @see [`htPhasor`](https://ta-lib.org/functions/ht_phasor)
 * @see [`mama`](https://ta-lib.org/functions/mama)
 * @see [`wma`](https://ta-lib.org/functions/wma)
 */
export function htTrendline(real: ArrayLike<number>, options?: HtTrendlineOptions): HtTrendlineResult {
  const o = optionsOf('HT_TRENDLINE', options);
  if (o.endIdx === undefined) {
    requireSeries('HT_TRENDLINE', 'real', real);
  }
  const core = coreOf('HT_TRENDLINE', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, htTrendlineLookbackPositional(core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = htTrendlinePositional(startIdx, endIdx, real, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live HT_TRENDLINE stream: one value per closed bar, bit-identical to
 * {@link htTrendline} over the same series. Open it with {@link htTrendlineOpen}
 * or {@link htTrendlineOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ht_trendline](https://ta-lib.org/functions/ht_trendline).
 */
export class HtTrendlineStream {
  /** @internal */
  core: Core;
  /** @internal */
  period: number = 0;
  /** @internal */
  periodWMASum: number = 0;
  /** @internal */
  periodWMASub: number = 0;
  /** @internal */
  trailingWMAValue: number = 0;
  /** @internal */
  iTrend1: number = 0;
  /** @internal */
  iTrend2: number = 0;
  /** @internal */
  iTrend3: number = 0;
  /** @internal */
  a: number = 0;
  /** @internal */
  b: number = 0;
  /** @internal */
  hilbertIdx: number = 0;
  /** @internal */
  detrender_Odd: Float64Array = new Float64Array(3);
  /** @internal */
  detrender_Even: Float64Array = new Float64Array(3);
  /** @internal */
  prev_detrender_Odd: number = 0;
  /** @internal */
  prev_detrender_Even: number = 0;
  /** @internal */
  prev_detrender_input_Odd: number = 0;
  /** @internal */
  prev_detrender_input_Even: number = 0;
  /** @internal */
  Q1_Odd: Float64Array = new Float64Array(3);
  /** @internal */
  Q1_Even: Float64Array = new Float64Array(3);
  /** @internal */
  prev_Q1_Odd: number = 0;
  /** @internal */
  prev_Q1_Even: number = 0;
  /** @internal */
  prev_Q1_input_Odd: number = 0;
  /** @internal */
  prev_Q1_input_Even: number = 0;
  /** @internal */
  jI_Odd: Float64Array = new Float64Array(3);
  /** @internal */
  jI_Even: Float64Array = new Float64Array(3);
  /** @internal */
  prev_jI_Odd: number = 0;
  /** @internal */
  prev_jI_Even: number = 0;
  /** @internal */
  prev_jI_input_Odd: number = 0;
  /** @internal */
  prev_jI_input_Even: number = 0;
  /** @internal */
  jQ_Odd: Float64Array = new Float64Array(3);
  /** @internal */
  jQ_Even: Float64Array = new Float64Array(3);
  /** @internal */
  prev_jQ_Odd: number = 0;
  /** @internal */
  prev_jQ_Even: number = 0;
  /** @internal */
  prev_jQ_input_Odd: number = 0;
  /** @internal */
  prev_jQ_input_Even: number = 0;
  /** @internal */
  prevQ2: number = 0;
  /** @internal */
  prevI2: number = 0;
  /** @internal */
  Re: number = 0;
  /** @internal */
  Im: number = 0;
  /** @internal */
  I1ForOddPrev2: number = 0;
  /** @internal */
  I1ForOddPrev3: number = 0;
  /** @internal */
  I1ForEvenPrev2: number = 0;
  /** @internal */
  I1ForEvenPrev3: number = 0;
  /** @internal */
  rad2Deg: number = 0;
  /** @internal */
  smoothPeriod: number = 0;
  /** @internal */
  streamParity: number = 0;
  /** @internal */
  ringPos_trailingWMAIdx: number = 0;
  /** @internal */
  ringCap_trailingWMAIdx: number = 0;
  /** @internal */
  ring_trailingWMAIdx_inReal: Float64Array = new Float64Array(1);
  /** @internal */
  winPos_i: number = 0;
  /** @internal */
  winCap_i: number = 1;
  /** @internal */
  win_i_inReal: Float64Array = new Float64Array(1);
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
  static of(core: Core): HtTrendlineStream {
    return new HtTrendlineStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link htTrendline} reports them over the same bars. Each accepted `update` and
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
      throw streamFailure('HT_TRENDLINE advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: Source price series.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('HT_TRENDLINE update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('HT_TRENDLINE update', 'real');
    }
    htTrendlineStepImpl(this, real);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: Source price series.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number): number {
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('HT_TRENDLINE peek', 'real');
    }
    const sp = this;
    const inReal = real;
    let i = 0;
    let tempReal = 0;
    let tempReal2 = 0;
    let adjustedPrevPeriod = 0;
    let smoothedValue = 0;
    let hilbertTempReal = 0;
    let detrender = 0;
    let Q1 = 0;
    let jI = 0;
    let jQ = 0;
    let Q2 = 0;
    let I2 = 0;
    let todayValue = 0;
    let DCPeriodInt = 0;
    let DCPeriod = 0;
    let I1ForEvenPrev2 = sp.I1ForEvenPrev2;
    let I1ForEvenPrev3 = sp.I1ForEvenPrev3;
    let I1ForOddPrev2 = sp.I1ForOddPrev2;
    let I1ForOddPrev3 = sp.I1ForOddPrev3;
    let Im = sp.Im;
    let Re = sp.Re;
    let cur_outReal = 0;
    let hilbertIdx = sp.hilbertIdx;
    let iTrend1 = sp.iTrend1;
    let iTrend2 = sp.iTrend2;
    let iTrend3 = sp.iTrend3;
    let period = sp.period;
    let periodWMASub = sp.periodWMASub;
    let periodWMASum = sp.periodWMASum;
    let prevI2 = sp.prevI2;
    let prevQ2 = sp.prevQ2;
    let prev_Q1_Even = sp.prev_Q1_Even;
    let prev_Q1_Odd = sp.prev_Q1_Odd;
    let prev_Q1_input_Even = sp.prev_Q1_input_Even;
    let prev_Q1_input_Odd = sp.prev_Q1_input_Odd;
    let prev_detrender_Even = sp.prev_detrender_Even;
    let prev_detrender_Odd = sp.prev_detrender_Odd;
    let prev_detrender_input_Even = sp.prev_detrender_input_Even;
    let prev_detrender_input_Odd = sp.prev_detrender_input_Odd;
    let prev_jI_Even = sp.prev_jI_Even;
    let prev_jI_Odd = sp.prev_jI_Odd;
    let prev_jI_input_Even = sp.prev_jI_input_Even;
    let prev_jI_input_Odd = sp.prev_jI_input_Odd;
    let prev_jQ_Even = sp.prev_jQ_Even;
    let prev_jQ_Odd = sp.prev_jQ_Odd;
    let prev_jQ_input_Even = sp.prev_jQ_input_Even;
    let prev_jQ_input_Odd = sp.prev_jQ_input_Odd;
    let smoothPeriod = sp.smoothPeriod;
    let trailingWMAValue = sp.trailingWMAValue;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    pkSlot0 = sp.winPos_i;
    pkVal0 = inReal;
    {
      const _a: number = 0.075, _b: number = period, _c: number = 0.54;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      adjustedPrevPeriod = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    todayValue = inReal;
    periodWMASub += todayValue;
    periodWMASub -= trailingWMAValue;
    periodWMASum += todayValue * 4.0;
    trailingWMAValue = sp.ring_trailingWMAIdx_inReal[sp.ringPos_trailingWMAIdx];
    smoothedValue = periodWMASum * 0.1;
    periodWMASum -= periodWMASub;
    if (sp.streamParity === 0) {
      /* Do the Hilbert Transforms for even price bar */
      hilbertTempReal = sp.a * smoothedValue;
      detrender = -sp.detrender_Even[hilbertIdx];
      detrender += hilbertTempReal;
      detrender -= prev_detrender_Even;
      prev_detrender_Even = sp.b * prev_detrender_input_Even;
      detrender += prev_detrender_Even;
      prev_detrender_input_Even = smoothedValue;
      detrender *= adjustedPrevPeriod;
      hilbertTempReal = sp.a * detrender;
      Q1 = -sp.Q1_Even[hilbertIdx];
      Q1 += hilbertTempReal;
      Q1 -= prev_Q1_Even;
      prev_Q1_Even = sp.b * prev_Q1_input_Even;
      Q1 += prev_Q1_Even;
      prev_Q1_input_Even = detrender;
      Q1 *= adjustedPrevPeriod;
      hilbertTempReal = sp.a * I1ForEvenPrev3;
      jI = -sp.jI_Even[hilbertIdx];
      jI += hilbertTempReal;
      jI -= prev_jI_Even;
      prev_jI_Even = sp.b * prev_jI_input_Even;
      jI += prev_jI_Even;
      prev_jI_input_Even = I1ForEvenPrev3;
      jI *= adjustedPrevPeriod;
      hilbertTempReal = sp.a * Q1;
      jQ = -sp.jQ_Even[hilbertIdx];
      jQ += hilbertTempReal;
      jQ -= prev_jQ_Even;
      prev_jQ_Even = sp.b * prev_jQ_input_Even;
      jQ += prev_jQ_Even;
      prev_jQ_input_Even = Q1;
      jQ *= adjustedPrevPeriod;
      if (++hilbertIdx === 3) {
        hilbertIdx = 0;
      }
      {
        const _a: number = 0.2, _b: number = Q1 + jI, _c: number = 0.8 * prevQ2;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        Q2 = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      {
        const _a: number = 0.2, _b: number = I1ForEvenPrev3 - jQ, _c: number = 0.8 * prevI2;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        I2 = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      /* The variable I1 is the detrender delayed for
       * 3 price bars.
       *
       * Save the current detrender value for being
       * used by the "odd" logic later.
       */
      I1ForOddPrev3 = I1ForOddPrev2;
      I1ForOddPrev2 = detrender;
    } else {
      /* Do the Hilbert Transforms for odd price bar */
      hilbertTempReal = sp.a * smoothedValue;
      detrender = -sp.detrender_Odd[hilbertIdx];
      detrender += hilbertTempReal;
      detrender -= prev_detrender_Odd;
      prev_detrender_Odd = sp.b * prev_detrender_input_Odd;
      detrender += prev_detrender_Odd;
      prev_detrender_input_Odd = smoothedValue;
      detrender *= adjustedPrevPeriod;
      hilbertTempReal = sp.a * detrender;
      Q1 = -sp.Q1_Odd[hilbertIdx];
      Q1 += hilbertTempReal;
      Q1 -= prev_Q1_Odd;
      prev_Q1_Odd = sp.b * prev_Q1_input_Odd;
      Q1 += prev_Q1_Odd;
      prev_Q1_input_Odd = detrender;
      Q1 *= adjustedPrevPeriod;
      hilbertTempReal = sp.a * I1ForOddPrev3;
      jI = -sp.jI_Odd[hilbertIdx];
      jI += hilbertTempReal;
      jI -= prev_jI_Odd;
      prev_jI_Odd = sp.b * prev_jI_input_Odd;
      jI += prev_jI_Odd;
      prev_jI_input_Odd = I1ForOddPrev3;
      jI *= adjustedPrevPeriod;
      hilbertTempReal = sp.a * Q1;
      jQ = -sp.jQ_Odd[hilbertIdx];
      jQ += hilbertTempReal;
      jQ -= prev_jQ_Odd;
      prev_jQ_Odd = sp.b * prev_jQ_input_Odd;
      jQ += prev_jQ_Odd;
      prev_jQ_input_Odd = Q1;
      jQ *= adjustedPrevPeriod;
      {
        const _a: number = 0.2, _b: number = Q1 + jI, _c: number = 0.8 * prevQ2;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        Q2 = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      {
        const _a: number = 0.2, _b: number = I1ForOddPrev3 - jQ, _c: number = 0.8 * prevI2;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        I2 = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      /* The varaiable I1 is the detrender delayed for
       * 3 price bars.
       *
       * Save the current detrender value for being
       * used by the "even" logic later.
       */
      I1ForEvenPrev3 = I1ForEvenPrev2;
      I1ForEvenPrev2 = detrender;
    }
    /* Adjust the period for next price bar */
    let _fma5: number;
    {
      const _a: number = I2, _b: number = prevI2, _c: number = Q2 * prevQ2;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      _fma5 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = 0.8, _b: number = Re, _c: number = 0.2 * _fma5;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      Re = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = 0.8, _b: number = Im, _c: number = 0.2 * (I2 * prevQ2 - Q2 * prevI2);
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      Im = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    prevQ2 = Q2;
    prevI2 = I2;
    tempReal = period;
    if (Im !== 0.0 && Re !== 0.0) {
      period = 360.0 / (Math.atan(Im / Re) * sp.rad2Deg);
    }
    tempReal2 = 1.5 * tempReal;
    if (period > tempReal2) {
      period = tempReal2;
    }
    tempReal2 = 0.67 * tempReal;
    if (period < tempReal2) {
      period = tempReal2;
    }
    if (period < 6) {
      period = 6;
    } else if (period > 50) {
      period = 50;
    }
    {
      const _a: number = 0.2, _b: number = period, _c: number = 0.8 * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      period = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = 0.67, _b: number = smoothPeriod, _c: number = 0.33 * period;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      smoothPeriod = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    /* Compute Trendline */
    DCPeriod = smoothPeriod + 0.5;
    /* A NaN period has no int value, and C leaves converting one undefined. */
    if (DCPeriod >= 0.0) {
      DCPeriodInt = (DCPeriod | 0);
    } else {
      DCPeriodInt = 0;
    }
    /* Average the RAW price over the dominant cycle period
     * (Ehlers, "Rocket Science for Traders": the Instantaneous
     * Trendline sums Price — not SmoothPrice, which only feeds
     * the Hilbert detrender above). See issue #88.
     */
    /* Sum the last DCPeriodInt (<= 50) raw prices. The fixed 50-iteration
     * loop with an inner guard is a streaming-friendly rewrite of the
     * data-dependent backward scan `for(i<DCPeriodInt) sum += inReal[idx--]`
     * (idx starting at today): identical terms in identical order, so
     * bit-for-bit unchanged, but the constant cap lets the rescan-window
     * machinery bound the window (DCPeriod is clamped to [6.5, 50.5]).
     */
    tempReal = 0.0;
    for (i = 0; i < 50; i += 1) {
      if (i < DCPeriodInt) {
        tempReal += (((sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i) !== pkSlot0) ? sp.win_i_inReal[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i] : pkVal0;
      }
    }
    if (DCPeriodInt > 0) {
      tempReal = tempReal / DCPeriodInt;
    }
    let _fma10: number;
    {
      const _a: number = 4.0, _b: number = tempReal, _c: number = 3.0 * iTrend1;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      _fma10 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    let _fma11: number;
    {
      const _a: number = 2.0, _b: number = iTrend2, _c: number = _fma10;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      _fma11 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    tempReal2 = (_fma11 + iTrend3) / 10.0;
    iTrend3 = iTrend2;
    iTrend2 = iTrend1;
    iTrend1 = tempReal;
    cur_outReal = tempReal2;
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
  clone(): HtTrendlineStream {
    const c = new HtTrendlineStream(this.core);
    c.period = this.period;
    c.periodWMASum = this.periodWMASum;
    c.periodWMASub = this.periodWMASub;
    c.trailingWMAValue = this.trailingWMAValue;
    c.iTrend1 = this.iTrend1;
    c.iTrend2 = this.iTrend2;
    c.iTrend3 = this.iTrend3;
    c.a = this.a;
    c.b = this.b;
    c.hilbertIdx = this.hilbertIdx;
    c.detrender_Odd = this.detrender_Odd.slice();
    c.detrender_Even = this.detrender_Even.slice();
    c.prev_detrender_Odd = this.prev_detrender_Odd;
    c.prev_detrender_Even = this.prev_detrender_Even;
    c.prev_detrender_input_Odd = this.prev_detrender_input_Odd;
    c.prev_detrender_input_Even = this.prev_detrender_input_Even;
    c.Q1_Odd = this.Q1_Odd.slice();
    c.Q1_Even = this.Q1_Even.slice();
    c.prev_Q1_Odd = this.prev_Q1_Odd;
    c.prev_Q1_Even = this.prev_Q1_Even;
    c.prev_Q1_input_Odd = this.prev_Q1_input_Odd;
    c.prev_Q1_input_Even = this.prev_Q1_input_Even;
    c.jI_Odd = this.jI_Odd.slice();
    c.jI_Even = this.jI_Even.slice();
    c.prev_jI_Odd = this.prev_jI_Odd;
    c.prev_jI_Even = this.prev_jI_Even;
    c.prev_jI_input_Odd = this.prev_jI_input_Odd;
    c.prev_jI_input_Even = this.prev_jI_input_Even;
    c.jQ_Odd = this.jQ_Odd.slice();
    c.jQ_Even = this.jQ_Even.slice();
    c.prev_jQ_Odd = this.prev_jQ_Odd;
    c.prev_jQ_Even = this.prev_jQ_Even;
    c.prev_jQ_input_Odd = this.prev_jQ_input_Odd;
    c.prev_jQ_input_Even = this.prev_jQ_input_Even;
    c.prevQ2 = this.prevQ2;
    c.prevI2 = this.prevI2;
    c.Re = this.Re;
    c.Im = this.Im;
    c.I1ForOddPrev2 = this.I1ForOddPrev2;
    c.I1ForOddPrev3 = this.I1ForOddPrev3;
    c.I1ForEvenPrev2 = this.I1ForEvenPrev2;
    c.I1ForEvenPrev3 = this.I1ForEvenPrev3;
    c.rad2Deg = this.rad2Deg;
    c.smoothPeriod = this.smoothPeriod;
    c.streamParity = this.streamParity;
    c.ringPos_trailingWMAIdx = this.ringPos_trailingWMAIdx;
    c.ringCap_trailingWMAIdx = this.ringCap_trailingWMAIdx;
    c.ring_trailingWMAIdx_inReal = this.ring_trailingWMAIdx_inReal.slice();
    c.winPos_i = this.winPos_i;
    c.winCap_i = this.winCap_i;
    c.win_i_inReal = this.win_i_inReal.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function htTrendlineStepImpl(sp: HtTrendlineStream, inReal: number): void {
  let i = 0;
  let tempReal = 0;
  let tempReal2 = 0;
  let adjustedPrevPeriod = 0;
  let smoothedValue = 0;
  let hilbertTempReal = 0;
  let detrender = 0;
  let Q1 = 0;
  let jI = 0;
  let jQ = 0;
  let Q2 = 0;
  let I2 = 0;
  let todayValue = 0;
  let DCPeriodInt = 0;
  let DCPeriod = 0;
  let ringCapL_trailingWMAIdx = 0;
  sp.win_i_inReal[sp.winPos_i] = inReal;
  {
    const _a: number = 0.075, _b: number = sp.period, _c: number = 0.54;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    adjustedPrevPeriod = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  todayValue = inReal;
  sp.periodWMASub += todayValue;
  sp.periodWMASub -= sp.trailingWMAValue;
  sp.periodWMASum += todayValue * 4.0;
  sp.trailingWMAValue = sp.ring_trailingWMAIdx_inReal[sp.ringPos_trailingWMAIdx];
  smoothedValue = sp.periodWMASum * 0.1;
  sp.periodWMASum -= sp.periodWMASub;
  if (sp.streamParity === 0) {
    /* Do the Hilbert Transforms for even price bar */
    hilbertTempReal = sp.a * smoothedValue;
    detrender = -sp.detrender_Even[sp.hilbertIdx];
    sp.detrender_Even[sp.hilbertIdx] = hilbertTempReal;
    detrender += hilbertTempReal;
    detrender -= sp.prev_detrender_Even;
    sp.prev_detrender_Even = sp.b * sp.prev_detrender_input_Even;
    detrender += sp.prev_detrender_Even;
    sp.prev_detrender_input_Even = smoothedValue;
    detrender *= adjustedPrevPeriod;
    hilbertTempReal = sp.a * detrender;
    Q1 = -sp.Q1_Even[sp.hilbertIdx];
    sp.Q1_Even[sp.hilbertIdx] = hilbertTempReal;
    Q1 += hilbertTempReal;
    Q1 -= sp.prev_Q1_Even;
    sp.prev_Q1_Even = sp.b * sp.prev_Q1_input_Even;
    Q1 += sp.prev_Q1_Even;
    sp.prev_Q1_input_Even = detrender;
    Q1 *= adjustedPrevPeriod;
    hilbertTempReal = sp.a * sp.I1ForEvenPrev3;
    jI = -sp.jI_Even[sp.hilbertIdx];
    sp.jI_Even[sp.hilbertIdx] = hilbertTempReal;
    jI += hilbertTempReal;
    jI -= sp.prev_jI_Even;
    sp.prev_jI_Even = sp.b * sp.prev_jI_input_Even;
    jI += sp.prev_jI_Even;
    sp.prev_jI_input_Even = sp.I1ForEvenPrev3;
    jI *= adjustedPrevPeriod;
    hilbertTempReal = sp.a * Q1;
    jQ = -sp.jQ_Even[sp.hilbertIdx];
    sp.jQ_Even[sp.hilbertIdx] = hilbertTempReal;
    jQ += hilbertTempReal;
    jQ -= sp.prev_jQ_Even;
    sp.prev_jQ_Even = sp.b * sp.prev_jQ_input_Even;
    jQ += sp.prev_jQ_Even;
    sp.prev_jQ_input_Even = Q1;
    jQ *= adjustedPrevPeriod;
    if (++sp.hilbertIdx === 3) {
      sp.hilbertIdx = 0;
    }
    {
      const _a: number = 0.2, _b: number = Q1 + jI, _c: number = 0.8 * sp.prevQ2;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      Q2 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = 0.2, _b: number = sp.I1ForEvenPrev3 - jQ, _c: number = 0.8 * sp.prevI2;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      I2 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    /* The variable I1 is the detrender delayed for
     * 3 price bars.
     *
     * Save the current detrender value for being
     * used by the "odd" logic later.
     */
    sp.I1ForOddPrev3 = sp.I1ForOddPrev2;
    sp.I1ForOddPrev2 = detrender;
  } else {
    /* Do the Hilbert Transforms for odd price bar */
    hilbertTempReal = sp.a * smoothedValue;
    detrender = -sp.detrender_Odd[sp.hilbertIdx];
    sp.detrender_Odd[sp.hilbertIdx] = hilbertTempReal;
    detrender += hilbertTempReal;
    detrender -= sp.prev_detrender_Odd;
    sp.prev_detrender_Odd = sp.b * sp.prev_detrender_input_Odd;
    detrender += sp.prev_detrender_Odd;
    sp.prev_detrender_input_Odd = smoothedValue;
    detrender *= adjustedPrevPeriod;
    hilbertTempReal = sp.a * detrender;
    Q1 = -sp.Q1_Odd[sp.hilbertIdx];
    sp.Q1_Odd[sp.hilbertIdx] = hilbertTempReal;
    Q1 += hilbertTempReal;
    Q1 -= sp.prev_Q1_Odd;
    sp.prev_Q1_Odd = sp.b * sp.prev_Q1_input_Odd;
    Q1 += sp.prev_Q1_Odd;
    sp.prev_Q1_input_Odd = detrender;
    Q1 *= adjustedPrevPeriod;
    hilbertTempReal = sp.a * sp.I1ForOddPrev3;
    jI = -sp.jI_Odd[sp.hilbertIdx];
    sp.jI_Odd[sp.hilbertIdx] = hilbertTempReal;
    jI += hilbertTempReal;
    jI -= sp.prev_jI_Odd;
    sp.prev_jI_Odd = sp.b * sp.prev_jI_input_Odd;
    jI += sp.prev_jI_Odd;
    sp.prev_jI_input_Odd = sp.I1ForOddPrev3;
    jI *= adjustedPrevPeriod;
    hilbertTempReal = sp.a * Q1;
    jQ = -sp.jQ_Odd[sp.hilbertIdx];
    sp.jQ_Odd[sp.hilbertIdx] = hilbertTempReal;
    jQ += hilbertTempReal;
    jQ -= sp.prev_jQ_Odd;
    sp.prev_jQ_Odd = sp.b * sp.prev_jQ_input_Odd;
    jQ += sp.prev_jQ_Odd;
    sp.prev_jQ_input_Odd = Q1;
    jQ *= adjustedPrevPeriod;
    {
      const _a: number = 0.2, _b: number = Q1 + jI, _c: number = 0.8 * sp.prevQ2;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      Q2 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = 0.2, _b: number = sp.I1ForOddPrev3 - jQ, _c: number = 0.8 * sp.prevI2;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      I2 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    /* The varaiable I1 is the detrender delayed for
     * 3 price bars.
     *
     * Save the current detrender value for being
     * used by the "even" logic later.
     */
    sp.I1ForEvenPrev3 = sp.I1ForEvenPrev2;
    sp.I1ForEvenPrev2 = detrender;
  }
  /* Adjust the period for next price bar */
  let _fma17: number;
  {
    const _a: number = I2, _b: number = sp.prevI2, _c: number = Q2 * sp.prevQ2;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    _fma17 = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  {
    const _a: number = 0.8, _b: number = sp.Re, _c: number = 0.2 * _fma17;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.Re = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  {
    const _a: number = 0.8, _b: number = sp.Im, _c: number = 0.2 * (I2 * sp.prevQ2 - Q2 * sp.prevI2);
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.Im = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  sp.prevQ2 = Q2;
  sp.prevI2 = I2;
  tempReal = sp.period;
  if (sp.Im !== 0.0 && sp.Re !== 0.0) {
    sp.period = 360.0 / (Math.atan(sp.Im / sp.Re) * sp.rad2Deg);
  }
  tempReal2 = 1.5 * tempReal;
  if (sp.period > tempReal2) {
    sp.period = tempReal2;
  }
  tempReal2 = 0.67 * tempReal;
  if (sp.period < tempReal2) {
    sp.period = tempReal2;
  }
  if (sp.period < 6) {
    sp.period = 6;
  } else if (sp.period > 50) {
    sp.period = 50;
  }
  {
    const _a: number = 0.2, _b: number = sp.period, _c: number = 0.8 * tempReal;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.period = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  {
    const _a: number = 0.67, _b: number = sp.smoothPeriod, _c: number = 0.33 * sp.period;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.smoothPeriod = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  /* Compute Trendline */
  DCPeriod = sp.smoothPeriod + 0.5;
  /* A NaN period has no int value, and C leaves converting one undefined. */
  if (DCPeriod >= 0.0) {
    DCPeriodInt = (DCPeriod | 0);
  } else {
    DCPeriodInt = 0;
  }
  /* Average the RAW price over the dominant cycle period
   * (Ehlers, "Rocket Science for Traders": the Instantaneous
   * Trendline sums Price — not SmoothPrice, which only feeds
   * the Hilbert detrender above). See issue #88.
   */
  /* Sum the last DCPeriodInt (<= 50) raw prices. The fixed 50-iteration
   * loop with an inner guard is a streaming-friendly rewrite of the
   * data-dependent backward scan `for(i<DCPeriodInt) sum += inReal[idx--]`
   * (idx starting at today): identical terms in identical order, so
   * bit-for-bit unchanged, but the constant cap lets the rescan-window
   * machinery bound the window (DCPeriod is clamped to [6.5, 50.5]).
   */
  tempReal = 0.0;
  for (i = 0; i < 50; i += 1) {
    if (i < DCPeriodInt) {
      tempReal += sp.win_i_inReal[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i];
    }
  }
  if (DCPeriodInt > 0) {
    tempReal = tempReal / DCPeriodInt;
  }
  let _fma22: number;
  {
    const _a: number = 4.0, _b: number = tempReal, _c: number = 3.0 * sp.iTrend1;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    _fma22 = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  let _fma23: number;
  {
    const _a: number = 2.0, _b: number = sp.iTrend2, _c: number = _fma22;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    _fma23 = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  tempReal2 = (_fma23 + sp.iTrend3) / 10.0;
  sp.iTrend3 = sp.iTrend2;
  sp.iTrend2 = sp.iTrend1;
  sp.iTrend1 = tempReal;
  sp.cur_outReal = tempReal2;
  /* Ooof... let's do the next price bar now! */
  ringCapL_trailingWMAIdx = sp.ringCap_trailingWMAIdx;
  sp.ring_trailingWMAIdx_inReal[sp.ringPos_trailingWMAIdx] = inReal;
  sp.ringPos_trailingWMAIdx = sp.ringPos_trailingWMAIdx + 1;
  if (sp.ringPos_trailingWMAIdx >= ringCapL_trailingWMAIdx) {
    sp.ringPos_trailingWMAIdx = 0;
  }
  sp.winPos_i = sp.winPos_i + 1;
  if (sp.winPos_i >= sp.winCap_i) {
    sp.winPos_i = 0;
  }
  sp.streamParity = 1 - sp.streamParity;
}

function htTrendlineOpenImpl(
  sp: HtTrendlineStream,
  inReal: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let outIdx = 0;
  let i = 0;
  let lookbackTotal = 0;
  let today = 0;
  let tempReal = 0;
  let tempReal2 = 0;
  let adjustedPrevPeriod = 0;
  let period = 0;
  let trailingWMAIdx = 0;
  let periodWMASum = 0;
  let periodWMASub = 0;
  let trailingWMAValue = 0;
  let smoothedValue = 0;
  let iTrend1 = 0;
  let iTrend2 = 0;
  let iTrend3 = 0;
  let a = 0;
  let b = 0;
  let hilbertTempReal = 0;
  let hilbertIdx = 0;
  const detrender_Odd = new Float64Array(3);
  const detrender_Even = new Float64Array(3);
  let detrender = 0;
  let prev_detrender_Odd = 0;
  let prev_detrender_Even = 0;
  let prev_detrender_input_Odd = 0;
  let prev_detrender_input_Even = 0;
  const Q1_Odd = new Float64Array(3);
  const Q1_Even = new Float64Array(3);
  let Q1 = 0;
  let prev_Q1_Odd = 0;
  let prev_Q1_Even = 0;
  let prev_Q1_input_Odd = 0;
  let prev_Q1_input_Even = 0;
  const jI_Odd = new Float64Array(3);
  const jI_Even = new Float64Array(3);
  let jI = 0;
  let prev_jI_Odd = 0;
  let prev_jI_Even = 0;
  let prev_jI_input_Odd = 0;
  let prev_jI_input_Even = 0;
  const jQ_Odd = new Float64Array(3);
  const jQ_Even = new Float64Array(3);
  let jQ = 0;
  let prev_jQ_Odd = 0;
  let prev_jQ_Even = 0;
  let prev_jQ_input_Odd = 0;
  let prev_jQ_input_Even = 0;
  let Q2 = 0;
  let I2 = 0;
  let prevQ2 = 0;
  let prevI2 = 0;
  let Re = 0;
  let Im = 0;
  let I1ForOddPrev2 = 0;
  let I1ForOddPrev3 = 0;
  let I1ForEvenPrev2 = 0;
  let I1ForEvenPrev3 = 0;
  let rad2Deg = 0;
  let todayValue = 0;
  let smoothPeriod = 0;
  let DCPeriodInt = 0;
  let DCPeriod = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  a = 0.0962;
  b = 0.5769;
  /* Variable used for the price smoother (a weighted moving average). */
  /* Variable to keep track of the last 3 ITrend */
  /* Variables used for the Hilbert Transormation */
  /* Variable used to calculate the dominant cycle phase */
  /* circular buffer already declared */
  iTrend3 = 0.0;
  iTrend2 = iTrend3;
  iTrend1 = iTrend2;
  /* Constant */
  tempReal = Math.atan(1);
  rad2Deg = 45.0 / tempReal;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = htTrendlineLookbackPositional(core);
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
  outBegIdx.value = startIdx;
  /* Initialize the price smoother, which is simply a weighted
   * moving average of the price.
   * To understand this algorithm, I strongly suggest to understand
   * first how TA_WMA is done.
   */
  trailingWMAIdx = startIdx - lookbackTotal;
  today = trailingWMAIdx;
  /* Initialization is same as WMA, except loop is unrolled
   * for speed optimization.
   */
  tempReal = inReal[today++];
  periodWMASub = tempReal;
  periodWMASum = tempReal;
  tempReal = inReal[today++];
  periodWMASub += tempReal;
  periodWMASum += tempReal * 2.0;
  tempReal = inReal[today++];
  periodWMASub += tempReal;
  periodWMASum += tempReal * 3.0;
  trailingWMAValue = 0.0;
  /* Subsequent WMA value are evaluated by using
   * the DO_PRICE_WMA macro.
   */
  i = 34;
  do {
    tempReal = inReal[today++];
    periodWMASub += tempReal;
    periodWMASub -= trailingWMAValue;
    periodWMASum += tempReal * 4.0;
    trailingWMAValue = inReal[trailingWMAIdx++];
    smoothedValue = periodWMASum * 0.1;
    periodWMASum -= periodWMASub;
  } while (--i !== 0);
  /* Initialize the circular buffers used by the hilbert
   * transform logic.
   * A buffer is used for odd day and another for even days.
   * This minimize the number of memory access and floating point
   * operations needed (note also that by using static circular buffer,
   * no large dynamic memory allocation is needed for storing
   * intermediate calculation!).
   */
  hilbertIdx = 0;
  detrender_Odd[0] = 0.0;
  detrender_Odd[1] = 0.0;
  detrender_Odd[2] = 0.0;
  detrender_Even[0] = 0.0;
  detrender_Even[1] = 0.0;
  detrender_Even[2] = 0.0;
  detrender = 0.0;
  prev_detrender_Odd = 0.0;
  prev_detrender_Even = 0.0;
  prev_detrender_input_Odd = 0.0;
  prev_detrender_input_Even = 0.0;
  Q1_Odd[0] = 0.0;
  Q1_Odd[1] = 0.0;
  Q1_Odd[2] = 0.0;
  Q1_Even[0] = 0.0;
  Q1_Even[1] = 0.0;
  Q1_Even[2] = 0.0;
  Q1 = 0.0;
  prev_Q1_Odd = 0.0;
  prev_Q1_Even = 0.0;
  prev_Q1_input_Odd = 0.0;
  prev_Q1_input_Even = 0.0;
  jI_Odd[0] = 0.0;
  jI_Odd[1] = 0.0;
  jI_Odd[2] = 0.0;
  jI_Even[0] = 0.0;
  jI_Even[1] = 0.0;
  jI_Even[2] = 0.0;
  jI = 0.0;
  prev_jI_Odd = 0.0;
  prev_jI_Even = 0.0;
  prev_jI_input_Odd = 0.0;
  prev_jI_input_Even = 0.0;
  jQ_Odd[0] = 0.0;
  jQ_Odd[1] = 0.0;
  jQ_Odd[2] = 0.0;
  jQ_Even[0] = 0.0;
  jQ_Even[1] = 0.0;
  jQ_Even[2] = 0.0;
  jQ = 0.0;
  prev_jQ_Odd = 0.0;
  prev_jQ_Even = 0.0;
  prev_jQ_input_Odd = 0.0;
  prev_jQ_input_Even = 0.0;
  period = 0.0;
  outIdx = 0;
  prevQ2 = 0.0;
  prevI2 = prevQ2;
  Im = 0.0;
  Re = Im;
  I1ForEvenPrev3 = 0.0;
  I1ForOddPrev3 = I1ForEvenPrev3;
  I1ForEvenPrev2 = 0.0;
  I1ForOddPrev2 = I1ForEvenPrev2;
  smoothPeriod = 0.0;
  /* The code is speed optimized and is most likely very
   * hard to follow if you do not already know well the
   * original algorithm.
   * To understadn better, it is strongly suggested to look
   * first at the Excel implementation in "test_MAMA.xls" included
   * in this package.
   */
  while (today <= endIdx) {
    {
      const _a: number = 0.075, _b: number = period, _c: number = 0.54;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      adjustedPrevPeriod = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    todayValue = inReal[today];
    periodWMASub += todayValue;
    periodWMASub -= trailingWMAValue;
    periodWMASum += todayValue * 4.0;
    trailingWMAValue = inReal[trailingWMAIdx++];
    smoothedValue = periodWMASum * 0.1;
    periodWMASum -= periodWMASub;
    if (((today % 2) | 0) === 0) {
      /* Do the Hilbert Transforms for even price bar */
      hilbertTempReal = a * smoothedValue;
      detrender = -detrender_Even[hilbertIdx];
      detrender_Even[hilbertIdx] = hilbertTempReal;
      detrender += hilbertTempReal;
      detrender -= prev_detrender_Even;
      prev_detrender_Even = b * prev_detrender_input_Even;
      detrender += prev_detrender_Even;
      prev_detrender_input_Even = smoothedValue;
      detrender *= adjustedPrevPeriod;
      hilbertTempReal = a * detrender;
      Q1 = -Q1_Even[hilbertIdx];
      Q1_Even[hilbertIdx] = hilbertTempReal;
      Q1 += hilbertTempReal;
      Q1 -= prev_Q1_Even;
      prev_Q1_Even = b * prev_Q1_input_Even;
      Q1 += prev_Q1_Even;
      prev_Q1_input_Even = detrender;
      Q1 *= adjustedPrevPeriod;
      hilbertTempReal = a * I1ForEvenPrev3;
      jI = -jI_Even[hilbertIdx];
      jI_Even[hilbertIdx] = hilbertTempReal;
      jI += hilbertTempReal;
      jI -= prev_jI_Even;
      prev_jI_Even = b * prev_jI_input_Even;
      jI += prev_jI_Even;
      prev_jI_input_Even = I1ForEvenPrev3;
      jI *= adjustedPrevPeriod;
      hilbertTempReal = a * Q1;
      jQ = -jQ_Even[hilbertIdx];
      jQ_Even[hilbertIdx] = hilbertTempReal;
      jQ += hilbertTempReal;
      jQ -= prev_jQ_Even;
      prev_jQ_Even = b * prev_jQ_input_Even;
      jQ += prev_jQ_Even;
      prev_jQ_input_Even = Q1;
      jQ *= adjustedPrevPeriod;
      if (++hilbertIdx === 3) {
        hilbertIdx = 0;
      }
      {
        const _a: number = 0.2, _b: number = Q1 + jI, _c: number = 0.8 * prevQ2;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        Q2 = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      {
        const _a: number = 0.2, _b: number = I1ForEvenPrev3 - jQ, _c: number = 0.8 * prevI2;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        I2 = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      /* The variable I1 is the detrender delayed for
       * 3 price bars.
       *
       * Save the current detrender value for being
       * used by the "odd" logic later.
       */
      I1ForOddPrev3 = I1ForOddPrev2;
      I1ForOddPrev2 = detrender;
    } else {
      /* Do the Hilbert Transforms for odd price bar */
      hilbertTempReal = a * smoothedValue;
      detrender = -detrender_Odd[hilbertIdx];
      detrender_Odd[hilbertIdx] = hilbertTempReal;
      detrender += hilbertTempReal;
      detrender -= prev_detrender_Odd;
      prev_detrender_Odd = b * prev_detrender_input_Odd;
      detrender += prev_detrender_Odd;
      prev_detrender_input_Odd = smoothedValue;
      detrender *= adjustedPrevPeriod;
      hilbertTempReal = a * detrender;
      Q1 = -Q1_Odd[hilbertIdx];
      Q1_Odd[hilbertIdx] = hilbertTempReal;
      Q1 += hilbertTempReal;
      Q1 -= prev_Q1_Odd;
      prev_Q1_Odd = b * prev_Q1_input_Odd;
      Q1 += prev_Q1_Odd;
      prev_Q1_input_Odd = detrender;
      Q1 *= adjustedPrevPeriod;
      hilbertTempReal = a * I1ForOddPrev3;
      jI = -jI_Odd[hilbertIdx];
      jI_Odd[hilbertIdx] = hilbertTempReal;
      jI += hilbertTempReal;
      jI -= prev_jI_Odd;
      prev_jI_Odd = b * prev_jI_input_Odd;
      jI += prev_jI_Odd;
      prev_jI_input_Odd = I1ForOddPrev3;
      jI *= adjustedPrevPeriod;
      hilbertTempReal = a * Q1;
      jQ = -jQ_Odd[hilbertIdx];
      jQ_Odd[hilbertIdx] = hilbertTempReal;
      jQ += hilbertTempReal;
      jQ -= prev_jQ_Odd;
      prev_jQ_Odd = b * prev_jQ_input_Odd;
      jQ += prev_jQ_Odd;
      prev_jQ_input_Odd = Q1;
      jQ *= adjustedPrevPeriod;
      {
        const _a: number = 0.2, _b: number = Q1 + jI, _c: number = 0.8 * prevQ2;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        Q2 = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      {
        const _a: number = 0.2, _b: number = I1ForOddPrev3 - jQ, _c: number = 0.8 * prevI2;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        I2 = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      /* The varaiable I1 is the detrender delayed for
       * 3 price bars.
       *
       * Save the current detrender value for being
       * used by the "even" logic later.
       */
      I1ForEvenPrev3 = I1ForEvenPrev2;
      I1ForEvenPrev2 = detrender;
    }
    /* Adjust the period for next price bar */
    let _fma29: number;
    {
      const _a: number = I2, _b: number = prevI2, _c: number = Q2 * prevQ2;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      _fma29 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = 0.8, _b: number = Re, _c: number = 0.2 * _fma29;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      Re = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = 0.8, _b: number = Im, _c: number = 0.2 * (I2 * prevQ2 - Q2 * prevI2);
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      Im = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    prevQ2 = Q2;
    prevI2 = I2;
    tempReal = period;
    if (Im !== 0.0 && Re !== 0.0) {
      period = 360.0 / (Math.atan(Im / Re) * rad2Deg);
    }
    tempReal2 = 1.5 * tempReal;
    if (period > tempReal2) {
      period = tempReal2;
    }
    tempReal2 = 0.67 * tempReal;
    if (period < tempReal2) {
      period = tempReal2;
    }
    if (period < 6) {
      period = 6;
    } else if (period > 50) {
      period = 50;
    }
    {
      const _a: number = 0.2, _b: number = period, _c: number = 0.8 * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      period = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = 0.67, _b: number = smoothPeriod, _c: number = 0.33 * period;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      smoothPeriod = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    /* Compute Trendline */
    DCPeriod = smoothPeriod + 0.5;
    /* A NaN period has no int value, and C leaves converting one undefined. */
    if (DCPeriod >= 0.0) {
      DCPeriodInt = (DCPeriod | 0);
    } else {
      DCPeriodInt = 0;
    }
    /* Average the RAW price over the dominant cycle period
     * (Ehlers, "Rocket Science for Traders": the Instantaneous
     * Trendline sums Price — not SmoothPrice, which only feeds
     * the Hilbert detrender above). See issue #88.
     */
    /* Sum the last DCPeriodInt (<= 50) raw prices. The fixed 50-iteration
     * loop with an inner guard is a streaming-friendly rewrite of the
     * data-dependent backward scan `for(i<DCPeriodInt) sum += inReal[idx--]`
     * (idx starting at today): identical terms in identical order, so
     * bit-for-bit unchanged, but the constant cap lets the rescan-window
     * machinery bound the window (DCPeriod is clamped to [6.5, 50.5]).
     */
    tempReal = 0.0;
    for (i = 0; i < 50; i += 1) {
      if (i < DCPeriodInt) {
        tempReal += inReal[today - i];
      }
    }
    if (DCPeriodInt > 0) {
      tempReal = tempReal / DCPeriodInt;
    }
    let _fma34: number;
    {
      const _a: number = 4.0, _b: number = tempReal, _c: number = 3.0 * iTrend1;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      _fma34 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    let _fma35: number;
    {
      const _a: number = 2.0, _b: number = iTrend2, _c: number = _fma34;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      _fma35 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    tempReal2 = (_fma35 + iTrend3) / 10.0;
    iTrend3 = iTrend2;
    iTrend2 = iTrend1;
    iTrend1 = tempReal;
    if (today >= startIdx) {
      outReal[((outIdx++ * outStride) | 0)] = tempReal2;
    }
    /* Ooof... let's do the next price bar now! */
    today += 1;
  }
  outNBElement.value = outIdx;
  const cap_trailingWMAIdx = today - trailingWMAIdx;
  if (cap_trailingWMAIdx < 1 || cap_trailingWMAIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_trailingWMAIdx = cap_trailingWMAIdx > 0 ? cap_trailingWMAIdx : 1;
  const capRing_trailingWMAIdx_inReal = new Float64Array(allocN_trailingWMAIdx);
  capRing_trailingWMAIdx_inReal.set(inReal.subarray(historyLen - cap_trailingWMAIdx, historyLen), 0);
  const cap_i = 50;
  if (cap_i < 1 || cap_i > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const capWin_i_inReal = inReal.slice(historyLen - cap_i, historyLen);
  sp.period = period;
  sp.periodWMASum = periodWMASum;
  sp.periodWMASub = periodWMASub;
  sp.trailingWMAValue = trailingWMAValue;
  sp.iTrend1 = iTrend1;
  sp.iTrend2 = iTrend2;
  sp.iTrend3 = iTrend3;
  sp.a = a;
  sp.b = b;
  sp.hilbertIdx = hilbertIdx;
  sp.detrender_Odd = detrender_Odd;
  sp.detrender_Even = detrender_Even;
  sp.prev_detrender_Odd = prev_detrender_Odd;
  sp.prev_detrender_Even = prev_detrender_Even;
  sp.prev_detrender_input_Odd = prev_detrender_input_Odd;
  sp.prev_detrender_input_Even = prev_detrender_input_Even;
  sp.Q1_Odd = Q1_Odd;
  sp.Q1_Even = Q1_Even;
  sp.prev_Q1_Odd = prev_Q1_Odd;
  sp.prev_Q1_Even = prev_Q1_Even;
  sp.prev_Q1_input_Odd = prev_Q1_input_Odd;
  sp.prev_Q1_input_Even = prev_Q1_input_Even;
  sp.jI_Odd = jI_Odd;
  sp.jI_Even = jI_Even;
  sp.prev_jI_Odd = prev_jI_Odd;
  sp.prev_jI_Even = prev_jI_Even;
  sp.prev_jI_input_Odd = prev_jI_input_Odd;
  sp.prev_jI_input_Even = prev_jI_input_Even;
  sp.jQ_Odd = jQ_Odd;
  sp.jQ_Even = jQ_Even;
  sp.prev_jQ_Odd = prev_jQ_Odd;
  sp.prev_jQ_Even = prev_jQ_Even;
  sp.prev_jQ_input_Odd = prev_jQ_input_Odd;
  sp.prev_jQ_input_Even = prev_jQ_input_Even;
  sp.prevQ2 = prevQ2;
  sp.prevI2 = prevI2;
  sp.Re = Re;
  sp.Im = Im;
  sp.I1ForOddPrev2 = I1ForOddPrev2;
  sp.I1ForOddPrev3 = I1ForOddPrev3;
  sp.I1ForEvenPrev2 = I1ForEvenPrev2;
  sp.I1ForEvenPrev3 = I1ForEvenPrev3;
  sp.rad2Deg = rad2Deg;
  sp.smoothPeriod = smoothPeriod;
  sp.streamParity = historyLen % 2;
  sp.ringPos_trailingWMAIdx = 0;
  sp.ringCap_trailingWMAIdx = cap_trailingWMAIdx;
  sp.ring_trailingWMAIdx_inReal = capRing_trailingWMAIdx_inReal;
  sp.winPos_i = 0;
  sp.winCap_i = cap_i;
  sp.win_i_inReal = capWin_i_inReal;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function htTrendlineOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): HtTrendlineStream {
  const sp = HtTrendlineStream.of(core);
  const retCode = htTrendlineOpenImpl(sp, inReal, startIdx, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('HT_TRENDLINE openAndFill', inReal.length, startIdx, htTrendlineLookbackPositional(core));
  }
  throw streamFailure('HT_TRENDLINE openAndFill', retCode);
}

/** @internal */
export function htTrendlineOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
): HtTrendlineStream {
  const sp = HtTrendlineStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = htTrendlineOpenImpl(sp, inReal, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('HT_TRENDLINE open', inReal.length, startIdx, htTrendlineLookbackPositional(core));
  }
  throw streamFailure('HT_TRENDLINE open', retCode);
}

/** @internal */
export function htTrendlineOpenPositional(
  inReal: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): HtTrendlineStream {
  requireArgument('HT_TRENDLINE open', 'real', inReal);
  requireHistory('HT_TRENDLINE open', 'real', inReal.length);
  return htTrendlineOpenInternal(core, float64(inReal), 0);
}

/** @internal */
export function htTrendlineOpenAndFillPositional(
  inReal: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): HtTrendlineStream {
  requireArgument('HT_TRENDLINE openAndFill', 'real', inReal);
  requireHistory('HT_TRENDLINE openAndFill', 'real', inReal.length);
  const guardOutLen = openFillCount('HT_TRENDLINE openAndFill', inReal.length, htTrendlineLookbackPositional(core));
  requireOutput('HT_TRENDLINE openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('HT_TRENDLINE openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('HT_TRENDLINE openAndFill', 'outReal', 'real');
  }
  return htTrendlineOpenAndFillInternal(core, inRealF64, 0, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live HT_TRENDLINE stream over the warm-up history: `value()` starts at
 * the last history bar, bit-identical to {@link htTrendline} there.
 *
 * @param real - Warm-up history. Source price series.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link HtTrendlineOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `htTrendlineLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a `Core`
 *   or an absent input.
 *
 * @example
 * ```ts
 * import { htTrendlineOpen } from 'ta-lib';
 *
 * const series = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = htTrendlineOpen(series);
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function htTrendlineOpen(real: ArrayLike<number>, options?: HtTrendlineOptions): HtTrendlineStream {
  const o = optionsOf('HT_TRENDLINE open', options);
  const core = coreOf('HT_TRENDLINE open', o.core);
  return htTrendlineOpenPositional(real, core);
}

/**
 * {@link htTrendlineOpen} that also computes the outputs over the whole history in
 * the same pass, bit-identically to {@link htTrendline}.
 *
 * @param real - Warm-up history. Source price series.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link HtTrendlineOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link htTrendline} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `htTrendlineLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input or a buffer too short, of the wrong type or starting
 *   where another buffer or a `Float64Array` input starts, or, on a history long
 *   enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { htTrendlineOpenAndFill } from 'ta-lib';
 *
 * const series = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = htTrendlineOpenAndFill(series);
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function htTrendlineOpenAndFill(real: ArrayLike<number>, options?: HtTrendlineOptions): HtTrendlineResult & { stream: HtTrendlineStream } {
  const o = optionsOf('HT_TRENDLINE openAndFill', options);
  const core = coreOf('HT_TRENDLINE openAndFill', o.core);
  const capacity = fillCapacity(htTrendlineLookbackPositional(core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = htTrendlineOpenAndFillPositional(real, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
