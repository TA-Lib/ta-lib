/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AB       Anatoliy Belsky
 *  MF       Mario Fortier
 *  WZ       wony (github @wony-zheng)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  181012 AB    Initial Version
 *  070526 MF,CC  Fix #98: the unstable period grew the summation window
 *                to period+u bars; window is now always 'period'.
 *  070726 WZ,CC  (#14) IMI has no unstable period; drop the unstable-period
 *                term from the lookback so TA_SetUnstablePeriod is a no-op.
 *  071326 MF,CC  Fix #112: an all-flat window (every close==open) leaves
 *                upsum==downsum==0, so 100*(0/0) emitted NaN from a *successful*
 *                call. Guard the divide, returning IMI's neutral center 50.0.
 *  092426 MF,CC  #440 ratio once per bar, not per element; branch-free split.
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function imiLookbackPositional(optInTimePeriod: number, _core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  return optInTimePeriod - 1;
}

/** @internal */
export function imiDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (imiLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function imiImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inOpen: Float64Array,
  inClose: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let lookback = 0;
  let outIdx = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  outIdx = 0;
  lookback = imiLookbackPositional(optInTimePeriod, core);
  if (startIdx < lookback) {
    startIdx = lookback;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  outBegIdx.value = startIdx;
  while (startIdx <= endIdx) {
    let upsum = 0.0;
    let downsum = 0.0;
    let i = 0;
    for (i = startIdx - (optInTimePeriod - 1); i <= startIdx; i += 1) {
      let diff = inClose[i] - inOpen[i];
      /* max(diff, 0) spelled with fabs is branch-free in every backend. Java
       * compiles a ternary to a branch and gcc an if/else; either mispredicts
       * on random data, though the if/else retires fewer instructions.
       */
      let up = (diff + Math.abs(diff)) * 0.5;
      upsum += up;
      downsum += up - diff;
    }
    /* #112: an all-flat window (every close==open) leaves upsum==downsum==0.
     * Guard the 0/0 so a successful call never emits NaN; IMI is a 0..100
     * oscillator, so no up/down bias returns its neutral center, 50.0.
     */
    outReal[outIdx] = (upsum + downsum === 0.0) ? 50.0 : 100.0 * (upsum / (upsum + downsum));
    startIdx += 1;
    outIdx += 1;
  }
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000']] as const;

/** @internal */
export function imiPositional(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('IMI', 'startIdx', startIdx);
  endIdx = requireInt32('IMI', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('IMI', 'timePeriod', optInTimePeriod);
  requireIndexRange('IMI', startIdx, endIdx);
  const guardLookback = imiLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('IMI', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('IMI', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('IMI', 'prices.open', inOpen, guardInLen);
  requireLength('IMI', 'prices.close', inClose, guardInLen);
  requireOutput('IMI', 'outReal', outReal, guardOutLen, Float64Array);
  const inOpenF64 = float64(inOpen);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outReal, inOpenF64)) {
    throw sharedMemory('IMI', 'outReal', 'prices.open');
  }
  if (overlapsPartly(outReal, inCloseF64)) {
    throw sharedMemory('IMI', 'outReal', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = imiImpl(core, startIdx, endIdx, inOpenF64, inCloseF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('IMI', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link imi}, {@link imiLookback}, {@link imiDisplayShift},
 * {@link imiOpen} and {@link imiOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface ImiOptions {
  /**
   * Rolling window length for the up/down body sums (default 14; range 2 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
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
  outReal?: Float64Array<ArrayBuffer> | undefined;
}

/** What {@link imi} returns, and {@link imiOpenAndFill} with its stream. */
export interface ImiResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /** IMI oscillator value, 0-100. `values[k]` is the value at input bar `begIdx + k`. */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link imi} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link ImiOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function imiLookback(options?: ImiOptions): number {
  const o = optionsOf('IMI lookback', options);
  return imiLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('IMI lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link imi}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link ImiOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function imiDisplayShift(outputIdx: number, options?: ImiOptions): number {
  const o = optionsOf('IMI displayShift', options);
  return imiDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('IMI displayShift', o.core));
}

/**
 * Intraday Momentum Index: an RSI-like 0-100 oscillator built from the
 * open-to-close body of each bar. Over a rolling window it ratios cumulative
 * up-body moves against total up+down body moves.
 *
 * Formula and more info at
 * [ta-lib.org/functions/imi](https://ta-lib.org/functions/imi).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link imiLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `open`: Open price of
 *   each bar. `close`: Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link ImiOptions}.
 * @returns `begIdx` and the values; see {@link ImiResult}. Each output is a view
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
 * import { imi } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { begIdx, values } = imi({ open, close }, { timePeriod: 14 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`rsi`](https://ta-lib.org/functions/rsi)
 */
export function imi(prices: { open: ArrayLike<number>; high?: ArrayLike<number> | undefined; low?: ArrayLike<number> | undefined; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: ImiOptions): ImiResult {
  requireArgument('IMI', 'prices', prices);
  const o = optionsOf('IMI', options);
  if (o.endIdx === undefined) {
    requireSeries('IMI', 'prices.open', prices.open);
    requireSeries('IMI', 'prices.close', prices.close);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('IMI', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, imiLookbackPositional(optInTimePeriod, core), prices.open, prices.close);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = imiPositional(startIdx, endIdx, prices.open, prices.close, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live IMI stream: one value per closed bar, bit-identical to {@link imi} over
 * the same series. Open it with {@link imiOpen} or {@link imiOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/imi](https://ta-lib.org/functions/imi).
 */
export class ImiStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  winPos_i: number = 0;
  /** @internal */
  winCap_i: number = 1;
  /** @internal */
  win_i_inOpen: Float64Array = new Float64Array(1);
  /** @internal */
  win_i_inClose: Float64Array = new Float64Array(1);
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
  static of(core: Core): ImiStream {
    return new ImiStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link imi} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('IMI advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param open - The bar's `open` value: Open price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(open: number, close: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('IMI update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(open) || !Number.isFinite(close)) {
      throw nonFiniteBar('IMI update', !Number.isFinite(open) ? 'open' : 'close');
    }
    imiStepImpl(this, open, close);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param open - The bar's `open` value: Open price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(open: number, close: number): number {
    if (!Number.isFinite(open) || !Number.isFinite(close)) {
      throw nonFiniteBar('IMI peek', !Number.isFinite(open) ? 'open' : 'close');
    }
    const sp = this;
    const inOpen = open;
    const inClose = close;
    let upsum = 0;
    let downsum = 0;
    let i = 0;
    let diff = 0;
    let up = 0;
    let cur_outReal = 0;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    let pkSlot1 = -1;
    let pkVal1 = 0;
    pkSlot0 = sp.winPos_i;
    pkVal0 = inOpen;
    pkSlot1 = sp.winPos_i;
    pkVal1 = inClose;
    upsum = 0.0;
    downsum = 0.0;
    for (i = sp.optInTimePeriod - 1; i >= 0; i -= 1) {
      diff = ((((sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i) !== pkSlot1) ? sp.win_i_inClose[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i] : pkVal1) - ((((sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i) !== pkSlot0) ? sp.win_i_inOpen[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i] : pkVal0);
      /* max(diff, 0) spelled with fabs is branch-free in every backend. Java
       * compiles a ternary to a branch and gcc an if/else; either mispredicts
       * on random data, though the if/else retires fewer instructions.
       */
      up = (diff + Math.abs(diff)) * 0.5;
      upsum += up;
      downsum += up - diff;
    }
    /* #112: an all-flat window (every close==open) leaves upsum==downsum==0.
     * Guard the 0/0 so a successful call never emits NaN; IMI is a 0..100
     * oscillator, so no up/down bias returns its neutral center, 50.0.
     */
    cur_outReal = (upsum + downsum === 0.0) ? 50.0 : 100.0 * (upsum / (upsum + downsum));
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
  clone(): ImiStream {
    const c = new ImiStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.winPos_i = this.winPos_i;
    c.winCap_i = this.winCap_i;
    c.win_i_inOpen = this.win_i_inOpen.slice();
    c.win_i_inClose = this.win_i_inClose.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function imiStepImpl(sp: ImiStream, inOpen: number, inClose: number): void {
  let upsum = 0;
  let downsum = 0;
  let i = 0;
  let diff = 0;
  let up = 0;
  sp.win_i_inOpen[sp.winPos_i] = inOpen;
  sp.win_i_inClose[sp.winPos_i] = inClose;
  upsum = 0.0;
  downsum = 0.0;
  for (i = sp.optInTimePeriod - 1; i >= 0; i -= 1) {
    diff = sp.win_i_inClose[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i] - sp.win_i_inOpen[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i];
    /* max(diff, 0) spelled with fabs is branch-free in every backend. Java
     * compiles a ternary to a branch and gcc an if/else; either mispredicts
     * on random data, though the if/else retires fewer instructions.
     */
    up = (diff + Math.abs(diff)) * 0.5;
    upsum += up;
    downsum += up - diff;
  }
  /* #112: an all-flat window (every close==open) leaves upsum==downsum==0.
   * Guard the 0/0 so a successful call never emits NaN; IMI is a 0..100
   * oscillator, so no up/down bias returns its neutral center, 50.0.
   */
  sp.cur_outReal = (upsum + downsum === 0.0) ? 50.0 : 100.0 * (upsum / (upsum + downsum));
  sp.winPos_i = sp.winPos_i + 1;
  if (sp.winPos_i >= sp.winCap_i) {
    sp.winPos_i = 0;
  }
}

function imiOpenImpl(
  sp: ImiStream,
  inOpen: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let lookback = 0;
  let outIdx = 0;
  const historyLen = inOpen.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inClose.length !== inOpen.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outIdx = 0;
  lookback = imiLookbackPositional(optInTimePeriod, core);
  if (startIdx < lookback) {
    startIdx = lookback;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outBegIdx.value = startIdx;
  while (startIdx <= endIdx) {
    let upsum = 0.0;
    let downsum = 0.0;
    let i = 0;
    for (i = startIdx - (optInTimePeriod - 1); i <= startIdx; i += 1) {
      let diff = inClose[i] - inOpen[i];
      /* max(diff, 0) spelled with fabs is branch-free in every backend. Java
       * compiles a ternary to a branch and gcc an if/else; either mispredicts
       * on random data, though the if/else retires fewer instructions.
       */
      let up = (diff + Math.abs(diff)) * 0.5;
      upsum += up;
      downsum += up - diff;
    }
    /* #112: an all-flat window (every close==open) leaves upsum==downsum==0.
     * Guard the 0/0 so a successful call never emits NaN; IMI is a 0..100
     * oscillator, so no up/down bias returns its neutral center, 50.0.
     */
    outReal[((outIdx * outStride) | 0)] = (upsum + downsum === 0.0) ? 50.0 : 100.0 * (upsum / (upsum + downsum));
    startIdx += 1;
    outIdx += 1;
  }
  outNBElement.value = outIdx;
  const cap_i = optInTimePeriod - 1 + 1;
  if (cap_i < 1 || cap_i > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const capWin_i_inOpen = inOpen.slice(historyLen - cap_i, historyLen);
  const capWin_i_inClose = inClose.slice(historyLen - cap_i, historyLen);
  sp.optInTimePeriod = optInTimePeriod;
  sp.winPos_i = 0;
  sp.winCap_i = cap_i;
  sp.win_i_inOpen = capWin_i_inOpen;
  sp.win_i_inClose = capWin_i_inClose;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function imiOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): ImiStream {
  const sp = ImiStream.of(core);
  const retCode = imiOpenImpl(sp, inOpen, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('IMI openAndFill', inOpen.length, startIdx, imiLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && imiLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('IMI openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('IMI openAndFill', retCode);
}

/** @internal */
export function imiOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): ImiStream {
  const sp = ImiStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = imiOpenImpl(sp, inOpen, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('IMI open', inOpen.length, startIdx, imiLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && imiLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('IMI open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('IMI open', retCode);
}

/** @internal */
export function imiOpenPositional(
  inOpen: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): ImiStream {
  optInTimePeriod = requireInt32('IMI open', 'timePeriod', optInTimePeriod);
  requireArgument('IMI open', 'prices.open', inOpen);
  requireHistory('IMI open', 'prices.open', inOpen.length);
  requireArgument('IMI open', 'prices.close', inClose);
  requireHistoryLength('IMI open', 'prices.close', inClose.length, inOpen.length);
  return imiOpenInternal(core, float64(inOpen), float64(inClose), 0, optInTimePeriod);
}

/** @internal */
export function imiOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): ImiStream {
  optInTimePeriod = requireInt32('IMI openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('IMI openAndFill', 'prices.open', inOpen);
  requireHistory('IMI openAndFill', 'prices.open', inOpen.length);
  requireArgument('IMI openAndFill', 'prices.close', inClose);
  const guardLookback = imiLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('IMI openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('IMI openAndFill', inOpen.length, guardLookback);
  requireHistoryLength('IMI openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('IMI openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inOpenF64 = float64(inOpen);
  const inCloseF64 = float64(inClose);
  if (outReal.buffer === inOpenF64.buffer && outReal.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('IMI openAndFill', 'outReal', 'prices.open');
  }
  if (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('IMI openAndFill', 'outReal', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outReal, inOpenF64)) {
    throw sharedMemory('IMI openAndFill', 'outReal', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outReal, inCloseF64)) {
    throw sharedMemory('IMI openAndFill', 'outReal', 'prices.close');
  }
  return imiOpenAndFillInternal(core, inOpenF64, inCloseF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live IMI stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link imi} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link ImiOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `imiLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or
 *   histories of different lengths.
 *
 * @example
 * ```ts
 * import { imiOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = imiOpen({ open, close }, { timePeriod: 14 });
 * const peeked = stream.peek(100.2, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 100.9); // the bar closes
 * ```
 */
export function imiOpen(prices: { open: ArrayLike<number>; high?: ArrayLike<number> | undefined; low?: ArrayLike<number> | undefined; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: ImiOptions): ImiStream {
  requireArgument('IMI open', 'prices', prices);
  const o = optionsOf('IMI open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('IMI open', o.core);
  return imiOpenPositional(prices.open, prices.close, optInTimePeriod, core);
}

/**
 * {@link imiOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link imi}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link ImiOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link imi} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `imiLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { imiOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = imiOpenAndFill({ open, close }, {
 *   timePeriod: 14,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function imiOpenAndFill(prices: { open: ArrayLike<number>; high?: ArrayLike<number> | undefined; low?: ArrayLike<number> | undefined; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: ImiOptions): ImiResult & { stream: ImiStream } {
  requireArgument('IMI openAndFill', 'prices', prices);
  const o = optionsOf('IMI openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('IMI openAndFill', o.core);
  const capacity = fillCapacity(imiLookbackPositional(optInTimePeriod, core), prices.open, prices.close);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = imiOpenAndFillPositional(prices.open, prices.close, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
