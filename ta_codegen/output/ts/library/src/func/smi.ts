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

import { badSink, clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { emaLookback } from './ema.ts';

/**
 * Number of leading input bars {@link smi} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Period of the high/low range (default 13; range 2 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInFastPeriod - Period of the second smoothing, applied to the first
 *   (default 2; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowPeriod - Period of the first smoothing, applied to the raw
 *   momentum (default 25; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param optInSignalPeriod - Smoothing period of the signal line (default 9; range
 *   2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function smiLookback(optInTimePeriod: number, optInFastPeriod: number, optInSlowPeriod: number, optInSignalPeriod: number, core: Core = Core.DEFAULT): number {
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
  return optInTimePeriod - 1 + emaLookback(optInSlowPeriod, core) + emaLookback(optInFastPeriod, core) + emaLookback(optInSignalPeriod, core);
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link smi}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Period of the high/low range (default 13; range 2 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInFastPeriod - Period of the second smoothing, applied to the first
 *   (default 2; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowPeriod - Period of the first smoothing, applied to the raw
 *   momentum (default 25; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param optInSignalPeriod - Smoothing period of the signal line (default 9; range
 *   2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function smiDisplayShift(optInTimePeriod: number, optInFastPeriod: number, optInSlowPeriod: number, optInSignalPeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (smiLookback(optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core) < 0) {
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
  lookbackTotal = smiLookback(optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core);
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
  lookbackSlow = emaLookback(optInSlowPeriod, core);
  lookbackFast = emaLookback(optInFastPeriod, core);
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
 *   exactly. `TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)` discards more of that
 *   warm-up. Implementations seeding from a single first sample — Tulip and
 *   TradingView among them — differ over the transient and agree once it decays.
 * - One output range covers both outputs, so the SMI values consumed by the signal
 *   line's own warm-up are not published.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link smiLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price series.
 * @param inLow - Low price series.
 * @param inClose - Close price series.
 * @param optInTimePeriod - Period of the high/low range (default 13; range 2 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInFastPeriod - Period of the second smoothing, applied to the first
 *   (default 2; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowPeriod - Period of the first smoothing, applied to the raw
 *   momentum (default 25; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param optInSignalPeriod - Smoothing period of the signal line (default 9; range
 *   2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outSMI - Stochastic Momentum Index, -100 to +100. Must hold the values
 *   produced and never be empty; the input length always suffices.
 * @param outSMISignal - Exponential average of the SMI line. Must hold the values
 *   produced and never be empty; the input length always suffices.
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
 * import { smi } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const smiOut = new Float64Array(high.length);
 * const smiSignal = new Float64Array(high.length);
 * const { begIdx, count } = smi(
 *   0, high.length - 1,
 *   high, low, close,
 *   13, 2, 25, 9,
 *   smiOut, smiSignal,
 * );
 * // smiOut[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`stoch`](https://ta-lib.org/functions/stoch)
 * @see [`stochrsi`](https://ta-lib.org/functions/stochrsi)
 * @see [`willr`](https://ta-lib.org/functions/willr)
 * @see [`macd`](https://ta-lib.org/functions/macd)
 */
export function smi(
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
  optInTimePeriod = requireInt32('SMI', 'optInTimePeriod', optInTimePeriod);
  optInFastPeriod = requireInt32('SMI', 'optInFastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('SMI', 'optInSlowPeriod', optInSlowPeriod);
  optInSignalPeriod = requireInt32('SMI', 'optInSignalPeriod', optInSignalPeriod);
  requireIndexRange('SMI', startIdx, endIdx);
  const guardStart = clampedStart('SMI', startIdx, smiLookback(optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('SMI', 'inHigh', inHigh, guardInLen);
  requireLength('SMI', 'inLow', inLow, guardInLen);
  requireLength('SMI', 'inClose', inClose, guardInLen);
  requireLength('SMI', 'outSMI', outSMI, guardOutLen);
  requireLength('SMI', 'outSMISignal', outSMISignal, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outSMI, inHighF64) || overlapsPartly(outSMI, inLowF64) || overlapsPartly(outSMI, inCloseF64) || overlapsPartly(outSMISignal, inHighF64) || overlapsPartly(outSMISignal, inLowF64) || overlapsPartly(outSMISignal, inCloseF64) || overlaps(outSMI, outSMISignal)) {
    throw failure('SMI', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = smiImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outSMI, outSMISignal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('SMI', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
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
      throw failure('SMI advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar, writing the new values into `out`.
   *
   * @param inHigh - The bar's `inHigh` value: High price series.
   * @param inLow - The bar's `inLow` value: Low price series.
   * @param inClose - The bar's `inClose` value: Close price series.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number, inClose: number, out: SmiOut): void {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('SMI update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (typeof out !== 'object' || out === null) {
      throw badSink('SMI update', 'SmiOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('SMI update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    smiStepImpl(this, inHigh, inLow, inClose);
    this.outRangeCount++;
    out.smi = this.cur_outSMI;
    out.smiSignal = this.cur_outSMISignal;
  }

  /**
   * Writes into `out` what the next `update` with the same bar would write,
   * committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price series.
   * @param inLow - The bar's `inLow` value: Low price series.
   * @param inClose - The bar's `inClose` value: Close price series.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number, inClose: number, out: SmiOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('SMI peek', 'SmiOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('SMI peek', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
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
    out.smi = cur_outSMI;
    out.smiSignal = cur_outSMISignal;
  }

  /**
   * The value at the last bar this stream counted; `peek` does not change it.
   *
   * @param out - Receives the outputs.
   */
  value(out: SmiOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('SMI value', 'SmiOut');
    }
    out.smi = this.cur_outSMI;
    out.smiSignal = this.cur_outSMISignal;
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
 * The outputs of one SMI bar, written into an object the caller owns and reuses:
 * valid until the next call that writes it.
 */
export class SmiOut {
  /** Stochastic Momentum Index, -100 to +100. */
  smi: number = 0;
  /** Exponential average of the SMI line. */
  smiSignal: number = 0;
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
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
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
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Declared Num, Num, Den, Den: the stream state keeps this order, and with
   * each stage's pair adjacent gcc 13 packs the steps two by two and then
   * reads 16 bytes across two of its own stores, which cannot be forwarded.
   */
  lookbackTotal = smiLookback(optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core);
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
  lookbackSlow = emaLookback(optInSlowPeriod, core);
  lookbackFast = emaLookback(optInFastPeriod, core);
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
    throw insufficientHistory('SMI openAndFill', inHigh.length, startIdx, smiLookback(optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core));
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
    throw insufficientHistory('SMI open', inHigh.length, startIdx, smiLookback(optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core));
  }
  throw streamFailure('SMI open', retCode);
}

/**
 * Opens a live SMI stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link smi} there.
 *
 * @param inHigh - Warm-up history. High price series. Every history has the same
 *   length.
 * @param inLow - Warm-up history. Low price series. Every history has the same
 *   length.
 * @param inClose - Warm-up history. Close price series. Every history has the same
 *   length.
 * @param optInTimePeriod - Period of the high/low range (default 13; range 2 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInFastPeriod - Period of the second smoothing, applied to the first
 *   (default 2; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowPeriod - Period of the first smoothing, applied to the raw
 *   momentum (default 25; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param optInSignalPeriod - Smoothing period of the signal line (default 9; range
 *   2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `smiLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { SmiOut, smiOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = smiOpen(high, low, close, 13, 2, 25, 9);
 * const peeked = new SmiOut();
 * const updated = new SmiOut();
 * stream.peek(101.4, 99.1, 100.9, peeked); // a forming bar: commits nothing
 * stream.update(101.4, 99.1, 100.9, updated); // the bar closes
 * ```
 */
export function smiOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInSignalPeriod: number,
  core: Core = Core.DEFAULT,
): SmiStream {
  optInTimePeriod = requireInt32('SMI open', 'optInTimePeriod', optInTimePeriod);
  optInFastPeriod = requireInt32('SMI open', 'optInFastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('SMI open', 'optInSlowPeriod', optInSlowPeriod);
  optInSignalPeriod = requireInt32('SMI open', 'optInSignalPeriod', optInSignalPeriod);
  requireArgument('SMI open', 'inHigh', inHigh);
  requireHistory('SMI open', inHigh.length);
  requireArgument('SMI open', 'inLow', inLow);
  requireArgument('SMI open', 'inClose', inClose);
  requireHistoryLength('SMI open', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('SMI open', 'inClose', inClose.length, inHigh.length);
  return smiOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
}

/**
 * {@link smiOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link smi}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price series. Every history has the same
 *   length.
 * @param inLow - Warm-up history. Low price series. Every history has the same
 *   length.
 * @param inClose - Warm-up history. Close price series. Every history has the same
 *   length.
 * @param optInTimePeriod - Period of the high/low range (default 13; range 2 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInFastPeriod - Period of the second smoothing, applied to the first
 *   (default 2; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowPeriod - Period of the first smoothing, applied to the raw
 *   momentum (default 25; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param optInSignalPeriod - Smoothing period of the signal line (default 9; range
 *   2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outSMI - Stochastic Momentum Index, -100 to +100. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param outSMISignal - Exponential average of the SMI line. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `smiLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range, histories of different lengths or an output too
 *   short or starting where another output or a `Float64Array` input starts, or, on
 *   a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
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
 * const smiOut = new Float64Array(high.length);
 * const smiSignal = new Float64Array(high.length);
 * const stream = smiOpenAndFill(
 *   high, low, close,
 *   13, 2, 25, 9,
 *   smiOut, smiSignal,
 * );
 * const { begIdx, count } = stream.outRange();
 * // smiOut[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function smiOpenAndFill(
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
  optInTimePeriod = requireInt32('SMI openAndFill', 'optInTimePeriod', optInTimePeriod);
  optInFastPeriod = requireInt32('SMI openAndFill', 'optInFastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('SMI openAndFill', 'optInSlowPeriod', optInSlowPeriod);
  optInSignalPeriod = requireInt32('SMI openAndFill', 'optInSignalPeriod', optInSignalPeriod);
  requireArgument('SMI openAndFill', 'inHigh', inHigh);
  requireHistory('SMI openAndFill', inHigh.length);
  requireArgument('SMI openAndFill', 'inLow', inLow);
  requireArgument('SMI openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('SMI openAndFill', inHigh.length, smiLookback(optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, core));
  requireHistoryLength('SMI openAndFill', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('SMI openAndFill', 'inClose', inClose.length, inHigh.length);
  requireLength('SMI openAndFill', 'outSMI', outSMI, guardOutLen);
  requireLength('SMI openAndFill', 'outSMISignal', outSMISignal, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outSMI.buffer === inHighF64.buffer && outSMI.byteOffset === inHighF64.byteOffset) || (outSMI.buffer === inLowF64.buffer && outSMI.byteOffset === inLowF64.byteOffset) || (outSMI.buffer === inCloseF64.buffer && outSMI.byteOffset === inCloseF64.byteOffset) || (outSMISignal.buffer === inHighF64.buffer && outSMISignal.byteOffset === inHighF64.byteOffset) || (outSMISignal.buffer === inLowF64.buffer && outSMISignal.byteOffset === inLowF64.byteOffset) || (outSMISignal.buffer === inCloseF64.buffer && outSMISignal.byteOffset === inCloseF64.byteOffset) || (outSMI.buffer === outSMISignal.buffer && outSMI.byteOffset === outSMISignal.byteOffset)) {
    throw streamFailure('SMI openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outSMI, inHighF64) || overlaps(outSMI, inLowF64) || overlaps(outSMI, inCloseF64) || overlaps(outSMISignal, inHighF64) || overlaps(outSMISignal, inLowF64) || overlaps(outSMISignal, inCloseF64) || overlaps(outSMI, outSMISignal))) {
    throw streamFailure('SMI openAndFill', RetCode.BAD_PARAM);
  }
  return smiOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, new MInteger(), new MInteger(), outSMI, outSMISignal);
}
