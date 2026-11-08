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

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function qstickLookbackPositional(optInTimePeriod: number, _core: Core = Core.DEFAULT): number {
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

/** @internal */
export function qstickDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (qstickLookbackPositional(optInTimePeriod, core) < 0) {
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

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000']] as const;

/** @internal */
export function qstickPositional(
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
  optInTimePeriod = requireInt32('QSTICK', 'timePeriod', optInTimePeriod);
  requireIndexRange('QSTICK', startIdx, endIdx);
  const guardLookback = qstickLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('QSTICK', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('QSTICK', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('QSTICK', 'prices.open', inOpen, guardInLen);
  requireLength('QSTICK', 'prices.close', inClose, guardInLen);
  requireOutput('QSTICK', 'outReal', outReal, guardOutLen, Float64Array);
  const inOpenF64 = float64(inOpen);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outReal, inOpenF64)) {
    throw sharedMemory('QSTICK', 'outReal', 'prices.open');
  }
  if (overlapsPartly(outReal, inCloseF64)) {
    throw sharedMemory('QSTICK', 'outReal', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = qstickImpl(core, startIdx, endIdx, inOpenF64, inCloseF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('QSTICK', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link qstick}, {@link qstickLookback}, {@link qstickDisplayShift},
 * {@link qstickOpen} and {@link qstickOpenAndFill}. Every key is optional: an
 * omitted or `undefined` key takes its default, and a call ignores the keys it has
 * no use for.
 */
export interface QstickOptions {
  /**
   * Number of bars averaged. Default 10, matching Tulip Indicators and
   * pandas-ta-classic. Other packages differ: TraderEvolution documents 1, and
   * AmiBroker community code commonly uses 8 (default 10; range 1 to 100000;
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
  outReal?: Float64Array<ArrayBuffer> | undefined;
}

/** What {@link qstick} returns, and {@link qstickOpenAndFill} with its stream. */
export interface QstickResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Average candle body over the window. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link qstick} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link QstickOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function qstickLookback(options?: QstickOptions): number {
  const o = optionsOf('QSTICK lookback', options);
  return qstickLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('QSTICK lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link qstick}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link QstickOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function qstickDisplayShift(outputIdx: number, options?: QstickOptions): number {
  const o = optionsOf('QSTICK displayShift', options);
  return qstickDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('QSTICK displayShift', o.core));
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
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link qstickLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `open`: Open price of
 *   each bar. `close`: Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link QstickOptions}.
 * @returns `begIdx` and the values; see {@link QstickResult}. Each output is a
 *   view of its buffer, the caller's or a new one.
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
 * import { qstick } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { begIdx, values } = qstick({ open, close }, { timePeriod: 10 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`cmo`](https://ta-lib.org/functions/cmo)
 * @see [`imi`](https://ta-lib.org/functions/imi)
 * @see [`mom`](https://ta-lib.org/functions/mom)
 * @see [`sma`](https://ta-lib.org/functions/sma)
 */
export function qstick(prices: { open: ArrayLike<number>; high?: ArrayLike<number> | undefined; low?: ArrayLike<number> | undefined; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: QstickOptions): QstickResult {
  requireArgument('QSTICK', 'prices', prices);
  const o = optionsOf('QSTICK', options);
  if (o.endIdx === undefined) {
    requireSeries('QSTICK', 'prices.open', prices.open);
    requireSeries('QSTICK', 'prices.close', prices.close);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('QSTICK', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, qstickLookbackPositional(optInTimePeriod, core), prices.open, prices.close);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = qstickPositional(startIdx, endIdx, prices.open, prices.close, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
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
      throw streamFailure('QSTICK advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param open - The bar's `open` value: Open price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(open: number, close: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('QSTICK update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(open) || !Number.isFinite(close)) {
      throw nonFiniteBar('QSTICK update', !Number.isFinite(open) ? 'open' : 'close');
    }
    qstickStepImpl(this, open, close);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param open - The bar's `open` value: Open price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(open: number, close: number): number {
    if (!Number.isFinite(open) || !Number.isFinite(close)) {
      throw nonFiniteBar('QSTICK peek', !Number.isFinite(open) ? 'open' : 'close');
    }
    const sp = this;
    const inOpen = open;
    const inClose = close;
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
  if (startIdx >= historyLen) {
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
    throw insufficientHistory('QSTICK openAndFill', inOpen.length, startIdx, qstickLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && qstickLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('QSTICK openAndFill', PARAM_RANGES, optInTimePeriod);
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
    throw insufficientHistory('QSTICK open', inOpen.length, startIdx, qstickLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && qstickLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('QSTICK open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('QSTICK open', retCode);
}

/** @internal */
export function qstickOpenPositional(
  inOpen: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): QstickStream {
  optInTimePeriod = requireInt32('QSTICK open', 'timePeriod', optInTimePeriod);
  requireArgument('QSTICK open', 'prices.open', inOpen);
  requireHistory('QSTICK open', 'prices.open', inOpen.length);
  requireArgument('QSTICK open', 'prices.close', inClose);
  requireHistoryLength('QSTICK open', 'prices.close', inClose.length, inOpen.length);
  return qstickOpenInternal(core, float64(inOpen), float64(inClose), 0, optInTimePeriod);
}

/** @internal */
export function qstickOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): QstickStream {
  optInTimePeriod = requireInt32('QSTICK openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('QSTICK openAndFill', 'prices.open', inOpen);
  requireHistory('QSTICK openAndFill', 'prices.open', inOpen.length);
  requireArgument('QSTICK openAndFill', 'prices.close', inClose);
  const guardLookback = qstickLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('QSTICK openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('QSTICK openAndFill', inOpen.length, guardLookback);
  requireHistoryLength('QSTICK openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('QSTICK openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inOpenF64 = float64(inOpen);
  const inCloseF64 = float64(inClose);
  if (outReal.buffer === inOpenF64.buffer && outReal.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('QSTICK openAndFill', 'outReal', 'prices.open');
  }
  if (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('QSTICK openAndFill', 'outReal', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outReal, inOpenF64)) {
    throw sharedMemory('QSTICK openAndFill', 'outReal', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outReal, inCloseF64)) {
    throw sharedMemory('QSTICK openAndFill', 'outReal', 'prices.close');
  }
  return qstickOpenAndFillInternal(core, inOpenF64, inCloseF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live QSTICK stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link qstick} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link QstickOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `qstickLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range
 *   or histories of different lengths.
 *
 * @example
 * ```ts
 * import { qstickOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = qstickOpen({ open, close }, { timePeriod: 10 });
 * const peeked = stream.peek(100.2, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 100.9); // the bar closes
 * ```
 */
export function qstickOpen(prices: { open: ArrayLike<number>; high?: ArrayLike<number> | undefined; low?: ArrayLike<number> | undefined; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: QstickOptions): QstickStream {
  requireArgument('QSTICK open', 'prices', prices);
  const o = optionsOf('QSTICK open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('QSTICK open', o.core);
  return qstickOpenPositional(prices.open, prices.close, optInTimePeriod, core);
}

/**
 * {@link qstickOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link qstick}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link QstickOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link qstick} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `qstickLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { qstickOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = qstickOpenAndFill({ open, close }, {
 *   timePeriod: 10,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function qstickOpenAndFill(prices: { open: ArrayLike<number>; high?: ArrayLike<number> | undefined; low?: ArrayLike<number> | undefined; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: QstickOptions): QstickResult & { stream: QstickStream } {
  requireArgument('QSTICK openAndFill', 'prices', prices);
  const o = optionsOf('QSTICK openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('QSTICK openAndFill', o.core);
  const capacity = fillCapacity(qstickLookbackPositional(optInTimePeriod, core), prices.open, prices.close);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = qstickOpenAndFillPositional(prices.open, prices.close, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
