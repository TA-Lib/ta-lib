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
 *  100726 KL,CC  Creation (#490).
 *  101126 MF,CC  Two scratch buffers; Senkou B is written in place (#490).
 */

import { badSink, clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { MedpriceStream, medprice, medpriceOpenAndFillInternal } from './medprice.ts';
import { MidpriceStream, midprice, midpriceOpenAndFillInternal } from './midprice.ts';

/**
 * Number of leading input bars {@link ichimoku} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param optInTenkanPeriod - Period of the conversion line (default 9; range 2 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInKijunPeriod - Period of the base line, and the forward shift of the
 *   two spans (default 26; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param optInSenkouBPeriod - Period of the second leading span (default 52; range
 *   2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function ichimokuLookback(optInTenkanPeriod: number, optInKijunPeriod: number, optInSenkouBPeriod: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((optInTenkanPeriod | 0) !== optInTenkanPeriod) {
    return -1;
  }
  if ((optInKijunPeriod | 0) !== optInKijunPeriod) {
    return -1;
  }
  if ((optInSenkouBPeriod | 0) !== optInSenkouBPeriod) {
    return -1;
  }
  if (optInTenkanPeriod === Core.INTEGER_DEFAULT) {
    optInTenkanPeriod = 9;
  } else if (optInTenkanPeriod < 2 || optInTenkanPeriod > 100000) {
    return -1;
  }
  if (optInKijunPeriod === Core.INTEGER_DEFAULT) {
    optInKijunPeriod = 26;
  } else if (optInKijunPeriod < 2 || optInKijunPeriod > 100000) {
    return -1;
  }
  if (optInSenkouBPeriod === Core.INTEGER_DEFAULT) {
    optInSenkouBPeriod = 52;
  } else if (optInSenkouBPeriod < 2 || optInSenkouBPeriod > 100000) {
    return -1;
  }
  let longest = 0;
  /* Nothing orders the three periods: any of them can be the longest. */
  longest = optInTenkanPeriod;
  if (optInKijunPeriod > longest) {
    longest = optInKijunPeriod;
  }
  if (optInSenkouBPeriod > longest) {
    longest = optInSenkouBPeriod;
  }
  return longest - 1;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link ichimoku}.
 *
 * The values are never shifted: this describes the drawing only.
 *
 * @param optInTenkanPeriod - Period of the conversion line (default 9; range 2 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInKijunPeriod - Period of the base line, and the forward shift of the
 *   two spans (default 26; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param optInSenkouBPeriod - Period of the second leading span (default 52; range
 *   2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function ichimokuDisplayShift(optInTenkanPeriod: number, optInKijunPeriod: number, optInSenkouBPeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (ichimokuLookback(optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, core) < 0) {
    return -2147483648;
  }
  if (optInTenkanPeriod === Core.INTEGER_DEFAULT) {
    optInTenkanPeriod = 9;
  } else if (optInTenkanPeriod < 2 || optInTenkanPeriod > 100000) {
    return -2147483648;
  }
  if (optInKijunPeriod === Core.INTEGER_DEFAULT) {
    optInKijunPeriod = 26;
  } else if (optInKijunPeriod < 2 || optInKijunPeriod > 100000) {
    return -2147483648;
  }
  if (optInSenkouBPeriod === Core.INTEGER_DEFAULT) {
    optInSenkouBPeriod = 52;
  } else if (optInSenkouBPeriod < 2 || optInSenkouBPeriod > 100000) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 4) {
    return -2147483648;
  }
  if (outputIdx === 0 || outputIdx === 1) {
    return 0;
  }
  /* Positive: the two spans are drawn ahead of the bar that computed them. */
  if (outputIdx === 2 || outputIdx === 3) {
    return optInKijunPeriod;
  }
  return 0;
}

/** @internal */
export function ichimokuImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  optInTenkanPeriod: number,
  optInKijunPeriod: number,
  optInSenkouBPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outTenkanSen: Float64Array,
  outKijunSen: Float64Array,
  outSenkouSpanA: Float64Array,
  outSenkouSpanB: Float64Array,
): RetCode {
  let lookbackTotal = 0;
  let n = 0;
  let i = 0;
  const tempBegIdx = new MInteger();
  const tempNbElement = new MInteger();
  let tempT!: Float64Array;
  let tempK!: Float64Array;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTenkanPeriod === Core.INTEGER_DEFAULT) {
    optInTenkanPeriod = 9;
  } else if (optInTenkanPeriod < 2 || optInTenkanPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInKijunPeriod === Core.INTEGER_DEFAULT) {
    optInKijunPeriod = 26;
  } else if (optInKijunPeriod < 2 || optInKijunPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSenkouBPeriod === Core.INTEGER_DEFAULT) {
    optInSenkouBPeriod = 52;
  } else if (optInSenkouBPeriod < 2 || optInSenkouBPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if ((outTenkanSen.buffer === outKijunSen.buffer && outTenkanSen.byteOffset === outKijunSen.byteOffset) || (outTenkanSen.buffer === outSenkouSpanA.buffer && outTenkanSen.byteOffset === outSenkouSpanA.byteOffset) || (outTenkanSen.buffer === outSenkouSpanB.buffer && outTenkanSen.byteOffset === outSenkouSpanB.byteOffset) || (outKijunSen.buffer === outSenkouSpanA.buffer && outKijunSen.byteOffset === outSenkouSpanA.byteOffset) || (outKijunSen.buffer === outSenkouSpanB.buffer && outKijunSen.byteOffset === outSenkouSpanB.byteOffset) || (outSenkouSpanA.buffer === outSenkouSpanB.buffer && outSenkouSpanA.byteOffset === outSenkouSpanB.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = ichimokuLookback(optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  n = endIdx - startIdx + 1;
  tempT = new Float64Array(n);
  tempK = new Float64Array(n);
  /* An output may alias inHigh or inLow: no output is written before the
   * last midprice call, which is the last read of either input.
   */
  const _xr0 = midprice(startIdx, endIdx, inHigh, inLow, optInTenkanPeriod, tempT, core);
  tempBegIdx.value = _xr0.begIdx;
  tempNbElement.value = _xr0.count;
  const _xr1 = midprice(startIdx, endIdx, inHigh, inLow, optInKijunPeriod, tempK, core);
  tempBegIdx.value = _xr1.begIdx;
  tempNbElement.value = _xr1.count;
  const _xr2 = midprice(startIdx, endIdx, inHigh, inLow, optInSenkouBPeriod, outSenkouSpanB, core);
  tempBegIdx.value = _xr2.begIdx;
  tempNbElement.value = _xr2.count;
  /* Span A halves the two rounded midpoints. (hh+ll+hh+ll)/4 is a different
   * double in the last bit.
   */
  const _xr3 = medprice(0, n - 1, tempT, tempK, outSenkouSpanA, core);
  tempBegIdx.value = _xr3.begIdx;
  tempNbElement.value = _xr3.count;
  for (i = 0; i < n; i += 1) {
    outTenkanSen[i] = tempT[i];
    outKijunSen[i] = tempK[i];
  }
  outBegIdx.value = startIdx;
  outNBElement.value = n;
  return RetCode.SUCCESS;
}

/**
 * Ichimoku Kinko Hyo, "one glance equilibrium chart": four lines built from highs
 * and lows alone. Goichi Hosoda's reading is that a market's balance is visible
 * without any smoothing — each line is the midpoint of a window, the mean of its
 * highest high and its lowest low, so it marks the level at which that stretch of
 * trading was evenly divided. The conversion line turns fastest, the base line is
 * the reference, and the two leading spans are drawn ahead of price, where the
 * band between them is read as support or resistance before it is reached.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ichimoku](https://ta-lib.org/functions/ichimoku).
 *
 * **Notes**
 * - Each line is `TA_MIDPRICE` over its own period, and Span A is `TA_MEDPRICE` of
 *   the Tenkan and Kijun lines: it halves two midpoints that are already rounded.
 *   Averaging the four extremes in one step is the same number in real arithmetic
 *   and can differ in the last bit.
 * - The two spans are drawn `kijunPeriod` bars ahead of the bar that computed
 *   them. That is a display shift: `TA_ICHIMOKU_DisplayShift` reports it, and it
 *   changes nothing about the values, the lookback or the returned range. Every
 *   output is written at the bar that computed it, so the last `kijunPeriod` span
 *   values are the cloud a chart draws past the last bar.
 * - Hosoda's 26 counts the current bar, which is a chart offset of 25.
 *   QuantConnect LEAN, pandas-ta and MetaTrader offset by 26, and that is what the
 *   display shift reports at the default periods; a caller who wants Hosoda's
 *   count draws the spans one bar earlier.
 * - The lookback is the longest of the three periods less one. Nothing orders the
 *   three, so any of them can be the longest.
 * - The Chikou span is the close drawn `kijunPeriod` bars back. It carries no
 *   computation and is not an output. Its usual signal, the close above the close
 *   `n` bars earlier, is `TA_MOM(close, n) > 0`.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link ichimokuLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price series.
 * @param inLow - Low price series.
 * @param optInTenkanPeriod - Period of the conversion line (default 9; range 2 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInKijunPeriod - Period of the base line, and the forward shift of the
 *   two spans (default 26; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param optInSenkouBPeriod - Period of the second leading span (default 52; range
 *   2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outTenkanSen - Conversion line. Must hold the values produced and never
 *   be empty; the input length always suffices.
 * @param outKijunSen - Base line. Must hold the values produced and never be
 *   empty; the input length always suffices.
 * @param outSenkouSpanA - First leading span, drawn ahead by the base period. Must
 *   hold the values produced and never be empty; the input length always suffices.
 * @param outSenkouSpanB - Second leading span, drawn ahead by the base period.
 *   Must hold the values produced and never be empty; the input length always
 *   suffices.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The range written: `begIdx` is the first bar with a value, `count` how
 *   many were written.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for an argument that is not an int32, a
 *   parameter outside its range, an array too short for the range, an empty output,
 *   two outputs sharing memory or an output sharing memory with a `Float64Array`
 *   input it is not. Checked before anything is written.
 *
 * @example
 * ```ts
 * import { ichimoku } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const tenkanSen = new Float64Array(high.length);
 * const kijunSen = new Float64Array(high.length);
 * const senkouSpanA = new Float64Array(high.length);
 * const senkouSpanB = new Float64Array(high.length);
 * const { begIdx, count } = ichimoku(
 *   0, high.length - 1,
 *   high, low,
 *   9, 26, 52,
 *   tenkanSen, kijunSen, senkouSpanA, senkouSpanB,
 * );
 * // tenkanSen[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`midprice`](https://ta-lib.org/functions/midprice)
 * @see [`medprice`](https://ta-lib.org/functions/medprice)
 * @see [`sar`](https://ta-lib.org/functions/sar)
 * @see [`bbands`](https://ta-lib.org/functions/bbands)
 */
export function ichimoku(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTenkanPeriod: number,
  optInKijunPeriod: number,
  optInSenkouBPeriod: number,
  outTenkanSen: Float64Array,
  outKijunSen: Float64Array,
  outSenkouSpanA: Float64Array,
  outSenkouSpanB: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('ICHIMOKU', 'startIdx', startIdx);
  endIdx = requireInt32('ICHIMOKU', 'endIdx', endIdx);
  optInTenkanPeriod = requireInt32('ICHIMOKU', 'optInTenkanPeriod', optInTenkanPeriod);
  optInKijunPeriod = requireInt32('ICHIMOKU', 'optInKijunPeriod', optInKijunPeriod);
  optInSenkouBPeriod = requireInt32('ICHIMOKU', 'optInSenkouBPeriod', optInSenkouBPeriod);
  requireIndexRange('ICHIMOKU', startIdx, endIdx);
  const guardStart = clampedStart('ICHIMOKU', startIdx, ichimokuLookback(optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('ICHIMOKU', 'inHigh', inHigh, guardInLen);
  requireLength('ICHIMOKU', 'inLow', inLow, guardInLen);
  requireLength('ICHIMOKU', 'outTenkanSen', outTenkanSen, guardOutLen);
  requireLength('ICHIMOKU', 'outKijunSen', outKijunSen, guardOutLen);
  requireLength('ICHIMOKU', 'outSenkouSpanA', outSenkouSpanA, guardOutLen);
  requireLength('ICHIMOKU', 'outSenkouSpanB', outSenkouSpanB, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlapsPartly(outTenkanSen, inHighF64) || overlapsPartly(outTenkanSen, inLowF64) || overlapsPartly(outKijunSen, inHighF64) || overlapsPartly(outKijunSen, inLowF64) || overlapsPartly(outSenkouSpanA, inHighF64) || overlapsPartly(outSenkouSpanA, inLowF64) || overlapsPartly(outSenkouSpanB, inHighF64) || overlapsPartly(outSenkouSpanB, inLowF64) || overlaps(outTenkanSen, outKijunSen) || overlaps(outTenkanSen, outSenkouSpanA) || overlaps(outTenkanSen, outSenkouSpanB) || overlaps(outKijunSen, outSenkouSpanA) || overlaps(outKijunSen, outSenkouSpanB) || overlaps(outSenkouSpanA, outSenkouSpanB)) {
    throw failure('ICHIMOKU', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = ichimokuImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, outBegIdx, outNBElement, outTenkanSen, outKijunSen, outSenkouSpanA, outSenkouSpanB);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('ICHIMOKU', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live ICHIMOKU stream: one value per closed bar, bit-identical to
 * {@link ichimoku} over the same series. Open it with {@link ichimokuOpen} or
 * {@link ichimokuOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/ichimoku](https://ta-lib.org/functions/ichimoku).
 */
export class IchimokuStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTenkanPeriod: number = 0;
  /** @internal */
  optInKijunPeriod: number = 0;
  /** @internal */
  optInSenkouBPeriod: number = 0;
  /** @internal */
  cur_outTenkanSen: number = 0;
  /** @internal */
  cur_outKijunSen: number = 0;
  /** @internal */
  cur_outSenkouSpanA: number = 0;
  /** @internal */
  cur_outSenkouSpanB: number = 0;
  /** @internal */
  sub0!: MidpriceStream;
  /** @internal */
  sub1!: MidpriceStream;
  /** @internal */
  sub2!: MidpriceStream;
  /** @internal */
  sub3!: MedpriceStream;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): IchimokuStream {
    return new IchimokuStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link ichimoku} reports them over the same bars. Each accepted `update` and
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
      throw failure('ICHIMOKU advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar, writing the new values into `out`.
   *
   * @param inHigh - The bar's `inHigh` value: High price series.
   * @param inLow - The bar's `inLow` value: Low price series.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number, out: IchimokuOut): void {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('ICHIMOKU update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (typeof out !== 'object' || out === null) {
      throw badSink('ICHIMOKU update', 'IchimokuOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('ICHIMOKU update', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    ichimokuStepImpl(this, inHigh, inLow);
    this.outRangeCount++;
    out.tenkanSen = this.cur_outTenkanSen;
    out.kijunSen = this.cur_outKijunSen;
    out.senkouSpanA = this.cur_outSenkouSpanA;
    out.senkouSpanB = this.cur_outSenkouSpanB;
  }

  /**
   * Writes into `out` what the next `update` with the same bar would write,
   * committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price series.
   * @param inLow - The bar's `inLow` value: Low price series.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number, out: IchimokuOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('ICHIMOKU peek', 'IchimokuOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('ICHIMOKU peek', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    const sp = this;
    let cur_tempT = 0;
    let cur_tempK = 0;
    let cur_outSenkouSpanB = 0;
    let cur_outSenkouSpanA = 0;
    let cur_outTenkanSen = 0;
    let cur_outKijunSen = 0;
    cur_tempT = sp.sub0.peek(inHigh, inLow);
    cur_tempK = sp.sub1.peek(inHigh, inLow);
    cur_outSenkouSpanB = sp.sub2.peek(inHigh, inLow);
    cur_outSenkouSpanA = sp.sub3.peek(cur_tempT, cur_tempK);
    cur_outTenkanSen = cur_tempT;
    cur_outKijunSen = cur_tempK;
    out.tenkanSen = cur_outTenkanSen;
    out.kijunSen = cur_outKijunSen;
    out.senkouSpanA = cur_outSenkouSpanA;
    out.senkouSpanB = cur_outSenkouSpanB;
  }

  /**
   * The value at the last bar this stream counted; `peek` does not change it.
   *
   * @param out - Receives the outputs.
   */
  value(out: IchimokuOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('ICHIMOKU value', 'IchimokuOut');
    }
    out.tenkanSen = this.cur_outTenkanSen;
    out.kijunSen = this.cur_outKijunSen;
    out.senkouSpanA = this.cur_outSenkouSpanA;
    out.senkouSpanB = this.cur_outSenkouSpanB;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): IchimokuStream {
    const c = new IchimokuStream(this.core);
    c.optInTenkanPeriod = this.optInTenkanPeriod;
    c.optInKijunPeriod = this.optInKijunPeriod;
    c.optInSenkouBPeriod = this.optInSenkouBPeriod;
    c.cur_outTenkanSen = this.cur_outTenkanSen;
    c.cur_outKijunSen = this.cur_outKijunSen;
    c.cur_outSenkouSpanA = this.cur_outSenkouSpanA;
    c.cur_outSenkouSpanB = this.cur_outSenkouSpanB;
    c.sub0 = this.sub0.clone();
    c.sub1 = this.sub1.clone();
    c.sub2 = this.sub2.clone();
    c.sub3 = this.sub3.clone();
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one ICHIMOKU bar, written into an object the caller owns and
 * reuses: valid until the next call that writes it.
 */
export class IchimokuOut {
  /** Conversion line. */
  tenkanSen: number = 0;
  /** Base line. */
  kijunSen: number = 0;
  /** First leading span, drawn ahead by the base period. */
  senkouSpanA: number = 0;
  /** Second leading span, drawn ahead by the base period. */
  senkouSpanB: number = 0;
}

function ichimokuStepImpl(sp: IchimokuStream, inHigh: number, inLow: number): void {
  let cur_tempT = 0;
  let cur_tempK = 0;
  let cur_outSenkouSpanB = 0;
  let cur_outSenkouSpanA = 0;
  let cur_outTenkanSen = 0;
  let cur_outKijunSen = 0;
  cur_tempT = sp.sub0.update(inHigh, inLow);
  cur_tempK = sp.sub1.update(inHigh, inLow);
  cur_outSenkouSpanB = sp.sub2.update(inHigh, inLow);
  cur_outSenkouSpanA = sp.sub3.update(cur_tempT, cur_tempK);
  cur_outTenkanSen = cur_tempT;
  cur_outKijunSen = cur_tempK;
  sp.cur_outTenkanSen = cur_outTenkanSen;
  sp.cur_outKijunSen = cur_outKijunSen;
  sp.cur_outSenkouSpanA = cur_outSenkouSpanA;
  sp.cur_outSenkouSpanB = cur_outSenkouSpanB;
}

function ichimokuOpenImpl(
  sp: IchimokuStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTenkanPeriod: number,
  optInKijunPeriod: number,
  optInSenkouBPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outTenkanSen: Float64Array,
  outKijunSen: Float64Array,
  outSenkouSpanA: Float64Array,
  outSenkouSpanB: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let lookbackTotal = 0;
  let n = 0;
  let i = 0;
  const tempBegIdx = new MInteger();
  const tempNbElement = new MInteger();
  let tempT!: Float64Array;
  let tempK!: Float64Array;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInTenkanPeriod === Core.INTEGER_DEFAULT) {
    optInTenkanPeriod = 9;
  } else if (optInTenkanPeriod < 2 || optInTenkanPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInKijunPeriod === Core.INTEGER_DEFAULT) {
    optInKijunPeriod = 26;
  } else if (optInKijunPeriod < 2 || optInKijunPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInSenkouBPeriod === Core.INTEGER_DEFAULT) {
    optInSenkouBPeriod = 52;
  } else if (optInSenkouBPeriod < 2 || optInSenkouBPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen < ichimokuLookback(optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const sc_outTenkanSen = outStride === 1 ? outTenkanSen : new Float64Array(historyLen);
  const sc_outKijunSen = outStride === 1 ? outKijunSen : new Float64Array(historyLen);
  const sc_outSenkouSpanA = outStride === 1 ? outSenkouSpanA : new Float64Array(historyLen);
  const sc_outSenkouSpanB = outStride === 1 ? outSenkouSpanB : new Float64Array(historyLen);
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = ichimokuLookback(optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  n = endIdx - startIdx + 1;
  tempT = new Float64Array(n);
  tempK = new Float64Array(n);
  /* An output may alias inHigh or inLow: no output is written before the
   * last midprice call, which is the last read of either input.
   */
  const sub0 = midpriceOpenAndFillInternal(core, inHigh, inLow, startIdx, optInTenkanPeriod, tempBegIdx, tempNbElement, tempT);
  const sub1 = midpriceOpenAndFillInternal(core, inHigh, inLow, startIdx, optInKijunPeriod, tempBegIdx, tempNbElement, tempK);
  const sub2 = midpriceOpenAndFillInternal(core, inHigh, inLow, startIdx, optInSenkouBPeriod, tempBegIdx, tempNbElement, sc_outSenkouSpanB);
  /* Span A halves the two rounded midpoints. (hh+ll+hh+ll)/4 is a different
   * double in the last bit.
   */
  const sub3 = medpriceOpenAndFillInternal(core, tempT.slice(0, (n - 1) + 1), tempK.slice(0, (n - 1) + 1), 0, tempBegIdx, tempNbElement, sc_outSenkouSpanA);
  for (i = 0; i < n; i += 1) {
    sc_outTenkanSen[i] = tempT[i];
    sc_outKijunSen[i] = tempK[i];
  }
  outBegIdx.value = startIdx;
  outNBElement.value = n;
  if (outNBElement.value < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  sp.optInTenkanPeriod = optInTenkanPeriod;
  sp.optInKijunPeriod = optInKijunPeriod;
  sp.optInSenkouBPeriod = optInSenkouBPeriod;
  sp.sub0 = sub0;
  sp.sub1 = sub1;
  sp.sub2 = sub2;
  sp.sub3 = sub3;
  sp.cur_outTenkanSen = sc_outTenkanSen[outNBElement.value - 1];
  sp.cur_outKijunSen = sc_outKijunSen[outNBElement.value - 1];
  sp.cur_outSenkouSpanA = sc_outSenkouSpanA[outNBElement.value - 1];
  sp.cur_outSenkouSpanB = sc_outSenkouSpanB[outNBElement.value - 1];
  return RetCode.SUCCESS;
}

/** @internal */
export function ichimokuOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTenkanPeriod: number,
  optInKijunPeriod: number,
  optInSenkouBPeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outTenkanSen: Float64Array,
  outKijunSen: Float64Array,
  outSenkouSpanA: Float64Array,
  outSenkouSpanB: Float64Array,
): IchimokuStream {
  const sp = IchimokuStream.of(core);
  const retCode = ichimokuOpenImpl(sp, inHigh, inLow, startIdx, optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, outBegIdx, outNBElement, outTenkanSen, outKijunSen, outSenkouSpanA, outSenkouSpanB, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ICHIMOKU openAndFill', inHigh.length, startIdx, ichimokuLookback(optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, core));
  }
  throw streamFailure('ICHIMOKU openAndFill', retCode);
}

/** @internal */
export function ichimokuOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTenkanPeriod: number,
  optInKijunPeriod: number,
  optInSenkouBPeriod: number,
): IchimokuStream {
  const sp = IchimokuStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outTenkanSen = new Float64Array(1);
  const sink_outKijunSen = new Float64Array(1);
  const sink_outSenkouSpanA = new Float64Array(1);
  const sink_outSenkouSpanB = new Float64Array(1);
  const retCode = ichimokuOpenImpl(sp, inHigh, inLow, startIdx, optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, outBegIdx, outNBElement, sink_outTenkanSen, sink_outKijunSen, sink_outSenkouSpanA, sink_outSenkouSpanB, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ICHIMOKU open', inHigh.length, startIdx, ichimokuLookback(optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, core));
  }
  throw streamFailure('ICHIMOKU open', retCode);
}

/**
 * Opens a live ICHIMOKU stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link ichimoku} there.
 *
 * @param inHigh - Warm-up history. High price series. Every history has the same
 *   length.
 * @param inLow - Warm-up history. Low price series. Every history has the same
 *   length.
 * @param optInTenkanPeriod - Period of the conversion line (default 9; range 2 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInKijunPeriod - Period of the base line, and the forward shift of the
 *   two spans (default 26; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param optInSenkouBPeriod - Period of the second leading span (default 52; range
 *   2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `ichimokuLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an
 *   empty one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { IchimokuOut, ichimokuOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = ichimokuOpen(high, low, 9, 26, 52);
 * const peeked = new IchimokuOut();
 * const updated = new IchimokuOut();
 * stream.peek(101.4, 99.1, peeked); // a forming bar: commits nothing
 * stream.update(101.4, 99.1, updated); // the bar closes
 * ```
 */
export function ichimokuOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTenkanPeriod: number,
  optInKijunPeriod: number,
  optInSenkouBPeriod: number,
  core: Core = Core.DEFAULT,
): IchimokuStream {
  optInTenkanPeriod = requireInt32('ICHIMOKU open', 'optInTenkanPeriod', optInTenkanPeriod);
  optInKijunPeriod = requireInt32('ICHIMOKU open', 'optInKijunPeriod', optInKijunPeriod);
  optInSenkouBPeriod = requireInt32('ICHIMOKU open', 'optInSenkouBPeriod', optInSenkouBPeriod);
  requireArgument('ICHIMOKU open', 'inHigh', inHigh);
  requireHistory('ICHIMOKU open', inHigh.length);
  requireArgument('ICHIMOKU open', 'inLow', inLow);
  requireHistoryLength('ICHIMOKU open', 'inLow', inLow.length, inHigh.length);
  return ichimokuOpenInternal(core, float64(inHigh), float64(inLow), 0, optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod);
}

/**
 * {@link ichimokuOpen} that also fills the outputs over the whole history in the
 * same pass, bit-identically to {@link ichimoku}. The range written is the
 * stream's `outRange()`.
 *
 * @param inHigh - Warm-up history. High price series. Every history has the same
 *   length.
 * @param inLow - Warm-up history. Low price series. Every history has the same
 *   length.
 * @param optInTenkanPeriod - Period of the conversion line (default 9; range 2 to
 *   100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInKijunPeriod - Period of the base line, and the forward shift of the
 *   two spans (default 26; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the
 *   default).
 * @param optInSenkouBPeriod - Period of the second leading span (default 52; range
 *   2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outTenkanSen - Conversion line. Must hold `historyLen - lookback` values
 *   and share no memory with an input or another output.
 * @param outKijunSen - Base line. Must hold `historyLen - lookback` values and
 *   share no memory with an input or another output.
 * @param outSenkouSpanA - First leading span, drawn ahead by the base period. Must
 *   hold `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param outSenkouSpanB - Second leading span, drawn ahead by the base period.
 *   Must hold `historyLen - lookback` values and share no memory with an input or
 *   another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `ichimokuLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an
 *   empty one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range, histories of different lengths or an output too
 *   short or starting where another output or a `Float64Array` input starts, or, on
 *   a history long enough to open, one sharing memory with another output or a
 *   `Float64Array` input.
 *
 * @example
 * ```ts
 * import { ichimokuOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const tenkanSen = new Float64Array(high.length);
 * const kijunSen = new Float64Array(high.length);
 * const senkouSpanA = new Float64Array(high.length);
 * const senkouSpanB = new Float64Array(high.length);
 * const stream = ichimokuOpenAndFill(
 *   high, low,
 *   9, 26, 52,
 *   tenkanSen, kijunSen, senkouSpanA, senkouSpanB,
 * );
 * const { begIdx, count } = stream.outRange();
 * // tenkanSen[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function ichimokuOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTenkanPeriod: number,
  optInKijunPeriod: number,
  optInSenkouBPeriod: number,
  outTenkanSen: Float64Array,
  outKijunSen: Float64Array,
  outSenkouSpanA: Float64Array,
  outSenkouSpanB: Float64Array,
  core: Core = Core.DEFAULT,
): IchimokuStream {
  optInTenkanPeriod = requireInt32('ICHIMOKU openAndFill', 'optInTenkanPeriod', optInTenkanPeriod);
  optInKijunPeriod = requireInt32('ICHIMOKU openAndFill', 'optInKijunPeriod', optInKijunPeriod);
  optInSenkouBPeriod = requireInt32('ICHIMOKU openAndFill', 'optInSenkouBPeriod', optInSenkouBPeriod);
  requireArgument('ICHIMOKU openAndFill', 'inHigh', inHigh);
  requireHistory('ICHIMOKU openAndFill', inHigh.length);
  requireArgument('ICHIMOKU openAndFill', 'inLow', inLow);
  const guardOutLen = openFillCount('ICHIMOKU openAndFill', inHigh.length, ichimokuLookback(optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, core));
  requireHistoryLength('ICHIMOKU openAndFill', 'inLow', inLow.length, inHigh.length);
  requireLength('ICHIMOKU openAndFill', 'outTenkanSen', outTenkanSen, guardOutLen);
  requireLength('ICHIMOKU openAndFill', 'outKijunSen', outKijunSen, guardOutLen);
  requireLength('ICHIMOKU openAndFill', 'outSenkouSpanA', outSenkouSpanA, guardOutLen);
  requireLength('ICHIMOKU openAndFill', 'outSenkouSpanB', outSenkouSpanB, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if ((outTenkanSen.buffer === inHighF64.buffer && outTenkanSen.byteOffset === inHighF64.byteOffset) || (outTenkanSen.buffer === inLowF64.buffer && outTenkanSen.byteOffset === inLowF64.byteOffset) || (outKijunSen.buffer === inHighF64.buffer && outKijunSen.byteOffset === inHighF64.byteOffset) || (outKijunSen.buffer === inLowF64.buffer && outKijunSen.byteOffset === inLowF64.byteOffset) || (outSenkouSpanA.buffer === inHighF64.buffer && outSenkouSpanA.byteOffset === inHighF64.byteOffset) || (outSenkouSpanA.buffer === inLowF64.buffer && outSenkouSpanA.byteOffset === inLowF64.byteOffset) || (outSenkouSpanB.buffer === inHighF64.buffer && outSenkouSpanB.byteOffset === inHighF64.byteOffset) || (outSenkouSpanB.buffer === inLowF64.buffer && outSenkouSpanB.byteOffset === inLowF64.byteOffset) || (outTenkanSen.buffer === outKijunSen.buffer && outTenkanSen.byteOffset === outKijunSen.byteOffset) || (outTenkanSen.buffer === outSenkouSpanA.buffer && outTenkanSen.byteOffset === outSenkouSpanA.byteOffset) || (outTenkanSen.buffer === outSenkouSpanB.buffer && outTenkanSen.byteOffset === outSenkouSpanB.byteOffset) || (outKijunSen.buffer === outSenkouSpanA.buffer && outKijunSen.byteOffset === outSenkouSpanA.byteOffset) || (outKijunSen.buffer === outSenkouSpanB.buffer && outKijunSen.byteOffset === outSenkouSpanB.byteOffset) || (outSenkouSpanA.buffer === outSenkouSpanB.buffer && outSenkouSpanA.byteOffset === outSenkouSpanB.byteOffset)) {
    throw streamFailure('ICHIMOKU openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outTenkanSen, inHighF64) || overlaps(outTenkanSen, inLowF64) || overlaps(outKijunSen, inHighF64) || overlaps(outKijunSen, inLowF64) || overlaps(outSenkouSpanA, inHighF64) || overlaps(outSenkouSpanA, inLowF64) || overlaps(outSenkouSpanB, inHighF64) || overlaps(outSenkouSpanB, inLowF64) || overlaps(outTenkanSen, outKijunSen) || overlaps(outTenkanSen, outSenkouSpanA) || overlaps(outTenkanSen, outSenkouSpanB) || overlaps(outKijunSen, outSenkouSpanA) || overlaps(outKijunSen, outSenkouSpanB) || overlaps(outSenkouSpanA, outSenkouSpanB))) {
    throw streamFailure('ICHIMOKU openAndFill', RetCode.BAD_PARAM);
  }
  return ichimokuOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, new MInteger(), new MInteger(), outTenkanSen, outKijunSen, outSenkouSpanA, outSenkouSpanB);
}
