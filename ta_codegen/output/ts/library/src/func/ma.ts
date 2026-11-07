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
import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { AlmaStream, almaLookbackPositional, almaOpenAndFillInternal, almaOpenAndFillPositional, almaOpenInternal, almaPeekTape, almaPositional, almaStepTape, almaTapeDetach } from './alma.ts';
import { DemaStream, demaLookbackPositional, demaOpenAndFillInternal, demaOpenAndFillPositional, demaOpenInternal, demaPeekTape, demaPositional, demaStepTape, demaTapeDetach } from './dema.ts';
import { EmaStream, emaLookbackPositional, emaOpenAndFillInternal, emaOpenAndFillPositional, emaOpenInternal, emaPeekTape, emaPositional, emaStepTape, emaTapeDetach } from './ema.ts';
import { HmaStream, hmaLookbackPositional, hmaOpenAndFillInternal, hmaOpenAndFillPositional, hmaOpenInternal, hmaPeekTape, hmaPositional, hmaStepTape, hmaTapeDetach } from './hma.ts';
import { KamaStream, kamaLookbackPositional, kamaOpenAndFillInternal, kamaOpenAndFillPositional, kamaOpenInternal, kamaPeekTape, kamaPositional, kamaStepTape, kamaTapeDetach } from './kama.ts';
import { MamaOut, MamaStream, mamaLookbackPositional, mamaOpenAndFillInternal, mamaOpenAndFillPositional, mamaOpenInternal, mamaPeekTape, mamaPositional, mamaStepTape, mamaTapeDetach } from './mama.ts';
import { RmaStream, rmaLookbackPositional, rmaOpenAndFillInternal, rmaOpenAndFillPositional, rmaOpenInternal, rmaPeekTape, rmaPositional, rmaStepTape, rmaTapeDetach } from './rma.ts';
import { SmaStream, smaLookbackPositional, smaOpenAndFillInternal, smaOpenAndFillPositional, smaOpenInternal, smaPeekTape, smaPositional, smaStepTape, smaTapeDetach } from './sma.ts';
import { T3Stream, t3LookbackPositional, t3OpenAndFillInternal, t3OpenAndFillPositional, t3OpenInternal, t3PeekTape, t3Positional, t3StepTape, t3TapeDetach } from './t3.ts';
import { TemaStream, temaLookbackPositional, temaOpenAndFillInternal, temaOpenAndFillPositional, temaOpenInternal, temaPeekTape, temaPositional, temaStepTape, temaTapeDetach } from './tema.ts';
import { TrimaStream, trimaLookbackPositional, trimaOpenAndFillInternal, trimaOpenAndFillPositional, trimaOpenInternal, trimaPeekTape, trimaPositional, trimaStepTape, trimaTapeDetach } from './trima.ts';
import { VidyaStream, vidyaLookbackPositional, vidyaOpenAndFillInternal, vidyaOpenAndFillPositional, vidyaOpenInternal, vidyaPeekTape, vidyaPositional, vidyaStepTape, vidyaTapeDetach } from './vidya.ts';
import { WmaStream, wmaLookbackPositional, wmaOpenAndFillInternal, wmaOpenAndFillPositional, wmaOpenInternal, wmaPeekTape, wmaPositional, wmaStepTape, wmaTapeDetach } from './wma.ts';
import { ZlemaStream, zlemaLookbackPositional, zlemaOpenAndFillInternal, zlemaOpenAndFillPositional, zlemaOpenInternal, zlemaPeekTape, zlemaPositional, zlemaStepTape, zlemaTapeDetach } from './zlema.ts';

