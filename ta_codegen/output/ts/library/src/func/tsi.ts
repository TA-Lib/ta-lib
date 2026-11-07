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
 *  090426 MF,CC  First version (#360).
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { emaLookbackPositional } from './ema.ts';

/** @internal */
export function tsiLookbackPositional(optInFirstPeriod: number, optInSecondPeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInFirstPeriod | 0) !== optInFirstPeriod) {
    return -1;
  }
  if ((optInSecondPeriod | 0) !== optInSecondPeriod) {
    return -1;
  }
  if (optInFirstPeriod === Core.INTEGER_DEFAULT) {
    optInFirstPeriod = 25;
  } else if (optInFirstPeriod < 2 || optInFirstPeriod > 100000) {
    return -1;
  }
  if (optInSecondPeriod === Core.INTEGER_DEFAULT) {
    optInSecondPeriod = 13;
  } else if (optInSecondPeriod < 2 || optInSecondPeriod > 100000) {
    return -1;
  }
  /* One bar forms the first close-to-close change, then the two EMA warm-ups
   * the pipeline stacks on it. Each term is exactly the lookback of the
   * function it comes from, which is also what makes TSI inherit
   * TA_FUNC_UNST_EMA from its callee.
   */
  return 1 + emaLookbackPositional(optInFirstPeriod, core) + emaLookbackPositional(optInSecondPeriod, core);
}

