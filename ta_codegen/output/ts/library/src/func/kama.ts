/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  120802 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  062704 MF     Fix limit case to avoid divid by zero (or by
 *                a value close to zero induce by the imprecision
 *                of floating points).
 *  070226 MF,CC  Allow period of 1: output is a copy of the input,
 *                consistent with TA_MA (issues #48, #59). The natural
 *                KAMA math at period=1 would be a fixed-alpha EMA
 *                (efficiency ratio is always 1), which would disagree
 *                with TA_MA's period-1 copy, so identity is explicit.
 *  082326 MF,CC  Fix #253. Recognize a flat window by counting bars and drop
 *                the fixed TA_IS_ZERO band beside the efficiency ratio, which
 *                forced the fastest adaptation on any instrument quoted small
 *                enough to fall under it.
 *  090626 MF,CC  Fix #390. Clamp the efficiency ratio to 1. A drifted sumROC1
 *                let it exceed its own mathematical maximum, and the squared
 *                smoothing constant then amplified instead of averaging.
 *  102326 MF,CC  #554 branch-free zero run, rare reset.
 */

import { FuncUnstId } from '../enums.ts';
import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma, fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link kama} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param optInTimePeriod - Lookback window for the efficiency ratio (default 30;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function kamaLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  let root = 0;
  root = (Math.sqrt(optInTimePeriod) | 0);
  if (optInTimePeriod === 1) {
    return core.unstableCount(FuncUnstId.KAMA, 0, 0);
  }
  return optInTimePeriod + core.unstableCount(FuncUnstId.KAMA, ((((25 * 4) | 0) * root) | 0), ((((25 * 8) | 0) * root) | 0));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link kama}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Lookback window for the efficiency ratio (default 30;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function kamaDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (kamaLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function kamaImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let constMax = 0;
  let constDiff = 0;
  let tempReal = 0;
  let tempReal2 = 0;
  let sumROC1 = 0;
  let periodROC = 0;
  let prevKAMA = 0;
  let i = 0;
  let today = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let trailingIdx = 0;
  let nullRun = 0;
  let live = 0;
  let trailingValue = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  constMax = 2.0 / (30.0 + 1.0);
  constDiff = 2.0 / (2.0 + 1.0) - constMax;
  /* Default return values */
  outBegIdx.value = 0;
  outNBElement.value = 0;
  /* No smoothing at period of 1: the output is a copy of the input
   * (same convention as TA_MA for every MAType). The unstable period
   * still delays the first output for API consistency.
   */
  if (optInTimePeriod === 1) {
    lookbackTotal = kamaLookback(optInTimePeriod, core);
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
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = kamaLookback(optInTimePeriod, core);
  /* Move up the start index if there is not
   * enough initial data.
   */
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  /* Initialize the variables by going through
   * the lookback period.
   */
  sumROC1 = 0.0;
  /* Consecutive 1-day changes of exactly zero, counted so that a flat window
   * can be recognized exactly (the shape #244 needed for MFI). sumROC1 cannot
   * answer that question itself once the window starts sliding: it is
   * maintained by add-then-subtract, so a window that has gone flat leaves it
   * holding rounding residue of arbitrary sign rather than zero, and the
   * efficiency ratio then divides that residue into itself.
   */
  nullRun = 0;
  today = startIdx - lookbackTotal;
  trailingIdx = today;
  i = optInTimePeriod;
  while (i-- > 0) {
    tempReal = inReal[today++];
    tempReal -= inReal[today];
    sumROC1 += Math.abs(tempReal);
    live = (!(Math.abs(tempReal) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
  }
  /* At this point sumROC1 represent the
   * summation of the 1-day price difference
   * over the (optInTimePeriod-1)
   */
  /* Calculate the first KAMA */
  /* The yesterday price is used here as the previous KAMA. */
  prevKAMA = inReal[today - 1];
  tempReal = inReal[today];
  tempReal2 = inReal[trailingIdx++];
  periodROC = tempReal - tempReal2;
  /* Save the trailing value. Do this because inReal
   * and outReal can be pointers to the same buffer.
   */
  trailingValue = tempReal2;
  /* Calculate the efficiency ratio.
   *
   * The ratio cannot exceed 1 in exact arithmetic, but sumROC1 drifts, so
   * clamp it: past 1 the squared smoothing constant amplifies instead of
   * averaging and the recurrence below stops being a convex combination.
   *
   * `sumROC1 <= 0.0` (#385) is now numerically redundant -- the clamp maps its
   * +Inf onto the same 1.0 -- so a value test written against it would pass
   * with it deleted. Keep it anyway: the divisor sweep requires a division to
   * be dominated by a test of its own divisor against a literal zero.
   */
  if (sumROC1 <= 0.0 || sumROC1 <= periodROC) {
    tempReal = 1.0;
  } else {
    tempReal = Math.abs(periodROC / sumROC1);
    if (tempReal > 1.0) {
      tempReal = 1.0;
    }
  }
  /* Calculate the smoothing constant */
  tempReal = fma(tempReal, constDiff, constMax);
  tempReal *= tempReal;
  /* Calculate the KAMA like an EMA, using the
   * smoothing constant as the adaptive factor.
   */
  prevKAMA = fma(inReal[today++] - prevKAMA, tempReal, prevKAMA);
  /* 'today' keep track of where the processing is within the
   * input.
   */
  /* Skip the unstable period. Do the whole processing
   * needed for KAMA, but do not write it in the output.
   */
  while (today <= startIdx) {
    tempReal = inReal[today];
    tempReal2 = inReal[trailingIdx++];
    periodROC = tempReal - tempReal2;
    /* Adjust sumROC1:
     *  - Remove trailing ROC1
     *  - Add new ROC1
     */
    sumROC1 -= Math.abs(trailingValue - tempReal2);
    sumROC1 += Math.abs(tempReal - inReal[today - 1]);
    /* Once a whole window of flat bars has gone by, every 1-day change it
     * spans is exactly zero, so the sum is known to be exactly zero, the
     * residue can be dropped, and the efficiency ratio is 1.
     */
    live = (!(Math.abs(tempReal - inReal[today - 1]) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
    if (nullRun >= optInTimePeriod) {
      nullRun = optInTimePeriod;
      sumROC1 = 0.0;
      tempReal = 1.0;
    } else if (sumROC1 <= 0.0 ||
        sumROC1 <= periodROC) /* Calculate the efficiency ratio */
    {
      tempReal = 1.0;
    } else {
      tempReal = Math.abs(periodROC / sumROC1);
      if (tempReal > 1.0) {
        tempReal = 1.0;
      }
    }
    /* Save the trailing value. Do this because inReal
     * and outReal can be pointers to the same buffer.
     */
    trailingValue = tempReal2;
    /* Calculate the smoothing constant */
    {
      const _a: number = tempReal, _b: number = constDiff, _c: number = constMax;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      tempReal = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    tempReal *= tempReal;
    /* Calculate the KAMA like an EMA, using the
     * smoothing constant as the adaptive factor.
     */
    {
      const _a: number = inReal[today++] - prevKAMA, _b: number = tempReal, _c: number = prevKAMA;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevKAMA = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
  }
  /* Write the first value. */
  outReal[0] = prevKAMA;
  outIdx = 1;
  outBegIdx.value = today - 1;
  /* Do the KAMA calculation for the requested range. */
  while (today <= endIdx) {
    tempReal = inReal[today];
    tempReal2 = inReal[trailingIdx++];
    periodROC = tempReal - tempReal2;
    /* Adjust sumROC1:
     *  - Remove trailing ROC1
     *  - Add new ROC1
     */
    sumROC1 -= Math.abs(trailingValue - tempReal2);
    sumROC1 += Math.abs(tempReal - inReal[today - 1]);
    /* Once a whole window of flat bars has gone by, every 1-day change it
     * spans is exactly zero, so the sum is known to be exactly zero, the
     * residue can be dropped, and the efficiency ratio is 1.
     */
    live = (!(Math.abs(tempReal - inReal[today - 1]) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
    if (nullRun >= optInTimePeriod) {
      nullRun = optInTimePeriod;
      sumROC1 = 0.0;
      tempReal = 1.0;
    } else if (sumROC1 <= 0.0 ||
        sumROC1 <= periodROC) /* Calculate the efficiency ratio */
    {
      tempReal = 1.0;
    } else {
      tempReal = Math.abs(periodROC / sumROC1);
      if (tempReal > 1.0) {
        tempReal = 1.0;
      }
    }
    /* Save the trailing value. Do this because inReal
     * and outReal can be pointers to the same buffer.
     */
    trailingValue = tempReal2;
    /* Calculate the smoothing constant */
    {
      const _a: number = tempReal, _b: number = constDiff, _c: number = constMax;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      tempReal = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    tempReal *= tempReal;
    /* Calculate the KAMA like an EMA, using the
     * smoothing constant as the adaptive factor.
     */
    {
      const _a: number = inReal[today++] - prevKAMA, _b: number = tempReal, _c: number = prevKAMA;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevKAMA = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    outReal[outIdx++] = prevKAMA;
  }
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * Kaufman Adaptive Moving Average: an EMA whose smoothing factor adapts each bar
 * to an efficiency ratio (directional move vs. total volatility). Reacts fast in
 * trends and smooths in ranging markets. Flat KAMA = non-trending/ranging market.
 * KAMA tracking price closely = efficient trend.
 *
 * Formula and more info at
 * [ta-lib.org/functions/kama](https://ta-lib.org/functions/kama).
 *
 * **Notes**
 * - A period of 1 performs no smoothing: the output is a copy of the input,
 *   consistent with `MA(period=1)` for every MAType. (The natural KAMA math at
 *   period 1 would degenerate to a fixed-alpha EMA because the efficiency ratio is
 *   always 1, so the copy is made explicit.) Allowed since 0.6.5.
 * - The output never leaves the range of the prices it has seen.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link kamaLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - Source price series.
 * @param optInTimePeriod - Lookback window for the efficiency ratio (default 30;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Adaptive moving average line. Must hold the values produced and
 *   never be empty; the input length always suffices.
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
 * import { kama } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const { begIdx, count } = kama(0, data.length - 1, data, 30, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`mama`](https://ta-lib.org/functions/mama)
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`ma`](https://ta-lib.org/functions/ma)
 */
export function kama(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('KAMA', 'startIdx', startIdx);
  endIdx = requireInt32('KAMA', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('KAMA', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('KAMA', startIdx, endIdx);
  const guardStart = clampedStart('KAMA', startIdx, kamaLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('KAMA', 'inReal', inReal, guardInLen);
  requireOutput('KAMA', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw failure('KAMA', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = kamaImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('KAMA', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live KAMA stream: one value per closed bar, bit-identical to {@link kama} over
 * the same series. Open it with {@link kamaOpen} or {@link kamaOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/kama](https://ta-lib.org/functions/kama).
 */
export class KamaStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  constMax: number = 0;
  /** @internal */
  constDiff: number = 0;
  /** @internal */
  sumROC1: number = 0;
  /** @internal */
  prevKAMA: number = 0;
  /** @internal */
  nullRun: number = 0;
  /** @internal */
  trailingValue: number = 0;
  /** @internal */
  lag1_inReal: number = 0;
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

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): KamaStream {
    return new KamaStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link kama} reports them over the same bars. Each accepted `update` and each
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
      throw failure('KAMA advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: Source price series.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('KAMA update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('KAMA update', 'inReal');
    }
    kamaStepImpl(this, inReal);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Source price series.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number): number {
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('KAMA peek', 'inReal');
    }
    const sp = this;
    let tempReal = 0;
    let tempReal2 = 0;
    let periodROC = 0;
    let live = 0;
    let cur_outReal = 0;
    let nullRun = sp.nullRun;
    let prevKAMA = sp.prevKAMA;
    let sumROC1 = sp.sumROC1;
    let trailingValue = sp.trailingValue;
    if (sp.optInTimePeriod === 1) {
      cur_outReal = inReal;
      return cur_outReal;
    }
    tempReal = inReal;
    tempReal2 = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
    periodROC = tempReal - tempReal2;
    /* Adjust sumROC1:
     *  - Remove trailing ROC1
     *  - Add new ROC1
     */
    sumROC1 -= Math.abs(trailingValue - tempReal2);
    sumROC1 += Math.abs(tempReal - sp.lag1_inReal);
    /* Once a whole window of flat bars has gone by, every 1-day change it
     * spans is exactly zero, so the sum is known to be exactly zero, the
     * residue can be dropped, and the efficiency ratio is 1.
     */
    live = (!(Math.abs(tempReal - sp.lag1_inReal) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
    if (nullRun >= sp.optInTimePeriod) {
      nullRun = sp.optInTimePeriod;
      sumROC1 = 0.0;
      tempReal = 1.0;
    } else if (sumROC1 <= 0.0 ||
        sumROC1 <= periodROC) /* Calculate the efficiency ratio */
    {
      tempReal = 1.0;
    } else {
      tempReal = Math.abs(periodROC / sumROC1);
      if (tempReal > 1.0) {
        tempReal = 1.0;
      }
    }
    /* Save the trailing value. Do this because inReal
     * and outReal can be pointers to the same buffer.
     */
    trailingValue = tempReal2;
    /* Calculate the smoothing constant */
    tempReal = fma(tempReal, sp.constDiff, sp.constMax);
    tempReal *= tempReal;
    /* Calculate the KAMA like an EMA, using the
     * smoothing constant as the adaptive factor.
     */
    prevKAMA = fma(inReal - prevKAMA, tempReal, prevKAMA);
    cur_outReal = prevKAMA;
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
  clone(): KamaStream {
    const c = new KamaStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.constMax = this.constMax;
    c.constDiff = this.constDiff;
    c.sumROC1 = this.sumROC1;
    c.prevKAMA = this.prevKAMA;
    c.nullRun = this.nullRun;
    c.trailingValue = this.trailingValue;
    c.lag1_inReal = this.lag1_inReal;
    c.ringPos_trailingIdx = this.ringPos_trailingIdx;
    c.ringCap_trailingIdx = this.ringCap_trailingIdx;
    c.ring_trailingIdx_inReal = this.ring_trailingIdx_inReal.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function kamaStepImpl(sp: KamaStream, inReal: number): void {
  let tempReal = 0;
  let tempReal2 = 0;
  let periodROC = 0;
  let live = 0;
  let ringCapL_trailingIdx = 0;
  if (sp.optInTimePeriod === 1) {
    sp.cur_outReal = inReal;
    return;
  }
  tempReal = inReal;
  tempReal2 = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
  periodROC = tempReal - tempReal2;
  /* Adjust sumROC1:
   *  - Remove trailing ROC1
   *  - Add new ROC1
   */
  sp.sumROC1 -= Math.abs(sp.trailingValue - tempReal2);
  sp.sumROC1 += Math.abs(tempReal - sp.lag1_inReal);
  /* Once a whole window of flat bars has gone by, every 1-day change it
   * spans is exactly zero, so the sum is known to be exactly zero, the
   * residue can be dropped, and the efficiency ratio is 1.
   */
  live = (!(Math.abs(tempReal - sp.lag1_inReal) <= 0.0) ? 1 : 0);
  sp.nullRun = (sp.nullRun + 1) & (live - 1);
  if (sp.nullRun >= sp.optInTimePeriod) {
    sp.nullRun = sp.optInTimePeriod;
    sp.sumROC1 = 0.0;
    tempReal = 1.0;
  } else if (sp.sumROC1 <= 0.0 ||
      sp.sumROC1 <= periodROC) /* Calculate the efficiency ratio */
  {
    tempReal = 1.0;
  } else {
    tempReal = Math.abs(periodROC / sp.sumROC1);
    if (tempReal > 1.0) {
      tempReal = 1.0;
    }
  }
  /* Save the trailing value. Do this because inReal
   * and outReal can be pointers to the same buffer.
   */
  sp.trailingValue = tempReal2;
  /* Calculate the smoothing constant */
  tempReal = fma(tempReal, sp.constDiff, sp.constMax);
  tempReal *= tempReal;
  /* Calculate the KAMA like an EMA, using the
   * smoothing constant as the adaptive factor.
   */
  sp.prevKAMA = fma(inReal - sp.prevKAMA, tempReal, sp.prevKAMA);
  sp.cur_outReal = sp.prevKAMA;
  sp.lag1_inReal = inReal;
  ringCapL_trailingIdx = sp.ringCap_trailingIdx;
  sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
  sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
  if (sp.ringPos_trailingIdx >= ringCapL_trailingIdx) {
    sp.ringPos_trailingIdx = 0;
  }
}

function kamaOpenImpl(
  sp: KamaStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let constMax = 0;
  let constDiff = 0;
  let tempReal = 0;
  let tempReal2 = 0;
  let sumROC1 = 0;
  let periodROC = 0;
  let prevKAMA = 0;
  let i = 0;
  let today = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let trailingIdx = 0;
  let nullRun = 0;
  let live = 0;
  let trailingValue = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (optInTimePeriod === 1) {
    let fillLb = kamaLookback(optInTimePeriod, core);
    if (startIdx > fillLb) {
      fillLb = startIdx;
    }
    if (historyLen < fillLb + 1) {
      return RetCode.INSUFFICIENT_HISTORY;
    }
    sp.optInTimePeriod = optInTimePeriod;
    sp.constMax = 0;
    sp.constDiff = 0;
    sp.sumROC1 = 0;
    sp.prevKAMA = 0;
    sp.nullRun = 0;
    sp.trailingValue = 0;
    sp.lag1_inReal = 0;
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
  constMax = 2.0 / (30.0 + 1.0);
  constDiff = 2.0 / (2.0 + 1.0) - constMax;
  /* Default return values */
  outBegIdx.value = 0;
  outNBElement.value = 0;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = kamaLookback(optInTimePeriod, core);
  /* Move up the start index if there is not
   * enough initial data.
   */
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Initialize the variables by going through
   * the lookback period.
   */
  sumROC1 = 0.0;
  /* Consecutive 1-day changes of exactly zero, counted so that a flat window
   * can be recognized exactly (the shape #244 needed for MFI). sumROC1 cannot
   * answer that question itself once the window starts sliding: it is
   * maintained by add-then-subtract, so a window that has gone flat leaves it
   * holding rounding residue of arbitrary sign rather than zero, and the
   * efficiency ratio then divides that residue into itself.
   */
  nullRun = 0;
  today = startIdx - lookbackTotal;
  trailingIdx = today;
  i = optInTimePeriod;
  while (i-- > 0) {
    tempReal = inReal[today++];
    tempReal -= inReal[today];
    sumROC1 += Math.abs(tempReal);
    live = (!(Math.abs(tempReal) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
  }
  /* At this point sumROC1 represent the
   * summation of the 1-day price difference
   * over the (optInTimePeriod-1)
   */
  /* Calculate the first KAMA */
  /* The yesterday price is used here as the previous KAMA. */
  prevKAMA = inReal[today - 1];
  tempReal = inReal[today];
  tempReal2 = inReal[trailingIdx++];
  periodROC = tempReal - tempReal2;
  /* Save the trailing value. Do this because inReal
   * and outReal can be pointers to the same buffer.
   */
  trailingValue = tempReal2;
  /* Calculate the efficiency ratio.
   *
   * The ratio cannot exceed 1 in exact arithmetic, but sumROC1 drifts, so
   * clamp it: past 1 the squared smoothing constant amplifies instead of
   * averaging and the recurrence below stops being a convex combination.
   *
   * `sumROC1 <= 0.0` (#385) is now numerically redundant -- the clamp maps its
   * +Inf onto the same 1.0 -- so a value test written against it would pass
   * with it deleted. Keep it anyway: the divisor sweep requires a division to
   * be dominated by a test of its own divisor against a literal zero.
   */
  if (sumROC1 <= 0.0 || sumROC1 <= periodROC) {
    tempReal = 1.0;
  } else {
    tempReal = Math.abs(periodROC / sumROC1);
    if (tempReal > 1.0) {
      tempReal = 1.0;
    }
  }
  /* Calculate the smoothing constant */
  tempReal = fma(tempReal, constDiff, constMax);
  tempReal *= tempReal;
  /* Calculate the KAMA like an EMA, using the
   * smoothing constant as the adaptive factor.
   */
  prevKAMA = fma(inReal[today++] - prevKAMA, tempReal, prevKAMA);
  /* 'today' keep track of where the processing is within the
   * input.
   */
  /* Skip the unstable period. Do the whole processing
   * needed for KAMA, but do not write it in the output.
   */
  while (today <= startIdx) {
    tempReal = inReal[today];
    tempReal2 = inReal[trailingIdx++];
    periodROC = tempReal - tempReal2;
    /* Adjust sumROC1:
     *  - Remove trailing ROC1
     *  - Add new ROC1
     */
    sumROC1 -= Math.abs(trailingValue - tempReal2);
    sumROC1 += Math.abs(tempReal - inReal[today - 1]);
    /* Once a whole window of flat bars has gone by, every 1-day change it
     * spans is exactly zero, so the sum is known to be exactly zero, the
     * residue can be dropped, and the efficiency ratio is 1.
     */
    live = (!(Math.abs(tempReal - inReal[today - 1]) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
    if (nullRun >= optInTimePeriod) {
      nullRun = optInTimePeriod;
      sumROC1 = 0.0;
      tempReal = 1.0;
    } else if (sumROC1 <= 0.0 ||
        sumROC1 <= periodROC) /* Calculate the efficiency ratio */
    {
      tempReal = 1.0;
    } else {
      tempReal = Math.abs(periodROC / sumROC1);
      if (tempReal > 1.0) {
        tempReal = 1.0;
      }
    }
    /* Save the trailing value. Do this because inReal
     * and outReal can be pointers to the same buffer.
     */
    trailingValue = tempReal2;
    /* Calculate the smoothing constant */
    {
      const _a: number = tempReal, _b: number = constDiff, _c: number = constMax;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      tempReal = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    tempReal *= tempReal;
    /* Calculate the KAMA like an EMA, using the
     * smoothing constant as the adaptive factor.
     */
    {
      const _a: number = inReal[today++] - prevKAMA, _b: number = tempReal, _c: number = prevKAMA;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevKAMA = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
  }
  /* Write the first value. */
  outReal[((0 * outStride) | 0)] = prevKAMA;
  outIdx = 1;
  outBegIdx.value = today - 1;
  /* Do the KAMA calculation for the requested range. */
  while (today <= endIdx) {
    tempReal = inReal[today];
    tempReal2 = inReal[trailingIdx++];
    periodROC = tempReal - tempReal2;
    /* Adjust sumROC1:
     *  - Remove trailing ROC1
     *  - Add new ROC1
     */
    sumROC1 -= Math.abs(trailingValue - tempReal2);
    sumROC1 += Math.abs(tempReal - inReal[today - 1]);
    /* Once a whole window of flat bars has gone by, every 1-day change it
     * spans is exactly zero, so the sum is known to be exactly zero, the
     * residue can be dropped, and the efficiency ratio is 1.
     */
    live = (!(Math.abs(tempReal - inReal[today - 1]) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
    if (nullRun >= optInTimePeriod) {
      nullRun = optInTimePeriod;
      sumROC1 = 0.0;
      tempReal = 1.0;
    } else if (sumROC1 <= 0.0 ||
        sumROC1 <= periodROC) /* Calculate the efficiency ratio */
    {
      tempReal = 1.0;
    } else {
      tempReal = Math.abs(periodROC / sumROC1);
      if (tempReal > 1.0) {
        tempReal = 1.0;
      }
    }
    /* Save the trailing value. Do this because inReal
     * and outReal can be pointers to the same buffer.
     */
    trailingValue = tempReal2;
    /* Calculate the smoothing constant */
    {
      const _a: number = tempReal, _b: number = constDiff, _c: number = constMax;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      tempReal = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    tempReal *= tempReal;
    /* Calculate the KAMA like an EMA, using the
     * smoothing constant as the adaptive factor.
     */
    {
      const _a: number = inReal[today++] - prevKAMA, _b: number = tempReal, _c: number = prevKAMA;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevKAMA = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    outReal[((outIdx++ * outStride) | 0)] = prevKAMA;
  }
  outNBElement.value = outIdx;
  const cap_trailingIdx = today - trailingIdx;
  if (cap_trailingIdx < 1 || cap_trailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_trailingIdx = cap_trailingIdx > 0 ? cap_trailingIdx : 1;
  const capRing_trailingIdx_inReal = new Float64Array(allocN_trailingIdx);
  capRing_trailingIdx_inReal.set(inReal.subarray(historyLen - cap_trailingIdx, historyLen), 0);
  sp.optInTimePeriod = optInTimePeriod;
  sp.constMax = constMax;
  sp.constDiff = constDiff;
  sp.sumROC1 = sumROC1;
  sp.prevKAMA = prevKAMA;
  sp.nullRun = nullRun;
  sp.trailingValue = trailingValue;
  sp.lag1_inReal = inReal[historyLen - 1];
  sp.ringPos_trailingIdx = 0;
  sp.ringCap_trailingIdx = cap_trailingIdx;
  sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function kamaOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): KamaStream {
  const sp = KamaStream.of(core);
  const retCode = kamaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('KAMA openAndFill', inReal.length, startIdx, kamaLookback(optInTimePeriod, core));
  }
  throw streamFailure('KAMA openAndFill', retCode);
}

/** @internal */
export function kamaOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): KamaStream {
  const sp = KamaStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = kamaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('KAMA open', inReal.length, startIdx, kamaLookback(optInTimePeriod, core));
  }
  throw streamFailure('KAMA open', retCode);
}

/**
 * Opens a live KAMA stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link kama} there.
 *
 * @param inReal - Warm-up history. Source price series.
 * @param optInTimePeriod - Lookback window for the efficiency ratio (default 30;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `kamaLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32 or a
 *   parameter outside its range.
 *
 * @example
 * ```ts
 * import { kamaOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = kamaOpen(data, 30);
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function kamaOpen(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): KamaStream {
  optInTimePeriod = requireInt32('KAMA open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('KAMA open', 'inReal', inReal);
  requireHistory('KAMA open', inReal.length);
  return kamaOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/**
 * {@link kamaOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link kama}. The range written is the stream's
 * `outRange()`.
 *
 * @param inReal - Warm-up history. Source price series.
 * @param optInTimePeriod - Lookback window for the efficiency ratio (default 30;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Adaptive moving average line. Must hold `historyLen - lookback`
 *   values and share no memory with an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `kamaLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or an output too short or starting where another
 *   output or a `Float64Array` input starts, or, on a history long enough to open,
 *   one sharing memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { kamaOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const stream = kamaOpenAndFill(data, 30, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function kamaOpenAndFill(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): KamaStream {
  optInTimePeriod = requireInt32('KAMA openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('KAMA openAndFill', 'inReal', inReal);
  requireHistory('KAMA openAndFill', inReal.length);
  const guardOutLen = openFillCount('KAMA openAndFill', inReal.length, kamaLookback(optInTimePeriod, core));
  requireOutput('KAMA openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if ((outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset)) {
    throw streamFailure('KAMA openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inRealF64))) {
    throw streamFailure('KAMA openAndFill', RetCode.BAD_PARAM);
  }
  return kamaOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/** @internal */
export function kamaStepTape(sp: KamaStream, tape: Float64Array, tapeBase: number, tapeMask: number, inReal: number): number {
  let tempReal = 0;
  let tempReal2 = 0;
  let periodROC = 0;
  let live = 0;
  tempReal = inReal;
  tempReal2 = tape[(tapeBase - sp.ringCap_trailingIdx) & tapeMask];
  periodROC = tempReal - tempReal2;
  /* Adjust sumROC1:
   *  - Remove trailing ROC1
   *  - Add new ROC1
   */
  sp.sumROC1 -= Math.abs(sp.trailingValue - tempReal2);
  sp.sumROC1 += Math.abs(tempReal - sp.lag1_inReal);
  /* Once a whole window of flat bars has gone by, every 1-day change it
   * spans is exactly zero, so the sum is known to be exactly zero, the
   * residue can be dropped, and the efficiency ratio is 1.
   */
  live = (!(Math.abs(tempReal - sp.lag1_inReal) <= 0.0) ? 1 : 0);
  sp.nullRun = (sp.nullRun + 1) & (live - 1);
  if (sp.nullRun >= sp.optInTimePeriod) {
    sp.nullRun = sp.optInTimePeriod;
    sp.sumROC1 = 0.0;
    tempReal = 1.0;
  } else if (sp.sumROC1 <= 0.0 ||
      sp.sumROC1 <= periodROC) /* Calculate the efficiency ratio */
  {
    tempReal = 1.0;
  } else {
    tempReal = Math.abs(periodROC / sp.sumROC1);
    if (tempReal > 1.0) {
      tempReal = 1.0;
    }
  }
  /* Save the trailing value. Do this because inReal
   * and outReal can be pointers to the same buffer.
   */
  sp.trailingValue = tempReal2;
  /* Calculate the smoothing constant */
  tempReal = fma(tempReal, sp.constDiff, sp.constMax);
  tempReal *= tempReal;
  /* Calculate the KAMA like an EMA, using the
   * smoothing constant as the adaptive factor.
   */
  sp.prevKAMA = fma(inReal - sp.prevKAMA, tempReal, sp.prevKAMA);
  sp.cur_outReal = sp.prevKAMA;
  sp.lag1_inReal = inReal;
  sp.outRangeCount++;
  return sp.cur_outReal;
}

/** @internal */
export function kamaPeekTape(sp: KamaStream, tape: Float64Array, tapeBase: number, tapeMask: number, inReal: number): number {
  let tempReal = 0;
  let tempReal2 = 0;
  let periodROC = 0;
  let live = 0;
  let cur_outReal = 0;
  let nullRun = sp.nullRun;
  let prevKAMA = sp.prevKAMA;
  let sumROC1 = sp.sumROC1;
  let trailingValue = sp.trailingValue;
  tempReal = inReal;
  tempReal2 = tape[(tapeBase - sp.ringCap_trailingIdx) & tapeMask];
  periodROC = tempReal - tempReal2;
  /* Adjust sumROC1:
   *  - Remove trailing ROC1
   *  - Add new ROC1
   */
  sumROC1 -= Math.abs(trailingValue - tempReal2);
  sumROC1 += Math.abs(tempReal - sp.lag1_inReal);
  /* Once a whole window of flat bars has gone by, every 1-day change it
   * spans is exactly zero, so the sum is known to be exactly zero, the
   * residue can be dropped, and the efficiency ratio is 1.
   */
  live = (!(Math.abs(tempReal - sp.lag1_inReal) <= 0.0) ? 1 : 0);
  nullRun = (nullRun + 1) & (live - 1);
  if (nullRun >= sp.optInTimePeriod) {
    nullRun = sp.optInTimePeriod;
    sumROC1 = 0.0;
    tempReal = 1.0;
  } else if (sumROC1 <= 0.0 ||
      sumROC1 <= periodROC) /* Calculate the efficiency ratio */
  {
    tempReal = 1.0;
  } else {
    tempReal = Math.abs(periodROC / sumROC1);
    if (tempReal > 1.0) {
      tempReal = 1.0;
    }
  }
  /* Save the trailing value. Do this because inReal
   * and outReal can be pointers to the same buffer.
   */
  trailingValue = tempReal2;
  /* Calculate the smoothing constant */
  tempReal = fma(tempReal, sp.constDiff, sp.constMax);
  tempReal *= tempReal;
  /* Calculate the KAMA like an EMA, using the
   * smoothing constant as the adaptive factor.
   */
  prevKAMA = fma(inReal - prevKAMA, tempReal, prevKAMA);
  cur_outReal = prevKAMA;
  return cur_outReal;
}

/** @internal */
export function kamaTapeDetach(sp: KamaStream): number {
  let reach = 0;
  sp.ring_trailingIdx_inReal = new Float64Array(0);
  if (sp.ringCap_trailingIdx > reach) {
    reach = sp.ringCap_trailingIdx;
  }
  return reach;
}
