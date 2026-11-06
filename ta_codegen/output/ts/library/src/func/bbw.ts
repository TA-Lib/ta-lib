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
 *  092526 MF,CC  First version (issue #447).
 */

import { MAType, MATypes } from '../enums.ts';
import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireReal, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { bbandsLookbackPositional } from './bbands.ts';
import { MaStream, maOpenAndFillInternal, maPositional } from './ma.ts';
import { VarStream, varOpenAndFillInternal, varPositional } from './var.ts';

/** @internal */
export function bbwLookbackPositional(optInTimePeriod: number, optInNbDevUp: number, optInNbDevDn: number, optInMAType: MAType | typeof Core.INTEGER_DEFAULT, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (typeof optInNbDevUp !== 'number') {
    return -1;
  }
  if (typeof optInNbDevDn !== 'number') {
    return -1;
  }
  if ((optInMAType | 0) !== optInMAType) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  if (optInNbDevUp === Core.REAL_DEFAULT) {
    optInNbDevUp = 2.0;
  } else if (!(optInNbDevUp >= Core.REAL_MIN && optInNbDevUp <= Core.REAL_MAX)) {
    return -1;
  }
  if (optInNbDevDn === Core.REAL_DEFAULT) {
    optInNbDevDn = 2.0;
  } else if (!(optInNbDevDn >= Core.REAL_MIN && optInNbDevDn <= Core.REAL_MAX)) {
    return -1;
  }
  if (optInMAType === Core.INTEGER_DEFAULT || optInMAType === MAType.DEFAULT) {
    optInMAType = MAType.SMA;
  } else if (optInMAType < MATypes.Min || optInMAType > MATypes.Max) {
    return -1;
  }
  return bbandsLookbackPositional(optInTimePeriod, optInNbDevUp, optInNbDevDn, (optInMAType as MAType), core);
}

/** @internal */
export function bbwDisplayShiftPositional(optInTimePeriod: number, optInNbDevUp: number, optInNbDevDn: number, optInMAType: MAType | typeof Core.INTEGER_DEFAULT, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (bbwLookbackPositional(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function bbwImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  optInNbDevUp: number,
  optInNbDevDn: number,
  optInMAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let i = 0;
  const maBegIdx = new MInteger();
  const maNbElement = new MInteger();
  let offset = 0;
  let middle = 0;
  let deviation = 0;
  let tempReal = 0;
  let upper = 0;
  let lower = 0;
  let tempBuffer!: Float64Array;
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
  if (optInNbDevUp === Core.REAL_DEFAULT) {
    optInNbDevUp = 2.0;
  } else if (!(optInNbDevUp >= Core.REAL_MIN && optInNbDevUp <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInNbDevDn === Core.REAL_DEFAULT) {
    optInNbDevDn = 2.0;
  } else if (!(optInNbDevDn >= Core.REAL_MIN && optInNbDevDn <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInMAType === Core.INTEGER_DEFAULT || optInMAType === MAType.DEFAULT) {
    optInMAType = MAType.SMA;
  } else if (optInMAType < MATypes.Min || optInMAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if (optInMAType === MAType.SMA) {
    /* Keep the middle band's divide in the recurrence, where the running
     * sums' dependency chain hides it; everything downstream of the
     * variance, the square root included, belongs in the tile pass, where
     * it vectorizes.
     */
    const _mid = new Float64Array(256);
    let maTotal = 0;
    let shift = 0;
    let varTotal1 = 0;
    let varTotal2 = 0;
    let meanValue1 = 0;
    let variance = 0;
    let _invPeriod = 0;
    let _tempReal = 0;
    let _peakTotal2 = 0;
    let _i = 0;
    let _j = 0;
    let _k = 0;
    let _outIdx = 0;
    let _tileBase = 0;
    let _tileEnd = 0;
    let _trailingIdx = 0;
    let _windowStart = 0;
    let _lookbackTotal = 0;
    let _barsSinceReseed = 0;
    _lookbackTotal = optInTimePeriod - 1;
    if (startIdx < _lookbackTotal) {
      startIdx = _lookbackTotal;
    }
    if (startIdx > endIdx) {
      outBegIdx.value = 0;
      outNBElement.value = 0;
      return RetCode.SUCCESS;
    }
    _invPeriod = 1.0 / optInTimePeriod;
    _trailingIdx = startIdx - _lookbackTotal;
    shift = inReal[_trailingIdx];
    maTotal = 0.0;
    varTotal1 = 0.0;
    varTotal2 = 0.0;
    for (_j = _trailingIdx; _j < startIdx; _j += 1) {
      maTotal += inReal[_j];
      _tempReal = inReal[_j] - shift;
      varTotal1 += _tempReal;
      _tempReal *= _tempReal;
      varTotal2 += _tempReal;
    }
    _i = startIdx;
    _outIdx = 0;
    _barsSinceReseed = ((32 * optInTimePeriod) | 0);
    _peakTotal2 = varTotal2;
    do {
      if (endIdx - _i > 255) {
        _tileEnd = _i + 255;
      } else {
        _tileEnd = endIdx;
      }
      _tileBase = _outIdx;
      do {
        maTotal += inReal[_i];
        _tempReal = inReal[_i] - shift;
        varTotal1 += _tempReal;
        _tempReal *= _tempReal;
        varTotal2 += _tempReal;
        _peakTotal2 = (varTotal2 > _peakTotal2) ? varTotal2 : _peakTotal2;
        meanValue1 = varTotal1 * _invPeriod;
        variance = varTotal2 * _invPeriod - meanValue1 * meanValue1;
        _mid[_outIdx - _tileBase] = maTotal / optInTimePeriod;
        maTotal -= inReal[_trailingIdx];
        _tempReal = inReal[_trailingIdx] - shift;
        varTotal1 -= _tempReal;
        _tempReal *= _tempReal;
        varTotal2 -= _tempReal;
        _trailingIdx += 1;
        _barsSinceReseed -= 1;
        if (variance < 1e-6 * (_peakTotal2 * _invPeriod) || _barsSinceReseed <= 0) {
          _barsSinceReseed = ((32 * optInTimePeriod) | 0);
          _windowStart = _i - _lookbackTotal;
          _tempReal = 0.0;
          for (_j = _windowStart; _j <= _i; _j += 1) {
            _tempReal += inReal[_j];
          }
          shift = _tempReal * _invPeriod;
          varTotal1 = 0.0;
          varTotal2 = 0.0;
          for (_j = _windowStart; _j <= _i; _j += 1) {
            _tempReal = inReal[_j] - shift;
            varTotal1 += _tempReal;
            _tempReal *= _tempReal;
            varTotal2 += _tempReal;
          }
          meanValue1 = varTotal1 * _invPeriod;
          variance = varTotal2 * _invPeriod - meanValue1 * meanValue1;
          if (variance < 1e-6 * (varTotal2 * _invPeriod)) {
            shift = inReal[_i];
            varTotal1 = 0.0;
            varTotal2 = 0.0;
            for (_j = _windowStart; _j <= _i; _j += 1) {
              _tempReal = inReal[_j] - shift;
              varTotal1 += _tempReal;
              _tempReal *= _tempReal;
              varTotal2 += _tempReal;
            }
            meanValue1 = varTotal1 * _invPeriod;
            variance = varTotal2 * _invPeriod - meanValue1 * meanValue1;
          }
          _peakTotal2 = varTotal2;
          if (variance < 1e-12 * (varTotal2 * _invPeriod)) {
            variance = 0.0;
          }
          _tempReal = inReal[_windowStart] - shift;
          varTotal1 -= _tempReal;
          _tempReal *= _tempReal;
          varTotal2 -= _tempReal;
        }
        /* outReal may be inReal: at startIdx == optInTimePeriod-1 this
         * slot is the first one the rebuild above reads.
         */
        outReal[_outIdx] = variance;
        _outIdx += 1;
        _i += 1;
      } while (_i <= _tileEnd);
      /* Each band is rounded as TA_BBANDS rounds it and the width is taken
       * from the two rounded bands, then scaled after the divide, which
       * keeps BBW bit-identical to ((upper - lower) / middle) * 100 over
       * TA_BBANDS' outputs.
       *
       * Store, then overwrite: gcc keeps a guarded or selected quotient
       * scalar under -ftrapping-math, and says nothing.
       */
      if (optInNbDevUp === optInNbDevDn) {
        for (_k = 0; _k < _outIdx - _tileBase; _k += 1) {
          middle = _mid[_k];
          tempReal = Math.sqrt(outReal[_tileBase + _k]) * optInNbDevUp;
          upper = middle + tempReal;
          lower = middle - tempReal;
          outReal[_tileBase + _k] = (upper - lower) / middle * 100.0;
          if (middle === 0.0) {
            outReal[_tileBase + _k] = 0.0;
          }
        }
      } else {
        for (_k = 0; _k < _outIdx - _tileBase; _k += 1) {
          middle = _mid[_k];
          deviation = Math.sqrt(outReal[_tileBase + _k]);
          upper = fma(deviation, optInNbDevUp, middle);
          lower = middle - deviation * optInNbDevDn;
          outReal[_tileBase + _k] = (upper - lower) / middle * 100.0;
          if (middle === 0.0) {
            outReal[_tileBase + _k] = 0.0;
          }
        }
      }
    } while (_i <= endIdx);
    outNBElement.value = _outIdx;
    outBegIdx.value = startIdx;
    return RetCode.SUCCESS;
  }
  if (bbwLookbackPositional(optInTimePeriod, optInNbDevUp, optInNbDevDn, (optInMAType as MAType), core) > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  tempBuffer = new Float64Array(endIdx - startIdx + 1);
  /* Before the variance: it may be written over inReal. */
  const _xr0 = maPositional(startIdx, endIdx, inReal, optInTimePeriod, (optInMAType as MAType), tempBuffer, core);
  maBegIdx.value = _xr0.begIdx;
  maNbElement.value = _xr0.count;
  /* From the moving average's begIdx, as TA_BBANDS enters its deviation:
   * the variance's shift and reseed schedule are anchored on its start.
   */
  const _xr1 = varPositional(maBegIdx.value, endIdx, inReal, optInTimePeriod, 1.0, outReal, core);
  outBegIdx.value = _xr1.begIdx;
  outNBElement.value = _xr1.count;
  offset = maNbElement.value - outNBElement.value;
  if (optInNbDevUp === optInNbDevDn) {
    for (i = 0; i < outNBElement.value; i += 1) {
      middle = tempBuffer[i + offset];
      tempReal = Math.sqrt(outReal[i]) * optInNbDevUp;
      upper = middle + tempReal;
      lower = middle - tempReal;
      outReal[i] = (upper - lower) / middle * 100.0;
      if (middle === 0.0) {
        outReal[i] = 0.0;
      }
    }
  } else {
    for (i = 0; i < outNBElement.value; i += 1) {
      middle = tempBuffer[i + offset];
      deviation = Math.sqrt(outReal[i]);
      upper = fma(deviation, optInNbDevUp, middle);
      lower = middle - deviation * optInNbDevDn;
      outReal[i] = (upper - lower) / middle * 100.0;
      if (middle === 0.0) {
        outReal[i] = 0.0;
      }
    }
  }
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['nbDevUp', Core.REAL_DEFAULT, Core.REAL_MIN, Core.REAL_MAX, 'in -3e37..3e37'], ['nbDevDn', Core.REAL_DEFAULT, Core.REAL_MIN, Core.REAL_MAX, 'in -3e37..3e37'], ['maType', Core.INTEGER_DEFAULT, MATypes.Min, MATypes.Max, 'a MAType']] as const;

/** @internal */
export function bbwPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInNbDevUp: number,
  optInNbDevDn: number,
  optInMAType: MAType | typeof Core.INTEGER_DEFAULT,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('BBW', 'startIdx', startIdx);
  endIdx = requireInt32('BBW', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('BBW', 'timePeriod', optInTimePeriod);
  optInNbDevUp = requireReal('BBW', 'nbDevUp', optInNbDevUp);
  optInNbDevDn = requireReal('BBW', 'nbDevDn', optInNbDevDn);
  optInMAType = requireInt32('BBW', 'maType', optInMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireIndexRange('BBW', startIdx, endIdx);
  const guardLookback = bbwLookbackPositional(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, core);
  if (guardLookback < 0) {
    throw rejectedParams('BBW', PARAM_RANGES, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType);
  }
  const guardStart = clampedStart('BBW', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('BBW', 'real', inReal, guardInLen);
  requireOutput('BBW', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('BBW', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = bbwImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('BBW', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link bbw}, {@link bbwLookback}, {@link bbwDisplayShift},
 * {@link bbwOpen} and {@link bbwOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface BbwOptions {
  /**
   * Periods for the MA and standard deviation (default 20; range 2 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  timePeriod?: number | undefined;
  /**
   * Standard-deviation multiplier for the upper band (default 2; `Core.REAL_DEFAULT`
   * selects the default).
   */
  nbDevUp?: number | undefined;
  /**
   * Standard-deviation multiplier for the lower band (default 2; `Core.REAL_DEFAULT`
   * selects the default).
   */
  nbDevDn?: number | undefined;
  /**
   * Moving-average type for the middle band (default `MAType.SMA`; values: 0=SMA,
   * 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA, 10=DISABLED,
   * 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA; `Core.INTEGER_DEFAULT` or
   * `MAType.DEFAULT` selects the default).
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

/** What {@link bbw} returns, and {@link bbwOpenAndFill} with its stream. */
export interface BbwResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Width of the bands as a percentage of the middle band. `values[k]` is the value
   * at input bar `begIdx + k`.
   */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link bbw} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link BbwOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function bbwLookback(options?: BbwOptions): number {
  const o = optionsOf('BBW lookback', options);
  return bbwLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.nbDevUp, Core.REAL_DEFAULT), orDefault(o.nbDevDn, Core.REAL_DEFAULT), orDefault(o.maType, Core.INTEGER_DEFAULT), coreOf('BBW lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link bbw}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link BbwOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function bbwDisplayShift(outputIdx: number, options?: BbwOptions): number {
  const o = optionsOf('BBW displayShift', options);
  return bbwDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.nbDevUp, Core.REAL_DEFAULT), orDefault(o.nbDevDn, Core.REAL_DEFAULT), orDefault(o.maType, Core.INTEGER_DEFAULT), outputIdx, coreOf('BBW displayShift', o.core));
}

/**
 * Bollinger BandWidth: the distance between the upper and lower Bollinger Bands as
 * a percentage of the middle band. Low values mark contracting volatility, the
 * setup John Bollinger calls the Squeeze; high values mark expanding volatility.
 *
 * Formula and more info at
 * [ta-lib.org/functions/bbw](https://ta-lib.org/functions/bbw).
 *
 * **Notes**
 * - **The input is assumed positive.** The middle band is the divisor. Where it is
 *   exactly 0 the output is 0. Near zero the divisor is small and the output very
 *   large; no error is returned. A series that reaches or crosses zero, such as an
 *   oscillator or a spread, has no meaningful result here.
 * - With Bollinger's settings (a simple moving average and two deviations on each
 *   side) BBW is 400 times the window's coefficient of variation: its standard
 *   deviation divided by its mean.
 * - The two deviation multipliers enter only through their sum.
 * - The result is in percent: 10 means the bands are 10% of the middle band apart.
 * - Any `optInMAType` other than SMA is a TA-Lib generalisation, as it is for
 *   BBANDS: the deviation stays the population standard deviation about the simple
 *   mean.
 * - Wherever the middle band is not 0, BBW is bit for bit
 *   `((upper - lower) / middle) * 100` computed from BBANDS' own outputs.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link bbwLookback} is a success with empty outputs.
 *
 * @param real - Input data series.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link BbwOptions}.
 * @returns `begIdx` and the values; see {@link BbwResult}. Each output is a view
 *   of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, an integer parameter that is not an int32, a real parameter that is not a
 *   number, a parameter outside its range, an input too short for the range, a
 *   buffer that is empty, too short or of the wrong type or a buffer sharing memory
 *   with a `Float64Array` input it is not. Checked before anything is written.
 *
 * @example
 * ```ts
 * import { MAType, bbw } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = bbw(data, {
 *   timePeriod: 20,
 *   nbDevUp: 2,
 *   nbDevDn: 2,
 *   maType: MAType.SMA,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`bbands`](https://ta-lib.org/functions/bbands)
 * @see [`percentb`](https://ta-lib.org/functions/percentb)
 * @see [`stddev`](https://ta-lib.org/functions/stddev)
 * @see [`natr`](https://ta-lib.org/functions/natr)
 */
export function bbw(real: ArrayLike<number>, options?: BbwOptions): BbwResult {
  const o = optionsOf('BBW', options);
  if (o.endIdx === undefined) {
    requireSeries('BBW', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInNbDevUp = orDefault(o.nbDevUp, Core.REAL_DEFAULT);
  const optInNbDevDn = orDefault(o.nbDevDn, Core.REAL_DEFAULT);
  const optInMAType = orDefault(o.maType, Core.INTEGER_DEFAULT);
  const core = coreOf('BBW', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, bbwLookbackPositional(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = bbwPositional(startIdx, endIdx, real, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live BBW stream: one value per closed bar, bit-identical to {@link bbw} over
 * the same series. Open it with {@link bbwOpen} or {@link bbwOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/bbw](https://ta-lib.org/functions/bbw).
 */
export class BbwStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  optInNbDevUp: number = 0;
  /** @internal */
  optInNbDevDn: number = 0;
  /** @internal */
  optInMAType: number = 0;
  /** @internal */
  cur_outReal: number = 0;
  /** @internal */
  sub0!: MaStream;
  /** @internal */
  sub1!: VarStream;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): BbwStream {
    return new BbwStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link bbw} reports them over the same bars. Each accepted `update` and each
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
      throw failure('BBW advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: Input data series.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('BBW update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('BBW update', 'inReal');
    }
    bbwStepImpl(this, inReal);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Input data series.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number): number {
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('BBW peek', 'inReal');
    }
    const sp = this;
    let deviation = 0;
    let lower = 0;
    let middle = 0;
    let tempReal = 0;
    let upper = 0;
    let cur_tempBuffer = 0;
    let cur_outReal = 0;
    cur_tempBuffer = sp.sub0.peek(inReal);
    cur_outReal = sp.sub1.peek(inReal);
    if (sp.optInNbDevUp === sp.optInNbDevDn) {
      middle = cur_tempBuffer;
      tempReal = Math.sqrt(cur_outReal) * sp.optInNbDevUp;
      upper = middle + tempReal;
      lower = middle - tempReal;
      cur_outReal = (upper - lower) / middle * 100.0;
      if (middle === 0.0) {
        cur_outReal = 0.0;
      }
    } else {
      middle = cur_tempBuffer;
      deviation = Math.sqrt(cur_outReal);
      upper = fma(deviation, sp.optInNbDevUp, middle);
      lower = middle - deviation * sp.optInNbDevDn;
      cur_outReal = (upper - lower) / middle * 100.0;
      if (middle === 0.0) {
        cur_outReal = 0.0;
      }
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
  clone(): BbwStream {
    const c = new BbwStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.optInNbDevUp = this.optInNbDevUp;
    c.optInNbDevDn = this.optInNbDevDn;
    c.optInMAType = this.optInMAType;
    c.cur_outReal = this.cur_outReal;
    c.sub0 = this.sub0.clone();
    c.sub1 = this.sub1.clone();
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function bbwStepImpl(sp: BbwStream, inReal: number): void {
  let deviation = 0;
  let lower = 0;
  let middle = 0;
  let tempReal = 0;
  let upper = 0;
  let cur_tempBuffer = 0;
  let cur_outReal = 0;
  cur_tempBuffer = sp.sub0.update(inReal);
  cur_outReal = sp.sub1.update(inReal);
  if (sp.optInNbDevUp === sp.optInNbDevDn) {
    middle = cur_tempBuffer;
    tempReal = Math.sqrt(cur_outReal) * sp.optInNbDevUp;
    upper = middle + tempReal;
    lower = middle - tempReal;
    cur_outReal = (upper - lower) / middle * 100.0;
    if (middle === 0.0) {
      cur_outReal = 0.0;
    }
  } else {
    middle = cur_tempBuffer;
    deviation = Math.sqrt(cur_outReal);
    upper = fma(deviation, sp.optInNbDevUp, middle);
    lower = middle - deviation * sp.optInNbDevDn;
    cur_outReal = (upper - lower) / middle * 100.0;
    if (middle === 0.0) {
      cur_outReal = 0.0;
    }
  }
  sp.cur_outReal = cur_outReal;
}

function bbwOpenImpl(
  sp: BbwStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInNbDevUp: number,
  optInNbDevDn: number,
  optInMAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let i = 0;
  const maBegIdx = new MInteger();
  const maNbElement = new MInteger();
  let offset = 0;
  let middle = 0;
  let deviation = 0;
  let tempReal = 0;
  let upper = 0;
  let lower = 0;
  let tempBuffer!: Float64Array;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInNbDevUp === Core.REAL_DEFAULT) {
    optInNbDevUp = 2.0;
  } else if (!(optInNbDevUp >= Core.REAL_MIN && optInNbDevUp <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInNbDevDn === Core.REAL_DEFAULT) {
    optInNbDevDn = 2.0;
  } else if (!(optInNbDevDn >= Core.REAL_MIN && optInNbDevDn <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInMAType === Core.INTEGER_DEFAULT || optInMAType === MAType.DEFAULT) {
    optInMAType = MAType.SMA;
  } else if (optInMAType < MATypes.Min || optInMAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen < bbwLookbackPositional(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType as MAType, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const sc_outReal = outStride === 1 ? outReal : new Float64Array(historyLen);
  if (bbwLookbackPositional(optInTimePeriod, optInNbDevUp, optInNbDevDn, (optInMAType as MAType), core) > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  tempBuffer = new Float64Array(endIdx - startIdx + 1);
  /* Before the variance: it may be written over inReal. */
  const sub0 = maOpenAndFillInternal(core, inReal, startIdx, optInTimePeriod, (optInMAType as MAType), maBegIdx, maNbElement, tempBuffer);
  /* From the moving average's begIdx, as TA_BBANDS enters its deviation:
   * the variance's shift and reseed schedule are anchored on its start.
   */
  const sub1 = varOpenAndFillInternal(core, inReal, maBegIdx.value, optInTimePeriod, 1.0, outBegIdx, outNBElement, sc_outReal);
  offset = maNbElement.value - outNBElement.value;
  if (optInNbDevUp === optInNbDevDn) {
    for (i = 0; i < outNBElement.value; i += 1) {
      middle = tempBuffer[i + offset];
      tempReal = Math.sqrt(sc_outReal[i]) * optInNbDevUp;
      upper = middle + tempReal;
      lower = middle - tempReal;
      sc_outReal[i] = (upper - lower) / middle * 100.0;
      if (middle === 0.0) {
        sc_outReal[i] = 0.0;
      }
    }
  } else {
    for (i = 0; i < outNBElement.value; i += 1) {
      middle = tempBuffer[i + offset];
      deviation = Math.sqrt(sc_outReal[i]);
      upper = fma(deviation, optInNbDevUp, middle);
      lower = middle - deviation * optInNbDevDn;
      sc_outReal[i] = (upper - lower) / middle * 100.0;
      if (middle === 0.0) {
        sc_outReal[i] = 0.0;
      }
    }
  }
  if (outNBElement.value < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.optInNbDevUp = optInNbDevUp;
  sp.optInNbDevDn = optInNbDevDn;
  sp.optInMAType = optInMAType;
  sp.sub0 = sub0;
  sp.sub1 = sub1;
  sp.cur_outReal = sc_outReal[outNBElement.value - 1];
  return RetCode.SUCCESS;
}

/** @internal */
export function bbwOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInNbDevUp: number,
  optInNbDevDn: number,
  optInMAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): BbwStream {
  const sp = BbwStream.of(core);
  const retCode = bbwOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('BBW openAndFill', inReal.length, startIdx, bbwLookbackPositional(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType as MAType, core));
  }
  if (retCode === RetCode.BAD_PARAM && bbwLookbackPositional(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType as MAType, core) < 0) {
    throw rejectedParams('BBW openAndFill', PARAM_RANGES, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType);
  }
  throw streamFailure('BBW openAndFill', retCode);
}

/** @internal */
export function bbwOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInNbDevUp: number,
  optInNbDevDn: number,
  optInMAType: number,
): BbwStream {
  const sp = BbwStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = bbwOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('BBW open', inReal.length, startIdx, bbwLookbackPositional(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType as MAType, core));
  }
  if (retCode === RetCode.BAD_PARAM && bbwLookbackPositional(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType as MAType, core) < 0) {
    throw rejectedParams('BBW open', PARAM_RANGES, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType);
  }
  throw streamFailure('BBW open', retCode);
}

/** @internal */
export function bbwOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInNbDevUp: number,
  optInNbDevDn: number,
  optInMAType: MAType | typeof Core.INTEGER_DEFAULT,
  core: Core = Core.DEFAULT,
): BbwStream {
  optInTimePeriod = requireInt32('BBW open', 'timePeriod', optInTimePeriod);
  optInNbDevUp = requireReal('BBW open', 'nbDevUp', optInNbDevUp);
  optInNbDevDn = requireReal('BBW open', 'nbDevDn', optInNbDevDn);
  optInMAType = requireInt32('BBW open', 'maType', optInMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('BBW open', 'real', inReal);
  requireHistory('BBW open', inReal.length);
  return bbwOpenInternal(core, float64(inReal), 0, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType);
}

/** @internal */
export function bbwOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInNbDevUp: number,
  optInNbDevDn: number,
  optInMAType: MAType | typeof Core.INTEGER_DEFAULT,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): BbwStream {
  optInTimePeriod = requireInt32('BBW openAndFill', 'timePeriod', optInTimePeriod);
  optInNbDevUp = requireReal('BBW openAndFill', 'nbDevUp', optInNbDevUp);
  optInNbDevDn = requireReal('BBW openAndFill', 'nbDevDn', optInNbDevDn);
  optInMAType = requireInt32('BBW openAndFill', 'maType', optInMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('BBW openAndFill', 'real', inReal);
  requireHistory('BBW openAndFill', inReal.length);
  const guardLookback = bbwLookbackPositional(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType as MAType, core);
  if (guardLookback < 0) {
    throw rejectedParams('BBW openAndFill', PARAM_RANGES, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType);
  }
  const guardOutLen = openFillCount('BBW openAndFill', inReal.length, guardLookback);
  requireOutput('BBW openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('BBW openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('BBW openAndFill', 'outReal', 'real');
  }
  return bbwOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live BBW stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link bbw} there.
 *
 * @param real - Warm-up history. Input data series.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link BbwOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `bbwLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a real parameter that is not a number
 *   or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { MAType, bbwOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = bbwOpen(data, {
 *   timePeriod: 20,
 *   nbDevUp: 2,
 *   nbDevDn: 2,
 *   maType: MAType.SMA,
 * });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function bbwOpen(real: ArrayLike<number>, options?: BbwOptions): BbwStream {
  const o = optionsOf('BBW open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInNbDevUp = orDefault(o.nbDevUp, Core.REAL_DEFAULT);
  const optInNbDevDn = orDefault(o.nbDevDn, Core.REAL_DEFAULT);
  const optInMAType = orDefault(o.maType, Core.INTEGER_DEFAULT);
  const core = coreOf('BBW open', o.core);
  return bbwOpenPositional(real, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, core);
}

/**
 * {@link bbwOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link bbw}.
 *
 * @param real - Warm-up history. Input data series.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link BbwOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link bbw} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `bbwLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a real parameter that is not a
 *   number, a parameter outside its range or a buffer too short, of the wrong type
 *   or starting where another buffer or a `Float64Array` input starts, or, on a
 *   history long enough to open, one sharing memory with another buffer or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { MAType, bbwOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = bbwOpenAndFill(data, {
 *   timePeriod: 20,
 *   nbDevUp: 2,
 *   nbDevDn: 2,
 *   maType: MAType.SMA,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function bbwOpenAndFill(real: ArrayLike<number>, options?: BbwOptions): BbwResult & { stream: BbwStream } {
  const o = optionsOf('BBW openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInNbDevUp = orDefault(o.nbDevUp, Core.REAL_DEFAULT);
  const optInNbDevDn = orDefault(o.nbDevDn, Core.REAL_DEFAULT);
  const optInMAType = orDefault(o.maType, Core.INTEGER_DEFAULT);
  const core = coreOf('BBW openAndFill', o.core);
  const capacity = fillCapacity(bbwLookbackPositional(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = bbwOpenAndFillPositional(real, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
