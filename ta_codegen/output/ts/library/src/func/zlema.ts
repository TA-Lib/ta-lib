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
 *  090426 MF,CC  Initial version (#347).
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';
import { emaLookbackPositional } from './ema.ts';

/** @internal */
export function zlemaLookbackPositional(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  /* ZLEMA owns no TA_FUNC_UNST_ id. It borrows EMA's through this call, which
   * is why zlema.yaml must not declare `unstable_period`.
   */
  return (((optInTimePeriod - 1) / 2) | 0) + emaLookbackPositional(optInTimePeriod, core);
}

/** @internal */
export function zlemaDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (zlemaLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function zlemaImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let emaBeta = 0;
  let optInK_1 = 0;
  let tempReal = 0;
  let prevMA = 0;
  let i = 0;
  let today = 0;
  let trailingIdx = 0;
  let outIdx = 0;
  let lag = 0;
  let lookbackTotal = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  emaBeta = (optInTimePeriod - 1) / (optInTimePeriod + 1);
  optInK_1 = 1.0 - emaBeta;
  if (emaBeta < 0.5) {
    emaBeta = 1.0 - optInK_1;
  }
  /* KEEP THIS ARITHMETIC EXACTLY AS WRITTEN -- the de-lag in one rounding
   * (2.0*c - l, not c + (c - l)), the seed sum accumulating from 0.0, and
   * k*v + beta*prevMA with ema.c's k and beta. Together they make ZLEMA
   * bit-for-bit equal to an EMA over a materialised de-lagged series, which
   * is the strongest gate this function has. Reordering any one breaks that equality silently, and
   * the de-lag spelling is worth more than rounding noise: c + (c - l) rounds
   * twice, which is 5e-12 relative where 2c - l cancels.
   */
  lag = (((optInTimePeriod - 1) / 2) | 0);
  lookbackTotal = zlemaLookbackPositional(optInTimePeriod, core);
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
  /* No smoothing at period of 1: the output is a copy of the input, the
   * convention TA_MA applies to every MAType. Explicit, because at period 1
   * lag is 0 and the recursion below, at a k of 1.0 and a beta of 0.0, does
   * not keep the sign of a -0.0 input. The unstable period still delays the
   * first output.
   */
  if (optInTimePeriod === 1) {
    outBegIdx.value = startIdx;
    outIdx = 0;
    today = startIdx;
    while (today <= endIdx) {
      outReal[outIdx++] = inReal[today++];
    }
    outNBElement.value = outIdx;
    return RetCode.SUCCESS;
  }
  outBegIdx.value = startIdx;
  /* In-place safe (outReal == inReal): both of a bar's reads precede its
   * write, and trailingIdx >= outIdx + optInTimePeriod - 1, so the trailing
   * read never reaches a slot already written.
   */
  trailingIdx = startIdx - lookbackTotal;
  today = trailingIdx + lag;
  i = optInTimePeriod;
  tempReal = 0.0;
  while (i-- > 0) {
    tempReal += 2.0 * inReal[today] - inReal[trailingIdx];
    today += 1;
    trailingIdx += 1;
  }
  prevMA = tempReal / optInTimePeriod;
  while (today <= startIdx) {
    prevMA = fma(emaBeta, prevMA, optInK_1 * (2.0 * inReal[today] - inReal[trailingIdx]));
    today += 1;
    trailingIdx += 1;
  }
  outReal[0] = prevMA;
  outIdx = 1;
  while (today <= endIdx) {
    prevMA = fma(emaBeta, prevMA, optInK_1 * (2.0 * inReal[today] - inReal[trailingIdx]));
    today += 1;
    trailingIdx += 1;
    outReal[outIdx++] = prevMA;
  }
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000']] as const;

/** @internal */
export function zlemaPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('ZLEMA', 'startIdx', startIdx);
  endIdx = requireInt32('ZLEMA', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('ZLEMA', 'timePeriod', optInTimePeriod);
  requireIndexRange('ZLEMA', startIdx, endIdx);
  const guardLookback = zlemaLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('ZLEMA', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('ZLEMA', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('ZLEMA', 'real', inReal, guardInLen);
  requireOutput('ZLEMA', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('ZLEMA', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = zlemaImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('ZLEMA', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link zlema}, {@link zlemaLookback}, {@link zlemaDisplayShift},
 * {@link zlemaOpen} and {@link zlemaOpenAndFill}. Every key is optional: an
 * omitted or `undefined` key takes its default, and a call ignores the keys it has
 * no use for.
 */
export interface ZlemaOptions {
  /**
   * Number of bars in the exponential average; the de-lag distance derives from it
   * (default 30; range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
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

/** What {@link zlema} returns, and {@link zlemaOpenAndFill} with its stream. */
export interface ZlemaResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Zero-lag exponential moving average of the input. `values[k]` is the value at
   * input bar `begIdx + k`.
   */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link zlema} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link ZlemaOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function zlemaLookback(options?: ZlemaOptions): number {
  const o = optionsOf('ZLEMA lookback', options);
  return zlemaLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('ZLEMA lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link zlema}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link ZlemaOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function zlemaDisplayShift(outputIdx: number, options?: ZlemaOptions): number {
  const o = optionsOf('ZLEMA displayShift', options);
  return zlemaDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('ZLEMA displayShift', o.core));
}

/**
 * Zero-Lag Exponential Moving Average: an
 * [`EMA`](https://ta-lib.org/functions/ema) applied not to price but to a
 * de-lagged series that extrapolates price forward by the EMA's own lag,
 * cancelling that lag to first order. It tracks a trend far more closely than an
 * EMA of the same length, at the cost of overshooting sharp reversals — the
 * extrapolation keeps pushing in the old direction for a bar or two. Read it as an
 * EMA that turns sooner: crossings of price and average, and changes in its slope,
 * arrive earlier than the equivalent EMA signal, and its overshoot after a spike
 * is a property of the filter rather than a move in the market. ZLEMA is also
 * selectable as a moving-average type (`TA_MAType_ZLEMA`) wherever an
 * `optInMAType` parameter is accepted ([`MA`](https://ta-lib.org/functions/ma),
 * [`BBANDS`](https://ta-lib.org/functions/bbands),
 * [`STOCH`](https://ta-lib.org/functions/stoch),
 * [`MACDEXT`](https://ta-lib.org/functions/macdext), ...).
 *
 * Formula and more info at
 * [ta-lib.org/functions/zlema](https://ta-lib.org/functions/zlema).
 *
 * **Notes**
 * - **The paper this indicator is usually credited to describes a different
 *   filter.** Ehlers and Way's *Zero Lag (Well, Almost)* specifies an
 *   error-correcting EMA with a per-bar gain search; neither the de-lagged series
 *   nor the `(n-1)/2` lag appears anywhere in it. What TA-Lib ships here is the
 *   de-lagged-EMA construction published under the "zero lag" name by Tulip
 *   Indicators, pandas-ta, TradingView Pine and others, for which no primary
 *   source is traceable.
 * - `lag` **truncates**: `Integer((n-1)/2)`. For an even period that is one bar
 *   shorter than the round-to-nearest convention some descriptions use, which
 *   moves the whole line, not just its warm-up. Tulip Indicators, pandas-ta and
 *   Pine all truncate. R's TTR instead takes the lag as `(n+1)/2` without
 *   truncating, and for an even period interpolates between the two bars either
 *   side of it: a de-lag one to one and a half bars longer than this one, so its
 *   line stays apart from these values however long the series runs.
 * - The de-lag is computed as `2 * Price - Price[lag]` in one rounding, rather
 *   than the algebraically equal `Price + (Price - Price[lag])` that Tulip
 *   Indicators, TradingView Pine and the Wikipedia statement use. The second
 *   form's extra rounding is one unit in the last place of the larger price —
 *   negligible against the de-lagged value, except where that value nearly
 *   cancels. When price is near double its value `lag` bars ago the two forms
 *   differ by about 5e-12 relative, so expect that much disagreement against those
 *   implementations on a strongly trending series, and do not attribute it to the
 *   seed or the smoothing factor.
 * - Implementations disagree on how the inner EMA is seeded — TA-Lib uses its own
 *   EMA convention (the simple average of the first `n` de-lagged values), where
 *   Tulip Indicators seeds from a single raw price and so emits its first value
 *   earlier and converges to these values only after many bars.
 * - ZLEMA inherits EMA's unstable period rather than owning one:
 *   `TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)` moves ZLEMA's first output too.
 * - A period of 1 performs no smoothing: the output is a copy of the input.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link zlemaLookback} is a success with empty outputs.
 *
 * @param real - Source price series, close by convention.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link ZlemaOptions}.
 * @returns `begIdx` and the values; see {@link ZlemaResult}. Each output is a view
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
 * import { zlema } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = zlema(data, { timePeriod: 30 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`ema`](https://ta-lib.org/functions/ema)
 * @see [`dema`](https://ta-lib.org/functions/dema)
 * @see [`tema`](https://ta-lib.org/functions/tema)
 * @see [`hma`](https://ta-lib.org/functions/hma)
 * @see [`ma`](https://ta-lib.org/functions/ma)
 */
export function zlema(real: ArrayLike<number>, options?: ZlemaOptions): ZlemaResult {
  const o = optionsOf('ZLEMA', options);
  if (o.endIdx === undefined) {
    requireSeries('ZLEMA', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('ZLEMA', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, zlemaLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = zlemaPositional(startIdx, endIdx, real, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live ZLEMA stream: one value per closed bar, bit-identical to {@link zlema}
 * over the same series. Open it with {@link zlemaOpen} or
 * {@link zlemaOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/zlema](https://ta-lib.org/functions/zlema).
 */
export class ZlemaStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  emaBeta: number = 0;
  /** @internal */
  optInK_1: number = 0;
  /** @internal */
  prevMA: number = 0;
  /** @internal */
  ringPos_trailingIdx: number = 0;
  /** @internal */
  ringCap_trailingIdx: number = 0;
  /** @internal */
  ring_trailingIdx_inReal: Float64Array = new Float64Array(1);
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
  static of(core: Core): ZlemaStream {
    return new ZlemaStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link zlema} reports them over the same bars. Each accepted `update` and each
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
      throw failure('ZLEMA advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: Source price series, close by
   *   convention.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('ZLEMA update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('ZLEMA update', 'inReal');
    }
    zlemaStepImpl(this, inReal);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Source price series, close by
   *   convention.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number): number {
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('ZLEMA peek', 'inReal');
    }
    const sp = this;
    let cur_outReal = 0;
    let prevMA = sp.prevMA;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    if (sp.optInTimePeriod === 1) {
      cur_outReal = inReal;
      return cur_outReal;
    }
    if (sp.ringCap_trailingIdx === 0) {
      pkSlot0 = 0;
      pkVal0 = inReal;
    }
    prevMA = fma(sp.emaBeta, prevMA, sp.optInK_1 * (2.0 * inReal - ((sp.ringPos_trailingIdx !== pkSlot0) ? sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] : pkVal0)));
    cur_outReal = prevMA;
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
  clone(): ZlemaStream {
    const c = new ZlemaStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.emaBeta = this.emaBeta;
    c.optInK_1 = this.optInK_1;
    c.prevMA = this.prevMA;
    c.ringPos_trailingIdx = this.ringPos_trailingIdx;
    c.ringCap_trailingIdx = this.ringCap_trailingIdx;
    c.ring_trailingIdx_inReal = this.ring_trailingIdx_inReal.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function zlemaStepImpl(sp: ZlemaStream, inReal: number): void {
  if (sp.optInTimePeriod === 1) {
    sp.cur_outReal = inReal;
    return;
  }
  if (sp.ringCap_trailingIdx === 0) {
    sp.ring_trailingIdx_inReal[0] = inReal;
  }
  sp.prevMA = fma(sp.emaBeta, sp.prevMA, sp.optInK_1 * (2.0 * inReal - sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx]));
  sp.cur_outReal = sp.prevMA;
  sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
  sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
  if (sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx) {
    sp.ringPos_trailingIdx = 0;
  }
}

function zlemaOpenImpl(
  sp: ZlemaStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let emaBeta = 0;
  let optInK_1 = 0;
  let tempReal = 0;
  let prevMA = 0;
  let i = 0;
  let today = 0;
  let trailingIdx = 0;
  let outIdx = 0;
  let lag = 0;
  let lookbackTotal = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (optInTimePeriod === 1) {
    let fillLb = zlemaLookbackPositional(optInTimePeriod, core);
    if (startIdx > fillLb) {
      fillLb = startIdx;
    }
    if (historyLen < fillLb + 1) {
      return RetCode.INSUFFICIENT_HISTORY;
    }
    sp.optInTimePeriod = optInTimePeriod;
    sp.emaBeta = 0;
    sp.optInK_1 = 0;
    sp.prevMA = 0;
    sp.ringPos_trailingIdx = 0;
    sp.ringCap_trailingIdx = 0;
    sp.ring_trailingIdx_inReal = new Float64Array(1);
    outBegIdx.value = fillLb;
    outNBElement.value = historyLen - fillLb;
    if (outStride === 0) {
      outReal[0] = inReal[historyLen - 1];
    } else {
      for (let fillIdx = 0; fillIdx < historyLen - fillLb; fillIdx++) {
        outReal[fillIdx] = inReal[fillLb + fillIdx];
      }
    }
    sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
    return RetCode.SUCCESS;
  }
  emaBeta = (optInTimePeriod - 1) / (optInTimePeriod + 1);
  optInK_1 = 1.0 - emaBeta;
  if (emaBeta < 0.5) {
    emaBeta = 1.0 - optInK_1;
  }
  /* KEEP THIS ARITHMETIC EXACTLY AS WRITTEN -- the de-lag in one rounding
   * (2.0*c - l, not c + (c - l)), the seed sum accumulating from 0.0, and
   * k*v + beta*prevMA with ema.c's k and beta. Together they make ZLEMA
   * bit-for-bit equal to an EMA over a materialised de-lagged series, which
   * is the strongest gate this function has. Reordering any one breaks that equality silently, and
   * the de-lag spelling is worth more than rounding noise: c + (c - l) rounds
   * twice, which is 5e-12 relative where 2c - l cancels.
   */
  lag = (((optInTimePeriod - 1) / 2) | 0);
  lookbackTotal = zlemaLookbackPositional(optInTimePeriod, core);
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
  outBegIdx.value = startIdx;
  /* In-place safe (outReal == inReal): both of a bar's reads precede its
   * write, and trailingIdx >= outIdx + optInTimePeriod - 1, so the trailing
   * read never reaches a slot already written.
   */
  trailingIdx = startIdx - lookbackTotal;
  today = trailingIdx + lag;
  i = optInTimePeriod;
  tempReal = 0.0;
  while (i-- > 0) {
    tempReal += 2.0 * inReal[today] - inReal[trailingIdx];
    today += 1;
    trailingIdx += 1;
  }
  prevMA = tempReal / optInTimePeriod;
  while (today <= startIdx) {
    prevMA = fma(emaBeta, prevMA, optInK_1 * (2.0 * inReal[today] - inReal[trailingIdx]));
    today += 1;
    trailingIdx += 1;
  }
  outReal[((0 * outStride) | 0)] = prevMA;
  outIdx = 1;
  while (today <= endIdx) {
    prevMA = fma(emaBeta, prevMA, optInK_1 * (2.0 * inReal[today] - inReal[trailingIdx]));
    today += 1;
    trailingIdx += 1;
    outReal[((outIdx++ * outStride) | 0)] = prevMA;
  }
  outNBElement.value = outIdx;
  const cap_trailingIdx = today - trailingIdx;
  if (cap_trailingIdx < 0 || cap_trailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_trailingIdx = cap_trailingIdx > 0 ? cap_trailingIdx : 1;
  const capRing_trailingIdx_inReal = new Float64Array(allocN_trailingIdx);
  capRing_trailingIdx_inReal.set(inReal.subarray(historyLen - cap_trailingIdx, historyLen), 0);
  sp.optInTimePeriod = optInTimePeriod;
  sp.emaBeta = emaBeta;
  sp.optInK_1 = optInK_1;
  sp.prevMA = prevMA;
  sp.ringPos_trailingIdx = 0;
  sp.ringCap_trailingIdx = cap_trailingIdx;
  sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function zlemaOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): ZlemaStream {
  const sp = ZlemaStream.of(core);
  const retCode = zlemaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ZLEMA openAndFill', inReal.length, startIdx, zlemaLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && zlemaLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('ZLEMA openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('ZLEMA openAndFill', retCode);
}

/** @internal */
export function zlemaOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): ZlemaStream {
  const sp = ZlemaStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = zlemaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ZLEMA open', inReal.length, startIdx, zlemaLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && zlemaLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('ZLEMA open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('ZLEMA open', retCode);
}

/** @internal */
export function zlemaOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): ZlemaStream {
  optInTimePeriod = requireInt32('ZLEMA open', 'timePeriod', optInTimePeriod);
  requireArgument('ZLEMA open', 'real', inReal);
  requireHistory('ZLEMA open', inReal.length);
  return zlemaOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/** @internal */
export function zlemaOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): ZlemaStream {
  optInTimePeriod = requireInt32('ZLEMA openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('ZLEMA openAndFill', 'real', inReal);
  requireHistory('ZLEMA openAndFill', inReal.length);
  const guardLookback = zlemaLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('ZLEMA openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('ZLEMA openAndFill', inReal.length, guardLookback);
  requireOutput('ZLEMA openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('ZLEMA openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('ZLEMA openAndFill', 'outReal', 'real');
  }
  return zlemaOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live ZLEMA stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link zlema} there.
 *
 * @param real - Warm-up history. Source price series, close by convention.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link ZlemaOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `zlemaLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32 or a parameter outside its
 *   range.
 *
 * @example
 * ```ts
 * import { zlemaOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = zlemaOpen(data, { timePeriod: 30 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function zlemaOpen(real: ArrayLike<number>, options?: ZlemaOptions): ZlemaStream {
  const o = optionsOf('ZLEMA open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('ZLEMA open', o.core);
  return zlemaOpenPositional(real, optInTimePeriod, core);
}

/**
 * {@link zlemaOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link zlema}.
 *
 * @param real - Warm-up history. Source price series, close by convention.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link ZlemaOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link zlema} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `zlemaLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for options that are not an object, a `core` that is not a `Core`, an absent
 *   input, an integer parameter that is not an int32, a parameter outside its range
 *   or a buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { zlemaOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = zlemaOpenAndFill(data, { timePeriod: 30 });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function zlemaOpenAndFill(real: ArrayLike<number>, options?: ZlemaOptions): ZlemaResult & { stream: ZlemaStream } {
  const o = optionsOf('ZLEMA openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('ZLEMA openAndFill', o.core);
  const capacity = fillCapacity(zlemaLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = zlemaOpenAndFillPositional(real, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/** @internal */
export function zlemaStepTape(sp: ZlemaStream, tape: Float64Array, tapeBase: number, tapeMask: number, inReal: number): number {
  sp.prevMA = fma(sp.emaBeta, sp.prevMA, sp.optInK_1 * (2.0 * inReal - tape[(tapeBase - sp.ringCap_trailingIdx) & tapeMask]));
  sp.cur_outReal = sp.prevMA;
  sp.outRangeCount++;
  return sp.cur_outReal;
}

/** @internal */
export function zlemaPeekTape(sp: ZlemaStream, tape: Float64Array, tapeBase: number, tapeMask: number, inReal: number): number {
  let cur_outReal = 0;
  let prevMA = sp.prevMA;
  let pkSlot0 = -1;
  let pkVal0 = 0;
  pkSlot0 = tapeBase & tapeMask;
  pkVal0 = inReal;
  prevMA = fma(sp.emaBeta, prevMA, sp.optInK_1 * (2.0 * inReal - ((((tapeBase - sp.ringCap_trailingIdx) & tapeMask) !== pkSlot0) ? tape[(tapeBase - sp.ringCap_trailingIdx) & tapeMask] : pkVal0)));
  cur_outReal = prevMA;
  return cur_outReal;
}

/** @internal */
export function zlemaTapeDetach(sp: ZlemaStream): number {
  let reach = 0;
  sp.ring_trailingIdx_inReal = new Float64Array(0);
  if (sp.ringCap_trailingIdx > reach) {
    reach = sp.ringCap_trailingIdx;
  }
  return reach;
}
