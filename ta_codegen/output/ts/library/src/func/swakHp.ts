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
import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function swakHpLookbackPositional(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
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

/** @internal */
export function swakHpDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (swakHpLookbackPositional(optInTimePeriod, core) < 0) {
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
  lookbackTotal = swakHpLookbackPositional(optInTimePeriod, core);
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

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 5, 100000, 'in 5..100000']] as const;

/** @internal */
export function swakHpPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('SWAK_HP', 'startIdx', startIdx);
  endIdx = requireInt32('SWAK_HP', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('SWAK_HP', 'timePeriod', optInTimePeriod);
  requireIndexRange('SWAK_HP', startIdx, endIdx);
  const guardLookback = swakHpLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('SWAK_HP', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('SWAK_HP', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('SWAK_HP', 'real', inReal, guardInLen);
  requireOutput('SWAK_HP', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('SWAK_HP', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = swakHpImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('SWAK_HP', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link swakHp}, {@link swakHpLookback}, {@link swakHpDisplayShift},
 * {@link swakHpOpen} and {@link swakHpOpenAndFill}. Every key is optional: an
 * omitted or `undefined` key takes its default, and a call ignores the keys it has
 * no use for.
 */
export interface SwakHpOptions {
  /**
   * Cutoff period; cycles longer than it are removed, cycles shorter pass (default
   * 20; range 5 to 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  timePeriod?: number | undefined;
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

/** What {@link swakHp} returns, and {@link swakHpOpenAndFill} with its stream. */
export interface SwakHpResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * The detrended line, centred on zero. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link swakHp} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SwakHpOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function swakHpLookback(options?: SwakHpOptions): number {
  const o = optionsOf('SWAK_HP lookback', options);
  return swakHpLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('SWAK_HP lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link swakHp}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SwakHpOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function swakHpDisplayShift(outputIdx: number, options?: SwakHpOptions): number {
  const o = optionsOf('SWAK_HP displayShift', options);
  return swakHpDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('SWAK_HP displayShift', o.core));
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
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link swakHpLookback} is a success with empty outputs.
 *
 * @param real - The series to detrend; Ehlers' default is the bar midpoint
 *   `(H+L)/2`, which the caller passes as `TA_MEDPRICE` output.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link SwakHpOptions}.
 * @returns `begIdx` and the values; see {@link SwakHpResult}. Each output is a
 *   view of its buffer, the caller's or a new one.
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
 * import { swakHp } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = swakHp(data, { timePeriod: 20 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`swak2php`](https://ta-lib.org/functions/swak_2php)
 * @see [`swakBp`](https://ta-lib.org/functions/swak_bp)
 * @see [`medprice`](https://ta-lib.org/functions/medprice)
 */
export function swakHp(real: ArrayLike<number>, options?: SwakHpOptions): SwakHpResult {
  const o = optionsOf('SWAK_HP', options);
  if (o.endIdx === undefined) {
    requireSeries('SWAK_HP', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('SWAK_HP', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, swakHpLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = swakHpPositional(startIdx, endIdx, real, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
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
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 5 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = swakHpLookbackPositional(optInTimePeriod, core);
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
    throw insufficientHistory('SWAK_HP openAndFill', inReal.length, startIdx, swakHpLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && swakHpLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('SWAK_HP openAndFill', PARAM_RANGES, optInTimePeriod);
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
    throw insufficientHistory('SWAK_HP open', inReal.length, startIdx, swakHpLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && swakHpLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('SWAK_HP open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('SWAK_HP open', retCode);
}

/** @internal */
export function swakHpOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): SwakHpStream {
  optInTimePeriod = requireInt32('SWAK_HP open', 'timePeriod', optInTimePeriod);
  requireArgument('SWAK_HP open', 'real', inReal);
  requireHistory('SWAK_HP open', inReal.length);
  return swakHpOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/** @internal */
export function swakHpOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): SwakHpStream {
  optInTimePeriod = requireInt32('SWAK_HP openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('SWAK_HP openAndFill', 'real', inReal);
  requireHistory('SWAK_HP openAndFill', inReal.length);
  const guardLookback = swakHpLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('SWAK_HP openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('SWAK_HP openAndFill', inReal.length, guardLookback);
  requireOutput('SWAK_HP openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('SWAK_HP openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('SWAK_HP openAndFill', 'outReal', 'real');
  }
  return swakHpOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live SWAK_HP stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link swakHp} there.
 *
 * @param real - Warm-up history. The series to detrend; Ehlers' default is the bar
 *   midpoint `(H+L)/2`, which the caller passes as `TA_MEDPRICE` output.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SwakHpOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `swakHpLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32 or a parameter outside its
 *   range.
 *
 * @example
 * ```ts
 * import { swakHpOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = swakHpOpen(data, { timePeriod: 20 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function swakHpOpen(real: ArrayLike<number>, options?: SwakHpOptions): SwakHpStream {
  const o = optionsOf('SWAK_HP open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('SWAK_HP open', o.core);
  return swakHpOpenPositional(real, optInTimePeriod, core);
}

/**
 * {@link swakHpOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link swakHp}.
 *
 * @param real - Warm-up history. The series to detrend; Ehlers' default is the bar
 *   midpoint `(H+L)/2`, which the caller passes as `TA_MEDPRICE` output.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link SwakHpOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link swakHp} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `swakHpLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range
 *   or a buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { swakHpOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = swakHpOpenAndFill(data, { timePeriod: 20 });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function swakHpOpenAndFill(real: ArrayLike<number>, options?: SwakHpOptions): SwakHpResult & { stream: SwakHpStream } {
  const o = optionsOf('SWAK_HP openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('SWAK_HP openAndFill', o.core);
  const capacity = fillCapacity(swakHpLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = swakHpOpenAndFillPositional(real, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
