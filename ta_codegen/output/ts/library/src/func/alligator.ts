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
 *  110726 MF,CC  First version (issue #584).
 */

import { badSink, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { MedpriceStream, medpriceOpenAndFillInternal, medpricePositional } from './medprice.ts';
import { RmaStream, rmaLookbackPositional, rmaOpenAndFillInternal, rmaPositional } from './rma.ts';

/** @internal */
export function alligatorLookbackPositional(optInJawPeriod: number, optInJawShift: number, optInTeethPeriod: number, optInTeethShift: number, optInLipsPeriod: number, optInLipsShift: number, core: Core = Core.DEFAULT): number {
  if ((optInJawPeriod | 0) !== optInJawPeriod) {
    return -1;
  }
  if ((optInJawShift | 0) !== optInJawShift) {
    return -1;
  }
  if ((optInTeethPeriod | 0) !== optInTeethPeriod) {
    return -1;
  }
  if ((optInTeethShift | 0) !== optInTeethShift) {
    return -1;
  }
  if ((optInLipsPeriod | 0) !== optInLipsPeriod) {
    return -1;
  }
  if ((optInLipsShift | 0) !== optInLipsShift) {
    return -1;
  }
  if (optInJawPeriod === Core.INTEGER_DEFAULT) {
    optInJawPeriod = 13;
  } else if (optInJawPeriod < 1 || optInJawPeriod > 100000) {
    return -1;
  }
  if (optInJawShift === Core.INTEGER_DEFAULT) {
    optInJawShift = 8;
  } else if (optInJawShift < 0 || optInJawShift > 100000) {
    return -1;
  }
  if (optInTeethPeriod === Core.INTEGER_DEFAULT) {
    optInTeethPeriod = 8;
  } else if (optInTeethPeriod < 1 || optInTeethPeriod > 100000) {
    return -1;
  }
  if (optInTeethShift === Core.INTEGER_DEFAULT) {
    optInTeethShift = 5;
  } else if (optInTeethShift < 0 || optInTeethShift > 100000) {
    return -1;
  }
  if (optInLipsPeriod === Core.INTEGER_DEFAULT) {
    optInLipsPeriod = 5;
  } else if (optInLipsPeriod < 1 || optInLipsPeriod > 100000) {
    return -1;
  }
  if (optInLipsShift === Core.INTEGER_DEFAULT) {
    optInLipsShift = 3;
  } else if (optInLipsShift < 0 || optInLipsShift > 100000) {
    return -1;
  }
  let jawLookback = 0;
  let teethLookback = 0;
  let lipsLookback = 0;
  let lookback = 0;
  /* Each term is exactly its leg's rma_lookback, never restated, so all three
   * legs inherit TA_FUNC_UNST_RMA. Nothing orders the periods: any leg can be
   * the longest.
   */
  jawLookback = rmaLookbackPositional(optInJawPeriod, core);
  teethLookback = rmaLookbackPositional(optInTeethPeriod, core);
  lipsLookback = rmaLookbackPositional(optInLipsPeriod, core);
  lookback = (jawLookback > teethLookback) ? jawLookback : teethLookback;
  return (lookback > lipsLookback) ? lookback : lipsLookback;
}

/** @internal */
export function alligatorDisplayShiftPositional(optInJawPeriod: number, optInJawShift: number, optInTeethPeriod: number, optInTeethShift: number, optInLipsPeriod: number, optInLipsShift: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (alligatorLookbackPositional(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, core) < 0) {
    return -2147483648;
  }
  if (optInJawPeriod === Core.INTEGER_DEFAULT) {
    optInJawPeriod = 13;
  } else if (optInJawPeriod < 1 || optInJawPeriod > 100000) {
    return -2147483648;
  }
  if (optInJawShift === Core.INTEGER_DEFAULT) {
    optInJawShift = 8;
  } else if (optInJawShift < 0 || optInJawShift > 100000) {
    return -2147483648;
  }
  if (optInTeethPeriod === Core.INTEGER_DEFAULT) {
    optInTeethPeriod = 8;
  } else if (optInTeethPeriod < 1 || optInTeethPeriod > 100000) {
    return -2147483648;
  }
  if (optInTeethShift === Core.INTEGER_DEFAULT) {
    optInTeethShift = 5;
  } else if (optInTeethShift < 0 || optInTeethShift > 100000) {
    return -2147483648;
  }
  if (optInLipsPeriod === Core.INTEGER_DEFAULT) {
    optInLipsPeriod = 5;
  } else if (optInLipsPeriod < 1 || optInLipsPeriod > 100000) {
    return -2147483648;
  }
  if (optInLipsShift === Core.INTEGER_DEFAULT) {
    optInLipsShift = 3;
  } else if (optInLipsShift < 0 || optInLipsShift > 100000) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 3) {
    return -2147483648;
  }
  if (outputIdx === 0) {
    return optInJawShift;
  }
  if (outputIdx === 1) {
    return optInTeethShift;
  }
  return optInLipsShift;
}

/** @internal */
export function alligatorImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  optInJawPeriod: number,
  optInJawShift: number,
  optInTeethPeriod: number,
  optInTeethShift: number,
  optInLipsPeriod: number,
  optInLipsShift: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outJaw: Float64Array,
  outTeeth: Float64Array,
  outLips: Float64Array,
): RetCode {
  let retCode: RetCode = RetCode.SUCCESS;
  let lookbackTotal = 0;
  let mpStartIdx = 0;
  const tempBegIdx = new MInteger();
  const tempNbElement = new MInteger();
  let tempMP!: Float64Array;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInJawPeriod === Core.INTEGER_DEFAULT) {
    optInJawPeriod = 13;
  } else if (optInJawPeriod < 1 || optInJawPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInJawShift === Core.INTEGER_DEFAULT) {
    optInJawShift = 8;
  } else if (optInJawShift < 0 || optInJawShift > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInTeethPeriod === Core.INTEGER_DEFAULT) {
    optInTeethPeriod = 8;
  } else if (optInTeethPeriod < 1 || optInTeethPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInTeethShift === Core.INTEGER_DEFAULT) {
    optInTeethShift = 5;
  } else if (optInTeethShift < 0 || optInTeethShift > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInLipsPeriod === Core.INTEGER_DEFAULT) {
    optInLipsPeriod = 5;
  } else if (optInLipsPeriod < 1 || optInLipsPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInLipsShift === Core.INTEGER_DEFAULT) {
    optInLipsShift = 3;
  } else if (optInLipsShift < 0 || optInLipsShift > 100000) {
    return RetCode.BAD_PARAM;
  }
  if ((outJaw.buffer === outTeeth.buffer && outJaw.byteOffset === outTeeth.byteOffset) || (outJaw.buffer === outLips.buffer && outJaw.byteOffset === outLips.byteOffset) || (outTeeth.buffer === outLips.buffer && outTeeth.byteOffset === outLips.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  lookbackTotal = alligatorLookbackPositional(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, core);
  /* Return before touching anything, so that an input which stops short of
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
  mpStartIdx = startIdx - lookbackTotal;
  tempMP = new Float64Array(endIdx - mpStartIdx + 1);
  /* Every price is consumed here, before any output is written: an output
   * may be aliased onto inHigh or inLow.
   */
  const _xr0 = medpricePositional(mpStartIdx, endIdx, inHigh, inLow, tempMP, core);
  tempBegIdx.value = _xr0.begIdx;
  tempNbElement.value = _xr0.count;
  retCode = RetCode.SUCCESS;
  /* Each leg is entered at startIdx (local index lookbackTotal) and seeds at
   * its OWN lookback before it, so each is exactly TA_RMA over the bars it
   * reads and no leg moves when another leg's period changes. A caller who
   * wants the short legs converged sets TA_FUNC_UNST_RMA.
   */
  const _xr1 = rmaPositional(lookbackTotal, endIdx - mpStartIdx, tempMP, optInJawPeriod, outJaw, core);
  tempBegIdx.value = _xr1.begIdx;
  tempNbElement.value = _xr1.count;
  retCode = RetCode.SUCCESS;
  if (tempNbElement.value === 0) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return retCode;
  }
  const _xr2 = rmaPositional(lookbackTotal, endIdx - mpStartIdx, tempMP, optInTeethPeriod, outTeeth, core);
  tempBegIdx.value = _xr2.begIdx;
  tempNbElement.value = _xr2.count;
  retCode = RetCode.SUCCESS;
  if (tempNbElement.value === 0) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return retCode;
  }
  const _xr3 = rmaPositional(lookbackTotal, endIdx - mpStartIdx, tempMP, optInLipsPeriod, outLips, core);
  tempBegIdx.value = _xr3.begIdx;
  tempNbElement.value = _xr3.count;
  retCode = RetCode.SUCCESS;
  if (tempNbElement.value === 0) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return retCode;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = tempNbElement.value;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['jawPeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['jawShift', Core.INTEGER_DEFAULT, 0, 100000, 'in 0..100000'], ['teethPeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['teethShift', Core.INTEGER_DEFAULT, 0, 100000, 'in 0..100000'], ['lipsPeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['lipsShift', Core.INTEGER_DEFAULT, 0, 100000, 'in 0..100000']] as const;

/** @internal */
export function alligatorPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInJawPeriod: number,
  optInJawShift: number,
  optInTeethPeriod: number,
  optInTeethShift: number,
  optInLipsPeriod: number,
  optInLipsShift: number,
  outJaw: Float64Array,
  outTeeth: Float64Array,
  outLips: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('ALLIGATOR', 'startIdx', startIdx);
  endIdx = requireInt32('ALLIGATOR', 'endIdx', endIdx);
  optInJawPeriod = requireInt32('ALLIGATOR', 'jawPeriod', optInJawPeriod);
  optInJawShift = requireInt32('ALLIGATOR', 'jawShift', optInJawShift);
  optInTeethPeriod = requireInt32('ALLIGATOR', 'teethPeriod', optInTeethPeriod);
  optInTeethShift = requireInt32('ALLIGATOR', 'teethShift', optInTeethShift);
  optInLipsPeriod = requireInt32('ALLIGATOR', 'lipsPeriod', optInLipsPeriod);
  optInLipsShift = requireInt32('ALLIGATOR', 'lipsShift', optInLipsShift);
  requireIndexRange('ALLIGATOR', startIdx, endIdx);
  const guardLookback = alligatorLookbackPositional(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, core);
  if (guardLookback < 0) {
    throw rejectedParams('ALLIGATOR', PARAM_RANGES, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift);
  }
  const guardStart = clampedStart('ALLIGATOR', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('ALLIGATOR', 'prices.high', inHigh, guardInLen);
  requireLength('ALLIGATOR', 'prices.low', inLow, guardInLen);
  requireOutput('ALLIGATOR', 'outJaw', outJaw, guardOutLen, Float64Array);
  requireOutput('ALLIGATOR', 'outTeeth', outTeeth, guardOutLen, Float64Array);
  requireOutput('ALLIGATOR', 'outLips', outLips, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlapsPartly(outJaw, inHighF64)) {
    throw sharedMemory('ALLIGATOR', 'outJaw', 'prices.high');
  }
  if (overlapsPartly(outJaw, inLowF64)) {
    throw sharedMemory('ALLIGATOR', 'outJaw', 'prices.low');
  }
  if (overlapsPartly(outTeeth, inHighF64)) {
    throw sharedMemory('ALLIGATOR', 'outTeeth', 'prices.high');
  }
  if (overlapsPartly(outTeeth, inLowF64)) {
    throw sharedMemory('ALLIGATOR', 'outTeeth', 'prices.low');
  }
  if (overlapsPartly(outLips, inHighF64)) {
    throw sharedMemory('ALLIGATOR', 'outLips', 'prices.high');
  }
  if (overlapsPartly(outLips, inLowF64)) {
    throw sharedMemory('ALLIGATOR', 'outLips', 'prices.low');
  }
  if (overlaps(outJaw, outTeeth)) {
    throw sharedMemory('ALLIGATOR', 'outJaw', 'outTeeth');
  }
  if (overlaps(outJaw, outLips)) {
    throw sharedMemory('ALLIGATOR', 'outJaw', 'outLips');
  }
  if (overlaps(outTeeth, outLips)) {
    throw sharedMemory('ALLIGATOR', 'outTeeth', 'outLips');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = alligatorImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, outBegIdx, outNBElement, outJaw, outTeeth, outLips);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('ALLIGATOR', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link alligator}, {@link alligatorLookback},
 * {@link alligatorDisplayShift}, {@link alligatorOpen} and
 * {@link alligatorOpenAndFill}. Every key is optional: an omitted or `undefined`
 * key takes its default, and a call ignores the keys it has no use for.
 */
export interface AlligatorOptions {
  /**
   * Number of bars in the jaw's smoothed moving average (default 13; range 1 to
   * 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  jawPeriod?: number | undefined;
  /**
   * Bars ahead of the computing bar at which a chart draws the jaw (default 8; range
   * 0 to 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  jawShift?: number | undefined;
  /**
   * Number of bars in the teeth's smoothed moving average (default 8; range 1 to
   * 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  teethPeriod?: number | undefined;
  /**
   * Bars ahead of the computing bar at which a chart draws the teeth (default 5;
   * range 0 to 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  teethShift?: number | undefined;
  /**
   * Number of bars in the lips' smoothed moving average (default 5; range 1 to
   * 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  lipsPeriod?: number | undefined;
  /**
   * Bars ahead of the computing bar at which a chart draws the lips (default 3;
   * range 0 to 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  lipsShift?: number | undefined;
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
   * Buffer for `outJaw`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outJaw?: Float64Array | undefined;
  /**
   * Buffer for `outTeeth`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outTeeth?: Float64Array | undefined;
  /**
   * Buffer for `outLips`, written from index 0 and allocated when omitted. It must
   * hold the values produced (for a batch call the input length always suffices, for
   * an `OpenAndFill` `historyLen - lookback`), and share no memory with another
   * buffer or with an input; a batch call may compute in place over a `Float64Array`
   * input of the same length.
   */
  outLips?: Float64Array | undefined;
}

/**
 * What {@link alligator} returns, and {@link alligatorOpenAndFill} with its
 * stream.
 */
export interface AlligatorResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Jaw: the slowest line, drawn ahead by the jaw shift. `outJaw[k]` is the value at
   * input bar `begIdx + k`.
   */
  outJaw: Float64Array;
  /**
   * Teeth: the middle line, drawn ahead by the teeth shift. `outTeeth[k]` is the
   * value at input bar `begIdx + k`.
   */
  outTeeth: Float64Array;
  /**
   * Lips: the fastest line, drawn ahead by the lips shift. `outLips[k]` is the value
   * at input bar `begIdx + k`.
   */
  outLips: Float64Array;
}

/**
 * Number of leading input bars {@link alligator} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AlligatorOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function alligatorLookback(options?: AlligatorOptions): number {
  const o = optionsOf('ALLIGATOR lookback', options);
  return alligatorLookbackPositional(orDefault(o.jawPeriod, Core.INTEGER_DEFAULT), orDefault(o.jawShift, Core.INTEGER_DEFAULT), orDefault(o.teethPeriod, Core.INTEGER_DEFAULT), orDefault(o.teethShift, Core.INTEGER_DEFAULT), orDefault(o.lipsPeriod, Core.INTEGER_DEFAULT), orDefault(o.lipsShift, Core.INTEGER_DEFAULT), coreOf('ALLIGATOR lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link alligator}.
 *
 * The values are never shifted: this describes the drawing only.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AlligatorOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function alligatorDisplayShift(outputIdx: number, options?: AlligatorOptions): number {
  const o = optionsOf('ALLIGATOR displayShift', options);
  return alligatorDisplayShiftPositional(orDefault(o.jawPeriod, Core.INTEGER_DEFAULT), orDefault(o.jawShift, Core.INTEGER_DEFAULT), orDefault(o.teethPeriod, Core.INTEGER_DEFAULT), orDefault(o.teethShift, Core.INTEGER_DEFAULT), orDefault(o.lipsPeriod, Core.INTEGER_DEFAULT), orDefault(o.lipsShift, Core.INTEGER_DEFAULT), outputIdx, coreOf('ALLIGATOR displayShift', o.core));
}

/**
 * Bill Williams' Alligator (*Trading Chaos*, 1995): three smoothed moving averages
 * of the median price. The Jaw, Teeth and Lips are each drawn a few bars ahead of
 * the bar that computed it. When the three lines are intertwined the alligator
 * sleeps and the market has no trend; when they fan out in order, Lips above Teeth
 * above Jaw or the reverse, it is feeding and a trend is under way. Price crossing
 * back into the lines is read as the trend tiring. The Alligator is a leg of
 * Williams' Profitunity system, alongside the Awesome Oscillator
 * ([`AO`](https://ta-lib.org/functions/ao)) and the Accelerator/Decelerator
 * ([`AC`](https://ta-lib.org/functions/ac)).
 *
 * Formula and more info at
 * [ta-lib.org/functions/alligator](https://ta-lib.org/functions/alligator).
 *
 * **Notes**
 * - Each line is `TA_RMA` of `TA_MEDPRICE` over its own period. Williams draws the
 *   lines ahead of price; that is a display shift: `TA_ALLIGATOR_DisplayShift`
 *   reports each line's shift, and it changes nothing about the values, the
 *   lookback or the returned range. Every output is written at the bar that
 *   computed it, so the last `shift` values of a line are the part a chart draws
 *   past the last bar.
 * - All three lines start at the bar of the longest period, and each shorter line
 *   seeds over the bars its own lookback reaches back from there, not at the first
 *   bar of the series. A chart that seeds every line at the first bar draws
 *   different short lines until the seed has decayed; setting the `RMA` unstable
 *   period closes the gap.
 *
 * `outJaw[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link alligatorLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `high`: High
 *   price of each bar. `low`: Low price of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link AlligatorOptions}.
 * @returns `begIdx` and the values; see {@link AlligatorResult}. Each output is a
 *   view of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, an integer parameter that is not an int32, a parameter outside its range,
 *   an input too short for the range, a buffer that is empty, too short or of the
 *   wrong type, two buffers sharing memory or a buffer sharing memory with a
 *   `Float64Array` input it is not. Checked before anything is written.
 *
 * @example
 * ```ts
 * import { alligator } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const { begIdx, outJaw, outTeeth, outLips } = alligator({ high, low }, {
 *   jawPeriod: 13,
 *   jawShift: 8,
 *   teethPeriod: 8,
 *   teethShift: 5,
 *   lipsPeriod: 5,
 *   lipsShift: 3,
 * });
 * // outJaw[k] is the value at bar begIdx + k
 * ```
 * @see [`rma`](https://ta-lib.org/functions/rma)
 * @see [`medprice`](https://ta-lib.org/functions/medprice)
 * @see [`ao`](https://ta-lib.org/functions/ao)
 * @see [`ac`](https://ta-lib.org/functions/ac)
 * @see [`ichimoku`](https://ta-lib.org/functions/ichimoku)
 */
export function alligator(prices: { high: ArrayLike<number>; low: ArrayLike<number> }, options?: AlligatorOptions): AlligatorResult {
  requireArgument('ALLIGATOR', 'prices', prices);
  const o = optionsOf('ALLIGATOR', options);
  if (o.endIdx === undefined) {
    requireSeries('ALLIGATOR', 'prices.high', prices.high);
    requireSeries('ALLIGATOR', 'prices.low', prices.low);
  }
  const optInJawPeriod = orDefault(o.jawPeriod, Core.INTEGER_DEFAULT);
  const optInJawShift = orDefault(o.jawShift, Core.INTEGER_DEFAULT);
  const optInTeethPeriod = orDefault(o.teethPeriod, Core.INTEGER_DEFAULT);
  const optInTeethShift = orDefault(o.teethShift, Core.INTEGER_DEFAULT);
  const optInLipsPeriod = orDefault(o.lipsPeriod, Core.INTEGER_DEFAULT);
  const optInLipsShift = orDefault(o.lipsShift, Core.INTEGER_DEFAULT);
  const core = coreOf('ALLIGATOR', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, alligatorLookbackPositional(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, core), prices.high, prices.low);
  const outJaw = o.outJaw === undefined ? new Float64Array(capacity) : o.outJaw;
  const outTeeth = o.outTeeth === undefined ? new Float64Array(capacity) : o.outTeeth;
  const outLips = o.outLips === undefined ? new Float64Array(capacity) : o.outLips;
  const range = alligatorPositional(startIdx, endIdx, prices.high, prices.low, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, outJaw, outTeeth, outLips, core);
  return { begIdx: range.begIdx, outJaw: outJaw.subarray(0, range.count), outTeeth: outTeeth.subarray(0, range.count), outLips: outLips.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live ALLIGATOR stream: one value per closed bar, bit-identical to
 * {@link alligator} over the same series. Open it with {@link alligatorOpen} or
 * {@link alligatorOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/alligator](https://ta-lib.org/functions/alligator).
 */
export class AlligatorStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInJawPeriod: number = 0;
  /** @internal */
  optInJawShift: number = 0;
  /** @internal */
  optInTeethPeriod: number = 0;
  /** @internal */
  optInTeethShift: number = 0;
  /** @internal */
  optInLipsPeriod: number = 0;
  /** @internal */
  optInLipsShift: number = 0;
  /** @internal */
  cur_outJaw: number = 0;
  /** @internal */
  cur_outTeeth: number = 0;
  /** @internal */
  cur_outLips: number = 0;
  /** @internal */
  sub0!: MedpriceStream;
  /** @internal */
  sub1!: RmaStream;
  /** @internal */
  sub2!: RmaStream;
  /** @internal */
  sub3!: RmaStream;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): AlligatorStream {
    return new AlligatorStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link alligator} reports them over the same bars. Each accepted `update` and
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
      throw failure('ALLIGATOR advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns its outputs.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param out - Receives the outputs and is returned; a new {@link AlligatorOut}
   *   when omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number, out?: AlligatorOut): AlligatorOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('ALLIGATOR update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('ALLIGATOR update', 'AlligatorOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('ALLIGATOR update', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    alligatorStepImpl(this, inHigh, inLow);
    this.outRangeCount++;
    if (out === undefined) {
      out = new AlligatorOut();
    }
    out.outJaw = this.cur_outJaw;
    out.outTeeth = this.cur_outTeeth;
    out.outLips = this.cur_outLips;
    return out;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @param out - Receives the outputs and is returned; a new {@link AlligatorOut}
   *   when omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number, out?: AlligatorOut): AlligatorOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('ALLIGATOR peek', 'AlligatorOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('ALLIGATOR peek', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    const sp = this;
    let cur_tempMP = 0;
    let cur_outJaw = 0;
    let cur_outTeeth = 0;
    let cur_outLips = 0;
    cur_tempMP = sp.sub0.peek(inHigh, inLow);
    cur_outJaw = sp.sub1.peek(cur_tempMP);
    cur_outTeeth = sp.sub2.peek(cur_tempMP);
    cur_outLips = sp.sub3.peek(cur_tempMP);
    if (out === undefined) {
      out = new AlligatorOut();
    }
    out.outJaw = cur_outJaw;
    out.outTeeth = cur_outTeeth;
    out.outLips = cur_outLips;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link AlligatorOut}
   *   when omitted.
   * @returns `out`, or the new sink.
   */
  value(out?: AlligatorOut): AlligatorOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('ALLIGATOR value', 'AlligatorOut');
    }
    if (out === undefined) {
      out = new AlligatorOut();
    }
    out.outJaw = this.cur_outJaw;
    out.outTeeth = this.cur_outTeeth;
    out.outLips = this.cur_outLips;
    return out;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): AlligatorStream {
    const c = new AlligatorStream(this.core);
    c.optInJawPeriod = this.optInJawPeriod;
    c.optInJawShift = this.optInJawShift;
    c.optInTeethPeriod = this.optInTeethPeriod;
    c.optInTeethShift = this.optInTeethShift;
    c.optInLipsPeriod = this.optInLipsPeriod;
    c.optInLipsShift = this.optInLipsShift;
    c.cur_outJaw = this.cur_outJaw;
    c.cur_outTeeth = this.cur_outTeeth;
    c.cur_outLips = this.cur_outLips;
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
 * The outputs of one ALLIGATOR bar, as `update`, `peek` and `value` return them.
 * Passed back to one of them, it is filled and returned again, with no allocation;
 * it holds its values until the next call that writes it. A new one holds `NaN`.
 */
export class AlligatorOut {
  /** Jaw: the slowest line, drawn ahead by the jaw shift. */
  outJaw: number = NaN;
  /** Teeth: the middle line, drawn ahead by the teeth shift. */
  outTeeth: number = NaN;
  /** Lips: the fastest line, drawn ahead by the lips shift. */
  outLips: number = NaN;
}

function alligatorStepImpl(sp: AlligatorStream, inHigh: number, inLow: number): void {
  let cur_tempMP = 0;
  let cur_outJaw = 0;
  let cur_outTeeth = 0;
  let cur_outLips = 0;
  cur_tempMP = sp.sub0.update(inHigh, inLow);
  cur_outJaw = sp.sub1.update(cur_tempMP);
  cur_outTeeth = sp.sub2.update(cur_tempMP);
  cur_outLips = sp.sub3.update(cur_tempMP);
  sp.cur_outJaw = cur_outJaw;
  sp.cur_outTeeth = cur_outTeeth;
  sp.cur_outLips = cur_outLips;
}

function alligatorOpenImpl(
  sp: AlligatorStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInJawPeriod: number,
  optInJawShift: number,
  optInTeethPeriod: number,
  optInTeethShift: number,
  optInLipsPeriod: number,
  optInLipsShift: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outJaw: Float64Array,
  outTeeth: Float64Array,
  outLips: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let lookbackTotal = 0;
  let mpStartIdx = 0;
  const tempBegIdx = new MInteger();
  const tempNbElement = new MInteger();
  let tempMP!: Float64Array;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInJawPeriod === Core.INTEGER_DEFAULT) {
    optInJawPeriod = 13;
  } else if (optInJawPeriod < 1 || optInJawPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInJawShift === Core.INTEGER_DEFAULT) {
    optInJawShift = 8;
  } else if (optInJawShift < 0 || optInJawShift > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInTeethPeriod === Core.INTEGER_DEFAULT) {
    optInTeethPeriod = 8;
  } else if (optInTeethPeriod < 1 || optInTeethPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInTeethShift === Core.INTEGER_DEFAULT) {
    optInTeethShift = 5;
  } else if (optInTeethShift < 0 || optInTeethShift > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInLipsPeriod === Core.INTEGER_DEFAULT) {
    optInLipsPeriod = 5;
  } else if (optInLipsPeriod < 1 || optInLipsPeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInLipsShift === Core.INTEGER_DEFAULT) {
    optInLipsShift = 3;
  } else if (optInLipsShift < 0 || optInLipsShift > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen < alligatorLookbackPositional(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, core) + 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const sc_outJaw = outStride === 1 ? outJaw : new Float64Array(historyLen);
  const sc_outTeeth = outStride === 1 ? outTeeth : new Float64Array(historyLen);
  const sc_outLips = outStride === 1 ? outLips : new Float64Array(historyLen);
  lookbackTotal = alligatorLookbackPositional(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, core);
  /* Return before touching anything, so that an input which stops short of
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
  mpStartIdx = startIdx - lookbackTotal;
  tempMP = new Float64Array(endIdx - mpStartIdx + 1);
  /* Every price is consumed here, before any output is written: an output
   * may be aliased onto inHigh or inLow.
   */
  const sub0 = medpriceOpenAndFillInternal(core, inHigh, inLow, mpStartIdx, tempBegIdx, tempNbElement, tempMP);
  /* Each leg is entered at startIdx (local index lookbackTotal) and seeds at
   * its OWN lookback before it, so each is exactly TA_RMA over the bars it
   * reads and no leg moves when another leg's period changes. A caller who
   * wants the short legs converged sets TA_FUNC_UNST_RMA.
   */
  const sub1 = rmaOpenAndFillInternal(core, tempMP.slice(0, (endIdx - mpStartIdx) + 1), lookbackTotal, optInJawPeriod, tempBegIdx, tempNbElement, sc_outJaw);
  if (tempNbElement.value === 0) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const sub2 = rmaOpenAndFillInternal(core, tempMP.slice(0, (endIdx - mpStartIdx) + 1), lookbackTotal, optInTeethPeriod, tempBegIdx, tempNbElement, sc_outTeeth);
  if (tempNbElement.value === 0) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  const sub3 = rmaOpenAndFillInternal(core, tempMP.slice(0, (endIdx - mpStartIdx) + 1), lookbackTotal, optInLipsPeriod, tempBegIdx, tempNbElement, sc_outLips);
  if (tempNbElement.value === 0) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = tempNbElement.value;
  if (outNBElement.value < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  sp.optInJawPeriod = optInJawPeriod;
  sp.optInJawShift = optInJawShift;
  sp.optInTeethPeriod = optInTeethPeriod;
  sp.optInTeethShift = optInTeethShift;
  sp.optInLipsPeriod = optInLipsPeriod;
  sp.optInLipsShift = optInLipsShift;
  sp.sub0 = sub0;
  sp.sub1 = sub1;
  sp.sub2 = sub2;
  sp.sub3 = sub3;
  sp.cur_outJaw = sc_outJaw[outNBElement.value - 1];
  sp.cur_outTeeth = sc_outTeeth[outNBElement.value - 1];
  sp.cur_outLips = sc_outLips[outNBElement.value - 1];
  return RetCode.SUCCESS;
}

/** @internal */
export function alligatorOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInJawPeriod: number,
  optInJawShift: number,
  optInTeethPeriod: number,
  optInTeethShift: number,
  optInLipsPeriod: number,
  optInLipsShift: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outJaw: Float64Array,
  outTeeth: Float64Array,
  outLips: Float64Array,
): AlligatorStream {
  const sp = AlligatorStream.of(core);
  const retCode = alligatorOpenImpl(sp, inHigh, inLow, startIdx, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, outBegIdx, outNBElement, outJaw, outTeeth, outLips, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ALLIGATOR openAndFill', inHigh.length, startIdx, alligatorLookbackPositional(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, core));
  }
  if (retCode === RetCode.BAD_PARAM && alligatorLookbackPositional(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, core) < 0) {
    throw rejectedParams('ALLIGATOR openAndFill', PARAM_RANGES, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift);
  }
  throw streamFailure('ALLIGATOR openAndFill', retCode);
}

/** @internal */
export function alligatorOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInJawPeriod: number,
  optInJawShift: number,
  optInTeethPeriod: number,
  optInTeethShift: number,
  optInLipsPeriod: number,
  optInLipsShift: number,
): AlligatorStream {
  const sp = AlligatorStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outJaw = new Float64Array(1);
  const sink_outTeeth = new Float64Array(1);
  const sink_outLips = new Float64Array(1);
  const retCode = alligatorOpenImpl(sp, inHigh, inLow, startIdx, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, outBegIdx, outNBElement, sink_outJaw, sink_outTeeth, sink_outLips, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ALLIGATOR open', inHigh.length, startIdx, alligatorLookbackPositional(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, core));
  }
  if (retCode === RetCode.BAD_PARAM && alligatorLookbackPositional(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, core) < 0) {
    throw rejectedParams('ALLIGATOR open', PARAM_RANGES, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift);
  }
  throw streamFailure('ALLIGATOR open', retCode);
}

/** @internal */
export function alligatorOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInJawPeriod: number,
  optInJawShift: number,
  optInTeethPeriod: number,
  optInTeethShift: number,
  optInLipsPeriod: number,
  optInLipsShift: number,
  core: Core = Core.DEFAULT,
): AlligatorStream {
  optInJawPeriod = requireInt32('ALLIGATOR open', 'jawPeriod', optInJawPeriod);
  optInJawShift = requireInt32('ALLIGATOR open', 'jawShift', optInJawShift);
  optInTeethPeriod = requireInt32('ALLIGATOR open', 'teethPeriod', optInTeethPeriod);
  optInTeethShift = requireInt32('ALLIGATOR open', 'teethShift', optInTeethShift);
  optInLipsPeriod = requireInt32('ALLIGATOR open', 'lipsPeriod', optInLipsPeriod);
  optInLipsShift = requireInt32('ALLIGATOR open', 'lipsShift', optInLipsShift);
  requireArgument('ALLIGATOR open', 'prices.high', inHigh);
  requireHistory('ALLIGATOR open', inHigh.length);
  requireArgument('ALLIGATOR open', 'prices.low', inLow);
  requireHistoryLength('ALLIGATOR open', 'prices.low', inLow.length, inHigh.length);
  return alligatorOpenInternal(core, float64(inHigh), float64(inLow), 0, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift);
}

/** @internal */
export function alligatorOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInJawPeriod: number,
  optInJawShift: number,
  optInTeethPeriod: number,
  optInTeethShift: number,
  optInLipsPeriod: number,
  optInLipsShift: number,
  outJaw: Float64Array,
  outTeeth: Float64Array,
  outLips: Float64Array,
  core: Core = Core.DEFAULT,
): AlligatorStream {
  optInJawPeriod = requireInt32('ALLIGATOR openAndFill', 'jawPeriod', optInJawPeriod);
  optInJawShift = requireInt32('ALLIGATOR openAndFill', 'jawShift', optInJawShift);
  optInTeethPeriod = requireInt32('ALLIGATOR openAndFill', 'teethPeriod', optInTeethPeriod);
  optInTeethShift = requireInt32('ALLIGATOR openAndFill', 'teethShift', optInTeethShift);
  optInLipsPeriod = requireInt32('ALLIGATOR openAndFill', 'lipsPeriod', optInLipsPeriod);
  optInLipsShift = requireInt32('ALLIGATOR openAndFill', 'lipsShift', optInLipsShift);
  requireArgument('ALLIGATOR openAndFill', 'prices.high', inHigh);
  requireHistory('ALLIGATOR openAndFill', inHigh.length);
  requireArgument('ALLIGATOR openAndFill', 'prices.low', inLow);
  const guardLookback = alligatorLookbackPositional(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, core);
  if (guardLookback < 0) {
    throw rejectedParams('ALLIGATOR openAndFill', PARAM_RANGES, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift);
  }
  const guardOutLen = openFillCount('ALLIGATOR openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('ALLIGATOR openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireOutput('ALLIGATOR openAndFill', 'outJaw', outJaw, guardOutLen, Float64Array);
  requireOutput('ALLIGATOR openAndFill', 'outTeeth', outTeeth, guardOutLen, Float64Array);
  requireOutput('ALLIGATOR openAndFill', 'outLips', outLips, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (outJaw.buffer === inHighF64.buffer && outJaw.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('ALLIGATOR openAndFill', 'outJaw', 'prices.high');
  }
  if (outJaw.buffer === inLowF64.buffer && outJaw.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('ALLIGATOR openAndFill', 'outJaw', 'prices.low');
  }
  if (outTeeth.buffer === inHighF64.buffer && outTeeth.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('ALLIGATOR openAndFill', 'outTeeth', 'prices.high');
  }
  if (outTeeth.buffer === inLowF64.buffer && outTeeth.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('ALLIGATOR openAndFill', 'outTeeth', 'prices.low');
  }
  if (outLips.buffer === inHighF64.buffer && outLips.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('ALLIGATOR openAndFill', 'outLips', 'prices.high');
  }
  if (outLips.buffer === inLowF64.buffer && outLips.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('ALLIGATOR openAndFill', 'outLips', 'prices.low');
  }
  if (outJaw.buffer === outTeeth.buffer && outJaw.byteOffset === outTeeth.byteOffset) {
    throw sharedMemory('ALLIGATOR openAndFill', 'outJaw', 'outTeeth');
  }
  if (outJaw.buffer === outLips.buffer && outJaw.byteOffset === outLips.byteOffset) {
    throw sharedMemory('ALLIGATOR openAndFill', 'outJaw', 'outLips');
  }
  if (outTeeth.buffer === outLips.buffer && outTeeth.byteOffset === outLips.byteOffset) {
    throw sharedMemory('ALLIGATOR openAndFill', 'outTeeth', 'outLips');
  }
  if (guardOutLen > 0 && overlaps(outJaw, inHighF64)) {
    throw sharedMemory('ALLIGATOR openAndFill', 'outJaw', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outJaw, inLowF64)) {
    throw sharedMemory('ALLIGATOR openAndFill', 'outJaw', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outTeeth, inHighF64)) {
    throw sharedMemory('ALLIGATOR openAndFill', 'outTeeth', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outTeeth, inLowF64)) {
    throw sharedMemory('ALLIGATOR openAndFill', 'outTeeth', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outLips, inHighF64)) {
    throw sharedMemory('ALLIGATOR openAndFill', 'outLips', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outLips, inLowF64)) {
    throw sharedMemory('ALLIGATOR openAndFill', 'outLips', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outJaw, outTeeth)) {
    throw sharedMemory('ALLIGATOR openAndFill', 'outJaw', 'outTeeth');
  }
  if (guardOutLen > 0 && overlaps(outJaw, outLips)) {
    throw sharedMemory('ALLIGATOR openAndFill', 'outJaw', 'outLips');
  }
  if (guardOutLen > 0 && overlaps(outTeeth, outLips)) {
    throw sharedMemory('ALLIGATOR openAndFill', 'outTeeth', 'outLips');
  }
  return alligatorOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, new MInteger(), new MInteger(), outJaw, outTeeth, outLips);
}

/**
 * Opens a live ALLIGATOR stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link alligator} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link AlligatorOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `alligatorLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, an integer parameter that is not an int32, a parameter
 *   outside its range or histories of different lengths.
 *
 * @example
 * ```ts
 * import { alligatorOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = alligatorOpen({ high, low }, {
 *   jawPeriod: 13,
 *   jawShift: 8,
 *   teethPeriod: 8,
 *   teethShift: 5,
 *   lipsPeriod: 5,
 *   lipsShift: 3,
 * });
 * const peeked = stream.peek(101.4, 99.1); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1); // the bar closes
 * ```
 */
export function alligatorOpen(prices: { high: ArrayLike<number>; low: ArrayLike<number> }, options?: AlligatorOptions): AlligatorStream {
  requireArgument('ALLIGATOR open', 'prices', prices);
  const o = optionsOf('ALLIGATOR open', options);
  const optInJawPeriod = orDefault(o.jawPeriod, Core.INTEGER_DEFAULT);
  const optInJawShift = orDefault(o.jawShift, Core.INTEGER_DEFAULT);
  const optInTeethPeriod = orDefault(o.teethPeriod, Core.INTEGER_DEFAULT);
  const optInTeethShift = orDefault(o.teethShift, Core.INTEGER_DEFAULT);
  const optInLipsPeriod = orDefault(o.lipsPeriod, Core.INTEGER_DEFAULT);
  const optInLipsShift = orDefault(o.lipsShift, Core.INTEGER_DEFAULT);
  const core = coreOf('ALLIGATOR open', o.core);
  return alligatorOpenPositional(prices.high, prices.low, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, core);
}

/**
 * {@link alligatorOpen} that also computes the outputs over the whole history in
 * the same pass, bit-identically to {@link alligator}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price of each bar. `low`: Low price of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link AlligatorOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link alligator} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `alligatorLookback(...) + 1` bars, an empty one included, and
 *   `TA_BAD_PARAM` for options that are not an object, a `core` that is not a
 *   `Core`, an absent input, an integer parameter that is not an int32, a parameter
 *   outside its range, histories of different lengths or a buffer too short, of the
 *   wrong type or starting where another buffer or a `Float64Array` input starts,
 *   or, on a history long enough to open, one sharing memory with another buffer or
 *   a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { alligatorOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const {
 *   stream,
 *   begIdx,
 *   outJaw,
 *   outTeeth,
 *   outLips,
 * } = alligatorOpenAndFill({ high, low }, {
 *   jawPeriod: 13,
 *   jawShift: 8,
 *   teethPeriod: 8,
 *   teethShift: 5,
 *   lipsPeriod: 5,
 *   lipsShift: 3,
 * });
 * // outJaw[k] is the value at bar begIdx + k
 * ```
 */
export function alligatorOpenAndFill(prices: { high: ArrayLike<number>; low: ArrayLike<number> }, options?: AlligatorOptions): AlligatorResult & { stream: AlligatorStream } {
  requireArgument('ALLIGATOR openAndFill', 'prices', prices);
  const o = optionsOf('ALLIGATOR openAndFill', options);
  const optInJawPeriod = orDefault(o.jawPeriod, Core.INTEGER_DEFAULT);
  const optInJawShift = orDefault(o.jawShift, Core.INTEGER_DEFAULT);
  const optInTeethPeriod = orDefault(o.teethPeriod, Core.INTEGER_DEFAULT);
  const optInTeethShift = orDefault(o.teethShift, Core.INTEGER_DEFAULT);
  const optInLipsPeriod = orDefault(o.lipsPeriod, Core.INTEGER_DEFAULT);
  const optInLipsShift = orDefault(o.lipsShift, Core.INTEGER_DEFAULT);
  const core = coreOf('ALLIGATOR openAndFill', o.core);
  const capacity = fillCapacity(alligatorLookbackPositional(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, core), prices.high, prices.low);
  const outJaw = o.outJaw === undefined ? new Float64Array(capacity) : o.outJaw;
  const outTeeth = o.outTeeth === undefined ? new Float64Array(capacity) : o.outTeeth;
  const outLips = o.outLips === undefined ? new Float64Array(capacity) : o.outLips;
  const stream = alligatorOpenAndFillPositional(prices.high, prices.low, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, outJaw, outTeeth, outLips, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outJaw: outJaw.subarray(0, range.count), outTeeth: outTeeth.subarray(0, range.count), outLips: outLips.subarray(0, range.count) };
}
