/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  RM       Robert Meier
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  120307 RM     Initial Version
 *  120907 MF     Handling of a few limit cases
 *  071226 MF,CC  Fused single-loop rewrite: maintain the three band running
 *                sums (close for the middle band; the pointwise High/Low map for
 *                the upper/lower bands) over one shared trailing window, instead
 *                of two scratch buffers + three sma() calls. Enables streaming
 *                and is bit-identical to the prior three-SMA form (verified vs
 *                v0.6.4).
 *  082326 MF,CC  Fix #253. Scale the High+Low cancellation test to its own
 *                operands instead of the fixed TA_IS_ZERO band, which widened
 *                the bands of any instrument quoted small enough to fall
 *                under it.
 */

import { badSink, clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { smaLookback } from './sma.ts';

/**
 * Number of leading input bars {@link accbands} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - SMA smoothing period for all three bands (default 20;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function accbandsLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  return smaLookback(optInTimePeriod, core);
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link accbands}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - SMA smoothing period for all three bands (default 20;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function accbandsDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (accbandsLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 3) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function accbandsImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
): RetCode {
  let periodTotalUpper = 0;
  let periodTotalMiddle = 0;
  let periodTotalLower = 0;
  let tempUpper = 0;
  let tempMiddle = 0;
  let tempLower = 0;
  let tempReal = 0;
  let i = 0;
  let outIdx = 0;
  let trailingIdx = 0;
  let lookbackTotal = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if ((outRealUpperBand.buffer === outRealMiddleBand.buffer && outRealUpperBand.byteOffset === outRealMiddleBand.byteOffset) || (outRealUpperBand.buffer === outRealLowerBand.buffer && outRealUpperBand.byteOffset === outRealLowerBand.byteOffset) || (outRealMiddleBand.buffer === outRealLowerBand.buffer && outRealMiddleBand.byteOffset === outRealLowerBand.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = smaLookback(optInTimePeriod, core);
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
  /* Each band is a simple moving average maintained as a running sum over a
   * shared trailing window (all three share optInTimePeriod, so one trailing
   * index walks all three windows in lockstep):
   *    middle = SMA( close )
   *    upper  = SMA( high * (1 + 4*(high-low)/(high+low)) )
   *    lower  = SMA( low  * (1 - 4*(high-low)/(high+low)) )
   * When high+low is zero the upper/lower map degenerates to high/low.
   * Fusing the three moving averages into one loop is bit-identical to the
   * former "two scratch buffers + three sma() calls": each accumulator's
   * add/record/subtract order is unchanged, and the High/Low map is a pure
   * function recomputed from the raw trailing bar.
   */
  periodTotalUpper = 0.0;
  periodTotalMiddle = 0.0;
  periodTotalLower = 0.0;
  trailingIdx = startIdx - lookbackTotal;
  /* Warm up the running sums with the initial period,
   * except for the last value.
   */
  i = trailingIdx;
  while (i < startIdx) {
    /* The band factor 4*(H-L)/(H+L) is a ratio of two prices, so it is
     * scale-free -- but H+L is a sum that CANCELS when the two prices have
     * opposite signs, and the factor then blows up on what is left of the
     * operands' last bits. Test the sum against ITS OWN operands, not against
     * a fixed band: an absolute threshold answers "cancelled" for every bar
     * of an instrument quoted small enough to fall under it, and widened
     * every band it touched (issue #253). Same test on all three sites, so
     * the bar that enters a running sum is the one that later leaves it.
     */
    tempReal = inHigh[i] + inLow[i];
    if (!(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(inHigh[i]) + Math.abs(inLow[i])))) {
      tempReal = 4 * (inHigh[i] - inLow[i]) / tempReal;
      periodTotalUpper += inHigh[i] * (1 + tempReal);
      periodTotalLower += inLow[i] * (1 - tempReal);
    } else {
      periodTotalUpper += inHigh[i];
      periodTotalLower += inLow[i];
    }
    periodTotalMiddle += inClose[i];
    i = i + 1;
  }
  /* Proceed with the calculation for the requested range.
   * Note that this algorithm allows the input and output to be the
   * same buffer: every trailing bar is read before any output is written.
   */
  outIdx = 0;
  while (i <= endIdx) {
    /* Add the incoming bar to each running sum. */
    tempReal = inHigh[i] + inLow[i];
    if (!(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(inHigh[i]) + Math.abs(inLow[i])))) {
      tempReal = 4 * (inHigh[i] - inLow[i]) / tempReal;
      periodTotalUpper += inHigh[i] * (1 + tempReal);
      periodTotalLower += inLow[i] * (1 - tempReal);
    } else {
      periodTotalUpper += inHigh[i];
      periodTotalLower += inLow[i];
    }
    periodTotalMiddle += inClose[i];
    i = i + 1;
    /* Record the current window sums. */
    tempUpper = periodTotalUpper;
    tempMiddle = periodTotalMiddle;
    tempLower = periodTotalLower;
    /* Remove the trailing bar from each running sum. */
    tempReal = inHigh[trailingIdx] + inLow[trailingIdx];
    if (!(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(inHigh[trailingIdx]) + Math.abs(inLow[trailingIdx])))) {
      tempReal = 4 * (inHigh[trailingIdx] - inLow[trailingIdx]) / tempReal;
      periodTotalUpper -= inHigh[trailingIdx] * (1 + tempReal);
      periodTotalLower -= inLow[trailingIdx] * (1 - tempReal);
    } else {
      periodTotalUpper -= inHigh[trailingIdx];
      periodTotalLower -= inLow[trailingIdx];
    }
    periodTotalMiddle -= inClose[trailingIdx];
    trailingIdx = trailingIdx + 1;
    /* Write the three bands. */
    outRealUpperBand[outIdx] = tempUpper / optInTimePeriod;
    outRealMiddleBand[outIdx] = tempMiddle / optInTimePeriod;
    outRealLowerBand[outIdx] = tempLower / optInTimePeriod;
    outIdx = outIdx + 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * Acceleration Bands: three overlap lines around price. The middle band is an SMA
 * of the close; the upper/lower bands are SMAs of the high/low scaled by an
 * intraday-range factor.
 *
 * Formula and more info at
 * [ta-lib.org/functions/accbands](https://ta-lib.org/functions/accbands).
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link accbandsLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param optInTimePeriod - SMA smoothing period for all three bands (default 20;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outRealUpperBand - SMA of the range-scaled high band. Must hold the
 *   values produced and never be empty; the input length always suffices.
 * @param outRealMiddleBand - SMA of the close. Must hold the values produced and
 *   never be empty; the input length always suffices.
 * @param outRealLowerBand - SMA of the range-scaled low band. Must hold the values
 *   produced and never be empty; the input length always suffices.
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
 * import { accbands } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const upperBand = new Float64Array(high.length);
 * const middleBand = new Float64Array(high.length);
 * const lowerBand = new Float64Array(high.length);
 * const { begIdx, count } = accbands(
 *   0, high.length - 1,
 *   high, low, close,
 *   20,
 *   upperBand, middleBand, lowerBand,
 * );
 * // upperBand[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`sma`](https://ta-lib.org/functions/sma)
 * @see [`bbands`](https://ta-lib.org/functions/bbands)
 */
export function accbands(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('ACCBANDS', 'startIdx', startIdx);
  endIdx = requireInt32('ACCBANDS', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('ACCBANDS', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('ACCBANDS', startIdx, endIdx);
  const guardStart = clampedStart('ACCBANDS', startIdx, accbandsLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('ACCBANDS', 'inHigh', inHigh, guardInLen);
  requireLength('ACCBANDS', 'inLow', inLow, guardInLen);
  requireLength('ACCBANDS', 'inClose', inClose, guardInLen);
  requireOutput('ACCBANDS', 'outRealUpperBand', outRealUpperBand, guardOutLen, Float64Array);
  requireOutput('ACCBANDS', 'outRealMiddleBand', outRealMiddleBand, guardOutLen, Float64Array);
  requireOutput('ACCBANDS', 'outRealLowerBand', outRealLowerBand, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outRealUpperBand, inHighF64) || overlapsPartly(outRealUpperBand, inLowF64) || overlapsPartly(outRealUpperBand, inCloseF64) || overlapsPartly(outRealMiddleBand, inHighF64) || overlapsPartly(outRealMiddleBand, inLowF64) || overlapsPartly(outRealMiddleBand, inCloseF64) || overlapsPartly(outRealLowerBand, inHighF64) || overlapsPartly(outRealLowerBand, inLowF64) || overlapsPartly(outRealLowerBand, inCloseF64) || overlaps(outRealUpperBand, outRealMiddleBand) || overlaps(outRealUpperBand, outRealLowerBand) || overlaps(outRealMiddleBand, outRealLowerBand)) {
    throw failure('ACCBANDS', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = accbandsImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInTimePeriod, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('ACCBANDS', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live ACCBANDS stream: one value per closed bar, bit-identical to
 * {@link accbands} over the same series. Open it with {@link accbandsOpen} or
 * {@link accbandsOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/accbands](https://ta-lib.org/functions/accbands).
 */
export class AccbandsStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  periodTotalUpper: number = 0;
  /** @internal */
  periodTotalMiddle: number = 0;
  /** @internal */
  periodTotalLower: number = 0;
  /** @internal */
  ringPos_trailingIdx: number = 0;
  /** @internal */
  ringCap_trailingIdx: number = 0;
  /** @internal */
  ring_trailingIdx_inHigh: Float64Array = new Float64Array(1);
  /** @internal */
  ring_trailingIdx_inLow: Float64Array = new Float64Array(1);
  /** @internal */
  ring_trailingIdx_inClose: Float64Array = new Float64Array(1);
  /** @internal */
  cur_outRealUpperBand: number = 0;
  /** @internal */
  cur_outRealMiddleBand: number = 0;
  /** @internal */
  cur_outRealLowerBand: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): AccbandsStream {
    return new AccbandsStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link accbands} reports them over the same bars. Each accepted `update` and
   * each `advance` adds one to `count`.
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
      throw failure('ACCBANDS advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar, writing the new values into `out`.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number, inClose: number, out: AccbandsOut): void {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('ACCBANDS update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (typeof out !== 'object' || out === null) {
      throw badSink('ACCBANDS update', 'AccbandsOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('ACCBANDS update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    accbandsStepImpl(this, inHigh, inLow, inClose);
    this.outRangeCount++;
    out.realUpperBand = this.cur_outRealUpperBand;
    out.realMiddleBand = this.cur_outRealMiddleBand;
    out.realLowerBand = this.cur_outRealLowerBand;
  }

  /**
   * Writes into `out` what the next `update` with the same bar would write,
   * committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number, inClose: number, out: AccbandsOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('ACCBANDS peek', 'AccbandsOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('ACCBANDS peek', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let tempUpper = 0;
    let tempMiddle = 0;
    let tempLower = 0;
    let tempReal = 0;
    let cur_outRealLowerBand = 0;
    let cur_outRealMiddleBand = 0;
    let cur_outRealUpperBand = 0;
    let periodTotalLower = sp.periodTotalLower;
    let periodTotalMiddle = sp.periodTotalMiddle;
    let periodTotalUpper = sp.periodTotalUpper;
    /* Add the incoming bar to each running sum. */
    tempReal = inHigh + inLow;
    if (!(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(inHigh) + Math.abs(inLow)))) {
      tempReal = 4 * (inHigh - inLow) / tempReal;
      periodTotalUpper += inHigh * (1 + tempReal);
      periodTotalLower += inLow * (1 - tempReal);
    } else {
      periodTotalUpper += inHigh;
      periodTotalLower += inLow;
    }
    periodTotalMiddle += inClose;
    /* Record the current window sums. */
    tempUpper = periodTotalUpper;
    tempMiddle = periodTotalMiddle;
    tempLower = periodTotalLower;
    /* Remove the trailing bar from each running sum. */
    tempReal = sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx] + sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx];
    if (!(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx]) + Math.abs(sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx])))) {
      tempReal = 4 * (sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx] - sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx]) / tempReal;
      periodTotalUpper -= sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx] * (1 + tempReal);
      periodTotalLower -= sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx] * (1 - tempReal);
    } else {
      periodTotalUpper -= sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx];
      periodTotalLower -= sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx];
    }
    periodTotalMiddle -= sp.ring_trailingIdx_inClose[sp.ringPos_trailingIdx];
    /* Write the three bands. */
    cur_outRealUpperBand = tempUpper / sp.optInTimePeriod;
    cur_outRealMiddleBand = tempMiddle / sp.optInTimePeriod;
    cur_outRealLowerBand = tempLower / sp.optInTimePeriod;
    out.realUpperBand = cur_outRealUpperBand;
    out.realMiddleBand = cur_outRealMiddleBand;
    out.realLowerBand = cur_outRealLowerBand;
  }

  /**
   * The value at the last bar this stream counted; `peek` does not change it.
   *
   * @param out - Receives the outputs.
   */
  value(out: AccbandsOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('ACCBANDS value', 'AccbandsOut');
    }
    out.realUpperBand = this.cur_outRealUpperBand;
    out.realMiddleBand = this.cur_outRealMiddleBand;
    out.realLowerBand = this.cur_outRealLowerBand;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): AccbandsStream {
    const c = new AccbandsStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.periodTotalUpper = this.periodTotalUpper;
    c.periodTotalMiddle = this.periodTotalMiddle;
    c.periodTotalLower = this.periodTotalLower;
    c.ringPos_trailingIdx = this.ringPos_trailingIdx;
    c.ringCap_trailingIdx = this.ringCap_trailingIdx;
    c.ring_trailingIdx_inHigh = this.ring_trailingIdx_inHigh.slice();
    c.ring_trailingIdx_inLow = this.ring_trailingIdx_inLow.slice();
    c.ring_trailingIdx_inClose = this.ring_trailingIdx_inClose.slice();
    c.cur_outRealUpperBand = this.cur_outRealUpperBand;
    c.cur_outRealMiddleBand = this.cur_outRealMiddleBand;
    c.cur_outRealLowerBand = this.cur_outRealLowerBand;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one ACCBANDS bar, written into an object the caller owns and
 * reuses: valid until the next call that writes it.
 */
export class AccbandsOut {
  /** SMA of the range-scaled high band. */
  realUpperBand: number = 0;
  /** SMA of the close. */
  realMiddleBand: number = 0;
  /** SMA of the range-scaled low band. */
  realLowerBand: number = 0;
}

function accbandsStepImpl(sp: AccbandsStream, inHigh: number, inLow: number, inClose: number): void {
  let tempUpper = 0;
  let tempMiddle = 0;
  let tempLower = 0;
  let tempReal = 0;
  let ringCapL_trailingIdx = 0;
  /* Add the incoming bar to each running sum. */
  tempReal = inHigh + inLow;
  if (!(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(inHigh) + Math.abs(inLow)))) {
    tempReal = 4 * (inHigh - inLow) / tempReal;
    sp.periodTotalUpper += inHigh * (1 + tempReal);
    sp.periodTotalLower += inLow * (1 - tempReal);
  } else {
    sp.periodTotalUpper += inHigh;
    sp.periodTotalLower += inLow;
  }
  sp.periodTotalMiddle += inClose;
  /* Record the current window sums. */
  tempUpper = sp.periodTotalUpper;
  tempMiddle = sp.periodTotalMiddle;
  tempLower = sp.periodTotalLower;
  /* Remove the trailing bar from each running sum. */
  tempReal = sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx] + sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx];
  if (!(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx]) + Math.abs(sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx])))) {
    tempReal = 4 * (sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx] - sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx]) / tempReal;
    sp.periodTotalUpper -= sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx] * (1 + tempReal);
    sp.periodTotalLower -= sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx] * (1 - tempReal);
  } else {
    sp.periodTotalUpper -= sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx];
    sp.periodTotalLower -= sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx];
  }
  sp.periodTotalMiddle -= sp.ring_trailingIdx_inClose[sp.ringPos_trailingIdx];
  /* Write the three bands. */
  sp.cur_outRealUpperBand = tempUpper / sp.optInTimePeriod;
  sp.cur_outRealMiddleBand = tempMiddle / sp.optInTimePeriod;
  sp.cur_outRealLowerBand = tempLower / sp.optInTimePeriod;
  ringCapL_trailingIdx = sp.ringCap_trailingIdx;
  sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx] = inHigh;
  sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx] = inLow;
  sp.ring_trailingIdx_inClose[sp.ringPos_trailingIdx] = inClose;
  sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
  if (sp.ringPos_trailingIdx >= ringCapL_trailingIdx) {
    sp.ringPos_trailingIdx = 0;
  }
}

