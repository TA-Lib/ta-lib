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
 *  AA       Andrew Atkinson
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  112400 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  062804 MF     Resolve div by zero bug on limit case.
 *  020605 AA     Fix #1117666 Lookback & out-of-bound bug.
 *  071126 MF,CC  Rewrite the combine into flat error-guards and a single-cursor
 *                offset index (offset = fastNb - *outNBElement). Bit-identical,
 *                streamable, and index-safe.
 *  092826 MF,CC  #459 fuse the fast and slow SMA into one pass over the input:
 *                two running sums, no intermediate buffer, no allocation.
 *                Bit-identical.
 *  092826 MF,CC  Fuse the fast and slow EMA into one pass (#459).
 */

import { MAType, MATypes } from '../enums.ts';
import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { emaLookbackPositional } from './ema.ts';
import { MaStream, maLookbackPositional, maOpenAndFillInternal, maPositional } from './ma.ts';

/** @internal */
export function apoLookbackPositional(optInFastPeriod: number, optInSlowPeriod: number, optInMAType: MAType | typeof Core.INTEGER_DEFAULT, core: Core = Core.DEFAULT): number {
  if ((optInFastPeriod | 0) !== optInFastPeriod) {
    return -1;
  }
  if ((optInSlowPeriod | 0) !== optInSlowPeriod) {
    return -1;
  }
  if ((optInMAType | 0) !== optInMAType) {
    return -1;
  }
  if (optInFastPeriod === Core.INTEGER_DEFAULT) {
    optInFastPeriod = 12;
  } else if (optInFastPeriod < 2 || optInFastPeriod > 100000) {
    return -1;
  }
  if (optInSlowPeriod === Core.INTEGER_DEFAULT) {
    optInSlowPeriod = 26;
  } else if (optInSlowPeriod < 2 || optInSlowPeriod > 100000) {
    return -1;
  }
  if (optInMAType === Core.INTEGER_DEFAULT || optInMAType === MAType.DEFAULT) {
    optInMAType = MAType.EMA;
  } else if (optInMAType < MATypes.Min || optInMAType > MATypes.Max) {
    return -1;
  }
  /* The slow MA is the key factor determining the lookback period. */
  return maLookbackPositional((optInSlowPeriod > optInFastPeriod ? optInSlowPeriod : optInFastPeriod), (optInMAType as MAType), core);
}

/** @internal */
export function apoDisplayShiftPositional(optInFastPeriod: number, optInSlowPeriod: number, optInMAType: MAType | typeof Core.INTEGER_DEFAULT, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (apoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInMAType, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function apoImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInMAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let tempBuffer!: Float64Array;
  let tempInteger = 0;
  const fastBeg = new MInteger();
  const fastNb = new MInteger();
  let offset = 0;
  let i = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInFastPeriod === Core.INTEGER_DEFAULT) {
    optInFastPeriod = 12;
  } else if (optInFastPeriod < 2 || optInFastPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowPeriod === Core.INTEGER_DEFAULT) {
    optInSlowPeriod = 26;
  } else if (optInSlowPeriod < 2 || optInSlowPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInMAType === Core.INTEGER_DEFAULT || optInMAType === MAType.DEFAULT) {
    optInMAType = MAType.EMA;
  } else if (optInMAType < MATypes.Min || optInMAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  /* Nothing to produce: the range ends before the lookback. Return before
   * touching anything.
   *
   * Without this the fast MA below runs first, and its lookback is SMALLER
   * than apo's own — so it reads the whole range and computes a result the
   * empty slow MA then discards. Observably identical (the slow MA's own early
   * return already yields 0,0 here), but it is the difference between "a range
   * that ends before the lookback reads nothing" being true of this function and
   * being false: with a caller-supplied inReal that stops short of endIdx, that
   * discarded work is an out-of-bounds read. Pinned by the zero-length no-I/O
   * probe over every guarded core.
   */
  if (maLookbackPositional((optInSlowPeriod > optInFastPeriod ? optInSlowPeriod : optInFastPeriod), (optInMAType as MAType), core) > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  if (optInMAType === MAType.SMA) {
    /* SMA fast path: the fast window is the newest optInFastPeriod bars of the
     * slow one, so ONE pass over the input serves both moving averages - two
     * running sums, no intermediate buffer and no allocation, where the general
     * path below makes two passes and allocates the fast MA in full.
     *
     * Bit-identical to that path. Each sum sees exactly the add/subtract
     * sequence TA_SMA gives it at its own period, starting from its own first
     * output bar - which is why the fast sum is walked alone over the bars the
     * slow MA does not reach (its running total is path-dependent, so arriving
     * at the first output bar by a shorter route would change the low bits) -
     * and each quotient is formed as sma.c forms it: the total AFTER adding the
     * new bar and BEFORE dropping the trailing one, divided by the period.
     *
     * inReal may alias outReal, as it may in the general path. outReal[_outIdx]
     * is written at bar _i with _outIdx <= _i-optInSlowPeriod+1 <= both trailing
     * indices, and both trailing bars are read before that write, so no bar is
     * overwritten before its last read.
     *
     * Every read is inside [0, endIdx]: the guard above leaves the slow
     * lookback no greater than endIdx, and the public tier rejects
     * endIdx < startIdx, so _slowStart <= endIdx and the seeding loops stop
     * one bar below it. There is nothing left for an empty-output arm to
     * catch, which is why this path has none.
     */
    let _fastTotal = 0;
    let _slowTotal = 0;
    let _fastValue = 0;
    let _slowValue = 0;
    let _i = 0;
    let _j = 0;
    let _outIdx = 0;
    let _fastStart = 0;
    let _slowStart = 0;
    let _fastTrailing = 0;
    let _slowTrailing = 0;
    /* Make sure slow is really slower than the fast period! if not, swap... */
    if (optInSlowPeriod < optInFastPeriod) {
      tempInteger = optInSlowPeriod;
      optInSlowPeriod = optInFastPeriod;
      optInFastPeriod = tempInteger;
    }
    _fastStart = optInFastPeriod - 1;
    if (_fastStart < startIdx) {
      _fastStart = startIdx;
    }
    _slowStart = optInSlowPeriod - 1;
    if (_slowStart < startIdx) {
      _slowStart = startIdx;
    }
    _fastTrailing = _fastStart - (optInFastPeriod - 1);
    _fastTotal = 0.0;
    for (_j = _fastTrailing; _j < _fastStart; _j += 1) {
      _fastTotal += inReal[_j];
    }
    _slowTrailing = _slowStart - (optInSlowPeriod - 1);
    _slowTotal = 0.0;
    for (_j = _slowTrailing; _j < _slowStart; _j += 1) {
      _slowTotal += inReal[_j];
    }
    /* The bars the fast MA has and the slow one does not: advance the fast sum
     * alone. No output, but the sum must arrive at _slowStart along the same
     * path TA_SMA would have taken.
     */
    for (_i = _fastStart; _i < _slowStart; _i += 1) {
      _fastTotal += inReal[_i];
      _fastTotal -= inReal[_fastTrailing];
      _fastTrailing += 1;
    }
    _outIdx = 0;
    for (_i = _slowStart; _i <= endIdx; _i += 1) {
      _fastTotal += inReal[_i];
      _fastValue = _fastTotal;
      _fastTotal -= inReal[_fastTrailing];
      _fastTrailing += 1;
      _slowTotal += inReal[_i];
      _slowValue = _slowTotal;
      _slowTotal -= inReal[_slowTrailing];
      _slowTrailing += 1;
      outReal[_outIdx] = _fastValue / optInFastPeriod - _slowValue / optInSlowPeriod;
      _outIdx += 1;
    }
    outBegIdx.value = _slowStart;
    outNBElement.value = _outIdx;
    return RetCode.SUCCESS;
  }
  if (optInMAType === MAType.EMA) {
    /* EMA fast path: both recursions in one loop, no buffer. Bit-identical to
     * the general path only while each EMA is seeded at its OWN lookback and
     * keeps ema.c's recursion spelling: the fast EMA starts earlier than the
     * slow one, and a shared seed bar would change every output.
     */
    let _eFastK = 0;
    let _eSlowK = 0;
    let _eFast = 0;
    let _eSlow = 0;
    let _eX = 0;
    let _eFastBeta = 0;
    let _eSlowBeta = 0;
    let _eN = 0;
    let _eToday = 0;
    let _eFastToday = 0;
    let _eSlowToday = 0;
    let _eSlowStart = 0;
    let _eOutIdx = 0;
    if (optInSlowPeriod < optInFastPeriod) {
      tempInteger = optInSlowPeriod;
      optInSlowPeriod = optInFastPeriod;
      optInFastPeriod = tempInteger;
    }
    _eFastBeta = (optInFastPeriod - 1) / (optInFastPeriod + 1);
    _eFastK = 1.0 - _eFastBeta;
    if (_eFastBeta < 0.5) {
      _eFastBeta = 1.0 - _eFastK;
    }
    _eSlowBeta = (optInSlowPeriod - 1) / (optInSlowPeriod + 1);
    _eSlowK = 1.0 - _eSlowBeta;
    if (_eSlowBeta < 0.5) {
      _eSlowBeta = 1.0 - _eSlowK;
    }
    _eFastToday = emaLookbackPositional(optInFastPeriod, core);
    if (_eFastToday < startIdx) {
      _eFastToday = startIdx;
    }
    _eFastToday -= emaLookbackPositional(optInFastPeriod, core);
    _eSlowStart = emaLookbackPositional(optInSlowPeriod, core);
    if (_eSlowStart < startIdx) {
      _eSlowStart = startIdx;
    }
    _eSlowToday = _eSlowStart - emaLookbackPositional(optInSlowPeriod, core);
    _eFast = 0.0;
    for (_eN = 0; _eN < optInFastPeriod; _eN += 1) {
      _eFast += inReal[_eFastToday++];
    }
    _eFast = _eFast / optInFastPeriod;
    while (_eFastToday <= _eSlowStart) {
      {
        const _a: number = _eFastBeta, _b: number = _eFast, _c: number = _eFastK * inReal[_eFastToday++];
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        _eFast = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
    }
    _eSlow = 0.0;
    for (_eN = 0; _eN < optInSlowPeriod; _eN += 1) {
      _eSlow += inReal[_eSlowToday++];
    }
    _eSlow = _eSlow / optInSlowPeriod;
    while (_eSlowToday <= _eSlowStart) {
      {
        const _a: number = _eSlowBeta, _b: number = _eSlow, _c: number = _eSlowK * inReal[_eSlowToday++];
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        _eSlow = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
    }
    _eOutIdx = 0;
    outReal[_eOutIdx] = _eFast - _eSlow;
    _eOutIdx += 1;
    _eToday = _eSlowStart + 1;
    while (_eToday <= endIdx) {
      _eX = inReal[_eToday++];
      {
        const _a: number = _eFastBeta, _b: number = _eFast, _c: number = _eFastK * _eX;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        _eFast = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      {
        const _a: number = _eSlowBeta, _b: number = _eSlow, _c: number = _eSlowK * _eX;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        _eSlow = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      outReal[_eOutIdx] = _eFast - _eSlow;
      _eOutIdx += 1;
    }
    outBegIdx.value = _eSlowStart;
    outNBElement.value = _eOutIdx;
    return RetCode.SUCCESS;
  }
  /* Allocate an intermediate buffer. */
  tempBuffer = new Float64Array(endIdx - startIdx + 1);
  /* Make sure slow is really slower than
   * the fast period! if not, swap...
   */
  if (optInSlowPeriod < optInFastPeriod) {
    /* swap */
    tempInteger = optInSlowPeriod;
    optInSlowPeriod = optInFastPeriod;
    optInFastPeriod = tempInteger;
  }
  /* Calculate the fast MA into the tempBuffer. */
  const _xr4 = maPositional(startIdx, endIdx, inReal, optInFastPeriod, (optInMAType as MAType), tempBuffer, core);
  fastBeg.value = _xr4.begIdx;
  fastNb.value = _xr4.count;
  /* Calculate the slow MA into the output. */
  const _xr5 = maPositional(startIdx, endIdx, inReal, optInSlowPeriod, (optInMAType as MAType), outReal, core);
  outBegIdx.value = _xr5.begIdx;
  outNBElement.value = _xr5.count;
  /* fastNb - *outNBElement == slowBeg - fastBeg (the fast MA has at least as
   * many outputs), so tempBuffer[i+offset] is the fast MA at the same bar as
   * outReal[i], with a non-negative index. An empty slow MA skips the loop.
   */
  offset = fastNb.value - outNBElement.value;
  /* Calculate (fast MA)-(slow MA) in the output. */
  for (i = 0; i < outNBElement.value; i += 1) {
    outReal[i] = tempBuffer[i + offset] - outReal[i];
  }
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['fastPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['slowPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['maType', Core.INTEGER_DEFAULT, MATypes.Min, MATypes.Max, 'a MAType']] as const;

/** @internal */
export function apoPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInMAType: MAType | typeof Core.INTEGER_DEFAULT,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('APO', 'startIdx', startIdx);
  endIdx = requireInt32('APO', 'endIdx', endIdx);
  optInFastPeriod = requireInt32('APO', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('APO', 'slowPeriod', optInSlowPeriod);
  optInMAType = requireInt32('APO', 'maType', optInMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireIndexRange('APO', startIdx, endIdx);
  const guardLookback = apoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInMAType, core);
  if (guardLookback < 0) {
    throw rejectedParams('APO', PARAM_RANGES, optInFastPeriod, optInSlowPeriod, optInMAType);
  }
  const guardStart = clampedStart('APO', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('APO', 'real', inReal, guardInLen);
  requireOutput('APO', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('APO', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = apoImpl(core, startIdx, endIdx, inRealF64, optInFastPeriod, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('APO', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link apo}, {@link apoLookback}, {@link apoDisplayShift},
 * {@link apoOpen} and {@link apoOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface ApoOptions {
  /**
   * Period of the fast moving average (default 12; range 2 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  fastPeriod?: number | undefined;
  /**
   * Period of the slow moving average (default 26; range 2 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  slowPeriod?: number | undefined;
  /**
   * Moving-average type used for both MAs (default `MAType.EMA`; values: 0=SMA,
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
  outReal?: Float64Array<ArrayBuffer> | undefined;
}

/** What {@link apo} returns, and {@link apoOpenAndFill} with its stream. */
export interface ApoResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /** Fast MA minus slow MA. `values[k]` is the value at input bar `begIdx + k`. */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link apo} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link ApoOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function apoLookback(options?: ApoOptions): number {
  const o = optionsOf('APO lookback', options);
  return apoLookbackPositional(orDefault(o.fastPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowPeriod, Core.INTEGER_DEFAULT), orDefault(o.maType, Core.INTEGER_DEFAULT), coreOf('APO lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link apo}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link ApoOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function apoDisplayShift(outputIdx: number, options?: ApoOptions): number {
  const o = optionsOf('APO displayShift', options);
  return apoDisplayShiftPositional(orDefault(o.fastPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowPeriod, Core.INTEGER_DEFAULT), orDefault(o.maType, Core.INTEGER_DEFAULT), outputIdx, coreOf('APO displayShift', o.core));
}

/**
 * Absolute Price Oscillator: the difference between a fast and a slow moving
 * average of the input, in price units. Measures short- vs long-term momentum.
 * Positive when fast MA > slow MA (upward momentum); negative otherwise.
 *
 * Formula and more info at
 * [ta-lib.org/functions/apo](https://ta-lib.org/functions/apo).
 *
 * **Notes**
 * - `maType` applies to both the fast and slow moving average. `MAType.MAMA`
 *   ignores its period argument, so with `maType = MAType.MAMA` the fast and slow
 *   MAs are identical and the output is zero at every bar.
 * - If the slow period is set smaller than the fast period, the two are swapped,
 *   as in `MACD`.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link apoLookback} is a success with empty outputs.
 *
 * @param real - Source data series.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link ApoOptions}.
 * @returns `begIdx` and the values; see {@link ApoResult}. Each output is a view
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
 * import { MAType, apo } from 'ta-lib';
 *
 * const series = new Float64Array(252); // your data, oldest first
 *
 * const { begIdx, values } = apo(series, {
 *   fastPeriod: 12,
 *   slowPeriod: 26,
 *   maType: MAType.EMA,
 * });
 * console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
 * ```
 * @see [`ppo`](https://ta-lib.org/functions/ppo)
 * @see [`macd`](https://ta-lib.org/functions/macd)
 * @see [`ma`](https://ta-lib.org/functions/ma)
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`sma`](https://ta-lib.org/functions/sma)
 */
export function apo(real: ArrayLike<number>, options?: ApoOptions): ApoResult {
  const o = optionsOf('APO', options);
  if (o.endIdx === undefined) {
    requireSeries('APO', 'real', real);
  }
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const optInMAType = orDefault(o.maType, Core.INTEGER_DEFAULT);
  const core = coreOf('APO', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, apoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInMAType, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = apoPositional(startIdx, endIdx, real, optInFastPeriod, optInSlowPeriod, optInMAType, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live APO stream: one value per closed bar, bit-identical to {@link apo} over
 * the same series. Open it with {@link apoOpen} or {@link apoOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/apo](https://ta-lib.org/functions/apo).
 */
export class ApoStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInFastPeriod: number = 0;
  /** @internal */
  optInSlowPeriod: number = 0;
  /** @internal */
  optInMAType: number = 0;
  /** @internal */
  cur_outReal: number = 0;
  /** @internal */
  sub0!: MaStream;
  /** @internal */
  sub1!: MaStream;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): ApoStream {
    return new ApoStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link apo} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('APO advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: Source data series.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('APO update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('APO update', 'real');
    }
    apoStepImpl(this, real);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: Source data series.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number): number {
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('APO peek', 'real');
    }
    const sp = this;
    const inReal = real;
    let cur_tempBuffer = 0;
    let cur_outReal = 0;
    cur_tempBuffer = sp.sub0.peek(inReal);
    cur_outReal = sp.sub1.peek(inReal);
    cur_outReal = cur_tempBuffer - cur_outReal;
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
  clone(): ApoStream {
    const c = new ApoStream(this.core);
    c.optInFastPeriod = this.optInFastPeriod;
    c.optInSlowPeriod = this.optInSlowPeriod;
    c.optInMAType = this.optInMAType;
    c.cur_outReal = this.cur_outReal;
    c.sub0 = this.sub0.clone();
    c.sub1 = this.sub1.clone();
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function apoStepImpl(sp: ApoStream, inReal: number): void {
  let cur_tempBuffer = 0;
  let cur_outReal = 0;
  cur_tempBuffer = sp.sub0.update(inReal);
  cur_outReal = sp.sub1.update(inReal);
  cur_outReal = cur_tempBuffer - cur_outReal;
  sp.cur_outReal = cur_outReal;
}

function apoOpenImpl(
  sp: ApoStream,
  inReal: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInMAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let tempBuffer!: Float64Array;
  let tempInteger = 0;
  const fastBeg = new MInteger();
  const fastNb = new MInteger();
  let offset = 0;
  let i = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInFastPeriod === Core.INTEGER_DEFAULT) {
    optInFastPeriod = 12;
  } else if (optInFastPeriod < 2 || optInFastPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowPeriod === Core.INTEGER_DEFAULT) {
    optInSlowPeriod = 26;
  } else if (optInSlowPeriod < 2 || optInSlowPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInMAType === Core.INTEGER_DEFAULT || optInMAType === MAType.DEFAULT) {
    optInMAType = MAType.EMA;
  } else if (optInMAType < MATypes.Min || optInMAType > MATypes.Max) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen < apoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInMAType as MAType, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const sc_outReal = outStride === 1 ? outReal : new Float64Array(historyLen);
  /* Nothing to produce: the range ends before the lookback. Return before
   * touching anything.
   *
   * Without this the fast MA below runs first, and its lookback is SMALLER
   * than apo's own — so it reads the whole range and computes a result the
   * empty slow MA then discards. Observably identical (the slow MA's own early
   * return already yields 0,0 here), but it is the difference between "a range
   * that ends before the lookback reads nothing" being true of this function and
   * being false: with a caller-supplied inReal that stops short of endIdx, that
   * discarded work is an out-of-bounds read. Pinned by the zero-length no-I/O
   * probe over every guarded core.
   */
  if (maLookbackPositional((optInSlowPeriod > optInFastPeriod ? optInSlowPeriod : optInFastPeriod), (optInMAType as MAType), core) > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Allocate an intermediate buffer. */
  tempBuffer = new Float64Array(endIdx - startIdx + 1);
  /* Make sure slow is really slower than
   * the fast period! if not, swap...
   */
  if (optInSlowPeriod < optInFastPeriod) {
    /* swap */
    tempInteger = optInSlowPeriod;
    optInSlowPeriod = optInFastPeriod;
    optInFastPeriod = tempInteger;
  }
  /* Calculate the fast MA into the tempBuffer. */
  const sub0 = maOpenAndFillInternal(core, inReal, startIdx, optInFastPeriod, (optInMAType as MAType), fastBeg, fastNb, tempBuffer);
  /* Calculate the slow MA into the output. */
  const sub1 = maOpenAndFillInternal(core, inReal, startIdx, optInSlowPeriod, (optInMAType as MAType), outBegIdx, outNBElement, sc_outReal);
  /* fastNb - *outNBElement == slowBeg - fastBeg (the fast MA has at least as
   * many outputs), so tempBuffer[i+offset] is the fast MA at the same bar as
   * outReal[i], with a non-negative index. An empty slow MA skips the loop.
   */
  offset = fastNb.value - outNBElement.value;
  /* Calculate (fast MA)-(slow MA) in the output. */
  for (i = 0; i < outNBElement.value; i += 1) {
    sc_outReal[i] = tempBuffer[i + offset] - sc_outReal[i];
  }
  if (outNBElement.value < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  sp.optInFastPeriod = optInFastPeriod;
  sp.optInSlowPeriod = optInSlowPeriod;
  sp.optInMAType = optInMAType;
  sp.sub0 = sub0;
  sp.sub1 = sub1;
  sp.cur_outReal = sc_outReal[outNBElement.value - 1];
  return RetCode.SUCCESS;
}

/** @internal */
export function apoOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInMAType: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): ApoStream {
  const sp = ApoStream.of(core);
  const retCode = apoOpenImpl(sp, inReal, startIdx, optInFastPeriod, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('APO openAndFill', inReal.length, startIdx, apoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInMAType as MAType, core));
  }
  if (retCode === RetCode.BAD_PARAM && apoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInMAType as MAType, core) < 0) {
    throw rejectedParams('APO openAndFill', PARAM_RANGES, optInFastPeriod, optInSlowPeriod, optInMAType);
  }
  throw streamFailure('APO openAndFill', retCode);
}

/** @internal */
export function apoOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInMAType: number,
): ApoStream {
  const sp = ApoStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = apoOpenImpl(sp, inReal, startIdx, optInFastPeriod, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('APO open', inReal.length, startIdx, apoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInMAType as MAType, core));
  }
  if (retCode === RetCode.BAD_PARAM && apoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInMAType as MAType, core) < 0) {
    throw rejectedParams('APO open', PARAM_RANGES, optInFastPeriod, optInSlowPeriod, optInMAType);
  }
  throw streamFailure('APO open', retCode);
}

/** @internal */
export function apoOpenPositional(
  inReal: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInMAType: MAType | typeof Core.INTEGER_DEFAULT,
  core: Core = Core.DEFAULT,
): ApoStream {
  optInFastPeriod = requireInt32('APO open', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('APO open', 'slowPeriod', optInSlowPeriod);
  optInMAType = requireInt32('APO open', 'maType', optInMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('APO open', 'real', inReal);
  requireHistory('APO open', 'real', inReal.length);
  return apoOpenInternal(core, float64(inReal), 0, optInFastPeriod, optInSlowPeriod, optInMAType);
}

/** @internal */
export function apoOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  optInMAType: MAType | typeof Core.INTEGER_DEFAULT,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): ApoStream {
  optInFastPeriod = requireInt32('APO openAndFill', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('APO openAndFill', 'slowPeriod', optInSlowPeriod);
  optInMAType = requireInt32('APO openAndFill', 'maType', optInMAType) as MAType | typeof Core.INTEGER_DEFAULT;
  requireArgument('APO openAndFill', 'real', inReal);
  requireHistory('APO openAndFill', 'real', inReal.length);
  const guardLookback = apoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInMAType as MAType, core);
  if (guardLookback < 0) {
    throw rejectedParams('APO openAndFill', PARAM_RANGES, optInFastPeriod, optInSlowPeriod, optInMAType);
  }
  const guardOutLen = openFillCount('APO openAndFill', inReal.length, guardLookback);
  requireOutput('APO openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('APO openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('APO openAndFill', 'outReal', 'real');
  }
  return apoOpenAndFillInternal(core, inRealF64, 0, optInFastPeriod, optInSlowPeriod, optInMAType, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live APO stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link apo} there.
 *
 * @param real - Warm-up history. Source data series.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link ApoOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `apoLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { MAType, apoOpen } from 'ta-lib';
 *
 * const series = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = apoOpen(series, {
 *   fastPeriod: 12,
 *   slowPeriod: 26,
 *   maType: MAType.EMA,
 * });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function apoOpen(real: ArrayLike<number>, options?: ApoOptions): ApoStream {
  const o = optionsOf('APO open', options);
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const optInMAType = orDefault(o.maType, Core.INTEGER_DEFAULT);
  const core = coreOf('APO open', o.core);
  return apoOpenPositional(real, optInFastPeriod, optInSlowPeriod, optInMAType, core);
}

/**
 * {@link apoOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link apo}.
 *
 * @param real - Warm-up history. Source data series.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link ApoOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link apo} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `apoLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or a
 *   buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { MAType, apoOpenAndFill } from 'ta-lib';
 *
 * const series = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = apoOpenAndFill(series, {
 *   fastPeriod: 12,
 *   slowPeriod: 26,
 *   maType: MAType.EMA,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function apoOpenAndFill(real: ArrayLike<number>, options?: ApoOptions): ApoResult & { stream: ApoStream } {
  const o = optionsOf('APO openAndFill', options);
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const optInMAType = orDefault(o.maType, Core.INTEGER_DEFAULT);
  const core = coreOf('APO openAndFill', o.core);
  const capacity = fillCapacity(apoLookbackPositional(optInFastPeriod, optInSlowPeriod, optInMAType, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = apoOpenAndFillPositional(real, optInFastPeriod, optInSlowPeriod, optInMAType, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
