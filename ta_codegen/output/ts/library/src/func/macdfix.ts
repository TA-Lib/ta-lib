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
 *  112400 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  071126 MF,CC  Inline the fixed-26/12 MACD lockstep pass (was a
 *                delegation to macd(...,0,0,...)); bit-exact, streamable.
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
export function macdfixLookbackPositional(optInSignalPeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInSignalPeriod | 0) !== optInSignalPeriod) {
    return -1;
  }
  if (optInSignalPeriod === Core.INTEGER_DEFAULT) {
    optInSignalPeriod = 9;
  } else if (optInSignalPeriod < 1 || optInSignalPeriod > 100000) {
    return -1;
  }
  /* The lookback is driven by the signal line output.
   *
   * (must also account for the initial data consume
   *  by the fix 26 period EMA).
   */
  return emaLookbackPositional(26, core) + emaLookbackPositional(optInSignalPeriod, core);
}

/** @internal */
export function macdfixDisplayShiftPositional(optInSignalPeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (macdfixLookbackPositional(optInSignalPeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 3) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function macdfixImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
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
  let lookbackTotal = 0;
  let lookbackSignal = 0;
  let optInFastPeriod = 0;
  let optInSlowPeriod = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInSignalPeriod === Core.INTEGER_DEFAULT) {
    optInSignalPeriod = 9;
  } else if (optInSignalPeriod < 1 || optInSignalPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if ((outMACD.buffer === outMACDSignal.buffer && outMACD.byteOffset === outMACDSignal.byteOffset) || (outMACD.buffer === outMACDHist.buffer && outMACD.byteOffset === outMACDHist.byteOffset) || (outMACDSignal.buffer === outMACDHist.buffer && outMACDSignal.byteOffset === outMACDHist.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  optInFastPeriod = 12;
  optInSlowPeriod = 26;
  /* MACDFIX is the fixed 26/12 MACD: the fast/slow periods and their
   * smoothing factors are hardcoded (the general MACD selects these
   * exact values when its fast/slow period arguments are 0). Only the
   * signal period is caller-provided.
   *    Fix 12 -> fastK = 0.15
   *    Fix 26 -> slowK = 0.075
   */
  fastK = 0.15;
  fastBeta = 1.0 - fastK;
  fastK = 1.0 - fastBeta;
  slowK = 0.075;
  slowBeta = 1.0 - slowK;
  slowK = 1.0 - slowBeta;
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
  lookbackTotal = lookbackSignal;
  lookbackTotal += emaLookbackPositional(26, core);
  /* fixed slow period */
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
   *    the period. The fast and slow seed windows end on the
   *    same bar. The signal EMA is seeded the same way from the
   *    first 'signal period' MACD-line values.
   *
   * In-place (an output == inReal) is supported: outputs at
   * [outIdx] are written only after inReal[startIdx+outIdx] was
   * read.
   */
  /* Seed each price EMA with a simple average of its first
   * 'period' price bars. The fast window is the tail of the
   * slow window: consume the leading slow-only bars first,
   * then accumulate both over the shared bars.
   */
  today = startIdx - lookbackTotal;
  tempReal = 0.0;
  i = optInSlowPeriod - optInFastPeriod;
  while (i-- > 0) {
    tempReal += inReal[today++];
  }
  prevFast = 0.0;
  i = optInFastPeriod;
  while (i-- > 0) {
    prevFast += inReal[today];
    tempReal += inReal[today++];
  }
  prevSlow = tempReal / optInSlowPeriod;
  prevFast = prevFast / optInFastPeriod;
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

const PARAM_RANGES = [['signalPeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000']] as const;

/** @internal */
export function macdfixPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInSignalPeriod: number,
  outMACD: Float64Array,
  outMACDSignal: Float64Array,
  outMACDHist: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('MACDFIX', 'startIdx', startIdx);
  endIdx = requireInt32('MACDFIX', 'endIdx', endIdx);
  optInSignalPeriod = requireInt32('MACDFIX', 'signalPeriod', optInSignalPeriod);
  requireIndexRange('MACDFIX', startIdx, endIdx);
  const guardLookback = macdfixLookbackPositional(optInSignalPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('MACDFIX', PARAM_RANGES, optInSignalPeriod);
  }
  const guardStart = clampedStart('MACDFIX', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('MACDFIX', 'real', inReal, guardInLen);
  requireOutput('MACDFIX', 'outMacd', outMACD, guardOutLen, Float64Array);
  requireOutput('MACDFIX', 'outMacdSignal', outMACDSignal, guardOutLen, Float64Array);
  requireOutput('MACDFIX', 'outMacdHist', outMACDHist, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outMACD, inRealF64)) {
    throw sharedMemory('MACDFIX', 'outMacd', 'real');
  }
  if (overlapsPartly(outMACDSignal, inRealF64)) {
    throw sharedMemory('MACDFIX', 'outMacdSignal', 'real');
  }
  if (overlapsPartly(outMACDHist, inRealF64)) {
    throw sharedMemory('MACDFIX', 'outMacdHist', 'real');
  }
  if (overlaps(outMACD, outMACDSignal)) {
    throw sharedMemory('MACDFIX', 'outMacd', 'outMacdSignal');
  }
  if (overlaps(outMACD, outMACDHist)) {
    throw sharedMemory('MACDFIX', 'outMacd', 'outMacdHist');
  }
  if (overlaps(outMACDSignal, outMACDHist)) {
    throw sharedMemory('MACDFIX', 'outMacdSignal', 'outMacdHist');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = macdfixImpl(core, startIdx, endIdx, inRealF64, optInSignalPeriod, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('MACDFIX', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link macdfix}, {@link macdfixLookback},
 * {@link macdfixDisplayShift}, {@link macdfixOpen} and {@link macdfixOpenAndFill}.
 * Every key is optional: an omitted or `undefined` key takes its default, and a
 * call ignores the keys it has no use for.
 */
export interface MacdfixOptions {
  /**
   * Smoothing period for the signal line (default 9; range 1 to 100000;
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

/** What {@link macdfix} returns, and {@link macdfixOpenAndFill} with its stream. */
export interface MacdfixResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /** Fixed EMA12 minus EMA26. `outMacd[k]` is the value at input bar `begIdx + k`. */
  outMacd: Float64Array;
  /** EMA of the MACD line. `outMacdSignal[k]` is the value at input bar `begIdx + k`. */
  outMacdSignal: Float64Array;
  /** MACD minus signal. `outMacdHist[k]` is the value at input bar `begIdx + k`. */
  outMacdHist: Float64Array;
}

/**
 * Number of leading input bars {@link macdfix} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MacdfixOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function macdfixLookback(options?: MacdfixOptions): number {
  const o = optionsOf('MACDFIX lookback', options);
  return macdfixLookbackPositional(orDefault(o.signalPeriod, Core.INTEGER_DEFAULT), coreOf('MACDFIX lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link macdfix}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MacdfixOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function macdfixDisplayShift(outputIdx: number, options?: MacdfixOptions): number {
  const o = optionsOf('MACDFIX displayShift', options);
  return macdfixDisplayShiftPositional(orDefault(o.signalPeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('MACDFIX displayShift', o.core));
}

/**
 * MACD with the fast/slow EMAs fixed to the classic 12/26 periods (with the
 * classic fixed smoothing factors 0.15 and 0.075), exposing only the signal
 * period. Signal-line crossovers and histogram sign flag momentum shifts.
 *
 * Formula and more info at
 * [ta-lib.org/functions/macdfix](https://ta-lib.org/functions/macdfix).
 *
 * **Notes**
 * - A signal period of 1 disables signal-line smoothing: the signal equals the
 *   MACD line and the histogram is zero. Before 0.6.5 this parameter value
 *   produced misaligned output (issues #48/#59).
 *
 * `outMacd[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link macdfixLookback} is a success with empty outputs.
 *
 * @param real - Source series (typically close)
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link MacdfixOptions}.
 * @returns `begIdx` and the values; see {@link MacdfixResult}. Each output is a
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
 * import { macdfix } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const {
 *   begIdx,
 *   outMacd,
 *   outMacdSignal,
 *   outMacdHist,
 * } = macdfix(data, { signalPeriod: 9 });
 * // outMacd[k] is the value at bar begIdx + k
 * ```
 * @see [`macd`](https://ta-lib.org/functions/macd)
 * @see [`macdext`](https://ta-lib.org/functions/macdext)
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`apo`](https://ta-lib.org/functions/apo)
 */
export function macdfix(real: ArrayLike<number>, options?: MacdfixOptions): MacdfixResult {
  const o = optionsOf('MACDFIX', options);
  if (o.endIdx === undefined) {
    requireSeries('MACDFIX', 'real', real);
  }
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MACDFIX', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, macdfixLookbackPositional(optInSignalPeriod, core), real);
  const outMACD = o.outMacd === undefined ? new Float64Array(capacity) : o.outMacd;
  const outMACDSignal = o.outMacdSignal === undefined ? new Float64Array(capacity) : o.outMacdSignal;
  const outMACDHist = o.outMacdHist === undefined ? new Float64Array(capacity) : o.outMacdHist;
  const range = macdfixPositional(startIdx, endIdx, real, optInSignalPeriod, outMACD, outMACDSignal, outMACDHist, core);
  return { begIdx: range.begIdx, outMacd: outMACD.subarray(0, range.count), outMacdSignal: outMACDSignal.subarray(0, range.count), outMacdHist: outMACDHist.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live MACDFIX stream: one value per closed bar, bit-identical to
 * {@link macdfix} over the same series. Open it with {@link macdfixOpen} or
 * {@link macdfixOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/macdfix](https://ta-lib.org/functions/macdfix).
 */
export class MacdfixStream {
  /** @internal */
  core: Core;
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
  static of(core: Core): MacdfixStream {
    return new MacdfixStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link macdfix} reports them over the same bars. Each accepted `update` and each
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
      throw failure('MACDFIX advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns its outputs.
   *
   * @param inReal - The bar's `inReal` value: Source series (typically close)
   * @param out - Receives the outputs and is returned; a new {@link MacdfixOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number, out?: MacdfixOut): MacdfixOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('MACDFIX update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('MACDFIX update', 'MacdfixOut');
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('MACDFIX update', 'inReal');
    }
    macdfixStepImpl(this, inReal);
    this.outRangeCount++;
    if (out === undefined) {
      out = new MacdfixOut();
    }
    out.outMacd = this.cur_outMACD;
    out.outMacdSignal = this.cur_outMACDSignal;
    out.outMacdHist = this.cur_outMACDHist;
    return out;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Source series (typically close)
   * @param out - Receives the outputs and is returned; a new {@link MacdfixOut} when
   *   omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number, out?: MacdfixOut): MacdfixOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('MACDFIX peek', 'MacdfixOut');
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('MACDFIX peek', 'inReal');
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
      out = new MacdfixOut();
    }
    out.outMacd = cur_outMACD;
    out.outMacdSignal = cur_outMACDSignal;
    out.outMacdHist = cur_outMACDHist;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link MacdfixOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   */
  value(out?: MacdfixOut): MacdfixOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('MACDFIX value', 'MacdfixOut');
    }
    if (out === undefined) {
      out = new MacdfixOut();
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
  clone(): MacdfixStream {
    const c = new MacdfixStream(this.core);
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
 * The outputs of one MACDFIX bar, as `update`, `peek` and `value` return them.
 * Passed back to one of them, it is filled and returned again, with no allocation;
 * it holds its values until the next call that writes it. A new one holds `NaN`.
 */
export class MacdfixOut {
  /** Fixed EMA12 minus EMA26. */
  outMacd: number = NaN;
  /** EMA of the MACD line. */
  outMacdSignal: number = NaN;
  /** MACD minus signal. */
  outMacdHist: number = NaN;
}

function macdfixStepImpl(sp: MacdfixStream, inReal: number): void {
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

function macdfixOpenImpl(
  sp: MacdfixStream,
  inReal: Float64Array,
  startIdx: number,
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
  let lookbackTotal = 0;
  let lookbackSignal = 0;
  let optInFastPeriod = 0;
  let optInSlowPeriod = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
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
  optInFastPeriod = 12;
  optInSlowPeriod = 26;
  /* MACDFIX is the fixed 26/12 MACD: the fast/slow periods and their
   * smoothing factors are hardcoded (the general MACD selects these
   * exact values when its fast/slow period arguments are 0). Only the
   * signal period is caller-provided.
   *    Fix 12 -> fastK = 0.15
   *    Fix 26 -> slowK = 0.075
   */
  fastK = 0.15;
  fastBeta = 1.0 - fastK;
  fastK = 1.0 - fastBeta;
  slowK = 0.075;
  slowBeta = 1.0 - slowK;
  slowK = 1.0 - slowBeta;
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
  lookbackTotal = lookbackSignal;
  lookbackTotal += emaLookbackPositional(26, core);
  /* fixed slow period */
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
   *    the period. The fast and slow seed windows end on the
   *    same bar. The signal EMA is seeded the same way from the
   *    first 'signal period' MACD-line values.
   *
   * In-place (an output == inReal) is supported: outputs at
   * [outIdx] are written only after inReal[startIdx+outIdx] was
   * read.
   */
  /* Seed each price EMA with a simple average of its first
   * 'period' price bars. The fast window is the tail of the
   * slow window: consume the leading slow-only bars first,
   * then accumulate both over the shared bars.
   */
  today = startIdx - lookbackTotal;
  tempReal = 0.0;
  i = optInSlowPeriod - optInFastPeriod;
  while (i-- > 0) {
    tempReal += inReal[today++];
  }
  prevFast = 0.0;
  i = optInFastPeriod;
  while (i-- > 0) {
    prevFast += inReal[today];
    tempReal += inReal[today++];
  }
  prevSlow = tempReal / optInSlowPeriod;
  prevFast = prevFast / optInFastPeriod;
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
export function macdfixOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInSignalPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outMACD: Float64Array,
  outMACDSignal: Float64Array,
  outMACDHist: Float64Array,
): MacdfixStream {
  const sp = MacdfixStream.of(core);
  const retCode = macdfixOpenImpl(sp, inReal, startIdx, optInSignalPeriod, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MACDFIX openAndFill', inReal.length, startIdx, macdfixLookbackPositional(optInSignalPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && macdfixLookbackPositional(optInSignalPeriod, core) < 0) {
    throw rejectedParams('MACDFIX openAndFill', PARAM_RANGES, optInSignalPeriod);
  }
  throw streamFailure('MACDFIX openAndFill', retCode);
}

/** @internal */
export function macdfixOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInSignalPeriod: number,
): MacdfixStream {
  const sp = MacdfixStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outMACD = new Float64Array(1);
  const sink_outMACDSignal = new Float64Array(1);
  const sink_outMACDHist = new Float64Array(1);
  const retCode = macdfixOpenImpl(sp, inReal, startIdx, optInSignalPeriod, outBegIdx, outNBElement, sink_outMACD, sink_outMACDSignal, sink_outMACDHist, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MACDFIX open', inReal.length, startIdx, macdfixLookbackPositional(optInSignalPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && macdfixLookbackPositional(optInSignalPeriod, core) < 0) {
    throw rejectedParams('MACDFIX open', PARAM_RANGES, optInSignalPeriod);
  }
  throw streamFailure('MACDFIX open', retCode);
}

/** @internal */
export function macdfixOpenPositional(
  inReal: ArrayLike<number>,
  optInSignalPeriod: number,
  core: Core = Core.DEFAULT,
): MacdfixStream {
  optInSignalPeriod = requireInt32('MACDFIX open', 'signalPeriod', optInSignalPeriod);
  requireArgument('MACDFIX open', 'real', inReal);
  requireHistory('MACDFIX open', inReal.length);
  return macdfixOpenInternal(core, float64(inReal), 0, optInSignalPeriod);
}

/** @internal */
export function macdfixOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInSignalPeriod: number,
  outMACD: Float64Array,
  outMACDSignal: Float64Array,
  outMACDHist: Float64Array,
  core: Core = Core.DEFAULT,
): MacdfixStream {
  optInSignalPeriod = requireInt32('MACDFIX openAndFill', 'signalPeriod', optInSignalPeriod);
  requireArgument('MACDFIX openAndFill', 'real', inReal);
  requireHistory('MACDFIX openAndFill', inReal.length);
  const guardLookback = macdfixLookbackPositional(optInSignalPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('MACDFIX openAndFill', PARAM_RANGES, optInSignalPeriod);
  }
  const guardOutLen = openFillCount('MACDFIX openAndFill', inReal.length, guardLookback);
  requireOutput('MACDFIX openAndFill', 'outMacd', outMACD, guardOutLen, Float64Array);
  requireOutput('MACDFIX openAndFill', 'outMacdSignal', outMACDSignal, guardOutLen, Float64Array);
  requireOutput('MACDFIX openAndFill', 'outMacdHist', outMACDHist, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outMACD.buffer === inRealF64.buffer && outMACD.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('MACDFIX openAndFill', 'outMacd', 'real');
  }
  if (outMACDSignal.buffer === inRealF64.buffer && outMACDSignal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('MACDFIX openAndFill', 'outMacdSignal', 'real');
  }
  if (outMACDHist.buffer === inRealF64.buffer && outMACDHist.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('MACDFIX openAndFill', 'outMacdHist', 'real');
  }
  if (outMACD.buffer === outMACDSignal.buffer && outMACD.byteOffset === outMACDSignal.byteOffset) {
    throw sharedMemory('MACDFIX openAndFill', 'outMacd', 'outMacdSignal');
  }
  if (outMACD.buffer === outMACDHist.buffer && outMACD.byteOffset === outMACDHist.byteOffset) {
    throw sharedMemory('MACDFIX openAndFill', 'outMacd', 'outMacdHist');
  }
  if (outMACDSignal.buffer === outMACDHist.buffer && outMACDSignal.byteOffset === outMACDHist.byteOffset) {
    throw sharedMemory('MACDFIX openAndFill', 'outMacdSignal', 'outMacdHist');
  }
  if (guardOutLen > 0 && overlaps(outMACD, inRealF64)) {
    throw sharedMemory('MACDFIX openAndFill', 'outMacd', 'real');
  }
  if (guardOutLen > 0 && overlaps(outMACDSignal, inRealF64)) {
    throw sharedMemory('MACDFIX openAndFill', 'outMacdSignal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outMACDHist, inRealF64)) {
    throw sharedMemory('MACDFIX openAndFill', 'outMacdHist', 'real');
  }
  if (guardOutLen > 0 && overlaps(outMACD, outMACDSignal)) {
    throw sharedMemory('MACDFIX openAndFill', 'outMacd', 'outMacdSignal');
  }
  if (guardOutLen > 0 && overlaps(outMACD, outMACDHist)) {
    throw sharedMemory('MACDFIX openAndFill', 'outMacd', 'outMacdHist');
  }
  if (guardOutLen > 0 && overlaps(outMACDSignal, outMACDHist)) {
    throw sharedMemory('MACDFIX openAndFill', 'outMacdSignal', 'outMacdHist');
  }
  return macdfixOpenAndFillInternal(core, inRealF64, 0, optInSignalPeriod, new MInteger(), new MInteger(), outMACD, outMACDSignal, outMACDHist);
}

/**
 * Opens a live MACDFIX stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link macdfix} there.
 *
 * @param real - Warm-up history. Source series (typically close)
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MacdfixOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `macdfixLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32 or a parameter outside its
 *   range.
 *
 * @example
 * ```ts
 * import { macdfixOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = macdfixOpen(data, { signalPeriod: 9 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function macdfixOpen(real: ArrayLike<number>, options?: MacdfixOptions): MacdfixStream {
  const o = optionsOf('MACDFIX open', options);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MACDFIX open', o.core);
  return macdfixOpenPositional(real, optInSignalPeriod, core);
}

/**
 * {@link macdfixOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link macdfix}.
 *
 * @param real - Warm-up history. Source series (typically close)
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link MacdfixOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link macdfix} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `macdfixLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range
 *   or a buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { macdfixOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const {
 *   stream,
 *   begIdx,
 *   outMacd,
 *   outMacdSignal,
 *   outMacdHist,
 * } = macdfixOpenAndFill(data, { signalPeriod: 9 });
 * // outMacd[k] is the value at bar begIdx + k
 * ```
 */
export function macdfixOpenAndFill(real: ArrayLike<number>, options?: MacdfixOptions): MacdfixResult & { stream: MacdfixStream } {
  const o = optionsOf('MACDFIX openAndFill', options);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MACDFIX openAndFill', o.core);
  const capacity = fillCapacity(macdfixLookbackPositional(optInSignalPeriod, core), real);
  const outMACD = o.outMacd === undefined ? new Float64Array(capacity) : o.outMacd;
  const outMACDSignal = o.outMacdSignal === undefined ? new Float64Array(capacity) : o.outMacdSignal;
  const outMACDHist = o.outMacdHist === undefined ? new Float64Array(capacity) : o.outMacdHist;
  const stream = macdfixOpenAndFillPositional(real, optInSignalPeriod, outMACD, outMACDSignal, outMACDHist, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outMacd: outMACD.subarray(0, range.count), outMacdSignal: outMACDSignal.subarray(0, range.count), outMacdHist: outMACDHist.subarray(0, range.count) };
}
