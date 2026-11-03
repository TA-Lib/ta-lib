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
 *  092926 MF,CC  First version (issue #474).
 *  102326 MF,CC  #554 TA_CMOU's branch-free split and zero run, rare reset.
 */

import { FuncUnstId } from '../enums.ts';
import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma, fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link vidya} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param optInTimePeriod - The EMA length whose alpha the CMO scales (default 12;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInCMOPeriod - Number of trailing price changes in the CMO (default 9;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function vidyaLookback(optInTimePeriod: number, optInCMOPeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if ((optInCMOPeriod | 0) !== optInCMOPeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 12;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  if (optInCMOPeriod === Core.INTEGER_DEFAULT) {
    optInCMOPeriod = 9;
  } else if (optInCMOPeriod < 2 || optInCMOPeriod > 100000) {
    return -1;
  }
  let root = 0;
  root = (Math.sqrt(optInCMOPeriod) | 0);
  if (optInTimePeriod === 1) {
    return core.unstableCount(FuncUnstId.VIDYA, 0, 0);
  }
  return optInCMOPeriod + core.unstableCount(FuncUnstId.VIDYA, ((((((((2 * 4) | 0) * (optInTimePeriod + 1)) | 0) * root) | 0) > 100000000) ? 100000000 : ((((((2 * 4) | 0) * (optInTimePeriod + 1)) | 0) * root) | 0)), ((((((((2 * 8) | 0) * (optInTimePeriod + 1)) | 0) * root) | 0) > 100000000) ? 100000000 : ((((((2 * 8) | 0) * (optInTimePeriod + 1)) | 0) * root) | 0)));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link vidya}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - The EMA length whose alpha the CMO scales (default 12;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInCMOPeriod - Number of trailing price changes in the CMO (default 9;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function vidyaDisplayShift(optInTimePeriod: number, optInCMOPeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (vidyaLookback(optInTimePeriod, optInCMOPeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function vidyaImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  optInCMOPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let outIdx = 0;
  let today = 0;
  let trailingIdx = 0;
  let lookbackTotal = 0;
  let i = 0;
  let nullRun = 0;
  let live = 0;
  let upSum = 0;
  let downSum = 0;
  let sum = 0;
  let diff = 0;
  let up = 0;
  let tempReal = 0;
  let prevValue = 0;
  let trailingValue = 0;
  let alpha = 0;
  let k = 0;
  let prevVIDYA = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 12;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInCMOPeriod === Core.INTEGER_DEFAULT) {
    optInCMOPeriod = 9;
  } else if (optInCMOPeriod < 2 || optInCMOPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  /* No smoothing at period 1: the output is a copy of the input, as MA gives
   * for every MAType. The unstable period still delays the first output.
   */
  if (optInTimePeriod === 1) {
    lookbackTotal = vidyaLookback(optInTimePeriod, optInCMOPeriod, core);
    if (startIdx < lookbackTotal) {
      startIdx = lookbackTotal;
    }
    if (startIdx > endIdx) {
      return RetCode.SUCCESS;
    }
    outBegIdx.value = startIdx;
    outIdx = 0;
    today = startIdx;
    while (today <= endIdx) {
      outReal[outIdx++] = inReal[today++];
    }
    outNBElement.value = outIdx;
    return RetCode.SUCCESS;
  }
  lookbackTotal = vidyaLookback(optInTimePeriod, optInCMOPeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  alpha = 2.0 / (optInTimePeriod + 1);
  /* The CMO below is TA_CMOU's loop, spelling and nullRun reset included, so
   * that from the first full window on VIDYA equals the composite of TA_CMOU
   * bit for bit. The recursion is seeded on the window's first price and
   * steps through the warm-up with the CMO of the changes seen so far. The
   * unstable period enters it that many bars before startIdx.
   */
  today = startIdx - lookbackTotal;
  trailingIdx = today + 1;
  prevValue = inReal[today];
  trailingValue = prevValue;
  prevVIDYA = prevValue;
  upSum = 0.0;
  downSum = 0.0;
  nullRun = 0;
  for (i = 0; i < optInCMOPeriod; i += 1) {
    today += 1;
    tempReal = inReal[today];
    diff = tempReal - prevValue;
    prevValue = tempReal;
    up = (diff + Math.abs(diff)) * 0.5;
    upSum += up;
    downSum += up - diff;
    live = (!(Math.abs(diff) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
    sum = upSum + downSum;
    if (sum > 0.0) {
      k = alpha * (Math.abs(100.0 * (upSum - downSum) / sum) / 100.0);
    } else {
      k = 0.0;
    }
    {
      const _a: number = prevValue - prevVIDYA, _b: number = k, _c: number = prevVIDYA;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevVIDYA = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
  }
  /* Each bar below drops the change that leaves the window after its own
   * update, ready for the next bar, as in TA_CMOU.
   */
  tempReal = inReal[trailingIdx];
  diff = tempReal - trailingValue;
  trailingValue = tempReal;
  trailingIdx += 1;
  up = (diff + Math.abs(diff)) * 0.5;
  upSum -= up;
  downSum -= up - diff;
  today += 1;
  /* Skip the unstable period: the whole computation, nothing written. */
  while (today <= startIdx) {
    tempReal = inReal[today];
    diff = tempReal - prevValue;
    prevValue = tempReal;
    up = (diff + Math.abs(diff)) * 0.5;
    upSum += up;
    downSum += up - diff;
    live = (!(Math.abs(diff) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
    if (nullRun >= optInCMOPeriod) {
      nullRun = optInCMOPeriod;
      upSum = 0.0;
      downSum = 0.0;
      k = 0.0;
    } else {
      sum = upSum + downSum;
      if (sum > 0.0) {
        k = alpha * (Math.abs(100.0 * (upSum - downSum) / sum) / 100.0);
      } else {
        k = 0.0;
      }
    }
    {
      const _a: number = prevValue - prevVIDYA, _b: number = k, _c: number = prevVIDYA;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevVIDYA = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    tempReal = inReal[trailingIdx];
    diff = tempReal - trailingValue;
    trailingValue = tempReal;
    trailingIdx += 1;
    up = (diff + Math.abs(diff)) * 0.5;
    upSum -= up;
    downSum -= up - diff;
    today += 1;
  }
  outReal[0] = prevVIDYA;
  outIdx = 1;
  while (today <= endIdx) {
    tempReal = inReal[today];
    diff = tempReal - prevValue;
    prevValue = tempReal;
    up = (diff + Math.abs(diff)) * 0.5;
    upSum += up;
    downSum += up - diff;
    live = (!(Math.abs(diff) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
    if (nullRun >= optInCMOPeriod) {
      nullRun = optInCMOPeriod;
      upSum = 0.0;
      downSum = 0.0;
      k = 0.0;
    } else {
      sum = upSum + downSum;
      if (sum > 0.0) {
        k = alpha * (Math.abs(100.0 * (upSum - downSum) / sum) / 100.0);
      } else {
        k = 0.0;
      }
    }
    {
      const _a: number = prevValue - prevVIDYA, _b: number = k, _c: number = prevVIDYA;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevVIDYA = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    outReal[outIdx++] = prevVIDYA;
    tempReal = inReal[trailingIdx];
    diff = tempReal - trailingValue;
    trailingValue = tempReal;
    trailingIdx += 1;
    up = (diff + Math.abs(diff)) * 0.5;
    upSum -= up;
    downSum -= up - diff;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * Variable Index Dynamic Average (Tushar Chande): an EMA whose smoothing factor is
 * scaled every bar by the absolute value of the Chande Momentum Oscillator. It
 * follows the price like an EMA in a one-way move and stops moving when up and
 * down moves balance, so it flattens out in consolidations.
 *
 * Formula and more info at
 * [ta-lib.org/functions/vidya](https://ta-lib.org/functions/vidya).
 *
 * **Notes**
 * - A period of 1 performs no smoothing: the output is a copy of the input,
 *   consistent with `MA(period=1)` for every MAType.
 * - Chande's 1992 article drives the same step with a ratio of standard
 *   deviations; this is his 1995 form, driven by the CMO.
 * - The CMO is Chande's unsmoothed one. An implementation driven by a
 *   Wilder-smoothed CMO (`CMO`) computes a different line that does not converge
 *   to this one.
 * - Being recursive, an output depends on how much history precedes it, and the
 *   seed's influence decays more slowly the closer the CMO stays to 0.
 *   Implementations that seed differently agree with this one only once that
 *   influence has decayed.
 * - As an `MA` type, the one period is n and the CMO period is (3n + 2) / 4 in
 *   integer division, Chande's 12:9 ratio.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link vidyaLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - Data on which to compute the average.
 * @param optInTimePeriod - The EMA length whose alpha the CMO scales (default 12;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInCMOPeriod - Number of trailing price changes in the CMO (default 9;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Variable Index Dynamic Average line. Must hold the values
 *   produced and never be empty; the input length always suffices.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The range written: `begIdx` is the first bar with a value, `count` how
 *   many were written.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for an argument that is not an int32, a
 *   parameter outside its range, an array too short for the range, an empty output
 *   or an output sharing memory with a `Float64Array` input it is not. Checked
 *   before anything is written.
 *
 * @example
 * ```ts
 * import { vidya } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const { begIdx, count } = vidya(0, data.length - 1, data, 12, 9, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`cmou`](https://ta-lib.org/functions/cmou)
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`kama`](https://ta-lib.org/functions/kama)
 * @see [`ma`](https://ta-lib.org/functions/ma)
 */
export function vidya(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInCMOPeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('VIDYA', 'startIdx', startIdx);
  endIdx = requireInt32('VIDYA', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('VIDYA', 'optInTimePeriod', optInTimePeriod);
  optInCMOPeriod = requireInt32('VIDYA', 'optInCMOPeriod', optInCMOPeriod);
  requireIndexRange('VIDYA', startIdx, endIdx);
  const guardStart = clampedStart('VIDYA', startIdx, vidyaLookback(optInTimePeriod, optInCMOPeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('VIDYA', 'inReal', inReal, guardInLen);
  requireLength('VIDYA', 'outReal', outReal, guardOutLen);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw failure('VIDYA', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = vidyaImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, optInCMOPeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('VIDYA', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live VIDYA stream: one value per closed bar, bit-identical to {@link vidya}
 * over the same series. Open it with {@link vidyaOpen} or
 * {@link vidyaOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/vidya](https://ta-lib.org/functions/vidya).
 */
export class VidyaStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  optInCMOPeriod: number = 0;
  /** @internal */
  nullRun: number = 0;
  /** @internal */
  upSum: number = 0;
  /** @internal */
  downSum: number = 0;
  /** @internal */
  prevValue: number = 0;
  /** @internal */
  trailingValue: number = 0;
  /** @internal */
  alpha: number = 0;
  /** @internal */
  prevVIDYA: number = 0;
  /** @internal */
  ringPos_trailingIdx: number = 0;
  /** @internal */
  ringCap_trailingIdx: number = 0;
  /** @internal */
  ring_trailingIdx_inReal: Float64Array = new Float64Array(1);
  /** @internal */
  cur_outReal: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  /** @internal */
  constructor(core: Core) {
    this.core = core;
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link vidya} reports them over the same bars. Each accepted `update` and each
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
      throw failure('VIDYA advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: Data on which to compute the average.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('VIDYA update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('VIDYA update', 'inReal');
    }
    vidyaStepImpl(this, inReal);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Data on which to compute the average.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number): number {
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('VIDYA peek', 'inReal');
    }
    const sp = this;
    let live = 0;
    let sum = 0;
    let diff = 0;
    let up = 0;
    let tempReal = 0;
    let k = 0;
    let cur_outReal = 0;
    let downSum = sp.downSum;
    let nullRun = sp.nullRun;
    let prevVIDYA = sp.prevVIDYA;
    let prevValue = sp.prevValue;
    let upSum = sp.upSum;
    if (sp.optInTimePeriod === 1) {
      cur_outReal = inReal;
      return cur_outReal;
    }
    tempReal = inReal;
    diff = tempReal - prevValue;
    prevValue = tempReal;
    up = (diff + Math.abs(diff)) * 0.5;
    upSum += up;
    downSum += up - diff;
    live = (!(Math.abs(diff) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
    if (nullRun >= sp.optInCMOPeriod) {
      nullRun = sp.optInCMOPeriod;
      upSum = 0.0;
      downSum = 0.0;
      k = 0.0;
    } else {
      sum = upSum + downSum;
      if (sum > 0.0) {
        k = sp.alpha * (Math.abs(100.0 * (upSum - downSum) / sum) / 100.0);
      } else {
        k = 0.0;
      }
    }
    prevVIDYA = fma(prevValue - prevVIDYA, k, prevVIDYA);
    cur_outReal = prevVIDYA;
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
  clone(): VidyaStream {
    const c = new VidyaStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.optInCMOPeriod = this.optInCMOPeriod;
    c.nullRun = this.nullRun;
    c.upSum = this.upSum;
    c.downSum = this.downSum;
    c.prevValue = this.prevValue;
    c.trailingValue = this.trailingValue;
    c.alpha = this.alpha;
    c.prevVIDYA = this.prevVIDYA;
    c.ringPos_trailingIdx = this.ringPos_trailingIdx;
    c.ringCap_trailingIdx = this.ringCap_trailingIdx;
    c.ring_trailingIdx_inReal = this.ring_trailingIdx_inReal.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function vidyaStepImpl(sp: VidyaStream, inReal: number): void {
  let live = 0;
  let sum = 0;
  let diff = 0;
  let up = 0;
  let tempReal = 0;
  let k = 0;
  let ringCapL_trailingIdx = 0;
  if (sp.optInTimePeriod === 1) {
    sp.cur_outReal = inReal;
    return;
  }
  tempReal = inReal;
  diff = tempReal - sp.prevValue;
  sp.prevValue = tempReal;
  up = (diff + Math.abs(diff)) * 0.5;
  sp.upSum += up;
  sp.downSum += up - diff;
  live = (!(Math.abs(diff) <= 0.0) ? 1 : 0);
  sp.nullRun = (sp.nullRun + 1) & (live - 1);
  if (sp.nullRun >= sp.optInCMOPeriod) {
    sp.nullRun = sp.optInCMOPeriod;
    sp.upSum = 0.0;
    sp.downSum = 0.0;
    k = 0.0;
  } else {
    sum = sp.upSum + sp.downSum;
    if (sum > 0.0) {
      k = sp.alpha * (Math.abs(100.0 * (sp.upSum - sp.downSum) / sum) / 100.0);
    } else {
      k = 0.0;
    }
  }
  sp.prevVIDYA = fma(sp.prevValue - sp.prevVIDYA, k, sp.prevVIDYA);
  sp.cur_outReal = sp.prevVIDYA;
  tempReal = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
  diff = tempReal - sp.trailingValue;
  sp.trailingValue = tempReal;
  up = (diff + Math.abs(diff)) * 0.5;
  sp.upSum -= up;
  sp.downSum -= up - diff;
  ringCapL_trailingIdx = sp.ringCap_trailingIdx;
  sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
  sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
  if (sp.ringPos_trailingIdx >= ringCapL_trailingIdx) {
    sp.ringPos_trailingIdx = 0;
  }
}

function vidyaOpenImpl(
  sp: VidyaStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInCMOPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let outIdx = 0;
  let today = 0;
  let trailingIdx = 0;
  let lookbackTotal = 0;
  let i = 0;
  let nullRun = 0;
  let live = 0;
  let upSum = 0;
  let downSum = 0;
  let sum = 0;
  let diff = 0;
  let up = 0;
  let tempReal = 0;
  let prevValue = 0;
  let trailingValue = 0;
  let alpha = 0;
  let k = 0;
  let prevVIDYA = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 12;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInCMOPeriod === Core.INTEGER_DEFAULT) {
    optInCMOPeriod = 9;
  } else if (optInCMOPeriod < 2 || optInCMOPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (optInTimePeriod === 1) {
    let fillLb = vidyaLookback(optInTimePeriod, optInCMOPeriod, core);
    if (startIdx > fillLb) {
      fillLb = startIdx;
    }
    if (historyLen < fillLb + 1) {
      return RetCode.INSUFFICIENT_HISTORY;
    }
    sp.optInTimePeriod = optInTimePeriod;
    sp.optInCMOPeriod = optInCMOPeriod;
    sp.nullRun = 0;
    sp.upSum = 0;
    sp.downSum = 0;
    sp.prevValue = 0;
    sp.trailingValue = 0;
    sp.alpha = 0;
    sp.prevVIDYA = 0;
    sp.ringPos_trailingIdx = 0;
    sp.ringCap_trailingIdx = 0;
    sp.ring_trailingIdx_inReal = new Float64Array(1);
    outBegIdx.value = fillLb;
    outNBElement.value = historyLen - fillLb;
    if (outStride === 0) {
      outReal[0] = inReal[historyLen - 1];
    } else {
      for (let fillIdx = 0; fillIdx < historyLen - fillLb; fillIdx++) {
        outReal[fillIdx] = inReal[fillLb + fillIdx];
      }
    }
    sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
    return RetCode.SUCCESS;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = vidyaLookback(optInTimePeriod, optInCMOPeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  alpha = 2.0 / (optInTimePeriod + 1);
  /* The CMO below is TA_CMOU's loop, spelling and nullRun reset included, so
   * that from the first full window on VIDYA equals the composite of TA_CMOU
   * bit for bit. The recursion is seeded on the window's first price and
   * steps through the warm-up with the CMO of the changes seen so far. The
   * unstable period enters it that many bars before startIdx.
   */
  today = startIdx - lookbackTotal;
  trailingIdx = today + 1;
  prevValue = inReal[today];
  trailingValue = prevValue;
  prevVIDYA = prevValue;
  upSum = 0.0;
  downSum = 0.0;
  nullRun = 0;
  for (i = 0; i < optInCMOPeriod; i += 1) {
    today += 1;
    tempReal = inReal[today];
    diff = tempReal - prevValue;
    prevValue = tempReal;
    up = (diff + Math.abs(diff)) * 0.5;
    upSum += up;
    downSum += up - diff;
    live = (!(Math.abs(diff) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
    sum = upSum + downSum;
    if (sum > 0.0) {
      k = alpha * (Math.abs(100.0 * (upSum - downSum) / sum) / 100.0);
    } else {
      k = 0.0;
    }
    {
      const _a: number = prevValue - prevVIDYA, _b: number = k, _c: number = prevVIDYA;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevVIDYA = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
  }
  /* Each bar below drops the change that leaves the window after its own
   * update, ready for the next bar, as in TA_CMOU.
   */
  tempReal = inReal[trailingIdx];
  diff = tempReal - trailingValue;
  trailingValue = tempReal;
  trailingIdx += 1;
  up = (diff + Math.abs(diff)) * 0.5;
  upSum -= up;
  downSum -= up - diff;
  today += 1;
  /* Skip the unstable period: the whole computation, nothing written. */
  while (today <= startIdx) {
    tempReal = inReal[today];
    diff = tempReal - prevValue;
    prevValue = tempReal;
    up = (diff + Math.abs(diff)) * 0.5;
    upSum += up;
    downSum += up - diff;
    live = (!(Math.abs(diff) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
    if (nullRun >= optInCMOPeriod) {
      nullRun = optInCMOPeriod;
      upSum = 0.0;
      downSum = 0.0;
      k = 0.0;
    } else {
      sum = upSum + downSum;
      if (sum > 0.0) {
        k = alpha * (Math.abs(100.0 * (upSum - downSum) / sum) / 100.0);
      } else {
        k = 0.0;
      }
    }
    {
      const _a: number = prevValue - prevVIDYA, _b: number = k, _c: number = prevVIDYA;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevVIDYA = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    tempReal = inReal[trailingIdx];
    diff = tempReal - trailingValue;
    trailingValue = tempReal;
    trailingIdx += 1;
    up = (diff + Math.abs(diff)) * 0.5;
    upSum -= up;
    downSum -= up - diff;
    today += 1;
  }
  outReal[((0 * outStride) | 0)] = prevVIDYA;
  outIdx = 1;
  while (today <= endIdx) {
    tempReal = inReal[today];
    diff = tempReal - prevValue;
    prevValue = tempReal;
    up = (diff + Math.abs(diff)) * 0.5;
    upSum += up;
    downSum += up - diff;
    live = (!(Math.abs(diff) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
    if (nullRun >= optInCMOPeriod) {
      nullRun = optInCMOPeriod;
      upSum = 0.0;
      downSum = 0.0;
      k = 0.0;
    } else {
      sum = upSum + downSum;
      if (sum > 0.0) {
        k = alpha * (Math.abs(100.0 * (upSum - downSum) / sum) / 100.0);
      } else {
        k = 0.0;
      }
    }
    {
      const _a: number = prevValue - prevVIDYA, _b: number = k, _c: number = prevVIDYA;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevVIDYA = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    outReal[((outIdx++ * outStride) | 0)] = prevVIDYA;
    tempReal = inReal[trailingIdx];
    diff = tempReal - trailingValue;
    trailingValue = tempReal;
    trailingIdx += 1;
    up = (diff + Math.abs(diff)) * 0.5;
    upSum -= up;
    downSum -= up - diff;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  const cap_trailingIdx = today - trailingIdx;
  if (cap_trailingIdx < 1 || cap_trailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_trailingIdx = cap_trailingIdx > 0 ? cap_trailingIdx : 1;
  const capRing_trailingIdx_inReal = new Float64Array(allocN_trailingIdx);
  capRing_trailingIdx_inReal.set(inReal.subarray(historyLen - cap_trailingIdx, historyLen), 0);
  sp.optInTimePeriod = optInTimePeriod;
  sp.optInCMOPeriod = optInCMOPeriod;
  sp.nullRun = nullRun;
  sp.upSum = upSum;
  sp.downSum = downSum;
  sp.prevValue = prevValue;
  sp.trailingValue = trailingValue;
  sp.alpha = alpha;
  sp.prevVIDYA = prevVIDYA;
  sp.ringPos_trailingIdx = 0;
  sp.ringCap_trailingIdx = cap_trailingIdx;
  sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function vidyaOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInCMOPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): VidyaStream {
  const sp = new VidyaStream(core);
  const retCode = vidyaOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInCMOPeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('VIDYA openAndFill', inReal.length, startIdx, vidyaLookback(optInTimePeriod, optInCMOPeriod, core));
  }
  throw streamFailure('VIDYA openAndFill', retCode);
}

/** @internal */
export function vidyaOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInCMOPeriod: number,
): VidyaStream {
  const sp = new VidyaStream(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = vidyaOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInCMOPeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('VIDYA open', inReal.length, startIdx, vidyaLookback(optInTimePeriod, optInCMOPeriod, core));
  }
  throw streamFailure('VIDYA open', retCode);
}

/**
 * Opens a live VIDYA stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link vidya} there.
 *
 * @param inReal - Warm-up history. Data on which to compute the average.
 * @param optInTimePeriod - The EMA length whose alpha the CMO scales (default 12;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInCMOPeriod - Number of trailing price changes in the CMO (default 9;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `vidyaLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32 or a
 *   parameter outside its range.
 *
 * @example
 * ```ts
 * import { vidyaOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = vidyaOpen(data, 12, 9);
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function vidyaOpen(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInCMOPeriod: number,
  core: Core = Core.DEFAULT,
): VidyaStream {
  optInTimePeriod = requireInt32('VIDYA open', 'optInTimePeriod', optInTimePeriod);
  optInCMOPeriod = requireInt32('VIDYA open', 'optInCMOPeriod', optInCMOPeriod);
  requireArgument('VIDYA open', 'inReal', inReal);
  requireHistory('VIDYA open', inReal.length);
  return vidyaOpenInternal(core, float64(inReal), 0, optInTimePeriod, optInCMOPeriod);
}

/**
 * {@link vidyaOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link vidya}. The range written is the stream's
 * `outRange()`.
 *
 * @param inReal - Warm-up history. Data on which to compute the average.
 * @param optInTimePeriod - The EMA length whose alpha the CMO scales (default 12;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInCMOPeriod - Number of trailing price changes in the CMO (default 9;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Variable Index Dynamic Average line. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `vidyaLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or an output too short or starting where another
 *   output or a `Float64Array` input starts, or, on a history long enough to open,
 *   one sharing memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { vidyaOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const stream = vidyaOpenAndFill(data, 12, 9, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function vidyaOpenAndFill(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInCMOPeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): VidyaStream {
  optInTimePeriod = requireInt32('VIDYA openAndFill', 'optInTimePeriod', optInTimePeriod);
  optInCMOPeriod = requireInt32('VIDYA openAndFill', 'optInCMOPeriod', optInCMOPeriod);
  requireArgument('VIDYA openAndFill', 'inReal', inReal);
  requireHistory('VIDYA openAndFill', inReal.length);
  const guardOutLen = openFillCount('VIDYA openAndFill', inReal.length, vidyaLookback(optInTimePeriod, optInCMOPeriod, core));
  requireLength('VIDYA openAndFill', 'outReal', outReal, guardOutLen);
  const inRealF64 = float64(inReal);
  if ((outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset)) {
    throw streamFailure('VIDYA openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inRealF64))) {
    throw streamFailure('VIDYA openAndFill', RetCode.BAD_PARAM);
  }
  return vidyaOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, optInCMOPeriod, new MInteger(), new MInteger(), outReal);
}

/** @internal */
export function vidyaStepTape(sp: VidyaStream, tape: Float64Array, tapeBase: number, tapeMask: number, inReal: number): number {
  let live = 0;
  let sum = 0;
  let diff = 0;
  let up = 0;
  let tempReal = 0;
  let k = 0;
  tempReal = inReal;
  diff = tempReal - sp.prevValue;
  sp.prevValue = tempReal;
  up = (diff + Math.abs(diff)) * 0.5;
  sp.upSum += up;
  sp.downSum += up - diff;
  live = (!(Math.abs(diff) <= 0.0) ? 1 : 0);
  sp.nullRun = (sp.nullRun + 1) & (live - 1);
  if (sp.nullRun >= sp.optInCMOPeriod) {
    sp.nullRun = sp.optInCMOPeriod;
    sp.upSum = 0.0;
    sp.downSum = 0.0;
    k = 0.0;
  } else {
    sum = sp.upSum + sp.downSum;
    if (sum > 0.0) {
      k = sp.alpha * (Math.abs(100.0 * (sp.upSum - sp.downSum) / sum) / 100.0);
    } else {
      k = 0.0;
    }
  }
  sp.prevVIDYA = fma(sp.prevValue - sp.prevVIDYA, k, sp.prevVIDYA);
  sp.cur_outReal = sp.prevVIDYA;
  tempReal = tape[(tapeBase - sp.ringCap_trailingIdx) & tapeMask];
  diff = tempReal - sp.trailingValue;
  sp.trailingValue = tempReal;
  up = (diff + Math.abs(diff)) * 0.5;
  sp.upSum -= up;
  sp.downSum -= up - diff;
  sp.outRangeCount++;
  return sp.cur_outReal;
}

/** @internal */
export function vidyaPeekTape(sp: VidyaStream, _tape: Float64Array, _tapeBase: number, _tapeMask: number, inReal: number): number {
  let live = 0;
  let sum = 0;
  let diff = 0;
  let up = 0;
  let tempReal = 0;
  let k = 0;
  let cur_outReal = 0;
  let downSum = sp.downSum;
  let nullRun = sp.nullRun;
  let prevVIDYA = sp.prevVIDYA;
  let prevValue = sp.prevValue;
  let upSum = sp.upSum;
  tempReal = inReal;
  diff = tempReal - prevValue;
  prevValue = tempReal;
  up = (diff + Math.abs(diff)) * 0.5;
  upSum += up;
  downSum += up - diff;
  live = (!(Math.abs(diff) <= 0.0) ? 1 : 0);
  nullRun = (nullRun + 1) & (live - 1);
  if (nullRun >= sp.optInCMOPeriod) {
    nullRun = sp.optInCMOPeriod;
    upSum = 0.0;
    downSum = 0.0;
    k = 0.0;
  } else {
    sum = upSum + downSum;
    if (sum > 0.0) {
      k = sp.alpha * (Math.abs(100.0 * (upSum - downSum) / sum) / 100.0);
    } else {
      k = 0.0;
    }
  }
  prevVIDYA = fma(prevValue - prevVIDYA, k, prevVIDYA);
  cur_outReal = prevVIDYA;
  return cur_outReal;
}

/** @internal */
export function vidyaTapeDetach(sp: VidyaStream): number {
  let reach = 0;
  sp.ring_trailingIdx_inReal = new Float64Array(0);
  if (sp.ringCap_trailingIdx > reach) {
    reach = sp.ringCap_trailingIdx;
  }
  return reach;
}
