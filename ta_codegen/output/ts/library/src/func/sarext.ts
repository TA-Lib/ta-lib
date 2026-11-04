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

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireReal, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { minusDm } from './minusDm.ts';

/**
 * Number of leading input bars {@link sarext} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInStartValue - Initial SAR/direction: 0 auto, >0 start long at value,
 *   \<0 start short at -value (default 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInOffsetOnReverse - Fractional offset applied to the stop on each
 *   reversal (default 0; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInAccelerationInitLong - Initial acceleration factor when long
 *   (default 0.02; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInAccelerationLong - AF increment per new long extreme (default 0.02;
 *   minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInAccelerationMaxLong - Cap on the long acceleration factor (default
 *   0.2; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInAccelerationInitShort - Initial acceleration factor when short
 *   (default 0.02; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInAccelerationShort - AF increment per new short extreme (default
 *   0.02; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInAccelerationMaxShort - Cap on the short acceleration factor (default
 *   0.2; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function sarextLookback(optInStartValue: number, optInOffsetOnReverse: number, optInAccelerationInitLong: number, optInAccelerationLong: number, optInAccelerationMaxLong: number, optInAccelerationInitShort: number, optInAccelerationShort: number, optInAccelerationMaxShort: number, core: Core = Core.DEFAULT): number {
  void core;
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

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link sarext}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInStartValue - Initial SAR/direction: 0 auto, >0 start long at value,
 *   \<0 start short at -value (default 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInOffsetOnReverse - Fractional offset applied to the stop on each
 *   reversal (default 0; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInAccelerationInitLong - Initial acceleration factor when long
 *   (default 0.02; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInAccelerationLong - AF increment per new long extreme (default 0.02;
 *   minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInAccelerationMaxLong - Cap on the long acceleration factor (default
 *   0.2; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInAccelerationInitShort - Initial acceleration factor when short
 *   (default 0.02; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInAccelerationShort - AF increment per new short extreme (default
 *   0.02; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInAccelerationMaxShort - Cap on the short acceleration factor (default
 *   0.2; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function sarextDisplayShift(optInStartValue: number, optInOffsetOnReverse: number, optInAccelerationInitLong: number, optInAccelerationLong: number, optInAccelerationMaxLong: number, optInAccelerationInitShort: number, optInAccelerationShort: number, optInAccelerationMaxShort: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (sarextLookback(optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, core) < 0) {
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
    const _xr0 = minusDm(startIdx, startIdx, inHigh, inLow, 1, ep_temp, core);
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
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link sarextLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param optInStartValue - Initial SAR/direction: 0 auto, >0 start long at value,
 *   \<0 start short at -value (default 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInOffsetOnReverse - Fractional offset applied to the stop on each
 *   reversal (default 0; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInAccelerationInitLong - Initial acceleration factor when long
 *   (default 0.02; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInAccelerationLong - AF increment per new long extreme (default 0.02;
 *   minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInAccelerationMaxLong - Cap on the long acceleration factor (default
 *   0.2; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInAccelerationInitShort - Initial acceleration factor when short
 *   (default 0.02; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInAccelerationShort - AF increment per new short extreme (default
 *   0.02; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInAccelerationMaxShort - Cap on the short acceleration factor (default
 *   0.2; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param outReal - SAR stop level; positive while long, negative while short. Must
 *   hold the values produced and never be empty; the input length always suffices.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The range written: `begIdx` is the first bar with a value, `count` how
 *   many were written.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for an argument that is not an int32, a
 *   real parameter that is not a number, a parameter outside its range, an array too
 *   short for the range, an empty output or an output sharing memory with a
 *   `Float64Array` input it is not. Checked before anything is written.
 *
 * @example
 * ```ts
 * import { sarext } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(high.length);
 * const { begIdx, count } = sarext(
 *   0, high.length - 1,
 *   high, low,
 *   0, 0, 0.02, 0.02, 0.2, 0.02, 0.02, 0.2,
 *   out,
 * );
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`sar`](https://ta-lib.org/functions/sar)
 * @see [`minusDm`](https://ta-lib.org/functions/minus_dm)
 */
