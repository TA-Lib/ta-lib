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
 *  090426 MF,CC  Initial version (#346).
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function vhfLookbackPositional(optInTimePeriod: number, _core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 28;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  return optInTimePeriod;
}

/** @internal */
export function vhfDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (vhfLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function vhfImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let today = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let i = 0;
  let highest = 0;
  let lowest = 0;
  let sumChange = 0;
  let prev = 0;
  let tempReal = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 28;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = vhfLookbackPositional(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  outIdx = 0;
  today = startIdx;
  while (today <= endIdx) {
    /* The two windows are NOT co-terminal: the extrema span the
     * optInTimePeriod newest closes, while the optInTimePeriod changes reach
     * one bar further back. That is what makes the lookback optInTimePeriod
     * rather than optInTimePeriod-1.
     */
    highest = inReal[today];
    lowest = highest;
    prev = highest;
    sumChange = 0.0;
    for (i = optInTimePeriod; i >= 0; i -= 1) {
      tempReal = inReal[today - i];
      if (i < optInTimePeriod) {
        sumChange += Math.abs(tempReal - prev);
        if (tempReal > highest) {
          highest = tempReal;
        }
        if (tempReal < lowest) {
          lowest = tempReal;
        }
      }
      prev = tempReal;
    }
    /* A fresh sum of non-negative magnitudes is exactly zero only on an
     * exactly flat window, which forces highest-lowest to zero too. Guard on
     * exact zero, never an epsilon band: a band carries the quote unit and
     * zeroes the filter for anything priced under it (issue #253).
     */
    if (sumChange > 0.0) {
      outReal[outIdx] = (highest - lowest) / sumChange;
    } else {
      outReal[outIdx] = 0.0;
    }
    outIdx += 1;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000']] as const;

/** @internal */
export function vhfPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('VHF', 'startIdx', startIdx);
  endIdx = requireInt32('VHF', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('VHF', 'timePeriod', optInTimePeriod);
  requireIndexRange('VHF', startIdx, endIdx);
  const guardLookback = vhfLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('VHF', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('VHF', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('VHF', 'real', inReal, guardInLen);
  requireOutput('VHF', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('VHF', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = vhfImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('VHF', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link vhf}, {@link vhfLookback}, {@link vhfDisplayShift},
 * {@link vhfOpen} and {@link vhfOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface VhfOptions {
  /**
   * Number of trailing closes spanned by the range (default 28; range 2 to 100000;
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

/** What {@link vhf} returns, and {@link vhfOpenAndFill} with its stream. */
export interface VhfResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Vertical Horizontal Filter value. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link vhf} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link VhfOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function vhfLookback(options?: VhfOptions): number {
  const o = optionsOf('VHF lookback', options);
  return vhfLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('VHF lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link vhf}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link VhfOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function vhfDisplayShift(outputIdx: number, options?: VhfOptions): number {
  const o = optionsOf('VHF displayShift', options);
  return vhfDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('VHF displayShift', o.core));
}

/**
 * Vertical Horizontal Filter: Adam White's trend-versus-range filter, the range a
 * window covered divided by the path it actually travelled. Bounded in \[0,1].
 * Values near 1 mean the market covered most of its path in one direction
 * (trending); values near 0 mean it retraced repeatedly and went nowhere (choppy).
 * Like ADX it measures trend *strength*, not direction, but it uses no smoothing
 * and carries no recursion. A common use is regime selection: run trend-following
 * logic while VHF is high, oscillator logic while it is low.
 *
 * Formula and more info at
 * [ta-lib.org/functions/vhf](https://ta-lib.org/functions/vhf).
 *
 * **Notes**
 * - A window whose closes are all identical has no vertical movement and no
 *   horizontal movement. VHF reports 0 there. Other libraries differ: Tulip
 *   Indicators leaves the division unguarded and emits NaN, pandas-ta-classic
 *   perturbs the numerator and emits +Inf.
 * - Adam White later described an 18-bar VHF smoothed by a 6-bar moving average.
 *   That variant is not implemented here; apply a moving average to `outReal` to
 *   obtain it.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link vhfLookback} is a success with empty outputs.
 *
 * @param real - Source price/value series, canonically the close.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link VhfOptions}.
 * @returns `begIdx` and the values; see {@link VhfResult}. Each output is a view
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
 * import { vhf } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = vhf(data, { timePeriod: 28 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`adx`](https://ta-lib.org/functions/adx)
 * @see [`cmo`](https://ta-lib.org/functions/cmo)
 * @see [`cmou`](https://ta-lib.org/functions/cmou)
 */
export function vhf(real: ArrayLike<number>, options?: VhfOptions): VhfResult {
  const o = optionsOf('VHF', options);
  if (o.endIdx === undefined) {
    requireSeries('VHF', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('VHF', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, vhfLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = vhfPositional(startIdx, endIdx, real, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live VHF stream: one value per closed bar, bit-identical to {@link vhf} over
 * the same series. Open it with {@link vhfOpen} or {@link vhfOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/vhf](https://ta-lib.org/functions/vhf).
 */
export class VhfStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  winPos_i: number = 0;
  /** @internal */
  winCap_i: number = 1;
  /** @internal */
  win_i_inReal: Float64Array = new Float64Array(1);
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
  static of(core: Core): VhfStream {
    return new VhfStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link vhf} reports them over the same bars. Each accepted `update` and each
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
      throw failure('VHF advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: Source price/value series, canonically
   *   the close.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('VHF update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('VHF update', 'inReal');
    }
    vhfStepImpl(this, inReal);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Source price/value series, canonically
   *   the close.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number): number {
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('VHF peek', 'inReal');
    }
    const sp = this;
    let i = 0;
    let highest = 0;
    let lowest = 0;
    let sumChange = 0;
    let prev = 0;
    let tempReal = 0;
    let cur_outReal = 0;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    pkSlot0 = sp.winPos_i;
    pkVal0 = inReal;
    /* The two windows are NOT co-terminal: the extrema span the
     * optInTimePeriod newest closes, while the optInTimePeriod changes reach
     * one bar further back. That is what makes the lookback optInTimePeriod
     * rather than optInTimePeriod-1.
     */
    highest = inReal;
    lowest = highest;
    prev = highest;
    sumChange = 0.0;
    for (i = sp.optInTimePeriod; i >= 0; i -= 1) {
      tempReal = (((sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i) !== pkSlot0) ? sp.win_i_inReal[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i] : pkVal0;
      if (i < sp.optInTimePeriod) {
        sumChange += Math.abs(tempReal - prev);
        if (tempReal > highest) {
          highest = tempReal;
        }
        if (tempReal < lowest) {
          lowest = tempReal;
        }
      }
      prev = tempReal;
    }
    /* A fresh sum of non-negative magnitudes is exactly zero only on an
     * exactly flat window, which forces highest-lowest to zero too. Guard on
     * exact zero, never an epsilon band: a band carries the quote unit and
     * zeroes the filter for anything priced under it (issue #253).
     */
    if (sumChange > 0.0) {
      cur_outReal = (highest - lowest) / sumChange;
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
  clone(): VhfStream {
    const c = new VhfStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.winPos_i = this.winPos_i;
    c.winCap_i = this.winCap_i;
    c.win_i_inReal = this.win_i_inReal.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function vhfStepImpl(sp: VhfStream, inReal: number): void {
  let i = 0;
  let highest = 0;
  let lowest = 0;
  let sumChange = 0;
  let prev = 0;
  let tempReal = 0;
  sp.win_i_inReal[sp.winPos_i] = inReal;
  /* The two windows are NOT co-terminal: the extrema span the
   * optInTimePeriod newest closes, while the optInTimePeriod changes reach
   * one bar further back. That is what makes the lookback optInTimePeriod
   * rather than optInTimePeriod-1.
   */
  highest = inReal;
  lowest = highest;
  prev = highest;
  sumChange = 0.0;
  for (i = sp.optInTimePeriod; i >= 0; i -= 1) {
    tempReal = sp.win_i_inReal[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i];
    if (i < sp.optInTimePeriod) {
      sumChange += Math.abs(tempReal - prev);
      if (tempReal > highest) {
        highest = tempReal;
      }
      if (tempReal < lowest) {
        lowest = tempReal;
      }
    }
    prev = tempReal;
  }
  /* A fresh sum of non-negative magnitudes is exactly zero only on an
   * exactly flat window, which forces highest-lowest to zero too. Guard on
   * exact zero, never an epsilon band: a band carries the quote unit and
   * zeroes the filter for anything priced under it (issue #253).
   */
  if (sumChange > 0.0) {
    sp.cur_outReal = (highest - lowest) / sumChange;
  } else {
    sp.cur_outReal = 0.0;
  }
  sp.winPos_i = sp.winPos_i + 1;
  if (sp.winPos_i >= sp.winCap_i) {
    sp.winPos_i = 0;
  }
}

function vhfOpenImpl(
  sp: VhfStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let today = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let i = 0;
  let highest = 0;
  let lowest = 0;
  let sumChange = 0;
  let prev = 0;
  let tempReal = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 28;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = vhfLookbackPositional(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outIdx = 0;
  today = startIdx;
  while (today <= endIdx) {
    /* The two windows are NOT co-terminal: the extrema span the
     * optInTimePeriod newest closes, while the optInTimePeriod changes reach
     * one bar further back. That is what makes the lookback optInTimePeriod
     * rather than optInTimePeriod-1.
     */
    highest = inReal[today];
    lowest = highest;
    prev = highest;
    sumChange = 0.0;
    for (i = optInTimePeriod; i >= 0; i -= 1) {
      tempReal = inReal[today - i];
      if (i < optInTimePeriod) {
        sumChange += Math.abs(tempReal - prev);
        if (tempReal > highest) {
          highest = tempReal;
        }
        if (tempReal < lowest) {
          lowest = tempReal;
        }
      }
      prev = tempReal;
    }
    /* A fresh sum of non-negative magnitudes is exactly zero only on an
     * exactly flat window, which forces highest-lowest to zero too. Guard on
     * exact zero, never an epsilon band: a band carries the quote unit and
     * zeroes the filter for anything priced under it (issue #253).
     */
    if (sumChange > 0.0) {
      outReal[((outIdx * outStride) | 0)] = (highest - lowest) / sumChange;
    } else {
      outReal[((outIdx * outStride) | 0)] = 0.0;
    }
    outIdx += 1;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  const cap_i = optInTimePeriod + 1;
  if (cap_i < 1 || cap_i > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const capWin_i_inReal = inReal.slice(historyLen - cap_i, historyLen);
  sp.optInTimePeriod = optInTimePeriod;
  sp.winPos_i = 0;
  sp.winCap_i = cap_i;
  sp.win_i_inReal = capWin_i_inReal;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function vhfOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): VhfStream {
  const sp = VhfStream.of(core);
  const retCode = vhfOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('VHF openAndFill', inReal.length, startIdx, vhfLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && vhfLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('VHF openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('VHF openAndFill', retCode);
}

/** @internal */
export function vhfOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): VhfStream {
  const sp = VhfStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = vhfOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('VHF open', inReal.length, startIdx, vhfLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && vhfLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('VHF open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('VHF open', retCode);
}

/** @internal */
export function vhfOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): VhfStream {
  optInTimePeriod = requireInt32('VHF open', 'timePeriod', optInTimePeriod);
  requireArgument('VHF open', 'real', inReal);
  requireHistory('VHF open', inReal.length);
  return vhfOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/** @internal */
export function vhfOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): VhfStream {
  optInTimePeriod = requireInt32('VHF openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('VHF openAndFill', 'real', inReal);
  requireHistory('VHF openAndFill', inReal.length);
  const guardLookback = vhfLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('VHF openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('VHF openAndFill', inReal.length, guardLookback);
  requireOutput('VHF openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('VHF openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('VHF openAndFill', 'outReal', 'real');
  }
  return vhfOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live VHF stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link vhf} there.
 *
 * @param real - Warm-up history. Source price/value series, canonically the close.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link VhfOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `vhfLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { vhfOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = vhfOpen(data, { timePeriod: 28 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function vhfOpen(real: ArrayLike<number>, options?: VhfOptions): VhfStream {
  const o = optionsOf('VHF open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('VHF open', o.core);
  return vhfOpenPositional(real, optInTimePeriod, core);
}

/**
 * {@link vhfOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link vhf}.
 *
 * @param real - Warm-up history. Source price/value series, canonically the close.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link VhfOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link vhf} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `vhfLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or a
 *   buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { vhfOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = vhfOpenAndFill(data, { timePeriod: 28 });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function vhfOpenAndFill(real: ArrayLike<number>, options?: VhfOptions): VhfResult & { stream: VhfStream } {
  const o = optionsOf('VHF openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('VHF openAndFill', o.core);
  const capacity = fillCapacity(vhfLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = vhfOpenAndFillPositional(real, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
