/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  083126 MF,CC  First version (issue #273).
 */

import { badSink, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireReal, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { AtrStream, atrLookbackPositional, atrOpenAndFillInternal, atrPositional } from './atr.ts';
import { EmaStream, emaLookbackPositional, emaOpenAndFillInternal, emaPositional } from './ema.ts';
import { TyppriceStream, typpriceOpenAndFillInternal, typpricePositional } from './typprice.ts';

/** @internal */
export function kcLookbackPositional(optInTimePeriod: number, optInATRPeriod: number, optInNbDev: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if ((optInATRPeriod | 0) !== optInATRPeriod) {
    return -1;
  }
  if (typeof optInNbDev !== 'number') {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  if (optInATRPeriod === Core.INTEGER_DEFAULT) {
    optInATRPeriod = 10;
  } else if (optInATRPeriod < 1 || optInATRPeriod > 100000) {
    return -1;
  }
  if (optInNbDev === Core.REAL_DEFAULT) {
    optInNbDev = 2.0;
  } else if (!(optInNbDev >= Core.REAL_MIN && optInNbDev <= Core.REAL_MAX)) {
    return -1;
  }
  let emaLookback = 0;
  let atrLookback = 0;
  /* A band value needs BOTH the centre line and the ATR at the same bar, so the
   * first valid output is the later of the two lookbacks. Each term is exactly
   * the lookback of the function it comes from and is never restated here, which
   * is what makes KC inherit TA_FUNC_UNST_EMA and TA_FUNC_UNST_ATR from its two
   * callees. Reporting the honest max keeps outBegIdx == lookback (issue #99),
   * which streaming's Open depends on.
   */
  emaLookback = emaLookbackPositional(optInTimePeriod, core);
  atrLookback = atrLookbackPositional(optInATRPeriod, core);
  return (emaLookback > atrLookback) ? emaLookback : atrLookback;
}

/** @internal */
export function kcDisplayShiftPositional(optInTimePeriod: number, optInATRPeriod: number, optInNbDev: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (kcLookbackPositional(optInTimePeriod, optInATRPeriod, optInNbDev, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 3) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function kcImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  optInTimePeriod: number,
  optInATRPeriod: number,
  optInNbDev: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
): RetCode {
  let retCode: RetCode = RetCode.SUCCESS;
  let i = 0;
  let lookbackTotal = 0;
  let emaLookback = 0;
  let tpStartIdx = 0;
  const tempBegIdx = new MInteger();
  const tempNbElement = new MInteger();
  let tempReal = 0;
  let middle = 0;
  let tempTP!: Float64Array;
  let tempATR!: Float64Array;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInATRPeriod === Core.INTEGER_DEFAULT) {
    optInATRPeriod = 10;
  } else if (optInATRPeriod < 1 || optInATRPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInNbDev === Core.REAL_DEFAULT) {
    optInNbDev = 2.0;
  } else if (!(optInNbDev >= Core.REAL_MIN && optInNbDev <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if ((outRealUpperBand.buffer === outRealMiddleBand.buffer && outRealUpperBand.byteOffset === outRealMiddleBand.byteOffset) || (outRealUpperBand.buffer === outRealLowerBand.buffer && outRealUpperBand.byteOffset === outRealLowerBand.byteOffset) || (outRealMiddleBand.buffer === outRealLowerBand.buffer && outRealMiddleBand.byteOffset === outRealLowerBand.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  emaLookback = emaLookbackPositional(optInTimePeriod, core);
  lookbackTotal = kcLookbackPositional(optInTimePeriod, optInATRPeriod, optInNbDev, core);
  /* Nothing to produce: the range ends before the lookback. Return before
   * touching anything, so that a caller-supplied input which stops short of
   * endIdx is never read past its end.
   */
  if (lookbackTotal > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Each leg is entered at its OWN lookback, so each is the shipped function
   * over this range and nothing here over-warms the shorter one: a caller who
   * wants the band converged sets TA_FUNC_UNST_ATR, exactly as they would when
   * calling TA_ATR directly. The typical price is a derived input, so it is
   * materialized only over the window the moving average reads.
   */
  tpStartIdx = startIdx - emaLookback;
  tempTP = new Float64Array(endIdx - tpStartIdx + 1);
  tempATR = new Float64Array(endIdx - startIdx + 1);
  const _xr0 = typpricePositional(tpStartIdx, endIdx, inHigh, inLow, inClose, tempTP, core);
  tempBegIdx.value = _xr0.begIdx;
  tempNbElement.value = _xr0.count;
  retCode = RetCode.SUCCESS;
  /* The ATR consumes the price inputs before the moving average below writes
   * the middle band, which may be aliased onto one of them.
   */
  const _xr1 = atrPositional(startIdx, endIdx, inHigh, inLow, inClose, optInATRPeriod, tempATR, core);
  tempBegIdx.value = _xr1.begIdx;
  tempNbElement.value = _xr1.count;
  retCode = RetCode.SUCCESS;
  /* tempTP is bar-tpStartIdx relative, so entering the moving average at its
   * own lookback puts its first output on startIdx, where the ATR's already is.
   */
  const _xr2 = emaPositional(emaLookback, endIdx - tpStartIdx, tempTP, optInTimePeriod, outRealMiddleBand, core);
  outBegIdx.value = _xr2.begIdx;
  outNBElement.value = _xr2.count;
  retCode = RetCode.SUCCESS;
  if (outNBElement.value === 0) {
    outNBElement.value = 0;
    return retCode;
  }
  outBegIdx.value = startIdx;
  for (i = 0; i < outNBElement.value; i += 1) {
    middle = outRealMiddleBand[i];
    tempReal = tempATR[i] * optInNbDev;
    outRealUpperBand[i] = middle + tempReal;
    outRealLowerBand[i] = middle - tempReal;
  }
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['atrPeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['nbDev', Core.REAL_DEFAULT, Core.REAL_MIN, Core.REAL_MAX, 'in -3e37..3e37']] as const;

/** @internal */
export function kcPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  optInATRPeriod: number,
  optInNbDev: number,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('KC', 'startIdx', startIdx);
  endIdx = requireInt32('KC', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('KC', 'timePeriod', optInTimePeriod);
  optInATRPeriod = requireInt32('KC', 'atrPeriod', optInATRPeriod);
  optInNbDev = requireReal('KC', 'nbDev', optInNbDev);
  requireIndexRange('KC', startIdx, endIdx);
  const guardLookback = kcLookbackPositional(optInTimePeriod, optInATRPeriod, optInNbDev, core);
  if (guardLookback < 0) {
    throw rejectedParams('KC', PARAM_RANGES, optInTimePeriod, optInATRPeriod, optInNbDev);
  }
  const guardStart = clampedStart('KC', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('KC', 'prices.high', inHigh, guardInLen);
  requireLength('KC', 'prices.low', inLow, guardInLen);
  requireLength('KC', 'prices.close', inClose, guardInLen);
  requireOutput('KC', 'outUpperBand', outRealUpperBand, guardOutLen, Float64Array);
  requireOutput('KC', 'outMiddleBand', outRealMiddleBand, guardOutLen, Float64Array);
  requireOutput('KC', 'outLowerBand', outRealLowerBand, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outRealUpperBand, inHighF64)) {
    throw sharedMemory('KC', 'outUpperBand', 'prices.high');
  }
  if (overlapsPartly(outRealUpperBand, inLowF64)) {
    throw sharedMemory('KC', 'outUpperBand', 'prices.low');
  }
  if (overlapsPartly(outRealUpperBand, inCloseF64)) {
    throw sharedMemory('KC', 'outUpperBand', 'prices.close');
  }
  if (overlapsPartly(outRealMiddleBand, inHighF64)) {
    throw sharedMemory('KC', 'outMiddleBand', 'prices.high');
  }
  if (overlapsPartly(outRealMiddleBand, inLowF64)) {
    throw sharedMemory('KC', 'outMiddleBand', 'prices.low');
  }
  if (overlapsPartly(outRealMiddleBand, inCloseF64)) {
    throw sharedMemory('KC', 'outMiddleBand', 'prices.close');
  }
  if (overlapsPartly(outRealLowerBand, inHighF64)) {
    throw sharedMemory('KC', 'outLowerBand', 'prices.high');
  }
  if (overlapsPartly(outRealLowerBand, inLowF64)) {
    throw sharedMemory('KC', 'outLowerBand', 'prices.low');
  }
  if (overlapsPartly(outRealLowerBand, inCloseF64)) {
    throw sharedMemory('KC', 'outLowerBand', 'prices.close');
  }
  if (overlaps(outRealUpperBand, outRealMiddleBand)) {
    throw sharedMemory('KC', 'outUpperBand', 'outMiddleBand');
  }
  if (overlaps(outRealUpperBand, outRealLowerBand)) {
    throw sharedMemory('KC', 'outUpperBand', 'outLowerBand');
  }
  if (overlaps(outRealMiddleBand, outRealLowerBand)) {
    throw sharedMemory('KC', 'outMiddleBand', 'outLowerBand');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = kcImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInTimePeriod, optInATRPeriod, optInNbDev, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('KC', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link kc}, {@link kcLookback}, {@link kcDisplayShift},
 * {@link kcOpen} and {@link kcOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface KcOptions {
  /**
   * Smoothing period of the typical price moving average (default 20; range 2 to
   * 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  timePeriod?: number | undefined;
  /**
   * Smoothing period of the Average True Range (default 10; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  atrPeriod?: number | undefined;
  /**
   * Multiplier applied to the Average True Range (default 2; `Core.REAL_DEFAULT`
   * selects the default).
   */
  nbDev?: number | undefined;
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
   * Buffer for `outUpperBand`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outUpperBand?: Float64Array | undefined;
  /**
   * Buffer for `outMiddleBand`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outMiddleBand?: Float64Array | undefined;
  /**
   * Buffer for `outLowerBand`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outLowerBand?: Float64Array | undefined;
}

/** What {@link kc} returns, and {@link kcOpenAndFill} with its stream. */
export interface KcResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Centre line plus the scaled Average True Range. `outUpperBand[k]` is the value
   * at input bar `begIdx + k`.
   */
  outUpperBand: Float64Array;
  /**
   * Exponential moving average of the typical price. `outMiddleBand[k]` is the value
   * at input bar `begIdx + k`.
   */
  outMiddleBand: Float64Array;
  /**
   * Centre line minus the scaled Average True Range. `outLowerBand[k]` is the value
   * at input bar `begIdx + k`.
   */
  outLowerBand: Float64Array;
}

/**
 * Number of leading input bars {@link kc} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link KcOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function kcLookback(options?: KcOptions): number {
  const o = optionsOf('KC lookback', options);
  return kcLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.atrPeriod, Core.INTEGER_DEFAULT), orDefault(o.nbDev, Core.REAL_DEFAULT), coreOf('KC lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link kc}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link KcOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function kcDisplayShift(outputIdx: number, options?: KcOptions): number {
  const o = optionsOf('KC displayShift', options);
  return kcDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.atrPeriod, Core.INTEGER_DEFAULT), orDefault(o.nbDev, Core.REAL_DEFAULT), outputIdx, coreOf('KC displayShift', o.core));
}

/**
 * Keltner Channels: three overlap lines around price. The centre line is an
 * exponential moving average of the typical price; the outer bands sit a multiple
 * of the Average True Range above and below it. The band width tracks volatility,
 * so the channel widens in fast markets and narrows in quiet ones.
 *
 * Formula and more info at
 * [ta-lib.org/functions/kc](https://ta-lib.org/functions/kc).
 *
 * **Notes**
 * - Several incompatible indicators are published under the name "Keltner
 *   Channel", disagreeing by percent rather than by rounding. This is the
 *   typical-price centre line with a Wilder-smoothed Average True Range band, the
 *   form implemented by TTR and ta4j.
 * - Chester Keltner's 1960 original smooths the typical price with a simple moving
 *   average and takes the band from the plain daily range; the widely charted
 *   modern variant centres on the close instead. Expect a visible difference
 *   against a package plotting either.
 * - TTR ties the Average True Range period to the centre line's period. Here the
 *   two are independent, so the band width can be tuned separately.
 * - The centre line and the band are separate recursions, each with its own
 *   warm-up. They are entered at their own lookbacks, so a caller who wants either
 *   one converged sets that function's unstable period — `TA_FUNC_UNST_EMA` for
 *   the centre line, `TA_FUNC_UNST_ATR` for the band.
 *
 * `outUpperBand[k]` is the value at input bar `begIdx + k`; every output is
 * exactly as long as the number of values written, and nothing is padded. A range
 * that ends before {@link kcLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `high`: High
 *   price of each bar. `low`: Low price of each bar. `close`: Close price of each
 *   bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link KcOptions}.
 * @returns `begIdx` and the values; see {@link KcResult}. Each output is a view of
 *   its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, an integer parameter that is not an int32, a real parameter that is not a
 *   number, a parameter outside its range, an input too short for the range, a
 *   buffer that is empty, too short or of the wrong type, two buffers sharing memory
 *   or a buffer sharing memory with a `Float64Array` input it is not. Checked before
 *   anything is written.
 *
 * @example
 * ```ts
 * import { kc } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const {
 *   begIdx,
 *   outUpperBand,
 *   outMiddleBand,
 *   outLowerBand,
 * } = kc({ high, low, close }, { timePeriod: 20, atrPeriod: 10, nbDev: 2 });
 * // outUpperBand[k] is the value at bar begIdx + k
 * ```
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`atr`](https://ta-lib.org/functions/atr)
 * @see [`typprice`](https://ta-lib.org/functions/typprice)
 * @see [`bbands`](https://ta-lib.org/functions/bbands)
 * @see [`accbands`](https://ta-lib.org/functions/accbands)
 */
export function kc(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: KcOptions): KcResult {
  requireArgument('KC', 'prices', prices);
  const o = optionsOf('KC', options);
  if (o.endIdx === undefined) {
    requireSeries('KC', 'prices.high', prices.high);
    requireSeries('KC', 'prices.low', prices.low);
    requireSeries('KC', 'prices.close', prices.close);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInATRPeriod = orDefault(o.atrPeriod, Core.INTEGER_DEFAULT);
  const optInNbDev = orDefault(o.nbDev, Core.REAL_DEFAULT);
  const core = coreOf('KC', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, kcLookbackPositional(optInTimePeriod, optInATRPeriod, optInNbDev, core), prices.high, prices.low, prices.close);
  const outRealUpperBand = o.outUpperBand === undefined ? new Float64Array(capacity) : o.outUpperBand;
  const outRealMiddleBand = o.outMiddleBand === undefined ? new Float64Array(capacity) : o.outMiddleBand;
  const outRealLowerBand = o.outLowerBand === undefined ? new Float64Array(capacity) : o.outLowerBand;
  const range = kcPositional(startIdx, endIdx, prices.high, prices.low, prices.close, optInTimePeriod, optInATRPeriod, optInNbDev, outRealUpperBand, outRealMiddleBand, outRealLowerBand, core);
  return { begIdx: range.begIdx, outUpperBand: outRealUpperBand.subarray(0, range.count), outMiddleBand: outRealMiddleBand.subarray(0, range.count), outLowerBand: outRealLowerBand.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live KC stream: one value per closed bar, bit-identical to {@link kc} over the
 * same series. Open it with {@link kcOpen} or {@link kcOpenAndFill}; there is
 * nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/kc](https://ta-lib.org/functions/kc).
 */
export class KcStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  optInATRPeriod: number = 0;
  /** @internal */
  optInNbDev: number = 0;
  /** @internal */
  cur_outRealUpperBand: number = 0;
  /** @internal */
  cur_outRealMiddleBand: number = 0;
  /** @internal */
  cur_outRealLowerBand: number = 0;
  /** @internal */
  sub0!: TyppriceStream;
  /** @internal */
  sub1!: AtrStream;
  /** @internal */
  sub2!: EmaStream;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): KcStream {
    return new KcStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link kc} reports them over the same bars. Each accepted `update` and each
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
      throw failure('KC advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns its outputs.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param out - Receives the outputs and is returned; a new {@link KcOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number, inClose: number, out?: KcOut): KcOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('KC update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('KC update', 'KcOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('KC update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    kcStepImpl(this, inHigh, inLow, inClose);
    this.outRangeCount++;
    if (out === undefined) {
      out = new KcOut();
    }
    out.outUpperBand = this.cur_outRealUpperBand;
    out.outMiddleBand = this.cur_outRealMiddleBand;
    out.outLowerBand = this.cur_outRealLowerBand;
    return out;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param out - Receives the outputs and is returned; a new {@link KcOut} when
   *   omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number, inClose: number, out?: KcOut): KcOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('KC peek', 'KcOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('KC peek', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let middle = 0;
    let tempReal = 0;
    let cur_tempTP = 0;
    let cur_tempATR = 0;
    let cur_outRealMiddleBand = 0;
    let cur_outRealUpperBand = 0;
    let cur_outRealLowerBand = 0;
    cur_tempTP = sp.sub0.peek(inHigh, inLow, inClose);
    cur_tempATR = sp.sub1.peek(inHigh, inLow, inClose);
    cur_outRealMiddleBand = sp.sub2.peek(cur_tempTP);
    middle = cur_outRealMiddleBand;
    tempReal = cur_tempATR * sp.optInNbDev;
    cur_outRealUpperBand = middle + tempReal;
    cur_outRealLowerBand = middle - tempReal;
    if (out === undefined) {
      out = new KcOut();
    }
    out.outUpperBand = cur_outRealUpperBand;
    out.outMiddleBand = cur_outRealMiddleBand;
    out.outLowerBand = cur_outRealLowerBand;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link KcOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   */
  value(out?: KcOut): KcOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('KC value', 'KcOut');
    }
    if (out === undefined) {
      out = new KcOut();
    }
    out.outUpperBand = this.cur_outRealUpperBand;
    out.outMiddleBand = this.cur_outRealMiddleBand;
    out.outLowerBand = this.cur_outRealLowerBand;
    return out;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): KcStream {
    const c = new KcStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.optInATRPeriod = this.optInATRPeriod;
    c.optInNbDev = this.optInNbDev;
    c.cur_outRealUpperBand = this.cur_outRealUpperBand;
    c.cur_outRealMiddleBand = this.cur_outRealMiddleBand;
    c.cur_outRealLowerBand = this.cur_outRealLowerBand;
    c.sub0 = this.sub0.clone();
    c.sub1 = this.sub1.clone();
    c.sub2 = this.sub2.clone();
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one KC bar, as `update`, `peek` and `value` return them. Passed
 * back to one of them, it is filled and returned again, with no allocation; it
 * holds its values until the next call that writes it. A new one holds `NaN`.
 */
export class KcOut {
  /** Centre line plus the scaled Average True Range. */
  outUpperBand: number = NaN;
  /** Exponential moving average of the typical price. */
  outMiddleBand: number = NaN;
  /** Centre line minus the scaled Average True Range. */
  outLowerBand: number = NaN;
}

function kcStepImpl(sp: KcStream, inHigh: number, inLow: number, inClose: number): void {
  let middle = 0;
  let tempReal = 0;
  let cur_tempTP = 0;
  let cur_tempATR = 0;
  let cur_outRealMiddleBand = 0;
  let cur_outRealUpperBand = 0;
  let cur_outRealLowerBand = 0;
  cur_tempTP = sp.sub0.update(inHigh, inLow, inClose);
  cur_tempATR = sp.sub1.update(inHigh, inLow, inClose);
  cur_outRealMiddleBand = sp.sub2.update(cur_tempTP);
  middle = cur_outRealMiddleBand;
  tempReal = cur_tempATR * sp.optInNbDev;
  cur_outRealUpperBand = middle + tempReal;
  cur_outRealLowerBand = middle - tempReal;
  sp.cur_outRealUpperBand = cur_outRealUpperBand;
  sp.cur_outRealMiddleBand = cur_outRealMiddleBand;
  sp.cur_outRealLowerBand = cur_outRealLowerBand;
}

function kcOpenImpl(
  sp: KcStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInATRPeriod: number,
  optInNbDev: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let i = 0;
  let lookbackTotal = 0;
  let emaLookback = 0;
  let tpStartIdx = 0;
  const tempBegIdx = new MInteger();
  const tempNbElement = new MInteger();
  let tempReal = 0;
  let middle = 0;
  let tempTP!: Float64Array;
  let tempATR!: Float64Array;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length || inClose.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInATRPeriod === Core.INTEGER_DEFAULT) {
    optInATRPeriod = 10;
  } else if (optInATRPeriod < 1 || optInATRPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInNbDev === Core.REAL_DEFAULT) {
    optInNbDev = 2.0;
  } else if (!(optInNbDev >= Core.REAL_MIN && optInNbDev <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen < kcLookbackPositional(optInTimePeriod, optInATRPeriod, optInNbDev, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const sc_outRealUpperBand = outStride === 1 ? outRealUpperBand : new Float64Array(historyLen);
  const sc_outRealMiddleBand = outStride === 1 ? outRealMiddleBand : new Float64Array(historyLen);
  const sc_outRealLowerBand = outStride === 1 ? outRealLowerBand : new Float64Array(historyLen);
  emaLookback = emaLookbackPositional(optInTimePeriod, core);
  lookbackTotal = kcLookbackPositional(optInTimePeriod, optInATRPeriod, optInNbDev, core);
  /* Nothing to produce: the range ends before the lookback. Return before
   * touching anything, so that a caller-supplied input which stops short of
   * endIdx is never read past its end.
   */
  if (lookbackTotal > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Each leg is entered at its OWN lookback, so each is the shipped function
   * over this range and nothing here over-warms the shorter one: a caller who
   * wants the band converged sets TA_FUNC_UNST_ATR, exactly as they would when
   * calling TA_ATR directly. The typical price is a derived input, so it is
   * materialized only over the window the moving average reads.
   */
  tpStartIdx = startIdx - emaLookback;
  tempTP = new Float64Array(endIdx - tpStartIdx + 1);
  tempATR = new Float64Array(endIdx - startIdx + 1);
  const sub0 = typpriceOpenAndFillInternal(core, inHigh, inLow, inClose, tpStartIdx, tempBegIdx, tempNbElement, tempTP);
  /* The ATR consumes the price inputs before the moving average below writes
   * the middle band, which may be aliased onto one of them.
   */
  const sub1 = atrOpenAndFillInternal(core, inHigh, inLow, inClose, startIdx, optInATRPeriod, tempBegIdx, tempNbElement, tempATR);
  /* tempTP is bar-tpStartIdx relative, so entering the moving average at its
   * own lookback puts its first output on startIdx, where the ATR's already is.
   */
  const sub2 = emaOpenAndFillInternal(core, tempTP.slice(0, (endIdx - tpStartIdx) + 1), emaLookback, optInTimePeriod, outBegIdx, outNBElement, sc_outRealMiddleBand);
  if (outNBElement.value === 0) {
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outBegIdx.value = startIdx;
  for (i = 0; i < outNBElement.value; i += 1) {
    middle = sc_outRealMiddleBand[i];
    tempReal = tempATR[i] * optInNbDev;
    sc_outRealUpperBand[i] = middle + tempReal;
    sc_outRealLowerBand[i] = middle - tempReal;
  }
  if (outNBElement.value < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.optInATRPeriod = optInATRPeriod;
  sp.optInNbDev = optInNbDev;
  sp.sub0 = sub0;
  sp.sub1 = sub1;
  sp.sub2 = sub2;
  sp.cur_outRealUpperBand = sc_outRealUpperBand[outNBElement.value - 1];
  sp.cur_outRealMiddleBand = sc_outRealMiddleBand[outNBElement.value - 1];
  sp.cur_outRealLowerBand = sc_outRealLowerBand[outNBElement.value - 1];
  return RetCode.SUCCESS;
}

/** @internal */
export function kcOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInATRPeriod: number,
  optInNbDev: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
): KcStream {
  const sp = KcStream.of(core);
  const retCode = kcOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, optInATRPeriod, optInNbDev, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('KC openAndFill', inHigh.length, startIdx, kcLookbackPositional(optInTimePeriod, optInATRPeriod, optInNbDev, core));
  }
  if (retCode === RetCode.BAD_PARAM && kcLookbackPositional(optInTimePeriod, optInATRPeriod, optInNbDev, core) < 0) {
    throw rejectedParams('KC openAndFill', PARAM_RANGES, optInTimePeriod, optInATRPeriod, optInNbDev);
  }
  throw streamFailure('KC openAndFill', retCode);
}

/** @internal */
export function kcOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInATRPeriod: number,
  optInNbDev: number,
): KcStream {
  const sp = KcStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outRealUpperBand = new Float64Array(1);
  const sink_outRealMiddleBand = new Float64Array(1);
  const sink_outRealLowerBand = new Float64Array(1);
  const retCode = kcOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, optInATRPeriod, optInNbDev, outBegIdx, outNBElement, sink_outRealUpperBand, sink_outRealMiddleBand, sink_outRealLowerBand, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('KC open', inHigh.length, startIdx, kcLookbackPositional(optInTimePeriod, optInATRPeriod, optInNbDev, core));
  }
  if (retCode === RetCode.BAD_PARAM && kcLookbackPositional(optInTimePeriod, optInATRPeriod, optInNbDev, core) < 0) {
    throw rejectedParams('KC open', PARAM_RANGES, optInTimePeriod, optInATRPeriod, optInNbDev);
  }
  throw streamFailure('KC open', retCode);
}

/** @internal */
export function kcOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  optInATRPeriod: number,
  optInNbDev: number,
  core: Core = Core.DEFAULT,
): KcStream {
  optInTimePeriod = requireInt32('KC open', 'timePeriod', optInTimePeriod);
  optInATRPeriod = requireInt32('KC open', 'atrPeriod', optInATRPeriod);
  optInNbDev = requireReal('KC open', 'nbDev', optInNbDev);
  requireArgument('KC open', 'prices.high', inHigh);
  requireHistory('KC open', inHigh.length);
  requireArgument('KC open', 'prices.low', inLow);
  requireArgument('KC open', 'prices.close', inClose);
  requireHistoryLength('KC open', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('KC open', 'prices.close', inClose.length, inHigh.length);
  return kcOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInTimePeriod, optInATRPeriod, optInNbDev);
}

/** @internal */
export function kcOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  optInATRPeriod: number,
  optInNbDev: number,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
  core: Core = Core.DEFAULT,
): KcStream {
  optInTimePeriod = requireInt32('KC openAndFill', 'timePeriod', optInTimePeriod);
  optInATRPeriod = requireInt32('KC openAndFill', 'atrPeriod', optInATRPeriod);
  optInNbDev = requireReal('KC openAndFill', 'nbDev', optInNbDev);
  requireArgument('KC openAndFill', 'prices.high', inHigh);
  requireHistory('KC openAndFill', inHigh.length);
  requireArgument('KC openAndFill', 'prices.low', inLow);
  requireArgument('KC openAndFill', 'prices.close', inClose);
  const guardLookback = kcLookbackPositional(optInTimePeriod, optInATRPeriod, optInNbDev, core);
  if (guardLookback < 0) {
    throw rejectedParams('KC openAndFill', PARAM_RANGES, optInTimePeriod, optInATRPeriod, optInNbDev);
  }
  const guardOutLen = openFillCount('KC openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('KC openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('KC openAndFill', 'prices.close', inClose.length, inHigh.length);
  requireOutput('KC openAndFill', 'outUpperBand', outRealUpperBand, guardOutLen, Float64Array);
  requireOutput('KC openAndFill', 'outMiddleBand', outRealMiddleBand, guardOutLen, Float64Array);
  requireOutput('KC openAndFill', 'outLowerBand', outRealLowerBand, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outRealUpperBand.buffer === inHighF64.buffer && outRealUpperBand.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('KC openAndFill', 'outUpperBand', 'prices.high');
  }
  if (outRealUpperBand.buffer === inLowF64.buffer && outRealUpperBand.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('KC openAndFill', 'outUpperBand', 'prices.low');
  }
  if (outRealUpperBand.buffer === inCloseF64.buffer && outRealUpperBand.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('KC openAndFill', 'outUpperBand', 'prices.close');
  }
  if (outRealMiddleBand.buffer === inHighF64.buffer && outRealMiddleBand.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('KC openAndFill', 'outMiddleBand', 'prices.high');
  }
  if (outRealMiddleBand.buffer === inLowF64.buffer && outRealMiddleBand.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('KC openAndFill', 'outMiddleBand', 'prices.low');
  }
  if (outRealMiddleBand.buffer === inCloseF64.buffer && outRealMiddleBand.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('KC openAndFill', 'outMiddleBand', 'prices.close');
  }
  if (outRealLowerBand.buffer === inHighF64.buffer && outRealLowerBand.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('KC openAndFill', 'outLowerBand', 'prices.high');
  }
  if (outRealLowerBand.buffer === inLowF64.buffer && outRealLowerBand.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('KC openAndFill', 'outLowerBand', 'prices.low');
  }
  if (outRealLowerBand.buffer === inCloseF64.buffer && outRealLowerBand.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('KC openAndFill', 'outLowerBand', 'prices.close');
  }
  if (outRealUpperBand.buffer === outRealMiddleBand.buffer && outRealUpperBand.byteOffset === outRealMiddleBand.byteOffset) {
    throw sharedMemory('KC openAndFill', 'outUpperBand', 'outMiddleBand');
  }
  if (outRealUpperBand.buffer === outRealLowerBand.buffer && outRealUpperBand.byteOffset === outRealLowerBand.byteOffset) {
    throw sharedMemory('KC openAndFill', 'outUpperBand', 'outLowerBand');
  }
  if (outRealMiddleBand.buffer === outRealLowerBand.buffer && outRealMiddleBand.byteOffset === outRealLowerBand.byteOffset) {
    throw sharedMemory('KC openAndFill', 'outMiddleBand', 'outLowerBand');
  }
  if (guardOutLen > 0 && overlaps(outRealUpperBand, inHighF64)) {
    throw sharedMemory('KC openAndFill', 'outUpperBand', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outRealUpperBand, inLowF64)) {
    throw sharedMemory('KC openAndFill', 'outUpperBand', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outRealUpperBand, inCloseF64)) {
    throw sharedMemory('KC openAndFill', 'outUpperBand', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outRealMiddleBand, inHighF64)) {
    throw sharedMemory('KC openAndFill', 'outMiddleBand', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outRealMiddleBand, inLowF64)) {
    throw sharedMemory('KC openAndFill', 'outMiddleBand', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outRealMiddleBand, inCloseF64)) {
    throw sharedMemory('KC openAndFill', 'outMiddleBand', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outRealLowerBand, inHighF64)) {
    throw sharedMemory('KC openAndFill', 'outLowerBand', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outRealLowerBand, inLowF64)) {
    throw sharedMemory('KC openAndFill', 'outLowerBand', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outRealLowerBand, inCloseF64)) {
    throw sharedMemory('KC openAndFill', 'outLowerBand', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outRealUpperBand, outRealMiddleBand)) {
    throw sharedMemory('KC openAndFill', 'outUpperBand', 'outMiddleBand');
  }
  if (guardOutLen > 0 && overlaps(outRealUpperBand, outRealLowerBand)) {
    throw sharedMemory('KC openAndFill', 'outUpperBand', 'outLowerBand');
  }
  if (guardOutLen > 0 && overlaps(outRealMiddleBand, outRealLowerBand)) {
    throw sharedMemory('KC openAndFill', 'outMiddleBand', 'outLowerBand');
  }
  return kcOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInTimePeriod, optInATRPeriod, optInNbDev, new MInteger(), new MInteger(), outRealUpperBand, outRealMiddleBand, outRealLowerBand);
}

/**
 * Opens a live KC stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link kc} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link KcOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `kcLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a real parameter that is not a
 *   number, a parameter outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { kcOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = kcOpen({ high, low, close }, {
 *   timePeriod: 20,
 *   atrPeriod: 10,
 *   nbDev: 2,
 * });
 * const peeked = stream.peek(101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function kcOpen(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: KcOptions): KcStream {
  requireArgument('KC open', 'prices', prices);
  const o = optionsOf('KC open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInATRPeriod = orDefault(o.atrPeriod, Core.INTEGER_DEFAULT);
  const optInNbDev = orDefault(o.nbDev, Core.REAL_DEFAULT);
  const core = coreOf('KC open', o.core);
  return kcOpenPositional(prices.high, prices.low, prices.close, optInTimePeriod, optInATRPeriod, optInNbDev, core);
}

/**
 * {@link kcOpen} that also computes the outputs over the whole history in the same
 * pass, bit-identically to {@link kc}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link KcOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link kc} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `kcLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a real parameter that is not a
 *   number, a parameter outside its range, histories of different lengths or a
 *   buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { kcOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const {
 *   stream,
 *   begIdx,
 *   outUpperBand,
 *   outMiddleBand,
 *   outLowerBand,
 * } = kcOpenAndFill({ high, low, close }, {
 *   timePeriod: 20,
 *   atrPeriod: 10,
 *   nbDev: 2,
 * });
 * // outUpperBand[k] is the value at bar begIdx + k
 * ```
 */
export function kcOpenAndFill(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: KcOptions): KcResult & { stream: KcStream } {
  requireArgument('KC openAndFill', 'prices', prices);
  const o = optionsOf('KC openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInATRPeriod = orDefault(o.atrPeriod, Core.INTEGER_DEFAULT);
  const optInNbDev = orDefault(o.nbDev, Core.REAL_DEFAULT);
  const core = coreOf('KC openAndFill', o.core);
  const capacity = fillCapacity(kcLookbackPositional(optInTimePeriod, optInATRPeriod, optInNbDev, core), prices.high, prices.low, prices.close);
  const outRealUpperBand = o.outUpperBand === undefined ? new Float64Array(capacity) : o.outUpperBand;
  const outRealMiddleBand = o.outMiddleBand === undefined ? new Float64Array(capacity) : o.outMiddleBand;
  const outRealLowerBand = o.outLowerBand === undefined ? new Float64Array(capacity) : o.outLowerBand;
  const stream = kcOpenAndFillPositional(prices.high, prices.low, prices.close, optInTimePeriod, optInATRPeriod, optInNbDev, outRealUpperBand, outRealMiddleBand, outRealLowerBand, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outUpperBand: outRealUpperBand.subarray(0, range.count), outMiddleBand: outRealMiddleBand.subarray(0, range.count), outLowerBand: outRealLowerBand.subarray(0, range.count) };
}
