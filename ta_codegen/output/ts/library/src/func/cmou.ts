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
 *  071626 MF,CC  Initial version (#124).
 *  082326 MF,CC  Fix #253. Recognize a flat window by counting bars, so the
 *                0/0 is guarded exactly instead of against the fixed
 *                TA_IS_ZERO band -- which zeroed the oscillator for any
 *                instrument quoted small enough to fall under it.
 *  102326 MF,CC  #554 branch-free up/down split and zero run, rare reset.
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function cmouLookbackPositional(optInTimePeriod: number, _core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  /* CMOU needs optInTimePeriod price changes -> optInTimePeriod+1 prices ->
   * the first output is at index optInTimePeriod.
   *
   * Unlike the shipped CMO, there is NO unstable period: CMOU is a plain
   * moving-window sum, so its lookback is exactly the period.
   */
  return optInTimePeriod;
}

/** @internal */
export function cmouDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (cmouLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cmouImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let outIdx = 0;
  let today = 0;
  let trailingIdx = 0;
  let lookbackTotal = 0;
  let i = 0;
  let nullRun = 0;
  let live = 0;
  let upSum = 0;
  let downSum = 0;
  let sum = 0;
  let diff = 0;
  let up = 0;
  let tempReal = 0;
  let prevValue = 0;
  let trailingValue = 0;
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
  /* CMOU -- unsmoothed Chande Momentum Oscillator (as in TradingView ta.cmo,
   * pandas-ta default). Over the trailing optInTimePeriod changes
   * d = inReal[i]-inReal[i-1]: Su = sum of up-moves (d>0), Sd = sum of
   * |down-moves| (d<0); CMOU = 100*(Su-Sd)/(Su+Sd), 0 for a flat window. A plain
   * moving-window sum (drop oldest change, add newest), NOT TA_CMO's Wilder
   * smoothing -- hence no unstable period.
   *
   * In-place safe (outReal == inReal): the trailing read inReal[trailingIdx]
   * is of a slot not yet written (trailingIdx >= outIdx), and the oldest
   * change's older endpoint comes from the `trailingValue` cache, not a re-read.
   */
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = cmouLookbackPositional(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  /* Accumulate the up/down sums over the first window: the optInTimePeriod
   * changes ending at startIdx (prices inReal[startIdx-optInTimePeriod ..
   * startIdx]). `trailingValue` caches the oldest price so the window's oldest
   * change can later be dropped by reading only the newer of its two prices.
   * `trailingIdx` points AT that newer price (one past the cached one).
   */
  today = startIdx - lookbackTotal;
  trailingIdx = today + 1;
  prevValue = inReal[today];
  trailingValue = prevValue;
  upSum = 0.0;
  downSum = 0.0;
  /* Consecutive changes of exactly zero, counted so that an empty window can
   * be recognized exactly (the shape #244 needed for MFI). The sums cannot
   * answer that question themselves once the window starts sliding: they are
   * maintained by add-then-subtract, so an emptied window leaves them holding
   * rounding residue of arbitrary sign rather than zero.
   *
   * Both the split and the run are branch-free, as an if/else on the sign or
   * on a zero mispredicts on random or flat-heavy prices. `up` is max(diff, 0)
   * spelled with fabs, which no backend turns back into a branch; adding its
   * +0.0 is exact because neither sum is ever -0.0.
   */
  nullRun = 0;
  for (i = 0; i < optInTimePeriod; i += 1) {
    today += 1;
    tempReal = inReal[today];
    diff = tempReal - prevValue;
    prevValue = tempReal;
    up = (diff + Math.abs(diff)) * 0.5;
    upSum += up;
    downSum += up - diff;
    live = (!(Math.abs(diff) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
  }
  /* Emit the first output (bar startIdx). Su+Sd is a sum of non-negative
   * magnitudes, so it is zero only for an exactly flat window; guard the 0/0
   * exactly and emit 0.0. Not against a fixed band: a price change carries the
   * quote unit, so a constant put against the total zeroed the oscillator for
   * any instrument quoted below it (issue #253).
   *
   * Scale-then-divide -- (100*(Su-Sd))/(Su+Sd), NOT the 100*((Su-Sd)/(Su+Sd))
   * order TA_CMO/RSI use -- so CMOU is BIT-IDENTICAL to the reference unsmoothed
   * CMO of Tulip Indicators (ti_cmo) and pandas-ta-classic (cmo, talib=False),
   * which both scale before dividing. The two orders differ by <=1 ULP.
   */
  outIdx = 0;
  sum = upSum + downSum;
  if (sum > 0.0) {
    outReal[outIdx++] = 100.0 * (upSum - downSum) / sum;
  } else {
    outReal[outIdx++] = 0.0;
  }
  /* Slide the window forward one bar at a time. Each bar drops the change
   * that leaves the window AFTER its output, ready for the next bar: the
   * sums see the same operations in the same order, and a peek, which keeps
   * nothing past its output, skips that half of the work. The oldest
   * change's older price comes from the cache.
   */
  tempReal = inReal[trailingIdx];
  diff = tempReal - trailingValue;
  trailingValue = tempReal;
  trailingIdx += 1;
  up = (diff + Math.abs(diff)) * 0.5;
  upSum -= up;
  downSum -= up - diff;
  today += 1;
  while (today <= endIdx) {
    /* Add the newest change: inReal[today] - inReal[today-1]. */
    tempReal = inReal[today];
    diff = tempReal - prevValue;
    prevValue = tempReal;
    up = (diff + Math.abs(diff)) * 0.5;
    upSum += up;
    downSum += up - diff;
    /* Once a whole period of flat bars has gone by, every change in the
     * window is exactly zero, so both sums are known to be exactly zero and
     * the residue can be dropped.
     */
    live = (!(Math.abs(diff) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
    if (nullRun >= optInTimePeriod) {
      nullRun = optInTimePeriod;
      upSum = 0.0;
      downSum = 0.0;
      outReal[outIdx++] = 0.0;
    } else {
      sum = upSum + downSum;
      if (sum > 0.0) {
        outReal[outIdx++] = 100.0 * (upSum - downSum) / sum;
      } else {
        outReal[outIdx++] = 0.0;
      }
    }
    tempReal = inReal[trailingIdx];
    diff = tempReal - trailingValue;
    trailingValue = tempReal;
    trailingIdx += 1;
    up = (diff + Math.abs(diff)) * 0.5;
    upSum -= up;
    downSum -= up - diff;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000']] as const;

/** @internal */
export function cmouPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CMOU', 'startIdx', startIdx);
  endIdx = requireInt32('CMOU', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('CMOU', 'timePeriod', optInTimePeriod);
  requireIndexRange('CMOU', startIdx, endIdx);
  const guardLookback = cmouLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('CMOU', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('CMOU', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CMOU', 'real', inReal, guardInLen);
  requireOutput('CMOU', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('CMOU', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cmouImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CMOU', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cmou}, {@link cmouLookback}, {@link cmouDisplayShift},
 * {@link cmouOpen} and {@link cmouOpenAndFill}. Every key is optional: an omitted
 * or `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface CmouOptions {
  /**
   * Number of trailing price changes summed (default 14; range 2 to 100000;
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
  outReal?: Float64Array | undefined;
}

/** What {@link cmou} returns, and {@link cmouOpenAndFill} with its stream. */
export interface CmouResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /** CMOU oscillator value. `values[k]` is the value at input bar `begIdx + k`. */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link cmou} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CmouOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cmouLookback(options?: CmouOptions): number {
  const o = optionsOf('CMOU lookback', options);
  return cmouLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('CMOU lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cmou}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CmouOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cmouDisplayShift(outputIdx: number, options?: CmouOptions): number {
  const o = optionsOf('CMOU displayShift', options);
  return cmouDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('CMOU displayShift', o.core));
}

/**
 * Chande Momentum Oscillator: Tushar Chande's original momentum oscillator,
 * computed from **plain moving-window sums** of the up-moves and down-moves over
 * the period. Bounded in \[-100,+100]; positive = net upward momentum, negative =
 * net downward. CMOU is the version as defined by Chande in his book *The New
 * Technical Trader* (1994), and is the more common implementation used by
 * TradingView (`ta.cmo`) and pandas-ta's default. See
 * [`CMO`](https://ta-lib.org/functions/cmo) for a smoothed variant of CMOU.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cmou](https://ta-lib.org/functions/cmou).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cmouLookback} is a success with empty outputs.
 *
 * @param real - Source price/value series.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link CmouOptions}.
 * @returns `begIdx` and the values; see {@link CmouResult}. Each output is a view
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
 * import { cmou } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = cmou(data, { timePeriod: 14 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`cmo`](https://ta-lib.org/functions/cmo)
 * @see [`rsi`](https://ta-lib.org/functions/rsi)
 */
export function cmou(real: ArrayLike<number>, options?: CmouOptions): CmouResult {
  const o = optionsOf('CMOU', options);
  if (o.endIdx === undefined) {
    requireSeries('CMOU', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('CMOU', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cmouLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = cmouPositional(startIdx, endIdx, real, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CMOU stream: one value per closed bar, bit-identical to {@link cmou} over
 * the same series. Open it with {@link cmouOpen} or {@link cmouOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cmou](https://ta-lib.org/functions/cmou).
 */
export class CmouStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  nullRun: number = 0;
  /** @internal */
  upSum: number = 0;
  /** @internal */
  downSum: number = 0;
  /** @internal */
  prevValue: number = 0;
  /** @internal */
  trailingValue: number = 0;
  /** @internal */
  ringPos_trailingIdx: number = 0;
  /** @internal */
  ringCap_trailingIdx: number = 0;
  /** @internal */
  ring_trailingIdx_inReal: Float64Array = new Float64Array(1);
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
  static of(core: Core): CmouStream {
    return new CmouStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cmou} reports them over the same bars. Each accepted `update` and each
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
      throw failure('CMOU advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: Source price/value series.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('CMOU update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('CMOU update', 'inReal');
    }
    cmouStepImpl(this, inReal);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Source price/value series.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number): number {
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('CMOU peek', 'inReal');
    }
    const sp = this;
    let live = 0;
    let sum = 0;
    let diff = 0;
    let up = 0;
    let tempReal = 0;
    let cur_outReal = 0;
    let downSum = sp.downSum;
    let nullRun = sp.nullRun;
    let prevValue = sp.prevValue;
    let upSum = sp.upSum;
    /* Add the newest change: inReal[today] - inReal[today-1]. */
    tempReal = inReal;
    diff = tempReal - prevValue;
    prevValue = tempReal;
    up = (diff + Math.abs(diff)) * 0.5;
    upSum += up;
    downSum += up - diff;
    /* Once a whole period of flat bars has gone by, every change in the
     * window is exactly zero, so both sums are known to be exactly zero and
     * the residue can be dropped.
     */
    live = (!(Math.abs(diff) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
    if (nullRun >= sp.optInTimePeriod) {
      nullRun = sp.optInTimePeriod;
      upSum = 0.0;
      downSum = 0.0;
      cur_outReal = 0.0;
    } else {
      sum = upSum + downSum;
      if (sum > 0.0) {
        cur_outReal = 100.0 * (upSum - downSum) / sum;
      } else {
        cur_outReal = 0.0;
      }
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
  clone(): CmouStream {
    const c = new CmouStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.nullRun = this.nullRun;
    c.upSum = this.upSum;
    c.downSum = this.downSum;
    c.prevValue = this.prevValue;
    c.trailingValue = this.trailingValue;
    c.ringPos_trailingIdx = this.ringPos_trailingIdx;
    c.ringCap_trailingIdx = this.ringCap_trailingIdx;
    c.ring_trailingIdx_inReal = this.ring_trailingIdx_inReal.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cmouStepImpl(sp: CmouStream, inReal: number): void {
  let live = 0;
  let sum = 0;
  let diff = 0;
  let up = 0;
  let tempReal = 0;
  let ringCapL_trailingIdx = 0;
  /* Add the newest change: inReal[today] - inReal[today-1]. */
  tempReal = inReal;
  diff = tempReal - sp.prevValue;
  sp.prevValue = tempReal;
  up = (diff + Math.abs(diff)) * 0.5;
  sp.upSum += up;
  sp.downSum += up - diff;
  /* Once a whole period of flat bars has gone by, every change in the
   * window is exactly zero, so both sums are known to be exactly zero and
   * the residue can be dropped.
   */
  live = (!(Math.abs(diff) <= 0.0) ? 1 : 0);
  sp.nullRun = (sp.nullRun + 1) & (live - 1);
  if (sp.nullRun >= sp.optInTimePeriod) {
    sp.nullRun = sp.optInTimePeriod;
    sp.upSum = 0.0;
    sp.downSum = 0.0;
    sp.cur_outReal = 0.0;
  } else {
    sum = sp.upSum + sp.downSum;
    if (sum > 0.0) {
      sp.cur_outReal = 100.0 * (sp.upSum - sp.downSum) / sum;
    } else {
      sp.cur_outReal = 0.0;
    }
  }
  tempReal = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
  diff = tempReal - sp.trailingValue;
  sp.trailingValue = tempReal;
  up = (diff + Math.abs(diff)) * 0.5;
  sp.upSum -= up;
  sp.downSum -= up - diff;
  ringCapL_trailingIdx = sp.ringCap_trailingIdx;
  sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
  sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
  if (sp.ringPos_trailingIdx >= ringCapL_trailingIdx) {
    sp.ringPos_trailingIdx = 0;
  }
}

function cmouOpenImpl(
  sp: CmouStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
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
  let upSum = 0;
  let downSum = 0;
  let sum = 0;
  let diff = 0;
  let up = 0;
  let tempReal = 0;
  let prevValue = 0;
  let trailingValue = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
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
  /* CMOU -- unsmoothed Chande Momentum Oscillator (as in TradingView ta.cmo,
   * pandas-ta default). Over the trailing optInTimePeriod changes
   * d = inReal[i]-inReal[i-1]: Su = sum of up-moves (d>0), Sd = sum of
   * |down-moves| (d<0); CMOU = 100*(Su-Sd)/(Su+Sd), 0 for a flat window. A plain
   * moving-window sum (drop oldest change, add newest), NOT TA_CMO's Wilder
   * smoothing -- hence no unstable period.
   *
   * In-place safe (outReal == inReal): the trailing read inReal[trailingIdx]
   * is of a slot not yet written (trailingIdx >= outIdx), and the oldest
   * change's older endpoint comes from the `trailingValue` cache, not a re-read.
   */
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = cmouLookbackPositional(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Accumulate the up/down sums over the first window: the optInTimePeriod
   * changes ending at startIdx (prices inReal[startIdx-optInTimePeriod ..
   * startIdx]). `trailingValue` caches the oldest price so the window's oldest
   * change can later be dropped by reading only the newer of its two prices.
   * `trailingIdx` points AT that newer price (one past the cached one).
   */
  today = startIdx - lookbackTotal;
  trailingIdx = today + 1;
  prevValue = inReal[today];
  trailingValue = prevValue;
  upSum = 0.0;
  downSum = 0.0;
  /* Consecutive changes of exactly zero, counted so that an empty window can
   * be recognized exactly (the shape #244 needed for MFI). The sums cannot
   * answer that question themselves once the window starts sliding: they are
   * maintained by add-then-subtract, so an emptied window leaves them holding
   * rounding residue of arbitrary sign rather than zero.
   *
   * Both the split and the run are branch-free, as an if/else on the sign or
   * on a zero mispredicts on random or flat-heavy prices. `up` is max(diff, 0)
   * spelled with fabs, which no backend turns back into a branch; adding its
   * +0.0 is exact because neither sum is ever -0.0.
   */
  nullRun = 0;
  for (i = 0; i < optInTimePeriod; i += 1) {
    today += 1;
    tempReal = inReal[today];
    diff = tempReal - prevValue;
    prevValue = tempReal;
    up = (diff + Math.abs(diff)) * 0.5;
    upSum += up;
    downSum += up - diff;
    live = (!(Math.abs(diff) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
  }
  /* Emit the first output (bar startIdx). Su+Sd is a sum of non-negative
   * magnitudes, so it is zero only for an exactly flat window; guard the 0/0
   * exactly and emit 0.0. Not against a fixed band: a price change carries the
   * quote unit, so a constant put against the total zeroed the oscillator for
   * any instrument quoted below it (issue #253).
   *
   * Scale-then-divide -- (100*(Su-Sd))/(Su+Sd), NOT the 100*((Su-Sd)/(Su+Sd))
   * order TA_CMO/RSI use -- so CMOU is BIT-IDENTICAL to the reference unsmoothed
   * CMO of Tulip Indicators (ti_cmo) and pandas-ta-classic (cmo, talib=False),
   * which both scale before dividing. The two orders differ by <=1 ULP.
   */
  outIdx = 0;
  sum = upSum + downSum;
  if (sum > 0.0) {
    outReal[((outIdx++ * outStride) | 0)] = 100.0 * (upSum - downSum) / sum;
  } else {
    outReal[((outIdx++ * outStride) | 0)] = 0.0;
  }
  /* Slide the window forward one bar at a time. Each bar drops the change
   * that leaves the window AFTER its output, ready for the next bar: the
   * sums see the same operations in the same order, and a peek, which keeps
   * nothing past its output, skips that half of the work. The oldest
   * change's older price comes from the cache.
   */
  tempReal = inReal[trailingIdx];
  diff = tempReal - trailingValue;
  trailingValue = tempReal;
  trailingIdx += 1;
  up = (diff + Math.abs(diff)) * 0.5;
  upSum -= up;
  downSum -= up - diff;
  today += 1;
  while (today <= endIdx) {
    /* Add the newest change: inReal[today] - inReal[today-1]. */
    tempReal = inReal[today];
    diff = tempReal - prevValue;
    prevValue = tempReal;
    up = (diff + Math.abs(diff)) * 0.5;
    upSum += up;
    downSum += up - diff;
    /* Once a whole period of flat bars has gone by, every change in the
     * window is exactly zero, so both sums are known to be exactly zero and
     * the residue can be dropped.
     */
    live = (!(Math.abs(diff) <= 0.0) ? 1 : 0);
    nullRun = (nullRun + 1) & (live - 1);
    if (nullRun >= optInTimePeriod) {
      nullRun = optInTimePeriod;
      upSum = 0.0;
      downSum = 0.0;
      outReal[((outIdx++ * outStride) | 0)] = 0.0;
    } else {
      sum = upSum + downSum;
      if (sum > 0.0) {
        outReal[((outIdx++ * outStride) | 0)] = 100.0 * (upSum - downSum) / sum;
      } else {
        outReal[((outIdx++ * outStride) | 0)] = 0.0;
      }
    }
    tempReal = inReal[trailingIdx];
    diff = tempReal - trailingValue;
    trailingValue = tempReal;
    trailingIdx += 1;
    up = (diff + Math.abs(diff)) * 0.5;
    upSum -= up;
    downSum -= up - diff;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  const cap_trailingIdx = today - trailingIdx;
  if (cap_trailingIdx < 1 || cap_trailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_trailingIdx = cap_trailingIdx > 0 ? cap_trailingIdx : 1;
  const capRing_trailingIdx_inReal = new Float64Array(allocN_trailingIdx);
  capRing_trailingIdx_inReal.set(inReal.subarray(historyLen - cap_trailingIdx, historyLen), 0);
  sp.optInTimePeriod = optInTimePeriod;
  sp.nullRun = nullRun;
  sp.upSum = upSum;
  sp.downSum = downSum;
  sp.prevValue = prevValue;
  sp.trailingValue = trailingValue;
  sp.ringPos_trailingIdx = 0;
  sp.ringCap_trailingIdx = cap_trailingIdx;
  sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cmouOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): CmouStream {
  const sp = CmouStream.of(core);
  const retCode = cmouOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CMOU openAndFill', inReal.length, startIdx, cmouLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && cmouLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('CMOU openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('CMOU openAndFill', retCode);
}

/** @internal */
export function cmouOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): CmouStream {
  const sp = CmouStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = cmouOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CMOU open', inReal.length, startIdx, cmouLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && cmouLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('CMOU open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('CMOU open', retCode);
}

/** @internal */
export function cmouOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): CmouStream {
  optInTimePeriod = requireInt32('CMOU open', 'timePeriod', optInTimePeriod);
  requireArgument('CMOU open', 'real', inReal);
  requireHistory('CMOU open', inReal.length);
  return cmouOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/** @internal */
export function cmouOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): CmouStream {
  optInTimePeriod = requireInt32('CMOU openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('CMOU openAndFill', 'real', inReal);
  requireHistory('CMOU openAndFill', inReal.length);
  const guardLookback = cmouLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('CMOU openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('CMOU openAndFill', inReal.length, guardLookback);
  requireOutput('CMOU openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('CMOU openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('CMOU openAndFill', 'outReal', 'real');
  }
  return cmouOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live CMOU stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link cmou} there.
 *
 * @param real - Warm-up history. Source price/value series.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CmouOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cmouLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { cmouOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = cmouOpen(data, { timePeriod: 14 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function cmouOpen(real: ArrayLike<number>, options?: CmouOptions): CmouStream {
  const o = optionsOf('CMOU open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('CMOU open', o.core);
  return cmouOpenPositional(real, optInTimePeriod, core);
}

/**
 * {@link cmouOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link cmou}.
 *
 * @param real - Warm-up history. Source price/value series.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CmouOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cmou} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cmouLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or a
 *   buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cmouOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = cmouOpenAndFill(data, { timePeriod: 14 });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cmouOpenAndFill(real: ArrayLike<number>, options?: CmouOptions): CmouResult & { stream: CmouStream } {
  const o = optionsOf('CMOU openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('CMOU openAndFill', o.core);
  const capacity = fillCapacity(cmouLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = cmouOpenAndFillPositional(real, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
