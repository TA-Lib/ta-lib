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
 *  022203 MF   Add MAMA
 *  040503 MF   Add T3
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  111603 MF   Allow period of 1. Just copy input into output.
 *  060907 MF   Use TA_SMA/TA_EMA instead of internal implementation.
 *  072226 MF,CC Add HMA (issue #139).
 *  072426 MF,CC TA_MAType_DISABLED: period-independent identity copy (issue #93).
 *  090426 MF,CC Add ZLEMA (issue #347).
 *  090426 MF,CC Add RMA (issue #348).
 *  092926 MF,CC Add VIDYA (issue #474).
 *  092926 MF,CC Add ALMA (issue #475).
 *  101926 MF,CC MAMA: FAMA goes to a scratch buffer (issue #552).
 */

import { MAType, MATypes } from '../enums.ts';
import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { AlmaStream, alma, almaLookback, almaOpenAndFill, almaOpenAndFillInternal, almaOpenInternal, almaPeekTape, almaStepTape, almaTapeDetach } from './alma.ts';
import { DemaStream, dema, demaLookback, demaOpenAndFill, demaOpenAndFillInternal, demaOpenInternal, demaPeekTape, demaStepTape, demaTapeDetach } from './dema.ts';
import { EmaStream, ema, emaLookback, emaOpenAndFill, emaOpenAndFillInternal, emaOpenInternal, emaPeekTape, emaStepTape, emaTapeDetach } from './ema.ts';
import { HmaStream, hma, hmaLookback, hmaOpenAndFill, hmaOpenAndFillInternal, hmaOpenInternal, hmaPeekTape, hmaStepTape, hmaTapeDetach } from './hma.ts';
import { KamaStream, kama, kamaLookback, kamaOpenAndFill, kamaOpenAndFillInternal, kamaOpenInternal, kamaPeekTape, kamaStepTape, kamaTapeDetach } from './kama.ts';
import { MamaOut, MamaStream, mama, mamaLookback, mamaOpenAndFill, mamaOpenAndFillInternal, mamaOpenInternal, mamaPeekTape, mamaStepTape, mamaTapeDetach } from './mama.ts';
import { RmaStream, rma, rmaLookback, rmaOpenAndFill, rmaOpenAndFillInternal, rmaOpenInternal, rmaPeekTape, rmaStepTape, rmaTapeDetach } from './rma.ts';
import { SmaStream, sma, smaLookback, smaOpenAndFill, smaOpenAndFillInternal, smaOpenInternal, smaPeekTape, smaStepTape, smaTapeDetach } from './sma.ts';
import { T3Stream, t3, t3Lookback, t3OpenAndFill, t3OpenAndFillInternal, t3OpenInternal, t3PeekTape, t3StepTape, t3TapeDetach } from './t3.ts';
import { TemaStream, tema, temaLookback, temaOpenAndFill, temaOpenAndFillInternal, temaOpenInternal, temaPeekTape, temaStepTape, temaTapeDetach } from './tema.ts';
import { TrimaStream, trima, trimaLookback, trimaOpenAndFill, trimaOpenAndFillInternal, trimaOpenInternal, trimaPeekTape, trimaStepTape, trimaTapeDetach } from './trima.ts';
import { VidyaStream, vidya, vidyaLookback, vidyaOpenAndFill, vidyaOpenAndFillInternal, vidyaOpenInternal, vidyaPeekTape, vidyaStepTape, vidyaTapeDetach } from './vidya.ts';
import { WmaStream, wma, wmaLookback, wmaOpenAndFill, wmaOpenAndFillInternal, wmaOpenInternal, wmaPeekTape, wmaStepTape, wmaTapeDetach } from './wma.ts';
import { ZlemaStream, zlema, zlemaLookback, zlemaOpenAndFill, zlemaOpenAndFillInternal, zlemaOpenInternal, zlemaPeekTape, zlemaStepTape, zlemaTapeDetach } from './zlema.ts';

/**
 * Number of leading input bars {@link ma} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Averaging window length (default 30; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param optInMAType - Which moving-average algorithm to dispatch to (default
 *   `MAType.SMA`; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
 *   7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA,
 *   15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function maLookback(optInTimePeriod: number, optInMAType: MAType | typeof Core.INTEGER_DEFAULT, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if ((optInMAType | 0) !== optInMAType) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  if (optInMAType === Core.INTEGER_DEFAULT || optInMAType === MAType.DEFAULT) {
    optInMAType = MAType.SMA;
  } else if (optInMAType < MATypes.Min || optInMAType > MATypes.Max) {
    return -1;
  }
  let retValue = 0;
  if (optInTimePeriod <= 1 || optInMAType === MAType.DISABLED) {
    return 0;
  }
  switch (optInMAType) {
    case MAType.SMA: {
      retValue = smaLookback(optInTimePeriod, core);
      break;
    }
    case MAType.EMA: {
      retValue = emaLookback(optInTimePeriod, core);
      break;
    }
    case MAType.WMA: {
      retValue = wmaLookback(optInTimePeriod, core);
      break;
    }
    case MAType.DEMA: {
      retValue = demaLookback(optInTimePeriod, core);
      break;
    }
    case MAType.TEMA: {
      retValue = temaLookback(optInTimePeriod, core);
      break;
    }
    case MAType.TRIMA: {
      retValue = trimaLookback(optInTimePeriod, core);
      break;
    }
    case MAType.KAMA: {
      retValue = kamaLookback(optInTimePeriod, core);
      break;
    }
    case MAType.MAMA: {
      retValue = mamaLookback(0.5, 0.05, core);
      break;
    }
    case MAType.T3: {
      retValue = t3Lookback(optInTimePeriod, 0.7, core);
      break;
    }
    case MAType.HMA: {
      retValue = hmaLookback(optInTimePeriod, core);
      break;
    }
    case MAType.ZLEMA: {
      retValue = zlemaLookback(optInTimePeriod, core);
      break;
    }
    case MAType.RMA: {
      retValue = rmaLookback(optInTimePeriod, core);
      break;
    }
    case MAType.VIDYA: {
      retValue = vidyaLookback(optInTimePeriod, (((((3 * optInTimePeriod) | 0) + 2) / 4) | 0), core);
      break;
    }
    case MAType.ALMA: {
      retValue = almaLookback(optInTimePeriod, 6.0, 0.85, core);
      break;
    }
    default: {
      retValue = 0;
      break;
    }
  }
  return retValue;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link ma}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Averaging window length (default 30; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param optInMAType - Which moving-average algorithm to dispatch to (default
 *   `MAType.SMA`; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
 *   7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA,
 *   15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function maDisplayShift(optInTimePeriod: number, optInMAType: MAType | typeof Core.INTEGER_DEFAULT, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (maLookback(optInTimePeriod, optInMAType, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function maImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  optInMAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let famaScratch!: Float64Array;
  let retCode: RetCode = RetCode.SUCCESS;
  let nbElement = 0;
  let outIdx = 0;
  let todayIdx = 0;
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
  if (optInMAType === Core.INTEGER_DEFAULT || optInMAType === MAType.DEFAULT) {
    optInMAType = MAType.SMA;
  } else if (optInMAType < MATypes.Min || optInMAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  /* Nothing to produce: the range ends before the lookback. Answer here
   * rather than forwarding.
   *
   * The VALUE is the same either way: ma_lookback returns exactly the lookback
   * the arm's callee computes for itself, from the same arguments the arm
   * passes it, and every callee clamps startIdx to that lookback and yields
   * 0,0 without reading. What changes is whose frame answers, and that is
   * visible to one caller only - the zero-length no-I/O probe. It hands the
   * body empty arrays on this range to check that nothing is read; forwarding
   * makes the callee's PUBLIC input bound (endIdx + 1 elements, no
   * sub-lookback escape) answer TA_BAD_PARAM before any array is reached, so
   * the probe cannot tell "read nothing" from "never ran". ma was the last
   * core withheld from that sweep for exactly this reason; apo, bbands, ppo,
   * pvo and stddev already carried this guard. No legitimate caller is
   * affected: ma's own public tier already requires endIdx + 1 input elements,
   * and on this range the callee's OUTPUT bound is 0, so a caller sizing by
   * the published formula was never rejected by it.
   *
   * It cannot mask a TA_BAD_PARAM. An optInMAType outside the enum is refused
   * by the generated entry point above this guard. An in-range member with no
   * arm here would reach ma_lookback's own default and get 0, and 0 > endIdx
   * is false for every endIdx the entry point admits, so control still reaches
   * the switch and still answers TA_BAD_PARAM. The identity path below is out
   * of the guard's reach for the same reason - its lookback is 0.
   */
  if (maLookback(optInTimePeriod, (optInMAType as MAType), core) > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  /* No-smoothing identity: period 1 (every MA type) or the explicit
   * TA_MAType_DISABLED (any period, issue #93). One copy path, lookback 0.
   */
  if (optInTimePeriod === 1 || optInMAType === MAType.DISABLED) {
    nbElement = endIdx - startIdx + 1;
    outNBElement.value = nbElement;
    for (todayIdx = startIdx, outIdx = 0; outIdx < nbElement; outIdx += 1, todayIdx += 1) {
      outReal[outIdx] = inReal[todayIdx];
    }
    outBegIdx.value = startIdx;
    return RetCode.SUCCESS;
  }
  /* Simply forward the job to the corresponding TA function. */
  switch (optInMAType) {
    case MAType.SMA: {
      const _xr0 = sma(startIdx, endIdx, inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = _xr0.begIdx;
      outNBElement.value = _xr0.count;
      retCode = RetCode.SUCCESS;
      break;
    }
    case MAType.EMA: {
      const _xr1 = ema(startIdx, endIdx, inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = _xr1.begIdx;
      outNBElement.value = _xr1.count;
      retCode = RetCode.SUCCESS;
      break;
    }
    case MAType.WMA: {
      const _xr2 = wma(startIdx, endIdx, inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = _xr2.begIdx;
      outNBElement.value = _xr2.count;
      retCode = RetCode.SUCCESS;
      break;
    }
    case MAType.DEMA: {
      const _xr3 = dema(startIdx, endIdx, inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = _xr3.begIdx;
      outNBElement.value = _xr3.count;
      retCode = RetCode.SUCCESS;
      break;
    }
    case MAType.TEMA: {
      const _xr4 = tema(startIdx, endIdx, inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = _xr4.begIdx;
      outNBElement.value = _xr4.count;
      retCode = RetCode.SUCCESS;
      break;
    }
    case MAType.TRIMA: {
      const _xr5 = trima(startIdx, endIdx, inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = _xr5.begIdx;
      outNBElement.value = _xr5.count;
      retCode = RetCode.SUCCESS;
      break;
    }
    case MAType.KAMA: {
      const _xr6 = kama(startIdx, endIdx, inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = _xr6.begIdx;
      outNBElement.value = _xr6.count;
      retCode = RetCode.SUCCESS;
      break;
    }
    case MAType.MAMA: {
      /* The optInTimePeriod is ignored. */
      famaScratch = new Float64Array(endIdx - startIdx + 1);
      const _xr7 = mama(startIdx, endIdx, inReal, 0.5, 0.05, outReal, famaScratch, core);
      outBegIdx.value = _xr7.begIdx;
      outNBElement.value = _xr7.count;
      retCode = RetCode.SUCCESS;
      break;
    }
    case MAType.T3: {
      const _xr8 = t3(startIdx, endIdx, inReal, optInTimePeriod, 0.7, outReal, core);
      outBegIdx.value = _xr8.begIdx;
      outNBElement.value = _xr8.count;
      retCode = RetCode.SUCCESS;
      break;
    }
    case MAType.HMA: {
      const _xr9 = hma(startIdx, endIdx, inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = _xr9.begIdx;
      outNBElement.value = _xr9.count;
      retCode = RetCode.SUCCESS;
      break;
    }
    case MAType.ZLEMA: {
      const _xr10 = zlema(startIdx, endIdx, inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = _xr10.begIdx;
      outNBElement.value = _xr10.count;
      retCode = RetCode.SUCCESS;
      break;
    }
    case MAType.RMA: {
      const _xr11 = rma(startIdx, endIdx, inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = _xr11.begIdx;
      outNBElement.value = _xr11.count;
      retCode = RetCode.SUCCESS;
      break;
    }
    case MAType.VIDYA: {
      /* The one period is the EMA length; the CMO period is round(3n/4),
       * Chande's 12:9 ratio.
       */
      const _xr12 = vidya(startIdx, endIdx, inReal, optInTimePeriod, (((((3 * optInTimePeriod) | 0) + 2) / 4) | 0), outReal, core);
      outBegIdx.value = _xr12.begIdx;
      outNBElement.value = _xr12.count;
      retCode = RetCode.SUCCESS;
      break;
    }
    case MAType.ALMA: {
      const _xr13 = alma(startIdx, endIdx, inReal, optInTimePeriod, 6.0, 0.85, outReal, core);
      outBegIdx.value = _xr13.begIdx;
      outNBElement.value = _xr13.count;
      retCode = RetCode.SUCCESS;
      break;
    }
    default: {
      retCode = RetCode.BAD_PARAM;
      break;
    }
  }
  return retCode;
}

/**
 * Generic moving-average dispatcher that forwards the job to the MA implementation
 * selected by optInMAType. Single uniform interface over all TA-Lib moving
 * averages.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ma](https://ta-lib.org/functions/ma).
 *
 * **Notes**
 * - A period of 1 performs no smoothing for every MAType: the output is a copy of
 *   the input.
 * - `TA_MAType_DISABLED` bypasses smoothing explicitly, for any period: the output
 *   is a copy of the input with a lookback of 0. Every function that takes an
 *   MAType parameter accepts it.
 * - `TA_MAType_DEFAULT` selects the documented default of the parameter it is
 *   passed to — SMA here, EMA for APO, PPO and PVO. Every function that takes an
 *   MAType parameter accepts it.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link maLookback} is a success with `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - Series to average.
 * @param optInTimePeriod - Averaging window length (default 30; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param optInMAType - Which moving-average algorithm to dispatch to (default
 *   `MAType.SMA`; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
 *   7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA,
 *   15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param outReal - Selected moving average of the input. Must hold the values
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
 * import { MAType, ma } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const { begIdx, count } = ma(0, data.length - 1, data, 30, MAType.SMA, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`sma`](https://ta-lib.org/functions/sma)
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`wma`](https://ta-lib.org/functions/wma)
 * @see [`dema`](https://ta-lib.org/functions/dema)
 * @see [`tema`](https://ta-lib.org/functions/tema)
 * @see [`trima`](https://ta-lib.org/functions/trima)
 * @see [`kama`](https://ta-lib.org/functions/kama)
 * @see [`mama`](https://ta-lib.org/functions/mama)
 * @see [`t3`](https://ta-lib.org/functions/t3)
 * @see [`hma`](https://ta-lib.org/functions/hma)
 * @see [`zlema`](https://ta-lib.org/functions/zlema)
 * @see [`rma`](https://ta-lib.org/functions/rma)
 * @see [`vidya`](https://ta-lib.org/functions/vidya)
 * @see [`alma`](https://ta-lib.org/functions/alma)
 */
export function ma(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInMAType: MAType | typeof Core.INTEGER_DEFAULT,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('MA', 'startIdx', startIdx);
  endIdx = requireInt32('MA', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('MA', 'optInTimePeriod', optInTimePeriod);
  optInMAType = requireInt32('MA', 'optInMAType', optInMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireIndexRange('MA', startIdx, endIdx);
  const guardStart = clampedStart('MA', startIdx, maLookback(optInTimePeriod, optInMAType, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('MA', 'inReal', inReal, guardInLen);
  requireLength('MA', 'outReal', outReal, guardOutLen);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw failure('MA', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = maImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, optInMAType, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('MA', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live MA stream: one value per closed bar, bit-identical to {@link ma} over the
 * same series. Open it with {@link maOpen} or {@link maOpenAndFill}; there is
 * nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ma](https://ta-lib.org/functions/ma).
 */
export class MaStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  optInMAType: number = 0;
  /** @internal */
  cur_outReal: number = 0;
  /** @internal */
  sub: SmaStream | EmaStream | WmaStream | DemaStream | TemaStream | TrimaStream | KamaStream | MamaStream | T3Stream | HmaStream | ZlemaStream | RmaStream | VidyaStream | AlmaStream | null = null;
  /** @internal */
  arm: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): MaStream {
    return new MaStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link ma} reports them over the same bars. Each accepted `update` and each
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
      throw failure('MA advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: Series to average.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('MA update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('MA update', 'inReal');
    }
    maStepImpl(this, inReal);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Series to average.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number): number {
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('MA peek', 'inReal');
    }
    const sp = this;
    const sub = sp.sub;
    switch (sp.arm) {
      case -1:
        return inReal;
      case 0: {
        return (sub as SmaStream).peek(inReal);
      }
      case 1: {
        return (sub as EmaStream).peek(inReal);
      }
      case 2: {
        return (sub as WmaStream).peek(inReal);
      }
      case 3: {
        return (sub as DemaStream).peek(inReal);
      }
      case 4: {
        return (sub as TemaStream).peek(inReal);
      }
      case 5: {
        return (sub as TrimaStream).peek(inReal);
      }
      case 6: {
        return (sub as KamaStream).peek(inReal);
      }
      case 7: {
        const subValue = (scratchMamaOut ??= new MamaOut());
        (sub as MamaStream).peek(inReal, subValue);
        return subValue.mama;
      }
      case 8: {
        return (sub as T3Stream).peek(inReal);
      }
      case 9: {
        return (sub as HmaStream).peek(inReal);
      }
      case 10: {
        return (sub as ZlemaStream).peek(inReal);
      }
      case 11: {
        return (sub as RmaStream).peek(inReal);
      }
      case 12: {
        return (sub as VidyaStream).peek(inReal);
      }
      case 13: {
        return (sub as AlmaStream).peek(inReal);
      }
      default:
        throw new Error('unreachable: open rejects arms without a sub-stream');
    }
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
  clone(): MaStream {
    const c = new MaStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.optInMAType = this.optInMAType;
    c.cur_outReal = this.cur_outReal;
    c.sub = this.sub === null ? null : this.sub.clone();
    c.arm = this.arm;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function maStepImpl(sp: MaStream, inReal: number): void {
  const sub = sp.sub;
  switch (sp.arm) {
    case -1:
      sp.cur_outReal = inReal;
      return;
    case 0: {
      sp.cur_outReal = (sub as SmaStream).update(inReal);
      return;
    }
    case 1: {
      sp.cur_outReal = (sub as EmaStream).update(inReal);
      return;
    }
    case 2: {
      sp.cur_outReal = (sub as WmaStream).update(inReal);
      return;
    }
    case 3: {
      sp.cur_outReal = (sub as DemaStream).update(inReal);
      return;
    }
    case 4: {
      sp.cur_outReal = (sub as TemaStream).update(inReal);
      return;
    }
    case 5: {
      sp.cur_outReal = (sub as TrimaStream).update(inReal);
      return;
    }
    case 6: {
      sp.cur_outReal = (sub as KamaStream).update(inReal);
      return;
    }
    case 7: {
      const subOut = (scratchMamaOut ??= new MamaOut());
      (sub as MamaStream).update(inReal, subOut);
      sp.cur_outReal = subOut.mama;
      return;
    }
    case 8: {
      sp.cur_outReal = (sub as T3Stream).update(inReal);
      return;
    }
    case 9: {
      sp.cur_outReal = (sub as HmaStream).update(inReal);
      return;
    }
    case 10: {
      sp.cur_outReal = (sub as ZlemaStream).update(inReal);
      return;
    }
    case 11: {
      sp.cur_outReal = (sub as RmaStream).update(inReal);
      return;
    }
    case 12: {
      sp.cur_outReal = (sub as VidyaStream).update(inReal);
      return;
    }
    case 13: {
      sp.cur_outReal = (sub as AlmaStream).update(inReal);
      return;
    }
    default:
      return;
  }
}

function maOpenImpl(
  sp: MaStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInMAType: number,
): RetCode {
  const core = sp.core;
  const historyLen = inReal.length;
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
  if (optInMAType === Core.INTEGER_DEFAULT || optInMAType === MAType.DEFAULT) {
    optInMAType = MAType.SMA;
  } else if (optInMAType < MATypes.Min || optInMAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if (historyLen < maLookback(optInTimePeriod, optInMAType as MAType, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (optInTimePeriod === 1 || optInMAType === MAType.DISABLED) {
    if (historyLen < maLookback(optInTimePeriod, optInMAType as MAType, core) + 1) {
      return RetCode.INSUFFICIENT_HISTORY;
    }
    sp.optInTimePeriod = optInTimePeriod;
    sp.optInMAType = optInMAType;
    sp.sub = null;
    sp.arm = -1;
    sp.cur_outReal = inReal[historyLen - 1];
    let fillLb = maLookback(optInTimePeriod, optInMAType as MAType, core);
    if (startIdx > fillLb) {
      fillLb = startIdx;
    }
    if (historyLen < fillLb + 1) {
      return RetCode.INSUFFICIENT_HISTORY;
    }
    sp.outRangeBegIdx = fillLb;
    sp.outRangeCount = historyLen - fillLb;
    return RetCode.SUCCESS;
  }
  switch (optInMAType) {
    case MAType.SMA: {
      const sub = smaOpenInternal(core, inReal, startIdx, optInTimePeriod);
      sp.outRangeBegIdx = sub.outRangeBegIdx;
      sp.outRangeCount = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 0;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.EMA: {
      const sub = emaOpenInternal(core, inReal, startIdx, optInTimePeriod);
      sp.outRangeBegIdx = sub.outRangeBegIdx;
      sp.outRangeCount = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 1;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.WMA: {
      const sub = wmaOpenInternal(core, inReal, startIdx, optInTimePeriod);
      sp.outRangeBegIdx = sub.outRangeBegIdx;
      sp.outRangeCount = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 2;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.DEMA: {
      const sub = demaOpenInternal(core, inReal, startIdx, optInTimePeriod);
      sp.outRangeBegIdx = sub.outRangeBegIdx;
      sp.outRangeCount = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 3;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.TEMA: {
      const sub = temaOpenInternal(core, inReal, startIdx, optInTimePeriod);
      sp.outRangeBegIdx = sub.outRangeBegIdx;
      sp.outRangeCount = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 4;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.TRIMA: {
      const sub = trimaOpenInternal(core, inReal, startIdx, optInTimePeriod);
      sp.outRangeBegIdx = sub.outRangeBegIdx;
      sp.outRangeCount = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 5;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.KAMA: {
      const sub = kamaOpenInternal(core, inReal, startIdx, optInTimePeriod);
      sp.outRangeBegIdx = sub.outRangeBegIdx;
      sp.outRangeCount = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 6;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.MAMA: {
      const sub = mamaOpenInternal(core, inReal, startIdx, 0.5, 0.05);
      sp.outRangeBegIdx = sub.outRangeBegIdx;
      sp.outRangeCount = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 7;
      sp.cur_outReal = sub.cur_outMAMA;
      break;
    }
    case MAType.T3: {
      const sub = t3OpenInternal(core, inReal, startIdx, optInTimePeriod, 0.7);
      sp.outRangeBegIdx = sub.outRangeBegIdx;
      sp.outRangeCount = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 8;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.HMA: {
      const sub = hmaOpenInternal(core, inReal, startIdx, optInTimePeriod);
      sp.outRangeBegIdx = sub.outRangeBegIdx;
      sp.outRangeCount = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 9;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.ZLEMA: {
      const sub = zlemaOpenInternal(core, inReal, startIdx, optInTimePeriod);
      sp.outRangeBegIdx = sub.outRangeBegIdx;
      sp.outRangeCount = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 10;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.RMA: {
      const sub = rmaOpenInternal(core, inReal, startIdx, optInTimePeriod);
      sp.outRangeBegIdx = sub.outRangeBegIdx;
      sp.outRangeCount = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 11;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.VIDYA: {
      const sub = vidyaOpenInternal(core, inReal, startIdx, optInTimePeriod, (((((3 * optInTimePeriod) | 0) + 2) / 4) | 0));
      sp.outRangeBegIdx = sub.outRangeBegIdx;
      sp.outRangeCount = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 12;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.ALMA: {
      const sub = almaOpenInternal(core, inReal, startIdx, optInTimePeriod, 6.0, 0.85);
      sp.outRangeBegIdx = sub.outRangeBegIdx;
      sp.outRangeCount = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 13;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    default:
      return RetCode.BAD_PARAM;
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.optInMAType = optInMAType;
  return RetCode.SUCCESS;
}

function maOpenAndFillImpl(
  sp: MaStream,
  inReal: Float64Array,
  optInTimePeriod: number,
  optInMAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  const core = sp.core;
  const historyLen = inReal.length;
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
  if (optInMAType === Core.INTEGER_DEFAULT || optInMAType === MAType.DEFAULT) {
    optInMAType = MAType.SMA;
  } else if (optInMAType < MATypes.Min || optInMAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if ((outReal.buffer === inReal.buffer && outReal.byteOffset === inReal.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  if (historyLen > maLookback(optInTimePeriod, optInMAType as MAType, core) && (overlaps(outReal, inReal))) {
    return RetCode.BAD_PARAM;
  }
  if (historyLen < maLookback(optInTimePeriod, optInMAType as MAType, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (optInTimePeriod === 1 || optInMAType === MAType.DISABLED) {
    if (historyLen < maLookback(optInTimePeriod, optInMAType as MAType, core) + 1) {
      return RetCode.INSUFFICIENT_HISTORY;
    }
    sp.optInTimePeriod = optInTimePeriod;
    sp.optInMAType = optInMAType;
    sp.sub = null;
    sp.arm = -1;
    let fillLb = maLookback(optInTimePeriod, optInMAType as MAType, core);
    outBegIdx.value = fillLb;
    outNBElement.value = historyLen - fillLb;
    for (let fillIdx = 0; fillIdx < historyLen - fillLb; fillIdx++) {
      outReal[fillIdx] = inReal[fillLb + fillIdx];
    }
    sp.cur_outReal = outReal[outNBElement.value - 1];
    return RetCode.SUCCESS;
  }
  switch (optInMAType) {
    case MAType.SMA: {
      const sub = smaOpenAndFill(inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = sub.outRangeBegIdx;
      outNBElement.value = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 0;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.EMA: {
      const sub = emaOpenAndFill(inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = sub.outRangeBegIdx;
      outNBElement.value = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 1;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.WMA: {
      const sub = wmaOpenAndFill(inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = sub.outRangeBegIdx;
      outNBElement.value = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 2;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.DEMA: {
      const sub = demaOpenAndFill(inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = sub.outRangeBegIdx;
      outNBElement.value = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 3;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.TEMA: {
      const sub = temaOpenAndFill(inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = sub.outRangeBegIdx;
      outNBElement.value = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 4;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.TRIMA: {
      const sub = trimaOpenAndFill(inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = sub.outRangeBegIdx;
      outNBElement.value = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 5;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.KAMA: {
      const sub = kamaOpenAndFill(inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = sub.outRangeBegIdx;
      outNBElement.value = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 6;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.MAMA: {
      const sub = mamaOpenAndFill(inReal, 0.5, 0.05, outReal, new Float64Array(historyLen), core);
      outBegIdx.value = sub.outRangeBegIdx;
      outNBElement.value = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 7;
      sp.cur_outReal = sub.cur_outMAMA;
      break;
    }
    case MAType.T3: {
      const sub = t3OpenAndFill(inReal, optInTimePeriod, 0.7, outReal, core);
      outBegIdx.value = sub.outRangeBegIdx;
      outNBElement.value = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 8;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.HMA: {
      const sub = hmaOpenAndFill(inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = sub.outRangeBegIdx;
      outNBElement.value = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 9;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.ZLEMA: {
      const sub = zlemaOpenAndFill(inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = sub.outRangeBegIdx;
      outNBElement.value = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 10;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.RMA: {
      const sub = rmaOpenAndFill(inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = sub.outRangeBegIdx;
      outNBElement.value = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 11;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.VIDYA: {
      const sub = vidyaOpenAndFill(inReal, optInTimePeriod, (((((3 * optInTimePeriod) | 0) + 2) / 4) | 0), outReal, core);
      outBegIdx.value = sub.outRangeBegIdx;
      outNBElement.value = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 12;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.ALMA: {
      const sub = almaOpenAndFill(inReal, optInTimePeriod, 6.0, 0.85, outReal, core);
      outBegIdx.value = sub.outRangeBegIdx;
      outNBElement.value = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 13;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    default:
      return RetCode.BAD_PARAM;
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.optInMAType = optInMAType;
  return RetCode.SUCCESS;
}

function maOpenAndFillInternalImpl(
  sp: MaStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInMAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  const core = sp.core;
  const historyLen = inReal.length;
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
  if (optInMAType === Core.INTEGER_DEFAULT || optInMAType === MAType.DEFAULT) {
    optInMAType = MAType.SMA;
  } else if (optInMAType < MATypes.Min || optInMAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if (historyLen < maLookback(optInTimePeriod, optInMAType as MAType, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (optInTimePeriod === 1 || optInMAType === MAType.DISABLED) {
    if (historyLen < maLookback(optInTimePeriod, optInMAType as MAType, core) + 1) {
      return RetCode.INSUFFICIENT_HISTORY;
    }
    sp.optInTimePeriod = optInTimePeriod;
    sp.optInMAType = optInMAType;
    sp.sub = null;
    sp.arm = -1;
    let fillLb = maLookback(optInTimePeriod, optInMAType as MAType, core);
    if (startIdx > fillLb) {
      fillLb = startIdx;
    }
    if (historyLen < fillLb + 1) {
      return RetCode.INSUFFICIENT_HISTORY;
    }
    outBegIdx.value = fillLb;
    outNBElement.value = historyLen - fillLb;
    for (let fillIdx = 0; fillIdx < historyLen - fillLb; fillIdx++) {
      outReal[fillIdx] = inReal[fillLb + fillIdx];
    }
    sp.cur_outReal = outReal[outNBElement.value - 1];
    return RetCode.SUCCESS;
  }
  switch (optInMAType) {
    case MAType.SMA: {
      const sub = smaOpenAndFillInternal(core, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
      sp.sub = sub;
      sp.arm = 0;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.EMA: {
      const sub = emaOpenAndFillInternal(core, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
      sp.sub = sub;
      sp.arm = 1;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.WMA: {
      const sub = wmaOpenAndFillInternal(core, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
      sp.sub = sub;
      sp.arm = 2;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.DEMA: {
      const sub = demaOpenAndFillInternal(core, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
      sp.sub = sub;
      sp.arm = 3;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.TEMA: {
      const sub = temaOpenAndFillInternal(core, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
      sp.sub = sub;
      sp.arm = 4;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.TRIMA: {
      const sub = trimaOpenAndFillInternal(core, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
      sp.sub = sub;
      sp.arm = 5;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.KAMA: {
      const sub = kamaOpenAndFillInternal(core, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
      sp.sub = sub;
      sp.arm = 6;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.MAMA: {
      const sub = mamaOpenAndFillInternal(core, inReal, startIdx, 0.5, 0.05, outBegIdx, outNBElement, outReal, new Float64Array(historyLen));
      sp.sub = sub;
      sp.arm = 7;
      sp.cur_outReal = sub.cur_outMAMA;
      break;
    }
    case MAType.T3: {
      const sub = t3OpenAndFillInternal(core, inReal, startIdx, optInTimePeriod, 0.7, outBegIdx, outNBElement, outReal);
      sp.sub = sub;
      sp.arm = 8;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.HMA: {
      const sub = hmaOpenAndFillInternal(core, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
      sp.sub = sub;
      sp.arm = 9;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.ZLEMA: {
      const sub = zlemaOpenAndFillInternal(core, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
      sp.sub = sub;
      sp.arm = 10;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.RMA: {
      const sub = rmaOpenAndFillInternal(core, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
      sp.sub = sub;
      sp.arm = 11;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.VIDYA: {
      const sub = vidyaOpenAndFillInternal(core, inReal, startIdx, optInTimePeriod, (((((3 * optInTimePeriod) | 0) + 2) / 4) | 0), outBegIdx, outNBElement, outReal);
      sp.sub = sub;
      sp.arm = 12;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.ALMA: {
      const sub = almaOpenAndFillInternal(core, inReal, startIdx, optInTimePeriod, 6.0, 0.85, outBegIdx, outNBElement, outReal);
      sp.sub = sub;
      sp.arm = 13;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    default:
      return RetCode.BAD_PARAM;
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.optInMAType = optInMAType;
  return RetCode.SUCCESS;
}

/** @internal */
export function maOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInMAType: number,
): MaStream {
  const sp = MaStream.of(core);
  const retCode = maOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInMAType);
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MA open', inReal.length, startIdx, maLookback(optInTimePeriod, optInMAType as MAType, core));
  }
  throw streamFailure('MA open', retCode);
}

/**
 * Opens a live MA stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link ma} there.
 *
 * @param inReal - Warm-up history. Series to average.
 * @param optInTimePeriod - Averaging window length (default 30; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param optInMAType - Which moving-average algorithm to dispatch to (default
 *   `MAType.SMA`; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
 *   7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA,
 *   15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `maLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty one,
 *   and `TA_BAD_PARAM` for an integer parameter that is not an int32 or a parameter
 *   outside its range.
 *
 * @example
 * ```ts
 * import { MAType, maOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = maOpen(data, 30, MAType.SMA);
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function maOpen(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInMAType: MAType | typeof Core.INTEGER_DEFAULT,
  core: Core = Core.DEFAULT,
): MaStream {
  optInTimePeriod = requireInt32('MA open', 'optInTimePeriod', optInTimePeriod);
  optInMAType = requireInt32('MA open', 'optInMAType', optInMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('MA open', 'inReal', inReal);
  requireHistory('MA open', inReal.length);
  return maOpenInternal(core, float64(inReal), 0, optInTimePeriod, optInMAType);
}

/**
 * {@link maOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link ma}. The range written is the stream's
 * `outRange()`.
 *
 * @param inReal - Warm-up history. Series to average.
 * @param optInTimePeriod - Averaging window length (default 30; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param optInMAType - Which moving-average algorithm to dispatch to (default
 *   `MAType.SMA`; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
 *   7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA,
 *   15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param outReal - Selected moving average of the input. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `maLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty one,
 *   and `TA_BAD_PARAM` for an integer parameter that is not an int32, a parameter
 *   outside its range or an output too short or starting where another output or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { MAType, maOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const stream = maOpenAndFill(data, 30, MAType.SMA, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function maOpenAndFill(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInMAType: MAType | typeof Core.INTEGER_DEFAULT,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): MaStream {
  optInTimePeriod = requireInt32('MA openAndFill', 'optInTimePeriod', optInTimePeriod);
  optInMAType = requireInt32('MA openAndFill', 'optInMAType', optInMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('MA openAndFill', 'inReal', inReal);
  requireHistory('MA openAndFill', inReal.length);
  const guardOutLen = openFillCount('MA openAndFill', inReal.length, maLookback(optInTimePeriod, optInMAType as MAType, core));
  requireLength('MA openAndFill', 'outReal', outReal, guardOutLen);
  const inRealF64 = float64(inReal);
  const sp = MaStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = maOpenAndFillImpl(sp, inRealF64, optInTimePeriod, optInMAType, outBegIdx, outNBElement, outReal);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MA openAndFill', inReal.length, 0, maLookback(optInTimePeriod, optInMAType as MAType, core));
  }
  throw streamFailure('MA openAndFill', retCode);
}

/** @internal */
export function maOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInMAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): MaStream {
  const sp = MaStream.of(core);
  const retCode = maOpenAndFillInternalImpl(sp, inReal, startIdx, optInTimePeriod, optInMAType, outBegIdx, outNBElement, outReal);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MA openAndFill', inReal.length, startIdx, maLookback(optInTimePeriod, optInMAType as MAType, core));
  }
  throw streamFailure('MA openAndFill', retCode);
}

/** @internal */
export function maStepTape(sp: MaStream, tape: Float64Array, tapeBase: number, tapeMask: number, inReal: number): number {
  switch (sp.arm) {
    case -1:
      sp.cur_outReal = inReal;
      break;
    case 0: {
      sp.cur_outReal = smaStepTape(sp.sub as SmaStream, tape, tapeBase, tapeMask, inReal);
      break;
    }
    case 1: {
      sp.cur_outReal = emaStepTape(sp.sub as EmaStream, tape, tapeBase, tapeMask, inReal);
      break;
    }
    case 2: {
      sp.cur_outReal = wmaStepTape(sp.sub as WmaStream, tape, tapeBase, tapeMask, inReal);
      break;
    }
    case 3: {
      sp.cur_outReal = demaStepTape(sp.sub as DemaStream, tape, tapeBase, tapeMask, inReal);
      break;
    }
    case 4: {
      sp.cur_outReal = temaStepTape(sp.sub as TemaStream, tape, tapeBase, tapeMask, inReal);
      break;
    }
    case 5: {
      sp.cur_outReal = trimaStepTape(sp.sub as TrimaStream, tape, tapeBase, tapeMask, inReal);
      break;
    }
    case 6: {
      sp.cur_outReal = kamaStepTape(sp.sub as KamaStream, tape, tapeBase, tapeMask, inReal);
      break;
    }
    case 7: {
      const sub = sp.sub as MamaStream;
      mamaStepTape(sub, tape, tapeBase, tapeMask, inReal);
      sp.cur_outReal = sub.cur_outMAMA;
      break;
    }
    case 8: {
      sp.cur_outReal = t3StepTape(sp.sub as T3Stream, tape, tapeBase, tapeMask, inReal);
      break;
    }
    case 9: {
      sp.cur_outReal = hmaStepTape(sp.sub as HmaStream, tape, tapeBase, tapeMask, inReal);
      break;
    }
    case 10: {
      sp.cur_outReal = zlemaStepTape(sp.sub as ZlemaStream, tape, tapeBase, tapeMask, inReal);
      break;
    }
    case 11: {
      sp.cur_outReal = rmaStepTape(sp.sub as RmaStream, tape, tapeBase, tapeMask, inReal);
      break;
    }
    case 12: {
      sp.cur_outReal = vidyaStepTape(sp.sub as VidyaStream, tape, tapeBase, tapeMask, inReal);
      break;
    }
    case 13: {
      sp.cur_outReal = almaStepTape(sp.sub as AlmaStream, tape, tapeBase, tapeMask, inReal);
      break;
    }
    default:
      break;
  }
  sp.outRangeCount++;
  return sp.cur_outReal;
}

/** @internal */
export function maPeekTape(sp: MaStream, tape: Float64Array, tapeBase: number, tapeMask: number, inReal: number): number {
  switch (sp.arm) {
    case -1:
      return inReal;
    case 0: {
      return smaPeekTape(sp.sub as SmaStream, tape, tapeBase, tapeMask, inReal);
    }
    case 1: {
      return emaPeekTape(sp.sub as EmaStream, tape, tapeBase, tapeMask, inReal);
    }
    case 2: {
      return wmaPeekTape(sp.sub as WmaStream, tape, tapeBase, tapeMask, inReal);
    }
    case 3: {
      return demaPeekTape(sp.sub as DemaStream, tape, tapeBase, tapeMask, inReal);
    }
    case 4: {
      return temaPeekTape(sp.sub as TemaStream, tape, tapeBase, tapeMask, inReal);
    }
    case 5: {
      return trimaPeekTape(sp.sub as TrimaStream, tape, tapeBase, tapeMask, inReal);
    }
    case 6: {
      return kamaPeekTape(sp.sub as KamaStream, tape, tapeBase, tapeMask, inReal);
    }
    case 7: {
      const subValue = (scratchMamaOut ??= new MamaOut());
      mamaPeekTape(sp.sub as MamaStream, tape, tapeBase, tapeMask, inReal, subValue);
      return subValue.mama;
    }
    case 8: {
      return t3PeekTape(sp.sub as T3Stream, tape, tapeBase, tapeMask, inReal);
    }
    case 9: {
      return hmaPeekTape(sp.sub as HmaStream, tape, tapeBase, tapeMask, inReal);
    }
    case 10: {
      return zlemaPeekTape(sp.sub as ZlemaStream, tape, tapeBase, tapeMask, inReal);
    }
    case 11: {
      return rmaPeekTape(sp.sub as RmaStream, tape, tapeBase, tapeMask, inReal);
    }
    case 12: {
      return vidyaPeekTape(sp.sub as VidyaStream, tape, tapeBase, tapeMask, inReal);
    }
    case 13: {
      return almaPeekTape(sp.sub as AlmaStream, tape, tapeBase, tapeMask, inReal);
    }
    default:
      throw new Error('unreachable: open rejects arms without a sub-stream');
  }
}

/** @internal */
export function maTapeDetach(sp: MaStream): number {
  switch (sp.arm) {
    case 0:
      return smaTapeDetach(sp.sub as SmaStream);
    case 1:
      return emaTapeDetach(sp.sub as EmaStream);
    case 2:
      return wmaTapeDetach(sp.sub as WmaStream);
    case 3:
      return demaTapeDetach(sp.sub as DemaStream);
    case 4:
      return temaTapeDetach(sp.sub as TemaStream);
    case 5:
      return trimaTapeDetach(sp.sub as TrimaStream);
    case 6:
      return kamaTapeDetach(sp.sub as KamaStream);
    case 7:
      return mamaTapeDetach(sp.sub as MamaStream);
    case 8:
      return t3TapeDetach(sp.sub as T3Stream);
    case 9:
      return hmaTapeDetach(sp.sub as HmaStream);
    case 10:
      return zlemaTapeDetach(sp.sub as ZlemaStream);
    case 11:
      return rmaTapeDetach(sp.sub as RmaStream);
    case 12:
      return vidyaTapeDetach(sp.sub as VidyaStream);
    case 13:
      return almaTapeDetach(sp.sub as AlmaStream);
    default:
      return 0;
  }
}

let scratchMamaOut: MamaOut | null = null;
