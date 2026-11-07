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
 *  082026 MF,CC  Initial version (#238).
 *  082326 MF,CC  Fix #253. Test the smoothed range exactly instead of against
 *                the fixed TA_IS_ZERO band, which zeroed the oscillator for
 *                any instrument quoted small enough to fall under it.
 */

import { badSink, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { emaLookbackPositional } from './ema.ts';

/** @internal */
export function smiLookbackPositional(optInTimePeriod: number, optInFastPeriod: number, optInSlowPeriod: number, optInSignalPeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if ((optInFastPeriod | 0) !== optInFastPeriod) {
    return -1;
  }
  if ((optInSlowPeriod | 0) !== optInSlowPeriod) {
    return -1;
  }
  if ((optInSignalPeriod | 0) !== optInSignalPeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 13;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  if (optInFastPeriod === Core.INTEGER_DEFAULT) {
    optInFastPeriod = 2;
  } else if (optInFastPeriod < 2 || optInFastPeriod > 100000) {
    return -1;
  }
  if (optInSlowPeriod === Core.INTEGER_DEFAULT) {
    optInSlowPeriod = 25;
  } else if (optInSlowPeriod < 2 || optInSlowPeriod > 100000) {
    return -1;
  }
  if (optInSignalPeriod === Core.INTEGER_DEFAULT) {
    optInSignalPeriod = 9;
  } else if (optInSignalPeriod < 2 || optInSignalPeriod > 100000) {
    return -1;
  }
  /* One high/low window, then the three EMA warm-ups the pipeline stacks on
   * top of it: slow smooths the raw momentum, fast smooths that, and signal
   * smooths the finished SMI line. Every term is exactly the lookback of the
   * function it comes from, so none of them is restated here -- which is also
   * what makes SMI inherit TA_FUNC_UNST_EMA from its callee.
   */
  return optInTimePeriod - 1 + emaLookbackPositional(optInSlowPeriod, core) + emaLookbackPositional(optInFastPeriod, core) + emaLookbackPositional(optInSignalPeriod, core);
}

/** @internal */
export function smiDisplayShiftPositional(optInTimePeriod: number, optInFastPeriod: number, optInSlowPeriod: number, optInSignalPeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (smiLookbackPositional(optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 2) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function smiImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  optInTimePeriod: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outSMI: Float64Array,
  outSMISignal: Float64Array,
): RetCode {
  let kSlow = 0;
  let kFast = 0;
  let kSignal = 0;
  let betaSlow = 0;
  let betaFast = 0;
  let betaSignal = 0;
  let highest = 0;
  let lowest = 0;
  let tmp = 0;
  let emaSlowNum = 0;
  let emaFastNum = 0;
  let emaSlowDen = 0;
  let emaFastDen = 0;
  let sumSlowNum = 0;
  let sumSlowDen = 0;
  let sumFastNum = 0;
  let sumFastDen = 0;
  let sumSignal = 0;
  let num = 0;
  let den = 0;
  let halfDen = 0;
  let smiValue = 0;
  let prevSignal = 0;
  let lookbackTotal = 0;
  let lookbackSlow = 0;
  let lookbackFast = 0;
  let today = 0;
  let trailingIdx = 0;
  let highestIdx = 0;
  let lowestIdx = 0;
  let i = 0;
  let outIdx = 0;
  let nBar = 0;
  let nFast = 0;
  let nSignal = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 13;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInFastPeriod === Core.INTEGER_DEFAULT) {
    optInFastPeriod = 2;
  } else if (optInFastPeriod < 2 || optInFastPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowPeriod === Core.INTEGER_DEFAULT) {
    optInSlowPeriod = 25;
  } else if (optInSlowPeriod < 2 || optInSlowPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSignalPeriod === Core.INTEGER_DEFAULT) {
    optInSignalPeriod = 9;
  } else if (optInSignalPeriod < 2 || optInSignalPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if ((outSMI.buffer === outSMISignal.buffer && outSMI.byteOffset === outSMISignal.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  /* Declared Num, Num, Den, Den: the stream state keeps this order, and with
   * each stage's pair adjacent gcc 13 packs the steps two by two and then
   * reads 16 bytes across two of its own stores, which cannot be forwarded.
   */
  lookbackTotal = smiLookbackPositional(optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  outBegIdx.value = startIdx;
  /* Blau's pipeline in one pass. A composed form is not available: the
   * streaming producer model carries exactly one intermediate series
   * (streaming.rs:2076) and SMI has two, the numerator and the denominator,
   * which must be smoothed separately before they are divided.
   *
   * Each of the three stages seeds the way ema.c does -- a simple average of
   * that stage's first 'period' inputs -- so the result is bit-identical to
   * TA_MAX + TA_MIN followed by four TA_EMA calls. The stage boundaries below
   * are the callee LOOKBACKS, not (period-1), so that a warm
   * TA_SetUnstablePeriod(TA_FUNC_UNST_EMA) folds in: each stage then seeds
   * from the values its predecessor would have published, exactly as the
   * composed form does. The seed sums accumulate from 0.0 in production
   * order; do not reorder or fuse them (0.0+x is not x for x=-0.0).
   */
  betaSlow = (optInSlowPeriod - 1) / (optInSlowPeriod + 1);
  kSlow = 1.0 - betaSlow;
  if (betaSlow < 0.5) {
    betaSlow = 1.0 - kSlow;
  }
  betaFast = (optInFastPeriod - 1) / (optInFastPeriod + 1);
  kFast = 1.0 - betaFast;
  if (betaFast < 0.5) {
    betaFast = 1.0 - kFast;
  }
  betaSignal = (optInSignalPeriod - 1) / (optInSignalPeriod + 1);
  kSignal = 1.0 - betaSignal;
  if (betaSignal < 0.5) {
    betaSignal = 1.0 - kSignal;
  }
  lookbackSlow = emaLookbackPositional(optInSlowPeriod, core);
  lookbackFast = emaLookbackPositional(optInFastPeriod, core);
  emaSlowNum = 0.0;
  emaSlowDen = 0.0;
  emaFastNum = 0.0;
  emaFastDen = 0.0;
  prevSignal = 0.0;
  smiValue = 0.0;
  sumSlowNum = 0.0;
  sumSlowDen = 0.0;
  sumFastNum = 0.0;
  sumFastDen = 0.0;
  sumSignal = 0.0;
  highest = 0.0;
  lowest = 0.0;
  highestIdx = -1;
  lowestIdx = -1;
  /* The first bar carrying a full high/low window. */
  trailingIdx = startIdx - lookbackTotal;
  today = trailingIdx + (optInTimePeriod - 1);
  nBar = 0;
  /* Warm-up. Runs through startIdx inclusive: the last pass here is the one
   * that completes the signal seed, so it produces the first output pair.
   */
  while (today <= startIdx) {
    /* Set the lowest low */
    tmp = inLow[today];
    if (lowestIdx < trailingIdx) {
      lowestIdx = trailingIdx;
      lowest = inLow[lowestIdx];
      i = lowestIdx;
      while (++i <= today) {
        tmp = inLow[i];
        if (tmp < lowest) {
          lowestIdx = i;
          lowest = tmp;
        }
      }
    } else if (tmp <= lowest) {
      lowestIdx = today;
      lowest = tmp;
    }
    /* Set the highest high */
    tmp = inHigh[today];
    if (highestIdx < trailingIdx) {
      highestIdx = trailingIdx;
      highest = inHigh[highestIdx];
      i = highestIdx;
      while (++i <= today) {
        tmp = inHigh[i];
        if (tmp > highest) {
          highestIdx = i;
          highest = tmp;
        }
      }
    } else if (tmp >= highest) {
      highestIdx = today;
      highest = tmp;
    }
    den = highest - lowest;
    num = inClose[today] - (highest + lowest) * 0.5;
    /* Stage 1: the slow EMA, over the raw momentum. */
    if (nBar < optInSlowPeriod) {
      sumSlowNum = sumSlowNum + num;
      sumSlowDen = sumSlowDen + den;
      if (nBar === optInSlowPeriod - 1) {
        emaSlowNum = sumSlowNum / optInSlowPeriod;
        emaSlowDen = sumSlowDen / optInSlowPeriod;
      }
    } else {
      {
        const _a: number = betaSlow, _b: number = emaSlowNum, _c: number = kSlow * num;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        emaSlowNum = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      {
        const _a: number = betaSlow, _b: number = emaSlowDen, _c: number = kSlow * den;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        emaSlowDen = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
    }
    /* Stage 2: the fast EMA, over what stage 1 publishes.
     *
     * The stage counters are compared BEFORE they are subtracted, never
     * after. Writing this as `nFast = nBar - lookbackSlow; if( nFast >= 0 )`
     * is correct in C, where the counters are signed, and broken everywhere
     * else: the Rust backend renders them as `usize`, so the subtraction
     * underflows for the first lookbackSlow bars -- a panic in a debug build
     * and a wrap in release. It would also be invisible to the cross-language
     * gate, which runs release servers at unstable period 0, where the branch
     * the wrap wrongly takes happens to be a no-op because both accumulators
     * are still 0.0.
     */
    if (nBar >= lookbackSlow) {
      nFast = nBar - lookbackSlow;
      if (nFast < optInFastPeriod) {
        sumFastNum = sumFastNum + emaSlowNum;
        sumFastDen = sumFastDen + emaSlowDen;
        if (nFast === optInFastPeriod - 1) {
          emaFastNum = sumFastNum / optInFastPeriod;
          emaFastDen = sumFastDen / optInFastPeriod;
        }
      } else {
        {
          const _a: number = betaFast, _b: number = emaFastNum, _c: number = kFast * emaSlowNum;
          const _p = _a * _b;
          let _t = 134217729 * _a;
          const _ah = _t - (_t - _a), _al = _a - _ah;
          _t = 134217729 * _b;
          const _bh = _t - (_t - _b), _bl = _b - _bh;
          const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
          const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
          const _v = _tl + _pl, _v2 = _v + _v;
          const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
          emaFastNum = _w && (_pl === 0 || _v === 0) ? _th
            : _w && _v2 !== (_th + _v2) - _th ? _th + _v
            : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
        }
        {
          const _a: number = betaFast, _b: number = emaFastDen, _c: number = kFast * emaSlowDen;
          const _p = _a * _b;
          let _t = 134217729 * _a;
          const _ah = _t - (_t - _a), _al = _a - _ah;
          _t = 134217729 * _b;
          const _bh = _t - (_t - _b), _bl = _b - _bh;
          const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
          const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
          const _v = _tl + _pl, _v2 = _v + _v;
          const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
          emaFastDen = _w && (_pl === 0 || _v === 0) ? _th
            : _w && _v2 !== (_th + _v2) - _th ? _th + _v
            : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
        }
      }
    }
    /* Stage 3: the SMI line, then the signal EMA over it. */
    if (nBar >= lookbackSlow + lookbackFast) {
      nSignal = nBar - lookbackSlow - lookbackFast;
      halfDen = 0.5 * emaFastDen;
      if (halfDen > 0.0) {
        smiValue = 100.0 * emaFastNum / halfDen;
      } else {
        smiValue = 0.0;
      }
      if (nSignal < optInSignalPeriod) {
        sumSignal = sumSignal + smiValue;
        if (nSignal === optInSignalPeriod - 1) {
          prevSignal = sumSignal / optInSignalPeriod;
        }
      } else {
        {
          const _a: number = betaSignal, _b: number = prevSignal, _c: number = kSignal * smiValue;
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
    nBar = nBar + 1;
    trailingIdx = trailingIdx + 1;
    today = today + 1;
  }
  outSMI[0] = smiValue;
  outSMISignal[0] = prevSignal;
  outIdx = 1;
  /* Stable zone. Every stage is a pure recursion from here on. */
  while (today <= endIdx) {
    /* Set the lowest low */
    tmp = inLow[today];
    if (lowestIdx < trailingIdx) {
      lowestIdx = trailingIdx;
      lowest = inLow[lowestIdx];
      i = lowestIdx;
      while (++i <= today) {
        tmp = inLow[i];
        if (tmp < lowest) {
          lowestIdx = i;
          lowest = tmp;
        }
      }
    } else if (tmp <= lowest) {
      lowestIdx = today;
      lowest = tmp;
    }
    /* Set the highest high */
    tmp = inHigh[today];
    if (highestIdx < trailingIdx) {
      highestIdx = trailingIdx;
      highest = inHigh[highestIdx];
      i = highestIdx;
      while (++i <= today) {
        tmp = inHigh[i];
        if (tmp > highest) {
          highestIdx = i;
          highest = tmp;
        }
      }
    } else if (tmp >= highest) {
      highestIdx = today;
      highest = tmp;
    }
    den = highest - lowest;
    num = inClose[today] - (highest + lowest) * 0.5;
    {
      const _a: number = betaSlow, _b: number = emaSlowNum, _c: number = kSlow * num;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaSlowNum = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = betaSlow, _b: number = emaSlowDen, _c: number = kSlow * den;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaSlowDen = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = betaFast, _b: number = emaFastNum, _c: number = kFast * emaSlowNum;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaFastNum = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = betaFast, _b: number = emaFastDen, _c: number = kFast * emaSlowDen;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaFastDen = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    /* The denominator is an EMA of an EMA of the high-low range: every term
     * is non-negative and every weight is positive, so it carries no
     * cancellation residue and is zero only when every range that reached it
     * was exactly zero -- 0/0, since a window of H == L bars makes num zero
     * too, reported as the neutral 0.0 by the CCI (#7) and IMI (#112)
     * convention. Test it exactly: the range carries the quote unit, so the
     * fixed TA_IS_ZERO band this used to be zeroed the oscillator for any
     * instrument quoted below it (issue #253). Issue #107's machine-flat
     * window is caught by the exact test as well, since the residue an
     * EMA leaves there is zero, not sub-epsilon.
     */
    halfDen = 0.5 * emaFastDen;
    if (halfDen > 0.0) {
      smiValue = 100.0 * emaFastNum / halfDen;
    } else {
      smiValue = 0.0;
    }
    {
      const _a: number = betaSignal, _b: number = prevSignal, _c: number = kSignal * smiValue;
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
    outSMI[outIdx] = smiValue;
    outSMISignal[outIdx] = prevSignal;
    outIdx = outIdx + 1;
    trailingIdx = trailingIdx + 1;
    today = today + 1;
  }
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['fastPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['slowPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['signalPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000']] as const;

/** @internal */
export function smiPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  outSMI: Float64Array,
  outSMISignal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('SMI', 'startIdx', startIdx);
  endIdx = requireInt32('SMI', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('SMI', 'timePeriod', optInTimePeriod);
  optInFastPeriod = requireInt32('SMI', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('SMI', 'slowPeriod', optInSlowPeriod);
  optInSignalPeriod = requireInt32('SMI', 'signalPeriod', optInSignalPeriod);
  requireIndexRange('SMI', startIdx, endIdx);
  const guardLookback = smiLookbackPositional(optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('SMI', PARAM_RANGES, optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
  }
  const guardStart = clampedStart('SMI', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('SMI', 'prices.high', inHigh, guardInLen);
  requireLength('SMI', 'prices.low', inLow, guardInLen);
  requireLength('SMI', 'prices.close', inClose, guardInLen);
  requireOutput('SMI', 'outSmi', outSMI, guardOutLen, Float64Array);
  requireOutput('SMI', 'outSmiSignal', outSMISignal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outSMI, inHighF64)) {
    throw sharedMemory('SMI', 'outSmi', 'prices.high');
  }
  if (overlapsPartly(outSMI, inLowF64)) {
    throw sharedMemory('SMI', 'outSmi', 'prices.low');
  }
  if (overlapsPartly(outSMI, inCloseF64)) {
    throw sharedMemory('SMI', 'outSmi', 'prices.close');
  }
  if (overlapsPartly(outSMISignal, inHighF64)) {
    throw sharedMemory('SMI', 'outSmiSignal', 'prices.high');
  }
  if (overlapsPartly(outSMISignal, inLowF64)) {
    throw sharedMemory('SMI', 'outSmiSignal', 'prices.low');
  }
  if (overlapsPartly(outSMISignal, inCloseF64)) {
    throw sharedMemory('SMI', 'outSmiSignal', 'prices.close');
  }
  if (overlaps(outSMI, outSMISignal)) {
    throw sharedMemory('SMI', 'outSmi', 'outSmiSignal');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = smiImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outSMI, outSMISignal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('SMI', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link smi}, {@link smiLookback}, {@link smiDisplayShift},
 * {@link smiOpen} and {@link smiOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface SmiOptions {
  /**
   * Period of the high/low range (default 13; range 2 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  timePeriod?: number | undefined;
  /**
   * Period of the second smoothing, applied to the first (default 2; range 2 to
   * 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  fastPeriod?: number | undefined;
  /**
   * Period of the first smoothing, applied to the raw momentum (default 25; range 2
   * to 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  slowPeriod?: number | undefined;
  /**
   * Smoothing period of the signal line (default 9; range 2 to 100000;
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
   * Buffer for `outSmi`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outSmi?: Float64Array | undefined;
  /**
   * Buffer for `outSmiSignal`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outSmiSignal?: Float64Array | undefined;
}

/** What {@link smi} returns, and {@link smiOpenAndFill} with its stream. */
export interface SmiResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Stochastic Momentum Index, -100 to +100. `outSmi[k]` is the value at input bar
   * `begIdx + k`.
   */
  outSmi: Float64Array;
  /**
   * Exponential average of the SMI line. `outSmiSignal[k]` is the value at input bar
   * `begIdx + k`.
   */
  outSmiSignal: Float64Array;
}

/**
 * Number of leading input bars {@link smi} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SmiOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function smiLookback(options?: SmiOptions): number {
  const o = optionsOf('SMI lookback', options);
  return smiLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.fastPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowPeriod, Core.INTEGER_DEFAULT), orDefault(o.signalPeriod, Core.INTEGER_DEFAULT), coreOf('SMI lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link smi}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SmiOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function smiDisplayShift(outputIdx: number, options?: SmiOptions): number {
  const o = optionsOf('SMI displayShift', options);
  return smiDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.fastPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowPeriod, Core.INTEGER_DEFAULT), orDefault(o.signalPeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('SMI displayShift', o.core));
}

/**
 * Stochastic Momentum Index: where the close sits relative to the **midpoint** of
 * the recent high/low range, double-smoothed. Lane's stochastic measures the close
 * against the bottom of the range; Blau measures it against the middle, then
 * smooths numerator and denominator separately with two exponential averages
 * before dividing, which is what buys the low-lag, smooth-contoured curve. The
 * result runs -100 to +100 rather than 0 to 100, so the zero line is the
 * reference: positive means the close is above the midpoint of its range, negative
 * below. Extreme readings mark overbought and oversold conditions, and crossings
 * of the signal line are the usual trade trigger.
 *
 * Formula and more info at
 * [ta-lib.org/functions/smi](https://ta-lib.org/functions/smi).
 *
 * **Notes**
 * - A window whose bars are all flat (every high equal to its low) leaves both the
 *   numerator and the denominator at zero. Rather than divide, SMI emits 0 there,
 *   its neutral value, as CCI and IMI emit theirs. Some implementations divide
 *   unguarded and return a non-finite value.
 * - Each exponential average is seeded with a simple average of its own first
 *   inputs, the same seeding TA-Lib's EMA uses, so the first published values
 *   converge toward an unlimited-history result rather than reproducing it
 *   exactly. `CoreBuilder.unstablePeriod(FuncUnstId.EMA, ...)` discards more of
 *   that warm-up. Implementations seeding from a single first sample — Tulip and
 *   TradingView among them — differ over the transient and agree once it decays.
 * - One output range covers both outputs, so the SMI values consumed by the signal
 *   line's own warm-up are not published.
 *
 * `outSmi[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link smiLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `high`: High
 *   price series. `low`: Low price series. `close`: Close price series.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link SmiOptions}.
 * @returns `begIdx` and the values; see {@link SmiResult}. Each output is a view
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
 * import { smi } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { begIdx, outSmi, outSmiSignal } = smi({ high, low, close }, {
 *   timePeriod: 13,
 *   fastPeriod: 2,
 *   slowPeriod: 25,
 *   signalPeriod: 9,
 * });
 * // outSmi[k] is the value at bar begIdx + k
 * ```
 * @see [`stoch`](https://ta-lib.org/functions/stoch)
 * @see [`stochrsi`](https://ta-lib.org/functions/stochrsi)
 * @see [`willr`](https://ta-lib.org/functions/willr)
 * @see [`macd`](https://ta-lib.org/functions/macd)
 */
export function smi(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: SmiOptions): SmiResult {
  requireArgument('SMI', 'prices', prices);
  const o = optionsOf('SMI', options);
  if (o.endIdx === undefined) {
    requireSeries('SMI', 'prices.high', prices.high);
    requireSeries('SMI', 'prices.low', prices.low);
    requireSeries('SMI', 'prices.close', prices.close);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('SMI', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, smiLookbackPositional(optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core), prices.high, prices.low, prices.close);
  const outSMI = o.outSmi === undefined ? new Float64Array(capacity) : o.outSmi;
  const outSMISignal = o.outSmiSignal === undefined ? new Float64Array(capacity) : o.outSmiSignal;
  const range = smiPositional(startIdx, endIdx, prices.high, prices.low, prices.close, optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outSMI, outSMISignal, core);
  return { begIdx: range.begIdx, outSmi: outSMI.subarray(0, range.count), outSmiSignal: outSMISignal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live SMI stream: one value per closed bar, bit-identical to {@link smi} over
 * the same series. Open it with {@link smiOpen} or {@link smiOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/smi](https://ta-lib.org/functions/smi).
 */
export class SmiStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  optInFastPeriod: number = 0;
  /** @internal */
  optInSlowPeriod: number = 0;
  /** @internal */
  optInSignalPeriod: number = 0;
  /** @internal */
  kSlow: number = 0;
  /** @internal */
  kFast: number = 0;
  /** @internal */
  kSignal: number = 0;
  /** @internal */
  betaSlow: number = 0;
  /** @internal */
  betaFast: number = 0;
  /** @internal */
  betaSignal: number = 0;
  /** @internal */
  highest: number = 0;
  /** @internal */
  lowest: number = 0;
  /** @internal */
  emaSlowNum: number = 0;
  /** @internal */
  emaFastNum: number = 0;
  /** @internal */
  emaSlowDen: number = 0;
  /** @internal */
  emaFastDen: number = 0;
  /** @internal */
  prevSignal: number = 0;
  /** @internal */
  trailingIdx: number = 0;
  /** @internal */
  highestIdx: number = 0;
  /** @internal */
  lowestIdx: number = 0;
  /** @internal */
  i: number = 0;
  /** @internal */
  today: number = 0;
  /** @internal */
  xMask: number = 0;
  /** @internal */
  x_inHigh: Float64Array = new Float64Array(1);
  /** @internal */
  x_inLow: Float64Array = new Float64Array(1);
  /** @internal */
  x_inClose: Float64Array = new Float64Array(1);
  /** @internal */
  cur_outSMI: number = 0;
  /** @internal */
  cur_outSMISignal: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): SmiStream {
    return new SmiStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link smi} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('SMI advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns its outputs.
   *
   * @param high - The bar's `high` value: High price series.
   * @param low - The bar's `low` value: Low price series.
   * @param close - The bar's `close` value: Close price series.
   * @param out - Receives the outputs and is returned; a new {@link SmiOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included, before anything moves.
   *   `TALibError` with code `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached
   *   bar `Core.INDEX_MAX`.
   */
  update(high: number, low: number, close: number, out?: SmiOut): SmiOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('SMI update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('SMI update', 'SmiOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('SMI update', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    smiStepImpl(this, high, low, close);
    this.outRangeCount++;
    if (out === undefined) {
      out = new SmiOut();
    }
    out.outSmi = this.cur_outSMI;
    out.outSmiSignal = this.cur_outSMISignal;
    return out;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param high - The bar's `high` value: High price series.
   * @param low - The bar's `low` value: Low price series.
   * @param close - The bar's `close` value: Close price series.
   * @param out - Receives the outputs and is returned; a new {@link SmiOut} when
   *   omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included.
   */
  peek(high: number, low: number, close: number, out?: SmiOut): SmiOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('SMI peek', 'SmiOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('SMI peek', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
    const inClose = close;
    let tmp = 0;
    let num = 0;
    let den = 0;
    let halfDen = 0;
    let smiValue = 0;
    let cur_outSMI = 0;
    let cur_outSMISignal = 0;
    let emaFastDen = sp.emaFastDen;
    let emaFastNum = sp.emaFastNum;
    let emaSlowDen = sp.emaSlowDen;
    let emaSlowNum = sp.emaSlowNum;
    let highest = sp.highest;
    let highestIdx = sp.highestIdx;
    let i = sp.i;
    let lowest = sp.lowest;
    let lowestIdx = sp.lowestIdx;
    let prevSignal = sp.prevSignal;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    let pkSlot1 = -1;
    let pkVal1 = 0;
    let pkSlot2 = -1;
    let pkVal2 = 0;
    pkSlot0 = sp.today & sp.xMask;
    pkVal0 = inHigh;
    pkSlot1 = sp.today & sp.xMask;
    pkVal1 = inLow;
    pkSlot2 = sp.today & sp.xMask;
    pkVal2 = inClose;
    /* Set the lowest low */
    tmp = ((sp.today & sp.xMask) !== pkSlot1) ? sp.x_inLow[sp.today & sp.xMask] : pkVal1;
    if (lowestIdx < sp.trailingIdx) {
      lowestIdx = sp.trailingIdx;
      lowest = ((lowestIdx & sp.xMask) !== pkSlot1) ? sp.x_inLow[lowestIdx & sp.xMask] : pkVal1;
      i = lowestIdx;
      while (++i <= sp.today) {
        tmp = ((i & sp.xMask) !== pkSlot1) ? sp.x_inLow[i & sp.xMask] : pkVal1;
        if (tmp < lowest) {
          lowestIdx = i;
          lowest = tmp;
        }
      }
    } else if (tmp <= lowest) {
      lowestIdx = sp.today;
      lowest = tmp;
    }
    /* Set the highest high */
    tmp = ((sp.today & sp.xMask) !== pkSlot0) ? sp.x_inHigh[sp.today & sp.xMask] : pkVal0;
    if (highestIdx < sp.trailingIdx) {
      highestIdx = sp.trailingIdx;
      highest = ((highestIdx & sp.xMask) !== pkSlot0) ? sp.x_inHigh[highestIdx & sp.xMask] : pkVal0;
      i = highestIdx;
      while (++i <= sp.today) {
        tmp = ((i & sp.xMask) !== pkSlot0) ? sp.x_inHigh[i & sp.xMask] : pkVal0;
        if (tmp > highest) {
          highestIdx = i;
          highest = tmp;
        }
      }
    } else if (tmp >= highest) {
      highestIdx = sp.today;
      highest = tmp;
    }
    den = highest - lowest;
    num = (((sp.today & sp.xMask) !== pkSlot2) ? sp.x_inClose[sp.today & sp.xMask] : pkVal2) - (highest + lowest) * 0.5;
    {
      const _a: number = sp.betaSlow, _b: number = emaSlowNum, _c: number = sp.kSlow * num;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaSlowNum = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = sp.betaSlow, _b: number = emaSlowDen, _c: number = sp.kSlow * den;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaSlowDen = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = sp.betaFast, _b: number = emaFastNum, _c: number = sp.kFast * emaSlowNum;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaFastNum = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = sp.betaFast, _b: number = emaFastDen, _c: number = sp.kFast * emaSlowDen;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaFastDen = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    /* The denominator is an EMA of an EMA of the high-low range: every term
     * is non-negative and every weight is positive, so it carries no
     * cancellation residue and is zero only when every range that reached it
     * was exactly zero -- 0/0, since a window of H == L bars makes num zero
     * too, reported as the neutral 0.0 by the CCI (#7) and IMI (#112)
     * convention. Test it exactly: the range carries the quote unit, so the
     * fixed TA_IS_ZERO band this used to be zeroed the oscillator for any
     * instrument quoted below it (issue #253). Issue #107's machine-flat
     * window is caught by the exact test as well, since the residue an
     * EMA leaves there is zero, not sub-epsilon.
     */
    halfDen = 0.5 * emaFastDen;
    if (halfDen > 0.0) {
      smiValue = 100.0 * emaFastNum / halfDen;
    } else {
      smiValue = 0.0;
    }
    {
      const _a: number = sp.betaSignal, _b: number = prevSignal, _c: number = sp.kSignal * smiValue;
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
    cur_outSMI = smiValue;
    cur_outSMISignal = prevSignal;
    if (out === undefined) {
      out = new SmiOut();
    }
    out.outSmi = cur_outSMI;
    out.outSmiSignal = cur_outSMISignal;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link SmiOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a sink that is not an object,
   *   `null` included.
   */
  value(out?: SmiOut): SmiOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('SMI value', 'SmiOut');
    }
    if (out === undefined) {
      out = new SmiOut();
    }
    out.outSmi = this.cur_outSMI;
    out.outSmiSignal = this.cur_outSMISignal;
    return out;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): SmiStream {
    const c = new SmiStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.optInFastPeriod = this.optInFastPeriod;
    c.optInSlowPeriod = this.optInSlowPeriod;
    c.optInSignalPeriod = this.optInSignalPeriod;
    c.kSlow = this.kSlow;
    c.kFast = this.kFast;
    c.kSignal = this.kSignal;
    c.betaSlow = this.betaSlow;
    c.betaFast = this.betaFast;
    c.betaSignal = this.betaSignal;
    c.highest = this.highest;
    c.lowest = this.lowest;
    c.emaSlowNum = this.emaSlowNum;
    c.emaFastNum = this.emaFastNum;
    c.emaSlowDen = this.emaSlowDen;
    c.emaFastDen = this.emaFastDen;
    c.prevSignal = this.prevSignal;
    c.trailingIdx = this.trailingIdx;
    c.highestIdx = this.highestIdx;
    c.lowestIdx = this.lowestIdx;
    c.i = this.i;
    c.today = this.today;
    c.xMask = this.xMask;
    c.x_inHigh = this.x_inHigh.slice();
    c.x_inLow = this.x_inLow.slice();
    c.x_inClose = this.x_inClose.slice();
    c.cur_outSMI = this.cur_outSMI;
    c.cur_outSMISignal = this.cur_outSMISignal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one SMI bar, as `update`, `peek` and `value` return them. Passed
 * back to one of them, it is filled and returned again, with no allocation; it
 * holds its values until the next call that writes it. A new one holds `NaN`.
 */
export class SmiOut {
  /** Stochastic Momentum Index, -100 to +100. */
  outSmi: number = NaN;
  /** Exponential average of the SMI line. */
  outSmiSignal: number = NaN;
}

function smiStepImpl(sp: SmiStream, inHigh: number, inLow: number, inClose: number): void {
  let tmp = 0;
  let num = 0;
  let den = 0;
  let halfDen = 0;
  let smiValue = 0;
  sp.x_inHigh[sp.today & sp.xMask] = inHigh;
  sp.x_inLow[sp.today & sp.xMask] = inLow;
  sp.x_inClose[sp.today & sp.xMask] = inClose;
  /* Set the lowest low */
  tmp = sp.x_inLow[sp.today & sp.xMask];
  if (sp.lowestIdx < sp.trailingIdx) {
    sp.lowestIdx = sp.trailingIdx;
    sp.lowest = sp.x_inLow[sp.lowestIdx & sp.xMask];
    sp.i = sp.lowestIdx;
    while (++sp.i <= sp.today) {
      tmp = sp.x_inLow[sp.i & sp.xMask];
      if (tmp < sp.lowest) {
        sp.lowestIdx = sp.i;
        sp.lowest = tmp;
      }
    }
  } else if (tmp <= sp.lowest) {
    sp.lowestIdx = sp.today;
    sp.lowest = tmp;
  }
  /* Set the highest high */
  tmp = sp.x_inHigh[sp.today & sp.xMask];
  if (sp.highestIdx < sp.trailingIdx) {
    sp.highestIdx = sp.trailingIdx;
    sp.highest = sp.x_inHigh[sp.highestIdx & sp.xMask];
    sp.i = sp.highestIdx;
    while (++sp.i <= sp.today) {
      tmp = sp.x_inHigh[sp.i & sp.xMask];
      if (tmp > sp.highest) {
        sp.highestIdx = sp.i;
        sp.highest = tmp;
      }
    }
  } else if (tmp >= sp.highest) {
    sp.highestIdx = sp.today;
    sp.highest = tmp;
  }
  den = sp.highest - sp.lowest;
  num = sp.x_inClose[sp.today & sp.xMask] - (sp.highest + sp.lowest) * 0.5;
  {
    const _a: number = sp.betaSlow, _b: number = sp.emaSlowNum, _c: number = sp.kSlow * num;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.emaSlowNum = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  {
    const _a: number = sp.betaSlow, _b: number = sp.emaSlowDen, _c: number = sp.kSlow * den;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.emaSlowDen = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  {
    const _a: number = sp.betaFast, _b: number = sp.emaFastNum, _c: number = sp.kFast * sp.emaSlowNum;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.emaFastNum = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  {
    const _a: number = sp.betaFast, _b: number = sp.emaFastDen, _c: number = sp.kFast * sp.emaSlowDen;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.emaFastDen = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  /* The denominator is an EMA of an EMA of the high-low range: every term
   * is non-negative and every weight is positive, so it carries no
   * cancellation residue and is zero only when every range that reached it
   * was exactly zero -- 0/0, since a window of H == L bars makes num zero
   * too, reported as the neutral 0.0 by the CCI (#7) and IMI (#112)
   * convention. Test it exactly: the range carries the quote unit, so the
   * fixed TA_IS_ZERO band this used to be zeroed the oscillator for any
   * instrument quoted below it (issue #253). Issue #107's machine-flat
   * window is caught by the exact test as well, since the residue an
   * EMA leaves there is zero, not sub-epsilon.
   */
  halfDen = 0.5 * sp.emaFastDen;
  if (halfDen > 0.0) {
    smiValue = 100.0 * sp.emaFastNum / halfDen;
  } else {
    smiValue = 0.0;
  }
  {
    const _a: number = sp.betaSignal, _b: number = sp.prevSignal, _c: number = sp.kSignal * smiValue;
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
  sp.cur_outSMI = smiValue;
  sp.cur_outSMISignal = sp.prevSignal;
  sp.trailingIdx = sp.trailingIdx + 1;
  sp.today = sp.today + 1;
}

function smiOpenImpl(
  sp: SmiStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outSMI: Float64Array,
  outSMISignal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let kSlow = 0;
  let kFast = 0;
  let kSignal = 0;
  let betaSlow = 0;
  let betaFast = 0;
  let betaSignal = 0;
  let highest = 0;
  let lowest = 0;
  let tmp = 0;
  let emaSlowNum = 0;
  let emaFastNum = 0;
  let emaSlowDen = 0;
  let emaFastDen = 0;
  let sumSlowNum = 0;
  let sumSlowDen = 0;
  let sumFastNum = 0;
  let sumFastDen = 0;
  let sumSignal = 0;
  let num = 0;
  let den = 0;
  let halfDen = 0;
  let smiValue = 0;
  let prevSignal = 0;
  let lookbackTotal = 0;
  let lookbackSlow = 0;
  let lookbackFast = 0;
  let today = 0;
  let trailingIdx = 0;
  let highestIdx = 0;
  let lowestIdx = 0;
  let i = 0;
  let outIdx = 0;
  let nBar = 0;
  let nFast = 0;
  let nSignal = 0;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length || inClose.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 13;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInFastPeriod === Core.INTEGER_DEFAULT) {
    optInFastPeriod = 2;
  } else if (optInFastPeriod < 2 || optInFastPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowPeriod === Core.INTEGER_DEFAULT) {
    optInSlowPeriod = 25;
  } else if (optInSlowPeriod < 2 || optInSlowPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSignalPeriod === Core.INTEGER_DEFAULT) {
    optInSignalPeriod = 9;
  } else if (optInSignalPeriod < 2 || optInSignalPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Declared Num, Num, Den, Den: the stream state keeps this order, and with
   * each stage's pair adjacent gcc 13 packs the steps two by two and then
   * reads 16 bytes across two of its own stores, which cannot be forwarded.
   */
  lookbackTotal = smiLookbackPositional(optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outBegIdx.value = startIdx;
  /* Blau's pipeline in one pass. A composed form is not available: the
   * streaming producer model carries exactly one intermediate series
   * (streaming.rs:2076) and SMI has two, the numerator and the denominator,
   * which must be smoothed separately before they are divided.
   *
   * Each of the three stages seeds the way ema.c does -- a simple average of
   * that stage's first 'period' inputs -- so the result is bit-identical to
   * TA_MAX + TA_MIN followed by four TA_EMA calls. The stage boundaries below
   * are the callee LOOKBACKS, not (period-1), so that a warm
   * TA_SetUnstablePeriod(TA_FUNC_UNST_EMA) folds in: each stage then seeds
   * from the values its predecessor would have published, exactly as the
   * composed form does. The seed sums accumulate from 0.0 in production
   * order; do not reorder or fuse them (0.0+x is not x for x=-0.0).
   */
  betaSlow = (optInSlowPeriod - 1) / (optInSlowPeriod + 1);
  kSlow = 1.0 - betaSlow;
  if (betaSlow < 0.5) {
    betaSlow = 1.0 - kSlow;
  }
  betaFast = (optInFastPeriod - 1) / (optInFastPeriod + 1);
  kFast = 1.0 - betaFast;
  if (betaFast < 0.5) {
    betaFast = 1.0 - kFast;
  }
  betaSignal = (optInSignalPeriod - 1) / (optInSignalPeriod + 1);
  kSignal = 1.0 - betaSignal;
  if (betaSignal < 0.5) {
    betaSignal = 1.0 - kSignal;
  }
  lookbackSlow = emaLookbackPositional(optInSlowPeriod, core);
  lookbackFast = emaLookbackPositional(optInFastPeriod, core);
  emaSlowNum = 0.0;
  emaSlowDen = 0.0;
  emaFastNum = 0.0;
  emaFastDen = 0.0;
  prevSignal = 0.0;
  smiValue = 0.0;
  sumSlowNum = 0.0;
  sumSlowDen = 0.0;
  sumFastNum = 0.0;
  sumFastDen = 0.0;
  sumSignal = 0.0;
  highest = 0.0;
  lowest = 0.0;
  highestIdx = -1;
  lowestIdx = -1;
  /* The first bar carrying a full high/low window. */
  trailingIdx = startIdx - lookbackTotal;
  today = trailingIdx + (optInTimePeriod - 1);
  nBar = 0;
  /* Warm-up. Runs through startIdx inclusive: the last pass here is the one
   * that completes the signal seed, so it produces the first output pair.
   */
  while (today <= startIdx) {
    /* Set the lowest low */
    tmp = inLow[today];
    if (lowestIdx < trailingIdx) {
      lowestIdx = trailingIdx;
      lowest = inLow[lowestIdx];
      i = lowestIdx;
      while (++i <= today) {
        tmp = inLow[i];
        if (tmp < lowest) {
          lowestIdx = i;
          lowest = tmp;
        }
      }
    } else if (tmp <= lowest) {
      lowestIdx = today;
      lowest = tmp;
    }
    /* Set the highest high */
    tmp = inHigh[today];
    if (highestIdx < trailingIdx) {
      highestIdx = trailingIdx;
      highest = inHigh[highestIdx];
      i = highestIdx;
      while (++i <= today) {
        tmp = inHigh[i];
        if (tmp > highest) {
          highestIdx = i;
          highest = tmp;
        }
      }
    } else if (tmp >= highest) {
      highestIdx = today;
      highest = tmp;
    }
    den = highest - lowest;
    num = inClose[today] - (highest + lowest) * 0.5;
    /* Stage 1: the slow EMA, over the raw momentum. */
    if (nBar < optInSlowPeriod) {
      sumSlowNum = sumSlowNum + num;
      sumSlowDen = sumSlowDen + den;
      if (nBar === optInSlowPeriod - 1) {
        emaSlowNum = sumSlowNum / optInSlowPeriod;
        emaSlowDen = sumSlowDen / optInSlowPeriod;
      }
    } else {
      {
        const _a: number = betaSlow, _b: number = emaSlowNum, _c: number = kSlow * num;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        emaSlowNum = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      {
        const _a: number = betaSlow, _b: number = emaSlowDen, _c: number = kSlow * den;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        emaSlowDen = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
    }
    /* Stage 2: the fast EMA, over what stage 1 publishes.
     *
     * The stage counters are compared BEFORE they are subtracted, never
     * after. Writing this as `nFast = nBar - lookbackSlow; if( nFast >= 0 )`
     * is correct in C, where the counters are signed, and broken everywhere
     * else: the Rust backend renders them as `usize`, so the subtraction
     * underflows for the first lookbackSlow bars -- a panic in a debug build
     * and a wrap in release. It would also be invisible to the cross-language
     * gate, which runs release servers at unstable period 0, where the branch
     * the wrap wrongly takes happens to be a no-op because both accumulators
     * are still 0.0.
     */
    if (nBar >= lookbackSlow) {
      nFast = nBar - lookbackSlow;
      if (nFast < optInFastPeriod) {
        sumFastNum = sumFastNum + emaSlowNum;
        sumFastDen = sumFastDen + emaSlowDen;
        if (nFast === optInFastPeriod - 1) {
          emaFastNum = sumFastNum / optInFastPeriod;
          emaFastDen = sumFastDen / optInFastPeriod;
        }
      } else {
        {
          const _a: number = betaFast, _b: number = emaFastNum, _c: number = kFast * emaSlowNum;
          const _p = _a * _b;
          let _t = 134217729 * _a;
          const _ah = _t - (_t - _a), _al = _a - _ah;
          _t = 134217729 * _b;
          const _bh = _t - (_t - _b), _bl = _b - _bh;
          const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
          const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
          const _v = _tl + _pl, _v2 = _v + _v;
          const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
          emaFastNum = _w && (_pl === 0 || _v === 0) ? _th
            : _w && _v2 !== (_th + _v2) - _th ? _th + _v
            : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
        }
        {
          const _a: number = betaFast, _b: number = emaFastDen, _c: number = kFast * emaSlowDen;
          const _p = _a * _b;
          let _t = 134217729 * _a;
          const _ah = _t - (_t - _a), _al = _a - _ah;
          _t = 134217729 * _b;
          const _bh = _t - (_t - _b), _bl = _b - _bh;
          const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
          const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
          const _v = _tl + _pl, _v2 = _v + _v;
          const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
          emaFastDen = _w && (_pl === 0 || _v === 0) ? _th
            : _w && _v2 !== (_th + _v2) - _th ? _th + _v
            : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
        }
      }
    }
    /* Stage 3: the SMI line, then the signal EMA over it. */
    if (nBar >= lookbackSlow + lookbackFast) {
      nSignal = nBar - lookbackSlow - lookbackFast;
      halfDen = 0.5 * emaFastDen;
      if (halfDen > 0.0) {
        smiValue = 100.0 * emaFastNum / halfDen;
      } else {
        smiValue = 0.0;
      }
      if (nSignal < optInSignalPeriod) {
        sumSignal = sumSignal + smiValue;
        if (nSignal === optInSignalPeriod - 1) {
          prevSignal = sumSignal / optInSignalPeriod;
        }
      } else {
        {
          const _a: number = betaSignal, _b: number = prevSignal, _c: number = kSignal * smiValue;
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
    nBar = nBar + 1;
    trailingIdx = trailingIdx + 1;
    today = today + 1;
  }
  outSMI[((0 * outStride) | 0)] = smiValue;
  outSMISignal[((0 * outStride) | 0)] = prevSignal;
  outIdx = 1;
  /* Stable zone. Every stage is a pure recursion from here on. */
  while (today <= endIdx) {
    /* Set the lowest low */
    tmp = inLow[today];
    if (lowestIdx < trailingIdx) {
      lowestIdx = trailingIdx;
      lowest = inLow[lowestIdx];
      i = lowestIdx;
      while (++i <= today) {
        tmp = inLow[i];
        if (tmp < lowest) {
          lowestIdx = i;
          lowest = tmp;
        }
      }
    } else if (tmp <= lowest) {
      lowestIdx = today;
      lowest = tmp;
    }
    /* Set the highest high */
    tmp = inHigh[today];
    if (highestIdx < trailingIdx) {
      highestIdx = trailingIdx;
      highest = inHigh[highestIdx];
      i = highestIdx;
      while (++i <= today) {
        tmp = inHigh[i];
        if (tmp > highest) {
          highestIdx = i;
          highest = tmp;
        }
      }
    } else if (tmp >= highest) {
      highestIdx = today;
      highest = tmp;
    }
    den = highest - lowest;
    num = inClose[today] - (highest + lowest) * 0.5;
    {
      const _a: number = betaSlow, _b: number = emaSlowNum, _c: number = kSlow * num;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaSlowNum = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = betaSlow, _b: number = emaSlowDen, _c: number = kSlow * den;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaSlowDen = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = betaFast, _b: number = emaFastNum, _c: number = kFast * emaSlowNum;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaFastNum = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = betaFast, _b: number = emaFastDen, _c: number = kFast * emaSlowDen;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaFastDen = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    /* The denominator is an EMA of an EMA of the high-low range: every term
     * is non-negative and every weight is positive, so it carries no
     * cancellation residue and is zero only when every range that reached it
     * was exactly zero -- 0/0, since a window of H == L bars makes num zero
     * too, reported as the neutral 0.0 by the CCI (#7) and IMI (#112)
     * convention. Test it exactly: the range carries the quote unit, so the
     * fixed TA_IS_ZERO band this used to be zeroed the oscillator for any
     * instrument quoted below it (issue #253). Issue #107's machine-flat
     * window is caught by the exact test as well, since the residue an
     * EMA leaves there is zero, not sub-epsilon.
     */
    halfDen = 0.5 * emaFastDen;
    if (halfDen > 0.0) {
      smiValue = 100.0 * emaFastNum / halfDen;
    } else {
      smiValue = 0.0;
    }
    {
      const _a: number = betaSignal, _b: number = prevSignal, _c: number = kSignal * smiValue;
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
    outSMI[((outIdx * outStride) | 0)] = smiValue;
    outSMISignal[((outIdx * outStride) | 0)] = prevSignal;
    outIdx = outIdx + 1;
    trailingIdx = trailingIdx + 1;
    today = today + 1;
  }
  outNBElement.value = outIdx;
  const capX = today - trailingIdx + 1;
  if (capX < 1 || capX > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  let physX = 1;
  while (physX < capX) {
    physX <<= 1;
  }
  const capX_inHigh = new Float64Array(physX);
  const capX_inLow = new Float64Array(physX);
  const capX_inClose = new Float64Array(physX);
  for (let fillJ = historyLen - capX; fillJ < historyLen; fillJ++) {
    capX_inHigh[fillJ & (physX - 1)] = inHigh[fillJ];
    capX_inLow[fillJ & (physX - 1)] = inLow[fillJ];
    capX_inClose[fillJ & (physX - 1)] = inClose[fillJ];
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.optInFastPeriod = optInFastPeriod;
  sp.optInSlowPeriod = optInSlowPeriod;
  sp.optInSignalPeriod = optInSignalPeriod;
  sp.kSlow = kSlow;
  sp.kFast = kFast;
  sp.kSignal = kSignal;
  sp.betaSlow = betaSlow;
  sp.betaFast = betaFast;
  sp.betaSignal = betaSignal;
  sp.highest = highest;
  sp.lowest = lowest;
  sp.emaSlowNum = emaSlowNum;
  sp.emaFastNum = emaFastNum;
  sp.emaSlowDen = emaSlowDen;
  sp.emaFastDen = emaFastDen;
  sp.prevSignal = prevSignal;
  sp.trailingIdx = trailingIdx;
  sp.highestIdx = highestIdx;
  sp.lowestIdx = lowestIdx;
  sp.i = i;
  sp.today = today;
  sp.xMask = physX - 1;
  sp.x_inHigh = capX_inHigh;
  sp.x_inLow = capX_inLow;
  sp.x_inClose = capX_inClose;
  sp.cur_outSMI = outSMI[(outNBElement.value - 1) * outStride];
  sp.cur_outSMISignal = outSMISignal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function smiOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outSMI: Float64Array,
  outSMISignal: Float64Array,
): SmiStream {
  const sp = SmiStream.of(core);
  const retCode = smiOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outSMI, outSMISignal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SMI openAndFill', inHigh.length, startIdx, smiLookbackPositional(optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && smiLookbackPositional(optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core) < 0) {
    throw rejectedParams('SMI openAndFill', PARAM_RANGES, optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
  }
  throw streamFailure('SMI openAndFill', retCode);
}

/** @internal */
export function smiOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
): SmiStream {
  const sp = SmiStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outSMI = new Float64Array(1);
  const sink_outSMISignal = new Float64Array(1);
  const retCode = smiOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, sink_outSMI, sink_outSMISignal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SMI open', inHigh.length, startIdx, smiLookbackPositional(optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && smiLookbackPositional(optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core) < 0) {
    throw rejectedParams('SMI open', PARAM_RANGES, optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
  }
  throw streamFailure('SMI open', retCode);
}

/** @internal */
export function smiOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  core: Core = Core.DEFAULT,
): SmiStream {
  optInTimePeriod = requireInt32('SMI open', 'timePeriod', optInTimePeriod);
  optInFastPeriod = requireInt32('SMI open', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('SMI open', 'slowPeriod', optInSlowPeriod);
  optInSignalPeriod = requireInt32('SMI open', 'signalPeriod', optInSignalPeriod);
  requireArgument('SMI open', 'prices.high', inHigh);
  requireHistory('SMI open', 'prices.high', inHigh.length);
  requireArgument('SMI open', 'prices.low', inLow);
  requireArgument('SMI open', 'prices.close', inClose);
  requireHistoryLength('SMI open', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('SMI open', 'prices.close', inClose.length, inHigh.length);
  return smiOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
}

/** @internal */
export function smiOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  outSMI: Float64Array,
  outSMISignal: Float64Array,
  core: Core = Core.DEFAULT,
): SmiStream {
  optInTimePeriod = requireInt32('SMI openAndFill', 'timePeriod', optInTimePeriod);
  optInFastPeriod = requireInt32('SMI openAndFill', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('SMI openAndFill', 'slowPeriod', optInSlowPeriod);
  optInSignalPeriod = requireInt32('SMI openAndFill', 'signalPeriod', optInSignalPeriod);
  requireArgument('SMI openAndFill', 'prices.high', inHigh);
  requireHistory('SMI openAndFill', 'prices.high', inHigh.length);
  requireArgument('SMI openAndFill', 'prices.low', inLow);
  requireArgument('SMI openAndFill', 'prices.close', inClose);
  const guardLookback = smiLookbackPositional(optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('SMI openAndFill', PARAM_RANGES, optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
  }
  const guardOutLen = openFillCount('SMI openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('SMI openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('SMI openAndFill', 'prices.close', inClose.length, inHigh.length);
  requireOutput('SMI openAndFill', 'outSmi', outSMI, guardOutLen, Float64Array);
  requireOutput('SMI openAndFill', 'outSmiSignal', outSMISignal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outSMI.buffer === inHighF64.buffer && outSMI.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('SMI openAndFill', 'outSmi', 'prices.high');
  }
  if (outSMI.buffer === inLowF64.buffer && outSMI.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('SMI openAndFill', 'outSmi', 'prices.low');
  }
  if (outSMI.buffer === inCloseF64.buffer && outSMI.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('SMI openAndFill', 'outSmi', 'prices.close');
  }
  if (outSMISignal.buffer === inHighF64.buffer && outSMISignal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('SMI openAndFill', 'outSmiSignal', 'prices.high');
  }
  if (outSMISignal.buffer === inLowF64.buffer && outSMISignal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('SMI openAndFill', 'outSmiSignal', 'prices.low');
  }
  if (outSMISignal.buffer === inCloseF64.buffer && outSMISignal.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('SMI openAndFill', 'outSmiSignal', 'prices.close');
  }
  if (outSMI.buffer === outSMISignal.buffer && outSMI.byteOffset === outSMISignal.byteOffset) {
    throw sharedMemory('SMI openAndFill', 'outSmi', 'outSmiSignal');
  }
  if (guardOutLen > 0 && overlaps(outSMI, inHighF64)) {
    throw sharedMemory('SMI openAndFill', 'outSmi', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outSMI, inLowF64)) {
    throw sharedMemory('SMI openAndFill', 'outSmi', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outSMI, inCloseF64)) {
    throw sharedMemory('SMI openAndFill', 'outSmi', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outSMISignal, inHighF64)) {
    throw sharedMemory('SMI openAndFill', 'outSmiSignal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outSMISignal, inLowF64)) {
    throw sharedMemory('SMI openAndFill', 'outSmiSignal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outSMISignal, inCloseF64)) {
    throw sharedMemory('SMI openAndFill', 'outSmiSignal', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outSMI, outSMISignal)) {
    throw sharedMemory('SMI openAndFill', 'outSmi', 'outSmiSignal');
  }
  return smiOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, new MInteger(), new MInteger(), outSMI, outSMISignal);
}

/**
 * Opens a live SMI stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link smi} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price series. `low`: Low price series. `close`: Close price
 *   series.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SmiOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `smiLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or
 *   histories of different lengths.
 *
 * @example
 * ```ts
 * import { smiOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = smiOpen({ high, low, close }, {
 *   timePeriod: 13,
 *   fastPeriod: 2,
 *   slowPeriod: 25,
 *   signalPeriod: 9,
 * });
 * const peeked = stream.peek(101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function smiOpen(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: SmiOptions): SmiStream {
  requireArgument('SMI open', 'prices', prices);
  const o = optionsOf('SMI open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('SMI open', o.core);
  return smiOpenPositional(prices.high, prices.low, prices.close, optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core);
}

/**
 * {@link smiOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link smi}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price series. `low`: Low price series. `close`: Close price
 *   series.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link SmiOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link smi} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `smiLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { smiOpenAndFill } from 'ta-lib';
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
 *   outSmi,
 *   outSmiSignal,
 * } = smiOpenAndFill({ high, low, close }, {
 *   timePeriod: 13,
 *   fastPeriod: 2,
 *   slowPeriod: 25,
 *   signalPeriod: 9,
 * });
 * // outSmi[k] is the value at bar begIdx + k
 * ```
 */
export function smiOpenAndFill(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: SmiOptions): SmiResult & { stream: SmiStream } {
  requireArgument('SMI openAndFill', 'prices', prices);
  const o = optionsOf('SMI openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('SMI openAndFill', o.core);
  const capacity = fillCapacity(smiLookbackPositional(optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core), prices.high, prices.low, prices.close);
  const outSMI = o.outSmi === undefined ? new Float64Array(capacity) : o.outSmi;
  const outSMISignal = o.outSmiSignal === undefined ? new Float64Array(capacity) : o.outSmiSignal;
  const stream = smiOpenAndFillPositional(prices.high, prices.low, prices.close, optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outSMI, outSMISignal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outSmi: outSMI.subarray(0, range.count), outSmiSignal: outSMISignal.subarray(0, range.count) };
}
