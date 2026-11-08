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


/* Schaff Trend Cycle, created by Doug Schaff (FX-Strategy.com), who released
 * its code in "Releasing the Code to the Schaff Trend Cycle", FXStreet.com,
 * February 15, 2008,
 * https://web.archive.org/web/20090418215759/mediaserver.fxstreet.com/Reports/99afdb5f-d41d-4a2c-802c-f5d787df886c/ebfbf387-4b27-4a0f-848c-039f4ab77c00.pdf
 *
 * List of contributors:
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
 *  092926 MF,CC  Initial version (#478).
 *  100726 MF,CC  #492. The Auto rule takes the bars the shorter EMA count gives up.
 */

import { FuncUnstId } from '../enums.ts';
import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { emaLookbackPositional } from './ema.ts';

/** @internal */
export function stcLookbackPositional(optInFastPeriod: number, optInSlowPeriod: number, optInCyclePeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInFastPeriod | 0) !== optInFastPeriod) {
    return -1;
  }
  if ((optInSlowPeriod | 0) !== optInSlowPeriod) {
    return -1;
  }
  if ((optInCyclePeriod | 0) !== optInCyclePeriod) {
    return -1;
  }
  if (optInFastPeriod === Core.INTEGER_DEFAULT) {
    optInFastPeriod = 23;
  } else if (optInFastPeriod < 2 || optInFastPeriod > 100000) {
    return -1;
  }
  if (optInSlowPeriod === Core.INTEGER_DEFAULT) {
    optInSlowPeriod = 50;
  } else if (optInSlowPeriod < 2 || optInSlowPeriod > 100000) {
    return -1;
  }
  if (optInCyclePeriod === Core.INTEGER_DEFAULT) {
    optInCyclePeriod = 10;
  } else if (optInCyclePeriod < 2 || optInCyclePeriod > 100000) {
    return -1;
  }
  let tempInteger = 0;
  if (optInSlowPeriod < optInFastPeriod) {
    tempInteger = optInSlowPeriod;
    optInSlowPeriod = optInFastPeriod;
    optInFastPeriod = tempInteger;
  }
  /* The MACD line's own lookback, which is what inherits TA_FUNC_UNST_EMA,
   * then one window per stochastic stage. The two 0.5 smoothers seed on
   * their first input, so they add only the unstable period.
   */
  return emaLookbackPositional(optInSlowPeriod, core) + ((2 * (optInCyclePeriod - 1)) | 0) + core.unstableCount(FuncUnstId.STC, (((((5 * 10) | 0) + 1) / 2) | 0) + ((3 * (optInSlowPeriod + 1)) | 0), (((((5 * 19) | 0) + 1) / 2) | 0) + ((3 * (optInSlowPeriod + 1)) | 0));
}

