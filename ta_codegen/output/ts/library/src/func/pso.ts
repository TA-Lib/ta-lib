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
 *  100626 KL,CC  Initial version (#473).
 *  100726 MF,CC  Batch tier: block scan of the Fast-K window (#473).
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma, fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { emaLookbackPositional } from './ema.ts';

/** @internal */
export function psoLookbackPositional(optInFastK_Period: number, optInEMAPeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInFastK_Period | 0) !== optInFastK_Period) {
    return -1;
  }
  if ((optInEMAPeriod | 0) !== optInEMAPeriod) {
    return -1;
  }
  if (optInFastK_Period === Core.INTEGER_DEFAULT) {
    optInFastK_Period = 8;
  } else if (optInFastK_Period < 1 || optInFastK_Period > 100000) {
    return -1;
  }
  if (optInEMAPeriod === Core.INTEGER_DEFAULT) {
    optInEMAPeriod = 5;
  } else if (optInEMAPeriod < 1 || optInEMAPeriod > 100000) {
    return -1;
  }
  /* One Fast-K window, then the two EMA warm-ups the author stacks on top of
   * it: the first smooths the normalised Fast-K, the second smooths the
   * first. Both terms are exactly the lookback of the function they come
   * from, so neither is restated here -- which is also what makes PSO
   * inherit TA_FUNC_UNST_EMA from its callee rather than take an id of its
   * own, and what carries the Auto warm-up levels of #492 through both
   * passes without this file knowing their rule.
   */
  return optInFastK_Period - 1 + emaLookbackPositional(optInEMAPeriod, core) + emaLookbackPositional(optInEMAPeriod, core);
}

