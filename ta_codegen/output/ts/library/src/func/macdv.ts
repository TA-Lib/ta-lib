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
 *  103126 MF,CC  Creation (#570).
 */

import { badSink, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { atrLookbackPositional } from './atr.ts';
import { emaLookbackPositional } from './ema.ts';

/** @internal */
export function macdvLookbackPositional(optInFastPeriod: number, optInSlowPeriod: number, optInSignalPeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInFastPeriod | 0) !== optInFastPeriod) {
    return -1;
  }
  if ((optInSlowPeriod | 0) !== optInSlowPeriod) {
    return -1;
  }
  if ((optInSignalPeriod | 0) !== optInSignalPeriod) {
    return -1;
  }
  if (optInFastPeriod === Core.INTEGER_DEFAULT) {
    optInFastPeriod = 12;
  } else if (optInFastPeriod < 2 || optInFastPeriod > 100000) {
    return -1;
  }
  if (optInSlowPeriod === Core.INTEGER_DEFAULT) {
    optInSlowPeriod = 26;
  } else if (optInSlowPeriod < 2 || optInSlowPeriod > 100000) {
    return -1;
  }
  if (optInSignalPeriod === Core.INTEGER_DEFAULT) {
    optInSignalPeriod = 9;
  } else if (optInSignalPeriod < 1 || optInSignalPeriod > 100000) {
    return -1;
  }
  let tempInteger = 0;
  let lookbackLine = 0;
  let lookbackATR = 0;
  /* Make sure slow is really slower than
   * the fast period! if not, swap...
   */
  if (optInSlowPeriod < optInFastPeriod) {
    /* swap */
    tempInteger = optInSlowPeriod;
    optInSlowPeriod = optInFastPeriod;
    optInFastPeriod = tempInteger;
  }
  /* The line needs the slow EMA and the Average True Range on the same bar,
   * so its first bar is the later of their two lookbacks. Each term is
   * exactly the lookback of the function it comes from and is never restated
   * here, which is what makes MACDV inherit TA_FUNC_UNST_EMA and
   * TA_FUNC_UNST_ATR from its callees.
   */
  lookbackLine = emaLookbackPositional(optInSlowPeriod, core);
  lookbackATR = atrLookbackPositional(optInSlowPeriod, core);
  if (lookbackATR > lookbackLine) {
    lookbackLine = lookbackATR;
  }
  return lookbackLine + emaLookbackPositional(optInSignalPeriod, core);
}

