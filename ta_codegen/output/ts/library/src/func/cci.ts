/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AF       Alexander Trufanov (github @trufanov-nok)
 *  CC       Claude Code (AI assistant)
 *
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  031202 MF     Template creation.
 *  052603 MF     Port to managed C++. Change to use CIRCBUF macros.
 *  061704 MF     Lower limit for period to 2, and correct algorithm
 *                to avoid cummulative error when value are close to
 *                the floating point epsilon.
 *  070626 AF,CC  Guard the final division with TA_IS_ZERO instead of an exact
 *                "!= 0.0" check: identical prices over the period leave
 *                sub-epsilon residue that the exact check divided into a
 *                spurious value (issue #7 / SF bug #107). Now returns 0.0.
 *  082326 MF,CC  Fix #253. Scale that flatness test to the window's own price
 *                level: the fixed band zeroed the whole output for any
 *                instrument quoted small enough to fall under it.
 *  090626 MF,CC  Fix #395. Test the divisor itself, not just the deviation it
 *                scales: `0.015*tempReal2` underflows to 0.0 on a denormal
 *                price the deviation's own band still calls "not flat", and
 *                the division returned +/-Inf under TA_SUCCESS.
 *  092826 MF,CC  Sum the window around the slot just stored, taking it from
 *                lastValue: a wide load over that slot waited for the store
 *                (#455). Same order, same values.
 *  102626 MF,CC  The window average is a running sum, rebuilt every period
 *                and whenever it falls well below its peak or an
 *                identical-price window holds a changed sum (#560).
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link cci} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Number of bars in the averaging/deviation window
 *   (default 14; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function cciLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  void core;
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

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cci}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Number of bars in the averaging/deviation window
 *   (default 14; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function cciDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (cciLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cciImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let tempReal = 0;
  let tempReal2 = 0;
  let tempReal3 = 0;
  let theAverage = 0;
  let lastValue = 0;
  let periodTotal = 0;
  let periodPeak = 0;
  let prevValue = 0;
  let i = 0;
  let j = 0;
  let k = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let sumDirty = 0;
  let flatRun = 0;
  let circBuffer!: Float64Array;
  let circBuffer_Idx = 0;
  let maxIdx_circBuffer = (30) - 1;
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
  /* This ptr will points on a circular buffer of
   * at least "optInTimePeriod" element.
   */
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
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
  /* Allocate a circular buffer equal to the requested
   * period.
   */
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  circBuffer = new Float64Array(optInTimePeriod);
  maxIdx_circBuffer = optInTimePeriod - 1;
  circBuffer_Idx = 0;
  /* Do the MA calculation using tight loops. */
  /* Add-up the initial period, except for the last value.
   * Fill up the circular buffer at the same time.
   */
  i = startIdx - lookbackTotal;
  if (optInTimePeriod > 1) {
    while (i < startIdx) {
      circBuffer[circBuffer_Idx] = (inHigh[i] + inLow[i] + inClose[i]) / 3;
      i += 1;
      circBuffer_Idx++;
      if (circBuffer_Idx > maxIdx_circBuffer) {
        circBuffer_Idx = 0;
      }
    }
  }
  /* The run of bars equal to the one before them, ending on the last bar
   * filled above.
   */
  prevValue = (inHigh[startIdx - 1] + inLow[startIdx - 1] + inClose[startIdx - 1]) / 3;
  flatRun = 0;
  k = startIdx - 1;
  while (k > startIdx - lookbackTotal && (inHigh[k - 1] + inLow[k - 1] + inClose[k - 1]) / 3 === prevValue) {
    flatRun += 1;
    k -= 1;
  }
  /* Proceed with the calculation for the requested range.
   * Note that this algorithm allows the inReal and
   * outReal to be the same buffer.
   */
  outIdx = 0;
  periodTotal = 0.0;
  periodPeak = 0.0;
  sumDirty = 0;
  do {
    lastValue = (inHigh[i] + inLow[i] + inClose[i]) / 3;
    if (lastValue === prevValue) {
      flatRun += +(flatRun < optInTimePeriod);
    } else {
      flatRun = 0;
    }
    prevValue = lastValue;
    /* A running sum carries the rounding of every bar it took and gave
     * back, at the scale of the sum when it did. The full pass on the bar
     * that stores into the last slot, and whenever the sum falls below 2/3
     * of the largest it held since the last pass, keeps that rounding at the
     * scale of the current window. An identical-price window must still
     * average to its own value, so it takes the full pass too unless the
     * sum took no change since its last one.
     */
    if (circBuffer_Idx !== optInTimePeriod - 1 && lastValue !== circBuffer[circBuffer_Idx]) {
      periodTotal += lastValue - circBuffer[circBuffer_Idx];
      sumDirty = 1;
    }
    circBuffer[circBuffer_Idx] = lastValue;
    tempReal = Math.abs(periodTotal);
    periodPeak = (tempReal > periodPeak) ? tempReal : periodPeak;
    if (circBuffer_Idx === optInTimePeriod - 1 || periodPeak > 1.5 * tempReal || sumDirty !== 0 && flatRun >= optInTimePeriod - 1) {
      tempReal = 0;
      for (j = 0; j < circBuffer_Idx; j += 1) {
        tempReal += circBuffer[j];
      }
      tempReal += lastValue;
      for (j = circBuffer_Idx + 1; j < optInTimePeriod; j += 1) {
        tempReal += circBuffer[j];
      }
      periodTotal = tempReal;
      periodPeak = Math.abs(tempReal);
      sumDirty = 0;
    }
    theAverage = periodTotal / optInTimePeriod;
    /* Do the summation of the ABS(TypePrice-average)
     * for the whole period, then its mean.
     */
    tempReal2 = 0;
    for (j = 0; j < circBuffer_Idx; j += 1) {
      tempReal2 += Math.abs(circBuffer[j] - theAverage);
    }
    tempReal2 += Math.abs(lastValue - theAverage);
    for (j = circBuffer_Idx + 1; j < optInTimePeriod; j += 1) {
      tempReal2 += Math.abs(circBuffer[j] - theAverage);
    }
    tempReal2 /= optInTimePeriod;
    /* And finally, the CCI... */
    tempReal = lastValue - theAverage;
    /* The third test is the divisor itself, and it is not implied by the
     * second: the deviation's band is RELATIVE and the product's underflow is
     * ABSOLUTE, so below ~1.6e-308 the band admits a deviation whose scaled
     * copy is exactly 0.0 (issue #395). An exact test, not a band -- the
     * flatness question is already answered above, and this one is only
     * asking whether the value the division uses exists.
     *
     * The first two tests are relative to the window's own price level (#253).
     * They ask "is this window flat?", and flatness is a property of the
     * prices relative to each other -- but a deviation carries the quote
     * unit, so the fixed TA_IS_ZERO band these used to be answered "flat" for
     * every window of an instrument quoted below it and zeroed the whole
     * output. The band is still wide enough (~90 ulp of the average) to
     * absorb the sub-epsilon residue an identical-price window leaves in the
     * average, which is what it was widened for in the first place (#7).
     */
    tempReal3 = Math.abs(theAverage);
    if (!(Math.abs(tempReal) <= 0.00000000000001 * (tempReal3)) && !(Math.abs(tempReal2) <= 0.00000000000001 * (tempReal3)) && 0.015 * tempReal2 !== 0.0) {
      outReal[outIdx++] = tempReal / (0.015 * tempReal2);
    } else {
      outReal[outIdx++] = 0.0;
    }
    /* Move forward the circular buffer indexes. */
    circBuffer_Idx++;
    if (circBuffer_Idx > maxIdx_circBuffer) {
      circBuffer_Idx = 0;
    }
    i += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  /* Free the circular buffer if it was dynamically allocated. */
  return RetCode.SUCCESS;
}

/**
 * Commodity Channel Index: measures the current typical price relative to its
 * simple moving average, scaled by mean absolute deviation. Momentum oscillator
 * flagging overbought/oversold extremes. CCI > +100 overbought; CCI \< -100
 * oversold.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cci](https://ta-lib.org/functions/cci).
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link cciLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param optInTimePeriod - Number of bars in the averaging/deviation window
 *   (default 14; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - CCI value per bar. Must hold the values produced and never be
 *   empty; the input length always suffices.
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
 * import { cci } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const out = new Float64Array(high.length);
 * const { begIdx, count } = cci(0, high.length - 1, high, low, close, 14, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`typprice`](https://ta-lib.org/functions/typprice)
 * @see [`sma`](https://ta-lib.org/functions/sma)
 */
export function cci(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CCI', 'startIdx', startIdx);
  endIdx = requireInt32('CCI', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('CCI', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('CCI', startIdx, endIdx);
  const guardStart = clampedStart('CCI', startIdx, cciLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CCI', 'inHigh', inHigh, guardInLen);
  requireLength('CCI', 'inLow', inLow, guardInLen);
  requireLength('CCI', 'inClose', inClose, guardInLen);
  requireOutput('CCI', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outReal, inHighF64) || overlapsPartly(outReal, inLowF64) || overlapsPartly(outReal, inCloseF64)) {
    throw failure('CCI', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cciImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CCI', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live CCI stream: one value per closed bar, bit-identical to {@link cci} over
 * the same series. Open it with {@link cciOpen} or {@link cciOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cci](https://ta-lib.org/functions/cci).
 */
export class CciStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  periodTotal: number = 0;
  /** @internal */
  periodPeak: number = 0;
  /** @internal */
  prevValue: number = 0;
  /** @internal */
  sumDirty: number = 0;
  /** @internal */
  flatRun: number = 0;
  /** @internal */
  circBuffer_Idx: number = 0;
  /** @internal */
  maxIdx_circBuffer: number = 0;
  /** @internal */
  cbSize_circBuffer: number = 0;
  /** @internal */
  cb_circBuffer: Float64Array = new Float64Array(1);
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
  static of(core: Core): CciStream {
    return new CciStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cci} reports them over the same bars. Each accepted `update` and each
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
      throw failure('CCI advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number, inClose: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('CCI update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('CCI update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    cciStepImpl(this, inHigh, inLow, inClose);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number, inClose: number): number {
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('CCI peek', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let tempReal = 0;
    let tempReal2 = 0;
    let tempReal3 = 0;
    let theAverage = 0;
    let lastValue = 0;
    let j = 0;
    let cur_outReal = 0;
    let flatRun = sp.flatRun;
    let periodPeak = sp.periodPeak;
    let periodTotal = sp.periodTotal;
    let prevValue = sp.prevValue;
    let sumDirty = sp.sumDirty;
    lastValue = (inHigh + inLow + inClose) / 3;
    if (lastValue === prevValue) {
      flatRun += +(flatRun < sp.optInTimePeriod);
    } else {
      flatRun = 0;
    }
    prevValue = lastValue;
    /* A running sum carries the rounding of every bar it took and gave
     * back, at the scale of the sum when it did. The full pass on the bar
     * that stores into the last slot, and whenever the sum falls below 2/3
     * of the largest it held since the last pass, keeps that rounding at the
     * scale of the current window. An identical-price window must still
     * average to its own value, so it takes the full pass too unless the
     * sum took no change since its last one.
     */
    if (sp.circBuffer_Idx !== sp.optInTimePeriod - 1 && lastValue !== sp.cb_circBuffer[sp.circBuffer_Idx]) {
      periodTotal += lastValue - sp.cb_circBuffer[sp.circBuffer_Idx];
      sumDirty = 1;
    }
    tempReal = Math.abs(periodTotal);
    periodPeak = (tempReal > periodPeak) ? tempReal : periodPeak;
    if (sp.circBuffer_Idx === sp.optInTimePeriod - 1 || periodPeak > 1.5 * tempReal || sumDirty !== 0 && flatRun >= sp.optInTimePeriod - 1) {
      tempReal = 0;
      for (j = 0; j < sp.circBuffer_Idx; j += 1) {
        tempReal += sp.cb_circBuffer[j];
      }
      tempReal += lastValue;
      for (j = sp.circBuffer_Idx + 1; j < sp.optInTimePeriod; j += 1) {
        tempReal += sp.cb_circBuffer[j];
      }
      periodTotal = tempReal;
      periodPeak = Math.abs(tempReal);
      sumDirty = 0;
    }
    theAverage = periodTotal / sp.optInTimePeriod;
    /* Do the summation of the ABS(TypePrice-average)
     * for the whole period, then its mean.
     */
    tempReal2 = 0;
    for (j = 0; j < sp.circBuffer_Idx; j += 1) {
      tempReal2 += Math.abs(sp.cb_circBuffer[j] - theAverage);
    }
    tempReal2 += Math.abs(lastValue - theAverage);
    for (j = sp.circBuffer_Idx + 1; j < sp.optInTimePeriod; j += 1) {
      tempReal2 += Math.abs(sp.cb_circBuffer[j] - theAverage);
    }
    tempReal2 /= sp.optInTimePeriod;
    /* And finally, the CCI... */
    tempReal = lastValue - theAverage;
    /* The third test is the divisor itself, and it is not implied by the
     * second: the deviation's band is RELATIVE and the product's underflow is
     * ABSOLUTE, so below ~1.6e-308 the band admits a deviation whose scaled
     * copy is exactly 0.0 (issue #395). An exact test, not a band -- the
     * flatness question is already answered above, and this one is only
     * asking whether the value the division uses exists.
     *
     * The first two tests are relative to the window's own price level (#253).
     * They ask "is this window flat?", and flatness is a property of the
     * prices relative to each other -- but a deviation carries the quote
     * unit, so the fixed TA_IS_ZERO band these used to be answered "flat" for
     * every window of an instrument quoted below it and zeroed the whole
     * output. The band is still wide enough (~90 ulp of the average) to
     * absorb the sub-epsilon residue an identical-price window leaves in the
     * average, which is what it was widened for in the first place (#7).
     */
    tempReal3 = Math.abs(theAverage);
    if (!(Math.abs(tempReal) <= 0.00000000000001 * (tempReal3)) && !(Math.abs(tempReal2) <= 0.00000000000001 * (tempReal3)) && 0.015 * tempReal2 !== 0.0) {
      cur_outReal = tempReal / (0.015 * tempReal2);
    } else {
      cur_outReal = 0.0;
    }
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
  clone(): CciStream {
    const c = new CciStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.periodTotal = this.periodTotal;
    c.periodPeak = this.periodPeak;
    c.prevValue = this.prevValue;
    c.sumDirty = this.sumDirty;
    c.flatRun = this.flatRun;
    c.circBuffer_Idx = this.circBuffer_Idx;
    c.maxIdx_circBuffer = this.maxIdx_circBuffer;
    c.cbSize_circBuffer = this.cbSize_circBuffer;
    c.cb_circBuffer = this.cb_circBuffer.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cciStepImpl(sp: CciStream, inHigh: number, inLow: number, inClose: number): void {
  let tempReal = 0;
  let tempReal2 = 0;
  let tempReal3 = 0;
  let theAverage = 0;
  let lastValue = 0;
  let j = 0;
  lastValue = (inHigh + inLow + inClose) / 3;
  if (lastValue === sp.prevValue) {
    sp.flatRun += +(sp.flatRun < sp.optInTimePeriod);
  } else {
    sp.flatRun = 0;
  }
  sp.prevValue = lastValue;
  /* A running sum carries the rounding of every bar it took and gave
   * back, at the scale of the sum when it did. The full pass on the bar
   * that stores into the last slot, and whenever the sum falls below 2/3
   * of the largest it held since the last pass, keeps that rounding at the
   * scale of the current window. An identical-price window must still
   * average to its own value, so it takes the full pass too unless the
   * sum took no change since its last one.
   */
  if (sp.circBuffer_Idx !== sp.optInTimePeriod - 1 && lastValue !== sp.cb_circBuffer[sp.circBuffer_Idx]) {
    sp.periodTotal += lastValue - sp.cb_circBuffer[sp.circBuffer_Idx];
    sp.sumDirty = 1;
  }
  sp.cb_circBuffer[sp.circBuffer_Idx] = lastValue;
  tempReal = Math.abs(sp.periodTotal);
  sp.periodPeak = (tempReal > sp.periodPeak) ? tempReal : sp.periodPeak;
  if (sp.circBuffer_Idx === sp.optInTimePeriod - 1 || sp.periodPeak > 1.5 * tempReal || sp.sumDirty !== 0 && sp.flatRun >= sp.optInTimePeriod - 1) {
    tempReal = 0;
    for (j = 0; j < sp.circBuffer_Idx; j += 1) {
      tempReal += sp.cb_circBuffer[j];
    }
    tempReal += lastValue;
    for (j = sp.circBuffer_Idx + 1; j < sp.optInTimePeriod; j += 1) {
      tempReal += sp.cb_circBuffer[j];
    }
    sp.periodTotal = tempReal;
    sp.periodPeak = Math.abs(tempReal);
    sp.sumDirty = 0;
  }
  theAverage = sp.periodTotal / sp.optInTimePeriod;
  /* Do the summation of the ABS(TypePrice-average)
   * for the whole period, then its mean.
   */
  tempReal2 = 0;
  for (j = 0; j < sp.circBuffer_Idx; j += 1) {
    tempReal2 += Math.abs(sp.cb_circBuffer[j] - theAverage);
  }
  tempReal2 += Math.abs(lastValue - theAverage);
  for (j = sp.circBuffer_Idx + 1; j < sp.optInTimePeriod; j += 1) {
    tempReal2 += Math.abs(sp.cb_circBuffer[j] - theAverage);
  }
  tempReal2 /= sp.optInTimePeriod;
  /* And finally, the CCI... */
  tempReal = lastValue - theAverage;
  /* The third test is the divisor itself, and it is not implied by the
   * second: the deviation's band is RELATIVE and the product's underflow is
   * ABSOLUTE, so below ~1.6e-308 the band admits a deviation whose scaled
   * copy is exactly 0.0 (issue #395). An exact test, not a band -- the
   * flatness question is already answered above, and this one is only
   * asking whether the value the division uses exists.
   *
   * The first two tests are relative to the window's own price level (#253).
   * They ask "is this window flat?", and flatness is a property of the
   * prices relative to each other -- but a deviation carries the quote
   * unit, so the fixed TA_IS_ZERO band these used to be answered "flat" for
   * every window of an instrument quoted below it and zeroed the whole
   * output. The band is still wide enough (~90 ulp of the average) to
   * absorb the sub-epsilon residue an identical-price window leaves in the
   * average, which is what it was widened for in the first place (#7).
   */
  tempReal3 = Math.abs(theAverage);
  if (!(Math.abs(tempReal) <= 0.00000000000001 * (tempReal3)) && !(Math.abs(tempReal2) <= 0.00000000000001 * (tempReal3)) && 0.015 * tempReal2 !== 0.0) {
    sp.cur_outReal = tempReal / (0.015 * tempReal2);
  } else {
    sp.cur_outReal = 0.0;
  }
  /* Move forward the circular buffer indexes. */
  sp.circBuffer_Idx = sp.circBuffer_Idx + 1;
  if (sp.circBuffer_Idx > sp.maxIdx_circBuffer) {
    sp.circBuffer_Idx = 0;
  }
}

function cciOpenImpl(
  sp: CciStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let tempReal = 0;
  let tempReal2 = 0;
  let tempReal3 = 0;
  let theAverage = 0;
  let lastValue = 0;
  let periodTotal = 0;
  let periodPeak = 0;
  let prevValue = 0;
  let i = 0;
  let j = 0;
  let k = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let sumDirty = 0;
  let flatRun = 0;
  let circBuffer!: Float64Array;
  let circBuffer_Idx = 0;
  let maxIdx_circBuffer = (30) - 1;
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
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* This ptr will points on a circular buffer of
   * at least "optInTimePeriod" element.
   */
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
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
  /* Allocate a circular buffer equal to the requested
   * period.
   */
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  circBuffer = new Float64Array(optInTimePeriod);
  maxIdx_circBuffer = optInTimePeriod - 1;
  circBuffer_Idx = 0;
  /* Do the MA calculation using tight loops. */
  /* Add-up the initial period, except for the last value.
   * Fill up the circular buffer at the same time.
   */
  i = startIdx - lookbackTotal;
  if (optInTimePeriod > 1) {
    while (i < startIdx) {
      circBuffer[circBuffer_Idx] = (inHigh[i] + inLow[i] + inClose[i]) / 3;
      i += 1;
      circBuffer_Idx++;
      if (circBuffer_Idx > maxIdx_circBuffer) {
        circBuffer_Idx = 0;
      }
    }
  }
  /* The run of bars equal to the one before them, ending on the last bar
   * filled above.
   */
  prevValue = (inHigh[startIdx - 1] + inLow[startIdx - 1] + inClose[startIdx - 1]) / 3;
  flatRun = 0;
  k = startIdx - 1;
  while (k > startIdx - lookbackTotal && (inHigh[k - 1] + inLow[k - 1] + inClose[k - 1]) / 3 === prevValue) {
    flatRun += 1;
    k -= 1;
  }
  /* Proceed with the calculation for the requested range.
   * Note that this algorithm allows the inReal and
   * outReal to be the same buffer.
   */
  outIdx = 0;
  periodTotal = 0.0;
  periodPeak = 0.0;
  sumDirty = 0;
  do {
    lastValue = (inHigh[i] + inLow[i] + inClose[i]) / 3;
    if (lastValue === prevValue) {
      flatRun += +(flatRun < optInTimePeriod);
    } else {
      flatRun = 0;
    }
    prevValue = lastValue;
    /* A running sum carries the rounding of every bar it took and gave
     * back, at the scale of the sum when it did. The full pass on the bar
     * that stores into the last slot, and whenever the sum falls below 2/3
     * of the largest it held since the last pass, keeps that rounding at the
     * scale of the current window. An identical-price window must still
     * average to its own value, so it takes the full pass too unless the
     * sum took no change since its last one.
     */
    if (circBuffer_Idx !== optInTimePeriod - 1 && lastValue !== circBuffer[circBuffer_Idx]) {
      periodTotal += lastValue - circBuffer[circBuffer_Idx];
      sumDirty = 1;
    }
    circBuffer[circBuffer_Idx] = lastValue;
    tempReal = Math.abs(periodTotal);
    periodPeak = (tempReal > periodPeak) ? tempReal : periodPeak;
    if (circBuffer_Idx === optInTimePeriod - 1 || periodPeak > 1.5 * tempReal || sumDirty !== 0 && flatRun >= optInTimePeriod - 1) {
      tempReal = 0;
      for (j = 0; j < circBuffer_Idx; j += 1) {
        tempReal += circBuffer[j];
      }
      tempReal += lastValue;
      for (j = circBuffer_Idx + 1; j < optInTimePeriod; j += 1) {
        tempReal += circBuffer[j];
      }
      periodTotal = tempReal;
      periodPeak = Math.abs(tempReal);
      sumDirty = 0;
    }
    theAverage = periodTotal / optInTimePeriod;
    /* Do the summation of the ABS(TypePrice-average)
     * for the whole period, then its mean.
     */
    tempReal2 = 0;
    for (j = 0; j < circBuffer_Idx; j += 1) {
      tempReal2 += Math.abs(circBuffer[j] - theAverage);
    }
    tempReal2 += Math.abs(lastValue - theAverage);
    for (j = circBuffer_Idx + 1; j < optInTimePeriod; j += 1) {
      tempReal2 += Math.abs(circBuffer[j] - theAverage);
    }
    tempReal2 /= optInTimePeriod;
    /* And finally, the CCI... */
    tempReal = lastValue - theAverage;
    /* The third test is the divisor itself, and it is not implied by the
     * second: the deviation's band is RELATIVE and the product's underflow is
     * ABSOLUTE, so below ~1.6e-308 the band admits a deviation whose scaled
     * copy is exactly 0.0 (issue #395). An exact test, not a band -- the
     * flatness question is already answered above, and this one is only
     * asking whether the value the division uses exists.
     *
     * The first two tests are relative to the window's own price level (#253).
     * They ask "is this window flat?", and flatness is a property of the
     * prices relative to each other -- but a deviation carries the quote
     * unit, so the fixed TA_IS_ZERO band these used to be answered "flat" for
     * every window of an instrument quoted below it and zeroed the whole
     * output. The band is still wide enough (~90 ulp of the average) to
     * absorb the sub-epsilon residue an identical-price window leaves in the
     * average, which is what it was widened for in the first place (#7).
     */
    tempReal3 = Math.abs(theAverage);
    if (!(Math.abs(tempReal) <= 0.00000000000001 * (tempReal3)) && !(Math.abs(tempReal2) <= 0.00000000000001 * (tempReal3)) && 0.015 * tempReal2 !== 0.0) {
      outReal[((outIdx++ * outStride) | 0)] = tempReal / (0.015 * tempReal2);
    } else {
      outReal[((outIdx++ * outStride) | 0)] = 0.0;
    }
    /* Move forward the circular buffer indexes. */
    circBuffer_Idx++;
    if (circBuffer_Idx > maxIdx_circBuffer) {
      circBuffer_Idx = 0;
    }
    i += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  /* Free the circular buffer if it was dynamically allocated. */
  const capCb_circBuffer = maxIdx_circBuffer + 1;
  if (capCb_circBuffer > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.periodTotal = periodTotal;
  sp.periodPeak = periodPeak;
  sp.prevValue = prevValue;
  sp.sumDirty = sumDirty;
  sp.flatRun = flatRun;
  sp.circBuffer_Idx = circBuffer_Idx;
  sp.maxIdx_circBuffer = maxIdx_circBuffer;
  sp.cbSize_circBuffer = capCb_circBuffer;
  sp.cb_circBuffer = circBuffer;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cciOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): CciStream {
  const sp = CciStream.of(core);
  const retCode = cciOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CCI openAndFill', inHigh.length, startIdx, cciLookback(optInTimePeriod, core));
  }
  throw streamFailure('CCI openAndFill', retCode);
}

/** @internal */
export function cciOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): CciStream {
  const sp = CciStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = cciOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CCI open', inHigh.length, startIdx, cciLookback(optInTimePeriod, core));
  }
  throw streamFailure('CCI open', retCode);
}

/**
 * Opens a live CCI stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link cci} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Number of bars in the averaging/deviation window
 *   (default 14; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cciLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { cciOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cciOpen(high, low, close, 14);
 * const peeked = stream.peek(101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cciOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): CciStream {
  optInTimePeriod = requireInt32('CCI open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('CCI open', 'inHigh', inHigh);
  requireHistory('CCI open', inHigh.length);
  requireArgument('CCI open', 'inLow', inLow);
  requireArgument('CCI open', 'inClose', inClose);
  requireHistoryLength('CCI open', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('CCI open', 'inClose', inClose.length, inHigh.length);
  return cciOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInTimePeriod);
}

/**
 * {@link cciOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link cci}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Number of bars in the averaging/deviation window
 *   (default 14; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - CCI value per bar. Must hold `historyLen - lookback` values and
 *   share no memory with an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cciLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range, histories of different lengths or an output too
 *   short or starting where another output or a `Float64Array` input starts, or, on
 *   a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cciOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const out = new Float64Array(high.length);
 * const stream = cciOpenAndFill(high, low, close, 14, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function cciOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): CciStream {
  optInTimePeriod = requireInt32('CCI openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('CCI openAndFill', 'inHigh', inHigh);
  requireHistory('CCI openAndFill', inHigh.length);
  requireArgument('CCI openAndFill', 'inLow', inLow);
  requireArgument('CCI openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('CCI openAndFill', inHigh.length, cciLookback(optInTimePeriod, core));
  requireHistoryLength('CCI openAndFill', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('CCI openAndFill', 'inClose', inClose.length, inHigh.length);
  requireOutput('CCI openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) || (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) || (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset)) {
    throw streamFailure('CCI openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inHighF64) || overlaps(outReal, inLowF64) || overlaps(outReal, inCloseF64))) {
    throw streamFailure('CCI openAndFill', RetCode.BAD_PARAM);
  }
  return cciOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}