/** @internal */
export function tsiDisplayShiftPositional(optInFirstPeriod: number, optInSecondPeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (tsiLookbackPositional(optInFirstPeriod, optInSecondPeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function tsiImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInFirstPeriod: number,
  optInSecondPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let kFirst = 0;
  let kSecond = 0;
  let betaFirst = 0;
  let betaSecond = 0;
  let emaFirstNum = 0;
  let emaFirstDen = 0;
  let emaSecondNum = 0;
  let emaSecondDen = 0;
  let sumFirstNum = 0;
  let sumFirstDen = 0;
  let sumSecondNum = 0;
  let sumSecondDen = 0;
  let prevClose = 0;
  let mom = 0;
  let absMom = 0;
  let tsiValue = 0;
  let lookbackTotal = 0;
  let lookbackFirst = 0;
  let today = 0;
  let outIdx = 0;
  let nBar = 0;
  let nSecond = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInFirstPeriod === Core.INTEGER_DEFAULT) {
    optInFirstPeriod = 25;
  } else if (optInFirstPeriod < 2 || optInFirstPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSecondPeriod === Core.INTEGER_DEFAULT) {
    optInSecondPeriod = 13;
  } else if (optInSecondPeriod < 2 || optInSecondPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  lookbackTotal = tsiLookbackPositional(optInFirstPeriod, optInSecondPeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  outBegIdx.value = startIdx;
  /* Blau's double smoothing in one pass: the signed momentum and its
   * magnitude are carried through the same two EMA stages, then divided.
   *
   * Each stage seeds the way ema.c does -- a simple average of that stage's
   * first 'period' inputs -- so the result is bit-identical to TA_MOM(1)
   * followed by two TA_EMA calls on each chain. The stage boundary below is
   * the callee LOOKBACK, not (period-1), so that a warm
   * TA_SetUnstablePeriod(TA_FUNC_UNST_EMA) folds in: the second stage then
   * seeds from the values the first would have published. The seed sums
   * accumulate from 0.0 in production order and the recurrence is
   * k*x + beta*prev with ema.c's k and beta; do
   * not reorder or fuse them (0.0+x is not x for x=-0.0). That order IS the
   * bit-exactness contract against the composed reference.
   *
   * prevClose is carried in a scalar rather than re-read from inReal[t-1]
   * because outReal may alias inReal: the slot holding close[t-1] may already
   * hold an output written a bar earlier.
   */
  betaFirst = (optInFirstPeriod - 1) / (optInFirstPeriod + 1);
  kFirst = 1.0 - betaFirst;
  if (betaFirst < 0.5) {
    betaFirst = 1.0 - kFirst;
  }
  betaSecond = (optInSecondPeriod - 1) / (optInSecondPeriod + 1);
  kSecond = 1.0 - betaSecond;
  if (betaSecond < 0.5) {
    betaSecond = 1.0 - kSecond;
  }
  lookbackFirst = emaLookbackPositional(optInFirstPeriod, core);
  emaFirstNum = 0.0;
  emaFirstDen = 0.0;
  emaSecondNum = 0.0;
  emaSecondDen = 0.0;
  sumFirstNum = 0.0;
  sumFirstDen = 0.0;
  sumSecondNum = 0.0;
  sumSecondDen = 0.0;
  /* The first bar carrying a close-to-close change. */
  today = startIdx - lookbackTotal + 1;
  prevClose = inReal[today - 1];
  nBar = 0;
  /* Warm-up. Runs through startIdx inclusive: the last pass here completes
   * the second stage's seed, so it produces the first output.
   */
  while (today <= startIdx) {
    mom = inReal[today] - prevClose;
    prevClose = inReal[today];
    absMom = Math.abs(mom);
    /* Stage 1: the first EMA, over the raw momentum and its magnitude. */
    if (nBar < optInFirstPeriod) {
      sumFirstNum = sumFirstNum + mom;
      sumFirstDen = sumFirstDen + absMom;
      if (nBar === optInFirstPeriod - 1) {
        emaFirstNum = sumFirstNum / optInFirstPeriod;
        emaFirstDen = sumFirstDen / optInFirstPeriod;
      }
    } else {
      {
        const _a: number = betaFirst, _b: number = emaFirstNum, _c: number = kFirst * mom;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        emaFirstNum = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      {
        const _a: number = betaFirst, _b: number = emaFirstDen, _c: number = kFirst * absMom;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        emaFirstDen = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
    }
    /* Stage 2: the second EMA, over what stage 1 publishes.
     *
     * The stage counter is compared BEFORE it is subtracted, never after.
     * Writing this as `nSecond = nBar - lookbackFirst; if( nSecond >= 0 )`
     * is correct in C, where the counters are signed, and broken everywhere
     * else: the Rust backend renders them as usize, so the subtraction
     * underflows for the first lookbackFirst bars -- a panic in a debug
     * build and a wrap in release. It would also be invisible to the
     * cross-language gate, which runs release servers at unstable period 0,
     * where the branch the wrap wrongly takes happens to be a no-op because
     * both accumulators are still 0.0. smi.c states the same rule.
     */
    if (nBar >= lookbackFirst) {
      nSecond = nBar - lookbackFirst;
      if (nSecond < optInSecondPeriod) {
        sumSecondNum = sumSecondNum + emaFirstNum;
        sumSecondDen = sumSecondDen + emaFirstDen;
        if (nSecond === optInSecondPeriod - 1) {
          emaSecondNum = sumSecondNum / optInSecondPeriod;
          emaSecondDen = sumSecondDen / optInSecondPeriod;
        }
      } else {
        {
          const _a: number = betaSecond, _b: number = emaSecondNum, _c: number = kSecond * emaFirstNum;
          const _p = _a * _b;
          let _t = 134217729 * _a;
          const _ah = _t - (_t - _a), _al = _a - _ah;
          _t = 134217729 * _b;
          const _bh = _t - (_t - _b), _bl = _b - _bh;
          const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
          const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
          const _v = _tl + _pl, _v2 = _v + _v;
          const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
          emaSecondNum = _w && (_pl === 0 || _v === 0) ? _th
            : _w && _v2 !== (_th + _v2) - _th ? _th + _v
            : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
        }
        {
          const _a: number = betaSecond, _b: number = emaSecondDen, _c: number = kSecond * emaFirstDen;
          const _p = _a * _b;
          let _t = 134217729 * _a;
          const _ah = _t - (_t - _a), _al = _a - _ah;
          _t = 134217729 * _b;
          const _bh = _t - (_t - _b), _bl = _b - _bh;
          const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
          const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
          const _v = _tl + _pl, _v2 = _v + _v;
          const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
          emaSecondDen = _w && (_pl === 0 || _v === 0) ? _th
            : _w && _v2 !== (_th + _v2) - _th ? _th + _v
            : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
        }
      }
    }
    nBar = nBar + 1;
    today = today + 1;
  }
  /* The denominator is an EMA of an EMA of |momentum|: every term is
   * non-negative and every weight positive, so it is zero only when every
   * change that reached it was exactly zero -- 0/0, since the numerator is
   * zero with it, reported as the neutral 0.0 by the CCI (#7) and IMI (#112)
   * convention. Tested exactly rather than against a fixed band: a price
   * change carries the quote unit, and TA_IS_ZERO zeroes the oscillator for
   * any instrument quoted below it (issue #253).
   */
  if (emaSecondDen > 0.0) {
    tsiValue = 100.0 * emaSecondNum / emaSecondDen;
  } else {
    tsiValue = 0.0;
  }
  outReal[0] = tsiValue;
  outIdx = 1;
  /* Stable zone. Both stages are a pure recursion from here on. */
  while (today <= endIdx) {
    mom = inReal[today] - prevClose;
    prevClose = inReal[today];
    absMom = Math.abs(mom);
    {
      const _a: number = betaFirst, _b: number = emaFirstNum, _c: number = kFirst * mom;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaFirstNum = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = betaFirst, _b: number = emaFirstDen, _c: number = kFirst * absMom;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaFirstDen = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = betaSecond, _b: number = emaSecondNum, _c: number = kSecond * emaFirstNum;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaSecondNum = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = betaSecond, _b: number = emaSecondDen, _c: number = kSecond * emaFirstDen;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaSecondDen = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    if (emaSecondDen > 0.0) {
      tsiValue = 100.0 * emaSecondNum / emaSecondDen;
    } else {
      tsiValue = 0.0;
    }
    outReal[outIdx] = tsiValue;
    outIdx = outIdx + 1;
    today = today + 1;
  }
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['firstPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['secondPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000']] as const;

/** @internal */
export function tsiPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInFirstPeriod: number,
  optInSecondPeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('TSI', 'startIdx', startIdx);
  endIdx = requireInt32('TSI', 'endIdx', endIdx);
  optInFirstPeriod = requireInt32('TSI', 'firstPeriod', optInFirstPeriod);
  optInSecondPeriod = requireInt32('TSI', 'secondPeriod', optInSecondPeriod);
  requireIndexRange('TSI', startIdx, endIdx);
  const guardLookback = tsiLookbackPositional(optInFirstPeriod, optInSecondPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('TSI', PARAM_RANGES, optInFirstPeriod, optInSecondPeriod);
  }
  const guardStart = clampedStart('TSI', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('TSI', 'real', inReal, guardInLen);
  requireOutput('TSI', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('TSI', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = tsiImpl(core, startIdx, endIdx, inRealF64, optInFirstPeriod, optInSecondPeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('TSI', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link tsi}, {@link tsiLookback}, {@link tsiDisplayShift},
 * {@link tsiOpen} and {@link tsiOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface TsiOptions {
  /**
   * Period of the first smoothing, applied to the raw momentum (default 25; range 2
   * to 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  firstPeriod?: number | undefined;
  /**
   * Period of the second smoothing, applied to the first (default 13; range 2 to
   * 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  secondPeriod?: number | undefined;
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

/** What {@link tsi} returns, and {@link tsiOpenAndFill} with its stream. */
export interface TsiResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * True Strength Index, -100 to +100. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link tsi} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link TsiOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function tsiLookback(options?: TsiOptions): number {
  const o = optionsOf('TSI lookback', options);
  return tsiLookbackPositional(orDefault(o.firstPeriod, Core.INTEGER_DEFAULT), orDefault(o.secondPeriod, Core.INTEGER_DEFAULT), coreOf('TSI lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link tsi}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link TsiOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function tsiDisplayShift(outputIdx: number, options?: TsiOptions): number {
  const o = optionsOf('TSI displayShift', options);
  return tsiDisplayShiftPositional(orDefault(o.firstPeriod, Core.INTEGER_DEFAULT), orDefault(o.secondPeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('TSI displayShift', o.core));
}

/**
 * True Strength Index: William Blau's double-smoothed momentum oscillator. The
 * one-bar price change is smoothed twice with exponential averages, and the same
 * pair of averages is applied to the magnitude of that change; the ratio of the
 * two is scaled by 100. Dividing the smoothed signed momentum by the smoothed
 * absolute momentum normalises the reading, so the result is bounded by -100 and
 * +100 and comparable across instruments. The double smoothing is what separates
 * it from a raw momentum plot: the curve is smooth enough to read while keeping
 * far less lag than a single average of the same total length. Zero is the
 * reference line — positive means the smoothed momentum is net upward, negative
 * net downward — and its crossings are the usual trade trigger. Extreme readings
 * mark overbought and oversold conditions, and divergence against price is the
 * classic Blau reading. A signal line is not part of the output; apply `EMA` to
 * `outReal` to obtain one, since no source agrees on its period.
 *
 * Formula and more info at
 * [ta-lib.org/functions/tsi](https://ta-lib.org/functions/tsi).
 *
 * **Notes**
 * - An input whose every change is exactly zero leaves both the numerator and the
 *   denominator at zero. Rather than divide, TSI emits 0 there, its neutral value,
 *   as CCI and IMI emit theirs. Some implementations divide unguarded and return a
 *   non-finite value.
 * - Each exponential average is seeded with a simple average of its own first
 *   inputs, the same seeding TA-Lib's EMA uses, so the first published values
 *   converge toward an unlimited-history result rather than reproducing it
 *   exactly. `CoreBuilder.unstablePeriod(FuncUnstId.EMA, ...)` discards more of
 *   that warm-up. Implementations seeding from a single first sample —
 *   trading-signals among them — differ over the transient and agree once it
 *   decays.
 * - The parameters are named by the order they are applied in, not fast and slow.
 *   Blau's published pair applies the longer average first, the inverse of the
 *   differenced fast/slow pairs elsewhere in the library, so swapping them
 *   silently returns a different indicator with the same lookback.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link tsiLookback} is a success with empty outputs.
 *
 * @param real - Source price/value series, canonically the close.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link TsiOptions}.
 * @returns `begIdx` and the values; see {@link TsiResult}. Each output is a view
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
 * import { tsi } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = tsi(data, { firstPeriod: 25, secondPeriod: 13 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`smi`](https://ta-lib.org/functions/smi)
 * @see [`macd`](https://ta-lib.org/functions/macd)
 * @see [`cmo`](https://ta-lib.org/functions/cmo)
 * @see [`rsi`](https://ta-lib.org/functions/rsi)
 */
export function tsi(real: ArrayLike<number>, options?: TsiOptions): TsiResult {
  const o = optionsOf('TSI', options);
  if (o.endIdx === undefined) {
    requireSeries('TSI', 'real', real);
  }
  const optInFirstPeriod = orDefault(o.firstPeriod, Core.INTEGER_DEFAULT);
  const optInSecondPeriod = orDefault(o.secondPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('TSI', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, tsiLookbackPositional(optInFirstPeriod, optInSecondPeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = tsiPositional(startIdx, endIdx, real, optInFirstPeriod, optInSecondPeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live TSI stream: one value per closed bar, bit-identical to {@link tsi} over
 * the same series. Open it with {@link tsiOpen} or {@link tsiOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/tsi](https://ta-lib.org/functions/tsi).
 */
export class TsiStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInFirstPeriod: number = 0;
  /** @internal */
  optInSecondPeriod: number = 0;
  /** @internal */
  kFirst: number = 0;
  /** @internal */
  kSecond: number = 0;
  /** @internal */
  betaFirst: number = 0;
  /** @internal */
  betaSecond: number = 0;
  /** @internal */
  emaFirstNum: number = 0;
  /** @internal */
  emaFirstDen: number = 0;
  /** @internal */
  emaSecondNum: number = 0;
  /** @internal */
  emaSecondDen: number = 0;
  /** @internal */
  prevClose: number = 0;
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
  static of(core: Core): TsiStream {
    return new TsiStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link tsi} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('TSI advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: Source price/value series, canonically the
   *   close.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('TSI update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('TSI update', 'real');
    }
    tsiStepImpl(this, real);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: Source price/value series, canonically the
   *   close.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number): number {
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('TSI peek', 'real');
    }
    const sp = this;
    const inReal = real;
    let mom = 0;
    let absMom = 0;
    let tsiValue = 0;
    let cur_outReal = 0;
    let emaFirstDen = sp.emaFirstDen;
    let emaFirstNum = sp.emaFirstNum;
    let emaSecondDen = sp.emaSecondDen;
    let emaSecondNum = sp.emaSecondNum;
    let prevClose = sp.prevClose;
    mom = inReal - prevClose;
    prevClose = inReal;
    absMom = Math.abs(mom);
    {
      const _a: number = sp.betaFirst, _b: number = emaFirstNum, _c: number = sp.kFirst * mom;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaFirstNum = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = sp.betaFirst, _b: number = emaFirstDen, _c: number = sp.kFirst * absMom;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaFirstDen = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = sp.betaSecond, _b: number = emaSecondNum, _c: number = sp.kSecond * emaFirstNum;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaSecondNum = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = sp.betaSecond, _b: number = emaSecondDen, _c: number = sp.kSecond * emaFirstDen;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaSecondDen = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    if (emaSecondDen > 0.0) {
      tsiValue = 100.0 * emaSecondNum / emaSecondDen;
    } else {
      tsiValue = 0.0;
    }
    cur_outReal = tsiValue;
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
  clone(): TsiStream {
    const c = new TsiStream(this.core);
    c.optInFirstPeriod = this.optInFirstPeriod;
    c.optInSecondPeriod = this.optInSecondPeriod;
    c.kFirst = this.kFirst;
    c.kSecond = this.kSecond;
    c.betaFirst = this.betaFirst;
    c.betaSecond = this.betaSecond;
    c.emaFirstNum = this.emaFirstNum;
    c.emaFirstDen = this.emaFirstDen;
    c.emaSecondNum = this.emaSecondNum;
    c.emaSecondDen = this.emaSecondDen;
    c.prevClose = this.prevClose;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function tsiStepImpl(sp: TsiStream, inReal: number): void {
  let mom = 0;
  let absMom = 0;
  let tsiValue = 0;
  mom = inReal - sp.prevClose;
  sp.prevClose = inReal;
  absMom = Math.abs(mom);
  {
    const _a: number = sp.betaFirst, _b: number = sp.emaFirstNum, _c: number = sp.kFirst * mom;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.emaFirstNum = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  {
    const _a: number = sp.betaFirst, _b: number = sp.emaFirstDen, _c: number = sp.kFirst * absMom;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.emaFirstDen = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  {
    const _a: number = sp.betaSecond, _b: number = sp.emaSecondNum, _c: number = sp.kSecond * sp.emaFirstNum;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.emaSecondNum = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  {
    const _a: number = sp.betaSecond, _b: number = sp.emaSecondDen, _c: number = sp.kSecond * sp.emaFirstDen;
    const _p = _a * _b;
    let _t = 134217729 * _a;
    const _ah = _t - (_t - _a), _al = _a - _ah;
    _t = 134217729 * _b;
    const _bh = _t - (_t - _b), _bl = _b - _bh;
    const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
    const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
    const _v = _tl + _pl, _v2 = _v + _v;
    const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
    sp.emaSecondDen = _w && (_pl === 0 || _v === 0) ? _th
      : _w && _v2 !== (_th + _v2) - _th ? _th + _v
      : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
  }
  if (sp.emaSecondDen > 0.0) {
    tsiValue = 100.0 * sp.emaSecondNum / sp.emaSecondDen;
  } else {
    tsiValue = 0.0;
  }
  sp.cur_outReal = tsiValue;
}

function tsiOpenImpl(
  sp: TsiStream,
  inReal: Float64Array,
  startIdx: number,
  optInFirstPeriod: number,
  optInSecondPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let kFirst = 0;
  let kSecond = 0;
  let betaFirst = 0;
  let betaSecond = 0;
  let emaFirstNum = 0;
  let emaFirstDen = 0;
  let emaSecondNum = 0;
  let emaSecondDen = 0;
  let sumFirstNum = 0;
  let sumFirstDen = 0;
  let sumSecondNum = 0;
  let sumSecondDen = 0;
  let prevClose = 0;
  let mom = 0;
  let absMom = 0;
  let tsiValue = 0;
  let lookbackTotal = 0;
  let lookbackFirst = 0;
  let today = 0;
  let outIdx = 0;
  let nBar = 0;
  let nSecond = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInFirstPeriod === Core.INTEGER_DEFAULT) {
    optInFirstPeriod = 25;
  } else if (optInFirstPeriod < 2 || optInFirstPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSecondPeriod === Core.INTEGER_DEFAULT) {
    optInSecondPeriod = 13;
  } else if (optInSecondPeriod < 2 || optInSecondPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  lookbackTotal = tsiLookbackPositional(optInFirstPeriod, optInSecondPeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outBegIdx.value = startIdx;
  /* Blau's double smoothing in one pass: the signed momentum and its
   * magnitude are carried through the same two EMA stages, then divided.
   *
   * Each stage seeds the way ema.c does -- a simple average of that stage's
   * first 'period' inputs -- so the result is bit-identical to TA_MOM(1)
   * followed by two TA_EMA calls on each chain. The stage boundary below is
   * the callee LOOKBACK, not (period-1), so that a warm
   * TA_SetUnstablePeriod(TA_FUNC_UNST_EMA) folds in: the second stage then
   * seeds from the values the first would have published. The seed sums
   * accumulate from 0.0 in production order and the recurrence is
   * k*x + beta*prev with ema.c's k and beta; do
   * not reorder or fuse them (0.0+x is not x for x=-0.0). That order IS the
   * bit-exactness contract against the composed reference.
   *
   * prevClose is carried in a scalar rather than re-read from inReal[t-1]
   * because outReal may alias inReal: the slot holding close[t-1] may already
   * hold an output written a bar earlier.
   */
  betaFirst = (optInFirstPeriod - 1) / (optInFirstPeriod + 1);
  kFirst = 1.0 - betaFirst;
  if (betaFirst < 0.5) {
    betaFirst = 1.0 - kFirst;
  }
  betaSecond = (optInSecondPeriod - 1) / (optInSecondPeriod + 1);
  kSecond = 1.0 - betaSecond;
  if (betaSecond < 0.5) {
    betaSecond = 1.0 - kSecond;
  }
  lookbackFirst = emaLookbackPositional(optInFirstPeriod, core);
  emaFirstNum = 0.0;
  emaFirstDen = 0.0;
  emaSecondNum = 0.0;
  emaSecondDen = 0.0;
  sumFirstNum = 0.0;
  sumFirstDen = 0.0;
  sumSecondNum = 0.0;
  sumSecondDen = 0.0;
  /* The first bar carrying a close-to-close change. */
  today = startIdx - lookbackTotal + 1;
  prevClose = inReal[today - 1];
  nBar = 0;
  /* Warm-up. Runs through startIdx inclusive: the last pass here completes
   * the second stage's seed, so it produces the first output.
   */
  while (today <= startIdx) {
    mom = inReal[today] - prevClose;
    prevClose = inReal[today];
    absMom = Math.abs(mom);
    /* Stage 1: the first EMA, over the raw momentum and its magnitude. */
    if (nBar < optInFirstPeriod) {
      sumFirstNum = sumFirstNum + mom;
      sumFirstDen = sumFirstDen + absMom;
      if (nBar === optInFirstPeriod - 1) {
        emaFirstNum = sumFirstNum / optInFirstPeriod;
        emaFirstDen = sumFirstDen / optInFirstPeriod;
      }
    } else {
      {
        const _a: number = betaFirst, _b: number = emaFirstNum, _c: number = kFirst * mom;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        emaFirstNum = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      {
        const _a: number = betaFirst, _b: number = emaFirstDen, _c: number = kFirst * absMom;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        emaFirstDen = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
    }
    /* Stage 2: the second EMA, over what stage 1 publishes.
     *
     * The stage counter is compared BEFORE it is subtracted, never after.
     * Writing this as `nSecond = nBar - lookbackFirst; if( nSecond >= 0 )`
     * is correct in C, where the counters are signed, and broken everywhere
     * else: the Rust backend renders them as usize, so the subtraction
     * underflows for the first lookbackFirst bars -- a panic in a debug
     * build and a wrap in release. It would also be invisible to the
     * cross-language gate, which runs release servers at unstable period 0,
     * where the branch the wrap wrongly takes happens to be a no-op because
     * both accumulators are still 0.0. smi.c states the same rule.
     */
    if (nBar >= lookbackFirst) {
      nSecond = nBar - lookbackFirst;
      if (nSecond < optInSecondPeriod) {
        sumSecondNum = sumSecondNum + emaFirstNum;
        sumSecondDen = sumSecondDen + emaFirstDen;
        if (nSecond === optInSecondPeriod - 1) {
          emaSecondNum = sumSecondNum / optInSecondPeriod;
          emaSecondDen = sumSecondDen / optInSecondPeriod;
        }
      } else {
        {
          const _a: number = betaSecond, _b: number = emaSecondNum, _c: number = kSecond * emaFirstNum;
          const _p = _a * _b;
          let _t = 134217729 * _a;
          const _ah = _t - (_t - _a), _al = _a - _ah;
          _t = 134217729 * _b;
          const _bh = _t - (_t - _b), _bl = _b - _bh;
          const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
          const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
          const _v = _tl + _pl, _v2 = _v + _v;
          const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
          emaSecondNum = _w && (_pl === 0 || _v === 0) ? _th
            : _w && _v2 !== (_th + _v2) - _th ? _th + _v
            : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
        }
        {
          const _a: number = betaSecond, _b: number = emaSecondDen, _c: number = kSecond * emaFirstDen;
          const _p = _a * _b;
          let _t = 134217729 * _a;
          const _ah = _t - (_t - _a), _al = _a - _ah;
          _t = 134217729 * _b;
          const _bh = _t - (_t - _b), _bl = _b - _bh;
          const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
          const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
          const _v = _tl + _pl, _v2 = _v + _v;
          const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
          emaSecondDen = _w && (_pl === 0 || _v === 0) ? _th
            : _w && _v2 !== (_th + _v2) - _th ? _th + _v
            : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
        }
      }
    }
    nBar = nBar + 1;
    today = today + 1;
  }
  /* The denominator is an EMA of an EMA of |momentum|: every term is
   * non-negative and every weight positive, so it is zero only when every
   * change that reached it was exactly zero -- 0/0, since the numerator is
   * zero with it, reported as the neutral 0.0 by the CCI (#7) and IMI (#112)
   * convention. Tested exactly rather than against a fixed band: a price
   * change carries the quote unit, and TA_IS_ZERO zeroes the oscillator for
   * any instrument quoted below it (issue #253).
   */
  if (emaSecondDen > 0.0) {
    tsiValue = 100.0 * emaSecondNum / emaSecondDen;
  } else {
    tsiValue = 0.0;
  }
  outReal[((0 * outStride) | 0)] = tsiValue;
  outIdx = 1;
  /* Stable zone. Both stages are a pure recursion from here on. */
  while (today <= endIdx) {
    mom = inReal[today] - prevClose;
    prevClose = inReal[today];
    absMom = Math.abs(mom);
    {
      const _a: number = betaFirst, _b: number = emaFirstNum, _c: number = kFirst * mom;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaFirstNum = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = betaFirst, _b: number = emaFirstDen, _c: number = kFirst * absMom;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaFirstDen = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = betaSecond, _b: number = emaSecondNum, _c: number = kSecond * emaFirstNum;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaSecondNum = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    {
      const _a: number = betaSecond, _b: number = emaSecondDen, _c: number = kSecond * emaFirstDen;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      emaSecondDen = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    if (emaSecondDen > 0.0) {
      tsiValue = 100.0 * emaSecondNum / emaSecondDen;
    } else {
      tsiValue = 0.0;
    }
    outReal[((outIdx * outStride) | 0)] = tsiValue;
    outIdx = outIdx + 1;
    today = today + 1;
  }
  outNBElement.value = outIdx;
  sp.optInFirstPeriod = optInFirstPeriod;
  sp.optInSecondPeriod = optInSecondPeriod;
  sp.kFirst = kFirst;
  sp.kSecond = kSecond;
  sp.betaFirst = betaFirst;
  sp.betaSecond = betaSecond;
  sp.emaFirstNum = emaFirstNum;
  sp.emaFirstDen = emaFirstDen;
  sp.emaSecondNum = emaSecondNum;
  sp.emaSecondDen = emaSecondDen;
  sp.prevClose = prevClose;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function tsiOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInFirstPeriod: number,
  optInSecondPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): TsiStream {
  const sp = TsiStream.of(core);
  const retCode = tsiOpenImpl(sp, inReal, startIdx, optInFirstPeriod, optInSecondPeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('TSI openAndFill', inReal.length, startIdx, tsiLookbackPositional(optInFirstPeriod, optInSecondPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && tsiLookbackPositional(optInFirstPeriod, optInSecondPeriod, core) < 0) {
    throw rejectedParams('TSI openAndFill', PARAM_RANGES, optInFirstPeriod, optInSecondPeriod);
  }
  throw streamFailure('TSI openAndFill', retCode);
}

/** @internal */
export function tsiOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInFirstPeriod: number,
  optInSecondPeriod: number,
): TsiStream {
  const sp = TsiStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = tsiOpenImpl(sp, inReal, startIdx, optInFirstPeriod, optInSecondPeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('TSI open', inReal.length, startIdx, tsiLookbackPositional(optInFirstPeriod, optInSecondPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && tsiLookbackPositional(optInFirstPeriod, optInSecondPeriod, core) < 0) {
    throw rejectedParams('TSI open', PARAM_RANGES, optInFirstPeriod, optInSecondPeriod);
  }
  throw streamFailure('TSI open', retCode);
}

/** @internal */
export function tsiOpenPositional(
  inReal: ArrayLike<number>,
  optInFirstPeriod: number,
  optInSecondPeriod: number,
  core: Core = Core.DEFAULT,
): TsiStream {
  optInFirstPeriod = requireInt32('TSI open', 'firstPeriod', optInFirstPeriod);
  optInSecondPeriod = requireInt32('TSI open', 'secondPeriod', optInSecondPeriod);
  requireArgument('TSI open', 'real', inReal);
  requireHistory('TSI open', 'real', inReal.length);
  return tsiOpenInternal(core, float64(inReal), 0, optInFirstPeriod, optInSecondPeriod);
}

/** @internal */
export function tsiOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInFirstPeriod: number,
  optInSecondPeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): TsiStream {
  optInFirstPeriod = requireInt32('TSI openAndFill', 'firstPeriod', optInFirstPeriod);
  optInSecondPeriod = requireInt32('TSI openAndFill', 'secondPeriod', optInSecondPeriod);
  requireArgument('TSI openAndFill', 'real', inReal);
  requireHistory('TSI openAndFill', 'real', inReal.length);
  const guardLookback = tsiLookbackPositional(optInFirstPeriod, optInSecondPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('TSI openAndFill', PARAM_RANGES, optInFirstPeriod, optInSecondPeriod);
  }
  const guardOutLen = openFillCount('TSI openAndFill', inReal.length, guardLookback);
  requireOutput('TSI openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('TSI openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('TSI openAndFill', 'outReal', 'real');
  }
  return tsiOpenAndFillInternal(core, inRealF64, 0, optInFirstPeriod, optInSecondPeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live TSI stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link tsi} there.
 *
 * @param real - Warm-up history. Source price/value series, canonically the close.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link TsiOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `tsiLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { tsiOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = tsiOpen(data, { firstPeriod: 25, secondPeriod: 13 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function tsiOpen(real: ArrayLike<number>, options?: TsiOptions): TsiStream {
  const o = optionsOf('TSI open', options);
  const optInFirstPeriod = orDefault(o.firstPeriod, Core.INTEGER_DEFAULT);
  const optInSecondPeriod = orDefault(o.secondPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('TSI open', o.core);
  return tsiOpenPositional(real, optInFirstPeriod, optInSecondPeriod, core);
}

/**
 * {@link tsiOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link tsi}.
 *
 * @param real - Warm-up history. Source price/value series, canonically the close.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link TsiOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link tsi} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `tsiLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or a
 *   buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { tsiOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = tsiOpenAndFill(data, {
 *   firstPeriod: 25,
 *   secondPeriod: 13,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function tsiOpenAndFill(real: ArrayLike<number>, options?: TsiOptions): TsiResult & { stream: TsiStream } {
  const o = optionsOf('TSI openAndFill', options);
  const optInFirstPeriod = orDefault(o.firstPeriod, Core.INTEGER_DEFAULT);
  const optInSecondPeriod = orDefault(o.secondPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('TSI openAndFill', o.core);
  const capacity = fillCapacity(tsiLookbackPositional(optInFirstPeriod, optInSecondPeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = tsiOpenAndFillPositional(real, optInFirstPeriod, optInSecondPeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
