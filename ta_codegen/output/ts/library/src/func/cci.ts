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

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function cciLookbackPositional(optInTimePeriod: number, _core: Core = Core.DEFAULT): number {
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
export function cciDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (cciLookbackPositional(optInTimePeriod, core) < 0) {
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

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000']] as const;

/** @internal */
export function cciPositional(
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
  optInTimePeriod = requireInt32('CCI', 'timePeriod', optInTimePeriod);
  requireIndexRange('CCI', startIdx, endIdx);
  const guardLookback = cciLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('CCI', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('CCI', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CCI', 'prices.high', inHigh, guardInLen);
  requireLength('CCI', 'prices.low', inLow, guardInLen);
  requireLength('CCI', 'prices.close', inClose, guardInLen);
  requireOutput('CCI', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outReal, inHighF64)) {
    throw sharedMemory('CCI', 'outReal', 'prices.high');
  }
  if (overlapsPartly(outReal, inLowF64)) {
    throw sharedMemory('CCI', 'outReal', 'prices.low');
  }
  if (overlapsPartly(outReal, inCloseF64)) {
    throw sharedMemory('CCI', 'outReal', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cciImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CCI', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cci}, {@link cciLookback}, {@link cciDisplayShift},
 * {@link cciOpen} and {@link cciOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface CciOptions {
  /**
   * Number of bars in the averaging/deviation window (default 14; range 2 to 100000;
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

/** What {@link cci} returns, and {@link cciOpenAndFill} with its stream. */
export interface CciResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /** CCI value per bar. `values[k]` is the value at input bar `begIdx + k`. */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link cci} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CciOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cciLookback(options?: CciOptions): number {
  const o = optionsOf('CCI lookback', options);
  return cciLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('CCI lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cci}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CciOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cciDisplayShift(outputIdx: number, options?: CciOptions): number {
  const o = optionsOf('CCI displayShift', options);
  return cciDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('CCI displayShift', o.core));
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
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cciLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `high`: High price of
 *   each bar. `low`: Low price of each bar. `close`: Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link CciOptions}.
 * @returns `begIdx` and the values; see {@link CciResult}. Each output is a view
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
 * import { cci } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { begIdx, values } = cci({ high, low, close }, { timePeriod: 14 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`typprice`](https://ta-lib.org/functions/typprice)
 * @see [`sma`](https://ta-lib.org/functions/sma)
 */
export function cci(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CciOptions): CciResult {
  requireArgument('CCI', 'prices', prices);
  const o = optionsOf('CCI', options);
  if (o.endIdx === undefined) {
    requireSeries('CCI', 'prices.high', prices.high);
    requireSeries('CCI', 'prices.low', prices.low);
    requireSeries('CCI', 'prices.close', prices.close);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('CCI', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cciLookbackPositional(optInTimePeriod, core), prices.high, prices.low, prices.close);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = cciPositional(startIdx, endIdx, prices.high, prices.low, prices.close, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
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
      throw streamFailure('CCI advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(high: number, low: number, close: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('CCI update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('CCI update', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    cciStepImpl(this, high, low, close);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(high: number, low: number, close: number): number {
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('CCI peek', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
    const inClose = close;
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
  if (startIdx >= historyLen) {
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
    throw insufficientHistory('CCI openAndFill', inHigh.length, startIdx, cciLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && cciLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('CCI openAndFill', PARAM_RANGES, optInTimePeriod);
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
    throw insufficientHistory('CCI open', inHigh.length, startIdx, cciLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && cciLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('CCI open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('CCI open', retCode);
}

/** @internal */
export function cciOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): CciStream {
  optInTimePeriod = requireInt32('CCI open', 'timePeriod', optInTimePeriod);
  requireArgument('CCI open', 'prices.high', inHigh);
  requireHistory('CCI open', 'prices.high', inHigh.length);
  requireArgument('CCI open', 'prices.low', inLow);
  requireArgument('CCI open', 'prices.close', inClose);
  requireHistoryLength('CCI open', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('CCI open', 'prices.close', inClose.length, inHigh.length);
  return cciOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInTimePeriod);
}

/** @internal */
export function cciOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): CciStream {
  optInTimePeriod = requireInt32('CCI openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('CCI openAndFill', 'prices.high', inHigh);
  requireHistory('CCI openAndFill', 'prices.high', inHigh.length);
  requireArgument('CCI openAndFill', 'prices.low', inLow);
  requireArgument('CCI openAndFill', 'prices.close', inClose);
  const guardLookback = cciLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('CCI openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('CCI openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('CCI openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('CCI openAndFill', 'prices.close', inClose.length, inHigh.length);
  requireOutput('CCI openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CCI openAndFill', 'outReal', 'prices.high');
  }
  if (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CCI openAndFill', 'outReal', 'prices.low');
  }
  if (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('CCI openAndFill', 'outReal', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outReal, inHighF64)) {
    throw sharedMemory('CCI openAndFill', 'outReal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outReal, inLowF64)) {
    throw sharedMemory('CCI openAndFill', 'outReal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outReal, inCloseF64)) {
    throw sharedMemory('CCI openAndFill', 'outReal', 'prices.close');
  }
  return cciOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live CCI stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link cci} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar. `close`: Close
 *   price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CciOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cciLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or
 *   histories of different lengths.
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
 * const stream = cciOpen({ high, low, close }, { timePeriod: 14 });
 * const peeked = stream.peek(101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cciOpen(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CciOptions): CciStream {
  requireArgument('CCI open', 'prices', prices);
  const o = optionsOf('CCI open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('CCI open', o.core);
  return cciOpenPositional(prices.high, prices.low, prices.close, optInTimePeriod, core);
}

/**
 * {@link cciOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link cci}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price of each bar. `low`: Low price of each bar. `close`: Close
 *   price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CciOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cci} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cciLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
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
 * const { stream, begIdx, values } = cciOpenAndFill({ high, low, close }, {
 *   timePeriod: 14,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cciOpenAndFill(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CciOptions): CciResult & { stream: CciStream } {
  requireArgument('CCI openAndFill', 'prices', prices);
  const o = optionsOf('CCI openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('CCI openAndFill', o.core);
  const capacity = fillCapacity(cciLookbackPositional(optInTimePeriod, core), prices.high, prices.low, prices.close);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = cciOpenAndFillPositional(prices.high, prices.low, prices.close, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
