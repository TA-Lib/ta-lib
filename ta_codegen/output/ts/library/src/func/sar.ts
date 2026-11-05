/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CF       Christo Fogelberg
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  010802 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  122104 MF,CF  Fix#1089506 for out-of-bound access to ep_temp.
 *  082726 MF,CC  Answer a rejected minus_dm before reading ep_temp, not after:
 *                the read was of an uninitialised local.
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireReal, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { minusDmPositional } from './minusDm.ts';

/** @internal */
export function sarLookbackPositional(optInAcceleration: number, optInMaximum: number, _core: Core = Core.DEFAULT): number {
  if (typeof optInAcceleration !== 'number') {
    return -1;
  }
  if (typeof optInMaximum !== 'number') {
    return -1;
  }
  if (optInAcceleration === Core.REAL_DEFAULT) {
    optInAcceleration = 0.02;
  } else if (!(optInAcceleration >= 0e0 && optInAcceleration <= Core.REAL_MAX)) {
    return -1;
  }
  if (optInMaximum === Core.REAL_DEFAULT) {
    optInMaximum = 0.2;
  } else if (!(optInMaximum >= 0e0 && optInMaximum <= Core.REAL_MAX)) {
    return -1;
  }
  /* SAR always sacrify one price bar to establish the
   * initial extreme price.
   */
  return 1;
}

