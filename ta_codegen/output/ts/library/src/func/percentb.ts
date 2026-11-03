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
 *  092526 MF,CC  First version (issue #449).
 */

import { MAType, MATypes } from '../enums.ts';
import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireReal, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { bbandsLookback } from './bbands.ts';
import { MaStream, ma, maOpenAndFillInternal } from './ma.ts';
import { VarStream, varOpenAndFillInternal, var_ } from './var.ts';

/**
 * Number of leading input bars {@link percentb} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Periods for the MA and standard deviation (default 20;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInNbDevUp - Standard-deviation multiplier for the upper band (default
 *   2; `Core.REAL_DEFAULT` selects the default).
 * @param optInNbDevDn - Standard-deviation multiplier for the lower band (default
 *   2; `Core.REAL_DEFAULT` selects the default).
 * @param optInMAType - Moving-average type for the middle band (default
 *   `MAType.SMA`; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
 *   7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA,
 *   15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function percentbLookback(optInTimePeriod: number, optInNbDevUp: number, optInNbDevDn: number, optInMAType: MAType | typeof Core.INTEGER_DEFAULT, core: Core = Core.DEFAULT): number {
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
  return bbandsLookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, (optInMAType as MAType), core);
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link percentb}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Periods for the MA and standard deviation (default 20;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInNbDevUp - Standard-deviation multiplier for the upper band (default
 *   2; `Core.REAL_DEFAULT` selects the default).
 * @param optInNbDevDn - Standard-deviation multiplier for the lower band (default
 *   2; `Core.REAL_DEFAULT` selects the default).
 * @param optInMAType - Moving-average type for the middle band (default
 *   `MAType.SMA`; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
 *   7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA,
 *   15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function percentbDisplayShift(optInTimePeriod: number, optInNbDevUp: number, optInNbDevDn: number, optInMAType: MAType | typeof Core.INTEGER_DEFAULT, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (percentbLookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function percentbImpl(
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
  const xBegIdx = new MInteger();
  const xNbElement = new MInteger();
  let offsetMA = 0;
  let offsetX = 0;
  let middle = 0;
  let deviation = 0;
  let tempReal = 0;
  let upper = 0;
  let lower = 0;
  let den = 0;
  let tempMA!: Float64Array;
  let tempX!: Float64Array;
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
     * sums' dependency chain hides it; the square root, the bands and the
     * quotient belong in the tile pass, where they vectorize. The tile pass
     * takes x from _x, never from inReal: when outReal is inReal, the
     * variances stored below already cover the tile's x slots.
     */
    const _mid = new Float64Array(256);
    const _x = new Float64Array(256);
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
        _x[_outIdx - _tileBase] = inReal[_i];
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
       * from the two rounded bands, so a zero width is the one the
       * composition over TA_BBANDS divides by.
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
          den = upper - lower;
          outReal[_tileBase + _k] = (_x[_k] - lower) / den;
          if (den === 0.0) {
            outReal[_tileBase + _k] = 0.5;
          }
        }
      } else {
        for (_k = 0; _k < _outIdx - _tileBase; _k += 1) {
          middle = _mid[_k];
          deviation = Math.sqrt(outReal[_tileBase + _k]);
          upper = fma(deviation, optInNbDevUp, middle);
          lower = middle - deviation * optInNbDevDn;
          den = upper - lower;
          outReal[_tileBase + _k] = (_x[_k] - lower) / den;
          if (den === 0.0) {
            outReal[_tileBase + _k] = 0.5;
          }
        }
      }
    } while (_i <= endIdx);
    outNBElement.value = _outIdx;
    outBegIdx.value = startIdx;
    return RetCode.SUCCESS;
  }
  if (percentbLookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, (optInMAType as MAType), core) > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  tempMA = new Float64Array(endIdx - startIdx + 1);
  tempX = new Float64Array(endIdx - startIdx + 1);
  /* Both before the variance: it may be written over inReal. */
  const _xr0 = ma(startIdx, endIdx, inReal, optInTimePeriod, (optInMAType as MAType), tempMA, core);
  maBegIdx.value = _xr0.begIdx;
  maNbElement.value = _xr0.count;
  const _xr1 = ma(maBegIdx.value, endIdx, inReal, 1, (optInMAType as MAType), tempX, core);
  xBegIdx.value = _xr1.begIdx;
  xNbElement.value = _xr1.count;
  /* From the moving average's begIdx, as TA_BBANDS enters its deviation:
   * the variance's shift and reseed schedule are anchored on its start.
   */
  const _xr2 = var_(maBegIdx.value, endIdx, inReal, optInTimePeriod, 1.0, outReal, core);
  outBegIdx.value = _xr2.begIdx;
  outNBElement.value = _xr2.count;
  offsetMA = maNbElement.value - outNBElement.value;
  offsetX = xNbElement.value - outNBElement.value;
  if (optInNbDevUp === optInNbDevDn) {
    for (i = 0; i < outNBElement.value; i += 1) {
      middle = tempMA[i + offsetMA];
      tempReal = Math.sqrt(outReal[i]) * optInNbDevUp;
      upper = middle + tempReal;
      lower = middle - tempReal;
      den = upper - lower;
      outReal[i] = (tempX[i + offsetX] - lower) / den;
      if (den === 0.0) {
        outReal[i] = 0.5;
      }
    }
  } else {
    for (i = 0; i < outNBElement.value; i += 1) {
      middle = tempMA[i + offsetMA];
      deviation = Math.sqrt(outReal[i]);
      upper = fma(deviation, optInNbDevUp, middle);
      lower = middle - deviation * optInNbDevDn;
      den = upper - lower;
      outReal[i] = (tempX[i + offsetX] - lower) / den;
      if (den === 0.0) {
        outReal[i] = 0.5;
      }
    }
  }
  return RetCode.SUCCESS;
}

