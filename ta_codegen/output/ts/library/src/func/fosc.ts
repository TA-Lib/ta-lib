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
 *  090426 MF,CC  First version. See issue #345.
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { fma, fmaSlow } from '../runtime/fma.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link fosc} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Number of bars in the regression window (default 5;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function foscLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 5;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  return optInTimePeriod;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link fosc}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Number of bars in the regression window (default 5;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function foscDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (foscLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function foscImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let outIdx = 0;
  let today = 0;
  let lookbackTotal = 0;
  let trailingIdx = 0;
  let SumX = 0;
  let SumXY = 0;
  let SumY = 0;
  let SumXSqr = 0;
  let Divisor = 0;
  let m = 0;
  let b = 0;
  let closeValue = 0;
  let i = 0;
  let j = 0;
  let windowStart = 0;
  let barsSinceReseed = 0;
  let tempValue1 = 0;
  let tempValue2 = 0;
  let trailingValue = 0;
  let weightedTrailing = 0;
  let sumAbs = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 5;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  /* The forecast compared against the close is the one made one bar EARLIER:
   * FOSC[t] = 100*(inReal[t] - TSF[t-1])/inReal[t]. The regression window
   * ends at t-1, so the lookback is period, not period-1.
   *
   * The window arithmetic below is TA_TSF's verbatim -- priming scan, O(1)
   * recurrence, both re-anchor triggers (#254) and the fused
   * `b + m*(double)optInTimePeriod` -- which is what makes FOSC bit-identical
   * to a TA_TSF call anchored one bar earlier. Reshaping any of it breaks
   * that silently.
   *
   * trailingValue -- the value the NEXT bar's window drops -- is read before
   * the output write because with outReal==inReal (#130) that write lands on
   * exactly that cell whenever startIdx is the clamped minimum. closeValue
   * carries no such constraint: it sits startIdx bars ahead of the cursor.
   */
  lookbackTotal = foscLookback(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  outIdx = 0;
  today = startIdx;
  trailingIdx = startIdx - lookbackTotal;
  SumX = optInTimePeriod * (optInTimePeriod - 1) * 0.5;
  SumXSqr = optInTimePeriod * (optInTimePeriod - 1) * (((2 * optInTimePeriod) | 0) - 1) / 6.0;
  Divisor = SumX * SumX - optInTimePeriod * SumXSqr;
  SumXY = 0;
  SumY = 0;
  sumAbs = 0;
  for (i = optInTimePeriod; i-- !== 0; ) {
    tempValue1 = inReal[today - 1 - i];
    SumY += tempValue1;
    SumXY += i * tempValue1;
    sumAbs += Math.abs(tempValue1);
  }
  m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
  b = (SumY - m * SumX) / optInTimePeriod;
  barsSinceReseed = ((32 * optInTimePeriod) | 0);
  trailingValue = inReal[trailingIdx];
  trailingIdx += 1;
  closeValue = inReal[today];
  if (closeValue !== 0.0) {
    outReal[outIdx++] = 100.0 * (closeValue - (fma(m, optInTimePeriod, b))) / closeValue;
  } else {
    outReal[outIdx++] = 0.0;
  }
  today += 1;
  while (today <= endIdx) {
    weightedTrailing = optInTimePeriod * trailingValue;
    SumXY = SumXY + SumY - weightedTrailing;
    SumY = SumY - trailingValue + inReal[today - 1];
    sumAbs = sumAbs - Math.abs(trailingValue) + Math.abs(inReal[today - 1]);
    barsSinceReseed -= 1;
    if (barsSinceReseed <= 0 || Math.abs(weightedTrailing) > 100.0 * sumAbs) {
      barsSinceReseed = ((32 * optInTimePeriod) | 0);
      windowStart = today - lookbackTotal;
      SumY = 0;
      SumXY = 0;
      sumAbs = 0;
      tempValue2 = (optInTimePeriod - 1);
      for (j = windowStart; j < today; j += 1) {
        tempValue1 = inReal[j];
        SumY += tempValue1;
        SumXY += tempValue2 * tempValue1;
        sumAbs += Math.abs(tempValue1);
        tempValue2 -= 1.0;
      }
    }
    m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
    b = (SumY - m * SumX) / optInTimePeriod;
    trailingValue = inReal[trailingIdx];
    trailingIdx += 1;
    closeValue = inReal[today];
    if (closeValue !== 0.0) {
      let _fma0: number;
      {
        const _a: number = m, _b: number = optInTimePeriod, _c: number = b;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        _fma0 = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      outReal[outIdx++] = 100.0 * (closeValue - _fma0) / closeValue;
    } else {
      outReal[outIdx++] = 0.0;
    }
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

/**
 * Forecast Oscillator: the percentage by which the close deviates from the Time
 * Series Forecast that was made one bar earlier for the current bar. Positive
 * means price came in above what the regression projected, negative below; the
 * value oscillates around zero and crosses it whenever price meets its own
 * forecast. Persistent readings far from zero say the trend is running ahead of,
 * or lagging, its regression line.
 *
 * Formula and more info at
 * [ta-lib.org/functions/fosc](https://ta-lib.org/functions/fosc).
 *
 * **Notes**
 * - **The input is assumed positive.** The current value is the divisor. Where it
 *   is exactly 0 the output is 0. Near zero the divisor is small and the output
 *   very large; no error is returned. A series that reaches or crosses zero, such
 *   as an oscillator or a spread, has no meaningful result here.
 * - Several vendors publish a "Chande Forecast Oscillator (CFO)" that compares the
 *   close to the regression value of the window *ending at the same bar*, with no
 *   lag. FOSC is the lagged form Chande and Achelis describe.
 * - The default window is Chande's own suggestion, shorter than the one TA-Lib's
 *   TSF and LINEARREG default to.
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link foscLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - Source price/value series.
 * @param optInTimePeriod - Number of bars in the regression window (default 5;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Percentage deviation of the close from the previous bar's
 *   forecast. Must hold the values produced and never be empty; the input length
 *   always suffices.
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
 * import { fosc } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const { begIdx, count } = fosc(0, data.length - 1, data, 5, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`tsf`](https://ta-lib.org/functions/tsf)
 * @see [`linearreg`](https://ta-lib.org/functions/linearreg)
 * @see [`cmou`](https://ta-lib.org/functions/cmou)
 */
export function fosc(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('FOSC', 'startIdx', startIdx);
  endIdx = requireInt32('FOSC', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('FOSC', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('FOSC', startIdx, endIdx);
  const guardStart = clampedStart('FOSC', startIdx, foscLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('FOSC', 'inReal', inReal, guardInLen);
  requireOutput('FOSC', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw failure('FOSC', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = foscImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('FOSC', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live FOSC stream: one value per closed bar, bit-identical to {@link fosc} over
 * the same series. Open it with {@link foscOpen} or {@link foscOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/fosc](https://ta-lib.org/functions/fosc).
 */
export class FoscStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  lookbackTotal: number = 0;
  /** @internal */
  trailingIdx: number = 0;
  /** @internal */
  SumX: number = 0;
  /** @internal */
  SumXY: number = 0;
  /** @internal */
  SumY: number = 0;
  /** @internal */
  Divisor: number = 0;
  /** @internal */
  barsSinceReseed: number = 0;
  /** @internal */
  trailingValue: number = 0;
  /** @internal */
  sumAbs: number = 0;
  /** @internal */
  j: number = 0;
  /** @internal */
  today: number = 0;
  /** @internal */
  lag1_inReal: number = 0;
  /** @internal */
  xMask: number = 0;
  /** @internal */
  x_inReal: Float64Array = new Float64Array(1);
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
  static of(core: Core): FoscStream {
    return new FoscStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link fosc} reports them over the same bars. Each accepted `update` and each
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
      throw failure('FOSC advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: Source price/value series.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('FOSC update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('FOSC update', 'inReal');
    }
    foscStepImpl(this, inReal);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Source price/value series.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number): number {
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('FOSC peek', 'inReal');
    }
    const sp = this;
    let m = 0;
    let b = 0;
    let closeValue = 0;
    let windowStart = 0;
    let tempValue1 = 0;
    let tempValue2 = 0;
    let weightedTrailing = 0;
    let SumXY = sp.SumXY;
    let SumY = sp.SumY;
    let barsSinceReseed = sp.barsSinceReseed;
    let cur_outReal = 0;
    let j = sp.j;
    let sumAbs = sp.sumAbs;
    let trailingIdx = sp.trailingIdx;
    let trailingValue = sp.trailingValue;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    pkSlot0 = sp.today & sp.xMask;
    pkVal0 = inReal;
    weightedTrailing = sp.optInTimePeriod * trailingValue;
    SumXY = SumXY + SumY - weightedTrailing;
    SumY = SumY - trailingValue + ((((sp.today - 1) & sp.xMask) !== pkSlot0) ? sp.x_inReal[(sp.today - 1) & sp.xMask] : pkVal0);
    sumAbs = sumAbs - Math.abs(trailingValue) + Math.abs((((sp.today - 1) & sp.xMask) !== pkSlot0) ? sp.x_inReal[(sp.today - 1) & sp.xMask] : pkVal0);
    barsSinceReseed -= 1;
    if (barsSinceReseed <= 0 || Math.abs(weightedTrailing) > 100.0 * sumAbs) {
      barsSinceReseed = ((32 * sp.optInTimePeriod) | 0);
      windowStart = sp.today - sp.lookbackTotal;
      SumY = 0;
      SumXY = 0;
      sumAbs = 0;
      tempValue2 = (sp.optInTimePeriod - 1);
      for (j = windowStart; j < sp.today; j += 1) {
        tempValue1 = ((j & sp.xMask) !== pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0;
        SumY += tempValue1;
        SumXY += tempValue2 * tempValue1;
        sumAbs += Math.abs(tempValue1);
        tempValue2 -= 1.0;
      }
    }
    m = (sp.optInTimePeriod * SumXY - sp.SumX * SumY) / sp.Divisor;
    b = (SumY - m * sp.SumX) / sp.optInTimePeriod;
    trailingValue = ((trailingIdx & sp.xMask) !== pkSlot0) ? sp.x_inReal[trailingIdx & sp.xMask] : pkVal0;
    trailingIdx += 1;
    closeValue = ((sp.today & sp.xMask) !== pkSlot0) ? sp.x_inReal[sp.today & sp.xMask] : pkVal0;
    if (closeValue !== 0.0) {
      cur_outReal = 100.0 * (closeValue - (fma(m, sp.optInTimePeriod, b))) / closeValue;
    } else {
      cur_outReal = 0.0;
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
  clone(): FoscStream {
    const c = new FoscStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.lookbackTotal = this.lookbackTotal;
    c.trailingIdx = this.trailingIdx;
    c.SumX = this.SumX;
    c.SumXY = this.SumXY;
    c.SumY = this.SumY;
    c.Divisor = this.Divisor;
    c.barsSinceReseed = this.barsSinceReseed;
    c.trailingValue = this.trailingValue;
    c.sumAbs = this.sumAbs;
    c.j = this.j;
    c.today = this.today;
    c.lag1_inReal = this.lag1_inReal;
    c.xMask = this.xMask;
    c.x_inReal = this.x_inReal.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function foscStepImpl(sp: FoscStream, inReal: number): void {
  let m = 0;
  let b = 0;
  let closeValue = 0;
  let windowStart = 0;
  let tempValue1 = 0;
  let tempValue2 = 0;
  let weightedTrailing = 0;
  sp.x_inReal[sp.today & sp.xMask] = inReal;
  weightedTrailing = sp.optInTimePeriod * sp.trailingValue;
  sp.SumXY = sp.SumXY + sp.SumY - weightedTrailing;
  sp.SumY = sp.SumY - sp.trailingValue + sp.x_inReal[(sp.today - 1) & sp.xMask];
  sp.sumAbs = sp.sumAbs - Math.abs(sp.trailingValue) + Math.abs(sp.x_inReal[(sp.today - 1) & sp.xMask]);
  sp.barsSinceReseed -= 1;
  if (sp.barsSinceReseed <= 0 || Math.abs(weightedTrailing) > 100.0 * sp.sumAbs) {
    sp.barsSinceReseed = ((32 * sp.optInTimePeriod) | 0);
    windowStart = sp.today - sp.lookbackTotal;
    sp.SumY = 0;
    sp.SumXY = 0;
    sp.sumAbs = 0;
    tempValue2 = (sp.optInTimePeriod - 1);
    for (sp.j = windowStart; sp.j < sp.today; sp.j += 1) {
      tempValue1 = sp.x_inReal[sp.j & sp.xMask];
      sp.SumY += tempValue1;
      sp.SumXY += tempValue2 * tempValue1;
      sp.sumAbs += Math.abs(tempValue1);
      tempValue2 -= 1.0;
    }
  }
  m = (sp.optInTimePeriod * sp.SumXY - sp.SumX * sp.SumY) / sp.Divisor;
  b = (sp.SumY - m * sp.SumX) / sp.optInTimePeriod;
  sp.trailingValue = sp.x_inReal[sp.trailingIdx & sp.xMask];
  sp.trailingIdx += 1;
  closeValue = sp.x_inReal[sp.today & sp.xMask];
  if (closeValue !== 0.0) {
    sp.cur_outReal = 100.0 * (closeValue - (fma(m, sp.optInTimePeriod, b))) / closeValue;
  } else {
    sp.cur_outReal = 0.0;
  }
  sp.today += 1;
  sp.lag1_inReal = inReal;
}

function foscOpenImpl(
  sp: FoscStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let outIdx = 0;
  let today = 0;
  let lookbackTotal = 0;
  let trailingIdx = 0;
  let SumX = 0;
  let SumXY = 0;
  let SumY = 0;
  let SumXSqr = 0;
  let Divisor = 0;
  let m = 0;
  let b = 0;
  let closeValue = 0;
  let i = 0;
  let j = 0;
  let windowStart = 0;
  let barsSinceReseed = 0;
  let tempValue1 = 0;
  let tempValue2 = 0;
  let trailingValue = 0;
  let weightedTrailing = 0;
  let sumAbs = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 5;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* The forecast compared against the close is the one made one bar EARLIER:
   * FOSC[t] = 100*(inReal[t] - TSF[t-1])/inReal[t]. The regression window
   * ends at t-1, so the lookback is period, not period-1.
   *
   * The window arithmetic below is TA_TSF's verbatim -- priming scan, O(1)
   * recurrence, both re-anchor triggers (#254) and the fused
   * `b + m*(double)optInTimePeriod` -- which is what makes FOSC bit-identical
   * to a TA_TSF call anchored one bar earlier. Reshaping any of it breaks
   * that silently.
   *
   * trailingValue -- the value the NEXT bar's window drops -- is read before
   * the output write because with outReal==inReal (#130) that write lands on
   * exactly that cell whenever startIdx is the clamped minimum. closeValue
   * carries no such constraint: it sits startIdx bars ahead of the cursor.
   */
  lookbackTotal = foscLookback(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outIdx = 0;
  today = startIdx;
  trailingIdx = startIdx - lookbackTotal;
  SumX = optInTimePeriod * (optInTimePeriod - 1) * 0.5;
  SumXSqr = optInTimePeriod * (optInTimePeriod - 1) * (((2 * optInTimePeriod) | 0) - 1) / 6.0;
  Divisor = SumX * SumX - optInTimePeriod * SumXSqr;
  SumXY = 0;
  SumY = 0;
  sumAbs = 0;
  for (i = optInTimePeriod; i-- !== 0; ) {
    tempValue1 = inReal[today - 1 - i];
    SumY += tempValue1;
    SumXY += i * tempValue1;
    sumAbs += Math.abs(tempValue1);
  }
  m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
  b = (SumY - m * SumX) / optInTimePeriod;
  barsSinceReseed = ((32 * optInTimePeriod) | 0);
  trailingValue = inReal[trailingIdx];
  trailingIdx += 1;
  closeValue = inReal[today];
  if (closeValue !== 0.0) {
    outReal[((outIdx++ * outStride) | 0)] = 100.0 * (closeValue - (fma(m, optInTimePeriod, b))) / closeValue;
  } else {
    outReal[((outIdx++ * outStride) | 0)] = 0.0;
  }
  today += 1;
  while (today <= endIdx) {
    weightedTrailing = optInTimePeriod * trailingValue;
    SumXY = SumXY + SumY - weightedTrailing;
    SumY = SumY - trailingValue + inReal[today - 1];
    sumAbs = sumAbs - Math.abs(trailingValue) + Math.abs(inReal[today - 1]);
    barsSinceReseed -= 1;
    if (barsSinceReseed <= 0 || Math.abs(weightedTrailing) > 100.0 * sumAbs) {
      barsSinceReseed = ((32 * optInTimePeriod) | 0);
      windowStart = today - lookbackTotal;
      SumY = 0;
      SumXY = 0;
      sumAbs = 0;
      tempValue2 = (optInTimePeriod - 1);
      for (j = windowStart; j < today; j += 1) {
        tempValue1 = inReal[j];
        SumY += tempValue1;
        SumXY += tempValue2 * tempValue1;
        sumAbs += Math.abs(tempValue1);
        tempValue2 -= 1.0;
      }
    }
    m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
    b = (SumY - m * SumX) / optInTimePeriod;
    trailingValue = inReal[trailingIdx];
    trailingIdx += 1;
    closeValue = inReal[today];
    if (closeValue !== 0.0) {
      let _fma0: number;
      {
        const _a: number = m, _b: number = optInTimePeriod, _c: number = b;
        const _p = _a * _b;
        let _t = 134217729 * _a;
        const _ah = _t - (_t - _a), _al = _a - _ah;
        _t = 134217729 * _b;
        const _bh = _t - (_t - _b), _bl = _b - _bh;
        const _pl = ((_ah * _bh - _p) + _ah * _bl + _al * _bh) + _al * _bl;
        const _th = _c + _p, _z = _th - _c, _tl = (_c - (_th - _z)) + (_p - _z);
        const _v = _tl + _pl, _v2 = _v + _v;
        const _w = _a * _a + _b * _b + _c * _c < 2 ** 1000 && _p * _p > 2 ** -1000;
        _fma0 = _w && (_pl === 0 || _v === 0) ? _th
          : _w && _v2 !== (_th + _v2) - _th ? _th + _v
          : _a === 0 || _b === 0 ? _p + _c : fmaSlow(_a, _b, _c, _th, _tl, _pl);
      }
      outReal[((outIdx++ * outStride) | 0)] = 100.0 * (closeValue - _fma0) / closeValue;
    } else {
      outReal[((outIdx++ * outStride) | 0)] = 0.0;
    }
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  const capX = today - trailingIdx + 1;
  if (capX < 1 || capX > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  let physX = 1;
  while (physX < capX) {
    physX <<= 1;
  }
  const capX_inReal = new Float64Array(physX);
  for (let fillJ = historyLen - capX; fillJ < historyLen; fillJ++) {
    capX_inReal[fillJ & (physX - 1)] = inReal[fillJ];
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.lookbackTotal = lookbackTotal;
  sp.trailingIdx = trailingIdx;
  sp.SumX = SumX;
  sp.SumXY = SumXY;
  sp.SumY = SumY;
  sp.Divisor = Divisor;
  sp.barsSinceReseed = barsSinceReseed;
  sp.trailingValue = trailingValue;
  sp.sumAbs = sumAbs;
  sp.j = j;
  sp.today = today;
  sp.lag1_inReal = inReal[historyLen - 1];
  sp.xMask = physX - 1;
  sp.x_inReal = capX_inReal;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function foscOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): FoscStream {
  const sp = FoscStream.of(core);
  const retCode = foscOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('FOSC openAndFill', inReal.length, startIdx, foscLookback(optInTimePeriod, core));
  }
  throw streamFailure('FOSC openAndFill', retCode);
}

/** @internal */
export function foscOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): FoscStream {
  const sp = FoscStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = foscOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('FOSC open', inReal.length, startIdx, foscLookback(optInTimePeriod, core));
  }
  throw streamFailure('FOSC open', retCode);
}

/**
 * Opens a live FOSC stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link fosc} there.
 *
 * @param inReal - Warm-up history. Source price/value series.
 * @param optInTimePeriod - Number of bars in the regression window (default 5;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `foscLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32 or a
 *   parameter outside its range.
 *
 * @example
 * ```ts
 * import { foscOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = foscOpen(data, 5);
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function foscOpen(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): FoscStream {
  optInTimePeriod = requireInt32('FOSC open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('FOSC open', 'inReal', inReal);
  requireHistory('FOSC open', inReal.length);
  return foscOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/**
 * {@link foscOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link fosc}. The range written is the stream's
 * `outRange()`.
 *
 * @param inReal - Warm-up history. Source price/value series.
 * @param optInTimePeriod - Number of bars in the regression window (default 5;
 *   range 2 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Percentage deviation of the close from the previous bar's
 *   forecast. Must hold `historyLen - lookback` values and share no memory with an
 *   input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `foscLookback(...) + 1` bars, `TA_OUT_OF_RANGE_START_INDEX` for an empty
 *   one, and `TA_BAD_PARAM` for an integer parameter that is not an int32, a
 *   parameter outside its range or an output too short or starting where another
 *   output or a `Float64Array` input starts, or, on a history long enough to open,
 *   one sharing memory with another output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { foscOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const stream = foscOpenAndFill(data, 5, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function foscOpenAndFill(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): FoscStream {
  optInTimePeriod = requireInt32('FOSC openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('FOSC openAndFill', 'inReal', inReal);
  requireHistory('FOSC openAndFill', inReal.length);
  const guardOutLen = openFillCount('FOSC openAndFill', inReal.length, foscLookback(optInTimePeriod, core));
  requireOutput('FOSC openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if ((outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset)) {
    throw streamFailure('FOSC openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inRealF64))) {
    throw streamFailure('FOSC openAndFill', RetCode.BAD_PARAM);
  }
  return foscOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}
