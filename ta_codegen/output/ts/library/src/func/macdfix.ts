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

import { badSink, clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { emaLookback } from './ema.ts';

/**
 * Number of leading input bars {@link macdfix} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param optInSignalPeriod - Smoothing period for the signal line (default 9;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function macdfixLookback(optInSignalPeriod: number, core: Core = Core.DEFAULT): number {
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
  return emaLookback(26, core) + emaLookback(optInSignalPeriod, core);
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link macdfix}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInSignalPeriod - Smoothing period for the signal line (default 9;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function macdfixDisplayShift(optInSignalPeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (macdfixLookback(optInSignalPeriod, core) < 0) {
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
  lookbackSignal = emaLookback(optInSignalPeriod, core);
  /* Move up the start index if there is not
   * enough initial data.
   */
  lookbackTotal = lookbackSignal;
  lookbackTotal += emaLookback(26, core);
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
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link macdfixLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - Source series (typically close)
 * @param optInSignalPeriod - Smoothing period for the signal line (default 9;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outMACD - Fixed EMA12 minus EMA26. Must hold the values produced and
 *   never be empty; the input length always suffices.
 * @param outMACDSignal - EMA of the MACD line. Must hold the values produced and
 *   never be empty; the input length always suffices.
 * @param outMACDHist - MACD minus signal. Must hold the values produced and never
 *   be empty; the input length always suffices.
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
 * import { macdfix } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const macd = new Float64Array(data.length);
 * const macdSignal = new Float64Array(data.length);
 * const macdHist = new Float64Array(data.length);
 * const { begIdx, count } = macdfix(
 *   0, data.length - 1,
 *   data,
 *   9,
 *   macd, macdSignal, macdHist,
 * );
 * // macd[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`macd`](https://ta-lib.org/functions/macd)
 * @see [`macdext`](https://ta-lib.org/functions/macdext)
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`apo`](https://ta-lib.org/functions/apo)
 */
export function macdfix(
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
  optInSignalPeriod = requireInt32('MACDFIX', 'optInSignalPeriod', optInSignalPeriod);
  requireIndexRange('MACDFIX', startIdx, endIdx);
  const guardStart = clampedStart('MACDFIX', startIdx, macdfixLookback(optInSignalPeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('MACDFIX', 'inReal', inReal, guardInLen);
  requireLength('MACDFIX', 'outMACD', outMACD, guardOutLen);
  requireLength('MACDFIX', 'outMACDSignal', outMACDSignal, guardOutLen);
  requireLength('MACDFIX', 'outMACDHist', outMACDHist, guardOutLen);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outMACD, inRealF64) || overlapsPartly(outMACDSignal, inRealF64) || overlapsPartly(outMACDHist, inRealF64) || overlaps(outMACD, outMACDSignal) || overlaps(outMACD, outMACDHist) || overlaps(outMACDSignal, outMACDHist)) {
    throw failure('MACDFIX', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = macdfixImpl(core, startIdx, endIdx, inRealF64, optInSignalPeriod, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('MACDFIX', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
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
   * Commits one closed bar, writing the new values into `out`.
   *
   * @param inReal - The bar's `inReal` value: Source series (typically close)
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number, out: MacdfixOut): void {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('MACDFIX update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (typeof out !== 'object' || out === null) {
      throw badSink('MACDFIX update', 'MacdfixOut');
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('MACDFIX update', 'inReal');
    }
    macdfixStepImpl(this, inReal);
    this.outRangeCount++;
    out.macd = this.cur_outMACD;
    out.macdSignal = this.cur_outMACDSignal;
    out.macdHist = this.cur_outMACDHist;
  }

  /**
   * Writes into `out` what the next `update` with the same bar would write,
   * committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Source series (typically close)
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number, out: MacdfixOut): void {
    if (typeof out !== 'object' || out === null) {
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
    out.macd = cur_outMACD;
    out.macdSignal = cur_outMACDSignal;
    out.macdHist = cur_outMACDHist;
  }

  /**
   * The value at the last bar this stream counted; `peek` does not change it.
   *
   * @param out - Receives the outputs.
   */
  value(out: MacdfixOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('MACDFIX value', 'MacdfixOut');
    }
    out.macd = this.cur_outMACD;
    out.macdSignal = this.cur_outMACDSignal;
    out.macdHist = this.cur_outMACDHist;
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
 * The outputs of one MACDFIX bar, written into an object the caller owns and
 * reuses: valid until the next call that writes it.
 */
export class MacdfixOut {
  /** Fixed EMA12 minus EMA26. */
  macd: number = 0;
  /** EMA of the MACD line. */
  macdSignal: number = 0;
  /** MACD minus signal. */
  macdHist: number = 0;
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
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInSignalPeriod === Core.INTEGER_DEFAULT) {
    optInSignalPeriod = 9;
  } else if (optInSignalPeriod < 1 || optInSignalPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
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
  lookbackSignal = emaLookback(optInSignalPeriod, core);
  /* Move up the start index if there is not
   * enough initial data.
   */
  lookbackTotal = lookbackSignal;
  lookbackTotal += emaLookback(26, core);
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
    throw insufficientHistory('MACDFIX openAndFill', inReal.length, startIdx, macdfixLookback(optInSignalPeriod, core));
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
    throw insufficientHistory('MACDFIX open', inReal.length, startIdx, macdfixLookback(optInSignalPeriod, core));
  }
  throw streamFailure('MACDFIX open', retCode);
}

/**
 * Opens a live MACDFIX stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link macdfix} there.
 *
 * @param inReal - Warm-up history. Source series (typically close)
 * @param optInSignalPeriod - Smoothing period for the signal line (default 9;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `macdfixLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32 or a
 *   parameter outside its range.
 *
 * @example
 * ```ts
 * import { MacdfixOut, macdfixOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = macdfixOpen(data, 9);
 * const peeked = new MacdfixOut();
 * const updated = new MacdfixOut();
 * stream.peek(100.9, peeked); // a forming bar: commits nothing
 * stream.update(100.9, updated); // the bar closes
 * ```
 */
export function macdfixOpen(
  inReal: ArrayLike<number>,
  optInSignalPeriod: number,
  core: Core = Core.DEFAULT,
): MacdfixStream {
  optInSignalPeriod = requireInt32('MACDFIX open', 'optInSignalPeriod', optInSignalPeriod);
  requireArgument('MACDFIX open', 'inReal', inReal);
  requireHistory('MACDFIX open', inReal.length);
  return macdfixOpenInternal(core, float64(inReal), 0, optInSignalPeriod);
}

/**
 * {@link macdfixOpen} that also fills the outputs over the whole history in the
 * same pass, bit-identically to {@link macdfix}. The range written is the stream's
 * `outRange()`.
 *
 * @param inReal - Warm-up history. Source series (typically close)
 * @param optInSignalPeriod - Smoothing period for the signal line (default 9;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outMACD - Fixed EMA12 minus EMA26. Must hold `historyLen - lookback`
 *   values and share no memory with an input or another output.
 * @param outMACDSignal - EMA of the MACD line. Must hold `historyLen - lookback`
 *   values and share no memory with an input or another output.
 * @param outMACDHist - MACD minus signal. Must hold `historyLen - lookback` values
 *   and share no memory with an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `macdfixLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or an output too short or starting where another
 *   output or a `Float64Array` input starts, or, on a history long enough to open,
 *   one sharing memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { macdfixOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const macd = new Float64Array(data.length);
 * const macdSignal = new Float64Array(data.length);
 * const macdHist = new Float64Array(data.length);
 * const stream = macdfixOpenAndFill(data, 9, macd, macdSignal, macdHist);
 * const { begIdx, count } = stream.outRange();
 * // macd[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function macdfixOpenAndFill(
  inReal: ArrayLike<number>,
  optInSignalPeriod: number,
  outMACD: Float64Array,
  outMACDSignal: Float64Array,
  outMACDHist: Float64Array,
  core: Core = Core.DEFAULT,
): MacdfixStream {
  optInSignalPeriod = requireInt32('MACDFIX openAndFill', 'optInSignalPeriod', optInSignalPeriod);
  requireArgument('MACDFIX openAndFill', 'inReal', inReal);
  requireHistory('MACDFIX openAndFill', inReal.length);
  const guardOutLen = openFillCount('MACDFIX openAndFill', inReal.length, macdfixLookback(optInSignalPeriod, core));
  requireLength('MACDFIX openAndFill', 'outMACD', outMACD, guardOutLen);
  requireLength('MACDFIX openAndFill', 'outMACDSignal', outMACDSignal, guardOutLen);
  requireLength('MACDFIX openAndFill', 'outMACDHist', outMACDHist, guardOutLen);
  const inRealF64 = float64(inReal);
  if ((outMACD.buffer === inRealF64.buffer && outMACD.byteOffset === inRealF64.byteOffset) || (outMACDSignal.buffer === inRealF64.buffer && outMACDSignal.byteOffset === inRealF64.byteOffset) || (outMACDHist.buffer === inRealF64.buffer && outMACDHist.byteOffset === inRealF64.byteOffset) || (outMACD.buffer === outMACDSignal.buffer && outMACD.byteOffset === outMACDSignal.byteOffset) || (outMACD.buffer === outMACDHist.buffer && outMACD.byteOffset === outMACDHist.byteOffset) || (outMACDSignal.buffer === outMACDHist.buffer && outMACDSignal.byteOffset === outMACDHist.byteOffset)) {
    throw streamFailure('MACDFIX openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outMACD, inRealF64) || overlaps(outMACDSignal, inRealF64) || overlaps(outMACDHist, inRealF64) || overlaps(outMACD, outMACDSignal) || overlaps(outMACD, outMACDHist) || overlaps(outMACDSignal, outMACDHist))) {
    throw streamFailure('MACDFIX openAndFill', RetCode.BAD_PARAM);
  }
  return macdfixOpenAndFillInternal(core, inRealF64, 0, optInSignalPeriod, new MInteger(), new MInteger(), outMACD, outMACDSignal, outMACDHist);
}
