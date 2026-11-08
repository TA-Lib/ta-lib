/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  091526 KL     First version (proposal-drafts issue #73).
 *  092226 MF,CC  Binary search above 128 values, one shift per bar (issue #432).
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function medianLookbackPositional(optInTimePeriod: number, _core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 10000) {
    return -1;
  }
  return optInTimePeriod - 1;
}

/** @internal */
export function medianDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (medianLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function medianImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let newValue = 0;
  let oldValue = 0;
  let result = 0;
  let lower = 0;
  let upper = 0;
  let lookbackTotal = 0;
  let outIdx = 0;
  let i = 0;
  let j = 0;
  let pos = 0;
  let nbSorted = 0;
  let lowerIdx = 0;
  let upperIdx = 0;
  let lo = 0;
  let hi = 0;
  let mid = 0;
  let ring!: Float64Array;
  let ring_Idx = 0;
  let maxIdx_ring = (30) - 1;
  let sorted!: Float64Array;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 10000) {
    return RetCode.BAD_PARAM;
  }
  /* The window is carried twice: "ring" by age, "sorted" by value. */
  lookbackTotal = optInTimePeriod - 1;
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  ring = new Float64Array(optInTimePeriod);
  maxIdx_ring = optInTimePeriod - 1;
  ring_Idx = 0;
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sorted = new Float64Array(optInTimePeriod);
  /* Never read: set so two handles opened over the same bars hold the same
   * state.
   */
  sorted[lookbackTotal] = 0.0;
  /* The two central ordinals, zero-based; the same slot at odd n. */
  lowerIdx = (((optInTimePeriod - 1) / 2) | 0);
  upperIdx = ((optInTimePeriod / 2) | 0);
  nbSorted = 0;
  i = startIdx - lookbackTotal;
  while (i < startIdx) {
    newValue = inReal[i];
    if (lookbackTotal < 128) {
      j = nbSorted;
      while (j > 0 && sorted[j - 1] > newValue) {
        sorted[j] = sorted[j - 1];
        j -= 1;
      }
    } else {
      lo = 0;
      hi = nbSorted;
      while (lo < hi) {
        mid = (((lo + hi) / 2) | 0);
        if (sorted[mid] <= newValue) {
          lo = mid + 1;
        } else {
          hi = mid;
        }
      }
      j = nbSorted;
      while (j > lo) {
        sorted[j] = sorted[j - 1];
        j -= 1;
      }
    }
    sorted[j] = newValue;
    nbSorted += 1;
    ring[ring_Idx] = newValue;
    i += 1;
    ring_Idx++;
    if (ring_Idx > maxIdx_ring) {
      ring_Idx = 0;
    }
  }
  /* Both scratch buffers hold copies and inReal is never read below i, so
   * inReal and outReal may be the same buffer.
   *
   * Every buffer store sits BELOW the output store: deriving the whole answer
   * read-only above it is what lets the streaming peek frame drop the state
   * update.
   *
   * Below 128 values a linear scan beats a binary search: one mispredicted
   * loop exit costs less than log2(n) unpredictable halvings.
   */
  outIdx = 0;
  do {
    newValue = inReal[i];
    /* pos counts the retained values <= newValue, so the full window is
     * sorted[0..pos-1], newValue, sorted[pos..].
     */
    if (lookbackTotal < 128) {
      pos = 0;
      while (pos < lookbackTotal && sorted[pos] <= newValue) {
        pos += 1;
      }
    } else {
      lo = 0;
      hi = lookbackTotal;
      while (lo < hi) {
        mid = (((lo + hi) / 2) | 0);
        if (sorted[mid] <= newValue) {
          lo = mid + 1;
        } else {
          hi = mid;
        }
      }
      pos = lo;
    }
    if (lowerIdx < pos) {
      lower = sorted[lowerIdx];
    } else if (lowerIdx === pos) {
      lower = newValue;
    } else {
      lower = sorted[lowerIdx - 1];
    }
    if (upperIdx < pos) {
      upper = sorted[upperIdx];
    } else if (upperIdx === pos) {
      upper = newValue;
    } else {
      upper = sorted[upperIdx - 1];
    }
    /* At odd n both reads are one value; averaging it would overflow above
     * DBL_MAX/2.
     */
    if (lowerIdx === upperIdx) {
      result = lower;
    } else {
      result = (lower + upper) / 2.0;
    }
    outReal[outIdx] = result;
    outIdx += 1;
    ring[ring_Idx] = newValue;
    ring_Idx++;
    if (ring_Idx > maxIdx_ring) {
      ring_Idx = 0;
    }
    oldValue = ring[ring_Idx];
    /* j is the first retained value >= oldValue. Keep every run of equal
     * values in age order (newValue goes after its equals, as above): the
     * oldest of a run is then the departing value bit for bit, which is what
     * keeps -0.0 and 0.0 apart. Inserting before the equals instead changes
     * no value but flips the sign of some zero outputs.
     */
    if (lookbackTotal < 128) {
      j = 0;
      while (j < lookbackTotal && sorted[j] < oldValue) {
        j += 1;
      }
    } else {
      lo = 0;
      hi = lookbackTotal;
      while (lo < hi) {
        mid = (((lo + hi) / 2) | 0);
        if (sorted[mid] < oldValue) {
          lo = mid + 1;
        } else {
          hi = mid;
        }
      }
      j = lo;
    }
    /* Evict oldValue and place newValue with one shift of the slots between
     * them.
     */
    if (j < pos) {
      while (j < pos - 1) {
        sorted[j] = sorted[j + 1];
        j += 1;
      }
      sorted[pos - 1] = newValue;
    } else {
      while (j > pos) {
        sorted[j] = sorted[j - 1];
        j -= 1;
      }
      sorted[pos] = newValue;
    }
    i += 1;
  } while (i <= endIdx);
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 10000, 'in 2..10000']] as const;

