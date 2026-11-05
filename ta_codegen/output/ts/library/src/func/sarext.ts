/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  PP       Peter Pudaite
 *  CF       Christo Fogelberg
 *
 * Change history:
 *
 *  MMDDYY BY    Description
 *  -------------------------------------------------------------------
 *  120802 MF    Template creation.
 *  091503 PP    Reworked TA_SAR to allow customisation of more SAR params.
 *  092103 MF    Some changes related on first round of tests
 *  092303 PP    Minor bug fixes.
 *  122104 MF,CF Fix#1089506 for out-of-bound access to ep_temp.
 *  082726 MF,CC Answer a rejected minus_dm before reading ep_temp, not after:
 *               the read was of an uninitialised local.
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireReal, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { minusDmPositional } from './minusDm.ts';

/** @internal */
export function sarextLookbackPositional(optInStartValue: number, optInOffsetOnReverse: number, optInAccelerationInitLong: number, optInAccelerationLong: number, optInAccelerationMaxLong: number, optInAccelerationInitShort: number, optInAccelerationShort: number, optInAccelerationMaxShort: number, _core: Core = Core.DEFAULT): number {
  if (typeof optInStartValue !== 'number') {
    return -1;
  }
  if (typeof optInOffsetOnReverse !== 'number') {
    return -1;
  }
  if (typeof optInAccelerationInitLong !== 'number') {
    return -1;
  }
  if (typeof optInAccelerationLong !== 'number') {
    return -1;
  }
  if (typeof optInAccelerationMaxLong !== 'number') {
    return -1;
  }
  if (typeof optInAccelerationInitShort !== 'number') {
    return -1;
  }
  if (typeof optInAccelerationShort !== 'number') {
    return -1;
  }
  if (typeof optInAccelerationMaxShort !== 'number') {
    return -1;
  }
  if (optInStartValue === Core.REAL_DEFAULT) {
    optInStartValue = 0.0;
  } else if (!(optInStartValue >= Core.REAL_MIN && optInStartValue <= Core.REAL_MAX)) {
    return -1;
  }
  if (optInOffsetOnReverse === Core.REAL_DEFAULT) {
    optInOffsetOnReverse = 0.0;
  } else if (!(optInOffsetOnReverse >= 0e0 && optInOffsetOnReverse <= Core.REAL_MAX)) {
    return -1;
  }
  if (optInAccelerationInitLong === Core.REAL_DEFAULT) {
    optInAccelerationInitLong = 0.02;
  } else if (!(optInAccelerationInitLong >= 0e0 && optInAccelerationInitLong <= Core.REAL_MAX)) {
    return -1;
  }
  if (optInAccelerationLong === Core.REAL_DEFAULT) {
    optInAccelerationLong = 0.02;
  } else if (!(optInAccelerationLong >= 0e0 && optInAccelerationLong <= Core.REAL_MAX)) {
    return -1;
  }
  if (optInAccelerationMaxLong === Core.REAL_DEFAULT) {
    optInAccelerationMaxLong = 0.2;
  } else if (!(optInAccelerationMaxLong >= 0e0 && optInAccelerationMaxLong <= Core.REAL_MAX)) {
    return -1;
  }
  if (optInAccelerationInitShort === Core.REAL_DEFAULT) {
    optInAccelerationInitShort = 0.02;
  } else if (!(optInAccelerationInitShort >= 0e0 && optInAccelerationInitShort <= Core.REAL_MAX)) {
    return -1;
  }
  if (optInAccelerationShort === Core.REAL_DEFAULT) {
    optInAccelerationShort = 0.02;
  } else if (!(optInAccelerationShort >= 0e0 && optInAccelerationShort <= Core.REAL_MAX)) {
    return -1;
  }
  if (optInAccelerationMaxShort === Core.REAL_DEFAULT) {
    optInAccelerationMaxShort = 0.2;
  } else if (!(optInAccelerationMaxShort >= 0e0 && optInAccelerationMaxShort <= Core.REAL_MAX)) {
    return -1;
  }
  /* SAR always sacrifices one price bar to establish the
   * initial extreme price.
   */
  return 1;
}

