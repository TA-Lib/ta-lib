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
 *  071626 MF,CC  Initial version (#119).
 *  092726 MF,CC  0 on a slow window of zero bars for the windowed MA types (#454).
 *  092826 MF,CC  #459 fuse the fast and slow SMA into one pass over the input:
 *                two running sums, no intermediate buffer, no allocation.
 *                Bit-identical.
 *  092826 MF,CC  Fuse the fast and slow EMA into one pass (#459).
 */

import { MAType, MATypes } from '../enums.ts';
import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { emaLookbackPositional } from './ema.ts';
import { MaStream, maLookbackPositional, maOpenAndFillInternal, maPositional } from './ma.ts';

/** @internal */
export function pvoLookbackPositional(optInFastPeriod: number, optInSlowPeriod: number, optInMAType: MAType | typeof Core.INTEGER_DEFAULT, core: Core = Core.DEFAULT): number {
  if ((optInFastPeriod | 0) !== optInFastPeriod) {
    return -1;
  }
  if ((optInSlowPeriod | 0) !== optInSlowPeriod) {
    return -1;
  }
  if ((optInMAType | 0) !== optInMAType) {
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
  if (optInMAType === Core.INTEGER_DEFAULT || optInMAType === MAType.DEFAULT) {
    optInMAType = MAType.EMA;
  } else if (optInMAType < MATypes.Min || optInMAType > MATypes.Max) {
    return -1;
  }
  /* Lookback is driven by the slowest MA. */
  return maLookbackPositional((optInSlowPeriod > optInFastPeriod ? optInSlowPeriod : optInFastPeriod), (optInMAType as MAType), core);
}

/** @internal */
export function pvoDisplayShiftPositional(optInFastPeriod: number, optInSlowPeriod: number, optInMAType: MAType | typeof Core.INTEGER_DEFAULT, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (pvoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInMAType, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function pvoImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inVolume: Float64Array,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInMAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let tempBuffer!: Float64Array;
  let tempReal = 0;
  let tempInteger = 0;
  const fastBeg = new MInteger();
  const fastNb = new MInteger();
  let offset = 0;
  let slowLookback = 0;
  let windowed = 0;
  let zeroRun = 0;
  let i = 0;
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
  if (optInMAType === Core.INTEGER_DEFAULT || optInMAType === MAType.DEFAULT) {
    optInMAType = MAType.EMA;
  } else if (optInMAType < MATypes.Min || optInMAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  /* Nothing to produce: the range ends before the lookback. Return before
   * touching anything.
   *
   * Without this the fast MA below runs first, and its lookback is SMALLER
   * than pvo's own — so it reads the whole range and computes a result the
   * empty slow MA then discards. Observably identical (the slow MA's own early
   * return already yields 0,0 here), but it is the difference between "a range
   * that ends before the lookback reads nothing" being true of this function and
   * being false: with a caller-supplied inVolume that stops short of endIdx, that
   * discarded work is an out-of-bounds read. Pinned by the zero-length no-I/O
   * probe over every guarded core.
   */
  if (maLookbackPositional((optInSlowPeriod > optInFastPeriod ? optInSlowPeriod : optInFastPeriod), (optInMAType as MAType), core) > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  if (optInMAType === MAType.SMA) {
    /* SMA fast path: the fast window is the newest optInFastPeriod bars of the
     * slow one, so ONE pass over the input serves both moving averages - two
     * running sums, no intermediate buffer and no allocation, where the general
     * path below makes two passes and allocates the fast MA in full.
     *
     * Bit-identical to that path. Each sum sees exactly the add/subtract
     * sequence TA_SMA gives it at its own period, starting from its own first
     * output bar - which is why the fast sum is walked alone over the bars the
     * slow MA does not reach (its running total is path-dependent, so arriving
     * at the first output bar by a shorter route would change the low bits) -
     * and each quotient is formed as sma.c forms it: the total AFTER adding the
     * new bar and BEFORE dropping the trailing one, divided by the period.
     *
     * SMA is one of the windowed types, so the dead-window rule of #454 applies
     * here too: _zeroRun is the same counter the general path keeps, warmed over
     * the same bars (which are the bars the slow sum is seeded from) and held at
     * the slow lookback once the window is dead.
     *
     * inVolume may alias outReal, as it may in the general path. outReal[_outIdx]
     * is written at bar _i with _outIdx <= _i-optInSlowPeriod+1 <= both trailing
     * indices, and every read of bar _i happens before that write, so no bar is
     * overwritten before its last read.
     *
     * Every read is inside [0, endIdx]: the guard above leaves the slow
     * lookback no greater than endIdx, and the public tier rejects
     * endIdx < startIdx, so _slowStart <= endIdx and the seeding loops stop
     * one bar below it. There is nothing left for an empty-output arm to
     * catch, which is why this path has none.
     */
    let _fastTotal = 0;
    let _slowTotal = 0;
    let _fastValue = 0;
    let _slowValue = 0;
    let _slowMA = 0;
    let _i = 0;
    let _j = 0;
    let _outIdx = 0;
    let _fastStart = 0;
    let _slowStart = 0;
    let _fastTrailing = 0;
    let _slowTrailing = 0;
    let _slowLookback = 0;
    let _zeroRun = 0;
    /* Make sure slow is really slower than the fast period! if not, swap... */
    if (optInSlowPeriod < optInFastPeriod) {
      tempInteger = optInSlowPeriod;
      optInSlowPeriod = optInFastPeriod;
      optInFastPeriod = tempInteger;
    }
    _fastStart = optInFastPeriod - 1;
    if (_fastStart < startIdx) {
      _fastStart = startIdx;
    }
    _slowStart = optInSlowPeriod - 1;
    if (_slowStart < startIdx) {
      _slowStart = startIdx;
    }
    _fastTrailing = _fastStart - (optInFastPeriod - 1);
    _fastTotal = 0.0;
    for (_j = _fastTrailing; _j < _fastStart; _j += 1) {
      _fastTotal += inVolume[_j];
    }
    /* One loop seeds the slow sum and warms the dead-window counter: the bars
     * it walks, [_slowStart-_slowLookback, _slowStart), are exactly the ones
     * the general path warms _zeroRun over.
     */
    _slowLookback = optInSlowPeriod - 1;
    _zeroRun = 0;
    _slowTrailing = _slowStart - _slowLookback;
    _slowTotal = 0.0;
    for (_j = _slowTrailing; _j < _slowStart; _j += 1) {
      _slowTotal += inVolume[_j];
      _zeroRun = (Math.abs(inVolume[_j]) <= 0.0) ? _zeroRun + 1 : 0;
    }
    /* The bars the fast MA has and the slow one does not: advance the fast sum
     * alone. No output, but the sum must arrive at _slowStart along the same
     * path TA_SMA would have taken.
     */
    for (_i = _fastStart; _i < _slowStart; _i += 1) {
      _fastTotal += inVolume[_i];
      _fastTotal -= inVolume[_fastTrailing];
      _fastTrailing += 1;
    }
    _outIdx = 0;
    for (_i = _slowStart; _i <= endIdx; _i += 1) {
      _zeroRun = (Math.abs(inVolume[_i]) <= 0.0) ? _zeroRun + 1 : 0;
      _fastTotal += inVolume[_i];
      _fastValue = _fastTotal;
      _fastTotal -= inVolume[_fastTrailing];
      _fastTrailing += 1;
      _slowTotal += inVolume[_i];
      _slowValue = _slowTotal;
      _slowTotal -= inVolume[_slowTrailing];
      _slowTrailing += 1;
      _slowMA = _slowValue / optInSlowPeriod;
      if (_zeroRun > _slowLookback) {
        _zeroRun = _slowLookback;
        outReal[_outIdx] = 0.0;
      } else if (!((-0.00000000000001 < _slowMA) && (_slowMA < 0.00000000000001))) {
        outReal[_outIdx] = (_fastValue / optInFastPeriod - _slowMA) / _slowMA * 100.0;
      } else {
        outReal[_outIdx] = 0.0;
      }
      _outIdx += 1;
    }
    outBegIdx.value = _slowStart;
    outNBElement.value = _outIdx;
    return RetCode.SUCCESS;
  }
  if (optInMAType === MAType.EMA) {
    /* EMA fast path: both recursions in one loop, no buffer. Bit-identical to
     * the general path only while each EMA is seeded at its OWN lookback and
     * keeps ema.c's recursion spelling: the fast EMA starts earlier than the
     * slow one, and a shared seed bar would change every output.
     */
    let _eFastK = 0;
    let _eSlowK = 0;
    let _eFast = 0;
    let _eSlow = 0;
    let _eX = 0;
    let _eFastBeta = 0;
    let _eSlowBeta = 0;
    let _eN = 0;
    let _eToday = 0;
    let _eFastToday = 0;
    let _eSlowToday = 0;
    let _eSlowStart = 0;
    let _eOutIdx = 0;
    if (optInSlowPeriod < optInFastPeriod) {
      tempInteger = optInSlowPeriod;
      optInSlowPeriod = optInFastPeriod;
      optInFastPeriod = tempInteger;
    }
    _eFastBeta = (optInFastPeriod - 1) / (optInFastPeriod + 1);
    _eFastK = 1.0 - _eFastBeta;
    if (_eFastBeta < 0.5) {
      _eFastBeta = 1.0 - _eFastK;
    }
    _eSlowBeta = (optInSlowPeriod - 1) / (optInSlowPeriod + 1);
    _eSlowK = 1.0 - _eSlowBeta;
    if (_eSlowBeta < 0.5) {
      _eSlowBeta = 1.0 - _eSlowK;
    }
    _eFastToday = emaLookbackPositional(optInFastPeriod, core);
    if (_eFastToday < startIdx) {
      _eFastToday = startIdx;
    }
    _eFastToday -= emaLookbackPositional(optInFastPeriod, core);
    _eSlowStart = emaLookbackPositional(optInSlowPeriod, core);
    if (_eSlowStart < startIdx) {
      _eSlowStart = startIdx;
    }
    _eSlowToday = _eSlowStart - emaLookbackPositional(optInSlowPeriod, core);
    _eFast = 0.0;
    for (_eN = 0; _eN < optInFastPeriod; _eN += 1) {
      _eFast += inVolume[_eFastToday++];
    }
    _eFast = _eFast / optInFastPeriod;
    while (_eFastToday <= _eSlowStart) {
      {
        const _a: number = _eFastBeta, _b: number = _eFast, _c: number = _eFastK * inVolume[_eFastToday++];
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        _eFast = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
    }
    _eSlow = 0.0;
    for (_eN = 0; _eN < optInSlowPeriod; _eN += 1) {
      _eSlow += inVolume[_eSlowToday++];
    }
    _eSlow = _eSlow / optInSlowPeriod;
    while (_eSlowToday <= _eSlowStart) {
      {
        const _a: number = _eSlowBeta, _b: number = _eSlow, _c: number = _eSlowK * inVolume[_eSlowToday++];
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        _eSlow = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
    }
    _eOutIdx = 0;
    if (!((-0.00000000000001 < _eSlow) && (_eSlow < 0.00000000000001))) {
      outReal[_eOutIdx] = (_eFast - _eSlow) / _eSlow * 100.0;
    } else {
      outReal[_eOutIdx] = 0.0;
    }
    _eOutIdx += 1;
    _eToday = _eSlowStart + 1;
    while (_eToday <= endIdx) {
      _eX = inVolume[_eToday++];
      {
        const _a: number = _eFastBeta, _b: number = _eFast, _c: number = _eFastK * _eX;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        _eFast = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      {
        const _a: number = _eSlowBeta, _b: number = _eSlow, _c: number = _eSlowK * _eX;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        _eSlow = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      if (!((-0.00000000000001 < _eSlow) && (_eSlow < 0.00000000000001))) {
        outReal[_eOutIdx] = (_eFast - _eSlow) / _eSlow * 100.0;
      } else {
        outReal[_eOutIdx] = 0.0;
      }
      _eOutIdx += 1;
    }
    outBegIdx.value = _eSlowStart;
    outNBElement.value = _eOutIdx;
    return RetCode.SUCCESS;
  }
  /* Allocate an intermediate buffer. */
  tempBuffer = new Float64Array(endIdx - startIdx + 1);
  /* Make sure slow is really slower than
   * the fast period! if not, swap...
   */
  if (optInSlowPeriod < optInFastPeriod) {
    /* swap */
    tempInteger = optInSlowPeriod;
    optInSlowPeriod = optInFastPeriod;
    optInFastPeriod = tempInteger;
  }
  /* Calculate the fast MA into the tempBuffer. */
  const _xr4 = maPositional(startIdx, endIdx, inVolume, optInFastPeriod, (optInMAType as MAType), tempBuffer, core);
  fastBeg.value = _xr4.begIdx;
  fastNb.value = _xr4.count;
  /* Calculate the slow MA into the output. */
  const _xr5 = maPositional(startIdx, endIdx, inVolume, optInSlowPeriod, (optInMAType as MAType), outReal, core);
  outBegIdx.value = _xr5.begIdx;
  outNBElement.value = _xr5.count;
  /* fastNb - *outNBElement == slowBeg - fastBeg (the fast MA has at least as
   * many outputs), so tempBuffer[i+offset] is the fast MA at the same bar as
   * outReal[i], with a non-negative index. An empty slow MA skips the loop.
   */
  offset = fastNb.value - outNBElement.value;
  /* A windowed slow MA (SMA, WMA, TRIMA, HMA) over bars that are all exactly
   * zero is exactly zero, but its running sums leave residue there that
   * TA_IS_ZERO does not catch, and residue over residue is noise where 0 is
   * documented. zeroRun counts the trailing zero bars, held at slowLookback once
   * the window is dead. The recursive MA types really are nonzero on such a
   * window, so they keep the plain loop.
   */
  slowLookback = maLookbackPositional(optInSlowPeriod, (optInMAType as MAType), core);
  windowed = (optInMAType === MAType.SMA || optInMAType === MAType.WMA || optInMAType === MAType.TRIMA || optInMAType === MAType.HMA ? 1 : 0);
  zeroRun = 0;
  for (i = outBegIdx.value - slowLookback; i < outBegIdx.value; i += 1) {
    zeroRun = (Math.abs(inVolume[i]) <= 0.0) ? zeroRun + 1 : 0;
  }
  if (windowed !== 0) {
    for (i = 0; i < outNBElement.value; i += 1) {
      zeroRun = (Math.abs(inVolume[outBegIdx.value + i]) <= 0.0) ? zeroRun + 1 : 0;
      tempReal = outReal[i];
      if (zeroRun > slowLookback) {
        zeroRun = slowLookback;
        outReal[i] = 0.0;
      } else if (!((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001))) {
        outReal[i] = (tempBuffer[i + offset] - tempReal) / tempReal * 100.0;
      } else {
        outReal[i] = 0.0;
      }
    }
  } else {
    /* Calculate ((fast MA)-(slow MA))/(slow MA) in the output. */
    for (i = 0; i < outNBElement.value; i += 1) {
      tempReal = outReal[i];
      if (!((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001))) {
        outReal[i] = (tempBuffer[i + offset] - tempReal) / tempReal * 100.0;
      } else {
        outReal[i] = 0.0;
      }
    }
  }
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['fastPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['slowPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['maType', Core.INTEGER_DEFAULT, MATypes.Min, MATypes.Max, 'a MAType']] as const;

/** @internal */
export function pvoPositional(
  startIdx: number,
  endIdx: number,
  inVolume: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInMAType: MAType | typeof Core.INTEGER_DEFAULT,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('PVO', 'startIdx', startIdx);
  endIdx = requireInt32('PVO', 'endIdx', endIdx);
  optInFastPeriod = requireInt32('PVO', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('PVO', 'slowPeriod', optInSlowPeriod);
  optInMAType = requireInt32('PVO', 'maType', optInMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireIndexRange('PVO', startIdx, endIdx);
  const guardLookback = pvoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInMAType, core);
  if (guardLookback < 0) {
    throw rejectedParams('PVO', PARAM_RANGES, optInFastPeriod, optInSlowPeriod, optInMAType);
  }
  const guardStart = clampedStart('PVO', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('PVO', 'prices.volume', inVolume, guardInLen);
  requireOutput('PVO', 'outReal', outReal, guardOutLen, Float64Array);
  const inVolumeF64 = float64(inVolume);
  if (overlapsPartly(outReal, inVolumeF64)) {
    throw sharedMemory('PVO', 'outReal', 'prices.volume');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = pvoImpl(core, startIdx, endIdx, inVolumeF64, optInFastPeriod, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('PVO', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link pvo}, {@link pvoLookback}, {@link pvoDisplayShift},
 * {@link pvoOpen} and {@link pvoOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface PvoOptions {
  /**
   * Period of the fast MA (default 12; range 2 to 100000; `Core.INTEGER_DEFAULT`
   * selects the default).
   */
  fastPeriod?: number | undefined;
  /**
   * Period of the slow MA (default 26; range 2 to 100000; `Core.INTEGER_DEFAULT`
   * selects the default).
   */
  slowPeriod?: number | undefined;
  /**
   * Moving average type used for both MAs (default `MAType.EMA`; values: 0=SMA,
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
   * Buffer for `values`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outReal?: Float64Array | undefined;
}

/** What {@link pvo} returns, and {@link pvoOpenAndFill} with its stream. */
export interface PvoResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /** PVO value in percent. `values[k]` is the value at input bar `begIdx + k`. */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link pvo} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link PvoOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function pvoLookback(options?: PvoOptions): number {
  const o = optionsOf('PVO lookback', options);
  return pvoLookbackPositional(orDefault(o.fastPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowPeriod, Core.INTEGER_DEFAULT), orDefault(o.maType, Core.INTEGER_DEFAULT), coreOf('PVO lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link pvo}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link PvoOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function pvoDisplayShift(outputIdx: number, options?: PvoOptions): number {
  const o = optionsOf('PVO displayShift', options);
  return pvoDisplayShiftPositional(orDefault(o.fastPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowPeriod, Core.INTEGER_DEFAULT), orDefault(o.maType, Core.INTEGER_DEFAULT), outputIdx, coreOf('PVO displayShift', o.core));
}

/**
 * Percentage Volume Oscillator: a variation of the [Percentage Price
 * Oscillator](https://ta-lib.org/functions/ppo) (PPO, created by Gerald Appel)
 * applied to the **volume** series instead of price. It is the difference between
 * a fast and slow moving average of volume, expressed as a percentage of the slow
 * MA. Positive when short-term volume is above its longer-term average (rising
 * participation), negative when below. The default periods (12, 26) match MACD and
 * PPO.
 *
 * Formula and more info at
 * [ta-lib.org/functions/pvo](https://ta-lib.org/functions/pvo).
 *
 * **Notes**
 * - `optInMAType` applies to both the fast and slow moving average.
 *   `TA_MAType_MAMA` ignores its period argument, so with
 *   `optInMAType = TA_MAType_MAMA` the fast and slow MAs are identical, making the
 *   numerator — and therefore the output — zero at every bar.
 * - If the slow period is set smaller than the fast period, the two are swapped,
 *   as in `MACD`.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link pvoLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `volume`:
 *   Volume of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link PvoOptions}.
 * @returns `begIdx` and the values; see {@link PvoResult}. Each output is a view
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
 * import { MAType, pvo } from 'ta-lib';
 *
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const { begIdx, values } = pvo({ volume }, {
 *   fastPeriod: 12,
 *   slowPeriod: 26,
 *   maType: MAType.EMA,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`ppo`](https://ta-lib.org/functions/ppo)
 * @see [`obv`](https://ta-lib.org/functions/obv)
 * @see [`macd`](https://ta-lib.org/functions/macd)
 */
export function pvo(prices: { volume: ArrayLike<number> }, options?: PvoOptions): PvoResult {
  requireArgument('PVO', 'prices', prices);
  const o = optionsOf('PVO', options);
  if (o.endIdx === undefined) {
    requireSeries('PVO', 'prices.volume', prices.volume);
  }
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const optInMAType = orDefault(o.maType, Core.INTEGER_DEFAULT);
  const core = coreOf('PVO', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.volume.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, pvoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInMAType, core), prices.volume);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = pvoPositional(startIdx, endIdx, prices.volume, optInFastPeriod, optInSlowPeriod, optInMAType, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live PVO stream: one value per closed bar, bit-identical to {@link pvo} over
 * the same series. Open it with {@link pvoOpen} or {@link pvoOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/pvo](https://ta-lib.org/functions/pvo).
 */
export class PvoStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInFastPeriod: number = 0;
  /** @internal */
  optInSlowPeriod: number = 0;
  /** @internal */
  optInMAType: number = 0;
  /** @internal */
  cur_outReal: number = 0;
  /** @internal */
  slowLookback: number = 0;
  /** @internal */
  windowed: number = 0;
  /** @internal */
  zeroRun: number = 0;
  /** @internal */
  sub0!: MaStream;
  /** @internal */
  sub1!: MaStream;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): PvoStream {
    return new PvoStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link pvo} reports them over the same bars. Each accepted `update` and each
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
      throw failure('PVO advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inVolume - The bar's `inVolume` value: Volume of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inVolume: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('PVO update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inVolume)) {
      throw nonFiniteBar('PVO update', 'inVolume');
    }
    pvoStepImpl(this, inVolume);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inVolume - The bar's `inVolume` value: Volume of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inVolume: number): number {
    if (!Number.isFinite(inVolume)) {
      throw nonFiniteBar('PVO peek', 'inVolume');
    }
    const sp = this;
    let tempReal = 0;
    let cur_tempBuffer = 0;
    let cur_outReal = 0;
    cur_tempBuffer = sp.sub0.peek(inVolume);
    cur_outReal = sp.sub1.peek(inVolume);
    let slowLookback = sp.slowLookback;
    let windowed = sp.windowed;
    let zeroRun = sp.zeroRun;
    if (windowed !== 0) {
      zeroRun = (Math.abs(inVolume) <= 0.0) ? zeroRun + 1 : 0;
      tempReal = cur_outReal;
      if (zeroRun > slowLookback) {
        zeroRun = slowLookback;
        cur_outReal = 0.0;
      } else if (!((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001))) {
        cur_outReal = (cur_tempBuffer - tempReal) / tempReal * 100.0;
      } else {
        cur_outReal = 0.0;
      }
    } else {
      /* Calculate ((fast MA)-(slow MA))/(slow MA) in the output. */
      tempReal = cur_outReal;
      if (!((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001))) {
        cur_outReal = (cur_tempBuffer - tempReal) / tempReal * 100.0;
      } else {
        cur_outReal = 0.0;
      }
    }
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
  clone(): PvoStream {
    const c = new PvoStream(this.core);
    c.optInFastPeriod = this.optInFastPeriod;
    c.optInSlowPeriod = this.optInSlowPeriod;
    c.optInMAType = this.optInMAType;
    c.cur_outReal = this.cur_outReal;
    c.slowLookback = this.slowLookback;
    c.windowed = this.windowed;
    c.zeroRun = this.zeroRun;
    c.sub0 = this.sub0.clone();
    c.sub1 = this.sub1.clone();
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function pvoStepImpl(sp: PvoStream, inVolume: number): void {
  let tempReal = 0;
  let cur_tempBuffer = 0;
  let cur_outReal = 0;
  cur_tempBuffer = sp.sub0.update(inVolume);
  cur_outReal = sp.sub1.update(inVolume);
  let slowLookback = sp.slowLookback;
  let windowed = sp.windowed;
  let zeroRun = sp.zeroRun;
  if (windowed !== 0) {
    zeroRun = (Math.abs(inVolume) <= 0.0) ? zeroRun + 1 : 0;
    tempReal = cur_outReal;
    if (zeroRun > slowLookback) {
      zeroRun = slowLookback;
      cur_outReal = 0.0;
    } else if (!((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001))) {
      cur_outReal = (cur_tempBuffer - tempReal) / tempReal * 100.0;
    } else {
      cur_outReal = 0.0;
    }
  } else {
    /* Calculate ((fast MA)-(slow MA))/(slow MA) in the output. */
    tempReal = cur_outReal;
    if (!((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001))) {
      cur_outReal = (cur_tempBuffer - tempReal) / tempReal * 100.0;
    } else {
      cur_outReal = 0.0;
    }
  }
  sp.zeroRun = zeroRun;
  sp.cur_outReal = cur_outReal;
}

function pvoOpenImpl(
  sp: PvoStream,
  inVolume: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInMAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let tempBuffer!: Float64Array;
  let tempReal = 0;
  let tempInteger = 0;
  const fastBeg = new MInteger();
  const fastNb = new MInteger();
  let offset = 0;
  let slowLookback = 0;
  let windowed = 0;
  let zeroRun = 0;
  let i = 0;
  const historyLen = inVolume.length;
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
  if (optInMAType === Core.INTEGER_DEFAULT || optInMAType === MAType.DEFAULT) {
    optInMAType = MAType.EMA;
  } else if (optInMAType < MATypes.Min || optInMAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen < pvoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInMAType as MAType, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const sc_outReal = outStride === 1 ? outReal : new Float64Array(historyLen);
  /* Nothing to produce: the range ends before the lookback. Return before
   * touching anything.
   *
   * Without this the fast MA below runs first, and its lookback is SMALLER
   * than pvo's own — so it reads the whole range and computes a result the
   * empty slow MA then discards. Observably identical (the slow MA's own early
   * return already yields 0,0 here), but it is the difference between "a range
   * that ends before the lookback reads nothing" being true of this function and
   * being false: with a caller-supplied inVolume that stops short of endIdx, that
   * discarded work is an out-of-bounds read. Pinned by the zero-length no-I/O
   * probe over every guarded core.
   */
  if (maLookbackPositional((optInSlowPeriod > optInFastPeriod ? optInSlowPeriod : optInFastPeriod), (optInMAType as MAType), core) > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Allocate an intermediate buffer. */
  tempBuffer = new Float64Array(endIdx - startIdx + 1);
  /* Make sure slow is really slower than
   * the fast period! if not, swap...
   */
  if (optInSlowPeriod < optInFastPeriod) {
    /* swap */
    tempInteger = optInSlowPeriod;
    optInSlowPeriod = optInFastPeriod;
    optInFastPeriod = tempInteger;
  }
  /* Calculate the fast MA into the tempBuffer. */
  const sub0 = maOpenAndFillInternal(core, inVolume, startIdx, optInFastPeriod, (optInMAType as MAType), fastBeg, fastNb, tempBuffer);
  /* Calculate the slow MA into the output. */
  const sub1 = maOpenAndFillInternal(core, inVolume, startIdx, optInSlowPeriod, (optInMAType as MAType), outBegIdx, outNBElement, sc_outReal);
  /* fastNb - *outNBElement == slowBeg - fastBeg (the fast MA has at least as
   * many outputs), so tempBuffer[i+offset] is the fast MA at the same bar as
   * outReal[i], with a non-negative index. An empty slow MA skips the loop.
   */
  offset = fastNb.value - outNBElement.value;
  /* A windowed slow MA (SMA, WMA, TRIMA, HMA) over bars that are all exactly
   * zero is exactly zero, but its running sums leave residue there that
   * TA_IS_ZERO does not catch, and residue over residue is noise where 0 is
   * documented. zeroRun counts the trailing zero bars, held at slowLookback once
   * the window is dead. The recursive MA types really are nonzero on such a
   * window, so they keep the plain loop.
   */
  slowLookback = maLookbackPositional(optInSlowPeriod, (optInMAType as MAType), core);
  windowed = (optInMAType === MAType.SMA || optInMAType === MAType.WMA || optInMAType === MAType.TRIMA || optInMAType === MAType.HMA ? 1 : 0);
  zeroRun = 0;
  for (i = outBegIdx.value - slowLookback; i < outBegIdx.value; i += 1) {
    zeroRun = (Math.abs(inVolume[i]) <= 0.0) ? zeroRun + 1 : 0;
  }
  if (windowed !== 0) {
    for (i = 0; i < outNBElement.value; i += 1) {
      zeroRun = (Math.abs(inVolume[outBegIdx.value + i]) <= 0.0) ? zeroRun + 1 : 0;
      tempReal = sc_outReal[i];
      if (zeroRun > slowLookback) {
        zeroRun = slowLookback;
        sc_outReal[i] = 0.0;
      } else if (!((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001))) {
        sc_outReal[i] = (tempBuffer[i + offset] - tempReal) / tempReal * 100.0;
      } else {
        sc_outReal[i] = 0.0;
      }
    }
  } else {
    /* Calculate ((fast MA)-(slow MA))/(slow MA) in the output. */
    for (i = 0; i < outNBElement.value; i += 1) {
      tempReal = sc_outReal[i];
      if (!((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001))) {
        sc_outReal[i] = (tempBuffer[i + offset] - tempReal) / tempReal * 100.0;
      } else {
        sc_outReal[i] = 0.0;
      }
    }
  }
  if (outNBElement.value < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  sp.optInFastPeriod = optInFastPeriod;
  sp.optInSlowPeriod = optInSlowPeriod;
  sp.optInMAType = optInMAType;
  sp.sub0 = sub0;
  sp.sub1 = sub1;
  sp.slowLookback = slowLookback;
  sp.windowed = windowed;
  sp.zeroRun = zeroRun;
  sp.cur_outReal = sc_outReal[outNBElement.value - 1];
  return RetCode.SUCCESS;
}

/** @internal */
export function pvoOpenAndFillInternal(
  core: Core,
  inVolume: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInMAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): PvoStream {
  const sp = PvoStream.of(core);
  const retCode = pvoOpenImpl(sp, inVolume, startIdx, optInFastPeriod, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('PVO openAndFill', inVolume.length, startIdx, pvoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInMAType as MAType, core));
  }
  if (retCode === RetCode.BAD_PARAM && pvoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInMAType as MAType, core) < 0) {
    throw rejectedParams('PVO openAndFill', PARAM_RANGES, optInFastPeriod, optInSlowPeriod, optInMAType);
  }
  throw streamFailure('PVO openAndFill', retCode);
}

/** @internal */
export function pvoOpenInternal(
  core: Core,
  inVolume: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInMAType: number,
): PvoStream {
  const sp = PvoStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = pvoOpenImpl(sp, inVolume, startIdx, optInFastPeriod, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('PVO open', inVolume.length, startIdx, pvoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInMAType as MAType, core));
  }
  if (retCode === RetCode.BAD_PARAM && pvoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInMAType as MAType, core) < 0) {
    throw rejectedParams('PVO open', PARAM_RANGES, optInFastPeriod, optInSlowPeriod, optInMAType);
  }
  throw streamFailure('PVO open', retCode);
}

/** @internal */
export function pvoOpenPositional(
  inVolume: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInMAType: MAType | typeof Core.INTEGER_DEFAULT,
  core: Core = Core.DEFAULT,
): PvoStream {
  optInFastPeriod = requireInt32('PVO open', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('PVO open', 'slowPeriod', optInSlowPeriod);
  optInMAType = requireInt32('PVO open', 'maType', optInMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('PVO open', 'prices.volume', inVolume);
  requireHistory('PVO open', inVolume.length);
  return pvoOpenInternal(core, float64(inVolume), 0, optInFastPeriod, optInSlowPeriod, optInMAType);
}

/** @internal */
export function pvoOpenAndFillPositional(
  inVolume: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInMAType: MAType | typeof Core.INTEGER_DEFAULT,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): PvoStream {
  optInFastPeriod = requireInt32('PVO openAndFill', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('PVO openAndFill', 'slowPeriod', optInSlowPeriod);
  optInMAType = requireInt32('PVO openAndFill', 'maType', optInMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('PVO openAndFill', 'prices.volume', inVolume);
  requireHistory('PVO openAndFill', inVolume.length);
  const guardLookback = pvoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInMAType as MAType, core);
  if (guardLookback < 0) {
    throw rejectedParams('PVO openAndFill', PARAM_RANGES, optInFastPeriod, optInSlowPeriod, optInMAType);
  }
  const guardOutLen = openFillCount('PVO openAndFill', inVolume.length, guardLookback);
  requireOutput('PVO openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inVolumeF64 = float64(inVolume);
  if (outReal.buffer === inVolumeF64.buffer && outReal.byteOffset === inVolumeF64.byteOffset) {
    throw sharedMemory('PVO openAndFill', 'outReal', 'prices.volume');
  }
  if (guardOutLen > 0 && overlaps(outReal, inVolumeF64)) {
    throw sharedMemory('PVO openAndFill', 'outReal', 'prices.volume');
  }
  return pvoOpenAndFillInternal(core, inVolumeF64, 0, optInFastPeriod, optInSlowPeriod, optInMAType, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live PVO stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link pvo} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `volume`: Volume of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link PvoOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `pvoLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { MAType, pvoOpen } from 'ta-lib';
 *
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const stream = pvoOpen({ volume }, {
 *   fastPeriod: 12,
 *   slowPeriod: 26,
 *   maType: MAType.EMA,
 * });
 * const peeked = stream.peek(12345); // a forming bar: commits nothing
 * const updated = stream.update(12345); // the bar closes
 * ```
 */
export function pvoOpen(prices: { volume: ArrayLike<number> }, options?: PvoOptions): PvoStream {
  requireArgument('PVO open', 'prices', prices);
  const o = optionsOf('PVO open', options);
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const optInMAType = orDefault(o.maType, Core.INTEGER_DEFAULT);
  const core = coreOf('PVO open', o.core);
  return pvoOpenPositional(prices.volume, optInFastPeriod, optInSlowPeriod, optInMAType, core);
}

/**
 * {@link pvoOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link pvo}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `volume`: Volume of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link PvoOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link pvo} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `pvoLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or a
 *   buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { MAType, pvoOpenAndFill } from 'ta-lib';
 *
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const { stream, begIdx, values } = pvoOpenAndFill({ volume }, {
 *   fastPeriod: 12,
 *   slowPeriod: 26,
 *   maType: MAType.EMA,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function pvoOpenAndFill(prices: { volume: ArrayLike<number> }, options?: PvoOptions): PvoResult & { stream: PvoStream } {
  requireArgument('PVO openAndFill', 'prices', prices);
  const o = optionsOf('PVO openAndFill', options);
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const optInMAType = orDefault(o.maType, Core.INTEGER_DEFAULT);
  const core = coreOf('PVO openAndFill', o.core);
  const capacity = fillCapacity(pvoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInMAType, core), prices.volume);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = pvoOpenAndFillPositional(prices.volume, optInFastPeriod, optInSlowPeriod, optInMAType, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
