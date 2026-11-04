/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin (@kevinlincg)
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  081226 KL   Initial version (#206).
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { emaLookback } from './ema.ts';

/**
 * Number of leading input bars {@link efi} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - EMA period applied to the force series (default 13;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function efiLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 13;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  /* One bar forms the first close-to-close change. */
  return 1 + emaLookback(optInTimePeriod, core);
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link efi}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - EMA period applied to the force series (default 13;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function efiDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (efiLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function efiImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inClose: Float64Array,
  inVolume: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let emaBeta = 0;
  let optInK_1 = 0;
  let tempReal = 0;
  let prevMA = 0;
  let prevClose = 0;
  let force = 0;
  let i = 0;
  let today = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 13;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  /* Alexander Elder's Force Index (Trading for a Living, 1993): the one-bar
   * close-to-close move weighted by that bar's volume, then smoothed with an
   * EMA. Elder's 2-period reading is the short-term form and 13 the
   * intermediate-term one -- that is the parameter, not a second formula.
   *
   *    force[t] = ( close[t] - close[t-1] ) * volume[t]
   *    EFI      = EMA( force, optInTimePeriod )
   *
   * The arithmetic below is ema.c's with inReal[t] replaced by force[t], kept
   * in exactly that shape on purpose: the seed accumulates from 0.0 in the
   * same order, and the recurrence is k*x + beta*prevMA with ema.c's k and
   * beta. That order IS the bit-exactness contract against the composed
   * reference in test_composite.c -- MOM, then MULT, then EMA -- so do not
   * tidy it. TRIX carries the same warning.
   *
   * Nothing on the data path divides by an input, so issue #112 is satisfied
   * structurally: a flat close gives force exactly 0.0 and output exactly
   * 0.0, and zero volume likewise. The only division is by the period, a
   * positive integer parameter.
   *
   * prevClose is carried in a scalar rather than re-read from inClose[t-1]
   * because the C API allows outReal to alias an input: at bar t the slot
   * holding close[t-1] may already have been overwritten by the output
   * written a bar earlier. cmou.c carries its trailing value for the same
   * reason.
   */
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = efiLookback(optInTimePeriod, core);
  /* After the lookback call: a double live across a call is saved and
   * restored around every fma call of the loops below, one more instruction
   * per bar.
   */
  emaBeta = (optInTimePeriod - 1) / (optInTimePeriod + 1);
  optInK_1 = 1.0 - emaBeta;
  if (emaBeta < 0.5) {
    emaBeta = 1.0 - optInK_1;
  }
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
  /* No smoothing at a period of 1: the output is the raw Force Index.
   * Explicit because the recursion, at a k of 1.0 and a beta of 0.0, does
   * not keep the sign of a -0.0 force: a down bar on zero volume.
   */
  if (optInTimePeriod === 1) {
    outBegIdx.value = startIdx;
    outIdx = 0;
    today = startIdx;
    prevClose = inClose[today - 1];
    while (today <= endIdx) {
      force = (inClose[today] - prevClose) * inVolume[today];
      prevClose = inClose[today];
      outReal[outIdx] = force;
      outIdx = outIdx + 1;
      today = today + 1;
    }
    outNBElement.value = outIdx;
    return RetCode.SUCCESS;
  }
  outBegIdx.value = startIdx;
  /* The first EMA value is a simple average of the first 'period' force
   * values; it then seeds the recursion. This is ema.c's seeding applied
   * to the force series rather than to the input array.
   */
  today = startIdx - lookbackTotal + 1;
  prevClose = inClose[today - 1];
  i = optInTimePeriod;
  tempReal = 0.0;
  while (i-- > 0) {
    force = (inClose[today] - prevClose) * inVolume[today];
    prevClose = inClose[today];
    tempReal += force;
    today = today + 1;
  }
  prevMA = tempReal / optInTimePeriod;
  while (today <= startIdx) {
    force = (inClose[today] - prevClose) * inVolume[today];
    prevClose = inClose[today];
    prevMA = fma(emaBeta, prevMA, optInK_1 * force);
    today = today + 1;
  }
  outReal[0] = prevMA;
  outIdx = 1;
  while (today <= endIdx) {
    force = (inClose[today] - prevClose) * inVolume[today];
    prevClose = inClose[today];
    prevMA = fma(emaBeta, prevMA, optInK_1 * force);
    outReal[outIdx] = prevMA;
    outIdx = outIdx + 1;
    today = today + 1;
  }
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * Alexander Elder's Force Index (*Trading for a Living*, 1993): volume-weighted
 * momentum. Each bar's close-to-close move is weighted by that bar's volume, and
 * the result is smoothed with an exponential moving average. The sign is the
 * direction of the move; the size combines how far price travelled with how much
 * volume stood behind it. Elder reads two settings — 2 for the short term, which
 * he pairs with a 22-period EMA of price to mark corrections against an
 * established trend, and 13 for the intermediate term, the default here. A
 * divergence against price can be confirmed by a zero-line cross. Beyond Elder,
 * much longer settings are also in use, 100 or so, for the longer-term balance
 * between buyers and sellers. Nothing normalises the result, so it scales with the
 * instrument's own volume: read its sign and its shape over time, not its level
 * against another instrument.
 *
 * Formula and more info at
 * [ta-lib.org/functions/efi](https://ta-lib.org/functions/efi).
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link efiLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inClose - Close price of each bar.
 * @param inVolume - Volume of each bar.
 * @param optInTimePeriod - EMA period applied to the force series (default 13;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Smoothed force. Must hold the values produced and never be
 *   empty; the input length always suffices.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The range written: `begIdx` is the first bar with a value, `count` how
 *   many were written.
 * @throws `TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or
 *   `TA_OUT_OF_RANGE_END_INDEX` for a range outside `0..Core.INDEX_MAX` or ending
 *   before it starts, and `TA_BAD_PARAM` for an argument that is not an int32, a
 *   parameter outside its range, an array too short for the range, an empty output
 *   or an output sharing memory with a `Float64Array` input it is not. Checked
 *   before anything is written.
 *
 * @example
 * ```ts
 * import { efi } from 'ta-lib';
 *
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const out = new Float64Array(close.length);
 * const { begIdx, count } = efi(0, close.length - 1, close, volume, 13, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`ad`](https://ta-lib.org/functions/ad)
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`mfi`](https://ta-lib.org/functions/mfi)
 * @see [`obv`](https://ta-lib.org/functions/obv)
 * @see [`pvo`](https://ta-lib.org/functions/pvo)
 */
export function efi(
  startIdx: number,
  endIdx: number,
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('EFI', 'startIdx', startIdx);
  endIdx = requireInt32('EFI', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('EFI', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('EFI', startIdx, endIdx);
  const guardStart = clampedStart('EFI', startIdx, efiLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('EFI', 'inClose', inClose, guardInLen);
  requireLength('EFI', 'inVolume', inVolume, guardInLen);
  requireOutput('EFI', 'outReal', outReal, guardOutLen, Float64Array);
  const inCloseF64 = float64(inClose);
  const inVolumeF64 = float64(inVolume);
  if (overlapsPartly(outReal, inCloseF64) || overlapsPartly(outReal, inVolumeF64)) {
    throw failure('EFI', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = efiImpl(core, startIdx, endIdx, inCloseF64, inVolumeF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('EFI', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live EFI stream: one value per closed bar, bit-identical to {@link efi} over
 * the same series. Open it with {@link efiOpen} or {@link efiOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/efi](https://ta-lib.org/functions/efi).
 */
export class EfiStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  prevClose: number = 0;
  /** @internal */
  emaBeta: number = 0;
  /** @internal */
  optInK_1: number = 0;
  /** @internal */
  prevMA: number = 0;
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
  static of(core: Core): EfiStream {
    return new EfiStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link efi} reports them over the same bars. Each accepted `update` and each
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
      throw failure('EFI advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param inVolume - The bar's `inVolume` value: Volume of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inClose: number, inVolume: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('EFI update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inClose) || !Number.isFinite(inVolume)) {
      throw nonFiniteBar('EFI update', !Number.isFinite(inClose) ? 'inClose' : 'inVolume');
    }
    efiStepImpl(this, inClose, inVolume);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inClose - The bar's `inClose` value: Close price of each bar.
   * @param inVolume - The bar's `inVolume` value: Volume of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inClose: number, inVolume: number): number {
    if (!Number.isFinite(inClose) || !Number.isFinite(inVolume)) {
      throw nonFiniteBar('EFI peek', !Number.isFinite(inClose) ? 'inClose' : 'inVolume');
    }
    const sp = this;
    let cur_outReal = 0;
    if (sp.optInTimePeriod === 1) {
      let force = 0;
      let prevClose = sp.prevClose;
      force = (inClose - prevClose) * inVolume;
      prevClose = inClose;
      cur_outReal = force;
    } else {
      let force = 0;
      let prevClose = sp.prevClose;
      let prevMA = sp.prevMA;
      force = (inClose - prevClose) * inVolume;
      prevClose = inClose;
      prevMA = fma(sp.emaBeta, prevMA, sp.optInK_1 * force);
      cur_outReal = prevMA;
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
  clone(): EfiStream {
    const c = new EfiStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.prevClose = this.prevClose;
    c.emaBeta = this.emaBeta;
    c.optInK_1 = this.optInK_1;
    c.prevMA = this.prevMA;
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function efiStepImpl(sp: EfiStream, inClose: number, inVolume: number): void {
  if (sp.optInTimePeriod === 1) {
    let force = 0;
    force = (inClose - sp.prevClose) * inVolume;
    sp.prevClose = inClose;
    sp.cur_outReal = force;
  } else {
    let force = 0;
    force = (inClose - sp.prevClose) * inVolume;
    sp.prevClose = inClose;
    sp.prevMA = fma(sp.emaBeta, sp.prevMA, sp.optInK_1 * force);
    sp.cur_outReal = sp.prevMA;
  }
}

function efiOpenImpl(
  sp: EfiStream,
  inClose: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  const historyLen = inClose.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inVolume.length !== inClose.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 13;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod === 1) {
    let emaBeta = 0;
    let optInK_1 = 0;
    let prevMA = 0;
    let prevClose = 0;
    let force = 0;
    let today = 0;
    let outIdx = 0;
    let lookbackTotal = 0;
    /* Alexander Elder's Force Index (Trading for a Living, 1993): the one-bar
     * close-to-close move weighted by that bar's volume, then smoothed with an
     * EMA. Elder's 2-period reading is the short-term form and 13 the
     * intermediate-term one -- that is the parameter, not a second formula.
     *
     *    force[t] = ( close[t] - close[t-1] ) * volume[t]
     *    EFI      = EMA( force, optInTimePeriod )
     *
     * The arithmetic below is ema.c's with inReal[t] replaced by force[t], kept
     * in exactly that shape on purpose: the seed accumulates from 0.0 in the
     * same order, and the recurrence is k*x + beta*prevMA with ema.c's k and
     * beta. That order IS the bit-exactness contract against the composed
     * reference in test_composite.c -- MOM, then MULT, then EMA -- so do not
     * tidy it. TRIX carries the same warning.
     *
     * Nothing on the data path divides by an input, so issue #112 is satisfied
     * structurally: a flat close gives force exactly 0.0 and output exactly
     * 0.0, and zero volume likewise. The only division is by the period, a
     * positive integer parameter.
     *
     * prevClose is carried in a scalar rather than re-read from inClose[t-1]
     * because the C API allows outReal to alias an input: at bar t the slot
     * holding close[t-1] may already have been overwritten by the output
     * written a bar earlier. cmou.c carries its trailing value for the same
     * reason.
     */
    /* Identify the minimum number of price bar needed
     * to calculate at least one output.
     */
    lookbackTotal = efiLookback(optInTimePeriod, core);
    /* After the lookback call: a double live across a call is saved and
     * restored around every fma call of the loops below, one more instruction
     * per bar.
     */
    emaBeta = (optInTimePeriod - 1) / (optInTimePeriod + 1);
    optInK_1 = 1.0 - emaBeta;
    if (emaBeta < 0.5) {
      emaBeta = 1.0 - optInK_1;
    }
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
    /* No smoothing at a period of 1: the output is the raw Force Index.
     * Explicit because the recursion, at a k of 1.0 and a beta of 0.0, does
     * not keep the sign of a -0.0 force: a down bar on zero volume.
     */
    outBegIdx.value = startIdx;
    outIdx = 0;
    today = startIdx;
    prevClose = inClose[today - 1];
    while (today <= endIdx) {
      force = (inClose[today] - prevClose) * inVolume[today];
      prevClose = inClose[today];
      outReal[((outIdx * outStride) | 0)] = force;
      outIdx = outIdx + 1;
      today = today + 1;
    }
    outNBElement.value = outIdx;
    sp.optInTimePeriod = optInTimePeriod;
    sp.prevClose = prevClose;
    sp.emaBeta = emaBeta;
    sp.optInK_1 = optInK_1;
    sp.prevMA = prevMA;
    sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
    return RetCode.SUCCESS;
  } else {
    let emaBeta = 0;
    let optInK_1 = 0;
    let tempReal = 0;
    let prevMA = 0;
    let prevClose = 0;
    let force = 0;
    let i = 0;
    let today = 0;
    let outIdx = 0;
    let lookbackTotal = 0;
    /* Alexander Elder's Force Index (Trading for a Living, 1993): the one-bar
     * close-to-close move weighted by that bar's volume, then smoothed with an
     * EMA. Elder's 2-period reading is the short-term form and 13 the
     * intermediate-term one -- that is the parameter, not a second formula.
     *
     *    force[t] = ( close[t] - close[t-1] ) * volume[t]
     *    EFI      = EMA( force, optInTimePeriod )
     *
     * The arithmetic below is ema.c's with inReal[t] replaced by force[t], kept
     * in exactly that shape on purpose: the seed accumulates from 0.0 in the
     * same order, and the recurrence is k*x + beta*prevMA with ema.c's k and
     * beta. That order IS the bit-exactness contract against the composed
     * reference in test_composite.c -- MOM, then MULT, then EMA -- so do not
     * tidy it. TRIX carries the same warning.
     *
     * Nothing on the data path divides by an input, so issue #112 is satisfied
     * structurally: a flat close gives force exactly 0.0 and output exactly
     * 0.0, and zero volume likewise. The only division is by the period, a
     * positive integer parameter.
     *
     * prevClose is carried in a scalar rather than re-read from inClose[t-1]
     * because the C API allows outReal to alias an input: at bar t the slot
     * holding close[t-1] may already have been overwritten by the output
     * written a bar earlier. cmou.c carries its trailing value for the same
     * reason.
     */
    /* Identify the minimum number of price bar needed
     * to calculate at least one output.
     */
    lookbackTotal = efiLookback(optInTimePeriod, core);
    /* After the lookback call: a double live across a call is saved and
     * restored around every fma call of the loops below, one more instruction
     * per bar.
     */
    emaBeta = (optInTimePeriod - 1) / (optInTimePeriod + 1);
    optInK_1 = 1.0 - emaBeta;
    if (emaBeta < 0.5) {
      emaBeta = 1.0 - optInK_1;
    }
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
    /* No smoothing at a period of 1: the output is the raw Force Index.
     * Explicit because the recursion, at a k of 1.0 and a beta of 0.0, does
     * not keep the sign of a -0.0 force: a down bar on zero volume.
     */
    outBegIdx.value = startIdx;
    /* The first EMA value is a simple average of the first 'period' force
     * values; it then seeds the recursion. This is ema.c's seeding applied
     * to the force series rather than to the input array.
     */
    today = startIdx - lookbackTotal + 1;
    prevClose = inClose[today - 1];
    i = optInTimePeriod;
    tempReal = 0.0;
    while (i-- > 0) {
      force = (inClose[today] - prevClose) * inVolume[today];
      prevClose = inClose[today];
      tempReal += force;
      today = today + 1;
    }
    prevMA = tempReal / optInTimePeriod;
    while (today <= startIdx) {
      force = (inClose[today] - prevClose) * inVolume[today];
      prevClose = inClose[today];
      prevMA = fma(emaBeta, prevMA, optInK_1 * force);
      today = today + 1;
    }
    outReal[((0 * outStride) | 0)] = prevMA;
    outIdx = 1;
    while (today <= endIdx) {
      force = (inClose[today] - prevClose) * inVolume[today];
      prevClose = inClose[today];
      prevMA = fma(emaBeta, prevMA, optInK_1 * force);
      outReal[((outIdx * outStride) | 0)] = prevMA;
      outIdx = outIdx + 1;
      today = today + 1;
    }
    outNBElement.value = outIdx;
    sp.optInTimePeriod = optInTimePeriod;
    sp.prevClose = prevClose;
    sp.emaBeta = emaBeta;
    sp.optInK_1 = optInK_1;
    sp.prevMA = prevMA;
    sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
    return RetCode.SUCCESS;
  }
}

/** @internal */
export function efiOpenAndFillInternal(
  core: Core,
  inClose: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): EfiStream {
  const sp = EfiStream.of(core);
  const retCode = efiOpenImpl(sp, inClose, inVolume, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('EFI openAndFill', inClose.length, startIdx, efiLookback(optInTimePeriod, core));
  }
  throw streamFailure('EFI openAndFill', retCode);
}

/** @internal */
export function efiOpenInternal(
  core: Core,
  inClose: Float64Array,
  inVolume: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): EfiStream {
  const sp = EfiStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = efiOpenImpl(sp, inClose, inVolume, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('EFI open', inClose.length, startIdx, efiLookback(optInTimePeriod, core));
  }
  throw streamFailure('EFI open', retCode);
}

/**
 * Opens a live EFI stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link efi} there.
 *
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param inVolume - Warm-up history. Volume of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - EMA period applied to the force series (default 13;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `efiLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   an integer parameter that is not an int32, a parameter outside its range or
 *   histories of different lengths.
 *
 * @example
 * ```ts
 * import { efiOpen } from 'ta-lib';
 *
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const stream = efiOpen(close, volume, 13);
 * const peeked = stream.peek(100.9, 12345); // a forming bar: commits nothing
 * const updated = stream.update(100.9, 12345); // the bar closes
 * ```
 */
export function efiOpen(
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): EfiStream {
  optInTimePeriod = requireInt32('EFI open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('EFI open', 'inClose', inClose);
  requireHistory('EFI open', inClose.length);
  requireArgument('EFI open', 'inVolume', inVolume);
  requireHistoryLength('EFI open', 'inVolume', inVolume.length, inClose.length);
  return efiOpenInternal(core, float64(inClose), float64(inVolume), 0, optInTimePeriod);
}

/**
 * {@link efiOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link efi}. The range written is the stream's
 * `outRange()`.
 *
 * @param inClose - Warm-up history. Close price of each bar. Every history has the
 *   same length.
 * @param inVolume - Warm-up history. Volume of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - EMA period applied to the force series (default 13;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Smoothed force. Must hold `historyLen - lookback` values and
 *   share no memory with an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `efiLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or an output too short or starting where another
 *   output or a `Float64Array` input starts, or, on a history long enough to open,
 *   one sharing memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { efiOpenAndFill } from 'ta-lib';
 *
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const out = new Float64Array(close.length);
 * const stream = efiOpenAndFill(close, volume, 13, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function efiOpenAndFill(
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): EfiStream {
  optInTimePeriod = requireInt32('EFI openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('EFI openAndFill', 'inClose', inClose);
  requireHistory('EFI openAndFill', inClose.length);
  requireArgument('EFI openAndFill', 'inVolume', inVolume);
  const guardOutLen = openFillCount('EFI openAndFill', inClose.length, efiLookback(optInTimePeriod, core));
  requireHistoryLength('EFI openAndFill', 'inVolume', inVolume.length, inClose.length);
  requireOutput('EFI openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inCloseF64 = float64(inClose);
  const inVolumeF64 = float64(inVolume);
  if ((outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset) || (outReal.buffer === inVolumeF64.buffer && outReal.byteOffset === inVolumeF64.byteOffset)) {
    throw streamFailure('EFI openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inCloseF64) || overlaps(outReal, inVolumeF64))) {
    throw streamFailure('EFI openAndFill', RetCode.BAD_PARAM);
  }
  return efiOpenAndFillInternal(core, inCloseF64, inVolumeF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}
