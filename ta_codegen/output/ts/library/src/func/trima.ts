/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CR       Chris (crokusek@hotmail.com)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  010503 MF     Initial Coding
 *  031703 MF     Fix #701060. Correct logic when using a range with
 *                startIdx/endIdx. Thanks to Chris for reporting this.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  071226 MF,CC  Widen the triangular-weight factor to double: (i+1)*(i+1)
 *                and i*(i+1) overflowed a 32-bit int at extreme periods
 *                (past ~92682), silently returning garbage. Bit-identical
 *                for every period where the int product fits.
 */

import { clampedStart, failure, float64, insufficientHistory, nonFiniteBar, openFillCount, overlaps, overlapsPartly, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/**
 * Number of leading input bars {@link trima} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param optInTimePeriod - Number of bars in the averaging window (default 30;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The lookback, or -1 if a parameter is out of range.
 */
export function trimaLookback(optInTimePeriod: number, core: Core = Core.DEFAULT): number {
  void core;
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  return optInTimePeriod - 1;
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link trima}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param optInTimePeriod - Number of bars in the averaging window (default 30;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outputIdx - Position of the output in the batch signature, from 0.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The display shift, or -2147483648 if a parameter is out of range or the
 *   index names no output.
 */
export function trimaDisplayShift(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (trimaLookback(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function trimaImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let lookbackTotal = 0;
  let numerator = 0;
  let numeratorSub = 0;
  let numeratorAdd = 0;
  let i = 0;
  let outIdx = 0;
  let todayIdx = 0;
  let trailingIdx = 0;
  let middleIdx = 0;
  let factor = 0;
  let tempReal = 0;
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
  /* Identify the minimum number of price bar needed
   * to calculate at least one output.
   */
  lookbackTotal = optInTimePeriod - 1;
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
  /* TRIMA Description
   * =================
   * The triangular MA is a weighted moving average. Instead of the
   * TA_WMA who put more weigth on the latest price bar, the triangular
   * put more weigth on the data in the middle of the specified period.
   *
   * Examples:
   *   For TimeSerie={a,b,c,d,e,f...} ('a' is the older price)
   *
   *   1st value for TRIMA 4-Period is:  ((1*a)+(2*b)+(2*c)+(1*d)) / 6
   *   2nd value for TRIMA 4-Period is:  ((1*b)+(2*c)+(2*d)+(1*e)) / 6
   *
   *   1st value for TRIMA 5-Period is:  ((1*a)+(2*b)+(3*c)+(2*d)+(1*e)) / 9
   *   2nd value for TRIMA 5-Period is:  ((1*b)+(2*c)+(3*d)+(2*e)+(1*f)) / 9
   *
   * Generally Accepted Implementation
   * ==================================
   * Using algebra, it can be demonstrated that the TRIMA is equivalent to
   * doing a SMA of a SMA. The following explain the rules:
   *
   *  (1) When the period is even, TRIMA(x,period)=SMA(SMA(x,period/2),(period/2)+1)
   *  (2) When the period is odd,  TRIMA(x,period)=SMA(SMA(x,(period+1)/2),(period+1)/2)
   *
   * In other word:
   *  (1) A period of 4 becomes TRIMA(x,4) = SMA( SMA( x, 2), 3 )
   *  (2) A period of 5 becomes TRIMA(x,5) = SMA( SMA( x, 3), 3 )
   *
   * The SMA of a SMA is the algorithm generaly found in books.
   *
   * Tradestation Implementation
   * ===========================
   * Tradestation deviate from the generally accepted implementation by
   * making the TRIMA to be as follow:
   *    TRIMA(x,period) = SMA( SMA( x, (int)(period/2)+1), (int)(period/2)+1 );
   * This formula is done regardless if the period is even or odd.
   *
   * In other word:
   *  (1) A period of 4 becomes TRIMA(x,4) = SMA( SMA( x, 3), 3 )
   *  (2) A period of 5 becomes TRIMA(x,5) = SMA( SMA( x, 3), 3 )
   *  (3) A period of 6 becomes TRIMA(x,5) = SMA( SMA( x, 4), 4 )
   *  (4) A period of 7 becomes TRIMA(x,5) = SMA( SMA( x, 4), 4 )
   *
   * It is not clear to me if the Tradestation approach is a bug or a deliberate
   * decision to do things differently.
   *
   * Metastock Implementation
   * ========================
   * Output is the same as the generally accepted implementation.
   *
   * TA-Lib Implementation
   * =====================
   * Output is also the same as the generally accepted implementation.
   *
   * For speed optimization and avoid memory allocation, TA-Lib use
   * a better algorithm than the usual SMA of a SMA.
   *
   * The calculation from one TRIMA value to the next is done by doing 4
   * little adjustment (the following show a TRIMA 4-period):
   *
   * TRIMA at time 'd': ((1*a)+(2*b)+(2*c)+(1*d)) / 6
   * TRIMA at time 'e': ((1*b)+(2*c)+(2*d)+(1*e)) / 6
   *
   * To go from TRIMA 'd' to 'e', the following is done:
   *       1) 'a' and 'b' are substract from the numerator.
   *       2) 'd' is added to the numerator.
   *       3) 'e' is added to the numerator.
   *       4) Calculate TRIMA by doing numerator / 6
   *       5) Repeat sequence for next output
   *
   * These operations are the same steps done by TA-LIB:
   *       1) is done by numeratorSub
   *       2) is done by numeratorAdd.
   *       3) is obtain from the latest input
   *       4) Calculate and write TRIMA in the output
   *       5) Repeat for next output.
   *
   * Of course, numerotrAdd and numeratorSub needs to be
   * adjusted for each iteration.
   *
   * The update of numeratorSub needs values from the input at
   * the trailingIdx and middleIdx position.
   *
   * The update of numeratorAdd needs values from the input at
   * the middleIdx and todayIdx.
   */
  outIdx = 0;
  if (((optInTimePeriod % 2) | 0) === 1) {
    /* Logic for Odd period */
    /* Calculate the factor which is 1 divided by the
     * sumation of the weight.
     *
     * The sum of the weight is calculated as follow:
     *
     * The simple sumation serie 1+2+3... n can be
     * express as n(n+1)/2
     *
     * From this logic, a "triangular" sumation formula
     * can be found depending if the period is odd or even.
     *
     * Odd Period Formula:
     *  period = 5 and with n=(int)(period/2)
     *  the formula for a "triangular" serie is:
     *    1+2+3+2+1 = (n*(n+1))+n+1
     *              = (n+1)*(n+1)
     *              = 3 * 3 = 9
     *
     * Even period Formula:
     *   period = 6 and with n=(int)(period/2)
     *   the formula for a "triangular" serie is:
     *    1+2+3+3+2+1 = n*(n+1)
     *                = 3 * 4 = 12
     */
    /* Note: the (i+1) factors are widened to double so the product
     *       cannot overflow a 32-bit int at extreme periods (i+1 reaches
     *       ~50000 near the API maximum, and (i+1)*(i+1) exceeds INT_MAX
     *       past period ~92682). For every period where the int product
     *       fits, the widened value is identical.
     */
    i = optInTimePeriod >> 1;
    factor = (i + 1) * (i + 1);
    factor = 1.0 / factor;
    /* Initialize all the variable before
     * starting to iterate for each output.
     */
    trailingIdx = startIdx - lookbackTotal;
    middleIdx = trailingIdx + i;
    todayIdx = middleIdx + i;
    numerator = 0.0;
    numeratorSub = 0.0;
    for (i = middleIdx; i >= trailingIdx; i -= 1) {
      tempReal = inReal[i];
      numeratorSub += tempReal;
      numerator += numeratorSub;
    }
    numeratorAdd = 0.0;
    middleIdx += 1;
    for (i = middleIdx; i <= todayIdx; i += 1) {
      tempReal = inReal[i];
      numeratorAdd += tempReal;
      numerator += numeratorAdd;
    }
    /* Write the first output */
    outIdx = 0;
    tempReal = inReal[trailingIdx++];
    outReal[outIdx++] = numerator * factor;
    todayIdx += 1;
    /* Note: The value at the trailingIdx was saved
     *       in tempReal to account for the case where
     *       outReal and inReal are ptr on the same
     *       buffer.
     */
    /* Iterate for remaining output */
    while (todayIdx <= endIdx) {
      /* Step (1) */
      numerator -= numeratorSub;
      numeratorSub -= tempReal;
      tempReal = inReal[middleIdx++];
      numeratorSub += tempReal;
      /* Step (2) */
      numerator += numeratorAdd;
      numeratorAdd -= tempReal;
      tempReal = inReal[todayIdx++];
      numeratorAdd += tempReal;
      /* Step (3) */
      numerator += tempReal;
      /* Step (4) */
      tempReal = inReal[trailingIdx++];
      outReal[outIdx++] = numerator * factor;
    }
  } else {
    /* Even logic.
     *
     * Very similar to the odd logic, except:
     *  - calculation of the factor is different.
     *  - the coverage of the numeratorSub and numeratorAdd is
     *    slightly different.
     *  - Adjustment of numeratorAdd is different. See Step (2).
     */
    i = optInTimePeriod >> 1;
    factor = i * (i + 1);
    /* widen: i*(i+1) overflows int past period ~92682 */
    factor = 1.0 / factor;
    /* Initialize all the variable before
     * starting to iterate for each output.
     */
    trailingIdx = startIdx - lookbackTotal;
    middleIdx = trailingIdx + i - 1;
    todayIdx = middleIdx + i;
    numerator = 0.0;
    numeratorSub = 0.0;
    for (i = middleIdx; i >= trailingIdx; i -= 1) {
      tempReal = inReal[i];
      numeratorSub += tempReal;
      numerator += numeratorSub;
    }
    numeratorAdd = 0.0;
    middleIdx += 1;
    for (i = middleIdx; i <= todayIdx; i += 1) {
      tempReal = inReal[i];
      numeratorAdd += tempReal;
      numerator += numeratorAdd;
    }
    /* Write the first output */
    outIdx = 0;
    tempReal = inReal[trailingIdx++];
    outReal[outIdx++] = numerator * factor;
    todayIdx += 1;
    /* Note: The value at the trailingIdx was saved
     *       in tempReal to account for the case where
     *       outReal and inReal are ptr on the same
     *       buffer.
     */
    /* Iterate for remaining output */
    while (todayIdx <= endIdx) {
      /* Step (1) */
      numerator -= numeratorSub;
      numeratorSub -= tempReal;
      tempReal = inReal[middleIdx++];
      numeratorSub += tempReal;
      /* Step (2) */
      numeratorAdd -= tempReal;
      numerator += numeratorAdd;
      tempReal = inReal[todayIdx++];
      numeratorAdd += tempReal;
      /* Step (3) */
      numerator += tempReal;
      /* Step (4) */
      tempReal = inReal[trailingIdx++];
      outReal[outIdx++] = numerator * factor;
    }
  }
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

/**
 * Triangular Moving Average: a double-smoothed moving average that weights prices
 * toward the middle of the window most heavily. Equivalent to an SMA of an SMA,
 * computed here via an incremental triangular-weighted running numerator.
 *
 * Formula and more info at
 * [ta-lib.org/functions/trima](https://ta-lib.org/functions/trima).
 *
 * **Notes**
 * - Follows the generally accepted (Metastock) definition rather than the
 *   TradeStation variant.
 * - A period of 1 performs no smoothing: the output is a copy of the input.
 *   Allowed since 0.6.5 (issues #48/#59).
 *
 * Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing
 * is padded. A range that ends before {@link trimaLookback} is a success with
 * `count` 0.
 *
 * @param startIdx - First bar of the requested range (inclusive).
 * @param endIdx - Last bar of the requested range (inclusive).
 * @param inReal - Source price series.
 * @param optInTimePeriod - Number of bars in the averaging window (default 30;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Triangular moving average. Must hold the values produced and
 *   never be empty; the input length always suffices.
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
 * import { trima } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const { begIdx, count } = trima(0, data.length - 1, data, 30, out);
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 * @see [`sma`](https://ta-lib.org/functions/sma)
 * @see [`wma`](https://ta-lib.org/functions/wma)
 * @see [`ma`](https://ta-lib.org/functions/ma)
 */
export function trima(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('TRIMA', 'startIdx', startIdx);
  endIdx = requireInt32('TRIMA', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('TRIMA', 'optInTimePeriod', optInTimePeriod);
  requireIndexRange('TRIMA', startIdx, endIdx);
  const guardStart = clampedStart('TRIMA', startIdx, trimaLookback(optInTimePeriod, core));
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('TRIMA', 'inReal', inReal, guardInLen);
  requireOutput('TRIMA', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw failure('TRIMA', RetCode.BAD_PARAM);
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = trimaImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('TRIMA', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**** Streaming API *****/

/**
 * A live TRIMA stream: one value per closed bar, bit-identical to {@link trima}
 * over the same series. Open it with {@link trimaOpen} or
 * {@link trimaOpenAndFill}; there is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/trima](https://ta-lib.org/functions/trima).
 */
export class TrimaStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  numerator: number = 0;
  /** @internal */
  numeratorSub: number = 0;
  /** @internal */
  numeratorAdd: number = 0;
  /** @internal */
  factor: number = 0;
  /** @internal */
  tempReal: number = 0;
  /** @internal */
  ringPos_middleIdx: number = 0;
  /** @internal */
  ringCap_middleIdx: number = 0;
  /** @internal */
  ring_middleIdx_inReal: Float64Array = new Float64Array(1);
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
  static of(core: Core): TrimaStream {
    return new TrimaStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link trima} reports them over the same bars. Each accepted `update` and each
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
      throw failure('TRIMA advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal - The bar's `inReal` value: Source price series.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('TRIMA update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('TRIMA update', 'inReal');
    }
    trimaStepImpl(this, inReal);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal - The bar's `inReal` value: Source price series.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal: number): number {
    if (!Number.isFinite(inReal)) {
      throw nonFiniteBar('TRIMA peek', 'inReal');
    }
    const sp = this;
    let cur_outReal = 0;
    if (((sp.optInTimePeriod % 2) | 0) === 1) {
      let numerator = sp.numerator;
      let numeratorAdd = sp.numeratorAdd;
      let numeratorSub = sp.numeratorSub;
      let tempReal = sp.tempReal;
      let pkSlot0 = -1;
      let pkVal0 = 0;
      let pkSlot1 = -1;
      let pkVal1 = 0;
      if (sp.ringCap_middleIdx === 0) {
        pkSlot0 = 0;
        pkVal0 = inReal;
      }
      if (sp.ringCap_trailingIdx === 0) {
        pkSlot1 = 0;
        pkVal1 = inReal;
      }
      /* Step (1) */
      numerator -= numeratorSub;
      numeratorSub -= tempReal;
      tempReal = (sp.ringPos_middleIdx !== pkSlot0) ? sp.ring_middleIdx_inReal[sp.ringPos_middleIdx] : pkVal0;
      numeratorSub += tempReal;
      /* Step (2) */
      numerator += numeratorAdd;
      numeratorAdd -= tempReal;
      tempReal = inReal;
      numeratorAdd += tempReal;
      /* Step (3) */
      numerator += tempReal;
      /* Step (4) */
      tempReal = (sp.ringPos_trailingIdx !== pkSlot1) ? sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] : pkVal1;
      cur_outReal = numerator * sp.factor;
    } else {
      let numerator = sp.numerator;
      let numeratorAdd = sp.numeratorAdd;
      let numeratorSub = sp.numeratorSub;
      let tempReal = sp.tempReal;
      /* Step (1) */
      numerator -= numeratorSub;
      numeratorSub -= tempReal;
      tempReal = sp.ring_middleIdx_inReal[sp.ringPos_middleIdx];
      numeratorSub += tempReal;
      /* Step (2) */
      numeratorAdd -= tempReal;
      numerator += numeratorAdd;
      tempReal = inReal;
      numeratorAdd += tempReal;
      /* Step (3) */
      numerator += tempReal;
      /* Step (4) */
      tempReal = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
      cur_outReal = numerator * sp.factor;
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
  clone(): TrimaStream {
    const c = new TrimaStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.numerator = this.numerator;
    c.numeratorSub = this.numeratorSub;
    c.numeratorAdd = this.numeratorAdd;
    c.factor = this.factor;
    c.tempReal = this.tempReal;
    c.ringPos_middleIdx = this.ringPos_middleIdx;
    c.ringCap_middleIdx = this.ringCap_middleIdx;
    c.ring_middleIdx_inReal = this.ring_middleIdx_inReal.slice();
    c.ringPos_trailingIdx = this.ringPos_trailingIdx;
    c.ringCap_trailingIdx = this.ringCap_trailingIdx;
    c.ring_trailingIdx_inReal = this.ring_trailingIdx_inReal.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function trimaStepImpl(sp: TrimaStream, inReal: number): void {
  if (((sp.optInTimePeriod % 2) | 0) === 1) {
    if (sp.ringCap_middleIdx === 0) {
      sp.ring_middleIdx_inReal[0] = inReal;
    }
    if (sp.ringCap_trailingIdx === 0) {
      sp.ring_trailingIdx_inReal[0] = inReal;
    }
    /* Step (1) */
    sp.numerator -= sp.numeratorSub;
    sp.numeratorSub -= sp.tempReal;
    sp.tempReal = sp.ring_middleIdx_inReal[sp.ringPos_middleIdx];
    sp.numeratorSub += sp.tempReal;
    /* Step (2) */
    sp.numerator += sp.numeratorAdd;
    sp.numeratorAdd -= sp.tempReal;
    sp.tempReal = inReal;
    sp.numeratorAdd += sp.tempReal;
    /* Step (3) */
    sp.numerator += sp.tempReal;
    /* Step (4) */
    sp.tempReal = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
    sp.cur_outReal = sp.numerator * sp.factor;
    sp.ring_middleIdx_inReal[sp.ringPos_middleIdx] = inReal;
    sp.ringPos_middleIdx = sp.ringPos_middleIdx + 1;
    if (sp.ringPos_middleIdx >= sp.ringCap_middleIdx) {
      sp.ringPos_middleIdx = 0;
    }
    sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
    sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
    if (sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx) {
      sp.ringPos_trailingIdx = 0;
    }
  } else {
    let ringCapL_middleIdx = 0;
    let ringCapL_trailingIdx = 0;
    /* Step (1) */
    sp.numerator -= sp.numeratorSub;
    sp.numeratorSub -= sp.tempReal;
    sp.tempReal = sp.ring_middleIdx_inReal[sp.ringPos_middleIdx];
    sp.numeratorSub += sp.tempReal;
    /* Step (2) */
    sp.numeratorAdd -= sp.tempReal;
    sp.numerator += sp.numeratorAdd;
    sp.tempReal = inReal;
    sp.numeratorAdd += sp.tempReal;
    /* Step (3) */
    sp.numerator += sp.tempReal;
    /* Step (4) */
    sp.tempReal = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
    sp.cur_outReal = sp.numerator * sp.factor;
    ringCapL_middleIdx = sp.ringCap_middleIdx;
    sp.ring_middleIdx_inReal[sp.ringPos_middleIdx] = inReal;
    sp.ringPos_middleIdx = sp.ringPos_middleIdx + 1;
    if (sp.ringPos_middleIdx >= ringCapL_middleIdx) {
      sp.ringPos_middleIdx = 0;
    }
    ringCapL_trailingIdx = sp.ringCap_trailingIdx;
    sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
    sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
    if (sp.ringPos_trailingIdx >= ringCapL_trailingIdx) {
      sp.ringPos_trailingIdx = 0;
    }
  }
}

function trimaOpenImpl(
  sp: TrimaStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen < 1) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 30;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (((optInTimePeriod % 2) | 0) === 1) {
    let lookbackTotal = 0;
    let numerator = 0;
    let numeratorSub = 0;
    let numeratorAdd = 0;
    let i = 0;
    let outIdx = 0;
    let todayIdx = 0;
    let trailingIdx = 0;
    let middleIdx = 0;
    let factor = 0;
    let tempReal = 0;
    /* Identify the minimum number of price bar needed
     * to calculate at least one output.
     */
    lookbackTotal = optInTimePeriod - 1;
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
    /* TRIMA Description
     * =================
     * The triangular MA is a weighted moving average. Instead of the
     * TA_WMA who put more weigth on the latest price bar, the triangular
     * put more weigth on the data in the middle of the specified period.
     *
     * Examples:
     *   For TimeSerie={a,b,c,d,e,f...} ('a' is the older price)
     *
     *   1st value for TRIMA 4-Period is:  ((1*a)+(2*b)+(2*c)+(1*d)) / 6
     *   2nd value for TRIMA 4-Period is:  ((1*b)+(2*c)+(2*d)+(1*e)) / 6
     *
     *   1st value for TRIMA 5-Period is:  ((1*a)+(2*b)+(3*c)+(2*d)+(1*e)) / 9
     *   2nd value for TRIMA 5-Period is:  ((1*b)+(2*c)+(3*d)+(2*e)+(1*f)) / 9
     *
     * Generally Accepted Implementation
     * ==================================
     * Using algebra, it can be demonstrated that the TRIMA is equivalent to
     * doing a SMA of a SMA. The following explain the rules:
     *
     *  (1) When the period is even, TRIMA(x,period)=SMA(SMA(x,period/2),(period/2)+1)
     *  (2) When the period is odd,  TRIMA(x,period)=SMA(SMA(x,(period+1)/2),(period+1)/2)
     *
     * In other word:
     *  (1) A period of 4 becomes TRIMA(x,4) = SMA( SMA( x, 2), 3 )
     *  (2) A period of 5 becomes TRIMA(x,5) = SMA( SMA( x, 3), 3 )
     *
     * The SMA of a SMA is the algorithm generaly found in books.
     *
     * Tradestation Implementation
     * ===========================
     * Tradestation deviate from the generally accepted implementation by
     * making the TRIMA to be as follow:
     *    TRIMA(x,period) = SMA( SMA( x, (int)(period/2)+1), (int)(period/2)+1 );
     * This formula is done regardless if the period is even or odd.
     *
     * In other word:
     *  (1) A period of 4 becomes TRIMA(x,4) = SMA( SMA( x, 3), 3 )
     *  (2) A period of 5 becomes TRIMA(x,5) = SMA( SMA( x, 3), 3 )
     *  (3) A period of 6 becomes TRIMA(x,5) = SMA( SMA( x, 4), 4 )
     *  (4) A period of 7 becomes TRIMA(x,5) = SMA( SMA( x, 4), 4 )
     *
     * It is not clear to me if the Tradestation approach is a bug or a deliberate
     * decision to do things differently.
     *
     * Metastock Implementation
     * ========================
     * Output is the same as the generally accepted implementation.
     *
     * TA-Lib Implementation
     * =====================
     * Output is also the same as the generally accepted implementation.
     *
     * For speed optimization and avoid memory allocation, TA-Lib use
     * a better algorithm than the usual SMA of a SMA.
     *
     * The calculation from one TRIMA value to the next is done by doing 4
     * little adjustment (the following show a TRIMA 4-period):
     *
     * TRIMA at time 'd': ((1*a)+(2*b)+(2*c)+(1*d)) / 6
     * TRIMA at time 'e': ((1*b)+(2*c)+(2*d)+(1*e)) / 6
     *
     * To go from TRIMA 'd' to 'e', the following is done:
     *       1) 'a' and 'b' are substract from the numerator.
     *       2) 'd' is added to the numerator.
     *       3) 'e' is added to the numerator.
     *       4) Calculate TRIMA by doing numerator / 6
     *       5) Repeat sequence for next output
     *
     * These operations are the same steps done by TA-LIB:
     *       1) is done by numeratorSub
     *       2) is done by numeratorAdd.
     *       3) is obtain from the latest input
     *       4) Calculate and write TRIMA in the output
     *       5) Repeat for next output.
     *
     * Of course, numerotrAdd and numeratorSub needs to be
     * adjusted for each iteration.
     *
     * The update of numeratorSub needs values from the input at
     * the trailingIdx and middleIdx position.
     *
     * The update of numeratorAdd needs values from the input at
     * the middleIdx and todayIdx.
     */
    outIdx = 0;
    /* Logic for Odd period */
    /* Calculate the factor which is 1 divided by the
     * sumation of the weight.
     *
     * The sum of the weight is calculated as follow:
     *
     * The simple sumation serie 1+2+3... n can be
     * express as n(n+1)/2
     *
     * From this logic, a "triangular" sumation formula
     * can be found depending if the period is odd or even.
     *
     * Odd Period Formula:
     *  period = 5 and with n=(int)(period/2)
     *  the formula for a "triangular" serie is:
     *    1+2+3+2+1 = (n*(n+1))+n+1
     *              = (n+1)*(n+1)
     *              = 3 * 3 = 9
     *
     * Even period Formula:
     *   period = 6 and with n=(int)(period/2)
     *   the formula for a "triangular" serie is:
     *    1+2+3+3+2+1 = n*(n+1)
     *                = 3 * 4 = 12
     */
    /* Note: the (i+1) factors are widened to double so the product
     *       cannot overflow a 32-bit int at extreme periods (i+1 reaches
     *       ~50000 near the API maximum, and (i+1)*(i+1) exceeds INT_MAX
     *       past period ~92682). For every period where the int product
     *       fits, the widened value is identical.
     */
    i = optInTimePeriod >> 1;
    factor = (i + 1) * (i + 1);
    factor = 1.0 / factor;
    /* Initialize all the variable before
     * starting to iterate for each output.
     */
    trailingIdx = startIdx - lookbackTotal;
    middleIdx = trailingIdx + i;
    todayIdx = middleIdx + i;
    numerator = 0.0;
    numeratorSub = 0.0;
    for (i = middleIdx; i >= trailingIdx; i -= 1) {
      tempReal = inReal[i];
      numeratorSub += tempReal;
      numerator += numeratorSub;
    }
    numeratorAdd = 0.0;
    middleIdx += 1;
    for (i = middleIdx; i <= todayIdx; i += 1) {
      tempReal = inReal[i];
      numeratorAdd += tempReal;
      numerator += numeratorAdd;
    }
    /* Write the first output */
    outIdx = 0;
    tempReal = inReal[trailingIdx++];
    outReal[((outIdx++ * outStride) | 0)] = numerator * factor;
    todayIdx += 1;
    /* Note: The value at the trailingIdx was saved
     *       in tempReal to account for the case where
     *       outReal and inReal are ptr on the same
     *       buffer.
     */
    /* Iterate for remaining output */
    while (todayIdx <= endIdx) {
      /* Step (1) */
      numerator -= numeratorSub;
      numeratorSub -= tempReal;
      tempReal = inReal[middleIdx++];
      numeratorSub += tempReal;
      /* Step (2) */
      numerator += numeratorAdd;
      numeratorAdd -= tempReal;
      tempReal = inReal[todayIdx++];
      numeratorAdd += tempReal;
      /* Step (3) */
      numerator += tempReal;
      /* Step (4) */
      tempReal = inReal[trailingIdx++];
      outReal[((outIdx++ * outStride) | 0)] = numerator * factor;
    }
    outNBElement.value = outIdx;
    outBegIdx.value = startIdx;
    const cap_middleIdx = todayIdx - middleIdx;
    if (cap_middleIdx < 0 || cap_middleIdx > historyLen) {
      return RetCode.INTERNAL_ERROR;
    }
    const allocN_middleIdx = cap_middleIdx > 0 ? cap_middleIdx : 1;
    const capRing_middleIdx_inReal = new Float64Array(allocN_middleIdx);
    capRing_middleIdx_inReal.set(inReal.subarray(historyLen - cap_middleIdx, historyLen), 0);
    const cap_trailingIdx = todayIdx - trailingIdx;
    if (cap_trailingIdx < 0 || cap_trailingIdx > historyLen) {
      return RetCode.INTERNAL_ERROR;
    }
    const allocN_trailingIdx = cap_trailingIdx > 0 ? cap_trailingIdx : 1;
    const capRing_trailingIdx_inReal = new Float64Array(allocN_trailingIdx);
    capRing_trailingIdx_inReal.set(inReal.subarray(historyLen - cap_trailingIdx, historyLen), 0);
    sp.optInTimePeriod = optInTimePeriod;
    sp.numerator = numerator;
    sp.numeratorSub = numeratorSub;
    sp.numeratorAdd = numeratorAdd;
    sp.factor = factor;
    sp.tempReal = tempReal;
    sp.ringPos_middleIdx = 0;
    sp.ringCap_middleIdx = cap_middleIdx;
    sp.ring_middleIdx_inReal = capRing_middleIdx_inReal;
    sp.ringPos_trailingIdx = 0;
    sp.ringCap_trailingIdx = cap_trailingIdx;
    sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
    sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
    return RetCode.SUCCESS;
  } else {
    let lookbackTotal = 0;
    let numerator = 0;
    let numeratorSub = 0;
    let numeratorAdd = 0;
    let i = 0;
    let outIdx = 0;
    let todayIdx = 0;
    let trailingIdx = 0;
    let middleIdx = 0;
    let factor = 0;
    let tempReal = 0;
    /* Identify the minimum number of price bar needed
     * to calculate at least one output.
     */
    lookbackTotal = optInTimePeriod - 1;
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
    /* TRIMA Description
     * =================
     * The triangular MA is a weighted moving average. Instead of the
     * TA_WMA who put more weigth on the latest price bar, the triangular
     * put more weigth on the data in the middle of the specified period.
     *
     * Examples:
     *   For TimeSerie={a,b,c,d,e,f...} ('a' is the older price)
     *
     *   1st value for TRIMA 4-Period is:  ((1*a)+(2*b)+(2*c)+(1*d)) / 6
     *   2nd value for TRIMA 4-Period is:  ((1*b)+(2*c)+(2*d)+(1*e)) / 6
     *
     *   1st value for TRIMA 5-Period is:  ((1*a)+(2*b)+(3*c)+(2*d)+(1*e)) / 9
     *   2nd value for TRIMA 5-Period is:  ((1*b)+(2*c)+(3*d)+(2*e)+(1*f)) / 9
     *
     * Generally Accepted Implementation
     * ==================================
     * Using algebra, it can be demonstrated that the TRIMA is equivalent to
     * doing a SMA of a SMA. The following explain the rules:
     *
     *  (1) When the period is even, TRIMA(x,period)=SMA(SMA(x,period/2),(period/2)+1)
     *  (2) When the period is odd,  TRIMA(x,period)=SMA(SMA(x,(period+1)/2),(period+1)/2)
     *
     * In other word:
     *  (1) A period of 4 becomes TRIMA(x,4) = SMA( SMA( x, 2), 3 )
     *  (2) A period of 5 becomes TRIMA(x,5) = SMA( SMA( x, 3), 3 )
     *
     * The SMA of a SMA is the algorithm generaly found in books.
     *
     * Tradestation Implementation
     * ===========================
     * Tradestation deviate from the generally accepted implementation by
     * making the TRIMA to be as follow:
     *    TRIMA(x,period) = SMA( SMA( x, (int)(period/2)+1), (int)(period/2)+1 );
     * This formula is done regardless if the period is even or odd.
     *
     * In other word:
     *  (1) A period of 4 becomes TRIMA(x,4) = SMA( SMA( x, 3), 3 )
     *  (2) A period of 5 becomes TRIMA(x,5) = SMA( SMA( x, 3), 3 )
     *  (3) A period of 6 becomes TRIMA(x,5) = SMA( SMA( x, 4), 4 )
     *  (4) A period of 7 becomes TRIMA(x,5) = SMA( SMA( x, 4), 4 )
     *
     * It is not clear to me if the Tradestation approach is a bug or a deliberate
     * decision to do things differently.
     *
     * Metastock Implementation
     * ========================
     * Output is the same as the generally accepted implementation.
     *
     * TA-Lib Implementation
     * =====================
     * Output is also the same as the generally accepted implementation.
     *
     * For speed optimization and avoid memory allocation, TA-Lib use
     * a better algorithm than the usual SMA of a SMA.
     *
     * The calculation from one TRIMA value to the next is done by doing 4
     * little adjustment (the following show a TRIMA 4-period):
     *
     * TRIMA at time 'd': ((1*a)+(2*b)+(2*c)+(1*d)) / 6
     * TRIMA at time 'e': ((1*b)+(2*c)+(2*d)+(1*e)) / 6
     *
     * To go from TRIMA 'd' to 'e', the following is done:
     *       1) 'a' and 'b' are substract from the numerator.
     *       2) 'd' is added to the numerator.
     *       3) 'e' is added to the numerator.
     *       4) Calculate TRIMA by doing numerator / 6
     *       5) Repeat sequence for next output
     *
     * These operations are the same steps done by TA-LIB:
     *       1) is done by numeratorSub
     *       2) is done by numeratorAdd.
     *       3) is obtain from the latest input
     *       4) Calculate and write TRIMA in the output
     *       5) Repeat for next output.
     *
     * Of course, numerotrAdd and numeratorSub needs to be
     * adjusted for each iteration.
     *
     * The update of numeratorSub needs values from the input at
     * the trailingIdx and middleIdx position.
     *
     * The update of numeratorAdd needs values from the input at
     * the middleIdx and todayIdx.
     */
    outIdx = 0;
    /* Even logic.
     *
     * Very similar to the odd logic, except:
     *  - calculation of the factor is different.
     *  - the coverage of the numeratorSub and numeratorAdd is
     *    slightly different.
     *  - Adjustment of numeratorAdd is different. See Step (2).
     */
    i = optInTimePeriod >> 1;
    factor = i * (i + 1);
    /* widen: i*(i+1) overflows int past period ~92682 */
    factor = 1.0 / factor;
    /* Initialize all the variable before
     * starting to iterate for each output.
     */
    trailingIdx = startIdx - lookbackTotal;
    middleIdx = trailingIdx + i - 1;
    todayIdx = middleIdx + i;
    numerator = 0.0;
    numeratorSub = 0.0;
    for (i = middleIdx; i >= trailingIdx; i -= 1) {
      tempReal = inReal[i];
      numeratorSub += tempReal;
      numerator += numeratorSub;
    }
    numeratorAdd = 0.0;
    middleIdx += 1;
    for (i = middleIdx; i <= todayIdx; i += 1) {
      tempReal = inReal[i];
      numeratorAdd += tempReal;
      numerator += numeratorAdd;
    }
    /* Write the first output */
    outIdx = 0;
    tempReal = inReal[trailingIdx++];
    outReal[((outIdx++ * outStride) | 0)] = numerator * factor;
    todayIdx += 1;
    /* Note: The value at the trailingIdx was saved
     *       in tempReal to account for the case where
     *       outReal and inReal are ptr on the same
     *       buffer.
     */
    /* Iterate for remaining output */
    while (todayIdx <= endIdx) {
      /* Step (1) */
      numerator -= numeratorSub;
      numeratorSub -= tempReal;
      tempReal = inReal[middleIdx++];
      numeratorSub += tempReal;
      /* Step (2) */
      numeratorAdd -= tempReal;
      numerator += numeratorAdd;
      tempReal = inReal[todayIdx++];
      numeratorAdd += tempReal;
      /* Step (3) */
      numerator += tempReal;
      /* Step (4) */
      tempReal = inReal[trailingIdx++];
      outReal[((outIdx++ * outStride) | 0)] = numerator * factor;
    }
    outNBElement.value = outIdx;
    outBegIdx.value = startIdx;
    const cap_middleIdx = todayIdx - middleIdx;
    if (cap_middleIdx < 1 || cap_middleIdx > historyLen) {
      return RetCode.INTERNAL_ERROR;
    }
    const allocN_middleIdx = cap_middleIdx > 0 ? cap_middleIdx : 1;
    const capRing_middleIdx_inReal = new Float64Array(allocN_middleIdx);
    capRing_middleIdx_inReal.set(inReal.subarray(historyLen - cap_middleIdx, historyLen), 0);
    const cap_trailingIdx = todayIdx - trailingIdx;
    if (cap_trailingIdx < 1 || cap_trailingIdx > historyLen) {
      return RetCode.INTERNAL_ERROR;
    }
    const allocN_trailingIdx = cap_trailingIdx > 0 ? cap_trailingIdx : 1;
    const capRing_trailingIdx_inReal = new Float64Array(allocN_trailingIdx);
    capRing_trailingIdx_inReal.set(inReal.subarray(historyLen - cap_trailingIdx, historyLen), 0);
    sp.optInTimePeriod = optInTimePeriod;
    sp.numerator = numerator;
    sp.numeratorSub = numeratorSub;
    sp.numeratorAdd = numeratorAdd;
    sp.factor = factor;
    sp.tempReal = tempReal;
    sp.ringPos_middleIdx = 0;
    sp.ringCap_middleIdx = cap_middleIdx;
    sp.ring_middleIdx_inReal = capRing_middleIdx_inReal;
    sp.ringPos_trailingIdx = 0;
    sp.ringCap_trailingIdx = cap_trailingIdx;
    sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
    sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
    return RetCode.SUCCESS;
  }
}

/** @internal */
export function trimaOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): TrimaStream {
  const sp = TrimaStream.of(core);
  const retCode = trimaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('TRIMA openAndFill', inReal.length, startIdx, trimaLookback(optInTimePeriod, core));
  }
  throw streamFailure('TRIMA openAndFill', retCode);
}

/** @internal */
export function trimaOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): TrimaStream {
  const sp = TrimaStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = trimaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('TRIMA open', inReal.length, startIdx, trimaLookback(optInTimePeriod, core));
  }
  throw streamFailure('TRIMA open', retCode);
}

/**
 * Opens a live TRIMA stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link trima} there.
 *
 * @param inReal - Warm-up history. Source price series.
 * @param optInTimePeriod - Number of bars in the averaging window (default 30;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `trimaLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { trimaOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = trimaOpen(data, 30);
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function trimaOpen(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): TrimaStream {
  optInTimePeriod = requireInt32('TRIMA open', 'optInTimePeriod', optInTimePeriod);
  requireArgument('TRIMA open', 'inReal', inReal);
  requireHistory('TRIMA open', inReal.length);
  return trimaOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/**
 * {@link trimaOpen} that also fills the outputs over the whole history in the same
 * pass, bit-identically to {@link trima}. The range written is the stream's
 * `outRange()`.
 *
 * @param inReal - Warm-up history. Source price series.
 * @param optInTimePeriod - Number of bars in the averaging window (default 30;
 *   range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
 * @param outReal - Triangular moving average. Must hold `historyLen - lookback`
 *   values and share no memory with an input or another output.
 * @param core - Unstable periods and candle settings to compute with;
 *   `Core.DEFAULT` when omitted.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `trimaLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM`
 *   for an integer parameter that is not an int32, a parameter outside its range or
 *   an output too short or starting where another output or a `Float64Array` input
 *   starts, or, on a history long enough to open, one sharing memory with another
 *   output or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { trimaOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const out = new Float64Array(data.length);
 * const stream = trimaOpenAndFill(data, 30, out);
 * const { begIdx, count } = stream.outRange();
 * // out[k] is the value at bar begIdx + k, for k < count
 * ```
 */
export function trimaOpenAndFill(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): TrimaStream {
  optInTimePeriod = requireInt32('TRIMA openAndFill', 'optInTimePeriod', optInTimePeriod);
  requireArgument('TRIMA openAndFill', 'inReal', inReal);
  requireHistory('TRIMA openAndFill', inReal.length);
  const guardOutLen = openFillCount('TRIMA openAndFill', inReal.length, trimaLookback(optInTimePeriod, core));
  requireOutput('TRIMA openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if ((outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset)) {
    throw streamFailure('TRIMA openAndFill', RetCode.BAD_PARAM);
  }
  if (guardOutLen > 0 && (overlaps(outReal, inRealF64))) {
    throw streamFailure('TRIMA openAndFill', RetCode.BAD_PARAM);
  }
  return trimaOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/** @internal */
export function trimaStepTape(sp: TrimaStream, tape: Float64Array, tapeBase: number, tapeMask: number, inReal: number): number {
  if (((sp.optInTimePeriod % 2) | 0) === 1) {
    /* Step (1) */
    sp.numerator -= sp.numeratorSub;
    sp.numeratorSub -= sp.tempReal;
    sp.tempReal = tape[(tapeBase - sp.ringCap_middleIdx) & tapeMask];
    sp.numeratorSub += sp.tempReal;
    /* Step (2) */
    sp.numerator += sp.numeratorAdd;
    sp.numeratorAdd -= sp.tempReal;
    sp.tempReal = inReal;
    sp.numeratorAdd += sp.tempReal;
    /* Step (3) */
    sp.numerator += sp.tempReal;
    /* Step (4) */
    sp.tempReal = tape[(tapeBase - sp.ringCap_trailingIdx) & tapeMask];
    sp.cur_outReal = sp.numerator * sp.factor;
  } else {
    /* Step (1) */
    sp.numerator -= sp.numeratorSub;
    sp.numeratorSub -= sp.tempReal;
    sp.tempReal = tape[(tapeBase - sp.ringCap_middleIdx) & tapeMask];
    sp.numeratorSub += sp.tempReal;
    /* Step (2) */
    sp.numeratorAdd -= sp.tempReal;
    sp.numerator += sp.numeratorAdd;
    sp.tempReal = inReal;
    sp.numeratorAdd += sp.tempReal;
    /* Step (3) */
    sp.numerator += sp.tempReal;
    /* Step (4) */
    sp.tempReal = tape[(tapeBase - sp.ringCap_trailingIdx) & tapeMask];
    sp.cur_outReal = sp.numerator * sp.factor;
  }
  sp.outRangeCount++;
  return sp.cur_outReal;
}

/** @internal */
export function trimaPeekTape(sp: TrimaStream, tape: Float64Array, tapeBase: number, tapeMask: number, inReal: number): number {
  let cur_outReal = 0;
  if (((sp.optInTimePeriod % 2) | 0) === 1) {
    let numerator = sp.numerator;
    let numeratorAdd = sp.numeratorAdd;
    let numeratorSub = sp.numeratorSub;
    let tempReal = sp.tempReal;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    pkSlot0 = tapeBase & tapeMask;
    pkVal0 = inReal;
    /* Step (1) */
    numerator -= numeratorSub;
    numeratorSub -= tempReal;
    tempReal = (((tapeBase - sp.ringCap_middleIdx) & tapeMask) !== pkSlot0) ? tape[(tapeBase - sp.ringCap_middleIdx) & tapeMask] : pkVal0;
    numeratorSub += tempReal;
    /* Step (2) */
    numerator += numeratorAdd;
    numeratorAdd -= tempReal;
    tempReal = inReal;
    numeratorAdd += tempReal;
    /* Step (3) */
    numerator += tempReal;
    /* Step (4) */
    tempReal = (((tapeBase - sp.ringCap_trailingIdx) & tapeMask) !== pkSlot0) ? tape[(tapeBase - sp.ringCap_trailingIdx) & tapeMask] : pkVal0;
    cur_outReal = numerator * sp.factor;
  } else {
    let numerator = sp.numerator;
    let numeratorAdd = sp.numeratorAdd;
    let numeratorSub = sp.numeratorSub;
    let tempReal = sp.tempReal;
    /* Step (1) */
    numerator -= numeratorSub;
    numeratorSub -= tempReal;
    tempReal = tape[(tapeBase - sp.ringCap_middleIdx) & tapeMask];
    numeratorSub += tempReal;
    /* Step (2) */
    numeratorAdd -= tempReal;
    numerator += numeratorAdd;
    tempReal = inReal;
    numeratorAdd += tempReal;
    /* Step (3) */
    numerator += tempReal;
    /* Step (4) */
    tempReal = tape[(tapeBase - sp.ringCap_trailingIdx) & tapeMask];
    cur_outReal = numerator * sp.factor;
  }
  return cur_outReal;
}

/** @internal */
export function trimaTapeDetach(sp: TrimaStream): number {
  let reach = 0;
  sp.ring_middleIdx_inReal = new Float64Array(0);
  if (sp.ringCap_middleIdx > reach) {
    reach = sp.ringCap_middleIdx;
  }
  sp.ring_trailingIdx_inReal = new Float64Array(0);
  if (sp.ringCap_trailingIdx > reach) {
    reach = sp.ringCap_trailingIdx;
  }
  return reach;
}
