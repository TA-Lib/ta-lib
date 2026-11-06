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
 *  090426 MF,CC  First version (issue #359).
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma, fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { emaLookbackPositional } from './ema.ts';

/** @internal */
export function massiLookbackPositional(optInFastPeriod: number, optInSlowPeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInFastPeriod | 0) !== optInFastPeriod) {
    return -1;
  }
  if ((optInSlowPeriod | 0) !== optInSlowPeriod) {
    return -1;
  }
  if (optInFastPeriod === Core.INTEGER_DEFAULT) {
    optInFastPeriod = 9;
  } else if (optInFastPeriod < 2 || optInFastPeriod > 100000) {
    return -1;
  }
  if (optInSlowPeriod === Core.INTEGER_DEFAULT) {
    optInSlowPeriod = 25;
  } else if (optInSlowPeriod < 2 || optInSlowPeriod > 100000) {
    return -1;
  }
  /* Two stacked EMA warm-ups over the high-low range, then the summation
   * window. The EMA term is exactly the callee's own lookback, which is what
   * makes MASSI inherit TA_FUNC_UNST_EMA -- and it shifts by 2u, not u.
   */
  return ((emaLookbackPositional(optInFastPeriod, core) * 2) | 0) + (optInSlowPeriod - 1);
}

