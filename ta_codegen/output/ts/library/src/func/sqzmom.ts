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
 *  100826 KL,CC  Creation (#488).
 *  101126 MF,CC  Reject factors out of order (#488).
 *  101126 MF,CC  DEV in one loop instead of three calls (#488).
 *  101126 MF,CC  Both bands from one rounded deviation, as bbands (#488).
 *  101126 MF,CC  One Keltner centre for both loops (#543).
 */

import { badSink, clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireReal, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { LinearregStream, linearreg, linearregLookback, linearregOpenAndFillInternal } from './linearreg.ts';
import { MidpriceStream, midprice, midpriceLookback, midpriceOpenAndFillInternal } from './midprice.ts';
import { SmaStream, sma, smaLookback, smaOpenAndFillInternal, smaOpenInternal } from './sma.ts';
import { StddevStream, stddev, stddevLookback, stddevOpenAndFillInternal } from './stddev.ts';
import { TrangeStream, trange, trangeLookback, trangeOpenAndFillInternal } from './trange.ts';

/**
 * Number of leading input bars {@link sqzmom} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInBBPeriod - Number of bars in the Bollinger Band window (default 20;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInNbDev - Number of standard deviations the Bollinger Bands are placed
 *   at (default 2; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInKCPeriod - Number of bars in the Keltner Channel, the true-range
 *   mean and the regression (default 20; range 2 to 100000; `Core.INTEGER_DEFAULT`
 *   selects the default).
 * @param optInFactorWide - Keltner width, in true-range means, of the widest
 *   channel (default 2; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInFactorNormal - Keltner width of the classic channel (default 1.5;
 *   minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInFactorNarrow - Keltner width of the tightest channel (default 1;
 *   minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function sqzmomLookback(optInBBPeriod: number, optInNbDev: number, optInKCPeriod: number, optInFactorWide: number, optInFactorNormal: number, optInFactorNarrow: number, core: Core = Core.DEFAULT): number {
  if ((optInBBPeriod | 0) !== optInBBPeriod) {
    return -1;
  }
  if (typeof optInNbDev !== 'number') {
    return -1;
  }
  if ((optInKCPeriod | 0) !== optInKCPeriod) {
    return -1;
  }
  if (typeof optInFactorWide !== 'number') {
    return -1;
  }
  if (typeof optInFactorNormal !== 'number') {
    return -1;
  }
  if (typeof optInFactorNarrow !== 'number') {
    return -1;
  }
  if (optInBBPeriod === Core.INTEGER_DEFAULT) {
    optInBBPeriod = 20;
  } else if (optInBBPeriod < 2 || optInBBPeriod > 100000) {
    return -1;
  }
  if (optInNbDev === Core.REAL_DEFAULT) {
    optInNbDev = 2.0;
  } else if (!(optInNbDev >= 0e0 && optInNbDev <= Core.REAL_MAX)) {
    return -1;
  }
  if (optInKCPeriod === Core.INTEGER_DEFAULT) {
    optInKCPeriod = 20;
  } else if (optInKCPeriod < 2 || optInKCPeriod > 100000) {
    return -1;
  }
  if (optInFactorWide === Core.REAL_DEFAULT) {
    optInFactorWide = 2.0;
  } else if (!(optInFactorWide >= 0e0 && optInFactorWide <= Core.REAL_MAX)) {
    return -1;
  }
  if (optInFactorNormal === Core.REAL_DEFAULT) {
    optInFactorNormal = 1.5;
  } else if (!(optInFactorNormal >= 0e0 && optInFactorNormal <= Core.REAL_MAX)) {
    return -1;
  }
  if (optInFactorNarrow === Core.REAL_DEFAULT) {
    optInFactorNarrow = 1.0;
  } else if (!(optInFactorNarrow >= 0e0 && optInFactorNarrow <= Core.REAL_MAX)) {
    return -1;
  }
  let lookbackTotal = 0;
  let leg = 0;
  /* The range check cannot order the three factors; without this the lookback
   * answers a usable number for a call that cannot run.
   */
  if (optInFactorNormal > optInFactorWide || optInFactorNarrow > optInFactorNormal) {
    return -1;
  }
  /* Three chains reach the first bar at which both outputs exist, and the
   * first output is the longest of them:
   *
   *   bands        sma(b) / stddev(b)           = b-1
   *   compression  trange -> sma(k)             = 1 + (k-1) = k
   *   momentum     midprice(k) -> linearreg(k)  = (k-1) + (k-1) = 2(k-1)
   *
   * The Keltner centre, sma(k) on the close, is k-1 and never dominates the
   * momentum chain it also feeds. At b = k = 20 the three read 19, 20 and 38,
   * which is where pandas, ta4j and trading-signals put their first value.
   * The factors scale a band that already exists, so they do not enter.
   */
  lookbackTotal = smaLookback(optInBBPeriod, core);
  leg = stddevLookback(optInBBPeriod, 1.0, core);
  if (leg > lookbackTotal) {
    lookbackTotal = leg;
  }
  leg = trangeLookback(core) + smaLookback(optInKCPeriod, core);
  if (leg > lookbackTotal) {
    lookbackTotal = leg;
  }
  leg = midpriceLookback(optInKCPeriod, core) + linearregLookback(optInKCPeriod, core);
  if (leg > lookbackTotal) {
    lookbackTotal = leg;
  }
  return lookbackTotal;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link sqzmom}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInBBPeriod - Number of bars in the Bollinger Band window (default 20;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInNbDev - Number of standard deviations the Bollinger Bands are placed
 *   at (default 2; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInKCPeriod - Number of bars in the Keltner Channel, the true-range
 *   mean and the regression (default 20; range 2 to 100000; `Core.INTEGER_DEFAULT`
 *   selects the default).
 * @param optInFactorWide - Keltner width, in true-range means, of the widest
 *   channel (default 2; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInFactorNormal - Keltner width of the classic channel (default 1.5;
 *   minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInFactorNarrow - Keltner width of the tightest channel (default 1;
 *   minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function sqzmomDisplayShift(optInBBPeriod: number, optInNbDev: number, optInKCPeriod: number, optInFactorWide: number, optInFactorNormal: number, optInFactorNarrow: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (sqzmomLookback(optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 2) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function sqzmomImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  optInBBPeriod: number,
  optInNbDev: number,
  optInKCPeriod: number,
  optInFactorWide: number,
  optInFactorNormal: number,
  optInFactorNarrow: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outMomentum: Float64Array,
  outSqueeze: Int32Array,
): RetCode {
  let lookbackTotal = 0;
  let stageLookback = 0;
  let stageIdx = 0;
  let stageLen = 0;
  let i = 0;
  let n = 0;
  const tempBegIdx = new MInteger();
  const tempNbElement = new MInteger();
  const kcmNb = new MInteger();
  const sdNb = new MInteger();
  let kcmShift = 0;
  let tempDev!: Float64Array;
  let tempWork!: Float64Array;
  let tempKCM!: Float64Array;
  let tempMid!: Float64Array;
  let tempSD!: Float64Array;
  let dev = 0;
  let up = 0;
  let lo = 0;
  let centre = 0;
  let band = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInBBPeriod === Core.INTEGER_DEFAULT) {
    optInBBPeriod = 20;
  } else if (optInBBPeriod < 2 || optInBBPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInNbDev === Core.REAL_DEFAULT) {
    optInNbDev = 2.0;
  } else if (!(optInNbDev >= 0e0 && optInNbDev <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInKCPeriod === Core.INTEGER_DEFAULT) {
    optInKCPeriod = 20;
  } else if (optInKCPeriod < 2 || optInKCPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInFactorWide === Core.REAL_DEFAULT) {
    optInFactorWide = 2.0;
  } else if (!(optInFactorWide >= 0e0 && optInFactorWide <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInFactorNormal === Core.REAL_DEFAULT) {
    optInFactorNormal = 1.5;
  } else if (!(optInFactorNormal >= 0e0 && optInFactorNormal <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInFactorNarrow === Core.REAL_DEFAULT) {
    optInFactorNarrow = 1.0;
  } else if (!(optInFactorNarrow >= 0e0 && optInFactorNarrow <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if ((outMomentum.buffer === outSqueeze.buffer && outMomentum.byteOffset === outSqueeze.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  /* Each rolling statistic is a call to the shipped function, so it carries
   * that function's own arithmetic bit for bit:
   *
   *   BBU, BBL = sma(close, b) +/- nbdev * stddev(close, b, 1.0)
   *   KCM      = sma(close, k)
   *   BAND     = sma(trange(high,low,close), k)      the simple mean of TR
   *   DMID     = midprice(high, low, k)
   *   DEV      = close - (DMID + KCM)/2
   *   momentum = linearreg(DEV, k)
   *
   * A composed sub-call cannot be handed an MA-type constant, so the bands
   * are the sma + stddev pair and not a bbands call.
   *
   * Two chains need their input one window earlier than the first output bar:
   * sma(k) over TR, and linearreg(k) over DEV. Both windows are k-1 long, so
   * one staged range serves both.
   */
  /* A channel wider than the one it should nest in breaks the ladder: a bar
   * could then be inside "narrow" and outside "normal". Ties are allowed, and
   * wide == normal is the classic on/off/none squeeze.
   */
  if (optInFactorNormal > optInFactorWide || optInFactorNarrow > optInFactorNormal) {
    return RetCode.BAD_PARAM;
  }
  lookbackTotal = sqzmomLookback(optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, core);
  if (lookbackTotal > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  n = endIdx - startIdx + 1;
  stageLookback = linearregLookback(optInKCPeriod, core);
  stageIdx = startIdx - stageLookback;
  stageLen = n + stageLookback;
  tempDev = new Float64Array(stageLen);
  tempWork = new Float64Array(stageLen);
  tempKCM = new Float64Array(stageLen);
  tempMid = new Float64Array(n);
  tempSD = new Float64Array(n);
  /* The momentum anchor: the Donchian midpoint and the Keltner centre, averaged.
   * midprice is bit-identical to donchian's middle band.
   */
  const _xr0 = midprice(stageIdx, endIdx, inHigh, inLow, optInKCPeriod, tempDev, core);
  outBegIdx.value = _xr0.begIdx;
  outNBElement.value = _xr0.count;
  const _xr1 = sma(stageIdx, endIdx, inClose, optInKCPeriod, tempKCM, core);
  tempBegIdx.value = _xr1.begIdx;
  kcmNb.value = _xr1.count;
  /* DEV, over the staged range. The close is read at the index midprice
   * reported for the range, which is what lets this loop stream: keep
   * outBegIdx as midprice's receiver, and reuse it for no call above here.
   */
  for (i = 0; i < stageLen; i += 1) {
    tempDev[i] = inClose[outBegIdx.value + i] - (tempDev[i] + tempKCM[i]) / 2.0;
  }
  /* The compression band: the simple mean of the true range, not an ATR.
   * trange's own lookback is 1, and stageIdx is at least 1 because the
   * compression chain alone puts lookbackTotal at k or more.
   */
  const _xr2 = trange(stageIdx, endIdx, inHigh, inLow, inClose, tempWork, core);
  tempBegIdx.value = _xr2.begIdx;
  tempNbElement.value = _xr2.count;
  const _xr3 = sma(0, stageLen - 1, tempWork, optInKCPeriod, tempWork, core);
  tempBegIdx.value = _xr3.begIdx;
  tempNbElement.value = _xr3.count;
  const _xr4 = sma(startIdx, endIdx, inClose, optInBBPeriod, tempMid, core);
  tempBegIdx.value = _xr4.begIdx;
  tempNbElement.value = _xr4.count;
  const _xr5 = stddev(startIdx, endIdx, inClose, optInBBPeriod, 1.0, tempSD, core);
  tempBegIdx.value = _xr5.begIdx;
  sdNb.value = _xr5.count;
  /* The centre is staged one regression window early; the shift puts it on
   * the bar the bands are on.
   */
  kcmShift = kcmNb.value - sdNb.value;
  /* LAST of the calls, because it is the first to write a caller buffer. Every
   * read of high, low and close is above it, so an output aliased onto one of
   * them -- which the abstract layer tests on purpose -- is still intact when it
   * is read.
   *
   * The regression window closes on the first output bar, so entering it at 0 of
   * the staged range puts its first fitted value on startIdx.
   */
  const _xr6 = linearreg(0, stageLen - 1, tempDev, optInKCPeriod, outMomentum, core);
  tempBegIdx.value = _xr6.begIdx;
  tempNbElement.value = _xr6.count;
  /* The ordinal. Every comparison is strict: a band exactly on a channel
   * edge is not inside that channel and not outside it either.
   *
   * Keep the deviation a rounded product added to and subtracted from the
   * middle band, as bbands does. Written as one expression per band, the
   * upper one contracts into a fused multiply-add and the lower one cannot,
   * and a band within one ulp of an edge then changes level.
   */
  for (i = 0; i < n; i += 1) {
    dev = tempSD[i] * optInNbDev;
    up = tempMid[i] + dev;
    lo = tempMid[i] - dev;
    centre = tempKCM[i + kcmShift];
    band = tempWork[i];
    if (lo > centre - optInFactorNarrow * band && up < centre + optInFactorNarrow * band) {
      outSqueeze[i] = 3;
    } else if (lo > centre - optInFactorNormal * band && up < centre + optInFactorNormal * band) {
      outSqueeze[i] = 2;
    } else if (lo > centre - optInFactorWide * band && up < centre + optInFactorWide * band) {
      outSqueeze[i] = 1;
    } else if (lo < centre - optInFactorWide * band && up > centre + optInFactorWide * band) {
      outSqueeze[i] = -1;
    } else {
      outSqueeze[i] = 0;
    }
  }
  outBegIdx.value = startIdx;
  outNBElement.value = n;
  return RetCode.SUCCESS;
}

/**
 * John Carter's TTM Squeeze, in the form LazyBear published on TradingView in 2014.
 * Volatility compression is read by comparing the Bollinger Bands against a
 * Keltner Channel: when the bands sit inside the channel the market is coiled, and
 * when they push outside it the coil has released. LazyBear replaced Carter's
 * simple momentum with a linear regression of the close against a Donchian/SMA
 * anchor, which is the histogram that StockCharts, ta4j and trading-signals
 * compute, and pandas-ta with `lazybear=True`. "Squeeze Pro" tests the bands
 * against three Keltner widths instead of one, so compression becomes a level
 * rather than a boolean. `outSqueeze` carries that level: 3, 2 and 1 for the
 * narrow, normal and wide channels, -1 once the bands are outside the wide
 * channel, and 0 in between. The classic squeeze-on flag is `outSqueeze >= 2`, and
 * with `wide == normal` the three states are exactly LazyBear's on / off / none.
 *
 * Formula and more info at
 * [ta-lib.org/functions/sqzmom](https://ta-lib.org/functions/sqzmom).
 *
 * **Notes**
 * - `BAND` is the simple mean of the true range over `k` bars, not an ATR: no
 *   Wilder smoothing, and so no unstable period anywhere in this function.
 * - The anchor is written three ways across the sources — `avg(avg(HH,LL), SMA)`,
 *   `((HH+LL)/2 + SMA)/2` and `0.25*(HH+LL) + 0.5*SMA`. All three give the same
 *   double, because scaling by a power of two is exact.
 * - The three channels share a centre and `BAND >= 0`, so they nest: inside the
 *   narrow one implies inside the normal one implies inside the wide one. The
 *   ordinal is therefore monotone, and the factors are required only to be ordered
 *   `wide >= normal >= narrow >= 0`, not strictly. Factors out of that order are
 *   rejected with `TA_BAD_PARAM`.
 * - The momentum window is the Keltner period, as in LazyBear's script, pandas and
 *   trading-signals.
 * - The three chains that feed the outputs do not reach their first value
 *   together. The first output bar is the latest of them, so where `b` is much
 *   larger than `k` the momentum exists before the state does and is still
 *   withheld until both do. pandas emits the earlier momentum; a capture compared
 *   against it must start at the later bar.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link sqzmomLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Closing price of each bar.
 * @param optInBBPeriod - Number of bars in the Bollinger Band window (default 20;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInNbDev - Number of standard deviations the Bollinger Bands are placed
 *   at (default 2; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInKCPeriod - Number of bars in the Keltner Channel, the true-range
 *   mean and the regression (default 20; range 2 to 100000; `Core.INTEGER_DEFAULT`
 *   selects the default).
 * @param optInFactorWide - Keltner width, in true-range means, of the widest
 *   channel (default 2; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInFactorNormal - Keltner width of the classic channel (default 1.5;
 *   minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInFactorNarrow - Keltner width of the tightest channel (default 1;
 *   minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param outMomentum - Linear regression of the close against the Donchian/SMA
 *   anchor. Must hold the values produced and never be empty; the input length
 *   always suffices.
 * @param outSqueeze - Compression level: 3 narrow, 2 normal, 1 wide, -1 released,
 *   0 otherwise. Must hold the values produced and never be empty; the input length
 *   always suffices.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The range written: `begIdx` is the first bar with a value, `count` how
 *   many were written.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for an argument that is not an int32, a
 *   real parameter that is not a number, a parameter outside its range, an array too
 *   short for the range, an empty output, two outputs sharing memory or an output
 *   sharing memory with a `Float64Array` input it is not. Checked before anything is
 *   written.
 *
 * @example
 * ```ts
 * import { sqzmom } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const momentum = new Float64Array(high.length);
 * const squeeze = new Int32Array(high.length);
 * const { begIdx, count } = sqzmom(
 *   0, high.length - 1,
 *   high, low, close,
 *   20, 2, 20, 2, 1.5, 1,
 *   momentum, squeeze,
 * );
 * // momentum[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`bbands`](https://ta-lib.org/functions/bbands)
 * @see [`kc`](https://ta-lib.org/functions/kc)
 * @see [`trange`](https://ta-lib.org/functions/trange)
 * @see [`midprice`](https://ta-lib.org/functions/midprice)
 * @see [`linearreg`](https://ta-lib.org/functions/linearreg)
 */
export function sqzmom(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInBBPeriod: number,
  optInNbDev: number,
  optInKCPeriod: number,
  optInFactorWide: number,
  optInFactorNormal: number,
  optInFactorNarrow: number,
  outMomentum: Float64Array,
  outSqueeze: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('SQZMOM', 'startIdx', startIdx);
  endIdx = requireInt32('SQZMOM', 'endIdx', endIdx);
  optInBBPeriod = requireInt32('SQZMOM', 'optInBBPeriod', optInBBPeriod);
  optInNbDev = requireReal('SQZMOM', 'optInNbDev', optInNbDev);
  optInKCPeriod = requireInt32('SQZMOM', 'optInKCPeriod', optInKCPeriod);
  optInFactorWide = requireReal('SQZMOM', 'optInFactorWide', optInFactorWide);
  optInFactorNormal = requireReal('SQZMOM', 'optInFactorNormal', optInFactorNormal);
  optInFactorNarrow = requireReal('SQZMOM', 'optInFactorNarrow', optInFactorNarrow);
  requireIndexRange('SQZMOM', startIdx, endIdx);
  const guardStart = clampedStart('SQZMOM', startIdx, sqzmomLookback(optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('SQZMOM', 'inHigh', inHigh, guardInLen);
  requireLength('SQZMOM', 'inLow', inLow, guardInLen);
  requireLength('SQZMOM', 'inClose', inClose, guardInLen);
  requireLength('SQZMOM', 'outMomentum', outMomentum, guardOutLen);
  requireLength('SQZMOM', 'outSqueeze', outSqueeze, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outMomentum, inHighF64) || overlapsPartly(outMomentum, inLowF64) || overlapsPartly(outMomentum, inCloseF64) || overlaps(outSqueeze, inHighF64) || overlaps(outSqueeze, inLowF64) || overlaps(outSqueeze, inCloseF64) || overlaps(outMomentum, outSqueeze)) {
    throw failure('SQZMOM', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = sqzmomImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, outBegIdx, outNBElement, outMomentum, outSqueeze);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('SQZMOM', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live SQZMOM stream: one value per closed bar, bit-identical to {@link sqzmom}
 * over the same series. Open it with {@link sqzmomOpen} or
 * {@link sqzmomOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/sqzmom](https://ta-lib.org/functions/sqzmom).
 */
export class SqzmomStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInBBPeriod: number = 0;
  /** @internal */
  optInNbDev: number = 0;
  /** @internal */
  optInKCPeriod: number = 0;
  /** @internal */
  optInFactorWide: number = 0;
  /** @internal */
  optInFactorNormal: number = 0;
  /** @internal */
  optInFactorNarrow: number = 0;
  /** @internal */
  cur_outMomentum: number = 0;
  /** @internal */
  cur_outSqueeze: number = 0;
  /** @internal */
  sub0!: MidpriceStream;
  /** @internal */
  sub1!: SmaStream;
  /** @internal */
  sub2!: TrangeStream;
  /** @internal */
  sub3!: SmaStream;
  /** @internal */
  sub4!: SmaStream;
  /** @internal */
  sub5!: StddevStream;
  /** @internal */
  sub6!: LinearregStream;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): SqzmomStream {
    return new SqzmomStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link sqzmom} reports them over the same bars. Each accepted `update` and each
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
      throw failure('SQZMOM advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar, writing the new values into `out`.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Closing price of each bar.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number, inClose: number, out: SqzmomOut): void {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('SQZMOM update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (typeof out !== 'object' || out === null) {
      throw badSink('SQZMOM update', 'SqzmomOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('SQZMOM update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    sqzmomStepImpl(this, inHigh, inLow, inClose);
    this.outRangeCount++;
    out.momentum = this.cur_outMomentum;
    out.squeeze = this.cur_outSqueeze;
  }

  /**
   * Writes into `out` what the next `update` with the same bar would write,
   * committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Closing price of each bar.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number, inClose: number, out: SqzmomOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('SQZMOM peek', 'SqzmomOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('SQZMOM peek', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let band = 0;
    let centre = 0;
    let dev = 0;
    let lo = 0;
    let up = 0;
    let cur_tempDev = 0;
    let cur_tempKCM = 0;
    let cur_tempWork = 0;
    let cur_tempMid = 0;
    let cur_tempSD = 0;
    let cur_outMomentum = 0;
    let cur_outSqueeze = 0;
    cur_tempDev = sp.sub0.peek(inHigh, inLow);
    cur_tempKCM = sp.sub1.peek(inClose);
    cur_tempDev = inClose - (cur_tempDev + cur_tempKCM) / 2.0;
    cur_tempWork = sp.sub2.peek(inHigh, inLow, inClose);
    cur_tempWork = sp.sub3.peek(cur_tempWork);
    cur_tempMid = sp.sub4.peek(inClose);
    cur_tempSD = sp.sub5.peek(inClose);
    cur_outMomentum = sp.sub6.peek(cur_tempDev);
    dev = cur_tempSD * sp.optInNbDev;
    up = cur_tempMid + dev;
    lo = cur_tempMid - dev;
    centre = cur_tempKCM;
    band = cur_tempWork;
    if (lo > centre - sp.optInFactorNarrow * band && up < centre + sp.optInFactorNarrow * band) {
      cur_outSqueeze = 3;
    } else if (lo > centre - sp.optInFactorNormal * band && up < centre + sp.optInFactorNormal * band) {
      cur_outSqueeze = 2;
    } else if (lo > centre - sp.optInFactorWide * band && up < centre + sp.optInFactorWide * band) {
      cur_outSqueeze = 1;
    } else if (lo < centre - sp.optInFactorWide * band && up > centre + sp.optInFactorWide * band) {
      cur_outSqueeze = -1;
    } else {
      cur_outSqueeze = 0;
    }
    out.momentum = cur_outMomentum;
    out.squeeze = cur_outSqueeze;
  }

  /**
   * The value at the last bar this stream counted; `peek` does not change it.
   *
   * @param out - Receives the outputs.
   */
  value(out: SqzmomOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('SQZMOM value', 'SqzmomOut');
    }
    out.momentum = this.cur_outMomentum;
    out.squeeze = this.cur_outSqueeze;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): SqzmomStream {
    const c = new SqzmomStream(this.core);
    c.optInBBPeriod = this.optInBBPeriod;
    c.optInNbDev = this.optInNbDev;
    c.optInKCPeriod = this.optInKCPeriod;
    c.optInFactorWide = this.optInFactorWide;
    c.optInFactorNormal = this.optInFactorNormal;
    c.optInFactorNarrow = this.optInFactorNarrow;
    c.cur_outMomentum = this.cur_outMomentum;
    c.cur_outSqueeze = this.cur_outSqueeze;
    c.sub0 = this.sub0.clone();
    c.sub1 = this.sub1.clone();
    c.sub2 = this.sub2.clone();
    c.sub3 = this.sub3.clone();
    c.sub4 = this.sub4.clone();
    c.sub5 = this.sub5.clone();
    c.sub6 = this.sub6.clone();
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one SQZMOM bar, written into an object the caller owns and
 * reuses: valid until the next call that writes it.
 */
export class SqzmomOut {
  /** Linear regression of the close against the Donchian/SMA anchor. */
  momentum: number = 0;
  /** Compression level: 3 narrow, 2 normal, 1 wide, -1 released, 0 otherwise. */
  squeeze: number = 0;
}

function sqzmomStepImpl(sp: SqzmomStream, inHigh: number, inLow: number, inClose: number): void {
  let band = 0;
  let centre = 0;
  let dev = 0;
  let lo = 0;
  let up = 0;
  let cur_tempDev = 0;
  let cur_tempKCM = 0;
  let cur_tempWork = 0;
  let cur_tempMid = 0;
  let cur_tempSD = 0;
  let cur_outMomentum = 0;
  let cur_outSqueeze = 0;
  cur_tempDev = sp.sub0.update(inHigh, inLow);
  cur_tempKCM = sp.sub1.update(inClose);
  cur_tempDev = inClose - (cur_tempDev + cur_tempKCM) / 2.0;
  cur_tempWork = sp.sub2.update(inHigh, inLow, inClose);
  cur_tempWork = sp.sub3.update(cur_tempWork);
  cur_tempMid = sp.sub4.update(inClose);
  cur_tempSD = sp.sub5.update(inClose);
  cur_outMomentum = sp.sub6.update(cur_tempDev);
  dev = cur_tempSD * sp.optInNbDev;
  up = cur_tempMid + dev;
  lo = cur_tempMid - dev;
  centre = cur_tempKCM;
  band = cur_tempWork;
  if (lo > centre - sp.optInFactorNarrow * band && up < centre + sp.optInFactorNarrow * band) {
    cur_outSqueeze = 3;
  } else if (lo > centre - sp.optInFactorNormal * band && up < centre + sp.optInFactorNormal * band) {
    cur_outSqueeze = 2;
  } else if (lo > centre - sp.optInFactorWide * band && up < centre + sp.optInFactorWide * band) {
    cur_outSqueeze = 1;
  } else if (lo < centre - sp.optInFactorWide * band && up > centre + sp.optInFactorWide * band) {
    cur_outSqueeze = -1;
  } else {
    cur_outSqueeze = 0;
  }
  sp.cur_outMomentum = cur_outMomentum;
  sp.cur_outSqueeze = cur_outSqueeze;
}

function sqzmomOpenImpl(
  sp: SqzmomStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInBBPeriod: number,
  optInNbDev: number,
  optInKCPeriod: number,
  optInFactorWide: number,
  optInFactorNormal: number,
  optInFactorNarrow: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outMomentum: Float64Array,
  outSqueeze: Int32Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let lookbackTotal = 0;
  let stageLookback = 0;
  let stageIdx = 0;
  let stageLen = 0;
  let i = 0;
  let n = 0;
  const tempBegIdx = new MInteger();
  const tempNbElement = new MInteger();
  const kcmNb = new MInteger();
  const sdNb = new MInteger();
  let kcmShift = 0;
  let tempDev!: Float64Array;
  let tempWork!: Float64Array;
  let tempKCM!: Float64Array;
  let tempMid!: Float64Array;
  let tempSD!: Float64Array;
  let dev = 0;
  let up = 0;
  let lo = 0;
  let centre = 0;
  let band = 0;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length || inClose.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInBBPeriod === Core.INTEGER_DEFAULT) {
    optInBBPeriod = 20;
  } else if (optInBBPeriod < 2 || optInBBPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInNbDev === Core.REAL_DEFAULT) {
    optInNbDev = 2.0;
  } else if (!(optInNbDev >= 0e0 && optInNbDev <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInKCPeriod === Core.INTEGER_DEFAULT) {
    optInKCPeriod = 20;
  } else if (optInKCPeriod < 2 || optInKCPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInFactorWide === Core.REAL_DEFAULT) {
    optInFactorWide = 2.0;
  } else if (!(optInFactorWide >= 0e0 && optInFactorWide <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInFactorNormal === Core.REAL_DEFAULT) {
    optInFactorNormal = 1.5;
  } else if (!(optInFactorNormal >= 0e0 && optInFactorNormal <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (optInFactorNarrow === Core.REAL_DEFAULT) {
    optInFactorNarrow = 1.0;
  } else if (!(optInFactorNarrow >= 0e0 && optInFactorNarrow <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen < sqzmomLookback(optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const sc_outMomentum = outStride === 1 ? outMomentum : new Float64Array(historyLen);
  const sc_outSqueeze = outStride === 1 ? outSqueeze : new Int32Array(historyLen);
  /* Each rolling statistic is a call to the shipped function, so it carries
   * that function's own arithmetic bit for bit:
   *
   *   BBU, BBL = sma(close, b) +/- nbdev * stddev(close, b, 1.0)
   *   KCM      = sma(close, k)
   *   BAND     = sma(trange(high,low,close), k)      the simple mean of TR
   *   DMID     = midprice(high, low, k)
   *   DEV      = close - (DMID + KCM)/2
   *   momentum = linearreg(DEV, k)
   *
   * A composed sub-call cannot be handed an MA-type constant, so the bands
   * are the sma + stddev pair and not a bbands call.
   *
   * Two chains need their input one window earlier than the first output bar:
   * sma(k) over TR, and linearreg(k) over DEV. Both windows are k-1 long, so
   * one staged range serves both.
   */
  /* A channel wider than the one it should nest in breaks the ladder: a bar
   * could then be inside "narrow" and outside "normal". Ties are allowed, and
   * wide == normal is the classic on/off/none squeeze.
   */
  if (optInFactorNormal > optInFactorWide || optInFactorNarrow > optInFactorNormal) {
    return RetCode.BAD_PARAM;
  }
  lookbackTotal = sqzmomLookback(optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, core);
  if (lookbackTotal > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  n = endIdx - startIdx + 1;
  stageLookback = linearregLookback(optInKCPeriod, core);
  stageIdx = startIdx - stageLookback;
  stageLen = n + stageLookback;
  tempDev = new Float64Array(stageLen);
  tempWork = new Float64Array(stageLen);
  tempKCM = new Float64Array(stageLen);
  tempMid = new Float64Array(n);
  tempSD = new Float64Array(n);
  /* The momentum anchor: the Donchian midpoint and the Keltner centre, averaged.
   * midprice is bit-identical to donchian's middle band.
   */
  const sub0 = midpriceOpenAndFillInternal(core, inHigh, inLow, stageIdx, optInKCPeriod, outBegIdx, outNBElement, tempDev);
  const sub1 = smaOpenAndFillInternal(core, inClose, stageIdx, optInKCPeriod, tempBegIdx, kcmNb, tempKCM);
  /* DEV, over the staged range. The close is read at the index midprice
   * reported for the range, which is what lets this loop stream: keep
   * outBegIdx as midprice's receiver, and reuse it for no call above here.
   */
  for (i = 0; i < stageLen; i += 1) {
    tempDev[i] = inClose[outBegIdx.value + i] - (tempDev[i] + tempKCM[i]) / 2.0;
  }
  /* The compression band: the simple mean of the true range, not an ATR.
   * trange's own lookback is 1, and stageIdx is at least 1 because the
   * compression chain alone puts lookbackTotal at k or more.
   */
  const sub2 = trangeOpenAndFillInternal(core, inHigh, inLow, inClose, stageIdx, tempBegIdx, tempNbElement, tempWork);
  const sub3 = smaOpenInternal(core, tempWork.slice(0, (stageLen - 1) + 1), 0, optInKCPeriod);
  const _xr0 = sma(0, stageLen - 1, tempWork, optInKCPeriod, tempWork, core);
  tempBegIdx.value = _xr0.begIdx;
  tempNbElement.value = _xr0.count;
  const sub4 = smaOpenAndFillInternal(core, inClose, startIdx, optInBBPeriod, tempBegIdx, tempNbElement, tempMid);
  const sub5 = stddevOpenAndFillInternal(core, inClose, startIdx, optInBBPeriod, 1.0, tempBegIdx, sdNb, tempSD);
  /* The centre is staged one regression window early; the shift puts it on
   * the bar the bands are on.
   */
  kcmShift = kcmNb.value - sdNb.value;
  /* LAST of the calls, because it is the first to write a caller buffer. Every
   * read of high, low and close is above it, so an output aliased onto one of
   * them -- which the abstract layer tests on purpose -- is still intact when it
   * is read.
   *
   * The regression window closes on the first output bar, so entering it at 0 of
   * the staged range puts its first fitted value on startIdx.
   */
  const sub6 = linearregOpenAndFillInternal(core, tempDev.slice(0, (stageLen - 1) + 1), 0, optInKCPeriod, tempBegIdx, tempNbElement, sc_outMomentum);
  /* The ordinal. Every comparison is strict: a band exactly on a channel
   * edge is not inside that channel and not outside it either.
   *
   * Keep the deviation a rounded product added to and subtracted from the
   * middle band, as bbands does. Written as one expression per band, the
   * upper one contracts into a fused multiply-add and the lower one cannot,
   * and a band within one ulp of an edge then changes level.
   */
  for (i = 0; i < n; i += 1) {
    dev = tempSD[i] * optInNbDev;
    up = tempMid[i] + dev;
    lo = tempMid[i] - dev;
    centre = tempKCM[i + kcmShift];
    band = tempWork[i];
    if (lo > centre - optInFactorNarrow * band && up < centre + optInFactorNarrow * band) {
      sc_outSqueeze[i] = 3;
    } else if (lo > centre - optInFactorNormal * band && up < centre + optInFactorNormal * band) {
      sc_outSqueeze[i] = 2;
    } else if (lo > centre - optInFactorWide * band && up < centre + optInFactorWide * band) {
      sc_outSqueeze[i] = 1;
    } else if (lo < centre - optInFactorWide * band && up > centre + optInFactorWide * band) {
      sc_outSqueeze[i] = -1;
    } else {
      sc_outSqueeze[i] = 0;
    }
  }
  outBegIdx.value = startIdx;
  outNBElement.value = n;
  if (outNBElement.value < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  sp.optInBBPeriod = optInBBPeriod;
  sp.optInNbDev = optInNbDev;
  sp.optInKCPeriod = optInKCPeriod;
  sp.optInFactorWide = optInFactorWide;
  sp.optInFactorNormal = optInFactorNormal;
  sp.optInFactorNarrow = optInFactorNarrow;
  sp.sub0 = sub0;
  sp.sub1 = sub1;
  sp.sub2 = sub2;
  sp.sub3 = sub3;
  sp.sub4 = sub4;
  sp.sub5 = sub5;
  sp.sub6 = sub6;
  sp.cur_outMomentum = sc_outMomentum[outNBElement.value - 1];
  sp.cur_outSqueeze = sc_outSqueeze[outNBElement.value - 1];
  return RetCode.SUCCESS;
}

/** @internal */
export function sqzmomOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInBBPeriod: number,
  optInNbDev: number,
  optInKCPeriod: number,
  optInFactorWide: number,
  optInFactorNormal: number,
  optInFactorNarrow: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outMomentum: Float64Array,
  outSqueeze: Int32Array,
): SqzmomStream {
  const sp = SqzmomStream.of(core);
  const retCode = sqzmomOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, outBegIdx, outNBElement, outMomentum, outSqueeze, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SQZMOM openAndFill', inHigh.length, startIdx, sqzmomLookback(optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, core));
  }
  throw streamFailure('SQZMOM openAndFill', retCode);
}

/** @internal */
export function sqzmomOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInBBPeriod: number,
  optInNbDev: number,
  optInKCPeriod: number,
  optInFactorWide: number,
  optInFactorNormal: number,
  optInFactorNarrow: number,
): SqzmomStream {
  const sp = SqzmomStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outMomentum = new Float64Array(1);
  const sink_outSqueeze = new Int32Array(1);
  const retCode = sqzmomOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, outBegIdx, outNBElement, sink_outMomentum, sink_outSqueeze, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('SQZMOM open', inHigh.length, startIdx, sqzmomLookback(optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, core));
  }
  throw streamFailure('SQZMOM open', retCode);
}

/**
 * Opens a live SQZMOM stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link sqzmom} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Closing price of each bar. Every history has
 *   the same length.
 * @param optInBBPeriod - Number of bars in the Bollinger Band window (default 20;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInNbDev - Number of standard deviations the Bollinger Bands are placed
 *   at (default 2; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInKCPeriod - Number of bars in the Keltner Channel, the true-range
 *   mean and the regression (default 20; range 2 to 100000; `Core.INTEGER_DEFAULT`
 *   selects the default).
 * @param optInFactorWide - Keltner width, in true-range means, of the widest
 *   channel (default 2; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInFactorNormal - Keltner width of the classic channel (default 1.5;
 *   minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInFactorNarrow - Keltner width of the tightest channel (default 1;
 *   minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `sqzmomLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a real
 *   parameter that is not a number, a parameter outside its range or histories of
 *   different lengths.
 *
 * @example
 * ```ts
 * import { SqzmomOut, sqzmomOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = sqzmomOpen(high, low, close, 20, 2, 20, 2, 1.5, 1);
 * const peeked = new SqzmomOut();
 * const updated = new SqzmomOut();
 * stream.peek(101.4, 99.1, 100.9, peeked); // a forming bar: commits nothing
 * stream.update(101.4, 99.1, 100.9, updated); // the bar closes
 * ```
 */
export function sqzmomOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInBBPeriod: number,
  optInNbDev: number,
  optInKCPeriod: number,
  optInFactorWide: number,
  optInFactorNormal: number,
  optInFactorNarrow: number,
  core: Core = Core.DEFAULT,
): SqzmomStream {
  optInBBPeriod = requireInt32('SQZMOM open', 'optInBBPeriod', optInBBPeriod);
  optInNbDev = requireReal('SQZMOM open', 'optInNbDev', optInNbDev);
  optInKCPeriod = requireInt32('SQZMOM open', 'optInKCPeriod', optInKCPeriod);
  optInFactorWide = requireReal('SQZMOM open', 'optInFactorWide', optInFactorWide);
  optInFactorNormal = requireReal('SQZMOM open', 'optInFactorNormal', optInFactorNormal);
  optInFactorNarrow = requireReal('SQZMOM open', 'optInFactorNarrow', optInFactorNarrow);
  requireArgument('SQZMOM open', 'inHigh', inHigh);
  requireHistory('SQZMOM open', inHigh.length);
  requireArgument('SQZMOM open', 'inLow', inLow);
  requireArgument('SQZMOM open', 'inClose', inClose);
  requireHistoryLength('SQZMOM open', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('SQZMOM open', 'inClose', inClose.length, inHigh.length);
  return sqzmomOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow);
}

/**
 * {@link sqzmomOpen} that also fills the outputs over the whole history in the
 * same pass, bit-identically to {@link sqzmom}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Closing price of each bar. Every history has
 *   the same length.
 * @param optInBBPeriod - Number of bars in the Bollinger Band window (default 20;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInNbDev - Number of standard deviations the Bollinger Bands are placed
 *   at (default 2; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInKCPeriod - Number of bars in the Keltner Channel, the true-range
 *   mean and the regression (default 20; range 2 to 100000; `Core.INTEGER_DEFAULT`
 *   selects the default).
 * @param optInFactorWide - Keltner width, in true-range means, of the widest
 *   channel (default 2; minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInFactorNormal - Keltner width of the classic channel (default 1.5;
 *   minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param optInFactorNarrow - Keltner width of the tightest channel (default 1;
 *   minimum 0; `Core.REAL_DEFAULT` selects the default).
 * @param outMomentum - Linear regression of the close against the Donchian/SMA
 *   anchor. Must hold `historyLen - lookback` values and share no memory with an
 *   input or another output.
 * @param outSqueeze - Compression level: 3 narrow, 2 normal, 1 wide, -1 released,
 *   0 otherwise. Must hold `historyLen - lookback` values and share no memory with
 *   an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `sqzmomLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a real
 *   parameter that is not a number, a parameter outside its range, histories of
 *   different lengths or an output too short or starting where another output or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { sqzmomOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const momentum = new Float64Array(high.length);
 * const squeeze = new Int32Array(high.length);
 * const stream = sqzmomOpenAndFill(
 *   high, low, close,
 *   20, 2, 20, 2, 1.5, 1,
 *   momentum, squeeze,
 * );
 * const { begIdx, count } = stream.outRange();
 * // momentum[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function sqzmomOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInBBPeriod: number,
  optInNbDev: number,
  optInKCPeriod: number,
  optInFactorWide: number,
  optInFactorNormal: number,
  optInFactorNarrow: number,
  outMomentum: Float64Array,
  outSqueeze: Int32Array,
  core: Core = Core.DEFAULT,
): SqzmomStream {
  optInBBPeriod = requireInt32('SQZMOM openAndFill', 'optInBBPeriod', optInBBPeriod);
  optInNbDev = requireReal('SQZMOM openAndFill', 'optInNbDev', optInNbDev);
  optInKCPeriod = requireInt32('SQZMOM openAndFill', 'optInKCPeriod', optInKCPeriod);
  optInFactorWide = requireReal('SQZMOM openAndFill', 'optInFactorWide', optInFactorWide);
  optInFactorNormal = requireReal('SQZMOM openAndFill', 'optInFactorNormal', optInFactorNormal);
  optInFactorNarrow = requireReal('SQZMOM openAndFill', 'optInFactorNarrow', optInFactorNarrow);
  requireArgument('SQZMOM openAndFill', 'inHigh', inHigh);
  requireHistory('SQZMOM openAndFill', inHigh.length);
  requireArgument('SQZMOM openAndFill', 'inLow', inLow);
  requireArgument('SQZMOM openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('SQZMOM openAndFill', inHigh.length, sqzmomLookback(optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, core));
  requireHistoryLength('SQZMOM openAndFill', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('SQZMOM openAndFill', 'inClose', inClose.length, inHigh.length);
  requireLength('SQZMOM openAndFill', 'outMomentum', outMomentum, guardOutLen);
  requireLength('SQZMOM openAndFill', 'outSqueeze', outSqueeze, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outMomentum.buffer === inHighF64.buffer && outMomentum.byteOffset === inHighF64.byteOffset) || (outMomentum.buffer === inLowF64.buffer && outMomentum.byteOffset === inLowF64.byteOffset) || (outMomentum.buffer === inCloseF64.buffer && outMomentum.byteOffset === inCloseF64.byteOffset) || (outSqueeze.buffer === inHighF64.buffer && outSqueeze.byteOffset === inHighF64.byteOffset) || (outSqueeze.buffer === inLowF64.buffer && outSqueeze.byteOffset === inLowF64.byteOffset) || (outSqueeze.buffer === inCloseF64.buffer && outSqueeze.byteOffset === inCloseF64.byteOffset) || (outMomentum.buffer === outSqueeze.buffer && outMomentum.byteOffset === outSqueeze.byteOffset)) {
    throw streamFailure('SQZMOM openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outMomentum, inHighF64) || overlaps(outMomentum, inLowF64) || overlaps(outMomentum, inCloseF64) || overlaps(outSqueeze, inHighF64) || overlaps(outSqueeze, inLowF64) || overlaps(outSqueeze, inCloseF64) || overlaps(outMomentum, outSqueeze))) {
    throw streamFailure('SQZMOM openAndFill', RetCode.BAD_PARAM);
  }
  return sqzmomOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, new MInteger(), new MInteger(), outMomentum, outSqueeze);
}