/** @internal */
export function sarextDisplayShiftPositional(optInStartValue: number, optInOffsetOnReverse: number, optInAccelerationInitLong: number, optInAccelerationLong: number, optInAccelerationMaxLong: number, optInAccelerationInitShort: number, optInAccelerationShort: number, optInAccelerationMaxShort: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (sarextLookbackPositional(optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function sarextImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  optInStartValue: number,
  optInOffsetOnReverse: number,
  optInAccelerationInitLong: number,
  optInAccelerationLong: number,
  optInAccelerationMaxLong: number,
  optInAccelerationInitShort: number,
  optInAccelerationShort: number,
  optInAccelerationMaxShort: number,
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
  let afLong = 0;
  let afShort = 0;
  let ep = 0;
  let sar = 0;
  const ep_temp = new Float64Array(1);
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInStartValue === Core.REAL_DEFAULT) {
    optInStartValue = 0.0;
  } else if (!(optInStartValue >= Core.REAL_MIN && optInStartValue <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInOffsetOnReverse === Core.REAL_DEFAULT) {
    optInOffsetOnReverse = 0.0;
  } else if (!(optInOffsetOnReverse >= 0e0 && optInOffsetOnReverse <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInAccelerationInitLong === Core.REAL_DEFAULT) {
    optInAccelerationInitLong = 0.02;
  } else if (!(optInAccelerationInitLong >= 0e0 && optInAccelerationInitLong <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInAccelerationLong === Core.REAL_DEFAULT) {
    optInAccelerationLong = 0.02;
  } else if (!(optInAccelerationLong >= 0e0 && optInAccelerationLong <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInAccelerationMaxLong === Core.REAL_DEFAULT) {
    optInAccelerationMaxLong = 0.2;
  } else if (!(optInAccelerationMaxLong >= 0e0 && optInAccelerationMaxLong <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInAccelerationInitShort === Core.REAL_DEFAULT) {
    optInAccelerationInitShort = 0.02;
  } else if (!(optInAccelerationInitShort >= 0e0 && optInAccelerationInitShort <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInAccelerationShort === Core.REAL_DEFAULT) {
    optInAccelerationShort = 0.02;
  } else if (!(optInAccelerationShort >= 0e0 && optInAccelerationShort <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInAccelerationMaxShort === Core.REAL_DEFAULT) {
    optInAccelerationMaxShort = 0.2;
  } else if (!(optInAccelerationMaxShort >= 0e0 && optInAccelerationMaxShort <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  /* > 0 indicates long. == 0 indicates short */
  /* This function is the same as TA_SAR, except that the caller has
   * greater control on the SAR dynamic and initial state.
   *
   * In additon, the TA_SAREXT returns negative values when the position
   * is short. This allow to distinguish when the SAR do actually reverse.
   */
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
   * In TA-Lib, the following default logic is used:
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
   *
   *
   * Can I force the initial SAR?
   * ============================
   * Yes. Using the optInStartValue_0 parameter:
   *  optInStartValue_0 >  0 : SAR is long at optInStartValue_0.
   *  optInStartValue_0 <  0 : SAR is short at fabs(optInStartValue_0).
   *
   * And when optInStartValue_0 == 0, the logic is the same as for TA_SAR
   * (See previous two sections).
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
  /* Check if the acceleration factors are being defined by the user.
   * Make sure the acceleration and maximum are coherent.
   * If not, correct the acceleration.
   * Default afLong = 0.02
   * Default afShort = 0.02
   */
  afLong = optInAccelerationInitLong;
  afShort = optInAccelerationInitShort;
  if (afLong > optInAccelerationMaxLong) {
    optInAccelerationInitLong = optInAccelerationMaxLong;
    afLong = optInAccelerationInitLong;
  }
  if (optInAccelerationLong > optInAccelerationMaxLong) {
    optInAccelerationLong = optInAccelerationMaxLong;
  }
  if (afShort > optInAccelerationMaxShort) {
    optInAccelerationInitShort = optInAccelerationMaxShort;
    afShort = optInAccelerationInitShort;
  }
  if (optInAccelerationShort > optInAccelerationMaxShort) {
    optInAccelerationShort = optInAccelerationMaxShort;
  }
  /* Initialise SAR calculations */
  if (optInStartValue === 0) {
    /* Default action */
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
  } else if (optInStartValue > 0) {
    /* Start Long */
    isLong = 1;
  } else {
    /* optInStartValue_0 < 0 => Start Short */
    isLong = 0;
  }
  outBegIdx.value = startIdx;
  outIdx = 0;
  /* Write the first SAR. */
  todayIdx = startIdx;
  newHigh = inHigh[todayIdx - 1];
  newLow = inLow[todayIdx - 1];
  if (optInStartValue === 0) {
    /* Default action */
    if (isLong === 1) {
      ep = inHigh[todayIdx];
      sar = newLow;
    } else {
      ep = inLow[todayIdx];
      sar = newHigh;
    }
  } else if (optInStartValue > 0) {
    /* Start Long at specified value. */
    ep = inHigh[todayIdx];
    sar = optInStartValue;
  } else {
    /* if optInStartValue < 0 => Start Short at specified value. */
    ep = inLow[todayIdx];
    sar = Math.abs(optInStartValue);
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
        if (optInOffsetOnReverse !== 0.0) {
          sar += sar * optInOffsetOnReverse;
        }
        outReal[outIdx++] = -sar;
        /* Adjust afShort and ep */
        afShort = optInAccelerationInitShort;
        ep = newLow;
        /* Calculate the new SAR */
        {
          const _a: number = afShort, _b: number = ep - sar, _c: number = sar;
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
        /* Adjust afLong and ep. */
        if (newHigh > ep) {
          ep = newHigh;
          afLong += optInAccelerationLong;
          if (afLong > optInAccelerationMaxLong) {
            afLong = optInAccelerationMaxLong;
          }
        }
        /* Calculate the new SAR */
        {
          const _a: number = afLong, _b: number = ep - sar, _c: number = sar;
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
      if (optInOffsetOnReverse !== 0.0) {
        sar -= sar * optInOffsetOnReverse;
      }
      outReal[outIdx++] = sar;
      /* Adjust afLong and ep */
      afLong = optInAccelerationInitLong;
      ep = newHigh;
      /* Calculate the new SAR */
      {
        const _a: number = afLong, _b: number = ep - sar, _c: number = sar;
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
      outReal[outIdx++] = -sar;
      /* Adjust afShort and ep. */
      if (newLow < ep) {
        ep = newLow;
        afShort += optInAccelerationShort;
        if (afShort > optInAccelerationMaxShort) {
          afShort = optInAccelerationMaxShort;
        }
      }
      /* Calculate the new SAR */
      {
        const _a: number = afShort, _b: number = ep - sar, _c: number = sar;
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

const PARAM_RANGES = [['startValue', Core.REAL_DEFAULT, Core.REAL_MIN, Core.REAL_MAX, 'in -3e37..3e37'], ['offsetOnReverse', Core.REAL_DEFAULT, 0e0, Core.REAL_MAX, 'in 0..3e37'], ['accelerationInitLong', Core.REAL_DEFAULT, 0e0, Core.REAL_MAX, 'in 0..3e37'], ['accelerationLong', Core.REAL_DEFAULT, 0e0, Core.REAL_MAX, 'in 0..3e37'], ['accelerationMaxLong', Core.REAL_DEFAULT, 0e0, Core.REAL_MAX, 'in 0..3e37'], ['accelerationInitShort', Core.REAL_DEFAULT, 0e0, Core.REAL_MAX, 'in 0..3e37'], ['accelerationShort', Core.REAL_DEFAULT, 0e0, Core.REAL_MAX, 'in 0..3e37'], ['accelerationMaxShort', Core.REAL_DEFAULT, 0e0, Core.REAL_MAX, 'in 0..3e37']] as const;

/** @internal */
export function sarextPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInStartValue: number,
  optInOffsetOnReverse: number,
  optInAccelerationInitLong: number,
  optInAccelerationLong: number,
  optInAccelerationMaxLong: number,
  optInAccelerationInitShort: number,
  optInAccelerationShort: number,
  optInAccelerationMaxShort: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('SAREXT', 'startIdx', startIdx);
  endIdx = requireInt32('SAREXT', 'endIdx', endIdx);
  optInStartValue = requireReal('SAREXT', 'startValue', optInStartValue);
  optInOffsetOnReverse = requireReal('SAREXT', 'offsetOnReverse', optInOffsetOnReverse);
  optInAccelerationInitLong = requireReal('SAREXT', 'accelerationInitLong', optInAccelerationInitLong);
  optInAccelerationLong = requireReal('SAREXT', 'accelerationLong', optInAccelerationLong);
  optInAccelerationMaxLong = requireReal('SAREXT', 'accelerationMaxLong', optInAccelerationMaxLong);
  optInAccelerationInitShort = requireReal('SAREXT', 'accelerationInitShort', optInAccelerationInitShort);
  optInAccelerationShort = requireReal('SAREXT', 'accelerationShort', optInAccelerationShort);
  optInAccelerationMaxShort = requireReal('SAREXT', 'accelerationMaxShort', optInAccelerationMaxShort);
  requireIndexRange('SAREXT', startIdx, endIdx);
  const guardLookback = sarextLookbackPositional(optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, core);
  if (guardLookback < 0) {
    throw rejectedParams('SAREXT', PARAM_RANGES, optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort);
  }
  const guardStart = clampedStart('SAREXT', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('SAREXT', 'prices.high', inHigh, guardInLen);
  requireLength('SAREXT', 'prices.low', inLow, guardInLen);
  requireOutput('SAREXT', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlapsPartly(outReal, inHighF64)) {
    throw sharedMemory('SAREXT', 'outReal', 'prices.high');
  }
  if (overlapsPartly(outReal, inLowF64)) {
    throw sharedMemory('SAREXT', 'outReal', 'prices.low');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = sarextImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('SAREXT', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link sarext}, {@link sarextLookback}, {@link sarextDisplayShift},
 * {@link sarextOpen} and {@link sarextOpenAndFill}. Every key is optional: an
 * omitted or `undefined` key takes its default, and a call ignores the keys it has
 * no use for.
 */
export interface SarextOptions {
  /**
   * Initial SAR/direction: 0 auto, >0 start long at value, \<0 start short at -value
   * (default 0; `Core.REAL_DEFAULT` selects the default).
   */
  startValue?: number | undefined;
  /**
   * Fractional offset applied to the stop on each reversal (default 0; minimum 0;
   * `Core.REAL_DEFAULT` selects the default).
   */
  offsetOnReverse?: number | undefined;
  /**
   * Initial acceleration factor when long (default 0.02; minimum 0;
   * `Core.REAL_DEFAULT` selects the default).
   */
  accelerationInitLong?: number | undefined;
  /**
   * AF increment per new long extreme (default 0.02; minimum 0; `Core.REAL_DEFAULT`
   * selects the default).
   */
  accelerationLong?: number | undefined;
  /**
   * Cap on the long acceleration factor (default 0.2; minimum 0; `Core.REAL_DEFAULT`
   * selects the default).
   */
  accelerationMaxLong?: number | undefined;
  /**
   * Initial acceleration factor when short (default 0.02; minimum 0;
   * `Core.REAL_DEFAULT` selects the default).
   */
  accelerationInitShort?: number | undefined;
  /**
   * AF increment per new short extreme (default 0.02; minimum 0; `Core.REAL_DEFAULT`
   * selects the default).
   */
  accelerationShort?: number | undefined;
  /**
   * Cap on the short acceleration factor (default 0.2; minimum 0;
   * `Core.REAL_DEFAULT` selects the default).
   */
  accelerationMaxShort?: number | undefined;
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

/** What {@link sarext} returns, and {@link sarextOpenAndFill} with its stream. */
export interface SarextResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * SAR stop level; positive while long, negative while short. `values[k]` is the
   * value at input bar `begIdx + k`.
   */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link sarext} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SarextOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function sarextLookback(options?: SarextOptions): number {
  const o = optionsOf('SAREXT lookback', options);
  return sarextLookbackPositional(orDefault(o.startValue, Core.REAL_DEFAULT), orDefault(o.offsetOnReverse, Core.REAL_DEFAULT), orDefault(o.accelerationInitLong, Core.REAL_DEFAULT), orDefault(o.accelerationLong, Core.REAL_DEFAULT), orDefault(o.accelerationMaxLong, Core.REAL_DEFAULT), orDefault(o.accelerationInitShort, Core.REAL_DEFAULT), orDefault(o.accelerationShort, Core.REAL_DEFAULT), orDefault(o.accelerationMaxShort, Core.REAL_DEFAULT), coreOf('SAREXT lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link sarext}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SarextOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function sarextDisplayShift(outputIdx: number, options?: SarextOptions): number {
  const o = optionsOf('SAREXT displayShift', options);
  return sarextDisplayShiftPositional(orDefault(o.startValue, Core.REAL_DEFAULT), orDefault(o.offsetOnReverse, Core.REAL_DEFAULT), orDefault(o.accelerationInitLong, Core.REAL_DEFAULT), orDefault(o.accelerationLong, Core.REAL_DEFAULT), orDefault(o.accelerationMaxLong, Core.REAL_DEFAULT), orDefault(o.accelerationInitShort, Core.REAL_DEFAULT), orDefault(o.accelerationShort, Core.REAL_DEFAULT), orDefault(o.accelerationMaxShort, Core.REAL_DEFAULT), outputIdx, coreOf('SAREXT displayShift', o.core));
}

/**
 * Extended Parabolic SAR (stop and reverse) giving the caller full control over
 * the initial state and separate acceleration factors for long and short
 * positions. Unlike SAR, it returns negative values while short so reversals are
 * distinguishable. Sign flip of the output marks a trend reversal (positive=long
 * stop, negative=short stop).
 *
 * Formula and more info at
 * [ta-lib.org/functions/sarext](https://ta-lib.org/functions/sarext).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link sarextLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `high`: High
 *   price of each bar. `low`: Low price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link SarextOptions}.
 * @returns `begIdx` and the values; see {@link SarextResult}. Each output is a
 *   view of its buffer, the caller's or a new one.
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
 * import { sarext } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = sarext({ high, low }, {
 *   startValue: 0,
 *   offsetOnReverse: 0,
 *   accelerationInitLong: 0.02,
 *   accelerationLong: 0.02,
 *   accelerationMaxLong: 0.2,
 *   accelerationInitShort: 0.02,
 *   accelerationShort: 0.02,
 *   accelerationMaxShort: 0.2,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`sar`](https://ta-lib.org/functions/sar)
 * @see [`minusDm`](https://ta-lib.org/functions/minus_dm)
 */
export function sarext(prices: { high: ArrayLike<number>; low: ArrayLike<number> }, options?: SarextOptions): SarextResult {
  requireArgument('SAREXT', 'prices', prices);
  const o = optionsOf('SAREXT', options);
  if (o.endIdx === undefined) {
    requireSeries('SAREXT', 'prices.high', prices.high);
    requireSeries('SAREXT', 'prices.low', prices.low);
  }
  const optInStartValue = orDefault(o.startValue, Core.REAL_DEFAULT);
  const optInOffsetOnReverse = orDefault(o.offsetOnReverse, Core.REAL_DEFAULT);
  const optInAccelerationInitLong = orDefault(o.accelerationInitLong, Core.REAL_DEFAULT);
  const optInAccelerationLong = orDefault(o.accelerationLong, Core.REAL_DEFAULT);
  const optInAccelerationMaxLong = orDefault(o.accelerationMaxLong, Core.REAL_DEFAULT);
  const optInAccelerationInitShort = orDefault(o.accelerationInitShort, Core.REAL_DEFAULT);
  const optInAccelerationShort = orDefault(o.accelerationShort, Core.REAL_DEFAULT);
  const optInAccelerationMaxShort = orDefault(o.accelerationMaxShort, Core.REAL_DEFAULT);
  const core = coreOf('SAREXT', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, sarextLookbackPositional(optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, core), prices.high, prices.low);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = sarextPositional(startIdx, endIdx, prices.high, prices.low, optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live SAREXT stream: one value per closed bar, bit-identical to {@link sarext}
 * over the same series. Open it with {@link sarextOpen} or
 * {@link sarextOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/sarext](https://ta-lib.org/functions/sarext).
 */
export class SarextStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInStartValue: number = 0;
  /** @internal */
  optInOffsetOnReverse: number = 0;
  /** @internal */
  optInAccelerationInitLong: number = 0;
  /** @internal */
  optInAccelerationLong: number = 0;
  /** @internal */
  optInAccelerationMaxLong: number = 0;
  /** @internal */
  optInAccelerationInitShort: number = 0;
  /** @internal */
  optInAccelerationShort: number = 0;
  /** @internal */
  optInAccelerationMaxShort: number = 0;
  /** @internal */
  isLong: number = 0;
  /** @internal */
  newHigh: number = 0;
  /** @internal */
  newLow: number = 0;
  /** @internal */
  afLong: number = 0;
  /** @internal */
  afShort: number = 0;
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
  static of(core: Core): SarextStream {
    return new SarextStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link sarext} reports them over the same bars. Each accepted `update` and each
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
      throw failure('SAREXT advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
      throw failure('SAREXT update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('SAREXT update', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    sarextStepImpl(this, inHigh, inLow);
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
      throw nonFiniteBar('SAREXT peek', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    const sp = this;
    let prevHigh = 0;
    let prevLow = 0;
    let afLong = sp.afLong;
    let afShort = sp.afShort;
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
        if (sp.optInOffsetOnReverse !== 0.0) {
          sar += sar * sp.optInOffsetOnReverse;
        }
        cur_outReal = -sar;
        /* Adjust afShort and ep */
        afShort = sp.optInAccelerationInitShort;
        ep = newLow;
        /* Calculate the new SAR */
        {
          const _a: number = afShort, _b: number = ep - sar, _c: number = sar;
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
        /* Adjust afLong and ep. */
        if (newHigh > ep) {
          ep = newHigh;
          afLong += sp.optInAccelerationLong;
          if (afLong > sp.optInAccelerationMaxLong) {
            afLong = sp.optInAccelerationMaxLong;
          }
        }
        /* Calculate the new SAR */
        {
          const _a: number = afLong, _b: number = ep - sar, _c: number = sar;
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
      if (sp.optInOffsetOnReverse !== 0.0) {
        sar -= sar * sp.optInOffsetOnReverse;
      }
      cur_outReal = sar;
      /* Adjust afLong and ep */
      afLong = sp.optInAccelerationInitLong;
      ep = newHigh;
      /* Calculate the new SAR */
      {
        const _a: number = afLong, _b: number = ep - sar, _c: number = sar;
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
      cur_outReal = -sar;
      /* Adjust afShort and ep. */
      if (newLow < ep) {
        ep = newLow;
        afShort += sp.optInAccelerationShort;
        if (afShort > sp.optInAccelerationMaxShort) {
          afShort = sp.optInAccelerationMaxShort;
        }
      }
      /* Calculate the new SAR */
      {
        const _a: number = afShort, _b: number = ep - sar, _c: number = sar;
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
  clone(): SarextStream {
    const c = new SarextStream(this.core);
    c.optInStartValue = this.optInStartValue;
    c.optInOffsetOnReverse = this.optInOffsetOnReverse;
    c.optInAccelerationInitLong = this.optInAccelerationInitLong;
    c.optInAccelerationLong = this.optInAccelerationLong;
    c.optInAccelerationMaxLong = this.optInAccelerationMaxLong;
    c.optInAccelerationInitShort = this.optInAccelerationInitShort;
    c.optInAccelerationShort = this.optInAccelerationShort;
    c.optInAccelerationMaxShort = this.optInAccelerationMaxShort;
    c.isLong = this.isLong;
    c.newHigh = this.newHigh;
    c.newLow = this.newLow;
    c.afLong = this.afLong;
    c.afShort = this.afShort;
    c.ep = this.ep;
    c.sar = this.sar;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function sarextStepImpl(sp: SarextStream, inHigh: number, inLow: number): void {
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
      if (sp.optInOffsetOnReverse !== 0.0) {
        sp.sar += sp.sar * sp.optInOffsetOnReverse;
      }
      sp.cur_outReal = -sp.sar;
      /* Adjust afShort and ep */
      sp.afShort = sp.optInAccelerationInitShort;
      sp.ep = sp.newLow;
      /* Calculate the new SAR */
      {
        const _a: number = sp.afShort, _b: number = sp.ep - sp.sar, _c: number = sp.sar;
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
      /* Adjust afLong and ep. */
      if (sp.newHigh > sp.ep) {
        sp.ep = sp.newHigh;
        sp.afLong += sp.optInAccelerationLong;
        if (sp.afLong > sp.optInAccelerationMaxLong) {
          sp.afLong = sp.optInAccelerationMaxLong;
        }
      }
      /* Calculate the new SAR */
      {
        const _a: number = sp.afLong, _b: number = sp.ep - sp.sar, _c: number = sp.sar;
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
    if (sp.optInOffsetOnReverse !== 0.0) {
      sp.sar -= sp.sar * sp.optInOffsetOnReverse;
    }
    sp.cur_outReal = sp.sar;
    /* Adjust afLong and ep */
    sp.afLong = sp.optInAccelerationInitLong;
    sp.ep = sp.newHigh;
    /* Calculate the new SAR */
    {
      const _a: number = sp.afLong, _b: number = sp.ep - sp.sar, _c: number = sp.sar;
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
    sp.cur_outReal = -sp.sar;
    /* Adjust afShort and ep. */
    if (sp.newLow < sp.ep) {
      sp.ep = sp.newLow;
      sp.afShort += sp.optInAccelerationShort;
      if (sp.afShort > sp.optInAccelerationMaxShort) {
        sp.afShort = sp.optInAccelerationMaxShort;
      }
    }
    /* Calculate the new SAR */
    {
      const _a: number = sp.afShort, _b: number = sp.ep - sp.sar, _c: number = sp.sar;
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

function sarextOpenImpl(
  sp: SarextStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInStartValue: number,
  optInOffsetOnReverse: number,
  optInAccelerationInitLong: number,
  optInAccelerationLong: number,
  optInAccelerationMaxLong: number,
  optInAccelerationInitShort: number,
  optInAccelerationShort: number,
  optInAccelerationMaxShort: number,
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
  let afLong = 0;
  let afShort = 0;
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
  if (optInStartValue === Core.REAL_DEFAULT) {
    optInStartValue = 0.0;
  } else if (!(optInStartValue >= Core.REAL_MIN && optInStartValue <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInOffsetOnReverse === Core.REAL_DEFAULT) {
    optInOffsetOnReverse = 0.0;
  } else if (!(optInOffsetOnReverse >= 0e0 && optInOffsetOnReverse <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInAccelerationInitLong === Core.REAL_DEFAULT) {
    optInAccelerationInitLong = 0.02;
  } else if (!(optInAccelerationInitLong >= 0e0 && optInAccelerationInitLong <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInAccelerationLong === Core.REAL_DEFAULT) {
    optInAccelerationLong = 0.02;
  } else if (!(optInAccelerationLong >= 0e0 && optInAccelerationLong <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInAccelerationMaxLong === Core.REAL_DEFAULT) {
    optInAccelerationMaxLong = 0.2;
  } else if (!(optInAccelerationMaxLong >= 0e0 && optInAccelerationMaxLong <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInAccelerationInitShort === Core.REAL_DEFAULT) {
    optInAccelerationInitShort = 0.02;
  } else if (!(optInAccelerationInitShort >= 0e0 && optInAccelerationInitShort <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInAccelerationShort === Core.REAL_DEFAULT) {
    optInAccelerationShort = 0.02;
  } else if (!(optInAccelerationShort >= 0e0 && optInAccelerationShort <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInAccelerationMaxShort === Core.REAL_DEFAULT) {
    optInAccelerationMaxShort = 0.2;
  } else if (!(optInAccelerationMaxShort >= 0e0 && optInAccelerationMaxShort <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* > 0 indicates long. == 0 indicates short */
  /* This function is the same as TA_SAR, except that the caller has
   * greater control on the SAR dynamic and initial state.
   *
   * In additon, the TA_SAREXT returns negative values when the position
   * is short. This allow to distinguish when the SAR do actually reverse.
   */
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
   * In TA-Lib, the following default logic is used:
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
   *
   *
   * Can I force the initial SAR?
   * ============================
   * Yes. Using the optInStartValue_0 parameter:
   *  optInStartValue_0 >  0 : SAR is long at optInStartValue_0.
   *  optInStartValue_0 <  0 : SAR is short at fabs(optInStartValue_0).
   *
   * And when optInStartValue_0 == 0, the logic is the same as for TA_SAR
   * (See previous two sections).
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
  /* Check if the acceleration factors are being defined by the user.
   * Make sure the acceleration and maximum are coherent.
   * If not, correct the acceleration.
   * Default afLong = 0.02
   * Default afShort = 0.02
   */
  afLong = optInAccelerationInitLong;
  afShort = optInAccelerationInitShort;
  if (afLong > optInAccelerationMaxLong) {
    optInAccelerationInitLong = optInAccelerationMaxLong;
    afLong = optInAccelerationInitLong;
  }
  if (optInAccelerationLong > optInAccelerationMaxLong) {
    optInAccelerationLong = optInAccelerationMaxLong;
  }
  if (afShort > optInAccelerationMaxShort) {
    optInAccelerationInitShort = optInAccelerationMaxShort;
    afShort = optInAccelerationInitShort;
  }
  if (optInAccelerationShort > optInAccelerationMaxShort) {
    optInAccelerationShort = optInAccelerationMaxShort;
  }
  /* Initialise SAR calculations */
  if (optInStartValue === 0) {
    /* Default action */
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
  } else if (optInStartValue > 0) {
    /* Start Long */
    isLong = 1;
  } else {
    /* optInStartValue_0 < 0 => Start Short */
    isLong = 0;
  }
  outBegIdx.value = startIdx;
  outIdx = 0;
  /* Write the first SAR. */
  todayIdx = startIdx;
  newHigh = inHigh[todayIdx - 1];
  newLow = inLow[todayIdx - 1];
  if (optInStartValue === 0) {
    /* Default action */
    if (isLong === 1) {
      ep = inHigh[todayIdx];
      sar = newLow;
    } else {
      ep = inLow[todayIdx];
      sar = newHigh;
    }
  } else if (optInStartValue > 0) {
    /* Start Long at specified value. */
    ep = inHigh[todayIdx];
    sar = optInStartValue;
  } else {
    /* if optInStartValue < 0 => Start Short at specified value. */
    ep = inLow[todayIdx];
    sar = Math.abs(optInStartValue);
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
        if (optInOffsetOnReverse !== 0.0) {
          sar += sar * optInOffsetOnReverse;
        }
        outReal[((outIdx++ * outStride) | 0)] = -sar;
        /* Adjust afShort and ep */
        afShort = optInAccelerationInitShort;
        ep = newLow;
        /* Calculate the new SAR */
        {
          const _a: number = afShort, _b: number = ep - sar, _c: number = sar;
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
        /* Adjust afLong and ep. */
        if (newHigh > ep) {
          ep = newHigh;
          afLong += optInAccelerationLong;
          if (afLong > optInAccelerationMaxLong) {
            afLong = optInAccelerationMaxLong;
          }
        }
        /* Calculate the new SAR */
        {
          const _a: number = afLong, _b: number = ep - sar, _c: number = sar;
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
      if (optInOffsetOnReverse !== 0.0) {
        sar -= sar * optInOffsetOnReverse;
      }
      outReal[((outIdx++ * outStride) | 0)] = sar;
      /* Adjust afLong and ep */
      afLong = optInAccelerationInitLong;
      ep = newHigh;
      /* Calculate the new SAR */
      {
        const _a: number = afLong, _b: number = ep - sar, _c: number = sar;
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
      outReal[((outIdx++ * outStride) | 0)] = -sar;
      /* Adjust afShort and ep. */
      if (newLow < ep) {
        ep = newLow;
        afShort += optInAccelerationShort;
        if (afShort > optInAccelerationMaxShort) {
          afShort = optInAccelerationMaxShort;
        }
      }
      /* Calculate the new SAR */
      {
        const _a: number = afShort, _b: number = ep - sar, _c: number = sar;
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
  sp.optInStartValue = optInStartValue;
  sp.optInOffsetOnReverse = optInOffsetOnReverse;
  sp.optInAccelerationInitLong = optInAccelerationInitLong;
  sp.optInAccelerationLong = optInAccelerationLong;
  sp.optInAccelerationMaxLong = optInAccelerationMaxLong;
  sp.optInAccelerationInitShort = optInAccelerationInitShort;
  sp.optInAccelerationShort = optInAccelerationShort;
  sp.optInAccelerationMaxShort = optInAccelerationMaxShort;
  sp.isLong = isLong;
  sp.newHigh = newHigh;
  sp.newLow = newLow;
  sp.afLong = afLong;
  sp.afShort = afShort;
  sp.ep = ep;
  sp.sar = sar;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function sarextOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInStartValue: number,
  optInOffsetOnReverse: number,
  optInAccelerationInitLong: number,
  optInAccelerationLong: number,
  optInAccelerationMaxLong: number,
  optInAccelerationInitShort: number,
  optInAccelerationShort: number,
  optInAccelerationMaxShort: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): SarextStream {
  const sp = SarextStream.of(core);
  const retCode = sarextOpenImpl(sp, inHigh, inLow, startIdx, optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SAREXT openAndFill', inHigh.length, startIdx, sarextLookbackPositional(optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, core));
  }
  if (retCode === RetCode.BAD_PARAM && sarextLookbackPositional(optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, core) < 0) {
    throw rejectedParams('SAREXT openAndFill', PARAM_RANGES, optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort);
  }
  throw streamFailure('SAREXT openAndFill', retCode);
}

/** @internal */
export function sarextOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInStartValue: number,
  optInOffsetOnReverse: number,
  optInAccelerationInitLong: number,
  optInAccelerationLong: number,
  optInAccelerationMaxLong: number,
  optInAccelerationInitShort: number,
  optInAccelerationShort: number,
  optInAccelerationMaxShort: number,
): SarextStream {
  const sp = SarextStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = sarextOpenImpl(sp, inHigh, inLow, startIdx, optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SAREXT open', inHigh.length, startIdx, sarextLookbackPositional(optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, core));
  }
  if (retCode === RetCode.BAD_PARAM && sarextLookbackPositional(optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, core) < 0) {
    throw rejectedParams('SAREXT open', PARAM_RANGES, optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort);
  }
  throw streamFailure('SAREXT open', retCode);
}

/** @internal */
export function sarextOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInStartValue: number,
  optInOffsetOnReverse: number,
  optInAccelerationInitLong: number,
  optInAccelerationLong: number,
  optInAccelerationMaxLong: number,
  optInAccelerationInitShort: number,
  optInAccelerationShort: number,
  optInAccelerationMaxShort: number,
  core: Core = Core.DEFAULT,
): SarextStream {
  optInStartValue = requireReal('SAREXT open', 'startValue', optInStartValue);
  optInOffsetOnReverse = requireReal('SAREXT open', 'offsetOnReverse', optInOffsetOnReverse);
  optInAccelerationInitLong = requireReal('SAREXT open', 'accelerationInitLong', optInAccelerationInitLong);
  optInAccelerationLong = requireReal('SAREXT open', 'accelerationLong', optInAccelerationLong);
  optInAccelerationMaxLong = requireReal('SAREXT open', 'accelerationMaxLong', optInAccelerationMaxLong);
  optInAccelerationInitShort = requireReal('SAREXT open', 'accelerationInitShort', optInAccelerationInitShort);
  optInAccelerationShort = requireReal('SAREXT open', 'accelerationShort', optInAccelerationShort);
  optInAccelerationMaxShort = requireReal('SAREXT open', 'accelerationMaxShort', optInAccelerationMaxShort);
  requireArgument('SAREXT open', 'prices.high', inHigh);
  requireHistory('SAREXT open', inHigh.length);
  requireArgument('SAREXT open', 'prices.low', inLow);
  requireHistoryLength('SAREXT open', 'prices.low', inLow.length, inHigh.length);
  return sarextOpenInternal(core, float64(inHigh), float64(inLow), 0, optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort);
}

/** @internal */
export function sarextOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInStartValue: number,
  optInOffsetOnReverse: number,
  optInAccelerationInitLong: number,
  optInAccelerationLong: number,
  optInAccelerationMaxLong: number,
  optInAccelerationInitShort: number,
  optInAccelerationShort: number,
  optInAccelerationMaxShort: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): SarextStream {
  optInStartValue = requireReal('SAREXT openAndFill', 'startValue', optInStartValue);
  optInOffsetOnReverse = requireReal('SAREXT openAndFill', 'offsetOnReverse', optInOffsetOnReverse);
  optInAccelerationInitLong = requireReal('SAREXT openAndFill', 'accelerationInitLong', optInAccelerationInitLong);
  optInAccelerationLong = requireReal('SAREXT openAndFill', 'accelerationLong', optInAccelerationLong);
  optInAccelerationMaxLong = requireReal('SAREXT openAndFill', 'accelerationMaxLong', optInAccelerationMaxLong);
  optInAccelerationInitShort = requireReal('SAREXT openAndFill', 'accelerationInitShort', optInAccelerationInitShort);
  optInAccelerationShort = requireReal('SAREXT openAndFill', 'accelerationShort', optInAccelerationShort);
  optInAccelerationMaxShort = requireReal('SAREXT openAndFill', 'accelerationMaxShort', optInAccelerationMaxShort);
  requireArgument('SAREXT openAndFill', 'prices.high', inHigh);
  requireHistory('SAREXT openAndFill', inHigh.length);
  requireArgument('SAREXT openAndFill', 'prices.low', inLow);
  const guardLookback = sarextLookbackPositional(optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, core);
  if (guardLookback < 0) {
    throw rejectedParams('SAREXT openAndFill', PARAM_RANGES, optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort);
  }
  const guardOutLen = openFillCount('SAREXT openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('SAREXT openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireOutput('SAREXT openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('SAREXT openAndFill', 'outReal', 'prices.high');
  }
  if (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('SAREXT openAndFill', 'outReal', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outReal, inHighF64)) {
    throw sharedMemory('SAREXT openAndFill', 'outReal', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outReal, inLowF64)) {
    throw sharedMemory('SAREXT openAndFill', 'outReal', 'prices.low');
  }
  return sarextOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live SAREXT stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link sarext} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SarextOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `sarextLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, a real parameter that is not a number, a parameter outside its range or
 *   histories of different lengths.
 *
 * @example
 * ```ts
 * import { sarextOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = sarextOpen({ high, low }, {
 *   startValue: 0,
 *   offsetOnReverse: 0,
 *   accelerationInitLong: 0.02,
 *   accelerationLong: 0.02,
 *   accelerationMaxLong: 0.2,
 *   accelerationInitShort: 0.02,
 *   accelerationShort: 0.02,
 *   accelerationMaxShort: 0.2,
 * });
 * const peeked = stream.peek(101.4, 99.1); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1); // the bar closes
 * ```
 */
export function sarextOpen(prices: { high: ArrayLike<number>; low: ArrayLike<number> }, options?: SarextOptions): SarextStream {
  requireArgument('SAREXT open', 'prices', prices);
  const o = optionsOf('SAREXT open', options);
  const optInStartValue = orDefault(o.startValue, Core.REAL_DEFAULT);
  const optInOffsetOnReverse = orDefault(o.offsetOnReverse, Core.REAL_DEFAULT);
  const optInAccelerationInitLong = orDefault(o.accelerationInitLong, Core.REAL_DEFAULT);
  const optInAccelerationLong = orDefault(o.accelerationLong, Core.REAL_DEFAULT);
  const optInAccelerationMaxLong = orDefault(o.accelerationMaxLong, Core.REAL_DEFAULT);
  const optInAccelerationInitShort = orDefault(o.accelerationInitShort, Core.REAL_DEFAULT);
  const optInAccelerationShort = orDefault(o.accelerationShort, Core.REAL_DEFAULT);
  const optInAccelerationMaxShort = orDefault(o.accelerationMaxShort, Core.REAL_DEFAULT);
  const core = coreOf('SAREXT open', o.core);
  return sarextOpenPositional(prices.high, prices.low, optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, core);
}

/**
 * {@link sarextOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link sarext}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link SarextOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link sarext} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `sarextLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, a real parameter that is not a number, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { sarextOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = sarextOpenAndFill({ high, low }, {
 *   startValue: 0,
 *   offsetOnReverse: 0,
 *   accelerationInitLong: 0.02,
 *   accelerationLong: 0.02,
 *   accelerationMaxLong: 0.2,
 *   accelerationInitShort: 0.02,
 *   accelerationShort: 0.02,
 *   accelerationMaxShort: 0.2,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function sarextOpenAndFill(prices: { high: ArrayLike<number>; low: ArrayLike<number> }, options?: SarextOptions): SarextResult & { stream: SarextStream } {
  requireArgument('SAREXT openAndFill', 'prices', prices);
  const o = optionsOf('SAREXT openAndFill', options);
  const optInStartValue = orDefault(o.startValue, Core.REAL_DEFAULT);
  const optInOffsetOnReverse = orDefault(o.offsetOnReverse, Core.REAL_DEFAULT);
  const optInAccelerationInitLong = orDefault(o.accelerationInitLong, Core.REAL_DEFAULT);
  const optInAccelerationLong = orDefault(o.accelerationLong, Core.REAL_DEFAULT);
  const optInAccelerationMaxLong = orDefault(o.accelerationMaxLong, Core.REAL_DEFAULT);
  const optInAccelerationInitShort = orDefault(o.accelerationInitShort, Core.REAL_DEFAULT);
  const optInAccelerationShort = orDefault(o.accelerationShort, Core.REAL_DEFAULT);
  const optInAccelerationMaxShort = orDefault(o.accelerationMaxShort, Core.REAL_DEFAULT);
  const core = coreOf('SAREXT openAndFill', o.core);
  const capacity = fillCapacity(sarextLookbackPositional(optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, core), prices.high, prices.low);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = sarextOpenAndFillPositional(prices.high, prices.low, optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
