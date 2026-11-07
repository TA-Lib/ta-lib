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
 *  090526 MF,CC  Initial version (#371).
 *  100426 MF,CC  Display shift (#500).
 */

import { badSink, batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function fractalLookbackPositional(optInLeftBars: number, optInRightBars: number, _core: Core = Core.DEFAULT): number {
  if ((optInLeftBars | 0) !== optInLeftBars) {
    return -1;
  }
  if ((optInRightBars | 0) !== optInRightBars) {
    return -1;
  }
  if (optInLeftBars === Core.INTEGER_DEFAULT) {
    optInLeftBars = 2;
  } else if (optInLeftBars < 1 || optInLeftBars > 100000) {
    return -1;
  }
  if (optInRightBars === Core.INTEGER_DEFAULT) {
    optInRightBars = 2;
  } else if (optInRightBars < 1 || optInRightBars > 100000) {
    return -1;
  }
  return optInLeftBars + optInRightBars;
}

/** @internal */
export function fractalDisplayShiftPositional(optInLeftBars: number, optInRightBars: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (fractalLookbackPositional(optInLeftBars, optInRightBars, core) < 0) {
    return -2147483648;
  }
  if (optInLeftBars === Core.INTEGER_DEFAULT) {
    optInLeftBars = 2;
  } else if (optInLeftBars < 1 || optInLeftBars > 100000) {
    return -2147483648;
  }
  if (optInRightBars === Core.INTEGER_DEFAULT) {
    optInRightBars = 2;
  } else if (optInRightBars < 1 || optInRightBars > 100000) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 2) {
    return -2147483648;
  }
  /* The flag written at a bar names the pivot this many bars back, which is
   * where a chart draws it.
   */
  return (-optInRightBars | 0);
}

/** @internal */
export function fractalImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  optInLeftBars: number,
  optInRightBars: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outSwingHigh: Int32Array,
  outSwingLow: Int32Array,
): RetCode {
  let today = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let i = 0;
  let pivotHigh = 0;
  let pivotLow = 0;
  let otherHigh = 0;
  let otherLow = 0;
  let tempHigh = 0;
  let tempLow = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInLeftBars === Core.INTEGER_DEFAULT) {
    optInLeftBars = 2;
  } else if (optInLeftBars < 1 || optInLeftBars > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInRightBars === Core.INTEGER_DEFAULT) {
    optInRightBars = 2;
  } else if (optInRightBars < 1 || optInRightBars > 100000) {
    return RetCode.BAD_PARAM;
  }
  if ((outSwingHigh.buffer === outSwingLow.buffer && outSwingHigh.byteOffset === outSwingLow.byteOffset)) {
    return RetCode.BAD_PARAM;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = fractalLookbackPositional(optInLeftBars, optInRightBars, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  outIdx = 0;
  today = startIdx;
  while (today <= endIdx) {
    pivotHigh = 0.0;
    pivotLow = 0.0;
    otherHigh = 0.0;
    otherLow = 0.0;
    for (i = optInLeftBars + optInRightBars; i >= 0; i -= 1) {
      tempHigh = inHigh[today - i];
      tempLow = inLow[today - i];
      if (i === optInRightBars) {
        pivotHigh = tempHigh;
        pivotLow = tempLow;
      } else if (i === optInLeftBars + optInRightBars) {
        /* optInLeftBars is at least 1, so the oldest bar of the window is
         * never the pivot and always seeds the rest-of-window extrema.
         */
        otherHigh = tempHigh;
        otherLow = tempLow;
      } else {
        if (tempHigh > otherHigh) {
          otherHigh = tempHigh;
        }
        if (tempLow < otherLow) {
          otherLow = tempLow;
        }
      }
    }
    if (pivotHigh > otherHigh) {
      outSwingHigh[outIdx] = 100;
    } else {
      outSwingHigh[outIdx] = 0;
    }
    if (pivotLow < otherLow) {
      outSwingLow[outIdx] = 100;
    } else {
      outSwingLow[outIdx] = 0;
    }
    outIdx += 1;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['leftBars', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000'], ['rightBars', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000']] as const;

/** @internal */
export function fractalPositional(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInLeftBars: number,
  optInRightBars: number,
  outSwingHigh: Int32Array,
  outSwingLow: Int32Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('FRACTAL', 'startIdx', startIdx);
  endIdx = requireInt32('FRACTAL', 'endIdx', endIdx);
  optInLeftBars = requireInt32('FRACTAL', 'leftBars', optInLeftBars);
  optInRightBars = requireInt32('FRACTAL', 'rightBars', optInRightBars);
  requireIndexRange('FRACTAL', startIdx, endIdx);
  const guardLookback = fractalLookbackPositional(optInLeftBars, optInRightBars, core);
  if (guardLookback < 0) {
    throw rejectedParams('FRACTAL', PARAM_RANGES, optInLeftBars, optInRightBars);
  }
  const guardStart = clampedStart('FRACTAL', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('FRACTAL', 'prices.high', inHigh, guardInLen);
  requireLength('FRACTAL', 'prices.low', inLow, guardInLen);
  requireOutput('FRACTAL', 'outSwingHigh', outSwingHigh, guardOutLen, Int32Array);
  requireOutput('FRACTAL', 'outSwingLow', outSwingLow, guardOutLen, Int32Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlaps(outSwingHigh, inHighF64)) {
    throw sharedMemory('FRACTAL', 'outSwingHigh', 'prices.high');
  }
  if (overlaps(outSwingHigh, inLowF64)) {
    throw sharedMemory('FRACTAL', 'outSwingHigh', 'prices.low');
  }
  if (overlaps(outSwingLow, inHighF64)) {
    throw sharedMemory('FRACTAL', 'outSwingLow', 'prices.high');
  }
  if (overlaps(outSwingLow, inLowF64)) {
    throw sharedMemory('FRACTAL', 'outSwingLow', 'prices.low');
  }
  if (overlaps(outSwingHigh, outSwingLow)) {
    throw sharedMemory('FRACTAL', 'outSwingHigh', 'outSwingLow');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = fractalImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInLeftBars, optInRightBars, outBegIdx, outNBElement, outSwingHigh, outSwingLow);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('FRACTAL', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link fractal}, {@link fractalLookback},
 * {@link fractalDisplayShift}, {@link fractalOpen} and {@link fractalOpenAndFill}.
 * Every key is optional: an omitted or `undefined` key takes its default, and a
 * call ignores the keys it has no use for.
 */
export interface FractalOptions {
  /**
   * Bars before the pivot that it must strictly dominate (default 2; range 1 to
   * 100000; `Core.INTEGER_DEFAULT` selects the default).
   */
  leftBars?: number | undefined;
  /**
   * Bars after the pivot that it must strictly dominate, and the delay before the
   * verdict is reported (default 2; range 1 to 100000; `Core.INTEGER_DEFAULT`
   * selects the default).
   */
  rightBars?: number | undefined;
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
   * Buffer for `outSwingHigh`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input.
   */
  outSwingHigh?: Int32Array | undefined;
  /**
   * Buffer for `outSwingLow`, written from index 0 and allocated when omitted. It
   * must hold the values produced (for a batch call the input length always
   * suffices, for an `OpenAndFill` `historyLen - lookback`), and share no memory
   * with another buffer or with an input.
   */
  outSwingLow?: Int32Array | undefined;
}

/** What {@link fractal} returns, and {@link fractalOpenAndFill} with its stream. */
export interface FractalResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * 100 when the bar `rightBars` back is a strict swing high, 0 otherwise.
   * `outSwingHigh[k]` is the value at input bar `begIdx + k`.
   */
  outSwingHigh: Int32Array;
  /**
   * 100 when the bar `rightBars` back is a strict swing low, 0 otherwise.
   * `outSwingLow[k]` is the value at input bar `begIdx + k`.
   */
  outSwingLow: Int32Array;
}

/**
 * Number of leading input bars {@link fractal} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link FractalOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function fractalLookback(options?: FractalOptions): number {
  const o = optionsOf('FRACTAL lookback', options);
  return fractalLookbackPositional(orDefault(o.leftBars, Core.INTEGER_DEFAULT), orDefault(o.rightBars, Core.INTEGER_DEFAULT), coreOf('FRACTAL lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link fractal}.
 *
 * The values are never shifted: this describes the drawing only.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link FractalOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function fractalDisplayShift(outputIdx: number, options?: FractalOptions): number {
  const o = optionsOf('FRACTAL displayShift', options);
  return fractalDisplayShiftPositional(orDefault(o.leftBars, Core.INTEGER_DEFAULT), orDefault(o.rightBars, Core.INTEGER_DEFAULT), outputIdx, coreOf('FRACTAL displayShift', o.core));
}

/**
 * Williams Fractal: a causal swing-pivot detector. A bar is a swing high when its
 * high strictly exceeds the highs of the `leftBars` bars before it and the
 * `rightBars` bars after it; a swing low is the mirror on the lows. Bill Williams'
 * original is the symmetric five-candle case; independent left and right arms
 * generalise it. The right arm cannot be known until it has closed, so the value
 * is written on the bar that completes the pivot, `rightBars` bars after the pivot
 * itself. Each output value therefore describes the bar `rightBars` back, not the
 * bar it is written at: a flag at output index `k` names input bar
 * `begIdx + k - rightBars`, whose price is `high[...]` / `low[...]` at that index.
 * The display shift both outputs report, `-rightBars`, is that offset: a chart
 * draws each flag that many bars to the left. The two outputs are independent
 * flags rather than one signed value, because an outside bar can be a swing high
 * and a swing low at once.
 *
 * Formula and more info at
 * [ta-lib.org/functions/fractal](https://ta-lib.org/functions/fractal).
 *
 * **Notes**
 * - Strict on both sides: a bar tied with any other bar of its window is not a
 *   pivot. TradingView's Pine runtime differs: its `ta.pivothigh` / `ta.pivotlow`
 *   let a tie with an older bar stand and let a tie with a newer bar cancel, i.e.
 *   non-strict left and strict right — so a plateau Pine reports as a pivot is not
 *   one here.
 * - Each output is decided on its own side: a high tied with any other high in the
 *   window forces `outSwingHigh` to 0 while leaving `outSwingLow` free to fire
 *   100, and the mirror holds. Only a window flat in both series emits 0 on both.
 *
 * `outSwingHigh[k]` is the value at input bar `begIdx + k`; every output is
 * exactly as long as the number of values written, and nothing is padded. A range
 * that ends before {@link fractalLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `high`: High
 *   price series. `low`: Low price series.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link FractalOptions}.
 * @returns `begIdx` and the values; see {@link FractalResult}. Each output is a
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
 * import { fractal } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const { begIdx, outSwingHigh, outSwingLow } = fractal({ high, low }, {
 *   leftBars: 2,
 *   rightBars: 2,
 * });
 * // outSwingHigh[k] is the value at bar begIdx + k
 * ```
 * @see [`aroon`](https://ta-lib.org/functions/aroon)
 * @see [`maxindex`](https://ta-lib.org/functions/maxindex)
 * @see [`minindex`](https://ta-lib.org/functions/minindex)
 * @see [`minmaxindex`](https://ta-lib.org/functions/minmaxindex)
 */
export function fractal(prices: { high: ArrayLike<number>; low: ArrayLike<number> }, options?: FractalOptions): FractalResult {
  requireArgument('FRACTAL', 'prices', prices);
  const o = optionsOf('FRACTAL', options);
  if (o.endIdx === undefined) {
    requireSeries('FRACTAL', 'prices.high', prices.high);
    requireSeries('FRACTAL', 'prices.low', prices.low);
  }
  const optInLeftBars = orDefault(o.leftBars, Core.INTEGER_DEFAULT);
  const optInRightBars = orDefault(o.rightBars, Core.INTEGER_DEFAULT);
  const core = coreOf('FRACTAL', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.high.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, fractalLookbackPositional(optInLeftBars, optInRightBars, core), prices.high, prices.low);
  const outSwingHigh = o.outSwingHigh === undefined ? new Int32Array(capacity) : o.outSwingHigh;
  const outSwingLow = o.outSwingLow === undefined ? new Int32Array(capacity) : o.outSwingLow;
  const range = fractalPositional(startIdx, endIdx, prices.high, prices.low, optInLeftBars, optInRightBars, outSwingHigh, outSwingLow, core);
  return { begIdx: range.begIdx, outSwingHigh: outSwingHigh.subarray(0, range.count), outSwingLow: outSwingLow.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live FRACTAL stream: one value per closed bar, bit-identical to
 * {@link fractal} over the same series. Open it with {@link fractalOpen} or
 * {@link fractalOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/fractal](https://ta-lib.org/functions/fractal).
 */
export class FractalStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInLeftBars: number = 0;
  /** @internal */
  optInRightBars: number = 0;
  /** @internal */
  winPos_i: number = 0;
  /** @internal */
  winCap_i: number = 1;
  /** @internal */
  win_i_inHigh: Float64Array = new Float64Array(1);
  /** @internal */
  win_i_inLow: Float64Array = new Float64Array(1);
  /** @internal */
  cur_outSwingHigh: number = 0;
  /** @internal */
  cur_outSwingLow: number = 0;
  /** @internal */
  outRangeBegIdx: number = 0;
  /** @internal */
  outRangeCount: number = 0;

  private constructor(core: Core) {
    this.core = core;
  }

  /** @internal */
  static of(core: Core): FractalStream {
    return new FractalStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link fractal} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('FRACTAL advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns its outputs.
   *
   * @param high - The bar's `high` value: High price series.
   * @param low - The bar's `low` value: Low price series.
   * @param out - Receives the outputs and is returned; a new {@link FractalOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included, before anything moves.
   *   `TALibError` with code `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached
   *   bar `Core.INDEX_MAX`.
   */
  update(high: number, low: number, out?: FractalOut): FractalOut {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('FRACTAL update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('FRACTAL update', 'FractalOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low)) {
      throw nonFiniteBar('FRACTAL update', !Number.isFinite(high) ? 'high' : 'low');
    }
    fractalStepImpl(this, high, low);
    this.outRangeCount++;
    if (out === undefined) {
      out = new FractalOut();
    }
    out.outSwingHigh = this.cur_outSwingHigh;
    out.outSwingLow = this.cur_outSwingLow;
    return out;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param high - The bar's `high` value: High price series.
   * @param low - The bar's `low` value: Low price series.
   * @param out - Receives the outputs and is returned; a new {@link FractalOut} when
   *   omitted.
   * @returns `out`, or the new sink, as `update` would fill it.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, or a sink that is not an object, `null` included.
   */
  peek(high: number, low: number, out?: FractalOut): FractalOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('FRACTAL peek', 'FractalOut');
    }
    if (!Number.isFinite(high) || !Number.isFinite(low)) {
      throw nonFiniteBar('FRACTAL peek', !Number.isFinite(high) ? 'high' : 'low');
    }
    const sp = this;
    const inHigh = high;
    const inLow = low;
    let i = 0;
    let pivotHigh = 0;
    let pivotLow = 0;
    let otherHigh = 0;
    let otherLow = 0;
    let tempHigh = 0;
    let tempLow = 0;
    let cur_outSwingHigh = 0;
    let cur_outSwingLow = 0;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    let pkSlot1 = -1;
    let pkVal1 = 0;
    pkSlot0 = sp.winPos_i;
    pkVal0 = inHigh;
    pkSlot1 = sp.winPos_i;
    pkVal1 = inLow;
    pivotHigh = 0.0;
    pivotLow = 0.0;
    otherHigh = 0.0;
    otherLow = 0.0;
    for (i = sp.optInLeftBars + sp.optInRightBars; i >= 0; i -= 1) {
      tempHigh = (((sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i) !== pkSlot0) ? sp.win_i_inHigh[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i] : pkVal0;
      tempLow = (((sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i) !== pkSlot1) ? sp.win_i_inLow[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i] : pkVal1;
      if (i === sp.optInRightBars) {
        pivotHigh = tempHigh;
        pivotLow = tempLow;
      } else if (i === sp.optInLeftBars + sp.optInRightBars) {
        /* optInLeftBars is at least 1, so the oldest bar of the window is
         * never the pivot and always seeds the rest-of-window extrema.
         */
        otherHigh = tempHigh;
        otherLow = tempLow;
      } else {
        if (tempHigh > otherHigh) {
          otherHigh = tempHigh;
        }
        if (tempLow < otherLow) {
          otherLow = tempLow;
        }
      }
    }
    if (pivotHigh > otherHigh) {
      cur_outSwingHigh = 100;
    } else {
      cur_outSwingHigh = 0;
    }
    if (pivotLow < otherLow) {
      cur_outSwingLow = 100;
    } else {
      cur_outSwingLow = 0;
    }
    if (out === undefined) {
      out = new FractalOut();
    }
    out.outSwingHigh = cur_outSwingHigh;
    out.outSwingLow = cur_outSwingLow;
    return out;
  }

  /**
   * The outputs at the last bar this stream counted; `peek` does not change them.
   *
   * @param out - Receives the outputs and is returned; a new {@link FractalOut} when
   *   omitted.
   * @returns `out`, or the new sink.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a sink that is not an object,
   *   `null` included.
   */
  value(out?: FractalOut): FractalOut {
    if (out !== undefined && (typeof out !== 'object' || out === null)) {
      throw badSink('FRACTAL value', 'FractalOut');
    }
    if (out === undefined) {
      out = new FractalOut();
    }
    out.outSwingHigh = this.cur_outSwingHigh;
    out.outSwingLow = this.cur_outSwingLow;
    return out;
  }

  /**
   * An independent fork: both streams evolve separately from here. Buffers are
   * copied and sub-streams cloned; the `Core` is shared.
   *
   * @returns A stream at the same bar.
   */
  clone(): FractalStream {
    const c = new FractalStream(this.core);
    c.optInLeftBars = this.optInLeftBars;
    c.optInRightBars = this.optInRightBars;
    c.winPos_i = this.winPos_i;
    c.winCap_i = this.winCap_i;
    c.win_i_inHigh = this.win_i_inHigh.slice();
    c.win_i_inLow = this.win_i_inLow.slice();
    c.cur_outSwingHigh = this.cur_outSwingHigh;
    c.cur_outSwingLow = this.cur_outSwingLow;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

/**
 * The outputs of one FRACTAL bar, as `update`, `peek` and `value` return them.
 * Passed back to one of them, it is filled and returned again, with no allocation;
 * it holds its values until the next call that writes it. A new one holds `NaN`.
 */
export class FractalOut {
  /** 100 when the bar `rightBars` back is a strict swing high, 0 otherwise. */
  outSwingHigh: number = NaN;
  /** 100 when the bar `rightBars` back is a strict swing low, 0 otherwise. */
  outSwingLow: number = NaN;
}

function fractalStepImpl(sp: FractalStream, inHigh: number, inLow: number): void {
  let i = 0;
  let pivotHigh = 0;
  let pivotLow = 0;
  let otherHigh = 0;
  let otherLow = 0;
  let tempHigh = 0;
  let tempLow = 0;
  sp.win_i_inHigh[sp.winPos_i] = inHigh;
  sp.win_i_inLow[sp.winPos_i] = inLow;
  pivotHigh = 0.0;
  pivotLow = 0.0;
  otherHigh = 0.0;
  otherLow = 0.0;
  for (i = sp.optInLeftBars + sp.optInRightBars; i >= 0; i -= 1) {
    tempHigh = sp.win_i_inHigh[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i];
    tempLow = sp.win_i_inLow[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i];
    if (i === sp.optInRightBars) {
      pivotHigh = tempHigh;
      pivotLow = tempLow;
    } else if (i === sp.optInLeftBars + sp.optInRightBars) {
      /* optInLeftBars is at least 1, so the oldest bar of the window is
       * never the pivot and always seeds the rest-of-window extrema.
       */
      otherHigh = tempHigh;
      otherLow = tempLow;
    } else {
      if (tempHigh > otherHigh) {
        otherHigh = tempHigh;
      }
      if (tempLow < otherLow) {
        otherLow = tempLow;
      }
    }
  }
  if (pivotHigh > otherHigh) {
    sp.cur_outSwingHigh = 100;
  } else {
    sp.cur_outSwingHigh = 0;
  }
  if (pivotLow < otherLow) {
    sp.cur_outSwingLow = 100;
  } else {
    sp.cur_outSwingLow = 0;
  }
  sp.winPos_i = sp.winPos_i + 1;
  if (sp.winPos_i >= sp.winCap_i) {
    sp.winPos_i = 0;
  }
}

function fractalOpenImpl(
  sp: FractalStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInLeftBars: number,
  optInRightBars: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outSwingHigh: Int32Array,
  outSwingLow: Int32Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let today = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let i = 0;
  let pivotHigh = 0;
  let pivotLow = 0;
  let otherHigh = 0;
  let otherLow = 0;
  let tempHigh = 0;
  let tempLow = 0;
  const historyLen = inHigh.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inLow.length !== inHigh.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInLeftBars === Core.INTEGER_DEFAULT) {
    optInLeftBars = 2;
  } else if (optInLeftBars < 1 || optInLeftBars > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInRightBars === Core.INTEGER_DEFAULT) {
    optInRightBars = 2;
  } else if (optInRightBars < 1 || optInRightBars > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = fractalLookbackPositional(optInLeftBars, optInRightBars, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outIdx = 0;
  today = startIdx;
  while (today <= endIdx) {
    pivotHigh = 0.0;
    pivotLow = 0.0;
    otherHigh = 0.0;
    otherLow = 0.0;
    for (i = optInLeftBars + optInRightBars; i >= 0; i -= 1) {
      tempHigh = inHigh[today - i];
      tempLow = inLow[today - i];
      if (i === optInRightBars) {
        pivotHigh = tempHigh;
        pivotLow = tempLow;
      } else if (i === optInLeftBars + optInRightBars) {
        /* optInLeftBars is at least 1, so the oldest bar of the window is
         * never the pivot and always seeds the rest-of-window extrema.
         */
        otherHigh = tempHigh;
        otherLow = tempLow;
      } else {
        if (tempHigh > otherHigh) {
          otherHigh = tempHigh;
        }
        if (tempLow < otherLow) {
          otherLow = tempLow;
        }
      }
    }
    if (pivotHigh > otherHigh) {
      outSwingHigh[((outIdx * outStride) | 0)] = 100;
    } else {
      outSwingHigh[((outIdx * outStride) | 0)] = 0;
    }
    if (pivotLow < otherLow) {
      outSwingLow[((outIdx * outStride) | 0)] = 100;
    } else {
      outSwingLow[((outIdx * outStride) | 0)] = 0;
    }
    outIdx += 1;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  const cap_i = optInLeftBars + optInRightBars + 1;
  if (cap_i < 1 || cap_i > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const capWin_i_inHigh = inHigh.slice(historyLen - cap_i, historyLen);
  const capWin_i_inLow = inLow.slice(historyLen - cap_i, historyLen);
  sp.optInLeftBars = optInLeftBars;
  sp.optInRightBars = optInRightBars;
  sp.winPos_i = 0;
  sp.winCap_i = cap_i;
  sp.win_i_inHigh = capWin_i_inHigh;
  sp.win_i_inLow = capWin_i_inLow;
  sp.cur_outSwingHigh = outSwingHigh[(outNBElement.value - 1) * outStride];
  sp.cur_outSwingLow = outSwingLow[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function fractalOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInLeftBars: number,
  optInRightBars: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outSwingHigh: Int32Array,
  outSwingLow: Int32Array,
): FractalStream {
  const sp = FractalStream.of(core);
  const retCode = fractalOpenImpl(sp, inHigh, inLow, startIdx, optInLeftBars, optInRightBars, outBegIdx, outNBElement, outSwingHigh, outSwingLow, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('FRACTAL openAndFill', inHigh.length, startIdx, fractalLookbackPositional(optInLeftBars, optInRightBars, core));
  }
  if (retCode === RetCode.BAD_PARAM && fractalLookbackPositional(optInLeftBars, optInRightBars, core) < 0) {
    throw rejectedParams('FRACTAL openAndFill', PARAM_RANGES, optInLeftBars, optInRightBars);
  }
  throw streamFailure('FRACTAL openAndFill', retCode);
}

/** @internal */
export function fractalOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInLeftBars: number,
  optInRightBars: number,
): FractalStream {
  const sp = FractalStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outSwingHigh = new Int32Array(1);
  const sink_outSwingLow = new Int32Array(1);
  const retCode = fractalOpenImpl(sp, inHigh, inLow, startIdx, optInLeftBars, optInRightBars, outBegIdx, outNBElement, sink_outSwingHigh, sink_outSwingLow, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('FRACTAL open', inHigh.length, startIdx, fractalLookbackPositional(optInLeftBars, optInRightBars, core));
  }
  if (retCode === RetCode.BAD_PARAM && fractalLookbackPositional(optInLeftBars, optInRightBars, core) < 0) {
    throw rejectedParams('FRACTAL open', PARAM_RANGES, optInLeftBars, optInRightBars);
  }
  throw streamFailure('FRACTAL open', retCode);
}

/** @internal */
export function fractalOpenPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInLeftBars: number,
  optInRightBars: number,
  core: Core = Core.DEFAULT,
): FractalStream {
  optInLeftBars = requireInt32('FRACTAL open', 'leftBars', optInLeftBars);
  optInRightBars = requireInt32('FRACTAL open', 'rightBars', optInRightBars);
  requireArgument('FRACTAL open', 'prices.high', inHigh);
  requireHistory('FRACTAL open', 'prices.high', inHigh.length);
  requireArgument('FRACTAL open', 'prices.low', inLow);
  requireHistoryLength('FRACTAL open', 'prices.low', inLow.length, inHigh.length);
  return fractalOpenInternal(core, float64(inHigh), float64(inLow), 0, optInLeftBars, optInRightBars);
}

/** @internal */
export function fractalOpenAndFillPositional(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInLeftBars: number,
  optInRightBars: number,
  outSwingHigh: Int32Array,
  outSwingLow: Int32Array,
  core: Core = Core.DEFAULT,
): FractalStream {
  optInLeftBars = requireInt32('FRACTAL openAndFill', 'leftBars', optInLeftBars);
  optInRightBars = requireInt32('FRACTAL openAndFill', 'rightBars', optInRightBars);
  requireArgument('FRACTAL openAndFill', 'prices.high', inHigh);
  requireHistory('FRACTAL openAndFill', 'prices.high', inHigh.length);
  requireArgument('FRACTAL openAndFill', 'prices.low', inLow);
  const guardLookback = fractalLookbackPositional(optInLeftBars, optInRightBars, core);
  if (guardLookback < 0) {
    throw rejectedParams('FRACTAL openAndFill', PARAM_RANGES, optInLeftBars, optInRightBars);
  }
  const guardOutLen = openFillCount('FRACTAL openAndFill', inHigh.length, guardLookback);
  requireHistoryLength('FRACTAL openAndFill', 'prices.low', inLow.length, inHigh.length);
  requireOutput('FRACTAL openAndFill', 'outSwingHigh', outSwingHigh, guardOutLen, Int32Array);
  requireOutput('FRACTAL openAndFill', 'outSwingLow', outSwingLow, guardOutLen, Int32Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (outSwingHigh.buffer === inHighF64.buffer && outSwingHigh.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('FRACTAL openAndFill', 'outSwingHigh', 'prices.high');
  }
  if (outSwingHigh.buffer === inLowF64.buffer && outSwingHigh.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('FRACTAL openAndFill', 'outSwingHigh', 'prices.low');
  }
  if (outSwingLow.buffer === inHighF64.buffer && outSwingLow.byteOffset === inHighF64.byteOffset) {
    throw sharedMemory('FRACTAL openAndFill', 'outSwingLow', 'prices.high');
  }
  if (outSwingLow.buffer === inLowF64.buffer && outSwingLow.byteOffset === inLowF64.byteOffset) {
    throw sharedMemory('FRACTAL openAndFill', 'outSwingLow', 'prices.low');
  }
  if (outSwingHigh.buffer === outSwingLow.buffer && outSwingHigh.byteOffset === outSwingLow.byteOffset) {
    throw sharedMemory('FRACTAL openAndFill', 'outSwingHigh', 'outSwingLow');
  }
  if (guardOutLen > 0 && overlaps(outSwingHigh, inHighF64)) {
    throw sharedMemory('FRACTAL openAndFill', 'outSwingHigh', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outSwingHigh, inLowF64)) {
    throw sharedMemory('FRACTAL openAndFill', 'outSwingHigh', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outSwingLow, inHighF64)) {
    throw sharedMemory('FRACTAL openAndFill', 'outSwingLow', 'prices.high');
  }
  if (guardOutLen > 0 && overlaps(outSwingLow, inLowF64)) {
    throw sharedMemory('FRACTAL openAndFill', 'outSwingLow', 'prices.low');
  }
  if (guardOutLen > 0 && overlaps(outSwingHigh, outSwingLow)) {
    throw sharedMemory('FRACTAL openAndFill', 'outSwingHigh', 'outSwingLow');
  }
  return fractalOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInLeftBars, optInRightBars, new MInteger(), new MInteger(), outSwingHigh, outSwingLow);
}

/**
 * Opens a live FRACTAL stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link fractal} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price series. `low`: Low price series.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link FractalOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `fractalLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range
 *   or histories of different lengths.
 *
 * @example
 * ```ts
 * import { fractalOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = fractalOpen({ high, low }, { leftBars: 2, rightBars: 2 });
 * const peeked = stream.peek(101.4, 99.1); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1); // the bar closes
 * ```
 */
export function fractalOpen(prices: { high: ArrayLike<number>; low: ArrayLike<number> }, options?: FractalOptions): FractalStream {
  requireArgument('FRACTAL open', 'prices', prices);
  const o = optionsOf('FRACTAL open', options);
  const optInLeftBars = orDefault(o.leftBars, Core.INTEGER_DEFAULT);
  const optInRightBars = orDefault(o.rightBars, Core.INTEGER_DEFAULT);
  const core = coreOf('FRACTAL open', o.core);
  return fractalOpenPositional(prices.high, prices.low, optInLeftBars, optInRightBars, core);
}

/**
 * {@link fractalOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link fractal}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `high`: High price series. `low`: Low price series.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link FractalOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link fractal} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `fractalLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { fractalOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const {
 *   stream,
 *   begIdx,
 *   outSwingHigh,
 *   outSwingLow,
 * } = fractalOpenAndFill({ high, low }, { leftBars: 2, rightBars: 2 });
 * // outSwingHigh[k] is the value at bar begIdx + k
 * ```
 */
export function fractalOpenAndFill(prices: { high: ArrayLike<number>; low: ArrayLike<number> }, options?: FractalOptions): FractalResult & { stream: FractalStream } {
  requireArgument('FRACTAL openAndFill', 'prices', prices);
  const o = optionsOf('FRACTAL openAndFill', options);
  const optInLeftBars = orDefault(o.leftBars, Core.INTEGER_DEFAULT);
  const optInRightBars = orDefault(o.rightBars, Core.INTEGER_DEFAULT);
  const core = coreOf('FRACTAL openAndFill', o.core);
  const capacity = fillCapacity(fractalLookbackPositional(optInLeftBars, optInRightBars, core), prices.high, prices.low);
  const outSwingHigh = o.outSwingHigh === undefined ? new Int32Array(capacity) : o.outSwingHigh;
  const outSwingLow = o.outSwingLow === undefined ? new Int32Array(capacity) : o.outSwingLow;
  const stream = fractalOpenAndFillPositional(prices.high, prices.low, optInLeftBars, optInRightBars, outSwingHigh, outSwingLow, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, outSwingHigh: outSwingHigh.subarray(0, range.count), outSwingLow: outSwingLow.subarray(0, range.count) };
}