/** @internal */
export function maLookbackPositional(optInTimePeriod: number, optInMAType: MAType | typeof Core.INTEGER_DEFAULT, core: Core = Core.DEFAULT): number {
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
      retValue = smaLookbackPositional(optInTimePeriod, core);
      break;
    }
    case MAType.EMA: {
      retValue = emaLookbackPositional(optInTimePeriod, core);
      break;
    }
    case MAType.WMA: {
      retValue = wmaLookbackPositional(optInTimePeriod, core);
      break;
    }
    case MAType.DEMA: {
      retValue = demaLookbackPositional(optInTimePeriod, core);
      break;
    }
    case MAType.TEMA: {
      retValue = temaLookbackPositional(optInTimePeriod, core);
      break;
    }
    case MAType.TRIMA: {
      retValue = trimaLookbackPositional(optInTimePeriod, core);
      break;
    }
    case MAType.KAMA: {
      retValue = kamaLookbackPositional(optInTimePeriod, core);
      break;
    }
    case MAType.MAMA: {
      retValue = mamaLookbackPositional(0.5, 0.05, core);
      break;
    }
    case MAType.T3: {
      retValue = t3LookbackPositional(optInTimePeriod, 0.7, core);
      break;
    }
    case MAType.HMA: {
      retValue = hmaLookbackPositional(optInTimePeriod, core);
      break;
    }
    case MAType.ZLEMA: {
      retValue = zlemaLookbackPositional(optInTimePeriod, core);
      break;
    }
    case MAType.RMA: {
      retValue = rmaLookbackPositional(optInTimePeriod, core);
      break;
    }
    case MAType.VIDYA: {
      retValue = vidyaLookbackPositional(optInTimePeriod, (((((3 * optInTimePeriod) | 0) + 2) / 4) | 0), core);
      break;
    }
    case MAType.ALMA: {
      retValue = almaLookbackPositional(optInTimePeriod, 6.0, 0.85, core);
      break;
    }
    default: {
      retValue = 0;
      break;
    }
  }
  return retValue;
}

