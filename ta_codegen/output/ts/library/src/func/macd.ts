/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  JPP      JP Pienaar (j.pienaar@mci.co.za)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  112400 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  080403 JPP    Fix #767653 for logic when swapping periods.
 *  070526 MF,CC  Speed optimization: compute the two price EMA, the
 *                signal line and the histogram in a single lockstep
 *                pass (bit-exact, no temporary buffers).
 *  080926 MF,CC  Explicit no-smoothing signal at a signal period of 1.
 */

import { badSink, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { emaLookbackPositional } from './ema.ts';

/** @internal */
export function macdLookbackPositional(optInFastPeriod: number, optInSlowPeriod: number, optInSignalPeriod: number, core: Core = Core.DEFAULT): number {
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
  /* The lookback is driven by the signal line output.
   *
   * (must also account for the initial data consume
   *  by the slow period).
   */
  /* Make sure slow is really slower than
   * the fast period! if not, swap...
   */
  if (optInSlowPeriod < optInFastPeriod) {
    /* swap */
    tempInteger = optInSlowPeriod;
    optInSlowPeriod = optInFastPeriod;
    optInFastPeriod = tempInteger;
  }
  return emaLookbackPositional(optInSlowPeriod, core) + emaLookbackPositional(optInSignalPeriod, core);
}

/** @internal */
export function macdDisplayShiftPositional(optInFastPeriod: number, optInSlowPeriod: number, optInSignalPeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (macdLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 3) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function macdImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outMACD: Float64Array,
  outMACDSignal: Float64Array,
  outMACDHist: Float64Array,
): RetCode {
  let prevFast = 0;
  let prevSlow = 0;
  let prevSignal = 0;
  let macdValue = 0;
  let tempReal = 0;
  let slowK = 0;
  let fastK = 0;
  let signalK = 0;
  let slowBeta = 0;
  let fastBeta = 0;
  let signalBeta = 0;
  let i = 0;
  let today = 0;
  let outIdx = 0;
  let tempInteger = 0;
  let lookbackTotal = 0;
  let lookbackSignal = 0;
  let lookbackSlow = 0;
  let fastToday = 0;
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
  if ((outMACD.buffer === outMACDSignal.buffer && outMACD.byteOffset === outMACDSignal.byteOffset) || (outMACD.buffer === outMACDHist.buffer && outMACD.byteOffset === outMACDHist.byteOffset) || (outMACDSignal.buffer === outMACDHist.buffer && outMACDSignal.byteOffset === outMACDHist.byteOffset)) {
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
  /* The fixed 26/12 MACD: k of 0.075 and 0.15, as near as a pair summing
   * to exactly 1.0 comes.
   */
  if (optInSlowPeriod === 0) {
    /* Fix 26 */
    optInSlowPeriod = 26;
    slowBeta = 1.0 - 0.075;
  } else {
    slowBeta = (optInSlowPeriod - 1) / (optInSlowPeriod + 1);
  }
  slowK = 1.0 - slowBeta;
  if (slowBeta < 0.5) {
    slowBeta = 1.0 - slowK;
  }
  if (optInFastPeriod === 0) {
    /* Fix 12 */
    optInFastPeriod = 12;
    fastBeta = 1.0 - 0.15;
  } else {
    fastBeta = (optInFastPeriod - 1) / (optInFastPeriod + 1);
  }
  fastK = 1.0 - fastBeta;
  if (fastBeta < 0.5) {
    fastBeta = 1.0 - fastK;
  }
  /* A signal period of 1 disables signal-line smoothing: the signal IS the
   * MACD line and the histogram is exactly zero. The recursion
   * below, at a k of 1.0 and a beta of 0.0, does not keep the sign of a
   * -0.0 line value; hence the explicit arm at each step.
   */
  signalBeta = (optInSignalPeriod - 1) / (optInSignalPeriod + 1);
  signalK = 1.0 - signalBeta;
  if (signalBeta < 0.5) {
    signalBeta = 1.0 - signalK;
  }
  lookbackSignal = emaLookbackPositional(optInSignalPeriod, core);
  /* Move up the start index if there is not
   * enough initial data.
   */
  lookbackSlow = emaLookbackPositional(optInSlowPeriod, core);
  lookbackTotal = lookbackSignal + lookbackSlow;
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  /* Everything is computed in a single lockstep pass: each bar
   * advances the fast and slow EMA (two independent recursions),
   * their difference is the MACD line, and each MACD-line value
   * is immediately fed into the signal EMA. No temporary buffers.
   *
   * The arithmetic order below is the bit-exactness contract
   * (do not reorder or fuse operations):
   *  - EMA recursion: k*x + beta*prev, with ema.c's k and beta.
   *  - Each EMA is seeded with the sum of its first 'period'
   *    inputs, accumulated from 0.0 in input order, divided by
   *    the period. The signal EMA is seeded the same way from the
   *    first 'signal period' MACD-line values.
   *
   * In-place (an output == inReal) is supported: outputs at
   * [outIdx] are written only after inReal[startIdx+outIdx] was
   * read.
   */
  /* Seed each price EMA with a simple average of its first
   * 'period' price bars, each window placed by that EMA's own
   * lookback, so that the line is TA_EMA(fast) - TA_EMA(slow) bit
   * for bit. The slow EMA then runs alone to the end of the fast
   * window.
   *
   * ema_lookback(n) - n must never decrease as n grows: a fast
   * window ending before the slow one would skip bars of the fast
   * EMA, and one starting before it would read below the lookback.
   */
  today = startIdx - lookbackTotal;
  tempReal = 0.0;
  i = optInSlowPeriod;
  while (i-- > 0) {
    tempReal += inReal[today++];
  }
  prevSlow = tempReal / optInSlowPeriod;
  fastToday = startIdx - lookbackTotal + (lookbackSlow - emaLookbackPositional(optInFastPeriod, core));
  prevFast = 0.0;
  i = optInFastPeriod;
  while (i-- > 0) {
    prevFast += inReal[fastToday++];
  }
  prevFast = prevFast / optInFastPeriod;
  while (today < fastToday) {
    tempReal = inReal[today++];
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
  /* Advance both EMA through their unstable period, up to the
   * first MACD-line bar.
   */
  while (today <= startIdx - lookbackSignal) {
    tempReal = inReal[today++];
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
  }
  macdValue = prevFast - prevSlow;
  /* Seed the signal EMA with a simple average of the first
   * 'signal period' MACD-line values, accumulated as they are
   * produced.
   */
  prevSignal = 0.0;
  prevSignal += macdValue;
  i = optInSignalPeriod - 1;
  while (i-- > 0) {
    tempReal = inReal[today++];
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
    macdValue = prevFast - prevSlow;
    prevSignal += macdValue;
  }
  prevSignal = prevSignal / optInSignalPeriod;
  /* Advance everything in lockstep through the unstable period
   * of the signal EMA, up to the first output bar.
   */
  while (today <= startIdx) {
    tempReal = inReal[today++];
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
    macdValue = prevFast - prevSlow;
    if (optInSignalPeriod === 1) {
      prevSignal = macdValue;
    } else {
      {
        const _a: number = signalBeta, _b: number = prevSignal, _c: number = signalK * macdValue;
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
  }
  /* Stable zone: keep advancing in lockstep and write the three
   * outputs.
   */
  outMACD[0] = macdValue;
  outMACDSignal[0] = prevSignal;
  outMACDHist[0] = macdValue - prevSignal;
  outIdx = 1;
  while (today <= endIdx) {
    tempReal = inReal[today++];
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
    macdValue = prevFast - prevSlow;
    if (optInSignalPeriod === 1) {
      prevSignal = macdValue;
    } else {
      {
        const _a: number = signalBeta, _b: number = prevSignal, _c: number = signalK * macdValue;
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
    outMACD[outIdx] = macdValue;
    outMACDSignal[outIdx] = prevSignal;
    outMACDHist[outIdx] = macdValue - prevSignal;
    outIdx += 1;
  }
  /* All done! Indicate the output limits and return success. */
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['fastPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['slowPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['signalPeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000']] as const;

/** @internal */
export function macdPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  outMACD: Float64Array,
  outMACDSignal: Float64Array,
  outMACDHist: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('MACD', 'startIdx', startIdx);
  endIdx = requireInt32('MACD', 'endIdx', endIdx);
  optInFastPeriod = requireInt32('MACD', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('MACD', 'slowPeriod', optInSlowPeriod);
  optInSignalPeriod = requireInt32('MACD', 'signalPeriod', optInSignalPeriod);
  requireIndexRange('MACD', startIdx, endIdx);
  const guardLookback = macdLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('MACD', PARAM_RANGES, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
  }
  const guardStart = clampedStart('MACD', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('MACD', 'real', inReal, guardInLen);
  requireOutput('MACD', 'outMacd', outMACD, guardOutLen, Float64Array);
  requireOutput('MACD', 'outMacdSignal', outMACDSignal, guardOutLen, Float64Array);
  requireOutput('MACD', 'outMacdHist', outMACDHist, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outMACD, inRealF64)) {
    throw sharedMemory('MACD', 'outMacd', 'real');
  }
  if (overlapsPartly(outMACDSignal, inRealF64)) {
    throw sharedMemory('MACD', 'outMacdSignal', 'real');
  }
  if (overlapsPartly(outMACDHist, inRealF64)) {
    throw sharedMemory('MACD', 'outMacdHist', 'real');
  }
  if (overlaps(outMACD, outMACDSignal)) {
    throw sharedMemory('MACD', 'outMacd', 'outMacdSignal');
  }
  if (overlaps(outMACD, outMACDHist)) {
    throw sharedMemory('MACD', 'outMacd', 'outMacdHist');
  }
  if (overlaps(outMACDSignal, outMACDHist)) {
    throw sharedMemory('MACD', 'outMacdSignal', 'outMacdHist');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = macdImpl(core, startIdx, endIdx, inRealF64, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('MACD', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link macd}, {@link macdLookback}, {@link macdDisplayShift},
 * {@link macdOpen} and {@link macdOpenAndFill}. Every key is optional: an omitted
 * or `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface MacdOptions {
  /**
   * Period of the fast EMA (default 12; range 2 to 100000; `Core.INTEGER_DEFAULT`
   * selects the default).
   */
  fastPeriod?: number | undefined;
  /**
   * Period of the slow EMA (default 26; range 2 to 100000; `Core.INTEGER_DEFAULT`
   * selects the default).
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
   * Buffer for `outMacd`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outMacd?: Float64Array | undefined;
  /**
   * Buffer for `outMacdSignal`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outMacdSignal?: Float64Array | undefined;
  /**
   * Buffer for `outMacdHist`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outMacdHist?: Float64Array | undefined;
}

/** What {@link macd} returns, and {@link macdOpenAndFill} with its stream. */
export interface MacdResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /** Fast EMA minus slow EMA. `outMacd[k]` is the value at input bar `begIdx + k`. */
  outMacd: Float64Array;
  /** EMA of the MACD line. `outMacdSignal[k]` is the value at input bar `begIdx + k`. */
  outMacdSignal: Float64Array;
  /** MACD minus signal line. `outMacdHist[k]` is the value at input bar `begIdx + k`. */
  outMacdHist: Float64Array;
}

/**
 * Number of leading input bars {@link macd} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MacdOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function macdLookback(options?: MacdOptions): number {
  const o = optionsOf('MACD lookback', options);
  return macdLookbackPositional(orDefault(o.fastPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowPeriod, Core.INTEGER_DEFAULT), orDefault(o.signalPeriod, Core.INTEGER_DEFAULT), coreOf('MACD lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link macd}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MacdOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function macdDisplayShift(outputIdx: number, options?: MacdOptions): number {
  const o = optionsOf('MACD displayShift', options);
  return macdDisplayShiftPositional(orDefault(o.fastPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowPeriod, Core.INTEGER_DEFAULT), orDefault(o.signalPeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('MACD displayShift', o.core));
}

/**
 * Moving Average Convergence/Divergence: the difference between a fast and a slow
 * EMA of the input, plus an EMA-smoothed signal line and their histogram. MACD
 * crossing its signal line and histogram sign changes flag momentum shifts.
 *
 * Formula and more info at
 * [ta-lib.org/functions/macd](https://ta-lib.org/functions/macd).
 *
 * **Notes**
 * - If the slow period is set smaller than the fast period, the two are swapped so
 *   the slow EMA is always the longer one.
 * - A signal period of 1 disables signal-line smoothing: the signal equals the
 *   MACD line and the histogram is zero. Before 0.6.5 this parameter value
 *   produced misaligned output (issues #48/#59).
 *
 * `outMacd[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link macdLookback} is a success with empty outputs.
 *
 * @param real - Input series (typically close)
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link MacdOptions}.
 * @returns `begIdx` and the values; see {@link MacdResult}. Each output is a view
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
 * import { macd } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const {
 *   begIdx,
 *   outMacd,
 *   outMacdSignal,
 *   outMacdHist,
 * } = macd(data, { fastPeriod: 12, slowPeriod: 26, signalPeriod: 9 });
 * // outMacd[k] is the value at bar begIdx + k
 * ```
 * @see [`macdext`](https://ta-lib.org/functions/macdext)
 * @see [`macdfix`](https://ta-lib.org/functions/macdfix)
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`apo`](https://ta-lib.org/functions/apo)
 */
export function macd(real: ArrayLike<number>, options?: MacdOptions): MacdResult {
  const o = optionsOf('MACD', options);
  if (o.endIdx === undefined) {
    requireSeries('MACD', 'real', real);
  }
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MACD', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, macdLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core), real);
  const outMACD = o.outMacd === undefined ? new Float64Array(capacity) : o.outMacd;
  const outMACDSignal = o.outMacdSignal === undefined ? new Float64Array(capacity) : o.outMacdSignal;
  const outMACDHist = o.outMacdHist === undefined ? new Float64Array(capacity) : o.outMacdHist;
  const range = macdPositional(startIdx, endIdx, real, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outMACD, outMACDSignal, outMACDHist, core);
  return { begIdx: range.begIdx, outMacd: outMACD.subarray(0, range.count), outMacdSignal: outMACDSignal.subarray(0, range.count), outMacdHist: outMACDHist.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live MACD stream: one value per closed bar, bit-identical to {@link macd} over
 * the same series. Open it with {@link macdOpen} or {@link macdOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/macd](https://ta-lib.org/functions/macd).
 */
export class MacdStream {
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
  cur_outMACD: number = 0;
  /** @internal */
  cur_outMACDSignal: number = 0;
  /** @internal */
  cur_outMACDHist: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): MacdStream {
    return new MacdStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link macd} reports them over the same bars. Each accepted `update` and each
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
      throw failure('MACD advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns its outputs.
   *
   * @param inReal - The bar's `inReal` value: Input series (typically close)
   * @param out - Receives the outputs and is returned; a new {@link MacdOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number, out?: MacdOut): MacdOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('MACD update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('MACD update', 'MacdOut');
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('MACD update', 'inReal');
    }
    macdStepImpl(this, inReal);
    this.outRangeCount++;
    if (out === undefined) {
      out = new MacdOut();
    }
    out.outMacd = this.cur_outMACD;
    out.outMacdSignal = this.cur_outMACDSignal;
    out.outMacdHist = this.cur_outMACDHist;
    return out;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Input series (typically close)
   * @param out - Receives the outputs and is returned; a new {@link MacdOut} when
   *   omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number, out?: MacdOut): MacdOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('MACD peek', 'MacdOut');
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('MACD peek', 'inReal');
    }
    const sp = this;
    let macdValue = 0;
    let tempReal = 0;
    let cur_outMACD = 0;
    let cur_outMACDHist = 0;
    let cur_outMACDSignal = 0;
    let prevFast = sp.prevFast;
    let prevSignal = sp.prevSignal;
    let prevSlow = sp.prevSlow;
    tempReal = inReal;
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
    macdValue = prevFast - prevSlow;
    if (sp.optInSignalPeriod === 1) {
      prevSignal = macdValue;
    } else {
      {
        const _a: number = sp.signalBeta, _b: number = prevSignal, _c: number = sp.signalK * macdValue;
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
    cur_outMACD = macdValue;
    cur_outMACDSignal = prevSignal;
    cur_outMACDHist = macdValue - prevSignal;
    if (out === undefined) {
      out = new MacdOut();
    }
    out.outMacd = cur_outMACD;
    out.outMacdSignal = cur_outMACDSignal;
    out.outMacdHist = cur_outMACDHist;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link MacdOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   */
  value(out?: MacdOut): MacdOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('MACD value', 'MacdOut');
    }
    if (out === undefined) {
      out = new MacdOut();
    }
    out.outMacd = this.cur_outMACD;
    out.outMacdSignal = this.cur_outMACDSignal;
    out.outMacdHist = this.cur_outMACDHist;
    return out;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): MacdStream {
    const c = new MacdStream(this.core);
    c.optInFastPeriod = this.optInFastPeriod;
    c.optInSlowPeriod = this.optInSlowPeriod;
    c.optInSignalPeriod = this.optInSignalPeriod;
    c.prevFast = this.prevFast;
    c.prevSlow = this.prevSlow;
    c.prevSignal = this.prevSignal;
    c.slowK = this.slowK;
    c.fastK = this.fastK;
    c.signalK = this.signalK;
    c.slowBeta = this.slowBeta;
    c.fastBeta = this.fastBeta;
    c.signalBeta = this.signalBeta;
    c.cur_outMACD = this.cur_outMACD;
    c.cur_outMACDSignal = this.cur_outMACDSignal;
    c.cur_outMACDHist = this.cur_outMACDHist;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one MACD bar, as `update`, `peek` and `value` return them. Passed
 * back to one of them, it is filled and returned again, with no allocation; it
 * holds its values until the next call that writes it. A new one holds `NaN`.
 */
export class MacdOut {
  /** Fast EMA minus slow EMA. */
  outMacd: number = NaN;
  /** EMA of the MACD line. */
  outMacdSignal: number = NaN;
  /** MACD minus signal line. */
  outMacdHist: number = NaN;
}

function macdStepImpl(sp: MacdStream, inReal: number): void {
  let macdValue = 0;
  let tempReal = 0;
  tempReal = inReal;
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
  macdValue = sp.prevFast - sp.prevSlow;
  if (sp.optInSignalPeriod === 1) {
    sp.prevSignal = macdValue;
  } else {
    {
      const _a: number = sp.signalBeta, _b: number = sp.prevSignal, _c: number = sp.signalK * macdValue;
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
  sp.cur_outMACD = macdValue;
  sp.cur_outMACDSignal = sp.prevSignal;
  sp.cur_outMACDHist = macdValue - sp.prevSignal;
}

function macdOpenImpl(
  sp: MacdStream,
  inReal: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outMACD: Float64Array,
  outMACDSignal: Float64Array,
  outMACDHist: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let prevFast = 0;
  let prevSlow = 0;
  let prevSignal = 0;
  let macdValue = 0;
  let tempReal = 0;
  let slowK = 0;
  let fastK = 0;
  let signalK = 0;
  let slowBeta = 0;
  let fastBeta = 0;
  let signalBeta = 0;
  let i = 0;
  let today = 0;
  let outIdx = 0;
  let tempInteger = 0;
  let lookbackTotal = 0;
  let lookbackSignal = 0;
  let lookbackSlow = 0;
  let fastToday = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
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
  /* The fixed 26/12 MACD: k of 0.075 and 0.15, as near as a pair summing
   * to exactly 1.0 comes.
   */
  if (optInSlowPeriod === 0) {
    /* Fix 26 */
    optInSlowPeriod = 26;
    slowBeta = 1.0 - 0.075;
  } else {
    slowBeta = (optInSlowPeriod - 1) / (optInSlowPeriod + 1);
  }
  slowK = 1.0 - slowBeta;
  if (slowBeta < 0.5) {
    slowBeta = 1.0 - slowK;
  }
  if (optInFastPeriod === 0) {
    /* Fix 12 */
    optInFastPeriod = 12;
    fastBeta = 1.0 - 0.15;
  } else {
    fastBeta = (optInFastPeriod - 1) / (optInFastPeriod + 1);
  }
  fastK = 1.0 - fastBeta;
  if (fastBeta < 0.5) {
    fastBeta = 1.0 - fastK;
  }
  /* A signal period of 1 disables signal-line smoothing: the signal IS the
   * MACD line and the histogram is exactly zero. The recursion
   * below, at a k of 1.0 and a beta of 0.0, does not keep the sign of a
   * -0.0 line value; hence the explicit arm at each step.
   */
  signalBeta = (optInSignalPeriod - 1) / (optInSignalPeriod + 1);
  signalK = 1.0 - signalBeta;
  if (signalBeta < 0.5) {
    signalBeta = 1.0 - signalK;
  }
  lookbackSignal = emaLookbackPositional(optInSignalPeriod, core);
  /* Move up the start index if there is not
   * enough initial data.
   */
  lookbackSlow = emaLookbackPositional(optInSlowPeriod, core);
  lookbackTotal = lookbackSignal + lookbackSlow;
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Everything is computed in a single lockstep pass: each bar
   * advances the fast and slow EMA (two independent recursions),
   * their difference is the MACD line, and each MACD-line value
   * is immediately fed into the signal EMA. No temporary buffers.
   *
   * The arithmetic order below is the bit-exactness contract
   * (do not reorder or fuse operations):
   *  - EMA recursion: k*x + beta*prev, with ema.c's k and beta.
   *  - Each EMA is seeded with the sum of its first 'period'
   *    inputs, accumulated from 0.0 in input order, divided by
   *    the period. The signal EMA is seeded the same way from the
   *    first 'signal period' MACD-line values.
   *
   * In-place (an output == inReal) is supported: outputs at
   * [outIdx] are written only after inReal[startIdx+outIdx] was
   * read.
   */
  /* Seed each price EMA with a simple average of its first
   * 'period' price bars, each window placed by that EMA's own
   * lookback, so that the line is TA_EMA(fast) - TA_EMA(slow) bit
   * for bit. The slow EMA then runs alone to the end of the fast
   * window.
   *
   * ema_lookback(n) - n must never decrease as n grows: a fast
   * window ending before the slow one would skip bars of the fast
   * EMA, and one starting before it would read below the lookback.
   */
  today = startIdx - lookbackTotal;
  tempReal = 0.0;
  i = optInSlowPeriod;
  while (i-- > 0) {
    tempReal += inReal[today++];
  }
  prevSlow = tempReal / optInSlowPeriod;
  fastToday = startIdx - lookbackTotal + (lookbackSlow - emaLookbackPositional(optInFastPeriod, core));
  prevFast = 0.0;
  i = optInFastPeriod;
  while (i-- > 0) {
    prevFast += inReal[fastToday++];
  }
  prevFast = prevFast / optInFastPeriod;
  while (today < fastToday) {
    tempReal = inReal[today++];
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
  /* Advance both EMA through their unstable period, up to the
   * first MACD-line bar.
   */
  while (today <= startIdx - lookbackSignal) {
    tempReal = inReal[today++];
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
  }
  macdValue = prevFast - prevSlow;
  /* Seed the signal EMA with a simple average of the first
   * 'signal period' MACD-line values, accumulated as they are
   * produced.
   */
  prevSignal = 0.0;
  prevSignal += macdValue;
  i = optInSignalPeriod - 1;
  while (i-- > 0) {
    tempReal = inReal[today++];
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
    macdValue = prevFast - prevSlow;
    prevSignal += macdValue;
  }
  prevSignal = prevSignal / optInSignalPeriod;
  /* Advance everything in lockstep through the unstable period
   * of the signal EMA, up to the first output bar.
   */
  while (today <= startIdx) {
    tempReal = inReal[today++];
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
    macdValue = prevFast - prevSlow;
    if (optInSignalPeriod === 1) {
      prevSignal = macdValue;
    } else {
      {
        const _a: number = signalBeta, _b: number = prevSignal, _c: number = signalK * macdValue;
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
  }
  /* Stable zone: keep advancing in lockstep and write the three
   * outputs.
   */
  outMACD[((0 * outStride) | 0)] = macdValue;
  outMACDSignal[((0 * outStride) | 0)] = prevSignal;
  outMACDHist[((0 * outStride) | 0)] = macdValue - prevSignal;
  outIdx = 1;
  while (today <= endIdx) {
    tempReal = inReal[today++];
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
    macdValue = prevFast - prevSlow;
    if (optInSignalPeriod === 1) {
      prevSignal = macdValue;
    } else {
      {
        const _a: number = signalBeta, _b: number = prevSignal, _c: number = signalK * macdValue;
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
    outMACD[((outIdx * outStride) | 0)] = macdValue;
    outMACDSignal[((outIdx * outStride) | 0)] = prevSignal;
    outMACDHist[((outIdx * outStride) | 0)] = macdValue - prevSignal;
    outIdx += 1;
  }
  /* All done! Indicate the output limits and return success. */
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  sp.optInFastPeriod = optInFastPeriod;
  sp.optInSlowPeriod = optInSlowPeriod;
  sp.optInSignalPeriod = optInSignalPeriod;
  sp.prevFast = prevFast;
  sp.prevSlow = prevSlow;
  sp.prevSignal = prevSignal;
  sp.slowK = slowK;
  sp.fastK = fastK;
  sp.signalK = signalK;
  sp.slowBeta = slowBeta;
  sp.fastBeta = fastBeta;
  sp.signalBeta = signalBeta;
  sp.cur_outMACD = outMACD[(outNBElement.value - 1) * outStride];
  sp.cur_outMACDSignal = outMACDSignal[(outNBElement.value - 1) * outStride];
  sp.cur_outMACDHist = outMACDHist[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function macdOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outMACD: Float64Array,
  outMACDSignal: Float64Array,
  outMACDHist: Float64Array,
): MacdStream {
  const sp = MacdStream.of(core);
  const retCode = macdOpenImpl(sp, inReal, startIdx, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MACD openAndFill', inReal.length, startIdx, macdLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && macdLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core) < 0) {
    throw rejectedParams('MACD openAndFill', PARAM_RANGES, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
  }
  throw streamFailure('MACD openAndFill', retCode);
}

/** @internal */
export function macdOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
): MacdStream {
  const sp = MacdStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outMACD = new Float64Array(1);
  const sink_outMACDSignal = new Float64Array(1);
  const sink_outMACDHist = new Float64Array(1);
  const retCode = macdOpenImpl(sp, inReal, startIdx, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, sink_outMACD, sink_outMACDSignal, sink_outMACDHist, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MACD open', inReal.length, startIdx, macdLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && macdLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core) < 0) {
    throw rejectedParams('MACD open', PARAM_RANGES, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
  }
  throw streamFailure('MACD open', retCode);
}

/** @internal */
export function macdOpenPositional(
  inReal: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  core: Core = Core.DEFAULT,
): MacdStream {
  optInFastPeriod = requireInt32('MACD open', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('MACD open', 'slowPeriod', optInSlowPeriod);
  optInSignalPeriod = requireInt32('MACD open', 'signalPeriod', optInSignalPeriod);
  requireArgument('MACD open', 'real', inReal);
  requireHistory('MACD open', inReal.length);
  return macdOpenInternal(core, float64(inReal), 0, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
}

/** @internal */
export function macdOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  outMACD: Float64Array,
  outMACDSignal: Float64Array,
  outMACDHist: Float64Array,
  core: Core = Core.DEFAULT,
): MacdStream {
  optInFastPeriod = requireInt32('MACD openAndFill', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('MACD openAndFill', 'slowPeriod', optInSlowPeriod);
  optInSignalPeriod = requireInt32('MACD openAndFill', 'signalPeriod', optInSignalPeriod);
  requireArgument('MACD openAndFill', 'real', inReal);
  requireHistory('MACD openAndFill', inReal.length);
  const guardLookback = macdLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('MACD openAndFill', PARAM_RANGES, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
  }
  const guardOutLen = openFillCount('MACD openAndFill', inReal.length, guardLookback);
  requireOutput('MACD openAndFill', 'outMacd', outMACD, guardOutLen, Float64Array);
  requireOutput('MACD openAndFill', 'outMacdSignal', outMACDSignal, guardOutLen, Float64Array);
  requireOutput('MACD openAndFill', 'outMacdHist', outMACDHist, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outMACD.buffer === inRealF64.buffer && outMACD.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('MACD openAndFill', 'outMacd', 'real');
  }
  if (outMACDSignal.buffer === inRealF64.buffer && outMACDSignal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('MACD openAndFill', 'outMacdSignal', 'real');
  }
  if (outMACDHist.buffer === inRealF64.buffer && outMACDHist.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('MACD openAndFill', 'outMacdHist', 'real');
  }
  if (outMACD.buffer === outMACDSignal.buffer && outMACD.byteOffset === outMACDSignal.byteOffset) {
    throw sharedMemory('MACD openAndFill', 'outMacd', 'outMacdSignal');
  }
  if (outMACD.buffer === outMACDHist.buffer && outMACD.byteOffset === outMACDHist.byteOffset) {
    throw sharedMemory('MACD openAndFill', 'outMacd', 'outMacdHist');
  }
  if (outMACDSignal.buffer === outMACDHist.buffer && outMACDSignal.byteOffset === outMACDHist.byteOffset) {
    throw sharedMemory('MACD openAndFill', 'outMacdSignal', 'outMacdHist');
  }
  if (guardOutLen > 0 && overlaps(outMACD, inRealF64)) {
    throw sharedMemory('MACD openAndFill', 'outMacd', 'real');
  }
  if (guardOutLen > 0 && overlaps(outMACDSignal, inRealF64)) {
    throw sharedMemory('MACD openAndFill', 'outMacdSignal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outMACDHist, inRealF64)) {
    throw sharedMemory('MACD openAndFill', 'outMacdHist', 'real');
  }
  if (guardOutLen > 0 && overlaps(outMACD, outMACDSignal)) {
    throw sharedMemory('MACD openAndFill', 'outMacd', 'outMacdSignal');
  }
  if (guardOutLen > 0 && overlaps(outMACD, outMACDHist)) {
    throw sharedMemory('MACD openAndFill', 'outMacd', 'outMacdHist');
  }
  if (guardOutLen > 0 && overlaps(outMACDSignal, outMACDHist)) {
    throw sharedMemory('MACD openAndFill', 'outMacdSignal', 'outMacdHist');
  }
  return macdOpenAndFillInternal(core, inRealF64, 0, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, new MInteger(), new MInteger(), outMACD, outMACDSignal, outMACDHist);
}

/**
 * Opens a live MACD stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link macd} there.
 *
 * @param real - Warm-up history. Input series (typically close)
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MacdOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `macdLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { macdOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = macdOpen(data, {
 *   fastPeriod: 12,
 *   slowPeriod: 26,
 *   signalPeriod: 9,
 * });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function macdOpen(real: ArrayLike<number>, options?: MacdOptions): MacdStream {
  const o = optionsOf('MACD open', options);
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MACD open', o.core);
  return macdOpenPositional(real, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core);
}

/**
 * {@link macdOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link macd}.
 *
 * @param real - Warm-up history. Input series (typically close)
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link MacdOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link macd} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `macdLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or a
 *   buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { macdOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const {
 *   stream,
 *   begIdx,
 *   outMacd,
 *   outMacdSignal,
 *   outMacdHist,
 * } = macdOpenAndFill(data, { fastPeriod: 12, slowPeriod: 26, signalPeriod: 9 });
 * // outMacd[k] is the value at bar begIdx + k
 * ```
 */
export function macdOpenAndFill(real: ArrayLike<number>, options?: MacdOptions): MacdResult & { stream: MacdStream } {
  const o = optionsOf('MACD openAndFill', options);
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MACD openAndFill', o.core);
  const capacity = fillCapacity(macdLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core), real);
  const outMACD = o.outMacd === undefined ? new Float64Array(capacity) : o.outMacd;
  const outMACDSignal = o.outMacdSignal === undefined ? new Float64Array(capacity) : o.outMacdSignal;
  const outMACDHist = o.outMacdHist === undefined ? new Float64Array(capacity) : o.outMacdHist;
  const stream = macdOpenAndFillPositional(real, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outMACD, outMACDSignal, outMACDHist, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outMacd: outMACD.subarray(0, range.count), outMacdSignal: outMACDSignal.subarray(0, range.count), outMacdHist: outMACDHist.subarray(0, range.count) };
}
