/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AC       Angelo Ciceri
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  120305 AC     Creation
 *  071226 MF,CC  Streaming-friendly rewrite: carry the confirmation state
 *                (countdown + cached 2nd-candle high/low) instead of the absolute
 *                bar index, so the per-bar logic reads no cursor. Bit-identical
 *                batch results (verified vs v0.6.4).
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link cdlhikkake} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function cdlhikkakeLookback(core: Core = Core.DEFAULT): number {
  void core;
  return 5;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdlhikkake}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function cdlhikkakeDisplayShift(outputIdx: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdlhikkakeImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  _inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): RetCode {
  let i = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let patternResult = 0;
  let cd = 0;
  let savedHigh = 0;
  let savedLow = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  savedHigh = 0.0;
  savedLow = 0.0;
  /* Confirmation-window countdown + cached 2nd-candle high/low: the pattern
   * state carried without an absolute bar index.
   */
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlhikkakeLookback(core);
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
  /* Do the calculation using tight loops. */
  /* Add-up the initial period, except for the last value. */
  cd = 0;
  patternResult = 0;
  i = startIdx - 3;
  while (i < startIdx) {
    /* copy here the pattern recognition code below */
    if (inHigh[i - 1] < inHigh[i - 2] &&
        inLow[i - 1] > inLow[i - 2] &&   /* 1st + 2nd: lower high and higher low */
        (inHigh[i] < inHigh[i - 1] &&
          inLow[i] < inLow[i - 1] ||     /* (bull) 3rd: lower high and lower low */
         inHigh[i] > inHigh[i - 1] &&
          inLow[i] > inLow[i - 1]))      /* (bear) 3rd: higher high and higher low */
    {
      patternResult = ((100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : -1)) | 0);
      savedHigh = inHigh[i - 1];
      savedLow = inLow[i - 1];
      cd = 4;
    } else if (cd > 0 &&
        (patternResult > 0 &&       /* search for confirmation if hikkake was no more than 3 bars ago */
          inClose[i] > savedHigh || /* close higher than the high of 2nd */
         patternResult < 0 &&
          inClose[i] < savedLow))   /* close lower than the low of 2nd */
    {
      cd = 0;
    }
    if (cd > 0) {
      cd -= 1;
    }
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first and second candle: inside bar (2nd has lower high and higher low than 1st)
   * - third candle: lower high and lower low than 2nd (higher high and higher low than 2nd)
   * Confirmation could come in the next 3 days with:
   * - a day that closes higher than the high (lower than the low) of the 2nd candle
   */
  outIdx = 0;
  do {
    if (inHigh[i - 1] < inHigh[i - 2] &&
        inLow[i - 1] > inLow[i - 2] &&   /* 1st + 2nd: lower high and higher low */
        (inHigh[i] < inHigh[i - 1] &&
          inLow[i] < inLow[i - 1] ||     /* (bull) 3rd: lower high and lower low */
         inHigh[i] > inHigh[i - 1] &&
          inLow[i] > inLow[i - 1]))      /* (bear) 3rd: higher high and higher low */
    {
      patternResult = ((100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : -1)) | 0);
      savedHigh = inHigh[i - 1];
      savedLow = inLow[i - 1];
      cd = 4;
      outInteger[outIdx++] = patternResult;
    } else if (cd > 0 &&
        (patternResult > 0 &&       /* search for confirmation if hikkake was no more than 3 bars ago */
          inClose[i] > savedHigh || /* close higher than the high of 2nd */
         patternResult < 0 &&
          inClose[i] < savedLow))   /* close lower than the low of 2nd */
    {
      outInteger[outIdx++] = patternResult + ((100 * ((patternResult > 0) ? 1 : -1)) | 0);
      cd = 0;
    } else {
      outInteger[outIdx++] = 0;
    }
    if (cd > 0) {
      cd -= 1;
    }
    i += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/**
 * A 3-bar pattern: an inside bar followed by a false breakout, optionally later
 * confirmed by a follow-through bar. Signals a bullish or bearish
 * reversal/continuation depending on the breakout direction. A false-breakout
 * setup: positive = bullish, negative = bearish; magnitude 200 flags the
 * confirming bar.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlhikkake](https://ta-lib.org/functions/cdlhikkake).
 *
 * **Notes**
 * - The name comes from the Japanese word for a deceptive move or "trap" —
 *   fitting, since the pattern exists to catch traders acting on a false breakout.
 *   Bulkowski's testing of the confirmed pattern found the trap itself barely
 *   beats a coin flip: the bullish variant continues as expected only 52% of the
 *   time and the bearish variant exactly 50% ("random"), both ranking in the
 *   bottom fifth (83rd-84th of 105) for post-breakout performance.
 *   ([thepatternsite.com](https://thepatternsite.com/HikkakeBull.html))
 * - A bar that is both a new setup and the confirmation of an earlier one reports
 *   the new setup; the earlier one ends unconfirmed.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link cdlhikkakeLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inOpen - Open price of each bar.
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param outInteger - +100/-100 at the hikkake (breakout) bar for bull/bear;
 *   +200/-200 at a later confirmation bar; 0 otherwise. Must hold the values
 *   produced and never be empty; the input length always suffices.
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
 * import { cdlhikkake } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const out = new Int32Array(open.length);
 * const { begIdx, count } = cdlhikkake(
 *   0, open.length - 1,
 *   open, high, low, close,
 *   out,
 * );
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`cdlhikkakemod`](https://ta-lib.org/functions/cdlhikkakemod)
 * @see [`cdlharami`](https://ta-lib.org/functions/cdlharami)
 */
export function cdlhikkake(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDLHIKKAKE', 'startIdx', startIdx);
  endIdx = requireInt32('CDLHIKKAKE', 'endIdx', endIdx);
  requireIndexRange('CDLHIKKAKE', startIdx, endIdx);
  const guardStart = clampedStart('CDLHIKKAKE', startIdx, cdlhikkakeLookback(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLHIKKAKE', 'inOpen', inOpen, guardInLen);
  requireLength('CDLHIKKAKE', 'inHigh', inHigh, guardInLen);
  requireLength('CDLHIKKAKE', 'inLow', inLow, guardInLen);
  requireLength('CDLHIKKAKE', 'inClose', inClose, guardInLen);
  requireOutput('CDLHIKKAKE', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64) || overlaps(outInteger, inHighF64) || overlaps(outInteger, inLowF64) || overlaps(outInteger, inCloseF64)) {
    throw failure('CDLHIKKAKE', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdlhikkakeImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLHIKKAKE', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live CDLHIKKAKE stream: one value per closed bar, bit-identical to
 * {@link cdlhikkake} over the same series. Open it with {@link cdlhikkakeOpen} or
 * {@link cdlhikkakeOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlhikkake](https://ta-lib.org/functions/cdlhikkake).
 */
export class CdlhikkakeStream {
  /** @internal */
  core: Core;
  /** @internal */
  patternResult: number = 0;
  /** @internal */
  cd: number = 0;
  /** @internal */
  savedHigh: number = 0;
  /** @internal */
  savedLow: number = 0;
  /** @internal */
  lag1_inHigh: number = 0;
  /** @internal */
  lag2_inHigh: number = 0;
  /** @internal */
  lag1_inLow: number = 0;
  /** @internal */
  lag2_inLow: number = 0;
  /** @internal */
  cur_outInteger: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): CdlhikkakeStream {
    return new CdlhikkakeStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cdlhikkake} reports them over the same bars. Each accepted `update` and
   * each `advance` adds one to `count`.
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
      throw failure('CDLHIKKAKE advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inOpen - The bar's `inOpen` value: Open price of each bar.
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inOpen: number, inHigh: number, inLow: number, inClose: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('CDLHIKKAKE update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inOpen) || !Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('CDLHIKKAKE update', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    cdlhikkakeStepImpl(this, inOpen, inHigh, inLow, inClose);
    this.outRangeCount++;
    return this.cur_outInteger;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inOpen - The bar's `inOpen` value: Open price of each bar.
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inOpen: number, inHigh: number, inLow: number, inClose: number): number {
    if (!Number.isFinite(inOpen) || !Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('CDLHIKKAKE peek', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let cd = sp.cd;
    let cur_outInteger = 0;
    let patternResult = sp.patternResult;
    let savedHigh = sp.savedHigh;
    let savedLow = sp.savedLow;
    if (sp.lag1_inHigh < sp.lag2_inHigh &&
        sp.lag1_inLow > sp.lag2_inLow &&   /* 1st + 2nd: lower high and higher low */
        (inHigh < sp.lag1_inHigh &&
          inLow < sp.lag1_inLow ||         /* (bull) 3rd: lower high and lower low */
         inHigh > sp.lag1_inHigh &&
          inLow > sp.lag1_inLow))          /* (bear) 3rd: higher high and higher low */
    {
      patternResult = ((100 * ((inHigh < sp.lag1_inHigh) ? 1 : -1)) | 0);
      savedHigh = sp.lag1_inHigh;
      savedLow = sp.lag1_inLow;
      cd = 4;
      cur_outInteger = patternResult;
    } else if (cd > 0 &&
        (patternResult > 0 &&    /* search for confirmation if hikkake was no more than 3 bars ago */
          inClose > savedHigh || /* close higher than the high of 2nd */
         patternResult < 0 &&
          inClose < savedLow))   /* close lower than the low of 2nd */
    {
      cur_outInteger = patternResult + ((100 * ((patternResult > 0) ? 1 : -1)) | 0);
      cd = 0;
    } else {
      cur_outInteger = 0;
    }
    return cur_outInteger;
  }

  /** The value at the last bar this stream counted; `peek` does not change it. */
  value(): number {
    return this.cur_outInteger;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): CdlhikkakeStream {
    const c = new CdlhikkakeStream(this.core);
    c.patternResult = this.patternResult;
    c.cd = this.cd;
    c.savedHigh = this.savedHigh;
    c.savedLow = this.savedLow;
    c.lag1_inHigh = this.lag1_inHigh;
    c.lag2_inHigh = this.lag2_inHigh;
    c.lag1_inLow = this.lag1_inLow;
    c.lag2_inLow = this.lag2_inLow;
    c.cur_outInteger = this.cur_outInteger;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cdlhikkakeStepImpl(sp: CdlhikkakeStream, _inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  if (sp.lag1_inHigh < sp.lag2_inHigh &&
      sp.lag1_inLow > sp.lag2_inLow &&   /* 1st + 2nd: lower high and higher low */
      (inHigh < sp.lag1_inHigh &&
        inLow < sp.lag1_inLow ||         /* (bull) 3rd: lower high and lower low */
       inHigh > sp.lag1_inHigh &&
        inLow > sp.lag1_inLow))          /* (bear) 3rd: higher high and higher low */
  {
    sp.patternResult = ((100 * ((inHigh < sp.lag1_inHigh) ? 1 : -1)) | 0);
    sp.savedHigh = sp.lag1_inHigh;
    sp.savedLow = sp.lag1_inLow;
    sp.cd = 4;
    sp.cur_outInteger = sp.patternResult;
  } else if (sp.cd > 0 &&
      (sp.patternResult > 0 &&    /* search for confirmation if hikkake was no more than 3 bars ago */
        inClose > sp.savedHigh || /* close higher than the high of 2nd */
       sp.patternResult < 0 &&
        inClose < sp.savedLow))   /* close lower than the low of 2nd */
  {
    sp.cur_outInteger = sp.patternResult + ((100 * ((sp.patternResult > 0) ? 1 : -1)) | 0);
    sp.cd = 0;
  } else {
    sp.cur_outInteger = 0;
  }
  if (sp.cd > 0) {
    sp.cd -= 1;
  }
  sp.lag2_inHigh = sp.lag1_inHigh;
  sp.lag1_inHigh = inHigh;
  sp.lag2_inLow = sp.lag1_inLow;
  sp.lag1_inLow = inLow;
}

function cdlhikkakeOpenImpl(
  sp: CdlhikkakeStream,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let i = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let patternResult = 0;
  let cd = 0;
  let savedHigh = 0;
  let savedLow = 0;
  const historyLen = inOpen.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inHigh.length !== inOpen.length || inLow.length !== inOpen.length || inClose.length !== inOpen.length) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  savedHigh = 0.0;
  savedLow = 0.0;
  /* Confirmation-window countdown + cached 2nd-candle high/low: the pattern
   * state carried without an absolute bar index.
   */
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlhikkakeLookback(core);
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
  /* Do the calculation using tight loops. */
  /* Add-up the initial period, except for the last value. */
  cd = 0;
  patternResult = 0;
  i = startIdx - 3;
  while (i < startIdx) {
    /* copy here the pattern recognition code below */
    if (inHigh[i - 1] < inHigh[i - 2] &&
        inLow[i - 1] > inLow[i - 2] &&   /* 1st + 2nd: lower high and higher low */
        (inHigh[i] < inHigh[i - 1] &&
          inLow[i] < inLow[i - 1] ||     /* (bull) 3rd: lower high and lower low */
         inHigh[i] > inHigh[i - 1] &&
          inLow[i] > inLow[i - 1]))      /* (bear) 3rd: higher high and higher low */
    {
      patternResult = ((100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : -1)) | 0);
      savedHigh = inHigh[i - 1];
      savedLow = inLow[i - 1];
      cd = 4;
    } else if (cd > 0 &&
        (patternResult > 0 &&       /* search for confirmation if hikkake was no more than 3 bars ago */
          inClose[i] > savedHigh || /* close higher than the high of 2nd */
         patternResult < 0 &&
          inClose[i] < savedLow))   /* close lower than the low of 2nd */
    {
      cd = 0;
    }
    if (cd > 0) {
      cd -= 1;
    }
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first and second candle: inside bar (2nd has lower high and higher low than 1st)
   * - third candle: lower high and lower low than 2nd (higher high and higher low than 2nd)
   * Confirmation could come in the next 3 days with:
   * - a day that closes higher than the high (lower than the low) of the 2nd candle
   */
  outIdx = 0;
  do {
    if (inHigh[i - 1] < inHigh[i - 2] &&
        inLow[i - 1] > inLow[i - 2] &&   /* 1st + 2nd: lower high and higher low */
        (inHigh[i] < inHigh[i - 1] &&
          inLow[i] < inLow[i - 1] ||     /* (bull) 3rd: lower high and lower low */
         inHigh[i] > inHigh[i - 1] &&
          inLow[i] > inLow[i - 1]))      /* (bear) 3rd: higher high and higher low */
    {
      patternResult = ((100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : -1)) | 0);
      savedHigh = inHigh[i - 1];
      savedLow = inLow[i - 1];
      cd = 4;
      outInteger[((outIdx++ * outStride) | 0)] = patternResult;
    } else if (cd > 0 &&
        (patternResult > 0 &&       /* search for confirmation if hikkake was no more than 3 bars ago */
          inClose[i] > savedHigh || /* close higher than the high of 2nd */
         patternResult < 0 &&
          inClose[i] < savedLow))   /* close lower than the low of 2nd */
    {
      outInteger[((outIdx++ * outStride) | 0)] = patternResult + ((100 * ((patternResult > 0) ? 1 : -1)) | 0);
      cd = 0;
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    if (cd > 0) {
      cd -= 1;
    }
    i += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  sp.patternResult = patternResult;
  sp.cd = cd;
  sp.savedHigh = savedHigh;
  sp.savedLow = savedLow;
  sp.lag1_inHigh = inHigh[historyLen - 1];
  sp.lag2_inHigh = inHigh[historyLen - 2];
  sp.lag1_inLow = inLow[historyLen - 1];
  sp.lag2_inLow = inLow[historyLen - 2];
  sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlhikkakeOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): CdlhikkakeStream {
  const sp = CdlhikkakeStream.of(core);
  const retCode = cdlhikkakeOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLHIKKAKE openAndFill', inOpen.length, startIdx, cdlhikkakeLookback(core));
  }
  throw streamFailure('CDLHIKKAKE openAndFill', retCode);
}

/** @internal */
export function cdlhikkakeOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): CdlhikkakeStream {
  const sp = CdlhikkakeStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdlhikkakeOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLHIKKAKE open', inOpen.length, startIdx, cdlhikkakeLookback(core));
  }
  throw streamFailure('CDLHIKKAKE open', retCode);
}

/**
 * Opens a live CDLHIKKAKE stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link cdlhikkake} there.
 *
 * @param inOpen - Warm-up history. Open price of each bar. Every history has the
 *   same length.
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlhikkakeLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an
 *   empty one, and `TA_BAD_PARAM` for histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdlhikkakeOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdlhikkakeOpen(open, high, low, close);
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdlhikkakeOpen(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): CdlhikkakeStream {
  requireArgument('CDLHIKKAKE open', 'inOpen', inOpen);
  requireHistory('CDLHIKKAKE open', inOpen.length);
  requireArgument('CDLHIKKAKE open', 'inHigh', inHigh);
  requireArgument('CDLHIKKAKE open', 'inLow', inLow);
  requireArgument('CDLHIKKAKE open', 'inClose', inClose);
  requireHistoryLength('CDLHIKKAKE open', 'inHigh', inHigh.length, inOpen.length);
  requireHistoryLength('CDLHIKKAKE open', 'inLow', inLow.length, inOpen.length);
  requireHistoryLength('CDLHIKKAKE open', 'inClose', inClose.length, inOpen.length);
  return cdlhikkakeOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/**
 * {@link cdlhikkakeOpen} that also fills the outputs over the whole history in the
 * same pass, bit-identically to {@link cdlhikkake}. The range written is the
 * stream's `outRange()`.
 *
 * @param inOpen - Warm-up history. Open price of each bar. Every history has the
 *   same length.
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param outInteger - +100/-100 at the hikkake (breakout) bar for bull/bear;
 *   +200/-200 at a later confirmation bar; 0 otherwise. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlhikkakeLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an
 *   empty one, and `TA_BAD_PARAM` for histories of different lengths or an output
 *   too short or starting where another output or a `Float64Array` input starts, or,
 *   on a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdlhikkakeOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const out = new Int32Array(open.length);
 * const stream = cdlhikkakeOpenAndFill(open, high, low, close, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function cdlhikkakeOpenAndFill(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): CdlhikkakeStream {
  requireArgument('CDLHIKKAKE openAndFill', 'inOpen', inOpen);
  requireHistory('CDLHIKKAKE openAndFill', inOpen.length);
  requireArgument('CDLHIKKAKE openAndFill', 'inHigh', inHigh);
  requireArgument('CDLHIKKAKE openAndFill', 'inLow', inLow);
  requireArgument('CDLHIKKAKE openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('CDLHIKKAKE openAndFill', inOpen.length, cdlhikkakeLookback(core));
  requireHistoryLength('CDLHIKKAKE openAndFill', 'inHigh', inHigh.length, inOpen.length);
  requireHistoryLength('CDLHIKKAKE openAndFill', 'inLow', inLow.length, inOpen.length);
  requireHistoryLength('CDLHIKKAKE openAndFill', 'inClose', inClose.length, inOpen.length);
  requireOutput('CDLHIKKAKE openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) || (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) || (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) || (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset)) {
    throw streamFailure('CDLHIKKAKE openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outInteger, inOpenF64) || overlaps(outInteger, inHighF64) || overlaps(outInteger, inLowF64) || overlaps(outInteger, inCloseF64))) {
    throw streamFailure('CDLHIKKAKE openAndFill', RetCode.BAD_PARAM);
  }
  return cdlhikkakeOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}