/** @internal */
export function massiDisplayShiftPositional(optInFastPeriod: number, optInSlowPeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (massiLookbackPositional(optInFastPeriod, optInSlowPeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function massiImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let optInK_1 = 0;
  let emaBeta = 0;
  let hl = 0;
  let ema1 = 0;
  let ema2 = 0;
  let sum1 = 0;
  let sum2 = 0;
  let ratio = 0;
  let total = 0;
  let tempReal = 0;
  let lookbackTotal = 0;
  let lookbackEma = 0;
  let lookbackEma2 = 0;
  let today = 0;
  let outIdx = 0;
  let nBar = 0;
  let n2 = 0;
  let ratioRing!: Float64Array;
  let ratioRing_Idx = 0;
  let maxIdx_ratioRing = (32) - 1;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInFastPeriod === Core.INTEGER_DEFAULT) {
    optInFastPeriod = 9;
  } else if (optInFastPeriod < 2 || optInFastPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowPeriod === Core.INTEGER_DEFAULT) {
    optInSlowPeriod = 25;
  } else if (optInSlowPeriod < 2 || optInSlowPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  lookbackEma = emaLookbackPositional(optInFastPeriod, core);
  lookbackEma2 = ((lookbackEma * 2) | 0);
  lookbackTotal = lookbackEma2 + (optInSlowPeriod - 1);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  if (optInSlowPeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  ratioRing = new Float64Array(optInSlowPeriod);
  maxIdx_ratioRing = optInSlowPeriod - 1;
  ratioRing_Idx = 0;
  outBegIdx.value = startIdx;
  /* Dorsey's pipeline in one pass: EMA of the high-low range, EMA of that,
   * their ratio summed over a trailing window.
   *
   * Each stage seeds the way ema.c does -- a simple average of that stage's
   * first optInFastPeriod inputs -- and its boundary below is the callee
   * LOOKBACK, never (optInFastPeriod - 1). The two coincide exactly at
   * unstable period 0, which is where every cross-language gate runs, so
   * confusing them is invisible until TA_SetUnstablePeriod(TA_FUNC_UNST_EMA)
   * is warmed. The seed sums accumulate from 0.0 in production order; do not
   * reorder or fuse them (0.0+x is not x for x=-0.0).
   */
  emaBeta = (optInFastPeriod - 1) / (optInFastPeriod + 1);
  optInK_1 = 1.0 - emaBeta;
  if (emaBeta < 0.5) {
    emaBeta = 1.0 - optInK_1;
  }
  ema1 = 0.0;
  ema2 = 0.0;
  sum1 = 0.0;
  sum2 = 0.0;
  total = 0.0;
  tempReal = 0.0;
  today = startIdx - lookbackTotal;
  nBar = 0;
  /* Runs through startIdx inclusive: that last pass fills the summation
   * window and so produces the first output.
   */
  while (today <= startIdx) {
    hl = inHigh[today] - inLow[today];
    if (nBar < optInFastPeriod) {
      sum1 = sum1 + hl;
      if (nBar === optInFastPeriod - 1) {
        ema1 = sum1 / optInFastPeriod;
      }
    } else {
      {
        const _a: number = emaBeta, _b: number = ema1, _c: number = optInK_1 * hl;
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
    /* The stage counter is compared BEFORE it is subtracted, never after.
     * `n2 = nBar - lookbackEma; if( n2 >= 0 )` is correct in C and broken
     * everywhere else: the Rust backend renders these as usize, so the
     * subtraction underflows for the first lookbackEma bars.
     */
    if (nBar >= lookbackEma) {
      n2 = nBar - lookbackEma;
      if (n2 < optInFastPeriod) {
        sum2 = sum2 + ema1;
        if (n2 === optInFastPeriod - 1) {
          ema2 = sum2 / optInFastPeriod;
        }
      } else {
        {
          const _a: number = emaBeta, _b: number = ema2, _c: number = optInK_1 * ema1;
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
    if (nBar >= lookbackEma2) {
      /* A flat market is the ratio's continuous limit, 1.0, not the zero
       * that an oscillator centred on zero would report: MASSI's own
       * neutral is optInSlowPeriod. Test ema2 exactly and never through an
       * epsilon band -- a smoothed price range carries the quote unit, so a
       * fixed band would pin the whole index at optInSlowPeriod for any
       * instrument quoted under it (issue #253).
       */
      if (ema2 === 0.0) {
        ratio = 1.0;
      } else {
        ratio = ema1 / ema2;
      }
      /* TA_SUM's accumulation order -- add, publish, subtract -- reproduced
       * over a ring: the slot written here was emptied out of `total` at the
       * end of the previous bar, so nothing has to be read before the store.
       */
      ratioRing[ratioRing_Idx] = ratio;
      total = total + ratio;
      ratioRing_Idx++;
      if (ratioRing_Idx > maxIdx_ratioRing) {
        ratioRing_Idx = 0;
      }
      if (nBar === lookbackTotal) {
        tempReal = total;
        total = total - ratioRing[ratioRing_Idx];
      }
    }
    nBar = nBar + 1;
    today = today + 1;
  }
  /* In-place safe: this store lands lookbackTotal bars behind the input
   * cursor, so no bar is written under a read still owed to it.
   */
  outReal[0] = tempReal;
  outIdx = 1;
  while (today <= endIdx) {
    hl = inHigh[today] - inLow[today];
    {
      const _a: number = emaBeta, _b: number = ema1, _c: number = optInK_1 * hl;
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
      const _a: number = emaBeta, _b: number = ema2, _c: number = optInK_1 * ema1;
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
    if (ema2 === 0.0) {
      ratio = 1.0;
    } else {
      ratio = ema1 / ema2;
    }
    ratioRing[ratioRing_Idx] = ratio;
    total = total + ratio;
    ratioRing_Idx++;
    if (ratioRing_Idx > maxIdx_ratioRing) {
      ratioRing_Idx = 0;
    }
    tempReal = total;
    total = total - ratioRing[ratioRing_Idx];
    outReal[outIdx] = tempReal;
    outIdx = outIdx + 1;
    today = today + 1;
  }
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['fastPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['slowPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000']] as const;

/** @internal */
export function massiPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('MASSI', 'startIdx', startIdx);
  endIdx = requireInt32('MASSI', 'endIdx', endIdx);
  optInFastPeriod = requireInt32('MASSI', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('MASSI', 'slowPeriod', optInSlowPeriod);
  requireIndexRange('MASSI', startIdx, endIdx);
  const guardLookback = massiLookbackPositional(optInFastPeriod, optInSlowPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('MASSI', PARAM_RANGES, optInFastPeriod, optInSlowPeriod);
  }
  const guardStart = clampedStart('MASSI', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('MASSI', 'prices.high', inHigh, guardInLen);
  requireLength('MASSI', 'prices.low', inLow, guardInLen);
  requireOutput('MASSI', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlapsPartly(outReal, inHighF64)) {
    throw sharedMemory('MASSI', 'outReal', 'prices.high');
  }
  if (overlapsPartly(outReal, inLowF64)) {
    throw sharedMemory('MASSI', 'outReal', 'prices.low');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = massiImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInFastPeriod, optInSlowPeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('MASSI', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link massi}, {@link massiLookback}, {@link massiDisplayShift},
 * {@link massiOpen} and {@link massiOpenAndFill}. Every key is optional: an
 * omitted or `undefined` key takes its default, and a call ignores the keys it has
 * no use for.
 */
export interface MassiOptions {
  /**
   * Number of bars in each of the two exponential averages of the high-low range
   * (default 9; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  fastPeriod?: number | undefined;
  /**
   * Number of bars the ratio is summed over (default 25; range 2 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  slowPeriod?: number | undefined;
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

/** What {@link massi} returns, and {@link massiOpenAndFill} with its stream. */
export interface MassiResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Summed ratio of the two smoothed high-low ranges. `values[k]` is the value at
   * input bar `begIdx + k`.
   */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link massi} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MassiOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function massiLookback(options?: MassiOptions): number {
  const o = optionsOf('MASSI lookback', options);
  return massiLookbackPositional(orDefault(o.fastPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowPeriod, Core.INTEGER_DEFAULT), coreOf('MASSI lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link massi}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MassiOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function massiDisplayShift(outputIdx: number, options?: MassiOptions): number {
  const o = optionsOf('MASSI displayShift', options);
  return massiDisplayShiftPositional(orDefault(o.fastPeriod, Core.INTEGER_DEFAULT), orDefault(o.slowPeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('MASSI displayShift', o.core));
}

/**
 * Mass Index: Donald Dorsey's non-directional measure of how the trading range
 * itself is expanding or contracting. The high-low range is smoothed by an
 * exponential moving average, that average is smoothed again by a second one of
 * the same length, and the ratio of the first to the second is summed over a
 * trailing window. Read it as a bulge detector, not a direction. A ratio above one
 * means the range is widening faster than its own smoothing can absorb, so the sum
 * rises; a narrowing range pulls it back down. Dorsey's own rule is the "reversal
 * bulge": the index rising above 27, then falling back under 26.5, warns that the
 * prevailing trend is about to reverse. Which way it reverses has to come from a
 * trend indicator, because the Mass Index has no sign of its own.
 *
 * Formula and more info at
 * [ta-lib.org/functions/massi](https://ta-lib.org/functions/massi).
 *
 * **Notes**
 * - The two periods are not interchangeable and are never swapped:
 *   `optInFastPeriod` is the length of both exponential averages,
 *   `optInSlowPeriod` the length of the summation window. Some implementations
 *   reorder them when the summation window is the shorter of the two; this one
 *   does not.
 * - A window in which every bar is exactly flat, high equal to low, leaves both
 *   averages at zero. The ratio is reported as 1 there, its continuous limit, so a
 *   flat market yields exactly `optInSlowPeriod` rather than a spurious zero.
 * - Implementations disagree on how the exponential averages are seeded. TA-Lib
 *   uses its own EMA convention, the simple average of the first `optInFastPeriod`
 *   inputs, where Tulip Indicators, ta4j and trading-signals seed from a single
 *   raw value and converge to these values only after many bars. Published sample
 *   vectors, including the one in Achelis, are seeded that way and match only in
 *   the tail.
 * - MASSI inherits EMA's unstable period rather than owning one, and inherits it
 *   twice: when `TA_FUNC_UNST_EMA`, set to a count or to an Auto level, discards
 *   `u` bars from an EMA of `optInFastPeriod`, MASSI's first output moves by 2u.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link massiLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `high`: High
 *   price. `low`: Low price.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link MassiOptions}.
 * @returns `begIdx` and the values; see {@link MassiResult}. Each output is a view
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
 * import { massi } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = massi({ high, low }, {
 *   fastPeriod: 9,
 *   slowPeriod: 25,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`cvi`](https://ta-lib.org/functions/cvi)
 * @see [`atr`](https://ta-lib.org/functions/atr)
 * @see [`natr`](https://ta-lib.org/functions/natr)
 * @see [`trange`](https://ta-lib.org/functions/trange)
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`sum`](https://ta-lib.org/functions/sum)
 */
export function massi(prices: { high: ArrayLike<number>; low: ArrayLike<number> }, options?: MassiOptions): MassiResult {
  requireArgument('MASSI', 'prices', prices);
  const o = optionsOf('MASSI', options);
  if (o.endIdx === undefined) {
    requireSeries('MASSI', 'prices.high', prices.high);
    requireSeries('MASSI', 'prices.low', prices.low);
  }
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MASSI', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, massiLookbackPositional(optInFastPeriod, optInSlowPeriod, core), prices.high, prices.low);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = massiPositional(startIdx, endIdx, prices.high, prices.low, optInFastPeriod, optInSlowPeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live MASSI stream: one value per closed bar, bit-identical to {@link massi}
 * over the same series. Open it with {@link massiOpen} or
 * {@link massiOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/massi](https://ta-lib.org/functions/massi).
 */
export class MassiStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInFastPeriod: number = 0;
  /** @internal */
  optInSlowPeriod: number = 0;
  /** @internal */
  optInK_1: number = 0;
  /** @internal */
  emaBeta: number = 0;
  /** @internal */
  ema1: number = 0;
  /** @internal */
  ema2: number = 0;
  /** @internal */
  total: number = 0;
  /** @internal */
  ratioRing_Idx: number = 0;
  /** @internal */
  maxIdx_ratioRing: number = 0;
  /** @internal */
  cbSize_ratioRing: number = 0;
  /** @internal */
  cb_ratioRing: Float64Array = new Float64Array(1);
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
  static of(core: Core): MassiStream {
    return new MassiStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link massi} reports them over the same bars. Each accepted `update` and each
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
      throw failure('MASSI advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inHigh - The bar's `inHigh` value: High price.
   * @param inLow - The bar's `inLow` value: Low price.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('MASSI update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('MASSI update', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    massiStepImpl(this, inHigh, inLow);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price.
   * @param inLow - The bar's `inLow` value: Low price.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number): number {
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('MASSI peek', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    const sp = this;
    let hl = 0;
    let ratio = 0;
    let tempReal = 0;
    let cur_outReal = 0;
    let ema1 = sp.ema1;
    let ema2 = sp.ema2;
    let ratioRing_Idx = sp.ratioRing_Idx;
    let total = sp.total;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    hl = inHigh - inLow;
    ema1 = fma(sp.emaBeta, ema1, sp.optInK_1 * hl);
    ema2 = fma(sp.emaBeta, ema2, sp.optInK_1 * ema1);
    if (ema2 === 0.0) {
      ratio = 1.0;
    } else {
      ratio = ema1 / ema2;
    }
    pkSlot0 = ratioRing_Idx;
    pkVal0 = ratio;
    total = total + ratio;
    ratioRing_Idx = ratioRing_Idx + 1;
    if (ratioRing_Idx > sp.maxIdx_ratioRing) {
      ratioRing_Idx = 0;
    }
    tempReal = total;
    total = total - ((ratioRing_Idx !== pkSlot0) ? sp.cb_ratioRing[ratioRing_Idx] : pkVal0);
    cur_outReal = tempReal;
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
  clone(): MassiStream {
    const c = new MassiStream(this.core);
    c.optInFastPeriod = this.optInFastPeriod;
    c.optInSlowPeriod = this.optInSlowPeriod;
    c.optInK_1 = this.optInK_1;
    c.emaBeta = this.emaBeta;
    c.ema1 = this.ema1;
    c.ema2 = this.ema2;
    c.total = this.total;
    c.ratioRing_Idx = this.ratioRing_Idx;
    c.maxIdx_ratioRing = this.maxIdx_ratioRing;
    c.cbSize_ratioRing = this.cbSize_ratioRing;
    c.cb_ratioRing = this.cb_ratioRing.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function massiStepImpl(sp: MassiStream, inHigh: number, inLow: number): void {
  let hl = 0;
  let ratio = 0;
  let tempReal = 0;
  hl = inHigh - inLow;
  sp.ema1 = fma(sp.emaBeta, sp.ema1, sp.optInK_1 * hl);
  sp.ema2 = fma(sp.emaBeta, sp.ema2, sp.optInK_1 * sp.ema1);
  if (sp.ema2 === 0.0) {
    ratio = 1.0;
  } else {
    ratio = sp.ema1 / sp.ema2;
  }
  sp.cb_ratioRing[sp.ratioRing_Idx] = ratio;
  sp.total = sp.total + ratio;
  sp.ratioRing_Idx = sp.ratioRing_Idx + 1;
  if (sp.ratioRing_Idx > sp.maxIdx_ratioRing) {
    sp.ratioRing_Idx = 0;
  }
  tempReal = sp.total;
  sp.total = sp.total - sp.cb_ratioRing[sp.ratioRing_Idx];
  sp.cur_outReal = tempReal;
}

function massiOpenImpl(
  sp: MassiStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let optInK_1 = 0;
  let emaBeta = 0;
  let hl = 0;
  let ema1 = 0;
  let ema2 = 0;
  let sum1 = 0;
  let sum2 = 0;
  let ratio = 0;
  let total = 0;
  let tempReal = 0;
  let lookbackTotal = 0;
  let lookbackEma = 0;
  let lookbackEma2 = 0;
  let today = 0;
  let outIdx = 0;
  let nBar = 0;
  let n2 = 0;
  let ratioRing!: Float64Array;
  let ratioRing_Idx = 0;
  let maxIdx_ratioRing = (32) - 1;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInFastPeriod === Core.INTEGER_DEFAULT) {
    optInFastPeriod = 9;
  } else if (optInFastPeriod < 2 || optInFastPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSlowPeriod === Core.INTEGER_DEFAULT) {
    optInSlowPeriod = 25;
  } else if (optInSlowPeriod < 2 || optInSlowPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  lookbackEma = emaLookbackPositional(optInFastPeriod, core);
  lookbackEma2 = ((lookbackEma * 2) | 0);
  lookbackTotal = lookbackEma2 + (optInSlowPeriod - 1);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (optInSlowPeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  ratioRing = new Float64Array(optInSlowPeriod);
  maxIdx_ratioRing = optInSlowPeriod - 1;
  ratioRing_Idx = 0;
  outBegIdx.value = startIdx;
  /* Dorsey's pipeline in one pass: EMA of the high-low range, EMA of that,
   * their ratio summed over a trailing window.
   *
   * Each stage seeds the way ema.c does -- a simple average of that stage's
   * first optInFastPeriod inputs -- and its boundary below is the callee
   * LOOKBACK, never (optInFastPeriod - 1). The two coincide exactly at
   * unstable period 0, which is where every cross-language gate runs, so
   * confusing them is invisible until TA_SetUnstablePeriod(TA_FUNC_UNST_EMA)
   * is warmed. The seed sums accumulate from 0.0 in production order; do not
   * reorder or fuse them (0.0+x is not x for x=-0.0).
   */
  emaBeta = (optInFastPeriod - 1) / (optInFastPeriod + 1);
  optInK_1 = 1.0 - emaBeta;
  if (emaBeta < 0.5) {
    emaBeta = 1.0 - optInK_1;
  }
  ema1 = 0.0;
  ema2 = 0.0;
  sum1 = 0.0;
  sum2 = 0.0;
  total = 0.0;
  tempReal = 0.0;
  today = startIdx - lookbackTotal;
  nBar = 0;
  /* Runs through startIdx inclusive: that last pass fills the summation
   * window and so produces the first output.
   */
  while (today <= startIdx) {
    hl = inHigh[today] - inLow[today];
    if (nBar < optInFastPeriod) {
      sum1 = sum1 + hl;
      if (nBar === optInFastPeriod - 1) {
        ema1 = sum1 / optInFastPeriod;
      }
    } else {
      {
        const _a: number = emaBeta, _b: number = ema1, _c: number = optInK_1 * hl;
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
    /* The stage counter is compared BEFORE it is subtracted, never after.
     * `n2 = nBar - lookbackEma; if( n2 >= 0 )` is correct in C and broken
     * everywhere else: the Rust backend renders these as usize, so the
     * subtraction underflows for the first lookbackEma bars.
     */
    if (nBar >= lookbackEma) {
      n2 = nBar - lookbackEma;
      if (n2 < optInFastPeriod) {
        sum2 = sum2 + ema1;
        if (n2 === optInFastPeriod - 1) {
          ema2 = sum2 / optInFastPeriod;
        }
      } else {
        {
          const _a: number = emaBeta, _b: number = ema2, _c: number = optInK_1 * ema1;
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
    if (nBar >= lookbackEma2) {
      /* A flat market is the ratio's continuous limit, 1.0, not the zero
       * that an oscillator centred on zero would report: MASSI's own
       * neutral is optInSlowPeriod. Test ema2 exactly and never through an
       * epsilon band -- a smoothed price range carries the quote unit, so a
       * fixed band would pin the whole index at optInSlowPeriod for any
       * instrument quoted under it (issue #253).
       */
      if (ema2 === 0.0) {
        ratio = 1.0;
      } else {
        ratio = ema1 / ema2;
      }
      /* TA_SUM's accumulation order -- add, publish, subtract -- reproduced
       * over a ring: the slot written here was emptied out of `total` at the
       * end of the previous bar, so nothing has to be read before the store.
       */
      ratioRing[ratioRing_Idx] = ratio;
      total = total + ratio;
      ratioRing_Idx++;
      if (ratioRing_Idx > maxIdx_ratioRing) {
        ratioRing_Idx = 0;
      }
      if (nBar === lookbackTotal) {
        tempReal = total;
        total = total - ratioRing[ratioRing_Idx];
      }
    }
    nBar = nBar + 1;
    today = today + 1;
  }
  /* In-place safe: this store lands lookbackTotal bars behind the input
   * cursor, so no bar is written under a read still owed to it.
   */
  outReal[((0 * outStride) | 0)] = tempReal;
  outIdx = 1;
  while (today <= endIdx) {
    hl = inHigh[today] - inLow[today];
    {
      const _a: number = emaBeta, _b: number = ema1, _c: number = optInK_1 * hl;
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
      const _a: number = emaBeta, _b: number = ema2, _c: number = optInK_1 * ema1;
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
    if (ema2 === 0.0) {
      ratio = 1.0;
    } else {
      ratio = ema1 / ema2;
    }
    ratioRing[ratioRing_Idx] = ratio;
    total = total + ratio;
    ratioRing_Idx++;
    if (ratioRing_Idx > maxIdx_ratioRing) {
      ratioRing_Idx = 0;
    }
    tempReal = total;
    total = total - ratioRing[ratioRing_Idx];
    outReal[((outIdx * outStride) | 0)] = tempReal;
    outIdx = outIdx + 1;
    today = today + 1;
  }
  outNBElement.value = outIdx;
  const capCb_ratioRing = maxIdx_ratioRing + 1;
  if (capCb_ratioRing > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sp.optInFastPeriod = optInFastPeriod;
  sp.optInSlowPeriod = optInSlowPeriod;
  sp.optInK_1 = optInK_1;
  sp.emaBeta = emaBeta;
  sp.ema1 = ema1;
  sp.ema2 = ema2;
  sp.total = total;
  sp.ratioRing_Idx = ratioRing_Idx;
  sp.maxIdx_ratioRing = maxIdx_ratioRing;
  sp.cbSize_ratioRing = capCb_ratioRing;
  sp.cb_ratioRing = ratioRing;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function massiOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): MassiStream {
  const sp = MassiStream.of(core);
  const retCode = massiOpenImpl(sp, inHigh, inLow, startIdx, optInFastPeriod, optInSlowPeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MASSI openAndFill', inHigh.length, startIdx, massiLookbackPositional(optInFastPeriod, optInSlowPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && massiLookbackPositional(optInFastPeriod, optInSlowPeriod, core) < 0) {
    throw rejectedParams('MASSI openAndFill', PARAM_RANGES, optInFastPeriod, optInSlowPeriod);
  }
  throw streamFailure('MASSI openAndFill', retCode);
}

/** @internal */
export function massiOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInFastPeriod: number,
  optInSlowPeriod: number,
): MassiStream {
  const sp = MassiStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = massiOpenImpl(sp, inHigh, inLow, startIdx, optInFastPeriod, optInSlowPeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MASSI open', inHigh.length, startIdx, massiLookbackPositional(optInFastPeriod, optInSlowPeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && massiLookbackPositional(optInFastPeriod, optInSlowPeriod, core) < 0) {
    throw rejectedParams('MASSI open', PARAM_RANGES, optInFastPeriod, optInSlowPeriod);
  }
  throw streamFailure('MASSI open', retCode);
}

/** @internal */
export function massiOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  core: Core = Core.DEFAULT,
): MassiStream {
  optInFastPeriod = requireInt32('MASSI open', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('MASSI open', 'slowPeriod', optInSlowPeriod);
  requireArgument('MASSI open', 'prices.high', inHigh);
  requireHistory('MASSI open', inHigh.length);
  requireArgument('MASSI open', 'prices.low', inLow);
  requireHistoryLength('MASSI open', 'prices.low', inLow.length, inHigh.length);
  return massiOpenInternal(core, float64(inHigh), float64(inLow), 0, optInFastPeriod, optInSlowPeriod);
}

/** @internal */
export function massiOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInFastPeriod: number,
  optInSlowPeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): MassiStream {
  optInFastPeriod = requireInt32('MASSI openAndFill', 'fastPeriod', optInFastPeriod);
  optInSlowPeriod = requireInt32('MASSI openAndFill', 'slowPeriod', optInSlowPeriod);
  requireArgument('MASSI openAndFill', 'prices.high', inHigh);
  requireHistory('MASSI openAndFill', inHigh.length);
  requireArgument('MASSI openAndFill', 'prices.low', inLow);
  const guardLookback = massiLookbackPositional(optInFastPeriod, optInSlowPeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('MASSI openAndFill', PARAM_RANGES, optInFastPeriod, optInSlowPeriod);
  }
  const guardOutLen = openFillCount('MASSI openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('MASSI openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireOutput('MASSI openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('MASSI openAndFill', 'outReal', 'prices.high');
  }
  if (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('MASSI openAndFill', 'outReal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outReal, inHighF64)) {
    throw sharedMemory('MASSI openAndFill', 'outReal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outReal, inLowF64)) {
    throw sharedMemory('MASSI openAndFill', 'outReal', 'prices.low');
  }
  return massiOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInFastPeriod, optInSlowPeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live MASSI stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link massi} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price. `low`: Low price.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MassiOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `massiLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range
 *   or histories of different lengths.
 *
 * @example
 * ```ts
 * import { massiOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = massiOpen({ high, low }, { fastPeriod: 9, slowPeriod: 25 });
 * const peeked = stream.peek(101.4, 99.1); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1); // the bar closes
 * ```
 */
export function massiOpen(prices: { high: ArrayLike<number>; low: ArrayLike<number> }, options?: MassiOptions): MassiStream {
  requireArgument('MASSI open', 'prices', prices);
  const o = optionsOf('MASSI open', options);
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MASSI open', o.core);
  return massiOpenPositional(prices.high, prices.low, optInFastPeriod, optInSlowPeriod, core);
}

/**
 * {@link massiOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link massi}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price. `low`: Low price.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link MassiOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link massi} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `massiLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { massiOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = massiOpenAndFill({ high, low }, {
 *   fastPeriod: 9,
 *   slowPeriod: 25,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function massiOpenAndFill(prices: { high: ArrayLike<number>; low: ArrayLike<number> }, options?: MassiOptions): MassiResult & { stream: MassiStream } {
  requireArgument('MASSI openAndFill', 'prices', prices);
  const o = optionsOf('MASSI openAndFill', options);
  const optInFastPeriod = orDefault(o.fastPeriod, Core.INTEGER_DEFAULT);
  const optInSlowPeriod = orDefault(o.slowPeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MASSI openAndFill', o.core);
  const capacity = fillCapacity(massiLookbackPositional(optInFastPeriod, optInSlowPeriod, core), prices.high, prices.low);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = massiOpenAndFillPositional(prices.high, prices.low, optInFastPeriod, optInSlowPeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
