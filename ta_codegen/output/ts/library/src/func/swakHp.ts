/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  100126 KL,CC  Creation (#486).
 *  100326 MF,CC  The newest output on one fused step (#486).
 *  100526 MF,CC  a1p without its cancellation at long periods (#486).
 */

import { FuncUnstId } from '../enums.ts';
import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link swakHp} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param optInTimePeriod - Cutoff period; cycles longer than it are removed,
 *   cycles shorter pass (default 20; range 5 to 100000; `Core.INTEGER_DEFAULT`
 *   selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function swakHpLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 5 || optInTimePeriod > 100000) {
    return -1;
  }
  /* No structural lookback: the one input slot and the one output slot are
   * seeded from the first bar rather than read from before it, and there is
   * no callee whose lookback could be inherited.
   */
  return core.unstableCount(FuncUnstId.SWAK_HP, (((((10 * optInTimePeriod) | 0) + 5) / 6) | 0), (((((19 * optInTimePeriod) | 0) + 5) / 6) | 0));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link swakHp}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Cutoff period; cycles longer than it are removed,
 *   cycles shorter pass (default 20; range 5 to 100000; `Core.INTEGER_DEFAULT`
 *   selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function swakHpDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (swakHpLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function swakHpImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let i = 0;
  let outIdx = 0;
  let today = 0;
  let lookbackTotal = 0;
  let h = 0;
  let sh = 0;
  let ch = 0;
  let a1p = 0;
  let c0 = 0;
  let a1 = 0;
  let x0 = 0;
  let x1 = 0;
  let y = 0;
  let y1 = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 5 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = swakHpLookback(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  /* The one-pole alpha (Ehlers, "Swiss Army Knife Indicator", Figure 5).
   * The paper says this one "is computed exactly the same as it is for the
   * EMA" -- meaning the cutoff-period formula below, not TA_EMA's
   * 2/(n+1). The paper's 360/P is a full turn, so 2*pi/P.
   *
   * The period range starts at 5 because of what this expression does below
   * it, not for taste: at P = 4 it is 0/0, and what the doubles make of that
   * is no filter. At P = 2, cos(w) is -1 and c0 is 0, a dead filter. No
   * contiguous range below 5 avoids both.
   *
   * Keep it in the half angle h = w/2, where it is 2*sin h*(cos h - sin h)
   * over 1 - 2*sin(h)^2. Written in w, cos(w) - 1 cancels more of the
   * numerator's digits the longer the period, and the cutoff drifts with
   * them.
   */
  h = 3.141592653589793 / optInTimePeriod;
  sh = Math.sin(h);
  ch = Math.cos(h);
  a1p = 2.0 * sh * (ch - sh) / (1.0 - 2.0 * sh * sh);
  /* The high-pass row: a (1, -1) numerator, so its DC gain is 0 and the line
   * is centred on zero rather than on price.
   */
  c0 = 1.0 - a1p / 2.0;
  a1 = 1.0 - a1p;
  today = startIdx - lookbackTotal;
  /* Start in the steady state of a constant input equal to the first bar: a
   * DC-gain-0 row answers 0 for a constant, so the output slot starts there
   * while the input slot holds the bar. The input slot is carried in a local
   * and never re-read from inReal, because outReal may alias it.
   */
  x1 = inReal[today];
  y1 = 0.0;
  /* Skip the unstable period: run the recurrence but publish nothing. */
  i = lookbackTotal;
  while (i !== 0) {
    x0 = inReal[today];
    y = fma(a1, y1, c0 * (x0 - x1));
    x1 = x0;
    y1 = y;
    today += 1;
    i -= 1;
  }
  outIdx = 0;
  while (today <= endIdx) {
    /* a1*y1 stays the outermost term. y1 is the newest output, so the bar
     * rate is the latency of whatever y1 crosses to become y: outermost, that
     * is one fused step. Nested inside, it is two, and every backend's last
     * bit moves with it.
     */
    x0 = inReal[today];
    y = fma(a1, y1, c0 * (x0 - x1));
    x1 = x0;
    y1 = y;
    outReal[outIdx] = y;
    outIdx = outIdx + 1;
    today += 1;
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/**
 * The high-pass row of John Ehlers' Swiss Army Knife filter: a one-pole detrender
 * that removes what is slower than the cutoff period and keeps what is faster.
 * Read it as an oscillator, not as price. Its DC gain is 0, so a flat market
 * returns zero and a trending one returns the trend's departure from itself rather
 * than its level. That is the point of a detrender: what remains is the cyclic
 * part of the series, centred on zero, which can then be measured or compared
 * across instruments whose price levels differ by orders of magnitude. The alpha
 * is the cutoff-period one. Ehlers notes that it "is computed exactly the same as
 * it is for the EMA", meaning the same construction, not `TA_EMA`'s `2/(n+1)`.
 *
 * Formula and more info at
 * [ta-lib.org/functions/swak_hp](https://ta-lib.org/functions/swak_hp).
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link swakHpLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - The series to detrend; Ehlers' default is the bar midpoint
 *   `(H+L)/2`, which the caller passes as `TA_MEDPRICE` output.
 * @param optInTimePeriod - Cutoff period; cycles longer than it are removed,
 *   cycles shorter pass (default 20; range 5 to 100000; `Core.INTEGER_DEFAULT`
 *   selects the default).
 * @param outReal - The detrended line, centred on zero. Must hold the values
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
 * import { swakHp } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const { begIdx, count } = swakHp(0, data.length - 1, data, 20, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`swak2php`](https://ta-lib.org/functions/swak_2php)
 * @see [`swakBp`](https://ta-lib.org/functions/swak_bp)
 * @see [`medprice`](https://ta-lib.org/functions/medprice)
 */
export function swakHp(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('SWAK_HP', 'startIdx', startIdx);
  endIdx = requireInt32('SWAK_HP', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('SWAK_HP', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('SWAK_HP', startIdx, endIdx);
  const guardStart = clampedStart('SWAK_HP', startIdx, swakHpLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('SWAK_HP', 'inReal', inReal, guardInLen);
  requireOutput('SWAK_HP', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw failure('SWAK_HP', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = swakHpImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('SWAK_HP', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live SWAK_HP stream: one value per closed bar, bit-identical to {@link swakHp}
 * over the same series. Open it with {@link swakHpOpen} or
 * {@link swakHpOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/swak_hp](https://ta-lib.org/functions/swak_hp).
 */
export class SwakHpStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  c0: number = 0;
  /** @internal */
  a1: number = 0;
  /** @internal */
  x1: number = 0;
  /** @internal */
  y1: number = 0;
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
  static of(core: Core): SwakHpStream {
    return new SwakHpStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link swakHp} reports them over the same bars. Each accepted `update` and each
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
      throw failure('SWAK_HP advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: The series to detrend; Ehlers' default
   *   is the bar midpoint `(H+L)/2`, which the caller passes as `TA_MEDPRICE` output.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('SWAK_HP update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('SWAK_HP update', 'inReal');
    }
    swakHpStepImpl(this, inReal);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: The series to detrend; Ehlers' default
   *   is the bar midpoint `(H+L)/2`, which the caller passes as `TA_MEDPRICE` output.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number): number {
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('SWAK_HP peek', 'inReal');
    }
    const sp = this;
    let x0 = 0;
    let y = 0;
    let cur_outReal = 0;
    let x1 = sp.x1;
    let y1 = sp.y1;
    /* a1*y1 stays the outermost term. y1 is the newest output, so the bar
     * rate is the latency of whatever y1 crosses to become y: outermost, that
     * is one fused step. Nested inside, it is two, and every backend's last
     * bit moves with it.
     */
    x0 = inReal;
    y = fma(sp.a1, y1, sp.c0 * (x0 - x1));
    x1 = x0;
    y1 = y;
    cur_outReal = y;
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
  clone(): SwakHpStream {
    const c = new SwakHpStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.c0 = this.c0;
    c.a1 = this.a1;
    c.x1 = this.x1;
    c.y1 = this.y1;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function swakHpStepImpl(sp: SwakHpStream, inReal: number): void {
  let x0 = 0;
  let y = 0;
  /* a1*y1 stays the outermost term. y1 is the newest output, so the bar
   * rate is the latency of whatever y1 crosses to become y: outermost, that
   * is one fused step. Nested inside, it is two, and every backend's last
   * bit moves with it.
   */
  x0 = inReal;
  y = fma(sp.a1, sp.y1, sp.c0 * (x0 - sp.x1));
  sp.x1 = x0;
  sp.y1 = y;
  sp.cur_outReal = y;
}

function swakHpOpenImpl(
  sp: SwakHpStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let i = 0;
  let outIdx = 0;
  let today = 0;
  let lookbackTotal = 0;
  let h = 0;
  let sh = 0;
  let ch = 0;
  let a1p = 0;
  let c0 = 0;
  let a1 = 0;
  let x0 = 0;
  let x1 = 0;
  let y = 0;
  let y1 = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 5 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = swakHpLookback(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* The one-pole alpha (Ehlers, "Swiss Army Knife Indicator", Figure 5).
   * The paper says this one "is computed exactly the same as it is for the
   * EMA" -- meaning the cutoff-period formula below, not TA_EMA's
   * 2/(n+1). The paper's 360/P is a full turn, so 2*pi/P.
   *
   * The period range starts at 5 because of what this expression does below
   * it, not for taste: at P = 4 it is 0/0, and what the doubles make of that
   * is no filter. At P = 2, cos(w) is -1 and c0 is 0, a dead filter. No
   * contiguous range below 5 avoids both.
   *
   * Keep it in the half angle h = w/2, where it is 2*sin h*(cos h - sin h)
   * over 1 - 2*sin(h)^2. Written in w, cos(w) - 1 cancels more of the
   * numerator's digits the longer the period, and the cutoff drifts with
   * them.
   */
  h = 3.141592653589793 / optInTimePeriod;
  sh = Math.sin(h);
  ch = Math.cos(h);
  a1p = 2.0 * sh * (ch - sh) / (1.0 - 2.0 * sh * sh);
  /* The high-pass row: a (1, -1) numerator, so its DC gain is 0 and the line
   * is centred on zero rather than on price.
   */
  c0 = 1.0 - a1p / 2.0;
  a1 = 1.0 - a1p;
  today = startIdx - lookbackTotal;
  /* Start in the steady state of a constant input equal to the first bar: a
   * DC-gain-0 row answers 0 for a constant, so the output slot starts there
   * while the input slot holds the bar. The input slot is carried in a local
   * and never re-read from inReal, because outReal may alias it.
   */
  x1 = inReal[today];
  y1 = 0.0;
  /* Skip the unstable period: run the recurrence but publish nothing. */
  i = lookbackTotal;
  while (i !== 0) {
    x0 = inReal[today];
    y = fma(a1, y1, c0 * (x0 - x1));
    x1 = x0;
    y1 = y;
    today += 1;
    i -= 1;
  }
  outIdx = 0;
  while (today <= endIdx) {
    /* a1*y1 stays the outermost term. y1 is the newest output, so the bar
     * rate is the latency of whatever y1 crosses to become y: outermost, that
     * is one fused step. Nested inside, it is two, and every backend's last
     * bit moves with it.
     */
    x0 = inReal[today];
    y = fma(a1, y1, c0 * (x0 - x1));
    x1 = x0;
    y1 = y;
    outReal[((outIdx * outStride) | 0)] = y;
    outIdx = outIdx + 1;
    today += 1;
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  sp.optInTimePeriod = optInTimePeriod;
  sp.c0 = c0;
  sp.a1 = a1;
  sp.x1 = x1;
  sp.y1 = y1;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function swakHpOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): SwakHpStream {
  const sp = SwakHpStream.of(core);
  const retCode = swakHpOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SWAK_HP openAndFill', inReal.length, startIdx, swakHpLookback(optInTimePeriod, core));
  }
  throw streamFailure('SWAK_HP openAndFill', retCode);
}

/** @internal */
export function swakHpOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): SwakHpStream {
  const sp = SwakHpStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = swakHpOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SWAK_HP open', inReal.length, startIdx, swakHpLookback(optInTimePeriod, core));
  }
  throw streamFailure('SWAK_HP open', retCode);
}

/**
 * Opens a live SWAK_HP stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link swakHp} there.
 *
 * @param inReal - Warm-up history. The series to detrend; Ehlers' default is the
 *   bar midpoint `(H+L)/2`, which the caller passes as `TA_MEDPRICE` output.
 * @param optInTimePeriod - Cutoff period; cycles longer than it are removed,
 *   cycles shorter pass (default 20; range 5 to 100000; `Core.INTEGER_DEFAULT`
 *   selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `swakHpLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { swakHpOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = swakHpOpen(data, 20);
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function swakHpOpen(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): SwakHpStream {
  optInTimePeriod = requireInt32('SWAK_HP open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('SWAK_HP open', 'inReal', inReal);
  requireHistory('SWAK_HP open', inReal.length);
  return swakHpOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/**
 * {@link swakHpOpen} that also fills the outputs over the whole history in the
 * same pass, bit-identically to {@link swakHp}. The range written is the stream's
 * `outRange()`.
 *
 * @param inReal - Warm-up history. The series to detrend; Ehlers' default is the
 *   bar midpoint `(H+L)/2`, which the caller passes as `TA_MEDPRICE` output.
 * @param optInTimePeriod - Cutoff period; cycles longer than it are removed,
 *   cycles shorter pass (default 20; range 5 to 100000; `Core.INTEGER_DEFAULT`
 *   selects the default).
 * @param outReal - The detrended line, centred on zero. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `swakHpLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for an integer parameter that is not an int32, a parameter outside its range or
 *   an output too short or starting where another output or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { swakHpOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const stream = swakHpOpenAndFill(data, 20, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function swakHpOpenAndFill(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): SwakHpStream {
  optInTimePeriod = requireInt32('SWAK_HP openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('SWAK_HP openAndFill', 'inReal', inReal);
  requireHistory('SWAK_HP openAndFill', inReal.length);
  const guardOutLen = openFillCount('SWAK_HP openAndFill', inReal.length, swakHpLookback(optInTimePeriod, core));
  requireOutput('SWAK_HP openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if ((outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset)) {
    throw streamFailure('SWAK_HP openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inRealF64))) {
    throw streamFailure('SWAK_HP openAndFill', RetCode.BAD_PARAM);
  }
  return swakHpOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}