/** @internal */
export function maDisplayShiftPositional(optInTimePeriod: number, optInMAType: MAType | typeof Core.INTEGER_DEFAULT, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (maLookbackPositional(optInTimePeriod, optInMAType, core) < 0) {
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
  if (maLookbackPositional(optInTimePeriod, (optInMAType as MAType), core) > endIdx) {
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
      const _xr0 = smaPositional(startIdx, endIdx, inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = _xr0.begIdx;
      outNBElement.value = _xr0.count;
      retCode = RetCode.SUCCESS;
      break;
    }
    case MAType.EMA: {
      const _xr1 = emaPositional(startIdx, endIdx, inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = _xr1.begIdx;
      outNBElement.value = _xr1.count;
      retCode = RetCode.SUCCESS;
      break;
    }
    case MAType.WMA: {
      const _xr2 = wmaPositional(startIdx, endIdx, inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = _xr2.begIdx;
      outNBElement.value = _xr2.count;
      retCode = RetCode.SUCCESS;
      break;
    }
    case MAType.DEMA: {
      const _xr3 = demaPositional(startIdx, endIdx, inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = _xr3.begIdx;
      outNBElement.value = _xr3.count;
      retCode = RetCode.SUCCESS;
      break;
    }
    case MAType.TEMA: {
      const _xr4 = temaPositional(startIdx, endIdx, inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = _xr4.begIdx;
      outNBElement.value = _xr4.count;
      retCode = RetCode.SUCCESS;
      break;
    }
    case MAType.TRIMA: {
      const _xr5 = trimaPositional(startIdx, endIdx, inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = _xr5.begIdx;
      outNBElement.value = _xr5.count;
      retCode = RetCode.SUCCESS;
      break;
    }
    case MAType.KAMA: {
      const _xr6 = kamaPositional(startIdx, endIdx, inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = _xr6.begIdx;
      outNBElement.value = _xr6.count;
      retCode = RetCode.SUCCESS;
      break;
    }
    case MAType.MAMA: {
      /* The optInTimePeriod is ignored. */
      famaScratch = new Float64Array(endIdx - startIdx + 1);
      const _xr7 = mamaPositional(startIdx, endIdx, inReal, 0.5, 0.05, outReal, famaScratch, core);
      outBegIdx.value = _xr7.begIdx;
      outNBElement.value = _xr7.count;
      retCode = RetCode.SUCCESS;
      break;
    }
    case MAType.T3: {
      const _xr8 = t3Positional(startIdx, endIdx, inReal, optInTimePeriod, 0.7, outReal, core);
      outBegIdx.value = _xr8.begIdx;
      outNBElement.value = _xr8.count;
      retCode = RetCode.SUCCESS;
      break;
    }
    case MAType.HMA: {
      const _xr9 = hmaPositional(startIdx, endIdx, inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = _xr9.begIdx;
      outNBElement.value = _xr9.count;
      retCode = RetCode.SUCCESS;
      break;
    }
    case MAType.ZLEMA: {
      const _xr10 = zlemaPositional(startIdx, endIdx, inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = _xr10.begIdx;
      outNBElement.value = _xr10.count;
      retCode = RetCode.SUCCESS;
      break;
    }
    case MAType.RMA: {
      const _xr11 = rmaPositional(startIdx, endIdx, inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = _xr11.begIdx;
      outNBElement.value = _xr11.count;
      retCode = RetCode.SUCCESS;
      break;
    }
    case MAType.VIDYA: {
      /* The one period is the EMA length; the CMO period is round(3n/4),
       * Chande's 12:9 ratio.
       */
      const _xr12 = vidyaPositional(startIdx, endIdx, inReal, optInTimePeriod, (((((3 * optInTimePeriod) | 0) + 2) / 4) | 0), outReal, core);
      outBegIdx.value = _xr12.begIdx;
      outNBElement.value = _xr12.count;
      retCode = RetCode.SUCCESS;
      break;
    }
    case MAType.ALMA: {
      const _xr13 = almaPositional(startIdx, endIdx, inReal, optInTimePeriod, 6.0, 0.85, outReal, core);
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

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['maType', Core.INTEGER_DEFAULT, MATypes.Min, MATypes.Max, 'a MAType']] as const;

/** @internal */
export function maPositional(
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
  optInTimePeriod = requireInt32('MA', 'timePeriod', optInTimePeriod);
  optInMAType = requireInt32('MA', 'maType', optInMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireIndexRange('MA', startIdx, endIdx);
  const guardLookback = maLookbackPositional(optInTimePeriod, optInMAType, core);
  if (guardLookback < 0) {
    throw rejectedParams('MA', PARAM_RANGES, optInTimePeriod, optInMAType);
  }
  const guardStart = clampedStart('MA', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('MA', 'real', inReal, guardInLen);
  requireOutput('MA', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('MA', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = maImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, optInMAType, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('MA', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link ma}, {@link maLookback}, {@link maDisplayShift},
 * {@link maOpen} and {@link maOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface MaOptions {
  /**
   * Averaging window length (default 30; range 1 to 100000; `Core.INTEGER_DEFAULT`
   * selects the default).
   */
  timePeriod?: number | undefined;
  /**
   * Which moving-average algorithm to dispatch to (default `MAType.SMA`; values:
   * 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA,
   * 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
   * `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
   */
  maType?: MAType | typeof Core.INTEGER_DEFAULT | undefined;
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

/** What {@link ma} returns, and {@link maOpenAndFill} with its stream. */
export interface MaResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Selected moving average of the input. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link ma} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MaOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function maLookback(options?: MaOptions): number {
  const o = optionsOf('MA lookback', options);
  return maLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.maType, Core.INTEGER_DEFAULT), coreOf('MA lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link ma}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MaOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function maDisplayShift(outputIdx: number, options?: MaOptions): number {
  const o = optionsOf('MA displayShift', options);
  return maDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.maType, Core.INTEGER_DEFAULT), outputIdx, coreOf('MA displayShift', o.core));
}

/**
 * Generic moving-average dispatcher that forwards the job to the MA implementation
 * selected by maType. Single uniform interface over all TA-Lib moving averages.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ma](https://ta-lib.org/functions/ma).
 *
 * **Notes**
 * - A period of 1 performs no smoothing for every MAType: the output is a copy of
 *   the input.
 * - `MAType.DISABLED` bypasses smoothing explicitly, for any period: the output is
 *   a copy of the input with a lookback of 0. Every function that takes an MAType
 *   parameter accepts it.
 * - `MAType.DEFAULT` selects the documented default of the parameter it is passed
 *   to — SMA here, EMA for APO, PPO and PVO. Every function that takes an MAType
 *   parameter accepts it.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link maLookback} is a success with empty outputs.
 *
 * @param real - Series to average.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link MaOptions}.
 * @returns `begIdx` and the values; see {@link MaResult}. Each output is a view of
 *   its buffer, the caller's or a new one.
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
 * import { MAType, ma } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = ma(data, { timePeriod: 30, maType: MAType.SMA });
 * // values[k] is the value at bar begIdx + k
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
export function ma(real: ArrayLike<number>, options?: MaOptions): MaResult {
  const o = optionsOf('MA', options);
  if (o.endIdx === undefined) {
    requireSeries('MA', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInMAType = orDefault(o.maType, Core.INTEGER_DEFAULT);
  const core = coreOf('MA', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, maLookbackPositional(optInTimePeriod, optInMAType, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = maPositional(startIdx, endIdx, real, optInTimePeriod, optInMAType, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
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
      throw streamFailure('MA advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: Series to average.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('MA update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('MA update', 'real');
    }
    maStepImpl(this, real);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: Series to average.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number): number {
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('MA peek', 'real');
    }
    const sp = this;
    const inReal = real;
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
        return subValue.outMama;
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
        throw failure('MA peek', RetCode.INTERNAL_ERROR);
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
      sp.cur_outReal = subOut.outMama;
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
  if (historyLen < maLookbackPositional(optInTimePeriod, optInMAType as MAType, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (optInTimePeriod === 1 || optInMAType === MAType.DISABLED) {
    if (historyLen < maLookbackPositional(optInTimePeriod, optInMAType as MAType, core) + 1) {
      return RetCode.INSUFFICIENT_HISTORY;
    }
    sp.optInTimePeriod = optInTimePeriod;
    sp.optInMAType = optInMAType;
    sp.sub = null;
    sp.arm = -1;
    sp.cur_outReal = inReal[historyLen - 1];
    let fillLb = maLookbackPositional(optInTimePeriod, optInMAType as MAType, core);
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
  if (historyLen > maLookbackPositional(optInTimePeriod, optInMAType as MAType, core) && (overlaps(outReal, inReal))) {
    return RetCode.BAD_PARAM;
  }
  if (historyLen < maLookbackPositional(optInTimePeriod, optInMAType as MAType, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (optInTimePeriod === 1 || optInMAType === MAType.DISABLED) {
    if (historyLen < maLookbackPositional(optInTimePeriod, optInMAType as MAType, core) + 1) {
      return RetCode.INSUFFICIENT_HISTORY;
    }
    sp.optInTimePeriod = optInTimePeriod;
    sp.optInMAType = optInMAType;
    sp.sub = null;
    sp.arm = -1;
    let fillLb = maLookbackPositional(optInTimePeriod, optInMAType as MAType, core);
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
      const sub = smaOpenAndFillPositional(inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = sub.outRangeBegIdx;
      outNBElement.value = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 0;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.EMA: {
      const sub = emaOpenAndFillPositional(inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = sub.outRangeBegIdx;
      outNBElement.value = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 1;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.WMA: {
      const sub = wmaOpenAndFillPositional(inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = sub.outRangeBegIdx;
      outNBElement.value = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 2;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.DEMA: {
      const sub = demaOpenAndFillPositional(inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = sub.outRangeBegIdx;
      outNBElement.value = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 3;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.TEMA: {
      const sub = temaOpenAndFillPositional(inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = sub.outRangeBegIdx;
      outNBElement.value = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 4;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.TRIMA: {
      const sub = trimaOpenAndFillPositional(inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = sub.outRangeBegIdx;
      outNBElement.value = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 5;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.KAMA: {
      const sub = kamaOpenAndFillPositional(inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = sub.outRangeBegIdx;
      outNBElement.value = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 6;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.MAMA: {
      const sub = mamaOpenAndFillPositional(inReal, 0.5, 0.05, outReal, new Float64Array(historyLen), core);
      outBegIdx.value = sub.outRangeBegIdx;
      outNBElement.value = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 7;
      sp.cur_outReal = sub.cur_outMAMA;
      break;
    }
    case MAType.T3: {
      const sub = t3OpenAndFillPositional(inReal, optInTimePeriod, 0.7, outReal, core);
      outBegIdx.value = sub.outRangeBegIdx;
      outNBElement.value = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 8;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.HMA: {
      const sub = hmaOpenAndFillPositional(inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = sub.outRangeBegIdx;
      outNBElement.value = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 9;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.ZLEMA: {
      const sub = zlemaOpenAndFillPositional(inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = sub.outRangeBegIdx;
      outNBElement.value = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 10;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.RMA: {
      const sub = rmaOpenAndFillPositional(inReal, optInTimePeriod, outReal, core);
      outBegIdx.value = sub.outRangeBegIdx;
      outNBElement.value = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 11;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.VIDYA: {
      const sub = vidyaOpenAndFillPositional(inReal, optInTimePeriod, (((((3 * optInTimePeriod) | 0) + 2) / 4) | 0), outReal, core);
      outBegIdx.value = sub.outRangeBegIdx;
      outNBElement.value = sub.outRangeCount;
      sp.sub = sub;
      sp.arm = 12;
      sp.cur_outReal = sub.cur_outReal;
      break;
    }
    case MAType.ALMA: {
      const sub = almaOpenAndFillPositional(inReal, optInTimePeriod, 6.0, 0.85, outReal, core);
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
  if (historyLen < maLookbackPositional(optInTimePeriod, optInMAType as MAType, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (optInTimePeriod === 1 || optInMAType === MAType.DISABLED) {
    if (historyLen < maLookbackPositional(optInTimePeriod, optInMAType as MAType, core) + 1) {
      return RetCode.INSUFFICIENT_HISTORY;
    }
    sp.optInTimePeriod = optInTimePeriod;
    sp.optInMAType = optInMAType;
    sp.sub = null;
    sp.arm = -1;
    let fillLb = maLookbackPositional(optInTimePeriod, optInMAType as MAType, core);
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
    throw insufficientHistory('MA open', inReal.length, startIdx, maLookbackPositional(optInTimePeriod, optInMAType as MAType, core));
  }
  if (retCode === RetCode.BAD_PARAM && maLookbackPositional(optInTimePeriod, optInMAType as MAType, core) < 0) {
    throw rejectedParams('MA open', PARAM_RANGES, optInTimePeriod, optInMAType);
  }
  throw streamFailure('MA open', retCode);
}

/** @internal */
export function maOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInMAType: MAType | typeof Core.INTEGER_DEFAULT,
  core: Core = Core.DEFAULT,
): MaStream {
  optInTimePeriod = requireInt32('MA open', 'timePeriod', optInTimePeriod);
  optInMAType = requireInt32('MA open', 'maType', optInMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('MA open', 'real', inReal);
  requireHistory('MA open', 'real', inReal.length);
  return maOpenInternal(core, float64(inReal), 0, optInTimePeriod, optInMAType);
}

/** @internal */
export function maOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInMAType: MAType | typeof Core.INTEGER_DEFAULT,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): MaStream {
  optInTimePeriod = requireInt32('MA openAndFill', 'timePeriod', optInTimePeriod);
  optInMAType = requireInt32('MA openAndFill', 'maType', optInMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('MA openAndFill', 'real', inReal);
  requireHistory('MA openAndFill', 'real', inReal.length);
  const guardLookback = maLookbackPositional(optInTimePeriod, optInMAType as MAType, core);
  if (guardLookback < 0) {
    throw rejectedParams('MA openAndFill', PARAM_RANGES, optInTimePeriod, optInMAType);
  }
  const guardOutLen = openFillCount('MA openAndFill', inReal.length, guardLookback);
  requireOutput('MA openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('MA openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('MA openAndFill', 'outReal', 'real');
  }
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
    throw insufficientHistory('MA openAndFill', inReal.length, 0, maLookbackPositional(optInTimePeriod, optInMAType as MAType, core));
  }
  if (retCode === RetCode.BAD_PARAM && maLookbackPositional(optInTimePeriod, optInMAType as MAType, core) < 0) {
    throw rejectedParams('MA openAndFill', PARAM_RANGES, optInTimePeriod, optInMAType);
  }
  throw streamFailure('MA openAndFill', retCode);
}

/**
 * Opens a live MA stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link ma} there.
 *
 * @param real - Warm-up history. Series to average.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MaOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `maLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { MAType, maOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = maOpen(data, { timePeriod: 30, maType: MAType.SMA });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function maOpen(real: ArrayLike<number>, options?: MaOptions): MaStream {
  const o = optionsOf('MA open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInMAType = orDefault(o.maType, Core.INTEGER_DEFAULT);
  const core = coreOf('MA open', o.core);
  return maOpenPositional(real, optInTimePeriod, optInMAType, core);
}

/**
 * {@link maOpen} that also computes the outputs over the whole history in the same
 * pass, bit-identically to {@link ma}.
 *
 * @param real - Warm-up history. Series to average.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link MaOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link ma} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `maLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or a
 *   buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { MAType, maOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = maOpenAndFill(data, {
 *   timePeriod: 30,
 *   maType: MAType.SMA,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function maOpenAndFill(real: ArrayLike<number>, options?: MaOptions): MaResult & { stream: MaStream } {
  const o = optionsOf('MA openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInMAType = orDefault(o.maType, Core.INTEGER_DEFAULT);
  const core = coreOf('MA openAndFill', o.core);
  const capacity = fillCapacity(maLookbackPositional(optInTimePeriod, optInMAType, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = maOpenAndFillPositional(real, optInTimePeriod, optInMAType, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
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
    throw insufficientHistory('MA openAndFill', inReal.length, startIdx, maLookbackPositional(optInTimePeriod, optInMAType as MAType, core));
  }
  if (retCode === RetCode.BAD_PARAM && maLookbackPositional(optInTimePeriod, optInMAType as MAType, core) < 0) {
    throw rejectedParams('MA openAndFill', PARAM_RANGES, optInTimePeriod, optInMAType);
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
      return subValue.outMama;
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
      throw failure('MA peek', RetCode.INTERNAL_ERROR);
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
