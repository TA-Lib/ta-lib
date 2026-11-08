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
 *  120802 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 */

import { FuncUnstId } from '../enums.ts';
import { badSink, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function htSineLookbackPositional(core: Core = Core.DEFAULT): number {
  /* 31 input are skip
   * +32 output are skip to account for misc lookback
   * ---
   *  63 Total Lookback
   *
   * 31 is for being compatible with Tradestation.
   * See mama_lookback for an explanation of the "32".
   */
  return 63 + core.unstableCount(FuncUnstId.HT_SINE, (80 + ((50 * 4) | 0)), (80 + ((50 * 8) | 0)));
}

/** @internal */
export function htSineDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 2) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function htSineImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outSine: Float64Array,
  outLeadSine: Float64Array,
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
  let deg2Rad = 0;
  let constDeg2RadBy360 = 0;
  let todayValue = 0;
  let smoothPeriod = 0;
  let smoothPrice!: Float64Array;
  let smoothPrice_Idx = 0;
  let maxIdx_smoothPrice = (50) - 1;
  let idx = 0;
  let DCPeriodInt = 0;
  let DCPhase = 0;
  let DCPeriod = 0;
  let imagPart = 0;
  let realPart = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if ((outSine.buffer === outLeadSine.buffer && outSine.byteOffset === outLeadSine.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  a = 0.0962;
  b = 0.5769;
  /* Variable used for the price smoother (a weighted moving average). */
  /* Variables used for the Hilbert Transormation */
  /* Varaible used to keep track of the previous
   * smooth price. In the case of this algorithm,
   * we will never need more than 50 values.
   */
  smoothPrice = new Float64Array(maxIdx_smoothPrice + 1);
  /* Variable used to calculate the dominant cycle phase */
  /* circular buffer already declared */
  /* The following could be replaced by constant eventually. */
  tempReal = Math.atan(1);
  rad2Deg = 45.0 / tempReal;
  deg2Rad = 1.0 / rad2Deg;
  constDeg2RadBy360 = tempReal * 8.0;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = htSineLookbackPositional(core);
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
  for (i = 0; i < 50; i += 1) {
    smoothPrice[i] = 0.0;
  }
  /* The code is speed optimized and is most likely very
   * hard to follow if you do not already know well the
   * original algorithm.
   * To understadn better, it is strongly suggested to look
   * first at the Excel implementation in "test_MAMA.xls" included
   * in this package.
   */
  DCPhase = 0.0;
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
    /* Remember the smoothedValue into the smoothPrice
     * circular buffer.
     */
    smoothPrice[smoothPrice_Idx] = smoothedValue;
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
    /* Compute Dominant Cycle Phase */
    DCPeriod = smoothPeriod + 0.5;
    /* A NaN period has no int value, and C leaves converting one undefined. */
    if (DCPeriod >= 0.0) {
      DCPeriodInt = (DCPeriod | 0);
    } else {
      DCPeriodInt = 0;
    }
    realPart = 0.0;
    imagPart = 0.0;
    /* idx is used to iterate for up to 50 of the last
     * value of smoothPrice.
     */
    idx = smoothPrice_Idx;
    for (i = 0; i < DCPeriodInt; i += 1) {
      tempReal = i * constDeg2RadBy360 / DCPeriodInt;
      tempReal2 = smoothPrice[idx];
      realPart += Math.sin(tempReal) * tempReal2;
      imagPart += Math.cos(tempReal) * tempReal2;
      if (idx === 0) {
        idx = 50 - 1;
      } else {
        idx -= 1;
      }
    }
    tempReal = Math.abs(imagPart);
    if (tempReal > 0.0) {
      DCPhase = Math.atan(realPart / imagPart) * rad2Deg;
    } else if (tempReal <= 0.01) {
      if (realPart < 0.0) {
        DCPhase -= 90.0;
      } else if (realPart > 0.0) {
        DCPhase += 90.0;
      }
    }
    DCPhase += 90.0;
    /* Compensate for one bar lag of the weighted moving average */
    DCPhase += 360.0 / smoothPeriod;
    if (imagPart < 0.0) {
      DCPhase += 180.0;
    }
    if (DCPhase > 315.0) {
      DCPhase -= 360.0;
    }
    if (today >= startIdx) {
      outSine[outIdx] = Math.sin(DCPhase * deg2Rad);
      outLeadSine[outIdx++] = Math.sin((DCPhase + 45) * deg2Rad);
    }
    /* Ooof... let's do the next price bar now! */
    smoothPrice_Idx++;
    if (smoothPrice_Idx > maxIdx_smoothPrice) {
      smoothPrice_Idx = 0;
    }
    today += 1;
  }
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function htSinePositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  outSine: Float64Array,
  outLeadSine: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('HT_SINE', 'startIdx', startIdx);
  endIdx = requireInt32('HT_SINE', 'endIdx', endIdx);
  requireIndexRange('HT_SINE', startIdx, endIdx);
  const guardStart = clampedStart('HT_SINE', startIdx, htSineLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('HT_SINE', 'real', inReal, guardInLen);
  requireOutput('HT_SINE', 'outSine', outSine, guardOutLen, Float64Array);
  requireOutput('HT_SINE', 'outLeadSine', outLeadSine, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outSine, inRealF64)) {
    throw sharedMemory('HT_SINE', 'outSine', 'real');
  }
  if (overlapsPartly(outLeadSine, inRealF64)) {
    throw sharedMemory('HT_SINE', 'outLeadSine', 'real');
  }
  if (overlaps(outSine, outLeadSine)) {
    throw sharedMemory('HT_SINE', 'outSine', 'outLeadSine');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = htSineImpl(core, startIdx, endIdx, inRealF64, outBegIdx, outNBElement, outSine, outLeadSine);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('HT_SINE', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link htSine}, {@link htSineLookback}, {@link htSineDisplayShift},
 * {@link htSineOpen} and {@link htSineOpenAndFill}. Every key is optional: an
 * omitted or `undefined` key takes its default, and a call ignores the keys it has
 * no use for.
 */
export interface HtSineOptions {
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
   * Buffer for `outSine`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outSine?: Float64Array<ArrayBuffer> | undefined;
  /**
   * Buffer for `outLeadSine`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outLeadSine?: Float64Array<ArrayBuffer> | undefined;
}

/** What {@link htSine} returns, and {@link htSineOpenAndFill} with its stream. */
export interface HtSineResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Sine of the dominant-cycle phase. `outSine[k]` is the value at input bar
   * `begIdx + k`.
   */
  outSine: Float64Array<ArrayBuffer>;
  /**
   * Sine of the phase advanced 45 degrees (lead). `outLeadSine[k]` is the value at
   * input bar `begIdx + k`.
   */
  outLeadSine: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link htSine} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link HtSineOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function htSineLookback(options?: HtSineOptions): number {
  const o = optionsOf('HT_SINE lookback', options);
  return htSineLookbackPositional(coreOf('HT_SINE lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link htSine}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link HtSineOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function htSineDisplayShift(outputIdx: number, options?: HtSineOptions): number {
  const o = optionsOf('HT_SINE displayShift', options);
  return htSineDisplayShiftPositional(outputIdx, coreOf('HT_SINE displayShift', o.core));
}

/**
 * Hilbert Transform SineWave: derives the dominant-cycle phase from price and
 * emits its sine plus a 45-degree-lead sine. The two curves cross near cycle
 * turning points. outSine and outLeadSine crossing marks cycle turning points.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ht_sine](https://ta-lib.org/functions/ht_sine).
 *
 * `outSine[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link htSineLookback} is a success with empty outputs.
 *
 * @param real - Source price series.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link HtSineOptions}.
 * @returns `begIdx` and the values; see {@link HtSineResult}. Each output is a
 *   view of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, an input too short for the range, a buffer that is empty, too short or of
 *   the wrong type, two buffers sharing memory or a buffer sharing memory with a
 *   `Float64Array` input it is not. Checked before anything is written.
 *
 * @example
 * ```ts
 * import { htSine } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, outSine, outLeadSine } = htSine(data);
 * // outSine[k] is the value at bar begIdx + k
 * ```
 * @see [`htDcphase`](https://ta-lib.org/functions/ht_dcphase)
 * @see [`htDcperiod`](https://ta-lib.org/functions/ht_dcperiod)
 * @see [`htPhasor`](https://ta-lib.org/functions/ht_phasor)
 * @see [`htTrendmode`](https://ta-lib.org/functions/ht_trendmode)
 * @see [`mama`](https://ta-lib.org/functions/mama)
 */
export function htSine(real: ArrayLike<number>, options?: HtSineOptions): HtSineResult {
  const o = optionsOf('HT_SINE', options);
  if (o.endIdx === undefined) {
    requireSeries('HT_SINE', 'real', real);
  }
  const core = coreOf('HT_SINE', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, htSineLookbackPositional(core), real);
  const outSine = o.outSine === undefined ? new Float64Array(capacity) : o.outSine;
  const outLeadSine = o.outLeadSine === undefined ? new Float64Array(capacity) : o.outLeadSine;
  const range = htSinePositional(startIdx, endIdx, real, outSine, outLeadSine, core);
  return { begIdx: range.begIdx, outSine: outSine.subarray(0, range.count), outLeadSine: outLeadSine.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live HT_SINE stream: one value per closed bar, bit-identical to {@link htSine}
 * over the same series. Open it with {@link htSineOpen} or
 * {@link htSineOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ht_sine](https://ta-lib.org/functions/ht_sine).
 */
export class HtSineStream {
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
  deg2Rad: number = 0;
  /** @internal */
  constDeg2RadBy360: number = 0;
  /** @internal */
  smoothPeriod: number = 0;
  /** @internal */
  DCPhase: number = 0;
  /** @internal */
  smoothPrice_Idx: number = 0;
  /** @internal */
  maxIdx_smoothPrice: number = 0;
  /** @internal */
  streamParity: number = 0;
  /** @internal */
  ringPos_trailingWMAIdx: number = 0;
  /** @internal */
  ringCap_trailingWMAIdx: number = 0;
  /** @internal */
  ring_trailingWMAIdx_inReal: Float64Array = new Float64Array(1);
  /** @internal */
  cbSize_smoothPrice: number = 0;
  /** @internal */
  cb_smoothPrice: Float64Array = new Float64Array(1);
  /** @internal */
  cur_outSine: number = 0;
  /** @internal */
  cur_outLeadSine: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): HtSineStream {
    return new HtSineStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link htSine} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('HT_SINE advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns its outputs.
   *
   * @param real - The bar's `real` value: Source price series.
   * @param out - Receives the outputs and is returned; a new {@link HtSineOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included, before anything moves.
   *   `TALibError` with code `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached
   *   bar `Core.INDEX_MAX`.
   */
  update(real: number, out?: HtSineOut): HtSineOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('HT_SINE update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('HT_SINE update', 'HtSineOut');
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('HT_SINE update', 'real');
    }
    htSineStepImpl(this, real);
    this.outRangeCount++;
    if (out === undefined) {
      out = new HtSineOut();
    }
    out.outSine = this.cur_outSine;
    out.outLeadSine = this.cur_outLeadSine;
    return out;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: Source price series.
   * @param out - Receives the outputs and is returned; a new {@link HtSineOut} when
   *   omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included.
   */
  peek(real: number, out?: HtSineOut): HtSineOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('HT_SINE peek', 'HtSineOut');
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('HT_SINE peek', 'real');
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
    let idx = 0;
    let DCPeriodInt = 0;
    let DCPeriod = 0;
    let imagPart = 0;
    let realPart = 0;
    let DCPhase = sp.DCPhase;
    let I1ForEvenPrev2 = sp.I1ForEvenPrev2;
    let I1ForEvenPrev3 = sp.I1ForEvenPrev3;
    let I1ForOddPrev2 = sp.I1ForOddPrev2;
    let I1ForOddPrev3 = sp.I1ForOddPrev3;
    let Im = sp.Im;
    let Re = sp.Re;
    let cur_outLeadSine = 0;
    let cur_outSine = 0;
    let hilbertIdx = sp.hilbertIdx;
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
    /* Remember the smoothedValue into the smoothPrice
     * circular buffer.
     */
    pkSlot0 = sp.smoothPrice_Idx;
    pkVal0 = smoothedValue;
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
    /* Compute Dominant Cycle Phase */
    DCPeriod = smoothPeriod + 0.5;
    /* A NaN period has no int value, and C leaves converting one undefined. */
    if (DCPeriod >= 0.0) {
      DCPeriodInt = (DCPeriod | 0);
    } else {
      DCPeriodInt = 0;
    }
    realPart = 0.0;
    imagPart = 0.0;
    /* idx is used to iterate for up to 50 of the last
     * value of smoothPrice.
     */
    idx = sp.smoothPrice_Idx;
    for (i = 0; i < DCPeriodInt; i += 1) {
      tempReal = i * sp.constDeg2RadBy360 / DCPeriodInt;
      tempReal2 = (idx !== pkSlot0) ? sp.cb_smoothPrice[idx] : pkVal0;
      realPart += Math.sin(tempReal) * tempReal2;
      imagPart += Math.cos(tempReal) * tempReal2;
      if (idx === 0) {
        idx = 50 - 1;
      } else {
        idx -= 1;
      }
    }
    tempReal = Math.abs(imagPart);
    if (tempReal > 0.0) {
      DCPhase = Math.atan(realPart / imagPart) * sp.rad2Deg;
    } else if (tempReal <= 0.01) {
      if (realPart < 0.0) {
        DCPhase -= 90.0;
      } else if (realPart > 0.0) {
        DCPhase += 90.0;
      }
    }
    DCPhase += 90.0;
    /* Compensate for one bar lag of the weighted moving average */
    DCPhase += 360.0 / smoothPeriod;
    if (imagPart < 0.0) {
      DCPhase += 180.0;
    }
    if (DCPhase > 315.0) {
      DCPhase -= 360.0;
    }
    cur_outSine = Math.sin(DCPhase * sp.deg2Rad);
    cur_outLeadSine = Math.sin((DCPhase + 45) * sp.deg2Rad);
    if (out === undefined) {
      out = new HtSineOut();
    }
    out.outSine = cur_outSine;
    out.outLeadSine = cur_outLeadSine;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link HtSineOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a sink that is not an object,
   *   `null` included.
   */
  value(out?: HtSineOut): HtSineOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('HT_SINE value', 'HtSineOut');
    }
    if (out === undefined) {
      out = new HtSineOut();
    }
    out.outSine = this.cur_outSine;
    out.outLeadSine = this.cur_outLeadSine;
    return out;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): HtSineStream {
    const c = new HtSineStream(this.core);
    c.period = this.period;
    c.periodWMASum = this.periodWMASum;
    c.periodWMASub = this.periodWMASub;
    c.trailingWMAValue = this.trailingWMAValue;
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
    c.deg2Rad = this.deg2Rad;
    c.constDeg2RadBy360 = this.constDeg2RadBy360;
    c.smoothPeriod = this.smoothPeriod;
    c.DCPhase = this.DCPhase;
    c.smoothPrice_Idx = this.smoothPrice_Idx;
    c.maxIdx_smoothPrice = this.maxIdx_smoothPrice;
    c.streamParity = this.streamParity;
    c.ringPos_trailingWMAIdx = this.ringPos_trailingWMAIdx;
    c.ringCap_trailingWMAIdx = this.ringCap_trailingWMAIdx;
    c.ring_trailingWMAIdx_inReal = this.ring_trailingWMAIdx_inReal.slice();
    c.cbSize_smoothPrice = this.cbSize_smoothPrice;
    c.cb_smoothPrice = this.cb_smoothPrice.slice();
    c.cur_outSine = this.cur_outSine;
    c.cur_outLeadSine = this.cur_outLeadSine;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one HT_SINE bar, as `update`, `peek` and `value` return them.
 * Passed back to one of them, it is filled and returned again, with no allocation;
 * it holds its values until the next call that writes it. A new one holds `NaN`.
 */
export class HtSineOut {
  /** Sine of the dominant-cycle phase. */
  outSine: number = NaN;
  /** Sine of the phase advanced 45 degrees (lead) */
  outLeadSine: number = NaN;
}

function htSineStepImpl(sp: HtSineStream, inReal: number): void {
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
  let idx = 0;
  let DCPeriodInt = 0;
  let DCPeriod = 0;
  let imagPart = 0;
  let realPart = 0;
  let ringCapL_trailingWMAIdx = 0;
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
  /* Remember the smoothedValue into the smoothPrice
   * circular buffer.
   */
  sp.cb_smoothPrice[sp.smoothPrice_Idx] = smoothedValue;
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
  let _fma15: number;
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
    _fma15 = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  {
    const _a: number = 0.8, _b: number = sp.Re, _c: number = 0.2 * _fma15;
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
  /* Compute Dominant Cycle Phase */
  DCPeriod = sp.smoothPeriod + 0.5;
  /* A NaN period has no int value, and C leaves converting one undefined. */
  if (DCPeriod >= 0.0) {
    DCPeriodInt = (DCPeriod | 0);
  } else {
    DCPeriodInt = 0;
  }
  realPart = 0.0;
  imagPart = 0.0;
  /* idx is used to iterate for up to 50 of the last
   * value of smoothPrice.
   */
  idx = sp.smoothPrice_Idx;
  for (i = 0; i < DCPeriodInt; i += 1) {
    tempReal = i * sp.constDeg2RadBy360 / DCPeriodInt;
    tempReal2 = sp.cb_smoothPrice[idx];
    realPart += Math.sin(tempReal) * tempReal2;
    imagPart += Math.cos(tempReal) * tempReal2;
    if (idx === 0) {
      idx = 50 - 1;
    } else {
      idx -= 1;
    }
  }
  tempReal = Math.abs(imagPart);
  if (tempReal > 0.0) {
    sp.DCPhase = Math.atan(realPart / imagPart) * sp.rad2Deg;
  } else if (tempReal <= 0.01) {
    if (realPart < 0.0) {
      sp.DCPhase -= 90.0;
    } else if (realPart > 0.0) {
      sp.DCPhase += 90.0;
    }
  }
  sp.DCPhase += 90.0;
  /* Compensate for one bar lag of the weighted moving average */
  sp.DCPhase += 360.0 / sp.smoothPeriod;
  if (imagPart < 0.0) {
    sp.DCPhase += 180.0;
  }
  if (sp.DCPhase > 315.0) {
    sp.DCPhase -= 360.0;
  }
  sp.cur_outSine = Math.sin(sp.DCPhase * sp.deg2Rad);
  sp.cur_outLeadSine = Math.sin((sp.DCPhase + 45) * sp.deg2Rad);
  /* Ooof... let's do the next price bar now! */
  sp.smoothPrice_Idx = sp.smoothPrice_Idx + 1;
  if (sp.smoothPrice_Idx > sp.maxIdx_smoothPrice) {
    sp.smoothPrice_Idx = 0;
  }
  ringCapL_trailingWMAIdx = sp.ringCap_trailingWMAIdx;
  sp.ring_trailingWMAIdx_inReal[sp.ringPos_trailingWMAIdx] = inReal;
  sp.ringPos_trailingWMAIdx = sp.ringPos_trailingWMAIdx + 1;
  if (sp.ringPos_trailingWMAIdx >= ringCapL_trailingWMAIdx) {
    sp.ringPos_trailingWMAIdx = 0;
  }
  sp.streamParity = 1 - sp.streamParity;
}

function htSineOpenImpl(
  sp: HtSineStream,
  inReal: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outSine: Float64Array,
  outLeadSine: Float64Array,
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
  let deg2Rad = 0;
  let constDeg2RadBy360 = 0;
  let todayValue = 0;
  let smoothPeriod = 0;
  let smoothPrice!: Float64Array;
  let smoothPrice_Idx = 0;
  let maxIdx_smoothPrice = (50) - 1;
  let idx = 0;
  let DCPeriodInt = 0;
  let DCPhase = 0;
  let DCPeriod = 0;
  let imagPart = 0;
  let realPart = 0;
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
  /* Variables used for the Hilbert Transormation */
  /* Varaible used to keep track of the previous
   * smooth price. In the case of this algorithm,
   * we will never need more than 50 values.
   */
  smoothPrice = new Float64Array(maxIdx_smoothPrice + 1);
  /* Variable used to calculate the dominant cycle phase */
  /* circular buffer already declared */
  /* The following could be replaced by constant eventually. */
  tempReal = Math.atan(1);
  rad2Deg = 45.0 / tempReal;
  deg2Rad = 1.0 / rad2Deg;
  constDeg2RadBy360 = tempReal * 8.0;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = htSineLookbackPositional(core);
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
  for (i = 0; i < 50; i += 1) {
    smoothPrice[i] = 0.0;
  }
  /* The code is speed optimized and is most likely very
   * hard to follow if you do not already know well the
   * original algorithm.
   * To understadn better, it is strongly suggested to look
   * first at the Excel implementation in "test_MAMA.xls" included
   * in this package.
   */
  DCPhase = 0.0;
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
    /* Remember the smoothedValue into the smoothPrice
     * circular buffer.
     */
    smoothPrice[smoothPrice_Idx] = smoothedValue;
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
    let _fma25: number;
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
      _fma25 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = 0.8, _b: number = Re, _c: number = 0.2 * _fma25;
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
    /* Compute Dominant Cycle Phase */
    DCPeriod = smoothPeriod + 0.5;
    /* A NaN period has no int value, and C leaves converting one undefined. */
    if (DCPeriod >= 0.0) {
      DCPeriodInt = (DCPeriod | 0);
    } else {
      DCPeriodInt = 0;
    }
    realPart = 0.0;
    imagPart = 0.0;
    /* idx is used to iterate for up to 50 of the last
     * value of smoothPrice.
     */
    idx = smoothPrice_Idx;
    for (i = 0; i < DCPeriodInt; i += 1) {
      tempReal = i * constDeg2RadBy360 / DCPeriodInt;
      tempReal2 = smoothPrice[idx];
      realPart += Math.sin(tempReal) * tempReal2;
      imagPart += Math.cos(tempReal) * tempReal2;
      if (idx === 0) {
        idx = 50 - 1;
      } else {
        idx -= 1;
      }
    }
    tempReal = Math.abs(imagPart);
    if (tempReal > 0.0) {
      DCPhase = Math.atan(realPart / imagPart) * rad2Deg;
    } else if (tempReal <= 0.01) {
      if (realPart < 0.0) {
        DCPhase -= 90.0;
      } else if (realPart > 0.0) {
        DCPhase += 90.0;
      }
    }
    DCPhase += 90.0;
    /* Compensate for one bar lag of the weighted moving average */
    DCPhase += 360.0 / smoothPeriod;
    if (imagPart < 0.0) {
      DCPhase += 180.0;
    }
    if (DCPhase > 315.0) {
      DCPhase -= 360.0;
    }
    if (today >= startIdx) {
      outSine[((outIdx * outStride) | 0)] = Math.sin(DCPhase * deg2Rad);
      outLeadSine[((outIdx++ * outStride) | 0)] = Math.sin((DCPhase + 45) * deg2Rad);
    }
    /* Ooof... let's do the next price bar now! */
    smoothPrice_Idx++;
    if (smoothPrice_Idx > maxIdx_smoothPrice) {
      smoothPrice_Idx = 0;
    }
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
  const capCb_smoothPrice = maxIdx_smoothPrice + 1;
  if (capCb_smoothPrice > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sp.period = period;
  sp.periodWMASum = periodWMASum;
  sp.periodWMASub = periodWMASub;
  sp.trailingWMAValue = trailingWMAValue;
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
  sp.deg2Rad = deg2Rad;
  sp.constDeg2RadBy360 = constDeg2RadBy360;
  sp.smoothPeriod = smoothPeriod;
  sp.DCPhase = DCPhase;
  sp.smoothPrice_Idx = smoothPrice_Idx;
  sp.maxIdx_smoothPrice = maxIdx_smoothPrice;
  sp.streamParity = historyLen % 2;
  sp.ringPos_trailingWMAIdx = 0;
  sp.ringCap_trailingWMAIdx = cap_trailingWMAIdx;
  sp.ring_trailingWMAIdx_inReal = capRing_trailingWMAIdx_inReal;
  sp.cbSize_smoothPrice = capCb_smoothPrice;
  sp.cb_smoothPrice = smoothPrice;
  sp.cur_outSine = outSine[(outNBElement.value - 1) * outStride];
  sp.cur_outLeadSine = outLeadSine[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function htSineOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outSine: Float64Array,
  outLeadSine: Float64Array,
): HtSineStream {
  const sp = HtSineStream.of(core);
  const retCode = htSineOpenImpl(sp, inReal, startIdx, outBegIdx, outNBElement, outSine, outLeadSine, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('HT_SINE openAndFill', inReal.length, startIdx, htSineLookbackPositional(core));
  }
  throw streamFailure('HT_SINE openAndFill', retCode);
}

/** @internal */
export function htSineOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
): HtSineStream {
  const sp = HtSineStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outSine = new Float64Array(1);
  const sink_outLeadSine = new Float64Array(1);
  const retCode = htSineOpenImpl(sp, inReal, startIdx, outBegIdx, outNBElement, sink_outSine, sink_outLeadSine, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('HT_SINE open', inReal.length, startIdx, htSineLookbackPositional(core));
  }
  throw streamFailure('HT_SINE open', retCode);
}

/** @internal */
export function htSineOpenPositional(
  inReal: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): HtSineStream {
  requireArgument('HT_SINE open', 'real', inReal);
  requireHistory('HT_SINE open', 'real', inReal.length);
  return htSineOpenInternal(core, float64(inReal), 0);
}

/** @internal */
export function htSineOpenAndFillPositional(
  inReal: ArrayLike<number>,
  outSine: Float64Array,
  outLeadSine: Float64Array,
  core: Core = Core.DEFAULT,
): HtSineStream {
  requireArgument('HT_SINE openAndFill', 'real', inReal);
  requireHistory('HT_SINE openAndFill', 'real', inReal.length);
  const guardOutLen = openFillCount('HT_SINE openAndFill', inReal.length, htSineLookbackPositional(core));
  requireOutput('HT_SINE openAndFill', 'outSine', outSine, guardOutLen, Float64Array);
  requireOutput('HT_SINE openAndFill', 'outLeadSine', outLeadSine, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outSine.buffer === inRealF64.buffer && outSine.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('HT_SINE openAndFill', 'outSine', 'real');
  }
  if (outLeadSine.buffer === inRealF64.buffer && outLeadSine.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('HT_SINE openAndFill', 'outLeadSine', 'real');
  }
  if (outSine.buffer === outLeadSine.buffer && outSine.byteOffset === outLeadSine.byteOffset) {
    throw sharedMemory('HT_SINE openAndFill', 'outSine', 'outLeadSine');
  }
  if (guardOutLen > 0 && overlaps(outSine, inRealF64)) {
    throw sharedMemory('HT_SINE openAndFill', 'outSine', 'real');
  }
  if (guardOutLen > 0 && overlaps(outLeadSine, inRealF64)) {
    throw sharedMemory('HT_SINE openAndFill', 'outLeadSine', 'real');
  }
  if (guardOutLen > 0 && overlaps(outSine, outLeadSine)) {
    throw sharedMemory('HT_SINE openAndFill', 'outSine', 'outLeadSine');
  }
  return htSineOpenAndFillInternal(core, inRealF64, 0, new MInteger(), new MInteger(), outSine, outLeadSine);
}

/**
 * Opens a live HT_SINE stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link htSine} there.
 *
 * @param real - Warm-up history. Source price series.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link HtSineOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `htSineLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core` or an absent
 *   input.
 *
 * @example
 * ```ts
 * import { htSineOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = htSineOpen(data);
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function htSineOpen(real: ArrayLike<number>, options?: HtSineOptions): HtSineStream {
  const o = optionsOf('HT_SINE open', options);
  const core = coreOf('HT_SINE open', o.core);
  return htSineOpenPositional(real, core);
}

/**
 * {@link htSineOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link htSine}.
 *
 * @param real - Warm-up history. Source price series.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link HtSineOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link htSine} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `htSineLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input or a buffer too short, of the wrong type or starting where another buffer
 *   or a `Float64Array` input starts, or, on a history long enough to open, one
 *   sharing memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { htSineOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const {
 *   stream,
 *   begIdx,
 *   outSine,
 *   outLeadSine,
 * } = htSineOpenAndFill(data);
 * // outSine[k] is the value at bar begIdx + k
 * ```
 */
export function htSineOpenAndFill(real: ArrayLike<number>, options?: HtSineOptions): HtSineResult & { stream: HtSineStream } {
  const o = optionsOf('HT_SINE openAndFill', options);
  const core = coreOf('HT_SINE openAndFill', o.core);
  const capacity = fillCapacity(htSineLookbackPositional(core), real);
  const outSine = o.outSine === undefined ? new Float64Array(capacity) : o.outSine;
  const outLeadSine = o.outLeadSine === undefined ? new Float64Array(capacity) : o.outLeadSine;
  const stream = htSineOpenAndFillPositional(real, outSine, outLeadSine, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outSine: outSine.subarray(0, range.count), outLeadSine: outLeadSine.subarray(0, range.count) };
}
