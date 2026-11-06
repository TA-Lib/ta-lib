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
 *  100726 MF,CC  #492. Under an Auto level the bars its two EMAs' cancelling costs.
 */

import { FuncUnstId } from '../enums.ts';
import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { emaLookbackPositional } from './ema.ts';

/** @internal */
export function adoscLookbackPositional(optInFastPeriod: number, optInSlowPeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInFastPeriod | 0) !== optInFastPeriod) {
    return -1;
  }
  if ((optInSlowPeriod | 0) !== optInSlowPeriod) {
    return -1;
  }
  if (optInFastPeriod === Core.INTEGER_DEFAULT) {
    optInFastPeriod = 3;
  } else if (optInFastPeriod < 2 || optInFastPeriod > 100000) {
    return -1;
  }
  if (optInSlowPeriod === Core.INTEGER_DEFAULT) {
    optInSlowPeriod = 10;
  } else if (optInSlowPeriod < 2 || optInSlowPeriod > 100000) {
    return -1;
  }
  let slowestPeriod = 0;
  /* Use the slowest EMA period to evaluate the total lookback. */
  if (optInFastPeriod < optInSlowPeriod) {
    slowestPeriod = optInSlowPeriod;
  } else {
    slowestPeriod = optInFastPeriod;
  }
  /* Both EMAs seed on one value at one bar, so two starts differ by a fast
   * term less a slow one, and the two can cancel in the first outputs: an
   * Auto level is held against what is left.
   */
  return emaLookbackPositional(slowestPeriod, core) + ((core.unstableCount(FuncUnstId.EMA, 1, 1) != core.unstableCount(FuncUnstId.EMA, 0, 0)) ? (core.unstableCount(FuncUnstId.EMA, ((((((15 * (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) | 0) + ((3 * ((((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 2) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 2) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) + (((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 4) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 4) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) + (((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 8) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 8) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) + (((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 16) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 16) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) + (((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 32) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 32) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) + (((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 64) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 64) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) + (((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 128) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 128) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) + (((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 256) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 256) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) + (((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 512) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 512) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) + (((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 1024) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 1024) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) + (((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 2048) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 2048) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) + (((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 4096) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 4096) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) + (((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 8192) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 8192) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) + (((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 16384) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 16384) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) + (((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 32768) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 32768) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) + (((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 65536) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 65536) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)))) | 0) + 7) / 8) | 0)), ((((((15 * (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) | 0) + ((3 * ((((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 2) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 2) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) + (((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 4) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 4) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) + (((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 8) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 8) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) + (((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 16) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 16) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) + (((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 32) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 32) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) + (((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 64) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 64) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) + (((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 128) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 128) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) + (((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 256) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 256) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) + (((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 512) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 512) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) + (((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 1024) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 1024) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) + (((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 2048) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 2048) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) + (((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 4096) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 4096) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) + (((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 8192) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 8192) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) + (((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 16384) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 16384) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) + (((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 32768) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 32768) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)) + (((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 65536) | 0)) < (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) ? ((((optInFastPeriod > optInSlowPeriod ? optInFastPeriod : optInSlowPeriod) / 65536) | 0)) : (optInFastPeriod < optInSlowPeriod ? optInFastPeriod : optInSlowPeriod)))) | 0) + 7) / 8) | 0))) - core.unstableCount(FuncUnstId.EMA, 0, 0)) : 0);
}