/** @internal */
export function medianPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('MEDIAN', 'startIdx', startIdx);
  endIdx = requireInt32('MEDIAN', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('MEDIAN', 'timePeriod', optInTimePeriod);
  requireIndexRange('MEDIAN', startIdx, endIdx);
  const guardLookback = medianLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('MEDIAN', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('MEDIAN', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('MEDIAN', 'real', inReal, guardInLen);
  requireOutput('MEDIAN', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('MEDIAN', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = medianImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('MEDIAN', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link median}, {@link medianLookback}, {@link medianDisplayShift},
 * {@link medianOpen} and {@link medianOpenAndFill}. Every key is optional: an
 * omitted or `undefined` key takes its default, and a call ignores the keys it has
 * no use for.
 */
export interface MedianOptions {
  /**
   * Number of trailing values in the window (default 30; range 2 to 10000;
   * `Core.INTEGER_DEFAULT` selects the default).
   */
  timePeriod?: number | undefined;
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
  outReal?: Float64Array<ArrayBuffer> | undefined;
}

/** What {@link median} returns, and {@link medianOpenAndFill} with its stream. */
export interface MedianResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Median of the trailing window. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link median} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MedianOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function medianLookback(options?: MedianOptions): number {
  const o = optionsOf('MEDIAN lookback', options);
  return medianLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('MEDIAN lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link median}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MedianOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function medianDisplayShift(outputIdx: number, options?: MedianOptions): number {
  const o = optionsOf('MEDIAN displayShift', options);
  return medianDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('MEDIAN displayShift', o.core));
}

/**
 * The middle order statistic of the trailing window: the central value when
 * `timePeriod` is odd, the mean of the two central values when it is even. A
 * robust measure of central tendency — unlike
 * [`SMA`](https://ta-lib.org/functions/sma), a single spike moves it by at most
 * one rank however large the spike is, which is what makes it useful as a filter
 * rather than as a level. Not to be confused with
 * [`MEDPRICE`](https://ta-lib.org/functions/medprice), which is `(High + Low) / 2`
 * of one bar and is not an order statistic.
 *
 * Formula and more info at
 * [ta-lib.org/functions/median](https://ta-lib.org/functions/median).
 *
 * **Notes**
 * - This is not [`PERCENTILE`](https://ta-lib.org/functions/percentile) at 50.
 *   `PERCENTILE` reports the nearest rank, which at even `n` selects the **lower**
 *   of the two central values: on a 4-bar window of `1, 2, 3, 4` this function
 *   returns `2.5` and `PERCENTILE` returns `2`. At odd `n` the two agree bit for
 *   bit.
 * - At even `n` the output can therefore be a value the series never traded at,
 *   which is the deliberate opposite of `PERCENTILE`'s design property.
 * - `timePeriod` is not restricted to odd values: TA-Lib has no odd-only range
 *   mechanism, and it would surprise any caller reaching for a 20-bar median.
 * - Every input value must be finite. A NaN makes every comparison against it
 *   false, which breaks the order the window is kept in, and the output can stay
 *   wrong long after the NaN has left the window.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link medianLookback} is a success with empty outputs.
 *
 * @param real - The series to take the median of.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link MedianOptions}.
 * @returns `begIdx` and the values; see {@link MedianResult}. Each output is a
 *   view of its buffer, the caller's or a new one.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for options that are not an object, a
 *   `core` that is not a `Core`, an absent input, a range bound that is not an
 *   int32, an integer parameter that is not an int32, a parameter outside its range,
 *   an input too short for the range, a buffer that is empty, too short or of the
 *   wrong type or a buffer sharing memory with a `Float64Array` input it is not.
 *   Checked before anything is written.
 *
 * @example
 * ```ts
 * import { median } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = median(data, { timePeriod: 30 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`percentile`](https://ta-lib.org/functions/percentile)
 * @see [`sma`](https://ta-lib.org/functions/sma)
 * @see [`medprice`](https://ta-lib.org/functions/medprice)
 */
export function median(real: ArrayLike<number>, options?: MedianOptions): MedianResult {
  const o = optionsOf('MEDIAN', options);
  if (o.endIdx === undefined) {
    requireSeries('MEDIAN', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MEDIAN', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, medianLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = medianPositional(startIdx, endIdx, real, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live MEDIAN stream: one value per closed bar, bit-identical to {@link median}
 * over the same series. Open it with {@link medianOpen} or
 * {@link medianOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/median](https://ta-lib.org/functions/median).
 */
export class MedianStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  lookbackTotal: number = 0;
  /** @internal */
  lowerIdx: number = 0;
  /** @internal */
  upperIdx: number = 0;
  /** @internal */
  ring_Idx: number = 0;
  /** @internal */
  maxIdx_ring: number = 0;
  /** @internal */
  sorted_Idx: number = 0;
  /** @internal */
  maxIdx_sorted: number = 0;
  /** @internal */
  cbSize_ring: number = 0;
  /** @internal */
  cb_ring: Float64Array = new Float64Array(1);
  /** @internal */
  cbSize_sorted: number = 0;
  /** @internal */
  cb_sorted: Float64Array = new Float64Array(1);
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
  static of(core: Core): MedianStream {
    return new MedianStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link median} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('MEDIAN advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: The series to take the median of.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('MEDIAN update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('MEDIAN update', 'real');
    }
    medianStepImpl(this, real);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: The series to take the median of.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number): number {
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('MEDIAN peek', 'real');
    }
    const sp = this;
    const inReal = real;
    let newValue = 0;
    let result = 0;
    let lower = 0;
    let upper = 0;
    let pos = 0;
    let lo = 0;
    let hi = 0;
    let mid = 0;
    let cur_outReal = 0;
    newValue = inReal;
    /* pos counts the retained values <= newValue, so the full window is
     * sorted[0..pos-1], newValue, sorted[pos..].
     */
    if (sp.lookbackTotal < 128) {
      pos = 0;
      while (pos < sp.lookbackTotal && sp.cb_sorted[pos] <= newValue) {
        pos += 1;
      }
    } else {
      lo = 0;
      hi = sp.lookbackTotal;
      while (lo < hi) {
        mid = (((lo + hi) / 2) | 0);
        if (sp.cb_sorted[mid] <= newValue) {
          lo = mid + 1;
        } else {
          hi = mid;
        }
      }
      pos = lo;
    }
    if (sp.lowerIdx < pos) {
      lower = sp.cb_sorted[sp.lowerIdx];
    } else if (sp.lowerIdx === pos) {
      lower = newValue;
    } else {
      lower = sp.cb_sorted[sp.lowerIdx - 1];
    }
    if (sp.upperIdx < pos) {
      upper = sp.cb_sorted[sp.upperIdx];
    } else if (sp.upperIdx === pos) {
      upper = newValue;
    } else {
      upper = sp.cb_sorted[sp.upperIdx - 1];
    }
    /* At odd n both reads are one value; averaging it would overflow above
     * DBL_MAX/2.
     */
    if (sp.lowerIdx === sp.upperIdx) {
      result = lower;
    } else {
      result = (lower + upper) / 2.0;
    }
    cur_outReal = result;
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
  clone(): MedianStream {
    const c = new MedianStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.lookbackTotal = this.lookbackTotal;
    c.lowerIdx = this.lowerIdx;
    c.upperIdx = this.upperIdx;
    c.ring_Idx = this.ring_Idx;
    c.maxIdx_ring = this.maxIdx_ring;
    c.sorted_Idx = this.sorted_Idx;
    c.maxIdx_sorted = this.maxIdx_sorted;
    c.cbSize_ring = this.cbSize_ring;
    c.cb_ring = this.cb_ring.slice();
    c.cbSize_sorted = this.cbSize_sorted;
    c.cb_sorted = this.cb_sorted.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function medianStepImpl(sp: MedianStream, inReal: number): void {
  let newValue = 0;
  let oldValue = 0;
  let result = 0;
  let lower = 0;
  let upper = 0;
  let j = 0;
  let pos = 0;
  let lo = 0;
  let hi = 0;
  let mid = 0;
  newValue = inReal;
  /* pos counts the retained values <= newValue, so the full window is
   * sorted[0..pos-1], newValue, sorted[pos..].
   */
  if (sp.lookbackTotal < 128) {
    pos = 0;
    while (pos < sp.lookbackTotal && sp.cb_sorted[pos] <= newValue) {
      pos += 1;
    }
  } else {
    lo = 0;
    hi = sp.lookbackTotal;
    while (lo < hi) {
      mid = (((lo + hi) / 2) | 0);
      if (sp.cb_sorted[mid] <= newValue) {
        lo = mid + 1;
      } else {
        hi = mid;
      }
    }
    pos = lo;
  }
  if (sp.lowerIdx < pos) {
    lower = sp.cb_sorted[sp.lowerIdx];
  } else if (sp.lowerIdx === pos) {
    lower = newValue;
  } else {
    lower = sp.cb_sorted[sp.lowerIdx - 1];
  }
  if (sp.upperIdx < pos) {
    upper = sp.cb_sorted[sp.upperIdx];
  } else if (sp.upperIdx === pos) {
    upper = newValue;
  } else {
    upper = sp.cb_sorted[sp.upperIdx - 1];
  }
  /* At odd n both reads are one value; averaging it would overflow above
   * DBL_MAX/2.
   */
  if (sp.lowerIdx === sp.upperIdx) {
    result = lower;
  } else {
    result = (lower + upper) / 2.0;
  }
  sp.cur_outReal = result;
  sp.cb_ring[sp.ring_Idx] = newValue;
  sp.ring_Idx = sp.ring_Idx + 1;
  if (sp.ring_Idx > sp.maxIdx_ring) {
    sp.ring_Idx = 0;
  }
  oldValue = sp.cb_ring[sp.ring_Idx];
  /* j is the first retained value >= oldValue. Keep every run of equal
   * values in age order (newValue goes after its equals, as above): the
   * oldest of a run is then the departing value bit for bit, which is what
   * keeps -0.0 and 0.0 apart. Inserting before the equals instead changes
   * no value but flips the sign of some zero outputs.
   */
  if (sp.lookbackTotal < 128) {
    j = 0;
    while (j < sp.lookbackTotal && sp.cb_sorted[j] < oldValue) {
      j += 1;
    }
  } else {
    lo = 0;
    hi = sp.lookbackTotal;
    while (lo < hi) {
      mid = (((lo + hi) / 2) | 0);
      if (sp.cb_sorted[mid] < oldValue) {
        lo = mid + 1;
      } else {
        hi = mid;
      }
    }
    j = lo;
  }
  /* Evict oldValue and place newValue with one shift of the slots between
   * them.
   */
  if (j < pos) {
    while (j < pos - 1) {
      sp.cb_sorted[j] = sp.cb_sorted[j + 1];
      j += 1;
    }
    sp.cb_sorted[pos - 1] = newValue;
  } else {
    while (j > pos) {
      sp.cb_sorted[j] = sp.cb_sorted[j - 1];
      j -= 1;
    }
    sp.cb_sorted[pos] = newValue;
  }
}

function medianOpenImpl(
  sp: MedianStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let newValue = 0;
  let oldValue = 0;
  let result = 0;
  let lower = 0;
  let upper = 0;
  let lookbackTotal = 0;
  let outIdx = 0;
  let i = 0;
  let j = 0;
  let pos = 0;
  let nbSorted = 0;
  let lowerIdx = 0;
  let upperIdx = 0;
  let lo = 0;
  let hi = 0;
  let mid = 0;
  let ring!: Float64Array;
  let ring_Idx = 0;
  let maxIdx_ring = (30) - 1;
  let sorted!: Float64Array;
  let sorted_Idx = 0;
  let maxIdx_sorted = (30) - 1;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 10000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* The window is carried twice: "ring" by age, "sorted" by value. */
  lookbackTotal = optInTimePeriod - 1;
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  ring = new Float64Array(optInTimePeriod);
  maxIdx_ring = optInTimePeriod - 1;
  ring_Idx = 0;
  if (optInTimePeriod < 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sorted = new Float64Array(optInTimePeriod);
  maxIdx_sorted = optInTimePeriod - 1;
  sorted_Idx = 0;
  /* Never read: set so two handles opened over the same bars hold the same
   * state.
   */
  sorted[lookbackTotal] = 0.0;
  /* The two central ordinals, zero-based; the same slot at odd n. */
  lowerIdx = (((optInTimePeriod - 1) / 2) | 0);
  upperIdx = ((optInTimePeriod / 2) | 0);
  nbSorted = 0;
  i = startIdx - lookbackTotal;
  while (i < startIdx) {
    newValue = inReal[i];
    if (lookbackTotal < 128) {
      j = nbSorted;
      while (j > 0 && sorted[j - 1] > newValue) {
        sorted[j] = sorted[j - 1];
        j -= 1;
      }
    } else {
      lo = 0;
      hi = nbSorted;
      while (lo < hi) {
        mid = (((lo + hi) / 2) | 0);
        if (sorted[mid] <= newValue) {
          lo = mid + 1;
        } else {
          hi = mid;
        }
      }
      j = nbSorted;
      while (j > lo) {
        sorted[j] = sorted[j - 1];
        j -= 1;
      }
    }
    sorted[j] = newValue;
    nbSorted += 1;
    ring[ring_Idx] = newValue;
    i += 1;
    ring_Idx++;
    if (ring_Idx > maxIdx_ring) {
      ring_Idx = 0;
    }
  }
  /* Both scratch buffers hold copies and inReal is never read below i, so
   * inReal and outReal may be the same buffer.
   *
   * Every buffer store sits BELOW the output store: deriving the whole answer
   * read-only above it is what lets the streaming peek frame drop the state
   * update.
   *
   * Below 128 values a linear scan beats a binary search: one mispredicted
   * loop exit costs less than log2(n) unpredictable halvings.
   */
  outIdx = 0;
  do {
    newValue = inReal[i];
    /* pos counts the retained values <= newValue, so the full window is
     * sorted[0..pos-1], newValue, sorted[pos..].
     */
    if (lookbackTotal < 128) {
      pos = 0;
      while (pos < lookbackTotal && sorted[pos] <= newValue) {
        pos += 1;
      }
    } else {
      lo = 0;
      hi = lookbackTotal;
      while (lo < hi) {
        mid = (((lo + hi) / 2) | 0);
        if (sorted[mid] <= newValue) {
          lo = mid + 1;
        } else {
          hi = mid;
        }
      }
      pos = lo;
    }
    if (lowerIdx < pos) {
      lower = sorted[lowerIdx];
    } else if (lowerIdx === pos) {
      lower = newValue;
    } else {
      lower = sorted[lowerIdx - 1];
    }
    if (upperIdx < pos) {
      upper = sorted[upperIdx];
    } else if (upperIdx === pos) {
      upper = newValue;
    } else {
      upper = sorted[upperIdx - 1];
    }
    /* At odd n both reads are one value; averaging it would overflow above
     * DBL_MAX/2.
     */
    if (lowerIdx === upperIdx) {
      result = lower;
    } else {
      result = (lower + upper) / 2.0;
    }
    outReal[((outIdx * outStride) | 0)] = result;
    outIdx += 1;
    ring[ring_Idx] = newValue;
    ring_Idx++;
    if (ring_Idx > maxIdx_ring) {
      ring_Idx = 0;
    }
    oldValue = ring[ring_Idx];
    /* j is the first retained value >= oldValue. Keep every run of equal
     * values in age order (newValue goes after its equals, as above): the
     * oldest of a run is then the departing value bit for bit, which is what
     * keeps -0.0 and 0.0 apart. Inserting before the equals instead changes
     * no value but flips the sign of some zero outputs.
     */
    if (lookbackTotal < 128) {
      j = 0;
      while (j < lookbackTotal && sorted[j] < oldValue) {
        j += 1;
      }
    } else {
      lo = 0;
      hi = lookbackTotal;
      while (lo < hi) {
        mid = (((lo + hi) / 2) | 0);
        if (sorted[mid] < oldValue) {
          lo = mid + 1;
        } else {
          hi = mid;
        }
      }
      j = lo;
    }
    /* Evict oldValue and place newValue with one shift of the slots between
     * them.
     */
    if (j < pos) {
      while (j < pos - 1) {
        sorted[j] = sorted[j + 1];
        j += 1;
      }
      sorted[pos - 1] = newValue;
    } else {
      while (j > pos) {
        sorted[j] = sorted[j - 1];
        j -= 1;
      }
      sorted[pos] = newValue;
    }
    i += 1;
  } while (i <= endIdx);
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const capCb_ring = maxIdx_ring + 1;
  if (capCb_ring > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  const capCb_sorted = maxIdx_sorted + 1;
  if (capCb_sorted > historyLen + 1) {
    return RetCode.INTERNAL_ERROR;
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.lookbackTotal = lookbackTotal;
  sp.lowerIdx = lowerIdx;
  sp.upperIdx = upperIdx;
  sp.ring_Idx = ring_Idx;
  sp.maxIdx_ring = maxIdx_ring;
  sp.sorted_Idx = sorted_Idx;
  sp.maxIdx_sorted = maxIdx_sorted;
  sp.cbSize_ring = capCb_ring;
  sp.cb_ring = ring;
  sp.cbSize_sorted = capCb_sorted;
  sp.cb_sorted = sorted;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function medianOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): MedianStream {
  const sp = MedianStream.of(core);
  const retCode = medianOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MEDIAN openAndFill', inReal.length, startIdx, medianLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && medianLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('MEDIAN openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('MEDIAN openAndFill', retCode);
}

/** @internal */
export function medianOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): MedianStream {
  const sp = MedianStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = medianOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('MEDIAN open', inReal.length, startIdx, medianLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && medianLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('MEDIAN open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('MEDIAN open', retCode);
}

/** @internal */
export function medianOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): MedianStream {
  optInTimePeriod = requireInt32('MEDIAN open', 'timePeriod', optInTimePeriod);
  requireArgument('MEDIAN open', 'real', inReal);
  requireHistory('MEDIAN open', 'real', inReal.length);
  return medianOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/** @internal */
export function medianOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): MedianStream {
  optInTimePeriod = requireInt32('MEDIAN openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('MEDIAN openAndFill', 'real', inReal);
  requireHistory('MEDIAN openAndFill', 'real', inReal.length);
  const guardLookback = medianLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('MEDIAN openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('MEDIAN openAndFill', inReal.length, guardLookback);
  requireOutput('MEDIAN openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('MEDIAN openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('MEDIAN openAndFill', 'outReal', 'real');
  }
  return medianOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live MEDIAN stream over the warm-up history: `value()` starts at the
 * last history bar, bit-identical to {@link median} there.
 *
 * @param real - Warm-up history. The series to take the median of.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link MedianOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `medianLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32 or a parameter outside its
 *   range.
 *
 * @example
 * ```ts
 * import { medianOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = medianOpen(data, { timePeriod: 30 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function medianOpen(real: ArrayLike<number>, options?: MedianOptions): MedianStream {
  const o = optionsOf('MEDIAN open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MEDIAN open', o.core);
  return medianOpenPositional(real, optInTimePeriod, core);
}

/**
 * {@link medianOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link median}.
 *
 * @param real - Warm-up history. The series to take the median of.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link MedianOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link median} returns them; the range is the
 *   stream's `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `medianLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range
 *   or a buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { medianOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = medianOpenAndFill(data, { timePeriod: 30 });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function medianOpenAndFill(real: ArrayLike<number>, options?: MedianOptions): MedianResult & { stream: MedianStream } {
  const o = optionsOf('MEDIAN openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('MEDIAN openAndFill', o.core);
  const capacity = fillCapacity(medianLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = medianOpenAndFillPositional(real, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
