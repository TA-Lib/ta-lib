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
 *  090426 MF,CC  Initial version (#367).
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link adr} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Number of bar ranges averaged. Published conventions
 *   differ and none is authoritative: TradingView's ADR indicator page works its
 *   example over 7 bars, TC2000's over 10, and the Qullamaggie screener community
 *   reads "ADR" as 20. The value shipped here is ATR's, so the two volatility
 *   measures are comparable out of the box (default 14; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function adrLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  return optInTimePeriod - 1;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link adr}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Number of bar ranges averaged. Published conventions
 *   differ and none is authoritative: TradingView's ADR indicator page works its
 *   example over 7 bars, TC2000's over 10, and the Qullamaggie screener community
 *   reads "ADR" as 20. The value shipped here is ATR's, so the two volatility
 *   measures are comparable out of the box (default 14; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function adrDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (adrLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function adrImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inHigh: Float64Array,
  inLow: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let periodTotal = 0;
  let tempReal = 0;
  let i = 0;
  let outIdx = 0;
  let trailingIdx = 0;
  let lookbackTotal = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  /* This is ta_codegen/input/sma/sma.c with inReal[x] replaced by
   * (inHigh[x] - inLow[x]), and deliberately nothing else -- the same
   * running-sum order, the same read-before-write of the trailing term, and
   * the same divide by the period. Keeping the arithmetic identical is what
   * makes the composed reference in test_adr.c -- TA_SUB followed by TA_SMA
   * -- bit-exact rather than merely close.
   *
   * In particular the last statement divides; it does NOT multiply by a
   * precomputed 1/period, which would cost up to one ULP against TA_SMA and
   * silently downgrade that memcmp into a tolerance argument.
   *
   * Averaging the ranges is not the same computation as differencing two
   * averages: SMA(high) - SMA(low) is algebraically equal but subtracts two
   * price-magnitude means to reach a range-magnitude answer, so it inherits
   * the larger scale's rounding.
   */
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = (optInTimePeriod - 1);
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
  /* Do the MA calculation using tight loops. */
  /* Add-up the initial period, except for the last value. */
  periodTotal = 0.0;
  trailingIdx = startIdx - lookbackTotal;
  i = trailingIdx;
  if (optInTimePeriod > 1) {
    while (i < startIdx) {
      periodTotal += (inHigh[i] - inLow[i]);
      i = i + 1;
    }
  }
  /* Proceed with the calculation for the requested range.
   * Note that this algorithm allows outReal to be the same
   * buffer as either input.
   */
  outIdx = 0;
  while (i <= endIdx) {
    periodTotal += (inHigh[i] - inLow[i]);
    i = i + 1;
    tempReal = periodTotal;
    periodTotal -= (inHigh[trailingIdx] - inLow[trailingIdx]);
    trailingIdx = trailingIdx + 1;
    outReal[outIdx] = tempReal / optInTimePeriod;
    outIdx = outIdx + 1;
  }
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/**
 * Average Day Range: the arithmetic mean of the last `optInTimePeriod` bar ranges,
 * high minus low. It answers how far price travels *within* a bar, and is read as
 * a volatility budget — a stop or a target much smaller than ADR is inside the
 * noise the instrument produces on an ordinary bar, one much larger asks for a
 * move that rarely happens. Same family as ATR, and deliberately the narrower
 * member: the range excludes the overnight gap, so on a gapping instrument ADR is
 * systematically smaller than ATR. Having both is the point.
 *
 * Formula and more info at
 * [ta-lib.org/functions/adr](https://ta-lib.org/functions/adr).
 *
 * **Notes**
 * - The mean of the ranges, not the difference of the means.
 *   `SMA(high) - SMA(low)` is algebraically the same quantity and is what both
 *   TradingView pages spell, but it subtracts two price-magnitude averages to
 *   reach a range-magnitude answer and inherits the larger scale's rounding;
 *   TC2000's `AVG(H-L, x)` and kand's `SMA(High-Low, period)` spell the form
 *   implemented here.
 * - The "day" is not a calendar day or a trading session. No TA-Lib function takes
 *   a timestamp or a session boundary, so the bars the caller passes *are* the
 *   days — pass daily bars for a daily range, hourly bars for an hourly one. This
 *   is the convention VWAP already ships under.
 * - `high` below `low` is not rejected. The library validates ranges and
 *   parameters, not price sanity, so a bar entered upside down contributes a
 *   negative range and the average simply comes out lower, possibly negative, with
 *   no error.
 * - Not the width of a Donchian channel. `MAX(high, n) - MIN(low, n)` is how far
 *   the window's extremes lie apart; ADR is the mean of the per-bar ranges, which
 *   is smaller whenever the window trends. `DONCHIAN` ships the two extremes that
 *   width is built from, not the width itself.
 * - The request this function answers (`TA-Lib/ta-lib-python#575`) named "Average
 *   Day Range" but the freqtrade code behind it computes
 *   `MAX(close, 24) - MIN(close, 24)`, a channel width on the closes with no
 *   averaging and no high/low. That is a different series and already reachable,
 *   as `TA_SUB(TA_MAX(close, 24), TA_MIN(close, 24))`.
 * - No percentage form is emitted. The two published ones disagree by more than
 *   20% on ordinary data — Qullamaggie's `100 · (SMA(high/low, 20) - 1)` averages
 *   ratios, TradingView's `(SMA(high, 14) - SMA(low, 14)) / close · 100` takes a
 *   ratio of averages — so picking one silently would ship a second indicator
 *   under this one's name.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link adrLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inHigh - High price of each bar.
 * @param inLow - Low price of each bar.
 * @param optInTimePeriod - Number of bar ranges averaged. Published conventions
 *   differ and none is authoritative: TradingView's ADR indicator page works its
 *   example over 7 bars, TC2000's over 10, and the Qullamaggie screener community
 *   reads "ADR" as 20. The value shipped here is ATR's, so the two volatility
 *   measures are comparable out of the box (default 14; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Average bar range over the window, in price units. Must hold
 *   the values produced and never be empty; the input length always suffices.
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
 * import { adr } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(high.length);
 * const { begIdx, count } = adr(0, high.length - 1, high, low, 14, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`atr`](https://ta-lib.org/functions/atr)
 * @see [`donchian`](https://ta-lib.org/functions/donchian)
 * @see [`natr`](https://ta-lib.org/functions/natr)
 * @see [`qstick`](https://ta-lib.org/functions/qstick)
 * @see [`trange`](https://ta-lib.org/functions/trange)
 */
export function adr(
  startIdx: number,
  endIdx: number,
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('ADR', 'startIdx', startIdx);
  endIdx = requireInt32('ADR', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('ADR', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('ADR', startIdx, endIdx);
  const guardStart = clampedStart('ADR', startIdx, adrLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('ADR', 'inHigh', inHigh, guardInLen);
  requireLength('ADR', 'inLow', inLow, guardInLen);
  requireOutput('ADR', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if (overlapsPartly(outReal, inHighF64) || overlapsPartly(outReal, inLowF64)) {
    throw failure('ADR', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = adrImpl(core, startIdx, endIdx, inHighF64, inLowF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('ADR', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live ADR stream: one value per closed bar, bit-identical to {@link adr} over
 * the same series. Open it with {@link adrOpen} or {@link adrOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/adr](https://ta-lib.org/functions/adr).
 */
export class AdrStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  periodTotal: number = 0;
  /** @internal */
  ringPos_trailingIdx: number = 0;
  /** @internal */
  ringCap_trailingIdx: number = 0;
  /** @internal */
  ring_trailingIdx_derived: Float64Array = new Float64Array(1);
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
  static of(core: Core): AdrStream {
    return new AdrStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link adr} reports them over the same bars. Each accepted `update` and each
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
      throw failure('ADR advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inHigh: number, inLow: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('ADR update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('ADR update', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    adrStepImpl(this, inHigh, inLow);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inHigh - The bar's `inHigh` value: High price of each bar.
   * @param inLow - The bar's `inLow` value: Low price of each bar.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inHigh: number, inLow: number): number {
    if (!Number.isFinite(inHigh) || !Number.isFinite(inLow)) {
      throw nonFiniteBar('ADR peek', !Number.isFinite(inHigh) ? 'inHigh' : 'inLow');
    }
    const sp = this;
    let tempReal = 0;
    let cur_outReal = 0;
    let periodTotal = sp.periodTotal;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    if (sp.ringCap_trailingIdx === 0) {
      pkSlot0 = 0;
      pkVal0 = (inHigh - inLow);
    }
    periodTotal += (inHigh - inLow);
    tempReal = periodTotal;
    periodTotal -= (sp.ringPos_trailingIdx !== pkSlot0) ? sp.ring_trailingIdx_derived[sp.ringPos_trailingIdx] : pkVal0;
    cur_outReal = tempReal / sp.optInTimePeriod;
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
  clone(): AdrStream {
    const c = new AdrStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.periodTotal = this.periodTotal;
    c.ringPos_trailingIdx = this.ringPos_trailingIdx;
    c.ringCap_trailingIdx = this.ringCap_trailingIdx;
    c.ring_trailingIdx_derived = this.ring_trailingIdx_derived.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function adrStepImpl(sp: AdrStream, inHigh: number, inLow: number): void {
  let tempReal = 0;
  if (sp.ringCap_trailingIdx === 0) {
    sp.ring_trailingIdx_derived[0] = (inHigh - inLow);
  }
  sp.periodTotal += (inHigh - inLow);
  tempReal = sp.periodTotal;
  sp.periodTotal -= sp.ring_trailingIdx_derived[sp.ringPos_trailingIdx];
  sp.cur_outReal = tempReal / sp.optInTimePeriod;
  sp.ring_trailingIdx_derived[sp.ringPos_trailingIdx] = (inHigh - inLow);
  sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
  if (sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx) {
    sp.ringPos_trailingIdx = 0;
  }
}

function adrOpenImpl(
  sp: AdrStream,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let periodTotal = 0;
  let tempReal = 0;
  let i = 0;
  let outIdx = 0;
  let trailingIdx = 0;
  let lookbackTotal = 0;
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
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 14;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* This is ta_codegen/input/sma/sma.c with inReal[x] replaced by
   * (inHigh[x] - inLow[x]), and deliberately nothing else -- the same
   * running-sum order, the same read-before-write of the trailing term, and
   * the same divide by the period. Keeping the arithmetic identical is what
   * makes the composed reference in test_adr.c -- TA_SUB followed by TA_SMA
   * -- bit-exact rather than merely close.
   *
   * In particular the last statement divides; it does NOT multiply by a
   * precomputed 1/period, which would cost up to one ULP against TA_SMA and
   * silently downgrade that memcmp into a tolerance argument.
   *
   * Averaging the ranges is not the same computation as differencing two
   * averages: SMA(high) - SMA(low) is algebraically equal but subtracts two
   * price-magnitude means to reach a range-magnitude answer, so it inherits
   * the larger scale's rounding.
   */
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = (optInTimePeriod - 1);
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
  /* Do the MA calculation using tight loops. */
  /* Add-up the initial period, except for the last value. */
  periodTotal = 0.0;
  trailingIdx = startIdx - lookbackTotal;
  i = trailingIdx;
  if (optInTimePeriod > 1) {
    while (i < startIdx) {
      periodTotal += (inHigh[i] - inLow[i]);
      i = i + 1;
    }
  }
  /* Proceed with the calculation for the requested range.
   * Note that this algorithm allows outReal to be the same
   * buffer as either input.
   */
  outIdx = 0;
  while (i <= endIdx) {
    periodTotal += (inHigh[i] - inLow[i]);
    i = i + 1;
    tempReal = periodTotal;
    periodTotal -= (inHigh[trailingIdx] - inLow[trailingIdx]);
    trailingIdx = trailingIdx + 1;
    outReal[((outIdx * outStride) | 0)] = tempReal / optInTimePeriod;
    outIdx = outIdx + 1;
  }
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const cap_trailingIdx = i - trailingIdx;
  if (cap_trailingIdx < 0 || cap_trailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_trailingIdx = cap_trailingIdx > 0 ? cap_trailingIdx : 1;
  const capRing_trailingIdx_derived = new Float64Array(allocN_trailingIdx);
  for (let fillJ = historyLen - cap_trailingIdx; fillJ < historyLen; fillJ++) {
    capRing_trailingIdx_derived[fillJ - (historyLen - cap_trailingIdx)] = (inHigh[fillJ] - inLow[fillJ]);
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.periodTotal = periodTotal;
  sp.ringPos_trailingIdx = 0;
  sp.ringCap_trailingIdx = cap_trailingIdx;
  sp.ring_trailingIdx_derived = capRing_trailingIdx_derived;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function adrOpenAndFillInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): AdrStream {
  const sp = AdrStream.of(core);
  const retCode = adrOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ADR openAndFill', inHigh.length, startIdx, adrLookback(optInTimePeriod, core));
  }
  throw streamFailure('ADR openAndFill', retCode);
}

/** @internal */
export function adrOpenInternal(
  core: Core,
  inHigh: Float64Array,
  inLow: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): AdrStream {
  const sp = AdrStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = adrOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('ADR open', inHigh.length, startIdx, adrLookback(optInTimePeriod, core));
  }
  throw streamFailure('ADR open', retCode);
}

/**
 * Opens a live ADR stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link adr} there.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Number of bar ranges averaged. Published conventions
 *   differ and none is authoritative: TradingView's ADR indicator page works its
 *   example over 7 bars, TC2000's over 10, and the Qullamaggie screener community
 *   reads "ADR" as 20. The value shipped here is ATR's, so the two volatility
 *   measures are comparable out of the box (default 14; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `adrLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   an integer parameter that is not an int32, a parameter outside its range or
 *   histories of different lengths.
 *
 * @example
 * ```ts
 * import { adrOpen } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const stream = adrOpen(high, low, 14);
 * const peeked = stream.peek(101.4, 99.1); // a forming bar: commits nothing
 * const updated = stream.update(101.4, 99.1); // the bar closes
 * ```
 */
export function adrOpen(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): AdrStream {
  optInTimePeriod = requireInt32('ADR open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('ADR open', 'inHigh', inHigh);
  requireHistory('ADR open', inHigh.length);
  requireArgument('ADR open', 'inLow', inLow);
  requireHistoryLength('ADR open', 'inLow', inLow.length, inHigh.length);
  return adrOpenInternal(core, float64(inHigh), float64(inLow), 0, optInTimePeriod);
}

/**
 * {@link adrOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link adr}. The range written is the stream's
 * `outRange()`.
 *
 * @param inHigh - Warm-up history. High price of each bar. Every history has the
 *   same length.
 * @param inLow - Warm-up history. Low price of each bar. Every history has the
 *   same length.
 * @param optInTimePeriod - Number of bar ranges averaged. Published conventions
 *   differ and none is authoritative: TradingView's ADR indicator page works its
 *   example over 7 bars, TC2000's over 10, and the Qullamaggie screener community
 *   reads "ADR" as 20. The value shipped here is ATR's, so the two volatility
 *   measures are comparable out of the box (default 14; range 1 to 100000;
 *   `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Average bar range over the window, in price units. Must hold
 *   `historyLen - lookback` values and share no memory with an input or another
 *   output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `adrLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or an output too short or starting where another
 *   output or a `Float64Array` input starts, or, on a history long enough to open,
 *   one sharing memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { adrOpenAndFill } from 'ta-lib';
 *
 * const high = Float64Array.from({ length: 252 }, (_, i) =>
 *   101 + 10 * Math.sin(0.1 * i));
 * const low = Float64Array.from({ length: 252 }, (_, i) =>
 *   99 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(high.length);
 * const stream = adrOpenAndFill(high, low, 14, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function adrOpenAndFill(
  inHigh: ArrayLike<number>,
  inLow: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): AdrStream {
  optInTimePeriod = requireInt32('ADR openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('ADR openAndFill', 'inHigh', inHigh);
  requireHistory('ADR openAndFill', inHigh.length);
  requireArgument('ADR openAndFill', 'inLow', inLow);
  const guardOutLen = openFillCount('ADR openAndFill', inHigh.length, adrLookback(optInTimePeriod, core));
  requireHistoryLength('ADR openAndFill', 'inLow', inLow.length, inHigh.length);
  requireOutput('ADR openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inHighF64 = float64(inHigh);
  const inLowF64 = float64(inLow);
  if ((outReal.buffer === inHighF64.buffer && outReal.byteOffset === inHighF64.byteOffset) || (outReal.buffer === inLowF64.buffer && outReal.byteOffset === inLowF64.byteOffset)) {
    throw streamFailure('ADR openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inHighF64) || overlaps(outReal, inLowF64))) {
    throw streamFailure('ADR openAndFill', RetCode.BAD_PARAM);
  }
  return adrOpenAndFillInternal(core, inHighF64, inLowF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}
