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
 *  093026 KL,CC  Creation (#477).
 *  093026 MF,CC  Rolling extrema in a fixed number of comparisons per bar.
 *  100726 MF,CC  #492. Under an Auto level the stop window is counted again.
 */

import { FuncUnstId } from '../enums.ts';
import { badSink, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireReal, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma, fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { atrLookbackPositional } from './atr.ts';

/** @internal */
export function ckspLookbackPositional(optInTimePeriod: number, optInMultiplier: number, optInStopPeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (typeof optInMultiplier !== 'number') {
    return -1;
  }
  if ((optInStopPeriod | 0) !== optInStopPeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 10;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  if (optInMultiplier === Core.REAL_DEFAULT) {
    optInMultiplier = 1.0;
  } else if (!(optInMultiplier >= 0e0 && optInMultiplier <= Core.REAL_MAX)) {
    return -1;
  }
  if (optInStopPeriod === Core.INTEGER_DEFAULT) {
    optInStopPeriod = 9;
  } else if (optInStopPeriod < 1 || optInStopPeriod > 100000) {
    return -1;
  }
  /* The first stops need the Average True Range at their own bar, and the
   * stop window reaches optInStopPeriod-1 first stops further back. The ATR
   * term is never restated here, which is what makes CKSP inherit
   * TA_FUNC_UNST_ATR.
   *
   * A stop can rest on a first stop optInStopPeriod-1 bars old, whose ATR
   * was that much closer to its seed: the first difference two starts show
   * is the smaller for it, and an Auto level is held against that one.
   */
  return atrLookbackPositional(optInTimePeriod, core) + optInStopPeriod - 1 + ((core.unstableCount(FuncUnstId.ATR, 1, 1) != core.unstableCount(FuncUnstId.ATR, 0, 0)) ? (core.unstableCount(FuncUnstId.ATR, optInStopPeriod - 1, optInStopPeriod - 1) - core.unstableCount(FuncUnstId.ATR, 0, 0)) : 0);
}

/** @internal */
export function ckspDisplayShiftPositional(optInTimePeriod: number, optInMultiplier: number, optInStopPeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (ckspLookbackPositional(optInTimePeriod, optInMultiplier, optInStopPeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 2) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function ckspImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  optInTimePeriod: number,
  optInMultiplier: number,
  optInStopPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outHighStop: Float64Array,
  outLowStop: Float64Array,
): RetCode {
  let hhBuf!: Float64Array;
  let hhBuf_Idx = 0;
  let maxIdx_hhBuf = (30) - 1;
  let llBuf!: Float64Array;
  let hsBuf!: Float64Array;
  let hsBuf_Idx = 0;
  let maxIdx_hsBuf = (30) - 1;
  let lsBuf!: Float64Array;
  let i = 0;
  let ip = 0;
  let iq = 0;
  let today = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let lastP = 0;
  let lastQ = 0;
  let prevATR = 0;
  let periodTotal = 0;
  let wAlpha = 0;
  let wBeta = 0;
  let val2 = 0;
  let val3 = 0;
  let greatest = 0;
  let tempCY = 0;
  let tempLT = 0;
  let tempHT = 0;
  let tempReal = 0;
  let hhPre = 0;
  let llPre = 0;
  let hsPre = 0;
  let lsPre = 0;
  let sufHi = 0;
  let sufLo = 0;
  let highest = 0;
  let lowest = 0;
  let band = 0;
  let highStop = 0;
  let lowStop = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 10;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInMultiplier === Core.REAL_DEFAULT) {
    optInMultiplier = 1.0;
  } else if (!(optInMultiplier >= 0e0 && optInMultiplier <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInStopPeriod === Core.INTEGER_DEFAULT) {
    optInStopPeriod = 9;
  } else if (optInStopPeriod < 1 || optInStopPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if ((outHighStop.buffer === outLowStop.buffer && outHighStop.byteOffset === outLowStop.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = ckspLookbackPositional(optInTimePeriod, optInMultiplier, optInStopPeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  /* Four rolling extrema, van Herk / Gil-Werman, one buffer each: the window
   * ending in slot j is the current block's prefix extremum joined with the
   * previous block's suffix extremum from slot j+1. A slot holds the raw value
   * while its block is filling and is turned into the suffix extremum, in
   * place, when the block completes; slot j+1 is always still the previous
   * block's when slot j is filled. The high and low sides share an index.
   *
   * The extrema are exact, so both outputs must stay bit-identical to
   * TA_MAX( TA_MAX(high) - x*TA_ATR ) and its mirror, whatever the block
   * phase; only the sign of a zero tied between +0.0 and -0.0 is free.
   */
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  hhBuf = new Float64Array(optInTimePeriod);
  maxIdx_hhBuf = optInTimePeriod - 1;
  hhBuf_Idx = 0;
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  llBuf = new Float64Array(optInTimePeriod);
  if (optInStopPeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  hsBuf = new Float64Array(optInStopPeriod);
  maxIdx_hsBuf = optInStopPeriod - 1;
  hsBuf_Idx = 0;
  if (optInStopPeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  lsBuf = new Float64Array(optInStopPeriod);
  lastP = optInTimePeriod - 1;
  lastQ = optInStopPeriod - 1;
  /* The Average True Range is carried inline rather than taken from a call,
   * because the two stages advance together one bar at a time and a
   * whole-range buffer between them would not stream.
   *
   * The arithmetic order below is the bit-exactness contract with TA_ATR (do
   * not reorder): True Range from high-low, then the two previous-close
   * distances in that order; the seed summed from 0.0 over the first 'period'
   * True Ranges and divided once; the same two Wilder coefficients, wBeta
   * rounded and wAlpha derived from it, in one fused statement.
   */
  wBeta = (optInTimePeriod - 1) / optInTimePeriod;
  wAlpha = 1.0 - wBeta;
  today = startIdx - lookbackTotal + 1;
  periodTotal = 0.0;
  i = optInTimePeriod;
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
  prevATR = periodTotal / optInTimePeriod;
  /* Skip the bars the lookback adds for the unstable period. Taking the count
   * from the lookback rather than naming the setting keeps the two from
   * disagreeing.
   */
  i = lookbackTotal - lastQ - optInTimePeriod;
  while (i !== 0) {
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
    i -= 1;
  }
  /* prevATR is now the Average True Range of bar today-1, the first bar with
   * a first stop. Its extreme window is one whole block, taken straight from
   * the input.
   */
  highest = inHigh[today - 1];
  lowest = inLow[today - 1];
  hhBuf[lastP] = highest;
  llBuf[lastP] = lowest;
  ip = lastP;
  while (ip > 0) {
    ip -= 1;
    tempHT = inHigh[today - 1 - lastP + ip];
    tempLT = inLow[today - 1 - lastP + ip];
    if (tempHT > highest) {
      highest = tempHT;
    }
    if (tempLT < lowest) {
      lowest = tempLT;
    }
    hhBuf[ip] = highest;
    llBuf[ip] = lowest;
  }
  hhPre = highest;
  llPre = lowest;
  /* The multiple of the ATR is formed on its own, never fused into the
   * offset, so that each first stop is TA_MAX - x*TA_ATR (or its mirror) as a
   * caller composing the three functions would compute it.
   */
  band = optInMultiplier * prevATR;
  highStop = highest - band;
  lowStop = lowest + band;
  hsPre = highStop;
  lsPre = lowStop;
  hsBuf[hsBuf_Idx] = highStop;
  lsBuf[hsBuf_Idx] = lowStop;
  hsBuf_Idx++;
  if (hsBuf_Idx > maxIdx_hsBuf) {
    hsBuf_Idx = 0;
  }
  /* Fill the first stop window, through startIdx inclusive. */
  while (today <= startIdx) {
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
    /* Keep this shape: the prefix extreme selected in a local, the
     * block-start override after it, one store. A store made only when the
     * bar sets a new extreme, or the override ahead of the select, compiles
     * to a data-dependent branch in the stream step, where the prefix lives
     * in the handle.
     */
    highest = hhPre;
    lowest = llPre;
    if (tempHT > highest) {
      highest = tempHT;
    }
    if (tempLT < lowest) {
      lowest = tempLT;
    }
    if (hhBuf_Idx === 0) {
      highest = tempHT;
      lowest = tempLT;
    }
    hhPre = highest;
    llPre = lowest;
    if (hhBuf_Idx < lastP) {
      tempReal = hhBuf[hhBuf_Idx + 1];
      if (tempReal > highest) {
        highest = tempReal;
      }
      tempReal = llBuf[hhBuf_Idx + 1];
      if (tempReal < lowest) {
        lowest = tempReal;
      }
    }
    hhBuf[hhBuf_Idx] = tempHT;
    llBuf[hhBuf_Idx] = tempLT;
    hhBuf_Idx++;
    if (hhBuf_Idx > maxIdx_hhBuf) {
      hhBuf_Idx = 0;
    }
    if (hhBuf_Idx === 0) {
      sufHi = hhBuf[lastP];
      sufLo = llBuf[lastP];
      ip = lastP;
      while (ip > 1) {
        ip -= 1;
        tempReal = hhBuf[ip];
        if (tempReal > sufHi) {
          sufHi = tempReal;
        }
        hhBuf[ip] = sufHi;
        tempReal = llBuf[ip];
        if (tempReal < sufLo) {
          sufLo = tempReal;
        }
        llBuf[ip] = sufLo;
      }
    }
    band = optInMultiplier * prevATR;
    highStop = highest - band;
    lowStop = lowest + band;
    if (highStop > hsPre) {
      hsPre = highStop;
    }
    if (lowStop < lsPre) {
      lsPre = lowStop;
    }
    hsBuf[hsBuf_Idx] = highStop;
    lsBuf[hsBuf_Idx] = lowStop;
    hsBuf_Idx++;
    if (hsBuf_Idx > maxIdx_hsBuf) {
      hsBuf_Idx = 0;
    }
    today += 1;
  }
  /* The first stop window is one whole block as well: its extremes are the
   * prefix extremes, and the block is complete.
   */
  sufHi = hsBuf[lastQ];
  sufLo = lsBuf[lastQ];
  iq = lastQ;
  while (iq > 1) {
    iq -= 1;
    tempReal = hsBuf[iq];
    if (tempReal > sufHi) {
      sufHi = tempReal;
    }
    hsBuf[iq] = sufHi;
    tempReal = lsBuf[iq];
    if (tempReal < sufLo) {
      sufLo = tempReal;
    }
    lsBuf[iq] = sufLo;
  }
  outHighStop[0] = hsPre;
  outLowStop[0] = lsPre;
  outIdx = 1;
  while (today <= endIdx) {
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
    highest = hhPre;
    lowest = llPre;
    if (tempHT > highest) {
      highest = tempHT;
    }
    if (tempLT < lowest) {
      lowest = tempLT;
    }
    if (hhBuf_Idx === 0) {
      highest = tempHT;
      lowest = tempLT;
    }
    hhPre = highest;
    llPre = lowest;
    if (hhBuf_Idx < lastP) {
      tempReal = hhBuf[hhBuf_Idx + 1];
      if (tempReal > highest) {
        highest = tempReal;
      }
      tempReal = llBuf[hhBuf_Idx + 1];
      if (tempReal < lowest) {
        lowest = tempReal;
      }
    }
    hhBuf[hhBuf_Idx] = tempHT;
    llBuf[hhBuf_Idx] = tempLT;
    hhBuf_Idx++;
    if (hhBuf_Idx > maxIdx_hhBuf) {
      hhBuf_Idx = 0;
    }
    band = optInMultiplier * prevATR;
    highStop = highest - band;
    lowStop = lowest + band;
    highest = hsPre;
    lowest = lsPre;
    if (highStop > highest) {
      highest = highStop;
    }
    if (lowStop < lowest) {
      lowest = lowStop;
    }
    if (hsBuf_Idx === 0) {
      highest = highStop;
      lowest = lowStop;
    }
    hsPre = highest;
    lsPre = lowest;
    if (hsBuf_Idx < lastQ) {
      tempReal = hsBuf[hsBuf_Idx + 1];
      if (tempReal > highest) {
        highest = tempReal;
      }
      tempReal = lsBuf[hsBuf_Idx + 1];
      if (tempReal < lowest) {
        lowest = tempReal;
      }
    }
    hsBuf[hsBuf_Idx] = highStop;
    lsBuf[hsBuf_Idx] = lowStop;
    hsBuf_Idx++;
    if (hsBuf_Idx > maxIdx_hsBuf) {
      hsBuf_Idx = 0;
    }
    outHighStop[outIdx] = highest;
    outLowStop[outIdx] = lowest;
    /* A completed block becomes its suffix extrema. Nothing this bar reads
     * them, so both passes stay below the output stores, where the peek
     * frame never runs them.
     */
    if (hhBuf_Idx === 0) {
      sufHi = hhBuf[lastP];
      sufLo = llBuf[lastP];
      ip = lastP;
      while (ip > 1) {
        ip -= 1;
        tempReal = hhBuf[ip];
        if (tempReal > sufHi) {
          sufHi = tempReal;
        }
        hhBuf[ip] = sufHi;
        tempReal = llBuf[ip];
        if (tempReal < sufLo) {
          sufLo = tempReal;
        }
        llBuf[ip] = sufLo;
      }
    }
    if (hsBuf_Idx === 0) {
      sufHi = hsBuf[lastQ];
      sufLo = lsBuf[lastQ];
      iq = lastQ;
      while (iq > 1) {
        iq -= 1;
        tempReal = hsBuf[iq];
        if (tempReal > sufHi) {
          sufHi = tempReal;
        }
        hsBuf[iq] = sufHi;
        tempReal = lsBuf[iq];
        if (tempReal < sufLo) {
          sufLo = tempReal;
        }
        lsBuf[iq] = sufLo;
      }
    }
    outIdx += 1;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['multiplier', Core.REAL_DEFAULT, 0e0, Core.REAL_MAX, 'in 0..3e37'], ['stopPeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000']] as const;

/** @internal */
export function ckspPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  optInMultiplier: number,
  optInStopPeriod: number,
  outHighStop: Float64Array,
  outLowStop: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CKSP', 'startIdx', startIdx);
  endIdx = requireInt32('CKSP', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('CKSP', 'timePeriod', optInTimePeriod);
  optInMultiplier = requireReal('CKSP', 'multiplier', optInMultiplier);
  optInStopPeriod = requireInt32('CKSP', 'stopPeriod', optInStopPeriod);
  requireIndexRange('CKSP', startIdx, endIdx);
  const guardLookback = ckspLookbackPositional(optInTimePeriod, optInMultiplier, optInStopPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('CKSP', PARAM_RANGES, optInTimePeriod, optInMultiplier, optInStopPeriod);
  }
  const guardStart = clampedStart('CKSP', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CKSP', 'prices.high', inHigh, guardInLen);
  requireLength('CKSP', 'prices.low', inLow, guardInLen);
  requireLength('CKSP', 'prices.close', inClose, guardInLen);
  requireOutput('CKSP', 'outHighStop', outHighStop, guardOutLen, Float64Array);
  requireOutput('CKSP', 'outLowStop', outLowStop, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outHighStop, inHighF64)) {
    throw sharedMemory('CKSP', 'outHighStop', 'prices.high');
  }
  if (overlapsPartly(outHighStop, inLowF64)) {
    throw sharedMemory('CKSP', 'outHighStop', 'prices.low');
  }
  if (overlapsPartly(outHighStop, inCloseF64)) {
    throw sharedMemory('CKSP', 'outHighStop', 'prices.close');
  }
  if (overlapsPartly(outLowStop, inHighF64)) {
    throw sharedMemory('CKSP', 'outLowStop', 'prices.high');
  }
  if (overlapsPartly(outLowStop, inLowF64)) {
    throw sharedMemory('CKSP', 'outLowStop', 'prices.low');
  }
  if (overlapsPartly(outLowStop, inCloseF64)) {
    throw sharedMemory('CKSP', 'outLowStop', 'prices.close');
  }
  if (overlaps(outHighStop, outLowStop)) {
    throw sharedMemory('CKSP', 'outHighStop', 'outLowStop');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = ckspImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInTimePeriod, optInMultiplier, optInStopPeriod, outBegIdx, outNBElement, outHighStop, outLowStop);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CKSP', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cksp}, {@link ckspLookback}, {@link ckspDisplayShift},
 * {@link ckspOpen} and {@link ckspOpenAndFill}. Every key is optional: an omitted
 * or `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface CkspOptions {
  /**
   * Window of the highest high and lowest low, and smoothing period of the Average
   * True Range (default 10; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
   * default).
   */
  timePeriod?: number | undefined;
  /**
   * Multiplier applied to the Average True Range to offset the first stops (default
   * 1; minimum 0; `Core.REAL_DEFAULT` selects the default).
   */
  multiplier?: number | undefined;
  /**
   * Window over which each first stop takes its extreme (default 9; range 1 to
   * 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  stopPeriod?: number | undefined;
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
   * Buffer for `outHighStop`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outHighStop?: Float64Array | undefined;
  /**
   * Buffer for `outLowStop`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outLowStop?: Float64Array | undefined;
}

/** What {@link cksp} returns, and {@link ckspOpenAndFill} with its stream. */
export interface CkspResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Highest of the recent first high stops; usually plotted as the short stop.
   * `outHighStop[k]` is the value at input bar `begIdx + k`.
   */
  outHighStop: Float64Array;
  /**
   * Lowest of the recent first low stops; usually plotted as the long stop.
   * `outLowStop[k]` is the value at input bar `begIdx + k`.
   */
  outLowStop: Float64Array;
}

/**
 * Number of leading input bars {@link cksp} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CkspOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function ckspLookback(options?: CkspOptions): number {
  const o = optionsOf('CKSP lookback', options);
  return ckspLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.multiplier, Core.REAL_DEFAULT), orDefault(o.stopPeriod, Core.INTEGER_DEFAULT), coreOf('CKSP lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cksp}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CkspOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function ckspDisplayShift(outputIdx: number, options?: CkspOptions): number {
  const o = optionsOf('CKSP displayShift', options);
  return ckspDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.multiplier, Core.REAL_DEFAULT), orDefault(o.stopPeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('CKSP displayShift', o.core));
}

/**
 * Chande and Kroll's two-line volatility stop. The highest high is offset down,
 * and the lowest low up, by a multiple of the Average True Range, and each line
 * then takes the extreme of its own recent values, so it moves only when a new
 * extreme enters the stop window or an old one leaves it. Price above both lines
 * reads as an uptrend and price below both as a downtrend.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cksp](https://ta-lib.org/functions/cksp).
 *
 * **Notes**
 * - The two lines are named for how they are built, not for the position they
 *   protect, because published implementations give "long stop" and "short stop"
 *   to opposite lines. `outHighStop` is the line most platforms plot as the short
 *   stop, `outLowStop` the one they plot as the long stop.
 * - Neither line is always above the other: a large multiplier pushes the high
 *   line below the low one.
 * - The Average True Range is this library's, whose first value is the average of
 *   the first full period of true ranges that have a previous close. Platforms
 *   that start the true range on the very first bar give slightly different values
 *   on early bars; the difference decays as the average warms up.
 * - The book's own settings are reported as a multiplier of 3 and a stop period of
 *   20; the defaults here are the ones charting platforms ship.
 * - A stop period of 1 leaves the first stops unchanged, which is the Chandelier
 *   Exit on both sides.
 * - Both lines inherit the Average True Range's warm-up, so a caller who wants
 *   them converged sets `TA_FUNC_UNST_ATR`, exactly as when calling that function
 *   directly.
 *
 * `outHighStop[k]` is the value at input bar `begIdx + k`; every output is exactly
 * as long as the number of values written, and nothing is padded. A range that
 * ends before {@link ckspLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `high`: High
 *   price of each bar. `low`: Low price of each bar. `close`: Close price of each
 *   bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link CkspOptions}.
 * @returns `begIdx` and the values; see {@link CkspResult}. Each output is a view
 *   of its buffer, the caller's or a new one.
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
 * import { cksp } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { begIdx, outHighStop, outLowStop } = cksp({ high, low, close }, {
 *   timePeriod: 10,
 *   multiplier: 1,
 *   stopPeriod: 9,
 * });
 * // outHighStop[k] is the value at bar begIdx + k
 * ```
 * @see [`atr`](https://ta-lib.org/functions/atr)
 * @see [`max`](https://ta-lib.org/functions/max)
 * @see [`min`](https://ta-lib.org/functions/min)
 * @see [`supertrend`](https://ta-lib.org/functions/supertrend)
 * @see [`kc`](https://ta-lib.org/functions/kc)
 * @see [`donchian`](https://ta-lib.org/functions/donchian)
 */
export function cksp(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: CkspOptions): CkspResult {
  requireArgument('CKSP', 'prices', prices);
  const o = optionsOf('CKSP', options);
  if (o.endIdx === undefined) {
    requireSeries('CKSP', 'prices.high', prices.high);
    requireSeries('CKSP', 'prices.low', prices.low);
    requireSeries('CKSP', 'prices.close', prices.close);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInMultiplier = orDefault(o.multiplier, Core.REAL_DEFAULT);
  const optInStopPeriod = orDefault(o.stopPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('CKSP', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, ckspLookbackPositional(optInTimePeriod, optInMultiplier, optInStopPeriod, core), prices.high, prices.low, prices.close);
  const outHighStop = o.outHighStop === undefined ? new Float64Array(capacity) : o.outHighStop;
  const outLowStop = o.outLowStop === undefined ? new Float64Array(capacity) : o.outLowStop;
  const range = ckspPositional(startIdx, endIdx, prices.high, prices.low, prices.close, optInTimePeriod, optInMultiplier, optInStopPeriod, outHighStop, outLowStop, core);
  return { begIdx: range.begIdx, outHighStop: outHighStop.subarray(0, range.count), outLowStop: outLowStop.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CKSP stream: one value per closed bar, bit-identical to {@link cksp} over
 * the same series. Open it with {@link ckspOpen} or {@link ckspOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cksp](https://ta-lib.org/functions/cksp).
 */
export class CkspStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  optInMultiplier: number = 0;
  /** @internal */
  optInStopPeriod: number = 0;
  /** @internal */
  lastP: number = 0;
  /** @internal */
  lastQ: number = 0;
  /** @internal */
  prevATR: number = 0;
  /** @internal */
  wAlpha: number = 0;
  /** @internal */
  wBeta: number = 0;
  /** @internal */
  hhPre: number = 0;
  /** @internal */
  llPre: number = 0;
  /** @internal */
  hsPre: number = 0;
  /** @internal */
  lsPre: number = 0;
  /** @internal */
  hhBuf_Idx: number = 0;
  /** @internal */
  hsBuf_Idx: number = 0;
  /** @internal */
  maxIdx_hhBuf: number = 0;
  /** @internal */
  llBuf_Idx: number = 0;
  /** @internal */
  maxIdx_llBuf: number = 0;
  /** @internal */
  maxIdx_hsBuf: number = 0;
  /** @internal */
  lsBuf_Idx: number = 0;
  /** @internal */
  maxIdx_lsBuf: number = 0;
  /** @internal */
  lag1_inClose: number = 0;
  /** @internal */
  cbSize_hhBuf: number = 0;
  /** @internal */
  cb_hhBuf: Float64Array = new Float64Array(1);
  /** @internal */
  cbSize_llBuf: number = 0;
  /** @internal */
  cb_llBuf: Float64Array = new Float64Array(1);
  /** @internal */
  cbSize_hsBuf: number = 0;
  /** @internal */
  cb_hsBuf: Float64Array = new Float64Array(1);
  /** @internal */
  cbSize_lsBuf: number = 0;
  /** @internal */
  cb_lsBuf: Float64Array = new Float64Array(1);
  /** @internal */
  cur_outHighStop: number = 0;
  /** @internal */
  cur_outLowStop: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): CkspStream {
    return new CkspStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cksp} reports them over the same bars. Each accepted `update` and each
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
      throw failure('CKSP advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns its outputs.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param out - Receives the outputs and is returned; a new {@link CkspOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number, inClose: number, out?: CkspOut): CkspOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('CKSP update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('CKSP update', 'CkspOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('CKSP update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    ckspStepImpl(this, inHigh, inLow, inClose);
    this.outRangeCount++;
    if (out === undefined) {
      out = new CkspOut();
    }
    out.outHighStop = this.cur_outHighStop;
    out.outLowStop = this.cur_outLowStop;
    return out;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param out - Receives the outputs and is returned; a new {@link CkspOut} when
   *   omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number, inClose: number, out?: CkspOut): CkspOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('CKSP peek', 'CkspOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('CKSP peek', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let val2 = 0;
    let val3 = 0;
    let greatest = 0;
    let tempCY = 0;
    let tempLT = 0;
    let tempHT = 0;
    let tempReal = 0;
    let highest = 0;
    let lowest = 0;
    let band = 0;
    let highStop = 0;
    let lowStop = 0;
    let cur_outHighStop = 0;
    let cur_outLowStop = 0;
    let hhBuf_Idx = sp.hhBuf_Idx;
    let hhPre = sp.hhPre;
    let hsBuf_Idx = sp.hsBuf_Idx;
    let hsPre = sp.hsPre;
    let llPre = sp.llPre;
    let lsPre = sp.lsPre;
    let prevATR = sp.prevATR;
    tempLT = inLow;
    tempHT = inHigh;
    tempCY = sp.lag1_inClose;
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
    prevATR = fma(sp.wBeta, prevATR, sp.wAlpha * greatest);
    highest = hhPre;
    lowest = llPre;
    if (tempHT > highest) {
      highest = tempHT;
    }
    if (tempLT < lowest) {
      lowest = tempLT;
    }
    if (hhBuf_Idx === 0) {
      highest = tempHT;
      lowest = tempLT;
    }
    hhPre = highest;
    llPre = lowest;
    if (hhBuf_Idx < sp.lastP) {
      tempReal = sp.cb_hhBuf[hhBuf_Idx + 1];
      if (tempReal > highest) {
        highest = tempReal;
      }
      tempReal = sp.cb_llBuf[hhBuf_Idx + 1];
      if (tempReal < lowest) {
        lowest = tempReal;
      }
    }
    hhBuf_Idx = hhBuf_Idx + 1;
    if (hhBuf_Idx > sp.maxIdx_hhBuf) {
      hhBuf_Idx = 0;
    }
    band = sp.optInMultiplier * prevATR;
    highStop = highest - band;
    lowStop = lowest + band;
    highest = hsPre;
    lowest = lsPre;
    if (highStop > highest) {
      highest = highStop;
    }
    if (lowStop < lowest) {
      lowest = lowStop;
    }
    if (hsBuf_Idx === 0) {
      highest = highStop;
      lowest = lowStop;
    }
    hsPre = highest;
    lsPre = lowest;
    if (hsBuf_Idx < sp.lastQ) {
      tempReal = sp.cb_hsBuf[hsBuf_Idx + 1];
      if (tempReal > highest) {
        highest = tempReal;
      }
      tempReal = sp.cb_lsBuf[hsBuf_Idx + 1];
      if (tempReal < lowest) {
        lowest = tempReal;
      }
    }
    hsBuf_Idx = hsBuf_Idx + 1;
    if (hsBuf_Idx > sp.maxIdx_hsBuf) {
      hsBuf_Idx = 0;
    }
    cur_outHighStop = highest;
    cur_outLowStop = lowest;
    if (out === undefined) {
      out = new CkspOut();
    }
    out.outHighStop = cur_outHighStop;
    out.outLowStop = cur_outLowStop;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link CkspOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   */
  value(out?: CkspOut): CkspOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('CKSP value', 'CkspOut');
    }
    if (out === undefined) {
      out = new CkspOut();
    }
    out.outHighStop = this.cur_outHighStop;
    out.outLowStop = this.cur_outLowStop;
    return out;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): CkspStream {
    const c = new CkspStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.optInMultiplier = this.optInMultiplier;
    c.optInStopPeriod = this.optInStopPeriod;
    c.lastP = this.lastP;
    c.lastQ = this.lastQ;
    c.prevATR = this.prevATR;
    c.wAlpha = this.wAlpha;
    c.wBeta = this.wBeta;
    c.hhPre = this.hhPre;
    c.llPre = this.llPre;
    c.hsPre = this.hsPre;
    c.lsPre = this.lsPre;
    c.hhBuf_Idx = this.hhBuf_Idx;
    c.hsBuf_Idx = this.hsBuf_Idx;
    c.maxIdx_hhBuf = this.maxIdx_hhBuf;
    c.llBuf_Idx = this.llBuf_Idx;
    c.maxIdx_llBuf = this.maxIdx_llBuf;
    c.maxIdx_hsBuf = this.maxIdx_hsBuf;
    c.lsBuf_Idx = this.lsBuf_Idx;
    c.maxIdx_lsBuf = this.maxIdx_lsBuf;
    c.lag1_inClose = this.lag1_inClose;
    c.cbSize_hhBuf = this.cbSize_hhBuf;
    c.cb_hhBuf = this.cb_hhBuf.slice();
    c.cbSize_llBuf = this.cbSize_llBuf;
    c.cb_llBuf = this.cb_llBuf.slice();
    c.cbSize_hsBuf = this.cbSize_hsBuf;
    c.cb_hsBuf = this.cb_hsBuf.slice();
    c.cbSize_lsBuf = this.cbSize_lsBuf;
    c.cb_lsBuf = this.cb_lsBuf.slice();
    c.cur_outHighStop = this.cur_outHighStop;
    c.cur_outLowStop = this.cur_outLowStop;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one CKSP bar, as `update`, `peek` and `value` return them. Passed
 * back to one of them, it is filled and returned again, with no allocation; it
 * holds its values until the next call that writes it. A new one holds `NaN`.
 */
export class CkspOut {
  /** Highest of the recent first high stops; usually plotted as the short stop. */
  outHighStop: number = NaN;
  /** Lowest of the recent first low stops; usually plotted as the long stop. */
  outLowStop: number = NaN;
}

function ckspStepImpl(sp: CkspStream, inHigh: number, inLow: number, inClose: number): void {
  let ip = 0;
  let iq = 0;
  let val2 = 0;
  let val3 = 0;
  let greatest = 0;
  let tempCY = 0;
  let tempLT = 0;
  let tempHT = 0;
  let tempReal = 0;
  let sufHi = 0;
  let sufLo = 0;
  let highest = 0;
  let lowest = 0;
  let band = 0;
  let highStop = 0;
  let lowStop = 0;
  tempLT = inLow;
  tempHT = inHigh;
  tempCY = sp.lag1_inClose;
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
  sp.prevATR = fma(sp.wBeta, sp.prevATR, sp.wAlpha * greatest);
  highest = sp.hhPre;
  lowest = sp.llPre;
  if (tempHT > highest) {
    highest = tempHT;
  }
  if (tempLT < lowest) {
    lowest = tempLT;
  }
  if (sp.hhBuf_Idx === 0) {
    highest = tempHT;
    lowest = tempLT;
  }
  sp.hhPre = highest;
  sp.llPre = lowest;
  if (sp.hhBuf_Idx < sp.lastP) {
    tempReal = sp.cb_hhBuf[sp.hhBuf_Idx + 1];
    if (tempReal > highest) {
      highest = tempReal;
    }
    tempReal = sp.cb_llBuf[sp.hhBuf_Idx + 1];
    if (tempReal < lowest) {
      lowest = tempReal;
    }
  }
  sp.cb_hhBuf[sp.hhBuf_Idx] = tempHT;
  sp.cb_llBuf[sp.hhBuf_Idx] = tempLT;
  sp.hhBuf_Idx = sp.hhBuf_Idx + 1;
  if (sp.hhBuf_Idx > sp.maxIdx_hhBuf) {
    sp.hhBuf_Idx = 0;
  }
  band = sp.optInMultiplier * sp.prevATR;
  highStop = highest - band;
  lowStop = lowest + band;
  highest = sp.hsPre;
  lowest = sp.lsPre;
  if (highStop > highest) {
    highest = highStop;
  }
  if (lowStop < lowest) {
    lowest = lowStop;
  }
  if (sp.hsBuf_Idx === 0) {
    highest = highStop;
    lowest = lowStop;
  }
  sp.hsPre = highest;
  sp.lsPre = lowest;
  if (sp.hsBuf_Idx < sp.lastQ) {
    tempReal = sp.cb_hsBuf[sp.hsBuf_Idx + 1];
    if (tempReal > highest) {
      highest = tempReal;
    }
    tempReal = sp.cb_lsBuf[sp.hsBuf_Idx + 1];
    if (tempReal < lowest) {
      lowest = tempReal;
    }
  }
  sp.cb_hsBuf[sp.hsBuf_Idx] = highStop;
  sp.cb_lsBuf[sp.hsBuf_Idx] = lowStop;
  sp.hsBuf_Idx = sp.hsBuf_Idx + 1;
  if (sp.hsBuf_Idx > sp.maxIdx_hsBuf) {
    sp.hsBuf_Idx = 0;
  }
  sp.cur_outHighStop = highest;
  sp.cur_outLowStop = lowest;
  /* A completed block becomes its suffix extrema. Nothing this bar reads
   * them, so both passes stay below the output stores, where the peek
   * frame never runs them.
   */
  if (sp.hhBuf_Idx === 0) {
    sufHi = sp.cb_hhBuf[sp.lastP];
    sufLo = sp.cb_llBuf[sp.lastP];
    ip = sp.lastP;
    while (ip > 1) {
      ip -= 1;
      tempReal = sp.cb_hhBuf[ip];
      if (tempReal > sufHi) {
        sufHi = tempReal;
      }
      sp.cb_hhBuf[ip] = sufHi;
      tempReal = sp.cb_llBuf[ip];
      if (tempReal < sufLo) {
        sufLo = tempReal;
      }
      sp.cb_llBuf[ip] = sufLo;
    }
  }
  if (sp.hsBuf_Idx === 0) {
    sufHi = sp.cb_hsBuf[sp.lastQ];
    sufLo = sp.cb_lsBuf[sp.lastQ];
    iq = sp.lastQ;
    while (iq > 1) {
      iq -= 1;
      tempReal = sp.cb_hsBuf[iq];
      if (tempReal > sufHi) {
        sufHi = tempReal;
      }
      sp.cb_hsBuf[iq] = sufHi;
      tempReal = sp.cb_lsBuf[iq];
      if (tempReal < sufLo) {
        sufLo = tempReal;
      }
      sp.cb_lsBuf[iq] = sufLo;
    }
  }
  sp.lag1_inClose = inClose;
}

function ckspOpenImpl(
  sp: CkspStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInMultiplier: number,
  optInStopPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outHighStop: Float64Array,
  outLowStop: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let hhBuf!: Float64Array;
  let hhBuf_Idx = 0;
  let maxIdx_hhBuf = (30) - 1;
  let llBuf!: Float64Array;
  let llBuf_Idx = 0;
  let maxIdx_llBuf = (30) - 1;
  let hsBuf!: Float64Array;
  let hsBuf_Idx = 0;
  let maxIdx_hsBuf = (30) - 1;
  let lsBuf!: Float64Array;
  let lsBuf_Idx = 0;
  let maxIdx_lsBuf = (30) - 1;
  let i = 0;
  let ip = 0;
  let iq = 0;
  let today = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let lastP = 0;
  let lastQ = 0;
  let prevATR = 0;
  let periodTotal = 0;
  let wAlpha = 0;
  let wBeta = 0;
  let val2 = 0;
  let val3 = 0;
  let greatest = 0;
  let tempCY = 0;
  let tempLT = 0;
  let tempHT = 0;
  let tempReal = 0;
  let hhPre = 0;
  let llPre = 0;
  let hsPre = 0;
  let lsPre = 0;
  let sufHi = 0;
  let sufLo = 0;
  let highest = 0;
  let lowest = 0;
  let band = 0;
  let highStop = 0;
  let lowStop = 0;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length || inClose.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 10;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInMultiplier === Core.REAL_DEFAULT) {
    optInMultiplier = 1.0;
  } else if (!(optInMultiplier >= 0e0 && optInMultiplier <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInStopPeriod === Core.INTEGER_DEFAULT) {
    optInStopPeriod = 9;
  } else if (optInStopPeriod < 1 || optInStopPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = ckspLookbackPositional(optInTimePeriod, optInMultiplier, optInStopPeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Four rolling extrema, van Herk / Gil-Werman, one buffer each: the window
   * ending in slot j is the current block's prefix extremum joined with the
   * previous block's suffix extremum from slot j+1. A slot holds the raw value
   * while its block is filling and is turned into the suffix extremum, in
   * place, when the block completes; slot j+1 is always still the previous
   * block's when slot j is filled. The high and low sides share an index.
   *
   * The extrema are exact, so both outputs must stay bit-identical to
   * TA_MAX( TA_MAX(high) - x*TA_ATR ) and its mirror, whatever the block
   * phase; only the sign of a zero tied between +0.0 and -0.0 is free.
   */
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  hhBuf = new Float64Array(optInTimePeriod);
  maxIdx_hhBuf = optInTimePeriod - 1;
  hhBuf_Idx = 0;
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  llBuf = new Float64Array(optInTimePeriod);
  maxIdx_llBuf = optInTimePeriod - 1;
  llBuf_Idx = 0;
  if (optInStopPeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  hsBuf = new Float64Array(optInStopPeriod);
  maxIdx_hsBuf = optInStopPeriod - 1;
  hsBuf_Idx = 0;
  if (optInStopPeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  lsBuf = new Float64Array(optInStopPeriod);
  maxIdx_lsBuf = optInStopPeriod - 1;
  lsBuf_Idx = 0;
  lastP = optInTimePeriod - 1;
  lastQ = optInStopPeriod - 1;
  /* The Average True Range is carried inline rather than taken from a call,
   * because the two stages advance together one bar at a time and a
   * whole-range buffer between them would not stream.
   *
   * The arithmetic order below is the bit-exactness contract with TA_ATR (do
   * not reorder): True Range from high-low, then the two previous-close
   * distances in that order; the seed summed from 0.0 over the first 'period'
   * True Ranges and divided once; the same two Wilder coefficients, wBeta
   * rounded and wAlpha derived from it, in one fused statement.
   */
  wBeta = (optInTimePeriod - 1) / optInTimePeriod;
  wAlpha = 1.0 - wBeta;
  today = startIdx - lookbackTotal + 1;
  periodTotal = 0.0;
  i = optInTimePeriod;
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
  prevATR = periodTotal / optInTimePeriod;
  /* Skip the bars the lookback adds for the unstable period. Taking the count
   * from the lookback rather than naming the setting keeps the two from
   * disagreeing.
   */
  i = lookbackTotal - lastQ - optInTimePeriod;
  while (i !== 0) {
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
    i -= 1;
  }
  /* prevATR is now the Average True Range of bar today-1, the first bar with
   * a first stop. Its extreme window is one whole block, taken straight from
   * the input.
   */
  highest = inHigh[today - 1];
  lowest = inLow[today - 1];
  hhBuf[lastP] = highest;
  llBuf[lastP] = lowest;
  ip = lastP;
  while (ip > 0) {
    ip -= 1;
    tempHT = inHigh[today - 1 - lastP + ip];
    tempLT = inLow[today - 1 - lastP + ip];
    if (tempHT > highest) {
      highest = tempHT;
    }
    if (tempLT < lowest) {
      lowest = tempLT;
    }
    hhBuf[ip] = highest;
    llBuf[ip] = lowest;
  }
  hhPre = highest;
  llPre = lowest;
  /* The multiple of the ATR is formed on its own, never fused into the
   * offset, so that each first stop is TA_MAX - x*TA_ATR (or its mirror) as a
   * caller composing the three functions would compute it.
   */
  band = optInMultiplier * prevATR;
  highStop = highest - band;
  lowStop = lowest + band;
  hsPre = highStop;
  lsPre = lowStop;
  hsBuf[hsBuf_Idx] = highStop;
  lsBuf[hsBuf_Idx] = lowStop;
  hsBuf_Idx++;
  if (hsBuf_Idx > maxIdx_hsBuf) {
    hsBuf_Idx = 0;
  }
  /* Fill the first stop window, through startIdx inclusive. */
  while (today <= startIdx) {
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
    /* Keep this shape: the prefix extreme selected in a local, the
     * block-start override after it, one store. A store made only when the
     * bar sets a new extreme, or the override ahead of the select, compiles
     * to a data-dependent branch in the stream step, where the prefix lives
     * in the handle.
     */
    highest = hhPre;
    lowest = llPre;
    if (tempHT > highest) {
      highest = tempHT;
    }
    if (tempLT < lowest) {
      lowest = tempLT;
    }
    if (hhBuf_Idx === 0) {
      highest = tempHT;
      lowest = tempLT;
    }
    hhPre = highest;
    llPre = lowest;
    if (hhBuf_Idx < lastP) {
      tempReal = hhBuf[hhBuf_Idx + 1];
      if (tempReal > highest) {
        highest = tempReal;
      }
      tempReal = llBuf[hhBuf_Idx + 1];
      if (tempReal < lowest) {
        lowest = tempReal;
      }
    }
    hhBuf[hhBuf_Idx] = tempHT;
    llBuf[hhBuf_Idx] = tempLT;
    hhBuf_Idx++;
    if (hhBuf_Idx > maxIdx_hhBuf) {
      hhBuf_Idx = 0;
    }
    if (hhBuf_Idx === 0) {
      sufHi = hhBuf[lastP];
      sufLo = llBuf[lastP];
      ip = lastP;
      while (ip > 1) {
        ip -= 1;
        tempReal = hhBuf[ip];
        if (tempReal > sufHi) {
          sufHi = tempReal;
        }
        hhBuf[ip] = sufHi;
        tempReal = llBuf[ip];
        if (tempReal < sufLo) {
          sufLo = tempReal;
        }
        llBuf[ip] = sufLo;
      }
    }
    band = optInMultiplier * prevATR;
    highStop = highest - band;
    lowStop = lowest + band;
    if (highStop > hsPre) {
      hsPre = highStop;
    }
    if (lowStop < lsPre) {
      lsPre = lowStop;
    }
    hsBuf[hsBuf_Idx] = highStop;
    lsBuf[hsBuf_Idx] = lowStop;
    hsBuf_Idx++;
    if (hsBuf_Idx > maxIdx_hsBuf) {
      hsBuf_Idx = 0;
    }
    today += 1;
  }
  /* The first stop window is one whole block as well: its extremes are the
   * prefix extremes, and the block is complete.
   */
  sufHi = hsBuf[lastQ];
  sufLo = lsBuf[lastQ];
  iq = lastQ;
  while (iq > 1) {
    iq -= 1;
    tempReal = hsBuf[iq];
    if (tempReal > sufHi) {
      sufHi = tempReal;
    }
    hsBuf[iq] = sufHi;
    tempReal = lsBuf[iq];
    if (tempReal < sufLo) {
      sufLo = tempReal;
    }
    lsBuf[iq] = sufLo;
  }
  outHighStop[((0 * outStride) | 0)] = hsPre;
  outLowStop[((0 * outStride) | 0)] = lsPre;
  outIdx = 1;
  while (today <= endIdx) {
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
    highest = hhPre;
    lowest = llPre;
    if (tempHT > highest) {
      highest = tempHT;
    }
    if (tempLT < lowest) {
      lowest = tempLT;
    }
    if (hhBuf_Idx === 0) {
      highest = tempHT;
      lowest = tempLT;
    }
    hhPre = highest;
    llPre = lowest;
    if (hhBuf_Idx < lastP) {
      tempReal = hhBuf[hhBuf_Idx + 1];
      if (tempReal > highest) {
        highest = tempReal;
      }
      tempReal = llBuf[hhBuf_Idx + 1];
      if (tempReal < lowest) {
        lowest = tempReal;
      }
    }
    hhBuf[hhBuf_Idx] = tempHT;
    llBuf[hhBuf_Idx] = tempLT;
    hhBuf_Idx++;
    if (hhBuf_Idx > maxIdx_hhBuf) {
      hhBuf_Idx = 0;
    }
    band = optInMultiplier * prevATR;
    highStop = highest - band;
    lowStop = lowest + band;
    highest = hsPre;
    lowest = lsPre;
    if (highStop > highest) {
      highest = highStop;
    }
    if (lowStop < lowest) {
      lowest = lowStop;
    }
    if (hsBuf_Idx === 0) {
      highest = highStop;
      lowest = lowStop;
    }
    hsPre = highest;
    lsPre = lowest;
    if (hsBuf_Idx < lastQ) {
      tempReal = hsBuf[hsBuf_Idx + 1];
      if (tempReal > highest) {
        highest = tempReal;
      }
      tempReal = lsBuf[hsBuf_Idx + 1];
      if (tempReal < lowest) {
        lowest = tempReal;
      }
    }
    hsBuf[hsBuf_Idx] = highStop;
    lsBuf[hsBuf_Idx] = lowStop;
    hsBuf_Idx++;
    if (hsBuf_Idx > maxIdx_hsBuf) {
      hsBuf_Idx = 0;
    }
    outHighStop[((outIdx * outStride) | 0)] = highest;
    outLowStop[((outIdx * outStride) | 0)] = lowest;
    /* A completed block becomes its suffix extrema. Nothing this bar reads
     * them, so both passes stay below the output stores, where the peek
     * frame never runs them.
     */
    if (hhBuf_Idx === 0) {
      sufHi = hhBuf[lastP];
      sufLo = llBuf[lastP];
      ip = lastP;
      while (ip > 1) {
        ip -= 1;
        tempReal = hhBuf[ip];
        if (tempReal > sufHi) {
          sufHi = tempReal;
        }
        hhBuf[ip] = sufHi;
        tempReal = llBuf[ip];
        if (tempReal < sufLo) {
          sufLo = tempReal;
        }
        llBuf[ip] = sufLo;
      }
    }
    if (hsBuf_Idx === 0) {
      sufHi = hsBuf[lastQ];
      sufLo = lsBuf[lastQ];
      iq = lastQ;
      while (iq > 1) {
        iq -= 1;
        tempReal = hsBuf[iq];
        if (tempReal > sufHi) {
          sufHi = tempReal;
        }
        hsBuf[iq] = sufHi;
        tempReal = lsBuf[iq];
        if (tempReal < sufLo) {
          sufLo = tempReal;
        }
        lsBuf[iq] = sufLo;
      }
    }
    outIdx += 1;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  const capCb_hhBuf = maxIdx_hhBuf + 1;
  if (capCb_hhBuf > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  const capCb_llBuf = maxIdx_llBuf + 1;
  if (capCb_llBuf > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  const capCb_hsBuf = maxIdx_hsBuf + 1;
  if (capCb_hsBuf > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  const capCb_lsBuf = maxIdx_lsBuf + 1;
  if (capCb_lsBuf > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.optInMultiplier = optInMultiplier;
  sp.optInStopPeriod = optInStopPeriod;
  sp.lastP = lastP;
  sp.lastQ = lastQ;
  sp.prevATR = prevATR;
  sp.wAlpha = wAlpha;
  sp.wBeta = wBeta;
  sp.hhPre = hhPre;
  sp.llPre = llPre;
  sp.hsPre = hsPre;
  sp.lsPre = lsPre;
  sp.hhBuf_Idx = hhBuf_Idx;
  sp.hsBuf_Idx = hsBuf_Idx;
  sp.maxIdx_hhBuf = maxIdx_hhBuf;
  sp.llBuf_Idx = llBuf_Idx;
  sp.maxIdx_llBuf = maxIdx_llBuf;
  sp.maxIdx_hsBuf = maxIdx_hsBuf;
  sp.lsBuf_Idx = lsBuf_Idx;
  sp.maxIdx_lsBuf = maxIdx_lsBuf;
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.cbSize_hhBuf = capCb_hhBuf;
  sp.cb_hhBuf = hhBuf;
  sp.cbSize_llBuf = capCb_llBuf;
  sp.cb_llBuf = llBuf;
  sp.cbSize_hsBuf = capCb_hsBuf;
  sp.cb_hsBuf = hsBuf;
  sp.cbSize_lsBuf = capCb_lsBuf;
  sp.cb_lsBuf = lsBuf;
  sp.cur_outHighStop = outHighStop[(outNBElement.value - 1) * outStride];
  sp.cur_outLowStop = outLowStop[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function ckspOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInMultiplier: number,
  optInStopPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outHighStop: Float64Array,
  outLowStop: Float64Array,
): CkspStream {
  const sp = CkspStream.of(core);
  const retCode = ckspOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, optInMultiplier, optInStopPeriod, outBegIdx, outNBElement, outHighStop, outLowStop, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CKSP openAndFill', inHigh.length, startIdx, ckspLookbackPositional(optInTimePeriod, optInMultiplier, optInStopPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && ckspLookbackPositional(optInTimePeriod, optInMultiplier, optInStopPeriod, core) < 0) {
    throw rejectedParams('CKSP openAndFill', PARAM_RANGES, optInTimePeriod, optInMultiplier, optInStopPeriod);
  }
  throw streamFailure('CKSP openAndFill', retCode);
}

/** @internal */
export function ckspOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInMultiplier: number,
  optInStopPeriod: number,
): CkspStream {
  const sp = CkspStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outHighStop = new Float64Array(1);
  const sink_outLowStop = new Float64Array(1);
  const retCode = ckspOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, optInMultiplier, optInStopPeriod, outBegIdx, outNBElement, sink_outHighStop, sink_outLowStop, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CKSP open', inHigh.length, startIdx, ckspLookbackPositional(optInTimePeriod, optInMultiplier, optInStopPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && ckspLookbackPositional(optInTimePeriod, optInMultiplier, optInStopPeriod, core) < 0) {
    throw rejectedParams('CKSP open', PARAM_RANGES, optInTimePeriod, optInMultiplier, optInStopPeriod);
  }
  throw streamFailure('CKSP open', retCode);
}

/** @internal */
export function ckspOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  optInMultiplier: number,
  optInStopPeriod: number,
  core: Core = Core.DEFAULT,
): CkspStream {
  optInTimePeriod = requireInt32('CKSP open', 'timePeriod', optInTimePeriod);
  optInMultiplier = requireReal('CKSP open', 'multiplier', optInMultiplier);
  optInStopPeriod = requireInt32('CKSP open', 'stopPeriod', optInStopPeriod);
  requireArgument('CKSP open', 'prices.high', inHigh);
  requireHistory('CKSP open', inHigh.length);
  requireArgument('CKSP open', 'prices.low', inLow);
  requireArgument('CKSP open', 'prices.close', inClose);
  requireHistoryLength('CKSP open', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('CKSP open', 'prices.close', inClose.length, inHigh.length);
  return ckspOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInTimePeriod, optInMultiplier, optInStopPeriod);
}

/** @internal */
export function ckspOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  optInMultiplier: number,
  optInStopPeriod: number,
  outHighStop: Float64Array,
  outLowStop: Float64Array,
  core: Core = Core.DEFAULT,
): CkspStream {
  optInTimePeriod = requireInt32('CKSP openAndFill', 'timePeriod', optInTimePeriod);
  optInMultiplier = requireReal('CKSP openAndFill', 'multiplier', optInMultiplier);
  optInStopPeriod = requireInt32('CKSP openAndFill', 'stopPeriod', optInStopPeriod);
  requireArgument('CKSP openAndFill', 'prices.high', inHigh);
  requireHistory('CKSP openAndFill', inHigh.length);
  requireArgument('CKSP openAndFill', 'prices.low', inLow);
  requireArgument('CKSP openAndFill', 'prices.close', inClose);
  const guardLookback = ckspLookbackPositional(optInTimePeriod, optInMultiplier, optInStopPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('CKSP openAndFill', PARAM_RANGES, optInTimePeriod, optInMultiplier, optInStopPeriod);
  }
  const guardOutLen = openFillCount('CKSP openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('CKSP openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('CKSP openAndFill', 'prices.close', inClose.length, inHigh.length);
  requireOutput('CKSP openAndFill', 'outHighStop', outHighStop, guardOutLen, Float64Array);
  requireOutput('CKSP openAndFill', 'outLowStop', outLowStop, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outHighStop.buffer === inHighF64.buffer && outHighStop.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CKSP openAndFill', 'outHighStop', 'prices.high');
  }
  if (outHighStop.buffer === inLowF64.buffer && outHighStop.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CKSP openAndFill', 'outHighStop', 'prices.low');
  }
  if (outHighStop.buffer === inCloseF64.buffer && outHighStop.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('CKSP openAndFill', 'outHighStop', 'prices.close');
  }
  if (outLowStop.buffer === inHighF64.buffer && outLowStop.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CKSP openAndFill', 'outLowStop', 'prices.high');
  }
  if (outLowStop.buffer === inLowF64.buffer && outLowStop.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CKSP openAndFill', 'outLowStop', 'prices.low');
  }
  if (outLowStop.buffer === inCloseF64.buffer && outLowStop.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('CKSP openAndFill', 'outLowStop', 'prices.close');
  }
  if (outHighStop.buffer === outLowStop.buffer && outHighStop.byteOffset === outLowStop.byteOffset) {
    throw sharedMemory('CKSP openAndFill', 'outHighStop', 'outLowStop');
  }
  if (guardOutLen > 0 && overlaps(outHighStop, inHighF64)) {
    throw sharedMemory('CKSP openAndFill', 'outHighStop', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outHighStop, inLowF64)) {
    throw sharedMemory('CKSP openAndFill', 'outHighStop', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outHighStop, inCloseF64)) {
    throw sharedMemory('CKSP openAndFill', 'outHighStop', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outLowStop, inHighF64)) {
    throw sharedMemory('CKSP openAndFill', 'outLowStop', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outLowStop, inLowF64)) {
    throw sharedMemory('CKSP openAndFill', 'outLowStop', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outLowStop, inCloseF64)) {
    throw sharedMemory('CKSP openAndFill', 'outLowStop', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outHighStop, outLowStop)) {
    throw sharedMemory('CKSP openAndFill', 'outHighStop', 'outLowStop');
  }
  return ckspOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInTimePeriod, optInMultiplier, optInStopPeriod, new MInteger(), new MInteger(), outHighStop, outLowStop);
}

/**
 * Opens a live CKSP stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link cksp} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CkspOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `ckspLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a real parameter that is not a
 *   number, a parameter outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { ckspOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = ckspOpen({ high, low, close }, {
 *   timePeriod: 10,
 *   multiplier: 1,
 *   stopPeriod: 9,
 * });
 * const peeked = stream.peek(101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function ckspOpen(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: CkspOptions): CkspStream {
  requireArgument('CKSP open', 'prices', prices);
  const o = optionsOf('CKSP open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInMultiplier = orDefault(o.multiplier, Core.REAL_DEFAULT);
  const optInStopPeriod = orDefault(o.stopPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('CKSP open', o.core);
  return ckspOpenPositional(prices.high, prices.low, prices.close, optInTimePeriod, optInMultiplier, optInStopPeriod, core);
}

/**
 * {@link ckspOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link cksp}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CkspOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cksp} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `ckspLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a real parameter that is not a
 *   number, a parameter outside its range, histories of different lengths or a
 *   buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { ckspOpenAndFill } from 'ta-lib';
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
 *   outHighStop,
 *   outLowStop,
 * } = ckspOpenAndFill({ high, low, close }, {
 *   timePeriod: 10,
 *   multiplier: 1,
 *   stopPeriod: 9,
 * });
 * // outHighStop[k] is the value at bar begIdx + k
 * ```
 */
export function ckspOpenAndFill(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: CkspOptions): CkspResult & { stream: CkspStream } {
  requireArgument('CKSP openAndFill', 'prices', prices);
  const o = optionsOf('CKSP openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInMultiplier = orDefault(o.multiplier, Core.REAL_DEFAULT);
  const optInStopPeriod = orDefault(o.stopPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('CKSP openAndFill', o.core);
  const capacity = fillCapacity(ckspLookbackPositional(optInTimePeriod, optInMultiplier, optInStopPeriod, core), prices.high, prices.low, prices.close);
  const outHighStop = o.outHighStop === undefined ? new Float64Array(capacity) : o.outHighStop;
  const outLowStop = o.outLowStop === undefined ? new Float64Array(capacity) : o.outLowStop;
  const stream = ckspOpenAndFillPositional(prices.high, prices.low, prices.close, optInTimePeriod, optInMultiplier, optInStopPeriod, outHighStop, outLowStop, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outHighStop: outHighStop.subarray(0, range.count), outLowStop: outLowStop.subarray(0, range.count) };
}
