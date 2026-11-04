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

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link vhf} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Number of trailing closes spanned by the range (default
 *   28; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function vhfLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  void core;
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

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link vhf}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Number of trailing closes spanned by the range (default
 *   28; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function vhfDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (vhfLookback(optInTimePeriod, core) < 0) {
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
  lookbackTotal = vhfLookback(optInTimePeriod, core);
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
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link vhfLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - Source price/value series, canonically the close.
 * @param optInTimePeriod - Number of trailing closes spanned by the range (default
 *   28; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Vertical Horizontal Filter value. Must hold the values produced
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
 * import { vhf } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const { begIdx, count } = vhf(0, data.length - 1, data, 28, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`adx`](https://ta-lib.org/functions/adx)
 * @see [`cmo`](https://ta-lib.org/functions/cmo)
 * @see [`cmou`](https://ta-lib.org/functions/cmou)
 */
export function vhf(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('VHF', 'startIdx', startIdx);
  endIdx = requireInt32('VHF', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('VHF', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('VHF', startIdx, endIdx);
  const guardStart = clampedStart('VHF', startIdx, vhfLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('VHF', 'inReal', inReal, guardInLen);
  requireOutput('VHF', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw failure('VHF', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = vhfImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('VHF', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
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
  lookbackTotal = vhfLookback(optInTimePeriod, core);
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
    throw insufficientHistory('VHF openAndFill', inReal.length, startIdx, vhfLookback(optInTimePeriod, core));
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
    throw insufficientHistory('VHF open', inReal.length, startIdx, vhfLookback(optInTimePeriod, core));
  }
  throw streamFailure('VHF open', retCode);
}

/**
 * Opens a live VHF stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link vhf} there.
 *
 * @param inReal - Warm-up history. Source price/value series, canonically the
 *   close.
 * @param optInTimePeriod - Number of trailing closes spanned by the range (default
 *   28; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `vhfLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { vhfOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = vhfOpen(data, 28);
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function vhfOpen(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): VhfStream {
  optInTimePeriod = requireInt32('VHF open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('VHF open', 'inReal', inReal);
  requireHistory('VHF open', inReal.length);
  return vhfOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/**
 * {@link vhfOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link vhf}. The range written is the stream's
 * `outRange()`.
 *
 * @param inReal - Warm-up history. Source price/value series, canonically the
 *   close.
 * @param optInTimePeriod - Number of trailing closes spanned by the range (default
 *   28; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Vertical Horizontal Filter value. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `vhfLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   an integer parameter that is not an int32, a parameter outside its range or an
 *   output too short or starting where another output or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { vhfOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const stream = vhfOpenAndFill(data, 28, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function vhfOpenAndFill(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): VhfStream {
  optInTimePeriod = requireInt32('VHF openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('VHF openAndFill', 'inReal', inReal);
  requireHistory('VHF openAndFill', inReal.length);
  const guardOutLen = openFillCount('VHF openAndFill', inReal.length, vhfLookback(optInTimePeriod, core));
  requireOutput('VHF openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if ((outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset)) {
    throw streamFailure('VHF openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inRealF64))) {
    throw streamFailure('VHF openAndFill', RetCode.BAD_PARAM);
  }
  return vhfOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}
