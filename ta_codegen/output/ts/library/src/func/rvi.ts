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
 *  090526 MF,CC  First version (issue #366).
 *  092226 MF,CC  #434 the three variance steps follow var.c.
 */

import { FuncUnstId } from '../enums.ts';
import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma, fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function rviLookbackPositional(optInTimePeriod: number, optInStdDevPeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if ((optInStdDevPeriod | 0) !== optInStdDevPeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  if (optInStdDevPeriod === Core.INTEGER_DEFAULT) {
    optInStdDevPeriod = 10;
  } else if (optInStdDevPeriod < 2 || optInStdDevPeriod > 100000) {
    return -1;
  }
  return optInStdDevPeriod - 1 + (optInTimePeriod - 1) + core.unstableCount(FuncUnstId.RVI, ((optInTimePeriod > 1) ? ((10 * optInTimePeriod) | 0) : 0), ((optInTimePeriod > 1) ? ((19 * optInTimePeriod) | 0) : 0));
}

/** @internal */
export function rviDisplayShiftPositional(optInTimePeriod: number, optInStdDevPeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (rviLookbackPositional(optInTimePeriod, optInStdDevPeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function rviImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  optInStdDevPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let tempReal = 0;
  let shift = 0;
  let periodTotal1 = 0;
  let periodTotal2 = 0;
  let meanValue1 = 0;
  let variance = 0;
  let peakTotal2 = 0;
  let invPeriod = 0;
  let sigma = 0;
  let delta = 0;
  let upValue = 0;
  let dnValue = 0;
  let upTotal = 0;
  let dnTotal = 0;
  let prevUp = 0;
  let prevDn = 0;
  let wAlpha = 0;
  let wBeta = 0;
  let total = 0;
  let i = 0;
  let j = 0;
  let outIdx = 0;
  let today = 0;
  let anchorIdx = 0;
  let trailingIdx = 0;
  let windowStart = 0;
  let barsSinceReseed = 0;
  let nbInitialElementNeeded = 0;
  let lookbackTotal = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInStdDevPeriod === Core.INTEGER_DEFAULT) {
    optInStdDevPeriod = 10;
  } else if (optInStdDevPeriod < 2 || optInStdDevPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = rviLookbackPositional(optInTimePeriod, optInStdDevPeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  /* wAlpha is derived FROM wBeta, never the reverse (rma.c): only that order
   * makes the pair sum to exactly 1, and TA_RMA over this function's two legs
   * has to be bit for bit what the fused step below computes.
   */
  wBeta = (optInTimePeriod - 1) / optInTimePeriod;
  wAlpha = 1.0 - wBeta;
  /* The per-bar sigma is var.c's step transcribed unchanged -- shifted running
   * sums against a near-window anchor, the reseed trigger, its floor, and the
   * re-remove under the new shift. Any algebraically equal spelling is a
   * different double, and the reference this is differenced against is
   * TA_STDDEV anchored at this call's own start.
   */
  nbInitialElementNeeded = optInStdDevPeriod - 1;
  invPeriod = 1.0 / optInStdDevPeriod;
  anchorIdx = startIdx - lookbackTotal;
  trailingIdx = anchorIdx;
  shift = inReal[anchorIdx];
  periodTotal1 = 0.0;
  periodTotal2 = 0.0;
  today = anchorIdx + nbInitialElementNeeded;
  for (j = anchorIdx; j < today; j += 1) {
    tempReal = inReal[j] - shift;
    periodTotal1 += tempReal;
    tempReal *= tempReal;
    periodTotal2 += tempReal;
  }
  barsSinceReseed = ((32 * optInStdDevPeriod) | 0);
  peakTotal2 = periodTotal2;
  /* Seed both legs with the simple average of the first 'optInTimePeriod'
   * volatilities, as rma.c seeds. optInStdDevPeriod >= 2 is what keeps the
   * inReal[today-1] below in bounds on the very first bar.
   */
  upTotal = 0.0;
  dnTotal = 0.0;
  for (i = optInTimePeriod; i > 0; i -= 1) {
    tempReal = inReal[today] - shift;
    periodTotal1 += tempReal;
    tempReal *= tempReal;
    periodTotal2 += tempReal;
    peakTotal2 = (periodTotal2 > peakTotal2) ? periodTotal2 : peakTotal2;
    meanValue1 = periodTotal1 * invPeriod;
    variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
    tempReal = inReal[trailingIdx] - shift;
    periodTotal1 -= tempReal;
    tempReal *= tempReal;
    periodTotal2 -= tempReal;
    trailingIdx += 1;
    barsSinceReseed -= 1;
    if (variance < 1e-6 * (peakTotal2 * invPeriod) || barsSinceReseed <= 0) {
      barsSinceReseed = ((32 * optInStdDevPeriod) | 0);
      windowStart = today - nbInitialElementNeeded;
      tempReal = 0.0;
      for (j = windowStart; j <= today; j += 1) {
        tempReal += inReal[j];
      }
      shift = tempReal * invPeriod;
      periodTotal1 = 0.0;
      periodTotal2 = 0.0;
      for (j = windowStart; j <= today; j += 1) {
        tempReal = inReal[j] - shift;
        periodTotal1 += tempReal;
        tempReal *= tempReal;
        periodTotal2 += tempReal;
      }
      meanValue1 = periodTotal1 * invPeriod;
      variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
      if (variance < 1e-6 * (periodTotal2 * invPeriod)) {
        shift = inReal[today];
        periodTotal1 = 0.0;
        periodTotal2 = 0.0;
        for (j = windowStart; j <= today; j += 1) {
          tempReal = inReal[j] - shift;
          periodTotal1 += tempReal;
          tempReal *= tempReal;
          periodTotal2 += tempReal;
        }
        meanValue1 = periodTotal1 * invPeriod;
        variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
      }
      peakTotal2 = periodTotal2;
      if (variance < 1e-12 * (periodTotal2 * invPeriod)) {
        variance = 0.0;
      }
      tempReal = inReal[windowStart] - shift;
      periodTotal1 -= tempReal;
      tempReal *= tempReal;
      periodTotal2 -= tempReal;
    }
    sigma = Math.sqrt(variance);
    delta = inReal[today] - inReal[today - 1];
    if (delta > 0.0) {
      upTotal += sigma;
    } else if (delta < 0.0) {
      dnTotal += sigma;
    }
    today += 1;
  }
  prevUp = upTotal / optInTimePeriod;
  prevDn = dnTotal / optInTimePeriod;
  /* Skip the unstable period. Same step, smoothed but not stored. */
  i = lookbackTotal - (optInStdDevPeriod - 1 + (optInTimePeriod - 1));
  while (i !== 0) {
    tempReal = inReal[today] - shift;
    periodTotal1 += tempReal;
    tempReal *= tempReal;
    periodTotal2 += tempReal;
    peakTotal2 = (periodTotal2 > peakTotal2) ? periodTotal2 : peakTotal2;
    meanValue1 = periodTotal1 * invPeriod;
    variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
    tempReal = inReal[trailingIdx] - shift;
    periodTotal1 -= tempReal;
    tempReal *= tempReal;
    periodTotal2 -= tempReal;
    trailingIdx += 1;
    barsSinceReseed -= 1;
    if (variance < 1e-6 * (peakTotal2 * invPeriod) || barsSinceReseed <= 0) {
      barsSinceReseed = ((32 * optInStdDevPeriod) | 0);
      windowStart = today - nbInitialElementNeeded;
      tempReal = 0.0;
      for (j = windowStart; j <= today; j += 1) {
        tempReal += inReal[j];
      }
      shift = tempReal * invPeriod;
      periodTotal1 = 0.0;
      periodTotal2 = 0.0;
      for (j = windowStart; j <= today; j += 1) {
        tempReal = inReal[j] - shift;
        periodTotal1 += tempReal;
        tempReal *= tempReal;
        periodTotal2 += tempReal;
      }
      meanValue1 = periodTotal1 * invPeriod;
      variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
      if (variance < 1e-6 * (periodTotal2 * invPeriod)) {
        shift = inReal[today];
        periodTotal1 = 0.0;
        periodTotal2 = 0.0;
        for (j = windowStart; j <= today; j += 1) {
          tempReal = inReal[j] - shift;
          periodTotal1 += tempReal;
          tempReal *= tempReal;
          periodTotal2 += tempReal;
        }
        meanValue1 = periodTotal1 * invPeriod;
        variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
      }
      peakTotal2 = periodTotal2;
      if (variance < 1e-12 * (periodTotal2 * invPeriod)) {
        variance = 0.0;
      }
      tempReal = inReal[windowStart] - shift;
      periodTotal1 -= tempReal;
      tempReal *= tempReal;
      periodTotal2 -= tempReal;
    }
    sigma = Math.sqrt(variance);
    delta = inReal[today] - inReal[today - 1];
    upValue = 0.0;
    dnValue = 0.0;
    if (delta > 0.0) {
      upValue = sigma;
    } else if (delta < 0.0) {
      dnValue = sigma;
    }
    {
      const _a: number = wBeta, _b: number = prevUp, _c: number = wAlpha * upValue;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevUp = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = wBeta, _b: number = prevDn, _c: number = wAlpha * dnValue;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevDn = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    today += 1;
    i -= 1;
  }
  /* A tie feeds neither leg, so both can be exactly zero at the same bar --
   * reachable on real data whenever the smoothing has no memory. Test the
   * total exactly: a band would carry the quote unit and zero the oscillator
   * for any instrument priced under it.
   */
  total = prevUp + prevDn;
  outReal[0] = (total === 0.0) ? 50.0 : 100.0 * (prevUp / total);
  outIdx = 1;
  while (today <= endIdx) {
    tempReal = inReal[today] - shift;
    periodTotal1 += tempReal;
    tempReal *= tempReal;
    periodTotal2 += tempReal;
    peakTotal2 = (periodTotal2 > peakTotal2) ? periodTotal2 : peakTotal2;
    meanValue1 = periodTotal1 * invPeriod;
    variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
    tempReal = inReal[trailingIdx] - shift;
    periodTotal1 -= tempReal;
    tempReal *= tempReal;
    periodTotal2 -= tempReal;
    trailingIdx += 1;
    barsSinceReseed -= 1;
    if (variance < 1e-6 * (peakTotal2 * invPeriod) || barsSinceReseed <= 0) {
      barsSinceReseed = ((32 * optInStdDevPeriod) | 0);
      windowStart = today - nbInitialElementNeeded;
      tempReal = 0.0;
      for (j = windowStart; j <= today; j += 1) {
        tempReal += inReal[j];
      }
      shift = tempReal * invPeriod;
      periodTotal1 = 0.0;
      periodTotal2 = 0.0;
      for (j = windowStart; j <= today; j += 1) {
        tempReal = inReal[j] - shift;
        periodTotal1 += tempReal;
        tempReal *= tempReal;
        periodTotal2 += tempReal;
      }
      meanValue1 = periodTotal1 * invPeriod;
      variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
      if (variance < 1e-6 * (periodTotal2 * invPeriod)) {
        shift = inReal[today];
        periodTotal1 = 0.0;
        periodTotal2 = 0.0;
        for (j = windowStart; j <= today; j += 1) {
          tempReal = inReal[j] - shift;
          periodTotal1 += tempReal;
          tempReal *= tempReal;
          periodTotal2 += tempReal;
        }
        meanValue1 = periodTotal1 * invPeriod;
        variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
      }
      peakTotal2 = periodTotal2;
      if (variance < 1e-12 * (periodTotal2 * invPeriod)) {
        variance = 0.0;
      }
      tempReal = inReal[windowStart] - shift;
      periodTotal1 -= tempReal;
      tempReal *= tempReal;
      periodTotal2 -= tempReal;
    }
    sigma = Math.sqrt(variance);
    delta = inReal[today] - inReal[today - 1];
    upValue = 0.0;
    dnValue = 0.0;
    if (delta > 0.0) {
      upValue = sigma;
    } else if (delta < 0.0) {
      dnValue = sigma;
    }
    {
      const _a: number = wBeta, _b: number = prevUp, _c: number = wAlpha * upValue;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevUp = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = wBeta, _b: number = prevDn, _c: number = wAlpha * dnValue;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevDn = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    total = prevUp + prevDn;
    outReal[outIdx++] = (total === 0.0) ? 50.0 : 100.0 * (prevUp / total);
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['stdDevPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000']] as const;

/** @internal */
export function rviPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInStdDevPeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('RVI', 'startIdx', startIdx);
  endIdx = requireInt32('RVI', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('RVI', 'timePeriod', optInTimePeriod);
  optInStdDevPeriod = requireInt32('RVI', 'stdDevPeriod', optInStdDevPeriod);
  requireIndexRange('RVI', startIdx, endIdx);
  const guardLookback = rviLookbackPositional(optInTimePeriod, optInStdDevPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('RVI', PARAM_RANGES, optInTimePeriod, optInStdDevPeriod);
  }
  const guardStart = clampedStart('RVI', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('RVI', 'real', inReal, guardInLen);
  requireOutput('RVI', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('RVI', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = rviImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, optInStdDevPeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('RVI', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link rvi}, {@link rviLookback}, {@link rviDisplayShift},
 * {@link rviOpen} and {@link rviOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface RviOptions {
  /**
   * Wilder smoothing period applied to both legs (default 14; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  timePeriod?: number | undefined;
  /**
   * Number of trailing values the standard deviation spans (default 10; range 2 to
   * 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  stdDevPeriod?: number | undefined;
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

/** What {@link rvi} returns, and {@link rviOpenAndFill} with its stream. */
export interface RviResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Relative Volatility Index value. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link rvi} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link RviOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function rviLookback(options?: RviOptions): number {
  const o = optionsOf('RVI lookback', options);
  return rviLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.stdDevPeriod, Core.INTEGER_DEFAULT), coreOf('RVI lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link rvi}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link RviOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function rviDisplayShift(outputIdx: number, options?: RviOptions): number {
  const o = optionsOf('RVI displayShift', options);
  return rviDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.stdDevPeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('RVI displayShift', o.core));
}

/**
 * Relative Volatility Index: Donald Dorsey's volatility oscillator, built exactly
 * like RSI except that the quantity routed to the up and down buckets is the
 * rolling standard deviation of price rather than the size of the move. The
 * direction of the close-to-close change still decides which bucket a bar feeds.
 * Bounded in 0..100. High values mean the recent volatility arrived mostly on up
 * bars, low values that it arrived mostly on down bars. Dorsey proposed it as a
 * confirming filter rather than a stand-alone signal: take a long entry only while
 * RVI is above 50, a short only while it is below.
 *
 * Formula and more info at
 * [ta-lib.org/functions/rvi](https://ta-lib.org/functions/rvi).
 *
 * **Notes**
 * - RVI is the 1993 version; [`RVIR`](https://ta-lib.org/functions/rvir) is the
 *   1995 revision.
 * - A tie contributes to neither bucket, matching RSI's treatment of an unchanged
 *   close. Descriptions that write the denominator as a smoothed `S` instead of
 *   `Up + Down` are counting ties as down bars, which is a different indicator.
 * - Unrelated to the Relative Vigor Index, which several platforms also abbreviate
 *   RVI.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link rviLookback} is a success with empty outputs.
 *
 * @param real - Source price/value series, canonically the close.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link RviOptions}.
 * @returns `begIdx` and the values; see {@link RviResult}. Each output is a view
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
 * import { rvi } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = rvi(data, { timePeriod: 14, stdDevPeriod: 10 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`rsi`](https://ta-lib.org/functions/rsi)
 * @see [`rma`](https://ta-lib.org/functions/rma)
 * @see [`stddev`](https://ta-lib.org/functions/stddev)
 * @see [`cmo`](https://ta-lib.org/functions/cmo)
 */
export function rvi(real: ArrayLike<number>, options?: RviOptions): RviResult {
  const o = optionsOf('RVI', options);
  if (o.endIdx === undefined) {
    requireSeries('RVI', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInStdDevPeriod = orDefault(o.stdDevPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('RVI', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, rviLookbackPositional(optInTimePeriod, optInStdDevPeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = rviPositional(startIdx, endIdx, real, optInTimePeriod, optInStdDevPeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live RVI stream: one value per closed bar, bit-identical to {@link rvi} over
 * the same series. Open it with {@link rviOpen} or {@link rviOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/rvi](https://ta-lib.org/functions/rvi).
 */
export class RviStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  optInStdDevPeriod: number = 0;
  /** @internal */
  shift: number = 0;
  /** @internal */
  periodTotal1: number = 0;
  /** @internal */
  periodTotal2: number = 0;
  /** @internal */
  peakTotal2: number = 0;
  /** @internal */
  invPeriod: number = 0;
  /** @internal */
  prevUp: number = 0;
  /** @internal */
  prevDn: number = 0;
  /** @internal */
  wAlpha: number = 0;
  /** @internal */
  wBeta: number = 0;
  /** @internal */
  trailingIdx: number = 0;
  /** @internal */
  barsSinceReseed: number = 0;
  /** @internal */
  nbInitialElementNeeded: number = 0;
  /** @internal */
  j: number = 0;
  /** @internal */
  windowStart: number = 0;
  /** @internal */
  today: number = 0;
  /** @internal */
  lag1_inReal: number = 0;
  /** @internal */
  xMask: number = 0;
  /** @internal */
  x_inReal: Float64Array = new Float64Array(1);
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
  static of(core: Core): RviStream {
    return new RviStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link rvi} reports them over the same bars. Each accepted `update` and each
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
      throw failure('RVI advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: Source price/value series, canonically
   *   the close.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('RVI update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('RVI update', 'inReal');
    }
    rviStepImpl(this, inReal);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Source price/value series, canonically
   *   the close.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number): number {
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('RVI peek', 'inReal');
    }
    const sp = this;
    let tempReal = 0;
    let meanValue1 = 0;
    let variance = 0;
    let sigma = 0;
    let delta = 0;
    let upValue = 0;
    let dnValue = 0;
    let total = 0;
    let barsSinceReseed = sp.barsSinceReseed;
    let cur_outReal = 0;
    let j = sp.j;
    let peakTotal2 = sp.peakTotal2;
    let periodTotal1 = sp.periodTotal1;
    let periodTotal2 = sp.periodTotal2;
    let prevDn = sp.prevDn;
    let prevUp = sp.prevUp;
    let shift = sp.shift;
    let trailingIdx = sp.trailingIdx;
    let windowStart = sp.windowStart;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    pkSlot0 = sp.today & sp.xMask;
    pkVal0 = inReal;
    tempReal = (((sp.today & sp.xMask) !== pkSlot0) ? sp.x_inReal[sp.today & sp.xMask] : pkVal0) - shift;
    periodTotal1 += tempReal;
    tempReal *= tempReal;
    periodTotal2 += tempReal;
    peakTotal2 = (periodTotal2 > peakTotal2) ? periodTotal2 : peakTotal2;
    meanValue1 = periodTotal1 * sp.invPeriod;
    variance = periodTotal2 * sp.invPeriod - meanValue1 * meanValue1;
    tempReal = (((trailingIdx & sp.xMask) !== pkSlot0) ? sp.x_inReal[trailingIdx & sp.xMask] : pkVal0) - shift;
    periodTotal1 -= tempReal;
    tempReal *= tempReal;
    periodTotal2 -= tempReal;
    trailingIdx += 1;
    barsSinceReseed -= 1;
    if (variance < 1e-6 * (peakTotal2 * sp.invPeriod) || barsSinceReseed <= 0) {
      barsSinceReseed = ((32 * sp.optInStdDevPeriod) | 0);
      windowStart = sp.today - sp.nbInitialElementNeeded;
      tempReal = 0.0;
      for (j = windowStart; j <= sp.today; j += 1) {
        tempReal += ((j & sp.xMask) !== pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0;
      }
      shift = tempReal * sp.invPeriod;
      periodTotal1 = 0.0;
      periodTotal2 = 0.0;
      for (j = windowStart; j <= sp.today; j += 1) {
        tempReal = (((j & sp.xMask) !== pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0) - shift;
        periodTotal1 += tempReal;
        tempReal *= tempReal;
        periodTotal2 += tempReal;
      }
      meanValue1 = periodTotal1 * sp.invPeriod;
      variance = periodTotal2 * sp.invPeriod - meanValue1 * meanValue1;
      if (variance < 1e-6 * (periodTotal2 * sp.invPeriod)) {
        shift = ((sp.today & sp.xMask) !== pkSlot0) ? sp.x_inReal[sp.today & sp.xMask] : pkVal0;
        periodTotal1 = 0.0;
        periodTotal2 = 0.0;
        for (j = windowStart; j <= sp.today; j += 1) {
          tempReal = (((j & sp.xMask) !== pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0) - shift;
          periodTotal1 += tempReal;
          tempReal *= tempReal;
          periodTotal2 += tempReal;
        }
        meanValue1 = periodTotal1 * sp.invPeriod;
        variance = periodTotal2 * sp.invPeriod - meanValue1 * meanValue1;
      }
      peakTotal2 = periodTotal2;
      if (variance < 1e-12 * (periodTotal2 * sp.invPeriod)) {
        variance = 0.0;
      }
      tempReal = (((windowStart & sp.xMask) !== pkSlot0) ? sp.x_inReal[windowStart & sp.xMask] : pkVal0) - shift;
      periodTotal1 -= tempReal;
      tempReal *= tempReal;
      periodTotal2 -= tempReal;
    }
    sigma = Math.sqrt(variance);
    delta = (((sp.today & sp.xMask) !== pkSlot0) ? sp.x_inReal[sp.today & sp.xMask] : pkVal0) - ((((sp.today - 1) & sp.xMask) !== pkSlot0) ? sp.x_inReal[(sp.today - 1) & sp.xMask] : pkVal0);
    upValue = 0.0;
    dnValue = 0.0;
    if (delta > 0.0) {
      upValue = sigma;
    } else if (delta < 0.0) {
      dnValue = sigma;
    }
    prevUp = fma(sp.wBeta, prevUp, sp.wAlpha * upValue);
    prevDn = fma(sp.wBeta, prevDn, sp.wAlpha * dnValue);
    total = prevUp + prevDn;
    cur_outReal = (total === 0.0) ? 50.0 : 100.0 * (prevUp / total);
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
  clone(): RviStream {
    const c = new RviStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.optInStdDevPeriod = this.optInStdDevPeriod;
    c.shift = this.shift;
    c.periodTotal1 = this.periodTotal1;
    c.periodTotal2 = this.periodTotal2;
    c.peakTotal2 = this.peakTotal2;
    c.invPeriod = this.invPeriod;
    c.prevUp = this.prevUp;
    c.prevDn = this.prevDn;
    c.wAlpha = this.wAlpha;
    c.wBeta = this.wBeta;
    c.trailingIdx = this.trailingIdx;
    c.barsSinceReseed = this.barsSinceReseed;
    c.nbInitialElementNeeded = this.nbInitialElementNeeded;
    c.j = this.j;
    c.windowStart = this.windowStart;
    c.today = this.today;
    c.lag1_inReal = this.lag1_inReal;
    c.xMask = this.xMask;
    c.x_inReal = this.x_inReal.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function rviStepImpl(sp: RviStream, inReal: number): void {
  let tempReal = 0;
  let meanValue1 = 0;
  let variance = 0;
  let sigma = 0;
  let delta = 0;
  let upValue = 0;
  let dnValue = 0;
  let total = 0;
  sp.x_inReal[sp.today & sp.xMask] = inReal;
  tempReal = sp.x_inReal[sp.today & sp.xMask] - sp.shift;
  sp.periodTotal1 += tempReal;
  tempReal *= tempReal;
  sp.periodTotal2 += tempReal;
  sp.peakTotal2 = (sp.periodTotal2 > sp.peakTotal2) ? sp.periodTotal2 : sp.peakTotal2;
  meanValue1 = sp.periodTotal1 * sp.invPeriod;
  variance = sp.periodTotal2 * sp.invPeriod - meanValue1 * meanValue1;
  tempReal = sp.x_inReal[sp.trailingIdx & sp.xMask] - sp.shift;
  sp.periodTotal1 -= tempReal;
  tempReal *= tempReal;
  sp.periodTotal2 -= tempReal;
  sp.trailingIdx += 1;
  sp.barsSinceReseed -= 1;
  if (variance < 1e-6 * (sp.peakTotal2 * sp.invPeriod) || sp.barsSinceReseed <= 0) {
    sp.barsSinceReseed = ((32 * sp.optInStdDevPeriod) | 0);
    sp.windowStart = sp.today - sp.nbInitialElementNeeded;
    tempReal = 0.0;
    for (sp.j = sp.windowStart; sp.j <= sp.today; sp.j += 1) {
      tempReal += sp.x_inReal[sp.j & sp.xMask];
    }
    sp.shift = tempReal * sp.invPeriod;
    sp.periodTotal1 = 0.0;
    sp.periodTotal2 = 0.0;
    for (sp.j = sp.windowStart; sp.j <= sp.today; sp.j += 1) {
      tempReal = sp.x_inReal[sp.j & sp.xMask] - sp.shift;
      sp.periodTotal1 += tempReal;
      tempReal *= tempReal;
      sp.periodTotal2 += tempReal;
    }
    meanValue1 = sp.periodTotal1 * sp.invPeriod;
    variance = sp.periodTotal2 * sp.invPeriod - meanValue1 * meanValue1;
    if (variance < 1e-6 * (sp.periodTotal2 * sp.invPeriod)) {
      sp.shift = sp.x_inReal[sp.today & sp.xMask];
      sp.periodTotal1 = 0.0;
      sp.periodTotal2 = 0.0;
      for (sp.j = sp.windowStart; sp.j <= sp.today; sp.j += 1) {
        tempReal = sp.x_inReal[sp.j & sp.xMask] - sp.shift;
        sp.periodTotal1 += tempReal;
        tempReal *= tempReal;
        sp.periodTotal2 += tempReal;
      }
      meanValue1 = sp.periodTotal1 * sp.invPeriod;
      variance = sp.periodTotal2 * sp.invPeriod - meanValue1 * meanValue1;
    }
    sp.peakTotal2 = sp.periodTotal2;
    if (variance < 1e-12 * (sp.periodTotal2 * sp.invPeriod)) {
      variance = 0.0;
    }
    tempReal = sp.x_inReal[sp.windowStart & sp.xMask] - sp.shift;
    sp.periodTotal1 -= tempReal;
    tempReal *= tempReal;
    sp.periodTotal2 -= tempReal;
  }
  sigma = Math.sqrt(variance);
  delta = sp.x_inReal[sp.today & sp.xMask] - sp.x_inReal[(sp.today - 1) & sp.xMask];
  upValue = 0.0;
  dnValue = 0.0;
  if (delta > 0.0) {
    upValue = sigma;
  } else if (delta < 0.0) {
    dnValue = sigma;
  }
  sp.prevUp = fma(sp.wBeta, sp.prevUp, sp.wAlpha * upValue);
  sp.prevDn = fma(sp.wBeta, sp.prevDn, sp.wAlpha * dnValue);
  total = sp.prevUp + sp.prevDn;
  sp.cur_outReal = (total === 0.0) ? 50.0 : 100.0 * (sp.prevUp / total);
  sp.today += 1;
  sp.lag1_inReal = inReal;
}

function rviOpenImpl(
  sp: RviStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInStdDevPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let tempReal = 0;
  let shift = 0;
  let periodTotal1 = 0;
  let periodTotal2 = 0;
  let meanValue1 = 0;
  let variance = 0;
  let peakTotal2 = 0;
  let invPeriod = 0;
  let sigma = 0;
  let delta = 0;
  let upValue = 0;
  let dnValue = 0;
  let upTotal = 0;
  let dnTotal = 0;
  let prevUp = 0;
  let prevDn = 0;
  let wAlpha = 0;
  let wBeta = 0;
  let total = 0;
  let i = 0;
  let j = 0;
  let outIdx = 0;
  let today = 0;
  let anchorIdx = 0;
  let trailingIdx = 0;
  let windowStart = 0;
  let barsSinceReseed = 0;
  let nbInitialElementNeeded = 0;
  let lookbackTotal = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInStdDevPeriod === Core.INTEGER_DEFAULT) {
    optInStdDevPeriod = 10;
  } else if (optInStdDevPeriod < 2 || optInStdDevPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = rviLookbackPositional(optInTimePeriod, optInStdDevPeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* wAlpha is derived FROM wBeta, never the reverse (rma.c): only that order
   * makes the pair sum to exactly 1, and TA_RMA over this function's two legs
   * has to be bit for bit what the fused step below computes.
   */
  wBeta = (optInTimePeriod - 1) / optInTimePeriod;
  wAlpha = 1.0 - wBeta;
  /* The per-bar sigma is var.c's step transcribed unchanged -- shifted running
   * sums against a near-window anchor, the reseed trigger, its floor, and the
   * re-remove under the new shift. Any algebraically equal spelling is a
   * different double, and the reference this is differenced against is
   * TA_STDDEV anchored at this call's own start.
   */
  nbInitialElementNeeded = optInStdDevPeriod - 1;
  invPeriod = 1.0 / optInStdDevPeriod;
  anchorIdx = startIdx - lookbackTotal;
  trailingIdx = anchorIdx;
  shift = inReal[anchorIdx];
  periodTotal1 = 0.0;
  periodTotal2 = 0.0;
  today = anchorIdx + nbInitialElementNeeded;
  for (j = anchorIdx; j < today; j += 1) {
    tempReal = inReal[j] - shift;
    periodTotal1 += tempReal;
    tempReal *= tempReal;
    periodTotal2 += tempReal;
  }
  barsSinceReseed = ((32 * optInStdDevPeriod) | 0);
  peakTotal2 = periodTotal2;
  /* Seed both legs with the simple average of the first 'optInTimePeriod'
   * volatilities, as rma.c seeds. optInStdDevPeriod >= 2 is what keeps the
   * inReal[today-1] below in bounds on the very first bar.
   */
  upTotal = 0.0;
  dnTotal = 0.0;
  for (i = optInTimePeriod; i > 0; i -= 1) {
    tempReal = inReal[today] - shift;
    periodTotal1 += tempReal;
    tempReal *= tempReal;
    periodTotal2 += tempReal;
    peakTotal2 = (periodTotal2 > peakTotal2) ? periodTotal2 : peakTotal2;
    meanValue1 = periodTotal1 * invPeriod;
    variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
    tempReal = inReal[trailingIdx] - shift;
    periodTotal1 -= tempReal;
    tempReal *= tempReal;
    periodTotal2 -= tempReal;
    trailingIdx += 1;
    barsSinceReseed -= 1;
    if (variance < 1e-6 * (peakTotal2 * invPeriod) || barsSinceReseed <= 0) {
      barsSinceReseed = ((32 * optInStdDevPeriod) | 0);
      windowStart = today - nbInitialElementNeeded;
      tempReal = 0.0;
      for (j = windowStart; j <= today; j += 1) {
        tempReal += inReal[j];
      }
      shift = tempReal * invPeriod;
      periodTotal1 = 0.0;
      periodTotal2 = 0.0;
      for (j = windowStart; j <= today; j += 1) {
        tempReal = inReal[j] - shift;
        periodTotal1 += tempReal;
        tempReal *= tempReal;
        periodTotal2 += tempReal;
      }
      meanValue1 = periodTotal1 * invPeriod;
      variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
      if (variance < 1e-6 * (periodTotal2 * invPeriod)) {
        shift = inReal[today];
        periodTotal1 = 0.0;
        periodTotal2 = 0.0;
        for (j = windowStart; j <= today; j += 1) {
          tempReal = inReal[j] - shift;
          periodTotal1 += tempReal;
          tempReal *= tempReal;
          periodTotal2 += tempReal;
        }
        meanValue1 = periodTotal1 * invPeriod;
        variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
      }
      peakTotal2 = periodTotal2;
      if (variance < 1e-12 * (periodTotal2 * invPeriod)) {
        variance = 0.0;
      }
      tempReal = inReal[windowStart] - shift;
      periodTotal1 -= tempReal;
      tempReal *= tempReal;
      periodTotal2 -= tempReal;
    }
    sigma = Math.sqrt(variance);
    delta = inReal[today] - inReal[today - 1];
    if (delta > 0.0) {
      upTotal += sigma;
    } else if (delta < 0.0) {
      dnTotal += sigma;
    }
    today += 1;
  }
  prevUp = upTotal / optInTimePeriod;
  prevDn = dnTotal / optInTimePeriod;
  /* Skip the unstable period. Same step, smoothed but not stored. */
  i = lookbackTotal - (optInStdDevPeriod - 1 + (optInTimePeriod - 1));
  while (i !== 0) {
    tempReal = inReal[today] - shift;
    periodTotal1 += tempReal;
    tempReal *= tempReal;
    periodTotal2 += tempReal;
    peakTotal2 = (periodTotal2 > peakTotal2) ? periodTotal2 : peakTotal2;
    meanValue1 = periodTotal1 * invPeriod;
    variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
    tempReal = inReal[trailingIdx] - shift;
    periodTotal1 -= tempReal;
    tempReal *= tempReal;
    periodTotal2 -= tempReal;
    trailingIdx += 1;
    barsSinceReseed -= 1;
    if (variance < 1e-6 * (peakTotal2 * invPeriod) || barsSinceReseed <= 0) {
      barsSinceReseed = ((32 * optInStdDevPeriod) | 0);
      windowStart = today - nbInitialElementNeeded;
      tempReal = 0.0;
      for (j = windowStart; j <= today; j += 1) {
        tempReal += inReal[j];
      }
      shift = tempReal * invPeriod;
      periodTotal1 = 0.0;
      periodTotal2 = 0.0;
      for (j = windowStart; j <= today; j += 1) {
        tempReal = inReal[j] - shift;
        periodTotal1 += tempReal;
        tempReal *= tempReal;
        periodTotal2 += tempReal;
      }
      meanValue1 = periodTotal1 * invPeriod;
      variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
      if (variance < 1e-6 * (periodTotal2 * invPeriod)) {
        shift = inReal[today];
        periodTotal1 = 0.0;
        periodTotal2 = 0.0;
        for (j = windowStart; j <= today; j += 1) {
          tempReal = inReal[j] - shift;
          periodTotal1 += tempReal;
          tempReal *= tempReal;
          periodTotal2 += tempReal;
        }
        meanValue1 = periodTotal1 * invPeriod;
        variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
      }
      peakTotal2 = periodTotal2;
      if (variance < 1e-12 * (periodTotal2 * invPeriod)) {
        variance = 0.0;
      }
      tempReal = inReal[windowStart] - shift;
      periodTotal1 -= tempReal;
      tempReal *= tempReal;
      periodTotal2 -= tempReal;
    }
    sigma = Math.sqrt(variance);
    delta = inReal[today] - inReal[today - 1];
    upValue = 0.0;
    dnValue = 0.0;
    if (delta > 0.0) {
      upValue = sigma;
    } else if (delta < 0.0) {
      dnValue = sigma;
    }
    {
      const _a: number = wBeta, _b: number = prevUp, _c: number = wAlpha * upValue;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevUp = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = wBeta, _b: number = prevDn, _c: number = wAlpha * dnValue;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevDn = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    today += 1;
    i -= 1;
  }
  /* A tie feeds neither leg, so both can be exactly zero at the same bar --
   * reachable on real data whenever the smoothing has no memory. Test the
   * total exactly: a band would carry the quote unit and zero the oscillator
   * for any instrument priced under it.
   */
  total = prevUp + prevDn;
  outReal[((0 * outStride) | 0)] = (total === 0.0) ? 50.0 : 100.0 * (prevUp / total);
  outIdx = 1;
  while (today <= endIdx) {
    tempReal = inReal[today] - shift;
    periodTotal1 += tempReal;
    tempReal *= tempReal;
    periodTotal2 += tempReal;
    peakTotal2 = (periodTotal2 > peakTotal2) ? periodTotal2 : peakTotal2;
    meanValue1 = periodTotal1 * invPeriod;
    variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
    tempReal = inReal[trailingIdx] - shift;
    periodTotal1 -= tempReal;
    tempReal *= tempReal;
    periodTotal2 -= tempReal;
    trailingIdx += 1;
    barsSinceReseed -= 1;
    if (variance < 1e-6 * (peakTotal2 * invPeriod) || barsSinceReseed <= 0) {
      barsSinceReseed = ((32 * optInStdDevPeriod) | 0);
      windowStart = today - nbInitialElementNeeded;
      tempReal = 0.0;
      for (j = windowStart; j <= today; j += 1) {
        tempReal += inReal[j];
      }
      shift = tempReal * invPeriod;
      periodTotal1 = 0.0;
      periodTotal2 = 0.0;
      for (j = windowStart; j <= today; j += 1) {
        tempReal = inReal[j] - shift;
        periodTotal1 += tempReal;
        tempReal *= tempReal;
        periodTotal2 += tempReal;
      }
      meanValue1 = periodTotal1 * invPeriod;
      variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
      if (variance < 1e-6 * (periodTotal2 * invPeriod)) {
        shift = inReal[today];
        periodTotal1 = 0.0;
        periodTotal2 = 0.0;
        for (j = windowStart; j <= today; j += 1) {
          tempReal = inReal[j] - shift;
          periodTotal1 += tempReal;
          tempReal *= tempReal;
          periodTotal2 += tempReal;
        }
        meanValue1 = periodTotal1 * invPeriod;
        variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
      }
      peakTotal2 = periodTotal2;
      if (variance < 1e-12 * (periodTotal2 * invPeriod)) {
        variance = 0.0;
      }
      tempReal = inReal[windowStart] - shift;
      periodTotal1 -= tempReal;
      tempReal *= tempReal;
      periodTotal2 -= tempReal;
    }
    sigma = Math.sqrt(variance);
    delta = inReal[today] - inReal[today - 1];
    upValue = 0.0;
    dnValue = 0.0;
    if (delta > 0.0) {
      upValue = sigma;
    } else if (delta < 0.0) {
      dnValue = sigma;
    }
    {
      const _a: number = wBeta, _b: number = prevUp, _c: number = wAlpha * upValue;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevUp = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = wBeta, _b: number = prevDn, _c: number = wAlpha * dnValue;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevDn = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    total = prevUp + prevDn;
    outReal[((outIdx++ * outStride) | 0)] = (total === 0.0) ? 50.0 : 100.0 * (prevUp / total);
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  const capX = today - trailingIdx + 1;
  if (capX < 1 || capX > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  let physX = 1;
  while (physX < capX) {
    physX <<= 1;
  }
  const capX_inReal = new Float64Array(physX);
  for (let fillJ = historyLen - capX; fillJ < historyLen; fillJ++) {
    capX_inReal[fillJ & (physX - 1)] = inReal[fillJ];
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.optInStdDevPeriod = optInStdDevPeriod;
  sp.shift = shift;
  sp.periodTotal1 = periodTotal1;
  sp.periodTotal2 = periodTotal2;
  sp.peakTotal2 = peakTotal2;
  sp.invPeriod = invPeriod;
  sp.prevUp = prevUp;
  sp.prevDn = prevDn;
  sp.wAlpha = wAlpha;
  sp.wBeta = wBeta;
  sp.trailingIdx = trailingIdx;
  sp.barsSinceReseed = barsSinceReseed;
  sp.nbInitialElementNeeded = nbInitialElementNeeded;
  sp.j = j;
  sp.windowStart = windowStart;
  sp.today = today;
  sp.lag1_inReal = inReal[historyLen - 1];
  sp.xMask = physX - 1;
  sp.x_inReal = capX_inReal;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function rviOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInStdDevPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RviStream {
  const sp = RviStream.of(core);
  const retCode = rviOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInStdDevPeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('RVI openAndFill', inReal.length, startIdx, rviLookbackPositional(optInTimePeriod, optInStdDevPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && rviLookbackPositional(optInTimePeriod, optInStdDevPeriod, core) < 0) {
    throw rejectedParams('RVI openAndFill', PARAM_RANGES, optInTimePeriod, optInStdDevPeriod);
  }
  throw streamFailure('RVI openAndFill', retCode);
}

/** @internal */
export function rviOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInStdDevPeriod: number,
): RviStream {
  const sp = RviStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = rviOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInStdDevPeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('RVI open', inReal.length, startIdx, rviLookbackPositional(optInTimePeriod, optInStdDevPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && rviLookbackPositional(optInTimePeriod, optInStdDevPeriod, core) < 0) {
    throw rejectedParams('RVI open', PARAM_RANGES, optInTimePeriod, optInStdDevPeriod);
  }
  throw streamFailure('RVI open', retCode);
}

/** @internal */
export function rviOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInStdDevPeriod: number,
  core: Core = Core.DEFAULT,
): RviStream {
  optInTimePeriod = requireInt32('RVI open', 'timePeriod', optInTimePeriod);
  optInStdDevPeriod = requireInt32('RVI open', 'stdDevPeriod', optInStdDevPeriod);
  requireArgument('RVI open', 'real', inReal);
  requireHistory('RVI open', inReal.length);
  return rviOpenInternal(core, float64(inReal), 0, optInTimePeriod, optInStdDevPeriod);
}

/** @internal */
export function rviOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInStdDevPeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): RviStream {
  optInTimePeriod = requireInt32('RVI openAndFill', 'timePeriod', optInTimePeriod);
  optInStdDevPeriod = requireInt32('RVI openAndFill', 'stdDevPeriod', optInStdDevPeriod);
  requireArgument('RVI openAndFill', 'real', inReal);
  requireHistory('RVI openAndFill', inReal.length);
  const guardLookback = rviLookbackPositional(optInTimePeriod, optInStdDevPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('RVI openAndFill', PARAM_RANGES, optInTimePeriod, optInStdDevPeriod);
  }
  const guardOutLen = openFillCount('RVI openAndFill', inReal.length, guardLookback);
  requireOutput('RVI openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('RVI openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('RVI openAndFill', 'outReal', 'real');
  }
  return rviOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, optInStdDevPeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live RVI stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link rvi} there.
 *
 * @param real - Warm-up history. Source price/value series, canonically the close.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link RviOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `rviLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { rviOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = rviOpen(data, { timePeriod: 14, stdDevPeriod: 10 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function rviOpen(real: ArrayLike<number>, options?: RviOptions): RviStream {
  const o = optionsOf('RVI open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInStdDevPeriod = orDefault(o.stdDevPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('RVI open', o.core);
  return rviOpenPositional(real, optInTimePeriod, optInStdDevPeriod, core);
}

/**
 * {@link rviOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link rvi}.
 *
 * @param real - Warm-up history. Source price/value series, canonically the close.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link RviOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link rvi} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `rviLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or a
 *   buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { rviOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = rviOpenAndFill(data, {
 *   timePeriod: 14,
 *   stdDevPeriod: 10,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function rviOpenAndFill(real: ArrayLike<number>, options?: RviOptions): RviResult & { stream: RviStream } {
  const o = optionsOf('RVI openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInStdDevPeriod = orDefault(o.stdDevPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('RVI openAndFill', o.core);
  const capacity = fillCapacity(rviLookbackPositional(optInTimePeriod, optInStdDevPeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = rviOpenAndFillPositional(real, optInTimePeriod, optInStdDevPeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
