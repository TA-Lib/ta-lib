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

import { badSink, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireReal, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { LinearregStream, linearregLookbackPositional, linearregOpenAndFillInternal, linearregPositional } from './linearreg.ts';
import { MidpriceStream, midpriceLookbackPositional, midpriceOpenAndFillInternal, midpricePositional } from './midprice.ts';
import { SmaStream, smaLookbackPositional, smaOpenAndFillInternal, smaOpenInternal, smaPositional } from './sma.ts';
import { StddevStream, stddevLookbackPositional, stddevOpenAndFillInternal, stddevPositional } from './stddev.ts';
import { TrangeStream, trangeLookbackPositional, trangeOpenAndFillInternal, trangePositional } from './trange.ts';

/** @internal */
export function sqzmomLookbackPositional(optInBBPeriod: number, optInNbDev: number, optInKCPeriod: number, optInFactorWide: number, optInFactorNormal: number, optInFactorNarrow: number, core: Core = Core.DEFAULT): number {
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
  lookbackTotal = smaLookbackPositional(optInBBPeriod, core);
  leg = stddevLookbackPositional(optInBBPeriod, 1.0, core);
  if (leg > lookbackTotal) {
    lookbackTotal = leg;
  }
  leg = trangeLookbackPositional(core) + smaLookbackPositional(optInKCPeriod, core);
  if (leg > lookbackTotal) {
    lookbackTotal = leg;
  }
  leg = midpriceLookbackPositional(optInKCPeriod, core) + linearregLookbackPositional(optInKCPeriod, core);
  if (leg > lookbackTotal) {
    lookbackTotal = leg;
  }
  return lookbackTotal;
}

/** @internal */
export function sqzmomDisplayShiftPositional(optInBBPeriod: number, optInNbDev: number, optInKCPeriod: number, optInFactorWide: number, optInFactorNormal: number, optInFactorNarrow: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (sqzmomLookbackPositional(optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, core) < 0) {
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
  lookbackTotal = sqzmomLookbackPositional(optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, core);
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
  stageLookback = linearregLookbackPositional(optInKCPeriod, core);
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
  const _xr0 = midpricePositional(stageIdx, endIdx, inHigh, inLow, optInKCPeriod, tempDev, core);
  outBegIdx.value = _xr0.begIdx;
  outNBElement.value = _xr0.count;
  const _xr1 = smaPositional(stageIdx, endIdx, inClose, optInKCPeriod, tempKCM, core);
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
  const _xr2 = trangePositional(stageIdx, endIdx, inHigh, inLow, inClose, tempWork, core);
  tempBegIdx.value = _xr2.begIdx;
  tempNbElement.value = _xr2.count;
  const _xr3 = smaPositional(0, stageLen - 1, tempWork, optInKCPeriod, tempWork, core);
  tempBegIdx.value = _xr3.begIdx;
  tempNbElement.value = _xr3.count;
  const _xr4 = smaPositional(startIdx, endIdx, inClose, optInBBPeriod, tempMid, core);
  tempBegIdx.value = _xr4.begIdx;
  tempNbElement.value = _xr4.count;
  const _xr5 = stddevPositional(startIdx, endIdx, inClose, optInBBPeriod, 1.0, tempSD, core);
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
  const _xr6 = linearregPositional(0, stageLen - 1, tempDev, optInKCPeriod, outMomentum, core);
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

const PARAM_RANGES = [['bbPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['nbDev', Core.REAL_DEFAULT, 0e0, Core.REAL_MAX, 'in 0..3e37'], ['kcPeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000'], ['factorWide', Core.REAL_DEFAULT, 0e0, Core.REAL_MAX, 'in 0..3e37'], ['factorNormal', Core.REAL_DEFAULT, 0e0, Core.REAL_MAX, 'in 0..3e37'], ['factorNarrow', Core.REAL_DEFAULT, 0e0, Core.REAL_MAX, 'in 0..3e37']] as const;

/** @internal */
export function sqzmomPositional(
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
  optInBBPeriod = requireInt32('SQZMOM', 'bbPeriod', optInBBPeriod);
  optInNbDev = requireReal('SQZMOM', 'nbDev', optInNbDev);
  optInKCPeriod = requireInt32('SQZMOM', 'kcPeriod', optInKCPeriod);
  optInFactorWide = requireReal('SQZMOM', 'factorWide', optInFactorWide);
  optInFactorNormal = requireReal('SQZMOM', 'factorNormal', optInFactorNormal);
  optInFactorNarrow = requireReal('SQZMOM', 'factorNarrow', optInFactorNarrow);
  requireIndexRange('SQZMOM', startIdx, endIdx);
  const guardLookback = sqzmomLookbackPositional(optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, core);
  if (guardLookback < 0) {
    throw rejectedParams('SQZMOM', PARAM_RANGES, optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow);
  }
  const guardStart = clampedStart('SQZMOM', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('SQZMOM', 'prices.high', inHigh, guardInLen);
  requireLength('SQZMOM', 'prices.low', inLow, guardInLen);
  requireLength('SQZMOM', 'prices.close', inClose, guardInLen);
  requireOutput('SQZMOM', 'outMomentum', outMomentum, guardOutLen, Float64Array);
  requireOutput('SQZMOM', 'outSqueeze', outSqueeze, guardOutLen, Int32Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outMomentum, inHighF64)) {
    throw sharedMemory('SQZMOM', 'outMomentum', 'prices.high');
  }
  if (overlapsPartly(outMomentum, inLowF64)) {
    throw sharedMemory('SQZMOM', 'outMomentum', 'prices.low');
  }
  if (overlapsPartly(outMomentum, inCloseF64)) {
    throw sharedMemory('SQZMOM', 'outMomentum', 'prices.close');
  }
  if (overlaps(outSqueeze, inHighF64)) {
    throw sharedMemory('SQZMOM', 'outSqueeze', 'prices.high');
  }
  if (overlaps(outSqueeze, inLowF64)) {
    throw sharedMemory('SQZMOM', 'outSqueeze', 'prices.low');
  }
  if (overlaps(outSqueeze, inCloseF64)) {
    throw sharedMemory('SQZMOM', 'outSqueeze', 'prices.close');
  }
  if (overlaps(outMomentum, outSqueeze)) {
    throw sharedMemory('SQZMOM', 'outMomentum', 'outSqueeze');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = sqzmomImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, outBegIdx, outNBElement, outMomentum, outSqueeze);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('SQZMOM', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link sqzmom}, {@link sqzmomLookback}, {@link sqzmomDisplayShift},
 * {@link sqzmomOpen} and {@link sqzmomOpenAndFill}. Every key is optional: an
 * omitted or `undefined` key takes its default, and a call ignores the keys it has
 * no use for.
 */
export interface SqzmomOptions {
  /**
   * Number of bars in the Bollinger Band window (default 20; range 2 to 100000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  bbPeriod?: number | undefined;
  /**
   * Number of standard deviations the Bollinger Bands are placed at (default 2;
   * minimum 0; `Core.REAL_DEFAULT` selects the default).
   */
  nbDev?: number | undefined;
  /**
   * Number of bars in the Keltner Channel, the true-range mean and the regression
   * (default 20; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  kcPeriod?: number | undefined;
  /**
   * Keltner width, in true-range means, of the widest channel (default 2; minimum 0;
   * `Core.REAL_DEFAULT` selects the default).
   */
  factorWide?: number | undefined;
  /**
   * Keltner width of the classic channel (default 1.5; minimum 0;
   * `Core.REAL_DEFAULT` selects the default).
   */
  factorNormal?: number | undefined;
  /**
   * Keltner width of the tightest channel (default 1; minimum 0; `Core.REAL_DEFAULT`
   * selects the default).
   */
  factorNarrow?: number | undefined;
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
   * Buffer for `outMomentum`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input; a batch call may compute in place over a
   * `Float64Array` input of the same length.
   */
  outMomentum?: Float64Array | undefined;
  /**
   * Buffer for `outSqueeze`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input.
   */
  outSqueeze?: Int32Array | undefined;
}

/** What {@link sqzmom} returns, and {@link sqzmomOpenAndFill} with its stream. */
export interface SqzmomResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Linear regression of the close against the Donchian/SMA anchor. `outMomentum[k]`
   * is the value at input bar `begIdx + k`.
   */
  outMomentum: Float64Array;
  /**
   * Compression level: 3 narrow, 2 normal, 1 wide, -1 released, 0 otherwise.
   * `outSqueeze[k]` is the value at input bar `begIdx + k`.
   */
  outSqueeze: Int32Array;
}

/**
 * Number of leading input bars {@link sqzmom} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SqzmomOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function sqzmomLookback(options?: SqzmomOptions): number {
  const o = optionsOf('SQZMOM lookback', options);
  return sqzmomLookbackPositional(orDefault(o.bbPeriod, Core.INTEGER_DEFAULT), orDefault(o.nbDev, Core.REAL_DEFAULT), orDefault(o.kcPeriod, Core.INTEGER_DEFAULT), orDefault(o.factorWide, Core.REAL_DEFAULT), orDefault(o.factorNormal, Core.REAL_DEFAULT), orDefault(o.factorNarrow, Core.REAL_DEFAULT), coreOf('SQZMOM lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link sqzmom}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SqzmomOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function sqzmomDisplayShift(outputIdx: number, options?: SqzmomOptions): number {
  const o = optionsOf('SQZMOM displayShift', options);
  return sqzmomDisplayShiftPositional(orDefault(o.bbPeriod, Core.INTEGER_DEFAULT), orDefault(o.nbDev, Core.REAL_DEFAULT), orDefault(o.kcPeriod, Core.INTEGER_DEFAULT), orDefault(o.factorWide, Core.REAL_DEFAULT), orDefault(o.factorNormal, Core.REAL_DEFAULT), orDefault(o.factorNarrow, Core.REAL_DEFAULT), outputIdx, coreOf('SQZMOM displayShift', o.core));
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
 * `outMomentum[k]` is the value at input bar `begIdx + k`; every output is exactly
 * as long as the number of values written, and nothing is padded. A range that
 * ends before {@link sqzmomLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `high`: High
 *   price of each bar. `low`: Low price of each bar. `close`: Closing price of each
 *   bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link SqzmomOptions}.
 * @returns `begIdx` and the values; see {@link SqzmomResult}. Each output is a
 *   view of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, an integer parameter that is not an int32, a real parameter that is not a
 *   number, a parameter outside its range, an input too short for the range, a
 *   buffer that is empty, too short or of the wrong type, two buffers sharing memory
 *   or a buffer sharing memory with a `Float64Array` input it is not. Checked before
 *   anything is written.
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
 * const { begIdx, outMomentum, outSqueeze } = sqzmom({ high, low, close }, {
 *   bbPeriod: 20,
 *   nbDev: 2,
 *   kcPeriod: 20,
 *   factorWide: 2,
 *   factorNormal: 1.5,
 *   factorNarrow: 1,
 * });
 * // outMomentum[k] is the value at bar begIdx + k
 * ```
 * @see [`bbands`](https://ta-lib.org/functions/bbands)
 * @see [`kc`](https://ta-lib.org/functions/kc)
 * @see [`trange`](https://ta-lib.org/functions/trange)
 * @see [`midprice`](https://ta-lib.org/functions/midprice)
 * @see [`linearreg`](https://ta-lib.org/functions/linearreg)
 */
export function sqzmom(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: SqzmomOptions): SqzmomResult {
  requireArgument('SQZMOM', 'prices', prices);
  const o = optionsOf('SQZMOM', options);
  if (o.endIdx === undefined) {
    requireSeries('SQZMOM', 'prices.high', prices.high);
    requireSeries('SQZMOM', 'prices.low', prices.low);
    requireSeries('SQZMOM', 'prices.close', prices.close);
  }
  const optInBBPeriod = orDefault(o.bbPeriod, Core.INTEGER_DEFAULT);
  const optInNbDev = orDefault(o.nbDev, Core.REAL_DEFAULT);
  const optInKCPeriod = orDefault(o.kcPeriod, Core.INTEGER_DEFAULT);
  const optInFactorWide = orDefault(o.factorWide, Core.REAL_DEFAULT);
  const optInFactorNormal = orDefault(o.factorNormal, Core.REAL_DEFAULT);
  const optInFactorNarrow = orDefault(o.factorNarrow, Core.REAL_DEFAULT);
  const core = coreOf('SQZMOM', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, sqzmomLookbackPositional(optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, core), prices.high, prices.low, prices.close);
  const outMomentum = o.outMomentum === undefined ? new Float64Array(capacity) : o.outMomentum;
  const outSqueeze = o.outSqueeze === undefined ? new Int32Array(capacity) : o.outSqueeze;
  const range = sqzmomPositional(startIdx, endIdx, prices.high, prices.low, prices.close, optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, outMomentum, outSqueeze, core);
  return { begIdx: range.begIdx, outMomentum: outMomentum.subarray(0, range.count), outSqueeze: outSqueeze.subarray(0, range.count) };
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
      throw streamFailure('SQZMOM advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns its outputs.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Closing price of each bar.
   * @param out - Receives the outputs and is returned; a new {@link SqzmomOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included, before anything moves.
   *   `TALibError` with code `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached
   *   bar `Core.INDEX_MAX`.
   */
  update(high: number, low: number, close: number, out?: SqzmomOut): SqzmomOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('SQZMOM update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('SQZMOM update', 'SqzmomOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('SQZMOM update', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    sqzmomStepImpl(this, high, low, close);
    this.outRangeCount++;
    if (out === undefined) {
      out = new SqzmomOut();
    }
    out.outMomentum = this.cur_outMomentum;
    out.outSqueeze = this.cur_outSqueeze;
    return out;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Closing price of each bar.
   * @param out - Receives the outputs and is returned; a new {@link SqzmomOut} when
   *   omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included.
   */
  peek(high: number, low: number, close: number, out?: SqzmomOut): SqzmomOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('SQZMOM peek', 'SqzmomOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('SQZMOM peek', !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
    const inClose = close;
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
    if (out === undefined) {
      out = new SqzmomOut();
    }
    out.outMomentum = cur_outMomentum;
    out.outSqueeze = cur_outSqueeze;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link SqzmomOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a sink that is not an object,
   *   `null` included.
   */
  value(out?: SqzmomOut): SqzmomOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('SQZMOM value', 'SqzmomOut');
    }
    if (out === undefined) {
      out = new SqzmomOut();
    }
    out.outMomentum = this.cur_outMomentum;
    out.outSqueeze = this.cur_outSqueeze;
    return out;
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
 * The outputs of one SQZMOM bar, as `update`, `peek` and `value` return them.
 * Passed back to one of them, it is filled and returned again, with no allocation;
 * it holds its values until the next call that writes it. A new one holds `NaN`.
 */
export class SqzmomOut {
  /** Linear regression of the close against the Donchian/SMA anchor. */
  outMomentum: number = NaN;
  /** Compression level: 3 narrow, 2 normal, 1 wide, -1 released, 0 otherwise. */
  outSqueeze: number = NaN;
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
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen < sqzmomLookbackPositional(optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, core) + 1) {
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
  lookbackTotal = sqzmomLookbackPositional(optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, core);
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
  stageLookback = linearregLookbackPositional(optInKCPeriod, core);
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
  const _xr0 = smaPositional(0, stageLen - 1, tempWork, optInKCPeriod, tempWork, core);
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
    throw insufficientHistory('SQZMOM openAndFill', inHigh.length, startIdx, sqzmomLookbackPositional(optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, core));
  }
  if (retCode === RetCode.BAD_PARAM && sqzmomLookbackPositional(optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, core) < 0) {
    throw rejectedParams('SQZMOM openAndFill', PARAM_RANGES, optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow);
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
    throw insufficientHistory('SQZMOM open', inHigh.length, startIdx, sqzmomLookbackPositional(optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, core));
  }
  if (retCode === RetCode.BAD_PARAM && sqzmomLookbackPositional(optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, core) < 0) {
    throw rejectedParams('SQZMOM open', PARAM_RANGES, optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow);
  }
  throw streamFailure('SQZMOM open', retCode);
}

/** @internal */
export function sqzmomOpenPositional(
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
  optInBBPeriod = requireInt32('SQZMOM open', 'bbPeriod', optInBBPeriod);
  optInNbDev = requireReal('SQZMOM open', 'nbDev', optInNbDev);
  optInKCPeriod = requireInt32('SQZMOM open', 'kcPeriod', optInKCPeriod);
  optInFactorWide = requireReal('SQZMOM open', 'factorWide', optInFactorWide);
  optInFactorNormal = requireReal('SQZMOM open', 'factorNormal', optInFactorNormal);
  optInFactorNarrow = requireReal('SQZMOM open', 'factorNarrow', optInFactorNarrow);
  requireArgument('SQZMOM open', 'prices.high', inHigh);
  requireHistory('SQZMOM open', 'prices.high', inHigh.length);
  requireArgument('SQZMOM open', 'prices.low', inLow);
  requireArgument('SQZMOM open', 'prices.close', inClose);
  requireHistoryLength('SQZMOM open', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('SQZMOM open', 'prices.close', inClose.length, inHigh.length);
  return sqzmomOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow);
}

/** @internal */
export function sqzmomOpenAndFillPositional(
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
  optInBBPeriod = requireInt32('SQZMOM openAndFill', 'bbPeriod', optInBBPeriod);
  optInNbDev = requireReal('SQZMOM openAndFill', 'nbDev', optInNbDev);
  optInKCPeriod = requireInt32('SQZMOM openAndFill', 'kcPeriod', optInKCPeriod);
  optInFactorWide = requireReal('SQZMOM openAndFill', 'factorWide', optInFactorWide);
  optInFactorNormal = requireReal('SQZMOM openAndFill', 'factorNormal', optInFactorNormal);
  optInFactorNarrow = requireReal('SQZMOM openAndFill', 'factorNarrow', optInFactorNarrow);
  requireArgument('SQZMOM openAndFill', 'prices.high', inHigh);
  requireHistory('SQZMOM openAndFill', 'prices.high', inHigh.length);
  requireArgument('SQZMOM openAndFill', 'prices.low', inLow);
  requireArgument('SQZMOM openAndFill', 'prices.close', inClose);
  const guardLookback = sqzmomLookbackPositional(optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, core);
  if (guardLookback < 0) {
    throw rejectedParams('SQZMOM openAndFill', PARAM_RANGES, optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow);
  }
  const guardOutLen = openFillCount('SQZMOM openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('SQZMOM openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireHistoryLength('SQZMOM openAndFill', 'prices.close', inClose.length, inHigh.length);
  requireOutput('SQZMOM openAndFill', 'outMomentum', outMomentum, guardOutLen, Float64Array);
  requireOutput('SQZMOM openAndFill', 'outSqueeze', outSqueeze, guardOutLen, Int32Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outMomentum.buffer === inHighF64.buffer && outMomentum.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('SQZMOM openAndFill', 'outMomentum', 'prices.high');
  }
  if (outMomentum.buffer === inLowF64.buffer && outMomentum.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('SQZMOM openAndFill', 'outMomentum', 'prices.low');
  }
  if (outMomentum.buffer === inCloseF64.buffer && outMomentum.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('SQZMOM openAndFill', 'outMomentum', 'prices.close');
  }
  if (outSqueeze.buffer === inHighF64.buffer && outSqueeze.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('SQZMOM openAndFill', 'outSqueeze', 'prices.high');
  }
  if (outSqueeze.buffer === inLowF64.buffer && outSqueeze.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('SQZMOM openAndFill', 'outSqueeze', 'prices.low');
  }
  if (outSqueeze.buffer === inCloseF64.buffer && outSqueeze.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('SQZMOM openAndFill', 'outSqueeze', 'prices.close');
  }
  if (outMomentum.buffer === outSqueeze.buffer && outMomentum.byteOffset === outSqueeze.byteOffset) {
    throw sharedMemory('SQZMOM openAndFill', 'outMomentum', 'outSqueeze');
  }
  if (guardOutLen > 0 && overlaps(outMomentum, inHighF64)) {
    throw sharedMemory('SQZMOM openAndFill', 'outMomentum', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outMomentum, inLowF64)) {
    throw sharedMemory('SQZMOM openAndFill', 'outMomentum', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outMomentum, inCloseF64)) {
    throw sharedMemory('SQZMOM openAndFill', 'outMomentum', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outSqueeze, inHighF64)) {
    throw sharedMemory('SQZMOM openAndFill', 'outSqueeze', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outSqueeze, inLowF64)) {
    throw sharedMemory('SQZMOM openAndFill', 'outSqueeze', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outSqueeze, inCloseF64)) {
    throw sharedMemory('SQZMOM openAndFill', 'outSqueeze', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outMomentum, outSqueeze)) {
    throw sharedMemory('SQZMOM openAndFill', 'outMomentum', 'outSqueeze');
  }
  return sqzmomOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, new MInteger(), new MInteger(), outMomentum, outSqueeze);
}

/**
 * Opens a live SQZMOM stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link sqzmom} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Closing price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link SqzmomOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `sqzmomLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a real parameter that is not a
 *   number, a parameter outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { sqzmomOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = sqzmomOpen({ high, low, close }, {
 *   bbPeriod: 20,
 *   nbDev: 2,
 *   kcPeriod: 20,
 *   factorWide: 2,
 *   factorNormal: 1.5,
 *   factorNarrow: 1,
 * });
 * const peeked = stream.peek(101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function sqzmomOpen(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: SqzmomOptions): SqzmomStream {
  requireArgument('SQZMOM open', 'prices', prices);
  const o = optionsOf('SQZMOM open', options);
  const optInBBPeriod = orDefault(o.bbPeriod, Core.INTEGER_DEFAULT);
  const optInNbDev = orDefault(o.nbDev, Core.REAL_DEFAULT);
  const optInKCPeriod = orDefault(o.kcPeriod, Core.INTEGER_DEFAULT);
  const optInFactorWide = orDefault(o.factorWide, Core.REAL_DEFAULT);
  const optInFactorNormal = orDefault(o.factorNormal, Core.REAL_DEFAULT);
  const optInFactorNarrow = orDefault(o.factorNarrow, Core.REAL_DEFAULT);
  const core = coreOf('SQZMOM open', o.core);
  return sqzmomOpenPositional(prices.high, prices.low, prices.close, optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, core);
}

/**
 * {@link sqzmomOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link sqzmom}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Closing price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link SqzmomOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link sqzmom} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `sqzmomLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a real parameter that is not a
 *   number, a parameter outside its range, histories of different lengths or a
 *   buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
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
 * const {
 *   stream,
 *   begIdx,
 *   outMomentum,
 *   outSqueeze,
 * } = sqzmomOpenAndFill({ high, low, close }, {
 *   bbPeriod: 20,
 *   nbDev: 2,
 *   kcPeriod: 20,
 *   factorWide: 2,
 *   factorNormal: 1.5,
 *   factorNarrow: 1,
 * });
 * // outMomentum[k] is the value at bar begIdx + k
 * ```
 */
export function sqzmomOpenAndFill(prices: { high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number> }, options?: SqzmomOptions): SqzmomResult & { stream: SqzmomStream } {
  requireArgument('SQZMOM openAndFill', 'prices', prices);
  const o = optionsOf('SQZMOM openAndFill', options);
  const optInBBPeriod = orDefault(o.bbPeriod, Core.INTEGER_DEFAULT);
  const optInNbDev = orDefault(o.nbDev, Core.REAL_DEFAULT);
  const optInKCPeriod = orDefault(o.kcPeriod, Core.INTEGER_DEFAULT);
  const optInFactorWide = orDefault(o.factorWide, Core.REAL_DEFAULT);
  const optInFactorNormal = orDefault(o.factorNormal, Core.REAL_DEFAULT);
  const optInFactorNarrow = orDefault(o.factorNarrow, Core.REAL_DEFAULT);
  const core = coreOf('SQZMOM openAndFill', o.core);
  const capacity = fillCapacity(sqzmomLookbackPositional(optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, core), prices.high, prices.low, prices.close);
  const outMomentum = o.outMomentum === undefined ? new Float64Array(capacity) : o.outMomentum;
  const outSqueeze = o.outSqueeze === undefined ? new Int32Array(capacity) : o.outSqueeze;
  const stream = sqzmomOpenAndFillPositional(prices.high, prices.low, prices.close, optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, outMomentum, outSqueeze, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outMomentum: outMomentum.subarray(0, range.count), outSqueeze: outSqueeze.subarray(0, range.count) };
}