/** @internal */
export function psoDisplayShiftPositional(optInFastK_Period: number, optInEMAPeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (psoLookbackPositional(optInFastK_Period, optInEMAPeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function psoImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  optInFastK_Period: number,
  optInEMAPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let sufHighest!: Float64Array;
  let preHighest!: Float64Array;
  let sufLowest!: Float64Array;
  let preLowest!: Float64Array;
  let emaK = 0;
  let emaBeta = 0;
  let highest = 0;
  let lowest = 0;
  let tmp = 0;
  let tempReal = 0;
  let fastK = 0;
  let nsk = 0;
  let ema1 = 0;
  let ema2 = 0;
  let sum1 = 0;
  let sum2 = 0;
  let lookbackTotal = 0;
  let lookbackEMA = 0;
  let warmBars = 0;
  let today = 0;
  let i = 0;
  let m = 0;
  let blockStart = 0;
  let blockNext = 0;
  let nAvail = 0;
  let outIdx = 0;
  let nBar = 0;
  let n2 = 0;
  let nOut = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInFastK_Period === Core.INTEGER_DEFAULT) {
    optInFastK_Period = 8;
  } else if (optInFastK_Period < 1 || optInFastK_Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInEMAPeriod === Core.INTEGER_DEFAULT) {
    optInEMAPeriod = 5;
  } else if (optInEMAPeriod < 1 || optInEMAPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  lookbackTotal = psoLookbackPositional(optInFastK_Period, optInEMAPeriod, core);
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
  /* Same values as pso_ALT1 below, which carries the formula. Only the
   * Fast-K window differs: a Van Herk / Gil-Werman block scan (WILLR's,
   * issue #147), so the cost per bar does not depend on the period or on
   * the shape of the input, where the cached extremum of pso_ALT1 rescans
   * its whole window on every bar of a flat or trending stretch. Every
   * scratch array holds copies, so the output may alias an input.
   */
  emaBeta = (optInEMAPeriod - 1) / (optInEMAPeriod + 1);
  emaK = 1.0 - emaBeta;
  if (emaBeta < 0.5) {
    emaBeta = 1.0 - emaK;
  }
  lookbackEMA = emaLookbackPositional(optInEMAPeriod, core);
  warmBars = lookbackEMA + lookbackEMA;
  ema1 = 0.0;
  ema2 = 0.0;
  sum1 = 0.0;
  sum2 = 0.0;
  nBar = 0;
  today = startIdx - warmBars;
  blockStart = today - (optInFastK_Period - 1);
  outIdx = 0;
  if (optInFastK_Period < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sufHighest = new Float64Array(optInFastK_Period);
  if (optInFastK_Period < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  preHighest = new Float64Array(optInFastK_Period);
  if (optInFastK_Period < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sufLowest = new Float64Array(optInFastK_Period);
  if (optInFastK_Period < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  preLowest = new Float64Array(optInFastK_Period);
  while (today <= endIdx) {
    /* Suffix extrema of the block [blockStart, today]. */
    i = today;
    highest = inHigh[i];
    lowest = inLow[i];
    sufHighest[optInFastK_Period - 1] = highest;
    sufLowest[optInFastK_Period - 1] = lowest;
    while (i > blockStart) {
      i -= 1;
      tmp = inHigh[i];
      if (tmp > highest) {
        highest = tmp;
      }
      tmp = inLow[i];
      if (tmp < lowest) {
        lowest = tmp;
      }
      sufHighest[i - blockStart] = highest;
      sufLowest[i - blockStart] = lowest;
    }
    /* Prefix extrema of the next block, clamped to what remains, stored
     * one slot up: slot 0 repeats the suffix so that bar 'today', whose
     * window is the block itself, runs the same combine as the others.
     */
    blockNext = blockStart + optInFastK_Period;
    nAvail = endIdx + 1 - blockNext;
    if (nAvail > optInFastK_Period - 1) {
      nAvail = optInFastK_Period - 1;
    }
    preHighest[0] = sufHighest[0];
    preLowest[0] = sufLowest[0];
    if (nAvail > 0) {
      i = 1;
      highest = inHigh[blockNext];
      lowest = inLow[blockNext];
      preHighest[1] = highest;
      preLowest[1] = lowest;
      while (i < nAvail) {
        tmp = inHigh[blockNext + i];
        if (tmp > highest) {
          highest = tmp;
        }
        tmp = inLow[blockNext + i];
        if (tmp < lowest) {
          lowest = tmp;
        }
        preHighest[i + 1] = highest;
        preLowest[i + 1] = lowest;
        i += 1;
      }
    }
    /* The squash runs as a pass of its own over what the block wrote:
     * a call inside the loop above it would have every carried value
     * saved and restored around it on each bar.
     */
    nOut = 0;
    m = 0;
    while (m <= nAvail) {
      highest = sufHighest[m];
      if (preHighest[m] > highest) {
        highest = preHighest[m];
      }
      lowest = sufLowest[m];
      if (preLowest[m] < lowest) {
        lowest = preLowest[m];
      }
      tempReal = highest - lowest;
      if (!(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(highest) + Math.abs(lowest)))) {
        fastK = (inClose[today + m] - lowest) / tempReal * 100.0;
      } else {
        fastK = 50.0;
      }
      nsk = 0.1 * (fastK - 50.0);
      if (nBar > warmBars) {
        {
          const _a: number = emaBeta, _b: number = ema1, _c: number = emaK * nsk;
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
          const _a: number = emaBeta, _b: number = ema2, _c: number = emaK * ema1;
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
        outReal[outIdx + nOut] = 0.5 * ema2;
        nOut = nOut + 1;
      } else {
        /* The two seeds, staged as pso_ALT1 stages them. */
        if (nBar < optInEMAPeriod) {
          sum1 = sum1 + nsk;
          if (nBar === optInEMAPeriod - 1) {
            ema1 = sum1 / optInEMAPeriod;
          }
        } else {
          {
            const _a: number = emaBeta, _b: number = ema1, _c: number = emaK * nsk;
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
        if (nBar >= lookbackEMA) {
          n2 = nBar - lookbackEMA;
          if (n2 < optInEMAPeriod) {
            sum2 = sum2 + ema1;
            if (n2 === optInEMAPeriod - 1) {
              ema2 = sum2 / optInEMAPeriod;
            }
          } else {
            {
              const _a: number = emaBeta, _b: number = ema2, _c: number = emaK * ema1;
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
        }
        if (nBar === warmBars) {
          outReal[outIdx + nOut] = 0.5 * ema2;
          nOut = nOut + 1;
        }
      }
      nBar = nBar + 1;
      m += 1;
    }
    i = 0;
    while (i < nOut) {
      outReal[outIdx] = Math.tanh(outReal[outIdx]);
      outIdx = outIdx + 1;
      i += 1;
    }
    today = today + nAvail + 1;
    blockStart = blockNext;
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['fastKPeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['emaPeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000']] as const;

/** @internal */
export function psoPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInFastK_Period: number,
  optInEMAPeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('PSO', 'startIdx', startIdx);
  endIdx = requireInt32('PSO', 'endIdx', endIdx);
  optInFastK_Period = requireInt32('PSO', 'fastKPeriod', optInFastK_Period);
  optInEMAPeriod = requireInt32('PSO', 'emaPeriod', optInEMAPeriod);
  requireIndexRange('PSO', startIdx, endIdx);
  const guardLookback = psoLookbackPositional(optInFastK_Period, optInEMAPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('PSO', PARAM_RANGES, optInFastK_Period, optInEMAPeriod);
  }
  const guardStart = clampedStart('PSO', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('PSO', 'prices.high', inHigh, guardInLen);
  requireLength('PSO', 'prices.low', inLow, guardInLen);
  requireLength('PSO', 'prices.close', inClose, guardInLen);
  requireOutput('PSO', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outReal, inHighF64)) {
    throw sharedMemory('PSO', 'outReal', 'prices.high');
  }
  if (overlapsPartly(outReal, inLowF64)) {
    throw sharedMemory('PSO', 'outReal', 'prices.low');
  }
  if (overlapsPartly(outReal, inCloseF64)) {
    throw sharedMemory('PSO', 'outReal', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = psoImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInFastK_Period, optInEMAPeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('PSO', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link pso}, {@link psoLookback}, {@link psoDisplayShift},
 * {@link psoOpen} and {@link psoOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface PsoOptions {
  /**
   * Time period for building the Fast-K line (default 8; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  fastKPeriod?: number | undefined;
  /**
   * Period of each of the two smoothing passes (default 5; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  emaPeriod?: number | undefined;
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

/** What {@link pso} returns, and {@link psoOpenAndFill} with its stream. */
export interface PsoResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Premier Stochastic Oscillator, -1 to +1. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link pso} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link PsoOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function psoLookback(options?: PsoOptions): number {
  const o = optionsOf('PSO lookback', options);
  return psoLookbackPositional(orDefault(o.fastKPeriod, Core.INTEGER_DEFAULT), orDefault(o.emaPeriod, Core.INTEGER_DEFAULT), coreOf('PSO lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link pso}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link PsoOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function psoDisplayShift(outputIdx: number, options?: PsoOptions): number {
  const o = optionsOf('PSO displayShift', options);
  return psoDisplayShiftPositional(orDefault(o.fastKPeriod, Core.INTEGER_DEFAULT), orDefault(o.emaPeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('PSO displayShift', o.core));
}

/**
 * Premier Stochastic Oscillator: a short-period Fast %K, recentred on zero and
 * rescaled, double-smoothed and then squashed into the range -1 to +1. Leibfarth's
 * reading is that the plain stochastic spends most of its life pinned at one end
 * or the other, so the extremes stop meaning anything; the two exponential passes
 * strip the bar-to-bar noise out of it, and the squash gives back a scale on which
 * the extremes are rare again. Readings beyond ±0.9 are the extremes, and ±0.2 the
 * band Leibfarth watches for the crossing back toward the middle.
 *
 * Formula and more info at
 * [ta-lib.org/functions/pso](https://ta-lib.org/functions/pso).
 *
 * **Notes**
 * - The affine step comes before the smoothing, as the author's listing spells it.
 *   Smoothing first and recentring after is the same value in real arithmetic and
 *   differs in the last bit or two in doubles.
 * - The squash is computed as `tanh(SS/2)`, which is the published quotient
 *   rewritten. The quotient form is `inf/inf` once `SS` exceeds 709.78, which a
 *   bar whose close lies outside its own high/low range can reach; `tanh`
 *   saturates at ±1 instead.
 * - A Fast-K window whose range is zero reads 50, the midpoint, so a flat market
 *   reads PSO 0 rather than the near-extreme the Fast-K convention of 0 would give
 *   it. The flatness test is the one STOCHF applies, against the window's own
 *   extremes rather than a fixed band.
 * - Each exponential pass is seeded with a simple average of its own first inputs,
 *   the same seeding TA-Lib's EMA uses, and the second pass seeds on what the
 *   first publishes. `CoreBuilder.unstablePeriod(FuncUnstId.EMA, ...)` discards
 *   more of that warm-up, through both passes. Implementations seeding each pass
 *   from a single first sample differ over the transient and agree once it decays.
 * - Leibfarth parameterises the smoothing as the square root of a longer period,
 *   25 in his article. The length is taken here directly, as an integer, because
 *   the sources that follow the square root disagree over how to round it.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link psoLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `high`: High price
 *   series. `low`: Low price series. `close`: Close price series.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link PsoOptions}.
 * @returns `begIdx` and the values; see {@link PsoResult}. Each output is a view
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
 * import { pso } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { begIdx, values } = pso({ high, low, close }, {
 *   fastKPeriod: 8,
 *   emaPeriod: 5,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`stochf`](https://ta-lib.org/functions/stochf)
 * @see [`stoch`](https://ta-lib.org/functions/stoch)
 * @see [`smi`](https://ta-lib.org/functions/smi)
 * @see [`willr`](https://ta-lib.org/functions/willr)
 */
export function pso(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: PsoOptions): PsoResult {
  requireArgument('PSO', 'prices', prices);
  const o = optionsOf('PSO', options);
  if (o.endIdx === undefined) {
    requireSeries('PSO', 'prices.high', prices.high);
    requireSeries('PSO', 'prices.low', prices.low);
    requireSeries('PSO', 'prices.close', prices.close);
  }
  const optInFastK_Period = orDefault(o.fastKPeriod, Core.INTEGER_DEFAULT);
  const optInEMAPeriod = orDefault(o.emaPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('PSO', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, psoLookbackPositional(optInFastK_Period, optInEMAPeriod, core), prices.high, prices.low, prices.close);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = psoPositional(startIdx, endIdx, prices.high, prices.low, prices.close, optInFastK_Period, optInEMAPeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/* Using pso_ALT1 for TA_ALT={STREAM,ALL_LANGUAGES} */

/**
 * A live PSO stream: one value per closed bar, bit-identical to {@link pso} over
 * the same series. Open it with {@link psoOpen} or {@link psoOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/pso](https://ta-lib.org/functions/pso).
 */
export class PsoStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInFastK_Period: number = 0;
  /** @internal */
  optInEMAPeriod: number = 0;
  /** @internal */
  emaK: number = 0;
  /** @internal */
  emaBeta: number = 0;
  /** @internal */
  highest: number = 0;
  /** @internal */
  lowest: number = 0;
  /** @internal */
  ema1: number = 0;
  /** @internal */
  ema2: number = 0;
  /** @internal */
  trailingIdx: number = 0;
  /** @internal */
  highestIdx: number = 0;
  /** @internal */
  lowestIdx: number = 0;
  /** @internal */
  i: number = 0;
  /** @internal */
  today: number = 0;
  /** @internal */
  xMask: number = 0;
  /** @internal */
  x_inHigh: Float64Array = new Float64Array(1);
  /** @internal */
  x_inLow: Float64Array = new Float64Array(1);
  /** @internal */
  x_inClose: Float64Array = new Float64Array(1);
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
  static of(core: Core): PsoStream {
    return new PsoStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link pso} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('PSO advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param high - The bar's `high` value: High price series.
   * @param low - The bar's `low` value: Low price series.
   * @param close - The bar's `close` value: Close price series.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(high: number, low: number, close: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('PSO update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('PSO update', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    psoStepImpl(this, high, low, close);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param high - The bar's `high` value: High price series.
   * @param low - The bar's `low` value: Low price series.
   * @param close - The bar's `close` value: Close price series.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(high: number, low: number, close: number): number {
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('PSO peek', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
    const inClose = close;
    let tmp = 0;
    let fastK = 0;
    let nsk = 0;
    let cur_outReal = 0;
    let ema1 = sp.ema1;
    let ema2 = sp.ema2;
    let highest = sp.highest;
    let highestIdx = sp.highestIdx;
    let i = sp.i;
    let lowest = sp.lowest;
    let lowestIdx = sp.lowestIdx;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    let pkSlot1 = -1;
    let pkVal1 = 0;
    let pkSlot2 = -1;
    let pkVal2 = 0;
    pkSlot0 = sp.today & sp.xMask;
    pkVal0 = inHigh;
    pkSlot1 = sp.today & sp.xMask;
    pkVal1 = inLow;
    pkSlot2 = sp.today & sp.xMask;
    pkVal2 = inClose;
    /* Set the lowest low */
    tmp = ((sp.today & sp.xMask) !== pkSlot1) ? sp.x_inLow[sp.today & sp.xMask] : pkVal1;
    if (lowestIdx < sp.trailingIdx) {
      lowestIdx = sp.trailingIdx;
      lowest = ((lowestIdx & sp.xMask) !== pkSlot1) ? sp.x_inLow[lowestIdx & sp.xMask] : pkVal1;
      i = lowestIdx;
      while (++i <= sp.today) {
        tmp = ((i & sp.xMask) !== pkSlot1) ? sp.x_inLow[i & sp.xMask] : pkVal1;
        if (tmp < lowest) {
          lowestIdx = i;
          lowest = tmp;
        }
      }
    } else if (tmp <= lowest) {
      lowestIdx = sp.today;
      lowest = tmp;
    }
    /* Set the highest high */
    tmp = ((sp.today & sp.xMask) !== pkSlot0) ? sp.x_inHigh[sp.today & sp.xMask] : pkVal0;
    if (highestIdx < sp.trailingIdx) {
      highestIdx = sp.trailingIdx;
      highest = ((highestIdx & sp.xMask) !== pkSlot0) ? sp.x_inHigh[highestIdx & sp.xMask] : pkVal0;
      i = highestIdx;
      while (++i <= sp.today) {
        tmp = ((i & sp.xMask) !== pkSlot0) ? sp.x_inHigh[i & sp.xMask] : pkVal0;
        if (tmp > highest) {
          highestIdx = i;
          highest = tmp;
        }
      }
    } else if (tmp >= highest) {
      highestIdx = sp.today;
      highest = tmp;
    }
    if (!(Math.abs(highest - lowest) <= 0.00000000000001 * (Math.abs(highest) + Math.abs(lowest)))) {
      fastK = ((((sp.today & sp.xMask) !== pkSlot2) ? sp.x_inClose[sp.today & sp.xMask] : pkVal2) - lowest) / (highest - lowest) * 100.0;
    } else {
      fastK = 50.0;
    }
    nsk = 0.1 * (fastK - 50.0);
    ema1 = fma(sp.emaBeta, ema1, sp.emaK * nsk);
    ema2 = fma(sp.emaBeta, ema2, sp.emaK * ema1);
    cur_outReal = Math.tanh(0.5 * ema2);
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
  clone(): PsoStream {
    const c = new PsoStream(this.core);
    c.optInFastK_Period = this.optInFastK_Period;
    c.optInEMAPeriod = this.optInEMAPeriod;
    c.emaK = this.emaK;
    c.emaBeta = this.emaBeta;
    c.highest = this.highest;
    c.lowest = this.lowest;
    c.ema1 = this.ema1;
    c.ema2 = this.ema2;
    c.trailingIdx = this.trailingIdx;
    c.highestIdx = this.highestIdx;
    c.lowestIdx = this.lowestIdx;
    c.i = this.i;
    c.today = this.today;
    c.xMask = this.xMask;
    c.x_inHigh = this.x_inHigh.slice();
    c.x_inLow = this.x_inLow.slice();
    c.x_inClose = this.x_inClose.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function psoStepImpl(sp: PsoStream, inHigh: number, inLow: number, inClose: number): void {
  let tmp = 0;
  let fastK = 0;
  let nsk = 0;
  sp.x_inHigh[sp.today & sp.xMask] = inHigh;
  sp.x_inLow[sp.today & sp.xMask] = inLow;
  sp.x_inClose[sp.today & sp.xMask] = inClose;
  /* Set the lowest low */
  tmp = sp.x_inLow[sp.today & sp.xMask];
  if (sp.lowestIdx < sp.trailingIdx) {
    sp.lowestIdx = sp.trailingIdx;
    sp.lowest = sp.x_inLow[sp.lowestIdx & sp.xMask];
    sp.i = sp.lowestIdx;
    while (++sp.i <= sp.today) {
      tmp = sp.x_inLow[sp.i & sp.xMask];
      if (tmp < sp.lowest) {
        sp.lowestIdx = sp.i;
        sp.lowest = tmp;
      }
    }
  } else if (tmp <= sp.lowest) {
    sp.lowestIdx = sp.today;
    sp.lowest = tmp;
  }
  /* Set the highest high */
  tmp = sp.x_inHigh[sp.today & sp.xMask];
  if (sp.highestIdx < sp.trailingIdx) {
    sp.highestIdx = sp.trailingIdx;
    sp.highest = sp.x_inHigh[sp.highestIdx & sp.xMask];
    sp.i = sp.highestIdx;
    while (++sp.i <= sp.today) {
      tmp = sp.x_inHigh[sp.i & sp.xMask];
      if (tmp > sp.highest) {
        sp.highestIdx = sp.i;
        sp.highest = tmp;
      }
    }
  } else if (tmp >= sp.highest) {
    sp.highestIdx = sp.today;
    sp.highest = tmp;
  }
  if (!(Math.abs(sp.highest - sp.lowest) <= 0.00000000000001 * (Math.abs(sp.highest) + Math.abs(sp.lowest)))) {
    fastK = (sp.x_inClose[sp.today & sp.xMask] - sp.lowest) / (sp.highest - sp.lowest) * 100.0;
  } else {
    fastK = 50.0;
  }
  nsk = 0.1 * (fastK - 50.0);
  sp.ema1 = fma(sp.emaBeta, sp.ema1, sp.emaK * nsk);
  sp.ema2 = fma(sp.emaBeta, sp.ema2, sp.emaK * sp.ema1);
  sp.cur_outReal = Math.tanh(0.5 * sp.ema2);
  sp.trailingIdx = sp.trailingIdx + 1;
  sp.today = sp.today + 1;
}

function psoOpenImpl(
  sp: PsoStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInFastK_Period: number,
  optInEMAPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let emaK = 0;
  let emaBeta = 0;
  let highest = 0;
  let lowest = 0;
  let tmp = 0;
  let fastK = 0;
  let nsk = 0;
  let ema1 = 0;
  let ema2 = 0;
  let sum1 = 0;
  let sum2 = 0;
  let lookbackTotal = 0;
  let lookbackEMA = 0;
  let today = 0;
  let trailingIdx = 0;
  let highestIdx = 0;
  let lowestIdx = 0;
  let i = 0;
  let outIdx = 0;
  let nBar = 0;
  let n2 = 0;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length || inClose.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInFastK_Period === Core.INTEGER_DEFAULT) {
    optInFastK_Period = 8;
  } else if (optInFastK_Period < 1 || optInFastK_Period > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInEMAPeriod === Core.INTEGER_DEFAULT) {
    optInEMAPeriod = 5;
  } else if (optInEMAPeriod < 1 || optInEMAPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  lookbackTotal = psoLookbackPositional(optInFastK_Period, optInEMAPeriod, core);
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
  outBegIdx.value = startIdx;
  /* Leibfarth's pipeline in one pass: a Fast-K window, the affine step that
   * centres it on zero, two EMA passes and the squash.
   *
   * The affine step comes BEFORE the smoothing, as the article's listing
   * spells it. Moving it after is equal in real arithmetic and differs by up
   * to 6.0e-16 absolute in doubles, which no golden at a sane tolerance can
   * see; only the composite gate against TA_STOCHF + TA_EMA + TA_EMA can.
   *
   * Each pass seeds the way ema.c does -- a simple average of that pass's
   * first optInEMAPeriod inputs, summed from 0.0 in production order -- so
   * the result is bit-identical to that composed chain. The stage boundary
   * below is the callee LOOKBACK, not (period-1), so that a warm
   * TA_SetUnstablePeriod(TA_FUNC_UNST_EMA) folds in: the second pass then
   * seeds from the values the first would have published, exactly as the
   * composed form does.
   *
   * At optInEMAPeriod == 1 the recursion runs at a k of 1.0 and a beta of
   * 0.0 where the composed chain copies: the same bits while every Fast-K
   * is finite.
   */
  emaBeta = (optInEMAPeriod - 1) / (optInEMAPeriod + 1);
  emaK = 1.0 - emaBeta;
  if (emaBeta < 0.5) {
    emaBeta = 1.0 - emaK;
  }
  lookbackEMA = emaLookbackPositional(optInEMAPeriod, core);
  ema1 = 0.0;
  ema2 = 0.0;
  sum1 = 0.0;
  sum2 = 0.0;
  highest = 0.0;
  lowest = 0.0;
  highestIdx = -1;
  lowestIdx = -1;
  /* The first bar carrying a full Fast-K window. */
  trailingIdx = startIdx - lookbackTotal;
  today = trailingIdx + (optInFastK_Period - 1);
  nBar = 0;
  /* Warm-up. Runs through startIdx inclusive: its last pass produces the
   * first output.
   */
  while (today <= startIdx) {
    /* Set the lowest low */
    tmp = inLow[today];
    if (lowestIdx < trailingIdx) {
      lowestIdx = trailingIdx;
      lowest = inLow[lowestIdx];
      i = lowestIdx;
      while (++i <= today) {
        tmp = inLow[i];
        if (tmp < lowest) {
          lowestIdx = i;
          lowest = tmp;
        }
      }
    } else if (tmp <= lowest) {
      lowestIdx = today;
      lowest = tmp;
    }
    /* Set the highest high */
    tmp = inHigh[today];
    if (highestIdx < trailingIdx) {
      highestIdx = trailingIdx;
      highest = inHigh[highestIdx];
      i = highestIdx;
      while (++i <= today) {
        tmp = inHigh[i];
        if (tmp > highest) {
          highestIdx = i;
          highest = tmp;
        }
      }
    } else if (tmp >= highest) {
      highestIdx = today;
      highest = tmp;
    }
    /* Fast-K, spelled as stochf.c spells it: divide by the range itself and
     * scale by 100.0 after, guarded by the very expression the division
     * uses, against ITS OWN two extremes rather than a fixed constant
     * (issue #253).
     *
     * Where STOCHF answers 0.0 on a flat window, PSO answers 50.0, the
     * Fast-K midpoint, so that a flat market reads PSO 0 instead of
     * -tanh(2.5) = -0.9866, a near-extreme oversold reading that nothing in
     * the window supports (#473 Q4, the neutral-point rule of #112).
     */
    if (!(Math.abs(highest - lowest) <= 0.00000000000001 * (Math.abs(highest) + Math.abs(lowest)))) {
      fastK = (inClose[today] - lowest) / (highest - lowest) * 100.0;
    } else {
      fastK = 50.0;
    }
    nsk = 0.1 * (fastK - 50.0);
    /* Pass 1, over the normalised Fast-K. */
    if (nBar < optInEMAPeriod) {
      sum1 = sum1 + nsk;
      if (nBar === optInEMAPeriod - 1) {
        ema1 = sum1 / optInEMAPeriod;
      }
    } else {
      {
        const _a: number = emaBeta, _b: number = ema1, _c: number = emaK * nsk;
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
    /* Pass 2, over what pass 1 publishes. Keep the comparison ahead of
     * the subtraction: the counters are unsigned in Rust.
     */
    if (nBar >= lookbackEMA) {
      n2 = nBar - lookbackEMA;
      if (n2 < optInEMAPeriod) {
        sum2 = sum2 + ema1;
        if (n2 === optInEMAPeriod - 1) {
          ema2 = sum2 / optInEMAPeriod;
        }
      } else {
        {
          const _a: number = emaBeta, _b: number = ema2, _c: number = emaK * ema1;
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
    }
    nBar = nBar + 1;
    trailingIdx = trailingIdx + 1;
    today = today + 1;
  }
  /* tanh(ss/2) rather than the published (e^ss - 1)/(e^ss + 1): the same
   * function, equal within 2.2e-16 on normal data, exactly odd and well
   * conditioned at zero. TA-Lib does not validate that a close lies inside
   * its bar, so ss is only bounded by [-5, 5] on well-formed input; the
   * literal form emits NaN from a successful call once ss exceeds 709.78,
   * which the house rule of #112 forbids (#473 Q3).
   */
  outReal[((0 * outStride) | 0)] = Math.tanh(0.5 * ema2);
  outIdx = 1;
  /* Stable zone. Both passes are pure recursions from here on. */
  while (today <= endIdx) {
    /* Set the lowest low */
    tmp = inLow[today];
    if (lowestIdx < trailingIdx) {
      lowestIdx = trailingIdx;
      lowest = inLow[lowestIdx];
      i = lowestIdx;
      while (++i <= today) {
        tmp = inLow[i];
        if (tmp < lowest) {
          lowestIdx = i;
          lowest = tmp;
        }
      }
    } else if (tmp <= lowest) {
      lowestIdx = today;
      lowest = tmp;
    }
    /* Set the highest high */
    tmp = inHigh[today];
    if (highestIdx < trailingIdx) {
      highestIdx = trailingIdx;
      highest = inHigh[highestIdx];
      i = highestIdx;
      while (++i <= today) {
        tmp = inHigh[i];
        if (tmp > highest) {
          highestIdx = i;
          highest = tmp;
        }
      }
    } else if (tmp >= highest) {
      highestIdx = today;
      highest = tmp;
    }
    if (!(Math.abs(highest - lowest) <= 0.00000000000001 * (Math.abs(highest) + Math.abs(lowest)))) {
      fastK = (inClose[today] - lowest) / (highest - lowest) * 100.0;
    } else {
      fastK = 50.0;
    }
    nsk = 0.1 * (fastK - 50.0);
    {
      const _a: number = emaBeta, _b: number = ema1, _c: number = emaK * nsk;
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
      const _a: number = emaBeta, _b: number = ema2, _c: number = emaK * ema1;
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
    outReal[((outIdx * outStride) | 0)] = Math.tanh(0.5 * ema2);
    outIdx = outIdx + 1;
    trailingIdx = trailingIdx + 1;
    today = today + 1;
  }
  outNBElement.value = outIdx;
  const capX = today - trailingIdx + 1;
  if (capX < 1 || capX > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  let physX = 1;
  while (physX < capX) {
    physX <<= 1;
  }
  const capX_inHigh = new Float64Array(physX);
  const capX_inLow = new Float64Array(physX);
  const capX_inClose = new Float64Array(physX);
  for (let fillJ = historyLen - capX; fillJ < historyLen; fillJ++) {
    capX_inHigh[fillJ & (physX - 1)] = inHigh[fillJ];
    capX_inLow[fillJ & (physX - 1)] = inLow[fillJ];
    capX_inClose[fillJ & (physX - 1)] = inClose[fillJ];
  }
  sp.optInFastK_Period = optInFastK_Period;
  sp.optInEMAPeriod = optInEMAPeriod;
  sp.emaK = emaK;
  sp.emaBeta = emaBeta;
  sp.highest = highest;
  sp.lowest = lowest;
  sp.ema1 = ema1;
  sp.ema2 = ema2;
  sp.trailingIdx = trailingIdx;
  sp.highestIdx = highestIdx;
  sp.lowestIdx = lowestIdx;
  sp.i = i;
  sp.today = today;
  sp.xMask = physX - 1;
  sp.x_inHigh = capX_inHigh;
  sp.x_inLow = capX_inLow;
  sp.x_inClose = capX_inClose;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function psoOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInFastK_Period: number,
  optInEMAPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): PsoStream {
  const sp = PsoStream.of(core);
  const retCode = psoOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInFastK_Period, optInEMAPeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('PSO openAndFill', inHigh.length, startIdx, psoLookbackPositional(optInFastK_Period, optInEMAPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && psoLookbackPositional(optInFastK_Period, optInEMAPeriod, core) < 0) {
    throw rejectedParams('PSO openAndFill', PARAM_RANGES, optInFastK_Period, optInEMAPeriod);
  }
  throw streamFailure('PSO openAndFill', retCode);
}

/** @internal */
export function psoOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInFastK_Period: number,
  optInEMAPeriod: number,
): PsoStream {
  const sp = PsoStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = psoOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInFastK_Period, optInEMAPeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('PSO open', inHigh.length, startIdx, psoLookbackPositional(optInFastK_Period, optInEMAPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && psoLookbackPositional(optInFastK_Period, optInEMAPeriod, core) < 0) {
    throw rejectedParams('PSO open', PARAM_RANGES, optInFastK_Period, optInEMAPeriod);
  }
  throw streamFailure('PSO open', retCode);
}

/** @internal */
export function psoOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInFastK_Period: number,
  optInEMAPeriod: number,
  core: Core = Core.DEFAULT,
): PsoStream {
  optInFastK_Period = requireInt32('PSO open', 'fastKPeriod', optInFastK_Period);
  optInEMAPeriod = requireInt32('PSO open', 'emaPeriod', optInEMAPeriod);
  requireArgument('PSO open', 'prices.high', inHigh);
  requireHistory('PSO open', 'prices.high', inHigh.length);
  requireArgument('PSO open', 'prices.low', inLow);
  requireArgument('PSO open', 'prices.close', inClose);
  requireHistoryLength('PSO open', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('PSO open', 'prices.close', inClose.length, inHigh.length);
  return psoOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInFastK_Period, optInEMAPeriod);
}

/** @internal */
export function psoOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInFastK_Period: number,
  optInEMAPeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): PsoStream {
  optInFastK_Period = requireInt32('PSO openAndFill', 'fastKPeriod', optInFastK_Period);
  optInEMAPeriod = requireInt32('PSO openAndFill', 'emaPeriod', optInEMAPeriod);
  requireArgument('PSO openAndFill', 'prices.high', inHigh);
  requireHistory('PSO openAndFill', 'prices.high', inHigh.length);
  requireArgument('PSO openAndFill', 'prices.low', inLow);
  requireArgument('PSO openAndFill', 'prices.close', inClose);
  const guardLookback = psoLookbackPositional(optInFastK_Period, optInEMAPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('PSO openAndFill', PARAM_RANGES, optInFastK_Period, optInEMAPeriod);
  }
  const guardOutLen = openFillCount('PSO openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('PSO openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('PSO openAndFill', 'prices.close', inClose.length, inHigh.length);
  requireOutput('PSO openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('PSO openAndFill', 'outReal', 'prices.high');
  }
  if (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('PSO openAndFill', 'outReal', 'prices.low');
  }
  if (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('PSO openAndFill', 'outReal', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outReal, inHighF64)) {
    throw sharedMemory('PSO openAndFill', 'outReal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outReal, inLowF64)) {
    throw sharedMemory('PSO openAndFill', 'outReal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outReal, inCloseF64)) {
    throw sharedMemory('PSO openAndFill', 'outReal', 'prices.close');
  }
  return psoOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInFastK_Period, optInEMAPeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live PSO stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link pso} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price series. `low`: Low price series. `close`: Close price series.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link PsoOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `psoLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or
 *   histories of different lengths.
 *
 * @example
 * ```ts
 * import { psoOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = psoOpen({ high, low, close }, { fastKPeriod: 8, emaPeriod: 5 });
 * const peeked = stream.peek(101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function psoOpen(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: PsoOptions): PsoStream {
  requireArgument('PSO open', 'prices', prices);
  const o = optionsOf('PSO open', options);
  const optInFastK_Period = orDefault(o.fastKPeriod, Core.INTEGER_DEFAULT);
  const optInEMAPeriod = orDefault(o.emaPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('PSO open', o.core);
  return psoOpenPositional(prices.high, prices.low, prices.close, optInFastK_Period, optInEMAPeriod, core);
}

/**
 * {@link psoOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link pso}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price series. `low`: Low price series. `close`: Close price series.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link PsoOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link pso} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `psoLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { psoOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = psoOpenAndFill({ high, low, close }, {
 *   fastKPeriod: 8,
 *   emaPeriod: 5,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function psoOpenAndFill(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: PsoOptions): PsoResult & { stream: PsoStream } {
  requireArgument('PSO openAndFill', 'prices', prices);
  const o = optionsOf('PSO openAndFill', options);
  const optInFastK_Period = orDefault(o.fastKPeriod, Core.INTEGER_DEFAULT);
  const optInEMAPeriod = orDefault(o.emaPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('PSO openAndFill', o.core);
  const capacity = fillCapacity(psoLookbackPositional(optInFastK_Period, optInEMAPeriod, core), prices.high, prices.low, prices.close);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = psoOpenAndFillPositional(prices.high, prices.low, prices.close, optInFastK_Period, optInEMAPeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
