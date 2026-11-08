/*! TA-LIB Copyright (c) 1999-2026, Mario Fortier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither name of author nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


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

import { badSink, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { emaLookbackPositional } from './ema.ts';
import { smaLookbackPositional } from './sma.ts';

/** @internal */
export function wavetrendLookbackPositional(optInChannelPeriod: number, optInAveragePeriod: number, optInSignalPeriod: number, core: Core = Core.DEFAULT): number {
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
  return emaLookbackPositional(optInChannelPeriod, core) + emaLookbackPositional(optInChannelPeriod, core) + emaLookbackPositional(optInAveragePeriod, core) + smaLookbackPositional(optInSignalPeriod, core);
}

/** @internal */
export function wavetrendDisplayShiftPositional(optInChannelPeriod: number, optInAveragePeriod: number, optInSignalPeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (wavetrendLookbackPositional(optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, core) < 0) {
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
  lookbackTotal = wavetrendLookbackPositional(optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, core);
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
  lookbackChannel = emaLookbackPositional(optInChannelPeriod, core);
  lookbackAverage = emaLookbackPositional(optInAveragePeriod, core);
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

const PARAM_RANGES = [['channelPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['averagePeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['signalPeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000']] as const;

/** @internal */
export function wavetrendPositional(
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
  optInChannelPeriod = requireInt32('WAVETREND', 'channelPeriod', optInChannelPeriod);
  optInAveragePeriod = requireInt32('WAVETREND', 'averagePeriod', optInAveragePeriod);
  optInSignalPeriod = requireInt32('WAVETREND', 'signalPeriod', optInSignalPeriod);
  requireIndexRange('WAVETREND', startIdx, endIdx);
  const guardLookback = wavetrendLookbackPositional(optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('WAVETREND', PARAM_RANGES, optInChannelPeriod, optInAveragePeriod, optInSignalPeriod);
  }
  const guardStart = clampedStart('WAVETREND', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('WAVETREND', 'prices.high', inHigh, guardInLen);
  requireLength('WAVETREND', 'prices.low', inLow, guardInLen);
  requireLength('WAVETREND', 'prices.close', inClose, guardInLen);
  requireOutput('WAVETREND', 'outWt1', outWT1, guardOutLen, Float64Array);
  requireOutput('WAVETREND', 'outWt2', outWT2, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outWT1, inHighF64)) {
    throw sharedMemory('WAVETREND', 'outWt1', 'prices.high');
  }
  if (overlapsPartly(outWT1, inLowF64)) {
    throw sharedMemory('WAVETREND', 'outWt1', 'prices.low');
  }
  if (overlapsPartly(outWT1, inCloseF64)) {
    throw sharedMemory('WAVETREND', 'outWt1', 'prices.close');
  }
  if (overlapsPartly(outWT2, inHighF64)) {
    throw sharedMemory('WAVETREND', 'outWt2', 'prices.high');
  }
  if (overlapsPartly(outWT2, inLowF64)) {
    throw sharedMemory('WAVETREND', 'outWt2', 'prices.low');
  }
  if (overlapsPartly(outWT2, inCloseF64)) {
    throw sharedMemory('WAVETREND', 'outWt2', 'prices.close');
  }
  if (overlaps(outWT1, outWT2)) {
    throw sharedMemory('WAVETREND', 'outWt1', 'outWt2');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = wavetrendImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, outBegIdx, outNBElement, outWT1, outWT2);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('WAVETREND', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link wavetrend}, {@link wavetrendLookback},
 * {@link wavetrendDisplayShift}, {@link wavetrendOpen} and
 * {@link wavetrendOpenAndFill}. Every key is optional: an omitted or `undefined`
 * key takes its default, and a call ignores the keys it has no use for.
 */
export interface WavetrendOptions {
  /**
   * Period of the price channel, used by both the average and the deviation (default
   * 10; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  channelPeriod?: number | undefined;
  /**
   * Smoothing for the oscillator line (default 21; range 1 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  averagePeriod?: number | undefined;
  /**
   * Period of the simple average making the signal line (default 4; range 1 to
   * 100000; `Core.INTEGER_DEFAULT` selects the default).
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
   * Buffer for `outWt1`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outWt1?: Float64Array<ArrayBuffer> | undefined;
  /**
   * Buffer for `outWt2`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outWt2?: Float64Array<ArrayBuffer> | undefined;
}

/**
 * What {@link wavetrend} returns, and {@link wavetrendOpenAndFill} with its
 * stream.
 */
export interface WavetrendResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /** WaveTrend oscillator line. `outWt1[k]` is the value at input bar `begIdx + k`. */
  outWt1: Float64Array<ArrayBuffer>;
  /**
   * Simple average of the oscillator line. `outWt2[k]` is the value at input bar
   * `begIdx + k`.
   */
  outWt2: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link wavetrend} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link WavetrendOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function wavetrendLookback(options?: WavetrendOptions): number {
  const o = optionsOf('WAVETREND lookback', options);
  return wavetrendLookbackPositional(orDefault(o.channelPeriod, Core.INTEGER_DEFAULT), orDefault(o.averagePeriod, Core.INTEGER_DEFAULT), orDefault(o.signalPeriod, Core.INTEGER_DEFAULT), coreOf('WAVETREND lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link wavetrend}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link WavetrendOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function wavetrendDisplayShift(outputIdx: number, options?: WavetrendOptions): number {
  const o = optionsOf('WAVETREND displayShift', options);
  return wavetrendDisplayShiftPositional(orDefault(o.channelPeriod, Core.INTEGER_DEFAULT), orDefault(o.averagePeriod, Core.INTEGER_DEFAULT), orDefault(o.signalPeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('WAVETREND displayShift', o.core));
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
 * - The middle stage is an exponential CCI, not `cci`: CCI averages with a simple
 *   moving average and takes the mean deviation around that window's own average,
 *   where this uses two exponential averages.
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
 *   before it publishes. `CoreBuilder.unstablePeriod(FuncUnstId.EMA, ...)`
 *   discards more of that warm-up, and it counts three times because there are
 *   three exponential stages.
 * - The difference the author also plots is `sub(outWt1, outWt2)`.
 *
 * `outWt1[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link wavetrendLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `high`: High price
 *   series. `low`: Low price series. `close`: Close price series.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link WavetrendOptions}.
 * @returns `begIdx` and the values; see {@link WavetrendResult}. Each output is a
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
 * import { wavetrend } from 'ta-lib';
 *
 * const high = new Float64Array(252); // your data, oldest first
 * const low = new Float64Array(252); // your data, oldest first
 * const close = new Float64Array(252); // your data, oldest first
 *
 * const { begIdx, outWt1, outWt2 } = wavetrend({ high, low, close }, {
 *   channelPeriod: 10,
 *   averagePeriod: 21,
 *   signalPeriod: 4,
 * });
 * console.log(outWt1.length + ' values from bar ' + begIdx + '; last outWt1 = ' + outWt1[outWt1.length - 1]);
 * ```
 * @see [`cci`](https://ta-lib.org/functions/cci)
 * @see [`smi`](https://ta-lib.org/functions/smi)
 * @see [`stochrsi`](https://ta-lib.org/functions/stochrsi)
 * @see [`tsi`](https://ta-lib.org/functions/tsi)
 */
export function wavetrend(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: WavetrendOptions): WavetrendResult {
  requireArgument('WAVETREND', 'prices', prices);
  const o = optionsOf('WAVETREND', options);
  if (o.endIdx === undefined) {
    requireSeries('WAVETREND', 'prices.high', prices.high);
    requireSeries('WAVETREND', 'prices.low', prices.low);
    requireSeries('WAVETREND', 'prices.close', prices.close);
  }
  const optInChannelPeriod = orDefault(o.channelPeriod, Core.INTEGER_DEFAULT);
  const optInAveragePeriod = orDefault(o.averagePeriod, Core.INTEGER_DEFAULT);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('WAVETREND', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, wavetrendLookbackPositional(optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, core), prices.high, prices.low, prices.close);
  const outWT1 = o.outWt1 === undefined ? new Float64Array(capacity) : o.outWt1;
  const outWT2 = o.outWt2 === undefined ? new Float64Array(capacity) : o.outWt2;
  const range = wavetrendPositional(startIdx, endIdx, prices.high, prices.low, prices.close, optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, outWT1, outWT2, core);
  return { begIdx: range.begIdx, outWt1: outWT1.subarray(0, range.count), outWt2: outWT2.subarray(0, range.count) };
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
      throw streamFailure('WAVETREND advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns its outputs.
   *
   * @param high - The bar's `high` value: High price series.
   * @param low - The bar's `low` value: Low price series.
   * @param close - The bar's `close` value: Close price series.
   * @param out - Receives the outputs and is returned; a new {@link WavetrendOut}
   *   when omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included, before anything moves.
   *   `TALibError` with code `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached
   *   bar `Core.INDEX_MAX`.
   */
  update(high: number, low: number, close: number, out?: WavetrendOut): WavetrendOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('WAVETREND update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('WAVETREND update', 'WavetrendOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('WAVETREND update', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    wavetrendStepImpl(this, high, low, close);
    this.outRangeCount++;
    if (out === undefined) {
      out = new WavetrendOut();
    }
    out.outWt1 = this.cur_outWT1;
    out.outWt2 = this.cur_outWT2;
    return out;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param high - The bar's `high` value: High price series.
   * @param low - The bar's `low` value: Low price series.
   * @param close - The bar's `close` value: Close price series.
   * @param out - Receives the outputs and is returned; a new {@link WavetrendOut}
   *   when omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included.
   */
  peek(high: number, low: number, close: number, out?: WavetrendOut): WavetrendOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('WAVETREND peek', 'WavetrendOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('WAVETREND peek', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
    const inClose = close;
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
    if (out === undefined) {
      out = new WavetrendOut();
    }
    out.outWt1 = cur_outWT1;
    out.outWt2 = cur_outWT2;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link WavetrendOut}
   *   when omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a sink that is not an object,
   *   `null` included.
   */
  value(out?: WavetrendOut): WavetrendOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('WAVETREND value', 'WavetrendOut');
    }
    if (out === undefined) {
      out = new WavetrendOut();
    }
    out.outWt1 = this.cur_outWT1;
    out.outWt2 = this.cur_outWT2;
    return out;
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
 * The outputs of one WAVETREND bar, as `update`, `peek` and `value` return them.
 * Passed back to one of them, it is filled and returned again, with no allocation;
 * it holds its values until the next call that writes it. A new one holds `NaN`.
 */
export class WavetrendOut {
  /** WaveTrend oscillator line. */
  outWt1: number = NaN;
  /** Simple average of the oscillator line. */
  outWt2: number = NaN;
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
  if (startIdx >= historyLen) {
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
  lookbackTotal = wavetrendLookbackPositional(optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, core);
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
  lookbackChannel = emaLookbackPositional(optInChannelPeriod, core);
  lookbackAverage = emaLookbackPositional(optInAveragePeriod, core);
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
    throw insufficientHistory('WAVETREND openAndFill', inHigh.length, startIdx, wavetrendLookbackPositional(optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && wavetrendLookbackPositional(optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, core) < 0) {
    throw rejectedParams('WAVETREND openAndFill', PARAM_RANGES, optInChannelPeriod, optInAveragePeriod, optInSignalPeriod);
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
    throw insufficientHistory('WAVETREND open', inHigh.length, startIdx, wavetrendLookbackPositional(optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && wavetrendLookbackPositional(optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, core) < 0) {
    throw rejectedParams('WAVETREND open', PARAM_RANGES, optInChannelPeriod, optInAveragePeriod, optInSignalPeriod);
  }
  throw streamFailure('WAVETREND open', retCode);
}

/** @internal */
export function wavetrendOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInChannelPeriod: number,
  optInAveragePeriod: number,
  optInSignalPeriod: number,
  core: Core = Core.DEFAULT,
): WavetrendStream {
  optInChannelPeriod = requireInt32('WAVETREND open', 'channelPeriod', optInChannelPeriod);
  optInAveragePeriod = requireInt32('WAVETREND open', 'averagePeriod', optInAveragePeriod);
  optInSignalPeriod = requireInt32('WAVETREND open', 'signalPeriod', optInSignalPeriod);
  requireArgument('WAVETREND open', 'prices.high', inHigh);
  requireHistory('WAVETREND open', 'prices.high', inHigh.length);
  requireArgument('WAVETREND open', 'prices.low', inLow);
  requireArgument('WAVETREND open', 'prices.close', inClose);
  requireHistoryLength('WAVETREND open', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('WAVETREND open', 'prices.close', inClose.length, inHigh.length);
  return wavetrendOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInChannelPeriod, optInAveragePeriod, optInSignalPeriod);
}

/** @internal */
export function wavetrendOpenAndFillPositional(
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
  optInChannelPeriod = requireInt32('WAVETREND openAndFill', 'channelPeriod', optInChannelPeriod);
  optInAveragePeriod = requireInt32('WAVETREND openAndFill', 'averagePeriod', optInAveragePeriod);
  optInSignalPeriod = requireInt32('WAVETREND openAndFill', 'signalPeriod', optInSignalPeriod);
  requireArgument('WAVETREND openAndFill', 'prices.high', inHigh);
  requireHistory('WAVETREND openAndFill', 'prices.high', inHigh.length);
  requireArgument('WAVETREND openAndFill', 'prices.low', inLow);
  requireArgument('WAVETREND openAndFill', 'prices.close', inClose);
  const guardLookback = wavetrendLookbackPositional(optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('WAVETREND openAndFill', PARAM_RANGES, optInChannelPeriod, optInAveragePeriod, optInSignalPeriod);
  }
  const guardOutLen = openFillCount('WAVETREND openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('WAVETREND openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('WAVETREND openAndFill', 'prices.close', inClose.length, inHigh.length);
  requireOutput('WAVETREND openAndFill', 'outWt1', outWT1, guardOutLen, Float64Array);
  requireOutput('WAVETREND openAndFill', 'outWt2', outWT2, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outWT1.buffer === inHighF64.buffer && outWT1.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('WAVETREND openAndFill', 'outWt1', 'prices.high');
  }
  if (outWT1.buffer === inLowF64.buffer && outWT1.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('WAVETREND openAndFill', 'outWt1', 'prices.low');
  }
  if (outWT1.buffer === inCloseF64.buffer && outWT1.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('WAVETREND openAndFill', 'outWt1', 'prices.close');
  }
  if (outWT2.buffer === inHighF64.buffer && outWT2.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('WAVETREND openAndFill', 'outWt2', 'prices.high');
  }
  if (outWT2.buffer === inLowF64.buffer && outWT2.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('WAVETREND openAndFill', 'outWt2', 'prices.low');
  }
  if (outWT2.buffer === inCloseF64.buffer && outWT2.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('WAVETREND openAndFill', 'outWt2', 'prices.close');
  }
  if (outWT1.buffer === outWT2.buffer && outWT1.byteOffset === outWT2.byteOffset) {
    throw sharedMemory('WAVETREND openAndFill', 'outWt1', 'outWt2');
  }
  if (guardOutLen > 0 && overlaps(outWT1, inHighF64)) {
    throw sharedMemory('WAVETREND openAndFill', 'outWt1', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outWT1, inLowF64)) {
    throw sharedMemory('WAVETREND openAndFill', 'outWt1', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outWT1, inCloseF64)) {
    throw sharedMemory('WAVETREND openAndFill', 'outWt1', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outWT2, inHighF64)) {
    throw sharedMemory('WAVETREND openAndFill', 'outWt2', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outWT2, inLowF64)) {
    throw sharedMemory('WAVETREND openAndFill', 'outWt2', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outWT2, inCloseF64)) {
    throw sharedMemory('WAVETREND openAndFill', 'outWt2', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outWT1, outWT2)) {
    throw sharedMemory('WAVETREND openAndFill', 'outWt1', 'outWt2');
  }
  return wavetrendOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, new MInteger(), new MInteger(), outWT1, outWT2);
}

/**
 * Opens a live WAVETREND stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link wavetrend} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price series. `low`: Low price series. `close`: Close price series.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link WavetrendOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `wavetrendLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, an integer parameter that is not an int32, a parameter
 *   outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { wavetrendOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = wavetrendOpen({ high, low, close }, {
 *   channelPeriod: 10,
 *   averagePeriod: 21,
 *   signalPeriod: 4,
 * });
 * const peeked = stream.peek(101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function wavetrendOpen(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: WavetrendOptions): WavetrendStream {
  requireArgument('WAVETREND open', 'prices', prices);
  const o = optionsOf('WAVETREND open', options);
  const optInChannelPeriod = orDefault(o.channelPeriod, Core.INTEGER_DEFAULT);
  const optInAveragePeriod = orDefault(o.averagePeriod, Core.INTEGER_DEFAULT);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('WAVETREND open', o.core);
  return wavetrendOpenPositional(prices.high, prices.low, prices.close, optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, core);
}

/**
 * {@link wavetrendOpen} that also computes the outputs over the whole history in
 * the same pass, bit-identically to {@link wavetrend}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `high`: High price series. `low`: Low price series. `close`: Close price series.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link WavetrendOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link wavetrend} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `wavetrendLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, an integer parameter that is not an int32, a parameter
 *   outside its range, histories of different lengths or a buffer too short, of the
 *   wrong type or starting where another buffer or a `Float64Array` input starts,
 *   or, on a history long enough to open, one sharing memory with another buffer or
 *   a `Float64Array` input.
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
 * const { stream, begIdx, outWt1, outWt2 } = wavetrendOpenAndFill({ high, low, close }, {
 *   channelPeriod: 10,
 *   averagePeriod: 21,
 *   signalPeriod: 4,
 * });
 * // outWt1[k] is the value at bar begIdx + k
 * ```
 */
export function wavetrendOpenAndFill(prices: { open?: ArrayLike<number> | undefined; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: WavetrendOptions): WavetrendResult & { stream: WavetrendStream } {
  requireArgument('WAVETREND openAndFill', 'prices', prices);
  const o = optionsOf('WAVETREND openAndFill', options);
  const optInChannelPeriod = orDefault(o.channelPeriod, Core.INTEGER_DEFAULT);
  const optInAveragePeriod = orDefault(o.averagePeriod, Core.INTEGER_DEFAULT);
  const optInSignalPeriod = orDefault(o.signalPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('WAVETREND openAndFill', o.core);
  const capacity = fillCapacity(wavetrendLookbackPositional(optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, core), prices.high, prices.low, prices.close);
  const outWT1 = o.outWt1 === undefined ? new Float64Array(capacity) : o.outWt1;
  const outWT2 = o.outWt2 === undefined ? new Float64Array(capacity) : o.outWt2;
  const stream = wavetrendOpenAndFillPositional(prices.high, prices.low, prices.close, optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, outWT1, outWT2, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outWt1: outWT1.subarray(0, range.count), outWt2: outWT2.subarray(0, range.count) };
}