function accbandsOpenImpl(
  sp: AccbandsStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let periodTotalUpper = 0;
  let periodTotalMiddle = 0;
  let periodTotalLower = 0;
  let tempUpper = 0;
  let tempMiddle = 0;
  let tempLower = 0;
  let tempReal = 0;
  let i = 0;
  let outIdx = 0;
  let trailingIdx = 0;
  let lookbackTotal = 0;
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
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = smaLookback(optInTimePeriod, core);
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
  /* Each band is a simple moving average maintained as a running sum over a
   * shared trailing window (all three share optInTimePeriod, so one trailing
   * index walks all three windows in lockstep):
   *    middle = SMA( close )
   *    upper  = SMA( high * (1 + 4*(high-low)/(high+low)) )
   *    lower  = SMA( low  * (1 - 4*(high-low)/(high+low)) )
   * When high+low is zero the upper/lower map degenerates to high/low.
   * Fusing the three moving averages into one loop is bit-identical to the
   * former "two scratch buffers + three sma() calls": each accumulator's
   * add/record/subtract order is unchanged, and the High/Low map is a pure
   * function recomputed from the raw trailing bar.
   */
  periodTotalUpper = 0.0;
  periodTotalMiddle = 0.0;
  periodTotalLower = 0.0;
  trailingIdx = startIdx - lookbackTotal;
  /* Warm up the running sums with the initial period,
   * except for the last value.
   */
  i = trailingIdx;
  while (i < startIdx) {
    /* The band factor 4*(H-L)/(H+L) is a ratio of two prices, so it is
     * scale-free -- but H+L is a sum that CANCELS when the two prices have
     * opposite signs, and the factor then blows up on what is left of the
     * operands' last bits. Test the sum against ITS OWN operands, not against
     * a fixed band: an absolute threshold answers "cancelled" for every bar
     * of an instrument quoted small enough to fall under it, and widened
     * every band it touched (issue #253). Same test on all three sites, so
     * the bar that enters a running sum is the one that later leaves it.
     */
    tempReal = inHigh[i] + inLow[i];
    if (!(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(inHigh[i]) + Math.abs(inLow[i])))) {
      tempReal = 4 * (inHigh[i] - inLow[i]) / tempReal;
      periodTotalUpper += inHigh[i] * (1 + tempReal);
      periodTotalLower += inLow[i] * (1 - tempReal);
    } else {
      periodTotalUpper += inHigh[i];
      periodTotalLower += inLow[i];
    }
    periodTotalMiddle += inClose[i];
    i = i + 1;
  }
  /* Proceed with the calculation for the requested range.
   * Note that this algorithm allows the input and output to be the
   * same buffer: every trailing bar is read before any output is written.
   */
  outIdx = 0;
  while (i <= endIdx) {
    /* Add the incoming bar to each running sum. */
    tempReal = inHigh[i] + inLow[i];
    if (!(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(inHigh[i]) + Math.abs(inLow[i])))) {
      tempReal = 4 * (inHigh[i] - inLow[i]) / tempReal;
      periodTotalUpper += inHigh[i] * (1 + tempReal);
      periodTotalLower += inLow[i] * (1 - tempReal);
    } else {
      periodTotalUpper += inHigh[i];
      periodTotalLower += inLow[i];
    }
    periodTotalMiddle += inClose[i];
    i = i + 1;
    /* Record the current window sums. */
    tempUpper = periodTotalUpper;
    tempMiddle = periodTotalMiddle;
    tempLower = periodTotalLower;
    /* Remove the trailing bar from each running sum. */
    tempReal = inHigh[trailingIdx] + inLow[trailingIdx];
    if (!(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(inHigh[trailingIdx]) + Math.abs(inLow[trailingIdx])))) {
      tempReal = 4 * (inHigh[trailingIdx] - inLow[trailingIdx]) / tempReal;
      periodTotalUpper -= inHigh[trailingIdx] * (1 + tempReal);
      periodTotalLower -= inLow[trailingIdx] * (1 - tempReal);
    } else {
      periodTotalUpper -= inHigh[trailingIdx];
      periodTotalLower -= inLow[trailingIdx];
    }
    periodTotalMiddle -= inClose[trailingIdx];
    trailingIdx = trailingIdx + 1;
    /* Write the three bands. */
    outRealUpperBand[((outIdx * outStride) | 0)] = tempUpper / optInTimePeriod;
    outRealMiddleBand[((outIdx * outStride) | 0)] = tempMiddle / optInTimePeriod;
    outRealLowerBand[((outIdx * outStride) | 0)] = tempLower / optInTimePeriod;
    outIdx = outIdx + 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  const cap_trailingIdx = i - trailingIdx;
  if (cap_trailingIdx < 1 || cap_trailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_trailingIdx = cap_trailingIdx > 0 ? cap_trailingIdx : 1;
  const capRing_trailingIdx_inHigh = new Float64Array(allocN_trailingIdx);
  capRing_trailingIdx_inHigh.set(inHigh.subarray(historyLen - cap_trailingIdx, historyLen), 0);
  const capRing_trailingIdx_inLow = new Float64Array(allocN_trailingIdx);
  capRing_trailingIdx_inLow.set(inLow.subarray(historyLen - cap_trailingIdx, historyLen), 0);
  const capRing_trailingIdx_inClose = new Float64Array(allocN_trailingIdx);
  capRing_trailingIdx_inClose.set(inClose.subarray(historyLen - cap_trailingIdx, historyLen), 0);
  sp.optInTimePeriod = optInTimePeriod;
  sp.periodTotalUpper = periodTotalUpper;
  sp.periodTotalMiddle = periodTotalMiddle;
  sp.periodTotalLower = periodTotalLower;
  sp.ringPos_trailingIdx = 0;
  sp.ringCap_trailingIdx = cap_trailingIdx;
  sp.ring_trailingIdx_inHigh = capRing_trailingIdx_inHigh;
  sp.ring_trailingIdx_inLow = capRing_trailingIdx_inLow;
  sp.ring_trailingIdx_inClose = capRing_trailingIdx_inClose;
  sp.cur_outRealUpperBand = outRealUpperBand[(outNBElement.value - 1) * outStride];
  sp.cur_outRealMiddleBand = outRealMiddleBand[(outNBElement.value - 1) * outStride];
  sp.cur_outRealLowerBand = outRealLowerBand[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function accbandsOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
): AccbandsStream {
  const sp = AccbandsStream.of(core);
  const retCode = accbandsOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ACCBANDS openAndFill', inHigh.length, startIdx, accbandsLookback(optInTimePeriod, core));
  }
  throw streamFailure('ACCBANDS openAndFill', retCode);
}

