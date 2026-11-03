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
 *  010802 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link rocr100} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Lookback distance (bars back) for the reference price
 *   (default 10; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function rocr100Lookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  void core;
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

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link rocr100}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Lookback distance (bars back) for the reference price
 *   (default 10; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function rocr100DisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (rocr100Lookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function rocr100Impl(
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
  /* Calculate Rate of change Ratio: (price / prevPrice) */
  outIdx = 0;
  inIdx = startIdx;
  trailingIdx = startIdx - optInTimePeriod;
  while (inIdx <= endIdx) {
    tempReal = inReal[trailingIdx++];
    if (tempReal !== 0.0) {
      outReal[outIdx++] = inReal[inIdx] / tempReal * 100.0;
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

/**
 * Rate-of-change ratio scaled by 100: current price as a percentage of the price
 * optInTimePeriod bars ago. Momentum measure centered at 100 and always positive.
 * Above 100 = price rose vs n bars ago; below 100 = price fell.
 *
 * Formula and more info at
 * [ta-lib.org/functions/rocr100](https://ta-lib.org/functions/rocr100).
 *
 * **Notes**
 * - **The input is assumed positive.** The price `optInTimePeriod` bars back is
 *   the divisor. Where it is exactly 0 the output is 0. Near zero the divisor is
 *   small and the output very large; no error is returned. A series that reaches
 *   or crosses zero, such as an oscillator or a spread, has no meaningful result
 *   here.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link rocr100Lookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - Input price/data series.
 * @param optInTimePeriod - Lookback distance (bars back) for the reference price
 *   (default 10; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Rate-of-change ratio times 100. Must hold the values produced
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
 * import { rocr100 } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const { begIdx, count } = rocr100(0, data.length - 1, data, 10, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`rocr`](https://ta-lib.org/functions/rocr)
 * @see [`roc`](https://ta-lib.org/functions/roc)
 * @see [`rocp`](https://ta-lib.org/functions/rocp)
 * @see [`mom`](https://ta-lib.org/functions/mom)
 */
export function rocr100(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('ROCR100', 'startIdx', startIdx);
  endIdx = requireInt32('ROCR100', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('ROCR100', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('ROCR100', startIdx, endIdx);
  const guardStart = clampedStart('ROCR100', startIdx, rocr100Lookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('ROCR100', 'inReal', inReal, guardInLen);
  requireLength('ROCR100', 'outReal', outReal, guardOutLen);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw failure('ROCR100', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = rocr100Impl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('ROCR100', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live ROCR100 stream: one value per closed bar, bit-identical to
 * {@link rocr100} over the same series. Open it with {@link rocr100Open} or
 * {@link rocr100OpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/rocr100](https://ta-lib.org/functions/rocr100).
 */
export class Rocr100Stream {
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

  /** @internal */
  constructor(core: Core) {
    this.core = core;
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link rocr100} reports them over the same bars. Each accepted `update` and each
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
      throw failure('ROCR100 advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: Input price/data series.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('ROCR100 update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('ROCR100 update', 'inReal');
    }
    rocr100StepImpl(this, inReal);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Input price/data series.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number): number {
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('ROCR100 peek', 'inReal');
    }
    const sp = this;
    let tempReal = 0;
    let cur_outReal = 0;
    tempReal = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
    if (tempReal !== 0.0) {
      cur_outReal = inReal / tempReal * 100.0;
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
  clone(): Rocr100Stream {
    const c = new Rocr100Stream(this.core);
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

function rocr100StepImpl(sp: Rocr100Stream, inReal: number): void {
  let tempReal = 0;
  let ringCapL_trailingIdx = 0;
  tempReal = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
  if (tempReal !== 0.0) {
    sp.cur_outReal = inReal / tempReal * 100.0;
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

function rocr100OpenImpl(
  sp: Rocr100Stream,
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
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 10;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
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
  /* Calculate Rate of change Ratio: (price / prevPrice) */
  outIdx = 0;
  inIdx = startIdx;
  trailingIdx = startIdx - optInTimePeriod;
  while (inIdx <= endIdx) {
    tempReal = inReal[trailingIdx++];
    if (tempReal !== 0.0) {
      outReal[((outIdx++ * outStride) | 0)] = inReal[inIdx] / tempReal * 100.0;
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
export function rocr100OpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): Rocr100Stream {
  const sp = new Rocr100Stream(core);
  const retCode = rocr100OpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ROCR100 openAndFill', inReal.length, startIdx, rocr100Lookback(optInTimePeriod, core));
  }
  throw streamFailure('ROCR100 openAndFill', retCode);
}

/** @internal */
export function rocr100OpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): Rocr100Stream {
  const sp = new Rocr100Stream(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = rocr100OpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ROCR100 open', inReal.length, startIdx, rocr100Lookback(optInTimePeriod, core));
  }
  throw streamFailure('ROCR100 open', retCode);
}

/**
 * Opens a live ROCR100 stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link rocr100} there.
 *
 * @param inReal - Warm-up history. Input price/data series.
 * @param optInTimePeriod - Lookback distance (bars back) for the reference price
 *   (default 10; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `rocr100Lookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32 or a
 *   parameter outside its range.
 *
 * @example
 * ```ts
 * import { rocr100Open } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = rocr100Open(data, 10);
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function rocr100Open(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): Rocr100Stream {
  optInTimePeriod = requireInt32('ROCR100 open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('ROCR100 open', 'inReal', inReal);
  requireHistory('ROCR100 open', inReal.length);
  return rocr100OpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/**
 * {@link rocr100Open} that also fills the outputs over the whole history in the
 * same pass, bit-identically to {@link rocr100}. The range written is the stream's
 * `outRange()`.
 *
 * @param inReal - Warm-up history. Input price/data series.
 * @param optInTimePeriod - Lookback distance (bars back) for the reference price
 *   (default 10; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Rate-of-change ratio times 100. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `rocr100Lookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or an output too short or starting where another
 *   output or a `Float64Array` input starts, or, on a history long enough to open,
 *   one sharing memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { rocr100OpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const stream = rocr100OpenAndFill(data, 10, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function rocr100OpenAndFill(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): Rocr100Stream {
  optInTimePeriod = requireInt32('ROCR100 openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('ROCR100 openAndFill', 'inReal', inReal);
  requireHistory('ROCR100 openAndFill', inReal.length);
  const guardOutLen = openFillCount('ROCR100 openAndFill', inReal.length, rocr100Lookback(optInTimePeriod, core));
  requireLength('ROCR100 openAndFill', 'outReal', outReal, guardOutLen);
  const inRealF64 = float64(inReal);
  if ((outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset)) {
    throw streamFailure('ROCR100 openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inRealF64))) {
    throw streamFailure('ROCR100 openAndFill', RetCode.BAD_PARAM);
  }
  return rocr100OpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}