/**
 * Bollinger Bands %B: where the input sits relative to its Bollinger Bands, 0 at
 * the lower band and 1 at the upper band. Values below 0 or above 1 mean the input
 * is outside the bands.
 *
 * Formula and more info at
 * [ta-lib.org/functions/percentb](https://ta-lib.org/functions/percentb).
 *
 * **Notes**
 * - An input on the middle band reads
 *   `optInNbDevDn / (optInNbDevUp + optInNbDevDn)`: 0.5 with equal multipliers.
 * - With a simple moving average and both multipliers equal to k, %B = 0.5 + z /
 *   (2k), where z is the z-score of the input in its window.
 * - The result is a ratio; multiply by 100 to read it as a percentage.
 * - Any `optInMAType` other than SMA is a TA-Lib generalisation, as it is for
 *   BBANDS: the deviation stays the population standard deviation about the simple
 *   mean.
 * - PERCENTB is bit for bit `(inReal - lower) / (upper - lower)` computed from
 *   BBANDS' own outputs, and 0.5 wherever those two bands are equal.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link percentbLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - Input data series.
 * @param optInTimePeriod - Periods for the MA and standard deviation (default 20;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInNbDevUp - Standard-deviation multiplier for the upper band (default
 *   2; `Core.REAL_DEFAULT` selects the default).
 * @param optInNbDevDn - Standard-deviation multiplier for the lower band (default
 *   2; `Core.REAL_DEFAULT` selects the default).
 * @param optInMAType - Moving-average type for the middle band (default
 *   `MAType.SMA`; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
 *   7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA,
 *   15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param outReal - Position of the input between the lower band (0) and the upper
 *   band (1) Must hold the values produced and never be empty; the input length
 *   always suffices.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The range written: `begIdx` is the first bar with a value, `count` how
 *   many were written.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for an argument that is not an int32, a
 *   real parameter that is not a number, a parameter outside its range, an array too
 *   short for the range, an empty output or an output sharing memory with a
 *   `Float64Array` input it is not. Checked before anything is written.
 *
 * @example
 * ```ts
 * import { MAType, percentb } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const { begIdx, count } = percentb(
 *   0, data.length - 1,
 *   data,
 *   20, 2, 2, MAType.SMA,
 *   out,
 * );
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`bbands`](https://ta-lib.org/functions/bbands)
 * @see [`bbw`](https://ta-lib.org/functions/bbw)
 * @see [`stochf`](https://ta-lib.org/functions/stochf)
 */