export function sarext(
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
  optInStartValue = requireReal('SAREXT', 'optInStartValue', optInStartValue);
  optInOffsetOnReverse = requireReal('SAREXT', 'optInOffsetOnReverse', optInOffsetOnReverse);
  optInAccelerationInitLong = requireReal('SAREXT', 'optInAccelerationInitLong', optInAccelerationInitLong);
  optInAccelerationLong = requireReal('SAREXT', 'optInAccelerationLong', optInAccelerationLong);
  optInAccelerationMaxLong = requireReal('SAREXT', 'optInAccelerationMaxLong', optInAccelerationMaxLong);
  optInAccelerationInitShort = requireReal('SAREXT', 'optInAccelerationInitShort', optInAccelerationInitShort);
  optInAccelerationShort = requireReal('SAREXT', 'optInAccelerationShort', optInAccelerationShort);
  optInAccelerationMaxShort = requireReal('SAREXT', 'optInAccelerationMaxShort', optInAccelerationMaxShort);
  requireIndexRange('SAREXT', startIdx, endIdx);
  const guardStart = clampedStart('SAREXT', startIdx, sarextLookback(optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('SAREXT', 'inHigh', inHigh, guardInLen);
  requireLength('SAREXT', 'inLow', inLow, guardInLen);
  requireOutput('SAREXT', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlapsPartly(outReal, inHighF64) || overlapsPartly(outReal, inLowF64)) {
    throw failure('SAREXT', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = sarextImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('SAREXT', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
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
    const _xr8 = minusDm(startIdx, startIdx, inHigh, inLow, 1, ep_temp, core);
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
    throw insufficientHistory('SAREXT openAndFill', inHigh.length, startIdx, sarextLookback(optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, core));
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
    throw insufficientHistory('SAREXT open', inHigh.length, startIdx, sarextLookback(optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, core));
  }
  throw streamFailure('SAREXT open', retCode);
}

/**
 * Opens a live SAREXT stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link sarext} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param optInStartValue - Initial SAR/direction: 0 auto, >0 start long at value,
 *   \<0 start short at -value (default 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInOffsetOnReverse - Fractional offset applied to the stop on each
 *   reversal (default 0; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInAccelerationInitLong - Initial acceleration factor when long
 *   (default 0.02; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInAccelerationLong - AF increment per new long extreme (default 0.02;
 *   minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInAccelerationMaxLong - Cap on the long acceleration factor (default
 *   0.2; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInAccelerationInitShort - Initial acceleration factor when short
 *   (default 0.02; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInAccelerationShort - AF increment per new short extreme (default
 *   0.02; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInAccelerationMaxShort - Cap on the short acceleration factor (default
 *   0.2; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `sarextLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for a real parameter that is not a number, a parameter outside its range or
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
 * const stream = sarextOpen(high, low, 0, 0, 0.02, 0.02, 0.2, 0.02, 0.02, 0.2);
 * const peeked = stream.peek(101.4, 99.1); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1); // the bar closes
 * ```
 */
export function sarextOpen(
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
  optInStartValue = requireReal('SAREXT open', 'optInStartValue', optInStartValue);
  optInOffsetOnReverse = requireReal('SAREXT open', 'optInOffsetOnReverse', optInOffsetOnReverse);
  optInAccelerationInitLong = requireReal('SAREXT open', 'optInAccelerationInitLong', optInAccelerationInitLong);
  optInAccelerationLong = requireReal('SAREXT open', 'optInAccelerationLong', optInAccelerationLong);
  optInAccelerationMaxLong = requireReal('SAREXT open', 'optInAccelerationMaxLong', optInAccelerationMaxLong);
  optInAccelerationInitShort = requireReal('SAREXT open', 'optInAccelerationInitShort', optInAccelerationInitShort);
  optInAccelerationShort = requireReal('SAREXT open', 'optInAccelerationShort', optInAccelerationShort);
  optInAccelerationMaxShort = requireReal('SAREXT open', 'optInAccelerationMaxShort', optInAccelerationMaxShort);
  requireArgument('SAREXT open', 'inHigh', inHigh);
  requireHistory('SAREXT open', inHigh.length);
  requireArgument('SAREXT open', 'inLow', inLow);
  requireHistoryLength('SAREXT open', 'inLow', inLow.length, inHigh.length);
  return sarextOpenInternal(core, float64(inHigh), float64(inLow), 0, optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort);
}

/**
 * {@link sarextOpen} that also fills the outputs over the whole history in the
 * same pass, bit-identically to {@link sarext}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param optInStartValue - Initial SAR/direction: 0 auto, >0 start long at value,
 *   \<0 start short at -value (default 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInOffsetOnReverse - Fractional offset applied to the stop on each
 *   reversal (default 0; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInAccelerationInitLong - Initial acceleration factor when long
 *   (default 0.02; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInAccelerationLong - AF increment per new long extreme (default 0.02;
 *   minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInAccelerationMaxLong - Cap on the long acceleration factor (default
 *   0.2; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInAccelerationInitShort - Initial acceleration factor when short
 *   (default 0.02; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInAccelerationShort - AF increment per new short extreme (default
 *   0.02; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInAccelerationMaxShort - Cap on the short acceleration factor (default
 *   0.2; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param outReal - SAR stop level; positive while long, negative while short. Must
 *   hold `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `sarextLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for a real parameter that is not a number, a parameter outside its range,
 *   histories of different lengths or an output too short or starting where another
 *   output or a `Float64Array` input starts, or, on a history long enough to open,
 *   one sharing memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { sarextOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(high.length);
 * const stream = sarextOpenAndFill(
 *   high, low,
 *   0, 0, 0.02, 0.02, 0.2, 0.02, 0.02, 0.2,
 *   out,
 * );
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function sarextOpenAndFill(
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
  optInStartValue = requireReal('SAREXT openAndFill', 'optInStartValue', optInStartValue);
  optInOffsetOnReverse = requireReal('SAREXT openAndFill', 'optInOffsetOnReverse', optInOffsetOnReverse);
  optInAccelerationInitLong = requireReal('SAREXT openAndFill', 'optInAccelerationInitLong', optInAccelerationInitLong);
  optInAccelerationLong = requireReal('SAREXT openAndFill', 'optInAccelerationLong', optInAccelerationLong);
  optInAccelerationMaxLong = requireReal('SAREXT openAndFill', 'optInAccelerationMaxLong', optInAccelerationMaxLong);
  optInAccelerationInitShort = requireReal('SAREXT openAndFill', 'optInAccelerationInitShort', optInAccelerationInitShort);
  optInAccelerationShort = requireReal('SAREXT openAndFill', 'optInAccelerationShort', optInAccelerationShort);
  optInAccelerationMaxShort = requireReal('SAREXT openAndFill', 'optInAccelerationMaxShort', optInAccelerationMaxShort);
  requireArgument('SAREXT openAndFill', 'inHigh', inHigh);
  requireHistory('SAREXT openAndFill', inHigh.length);
  requireArgument('SAREXT openAndFill', 'inLow', inLow);
  const guardOutLen = openFillCount('SAREXT openAndFill', inHigh.length, sarextLookback(optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, core));
  requireHistoryLength('SAREXT openAndFill', 'inLow', inLow.length, inHigh.length);
  requireOutput('SAREXT openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if ((outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) || (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset)) {
    throw streamFailure('SAREXT openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inHighF64) || overlaps(outReal, inLowF64))) {
    throw streamFailure('SAREXT openAndFill', RetCode.BAD_PARAM);
  }
  return sarextOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, new MInteger(), new MInteger(), outReal);
}