/** @internal */
export function accbandsOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): AccbandsStream {
  const sp = AccbandsStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outRealUpperBand = new Float64Array(1);
  const sink_outRealMiddleBand = new Float64Array(1);
  const sink_outRealLowerBand = new Float64Array(1);
  const retCode = accbandsOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outRealUpperBand, sink_outRealMiddleBand, sink_outRealLowerBand, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ACCBANDS open', inHigh.length, startIdx, accbandsLookback(optInTimePeriod, core));
  }
  throw streamFailure('ACCBANDS open', retCode);
}

/**
 * Opens a live ACCBANDS stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link accbands} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - SMA smoothing period for all three bands (default 20;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `accbandsLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an
 *   empty one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { AccbandsOut, accbandsOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = accbandsOpen(high, low, close, 20);
 * const peeked = new AccbandsOut();
 * const updated = new AccbandsOut();
 * stream.peek(101.4, 99.1, 100.9, peeked); // a forming bar: commits nothing
 * stream.update(101.4, 99.1, 100.9, updated); // the bar closes
 * ```
 */
export function accbandsOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): AccbandsStream {
  optInTimePeriod = requireInt32('ACCBANDS open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('ACCBANDS open', 'inHigh', inHigh);
  requireHistory('ACCBANDS open', inHigh.length);
  requireArgument('ACCBANDS open', 'inLow', inLow);
  requireArgument('ACCBANDS open', 'inClose', inClose);
  requireHistoryLength('ACCBANDS open', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('ACCBANDS open', 'inClose', inClose.length, inHigh.length);
  return accbandsOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInTimePeriod);
}

/**
 * {@link accbandsOpen} that also fills the outputs over the whole history in the
 * same pass, bit-identically to {@link accbands}. The range written is the
 * stream's `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - SMA smoothing period for all three bands (default 20;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outRealUpperBand - SMA of the range-scaled high band. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param outRealMiddleBand - SMA of the close. Must hold `historyLen - lookback`
 *   values and share no memory with an input or another output.
 * @param outRealLowerBand - SMA of the range-scaled low band. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `accbandsLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an
 *   empty one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range, histories of different lengths or an output too
 *   short or starting where another output or a `Float64Array` input starts, or, on
 *   a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { accbandsOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const upperBand = new Float64Array(high.length);
 * const middleBand = new Float64Array(high.length);
 * const lowerBand = new Float64Array(high.length);
 * const stream = accbandsOpenAndFill(
 *   high, low, close,
 *   20,
 *   upperBand, middleBand, lowerBand,
 * );
 * const { begIdx, count } = stream.outRange();
 * // upperBand[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function accbandsOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
  core: Core = Core.DEFAULT,
): AccbandsStream {
  optInTimePeriod = requireInt32('ACCBANDS openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('ACCBANDS openAndFill', 'inHigh', inHigh);
  requireHistory('ACCBANDS openAndFill', inHigh.length);
  requireArgument('ACCBANDS openAndFill', 'inLow', inLow);
  requireArgument('ACCBANDS openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('ACCBANDS openAndFill', inHigh.length, accbandsLookback(optInTimePeriod, core));
  requireHistoryLength('ACCBANDS openAndFill', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('ACCBANDS openAndFill', 'inClose', inClose.length, inHigh.length);
  requireOutput('ACCBANDS openAndFill', 'outRealUpperBand', outRealUpperBand, guardOutLen, Float64Array);
  requireOutput('ACCBANDS openAndFill', 'outRealMiddleBand', outRealMiddleBand, guardOutLen, Float64Array);
  requireOutput('ACCBANDS openAndFill', 'outRealLowerBand', outRealLowerBand, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outRealUpperBand.buffer === inHighF64.buffer && outRealUpperBand.byteOffset === inHighF64.byteOffset) || (outRealUpperBand.buffer === inLowF64.buffer && outRealUpperBand.byteOffset === inLowF64.byteOffset) || (outRealUpperBand.buffer === inCloseF64.buffer && outRealUpperBand.byteOffset === inCloseF64.byteOffset) || (outRealMiddleBand.buffer === inHighF64.buffer && outRealMiddleBand.byteOffset === inHighF64.byteOffset) || (outRealMiddleBand.buffer === inLowF64.buffer && outRealMiddleBand.byteOffset === inLowF64.byteOffset) || (outRealMiddleBand.buffer === inCloseF64.buffer && outRealMiddleBand.byteOffset === inCloseF64.byteOffset) || (outRealLowerBand.buffer === inHighF64.buffer && outRealLowerBand.byteOffset === inHighF64.byteOffset) || (outRealLowerBand.buffer === inLowF64.buffer && outRealLowerBand.byteOffset === inLowF64.byteOffset) || (outRealLowerBand.buffer === inCloseF64.buffer && outRealLowerBand.byteOffset === inCloseF64.byteOffset) || (outRealUpperBand.buffer === outRealMiddleBand.buffer && outRealUpperBand.byteOffset === outRealMiddleBand.byteOffset) || (outRealUpperBand.buffer === outRealLowerBand.buffer && outRealUpperBand.byteOffset === outRealLowerBand.byteOffset) || (outRealMiddleBand.buffer === outRealLowerBand.buffer && outRealMiddleBand.byteOffset === outRealLowerBand.byteOffset)) {
    throw streamFailure('ACCBANDS openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outRealUpperBand, inHighF64) || overlaps(outRealUpperBand, inLowF64) || overlaps(outRealUpperBand, inCloseF64) || overlaps(outRealMiddleBand, inHighF64) || overlaps(outRealMiddleBand, inLowF64) || overlaps(outRealMiddleBand, inCloseF64) || overlaps(outRealLowerBand, inHighF64) || overlaps(outRealLowerBand, inLowF64) || overlaps(outRealLowerBand, inCloseF64) || overlaps(outRealUpperBand, outRealMiddleBand) || overlaps(outRealUpperBand, outRealLowerBand) || overlaps(outRealMiddleBand, outRealLowerBand))) {
    throw streamFailure('ACCBANDS openAndFill', RetCode.BAD_PARAM);
  }
  return accbandsOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outRealUpperBand, outRealMiddleBand, outRealLowerBand);
}