/** @internal */
export function macdvDisplayShiftPositional(optInFastPeriod: number, optInSlowPeriod: number, optInSignalPeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (macdvLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 3) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function macdvImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outMACDV: Float64Array,
  outMACDVSignal: Float64Array,
  outMACDVHist: Float64Array,
): RetCode {
  let prevFast = 0;
  let prevSlow = 0;
  let prevATR = 0;
  let prevSignal = 0;
  let lineValue = 0;
  let tempReal = 0;
  let slowK = 0;
  let fastK = 0;
  let signalK = 0;
  let slowBeta = 0;
  let fastBeta = 0;
  let signalBeta = 0;
  let wAlpha = 0;
  let wBeta = 0;
  let periodTotal = 0;
  let val2 = 0;
  let val3 = 0;
  let greatest = 0;
  let spread = 0;
  let bandScale = 0;
  let periodScale = 0;
  let tempCY = 0;
  let tempLT = 0;
  let tempHT = 0;
  let i = 0;
  let today = 0;
  let outIdx = 0;
  let tempInteger = 0;
  let lookbackTotal = 0;
  let lookbackSignal = 0;
  let lookbackSlow = 0;
  let lookbackFast = 0;
  let lookbackATR = 0;
  let lineIdx = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInFastPeriod === Core.INTEGER_DEFAULT) {
    optInFastPeriod = 12;
  } else if (optInFastPeriod < 2 || optInFastPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowPeriod === Core.INTEGER_DEFAULT) {
    optInSlowPeriod = 26;
  } else if (optInSlowPeriod < 2 || optInSlowPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSignalPeriod === Core.INTEGER_DEFAULT) {
    optInSignalPeriod = 9;
  } else if (optInSignalPeriod < 1 || optInSignalPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if ((outMACDV.buffer === outMACDVSignal.buffer && outMACDV.byteOffset === outMACDVSignal.byteOffset) || (outMACDV.buffer === outMACDVHist.buffer && outMACDV.byteOffset === outMACDVHist.byteOffset) || (outMACDVSignal.buffer === outMACDVHist.buffer && outMACDVSignal.byteOffset === outMACDVHist.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  /* Make sure slow is really slower than
   * the fast period! if not, swap...
   */
  if (optInSlowPeriod < optInFastPeriod) {
    /* swap */
    tempInteger = optInSlowPeriod;
    optInSlowPeriod = optInFastPeriod;
    optInFastPeriod = tempInteger;
  }
  /* ema.c's k and beta for the three EMA, macd.c's spelling. */
  slowBeta = (optInSlowPeriod - 1) / (optInSlowPeriod + 1);
  slowK = 1.0 - slowBeta;
  if (slowBeta < 0.5) {
    slowBeta = 1.0 - slowK;
  }
  fastBeta = (optInFastPeriod - 1) / (optInFastPeriod + 1);
  fastK = 1.0 - fastBeta;
  if (fastBeta < 0.5) {
    fastBeta = 1.0 - fastK;
  }
  /* A signal period of 1 disables signal-line smoothing: the signal IS the
   * MACD-V line and the histogram is exactly zero. The recursion below, at a
   * k of 1.0 and a beta of 0.0, does not keep the sign of a -0.0 line value;
   * hence the explicit arm at each step.
   */
  signalBeta = (optInSignalPeriod - 1) / (optInSignalPeriod + 1);
  signalK = 1.0 - signalBeta;
  if (signalBeta < 0.5) {
    signalBeta = 1.0 - signalK;
  }
  /* atr.c's Wilder coefficients: wAlpha derived FROM wBeta, never the
   * reverse, so that the pair is the one TA_ATR runs.
   */
  wBeta = (optInSlowPeriod - 1) / optInSlowPeriod;
  wAlpha = 1.0 - wBeta;
  /* An EMA of a constant can settle short of it by a number of ulps that
   * grows in proportion to the period.
   */
  periodScale = (optInSlowPeriod + 1);
  lookbackSignal = emaLookbackPositional(optInSignalPeriod, core);
  lookbackSlow = emaLookbackPositional(optInSlowPeriod, core);
  lookbackFast = emaLookbackPositional(optInFastPeriod, core);
  lookbackATR = atrLookbackPositional(optInSlowPeriod, core);
  lookbackTotal = macdvLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core);
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
  /* Everything is computed in a single lockstep pass: each bar advances the
   * fast EMA, the slow EMA and the Average True Range, the scaled quotient
   * is the MACD-V line, and each line value is immediately fed into the
   * signal EMA. No temporary buffers.
   *
   * The arithmetic order below is the bit-exactness contract with TA_EMA,
   * TA_MACD and TA_ATR (do not reorder or fuse operations):
   *  - EMA recursion: k*x + beta*prev, with ema.c's k and beta.
   *  - True Range: start from high-low, then compare/replace with the two
   *    previous-close distances, in that order; Wilder smoothing in ONE
   *    statement.
   *  - Each recursion is seeded with the sum of its first 'period' inputs,
   *    accumulated from 0.0 in input order, divided by the period. The
   *    signal EMA is seeded the same way from the first 'signal period'
   *    line values.
   *  - The line is the spread divided by the Average True Range, then
   *    scaled: what a caller dividing TA_MACD by TA_ATR computes.
   *
   * In-place (an output == an input) is supported: outputs at [outIdx] are
   * written only after every input at or before bar startIdx+outIdx was
   * read, and the previous close is re-read from the input one bar back,
   * which no output store has reached.
   */
  /* Each leg is seeded at its OWN lookback and warmed up to lineIdx, the
   * first MACD-V line bar, so each is the shipped function entered at that
   * bar and nothing here over-warms the shorter one: a caller who wants a
   * leg converged sets that function's unstable period, exactly as when
   * calling it directly.
   */
  lineIdx = startIdx - lookbackSignal;
  /* Slow EMA. */
  today = lineIdx - lookbackSlow;
  tempReal = 0.0;
  i = optInSlowPeriod;
  while (i-- > 0) {
    tempReal += inClose[today++];
  }
  prevSlow = tempReal / optInSlowPeriod;
  while (today <= lineIdx) {
    tempReal = inClose[today++];
    {
      const _a: number = slowBeta, _b: number = prevSlow, _c: number = slowK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevSlow = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
  }
  /* Fast EMA. */
  today = lineIdx - lookbackFast;
  tempReal = 0.0;
  i = optInFastPeriod;
  while (i-- > 0) {
    tempReal += inClose[today++];
  }
  prevFast = tempReal / optInFastPeriod;
  while (today <= lineIdx) {
    tempReal = inClose[today++];
    {
      const _a: number = fastBeta, _b: number = prevFast, _c: number = fastK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevFast = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
  }
  /* Average True Range: the first True Range consumes a previous close. */
  today = lineIdx - lookbackATR + 1;
  periodTotal = 0.0;
  i = optInSlowPeriod;
  while (i-- > 0) {
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    periodTotal += greatest;
    today += 1;
  }
  prevATR = periodTotal / optInSlowPeriod;
  while (today <= lineIdx) {
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    {
      const _a: number = wBeta, _b: number = prevATR, _c: number = wAlpha * greatest;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevATR = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    today += 1;
  }
  /* Without price movement the line answers its neutral point, 0.0 (#570).
   * In a flat run the Wilder average only decays, to a subnormal floor,
   * while the spread stalls at the EMAs' rounding residue, so an exact test
   * lets their quotient grow until it overflows. Both are therefore tested
   * against the EMAs' own magnitude times the residue's period factor: that
   * also bounds |line| for any finite input, which a scale taken from the
   * close cannot, since a close may lie outside its bar and be zero.
   */
  spread = prevFast - prevSlow;
  bandScale = Math.abs(prevFast);
  if (Math.abs(prevSlow) > bandScale) {
    bandScale = Math.abs(prevSlow);
  }
  bandScale = bandScale * periodScale;
  lineValue = 0.0;
  if (!(Math.abs(spread) <= 0.00000000000001 * (bandScale)) && !(Math.abs(prevATR) <= 0.00000000000001 * (bandScale)) && prevATR >= 2.2250738585072014e-308) {
    lineValue = spread / prevATR * 100.0;
  }
  /* Seed the signal EMA with a simple average of the first 'signal period'
   * line values, accumulated as they are produced.
   */
  prevSignal = 0.0;
  prevSignal += lineValue;
  i = optInSignalPeriod - 1;
  while (i-- > 0) {
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    tempReal = inClose[today];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    {
      const _a: number = wBeta, _b: number = prevATR, _c: number = wAlpha * greatest;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevATR = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = fastBeta, _b: number = prevFast, _c: number = fastK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevFast = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = slowBeta, _b: number = prevSlow, _c: number = slowK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevSlow = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    spread = prevFast - prevSlow;
    bandScale = Math.abs(prevFast);
    if (Math.abs(prevSlow) > bandScale) {
      bandScale = Math.abs(prevSlow);
    }
    bandScale = bandScale * periodScale;
    lineValue = 0.0;
    if (!(Math.abs(spread) <= 0.00000000000001 * (bandScale)) && !(Math.abs(prevATR) <= 0.00000000000001 * (bandScale)) && prevATR >= 2.2250738585072014e-308) {
      lineValue = spread / prevATR * 100.0;
    }
    prevSignal += lineValue;
    today += 1;
  }
  prevSignal = prevSignal / optInSignalPeriod;
  /* Advance everything in lockstep through the unstable period of the
   * signal EMA, up to the first output bar.
   */
  while (today <= startIdx) {
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    tempReal = inClose[today];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    {
      const _a: number = wBeta, _b: number = prevATR, _c: number = wAlpha * greatest;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevATR = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = fastBeta, _b: number = prevFast, _c: number = fastK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevFast = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = slowBeta, _b: number = prevSlow, _c: number = slowK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevSlow = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    spread = prevFast - prevSlow;
    bandScale = Math.abs(prevFast);
    if (Math.abs(prevSlow) > bandScale) {
      bandScale = Math.abs(prevSlow);
    }
    bandScale = bandScale * periodScale;
    lineValue = 0.0;
    if (!(Math.abs(spread) <= 0.00000000000001 * (bandScale)) && !(Math.abs(prevATR) <= 0.00000000000001 * (bandScale)) && prevATR >= 2.2250738585072014e-308) {
      lineValue = spread / prevATR * 100.0;
    }
    if (optInSignalPeriod === 1) {
      prevSignal = lineValue;
    } else {
      {
        const _a: number = signalBeta, _b: number = prevSignal, _c: number = signalK * lineValue;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        prevSignal = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
    }
    today += 1;
  }
  /* Stable zone: keep advancing in lockstep and write the three outputs. */
  outMACDV[0] = lineValue;
  outMACDVSignal[0] = prevSignal;
  outMACDVHist[0] = lineValue - prevSignal;
  outIdx = 1;
  while (today <= endIdx) {
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    tempReal = inClose[today];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    {
      const _a: number = wBeta, _b: number = prevATR, _c: number = wAlpha * greatest;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevATR = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = fastBeta, _b: number = prevFast, _c: number = fastK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevFast = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = slowBeta, _b: number = prevSlow, _c: number = slowK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevSlow = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    spread = prevFast - prevSlow;
    bandScale = Math.abs(prevFast);
    if (Math.abs(prevSlow) > bandScale) {
      bandScale = Math.abs(prevSlow);
    }
    bandScale = bandScale * periodScale;
    lineValue = 0.0;
    if (!(Math.abs(spread) <= 0.00000000000001 * (bandScale)) && !(Math.abs(prevATR) <= 0.00000000000001 * (bandScale)) && prevATR >= 2.2250738585072014e-308) {
      lineValue = spread / prevATR * 100.0;
    }
    if (optInSignalPeriod === 1) {
      prevSignal = lineValue;
    } else {
      {
        const _a: number = signalBeta, _b: number = prevSignal, _c: number = signalK * lineValue;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        prevSignal = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
    }
    outMACDV[outIdx] = lineValue;
    outMACDVSignal[outIdx] = prevSignal;
    outMACDVHist[outIdx] = lineValue - prevSignal;
    outIdx += 1;
    today += 1;
  }
  /* All done! Indicate the output limits and return success. */
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['fastPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['slowPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['signalPeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000']] as const;

/** @internal */
export function macdvPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  outMACDV: Float64Array,
  outMACDVSignal: Float64Array,
  outMACDVHist: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('MACDV', 'startIdx', startIdx);
  endIdx = requireInt32('MACDV', 'endIdx', endIdx);
  optInFastPeriod = requireInt32('MACDV', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('MACDV', 'slowPeriod', optInSlowPeriod);
  optInSignalPeriod = requireInt32('MACDV', 'signalPeriod', optInSignalPeriod);
  requireIndexRange('MACDV', startIdx, endIdx);
  const guardLookback = macdvLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('MACDV', PARAM_RANGES, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
  }
  const guardStart = clampedStart('MACDV', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('MACDV', 'prices.high', inHigh, guardInLen);
  requireLength('MACDV', 'prices.low', inLow, guardInLen);
  requireLength('MACDV', 'prices.close', inClose, guardInLen);
  requireOutput('MACDV', 'outMacdv', outMACDV, guardOutLen, Float64Array);
  requireOutput('MACDV', 'outMacdvSignal', outMACDVSignal, guardOutLen, Float64Array);
  requireOutput('MACDV', 'outMacdvHist', outMACDVHist, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outMACDV, inHighF64)) {
    throw sharedMemory('MACDV', 'outMacdv', 'prices.high');
  }
  if (overlapsPartly(outMACDV, inLowF64)) {
    throw sharedMemory('MACDV', 'outMacdv', 'prices.low');
  }
  if (overlapsPartly(outMACDV, inCloseF64)) {
    throw sharedMemory('MACDV', 'outMacdv', 'prices.close');
  }
  if (overlapsPartly(outMACDVSignal, inHighF64)) {
    throw sharedMemory('MACDV', 'outMacdvSignal', 'prices.high');
  }
  if (overlapsPartly(outMACDVSignal, inLowF64)) {
    throw sharedMemory('MACDV', 'outMacdvSignal', 'prices.low');
  }
  if (overlapsPartly(outMACDVSignal, inCloseF64)) {
    throw sharedMemory('MACDV', 'outMacdvSignal', 'prices.close');
  }
  if (overlapsPartly(outMACDVHist, inHighF64)) {
    throw sharedMemory('MACDV', 'outMacdvHist', 'prices.high');
  }
  if (overlapsPartly(outMACDVHist, inLowF64)) {
    throw sharedMemory('MACDV', 'outMacdvHist', 'prices.low');
  }
  if (overlapsPartly(outMACDVHist, inCloseF64)) {
    throw sharedMemory('MACDV', 'outMacdvHist', 'prices.close');
  }
  if (overlaps(outMACDV, outMACDVSignal)) {
    throw sharedMemory('MACDV', 'outMacdv', 'outMacdvSignal');
  }
  if (overlaps(outMACDV, outMACDVHist)) {
    throw sharedMemory('MACDV', 'outMacdv', 'outMacdvHist');
  }
  if (overlaps(outMACDVSignal, outMACDVHist)) {
    throw sharedMemory('MACDV', 'outMacdvSignal', 'outMacdvHist');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = macdvImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outMACDV, outMACDVSignal, outMACDVHist);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('MACDV', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link macdv}, {@link macdvLookback}, {@link macdvDisplayShift},
 * {@link macdvOpen} and {@link macdvOpenAndFill}. Every key is optional: an
 * omitted or `undefined` key takes its default, and a call ignores the keys it has
 * no use for.
 */
export interface MacdvOptions {
  /**
   * Period of the fast EMA (default 12; range 2 to 100000; `Core.INTEGER_DEFAULT`
   * selects the default).
   */
  fastPeriod?: number | undefined;
  /**
   * Period of the slow EMA and of the Average True Range (default 26; range 2 to
   * 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  slowPeriod?: number | undefined;
  /**
   * Smoothing period of the signal line (default 9; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  signalPeriod?: number | undefined;
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
   * Buffer for `outMacdv`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outMacdv?: Float64Array<ArrayBuffer> | undefined;
  /**
   * Buffer for `outMacdvSignal`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outMacdvSignal?: Float64Array<ArrayBuffer> | undefined;
  /**
   * Buffer for `outMacdvHist`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outMacdvHist?: Float64Array<ArrayBuffer> | undefined;
}

/** What {@link macdv} returns, and {@link macdvOpenAndFill} with its stream. */
export interface MacdvResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Fast EMA minus slow EMA, divided by the Average True Range, times 100.
   * `outMacdv[k]` is the value at input bar `begIdx + k`.
   */
  outMacdv: Float64Array<ArrayBuffer>;
  /**
   * EMA of the MACD-V line. `outMacdvSignal[k]` is the value at input bar
   * `begIdx + k`.
   */
  outMacdvSignal: Float64Array<ArrayBuffer>;
  /**
   * MACD-V minus signal line. `outMacdvHist[k]` is the value at input bar
   * `begIdx + k`.
   */
  outMacdvHist: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link macdv} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MacdvOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function macdvLookback(options?: MacdvOptions): number {
  const o = optionsOf('MACDV lookback', options);
  return macdvLookbackPositional(orDefault(o.fastPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowPeriod, Core.INTEGER_DEFAULT), orDefault(o.signalPeriod, Core.INTEGER_DEFAULT), coreOf('MACDV lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link macdv}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MacdvOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function macdvDisplayShift(outputIdx: number, options?: MacdvOptions): number {
  const o = optionsOf('MACDV displayShift', options);
  return macdvDisplayShiftPositional(orDefault(o.fastPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowPeriod, Core.INTEGER_DEFAULT), orDefault(o.signalPeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('MACDV displayShift', o.core));
}

/**
 * MACD-V, Alex Spiroglou's volatility-normalised MACD: the spread of a fast and a
 * slow EMA of the close over the Average True Range, times 100. Like MACD it comes
 * with an EMA-smoothed signal line and their histogram. Dividing by the ATR
 * expresses momentum in units of the instrument's own volatility, so one reading
 * means the same thing across instruments, timeframes and volatility regimes,
 * where the raw MACD spread does not. Spiroglou reads the line in ranges: beyond
 * 150 or -150 is an overextended move, between -50 and 50 a ranging market, and
 * the bands in between a trend.
 *
 * Formula and more info at
 * [ta-lib.org/functions/macdv](https://ta-lib.org/functions/macdv).
 *
 * **Notes**
 * - The Average True Range is Wilder's, over the slow period: the publication's
 *   `ATR(26)` beside the 26-bar slow EMA, so one parameter sets both. Without
 *   price movement the line falls to 0, its neutral point.
 * - The two price EMAs and the Average True Range each warm up as when calling
 *   `EMA` and `ATR` directly, so a caller who wants the line converged sets
 *   `FuncUnstId.EMA` and `FuncUnstId.ATR`, as for those functions.
 * - If the slow period is set smaller than the fast period, the two are swapped so
 *   the slow EMA is always the longer one, as `MACD` does; the ATR period follows
 *   the slow period after the swap.
 * - A signal period of 1 disables signal-line smoothing: the signal equals the
 *   MACD-V line and the histogram is zero.
 * - The publication's MACD section writes the histogram as the signal line minus
 *   the MACD line; its reading of the histogram ("above 40" is stretched to the
 *   upside) holds only for MACD-V minus the signal, which is what is emitted here,
 *   as for `MACD`.
 *
 * `outMacdv[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link macdvLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `high`: High price of
 *   each bar. `low`: Low price of each bar. `close`: Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link MacdvOptions}.
 * @returns `begIdx` and the values; see {@link MacdvResult}. Each output is a view
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
 * import { macdv } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const {
 *   begIdx,
 *   outMacdv,
 *   outMacdvSignal,
 *   outMacdvHist,
 * } = macdv({ high, low, close }, {
 *   fastPeriod: 12,
 *   slowPeriod: 26,
 *   signalPeriod: 9,
 * });
 * // outMacdv[k] is the value at bar begIdx + k
 * ```
 * @see [`macd`](https://ta-lib.org/functions/macd)
 * @see [`atr`](https://ta-lib.org/functions/atr)
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`apo`](https://ta-lib.org/functions/apo)
 * @see [`ppo`](https://ta-lib.org/functions/ppo)
 */
export function macdv(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: MacdvOptions): MacdvResult {
  requireArgument('MACDV', 'prices', prices);
  const o = optionsOf('MACDV', options);
  if (o.endIdx === undefined) {
    requireSeries('MACDV', 'prices.high', prices.high);
    requireSeries('MACDV', 'prices.low', prices.low);
    requireSeries('MACDV', 'prices.close', prices.close);
  }
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MACDV', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, macdvLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core), prices.high, prices.low, prices.close);
  const outMACDV = o.outMacdv === undefined ? new Float64Array(capacity) : o.outMacdv;
  const outMACDVSignal = o.outMacdvSignal === undefined ? new Float64Array(capacity) : o.outMacdvSignal;
  const outMACDVHist = o.outMacdvHist === undefined ? new Float64Array(capacity) : o.outMacdvHist;
  const range = macdvPositional(startIdx, endIdx, prices.high, prices.low, prices.close, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outMACDV, outMACDVSignal, outMACDVHist, core);
  return { begIdx: range.begIdx, outMacdv: outMACDV.subarray(0, range.count), outMacdvSignal: outMACDVSignal.subarray(0, range.count), outMacdvHist: outMACDVHist.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live MACDV stream: one value per closed bar, bit-identical to {@link macdv}
 * over the same series. Open it with {@link macdvOpen} or
 * {@link macdvOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/macdv](https://ta-lib.org/functions/macdv).
 */
export class MacdvStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInFastPeriod: number = 0;
  /** @internal */
  optInSlowPeriod: number = 0;
  /** @internal */
  optInSignalPeriod: number = 0;
  /** @internal */
  prevFast: number = 0;
  /** @internal */
  prevSlow: number = 0;
  /** @internal */
  prevATR: number = 0;
  /** @internal */
  prevSignal: number = 0;
  /** @internal */
  slowK: number = 0;
  /** @internal */
  fastK: number = 0;
  /** @internal */
  signalK: number = 0;
  /** @internal */
  slowBeta: number = 0;
  /** @internal */
  fastBeta: number = 0;
  /** @internal */
  signalBeta: number = 0;
  /** @internal */
  wAlpha: number = 0;
  /** @internal */
  wBeta: number = 0;
  /** @internal */
  periodScale: number = 0;
  /** @internal */
  lag1_inClose: number = 0;
  /** @internal */
  cur_outMACDV: number = 0;
  /** @internal */
  cur_outMACDVSignal: number = 0;
  /** @internal */
  cur_outMACDVHist: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): MacdvStream {
    return new MacdvStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link macdv} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('MACDV advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns its outputs.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @param out - Receives the outputs and is returned; a new {@link MacdvOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included, before anything moves.
   *   `TALibError` with code `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached
   *   bar `Core.INDEX_MAX`.
   */
  update(high: number, low: number, close: number, out?: MacdvOut): MacdvOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('MACDV update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('MACDV update', 'MacdvOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('MACDV update', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    macdvStepImpl(this, high, low, close);
    this.outRangeCount++;
    if (out === undefined) {
      out = new MacdvOut();
    }
    out.outMacdv = this.cur_outMACDV;
    out.outMacdvSignal = this.cur_outMACDVSignal;
    out.outMacdvHist = this.cur_outMACDVHist;
    return out;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @param out - Receives the outputs and is returned; a new {@link MacdvOut} when
   *   omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included.
   */
  peek(high: number, low: number, close: number, out?: MacdvOut): MacdvOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('MACDV peek', 'MacdvOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('MACDV peek', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
    const inClose = close;
    let lineValue = 0;
    let tempReal = 0;
    let val2 = 0;
    let val3 = 0;
    let greatest = 0;
    let spread = 0;
    let bandScale = 0;
    let tempCY = 0;
    let tempLT = 0;
    let tempHT = 0;
    let cur_outMACDV = 0;
    let cur_outMACDVHist = 0;
    let cur_outMACDVSignal = 0;
    let prevATR = sp.prevATR;
    let prevFast = sp.prevFast;
    let prevSignal = sp.prevSignal;
    let prevSlow = sp.prevSlow;
    tempLT = inLow;
    tempHT = inHigh;
    tempCY = sp.lag1_inClose;
    tempReal = inClose;
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    {
      const _a: number = sp.wBeta, _b: number = prevATR, _c: number = sp.wAlpha * greatest;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevATR = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = sp.fastBeta, _b: number = prevFast, _c: number = sp.fastK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevFast = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = sp.slowBeta, _b: number = prevSlow, _c: number = sp.slowK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevSlow = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    spread = prevFast - prevSlow;
    bandScale = Math.abs(prevFast);
    if (Math.abs(prevSlow) > bandScale) {
      bandScale = Math.abs(prevSlow);
    }
    bandScale = bandScale * sp.periodScale;
    lineValue = 0.0;
    if (!(Math.abs(spread) <= 0.00000000000001 * (bandScale)) && !(Math.abs(prevATR) <= 0.00000000000001 * (bandScale)) && prevATR >= 2.2250738585072014e-308) {
      lineValue = spread / prevATR * 100.0;
    }
    if (sp.optInSignalPeriod === 1) {
      prevSignal = lineValue;
    } else {
      {
        const _a: number = sp.signalBeta, _b: number = prevSignal, _c: number = sp.signalK * lineValue;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        prevSignal = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
    }
    cur_outMACDV = lineValue;
    cur_outMACDVSignal = prevSignal;
    cur_outMACDVHist = lineValue - prevSignal;
    if (out === undefined) {
      out = new MacdvOut();
    }
    out.outMacdv = cur_outMACDV;
    out.outMacdvSignal = cur_outMACDVSignal;
    out.outMacdvHist = cur_outMACDVHist;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link MacdvOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a sink that is not an object,
   *   `null` included.
   */
  value(out?: MacdvOut): MacdvOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('MACDV value', 'MacdvOut');
    }
    if (out === undefined) {
      out = new MacdvOut();
    }
    out.outMacdv = this.cur_outMACDV;
    out.outMacdvSignal = this.cur_outMACDVSignal;
    out.outMacdvHist = this.cur_outMACDVHist;
    return out;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): MacdvStream {
    const c = new MacdvStream(this.core);
    c.optInFastPeriod = this.optInFastPeriod;
    c.optInSlowPeriod = this.optInSlowPeriod;
    c.optInSignalPeriod = this.optInSignalPeriod;
    c.prevFast = this.prevFast;
    c.prevSlow = this.prevSlow;
    c.prevATR = this.prevATR;
    c.prevSignal = this.prevSignal;
    c.slowK = this.slowK;
    c.fastK = this.fastK;
    c.signalK = this.signalK;
    c.slowBeta = this.slowBeta;
    c.fastBeta = this.fastBeta;
    c.signalBeta = this.signalBeta;
    c.wAlpha = this.wAlpha;
    c.wBeta = this.wBeta;
    c.periodScale = this.periodScale;
    c.lag1_inClose = this.lag1_inClose;
    c.cur_outMACDV = this.cur_outMACDV;
    c.cur_outMACDVSignal = this.cur_outMACDVSignal;
    c.cur_outMACDVHist = this.cur_outMACDVHist;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one MACDV bar, as `update`, `peek` and `value` return them.
 * Passed back to one of them, it is filled and returned again, with no allocation;
 * it holds its values until the next call that writes it. A new one holds `NaN`.
 */
export class MacdvOut {
  /** Fast EMA minus slow EMA, divided by the Average True Range, times 100. */
  outMacdv: number = NaN;
  /** EMA of the MACD-V line. */
  outMacdvSignal: number = NaN;
  /** MACD-V minus signal line. */
  outMacdvHist: number = NaN;
}

function macdvStepImpl(sp: MacdvStream, inHigh: number, inLow: number, inClose: number): void {
  let lineValue = 0;
  let tempReal = 0;
  let val2 = 0;
  let val3 = 0;
  let greatest = 0;
  let spread = 0;
  let bandScale = 0;
  let tempCY = 0;
  let tempLT = 0;
  let tempHT = 0;
  tempLT = inLow;
  tempHT = inHigh;
  tempCY = sp.lag1_inClose;
  tempReal = inClose;
  greatest = tempHT - tempLT;
  /* val1 */
  val2 = Math.abs(tempCY - tempHT);
  if (val2 > greatest) {
    greatest = val2;
  }
  val3 = Math.abs(tempCY - tempLT);
  if (val3 > greatest) {
    greatest = val3;
  }
  {
    const _a: number = sp.wBeta, _b: number = sp.prevATR, _c: number = sp.wAlpha * greatest;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.prevATR = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  {
    const _a: number = sp.fastBeta, _b: number = sp.prevFast, _c: number = sp.fastK * tempReal;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.prevFast = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  {
    const _a: number = sp.slowBeta, _b: number = sp.prevSlow, _c: number = sp.slowK * tempReal;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.prevSlow = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  spread = sp.prevFast - sp.prevSlow;
  bandScale = Math.abs(sp.prevFast);
  if (Math.abs(sp.prevSlow) > bandScale) {
    bandScale = Math.abs(sp.prevSlow);
  }
  bandScale = bandScale * sp.periodScale;
  lineValue = 0.0;
  if (!(Math.abs(spread) <= 0.00000000000001 * (bandScale)) && !(Math.abs(sp.prevATR) <= 0.00000000000001 * (bandScale)) && sp.prevATR >= 2.2250738585072014e-308) {
    lineValue = spread / sp.prevATR * 100.0;
  }
  if (sp.optInSignalPeriod === 1) {
    sp.prevSignal = lineValue;
  } else {
    {
      const _a: number = sp.signalBeta, _b: number = sp.prevSignal, _c: number = sp.signalK * lineValue;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      sp.prevSignal = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
  }
  sp.cur_outMACDV = lineValue;
  sp.cur_outMACDVSignal = sp.prevSignal;
  sp.cur_outMACDVHist = lineValue - sp.prevSignal;
  sp.lag1_inClose = inClose;
}

function macdvOpenImpl(
  sp: MacdvStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outMACDV: Float64Array,
  outMACDVSignal: Float64Array,
  outMACDVHist: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let prevFast = 0;
  let prevSlow = 0;
  let prevATR = 0;
  let prevSignal = 0;
  let lineValue = 0;
  let tempReal = 0;
  let slowK = 0;
  let fastK = 0;
  let signalK = 0;
  let slowBeta = 0;
  let fastBeta = 0;
  let signalBeta = 0;
  let wAlpha = 0;
  let wBeta = 0;
  let periodTotal = 0;
  let val2 = 0;
  let val3 = 0;
  let greatest = 0;
  let spread = 0;
  let bandScale = 0;
  let periodScale = 0;
  let tempCY = 0;
  let tempLT = 0;
  let tempHT = 0;
  let i = 0;
  let today = 0;
  let outIdx = 0;
  let tempInteger = 0;
  let lookbackTotal = 0;
  let lookbackSignal = 0;
  let lookbackSlow = 0;
  let lookbackFast = 0;
  let lookbackATR = 0;
  let lineIdx = 0;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length || inClose.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInFastPeriod === Core.INTEGER_DEFAULT) {
    optInFastPeriod = 12;
  } else if (optInFastPeriod < 2 || optInFastPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowPeriod === Core.INTEGER_DEFAULT) {
    optInSlowPeriod = 26;
  } else if (optInSlowPeriod < 2 || optInSlowPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSignalPeriod === Core.INTEGER_DEFAULT) {
    optInSignalPeriod = 9;
  } else if (optInSignalPeriod < 1 || optInSignalPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Make sure slow is really slower than
   * the fast period! if not, swap...
   */
  if (optInSlowPeriod < optInFastPeriod) {
    /* swap */
    tempInteger = optInSlowPeriod;
    optInSlowPeriod = optInFastPeriod;
    optInFastPeriod = tempInteger;
  }
  /* ema.c's k and beta for the three EMA, macd.c's spelling. */
  slowBeta = (optInSlowPeriod - 1) / (optInSlowPeriod + 1);
  slowK = 1.0 - slowBeta;
  if (slowBeta < 0.5) {
    slowBeta = 1.0 - slowK;
  }
  fastBeta = (optInFastPeriod - 1) / (optInFastPeriod + 1);
  fastK = 1.0 - fastBeta;
  if (fastBeta < 0.5) {
    fastBeta = 1.0 - fastK;
  }
  /* A signal period of 1 disables signal-line smoothing: the signal IS the
   * MACD-V line and the histogram is exactly zero. The recursion below, at a
   * k of 1.0 and a beta of 0.0, does not keep the sign of a -0.0 line value;
   * hence the explicit arm at each step.
   */
  signalBeta = (optInSignalPeriod - 1) / (optInSignalPeriod + 1);
  signalK = 1.0 - signalBeta;
  if (signalBeta < 0.5) {
    signalBeta = 1.0 - signalK;
  }
  /* atr.c's Wilder coefficients: wAlpha derived FROM wBeta, never the
   * reverse, so that the pair is the one TA_ATR runs.
   */
  wBeta = (optInSlowPeriod - 1) / optInSlowPeriod;
  wAlpha = 1.0 - wBeta;
  /* An EMA of a constant can settle short of it by a number of ulps that
   * grows in proportion to the period.
   */
  periodScale = (optInSlowPeriod + 1);
  lookbackSignal = emaLookbackPositional(optInSignalPeriod, core);
  lookbackSlow = emaLookbackPositional(optInSlowPeriod, core);
  lookbackFast = emaLookbackPositional(optInFastPeriod, core);
  lookbackATR = atrLookbackPositional(optInSlowPeriod, core);
  lookbackTotal = macdvLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core);
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
  /* Everything is computed in a single lockstep pass: each bar advances the
   * fast EMA, the slow EMA and the Average True Range, the scaled quotient
   * is the MACD-V line, and each line value is immediately fed into the
   * signal EMA. No temporary buffers.
   *
   * The arithmetic order below is the bit-exactness contract with TA_EMA,
   * TA_MACD and TA_ATR (do not reorder or fuse operations):
   *  - EMA recursion: k*x + beta*prev, with ema.c's k and beta.
   *  - True Range: start from high-low, then compare/replace with the two
   *    previous-close distances, in that order; Wilder smoothing in ONE
   *    statement.
   *  - Each recursion is seeded with the sum of its first 'period' inputs,
   *    accumulated from 0.0 in input order, divided by the period. The
   *    signal EMA is seeded the same way from the first 'signal period'
   *    line values.
   *  - The line is the spread divided by the Average True Range, then
   *    scaled: what a caller dividing TA_MACD by TA_ATR computes.
   *
   * In-place (an output == an input) is supported: outputs at [outIdx] are
   * written only after every input at or before bar startIdx+outIdx was
   * read, and the previous close is re-read from the input one bar back,
   * which no output store has reached.
   */
  /* Each leg is seeded at its OWN lookback and warmed up to lineIdx, the
   * first MACD-V line bar, so each is the shipped function entered at that
   * bar and nothing here over-warms the shorter one: a caller who wants a
   * leg converged sets that function's unstable period, exactly as when
   * calling it directly.
   */
  lineIdx = startIdx - lookbackSignal;
  /* Slow EMA. */
  today = lineIdx - lookbackSlow;
  tempReal = 0.0;
  i = optInSlowPeriod;
  while (i-- > 0) {
    tempReal += inClose[today++];
  }
  prevSlow = tempReal / optInSlowPeriod;
  while (today <= lineIdx) {
    tempReal = inClose[today++];
    {
      const _a: number = slowBeta, _b: number = prevSlow, _c: number = slowK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevSlow = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
  }
  /* Fast EMA. */
  today = lineIdx - lookbackFast;
  tempReal = 0.0;
  i = optInFastPeriod;
  while (i-- > 0) {
    tempReal += inClose[today++];
  }
  prevFast = tempReal / optInFastPeriod;
  while (today <= lineIdx) {
    tempReal = inClose[today++];
    {
      const _a: number = fastBeta, _b: number = prevFast, _c: number = fastK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevFast = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
  }
  /* Average True Range: the first True Range consumes a previous close. */
  today = lineIdx - lookbackATR + 1;
  periodTotal = 0.0;
  i = optInSlowPeriod;
  while (i-- > 0) {
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    periodTotal += greatest;
    today += 1;
  }
  prevATR = periodTotal / optInSlowPeriod;
  while (today <= lineIdx) {
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    {
      const _a: number = wBeta, _b: number = prevATR, _c: number = wAlpha * greatest;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevATR = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    today += 1;
  }
  /* Without price movement the line answers its neutral point, 0.0 (#570).
   * In a flat run the Wilder average only decays, to a subnormal floor,
   * while the spread stalls at the EMAs' rounding residue, so an exact test
   * lets their quotient grow until it overflows. Both are therefore tested
   * against the EMAs' own magnitude times the residue's period factor: that
   * also bounds |line| for any finite input, which a scale taken from the
   * close cannot, since a close may lie outside its bar and be zero.
   */
  spread = prevFast - prevSlow;
  bandScale = Math.abs(prevFast);
  if (Math.abs(prevSlow) > bandScale) {
    bandScale = Math.abs(prevSlow);
  }
  bandScale = bandScale * periodScale;
  lineValue = 0.0;
  if (!(Math.abs(spread) <= 0.00000000000001 * (bandScale)) && !(Math.abs(prevATR) <= 0.00000000000001 * (bandScale)) && prevATR >= 2.2250738585072014e-308) {
    lineValue = spread / prevATR * 100.0;
  }
  /* Seed the signal EMA with a simple average of the first 'signal period'
   * line values, accumulated as they are produced.
   */
  prevSignal = 0.0;
  prevSignal += lineValue;
  i = optInSignalPeriod - 1;
  while (i-- > 0) {
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    tempReal = inClose[today];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    {
      const _a: number = wBeta, _b: number = prevATR, _c: number = wAlpha * greatest;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevATR = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = fastBeta, _b: number = prevFast, _c: number = fastK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevFast = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = slowBeta, _b: number = prevSlow, _c: number = slowK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevSlow = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    spread = prevFast - prevSlow;
    bandScale = Math.abs(prevFast);
    if (Math.abs(prevSlow) > bandScale) {
      bandScale = Math.abs(prevSlow);
    }
    bandScale = bandScale * periodScale;
    lineValue = 0.0;
    if (!(Math.abs(spread) <= 0.00000000000001 * (bandScale)) && !(Math.abs(prevATR) <= 0.00000000000001 * (bandScale)) && prevATR >= 2.2250738585072014e-308) {
      lineValue = spread / prevATR * 100.0;
    }
    prevSignal += lineValue;
    today += 1;
  }
  prevSignal = prevSignal / optInSignalPeriod;
  /* Advance everything in lockstep through the unstable period of the
   * signal EMA, up to the first output bar.
   */
  while (today <= startIdx) {
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    tempReal = inClose[today];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    {
      const _a: number = wBeta, _b: number = prevATR, _c: number = wAlpha * greatest;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevATR = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = fastBeta, _b: number = prevFast, _c: number = fastK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevFast = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = slowBeta, _b: number = prevSlow, _c: number = slowK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevSlow = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    spread = prevFast - prevSlow;
    bandScale = Math.abs(prevFast);
    if (Math.abs(prevSlow) > bandScale) {
      bandScale = Math.abs(prevSlow);
    }
    bandScale = bandScale * periodScale;
    lineValue = 0.0;
    if (!(Math.abs(spread) <= 0.00000000000001 * (bandScale)) && !(Math.abs(prevATR) <= 0.00000000000001 * (bandScale)) && prevATR >= 2.2250738585072014e-308) {
      lineValue = spread / prevATR * 100.0;
    }
    if (optInSignalPeriod === 1) {
      prevSignal = lineValue;
    } else {
      {
        const _a: number = signalBeta, _b: number = prevSignal, _c: number = signalK * lineValue;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        prevSignal = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
    }
    today += 1;
  }
  /* Stable zone: keep advancing in lockstep and write the three outputs. */
  outMACDV[((0 * outStride) | 0)] = lineValue;
  outMACDVSignal[((0 * outStride) | 0)] = prevSignal;
  outMACDVHist[((0 * outStride) | 0)] = lineValue - prevSignal;
  outIdx = 1;
  while (today <= endIdx) {
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    tempReal = inClose[today];
    greatest = tempHT - tempLT;
    /* val1 */
    val2 = Math.abs(tempCY - tempHT);
    if (val2 > greatest) {
      greatest = val2;
    }
    val3 = Math.abs(tempCY - tempLT);
    if (val3 > greatest) {
      greatest = val3;
    }
    {
      const _a: number = wBeta, _b: number = prevATR, _c: number = wAlpha * greatest;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevATR = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = fastBeta, _b: number = prevFast, _c: number = fastK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevFast = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = slowBeta, _b: number = prevSlow, _c: number = slowK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevSlow = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    spread = prevFast - prevSlow;
    bandScale = Math.abs(prevFast);
    if (Math.abs(prevSlow) > bandScale) {
      bandScale = Math.abs(prevSlow);
    }
    bandScale = bandScale * periodScale;
    lineValue = 0.0;
    if (!(Math.abs(spread) <= 0.00000000000001 * (bandScale)) && !(Math.abs(prevATR) <= 0.00000000000001 * (bandScale)) && prevATR >= 2.2250738585072014e-308) {
      lineValue = spread / prevATR * 100.0;
    }
    if (optInSignalPeriod === 1) {
      prevSignal = lineValue;
    } else {
      {
        const _a: number = signalBeta, _b: number = prevSignal, _c: number = signalK * lineValue;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        prevSignal = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
    }
    outMACDV[((outIdx * outStride) | 0)] = lineValue;
    outMACDVSignal[((outIdx * outStride) | 0)] = prevSignal;
    outMACDVHist[((outIdx * outStride) | 0)] = lineValue - prevSignal;
    outIdx += 1;
    today += 1;
  }
  /* All done! Indicate the output limits and return success. */
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  sp.optInFastPeriod = optInFastPeriod;
  sp.optInSlowPeriod = optInSlowPeriod;
  sp.optInSignalPeriod = optInSignalPeriod;
  sp.prevFast = prevFast;
  sp.prevSlow = prevSlow;
  sp.prevATR = prevATR;
  sp.prevSignal = prevSignal;
  sp.slowK = slowK;
  sp.fastK = fastK;
  sp.signalK = signalK;
  sp.slowBeta = slowBeta;
  sp.fastBeta = fastBeta;
  sp.signalBeta = signalBeta;
  sp.wAlpha = wAlpha;
  sp.wBeta = wBeta;
  sp.periodScale = periodScale;
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.cur_outMACDV = outMACDV[(outNBElement.value - 1) * outStride];
  sp.cur_outMACDVSignal = outMACDVSignal[(outNBElement.value - 1) * outStride];
  sp.cur_outMACDVHist = outMACDVHist[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function macdvOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outMACDV: Float64Array,
  outMACDVSignal: Float64Array,
  outMACDVHist: Float64Array,
): MacdvStream {
  const sp = MacdvStream.of(core);
  const retCode = macdvOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outMACDV, outMACDVSignal, outMACDVHist, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MACDV openAndFill', inHigh.length, startIdx, macdvLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && macdvLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core) < 0) {
    throw rejectedParams('MACDV openAndFill', PARAM_RANGES, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
  }
  throw streamFailure('MACDV openAndFill', retCode);
}

/** @internal */
export function macdvOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
): MacdvStream {
  const sp = MacdvStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outMACDV = new Float64Array(1);
  const sink_outMACDVSignal = new Float64Array(1);
  const sink_outMACDVHist = new Float64Array(1);
  const retCode = macdvOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, sink_outMACDV, sink_outMACDVSignal, sink_outMACDVHist, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MACDV open', inHigh.length, startIdx, macdvLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && macdvLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core) < 0) {
    throw rejectedParams('MACDV open', PARAM_RANGES, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
  }
  throw streamFailure('MACDV open', retCode);
}

/** @internal */
export function macdvOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  core: Core = Core.DEFAULT,
): MacdvStream {
  optInFastPeriod = requireInt32('MACDV open', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('MACDV open', 'slowPeriod', optInSlowPeriod);
  optInSignalPeriod = requireInt32('MACDV open', 'signalPeriod', optInSignalPeriod);
  requireArgument('MACDV open', 'prices.high', inHigh);
  requireHistory('MACDV open', 'prices.high', inHigh.length);
  requireArgument('MACDV open', 'prices.low', inLow);
  requireArgument('MACDV open', 'prices.close', inClose);
  requireHistoryLength('MACDV open', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('MACDV open', 'prices.close', inClose.length, inHigh.length);
  return macdvOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
}

/** @internal */
export function macdvOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  outMACDV: Float64Array,
  outMACDVSignal: Float64Array,
  outMACDVHist: Float64Array,
  core: Core = Core.DEFAULT,
): MacdvStream {
  optInFastPeriod = requireInt32('MACDV openAndFill', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('MACDV openAndFill', 'slowPeriod', optInSlowPeriod);
  optInSignalPeriod = requireInt32('MACDV openAndFill', 'signalPeriod', optInSignalPeriod);
  requireArgument('MACDV openAndFill', 'prices.high', inHigh);
  requireHistory('MACDV openAndFill', 'prices.high', inHigh.length);
  requireArgument('MACDV openAndFill', 'prices.low', inLow);
  requireArgument('MACDV openAndFill', 'prices.close', inClose);
  const guardLookback = macdvLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('MACDV openAndFill', PARAM_RANGES, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
  }
  const guardOutLen = openFillCount('MACDV openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('MACDV openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('MACDV openAndFill', 'prices.close', inClose.length, inHigh.length);
  requireOutput('MACDV openAndFill', 'outMacdv', outMACDV, guardOutLen, Float64Array);
  requireOutput('MACDV openAndFill', 'outMacdvSignal', outMACDVSignal, guardOutLen, Float64Array);
  requireOutput('MACDV openAndFill', 'outMacdvHist', outMACDVHist, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outMACDV.buffer === inHighF64.buffer && outMACDV.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('MACDV openAndFill', 'outMacdv', 'prices.high');
  }
  if (outMACDV.buffer === inLowF64.buffer && outMACDV.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('MACDV openAndFill', 'outMacdv', 'prices.low');
  }
  if (outMACDV.buffer === inCloseF64.buffer && outMACDV.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('MACDV openAndFill', 'outMacdv', 'prices.close');
  }
  if (outMACDVSignal.buffer === inHighF64.buffer && outMACDVSignal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('MACDV openAndFill', 'outMacdvSignal', 'prices.high');
  }
  if (outMACDVSignal.buffer === inLowF64.buffer && outMACDVSignal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('MACDV openAndFill', 'outMacdvSignal', 'prices.low');
  }
  if (outMACDVSignal.buffer === inCloseF64.buffer && outMACDVSignal.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('MACDV openAndFill', 'outMacdvSignal', 'prices.close');
  }
  if (outMACDVHist.buffer === inHighF64.buffer && outMACDVHist.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('MACDV openAndFill', 'outMacdvHist', 'prices.high');
  }
  if (outMACDVHist.buffer === inLowF64.buffer && outMACDVHist.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('MACDV openAndFill', 'outMacdvHist', 'prices.low');
  }
  if (outMACDVHist.buffer === inCloseF64.buffer && outMACDVHist.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('MACDV openAndFill', 'outMacdvHist', 'prices.close');
  }
  if (outMACDV.buffer === outMACDVSignal.buffer && outMACDV.byteOffset === outMACDVSignal.byteOffset) {
    throw sharedMemory('MACDV openAndFill', 'outMacdv', 'outMacdvSignal');
  }
  if (outMACDV.buffer === outMACDVHist.buffer && outMACDV.byteOffset === outMACDVHist.byteOffset) {
    throw sharedMemory('MACDV openAndFill', 'outMacdv', 'outMacdvHist');
  }
  if (outMACDVSignal.buffer === outMACDVHist.buffer && outMACDVSignal.byteOffset === outMACDVHist.byteOffset) {
    throw sharedMemory('MACDV openAndFill', 'outMacdvSignal', 'outMacdvHist');
  }
  if (guardOutLen > 0 && overlaps(outMACDV, inHighF64)) {
    throw sharedMemory('MACDV openAndFill', 'outMacdv', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outMACDV, inLowF64)) {
    throw sharedMemory('MACDV openAndFill', 'outMacdv', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outMACDV, inCloseF64)) {
    throw sharedMemory('MACDV openAndFill', 'outMacdv', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outMACDVSignal, inHighF64)) {
    throw sharedMemory('MACDV openAndFill', 'outMacdvSignal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outMACDVSignal, inLowF64)) {
    throw sharedMemory('MACDV openAndFill', 'outMacdvSignal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outMACDVSignal, inCloseF64)) {
    throw sharedMemory('MACDV openAndFill', 'outMacdvSignal', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outMACDVHist, inHighF64)) {
    throw sharedMemory('MACDV openAndFill', 'outMacdvHist', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outMACDVHist, inLowF64)) {
    throw sharedMemory('MACDV openAndFill', 'outMacdvHist', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outMACDVHist, inCloseF64)) {
    throw sharedMemory('MACDV openAndFill', 'outMacdvHist', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outMACDV, outMACDVSignal)) {
    throw sharedMemory('MACDV openAndFill', 'outMacdv', 'outMacdvSignal');
  }
  if (guardOutLen > 0 && overlaps(outMACDV, outMACDVHist)) {
    throw sharedMemory('MACDV openAndFill', 'outMacdv', 'outMacdvHist');
  }
  if (guardOutLen > 0 && overlaps(outMACDVSignal, outMACDVHist)) {
    throw sharedMemory('MACDV openAndFill', 'outMacdvSignal', 'outMacdvHist');
  }
  return macdvOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, new MInteger(), new MInteger(), outMACDV, outMACDVSignal, outMACDVHist);
}

/**
 * Opens a live MACDV stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link macdv} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar. `close`: Close
 *   price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MacdvOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `macdvLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range
 *   or histories of different lengths.
 *
 * @example
 * ```ts
 * import { macdvOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = macdvOpen({ high, low, close }, {
 *   fastPeriod: 12,
 *   slowPeriod: 26,
 *   signalPeriod: 9,
 * });
 * const peeked = stream.peek(101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function macdvOpen(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: MacdvOptions): MacdvStream {
  requireArgument('MACDV open', 'prices', prices);
  const o = optionsOf('MACDV open', options);
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MACDV open', o.core);
  return macdvOpenPositional(prices.high, prices.low, prices.close, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core);
}

/**
 * {@link macdvOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link macdv}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar. `close`: Close
 *   price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link MacdvOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link macdv} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `macdvLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { macdvOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const {
 *   stream,
 *   begIdx,
 *   outMacdv,
 *   outMacdvSignal,
 *   outMacdvHist,
 * } = macdvOpenAndFill({ high, low, close }, {
 *   fastPeriod: 12,
 *   slowPeriod: 26,
 *   signalPeriod: 9,
 * });
 * // outMacdv[k] is the value at bar begIdx + k
 * ```
 */
export function macdvOpenAndFill(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: MacdvOptions): MacdvResult & { stream: MacdvStream } {
  requireArgument('MACDV openAndFill', 'prices', prices);
  const o = optionsOf('MACDV openAndFill', options);
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MACDV openAndFill', o.core);
  const capacity = fillCapacity(macdvLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core), prices.high, prices.low, prices.close);
  const outMACDV = o.outMacdv === undefined ? new Float64Array(capacity) : o.outMacdv;
  const outMACDVSignal = o.outMacdvSignal === undefined ? new Float64Array(capacity) : o.outMacdvSignal;
  const outMACDVHist = o.outMacdvHist === undefined ? new Float64Array(capacity) : o.outMacdvHist;
  const stream = macdvOpenAndFillPositional(prices.high, prices.low, prices.close, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outMACDV, outMACDVSignal, outMACDVHist, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outMacdv: outMACDV.subarray(0, range.count), outMacdvSignal: outMACDVSignal.subarray(0, range.count), outMacdvHist: outMACDVHist.subarray(0, range.count) };
}
