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
 *  090426 MF,CC  First version (issue #365).
 */

import { MAType, MATypes } from '../enums.ts';
import { badSink, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { StochOut, StochStream, stochLookbackPositional, stochOpenAndFillInternal, stochPositional } from './stoch.ts';

/** @internal */
export function kdjLookbackPositional(optInFastK_Period: number, optInSlowK_Period: number, optInSlowK_MAType: MAType | typeof Core.INTEGER_DEFAULT, optInSlowD_Period: number, optInSlowD_MAType: MAType | typeof Core.INTEGER_DEFAULT, core: Core = Core.DEFAULT): number {
  if ((optInFastK_Period | 0) !== optInFastK_Period) {
    return -1;
  }
  if ((optInSlowK_Period | 0) !== optInSlowK_Period) {
    return -1;
  }
  if ((optInSlowK_MAType | 0) !== optInSlowK_MAType) {
    return -1;
  }
  if ((optInSlowD_Period | 0) !== optInSlowD_Period) {
    return -1;
  }
  if ((optInSlowD_MAType | 0) !== optInSlowD_MAType) {
    return -1;
  }
  if (optInFastK_Period === Core.INTEGER_DEFAULT) {
    optInFastK_Period = 9;
  } else if (optInFastK_Period < 1 || optInFastK_Period > 100000) {
    return -1;
  }
  if (optInSlowK_Period === Core.INTEGER_DEFAULT) {
    optInSlowK_Period = 3;
  } else if (optInSlowK_Period < 1 || optInSlowK_Period > 100000) {
    return -1;
  }
  if (optInSlowK_MAType === Core.INTEGER_DEFAULT || optInSlowK_MAType === MAType.DEFAULT) {
    optInSlowK_MAType = MAType.RMA;
  } else if (optInSlowK_MAType < MATypes.Min || optInSlowK_MAType > MATypes.Max) {
    return -1;
  }
  if (optInSlowD_Period === Core.INTEGER_DEFAULT) {
    optInSlowD_Period = 3;
  } else if (optInSlowD_Period < 1 || optInSlowD_Period > 100000) {
    return -1;
  }
  if (optInSlowD_MAType === Core.INTEGER_DEFAULT || optInSlowD_MAType === MAType.DEFAULT) {
    optInSlowD_MAType = MAType.RMA;
  } else if (optInSlowD_MAType < MATypes.Min || optInSlowD_MAType > MATypes.Max) {
    return -1;
  }
  return stochLookbackPositional(optInFastK_Period, optInSlowK_Period, (optInSlowK_MAType as MAType), optInSlowD_Period, (optInSlowD_MAType as MAType), core);
}

/** @internal */
export function kdjDisplayShiftPositional(optInFastK_Period: number, optInSlowK_Period: number, optInSlowK_MAType: MAType | typeof Core.INTEGER_DEFAULT, optInSlowD_Period: number, optInSlowD_MAType: MAType | typeof Core.INTEGER_DEFAULT, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (kdjLookbackPositional(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 3) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function kdjImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  optInFastK_Period: number,
  optInSlowK_Period: number,
  optInSlowK_MAType: number,
  optInSlowD_Period: number,
  optInSlowD_MAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outK: Float64Array,
  outD: Float64Array,
  outJ: Float64Array,
): RetCode {
  let retCode: RetCode = RetCode.SUCCESS;
  let i = 0;
  let lookbackTotal = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInFastK_Period === Core.INTEGER_DEFAULT) {
    optInFastK_Period = 9;
  } else if (optInFastK_Period < 1 || optInFastK_Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowK_Period === Core.INTEGER_DEFAULT) {
    optInSlowK_Period = 3;
  } else if (optInSlowK_Period < 1 || optInSlowK_Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowK_MAType === Core.INTEGER_DEFAULT || optInSlowK_MAType === MAType.DEFAULT) {
    optInSlowK_MAType = MAType.RMA;
  } else if (optInSlowK_MAType < MATypes.Min || optInSlowK_MAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowD_Period === Core.INTEGER_DEFAULT) {
    optInSlowD_Period = 3;
  } else if (optInSlowD_Period < 1 || optInSlowD_Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowD_MAType === Core.INTEGER_DEFAULT || optInSlowD_MAType === MAType.DEFAULT) {
    optInSlowD_MAType = MAType.RMA;
  } else if (optInSlowD_MAType < MATypes.Min || optInSlowD_MAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if ((outK.buffer === outD.buffer && outK.byteOffset === outD.byteOffset) || (outK.buffer === outJ.buffer && outK.byteOffset === outJ.byteOffset) || (outD.buffer === outJ.buffer && outD.byteOffset === outJ.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  lookbackTotal = kdjLookbackPositional(optInFastK_Period, optInSlowK_Period, (optInSlowK_MAType as MAType), optInSlowD_Period, (optInSlowD_MAType as MAType), core);
  /* Nothing to produce: the range ends before the lookback. Answering here
   * keeps the sub-call out of the phantom-I/O sweep's zero-length range, where
   * its own argument check would reject before any array is touched.
   */
  if (lookbackTotal > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  const _xr0 = stochPositional(startIdx, endIdx, inHigh, inLow, inClose, optInFastK_Period, optInSlowK_Period, (optInSlowK_MAType as MAType), optInSlowD_Period, (optInSlowD_MAType as MAType), outK, outD, core);
  outBegIdx.value = _xr0.begIdx;
  outNBElement.value = _xr0.count;
  retCode = RetCode.SUCCESS;
  if (outNBElement.value === 0) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return retCode;
  }
  /* Keep this one Sub expression: spelling it as an Add of a negated term
   * would arm the multiply-add fusion and move the last bits of J.
   */
  for (i = 0; i < outNBElement.value; i += 1) {
    outJ[i] = 3.0 * outK[i] - 2.0 * outD[i];
  }
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['fastKPeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['slowKPeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['slowKMaType', Core.INTEGER_DEFAULT, MATypes.Min, MATypes.Max, 'a MAType'], ['slowDPeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['slowDMaType', Core.INTEGER_DEFAULT, MATypes.Min, MATypes.Max, 'a MAType']] as const;

/** @internal */
export function kdjPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInFastK_Period: number,
  optInSlowK_Period: number,
  optInSlowK_MAType: MAType | typeof Core.INTEGER_DEFAULT,
  optInSlowD_Period: number,
  optInSlowD_MAType: MAType | typeof Core.INTEGER_DEFAULT,
  outK: Float64Array,
  outD: Float64Array,
  outJ: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('KDJ', 'startIdx', startIdx);
  endIdx = requireInt32('KDJ', 'endIdx', endIdx);
  optInFastK_Period = requireInt32('KDJ', 'fastKPeriod', optInFastK_Period);
  optInSlowK_Period = requireInt32('KDJ', 'slowKPeriod', optInSlowK_Period);
  optInSlowK_MAType = requireInt32('KDJ', 'slowKMaType', optInSlowK_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  optInSlowD_Period = requireInt32('KDJ', 'slowDPeriod', optInSlowD_Period);
  optInSlowD_MAType = requireInt32('KDJ', 'slowDMaType', optInSlowD_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireIndexRange('KDJ', startIdx, endIdx);
  const guardLookback = kdjLookbackPositional(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, core);
  if (guardLookback < 0) {
    throw rejectedParams('KDJ', PARAM_RANGES, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType);
  }
  const guardStart = clampedStart('KDJ', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('KDJ', 'prices.high', inHigh, guardInLen);
  requireLength('KDJ', 'prices.low', inLow, guardInLen);
  requireLength('KDJ', 'prices.close', inClose, guardInLen);
  requireOutput('KDJ', 'outK', outK, guardOutLen, Float64Array);
  requireOutput('KDJ', 'outD', outD, guardOutLen, Float64Array);
  requireOutput('KDJ', 'outJ', outJ, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outK, inHighF64)) {
    throw sharedMemory('KDJ', 'outK', 'prices.high');
  }
  if (overlapsPartly(outK, inLowF64)) {
    throw sharedMemory('KDJ', 'outK', 'prices.low');
  }
  if (overlapsPartly(outK, inCloseF64)) {
    throw sharedMemory('KDJ', 'outK', 'prices.close');
  }
  if (overlapsPartly(outD, inHighF64)) {
    throw sharedMemory('KDJ', 'outD', 'prices.high');
  }
  if (overlapsPartly(outD, inLowF64)) {
    throw sharedMemory('KDJ', 'outD', 'prices.low');
  }
  if (overlapsPartly(outD, inCloseF64)) {
    throw sharedMemory('KDJ', 'outD', 'prices.close');
  }
  if (overlapsPartly(outJ, inHighF64)) {
    throw sharedMemory('KDJ', 'outJ', 'prices.high');
  }
  if (overlapsPartly(outJ, inLowF64)) {
    throw sharedMemory('KDJ', 'outJ', 'prices.low');
  }
  if (overlapsPartly(outJ, inCloseF64)) {
    throw sharedMemory('KDJ', 'outJ', 'prices.close');
  }
  if (overlaps(outK, outD)) {
    throw sharedMemory('KDJ', 'outK', 'outD');
  }
  if (overlaps(outK, outJ)) {
    throw sharedMemory('KDJ', 'outK', 'outJ');
  }
  if (overlaps(outD, outJ)) {
    throw sharedMemory('KDJ', 'outD', 'outJ');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = kdjImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, outK, outD, outJ);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('KDJ', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link kdj}, {@link kdjLookback}, {@link kdjDisplayShift},
 * {@link kdjOpen} and {@link kdjOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface KdjOptions {
  /**
   * Lookback window for the raw stochastic high-low range (default 9; range 1 to
   * 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  fastKPeriod?: number | undefined;
  /**
   * Smoothing period turning the raw stochastic into K (default 3; range 1 to
   * 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  slowKPeriod?: number | undefined;
  /**
   * MA type used to smooth into K (default `MAType.RMA`; values: 0=SMA, 1=EMA,
   * 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA, 10=DISABLED,
   * 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA; `Core.INTEGER_DEFAULT` or
   * `MAType.DEFAULT` selects the default).
   */
  slowKMaType?: MAType | typeof Core.INTEGER_DEFAULT | undefined;
  /**
   * Smoothing period for the D signal line (default 3; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  slowDPeriod?: number | undefined;
  /**
   * MA type used for the D line (default `MAType.RMA`; values: 0=SMA, 1=EMA, 2=WMA,
   * 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT,
   * 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT`
   * selects the default).
   */
  slowDMaType?: MAType | typeof Core.INTEGER_DEFAULT | undefined;
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
   * Buffer for `outK`, written from index 0 and allocated when omitted. It must hold
   * the values produced (for a batch call the input length always suffices, for an
   * `OpenAndFill` `historyLen - lookback`), and share no memory with another buffer
   * or with an input; a batch call may compute in place over a `Float64Array` input
   * of the same length.
   */
  outK?: Float64Array | undefined;
  /**
   * Buffer for `outD`, written from index 0 and allocated when omitted. It must hold
   * the values produced (for a batch call the input length always suffices, for an
   * `OpenAndFill` `historyLen - lookback`), and share no memory with another buffer
   * or with an input; a batch call may compute in place over a `Float64Array` input
   * of the same length.
   */
  outD?: Float64Array | undefined;
  /**
   * Buffer for `outJ`, written from index 0 and allocated when omitted. It must hold
   * the values produced (for a batch call the input length always suffices, for an
   * `OpenAndFill` `historyLen - lookback`), and share no memory with another buffer
   * or with an input; a batch call may compute in place over a `Float64Array` input
   * of the same length.
   */
  outJ?: Float64Array | undefined;
}

/** What {@link kdj} returns, and {@link kdjOpenAndFill} with its stream. */
export interface KdjResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Raw stochastic smoothed by SlowK_Period MA. `outK[k]` is the value at input bar
   * `begIdx + k`.
   */
  outK: Float64Array;
  /**
   * Signal line: K smoothed by SlowD_Period MA. `outD[k]` is the value at input bar
   * `begIdx + k`.
   */
  outD: Float64Array;
  /**
   * Divergence line, three parts K less two parts D. `outJ[k]` is the value at input
   * bar `begIdx + k`.
   */
  outJ: Float64Array;
}

/**
 * Number of leading input bars {@link kdj} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link KdjOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function kdjLookback(options?: KdjOptions): number {
  const o = optionsOf('KDJ lookback', options);
  return kdjLookbackPositional(orDefault(o.fastKPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowKPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowKMaType, Core.INTEGER_DEFAULT), orDefault(o.slowDPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowDMaType, Core.INTEGER_DEFAULT), coreOf('KDJ lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link kdj}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link KdjOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function kdjDisplayShift(outputIdx: number, options?: KdjOptions): number {
  const o = optionsOf('KDJ displayShift', options);
  return kdjDisplayShiftPositional(orDefault(o.fastKPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowKPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowKMaType, Core.INTEGER_DEFAULT), orDefault(o.slowDPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowDMaType, Core.INTEGER_DEFAULT), outputIdx, coreOf('KDJ displayShift', o.core));
}

/**
 * The stochastic oscillator as it is drawn on Chinese-market platforms: the
 * Slow-%K and Slow-%D lines plus a third line J that amplifies the gap between
 * them. K and D are read like any stochastic — above 80 overbought, below 20
 * oversold, a K/D crossing signalling a momentum shift — while J is a divergence
 * gauge whose excursions outside the 0-100 band mark the strongest moves. Both
 * smoothing stages default to Wilder's moving average, which is the smoother the
 * original formula language specifies; selecting a simple moving average for both
 * reproduces the classic Slow Stochastic with a J line attached.
 *
 * Formula and more info at
 * [ta-lib.org/functions/kdj](https://ta-lib.org/functions/kdj).
 *
 * **Notes**
 * - The default smoothing is Wilder's moving average. The originating 通达信
 *   (Tongdaxin) formula language writes each stage as `SMA(X, N, 1)`, a recurrence
 *   with weight 1/N on the new value, which is Wilder's smoothing under another
 *   name — not a simple average.
 * - How that recurrence is started is a TA-Lib house convention, not something the
 *   originating specification settles: like every other Wilder-smoothed function
 *   here, the first value is the simple average of the first N inputs, and callers
 *   who want the transient gone set the unstable period. Platforms that seed the
 *   recurrence at 50, or at the first raw value, differ for the first several
 *   dozen bars.
 * - J is deliberately unbounded. It leaves the 0-100 band routinely and is never
 *   clamped.
 * - When the high-low range over the window is zero, the raw stochastic is set to
 *   0 instead of being undefined.
 *
 * `outK[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link kdjLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `high`: High
 *   price of each bar. `low`: Low price of each bar. `close`: Close price of each
 *   bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link KdjOptions}.
 * @returns `begIdx` and the values; see {@link KdjResult}. Each output is a view
 *   of its buffer, the caller's or a new one.
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
 * import { MAType, kdj } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { begIdx, outK, outD, outJ } = kdj({ high, low, close }, {
 *   fastKPeriod: 9,
 *   slowKPeriod: 3,
 *   slowKMaType: MAType.RMA,
 *   slowDPeriod: 3,
 *   slowDMaType: MAType.RMA,
 * });
 * // outK[k] is the value at bar begIdx + k
 * ```
 * @see [`stoch`](https://ta-lib.org/functions/stoch)
 * @see [`stochf`](https://ta-lib.org/functions/stochf)
 * @see [`rma`](https://ta-lib.org/functions/rma)
 * @see [`ma`](https://ta-lib.org/functions/ma)
 */
export function kdj(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: KdjOptions): KdjResult {
  requireArgument('KDJ', 'prices', prices);
  const o = optionsOf('KDJ', options);
  if (o.endIdx === undefined) {
    requireSeries('KDJ', 'prices.high', prices.high);
    requireSeries('KDJ', 'prices.low', prices.low);
    requireSeries('KDJ', 'prices.close', prices.close);
  }
  const optInFastK_Period = orDefault(o.fastKPeriod, Core.INTEGER_DEFAULT);
  const optInSlowK_Period = orDefault(o.slowKPeriod, Core.INTEGER_DEFAULT);
  const optInSlowK_MAType = orDefault(o.slowKMaType, Core.INTEGER_DEFAULT);
  const optInSlowD_Period = orDefault(o.slowDPeriod, Core.INTEGER_DEFAULT);
  const optInSlowD_MAType = orDefault(o.slowDMaType, Core.INTEGER_DEFAULT);
  const core = coreOf('KDJ', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, kdjLookbackPositional(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, core), prices.high, prices.low, prices.close);
  const outK = o.outK === undefined ? new Float64Array(capacity) : o.outK;
  const outD = o.outD === undefined ? new Float64Array(capacity) : o.outD;
  const outJ = o.outJ === undefined ? new Float64Array(capacity) : o.outJ;
  const range = kdjPositional(startIdx, endIdx, prices.high, prices.low, prices.close, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, outK, outD, outJ, core);
  return { begIdx: range.begIdx, outK: outK.subarray(0, range.count), outD: outD.subarray(0, range.count), outJ: outJ.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live KDJ stream: one value per closed bar, bit-identical to {@link kdj} over
 * the same series. Open it with {@link kdjOpen} or {@link kdjOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/kdj](https://ta-lib.org/functions/kdj).
 */
export class KdjStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInFastK_Period: number = 0;
  /** @internal */
  optInSlowK_Period: number = 0;
  /** @internal */
  optInSlowK_MAType: number = 0;
  /** @internal */
  optInSlowD_Period: number = 0;
  /** @internal */
  optInSlowD_MAType: number = 0;
  /** @internal */
  cur_outK: number = 0;
  /** @internal */
  cur_outD: number = 0;
  /** @internal */
  cur_outJ: number = 0;
  /** @internal */
  sub0!: StochStream;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): KdjStream {
    return new KdjStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link kdj} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('KDJ advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns its outputs.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @param out - Receives the outputs and is returned; a new {@link KdjOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included, before anything moves.
   *   `TALibError` with code `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached
   *   bar `Core.INDEX_MAX`.
   */
  update(high: number, low: number, close: number, out?: KdjOut): KdjOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('KDJ update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('KDJ update', 'KdjOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('KDJ update', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    kdjStepImpl(this, high, low, close);
    this.outRangeCount++;
    if (out === undefined) {
      out = new KdjOut();
    }
    out.outK = this.cur_outK;
    out.outD = this.cur_outD;
    out.outJ = this.cur_outJ;
    return out;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @param out - Receives the outputs and is returned; a new {@link KdjOut} when
   *   omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included.
   */
  peek(high: number, low: number, close: number, out?: KdjOut): KdjOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('KDJ peek', 'KdjOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('KDJ peek', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
    const inClose = close;
    let cur_outK = 0;
    let cur_outD = 0;
    let cur_outJ = 0;
    {
      const subOut0 = (scratchStochOut ??= new StochOut());
      sp.sub0.peek(inHigh, inLow, inClose, subOut0);
      cur_outK = subOut0.outSlowK;
      cur_outD = subOut0.outSlowD;
    }
    cur_outJ = 3.0 * cur_outK - 2.0 * cur_outD;
    if (out === undefined) {
      out = new KdjOut();
    }
    out.outK = cur_outK;
    out.outD = cur_outD;
    out.outJ = cur_outJ;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link KdjOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a sink that is not an object,
   *   `null` included.
   */
  value(out?: KdjOut): KdjOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('KDJ value', 'KdjOut');
    }
    if (out === undefined) {
      out = new KdjOut();
    }
    out.outK = this.cur_outK;
    out.outD = this.cur_outD;
    out.outJ = this.cur_outJ;
    return out;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): KdjStream {
    const c = new KdjStream(this.core);
    c.optInFastK_Period = this.optInFastK_Period;
    c.optInSlowK_Period = this.optInSlowK_Period;
    c.optInSlowK_MAType = this.optInSlowK_MAType;
    c.optInSlowD_Period = this.optInSlowD_Period;
    c.optInSlowD_MAType = this.optInSlowD_MAType;
    c.cur_outK = this.cur_outK;
    c.cur_outD = this.cur_outD;
    c.cur_outJ = this.cur_outJ;
    c.sub0 = this.sub0.clone();
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one KDJ bar, as `update`, `peek` and `value` return them. Passed
 * back to one of them, it is filled and returned again, with no allocation; it
 * holds its values until the next call that writes it. A new one holds `NaN`.
 */
export class KdjOut {
  /** Raw stochastic smoothed by SlowK_Period MA. */
  outK: number = NaN;
  /** Signal line: K smoothed by SlowD_Period MA. */
  outD: number = NaN;
  /** Divergence line, three parts K less two parts D. */
  outJ: number = NaN;
}

function kdjStepImpl(sp: KdjStream, inHigh: number, inLow: number, inClose: number): void {
  let cur_outK = 0;
  let cur_outD = 0;
  let cur_outJ = 0;
  {
    const subOut0 = (scratchStochOut ??= new StochOut());
    sp.sub0.update(inHigh, inLow, inClose, subOut0);
    cur_outK = subOut0.outSlowK;
    cur_outD = subOut0.outSlowD;
  }
  cur_outJ = 3.0 * cur_outK - 2.0 * cur_outD;
  sp.cur_outK = cur_outK;
  sp.cur_outD = cur_outD;
  sp.cur_outJ = cur_outJ;
}

function kdjOpenImpl(
  sp: KdjStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInFastK_Period: number,
  optInSlowK_Period: number,
  optInSlowK_MAType: number,
  optInSlowD_Period: number,
  optInSlowD_MAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outK: Float64Array,
  outD: Float64Array,
  outJ: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let i = 0;
  let lookbackTotal = 0;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length || inClose.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInFastK_Period === Core.INTEGER_DEFAULT) {
    optInFastK_Period = 9;
  } else if (optInFastK_Period < 1 || optInFastK_Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowK_Period === Core.INTEGER_DEFAULT) {
    optInSlowK_Period = 3;
  } else if (optInSlowK_Period < 1 || optInSlowK_Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowK_MAType === Core.INTEGER_DEFAULT || optInSlowK_MAType === MAType.DEFAULT) {
    optInSlowK_MAType = MAType.RMA;
  } else if (optInSlowK_MAType < MATypes.Min || optInSlowK_MAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowD_Period === Core.INTEGER_DEFAULT) {
    optInSlowD_Period = 3;
  } else if (optInSlowD_Period < 1 || optInSlowD_Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowD_MAType === Core.INTEGER_DEFAULT || optInSlowD_MAType === MAType.DEFAULT) {
    optInSlowD_MAType = MAType.RMA;
  } else if (optInSlowD_MAType < MATypes.Min || optInSlowD_MAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen < kdjLookbackPositional(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType as MAType, optInSlowD_Period, optInSlowD_MAType as MAType, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const sc_outK = outStride === 1 ? outK : new Float64Array(historyLen);
  const sc_outD = outStride === 1 ? outD : new Float64Array(historyLen);
  const sc_outJ = outStride === 1 ? outJ : new Float64Array(historyLen);
  lookbackTotal = kdjLookbackPositional(optInFastK_Period, optInSlowK_Period, (optInSlowK_MAType as MAType), optInSlowD_Period, (optInSlowD_MAType as MAType), core);
  /* Nothing to produce: the range ends before the lookback. Answering here
   * keeps the sub-call out of the phantom-I/O sweep's zero-length range, where
   * its own argument check would reject before any array is touched.
   */
  if (lookbackTotal > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const sub0 = stochOpenAndFillInternal(core, inHigh, inLow, inClose, startIdx, optInFastK_Period, optInSlowK_Period, (optInSlowK_MAType as MAType), optInSlowD_Period, (optInSlowD_MAType as MAType), outBegIdx, outNBElement, sc_outK, sc_outD);
  if (outNBElement.value === 0) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Keep this one Sub expression: spelling it as an Add of a negated term
   * would arm the multiply-add fusion and move the last bits of J.
   */
  for (i = 0; i < outNBElement.value; i += 1) {
    sc_outJ[i] = 3.0 * sc_outK[i] - 2.0 * sc_outD[i];
  }
  if (outNBElement.value < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  sp.optInFastK_Period = optInFastK_Period;
  sp.optInSlowK_Period = optInSlowK_Period;
  sp.optInSlowK_MAType = optInSlowK_MAType;
  sp.optInSlowD_Period = optInSlowD_Period;
  sp.optInSlowD_MAType = optInSlowD_MAType;
  sp.sub0 = sub0;
  sp.cur_outK = sc_outK[outNBElement.value - 1];
  sp.cur_outD = sc_outD[outNBElement.value - 1];
  sp.cur_outJ = sc_outJ[outNBElement.value - 1];
  return RetCode.SUCCESS;
}

/** @internal */
export function kdjOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInFastK_Period: number,
  optInSlowK_Period: number,
  optInSlowK_MAType: number,
  optInSlowD_Period: number,
  optInSlowD_MAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outK: Float64Array,
  outD: Float64Array,
  outJ: Float64Array,
): KdjStream {
  const sp = KdjStream.of(core);
  const retCode = kdjOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, outK, outD, outJ, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('KDJ openAndFill', inHigh.length, startIdx, kdjLookbackPositional(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType as MAType, optInSlowD_Period, optInSlowD_MAType as MAType, core));
  }
  if (retCode === RetCode.BAD_PARAM && kdjLookbackPositional(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType as MAType, optInSlowD_Period, optInSlowD_MAType as MAType, core) < 0) {
    throw rejectedParams('KDJ openAndFill', PARAM_RANGES, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType);
  }
  throw streamFailure('KDJ openAndFill', retCode);
}

/** @internal */
export function kdjOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInFastK_Period: number,
  optInSlowK_Period: number,
  optInSlowK_MAType: number,
  optInSlowD_Period: number,
  optInSlowD_MAType: number,
): KdjStream {
  const sp = KdjStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outK = new Float64Array(1);
  const sink_outD = new Float64Array(1);
  const sink_outJ = new Float64Array(1);
  const retCode = kdjOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, sink_outK, sink_outD, sink_outJ, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('KDJ open', inHigh.length, startIdx, kdjLookbackPositional(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType as MAType, optInSlowD_Period, optInSlowD_MAType as MAType, core));
  }
  if (retCode === RetCode.BAD_PARAM && kdjLookbackPositional(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType as MAType, optInSlowD_Period, optInSlowD_MAType as MAType, core) < 0) {
    throw rejectedParams('KDJ open', PARAM_RANGES, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType);
  }
  throw streamFailure('KDJ open', retCode);
}

/** @internal */
export function kdjOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInFastK_Period: number,
  optInSlowK_Period: number,
  optInSlowK_MAType: MAType | typeof Core.INTEGER_DEFAULT,
  optInSlowD_Period: number,
  optInSlowD_MAType: MAType | typeof Core.INTEGER_DEFAULT,
  core: Core = Core.DEFAULT,
): KdjStream {
  optInFastK_Period = requireInt32('KDJ open', 'fastKPeriod', optInFastK_Period);
  optInSlowK_Period = requireInt32('KDJ open', 'slowKPeriod', optInSlowK_Period);
  optInSlowK_MAType = requireInt32('KDJ open', 'slowKMaType', optInSlowK_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  optInSlowD_Period = requireInt32('KDJ open', 'slowDPeriod', optInSlowD_Period);
  optInSlowD_MAType = requireInt32('KDJ open', 'slowDMaType', optInSlowD_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('KDJ open', 'prices.high', inHigh);
  requireHistory('KDJ open', 'prices.high', inHigh.length);
  requireArgument('KDJ open', 'prices.low', inLow);
  requireArgument('KDJ open', 'prices.close', inClose);
  requireHistoryLength('KDJ open', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('KDJ open', 'prices.close', inClose.length, inHigh.length);
  return kdjOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType);
}

/** @internal */
export function kdjOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInFastK_Period: number,
  optInSlowK_Period: number,
  optInSlowK_MAType: MAType | typeof Core.INTEGER_DEFAULT,
  optInSlowD_Period: number,
  optInSlowD_MAType: MAType | typeof Core.INTEGER_DEFAULT,
  outK: Float64Array,
  outD: Float64Array,
  outJ: Float64Array,
  core: Core = Core.DEFAULT,
): KdjStream {
  optInFastK_Period = requireInt32('KDJ openAndFill', 'fastKPeriod', optInFastK_Period);
  optInSlowK_Period = requireInt32('KDJ openAndFill', 'slowKPeriod', optInSlowK_Period);
  optInSlowK_MAType = requireInt32('KDJ openAndFill', 'slowKMaType', optInSlowK_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  optInSlowD_Period = requireInt32('KDJ openAndFill', 'slowDPeriod', optInSlowD_Period);
  optInSlowD_MAType = requireInt32('KDJ openAndFill', 'slowDMaType', optInSlowD_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('KDJ openAndFill', 'prices.high', inHigh);
  requireHistory('KDJ openAndFill', 'prices.high', inHigh.length);
  requireArgument('KDJ openAndFill', 'prices.low', inLow);
  requireArgument('KDJ openAndFill', 'prices.close', inClose);
  const guardLookback = kdjLookbackPositional(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType as MAType, optInSlowD_Period, optInSlowD_MAType as MAType, core);
  if (guardLookback < 0) {
    throw rejectedParams('KDJ openAndFill', PARAM_RANGES, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType);
  }
  const guardOutLen = openFillCount('KDJ openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('KDJ openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('KDJ openAndFill', 'prices.close', inClose.length, inHigh.length);
  requireOutput('KDJ openAndFill', 'outK', outK, guardOutLen, Float64Array);
  requireOutput('KDJ openAndFill', 'outD', outD, guardOutLen, Float64Array);
  requireOutput('KDJ openAndFill', 'outJ', outJ, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outK.buffer === inHighF64.buffer && outK.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('KDJ openAndFill', 'outK', 'prices.high');
  }
  if (outK.buffer === inLowF64.buffer && outK.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('KDJ openAndFill', 'outK', 'prices.low');
  }
  if (outK.buffer === inCloseF64.buffer && outK.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('KDJ openAndFill', 'outK', 'prices.close');
  }
  if (outD.buffer === inHighF64.buffer && outD.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('KDJ openAndFill', 'outD', 'prices.high');
  }
  if (outD.buffer === inLowF64.buffer && outD.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('KDJ openAndFill', 'outD', 'prices.low');
  }
  if (outD.buffer === inCloseF64.buffer && outD.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('KDJ openAndFill', 'outD', 'prices.close');
  }
  if (outJ.buffer === inHighF64.buffer && outJ.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('KDJ openAndFill', 'outJ', 'prices.high');
  }
  if (outJ.buffer === inLowF64.buffer && outJ.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('KDJ openAndFill', 'outJ', 'prices.low');
  }
  if (outJ.buffer === inCloseF64.buffer && outJ.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('KDJ openAndFill', 'outJ', 'prices.close');
  }
  if (outK.buffer === outD.buffer && outK.byteOffset === outD.byteOffset) {
    throw sharedMemory('KDJ openAndFill', 'outK', 'outD');
  }
  if (outK.buffer === outJ.buffer && outK.byteOffset === outJ.byteOffset) {
    throw sharedMemory('KDJ openAndFill', 'outK', 'outJ');
  }
  if (outD.buffer === outJ.buffer && outD.byteOffset === outJ.byteOffset) {
    throw sharedMemory('KDJ openAndFill', 'outD', 'outJ');
  }
  if (guardOutLen > 0 && overlaps(outK, inHighF64)) {
    throw sharedMemory('KDJ openAndFill', 'outK', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outK, inLowF64)) {
    throw sharedMemory('KDJ openAndFill', 'outK', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outK, inCloseF64)) {
    throw sharedMemory('KDJ openAndFill', 'outK', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outD, inHighF64)) {
    throw sharedMemory('KDJ openAndFill', 'outD', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outD, inLowF64)) {
    throw sharedMemory('KDJ openAndFill', 'outD', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outD, inCloseF64)) {
    throw sharedMemory('KDJ openAndFill', 'outD', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outJ, inHighF64)) {
    throw sharedMemory('KDJ openAndFill', 'outJ', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outJ, inLowF64)) {
    throw sharedMemory('KDJ openAndFill', 'outJ', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outJ, inCloseF64)) {
    throw sharedMemory('KDJ openAndFill', 'outJ', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outK, outD)) {
    throw sharedMemory('KDJ openAndFill', 'outK', 'outD');
  }
  if (guardOutLen > 0 && overlaps(outK, outJ)) {
    throw sharedMemory('KDJ openAndFill', 'outK', 'outJ');
  }
  if (guardOutLen > 0 && overlaps(outD, outJ)) {
    throw sharedMemory('KDJ openAndFill', 'outD', 'outJ');
  }
  return kdjOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, new MInteger(), new MInteger(), outK, outD, outJ);
}

/**
 * Opens a live KDJ stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link kdj} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link KdjOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `kdjLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or
 *   histories of different lengths.
 *
 * @example
 * ```ts
 * import { MAType, kdjOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = kdjOpen({ high, low, close }, {
 *   fastKPeriod: 9,
 *   slowKPeriod: 3,
 *   slowKMaType: MAType.RMA,
 *   slowDPeriod: 3,
 *   slowDMaType: MAType.RMA,
 * });
 * const peeked = stream.peek(101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function kdjOpen(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: KdjOptions): KdjStream {
  requireArgument('KDJ open', 'prices', prices);
  const o = optionsOf('KDJ open', options);
  const optInFastK_Period = orDefault(o.fastKPeriod, Core.INTEGER_DEFAULT);
  const optInSlowK_Period = orDefault(o.slowKPeriod, Core.INTEGER_DEFAULT);
  const optInSlowK_MAType = orDefault(o.slowKMaType, Core.INTEGER_DEFAULT);
  const optInSlowD_Period = orDefault(o.slowDPeriod, Core.INTEGER_DEFAULT);
  const optInSlowD_MAType = orDefault(o.slowDMaType, Core.INTEGER_DEFAULT);
  const core = coreOf('KDJ open', o.core);
  return kdjOpenPositional(prices.high, prices.low, prices.close, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, core);
}

/**
 * {@link kdjOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link kdj}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link KdjOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link kdj} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `kdjLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { MAType, kdjOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, outK, outD, outJ } = kdjOpenAndFill({ high, low, close }, {
 *   fastKPeriod: 9,
 *   slowKPeriod: 3,
 *   slowKMaType: MAType.RMA,
 *   slowDPeriod: 3,
 *   slowDMaType: MAType.RMA,
 * });
 * // outK[k] is the value at bar begIdx + k
 * ```
 */
export function kdjOpenAndFill(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: KdjOptions): KdjResult & { stream: KdjStream } {
  requireArgument('KDJ openAndFill', 'prices', prices);
  const o = optionsOf('KDJ openAndFill', options);
  const optInFastK_Period = orDefault(o.fastKPeriod, Core.INTEGER_DEFAULT);
  const optInSlowK_Period = orDefault(o.slowKPeriod, Core.INTEGER_DEFAULT);
  const optInSlowK_MAType = orDefault(o.slowKMaType, Core.INTEGER_DEFAULT);
  const optInSlowD_Period = orDefault(o.slowDPeriod, Core.INTEGER_DEFAULT);
  const optInSlowD_MAType = orDefault(o.slowDMaType, Core.INTEGER_DEFAULT);
  const core = coreOf('KDJ openAndFill', o.core);
  const capacity = fillCapacity(kdjLookbackPositional(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, core), prices.high, prices.low, prices.close);
  const outK = o.outK === undefined ? new Float64Array(capacity) : o.outK;
  const outD = o.outD === undefined ? new Float64Array(capacity) : o.outD;
  const outJ = o.outJ === undefined ? new Float64Array(capacity) : o.outJ;
  const stream = kdjOpenAndFillPositional(prices.high, prices.low, prices.close, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, outK, outD, outJ, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outK: outK.subarray(0, range.count), outD: outD.subarray(0, range.count), outJ: outJ.subarray(0, range.count) };
}

let scratchStochOut: StochOut | null = null;