/** @internal */
export function adoscDisplayShiftPositional(optInFastPeriod: number, optInSlowPeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (adoscLookbackPositional(optInFastPeriod, optInSlowPeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function adoscImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  inVolume: Float64Array,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let today = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let high = 0;
  let low = 0;
  let close = 0;
  let tmp = 0;
  let slowEMA = 0;
  let slowk = 0;
  let one_minus_slowk = 0;
  let fastEMA = 0;
  let fastk = 0;
  let one_minus_fastk = 0;
  let ad = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInFastPeriod === Core.INTEGER_DEFAULT) {
    optInFastPeriod = 3;
  } else if (optInFastPeriod < 2 || optInFastPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowPeriod === Core.INTEGER_DEFAULT) {
    optInSlowPeriod = 10;
  } else if (optInSlowPeriod < 2 || optInSlowPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  /* Implementation Note:
   *     The fastEMA varaible is not neceseraly the
   *     fastest EMA.
   *     In the same way, slowEMA is not neceseraly the
   *     slowest EMA.
   *
   *     The ADOSC is always the (fastEMA - slowEMA) regardless
   *     of the period specified. In other word:
   *
   *     ADOSC(3,10) = EMA(3,AD) - EMA(10,AD)
   *
   *        while
   *
   *     ADOSC(10,3) = EMA(10,AD)- EMA(3,AD)
   *
   *     In the first case the EMA(3) is truly a faster EMA,
   *     while in the second case, the EMA(10) is still call
   *     fastEMA in the algorithm, even if it is in fact slower.
   *
   *     This gives more flexibility to the user if they want to
   *     experiment with unusual parameter settings.
   */
  /* Adjust startIdx to account for the lookback period. */
  lookbackTotal = adoscLookbackPositional(optInFastPeriod, optInSlowPeriod, core);
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
  today = startIdx - lookbackTotal;
  /* The following variables are used to
   * calculate the "ad".
   */
  ad = 0.0;
  /* Constants for EMA. Each pair must sum to exactly 1.0, or a flat A/D
   * line drifts off its level.
   */
  one_minus_fastk = (optInFastPeriod - 1) / (optInFastPeriod + 1);
  fastk = 1.0 - one_minus_fastk;
  if (one_minus_fastk < 0.5) {
    one_minus_fastk = 1.0 - fastk;
  }
  one_minus_slowk = (optInSlowPeriod - 1) / (optInSlowPeriod + 1);
  slowk = 1.0 - one_minus_slowk;
  if (one_minus_slowk < 0.5) {
    one_minus_slowk = 1.0 - slowk;
  }
  /* Initialize the two EMA
   *
   * Use the same range of initialization inputs for
   * both EMA and simply seed with the first A/D value.
   *
   * Note: Metastock do the same.
   */
  high = inHigh[today];
  low = inLow[today];
  tmp = high - low;
  close = inClose[today];
  if (tmp > 0.0) {
    ad += (close - low - (high - close)) / tmp * inVolume[today];
  }
  today += 1;
  fastEMA = ad;
  slowEMA = ad;
  /* Initialize the EMA and skip the unstable period. */
  while (today < startIdx) {
    high = inHigh[today];
    low = inLow[today];
    tmp = high - low;
    close = inClose[today];
    if (tmp > 0.0) {
      ad += (close - low - (high - close)) / tmp * inVolume[today];
    }
    today += 1;
    {
      const _a: number = one_minus_fastk, _b: number = fastEMA, _c: number = fastk * ad;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      fastEMA = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = one_minus_slowk, _b: number = slowEMA, _c: number = slowk * ad;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      slowEMA = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
  }
  /* Perform the calculation for the requested range */
  outIdx = 0;
  while (today <= endIdx) {
    high = inHigh[today];
    low = inLow[today];
    tmp = high - low;
    close = inClose[today];
    if (tmp > 0.0) {
      ad += (close - low - (high - close)) / tmp * inVolume[today];
    }
    today += 1;
    {
      const _a: number = one_minus_fastk, _b: number = fastEMA, _c: number = fastk * ad;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      fastEMA = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = one_minus_slowk, _b: number = slowEMA, _c: number = slowk * ad;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      slowEMA = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    outReal[outIdx++] = fastEMA - slowEMA;
  }
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['fastPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['slowPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000']] as const;

/** @internal */
export function adoscPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('ADOSC', 'startIdx', startIdx);
  endIdx = requireInt32('ADOSC', 'endIdx', endIdx);
  optInFastPeriod = requireInt32('ADOSC', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('ADOSC', 'slowPeriod', optInSlowPeriod);
  requireIndexRange('ADOSC', startIdx, endIdx);
  const guardLookback = adoscLookbackPositional(optInFastPeriod, optInSlowPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('ADOSC', PARAM_RANGES, optInFastPeriod, optInSlowPeriod);
  }
  const guardStart = clampedStart('ADOSC', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('ADOSC', 'prices.high', inHigh, guardInLen);
  requireLength('ADOSC', 'prices.low', inLow, guardInLen);
  requireLength('ADOSC', 'prices.close', inClose, guardInLen);
  requireLength('ADOSC', 'prices.volume', inVolume, guardInLen);
  requireOutput('ADOSC', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  const inVolumeF64 = float64(inVolume);
  if (overlapsPartly(outReal, inHighF64)) {
    throw sharedMemory('ADOSC', 'outReal', 'prices.high');
  }
  if (overlapsPartly(outReal, inLowF64)) {
    throw sharedMemory('ADOSC', 'outReal', 'prices.low');
  }
  if (overlapsPartly(outReal, inCloseF64)) {
    throw sharedMemory('ADOSC', 'outReal', 'prices.close');
  }
  if (overlapsPartly(outReal, inVolumeF64)) {
    throw sharedMemory('ADOSC', 'outReal', 'prices.volume');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = adoscImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, inVolumeF64, optInFastPeriod, optInSlowPeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('ADOSC', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link adosc}, {@link adoscLookback}, {@link adoscDisplayShift},
 * {@link adoscOpen} and {@link adoscOpenAndFill}. Every key is optional: an
 * omitted or `undefined` key takes its default, and a call ignores the keys it has
 * no use for.
 */
export interface AdoscOptions {
  /**
   * Period of the fast A/D EMA (default 3; range 2 to 100000; `Core.INTEGER_DEFAULT`
   * selects the default).
   */
  fastPeriod?: number | undefined;
  /**
   * Period of the slow A/D EMA (default 10; range 2 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  slowPeriod?: number | undefined;
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
  outReal?: Float64Array | undefined;
}

/** What {@link adosc} returns, and {@link adoscOpenAndFill} with its stream. */
export interface AdoscResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Fast-EMA minus slow-EMA of the A/D line. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link adosc} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AdoscOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function adoscLookback(options?: AdoscOptions): number {
  const o = optionsOf('ADOSC lookback', options);
  return adoscLookbackPositional(orDefault(o.fastPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowPeriod, Core.INTEGER_DEFAULT), coreOf('ADOSC lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link adosc}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AdoscOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function adoscDisplayShift(outputIdx: number, options?: AdoscOptions): number {
  const o = optionsOf('ADOSC displayShift', options);
  return adoscDisplayShiftPositional(orDefault(o.fastPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowPeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('ADOSC displayShift', o.core));
}

/**
 * Chaikin A/D Oscillator: the difference between a fast and a slow EMA of the
 * Accumulation/Distribution line. Highlights momentum in accumulation/distribution
 * volume flow. Positive/rising suggests accumulation; negative/falling suggests
 * distribution.
 *
 * Formula and more info at
 * [ta-lib.org/functions/adosc](https://ta-lib.org/functions/adosc).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link adoscLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `high`: High
 *   price of each bar. `low`: Low price of each bar. `close`: Close price of each
 *   bar. `volume`: Volume of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link AdoscOptions}.
 * @returns `begIdx` and the values; see {@link AdoscResult}. Each output is a view
 *   of its buffer, the caller's or a new one.
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
 * import { adosc } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const { begIdx, values } = adosc({ high, low, close, volume }, {
 *   fastPeriod: 3,
 *   slowPeriod: 10,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`ad`](https://ta-lib.org/functions/ad)
 * @see [`ema`](https://ta-lib.org/functions/ema)
 */
export function adosc(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume: ArrayLike<number> }, options?: AdoscOptions): AdoscResult {
  requireArgument('ADOSC', 'prices', prices);
  const o = optionsOf('ADOSC', options);
  if (o.endIdx === undefined) {
    requireSeries('ADOSC', 'prices.high', prices.high);
    requireSeries('ADOSC', 'prices.low', prices.low);
    requireSeries('ADOSC', 'prices.close', prices.close);
    requireSeries('ADOSC', 'prices.volume', prices.volume);
  }
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('ADOSC', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, adoscLookbackPositional(optInFastPeriod, optInSlowPeriod, core), prices.high, prices.low, prices.close, prices.volume);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = adoscPositional(startIdx, endIdx, prices.high, prices.low, prices.close, prices.volume, optInFastPeriod, optInSlowPeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live ADOSC stream: one value per closed bar, bit-identical to {@link adosc}
 * over the same series. Open it with {@link adoscOpen} or
 * {@link adoscOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/adosc](https://ta-lib.org/functions/adosc).
 */
export class AdoscStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInFastPeriod: number = 0;
  /** @internal */
  optInSlowPeriod: number = 0;
  /** @internal */
  slowEMA: number = 0;
  /** @internal */
  slowk: number = 0;
  /** @internal */
  one_minus_slowk: number = 0;
  /** @internal */
  fastEMA: number = 0;
  /** @internal */
  fastk: number = 0;
  /** @internal */
  one_minus_fastk: number = 0;
  /** @internal */
  ad: number = 0;
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
  static of(core: Core): AdoscStream {
    return new AdoscStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link adosc} reports them over the same bars. Each accepted `update` and each
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
      throw failure('ADOSC advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param inVolume - The bar's `inVolume` value: Volume of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number, inClose: number, inVolume: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('ADOSC update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose) || !Number.isFinite(inVolume)) {
      throw nonFiniteBar('ADOSC update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : !Number.isFinite(inClose) ? 'inClose' : 'inVolume');
    }
    adoscStepImpl(this, inHigh, inLow, inClose, inVolume);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param inVolume - The bar's `inVolume` value: Volume of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number, inClose: number, inVolume: number): number {
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose) || !Number.isFinite(inVolume)) {
      throw nonFiniteBar('ADOSC peek', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : !Number.isFinite(inClose) ? 'inClose' : 'inVolume');
    }
    const sp = this;
    let high = 0;
    let low = 0;
    let close = 0;
    let tmp = 0;
    let ad = sp.ad;
    let cur_outReal = 0;
    let fastEMA = sp.fastEMA;
    let slowEMA = sp.slowEMA;
    high = inHigh;
    low = inLow;
    tmp = high - low;
    close = inClose;
    if (tmp > 0.0) {
      ad += (close - low - (high - close)) / tmp * inVolume;
    }
    {
      const _a: number = sp.one_minus_fastk, _b: number = fastEMA, _c: number = sp.fastk * ad;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      fastEMA = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = sp.one_minus_slowk, _b: number = slowEMA, _c: number = sp.slowk * ad;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      slowEMA = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    cur_outReal = fastEMA - slowEMA;
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
  clone(): AdoscStream {
    const c = new AdoscStream(this.core);
    c.optInFastPeriod = this.optInFastPeriod;
    c.optInSlowPeriod = this.optInSlowPeriod;
    c.slowEMA = this.slowEMA;
    c.slowk = this.slowk;
    c.one_minus_slowk = this.one_minus_slowk;
    c.fastEMA = this.fastEMA;
    c.fastk = this.fastk;
    c.one_minus_fastk = this.one_minus_fastk;
    c.ad = this.ad;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function adoscStepImpl(sp: AdoscStream, inHigh: number, inLow: number, inClose: number, inVolume: number): void {
  let high = 0;
  let low = 0;
  let close = 0;
  let tmp = 0;
  high = inHigh;
  low = inLow;
  tmp = high - low;
  close = inClose;
  if (tmp > 0.0) {
    sp.ad += (close - low - (high - close)) / tmp * inVolume;
  }
  {
    const _a: number = sp.one_minus_fastk, _b: number = sp.fastEMA, _c: number = sp.fastk * sp.ad;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.fastEMA = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  {
    const _a: number = sp.one_minus_slowk, _b: number = sp.slowEMA, _c: number = sp.slowk * sp.ad;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.slowEMA = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  sp.cur_outReal = sp.fastEMA - sp.slowEMA;
}

function adoscOpenImpl(
  sp: AdoscStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let today = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let high = 0;
  let low = 0;
  let close = 0;
  let tmp = 0;
  let slowEMA = 0;
  let slowk = 0;
  let one_minus_slowk = 0;
  let fastEMA = 0;
  let fastk = 0;
  let one_minus_fastk = 0;
  let ad = 0;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length || inClose.length !== inHigh.length || inVolume.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInFastPeriod === Core.INTEGER_DEFAULT) {
    optInFastPeriod = 3;
  } else if (optInFastPeriod < 2 || optInFastPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowPeriod === Core.INTEGER_DEFAULT) {
    optInSlowPeriod = 10;
  } else if (optInSlowPeriod < 2 || optInSlowPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Implementation Note:
   *     The fastEMA varaible is not neceseraly the
   *     fastest EMA.
   *     In the same way, slowEMA is not neceseraly the
   *     slowest EMA.
   *
   *     The ADOSC is always the (fastEMA - slowEMA) regardless
   *     of the period specified. In other word:
   *
   *     ADOSC(3,10) = EMA(3,AD) - EMA(10,AD)
   *
   *        while
   *
   *     ADOSC(10,3) = EMA(10,AD)- EMA(3,AD)
   *
   *     In the first case the EMA(3) is truly a faster EMA,
   *     while in the second case, the EMA(10) is still call
   *     fastEMA in the algorithm, even if it is in fact slower.
   *
   *     This gives more flexibility to the user if they want to
   *     experiment with unusual parameter settings.
   */
  /* Adjust startIdx to account for the lookback period. */
  lookbackTotal = adoscLookbackPositional(optInFastPeriod, optInSlowPeriod, core);
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
  today = startIdx - lookbackTotal;
  /* The following variables are used to
   * calculate the "ad".
   */
  ad = 0.0;
  /* Constants for EMA. Each pair must sum to exactly 1.0, or a flat A/D
   * line drifts off its level.
   */
  one_minus_fastk = (optInFastPeriod - 1) / (optInFastPeriod + 1);
  fastk = 1.0 - one_minus_fastk;
  if (one_minus_fastk < 0.5) {
    one_minus_fastk = 1.0 - fastk;
  }
  one_minus_slowk = (optInSlowPeriod - 1) / (optInSlowPeriod + 1);
  slowk = 1.0 - one_minus_slowk;
  if (one_minus_slowk < 0.5) {
    one_minus_slowk = 1.0 - slowk;
  }
  /* Initialize the two EMA
   *
   * Use the same range of initialization inputs for
   * both EMA and simply seed with the first A/D value.
   *
   * Note: Metastock do the same.
   */
  high = inHigh[today];
  low = inLow[today];
  tmp = high - low;
  close = inClose[today];
  if (tmp > 0.0) {
    ad += (close - low - (high - close)) / tmp * inVolume[today];
  }
  today += 1;
  fastEMA = ad;
  slowEMA = ad;
  /* Initialize the EMA and skip the unstable period. */
  while (today < startIdx) {
    high = inHigh[today];
    low = inLow[today];
    tmp = high - low;
    close = inClose[today];
    if (tmp > 0.0) {
      ad += (close - low - (high - close)) / tmp * inVolume[today];
    }
    today += 1;
    {
      const _a: number = one_minus_fastk, _b: number = fastEMA, _c: number = fastk * ad;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      fastEMA = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = one_minus_slowk, _b: number = slowEMA, _c: number = slowk * ad;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      slowEMA = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
  }
  /* Perform the calculation for the requested range */
  outIdx = 0;
  while (today <= endIdx) {
    high = inHigh[today];
    low = inLow[today];
    tmp = high - low;
    close = inClose[today];
    if (tmp > 0.0) {
      ad += (close - low - (high - close)) / tmp * inVolume[today];
    }
    today += 1;
    {
      const _a: number = one_minus_fastk, _b: number = fastEMA, _c: number = fastk * ad;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      fastEMA = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = one_minus_slowk, _b: number = slowEMA, _c: number = slowk * ad;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      slowEMA = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    outReal[((outIdx++ * outStride) | 0)] = fastEMA - slowEMA;
  }
  outNBElement.value = outIdx;
  sp.optInFastPeriod = optInFastPeriod;
  sp.optInSlowPeriod = optInSlowPeriod;
  sp.slowEMA = slowEMA;
  sp.slowk = slowk;
  sp.one_minus_slowk = one_minus_slowk;
  sp.fastEMA = fastEMA;
  sp.fastk = fastk;
  sp.one_minus_fastk = one_minus_fastk;
  sp.ad = ad;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function adoscOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): AdoscStream {
  const sp = AdoscStream.of(core);
  const retCode = adoscOpenImpl(sp, inHigh, inLow, inClose, inVolume, startIdx, optInFastPeriod, optInSlowPeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ADOSC openAndFill', inHigh.length, startIdx, adoscLookbackPositional(optInFastPeriod, optInSlowPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && adoscLookbackPositional(optInFastPeriod, optInSlowPeriod, core) < 0) {
    throw rejectedParams('ADOSC openAndFill', PARAM_RANGES, optInFastPeriod, optInSlowPeriod);
  }
  throw streamFailure('ADOSC openAndFill', retCode);
}

/** @internal */
export function adoscOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
): AdoscStream {
  const sp = AdoscStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = adoscOpenImpl(sp, inHigh, inLow, inClose, inVolume, startIdx, optInFastPeriod, optInSlowPeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ADOSC open', inHigh.length, startIdx, adoscLookbackPositional(optInFastPeriod, optInSlowPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && adoscLookbackPositional(optInFastPeriod, optInSlowPeriod, core) < 0) {
    throw rejectedParams('ADOSC open', PARAM_RANGES, optInFastPeriod, optInSlowPeriod);
  }
  throw streamFailure('ADOSC open', retCode);
}

/** @internal */
export function adoscOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  core: Core = Core.DEFAULT,
): AdoscStream {
  optInFastPeriod = requireInt32('ADOSC open', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('ADOSC open', 'slowPeriod', optInSlowPeriod);
  requireArgument('ADOSC open', 'prices.high', inHigh);
  requireHistory('ADOSC open', inHigh.length);
  requireArgument('ADOSC open', 'prices.low', inLow);
  requireArgument('ADOSC open', 'prices.close', inClose);
  requireArgument('ADOSC open', 'prices.volume', inVolume);
  requireHistoryLength('ADOSC open', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('ADOSC open', 'prices.close', inClose.length, inHigh.length);
  requireHistoryLength('ADOSC open', 'prices.volume', inVolume.length, inHigh.length);
  return adoscOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), float64(inVolume), 0, optInFastPeriod, optInSlowPeriod);
}

/** @internal */
export function adoscOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): AdoscStream {
  optInFastPeriod = requireInt32('ADOSC openAndFill', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('ADOSC openAndFill', 'slowPeriod', optInSlowPeriod);
  requireArgument('ADOSC openAndFill', 'prices.high', inHigh);
  requireHistory('ADOSC openAndFill', inHigh.length);
  requireArgument('ADOSC openAndFill', 'prices.low', inLow);
  requireArgument('ADOSC openAndFill', 'prices.close', inClose);
  requireArgument('ADOSC openAndFill', 'prices.volume', inVolume);
  const guardLookback = adoscLookbackPositional(optInFastPeriod, optInSlowPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('ADOSC openAndFill', PARAM_RANGES, optInFastPeriod, optInSlowPeriod);
  }
  const guardOutLen = openFillCount('ADOSC openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('ADOSC openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('ADOSC openAndFill', 'prices.close', inClose.length, inHigh.length);
  requireHistoryLength('ADOSC openAndFill', 'prices.volume', inVolume.length, inHigh.length);
  requireOutput('ADOSC openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  const inVolumeF64 = float64(inVolume);
  if (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('ADOSC openAndFill', 'outReal', 'prices.high');
  }
  if (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('ADOSC openAndFill', 'outReal', 'prices.low');
  }
  if (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('ADOSC openAndFill', 'outReal', 'prices.close');
  }
  if (outReal.buffer === inVolumeF64.buffer && outReal.byteOffset === inVolumeF64.byteOffset) {
    throw sharedMemory('ADOSC openAndFill', 'outReal', 'prices.volume');
  }
  if (guardOutLen > 0 && overlaps(outReal, inHighF64)) {
    throw sharedMemory('ADOSC openAndFill', 'outReal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outReal, inLowF64)) {
    throw sharedMemory('ADOSC openAndFill', 'outReal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outReal, inCloseF64)) {
    throw sharedMemory('ADOSC openAndFill', 'outReal', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outReal, inVolumeF64)) {
    throw sharedMemory('ADOSC openAndFill', 'outReal', 'prices.volume');
  }
  return adoscOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, inVolumeF64, 0, optInFastPeriod, optInSlowPeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live ADOSC stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link adosc} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar. `volume`: Volume of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AdoscOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `adoscLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range
 *   or histories of different lengths.
 *
 * @example
 * ```ts
 * import { adoscOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const stream = adoscOpen({ high, low, close, volume }, {
 *   fastPeriod: 3,
 *   slowPeriod: 10,
 * });
 * const peeked = stream.peek(101.4, 99.1, 100.9, 12345); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9, 12345); // the bar closes
 * ```
 */
export function adoscOpen(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume: ArrayLike<number> }, options?: AdoscOptions): AdoscStream {
  requireArgument('ADOSC open', 'prices', prices);
  const o = optionsOf('ADOSC open', options);
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('ADOSC open', o.core);
  return adoscOpenPositional(prices.high, prices.low, prices.close, prices.volume, optInFastPeriod, optInSlowPeriod, core);
}

/**
 * {@link adoscOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link adosc}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar. `volume`: Volume of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link AdoscOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link adosc} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `adoscLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { adoscOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const { stream, begIdx, values } = adoscOpenAndFill({ high, low, close, volume }, {
 *   fastPeriod: 3,
 *   slowPeriod: 10,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function adoscOpenAndFill(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume: ArrayLike<number> }, options?: AdoscOptions): AdoscResult & { stream: AdoscStream } {
  requireArgument('ADOSC openAndFill', 'prices', prices);
  const o = optionsOf('ADOSC openAndFill', options);
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('ADOSC openAndFill', o.core);
  const capacity = fillCapacity(adoscLookbackPositional(optInFastPeriod, optInSlowPeriod, core), prices.high, prices.low, prices.close, prices.volume);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = adoscOpenAndFillPositional(prices.high, prices.low, prices.close, prices.volume, optInFastPeriod, optInSlowPeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
