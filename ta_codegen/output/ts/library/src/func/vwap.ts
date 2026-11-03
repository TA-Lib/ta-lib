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
 *  082126 MF,CC  First version (issue #237).
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link vwap} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function vwapLookback(core: Core = Core.DEFAULT): number {
  void core;
  /* Cumulative from the first bar of the requested range, so the very
   * first bar already has a complete answer and nothing is consumed
   * before it. Same shape as ta_AD.c and ta_OBV.c.
   */
  return 0;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link vwap}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function vwapDisplayShift(outputIdx: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function vwapImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  inVolume: Float64Array,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let sumPV = 0;
  let sumV = 0;
  let typPrice = 0;
  let volume = 0;
  let tempReal = 0;
  let vwap = 0;
  let outIdx = 0;
  let i = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  /* Volume Weighted Average Price: the average typical price paid per
   * unit of volume, accumulated from the first bar of the range.
   *
   *    VWAP = sum( TYPPRICE * Volume ) / sum( Volume )
   *
   * Every charting package anchors this to a trading session and resets
   * the two sums at each session boundary. TA-Lib takes no timestamp on
   * any function, so the anchor is the caller's choice of range: pass one
   * session's slice of bars to get that session's VWAP. This is how AD
   * and OBV, the other two cumulative volume functions, are already used
   * across session boundaries (issue #237).
   */
  sumPV = 0.0;
  sumV = 0.0;
  vwap = 0.0;
  outIdx = 0;
  for (i = startIdx; i <= endIdx; i += 1) {
    /* The typical price is written exactly as in ta_TYPPRICE.c so that the
     * two agree bit for bit and this stays a true composite of it.
     */
    typPrice = (inHigh[i] + inLow[i] + inClose[i]) / 3.0;
    volume = inVolume[i];
    /* A bar is weighted only if both of its terms are real numbers. That is
     * the whole condition: a NaN or an infinity in the price or the volume
     * is the only way a bar cannot be weighted, and every other bar --
     * including one that traded nothing -- is weighted normally.
     *
     * The test gates BOTH adds. Letting the volume in without its matching
     * price term would leave a weight in the divisor that nothing paid for,
     * biasing every later value: a NaN close with a good volume would drag
     * the next value 25% low.
     *
     * Skipping the bar is what makes this recoverable. These are CUMULATIVE
     * sums with no trailing term to subtract anything back out, so a single
     * non-finite bar allowed in would leave both sums non-finite for the
     * REST of the call -- the line would repeat one stale value on every
     * later bar however clean it was, silently, and looking like a plausible
     * price the whole way. Skipping keeps the state usable, so the average
     * resumes on the very next bar that can be weighted.
     *
     * Testing the two INPUTS, not the product and not the candidate sums, is
     * a measured choice:
     *
     *   - The candidate sums would have to be committed conditionally, which
     *     puts four cmovs in the loop-carried dependency chain and costs
     *     +60% on this loop. Both forms below leave the adds unconditional
     *     inside a predicted branch and measure free.
     *   - The product alone would also detect every unusable bar, one test
     *     instead of two, and measures the same. But it would additionally
     *     drop a WELL-FORMED bar whose price and volume are both finite and
     *     whose product merely overflows -- silently, and taking that bar's
     *     volume out of the divisor with it. Testing the inputs leaves that
     *     case exactly as it was before this guard existed: the overflow
     *     reaches the sum and the call reports Inf, which is the documented
     *     `double` overflow class rather than an indicator defect, and is
     *     louder than a freeze.
     *
     * So this changes behaviour for one thing only: a bar whose price or
     * volume is not a finite number. On finite data the test is always true
     * and no value the function has ever produced moves. Only the batch path
     * needs it -- the streaming Update/Peek entry points reject a non-finite
     * bar with TA_BAD_PARAM before it reaches any accumulator.
     */
    /* The product is kept in its own statement so no compiler may contract it
     * into an FMA. Contracting here would make the C output disagree with the
     * Rust, Java and C# backends under the cross-language bitwise gate. Same
     * reason as in ta_codegen/input/vwma/vwma.c.
     *
     * Computed before the guard rather than inside it, and unconditionally,
     * so it stays a per-bar temporary. Assigned only on the taken arm it
     * would instead be live across bars, and the streaming tier would carry
     * it as a fourth state field in every handle -- 8 bytes to hold a value
     * no later bar reads. The multiply on a skipped bar is discarded.
     */
    tempReal = typPrice * volume;
    if ((Number.isFinite(typPrice)) && (Number.isFinite(volume))) {
      sumPV += tempReal;
      sumV += volume;
    }
    /* Bars that traded nothing carry no weight, so a zero-volume bar in
     * the middle of a series leaves both sums untouched and repeats the
     * previous value on its own -- no arm needed for that. A bar skipped
     * by the guard above repeats it for the same reason.
     *
     * The arm below is for the one case the ratio cannot express: a
     * leading run of bars before any volume has traded, where there are
     * no weights at all and the weighted mean is undefined. The last
     * value computed is carried forward instead, which is 0.0 until the
     * first bar with volume. Volume is non-negative, so once the divisor
     * leaves zero it never returns and this arm cannot fire again.
     *
     * A successful call therefore never emits NaN or Inf (issue #112),
     * which is the divergence from pandas-ta-classic and from
     * trading-signals: the first emits NaN there, the second no bar at
     * all. Testing sumV rather than the bar's own volume also keeps a
     * negative divisor -- which no non-negative volume series can
     * produce -- out of a price-scale output, as ta_CMF.c does.
     */
    if (sumV > 0.0) {
      vwap = sumPV / sumV;
    }
    outReal[outIdx++] = vwap;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * Volume Weighted Average Price: the average price paid per unit of volume traded,
 * accumulated from the first bar of the range onward. Because every bar is
 * weighted by the volume that traded on it, VWAP tracks where the bulk of the
 * money actually changed hands rather than where the last trade printed. Price
 * above VWAP is read as buyers paying up relative to the session's average cost,
 * price below it as the reverse, which is why execution desks quote fills against
 * it. VWAP is a running mean, not a moving average: it has no window and no decay,
 * so each new bar carries a smaller share of the total and the line grows steadily
 * more sluggish the further it runs from its anchor. It stays within the range of
 * the typical prices it averages, but over a long trending range it can sit far
 * from the current price.
 *
 * Formula and more info at
 * [ta-lib.org/functions/vwap](https://ta-lib.org/functions/vwap).
 *
 * **Notes**
 * - The sums run from the first bar of the range and are never reset. Charting
 *   packages anchor VWAP to a trading session and restart it at each session
 *   boundary; no TA-Lib function takes a timestamp or a session boundary, so the
 *   anchor is the range the caller asks for — pass one session's bars to get that
 *   session's VWAP. This is how AD and OBV, the other cumulative volume functions,
 *   are already used across sessions.
 * - Volume is expected to be non-negative. A zero-volume bar carries no weight, so
 *   one occurring after volume has traded leaves the average exactly where it was.
 *   Before *any* volume has traded there are no weights at all and the weighted
 *   mean is undefined; those bars carry the previous value forward, which is 0
 *   until the first bar with volume. A successful call never emits NaN or ±Inf.
 *   Other implementations differ here: pandas-ta-classic divides through and emits
 *   NaN, and trading-signals emits no value for the bar at all.
 * - A bar whose price or volume is not a finite number cannot be weighted, so it
 *   is left out of the average entirely and repeats the previous value. It is
 *   skipped, not absorbed: the running average stays usable and resumes on the
 *   next bar that can be weighted, rather than being held at one stale value for
 *   the remainder of the range.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link vwapLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param inVolume - Volume of each bar.
 * @param outReal - Volume weighted average price, cumulative from the first bar of
 *   the range. Must hold the values produced and never be empty; the input length
 *   always suffices.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The range written: `begIdx` is the first bar with a value, `count` how
 *   many were written.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for an argument that is not an int32, an
 *   array too short for the range, an empty output or an output sharing memory with
 *   a `Float64Array` input it is not. Checked before anything is written.
 *
 * @example
 * ```ts
 * import { vwap } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const out = new Float64Array(high.length);
 * const { begIdx, count } = vwap(
 *   0, high.length - 1,
 *   high, low, close, volume,
 *   out,
 * );
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`ad`](https://ta-lib.org/functions/ad)
 * @see [`obv`](https://ta-lib.org/functions/obv)
 * @see [`typprice`](https://ta-lib.org/functions/typprice)
 * @see [`vwma`](https://ta-lib.org/functions/vwma)
 */
export function vwap(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('VWAP', 'startIdx', startIdx);
  endIdx = requireInt32('VWAP', 'endIdx', endIdx);
  requireIndexRange('VWAP', startIdx, endIdx);
  const guardStart = clampedStart('VWAP', startIdx, vwapLookback(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('VWAP', 'inHigh', inHigh, guardInLen);
  requireLength('VWAP', 'inLow', inLow, guardInLen);
  requireLength('VWAP', 'inClose', inClose, guardInLen);
  requireLength('VWAP', 'inVolume', inVolume, guardInLen);
  requireLength('VWAP', 'outReal', outReal, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  const inVolumeF64 = float64(inVolume);
  if (overlapsPartly(outReal, inHighF64) || overlapsPartly(outReal, inLowF64) || overlapsPartly(outReal, inCloseF64) || overlapsPartly(outReal, inVolumeF64)) {
    throw failure('VWAP', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = vwapImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, inVolumeF64, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('VWAP', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live VWAP stream: one value per closed bar, bit-identical to {@link vwap} over
 * the same series. Open it with {@link vwapOpen} or {@link vwapOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/vwap](https://ta-lib.org/functions/vwap).
 */
export class VwapStream {
  /** @internal */
  core: Core;
  /** @internal */
  sumPV: number = 0;
  /** @internal */
  sumV: number = 0;
  /** @internal */
  vwap: number = 0;
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
   * {@link vwap} reports them over the same bars. Each accepted `update` and each
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
      throw failure('VWAP advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param inVolume - The bar's `inVolume` value: Volume of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number, inClose: number, inVolume: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('VWAP update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose) || !Number.isFinite(inVolume)) {
      throw nonFiniteBar('VWAP update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : !Number.isFinite(inClose) ? 'inClose' : 'inVolume');
    }
    vwapStepImpl(this, inHigh, inLow, inClose, inVolume);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param inVolume - The bar's `inVolume` value: Volume of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number, inClose: number, inVolume: number): number {
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose) || !Number.isFinite(inVolume)) {
      throw nonFiniteBar('VWAP peek', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : !Number.isFinite(inClose) ? 'inClose' : 'inVolume');
    }
    const sp = this;
    let typPrice = 0;
    let volume = 0;
    let tempReal = 0;
    let cur_outReal = 0;
    let sumPV = sp.sumPV;
    let sumV = sp.sumV;
    let vwap = sp.vwap;
    /* The typical price is written exactly as in ta_TYPPRICE.c so that the
     * two agree bit for bit and this stays a true composite of it.
     */
    typPrice = (inHigh + inLow + inClose) / 3.0;
    volume = inVolume;
    /* A bar is weighted only if both of its terms are real numbers. That is
     * the whole condition: a NaN or an infinity in the price or the volume
     * is the only way a bar cannot be weighted, and every other bar --
     * including one that traded nothing -- is weighted normally.
     *
     * The test gates BOTH adds. Letting the volume in without its matching
     * price term would leave a weight in the divisor that nothing paid for,
     * biasing every later value: a NaN close with a good volume would drag
     * the next value 25% low.
     *
     * Skipping the bar is what makes this recoverable. These are CUMULATIVE
     * sums with no trailing term to subtract anything back out, so a single
     * non-finite bar allowed in would leave both sums non-finite for the
     * REST of the call -- the line would repeat one stale value on every
     * later bar however clean it was, silently, and looking like a plausible
     * price the whole way. Skipping keeps the state usable, so the average
     * resumes on the very next bar that can be weighted.
     *
     * Testing the two INPUTS, not the product and not the candidate sums, is
     * a measured choice:
     *
     *   - The candidate sums would have to be committed conditionally, which
     *     puts four cmovs in the loop-carried dependency chain and costs
     *     +60% on this loop. Both forms below leave the adds unconditional
     *     inside a predicted branch and measure free.
     *   - The product alone would also detect every unusable bar, one test
     *     instead of two, and measures the same. But it would additionally
     *     drop a WELL-FORMED bar whose price and volume are both finite and
     *     whose product merely overflows -- silently, and taking that bar's
     *     volume out of the divisor with it. Testing the inputs leaves that
     *     case exactly as it was before this guard existed: the overflow
     *     reaches the sum and the call reports Inf, which is the documented
     *     `double` overflow class rather than an indicator defect, and is
     *     louder than a freeze.
     *
     * So this changes behaviour for one thing only: a bar whose price or
     * volume is not a finite number. On finite data the test is always true
     * and no value the function has ever produced moves. Only the batch path
     * needs it -- the streaming Update/Peek entry points reject a non-finite
     * bar with TA_BAD_PARAM before it reaches any accumulator.
     */
    /* The product is kept in its own statement so no compiler may contract it
     * into an FMA. Contracting here would make the C output disagree with the
     * Rust, Java and C# backends under the cross-language bitwise gate. Same
     * reason as in ta_codegen/input/vwma/vwma.c.
     *
     * Computed before the guard rather than inside it, and unconditionally,
     * so it stays a per-bar temporary. Assigned only on the taken arm it
     * would instead be live across bars, and the streaming tier would carry
     * it as a fourth state field in every handle -- 8 bytes to hold a value
     * no later bar reads. The multiply on a skipped bar is discarded.
     */
    tempReal = typPrice * volume;
    if ((Number.isFinite(typPrice)) && (Number.isFinite(volume))) {
      sumPV += tempReal;
      sumV += volume;
    }
    /* Bars that traded nothing carry no weight, so a zero-volume bar in
     * the middle of a series leaves both sums untouched and repeats the
     * previous value on its own -- no arm needed for that. A bar skipped
     * by the guard above repeats it for the same reason.
     *
     * The arm below is for the one case the ratio cannot express: a
     * leading run of bars before any volume has traded, where there are
     * no weights at all and the weighted mean is undefined. The last
     * value computed is carried forward instead, which is 0.0 until the
     * first bar with volume. Volume is non-negative, so once the divisor
     * leaves zero it never returns and this arm cannot fire again.
     *
     * A successful call therefore never emits NaN or Inf (issue #112),
     * which is the divergence from pandas-ta-classic and from
     * trading-signals: the first emits NaN there, the second no bar at
     * all. Testing sumV rather than the bar's own volume also keeps a
     * negative divisor -- which no non-negative volume series can
     * produce -- out of a price-scale output, as ta_CMF.c does.
     */
    if (sumV > 0.0) {
      vwap = sumPV / sumV;
    }
    cur_outReal = vwap;
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
  clone(): VwapStream {
    const c = new VwapStream(this.core);
    c.sumPV = this.sumPV;
    c.sumV = this.sumV;
    c.vwap = this.vwap;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function vwapStepImpl(sp: VwapStream, inHigh: number, inLow: number, inClose: number, inVolume: number): void {
  let typPrice = 0;
  let volume = 0;
  let tempReal = 0;
  /* The typical price is written exactly as in ta_TYPPRICE.c so that the
   * two agree bit for bit and this stays a true composite of it.
   */
  typPrice = (inHigh + inLow + inClose) / 3.0;
  volume = inVolume;
  /* A bar is weighted only if both of its terms are real numbers. That is
   * the whole condition: a NaN or an infinity in the price or the volume
   * is the only way a bar cannot be weighted, and every other bar --
   * including one that traded nothing -- is weighted normally.
   *
   * The test gates BOTH adds. Letting the volume in without its matching
   * price term would leave a weight in the divisor that nothing paid for,
   * biasing every later value: a NaN close with a good volume would drag
   * the next value 25% low.
   *
   * Skipping the bar is what makes this recoverable. These are CUMULATIVE
   * sums with no trailing term to subtract anything back out, so a single
   * non-finite bar allowed in would leave both sums non-finite for the
   * REST of the call -- the line would repeat one stale value on every
   * later bar however clean it was, silently, and looking like a plausible
   * price the whole way. Skipping keeps the state usable, so the average
   * resumes on the very next bar that can be weighted.
   *
   * Testing the two INPUTS, not the product and not the candidate sums, is
   * a measured choice:
   *
   *   - The candidate sums would have to be committed conditionally, which
   *     puts four cmovs in the loop-carried dependency chain and costs
   *     +60% on this loop. Both forms below leave the adds unconditional
   *     inside a predicted branch and measure free.
   *   - The product alone would also detect every unusable bar, one test
   *     instead of two, and measures the same. But it would additionally
   *     drop a WELL-FORMED bar whose price and volume are both finite and
   *     whose product merely overflows -- silently, and taking that bar's
   *     volume out of the divisor with it. Testing the inputs leaves that
   *     case exactly as it was before this guard existed: the overflow
   *     reaches the sum and the call reports Inf, which is the documented
   *     `double` overflow class rather than an indicator defect, and is
   *     louder than a freeze.
   *
   * So this changes behaviour for one thing only: a bar whose price or
   * volume is not a finite number. On finite data the test is always true
   * and no value the function has ever produced moves. Only the batch path
   * needs it -- the streaming Update/Peek entry points reject a non-finite
   * bar with TA_BAD_PARAM before it reaches any accumulator.
   */
  /* The product is kept in its own statement so no compiler may contract it
   * into an FMA. Contracting here would make the C output disagree with the
   * Rust, Java and C# backends under the cross-language bitwise gate. Same
   * reason as in ta_codegen/input/vwma/vwma.c.
   *
   * Computed before the guard rather than inside it, and unconditionally,
   * so it stays a per-bar temporary. Assigned only on the taken arm it
   * would instead be live across bars, and the streaming tier would carry
   * it as a fourth state field in every handle -- 8 bytes to hold a value
   * no later bar reads. The multiply on a skipped bar is discarded.
   */
  tempReal = typPrice * volume;
  if ((Number.isFinite(typPrice)) && (Number.isFinite(volume))) {
    sp.sumPV += tempReal;
    sp.sumV += volume;
  }
  /* Bars that traded nothing carry no weight, so a zero-volume bar in
   * the middle of a series leaves both sums untouched and repeats the
   * previous value on its own -- no arm needed for that. A bar skipped
   * by the guard above repeats it for the same reason.
   *
   * The arm below is for the one case the ratio cannot express: a
   * leading run of bars before any volume has traded, where there are
   * no weights at all and the weighted mean is undefined. The last
   * value computed is carried forward instead, which is 0.0 until the
   * first bar with volume. Volume is non-negative, so once the divisor
   * leaves zero it never returns and this arm cannot fire again.
   *
   * A successful call therefore never emits NaN or Inf (issue #112),
   * which is the divergence from pandas-ta-classic and from
   * trading-signals: the first emits NaN there, the second no bar at
   * all. Testing sumV rather than the bar's own volume also keeps a
   * negative divisor -- which no non-negative volume series can
   * produce -- out of a price-scale output, as ta_CMF.c does.
   */
  if (sp.sumV > 0.0) {
    sp.vwap = sp.sumPV / sp.sumV;
  }
  sp.cur_outReal = sp.vwap;
}

function vwapOpenImpl(
  sp: VwapStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let sumPV = 0;
  let sumV = 0;
  let typPrice = 0;
  let volume = 0;
  let tempReal = 0;
  let vwap = 0;
  let outIdx = 0;
  let i = 0;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length || inClose.length !== inHigh.length || inVolume.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Volume Weighted Average Price: the average typical price paid per
   * unit of volume, accumulated from the first bar of the range.
   *
   *    VWAP = sum( TYPPRICE * Volume ) / sum( Volume )
   *
   * Every charting package anchors this to a trading session and resets
   * the two sums at each session boundary. TA-Lib takes no timestamp on
   * any function, so the anchor is the caller's choice of range: pass one
   * session's slice of bars to get that session's VWAP. This is how AD
   * and OBV, the other two cumulative volume functions, are already used
   * across session boundaries (issue #237).
   */
  sumPV = 0.0;
  sumV = 0.0;
  vwap = 0.0;
  outIdx = 0;
  for (i = startIdx; i <= endIdx; i += 1) {
    /* The typical price is written exactly as in ta_TYPPRICE.c so that the
     * two agree bit for bit and this stays a true composite of it.
     */
    typPrice = (inHigh[i] + inLow[i] + inClose[i]) / 3.0;
    volume = inVolume[i];
    /* A bar is weighted only if both of its terms are real numbers. That is
     * the whole condition: a NaN or an infinity in the price or the volume
     * is the only way a bar cannot be weighted, and every other bar --
     * including one that traded nothing -- is weighted normally.
     *
     * The test gates BOTH adds. Letting the volume in without its matching
     * price term would leave a weight in the divisor that nothing paid for,
     * biasing every later value: a NaN close with a good volume would drag
     * the next value 25% low.
     *
     * Skipping the bar is what makes this recoverable. These are CUMULATIVE
     * sums with no trailing term to subtract anything back out, so a single
     * non-finite bar allowed in would leave both sums non-finite for the
     * REST of the call -- the line would repeat one stale value on every
     * later bar however clean it was, silently, and looking like a plausible
     * price the whole way. Skipping keeps the state usable, so the average
     * resumes on the very next bar that can be weighted.
     *
     * Testing the two INPUTS, not the product and not the candidate sums, is
     * a measured choice:
     *
     *   - The candidate sums would have to be committed conditionally, which
     *     puts four cmovs in the loop-carried dependency chain and costs
     *     +60% on this loop. Both forms below leave the adds unconditional
     *     inside a predicted branch and measure free.
     *   - The product alone would also detect every unusable bar, one test
     *     instead of two, and measures the same. But it would additionally
     *     drop a WELL-FORMED bar whose price and volume are both finite and
     *     whose product merely overflows -- silently, and taking that bar's
     *     volume out of the divisor with it. Testing the inputs leaves that
     *     case exactly as it was before this guard existed: the overflow
     *     reaches the sum and the call reports Inf, which is the documented
     *     `double` overflow class rather than an indicator defect, and is
     *     louder than a freeze.
     *
     * So this changes behaviour for one thing only: a bar whose price or
     * volume is not a finite number. On finite data the test is always true
     * and no value the function has ever produced moves. Only the batch path
     * needs it -- the streaming Update/Peek entry points reject a non-finite
     * bar with TA_BAD_PARAM before it reaches any accumulator.
     */
    /* The product is kept in its own statement so no compiler may contract it
     * into an FMA. Contracting here would make the C output disagree with the
     * Rust, Java and C# backends under the cross-language bitwise gate. Same
     * reason as in ta_codegen/input/vwma/vwma.c.
     *
     * Computed before the guard rather than inside it, and unconditionally,
     * so it stays a per-bar temporary. Assigned only on the taken arm it
     * would instead be live across bars, and the streaming tier would carry
     * it as a fourth state field in every handle -- 8 bytes to hold a value
     * no later bar reads. The multiply on a skipped bar is discarded.
     */
    tempReal = typPrice * volume;
    if ((Number.isFinite(typPrice)) && (Number.isFinite(volume))) {
      sumPV += tempReal;
      sumV += volume;
    }
    /* Bars that traded nothing carry no weight, so a zero-volume bar in
     * the middle of a series leaves both sums untouched and repeats the
     * previous value on its own -- no arm needed for that. A bar skipped
     * by the guard above repeats it for the same reason.
     *
     * The arm below is for the one case the ratio cannot express: a
     * leading run of bars before any volume has traded, where there are
     * no weights at all and the weighted mean is undefined. The last
     * value computed is carried forward instead, which is 0.0 until the
     * first bar with volume. Volume is non-negative, so once the divisor
     * leaves zero it never returns and this arm cannot fire again.
     *
     * A successful call therefore never emits NaN or Inf (issue #112),
     * which is the divergence from pandas-ta-classic and from
     * trading-signals: the first emits NaN there, the second no bar at
     * all. Testing sumV rather than the bar's own volume also keeps a
     * negative divisor -- which no non-negative volume series can
     * produce -- out of a price-scale output, as ta_CMF.c does.
     */
    if (sumV > 0.0) {
      vwap = sumPV / sumV;
    }
    outReal[((outIdx++ * outStride) | 0)] = vwap;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  sp.sumPV = sumPV;
  sp.sumV = sumV;
  sp.vwap = vwap;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function vwapOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): VwapStream {
  const sp = new VwapStream(core);
  const retCode = vwapOpenImpl(sp, inHigh, inLow, inClose, inVolume, startIdx, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('VWAP openAndFill', inHigh.length, startIdx, vwapLookback(core));
  }
  throw streamFailure('VWAP openAndFill', retCode);
}

/** @internal */
export function vwapOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
): VwapStream {
  const sp = new VwapStream(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = vwapOpenImpl(sp, inHigh, inLow, inClose, inVolume, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('VWAP open', inHigh.length, startIdx, vwapLookback(core));
  }
  throw streamFailure('VWAP open', retCode);
}

/**
 * Opens a live VWAP stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link vwap} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param inVolume - Warm-up history. Volume of each bar. Every history has the
 *   same length.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `vwapLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for histories of different lengths.
 *
 * @example
 * ```ts
 * import { vwapOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const stream = vwapOpen(high, low, close, volume);
 * const peeked = stream.peek(101.4, 99.1, 100.9, 12345); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9, 12345); // the bar closes
 * ```
 */
export function vwapOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): VwapStream {
  requireArgument('VWAP open', 'inHigh', inHigh);
  requireHistory('VWAP open', inHigh.length);
  requireArgument('VWAP open', 'inLow', inLow);
  requireArgument('VWAP open', 'inClose', inClose);
  requireArgument('VWAP open', 'inVolume', inVolume);
  requireHistoryLength('VWAP open', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('VWAP open', 'inClose', inClose.length, inHigh.length);
  requireHistoryLength('VWAP open', 'inVolume', inVolume.length, inHigh.length);
  return vwapOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), float64(inVolume), 0);
}

/**
 * {@link vwapOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link vwap}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param inVolume - Warm-up history. Volume of each bar. Every history has the
 *   same length.
 * @param outReal - Volume weighted average price, cumulative from the first bar of
 *   the range. Must hold `historyLen - lookback` values and share no memory with an
 *   input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `vwapLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for histories of different lengths or an output too
 *   short or starting where another output or a `Float64Array` input starts, or, on
 *   a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { vwapOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const out = new Float64Array(high.length);
 * const stream = vwapOpenAndFill(high, low, close, volume, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function vwapOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): VwapStream {
  requireArgument('VWAP openAndFill', 'inHigh', inHigh);
  requireHistory('VWAP openAndFill', inHigh.length);
  requireArgument('VWAP openAndFill', 'inLow', inLow);
  requireArgument('VWAP openAndFill', 'inClose', inClose);
  requireArgument('VWAP openAndFill', 'inVolume', inVolume);
  const guardOutLen = openFillCount('VWAP openAndFill', inHigh.length, vwapLookback(core));
  requireHistoryLength('VWAP openAndFill', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('VWAP openAndFill', 'inClose', inClose.length, inHigh.length);
  requireHistoryLength('VWAP openAndFill', 'inVolume', inVolume.length, inHigh.length);
  requireLength('VWAP openAndFill', 'outReal', outReal, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  const inVolumeF64 = float64(inVolume);
  if ((outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) || (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) || (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset) || (outReal.buffer === inVolumeF64.buffer && outReal.byteOffset === inVolumeF64.byteOffset)) {
    throw streamFailure('VWAP openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inHighF64) || overlaps(outReal, inLowF64) || overlaps(outReal, inCloseF64) || overlaps(outReal, inVolumeF64))) {
    throw streamFailure('VWAP openAndFill', RetCode.BAD_PARAM);
  }
  return vwapOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, inVolumeF64, 0, new MInteger(), new MInteger(), outReal);
}
