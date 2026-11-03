/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  082426 MF,CC Fix #255. Re-anchor the running sums: every 32*period bars,
 *               and on the bar a large value leaves the window.
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link wma} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Number of bars in the weighting window (default 30;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function wmaLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  return optInTimePeriod - 1;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link wma}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Number of bars in the weighting window (default 30;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function wmaDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (wmaLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function wmaImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let inIdx = 0;
  let outIdx = 0;
  let i = 0;
  let trailingIdx = 0;
  let j = 0;
  let rw = 0;
  let lookbackWin = 0;
  let barsSinceReseed = 0;
  let periodSum = 0;
  let periodSub = 0;
  let tempReal = 0;
  let trailingValue = 0;
  let divider = 0;
  let lookbackTotal = 0;
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
  lookbackTotal = optInTimePeriod - 1;
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
  /* To make the rest more efficient, handle exception
   * case where the user is asking for a period of '1'.
   * In that case outputs equals inputs for the requested
   * range.
   */
  if (optInTimePeriod === 1) {
    outBegIdx.value = startIdx;
    outNBElement.value = endIdx - startIdx + 1;
    /* Element loop, not a block copy: the C single-precision variant reads a
     * float array, so a double-sized byte copy would reinterpret and
     * over-read it (#137). Forward order keeps the in-place case correct (#94).
     */
    inIdx = startIdx;
    for (i = 0; i < outNBElement.value; i += 1) {
      outReal[i] = inReal[inIdx++];
    }
    return RetCode.SUCCESS;
  }
  /* Weighted denominator 1+2+...+n = n(n+1)/2. Computed in double: the
   * int product n*(n+1) overflows int32 at n>=46341 (#142).
   */
  divider = optInTimePeriod * (optInTimePeriod + 1) / 2.0;
  /* The algo used here use a very basic property of
   * multiplication/addition: (x*2) = x+x
   *
   * As an example, a 3 period weighted can be
   * interpreted in two way:
   *  (x1*1)+(x2*2)+(x3*3)
   *      OR
   *  x1+x2+x2+x3+x3+x3 (this is the periodSum)
   *
   * When you move forward in the time serie
   * you can quickly adjust the periodSum for the
   * period by substracting:
   *   x1+x2+x3 (This is the periodSub)
   * Making the new periodSum equals to:
   *   x2+x3+x3
   *
   * You can then add the new price bar
   * which is x4+x4+x4 giving:
   *   x2+x3+x3+x4+x4+x4
   *
   * At this point one iteration is completed and you can
   * see that we are back to the step 1 of this example.
   *
   * Why making it so un-intuitive? The number of memory
   * access and floating point operations are kept to a
   * minimum with this algo.
   */
  outIdx = 0;
  trailingIdx = startIdx - lookbackTotal;
  /* Evaluate the initial periodSum/periodSub and trailingValue. */
  lookbackWin = optInTimePeriod - 1;
  periodSub = 0.0;
  periodSum = periodSub;
  inIdx = trailingIdx;
  i = 1;
  while (inIdx < startIdx) {
    tempReal = inReal[inIdx];
    inIdx += 1;
    periodSub += tempReal;
    periodSum += tempReal * i;
    i += 1;
  }
  barsSinceReseed = ((8 * optInTimePeriod) | 0);
  trailingValue = 0.0;
  /* Tight loop for the requested range. */
  while (inIdx <= endIdx) {
    /* Add the current price bar to the sum
     * who are carried through the iterations.
     */
    tempReal = inReal[inIdx];
    periodSub += tempReal;
    periodSub -= trailingValue;
    periodSum += tempReal * optInTimePeriod;
    /* Re-anchor: rebuild both totals from the window itself.
     *
     * periodSum and periodSub were running totals that were never
     * recomputed, so each bar's rounding joined a residue no later bar
     * could subtract, and its size was set by the largest value the totals
     * had ever held rather than by the current window. That is the defect
     * #254 fixed in the LINEARREG family, and `periodSum -= periodSub`
     * below is the same weight-shifting identity as that family's
     * `SumXY = SumXY + SumY - period*trailingValue` -- which is why WMA has
     * it and TA_SMA, whose output lives at its own sum's scale, does not.
     * Measured before the fix: worst range disagreement 1.41e-08 at 200000
     * bars against a 1e-10 tier, over the tier from ~10000 bars on ordinary
     * closes or ~1000 with one large print. After: 1.79e-12, flat in call
     * length.
     *
     * ONE TRIGGER, NOT TWO, AND THE INTERVAL IS 8*period NOT 32. The
     * LINEARREG family also carries an OUTLIER trigger (rebuild when the
     * departing value outweighs the window) because for a slope the
     * interval alone FAILS the tier outright, at 2.38e-10. WMA is not in
     * that position: its weights are bounded by `period` and its divider is
     * period*(period+1)/2, which dilutes the residue enough that the
     * interval alone holds. Swept over periods 2, 3, 4, 14, 50, 200, 1000,
     * 5000 and 20000 on 60000 bars, clean and with a 1000x print, the worst
     * is 2.2e-11 -- 4.6x inside the band, and the margin does not thin at
     * either end of the period range. Measured, the trigger bought 1.4e-11 -> 7e-12 and cost 1.17x
     * here and 1.65x in TA_HMA, whose three fused stages each pay it. The
     * shorter interval buys most of the accuracy for ~1.1x instead.
     *
     * The rebuild walks the window OLDEST FIRST with the weight counting UP
     * from 1 -- the priming scan's own order and weighting -- so a
     * re-anchored bar is bit-identical to the same bar computed by a call
     * that started there. That identity is what the range-stability
     * contract measures, and what test_wma.c W2/W3 assert.
     *
     * The loop start is written INLINE rather than through a `windowStart`
     * local: only that form is recognised as a rescan window, which is what
     * keeps this on the stream classifier's primary path. See
     * docs/ta_codegen_input_code.md.
     *
     * Reading the window is safe when outReal aliases inReal: the outputs
     * written so far occupy [0, outIdx-1], and the window starts at
     * startIdx-lookbackTotal+outIdx, which is >= outIdx.
     */
    barsSinceReseed -= 1;
    if (barsSinceReseed <= 0) {
      barsSinceReseed = ((8 * optInTimePeriod) | 0);
      periodSub = 0.0;
      periodSum = 0.0;
      rw = 1;
      for (j = inIdx - lookbackWin; j <= inIdx; j += 1) {
        tempReal = inReal[j];
        periodSub += tempReal;
        periodSum += tempReal * rw;
        rw += 1;
      }
    }
    /* Save the trailing value for being substract at
     * the next iteration.
     * (must be saved here just in case outReal and
     *  inReal are the same buffer).
     */
    trailingValue = inReal[trailingIdx];
    trailingIdx += 1;
    /* Calculate the WMA for this price bar. */
    outReal[outIdx++] = periodSum / divider;
    /* Prepare the periodSum for the next iteration. */
    periodSum -= periodSub;
    inIdx += 1;
  }
  /* Set output limits. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/**
 * Linearly weighted moving average: each of the last N prices is weighted by its
 * position, oldest getting weight 1 and newest weight N. Smooths price while
 * emphasizing recent bars.
 *
 * Formula and more info at
 * [ta-lib.org/functions/wma](https://ta-lib.org/functions/wma).
 *
 * **Notes**
 * - A period of 1 performs no smoothing: the output is a copy of the input.
 *   Allowed since 0.6.5 (issues #48/#59).
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link wmaLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - Source price/data series.
 * @param optInTimePeriod - Number of bars in the weighting window (default 30;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Weighted moving average series. Must hold the values produced
 *   and never be empty; the input length always suffices.
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
 * import { wma } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const { begIdx, count } = wma(0, data.length - 1, data, 30, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`sma`](https://ta-lib.org/functions/sma)
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`ma`](https://ta-lib.org/functions/ma)
 * @see [`dema`](https://ta-lib.org/functions/dema)
 * @see [`tema`](https://ta-lib.org/functions/tema)
 */
export function wma(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('WMA', 'startIdx', startIdx);
  endIdx = requireInt32('WMA', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('WMA', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('WMA', startIdx, endIdx);
  const guardStart = clampedStart('WMA', startIdx, wmaLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('WMA', 'inReal', inReal, guardInLen);
  requireLength('WMA', 'outReal', outReal, guardOutLen);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw failure('WMA', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = wmaImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('WMA', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live WMA stream: one value per closed bar, bit-identical to {@link wma} over
 * the same series. Open it with {@link wmaOpen} or {@link wmaOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/wma](https://ta-lib.org/functions/wma).
 */
export class WmaStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  lookbackWin: number = 0;
  /** @internal */
  barsSinceReseed: number = 0;
  /** @internal */
  periodSum: number = 0;
  /** @internal */
  periodSub: number = 0;
  /** @internal */
  trailingValue: number = 0;
  /** @internal */
  divider: number = 0;
  /** @internal */
  ringPos_trailingIdx: number = 0;
  /** @internal */
  ringCap_trailingIdx: number = 0;
  /** @internal */
  ring_trailingIdx_inReal: Float64Array = new Float64Array(1);
  /** @internal */
  winPos_j: number = 0;
  /** @internal */
  winCap_j: number = 1;
  /** @internal */
  win_j_inReal: Float64Array = new Float64Array(1);
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
   * {@link wma} reports them over the same bars. Each accepted `update` and each
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
      throw failure('WMA advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: Source price/data series.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('WMA update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('WMA update', 'inReal');
    }
    wmaStepImpl(this, inReal);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Source price/data series.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number): number {
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('WMA peek', 'inReal');
    }
    const sp = this;
    let j = 0;
    let rw = 0;
    let tempReal = 0;
    let barsSinceReseed = sp.barsSinceReseed;
    let cur_outReal = 0;
    let periodSum = sp.periodSum;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    if (sp.optInTimePeriod === 1) {
      cur_outReal = inReal;
      return cur_outReal;
    }
    pkSlot0 = sp.winPos_j;
    pkVal0 = inReal;
    /* Add the current price bar to the sum
     * who are carried through the iterations.
     */
    tempReal = inReal;
    periodSum += tempReal * sp.optInTimePeriod;
    /* Re-anchor: rebuild both totals from the window itself.
     *
     * periodSum and periodSub were running totals that were never
     * recomputed, so each bar's rounding joined a residue no later bar
     * could subtract, and its size was set by the largest value the totals
     * had ever held rather than by the current window. That is the defect
     * #254 fixed in the LINEARREG family, and `periodSum -= periodSub`
     * below is the same weight-shifting identity as that family's
     * `SumXY = SumXY + SumY - period*trailingValue` -- which is why WMA has
     * it and TA_SMA, whose output lives at its own sum's scale, does not.
     * Measured before the fix: worst range disagreement 1.41e-08 at 200000
     * bars against a 1e-10 tier, over the tier from ~10000 bars on ordinary
     * closes or ~1000 with one large print. After: 1.79e-12, flat in call
     * length.
     *
     * ONE TRIGGER, NOT TWO, AND THE INTERVAL IS 8*period NOT 32. The
     * LINEARREG family also carries an OUTLIER trigger (rebuild when the
     * departing value outweighs the window) because for a slope the
     * interval alone FAILS the tier outright, at 2.38e-10. WMA is not in
     * that position: its weights are bounded by `period` and its divider is
     * period*(period+1)/2, which dilutes the residue enough that the
     * interval alone holds. Swept over periods 2, 3, 4, 14, 50, 200, 1000,
     * 5000 and 20000 on 60000 bars, clean and with a 1000x print, the worst
     * is 2.2e-11 -- 4.6x inside the band, and the margin does not thin at
     * either end of the period range. Measured, the trigger bought 1.4e-11 -> 7e-12 and cost 1.17x
     * here and 1.65x in TA_HMA, whose three fused stages each pay it. The
     * shorter interval buys most of the accuracy for ~1.1x instead.
     *
     * The rebuild walks the window OLDEST FIRST with the weight counting UP
     * from 1 -- the priming scan's own order and weighting -- so a
     * re-anchored bar is bit-identical to the same bar computed by a call
     * that started there. That identity is what the range-stability
     * contract measures, and what test_wma.c W2/W3 assert.
     *
     * The loop start is written INLINE rather than through a `windowStart`
     * local: only that form is recognised as a rescan window, which is what
     * keeps this on the stream classifier's primary path. See
     * docs/ta_codegen_input_code.md.
     *
     * Reading the window is safe when outReal aliases inReal: the outputs
     * written so far occupy [0, outIdx-1], and the window starts at
     * startIdx-lookbackTotal+outIdx, which is >= outIdx.
     */
    barsSinceReseed -= 1;
    if (barsSinceReseed <= 0) {
      barsSinceReseed = ((8 * sp.optInTimePeriod) | 0);
      periodSum = 0.0;
      rw = 1;
      for (j = sp.lookbackWin; j >= 0; j -= 1) {
        tempReal = (((sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j) !== pkSlot0) ? sp.win_j_inReal[(sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j] : pkVal0;
        periodSum += tempReal * rw;
        rw += 1;
      }
    }
    /* Save the trailing value for being substract at
     * the next iteration.
     * (must be saved here just in case outReal and
     *  inReal are the same buffer).
     */
    /* Calculate the WMA for this price bar. */
    cur_outReal = periodSum / sp.divider;
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
  clone(): WmaStream {
    const c = new WmaStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.lookbackWin = this.lookbackWin;
    c.barsSinceReseed = this.barsSinceReseed;
    c.periodSum = this.periodSum;
    c.periodSub = this.periodSub;
    c.trailingValue = this.trailingValue;
    c.divider = this.divider;
    c.ringPos_trailingIdx = this.ringPos_trailingIdx;
    c.ringCap_trailingIdx = this.ringCap_trailingIdx;
    c.ring_trailingIdx_inReal = this.ring_trailingIdx_inReal.slice();
    c.winPos_j = this.winPos_j;
    c.winCap_j = this.winCap_j;
    c.win_j_inReal = this.win_j_inReal.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function wmaStepImpl(sp: WmaStream, inReal: number): void {
  let j = 0;
  let rw = 0;
  let tempReal = 0;
  let ringCapL_trailingIdx = 0;
  if (sp.optInTimePeriod === 1) {
    sp.cur_outReal = inReal;
    return;
  }
  sp.win_j_inReal[sp.winPos_j] = inReal;
  /* Add the current price bar to the sum
   * who are carried through the iterations.
   */
  tempReal = inReal;
  sp.periodSub += tempReal;
  sp.periodSub -= sp.trailingValue;
  sp.periodSum += tempReal * sp.optInTimePeriod;
  /* Re-anchor: rebuild both totals from the window itself.
   *
   * periodSum and periodSub were running totals that were never
   * recomputed, so each bar's rounding joined a residue no later bar
   * could subtract, and its size was set by the largest value the totals
   * had ever held rather than by the current window. That is the defect
   * #254 fixed in the LINEARREG family, and `periodSum -= periodSub`
   * below is the same weight-shifting identity as that family's
   * `SumXY = SumXY + SumY - period*trailingValue` -- which is why WMA has
   * it and TA_SMA, whose output lives at its own sum's scale, does not.
   * Measured before the fix: worst range disagreement 1.41e-08 at 200000
   * bars against a 1e-10 tier, over the tier from ~10000 bars on ordinary
   * closes or ~1000 with one large print. After: 1.79e-12, flat in call
   * length.
   *
   * ONE TRIGGER, NOT TWO, AND THE INTERVAL IS 8*period NOT 32. The
   * LINEARREG family also carries an OUTLIER trigger (rebuild when the
   * departing value outweighs the window) because for a slope the
   * interval alone FAILS the tier outright, at 2.38e-10. WMA is not in
   * that position: its weights are bounded by `period` and its divider is
   * period*(period+1)/2, which dilutes the residue enough that the
   * interval alone holds. Swept over periods 2, 3, 4, 14, 50, 200, 1000,
   * 5000 and 20000 on 60000 bars, clean and with a 1000x print, the worst
   * is 2.2e-11 -- 4.6x inside the band, and the margin does not thin at
   * either end of the period range. Measured, the trigger bought 1.4e-11 -> 7e-12 and cost 1.17x
   * here and 1.65x in TA_HMA, whose three fused stages each pay it. The
   * shorter interval buys most of the accuracy for ~1.1x instead.
   *
   * The rebuild walks the window OLDEST FIRST with the weight counting UP
   * from 1 -- the priming scan's own order and weighting -- so a
   * re-anchored bar is bit-identical to the same bar computed by a call
   * that started there. That identity is what the range-stability
   * contract measures, and what test_wma.c W2/W3 assert.
   *
   * The loop start is written INLINE rather than through a `windowStart`
   * local: only that form is recognised as a rescan window, which is what
   * keeps this on the stream classifier's primary path. See
   * docs/ta_codegen_input_code.md.
   *
   * Reading the window is safe when outReal aliases inReal: the outputs
   * written so far occupy [0, outIdx-1], and the window starts at
   * startIdx-lookbackTotal+outIdx, which is >= outIdx.
   */
  sp.barsSinceReseed -= 1;
  if (sp.barsSinceReseed <= 0) {
    sp.barsSinceReseed = ((8 * sp.optInTimePeriod) | 0);
    sp.periodSub = 0.0;
    sp.periodSum = 0.0;
    rw = 1;
    for (j = sp.lookbackWin; j >= 0; j -= 1) {
      tempReal = sp.win_j_inReal[(sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j];
      sp.periodSub += tempReal;
      sp.periodSum += tempReal * rw;
      rw += 1;
    }
  }
  /* Save the trailing value for being substract at
   * the next iteration.
   * (must be saved here just in case outReal and
   *  inReal are the same buffer).
   */
  sp.trailingValue = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
  /* Calculate the WMA for this price bar. */
  sp.cur_outReal = sp.periodSum / sp.divider;
  /* Prepare the periodSum for the next iteration. */
  sp.periodSum -= sp.periodSub;
  ringCapL_trailingIdx = sp.ringCap_trailingIdx;
  sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
  sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
  if (sp.ringPos_trailingIdx >= ringCapL_trailingIdx) {
    sp.ringPos_trailingIdx = 0;
  }
  sp.winPos_j = sp.winPos_j + 1;
  if (sp.winPos_j >= sp.winCap_j) {
    sp.winPos_j = 0;
  }
}

function wmaOpenImpl(
  sp: WmaStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let inIdx = 0;
  let outIdx = 0;
  let i = 0;
  let trailingIdx = 0;
  let j = 0;
  let rw = 0;
  let lookbackWin = 0;
  let barsSinceReseed = 0;
  let periodSum = 0;
  let periodSub = 0;
  let tempReal = 0;
  let trailingValue = 0;
  let divider = 0;
  let lookbackTotal = 0;
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
    let fillLb = wmaLookback(optInTimePeriod, core);
    if (startIdx > fillLb) {
      fillLb = startIdx;
    }
    if (historyLen < fillLb + 1) {
      return RetCode.INSUFFICIENT_HISTORY;
    }
    sp.optInTimePeriod = optInTimePeriod;
    sp.lookbackWin = 0;
    sp.barsSinceReseed = 0;
    sp.periodSum = 0;
    sp.periodSub = 0;
    sp.trailingValue = 0;
    sp.divider = 0;
    sp.ringPos_trailingIdx = 0;
    sp.ringCap_trailingIdx = 0;
    sp.ring_trailingIdx_inReal = new Float64Array(1);
    sp.winPos_j = 0;
    sp.winCap_j = 1;
    sp.win_j_inReal = new Float64Array(1);
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
  lookbackTotal = optInTimePeriod - 1;
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
  /* Weighted denominator 1+2+...+n = n(n+1)/2. Computed in double: the
   * int product n*(n+1) overflows int32 at n>=46341 (#142).
   */
  divider = optInTimePeriod * (optInTimePeriod + 1) / 2.0;
  /* The algo used here use a very basic property of
   * multiplication/addition: (x*2) = x+x
   *
   * As an example, a 3 period weighted can be
   * interpreted in two way:
   *  (x1*1)+(x2*2)+(x3*3)
   *      OR
   *  x1+x2+x2+x3+x3+x3 (this is the periodSum)
   *
   * When you move forward in the time serie
   * you can quickly adjust the periodSum for the
   * period by substracting:
   *   x1+x2+x3 (This is the periodSub)
   * Making the new periodSum equals to:
   *   x2+x3+x3
   *
   * You can then add the new price bar
   * which is x4+x4+x4 giving:
   *   x2+x3+x3+x4+x4+x4
   *
   * At this point one iteration is completed and you can
   * see that we are back to the step 1 of this example.
   *
   * Why making it so un-intuitive? The number of memory
   * access and floating point operations are kept to a
   * minimum with this algo.
   */
  outIdx = 0;
  trailingIdx = startIdx - lookbackTotal;
  /* Evaluate the initial periodSum/periodSub and trailingValue. */
  lookbackWin = optInTimePeriod - 1;
  periodSub = 0.0;
  periodSum = periodSub;
  inIdx = trailingIdx;
  i = 1;
  while (inIdx < startIdx) {
    tempReal = inReal[inIdx];
    inIdx += 1;
    periodSub += tempReal;
    periodSum += tempReal * i;
    i += 1;
  }
  barsSinceReseed = ((8 * optInTimePeriod) | 0);
  trailingValue = 0.0;
  /* Tight loop for the requested range. */
  while (inIdx <= endIdx) {
    /* Add the current price bar to the sum
     * who are carried through the iterations.
     */
    tempReal = inReal[inIdx];
    periodSub += tempReal;
    periodSub -= trailingValue;
    periodSum += tempReal * optInTimePeriod;
    /* Re-anchor: rebuild both totals from the window itself.
     *
     * periodSum and periodSub were running totals that were never
     * recomputed, so each bar's rounding joined a residue no later bar
     * could subtract, and its size was set by the largest value the totals
     * had ever held rather than by the current window. That is the defect
     * #254 fixed in the LINEARREG family, and `periodSum -= periodSub`
     * below is the same weight-shifting identity as that family's
     * `SumXY = SumXY + SumY - period*trailingValue` -- which is why WMA has
     * it and TA_SMA, whose output lives at its own sum's scale, does not.
     * Measured before the fix: worst range disagreement 1.41e-08 at 200000
     * bars against a 1e-10 tier, over the tier from ~10000 bars on ordinary
     * closes or ~1000 with one large print. After: 1.79e-12, flat in call
     * length.
     *
     * ONE TRIGGER, NOT TWO, AND THE INTERVAL IS 8*period NOT 32. The
     * LINEARREG family also carries an OUTLIER trigger (rebuild when the
     * departing value outweighs the window) because for a slope the
     * interval alone FAILS the tier outright, at 2.38e-10. WMA is not in
     * that position: its weights are bounded by `period` and its divider is
     * period*(period+1)/2, which dilutes the residue enough that the
     * interval alone holds. Swept over periods 2, 3, 4, 14, 50, 200, 1000,
     * 5000 and 20000 on 60000 bars, clean and with a 1000x print, the worst
     * is 2.2e-11 -- 4.6x inside the band, and the margin does not thin at
     * either end of the period range. Measured, the trigger bought 1.4e-11 -> 7e-12 and cost 1.17x
     * here and 1.65x in TA_HMA, whose three fused stages each pay it. The
     * shorter interval buys most of the accuracy for ~1.1x instead.
     *
     * The rebuild walks the window OLDEST FIRST with the weight counting UP
     * from 1 -- the priming scan's own order and weighting -- so a
     * re-anchored bar is bit-identical to the same bar computed by a call
     * that started there. That identity is what the range-stability
     * contract measures, and what test_wma.c W2/W3 assert.
     *
     * The loop start is written INLINE rather than through a `windowStart`
     * local: only that form is recognised as a rescan window, which is what
     * keeps this on the stream classifier's primary path. See
     * docs/ta_codegen_input_code.md.
     *
     * Reading the window is safe when outReal aliases inReal: the outputs
     * written so far occupy [0, outIdx-1], and the window starts at
     * startIdx-lookbackTotal+outIdx, which is >= outIdx.
     */
    barsSinceReseed -= 1;
    if (barsSinceReseed <= 0) {
      barsSinceReseed = ((8 * optInTimePeriod) | 0);
      periodSub = 0.0;
      periodSum = 0.0;
      rw = 1;
      for (j = inIdx - lookbackWin; j <= inIdx; j += 1) {
        tempReal = inReal[j];
        periodSub += tempReal;
        periodSum += tempReal * rw;
        rw += 1;
      }
    }
    /* Save the trailing value for being substract at
     * the next iteration.
     * (must be saved here just in case outReal and
     *  inReal are the same buffer).
     */
    trailingValue = inReal[trailingIdx];
    trailingIdx += 1;
    /* Calculate the WMA for this price bar. */
    outReal[((outIdx++ * outStride) | 0)] = periodSum / divider;
    /* Prepare the periodSum for the next iteration. */
    periodSum -= periodSub;
    inIdx += 1;
  }
  /* Set output limits. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const cap_trailingIdx = inIdx - trailingIdx;
  if (cap_trailingIdx < 1 || cap_trailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_trailingIdx = cap_trailingIdx > 0 ? cap_trailingIdx : 1;
  const capRing_trailingIdx_inReal = new Float64Array(allocN_trailingIdx);
  capRing_trailingIdx_inReal.set(inReal.subarray(historyLen - cap_trailingIdx, historyLen), 0);
  const cap_j = lookbackWin + 1;
  if (cap_j < 1 || cap_j > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const capWin_j_inReal = inReal.slice(historyLen - cap_j, historyLen);
  sp.optInTimePeriod = optInTimePeriod;
  sp.lookbackWin = lookbackWin;
  sp.barsSinceReseed = barsSinceReseed;
  sp.periodSum = periodSum;
  sp.periodSub = periodSub;
  sp.trailingValue = trailingValue;
  sp.divider = divider;
  sp.ringPos_trailingIdx = 0;
  sp.ringCap_trailingIdx = cap_trailingIdx;
  sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
  sp.winPos_j = 0;
  sp.winCap_j = cap_j;
  sp.win_j_inReal = capWin_j_inReal;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function wmaOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): WmaStream {
  const sp = new WmaStream(core);
  const retCode = wmaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('WMA openAndFill', inReal.length, startIdx, wmaLookback(optInTimePeriod, core));
  }
  throw streamFailure('WMA openAndFill', retCode);
}

/** @internal */
export function wmaOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): WmaStream {
  const sp = new WmaStream(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = wmaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('WMA open', inReal.length, startIdx, wmaLookback(optInTimePeriod, core));
  }
  throw streamFailure('WMA open', retCode);
}

/**
 * Opens a live WMA stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link wma} there.
 *
 * @param inReal - Warm-up history. Source price/data series.
 * @param optInTimePeriod - Number of bars in the weighting window (default 30;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `wmaLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32 or a
 *   parameter outside its range.
 *
 * @example
 * ```ts
 * import { wmaOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = wmaOpen(data, 30);
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function wmaOpen(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): WmaStream {
  optInTimePeriod = requireInt32('WMA open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('WMA open', 'inReal', inReal);
  requireHistory('WMA open', inReal.length);
  return wmaOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/**
 * {@link wmaOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link wma}. The range written is the stream's
 * `outRange()`.
 *
 * @param inReal - Warm-up history. Source price/data series.
 * @param optInTimePeriod - Number of bars in the weighting window (default 30;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Weighted moving average series. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `wmaLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or an output too short or starting where another
 *   output or a `Float64Array` input starts, or, on a history long enough to open,
 *   one sharing memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { wmaOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const stream = wmaOpenAndFill(data, 30, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function wmaOpenAndFill(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): WmaStream {
  optInTimePeriod = requireInt32('WMA openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('WMA openAndFill', 'inReal', inReal);
  requireHistory('WMA openAndFill', inReal.length);
  const guardOutLen = openFillCount('WMA openAndFill', inReal.length, wmaLookback(optInTimePeriod, core));
  requireLength('WMA openAndFill', 'outReal', outReal, guardOutLen);
  const inRealF64 = float64(inReal);
  if ((outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset)) {
    throw streamFailure('WMA openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inRealF64))) {
    throw streamFailure('WMA openAndFill', RetCode.BAD_PARAM);
  }
  return wmaOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/** @internal */
export function wmaStepTape(sp: WmaStream, tape: Float64Array, tapeBase: number, tapeMask: number, inReal: number): number {
  let j = 0;
  let rw = 0;
  let tempReal = 0;
  /* Add the current price bar to the sum
   * who are carried through the iterations.
   */
  tempReal = inReal;
  sp.periodSub += tempReal;
  sp.periodSub -= sp.trailingValue;
  sp.periodSum += tempReal * sp.optInTimePeriod;
  /* Re-anchor: rebuild both totals from the window itself.
   *
   * periodSum and periodSub were running totals that were never
   * recomputed, so each bar's rounding joined a residue no later bar
   * could subtract, and its size was set by the largest value the totals
   * had ever held rather than by the current window. That is the defect
   * #254 fixed in the LINEARREG family, and `periodSum -= periodSub`
   * below is the same weight-shifting identity as that family's
   * `SumXY = SumXY + SumY - period*trailingValue` -- which is why WMA has
   * it and TA_SMA, whose output lives at its own sum's scale, does not.
   * Measured before the fix: worst range disagreement 1.41e-08 at 200000
   * bars against a 1e-10 tier, over the tier from ~10000 bars on ordinary
   * closes or ~1000 with one large print. After: 1.79e-12, flat in call
   * length.
   *
   * ONE TRIGGER, NOT TWO, AND THE INTERVAL IS 8*period NOT 32. The
   * LINEARREG family also carries an OUTLIER trigger (rebuild when the
   * departing value outweighs the window) because for a slope the
   * interval alone FAILS the tier outright, at 2.38e-10. WMA is not in
   * that position: its weights are bounded by `period` and its divider is
   * period*(period+1)/2, which dilutes the residue enough that the
   * interval alone holds. Swept over periods 2, 3, 4, 14, 50, 200, 1000,
   * 5000 and 20000 on 60000 bars, clean and with a 1000x print, the worst
   * is 2.2e-11 -- 4.6x inside the band, and the margin does not thin at
   * either end of the period range. Measured, the trigger bought 1.4e-11 -> 7e-12 and cost 1.17x
   * here and 1.65x in TA_HMA, whose three fused stages each pay it. The
   * shorter interval buys most of the accuracy for ~1.1x instead.
   *
   * The rebuild walks the window OLDEST FIRST with the weight counting UP
   * from 1 -- the priming scan's own order and weighting -- so a
   * re-anchored bar is bit-identical to the same bar computed by a call
   * that started there. That identity is what the range-stability
   * contract measures, and what test_wma.c W2/W3 assert.
   *
   * The loop start is written INLINE rather than through a `windowStart`
   * local: only that form is recognised as a rescan window, which is what
   * keeps this on the stream classifier's primary path. See
   * docs/ta_codegen_input_code.md.
   *
   * Reading the window is safe when outReal aliases inReal: the outputs
   * written so far occupy [0, outIdx-1], and the window starts at
   * startIdx-lookbackTotal+outIdx, which is >= outIdx.
   */
  sp.barsSinceReseed -= 1;
  if (sp.barsSinceReseed <= 0) {
    sp.barsSinceReseed = ((8 * sp.optInTimePeriod) | 0);
    sp.periodSub = 0.0;
    sp.periodSum = 0.0;
    rw = 1;
    for (j = sp.lookbackWin; j >= 0; j -= 1) {
      tempReal = tape[(tapeBase - j) & tapeMask];
      sp.periodSub += tempReal;
      sp.periodSum += tempReal * rw;
      rw += 1;
    }
  }
  /* Save the trailing value for being substract at
   * the next iteration.
   * (must be saved here just in case outReal and
   *  inReal are the same buffer).
   */
  sp.trailingValue = tape[(tapeBase - sp.ringCap_trailingIdx) & tapeMask];
  /* Calculate the WMA for this price bar. */
  sp.cur_outReal = sp.periodSum / sp.divider;
  /* Prepare the periodSum for the next iteration. */
  sp.periodSum -= sp.periodSub;
  sp.outRangeCount++;
  return sp.cur_outReal;
}

/** @internal */
export function wmaPeekTape(sp: WmaStream, tape: Float64Array, tapeBase: number, tapeMask: number, inReal: number): number {
  let j = 0;
  let rw = 0;
  let tempReal = 0;
  let barsSinceReseed = sp.barsSinceReseed;
  let cur_outReal = 0;
  let periodSum = sp.periodSum;
  let pkSlot0 = -1;
  let pkVal0 = 0;
  pkSlot0 = tapeBase & tapeMask;
  pkVal0 = inReal;
  /* Add the current price bar to the sum
   * who are carried through the iterations.
   */
  tempReal = inReal;
  periodSum += tempReal * sp.optInTimePeriod;
  /* Re-anchor: rebuild both totals from the window itself.
   *
   * periodSum and periodSub were running totals that were never
   * recomputed, so each bar's rounding joined a residue no later bar
   * could subtract, and its size was set by the largest value the totals
   * had ever held rather than by the current window. That is the defect
   * #254 fixed in the LINEARREG family, and `periodSum -= periodSub`
   * below is the same weight-shifting identity as that family's
   * `SumXY = SumXY + SumY - period*trailingValue` -- which is why WMA has
   * it and TA_SMA, whose output lives at its own sum's scale, does not.
   * Measured before the fix: worst range disagreement 1.41e-08 at 200000
   * bars against a 1e-10 tier, over the tier from ~10000 bars on ordinary
   * closes or ~1000 with one large print. After: 1.79e-12, flat in call
   * length.
   *
   * ONE TRIGGER, NOT TWO, AND THE INTERVAL IS 8*period NOT 32. The
   * LINEARREG family also carries an OUTLIER trigger (rebuild when the
   * departing value outweighs the window) because for a slope the
   * interval alone FAILS the tier outright, at 2.38e-10. WMA is not in
   * that position: its weights are bounded by `period` and its divider is
   * period*(period+1)/2, which dilutes the residue enough that the
   * interval alone holds. Swept over periods 2, 3, 4, 14, 50, 200, 1000,
   * 5000 and 20000 on 60000 bars, clean and with a 1000x print, the worst
   * is 2.2e-11 -- 4.6x inside the band, and the margin does not thin at
   * either end of the period range. Measured, the trigger bought 1.4e-11 -> 7e-12 and cost 1.17x
   * here and 1.65x in TA_HMA, whose three fused stages each pay it. The
   * shorter interval buys most of the accuracy for ~1.1x instead.
   *
   * The rebuild walks the window OLDEST FIRST with the weight counting UP
   * from 1 -- the priming scan's own order and weighting -- so a
   * re-anchored bar is bit-identical to the same bar computed by a call
   * that started there. That identity is what the range-stability
   * contract measures, and what test_wma.c W2/W3 assert.
   *
   * The loop start is written INLINE rather than through a `windowStart`
   * local: only that form is recognised as a rescan window, which is what
   * keeps this on the stream classifier's primary path. See
   * docs/ta_codegen_input_code.md.
   *
   * Reading the window is safe when outReal aliases inReal: the outputs
   * written so far occupy [0, outIdx-1], and the window starts at
   * startIdx-lookbackTotal+outIdx, which is >= outIdx.
   */
  barsSinceReseed -= 1;
  if (barsSinceReseed <= 0) {
    barsSinceReseed = ((8 * sp.optInTimePeriod) | 0);
    periodSum = 0.0;
    rw = 1;
    for (j = sp.lookbackWin; j >= 0; j -= 1) {
      tempReal = (((tapeBase - j) & tapeMask) !== pkSlot0) ? tape[(tapeBase - j) & tapeMask] : pkVal0;
      periodSum += tempReal * rw;
      rw += 1;
    }
  }
  /* Save the trailing value for being substract at
   * the next iteration.
   * (must be saved here just in case outReal and
   *  inReal are the same buffer).
   */
  /* Calculate the WMA for this price bar. */
  cur_outReal = periodSum / sp.divider;
  return cur_outReal;
}

/** @internal */
export function wmaTapeDetach(sp: WmaStream): number {
  let reach = 0;
  sp.ring_trailingIdx_inReal = new Float64Array(0);
  if (sp.ringCap_trailingIdx > reach) {
    reach = sp.ringCap_trailingIdx;
  }
  sp.win_j_inReal = new Float64Array(0);
  if (sp.winCap_j - 1 > reach) {
    reach = sp.winCap_j - 1;
  }
  return reach;
}
