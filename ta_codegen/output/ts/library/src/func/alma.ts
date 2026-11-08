/* Arnaud Legoux Moving Average, created by Arnaud Legoux and Dimitris
 * Kouzis-Loukas: "ALMA; In search for the perfect Moving Average", 2009,
 * https://web.archive.org/web/20110904091012/www.arnaudlegoux.com/wp-content/uploads/2011/03/ALMA-Arnaud-Legoux-Moving-Average.pdf
 *
 * List of contributors:
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
 *  092926 MF,CC  First version (issue #475).
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireReal, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/* Using alma_ALT1 for TA_ALT={BATCH,ALL_LANGUAGES} */

/** @internal */
export function almaLookbackPositional(optInTimePeriod: number, optInSigma: number, optInOffset: number, _core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (typeof optInSigma !== 'number') {
    return -1;
  }
  if (typeof optInOffset !== 'number') {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 9;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  if (optInSigma === Core.REAL_DEFAULT) {
    optInSigma = 6.0;
  } else if (!(optInSigma >= 1e-2 && optInSigma <= Core.REAL_MAX)) {
    return -1;
  }
  if (optInOffset === Core.REAL_DEFAULT) {
    optInOffset = 0.85;
  } else if (!(optInOffset >= 0e0 && optInOffset <= 1e0)) {
    return -1;
  }
  return optInTimePeriod - 1;
}

/** @internal */
export function almaDisplayShiftPositional(optInTimePeriod: number, optInSigma: number, optInOffset: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (almaLookbackPositional(optInTimePeriod, optInSigma, optInOffset, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function almaImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  optInSigma: number,
  optInOffset: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let sum = 0;
  let s = 0;
  let twoSSq = 0;
  let d = 0;
  let norm = 0;
  let wj = 0;
  let s0 = 0;
  let s1 = 0;
  let s2 = 0;
  let s3 = 0;
  let lookbackTotal = 0;
  let outIdx = 0;
  let i = 0;
  let j = 0;
  let w = 0;
  let m = 0;
  let b = 0;
  let weights!: Float64Array;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 9;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSigma === Core.REAL_DEFAULT) {
    optInSigma = 6.0;
  } else if (!(optInSigma >= 1e-2 && optInSigma <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInOffset === Core.REAL_DEFAULT) {
    optInOffset = 0.85;
  } else if (!(optInOffset >= 0e0 && optInOffset <= 1e0)) {
    return RetCode.BAD_PARAM;
  }
  /* Filled once and read as a plain array. */
  lookbackTotal = optInTimePeriod - 1;
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  /* A dot product seeded with 0.0 would turn a -0.0 input into +0.0. */
  if (optInTimePeriod === 1) {
    outBegIdx.value = startIdx;
    outNBElement.value = endIdx - startIdx + 1;
    i = startIdx;
    for (outIdx = 0; outIdx < outNBElement.value; outIdx += 1) {
      outReal[outIdx] = inReal[i++];
    }
    return RetCode.SUCCESS;
  }
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  weights = new Float64Array(optInTimePeriod);
  /* The expression order is the spec: it is what makes the output bitwise
   * equal to Tulip's beta/alma.c. Keep the floor in binary64 and (j-m) squared
   * in double (an int square overflows at large periods).
   */
  m = (Math.floor(optInOffset * lookbackTotal) | 0);
  s = optInTimePeriod / optInSigma;
  twoSSq = 2.0 * s * s;
  norm = 0.0;
  for (j = 0; j < optInTimePeriod; j += 1) {
    d = (j - m);
    weights[j] = Math.exp(-(d * d) / twoSSq);
    norm += weights[j];
  }
  for (j = 0; j < optInTimePeriod; j += 1) {
    weights[j] = weights[j] / norm;
  }
  /* 4 bars per pass, each with its own accumulator summed in the same
   * order as the base: the same bits, with 4 independent add chains in
   * flight instead of one. Every window of a pass is read before any of its
   * outputs is written, and the next pass reads past them, so outReal may
   * alias inReal.
   */
  outIdx = 0;
  i = startIdx;
  while (i + 3 <= endIdx) {
    s0 = 0.0;
    s1 = 0.0;
    s2 = 0.0;
    s3 = 0.0;
    b = i - lookbackTotal;
    for (j = 0; j < optInTimePeriod; j += 1) {
      wj = weights[j];
      s0 += wj * inReal[b];
      s1 += wj * inReal[b + 1];
      s2 += wj * inReal[b + 2];
      s3 += wj * inReal[b + 3];
      b += 1;
    }
    outReal[outIdx] = s0;
    outReal[outIdx + 1] = s1;
    outReal[outIdx + 2] = s2;
    outReal[outIdx + 3] = s3;
    outIdx += 4;
    i += 4;
  }
  while (i <= endIdx) {
    sum = 0.0;
    w = 0;
    for (j = i - lookbackTotal; j <= i; j += 1) {
      sum += weights[w] * inReal[j];
      w += 1;
    }
    outReal[outIdx] = sum;
    outIdx += 1;
    i += 1;
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['sigma', Core.REAL_DEFAULT, 1e-2, Core.REAL_MAX, 'in 0.01..3e37'], ['offset', Core.REAL_DEFAULT, 0e0, 1e0, 'in 0..1']] as const;

/** @internal */
export function almaPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInSigma: number,
  optInOffset: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('ALMA', 'startIdx', startIdx);
  endIdx = requireInt32('ALMA', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('ALMA', 'timePeriod', optInTimePeriod);
  optInSigma = requireReal('ALMA', 'sigma', optInSigma);
  optInOffset = requireReal('ALMA', 'offset', optInOffset);
  requireIndexRange('ALMA', startIdx, endIdx);
  const guardLookback = almaLookbackPositional(optInTimePeriod, optInSigma, optInOffset, core);
  if (guardLookback < 0) {
    throw rejectedParams('ALMA', PARAM_RANGES, optInTimePeriod, optInSigma, optInOffset);
  }
  const guardStart = clampedStart('ALMA', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('ALMA', 'real', inReal, guardInLen);
  requireOutput('ALMA', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('ALMA', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = almaImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, optInSigma, optInOffset, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('ALMA', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link alma}, {@link almaLookback}, {@link almaDisplayShift},
 * {@link almaOpen} and {@link almaOpenAndFill}. Every key is optional: an omitted
 * or `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface AlmaOptions {
  /**
   * Number of bars in the window (default 9; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  timePeriod?: number | undefined;
  /**
   * Divides the period to give the Gaussian's width in bars (default 6; minimum
   * 0.01; `Core.REAL_DEFAULT` selects the default).
   */
  sigma?: number | undefined;
  /**
   * Position of the peak weight, 0 at the oldest bar and 1 at the newest (default
   * 0.85; range 0 to 1; `Core.REAL_DEFAULT` selects the default).
   */
  offset?: number | undefined;
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

/** What {@link alma} returns, and {@link almaOpenAndFill} with its stream. */
export interface AlmaResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Arnaud Legoux Moving Average line. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link alma} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AlmaOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function almaLookback(options?: AlmaOptions): number {
  const o = optionsOf('ALMA lookback', options);
  return almaLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.sigma, Core.REAL_DEFAULT), orDefault(o.offset, Core.REAL_DEFAULT), coreOf('ALMA lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link alma}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AlmaOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function almaDisplayShift(outputIdx: number, options?: AlmaOptions): number {
  const o = optionsOf('ALMA displayShift', options);
  return almaDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.sigma, Core.REAL_DEFAULT), orDefault(o.offset, Core.REAL_DEFAULT), outputIdx, coreOf('ALMA displayShift', o.core));
}

/**
 * Arnaud Legoux Moving Average: the last N inputs weighted by a Gaussian whose
 * peak sits a fraction `offset` of the way from the oldest to the newest bar.
 * Moving the offset toward 1 puts the peak on recent bars and cuts the lag; moving
 * it toward 0 puts the peak on old bars and lags most. Smoothing is greatest with
 * the peak mid-window, near 0.5. Sigma sets the width: the Gaussian's standard
 * deviation is `N / sigma` bars, so a larger sigma concentrates the weight around
 * the peak and smooths less. The weights are non-negative and sum to one, so the
 * line stays within the range of its window, up to rounding. At the default shape
 * and a period of 5 or more, it lags about half as much as an `SMA` of the same
 * period and passes about twice its noise.
 *
 * Formula and more info at
 * [ta-lib.org/functions/alma](https://ta-lib.org/functions/alma).
 *
 * **Notes**
 * - The peak is floored to a whole bar, as in the authors' code. At periods below
 *   about 5 this puts it on an older bar: at period 2 the line is almost entirely
 *   the previous bar.
 * - `MAType.ALMA` runs this function at the default sigma and offset, so the line
 *   in `MA` and every function taking an MAType has the lag and noise stated
 *   above.
 * - The published paper's summary formula uses a different parameterisation; this
 *   is the form of the authors' own implementation.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link almaLookback} is a success with empty outputs.
 *
 * @param real - Data on which to compute the average.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link AlmaOptions}.
 * @returns `begIdx` and the values; see {@link AlmaResult}. Each output is a view
 *   of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, an integer parameter that is not an int32, a real parameter that is not a
 *   number, a parameter outside its range, an input too short for the range, a
 *   buffer that is empty, too short or of the wrong type or a buffer sharing memory
 *   with a `Float64Array` input it is not. Checked before anything is written.
 *
 * @example
 * ```ts
 * import { alma } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = alma(data, {
 *   timePeriod: 9,
 *   sigma: 6,
 *   offset: 0.85,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`wma`](https://ta-lib.org/functions/wma)
 * @see [`sma`](https://ta-lib.org/functions/sma)
 * @see [`trima`](https://ta-lib.org/functions/trima)
 * @see [`hma`](https://ta-lib.org/functions/hma)
 */
export function alma(real: ArrayLike<number>, options?: AlmaOptions): AlmaResult {
  const o = optionsOf('ALMA', options);
  if (o.endIdx === undefined) {
    requireSeries('ALMA', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInSigma = orDefault(o.sigma, Core.REAL_DEFAULT);
  const optInOffset = orDefault(o.offset, Core.REAL_DEFAULT);
  const core = coreOf('ALMA', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, almaLookbackPositional(optInTimePeriod, optInSigma, optInOffset, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = almaPositional(startIdx, endIdx, real, optInTimePeriod, optInSigma, optInOffset, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live ALMA stream: one value per closed bar, bit-identical to {@link alma} over
 * the same series. Open it with {@link almaOpen} or {@link almaOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/alma](https://ta-lib.org/functions/alma).
 */
export class AlmaStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  optInSigma: number = 0;
  /** @internal */
  optInOffset: number = 0;
  /** @internal */
  lookbackTotal: number = 0;
  /** @internal */
  weights_Idx: number = 0;
  /** @internal */
  maxIdx_weights: number = 0;
  /** @internal */
  winPos_j: number = 0;
  /** @internal */
  winCap_j: number = 1;
  /** @internal */
  win_j_inReal: Float64Array = new Float64Array(1);
  /** @internal */
  cbSize_weights: number = 0;
  /** @internal */
  cb_weights: Float64Array = new Float64Array(1);
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
  static of(core: Core): AlmaStream {
    return new AlmaStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link alma} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('ALMA advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: Data on which to compute the average.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('ALMA update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('ALMA update', 'real');
    }
    almaStepImpl(this, real);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: Data on which to compute the average.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number): number {
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('ALMA peek', 'real');
    }
    const sp = this;
    const inReal = real;
    let sum = 0;
    let j = 0;
    let w = 0;
    let cur_outReal = 0;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    if (sp.optInTimePeriod === 1) {
      cur_outReal = inReal;
      return cur_outReal;
    }
    pkSlot0 = sp.winPos_j;
    pkVal0 = inReal;
    sum = 0.0;
    w = 0;
    for (j = sp.lookbackTotal; j >= 0; j -= 1) {
      sum += sp.cb_weights[w] * ((((sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j) !== pkSlot0) ? sp.win_j_inReal[(sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j] : pkVal0);
      w += 1;
    }
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
  clone(): AlmaStream {
    const c = new AlmaStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.optInSigma = this.optInSigma;
    c.optInOffset = this.optInOffset;
    c.lookbackTotal = this.lookbackTotal;
    c.weights_Idx = this.weights_Idx;
    c.maxIdx_weights = this.maxIdx_weights;
    c.winPos_j = this.winPos_j;
    c.winCap_j = this.winCap_j;
    c.win_j_inReal = this.win_j_inReal.slice();
    c.cbSize_weights = this.cbSize_weights;
    c.cb_weights = this.cb_weights.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function almaStepImpl(sp: AlmaStream, inReal: number): void {
  let sum = 0;
  let j = 0;
  let w = 0;
  if (sp.optInTimePeriod === 1) {
    sp.cur_outReal = inReal;
    return;
  }
  sp.win_j_inReal[sp.winPos_j] = inReal;
  sum = 0.0;
  w = 0;
  for (j = sp.lookbackTotal; j >= 0; j -= 1) {
    sum += sp.cb_weights[w] * sp.win_j_inReal[(sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j];
    w += 1;
  }
  sp.cur_outReal = sum;
  sp.winPos_j = sp.winPos_j + 1;
  if (sp.winPos_j >= sp.winCap_j) {
    sp.winPos_j = 0;
  }
}

function almaOpenImpl(
  sp: AlmaStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInSigma: number,
  optInOffset: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let sum = 0;
  let s = 0;
  let twoSSq = 0;
  let d = 0;
  let norm = 0;
  let lookbackTotal = 0;
  let outIdx = 0;
  let i = 0;
  let j = 0;
  let w = 0;
  let m = 0;
  let weights!: Float64Array;
  let weights_Idx = 0;
  let maxIdx_weights = (30) - 1;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 9;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSigma === Core.REAL_DEFAULT) {
    optInSigma = 6.0;
  } else if (!(optInSigma >= 1e-2 && optInSigma <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInOffset === Core.REAL_DEFAULT) {
    optInOffset = 0.85;
  } else if (!(optInOffset >= 0e0 && optInOffset <= 1e0)) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (optInTimePeriod === 1) {
    let fillLb = almaLookbackPositional(optInTimePeriod, optInSigma, optInOffset, core);
    if (startIdx > fillLb) {
      fillLb = startIdx;
    }
    if (historyLen < fillLb + 1) {
      return RetCode.INSUFFICIENT_HISTORY;
    }
    sp.optInTimePeriod = optInTimePeriod;
    sp.optInSigma = optInSigma;
    sp.optInOffset = optInOffset;
    sp.lookbackTotal = 0;
    sp.weights_Idx = 0;
    sp.maxIdx_weights = 0;
    sp.winPos_j = 0;
    sp.winCap_j = 1;
    sp.win_j_inReal = new Float64Array(1);
    sp.cbSize_weights = 0;
    sp.cb_weights = new Float64Array(1);
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
  /* Filled once and read as a plain array. */
  lookbackTotal = optInTimePeriod - 1;
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  weights = new Float64Array(optInTimePeriod);
  maxIdx_weights = optInTimePeriod - 1;
  weights_Idx = 0;
  /* The expression order is the spec: it is what makes the output bitwise
   * equal to Tulip's beta/alma.c. Keep the floor in binary64 and (j-m) squared
   * in double (an int square overflows at large periods).
   */
  m = (Math.floor(optInOffset * lookbackTotal) | 0);
  s = optInTimePeriod / optInSigma;
  twoSSq = 2.0 * s * s;
  norm = 0.0;
  for (j = 0; j < optInTimePeriod; j += 1) {
    d = (j - m);
    weights[j] = Math.exp(-(d * d) / twoSSq);
    norm += weights[j];
  }
  for (j = 0; j < optInTimePeriod; j += 1) {
    weights[j] = weights[j] / norm;
  }
  /* Weight 0 is the oldest bar. Each output is written after its window is
   * read, and the next window starts past it, so outReal may alias inReal.
   */
  outIdx = 0;
  i = startIdx;
  while (i <= endIdx) {
    sum = 0.0;
    w = 0;
    for (j = i - lookbackTotal; j <= i; j += 1) {
      sum += weights[w] * inReal[j];
      w += 1;
    }
    outReal[((outIdx * outStride) | 0)] = sum;
    outIdx += 1;
    i += 1;
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const cap_j = lookbackTotal + 1;
  if (cap_j < 1 || cap_j > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const capWin_j_inReal = inReal.slice(historyLen - cap_j, historyLen);
  const capCb_weights = maxIdx_weights + 1;
  if (capCb_weights > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.optInSigma = optInSigma;
  sp.optInOffset = optInOffset;
  sp.lookbackTotal = lookbackTotal;
  sp.weights_Idx = weights_Idx;
  sp.maxIdx_weights = maxIdx_weights;
  sp.winPos_j = 0;
  sp.winCap_j = cap_j;
  sp.win_j_inReal = capWin_j_inReal;
  sp.cbSize_weights = capCb_weights;
  sp.cb_weights = weights;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function almaOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInSigma: number,
  optInOffset: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): AlmaStream {
  const sp = AlmaStream.of(core);
  const retCode = almaOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInSigma, optInOffset, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ALMA openAndFill', inReal.length, startIdx, almaLookbackPositional(optInTimePeriod, optInSigma, optInOffset, core));
  }
  if (retCode === RetCode.BAD_PARAM && almaLookbackPositional(optInTimePeriod, optInSigma, optInOffset, core) < 0) {
    throw rejectedParams('ALMA openAndFill', PARAM_RANGES, optInTimePeriod, optInSigma, optInOffset);
  }
  throw streamFailure('ALMA openAndFill', retCode);
}

/** @internal */
export function almaOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInSigma: number,
  optInOffset: number,
): AlmaStream {
  const sp = AlmaStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = almaOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInSigma, optInOffset, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ALMA open', inReal.length, startIdx, almaLookbackPositional(optInTimePeriod, optInSigma, optInOffset, core));
  }
  if (retCode === RetCode.BAD_PARAM && almaLookbackPositional(optInTimePeriod, optInSigma, optInOffset, core) < 0) {
    throw rejectedParams('ALMA open', PARAM_RANGES, optInTimePeriod, optInSigma, optInOffset);
  }
  throw streamFailure('ALMA open', retCode);
}

/** @internal */
export function almaOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInSigma: number,
  optInOffset: number,
  core: Core = Core.DEFAULT,
): AlmaStream {
  optInTimePeriod = requireInt32('ALMA open', 'timePeriod', optInTimePeriod);
  optInSigma = requireReal('ALMA open', 'sigma', optInSigma);
  optInOffset = requireReal('ALMA open', 'offset', optInOffset);
  requireArgument('ALMA open', 'real', inReal);
  requireHistory('ALMA open', 'real', inReal.length);
  return almaOpenInternal(core, float64(inReal), 0, optInTimePeriod, optInSigma, optInOffset);
}

/** @internal */
export function almaOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInSigma: number,
  optInOffset: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): AlmaStream {
  optInTimePeriod = requireInt32('ALMA openAndFill', 'timePeriod', optInTimePeriod);
  optInSigma = requireReal('ALMA openAndFill', 'sigma', optInSigma);
  optInOffset = requireReal('ALMA openAndFill', 'offset', optInOffset);
  requireArgument('ALMA openAndFill', 'real', inReal);
  requireHistory('ALMA openAndFill', 'real', inReal.length);
  const guardLookback = almaLookbackPositional(optInTimePeriod, optInSigma, optInOffset, core);
  if (guardLookback < 0) {
    throw rejectedParams('ALMA openAndFill', PARAM_RANGES, optInTimePeriod, optInSigma, optInOffset);
  }
  const guardOutLen = openFillCount('ALMA openAndFill', inReal.length, guardLookback);
  requireOutput('ALMA openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('ALMA openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('ALMA openAndFill', 'outReal', 'real');
  }
  return almaOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, optInSigma, optInOffset, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live ALMA stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link alma} there.
 *
 * @param real - Warm-up history. Data on which to compute the average.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AlmaOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `almaLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a real parameter that is not a number
 *   or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { almaOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = almaOpen(data, { timePeriod: 9, sigma: 6, offset: 0.85 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function almaOpen(real: ArrayLike<number>, options?: AlmaOptions): AlmaStream {
  const o = optionsOf('ALMA open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInSigma = orDefault(o.sigma, Core.REAL_DEFAULT);
  const optInOffset = orDefault(o.offset, Core.REAL_DEFAULT);
  const core = coreOf('ALMA open', o.core);
  return almaOpenPositional(real, optInTimePeriod, optInSigma, optInOffset, core);
}

/**
 * {@link almaOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link alma}.
 *
 * @param real - Warm-up history. Data on which to compute the average.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link AlmaOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link alma} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `almaLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a real parameter that is not a
 *   number, a parameter outside its range or a buffer too short, of the wrong type
 *   or starting where another buffer or a `Float64Array` input starts, or, on a
 *   history long enough to open, one sharing memory with another buffer or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { almaOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = almaOpenAndFill(data, {
 *   timePeriod: 9,
 *   sigma: 6,
 *   offset: 0.85,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function almaOpenAndFill(real: ArrayLike<number>, options?: AlmaOptions): AlmaResult & { stream: AlmaStream } {
  const o = optionsOf('ALMA openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInSigma = orDefault(o.sigma, Core.REAL_DEFAULT);
  const optInOffset = orDefault(o.offset, Core.REAL_DEFAULT);
  const core = coreOf('ALMA openAndFill', o.core);
  const capacity = fillCapacity(almaLookbackPositional(optInTimePeriod, optInSigma, optInOffset, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = almaOpenAndFillPositional(real, optInTimePeriod, optInSigma, optInOffset, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/** @internal */
export function almaStepTape(sp: AlmaStream, tape: Float64Array, tapeBase: number, tapeMask: number, _inReal: number): number {
  let sum = 0;
  let j = 0;
  let w = 0;
  sum = 0.0;
  w = 0;
  for (j = sp.lookbackTotal; j >= 0; j -= 1) {
    sum += sp.cb_weights[w] * tape[(tapeBase - j) & tapeMask];
    w += 1;
  }
  sp.cur_outReal = sum;
  sp.outRangeCount++;
  return sp.cur_outReal;
}

/** @internal */
export function almaPeekTape(sp: AlmaStream, tape: Float64Array, tapeBase: number, tapeMask: number, inReal: number): number {
  let sum = 0;
  let j = 0;
  let w = 0;
  let cur_outReal = 0;
  let pkSlot0 = -1;
  let pkVal0 = 0;
  pkSlot0 = tapeBase & tapeMask;
  pkVal0 = inReal;
  sum = 0.0;
  w = 0;
  for (j = sp.lookbackTotal; j >= 0; j -= 1) {
    sum += sp.cb_weights[w] * ((((tapeBase - j) & tapeMask) !== pkSlot0) ? tape[(tapeBase - j) & tapeMask] : pkVal0);
    w += 1;
  }
  cur_outReal = sum;
  return cur_outReal;
}

/** @internal */
export function almaTapeDetach(sp: AlmaStream): number {
  let reach = 0;
  sp.win_j_inReal = new Float64Array(0);
  if (sp.winCap_j - 1 > reach) {
    reach = sp.winCap_j - 1;
  }
  return reach;
}
