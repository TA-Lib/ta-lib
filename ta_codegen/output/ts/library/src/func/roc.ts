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
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function rocLookbackPositional(optInTimePeriod: number, _core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 10;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  return optInTimePeriod;
}

/** @internal */
export function rocDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (rocLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function rocImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let inIdx = 0;
  let outIdx = 0;
  let trailingIdx = 0;
  let tempReal = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 10;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  /* The interpretation of the rate of change varies widely depending
   * which software and/or books you are refering to.
   *
   * The following is the table of Rate-Of-Change implemented in TA-LIB:
   *       MOM     = (price - prevPrice)         [Momentum]
   *       ROC     = ((price/prevPrice)-1)*100   [Rate of change]
   *       ROCP    = (price-prevPrice)/prevPrice [Rate of change Percentage]
   *       ROCR    = (price/prevPrice)           [Rate of change ratio]
   *       ROCR100 = (price/prevPrice)*100       [Rate of change ratio 100 Scale]
   *
   * Here are the equivalent function in other software:
   *       TA-Lib  |   Tradestation   |    Metastock
   *       =================================================
   *       MOM     |   Momentum       |    ROC (Point)
   *       ROC     |   ROC            |    ROC (Percent)
   *       ROCP    |   PercentChange  |    -
   *       ROCR    |   -              |    -
   *       ROCR100 |   -              |    MO
   *
   * The MOM function is the only one who is not normalized, and thus
   * should be avoided for comparing different time serie of prices.
   *
   * ROC and ROCP are centered at zero and can have positive and negative
   * value. Here are some equivalence:
   *    ROC = ROCP/100
   *        = ((price-prevPrice)/prevPrice)/100
   *        = ((price/prevPrice)-1)*100
   *
   * ROCR and ROCR100 are ratio respectively centered at 1 and 100 and are
   * always positive values.
   */
  /* Move up the start index if there is not
   * enough initial data.
   */
  if (startIdx < optInTimePeriod) {
    startIdx = optInTimePeriod;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  /* Calculate Rate of change: ((price / prevPrice)-1)*100 */
  outIdx = 0;
  inIdx = startIdx;
  trailingIdx = startIdx - optInTimePeriod;
  while (inIdx <= endIdx) {
    tempReal = inReal[trailingIdx++];
    if (tempReal !== 0.0) {
      outReal[outIdx++] = (inReal[inIdx] / tempReal - 1.0) * 100.0;
    } else {
      outReal[outIdx++] = 0.0;
    }
    inIdx += 1;
  }
  /* Set output limits. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000']] as const;

/** @internal */
export function rocPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('ROC', 'startIdx', startIdx);
  endIdx = requireInt32('ROC', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('ROC', 'timePeriod', optInTimePeriod);
  requireIndexRange('ROC', startIdx, endIdx);
  const guardLookback = rocLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('ROC', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('ROC', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('ROC', 'real', inReal, guardInLen);
  requireOutput('ROC', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('ROC', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = rocImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('ROC', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link roc}, {@link rocLookback}, {@link rocDisplayShift},
 * {@link rocOpen} and {@link rocOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface RocOptions {
  /**
   * Lookback distance to the prior price (default 10; range 1 to 100000;
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

/** What {@link roc} returns, and {@link rocOpenAndFill} with its stream. */
export interface RocResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /** Percent rate of change. `values[k]` is the value at input bar `begIdx + k`. */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link roc} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link RocOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function rocLookback(options?: RocOptions): number {
  const o = optionsOf('ROC lookback', options);
  return rocLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('ROC lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link roc}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link RocOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function rocDisplayShift(outputIdx: number, options?: RocOptions): number {
  const o = optionsOf('ROC displayShift', options);
  return rocDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('ROC displayShift', o.core));
}

/**
 * Rate-of-change momentum oscillator: the percent change of price versus the price
 * timePeriod bars earlier. Centered at zero with positive and negative values.
 * Positive when price rose over the period, negative when it fell; magnitude
 * scales the move.
 *
 * Formula and more info at
 * [ta-lib.org/functions/roc](https://ta-lib.org/functions/roc).
 *
 * **Notes**
 * - **The input is assumed positive.** The price `timePeriod` bars back is the
 *   divisor. Where it is exactly 0 the output is 0. Near zero the divisor is small
 *   and the output very large; no error is returned. A series that reaches or
 *   crosses zero, such as an oscillator or a spread, has no meaningful result
 *   here.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link rocLookback} is a success with empty outputs.
 *
 * @param real - Input price series.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link RocOptions}.
 * @returns `begIdx` and the values; see {@link RocResult}. Each output is a view
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
 * import { roc } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = roc(data, { timePeriod: 10 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`mom`](https://ta-lib.org/functions/mom)
 * @see [`rocp`](https://ta-lib.org/functions/rocp)
 * @see [`rocr`](https://ta-lib.org/functions/rocr)
 * @see [`rocr100`](https://ta-lib.org/functions/rocr100)
 */
export function roc(real: ArrayLike<number>, options?: RocOptions): RocResult {
  const o = optionsOf('ROC', options);
  if (o.endIdx === undefined) {
    requireSeries('ROC', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('ROC', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, rocLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = rocPositional(startIdx, endIdx, real, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live ROC stream: one value per closed bar, bit-identical to {@link roc} over
 * the same series. Open it with {@link rocOpen} or {@link rocOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/roc](https://ta-lib.org/functions/roc).
 */
export class RocStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
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
  static of(core: Core): RocStream {
    return new RocStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link roc} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('ROC advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: Input price series.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('ROC update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('ROC update', 'real');
    }
    rocStepImpl(this, real);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: Input price series.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number): number {
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('ROC peek', 'real');
    }
    const sp = this;
    const inReal = real;
    let tempReal = 0;
    let cur_outReal = 0;
    tempReal = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
    if (tempReal !== 0.0) {
      cur_outReal = (inReal / tempReal - 1.0) * 100.0;
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
  clone(): RocStream {
    const c = new RocStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.ringPos_trailingIdx = this.ringPos_trailingIdx;
    c.ringCap_trailingIdx = this.ringCap_trailingIdx;
    c.ring_trailingIdx_inReal = this.ring_trailingIdx_inReal.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function rocStepImpl(sp: RocStream, inReal: number): void {
  let tempReal = 0;
  let ringCapL_trailingIdx = 0;
  tempReal = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
  if (tempReal !== 0.0) {
    sp.cur_outReal = (inReal / tempReal - 1.0) * 100.0;
  } else {
    sp.cur_outReal = 0.0;
  }
  ringCapL_trailingIdx = sp.ringCap_trailingIdx;
  sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
  sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
  if (sp.ringPos_trailingIdx >= ringCapL_trailingIdx) {
    sp.ringPos_trailingIdx = 0;
  }
}

function rocOpenImpl(
  sp: RocStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let inIdx = 0;
  let outIdx = 0;
  let trailingIdx = 0;
  let tempReal = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 10;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* The interpretation of the rate of change varies widely depending
   * which software and/or books you are refering to.
   *
   * The following is the table of Rate-Of-Change implemented in TA-LIB:
   *       MOM     = (price - prevPrice)         [Momentum]
   *       ROC     = ((price/prevPrice)-1)*100   [Rate of change]
   *       ROCP    = (price-prevPrice)/prevPrice [Rate of change Percentage]
   *       ROCR    = (price/prevPrice)           [Rate of change ratio]
   *       ROCR100 = (price/prevPrice)*100       [Rate of change ratio 100 Scale]
   *
   * Here are the equivalent function in other software:
   *       TA-Lib  |   Tradestation   |    Metastock
   *       =================================================
   *       MOM     |   Momentum       |    ROC (Point)
   *       ROC     |   ROC            |    ROC (Percent)
   *       ROCP    |   PercentChange  |    -
   *       ROCR    |   -              |    -
   *       ROCR100 |   -              |    MO
   *
   * The MOM function is the only one who is not normalized, and thus
   * should be avoided for comparing different time serie of prices.
   *
   * ROC and ROCP are centered at zero and can have positive and negative
   * value. Here are some equivalence:
   *    ROC = ROCP/100
   *        = ((price-prevPrice)/prevPrice)/100
   *        = ((price/prevPrice)-1)*100
   *
   * ROCR and ROCR100 are ratio respectively centered at 1 and 100 and are
   * always positive values.
   */
  /* Move up the start index if there is not
   * enough initial data.
   */
  if (startIdx < optInTimePeriod) {
    startIdx = optInTimePeriod;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Calculate Rate of change: ((price / prevPrice)-1)*100 */
  outIdx = 0;
  inIdx = startIdx;
  trailingIdx = startIdx - optInTimePeriod;
  while (inIdx <= endIdx) {
    tempReal = inReal[trailingIdx++];
    if (tempReal !== 0.0) {
      outReal[((outIdx++ * outStride) | 0)] = (inReal[inIdx] / tempReal - 1.0) * 100.0;
    } else {
      outReal[((outIdx++ * outStride) | 0)] = 0.0;
    }
    inIdx += 1;
  }
  /* Set output limits. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const cap_trailingIdx = inIdx - trailingIdx;
  if (cap_trailingIdx < 1 || cap_trailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_trailingIdx = cap_trailingIdx > 0 ? cap_trailingIdx : 1;
  const capRing_trailingIdx_inReal = new Float64Array(allocN_trailingIdx);
  capRing_trailingIdx_inReal.set(inReal.subarray(historyLen - cap_trailingIdx, historyLen), 0);
  sp.optInTimePeriod = optInTimePeriod;
  sp.ringPos_trailingIdx = 0;
  sp.ringCap_trailingIdx = cap_trailingIdx;
  sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function rocOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RocStream {
  const sp = RocStream.of(core);
  const retCode = rocOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ROC openAndFill', inReal.length, startIdx, rocLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && rocLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('ROC openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('ROC openAndFill', retCode);
}

/** @internal */
export function rocOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): RocStream {
  const sp = RocStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = rocOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ROC open', inReal.length, startIdx, rocLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && rocLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('ROC open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('ROC open', retCode);
}

/** @internal */
export function rocOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): RocStream {
  optInTimePeriod = requireInt32('ROC open', 'timePeriod', optInTimePeriod);
  requireArgument('ROC open', 'real', inReal);
  requireHistory('ROC open', 'real', inReal.length);
  return rocOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/** @internal */
export function rocOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): RocStream {
  optInTimePeriod = requireInt32('ROC openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('ROC openAndFill', 'real', inReal);
  requireHistory('ROC openAndFill', 'real', inReal.length);
  const guardLookback = rocLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('ROC openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('ROC openAndFill', inReal.length, guardLookback);
  requireOutput('ROC openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('ROC openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('ROC openAndFill', 'outReal', 'real');
  }
  return rocOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live ROC stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link roc} there.
 *
 * @param real - Warm-up history. Input price series.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link RocOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `rocLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { rocOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = rocOpen(data, { timePeriod: 10 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function rocOpen(real: ArrayLike<number>, options?: RocOptions): RocStream {
  const o = optionsOf('ROC open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('ROC open', o.core);
  return rocOpenPositional(real, optInTimePeriod, core);
}

/**
 * {@link rocOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link roc}.
 *
 * @param real - Warm-up history. Input price series.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link RocOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link roc} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `rocLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or a
 *   buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { rocOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = rocOpenAndFill(data, { timePeriod: 10 });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function rocOpenAndFill(real: ArrayLike<number>, options?: RocOptions): RocResult & { stream: RocStream } {
  const o = optionsOf('ROC openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('ROC openAndFill', o.core);
  const capacity = fillCapacity(rocLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = rocOpenAndFillPositional(real, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
