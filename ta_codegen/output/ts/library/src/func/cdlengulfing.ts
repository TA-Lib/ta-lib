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
 *  102404 AC     Creation
 *  040309 AC     Increased flexibility to allow real bodies matching
 *                on one end (Greg Morris - "Candlestick charting explained")
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function cdlengulfingLookbackPositional(_core: Core = Core.DEFAULT): number {
  return 2;
}

/** @internal */
export function cdlengulfingDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdlengulfingImpl(
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
  lookbackTotal = cdlengulfingLookbackPositional(core);
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
   */
  outIdx = 0;
  do {
    if (((inClose[i] >= inOpen[i]) ? 1 : -1) === 1 &&
         ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === -1 && /* white engulfs black */
         (inClose[i] >= inOpen[i - 1] &&
           inOpen[i] < inClose[i - 1] ||
          inClose[i] > inOpen[i - 1] &&
           inOpen[i] <= inClose[i - 1]) ||
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === -1 &&
         ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === 1 &&  /* black engulfs white */
         (inOpen[i] >= inClose[i - 1] &&
           inClose[i] < inOpen[i - 1] ||
          inOpen[i] > inClose[i - 1] &&
           inClose[i] <= inOpen[i - 1]))
    {
      if (inOpen[i] !== inClose[i - 1] && inClose[i] !== inOpen[i - 1]) {
        outInteger[outIdx++] = ((((inClose[i] >= inOpen[i]) ? 1 : -1) * 100) | 0);
      } else {
        outInteger[outIdx++] = ((((inClose[i] >= inOpen[i]) ? 1 : -1) * 80) | 0);
      }
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

/** @internal */
export function cdlengulfingPositional(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDLENGULFING', 'startIdx', startIdx);
  endIdx = requireInt32('CDLENGULFING', 'endIdx', endIdx);
  requireIndexRange('CDLENGULFING', startIdx, endIdx);
  const guardStart = clampedStart('CDLENGULFING', startIdx, cdlengulfingLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLENGULFING', 'prices.open', inOpen, guardInLen);
  requireLength('CDLENGULFING', 'prices.high', inHigh, guardInLen);
  requireLength('CDLENGULFING', 'prices.low', inLow, guardInLen);
  requireLength('CDLENGULFING', 'prices.close', inClose, guardInLen);
  requireOutput('CDLENGULFING', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLENGULFING', 'outInteger', 'prices.open');
  }
  if (overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLENGULFING', 'outInteger', 'prices.high');
  }
  if (overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLENGULFING', 'outInteger', 'prices.low');
  }
  if (overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLENGULFING', 'outInteger', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdlengulfingImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLENGULFING', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cdlengulfing}, {@link cdlengulfingLookback},
 * {@link cdlengulfingDisplayShift}, {@link cdlengulfingOpen} and
 * {@link cdlengulfingOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface CdlengulfingOptions {
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
   * buffer or with an input.
   */
  outInteger?: Int32Array | undefined;
}

/**
 * What {@link cdlengulfing} returns, and {@link cdlengulfingOpenAndFill} with its
 * stream.
 */
export interface CdlengulfingResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * +100/+80 (bullish, white engulfs black), -100/-80 (bearish, black engulfs
   * white), 0 otherwise. Magnitude 100 when the second body strictly engulfs both
   * ends; 80 when the bodies share an exact endpoint (open\[i]==close\[i-1] or
   * close\[i]==open\[i-1]). `values[k]` is the value at input bar `begIdx + k`.
   */
  values: Int32Array;
}

/**
 * Number of leading input bars {@link cdlengulfing} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlengulfingOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlengulfingLookback(options?: CdlengulfingOptions): number {
  const o = optionsOf('CDLENGULFING lookback', options);
  return cdlengulfingLookbackPositional(coreOf('CDLENGULFING lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdlengulfing}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlengulfingOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlengulfingDisplayShift(outputIdx: number, options?: CdlengulfingOptions): number {
  const o = optionsOf('CDLENGULFING displayShift', options);
  return cdlengulfingDisplayShiftPositional(outputIdx, coreOf('CDLENGULFING displayShift', o.core));
}

/**
 * A two-candle reversal pattern where the second candle's real body engulfs the
 * first candle's opposite-colored real body. Bullish (white engulfs black) or
 * bearish (black engulfs white) reversal signal; ideally after a downtrend
 * (bullish) or uptrend (bearish), which the code does not verify.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlengulfing](https://ta-lib.org/functions/cdlengulfing).
 *
 * **Notes**
 * - Does not verify the prior trend (down for bullish, up for bearish) the
 *   reversal classically assumes.
 * - Bulkowski's testing found bearish Engulfing has a strong 79% reversal rate
 *   (5th-best of 103 patterns by that measure alone) but a weak overall
 *   post-breakout performance rank of 91st of 103 — the reversal fires reliably
 *   but rarely sustains. Bullish Engulfing reverses 63% of the time with a
 *   similarly weak overall rank of 84th of 103.
 *   ([thepatternsite.com](https://thepatternsite.com/BearEngulfing.html))
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cdlengulfingLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `open`: Open
 *   price of each bar. `high`: High price of each bar. `low`: Low price of each bar.
 *   `close`: Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link CdlengulfingOptions}.
 * @returns `begIdx` and the values; see {@link CdlengulfingResult}. Each output is
 *   a view of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, an input too short for the range, a buffer that is empty, too short or of
 *   the wrong type or a buffer sharing memory with a `Float64Array` input it is not.
 *   Checked before anything is written.
 *
 * @example
 * ```ts
 * import { cdlengulfing } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { begIdx, values } = cdlengulfing({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`cdlharami`](https://ta-lib.org/functions/cdlharami)
 * @see [`cdlcounterattack`](https://ta-lib.org/functions/cdlcounterattack)
 * @see [`cdlharamicross`](https://ta-lib.org/functions/cdlharamicross)
 */
export function cdlengulfing(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: CdlengulfingOptions): CdlengulfingResult {
  requireArgument('CDLENGULFING', 'prices', prices);
  const o = optionsOf('CDLENGULFING', options);
  if (o.endIdx === undefined) {
    requireSeries('CDLENGULFING', 'prices.open', prices.open);
    requireSeries('CDLENGULFING', 'prices.high', prices.high);
    requireSeries('CDLENGULFING', 'prices.low', prices.low);
    requireSeries('CDLENGULFING', 'prices.close', prices.close);
  }
  const core = coreOf('CDLENGULFING', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cdlengulfingLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const range = cdlengulfingPositional(startIdx, endIdx, prices.open, prices.high, prices.low, prices.close, outInteger, core);
  return { begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CDLENGULFING stream: one value per closed bar, bit-identical to
 * {@link cdlengulfing} over the same series. Open it with {@link cdlengulfingOpen}
 * or {@link cdlengulfingOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlengulfing](https://ta-lib.org/functions/cdlengulfing).
 */
export class CdlengulfingStream {
  /** @internal */
  core: Core;
  /** @internal */
  lag1_inOpen: number = 0;
  /** @internal */
  lag1_inClose: number = 0;
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
  static of(core: Core): CdlengulfingStream {
    return new CdlengulfingStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cdlengulfing} reports them over the same bars. Each accepted `update` and
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
      throw streamFailure('CDLENGULFING advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param open - The bar's `open` value: Open price of each bar.
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(open: number, high: number, low: number, close: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('CDLENGULFING update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(open) || !Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('CDLENGULFING update', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    cdlengulfingStepImpl(this, open, high, low, close);
    this.outRangeCount++;
    return this.cur_outInteger;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param open - The bar's `open` value: Open price of each bar.
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(open: number, high: number, low: number, close: number): number {
    if (!Number.isFinite(open) || !Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('CDLENGULFING peek', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inOpen = open;
    const inClose = close;
    let cur_outInteger = 0;
    if (((inClose >= inOpen) ? 1 : -1) === 1 &&
         ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === -1 && /* white engulfs black */
         (inClose >= sp.lag1_inOpen &&
           inOpen < sp.lag1_inClose ||
          inClose > sp.lag1_inOpen &&
           inOpen <= sp.lag1_inClose) ||
        ((inClose >= inOpen) ? 1 : -1) === -1 &&
         ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === 1 &&  /* black engulfs white */
         (inOpen >= sp.lag1_inClose &&
           inClose < sp.lag1_inOpen ||
          inOpen > sp.lag1_inClose &&
           inClose <= sp.lag1_inOpen))
    {
      if (inOpen !== sp.lag1_inClose && inClose !== sp.lag1_inOpen) {
        cur_outInteger = ((((inClose >= inOpen) ? 1 : -1) * 100) | 0);
      } else {
        cur_outInteger = ((((inClose >= inOpen) ? 1 : -1) * 80) | 0);
      }
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
  clone(): CdlengulfingStream {
    const c = new CdlengulfingStream(this.core);
    c.lag1_inOpen = this.lag1_inOpen;
    c.lag1_inClose = this.lag1_inClose;
    c.cur_outInteger = this.cur_outInteger;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cdlengulfingStepImpl(sp: CdlengulfingStream, inOpen: number, _inHigh: number, _inLow: number, inClose: number): void {
  if (((inClose >= inOpen) ? 1 : -1) === 1 &&
       ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === -1 && /* white engulfs black */
       (inClose >= sp.lag1_inOpen &&
         inOpen < sp.lag1_inClose ||
        inClose > sp.lag1_inOpen &&
         inOpen <= sp.lag1_inClose) ||
      ((inClose >= inOpen) ? 1 : -1) === -1 &&
       ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) === 1 &&  /* black engulfs white */
       (inOpen >= sp.lag1_inClose &&
         inClose < sp.lag1_inOpen ||
        inOpen > sp.lag1_inClose &&
         inClose <= sp.lag1_inOpen))
  {
    if (inOpen !== sp.lag1_inClose && inClose !== sp.lag1_inOpen) {
      sp.cur_outInteger = ((((inClose >= inOpen) ? 1 : -1) * 100) | 0);
    } else {
      sp.cur_outInteger = ((((inClose >= inOpen) ? 1 : -1) * 80) | 0);
    }
  } else {
    sp.cur_outInteger = 0;
  }
  sp.lag1_inOpen = inOpen;
  sp.lag1_inClose = inClose;
}

function cdlengulfingOpenImpl(
  sp: CdlengulfingStream,
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
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inHigh.length !== inOpen.length || inLow.length !== inOpen.length || inClose.length !== inOpen.length) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlengulfingLookbackPositional(core);
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
   */
  outIdx = 0;
  do {
    if (((inClose[i] >= inOpen[i]) ? 1 : -1) === 1 &&
         ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === -1 && /* white engulfs black */
         (inClose[i] >= inOpen[i - 1] &&
           inOpen[i] < inClose[i - 1] ||
          inClose[i] > inOpen[i - 1] &&
           inOpen[i] <= inClose[i - 1]) ||
        ((inClose[i] >= inOpen[i]) ? 1 : -1) === -1 &&
         ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) === 1 &&  /* black engulfs white */
         (inOpen[i] >= inClose[i - 1] &&
           inClose[i] < inOpen[i - 1] ||
          inOpen[i] > inClose[i - 1] &&
           inClose[i] <= inOpen[i - 1]))
    {
      if (inOpen[i] !== inClose[i - 1] && inClose[i] !== inOpen[i - 1]) {
        outInteger[((outIdx++ * outStride) | 0)] = ((((inClose[i] >= inOpen[i]) ? 1 : -1) * 100) | 0);
      } else {
        outInteger[((outIdx++ * outStride) | 0)] = ((((inClose[i] >= inOpen[i]) ? 1 : -1) * 80) | 0);
      }
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    i += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  sp.lag1_inOpen = inOpen[historyLen - 1];
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlengulfingOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): CdlengulfingStream {
  const sp = CdlengulfingStream.of(core);
  const retCode = cdlengulfingOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLENGULFING openAndFill', inOpen.length, startIdx, cdlengulfingLookbackPositional(core));
  }
  throw streamFailure('CDLENGULFING openAndFill', retCode);
}

/** @internal */
export function cdlengulfingOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): CdlengulfingStream {
  const sp = CdlengulfingStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdlengulfingOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLENGULFING open', inOpen.length, startIdx, cdlengulfingLookbackPositional(core));
  }
  throw streamFailure('CDLENGULFING open', retCode);
}

/** @internal */
export function cdlengulfingOpenPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): CdlengulfingStream {
  requireArgument('CDLENGULFING open', 'prices.open', inOpen);
  requireHistory('CDLENGULFING open', 'prices.open', inOpen.length);
  requireArgument('CDLENGULFING open', 'prices.high', inHigh);
  requireArgument('CDLENGULFING open', 'prices.low', inLow);
  requireArgument('CDLENGULFING open', 'prices.close', inClose);
  requireHistoryLength('CDLENGULFING open', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLENGULFING open', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLENGULFING open', 'prices.close', inClose.length, inOpen.length);
  return cdlengulfingOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/** @internal */
export function cdlengulfingOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): CdlengulfingStream {
  requireArgument('CDLENGULFING openAndFill', 'prices.open', inOpen);
  requireHistory('CDLENGULFING openAndFill', 'prices.open', inOpen.length);
  requireArgument('CDLENGULFING openAndFill', 'prices.high', inHigh);
  requireArgument('CDLENGULFING openAndFill', 'prices.low', inLow);
  requireArgument('CDLENGULFING openAndFill', 'prices.close', inClose);
  const guardOutLen = openFillCount('CDLENGULFING openAndFill', inOpen.length, cdlengulfingLookbackPositional(core));
  requireHistoryLength('CDLENGULFING openAndFill', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLENGULFING openAndFill', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLENGULFING openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('CDLENGULFING openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('CDLENGULFING openAndFill', 'outInteger', 'prices.open');
  }
  if (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CDLENGULFING openAndFill', 'outInteger', 'prices.high');
  }
  if (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CDLENGULFING openAndFill', 'outInteger', 'prices.low');
  }
  if (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('CDLENGULFING openAndFill', 'outInteger', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLENGULFING openAndFill', 'outInteger', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLENGULFING openAndFill', 'outInteger', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLENGULFING openAndFill', 'outInteger', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLENGULFING openAndFill', 'outInteger', 'prices.close');
  }
  return cdlengulfingOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}

/**
 * Opens a live CDLENGULFING stream over the warm-up history: `value()` starts at
 * the last history bar, bit-identical to {@link cdlengulfing} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `open`: Open price of each bar. `high`: High price of each bar. `low`:
 *   Low price of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlengulfingOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlengulfingLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input or histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdlengulfingOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdlengulfingOpen({ open, high, low, close });
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdlengulfingOpen(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: CdlengulfingOptions): CdlengulfingStream {
  requireArgument('CDLENGULFING open', 'prices', prices);
  const o = optionsOf('CDLENGULFING open', options);
  const core = coreOf('CDLENGULFING open', o.core);
  return cdlengulfingOpenPositional(prices.open, prices.high, prices.low, prices.close, core);
}

/**
 * {@link cdlengulfingOpen} that also computes the outputs over the whole history
 * in the same pass, bit-identically to {@link cdlengulfing}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `open`: Open price of each bar. `high`: High price of each bar. `low`:
 *   Low price of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CdlengulfingOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cdlengulfing} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlengulfingLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, histories of different lengths or a buffer too short,
 *   of the wrong type or starting where another buffer or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdlengulfingOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = cdlengulfingOpenAndFill({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cdlengulfingOpenAndFill(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: CdlengulfingOptions): CdlengulfingResult & { stream: CdlengulfingStream } {
  requireArgument('CDLENGULFING openAndFill', 'prices', prices);
  const o = optionsOf('CDLENGULFING openAndFill', options);
  const core = coreOf('CDLENGULFING openAndFill', o.core);
  const capacity = fillCapacity(cdlengulfingLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const stream = cdlengulfingOpenAndFillPositional(prices.open, prices.high, prices.low, prices.close, outInteger, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}
