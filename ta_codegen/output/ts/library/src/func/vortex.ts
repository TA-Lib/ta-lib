/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090626 KL     First version (issue #349).
 *  102326 MF,CC  #554 branch-free zero run, rare reset.
 *  102426 MF,CC  #554 zero-run test on the true range itself.
 */

import { badSink, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function vortexLookbackPositional(optInTimePeriod: number, _core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  /* Every per-bar term (TR, |H-prevL|, |L-prevH|) needs the prior bar, so
   * bar 0 is consumed exactly as TA_TRANGE consumes it, and the window then
   * needs optInTimePeriod terms: 1 + (optInTimePeriod - 1) = optInTimePeriod.
   * First valid output index is n, not n-1.
   */
  return optInTimePeriod;
}

/** @internal */
export function vortexDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (vortexLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 2) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function vortexImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outPlusVI: Float64Array,
  outMinusVI: Float64Array,
): RetCode {
  let outIdx = 0;
  let today = 0;
  let trailingIdx = 0;
  let lookbackTotal = 0;
  let i = 0;
  let nullRun = 0;
  let live = 0;
  let sTR = 0;
  let sVMP = 0;
  let sVMM = 0;
  let curTR = 0;
  let curVMP = 0;
  let curVMM = 0;
  let trueRange = 0;
  let tempDouble = 0;
  let tempLT = 0;
  let tempHT = 0;
  let tempCY = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if ((outPlusVI.buffer === outMinusVI.buffer && outPlusVI.byteOffset === outMinusVI.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  /* Vortex Indicator (Botes & Siepman, TASC 28:1, Jan 2010): two lines,
   * each a rolling sum of "vortex movement" normalized by the rolling sum
   * of true range over the same optInTimePeriod bars.
   *
   *   TR[i]  = max( H[i]-L[i], |C[i-1]-H[i]|, |C[i-1]-L[i]| )   == TA_TRANGE
   *   VMP[i] = |H[i] - L[i-1]|
   *   VMM[i] = |L[i] - H[i-1]|
   *   +VI = SUM(VMP, n) / SUM(TR, n),  -VI = SUM(VMM, n) / SUM(TR, n)
   *
   * No smoothing, no recursion, nothing to seed. The TR expansion below is
   * TA_TRANGE's own operation order, bit for bit -- the differential test
   * composes TA_TRANGE + TA_SUM and asserts equality with memcmp.
   *
   * The trailing terms are recomputed from the inputs rather than carried
   * in a ring; the subtraction re-reads bars trailingIdx and trailingIdx-1,
   * both of which sit at or ahead of the output slot, which is why the
   * outputs are written LAST (see the loop comment).
   */
  lookbackTotal = vortexLookbackPositional(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  /* Prime the three window sums over the optInTimePeriod-1 terms before the
   * first output bar: [startIdx-optInTimePeriod+1, startIdx). Each term at
   * bar i reads bar i-1, so the earliest read is bar startIdx-optInTimePeriod
   * >= 0.
   */
  sTR = 0.0;
  sVMP = 0.0;
  sVMM = 0.0;
  nullRun = 0;
  for (i = startIdx - optInTimePeriod + 1; i < startIdx; i += 1) {
    tempLT = inLow[i];
    tempHT = inHigh[i];
    tempCY = inClose[i - 1];
    trueRange = tempHT - tempLT;
    tempDouble = Math.abs(tempCY - tempHT);
    if (tempDouble > trueRange) {
      trueRange = tempDouble;
    }
    tempDouble = Math.abs(tempCY - tempLT);
    if (tempDouble > trueRange) {
      trueRange = tempDouble;
    }
    sTR += trueRange;
    sVMP += Math.abs(inHigh[i] - inLow[i - 1]);
    sVMM += Math.abs(inLow[i] - inHigh[i - 1]);
    live = (!(trueRange <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
  }
  outIdx = 0;
  today = startIdx;
  trailingIdx = startIdx - optInTimePeriod + 1;
  while (today <= endIdx) {
    /* Add on today's terms. */
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    trueRange = tempHT - tempLT;
    tempDouble = Math.abs(tempCY - tempHT);
    if (tempDouble > trueRange) {
      trueRange = tempDouble;
    }
    tempDouble = Math.abs(tempCY - tempLT);
    if (tempDouble > trueRange) {
      trueRange = tempDouble;
    }
    sTR += trueRange;
    sVMP += Math.abs(inHigh[today] - inLow[today - 1]);
    sVMM += Math.abs(inLow[today] - inHigh[today - 1]);
    live = (!(trueRange <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
    /* Once the whole window is flat, every TRUE-RANGE term in it is
     * provably zero (nullRun counts exactly that), so sTR's only content
     * is the running add/subtract's rounding residue -- purge it, and the
     * exact division gate below recognizes the case with no absolute
     * band. ONLY sTR: a zero true range (H == L == prevClose) does NOT
     * zero that bar's vortex terms, which read the PREVIOUS bar's
     * extremes -- a spread bar followed by a halt leaves |H - prevL| and
     * |L - prevH| alive inside the window, and zeroing the numerator sums
     * would poison both lines permanently (an unreachable negative -VI).
     * ULTOSC can reseed all its totals because its predicate covers both
     * of its per-bar terms; VORTEX's covers only the denominator's.
     */
    if (nullRun >= optInTimePeriod) {
      nullRun = optInTimePeriod;
      sTR = 0.0;
    }
    /* Record the current window sums, then retire the trailing bar's
     * terms so the sums are ready for the next iteration.
     */
    curTR = sTR;
    curVMP = sVMP;
    curVMM = sVMM;
    tempLT = inLow[trailingIdx];
    tempHT = inHigh[trailingIdx];
    tempCY = inClose[trailingIdx - 1];
    trueRange = tempHT - tempLT;
    tempDouble = Math.abs(tempCY - tempHT);
    if (tempDouble > trueRange) {
      trueRange = tempDouble;
    }
    tempDouble = Math.abs(tempCY - tempLT);
    if (tempDouble > trueRange) {
      trueRange = tempDouble;
    }
    sTR -= trueRange;
    sVMP -= Math.abs(inHigh[trailingIdx] - inLow[trailingIdx - 1]);
    sVMM -= Math.abs(inLow[trailingIdx] - inHigh[trailingIdx - 1]);
    trailingIdx += 1;
    /* Last operation is to write the outputs. Must be done after the
     * trailing bar has been fully consumed: the caller is allowed to pass
     * an output buffer aliasing any input, and the trailing reads above
     * touch bars trailingIdx-1 == outIdx and trailingIdx == outIdx+1 --
     * an emit-first order would have clobbered them (ULTOSC's own rule;
     * ACCBANDS' multi-output form).
     *
     * Zero-denominator gate, on the DENOMINATOR itself and exact: the
     * flat-run reseed above removes the running sums' residue, so
     * `curTR > 0.0` is a precise test -- ULTOSC gates its divisions the
     * same way after the same reseed. The flat-bar count alone is only a
     * proxy for sTR == 0 in exact arithmetic: floating-point absorption
     * can zero the running sum while the window still holds a live term
     * (a large spread swallows a 1-ULP one; the later subtract leaves
     * exactly 0.0 with nullRun far below n), and an ungated division
     * then emits NaN/Inf, which VORTEX does not declare. An absolute
     * TA_IS_ZERO band is no better: it zeroes legitimate ratios on any
     * instrument quoted below 1e-14, and the QUOTE-UNIT/SCALE gate
     * rejects it (VORTEX is homogeneous of degree 0).
     */
    if (curTR > 0.0) {
      outPlusVI[outIdx] = curVMP / curTR;
      outMinusVI[outIdx] = curVMM / curTR;
    } else {
      outPlusVI[outIdx] = 0.0;
      outMinusVI[outIdx] = 0.0;
    }
    outIdx += 1;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000']] as const;

/** @internal */
export function vortexPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outPlusVI: Float64Array,
  outMinusVI: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('VORTEX', 'startIdx', startIdx);
  endIdx = requireInt32('VORTEX', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('VORTEX', 'timePeriod', optInTimePeriod);
  requireIndexRange('VORTEX', startIdx, endIdx);
  const guardLookback = vortexLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('VORTEX', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('VORTEX', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('VORTEX', 'prices.high', inHigh, guardInLen);
  requireLength('VORTEX', 'prices.low', inLow, guardInLen);
  requireLength('VORTEX', 'prices.close', inClose, guardInLen);
  requireOutput('VORTEX', 'outPlusVi', outPlusVI, guardOutLen, Float64Array);
  requireOutput('VORTEX', 'outMinusVi', outMinusVI, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outPlusVI, inHighF64)) {
    throw sharedMemory('VORTEX', 'outPlusVi', 'prices.high');
  }
  if (overlapsPartly(outPlusVI, inLowF64)) {
    throw sharedMemory('VORTEX', 'outPlusVi', 'prices.low');
  }
  if (overlapsPartly(outPlusVI, inCloseF64)) {
    throw sharedMemory('VORTEX', 'outPlusVi', 'prices.close');
  }
  if (overlapsPartly(outMinusVI, inHighF64)) {
    throw sharedMemory('VORTEX', 'outMinusVi', 'prices.high');
  }
  if (overlapsPartly(outMinusVI, inLowF64)) {
    throw sharedMemory('VORTEX', 'outMinusVi', 'prices.low');
  }
  if (overlapsPartly(outMinusVI, inCloseF64)) {
    throw sharedMemory('VORTEX', 'outMinusVi', 'prices.close');
  }
  if (overlaps(outPlusVI, outMinusVI)) {
    throw sharedMemory('VORTEX', 'outPlusVi', 'outMinusVi');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = vortexImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInTimePeriod, outBegIdx, outNBElement, outPlusVI, outMinusVI);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('VORTEX', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link vortex}, {@link vortexLookback}, {@link vortexDisplayShift},
 * {@link vortexOpen} and {@link vortexOpenAndFill}. Every key is optional: an
 * omitted or `undefined` key takes its default, and a call ignores the keys it has
 * no use for.
 */
export interface VortexOptions {
  /**
   * Number of bars in the rolling sums (default 14; range 1 to 100000;
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
   * Buffer for `outPlusVi`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outPlusVi?: Float64Array<ArrayBuffer> | undefined;
  /**
   * Buffer for `outMinusVi`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outMinusVi?: Float64Array<ArrayBuffer> | undefined;
}

/** What {@link vortex} returns, and {@link vortexOpenAndFill} with its stream. */
export interface VortexResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Positive vortex line (+VI). `outPlusVi[k]` is the value at input bar
   * `begIdx + k`.
   */
  outPlusVi: Float64Array<ArrayBuffer>;
  /**
   * Negative vortex line (−VI). `outMinusVi[k]` is the value at input bar
   * `begIdx + k`.
   */
  outMinusVi: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link vortex} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link VortexOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function vortexLookback(options?: VortexOptions): number {
  const o = optionsOf('VORTEX lookback', options);
  return vortexLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('VORTEX lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link vortex}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link VortexOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function vortexDisplayShift(outputIdx: number, options?: VortexOptions): number {
  const o = optionsOf('VORTEX displayShift', options);
  return vortexDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('VORTEX displayShift', o.core));
}

/**
 * Vortex Indicator: Etienne Botes and Douglas Siepman's two-line trend indicator
 * (*Technical Analysis of Stocks & Commodities* 28:1, January 2010). Positive and
 * negative "vortex movement" — the reach from today's high to yesterday's low and
 * from today's low to yesterday's high — each summed over the period and
 * normalized by the summed true range. A +VI line crossing above −VI is the
 * bullish signal the authors describe; the two lines are conventionally plotted
 * together.
 *
 * Formula and more info at
 * [ta-lib.org/functions/vortex](https://ta-lib.org/functions/vortex).
 *
 * **Notes**
 * - Bar 0 has no term (all three need a prior bar) and is consumed exactly as
 *   [`TRANGE`](https://ta-lib.org/functions/trange) consumes it, so the first
 *   output sits at index `timePeriod`, not `timePeriod − 1`.
 * - Not start-dependent: each output depends only on the finite trailing window.
 *   No unstable period.
 *
 * `outPlusVi[k]` is the value at input bar `begIdx + k`; every output is exactly
 * as long as the number of values written, and nothing is padded. A range that
 * ends before {@link vortexLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `high`: High price
 *   series. `low`: Low price series. `close`: Close price series.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link VortexOptions}.
 * @returns `begIdx` and the values; see {@link VortexResult}. Each output is a
 *   view of its buffer, the caller's or a new one.
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
 * import { vortex } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { begIdx, outPlusVi, outMinusVi } = vortex({ high, low, close }, {
 *   timePeriod: 14,
 * });
 * // outPlusVi[k] is the value at bar begIdx + k
 * ```
 * @see [`trange`](https://ta-lib.org/functions/trange)
 * @see [`plusDi`](https://ta-lib.org/functions/plus_di)
 * @see [`minusDi`](https://ta-lib.org/functions/minus_di)
 * @see [`adx`](https://ta-lib.org/functions/adx)
 */
export function vortex(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: VortexOptions): VortexResult {
  requireArgument('VORTEX', 'prices', prices);
  const o = optionsOf('VORTEX', options);
  if (o.endIdx === undefined) {
    requireSeries('VORTEX', 'prices.high', prices.high);
    requireSeries('VORTEX', 'prices.low', prices.low);
    requireSeries('VORTEX', 'prices.close', prices.close);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('VORTEX', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, vortexLookbackPositional(optInTimePeriod, core), prices.high, prices.low, prices.close);
  const outPlusVI = o.outPlusVi === undefined ? new Float64Array(capacity) : o.outPlusVi;
  const outMinusVI = o.outMinusVi === undefined ? new Float64Array(capacity) : o.outMinusVi;
  const range = vortexPositional(startIdx, endIdx, prices.high, prices.low, prices.close, optInTimePeriod, outPlusVI, outMinusVI, core);
  return { begIdx: range.begIdx, outPlusVi: outPlusVI.subarray(0, range.count), outMinusVi: outMinusVI.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live VORTEX stream: one value per closed bar, bit-identical to {@link vortex}
 * over the same series. Open it with {@link vortexOpen} or
 * {@link vortexOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/vortex](https://ta-lib.org/functions/vortex).
 */
export class VortexStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  nullRun: number = 0;
  /** @internal */
  sTR: number = 0;
  /** @internal */
  sVMP: number = 0;
  /** @internal */
  sVMM: number = 0;
  /** @internal */
  lag1_inHigh: number = 0;
  /** @internal */
  lag1_inLow: number = 0;
  /** @internal */
  lag1_inClose: number = 0;
  /** @internal */
  ringPos_trailingIdx: number = 0;
  /** @internal */
  ringCap_trailingIdx: number = 2;
  /** @internal */
  ringLag_trailingIdx: number = 0;
  /** @internal */
  ring_trailingIdx_inHigh: Float64Array = new Float64Array(2);
  /** @internal */
  ring_trailingIdx_inLow: Float64Array = new Float64Array(2);
  /** @internal */
  ring_trailingIdx_inClose: Float64Array = new Float64Array(2);
  /** @internal */
  cur_outPlusVI: number = 0;
  /** @internal */
  cur_outMinusVI: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): VortexStream {
    return new VortexStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link vortex} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('VORTEX advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns its outputs.
   *
   * @param high - The bar's `high` value: High price series.
   * @param low - The bar's `low` value: Low price series.
   * @param close - The bar's `close` value: Close price series.
   * @param out - Receives the outputs and is returned; a new {@link VortexOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included, before anything moves.
   *   `TALibError` with code `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached
   *   bar `Core.INDEX_MAX`.
   */
  update(high: number, low: number, close: number, out?: VortexOut): VortexOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('VORTEX update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('VORTEX update', 'VortexOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('VORTEX update', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    vortexStepImpl(this, high, low, close);
    this.outRangeCount++;
    if (out === undefined) {
      out = new VortexOut();
    }
    out.outPlusVi = this.cur_outPlusVI;
    out.outMinusVi = this.cur_outMinusVI;
    return out;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param high - The bar's `high` value: High price series.
   * @param low - The bar's `low` value: Low price series.
   * @param close - The bar's `close` value: Close price series.
   * @param out - Receives the outputs and is returned; a new {@link VortexOut} when
   *   omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included.
   */
  peek(high: number, low: number, close: number, out?: VortexOut): VortexOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('VORTEX peek', 'VortexOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('VORTEX peek', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
    const inClose = close;
    let live = 0;
    let curTR = 0;
    let curVMP = 0;
    let curVMM = 0;
    let trueRange = 0;
    let tempDouble = 0;
    let tempLT = 0;
    let tempHT = 0;
    let tempCY = 0;
    let cur_outMinusVI = 0;
    let cur_outPlusVI = 0;
    let nullRun = sp.nullRun;
    let sTR = sp.sTR;
    let sVMM = sp.sVMM;
    let sVMP = sp.sVMP;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    let pkSlot1 = -1;
    let pkVal1 = 0;
    let pkSlot2 = -1;
    let pkVal2 = 0;
    pkSlot0 = sp.ringPos_trailingIdx;
    pkVal0 = inHigh;
    pkSlot1 = sp.ringPos_trailingIdx;
    pkVal1 = inLow;
    pkSlot2 = sp.ringPos_trailingIdx;
    pkVal2 = inClose;
    /* Add on today's terms. */
    tempLT = inLow;
    tempHT = inHigh;
    tempCY = sp.lag1_inClose;
    trueRange = tempHT - tempLT;
    tempDouble = Math.abs(tempCY - tempHT);
    if (tempDouble > trueRange) {
      trueRange = tempDouble;
    }
    tempDouble = Math.abs(tempCY - tempLT);
    if (tempDouble > trueRange) {
      trueRange = tempDouble;
    }
    sTR += trueRange;
    sVMP += Math.abs(inHigh - sp.lag1_inLow);
    sVMM += Math.abs(inLow - sp.lag1_inHigh);
    live = (!(trueRange <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
    /* Once the whole window is flat, every TRUE-RANGE term in it is
     * provably zero (nullRun counts exactly that), so sTR's only content
     * is the running add/subtract's rounding residue -- purge it, and the
     * exact division gate below recognizes the case with no absolute
     * band. ONLY sTR: a zero true range (H == L == prevClose) does NOT
     * zero that bar's vortex terms, which read the PREVIOUS bar's
     * extremes -- a spread bar followed by a halt leaves |H - prevL| and
     * |L - prevH| alive inside the window, and zeroing the numerator sums
     * would poison both lines permanently (an unreachable negative -VI).
     * ULTOSC can reseed all its totals because its predicate covers both
     * of its per-bar terms; VORTEX's covers only the denominator's.
     */
    if (nullRun >= sp.optInTimePeriod) {
      nullRun = sp.optInTimePeriod;
      sTR = 0.0;
    }
    /* Record the current window sums, then retire the trailing bar's
     * terms so the sums are ready for the next iteration.
     */
    curTR = sTR;
    curVMP = sVMP;
    curVMM = sVMM;
    tempLT = ((((sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx) % sp.ringCap_trailingIdx) | 0) !== pkSlot1) ? sp.ring_trailingIdx_inLow[(((sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx) % sp.ringCap_trailingIdx) | 0)] : pkVal1;
    tempHT = ((((sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx) % sp.ringCap_trailingIdx) | 0) !== pkSlot0) ? sp.ring_trailingIdx_inHigh[(((sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx) % sp.ringCap_trailingIdx) | 0)] : pkVal0;
    tempCY = ((((sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx - 1) % sp.ringCap_trailingIdx) | 0) !== pkSlot2) ? sp.ring_trailingIdx_inClose[(((sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx - 1) % sp.ringCap_trailingIdx) | 0)] : pkVal2;
    trueRange = tempHT - tempLT;
    tempDouble = Math.abs(tempCY - tempHT);
    if (tempDouble > trueRange) {
      trueRange = tempDouble;
    }
    tempDouble = Math.abs(tempCY - tempLT);
    if (tempDouble > trueRange) {
      trueRange = tempDouble;
    }
    sTR -= trueRange;
    sVMP -= Math.abs((((((sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx) % sp.ringCap_trailingIdx) | 0) !== pkSlot0) ? sp.ring_trailingIdx_inHigh[(((sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx) % sp.ringCap_trailingIdx) | 0)] : pkVal0) - (((((sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx - 1) % sp.ringCap_trailingIdx) | 0) !== pkSlot1) ? sp.ring_trailingIdx_inLow[(((sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx - 1) % sp.ringCap_trailingIdx) | 0)] : pkVal1));
    sVMM -= Math.abs((((((sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx) % sp.ringCap_trailingIdx) | 0) !== pkSlot1) ? sp.ring_trailingIdx_inLow[(((sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx) % sp.ringCap_trailingIdx) | 0)] : pkVal1) - (((((sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx - 1) % sp.ringCap_trailingIdx) | 0) !== pkSlot0) ? sp.ring_trailingIdx_inHigh[(((sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx - 1) % sp.ringCap_trailingIdx) | 0)] : pkVal0));
    /* Last operation is to write the outputs. Must be done after the
     * trailing bar has been fully consumed: the caller is allowed to pass
     * an output buffer aliasing any input, and the trailing reads above
     * touch bars trailingIdx-1 == outIdx and trailingIdx == outIdx+1 --
     * an emit-first order would have clobbered them (ULTOSC's own rule;
     * ACCBANDS' multi-output form).
     *
     * Zero-denominator gate, on the DENOMINATOR itself and exact: the
     * flat-run reseed above removes the running sums' residue, so
     * `curTR > 0.0` is a precise test -- ULTOSC gates its divisions the
     * same way after the same reseed. The flat-bar count alone is only a
     * proxy for sTR == 0 in exact arithmetic: floating-point absorption
     * can zero the running sum while the window still holds a live term
     * (a large spread swallows a 1-ULP one; the later subtract leaves
     * exactly 0.0 with nullRun far below n), and an ungated division
     * then emits NaN/Inf, which VORTEX does not declare. An absolute
     * TA_IS_ZERO band is no better: it zeroes legitimate ratios on any
     * instrument quoted below 1e-14, and the QUOTE-UNIT/SCALE gate
     * rejects it (VORTEX is homogeneous of degree 0).
     */
    if (curTR > 0.0) {
      cur_outPlusVI = curVMP / curTR;
      cur_outMinusVI = curVMM / curTR;
    } else {
      cur_outPlusVI = 0.0;
      cur_outMinusVI = 0.0;
    }
    if (out === undefined) {
      out = new VortexOut();
    }
    out.outPlusVi = cur_outPlusVI;
    out.outMinusVi = cur_outMinusVI;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link VortexOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a sink that is not an object,
   *   `null` included.
   */
  value(out?: VortexOut): VortexOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('VORTEX value', 'VortexOut');
    }
    if (out === undefined) {
      out = new VortexOut();
    }
    out.outPlusVi = this.cur_outPlusVI;
    out.outMinusVi = this.cur_outMinusVI;
    return out;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): VortexStream {
    const c = new VortexStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.nullRun = this.nullRun;
    c.sTR = this.sTR;
    c.sVMP = this.sVMP;
    c.sVMM = this.sVMM;
    c.lag1_inHigh = this.lag1_inHigh;
    c.lag1_inLow = this.lag1_inLow;
    c.lag1_inClose = this.lag1_inClose;
    c.ringPos_trailingIdx = this.ringPos_trailingIdx;
    c.ringCap_trailingIdx = this.ringCap_trailingIdx;
    c.ringLag_trailingIdx = this.ringLag_trailingIdx;
    c.ring_trailingIdx_inHigh = this.ring_trailingIdx_inHigh.slice();
    c.ring_trailingIdx_inLow = this.ring_trailingIdx_inLow.slice();
    c.ring_trailingIdx_inClose = this.ring_trailingIdx_inClose.slice();
    c.cur_outPlusVI = this.cur_outPlusVI;
    c.cur_outMinusVI = this.cur_outMinusVI;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one VORTEX bar, as `update`, `peek` and `value` return them.
 * Passed back to one of them, it is filled and returned again, with no allocation;
 * it holds its values until the next call that writes it. A new one holds `NaN`.
 */
export class VortexOut {
  /** Positive vortex line (+VI) */
  outPlusVi: number = NaN;
  /** Negative vortex line (−VI) */
  outMinusVi: number = NaN;
}

function vortexStepImpl(sp: VortexStream, inHigh: number, inLow: number, inClose: number): void {
  let live = 0;
  let curTR = 0;
  let curVMP = 0;
  let curVMM = 0;
  let trueRange = 0;
  let tempDouble = 0;
  let tempLT = 0;
  let tempHT = 0;
  let tempCY = 0;
  sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx] = inHigh;
  sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx] = inLow;
  sp.ring_trailingIdx_inClose[sp.ringPos_trailingIdx] = inClose;
  /* Add on today's terms. */
  tempLT = inLow;
  tempHT = inHigh;
  tempCY = sp.lag1_inClose;
  trueRange = tempHT - tempLT;
  tempDouble = Math.abs(tempCY - tempHT);
  if (tempDouble > trueRange) {
    trueRange = tempDouble;
  }
  tempDouble = Math.abs(tempCY - tempLT);
  if (tempDouble > trueRange) {
    trueRange = tempDouble;
  }
  sp.sTR += trueRange;
  sp.sVMP += Math.abs(inHigh - sp.lag1_inLow);
  sp.sVMM += Math.abs(inLow - sp.lag1_inHigh);
  live = (!(trueRange <= 0.0) ? 1 : 0);
  sp.nullRun = (sp.nullRun + 1) & (live - 1);
  /* Once the whole window is flat, every TRUE-RANGE term in it is
   * provably zero (nullRun counts exactly that), so sTR's only content
   * is the running add/subtract's rounding residue -- purge it, and the
   * exact division gate below recognizes the case with no absolute
   * band. ONLY sTR: a zero true range (H == L == prevClose) does NOT
   * zero that bar's vortex terms, which read the PREVIOUS bar's
   * extremes -- a spread bar followed by a halt leaves |H - prevL| and
   * |L - prevH| alive inside the window, and zeroing the numerator sums
   * would poison both lines permanently (an unreachable negative -VI).
   * ULTOSC can reseed all its totals because its predicate covers both
   * of its per-bar terms; VORTEX's covers only the denominator's.
   */
  if (sp.nullRun >= sp.optInTimePeriod) {
    sp.nullRun = sp.optInTimePeriod;
    sp.sTR = 0.0;
  }
  /* Record the current window sums, then retire the trailing bar's
   * terms so the sums are ready for the next iteration.
   */
  curTR = sp.sTR;
  curVMP = sp.sVMP;
  curVMM = sp.sVMM;
  tempLT = sp.ring_trailingIdx_inLow[(((sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx) % sp.ringCap_trailingIdx) | 0)];
  tempHT = sp.ring_trailingIdx_inHigh[(((sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx) % sp.ringCap_trailingIdx) | 0)];
  tempCY = sp.ring_trailingIdx_inClose[(((sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx - 1) % sp.ringCap_trailingIdx) | 0)];
  trueRange = tempHT - tempLT;
  tempDouble = Math.abs(tempCY - tempHT);
  if (tempDouble > trueRange) {
    trueRange = tempDouble;
  }
  tempDouble = Math.abs(tempCY - tempLT);
  if (tempDouble > trueRange) {
    trueRange = tempDouble;
  }
  sp.sTR -= trueRange;
  sp.sVMP -= Math.abs(sp.ring_trailingIdx_inHigh[(((sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx) % sp.ringCap_trailingIdx) | 0)] - sp.ring_trailingIdx_inLow[(((sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx - 1) % sp.ringCap_trailingIdx) | 0)]);
  sp.sVMM -= Math.abs(sp.ring_trailingIdx_inLow[(((sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx) % sp.ringCap_trailingIdx) | 0)] - sp.ring_trailingIdx_inHigh[(((sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx - 1) % sp.ringCap_trailingIdx) | 0)]);
  /* Last operation is to write the outputs. Must be done after the
   * trailing bar has been fully consumed: the caller is allowed to pass
   * an output buffer aliasing any input, and the trailing reads above
   * touch bars trailingIdx-1 == outIdx and trailingIdx == outIdx+1 --
   * an emit-first order would have clobbered them (ULTOSC's own rule;
   * ACCBANDS' multi-output form).
   *
   * Zero-denominator gate, on the DENOMINATOR itself and exact: the
   * flat-run reseed above removes the running sums' residue, so
   * `curTR > 0.0` is a precise test -- ULTOSC gates its divisions the
   * same way after the same reseed. The flat-bar count alone is only a
   * proxy for sTR == 0 in exact arithmetic: floating-point absorption
   * can zero the running sum while the window still holds a live term
   * (a large spread swallows a 1-ULP one; the later subtract leaves
   * exactly 0.0 with nullRun far below n), and an ungated division
   * then emits NaN/Inf, which VORTEX does not declare. An absolute
   * TA_IS_ZERO band is no better: it zeroes legitimate ratios on any
   * instrument quoted below 1e-14, and the QUOTE-UNIT/SCALE gate
   * rejects it (VORTEX is homogeneous of degree 0).
   */
  if (curTR > 0.0) {
    sp.cur_outPlusVI = curVMP / curTR;
    sp.cur_outMinusVI = curVMM / curTR;
  } else {
    sp.cur_outPlusVI = 0.0;
    sp.cur_outMinusVI = 0.0;
  }
  sp.lag1_inHigh = inHigh;
  sp.lag1_inLow = inLow;
  sp.lag1_inClose = inClose;
  sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
  if (sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx) {
    sp.ringPos_trailingIdx = 0;
  }
}

function vortexOpenImpl(
  sp: VortexStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outPlusVI: Float64Array,
  outMinusVI: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let outIdx = 0;
  let today = 0;
  let trailingIdx = 0;
  let lookbackTotal = 0;
  let i = 0;
  let nullRun = 0;
  let live = 0;
  let sTR = 0;
  let sVMP = 0;
  let sVMM = 0;
  let curTR = 0;
  let curVMP = 0;
  let curVMM = 0;
  let trueRange = 0;
  let tempDouble = 0;
  let tempLT = 0;
  let tempHT = 0;
  let tempCY = 0;
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
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Vortex Indicator (Botes & Siepman, TASC 28:1, Jan 2010): two lines,
   * each a rolling sum of "vortex movement" normalized by the rolling sum
   * of true range over the same optInTimePeriod bars.
   *
   *   TR[i]  = max( H[i]-L[i], |C[i-1]-H[i]|, |C[i-1]-L[i]| )   == TA_TRANGE
   *   VMP[i] = |H[i] - L[i-1]|
   *   VMM[i] = |L[i] - H[i-1]|
   *   +VI = SUM(VMP, n) / SUM(TR, n),  -VI = SUM(VMM, n) / SUM(TR, n)
   *
   * No smoothing, no recursion, nothing to seed. The TR expansion below is
   * TA_TRANGE's own operation order, bit for bit -- the differential test
   * composes TA_TRANGE + TA_SUM and asserts equality with memcmp.
   *
   * The trailing terms are recomputed from the inputs rather than carried
   * in a ring; the subtraction re-reads bars trailingIdx and trailingIdx-1,
   * both of which sit at or ahead of the output slot, which is why the
   * outputs are written LAST (see the loop comment).
   */
  lookbackTotal = vortexLookbackPositional(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Prime the three window sums over the optInTimePeriod-1 terms before the
   * first output bar: [startIdx-optInTimePeriod+1, startIdx). Each term at
   * bar i reads bar i-1, so the earliest read is bar startIdx-optInTimePeriod
   * >= 0.
   */
  sTR = 0.0;
  sVMP = 0.0;
  sVMM = 0.0;
  nullRun = 0;
  for (i = startIdx - optInTimePeriod + 1; i < startIdx; i += 1) {
    tempLT = inLow[i];
    tempHT = inHigh[i];
    tempCY = inClose[i - 1];
    trueRange = tempHT - tempLT;
    tempDouble = Math.abs(tempCY - tempHT);
    if (tempDouble > trueRange) {
      trueRange = tempDouble;
    }
    tempDouble = Math.abs(tempCY - tempLT);
    if (tempDouble > trueRange) {
      trueRange = tempDouble;
    }
    sTR += trueRange;
    sVMP += Math.abs(inHigh[i] - inLow[i - 1]);
    sVMM += Math.abs(inLow[i] - inHigh[i - 1]);
    live = (!(trueRange <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
  }
  outIdx = 0;
  today = startIdx;
  trailingIdx = startIdx - optInTimePeriod + 1;
  while (today <= endIdx) {
    /* Add on today's terms. */
    tempLT = inLow[today];
    tempHT = inHigh[today];
    tempCY = inClose[today - 1];
    trueRange = tempHT - tempLT;
    tempDouble = Math.abs(tempCY - tempHT);
    if (tempDouble > trueRange) {
      trueRange = tempDouble;
    }
    tempDouble = Math.abs(tempCY - tempLT);
    if (tempDouble > trueRange) {
      trueRange = tempDouble;
    }
    sTR += trueRange;
    sVMP += Math.abs(inHigh[today] - inLow[today - 1]);
    sVMM += Math.abs(inLow[today] - inHigh[today - 1]);
    live = (!(trueRange <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
    /* Once the whole window is flat, every TRUE-RANGE term in it is
     * provably zero (nullRun counts exactly that), so sTR's only content
     * is the running add/subtract's rounding residue -- purge it, and the
     * exact division gate below recognizes the case with no absolute
     * band. ONLY sTR: a zero true range (H == L == prevClose) does NOT
     * zero that bar's vortex terms, which read the PREVIOUS bar's
     * extremes -- a spread bar followed by a halt leaves |H - prevL| and
     * |L - prevH| alive inside the window, and zeroing the numerator sums
     * would poison both lines permanently (an unreachable negative -VI).
     * ULTOSC can reseed all its totals because its predicate covers both
     * of its per-bar terms; VORTEX's covers only the denominator's.
     */
    if (nullRun >= optInTimePeriod) {
      nullRun = optInTimePeriod;
      sTR = 0.0;
    }
    /* Record the current window sums, then retire the trailing bar's
     * terms so the sums are ready for the next iteration.
     */
    curTR = sTR;
    curVMP = sVMP;
    curVMM = sVMM;
    tempLT = inLow[trailingIdx];
    tempHT = inHigh[trailingIdx];
    tempCY = inClose[trailingIdx - 1];
    trueRange = tempHT - tempLT;
    tempDouble = Math.abs(tempCY - tempHT);
    if (tempDouble > trueRange) {
      trueRange = tempDouble;
    }
    tempDouble = Math.abs(tempCY - tempLT);
    if (tempDouble > trueRange) {
      trueRange = tempDouble;
    }
    sTR -= trueRange;
    sVMP -= Math.abs(inHigh[trailingIdx] - inLow[trailingIdx - 1]);
    sVMM -= Math.abs(inLow[trailingIdx] - inHigh[trailingIdx - 1]);
    trailingIdx += 1;
    /* Last operation is to write the outputs. Must be done after the
     * trailing bar has been fully consumed: the caller is allowed to pass
     * an output buffer aliasing any input, and the trailing reads above
     * touch bars trailingIdx-1 == outIdx and trailingIdx == outIdx+1 --
     * an emit-first order would have clobbered them (ULTOSC's own rule;
     * ACCBANDS' multi-output form).
     *
     * Zero-denominator gate, on the DENOMINATOR itself and exact: the
     * flat-run reseed above removes the running sums' residue, so
     * `curTR > 0.0` is a precise test -- ULTOSC gates its divisions the
     * same way after the same reseed. The flat-bar count alone is only a
     * proxy for sTR == 0 in exact arithmetic: floating-point absorption
     * can zero the running sum while the window still holds a live term
     * (a large spread swallows a 1-ULP one; the later subtract leaves
     * exactly 0.0 with nullRun far below n), and an ungated division
     * then emits NaN/Inf, which VORTEX does not declare. An absolute
     * TA_IS_ZERO band is no better: it zeroes legitimate ratios on any
     * instrument quoted below 1e-14, and the QUOTE-UNIT/SCALE gate
     * rejects it (VORTEX is homogeneous of degree 0).
     */
    if (curTR > 0.0) {
      outPlusVI[((outIdx * outStride) | 0)] = curVMP / curTR;
      outMinusVI[((outIdx * outStride) | 0)] = curVMM / curTR;
    } else {
      outPlusVI[((outIdx * outStride) | 0)] = 0.0;
      outMinusVI[((outIdx * outStride) | 0)] = 0.0;
    }
    outIdx += 1;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  const capLag_trailingIdx = today - trailingIdx;
  const cap_trailingIdx = capLag_trailingIdx + 2;
  if (capLag_trailingIdx < 0 || cap_trailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_trailingIdx = cap_trailingIdx > 0 ? cap_trailingIdx : 1;
  const capRing_trailingIdx_inHigh = new Float64Array(allocN_trailingIdx);
  for (let fillJ = historyLen - cap_trailingIdx; fillJ < historyLen; fillJ++) {
    capRing_trailingIdx_inHigh[fillJ % cap_trailingIdx] = inHigh[fillJ];
  }
  const capRing_trailingIdx_inLow = new Float64Array(allocN_trailingIdx);
  for (let fillJ = historyLen - cap_trailingIdx; fillJ < historyLen; fillJ++) {
    capRing_trailingIdx_inLow[fillJ % cap_trailingIdx] = inLow[fillJ];
  }
  const capRing_trailingIdx_inClose = new Float64Array(allocN_trailingIdx);
  for (let fillJ = historyLen - cap_trailingIdx; fillJ < historyLen; fillJ++) {
    capRing_trailingIdx_inClose[fillJ % cap_trailingIdx] = inClose[fillJ];
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.nullRun = nullRun;
  sp.sTR = sTR;
  sp.sVMP = sVMP;
  sp.sVMM = sVMM;
  sp.lag1_inHigh = inHigh[historyLen - 1];
  sp.lag1_inLow = inLow[historyLen - 1];
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.ringPos_trailingIdx = historyLen % cap_trailingIdx;
  sp.ringCap_trailingIdx = cap_trailingIdx;
  sp.ringLag_trailingIdx = capLag_trailingIdx;
  sp.ring_trailingIdx_inHigh = capRing_trailingIdx_inHigh;
  sp.ring_trailingIdx_inLow = capRing_trailingIdx_inLow;
  sp.ring_trailingIdx_inClose = capRing_trailingIdx_inClose;
  sp.cur_outPlusVI = outPlusVI[(outNBElement.value - 1) * outStride];
  sp.cur_outMinusVI = outMinusVI[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function vortexOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outPlusVI: Float64Array,
  outMinusVI: Float64Array,
): VortexStream {
  const sp = VortexStream.of(core);
  const retCode = vortexOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outPlusVI, outMinusVI, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('VORTEX openAndFill', inHigh.length, startIdx, vortexLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && vortexLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('VORTEX openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('VORTEX openAndFill', retCode);
}

/** @internal */
export function vortexOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): VortexStream {
  const sp = VortexStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outPlusVI = new Float64Array(1);
  const sink_outMinusVI = new Float64Array(1);
  const retCode = vortexOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outPlusVI, sink_outMinusVI, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('VORTEX open', inHigh.length, startIdx, vortexLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && vortexLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('VORTEX open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('VORTEX open', retCode);
}

/** @internal */
export function vortexOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): VortexStream {
  optInTimePeriod = requireInt32('VORTEX open', 'timePeriod', optInTimePeriod);
  requireArgument('VORTEX open', 'prices.high', inHigh);
  requireHistory('VORTEX open', 'prices.high', inHigh.length);
  requireArgument('VORTEX open', 'prices.low', inLow);
  requireArgument('VORTEX open', 'prices.close', inClose);
  requireHistoryLength('VORTEX open', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('VORTEX open', 'prices.close', inClose.length, inHigh.length);
  return vortexOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInTimePeriod);
}

/** @internal */
export function vortexOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outPlusVI: Float64Array,
  outMinusVI: Float64Array,
  core: Core = Core.DEFAULT,
): VortexStream {
  optInTimePeriod = requireInt32('VORTEX openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('VORTEX openAndFill', 'prices.high', inHigh);
  requireHistory('VORTEX openAndFill', 'prices.high', inHigh.length);
  requireArgument('VORTEX openAndFill', 'prices.low', inLow);
  requireArgument('VORTEX openAndFill', 'prices.close', inClose);
  const guardLookback = vortexLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('VORTEX openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('VORTEX openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('VORTEX openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('VORTEX openAndFill', 'prices.close', inClose.length, inHigh.length);
  requireOutput('VORTEX openAndFill', 'outPlusVi', outPlusVI, guardOutLen, Float64Array);
  requireOutput('VORTEX openAndFill', 'outMinusVi', outMinusVI, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outPlusVI.buffer === inHighF64.buffer && outPlusVI.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('VORTEX openAndFill', 'outPlusVi', 'prices.high');
  }
  if (outPlusVI.buffer === inLowF64.buffer && outPlusVI.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('VORTEX openAndFill', 'outPlusVi', 'prices.low');
  }
  if (outPlusVI.buffer === inCloseF64.buffer && outPlusVI.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('VORTEX openAndFill', 'outPlusVi', 'prices.close');
  }
  if (outMinusVI.buffer === inHighF64.buffer && outMinusVI.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('VORTEX openAndFill', 'outMinusVi', 'prices.high');
  }
  if (outMinusVI.buffer === inLowF64.buffer && outMinusVI.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('VORTEX openAndFill', 'outMinusVi', 'prices.low');
  }
  if (outMinusVI.buffer === inCloseF64.buffer && outMinusVI.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('VORTEX openAndFill', 'outMinusVi', 'prices.close');
  }
  if (outPlusVI.buffer === outMinusVI.buffer && outPlusVI.byteOffset === outMinusVI.byteOffset) {
    throw sharedMemory('VORTEX openAndFill', 'outPlusVi', 'outMinusVi');
  }
  if (guardOutLen > 0 && overlaps(outPlusVI, inHighF64)) {
    throw sharedMemory('VORTEX openAndFill', 'outPlusVi', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outPlusVI, inLowF64)) {
    throw sharedMemory('VORTEX openAndFill', 'outPlusVi', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outPlusVI, inCloseF64)) {
    throw sharedMemory('VORTEX openAndFill', 'outPlusVi', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outMinusVI, inHighF64)) {
    throw sharedMemory('VORTEX openAndFill', 'outMinusVi', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outMinusVI, inLowF64)) {
    throw sharedMemory('VORTEX openAndFill', 'outMinusVi', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outMinusVI, inCloseF64)) {
    throw sharedMemory('VORTEX openAndFill', 'outMinusVi', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outPlusVI, outMinusVI)) {
    throw sharedMemory('VORTEX openAndFill', 'outPlusVi', 'outMinusVi');
  }
  return vortexOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outPlusVI, outMinusVI);
}

/**
 * Opens a live VORTEX stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link vortex} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price series. `low`: Low price series. `close`: Close price series.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link VortexOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `vortexLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range
 *   or histories of different lengths.
 *
 * @example
 * ```ts
 * import { vortexOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = vortexOpen({ high, low, close }, { timePeriod: 14 });
 * const peeked = stream.peek(101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function vortexOpen(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: VortexOptions): VortexStream {
  requireArgument('VORTEX open', 'prices', prices);
  const o = optionsOf('VORTEX open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('VORTEX open', o.core);
  return vortexOpenPositional(prices.high, prices.low, prices.close, optInTimePeriod, core);
}

/**
 * {@link vortexOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link vortex}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price series. `low`: Low price series. `close`: Close price series.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link VortexOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link vortex} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `vortexLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { vortexOpenAndFill } from 'ta-lib';
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
 *   outPlusVi,
 *   outMinusVi,
 * } = vortexOpenAndFill({ high, low, close }, { timePeriod: 14 });
 * // outPlusVi[k] is the value at bar begIdx + k
 * ```
 */
export function vortexOpenAndFill(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: VortexOptions): VortexResult & { stream: VortexStream } {
  requireArgument('VORTEX openAndFill', 'prices', prices);
  const o = optionsOf('VORTEX openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('VORTEX openAndFill', o.core);
  const capacity = fillCapacity(vortexLookbackPositional(optInTimePeriod, core), prices.high, prices.low, prices.close);
  const outPlusVI = o.outPlusVi === undefined ? new Float64Array(capacity) : o.outPlusVi;
  const outMinusVI = o.outMinusVi === undefined ? new Float64Array(capacity) : o.outMinusVi;
  const stream = vortexOpenAndFillPositional(prices.high, prices.low, prices.close, optInTimePeriod, outPlusVI, outMinusVI, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outPlusVi: outPlusVI.subarray(0, range.count), outMinusVi: outMinusVI.subarray(0, range.count) };
}
