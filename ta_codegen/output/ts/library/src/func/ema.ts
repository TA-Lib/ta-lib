/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  080926 MF,CC Explicit no-smoothing copy at a period of 1.
 *  081026 MF,CC Fold the internal variant into EMA (issue #183).
 */

import { FuncUnstId } from '../enums.ts';
import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link ema} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param optInTimePeriod - Number of bars in the average; sets smoothing k =
 *   2/(period+1) (default 30; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function emaLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  return optInTimePeriod - 1 + core.unstableCount(FuncUnstId.EMA, ((optInTimePeriod > 1) ? (((((10 * optInTimePeriod) | 0) + 1) / 2) | 0) : 0), ((optInTimePeriod > 1) ? (((((19 * optInTimePeriod) | 0) + 1) / 2) | 0) : 0));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link ema}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Number of bars in the average; sets smoothing k =
 *   2/(period+1) (default 30; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function emaDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (emaLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function emaImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let emaBeta = 0;
  let optInK_1 = 0;
  let tempReal = 0;
  let prevMA = 0;
  let i = 0;
  let today = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = emaLookback(optInTimePeriod, core);
  /* After the lookback call: a double live across a call is saved and
   * restored around every fma call of the loops below, one more instruction
   * per bar.
   *
   * emaBeta + optInK_1 must be exactly 1.0, or a flat input drifts off
   * its level. Each subtraction is exact only from an operand in
   * [0.5,1): at a period of 2 that is optInK_1, above it emaBeta.
   *
   * Above it emaBeta stays as the divide wrote it: the second subtraction
   * would not change a bit, and a register last written by a subtraction
   * costs each FMA reading it one more cycle on Intel P-cores.
   */
  emaBeta = (optInTimePeriod - 1) / (optInTimePeriod + 1);
  optInK_1 = 1.0 - emaBeta;
  if (emaBeta < 0.5) {
    emaBeta = 1.0 - optInK_1;
  }
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
  /* No smoothing at period of 1: the output is a copy of the input
   * (same convention as TA_MA for every MAType). Explicit because the
   * recursion below, at a k of 1.0 and a beta of 0.0, does not keep the
   * sign of a -0.0 input. The unstable period still delays the first
   * output.
   */
  if (optInTimePeriod === 1) {
    outBegIdx.value = startIdx;
    outIdx = 0;
    today = startIdx;
    while (today <= endIdx) {
      outReal[outIdx++] = inReal[today++];
    }
    outNBElement.value = outIdx;
    return RetCode.SUCCESS;
  }
  outBegIdx.value = startIdx;
  /* Do the EMA calculation using tight loops. */
  today = startIdx - lookbackTotal;
  i = optInTimePeriod;
  tempReal = 0.0;
  while (i-- > 0) {
    tempReal += inReal[today++];
  }
  prevMA = tempReal / optInTimePeriod;
  while (today <= startIdx) {
    prevMA = fma(emaBeta, prevMA, optInK_1 * inReal[today++]);
  }
  outReal[0] = prevMA;
  outIdx = 1;
  while (today <= endIdx) {
    prevMA = fma(emaBeta, prevMA, optInK_1 * inReal[today++]);
    outReal[outIdx++] = prevMA;
  }
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * Exponential moving average that weights recent prices more heavily via a
 * recursive smoothing factor. A core building block seeding or composing many
 * other indicators. Reacts faster than SMA; price above/below EMA suggests up/down
 * trend.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ema](https://ta-lib.org/functions/ema).
 *
 * **Notes**
 * - A period of 1 performs no smoothing: the output is a copy of the input.
 *   Allowed since 0.6.5 (issues #48/#59).
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link emaLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - price/data series to smooth.
 * @param optInTimePeriod - Number of bars in the average; sets smoothing k =
 *   2/(period+1) (default 30; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param outReal - the exponential moving average. Must hold the values produced
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
 * import { ema } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const { begIdx, count } = ema(0, data.length - 1, data, 30, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`sma`](https://ta-lib.org/functions/sma)
 * @see [`dema`](https://ta-lib.org/functions/dema)
 * @see [`tema`](https://ta-lib.org/functions/tema)
 * @see [`ma`](https://ta-lib.org/functions/ma)
 * @see [`macd`](https://ta-lib.org/functions/macd)
 * @see [`t3`](https://ta-lib.org/functions/t3)
 */
export function ema(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('EMA', 'startIdx', startIdx);
  endIdx = requireInt32('EMA', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('EMA', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('EMA', startIdx, endIdx);
  const guardStart = clampedStart('EMA', startIdx, emaLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('EMA', 'inReal', inReal, guardInLen);
  requireLength('EMA', 'outReal', outReal, guardOutLen);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw failure('EMA', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = emaImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('EMA', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live EMA stream: one value per closed bar, bit-identical to {@link ema} over
 * the same series. Open it with {@link emaOpen} or {@link emaOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ema](https://ta-lib.org/functions/ema).
 */
export class EmaStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  emaBeta: number = 0;
  /** @internal */
  optInK_1: number = 0;
  /** @internal */
  prevMA: number = 0;
  /** @internal */
  cur_outReal: number = 0;
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
   * {@link ema} reports them over the same bars. Each accepted `update` and each
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
      throw failure('EMA advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: price/data series to smooth.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('EMA update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('EMA update', 'inReal');
    }
    emaStepImpl(this, inReal);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: price/data series to smooth.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number): number {
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('EMA peek', 'inReal');
    }
    const sp = this;
    let cur_outReal = 0;
    let prevMA = sp.prevMA;
    if (sp.optInTimePeriod === 1) {
      cur_outReal = inReal;
      return cur_outReal;
    }
    prevMA = fma(sp.emaBeta, prevMA, sp.optInK_1 * inReal);
    cur_outReal = prevMA;
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
  clone(): EmaStream {
    const c = new EmaStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.emaBeta = this.emaBeta;
    c.optInK_1 = this.optInK_1;
    c.prevMA = this.prevMA;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function emaStepImpl(sp: EmaStream, inReal: number): void {
  if (sp.optInTimePeriod === 1) {
    sp.cur_outReal = inReal;
    return;
  }
  sp.prevMA = fma(sp.emaBeta, sp.prevMA, sp.optInK_1 * inReal);
  sp.cur_outReal = sp.prevMA;
}

function emaOpenImpl(
  sp: EmaStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let emaBeta = 0;
  let optInK_1 = 0;
  let tempReal = 0;
  let prevMA = 0;
  let i = 0;
  let today = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (optInTimePeriod === 1) {
    let fillLb = emaLookback(optInTimePeriod, core);
    if (startIdx > fillLb) {
      fillLb = startIdx;
    }
    if (historyLen < fillLb + 1) {
      return RetCode.INSUFFICIENT_HISTORY;
    }
    sp.optInTimePeriod = optInTimePeriod;
    sp.emaBeta = 0;
    sp.optInK_1 = 0;
    sp.prevMA = 0;
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
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = emaLookback(optInTimePeriod, core);
  /* After the lookback call: a double live across a call is saved and
   * restored around every fma call of the loops below, one more instruction
   * per bar.
   *
   * emaBeta + optInK_1 must be exactly 1.0, or a flat input drifts off
   * its level. Each subtraction is exact only from an operand in
   * [0.5,1): at a period of 2 that is optInK_1, above it emaBeta.
   *
   * Above it emaBeta stays as the divide wrote it: the second subtraction
   * would not change a bit, and a register last written by a subtraction
   * costs each FMA reading it one more cycle on Intel P-cores.
   */
  emaBeta = (optInTimePeriod - 1) / (optInTimePeriod + 1);
  optInK_1 = 1.0 - emaBeta;
  if (emaBeta < 0.5) {
    emaBeta = 1.0 - optInK_1;
  }
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
  outBegIdx.value = startIdx;
  /* Do the EMA calculation using tight loops. */
  today = startIdx - lookbackTotal;
  i = optInTimePeriod;
  tempReal = 0.0;
  while (i-- > 0) {
    tempReal += inReal[today++];
  }
  prevMA = tempReal / optInTimePeriod;
  while (today <= startIdx) {
    prevMA = fma(emaBeta, prevMA, optInK_1 * inReal[today++]);
  }
  outReal[((0 * outStride) | 0)] = prevMA;
  outIdx = 1;
  while (today <= endIdx) {
    prevMA = fma(emaBeta, prevMA, optInK_1 * inReal[today++]);
    outReal[((outIdx++ * outStride) | 0)] = prevMA;
  }
  outNBElement.value = outIdx;
  sp.optInTimePeriod = optInTimePeriod;
  sp.emaBeta = emaBeta;
  sp.optInK_1 = optInK_1;
  sp.prevMA = prevMA;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function emaOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): EmaStream {
  const sp = new EmaStream(core);
  const retCode = emaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('EMA openAndFill', inReal.length, startIdx, emaLookback(optInTimePeriod, core));
  }
  throw streamFailure('EMA openAndFill', retCode);
}

/** @internal */
export function emaOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): EmaStream {
  const sp = new EmaStream(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = emaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('EMA open', inReal.length, startIdx, emaLookback(optInTimePeriod, core));
  }
  throw streamFailure('EMA open', retCode);
}

/**
 * Opens a live EMA stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link ema} there.
 *
 * @param inReal - Warm-up history. price/data series to smooth.
 * @param optInTimePeriod - Number of bars in the average; sets smoothing k =
 *   2/(period+1) (default 30; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `emaLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32 or a
 *   parameter outside its range.
 *
 * @example
 * ```ts
 * import { emaOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = emaOpen(data, 30);
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function emaOpen(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): EmaStream {
  optInTimePeriod = requireInt32('EMA open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('EMA open', 'inReal', inReal);
  requireHistory('EMA open', inReal.length);
  return emaOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/**
 * {@link emaOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link ema}. The range written is the stream's
 * `outRange()`.
 *
 * @param inReal - Warm-up history. price/data series to smooth.
 * @param optInTimePeriod - Number of bars in the average; sets smoothing k =
 *   2/(period+1) (default 30; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param outReal - the exponential moving average. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `emaLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or an output too short or starting where another
 *   output or a `Float64Array` input starts, or, on a history long enough to open,
 *   one sharing memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { emaOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const stream = emaOpenAndFill(data, 30, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function emaOpenAndFill(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): EmaStream {
  optInTimePeriod = requireInt32('EMA openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('EMA openAndFill', 'inReal', inReal);
  requireHistory('EMA openAndFill', inReal.length);
  const guardOutLen = openFillCount('EMA openAndFill', inReal.length, emaLookback(optInTimePeriod, core));
  requireLength('EMA openAndFill', 'outReal', outReal, guardOutLen);
  const inRealF64 = float64(inReal);
  if ((outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset)) {
    throw streamFailure('EMA openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inRealF64))) {
    throw streamFailure('EMA openAndFill', RetCode.BAD_PARAM);
  }
  return emaOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/** @internal */
export function emaStepTape(sp: EmaStream, _tape: Float64Array, _tapeBase: number, _tapeMask: number, inReal: number): number {
  emaStepImpl(sp, inReal);
  sp.outRangeCount++;
  return sp.cur_outReal;
}

/** @internal */
export function emaPeekTape(sp: EmaStream, _tape: Float64Array, _tapeBase: number, _tapeMask: number, inReal: number): number {
  return sp.peek(inReal);
}

/** @internal */
export function emaTapeDetach(_sp: EmaStream): number {
  return 0;
}