/** @internal */
export function stcDisplayShiftPositional(optInFastPeriod: number, optInSlowPeriod: number, optInCyclePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (stcLookbackPositional(optInFastPeriod, optInSlowPeriod, optInCyclePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function stcImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInCyclePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let lineRing!: Float64Array;
  let lineRing_Idx = 0;
  let maxIdx_lineRing = (30) - 1;
  let lineSufHi!: Float64Array;
  let lineSufLo!: Float64Array;
  let pfRing!: Float64Array;
  let pfRing_Idx = 0;
  let maxIdx_pfRing = (30) - 1;
  let pfSufHi!: Float64Array;
  let pfSufLo!: Float64Array;
  let prevFast = 0;
  let prevSlow = 0;
  let fastK = 0;
  let slowK = 0;
  let tempReal = 0;
  let lineValue = 0;
  let fastBeta = 0;
  let slowBeta = 0;
  let lowest = 0;
  let highest = 0;
  let range = 0;
  let frac1 = 0;
  let frac2 = 0;
  let pf = 0;
  let pff = 0;
  let lineHi = 0;
  let lineLo = 0;
  let pfHi = 0;
  let pfLo = 0;
  let sufHi = 0;
  let sufLo = 0;
  let i = 0;
  let today = 0;
  let fastToday = 0;
  let lineStart = 0;
  let outIdx = 0;
  let tempInteger = 0;
  let lookbackTotal = 0;
  let lookbackSlow = 0;
  let lastIdx = 0;
  let nLine = 0;
  let nPF = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInFastPeriod === Core.INTEGER_DEFAULT) {
    optInFastPeriod = 23;
  } else if (optInFastPeriod < 2 || optInFastPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowPeriod === Core.INTEGER_DEFAULT) {
    optInSlowPeriod = 50;
  } else if (optInSlowPeriod < 2 || optInSlowPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInCyclePeriod === Core.INTEGER_DEFAULT) {
    optInCyclePeriod = 10;
  } else if (optInCyclePeriod < 2 || optInCyclePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowPeriod < optInFastPeriod) {
    tempInteger = optInSlowPeriod;
    optInSlowPeriod = optInFastPeriod;
    optInFastPeriod = tempInteger;
  }
  lookbackTotal = stcLookbackPositional(optInFastPeriod, optInSlowPeriod, optInCyclePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  outBegIdx.value = startIdx;
  fastBeta = (optInFastPeriod - 1) / (optInFastPeriod + 1);
  fastK = 1.0 - fastBeta;
  if (fastBeta < 0.5) {
    fastBeta = 1.0 - fastK;
  }
  slowBeta = (optInSlowPeriod - 1) / (optInSlowPeriod + 1);
  slowK = 1.0 - slowBeta;
  if (slowBeta < 0.5) {
    slowBeta = 1.0 - slowK;
  }
  /* Rolling extrema, van Herk / Gil-Werman: the window ending in slot j is
   * the current block's prefix extremum joined with the previous block's
   * suffix extremum from slot j+1. The extrema are exact, so the output must
   * stay bit-identical to a full rescan of each window.
   */
  if (optInCyclePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  lineRing = new Float64Array(optInCyclePeriod);
  maxIdx_lineRing = optInCyclePeriod - 1;
  lineRing_Idx = 0;
  if (optInCyclePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  lineSufHi = new Float64Array(optInCyclePeriod);
  if (optInCyclePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  lineSufLo = new Float64Array(optInCyclePeriod);
  if (optInCyclePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  pfRing = new Float64Array(optInCyclePeriod);
  maxIdx_pfRing = optInCyclePeriod - 1;
  pfRing_Idx = 0;
  if (optInCyclePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  pfSufHi = new Float64Array(optInCyclePeriod);
  if (optInCyclePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  pfSufLo = new Float64Array(optInCyclePeriod);
  lastIdx = optInCyclePeriod - 1;
  /* The line is TA_MACD's: each SMA seed placed by its own EMA lookback, then
   * both EMAs advanced to lineStart, so that from lineStart on it is
   * TA_EMA(fast) - TA_EMA(slow) bit for bit. That placement reads out of
   * bounds or skips bars unless ema_lookback(n) - n never decreases as n
   * grows. The chain is fed from
   * lineStart, not from the EMA seed: TA_FUNC_UNST_EMA then reaches only the
   * line, while TA_FUNC_UNST_STC moves the whole chain back and so warms the
   * line and both smoothers.
   */
  lookbackSlow = emaLookbackPositional(optInSlowPeriod, core);
  lineStart = startIdx - (lookbackTotal - lookbackSlow);
  today = startIdx - lookbackTotal;
  tempReal = 0.0;
  i = optInSlowPeriod;
  while (i-- > 0) {
    tempReal += inReal[today++];
  }
  prevSlow = tempReal / optInSlowPeriod;
  fastToday = startIdx - lookbackTotal + (lookbackSlow - emaLookbackPositional(optInFastPeriod, core));
  prevFast = 0.0;
  i = optInFastPeriod;
  while (i-- > 0) {
    prevFast += inReal[fastToday++];
  }
  prevFast = prevFast / optInFastPeriod;
  while (today < fastToday) {
    tempReal = inReal[today++];
    {
      const _a: number = slowBeta, _b: number = prevSlow, _c: number = slowK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevSlow = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
  }
  while (today <= lineStart) {
    tempReal = inReal[today++];
    {
      const _a: number = fastBeta, _b: number = prevFast, _c: number = fastK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevFast = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = slowBeta, _b: number = prevSlow, _c: number = slowK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevSlow = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
  }
  /* A zero range holds the previous fraction (0.0 before any), and the test
   * is exact: in a sustained trend PF saturates at 100 and the second
   * range reaches exactly 0 while the output must stay at 100.
   */
  frac1 = 0.0;
  frac2 = 0.0;
  pf = 0.0;
  pff = 0.0;
  pfHi = 0.0;
  pfLo = 0.0;
  nPF = 0;
  lineValue = prevFast - prevSlow;
  lineRing[lineRing_Idx] = lineValue;
  lineRing_Idx++;
  if (lineRing_Idx > maxIdx_lineRing) {
    lineRing_Idx = 0;
  }
  lineHi = lineValue;
  lineLo = lineValue;
  nLine = 1;
  /* Warm-up, through startIdx inclusive. Each stage starts once its
   * window is full, and each smoother is seeded on its first input.
   */
  while (today <= startIdx) {
    tempReal = inReal[today];
    {
      const _a: number = fastBeta, _b: number = prevFast, _c: number = fastK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevFast = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = slowBeta, _b: number = prevSlow, _c: number = slowK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevSlow = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    lineValue = prevFast - prevSlow;
    nLine = nLine + 1;
    lineRing[lineRing_Idx] = lineValue;
    if (lineRing_Idx === 0) {
      lineHi = lineValue;
      lineLo = lineValue;
    } else {
      if (lineValue > lineHi) {
        lineHi = lineValue;
      }
      if (lineValue < lineLo) {
        lineLo = lineValue;
      }
    }
    highest = lineHi;
    lowest = lineLo;
    if (nLine >= optInCyclePeriod && lineRing_Idx < lastIdx) {
      tempReal = lineSufHi[lineRing_Idx + 1];
      if (tempReal > highest) {
        highest = tempReal;
      }
      tempReal = lineSufLo[lineRing_Idx + 1];
      if (tempReal < lowest) {
        lowest = tempReal;
      }
    }
    lineRing_Idx++;
    if (lineRing_Idx > maxIdx_lineRing) {
      lineRing_Idx = 0;
    }
    if (lineRing_Idx === 0) {
      sufHi = lineRing[lastIdx];
      sufLo = sufHi;
      lineSufHi[lastIdx] = sufHi;
      lineSufLo[lastIdx] = sufLo;
      i = lastIdx;
      while (i > 0) {
        i -= 1;
        tempReal = lineRing[i];
        if (tempReal > sufHi) {
          sufHi = tempReal;
        }
        if (tempReal < sufLo) {
          sufLo = tempReal;
        }
        lineSufHi[i] = sufHi;
        lineSufLo[i] = sufLo;
      }
    }
    if (nLine >= optInCyclePeriod) {
      range = highest - lowest;
      if (range > 0.0) {
        frac1 = (lineValue - lowest) / range * 100.0;
      }
      if (nPF === 0) {
        pf = frac1;
      } else {
        {
          const _a: number = 0.5, _b: number = frac1 - pf, _c: number = pf;
          const _p = _a * _b;
          let _t = 134217729 * _a;
          const _ah = _t - (_t - _a), _al = _a - _ah;
          _t = 134217729 * _b;
          const _bh = _t - (_t - _b), _bl = _b - _bh;
          const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
          const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
          const _v = _tl + _pl, _v2 = _v + _v;
          const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
          pf = _w && (_pl === 0 || _v === 0) ? _th
            : _w && _v2 !== (_th + _v2) - _th ? _th + _v
            : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
        }
      }
      nPF = nPF + 1;
      pfRing[pfRing_Idx] = pf;
      if (pfRing_Idx === 0) {
        pfHi = pf;
        pfLo = pf;
      } else {
        if (pf > pfHi) {
          pfHi = pf;
        }
        if (pf < pfLo) {
          pfLo = pf;
        }
      }
      highest = pfHi;
      lowest = pfLo;
      if (nPF >= optInCyclePeriod && pfRing_Idx < lastIdx) {
        tempReal = pfSufHi[pfRing_Idx + 1];
        if (tempReal > highest) {
          highest = tempReal;
        }
        tempReal = pfSufLo[pfRing_Idx + 1];
        if (tempReal < lowest) {
          lowest = tempReal;
        }
      }
      pfRing_Idx++;
      if (pfRing_Idx > maxIdx_pfRing) {
        pfRing_Idx = 0;
      }
      if (pfRing_Idx === 0) {
        sufHi = pfRing[lastIdx];
        sufLo = sufHi;
        pfSufHi[lastIdx] = sufHi;
        pfSufLo[lastIdx] = sufLo;
        i = lastIdx;
        while (i > 0) {
          i -= 1;
          tempReal = pfRing[i];
          if (tempReal > sufHi) {
            sufHi = tempReal;
          }
          if (tempReal < sufLo) {
            sufLo = tempReal;
          }
          pfSufHi[i] = sufHi;
          pfSufLo[i] = sufLo;
        }
      }
      if (nPF >= optInCyclePeriod) {
        range = highest - lowest;
        if (range > 0.0) {
          frac2 = (pf - lowest) / range * 100.0;
        }
        if (nPF === optInCyclePeriod) {
          pff = frac2;
        } else {
          {
            const _a: number = 0.5, _b: number = frac2 - pff, _c: number = pff;
            const _p = _a * _b;
            let _t = 134217729 * _a;
            const _ah = _t - (_t - _a), _al = _a - _ah;
            _t = 134217729 * _b;
            const _bh = _t - (_t - _b), _bl = _b - _bh;
            const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
            const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
            const _v = _tl + _pl, _v2 = _v + _v;
            const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
            pff = _w && (_pl === 0 || _v === 0) ? _th
              : _w && _v2 !== (_th + _v2) - _th ? _th + _v
              : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
          }
        }
      }
    }
    today = today + 1;
  }
  outReal[0] = pff;
  outIdx = 1;
  while (today <= endIdx) {
    tempReal = inReal[today];
    {
      const _a: number = fastBeta, _b: number = prevFast, _c: number = fastK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevFast = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = slowBeta, _b: number = prevSlow, _c: number = slowK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevSlow = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    lineValue = prevFast - prevSlow;
    lineRing[lineRing_Idx] = lineValue;
    if (lineRing_Idx === 0) {
      lineHi = lineValue;
      lineLo = lineValue;
    } else {
      if (lineValue > lineHi) {
        lineHi = lineValue;
      }
      if (lineValue < lineLo) {
        lineLo = lineValue;
      }
    }
    highest = lineHi;
    lowest = lineLo;
    if (lineRing_Idx < lastIdx) {
      tempReal = lineSufHi[lineRing_Idx + 1];
      if (tempReal > highest) {
        highest = tempReal;
      }
      tempReal = lineSufLo[lineRing_Idx + 1];
      if (tempReal < lowest) {
        lowest = tempReal;
      }
    }
    lineRing_Idx++;
    if (lineRing_Idx > maxIdx_lineRing) {
      lineRing_Idx = 0;
    }
    if (lineRing_Idx === 0) {
      sufHi = lineRing[lastIdx];
      sufLo = sufHi;
      lineSufHi[lastIdx] = sufHi;
      lineSufLo[lastIdx] = sufLo;
      i = lastIdx;
      while (i > 0) {
        i -= 1;
        tempReal = lineRing[i];
        if (tempReal > sufHi) {
          sufHi = tempReal;
        }
        if (tempReal < sufLo) {
          sufLo = tempReal;
        }
        lineSufHi[i] = sufHi;
        lineSufLo[i] = sufLo;
      }
    }
    range = highest - lowest;
    if (range > 0.0) {
      frac1 = (lineValue - lowest) / range * 100.0;
    }
    {
      const _a: number = 0.5, _b: number = frac1 - pf, _c: number = pf;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      pf = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    pfRing[pfRing_Idx] = pf;
    if (pfRing_Idx === 0) {
      pfHi = pf;
      pfLo = pf;
    } else {
      if (pf > pfHi) {
        pfHi = pf;
      }
      if (pf < pfLo) {
        pfLo = pf;
      }
    }
    highest = pfHi;
    lowest = pfLo;
    if (pfRing_Idx < lastIdx) {
      tempReal = pfSufHi[pfRing_Idx + 1];
      if (tempReal > highest) {
        highest = tempReal;
      }
      tempReal = pfSufLo[pfRing_Idx + 1];
      if (tempReal < lowest) {
        lowest = tempReal;
      }
    }
    pfRing_Idx++;
    if (pfRing_Idx > maxIdx_pfRing) {
      pfRing_Idx = 0;
    }
    if (pfRing_Idx === 0) {
      sufHi = pfRing[lastIdx];
      sufLo = sufHi;
      pfSufHi[lastIdx] = sufHi;
      pfSufLo[lastIdx] = sufLo;
      i = lastIdx;
      while (i > 0) {
        i -= 1;
        tempReal = pfRing[i];
        if (tempReal > sufHi) {
          sufHi = tempReal;
        }
        if (tempReal < sufLo) {
          sufLo = tempReal;
        }
        pfSufHi[i] = sufHi;
        pfSufLo[i] = sufLo;
      }
    }
    range = highest - lowest;
    if (range > 0.0) {
      frac2 = (pf - lowest) / range * 100.0;
    }
    {
      const _a: number = 0.5, _b: number = frac2 - pff, _c: number = pff;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      pff = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    outReal[outIdx++] = pff;
    today = today + 1;
  }
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['fastPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['slowPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['cyclePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000']] as const;

/** @internal */
export function stcPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInCyclePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('STC', 'startIdx', startIdx);
  endIdx = requireInt32('STC', 'endIdx', endIdx);
  optInFastPeriod = requireInt32('STC', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('STC', 'slowPeriod', optInSlowPeriod);
  optInCyclePeriod = requireInt32('STC', 'cyclePeriod', optInCyclePeriod);
  requireIndexRange('STC', startIdx, endIdx);
  const guardLookback = stcLookbackPositional(optInFastPeriod, optInSlowPeriod, optInCyclePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('STC', PARAM_RANGES, optInFastPeriod, optInSlowPeriod, optInCyclePeriod);
  }
  const guardStart = clampedStart('STC', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('STC', 'real', inReal, guardInLen);
  requireOutput('STC', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('STC', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = stcImpl(core, startIdx, endIdx, inRealF64, optInFastPeriod, optInSlowPeriod, optInCyclePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('STC', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link stc}, {@link stcLookback}, {@link stcDisplayShift},
 * {@link stcOpen} and {@link stcOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface StcOptions {
  /**
   * Period of the fast EMA (default 23; range 2 to 100000; `Core.INTEGER_DEFAULT`
   * selects the default).
   */
  fastPeriod?: number | undefined;
  /**
   * Period of the slow EMA (default 50; range 2 to 100000; `Core.INTEGER_DEFAULT`
   * selects the default).
   */
  slowPeriod?: number | undefined;
  /**
   * Window of both stochastic stages (default 10; range 2 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  cyclePeriod?: number | undefined;
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

/** What {@link stc} returns, and {@link stcOpenAndFill} with its stream. */
export interface StcResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Schaff Trend Cycle, from 0 to 100. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link stc} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * It includes `core`'s unstable period for this function.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link StcOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function stcLookback(options?: StcOptions): number {
  const o = optionsOf('STC lookback', options);
  return stcLookbackPositional(orDefault(o.fastPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowPeriod, Core.INTEGER_DEFAULT), orDefault(o.cyclePeriod, Core.INTEGER_DEFAULT), coreOf('STC lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link stc}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link StcOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function stcDisplayShift(outputIdx: number, options?: StcOptions): number {
  const o = optionsOf('STC displayShift', options);
  return stcDisplayShiftPositional(orDefault(o.fastPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowPeriod, Core.INTEGER_DEFAULT), orDefault(o.cyclePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('STC displayShift', o.core));
}

/**
 * Schaff Trend Cycle (Doug Schaff): a MACD line passed twice through a stochastic,
 * each pass smoothed by half. Bounded 0 to 100, read against 25 and 75: turning up
 * from below 25 is bullish, turning down from above 75 bearish.
 *
 * Formula and more info at
 * [ta-lib.org/functions/stc](https://ta-lib.org/functions/stc).
 *
 * **Notes**
 * - The smoothing factor is fixed at 0.5, as in Schaff's published code.
 * - The MACD line is TA-Lib's: both EMAs are seeded with a simple average over
 *   windows ending on the same bar. The published code runs the EMAs from the
 *   first bar of the data.
 * - If the slow period is set smaller than the fast period, the two are swapped,
 *   as in `MACD`.
 * - Being recursive, an output depends on how much history precedes it. The
 *   unstable period warms the two smoothers; the EMA unstable period warms the
 *   MACD line.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link stcLookback} is a success with empty outputs.
 *
 * @param real - Input series (typically close)
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link StcOptions}.
 * @returns `begIdx` and the values; see {@link StcResult}. Each output is a view
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
 * import { stc } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = stc(data, {
 *   fastPeriod: 23,
 *   slowPeriod: 50,
 *   cyclePeriod: 10,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`macd`](https://ta-lib.org/functions/macd)
 * @see [`stochf`](https://ta-lib.org/functions/stochf)
 * @see [`stochrsi`](https://ta-lib.org/functions/stochrsi)
 */
export function stc(real: ArrayLike<number>, options?: StcOptions): StcResult {
  const o = optionsOf('STC', options);
  if (o.endIdx === undefined) {
    requireSeries('STC', 'real', real);
  }
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const optInCyclePeriod = orDefault(o.cyclePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('STC', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, stcLookbackPositional(optInFastPeriod, optInSlowPeriod, optInCyclePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = stcPositional(startIdx, endIdx, real, optInFastPeriod, optInSlowPeriod, optInCyclePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live STC stream: one value per closed bar, bit-identical to {@link stc} over
 * the same series. Open it with {@link stcOpen} or {@link stcOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/stc](https://ta-lib.org/functions/stc).
 */
export class StcStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInFastPeriod: number = 0;
  /** @internal */
  optInSlowPeriod: number = 0;
  /** @internal */
  optInCyclePeriod: number = 0;
  /** @internal */
  prevFast: number = 0;
  /** @internal */
  prevSlow: number = 0;
  /** @internal */
  fastK: number = 0;
  /** @internal */
  slowK: number = 0;
  /** @internal */
  fastBeta: number = 0;
  /** @internal */
  slowBeta: number = 0;
  /** @internal */
  frac1: number = 0;
  /** @internal */
  frac2: number = 0;
  /** @internal */
  pf: number = 0;
  /** @internal */
  pff: number = 0;
  /** @internal */
  lineHi: number = 0;
  /** @internal */
  lineLo: number = 0;
  /** @internal */
  pfHi: number = 0;
  /** @internal */
  pfLo: number = 0;
  /** @internal */
  lastIdx: number = 0;
  /** @internal */
  lineRing_Idx: number = 0;
  /** @internal */
  pfRing_Idx: number = 0;
  /** @internal */
  maxIdx_lineRing: number = 0;
  /** @internal */
  lineSufHi_Idx: number = 0;
  /** @internal */
  maxIdx_lineSufHi: number = 0;
  /** @internal */
  lineSufLo_Idx: number = 0;
  /** @internal */
  maxIdx_lineSufLo: number = 0;
  /** @internal */
  maxIdx_pfRing: number = 0;
  /** @internal */
  pfSufHi_Idx: number = 0;
  /** @internal */
  maxIdx_pfSufHi: number = 0;
  /** @internal */
  pfSufLo_Idx: number = 0;
  /** @internal */
  maxIdx_pfSufLo: number = 0;
  /** @internal */
  cbSize_lineRing: number = 0;
  /** @internal */
  cb_lineRing: Float64Array = new Float64Array(1);
  /** @internal */
  cbSize_lineSufHi: number = 0;
  /** @internal */
  cb_lineSufHi: Float64Array = new Float64Array(1);
  /** @internal */
  cbSize_lineSufLo: number = 0;
  /** @internal */
  cb_lineSufLo: Float64Array = new Float64Array(1);
  /** @internal */
  cbSize_pfRing: number = 0;
  /** @internal */
  cb_pfRing: Float64Array = new Float64Array(1);
  /** @internal */
  cbSize_pfSufHi: number = 0;
  /** @internal */
  cb_pfSufHi: Float64Array = new Float64Array(1);
  /** @internal */
  cbSize_pfSufLo: number = 0;
  /** @internal */
  cb_pfSufLo: Float64Array = new Float64Array(1);
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
  static of(core: Core): StcStream {
    return new StcStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link stc} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('STC advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: Input series (typically close)
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('STC update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('STC update', 'real');
    }
    stcStepImpl(this, real);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: Input series (typically close)
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number): number {
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('STC peek', 'real');
    }
    const sp = this;
    const inReal = real;
    let tempReal = 0;
    let lineValue = 0;
    let lowest = 0;
    let highest = 0;
    let range = 0;
    let sufHi = 0;
    let sufLo = 0;
    let i = 0;
    let cur_outReal = 0;
    let frac1 = sp.frac1;
    let frac2 = sp.frac2;
    let lineHi = sp.lineHi;
    let lineLo = sp.lineLo;
    let lineRing_Idx = sp.lineRing_Idx;
    let pf = sp.pf;
    let pfHi = sp.pfHi;
    let pfLo = sp.pfLo;
    let pfRing_Idx = sp.pfRing_Idx;
    let pff = sp.pff;
    let prevFast = sp.prevFast;
    let prevSlow = sp.prevSlow;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    let pkSlot1 = -1;
    let pkVal1 = 0;
    tempReal = inReal;
    {
      const _a: number = sp.fastBeta, _b: number = prevFast, _c: number = sp.fastK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevFast = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = sp.slowBeta, _b: number = prevSlow, _c: number = sp.slowK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevSlow = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    lineValue = prevFast - prevSlow;
    pkSlot0 = lineRing_Idx;
    pkVal0 = lineValue;
    if (lineRing_Idx === 0) {
      lineHi = lineValue;
      lineLo = lineValue;
    } else {
      if (lineValue > lineHi) {
        lineHi = lineValue;
      }
      if (lineValue < lineLo) {
        lineLo = lineValue;
      }
    }
    highest = lineHi;
    lowest = lineLo;
    if (lineRing_Idx < sp.lastIdx) {
      tempReal = sp.cb_lineSufHi[lineRing_Idx + 1];
      if (tempReal > highest) {
        highest = tempReal;
      }
      tempReal = sp.cb_lineSufLo[lineRing_Idx + 1];
      if (tempReal < lowest) {
        lowest = tempReal;
      }
    }
    lineRing_Idx = lineRing_Idx + 1;
    if (lineRing_Idx > sp.maxIdx_lineRing) {
      lineRing_Idx = 0;
    }
    if (lineRing_Idx === 0) {
      sufHi = (sp.lastIdx !== pkSlot0) ? sp.cb_lineRing[sp.lastIdx] : pkVal0;
      sufLo = sufHi;
      i = sp.lastIdx;
      while (i > 0) {
        i -= 1;
        tempReal = (i !== pkSlot0) ? sp.cb_lineRing[i] : pkVal0;
        if (tempReal > sufHi) {
          sufHi = tempReal;
        }
        if (tempReal < sufLo) {
          sufLo = tempReal;
        }
      }
    }
    range = highest - lowest;
    if (range > 0.0) {
      frac1 = (lineValue - lowest) / range * 100.0;
    }
    {
      const _a: number = 0.5, _b: number = frac1 - pf, _c: number = pf;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      pf = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    pkSlot1 = pfRing_Idx;
    pkVal1 = pf;
    if (pfRing_Idx === 0) {
      pfHi = pf;
      pfLo = pf;
    } else {
      if (pf > pfHi) {
        pfHi = pf;
      }
      if (pf < pfLo) {
        pfLo = pf;
      }
    }
    highest = pfHi;
    lowest = pfLo;
    if (pfRing_Idx < sp.lastIdx) {
      tempReal = sp.cb_pfSufHi[pfRing_Idx + 1];
      if (tempReal > highest) {
        highest = tempReal;
      }
      tempReal = sp.cb_pfSufLo[pfRing_Idx + 1];
      if (tempReal < lowest) {
        lowest = tempReal;
      }
    }
    pfRing_Idx = pfRing_Idx + 1;
    if (pfRing_Idx > sp.maxIdx_pfRing) {
      pfRing_Idx = 0;
    }
    if (pfRing_Idx === 0) {
      sufHi = (sp.lastIdx !== pkSlot1) ? sp.cb_pfRing[sp.lastIdx] : pkVal1;
      sufLo = sufHi;
      i = sp.lastIdx;
      while (i > 0) {
        i -= 1;
        tempReal = (i !== pkSlot1) ? sp.cb_pfRing[i] : pkVal1;
        if (tempReal > sufHi) {
          sufHi = tempReal;
        }
        if (tempReal < sufLo) {
          sufLo = tempReal;
        }
      }
    }
    range = highest - lowest;
    if (range > 0.0) {
      frac2 = (pf - lowest) / range * 100.0;
    }
    {
      const _a: number = 0.5, _b: number = frac2 - pff, _c: number = pff;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      pff = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    cur_outReal = pff;
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
  clone(): StcStream {
    const c = new StcStream(this.core);
    c.optInFastPeriod = this.optInFastPeriod;
    c.optInSlowPeriod = this.optInSlowPeriod;
    c.optInCyclePeriod = this.optInCyclePeriod;
    c.prevFast = this.prevFast;
    c.prevSlow = this.prevSlow;
    c.fastK = this.fastK;
    c.slowK = this.slowK;
    c.fastBeta = this.fastBeta;
    c.slowBeta = this.slowBeta;
    c.frac1 = this.frac1;
    c.frac2 = this.frac2;
    c.pf = this.pf;
    c.pff = this.pff;
    c.lineHi = this.lineHi;
    c.lineLo = this.lineLo;
    c.pfHi = this.pfHi;
    c.pfLo = this.pfLo;
    c.lastIdx = this.lastIdx;
    c.lineRing_Idx = this.lineRing_Idx;
    c.pfRing_Idx = this.pfRing_Idx;
    c.maxIdx_lineRing = this.maxIdx_lineRing;
    c.lineSufHi_Idx = this.lineSufHi_Idx;
    c.maxIdx_lineSufHi = this.maxIdx_lineSufHi;
    c.lineSufLo_Idx = this.lineSufLo_Idx;
    c.maxIdx_lineSufLo = this.maxIdx_lineSufLo;
    c.maxIdx_pfRing = this.maxIdx_pfRing;
    c.pfSufHi_Idx = this.pfSufHi_Idx;
    c.maxIdx_pfSufHi = this.maxIdx_pfSufHi;
    c.pfSufLo_Idx = this.pfSufLo_Idx;
    c.maxIdx_pfSufLo = this.maxIdx_pfSufLo;
    c.cbSize_lineRing = this.cbSize_lineRing;
    c.cb_lineRing = this.cb_lineRing.slice();
    c.cbSize_lineSufHi = this.cbSize_lineSufHi;
    c.cb_lineSufHi = this.cb_lineSufHi.slice();
    c.cbSize_lineSufLo = this.cbSize_lineSufLo;
    c.cb_lineSufLo = this.cb_lineSufLo.slice();
    c.cbSize_pfRing = this.cbSize_pfRing;
    c.cb_pfRing = this.cb_pfRing.slice();
    c.cbSize_pfSufHi = this.cbSize_pfSufHi;
    c.cb_pfSufHi = this.cb_pfSufHi.slice();
    c.cbSize_pfSufLo = this.cbSize_pfSufLo;
    c.cb_pfSufLo = this.cb_pfSufLo.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function stcStepImpl(sp: StcStream, inReal: number): void {
  let tempReal = 0;
  let lineValue = 0;
  let lowest = 0;
  let highest = 0;
  let range = 0;
  let sufHi = 0;
  let sufLo = 0;
  let i = 0;
  tempReal = inReal;
  {
    const _a: number = sp.fastBeta, _b: number = sp.prevFast, _c: number = sp.fastK * tempReal;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.prevFast = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  {
    const _a: number = sp.slowBeta, _b: number = sp.prevSlow, _c: number = sp.slowK * tempReal;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.prevSlow = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  lineValue = sp.prevFast - sp.prevSlow;
  sp.cb_lineRing[sp.lineRing_Idx] = lineValue;
  if (sp.lineRing_Idx === 0) {
    sp.lineHi = lineValue;
    sp.lineLo = lineValue;
  } else {
    if (lineValue > sp.lineHi) {
      sp.lineHi = lineValue;
    }
    if (lineValue < sp.lineLo) {
      sp.lineLo = lineValue;
    }
  }
  highest = sp.lineHi;
  lowest = sp.lineLo;
  if (sp.lineRing_Idx < sp.lastIdx) {
    tempReal = sp.cb_lineSufHi[sp.lineRing_Idx + 1];
    if (tempReal > highest) {
      highest = tempReal;
    }
    tempReal = sp.cb_lineSufLo[sp.lineRing_Idx + 1];
    if (tempReal < lowest) {
      lowest = tempReal;
    }
  }
  sp.lineRing_Idx = sp.lineRing_Idx + 1;
  if (sp.lineRing_Idx > sp.maxIdx_lineRing) {
    sp.lineRing_Idx = 0;
  }
  if (sp.lineRing_Idx === 0) {
    sufHi = sp.cb_lineRing[sp.lastIdx];
    sufLo = sufHi;
    sp.cb_lineSufHi[sp.lastIdx] = sufHi;
    sp.cb_lineSufLo[sp.lastIdx] = sufLo;
    i = sp.lastIdx;
    while (i > 0) {
      i -= 1;
      tempReal = sp.cb_lineRing[i];
      if (tempReal > sufHi) {
        sufHi = tempReal;
      }
      if (tempReal < sufLo) {
        sufLo = tempReal;
      }
      sp.cb_lineSufHi[i] = sufHi;
      sp.cb_lineSufLo[i] = sufLo;
    }
  }
  range = highest - lowest;
  if (range > 0.0) {
    sp.frac1 = (lineValue - lowest) / range * 100.0;
  }
  {
    const _a: number = 0.5, _b: number = sp.frac1 - sp.pf, _c: number = sp.pf;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.pf = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  sp.cb_pfRing[sp.pfRing_Idx] = sp.pf;
  if (sp.pfRing_Idx === 0) {
    sp.pfHi = sp.pf;
    sp.pfLo = sp.pf;
  } else {
    if (sp.pf > sp.pfHi) {
      sp.pfHi = sp.pf;
    }
    if (sp.pf < sp.pfLo) {
      sp.pfLo = sp.pf;
    }
  }
  highest = sp.pfHi;
  lowest = sp.pfLo;
  if (sp.pfRing_Idx < sp.lastIdx) {
    tempReal = sp.cb_pfSufHi[sp.pfRing_Idx + 1];
    if (tempReal > highest) {
      highest = tempReal;
    }
    tempReal = sp.cb_pfSufLo[sp.pfRing_Idx + 1];
    if (tempReal < lowest) {
      lowest = tempReal;
    }
  }
  sp.pfRing_Idx = sp.pfRing_Idx + 1;
  if (sp.pfRing_Idx > sp.maxIdx_pfRing) {
    sp.pfRing_Idx = 0;
  }
  if (sp.pfRing_Idx === 0) {
    sufHi = sp.cb_pfRing[sp.lastIdx];
    sufLo = sufHi;
    sp.cb_pfSufHi[sp.lastIdx] = sufHi;
    sp.cb_pfSufLo[sp.lastIdx] = sufLo;
    i = sp.lastIdx;
    while (i > 0) {
      i -= 1;
      tempReal = sp.cb_pfRing[i];
      if (tempReal > sufHi) {
        sufHi = tempReal;
      }
      if (tempReal < sufLo) {
        sufLo = tempReal;
      }
      sp.cb_pfSufHi[i] = sufHi;
      sp.cb_pfSufLo[i] = sufLo;
    }
  }
  range = highest - lowest;
  if (range > 0.0) {
    sp.frac2 = (sp.pf - lowest) / range * 100.0;
  }
  {
    const _a: number = 0.5, _b: number = sp.frac2 - sp.pff, _c: number = sp.pff;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.pff = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  sp.cur_outReal = sp.pff;
}

function stcOpenImpl(
  sp: StcStream,
  inReal: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInCyclePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let lineRing!: Float64Array;
  let lineRing_Idx = 0;
  let maxIdx_lineRing = (30) - 1;
  let lineSufHi!: Float64Array;
  let lineSufHi_Idx = 0;
  let maxIdx_lineSufHi = (30) - 1;
  let lineSufLo!: Float64Array;
  let lineSufLo_Idx = 0;
  let maxIdx_lineSufLo = (30) - 1;
  let pfRing!: Float64Array;
  let pfRing_Idx = 0;
  let maxIdx_pfRing = (30) - 1;
  let pfSufHi!: Float64Array;
  let pfSufHi_Idx = 0;
  let maxIdx_pfSufHi = (30) - 1;
  let pfSufLo!: Float64Array;
  let pfSufLo_Idx = 0;
  let maxIdx_pfSufLo = (30) - 1;
  let prevFast = 0;
  let prevSlow = 0;
  let fastK = 0;
  let slowK = 0;
  let tempReal = 0;
  let lineValue = 0;
  let fastBeta = 0;
  let slowBeta = 0;
  let lowest = 0;
  let highest = 0;
  let range = 0;
  let frac1 = 0;
  let frac2 = 0;
  let pf = 0;
  let pff = 0;
  let lineHi = 0;
  let lineLo = 0;
  let pfHi = 0;
  let pfLo = 0;
  let sufHi = 0;
  let sufLo = 0;
  let i = 0;
  let today = 0;
  let fastToday = 0;
  let lineStart = 0;
  let outIdx = 0;
  let tempInteger = 0;
  let lookbackTotal = 0;
  let lookbackSlow = 0;
  let lastIdx = 0;
  let nLine = 0;
  let nPF = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInFastPeriod === Core.INTEGER_DEFAULT) {
    optInFastPeriod = 23;
  } else if (optInFastPeriod < 2 || optInFastPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowPeriod === Core.INTEGER_DEFAULT) {
    optInSlowPeriod = 50;
  } else if (optInSlowPeriod < 2 || optInSlowPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInCyclePeriod === Core.INTEGER_DEFAULT) {
    optInCyclePeriod = 10;
  } else if (optInCyclePeriod < 2 || optInCyclePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (optInSlowPeriod < optInFastPeriod) {
    tempInteger = optInSlowPeriod;
    optInSlowPeriod = optInFastPeriod;
    optInFastPeriod = tempInteger;
  }
  lookbackTotal = stcLookbackPositional(optInFastPeriod, optInSlowPeriod, optInCyclePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outBegIdx.value = startIdx;
  fastBeta = (optInFastPeriod - 1) / (optInFastPeriod + 1);
  fastK = 1.0 - fastBeta;
  if (fastBeta < 0.5) {
    fastBeta = 1.0 - fastK;
  }
  slowBeta = (optInSlowPeriod - 1) / (optInSlowPeriod + 1);
  slowK = 1.0 - slowBeta;
  if (slowBeta < 0.5) {
    slowBeta = 1.0 - slowK;
  }
  /* Rolling extrema, van Herk / Gil-Werman: the window ending in slot j is
   * the current block's prefix extremum joined with the previous block's
   * suffix extremum from slot j+1. The extrema are exact, so the output must
   * stay bit-identical to a full rescan of each window.
   */
  if (optInCyclePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  lineRing = new Float64Array(optInCyclePeriod);
  maxIdx_lineRing = optInCyclePeriod - 1;
  lineRing_Idx = 0;
  if (optInCyclePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  lineSufHi = new Float64Array(optInCyclePeriod);
  maxIdx_lineSufHi = optInCyclePeriod - 1;
  lineSufHi_Idx = 0;
  if (optInCyclePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  lineSufLo = new Float64Array(optInCyclePeriod);
  maxIdx_lineSufLo = optInCyclePeriod - 1;
  lineSufLo_Idx = 0;
  if (optInCyclePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  pfRing = new Float64Array(optInCyclePeriod);
  maxIdx_pfRing = optInCyclePeriod - 1;
  pfRing_Idx = 0;
  if (optInCyclePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  pfSufHi = new Float64Array(optInCyclePeriod);
  maxIdx_pfSufHi = optInCyclePeriod - 1;
  pfSufHi_Idx = 0;
  if (optInCyclePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  pfSufLo = new Float64Array(optInCyclePeriod);
  maxIdx_pfSufLo = optInCyclePeriod - 1;
  pfSufLo_Idx = 0;
  lastIdx = optInCyclePeriod - 1;
  /* The line is TA_MACD's: each SMA seed placed by its own EMA lookback, then
   * both EMAs advanced to lineStart, so that from lineStart on it is
   * TA_EMA(fast) - TA_EMA(slow) bit for bit. That placement reads out of
   * bounds or skips bars unless ema_lookback(n) - n never decreases as n
   * grows. The chain is fed from
   * lineStart, not from the EMA seed: TA_FUNC_UNST_EMA then reaches only the
   * line, while TA_FUNC_UNST_STC moves the whole chain back and so warms the
   * line and both smoothers.
   */
  lookbackSlow = emaLookbackPositional(optInSlowPeriod, core);
  lineStart = startIdx - (lookbackTotal - lookbackSlow);
  today = startIdx - lookbackTotal;
  tempReal = 0.0;
  i = optInSlowPeriod;
  while (i-- > 0) {
    tempReal += inReal[today++];
  }
  prevSlow = tempReal / optInSlowPeriod;
  fastToday = startIdx - lookbackTotal + (lookbackSlow - emaLookbackPositional(optInFastPeriod, core));
  prevFast = 0.0;
  i = optInFastPeriod;
  while (i-- > 0) {
    prevFast += inReal[fastToday++];
  }
  prevFast = prevFast / optInFastPeriod;
  while (today < fastToday) {
    tempReal = inReal[today++];
    {
      const _a: number = slowBeta, _b: number = prevSlow, _c: number = slowK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevSlow = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
  }
  while (today <= lineStart) {
    tempReal = inReal[today++];
    {
      const _a: number = fastBeta, _b: number = prevFast, _c: number = fastK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevFast = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = slowBeta, _b: number = prevSlow, _c: number = slowK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevSlow = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
  }
  /* A zero range holds the previous fraction (0.0 before any), and the test
   * is exact: in a sustained trend PF saturates at 100 and the second
   * range reaches exactly 0 while the output must stay at 100.
   */
  frac1 = 0.0;
  frac2 = 0.0;
  pf = 0.0;
  pff = 0.0;
  pfHi = 0.0;
  pfLo = 0.0;
  nPF = 0;
  lineValue = prevFast - prevSlow;
  lineRing[lineRing_Idx] = lineValue;
  lineRing_Idx++;
  if (lineRing_Idx > maxIdx_lineRing) {
    lineRing_Idx = 0;
  }
  lineHi = lineValue;
  lineLo = lineValue;
  nLine = 1;
  /* Warm-up, through startIdx inclusive. Each stage starts once its
   * window is full, and each smoother is seeded on its first input.
   */
  while (today <= startIdx) {
    tempReal = inReal[today];
    {
      const _a: number = fastBeta, _b: number = prevFast, _c: number = fastK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevFast = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = slowBeta, _b: number = prevSlow, _c: number = slowK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevSlow = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    lineValue = prevFast - prevSlow;
    nLine = nLine + 1;
    lineRing[lineRing_Idx] = lineValue;
    if (lineRing_Idx === 0) {
      lineHi = lineValue;
      lineLo = lineValue;
    } else {
      if (lineValue > lineHi) {
        lineHi = lineValue;
      }
      if (lineValue < lineLo) {
        lineLo = lineValue;
      }
    }
    highest = lineHi;
    lowest = lineLo;
    if (nLine >= optInCyclePeriod && lineRing_Idx < lastIdx) {
      tempReal = lineSufHi[lineRing_Idx + 1];
      if (tempReal > highest) {
        highest = tempReal;
      }
      tempReal = lineSufLo[lineRing_Idx + 1];
      if (tempReal < lowest) {
        lowest = tempReal;
      }
    }
    lineRing_Idx++;
    if (lineRing_Idx > maxIdx_lineRing) {
      lineRing_Idx = 0;
    }
    if (lineRing_Idx === 0) {
      sufHi = lineRing[lastIdx];
      sufLo = sufHi;
      lineSufHi[lastIdx] = sufHi;
      lineSufLo[lastIdx] = sufLo;
      i = lastIdx;
      while (i > 0) {
        i -= 1;
        tempReal = lineRing[i];
        if (tempReal > sufHi) {
          sufHi = tempReal;
        }
        if (tempReal < sufLo) {
          sufLo = tempReal;
        }
        lineSufHi[i] = sufHi;
        lineSufLo[i] = sufLo;
      }
    }
    if (nLine >= optInCyclePeriod) {
      range = highest - lowest;
      if (range > 0.0) {
        frac1 = (lineValue - lowest) / range * 100.0;
      }
      if (nPF === 0) {
        pf = frac1;
      } else {
        {
          const _a: number = 0.5, _b: number = frac1 - pf, _c: number = pf;
          const _p = _a * _b;
          let _t = 134217729 * _a;
          const _ah = _t - (_t - _a), _al = _a - _ah;
          _t = 134217729 * _b;
          const _bh = _t - (_t - _b), _bl = _b - _bh;
          const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
          const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
          const _v = _tl + _pl, _v2 = _v + _v;
          const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
          pf = _w && (_pl === 0 || _v === 0) ? _th
            : _w && _v2 !== (_th + _v2) - _th ? _th + _v
            : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
        }
      }
      nPF = nPF + 1;
      pfRing[pfRing_Idx] = pf;
      if (pfRing_Idx === 0) {
        pfHi = pf;
        pfLo = pf;
      } else {
        if (pf > pfHi) {
          pfHi = pf;
        }
        if (pf < pfLo) {
          pfLo = pf;
        }
      }
      highest = pfHi;
      lowest = pfLo;
      if (nPF >= optInCyclePeriod && pfRing_Idx < lastIdx) {
        tempReal = pfSufHi[pfRing_Idx + 1];
        if (tempReal > highest) {
          highest = tempReal;
        }
        tempReal = pfSufLo[pfRing_Idx + 1];
        if (tempReal < lowest) {
          lowest = tempReal;
        }
      }
      pfRing_Idx++;
      if (pfRing_Idx > maxIdx_pfRing) {
        pfRing_Idx = 0;
      }
      if (pfRing_Idx === 0) {
        sufHi = pfRing[lastIdx];
        sufLo = sufHi;
        pfSufHi[lastIdx] = sufHi;
        pfSufLo[lastIdx] = sufLo;
        i = lastIdx;
        while (i > 0) {
          i -= 1;
          tempReal = pfRing[i];
          if (tempReal > sufHi) {
            sufHi = tempReal;
          }
          if (tempReal < sufLo) {
            sufLo = tempReal;
          }
          pfSufHi[i] = sufHi;
          pfSufLo[i] = sufLo;
        }
      }
      if (nPF >= optInCyclePeriod) {
        range = highest - lowest;
        if (range > 0.0) {
          frac2 = (pf - lowest) / range * 100.0;
        }
        if (nPF === optInCyclePeriod) {
          pff = frac2;
        } else {
          {
            const _a: number = 0.5, _b: number = frac2 - pff, _c: number = pff;
            const _p = _a * _b;
            let _t = 134217729 * _a;
            const _ah = _t - (_t - _a), _al = _a - _ah;
            _t = 134217729 * _b;
            const _bh = _t - (_t - _b), _bl = _b - _bh;
            const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
            const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
            const _v = _tl + _pl, _v2 = _v + _v;
            const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
            pff = _w && (_pl === 0 || _v === 0) ? _th
              : _w && _v2 !== (_th + _v2) - _th ? _th + _v
              : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
          }
        }
      }
    }
    today = today + 1;
  }
  outReal[((0 * outStride) | 0)] = pff;
  outIdx = 1;
  while (today <= endIdx) {
    tempReal = inReal[today];
    {
      const _a: number = fastBeta, _b: number = prevFast, _c: number = fastK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevFast = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = slowBeta, _b: number = prevSlow, _c: number = slowK * tempReal;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      prevSlow = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    lineValue = prevFast - prevSlow;
    lineRing[lineRing_Idx] = lineValue;
    if (lineRing_Idx === 0) {
      lineHi = lineValue;
      lineLo = lineValue;
    } else {
      if (lineValue > lineHi) {
        lineHi = lineValue;
      }
      if (lineValue < lineLo) {
        lineLo = lineValue;
      }
    }
    highest = lineHi;
    lowest = lineLo;
    if (lineRing_Idx < lastIdx) {
      tempReal = lineSufHi[lineRing_Idx + 1];
      if (tempReal > highest) {
        highest = tempReal;
      }
      tempReal = lineSufLo[lineRing_Idx + 1];
      if (tempReal < lowest) {
        lowest = tempReal;
      }
    }
    lineRing_Idx++;
    if (lineRing_Idx > maxIdx_lineRing) {
      lineRing_Idx = 0;
    }
    if (lineRing_Idx === 0) {
      sufHi = lineRing[lastIdx];
      sufLo = sufHi;
      lineSufHi[lastIdx] = sufHi;
      lineSufLo[lastIdx] = sufLo;
      i = lastIdx;
      while (i > 0) {
        i -= 1;
        tempReal = lineRing[i];
        if (tempReal > sufHi) {
          sufHi = tempReal;
        }
        if (tempReal < sufLo) {
          sufLo = tempReal;
        }
        lineSufHi[i] = sufHi;
        lineSufLo[i] = sufLo;
      }
    }
    range = highest - lowest;
    if (range > 0.0) {
      frac1 = (lineValue - lowest) / range * 100.0;
    }
    {
      const _a: number = 0.5, _b: number = frac1 - pf, _c: number = pf;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      pf = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    pfRing[pfRing_Idx] = pf;
    if (pfRing_Idx === 0) {
      pfHi = pf;
      pfLo = pf;
    } else {
      if (pf > pfHi) {
        pfHi = pf;
      }
      if (pf < pfLo) {
        pfLo = pf;
      }
    }
    highest = pfHi;
    lowest = pfLo;
    if (pfRing_Idx < lastIdx) {
      tempReal = pfSufHi[pfRing_Idx + 1];
      if (tempReal > highest) {
        highest = tempReal;
      }
      tempReal = pfSufLo[pfRing_Idx + 1];
      if (tempReal < lowest) {
        lowest = tempReal;
      }
    }
    pfRing_Idx++;
    if (pfRing_Idx > maxIdx_pfRing) {
      pfRing_Idx = 0;
    }
    if (pfRing_Idx === 0) {
      sufHi = pfRing[lastIdx];
      sufLo = sufHi;
      pfSufHi[lastIdx] = sufHi;
      pfSufLo[lastIdx] = sufLo;
      i = lastIdx;
      while (i > 0) {
        i -= 1;
        tempReal = pfRing[i];
        if (tempReal > sufHi) {
          sufHi = tempReal;
        }
        if (tempReal < sufLo) {
          sufLo = tempReal;
        }
        pfSufHi[i] = sufHi;
        pfSufLo[i] = sufLo;
      }
    }
    range = highest - lowest;
    if (range > 0.0) {
      frac2 = (pf - lowest) / range * 100.0;
    }
    {
      const _a: number = 0.5, _b: number = frac2 - pff, _c: number = pff;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      pff = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    outReal[((outIdx++ * outStride) | 0)] = pff;
    today = today + 1;
  }
  outNBElement.value = outIdx;
  const capCb_lineRing = maxIdx_lineRing + 1;
  if (capCb_lineRing > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  const capCb_lineSufHi = maxIdx_lineSufHi + 1;
  if (capCb_lineSufHi > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  const capCb_lineSufLo = maxIdx_lineSufLo + 1;
  if (capCb_lineSufLo > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  const capCb_pfRing = maxIdx_pfRing + 1;
  if (capCb_pfRing > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  const capCb_pfSufHi = maxIdx_pfSufHi + 1;
  if (capCb_pfSufHi > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  const capCb_pfSufLo = maxIdx_pfSufLo + 1;
  if (capCb_pfSufLo > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sp.optInFastPeriod = optInFastPeriod;
  sp.optInSlowPeriod = optInSlowPeriod;
  sp.optInCyclePeriod = optInCyclePeriod;
  sp.prevFast = prevFast;
  sp.prevSlow = prevSlow;
  sp.fastK = fastK;
  sp.slowK = slowK;
  sp.fastBeta = fastBeta;
  sp.slowBeta = slowBeta;
  sp.frac1 = frac1;
  sp.frac2 = frac2;
  sp.pf = pf;
  sp.pff = pff;
  sp.lineHi = lineHi;
  sp.lineLo = lineLo;
  sp.pfHi = pfHi;
  sp.pfLo = pfLo;
  sp.lastIdx = lastIdx;
  sp.lineRing_Idx = lineRing_Idx;
  sp.pfRing_Idx = pfRing_Idx;
  sp.maxIdx_lineRing = maxIdx_lineRing;
  sp.lineSufHi_Idx = lineSufHi_Idx;
  sp.maxIdx_lineSufHi = maxIdx_lineSufHi;
  sp.lineSufLo_Idx = lineSufLo_Idx;
  sp.maxIdx_lineSufLo = maxIdx_lineSufLo;
  sp.maxIdx_pfRing = maxIdx_pfRing;
  sp.pfSufHi_Idx = pfSufHi_Idx;
  sp.maxIdx_pfSufHi = maxIdx_pfSufHi;
  sp.pfSufLo_Idx = pfSufLo_Idx;
  sp.maxIdx_pfSufLo = maxIdx_pfSufLo;
  sp.cbSize_lineRing = capCb_lineRing;
  sp.cb_lineRing = lineRing;
  sp.cbSize_lineSufHi = capCb_lineSufHi;
  sp.cb_lineSufHi = lineSufHi;
  sp.cbSize_lineSufLo = capCb_lineSufLo;
  sp.cb_lineSufLo = lineSufLo;
  sp.cbSize_pfRing = capCb_pfRing;
  sp.cb_pfRing = pfRing;
  sp.cbSize_pfSufHi = capCb_pfSufHi;
  sp.cb_pfSufHi = pfSufHi;
  sp.cbSize_pfSufLo = capCb_pfSufLo;
  sp.cb_pfSufLo = pfSufLo;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function stcOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInCyclePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): StcStream {
  const sp = StcStream.of(core);
  const retCode = stcOpenImpl(sp, inReal, startIdx, optInFastPeriod, optInSlowPeriod, optInCyclePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('STC openAndFill', inReal.length, startIdx, stcLookbackPositional(optInFastPeriod, optInSlowPeriod, optInCyclePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && stcLookbackPositional(optInFastPeriod, optInSlowPeriod, optInCyclePeriod, core) < 0) {
    throw rejectedParams('STC openAndFill', PARAM_RANGES, optInFastPeriod, optInSlowPeriod, optInCyclePeriod);
  }
  throw streamFailure('STC openAndFill', retCode);
}

/** @internal */
export function stcOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInCyclePeriod: number,
): StcStream {
  const sp = StcStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = stcOpenImpl(sp, inReal, startIdx, optInFastPeriod, optInSlowPeriod, optInCyclePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('STC open', inReal.length, startIdx, stcLookbackPositional(optInFastPeriod, optInSlowPeriod, optInCyclePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && stcLookbackPositional(optInFastPeriod, optInSlowPeriod, optInCyclePeriod, core) < 0) {
    throw rejectedParams('STC open', PARAM_RANGES, optInFastPeriod, optInSlowPeriod, optInCyclePeriod);
  }
  throw streamFailure('STC open', retCode);
}

/** @internal */
export function stcOpenPositional(
  inReal: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInCyclePeriod: number,
  core: Core = Core.DEFAULT,
): StcStream {
  optInFastPeriod = requireInt32('STC open', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('STC open', 'slowPeriod', optInSlowPeriod);
  optInCyclePeriod = requireInt32('STC open', 'cyclePeriod', optInCyclePeriod);
  requireArgument('STC open', 'real', inReal);
  requireHistory('STC open', 'real', inReal.length);
  return stcOpenInternal(core, float64(inReal), 0, optInFastPeriod, optInSlowPeriod, optInCyclePeriod);
}

/** @internal */
export function stcOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInCyclePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): StcStream {
  optInFastPeriod = requireInt32('STC openAndFill', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('STC openAndFill', 'slowPeriod', optInSlowPeriod);
  optInCyclePeriod = requireInt32('STC openAndFill', 'cyclePeriod', optInCyclePeriod);
  requireArgument('STC openAndFill', 'real', inReal);
  requireHistory('STC openAndFill', 'real', inReal.length);
  const guardLookback = stcLookbackPositional(optInFastPeriod, optInSlowPeriod, optInCyclePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('STC openAndFill', PARAM_RANGES, optInFastPeriod, optInSlowPeriod, optInCyclePeriod);
  }
  const guardOutLen = openFillCount('STC openAndFill', inReal.length, guardLookback);
  requireOutput('STC openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('STC openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('STC openAndFill', 'outReal', 'real');
  }
  return stcOpenAndFillInternal(core, inRealF64, 0, optInFastPeriod, optInSlowPeriod, optInCyclePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live STC stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link stc} there.
 *
 * @param real - Warm-up history. Input series (typically close)
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link StcOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `stcLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { stcOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = stcOpen(data, {
 *   fastPeriod: 23,
 *   slowPeriod: 50,
 *   cyclePeriod: 10,
 * });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function stcOpen(real: ArrayLike<number>, options?: StcOptions): StcStream {
  const o = optionsOf('STC open', options);
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const optInCyclePeriod = orDefault(o.cyclePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('STC open', o.core);
  return stcOpenPositional(real, optInFastPeriod, optInSlowPeriod, optInCyclePeriod, core);
}

/**
 * {@link stcOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link stc}.
 *
 * @param real - Warm-up history. Input series (typically close)
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link StcOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link stc} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `stcLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or a
 *   buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { stcOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = stcOpenAndFill(data, {
 *   fastPeriod: 23,
 *   slowPeriod: 50,
 *   cyclePeriod: 10,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function stcOpenAndFill(real: ArrayLike<number>, options?: StcOptions): StcResult & { stream: StcStream } {
  const o = optionsOf('STC openAndFill', options);
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const optInCyclePeriod = orDefault(o.cyclePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('STC openAndFill', o.core);
  const capacity = fillCapacity(stcLookbackPositional(optInFastPeriod, optInSlowPeriod, optInCyclePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = stcOpenAndFillPositional(real, optInFastPeriod, optInSlowPeriod, optInCyclePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
