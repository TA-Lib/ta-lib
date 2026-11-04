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
 *  083126 MF,CC  First version (issue #273).
 */

import { badSink, clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireReal, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { AtrStream, atr, atrLookback as atrLookback_, atrOpenAndFillInternal } from './atr.ts';
import { EmaStream, ema, emaLookback as emaLookback_, emaOpenAndFillInternal } from './ema.ts';
import { TyppriceStream, typprice, typpriceOpenAndFillInternal } from './typprice.ts';

/**
 * Number of leading input bars {@link kc} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Smoothing period of the typical price moving average
 *   (default 20; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInATRPeriod - Smoothing period of the Average True Range (default 10;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInNbDev - Multiplier applied to the Average True Range (default 2;
 *   `Core.REAL_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function kcLookback(optInTimePeriod: number, optInATRPeriod: number, optInNbDev: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if ((optInATRPeriod | 0) !== optInATRPeriod) {
    return -1;
  }
  if (typeof optInNbDev !== 'number') {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  if (optInATRPeriod === Core.INTEGER_DEFAULT) {
    optInATRPeriod = 10;
  } else if (optInATRPeriod < 1 || optInATRPeriod > 100000) {
    return -1;
  }
  if (optInNbDev === Core.REAL_DEFAULT) {
    optInNbDev = 2.0;
  } else if (!(optInNbDev >= Core.REAL_MIN && optInNbDev <= Core.REAL_MAX)) {
    return -1;
  }
  let emaLookback = 0;
  let atrLookback = 0;
  /* A band value needs BOTH the centre line and the ATR at the same bar, so the
   * first valid output is the later of the two lookbacks. Each term is exactly
   * the lookback of the function it comes from and is never restated here, which
   * is what makes KC inherit TA_FUNC_UNST_EMA and TA_FUNC_UNST_ATR from its two
   * callees. Reporting the honest max keeps outBegIdx == lookback (issue #99),
   * which streaming's Open depends on.
   */
  emaLookback = emaLookback_(optInTimePeriod, core);
  atrLookback = atrLookback_(optInATRPeriod, core);
  return (emaLookback > atrLookback) ? emaLookback : atrLookback;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link kc}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Smoothing period of the typical price moving average
 *   (default 20; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInATRPeriod - Smoothing period of the Average True Range (default 10;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInNbDev - Multiplier applied to the Average True Range (default 2;
 *   `Core.REAL_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function kcDisplayShift(optInTimePeriod: number, optInATRPeriod: number, optInNbDev: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (kcLookback(optInTimePeriod, optInATRPeriod, optInNbDev, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 3) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function kcImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  optInTimePeriod: number,
  optInATRPeriod: number,
  optInNbDev: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
): RetCode {
  let retCode: RetCode = RetCode.SUCCESS;
  let i = 0;
  let lookbackTotal = 0;
  let emaLookback = 0;
  let tpStartIdx = 0;
  const tempBegIdx = new MInteger();
  const tempNbElement = new MInteger();
  let tempReal = 0;
  let middle = 0;
  let tempTP!: Float64Array;
  let tempATR!: Float64Array;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInATRPeriod === Core.INTEGER_DEFAULT) {
    optInATRPeriod = 10;
  } else if (optInATRPeriod < 1 || optInATRPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInNbDev === Core.REAL_DEFAULT) {
    optInNbDev = 2.0;
  } else if (!(optInNbDev >= Core.REAL_MIN && optInNbDev <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if ((outRealUpperBand.buffer === outRealMiddleBand.buffer && outRealUpperBand.byteOffset === outRealMiddleBand.byteOffset) || (outRealUpperBand.buffer === outRealLowerBand.buffer && outRealUpperBand.byteOffset === outRealLowerBand.byteOffset) || (outRealMiddleBand.buffer === outRealLowerBand.buffer && outRealMiddleBand.byteOffset === outRealLowerBand.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  emaLookback = emaLookback_(optInTimePeriod, core);
  lookbackTotal = kcLookback(optInTimePeriod, optInATRPeriod, optInNbDev, core);
  /* Nothing to produce: the range ends before the lookback. Return before
   * touching anything, so that a caller-supplied input which stops short of
   * endIdx is never read past its end.
   */
  if (lookbackTotal > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Each leg is entered at its OWN lookback, so each is the shipped function
   * over this range and nothing here over-warms the shorter one: a caller who
   * wants the band converged sets TA_FUNC_UNST_ATR, exactly as they would when
   * calling TA_ATR directly. The typical price is a derived input, so it is
   * materialized only over the window the moving average reads.
   */
  tpStartIdx = startIdx - emaLookback;
  tempTP = new Float64Array(endIdx - tpStartIdx + 1);
  tempATR = new Float64Array(endIdx - startIdx + 1);
  const _xr0 = typprice(tpStartIdx, endIdx, inHigh, inLow, inClose, tempTP, core);
  tempBegIdx.value = _xr0.begIdx;
  tempNbElement.value = _xr0.count;
  retCode = RetCode.SUCCESS;
  /* The ATR consumes the price inputs before the moving average below writes
   * the middle band, which may be aliased onto one of them.
   */
  const _xr1 = atr(startIdx, endIdx, inHigh, inLow, inClose, optInATRPeriod, tempATR, core);
  tempBegIdx.value = _xr1.begIdx;
  tempNbElement.value = _xr1.count;
  retCode = RetCode.SUCCESS;
  /* tempTP is bar-tpStartIdx relative, so entering the moving average at its
   * own lookback puts its first output on startIdx, where the ATR's already is.
   */
  const _xr2 = ema(emaLookback, endIdx - tpStartIdx, tempTP, optInTimePeriod, outRealMiddleBand, core);
  outBegIdx.value = _xr2.begIdx;
  outNBElement.value = _xr2.count;
  retCode = RetCode.SUCCESS;
  if (outNBElement.value === 0) {
    outNBElement.value = 0;
    return retCode;
  }
  outBegIdx.value = startIdx;
  for (i = 0; i < outNBElement.value; i += 1) {
    middle = outRealMiddleBand[i];
    tempReal = tempATR[i] * optInNbDev;
    outRealUpperBand[i] = middle + tempReal;
    outRealLowerBand[i] = middle - tempReal;
  }
  return RetCode.SUCCESS;
}

/**
 * Keltner Channels: three overlap lines around price. The centre line is an
 * exponential moving average of the typical price; the outer bands sit a multiple
 * of the Average True Range above and below it. The band width tracks volatility,
 * so the channel widens in fast markets and narrows in quiet ones.
 *
 * Formula and more info at
 * [ta-lib.org/functions/kc](https://ta-lib.org/functions/kc).
 *
 * **Notes**
 * - Several incompatible indicators are published under the name "Keltner
 *   Channel", disagreeing by percent rather than by rounding. This is the
 *   typical-price centre line with a Wilder-smoothed Average True Range band, the
 *   form implemented by TTR and ta4j.
 * - Chester Keltner's 1960 original smooths the typical price with a simple moving
 *   average and takes the band from the plain daily range; the widely charted
 *   modern variant centres on the close instead. Expect a visible difference
 *   against a package plotting either.
 * - TTR ties the Average True Range period to the centre line's period. Here the
 *   two are independent, so the band width can be tuned separately.
 * - The centre line and the band are separate recursions, each with its own
 *   warm-up. They are entered at their own lookbacks, so a caller who wants either
 *   one converged sets that function's unstable period — `TA_FUNC_UNST_EMA` for
 *   the centre line, `TA_FUNC_UNST_ATR` for the band.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link kcLookback} is a success with `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param inClose - Close price of each bar.
 * @param optInTimePeriod - Smoothing period of the typical price moving average
 *   (default 20; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInATRPeriod - Smoothing period of the Average True Range (default 10;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInNbDev - Multiplier applied to the Average True Range (default 2;
 *   `Core.REAL_DEFAULT` selects the default).
 * @param outRealUpperBand - Centre line plus the scaled Average True Range. Must
 *   hold the values produced and never be empty; the input length always suffices.
 * @param outRealMiddleBand - Exponential moving average of the typical price. Must
 *   hold the values produced and never be empty; the input length always suffices.
 * @param outRealLowerBand - Centre line minus the scaled Average True Range. Must
 *   hold the values produced and never be empty; the input length always suffices.
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
 * import { kc } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const upperBand = new Float64Array(high.length);
 * const middleBand = new Float64Array(high.length);
 * const lowerBand = new Float64Array(high.length);
 * const { begIdx, count } = kc(
 *   0, high.length - 1,
 *   high, low, close,
 *   20, 10, 2,
 *   upperBand, middleBand, lowerBand,
 * );
 * // upperBand[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`atr`](https://ta-lib.org/functions/atr)
 * @see [`typprice`](https://ta-lib.org/functions/typprice)
 * @see [`bbands`](https://ta-lib.org/functions/bbands)
 * @see [`accbands`](https://ta-lib.org/functions/accbands)
 */
export function kc(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  optInATRPeriod: number,
  optInNbDev: number,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('KC', 'startIdx', startIdx);
  endIdx = requireInt32('KC', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('KC', 'optInTimePeriod', optInTimePeriod);
  optInATRPeriod = requireInt32('KC', 'optInATRPeriod', optInATRPeriod);
  optInNbDev = requireReal('KC', 'optInNbDev', optInNbDev);
  requireIndexRange('KC', startIdx, endIdx);
  const guardStart = clampedStart('KC', startIdx, kcLookback(optInTimePeriod, optInATRPeriod, optInNbDev, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('KC', 'inHigh', inHigh, guardInLen);
  requireLength('KC', 'inLow', inLow, guardInLen);
  requireLength('KC', 'inClose', inClose, guardInLen);
  requireLength('KC', 'outRealUpperBand', outRealUpperBand, guardOutLen);
  requireLength('KC', 'outRealMiddleBand', outRealMiddleBand, guardOutLen);
  requireLength('KC', 'outRealLowerBand', outRealLowerBand, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlapsPartly(outRealUpperBand, inHighF64) || overlapsPartly(outRealUpperBand, inLowF64) || overlapsPartly(outRealUpperBand, inCloseF64) || overlapsPartly(outRealMiddleBand, inHighF64) || overlapsPartly(outRealMiddleBand, inLowF64) || overlapsPartly(outRealMiddleBand, inCloseF64) || overlapsPartly(outRealLowerBand, inHighF64) || overlapsPartly(outRealLowerBand, inLowF64) || overlapsPartly(outRealLowerBand, inCloseF64) || overlaps(outRealUpperBand, outRealMiddleBand) || overlaps(outRealUpperBand, outRealLowerBand) || overlaps(outRealMiddleBand, outRealLowerBand)) {
    throw failure('KC', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = kcImpl(core, startIdx, endIdx, inHighF64, inLowF64, inCloseF64, optInTimePeriod, optInATRPeriod, optInNbDev, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('KC', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live KC stream: one value per closed bar, bit-identical to {@link kc} over the
 * same series. Open it with {@link kcOpen} or {@link kcOpenAndFill}; there is
 * nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/kc](https://ta-lib.org/functions/kc).
 */
export class KcStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  optInATRPeriod: number = 0;
  /** @internal */
  optInNbDev: number = 0;
  /** @internal */
  cur_outRealUpperBand: number = 0;
  /** @internal */
  cur_outRealMiddleBand: number = 0;
  /** @internal */
  cur_outRealLowerBand: number = 0;
  /** @internal */
  sub0!: TyppriceStream;
  /** @internal */
  sub1!: AtrStream;
  /** @internal */
  sub2!: EmaStream;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): KcStream {
    return new KcStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link kc} reports them over the same bars. Each accepted `update` and each
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
      throw failure('KC advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar, writing the new values into `out`.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number, inClose: number, out: KcOut): void {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('KC update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (typeof out !== 'object' || out === null) {
      throw badSink('KC update', 'KcOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('KC update', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    kcStepImpl(this, inHigh, inLow, inClose);
    this.outRangeCount++;
    out.realUpperBand = this.cur_outRealUpperBand;
    out.realMiddleBand = this.cur_outRealMiddleBand;
    out.realLowerBand = this.cur_outRealLowerBand;
  }

  /**
   * Writes into `out` what the next `update` with the same bar would write,
   * committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param out - Receives the bar's outputs.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number, inClose: number, out: KcOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('KC peek', 'KcOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow) || !Number.isFinite(inClose)) {
      throw nonFiniteBar('KC peek', !Number.isFinite(inHigh) ? 'inHigh' : !Number.isFinite(inLow) ? 'inLow' : 'inClose');
    }
    const sp = this;
    let middle = 0;
    let tempReal = 0;
    let cur_tempTP = 0;
    let cur_tempATR = 0;
    let cur_outRealMiddleBand = 0;
    let cur_outRealUpperBand = 0;
    let cur_outRealLowerBand = 0;
    cur_tempTP = sp.sub0.peek(inHigh, inLow, inClose);
    cur_tempATR = sp.sub1.peek(inHigh, inLow, inClose);
    cur_outRealMiddleBand = sp.sub2.peek(cur_tempTP);
    middle = cur_outRealMiddleBand;
    tempReal = cur_tempATR * sp.optInNbDev;
    cur_outRealUpperBand = middle + tempReal;
    cur_outRealLowerBand = middle - tempReal;
    out.realUpperBand = cur_outRealUpperBand;
    out.realMiddleBand = cur_outRealMiddleBand;
    out.realLowerBand = cur_outRealLowerBand;
  }

  /**
   * The value at the last bar this stream counted; `peek` does not change it.
   *
   * @param out - Receives the outputs.
   */
  value(out: KcOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('KC value', 'KcOut');
    }
    out.realUpperBand = this.cur_outRealUpperBand;
    out.realMiddleBand = this.cur_outRealMiddleBand;
    out.realLowerBand = this.cur_outRealLowerBand;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): KcStream {
    const c = new KcStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.optInATRPeriod = this.optInATRPeriod;
    c.optInNbDev = this.optInNbDev;
    c.cur_outRealUpperBand = this.cur_outRealUpperBand;
    c.cur_outRealMiddleBand = this.cur_outRealMiddleBand;
    c.cur_outRealLowerBand = this.cur_outRealLowerBand;
    c.sub0 = this.sub0.clone();
    c.sub1 = this.sub1.clone();
    c.sub2 = this.sub2.clone();
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one KC bar, written into an object the caller owns and reuses:
 * valid until the next call that writes it.
 */
export class KcOut {
  /** Centre line plus the scaled Average True Range. */
  realUpperBand: number = 0;
  /** Exponential moving average of the typical price. */
  realMiddleBand: number = 0;
  /** Centre line minus the scaled Average True Range. */
  realLowerBand: number = 0;
}

function kcStepImpl(sp: KcStream, inHigh: number, inLow: number, inClose: number): void {
  let middle = 0;
  let tempReal = 0;
  let cur_tempTP = 0;
  let cur_tempATR = 0;
  let cur_outRealMiddleBand = 0;
  let cur_outRealUpperBand = 0;
  let cur_outRealLowerBand = 0;
  cur_tempTP = sp.sub0.update(inHigh, inLow, inClose);
  cur_tempATR = sp.sub1.update(inHigh, inLow, inClose);
  cur_outRealMiddleBand = sp.sub2.update(cur_tempTP);
  middle = cur_outRealMiddleBand;
  tempReal = cur_tempATR * sp.optInNbDev;
  cur_outRealUpperBand = middle + tempReal;
  cur_outRealLowerBand = middle - tempReal;
  sp.cur_outRealUpperBand = cur_outRealUpperBand;
  sp.cur_outRealMiddleBand = cur_outRealMiddleBand;
  sp.cur_outRealLowerBand = cur_outRealLowerBand;
}

function kcOpenImpl(
  sp: KcStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInATRPeriod: number,
  optInNbDev: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let i = 0;
  let lookbackTotal = 0;
  let emaLookback = 0;
  let tpStartIdx = 0;
  const tempBegIdx = new MInteger();
  const tempNbElement = new MInteger();
  let tempReal = 0;
  let middle = 0;
  let tempTP!: Float64Array;
  let tempATR!: Float64Array;
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
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 20;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInATRPeriod === Core.INTEGER_DEFAULT) {
    optInATRPeriod = 10;
  } else if (optInATRPeriod < 1 || optInATRPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInNbDev === Core.REAL_DEFAULT) {
    optInNbDev = 2.0;
  } else if (!(optInNbDev >= Core.REAL_MIN && optInNbDev <= Core.REAL_MAX)) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen < kcLookback(optInTimePeriod, optInATRPeriod, optInNbDev, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const sc_outRealUpperBand = outStride === 1 ? outRealUpperBand : new Float64Array(historyLen);
  const sc_outRealMiddleBand = outStride === 1 ? outRealMiddleBand : new Float64Array(historyLen);
  const sc_outRealLowerBand = outStride === 1 ? outRealLowerBand : new Float64Array(historyLen);
  emaLookback = emaLookback_(optInTimePeriod, core);
  lookbackTotal = kcLookback(optInTimePeriod, optInATRPeriod, optInNbDev, core);
  /* Nothing to produce: the range ends before the lookback. Return before
   * touching anything, so that a caller-supplied input which stops short of
   * endIdx is never read past its end.
   */
  if (lookbackTotal > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Each leg is entered at its OWN lookback, so each is the shipped function
   * over this range and nothing here over-warms the shorter one: a caller who
   * wants the band converged sets TA_FUNC_UNST_ATR, exactly as they would when
   * calling TA_ATR directly. The typical price is a derived input, so it is
   * materialized only over the window the moving average reads.
   */
  tpStartIdx = startIdx - emaLookback;
  tempTP = new Float64Array(endIdx - tpStartIdx + 1);
  tempATR = new Float64Array(endIdx - startIdx + 1);
  const sub0 = typpriceOpenAndFillInternal(core, inHigh, inLow, inClose, tpStartIdx, tempBegIdx, tempNbElement, tempTP);
  /* The ATR consumes the price inputs before the moving average below writes
   * the middle band, which may be aliased onto one of them.
   */
  const sub1 = atrOpenAndFillInternal(core, inHigh, inLow, inClose, startIdx, optInATRPeriod, tempBegIdx, tempNbElement, tempATR);
  /* tempTP is bar-tpStartIdx relative, so entering the moving average at its
   * own lookback puts its first output on startIdx, where the ATR's already is.
   */
  const sub2 = emaOpenAndFillInternal(core, tempTP.slice(0, (endIdx - tpStartIdx) + 1), emaLookback, optInTimePeriod, outBegIdx, outNBElement, sc_outRealMiddleBand);
  if (outNBElement.value === 0) {
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outBegIdx.value = startIdx;
  for (i = 0; i < outNBElement.value; i += 1) {
    middle = sc_outRealMiddleBand[i];
    tempReal = tempATR[i] * optInNbDev;
    sc_outRealUpperBand[i] = middle + tempReal;
    sc_outRealLowerBand[i] = middle - tempReal;
  }
  if (outNBElement.value < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.optInATRPeriod = optInATRPeriod;
  sp.optInNbDev = optInNbDev;
  sp.sub0 = sub0;
  sp.sub1 = sub1;
  sp.sub2 = sub2;
  sp.cur_outRealUpperBand = sc_outRealUpperBand[outNBElement.value - 1];
  sp.cur_outRealMiddleBand = sc_outRealMiddleBand[outNBElement.value - 1];
  sp.cur_outRealLowerBand = sc_outRealLowerBand[outNBElement.value - 1];
  return RetCode.SUCCESS;
}

/** @internal */
export function kcOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInATRPeriod: number,
  optInNbDev: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
): KcStream {
  const sp = KcStream.of(core);
  const retCode = kcOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, optInATRPeriod, optInNbDev, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('KC openAndFill', inHigh.length, startIdx, kcLookback(optInTimePeriod, optInATRPeriod, optInNbDev, core));
  }
  throw streamFailure('KC openAndFill', retCode);
}

/** @internal */
export function kcOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  optInATRPeriod: number,
  optInNbDev: number,
): KcStream {
  const sp = KcStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outRealUpperBand = new Float64Array(1);
  const sink_outRealMiddleBand = new Float64Array(1);
  const sink_outRealLowerBand = new Float64Array(1);
  const retCode = kcOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, optInATRPeriod, optInNbDev, outBegIdx, outNBElement, sink_outRealUpperBand, sink_outRealMiddleBand, sink_outRealLowerBand, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('KC open', inHigh.length, startIdx, kcLookback(optInTimePeriod, optInATRPeriod, optInNbDev, core));
  }
  throw streamFailure('KC open', retCode);
}

/**
 * Opens a live KC stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link kc} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Smoothing period of the typical price moving average
 *   (default 20; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInATRPeriod - Smoothing period of the Average True Range (default 10;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInNbDev - Multiplier applied to the Average True Range (default 2;
 *   `Core.REAL_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `kcLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty one,
 *   and `TA_BAD_PARAM` for an integer parameter that is not an int32, a real
 *   parameter that is not a number, a parameter outside its range or histories of
 *   different lengths.
 *
 * @example
 * ```ts
 * import { KcOut, kcOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = kcOpen(high, low, close, 20, 10, 2);
 * const peeked = new KcOut();
 * const updated = new KcOut();
 * stream.peek(101.4, 99.1, 100.9, peeked); // a forming bar: commits nothing
 * stream.update(101.4, 99.1, 100.9, updated); // the bar closes
 * ```
 */
export function kcOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  optInATRPeriod: number,
  optInNbDev: number,
  core: Core = Core.DEFAULT,
): KcStream {
  optInTimePeriod = requireInt32('KC open', 'optInTimePeriod', optInTimePeriod);
  optInATRPeriod = requireInt32('KC open', 'optInATRPeriod', optInATRPeriod);
  optInNbDev = requireReal('KC open', 'optInNbDev', optInNbDev);
  requireArgument('KC open', 'inHigh', inHigh);
  requireHistory('KC open', inHigh.length);
  requireArgument('KC open', 'inLow', inLow);
  requireArgument('KC open', 'inClose', inClose);
  requireHistoryLength('KC open', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('KC open', 'inClose', inClose.length, inHigh.length);
  return kcOpenInternal(core, float64(inHigh), float64(inLow), float64(inClose), 0, optInTimePeriod, optInATRPeriod, optInNbDev);
}

/**
 * {@link kcOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link kc}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Smoothing period of the typical price moving average
 *   (default 20; range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInATRPeriod - Smoothing period of the Average True Range (default 10;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInNbDev - Multiplier applied to the Average True Range (default 2;
 *   `Core.REAL_DEFAULT` selects the default).
 * @param outRealUpperBand - Centre line plus the scaled Average True Range. Must
 *   hold `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param outRealMiddleBand - Exponential moving average of the typical price. Must
 *   hold `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param outRealLowerBand - Centre line minus the scaled Average True Range. Must
 *   hold `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `kcLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty one,
 *   and `TA_BAD_PARAM` for an integer parameter that is not an int32, a real
 *   parameter that is not a number, a parameter outside its range, histories of
 *   different lengths or an output too short or starting where another output or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { kcOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const upperBand = new Float64Array(high.length);
 * const middleBand = new Float64Array(high.length);
 * const lowerBand = new Float64Array(high.length);
 * const stream = kcOpenAndFill(
 *   high, low, close,
 *   20, 10, 2,
 *   upperBand, middleBand, lowerBand,
 * );
 * const { begIdx, count } = stream.outRange();
 * // upperBand[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function kcOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  optInTimePeriod: number,
  optInATRPeriod: number,
  optInNbDev: number,
  outRealUpperBand: Float64Array,
  outRealMiddleBand: Float64Array,
  outRealLowerBand: Float64Array,
  core: Core = Core.DEFAULT,
): KcStream {
  optInTimePeriod = requireInt32('KC openAndFill', 'optInTimePeriod', optInTimePeriod);
  optInATRPeriod = requireInt32('KC openAndFill', 'optInATRPeriod', optInATRPeriod);
  optInNbDev = requireReal('KC openAndFill', 'optInNbDev', optInNbDev);
  requireArgument('KC openAndFill', 'inHigh', inHigh);
  requireHistory('KC openAndFill', inHigh.length);
  requireArgument('KC openAndFill', 'inLow', inLow);
  requireArgument('KC openAndFill', 'inClose', inClose);
  const guardOutLen = openFillCount('KC openAndFill', inHigh.length, kcLookback(optInTimePeriod, optInATRPeriod, optInNbDev, core));
  requireHistoryLength('KC openAndFill', 'inLow', inLow.length, inHigh.length);
  requireHistoryLength('KC openAndFill', 'inClose', inClose.length, inHigh.length);
  requireLength('KC openAndFill', 'outRealUpperBand', outRealUpperBand, guardOutLen);
  requireLength('KC openAndFill', 'outRealMiddleBand', outRealMiddleBand, guardOutLen);
  requireLength('KC openAndFill', 'outRealLowerBand', outRealLowerBand, guardOutLen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if ((outRealUpperBand.buffer === inHighF64.buffer && outRealUpperBand.byteOffset === inHighF64.byteOffset) || (outRealUpperBand.buffer === inLowF64.buffer && outRealUpperBand.byteOffset === inLowF64.byteOffset) || (outRealUpperBand.buffer === inCloseF64.buffer && outRealUpperBand.byteOffset === inCloseF64.byteOffset) || (outRealMiddleBand.buffer === inHighF64.buffer && outRealMiddleBand.byteOffset === inHighF64.byteOffset) || (outRealMiddleBand.buffer === inLowF64.buffer && outRealMiddleBand.byteOffset === inLowF64.byteOffset) || (outRealMiddleBand.buffer === inCloseF64.buffer && outRealMiddleBand.byteOffset === inCloseF64.byteOffset) || (outRealLowerBand.buffer === inHighF64.buffer && outRealLowerBand.byteOffset === inHighF64.byteOffset) || (outRealLowerBand.buffer === inLowF64.buffer && outRealLowerBand.byteOffset === inLowF64.byteOffset) || (outRealLowerBand.buffer === inCloseF64.buffer && outRealLowerBand.byteOffset === inCloseF64.byteOffset) || (outRealUpperBand.buffer === outRealMiddleBand.buffer && outRealUpperBand.byteOffset === outRealMiddleBand.byteOffset) || (outRealUpperBand.buffer === outRealLowerBand.buffer && outRealUpperBand.byteOffset === outRealLowerBand.byteOffset) || (outRealMiddleBand.buffer === outRealLowerBand.buffer && outRealMiddleBand.byteOffset === outRealLowerBand.byteOffset)) {
    throw streamFailure('KC openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outRealUpperBand, inHighF64) || overlaps(outRealUpperBand, inLowF64) || overlaps(outRealUpperBand, inCloseF64) || overlaps(outRealMiddleBand, inHighF64) || overlaps(outRealMiddleBand, inLowF64) || overlaps(outRealMiddleBand, inCloseF64) || overlaps(outRealLowerBand, inHighF64) || overlaps(outRealLowerBand, inLowF64) || overlaps(outRealLowerBand, inCloseF64) || overlaps(outRealUpperBand, outRealMiddleBand) || overlaps(outRealUpperBand, outRealLowerBand) || overlaps(outRealMiddleBand, outRealLowerBand))) {
    throw streamFailure('KC openAndFill', RetCode.BAD_PARAM);
  }
  return kcOpenAndFillInternal(core, inHighF64, inLowF64, inCloseF64, 0, optInTimePeriod, optInATRPeriod, optInNbDev, new MInteger(), new MInteger(), outRealUpperBand, outRealMiddleBand, outRealLowerBand);
}
