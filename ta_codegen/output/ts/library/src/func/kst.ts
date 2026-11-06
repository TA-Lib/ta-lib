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
 *  092926 MF,CC  First version (#472).
 */

import { badSink, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function kstLookbackPositional(optInROC1Period: number, optInROC2Period: number, optInROC3Period: number, optInROC4Period: number, optInSMA1Period: number, optInSMA2Period: number, optInSMA3Period: number, optInSMA4Period: number, optInSignalPeriod: number, _core: Core = Core.DEFAULT): number {
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
  if ((optInSMA1Period | 0) !== optInSMA1Period) {
    return -1;
  }
  if ((optInSMA2Period | 0) !== optInSMA2Period) {
    return -1;
  }
  if ((optInSMA3Period | 0) !== optInSMA3Period) {
    return -1;
  }
  if ((optInSMA4Period | 0) !== optInSMA4Period) {
    return -1;
  }
  if ((optInSignalPeriod | 0) !== optInSignalPeriod) {
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
  if (optInSMA1Period === Core.INTEGER_DEFAULT) {
    optInSMA1Period = 10;
  } else if (optInSMA1Period < 1 || optInSMA1Period > 100000) {
    return -1;
  }
  if (optInSMA2Period === Core.INTEGER_DEFAULT) {
    optInSMA2Period = 10;
  } else if (optInSMA2Period < 1 || optInSMA2Period > 100000) {
    return -1;
  }
  if (optInSMA3Period === Core.INTEGER_DEFAULT) {
    optInSMA3Period = 10;
  } else if (optInSMA3Period < 1 || optInSMA3Period > 100000) {
    return -1;
  }
  if (optInSMA4Period === Core.INTEGER_DEFAULT) {
    optInSMA4Period = 15;
  } else if (optInSMA4Period < 1 || optInSMA4Period > 100000) {
    return -1;
  }
  if (optInSignalPeriod === Core.INTEGER_DEFAULT) {
    optInSignalPeriod = 9;
  } else if (optInSignalPeriod < 1 || optInSignalPeriod > 100000) {
    return -1;
  }
  let legMax = 0;
  let leg = 0;
  legMax = optInROC1Period + optInSMA1Period - 1;
  leg = optInROC2Period + optInSMA2Period - 1;
  if (leg > legMax) {
    legMax = leg;
  }
  leg = optInROC3Period + optInSMA3Period - 1;
  if (leg > legMax) {
    legMax = leg;
  }
  leg = optInROC4Period + optInSMA4Period - 1;
  if (leg > legMax) {
    legMax = leg;
  }
  return legMax + optInSignalPeriod - 1;
}

/** @internal */
export function kstDisplayShiftPositional(optInROC1Period: number, optInROC2Period: number, optInROC3Period: number, optInROC4Period: number, optInSMA1Period: number, optInSMA2Period: number, optInSMA3Period: number, optInSMA4Period: number, optInSignalPeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (kstLookbackPositional(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 2) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function kstImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInROC1Period: number,
  optInROC2Period: number,
  optInROC3Period: number,
  optInROC4Period: number,
  optInSMA1Period: number,
  optInSMA2Period: number,
  optInSMA3Period: number,
  optInSMA4Period: number,
  optInSignalPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outKST: Float64Array,
  outKSTSignal: Float64Array,
): RetCode {
  let outIdx = 0;
  let inIdx = 0;
  let lookbackTotal = 0;
  let sigStart = 0;
  let den1 = 0;
  let den2 = 0;
  let den3 = 0;
  let den4 = 0;
  let prior = 0;
  let roc = 0;
  let kst = 0;
  let sig = 0;
  let total1 = 0;
  let total2 = 0;
  let total3 = 0;
  let total4 = 0;
  let sigTotal = 0;
  let rcma1 = 0;
  let rcma2 = 0;
  let rcma3 = 0;
  let rcma4 = 0;
  let ring1!: Float64Array;
  let ring1_Idx = 0;
  let maxIdx_ring1 = (30) - 1;
  let ring2!: Float64Array;
  let ring2_Idx = 0;
  let maxIdx_ring2 = (30) - 1;
  let ring3!: Float64Array;
  let ring3_Idx = 0;
  let maxIdx_ring3 = (30) - 1;
  let ring4!: Float64Array;
  let ring4_Idx = 0;
  let maxIdx_ring4 = (30) - 1;
  let sigRing!: Float64Array;
  let sigRing_Idx = 0;
  let maxIdx_sigRing = (30) - 1;
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
  if (optInSMA1Period === Core.INTEGER_DEFAULT) {
    optInSMA1Period = 10;
  } else if (optInSMA1Period < 1 || optInSMA1Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSMA2Period === Core.INTEGER_DEFAULT) {
    optInSMA2Period = 10;
  } else if (optInSMA2Period < 1 || optInSMA2Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSMA3Period === Core.INTEGER_DEFAULT) {
    optInSMA3Period = 10;
  } else if (optInSMA3Period < 1 || optInSMA3Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSMA4Period === Core.INTEGER_DEFAULT) {
    optInSMA4Period = 15;
  } else if (optInSMA4Period < 1 || optInSMA4Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSignalPeriod === Core.INTEGER_DEFAULT) {
    optInSignalPeriod = 9;
  } else if (optInSignalPeriod < 1 || optInSignalPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if ((outKST.buffer === outKSTSignal.buffer && outKST.byteOffset === outKSTSignal.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  /* Bit-exact with TA_ROC per leg into TA_SMA, each SMA called at the start
   * its consumer needs: keep every running sum's add/subtract order and the
   * left-to-right weighted sum, or the composite differential stops being
   * exact. Each ring holds its stage's last SMA-period values, so the only
   * inputs read are the current bar and each leg's lagged denominator,
   * both at or after the slot the outputs write: outKST or outKSTSignal
   * may alias inReal.
   */
  lookbackTotal = kstLookbackPositional(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  if (optInSMA1Period < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  ring1 = new Float64Array(optInSMA1Period);
  maxIdx_ring1 = optInSMA1Period - 1;
  ring1_Idx = 0;
  if (optInSMA2Period < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  ring2 = new Float64Array(optInSMA2Period);
  maxIdx_ring2 = optInSMA2Period - 1;
  ring2_Idx = 0;
  if (optInSMA3Period < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  ring3 = new Float64Array(optInSMA3Period);
  maxIdx_ring3 = optInSMA3Period - 1;
  ring3_Idx = 0;
  if (optInSMA4Period < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  ring4 = new Float64Array(optInSMA4Period);
  maxIdx_ring4 = optInSMA4Period - 1;
  ring4_Idx = 0;
  if (optInSignalPeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sigRing = new Float64Array(optInSignalPeriod);
  maxIdx_sigRing = optInSignalPeriod - 1;
  sigRing_Idx = 0;
  sigStart = startIdx - (optInSignalPeriod - 1);
  sigTotal = 0.0;
  total4 = sigTotal;
  total3 = total4;
  total2 = total3;
  total1 = total2;
  inIdx = sigStart - (optInSMA1Period - 1);
  den1 = inIdx - optInROC1Period;
  while (inIdx < sigStart) {
    prior = inReal[den1];
    den1 += 1;
    roc = (prior !== 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
    total1 += roc;
    ring1[ring1_Idx] = roc;
    ring1_Idx++;
    if (ring1_Idx > maxIdx_ring1) {
      ring1_Idx = 0;
    }
    inIdx += 1;
  }
  inIdx = sigStart - (optInSMA2Period - 1);
  den2 = inIdx - optInROC2Period;
  while (inIdx < sigStart) {
    prior = inReal[den2];
    den2 += 1;
    roc = (prior !== 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
    total2 += roc;
    ring2[ring2_Idx] = roc;
    ring2_Idx++;
    if (ring2_Idx > maxIdx_ring2) {
      ring2_Idx = 0;
    }
    inIdx += 1;
  }
  inIdx = sigStart - (optInSMA3Period - 1);
  den3 = inIdx - optInROC3Period;
  while (inIdx < sigStart) {
    prior = inReal[den3];
    den3 += 1;
    roc = (prior !== 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
    total3 += roc;
    ring3[ring3_Idx] = roc;
    ring3_Idx++;
    if (ring3_Idx > maxIdx_ring3) {
      ring3_Idx = 0;
    }
    inIdx += 1;
  }
  inIdx = sigStart - (optInSMA4Period - 1);
  den4 = inIdx - optInROC4Period;
  while (inIdx < sigStart) {
    prior = inReal[den4];
    den4 += 1;
    roc = (prior !== 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
    total4 += roc;
    ring4[ring4_Idx] = roc;
    ring4_Idx++;
    if (ring4_Idx > maxIdx_ring4) {
      ring4_Idx = 0;
    }
    inIdx += 1;
  }
  inIdx = sigStart;
  while (inIdx < startIdx) {
    prior = inReal[den1];
    den1 += 1;
    roc = (prior !== 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
    total1 += roc;
    rcma1 = total1 / optInSMA1Period;
    ring1[ring1_Idx] = roc;
    ring1_Idx++;
    if (ring1_Idx > maxIdx_ring1) {
      ring1_Idx = 0;
    }
    total1 -= ring1[ring1_Idx];
    prior = inReal[den2];
    den2 += 1;
    roc = (prior !== 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
    total2 += roc;
    rcma2 = total2 / optInSMA2Period;
    ring2[ring2_Idx] = roc;
    ring2_Idx++;
    if (ring2_Idx > maxIdx_ring2) {
      ring2_Idx = 0;
    }
    total2 -= ring2[ring2_Idx];
    prior = inReal[den3];
    den3 += 1;
    roc = (prior !== 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
    total3 += roc;
    rcma3 = total3 / optInSMA3Period;
    ring3[ring3_Idx] = roc;
    ring3_Idx++;
    if (ring3_Idx > maxIdx_ring3) {
      ring3_Idx = 0;
    }
    total3 -= ring3[ring3_Idx];
    prior = inReal[den4];
    den4 += 1;
    roc = (prior !== 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
    total4 += roc;
    rcma4 = total4 / optInSMA4Period;
    ring4[ring4_Idx] = roc;
    ring4_Idx++;
    if (ring4_Idx > maxIdx_ring4) {
      ring4_Idx = 0;
    }
    total4 -= ring4[ring4_Idx];
    let _fma0: number;
    {
      const _a: number = 2.0, _b: number = rcma2, _c: number = rcma1;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      _fma0 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    let _fma1: number;
    {
      const _a: number = 3.0, _b: number = rcma3, _c: number = _fma0;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      _fma1 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = 4.0, _b: number = rcma4, _c: number = _fma1;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      kst = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    sigTotal += kst;
    sigRing[sigRing_Idx] = kst;
    sigRing_Idx++;
    if (sigRing_Idx > maxIdx_sigRing) {
      sigRing_Idx = 0;
    }
    inIdx += 1;
  }
  outIdx = 0;
  while (inIdx <= endIdx) {
    prior = inReal[den1];
    den1 += 1;
    roc = (prior !== 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
    total1 += roc;
    rcma1 = total1 / optInSMA1Period;
    ring1[ring1_Idx] = roc;
    ring1_Idx++;
    if (ring1_Idx > maxIdx_ring1) {
      ring1_Idx = 0;
    }
    total1 -= ring1[ring1_Idx];
    prior = inReal[den2];
    den2 += 1;
    roc = (prior !== 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
    total2 += roc;
    rcma2 = total2 / optInSMA2Period;
    ring2[ring2_Idx] = roc;
    ring2_Idx++;
    if (ring2_Idx > maxIdx_ring2) {
      ring2_Idx = 0;
    }
    total2 -= ring2[ring2_Idx];
    prior = inReal[den3];
    den3 += 1;
    roc = (prior !== 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
    total3 += roc;
    rcma3 = total3 / optInSMA3Period;
    ring3[ring3_Idx] = roc;
    ring3_Idx++;
    if (ring3_Idx > maxIdx_ring3) {
      ring3_Idx = 0;
    }
    total3 -= ring3[ring3_Idx];
    prior = inReal[den4];
    den4 += 1;
    roc = (prior !== 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
    total4 += roc;
    rcma4 = total4 / optInSMA4Period;
    ring4[ring4_Idx] = roc;
    ring4_Idx++;
    if (ring4_Idx > maxIdx_ring4) {
      ring4_Idx = 0;
    }
    total4 -= ring4[ring4_Idx];
    let _fma3: number;
    {
      const _a: number = 2.0, _b: number = rcma2, _c: number = rcma1;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      _fma3 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    let _fma4: number;
    {
      const _a: number = 3.0, _b: number = rcma3, _c: number = _fma3;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      _fma4 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = 4.0, _b: number = rcma4, _c: number = _fma4;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      kst = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    sigTotal += kst;
    sig = sigTotal / optInSignalPeriod;
    sigRing[sigRing_Idx] = kst;
    sigRing_Idx++;
    if (sigRing_Idx > maxIdx_sigRing) {
      sigRing_Idx = 0;
    }
    sigTotal -= sigRing[sigRing_Idx];
    outKST[outIdx] = kst;
    outKSTSignal[outIdx] = sig;
    outIdx += 1;
    inIdx += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['roc1Period', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['roc2Period', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['roc3Period', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['roc4Period', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['sma1Period', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['sma2Period', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['sma3Period', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['sma4Period', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['signalPeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000']] as const;

/** @internal */
export function kstPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInROC1Period: number,
  optInROC2Period: number,
  optInROC3Period: number,
  optInROC4Period: number,
  optInSMA1Period: number,
  optInSMA2Period: number,
  optInSMA3Period: number,
  optInSMA4Period: number,
  optInSignalPeriod: number,
  outKST: Float64Array,
  outKSTSignal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('KST', 'startIdx', startIdx);
  endIdx = requireInt32('KST', 'endIdx', endIdx);
  optInROC1Period = requireInt32('KST', 'roc1Period', optInROC1Period);
  optInROC2Period = requireInt32('KST', 'roc2Period', optInROC2Period);
  optInROC3Period = requireInt32('KST', 'roc3Period', optInROC3Period);
  optInROC4Period = requireInt32('KST', 'roc4Period', optInROC4Period);
  optInSMA1Period = requireInt32('KST', 'sma1Period', optInSMA1Period);
  optInSMA2Period = requireInt32('KST', 'sma2Period', optInSMA2Period);
  optInSMA3Period = requireInt32('KST', 'sma3Period', optInSMA3Period);
  optInSMA4Period = requireInt32('KST', 'sma4Period', optInSMA4Period);
  optInSignalPeriod = requireInt32('KST', 'signalPeriod', optInSignalPeriod);
  requireIndexRange('KST', startIdx, endIdx);
  const guardLookback = kstLookbackPositional(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('KST', PARAM_RANGES, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod);
  }
  const guardStart = clampedStart('KST', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('KST', 'real', inReal, guardInLen);
  requireOutput('KST', 'outKst', outKST, guardOutLen, Float64Array);
  requireOutput('KST', 'outKstSignal', outKSTSignal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outKST, inRealF64)) {
    throw sharedMemory('KST', 'outKst', 'real');
  }
  if (overlapsPartly(outKSTSignal, inRealF64)) {
    throw sharedMemory('KST', 'outKstSignal', 'real');
  }
  if (overlaps(outKST, outKSTSignal)) {
    throw sharedMemory('KST', 'outKst', 'outKstSignal');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = kstImpl(core, startIdx, endIdx, inRealF64, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod, outBegIdx, outNBElement, outKST, outKSTSignal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('KST', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link kst}, {@link kstLookback}, {@link kstDisplayShift},
 * {@link kstOpen} and {@link kstOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface KstOptions {
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
   * Simple-moving-average period smoothing leg 1 (default 10; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  sma1Period?: number | undefined;
  /**
   * Simple-moving-average period smoothing leg 2 (default 10; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  sma2Period?: number | undefined;
  /**
   * Simple-moving-average period smoothing leg 3 (default 10; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  sma3Period?: number | undefined;
  /**
   * Simple-moving-average period smoothing leg 4 (default 15; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  sma4Period?: number | undefined;
  /**
   * Simple-moving-average period of the signal line (default 9; range 1 to 100000;
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
   * Buffer for `outKst`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outKst?: Float64Array | undefined;
  /**
   * Buffer for `outKstSignal`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outKstSignal?: Float64Array | undefined;
}

/** What {@link kst} returns, and {@link kstOpenAndFill} with its stream. */
export interface KstResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /** Know Sure Thing line. `outKst[k]` is the value at input bar `begIdx + k`. */
  outKst: Float64Array;
  /**
   * Simple moving average of the line. `outKstSignal[k]` is the value at input bar
   * `begIdx + k`.
   */
  outKstSignal: Float64Array;
}

/**
 * Number of leading input bars {@link kst} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link KstOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function kstLookback(options?: KstOptions): number {
  const o = optionsOf('KST lookback', options);
  return kstLookbackPositional(orDefault(o.roc1Period, Core.INTEGER_DEFAULT), orDefault(o.roc2Period, Core.INTEGER_DEFAULT), orDefault(o.roc3Period, Core.INTEGER_DEFAULT), orDefault(o.roc4Period, Core.INTEGER_DEFAULT), orDefault(o.sma1Period, Core.INTEGER_DEFAULT), orDefault(o.sma2Period, Core.INTEGER_DEFAULT), orDefault(o.sma3Period, Core.INTEGER_DEFAULT), orDefault(o.sma4Period, Core.INTEGER_DEFAULT), orDefault(o.signalPeriod, Core.INTEGER_DEFAULT), coreOf('KST lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link kst}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link KstOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function kstDisplayShift(outputIdx: number, options?: KstOptions): number {
  const o = optionsOf('KST displayShift', options);
  return kstDisplayShiftPositional(orDefault(o.roc1Period, Core.INTEGER_DEFAULT), orDefault(o.roc2Period, Core.INTEGER_DEFAULT), orDefault(o.roc3Period, Core.INTEGER_DEFAULT), orDefault(o.roc4Period, Core.INTEGER_DEFAULT), orDefault(o.sma1Period, Core.INTEGER_DEFAULT), orDefault(o.sma2Period, Core.INTEGER_DEFAULT), orDefault(o.sma3Period, Core.INTEGER_DEFAULT), orDefault(o.sma4Period, Core.INTEGER_DEFAULT), orDefault(o.signalPeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('KST displayShift', o.core));
}

/**
 * Know Sure Thing: Martin J. Pring's momentum oscillator, a weighted sum of four
 * smoothed rates of change with a moving-average signal line. Each leg smooths a
 * rate of change over a different span, and the longer legs carry the larger
 * weights, so the line follows the dominant trend while the short legs make it
 * turn early. Readings above zero mean the combined momentum is positive. The
 * usual signals are the line crossing its signal line and the line changing
 * direction. The level is unbounded, and the usual comparison is against the
 * line's own history.
 *
 * Formula and more info at
 * [ta-lib.org/functions/kst](https://ta-lib.org/functions/kst).
 *
 * **Notes**
 * - **The input is assumed positive.** The price each rate of change looks back to
 *   is its divisor. Near zero the divisor is small and the output very large; no
 *   error is returned. A series that reaches or crosses zero, such as an
 *   oscillator or a spread, has no meaningful result here.
 * - The weights are 1 to 4 and nothing divides by their total. Pring's daily table
 *   prints each leg's SMA period times its weight (a total of 120) only to show
 *   how much the longest leg dominates.
 * - Both outputs start at the first bar where the signal line exists, so the first
 *   few bars where the KST line alone is defined are not emitted. Setting the
 *   signal period to 1 returns the line from its own first bar, and the signal
 *   output is then a copy of it.
 * - Each rate of change follows `ROC`: a zero price in the denominator makes that
 *   term 0.
 * - The legs can be in any order. The weights go with leg position, not with the
 *   length of the rate of change.
 * - Pring's daily page uses a 10-day signal. The default signal period follows the
 *   charting platforms instead.
 *
 * `outKst[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link kstLookback} is a success with empty outputs.
 *
 * @param real - Source price series (canonically the close)
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link KstOptions}.
 * @returns `begIdx` and the values; see {@link KstResult}. Each output is a view
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
 * import { kst } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, outKst, outKstSignal } = kst(data, {
 *   roc1Period: 10,
 *   roc2Period: 15,
 *   roc3Period: 20,
 *   roc4Period: 30,
 *   sma1Period: 10,
 *   sma2Period: 10,
 *   sma3Period: 10,
 *   sma4Period: 15,
 *   signalPeriod: 9,
 * });
 * // outKst[k] is the value at bar begIdx + k
 * ```
 * @see [`kstext`](https://ta-lib.org/functions/kstext)
 * @see [`roc`](https://ta-lib.org/functions/roc)
 * @see [`sma`](https://ta-lib.org/functions/sma)
 * @see [`coppock`](https://ta-lib.org/functions/coppock)
 * @see [`macd`](https://ta-lib.org/functions/macd)
 */
export function kst(real: ArrayLike<number>, options?: KstOptions): KstResult {
  const o = optionsOf('KST', options);
  if (o.endIdx === undefined) {
    requireSeries('KST', 'real', real);
  }
  const optInROC1Period = orDefault(o.roc1Period, Core.INTEGER_DEFAULT);
  const optInROC2Period = orDefault(o.roc2Period, Core.INTEGER_DEFAULT);
  const optInROC3Period = orDefault(o.roc3Period, Core.INTEGER_DEFAULT);
  const optInROC4Period = orDefault(o.roc4Period, Core.INTEGER_DEFAULT);
  const optInSMA1Period = orDefault(o.sma1Period, Core.INTEGER_DEFAULT);
  const optInSMA2Period = orDefault(o.sma2Period, Core.INTEGER_DEFAULT);
  const optInSMA3Period = orDefault(o.sma3Period, Core.INTEGER_DEFAULT);
  const optInSMA4Period = orDefault(o.sma4Period, Core.INTEGER_DEFAULT);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('KST', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, kstLookbackPositional(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod, core), real);
  const outKST = o.outKst === undefined ? new Float64Array(capacity) : o.outKst;
  const outKSTSignal = o.outKstSignal === undefined ? new Float64Array(capacity) : o.outKstSignal;
  const range = kstPositional(startIdx, endIdx, real, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod, outKST, outKSTSignal, core);
  return { begIdx: range.begIdx, outKst: outKST.subarray(0, range.count), outKstSignal: outKSTSignal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live KST stream: one value per closed bar, bit-identical to {@link kst} over
 * the same series. Open it with {@link kstOpen} or {@link kstOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/kst](https://ta-lib.org/functions/kst).
 */
export class KstStream {
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
  optInSMA1Period: number = 0;
  /** @internal */
  optInSMA2Period: number = 0;
  /** @internal */
  optInSMA3Period: number = 0;
  /** @internal */
  optInSMA4Period: number = 0;
  /** @internal */
  optInSignalPeriod: number = 0;
  /** @internal */
  total1: number = 0;
  /** @internal */
  total2: number = 0;
  /** @internal */
  total3: number = 0;
  /** @internal */
  total4: number = 0;
  /** @internal */
  sigTotal: number = 0;
  /** @internal */
  ring1_Idx: number = 0;
  /** @internal */
  ring2_Idx: number = 0;
  /** @internal */
  ring3_Idx: number = 0;
  /** @internal */
  ring4_Idx: number = 0;
  /** @internal */
  sigRing_Idx: number = 0;
  /** @internal */
  maxIdx_ring1: number = 0;
  /** @internal */
  maxIdx_ring2: number = 0;
  /** @internal */
  maxIdx_ring3: number = 0;
  /** @internal */
  maxIdx_ring4: number = 0;
  /** @internal */
  maxIdx_sigRing: number = 0;
  /** @internal */
  ringPos_den1: number = 0;
  /** @internal */
  ringCap_den1: number = 0;
  /** @internal */
  ring_den1_inReal: Float64Array = new Float64Array(1);
  /** @internal */
  ringPos_den2: number = 0;
  /** @internal */
  ringCap_den2: number = 0;
  /** @internal */
  ring_den2_inReal: Float64Array = new Float64Array(1);
  /** @internal */
  ringPos_den3: number = 0;
  /** @internal */
  ringCap_den3: number = 0;
  /** @internal */
  ring_den3_inReal: Float64Array = new Float64Array(1);
  /** @internal */
  ringPos_den4: number = 0;
  /** @internal */
  ringCap_den4: number = 0;
  /** @internal */
  ring_den4_inReal: Float64Array = new Float64Array(1);
  /** @internal */
  cbSize_ring1: number = 0;
  /** @internal */
  cb_ring1: Float64Array = new Float64Array(1);
  /** @internal */
  cbSize_ring2: number = 0;
  /** @internal */
  cb_ring2: Float64Array = new Float64Array(1);
  /** @internal */
  cbSize_ring3: number = 0;
  /** @internal */
  cb_ring3: Float64Array = new Float64Array(1);
  /** @internal */
  cbSize_ring4: number = 0;
  /** @internal */
  cb_ring4: Float64Array = new Float64Array(1);
  /** @internal */
  cbSize_sigRing: number = 0;
  /** @internal */
  cb_sigRing: Float64Array = new Float64Array(1);
  /** @internal */
  cur_outKST: number = 0;
  /** @internal */
  cur_outKSTSignal: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): KstStream {
    return new KstStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link kst} reports them over the same bars. Each accepted `update` and each
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
      throw failure('KST advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns its outputs.
   *
   * @param inReal - The bar's `inReal` value: Source price series (canonically the
   *   close)
   * @param out - Receives the outputs and is returned; a new {@link KstOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number, out?: KstOut): KstOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('KST update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('KST update', 'KstOut');
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('KST update', 'inReal');
    }
    kstStepImpl(this, inReal);
    this.outRangeCount++;
    if (out === undefined) {
      out = new KstOut();
    }
    out.outKst = this.cur_outKST;
    out.outKstSignal = this.cur_outKSTSignal;
    return out;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Source price series (canonically the
   *   close)
   * @param out - Receives the outputs and is returned; a new {@link KstOut} when
   *   omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number, out?: KstOut): KstOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('KST peek', 'KstOut');
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('KST peek', 'inReal');
    }
    const sp = this;
    let prior = 0;
    let roc = 0;
    let kst = 0;
    let sig = 0;
    let rcma1 = 0;
    let rcma2 = 0;
    let rcma3 = 0;
    let rcma4 = 0;
    let cur_outKST = 0;
    let cur_outKSTSignal = 0;
    let ring1_Idx = sp.ring1_Idx;
    let ring2_Idx = sp.ring2_Idx;
    let ring3_Idx = sp.ring3_Idx;
    let ring4_Idx = sp.ring4_Idx;
    let sigRing_Idx = sp.sigRing_Idx;
    let sigTotal = sp.sigTotal;
    let total1 = sp.total1;
    let total2 = sp.total2;
    let total3 = sp.total3;
    let total4 = sp.total4;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    let pkSlot1 = -1;
    let pkVal1 = 0;
    let pkSlot2 = -1;
    let pkVal2 = 0;
    let pkSlot3 = -1;
    let pkVal3 = 0;
    let pkSlot4 = -1;
    let pkVal4 = 0;
    let pkSlot5 = -1;
    let pkVal5 = 0;
    let pkSlot6 = -1;
    let pkVal6 = 0;
    let pkSlot7 = -1;
    let pkVal7 = 0;
    let pkSlot8 = -1;
    let pkVal8 = 0;
    if (sp.ringCap_den1 === 0) {
      pkSlot0 = 0;
      pkVal0 = inReal;
    }
    if (sp.ringCap_den2 === 0) {
      pkSlot1 = 0;
      pkVal1 = inReal;
    }
    if (sp.ringCap_den3 === 0) {
      pkSlot2 = 0;
      pkVal2 = inReal;
    }
    if (sp.ringCap_den4 === 0) {
      pkSlot3 = 0;
      pkVal3 = inReal;
    }
    prior = (sp.ringPos_den1 !== pkSlot0) ? sp.ring_den1_inReal[sp.ringPos_den1] : pkVal0;
    roc = (prior !== 0.0) ? (inReal / prior - 1.0) * 100.0 : 0.0;
    total1 += roc;
    rcma1 = total1 / sp.optInSMA1Period;
    pkSlot4 = ring1_Idx;
    pkVal4 = roc;
    ring1_Idx = ring1_Idx + 1;
    if (ring1_Idx > sp.maxIdx_ring1) {
      ring1_Idx = 0;
    }
    total1 -= (ring1_Idx !== pkSlot4) ? sp.cb_ring1[ring1_Idx] : pkVal4;
    prior = (sp.ringPos_den2 !== pkSlot1) ? sp.ring_den2_inReal[sp.ringPos_den2] : pkVal1;
    roc = (prior !== 0.0) ? (inReal / prior - 1.0) * 100.0 : 0.0;
    total2 += roc;
    rcma2 = total2 / sp.optInSMA2Period;
    pkSlot5 = ring2_Idx;
    pkVal5 = roc;
    ring2_Idx = ring2_Idx + 1;
    if (ring2_Idx > sp.maxIdx_ring2) {
      ring2_Idx = 0;
    }
    total2 -= (ring2_Idx !== pkSlot5) ? sp.cb_ring2[ring2_Idx] : pkVal5;
    prior = (sp.ringPos_den3 !== pkSlot2) ? sp.ring_den3_inReal[sp.ringPos_den3] : pkVal2;
    roc = (prior !== 0.0) ? (inReal / prior - 1.0) * 100.0 : 0.0;
    total3 += roc;
    rcma3 = total3 / sp.optInSMA3Period;
    pkSlot6 = ring3_Idx;
    pkVal6 = roc;
    ring3_Idx = ring3_Idx + 1;
    if (ring3_Idx > sp.maxIdx_ring3) {
      ring3_Idx = 0;
    }
    total3 -= (ring3_Idx !== pkSlot6) ? sp.cb_ring3[ring3_Idx] : pkVal6;
    prior = (sp.ringPos_den4 !== pkSlot3) ? sp.ring_den4_inReal[sp.ringPos_den4] : pkVal3;
    roc = (prior !== 0.0) ? (inReal / prior - 1.0) * 100.0 : 0.0;
    total4 += roc;
    rcma4 = total4 / sp.optInSMA4Period;
    pkSlot7 = ring4_Idx;
    pkVal7 = roc;
    ring4_Idx = ring4_Idx + 1;
    if (ring4_Idx > sp.maxIdx_ring4) {
      ring4_Idx = 0;
    }
    total4 -= (ring4_Idx !== pkSlot7) ? sp.cb_ring4[ring4_Idx] : pkVal7;
    let _fma0: number;
    {
      const _a: number = 2.0, _b: number = rcma2, _c: number = rcma1;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      _fma0 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    let _fma1: number;
    {
      const _a: number = 3.0, _b: number = rcma3, _c: number = _fma0;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      _fma1 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = 4.0, _b: number = rcma4, _c: number = _fma1;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      kst = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    sigTotal += kst;
    sig = sigTotal / sp.optInSignalPeriod;
    pkSlot8 = sigRing_Idx;
    pkVal8 = kst;
    sigRing_Idx = sigRing_Idx + 1;
    if (sigRing_Idx > sp.maxIdx_sigRing) {
      sigRing_Idx = 0;
    }
    sigTotal -= (sigRing_Idx !== pkSlot8) ? sp.cb_sigRing[sigRing_Idx] : pkVal8;
    cur_outKST = kst;
    cur_outKSTSignal = sig;
    if (out === undefined) {
      out = new KstOut();
    }
    out.outKst = cur_outKST;
    out.outKstSignal = cur_outKSTSignal;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link KstOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   */
  value(out?: KstOut): KstOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('KST value', 'KstOut');
    }
    if (out === undefined) {
      out = new KstOut();
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
  clone(): KstStream {
    const c = new KstStream(this.core);
    c.optInROC1Period = this.optInROC1Period;
    c.optInROC2Period = this.optInROC2Period;
    c.optInROC3Period = this.optInROC3Period;
    c.optInROC4Period = this.optInROC4Period;
    c.optInSMA1Period = this.optInSMA1Period;
    c.optInSMA2Period = this.optInSMA2Period;
    c.optInSMA3Period = this.optInSMA3Period;
    c.optInSMA4Period = this.optInSMA4Period;
    c.optInSignalPeriod = this.optInSignalPeriod;
    c.total1 = this.total1;
    c.total2 = this.total2;
    c.total3 = this.total3;
    c.total4 = this.total4;
    c.sigTotal = this.sigTotal;
    c.ring1_Idx = this.ring1_Idx;
    c.ring2_Idx = this.ring2_Idx;
    c.ring3_Idx = this.ring3_Idx;
    c.ring4_Idx = this.ring4_Idx;
    c.sigRing_Idx = this.sigRing_Idx;
    c.maxIdx_ring1 = this.maxIdx_ring1;
    c.maxIdx_ring2 = this.maxIdx_ring2;
    c.maxIdx_ring3 = this.maxIdx_ring3;
    c.maxIdx_ring4 = this.maxIdx_ring4;
    c.maxIdx_sigRing = this.maxIdx_sigRing;
    c.ringPos_den1 = this.ringPos_den1;
    c.ringCap_den1 = this.ringCap_den1;
    c.ring_den1_inReal = this.ring_den1_inReal.slice();
    c.ringPos_den2 = this.ringPos_den2;
    c.ringCap_den2 = this.ringCap_den2;
    c.ring_den2_inReal = this.ring_den2_inReal.slice();
    c.ringPos_den3 = this.ringPos_den3;
    c.ringCap_den3 = this.ringCap_den3;
    c.ring_den3_inReal = this.ring_den3_inReal.slice();
    c.ringPos_den4 = this.ringPos_den4;
    c.ringCap_den4 = this.ringCap_den4;
    c.ring_den4_inReal = this.ring_den4_inReal.slice();
    c.cbSize_ring1 = this.cbSize_ring1;
    c.cb_ring1 = this.cb_ring1.slice();
    c.cbSize_ring2 = this.cbSize_ring2;
    c.cb_ring2 = this.cb_ring2.slice();
    c.cbSize_ring3 = this.cbSize_ring3;
    c.cb_ring3 = this.cb_ring3.slice();
    c.cbSize_ring4 = this.cbSize_ring4;
    c.cb_ring4 = this.cb_ring4.slice();
    c.cbSize_sigRing = this.cbSize_sigRing;
    c.cb_sigRing = this.cb_sigRing.slice();
    c.cur_outKST = this.cur_outKST;
    c.cur_outKSTSignal = this.cur_outKSTSignal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one KST bar, as `update`, `peek` and `value` return them. Passed
 * back to one of them, it is filled and returned again, with no allocation; it
 * holds its values until the next call that writes it. A new one holds `NaN`.
 */
export class KstOut {
  /** Know Sure Thing line. */
  outKst: number = NaN;
  /** Simple moving average of the line. */
  outKstSignal: number = NaN;
}

function kstStepImpl(sp: KstStream, inReal: number): void {
  let prior = 0;
  let roc = 0;
  let kst = 0;
  let sig = 0;
  let rcma1 = 0;
  let rcma2 = 0;
  let rcma3 = 0;
  let rcma4 = 0;
  if (sp.ringCap_den1 === 0) {
    sp.ring_den1_inReal[0] = inReal;
  }
  if (sp.ringCap_den2 === 0) {
    sp.ring_den2_inReal[0] = inReal;
  }
  if (sp.ringCap_den3 === 0) {
    sp.ring_den3_inReal[0] = inReal;
  }
  if (sp.ringCap_den4 === 0) {
    sp.ring_den4_inReal[0] = inReal;
  }
  prior = sp.ring_den1_inReal[sp.ringPos_den1];
  roc = (prior !== 0.0) ? (inReal / prior - 1.0) * 100.0 : 0.0;
  sp.total1 += roc;
  rcma1 = sp.total1 / sp.optInSMA1Period;
  sp.cb_ring1[sp.ring1_Idx] = roc;
  sp.ring1_Idx = sp.ring1_Idx + 1;
  if (sp.ring1_Idx > sp.maxIdx_ring1) {
    sp.ring1_Idx = 0;
  }
  sp.total1 -= sp.cb_ring1[sp.ring1_Idx];
  prior = sp.ring_den2_inReal[sp.ringPos_den2];
  roc = (prior !== 0.0) ? (inReal / prior - 1.0) * 100.0 : 0.0;
  sp.total2 += roc;
  rcma2 = sp.total2 / sp.optInSMA2Period;
  sp.cb_ring2[sp.ring2_Idx] = roc;
  sp.ring2_Idx = sp.ring2_Idx + 1;
  if (sp.ring2_Idx > sp.maxIdx_ring2) {
    sp.ring2_Idx = 0;
  }
  sp.total2 -= sp.cb_ring2[sp.ring2_Idx];
  prior = sp.ring_den3_inReal[sp.ringPos_den3];
  roc = (prior !== 0.0) ? (inReal / prior - 1.0) * 100.0 : 0.0;
  sp.total3 += roc;
  rcma3 = sp.total3 / sp.optInSMA3Period;
  sp.cb_ring3[sp.ring3_Idx] = roc;
  sp.ring3_Idx = sp.ring3_Idx + 1;
  if (sp.ring3_Idx > sp.maxIdx_ring3) {
    sp.ring3_Idx = 0;
  }
  sp.total3 -= sp.cb_ring3[sp.ring3_Idx];
  prior = sp.ring_den4_inReal[sp.ringPos_den4];
  roc = (prior !== 0.0) ? (inReal / prior - 1.0) * 100.0 : 0.0;
  sp.total4 += roc;
  rcma4 = sp.total4 / sp.optInSMA4Period;
  sp.cb_ring4[sp.ring4_Idx] = roc;
  sp.ring4_Idx = sp.ring4_Idx + 1;
  if (sp.ring4_Idx > sp.maxIdx_ring4) {
    sp.ring4_Idx = 0;
  }
  sp.total4 -= sp.cb_ring4[sp.ring4_Idx];
  let _fma3: number;
  {
    const _a: number = 2.0, _b: number = rcma2, _c: number = rcma1;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    _fma3 = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  let _fma4: number;
  {
    const _a: number = 3.0, _b: number = rcma3, _c: number = _fma3;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    _fma4 = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  {
    const _a: number = 4.0, _b: number = rcma4, _c: number = _fma4;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    kst = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  sp.sigTotal += kst;
  sig = sp.sigTotal / sp.optInSignalPeriod;
  sp.cb_sigRing[sp.sigRing_Idx] = kst;
  sp.sigRing_Idx = sp.sigRing_Idx + 1;
  if (sp.sigRing_Idx > sp.maxIdx_sigRing) {
    sp.sigRing_Idx = 0;
  }
  sp.sigTotal -= sp.cb_sigRing[sp.sigRing_Idx];
  sp.cur_outKST = kst;
  sp.cur_outKSTSignal = sig;
  sp.ring_den1_inReal[sp.ringPos_den1] = inReal;
  sp.ringPos_den1 = sp.ringPos_den1 + 1;
  if (sp.ringPos_den1 >= sp.ringCap_den1) {
    sp.ringPos_den1 = 0;
  }
  sp.ring_den2_inReal[sp.ringPos_den2] = inReal;
  sp.ringPos_den2 = sp.ringPos_den2 + 1;
  if (sp.ringPos_den2 >= sp.ringCap_den2) {
    sp.ringPos_den2 = 0;
  }
  sp.ring_den3_inReal[sp.ringPos_den3] = inReal;
  sp.ringPos_den3 = sp.ringPos_den3 + 1;
  if (sp.ringPos_den3 >= sp.ringCap_den3) {
    sp.ringPos_den3 = 0;
  }
  sp.ring_den4_inReal[sp.ringPos_den4] = inReal;
  sp.ringPos_den4 = sp.ringPos_den4 + 1;
  if (sp.ringPos_den4 >= sp.ringCap_den4) {
    sp.ringPos_den4 = 0;
  }
}

function kstOpenImpl(
  sp: KstStream,
  inReal: Float64Array,
  startIdx: number,
  optInROC1Period: number,
  optInROC2Period: number,
  optInROC3Period: number,
  optInROC4Period: number,
  optInSMA1Period: number,
  optInSMA2Period: number,
  optInSMA3Period: number,
  optInSMA4Period: number,
  optInSignalPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outKST: Float64Array,
  outKSTSignal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let outIdx = 0;
  let inIdx = 0;
  let lookbackTotal = 0;
  let sigStart = 0;
  let den1 = 0;
  let den2 = 0;
  let den3 = 0;
  let den4 = 0;
  let prior = 0;
  let roc = 0;
  let kst = 0;
  let sig = 0;
  let total1 = 0;
  let total2 = 0;
  let total3 = 0;
  let total4 = 0;
  let sigTotal = 0;
  let rcma1 = 0;
  let rcma2 = 0;
  let rcma3 = 0;
  let rcma4 = 0;
  let ring1!: Float64Array;
  let ring1_Idx = 0;
  let maxIdx_ring1 = (30) - 1;
  let ring2!: Float64Array;
  let ring2_Idx = 0;
  let maxIdx_ring2 = (30) - 1;
  let ring3!: Float64Array;
  let ring3_Idx = 0;
  let maxIdx_ring3 = (30) - 1;
  let ring4!: Float64Array;
  let ring4_Idx = 0;
  let maxIdx_ring4 = (30) - 1;
  let sigRing!: Float64Array;
  let sigRing_Idx = 0;
  let maxIdx_sigRing = (30) - 1;
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
  if (optInSMA1Period === Core.INTEGER_DEFAULT) {
    optInSMA1Period = 10;
  } else if (optInSMA1Period < 1 || optInSMA1Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSMA2Period === Core.INTEGER_DEFAULT) {
    optInSMA2Period = 10;
  } else if (optInSMA2Period < 1 || optInSMA2Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSMA3Period === Core.INTEGER_DEFAULT) {
    optInSMA3Period = 10;
  } else if (optInSMA3Period < 1 || optInSMA3Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSMA4Period === Core.INTEGER_DEFAULT) {
    optInSMA4Period = 15;
  } else if (optInSMA4Period < 1 || optInSMA4Period > 100000) {
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
  /* Bit-exact with TA_ROC per leg into TA_SMA, each SMA called at the start
   * its consumer needs: keep every running sum's add/subtract order and the
   * left-to-right weighted sum, or the composite differential stops being
   * exact. Each ring holds its stage's last SMA-period values, so the only
   * inputs read are the current bar and each leg's lagged denominator,
   * both at or after the slot the outputs write: outKST or outKSTSignal
   * may alias inReal.
   */
  lookbackTotal = kstLookbackPositional(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (optInSMA1Period < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  ring1 = new Float64Array(optInSMA1Period);
  maxIdx_ring1 = optInSMA1Period - 1;
  ring1_Idx = 0;
  if (optInSMA2Period < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  ring2 = new Float64Array(optInSMA2Period);
  maxIdx_ring2 = optInSMA2Period - 1;
  ring2_Idx = 0;
  if (optInSMA3Period < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  ring3 = new Float64Array(optInSMA3Period);
  maxIdx_ring3 = optInSMA3Period - 1;
  ring3_Idx = 0;
  if (optInSMA4Period < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  ring4 = new Float64Array(optInSMA4Period);
  maxIdx_ring4 = optInSMA4Period - 1;
  ring4_Idx = 0;
  if (optInSignalPeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sigRing = new Float64Array(optInSignalPeriod);
  maxIdx_sigRing = optInSignalPeriod - 1;
  sigRing_Idx = 0;
  sigStart = startIdx - (optInSignalPeriod - 1);
  sigTotal = 0.0;
  total4 = sigTotal;
  total3 = total4;
  total2 = total3;
  total1 = total2;
  inIdx = sigStart - (optInSMA1Period - 1);
  den1 = inIdx - optInROC1Period;
  while (inIdx < sigStart) {
    prior = inReal[den1];
    den1 += 1;
    roc = (prior !== 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
    total1 += roc;
    ring1[ring1_Idx] = roc;
    ring1_Idx++;
    if (ring1_Idx > maxIdx_ring1) {
      ring1_Idx = 0;
    }
    inIdx += 1;
  }
  inIdx = sigStart - (optInSMA2Period - 1);
  den2 = inIdx - optInROC2Period;
  while (inIdx < sigStart) {
    prior = inReal[den2];
    den2 += 1;
    roc = (prior !== 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
    total2 += roc;
    ring2[ring2_Idx] = roc;
    ring2_Idx++;
    if (ring2_Idx > maxIdx_ring2) {
      ring2_Idx = 0;
    }
    inIdx += 1;
  }
  inIdx = sigStart - (optInSMA3Period - 1);
  den3 = inIdx - optInROC3Period;
  while (inIdx < sigStart) {
    prior = inReal[den3];
    den3 += 1;
    roc = (prior !== 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
    total3 += roc;
    ring3[ring3_Idx] = roc;
    ring3_Idx++;
    if (ring3_Idx > maxIdx_ring3) {
      ring3_Idx = 0;
    }
    inIdx += 1;
  }
  inIdx = sigStart - (optInSMA4Period - 1);
  den4 = inIdx - optInROC4Period;
  while (inIdx < sigStart) {
    prior = inReal[den4];
    den4 += 1;
    roc = (prior !== 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
    total4 += roc;
    ring4[ring4_Idx] = roc;
    ring4_Idx++;
    if (ring4_Idx > maxIdx_ring4) {
      ring4_Idx = 0;
    }
    inIdx += 1;
  }
  inIdx = sigStart;
  while (inIdx < startIdx) {
    prior = inReal[den1];
    den1 += 1;
    roc = (prior !== 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
    total1 += roc;
    rcma1 = total1 / optInSMA1Period;
    ring1[ring1_Idx] = roc;
    ring1_Idx++;
    if (ring1_Idx > maxIdx_ring1) {
      ring1_Idx = 0;
    }
    total1 -= ring1[ring1_Idx];
    prior = inReal[den2];
    den2 += 1;
    roc = (prior !== 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
    total2 += roc;
    rcma2 = total2 / optInSMA2Period;
    ring2[ring2_Idx] = roc;
    ring2_Idx++;
    if (ring2_Idx > maxIdx_ring2) {
      ring2_Idx = 0;
    }
    total2 -= ring2[ring2_Idx];
    prior = inReal[den3];
    den3 += 1;
    roc = (prior !== 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
    total3 += roc;
    rcma3 = total3 / optInSMA3Period;
    ring3[ring3_Idx] = roc;
    ring3_Idx++;
    if (ring3_Idx > maxIdx_ring3) {
      ring3_Idx = 0;
    }
    total3 -= ring3[ring3_Idx];
    prior = inReal[den4];
    den4 += 1;
    roc = (prior !== 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
    total4 += roc;
    rcma4 = total4 / optInSMA4Period;
    ring4[ring4_Idx] = roc;
    ring4_Idx++;
    if (ring4_Idx > maxIdx_ring4) {
      ring4_Idx = 0;
    }
    total4 -= ring4[ring4_Idx];
    let _fma6: number;
    {
      const _a: number = 2.0, _b: number = rcma2, _c: number = rcma1;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      _fma6 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    let _fma7: number;
    {
      const _a: number = 3.0, _b: number = rcma3, _c: number = _fma6;
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
      const _a: number = 4.0, _b: number = rcma4, _c: number = _fma7;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      kst = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    sigTotal += kst;
    sigRing[sigRing_Idx] = kst;
    sigRing_Idx++;
    if (sigRing_Idx > maxIdx_sigRing) {
      sigRing_Idx = 0;
    }
    inIdx += 1;
  }
  outIdx = 0;
  while (inIdx <= endIdx) {
    prior = inReal[den1];
    den1 += 1;
    roc = (prior !== 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
    total1 += roc;
    rcma1 = total1 / optInSMA1Period;
    ring1[ring1_Idx] = roc;
    ring1_Idx++;
    if (ring1_Idx > maxIdx_ring1) {
      ring1_Idx = 0;
    }
    total1 -= ring1[ring1_Idx];
    prior = inReal[den2];
    den2 += 1;
    roc = (prior !== 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
    total2 += roc;
    rcma2 = total2 / optInSMA2Period;
    ring2[ring2_Idx] = roc;
    ring2_Idx++;
    if (ring2_Idx > maxIdx_ring2) {
      ring2_Idx = 0;
    }
    total2 -= ring2[ring2_Idx];
    prior = inReal[den3];
    den3 += 1;
    roc = (prior !== 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
    total3 += roc;
    rcma3 = total3 / optInSMA3Period;
    ring3[ring3_Idx] = roc;
    ring3_Idx++;
    if (ring3_Idx > maxIdx_ring3) {
      ring3_Idx = 0;
    }
    total3 -= ring3[ring3_Idx];
    prior = inReal[den4];
    den4 += 1;
    roc = (prior !== 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
    total4 += roc;
    rcma4 = total4 / optInSMA4Period;
    ring4[ring4_Idx] = roc;
    ring4_Idx++;
    if (ring4_Idx > maxIdx_ring4) {
      ring4_Idx = 0;
    }
    total4 -= ring4[ring4_Idx];
    let _fma9: number;
    {
      const _a: number = 2.0, _b: number = rcma2, _c: number = rcma1;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      _fma9 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    let _fma10: number;
    {
      const _a: number = 3.0, _b: number = rcma3, _c: number = _fma9;
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
      const _a: number = 4.0, _b: number = rcma4, _c: number = _fma10;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      kst = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    sigTotal += kst;
    sig = sigTotal / optInSignalPeriod;
    sigRing[sigRing_Idx] = kst;
    sigRing_Idx++;
    if (sigRing_Idx > maxIdx_sigRing) {
      sigRing_Idx = 0;
    }
    sigTotal -= sigRing[sigRing_Idx];
    outKST[((outIdx * outStride) | 0)] = kst;
    outKSTSignal[((outIdx * outStride) | 0)] = sig;
    outIdx += 1;
    inIdx += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  const cap_den1 = inIdx - den1;
  if (cap_den1 < 0 || cap_den1 > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_den1 = cap_den1 > 0 ? cap_den1 : 1;
  const capRing_den1_inReal = new Float64Array(allocN_den1);
  capRing_den1_inReal.set(inReal.subarray(historyLen - cap_den1, historyLen), 0);
  const cap_den2 = inIdx - den2;
  if (cap_den2 < 0 || cap_den2 > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_den2 = cap_den2 > 0 ? cap_den2 : 1;
  const capRing_den2_inReal = new Float64Array(allocN_den2);
  capRing_den2_inReal.set(inReal.subarray(historyLen - cap_den2, historyLen), 0);
  const cap_den3 = inIdx - den3;
  if (cap_den3 < 0 || cap_den3 > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_den3 = cap_den3 > 0 ? cap_den3 : 1;
  const capRing_den3_inReal = new Float64Array(allocN_den3);
  capRing_den3_inReal.set(inReal.subarray(historyLen - cap_den3, historyLen), 0);
  const cap_den4 = inIdx - den4;
  if (cap_den4 < 0 || cap_den4 > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_den4 = cap_den4 > 0 ? cap_den4 : 1;
  const capRing_den4_inReal = new Float64Array(allocN_den4);
  capRing_den4_inReal.set(inReal.subarray(historyLen - cap_den4, historyLen), 0);
  const capCb_ring1 = maxIdx_ring1 + 1;
  if (capCb_ring1 > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  const capCb_ring2 = maxIdx_ring2 + 1;
  if (capCb_ring2 > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  const capCb_ring3 = maxIdx_ring3 + 1;
  if (capCb_ring3 > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  const capCb_ring4 = maxIdx_ring4 + 1;
  if (capCb_ring4 > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  const capCb_sigRing = maxIdx_sigRing + 1;
  if (capCb_sigRing > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sp.optInROC1Period = optInROC1Period;
  sp.optInROC2Period = optInROC2Period;
  sp.optInROC3Period = optInROC3Period;
  sp.optInROC4Period = optInROC4Period;
  sp.optInSMA1Period = optInSMA1Period;
  sp.optInSMA2Period = optInSMA2Period;
  sp.optInSMA3Period = optInSMA3Period;
  sp.optInSMA4Period = optInSMA4Period;
  sp.optInSignalPeriod = optInSignalPeriod;
  sp.total1 = total1;
  sp.total2 = total2;
  sp.total3 = total3;
  sp.total4 = total4;
  sp.sigTotal = sigTotal;
  sp.ring1_Idx = ring1_Idx;
  sp.ring2_Idx = ring2_Idx;
  sp.ring3_Idx = ring3_Idx;
  sp.ring4_Idx = ring4_Idx;
  sp.sigRing_Idx = sigRing_Idx;
  sp.maxIdx_ring1 = maxIdx_ring1;
  sp.maxIdx_ring2 = maxIdx_ring2;
  sp.maxIdx_ring3 = maxIdx_ring3;
  sp.maxIdx_ring4 = maxIdx_ring4;
  sp.maxIdx_sigRing = maxIdx_sigRing;
  sp.ringPos_den1 = 0;
  sp.ringCap_den1 = cap_den1;
  sp.ring_den1_inReal = capRing_den1_inReal;
  sp.ringPos_den2 = 0;
  sp.ringCap_den2 = cap_den2;
  sp.ring_den2_inReal = capRing_den2_inReal;
  sp.ringPos_den3 = 0;
  sp.ringCap_den3 = cap_den3;
  sp.ring_den3_inReal = capRing_den3_inReal;
  sp.ringPos_den4 = 0;
  sp.ringCap_den4 = cap_den4;
  sp.ring_den4_inReal = capRing_den4_inReal;
  sp.cbSize_ring1 = capCb_ring1;
  sp.cb_ring1 = ring1;
  sp.cbSize_ring2 = capCb_ring2;
  sp.cb_ring2 = ring2;
  sp.cbSize_ring3 = capCb_ring3;
  sp.cb_ring3 = ring3;
  sp.cbSize_ring4 = capCb_ring4;
  sp.cb_ring4 = ring4;
  sp.cbSize_sigRing = capCb_sigRing;
  sp.cb_sigRing = sigRing;
  sp.cur_outKST = outKST[(outNBElement.value - 1) * outStride];
  sp.cur_outKSTSignal = outKSTSignal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function kstOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInROC1Period: number,
  optInROC2Period: number,
  optInROC3Period: number,
  optInROC4Period: number,
  optInSMA1Period: number,
  optInSMA2Period: number,
  optInSMA3Period: number,
  optInSMA4Period: number,
  optInSignalPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outKST: Float64Array,
  outKSTSignal: Float64Array,
): KstStream {
  const sp = KstStream.of(core);
  const retCode = kstOpenImpl(sp, inReal, startIdx, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod, outBegIdx, outNBElement, outKST, outKSTSignal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('KST openAndFill', inReal.length, startIdx, kstLookbackPositional(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && kstLookbackPositional(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod, core) < 0) {
    throw rejectedParams('KST openAndFill', PARAM_RANGES, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod);
  }
  throw streamFailure('KST openAndFill', retCode);
}

/** @internal */
export function kstOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInROC1Period: number,
  optInROC2Period: number,
  optInROC3Period: number,
  optInROC4Period: number,
  optInSMA1Period: number,
  optInSMA2Period: number,
  optInSMA3Period: number,
  optInSMA4Period: number,
  optInSignalPeriod: number,
): KstStream {
  const sp = KstStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outKST = new Float64Array(1);
  const sink_outKSTSignal = new Float64Array(1);
  const retCode = kstOpenImpl(sp, inReal, startIdx, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod, outBegIdx, outNBElement, sink_outKST, sink_outKSTSignal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('KST open', inReal.length, startIdx, kstLookbackPositional(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && kstLookbackPositional(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod, core) < 0) {
    throw rejectedParams('KST open', PARAM_RANGES, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod);
  }
  throw streamFailure('KST open', retCode);
}

/** @internal */
export function kstOpenPositional(
  inReal: ArrayLike<number>,
  optInROC1Period: number,
  optInROC2Period: number,
  optInROC3Period: number,
  optInROC4Period: number,
  optInSMA1Period: number,
  optInSMA2Period: number,
  optInSMA3Period: number,
  optInSMA4Period: number,
  optInSignalPeriod: number,
  core: Core = Core.DEFAULT,
): KstStream {
  optInROC1Period = requireInt32('KST open', 'roc1Period', optInROC1Period);
  optInROC2Period = requireInt32('KST open', 'roc2Period', optInROC2Period);
  optInROC3Period = requireInt32('KST open', 'roc3Period', optInROC3Period);
  optInROC4Period = requireInt32('KST open', 'roc4Period', optInROC4Period);
  optInSMA1Period = requireInt32('KST open', 'sma1Period', optInSMA1Period);
  optInSMA2Period = requireInt32('KST open', 'sma2Period', optInSMA2Period);
  optInSMA3Period = requireInt32('KST open', 'sma3Period', optInSMA3Period);
  optInSMA4Period = requireInt32('KST open', 'sma4Period', optInSMA4Period);
  optInSignalPeriod = requireInt32('KST open', 'signalPeriod', optInSignalPeriod);
  requireArgument('KST open', 'real', inReal);
  requireHistory('KST open', inReal.length);
  return kstOpenInternal(core, float64(inReal), 0, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod);
}

/** @internal */
export function kstOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInROC1Period: number,
  optInROC2Period: number,
  optInROC3Period: number,
  optInROC4Period: number,
  optInSMA1Period: number,
  optInSMA2Period: number,
  optInSMA3Period: number,
  optInSMA4Period: number,
  optInSignalPeriod: number,
  outKST: Float64Array,
  outKSTSignal: Float64Array,
  core: Core = Core.DEFAULT,
): KstStream {
  optInROC1Period = requireInt32('KST openAndFill', 'roc1Period', optInROC1Period);
  optInROC2Period = requireInt32('KST openAndFill', 'roc2Period', optInROC2Period);
  optInROC3Period = requireInt32('KST openAndFill', 'roc3Period', optInROC3Period);
  optInROC4Period = requireInt32('KST openAndFill', 'roc4Period', optInROC4Period);
  optInSMA1Period = requireInt32('KST openAndFill', 'sma1Period', optInSMA1Period);
  optInSMA2Period = requireInt32('KST openAndFill', 'sma2Period', optInSMA2Period);
  optInSMA3Period = requireInt32('KST openAndFill', 'sma3Period', optInSMA3Period);
  optInSMA4Period = requireInt32('KST openAndFill', 'sma4Period', optInSMA4Period);
  optInSignalPeriod = requireInt32('KST openAndFill', 'signalPeriod', optInSignalPeriod);
  requireArgument('KST openAndFill', 'real', inReal);
  requireHistory('KST openAndFill', inReal.length);
  const guardLookback = kstLookbackPositional(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('KST openAndFill', PARAM_RANGES, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod);
  }
  const guardOutLen = openFillCount('KST openAndFill', inReal.length, guardLookback);
  requireOutput('KST openAndFill', 'outKst', outKST, guardOutLen, Float64Array);
  requireOutput('KST openAndFill', 'outKstSignal', outKSTSignal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outKST.buffer === inRealF64.buffer && outKST.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('KST openAndFill', 'outKst', 'real');
  }
  if (outKSTSignal.buffer === inRealF64.buffer && outKSTSignal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('KST openAndFill', 'outKstSignal', 'real');
  }
  if (outKST.buffer === outKSTSignal.buffer && outKST.byteOffset === outKSTSignal.byteOffset) {
    throw sharedMemory('KST openAndFill', 'outKst', 'outKstSignal');
  }
  if (guardOutLen > 0 && overlaps(outKST, inRealF64)) {
    throw sharedMemory('KST openAndFill', 'outKst', 'real');
  }
  if (guardOutLen > 0 && overlaps(outKSTSignal, inRealF64)) {
    throw sharedMemory('KST openAndFill', 'outKstSignal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outKST, outKSTSignal)) {
    throw sharedMemory('KST openAndFill', 'outKst', 'outKstSignal');
  }
  return kstOpenAndFillInternal(core, inRealF64, 0, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod, new MInteger(), new MInteger(), outKST, outKSTSignal);
}

/**
 * Opens a live KST stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link kst} there.
 *
 * @param real - Warm-up history. Source price series (canonically the close)
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link KstOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `kstLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { kstOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = kstOpen(data, {
 *   roc1Period: 10,
 *   roc2Period: 15,
 *   roc3Period: 20,
 *   roc4Period: 30,
 *   sma1Period: 10,
 *   sma2Period: 10,
 *   sma3Period: 10,
 *   sma4Period: 15,
 *   signalPeriod: 9,
 * });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function kstOpen(real: ArrayLike<number>, options?: KstOptions): KstStream {
  const o = optionsOf('KST open', options);
  const optInROC1Period = orDefault(o.roc1Period, Core.INTEGER_DEFAULT);
  const optInROC2Period = orDefault(o.roc2Period, Core.INTEGER_DEFAULT);
  const optInROC3Period = orDefault(o.roc3Period, Core.INTEGER_DEFAULT);
  const optInROC4Period = orDefault(o.roc4Period, Core.INTEGER_DEFAULT);
  const optInSMA1Period = orDefault(o.sma1Period, Core.INTEGER_DEFAULT);
  const optInSMA2Period = orDefault(o.sma2Period, Core.INTEGER_DEFAULT);
  const optInSMA3Period = orDefault(o.sma3Period, Core.INTEGER_DEFAULT);
  const optInSMA4Period = orDefault(o.sma4Period, Core.INTEGER_DEFAULT);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('KST open', o.core);
  return kstOpenPositional(real, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod, core);
}

/**
 * {@link kstOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link kst}.
 *
 * @param real - Warm-up history. Source price series (canonically the close)
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link KstOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link kst} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `kstLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or a
 *   buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { kstOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const {
 *   stream,
 *   begIdx,
 *   outKst,
 *   outKstSignal,
 * } = kstOpenAndFill(data, {
 *   roc1Period: 10,
 *   roc2Period: 15,
 *   roc3Period: 20,
 *   roc4Period: 30,
 *   sma1Period: 10,
 *   sma2Period: 10,
 *   sma3Period: 10,
 *   sma4Period: 15,
 *   signalPeriod: 9,
 * });
 * // outKst[k] is the value at bar begIdx + k
 * ```
 */
export function kstOpenAndFill(real: ArrayLike<number>, options?: KstOptions): KstResult & { stream: KstStream } {
  const o = optionsOf('KST openAndFill', options);
  const optInROC1Period = orDefault(o.roc1Period, Core.INTEGER_DEFAULT);
  const optInROC2Period = orDefault(o.roc2Period, Core.INTEGER_DEFAULT);
  const optInROC3Period = orDefault(o.roc3Period, Core.INTEGER_DEFAULT);
  const optInROC4Period = orDefault(o.roc4Period, Core.INTEGER_DEFAULT);
  const optInSMA1Period = orDefault(o.sma1Period, Core.INTEGER_DEFAULT);
  const optInSMA2Period = orDefault(o.sma2Period, Core.INTEGER_DEFAULT);
  const optInSMA3Period = orDefault(o.sma3Period, Core.INTEGER_DEFAULT);
  const optInSMA4Period = orDefault(o.sma4Period, Core.INTEGER_DEFAULT);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('KST openAndFill', o.core);
  const capacity = fillCapacity(kstLookbackPositional(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod, core), real);
  const outKST = o.outKst === undefined ? new Float64Array(capacity) : o.outKst;
  const outKSTSignal = o.outKstSignal === undefined ? new Float64Array(capacity) : o.outKstSignal;
  const stream = kstOpenAndFillPositional(real, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod, outKST, outKSTSignal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outKst: outKST.subarray(0, range.count), outKstSignal: outKSTSignal.subarray(0, range.count) };
}
