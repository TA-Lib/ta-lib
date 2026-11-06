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
 *  092726 MF,CC  Initial version (#451).
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireReal, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function asiLookbackPositional(optInLimitMove: number, _core: Core = Core.DEFAULT): number {
  if (typeof optInLimitMove !== 'number') {
    return -1;
  }
  if (optInLimitMove === Core.REAL_DEFAULT) {
    optInLimitMove = 3.0;
  } else if (!(optInLimitMove >= 1e-8 && optInLimitMove <= Core.REAL_MAX)) {
    return -1;
  }
  return 0;
}

/** @internal */
export function asiDisplayShiftPositional(optInLimitMove: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (asiLookbackPositional(optInLimitMove, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function asiImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  optInLimitMove: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let i = 0;
  let outIdx = 0;
  let prevClose = 0;
  let prevBody = 0;
  let tempOpen = 0;
  let tempHigh = 0;
  let tempLow = 0;
  let tempClose = 0;
  let body = 0;
  let n = 0;
  let up = 0;
  let dn = 0;
  let rg = 0;
  let k = 0;
  let r = 0;
  let swing = 0;
  let sum = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInLimitMove === Core.REAL_DEFAULT) {
    optInLimitMove = 3.0;
  } else if (!(optInLimitMove >= 1e-8 && optInLimitMove <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  /* SI's per-bar statements, kept identical: the regression test holds this
   * function bit for bit to CUMSUM over SI. Bar startIdx is read before the
   * anchor store, which may overwrite it when outReal aliases an input.
   */
  prevClose = inClose[startIdx];
  prevBody = prevClose - inOpen[startIdx];
  sum = 0.0;
  outReal[0] = sum;
  outIdx = 1;
  for (i = startIdx + 1; i <= endIdx; i += 1) {
    tempOpen = inOpen[i];
    tempHigh = inHigh[i];
    tempLow = inLow[i];
    tempClose = inClose[i];
    body = tempClose - tempOpen;
    n = tempClose - prevClose;
    n += 0.5 * body;
    n += 0.25 * prevBody;
    up = Math.abs(tempHigh - prevClose);
    dn = Math.abs(tempLow - prevClose);
    rg = Math.abs(tempHigh - tempLow);
    k = (up > dn ? up : dn);
    r = ((up - 0.5 * dn) > (dn - 0.5 * up) ? (up - 0.5 * dn) : (dn - 0.5 * up));
    r = (r > rg ? r : rg);
    r += 0.25 * Math.abs(prevBody);
    if (r === 0.0) {
      swing = 0.0;
    } else {
      swing = 50.0 * (n / r) * (k / optInLimitMove);
    }
    sum += swing;
    outReal[outIdx] = sum;
    outIdx += 1;
    prevClose = tempClose;
    prevBody = body;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['limitMove', Core.REAL_DEFAULT, 1e-8, Core.REAL_MAX, 'in 0.00000001..3e37']] as const;

/** @internal */
export function asiPositional(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInLimitMove: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('ASI', 'startIdx', startIdx);
  endIdx = requireInt32('ASI', 'endIdx', endIdx);
  optInLimitMove = requireReal('ASI', 'limitMove', optInLimitMove);
  requireIndexRange('ASI', startIdx, endIdx);
  const guardLookback = asiLookbackPositional(optInLimitMove, core);
  if (guardLookback < 0) {
    throw rejectedParams('ASI', PARAM_RANGES, optInLimitMove);
  }
  const guardStart = clampedStart('ASI', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('ASI', 'prices.open', inOpen, guardInLen);
  requireLength('ASI', 'prices.high', inHigh, guardInLen);
  requireLength('ASI', 'prices.low', inLow, guardInLen);
  requireLength('ASI', 'prices.close', inClose, guardInLen);
  requireOutput('ASI', 'outReal', outReal, guardOutLen, Float64Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outReal, inOpenF64)) {
    throw sharedMemory('ASI', 'outReal', 'prices.open');
  }
  if (overlapsPartly(outReal, inHighF64)) {
    throw sharedMemory('ASI', 'outReal', 'prices.high');
  }
  if (overlapsPartly(outReal, inLowF64)) {
    throw sharedMemory('ASI', 'outReal', 'prices.low');
  }
  if (overlapsPartly(outReal, inCloseF64)) {
    throw sharedMemory('ASI', 'outReal', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = asiImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, optInLimitMove, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('ASI', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link asi}, {@link asiLookback}, {@link asiDisplayShift},
 * {@link asiOpen} and {@link asiOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface AsiOptions {
  /**
   * Limit move, the largest one-bar price move the index is scaled against, in price
   * units (default 3; minimum 0.00000001; `Core.REAL_DEFAULT` selects the default).
   */
  limitMove?: number | undefined;
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

/** What {@link asi} returns, and {@link asiOpenAndFill} with its stream. */
export interface AsiResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Running total of the swing index from the first bar of the range. `values[k]` is
   * the value at input bar `begIdx + k`.
   */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link asi} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AsiOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function asiLookback(options?: AsiOptions): number {
  const o = optionsOf('ASI lookback', options);
  return asiLookbackPositional(orDefault(o.limitMove, Core.REAL_DEFAULT), coreOf('ASI lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link asi}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AsiOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function asiDisplayShift(outputIdx: number, options?: AsiOptions): number {
  const o = optionsOf('ASI displayShift', options);
  return asiDisplayShiftPositional(orDefault(o.limitMove, Core.REAL_DEFAULT), outputIdx, coreOf('ASI displayShift', o.core));
}

/**
 * Wilder's Accumulative Swing Index: the running total of the Swing Index (SI), a
 * line meant to trace the market's real swings through the noise of the daily
 * closes. Wilder draws trendlines and breakout points on it as on a price chart: a
 * breakout of the ASI that the price has not confirmed yet is his signal. J.
 * Welles Wilder Jr. introduced both in 1978. The line starts at 0 on the first bar
 * of the requested range, so only its shape carries meaning, not its level.
 *
 * Formula and more info at
 * [ta-lib.org/functions/asi](https://ta-lib.org/functions/asi).
 *
 * **Notes**
 * - Wilder accumulates Swing Index values rounded to whole numbers; the running
 *   total here is of the unrounded values.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link asiLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `open`: Open
 *   price of each bar. `high`: High price of each bar. `low`: Low price of each bar.
 *   `close`: Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link AsiOptions}.
 * @returns `begIdx` and the values; see {@link AsiResult}. Each output is a view
 *   of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, a real parameter that is not a number, a parameter outside its range, an
 *   input too short for the range, a buffer that is empty, too short or of the wrong
 *   type or a buffer sharing memory with a `Float64Array` input it is not. Checked
 *   before anything is written.
 *
 * @example
 * ```ts
 * import { asi } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { begIdx, values } = asi({ open, high, low, close }, { limitMove: 3 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`si`](https://ta-lib.org/functions/si)
 * @see [`wad`](https://ta-lib.org/functions/wad)
 * @see [`cumsum`](https://ta-lib.org/functions/cumsum)
 */
export function asi(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: AsiOptions): AsiResult {
  requireArgument('ASI', 'prices', prices);
  const o = optionsOf('ASI', options);
  if (o.endIdx === undefined) {
    requireSeries('ASI', 'prices.open', prices.open);
    requireSeries('ASI', 'prices.high', prices.high);
    requireSeries('ASI', 'prices.low', prices.low);
    requireSeries('ASI', 'prices.close', prices.close);
  }
  const optInLimitMove = orDefault(o.limitMove, Core.REAL_DEFAULT);
  const core = coreOf('ASI', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, asiLookbackPositional(optInLimitMove, core), prices.open, prices.high, prices.low, prices.close);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = asiPositional(startIdx, endIdx, prices.open, prices.high, prices.low, prices.close, optInLimitMove, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live ASI stream: one value per closed bar, bit-identical to {@link asi} over
 * the same series. Open it with {@link asiOpen} or {@link asiOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/asi](https://ta-lib.org/functions/asi).
 */
export class AsiStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInLimitMove: number = 0;
  /** @internal */
  prevClose: number = 0;
  /** @internal */
  prevBody: number = 0;
  /** @internal */
  sum: number = 0;
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
  static of(core: Core): AsiStream {
    return new AsiStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link asi} reports them over the same bars. Each accepted `update` and each
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
      throw failure('ASI advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inOpen - The bar's `inOpen` value: Open price of each bar.
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inOpen: number, inHigh: number, inLow: number, inClose: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('ASI update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inOpen) || !Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('ASI update', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    asiStepImpl(this, inOpen, inHigh, inLow, inClose);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inOpen - The bar's `inOpen` value: Open price of each bar.
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inOpen: number, inHigh: number, inLow: number, inClose: number): number {
    if (!Number.isFinite(inOpen) || !Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('ASI peek', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let tempOpen = 0;
    let tempHigh = 0;
    let tempLow = 0;
    let tempClose = 0;
    let body = 0;
    let n = 0;
    let up = 0;
    let dn = 0;
    let rg = 0;
    let k = 0;
    let r = 0;
    let swing = 0;
    let cur_outReal = 0;
    let sum = sp.sum;
    tempOpen = inOpen;
    tempHigh = inHigh;
    tempLow = inLow;
    tempClose = inClose;
    body = tempClose - tempOpen;
    n = tempClose - sp.prevClose;
    n += 0.5 * body;
    n += 0.25 * sp.prevBody;
    up = Math.abs(tempHigh - sp.prevClose);
    dn = Math.abs(tempLow - sp.prevClose);
    rg = Math.abs(tempHigh - tempLow);
    k = (up > dn ? up : dn);
    r = ((up - 0.5 * dn) > (dn - 0.5 * up) ? (up - 0.5 * dn) : (dn - 0.5 * up));
    r = (r > rg ? r : rg);
    r += 0.25 * Math.abs(sp.prevBody);
    if (r === 0.0) {
      swing = 0.0;
    } else {
      swing = 50.0 * (n / r) * (k / sp.optInLimitMove);
    }
    sum += swing;
    cur_outReal = sum;
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
  clone(): AsiStream {
    const c = new AsiStream(this.core);
    c.optInLimitMove = this.optInLimitMove;
    c.prevClose = this.prevClose;
    c.prevBody = this.prevBody;
    c.sum = this.sum;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function asiStepImpl(sp: AsiStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  let tempOpen = 0;
  let tempHigh = 0;
  let tempLow = 0;
  let tempClose = 0;
  let body = 0;
  let n = 0;
  let up = 0;
  let dn = 0;
  let rg = 0;
  let k = 0;
  let r = 0;
  let swing = 0;
  tempOpen = inOpen;
  tempHigh = inHigh;
  tempLow = inLow;
  tempClose = inClose;
  body = tempClose - tempOpen;
  n = tempClose - sp.prevClose;
  n += 0.5 * body;
  n += 0.25 * sp.prevBody;
  up = Math.abs(tempHigh - sp.prevClose);
  dn = Math.abs(tempLow - sp.prevClose);
  rg = Math.abs(tempHigh - tempLow);
  k = (up > dn ? up : dn);
  r = ((up - 0.5 * dn) > (dn - 0.5 * up) ? (up - 0.5 * dn) : (dn - 0.5 * up));
  r = (r > rg ? r : rg);
  r += 0.25 * Math.abs(sp.prevBody);
  if (r === 0.0) {
    swing = 0.0;
  } else {
    swing = 50.0 * (n / r) * (k / sp.optInLimitMove);
  }
  sp.sum += swing;
  sp.cur_outReal = sp.sum;
  sp.prevClose = tempClose;
  sp.prevBody = body;
}

function asiOpenImpl(
  sp: AsiStream,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInLimitMove: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let i = 0;
  let outIdx = 0;
  let prevClose = 0;
  let prevBody = 0;
  let tempOpen = 0;
  let tempHigh = 0;
  let tempLow = 0;
  let tempClose = 0;
  let body = 0;
  let n = 0;
  let up = 0;
  let dn = 0;
  let rg = 0;
  let k = 0;
  let r = 0;
  let swing = 0;
  let sum = 0;
  const historyLen = inOpen.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inHigh.length !== inOpen.length || inLow.length !== inOpen.length || inClose.length !== inOpen.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInLimitMove === Core.REAL_DEFAULT) {
    optInLimitMove = 3.0;
  } else if (!(optInLimitMove >= 1e-8 && optInLimitMove <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* SI's per-bar statements, kept identical: the regression test holds this
   * function bit for bit to CUMSUM over SI. Bar startIdx is read before the
   * anchor store, which may overwrite it when outReal aliases an input.
   */
  prevClose = inClose[startIdx];
  prevBody = prevClose - inOpen[startIdx];
  sum = 0.0;
  outReal[((0 * outStride) | 0)] = sum;
  outIdx = 1;
  for (i = startIdx + 1; i <= endIdx; i += 1) {
    tempOpen = inOpen[i];
    tempHigh = inHigh[i];
    tempLow = inLow[i];
    tempClose = inClose[i];
    body = tempClose - tempOpen;
    n = tempClose - prevClose;
    n += 0.5 * body;
    n += 0.25 * prevBody;
    up = Math.abs(tempHigh - prevClose);
    dn = Math.abs(tempLow - prevClose);
    rg = Math.abs(tempHigh - tempLow);
    k = (up > dn ? up : dn);
    r = ((up - 0.5 * dn) > (dn - 0.5 * up) ? (up - 0.5 * dn) : (dn - 0.5 * up));
    r = (r > rg ? r : rg);
    r += 0.25 * Math.abs(prevBody);
    if (r === 0.0) {
      swing = 0.0;
    } else {
      swing = 50.0 * (n / r) * (k / optInLimitMove);
    }
    sum += swing;
    outReal[((outIdx * outStride) | 0)] = sum;
    outIdx += 1;
    prevClose = tempClose;
    prevBody = body;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  sp.optInLimitMove = optInLimitMove;
  sp.prevClose = prevClose;
  sp.prevBody = prevBody;
  sp.sum = sum;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function asiOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInLimitMove: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): AsiStream {
  const sp = AsiStream.of(core);
  const retCode = asiOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, optInLimitMove, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ASI openAndFill', inOpen.length, startIdx, asiLookbackPositional(optInLimitMove, core));
  }
  if (retCode === RetCode.BAD_PARAM && asiLookbackPositional(optInLimitMove, core) < 0) {
    throw rejectedParams('ASI openAndFill', PARAM_RANGES, optInLimitMove);
  }
  throw streamFailure('ASI openAndFill', retCode);
}

/** @internal */
export function asiOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInLimitMove: number,
): AsiStream {
  const sp = AsiStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = asiOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, optInLimitMove, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ASI open', inOpen.length, startIdx, asiLookbackPositional(optInLimitMove, core));
  }
  if (retCode === RetCode.BAD_PARAM && asiLookbackPositional(optInLimitMove, core) < 0) {
    throw rejectedParams('ASI open', PARAM_RANGES, optInLimitMove);
  }
  throw streamFailure('ASI open', retCode);
}

/** @internal */
export function asiOpenPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInLimitMove: number,
  core: Core = Core.DEFAULT,
): AsiStream {
  optInLimitMove = requireReal('ASI open', 'limitMove', optInLimitMove);
  requireArgument('ASI open', 'prices.open', inOpen);
  requireHistory('ASI open', inOpen.length);
  requireArgument('ASI open', 'prices.high', inHigh);
  requireArgument('ASI open', 'prices.low', inLow);
  requireArgument('ASI open', 'prices.close', inClose);
  requireHistoryLength('ASI open', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('ASI open', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('ASI open', 'prices.close', inClose.length, inOpen.length);
  return asiOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0, optInLimitMove);
}

/** @internal */
export function asiOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInLimitMove: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): AsiStream {
  optInLimitMove = requireReal('ASI openAndFill', 'limitMove', optInLimitMove);
  requireArgument('ASI openAndFill', 'prices.open', inOpen);
  requireHistory('ASI openAndFill', inOpen.length);
  requireArgument('ASI openAndFill', 'prices.high', inHigh);
  requireArgument('ASI openAndFill', 'prices.low', inLow);
  requireArgument('ASI openAndFill', 'prices.close', inClose);
  const guardLookback = asiLookbackPositional(optInLimitMove, core);
  if (guardLookback < 0) {
    throw rejectedParams('ASI openAndFill', PARAM_RANGES, optInLimitMove);
  }
  const guardOutLen = openFillCount('ASI openAndFill', inOpen.length, guardLookback);
  requireHistoryLength('ASI openAndFill', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('ASI openAndFill', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('ASI openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('ASI openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outReal.buffer === inOpenF64.buffer && outReal.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('ASI openAndFill', 'outReal', 'prices.open');
  }
  if (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('ASI openAndFill', 'outReal', 'prices.high');
  }
  if (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('ASI openAndFill', 'outReal', 'prices.low');
  }
  if (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('ASI openAndFill', 'outReal', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outReal, inOpenF64)) {
    throw sharedMemory('ASI openAndFill', 'outReal', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outReal, inHighF64)) {
    throw sharedMemory('ASI openAndFill', 'outReal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outReal, inLowF64)) {
    throw sharedMemory('ASI openAndFill', 'outReal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outReal, inCloseF64)) {
    throw sharedMemory('ASI openAndFill', 'outReal', 'prices.close');
  }
  return asiOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, optInLimitMove, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live ASI stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link asi} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `open`: Open price of each bar. `high`: High price of each bar. `low`:
 *   Low price of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AsiOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `asiLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   a real parameter that is not a number, a parameter outside its range or
 *   histories of different lengths.
 *
 * @example
 * ```ts
 * import { asiOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = asiOpen({ open, high, low, close }, { limitMove: 3 });
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function asiOpen(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: AsiOptions): AsiStream {
  requireArgument('ASI open', 'prices', prices);
  const o = optionsOf('ASI open', options);
  const optInLimitMove = orDefault(o.limitMove, Core.REAL_DEFAULT);
  const core = coreOf('ASI open', o.core);
  return asiOpenPositional(prices.open, prices.high, prices.low, prices.close, optInLimitMove, core);
}

/**
 * {@link asiOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link asi}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `open`: Open price of each bar. `high`: High price of each bar. `low`:
 *   Low price of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link AsiOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link asi} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `asiLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   a real parameter that is not a number, a parameter outside its range, histories
 *   of different lengths or a buffer too short, of the wrong type or starting where
 *   another buffer or a `Float64Array` input starts, or, on a history long enough to
 *   open, one sharing memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { asiOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = asiOpenAndFill({ open, high, low, close }, {
 *   limitMove: 3,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function asiOpenAndFill(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: AsiOptions): AsiResult & { stream: AsiStream } {
  requireArgument('ASI openAndFill', 'prices', prices);
  const o = optionsOf('ASI openAndFill', options);
  const optInLimitMove = orDefault(o.limitMove, Core.REAL_DEFAULT);
  const core = coreOf('ASI openAndFill', o.core);
  const capacity = fillCapacity(asiLookbackPositional(optInLimitMove, core), prices.open, prices.high, prices.low, prices.close);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = asiOpenAndFillPositional(prices.open, prices.high, prices.low, prices.close, optInLimitMove, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
