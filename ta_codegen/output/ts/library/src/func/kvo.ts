/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  100726 KL,CC  Initial version (#484).
 *  101426 MF,CC  The EMA step is TA_EMA's.
 */

import { FuncUnstId } from '../enums.ts';
import { badSink, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { emaLookbackPositional } from './ema.ts';

/** @internal */
export function kvoLookbackPositional(optInFastPeriod: number, optInSlowPeriod: number, optInSignalPeriod: number, core: Core = Core.DEFAULT): number {
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
    optInFastPeriod = 34;
  } else if (optInFastPeriod < 2 || optInFastPeriod > 100000) {
    return -1;
  }
  if (optInSlowPeriod === Core.INTEGER_DEFAULT) {
    optInSlowPeriod = 55;
  } else if (optInSlowPeriod < 2 || optInSlowPeriod > 100000) {
    return -1;
  }
  if (optInSignalPeriod === Core.INTEGER_DEFAULT) {
    optInSignalPeriod = 13;
  } else if (optInSignalPeriod < 2 || optInSignalPeriod > 100000) {
    return -1;
  }
  let longestPeriod = 0;
  /* The three EMAs seed on one bar, so one EMA lookback of the longest
   * covers all three. The 1 is the reference bar before it.
   */
  longestPeriod = optInFastPeriod;
  if (optInSlowPeriod > longestPeriod) {
    longestPeriod = optInSlowPeriod;
  }
  if (optInSignalPeriod > longestPeriod) {
    longestPeriod = optInSignalPeriod;
  }
  return 1 + emaLookbackPositional(longestPeriod, core) + ((core.unstableCount(FuncUnstId.EMA, 1, 1) != core.unstableCount(FuncUnstId.EMA, 0, 0)) ? (core.unstableCount(FuncUnstId.EMA, ((((optInFastPeriod > (optInSlowPeriod > optInSignalPeriod ? optInSlowPeriod : optInSignalPeriod) ? optInFastPeriod : (optInSlowPeriod > optInSignalPeriod ? optInSlowPeriod : optInSignalPeriod)) > 1) ? ((((((((((4 * 10) | 0) + 23) / 3) | 0) * (optInFastPeriod > (optInSlowPeriod > optInSignalPeriod ? optInSlowPeriod : optInSignalPeriod) ? optInFastPeriod : (optInSlowPeriod > optInSignalPeriod ? optInSlowPeriod : optInSignalPeriod))) | 0) + 1) / 2) | 0) : 0) - (((optInFastPeriod > (optInSlowPeriod > optInSignalPeriod ? optInSlowPeriod : optInSignalPeriod) ? optInFastPeriod : (optInSlowPeriod > optInSignalPeriod ? optInSlowPeriod : optInSignalPeriod)) > 1) ? (((((10 * (optInFastPeriod > (optInSlowPeriod > optInSignalPeriod ? optInSlowPeriod : optInSignalPeriod) ? optInFastPeriod : (optInSlowPeriod > optInSignalPeriod ? optInSlowPeriod : optInSignalPeriod))) | 0) + 1) / 2) | 0) : 0)), ((((optInFastPeriod > (optInSlowPeriod > optInSignalPeriod ? optInSlowPeriod : optInSignalPeriod) ? optInFastPeriod : (optInSlowPeriod > optInSignalPeriod ? optInSlowPeriod : optInSignalPeriod)) > 1) ? ((((((((((4 * 19) | 0) + 23) / 3) | 0) * (optInFastPeriod > (optInSlowPeriod > optInSignalPeriod ? optInSlowPeriod : optInSignalPeriod) ? optInFastPeriod : (optInSlowPeriod > optInSignalPeriod ? optInSlowPeriod : optInSignalPeriod))) | 0) + 1) / 2) | 0) : 0) - (((optInFastPeriod > (optInSlowPeriod > optInSignalPeriod ? optInSlowPeriod : optInSignalPeriod) ? optInFastPeriod : (optInSlowPeriod > optInSignalPeriod ? optInSlowPeriod : optInSignalPeriod)) > 1) ? (((((19 * (optInFastPeriod > (optInSlowPeriod > optInSignalPeriod ? optInSlowPeriod : optInSignalPeriod) ? optInFastPeriod : (optInSlowPeriod > optInSignalPeriod ? optInSlowPeriod : optInSignalPeriod))) | 0) + 1) / 2) | 0) : 0))) - core.unstableCount(FuncUnstId.EMA, 0, 0)) : 0);
}

/** @internal */
export function kvoDisplayShiftPositional(optInFastPeriod: number, optInSlowPeriod: number, optInSignalPeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (kvoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 2) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function kvoImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  inVolume: Float64Array,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outKVO: Float64Array,
  outKVOSignal: Float64Array,
): RetCode {
  let kFast = 0;
  let kSlow = 0;
  let kSignal = 0;
  let betaFast = 0;
  let betaSlow = 0;
  let betaSignal = 0;
  let hlc = 0;
  let prevHlc = 0;
  let dmToday = 0;
  let prevDm = 0;
  let emaFast = 0;
  let emaSlow = 0;
  let cm = 0;
  let vf = 0;
  let trend = 0;
  let prevTrend = 0;
  let kvoValue = 0;
  let signalValue = 0;
  let lookbackTotal = 0;
  let today = 0;
  let outIdx = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInFastPeriod === Core.INTEGER_DEFAULT) {
    optInFastPeriod = 34;
  } else if (optInFastPeriod < 2 || optInFastPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowPeriod === Core.INTEGER_DEFAULT) {
    optInSlowPeriod = 55;
  } else if (optInSlowPeriod < 2 || optInSlowPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSignalPeriod === Core.INTEGER_DEFAULT) {
    optInSignalPeriod = 13;
  } else if (optInSignalPeriod < 2 || optInSignalPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if ((outKVO.buffer === outKVOSignal.buffer && outKVO.byteOffset === outKVOSignal.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  /* The C stream struct follows this order, and a bad one can stall every
   * streamed update without changing a value. prevHlc/prevDm and
   * emaFast/emaSlow are each stored as one 16-byte pair and must sit 16-byte
   * aligned in the struct; prevTrend keeps cm and signalValue apart so no
   * load spans two stores.
   */
  /* Stephen J. Klinger, "Identifying Trends With Volume Analysis", Technical
   * Analysis of Stocks & Commodities V15:12 (December 1997), with the 1997
   * factor |2*(dm/cm) - 1| (#484, form A).
   *
   *    trend = +1 if H+L+C rose, -1 if it fell, unchanged on a tie
   *    cm    = previous + current range on a trend change, else cm + range
   *    vf    = volume * |2*(dm/cm) - 1| * 100 * trend
   *
   * The committed corpus holds a single tie in H+L+C, and reading it any other
   * way moves KVO by up to 1e7 from the next bar on: the tie rule is gated by
   * a built series in test_kvo.c, not by the corpus.
   *
   * All three EMAs seed raw on the first volume force, not on an SMA as
   * TA_EMA does. Each k + beta pair must sum to exactly 1.0, as in ema.c.
   */
  lookbackTotal = kvoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core);
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
  betaFast = (optInFastPeriod - 1) / (optInFastPeriod + 1);
  kFast = 1.0 - betaFast;
  if (betaFast < 0.5) {
    betaFast = 1.0 - kFast;
  }
  betaSlow = (optInSlowPeriod - 1) / (optInSlowPeriod + 1);
  kSlow = 1.0 - betaSlow;
  if (betaSlow < 0.5) {
    betaSlow = 1.0 - kSlow;
  }
  betaSignal = (optInSignalPeriod - 1) / (optInSignalPeriod + 1);
  kSignal = 1.0 - betaSignal;
  if (betaSignal < 0.5) {
    betaSignal = 1.0 - kSignal;
  }
  /* The anchor: only its H+L+C and range are read, and the trend starts at +1. */
  today = startIdx - lookbackTotal;
  prevHlc = inHigh[today] + inLow[today] + inClose[today];
  prevDm = inHigh[today] - inLow[today];
  prevTrend = 1.0;
  today = today + 1;
  /* The seed bar: cm opens on both ranges whatever the trend, and both
   * averages start at this volume force, so KVO and the trigger are exactly 0.
   */
  hlc = inHigh[today] + inLow[today] + inClose[today];
  dmToday = inHigh[today] - inLow[today];
  if (hlc > prevHlc) {
    trend = 1.0;
  } else if (hlc < prevHlc) {
    trend = -1.0;
  } else {
    trend = prevTrend;
  }
  cm = prevDm + dmToday;
  if (cm === 0.0) {
    vf = 0.0;
  } else {
    vf = inVolume[today] * Math.abs(2.0 * (dmToday / cm) - 1.0) * 100.0 * trend;
  }
  emaFast = vf;
  emaSlow = vf;
  kvoValue = emaFast - emaSlow;
  signalValue = kvoValue;
  prevHlc = hlc;
  prevDm = dmToday;
  prevTrend = trend;
  today = today + 1;
  while (today < startIdx) {
    hlc = inHigh[today] + inLow[today] + inClose[today];
    dmToday = inHigh[today] - inLow[today];
    if (hlc > prevHlc) {
      trend = 1.0;
    } else if (hlc < prevHlc) {
      trend = -1.0;
    } else {
      trend = prevTrend;
    }
    /* A branch, not a select: LLVM turns a select between the two stored
     * ranges into a select of their address, which serializes each streamed
     * update behind the previous one on ARM64.
     */
    if (trend !== prevTrend) {
      cm = prevDm + dmToday;
    } else {
      cm = cm + dmToday;
    }
    if (cm === 0.0) {
      vf = 0.0;
    } else {
      vf = inVolume[today] * Math.abs(2.0 * (dmToday / cm) - 1.0) * 100.0 * trend;
    }
    {
      const _a: number = betaFast, _b: number = emaFast, _c: number = kFast * vf;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaFast = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = betaSlow, _b: number = emaSlow, _c: number = kSlow * vf;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaSlow = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    kvoValue = emaFast - emaSlow;
    {
      const _a: number = betaSignal, _b: number = signalValue, _c: number = kSignal * kvoValue;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      signalValue = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    prevHlc = hlc;
    prevDm = dmToday;
    prevTrend = trend;
    today = today + 1;
  }
  outIdx = 0;
  while (today <= endIdx) {
    hlc = inHigh[today] + inLow[today] + inClose[today];
    dmToday = inHigh[today] - inLow[today];
    if (hlc > prevHlc) {
      trend = 1.0;
    } else if (hlc < prevHlc) {
      trend = -1.0;
    } else {
      trend = prevTrend;
    }
    if (trend !== prevTrend) {
      cm = prevDm + dmToday;
    } else {
      cm = cm + dmToday;
    }
    if (cm === 0.0) {
      vf = 0.0;
    } else {
      vf = inVolume[today] * Math.abs(2.0 * (dmToday / cm) - 1.0) * 100.0 * trend;
    }
    {
      const _a: number = betaFast, _b: number = emaFast, _c: number = kFast * vf;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaFast = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = betaSlow, _b: number = emaSlow, _c: number = kSlow * vf;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaSlow = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    kvoValue = emaFast - emaSlow;
    {
      const _a: number = betaSignal, _b: number = signalValue, _c: number = kSignal * kvoValue;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      signalValue = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    outKVO[outIdx] = kvoValue;
    outKVOSignal[outIdx] = signalValue;
    outIdx = outIdx + 1;
    prevHlc = hlc;
    prevDm = dmToday;
    prevTrend = trend;
    today = today + 1;
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['fastPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['slowPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['signalPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000']] as const;

/** @internal */
export function kvoPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  outKVO: Float64Array,
  outKVOSignal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('KVO', 'startIdx', startIdx);
  endIdx = requireInt32('KVO', 'endIdx', endIdx);
  optInFastPeriod = requireInt32('KVO', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('KVO', 'slowPeriod', optInSlowPeriod);
  optInSignalPeriod = requireInt32('KVO', 'signalPeriod', optInSignalPeriod);
  requireIndexRange('KVO', startIdx, endIdx);
  const guardLookback = kvoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('KVO', PARAM_RANGES, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
  }
  const guardStart = clampedStart('KVO', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('KVO', 'prices.high', inHigh, guardInLen);
  requireLength('KVO', 'prices.low', inLow, guardInLen);
  requireLength('KVO', 'prices.close', inClose, guardInLen);
  requireLength('KVO', 'prices.volume', inVolume, guardInLen);
  requireOutput('KVO', 'outKvo', outKVO, guardOutLen, Float64Array);
  requireOutput('KVO', 'outKvoSignal', outKVOSignal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  const inVolumeF64 = float64(inVolume);
  if (overlapsPartly(outKVO, inHighF64)) {
    throw sharedMemory('KVO', 'outKvo', 'prices.high');
  }
  if (overlapsPartly(outKVO, inLowF64)) {
    throw sharedMemory('KVO', 'outKvo', 'prices.low');
  }
  if (overlapsPartly(outKVO, inCloseF64)) {
    throw sharedMemory('KVO', 'outKvo', 'prices.close');
  }
  if (overlapsPartly(outKVO, inVolumeF64)) {
    throw sharedMemory('KVO', 'outKvo', 'prices.volume');
  }
  if (overlapsPartly(outKVOSignal, inHighF64)) {
    throw sharedMemory('KVO', 'outKvoSignal', 'prices.high');
  }
  if (overlapsPartly(outKVOSignal, inLowF64)) {
    throw sharedMemory('KVO', 'outKvoSignal', 'prices.low');
  }
  if (overlapsPartly(outKVOSignal, inCloseF64)) {
    throw sharedMemory('KVO', 'outKvoSignal', 'prices.close');
  }
  if (overlapsPartly(outKVOSignal, inVolumeF64)) {
    throw sharedMemory('KVO', 'outKvoSignal', 'prices.volume');
  }
  if (overlaps(outKVO, outKVOSignal)) {
    throw sharedMemory('KVO', 'outKvo', 'outKvoSignal');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = kvoImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, inVolumeF64, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outKVO, outKVOSignal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('KVO', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link kvo}, {@link kvoLookback}, {@link kvoDisplayShift},
 * {@link kvoOpen} and {@link kvoOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface KvoOptions {
  /**
   * Period of the faster smoothing of the volume force (default 34; range 2 to
   * 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  fastPeriod?: number | undefined;
  /**
   * Period of the slower smoothing of the volume force (default 55; range 2 to
   * 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  slowPeriod?: number | undefined;
  /**
   * Smoothing period of the trigger line (default 13; range 2 to 100000;
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
   * Buffer for `outKvo`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outKvo?: Float64Array<ArrayBuffer> | undefined;
  /**
   * Buffer for `outKvoSignal`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outKvoSignal?: Float64Array<ArrayBuffer> | undefined;
}

/** What {@link kvo} returns, and {@link kvoOpenAndFill} with its stream. */
export interface KvoResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /** Klinger Volume Oscillator. `outKvo[k]` is the value at input bar `begIdx + k`. */
  outKvo: Float64Array<ArrayBuffer>;
  /**
   * Trigger line, an exponential average of the oscillator. `outKvoSignal[k]` is the
   * value at input bar `begIdx + k`.
   */
  outKvoSignal: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link kvo} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link KvoOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function kvoLookback(options?: KvoOptions): number {
  const o = optionsOf('KVO lookback', options);
  return kvoLookbackPositional(orDefault(o.fastPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowPeriod, Core.INTEGER_DEFAULT), orDefault(o.signalPeriod, Core.INTEGER_DEFAULT), coreOf('KVO lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link kvo}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link KvoOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function kvoDisplayShift(outputIdx: number, options?: KvoOptions): number {
  const o = optionsOf('KVO displayShift', options);
  return kvoDisplayShiftPositional(orDefault(o.fastPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowPeriod, Core.INTEGER_DEFAULT), orDefault(o.signalPeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('KVO displayShift', o.core));
}

/**
 * Klinger Volume Oscillator: each bar's volume is signed by the direction of its
 * high-low-close sum and weighted by where the bar's own range sits inside the
 * cumulative range of the current trend run, then the resulting volume force is
 * smoothed twice and differenced. Klinger's reading is that volume alone says how
 * much trading happened but not what it accomplished, so he scales it by the share
 * of the trend's accumulated range that this bar actually covered. The oscillator
 * crosses zero as the balance of that force changes hands, and crosses its own
 * trigger line earlier.
 *
 * Formula and more info at
 * [ta-lib.org/functions/kvo](https://ta-lib.org/functions/kvo).
 *
 * **Notes**
 * - The trend is **held** when the high-low-close sum repeats, which is the
 *   article's own tenet: the existing trend is maintained on equality. A reading
 *   that breaks the tie in either direction changes every later bar, because the
 *   cumulative range is reset by a trend change.
 * - The cumulative range is reset by every trend change, so two calls starting at
 *   different bars carry different range state only until the first trend change
 *   after both; what remains between them is the seed error of the three averages,
 *   which fades at the rate of the longest period. KVO converges, but as a small
 *   difference of two large averages it needs more bars than one of them would.
 * - All three exponential averages are seeded from the volume force of the same
 *   anchor bar rather than from a simple average of their own first inputs, so one
 *   warm-up covers all three. KVO is exactly zero on that seed bar, which falls
 *   inside the warm-up and is never output.
 *   `CoreBuilder.unstablePeriod(FuncUnstId.EMA, ...)` discards more of that
 *   warm-up.
 * - The weight is the 1997 form, `|2*(dm/cm) - 1|`, which is what the December
 *   1997 Traders' Tips print. Some later implementations use `|2*(dm/cm - 1)|`,
 *   which is a different series.
 * - A cumulative range of exactly zero leaves the volume force at zero rather than
 *   dividing.
 *
 * `outKvo[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link kvoLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `high`: High price
 *   series. `low`: Low price series. `close`: Close price series. `volume`: Volume
 *   series.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link KvoOptions}.
 * @returns `begIdx` and the values; see {@link KvoResult}. Each output is a view
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
 * import { kvo } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const { begIdx, outKvo, outKvoSignal } = kvo({ high, low, close, volume }, {
 *   fastPeriod: 34,
 *   slowPeriod: 55,
 *   signalPeriod: 13,
 * });
 * // outKvo[k] is the value at bar begIdx + k
 * ```
 * @see [`adosc`](https://ta-lib.org/functions/adosc)
 * @see [`obv`](https://ta-lib.org/functions/obv)
 * @see [`mfi`](https://ta-lib.org/functions/mfi)
 * @see [`ad`](https://ta-lib.org/functions/ad)
 */
export function kvo(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume: ArrayLike<number>; openInterest?: ArrayLike<number> | undefined }, options?: KvoOptions): KvoResult {
  requireArgument('KVO', 'prices', prices);
  const o = optionsOf('KVO', options);
  if (o.endIdx === undefined) {
    requireSeries('KVO', 'prices.high', prices.high);
    requireSeries('KVO', 'prices.low', prices.low);
    requireSeries('KVO', 'prices.close', prices.close);
    requireSeries('KVO', 'prices.volume', prices.volume);
  }
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('KVO', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, kvoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core), prices.high, prices.low, prices.close, prices.volume);
  const outKVO = o.outKvo === undefined ? new Float64Array(capacity) : o.outKvo;
  const outKVOSignal = o.outKvoSignal === undefined ? new Float64Array(capacity) : o.outKvoSignal;
  const range = kvoPositional(startIdx, endIdx, prices.high, prices.low, prices.close, prices.volume, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outKVO, outKVOSignal, core);
  return { begIdx: range.begIdx, outKvo: outKVO.subarray(0, range.count), outKvoSignal: outKVOSignal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live KVO stream: one value per closed bar, bit-identical to {@link kvo} over
 * the same series. Open it with {@link kvoOpen} or {@link kvoOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/kvo](https://ta-lib.org/functions/kvo).
 */
export class KvoStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInFastPeriod: number = 0;
  /** @internal */
  optInSlowPeriod: number = 0;
  /** @internal */
  optInSignalPeriod: number = 0;
  /** @internal */
  kFast: number = 0;
  /** @internal */
  kSlow: number = 0;
  /** @internal */
  kSignal: number = 0;
  /** @internal */
  betaFast: number = 0;
  /** @internal */
  betaSlow: number = 0;
  /** @internal */
  betaSignal: number = 0;
  /** @internal */
  prevHlc: number = 0;
  /** @internal */
  prevDm: number = 0;
  /** @internal */
  emaFast: number = 0;
  /** @internal */
  emaSlow: number = 0;
  /** @internal */
  cm: number = 0;
  /** @internal */
  prevTrend: number = 0;
  /** @internal */
  signalValue: number = 0;
  /** @internal */
  cur_outKVO: number = 0;
  /** @internal */
  cur_outKVOSignal: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): KvoStream {
    return new KvoStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link kvo} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('KVO advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns its outputs.
   *
   * @param high - The bar's `high` value: High price series.
   * @param low - The bar's `low` value: Low price series.
   * @param close - The bar's `close` value: Close price series.
   * @param volume - The bar's `volume` value: Volume series.
   * @param out - Receives the outputs and is returned; a new {@link KvoOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included, before anything moves.
   *   `TALibError` with code `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached
   *   bar `Core.INDEX_MAX`.
   */
  update(high: number, low: number, close: number, volume: number, out?: KvoOut): KvoOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('KVO update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('KVO update', 'KvoOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close) || !Number.isFinite(volume)) {
      throw nonFiniteBar('KVO update', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : !Number.isFinite(close) ? 'close' : 'volume');
    }
    kvoStepImpl(this, high, low, close, volume);
    this.outRangeCount++;
    if (out === undefined) {
      out = new KvoOut();
    }
    out.outKvo = this.cur_outKVO;
    out.outKvoSignal = this.cur_outKVOSignal;
    return out;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param high - The bar's `high` value: High price series.
   * @param low - The bar's `low` value: Low price series.
   * @param close - The bar's `close` value: Close price series.
   * @param volume - The bar's `volume` value: Volume series.
   * @param out - Receives the outputs and is returned; a new {@link KvoOut} when
   *   omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included.
   */
  peek(high: number, low: number, close: number, volume: number, out?: KvoOut): KvoOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('KVO peek', 'KvoOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close) || !Number.isFinite(volume)) {
      throw nonFiniteBar('KVO peek', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : !Number.isFinite(close) ? 'close' : 'volume');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
    const inClose = close;
    const inVolume = volume;
    let hlc = 0;
    let dmToday = 0;
    let vf = 0;
    let trend = 0;
    let kvoValue = 0;
    let cm = sp.cm;
    let cur_outKVO = 0;
    let cur_outKVOSignal = 0;
    let emaFast = sp.emaFast;
    let emaSlow = sp.emaSlow;
    let signalValue = sp.signalValue;
    hlc = inHigh + inLow + inClose;
    dmToday = inHigh - inLow;
    if (hlc > sp.prevHlc) {
      trend = 1.0;
    } else if (hlc < sp.prevHlc) {
      trend = -1.0;
    } else {
      trend = sp.prevTrend;
    }
    if (trend !== sp.prevTrend) {
      cm = sp.prevDm + dmToday;
    } else {
      cm = cm + dmToday;
    }
    if (cm === 0.0) {
      vf = 0.0;
    } else {
      vf = inVolume * Math.abs(2.0 * (dmToday / cm) - 1.0) * 100.0 * trend;
    }
    {
      const _a: number = sp.betaFast, _b: number = emaFast, _c: number = sp.kFast * vf;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaFast = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = sp.betaSlow, _b: number = emaSlow, _c: number = sp.kSlow * vf;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaSlow = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    kvoValue = emaFast - emaSlow;
    {
      const _a: number = sp.betaSignal, _b: number = signalValue, _c: number = sp.kSignal * kvoValue;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      signalValue = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    cur_outKVO = kvoValue;
    cur_outKVOSignal = signalValue;
    if (out === undefined) {
      out = new KvoOut();
    }
    out.outKvo = cur_outKVO;
    out.outKvoSignal = cur_outKVOSignal;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link KvoOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a sink that is not an object,
   *   `null` included.
   */
  value(out?: KvoOut): KvoOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('KVO value', 'KvoOut');
    }
    if (out === undefined) {
      out = new KvoOut();
    }
    out.outKvo = this.cur_outKVO;
    out.outKvoSignal = this.cur_outKVOSignal;
    return out;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): KvoStream {
    const c = new KvoStream(this.core);
    c.optInFastPeriod = this.optInFastPeriod;
    c.optInSlowPeriod = this.optInSlowPeriod;
    c.optInSignalPeriod = this.optInSignalPeriod;
    c.kFast = this.kFast;
    c.kSlow = this.kSlow;
    c.kSignal = this.kSignal;
    c.betaFast = this.betaFast;
    c.betaSlow = this.betaSlow;
    c.betaSignal = this.betaSignal;
    c.prevHlc = this.prevHlc;
    c.prevDm = this.prevDm;
    c.emaFast = this.emaFast;
    c.emaSlow = this.emaSlow;
    c.cm = this.cm;
    c.prevTrend = this.prevTrend;
    c.signalValue = this.signalValue;
    c.cur_outKVO = this.cur_outKVO;
    c.cur_outKVOSignal = this.cur_outKVOSignal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one KVO bar, as `update`, `peek` and `value` return them. Passed
 * back to one of them, it is filled and returned again, with no allocation; it
 * holds its values until the next call that writes it. A new one holds `NaN`.
 */
export class KvoOut {
  /** Klinger Volume Oscillator. */
  outKvo: number = NaN;
  /** Trigger line, an exponential average of the oscillator. */
  outKvoSignal: number = NaN;
}

function kvoStepImpl(sp: KvoStream, inHigh: number, inLow: number, inClose: number, inVolume: number): void {
  let hlc = 0;
  let dmToday = 0;
  let vf = 0;
  let trend = 0;
  let kvoValue = 0;
  hlc = inHigh + inLow + inClose;
  dmToday = inHigh - inLow;
  if (hlc > sp.prevHlc) {
    trend = 1.0;
  } else if (hlc < sp.prevHlc) {
    trend = -1.0;
  } else {
    trend = sp.prevTrend;
  }
  if (trend !== sp.prevTrend) {
    sp.cm = sp.prevDm + dmToday;
  } else {
    sp.cm = sp.cm + dmToday;
  }
  if (sp.cm === 0.0) {
    vf = 0.0;
  } else {
    vf = inVolume * Math.abs(2.0 * (dmToday / sp.cm) - 1.0) * 100.0 * trend;
  }
  {
    const _a: number = sp.betaFast, _b: number = sp.emaFast, _c: number = sp.kFast * vf;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.emaFast = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  {
    const _a: number = sp.betaSlow, _b: number = sp.emaSlow, _c: number = sp.kSlow * vf;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.emaSlow = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  kvoValue = sp.emaFast - sp.emaSlow;
  {
    const _a: number = sp.betaSignal, _b: number = sp.signalValue, _c: number = sp.kSignal * kvoValue;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.signalValue = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  sp.cur_outKVO = kvoValue;
  sp.cur_outKVOSignal = sp.signalValue;
  sp.prevHlc = hlc;
  sp.prevDm = dmToday;
  sp.prevTrend = trend;
}

function kvoOpenImpl(
  sp: KvoStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outKVO: Float64Array,
  outKVOSignal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let kFast = 0;
  let kSlow = 0;
  let kSignal = 0;
  let betaFast = 0;
  let betaSlow = 0;
  let betaSignal = 0;
  let hlc = 0;
  let prevHlc = 0;
  let dmToday = 0;
  let prevDm = 0;
  let emaFast = 0;
  let emaSlow = 0;
  let cm = 0;
  let vf = 0;
  let trend = 0;
  let prevTrend = 0;
  let kvoValue = 0;
  let signalValue = 0;
  let lookbackTotal = 0;
  let today = 0;
  let outIdx = 0;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length || inClose.length !== inHigh.length || inVolume.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInFastPeriod === Core.INTEGER_DEFAULT) {
    optInFastPeriod = 34;
  } else if (optInFastPeriod < 2 || optInFastPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowPeriod === Core.INTEGER_DEFAULT) {
    optInSlowPeriod = 55;
  } else if (optInSlowPeriod < 2 || optInSlowPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSignalPeriod === Core.INTEGER_DEFAULT) {
    optInSignalPeriod = 13;
  } else if (optInSignalPeriod < 2 || optInSignalPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* The C stream struct follows this order, and a bad one can stall every
   * streamed update without changing a value. prevHlc/prevDm and
   * emaFast/emaSlow are each stored as one 16-byte pair and must sit 16-byte
   * aligned in the struct; prevTrend keeps cm and signalValue apart so no
   * load spans two stores.
   */
  /* Stephen J. Klinger, "Identifying Trends With Volume Analysis", Technical
   * Analysis of Stocks & Commodities V15:12 (December 1997), with the 1997
   * factor |2*(dm/cm) - 1| (#484, form A).
   *
   *    trend = +1 if H+L+C rose, -1 if it fell, unchanged on a tie
   *    cm    = previous + current range on a trend change, else cm + range
   *    vf    = volume * |2*(dm/cm) - 1| * 100 * trend
   *
   * The committed corpus holds a single tie in H+L+C, and reading it any other
   * way moves KVO by up to 1e7 from the next bar on: the tie rule is gated by
   * a built series in test_kvo.c, not by the corpus.
   *
   * All three EMAs seed raw on the first volume force, not on an SMA as
   * TA_EMA does. Each k + beta pair must sum to exactly 1.0, as in ema.c.
   */
  lookbackTotal = kvoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core);
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
  betaFast = (optInFastPeriod - 1) / (optInFastPeriod + 1);
  kFast = 1.0 - betaFast;
  if (betaFast < 0.5) {
    betaFast = 1.0 - kFast;
  }
  betaSlow = (optInSlowPeriod - 1) / (optInSlowPeriod + 1);
  kSlow = 1.0 - betaSlow;
  if (betaSlow < 0.5) {
    betaSlow = 1.0 - kSlow;
  }
  betaSignal = (optInSignalPeriod - 1) / (optInSignalPeriod + 1);
  kSignal = 1.0 - betaSignal;
  if (betaSignal < 0.5) {
    betaSignal = 1.0 - kSignal;
  }
  /* The anchor: only its H+L+C and range are read, and the trend starts at +1. */
  today = startIdx - lookbackTotal;
  prevHlc = inHigh[today] + inLow[today] + inClose[today];
  prevDm = inHigh[today] - inLow[today];
  prevTrend = 1.0;
  today = today + 1;
  /* The seed bar: cm opens on both ranges whatever the trend, and both
   * averages start at this volume force, so KVO and the trigger are exactly 0.
   */
  hlc = inHigh[today] + inLow[today] + inClose[today];
  dmToday = inHigh[today] - inLow[today];
  if (hlc > prevHlc) {
    trend = 1.0;
  } else if (hlc < prevHlc) {
    trend = -1.0;
  } else {
    trend = prevTrend;
  }
  cm = prevDm + dmToday;
  if (cm === 0.0) {
    vf = 0.0;
  } else {
    vf = inVolume[today] * Math.abs(2.0 * (dmToday / cm) - 1.0) * 100.0 * trend;
  }
  emaFast = vf;
  emaSlow = vf;
  kvoValue = emaFast - emaSlow;
  signalValue = kvoValue;
  prevHlc = hlc;
  prevDm = dmToday;
  prevTrend = trend;
  today = today + 1;
  while (today < startIdx) {
    hlc = inHigh[today] + inLow[today] + inClose[today];
    dmToday = inHigh[today] - inLow[today];
    if (hlc > prevHlc) {
      trend = 1.0;
    } else if (hlc < prevHlc) {
      trend = -1.0;
    } else {
      trend = prevTrend;
    }
    /* A branch, not a select: LLVM turns a select between the two stored
     * ranges into a select of their address, which serializes each streamed
     * update behind the previous one on ARM64.
     */
    if (trend !== prevTrend) {
      cm = prevDm + dmToday;
    } else {
      cm = cm + dmToday;
    }
    if (cm === 0.0) {
      vf = 0.0;
    } else {
      vf = inVolume[today] * Math.abs(2.0 * (dmToday / cm) - 1.0) * 100.0 * trend;
    }
    {
      const _a: number = betaFast, _b: number = emaFast, _c: number = kFast * vf;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaFast = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = betaSlow, _b: number = emaSlow, _c: number = kSlow * vf;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaSlow = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    kvoValue = emaFast - emaSlow;
    {
      const _a: number = betaSignal, _b: number = signalValue, _c: number = kSignal * kvoValue;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      signalValue = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    prevHlc = hlc;
    prevDm = dmToday;
    prevTrend = trend;
    today = today + 1;
  }
  outIdx = 0;
  while (today <= endIdx) {
    hlc = inHigh[today] + inLow[today] + inClose[today];
    dmToday = inHigh[today] - inLow[today];
    if (hlc > prevHlc) {
      trend = 1.0;
    } else if (hlc < prevHlc) {
      trend = -1.0;
    } else {
      trend = prevTrend;
    }
    if (trend !== prevTrend) {
      cm = prevDm + dmToday;
    } else {
      cm = cm + dmToday;
    }
    if (cm === 0.0) {
      vf = 0.0;
    } else {
      vf = inVolume[today] * Math.abs(2.0 * (dmToday / cm) - 1.0) * 100.0 * trend;
    }
    {
      const _a: number = betaFast, _b: number = emaFast, _c: number = kFast * vf;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaFast = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = betaSlow, _b: number = emaSlow, _c: number = kSlow * vf;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaSlow = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    kvoValue = emaFast - emaSlow;
    {
      const _a: number = betaSignal, _b: number = signalValue, _c: number = kSignal * kvoValue;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      signalValue = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    outKVO[((outIdx * outStride) | 0)] = kvoValue;
    outKVOSignal[((outIdx * outStride) | 0)] = signalValue;
    outIdx = outIdx + 1;
    prevHlc = hlc;
    prevDm = dmToday;
    prevTrend = trend;
    today = today + 1;
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  sp.optInFastPeriod = optInFastPeriod;
  sp.optInSlowPeriod = optInSlowPeriod;
  sp.optInSignalPeriod = optInSignalPeriod;
  sp.kFast = kFast;
  sp.kSlow = kSlow;
  sp.kSignal = kSignal;
  sp.betaFast = betaFast;
  sp.betaSlow = betaSlow;
  sp.betaSignal = betaSignal;
  sp.prevHlc = prevHlc;
  sp.prevDm = prevDm;
  sp.emaFast = emaFast;
  sp.emaSlow = emaSlow;
  sp.cm = cm;
  sp.prevTrend = prevTrend;
  sp.signalValue = signalValue;
  sp.cur_outKVO = outKVO[(outNBElement.value - 1) * outStride];
  sp.cur_outKVOSignal = outKVOSignal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function kvoOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outKVO: Float64Array,
  outKVOSignal: Float64Array,
): KvoStream {
  const sp = KvoStream.of(core);
  const retCode = kvoOpenImpl(sp, inHigh, inLow, inClose, inVolume, startIdx, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outKVO, outKVOSignal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('KVO openAndFill', inHigh.length, startIdx, kvoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && kvoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core) < 0) {
    throw rejectedParams('KVO openAndFill', PARAM_RANGES, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
  }
  throw streamFailure('KVO openAndFill', retCode);
}

/** @internal */
export function kvoOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
): KvoStream {
  const sp = KvoStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outKVO = new Float64Array(1);
  const sink_outKVOSignal = new Float64Array(1);
  const retCode = kvoOpenImpl(sp, inHigh, inLow, inClose, inVolume, startIdx, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, sink_outKVO, sink_outKVOSignal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('KVO open', inHigh.length, startIdx, kvoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && kvoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core) < 0) {
    throw rejectedParams('KVO open', PARAM_RANGES, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
  }
  throw streamFailure('KVO open', retCode);
}

/** @internal */
export function kvoOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  core: Core = Core.DEFAULT,
): KvoStream {
  optInFastPeriod = requireInt32('KVO open', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('KVO open', 'slowPeriod', optInSlowPeriod);
  optInSignalPeriod = requireInt32('KVO open', 'signalPeriod', optInSignalPeriod);
  requireArgument('KVO open', 'prices.high', inHigh);
  requireHistory('KVO open', 'prices.high', inHigh.length);
  requireArgument('KVO open', 'prices.low', inLow);
  requireArgument('KVO open', 'prices.close', inClose);
  requireArgument('KVO open', 'prices.volume', inVolume);
  requireHistoryLength('KVO open', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('KVO open', 'prices.close', inClose.length, inHigh.length);
  requireHistoryLength('KVO open', 'prices.volume', inVolume.length, inHigh.length);
  return kvoOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), float64(inVolume), 0, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
}

/** @internal */
export function kvoOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  outKVO: Float64Array,
  outKVOSignal: Float64Array,
  core: Core = Core.DEFAULT,
): KvoStream {
  optInFastPeriod = requireInt32('KVO openAndFill', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('KVO openAndFill', 'slowPeriod', optInSlowPeriod);
  optInSignalPeriod = requireInt32('KVO openAndFill', 'signalPeriod', optInSignalPeriod);
  requireArgument('KVO openAndFill', 'prices.high', inHigh);
  requireHistory('KVO openAndFill', 'prices.high', inHigh.length);
  requireArgument('KVO openAndFill', 'prices.low', inLow);
  requireArgument('KVO openAndFill', 'prices.close', inClose);
  requireArgument('KVO openAndFill', 'prices.volume', inVolume);
  const guardLookback = kvoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('KVO openAndFill', PARAM_RANGES, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
  }
  const guardOutLen = openFillCount('KVO openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('KVO openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('KVO openAndFill', 'prices.close', inClose.length, inHigh.length);
  requireHistoryLength('KVO openAndFill', 'prices.volume', inVolume.length, inHigh.length);
  requireOutput('KVO openAndFill', 'outKvo', outKVO, guardOutLen, Float64Array);
  requireOutput('KVO openAndFill', 'outKvoSignal', outKVOSignal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  const inVolumeF64 = float64(inVolume);
  if (outKVO.buffer === inHighF64.buffer && outKVO.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('KVO openAndFill', 'outKvo', 'prices.high');
  }
  if (outKVO.buffer === inLowF64.buffer && outKVO.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('KVO openAndFill', 'outKvo', 'prices.low');
  }
  if (outKVO.buffer === inCloseF64.buffer && outKVO.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('KVO openAndFill', 'outKvo', 'prices.close');
  }
  if (outKVO.buffer === inVolumeF64.buffer && outKVO.byteOffset === inVolumeF64.byteOffset) {
    throw sharedMemory('KVO openAndFill', 'outKvo', 'prices.volume');
  }
  if (outKVOSignal.buffer === inHighF64.buffer && outKVOSignal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('KVO openAndFill', 'outKvoSignal', 'prices.high');
  }
  if (outKVOSignal.buffer === inLowF64.buffer && outKVOSignal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('KVO openAndFill', 'outKvoSignal', 'prices.low');
  }
  if (outKVOSignal.buffer === inCloseF64.buffer && outKVOSignal.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('KVO openAndFill', 'outKvoSignal', 'prices.close');
  }
  if (outKVOSignal.buffer === inVolumeF64.buffer && outKVOSignal.byteOffset === inVolumeF64.byteOffset) {
    throw sharedMemory('KVO openAndFill', 'outKvoSignal', 'prices.volume');
  }
  if (outKVO.buffer === outKVOSignal.buffer && outKVO.byteOffset === outKVOSignal.byteOffset) {
    throw sharedMemory('KVO openAndFill', 'outKvo', 'outKvoSignal');
  }
  if (guardOutLen > 0 && overlaps(outKVO, inHighF64)) {
    throw sharedMemory('KVO openAndFill', 'outKvo', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outKVO, inLowF64)) {
    throw sharedMemory('KVO openAndFill', 'outKvo', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outKVO, inCloseF64)) {
    throw sharedMemory('KVO openAndFill', 'outKvo', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outKVO, inVolumeF64)) {
    throw sharedMemory('KVO openAndFill', 'outKvo', 'prices.volume');
  }
  if (guardOutLen > 0 && overlaps(outKVOSignal, inHighF64)) {
    throw sharedMemory('KVO openAndFill', 'outKvoSignal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outKVOSignal, inLowF64)) {
    throw sharedMemory('KVO openAndFill', 'outKvoSignal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outKVOSignal, inCloseF64)) {
    throw sharedMemory('KVO openAndFill', 'outKvoSignal', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outKVOSignal, inVolumeF64)) {
    throw sharedMemory('KVO openAndFill', 'outKvoSignal', 'prices.volume');
  }
  if (guardOutLen > 0 && overlaps(outKVO, outKVOSignal)) {
    throw sharedMemory('KVO openAndFill', 'outKvo', 'outKvoSignal');
  }
  return kvoOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, inVolumeF64, 0, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, new MInteger(), new MInteger(), outKVO, outKVOSignal);
}

/**
 * Opens a live KVO stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link kvo} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price series. `low`: Low price series. `close`: Close price series.
 *   `volume`: Volume series.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link KvoOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `kvoLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or
 *   histories of different lengths.
 *
 * @example
 * ```ts
 * import { kvoOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const stream = kvoOpen({ high, low, close, volume }, {
 *   fastPeriod: 34,
 *   slowPeriod: 55,
 *   signalPeriod: 13,
 * });
 * const peeked = stream.peek(101.4, 99.1, 100.9, 12345); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9, 12345); // the bar closes
 * ```
 */
export function kvoOpen(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume: ArrayLike<number>; openInterest?: ArrayLike<number> | undefined }, options?: KvoOptions): KvoStream {
  requireArgument('KVO open', 'prices', prices);
  const o = optionsOf('KVO open', options);
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('KVO open', o.core);
  return kvoOpenPositional(prices.high, prices.low, prices.close, prices.volume, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core);
}

/**
 * {@link kvoOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link kvo}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price series. `low`: Low price series. `close`: Close price series.
 *   `volume`: Volume series.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link KvoOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link kvo} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `kvoLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { kvoOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const {
 *   stream,
 *   begIdx,
 *   outKvo,
 *   outKvoSignal,
 * } = kvoOpenAndFill({ high, low, close, volume }, {
 *   fastPeriod: 34,
 *   slowPeriod: 55,
 *   signalPeriod: 13,
 * });
 * // outKvo[k] is the value at bar begIdx + k
 * ```
 */
export function kvoOpenAndFill(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume: ArrayLike<number>; openInterest?: ArrayLike<number> | undefined }, options?: KvoOptions): KvoResult & { stream: KvoStream } {
  requireArgument('KVO openAndFill', 'prices', prices);
  const o = optionsOf('KVO openAndFill', options);
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('KVO openAndFill', o.core);
  const capacity = fillCapacity(kvoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core), prices.high, prices.low, prices.close, prices.volume);
  const outKVO = o.outKvo === undefined ? new Float64Array(capacity) : o.outKvo;
  const outKVOSignal = o.outKvoSignal === undefined ? new Float64Array(capacity) : o.outKvoSignal;
  const stream = kvoOpenAndFillPositional(prices.high, prices.low, prices.close, prices.volume, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outKVO, outKVOSignal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outKvo: outKVO.subarray(0, range.count), outKvoSignal: outKVOSignal.subarray(0, range.count) };
}