/** @internal */
export function sarDisplayShiftPositional(optInAcceleration: number, optInMaximum: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (sarLookbackPositional(optInAcceleration, optInMaximum, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function sarImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  optInAcceleration: number,
  optInMaximum: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let isLong = 0;
  let todayIdx = 0;
  let outIdx = 0;
  const tempInt = new MInteger();
  let newHigh = 0;
  let newLow = 0;
  let prevHigh = 0;
  let prevLow = 0;
  let af = 0;
  let ep = 0;
  let sar = 0;
  const ep_temp = new Float64Array(1);
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInAcceleration === Core.REAL_DEFAULT) {
    optInAcceleration = 0.02;
  } else if (!(optInAcceleration >= 0e0 && optInAcceleration <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInMaximum === Core.REAL_DEFAULT) {
    optInMaximum = 0.2;
  } else if (!(optInMaximum >= 0e0 && optInMaximum <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  /* > 0 indicates long. == 0 indicates short */
  /* Implementation of the SAR has been a little bit open to interpretation
   * since Wilder (the original author) did not define a precise algorithm
   * on how to bootstrap the algorithm. Take any existing software application
   * and you will see slight variation on how the algorithm was adapted.
   *
   * What is the initial trade direction? Long or short?
   * ===================================================
   * The interpretation of what should be the initial SAR values is
   * open to interpretation, particularly since the caller to the function
   * does not specify the initial direction of the trade.
   *
   * In TA-Lib, the following logic is used:
   *  - Calculate +DM and -DM between the first and
   *    second bar. The highest directional indication will
   *    indicate the assumed direction of the trade for the second
   *    price bar.
   *  - In the case of a tie between +DM and -DM,
   *    the direction is LONG by default.
   *
   * What is the initial "extreme point" and thus SAR?
   * =================================================
   * The following shows how different people took different approach:
   *  - Metastock use the first price bar high/low depending of
   *    the direction. No SAR is calculated for the first price
   *    bar.
   *  - Tradestation use the closing price of the second bar. No
   *    SAR are calculated for the first price bar.
   *  - Wilder (the original author) use the SIP from the
   *    previous trade (cannot be implement here since the
   *    direction and length of the previous trade is unknonw).
   *  - The Magazine TASC seems to follow Wilder approach which
   *    is not practical here.
   *
   * TA-Lib "consume" the first price bar and use its high/low as the
   * initial SAR of the second price bar. I found that approach to be
   * the closest to Wilders idea of having the first entry day use
   * the previous extreme point, except that here the extreme point is
   * derived solely from the first price bar. I found the same approach
   * to be used by Metastock.
   */
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   *
   * Move up the start index if there is not
   * enough initial data.
   */
  if (startIdx < 1) {
    startIdx = 1;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  /* Make sure the acceleration and maximum are coherent.
   * If not, correct the acceleration.
   */
  af = optInAcceleration;
  if (af > optInMaximum) {
    optInAcceleration = optInMaximum;
    af = optInAcceleration;
  }
  /* Identify if the initial direction is long or short.
   * (ep is just used as a temp buffer here, the name
   *  of the parameter is not significant).
   */
  const _xr0 = minusDmPositional(startIdx, startIdx, inHigh, inLow, 1, ep_temp, core);
  tempInt.value = _xr0.begIdx;
  tempInt.value = _xr0.count;
  if (ep_temp[0] > 0) {
    isLong = 0;
  } else {
    isLong = 1;
  }
  outBegIdx.value = startIdx;
  outIdx = 0;
  /* Write the first SAR. */
  todayIdx = startIdx;
  newHigh = inHigh[todayIdx - 1];
  newLow = inLow[todayIdx - 1];
  if (isLong === 1) {
    ep = inHigh[todayIdx];
    sar = newLow;
  } else {
    ep = inLow[todayIdx];
    sar = newHigh;
  }
  /* Cheat on the newLow and newHigh for the
   * first iteration.
   */
  newLow = inLow[todayIdx];
  newHigh = inHigh[todayIdx];
  while (todayIdx <= endIdx) {
    prevLow = newLow;
    prevHigh = newHigh;
    newLow = inLow[todayIdx];
    newHigh = inHigh[todayIdx];
    todayIdx += 1;
    if (isLong === 1) {
      /* Switch to short if the low penetrates the SAR value. */
      if (newLow <= sar) {
        /* Switch and Overide the SAR with the ep */
        isLong = 0;
        sar = ep;
        /* Make sure the overide SAR is within
         * yesterday's and today's range.
         */
        if (sar < prevHigh) {
          sar = prevHigh;
        }
        if (sar < newHigh) {
          sar = newHigh;
        }
        /* Output the overide SAR */
        outReal[outIdx++] = sar;
        /* Adjust af and ep */
        af = optInAcceleration;
        ep = newLow;
        /* Calculate the new SAR */
        {
          const _a: number = af, _b: number = ep - sar, _c: number = sar;
          const _p = _a * _b;
          let _t = 134217729 * _a;
          const _ah = _t - (_t - _a), _al = _a - _ah;
          _t = 134217729 * _b;
          const _bh = _t - (_t - _b), _bl = _b - _bh;
          const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
          const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
          const _v = _tl + _pl, _v2 = _v + _v;
          const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
          sar = _w && (_pl === 0 || _v === 0) ? _th
            : _w && _v2 !== (_th + _v2) - _th ? _th + _v
            : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
        }
        /* Make sure the new SAR is within
         * yesterday's and today's range.
         */
        if (sar < prevHigh) {
          sar = prevHigh;
        }
        if (sar < newHigh) {
          sar = newHigh;
        }
      } else {
        /* No switch */
        /* Output the SAR (was calculated in the previous iteration) */
        outReal[outIdx++] = sar;
        /* Adjust af and ep. */
        if (newHigh > ep) {
          ep = newHigh;
          af += optInAcceleration;
          if (af > optInMaximum) {
            af = optInMaximum;
          }
        }
        /* Calculate the new SAR */
        {
          const _a: number = af, _b: number = ep - sar, _c: number = sar;
          const _p = _a * _b;
          let _t = 134217729 * _a;
          const _ah = _t - (_t - _a), _al = _a - _ah;
          _t = 134217729 * _b;
          const _bh = _t - (_t - _b), _bl = _b - _bh;
          const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
          const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
          const _v = _tl + _pl, _v2 = _v + _v;
          const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
          sar = _w && (_pl === 0 || _v === 0) ? _th
            : _w && _v2 !== (_th + _v2) - _th ? _th + _v
            : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
        }
        /* Make sure the new SAR is within
         * yesterday's and today's range.
         */
        if (sar > prevLow) {
          sar = prevLow;
        }
        if (sar > newLow) {
          sar = newLow;
        }
      }
    /* Switch to long if the high penetrates the SAR value. */
    } else if (newHigh >= sar) {
      /* Switch and Overide the SAR with the ep */
      isLong = 1;
      sar = ep;
      /* Make sure the overide SAR is within
       * yesterday's and today's range.
       */
      if (sar > prevLow) {
        sar = prevLow;
      }
      if (sar > newLow) {
        sar = newLow;
      }
      /* Output the overide SAR */
      outReal[outIdx++] = sar;
      /* Adjust af and ep */
      af = optInAcceleration;
      ep = newHigh;
      /* Calculate the new SAR */
      {
        const _a: number = af, _b: number = ep - sar, _c: number = sar;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        sar = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      /* Make sure the new SAR is within
       * yesterday's and today's range.
       */
      if (sar > prevLow) {
        sar = prevLow;
      }
      if (sar > newLow) {
        sar = newLow;
      }
    } else {
      /* No switch */
      /* Output the SAR (was calculated in the previous iteration) */
      outReal[outIdx++] = sar;
      /* Adjust af and ep. */
      if (newLow < ep) {
        ep = newLow;
        af += optInAcceleration;
        if (af > optInMaximum) {
          af = optInMaximum;
        }
      }
      /* Calculate the new SAR */
      {
        const _a: number = af, _b: number = ep - sar, _c: number = sar;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        sar = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      /* Make sure the new SAR is within
       * yesterday's and today's range.
       */
      if (sar < prevHigh) {
        sar = prevHigh;
      }
      if (sar < newHigh) {
        sar = newHigh;
      }
    }
  }
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['acceleration', Core.REAL_DEFAULT, 0e0, Core.REAL_MAX, 'in 0..3e37'], ['maximum', Core.REAL_DEFAULT, 0e0, Core.REAL_MAX, 'in 0..3e37']] as const;

/** @internal */
export function sarPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInAcceleration: number,
  optInMaximum: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('SAR', 'startIdx', startIdx);
  endIdx = requireInt32('SAR', 'endIdx', endIdx);
  optInAcceleration = requireReal('SAR', 'acceleration', optInAcceleration);
  optInMaximum = requireReal('SAR', 'maximum', optInMaximum);
  requireIndexRange('SAR', startIdx, endIdx);
  const guardLookback = sarLookbackPositional(optInAcceleration, optInMaximum, core);
  if (guardLookback < 0) {
    throw rejectedParams('SAR', PARAM_RANGES, optInAcceleration, optInMaximum);
  }
  const guardStart = clampedStart('SAR', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('SAR', 'prices.high', inHigh, guardInLen);
  requireLength('SAR', 'prices.low', inLow, guardInLen);
  requireOutput('SAR', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlapsPartly(outReal, inHighF64)) {
    throw sharedMemory('SAR', 'outReal', 'prices.high');
  }
  if (overlapsPartly(outReal, inLowF64)) {
    throw sharedMemory('SAR', 'outReal', 'prices.low');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = sarImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInAcceleration, optInMaximum, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('SAR', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link sar}, {@link sarLookback}, {@link sarDisplayShift},
 * {@link sarOpen} and {@link sarOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface SarOptions {
  /**
   * Step added to the acceleration factor on each new extreme point (default 0.02;
   * minimum 0; `Core.REAL_DEFAULT` selects the default).
   */
  acceleration?: number | undefined;
  /**
   * Ceiling on the acceleration factor (default 0.2; minimum 0; `Core.REAL_DEFAULT`
   * selects the default).
   */
  maximum?: number | undefined;
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

/** What {@link sar} returns, and {@link sarOpenAndFill} with its stream. */
export interface SarResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Parabolic SAR stop/reverse level per bar. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link sar} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SarOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function sarLookback(options?: SarOptions): number {
  const o = optionsOf('SAR lookback', options);
  return sarLookbackPositional(orDefault(o.acceleration, Core.REAL_DEFAULT), orDefault(o.maximum, Core.REAL_DEFAULT), coreOf('SAR lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link sar}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SarOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function sarDisplayShift(outputIdx: number, options?: SarOptions): number {
  const o = optionsOf('SAR displayShift', options);
  return sarDisplayShiftPositional(orDefault(o.acceleration, Core.REAL_DEFAULT), orDefault(o.maximum, Core.REAL_DEFAULT), outputIdx, coreOf('SAR displayShift', o.core));
}

/**
 * Wilder's Parabolic SAR (Stop And Reverse): a trailing stop/reverse level that
 * accelerates toward price via an acceleration factor. Signals trend direction and
 * trailing exit points. SAR below price = uptrend (long); SAR above price =
 * downtrend (short). Price crossing SAR flips direction.
 *
 * Formula and more info at
 * [ta-lib.org/functions/sar](https://ta-lib.org/functions/sar).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link sarLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `high`: High
 *   price of each bar. `low`: Low price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link SarOptions}.
 * @returns `begIdx` and the values; see {@link SarResult}. Each output is a view
 *   of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, a real parameter that is not a number, a parameter outside its range, an
 *   input too short for the range, a buffer that is empty, too short or of the wrong
 *   type or a buffer sharing memory with a `Float64Array` input it is not. Checked
 *   before anything is written.
 *
 * @example
 * ```ts
 * import { sar } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = sar({ high, low }, {
 *   acceleration: 0.02,
 *   maximum: 0.2,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`sarext`](https://ta-lib.org/functions/sarext)
 * @see [`minusDm`](https://ta-lib.org/functions/minus_dm)
 * @see [`plusDm`](https://ta-lib.org/functions/plus_dm)
 */
export function sar(prices: { high: ArrayLike<number>; low: ArrayLike<number> }, options?: SarOptions): SarResult {
  requireArgument('SAR', 'prices', prices);
  const o = optionsOf('SAR', options);
  if (o.endIdx === undefined) {
    requireSeries('SAR', 'prices.high', prices.high);
    requireSeries('SAR', 'prices.low', prices.low);
  }
  const optInAcceleration = orDefault(o.acceleration, Core.REAL_DEFAULT);
  const optInMaximum = orDefault(o.maximum, Core.REAL_DEFAULT);
  const core = coreOf('SAR', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, sarLookbackPositional(optInAcceleration, optInMaximum, core), prices.high, prices.low);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = sarPositional(startIdx, endIdx, prices.high, prices.low, optInAcceleration, optInMaximum, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live SAR stream: one value per closed bar, bit-identical to {@link sar} over
 * the same series. Open it with {@link sarOpen} or {@link sarOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/sar](https://ta-lib.org/functions/sar).
 */
export class SarStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInAcceleration: number = 0;
  /** @internal */
  optInMaximum: number = 0;
  /** @internal */
  isLong: number = 0;
  /** @internal */
  newHigh: number = 0;
  /** @internal */
  newLow: number = 0;
  /** @internal */
  af: number = 0;
  /** @internal */
  ep: number = 0;
  /** @internal */
  sar: number = 0;
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
  static of(core: Core): SarStream {
    return new SarStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link sar} reports them over the same bars. Each accepted `update` and each
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
      throw failure('SAR advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('SAR update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('SAR update', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    sarStepImpl(this, inHigh, inLow);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number): number {
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('SAR peek', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    const sp = this;
    let prevHigh = 0;
    let prevLow = 0;
    let af = sp.af;
    let cur_outReal = 0;
    let ep = sp.ep;
    let isLong = sp.isLong;
    let newHigh = sp.newHigh;
    let newLow = sp.newLow;
    let sar = sp.sar;
    prevLow = newLow;
    prevHigh = newHigh;
    newLow = inLow;
    newHigh = inHigh;
    if (isLong === 1) {
      /* Switch to short if the low penetrates the SAR value. */
      if (newLow <= sar) {
        /* Switch and Overide the SAR with the ep */
        isLong = 0;
        sar = ep;
        /* Make sure the overide SAR is within
         * yesterday's and today's range.
         */
        if (sar < prevHigh) {
          sar = prevHigh;
        }
        if (sar < newHigh) {
          sar = newHigh;
        }
        /* Output the overide SAR */
        cur_outReal = sar;
        /* Adjust af and ep */
        af = sp.optInAcceleration;
        ep = newLow;
        /* Calculate the new SAR */
        {
          const _a: number = af, _b: number = ep - sar, _c: number = sar;
          const _p = _a * _b;
          let _t = 134217729 * _a;
          const _ah = _t - (_t - _a), _al = _a - _ah;
          _t = 134217729 * _b;
          const _bh = _t - (_t - _b), _bl = _b - _bh;
          const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
          const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
          const _v = _tl + _pl, _v2 = _v + _v;
          const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
          sar = _w && (_pl === 0 || _v === 0) ? _th
            : _w && _v2 !== (_th + _v2) - _th ? _th + _v
            : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
        }
        /* Make sure the new SAR is within
         * yesterday's and today's range.
         */
        if (sar < prevHigh) {
          sar = prevHigh;
        }
        if (sar < newHigh) {
          sar = newHigh;
        }
      } else {
        /* No switch */
        /* Output the SAR (was calculated in the previous iteration) */
        cur_outReal = sar;
        /* Adjust af and ep. */
        if (newHigh > ep) {
          ep = newHigh;
          af += sp.optInAcceleration;
          if (af > sp.optInMaximum) {
            af = sp.optInMaximum;
          }
        }
        /* Calculate the new SAR */
        {
          const _a: number = af, _b: number = ep - sar, _c: number = sar;
          const _p = _a * _b;
          let _t = 134217729 * _a;
          const _ah = _t - (_t - _a), _al = _a - _ah;
          _t = 134217729 * _b;
          const _bh = _t - (_t - _b), _bl = _b - _bh;
          const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
          const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
          const _v = _tl + _pl, _v2 = _v + _v;
          const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
          sar = _w && (_pl === 0 || _v === 0) ? _th
            : _w && _v2 !== (_th + _v2) - _th ? _th + _v
            : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
        }
        /* Make sure the new SAR is within
         * yesterday's and today's range.
         */
        if (sar > prevLow) {
          sar = prevLow;
        }
        if (sar > newLow) {
          sar = newLow;
        }
      }
    /* Switch to long if the high penetrates the SAR value. */
    } else if (newHigh >= sar) {
      /* Switch and Overide the SAR with the ep */
      isLong = 1;
      sar = ep;
      /* Make sure the overide SAR is within
       * yesterday's and today's range.
       */
      if (sar > prevLow) {
        sar = prevLow;
      }
      if (sar > newLow) {
        sar = newLow;
      }
      /* Output the overide SAR */
      cur_outReal = sar;
      /* Adjust af and ep */
      af = sp.optInAcceleration;
      ep = newHigh;
      /* Calculate the new SAR */
      {
        const _a: number = af, _b: number = ep - sar, _c: number = sar;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        sar = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      /* Make sure the new SAR is within
       * yesterday's and today's range.
       */
      if (sar > prevLow) {
        sar = prevLow;
      }
      if (sar > newLow) {
        sar = newLow;
      }
    } else {
      /* No switch */
      /* Output the SAR (was calculated in the previous iteration) */
      cur_outReal = sar;
      /* Adjust af and ep. */
      if (newLow < ep) {
        ep = newLow;
        af += sp.optInAcceleration;
        if (af > sp.optInMaximum) {
          af = sp.optInMaximum;
        }
      }
      /* Calculate the new SAR */
      {
        const _a: number = af, _b: number = ep - sar, _c: number = sar;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        sar = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      /* Make sure the new SAR is within
       * yesterday's and today's range.
       */
      if (sar < prevHigh) {
        sar = prevHigh;
      }
      if (sar < newHigh) {
        sar = newHigh;
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
  clone(): SarStream {
    const c = new SarStream(this.core);
    c.optInAcceleration = this.optInAcceleration;
    c.optInMaximum = this.optInMaximum;
    c.isLong = this.isLong;
    c.newHigh = this.newHigh;
    c.newLow = this.newLow;
    c.af = this.af;
    c.ep = this.ep;
    c.sar = this.sar;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function sarStepImpl(sp: SarStream, inHigh: number, inLow: number): void {
  let prevHigh = 0;
  let prevLow = 0;
  prevLow = sp.newLow;
  prevHigh = sp.newHigh;
  sp.newLow = inLow;
  sp.newHigh = inHigh;
  if (sp.isLong === 1) {
    /* Switch to short if the low penetrates the SAR value. */
    if (sp.newLow <= sp.sar) {
      /* Switch and Overide the SAR with the ep */
      sp.isLong = 0;
      sp.sar = sp.ep;
      /* Make sure the overide SAR is within
       * yesterday's and today's range.
       */
      if (sp.sar < prevHigh) {
        sp.sar = prevHigh;
      }
      if (sp.sar < sp.newHigh) {
        sp.sar = sp.newHigh;
      }
      /* Output the overide SAR */
      sp.cur_outReal = sp.sar;
      /* Adjust af and ep */
      sp.af = sp.optInAcceleration;
      sp.ep = sp.newLow;
      /* Calculate the new SAR */
      {
        const _a: number = sp.af, _b: number = sp.ep - sp.sar, _c: number = sp.sar;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        sp.sar = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      /* Make sure the new SAR is within
       * yesterday's and today's range.
       */
      if (sp.sar < prevHigh) {
        sp.sar = prevHigh;
      }
      if (sp.sar < sp.newHigh) {
        sp.sar = sp.newHigh;
      }
    } else {
      /* No switch */
      /* Output the SAR (was calculated in the previous iteration) */
      sp.cur_outReal = sp.sar;
      /* Adjust af and ep. */
      if (sp.newHigh > sp.ep) {
        sp.ep = sp.newHigh;
        sp.af += sp.optInAcceleration;
        if (sp.af > sp.optInMaximum) {
          sp.af = sp.optInMaximum;
        }
      }
      /* Calculate the new SAR */
      {
        const _a: number = sp.af, _b: number = sp.ep - sp.sar, _c: number = sp.sar;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        sp.sar = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      /* Make sure the new SAR is within
       * yesterday's and today's range.
       */
      if (sp.sar > prevLow) {
        sp.sar = prevLow;
      }
      if (sp.sar > sp.newLow) {
        sp.sar = sp.newLow;
      }
    }
  /* Switch to long if the high penetrates the SAR value. */
  } else if (sp.newHigh >= sp.sar) {
    /* Switch and Overide the SAR with the ep */
    sp.isLong = 1;
    sp.sar = sp.ep;
    /* Make sure the overide SAR is within
     * yesterday's and today's range.
     */
    if (sp.sar > prevLow) {
      sp.sar = prevLow;
    }
    if (sp.sar > sp.newLow) {
      sp.sar = sp.newLow;
    }
    /* Output the overide SAR */
    sp.cur_outReal = sp.sar;
    /* Adjust af and ep */
    sp.af = sp.optInAcceleration;
    sp.ep = sp.newHigh;
    /* Calculate the new SAR */
    {
      const _a: number = sp.af, _b: number = sp.ep - sp.sar, _c: number = sp.sar;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      sp.sar = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    /* Make sure the new SAR is within
     * yesterday's and today's range.
     */
    if (sp.sar > prevLow) {
      sp.sar = prevLow;
    }
    if (sp.sar > sp.newLow) {
      sp.sar = sp.newLow;
    }
  } else {
    /* No switch */
    /* Output the SAR (was calculated in the previous iteration) */
    sp.cur_outReal = sp.sar;
    /* Adjust af and ep. */
    if (sp.newLow < sp.ep) {
      sp.ep = sp.newLow;
      sp.af += sp.optInAcceleration;
      if (sp.af > sp.optInMaximum) {
        sp.af = sp.optInMaximum;
      }
    }
    /* Calculate the new SAR */
    {
      const _a: number = sp.af, _b: number = sp.ep - sp.sar, _c: number = sp.sar;
      const _p = _a * _b;
      let _t = 134217729 * _a;
      const _ah = _t - (_t - _a), _al = _a - _ah;
      _t = 134217729 * _b;
      const _bh = _t - (_t - _b), _bl = _b - _bh;
      const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
      const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
      const _v = _tl + _pl, _v2 = _v + _v;
      const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
      sp.sar = _w && (_pl === 0 || _v === 0) ? _th
        : _w && _v2 !== (_th + _v2) - _th ? _th + _v
        : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
    }
    /* Make sure the new SAR is within
     * yesterday's and today's range.
     */
    if (sp.sar < prevHigh) {
      sp.sar = prevHigh;
    }
    if (sp.sar < sp.newHigh) {
      sp.sar = sp.newHigh;
    }
  }
}

function sarOpenImpl(
  sp: SarStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInAcceleration: number,
  optInMaximum: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let isLong = 0;
  let todayIdx = 0;
  let outIdx = 0;
  const tempInt = new MInteger();
  let newHigh = 0;
  let newLow = 0;
  let prevHigh = 0;
  let prevLow = 0;
  let af = 0;
  let ep = 0;
  let sar = 0;
  const ep_temp = new Float64Array(1);
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInAcceleration === Core.REAL_DEFAULT) {
    optInAcceleration = 0.02;
  } else if (!(optInAcceleration >= 0e0 && optInAcceleration <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInMaximum === Core.REAL_DEFAULT) {
    optInMaximum = 0.2;
  } else if (!(optInMaximum >= 0e0 && optInMaximum <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* > 0 indicates long. == 0 indicates short */
  /* Implementation of the SAR has been a little bit open to interpretation
   * since Wilder (the original author) did not define a precise algorithm
   * on how to bootstrap the algorithm. Take any existing software application
   * and you will see slight variation on how the algorithm was adapted.
   *
   * What is the initial trade direction? Long or short?
   * ===================================================
   * The interpretation of what should be the initial SAR values is
   * open to interpretation, particularly since the caller to the function
   * does not specify the initial direction of the trade.
   *
   * In TA-Lib, the following logic is used:
   *  - Calculate +DM and -DM between the first and
   *    second bar. The highest directional indication will
   *    indicate the assumed direction of the trade for the second
   *    price bar.
   *  - In the case of a tie between +DM and -DM,
   *    the direction is LONG by default.
   *
   * What is the initial "extreme point" and thus SAR?
   * =================================================
   * The following shows how different people took different approach:
   *  - Metastock use the first price bar high/low depending of
   *    the direction. No SAR is calculated for the first price
   *    bar.
   *  - Tradestation use the closing price of the second bar. No
   *    SAR are calculated for the first price bar.
   *  - Wilder (the original author) use the SIP from the
   *    previous trade (cannot be implement here since the
   *    direction and length of the previous trade is unknonw).
   *  - The Magazine TASC seems to follow Wilder approach which
   *    is not practical here.
   *
   * TA-Lib "consume" the first price bar and use its high/low as the
   * initial SAR of the second price bar. I found that approach to be
   * the closest to Wilders idea of having the first entry day use
   * the previous extreme point, except that here the extreme point is
   * derived solely from the first price bar. I found the same approach
   * to be used by Metastock.
   */
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   *
   * Move up the start index if there is not
   * enough initial data.
   */
  if (startIdx < 1) {
    startIdx = 1;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Make sure the acceleration and maximum are coherent.
   * If not, correct the acceleration.
   */
  af = optInAcceleration;
  if (af > optInMaximum) {
    optInAcceleration = optInMaximum;
    af = optInAcceleration;
  }
  /* Identify if the initial direction is long or short.
   * (ep is just used as a temp buffer here, the name
   *  of the parameter is not significant).
   */
  const _xr8 = minusDmPositional(startIdx, startIdx, inHigh, inLow, 1, ep_temp, core);
  tempInt.value = _xr8.begIdx;
  tempInt.value = _xr8.count;
  if (ep_temp[0] > 0) {
    isLong = 0;
  } else {
    isLong = 1;
  }
  outBegIdx.value = startIdx;
  outIdx = 0;
  /* Write the first SAR. */
  todayIdx = startIdx;
  newHigh = inHigh[todayIdx - 1];
  newLow = inLow[todayIdx - 1];
  if (isLong === 1) {
    ep = inHigh[todayIdx];
    sar = newLow;
  } else {
    ep = inLow[todayIdx];
    sar = newHigh;
  }
  /* Cheat on the newLow and newHigh for the
   * first iteration.
   */
  newLow = inLow[todayIdx];
  newHigh = inHigh[todayIdx];
  while (todayIdx <= endIdx) {
    prevLow = newLow;
    prevHigh = newHigh;
    newLow = inLow[todayIdx];
    newHigh = inHigh[todayIdx];
    todayIdx += 1;
    if (isLong === 1) {
      /* Switch to short if the low penetrates the SAR value. */
      if (newLow <= sar) {
        /* Switch and Overide the SAR with the ep */
        isLong = 0;
        sar = ep;
        /* Make sure the overide SAR is within
         * yesterday's and today's range.
         */
        if (sar < prevHigh) {
          sar = prevHigh;
        }
        if (sar < newHigh) {
          sar = newHigh;
        }
        /* Output the overide SAR */
        outReal[((outIdx++ * outStride) | 0)] = sar;
        /* Adjust af and ep */
        af = optInAcceleration;
        ep = newLow;
        /* Calculate the new SAR */
        {
          const _a: number = af, _b: number = ep - sar, _c: number = sar;
          const _p = _a * _b;
          let _t = 134217729 * _a;
          const _ah = _t - (_t - _a), _al = _a - _ah;
          _t = 134217729 * _b;
          const _bh = _t - (_t - _b), _bl = _b - _bh;
          const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
          const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
          const _v = _tl + _pl, _v2 = _v + _v;
          const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
          sar = _w && (_pl === 0 || _v === 0) ? _th
            : _w && _v2 !== (_th + _v2) - _th ? _th + _v
            : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
        }
        /* Make sure the new SAR is within
         * yesterday's and today's range.
         */
        if (sar < prevHigh) {
          sar = prevHigh;
        }
        if (sar < newHigh) {
          sar = newHigh;
        }
      } else {
        /* No switch */
        /* Output the SAR (was calculated in the previous iteration) */
        outReal[((outIdx++ * outStride) | 0)] = sar;
        /* Adjust af and ep. */
        if (newHigh > ep) {
          ep = newHigh;
          af += optInAcceleration;
          if (af > optInMaximum) {
            af = optInMaximum;
          }
        }
        /* Calculate the new SAR */
        {
          const _a: number = af, _b: number = ep - sar, _c: number = sar;
          const _p = _a * _b;
          let _t = 134217729 * _a;
          const _ah = _t - (_t - _a), _al = _a - _ah;
          _t = 134217729 * _b;
          const _bh = _t - (_t - _b), _bl = _b - _bh;
          const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
          const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
          const _v = _tl + _pl, _v2 = _v + _v;
          const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
          sar = _w && (_pl === 0 || _v === 0) ? _th
            : _w && _v2 !== (_th + _v2) - _th ? _th + _v
            : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
        }
        /* Make sure the new SAR is within
         * yesterday's and today's range.
         */
        if (sar > prevLow) {
          sar = prevLow;
        }
        if (sar > newLow) {
          sar = newLow;
        }
      }
    /* Switch to long if the high penetrates the SAR value. */
    } else if (newHigh >= sar) {
      /* Switch and Overide the SAR with the ep */
      isLong = 1;
      sar = ep;
      /* Make sure the overide SAR is within
       * yesterday's and today's range.
       */
      if (sar > prevLow) {
        sar = prevLow;
      }
      if (sar > newLow) {
        sar = newLow;
      }
      /* Output the overide SAR */
      outReal[((outIdx++ * outStride) | 0)] = sar;
      /* Adjust af and ep */
      af = optInAcceleration;
      ep = newHigh;
      /* Calculate the new SAR */
      {
        const _a: number = af, _b: number = ep - sar, _c: number = sar;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        sar = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      /* Make sure the new SAR is within
       * yesterday's and today's range.
       */
      if (sar > prevLow) {
        sar = prevLow;
      }
      if (sar > newLow) {
        sar = newLow;
      }
    } else {
      /* No switch */
      /* Output the SAR (was calculated in the previous iteration) */
      outReal[((outIdx++ * outStride) | 0)] = sar;
      /* Adjust af and ep. */
      if (newLow < ep) {
        ep = newLow;
        af += optInAcceleration;
        if (af > optInMaximum) {
          af = optInMaximum;
        }
      }
      /* Calculate the new SAR */
      {
        const _a: number = af, _b: number = ep - sar, _c: number = sar;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        sar = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      /* Make sure the new SAR is within
       * yesterday's and today's range.
       */
      if (sar < prevHigh) {
        sar = prevHigh;
      }
      if (sar < newHigh) {
        sar = newHigh;
      }
    }
  }
  outNBElement.value = outIdx;
  sp.optInAcceleration = optInAcceleration;
  sp.optInMaximum = optInMaximum;
  sp.isLong = isLong;
  sp.newHigh = newHigh;
  sp.newLow = newLow;
  sp.af = af;
  sp.ep = ep;
  sp.sar = sar;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function sarOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInAcceleration: number,
  optInMaximum: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): SarStream {
  const sp = SarStream.of(core);
  const retCode = sarOpenImpl(sp, inHigh, inLow, startIdx, optInAcceleration, optInMaximum, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SAR openAndFill', inHigh.length, startIdx, sarLookbackPositional(optInAcceleration, optInMaximum, core));
  }
  if (retCode === RetCode.BAD_PARAM && sarLookbackPositional(optInAcceleration, optInMaximum, core) < 0) {
    throw rejectedParams('SAR openAndFill', PARAM_RANGES, optInAcceleration, optInMaximum);
  }
  throw streamFailure('SAR openAndFill', retCode);
}

/** @internal */
export function sarOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInAcceleration: number,
  optInMaximum: number,
): SarStream {
  const sp = SarStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = sarOpenImpl(sp, inHigh, inLow, startIdx, optInAcceleration, optInMaximum, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SAR open', inHigh.length, startIdx, sarLookbackPositional(optInAcceleration, optInMaximum, core));
  }
  if (retCode === RetCode.BAD_PARAM && sarLookbackPositional(optInAcceleration, optInMaximum, core) < 0) {
    throw rejectedParams('SAR open', PARAM_RANGES, optInAcceleration, optInMaximum);
  }
  throw streamFailure('SAR open', retCode);
}

/** @internal */
export function sarOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInAcceleration: number,
  optInMaximum: number,
  core: Core = Core.DEFAULT,
): SarStream {
  optInAcceleration = requireReal('SAR open', 'acceleration', optInAcceleration);
  optInMaximum = requireReal('SAR open', 'maximum', optInMaximum);
  requireArgument('SAR open', 'prices.high', inHigh);
  requireHistory('SAR open', inHigh.length);
  requireArgument('SAR open', 'prices.low', inLow);
  requireHistoryLength('SAR open', 'prices.low', inLow.length, inHigh.length);
  return sarOpenInternal(core, float64(inHigh), float64(inLow), 0, optInAcceleration, optInMaximum);
}

/** @internal */
export function sarOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInAcceleration: number,
  optInMaximum: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): SarStream {
  optInAcceleration = requireReal('SAR openAndFill', 'acceleration', optInAcceleration);
  optInMaximum = requireReal('SAR openAndFill', 'maximum', optInMaximum);
  requireArgument('SAR openAndFill', 'prices.high', inHigh);
  requireHistory('SAR openAndFill', inHigh.length);
  requireArgument('SAR openAndFill', 'prices.low', inLow);
  const guardLookback = sarLookbackPositional(optInAcceleration, optInMaximum, core);
  if (guardLookback < 0) {
    throw rejectedParams('SAR openAndFill', PARAM_RANGES, optInAcceleration, optInMaximum);
  }
  const guardOutLen = openFillCount('SAR openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('SAR openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireOutput('SAR openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('SAR openAndFill', 'outReal', 'prices.high');
  }
  if (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('SAR openAndFill', 'outReal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outReal, inHighF64)) {
    throw sharedMemory('SAR openAndFill', 'outReal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outReal, inLowF64)) {
    throw sharedMemory('SAR openAndFill', 'outReal', 'prices.low');
  }
  return sarOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInAcceleration, optInMaximum, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live SAR stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link sar} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SarOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `sarLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   a real parameter that is not a number, a parameter outside its range or
 *   histories of different lengths.
 *
 * @example
 * ```ts
 * import { sarOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = sarOpen({ high, low }, { acceleration: 0.02, maximum: 0.2 });
 * const peeked = stream.peek(101.4, 99.1); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1); // the bar closes
 * ```
 */
export function sarOpen(prices: { high: ArrayLike<number>; low: ArrayLike<number> }, options?: SarOptions): SarStream {
  requireArgument('SAR open', 'prices', prices);
  const o = optionsOf('SAR open', options);
  const optInAcceleration = orDefault(o.acceleration, Core.REAL_DEFAULT);
  const optInMaximum = orDefault(o.maximum, Core.REAL_DEFAULT);
  const core = coreOf('SAR open', o.core);
  return sarOpenPositional(prices.high, prices.low, optInAcceleration, optInMaximum, core);
}

/**
 * {@link sarOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link sar}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link SarOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link sar} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `sarLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   a real parameter that is not a number, a parameter outside its range, histories
 *   of different lengths or a buffer too short, of the wrong type or starting where
 *   another buffer or a `Float64Array` input starts, or, on a history long enough to
 *   open, one sharing memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { sarOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = sarOpenAndFill({ high, low }, {
 *   acceleration: 0.02,
 *   maximum: 0.2,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function sarOpenAndFill(prices: { high: ArrayLike<number>; low: ArrayLike<number> }, options?: SarOptions): SarResult & { stream: SarStream } {
  requireArgument('SAR openAndFill', 'prices', prices);
  const o = optionsOf('SAR openAndFill', options);
  const optInAcceleration = orDefault(o.acceleration, Core.REAL_DEFAULT);
  const optInMaximum = orDefault(o.maximum, Core.REAL_DEFAULT);
  const core = coreOf('SAR openAndFill', o.core);
  const capacity = fillCapacity(sarLookbackPositional(optInAcceleration, optInMaximum, core), prices.high, prices.low);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = sarOpenAndFillPositional(prices.high, prices.low, optInAcceleration, optInMaximum, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
