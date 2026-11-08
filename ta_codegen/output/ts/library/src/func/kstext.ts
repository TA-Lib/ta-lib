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
 *  100126 MF,CC  First version (#491).
 *  101126 MF,CC  Each lookback is asked once per call (#522).
 */

import { MAType, MATypes } from '../enums.ts';
import { badSink, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { MaStream, maLookbackPositional, maOpenAndFillInternal, maOpenInternal, maPositional } from './ma.ts';
import { RocStream, rocOpenAndFillInternal, rocPositional } from './roc.ts';

/** @internal */
export function kstextLookbackPositional(optInROC1Period: number, optInROC2Period: number, optInROC3Period: number, optInROC4Period: number, optInMA1Period: number, optInMA2Period: number, optInMA3Period: number, optInMA4Period: number, optInSignalPeriod: number, optInROCMAType: MAType | typeof Core.INTEGER_DEFAULT, optInSignalMAType: MAType | typeof Core.INTEGER_DEFAULT, core: Core = Core.DEFAULT): number {
  if ((optInROC1Period | 0) !== optInROC1Period) {
    return -1;
  }
  if ((optInROC2Period | 0) !== optInROC2Period) {
    return -1;
  }
  if ((optInROC3Period | 0) !== optInROC3Period) {
    return -1;
  }
  if ((optInROC4Period | 0) !== optInROC4Period) {
    return -1;
  }
  if ((optInMA1Period | 0) !== optInMA1Period) {
    return -1;
  }
  if ((optInMA2Period | 0) !== optInMA2Period) {
    return -1;
  }
  if ((optInMA3Period | 0) !== optInMA3Period) {
    return -1;
  }
  if ((optInMA4Period | 0) !== optInMA4Period) {
    return -1;
  }
  if ((optInSignalPeriod | 0) !== optInSignalPeriod) {
    return -1;
  }
  if ((optInROCMAType | 0) !== optInROCMAType) {
    return -1;
  }
  if ((optInSignalMAType | 0) !== optInSignalMAType) {
    return -1;
  }
  if (optInROC1Period === Core.INTEGER_DEFAULT) {
    optInROC1Period = 10;
  } else if (optInROC1Period < 1 || optInROC1Period > 100000) {
    return -1;
  }
  if (optInROC2Period === Core.INTEGER_DEFAULT) {
    optInROC2Period = 15;
  } else if (optInROC2Period < 1 || optInROC2Period > 100000) {
    return -1;
  }
  if (optInROC3Period === Core.INTEGER_DEFAULT) {
    optInROC3Period = 20;
  } else if (optInROC3Period < 1 || optInROC3Period > 100000) {
    return -1;
  }
  if (optInROC4Period === Core.INTEGER_DEFAULT) {
    optInROC4Period = 30;
  } else if (optInROC4Period < 1 || optInROC4Period > 100000) {
    return -1;
  }
  if (optInMA1Period === Core.INTEGER_DEFAULT) {
    optInMA1Period = 10;
  } else if (optInMA1Period < 1 || optInMA1Period > 100000) {
    return -1;
  }
  if (optInMA2Period === Core.INTEGER_DEFAULT) {
    optInMA2Period = 10;
  } else if (optInMA2Period < 1 || optInMA2Period > 100000) {
    return -1;
  }
  if (optInMA3Period === Core.INTEGER_DEFAULT) {
    optInMA3Period = 10;
  } else if (optInMA3Period < 1 || optInMA3Period > 100000) {
    return -1;
  }
  if (optInMA4Period === Core.INTEGER_DEFAULT) {
    optInMA4Period = 15;
  } else if (optInMA4Period < 1 || optInMA4Period > 100000) {
    return -1;
  }
  if (optInSignalPeriod === Core.INTEGER_DEFAULT) {
    optInSignalPeriod = 9;
  } else if (optInSignalPeriod < 1 || optInSignalPeriod > 100000) {
    return -1;
  }
  if (optInROCMAType === Core.INTEGER_DEFAULT || optInROCMAType === MAType.DEFAULT) {
    optInROCMAType = MAType.SMA;
  } else if (optInROCMAType < MATypes.Min || optInROCMAType > MATypes.Max) {
    return -1;
  }
  if (optInSignalMAType === Core.INTEGER_DEFAULT || optInSignalMAType === MAType.DEFAULT) {
    optInSignalMAType = MAType.SMA;
  } else if (optInSignalMAType < MATypes.Min || optInSignalMAType > MATypes.Max) {
    return -1;
  }
  let legMax = 0;
  let leg = 0;
  legMax = optInROC1Period + maLookbackPositional(optInMA1Period, (optInROCMAType as MAType), core);
  leg = optInROC2Period + maLookbackPositional(optInMA2Period, (optInROCMAType as MAType), core);
  if (leg > legMax) {
    legMax = leg;
  }
  leg = optInROC3Period + maLookbackPositional(optInMA3Period, (optInROCMAType as MAType), core);
  if (leg > legMax) {
    legMax = leg;
  }
  leg = optInROC4Period + maLookbackPositional(optInMA4Period, (optInROCMAType as MAType), core);
  if (leg > legMax) {
    legMax = leg;
  }
  return legMax + maLookbackPositional(optInSignalPeriod, (optInSignalMAType as MAType), core);
}

/** @internal */
export function kstextDisplayShiftPositional(optInROC1Period: number, optInROC2Period: number, optInROC3Period: number, optInROC4Period: number, optInMA1Period: number, optInMA2Period: number, optInMA3Period: number, optInMA4Period: number, optInSignalPeriod: number, optInROCMAType: MAType | typeof Core.INTEGER_DEFAULT, optInSignalMAType: MAType | typeof Core.INTEGER_DEFAULT, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (kstextLookbackPositional(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 2) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function kstextImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInROC1Period: number,
  optInROC2Period: number,
  optInROC3Period: number,
  optInROC4Period: number,
  optInMA1Period: number,
  optInMA2Period: number,
  optInMA3Period: number,
  optInMA4Period: number,
  optInSignalPeriod: number,
  optInROCMAType: number,
  optInSignalMAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outKST: Float64Array,
  outKSTSignal: Float64Array,
): RetCode {
  let kstBuffer!: Float64Array;
  let tempBuffer!: Float64Array;
  let lookbackTotal = 0;
  let lookbackSignal = 0;
  let lookbackMA = 0;
  let sigStart = 0;
  let lookbackMA1 = 0;
  let lookbackMA2 = 0;
  let lookbackMA3 = 0;
  let lookbackMA4 = 0;
  let tempInteger = 0;
  const tempBegIdx = new MInteger();
  const rocNb = new MInteger();
  const kstNb = new MInteger();
  const legNb = new MInteger();
  const sigNb = new MInteger();
  let i = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInROC1Period === Core.INTEGER_DEFAULT) {
    optInROC1Period = 10;
  } else if (optInROC1Period < 1 || optInROC1Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInROC2Period === Core.INTEGER_DEFAULT) {
    optInROC2Period = 15;
  } else if (optInROC2Period < 1 || optInROC2Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInROC3Period === Core.INTEGER_DEFAULT) {
    optInROC3Period = 20;
  } else if (optInROC3Period < 1 || optInROC3Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInROC4Period === Core.INTEGER_DEFAULT) {
    optInROC4Period = 30;
  } else if (optInROC4Period < 1 || optInROC4Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInMA1Period === Core.INTEGER_DEFAULT) {
    optInMA1Period = 10;
  } else if (optInMA1Period < 1 || optInMA1Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInMA2Period === Core.INTEGER_DEFAULT) {
    optInMA2Period = 10;
  } else if (optInMA2Period < 1 || optInMA2Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInMA3Period === Core.INTEGER_DEFAULT) {
    optInMA3Period = 10;
  } else if (optInMA3Period < 1 || optInMA3Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInMA4Period === Core.INTEGER_DEFAULT) {
    optInMA4Period = 15;
  } else if (optInMA4Period < 1 || optInMA4Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSignalPeriod === Core.INTEGER_DEFAULT) {
    optInSignalPeriod = 9;
  } else if (optInSignalPeriod < 1 || optInSignalPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInROCMAType === Core.INTEGER_DEFAULT || optInROCMAType === MAType.DEFAULT) {
    optInROCMAType = MAType.SMA;
  } else if (optInROCMAType < MATypes.Min || optInROCMAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if (optInSignalMAType === Core.INTEGER_DEFAULT || optInSignalMAType === MAType.DEFAULT) {
    optInSignalMAType = MAType.SMA;
  } else if (optInSignalMAType < MATypes.Min || optInSignalMAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if ((outKST.buffer === outKSTSignal.buffer && outKST.byteOffset === outKSTSignal.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  /* With every type SMA this is bit-exact with kst(): each leg's average
   * starts on the first bar the signal consumes, and the line accumulates
   * its legs left to right. Changing either breaks the equality.
   */
  outBegIdx.value = 0;
  outNBElement.value = 0;
  /* lookbackTotal must stay kstext_lookback() of the same parameters. */
  lookbackSignal = maLookbackPositional(optInSignalPeriod, (optInSignalMAType as MAType), core);
  lookbackMA1 = maLookbackPositional(optInMA1Period, (optInROCMAType as MAType), core);
  lookbackMA2 = maLookbackPositional(optInMA2Period, (optInROCMAType as MAType), core);
  lookbackMA3 = maLookbackPositional(optInMA3Period, (optInROCMAType as MAType), core);
  lookbackMA4 = maLookbackPositional(optInMA4Period, (optInROCMAType as MAType), core);
  lookbackTotal = optInROC1Period + lookbackMA1;
  tempInteger = optInROC2Period + lookbackMA2;
  if (tempInteger > lookbackTotal) {
    lookbackTotal = tempInteger;
  }
  tempInteger = optInROC3Period + lookbackMA3;
  if (tempInteger > lookbackTotal) {
    lookbackTotal = tempInteger;
  }
  tempInteger = optInROC4Period + lookbackMA4;
  if (tempInteger > lookbackTotal) {
    lookbackTotal = tempInteger;
  }
  lookbackTotal += lookbackSignal;
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  sigStart = startIdx - lookbackSignal;
  /* Both buffers are sized by the longest leg average: a rate of change
   * starts its average's lookback before sigStart.
   */
  lookbackMA = lookbackMA1;
  if (lookbackMA2 > lookbackMA) {
    lookbackMA = lookbackMA2;
  }
  if (lookbackMA3 > lookbackMA) {
    lookbackMA = lookbackMA3;
  }
  if (lookbackMA4 > lookbackMA) {
    lookbackMA = lookbackMA4;
  }
  tempInteger = endIdx - sigStart + 1 + lookbackMA;
  kstBuffer = new Float64Array(tempInteger);
  tempBuffer = new Float64Array(tempInteger);
  const _xr0 = rocPositional(sigStart - lookbackMA1, endIdx, inReal, optInROC1Period, kstBuffer, core);
  tempBegIdx.value = _xr0.begIdx;
  rocNb.value = _xr0.count;
  const _xr1 = maPositional(0, rocNb.value - 1, kstBuffer, optInMA1Period, (optInROCMAType as MAType), kstBuffer, core);
  tempBegIdx.value = _xr1.begIdx;
  kstNb.value = _xr1.count;
  const _xr2 = rocPositional(sigStart - lookbackMA2, endIdx, inReal, optInROC2Period, tempBuffer, core);
  tempBegIdx.value = _xr2.begIdx;
  rocNb.value = _xr2.count;
  const _xr3 = maPositional(0, rocNb.value - 1, tempBuffer, optInMA2Period, (optInROCMAType as MAType), tempBuffer, core);
  tempBegIdx.value = _xr3.begIdx;
  legNb.value = _xr3.count;
  for (i = 0; i < kstNb.value; i += 1) {
    {
      const _a: number = 2.0, _b: number = tempBuffer[i], _c: number = kstBuffer[i];
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      kstBuffer[i] = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
  }
  const _xr5 = rocPositional(sigStart - lookbackMA3, endIdx, inReal, optInROC3Period, tempBuffer, core);
  tempBegIdx.value = _xr5.begIdx;
  rocNb.value = _xr5.count;
  const _xr6 = maPositional(0, rocNb.value - 1, tempBuffer, optInMA3Period, (optInROCMAType as MAType), tempBuffer, core);
  tempBegIdx.value = _xr6.begIdx;
  legNb.value = _xr6.count;
  for (i = 0; i < kstNb.value; i += 1) {
    {
      const _a: number = 3.0, _b: number = tempBuffer[i], _c: number = kstBuffer[i];
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      kstBuffer[i] = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
  }
  const _xr8 = rocPositional(sigStart - lookbackMA4, endIdx, inReal, optInROC4Period, tempBuffer, core);
  tempBegIdx.value = _xr8.begIdx;
  rocNb.value = _xr8.count;
  const _xr9 = maPositional(0, rocNb.value - 1, tempBuffer, optInMA4Period, (optInROCMAType as MAType), tempBuffer, core);
  tempBegIdx.value = _xr9.begIdx;
  legNb.value = _xr9.count;
  for (i = 0; i < kstNb.value; i += 1) {
    {
      const _a: number = 4.0, _b: number = tempBuffer[i], _c: number = kstBuffer[i];
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      kstBuffer[i] = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
  }
  /* Every read of inReal is done: an output may alias it. */
  outKST.set(kstBuffer.subarray(lookbackSignal, lookbackSignal + (endIdx - startIdx + 1)), 0);
  const _xr11 = maPositional(0, kstNb.value - 1, kstBuffer, optInSignalPeriod, (optInSignalMAType as MAType), outKSTSignal, core);
  tempBegIdx.value = _xr11.begIdx;
  sigNb.value = _xr11.count;
  outBegIdx.value = startIdx;
  outNBElement.value = sigNb.value;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['roc1Period', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['roc2Period', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['roc3Period', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['roc4Period', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['ma1Period', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['ma2Period', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['ma3Period', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['ma4Period', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['signalPeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['rocMaType', Core.INTEGER_DEFAULT, MATypes.Min, MATypes.Max, 'a MAType'], ['signalMaType', Core.INTEGER_DEFAULT, MATypes.Min, MATypes.Max, 'a MAType']] as const;

/** @internal */
export function kstextPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInROC1Period: number,
  optInROC2Period: number,
  optInROC3Period: number,
  optInROC4Period: number,
  optInMA1Period: number,
  optInMA2Period: number,
  optInMA3Period: number,
  optInMA4Period: number,
  optInSignalPeriod: number,
  optInROCMAType: MAType | typeof Core.INTEGER_DEFAULT,
  optInSignalMAType: MAType | typeof Core.INTEGER_DEFAULT,
  outKST: Float64Array,
  outKSTSignal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('KSTEXT', 'startIdx', startIdx);
  endIdx = requireInt32('KSTEXT', 'endIdx', endIdx);
  optInROC1Period = requireInt32('KSTEXT', 'roc1Period', optInROC1Period);
  optInROC2Period = requireInt32('KSTEXT', 'roc2Period', optInROC2Period);
  optInROC3Period = requireInt32('KSTEXT', 'roc3Period', optInROC3Period);
  optInROC4Period = requireInt32('KSTEXT', 'roc4Period', optInROC4Period);
  optInMA1Period = requireInt32('KSTEXT', 'ma1Period', optInMA1Period);
  optInMA2Period = requireInt32('KSTEXT', 'ma2Period', optInMA2Period);
  optInMA3Period = requireInt32('KSTEXT', 'ma3Period', optInMA3Period);
  optInMA4Period = requireInt32('KSTEXT', 'ma4Period', optInMA4Period);
  optInSignalPeriod = requireInt32('KSTEXT', 'signalPeriod', optInSignalPeriod);
  optInROCMAType = requireInt32('KSTEXT', 'rocMaType', optInROCMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  optInSignalMAType = requireInt32('KSTEXT', 'signalMaType', optInSignalMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireIndexRange('KSTEXT', startIdx, endIdx);
  const guardLookback = kstextLookbackPositional(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType, core);
  if (guardLookback < 0) {
    throw rejectedParams('KSTEXT', PARAM_RANGES, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType);
  }
  const guardStart = clampedStart('KSTEXT', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('KSTEXT', 'real', inReal, guardInLen);
  requireOutput('KSTEXT', 'outKst', outKST, guardOutLen, Float64Array);
  requireOutput('KSTEXT', 'outKstSignal', outKSTSignal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outKST, inRealF64)) {
    throw sharedMemory('KSTEXT', 'outKst', 'real');
  }
  if (overlapsPartly(outKSTSignal, inRealF64)) {
    throw sharedMemory('KSTEXT', 'outKstSignal', 'real');
  }
  if (overlaps(outKST, outKSTSignal)) {
    throw sharedMemory('KSTEXT', 'outKst', 'outKstSignal');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = kstextImpl(core, startIdx, endIdx, inRealF64, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType, outBegIdx, outNBElement, outKST, outKSTSignal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('KSTEXT', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link kstext}, {@link kstextLookback}, {@link kstextDisplayShift},
 * {@link kstextOpen} and {@link kstextOpenAndFill}. Every key is optional: an
 * omitted or `undefined` key takes its default, and a call ignores the keys it has
 * no use for.
 */
export interface KstextOptions {
  /**
   * Rate-of-change period of leg 1 (weight 1) (default 10; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  roc1Period?: number | undefined;
  /**
   * Rate-of-change period of leg 2 (weight 2) (default 15; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  roc2Period?: number | undefined;
  /**
   * Rate-of-change period of leg 3 (weight 3) (default 20; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  roc3Period?: number | undefined;
  /**
   * Rate-of-change period of leg 4 (weight 4) (default 30; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  roc4Period?: number | undefined;
  /**
   * Period of the MA smoothing leg 1 (default 10; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  ma1Period?: number | undefined;
  /**
   * Period of the MA smoothing leg 2 (default 10; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  ma2Period?: number | undefined;
  /**
   * Period of the MA smoothing leg 3 (default 10; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  ma3Period?: number | undefined;
  /**
   * Period of the MA smoothing leg 4 (default 15; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  ma4Period?: number | undefined;
  /**
   * Period of the signal-line MA (default 9; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  signalPeriod?: number | undefined;
  /**
   * MA type smoothing the four legs (default `MAType.SMA`; values: 0=SMA, 1=EMA,
   * 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA, 10=DISABLED,
   * 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA; `Core.INTEGER_DEFAULT` or
   * `MAType.DEFAULT` selects the default).
   */
  rocMaType?: MAType | typeof Core.INTEGER_DEFAULT | undefined;
  /**
   * MA type for the signal line (default `MAType.SMA`; values: 0=SMA, 1=EMA, 2=WMA,
   * 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT,
   * 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT`
   * selects the default).
   */
  signalMaType?: MAType | typeof Core.INTEGER_DEFAULT | undefined;
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
   * Buffer for `outKst`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outKst?: Float64Array<ArrayBuffer> | undefined;
  /**
   * Buffer for `outKstSignal`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outKstSignal?: Float64Array<ArrayBuffer> | undefined;
}

/** What {@link kstext} returns, and {@link kstextOpenAndFill} with its stream. */
export interface KstextResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /** Know Sure Thing line. `outKst[k]` is the value at input bar `begIdx + k`. */
  outKst: Float64Array<ArrayBuffer>;
  /**
   * Signal line: MA of the line. `outKstSignal[k]` is the value at input bar
   * `begIdx + k`.
   */
  outKstSignal: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link kstext} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link KstextOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function kstextLookback(options?: KstextOptions): number {
  const o = optionsOf('KSTEXT lookback', options);
  return kstextLookbackPositional(orDefault(o.roc1Period, Core.INTEGER_DEFAULT), orDefault(o.roc2Period, Core.INTEGER_DEFAULT), orDefault(o.roc3Period, Core.INTEGER_DEFAULT), orDefault(o.roc4Period, Core.INTEGER_DEFAULT), orDefault(o.ma1Period, Core.INTEGER_DEFAULT), orDefault(o.ma2Period, Core.INTEGER_DEFAULT), orDefault(o.ma3Period, Core.INTEGER_DEFAULT), orDefault(o.ma4Period, Core.INTEGER_DEFAULT), orDefault(o.signalPeriod, Core.INTEGER_DEFAULT), orDefault(o.rocMaType, Core.INTEGER_DEFAULT), orDefault(o.signalMaType, Core.INTEGER_DEFAULT), coreOf('KSTEXT lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link kstext}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link KstextOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function kstextDisplayShift(outputIdx: number, options?: KstextOptions): number {
  const o = optionsOf('KSTEXT displayShift', options);
  return kstextDisplayShiftPositional(orDefault(o.roc1Period, Core.INTEGER_DEFAULT), orDefault(o.roc2Period, Core.INTEGER_DEFAULT), orDefault(o.roc3Period, Core.INTEGER_DEFAULT), orDefault(o.roc4Period, Core.INTEGER_DEFAULT), orDefault(o.ma1Period, Core.INTEGER_DEFAULT), orDefault(o.ma2Period, Core.INTEGER_DEFAULT), orDefault(o.ma3Period, Core.INTEGER_DEFAULT), orDefault(o.ma4Period, Core.INTEGER_DEFAULT), orDefault(o.signalPeriod, Core.INTEGER_DEFAULT), orDefault(o.rocMaType, Core.INTEGER_DEFAULT), orDefault(o.signalMaType, Core.INTEGER_DEFAULT), outputIdx, coreOf('KSTEXT displayShift', o.core));
}

/**
 * Know Sure Thing with selectable moving averages: Pring's weighted sum of four
 * smoothed rates of change, with a signal line. One MA type smooths the four legs
 * and another the signal line. `KST` is Pring's definition, with simple averages
 * throughout. Formulas published since then smooth the legs exponentially, and
 * Pring allows a simple or an exponential signal line. The reading is the same as
 * `KST`'s: above zero the combined momentum is positive, and the usual signals are
 * the line crossing its signal line and the line changing direction.
 *
 * Formula and more info at
 * [ta-lib.org/functions/kstext](https://ta-lib.org/functions/kstext).
 *
 * **Notes**
 * - **The input is assumed positive.** The price each rate of change looks back to
 *   is its divisor. Near zero the divisor is small and the output very large; no
 *   error is returned. A series that reaches or crosses zero, such as an
 *   oscillator or a spread, has no meaningful result here.
 * - With both MA types set to `MAType.SMA` the outputs are those of `KST`.
 * - Both outputs start at the first bar where the signal line exists. A signal
 *   period of 1 disables signal-line smoothing for every signal MAType: the signal
 *   is then a copy of the line.
 * - The unstable period of a selected MA type lengthens the lookback, for the legs
 *   and for the signal line separately.
 * - `MAType.MAMA` ignores its period argument, so where it is selected every
 *   period above 1 gives the same average.
 * - Each rate of change follows `ROC`: a zero price in the denominator makes that
 *   term 0.
 * - The weights go with leg position, not with the length of the rate of change.
 *
 * `outKst[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link kstextLookback} is a success with empty outputs.
 *
 * @param real - Source price series (canonically the close)
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link KstextOptions}.
 * @returns `begIdx` and the values; see {@link KstextResult}. Each output is a
 *   view of its buffer, the caller's or a new one.
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
 * import { MAType, kstext } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, outKst, outKstSignal } = kstext(data, {
 *   roc1Period: 10,
 *   roc2Period: 15,
 *   roc3Period: 20,
 *   roc4Period: 30,
 *   ma1Period: 10,
 *   ma2Period: 10,
 *   ma3Period: 10,
 *   ma4Period: 15,
 *   signalPeriod: 9,
 *   rocMaType: MAType.SMA,
 *   signalMaType: MAType.SMA,
 * });
 * // outKst[k] is the value at bar begIdx + k
 * ```
 * @see [`kst`](https://ta-lib.org/functions/kst)
 * @see [`roc`](https://ta-lib.org/functions/roc)
 * @see [`ma`](https://ta-lib.org/functions/ma)
 * @see [`macdext`](https://ta-lib.org/functions/macdext)
 * @see [`coppock`](https://ta-lib.org/functions/coppock)
 */
export function kstext(real: ArrayLike<number>, options?: KstextOptions): KstextResult {
  const o = optionsOf('KSTEXT', options);
  if (o.endIdx === undefined) {
    requireSeries('KSTEXT', 'real', real);
  }
  const optInROC1Period = orDefault(o.roc1Period, Core.INTEGER_DEFAULT);
  const optInROC2Period = orDefault(o.roc2Period, Core.INTEGER_DEFAULT);
  const optInROC3Period = orDefault(o.roc3Period, Core.INTEGER_DEFAULT);
  const optInROC4Period = orDefault(o.roc4Period, Core.INTEGER_DEFAULT);
  const optInMA1Period = orDefault(o.ma1Period, Core.INTEGER_DEFAULT);
  const optInMA2Period = orDefault(o.ma2Period, Core.INTEGER_DEFAULT);
  const optInMA3Period = orDefault(o.ma3Period, Core.INTEGER_DEFAULT);
  const optInMA4Period = orDefault(o.ma4Period, Core.INTEGER_DEFAULT);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const optInROCMAType = orDefault(o.rocMaType, Core.INTEGER_DEFAULT);
  const optInSignalMAType = orDefault(o.signalMaType, Core.INTEGER_DEFAULT);
  const core = coreOf('KSTEXT', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, kstextLookbackPositional(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType, core), real);
  const outKST = o.outKst === undefined ? new Float64Array(capacity) : o.outKst;
  const outKSTSignal = o.outKstSignal === undefined ? new Float64Array(capacity) : o.outKstSignal;
  const range = kstextPositional(startIdx, endIdx, real, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType, outKST, outKSTSignal, core);
  return { begIdx: range.begIdx, outKst: outKST.subarray(0, range.count), outKstSignal: outKSTSignal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live KSTEXT stream: one value per closed bar, bit-identical to {@link kstext}
 * over the same series. Open it with {@link kstextOpen} or
 * {@link kstextOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/kstext](https://ta-lib.org/functions/kstext).
 */
export class KstextStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInROC1Period: number = 0;
  /** @internal */
  optInROC2Period: number = 0;
  /** @internal */
  optInROC3Period: number = 0;
  /** @internal */
  optInROC4Period: number = 0;
  /** @internal */
  optInMA1Period: number = 0;
  /** @internal */
  optInMA2Period: number = 0;
  /** @internal */
  optInMA3Period: number = 0;
  /** @internal */
  optInMA4Period: number = 0;
  /** @internal */
  optInSignalPeriod: number = 0;
  /** @internal */
  optInROCMAType: number = 0;
  /** @internal */
  optInSignalMAType: number = 0;
  /** @internal */
  cur_outKST: number = 0;
  /** @internal */
  cur_outKSTSignal: number = 0;
  /** @internal */
  sub0!: RocStream;
  /** @internal */
  sub1!: MaStream;
  /** @internal */
  sub2!: RocStream;
  /** @internal */
  sub3!: MaStream;
  /** @internal */
  sub4!: RocStream;
  /** @internal */
  sub5!: MaStream;
  /** @internal */
  sub6!: RocStream;
  /** @internal */
  sub7!: MaStream;
  /** @internal */
  sub8!: MaStream;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): KstextStream {
    return new KstextStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link kstext} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('KSTEXT advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns its outputs.
   *
   * @param real - The bar's `real` value: Source price series (canonically the
   *   close)
   * @param out - Receives the outputs and is returned; a new {@link KstextOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included, before anything moves.
   *   `TALibError` with code `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached
   *   bar `Core.INDEX_MAX`.
   */
  update(real: number, out?: KstextOut): KstextOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('KSTEXT update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('KSTEXT update', 'KstextOut');
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('KSTEXT update', 'real');
    }
    kstextStepImpl(this, real);
    this.outRangeCount++;
    if (out === undefined) {
      out = new KstextOut();
    }
    out.outKst = this.cur_outKST;
    out.outKstSignal = this.cur_outKSTSignal;
    return out;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: Source price series (canonically the
   *   close)
   * @param out - Receives the outputs and is returned; a new {@link KstextOut} when
   *   omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included.
   */
  peek(real: number, out?: KstextOut): KstextOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('KSTEXT peek', 'KstextOut');
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('KSTEXT peek', 'real');
    }
    const sp = this;
    const inReal = real;
    let cur_kstBuffer = 0;
    let cur_tempBuffer = 0;
    let cur_outKSTSignal = 0;
    let cur_outKST = 0;
    cur_kstBuffer = sp.sub0.peek(inReal);
    cur_kstBuffer = sp.sub1.peek(cur_kstBuffer);
    cur_tempBuffer = sp.sub2.peek(inReal);
    cur_tempBuffer = sp.sub3.peek(cur_tempBuffer);
    {
      const _a: number = 2.0, _b: number = cur_tempBuffer, _c: number = cur_kstBuffer;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      cur_kstBuffer = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    cur_tempBuffer = sp.sub4.peek(inReal);
    cur_tempBuffer = sp.sub5.peek(cur_tempBuffer);
    {
      const _a: number = 3.0, _b: number = cur_tempBuffer, _c: number = cur_kstBuffer;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      cur_kstBuffer = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    cur_tempBuffer = sp.sub6.peek(inReal);
    cur_tempBuffer = sp.sub7.peek(cur_tempBuffer);
    {
      const _a: number = 4.0, _b: number = cur_tempBuffer, _c: number = cur_kstBuffer;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      cur_kstBuffer = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    cur_outKSTSignal = sp.sub8.peek(cur_kstBuffer);
    cur_outKST = cur_kstBuffer;
    if (out === undefined) {
      out = new KstextOut();
    }
    out.outKst = cur_outKST;
    out.outKstSignal = cur_outKSTSignal;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link KstextOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a sink that is not an object,
   *   `null` included.
   */
  value(out?: KstextOut): KstextOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('KSTEXT value', 'KstextOut');
    }
    if (out === undefined) {
      out = new KstextOut();
    }
    out.outKst = this.cur_outKST;
    out.outKstSignal = this.cur_outKSTSignal;
    return out;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): KstextStream {
    const c = new KstextStream(this.core);
    c.optInROC1Period = this.optInROC1Period;
    c.optInROC2Period = this.optInROC2Period;
    c.optInROC3Period = this.optInROC3Period;
    c.optInROC4Period = this.optInROC4Period;
    c.optInMA1Period = this.optInMA1Period;
    c.optInMA2Period = this.optInMA2Period;
    c.optInMA3Period = this.optInMA3Period;
    c.optInMA4Period = this.optInMA4Period;
    c.optInSignalPeriod = this.optInSignalPeriod;
    c.optInROCMAType = this.optInROCMAType;
    c.optInSignalMAType = this.optInSignalMAType;
    c.cur_outKST = this.cur_outKST;
    c.cur_outKSTSignal = this.cur_outKSTSignal;
    c.sub0 = this.sub0.clone();
    c.sub1 = this.sub1.clone();
    c.sub2 = this.sub2.clone();
    c.sub3 = this.sub3.clone();
    c.sub4 = this.sub4.clone();
    c.sub5 = this.sub5.clone();
    c.sub6 = this.sub6.clone();
    c.sub7 = this.sub7.clone();
    c.sub8 = this.sub8.clone();
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one KSTEXT bar, as `update`, `peek` and `value` return them.
 * Passed back to one of them, it is filled and returned again, with no allocation;
 * it holds its values until the next call that writes it. A new one holds `NaN`.
 */
export class KstextOut {
  /** Know Sure Thing line. */
  outKst: number = NaN;
  /** Signal line: MA of the line. */
  outKstSignal: number = NaN;
}

function kstextStepImpl(sp: KstextStream, inReal: number): void {
  let cur_kstBuffer = 0;
  let cur_tempBuffer = 0;
  let cur_outKSTSignal = 0;
  cur_kstBuffer = sp.sub0.update(inReal);
  cur_kstBuffer = sp.sub1.update(cur_kstBuffer);
  cur_tempBuffer = sp.sub2.update(inReal);
  cur_tempBuffer = sp.sub3.update(cur_tempBuffer);
  {
    const _a: number = 2.0, _b: number = cur_tempBuffer, _c: number = cur_kstBuffer;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    cur_kstBuffer = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  cur_tempBuffer = sp.sub4.update(inReal);
  cur_tempBuffer = sp.sub5.update(cur_tempBuffer);
  {
    const _a: number = 3.0, _b: number = cur_tempBuffer, _c: number = cur_kstBuffer;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    cur_kstBuffer = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  cur_tempBuffer = sp.sub6.update(inReal);
  cur_tempBuffer = sp.sub7.update(cur_tempBuffer);
  {
    const _a: number = 4.0, _b: number = cur_tempBuffer, _c: number = cur_kstBuffer;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    cur_kstBuffer = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  cur_outKSTSignal = sp.sub8.update(cur_kstBuffer);
  sp.cur_outKST = cur_kstBuffer;
  sp.cur_outKSTSignal = cur_outKSTSignal;
}

function kstextOpenImpl(
  sp: KstextStream,
  inReal: Float64Array,
  startIdx: number,
  optInROC1Period: number,
  optInROC2Period: number,
  optInROC3Period: number,
  optInROC4Period: number,
  optInMA1Period: number,
  optInMA2Period: number,
  optInMA3Period: number,
  optInMA4Period: number,
  optInSignalPeriod: number,
  optInROCMAType: number,
  optInSignalMAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outKST: Float64Array,
  outKSTSignal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let kstBuffer!: Float64Array;
  let tempBuffer!: Float64Array;
  let lookbackTotal = 0;
  let lookbackSignal = 0;
  let lookbackMA = 0;
  let sigStart = 0;
  let lookbackMA1 = 0;
  let lookbackMA2 = 0;
  let lookbackMA3 = 0;
  let lookbackMA4 = 0;
  let tempInteger = 0;
  const tempBegIdx = new MInteger();
  const rocNb = new MInteger();
  const kstNb = new MInteger();
  const legNb = new MInteger();
  const sigNb = new MInteger();
  let i = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInROC1Period === Core.INTEGER_DEFAULT) {
    optInROC1Period = 10;
  } else if (optInROC1Period < 1 || optInROC1Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInROC2Period === Core.INTEGER_DEFAULT) {
    optInROC2Period = 15;
  } else if (optInROC2Period < 1 || optInROC2Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInROC3Period === Core.INTEGER_DEFAULT) {
    optInROC3Period = 20;
  } else if (optInROC3Period < 1 || optInROC3Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInROC4Period === Core.INTEGER_DEFAULT) {
    optInROC4Period = 30;
  } else if (optInROC4Period < 1 || optInROC4Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInMA1Period === Core.INTEGER_DEFAULT) {
    optInMA1Period = 10;
  } else if (optInMA1Period < 1 || optInMA1Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInMA2Period === Core.INTEGER_DEFAULT) {
    optInMA2Period = 10;
  } else if (optInMA2Period < 1 || optInMA2Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInMA3Period === Core.INTEGER_DEFAULT) {
    optInMA3Period = 10;
  } else if (optInMA3Period < 1 || optInMA3Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInMA4Period === Core.INTEGER_DEFAULT) {
    optInMA4Period = 15;
  } else if (optInMA4Period < 1 || optInMA4Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSignalPeriod === Core.INTEGER_DEFAULT) {
    optInSignalPeriod = 9;
  } else if (optInSignalPeriod < 1 || optInSignalPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInROCMAType === Core.INTEGER_DEFAULT || optInROCMAType === MAType.DEFAULT) {
    optInROCMAType = MAType.SMA;
  } else if (optInROCMAType < MATypes.Min || optInROCMAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if (optInSignalMAType === Core.INTEGER_DEFAULT || optInSignalMAType === MAType.DEFAULT) {
    optInSignalMAType = MAType.SMA;
  } else if (optInSignalMAType < MATypes.Min || optInSignalMAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen < kstextLookbackPositional(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType as MAType, optInSignalMAType as MAType, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const sc_outKST = outStride === 1 ? outKST : new Float64Array(historyLen);
  const sc_outKSTSignal = outStride === 1 ? outKSTSignal : new Float64Array(historyLen);
  /* With every type SMA this is bit-exact with kst(): each leg's average
   * starts on the first bar the signal consumes, and the line accumulates
   * its legs left to right. Changing either breaks the equality.
   */
  outBegIdx.value = 0;
  outNBElement.value = 0;
  /* lookbackTotal must stay kstext_lookback() of the same parameters. */
  lookbackSignal = maLookbackPositional(optInSignalPeriod, (optInSignalMAType as MAType), core);
  lookbackMA1 = maLookbackPositional(optInMA1Period, (optInROCMAType as MAType), core);
  lookbackMA2 = maLookbackPositional(optInMA2Period, (optInROCMAType as MAType), core);
  lookbackMA3 = maLookbackPositional(optInMA3Period, (optInROCMAType as MAType), core);
  lookbackMA4 = maLookbackPositional(optInMA4Period, (optInROCMAType as MAType), core);
  lookbackTotal = optInROC1Period + lookbackMA1;
  tempInteger = optInROC2Period + lookbackMA2;
  if (tempInteger > lookbackTotal) {
    lookbackTotal = tempInteger;
  }
  tempInteger = optInROC3Period + lookbackMA3;
  if (tempInteger > lookbackTotal) {
    lookbackTotal = tempInteger;
  }
  tempInteger = optInROC4Period + lookbackMA4;
  if (tempInteger > lookbackTotal) {
    lookbackTotal = tempInteger;
  }
  lookbackTotal += lookbackSignal;
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  sigStart = startIdx - lookbackSignal;
  /* Both buffers are sized by the longest leg average: a rate of change
   * starts its average's lookback before sigStart.
   */
  lookbackMA = lookbackMA1;
  if (lookbackMA2 > lookbackMA) {
    lookbackMA = lookbackMA2;
  }
  if (lookbackMA3 > lookbackMA) {
    lookbackMA = lookbackMA3;
  }
  if (lookbackMA4 > lookbackMA) {
    lookbackMA = lookbackMA4;
  }
  tempInteger = endIdx - sigStart + 1 + lookbackMA;
  kstBuffer = new Float64Array(tempInteger);
  tempBuffer = new Float64Array(tempInteger);
  const sub0 = rocOpenAndFillInternal(core, inReal, sigStart - lookbackMA1, optInROC1Period, tempBegIdx, rocNb, kstBuffer);
  const sub1 = maOpenInternal(core, kstBuffer.slice(0, (rocNb.value - 1) + 1), 0, optInMA1Period, (optInROCMAType as MAType));
  const _xr6 = maPositional(0, rocNb.value - 1, kstBuffer, optInMA1Period, (optInROCMAType as MAType), kstBuffer, core);
  tempBegIdx.value = _xr6.begIdx;
  kstNb.value = _xr6.count;
  const sub2 = rocOpenAndFillInternal(core, inReal, sigStart - lookbackMA2, optInROC2Period, tempBegIdx, rocNb, tempBuffer);
  const sub3 = maOpenInternal(core, tempBuffer.slice(0, (rocNb.value - 1) + 1), 0, optInMA2Period, (optInROCMAType as MAType));
  const _xr7 = maPositional(0, rocNb.value - 1, tempBuffer, optInMA2Period, (optInROCMAType as MAType), tempBuffer, core);
  tempBegIdx.value = _xr7.begIdx;
  legNb.value = _xr7.count;
  for (i = 0; i < kstNb.value; i += 1) {
    {
      const _a: number = 2.0, _b: number = tempBuffer[i], _c: number = kstBuffer[i];
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      kstBuffer[i] = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
  }
  const sub4 = rocOpenAndFillInternal(core, inReal, sigStart - lookbackMA3, optInROC3Period, tempBegIdx, rocNb, tempBuffer);
  const sub5 = maOpenInternal(core, tempBuffer.slice(0, (rocNb.value - 1) + 1), 0, optInMA3Period, (optInROCMAType as MAType));
  const _xr9 = maPositional(0, rocNb.value - 1, tempBuffer, optInMA3Period, (optInROCMAType as MAType), tempBuffer, core);
  tempBegIdx.value = _xr9.begIdx;
  legNb.value = _xr9.count;
  for (i = 0; i < kstNb.value; i += 1) {
    {
      const _a: number = 3.0, _b: number = tempBuffer[i], _c: number = kstBuffer[i];
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      kstBuffer[i] = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
  }
  const sub6 = rocOpenAndFillInternal(core, inReal, sigStart - lookbackMA4, optInROC4Period, tempBegIdx, rocNb, tempBuffer);
  const sub7 = maOpenInternal(core, tempBuffer.slice(0, (rocNb.value - 1) + 1), 0, optInMA4Period, (optInROCMAType as MAType));
  const _xr11 = maPositional(0, rocNb.value - 1, tempBuffer, optInMA4Period, (optInROCMAType as MAType), tempBuffer, core);
  tempBegIdx.value = _xr11.begIdx;
  legNb.value = _xr11.count;
  for (i = 0; i < kstNb.value; i += 1) {
    {
      const _a: number = 4.0, _b: number = tempBuffer[i], _c: number = kstBuffer[i];
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      kstBuffer[i] = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
  }
  /* Every read of inReal is done: an output may alias it. */
  sc_outKST.set(kstBuffer.subarray(lookbackSignal, lookbackSignal + (endIdx - startIdx + 1)), 0);
  const sub8 = maOpenAndFillInternal(core, kstBuffer.slice(0, (kstNb.value - 1) + 1), 0, optInSignalPeriod, (optInSignalMAType as MAType), tempBegIdx, sigNb, sc_outKSTSignal);
  outBegIdx.value = startIdx;
  outNBElement.value = sigNb.value;
  if (outNBElement.value < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  sp.optInROC1Period = optInROC1Period;
  sp.optInROC2Period = optInROC2Period;
  sp.optInROC3Period = optInROC3Period;
  sp.optInROC4Period = optInROC4Period;
  sp.optInMA1Period = optInMA1Period;
  sp.optInMA2Period = optInMA2Period;
  sp.optInMA3Period = optInMA3Period;
  sp.optInMA4Period = optInMA4Period;
  sp.optInSignalPeriod = optInSignalPeriod;
  sp.optInROCMAType = optInROCMAType;
  sp.optInSignalMAType = optInSignalMAType;
  sp.sub0 = sub0;
  sp.sub1 = sub1;
  sp.sub2 = sub2;
  sp.sub3 = sub3;
  sp.sub4 = sub4;
  sp.sub5 = sub5;
  sp.sub6 = sub6;
  sp.sub7 = sub7;
  sp.sub8 = sub8;
  sp.cur_outKST = sc_outKST[outNBElement.value - 1];
  sp.cur_outKSTSignal = sc_outKSTSignal[outNBElement.value - 1];
  return RetCode.SUCCESS;
}

/** @internal */
export function kstextOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInROC1Period: number,
  optInROC2Period: number,
  optInROC3Period: number,
  optInROC4Period: number,
  optInMA1Period: number,
  optInMA2Period: number,
  optInMA3Period: number,
  optInMA4Period: number,
  optInSignalPeriod: number,
  optInROCMAType: number,
  optInSignalMAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outKST: Float64Array,
  outKSTSignal: Float64Array,
): KstextStream {
  const sp = KstextStream.of(core);
  const retCode = kstextOpenImpl(sp, inReal, startIdx, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType, outBegIdx, outNBElement, outKST, outKSTSignal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('KSTEXT openAndFill', inReal.length, startIdx, kstextLookbackPositional(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType as MAType, optInSignalMAType as MAType, core));
  }
  if (retCode === RetCode.BAD_PARAM && kstextLookbackPositional(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType as MAType, optInSignalMAType as MAType, core) < 0) {
    throw rejectedParams('KSTEXT openAndFill', PARAM_RANGES, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType);
  }
  throw streamFailure('KSTEXT openAndFill', retCode);
}

/** @internal */
export function kstextOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInROC1Period: number,
  optInROC2Period: number,
  optInROC3Period: number,
  optInROC4Period: number,
  optInMA1Period: number,
  optInMA2Period: number,
  optInMA3Period: number,
  optInMA4Period: number,
  optInSignalPeriod: number,
  optInROCMAType: number,
  optInSignalMAType: number,
): KstextStream {
  const sp = KstextStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outKST = new Float64Array(1);
  const sink_outKSTSignal = new Float64Array(1);
  const retCode = kstextOpenImpl(sp, inReal, startIdx, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType, outBegIdx, outNBElement, sink_outKST, sink_outKSTSignal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('KSTEXT open', inReal.length, startIdx, kstextLookbackPositional(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType as MAType, optInSignalMAType as MAType, core));
  }
  if (retCode === RetCode.BAD_PARAM && kstextLookbackPositional(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType as MAType, optInSignalMAType as MAType, core) < 0) {
    throw rejectedParams('KSTEXT open', PARAM_RANGES, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType);
  }
  throw streamFailure('KSTEXT open', retCode);
}

/** @internal */
export function kstextOpenPositional(
  inReal: ArrayLike<number>,
  optInROC1Period: number,
  optInROC2Period: number,
  optInROC3Period: number,
  optInROC4Period: number,
  optInMA1Period: number,
  optInMA2Period: number,
  optInMA3Period: number,
  optInMA4Period: number,
  optInSignalPeriod: number,
  optInROCMAType: MAType | typeof Core.INTEGER_DEFAULT,
  optInSignalMAType: MAType | typeof Core.INTEGER_DEFAULT,
  core: Core = Core.DEFAULT,
): KstextStream {
  optInROC1Period = requireInt32('KSTEXT open', 'roc1Period', optInROC1Period);
  optInROC2Period = requireInt32('KSTEXT open', 'roc2Period', optInROC2Period);
  optInROC3Period = requireInt32('KSTEXT open', 'roc3Period', optInROC3Period);
  optInROC4Period = requireInt32('KSTEXT open', 'roc4Period', optInROC4Period);
  optInMA1Period = requireInt32('KSTEXT open', 'ma1Period', optInMA1Period);
  optInMA2Period = requireInt32('KSTEXT open', 'ma2Period', optInMA2Period);
  optInMA3Period = requireInt32('KSTEXT open', 'ma3Period', optInMA3Period);
  optInMA4Period = requireInt32('KSTEXT open', 'ma4Period', optInMA4Period);
  optInSignalPeriod = requireInt32('KSTEXT open', 'signalPeriod', optInSignalPeriod);
  optInROCMAType = requireInt32('KSTEXT open', 'rocMaType', optInROCMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  optInSignalMAType = requireInt32('KSTEXT open', 'signalMaType', optInSignalMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('KSTEXT open', 'real', inReal);
  requireHistory('KSTEXT open', 'real', inReal.length);
  return kstextOpenInternal(core, float64(inReal), 0, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType);
}

/** @internal */
export function kstextOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInROC1Period: number,
  optInROC2Period: number,
  optInROC3Period: number,
  optInROC4Period: number,
  optInMA1Period: number,
  optInMA2Period: number,
  optInMA3Period: number,
  optInMA4Period: number,
  optInSignalPeriod: number,
  optInROCMAType: MAType | typeof Core.INTEGER_DEFAULT,
  optInSignalMAType: MAType | typeof Core.INTEGER_DEFAULT,
  outKST: Float64Array,
  outKSTSignal: Float64Array,
  core: Core = Core.DEFAULT,
): KstextStream {
  optInROC1Period = requireInt32('KSTEXT openAndFill', 'roc1Period', optInROC1Period);
  optInROC2Period = requireInt32('KSTEXT openAndFill', 'roc2Period', optInROC2Period);
  optInROC3Period = requireInt32('KSTEXT openAndFill', 'roc3Period', optInROC3Period);
  optInROC4Period = requireInt32('KSTEXT openAndFill', 'roc4Period', optInROC4Period);
  optInMA1Period = requireInt32('KSTEXT openAndFill', 'ma1Period', optInMA1Period);
  optInMA2Period = requireInt32('KSTEXT openAndFill', 'ma2Period', optInMA2Period);
  optInMA3Period = requireInt32('KSTEXT openAndFill', 'ma3Period', optInMA3Period);
  optInMA4Period = requireInt32('KSTEXT openAndFill', 'ma4Period', optInMA4Period);
  optInSignalPeriod = requireInt32('KSTEXT openAndFill', 'signalPeriod', optInSignalPeriod);
  optInROCMAType = requireInt32('KSTEXT openAndFill', 'rocMaType', optInROCMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  optInSignalMAType = requireInt32('KSTEXT openAndFill', 'signalMaType', optInSignalMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('KSTEXT openAndFill', 'real', inReal);
  requireHistory('KSTEXT openAndFill', 'real', inReal.length);
  const guardLookback = kstextLookbackPositional(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType as MAType, optInSignalMAType as MAType, core);
  if (guardLookback < 0) {
    throw rejectedParams('KSTEXT openAndFill', PARAM_RANGES, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType);
  }
  const guardOutLen = openFillCount('KSTEXT openAndFill', inReal.length, guardLookback);
  requireOutput('KSTEXT openAndFill', 'outKst', outKST, guardOutLen, Float64Array);
  requireOutput('KSTEXT openAndFill', 'outKstSignal', outKSTSignal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outKST.buffer === inRealF64.buffer && outKST.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('KSTEXT openAndFill', 'outKst', 'real');
  }
  if (outKSTSignal.buffer === inRealF64.buffer && outKSTSignal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('KSTEXT openAndFill', 'outKstSignal', 'real');
  }
  if (outKST.buffer === outKSTSignal.buffer && outKST.byteOffset === outKSTSignal.byteOffset) {
    throw sharedMemory('KSTEXT openAndFill', 'outKst', 'outKstSignal');
  }
  if (guardOutLen > 0 && overlaps(outKST, inRealF64)) {
    throw sharedMemory('KSTEXT openAndFill', 'outKst', 'real');
  }
  if (guardOutLen > 0 && overlaps(outKSTSignal, inRealF64)) {
    throw sharedMemory('KSTEXT openAndFill', 'outKstSignal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outKST, outKSTSignal)) {
    throw sharedMemory('KSTEXT openAndFill', 'outKst', 'outKstSignal');
  }
  return kstextOpenAndFillInternal(core, inRealF64, 0, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType, new MInteger(), new MInteger(), outKST, outKSTSignal);
}

/**
 * Opens a live KSTEXT stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link kstext} there.
 *
 * @param real - Warm-up history. Source price series (canonically the close)
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link KstextOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `kstextLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32 or a parameter outside its
 *   range.
 *
 * @example
 * ```ts
 * import { MAType, kstextOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = kstextOpen(data, {
 *   roc1Period: 10,
 *   roc2Period: 15,
 *   roc3Period: 20,
 *   roc4Period: 30,
 *   ma1Period: 10,
 *   ma2Period: 10,
 *   ma3Period: 10,
 *   ma4Period: 15,
 *   signalPeriod: 9,
 *   rocMaType: MAType.SMA,
 *   signalMaType: MAType.SMA,
 * });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function kstextOpen(real: ArrayLike<number>, options?: KstextOptions): KstextStream {
  const o = optionsOf('KSTEXT open', options);
  const optInROC1Period = orDefault(o.roc1Period, Core.INTEGER_DEFAULT);
  const optInROC2Period = orDefault(o.roc2Period, Core.INTEGER_DEFAULT);
  const optInROC3Period = orDefault(o.roc3Period, Core.INTEGER_DEFAULT);
  const optInROC4Period = orDefault(o.roc4Period, Core.INTEGER_DEFAULT);
  const optInMA1Period = orDefault(o.ma1Period, Core.INTEGER_DEFAULT);
  const optInMA2Period = orDefault(o.ma2Period, Core.INTEGER_DEFAULT);
  const optInMA3Period = orDefault(o.ma3Period, Core.INTEGER_DEFAULT);
  const optInMA4Period = orDefault(o.ma4Period, Core.INTEGER_DEFAULT);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const optInROCMAType = orDefault(o.rocMaType, Core.INTEGER_DEFAULT);
  const optInSignalMAType = orDefault(o.signalMaType, Core.INTEGER_DEFAULT);
  const core = coreOf('KSTEXT open', o.core);
  return kstextOpenPositional(real, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType, core);
}

/**
 * {@link kstextOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link kstext}.
 *
 * @param real - Warm-up history. Source price series (canonically the close)
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link KstextOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link kstext} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `kstextLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range
 *   or a buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { MAType, kstextOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const {
 *   stream,
 *   begIdx,
 *   outKst,
 *   outKstSignal,
 * } = kstextOpenAndFill(data, {
 *   roc1Period: 10,
 *   roc2Period: 15,
 *   roc3Period: 20,
 *   roc4Period: 30,
 *   ma1Period: 10,
 *   ma2Period: 10,
 *   ma3Period: 10,
 *   ma4Period: 15,
 *   signalPeriod: 9,
 *   rocMaType: MAType.SMA,
 *   signalMaType: MAType.SMA,
 * });
 * // outKst[k] is the value at bar begIdx + k
 * ```
 */
export function kstextOpenAndFill(real: ArrayLike<number>, options?: KstextOptions): KstextResult & { stream: KstextStream } {
  const o = optionsOf('KSTEXT openAndFill', options);
  const optInROC1Period = orDefault(o.roc1Period, Core.INTEGER_DEFAULT);
  const optInROC2Period = orDefault(o.roc2Period, Core.INTEGER_DEFAULT);
  const optInROC3Period = orDefault(o.roc3Period, Core.INTEGER_DEFAULT);
  const optInROC4Period = orDefault(o.roc4Period, Core.INTEGER_DEFAULT);
  const optInMA1Period = orDefault(o.ma1Period, Core.INTEGER_DEFAULT);
  const optInMA2Period = orDefault(o.ma2Period, Core.INTEGER_DEFAULT);
  const optInMA3Period = orDefault(o.ma3Period, Core.INTEGER_DEFAULT);
  const optInMA4Period = orDefault(o.ma4Period, Core.INTEGER_DEFAULT);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const optInROCMAType = orDefault(o.rocMaType, Core.INTEGER_DEFAULT);
  const optInSignalMAType = orDefault(o.signalMaType, Core.INTEGER_DEFAULT);
  const core = coreOf('KSTEXT openAndFill', o.core);
  const capacity = fillCapacity(kstextLookbackPositional(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType, core), real);
  const outKST = o.outKst === undefined ? new Float64Array(capacity) : o.outKst;
  const outKSTSignal = o.outKstSignal === undefined ? new Float64Array(capacity) : o.outKstSignal;
  const stream = kstextOpenAndFillPositional(real, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType, outKST, outKSTSignal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outKst: outKST.subarray(0, range.count), outKstSignal: outKSTSignal.subarray(0, range.count) };
}
