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
 *  121104 AC     Creation
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link cdl3outside} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function cdl3outsideLookback(core: Core = Core.DEFAULT): number {
  void core;
  return 3;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdl3outside}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function cdl3outsideDisplayShift(outputIdx: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdl3outsideImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inOpen: Float64Array,
  _inHigh: Float64Array,
  _inLow: Float64Array,
  inClose: Float64Array,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): RetCode {
  let i = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdl3outsideLookback(core);
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
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first: black (white) real body
   * - second: white (black) real body that engulfs the prior real body
   * - third: candle that closes higher (lower) than the second candle
   */
  outIdx = 0;
  do {
    if (((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === 1 &&
         ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === -1 && /* white engulfs black */
         inClose[i - 1] > inOpen[i - 2] &&
         inOpen[i - 1] < inClose[i - 2] &&
         inClose[i] > inClose[i - 1] ||                         /* third candle higher */
        ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === -1 &&
         ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === 1 &&  /* black engulfs white */
         inOpen[i - 1] > inClose[i - 2] &&
         inClose[i - 1] < inOpen[i - 2] &&
         inClose[i] < inClose[i - 1])                           /* third candle lower */
    {
      outInteger[outIdx++] = ((((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) * 100) | 0);
    } else {
      outInteger[outIdx++] = 0;
    }
    i += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/**
 * A three-candle pattern: an engulfing pair (candle 2's body fully engulfs candle
 * 1's body) followed by a third candle that confirms in the engulfing direction.
 * Signals a bullish reversal (Three Outside Up) or bearish reversal (Three Outside
 * Down).
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdl3outside](https://ta-lib.org/functions/cdl3outside).
 *
 * **Notes**
 * - Does not verify the prior trend the pattern classically assumes (three outside
 *   up is meaningful in a downtrend, three outside down in an uptrend).
 * - Bulkowski's testing puts Three Outside Up at a 75% bullish-reversal success
 *   rate versus 69% for Three Outside Down — both notably higher than the closely
 *   related Three Inside Up/Down (65%/60%), i.e. the engulfing "outside" variant
 *   tests as more reliable than the harami "inside" variant.
 *   ([thepatternsite.com](https://thepatternsite.com/ThreeOutsideUp.html))
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link cdl3outsideLookback} is a success
 * with `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inOpen - Open price of each bar.
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param outInteger - +100 for Three Outside Up (bullish), -100 for Three Outside
 *   Down (bearish), 0 when no pattern. Emits both signs; value is candle i-1's color * 100.
 *   Must hold the values produced and never be empty; the input length always
 *   suffices.
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
 * import { cdl3outside } from 'ta-lib';
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
 * const { begIdx, count } = cdl3outside(
 *   0, open.length - 1,
 *   open, high, low, close,
 *   out,
 * );
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`cdl3inside`](https://ta-lib.org/functions/cdl3inside)
 * @see [`cdlengulfing`](https://ta-lib.org/functions/cdlengulfing)
 * @see [`cdl3linestrike`](https://ta-lib.org/functions/cdl3linestrike)
 */
export function cdl3outside(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDL3OUTSIDE', 'startIdx', startIdx);
  endIdx = requireInt32('CDL3OUTSIDE', 'endIdx', endIdx);
  requireIndexRange('CDL3OUTSIDE', startIdx, endIdx);
  const guardStart = clampedStart('CDL3OUTSIDE', startIdx, cdl3outsideLookback(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDL3OUTSIDE', 'inOpen', inOpen, guardInLen);
  requireLength('CDL3OUTSIDE', 'inHigh', inHigh, guardInLen);
  requireLength('CDL3OUTSIDE', 'inLow', inLow, guardInLen);
  requireLength('CDL3OUTSIDE', 'inClose', inClose, guardInLen);
  requireLength('CDL3OUTSIDE', 'outInteger', outInteger, guardOutLen);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64) || overlaps(outInteger, inHighF64) || overlaps(outInteger, inLowF64) || overlaps(outInteger, inCloseF64)) {
    throw failure('CDL3OUTSIDE', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdl3outsideImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDL3OUTSIDE', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live CDL3OUTSIDE stream: one value per closed bar, bit-identical to
 * {@link cdl3outside} over the same series. Open it with {@link cdl3outsideOpen}
 * or {@link cdl3outsideOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdl3outside](https://ta-lib.org/functions/cdl3outside).
 */
export class Cdl3outsideStream {
  /** @internal */
  core: Core;
  /** @internal */
  lag1_inOpen: number = 0;
  /** @internal */
  lag2_inOpen: number = 0;
  /** @internal */
  lag1_inClose: number = 0;
  /** @internal */
  lag2_inClose: number = 0;
  /** @internal */
  cur_outInteger: number = 0;
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
   * {@link cdl3outside} reports them over the same bars. Each accepted `update` and
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
      throw failure('CDL3OUTSIDE advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw failure('CDL3OUTSIDE update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inOpen) || !Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('CDL3OUTSIDE update', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    cdl3outsideStepImpl(this, inOpen, inHigh, inLow, inClose);
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
      throw nonFiniteBar('CDL3OUTSIDE peek', !Number.isFinite(inOpen) ? 'inOpen' : !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let cur_outInteger = 0;
    if (((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === 1 &&
         ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === -1 && /* white engulfs black */
         sp.lag1_inClose > sp.lag2_inOpen &&
         sp.lag1_inOpen < sp.lag2_inClose &&
         inClose > sp.lag1_inClose ||                             /* third candle higher */
        ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === -1 &&
         ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === 1 &&  /* black engulfs white */
         sp.lag1_inOpen > sp.lag2_inClose &&
         sp.lag1_inClose < sp.lag2_inOpen &&
         inClose < sp.lag1_inClose)                               /* third candle lower */
    {
      cur_outInteger = ((((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) * 100) | 0);
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
  clone(): Cdl3outsideStream {
    const c = new Cdl3outsideStream(this.core);
    c.lag1_inOpen = this.lag1_inOpen;
    c.lag2_inOpen = this.lag2_inOpen;
    c.lag1_inClose = this.lag1_inClose;
    c.lag2_inClose = this.lag2_inClose;
    c.cur_outInteger = this.cur_outInteger;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cdl3outsideStepImpl(sp: Cdl3outsideStream, inOpen: number, _inHigh: number, _inLow: number, inClose: number): void {
  if (((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === 1 &&
       ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === -1 && /* white engulfs black */
       sp.lag1_inClose > sp.lag2_inOpen &&
       sp.lag1_inOpen < sp.lag2_inClose &&
       inClose > sp.lag1_inClose ||                             /* third candle higher */
      ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === -1 &&
       ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) === 1 &&  /* black engulfs white */
       sp.lag1_inOpen > sp.lag2_inClose &&
       sp.lag1_inClose < sp.lag2_inOpen &&
       inClose < sp.lag1_inClose)                               /* third candle lower */
  {
    sp.cur_outInteger = ((((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) * 100) | 0);
  } else {
    sp.cur_outInteger = 0;
  }
  sp.lag2_inOpen = sp.lag1_inOpen;
  sp.lag1_inOpen = inOpen;
  sp.lag2_inClose = sp.lag1_inClose;
  sp.lag1_inClose = inClose;
}

function cdl3outsideOpenImpl(
  sp: Cdl3outsideStream,
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
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdl3outsideLookback(core);
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
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first: black (white) real body
   * - second: white (black) real body that engulfs the prior real body
   * - third: candle that closes higher (lower) than the second candle
   */
  outIdx = 0;
  do {
    if (((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === 1 &&
         ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === -1 && /* white engulfs black */
         inClose[i - 1] > inOpen[i - 2] &&
         inOpen[i - 1] < inClose[i - 2] &&
         inClose[i] > inClose[i - 1] ||                         /* third candle higher */
        ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === -1 &&
         ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) === 1 &&  /* black engulfs white */
         inOpen[i - 1] > inClose[i - 2] &&
         inClose[i - 1] < inOpen[i - 2] &&
         inClose[i] < inClose[i - 1])                           /* third candle lower */
    {
      outInteger[((outIdx++ * outStride) | 0)] = ((((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) * 100) | 0);
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    i += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  sp.lag1_inOpen = inOpen[historyLen - 1];
  sp.lag2_inOpen = inOpen[historyLen - 2];
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.lag2_inClose = inClose[historyLen - 2];
  sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cdl3outsideOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): Cdl3outsideStream {
  const sp = new Cdl3outsideStream(core);
  const retCode = cdl3outsideOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDL3OUTSIDE openAndFill', inOpen.length, startIdx, cdl3outsideLookback(core));
  }
  throw streamFailure('CDL3OUTSIDE openAndFill', retCode);
}

/** @internal */
export function cdl3outsideOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): Cdl3outsideStream {
  const sp = new Cdl3outsideStream(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdl3outsideOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDL3OUTSIDE open', inOpen.length, startIdx, cdl3outsideLookback(core));
  }
  throw streamFailure('CDL3OUTSIDE open', retCode);
}

/**
 * Opens a live CDL3OUTSIDE stream over the warm-up history: `value()` starts at
 * the last history bar, bit-identical to {@link cdl3outside} there.
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
 *   than `cdl3outsideLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an
 *   empty one, and `TA_BAD_PARAM` for histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdl3outsideOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdl3outsideOpen(open, high, low, close);
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdl3outsideOpen(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): Cdl3outsideStream {
  requireArgument('CDL3OUTSIDE open', 'inOpen', inOpen);
  requireHistory('CDL3OUTSIDE open', inOpen.length);
  requireArgument('CDL3OUTSIDE open', 'inHigh', inHigh);
  requireArgument('CDL3OUTSIDE open', 'inLow', inLow);
  requireArgument('CDL3OUTSIDE open', 'inClose', inClose);
  requireHistoryLength('CDL3OUTSIDE open', 'inHigh', inHigh.length, inOpen.length);
  requireHistoryLength('CDL3OUTSIDE open', 'inLow', inLow.length, inOpen.length);
  requireHistoryLength('CDL3OUTSIDE open', 'inClose', inClose.length, inOpen.length);
  return cdl3outsideOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/**
 * {@link cdl3outsideOpen} that also fills the outputs over the whole history in
 * the same pass, bit-identically to {@link cdl3outside}. The range written is the
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
 * @param outInteger - +100 for Three Outside Up (bullish), -100 for Three Outside
 *   Down (bearish), 0 when no pattern. Emits both signs; value is candle i-1's color * 100.
 *   Must hold `historyLen - lookback` values and share no memory with an input or
 *   another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdl3outsideLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an
 *   empty one, and `TA_BAD_PARAM` for histories of different lengths or an output
 *   too short or starting where another output or a `Float64Array` input starts, or,
 *   on a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdl3outsideOpenAndFill } from 'ta-lib';
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
 * const stream = cdl3outsideOpenAndFill(open, high, low, close, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function cdl3outsideOpenAndFill(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): Cdl3outsideStream {
  requireArgument('CDL3OUTSIDE openAndFill', 'inOpen', inOpen);
  requireHistory('CDL3OUTSIDE openAndFill', inOpen.length);
  requireArgument('CDL3OUTSIDE openAndFill', 'inHigh', inHigh);
  requireArgument('CDL3OUTSIDE openAndFill', 'inLow', inLow);
  requireArgument('CDL3OUTSIDE openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('CDL3OUTSIDE openAndFill', inOpen.length, cdl3outsideLookback(core));
  requireHistoryLength('CDL3OUTSIDE openAndFill', 'inHigh', inHigh.length, inOpen.length);
  requireHistoryLength('CDL3OUTSIDE openAndFill', 'inLow', inLow.length, inOpen.length);
  requireHistoryLength('CDL3OUTSIDE openAndFill', 'inClose', inClose.length, inOpen.length);
  requireLength('CDL3OUTSIDE openAndFill', 'outInteger', outInteger, guardOutLen);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) || (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) || (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) || (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset)) {
    throw streamFailure('CDL3OUTSIDE openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outInteger, inOpenF64) || overlaps(outInteger, inHighF64) || overlaps(outInteger, inLowF64) || overlaps(outInteger, inCloseF64))) {
    throw streamFailure('CDL3OUTSIDE openAndFill', RetCode.BAD_PARAM);
  }
  return cdl3outsideOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}
