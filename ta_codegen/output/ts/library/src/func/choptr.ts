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
 *  092926 MF,CC  Initial version (#469).
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link choptr} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Number of bars in the window (default 14; range 2 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function choptrLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  return optInTimePeriod;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link choptr}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Number of bars in the window (default 14; range 2 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function choptrDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (choptrLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function choptrImpl(
  core: Core,
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
  let today = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let i = 0;
  let highest = 0;
  let lowest = 0;
  let sumTR = 0;
  let logPeriod = 0;
  let tempHT = 0;
  let tempLT = 0;
  let prevClose = 0;
  let trueHigh = 0;
  let trueLow = 0;
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
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = choptrLookback(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  logPeriod = Math.log10(optInTimePeriod);
  outIdx = 0;
  today = startIdx;
  while (today <= endIdx) {
    /* Re-summed oldest to newest every bar, never a running total: a
     * running total leaves a residue on a window of zero true ranges, which
     * the exact guard below would then read as a trend.
     */
    highest = inHigh[today];
    lowest = inLow[today];
    sumTR = 0.0;
    prevClose = 0.0;
    for (i = optInTimePeriod; i >= 0; i -= 1) {
      if (i < optInTimePeriod) {
        tempHT = inHigh[today - i];
        tempLT = inLow[today - i];
        trueHigh = tempHT;
        if (prevClose > trueHigh) {
          trueHigh = prevClose;
        }
        trueLow = tempLT;
        if (prevClose < trueLow) {
          trueLow = prevClose;
        }
        sumTR += trueHigh - trueLow;
        if (trueHigh > highest) {
          highest = trueHigh;
        }
        if (trueLow < lowest) {
          lowest = trueLow;
        }
      }
      prevClose = inClose[today - i];
    }
    /* The seeds are the raw extrema of today's bar, which its own true
     * high and low always enclose, so they never outrun the true box.
     *
     * Exact test, never an epsilon band (issue #253). Every true range lies
     * inside the box, so a positive sum implies a positive box. Keep the
     * "<=" form: a NaN sum must fall through to the log, not read as 100.
     * Keep the parenthesised quotient: it returns exactly 100 whenever the
     * computed ratio is exactly the period.
     */
    if (sumTR <= 0.0) {
      outReal[outIdx] = 100.0;
    } else {
      outReal[outIdx] = 100.0 * (Math.log10(sumTR / (highest - lowest)) / logPeriod);
    }
    outIdx += 1;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * Choppiness Index with a true-range box: the true range travelled over a window
 * against the span from its highest true high to its lowest true low. Bill
 * Dreiss's form as published in 1993. Log-scaled so that a straight run reads 0
 * and bars that each fill the whole box read 100. Measures whether the market is
 * trending, not in which direction: low values mean a directional run, high values
 * sideways chop.
 *
 * Formula and more info at
 * [ta-lib.org/functions/choptr](https://ta-lib.org/functions/choptr).
 *
 * **Notes**
 * - The box reaches the close just before each bar. Most charting platforms
 *   instead use the window's highest high minus its lowest low; that form is CHOP.
 *   The two differ only on bars where the close before the window lies outside the
 *   window's high-low range: a gap into the window.
 * - With every close inside its own bar's high-low range, the value stays within 0
 *   to 100, up to rounding.
 * - A window with no true range reports 100.
 * - Dreiss's 3-bar smoothing of the index is not built in; apply a moving average
 *   to `outReal` to obtain it.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link choptrLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param optInTimePeriod - Number of bars in the window (default 14; range 2 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Choppiness Index value. Must hold the values produced and never
 *   be empty; the input length always suffices.
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
 * import { choptr } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const out = new Float64Array(high.length);
 * const { begIdx, count } = choptr(0, high.length - 1, high, low, close, 14, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`chop`](https://ta-lib.org/functions/chop)
 * @see [`vhf`](https://ta-lib.org/functions/vhf)
 * @see [`adx`](https://ta-lib.org/functions/adx)
 * @see [`trange`](https://ta-lib.org/functions/trange)
 */
export function choptr(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CHOPTR', 'startIdx', startIdx);
  endIdx = requireInt32('CHOPTR', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('CHOPTR', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('CHOPTR', startIdx, endIdx);
  const guardStart = clampedStart('CHOPTR', startIdx, choptrLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CHOPTR', 'inHigh', inHigh, guardInLen);
  requireLength('CHOPTR', 'inLow', inLow, guardInLen);
  requireLength('CHOPTR', 'inClose', inClose, guardInLen);
  requireOutput('CHOPTR', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outReal, inHighF64) || overlapsPartly(outReal, inLowF64) || overlapsPartly(outReal, inCloseF64)) {
    throw failure('CHOPTR', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = choptrImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CHOPTR', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live CHOPTR stream: one value per closed bar, bit-identical to {@link choptr}
 * over the same series. Open it with {@link choptrOpen} or
 * {@link choptrOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/choptr](https://ta-lib.org/functions/choptr).
 */
export class ChoptrStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  logPeriod: number = 0;
  /** @internal */
  winPos_i: number = 0;
  /** @internal */
  winCap_i: number = 1;
  /** @internal */
  win_i_inHigh: Float64Array = new Float64Array(1);
  /** @internal */
  win_i_inLow: Float64Array = new Float64Array(1);
  /** @internal */
  win_i_inClose: Float64Array = new Float64Array(1);
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
  static of(core: Core): ChoptrStream {
    return new ChoptrStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link choptr} reports them over the same bars. Each accepted `update` and each
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
      throw failure('CHOPTR advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw failure('CHOPTR update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('CHOPTR update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    choptrStepImpl(this, inHigh, inLow, inClose);
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
      throw nonFiniteBar('CHOPTR peek', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let i = 0;
    let highest = 0;
    let lowest = 0;
    let sumTR = 0;
    let tempHT = 0;
    let tempLT = 0;
    let prevClose = 0;
    let trueHigh = 0;
    let trueLow = 0;
    let cur_outReal = 0;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    let pkSlot1 = -1;
    let pkVal1 = 0;
    let pkSlot2 = -1;
    let pkVal2 = 0;
    pkSlot0 = sp.winPos_i;
    pkVal0 = inHigh;
    pkSlot1 = sp.winPos_i;
    pkVal1 = inLow;
    pkSlot2 = sp.winPos_i;
    pkVal2 = inClose;
    /* Re-summed oldest to newest every bar, never a running total: a
     * running total leaves a residue on a window of zero true ranges, which
     * the exact guard below would then read as a trend.
     */
    highest = inHigh;
    lowest = inLow;
    sumTR = 0.0;
    prevClose = 0.0;
    for (i = sp.optInTimePeriod; i >= 0; i -= 1) {
      if (i < sp.optInTimePeriod) {
        tempHT = (((sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i) !== pkSlot0) ? sp.win_i_inHigh[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i] : pkVal0;
        tempLT = (((sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i) !== pkSlot1) ? sp.win_i_inLow[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i] : pkVal1;
        trueHigh = tempHT;
        if (prevClose > trueHigh) {
          trueHigh = prevClose;
        }
        trueLow = tempLT;
        if (prevClose < trueLow) {
          trueLow = prevClose;
        }
        sumTR += trueHigh - trueLow;
        if (trueHigh > highest) {
          highest = trueHigh;
        }
        if (trueLow < lowest) {
          lowest = trueLow;
        }
      }
      prevClose = (((sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i) !== pkSlot2) ? sp.win_i_inClose[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i] : pkVal2;
    }
    /* The seeds are the raw extrema of today's bar, which its own true
     * high and low always enclose, so they never outrun the true box.
     *
     * Exact test, never an epsilon band (issue #253). Every true range lies
     * inside the box, so a positive sum implies a positive box. Keep the
     * "<=" form: a NaN sum must fall through to the log, not read as 100.
     * Keep the parenthesised quotient: it returns exactly 100 whenever the
     * computed ratio is exactly the period.
     */
    if (sumTR <= 0.0) {
      cur_outReal = 100.0;
    } else {
      cur_outReal = 100.0 * (Math.log10(sumTR / (highest - lowest)) / sp.logPeriod);
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
  clone(): ChoptrStream {
    const c = new ChoptrStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.logPeriod = this.logPeriod;
    c.winPos_i = this.winPos_i;
    c.winCap_i = this.winCap_i;
    c.win_i_inHigh = this.win_i_inHigh.slice();
    c.win_i_inLow = this.win_i_inLow.slice();
    c.win_i_inClose = this.win_i_inClose.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function choptrStepImpl(sp: ChoptrStream, inHigh: number, inLow: number, inClose: number): void {
  let i = 0;
  let highest = 0;
  let lowest = 0;
  let sumTR = 0;
  let tempHT = 0;
  let tempLT = 0;
  let prevClose = 0;
  let trueHigh = 0;
  let trueLow = 0;
  sp.win_i_inHigh[sp.winPos_i] = inHigh;
  sp.win_i_inLow[sp.winPos_i] = inLow;
  sp.win_i_inClose[sp.winPos_i] = inClose;
  /* Re-summed oldest to newest every bar, never a running total: a
   * running total leaves a residue on a window of zero true ranges, which
   * the exact guard below would then read as a trend.
   */
  highest = inHigh;
  lowest = inLow;
  sumTR = 0.0;
  prevClose = 0.0;
  for (i = sp.optInTimePeriod; i >= 0; i -= 1) {
    if (i < sp.optInTimePeriod) {
      tempHT = sp.win_i_inHigh[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i];
      tempLT = sp.win_i_inLow[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i];
      trueHigh = tempHT;
      if (prevClose > trueHigh) {
        trueHigh = prevClose;
      }
      trueLow = tempLT;
      if (prevClose < trueLow) {
        trueLow = prevClose;
      }
      sumTR += trueHigh - trueLow;
      if (trueHigh > highest) {
        highest = trueHigh;
      }
      if (trueLow < lowest) {
        lowest = trueLow;
      }
    }
    prevClose = sp.win_i_inClose[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i];
  }
  /* The seeds are the raw extrema of today's bar, which its own true
   * high and low always enclose, so they never outrun the true box.
   *
   * Exact test, never an epsilon band (issue #253). Every true range lies
   * inside the box, so a positive sum implies a positive box. Keep the
   * "<=" form: a NaN sum must fall through to the log, not read as 100.
   * Keep the parenthesised quotient: it returns exactly 100 whenever the
   * computed ratio is exactly the period.
   */
  if (sumTR <= 0.0) {
    sp.cur_outReal = 100.0;
  } else {
    sp.cur_outReal = 100.0 * (Math.log10(sumTR / (highest - lowest)) / sp.logPeriod);
  }
  sp.winPos_i = sp.winPos_i + 1;
  if (sp.winPos_i >= sp.winCap_i) {
    sp.winPos_i = 0;
  }
}

function choptrOpenImpl(
  sp: ChoptrStream,
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
  const core = sp.core;
  let today = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let i = 0;
  let highest = 0;
  let lowest = 0;
  let sumTR = 0;
  let logPeriod = 0;
  let tempHT = 0;
  let tempLT = 0;
  let prevClose = 0;
  let trueHigh = 0;
  let trueLow = 0;
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
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = choptrLookback(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  logPeriod = Math.log10(optInTimePeriod);
  outIdx = 0;
  today = startIdx;
  while (today <= endIdx) {
    /* Re-summed oldest to newest every bar, never a running total: a
     * running total leaves a residue on a window of zero true ranges, which
     * the exact guard below would then read as a trend.
     */
    highest = inHigh[today];
    lowest = inLow[today];
    sumTR = 0.0;
    prevClose = 0.0;
    for (i = optInTimePeriod; i >= 0; i -= 1) {
      if (i < optInTimePeriod) {
        tempHT = inHigh[today - i];
        tempLT = inLow[today - i];
        trueHigh = tempHT;
        if (prevClose > trueHigh) {
          trueHigh = prevClose;
        }
        trueLow = tempLT;
        if (prevClose < trueLow) {
          trueLow = prevClose;
        }
        sumTR += trueHigh - trueLow;
        if (trueHigh > highest) {
          highest = trueHigh;
        }
        if (trueLow < lowest) {
          lowest = trueLow;
        }
      }
      prevClose = inClose[today - i];
    }
    /* The seeds are the raw extrema of today's bar, which its own true
     * high and low always enclose, so they never outrun the true box.
     *
     * Exact test, never an epsilon band (issue #253). Every true range lies
     * inside the box, so a positive sum implies a positive box. Keep the
     * "<=" form: a NaN sum must fall through to the log, not read as 100.
     * Keep the parenthesised quotient: it returns exactly 100 whenever the
     * computed ratio is exactly the period.
     */
    if (sumTR <= 0.0) {
      outReal[((outIdx * outStride) | 0)] = 100.0;
    } else {
      outReal[((outIdx * outStride) | 0)] = 100.0 * (Math.log10(sumTR / (highest - lowest)) / logPeriod);
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
  const capWin_i_inHigh = inHigh.slice(historyLen - cap_i, historyLen);
  const capWin_i_inLow = inLow.slice(historyLen - cap_i, historyLen);
  const capWin_i_inClose = inClose.slice(historyLen - cap_i, historyLen);
  sp.optInTimePeriod = optInTimePeriod;
  sp.logPeriod = logPeriod;
  sp.winPos_i = 0;
  sp.winCap_i = cap_i;
  sp.win_i_inHigh = capWin_i_inHigh;
  sp.win_i_inLow = capWin_i_inLow;
  sp.win_i_inClose = capWin_i_inClose;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function choptrOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): ChoptrStream {
  const sp = ChoptrStream.of(core);
  const retCode = choptrOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CHOPTR openAndFill', inHigh.length, startIdx, choptrLookback(optInTimePeriod, core));
  }
  throw streamFailure('CHOPTR openAndFill', retCode);
}

/** @internal */
export function choptrOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): ChoptrStream {
  const sp = ChoptrStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = choptrOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CHOPTR open', inHigh.length, startIdx, choptrLookback(optInTimePeriod, core));
  }
  throw streamFailure('CHOPTR open', retCode);
}

/**
 * Opens a live CHOPTR stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link choptr} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Number of bars in the window (default 14; range 2 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `choptrLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { choptrOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = choptrOpen(high, low, close, 14);
 * const peeked = stream.peek(101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function choptrOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): ChoptrStream {
  optInTimePeriod = requireInt32('CHOPTR open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('CHOPTR open', 'inHigh', inHigh);
  requireHistory('CHOPTR open', inHigh.length);
  requireArgument('CHOPTR open', 'inLow', inLow);
  requireArgument('CHOPTR open', 'inClose', inClose);
  requireHistoryLength('CHOPTR open', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('CHOPTR open', 'inClose', inClose.length, inHigh.length);
  return choptrOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInTimePeriod);
}

/**
 * {@link choptrOpen} that also fills the outputs over the whole history in the
 * same pass, bit-identically to {@link choptr}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Number of bars in the window (default 14; range 2 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Choppiness Index value. Must hold `historyLen - lookback`
 *   values and share no memory with an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `choptrLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range, histories of different lengths or an output too
 *   short or starting where another output or a `Float64Array` input starts, or, on
 *   a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { choptrOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const out = new Float64Array(high.length);
 * const stream = choptrOpenAndFill(high, low, close, 14, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function choptrOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): ChoptrStream {
  optInTimePeriod = requireInt32('CHOPTR openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('CHOPTR openAndFill', 'inHigh', inHigh);
  requireHistory('CHOPTR openAndFill', inHigh.length);
  requireArgument('CHOPTR openAndFill', 'inLow', inLow);
  requireArgument('CHOPTR openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('CHOPTR openAndFill', inHigh.length, choptrLookback(optInTimePeriod, core));
  requireHistoryLength('CHOPTR openAndFill', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('CHOPTR openAndFill', 'inClose', inClose.length, inHigh.length);
  requireOutput('CHOPTR openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) || (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) || (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset)) {
    throw streamFailure('CHOPTR openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inHighF64) || overlaps(outReal, inLowF64) || overlaps(outReal, inCloseF64))) {
    throw streamFailure('CHOPTR openAndFill', RetCode.BAD_PARAM);
  }
  return choptrOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}
