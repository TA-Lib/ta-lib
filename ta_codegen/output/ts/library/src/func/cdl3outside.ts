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

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function cdl3outsideLookbackPositional(_core: Core = Core.DEFAULT): number {
  return 3;
}

/** @internal */
export function cdl3outsideDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
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
  lookbackTotal = cdl3outsideLookbackPositional(core);
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

/** @internal */
export function cdl3outsidePositional(
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
  const guardStart = clampedStart('CDL3OUTSIDE', startIdx, cdl3outsideLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDL3OUTSIDE', 'prices.open', inOpen, guardInLen);
  requireLength('CDL3OUTSIDE', 'prices.high', inHigh, guardInLen);
  requireLength('CDL3OUTSIDE', 'prices.low', inLow, guardInLen);
  requireLength('CDL3OUTSIDE', 'prices.close', inClose, guardInLen);
  requireOutput('CDL3OUTSIDE', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDL3OUTSIDE', 'outInteger', 'prices.open');
  }
  if (overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDL3OUTSIDE', 'outInteger', 'prices.high');
  }
  if (overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDL3OUTSIDE', 'outInteger', 'prices.low');
  }
  if (overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDL3OUTSIDE', 'outInteger', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdl3outsideImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDL3OUTSIDE', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cdl3outside}, {@link cdl3outsideLookback},
 * {@link cdl3outsideDisplayShift}, {@link cdl3outsideOpen} and
 * {@link cdl3outsideOpenAndFill}. Every key is optional: an omitted or `undefined`
 * key takes its default, and a call ignores the keys it has no use for.
 */
export interface Cdl3outsideOptions {
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
 * What {@link cdl3outside} returns, and {@link cdl3outsideOpenAndFill} with its
 * stream.
 */
export interface Cdl3outsideResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * +100 for Three Outside Up (bullish), -100 for Three Outside Down (bearish), 0
   * when no pattern. Emits both signs; value is candle i-1's color * 100.
   * `values[k]` is the value at input bar `begIdx + k`.
   */
  values: Int32Array;
}

/**
 * Number of leading input bars {@link cdl3outside} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link Cdl3outsideOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdl3outsideLookback(options?: Cdl3outsideOptions): number {
  const o = optionsOf('CDL3OUTSIDE lookback', options);
  return cdl3outsideLookbackPositional(coreOf('CDL3OUTSIDE lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdl3outside}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link Cdl3outsideOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdl3outsideDisplayShift(outputIdx: number, options?: Cdl3outsideOptions): number {
  const o = optionsOf('CDL3OUTSIDE displayShift', options);
  return cdl3outsideDisplayShiftPositional(outputIdx, coreOf('CDL3OUTSIDE displayShift', o.core));
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
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cdl3outsideLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `open`: Open
 *   price of each bar. `high`: High price of each bar. `low`: Low price of each bar.
 *   `close`: Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link Cdl3outsideOptions}.
 * @returns `begIdx` and the values; see {@link Cdl3outsideResult}. Each output is
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
 * const { begIdx, values } = cdl3outside({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`cdl3inside`](https://ta-lib.org/functions/cdl3inside)
 * @see [`cdlengulfing`](https://ta-lib.org/functions/cdlengulfing)
 * @see [`cdl3linestrike`](https://ta-lib.org/functions/cdl3linestrike)
 */
export function cdl3outside(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: Cdl3outsideOptions): Cdl3outsideResult {
  requireArgument('CDL3OUTSIDE', 'prices', prices);
  const o = optionsOf('CDL3OUTSIDE', options);
  if (o.endIdx === undefined) {
    requireSeries('CDL3OUTSIDE', 'prices.open', prices.open);
    requireSeries('CDL3OUTSIDE', 'prices.high', prices.high);
    requireSeries('CDL3OUTSIDE', 'prices.low', prices.low);
    requireSeries('CDL3OUTSIDE', 'prices.close', prices.close);
  }
  const core = coreOf('CDL3OUTSIDE', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cdl3outsideLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const range = cdl3outsidePositional(startIdx, endIdx, prices.open, prices.high, prices.low, prices.close, outInteger, core);
  return { begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
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

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): Cdl3outsideStream {
    return new Cdl3outsideStream(core);
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
      throw streamFailure('CDL3OUTSIDE advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw streamFailure('CDL3OUTSIDE update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(open) || !Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('CDL3OUTSIDE update', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    cdl3outsideStepImpl(this, open, high, low, close);
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
      throw nonFiniteBar('CDL3OUTSIDE peek', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inClose = close;
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
  lookbackTotal = cdl3outsideLookbackPositional(core);
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
  const sp = Cdl3outsideStream.of(core);
  const retCode = cdl3outsideOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDL3OUTSIDE openAndFill', inOpen.length, startIdx, cdl3outsideLookbackPositional(core));
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
  const sp = Cdl3outsideStream.of(core);
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
    throw insufficientHistory('CDL3OUTSIDE open', inOpen.length, startIdx, cdl3outsideLookbackPositional(core));
  }
  throw streamFailure('CDL3OUTSIDE open', retCode);
}

/** @internal */
export function cdl3outsideOpenPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): Cdl3outsideStream {
  requireArgument('CDL3OUTSIDE open', 'prices.open', inOpen);
  requireHistory('CDL3OUTSIDE open', 'prices.open', inOpen.length);
  requireArgument('CDL3OUTSIDE open', 'prices.high', inHigh);
  requireArgument('CDL3OUTSIDE open', 'prices.low', inLow);
  requireArgument('CDL3OUTSIDE open', 'prices.close', inClose);
  requireHistoryLength('CDL3OUTSIDE open', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDL3OUTSIDE open', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDL3OUTSIDE open', 'prices.close', inClose.length, inOpen.length);
  return cdl3outsideOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/** @internal */
export function cdl3outsideOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): Cdl3outsideStream {
  requireArgument('CDL3OUTSIDE openAndFill', 'prices.open', inOpen);
  requireHistory('CDL3OUTSIDE openAndFill', 'prices.open', inOpen.length);
  requireArgument('CDL3OUTSIDE openAndFill', 'prices.high', inHigh);
  requireArgument('CDL3OUTSIDE openAndFill', 'prices.low', inLow);
  requireArgument('CDL3OUTSIDE openAndFill', 'prices.close', inClose);
  const guardOutLen = openFillCount('CDL3OUTSIDE openAndFill', inOpen.length, cdl3outsideLookbackPositional(core));
  requireHistoryLength('CDL3OUTSIDE openAndFill', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDL3OUTSIDE openAndFill', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDL3OUTSIDE openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('CDL3OUTSIDE openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('CDL3OUTSIDE openAndFill', 'outInteger', 'prices.open');
  }
  if (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CDL3OUTSIDE openAndFill', 'outInteger', 'prices.high');
  }
  if (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CDL3OUTSIDE openAndFill', 'outInteger', 'prices.low');
  }
  if (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('CDL3OUTSIDE openAndFill', 'outInteger', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDL3OUTSIDE openAndFill', 'outInteger', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDL3OUTSIDE openAndFill', 'outInteger', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDL3OUTSIDE openAndFill', 'outInteger', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDL3OUTSIDE openAndFill', 'outInteger', 'prices.close');
  }
  return cdl3outsideOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}

/**
 * Opens a live CDL3OUTSIDE stream over the warm-up history: `value()` starts at
 * the last history bar, bit-identical to {@link cdl3outside} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `open`: Open price of each bar. `high`: High price of each bar. `low`:
 *   Low price of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link Cdl3outsideOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdl3outsideLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input or histories of different lengths.
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
 * const stream = cdl3outsideOpen({ open, high, low, close });
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdl3outsideOpen(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: Cdl3outsideOptions): Cdl3outsideStream {
  requireArgument('CDL3OUTSIDE open', 'prices', prices);
  const o = optionsOf('CDL3OUTSIDE open', options);
  const core = coreOf('CDL3OUTSIDE open', o.core);
  return cdl3outsideOpenPositional(prices.open, prices.high, prices.low, prices.close, core);
}

/**
 * {@link cdl3outsideOpen} that also computes the outputs over the whole history in
 * the same pass, bit-identically to {@link cdl3outside}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `open`: Open price of each bar. `high`: High price of each bar. `low`:
 *   Low price of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link Cdl3outsideOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cdl3outside} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdl3outsideLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, histories of different lengths or a buffer too short,
 *   of the wrong type or starting where another buffer or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   buffer or a `Float64Array` input.
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
 * const { stream, begIdx, values } = cdl3outsideOpenAndFill({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cdl3outsideOpenAndFill(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: Cdl3outsideOptions): Cdl3outsideResult & { stream: Cdl3outsideStream } {
  requireArgument('CDL3OUTSIDE openAndFill', 'prices', prices);
  const o = optionsOf('CDL3OUTSIDE openAndFill', options);
  const core = coreOf('CDL3OUTSIDE openAndFill', o.core);
  const capacity = fillCapacity(cdl3outsideLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const stream = cdl3outsideOpenAndFillPositional(prices.open, prices.high, prices.low, prices.close, outInteger, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}
