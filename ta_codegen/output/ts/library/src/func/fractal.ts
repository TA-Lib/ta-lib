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

import { badSink, clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link fractal} consumes before its first value:
 * the index of the first bar with a value when the whole series is requested.
 *
 * @param optInLeftBars - Bars before the pivot that it must strictly dominate
 *   (default 2; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInRightBars - Bars after the pivot that it must strictly dominate, and
 *   the delay before the verdict is reported (default 2; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function fractalLookback(optInLeftBars: number, optInRightBars: number, core: Core = Core.DEFAULT): number {
  void core;
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

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link fractal}.
 *
 * The values are never shifted: this describes the drawing only.
 *
 * @param optInLeftBars - Bars before the pivot that it must strictly dominate
 *   (default 2; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInRightBars - Bars after the pivot that it must strictly dominate, and
 *   the delay before the verdict is reported (default 2; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function fractalDisplayShift(optInLeftBars: number, optInRightBars: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (fractalLookback(optInLeftBars, optInRightBars, core) < 0) {
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
  lookbackTotal = fractalLookback(optInLeftBars, optInRightBars, core);
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

/**
 * Williams Fractal: a causal swing-pivot detector. A bar is a swing high when its
 * high strictly exceeds the highs of the `optInLeftBars` bars before it and the
 * `optInRightBars` bars after it; a swing low is the mirror on the lows. Bill
 * Williams' original is the symmetric five-candle case; independent left and right
 * arms generalise it. The right arm cannot be known until it has closed, so the
 * value is written on the bar that completes the pivot, `optInRightBars` bars
 * after the pivot itself. Each output value therefore describes the bar
 * `optInRightBars` back, not the bar it is written at: a flag at output index `k`
 * names input bar `outBegIdx + k - optInRightBars`, whose price is `inHigh[...]` /
 * `inLow[...]` at that index. The display shift both outputs report,
 * `-optInRightBars`, is that offset: a chart draws each flag that many bars to the
 * left. The two outputs are independent flags rather than one signed value,
 * because an outside bar can be a swing high and a swing low at once.
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
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link fractalLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price series.
 * @param inLow - Low price series.
 * @param optInLeftBars - Bars before the pivot that it must strictly dominate
 *   (default 2; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInRightBars - Bars after the pivot that it must strictly dominate, and
 *   the delay before the verdict is reported (default 2; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param outSwingHigh - 100 when the bar `optInRightBars` back is a strict swing
 *   high, 0 otherwise. Must hold the values produced and never be empty; the input
 *   length always suffices.
 * @param outSwingLow - 100 when the bar `optInRightBars` back is a strict swing
 *   low, 0 otherwise. Must hold the values produced and never be empty; the input
 *   length always suffices.
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
 * import { fractal } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const swingHigh = new Int32Array(high.length);
 * const swingLow = new Int32Array(high.length);
 * const { begIdx, count } = fractal(
 *   0, high.length - 1,
 *   high, low,
 *   2, 2,
 *   swingHigh, swingLow,
 * );
 * // swingHigh[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`aroon`](https://ta-lib.org/functions/aroon)
 * @see [`maxindex`](https://ta-lib.org/functions/maxindex)
 * @see [`minindex`](https://ta-lib.org/functions/minindex)
 * @see [`minmaxindex`](https://ta-lib.org/functions/minmaxindex)
 */
export function fractal(
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
  optInLeftBars = requireInt32('FRACTAL', 'optInLeftBars', optInLeftBars);
  optInRightBars = requireInt32('FRACTAL', 'optInRightBars', optInRightBars);
  requireIndexRange('FRACTAL', startIdx, endIdx);
  const guardStart = clampedStart('FRACTAL', startIdx, fractalLookback(optInLeftBars, optInRightBars, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('FRACTAL', 'inHigh', inHigh, guardInLen);
  requireLength('FRACTAL', 'inLow', inLow, guardInLen);
  requireOutput('FRACTAL', 'outSwingHigh', outSwingHigh, guardOutLen, Int32Array);
  requireOutput('FRACTAL', 'outSwingLow', outSwingLow, guardOutLen, Int32Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlaps(outSwingHigh, inHighF64) || overlaps(outSwingHigh, inLowF64) || overlaps(outSwingLow, inHighF64) || overlaps(outSwingLow, inLowF64) || overlaps(outSwingHigh, outSwingLow)) {
    throw failure('FRACTAL', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = fractalImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInLeftBars, optInRightBars, outBegIdx, outNBElement, outSwingHigh, outSwingLow);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('FRACTAL', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
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
      throw failure('FRACTAL advance', RetCode.OUT_OF_RANGE_END_INDEX);
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
  update(inHigh: number, inLow: number, out: FractalOut): void {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('FRACTAL update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (typeof out !== 'object' || out === null) {
      throw badSink('FRACTAL update', 'FractalOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('FRACTAL update', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    fractalStepImpl(this, inHigh, inLow);
    this.outRangeCount++;
    out.swingHigh = this.cur_outSwingHigh;
    out.swingLow = this.cur_outSwingLow;
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
  peek(inHigh: number, inLow: number, out: FractalOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('FRACTAL peek', 'FractalOut');
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('FRACTAL peek', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    const sp = this;
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
    out.swingHigh = cur_outSwingHigh;
    out.swingLow = cur_outSwingLow;
  }

  /**
   * The value at the last bar this stream counted; `peek` does not change it.
   *
   * @param out - Receives the outputs.
   */
  value(out: FractalOut): void {
    if (typeof out !== 'object' || out === null) {
      throw badSink('FRACTAL value', 'FractalOut');
    }
    out.swingHigh = this.cur_outSwingHigh;
    out.swingLow = this.cur_outSwingLow;
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
 * The outputs of one FRACTAL bar, written into an object the caller owns and
 * reuses: valid until the next call that writes it.
 */
export class FractalOut {
  /** 100 when the bar `optInRightBars` back is a strict swing high, 0 otherwise. */
  swingHigh: number = 0;
  /** 100 when the bar `optInRightBars` back is a strict swing low, 0 otherwise. */
  swingLow: number = 0;
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
  if (historyLen < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
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
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = fractalLookback(optInLeftBars, optInRightBars, core);
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
    throw insufficientHistory('FRACTAL openAndFill', inHigh.length, startIdx, fractalLookback(optInLeftBars, optInRightBars, core));
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
    throw insufficientHistory('FRACTAL open', inHigh.length, startIdx, fractalLookback(optInLeftBars, optInRightBars, core));
  }
  throw streamFailure('FRACTAL open', retCode);
}

/**
 * Opens a live FRACTAL stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link fractal} there.
 *
 * @param inHigh - Warm-up history. High price series. Every history has the same
 *   length.
 * @param inLow - Warm-up history. Low price series. Every history has the same
 *   length.
 * @param optInLeftBars - Bars before the pivot that it must strictly dominate
 *   (default 2; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInRightBars - Bars after the pivot that it must strictly dominate, and
 *   the delay before the verdict is reported (default 2; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `fractalLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for an integer parameter that is not an int32, a parameter outside its range or
 *   histories of different lengths.
 *
 * @example
 * ```ts
 * import { FractalOut, fractalOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = fractalOpen(high, low, 2, 2);
 * const peeked = new FractalOut();
 * const updated = new FractalOut();
 * stream.peek(101.4, 99.1, peeked); // a forming bar: commits nothing
 * stream.update(101.4, 99.1, updated); // the bar closes
 * ```
 */
export function fractalOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInLeftBars: number,
  optInRightBars: number,
  core: Core = Core.DEFAULT,
): FractalStream {
  optInLeftBars = requireInt32('FRACTAL open', 'optInLeftBars', optInLeftBars);
  optInRightBars = requireInt32('FRACTAL open', 'optInRightBars', optInRightBars);
  requireArgument('FRACTAL open', 'inHigh', inHigh);
  requireHistory('FRACTAL open', inHigh.length);
  requireArgument('FRACTAL open', 'inLow', inLow);
  requireHistoryLength('FRACTAL open', 'inLow', inLow.length, inHigh.length);
  return fractalOpenInternal(core, float64(inHigh), float64(inLow), 0, optInLeftBars, optInRightBars);
}

/**
 * {@link fractalOpen} that also fills the outputs over the whole history in the
 * same pass, bit-identically to {@link fractal}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price series. Every history has the same
 *   length.
 * @param inLow - Warm-up history. Low price series. Every history has the same
 *   length.
 * @param optInLeftBars - Bars before the pivot that it must strictly dominate
 *   (default 2; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param optInRightBars - Bars after the pivot that it must strictly dominate, and
 *   the delay before the verdict is reported (default 2; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param outSwingHigh - 100 when the bar `optInRightBars` back is a strict swing
 *   high, 0 otherwise. Must hold `historyLen - lookback` values and share no memory
 *   with an input or another output.
 * @param outSwingLow - 100 when the bar `optInRightBars` back is a strict swing
 *   low, 0 otherwise. Must hold `historyLen - lookback` values and share no memory
 *   with an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `fractalLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or an output too short or starting where another
 *   output or a `Float64Array` input starts, or, on a history long enough to open,
 *   one sharing memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { fractalOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const swingHigh = new Int32Array(high.length);
 * const swingLow = new Int32Array(high.length);
 * const stream = fractalOpenAndFill(high, low, 2, 2, swingHigh, swingLow);
 * const { begIdx, count } = stream.outRange();
 * // swingHigh[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function fractalOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInLeftBars: number,
  optInRightBars: number,
  outSwingHigh: Int32Array,
  outSwingLow: Int32Array,
  core: Core = Core.DEFAULT,
): FractalStream {
  optInLeftBars = requireInt32('FRACTAL openAndFill', 'optInLeftBars', optInLeftBars);
  optInRightBars = requireInt32('FRACTAL openAndFill', 'optInRightBars', optInRightBars);
  requireArgument('FRACTAL openAndFill', 'inHigh', inHigh);
  requireHistory('FRACTAL openAndFill', inHigh.length);
  requireArgument('FRACTAL openAndFill', 'inLow', inLow);
  const guardOutLen = openFillCount('FRACTAL openAndFill', inHigh.length, fractalLookback(optInLeftBars, optInRightBars, core));
  requireHistoryLength('FRACTAL openAndFill', 'inLow', inLow.length, inHigh.length);
  requireOutput('FRACTAL openAndFill', 'outSwingHigh', outSwingHigh, guardOutLen, Int32Array);
  requireOutput('FRACTAL openAndFill', 'outSwingLow', outSwingLow, guardOutLen, Int32Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if ((outSwingHigh.buffer === inHighF64.buffer && outSwingHigh.byteOffset === inHighF64.byteOffset) || (outSwingHigh.buffer === inLowF64.buffer && outSwingHigh.byteOffset === inLowF64.byteOffset) || (outSwingLow.buffer === inHighF64.buffer && outSwingLow.byteOffset === inHighF64.byteOffset) || (outSwingLow.buffer === inLowF64.buffer && outSwingLow.byteOffset === inLowF64.byteOffset) || (outSwingHigh.buffer === outSwingLow.buffer && outSwingHigh.byteOffset === outSwingLow.byteOffset)) {
    throw streamFailure('FRACTAL openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outSwingHigh, inHighF64) || overlaps(outSwingHigh, inLowF64) || overlaps(outSwingLow, inHighF64) || overlaps(outSwingLow, inLowF64) || overlaps(outSwingHigh, outSwingLow))) {
    throw streamFailure('FRACTAL openAndFill', RetCode.BAD_PARAM);
  }
  return fractalOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInLeftBars, optInRightBars, new MInteger(), new MInteger(), outSwingHigh, outSwingLow);
}
