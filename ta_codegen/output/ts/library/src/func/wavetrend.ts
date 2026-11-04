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
 *  100626 KL,CC  Initial version (#476).
 *  101126 MF,CC  The EMA betas stay as the divide wrote them above a
 *                period of 2. The fixpoint test waits for a seeded
 *                average.
 */

import { badSink, clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { emaLookback } from './ema.ts';
import { smaLookback } from './sma.ts';

/**
 * Number of leading input bars {@link wavetrend} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param optInChannelPeriod - Period of the price channel, used by both the
 *   average and the deviation (default 10; range 2 to 100000; `Core.INTEGER_DEFAULT`
 *   selects the default).
 * @param optInAveragePeriod - Smoothing for the oscillator line (default 21; range
 *   1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSignalPeriod - Period of the simple average making the signal line
 *   (default 4; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function wavetrendLookback(optInChannelPeriod: number, optInAveragePeriod: number, optInSignalPeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInChannelPeriod | 0) !== optInChannelPeriod) {
    return -1;
  }
  if ((optInAveragePeriod | 0) !== optInAveragePeriod) {
    return -1;
  }
  if ((optInSignalPeriod | 0) !== optInSignalPeriod) {
    return -1;
  }
  if (optInChannelPeriod === Core.INTEGER_DEFAULT) {
    optInChannelPeriod = 10;
  } else if (optInChannelPeriod < 2 || optInChannelPeriod > 100000) {
    return -1;
  }
  if (optInAveragePeriod === Core.INTEGER_DEFAULT) {
    optInAveragePeriod = 21;
  } else if (optInAveragePeriod < 1 || optInAveragePeriod > 100000) {
    return -1;
  }
  if (optInSignalPeriod === Core.INTEGER_DEFAULT) {
    optInSignalPeriod = 4;
  } else if (optInSignalPeriod < 1 || optInSignalPeriod > 100000) {
    return -1;
  }
  /* Keep every term a callee's own lookback: that is how WAVETREND inherits
   * TA_FUNC_UNST_EMA, once per exponential stage.
   */
  return emaLookback(optInChannelPeriod, core) + emaLookback(optInChannelPeriod, core) + emaLookback(optInAveragePeriod, core) + smaLookback(optInSignalPeriod, core);
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link wavetrend}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInChannelPeriod - Period of the price channel, used by both the
 *   average and the deviation (default 10; range 2 to 100000; `Core.INTEGER_DEFAULT`
 *   selects the default).
 * @param optInAveragePeriod - Smoothing for the oscillator line (default 21; range
 *   1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSignalPeriod - Period of the simple average making the signal line
 *   (default 4; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function wavetrendDisplayShift(optInChannelPeriod: number, optInAveragePeriod: number, optInSignalPeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (wavetrendLookback(optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 2) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function wavetrendImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  optInChannelPeriod: number,
  optInAveragePeriod: number,
  optInSignalPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outWT1: Float64Array,
  outWT2: Float64Array,
): RetCode {
  let k1 = 0;
  let beta1 = 0;
  let k2 = 0;
  let beta2 = 0;
  let ap = 0;
  let esa = 0;
  let dev = 0;
  let d = 0;
  let ci = 0;
  let wt1 = 0;
  let prevAp = 0;
  let prevEsa = 0;
  let num = 0;
  let scaledDev = 0;
  let sumEsa = 0;
  let sumD = 0;
  let sumCi = 0;
  let sumSignal = 0;
  let lookbackTotal = 0;
  let lookbackChannel = 0;
  let lookbackAverage = 0;
  let today = 0;
  let outIdx = 0;
  let nAp = 0;
  let nDev = 0;
  let nCi = 0;
  let wtBuffer!: Float64Array;
  let wtBuffer_Idx = 0;
  let maxIdx_wtBuffer = (32) - 1;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInChannelPeriod === Core.INTEGER_DEFAULT) {
    optInChannelPeriod = 10;
  } else if (optInChannelPeriod < 2 || optInChannelPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInAveragePeriod === Core.INTEGER_DEFAULT) {
    optInAveragePeriod = 21;
  } else if (optInAveragePeriod < 1 || optInAveragePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSignalPeriod === Core.INTEGER_DEFAULT) {
    optInSignalPeriod = 4;
  } else if (optInSignalPeriod < 1 || optInSignalPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if ((outWT1.buffer === outWT2.buffer && outWT1.byteOffset === outWT2.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  /* LazyBear's WaveTrend Oscillator (TradingView, 2014), lines 14 to 21: the
   * typical price's distance from its own exponential average, normalised by
   * an exponential average of that distance's absolute value and by Lambert's
   * CCI constant, then smoothed.
   *
   *    ap  = (H + L + C)/3
   *    esa = EMA(ap, n1)
   *    d   = EMA(|ap - esa|, n1)
   *    ci  = (ap - esa) / (0.015 * d)
   *    WT1 = EMA(ci, n2)        WT2 = SMA(WT1, n3)
   *
   * The middle stage is an exponential CCI, not TA_CCI: cci.c averages with
   * an SMA and takes the mean deviation around that window's own SMA, where
   * this uses two exponential averages.
   *
   * Bit-identical to the composed chain on moving data: each stage seeds as
   * ema.c and sma.c do, and each stage boundary is the callee's LOOKBACK,
   * never (period-1), or an EMA unstable period breaks it.
   */
  /* This ptr will point on a circular buffer of at least
   * "optInSignalPeriod" element.
   */
  lookbackTotal = wavetrendLookback(optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, core);
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
  wtBuffer = new Float64Array(optInSignalPeriod);
  maxIdx_wtBuffer = optInSignalPeriod - 1;
  wtBuffer_Idx = 0;
  lookbackChannel = emaLookback(optInChannelPeriod, core);
  lookbackAverage = emaLookback(optInAveragePeriod, core);
  /* k and beta must sum to exactly 1.0, or a flat input drifts off its
   * level. Each subtraction is exact only from an operand in [0.5,1): at a
   * period of 2 or less that is k, above it beta. Above it beta stays as the
   * divide wrote it: a register last written by a subtraction costs each FMA
   * reading it one more cycle on Intel P-cores.
   */
  beta1 = (optInChannelPeriod - 1) / (optInChannelPeriod + 1);
  k1 = 1.0 - beta1;
  if (beta1 < 0.5) {
    beta1 = 1.0 - k1;
  }
  beta2 = (optInAveragePeriod - 1) / (optInAveragePeriod + 1);
  k2 = 1.0 - beta2;
  if (beta2 < 0.5) {
    beta2 = 1.0 - k2;
  }
  esa = 0.0;
  d = 0.0;
  ci = 0.0;
  wt1 = 0.0;
  sumEsa = 0.0;
  sumD = 0.0;
  sumCi = 0.0;
  sumSignal = 0.0;
  nAp = 0;
  prevAp = 0.0;
  prevEsa = 0.0;
  today = startIdx - lookbackTotal;
  /* Warm-up. Runs through startIdx inclusive: the last pass is the one that
   * completes the signal window, so it leaves the first output in the state.
   */
  while (today <= startIdx) {
    ap = (inHigh[today] + inLow[today] + inClose[today]) / 3.0;
    /* Stage 1: the exponential average of the typical price. */
    if (nAp < optInChannelPeriod) {
      sumEsa = sumEsa + ap;
      if (nAp === optInChannelPeriod - 1) {
        esa = sumEsa / optInChannelPeriod;
      }
    } else {
      {
        const _a: number = beta1, _b: number = esa, _c: number = k1 * ap;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        esa = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
    }
    /* Stage 2: the exponential average of the absolute distance, over what
     * stage 1 publishes. The counter is compared before it is subtracted,
     * never after: the Rust backend renders these as usize.
     */
    if (nAp >= lookbackChannel) {
      nDev = nAp - lookbackChannel;
      /* GUARD 1, the fixpoint test. With a constant input the exponential
       * step stops moving once k*|ap - esa| falls under half an ulp, and
       * esa then FREEZES up to (n1+1)/4 ulps away from the price. That
       * frozen residue is a real non-zero distance, so the naive form
       * divides it by its own exponential average and walks to
       * +/-1/0.015 = +/-66.67 -- an extreme reading produced by nothing
       * but rounding. When the pair has not moved, the average has reached
       * its fixpoint and the distance is exactly that residue, so the
       * numerator is taken as zero. The test is exact, so it is
       * independent of scale and period.
       *
       * Only once prevEsa is an average: on the bar that seeds esa it is
       * still the 0.0 placeholder, which a seed of exactly 0.0 equals.
       */
      num = ap - esa;
      if (nAp >= optInChannelPeriod && ap === prevAp && esa === prevEsa) {
        num = 0.0;
      }
      dev = Math.abs(num);
      if (nDev < optInChannelPeriod) {
        sumD = sumD + dev;
        if (nDev === optInChannelPeriod - 1) {
          d = sumD / optInChannelPeriod;
        }
      } else {
        {
          const _a: number = beta1, _b: number = d, _c: number = k1 * dev;
          const _p = _a * _b;
          let _t = 134217729 * _a;
          const _ah = _t - (_t - _a), _al = _a - _ah;
          _t = 134217729 * _b;
          const _bh = _t - (_t - _b), _bl = _b - _bh;
          const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
          const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
          const _v = _tl + _pl, _v2 = _v + _v;
          const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
          d = _w && (_pl === 0 || _v === 0) ? _th
            : _w && _v2 !== (_th + _v2) - _th ? _th + _v
            : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
        }
      }
      /* Stage 3: the oscillator, then its own smoothing. */
      if (nDev >= lookbackChannel) {
        nCi = nDev - lookbackChannel;
        /* GUARD 2, the exact divisor. Test the PRODUCT the division uses,
         * not the deviation: 0.015*d underflows to zero while d is still
         * non-zero (#395). A zero divisor is 0/0 -- no distance against no
         * average distance -- so the oscillator reads its neutral 0.0
         * (#112), as tsi.c does.
         */
        scaledDev = 0.015 * d;
        if (scaledDev > 0.0) {
          ci = num / scaledDev;
        } else {
          ci = 0.0;
        }
        if (nCi < optInAveragePeriod) {
          sumCi = sumCi + ci;
          if (nCi === optInAveragePeriod - 1) {
            wt1 = sumCi / optInAveragePeriod;
          }
        } else {
          {
            const _a: number = beta2, _b: number = wt1, _c: number = k2 * ci;
            const _p = _a * _b;
            let _t = 134217729 * _a;
            const _ah = _t - (_t - _a), _al = _a - _ah;
            _t = 134217729 * _b;
            const _bh = _t - (_t - _b), _bl = _b - _bh;
            const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
            const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
            const _v = _tl + _pl, _v2 = _v + _v;
            const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
            wt1 = _w && (_pl === 0 || _v === 0) ? _th
              : _w && _v2 !== (_th + _v2) - _th ? _th + _v
              : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
          }
        }
        if (nCi >= lookbackAverage) {
          wtBuffer[wtBuffer_Idx] = wt1;
          sumSignal = sumSignal + wt1;
          wtBuffer_Idx++;
          if (wtBuffer_Idx > maxIdx_wtBuffer) {
            wtBuffer_Idx = 0;
          }
        }
      }
    }
    prevAp = ap;
    prevEsa = esa;
    nAp = nAp + 1;
    today = today + 1;
  }
  /* The first output. The warm-up's last pass stored this bar's WT1 into the
   * ring and added it to the running sum, so the average is ready here.
   */
  outWT1[0] = wt1;
  outWT2[0] = sumSignal / optInSignalPeriod;
  outIdx = 1;
  sumSignal = sumSignal - wtBuffer[wtBuffer_Idx];
  /* Stable zone. One bar past the first output every stage is past its seed,
   * at every reachable parameter triple: the shortest case, 2/1/1, arrives
   * with the deviation average and the oscillator smoothing both one bar
   * into their recursions. So nothing here branches on a counter, and the
   * stores always run -- which is what keeps the managed backends' peek
   * frames from carrying a seeded output local.
   */
  while (today <= endIdx) {
    ap = (inHigh[today] + inLow[today] + inClose[today]) / 3.0;
    {
      const _a: number = beta1, _b: number = esa, _c: number = k1 * ap;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      esa = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    num = ap - esa;
    if (ap === prevAp && esa === prevEsa) {
      num = 0.0;
    }
    dev = Math.abs(num);
    {
      const _a: number = beta1, _b: number = d, _c: number = k1 * dev;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      d = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    scaledDev = 0.015 * d;
    if (scaledDev > 0.0) {
      ci = num / scaledDev;
    } else {
      ci = 0.0;
    }
    {
      const _a: number = beta2, _b: number = wt1, _c: number = k2 * ci;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      wt1 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    wtBuffer[wtBuffer_Idx] = wt1;
    sumSignal = sumSignal + wt1;
    outWT1[outIdx] = wt1;
    outWT2[outIdx] = sumSignal / optInSignalPeriod;
    outIdx = outIdx + 1;
    wtBuffer_Idx++;
    if (wtBuffer_Idx > maxIdx_wtBuffer) {
      wtBuffer_Idx = 0;
    }
    sumSignal = sumSignal - wtBuffer[wtBuffer_Idx];
    prevAp = ap;
    prevEsa = esa;
    today = today + 1;
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/**
 * WaveTrend Oscillator: how far the typical price sits from its own exponential
 * average, divided by an exponential average of that distance and by Lambert's CCI
 * constant, then smoothed. The reading is the plain stochastic's complaint
 * answered a different way — rather than bounding the oscillator by construction,
 * it scales it by how far price has recently been travelling, so the same numeric
 * level means the same thing in a quiet market and a fast one. The oscillator line
 * and its short simple average cross; the crossings that matter are the ones
 * beyond the extremes, which the author draws at ±53 and ±60.
 *
 * Formula and more info at
 * [ta-lib.org/functions/wavetrend](https://ta-lib.org/functions/wavetrend).
 *
 * **Notes**
 * - The middle stage is an exponential CCI, not `TA_CCI`: CCI averages with a
 *   simple moving average and takes the mean deviation around that window's own
 *   average, where this uses two exponential averages.
 * - On a market that has stopped moving, the exponential average stops moving too
 *   — once its step falls under half an ulp it freezes, a few ulps away from the
 *   price. That frozen gap is a real non-zero distance, so dividing by its own
 *   average walks the oscillator to ±66.67, an extreme reading produced by nothing
 *   but rounding. This answers 0 instead, by taking the distance as zero whenever
 *   the price and its average both repeat. Implementations without that test drift
 *   to the extreme on flat data, at a bar that depends on their arithmetic.
 * - A zero divisor is tested on the scaled deviation, the quantity the division
 *   actually uses, and answers the neutral 0 rather than dividing.
 * - Each exponential average is seeded with a simple average of its own first
 *   inputs, the same seeding TA-Lib's EMA uses, and each seeds on what the stage
 *   before it publishes. `TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)` discards
 *   more of that warm-up, and it counts three times because there are three
 *   exponential stages.
 * - The difference the author also plots is `TA_SUB(outWT1, outWT2)`.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link wavetrendLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price series.
 * @param inLow - Low price series.
 * @param inClose - Close price series.
 * @param optInChannelPeriod - Period of the price channel, used by both the
 *   average and the deviation (default 10; range 2 to 100000; `Core.INTEGER_DEFAULT`
 *   selects the default).
 * @param optInAveragePeriod - Smoothing for the oscillator line (default 21; range
 *   1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSignalPeriod - Period of the simple average making the signal line
 *   (default 4; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outWT1 - WaveTrend oscillator line. Must hold the values produced and
 *   never be empty; the input length always suffices.
 * @param outWT2 - Simple average of the oscillator line. Must hold the values
 *   produced and never be empty; the input length always suffices.
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
 * import { wavetrend } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const wt1 = new Float64Array(high.length);
 * const wt2 = new Float64Array(high.length);
 * const { begIdx, count } = wavetrend(
 *   0, high.length - 1,
 *   high, low, close,
 *   10, 21, 4,
 *   wt1, wt2,
 * );
 * // wt1[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`cci`](https://ta-lib.org/functions/cci)
 * @see [`smi`](https://ta-lib.org/functions/smi)
 * @see [`stochrsi`](https://ta-lib.org/functions/stochrsi)
 * @see [`tsi`](https://ta-lib.org/functions/tsi)
 */
export function wavetrend(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInChannelPeriod: number,
  optInAveragePeriod: number,
  optInSignalPeriod: number,
  outWT1: Float64Array,
  outWT2: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('WAVETREND', 'startIdx', startIdx);
  endIdx = requireInt32('WAVETREND', 'endIdx', endIdx);
  optInChannelPeriod = requireInt32('WAVETREND', 'optInChannelPeriod', optInChannelPeriod);
  optInAveragePeriod = requireInt32('WAVETREND', 'optInAveragePeriod', optInAveragePeriod);
  optInSignalPeriod = requireInt32('WAVETREND', 'optInSignalPeriod', optInSignalPeriod);
  requireIndexRange('WAVETREND', startIdx, endIdx);
  const guardStart = clampedStart('WAVETREND', startIdx, wavetrendLookback(optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('WAVETREND', 'inHigh', inHigh, guardInLen);
  requireLength('WAVETREND', 'inLow', inLow, guardInLen);
  requireLength('WAVETREND', 'inClose', inClose, guardInLen);
  requireOutput('WAVETREND', 'outWT1', outWT1, guardOutLen, Float64Array);
  requireOutput('WAVETREND', 'outWT2', outWT2, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outWT1, inHighF64) || overlapsPartly(outWT1, inLowF64) || overlapsPartly(outWT1, inCloseF64) || overlapsPartly(outWT2, inHighF64) || overlapsPartly(outWT2, inLowF64) || overlapsPartly(outWT2, inCloseF64) || overlaps(outWT1, outWT2)) {
    throw failure('WAVETREND', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = wavetrendImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, outBegIdx, outNBElement, outWT1, outWT2);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('WAVETREND', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live WAVETREND stream: one value per closed bar, bit-identical to
 * {@link wavetrend} over the same series. Open it with {@link wavetrendOpen} or
 * {@link wavetrendOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/wavetrend](https://ta-lib.org/functions/wavetrend).
 */
export class WavetrendStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInChannelPeriod: number = 0;
  /** @internal */
  optInAveragePeriod: number = 0;
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
  esa: number = 0;
  /** @internal */
  d: number = 0;
  /** @internal */
  wt1: number = 0;
  /** @internal */
  prevAp: number = 0;
  /** @internal */
  prevEsa: number = 0;
  /** @internal */
  sumSignal: number = 0;
  /** @internal */
  wtBuffer_Idx: number = 0;
  /** @internal */
  maxIdx_wtBuffer: number = 0;
  /** @internal */
  cbSize_wtBuffer: number = 0;
  /** @internal */
  cb_wtBuffer: Float64Array = new Float64Array(1);
  /** @internal */
  cur_outWT1: number = 0;
  /** @internal */
  cur_outWT2: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): WavetrendStream {
    return new WavetrendStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link wavetrend} reports them over the same bars. Each accepted `update` and
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
      throw failure('WAVETREND advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar, writing the new values into `out`.
   *
   * @param inHigh - The bar's `inHigh` value: High price series.
   * @param inLow - The bar's `inLow` value: Low price series.
   * @param inClose - The bar's `inClose` value: Close price series.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number, inClose: number, out: WavetrendOut): void {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('WAVETREND update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (typeof out !== 'object' || out === null) {
      throw badSink('WAVETREND update', 'WavetrendOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('WAVETREND update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    wavetrendStepImpl(this, inHigh, inLow, inClose);
    this.outRangeCount++;
    out.wT1 = this.cur_outWT1;
    out.wT2 = this.cur_outWT2;
  }

  /**
   * Writes into `out` what the next `update` with the same bar would write,
   * committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price series.
   * @param inLow - The bar's `inLow` value: Low price series.
   * @param inClose - The bar's `inClose` value: Close price series.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number, inClose: number, out: WavetrendOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('WAVETREND peek', 'WavetrendOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('WAVETREND peek', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let ap = 0;
    let dev = 0;
    let ci = 0;
    let num = 0;
    let scaledDev = 0;
    let cur_outWT1 = 0;
    let cur_outWT2 = 0;
    let d = sp.d;
    let esa = sp.esa;
    let sumSignal = sp.sumSignal;
    let wt1 = sp.wt1;
    ap = (inHigh + inLow + inClose) / 3.0;
    {
      const _a: number = sp.beta1, _b: number = esa, _c: number = sp.k1 * ap;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      esa = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    num = ap - esa;
    if (ap === sp.prevAp && esa === sp.prevEsa) {
      num = 0.0;
    }
    dev = Math.abs(num);
    {
      const _a: number = sp.beta1, _b: number = d, _c: number = sp.k1 * dev;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      d = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    scaledDev = 0.015 * d;
    if (scaledDev > 0.0) {
      ci = num / scaledDev;
    } else {
      ci = 0.0;
    }
    {
      const _a: number = sp.beta2, _b: number = wt1, _c: number = sp.k2 * ci;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      wt1 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    sumSignal = sumSignal + wt1;
    cur_outWT1 = wt1;
    cur_outWT2 = sumSignal / sp.optInSignalPeriod;
    out.wT1 = cur_outWT1;
    out.wT2 = cur_outWT2;
  }

  /**
   * The value at the last bar this stream counted; `peek` does not change it.
   *
   * @param out - Receives the outputs.
   */
  value(out: WavetrendOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('WAVETREND value', 'WavetrendOut');
    }
    out.wT1 = this.cur_outWT1;
    out.wT2 = this.cur_outWT2;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): WavetrendStream {
    const c = new WavetrendStream(this.core);
    c.optInChannelPeriod = this.optInChannelPeriod;
    c.optInAveragePeriod = this.optInAveragePeriod;
    c.optInSignalPeriod = this.optInSignalPeriod;
    c.k1 = this.k1;
    c.beta1 = this.beta1;
    c.k2 = this.k2;
    c.beta2 = this.beta2;
    c.esa = this.esa;
    c.d = this.d;
    c.wt1 = this.wt1;
    c.prevAp = this.prevAp;
    c.prevEsa = this.prevEsa;
    c.sumSignal = this.sumSignal;
    c.wtBuffer_Idx = this.wtBuffer_Idx;
    c.maxIdx_wtBuffer = this.maxIdx_wtBuffer;
    c.cbSize_wtBuffer = this.cbSize_wtBuffer;
    c.cb_wtBuffer = this.cb_wtBuffer.slice();
    c.cur_outWT1 = this.cur_outWT1;
    c.cur_outWT2 = this.cur_outWT2;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one WAVETREND bar, written into an object the caller owns and
 * reuses: valid until the next call that writes it.
 */
export class WavetrendOut {
  /** WaveTrend oscillator line. */
  wT1: number = 0;
  /** Simple average of the oscillator line. */
  wT2: number = 0;
}

function wavetrendStepImpl(sp: WavetrendStream, inHigh: number, inLow: number, inClose: number): void {
  let ap = 0;
  let dev = 0;
  let ci = 0;
  let num = 0;
  let scaledDev = 0;
  ap = (inHigh + inLow + inClose) / 3.0;
  {
    const _a: number = sp.beta1, _b: number = sp.esa, _c: number = sp.k1 * ap;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.esa = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  num = ap - sp.esa;
  if (ap === sp.prevAp && sp.esa === sp.prevEsa) {
    num = 0.0;
  }
  dev = Math.abs(num);
  {
    const _a: number = sp.beta1, _b: number = sp.d, _c: number = sp.k1 * dev;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.d = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  scaledDev = 0.015 * sp.d;
  if (scaledDev > 0.0) {
    ci = num / scaledDev;
  } else {
    ci = 0.0;
  }
  {
    const _a: number = sp.beta2, _b: number = sp.wt1, _c: number = sp.k2 * ci;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.wt1 = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  sp.cb_wtBuffer[sp.wtBuffer_Idx] = sp.wt1;
  sp.sumSignal = sp.sumSignal + sp.wt1;
  sp.cur_outWT1 = sp.wt1;
  sp.cur_outWT2 = sp.sumSignal / sp.optInSignalPeriod;
  sp.wtBuffer_Idx = sp.wtBuffer_Idx + 1;
  if (sp.wtBuffer_Idx > sp.maxIdx_wtBuffer) {
    sp.wtBuffer_Idx = 0;
  }
  sp.sumSignal = sp.sumSignal - sp.cb_wtBuffer[sp.wtBuffer_Idx];
  sp.prevAp = ap;
  sp.prevEsa = sp.esa;
}

function wavetrendOpenImpl(
  sp: WavetrendStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInChannelPeriod: number,
  optInAveragePeriod: number,
  optInSignalPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outWT1: Float64Array,
  outWT2: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let k1 = 0;
  let beta1 = 0;
  let k2 = 0;
  let beta2 = 0;
  let ap = 0;
  let esa = 0;
  let dev = 0;
  let d = 0;
  let ci = 0;
  let wt1 = 0;
  let prevAp = 0;
  let prevEsa = 0;
  let num = 0;
  let scaledDev = 0;
  let sumEsa = 0;
  let sumD = 0;
  let sumCi = 0;
  let sumSignal = 0;
  let lookbackTotal = 0;
  let lookbackChannel = 0;
  let lookbackAverage = 0;
  let today = 0;
  let outIdx = 0;
  let nAp = 0;
  let nDev = 0;
  let nCi = 0;
  let wtBuffer!: Float64Array;
  let wtBuffer_Idx = 0;
  let maxIdx_wtBuffer = (32) - 1;
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
  if (optInChannelPeriod === Core.INTEGER_DEFAULT) {
    optInChannelPeriod = 10;
  } else if (optInChannelPeriod < 2 || optInChannelPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInAveragePeriod === Core.INTEGER_DEFAULT) {
    optInAveragePeriod = 21;
  } else if (optInAveragePeriod < 1 || optInAveragePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSignalPeriod === Core.INTEGER_DEFAULT) {
    optInSignalPeriod = 4;
  } else if (optInSignalPeriod < 1 || optInSignalPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* LazyBear's WaveTrend Oscillator (TradingView, 2014), lines 14 to 21: the
   * typical price's distance from its own exponential average, normalised by
   * an exponential average of that distance's absolute value and by Lambert's
   * CCI constant, then smoothed.
   *
   *    ap  = (H + L + C)/3
   *    esa = EMA(ap, n1)
   *    d   = EMA(|ap - esa|, n1)
   *    ci  = (ap - esa) / (0.015 * d)
   *    WT1 = EMA(ci, n2)        WT2 = SMA(WT1, n3)
   *
   * The middle stage is an exponential CCI, not TA_CCI: cci.c averages with
   * an SMA and takes the mean deviation around that window's own SMA, where
   * this uses two exponential averages.
   *
   * Bit-identical to the composed chain on moving data: each stage seeds as
   * ema.c and sma.c do, and each stage boundary is the callee's LOOKBACK,
   * never (period-1), or an EMA unstable period breaks it.
   */
  /* This ptr will point on a circular buffer of at least
   * "optInSignalPeriod" element.
   */
  lookbackTotal = wavetrendLookback(optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, core);
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
  wtBuffer = new Float64Array(optInSignalPeriod);
  maxIdx_wtBuffer = optInSignalPeriod - 1;
  wtBuffer_Idx = 0;
  lookbackChannel = emaLookback(optInChannelPeriod, core);
  lookbackAverage = emaLookback(optInAveragePeriod, core);
  /* k and beta must sum to exactly 1.0, or a flat input drifts off its
   * level. Each subtraction is exact only from an operand in [0.5,1): at a
   * period of 2 or less that is k, above it beta. Above it beta stays as the
   * divide wrote it: a register last written by a subtraction costs each FMA
   * reading it one more cycle on Intel P-cores.
   */
  beta1 = (optInChannelPeriod - 1) / (optInChannelPeriod + 1);
  k1 = 1.0 - beta1;
  if (beta1 < 0.5) {
    beta1 = 1.0 - k1;
  }
  beta2 = (optInAveragePeriod - 1) / (optInAveragePeriod + 1);
  k2 = 1.0 - beta2;
  if (beta2 < 0.5) {
    beta2 = 1.0 - k2;
  }
  esa = 0.0;
  d = 0.0;
  ci = 0.0;
  wt1 = 0.0;
  sumEsa = 0.0;
  sumD = 0.0;
  sumCi = 0.0;
  sumSignal = 0.0;
  nAp = 0;
  prevAp = 0.0;
  prevEsa = 0.0;
  today = startIdx - lookbackTotal;
  /* Warm-up. Runs through startIdx inclusive: the last pass is the one that
   * completes the signal window, so it leaves the first output in the state.
   */
  while (today <= startIdx) {
    ap = (inHigh[today] + inLow[today] + inClose[today]) / 3.0;
    /* Stage 1: the exponential average of the typical price. */
    if (nAp < optInChannelPeriod) {
      sumEsa = sumEsa + ap;
      if (nAp === optInChannelPeriod - 1) {
        esa = sumEsa / optInChannelPeriod;
      }
    } else {
      {
        const _a: number = beta1, _b: number = esa, _c: number = k1 * ap;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        esa = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
    }
    /* Stage 2: the exponential average of the absolute distance, over what
     * stage 1 publishes. The counter is compared before it is subtracted,
     * never after: the Rust backend renders these as usize.
     */
    if (nAp >= lookbackChannel) {
      nDev = nAp - lookbackChannel;
      /* GUARD 1, the fixpoint test. With a constant input the exponential
       * step stops moving once k*|ap - esa| falls under half an ulp, and
       * esa then FREEZES up to (n1+1)/4 ulps away from the price. That
       * frozen residue is a real non-zero distance, so the naive form
       * divides it by its own exponential average and walks to
       * +/-1/0.015 = +/-66.67 -- an extreme reading produced by nothing
       * but rounding. When the pair has not moved, the average has reached
       * its fixpoint and the distance is exactly that residue, so the
       * numerator is taken as zero. The test is exact, so it is
       * independent of scale and period.
       *
       * Only once prevEsa is an average: on the bar that seeds esa it is
       * still the 0.0 placeholder, which a seed of exactly 0.0 equals.
       */
      num = ap - esa;
      if (nAp >= optInChannelPeriod && ap === prevAp && esa === prevEsa) {
        num = 0.0;
      }
      dev = Math.abs(num);
      if (nDev < optInChannelPeriod) {
        sumD = sumD + dev;
        if (nDev === optInChannelPeriod - 1) {
          d = sumD / optInChannelPeriod;
        }
      } else {
        {
          const _a: number = beta1, _b: number = d, _c: number = k1 * dev;
          const _p = _a * _b;
          let _t = 134217729 * _a;
          const _ah = _t - (_t - _a), _al = _a - _ah;
          _t = 134217729 * _b;
          const _bh = _t - (_t - _b), _bl = _b - _bh;
          const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
          const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
          const _v = _tl + _pl, _v2 = _v + _v;
          const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
          d = _w && (_pl === 0 || _v === 0) ? _th
            : _w && _v2 !== (_th + _v2) - _th ? _th + _v
            : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
        }
      }
      /* Stage 3: the oscillator, then its own smoothing. */
      if (nDev >= lookbackChannel) {
        nCi = nDev - lookbackChannel;
        /* GUARD 2, the exact divisor. Test the PRODUCT the division uses,
         * not the deviation: 0.015*d underflows to zero while d is still
         * non-zero (#395). A zero divisor is 0/0 -- no distance against no
         * average distance -- so the oscillator reads its neutral 0.0
         * (#112), as tsi.c does.
         */
        scaledDev = 0.015 * d;
        if (scaledDev > 0.0) {
          ci = num / scaledDev;
        } else {
          ci = 0.0;
        }
        if (nCi < optInAveragePeriod) {
          sumCi = sumCi + ci;
          if (nCi === optInAveragePeriod - 1) {
            wt1 = sumCi / optInAveragePeriod;
          }
        } else {
          {
            const _a: number = beta2, _b: number = wt1, _c: number = k2 * ci;
            const _p = _a * _b;
            let _t = 134217729 * _a;
            const _ah = _t - (_t - _a), _al = _a - _ah;
            _t = 134217729 * _b;
            const _bh = _t - (_t - _b), _bl = _b - _bh;
            const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
            const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
            const _v = _tl + _pl, _v2 = _v + _v;
            const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
            wt1 = _w && (_pl === 0 || _v === 0) ? _th
              : _w && _v2 !== (_th + _v2) - _th ? _th + _v
              : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
          }
        }
        if (nCi >= lookbackAverage) {
          wtBuffer[wtBuffer_Idx] = wt1;
          sumSignal = sumSignal + wt1;
          wtBuffer_Idx++;
          if (wtBuffer_Idx > maxIdx_wtBuffer) {
            wtBuffer_Idx = 0;
          }
        }
      }
    }
    prevAp = ap;
    prevEsa = esa;
    nAp = nAp + 1;
    today = today + 1;
  }
  /* The first output. The warm-up's last pass stored this bar's WT1 into the
   * ring and added it to the running sum, so the average is ready here.
   */
  outWT1[((0 * outStride) | 0)] = wt1;
  outWT2[((0 * outStride) | 0)] = sumSignal / optInSignalPeriod;
  outIdx = 1;
  sumSignal = sumSignal - wtBuffer[wtBuffer_Idx];
  /* Stable zone. One bar past the first output every stage is past its seed,
   * at every reachable parameter triple: the shortest case, 2/1/1, arrives
   * with the deviation average and the oscillator smoothing both one bar
   * into their recursions. So nothing here branches on a counter, and the
   * stores always run -- which is what keeps the managed backends' peek
   * frames from carrying a seeded output local.
   */
  while (today <= endIdx) {
    ap = (inHigh[today] + inLow[today] + inClose[today]) / 3.0;
    {
      const _a: number = beta1, _b: number = esa, _c: number = k1 * ap;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      esa = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    num = ap - esa;
    if (ap === prevAp && esa === prevEsa) {
      num = 0.0;
    }
    dev = Math.abs(num);
    {
      const _a: number = beta1, _b: number = d, _c: number = k1 * dev;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      d = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    scaledDev = 0.015 * d;
    if (scaledDev > 0.0) {
      ci = num / scaledDev;
    } else {
      ci = 0.0;
    }
    {
      const _a: number = beta2, _b: number = wt1, _c: number = k2 * ci;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      wt1 = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    wtBuffer[wtBuffer_Idx] = wt1;
    sumSignal = sumSignal + wt1;
    outWT1[((outIdx * outStride) | 0)] = wt1;
    outWT2[((outIdx * outStride) | 0)] = sumSignal / optInSignalPeriod;
    outIdx = outIdx + 1;
    wtBuffer_Idx++;
    if (wtBuffer_Idx > maxIdx_wtBuffer) {
      wtBuffer_Idx = 0;
    }
    sumSignal = sumSignal - wtBuffer[wtBuffer_Idx];
    prevAp = ap;
    prevEsa = esa;
    today = today + 1;
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const capCb_wtBuffer = maxIdx_wtBuffer + 1;
  if (capCb_wtBuffer > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sp.optInChannelPeriod = optInChannelPeriod;
  sp.optInAveragePeriod = optInAveragePeriod;
  sp.optInSignalPeriod = optInSignalPeriod;
  sp.k1 = k1;
  sp.beta1 = beta1;
  sp.k2 = k2;
  sp.beta2 = beta2;
  sp.esa = esa;
  sp.d = d;
  sp.wt1 = wt1;
  sp.prevAp = prevAp;
  sp.prevEsa = prevEsa;
  sp.sumSignal = sumSignal;
  sp.wtBuffer_Idx = wtBuffer_Idx;
  sp.maxIdx_wtBuffer = maxIdx_wtBuffer;
  sp.cbSize_wtBuffer = capCb_wtBuffer;
  sp.cb_wtBuffer = wtBuffer;
  sp.cur_outWT1 = outWT1[(outNBElement.value - 1) * outStride];
  sp.cur_outWT2 = outWT2[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function wavetrendOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInChannelPeriod: number,
  optInAveragePeriod: number,
  optInSignalPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outWT1: Float64Array,
  outWT2: Float64Array,
): WavetrendStream {
  const sp = WavetrendStream.of(core);
  const retCode = wavetrendOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, outBegIdx, outNBElement, outWT1, outWT2, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('WAVETREND openAndFill', inHigh.length, startIdx, wavetrendLookback(optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, core));
  }
  throw streamFailure('WAVETREND openAndFill', retCode);
}

/** @internal */
export function wavetrendOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInChannelPeriod: number,
  optInAveragePeriod: number,
  optInSignalPeriod: number,
): WavetrendStream {
  const sp = WavetrendStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outWT1 = new Float64Array(1);
  const sink_outWT2 = new Float64Array(1);
  const retCode = wavetrendOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, outBegIdx, outNBElement, sink_outWT1, sink_outWT2, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('WAVETREND open', inHigh.length, startIdx, wavetrendLookback(optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, core));
  }
  throw streamFailure('WAVETREND open', retCode);
}

/**
 * Opens a live WAVETREND stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link wavetrend} there.
 *
 * @param inHigh - Warm-up history. High price series. Every history has the same
 *   length.
 * @param inLow - Warm-up history. Low price series. Every history has the same
 *   length.
 * @param inClose - Warm-up history. Close price series. Every history has the same
 *   length.
 * @param optInChannelPeriod - Period of the price channel, used by both the
 *   average and the deviation (default 10; range 2 to 100000; `Core.INTEGER_DEFAULT`
 *   selects the default).
 * @param optInAveragePeriod - Smoothing for the oscillator line (default 21; range
 *   1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSignalPeriod - Period of the simple average making the signal line
 *   (default 4; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `wavetrendLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for an integer parameter that is not an int32, a parameter
 *   outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { WavetrendOut, wavetrendOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = wavetrendOpen(high, low, close, 10, 21, 4);
 * const peeked = new WavetrendOut();
 * const updated = new WavetrendOut();
 * stream.peek(101.4, 99.1, 100.9, peeked); // a forming bar: commits nothing
 * stream.update(101.4, 99.1, 100.9, updated); // the bar closes
 * ```
 */
export function wavetrendOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInChannelPeriod: number,
  optInAveragePeriod: number,
  optInSignalPeriod: number,
  core: Core = Core.DEFAULT,
): WavetrendStream {
  optInChannelPeriod = requireInt32('WAVETREND open', 'optInChannelPeriod', optInChannelPeriod);
  optInAveragePeriod = requireInt32('WAVETREND open', 'optInAveragePeriod', optInAveragePeriod);
  optInSignalPeriod = requireInt32('WAVETREND open', 'optInSignalPeriod', optInSignalPeriod);
  requireArgument('WAVETREND open', 'inHigh', inHigh);
  requireHistory('WAVETREND open', inHigh.length);
  requireArgument('WAVETREND open', 'inLow', inLow);
  requireArgument('WAVETREND open', 'inClose', inClose);
  requireHistoryLength('WAVETREND open', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('WAVETREND open', 'inClose', inClose.length, inHigh.length);
  return wavetrendOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInChannelPeriod, optInAveragePeriod, optInSignalPeriod);
}

/**
 * {@link wavetrendOpen} that also fills the outputs over the whole history in the
 * same pass, bit-identically to {@link wavetrend}. The range written is the
 * stream's `outRange()`.
 *
 * @param inHigh - Warm-up history. High price series. Every history has the same
 *   length.
 * @param inLow - Warm-up history. Low price series. Every history has the same
 *   length.
 * @param inClose - Warm-up history. Close price series. Every history has the same
 *   length.
 * @param optInChannelPeriod - Period of the price channel, used by both the
 *   average and the deviation (default 10; range 2 to 100000; `Core.INTEGER_DEFAULT`
 *   selects the default).
 * @param optInAveragePeriod - Smoothing for the oscillator line (default 21; range
 *   1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInSignalPeriod - Period of the simple average making the signal line
 *   (default 4; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outWT1 - WaveTrend oscillator line. Must hold `historyLen - lookback`
 *   values and share no memory with an input or another output.
 * @param outWT2 - Simple average of the oscillator line. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `wavetrendLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for an integer parameter that is not an int32, a parameter
 *   outside its range, histories of different lengths or an output too short or
 *   starting where another output or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another output or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { wavetrendOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const wt1 = new Float64Array(high.length);
 * const wt2 = new Float64Array(high.length);
 * const stream = wavetrendOpenAndFill(high, low, close, 10, 21, 4, wt1, wt2);
 * const { begIdx, count } = stream.outRange();
 * // wt1[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function wavetrendOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInChannelPeriod: number,
  optInAveragePeriod: number,
  optInSignalPeriod: number,
  outWT1: Float64Array,
  outWT2: Float64Array,
  core: Core = Core.DEFAULT,
): WavetrendStream {
  optInChannelPeriod = requireInt32('WAVETREND openAndFill', 'optInChannelPeriod', optInChannelPeriod);
  optInAveragePeriod = requireInt32('WAVETREND openAndFill', 'optInAveragePeriod', optInAveragePeriod);
  optInSignalPeriod = requireInt32('WAVETREND openAndFill', 'optInSignalPeriod', optInSignalPeriod);
  requireArgument('WAVETREND openAndFill', 'inHigh', inHigh);
  requireHistory('WAVETREND openAndFill', inHigh.length);
  requireArgument('WAVETREND openAndFill', 'inLow', inLow);
  requireArgument('WAVETREND openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('WAVETREND openAndFill', inHigh.length, wavetrendLookback(optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, core));
  requireHistoryLength('WAVETREND openAndFill', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('WAVETREND openAndFill', 'inClose', inClose.length, inHigh.length);
  requireOutput('WAVETREND openAndFill', 'outWT1', outWT1, guardOutLen, Float64Array);
  requireOutput('WAVETREND openAndFill', 'outWT2', outWT2, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outWT1.buffer === inHighF64.buffer && outWT1.byteOffset === inHighF64.byteOffset) || (outWT1.buffer === inLowF64.buffer && outWT1.byteOffset === inLowF64.byteOffset) || (outWT1.buffer === inCloseF64.buffer && outWT1.byteOffset === inCloseF64.byteOffset) || (outWT2.buffer === inHighF64.buffer && outWT2.byteOffset === inHighF64.byteOffset) || (outWT2.buffer === inLowF64.buffer && outWT2.byteOffset === inLowF64.byteOffset) || (outWT2.buffer === inCloseF64.buffer && outWT2.byteOffset === inCloseF64.byteOffset) || (outWT1.buffer === outWT2.buffer && outWT1.byteOffset === outWT2.byteOffset)) {
    throw streamFailure('WAVETREND openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outWT1, inHighF64) || overlaps(outWT1, inLowF64) || overlaps(outWT1, inCloseF64) || overlaps(outWT2, inHighF64) || overlaps(outWT2, inLowF64) || overlaps(outWT2, inCloseF64) || overlaps(outWT1, outWT2))) {
    throw streamFailure('WAVETREND openAndFill', RetCode.BAD_PARAM);
  }
  return wavetrendOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, new MInteger(), new MInteger(), outWT1, outWT2);
}