export function percentb(
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
  startIdx = requireInt32('PERCENTB', 'startIdx', startIdx);
  endIdx = requireInt32('PERCENTB', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('PERCENTB', 'optInTimePeriod', optInTimePeriod);
  optInNbDevUp = requireReal('PERCENTB', 'optInNbDevUp', optInNbDevUp);
  optInNbDevDn = requireReal('PERCENTB', 'optInNbDevDn', optInNbDevDn);
  optInMAType = requireInt32('PERCENTB', 'optInMAType', optInMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireIndexRange('PERCENTB', startIdx, endIdx);
  const guardStart = clampedStart('PERCENTB', startIdx, percentbLookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('PERCENTB', 'inReal', inReal, guardInLen);
  requireLength('PERCENTB', 'outReal', outReal, guardOutLen);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw failure('PERCENTB', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = percentbImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('PERCENTB', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/* Using percentb_ALT2 for TA_ALT={STREAM,ALL_LANGUAGES} */

/**
 * A live PERCENTB stream: one value per closed bar, bit-identical to
 * {@link percentb} over the same series. Open it with {@link percentbOpen} or
 * {@link percentbOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/percentb](https://ta-lib.org/functions/percentb).
 */
export class PercentbStream {
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

  /** @internal */
  constructor(core: Core) {
    this.core = core;
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link percentb} reports them over the same bars. Each accepted `update` and
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
      throw failure('PERCENTB advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw failure('PERCENTB update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('PERCENTB update', 'inReal');
    }
    percentbStepImpl(this, inReal);
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
      throw nonFiniteBar('PERCENTB peek', 'inReal');
    }
    const sp = this;
    let den = 0;
    let deviation = 0;
    let lower = 0;
    let middle = 0;
    let tempReal = 0;
    let upper = 0;
    let cur_tempX = 0;
    let cur_tempMA = 0;
    let cur_outReal = 0;
    cur_tempX = inReal;
    cur_tempMA = sp.sub0.peek(inReal);
    cur_outReal = sp.sub1.peek(inReal);
    if (sp.optInNbDevUp === sp.optInNbDevDn) {
      middle = cur_tempMA;
      tempReal = Math.sqrt(cur_outReal) * sp.optInNbDevUp;
      upper = middle + tempReal;
      lower = middle - tempReal;
      den = upper - lower;
      cur_outReal = (cur_tempX - lower) / den;
      if (den === 0.0) {
        cur_outReal = 0.5;
      }
    } else {
      middle = cur_tempMA;
      deviation = Math.sqrt(cur_outReal);
      upper = fma(deviation, sp.optInNbDevUp, middle);
      lower = middle - deviation * sp.optInNbDevDn;
      den = upper - lower;
      cur_outReal = (cur_tempX - lower) / den;
      if (den === 0.0) {
        cur_outReal = 0.5;
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
  clone(): PercentbStream {
    const c = new PercentbStream(this.core);
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

function percentbStepImpl(sp: PercentbStream, inReal: number): void {
  let den = 0;
  let deviation = 0;
  let lower = 0;
  let middle = 0;
  let tempReal = 0;
  let upper = 0;
  let cur_tempX = 0;
  let cur_tempMA = 0;
  let cur_outReal = 0;
  cur_tempX = inReal;
  cur_tempMA = sp.sub0.update(inReal);
  cur_outReal = sp.sub1.update(inReal);
  if (sp.optInNbDevUp === sp.optInNbDevDn) {
    middle = cur_tempMA;
    tempReal = Math.sqrt(cur_outReal) * sp.optInNbDevUp;
    upper = middle + tempReal;
    lower = middle - tempReal;
    den = upper - lower;
    cur_outReal = (cur_tempX - lower) / den;
    if (den === 0.0) {
      cur_outReal = 0.5;
    }
  } else {
    middle = cur_tempMA;
    deviation = Math.sqrt(cur_outReal);
    upper = fma(deviation, sp.optInNbDevUp, middle);
    lower = middle - deviation * sp.optInNbDevDn;
    den = upper - lower;
    cur_outReal = (cur_tempX - lower) / den;
    if (den === 0.0) {
      cur_outReal = 0.5;
    }
  }
  sp.cur_outReal = cur_outReal;
}

function percentbOpenImpl(
  sp: PercentbStream,
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
  let lookbackTotal = 0;
  let firstIdx = 0;
  let today = 0;
  let outIdx = 0;
  const maBegIdx = new MInteger();
  const maNbElement = new MInteger();
  let offset = 0;
  let middle = 0;
  let deviation = 0;
  let tempReal = 0;
  let upper = 0;
  let lower = 0;
  let den = 0;
  let tempMA!: Float64Array;
  let tempX!: Float64Array;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
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
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen < percentbLookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType as MAType, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const sc_outReal = outStride === 1 ? outReal : new Float64Array(historyLen);
  lookbackTotal = percentbLookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, (optInMAType as MAType), core);
  firstIdx = startIdx;
  if (firstIdx < lookbackTotal) {
    firstIdx = lookbackTotal;
  }
  if (firstIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  tempX = new Float64Array(endIdx - firstIdx + 1);
  tempMA = new Float64Array(endIdx - startIdx + 1);
  outIdx = 0;
  today = firstIdx;
  while (today <= endIdx) {
    tempX[outIdx++] = inReal[today];
    today += 1;
  }
  const sub0 = maOpenAndFillInternal(core, inReal, startIdx, optInTimePeriod, (optInMAType as MAType), maBegIdx, maNbElement, tempMA);
  const sub1 = varOpenAndFillInternal(core, inReal, maBegIdx.value, optInTimePeriod, 1.0, outBegIdx, outNBElement, sc_outReal);
  offset = maNbElement.value - outNBElement.value;
  if (optInNbDevUp === optInNbDevDn) {
    for (i = 0; i < outNBElement.value; i += 1) {
      middle = tempMA[i + offset];
      tempReal = Math.sqrt(sc_outReal[i]) * optInNbDevUp;
      upper = middle + tempReal;
      lower = middle - tempReal;
      den = upper - lower;
      sc_outReal[i] = (tempX[i] - lower) / den;
      if (den === 0.0) {
        sc_outReal[i] = 0.5;
      }
    }
  } else {
    for (i = 0; i < outNBElement.value; i += 1) {
      middle = tempMA[i + offset];
      deviation = Math.sqrt(sc_outReal[i]);
      upper = fma(deviation, optInNbDevUp, middle);
      lower = middle - deviation * optInNbDevDn;
      den = upper - lower;
      sc_outReal[i] = (tempX[i] - lower) / den;
      if (den === 0.0) {
        sc_outReal[i] = 0.5;
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
export function percentbOpenAndFillInternal(
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
): PercentbStream {
  const sp = new PercentbStream(core);
  const retCode = percentbOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('PERCENTB openAndFill', inReal.length, startIdx, percentbLookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType as MAType, core));
  }
  throw streamFailure('PERCENTB openAndFill', retCode);
}

/** @internal */
export function percentbOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInNbDevUp: number,
  optInNbDevDn: number,
  optInMAType: number,
): PercentbStream {
  const sp = new PercentbStream(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = percentbOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('PERCENTB open', inReal.length, startIdx, percentbLookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType as MAType, core));
  }
  throw streamFailure('PERCENTB open', retCode);
}

/**
 * Opens a live PERCENTB stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link percentb} there.
 *
 * @param inReal - Warm-up history. Input data series.
 * @param optInTimePeriod - Periods for the MA and standard deviation (default 20;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInNbDevUp - Standard-deviation multiplier for the upper band (default
 *   2; `Core.REAL_DEFAULT` selects the default).
 * @param optInNbDevDn - Standard-deviation multiplier for the lower band (default
 *   2; `Core.REAL_DEFAULT` selects the default).
 * @param optInMAType - Moving-average type for the middle band (default
 *   `MAType.SMA`; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
 *   7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA,
 *   15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `percentbLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an
 *   empty one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   real parameter that is not a number or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { MAType, percentbOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = percentbOpen(data, 20, 2, 2, MAType.SMA);
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function percentbOpen(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInNbDevUp: number,
  optInNbDevDn: number,
  optInMAType: MAType | typeof Core.INTEGER_DEFAULT,
  core: Core = Core.DEFAULT,
): PercentbStream {
  optInTimePeriod = requireInt32('PERCENTB open', 'optInTimePeriod', optInTimePeriod);
  optInNbDevUp = requireReal('PERCENTB open', 'optInNbDevUp', optInNbDevUp);
  optInNbDevDn = requireReal('PERCENTB open', 'optInNbDevDn', optInNbDevDn);
  optInMAType = requireInt32('PERCENTB open', 'optInMAType', optInMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('PERCENTB open', 'inReal', inReal);
  requireHistory('PERCENTB open', inReal.length);
  return percentbOpenInternal(core, float64(inReal), 0, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType);
}

/**
 * {@link percentbOpen} that also fills the outputs over the whole history in the
 * same pass, bit-identically to {@link percentb}. The range written is the
 * stream's `outRange()`.
 *
 * @param inReal - Warm-up history. Input data series.
 * @param optInTimePeriod - Periods for the MA and standard deviation (default 20;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInNbDevUp - Standard-deviation multiplier for the upper band (default
 *   2; `Core.REAL_DEFAULT` selects the default).
 * @param optInNbDevDn - Standard-deviation multiplier for the lower band (default
 *   2; `Core.REAL_DEFAULT` selects the default).
 * @param optInMAType - Moving-average type for the middle band (default
 *   `MAType.SMA`; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
 *   7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA,
 *   15=ALMA; `Core.INTEGER_DEFAULT` or `MAType.DEFAULT` selects the default).
 * @param outReal - Position of the input between the lower band (0) and the upper
 *   band (1) Must hold `historyLen - lookback` values and share no memory with an
 *   input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `percentbLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an
 *   empty one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   real parameter that is not a number, a parameter outside its range or an output
 *   too short or starting where another output or a `Float64Array` input starts, or,
 *   on a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { MAType, percentbOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const stream = percentbOpenAndFill(data, 20, 2, 2, MAType.SMA, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function percentbOpenAndFill(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInNbDevUp: number,
  optInNbDevDn: number,
  optInMAType: MAType | typeof Core.INTEGER_DEFAULT,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): PercentbStream {
  optInTimePeriod = requireInt32('PERCENTB openAndFill', 'optInTimePeriod', optInTimePeriod);
  optInNbDevUp = requireReal('PERCENTB openAndFill', 'optInNbDevUp', optInNbDevUp);
  optInNbDevDn = requireReal('PERCENTB openAndFill', 'optInNbDevDn', optInNbDevDn);
  optInMAType = requireInt32('PERCENTB openAndFill', 'optInMAType', optInMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('PERCENTB openAndFill', 'inReal', inReal);
  requireHistory('PERCENTB openAndFill', inReal.length);
  const guardOutLen = openFillCount('PERCENTB openAndFill', inReal.length, percentbLookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType as MAType, core));
  requireLength('PERCENTB openAndFill', 'outReal', outReal, guardOutLen);
  const inRealF64 = float64(inReal);
  if ((outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset)) {
    throw streamFailure('PERCENTB openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inRealF64))) {
    throw streamFailure('PERCENTB openAndFill', RetCode.BAD_PARAM);
  }
  return percentbOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, new MInteger(), new MInteger(), outReal);
}
