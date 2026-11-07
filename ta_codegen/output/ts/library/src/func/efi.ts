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

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { emaLookbackPositional } from './ema.ts';

/** @internal */
export function efiLookbackPositional(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 13;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  /* One bar forms the first close-to-close change. */
  return 1 + emaLookbackPositional(optInTimePeriod, core);
}

/** @internal */
export function efiDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (efiLookbackPositional(optInTimePeriod, core) < 0) {
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
  lookbackTotal = efiLookbackPositional(optInTimePeriod, core);
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

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000']] as const;

/** @internal */
export function efiPositional(
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
  optInTimePeriod = requireInt32('EFI', 'timePeriod', optInTimePeriod);
  requireIndexRange('EFI', startIdx, endIdx);
  const guardLookback = efiLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('EFI', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('EFI', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('EFI', 'prices.close', inClose, guardInLen);
  requireLength('EFI', 'prices.volume', inVolume, guardInLen);
  requireOutput('EFI', 'outReal', outReal, guardOutLen, Float64Array);
  const inCloseF64 = float64(inClose);
  const inVolumeF64 = float64(inVolume);
  if (overlapsPartly(outReal, inCloseF64)) {
    throw sharedMemory('EFI', 'outReal', 'prices.close');
  }
  if (overlapsPartly(outReal, inVolumeF64)) {
    throw sharedMemory('EFI', 'outReal', 'prices.volume');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = efiImpl(core, startIdx, endIdx, inCloseF64, inVolumeF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('EFI', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link efi}, {@link efiLookback}, {@link efiDisplayShift},
 * {@link efiOpen} and {@link efiOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface EfiOptions {
  /**
   * EMA period applied to the force series (default 13; range 1 to 100000;
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
  outReal?: Float64Array | undefined;
}

/** What {@link efi} returns, and {@link efiOpenAndFill} with its stream. */
export interface EfiResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /** Smoothed force. `values[k]` is the value at input bar `begIdx + k`. */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link efi} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link EfiOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function efiLookback(options?: EfiOptions): number {
  const o = optionsOf('EFI lookback', options);
  return efiLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('EFI lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link efi}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link EfiOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function efiDisplayShift(outputIdx: number, options?: EfiOptions): number {
  const o = optionsOf('EFI displayShift', options);
  return efiDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('EFI displayShift', o.core));
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
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link efiLookback} is a success with empty outputs.
 *
 * @param prices - One series per price component, all of one length. `close`:
 *   Close price of each bar. `volume`: Volume of each bar.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link EfiOptions}.
 * @returns `begIdx` and the values; see {@link EfiResult}. Each output is a view
 *   of its buffer, the caller's or a new one.
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
 * import { efi } from 'ta-lib';
 *
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const { begIdx, values } = efi({ close, volume }, { timePeriod: 13 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`ad`](https://ta-lib.org/functions/ad)
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`mfi`](https://ta-lib.org/functions/mfi)
 * @see [`obv`](https://ta-lib.org/functions/obv)
 * @see [`pvo`](https://ta-lib.org/functions/pvo)
 */
export function efi(prices: { close: ArrayLike<number>; volume: ArrayLike<number> }, options?: EfiOptions): EfiResult {
  requireArgument('EFI', 'prices', prices);
  const o = optionsOf('EFI', options);
  if (o.endIdx === undefined) {
    requireSeries('EFI', 'prices.close', prices.close);
    requireSeries('EFI', 'prices.volume', prices.volume);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('EFI', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? prices.close.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, efiLookbackPositional(optInTimePeriod, core), prices.close, prices.volume);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = efiPositional(startIdx, endIdx, prices.close, prices.volume, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
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
      throw streamFailure('EFI advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param close - The bar's `close` value: Close price of each bar.
   * @param volume - The bar's `volume` value: Volume of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(close: number, volume: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('EFI update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(close) || !Number.isFinite(volume)) {
      throw nonFiniteBar('EFI update', !Number.isFinite(close) ? 'close' : 'volume');
    }
    efiStepImpl(this, close, volume);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param close - The bar's `close` value: Close price of each bar.
   * @param volume - The bar's `volume` value: Volume of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(close: number, volume: number): number {
    if (!Number.isFinite(close) || !Number.isFinite(volume)) {
      throw nonFiniteBar('EFI peek', !Number.isFinite(close) ? 'close' : 'volume');
    }
    const sp = this;
    const inClose = close;
    const inVolume = volume;
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
    lookbackTotal = efiLookbackPositional(optInTimePeriod, core);
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
    lookbackTotal = efiLookbackPositional(optInTimePeriod, core);
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
    throw insufficientHistory('EFI openAndFill', inClose.length, startIdx, efiLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && efiLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('EFI openAndFill', PARAM_RANGES, optInTimePeriod);
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
    throw insufficientHistory('EFI open', inClose.length, startIdx, efiLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && efiLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('EFI open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('EFI open', retCode);
}

/** @internal */
export function efiOpenPositional(
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): EfiStream {
  optInTimePeriod = requireInt32('EFI open', 'timePeriod', optInTimePeriod);
  requireArgument('EFI open', 'prices.close', inClose);
  requireHistory('EFI open', 'prices.close', inClose.length);
  requireArgument('EFI open', 'prices.volume', inVolume);
  requireHistoryLength('EFI open', 'prices.volume', inVolume.length, inClose.length);
  return efiOpenInternal(core, float64(inClose), float64(inVolume), 0, optInTimePeriod);
}

/** @internal */
export function efiOpenAndFillPositional(
  inClose: ArrayLike<number>,
  inVolume: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): EfiStream {
  optInTimePeriod = requireInt32('EFI openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('EFI openAndFill', 'prices.close', inClose);
  requireHistory('EFI openAndFill', 'prices.close', inClose.length);
  requireArgument('EFI openAndFill', 'prices.volume', inVolume);
  const guardLookback = efiLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('EFI openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('EFI openAndFill', inClose.length, guardLookback);
  requireHistoryLength('EFI openAndFill', 'prices.volume', inVolume.length, inClose.length);
  requireOutput('EFI openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inCloseF64 = float64(inClose);
  const inVolumeF64 = float64(inVolume);
  if (outReal.buffer === inCloseF64.buffer && outReal.byteOffset === inCloseF64.byteOffset) {
    throw sharedMemory('EFI openAndFill', 'outReal', 'prices.close');
  }
  if (outReal.buffer === inVolumeF64.buffer && outReal.byteOffset === inVolumeF64.byteOffset) {
    throw sharedMemory('EFI openAndFill', 'outReal', 'prices.volume');
  }
  if (guardOutLen > 0 && overlaps(outReal, inCloseF64)) {
    throw sharedMemory('EFI openAndFill', 'outReal', 'prices.close');
  }
  if (guardOutLen > 0 && overlaps(outReal, inVolumeF64)) {
    throw sharedMemory('EFI openAndFill', 'outReal', 'prices.volume');
  }
  return efiOpenAndFillInternal(core, inCloseF64, inVolumeF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live EFI stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link efi} there.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `close`: Close price of each bar. `volume`: Volume of each bar.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link EfiOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `efiLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
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
 * const stream = efiOpen({ close, volume }, { timePeriod: 13 });
 * const peeked = stream.peek(100.9, 12345); // a forming bar: commits nothing
 * const updated = stream.update(100.9, 12345); // the bar closes
 * ```
 */
export function efiOpen(prices: { close: ArrayLike<number>; volume: ArrayLike<number> }, options?: EfiOptions): EfiStream {
  requireArgument('EFI open', 'prices', prices);
  const o = optionsOf('EFI open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('EFI open', o.core);
  return efiOpenPositional(prices.close, prices.volume, optInTimePeriod, core);
}

/**
 * {@link efiOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link efi}.
 *
 * @param prices - Warm-up history. One series per price component, all of one
 *   length. `close`: Close price of each bar. `volume`: Volume of each bar.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link EfiOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link efi} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `efiLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { efiOpenAndFill } from 'ta-lib';
 *
 * const close = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i));
 * const volume = Float64Array.from({ length: 252 }, (_, i) =>
 *   10000 + 100 * i + 2000 * Math.sin(0.3 * i));
 * const { stream, begIdx, values } = efiOpenAndFill({ close, volume }, {
 *   timePeriod: 13,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function efiOpenAndFill(prices: { close: ArrayLike<number>; volume: ArrayLike<number> }, options?: EfiOptions): EfiResult & { stream: EfiStream } {
  requireArgument('EFI openAndFill', 'prices', prices);
  const o = optionsOf('EFI openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('EFI openAndFill', o.core);
  const capacity = fillCapacity(efiLookbackPositional(optInTimePeriod, core), prices.close, prices.volume);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = efiOpenAndFillPositional(prices.close, prices.volume, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
