/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AC       Angelo Ciceri
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  102404 AC     Creation
 *  040309 AC     Increased flexibility to allow real bodies matching
 *                on one end (Greg Morris - "Candlestick charting explained")
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 *  103126 MF,CC  Join the long and doji tests into the one pre-check
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { CandleSettingType, Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function cdlharamicrossLookbackPositional(core: Core = Core.DEFAULT): number {
  const BodyDoji_avgPeriod = core.candleSettings[CandleSettingType.BODY_DOJI].avgPeriod;
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  return (BodyDoji_avgPeriod > BodyLong_avgPeriod ? BodyDoji_avgPeriod : BodyLong_avgPeriod) + 1;
}

/** @internal */
export function cdlharamicrossDisplayShiftPositional(outputIdx: number, _core: Core = Core.DEFAULT): number {
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cdlharamicrossImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): RetCode {
  let BodyDojiPeriodTotal = 0;
  let BodyLongPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let BodyDojiTrailingIdx = 0;
  let BodyLongTrailingIdx = 0;
  let lookbackTotal = 0;
  const BodyDoji_rangeType: number = core.candleSettings[CandleSettingType.BODY_DOJI].rangeType;
  const BodyDoji_avgPeriod = core.candleSettings[CandleSettingType.BODY_DOJI].avgPeriod;
  const BodyDoji_factor = core.candleSettings[CandleSettingType.BODY_DOJI].factor;
  const BodyLong_rangeType: number = core.candleSettings[CandleSettingType.BODY_LONG].rangeType;
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  const BodyLong_factor = core.candleSettings[CandleSettingType.BODY_LONG].factor;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlharamicrossLookbackPositional(core);
  /* Move up the start index if there is not
   * enough initial data.
   */
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  /* Do the calculation using tight loops. */
  /* Add-up the initial period, except for the last value. */
  BodyLongPeriodTotal = 0;
  BodyDojiPeriodTotal = 0;
  BodyLongTrailingIdx = startIdx - 1 - BodyLong_avgPeriod;
  BodyDojiTrailingIdx = startIdx - BodyDoji_avgPeriod;
  i = BodyLongTrailingIdx;
  while (i < startIdx - 1) {
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = BodyDojiTrailingIdx;
  while (i < startIdx) {
    BodyDojiPeriodTotal += ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first candle: long white (black) real body
   * - second candle: doji totally engulfed by the first
   * The meaning of "doji" and "long" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (Math.abs(inClose[i - 1] - inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0))))) /* 1st: long */
    {
      if (Math.abs(inClose[i] - inOpen[i]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod !== 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyDoji_rangeType === 2) ? 2.0 : 1.0))))) /* 2nd: doji */
      {
        if ((inClose[i] > inOpen[i] ? inClose[i] : inOpen[i]) < (inClose[i - 1] > inOpen[i - 1] ? inClose[i - 1] : inOpen[i - 1]) && /* 2nd is engulfed by 1st */
            (inClose[i] < inOpen[i] ? inClose[i] : inOpen[i]) > (inClose[i - 1] < inOpen[i - 1] ? inClose[i - 1] : inOpen[i - 1]))
        {
          outInteger[outIdx++] = (((-((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) | 0) * 100) | 0);
        } else if ((inClose[i] > inOpen[i] ? inClose[i] : inOpen[i]) <= (inClose[i - 1] > inOpen[i - 1] ? inClose[i - 1] : inOpen[i - 1]) && /* 2nd is engulfed by 1st */
            (inClose[i] < inOpen[i] ? inClose[i] : inOpen[i]) >= (inClose[i - 1] < inOpen[i - 1] ? inClose[i - 1] : inOpen[i - 1])) /* (one end of real body can match; engulfing guaranteed by "long" and "doji") */
        {
          outInteger[outIdx++] = (((-((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) | 0) * 80) | 0);
        } else {
          outInteger[outIdx++] = 0;
        }
      } else {
        outInteger[outIdx++] = 0;
      }
    } else {
      outInteger[outIdx++] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyLongTrailingIdx] - (((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inClose[BodyLongTrailingIdx]) : (inOpen[BodyLongTrailingIdx]))) + ((((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inOpen[BodyLongTrailingIdx]) : (inClose[BodyLongTrailingIdx])) - inLow[BodyLongTrailingIdx])) : 0.0)));
    BodyDojiPeriodTotal += ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : ((BodyDoji_rangeType === 1) ? (inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) : ((BodyDoji_rangeType === 2) ? ((inHigh[BodyDojiTrailingIdx] - (((inClose[BodyDojiTrailingIdx]) >= (inOpen[BodyDojiTrailingIdx])) ? (inClose[BodyDojiTrailingIdx]) : (inOpen[BodyDojiTrailingIdx]))) + ((((inClose[BodyDojiTrailingIdx]) >= (inOpen[BodyDojiTrailingIdx])) ? (inOpen[BodyDojiTrailingIdx]) : (inClose[BodyDojiTrailingIdx])) - inLow[BodyDojiTrailingIdx])) : 0.0)));
    i += 1;
    BodyLongTrailingIdx += 1;
    BodyDojiTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlharamicrossPositional(
  startIdx: number,
  endIdx: number,
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CDLHARAMICROSS', 'startIdx', startIdx);
  endIdx = requireInt32('CDLHARAMICROSS', 'endIdx', endIdx);
  requireIndexRange('CDLHARAMICROSS', startIdx, endIdx);
  const guardStart = clampedStart('CDLHARAMICROSS', startIdx, cdlharamicrossLookbackPositional(core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CDLHARAMICROSS', 'prices.open', inOpen, guardInLen);
  requireLength('CDLHARAMICROSS', 'prices.high', inHigh, guardInLen);
  requireLength('CDLHARAMICROSS', 'prices.low', inLow, guardInLen);
  requireLength('CDLHARAMICROSS', 'prices.close', inClose, guardInLen);
  requireOutput('CDLHARAMICROSS', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLHARAMICROSS', 'outInteger', 'prices.open');
  }
  if (overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLHARAMICROSS', 'outInteger', 'prices.high');
  }
  if (overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLHARAMICROSS', 'outInteger', 'prices.low');
  }
  if (overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLHARAMICROSS', 'outInteger', 'prices.close');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cdlharamicrossImpl(core, startIdx, endIdx, inOpenF64, inHighF64, inLowF64, inCloseF64, outBegIdx, outNBElement, outInteger);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CDLHARAMICROSS', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cdlharamicross}, {@link cdlharamicrossLookback},
 * {@link cdlharamicrossDisplayShift}, {@link cdlharamicrossOpen} and
 * {@link cdlharamicrossOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface CdlharamicrossOptions {
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
   * buffer or with an input.
   */
  outInteger?: Int32Array<ArrayBuffer> | undefined;
}

/**
 * What {@link cdlharamicross} returns, and {@link cdlharamicrossOpenAndFill} with
 * its stream.
 */
export interface CdlharamicrossResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * +100/+80 when the first candle is black (bullish), -100/-80 when the first
   * candle is white (bearish), 0 otherwise. Magnitude 100 for strict containment
   * inside the first body, 80 when one or both real-body ends match. `values[k]` is
   * the value at input bar `begIdx + k`.
   */
  values: Int32Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link cdlharamicross} consumes before its first
 * value: the index of the first bar with a value when the whole series is
 * requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlharamicrossOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlharamicrossLookback(options?: CdlharamicrossOptions): number {
  const o = optionsOf('CDLHARAMICROSS lookback', options);
  return cdlharamicrossLookbackPositional(coreOf('CDLHARAMICROSS lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cdlharamicross}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlharamicrossOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cdlharamicrossDisplayShift(outputIdx: number, options?: CdlharamicrossOptions): number {
  const o = optionsOf('CDLHARAMICROSS displayShift', options);
  return cdlharamicrossDisplayShiftPositional(outputIdx, coreOf('CDLHARAMICROSS displayShift', o.core));
}

/**
 * A two-candle reversal pattern: a long real body followed by a doji whose real
 * body is contained within the first candle's real body (the doji variant of the
 * Harami). Bullish after a black first candle, bearish after a white first candle.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlharamicross](https://ta-lib.org/functions/cdlharamicross).
 *
 * **Notes**
 * - Does not verify the prior trend (downtrend for bullish, uptrend for bearish)
 *   that the reversal signal assumes.
 * - Bulkowski's testing found the bearish Harami Cross behaves opposite its
 *   textbook label even more strongly than the plain Harami: it acts as a bullish
 *   CONTINUATION 57% of the time rather than a bearish reversal, and the bullish
 *   Harami Cross likewise fails to reverse the downtrend 55% of the time.
 *   ([thepatternsite.com](https://thepatternsite.com/HaramiCrossBear.html))
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cdlharamicrossLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component this function reads, all of one
 *   length; other components may be present and are ignored. `open`: Open price of
 *   each bar. `high`: High price of each bar. `low`: Low price of each bar. `close`:
 *   Close price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link CdlharamicrossOptions}.
 * @returns `begIdx` and the values; see {@link CdlharamicrossResult}. Each output
 *   is a view of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, an input too short for the range, a buffer that is empty, too short or of
 *   the wrong type or a buffer sharing memory with a `Float64Array` input it is not.
 *   Checked before anything is written.
 *
 * @example
 * ```ts
 * import { cdlharamicross } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { begIdx, values } = cdlharamicross({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`cdlharami`](https://ta-lib.org/functions/cdlharami)
 * @see [`cdldoji`](https://ta-lib.org/functions/cdldoji)
 */
export function cdlharamicross(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlharamicrossOptions): CdlharamicrossResult {
  requireArgument('CDLHARAMICROSS', 'prices', prices);
  const o = optionsOf('CDLHARAMICROSS', options);
  if (o.endIdx === undefined) {
    requireSeries('CDLHARAMICROSS', 'prices.open', prices.open);
    requireSeries('CDLHARAMICROSS', 'prices.high', prices.high);
    requireSeries('CDLHARAMICROSS', 'prices.low', prices.low);
    requireSeries('CDLHARAMICROSS', 'prices.close', prices.close);
  }
  const core = coreOf('CDLHARAMICROSS', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.open.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cdlharamicrossLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const range = cdlharamicrossPositional(startIdx, endIdx, prices.open, prices.high, prices.low, prices.close, outInteger, core);
  return { begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CDLHARAMICROSS stream: one value per closed bar, bit-identical to
 * {@link cdlharamicross} over the same series. Open it with
 * {@link cdlharamicrossOpen} or {@link cdlharamicrossOpenAndFill}; there is
 * nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cdlharamicross](https://ta-lib.org/functions/cdlharamicross).
 */
export class CdlharamicrossStream {
  /** @internal */
  core: Core;
  /** @internal */
  BodyDojiPeriodTotal: number = 0;
  /** @internal */
  BodyLongPeriodTotal: number = 0;
  /** @internal */
  lag1_inOpen: number = 0;
  /** @internal */
  lag1_inHigh: number = 0;
  /** @internal */
  lag1_inLow: number = 0;
  /** @internal */
  lag1_inClose: number = 0;
  /** @internal */
  ringPos_BodyDojiTrailingIdx: number = 0;
  /** @internal */
  ringCap_BodyDojiTrailingIdx: number = 0;
  /** @internal */
  ring_BodyDojiTrailingIdx_derived: Float64Array = new Float64Array(1);
  /** @internal */
  ringPos_BodyLongTrailingIdx: number = 0;
  /** @internal */
  ringCap_BodyLongTrailingIdx: number = 0;
  /** @internal */
  ring_BodyLongTrailingIdx_derived: Float64Array = new Float64Array(1);
  /** @internal */
  cs_BodyDoji_rangeType: number = 0;
  /** @internal */
  cs_BodyDoji_avgPeriod: number = 0;
  /** @internal */
  cs_BodyDoji_factor: number = 0;
  /** @internal */
  cs_BodyLong_rangeType: number = 0;
  /** @internal */
  cs_BodyLong_avgPeriod: number = 0;
  /** @internal */
  cs_BodyLong_factor: number = 0;
  /** @internal */
  cur_outInteger: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): CdlharamicrossStream {
    return new CdlharamicrossStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cdlharamicross} reports them over the same bars. Each accepted `update`
   * and each `advance` adds one to `count`.
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
      throw streamFailure('CDLHARAMICROSS advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param open - The bar's `open` value: Open price of each bar.
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(open: number, high: number, low: number, close: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('CDLHARAMICROSS update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(open) || !Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('CDLHARAMICROSS update', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    cdlharamicrossStepImpl(this, open, high, low, close);
    this.outRangeCount++;
    return this.cur_outInteger;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param open - The bar's `open` value: Open price of each bar.
   * @param high - The bar's `high` value: High price of each bar.
   * @param low - The bar's `low` value: Low price of each bar.
   * @param close - The bar's `close` value: Close price of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(open: number, high: number, low: number, close: number): number {
    if (!Number.isFinite(open) || !Number.isFinite(high) || !Number.isFinite(low) || !Number.isFinite(close)) {
      throw nonFiniteBar('CDLHARAMICROSS peek', !Number.isFinite(open) ? 'open' : !Number.isFinite(high) ? 'high' : !Number.isFinite(low) ? 'low' : 'close');
    }
    const sp = this;
    const inOpen = open;
    const inHigh = high;
    const inLow = low;
    const inClose = close;
    let cur_outInteger = 0;
    const BodyDoji_rangeType = sp.cs_BodyDoji_rangeType;
    const BodyDoji_avgPeriod = sp.cs_BodyDoji_avgPeriod;
    const BodyDoji_factor = sp.cs_BodyDoji_factor;
    const BodyLong_rangeType = sp.cs_BodyLong_rangeType;
    const BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
    const BodyLong_factor = sp.cs_BodyLong_factor;
    if (Math.abs(sp.lag1_inClose - sp.lag1_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0))))) /* 1st: long */
    {
      if (Math.abs(inClose - inOpen) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod !== 0) ? (sp.BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyDoji_rangeType === 1) ? (inHigh - inLow) : ((BodyDoji_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyDoji_rangeType === 2) ? 2.0 : 1.0))))) /* 2nd: doji */
      {
        if ((inClose > inOpen ? inClose : inOpen) < (sp.lag1_inClose > sp.lag1_inOpen ? sp.lag1_inClose : sp.lag1_inOpen) && /* 2nd is engulfed by 1st */
            (inClose < inOpen ? inClose : inOpen) > (sp.lag1_inClose < sp.lag1_inOpen ? sp.lag1_inClose : sp.lag1_inOpen))
        {
          cur_outInteger = (((-((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) | 0) * 100) | 0);
        } else if ((inClose > inOpen ? inClose : inOpen) <= (sp.lag1_inClose > sp.lag1_inOpen ? sp.lag1_inClose : sp.lag1_inOpen) && /* 2nd is engulfed by 1st */
            (inClose < inOpen ? inClose : inOpen) >= (sp.lag1_inClose < sp.lag1_inOpen ? sp.lag1_inClose : sp.lag1_inOpen)) /* (one end of real body can match; engulfing guaranteed by "long" and "doji") */
        {
          cur_outInteger = (((-((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) | 0) * 80) | 0);
        } else {
          cur_outInteger = 0;
        }
      } else {
        cur_outInteger = 0;
      }
    } else {
      cur_outInteger = 0;
    }
    return cur_outInteger;
  }

  /** The value at the last bar this stream counted; `peek` does not change it. */
  value(): number {
    return this.cur_outInteger;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): CdlharamicrossStream {
    const c = new CdlharamicrossStream(this.core);
    c.BodyDojiPeriodTotal = this.BodyDojiPeriodTotal;
    c.BodyLongPeriodTotal = this.BodyLongPeriodTotal;
    c.lag1_inOpen = this.lag1_inOpen;
    c.lag1_inHigh = this.lag1_inHigh;
    c.lag1_inLow = this.lag1_inLow;
    c.lag1_inClose = this.lag1_inClose;
    c.ringPos_BodyDojiTrailingIdx = this.ringPos_BodyDojiTrailingIdx;
    c.ringCap_BodyDojiTrailingIdx = this.ringCap_BodyDojiTrailingIdx;
    c.ring_BodyDojiTrailingIdx_derived = this.ring_BodyDojiTrailingIdx_derived.slice();
    c.ringPos_BodyLongTrailingIdx = this.ringPos_BodyLongTrailingIdx;
    c.ringCap_BodyLongTrailingIdx = this.ringCap_BodyLongTrailingIdx;
    c.ring_BodyLongTrailingIdx_derived = this.ring_BodyLongTrailingIdx_derived.slice();
    c.cs_BodyDoji_rangeType = this.cs_BodyDoji_rangeType;
    c.cs_BodyDoji_avgPeriod = this.cs_BodyDoji_avgPeriod;
    c.cs_BodyDoji_factor = this.cs_BodyDoji_factor;
    c.cs_BodyLong_rangeType = this.cs_BodyLong_rangeType;
    c.cs_BodyLong_avgPeriod = this.cs_BodyLong_avgPeriod;
    c.cs_BodyLong_factor = this.cs_BodyLong_factor;
    c.cur_outInteger = this.cur_outInteger;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cdlharamicrossStepImpl(sp: CdlharamicrossStream, inOpen: number, inHigh: number, inLow: number, inClose: number): void {
  const BodyDoji_rangeType = sp.cs_BodyDoji_rangeType;
  const BodyDoji_avgPeriod = sp.cs_BodyDoji_avgPeriod;
  const BodyDoji_factor = sp.cs_BodyDoji_factor;
  const BodyLong_rangeType = sp.cs_BodyLong_rangeType;
  const BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
  const BodyLong_factor = sp.cs_BodyLong_factor;
  if (sp.ringCap_BodyDojiTrailingIdx === 0) {
    sp.ring_BodyDojiTrailingIdx_derived[0] = ((BodyDoji_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyDoji_rangeType === 1) ? (inHigh - inLow) : ((BodyDoji_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
  if (sp.ringCap_BodyLongTrailingIdx === 0) {
    sp.ring_BodyLongTrailingIdx_derived[0] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType === 1) ? (inHigh - inLow) : ((BodyLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  }
  if (Math.abs(sp.lag1_inClose - sp.lag1_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0))))) /* 1st: long */
  {
    if (Math.abs(inClose - inOpen) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod !== 0) ? (sp.BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyDoji_rangeType === 1) ? (inHigh - inLow) : ((BodyDoji_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyDoji_rangeType === 2) ? 2.0 : 1.0))))) /* 2nd: doji */
    {
      if ((inClose > inOpen ? inClose : inOpen) < (sp.lag1_inClose > sp.lag1_inOpen ? sp.lag1_inClose : sp.lag1_inOpen) && /* 2nd is engulfed by 1st */
          (inClose < inOpen ? inClose : inOpen) > (sp.lag1_inClose < sp.lag1_inOpen ? sp.lag1_inClose : sp.lag1_inOpen))
      {
        sp.cur_outInteger = (((-((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) | 0) * 100) | 0);
      } else if ((inClose > inOpen ? inClose : inOpen) <= (sp.lag1_inClose > sp.lag1_inOpen ? sp.lag1_inClose : sp.lag1_inOpen) && /* 2nd is engulfed by 1st */
          (inClose < inOpen ? inClose : inOpen) >= (sp.lag1_inClose < sp.lag1_inOpen ? sp.lag1_inClose : sp.lag1_inOpen)) /* (one end of real body can match; engulfing guaranteed by "long" and "doji") */
      {
        sp.cur_outInteger = (((-((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) | 0) * 80) | 0);
      } else {
        sp.cur_outInteger = 0;
      }
    } else {
      sp.cur_outInteger = 0;
    }
  } else {
    sp.cur_outInteger = 0;
  }
  /* add the current range and subtract the first range: this is done after the pattern recognition
   * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
   */
  sp.BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyLong_rangeType === 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyLong_rangeType === 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0))) - sp.ring_BodyLongTrailingIdx_derived[sp.ringPos_BodyLongTrailingIdx];
  sp.BodyDojiPeriodTotal += ((BodyDoji_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyDoji_rangeType === 1) ? (inHigh - inLow) : ((BodyDoji_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0))) - sp.ring_BodyDojiTrailingIdx_derived[sp.ringPos_BodyDojiTrailingIdx];
  sp.lag1_inOpen = inOpen;
  sp.lag1_inHigh = inHigh;
  sp.lag1_inLow = inLow;
  sp.lag1_inClose = inClose;
  sp.ring_BodyDojiTrailingIdx_derived[sp.ringPos_BodyDojiTrailingIdx] = ((BodyDoji_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyDoji_rangeType === 1) ? (inHigh - inLow) : ((BodyDoji_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  sp.ringPos_BodyDojiTrailingIdx = sp.ringPos_BodyDojiTrailingIdx + 1;
  if (sp.ringPos_BodyDojiTrailingIdx >= sp.ringCap_BodyDojiTrailingIdx) {
    sp.ringPos_BodyDojiTrailingIdx = 0;
  }
  sp.ring_BodyLongTrailingIdx_derived[sp.ringPos_BodyLongTrailingIdx] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType === 1) ? (inHigh - inLow) : ((BodyLong_rangeType === 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
  sp.ringPos_BodyLongTrailingIdx = sp.ringPos_BodyLongTrailingIdx + 1;
  if (sp.ringPos_BodyLongTrailingIdx >= sp.ringCap_BodyLongTrailingIdx) {
    sp.ringPos_BodyLongTrailingIdx = 0;
  }
}

function cdlharamicrossOpenImpl(
  sp: CdlharamicrossStream,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let BodyDojiPeriodTotal = 0;
  let BodyLongPeriodTotal = 0;
  let i = 0;
  let outIdx = 0;
  let BodyDojiTrailingIdx = 0;
  let BodyLongTrailingIdx = 0;
  let lookbackTotal = 0;
  const historyLen = inOpen.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inHigh.length !== inOpen.length || inLow.length !== inOpen.length || inClose.length !== inOpen.length) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const BodyDoji_rangeType: number = core.candleSettings[CandleSettingType.BODY_DOJI].rangeType;
  const BodyDoji_avgPeriod = core.candleSettings[CandleSettingType.BODY_DOJI].avgPeriod;
  const BodyDoji_factor = core.candleSettings[CandleSettingType.BODY_DOJI].factor;
  const BodyLong_rangeType: number = core.candleSettings[CandleSettingType.BODY_LONG].rangeType;
  const BodyLong_avgPeriod = core.candleSettings[CandleSettingType.BODY_LONG].avgPeriod;
  const BodyLong_factor = core.candleSettings[CandleSettingType.BODY_LONG].factor;
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = cdlharamicrossLookbackPositional(core);
  /* Move up the start index if there is not
   * enough initial data.
   */
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Do the calculation using tight loops. */
  /* Add-up the initial period, except for the last value. */
  BodyLongPeriodTotal = 0;
  BodyDojiPeriodTotal = 0;
  BodyLongTrailingIdx = startIdx - 1 - BodyLong_avgPeriod;
  BodyDojiTrailingIdx = startIdx - BodyDoji_avgPeriod;
  i = BodyLongTrailingIdx;
  while (i < startIdx - 1) {
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = BodyDojiTrailingIdx;
  while (i < startIdx) {
    BodyDojiPeriodTotal += ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
    i += 1;
  }
  i = startIdx;
  /* Proceed with the calculation for the requested range.
   * Must have:
   * - first candle: long white (black) real body
   * - second candle: doji totally engulfed by the first
   * The meaning of "doji" and "long" is specified with TA_SetCandleSettings
   */
  outIdx = 0;
  do {
    if (Math.abs(inClose[i - 1] - inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod !== 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((BodyLong_rangeType === 2) ? 2.0 : 1.0))))) /* 1st: long */
    {
      if (Math.abs(inClose[i] - inOpen[i]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod !== 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyDoji_rangeType === 2) ? 2.0 : 1.0))))) /* 2nd: doji */
      {
        if ((inClose[i] > inOpen[i] ? inClose[i] : inOpen[i]) < (inClose[i - 1] > inOpen[i - 1] ? inClose[i - 1] : inOpen[i - 1]) && /* 2nd is engulfed by 1st */
            (inClose[i] < inOpen[i] ? inClose[i] : inOpen[i]) > (inClose[i - 1] < inOpen[i - 1] ? inClose[i - 1] : inOpen[i - 1]))
        {
          outInteger[((outIdx++ * outStride) | 0)] = (((-((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) | 0) * 100) | 0);
        } else if ((inClose[i] > inOpen[i] ? inClose[i] : inOpen[i]) <= (inClose[i - 1] > inOpen[i - 1] ? inClose[i - 1] : inOpen[i - 1]) && /* 2nd is engulfed by 1st */
            (inClose[i] < inOpen[i] ? inClose[i] : inOpen[i]) >= (inClose[i - 1] < inOpen[i - 1] ? inClose[i - 1] : inOpen[i - 1])) /* (one end of real body can match; engulfing guaranteed by "long" and "doji") */
        {
          outInteger[((outIdx++ * outStride) | 0)] = (((-((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) | 0) * 80) | 0);
        } else {
          outInteger[((outIdx++ * outStride) | 0)] = 0;
        }
      } else {
        outInteger[((outIdx++ * outStride) | 0)] = 0;
      }
    } else {
      outInteger[((outIdx++ * outStride) | 0)] = 0;
    }
    /* add the current range and subtract the first range: this is done after the pattern recognition
     * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
     */
    BodyLongPeriodTotal += ((BodyLong_rangeType === 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType === 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType === 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((BodyLong_rangeType === 0) ? (Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType === 1) ? (inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType === 2) ? ((inHigh[BodyLongTrailingIdx] - (((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inClose[BodyLongTrailingIdx]) : (inOpen[BodyLongTrailingIdx]))) + ((((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inOpen[BodyLongTrailingIdx]) : (inClose[BodyLongTrailingIdx])) - inLow[BodyLongTrailingIdx])) : 0.0)));
    BodyDojiPeriodTotal += ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType === 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType === 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : ((BodyDoji_rangeType === 1) ? (inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) : ((BodyDoji_rangeType === 2) ? ((inHigh[BodyDojiTrailingIdx] - (((inClose[BodyDojiTrailingIdx]) >= (inOpen[BodyDojiTrailingIdx])) ? (inClose[BodyDojiTrailingIdx]) : (inOpen[BodyDojiTrailingIdx]))) + ((((inClose[BodyDojiTrailingIdx]) >= (inOpen[BodyDojiTrailingIdx])) ? (inOpen[BodyDojiTrailingIdx]) : (inClose[BodyDojiTrailingIdx])) - inLow[BodyDojiTrailingIdx])) : 0.0)));
    i += 1;
    BodyLongTrailingIdx += 1;
    BodyDojiTrailingIdx += 1;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const cap_BodyDojiTrailingIdx = i - BodyDojiTrailingIdx;
  if (cap_BodyDojiTrailingIdx < 0 || cap_BodyDojiTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_BodyDojiTrailingIdx = cap_BodyDojiTrailingIdx > 0 ? cap_BodyDojiTrailingIdx : 1;
  const capRing_BodyDojiTrailingIdx_derived = new Float64Array(allocN_BodyDojiTrailingIdx);
  for (let fillJ = historyLen - cap_BodyDojiTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_BodyDojiTrailingIdx_derived[fillJ - (historyLen - cap_BodyDojiTrailingIdx)] = ((BodyDoji_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyDoji_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyDoji_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  const cap_BodyLongTrailingIdx = i - BodyLongTrailingIdx;
  if (cap_BodyLongTrailingIdx < 0 || cap_BodyLongTrailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_BodyLongTrailingIdx = cap_BodyLongTrailingIdx > 0 ? cap_BodyLongTrailingIdx : 1;
  const capRing_BodyLongTrailingIdx_derived = new Float64Array(allocN_BodyLongTrailingIdx);
  for (let fillJ = historyLen - cap_BodyLongTrailingIdx; fillJ < historyLen; fillJ++) {
    capRing_BodyLongTrailingIdx_derived[fillJ - (historyLen - cap_BodyLongTrailingIdx)] = ((BodyLong_rangeType === 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyLong_rangeType === 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyLong_rangeType === 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
  }
  sp.BodyDojiPeriodTotal = BodyDojiPeriodTotal;
  sp.BodyLongPeriodTotal = BodyLongPeriodTotal;
  sp.lag1_inOpen = inOpen[historyLen - 1];
  sp.lag1_inHigh = inHigh[historyLen - 1];
  sp.lag1_inLow = inLow[historyLen - 1];
  sp.lag1_inClose = inClose[historyLen - 1];
  sp.ringPos_BodyDojiTrailingIdx = 0;
  sp.ringCap_BodyDojiTrailingIdx = cap_BodyDojiTrailingIdx;
  sp.ring_BodyDojiTrailingIdx_derived = capRing_BodyDojiTrailingIdx_derived;
  sp.ringPos_BodyLongTrailingIdx = 0;
  sp.ringCap_BodyLongTrailingIdx = cap_BodyLongTrailingIdx;
  sp.ring_BodyLongTrailingIdx_derived = capRing_BodyLongTrailingIdx_derived;
  sp.cs_BodyDoji_rangeType = BodyDoji_rangeType;
  sp.cs_BodyDoji_avgPeriod = BodyDoji_avgPeriod;
  sp.cs_BodyDoji_factor = BodyDoji_factor;
  sp.cs_BodyLong_rangeType = BodyLong_rangeType;
  sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
  sp.cs_BodyLong_factor = BodyLong_factor;
  sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cdlharamicrossOpenAndFillInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outInteger: Int32Array,
): CdlharamicrossStream {
  const sp = CdlharamicrossStream.of(core);
  const retCode = cdlharamicrossOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLHARAMICROSS openAndFill', inOpen.length, startIdx, cdlharamicrossLookbackPositional(core));
  }
  throw streamFailure('CDLHARAMICROSS openAndFill', retCode);
}

/** @internal */
export function cdlharamicrossOpenInternal(
  core: Core,
  inOpen: Float64Array,
  inHigh: Float64Array,
  inLow: Float64Array,
  inClose: Float64Array,
  startIdx: number,
): CdlharamicrossStream {
  const sp = CdlharamicrossStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outInteger = new Int32Array(1);
  const retCode = cdlharamicrossOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CDLHARAMICROSS open', inOpen.length, startIdx, cdlharamicrossLookbackPositional(core));
  }
  throw streamFailure('CDLHARAMICROSS open', retCode);
}

/** @internal */
export function cdlharamicrossOpenPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  core: Core = Core.DEFAULT,
): CdlharamicrossStream {
  requireArgument('CDLHARAMICROSS open', 'prices.open', inOpen);
  requireHistory('CDLHARAMICROSS open', 'prices.open', inOpen.length);
  requireArgument('CDLHARAMICROSS open', 'prices.high', inHigh);
  requireArgument('CDLHARAMICROSS open', 'prices.low', inLow);
  requireArgument('CDLHARAMICROSS open', 'prices.close', inClose);
  requireHistoryLength('CDLHARAMICROSS open', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLHARAMICROSS open', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLHARAMICROSS open', 'prices.close', inClose.length, inOpen.length);
  return cdlharamicrossOpenInternal(core, float64(inOpen), float64(inHigh), float64(inLow), float64(inClose), 0);
}

/** @internal */
export function cdlharamicrossOpenAndFillPositional(
  inOpen: ArrayLike<number>,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  inClose: ArrayLike<number>,
  outInteger: Int32Array,
  core: Core = Core.DEFAULT,
): CdlharamicrossStream {
  requireArgument('CDLHARAMICROSS openAndFill', 'prices.open', inOpen);
  requireHistory('CDLHARAMICROSS openAndFill', 'prices.open', inOpen.length);
  requireArgument('CDLHARAMICROSS openAndFill', 'prices.high', inHigh);
  requireArgument('CDLHARAMICROSS openAndFill', 'prices.low', inLow);
  requireArgument('CDLHARAMICROSS openAndFill', 'prices.close', inClose);
  const guardOutLen = openFillCount('CDLHARAMICROSS openAndFill', inOpen.length, cdlharamicrossLookbackPositional(core));
  requireHistoryLength('CDLHARAMICROSS openAndFill', 'prices.high', inHigh.length, inOpen.length);
  requireHistoryLength('CDLHARAMICROSS openAndFill', 'prices.low', inLow.length, inOpen.length);
  requireHistoryLength('CDLHARAMICROSS openAndFill', 'prices.close', inClose.length, inOpen.length);
  requireOutput('CDLHARAMICROSS openAndFill', 'outInteger', outInteger, guardOutLen, Int32Array);
  const inOpenF64 = float64(inOpen);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  const inCloseF64 = float64(inClose);
  if (outInteger.buffer === inOpenF64.buffer && outInteger.byteOffset === inOpenF64.byteOffset) {
    throw sharedMemory('CDLHARAMICROSS openAndFill', 'outInteger', 'prices.open');
  }
  if (outInteger.buffer === inHighF64.buffer && outInteger.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('CDLHARAMICROSS openAndFill', 'outInteger', 'prices.high');
  }
  if (outInteger.buffer === inLowF64.buffer && outInteger.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('CDLHARAMICROSS openAndFill', 'outInteger', 'prices.low');
  }
  if (outInteger.buffer === inCloseF64.buffer && outInteger.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('CDLHARAMICROSS openAndFill', 'outInteger', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inOpenF64)) {
    throw sharedMemory('CDLHARAMICROSS openAndFill', 'outInteger', 'prices.open');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inHighF64)) {
    throw sharedMemory('CDLHARAMICROSS openAndFill', 'outInteger', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inLowF64)) {
    throw sharedMemory('CDLHARAMICROSS openAndFill', 'outInteger', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outInteger, inCloseF64)) {
    throw sharedMemory('CDLHARAMICROSS openAndFill', 'outInteger', 'prices.close');
  }
  return cdlharamicrossOpenAndFillInternal(core, inOpenF64, inHighF64, inLowF64, inCloseF64, 0, new MInteger(), new MInteger(), outInteger);
}

/**
 * Opens a live CDLHARAMICROSS stream over the warm-up history: `value()` starts at
 * the last history bar, bit-identical to {@link cdlharamicross} there.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CdlharamicrossOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlharamicrossLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input or histories of different lengths.
 *
 * @example
 * ```ts
 * import { cdlharamicrossOpen } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const stream = cdlharamicrossOpen({ open, high, low, close });
 * const peeked = stream.peek(100.2, 101.4, 99.1, 100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.2, 101.4, 99.1, 100.9); // the bar closes
 * ```
 */
export function cdlharamicrossOpen(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlharamicrossOptions): CdlharamicrossStream {
  requireArgument('CDLHARAMICROSS open', 'prices', prices);
  const o = optionsOf('CDLHARAMICROSS open', options);
  const core = coreOf('CDLHARAMICROSS open', o.core);
  return cdlharamicrossOpenPositional(prices.open, prices.high, prices.low, prices.close, core);
}

/**
 * {@link cdlharamicrossOpen} that also computes the outputs over the whole history
 * in the same pass, bit-identically to {@link cdlharamicross}.
 *
 * @param prices - Warm-up history. One series per price component this function
 *   reads, all of one length; other components may be present and are ignored.
 *   `open`: Open price of each bar. `high`: High price of each bar. `low`: Low price
 *   of each bar. `close`: Close price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CdlharamicrossOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cdlharamicross} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cdlharamicrossLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, histories of different lengths or a buffer too short,
 *   of the wrong type or starting where another buffer or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cdlharamicrossOpenAndFill } from 'ta-lib';
 *
 * const open = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i - 0.05));
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const { stream, begIdx, values } = cdlharamicrossOpenAndFill({ open, high, low, close });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cdlharamicrossOpenAndFill(prices: { open: ArrayLike<number>; high: ArrayLike<number>; low: ArrayLike<number>; close: ArrayLike<number>; volume?: ArrayLike<number> | undefined; openInterest?: ArrayLike<number> | undefined }, options?: CdlharamicrossOptions): CdlharamicrossResult & { stream: CdlharamicrossStream } {
  requireArgument('CDLHARAMICROSS openAndFill', 'prices', prices);
  const o = optionsOf('CDLHARAMICROSS openAndFill', options);
  const core = coreOf('CDLHARAMICROSS openAndFill', o.core);
  const capacity = fillCapacity(cdlharamicrossLookbackPositional(core), prices.open, prices.high, prices.low, prices.close);
  const outInteger = o.outInteger === undefined ? new Int32Array(capacity) : o.outInteger;
  const stream = cdlharamicrossOpenAndFillPositional(prices.open, prices.high, prices.low, prices.close, outInteger, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outInteger.subarray(0, range.count) };
}
