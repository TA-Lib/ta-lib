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
 *  023003 MF   Initial Coding of MAMA.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 */

import { FuncUnstId } from '../enums.ts';
import { badSink, clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireReal, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link mama} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param optInFastLimit - Upper bound on the adaptive smoothing factor (default
 *   0.5; range 0.01 to 0.99; `Core.REAL_DEFAULT` selects the default).
 * @param optInSlowLimit - Lower bound on the adaptive smoothing factor (default
 *   0.05; range 0.01 to 0.99; `Core.REAL_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function mamaLookback(optInFastLimit: number, optInSlowLimit: number, core: Core = Core.DEFAULT): number {
  if (typeof optInFastLimit !== 'number') {
    return -1;
  }
  if (typeof optInSlowLimit !== 'number') {
    return -1;
  }
  if (optInFastLimit === Core.REAL_DEFAULT) {
    optInFastLimit = 0.5;
  } else if (!(optInFastLimit >= 1e-2 && optInFastLimit <= 9.9e-1)) {
    return -1;
  }
  if (optInSlowLimit === Core.REAL_DEFAULT) {
    optInSlowLimit = 0.05;
  } else if (!(optInSlowLimit >= 1e-2 && optInSlowLimit <= 9.9e-1)) {
    return -1;
  }
  let limit = 0;
  let count4 = 0;
  let count8 = 0;
  /* ceil( 2*K / max(fast, slow) ) per level, in this order of operations: the
   * count is defined as this double expression, not as the real quotient.
   */
  limit = (optInFastLimit > optInSlowLimit) ? optInFastLimit : optInSlowLimit;
  count4 = (Math.ceil(20.0 / limit) | 0);
  count8 = (Math.ceil(38.0 / limit) | 0);
  /* Lookback is a fix amount + the unstable period.
   *
   *
   * The fix lookback is 32 and is establish as follow:
   *
   *         12 price bar to be compatible with the implementation
   *            of TradeStation found in John Ehlers book.
   *          6 price bars for the Detrender
   *          6 price bars for Q1
   *          3 price bars for jI
   *          3 price bars for jQ
   *          1 price bar for Re/Im
   *          1 price bar for the Delta Phase
   *        -------
   *         32 Total
   */
  return 32 + core.unstableCount(FuncUnstId.MAMA, (80 + ((50 * 4) | 0)) + count4, (80 + ((50 * 8) | 0)) + count8);
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link mama}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInFastLimit - Upper bound on the adaptive smoothing factor (default
 *   0.5; range 0.01 to 0.99; `Core.REAL_DEFAULT` selects the default).
 * @param optInSlowLimit - Lower bound on the adaptive smoothing factor (default
 *   0.05; range 0.01 to 0.99; `Core.REAL_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function mamaDisplayShift(optInFastLimit: number, optInSlowLimit: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (mamaLookback(optInFastLimit, optInSlowLimit, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 2) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function mamaImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInFastLimit: number,
  optInSlowLimit: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outMAMA: Float64Array,
  outFAMA: Float64Array,
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
  let mama = 0;
  let fama = 0;
  let todayValue = 0;
  let prevPhase = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInFastLimit === Core.REAL_DEFAULT) {
    optInFastLimit = 0.5;
  } else if (!(optInFastLimit >= 1e-2 && optInFastLimit <= 9.9e-1)) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowLimit === Core.REAL_DEFAULT) {
    optInSlowLimit = 0.05;
  } else if (!(optInSlowLimit >= 1e-2 && optInSlowLimit <= 9.9e-1)) {
    return RetCode.BAD_PARAM;
  }
  if ((outMAMA.buffer === outFAMA.buffer && outMAMA.byteOffset === outFAMA.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  a = 0.0962;
  b = 0.5769;
  /* Variable used for the price smoother (a weighted moving average). */
  /* Variables used for the Hilbert Transormation */
  /* Constant */
  rad2Deg = 180.0 / (4.0 * Math.atan(1));
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = mamaLookback(optInFastLimit, optInSlowLimit, core);
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
  i = 9;
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
  fama = 0.0;
  mama = fama;
  I1ForEvenPrev3 = 0.0;
  I1ForOddPrev3 = I1ForEvenPrev3;
  I1ForEvenPrev2 = 0.0;
  I1ForOddPrev2 = I1ForEvenPrev2;
  prevPhase = 0.0;
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
      /* Put Alpha in tempReal2 */
      if (I1ForEvenPrev3 !== 0.0) {
        tempReal2 = Math.atan(Q1 / I1ForEvenPrev3) * rad2Deg;
      } else {
        tempReal2 = 0.0;
      }
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
       * used by the "odd" logic later.
       */
      I1ForEvenPrev3 = I1ForEvenPrev2;
      I1ForEvenPrev2 = detrender;
      /* Put Alpha in tempReal2 */
      if (I1ForOddPrev3 !== 0.0) {
        tempReal2 = Math.atan(Q1 / I1ForOddPrev3) * rad2Deg;
      } else {
        tempReal2 = 0.0;
      }
    }
    /* Put Delta Phase into tempReal */
    tempReal = prevPhase - tempReal2;
    prevPhase = tempReal2;
    if (tempReal < 1.0) {
      tempReal = 1.0;
    }
    /* Put Alpha into tempReal */
    if (tempReal > 1.0) {
      tempReal = optInFastLimit / tempReal;
      if (tempReal < optInSlowLimit) {
        tempReal = optInSlowLimit;
      }
    } else {
      tempReal = optInFastLimit;
    }
    /* Calculate MAMA, FAMA */
    {
      const _a: number = 1 - tempReal, _b: number = mama, _c: number = tempReal * todayValue;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      mama = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    tempReal *= 0.5;
    {
      const _a: number = 1 - tempReal, _b: number = fama, _c: number = tempReal * mama;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      fama = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    if (today >= startIdx) {
      outFAMA[outIdx] = fama;
      outMAMA[outIdx++] = mama;
    }
    /* Adjust the period for next price bar */
    let _fma7: number;
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
      _fma7 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = 0.8, _b: number = Re, _c: number = 0.2 * _fma7;
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
    /* Ooof... let's do the next price bar now! */
    today += 1;
  }
  /* Default return values */
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * MESA Adaptive Moving Average: an adaptive EMA whose smoothing factor is driven
 * by the dominant-cycle phase rate measured with a Hilbert transform. Emits two
 * lines, MAMA and its slower follower FAMA. MAMA crossing above FAMA is bullish;
 * crossing below is bearish.
 *
 * Formula and more info at
 * [ta-lib.org/functions/mama](https://ta-lib.org/functions/mama).
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link mamaLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - Price series to smooth.
 * @param optInFastLimit - Upper bound on the adaptive smoothing factor (default
 *   0.5; range 0.01 to 0.99; `Core.REAL_DEFAULT` selects the default).
 * @param optInSlowLimit - Lower bound on the adaptive smoothing factor (default
 *   0.05; range 0.01 to 0.99; `Core.REAL_DEFAULT` selects the default).
 * @param outMAMA - Adaptive moving average (fast line) Must hold the values
 *   produced and never be empty; the input length always suffices.
 * @param outFAMA - Following adaptive moving average, using half the alpha (slow
 *   line) Must hold the values produced and never be empty; the input length always
 *   suffices.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The range written: `begIdx` is the first bar with a value, `count` how
 *   many were written.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for an argument that is not an int32, a
 *   real parameter that is not a number, a parameter outside its range, an array too
 *   short for the range, an empty output, two outputs sharing memory or an output
 *   sharing memory with a `Float64Array` input it is not. Checked before anything is
 *   written.
 *
 * @example
 * ```ts
 * import { mama } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const mamaOut = new Float64Array(data.length);
 * const fama = new Float64Array(data.length);
 * const { begIdx, count } = mama(
 *   0, data.length - 1,
 *   data,
 *   0.5, 0.05,
 *   mamaOut, fama,
 * );
 * // mamaOut[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`ma`](https://ta-lib.org/functions/ma)
 * @see [`wma`](https://ta-lib.org/functions/wma)
 * @see [`htDcperiod`](https://ta-lib.org/functions/ht_dcperiod)
 */
export function mama(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInFastLimit: number,
  optInSlowLimit: number,
  outMAMA: Float64Array,
  outFAMA: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('MAMA', 'startIdx', startIdx);
  endIdx = requireInt32('MAMA', 'endIdx', endIdx);
  optInFastLimit = requireReal('MAMA', 'optInFastLimit', optInFastLimit);
  optInSlowLimit = requireReal('MAMA', 'optInSlowLimit', optInSlowLimit);
  requireIndexRange('MAMA', startIdx, endIdx);
  const guardStart = clampedStart('MAMA', startIdx, mamaLookback(optInFastLimit, optInSlowLimit, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('MAMA', 'inReal', inReal, guardInLen);
  requireLength('MAMA', 'outMAMA', outMAMA, guardOutLen);
  requireLength('MAMA', 'outFAMA', outFAMA, guardOutLen);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outMAMA, inRealF64) || overlapsPartly(outFAMA, inRealF64) || overlaps(outMAMA, outFAMA)) {
    throw failure('MAMA', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = mamaImpl(core, startIdx, endIdx, inRealF64, optInFastLimit, optInSlowLimit, outBegIdx, outNBElement, outMAMA, outFAMA);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('MAMA', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live MAMA stream: one value per closed bar, bit-identical to {@link mama} over
 * the same series. Open it with {@link mamaOpen} or {@link mamaOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/mama](https://ta-lib.org/functions/mama).
 */
export class MamaStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInFastLimit: number = 0;
  /** @internal */
  optInSlowLimit: number = 0;
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
  mama: number = 0;
  /** @internal */
  fama: number = 0;
  /** @internal */
  prevPhase: number = 0;
  /** @internal */
  streamParity: number = 0;
  /** @internal */
  ringPos_trailingWMAIdx: number = 0;
  /** @internal */
  ringCap_trailingWMAIdx: number = 0;
  /** @internal */
  ring_trailingWMAIdx_inReal: Float64Array = new Float64Array(1);
  /** @internal */
  cur_outMAMA: number = 0;
  /** @internal */
  cur_outFAMA: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): MamaStream {
    return new MamaStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link mama} reports them over the same bars. Each accepted `update` and each
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
      throw failure('MAMA advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar, writing the new values into `out`.
   *
   * @param inReal - The bar's `inReal` value: Price series to smooth.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number, out: MamaOut): void {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('MAMA update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (typeof out !== 'object' || out === null) {
      throw badSink('MAMA update', 'MamaOut');
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('MAMA update', 'inReal');
    }
    mamaStepImpl(this, inReal);
    this.outRangeCount++;
    out.mama = this.cur_outMAMA;
    out.fama = this.cur_outFAMA;
  }

  /**
   * Writes into `out` what the next `update` with the same bar would write,
   * committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Price series to smooth.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number, out: MamaOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('MAMA peek', 'MamaOut');
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('MAMA peek', 'inReal');
    }
    const sp = this;
    let tempReal = 0;
    let tempReal2 = 0;
    let adjustedPrevPeriod = 0;
    let smoothedValue = 0;
    let hilbertTempReal = 0;
    let detrender = 0;
    let Q1 = 0;
    let todayValue = 0;
    let I1ForEvenPrev2 = sp.I1ForEvenPrev2;
    let I1ForEvenPrev3 = sp.I1ForEvenPrev3;
    let I1ForOddPrev2 = sp.I1ForOddPrev2;
    let I1ForOddPrev3 = sp.I1ForOddPrev3;
    let cur_outFAMA = 0;
    let cur_outMAMA = 0;
    let fama = sp.fama;
    let hilbertIdx = sp.hilbertIdx;
    let mama = sp.mama;
    let periodWMASub = sp.periodWMASub;
    let periodWMASum = sp.periodWMASum;
    let prevPhase = sp.prevPhase;
    let prev_Q1_Even = sp.prev_Q1_Even;
    let prev_Q1_Odd = sp.prev_Q1_Odd;
    let prev_Q1_input_Even = sp.prev_Q1_input_Even;
    let prev_Q1_input_Odd = sp.prev_Q1_input_Odd;
    let prev_detrender_Even = sp.prev_detrender_Even;
    let prev_detrender_Odd = sp.prev_detrender_Odd;
    let prev_detrender_input_Even = sp.prev_detrender_input_Even;
    let prev_detrender_input_Odd = sp.prev_detrender_input_Odd;
    let trailingWMAValue = sp.trailingWMAValue;
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
      hilbertTempReal = sp.a * Q1;
      if (++hilbertIdx === 3) {
        hilbertIdx = 0;
      }
      /* The variable I1 is the detrender delayed for
       * 3 price bars.
       *
       * Save the current detrender value for being
       * used by the "odd" logic later.
       */
      I1ForOddPrev3 = I1ForOddPrev2;
      I1ForOddPrev2 = detrender;
      /* Put Alpha in tempReal2 */
      if (I1ForEvenPrev3 !== 0.0) {
        tempReal2 = Math.atan(Q1 / I1ForEvenPrev3) * sp.rad2Deg;
      } else {
        tempReal2 = 0.0;
      }
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
      hilbertTempReal = sp.a * Q1;
      /* The varaiable I1 is the detrender delayed for
       * 3 price bars.
       *
       * Save the current detrender value for being
       * used by the "odd" logic later.
       */
      I1ForEvenPrev3 = I1ForEvenPrev2;
      I1ForEvenPrev2 = detrender;
      /* Put Alpha in tempReal2 */
      if (I1ForOddPrev3 !== 0.0) {
        tempReal2 = Math.atan(Q1 / I1ForOddPrev3) * sp.rad2Deg;
      } else {
        tempReal2 = 0.0;
      }
    }
    /* Put Delta Phase into tempReal */
    tempReal = prevPhase - tempReal2;
    prevPhase = tempReal2;
    if (tempReal < 1.0) {
      tempReal = 1.0;
    }
    /* Put Alpha into tempReal */
    if (tempReal > 1.0) {
      tempReal = sp.optInFastLimit / tempReal;
      if (tempReal < sp.optInSlowLimit) {
        tempReal = sp.optInSlowLimit;
      }
    } else {
      tempReal = sp.optInFastLimit;
    }
    /* Calculate MAMA, FAMA */
    {
      const _a: number = 1 - tempReal, _b: number = mama, _c: number = tempReal * todayValue;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      mama = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    tempReal *= 0.5;
    {
      const _a: number = 1 - tempReal, _b: number = fama, _c: number = tempReal * mama;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      fama = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    cur_outFAMA = fama;
    cur_outMAMA = mama;
    out.mama = cur_outMAMA;
    out.fama = cur_outFAMA;
  }

  /**
   * The value at the last bar this stream counted; `peek` does not change it.
   *
   * @param out - Receives the outputs.
   */
  value(out: MamaOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('MAMA value', 'MamaOut');
    }
    out.mama = this.cur_outMAMA;
    out.fama = this.cur_outFAMA;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): MamaStream {
    const c = new MamaStream(this.core);
    c.optInFastLimit = this.optInFastLimit;
    c.optInSlowLimit = this.optInSlowLimit;
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
    c.mama = this.mama;
    c.fama = this.fama;
    c.prevPhase = this.prevPhase;
    c.streamParity = this.streamParity;
    c.ringPos_trailingWMAIdx = this.ringPos_trailingWMAIdx;
    c.ringCap_trailingWMAIdx = this.ringCap_trailingWMAIdx;
    c.ring_trailingWMAIdx_inReal = this.ring_trailingWMAIdx_inReal.slice();
    c.cur_outMAMA = this.cur_outMAMA;
    c.cur_outFAMA = this.cur_outFAMA;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one MAMA bar, written into an object the caller owns and reuses:
 * valid until the next call that writes it.
 */
export class MamaOut {
  /** Adaptive moving average (fast line) */
  mama: number = 0;
  /** Following adaptive moving average, using half the alpha (slow line) */
  fama: number = 0;
}

function mamaStepImpl(sp: MamaStream, inReal: number): void {
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
    /* Put Alpha in tempReal2 */
    if (sp.I1ForEvenPrev3 !== 0.0) {
      tempReal2 = Math.atan(Q1 / sp.I1ForEvenPrev3) * sp.rad2Deg;
    } else {
      tempReal2 = 0.0;
    }
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
     * used by the "odd" logic later.
     */
    sp.I1ForEvenPrev3 = sp.I1ForEvenPrev2;
    sp.I1ForEvenPrev2 = detrender;
    /* Put Alpha in tempReal2 */
    if (sp.I1ForOddPrev3 !== 0.0) {
      tempReal2 = Math.atan(Q1 / sp.I1ForOddPrev3) * sp.rad2Deg;
    } else {
      tempReal2 = 0.0;
    }
  }
  /* Put Delta Phase into tempReal */
  tempReal = sp.prevPhase - tempReal2;
  sp.prevPhase = tempReal2;
  if (tempReal < 1.0) {
    tempReal = 1.0;
  }
  /* Put Alpha into tempReal */
  if (tempReal > 1.0) {
    tempReal = sp.optInFastLimit / tempReal;
    if (tempReal < sp.optInSlowLimit) {
      tempReal = sp.optInSlowLimit;
    }
  } else {
    tempReal = sp.optInFastLimit;
  }
  /* Calculate MAMA, FAMA */
  {
    const _a: number = 1 - tempReal, _b: number = sp.mama, _c: number = tempReal * todayValue;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.mama = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  tempReal *= 0.5;
  {
    const _a: number = 1 - tempReal, _b: number = sp.fama, _c: number = tempReal * sp.mama;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.fama = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  sp.cur_outFAMA = sp.fama;
  sp.cur_outMAMA = sp.mama;
  /* Adjust the period for next price bar */
  let _fma10: number;
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
    _fma10 = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  {
    const _a: number = 0.8, _b: number = sp.Re, _c: number = 0.2 * _fma10;
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
  /* Ooof... let's do the next price bar now! */
  ringCapL_trailingWMAIdx = sp.ringCap_trailingWMAIdx;
  sp.ring_trailingWMAIdx_inReal[sp.ringPos_trailingWMAIdx] = inReal;
  sp.ringPos_trailingWMAIdx = sp.ringPos_trailingWMAIdx + 1;
  if (sp.ringPos_trailingWMAIdx >= ringCapL_trailingWMAIdx) {
    sp.ringPos_trailingWMAIdx = 0;
  }
  sp.streamParity = 1 - sp.streamParity;
}

function mamaOpenImpl(
  sp: MamaStream,
  inReal: Float64Array,
  startIdx: number,
  optInFastLimit: number,
  optInSlowLimit: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outMAMA: Float64Array,
  outFAMA: Float64Array,
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
  let mama = 0;
  let fama = 0;
  let todayValue = 0;
  let prevPhase = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInFastLimit === Core.REAL_DEFAULT) {
    optInFastLimit = 0.5;
  } else if (!(optInFastLimit >= 1e-2 && optInFastLimit <= 9.9e-1)) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowLimit === Core.REAL_DEFAULT) {
    optInSlowLimit = 0.05;
  } else if (!(optInSlowLimit >= 1e-2 && optInSlowLimit <= 9.9e-1)) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  a = 0.0962;
  b = 0.5769;
  /* Variable used for the price smoother (a weighted moving average). */
  /* Variables used for the Hilbert Transormation */
  /* Constant */
  rad2Deg = 180.0 / (4.0 * Math.atan(1));
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = mamaLookback(optInFastLimit, optInSlowLimit, core);
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
  i = 9;
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
  fama = 0.0;
  mama = fama;
  I1ForEvenPrev3 = 0.0;
  I1ForOddPrev3 = I1ForEvenPrev3;
  I1ForEvenPrev2 = 0.0;
  I1ForOddPrev2 = I1ForEvenPrev2;
  prevPhase = 0.0;
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
      /* Put Alpha in tempReal2 */
      if (I1ForEvenPrev3 !== 0.0) {
        tempReal2 = Math.atan(Q1 / I1ForEvenPrev3) * rad2Deg;
      } else {
        tempReal2 = 0.0;
      }
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
       * used by the "odd" logic later.
       */
      I1ForEvenPrev3 = I1ForEvenPrev2;
      I1ForEvenPrev2 = detrender;
      /* Put Alpha in tempReal2 */
      if (I1ForOddPrev3 !== 0.0) {
        tempReal2 = Math.atan(Q1 / I1ForOddPrev3) * rad2Deg;
      } else {
        tempReal2 = 0.0;
      }
    }
    /* Put Delta Phase into tempReal */
    tempReal = prevPhase - tempReal2;
    prevPhase = tempReal2;
    if (tempReal < 1.0) {
      tempReal = 1.0;
    }
    /* Put Alpha into tempReal */
    if (tempReal > 1.0) {
      tempReal = optInFastLimit / tempReal;
      if (tempReal < optInSlowLimit) {
        tempReal = optInSlowLimit;
      }
    } else {
      tempReal = optInFastLimit;
    }
    /* Calculate MAMA, FAMA */
    {
      const _a: number = 1 - tempReal, _b: number = mama, _c: number = tempReal * todayValue;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      mama = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    tempReal *= 0.5;
    {
      const _a: number = 1 - tempReal, _b: number = fama, _c: number = tempReal * mama;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      fama = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    if (today >= startIdx) {
      outFAMA[((outIdx * outStride) | 0)] = fama;
      outMAMA[((outIdx++ * outStride) | 0)] = mama;
    }
    /* Adjust the period for next price bar */
    let _fma21: number;
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
      _fma21 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = 0.8, _b: number = Re, _c: number = 0.2 * _fma21;
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
    /* Ooof... let's do the next price bar now! */
    today += 1;
  }
  /* Default return values */
  outNBElement.value = outIdx;
  const cap_trailingWMAIdx = today - trailingWMAIdx;
  if (cap_trailingWMAIdx < 1 || cap_trailingWMAIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_trailingWMAIdx = cap_trailingWMAIdx > 0 ? cap_trailingWMAIdx : 1;
  const capRing_trailingWMAIdx_inReal = new Float64Array(allocN_trailingWMAIdx);
  capRing_trailingWMAIdx_inReal.set(inReal.subarray(historyLen - cap_trailingWMAIdx, historyLen), 0);
  sp.optInFastLimit = optInFastLimit;
  sp.optInSlowLimit = optInSlowLimit;
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
  sp.mama = mama;
  sp.fama = fama;
  sp.prevPhase = prevPhase;
  sp.streamParity = historyLen % 2;
  sp.ringPos_trailingWMAIdx = 0;
  sp.ringCap_trailingWMAIdx = cap_trailingWMAIdx;
  sp.ring_trailingWMAIdx_inReal = capRing_trailingWMAIdx_inReal;
  sp.cur_outMAMA = outMAMA[(outNBElement.value - 1) * outStride];
  sp.cur_outFAMA = outFAMA[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function mamaOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInFastLimit: number,
  optInSlowLimit: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outMAMA: Float64Array,
  outFAMA: Float64Array,
): MamaStream {
  const sp = MamaStream.of(core);
  const retCode = mamaOpenImpl(sp, inReal, startIdx, optInFastLimit, optInSlowLimit, outBegIdx, outNBElement, outMAMA, outFAMA, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MAMA openAndFill', inReal.length, startIdx, mamaLookback(optInFastLimit, optInSlowLimit, core));
  }
  throw streamFailure('MAMA openAndFill', retCode);
}

/** @internal */
export function mamaOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInFastLimit: number,
  optInSlowLimit: number,
): MamaStream {
  const sp = MamaStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outMAMA = new Float64Array(1);
  const sink_outFAMA = new Float64Array(1);
  const retCode = mamaOpenImpl(sp, inReal, startIdx, optInFastLimit, optInSlowLimit, outBegIdx, outNBElement, sink_outMAMA, sink_outFAMA, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MAMA open', inReal.length, startIdx, mamaLookback(optInFastLimit, optInSlowLimit, core));
  }
  throw streamFailure('MAMA open', retCode);
}

/**
 * Opens a live MAMA stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link mama} there.
 *
 * @param inReal - Warm-up history. Price series to smooth.
 * @param optInFastLimit - Upper bound on the adaptive smoothing factor (default
 *   0.5; range 0.01 to 0.99; `Core.REAL_DEFAULT` selects the default).
 * @param optInSlowLimit - Lower bound on the adaptive smoothing factor (default
 *   0.05; range 0.01 to 0.99; `Core.REAL_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `mamaLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for a real parameter that is not a number or a parameter
 *   outside its range.
 *
 * @example
 * ```ts
 * import { MamaOut, mamaOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = mamaOpen(data, 0.5, 0.05);
 * const peeked = new MamaOut();
 * const updated = new MamaOut();
 * stream.peek(100.9, peeked); // a forming bar: commits nothing
 * stream.update(100.9, updated); // the bar closes
 * ```
 */
export function mamaOpen(
  inReal: ArrayLike<number>,
  optInFastLimit: number,
  optInSlowLimit: number,
  core: Core = Core.DEFAULT,
): MamaStream {
  optInFastLimit = requireReal('MAMA open', 'optInFastLimit', optInFastLimit);
  optInSlowLimit = requireReal('MAMA open', 'optInSlowLimit', optInSlowLimit);
  requireArgument('MAMA open', 'inReal', inReal);
  requireHistory('MAMA open', inReal.length);
  return mamaOpenInternal(core, float64(inReal), 0, optInFastLimit, optInSlowLimit);
}

/**
 * {@link mamaOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link mama}. The range written is the stream's
 * `outRange()`.
 *
 * @param inReal - Warm-up history. Price series to smooth.
 * @param optInFastLimit - Upper bound on the adaptive smoothing factor (default
 *   0.5; range 0.01 to 0.99; `Core.REAL_DEFAULT` selects the default).
 * @param optInSlowLimit - Lower bound on the adaptive smoothing factor (default
 *   0.05; range 0.01 to 0.99; `Core.REAL_DEFAULT` selects the default).
 * @param outMAMA - Adaptive moving average (fast line) Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param outFAMA - Following adaptive moving average, using half the alpha (slow
 *   line) Must hold `historyLen - lookback` values and share no memory with an input
 *   or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `mamaLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for a real parameter that is not a number, a parameter
 *   outside its range or an output too short or starting where another output or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { mamaOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const mamaOut = new Float64Array(data.length);
 * const fama = new Float64Array(data.length);
 * const stream = mamaOpenAndFill(data, 0.5, 0.05, mamaOut, fama);
 * const { begIdx, count } = stream.outRange();
 * // mamaOut[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function mamaOpenAndFill(
  inReal: ArrayLike<number>,
  optInFastLimit: number,
  optInSlowLimit: number,
  outMAMA: Float64Array,
  outFAMA: Float64Array,
  core: Core = Core.DEFAULT,
): MamaStream {
  optInFastLimit = requireReal('MAMA openAndFill', 'optInFastLimit', optInFastLimit);
  optInSlowLimit = requireReal('MAMA openAndFill', 'optInSlowLimit', optInSlowLimit);
  requireArgument('MAMA openAndFill', 'inReal', inReal);
  requireHistory('MAMA openAndFill', inReal.length);
  const guardOutLen = openFillCount('MAMA openAndFill', inReal.length, mamaLookback(optInFastLimit, optInSlowLimit, core));
  requireLength('MAMA openAndFill', 'outMAMA', outMAMA, guardOutLen);
  requireLength('MAMA openAndFill', 'outFAMA', outFAMA, guardOutLen);
  const inRealF64 = float64(inReal);
  if ((outMAMA.buffer === inRealF64.buffer && outMAMA.byteOffset === inRealF64.byteOffset) || (outFAMA.buffer === inRealF64.buffer && outFAMA.byteOffset === inRealF64.byteOffset) || (outMAMA.buffer === outFAMA.buffer && outMAMA.byteOffset === outFAMA.byteOffset)) {
    throw streamFailure('MAMA openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outMAMA, inRealF64) || overlaps(outFAMA, inRealF64) || overlaps(outMAMA, outFAMA))) {
    throw streamFailure('MAMA openAndFill', RetCode.BAD_PARAM);
  }
  return mamaOpenAndFillInternal(core, inRealF64, 0, optInFastLimit, optInSlowLimit, new MInteger(), new MInteger(), outMAMA, outFAMA);
}

/** @internal */
export function mamaStepTape(sp: MamaStream, tape: Float64Array, tapeBase: number, tapeMask: number, inReal: number): void {
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
  sp.trailingWMAValue = tape[(tapeBase - sp.ringCap_trailingWMAIdx) & tapeMask];
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
    /* Put Alpha in tempReal2 */
    if (sp.I1ForEvenPrev3 !== 0.0) {
      tempReal2 = Math.atan(Q1 / sp.I1ForEvenPrev3) * sp.rad2Deg;
    } else {
      tempReal2 = 0.0;
    }
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
     * used by the "odd" logic later.
     */
    sp.I1ForEvenPrev3 = sp.I1ForEvenPrev2;
    sp.I1ForEvenPrev2 = detrender;
    /* Put Alpha in tempReal2 */
    if (sp.I1ForOddPrev3 !== 0.0) {
      tempReal2 = Math.atan(Q1 / sp.I1ForOddPrev3) * sp.rad2Deg;
    } else {
      tempReal2 = 0.0;
    }
  }
  /* Put Delta Phase into tempReal */
  tempReal = sp.prevPhase - tempReal2;
  sp.prevPhase = tempReal2;
  if (tempReal < 1.0) {
    tempReal = 1.0;
  }
  /* Put Alpha into tempReal */
  if (tempReal > 1.0) {
    tempReal = sp.optInFastLimit / tempReal;
    if (tempReal < sp.optInSlowLimit) {
      tempReal = sp.optInSlowLimit;
    }
  } else {
    tempReal = sp.optInFastLimit;
  }
  /* Calculate MAMA, FAMA */
  {
    const _a: number = 1 - tempReal, _b: number = sp.mama, _c: number = tempReal * todayValue;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.mama = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  tempReal *= 0.5;
  {
    const _a: number = 1 - tempReal, _b: number = sp.fama, _c: number = tempReal * sp.mama;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.fama = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  sp.cur_outFAMA = sp.fama;
  sp.cur_outMAMA = sp.mama;
  /* Adjust the period for next price bar */
  let _fma32: number;
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
    _fma32 = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  {
    const _a: number = 0.8, _b: number = sp.Re, _c: number = 0.2 * _fma32;
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
  /* Ooof... let's do the next price bar now! */
  sp.streamParity = 1 - sp.streamParity;
  sp.outRangeCount++;
}

/** @internal */
export function mamaPeekTape(sp: MamaStream, tape: Float64Array, tapeBase: number, tapeMask: number, inReal: number, out: MamaOut): void {
  let tempReal = 0;
  let tempReal2 = 0;
  let adjustedPrevPeriod = 0;
  let smoothedValue = 0;
  let hilbertTempReal = 0;
  let detrender = 0;
  let Q1 = 0;
  let todayValue = 0;
  let I1ForEvenPrev2 = sp.I1ForEvenPrev2;
  let I1ForEvenPrev3 = sp.I1ForEvenPrev3;
  let I1ForOddPrev2 = sp.I1ForOddPrev2;
  let I1ForOddPrev3 = sp.I1ForOddPrev3;
  let cur_outFAMA = 0;
  let cur_outMAMA = 0;
  let fama = sp.fama;
  let hilbertIdx = sp.hilbertIdx;
  let mama = sp.mama;
  let periodWMASub = sp.periodWMASub;
  let periodWMASum = sp.periodWMASum;
  let prevPhase = sp.prevPhase;
  let prev_Q1_Even = sp.prev_Q1_Even;
  let prev_Q1_Odd = sp.prev_Q1_Odd;
  let prev_Q1_input_Even = sp.prev_Q1_input_Even;
  let prev_Q1_input_Odd = sp.prev_Q1_input_Odd;
  let prev_detrender_Even = sp.prev_detrender_Even;
  let prev_detrender_Odd = sp.prev_detrender_Odd;
  let prev_detrender_input_Even = sp.prev_detrender_input_Even;
  let prev_detrender_input_Odd = sp.prev_detrender_input_Odd;
  let trailingWMAValue = sp.trailingWMAValue;
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
  periodWMASub += todayValue;
  periodWMASub -= trailingWMAValue;
  periodWMASum += todayValue * 4.0;
  trailingWMAValue = tape[(tapeBase - sp.ringCap_trailingWMAIdx) & tapeMask];
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
    hilbertTempReal = sp.a * Q1;
    if (++hilbertIdx === 3) {
      hilbertIdx = 0;
    }
    /* The variable I1 is the detrender delayed for
     * 3 price bars.
     *
     * Save the current detrender value for being
     * used by the "odd" logic later.
     */
    I1ForOddPrev3 = I1ForOddPrev2;
    I1ForOddPrev2 = detrender;
    /* Put Alpha in tempReal2 */
    if (I1ForEvenPrev3 !== 0.0) {
      tempReal2 = Math.atan(Q1 / I1ForEvenPrev3) * sp.rad2Deg;
    } else {
      tempReal2 = 0.0;
    }
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
    hilbertTempReal = sp.a * Q1;
    /* The varaiable I1 is the detrender delayed for
     * 3 price bars.
     *
     * Save the current detrender value for being
     * used by the "odd" logic later.
     */
    I1ForEvenPrev3 = I1ForEvenPrev2;
    I1ForEvenPrev2 = detrender;
    /* Put Alpha in tempReal2 */
    if (I1ForOddPrev3 !== 0.0) {
      tempReal2 = Math.atan(Q1 / I1ForOddPrev3) * sp.rad2Deg;
    } else {
      tempReal2 = 0.0;
    }
  }
  /* Put Delta Phase into tempReal */
  tempReal = prevPhase - tempReal2;
  prevPhase = tempReal2;
  if (tempReal < 1.0) {
    tempReal = 1.0;
  }
  /* Put Alpha into tempReal */
  if (tempReal > 1.0) {
    tempReal = sp.optInFastLimit / tempReal;
    if (tempReal < sp.optInSlowLimit) {
      tempReal = sp.optInSlowLimit;
    }
  } else {
    tempReal = sp.optInFastLimit;
  }
  /* Calculate MAMA, FAMA */
  {
    const _a: number = 1 - tempReal, _b: number = mama, _c: number = tempReal * todayValue;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    mama = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  tempReal *= 0.5;
  {
    const _a: number = 1 - tempReal, _b: number = fama, _c: number = tempReal * mama;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    fama = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  cur_outFAMA = fama;
  cur_outMAMA = mama;
  out.mama = cur_outMAMA;
  out.fama = cur_outFAMA;
}

/** @internal */
export function mamaTapeDetach(sp: MamaStream): number {
  let reach = 0;
  sp.ring_trailingWMAIdx_inReal = new Float64Array(0);
  if (sp.ringCap_trailingWMAIdx > reach) {
    reach = sp.ringCap_trailingWMAIdx;
  }
  return reach;
}
