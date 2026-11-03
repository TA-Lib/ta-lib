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

import { badSink, clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { atrLookback } from './atr.ts';
import { emaLookback } from './ema.ts';

/**
 * Number of leading input bars {@link macdv} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInFastPeriod - Period of the fast EMA (default 12; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowPeriod - Period of the slow EMA and of the Average True Range
 *   (default 26; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSignalPeriod - Smoothing period of the signal line (default 9; range
 *   1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function macdvLookback(optInFastPeriod: number, optInSlowPeriod: number, optInSignalPeriod: number, core: Core = Core.DEFAULT): number {
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
  lookbackLine = emaLookback(optInSlowPeriod, core);
  lookbackATR = atrLookback(optInSlowPeriod, core);
  if (lookbackATR > lookbackLine) {
    lookbackLine = lookbackATR;
  }
  return lookbackLine + emaLookback(optInSignalPeriod, core);
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link macdv}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInFastPeriod - Period of the fast EMA (default 12; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowPeriod - Period of the slow EMA and of the Average True Range
 *   (default 26; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSignalPeriod - Smoothing period of the signal line (default 9; range
 *   1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function macdvDisplayShift(optInFastPeriod: number, optInSlowPeriod: number, optInSignalPeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (macdvLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core) < 0) {
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
  lookbackSignal = emaLookback(optInSignalPeriod, core);
  lookbackSlow = emaLookback(optInSlowPeriod, core);
  lookbackFast = emaLookback(optInFastPeriod, core);
  lookbackATR = atrLookback(optInSlowPeriod, core);
  lookbackTotal = macdvLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core);
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
 *   `TA_FUNC_UNST_EMA` and `TA_FUNC_UNST_ATR`, as for those functions.
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
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link macdvLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param optInFastPeriod - Period of the fast EMA (default 12; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowPeriod - Period of the slow EMA and of the Average True Range
 *   (default 26; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSignalPeriod - Smoothing period of the signal line (default 9; range
 *   1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outMACDV - Fast EMA minus slow EMA, divided by the Average True Range,
 *   times 100. Must hold the values produced and never be empty; the input length
 *   always suffices.
 * @param outMACDVSignal - EMA of the MACD-V line. Must hold the values produced
 *   and never be empty; the input length always suffices.
 * @param outMACDVHist - MACD-V minus signal line. Must hold the values produced
 *   and never be empty; the input length always suffices.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The range written: `begIdx` is the first bar with a value, `count` how
 *   many were written.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for an argument that is not an int32, a
 *   parameter outside its range, an array too short for the range, an empty output,
 *   two outputs sharing memory or an output sharing memory with a `Float64Array`
 *   input it is not. Checked before anything is written.
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
 * const macdvOut = new Float64Array(high.length);
 * const macdvSignal = new Float64Array(high.length);
 * const macdvHist = new Float64Array(high.length);
 * const { begIdx, count } = macdv(
 *   0, high.length - 1,
 *   high, low, close,
 *   12, 26, 9,
 *   macdvOut, macdvSignal, macdvHist,
 * );
 * // macdvOut[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`macd`](https://ta-lib.org/functions/macd)
 * @see [`atr`](https://ta-lib.org/functions/atr)
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`apo`](https://ta-lib.org/functions/apo)
 * @see [`ppo`](https://ta-lib.org/functions/ppo)
 */
export function macdv(
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
  optInFastPeriod = requireInt32('MACDV', 'optInFastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('MACDV', 'optInSlowPeriod', optInSlowPeriod);
  optInSignalPeriod = requireInt32('MACDV', 'optInSignalPeriod', optInSignalPeriod);
  requireIndexRange('MACDV', startIdx, endIdx);
  const guardStart = clampedStart('MACDV', startIdx, macdvLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('MACDV', 'inHigh', inHigh, guardInLen);
  requireLength('MACDV', 'inLow', inLow, guardInLen);
  requireLength('MACDV', 'inClose', inClose, guardInLen);
  requireLength('MACDV', 'outMACDV', outMACDV, guardOutLen);
  requireLength('MACDV', 'outMACDVSignal', outMACDVSignal, guardOutLen);
  requireLength('MACDV', 'outMACDVHist', outMACDVHist, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outMACDV, inHighF64) || overlapsPartly(outMACDV, inLowF64) || overlapsPartly(outMACDV, inCloseF64) || overlapsPartly(outMACDVSignal, inHighF64) || overlapsPartly(outMACDVSignal, inLowF64) || overlapsPartly(outMACDVSignal, inCloseF64) || overlapsPartly(outMACDVHist, inHighF64) || overlapsPartly(outMACDVHist, inLowF64) || overlapsPartly(outMACDVHist, inCloseF64) || overlaps(outMACDV, outMACDVSignal) || overlaps(outMACDV, outMACDVHist) || overlaps(outMACDVSignal, outMACDVHist)) {
    throw failure('MACDV', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = macdvImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outMACDV, outMACDVSignal, outMACDVHist);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('MACDV', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
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

  /** @internal */
  constructor(core: Core) {
    this.core = core;
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
      throw failure('MACDV advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar, writing the new values into `out`.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number, inClose: number, out: MacdvOut): void {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('MACDV update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (typeof out !== 'object' || out === null) {
      throw badSink('MACDV update', 'MacdvOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('MACDV update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    macdvStepImpl(this, inHigh, inLow, inClose);
    this.outRangeCount++;
    out.macdv = this.cur_outMACDV;
    out.macdvSignal = this.cur_outMACDVSignal;
    out.macdvHist = this.cur_outMACDVHist;
  }

  /**
   * Writes into `out` what the next `update` with the same bar would write,
   * committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number, inClose: number, out: MacdvOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('MACDV peek', 'MacdvOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('MACDV peek', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
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
    out.macdv = cur_outMACDV;
    out.macdvSignal = cur_outMACDVSignal;
    out.macdvHist = cur_outMACDVHist;
  }

  /**
   * The value at the last bar this stream counted; `peek` does not change it.
   *
   * @param out - Receives the outputs.
   */
  value(out: MacdvOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('MACDV value', 'MacdvOut');
    }
    out.macdv = this.cur_outMACDV;
    out.macdvSignal = this.cur_outMACDVSignal;
    out.macdvHist = this.cur_outMACDVHist;
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
 * The outputs of one MACDV bar, written into an object the caller owns and reuses:
 * valid until the next call that writes it.
 */
export class MacdvOut {
  /** Fast EMA minus slow EMA, divided by the Average True Range, times 100. */
  macdv: number = 0;
  /** EMA of the MACD-V line. */
  macdvSignal: number = 0;
  /** MACD-V minus signal line. */
  macdvHist: number = 0;
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
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
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
  if (startIdx > endIdx) {
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
  lookbackSignal = emaLookback(optInSignalPeriod, core);
  lookbackSlow = emaLookback(optInSlowPeriod, core);
  lookbackFast = emaLookback(optInFastPeriod, core);
  lookbackATR = atrLookback(optInSlowPeriod, core);
  lookbackTotal = macdvLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core);
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
  const sp = new MacdvStream(core);
  const retCode = macdvOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outMACDV, outMACDVSignal, outMACDVHist, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MACDV openAndFill', inHigh.length, startIdx, macdvLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core));
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
  const sp = new MacdvStream(core);
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
    throw insufficientHistory('MACDV open', inHigh.length, startIdx, macdvLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core));
  }
  throw streamFailure('MACDV open', retCode);
}

/**
 * Opens a live MACDV stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link macdv} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInFastPeriod - Period of the fast EMA (default 12; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowPeriod - Period of the slow EMA and of the Average True Range
 *   (default 26; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSignalPeriod - Smoothing period of the signal line (default 9; range
 *   1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `macdvLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { MacdvOut, macdvOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = macdvOpen(high, low, close, 12, 26, 9);
 * const peeked = new MacdvOut();
 * const updated = new MacdvOut();
 * stream.peek(101.4, 99.1, 100.9, peeked); // a forming bar: commits nothing
 * stream.update(101.4, 99.1, 100.9, updated); // the bar closes
 * ```
 */
export function macdvOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  core: Core = Core.DEFAULT,
): MacdvStream {
  optInFastPeriod = requireInt32('MACDV open', 'optInFastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('MACDV open', 'optInSlowPeriod', optInSlowPeriod);
  optInSignalPeriod = requireInt32('MACDV open', 'optInSignalPeriod', optInSignalPeriod);
  requireArgument('MACDV open', 'inHigh', inHigh);
  requireHistory('MACDV open', inHigh.length);
  requireArgument('MACDV open', 'inLow', inLow);
  requireArgument('MACDV open', 'inClose', inClose);
  requireHistoryLength('MACDV open', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('MACDV open', 'inClose', inClose.length, inHigh.length);
  return macdvOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
}

/**
 * {@link macdvOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link macdv}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInFastPeriod - Period of the fast EMA (default 12; range 2 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowPeriod - Period of the slow EMA and of the Average True Range
 *   (default 26; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSignalPeriod - Smoothing period of the signal line (default 9; range
 *   1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outMACDV - Fast EMA minus slow EMA, divided by the Average True Range,
 *   times 100. Must hold `historyLen - lookback` values and share no memory with an
 *   input or another output.
 * @param outMACDVSignal - EMA of the MACD-V line. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param outMACDVHist - MACD-V minus signal line. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `macdvLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range, histories of different lengths or an output too
 *   short or starting where another output or a `Float64Array` input starts, or, on
 *   a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
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
 * const macdvOut = new Float64Array(high.length);
 * const macdvSignal = new Float64Array(high.length);
 * const macdvHist = new Float64Array(high.length);
 * const stream = macdvOpenAndFill(
 *   high, low, close,
 *   12, 26, 9,
 *   macdvOut, macdvSignal, macdvHist,
 * );
 * const { begIdx, count } = stream.outRange();
 * // macdvOut[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function macdvOpenAndFill(
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
  optInFastPeriod = requireInt32('MACDV openAndFill', 'optInFastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('MACDV openAndFill', 'optInSlowPeriod', optInSlowPeriod);
  optInSignalPeriod = requireInt32('MACDV openAndFill', 'optInSignalPeriod', optInSignalPeriod);
  requireArgument('MACDV openAndFill', 'inHigh', inHigh);
  requireHistory('MACDV openAndFill', inHigh.length);
  requireArgument('MACDV openAndFill', 'inLow', inLow);
  requireArgument('MACDV openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('MACDV openAndFill', inHigh.length, macdvLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core));
  requireHistoryLength('MACDV openAndFill', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('MACDV openAndFill', 'inClose', inClose.length, inHigh.length);
  requireLength('MACDV openAndFill', 'outMACDV', outMACDV, guardOutLen);
  requireLength('MACDV openAndFill', 'outMACDVSignal', outMACDVSignal, guardOutLen);
  requireLength('MACDV openAndFill', 'outMACDVHist', outMACDVHist, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outMACDV.buffer === inHighF64.buffer && outMACDV.byteOffset === inHighF64.byteOffset) || (outMACDV.buffer === inLowF64.buffer && outMACDV.byteOffset === inLowF64.byteOffset) || (outMACDV.buffer === inCloseF64.buffer && outMACDV.byteOffset === inCloseF64.byteOffset) || (outMACDVSignal.buffer === inHighF64.buffer && outMACDVSignal.byteOffset === inHighF64.byteOffset) || (outMACDVSignal.buffer === inLowF64.buffer && outMACDVSignal.byteOffset === inLowF64.byteOffset) || (outMACDVSignal.buffer === inCloseF64.buffer && outMACDVSignal.byteOffset === inCloseF64.byteOffset) || (outMACDVHist.buffer === inHighF64.buffer && outMACDVHist.byteOffset === inHighF64.byteOffset) || (outMACDVHist.buffer === inLowF64.buffer && outMACDVHist.byteOffset === inLowF64.byteOffset) || (outMACDVHist.buffer === inCloseF64.buffer && outMACDVHist.byteOffset === inCloseF64.byteOffset) || (outMACDV.buffer === outMACDVSignal.buffer && outMACDV.byteOffset === outMACDVSignal.byteOffset) || (outMACDV.buffer === outMACDVHist.buffer && outMACDV.byteOffset === outMACDVHist.byteOffset) || (outMACDVSignal.buffer === outMACDVHist.buffer && outMACDVSignal.byteOffset === outMACDVHist.byteOffset)) {
    throw streamFailure('MACDV openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outMACDV, inHighF64) || overlaps(outMACDV, inLowF64) || overlaps(outMACDV, inCloseF64) || overlaps(outMACDVSignal, inHighF64) || overlaps(outMACDVSignal, inLowF64) || overlaps(outMACDVSignal, inCloseF64) || overlaps(outMACDVHist, inHighF64) || overlaps(outMACDVHist, inLowF64) || overlaps(outMACDVHist, inCloseF64) || overlaps(outMACDV, outMACDVSignal) || overlaps(outMACDV, outMACDVHist) || overlaps(outMACDVSignal, outMACDVHist))) {
    throw streamFailure('MACDV openAndFill', RetCode.BAD_PARAM);
  }
  return macdvOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, new MInteger(), new MInteger(), outMACDV, outMACDVSignal, outMACDVHist);
}
