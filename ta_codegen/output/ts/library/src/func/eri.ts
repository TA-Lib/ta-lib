/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090626 KL     First version (issue #361).
 */

import { badSink, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { emaLookbackPositional } from './ema.ts';

/** @internal */
export function eriLookbackPositional(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 13;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  /* Exactly the EMA of close underneath: its lookback, unstable period
   * included, is this function's lookback.
   */
  return emaLookbackPositional(optInTimePeriod, core);
}

/** @internal */
export function eriDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (eriLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 2) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function eriImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outBullPower: Float64Array,
  outBearPower: Float64Array,
): RetCode {
  let outIdx = 0;
  let today = 0;
  let lookbackTotal = 0;
  let i = 0;
  let prevMA = 0;
  let tempReal = 0;
  let k = 0;
  let beta = 0;
  let tempHT = 0;
  let tempLT = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 13;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if ((outBullPower.buffer === outBearPower.buffer && outBullPower.byteOffset === outBearPower.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  /* Elder Ray Index (Alexander Elder, Trading for a Living, 1993): how far
   * the bar's extremes sit from one shared EMA of close.
   *
   *   Bull Power = High - EMA(Close, n)
   *   Bear Power = Low  - EMA(Close, n)
   *
   * One fused loop, not ema() + a combine map: a composed form cannot
   * stream (raw bar inputs are outside check_map_step's provenance), which
   * is the same reason ACCBANDS is fused. The EMA is ema.c op for op --
   * sequential seed sum from 0.0 then one divide, the unstable-period
   * warm-up consumed bar by bar -- so the differential against shipped
   * TA_EMA holds bitwise.
   *
   * No division in the per-bar map: no 0/0, no NaN path (#112 by
   * construction). Bull >= Bear on every bar since high >= low.
   */
  lookbackTotal = eriLookbackPositional(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  /* Period 1: ema.c's explicit copy arm, kept here for the same reason it
   * exists there. At n == 1 the recursion below does not keep the sign
   * of a -0.0 close, so without this arm `High - TA_EMA(Close, 1)` is not
   * what this function returns. The unstable period still delays the first
   * output, through the shared lookback above.
   */
  if (optInTimePeriod === 1) {
    outIdx = 0;
    today = startIdx;
    while (today <= endIdx) {
      tempHT = inHigh[today];
      tempLT = inLow[today];
      tempReal = inClose[today];
      outBullPower[outIdx] = tempHT - tempReal;
      outBearPower[outIdx] = tempLT - tempReal;
      outIdx += 1;
      today += 1;
    }
    outBegIdx.value = startIdx;
    outNBElement.value = outIdx;
    return RetCode.SUCCESS;
  }
  beta = (optInTimePeriod - 1) / (optInTimePeriod + 1);
  k = 1.0 - beta;
  if (beta < 0.5) {
    beta = 1.0 - k;
  }
  /* Seed: ema.c's DEFAULT arm, op for op. */
  today = startIdx - lookbackTotal;
  i = optInTimePeriod;
  tempReal = 0.0;
  while (i-- > 0) {
    tempReal += inClose[today++];
  }
  prevMA = tempReal / optInTimePeriod;
  /* The warm-up also consumes the EMA unstable period. */
  while (today <= startIdx) {
    prevMA = fma(beta, prevMA, k * inClose[today++]);
  }
  /* prevMA is the EMA at bar startIdx; today == startIdx + 1. Load the
   * extremes into temps BEFORE writing either output: with two outputs
   * over three inputs the caller may alias any pair, and the second write
   * must not read a clobbered bar.
   */
  tempHT = inHigh[startIdx];
  tempLT = inLow[startIdx];
  outBullPower[0] = tempHT - prevMA;
  outBearPower[0] = tempLT - prevMA;
  outIdx = 1;
  while (today <= endIdx) {
    prevMA = fma(beta, prevMA, k * inClose[today]);
    tempHT = inHigh[today];
    tempLT = inLow[today];
    outBullPower[outIdx] = tempHT - prevMA;
    outBearPower[outIdx] = tempLT - prevMA;
    outIdx += 1;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000']] as const;

/** @internal */
export function eriPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outBullPower: Float64Array,
  outBearPower: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('ERI', 'startIdx', startIdx);
  endIdx = requireInt32('ERI', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('ERI', 'timePeriod', optInTimePeriod);
  requireIndexRange('ERI', startIdx, endIdx);
  const guardLookback = eriLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('ERI', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('ERI', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('ERI', 'prices.high', inHigh, guardInLen);
  requireLength('ERI', 'prices.low', inLow, guardInLen);
  requireLength('ERI', 'prices.close', inClose, guardInLen);
  requireOutput('ERI', 'outBullPower', outBullPower, guardOutLen, Float64Array);
  requireOutput('ERI', 'outBearPower', outBearPower, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outBullPower, inHighF64)) {
    throw sharedMemory('ERI', 'outBullPower', 'prices.high');
  }
  if (overlapsPartly(outBullPower, inLowF64)) {
    throw sharedMemory('ERI', 'outBullPower', 'prices.low');
  }
  if (overlapsPartly(outBullPower, inCloseF64)) {
    throw sharedMemory('ERI', 'outBullPower', 'prices.close');
  }
  if (overlapsPartly(outBearPower, inHighF64)) {
    throw sharedMemory('ERI', 'outBearPower', 'prices.high');
  }
  if (overlapsPartly(outBearPower, inLowF64)) {
    throw sharedMemory('ERI', 'outBearPower', 'prices.low');
  }
  if (overlapsPartly(outBearPower, inCloseF64)) {
    throw sharedMemory('ERI', 'outBearPower', 'prices.close');
  }
  if (overlaps(outBullPower, outBearPower)) {
    throw sharedMemory('ERI', 'outBullPower', 'outBearPower');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = eriImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInTimePeriod, outBegIdx, outNBElement, outBullPower, outBearPower);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('ERI', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link eri}, {@link eriLookback}, {@link eriDisplayShift},
 * {@link eriOpen} and {@link eriOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface EriOptions {
  /**
   * Number of bars in the EMA of close (default 13; range 1 to 100000;
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
   * Buffer for `outBullPower`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outBullPower?: Float64Array<ArrayBuffer> | undefined;
  /**
   * Buffer for `outBearPower`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outBearPower?: Float64Array<ArrayBuffer> | undefined;
}

/** What {@link eri} returns, and {@link eriOpenAndFill} with its stream. */
export interface EriResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * High minus the EMA of close. `outBullPower[k]` is the value at input bar
   * `begIdx + k`.
   */
  outBullPower: Float64Array<ArrayBuffer>;
  /**
   * Low minus the EMA of close. `outBearPower[k]` is the value at input bar
   * `begIdx + k`.
   */
  outBearPower: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link eri} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link EriOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function eriLookback(options?: EriOptions): number {
  const o = optionsOf('ERI lookback', options);
  return eriLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('ERI lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link eri}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link EriOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function eriDisplayShift(outputIdx: number, options?: EriOptions): number {
  const o = optionsOf('ERI displayShift', options);
  return eriDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('ERI displayShift', o.core));
}

/**
 * Elder Ray Index: Alexander Elder's Bull Power / Bear Power pair from *Trading
 * for a Living* (1993) — how far the bar's high and low sit from an EMA of the
 * close. Bulls strong enough to push the high above the average read as positive
 * Bull Power; bears dragging the low below it read as negative Bear Power.
 *
 * Formula and more info at
 * [ta-lib.org/functions/eri](https://ta-lib.org/functions/eri).
 *
 * **Notes**
 * - ERI is a cancelling difference: near the zero crossings that carry its signal,
 *   tiny EMA discrepancies are amplified without bound in relative terms. Compare
 *   against external values with an absolute tolerance.
 * - No MAType parameter: every canonical source fixes the EMA, and a selectable
 *   average would invent a variant nobody ships.
 *
 * `outBullPower[k]` is the value at input bar `begIdx + k`; every output is
 * exactly as long as the number of values written, and nothing is padded. A range
 * that ends before {@link eriLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `high`: High price
 *   series. `low`: Low price series. `close`: Close price series.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link EriOptions}.
 * @returns `begIdx` and the values; see {@link EriResult}. Each output is a view
 *   of its buffer, the caller's or a new one.
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
 * import { eri } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { begIdx, outBullPower, outBearPower } = eri({ high, low, close }, {
 *   timePeriod: 13,
 * });
 * // outBullPower[k] is the value at bar begIdx + k
 * ```
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`efi`](https://ta-lib.org/functions/efi)
 * @see [`macd`](https://ta-lib.org/functions/macd)
 */
export function eri(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: EriOptions): EriResult {
  requireArgument('ERI', 'prices', prices);
  const o = optionsOf('ERI', options);
  if (o.endIdx === undefined) {
    requireSeries('ERI', 'prices.high', prices.high);
    requireSeries('ERI', 'prices.low', prices.low);
    requireSeries('ERI', 'prices.close', prices.close);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('ERI', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, eriLookbackPositional(optInTimePeriod, core), prices.high, prices.low, prices.close);
  const outBullPower = o.outBullPower === undefined ? new Float64Array(capacity) : o.outBullPower;
  const outBearPower = o.outBearPower === undefined ? new Float64Array(capacity) : o.outBearPower;
  const range = eriPositional(startIdx, endIdx, prices.high, prices.low, prices.close, optInTimePeriod, outBullPower, outBearPower, core);
  return { begIdx: range.begIdx, outBullPower: outBullPower.subarray(0, range.count), outBearPower: outBearPower.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live ERI stream: one value per closed bar, bit-identical to {@link eri} over
 * the same series. Open it with {@link eriOpen} or {@link eriOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/eri](https://ta-lib.org/functions/eri).
 */
export class EriStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  prevMA: number = 0;
  /** @internal */
  k: number = 0;
  /** @internal */
  beta: number = 0;
  /** @internal */
  cur_outBullPower: number = 0;
  /** @internal */
  cur_outBearPower: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): EriStream {
    return new EriStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link eri} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('ERI advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns its outputs.
   *
   * @param high - The bar's `high` value: High price series.
   * @param low - The bar's `low` value: Low price series.
   * @param close - The bar's `close` value: Close price series.
   * @param out - Receives the outputs and is returned; a new {@link EriOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included, before anything moves.
   *   `TALibError` with code `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached
   *   bar `Core.INDEX_MAX`.
   */
  update(high: number, low: number, close: number, out?: EriOut): EriOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('ERI update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('ERI update', 'EriOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('ERI update', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    eriStepImpl(this, high, low, close);
    this.outRangeCount++;
    if (out === undefined) {
      out = new EriOut();
    }
    out.outBullPower = this.cur_outBullPower;
    out.outBearPower = this.cur_outBearPower;
    return out;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param high - The bar's `high` value: High price series.
   * @param low - The bar's `low` value: Low price series.
   * @param close - The bar's `close` value: Close price series.
   * @param out - Receives the outputs and is returned; a new {@link EriOut} when
   *   omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included.
   */
  peek(high: number, low: number, close: number, out?: EriOut): EriOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('ERI peek', 'EriOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('ERI peek', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
    const inClose = close;
    let cur_outBullPower = 0;
    let cur_outBearPower = 0;
    if (sp.optInTimePeriod === 1) {
      let tempReal = 0;
      let tempHT = 0;
      let tempLT = 0;
      tempHT = inHigh;
      tempLT = inLow;
      tempReal = inClose;
      cur_outBullPower = tempHT - tempReal;
      cur_outBearPower = tempLT - tempReal;
    } else {
      let tempHT = 0;
      let tempLT = 0;
      let prevMA = sp.prevMA;
      prevMA = fma(sp.beta, prevMA, sp.k * inClose);
      tempHT = inHigh;
      tempLT = inLow;
      cur_outBullPower = tempHT - prevMA;
      cur_outBearPower = tempLT - prevMA;
    }
    if (out === undefined) {
      out = new EriOut();
    }
    out.outBullPower = cur_outBullPower;
    out.outBearPower = cur_outBearPower;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link EriOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a sink that is not an object,
   *   `null` included.
   */
  value(out?: EriOut): EriOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('ERI value', 'EriOut');
    }
    if (out === undefined) {
      out = new EriOut();
    }
    out.outBullPower = this.cur_outBullPower;
    out.outBearPower = this.cur_outBearPower;
    return out;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): EriStream {
    const c = new EriStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.prevMA = this.prevMA;
    c.k = this.k;
    c.beta = this.beta;
    c.cur_outBullPower = this.cur_outBullPower;
    c.cur_outBearPower = this.cur_outBearPower;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one ERI bar, as `update`, `peek` and `value` return them. Passed
 * back to one of them, it is filled and returned again, with no allocation; it
 * holds its values until the next call that writes it. A new one holds `NaN`.
 */
export class EriOut {
  /** High minus the EMA of close. */
  outBullPower: number = NaN;
  /** Low minus the EMA of close. */
  outBearPower: number = NaN;
}

function eriStepImpl(sp: EriStream, inHigh: number, inLow: number, inClose: number): void {
  if (sp.optInTimePeriod === 1) {
    let tempReal = 0;
    let tempHT = 0;
    let tempLT = 0;
    tempHT = inHigh;
    tempLT = inLow;
    tempReal = inClose;
    sp.cur_outBullPower = tempHT - tempReal;
    sp.cur_outBearPower = tempLT - tempReal;
  } else {
    let tempHT = 0;
    let tempLT = 0;
    sp.prevMA = fma(sp.beta, sp.prevMA, sp.k * inClose);
    tempHT = inHigh;
    tempLT = inLow;
    sp.cur_outBullPower = tempHT - sp.prevMA;
    sp.cur_outBearPower = tempLT - sp.prevMA;
  }
}

function eriOpenImpl(
  sp: EriStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outBullPower: Float64Array,
  outBearPower: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length || inClose.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 13;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod === 1) {
    let outIdx = 0;
    let today = 0;
    let lookbackTotal = 0;
    let prevMA = 0;
    let tempReal = 0;
    let k = 0;
    let beta = 0;
    let tempHT = 0;
    let tempLT = 0;
    /* Elder Ray Index (Alexander Elder, Trading for a Living, 1993): how far
     * the bar's extremes sit from one shared EMA of close.
     *
     *   Bull Power = High - EMA(Close, n)
     *   Bear Power = Low  - EMA(Close, n)
     *
     * One fused loop, not ema() + a combine map: a composed form cannot
     * stream (raw bar inputs are outside check_map_step's provenance), which
     * is the same reason ACCBANDS is fused. The EMA is ema.c op for op --
     * sequential seed sum from 0.0 then one divide, the unstable-period
     * warm-up consumed bar by bar -- so the differential against shipped
     * TA_EMA holds bitwise.
     *
     * No division in the per-bar map: no 0/0, no NaN path (#112 by
     * construction). Bull >= Bear on every bar since high >= low.
     */
    lookbackTotal = eriLookbackPositional(optInTimePeriod, core);
    if (startIdx < lookbackTotal) {
      startIdx = lookbackTotal;
    }
    /* Make sure there is still something to evaluate. */
    if (startIdx > endIdx) {
      outBegIdx.value = 0;
      outNBElement.value = 0;
      return RetCode.INSUFFICIENT_HISTORY;
    }
    /* Period 1: ema.c's explicit copy arm, kept here for the same reason it
     * exists there. At n == 1 the recursion below does not keep the sign
     * of a -0.0 close, so without this arm `High - TA_EMA(Close, 1)` is not
     * what this function returns. The unstable period still delays the first
     * output, through the shared lookback above.
     */
    outIdx = 0;
    today = startIdx;
    while (today <= endIdx) {
      tempHT = inHigh[today];
      tempLT = inLow[today];
      tempReal = inClose[today];
      outBullPower[((outIdx * outStride) | 0)] = tempHT - tempReal;
      outBearPower[((outIdx * outStride) | 0)] = tempLT - tempReal;
      outIdx += 1;
      today += 1;
    }
    outBegIdx.value = startIdx;
    outNBElement.value = outIdx;
    sp.optInTimePeriod = optInTimePeriod;
    sp.prevMA = prevMA;
    sp.k = k;
    sp.beta = beta;
    sp.cur_outBullPower = outBullPower[(outNBElement.value - 1) * outStride];
    sp.cur_outBearPower = outBearPower[(outNBElement.value - 1) * outStride];
    return RetCode.SUCCESS;
  } else {
    let outIdx = 0;
    let today = 0;
    let lookbackTotal = 0;
    let i = 0;
    let prevMA = 0;
    let tempReal = 0;
    let k = 0;
    let beta = 0;
    let tempHT = 0;
    let tempLT = 0;
    /* Elder Ray Index (Alexander Elder, Trading for a Living, 1993): how far
     * the bar's extremes sit from one shared EMA of close.
     *
     *   Bull Power = High - EMA(Close, n)
     *   Bear Power = Low  - EMA(Close, n)
     *
     * One fused loop, not ema() + a combine map: a composed form cannot
     * stream (raw bar inputs are outside check_map_step's provenance), which
     * is the same reason ACCBANDS is fused. The EMA is ema.c op for op --
     * sequential seed sum from 0.0 then one divide, the unstable-period
     * warm-up consumed bar by bar -- so the differential against shipped
     * TA_EMA holds bitwise.
     *
     * No division in the per-bar map: no 0/0, no NaN path (#112 by
     * construction). Bull >= Bear on every bar since high >= low.
     */
    lookbackTotal = eriLookbackPositional(optInTimePeriod, core);
    if (startIdx < lookbackTotal) {
      startIdx = lookbackTotal;
    }
    /* Make sure there is still something to evaluate. */
    if (startIdx > endIdx) {
      outBegIdx.value = 0;
      outNBElement.value = 0;
      return RetCode.INSUFFICIENT_HISTORY;
    }
    /* Period 1: ema.c's explicit copy arm, kept here for the same reason it
     * exists there. At n == 1 the recursion below does not keep the sign
     * of a -0.0 close, so without this arm `High - TA_EMA(Close, 1)` is not
     * what this function returns. The unstable period still delays the first
     * output, through the shared lookback above.
     */
    beta = (optInTimePeriod - 1) / (optInTimePeriod + 1);
    k = 1.0 - beta;
    if (beta < 0.5) {
      beta = 1.0 - k;
    }
    /* Seed: ema.c's DEFAULT arm, op for op. */
    today = startIdx - lookbackTotal;
    i = optInTimePeriod;
    tempReal = 0.0;
    while (i-- > 0) {
      tempReal += inClose[today++];
    }
    prevMA = tempReal / optInTimePeriod;
    /* The warm-up also consumes the EMA unstable period. */
    while (today <= startIdx) {
      prevMA = fma(beta, prevMA, k * inClose[today++]);
    }
    /* prevMA is the EMA at bar startIdx; today == startIdx + 1. Load the
     * extremes into temps BEFORE writing either output: with two outputs
     * over three inputs the caller may alias any pair, and the second write
     * must not read a clobbered bar.
     */
    tempHT = inHigh[startIdx];
    tempLT = inLow[startIdx];
    outBullPower[((0 * outStride) | 0)] = tempHT - prevMA;
    outBearPower[((0 * outStride) | 0)] = tempLT - prevMA;
    outIdx = 1;
    while (today <= endIdx) {
      prevMA = fma(beta, prevMA, k * inClose[today]);
      tempHT = inHigh[today];
      tempLT = inLow[today];
      outBullPower[((outIdx * outStride) | 0)] = tempHT - prevMA;
      outBearPower[((outIdx * outStride) | 0)] = tempLT - prevMA;
      outIdx += 1;
      today += 1;
    }
    outBegIdx.value = startIdx;
    outNBElement.value = outIdx;
    sp.optInTimePeriod = optInTimePeriod;
    sp.prevMA = prevMA;
    sp.k = k;
    sp.beta = beta;
    sp.cur_outBullPower = outBullPower[(outNBElement.value - 1) * outStride];
    sp.cur_outBearPower = outBearPower[(outNBElement.value - 1) * outStride];
    return RetCode.SUCCESS;
  }
}

/** @internal */
export function eriOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outBullPower: Float64Array,
  outBearPower: Float64Array,
): EriStream {
  const sp = EriStream.of(core);
  const retCode = eriOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outBullPower, outBearPower, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ERI openAndFill', inHigh.length, startIdx, eriLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && eriLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('ERI openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('ERI openAndFill', retCode);
}

/** @internal */
export function eriOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): EriStream {
  const sp = EriStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outBullPower = new Float64Array(1);
  const sink_outBearPower = new Float64Array(1);
  const retCode = eriOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outBullPower, sink_outBearPower, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ERI open', inHigh.length, startIdx, eriLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && eriLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('ERI open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('ERI open', retCode);
}

/** @internal */
export function eriOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): EriStream {
  optInTimePeriod = requireInt32('ERI open', 'timePeriod', optInTimePeriod);
  requireArgument('ERI open', 'prices.high', inHigh);
  requireHistory('ERI open', 'prices.high', inHigh.length);
  requireArgument('ERI open', 'prices.low', inLow);
  requireArgument('ERI open', 'prices.close', inClose);
  requireHistoryLength('ERI open', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('ERI open', 'prices.close', inClose.length, inHigh.length);
  return eriOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInTimePeriod);
}

/** @internal */
export function eriOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outBullPower: Float64Array,
  outBearPower: Float64Array,
  core: Core = Core.DEFAULT,
): EriStream {
  optInTimePeriod = requireInt32('ERI openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('ERI openAndFill', 'prices.high', inHigh);
  requireHistory('ERI openAndFill', 'prices.high', inHigh.length);
  requireArgument('ERI openAndFill', 'prices.low', inLow);
  requireArgument('ERI openAndFill', 'prices.close', inClose);
  const guardLookback = eriLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('ERI openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('ERI openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('ERI openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('ERI openAndFill', 'prices.close', inClose.length, inHigh.length);
  requireOutput('ERI openAndFill', 'outBullPower', outBullPower, guardOutLen, Float64Array);
  requireOutput('ERI openAndFill', 'outBearPower', outBearPower, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outBullPower.buffer === inHighF64.buffer && outBullPower.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('ERI openAndFill', 'outBullPower', 'prices.high');
  }
  if (outBullPower.buffer === inLowF64.buffer && outBullPower.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('ERI openAndFill', 'outBullPower', 'prices.low');
  }
  if (outBullPower.buffer === inCloseF64.buffer && outBullPower.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('ERI openAndFill', 'outBullPower', 'prices.close');
  }
  if (outBearPower.buffer === inHighF64.buffer && outBearPower.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('ERI openAndFill', 'outBearPower', 'prices.high');
  }
  if (outBearPower.buffer === inLowF64.buffer && outBearPower.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('ERI openAndFill', 'outBearPower', 'prices.low');
  }
  if (outBearPower.buffer === inCloseF64.buffer && outBearPower.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('ERI openAndFill', 'outBearPower', 'prices.close');
  }
  if (outBullPower.buffer === outBearPower.buffer && outBullPower.byteOffset === outBearPower.byteOffset) {
    throw sharedMemory('ERI openAndFill', 'outBullPower', 'outBearPower');
  }
  if (guardOutLen > 0 && overlaps(outBullPower, inHighF64)) {
    throw sharedMemory('ERI openAndFill', 'outBullPower', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outBullPower, inLowF64)) {
    throw sharedMemory('ERI openAndFill', 'outBullPower', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outBullPower, inCloseF64)) {
    throw sharedMemory('ERI openAndFill', 'outBullPower', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outBearPower, inHighF64)) {
    throw sharedMemory('ERI openAndFill', 'outBearPower', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outBearPower, inLowF64)) {
    throw sharedMemory('ERI openAndFill', 'outBearPower', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outBearPower, inCloseF64)) {
    throw sharedMemory('ERI openAndFill', 'outBearPower', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outBullPower, outBearPower)) {
    throw sharedMemory('ERI openAndFill', 'outBullPower', 'outBearPower');
  }
  return eriOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outBullPower, outBearPower);
}

/**
 * Opens a live ERI stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link eri} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price series. `low`: Low price series. `close`: Close price series.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link EriOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `eriLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or
 *   histories of different lengths.
 *
 * @example
 * ```ts
 * import { eriOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = eriOpen({ high, low, close }, { timePeriod: 13 });
 * const peeked = stream.peek(101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function eriOpen(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: EriOptions): EriStream {
  requireArgument('ERI open', 'prices', prices);
  const o = optionsOf('ERI open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('ERI open', o.core);
  return eriOpenPositional(prices.high, prices.low, prices.close, optInTimePeriod, core);
}

/**
 * {@link eriOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link eri}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price series. `low`: Low price series. `close`: Close price series.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link EriOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link eri} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `eriLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { eriOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const {
 *   stream,
 *   begIdx,
 *   outBullPower,
 *   outBearPower,
 * } = eriOpenAndFill({ high, low, close }, { timePeriod: 13 });
 * // outBullPower[k] is the value at bar begIdx + k
 * ```
 */
export function eriOpenAndFill(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: EriOptions): EriResult & { stream: EriStream } {
  requireArgument('ERI openAndFill', 'prices', prices);
  const o = optionsOf('ERI openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('ERI openAndFill', o.core);
  const capacity = fillCapacity(eriLookbackPositional(optInTimePeriod, core), prices.high, prices.low, prices.close);
  const outBullPower = o.outBullPower === undefined ? new Float64Array(capacity) : o.outBullPower;
  const outBearPower = o.outBearPower === undefined ? new Float64Array(capacity) : o.outBearPower;
  const stream = eriOpenAndFillPositional(prices.high, prices.low, prices.close, optInTimePeriod, outBullPower, outBearPower, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outBullPower: outBullPower.subarray(0, range.count), outBearPower: outBearPower.subarray(0, range.count) };
}
