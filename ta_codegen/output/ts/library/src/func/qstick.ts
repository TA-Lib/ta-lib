/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin (@kevinlincg)
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  081226 KL   Initial version.
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link qstick} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Number of bars averaged. Default 10, matching Tulip
 *   Indicators and pandas-ta-classic. Other packages differ: TraderEvolution
 *   documents 1, and AmiBroker community code commonly uses 8 (default 10; range 1
 *   to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function qstickLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 10;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  return optInTimePeriod - 1;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link qstick}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Number of bars averaged. Default 10, matching Tulip
 *   Indicators and pandas-ta-classic. Other packages differ: TraderEvolution
 *   documents 1, and AmiBroker community code commonly uses 8 (default 10; range 1
 *   to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function qstickDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (qstickLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function qstickImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inOpen: Float64Array,
  inClose: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let periodTotal = 0;
  let tempReal = 0;
  let i = 0;
  let outIdx = 0;
  let trailingIdx = 0;
  let lookbackTotal = 0;
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
  /* Qstick (Chande & Kroll, The New Technical Trader, 1994): a simple moving
   * average of the candle body, close minus open. Above zero means bodies
   * were predominantly bullish over the window; the zero crossings are the
   * signal.
   *
   * This is ta_codegen/input/sma/sma.c with inReal[x] replaced by
   * (inClose[x] - inOpen[x]), and deliberately nothing else: the same
   * running-sum order, the same read-before-write of the trailing term, and
   * the same divide by the period. Keeping the arithmetic identical is what
   * makes the composed reference in test_composite.c -- TA_SUB followed by
   * TA_SMA -- bit-exact rather than merely close, so any future drift in
   * either path is a hard failure instead of a tolerance argument.
   *
   * In particular the last statement divides; it does NOT multiply by a
   * precomputed 1/period. Tulip's qstick.c multiplies, which costs it up to
   * one ULP against TA_SMA. Dividing buys the memcmp differential, which is
   * the stronger of the two gates.
   */
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = (optInTimePeriod - 1);
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
  /* Do the MA calculation using tight loops. */
  /* Add-up the initial period, except for the last value. */
  periodTotal = 0.0;
  trailingIdx = startIdx - lookbackTotal;
  i = trailingIdx;
  if (optInTimePeriod > 1) {
    while (i < startIdx) {
      periodTotal += (inClose[i] - inOpen[i]);
      i = i + 1;
    }
  }
  /* Proceed with the calculation for the requested range.
   * Note that this algorithm allows outReal to be the same
   * buffer as either input.
   */
  outIdx = 0;
  while (i <= endIdx) {
    periodTotal += (inClose[i] - inOpen[i]);
    i = i + 1;
    tempReal = periodTotal;
    periodTotal -= (inClose[trailingIdx] - inOpen[trailingIdx]);
    trailingIdx = trailingIdx + 1;
    outReal[outIdx] = tempReal / optInTimePeriod;
    outIdx = outIdx + 1;
  }
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/**
 * Tushar Chande and Stanley Kroll's Qstick (*The New Technical Trader*, 1994): a
 * simple moving average of the candle body, close minus open. It measures how
 * bullish or bearish the bodies have been over the window, independently of the
 * wicks — above zero the bodies closed up on balance, below zero they closed down,
 * and the zero-line crossings are the signal.
 *
 * Formula and more info at
 * [ta-lib.org/functions/qstick](https://ta-lib.org/functions/qstick).
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link qstickLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inOpen - Open price of each bar.
 * @param inClose - Close price of each bar.
 * @param optInTimePeriod - Number of bars averaged. Default 10, matching Tulip
 *   Indicators and pandas-ta-classic. Other packages differ: TraderEvolution
 *   documents 1, and AmiBroker community code commonly uses 8 (default 10; range 1
 *   to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Average candle body over the window. Must hold the values
 *   produced and never be empty; the input length always suffices.
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
 * import { qstick } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const out = new Float64Array(open.length);
 * const { begIdx, count } = qstick(0, open.length - 1, open, close, 10, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`cmo`](https://ta-lib.org/functions/cmo)
 * @see [`imi`](https://ta-lib.org/functions/imi)
 * @see [`mom`](https://ta-lib.org/functions/mom)
 * @see [`sma`](https://ta-lib.org/functions/sma)
 */
export function qstick(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('QSTICK', 'startIdx', startIdx);
  endIdx = requireInt32('QSTICK', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('QSTICK', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('QSTICK', startIdx, endIdx);
  const guardStart = clampedStart('QSTICK', startIdx, qstickLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('QSTICK', 'inOpen', inOpen, guardInLen);
  requireLength('QSTICK', 'inClose', inClose, guardInLen);
  requireOutput('QSTICK', 'outReal', outReal, guardOutLen, Float64Array);
  const inOpenF64 = float64(inOpen);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outReal, inOpenF64) || overlapsPartly(outReal, inCloseF64)) {
    throw failure('QSTICK', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = qstickImpl(core, startIdx, endIdx, inOpenF64, inCloseF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('QSTICK', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live QSTICK stream: one value per closed bar, bit-identical to {@link qstick}
 * over the same series. Open it with {@link qstickOpen} or
 * {@link qstickOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/qstick](https://ta-lib.org/functions/qstick).
 */
export class QstickStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  periodTotal: number = 0;
  /** @internal */
  ringPos_trailingIdx: number = 0;
  /** @internal */
  ringCap_trailingIdx: number = 0;
  /** @internal */
  ring_trailingIdx_derived: Float64Array = new Float64Array(1);
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
  static of(core: Core): QstickStream {
    return new QstickStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link qstick} reports them over the same bars. Each accepted `update` and each
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
      throw failure('QSTICK advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inOpen - The bar's `inOpen` value: Open price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inOpen: number, inClose: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('QSTICK update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inOpen) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('QSTICK update', !Number.isFinite(inOpen) ? 'inOpen' : 'inClose');
    }
    qstickStepImpl(this, inOpen, inClose);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inOpen - The bar's `inOpen` value: Open price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inOpen: number, inClose: number): number {
    if (!Number.isFinite(inOpen) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('QSTICK peek', !Number.isFinite(inOpen) ? 'inOpen' : 'inClose');
    }
    const sp = this;
    let tempReal = 0;
    let cur_outReal = 0;
    let periodTotal = sp.periodTotal;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    if (sp.ringCap_trailingIdx === 0) {
      pkSlot0 = 0;
      pkVal0 = (inClose - inOpen);
    }
    periodTotal += (inClose - inOpen);
    tempReal = periodTotal;
    periodTotal -= (sp.ringPos_trailingIdx !== pkSlot0) ? sp.ring_trailingIdx_derived[sp.ringPos_trailingIdx] : pkVal0;
    cur_outReal = tempReal / sp.optInTimePeriod;
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
  clone(): QstickStream {
    const c = new QstickStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.periodTotal = this.periodTotal;
    c.ringPos_trailingIdx = this.ringPos_trailingIdx;
    c.ringCap_trailingIdx = this.ringCap_trailingIdx;
    c.ring_trailingIdx_derived = this.ring_trailingIdx_derived.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function qstickStepImpl(sp: QstickStream, inOpen: number, inClose: number): void {
  let tempReal = 0;
  if (sp.ringCap_trailingIdx === 0) {
    sp.ring_trailingIdx_derived[0] = (inClose - inOpen);
  }
  sp.periodTotal += (inClose - inOpen);
  tempReal = sp.periodTotal;
  sp.periodTotal -= sp.ring_trailingIdx_derived[sp.ringPos_trailingIdx];
  sp.cur_outReal = tempReal / sp.optInTimePeriod;
  sp.ring_trailingIdx_derived[sp.ringPos_trailingIdx] = (inClose - inOpen);
  sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
  if (sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx) {
    sp.ringPos_trailingIdx = 0;
  }
}

function qstickOpenImpl(
  sp: QstickStream,
  inOpen: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let periodTotal = 0;
  let tempReal = 0;
  let i = 0;
  let outIdx = 0;
  let trailingIdx = 0;
  let lookbackTotal = 0;
  const historyLen = inOpen.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inClose.length !== inOpen.length) {
    return RetCode.BAD_PARAM;
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
  /* Qstick (Chande & Kroll, The New Technical Trader, 1994): a simple moving
   * average of the candle body, close minus open. Above zero means bodies
   * were predominantly bullish over the window; the zero crossings are the
   * signal.
   *
   * This is ta_codegen/input/sma/sma.c with inReal[x] replaced by
   * (inClose[x] - inOpen[x]), and deliberately nothing else: the same
   * running-sum order, the same read-before-write of the trailing term, and
   * the same divide by the period. Keeping the arithmetic identical is what
   * makes the composed reference in test_composite.c -- TA_SUB followed by
   * TA_SMA -- bit-exact rather than merely close, so any future drift in
   * either path is a hard failure instead of a tolerance argument.
   *
   * In particular the last statement divides; it does NOT multiply by a
   * precomputed 1/period. Tulip's qstick.c multiplies, which costs it up to
   * one ULP against TA_SMA. Dividing buys the memcmp differential, which is
   * the stronger of the two gates.
   */
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = (optInTimePeriod - 1);
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
  /* Do the MA calculation using tight loops. */
  /* Add-up the initial period, except for the last value. */
  periodTotal = 0.0;
  trailingIdx = startIdx - lookbackTotal;
  i = trailingIdx;
  if (optInTimePeriod > 1) {
    while (i < startIdx) {
      periodTotal += (inClose[i] - inOpen[i]);
      i = i + 1;
    }
  }
  /* Proceed with the calculation for the requested range.
   * Note that this algorithm allows outReal to be the same
   * buffer as either input.
   */
  outIdx = 0;
  while (i <= endIdx) {
    periodTotal += (inClose[i] - inOpen[i]);
    i = i + 1;
    tempReal = periodTotal;
    periodTotal -= (inClose[trailingIdx] - inOpen[trailingIdx]);
    trailingIdx = trailingIdx + 1;
    outReal[((outIdx * outStride) | 0)] = tempReal / optInTimePeriod;
    outIdx = outIdx + 1;
  }
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const cap_trailingIdx = i - trailingIdx;
  if (cap_trailingIdx < 0 || cap_trailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_trailingIdx = cap_trailingIdx > 0 ? cap_trailingIdx : 1;
  const capRing_trailingIdx_derived = new Float64Array(allocN_trailingIdx);
  for (let fillJ = historyLen - cap_trailingIdx; fillJ < historyLen; fillJ++) {
    capRing_trailingIdx_derived[fillJ - (historyLen - cap_trailingIdx)] = (inClose[fillJ] - inOpen[fillJ]);
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.periodTotal = periodTotal;
  sp.ringPos_trailingIdx = 0;
  sp.ringCap_trailingIdx = cap_trailingIdx;
  sp.ring_trailingIdx_derived = capRing_trailingIdx_derived;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function qstickOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): QstickStream {
  const sp = QstickStream.of(core);
  const retCode = qstickOpenImpl(sp, inOpen, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('QSTICK openAndFill', inOpen.length, startIdx, qstickLookback(optInTimePeriod, core));
  }
  throw streamFailure('QSTICK openAndFill', retCode);
}

/** @internal */
export function qstickOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): QstickStream {
  const sp = QstickStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = qstickOpenImpl(sp, inOpen, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('QSTICK open', inOpen.length, startIdx, qstickLookback(optInTimePeriod, core));
  }
  throw streamFailure('QSTICK open', retCode);
}

/**
 * Opens a live QSTICK stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link qstick} there.
 *
 * @param inOpen - Warm-up history. Open price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Number of bars averaged. Default 10, matching Tulip
 *   Indicators and pandas-ta-classic. Other packages differ: TraderEvolution
 *   documents 1, and AmiBroker community code commonly uses 8 (default 10; range 1
 *   to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `qstickLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for an integer parameter that is not an int32, a parameter outside its range or
 *   histories of different lengths.
 *
 * @example
 * ```ts
 * import { qstickOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = qstickOpen(open, close, 10);
 * const peeked = stream.peek(100.2, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 100.9); // the bar closes
 * ```
 */
export function qstickOpen(
  inOpen: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): QstickStream {
  optInTimePeriod = requireInt32('QSTICK open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('QSTICK open', 'inOpen', inOpen);
  requireHistory('QSTICK open', inOpen.length);
  requireArgument('QSTICK open', 'inClose', inClose);
  requireHistoryLength('QSTICK open', 'inClose', inClose.length, inOpen.length);
  return qstickOpenInternal(core, float64(inOpen), float64(inClose), 0, optInTimePeriod);
}

/**
 * {@link qstickOpen} that also fills the outputs over the whole history in the
 * same pass, bit-identically to {@link qstick}. The range written is the stream's
 * `outRange()`.
 *
 * @param inOpen - Warm-up history. Open price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Number of bars averaged. Default 10, matching Tulip
 *   Indicators and pandas-ta-classic. Other packages differ: TraderEvolution
 *   documents 1, and AmiBroker community code commonly uses 8 (default 10; range 1
 *   to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Average candle body over the window. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `qstickLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or an output too short or starting where another
 *   output or a `Float64Array` input starts, or, on a history long enough to open,
 *   one sharing memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { qstickOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const out = new Float64Array(open.length);
 * const stream = qstickOpenAndFill(open, close, 10, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function qstickOpenAndFill(
  inOpen: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): QstickStream {
  optInTimePeriod = requireInt32('QSTICK openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('QSTICK openAndFill', 'inOpen', inOpen);
  requireHistory('QSTICK openAndFill', inOpen.length);
  requireArgument('QSTICK openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('QSTICK openAndFill', inOpen.length, qstickLookback(optInTimePeriod, core));
  requireHistoryLength('QSTICK openAndFill', 'inClose', inClose.length, inOpen.length);
  requireOutput('QSTICK openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inOpenF64 = float64(inOpen);
  const inCloseF64 = float64(inClose);
  if ((outReal.buffer === inOpenF64.buffer && outReal.byteOffset === inOpenF64.byteOffset) || (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset)) {
    throw streamFailure('QSTICK openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inOpenF64) || overlaps(outReal, inCloseF64))) {
    throw streamFailure('QSTICK openAndFill', RetCode.BAD_PARAM);
  }
  return qstickOpenAndFillInternal(core, inOpenF64, inCloseF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}
