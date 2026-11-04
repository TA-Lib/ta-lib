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
import { badSink, clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { StochOut, StochStream, stoch, stochLookback, stochOpenAndFillInternal } from './stoch.ts';

/**
 * Number of leading input bars {@link kdj} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInFastK_Period - Lookback window for the raw stochastic high-low range
 *   (default 9; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowK_Period - Smoothing period turning the raw stochastic into K
 *   (default 3; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowK_MAType - MA type used to smooth into K (default `MAType.RMA`;
 *   values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3,
 *   9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
 *   `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param optInSlowD_Period - Smoothing period for the D signal line (default 3;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowD_MAType - MA type used for the D line (default `MAType.RMA`;
 *   values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3,
 *   9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
 *   `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function kdjLookback(optInFastK_Period: number, optInSlowK_Period: number, optInSlowK_MAType: MAType | typeof Core.INTEGER_DEFAULT, optInSlowD_Period: number, optInSlowD_MAType: MAType | typeof Core.INTEGER_DEFAULT, core: Core = Core.DEFAULT): number {
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
  return stochLookback(optInFastK_Period, optInSlowK_Period, (optInSlowK_MAType as MAType), optInSlowD_Period, (optInSlowD_MAType as MAType), core);
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link kdj}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInFastK_Period - Lookback window for the raw stochastic high-low range
 *   (default 9; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowK_Period - Smoothing period turning the raw stochastic into K
 *   (default 3; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowK_MAType - MA type used to smooth into K (default `MAType.RMA`;
 *   values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3,
 *   9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
 *   `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param optInSlowD_Period - Smoothing period for the D signal line (default 3;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowD_MAType - MA type used for the D line (default `MAType.RMA`;
 *   values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3,
 *   9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
 *   `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function kdjDisplayShift(optInFastK_Period: number, optInSlowK_Period: number, optInSlowK_MAType: MAType | typeof Core.INTEGER_DEFAULT, optInSlowD_Period: number, optInSlowD_MAType: MAType | typeof Core.INTEGER_DEFAULT, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (kdjLookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, core) < 0) {
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
  lookbackTotal = kdjLookback(optInFastK_Period, optInSlowK_Period, (optInSlowK_MAType as MAType), optInSlowD_Period, (optInSlowD_MAType as MAType), core);
  /* Nothing to produce: the range ends before the lookback. Answering here
   * keeps the sub-call out of the phantom-I/O sweep's zero-length range, where
   * its own argument check would reject before any array is touched.
   */
  if (lookbackTotal > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  const _xr0 = stoch(startIdx, endIdx, inHigh, inLow, inClose, optInFastK_Period, optInSlowK_Period, (optInSlowK_MAType as MAType), optInSlowD_Period, (optInSlowD_MAType as MAType), outK, outD, core);
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
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link kdjLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param optInFastK_Period - Lookback window for the raw stochastic high-low range
 *   (default 9; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowK_Period - Smoothing period turning the raw stochastic into K
 *   (default 3; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowK_MAType - MA type used to smooth into K (default `MAType.RMA`;
 *   values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3,
 *   9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
 *   `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param optInSlowD_Period - Smoothing period for the D signal line (default 3;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowD_MAType - MA type used for the D line (default `MAType.RMA`;
 *   values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3,
 *   9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
 *   `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param outK - Raw stochastic smoothed by SlowK_Period MA. Must hold the values
 *   produced and never be empty; the input length always suffices.
 * @param outD - Signal line: K smoothed by SlowD_Period MA. Must hold the values
 *   produced and never be empty; the input length always suffices.
 * @param outJ - Divergence line, three parts K less two parts D. Must hold the
 *   values produced and never be empty; the input length always suffices.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The range written: `begIdx` is the first bar with a value, `count` how
 *   many were written.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for an argument that is not an int32, a
 *   parameter outside its range, an array too short for the range, an empty output,
 *   two outputs sharing memory or an output sharing memory with a `Float64Array`
 *   input it is not. Checked before anything is written.
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
 * const kOut = new Float64Array(high.length);
 * const d = new Float64Array(high.length);
 * const j = new Float64Array(high.length);
 * const { begIdx, count } = kdj(
 *   0, high.length - 1,
 *   high, low, close,
 *   9, 3, MAType.RMA, 3, MAType.RMA,
 *   kOut, d, j,
 * );
 * // kOut[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`stoch`](https://ta-lib.org/functions/stoch)
 * @see [`stochf`](https://ta-lib.org/functions/stochf)
 * @see [`rma`](https://ta-lib.org/functions/rma)
 * @see [`ma`](https://ta-lib.org/functions/ma)
 */
export function kdj(
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
  optInFastK_Period = requireInt32('KDJ', 'optInFastK_Period', optInFastK_Period);
  optInSlowK_Period = requireInt32('KDJ', 'optInSlowK_Period', optInSlowK_Period);
  optInSlowK_MAType = requireInt32('KDJ', 'optInSlowK_MAType', optInSlowK_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  optInSlowD_Period = requireInt32('KDJ', 'optInSlowD_Period', optInSlowD_Period);
  optInSlowD_MAType = requireInt32('KDJ', 'optInSlowD_MAType', optInSlowD_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireIndexRange('KDJ', startIdx, endIdx);
  const guardStart = clampedStart('KDJ', startIdx, kdjLookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('KDJ', 'inHigh', inHigh, guardInLen);
  requireLength('KDJ', 'inLow', inLow, guardInLen);
  requireLength('KDJ', 'inClose', inClose, guardInLen);
  requireLength('KDJ', 'outK', outK, guardOutLen);
  requireLength('KDJ', 'outD', outD, guardOutLen);
  requireLength('KDJ', 'outJ', outJ, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outK, inHighF64) || overlapsPartly(outK, inLowF64) || overlapsPartly(outK, inCloseF64) || overlapsPartly(outD, inHighF64) || overlapsPartly(outD, inLowF64) || overlapsPartly(outD, inCloseF64) || overlapsPartly(outJ, inHighF64) || overlapsPartly(outJ, inLowF64) || overlapsPartly(outJ, inCloseF64) || overlaps(outK, outD) || overlaps(outK, outJ) || overlaps(outD, outJ)) {
    throw failure('KDJ', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = kdjImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, outK, outD, outJ);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('KDJ', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
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
      throw failure('KDJ advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar, writing the new values into `out`.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number, inClose: number, out: KdjOut): void {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('KDJ update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (typeof out !== 'object' || out === null) {
      throw badSink('KDJ update', 'KdjOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('KDJ update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    kdjStepImpl(this, inHigh, inLow, inClose);
    this.outRangeCount++;
    out.k = this.cur_outK;
    out.d = this.cur_outD;
    out.j = this.cur_outJ;
  }

  /**
   * Writes into `out` what the next `update` with the same bar would write,
   * committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number, inClose: number, out: KdjOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('KDJ peek', 'KdjOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('KDJ peek', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let cur_outK = 0;
    let cur_outD = 0;
    let cur_outJ = 0;
    {
      const subOut0 = (scratchStochOut ??= new StochOut());
      sp.sub0.peek(inHigh, inLow, inClose, subOut0);
      cur_outK = subOut0.slowK;
      cur_outD = subOut0.slowD;
    }
    cur_outJ = 3.0 * cur_outK - 2.0 * cur_outD;
    out.k = cur_outK;
    out.d = cur_outD;
    out.j = cur_outJ;
  }

  /**
   * The value at the last bar this stream counted; `peek` does not change it.
   *
   * @param out - Receives the outputs.
   */
  value(out: KdjOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('KDJ value', 'KdjOut');
    }
    out.k = this.cur_outK;
    out.d = this.cur_outD;
    out.j = this.cur_outJ;
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
 * The outputs of one KDJ bar, written into an object the caller owns and reuses:
 * valid until the next call that writes it.
 */
export class KdjOut {
  /** Raw stochastic smoothed by SlowK_Period MA. */
  k: number = 0;
  /** Signal line: K smoothed by SlowD_Period MA. */
  d: number = 0;
  /** Divergence line, three parts K less two parts D. */
  j: number = 0;
}

function kdjStepImpl(sp: KdjStream, inHigh: number, inLow: number, inClose: number): void {
  let cur_outK = 0;
  let cur_outD = 0;
  let cur_outJ = 0;
  {
    const subOut0 = (scratchStochOut ??= new StochOut());
    sp.sub0.update(inHigh, inLow, inClose, subOut0);
    cur_outK = subOut0.slowK;
    cur_outD = subOut0.slowD;
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
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
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
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen < kdjLookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType as MAType, optInSlowD_Period, optInSlowD_MAType as MAType, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const sc_outK = outStride === 1 ? outK : new Float64Array(historyLen);
  const sc_outD = outStride === 1 ? outD : new Float64Array(historyLen);
  const sc_outJ = outStride === 1 ? outJ : new Float64Array(historyLen);
  lookbackTotal = kdjLookback(optInFastK_Period, optInSlowK_Period, (optInSlowK_MAType as MAType), optInSlowD_Period, (optInSlowD_MAType as MAType), core);
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
    throw insufficientHistory('KDJ openAndFill', inHigh.length, startIdx, kdjLookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType as MAType, optInSlowD_Period, optInSlowD_MAType as MAType, core));
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
    throw insufficientHistory('KDJ open', inHigh.length, startIdx, kdjLookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType as MAType, optInSlowD_Period, optInSlowD_MAType as MAType, core));
  }
  throw streamFailure('KDJ open', retCode);
}

/**
 * Opens a live KDJ stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link kdj} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInFastK_Period - Lookback window for the raw stochastic high-low range
 *   (default 9; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowK_Period - Smoothing period turning the raw stochastic into K
 *   (default 3; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowK_MAType - MA type used to smooth into K (default `MAType.RMA`;
 *   values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3,
 *   9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
 *   `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param optInSlowD_Period - Smoothing period for the D signal line (default 3;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowD_MAType - MA type used for the D line (default `MAType.RMA`;
 *   values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3,
 *   9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
 *   `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `kdjLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { KdjOut, MAType, kdjOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = kdjOpen(high, low, close, 9, 3, MAType.RMA, 3, MAType.RMA);
 * const peeked = new KdjOut();
 * const updated = new KdjOut();
 * stream.peek(101.4, 99.1, 100.9, peeked); // a forming bar: commits nothing
 * stream.update(101.4, 99.1, 100.9, updated); // the bar closes
 * ```
 */
export function kdjOpen(
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
  optInFastK_Period = requireInt32('KDJ open', 'optInFastK_Period', optInFastK_Period);
  optInSlowK_Period = requireInt32('KDJ open', 'optInSlowK_Period', optInSlowK_Period);
  optInSlowK_MAType = requireInt32('KDJ open', 'optInSlowK_MAType', optInSlowK_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  optInSlowD_Period = requireInt32('KDJ open', 'optInSlowD_Period', optInSlowD_Period);
  optInSlowD_MAType = requireInt32('KDJ open', 'optInSlowD_MAType', optInSlowD_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('KDJ open', 'inHigh', inHigh);
  requireHistory('KDJ open', inHigh.length);
  requireArgument('KDJ open', 'inLow', inLow);
  requireArgument('KDJ open', 'inClose', inClose);
  requireHistoryLength('KDJ open', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('KDJ open', 'inClose', inClose.length, inHigh.length);
  return kdjOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType);
}

/**
 * {@link kdjOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link kdj}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInFastK_Period - Lookback window for the raw stochastic high-low range
 *   (default 9; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowK_Period - Smoothing period turning the raw stochastic into K
 *   (default 3; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowK_MAType - MA type used to smooth into K (default `MAType.RMA`;
 *   values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3,
 *   9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
 *   `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param optInSlowD_Period - Smoothing period for the D signal line (default 3;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSlowD_MAType - MA type used for the D line (default `MAType.RMA`;
 *   values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3,
 *   9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
 *   `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param outK - Raw stochastic smoothed by SlowK_Period MA. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param outD - Signal line: K smoothed by SlowD_Period MA. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param outJ - Divergence line, three parts K less two parts D. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `kdjLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range, histories of different lengths or an output too
 *   short or starting where another output or a `Float64Array` input starts, or, on
 *   a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
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
 * const kOut = new Float64Array(high.length);
 * const d = new Float64Array(high.length);
 * const j = new Float64Array(high.length);
 * const stream = kdjOpenAndFill(
 *   high, low, close,
 *   9, 3, MAType.RMA, 3, MAType.RMA,
 *   kOut, d, j,
 * );
 * const { begIdx, count } = stream.outRange();
 * // kOut[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function kdjOpenAndFill(
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
  optInFastK_Period = requireInt32('KDJ openAndFill', 'optInFastK_Period', optInFastK_Period);
  optInSlowK_Period = requireInt32('KDJ openAndFill', 'optInSlowK_Period', optInSlowK_Period);
  optInSlowK_MAType = requireInt32('KDJ openAndFill', 'optInSlowK_MAType', optInSlowK_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  optInSlowD_Period = requireInt32('KDJ openAndFill', 'optInSlowD_Period', optInSlowD_Period);
  optInSlowD_MAType = requireInt32('KDJ openAndFill', 'optInSlowD_MAType', optInSlowD_MAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('KDJ openAndFill', 'inHigh', inHigh);
  requireHistory('KDJ openAndFill', inHigh.length);
  requireArgument('KDJ openAndFill', 'inLow', inLow);
  requireArgument('KDJ openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('KDJ openAndFill', inHigh.length, kdjLookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType as MAType, optInSlowD_Period, optInSlowD_MAType as MAType, core));
  requireHistoryLength('KDJ openAndFill', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('KDJ openAndFill', 'inClose', inClose.length, inHigh.length);
  requireLength('KDJ openAndFill', 'outK', outK, guardOutLen);
  requireLength('KDJ openAndFill', 'outD', outD, guardOutLen);
  requireLength('KDJ openAndFill', 'outJ', outJ, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outK.buffer === inHighF64.buffer && outK.byteOffset === inHighF64.byteOffset) || (outK.buffer === inLowF64.buffer && outK.byteOffset === inLowF64.byteOffset) || (outK.buffer === inCloseF64.buffer && outK.byteOffset === inCloseF64.byteOffset) || (outD.buffer === inHighF64.buffer && outD.byteOffset === inHighF64.byteOffset) || (outD.buffer === inLowF64.buffer && outD.byteOffset === inLowF64.byteOffset) || (outD.buffer === inCloseF64.buffer && outD.byteOffset === inCloseF64.byteOffset) || (outJ.buffer === inHighF64.buffer && outJ.byteOffset === inHighF64.byteOffset) || (outJ.buffer === inLowF64.buffer && outJ.byteOffset === inLowF64.byteOffset) || (outJ.buffer === inCloseF64.buffer && outJ.byteOffset === inCloseF64.byteOffset) || (outK.buffer === outD.buffer && outK.byteOffset === outD.byteOffset) || (outK.buffer === outJ.buffer && outK.byteOffset === outJ.byteOffset) || (outD.buffer === outJ.buffer && outD.byteOffset === outJ.byteOffset)) {
    throw streamFailure('KDJ openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outK, inHighF64) || overlaps(outK, inLowF64) || overlaps(outK, inCloseF64) || overlaps(outD, inHighF64) || overlaps(outD, inLowF64) || overlaps(outD, inCloseF64) || overlaps(outJ, inHighF64) || overlaps(outJ, inLowF64) || overlaps(outJ, inCloseF64) || overlaps(outK, outD) || overlaps(outK, outJ) || overlaps(outD, outJ))) {
    throw streamFailure('KDJ openAndFill', RetCode.BAD_PARAM);
  }
  return kdjOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, new MInteger(), new MInteger(), outK, outD, outJ);
}

let scratchStochOut: StochOut | null = null;
