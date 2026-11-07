/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  PP       Peter Pudaite
 *  AA       Andrew Atkinson
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120802 MF   Template creation.
 *  101103 PP   Initial creation of code.
 *  112603 MF   Add independent control to the RSI period.
 *  020605 AA   Fix #1117656. NULL pointer assignement.
 */

import { MAType, MATypes } from '../enums.ts';
import { badSink, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { RsiStream, rsiLookbackPositional, rsiOpenAndFillInternal, rsiPositional } from './rsi.ts';
import { StochfOut, StochfStream, stochfLookbackPositional, stochfOpenAndFillInternal, stochfPositional } from './stochf.ts';

/** @internal */
export function stochrsiLookbackPositional(optInTimePeriod: number, optInFastK_Period: number, optInFastD_Period: number, optInFastD_MAType: MAType | typeof Core.INTEGER_DEFAULT, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if ((optInFastK_Period | 0) !== optInFastK_Period) {
    return -1;
  }
  if ((optInFastD_Period | 0) !== optInFastD_Period) {
    return -1;
  }
  if ((optInFastD_MAType | 0) !== optInFastD_MAType) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  if (optInFastK_Period === Core.INTEGER_DEFAULT) {
    optInFastK_Period = 5;
  } else if (optInFastK_Period < 1 || optInFastK_Period > 100000) {
    return -1;
  }
  if (optInFastD_Period === Core.INTEGER_DEFAULT) {
    optInFastD_Period = 3;
  } else if (optInFastD_Period < 1 || optInFastD_Period > 100000) {
    return -1;
  }
  if (optInFastD_MAType === Core.INTEGER_DEFAULT || optInFastD_MAType === MAType.DEFAULT) {
    optInFastD_MAType = MAType.SMA;
  } else if (optInFastD_MAType < MATypes.Min || optInFastD_MAType > MATypes.Max) {
    return -1;
  }
  let retValue = 0;
  retValue = rsiLookbackPositional(optInTimePeriod, core) + stochfLookbackPositional(optInFastK_Period, optInFastD_Period, (optInFastD_MAType as MAType), core);
  return retValue;
}

/** @internal */
export function stochrsiDisplayShiftPositional(optInTimePeriod: number, optInFastK_Period: number, optInFastD_Period: number, optInFastD_MAType: MAType | typeof Core.INTEGER_DEFAULT, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (stochrsiLookbackPositional(optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 2) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function stochrsiImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  optInFastK_Period: number,
  optInFastD_Period: number,
  optInFastD_MAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outFastK: Float64Array,
  outFastD: Float64Array,
): RetCode {
  let tempRSIBuffer!: Float64Array;
  let retCode: RetCode = RetCode.SUCCESS;
  let lookbackTotal = 0;
  let lookbackSTOCHF = 0;
  let tempArraySize = 0;
  const outBegIdx1 = new MInteger();
  const outBegIdx2 = new MInteger();
  const outNbElement1 = new MInteger();
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInFastK_Period === Core.INTEGER_DEFAULT) {
    optInFastK_Period = 5;
  } else if (optInFastK_Period < 1 || optInFastK_Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInFastD_Period === Core.INTEGER_DEFAULT) {
    optInFastD_Period = 3;
  } else if (optInFastD_Period < 1 || optInFastD_Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInFastD_MAType === Core.INTEGER_DEFAULT || optInFastD_MAType === MAType.DEFAULT) {
    optInFastD_MAType = MAType.SMA;
  } else if (optInFastD_MAType < MATypes.Min || optInFastD_MAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if ((outFastK.buffer === outFastD.buffer && outFastK.byteOffset === outFastD.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  /* Stochastic RSI
   *
   * Reference: "Stochastic RSI and Dynamic Momentum Index"
   *            by Tushar Chande and Stanley Kroll
   *            Stock&Commodities V.11:5 (189-199)
   *
   * The TA-Lib version offer flexibility beyond what is explain
   * in the Stock&Commodities article.
   *
   * To calculate the "Unsmoothed stochastic RSI" with symetry like
   * explain in the article, keep the optInTimePeriod and optInFastK_Period
   * equal. Example:
   *
   *    unsmoothed stoch RSI 14 : optInTimePeriod   = 14
   *                              optInFastK_Period = 14
   *                              optInFastD_Period = 'x'
   *
   * The outFastK is the unsmoothed RSI discuss in the article.
   *
   * You can set the optInFastD_Period to smooth the RSI. The smooth
   * version will be found in outFastD. The outFastK will still contain
   * the unsmoothed stoch RSI. If you do not care about the smoothing of
   * the StochRSI, just leave optInFastD_Period to 1 and ignore outFastD.
   */
  outBegIdx.value = 0;
  outNBElement.value = 0;
  /* Adjust startIdx to account for the lookback period. */
  lookbackSTOCHF = stochfLookbackPositional(optInFastK_Period, optInFastD_Period, (optInFastD_MAType as MAType), core);
  lookbackTotal = rsiLookbackPositional(optInTimePeriod, core) + lookbackSTOCHF;
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  outBegIdx.value = startIdx;
  tempArraySize = endIdx - startIdx + 1 + lookbackSTOCHF;
  tempRSIBuffer = new Float64Array(tempArraySize);
  const _xr0 = rsiPositional(startIdx - lookbackSTOCHF, endIdx, inReal, optInTimePeriod, tempRSIBuffer, core);
  outBegIdx1.value = _xr0.begIdx;
  outNbElement1.value = _xr0.count;
  retCode = RetCode.SUCCESS;
  if (outNbElement1.value === 0) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return retCode;
  }
  const _xr1 = stochfPositional(0, tempArraySize - 1, tempRSIBuffer, tempRSIBuffer, tempRSIBuffer, optInFastK_Period, optInFastD_Period, (optInFastD_MAType as MAType), outFastK, outFastD, core);
  outBegIdx2.value = _xr1.begIdx;
  outNBElement.value = _xr1.count;
  retCode = RetCode.SUCCESS;
  if (outNBElement.value === 0) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return retCode;
  }
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['fastKPeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['fastDPeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['fastDMaType', Core.INTEGER_DEFAULT, MATypes.Min, MATypes.Max, 'a MAType']] as const;

/** @internal */
export function stochrsiPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInFastK_Period: number,
  optInFastD_Period: number,
  optInFastD_MAType: MAType | typeof Core.INTEGER_DEFAULT,
  outFastK: Float64Array,
  outFastD: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('STOCHRSI', 'startIdx', startIdx);
  endIdx = requireInt32('STOCHRSI', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('STOCHRSI', 'timePeriod', optInTimePeriod);
  optInFastK_Period = requireInt32('STOCHRSI', 'fastKPeriod', optInFastK_Period);
  optInFastD_Period = requireInt32('STOCHRSI', 'fastDPeriod', optInFastD_Period);
  optInFastD_MAType = requireInt32('STOCHRSI', 'fastDMaType', optInFastD_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireIndexRange('STOCHRSI', startIdx, endIdx);
  const guardLookback = stochrsiLookbackPositional(optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType, core);
  if (guardLookback < 0) {
    throw rejectedParams('STOCHRSI', PARAM_RANGES, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType);
  }
  const guardStart = clampedStart('STOCHRSI', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('STOCHRSI', 'real', inReal, guardInLen);
  requireOutput('STOCHRSI', 'outFastK', outFastK, guardOutLen, Float64Array);
  requireOutput('STOCHRSI', 'outFastD', outFastD, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outFastK, inRealF64)) {
    throw sharedMemory('STOCHRSI', 'outFastK', 'real');
  }
  if (overlapsPartly(outFastD, inRealF64)) {
    throw sharedMemory('STOCHRSI', 'outFastD', 'real');
  }
  if (overlaps(outFastK, outFastD)) {
    throw sharedMemory('STOCHRSI', 'outFastK', 'outFastD');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = stochrsiImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx, outNBElement, outFastK, outFastD);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('STOCHRSI', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link stochrsi}, {@link stochrsiLookback},
 * {@link stochrsiDisplayShift}, {@link stochrsiOpen} and
 * {@link stochrsiOpenAndFill}. Every key is optional: an omitted or `undefined`
 * key takes its default, and a call ignores the keys it has no use for.
 */
export interface StochrsiOptions {
  /**
   * RSI period (default 14; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
   * default).
   */
  timePeriod?: number | undefined;
  /**
   * Lookback window for the RSI min/max stochastic (default 5; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  fastKPeriod?: number | undefined;
  /**
   * Smoothing period for %D (default 3; range 1 to 100000; `Core.INTEGER_DEFAULT`
   * selects the default).
   */
  fastDPeriod?: number | undefined;
  /**
   * MA type used to smooth %D (default `MAType.SMA`; values: 0=SMA, 1=EMA, 2=WMA,
   * 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT,
   * 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT`
   * selects the default).
   */
  fastDMaType?: MAType | typeof Core.INTEGER_DEFAULT | undefined;
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
   * Buffer for `outFastK`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outFastK?: Float64Array | undefined;
  /**
   * Buffer for `outFastD`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outFastD?: Float64Array | undefined;
}

/** What {@link stochrsi} returns, and {@link stochrsiOpenAndFill} with its stream. */
export interface StochrsiResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Unsmoothed stochastic of the RSI (raw %K). `outFastK[k]` is the value at input
   * bar `begIdx + k`.
   */
  outFastK: Float64Array;
  /**
   * %K smoothed over FastD_Period (signal line). `outFastD[k]` is the value at input
   * bar `begIdx + k`.
   */
  outFastD: Float64Array;
}

/**
 * Number of leading input bars {@link stochrsi} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link StochrsiOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function stochrsiLookback(options?: StochrsiOptions): number {
  const o = optionsOf('STOCHRSI lookback', options);
  return stochrsiLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.fastKPeriod, Core.INTEGER_DEFAULT), orDefault(o.fastDPeriod, Core.INTEGER_DEFAULT), orDefault(o.fastDMaType, Core.INTEGER_DEFAULT), coreOf('STOCHRSI lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link stochrsi}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link StochrsiOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function stochrsiDisplayShift(outputIdx: number, options?: StochrsiOptions): number {
  const o = optionsOf('STOCHRSI displayShift', options);
  return stochrsiDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.fastKPeriod, Core.INTEGER_DEFAULT), orDefault(o.fastDPeriod, Core.INTEGER_DEFAULT), orDefault(o.fastDMaType, Core.INTEGER_DEFAULT), outputIdx, coreOf('STOCHRSI displayShift', o.core));
}

/**
 * Applies the Fast Stochastic (STOCHF) oscillator to an RSI series instead of
 * price, measuring where RSI sits within its recent min/max range. Oscillates
 * 0-100; high = RSI near its recent top, low = near its recent bottom.
 *
 * Formula and more info at
 * [ta-lib.org/functions/stochrsi](https://ta-lib.org/functions/stochrsi).
 *
 * **Notes**
 * - To reproduce the original article's unsmoothed Stochastic RSI, set the RSI
 *   period equal to the %K period and read the raw %K output.
 * - When the RSI's recent range is zero, %K is set to 0 instead of being
 *   undefined.
 *
 * `outFastK[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link stochrsiLookback} is a success with empty outputs.
 *
 * @param real - Source series fed into the RSI calculation.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link StochrsiOptions}.
 * @returns `begIdx` and the values; see {@link StochrsiResult}. Each output is a
 *   view of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, an integer parameter that is not an int32, a parameter outside its range,
 *   an input too short for the range, a buffer that is empty, too short or of the
 *   wrong type, two buffers sharing memory or a buffer sharing memory with a
 *   `Float64Array` input it is not. Checked before anything is written.
 *
 * @example
 * ```ts
 * import { MAType, stochrsi } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, outFastK, outFastD } = stochrsi(data, {
 *   timePeriod: 14,
 *   fastKPeriod: 5,
 *   fastDPeriod: 3,
 *   fastDMaType: MAType.SMA,
 * });
 * // outFastK[k] is the value at bar begIdx + k
 * ```
 * @see [`rsi`](https://ta-lib.org/functions/rsi)
 * @see [`stochf`](https://ta-lib.org/functions/stochf)
 * @see [`stoch`](https://ta-lib.org/functions/stoch)
 * @see [`ma`](https://ta-lib.org/functions/ma)
 */
export function stochrsi(real: ArrayLike<number>, options?: StochrsiOptions): StochrsiResult {
  const o = optionsOf('STOCHRSI', options);
  if (o.endIdx === undefined) {
    requireSeries('STOCHRSI', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInFastK_Period = orDefault(o.fastKPeriod, Core.INTEGER_DEFAULT);
  const optInFastD_Period = orDefault(o.fastDPeriod, Core.INTEGER_DEFAULT);
  const optInFastD_MAType = orDefault(o.fastDMaType, Core.INTEGER_DEFAULT);
  const core = coreOf('STOCHRSI', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, stochrsiLookbackPositional(optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType, core), real);
  const outFastK = o.outFastK === undefined ? new Float64Array(capacity) : o.outFastK;
  const outFastD = o.outFastD === undefined ? new Float64Array(capacity) : o.outFastD;
  const range = stochrsiPositional(startIdx, endIdx, real, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outFastK, outFastD, core);
  return { begIdx: range.begIdx, outFastK: outFastK.subarray(0, range.count), outFastD: outFastD.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live STOCHRSI stream: one value per closed bar, bit-identical to
 * {@link stochrsi} over the same series. Open it with {@link stochrsiOpen} or
 * {@link stochrsiOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/stochrsi](https://ta-lib.org/functions/stochrsi).
 */
export class StochrsiStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  optInFastK_Period: number = 0;
  /** @internal */
  optInFastD_Period: number = 0;
  /** @internal */
  optInFastD_MAType: number = 0;
  /** @internal */
  cur_outFastK: number = 0;
  /** @internal */
  cur_outFastD: number = 0;
  /** @internal */
  sub0!: RsiStream;
  /** @internal */
  sub1!: StochfStream;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): StochrsiStream {
    return new StochrsiStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link stochrsi} reports them over the same bars. Each accepted `update` and
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
      throw streamFailure('STOCHRSI advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns its outputs.
   *
   * @param real - The bar's `real` value: Source series fed into the RSI
   *   calculation.
   * @param out - Receives the outputs and is returned; a new {@link StochrsiOut}
   *   when omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included, before anything moves.
   *   `TALibError` with code `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached
   *   bar `Core.INDEX_MAX`.
   */
  update(real: number, out?: StochrsiOut): StochrsiOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('STOCHRSI update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('STOCHRSI update', 'StochrsiOut');
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('STOCHRSI update', 'real');
    }
    stochrsiStepImpl(this, real);
    this.outRangeCount++;
    if (out === undefined) {
      out = new StochrsiOut();
    }
    out.outFastK = this.cur_outFastK;
    out.outFastD = this.cur_outFastD;
    return out;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: Source series fed into the RSI
   *   calculation.
   * @param out - Receives the outputs and is returned; a new {@link StochrsiOut}
   *   when omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included.
   */
  peek(real: number, out?: StochrsiOut): StochrsiOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('STOCHRSI peek', 'StochrsiOut');
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('STOCHRSI peek', 'real');
    }
    const sp = this;
    const inReal = real;
    let cur_tempRSIBuffer = 0;
    let cur_outFastK = 0;
    let cur_outFastD = 0;
    cur_tempRSIBuffer = sp.sub0.peek(inReal);
    {
      const subOut1 = (scratchStochfOut ??= new StochfOut());
      sp.sub1.peek(cur_tempRSIBuffer, cur_tempRSIBuffer, cur_tempRSIBuffer, subOut1);
      cur_outFastK = subOut1.outFastK;
      cur_outFastD = subOut1.outFastD;
    }
    if (out === undefined) {
      out = new StochrsiOut();
    }
    out.outFastK = cur_outFastK;
    out.outFastD = cur_outFastD;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link StochrsiOut}
   *   when omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a sink that is not an object,
   *   `null` included.
   */
  value(out?: StochrsiOut): StochrsiOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('STOCHRSI value', 'StochrsiOut');
    }
    if (out === undefined) {
      out = new StochrsiOut();
    }
    out.outFastK = this.cur_outFastK;
    out.outFastD = this.cur_outFastD;
    return out;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): StochrsiStream {
    const c = new StochrsiStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.optInFastK_Period = this.optInFastK_Period;
    c.optInFastD_Period = this.optInFastD_Period;
    c.optInFastD_MAType = this.optInFastD_MAType;
    c.cur_outFastK = this.cur_outFastK;
    c.cur_outFastD = this.cur_outFastD;
    c.sub0 = this.sub0.clone();
    c.sub1 = this.sub1.clone();
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one STOCHRSI bar, as `update`, `peek` and `value` return them.
 * Passed back to one of them, it is filled and returned again, with no allocation;
 * it holds its values until the next call that writes it. A new one holds `NaN`.
 */
export class StochrsiOut {
  /** Unsmoothed stochastic of the RSI (raw %K) */
  outFastK: number = NaN;
  /** %K smoothed over FastD_Period (signal line) */
  outFastD: number = NaN;
}

function stochrsiStepImpl(sp: StochrsiStream, inReal: number): void {
  let cur_tempRSIBuffer = 0;
  let cur_outFastK = 0;
  let cur_outFastD = 0;
  cur_tempRSIBuffer = sp.sub0.update(inReal);
  {
    const subOut1 = (scratchStochfOut ??= new StochfOut());
    sp.sub1.update(cur_tempRSIBuffer, cur_tempRSIBuffer, cur_tempRSIBuffer, subOut1);
    cur_outFastK = subOut1.outFastK;
    cur_outFastD = subOut1.outFastD;
  }
  sp.cur_outFastK = cur_outFastK;
  sp.cur_outFastD = cur_outFastD;
}

function stochrsiOpenImpl(
  sp: StochrsiStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInFastK_Period: number,
  optInFastD_Period: number,
  optInFastD_MAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outFastK: Float64Array,
  outFastD: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let tempRSIBuffer!: Float64Array;
  let lookbackTotal = 0;
  let lookbackSTOCHF = 0;
  let tempArraySize = 0;
  const outBegIdx1 = new MInteger();
  const outBegIdx2 = new MInteger();
  const outNbElement1 = new MInteger();
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInFastK_Period === Core.INTEGER_DEFAULT) {
    optInFastK_Period = 5;
  } else if (optInFastK_Period < 1 || optInFastK_Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInFastD_Period === Core.INTEGER_DEFAULT) {
    optInFastD_Period = 3;
  } else if (optInFastD_Period < 1 || optInFastD_Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInFastD_MAType === Core.INTEGER_DEFAULT || optInFastD_MAType === MAType.DEFAULT) {
    optInFastD_MAType = MAType.SMA;
  } else if (optInFastD_MAType < MATypes.Min || optInFastD_MAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen < stochrsiLookbackPositional(optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType as MAType, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const sc_outFastK = outStride === 1 ? outFastK : new Float64Array(historyLen);
  const sc_outFastD = outStride === 1 ? outFastD : new Float64Array(historyLen);
  /* Stochastic RSI
   *
   * Reference: "Stochastic RSI and Dynamic Momentum Index"
   *            by Tushar Chande and Stanley Kroll
   *            Stock&Commodities V.11:5 (189-199)
   *
   * The TA-Lib version offer flexibility beyond what is explain
   * in the Stock&Commodities article.
   *
   * To calculate the "Unsmoothed stochastic RSI" with symetry like
   * explain in the article, keep the optInTimePeriod and optInFastK_Period
   * equal. Example:
   *
   *    unsmoothed stoch RSI 14 : optInTimePeriod   = 14
   *                              optInFastK_Period = 14
   *                              optInFastD_Period = 'x'
   *
   * The outFastK is the unsmoothed RSI discuss in the article.
   *
   * You can set the optInFastD_Period to smooth the RSI. The smooth
   * version will be found in outFastD. The outFastK will still contain
   * the unsmoothed stoch RSI. If you do not care about the smoothing of
   * the StochRSI, just leave optInFastD_Period to 1 and ignore outFastD.
   */
  outBegIdx.value = 0;
  outNBElement.value = 0;
  /* Adjust startIdx to account for the lookback period. */
  lookbackSTOCHF = stochfLookbackPositional(optInFastK_Period, optInFastD_Period, (optInFastD_MAType as MAType), core);
  lookbackTotal = rsiLookbackPositional(optInTimePeriod, core) + lookbackSTOCHF;
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
  tempArraySize = endIdx - startIdx + 1 + lookbackSTOCHF;
  tempRSIBuffer = new Float64Array(tempArraySize);
  const sub0 = rsiOpenAndFillInternal(core, inReal, startIdx - lookbackSTOCHF, optInTimePeriod, outBegIdx1, outNbElement1, tempRSIBuffer);
  if (outNbElement1.value === 0) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const sub1 = stochfOpenAndFillInternal(core, tempRSIBuffer.slice(0, (tempArraySize - 1) + 1), tempRSIBuffer.slice(0, (tempArraySize - 1) + 1), tempRSIBuffer.slice(0, (tempArraySize - 1) + 1), 0, optInFastK_Period, optInFastD_Period, (optInFastD_MAType as MAType), outBegIdx2, outNBElement, sc_outFastK, sc_outFastD);
  if (outNBElement.value === 0) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (outNBElement.value < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.optInFastK_Period = optInFastK_Period;
  sp.optInFastD_Period = optInFastD_Period;
  sp.optInFastD_MAType = optInFastD_MAType;
  sp.sub0 = sub0;
  sp.sub1 = sub1;
  sp.cur_outFastK = sc_outFastK[outNBElement.value - 1];
  sp.cur_outFastD = sc_outFastD[outNBElement.value - 1];
  return RetCode.SUCCESS;
}

/** @internal */
export function stochrsiOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInFastK_Period: number,
  optInFastD_Period: number,
  optInFastD_MAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outFastK: Float64Array,
  outFastD: Float64Array,
): StochrsiStream {
  const sp = StochrsiStream.of(core);
  const retCode = stochrsiOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx, outNBElement, outFastK, outFastD, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('STOCHRSI openAndFill', inReal.length, startIdx, stochrsiLookbackPositional(optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType as MAType, core));
  }
  if (retCode === RetCode.BAD_PARAM && stochrsiLookbackPositional(optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType as MAType, core) < 0) {
    throw rejectedParams('STOCHRSI openAndFill', PARAM_RANGES, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType);
  }
  throw streamFailure('STOCHRSI openAndFill', retCode);
}

/** @internal */
export function stochrsiOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInFastK_Period: number,
  optInFastD_Period: number,
  optInFastD_MAType: number,
): StochrsiStream {
  const sp = StochrsiStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outFastK = new Float64Array(1);
  const sink_outFastD = new Float64Array(1);
  const retCode = stochrsiOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx, outNBElement, sink_outFastK, sink_outFastD, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('STOCHRSI open', inReal.length, startIdx, stochrsiLookbackPositional(optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType as MAType, core));
  }
  if (retCode === RetCode.BAD_PARAM && stochrsiLookbackPositional(optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType as MAType, core) < 0) {
    throw rejectedParams('STOCHRSI open', PARAM_RANGES, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType);
  }
  throw streamFailure('STOCHRSI open', retCode);
}

/** @internal */
export function stochrsiOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInFastK_Period: number,
  optInFastD_Period: number,
  optInFastD_MAType: MAType | typeof Core.INTEGER_DEFAULT,
  core: Core = Core.DEFAULT,
): StochrsiStream {
  optInTimePeriod = requireInt32('STOCHRSI open', 'timePeriod', optInTimePeriod);
  optInFastK_Period = requireInt32('STOCHRSI open', 'fastKPeriod', optInFastK_Period);
  optInFastD_Period = requireInt32('STOCHRSI open', 'fastDPeriod', optInFastD_Period);
  optInFastD_MAType = requireInt32('STOCHRSI open', 'fastDMaType', optInFastD_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('STOCHRSI open', 'real', inReal);
  requireHistory('STOCHRSI open', 'real', inReal.length);
  return stochrsiOpenInternal(core, float64(inReal), 0, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType);
}

/** @internal */
export function stochrsiOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInFastK_Period: number,
  optInFastD_Period: number,
  optInFastD_MAType: MAType | typeof Core.INTEGER_DEFAULT,
  outFastK: Float64Array,
  outFastD: Float64Array,
  core: Core = Core.DEFAULT,
): StochrsiStream {
  optInTimePeriod = requireInt32('STOCHRSI openAndFill', 'timePeriod', optInTimePeriod);
  optInFastK_Period = requireInt32('STOCHRSI openAndFill', 'fastKPeriod', optInFastK_Period);
  optInFastD_Period = requireInt32('STOCHRSI openAndFill', 'fastDPeriod', optInFastD_Period);
  optInFastD_MAType = requireInt32('STOCHRSI openAndFill', 'fastDMaType', optInFastD_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('STOCHRSI openAndFill', 'real', inReal);
  requireHistory('STOCHRSI openAndFill', 'real', inReal.length);
  const guardLookback = stochrsiLookbackPositional(optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType as MAType, core);
  if (guardLookback < 0) {
    throw rejectedParams('STOCHRSI openAndFill', PARAM_RANGES, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType);
  }
  const guardOutLen = openFillCount('STOCHRSI openAndFill', inReal.length, guardLookback);
  requireOutput('STOCHRSI openAndFill', 'outFastK', outFastK, guardOutLen, Float64Array);
  requireOutput('STOCHRSI openAndFill', 'outFastD', outFastD, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outFastK.buffer === inRealF64.buffer && outFastK.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('STOCHRSI openAndFill', 'outFastK', 'real');
  }
  if (outFastD.buffer === inRealF64.buffer && outFastD.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('STOCHRSI openAndFill', 'outFastD', 'real');
  }
  if (outFastK.buffer === outFastD.buffer && outFastK.byteOffset === outFastD.byteOffset) {
    throw sharedMemory('STOCHRSI openAndFill', 'outFastK', 'outFastD');
  }
  if (guardOutLen > 0 && overlaps(outFastK, inRealF64)) {
    throw sharedMemory('STOCHRSI openAndFill', 'outFastK', 'real');
  }
  if (guardOutLen > 0 && overlaps(outFastD, inRealF64)) {
    throw sharedMemory('STOCHRSI openAndFill', 'outFastD', 'real');
  }
  if (guardOutLen > 0 && overlaps(outFastK, outFastD)) {
    throw sharedMemory('STOCHRSI openAndFill', 'outFastK', 'outFastD');
  }
  return stochrsiOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType, new MInteger(), new MInteger(), outFastK, outFastD);
}

/**
 * Opens a live STOCHRSI stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link stochrsi} there.
 *
 * @param real - Warm-up history. Source series fed into the RSI calculation.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link StochrsiOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `stochrsiLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32 or a parameter outside its
 *   range.
 *
 * @example
 * ```ts
 * import { MAType, stochrsiOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = stochrsiOpen(data, {
 *   timePeriod: 14,
 *   fastKPeriod: 5,
 *   fastDPeriod: 3,
 *   fastDMaType: MAType.SMA,
 * });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function stochrsiOpen(real: ArrayLike<number>, options?: StochrsiOptions): StochrsiStream {
  const o = optionsOf('STOCHRSI open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInFastK_Period = orDefault(o.fastKPeriod, Core.INTEGER_DEFAULT);
  const optInFastD_Period = orDefault(o.fastDPeriod, Core.INTEGER_DEFAULT);
  const optInFastD_MAType = orDefault(o.fastDMaType, Core.INTEGER_DEFAULT);
  const core = coreOf('STOCHRSI open', o.core);
  return stochrsiOpenPositional(real, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType, core);
}

/**
 * {@link stochrsiOpen} that also computes the outputs over the whole history in
 * the same pass, bit-identically to {@link stochrsi}.
 *
 * @param real - Warm-up history. Source series fed into the RSI calculation.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link StochrsiOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link stochrsi} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `stochrsiLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range
 *   or a buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { MAType, stochrsiOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, outFastK, outFastD } = stochrsiOpenAndFill(data, {
 *   timePeriod: 14,
 *   fastKPeriod: 5,
 *   fastDPeriod: 3,
 *   fastDMaType: MAType.SMA,
 * });
 * // outFastK[k] is the value at bar begIdx + k
 * ```
 */
export function stochrsiOpenAndFill(real: ArrayLike<number>, options?: StochrsiOptions): StochrsiResult & { stream: StochrsiStream } {
  const o = optionsOf('STOCHRSI openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInFastK_Period = orDefault(o.fastKPeriod, Core.INTEGER_DEFAULT);
  const optInFastD_Period = orDefault(o.fastDPeriod, Core.INTEGER_DEFAULT);
  const optInFastD_MAType = orDefault(o.fastDMaType, Core.INTEGER_DEFAULT);
  const core = coreOf('STOCHRSI openAndFill', o.core);
  const capacity = fillCapacity(stochrsiLookbackPositional(optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType, core), real);
  const outFastK = o.outFastK === undefined ? new Float64Array(capacity) : o.outFastK;
  const outFastD = o.outFastD === undefined ? new Float64Array(capacity) : o.outFastD;
  const stream = stochrsiOpenAndFillPositional(real, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outFastK, outFastD, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outFastK: outFastK.subarray(0, range.count), outFastD: outFastD.subarray(0, range.count) };
}

let scratchStochfOut: StochfOut | null = null;
