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
import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function emaLookbackPositional(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
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

/** @internal */
export function emaDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (emaLookbackPositional(optInTimePeriod, core) < 0) {
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
  lookbackTotal = emaLookbackPositional(optInTimePeriod, core);
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

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000']] as const;

/** @internal */
export function emaPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('EMA', 'startIdx', startIdx);
  endIdx = requireInt32('EMA', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('EMA', 'timePeriod', optInTimePeriod);
  requireIndexRange('EMA', startIdx, endIdx);
  const guardLookback = emaLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('EMA', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('EMA', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('EMA', 'real', inReal, guardInLen);
  requireOutput('EMA', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('EMA', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = emaImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('EMA', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link ema}, {@link emaLookback}, {@link emaDisplayShift},
 * {@link emaOpen} and {@link emaOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface EmaOptions {
  /**
   * Number of bars in the average; sets smoothing k = 2/(period+1) (default 30;
   * range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
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
  outReal?: Float64Array<ArrayBuffer> | undefined;
}

/** What {@link ema} returns, and {@link emaOpenAndFill} with its stream. */
export interface EmaResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * the exponential moving average. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link ema} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link EmaOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function emaLookback(options?: EmaOptions): number {
  const o = optionsOf('EMA lookback', options);
  return emaLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('EMA lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link ema}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link EmaOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function emaDisplayShift(outputIdx: number, options?: EmaOptions): number {
  const o = optionsOf('EMA displayShift', options);
  return emaDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('EMA displayShift', o.core));
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
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link emaLookback} is a success with empty outputs.
 *
 * @param real - price/data series to smooth.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link EmaOptions}.
 * @returns `begIdx` and the values; see {@link EmaResult}. Each output is a view
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
 * import { ema } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = ema(data, { timePeriod: 30 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`sma`](https://ta-lib.org/functions/sma)
 * @see [`dema`](https://ta-lib.org/functions/dema)
 * @see [`tema`](https://ta-lib.org/functions/tema)
 * @see [`ma`](https://ta-lib.org/functions/ma)
 * @see [`macd`](https://ta-lib.org/functions/macd)
 * @see [`t3`](https://ta-lib.org/functions/t3)
 */
export function ema(real: ArrayLike<number>, options?: EmaOptions): EmaResult {
  const o = optionsOf('EMA', options);
  if (o.endIdx === undefined) {
    requireSeries('EMA', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('EMA', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, emaLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = emaPositional(startIdx, endIdx, real, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
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

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): EmaStream {
    return new EmaStream(core);
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
      throw streamFailure('EMA advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: price/data series to smooth.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('EMA update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('EMA update', 'real');
    }
    emaStepImpl(this, real);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: price/data series to smooth.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number): number {
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('EMA peek', 'real');
    }
    const sp = this;
    const inReal = real;
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
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (optInTimePeriod === 1) {
    let fillLb = emaLookbackPositional(optInTimePeriod, core);
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
  lookbackTotal = emaLookbackPositional(optInTimePeriod, core);
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
  const sp = EmaStream.of(core);
  const retCode = emaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('EMA openAndFill', inReal.length, startIdx, emaLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && emaLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('EMA openAndFill', PARAM_RANGES, optInTimePeriod);
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
  const sp = EmaStream.of(core);
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
    throw insufficientHistory('EMA open', inReal.length, startIdx, emaLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && emaLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('EMA open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('EMA open', retCode);
}

/** @internal */
export function emaOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): EmaStream {
  optInTimePeriod = requireInt32('EMA open', 'timePeriod', optInTimePeriod);
  requireArgument('EMA open', 'real', inReal);
  requireHistory('EMA open', 'real', inReal.length);
  return emaOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/** @internal */
export function emaOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): EmaStream {
  optInTimePeriod = requireInt32('EMA openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('EMA openAndFill', 'real', inReal);
  requireHistory('EMA openAndFill', 'real', inReal.length);
  const guardLookback = emaLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('EMA openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('EMA openAndFill', inReal.length, guardLookback);
  requireOutput('EMA openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('EMA openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('EMA openAndFill', 'outReal', 'real');
  }
  return emaOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live EMA stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link ema} there.
 *
 * @param real - Warm-up history. price/data series to smooth.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link EmaOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `emaLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { emaOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = emaOpen(data, { timePeriod: 30 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function emaOpen(real: ArrayLike<number>, options?: EmaOptions): EmaStream {
  const o = optionsOf('EMA open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('EMA open', o.core);
  return emaOpenPositional(real, optInTimePeriod, core);
}

/**
 * {@link emaOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link ema}.
 *
 * @param real - Warm-up history. price/data series to smooth.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link EmaOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link ema} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `emaLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or a
 *   buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { emaOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = emaOpenAndFill(data, { timePeriod: 30 });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function emaOpenAndFill(real: ArrayLike<number>, options?: EmaOptions): EmaResult & { stream: EmaStream } {
  const o = optionsOf('EMA openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('EMA openAndFill', o.core);
  const capacity = fillCapacity(emaLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = emaOpenAndFillPositional(real, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
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
