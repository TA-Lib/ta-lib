/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  100626 KL,CC  Initial version (#479).
 *  101026 MF,CC  The EMA betas stay as the divide wrote them above a
 *                period of 2.
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma, fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { emaLookbackPositional } from './ema.ts';
import { rsiLookbackPositional } from './rsi.ts';
import { smaLookbackPositional } from './sma.ts';

/** @internal */
export function doscLookbackPositional(optInTimePeriod: number, optInFirstPeriod: number, optInSecondPeriod: number, optInSignalPeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if ((optInFirstPeriod | 0) !== optInFirstPeriod) {
    return -1;
  }
  if ((optInSecondPeriod | 0) !== optInSecondPeriod) {
    return -1;
  }
  if ((optInSignalPeriod | 0) !== optInSignalPeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  if (optInFirstPeriod === Core.INTEGER_DEFAULT) {
    optInFirstPeriod = 5;
  } else if (optInFirstPeriod < 2 || optInFirstPeriod > 100000) {
    return -1;
  }
  if (optInSecondPeriod === Core.INTEGER_DEFAULT) {
    optInSecondPeriod = 3;
  } else if (optInSecondPeriod < 2 || optInSecondPeriod > 100000) {
    return -1;
  }
  if (optInSignalPeriod === Core.INTEGER_DEFAULT) {
    optInSignalPeriod = 9;
  } else if (optInSignalPeriod < 2 || optInSignalPeriod > 100000) {
    return -1;
  }
  /* Keep every term a callee's lookback call: that is how DOSC inherits
   * TA_FUNC_UNST_RSI and TA_FUNC_UNST_EMA with no id of its own. The EMA
   * term counts twice, once per smoothing stage.
   */
  return rsiLookbackPositional(optInTimePeriod, core) + emaLookbackPositional(optInFirstPeriod, core) + emaLookbackPositional(optInSecondPeriod, core) + smaLookbackPositional(optInSignalPeriod, core);
}

/** @internal */
export function doscDisplayShiftPositional(optInTimePeriod: number, optInFirstPeriod: number, optInSecondPeriod: number, optInSignalPeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (doscLookbackPositional(optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function doscImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  optInFirstPeriod: number,
  optInSecondPeriod: number,
  optInSignalPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let k1 = 0;
  let beta1 = 0;
  let k2 = 0;
  let beta2 = 0;
  let invPeriod = 0;
  let prevGain = 0;
  let prevLoss = 0;
  let prevValue = 0;
  let gainDelta = 0;
  let tempValue1 = 0;
  let tempValue2 = 0;
  let rsiValue = 0;
  let ema1 = 0;
  let ema2 = 0;
  let sum1 = 0;
  let sum2 = 0;
  let sumSignal = 0;
  let lookbackTotal = 0;
  let lookbackRSI = 0;
  let lookbackEMA1 = 0;
  let lookbackEMA2 = 0;
  let skipRSI = 0;
  let today = 0;
  let i = 0;
  let outIdx = 0;
  let rsiBar = 0;
  let nRsi = 0;
  let n1 = 0;
  let n2 = 0;
  let dsBuffer!: Float64Array;
  let dsBuffer_Idx = 0;
  let maxIdx_dsBuffer = (32) - 1;
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
  if (optInFirstPeriod === Core.INTEGER_DEFAULT) {
    optInFirstPeriod = 5;
  } else if (optInFirstPeriod < 2 || optInFirstPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSecondPeriod === Core.INTEGER_DEFAULT) {
    optInSecondPeriod = 3;
  } else if (optInSecondPeriod < 2 || optInSecondPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSignalPeriod === Core.INTEGER_DEFAULT) {
    optInSignalPeriod = 9;
  } else if (optInSignalPeriod < 2 || optInSignalPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  /* Constance Brown's "triple smoothed derivative of RSI plotted as a
   * histogram" (MTA Journal, 1994): MACD's histogram construction applied to
   * a double-smoothed RSI.
   *
   *    S1_t   = EMA(RSI(x, t), f)_t
   *    DS_t   = EMA(S1, s)_t
   *    DOSC_t = DS_t - SMA(DS, g)_t
   *
   * The output must stay bit-identical to
   * TA_RSI -> TA_EMA -> TA_EMA -> TA_SMA -> TA_SUB, so every stage keeps its
   * callee's operation order, and every stage boundary is the callee's
   * LOOKBACK, not (period-1): a warm unstable period then folds in.
   *
   * Compare the counters BEFORE subtracting them: the Rust backend renders
   * them usize, where `n = nRsi - skipRSI; if( n >= 0 )` underflows.
   */
  /* This ptr will point on a circular buffer of at least
   * "optInSignalPeriod" element.
   */
  lookbackTotal = doscLookbackPositional(optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod, core);
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
  if (optInSignalPeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  dsBuffer = new Float64Array(optInSignalPeriod);
  maxIdx_dsBuffer = optInSignalPeriod - 1;
  dsBuffer_Idx = 0;
  lookbackRSI = rsiLookbackPositional(optInTimePeriod, core);
  lookbackEMA1 = emaLookbackPositional(optInFirstPeriod, core);
  lookbackEMA2 = emaLookbackPositional(optInSecondPeriod, core);
  /* The RSI values the composed chain never publishes. Derive the count
   * from the lookback, never from TA_GetUnstablePeriod: under the Auto
   * levels it is a function of the period.
   */
  skipRSI = lookbackRSI - optInTimePeriod;
  /* k and beta must sum to exactly 1.0, or a flat input drifts off its
   * level. Each subtraction is exact only from an operand in [0.5,1): at a
   * period of 2 that is k, above it beta. Above it beta stays as the divide
   * wrote it: a register last written by a subtraction costs each FMA
   * reading it one more cycle on Intel P-cores.
   */
  beta1 = (optInFirstPeriod - 1) / (optInFirstPeriod + 1);
  k1 = 1.0 - beta1;
  if (beta1 < 0.5) {
    beta1 = 1.0 - k1;
  }
  beta2 = (optInSecondPeriod - 1) / (optInSecondPeriod + 1);
  k2 = 1.0 - beta2;
  if (beta2 < 0.5) {
    beta2 = 1.0 - k2;
  }
  ema1 = 0.0;
  ema2 = 0.0;
  sum1 = 0.0;
  sum2 = 0.0;
  sumSignal = 0.0;
  nRsi = 0;
  invPeriod = 1.0 / optInTimePeriod;
  today = startIdx - lookbackTotal;
  rsiBar = today + optInTimePeriod;
  prevValue = inReal[today];
  prevGain = 0.0;
  prevLoss = 0.0;
  today = today + 1;
  for (i = optInTimePeriod; i > 0; i -= 1) {
    tempValue1 = inReal[today];
    today = today + 1;
    tempValue2 = tempValue1 - prevValue;
    prevValue = tempValue1;
    gainDelta = (tempValue2 > 0.0) ? tempValue2 : 0.0;
    prevGain += gainDelta;
    prevLoss += gainDelta - tempValue2;
  }
  prevLoss *= invPeriod;
  prevGain *= invPeriod;
  tempValue1 = prevGain + prevLoss;
  if (tempValue1 > 0.0) {
    rsiValue = 100.0 * (prevGain / tempValue1);
  } else {
    rsiValue = 50.0;
  }
  /* Warm-up. Feeds every RSI value before startIdx through the chain and
   * leaves rsiValue holding startIdx's own. Nothing is emitted here: the
   * first bar whose signal window is full is startIdx, by construction of
   * the lookback.
   */
  while (rsiBar < startIdx) {
    if (nRsi >= skipRSI) {
      n1 = nRsi - skipRSI;
      if (n1 < optInFirstPeriod) {
        sum1 = sum1 + rsiValue;
        if (n1 === optInFirstPeriod - 1) {
          ema1 = sum1 / optInFirstPeriod;
        }
      } else {
        {
          const _a: number = beta1, _b: number = ema1, _c: number = k1 * rsiValue;
          const _p = _a * _b;
          let _t = 134217729 * _a;
          const _ah = _t - (_t - _a), _al = _a - _ah;
          _t = 134217729 * _b;
          const _bh = _t - (_t - _b), _bl = _b - _bh;
          const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
          const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
          const _v = _tl + _pl, _v2 = _v + _v;
          const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
          ema1 = _w && (_pl === 0 || _v === 0) ? _th
            : _w && _v2 !== (_th + _v2) - _th ? _th + _v
            : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
        }
      }
      if (n1 >= lookbackEMA1) {
        n2 = n1 - lookbackEMA1;
        if (n2 < optInSecondPeriod) {
          sum2 = sum2 + ema1;
          if (n2 === optInSecondPeriod - 1) {
            ema2 = sum2 / optInSecondPeriod;
          }
        } else {
          {
            const _a: number = beta2, _b: number = ema2, _c: number = k2 * ema1;
            const _p = _a * _b;
            let _t = 134217729 * _a;
            const _ah = _t - (_t - _a), _al = _a - _ah;
            _t = 134217729 * _b;
            const _bh = _t - (_t - _b), _bl = _b - _bh;
            const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
            const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
            const _v = _tl + _pl, _v2 = _v + _v;
            const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
            ema2 = _w && (_pl === 0 || _v === 0) ? _th
              : _w && _v2 !== (_th + _v2) - _th ? _th + _v
              : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
          }
        }
        if (n2 >= lookbackEMA2) {
          dsBuffer[dsBuffer_Idx] = ema2;
          sumSignal = sumSignal + ema2;
          dsBuffer_Idx++;
          if (dsBuffer_Idx > maxIdx_dsBuffer) {
            dsBuffer_Idx = 0;
          }
        }
      }
    }
    nRsi = nRsi + 1;
    tempValue1 = inReal[today];
    today = today + 1;
    tempValue2 = tempValue1 - prevValue;
    prevValue = tempValue1;
    prevLoss *= (optInTimePeriod - 1);
    prevGain *= (optInTimePeriod - 1);
    gainDelta = (tempValue2 > 0.0) ? tempValue2 : 0.0;
    prevGain += gainDelta;
    prevLoss += gainDelta - tempValue2;
    prevLoss *= invPeriod;
    prevGain *= invPeriod;
    tempValue1 = prevGain + prevLoss;
    if (tempValue1 > 0.0) {
      rsiValue = 100.0 * (prevGain / tempValue1);
    } else {
      rsiValue = 50.0;
    }
    rsiBar = rsiBar + 1;
  }
  /* The first output. Every stage is past its seed here, even at periods
   * 2/2/2/2, so the store below and the one in the stable loop are
   * unconditional. Keep them so: under a guard, the managed peek frames
   * seed a dead output local.
   */
  ema1 = fma(beta1, ema1, k1 * rsiValue);
  ema2 = fma(beta2, ema2, k2 * ema1);
  dsBuffer[dsBuffer_Idx] = ema2;
  sumSignal = sumSignal + ema2;
  outReal[0] = ema2 - sumSignal / optInSignalPeriod;
  outIdx = 1;
  dsBuffer_Idx++;
  if (dsBuffer_Idx > maxIdx_dsBuffer) {
    dsBuffer_Idx = 0;
  }
  sumSignal = sumSignal - dsBuffer[dsBuffer_Idx];
  /* Stable zone. */
  while (today <= endIdx) {
    tempValue1 = inReal[today];
    today = today + 1;
    tempValue2 = tempValue1 - prevValue;
    prevValue = tempValue1;
    prevLoss *= (optInTimePeriod - 1);
    prevGain *= (optInTimePeriod - 1);
    gainDelta = (tempValue2 > 0.0) ? tempValue2 : 0.0;
    prevGain += gainDelta;
    prevLoss += gainDelta - tempValue2;
    prevLoss *= invPeriod;
    prevGain *= invPeriod;
    tempValue1 = prevGain + prevLoss;
    if (tempValue1 > 0.0) {
      rsiValue = 100.0 * (prevGain / tempValue1);
    } else {
      rsiValue = 50.0;
    }
    {
      const _a: number = beta1, _b: number = ema1, _c: number = k1 * rsiValue;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      ema1 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = beta2, _b: number = ema2, _c: number = k2 * ema1;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      ema2 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    dsBuffer[dsBuffer_Idx] = ema2;
    sumSignal = sumSignal + ema2;
    outReal[outIdx] = ema2 - sumSignal / optInSignalPeriod;
    outIdx = outIdx + 1;
    dsBuffer_Idx++;
    if (dsBuffer_Idx > maxIdx_dsBuffer) {
      dsBuffer_Idx = 0;
    }
    sumSignal = sumSignal - dsBuffer[dsBuffer_Idx];
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['firstPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['secondPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['signalPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000']] as const;

/** @internal */
export function doscPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInFirstPeriod: number,
  optInSecondPeriod: number,
  optInSignalPeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('DOSC', 'startIdx', startIdx);
  endIdx = requireInt32('DOSC', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('DOSC', 'timePeriod', optInTimePeriod);
  optInFirstPeriod = requireInt32('DOSC', 'firstPeriod', optInFirstPeriod);
  optInSecondPeriod = requireInt32('DOSC', 'secondPeriod', optInSecondPeriod);
  optInSignalPeriod = requireInt32('DOSC', 'signalPeriod', optInSignalPeriod);
  requireIndexRange('DOSC', startIdx, endIdx);
  const guardLookback = doscLookbackPositional(optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('DOSC', PARAM_RANGES, optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod);
  }
  const guardStart = clampedStart('DOSC', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('DOSC', 'real', inReal, guardInLen);
  requireOutput('DOSC', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('DOSC', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = doscImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('DOSC', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link dosc}, {@link doscLookback}, {@link doscDisplayShift},
 * {@link doscOpen} and {@link doscOpenAndFill}. Every key is optional: an omitted
 * or `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface DoscOptions {
  /**
   * Period of the RSI (default 14; range 2 to 100000; `Core.INTEGER_DEFAULT` selects
   * the default).
   */
  timePeriod?: number | undefined;
  /**
   * Period of the first smoothing, applied to the RSI (default 5; range 2 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  firstPeriod?: number | undefined;
  /**
   * Period of the second smoothing, applied to the first (default 3; range 2 to
   * 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  secondPeriod?: number | undefined;
  /**
   * Period of the simple average subtracted from the smoothed line (default 9; range
   * 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  signalPeriod?: number | undefined;
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

/** What {@link dosc} returns, and {@link doscOpenAndFill} with its stream. */
export interface DoscResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Derivative Oscillator, in RSI points, centred on zero. `values[k]` is the value
   * at input bar `begIdx + k`.
   */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link dosc} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link DoscOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function doscLookback(options?: DoscOptions): number {
  const o = optionsOf('DOSC lookback', options);
  return doscLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.firstPeriod, Core.INTEGER_DEFAULT), orDefault(o.secondPeriod, Core.INTEGER_DEFAULT), orDefault(o.signalPeriod, Core.INTEGER_DEFAULT), coreOf('DOSC lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link dosc}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link DoscOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function doscDisplayShift(outputIdx: number, options?: DoscOptions): number {
  const o = optionsOf('DOSC displayShift', options);
  return doscDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), orDefault(o.firstPeriod, Core.INTEGER_DEFAULT), orDefault(o.secondPeriod, Core.INTEGER_DEFAULT), orDefault(o.signalPeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('DOSC displayShift', o.core));
}

/**
 * Derivative Oscillator: Wilder's RSI, smoothed by two exponential averages in
 * series, with a simple average of that smoothed line subtracted from it.
 * Constance Brown's reading is that the RSI's own swings are too noisy to time
 * with, so she smooths it twice and then plots the distance from its own average
 * as a histogram — MACD's histogram construction, applied to a smoothed RSI rather
 * than to price. The result is in RSI points, centred on zero: crossings of the
 * zero line mark the turn, and the height measures how far the smoothed RSI has
 * run from its mean.
 *
 * Formula and more info at
 * [ta-lib.org/functions/dosc](https://ta-lib.org/functions/dosc).
 *
 * **Notes**
 * - Every stage is a function TA-Lib already ships, and the output is identical,
 *   bit for bit, to `TA_RSI` followed by two `TA_EMA` calls, a `TA_SMA` and a
 *   `TA_SUB`. That chain takes five calls and their intermediate buffers; this
 *   computes it in one pass without materialising them.
 * - Each exponential average is seeded with a simple average of its own first
 *   inputs, the same seeding TA-Lib's EMA uses, and the second seeds on what the
 *   first publishes. `TA_SetUnstablePeriod` on either `TA_FUNC_UNST_RSI` or
 *   `TA_FUNC_UNST_EMA` discards more of that warm-up, and the EMA setting counts
 *   twice because there are two exponential stages. Implementations seeding each
 *   stage from a single first sample differ over the transient and agree once it
 *   decays.
 * - The degenerate reading is whatever `TA_RSI` answers when neither a gain nor a
 *   loss has been seen since the seed. Some other implementations answer 100 there
 *   and will disagree over that stretch.
 * - The periods are independent: no ordering between the two smoothing periods is
 *   required or checked. Swapping them changes the outputs, most at the start:
 *   each average is seeded on its own inputs, and that difference decays to
 *   rounding level. A signal period of 1 would make the output identically zero,
 *   so the minimum is 2.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link doscLookback} is a success with empty outputs.
 *
 * @param real - Input series, usually the close.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link DoscOptions}.
 * @returns `begIdx` and the values; see {@link DoscResult}. Each output is a view
 *   of its buffer, the caller's or a new one.
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
 * import { dosc } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = dosc(data, {
 *   timePeriod: 14,
 *   firstPeriod: 5,
 *   secondPeriod: 3,
 *   signalPeriod: 9,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`rsi`](https://ta-lib.org/functions/rsi)
 * @see [`stochrsi`](https://ta-lib.org/functions/stochrsi)
 * @see [`macd`](https://ta-lib.org/functions/macd)
 * @see [`ac`](https://ta-lib.org/functions/ac)
 */
export function dosc(real: ArrayLike<number>, options?: DoscOptions): DoscResult {
  const o = optionsOf('DOSC', options);
  if (o.endIdx === undefined) {
    requireSeries('DOSC', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInFirstPeriod = orDefault(o.firstPeriod, Core.INTEGER_DEFAULT);
  const optInSecondPeriod = orDefault(o.secondPeriod, Core.INTEGER_DEFAULT);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('DOSC', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, doscLookbackPositional(optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = doscPositional(startIdx, endIdx, real, optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live DOSC stream: one value per closed bar, bit-identical to {@link dosc} over
 * the same series. Open it with {@link doscOpen} or {@link doscOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/dosc](https://ta-lib.org/functions/dosc).
 */
export class DoscStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  optInFirstPeriod: number = 0;
  /** @internal */
  optInSecondPeriod: number = 0;
  /** @internal */
  optInSignalPeriod: number = 0;
  /** @internal */
  k1: number = 0;
  /** @internal */
  beta1: number = 0;
  /** @internal */
  k2: number = 0;
  /** @internal */
  beta2: number = 0;
  /** @internal */
  invPeriod: number = 0;
  /** @internal */
  prevGain: number = 0;
  /** @internal */
  prevLoss: number = 0;
  /** @internal */
  prevValue: number = 0;
  /** @internal */
  ema1: number = 0;
  /** @internal */
  ema2: number = 0;
  /** @internal */
  sumSignal: number = 0;
  /** @internal */
  dsBuffer_Idx: number = 0;
  /** @internal */
  maxIdx_dsBuffer: number = 0;
  /** @internal */
  cbSize_dsBuffer: number = 0;
  /** @internal */
  cb_dsBuffer: Float64Array = new Float64Array(1);
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
  static of(core: Core): DoscStream {
    return new DoscStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link dosc} reports them over the same bars. Each accepted `update` and each
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
      throw failure('DOSC advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: Input series, usually the close.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('DOSC update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('DOSC update', 'inReal');
    }
    doscStepImpl(this, inReal);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Input series, usually the close.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number): number {
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('DOSC peek', 'inReal');
    }
    const sp = this;
    let gainDelta = 0;
    let tempValue1 = 0;
    let tempValue2 = 0;
    let rsiValue = 0;
    let cur_outReal = 0;
    let ema1 = sp.ema1;
    let ema2 = sp.ema2;
    let prevGain = sp.prevGain;
    let prevLoss = sp.prevLoss;
    let prevValue = sp.prevValue;
    let sumSignal = sp.sumSignal;
    tempValue1 = inReal;
    tempValue2 = tempValue1 - prevValue;
    prevValue = tempValue1;
    prevLoss *= (sp.optInTimePeriod - 1);
    prevGain *= (sp.optInTimePeriod - 1);
    gainDelta = (tempValue2 > 0.0) ? tempValue2 : 0.0;
    prevGain += gainDelta;
    prevLoss += gainDelta - tempValue2;
    prevLoss *= sp.invPeriod;
    prevGain *= sp.invPeriod;
    tempValue1 = prevGain + prevLoss;
    if (tempValue1 > 0.0) {
      rsiValue = 100.0 * (prevGain / tempValue1);
    } else {
      rsiValue = 50.0;
    }
    ema1 = fma(sp.beta1, ema1, sp.k1 * rsiValue);
    ema2 = fma(sp.beta2, ema2, sp.k2 * ema1);
    sumSignal = sumSignal + ema2;
    cur_outReal = ema2 - sumSignal / sp.optInSignalPeriod;
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
  clone(): DoscStream {
    const c = new DoscStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.optInFirstPeriod = this.optInFirstPeriod;
    c.optInSecondPeriod = this.optInSecondPeriod;
    c.optInSignalPeriod = this.optInSignalPeriod;
    c.k1 = this.k1;
    c.beta1 = this.beta1;
    c.k2 = this.k2;
    c.beta2 = this.beta2;
    c.invPeriod = this.invPeriod;
    c.prevGain = this.prevGain;
    c.prevLoss = this.prevLoss;
    c.prevValue = this.prevValue;
    c.ema1 = this.ema1;
    c.ema2 = this.ema2;
    c.sumSignal = this.sumSignal;
    c.dsBuffer_Idx = this.dsBuffer_Idx;
    c.maxIdx_dsBuffer = this.maxIdx_dsBuffer;
    c.cbSize_dsBuffer = this.cbSize_dsBuffer;
    c.cb_dsBuffer = this.cb_dsBuffer.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function doscStepImpl(sp: DoscStream, inReal: number): void {
  let gainDelta = 0;
  let tempValue1 = 0;
  let tempValue2 = 0;
  let rsiValue = 0;
  tempValue1 = inReal;
  tempValue2 = tempValue1 - sp.prevValue;
  sp.prevValue = tempValue1;
  sp.prevLoss *= (sp.optInTimePeriod - 1);
  sp.prevGain *= (sp.optInTimePeriod - 1);
  gainDelta = (tempValue2 > 0.0) ? tempValue2 : 0.0;
  sp.prevGain += gainDelta;
  sp.prevLoss += gainDelta - tempValue2;
  sp.prevLoss *= sp.invPeriod;
  sp.prevGain *= sp.invPeriod;
  tempValue1 = sp.prevGain + sp.prevLoss;
  if (tempValue1 > 0.0) {
    rsiValue = 100.0 * (sp.prevGain / tempValue1);
  } else {
    rsiValue = 50.0;
  }
  sp.ema1 = fma(sp.beta1, sp.ema1, sp.k1 * rsiValue);
  sp.ema2 = fma(sp.beta2, sp.ema2, sp.k2 * sp.ema1);
  sp.cb_dsBuffer[sp.dsBuffer_Idx] = sp.ema2;
  sp.sumSignal = sp.sumSignal + sp.ema2;
  sp.cur_outReal = sp.ema2 - sp.sumSignal / sp.optInSignalPeriod;
  sp.dsBuffer_Idx = sp.dsBuffer_Idx + 1;
  if (sp.dsBuffer_Idx > sp.maxIdx_dsBuffer) {
    sp.dsBuffer_Idx = 0;
  }
  sp.sumSignal = sp.sumSignal - sp.cb_dsBuffer[sp.dsBuffer_Idx];
}

function doscOpenImpl(
  sp: DoscStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInFirstPeriod: number,
  optInSecondPeriod: number,
  optInSignalPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let k1 = 0;
  let beta1 = 0;
  let k2 = 0;
  let beta2 = 0;
  let invPeriod = 0;
  let prevGain = 0;
  let prevLoss = 0;
  let prevValue = 0;
  let gainDelta = 0;
  let tempValue1 = 0;
  let tempValue2 = 0;
  let rsiValue = 0;
  let ema1 = 0;
  let ema2 = 0;
  let sum1 = 0;
  let sum2 = 0;
  let sumSignal = 0;
  let lookbackTotal = 0;
  let lookbackRSI = 0;
  let lookbackEMA1 = 0;
  let lookbackEMA2 = 0;
  let skipRSI = 0;
  let today = 0;
  let i = 0;
  let outIdx = 0;
  let rsiBar = 0;
  let nRsi = 0;
  let n1 = 0;
  let n2 = 0;
  let dsBuffer!: Float64Array;
  let dsBuffer_Idx = 0;
  let maxIdx_dsBuffer = (32) - 1;
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
  if (optInFirstPeriod === Core.INTEGER_DEFAULT) {
    optInFirstPeriod = 5;
  } else if (optInFirstPeriod < 2 || optInFirstPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSecondPeriod === Core.INTEGER_DEFAULT) {
    optInSecondPeriod = 3;
  } else if (optInSecondPeriod < 2 || optInSecondPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSignalPeriod === Core.INTEGER_DEFAULT) {
    optInSignalPeriod = 9;
  } else if (optInSignalPeriod < 2 || optInSignalPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Constance Brown's "triple smoothed derivative of RSI plotted as a
   * histogram" (MTA Journal, 1994): MACD's histogram construction applied to
   * a double-smoothed RSI.
   *
   *    S1_t   = EMA(RSI(x, t), f)_t
   *    DS_t   = EMA(S1, s)_t
   *    DOSC_t = DS_t - SMA(DS, g)_t
   *
   * The output must stay bit-identical to
   * TA_RSI -> TA_EMA -> TA_EMA -> TA_SMA -> TA_SUB, so every stage keeps its
   * callee's operation order, and every stage boundary is the callee's
   * LOOKBACK, not (period-1): a warm unstable period then folds in.
   *
   * Compare the counters BEFORE subtracting them: the Rust backend renders
   * them usize, where `n = nRsi - skipRSI; if( n >= 0 )` underflows.
   */
  /* This ptr will point on a circular buffer of at least
   * "optInSignalPeriod" element.
   */
  lookbackTotal = doscLookbackPositional(optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod, core);
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
  if (optInSignalPeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  dsBuffer = new Float64Array(optInSignalPeriod);
  maxIdx_dsBuffer = optInSignalPeriod - 1;
  dsBuffer_Idx = 0;
  lookbackRSI = rsiLookbackPositional(optInTimePeriod, core);
  lookbackEMA1 = emaLookbackPositional(optInFirstPeriod, core);
  lookbackEMA2 = emaLookbackPositional(optInSecondPeriod, core);
  /* The RSI values the composed chain never publishes. Derive the count
   * from the lookback, never from TA_GetUnstablePeriod: under the Auto
   * levels it is a function of the period.
   */
  skipRSI = lookbackRSI - optInTimePeriod;
  /* k and beta must sum to exactly 1.0, or a flat input drifts off its
   * level. Each subtraction is exact only from an operand in [0.5,1): at a
   * period of 2 that is k, above it beta. Above it beta stays as the divide
   * wrote it: a register last written by a subtraction costs each FMA
   * reading it one more cycle on Intel P-cores.
   */
  beta1 = (optInFirstPeriod - 1) / (optInFirstPeriod + 1);
  k1 = 1.0 - beta1;
  if (beta1 < 0.5) {
    beta1 = 1.0 - k1;
  }
  beta2 = (optInSecondPeriod - 1) / (optInSecondPeriod + 1);
  k2 = 1.0 - beta2;
  if (beta2 < 0.5) {
    beta2 = 1.0 - k2;
  }
  ema1 = 0.0;
  ema2 = 0.0;
  sum1 = 0.0;
  sum2 = 0.0;
  sumSignal = 0.0;
  nRsi = 0;
  invPeriod = 1.0 / optInTimePeriod;
  today = startIdx - lookbackTotal;
  rsiBar = today + optInTimePeriod;
  prevValue = inReal[today];
  prevGain = 0.0;
  prevLoss = 0.0;
  today = today + 1;
  for (i = optInTimePeriod; i > 0; i -= 1) {
    tempValue1 = inReal[today];
    today = today + 1;
    tempValue2 = tempValue1 - prevValue;
    prevValue = tempValue1;
    gainDelta = (tempValue2 > 0.0) ? tempValue2 : 0.0;
    prevGain += gainDelta;
    prevLoss += gainDelta - tempValue2;
  }
  prevLoss *= invPeriod;
  prevGain *= invPeriod;
  tempValue1 = prevGain + prevLoss;
  if (tempValue1 > 0.0) {
    rsiValue = 100.0 * (prevGain / tempValue1);
  } else {
    rsiValue = 50.0;
  }
  /* Warm-up. Feeds every RSI value before startIdx through the chain and
   * leaves rsiValue holding startIdx's own. Nothing is emitted here: the
   * first bar whose signal window is full is startIdx, by construction of
   * the lookback.
   */
  while (rsiBar < startIdx) {
    if (nRsi >= skipRSI) {
      n1 = nRsi - skipRSI;
      if (n1 < optInFirstPeriod) {
        sum1 = sum1 + rsiValue;
        if (n1 === optInFirstPeriod - 1) {
          ema1 = sum1 / optInFirstPeriod;
        }
      } else {
        {
          const _a: number = beta1, _b: number = ema1, _c: number = k1 * rsiValue;
          const _p = _a * _b;
          let _t = 134217729 * _a;
          const _ah = _t - (_t - _a), _al = _a - _ah;
          _t = 134217729 * _b;
          const _bh = _t - (_t - _b), _bl = _b - _bh;
          const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
          const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
          const _v = _tl + _pl, _v2 = _v + _v;
          const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
          ema1 = _w && (_pl === 0 || _v === 0) ? _th
            : _w && _v2 !== (_th + _v2) - _th ? _th + _v
            : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
        }
      }
      if (n1 >= lookbackEMA1) {
        n2 = n1 - lookbackEMA1;
        if (n2 < optInSecondPeriod) {
          sum2 = sum2 + ema1;
          if (n2 === optInSecondPeriod - 1) {
            ema2 = sum2 / optInSecondPeriod;
          }
        } else {
          {
            const _a: number = beta2, _b: number = ema2, _c: number = k2 * ema1;
            const _p = _a * _b;
            let _t = 134217729 * _a;
            const _ah = _t - (_t - _a), _al = _a - _ah;
            _t = 134217729 * _b;
            const _bh = _t - (_t - _b), _bl = _b - _bh;
            const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
            const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
            const _v = _tl + _pl, _v2 = _v + _v;
            const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
            ema2 = _w && (_pl === 0 || _v === 0) ? _th
              : _w && _v2 !== (_th + _v2) - _th ? _th + _v
              : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
          }
        }
        if (n2 >= lookbackEMA2) {
          dsBuffer[dsBuffer_Idx] = ema2;
          sumSignal = sumSignal + ema2;
          dsBuffer_Idx++;
          if (dsBuffer_Idx > maxIdx_dsBuffer) {
            dsBuffer_Idx = 0;
          }
        }
      }
    }
    nRsi = nRsi + 1;
    tempValue1 = inReal[today];
    today = today + 1;
    tempValue2 = tempValue1 - prevValue;
    prevValue = tempValue1;
    prevLoss *= (optInTimePeriod - 1);
    prevGain *= (optInTimePeriod - 1);
    gainDelta = (tempValue2 > 0.0) ? tempValue2 : 0.0;
    prevGain += gainDelta;
    prevLoss += gainDelta - tempValue2;
    prevLoss *= invPeriod;
    prevGain *= invPeriod;
    tempValue1 = prevGain + prevLoss;
    if (tempValue1 > 0.0) {
      rsiValue = 100.0 * (prevGain / tempValue1);
    } else {
      rsiValue = 50.0;
    }
    rsiBar = rsiBar + 1;
  }
  /* The first output. Every stage is past its seed here, even at periods
   * 2/2/2/2, so the store below and the one in the stable loop are
   * unconditional. Keep them so: under a guard, the managed peek frames
   * seed a dead output local.
   */
  ema1 = fma(beta1, ema1, k1 * rsiValue);
  ema2 = fma(beta2, ema2, k2 * ema1);
  dsBuffer[dsBuffer_Idx] = ema2;
  sumSignal = sumSignal + ema2;
  outReal[((0 * outStride) | 0)] = ema2 - sumSignal / optInSignalPeriod;
  outIdx = 1;
  dsBuffer_Idx++;
  if (dsBuffer_Idx > maxIdx_dsBuffer) {
    dsBuffer_Idx = 0;
  }
  sumSignal = sumSignal - dsBuffer[dsBuffer_Idx];
  /* Stable zone. */
  while (today <= endIdx) {
    tempValue1 = inReal[today];
    today = today + 1;
    tempValue2 = tempValue1 - prevValue;
    prevValue = tempValue1;
    prevLoss *= (optInTimePeriod - 1);
    prevGain *= (optInTimePeriod - 1);
    gainDelta = (tempValue2 > 0.0) ? tempValue2 : 0.0;
    prevGain += gainDelta;
    prevLoss += gainDelta - tempValue2;
    prevLoss *= invPeriod;
    prevGain *= invPeriod;
    tempValue1 = prevGain + prevLoss;
    if (tempValue1 > 0.0) {
      rsiValue = 100.0 * (prevGain / tempValue1);
    } else {
      rsiValue = 50.0;
    }
    {
      const _a: number = beta1, _b: number = ema1, _c: number = k1 * rsiValue;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      ema1 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = beta2, _b: number = ema2, _c: number = k2 * ema1;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      ema2 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    dsBuffer[dsBuffer_Idx] = ema2;
    sumSignal = sumSignal + ema2;
    outReal[((outIdx * outStride) | 0)] = ema2 - sumSignal / optInSignalPeriod;
    outIdx = outIdx + 1;
    dsBuffer_Idx++;
    if (dsBuffer_Idx > maxIdx_dsBuffer) {
      dsBuffer_Idx = 0;
    }
    sumSignal = sumSignal - dsBuffer[dsBuffer_Idx];
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const capCb_dsBuffer = maxIdx_dsBuffer + 1;
  if (capCb_dsBuffer > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.optInFirstPeriod = optInFirstPeriod;
  sp.optInSecondPeriod = optInSecondPeriod;
  sp.optInSignalPeriod = optInSignalPeriod;
  sp.k1 = k1;
  sp.beta1 = beta1;
  sp.k2 = k2;
  sp.beta2 = beta2;
  sp.invPeriod = invPeriod;
  sp.prevGain = prevGain;
  sp.prevLoss = prevLoss;
  sp.prevValue = prevValue;
  sp.ema1 = ema1;
  sp.ema2 = ema2;
  sp.sumSignal = sumSignal;
  sp.dsBuffer_Idx = dsBuffer_Idx;
  sp.maxIdx_dsBuffer = maxIdx_dsBuffer;
  sp.cbSize_dsBuffer = capCb_dsBuffer;
  sp.cb_dsBuffer = dsBuffer;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function doscOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInFirstPeriod: number,
  optInSecondPeriod: number,
  optInSignalPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): DoscStream {
  const sp = DoscStream.of(core);
  const retCode = doscOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('DOSC openAndFill', inReal.length, startIdx, doscLookbackPositional(optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && doscLookbackPositional(optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod, core) < 0) {
    throw rejectedParams('DOSC openAndFill', PARAM_RANGES, optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod);
  }
  throw streamFailure('DOSC openAndFill', retCode);
}

/** @internal */
export function doscOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInFirstPeriod: number,
  optInSecondPeriod: number,
  optInSignalPeriod: number,
): DoscStream {
  const sp = DoscStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = doscOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('DOSC open', inReal.length, startIdx, doscLookbackPositional(optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && doscLookbackPositional(optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod, core) < 0) {
    throw rejectedParams('DOSC open', PARAM_RANGES, optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod);
  }
  throw streamFailure('DOSC open', retCode);
}

/** @internal */
export function doscOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInFirstPeriod: number,
  optInSecondPeriod: number,
  optInSignalPeriod: number,
  core: Core = Core.DEFAULT,
): DoscStream {
  optInTimePeriod = requireInt32('DOSC open', 'timePeriod', optInTimePeriod);
  optInFirstPeriod = requireInt32('DOSC open', 'firstPeriod', optInFirstPeriod);
  optInSecondPeriod = requireInt32('DOSC open', 'secondPeriod', optInSecondPeriod);
  optInSignalPeriod = requireInt32('DOSC open', 'signalPeriod', optInSignalPeriod);
  requireArgument('DOSC open', 'real', inReal);
  requireHistory('DOSC open', inReal.length);
  return doscOpenInternal(core, float64(inReal), 0, optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod);
}

/** @internal */
export function doscOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  optInFirstPeriod: number,
  optInSecondPeriod: number,
  optInSignalPeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): DoscStream {
  optInTimePeriod = requireInt32('DOSC openAndFill', 'timePeriod', optInTimePeriod);
  optInFirstPeriod = requireInt32('DOSC openAndFill', 'firstPeriod', optInFirstPeriod);
  optInSecondPeriod = requireInt32('DOSC openAndFill', 'secondPeriod', optInSecondPeriod);
  optInSignalPeriod = requireInt32('DOSC openAndFill', 'signalPeriod', optInSignalPeriod);
  requireArgument('DOSC openAndFill', 'real', inReal);
  requireHistory('DOSC openAndFill', inReal.length);
  const guardLookback = doscLookbackPositional(optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('DOSC openAndFill', PARAM_RANGES, optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod);
  }
  const guardOutLen = openFillCount('DOSC openAndFill', inReal.length, guardLookback);
  requireOutput('DOSC openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('DOSC openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('DOSC openAndFill', 'outReal', 'real');
  }
  return doscOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live DOSC stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link dosc} there.
 *
 * @param real - Warm-up history. Input series, usually the close.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link DoscOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `doscLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { doscOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = doscOpen(data, {
 *   timePeriod: 14,
 *   firstPeriod: 5,
 *   secondPeriod: 3,
 *   signalPeriod: 9,
 * });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function doscOpen(real: ArrayLike<number>, options?: DoscOptions): DoscStream {
  const o = optionsOf('DOSC open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInFirstPeriod = orDefault(o.firstPeriod, Core.INTEGER_DEFAULT);
  const optInSecondPeriod = orDefault(o.secondPeriod, Core.INTEGER_DEFAULT);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('DOSC open', o.core);
  return doscOpenPositional(real, optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod, core);
}

/**
 * {@link doscOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link dosc}.
 *
 * @param real - Warm-up history. Input series, usually the close.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link DoscOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link dosc} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `doscLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or a
 *   buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { doscOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = doscOpenAndFill(data, {
 *   timePeriod: 14,
 *   firstPeriod: 5,
 *   secondPeriod: 3,
 *   signalPeriod: 9,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function doscOpenAndFill(real: ArrayLike<number>, options?: DoscOptions): DoscResult & { stream: DoscStream } {
  const o = optionsOf('DOSC openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const optInFirstPeriod = orDefault(o.firstPeriod, Core.INTEGER_DEFAULT);
  const optInSecondPeriod = orDefault(o.secondPeriod, Core.INTEGER_DEFAULT);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('DOSC openAndFill', o.core);
  const capacity = fillCapacity(doscLookbackPositional(optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = doscOpenAndFillPositional(real, optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
