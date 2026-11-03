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

import { badSink, clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { emaLookback } from './ema.ts';

/**
 * Number of leading input bars {@link eri} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Number of bars in the EMA of close (default 13; range 1
 *   to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function eriLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
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
  return emaLookback(optInTimePeriod, core);
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link eri}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Number of bars in the EMA of close (default 13; range 1
 *   to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function eriDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (eriLookback(optInTimePeriod, core) < 0) {
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
  lookbackTotal = eriLookback(optInTimePeriod, core);
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
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link eriLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price series.
 * @param inLow - Low price series.
 * @param inClose - Close price series.
 * @param optInTimePeriod - Number of bars in the EMA of close (default 13; range 1
 *   to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outBullPower - High minus the EMA of close. Must hold the values produced
 *   and never be empty; the input length always suffices.
 * @param outBearPower - Low minus the EMA of close. Must hold the values produced
 *   and never be empty; the input length always suffices.
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
 * import { eri } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const bullPower = new Float64Array(high.length);
 * const bearPower = new Float64Array(high.length);
 * const { begIdx, count } = eri(
 *   0, high.length - 1,
 *   high, low, close,
 *   13,
 *   bullPower, bearPower,
 * );
 * // bullPower[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`efi`](https://ta-lib.org/functions/efi)
 * @see [`macd`](https://ta-lib.org/functions/macd)
 */
export function eri(
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
  optInTimePeriod = requireInt32('ERI', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('ERI', startIdx, endIdx);
  const guardStart = clampedStart('ERI', startIdx, eriLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('ERI', 'inHigh', inHigh, guardInLen);
  requireLength('ERI', 'inLow', inLow, guardInLen);
  requireLength('ERI', 'inClose', inClose, guardInLen);
  requireLength('ERI', 'outBullPower', outBullPower, guardOutLen);
  requireLength('ERI', 'outBearPower', outBearPower, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outBullPower, inHighF64) || overlapsPartly(outBullPower, inLowF64) || overlapsPartly(outBullPower, inCloseF64) || overlapsPartly(outBearPower, inHighF64) || overlapsPartly(outBearPower, inLowF64) || overlapsPartly(outBearPower, inCloseF64) || overlaps(outBullPower, outBearPower)) {
    throw failure('ERI', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = eriImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInTimePeriod, outBegIdx, outNBElement, outBullPower, outBearPower);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('ERI', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
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

  /** @internal */
  constructor(core: Core) {
    this.core = core;
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
      throw failure('ERI advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar, writing the new values into `out`.
   *
   * @param inHigh - The bar's `inHigh` value: High price series.
   * @param inLow - The bar's `inLow` value: Low price series.
   * @param inClose - The bar's `inClose` value: Close price series.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number, inClose: number, out: EriOut): void {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('ERI update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (typeof out !== 'object' || out === null) {
      throw badSink('ERI update', 'EriOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('ERI update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    eriStepImpl(this, inHigh, inLow, inClose);
    this.outRangeCount++;
    out.bullPower = this.cur_outBullPower;
    out.bearPower = this.cur_outBearPower;
  }

  /**
   * Writes into `out` what the next `update` with the same bar would write,
   * committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price series.
   * @param inLow - The bar's `inLow` value: Low price series.
   * @param inClose - The bar's `inClose` value: Close price series.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number, inClose: number, out: EriOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('ERI peek', 'EriOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('ERI peek', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
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
    out.bullPower = cur_outBullPower;
    out.bearPower = cur_outBearPower;
  }

  /**
   * The value at the last bar this stream counted; `peek` does not change it.
   *
   * @param out - Receives the outputs.
   */
  value(out: EriOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('ERI value', 'EriOut');
    }
    out.bullPower = this.cur_outBullPower;
    out.bearPower = this.cur_outBearPower;
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
 * The outputs of one ERI bar, written into an object the caller owns and reuses:
 * valid until the next call that writes it.
 */
export class EriOut {
  /** High minus the EMA of close. */
  bullPower: number = 0;
  /** Low minus the EMA of close. */
  bearPower: number = 0;
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
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
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
    lookbackTotal = eriLookback(optInTimePeriod, core);
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
    lookbackTotal = eriLookback(optInTimePeriod, core);
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
  const sp = new EriStream(core);
  const retCode = eriOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outBullPower, outBearPower, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ERI openAndFill', inHigh.length, startIdx, eriLookback(optInTimePeriod, core));
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
  const sp = new EriStream(core);
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
    throw insufficientHistory('ERI open', inHigh.length, startIdx, eriLookback(optInTimePeriod, core));
  }
  throw streamFailure('ERI open', retCode);
}

/**
 * Opens a live ERI stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link eri} there.
 *
 * @param inHigh - Warm-up history. High price series. Every history has the same
 *   length.
 * @param inLow - Warm-up history. Low price series. Every history has the same
 *   length.
 * @param inClose - Warm-up history. Close price series. Every history has the same
 *   length.
 * @param optInTimePeriod - Number of bars in the EMA of close (default 13; range 1
 *   to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `eriLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { EriOut, eriOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = eriOpen(high, low, close, 13);
 * const peeked = new EriOut();
 * const updated = new EriOut();
 * stream.peek(101.4, 99.1, 100.9, peeked); // a forming bar: commits nothing
 * stream.update(101.4, 99.1, 100.9, updated); // the bar closes
 * ```
 */
export function eriOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): EriStream {
  optInTimePeriod = requireInt32('ERI open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('ERI open', 'inHigh', inHigh);
  requireHistory('ERI open', inHigh.length);
  requireArgument('ERI open', 'inLow', inLow);
  requireArgument('ERI open', 'inClose', inClose);
  requireHistoryLength('ERI open', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('ERI open', 'inClose', inClose.length, inHigh.length);
  return eriOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInTimePeriod);
}

/**
 * {@link eriOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link eri}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price series. Every history has the same
 *   length.
 * @param inLow - Warm-up history. Low price series. Every history has the same
 *   length.
 * @param inClose - Warm-up history. Close price series. Every history has the same
 *   length.
 * @param optInTimePeriod - Number of bars in the EMA of close (default 13; range 1
 *   to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outBullPower - High minus the EMA of close. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param outBearPower - Low minus the EMA of close. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `eriLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range, histories of different lengths or an output too
 *   short or starting where another output or a `Float64Array` input starts, or, on
 *   a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
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
 * const bullPower = new Float64Array(high.length);
 * const bearPower = new Float64Array(high.length);
 * const stream = eriOpenAndFill(high, low, close, 13, bullPower, bearPower);
 * const { begIdx, count } = stream.outRange();
 * // bullPower[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function eriOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outBullPower: Float64Array,
  outBearPower: Float64Array,
  core: Core = Core.DEFAULT,
): EriStream {
  optInTimePeriod = requireInt32('ERI openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('ERI openAndFill', 'inHigh', inHigh);
  requireHistory('ERI openAndFill', inHigh.length);
  requireArgument('ERI openAndFill', 'inLow', inLow);
  requireArgument('ERI openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('ERI openAndFill', inHigh.length, eriLookback(optInTimePeriod, core));
  requireHistoryLength('ERI openAndFill', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('ERI openAndFill', 'inClose', inClose.length, inHigh.length);
  requireLength('ERI openAndFill', 'outBullPower', outBullPower, guardOutLen);
  requireLength('ERI openAndFill', 'outBearPower', outBearPower, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outBullPower.buffer === inHighF64.buffer && outBullPower.byteOffset === inHighF64.byteOffset) || (outBullPower.buffer === inLowF64.buffer && outBullPower.byteOffset === inLowF64.byteOffset) || (outBullPower.buffer === inCloseF64.buffer && outBullPower.byteOffset === inCloseF64.byteOffset) || (outBearPower.buffer === inHighF64.buffer && outBearPower.byteOffset === inHighF64.byteOffset) || (outBearPower.buffer === inLowF64.buffer && outBearPower.byteOffset === inLowF64.byteOffset) || (outBearPower.buffer === inCloseF64.buffer && outBearPower.byteOffset === inCloseF64.byteOffset) || (outBullPower.buffer === outBearPower.buffer && outBullPower.byteOffset === outBearPower.byteOffset)) {
    throw streamFailure('ERI openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outBullPower, inHighF64) || overlaps(outBullPower, inLowF64) || overlaps(outBullPower, inCloseF64) || overlaps(outBearPower, inHighF64) || overlaps(outBearPower, inLowF64) || overlaps(outBearPower, inCloseF64) || overlaps(outBullPower, outBearPower))) {
    throw streamFailure('ERI openAndFill', RetCode.BAD_PARAM);
  }
  return eriOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outBullPower, outBearPower);
}
