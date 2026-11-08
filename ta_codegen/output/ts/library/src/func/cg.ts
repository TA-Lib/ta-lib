/*! TA-LIB Copyright (c) 1999-2026, Mario Fortier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither name of author nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


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
 *  092526 MF,CC  Initial version (#450).
 *  092626 MF,CC  O(1) per bar over exact sums (#453).
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function cgLookbackPositional(optInTimePeriod: number, _core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 10;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return -1;
  }
  return optInTimePeriod - 1;
}

/** @internal */
export function cgDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (cgLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function cgImpl(
  core: Core,
  startIdx: number,
  endIdx: number,
  inReal: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let today = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let trailingIdx = 0;
  let j = 0;
  let k = 0;
  let limbs = 0;
  let fits = 0;
  let maxAt = 0;
  let failAt = 0;
  let stickyBars = 0;
  let fit2Mid = 0;
  let fit2Lo = 0;
  let fit3Mid = 0;
  let num = 0;
  let den = 0;
  let value = 0;
  let periodDouble = 0;
  let flatValue = 0;
  let weightTotal = 0;
  let width2 = 0;
  let width3 = 0;
  let ylim2 = 0;
  let ylim3 = 0;
  let head2 = 0;
  let head3 = 0;
  let scale = 0;
  let width = 0;
  let invWidth = 0;
  let widthSq = 0;
  let invWidthSq = 0;
  let ylim = 0;
  let maxAbs = 0;
  let half = 0;
  let scale2Mid = 0;
  let scale2Lo = 0;
  let scale3Mid = 0;
  let scale3Lo = 0;
  let x = 0;
  let y = 0;
  let a = 0;
  let b = 0;
  let c = 0;
  let q = 0;
  let t = 0;
  let denA = 0;
  let denB = 0;
  let denC = 0;
  let numA = 0;
  let numB = 0;
  let numC = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 10;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = cgLookbackPositional(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    return RetCode.SUCCESS;
  }
  periodDouble = optInTimePeriod;
  flatValue = -(periodDouble + 1.0) * 0.5;
  /* Each window value is held as y = x*scale, an integer, split into 2 or 3
   * integer-valued limbs of `width`. The bounds keep every limb of both the
   * weighted and the plain sum below 2^53 while the total weight is at most
   * 2^tb, so every add and subtract below is exact, and every product is by a
   * power of two or of two integers whose result is below 2^53. That makes
   * each output depend on its window alone, whatever scale or limb count the
   * call reached it with. Keep it that way: a rounded operation anywhere in
   * the limb arithmetic makes the output depend on the start index.
   *
   * 2 limbs: width 2^(53-tb), |y| < 2^(104-2tb).
   * 3 limbs: width min(2^(53-tb), 2^26), |y| < 2^(51-tb) * width^2.
   *
   * Keep every product out of an addition in the same expression: each
   * product is exact, so fusing would change no value, only cost a call
   * where FMA is not inlined.
   */
  weightTotal = 1.0;
  while (weightTotal < periodDouble * (periodDouble + 1.0) * 0.5) {
    weightTotal *= 2.0;
  }
  width2 = 9007199254740992.0 / weightTotal;
  ylim2 = 0.25 * width2 * width2;
  width3 = width2;
  if (width3 > 67108864.0) {
    width3 = 67108864.0;
  }
  ylim3 = 0.25 * width2 * width3 * width3;
  /* A scale below the largest that fits leaves room for the window's
   * magnitude to grow before a rebuild, split evenly with the room left for
   * finer values.
   */
  head2 = 1.0;
  while (head2 * head2 * 9007199254740992.0 < ylim2) {
    head2 *= 2.0;
  }
  head3 = 1.0;
  while (head3 * head3 * 9007199254740992.0 < ylim3) {
    head3 *= 2.0;
  }
  limbs = 0;
  stickyBars = 0;
  scale = 1.0;
  width = 1.0;
  invWidth = 1.0;
  widthSq = 1.0;
  invWidthSq = 1.0;
  ylim = 0.0;
  half = 1.0;
  denA = 0.0;
  denB = 0.0;
  denC = 0.0;
  numA = 0.0;
  numB = 0.0;
  numC = 0.0;
  num = 0.0;
  den = 0.0;
  outIdx = 0;
  today = startIdx;
  trailingIdx = startIdx - lookbackTotal;
  while (today <= endIdx) {
    /* Between bars the sums hold the window less its oldest value. */
    fits = 0;
    x = inReal[today];
    y = x * scale;
    if (limbs === 2) {
      if (Math.abs(y) < ylim && (y !== 0.0 || x === 0.0)) {
        t = y * invWidth;
        a = t + 6755399441055744.0 - 6755399441055744.0;
        t = a * width;
        c = y - t;
        if (c + 6755399441055744.0 - 6755399441055744.0 === c) {
          denA += a;
          denC += c;
          numA += denA;
          numC += denC;
          fits = 1;
        }
      }
    } else if (limbs === 3) {
      if (Math.abs(y) < ylim && (y !== 0.0 || x === 0.0)) {
        t = y * invWidthSq;
        a = t + 6755399441055744.0 - 6755399441055744.0;
        t = a * widthSq;
        q = y - t;
        t = q * invWidth;
        b = t + 6755399441055744.0 - 6755399441055744.0;
        t = b * width;
        c = q - t;
        if (c + 6755399441055744.0 - 6755399441055744.0 === c) {
          denA += a;
          denB += b;
          denC += c;
          numA += denA;
          numB += denB;
          numC += denC;
          fits = 1;
        }
      }
    }
    if (fits === 0 && limbs === 0 && stickyBars > 0) {
      /* Both witnesses of the last failed fit are still in the window, so
       * it cannot fit either: its largest value can only be larger, which
       * only coarsens the scale the failing value already missed.
       */
      stickyBars -= 1;
      num = 0.0;
      den = 0.0;
      for (j = today - lookbackTotal; j <= today; j += 1) {
        den += inReal[j];
        num += den;
      }
    } else if (fits === 0) {
      /* The incoming value does not fit the scale: pick one from the
       * window alone.
       */
      maxAbs = 0.0;
      maxAt = 0;
      k = 0;
      for (j = today - lookbackTotal; j <= today; j += 1) {
        x = Math.abs(inReal[j]);
        if (x >= maxAbs) {
          maxAbs = x;
          maxAt = k;
        }
        k += 1;
      }
      /* The largest power of two not above the window's largest
       * magnitude. The search may start from any finite power of two, so
       * it starts from the last window's; keep it uncapped, or a start far
       * from the answer stops short and the fit then depends on the
       * previous window. A window with an infinite magnitude fails the fit
       * at any scale, so it keeps the last one.
       */
      if (maxAbs > 0.0 && maxAbs <= 1.7976931348623157e308) {
        while (half * 65536.0 <= maxAbs) {
          half *= 65536.0;
        }
        while (half * 2.0 <= maxAbs) {
          half *= 2.0;
        }
        while (half > maxAbs * 65536.0) {
          half *= 1.52587890625e-5;
        }
        while (half > maxAbs) {
          half *= 0.5;
        }
      }
      /* The largest scale each limb count allows, and the same with
       * headroom, at most 2^1022 so that the scale itself is finite. The
       * largest 3-limb scale is the most permissive there is: the window
       * fits some scale only if every value is an integer at that one. A
       * scale below 1 can round a small value to 0, which is not a fit.
       */
      scale2Lo = ylim2 * 0.5 / half;
      if (scale2Lo > 4.49423283715579e307) {
        scale2Lo = 4.49423283715579e307;
      }
      scale2Mid = scale2Lo / head2;
      scale3Lo = ylim3 * 0.5 / half;
      if (scale3Lo > 4.49423283715579e307) {
        scale3Lo = 4.49423283715579e307;
      }
      scale3Mid = scale3Lo / head3;
      /* The same pass sums the window as it stands, oldest first, for when
       * nothing fits: non-finite values, bits below 2^-1022, or too wide a
       * span of magnitudes.
       */
      fit2Mid = 1;
      fit2Lo = 1;
      fit3Mid = 1;
      failAt = -1;
      num = 0.0;
      den = 0.0;
      k = 0;
      for (j = today - lookbackTotal; j <= today; j += 1) {
        x = inReal[j];
        den += x;
        num += den;
        y = Math.abs(x * scale2Mid);
        if (!(y < ylim2) || y < 4503599627370496.0 && y + 4503599627370496.0 - 4503599627370496.0 !== y || y === 0.0 && x !== 0.0) {
          fit2Mid = 0;
        }
        y = Math.abs(x * scale2Lo);
        if (!(y < ylim2) || y < 4503599627370496.0 && y + 4503599627370496.0 - 4503599627370496.0 !== y || y === 0.0 && x !== 0.0) {
          fit2Lo = 0;
        }
        y = Math.abs(x * scale3Mid);
        if (!(y < ylim3) || y < 4503599627370496.0 && y + 4503599627370496.0 - 4503599627370496.0 !== y || y === 0.0 && x !== 0.0) {
          fit3Mid = 0;
        }
        y = Math.abs(x * scale3Lo);
        if (!(y < ylim3) || y < 4503599627370496.0 && y + 4503599627370496.0 - 4503599627370496.0 !== y || y === 0.0 && x !== 0.0) {
          failAt = k;
        }
        k += 1;
      }
      limbs = 0;
      if (failAt >= 0) {
        if (failAt < maxAt) {
          stickyBars = failAt;
        } else {
          stickyBars = maxAt;
        }
      } else {
        if (fit2Mid === 1 || fit2Lo === 1) {
          limbs = 2;
          width = width2;
          ylim = ylim2;
          if (fit2Mid === 1) {
            scale = scale2Mid;
          } else {
            scale = scale2Lo;
          }
        } else {
          limbs = 3;
          width = width3;
          ylim = ylim3;
          if (fit3Mid === 1) {
            scale = scale3Mid;
          } else {
            scale = scale3Lo;
          }
        }
        invWidth = 1.0 / width;
        widthSq = width * width;
        invWidthSq = invWidth * invWidth;
        denA = 0.0;
        denB = 0.0;
        denC = 0.0;
        numA = 0.0;
        numB = 0.0;
        numC = 0.0;
        if (limbs === 2) {
          for (j = today - lookbackTotal; j <= today; j += 1) {
            y = inReal[j] * scale;
            t = y * invWidth;
            a = t + 6755399441055744.0 - 6755399441055744.0;
            t = a * width;
            c = y - t;
            denA += a;
            denC += c;
            numA += denA;
            numC += denC;
          }
        } else {
          for (j = today - lookbackTotal; j <= today; j += 1) {
            y = inReal[j] * scale;
            t = y * invWidthSq;
            a = t + 6755399441055744.0 - 6755399441055744.0;
            t = a * widthSq;
            q = y - t;
            t = q * invWidth;
            b = t + 6755399441055744.0 - 6755399441055744.0;
            t = b * width;
            c = q - t;
            denA += a;
            denB += b;
            denC += c;
            numA += denA;
            numB += denB;
            numC += denC;
          }
        }
      }
    }
    /* One rounding each: the limbs are carried into range first, so the
     * last add sees two exact values.
     */
    if (limbs === 2) {
      t = denA * width;
      den = t + denC;
      t = numA * width;
      num = t + numC;
    } else if (limbs === 3) {
      t = denC * invWidth;
      q = t + 6755399441055744.0 - 6755399441055744.0;
      b = denB + q;
      t = q * width;
      c = denC - t;
      t = b * invWidth;
      q = t + 6755399441055744.0 - 6755399441055744.0;
      t = q * width;
      b = b - t;
      t = b * width;
      c = t + c;
      t = denA + q;
      t = t * widthSq;
      den = t + c;
      t = numC * invWidth;
      q = t + 6755399441055744.0 - 6755399441055744.0;
      b = numB + q;
      t = q * width;
      c = numC - t;
      t = b * invWidth;
      q = t + 6755399441055744.0 - 6755399441055744.0;
      t = q * width;
      b = b - t;
      t = b * width;
      c = t + c;
      t = numA + q;
      t = t * widthSq;
      num = t + c;
    }
    /* The denominator is a signed sum, so only an exact zero is degenerate.
     * It is answered with the flat-window value, which keeps every output a
     * function of its own window; an epsilon band would carry the quote unit
     * (#253).
     */
    if (den !== 0.0) {
      value = -num / den;
    } else {
      value = flatValue;
    }
    if (limbs === 2) {
      y = inReal[trailingIdx] * scale;
      t = y * invWidth;
      a = t + 6755399441055744.0 - 6755399441055744.0;
      t = a * width;
      c = y - t;
      denA -= a;
      denC -= c;
      numA -= periodDouble * a;
      numC -= periodDouble * c;
    } else if (limbs === 3) {
      y = inReal[trailingIdx] * scale;
      t = y * invWidthSq;
      a = t + 6755399441055744.0 - 6755399441055744.0;
      t = a * widthSq;
      q = y - t;
      t = q * invWidth;
      b = t + 6755399441055744.0 - 6755399441055744.0;
      t = b * width;
      c = q - t;
      denA -= a;
      denB -= b;
      denC -= c;
      numA -= periodDouble * a;
      numB -= periodDouble * b;
      numC -= periodDouble * c;
    }
    /* After the trailing value is read: outReal may be inReal. */
    outReal[outIdx] = value;
    trailingIdx += 1;
    outIdx += 1;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 2, 100000, 'in 2..100000']] as const;

/** @internal */
export function cgPositional(
  startIdx: number,
  endIdx: number,
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('CG', 'startIdx', startIdx);
  endIdx = requireInt32('CG', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('CG', 'timePeriod', optInTimePeriod);
  requireIndexRange('CG', startIdx, endIdx);
  const guardLookback = cgLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('CG', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('CG', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('CG', 'real', inReal, guardInLen);
  requireOutput('CG', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (overlapsPartly(outReal, inRealF64)) {
    throw sharedMemory('CG', 'outReal', 'real');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = cgImpl(core, startIdx, endIdx, inRealF64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('CG', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link cg}, {@link cgLookback}, {@link cgDisplayShift},
 * {@link cgOpen} and {@link cgOpenAndFill}. Every key is optional: an omitted or
 * `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface CgOptions {
  /**
   * Number of bars in the window (default 10; range 2 to 100000;
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

/** What {@link cg} returns, and {@link cgOpenAndFill} with its stream. */
export interface CgResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Negated center of gravity of the window. `values[k]` is the value at input bar
   * `begIdx + k`.
   */
  values: Float64Array<ArrayBuffer>;
}

/**
 * Number of leading input bars {@link cg} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CgOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cgLookback(options?: CgOptions): number {
  const o = optionsOf('CG lookback', options);
  return cgLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('CG lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link cg}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CgOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function cgDisplayShift(outputIdx: number, options?: CgOptions): number {
  const o = optionsOf('CG displayShift', options);
  return cgDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('CG displayShift', o.core));
}

/**
 * John Ehlers' Center of Gravity oscillator: the balance point of the last
 * `timePeriod` values, each weighted by its value and placed at its position
 * counting back from the current bar, which is position 1, negated so that it
 * rises with price. It is smooth and has essentially no lag. Ehlers reads turning
 * points from it and trades its crossings with a copy of itself delayed by one
 * bar. For a positive series it stays between -timePeriod and -1, and a flat
 * window sits at the midpoint, -(timePeriod+1)/2.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cg](https://ta-lib.org/functions/cg).
 *
 * **Notes**
 * - **The input is assumed positive.** The sum of the window is the divisor. Near
 *   zero the divisor is small and the output very large; no error is returned. A
 *   series that reaches or crosses zero, such as an oscillator or a spread, has no
 *   meaningful result here.
 * - Ehlers' default input is the median price (H+L)/2: pass the output of MEDPRICE
 *   to reproduce it.
 * - His 2004 book presents the same oscillator shifted up by (timePeriod+1)/2, so
 *   that a flat window reads 0: add (timePeriod+1)/2 to the output to obtain that
 *   form.
 * - Where the window sums to exactly zero the author's listing keeps its previous
 *   value; TA-Lib returns -(timePeriod+1)/2, so every value depends on its own
 *   window alone.
 * - Both sums are exact before the divide, so a window summing to exactly zero is
 *   always recognised and the value does not depend on where the call started. The
 *   exception is a window holding a non-finite value, a value with bits below
 *   2^-1022 (every subnormal, and only values under about 2e-292), or values that
 *   together span more binary digits than the sums can hold exactly (97 at the
 *   default period, 84 at 1000, 58 at 100000): it is summed in floating point,
 *   oldest value first.
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link cgLookback} is a success with empty outputs.
 *
 * @param real - The series to measure.
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link CgOptions}.
 * @returns `begIdx` and the values; see {@link CgResult}. Each output is a view of
 *   its buffer, the caller's or a new one.
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
 * import { cg } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { begIdx, values } = cg(data, { timePeriod: 10 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`wma`](https://ta-lib.org/functions/wma)
 * @see [`medprice`](https://ta-lib.org/functions/medprice)
 * @see [`cti`](https://ta-lib.org/functions/cti)
 */
export function cg(real: ArrayLike<number>, options?: CgOptions): CgResult {
  const o = optionsOf('CG', options);
  if (o.endIdx === undefined) {
    requireSeries('CG', 'real', real);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('CG', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, cgLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = cgPositional(startIdx, endIdx, real, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live CG stream: one value per closed bar, bit-identical to {@link cg} over the
 * same series. Open it with {@link cgOpen} or {@link cgOpenAndFill}; there is
 * nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/cg](https://ta-lib.org/functions/cg).
 */
export class CgStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  lookbackTotal: number = 0;
  /** @internal */
  limbs: number = 0;
  /** @internal */
  stickyBars: number = 0;
  /** @internal */
  num: number = 0;
  /** @internal */
  den: number = 0;
  /** @internal */
  periodDouble: number = 0;
  /** @internal */
  flatValue: number = 0;
  /** @internal */
  width2: number = 0;
  /** @internal */
  width3: number = 0;
  /** @internal */
  ylim2: number = 0;
  /** @internal */
  ylim3: number = 0;
  /** @internal */
  head2: number = 0;
  /** @internal */
  head3: number = 0;
  /** @internal */
  scale: number = 0;
  /** @internal */
  width: number = 0;
  /** @internal */
  invWidth: number = 0;
  /** @internal */
  widthSq: number = 0;
  /** @internal */
  invWidthSq: number = 0;
  /** @internal */
  ylim: number = 0;
  /** @internal */
  half: number = 0;
  /** @internal */
  denA: number = 0;
  /** @internal */
  denB: number = 0;
  /** @internal */
  denC: number = 0;
  /** @internal */
  numA: number = 0;
  /** @internal */
  numB: number = 0;
  /** @internal */
  numC: number = 0;
  /** @internal */
  ringPos_trailingIdx: number = 0;
  /** @internal */
  ringCap_trailingIdx: number = 0;
  /** @internal */
  ring_trailingIdx_inReal: Float64Array = new Float64Array(1);
  /** @internal */
  winPos_j: number = 0;
  /** @internal */
  winCap_j: number = 1;
  /** @internal */
  win_j_inReal: Float64Array = new Float64Array(1);
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
  static of(core: Core): CgStream {
    return new CgStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link cg} reports them over the same bars. Each accepted `update` and each
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
      throw streamFailure('CG advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param real - The bar's `real` value: The series to measure.
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(real: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw streamFailure('CG update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('CG update', 'real');
    }
    cgStepImpl(this, real);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param real - The bar's `real` value: The series to measure.
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(real: number): number {
    if (!Number.isFinite(real)) {
      throw nonFiniteBar('CG peek', 'real');
    }
    const sp = this;
    const inReal = real;
    let j = 0;
    let k = 0;
    let fits = 0;
    let maxAt = 0;
    let failAt = 0;
    let fit2Mid = 0;
    let fit2Lo = 0;
    let fit3Mid = 0;
    let value = 0;
    let maxAbs = 0;
    let scale2Mid = 0;
    let scale2Lo = 0;
    let scale3Mid = 0;
    let scale3Lo = 0;
    let x = 0;
    let y = 0;
    let a = 0;
    let b = 0;
    let c = 0;
    let q = 0;
    let t = 0;
    let cur_outReal = 0;
    let den = sp.den;
    let denA = sp.denA;
    let denB = sp.denB;
    let denC = sp.denC;
    let half = sp.half;
    let invWidth = sp.invWidth;
    let invWidthSq = sp.invWidthSq;
    let limbs = sp.limbs;
    let num = sp.num;
    let numA = sp.numA;
    let numB = sp.numB;
    let numC = sp.numC;
    let scale = sp.scale;
    let stickyBars = sp.stickyBars;
    let width = sp.width;
    let widthSq = sp.widthSq;
    let ylim = sp.ylim;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    pkSlot0 = sp.winPos_j;
    pkVal0 = inReal;
    /* Between bars the sums hold the window less its oldest value. */
    fits = 0;
    x = inReal;
    y = x * scale;
    if (limbs === 2) {
      if (Math.abs(y) < ylim && (y !== 0.0 || x === 0.0)) {
        t = y * invWidth;
        a = t + 6755399441055744.0 - 6755399441055744.0;
        t = a * width;
        c = y - t;
        if (c + 6755399441055744.0 - 6755399441055744.0 === c) {
          denA += a;
          denC += c;
          numA += denA;
          numC += denC;
          fits = 1;
        }
      }
    } else if (limbs === 3) {
      if (Math.abs(y) < ylim && (y !== 0.0 || x === 0.0)) {
        t = y * invWidthSq;
        a = t + 6755399441055744.0 - 6755399441055744.0;
        t = a * widthSq;
        q = y - t;
        t = q * invWidth;
        b = t + 6755399441055744.0 - 6755399441055744.0;
        t = b * width;
        c = q - t;
        if (c + 6755399441055744.0 - 6755399441055744.0 === c) {
          denA += a;
          denB += b;
          denC += c;
          numA += denA;
          numB += denB;
          numC += denC;
          fits = 1;
        }
      }
    }
    if (fits === 0 && limbs === 0 && stickyBars > 0) {
      /* Both witnesses of the last failed fit are still in the window, so
       * it cannot fit either: its largest value can only be larger, which
       * only coarsens the scale the failing value already missed.
       */
      stickyBars -= 1;
      num = 0.0;
      den = 0.0;
      for (j = sp.lookbackTotal; j >= 0; j -= 1) {
        den += (((sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j) !== pkSlot0) ? sp.win_j_inReal[(sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j] : pkVal0;
        num += den;
      }
    } else if (fits === 0) {
      /* The incoming value does not fit the scale: pick one from the
       * window alone.
       */
      maxAbs = 0.0;
      maxAt = 0;
      k = 0;
      for (j = sp.lookbackTotal; j >= 0; j -= 1) {
        x = Math.abs((((sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j) !== pkSlot0) ? sp.win_j_inReal[(sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j] : pkVal0);
        if (x >= maxAbs) {
          maxAbs = x;
          maxAt = k;
        }
        k += 1;
      }
      /* The largest power of two not above the window's largest
       * magnitude. The search may start from any finite power of two, so
       * it starts from the last window's; keep it uncapped, or a start far
       * from the answer stops short and the fit then depends on the
       * previous window. A window with an infinite magnitude fails the fit
       * at any scale, so it keeps the last one.
       */
      if (maxAbs > 0.0 && maxAbs <= 1.7976931348623157e308) {
        while (half * 65536.0 <= maxAbs) {
          half *= 65536.0;
        }
        while (half * 2.0 <= maxAbs) {
          half *= 2.0;
        }
        while (half > maxAbs * 65536.0) {
          half *= 1.52587890625e-5;
        }
        while (half > maxAbs) {
          half *= 0.5;
        }
      }
      /* The largest scale each limb count allows, and the same with
       * headroom, at most 2^1022 so that the scale itself is finite. The
       * largest 3-limb scale is the most permissive there is: the window
       * fits some scale only if every value is an integer at that one. A
       * scale below 1 can round a small value to 0, which is not a fit.
       */
      scale2Lo = sp.ylim2 * 0.5 / half;
      if (scale2Lo > 4.49423283715579e307) {
        scale2Lo = 4.49423283715579e307;
      }
      scale2Mid = scale2Lo / sp.head2;
      scale3Lo = sp.ylim3 * 0.5 / half;
      if (scale3Lo > 4.49423283715579e307) {
        scale3Lo = 4.49423283715579e307;
      }
      scale3Mid = scale3Lo / sp.head3;
      /* The same pass sums the window as it stands, oldest first, for when
       * nothing fits: non-finite values, bits below 2^-1022, or too wide a
       * span of magnitudes.
       */
      fit2Mid = 1;
      fit2Lo = 1;
      fit3Mid = 1;
      failAt = -1;
      num = 0.0;
      den = 0.0;
      k = 0;
      for (j = sp.lookbackTotal; j >= 0; j -= 1) {
        x = (((sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j) !== pkSlot0) ? sp.win_j_inReal[(sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j] : pkVal0;
        den += x;
        num += den;
        y = Math.abs(x * scale2Mid);
        if (!(y < sp.ylim2) || y < 4503599627370496.0 && y + 4503599627370496.0 - 4503599627370496.0 !== y || y === 0.0 && x !== 0.0) {
          fit2Mid = 0;
        }
        y = Math.abs(x * scale2Lo);
        if (!(y < sp.ylim2) || y < 4503599627370496.0 && y + 4503599627370496.0 - 4503599627370496.0 !== y || y === 0.0 && x !== 0.0) {
          fit2Lo = 0;
        }
        y = Math.abs(x * scale3Mid);
        if (!(y < sp.ylim3) || y < 4503599627370496.0 && y + 4503599627370496.0 - 4503599627370496.0 !== y || y === 0.0 && x !== 0.0) {
          fit3Mid = 0;
        }
        y = Math.abs(x * scale3Lo);
        if (!(y < sp.ylim3) || y < 4503599627370496.0 && y + 4503599627370496.0 - 4503599627370496.0 !== y || y === 0.0 && x !== 0.0) {
          failAt = k;
        }
        k += 1;
      }
      limbs = 0;
      if (failAt >= 0) {
        if (failAt < maxAt) {
          stickyBars = failAt;
        } else {
          stickyBars = maxAt;
        }
      } else {
        if (fit2Mid === 1 || fit2Lo === 1) {
          limbs = 2;
          width = sp.width2;
          ylim = sp.ylim2;
          if (fit2Mid === 1) {
            scale = scale2Mid;
          } else {
            scale = scale2Lo;
          }
        } else {
          limbs = 3;
          width = sp.width3;
          ylim = sp.ylim3;
          if (fit3Mid === 1) {
            scale = scale3Mid;
          } else {
            scale = scale3Lo;
          }
        }
        invWidth = 1.0 / width;
        widthSq = width * width;
        invWidthSq = invWidth * invWidth;
        denA = 0.0;
        denB = 0.0;
        denC = 0.0;
        numA = 0.0;
        numB = 0.0;
        numC = 0.0;
        if (limbs === 2) {
          for (j = sp.lookbackTotal; j >= 0; j -= 1) {
            y = ((((sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j) !== pkSlot0) ? sp.win_j_inReal[(sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j] : pkVal0) * scale;
            t = y * invWidth;
            a = t + 6755399441055744.0 - 6755399441055744.0;
            t = a * width;
            c = y - t;
            denA += a;
            denC += c;
            numA += denA;
            numC += denC;
          }
        } else {
          for (j = sp.lookbackTotal; j >= 0; j -= 1) {
            y = ((((sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j) !== pkSlot0) ? sp.win_j_inReal[(sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j] : pkVal0) * scale;
            t = y * invWidthSq;
            a = t + 6755399441055744.0 - 6755399441055744.0;
            t = a * widthSq;
            q = y - t;
            t = q * invWidth;
            b = t + 6755399441055744.0 - 6755399441055744.0;
            t = b * width;
            c = q - t;
            denA += a;
            denB += b;
            denC += c;
            numA += denA;
            numB += denB;
            numC += denC;
          }
        }
      }
    }
    /* One rounding each: the limbs are carried into range first, so the
     * last add sees two exact values.
     */
    if (limbs === 2) {
      t = denA * width;
      den = t + denC;
      t = numA * width;
      num = t + numC;
    } else if (limbs === 3) {
      t = denC * invWidth;
      q = t + 6755399441055744.0 - 6755399441055744.0;
      b = denB + q;
      t = q * width;
      c = denC - t;
      t = b * invWidth;
      q = t + 6755399441055744.0 - 6755399441055744.0;
      t = q * width;
      b = b - t;
      t = b * width;
      c = t + c;
      t = denA + q;
      t = t * widthSq;
      den = t + c;
      t = numC * invWidth;
      q = t + 6755399441055744.0 - 6755399441055744.0;
      b = numB + q;
      t = q * width;
      c = numC - t;
      t = b * invWidth;
      q = t + 6755399441055744.0 - 6755399441055744.0;
      t = q * width;
      b = b - t;
      t = b * width;
      c = t + c;
      t = numA + q;
      t = t * widthSq;
      num = t + c;
    }
    /* The denominator is a signed sum, so only an exact zero is degenerate.
     * It is answered with the flat-window value, which keeps every output a
     * function of its own window; an epsilon band would carry the quote unit
     * (#253).
     */
    if (den !== 0.0) {
      value = -num / den;
    } else {
      value = sp.flatValue;
    }
    if (limbs === 2) {
      y = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] * scale;
      t = y * invWidth;
      a = t + 6755399441055744.0 - 6755399441055744.0;
      t = a * width;
      c = y - t;
      denA -= a;
      denC -= c;
      numA -= sp.periodDouble * a;
      numC -= sp.periodDouble * c;
    } else if (limbs === 3) {
      y = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] * scale;
      t = y * invWidthSq;
      a = t + 6755399441055744.0 - 6755399441055744.0;
      t = a * widthSq;
      q = y - t;
      t = q * invWidth;
      b = t + 6755399441055744.0 - 6755399441055744.0;
      t = b * width;
      c = q - t;
      denA -= a;
      denB -= b;
      denC -= c;
      numA -= sp.periodDouble * a;
      numB -= sp.periodDouble * b;
      numC -= sp.periodDouble * c;
    }
    /* After the trailing value is read: outReal may be inReal. */
    cur_outReal = value;
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
  clone(): CgStream {
    const c = new CgStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.lookbackTotal = this.lookbackTotal;
    c.limbs = this.limbs;
    c.stickyBars = this.stickyBars;
    c.num = this.num;
    c.den = this.den;
    c.periodDouble = this.periodDouble;
    c.flatValue = this.flatValue;
    c.width2 = this.width2;
    c.width3 = this.width3;
    c.ylim2 = this.ylim2;
    c.ylim3 = this.ylim3;
    c.head2 = this.head2;
    c.head3 = this.head3;
    c.scale = this.scale;
    c.width = this.width;
    c.invWidth = this.invWidth;
    c.widthSq = this.widthSq;
    c.invWidthSq = this.invWidthSq;
    c.ylim = this.ylim;
    c.half = this.half;
    c.denA = this.denA;
    c.denB = this.denB;
    c.denC = this.denC;
    c.numA = this.numA;
    c.numB = this.numB;
    c.numC = this.numC;
    c.ringPos_trailingIdx = this.ringPos_trailingIdx;
    c.ringCap_trailingIdx = this.ringCap_trailingIdx;
    c.ring_trailingIdx_inReal = this.ring_trailingIdx_inReal.slice();
    c.winPos_j = this.winPos_j;
    c.winCap_j = this.winCap_j;
    c.win_j_inReal = this.win_j_inReal.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function cgStepImpl(sp: CgStream, inReal: number): void {
  let j = 0;
  let k = 0;
  let fits = 0;
  let maxAt = 0;
  let failAt = 0;
  let fit2Mid = 0;
  let fit2Lo = 0;
  let fit3Mid = 0;
  let value = 0;
  let maxAbs = 0;
  let scale2Mid = 0;
  let scale2Lo = 0;
  let scale3Mid = 0;
  let scale3Lo = 0;
  let x = 0;
  let y = 0;
  let a = 0;
  let b = 0;
  let c = 0;
  let q = 0;
  let t = 0;
  let ringCapL_trailingIdx = 0;
  sp.win_j_inReal[sp.winPos_j] = inReal;
  /* Between bars the sums hold the window less its oldest value. */
  fits = 0;
  x = inReal;
  y = x * sp.scale;
  if (sp.limbs === 2) {
    if (Math.abs(y) < sp.ylim && (y !== 0.0 || x === 0.0)) {
      t = y * sp.invWidth;
      a = t + 6755399441055744.0 - 6755399441055744.0;
      t = a * sp.width;
      c = y - t;
      if (c + 6755399441055744.0 - 6755399441055744.0 === c) {
        sp.denA += a;
        sp.denC += c;
        sp.numA += sp.denA;
        sp.numC += sp.denC;
        fits = 1;
      }
    }
  } else if (sp.limbs === 3) {
    if (Math.abs(y) < sp.ylim && (y !== 0.0 || x === 0.0)) {
      t = y * sp.invWidthSq;
      a = t + 6755399441055744.0 - 6755399441055744.0;
      t = a * sp.widthSq;
      q = y - t;
      t = q * sp.invWidth;
      b = t + 6755399441055744.0 - 6755399441055744.0;
      t = b * sp.width;
      c = q - t;
      if (c + 6755399441055744.0 - 6755399441055744.0 === c) {
        sp.denA += a;
        sp.denB += b;
        sp.denC += c;
        sp.numA += sp.denA;
        sp.numB += sp.denB;
        sp.numC += sp.denC;
        fits = 1;
      }
    }
  }
  if (fits === 0 && sp.limbs === 0 && sp.stickyBars > 0) {
    /* Both witnesses of the last failed fit are still in the window, so
     * it cannot fit either: its largest value can only be larger, which
     * only coarsens the scale the failing value already missed.
     */
    sp.stickyBars -= 1;
    sp.num = 0.0;
    sp.den = 0.0;
    for (j = sp.lookbackTotal; j >= 0; j -= 1) {
      sp.den += sp.win_j_inReal[(sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j];
      sp.num += sp.den;
    }
  } else if (fits === 0) {
    /* The incoming value does not fit the scale: pick one from the
     * window alone.
     */
    maxAbs = 0.0;
    maxAt = 0;
    k = 0;
    for (j = sp.lookbackTotal; j >= 0; j -= 1) {
      x = Math.abs(sp.win_j_inReal[(sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j]);
      if (x >= maxAbs) {
        maxAbs = x;
        maxAt = k;
      }
      k += 1;
    }
    /* The largest power of two not above the window's largest
     * magnitude. The search may start from any finite power of two, so
     * it starts from the last window's; keep it uncapped, or a start far
     * from the answer stops short and the fit then depends on the
     * previous window. A window with an infinite magnitude fails the fit
     * at any scale, so it keeps the last one.
     */
    if (maxAbs > 0.0 && maxAbs <= 1.7976931348623157e308) {
      while (sp.half * 65536.0 <= maxAbs) {
        sp.half *= 65536.0;
      }
      while (sp.half * 2.0 <= maxAbs) {
        sp.half *= 2.0;
      }
      while (sp.half > maxAbs * 65536.0) {
        sp.half *= 1.52587890625e-5;
      }
      while (sp.half > maxAbs) {
        sp.half *= 0.5;
      }
    }
    /* The largest scale each limb count allows, and the same with
     * headroom, at most 2^1022 so that the scale itself is finite. The
     * largest 3-limb scale is the most permissive there is: the window
     * fits some scale only if every value is an integer at that one. A
     * scale below 1 can round a small value to 0, which is not a fit.
     */
    scale2Lo = sp.ylim2 * 0.5 / sp.half;
    if (scale2Lo > 4.49423283715579e307) {
      scale2Lo = 4.49423283715579e307;
    }
    scale2Mid = scale2Lo / sp.head2;
    scale3Lo = sp.ylim3 * 0.5 / sp.half;
    if (scale3Lo > 4.49423283715579e307) {
      scale3Lo = 4.49423283715579e307;
    }
    scale3Mid = scale3Lo / sp.head3;
    /* The same pass sums the window as it stands, oldest first, for when
     * nothing fits: non-finite values, bits below 2^-1022, or too wide a
     * span of magnitudes.
     */
    fit2Mid = 1;
    fit2Lo = 1;
    fit3Mid = 1;
    failAt = -1;
    sp.num = 0.0;
    sp.den = 0.0;
    k = 0;
    for (j = sp.lookbackTotal; j >= 0; j -= 1) {
      x = sp.win_j_inReal[(sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j];
      sp.den += x;
      sp.num += sp.den;
      y = Math.abs(x * scale2Mid);
      if (!(y < sp.ylim2) || y < 4503599627370496.0 && y + 4503599627370496.0 - 4503599627370496.0 !== y || y === 0.0 && x !== 0.0) {
        fit2Mid = 0;
      }
      y = Math.abs(x * scale2Lo);
      if (!(y < sp.ylim2) || y < 4503599627370496.0 && y + 4503599627370496.0 - 4503599627370496.0 !== y || y === 0.0 && x !== 0.0) {
        fit2Lo = 0;
      }
      y = Math.abs(x * scale3Mid);
      if (!(y < sp.ylim3) || y < 4503599627370496.0 && y + 4503599627370496.0 - 4503599627370496.0 !== y || y === 0.0 && x !== 0.0) {
        fit3Mid = 0;
      }
      y = Math.abs(x * scale3Lo);
      if (!(y < sp.ylim3) || y < 4503599627370496.0 && y + 4503599627370496.0 - 4503599627370496.0 !== y || y === 0.0 && x !== 0.0) {
        failAt = k;
      }
      k += 1;
    }
    sp.limbs = 0;
    if (failAt >= 0) {
      if (failAt < maxAt) {
        sp.stickyBars = failAt;
      } else {
        sp.stickyBars = maxAt;
      }
    } else {
      if (fit2Mid === 1 || fit2Lo === 1) {
        sp.limbs = 2;
        sp.width = sp.width2;
        sp.ylim = sp.ylim2;
        if (fit2Mid === 1) {
          sp.scale = scale2Mid;
        } else {
          sp.scale = scale2Lo;
        }
      } else {
        sp.limbs = 3;
        sp.width = sp.width3;
        sp.ylim = sp.ylim3;
        if (fit3Mid === 1) {
          sp.scale = scale3Mid;
        } else {
          sp.scale = scale3Lo;
        }
      }
      sp.invWidth = 1.0 / sp.width;
      sp.widthSq = sp.width * sp.width;
      sp.invWidthSq = sp.invWidth * sp.invWidth;
      sp.denA = 0.0;
      sp.denB = 0.0;
      sp.denC = 0.0;
      sp.numA = 0.0;
      sp.numB = 0.0;
      sp.numC = 0.0;
      if (sp.limbs === 2) {
        for (j = sp.lookbackTotal; j >= 0; j -= 1) {
          y = sp.win_j_inReal[(sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j] * sp.scale;
          t = y * sp.invWidth;
          a = t + 6755399441055744.0 - 6755399441055744.0;
          t = a * sp.width;
          c = y - t;
          sp.denA += a;
          sp.denC += c;
          sp.numA += sp.denA;
          sp.numC += sp.denC;
        }
      } else {
        for (j = sp.lookbackTotal; j >= 0; j -= 1) {
          y = sp.win_j_inReal[(sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j] * sp.scale;
          t = y * sp.invWidthSq;
          a = t + 6755399441055744.0 - 6755399441055744.0;
          t = a * sp.widthSq;
          q = y - t;
          t = q * sp.invWidth;
          b = t + 6755399441055744.0 - 6755399441055744.0;
          t = b * sp.width;
          c = q - t;
          sp.denA += a;
          sp.denB += b;
          sp.denC += c;
          sp.numA += sp.denA;
          sp.numB += sp.denB;
          sp.numC += sp.denC;
        }
      }
    }
  }
  /* One rounding each: the limbs are carried into range first, so the
   * last add sees two exact values.
   */
  if (sp.limbs === 2) {
    t = sp.denA * sp.width;
    sp.den = t + sp.denC;
    t = sp.numA * sp.width;
    sp.num = t + sp.numC;
  } else if (sp.limbs === 3) {
    t = sp.denC * sp.invWidth;
    q = t + 6755399441055744.0 - 6755399441055744.0;
    b = sp.denB + q;
    t = q * sp.width;
    c = sp.denC - t;
    t = b * sp.invWidth;
    q = t + 6755399441055744.0 - 6755399441055744.0;
    t = q * sp.width;
    b = b - t;
    t = b * sp.width;
    c = t + c;
    t = sp.denA + q;
    t = t * sp.widthSq;
    sp.den = t + c;
    t = sp.numC * sp.invWidth;
    q = t + 6755399441055744.0 - 6755399441055744.0;
    b = sp.numB + q;
    t = q * sp.width;
    c = sp.numC - t;
    t = b * sp.invWidth;
    q = t + 6755399441055744.0 - 6755399441055744.0;
    t = q * sp.width;
    b = b - t;
    t = b * sp.width;
    c = t + c;
    t = sp.numA + q;
    t = t * sp.widthSq;
    sp.num = t + c;
  }
  /* The denominator is a signed sum, so only an exact zero is degenerate.
   * It is answered with the flat-window value, which keeps every output a
   * function of its own window; an epsilon band would carry the quote unit
   * (#253).
   */
  if (sp.den !== 0.0) {
    value = -sp.num / sp.den;
  } else {
    value = sp.flatValue;
  }
  if (sp.limbs === 2) {
    y = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] * sp.scale;
    t = y * sp.invWidth;
    a = t + 6755399441055744.0 - 6755399441055744.0;
    t = a * sp.width;
    c = y - t;
    sp.denA -= a;
    sp.denC -= c;
    sp.numA -= sp.periodDouble * a;
    sp.numC -= sp.periodDouble * c;
  } else if (sp.limbs === 3) {
    y = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] * sp.scale;
    t = y * sp.invWidthSq;
    a = t + 6755399441055744.0 - 6755399441055744.0;
    t = a * sp.widthSq;
    q = y - t;
    t = q * sp.invWidth;
    b = t + 6755399441055744.0 - 6755399441055744.0;
    t = b * sp.width;
    c = q - t;
    sp.denA -= a;
    sp.denB -= b;
    sp.denC -= c;
    sp.numA -= sp.periodDouble * a;
    sp.numB -= sp.periodDouble * b;
    sp.numC -= sp.periodDouble * c;
  }
  /* After the trailing value is read: outReal may be inReal. */
  sp.cur_outReal = value;
  ringCapL_trailingIdx = sp.ringCap_trailingIdx;
  sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
  sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
  if (sp.ringPos_trailingIdx >= ringCapL_trailingIdx) {
    sp.ringPos_trailingIdx = 0;
  }
  sp.winPos_j = sp.winPos_j + 1;
  if (sp.winPos_j >= sp.winCap_j) {
    sp.winPos_j = 0;
  }
}

function cgOpenImpl(
  sp: CgStream,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  const core = sp.core;
  let today = 0;
  let outIdx = 0;
  let lookbackTotal = 0;
  let trailingIdx = 0;
  let j = 0;
  let k = 0;
  let limbs = 0;
  let fits = 0;
  let maxAt = 0;
  let failAt = 0;
  let stickyBars = 0;
  let fit2Mid = 0;
  let fit2Lo = 0;
  let fit3Mid = 0;
  let num = 0;
  let den = 0;
  let value = 0;
  let periodDouble = 0;
  let flatValue = 0;
  let weightTotal = 0;
  let width2 = 0;
  let width3 = 0;
  let ylim2 = 0;
  let ylim3 = 0;
  let head2 = 0;
  let head3 = 0;
  let scale = 0;
  let width = 0;
  let invWidth = 0;
  let widthSq = 0;
  let invWidthSq = 0;
  let ylim = 0;
  let maxAbs = 0;
  let half = 0;
  let scale2Mid = 0;
  let scale2Lo = 0;
  let scale3Mid = 0;
  let scale3Lo = 0;
  let x = 0;
  let y = 0;
  let a = 0;
  let b = 0;
  let c = 0;
  let q = 0;
  let t = 0;
  let denA = 0;
  let denB = 0;
  let denC = 0;
  let numA = 0;
  let numB = 0;
  let numC = 0;
  const historyLen = inReal.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 10;
  } else if (optInTimePeriod < 2 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  outBegIdx.value = 0;
  outNBElement.value = 0;
  lookbackTotal = cgLookbackPositional(optInTimePeriod, core);
  if (startIdx < lookbackTotal) {
    startIdx = lookbackTotal;
  }
  if (startIdx > endIdx) {
    return RetCode.INSUFFICIENT_HISTORY;
  }
  periodDouble = optInTimePeriod;
  flatValue = -(periodDouble + 1.0) * 0.5;
  /* Each window value is held as y = x*scale, an integer, split into 2 or 3
   * integer-valued limbs of `width`. The bounds keep every limb of both the
   * weighted and the plain sum below 2^53 while the total weight is at most
   * 2^tb, so every add and subtract below is exact, and every product is by a
   * power of two or of two integers whose result is below 2^53. That makes
   * each output depend on its window alone, whatever scale or limb count the
   * call reached it with. Keep it that way: a rounded operation anywhere in
   * the limb arithmetic makes the output depend on the start index.
   *
   * 2 limbs: width 2^(53-tb), |y| < 2^(104-2tb).
   * 3 limbs: width min(2^(53-tb), 2^26), |y| < 2^(51-tb) * width^2.
   *
   * Keep every product out of an addition in the same expression: each
   * product is exact, so fusing would change no value, only cost a call
   * where FMA is not inlined.
   */
  weightTotal = 1.0;
  while (weightTotal < periodDouble * (periodDouble + 1.0) * 0.5) {
    weightTotal *= 2.0;
  }
  width2 = 9007199254740992.0 / weightTotal;
  ylim2 = 0.25 * width2 * width2;
  width3 = width2;
  if (width3 > 67108864.0) {
    width3 = 67108864.0;
  }
  ylim3 = 0.25 * width2 * width3 * width3;
  /* A scale below the largest that fits leaves room for the window's
   * magnitude to grow before a rebuild, split evenly with the room left for
   * finer values.
   */
  head2 = 1.0;
  while (head2 * head2 * 9007199254740992.0 < ylim2) {
    head2 *= 2.0;
  }
  head3 = 1.0;
  while (head3 * head3 * 9007199254740992.0 < ylim3) {
    head3 *= 2.0;
  }
  limbs = 0;
  stickyBars = 0;
  scale = 1.0;
  width = 1.0;
  invWidth = 1.0;
  widthSq = 1.0;
  invWidthSq = 1.0;
  ylim = 0.0;
  half = 1.0;
  denA = 0.0;
  denB = 0.0;
  denC = 0.0;
  numA = 0.0;
  numB = 0.0;
  numC = 0.0;
  num = 0.0;
  den = 0.0;
  outIdx = 0;
  today = startIdx;
  trailingIdx = startIdx - lookbackTotal;
  while (today <= endIdx) {
    /* Between bars the sums hold the window less its oldest value. */
    fits = 0;
    x = inReal[today];
    y = x * scale;
    if (limbs === 2) {
      if (Math.abs(y) < ylim && (y !== 0.0 || x === 0.0)) {
        t = y * invWidth;
        a = t + 6755399441055744.0 - 6755399441055744.0;
        t = a * width;
        c = y - t;
        if (c + 6755399441055744.0 - 6755399441055744.0 === c) {
          denA += a;
          denC += c;
          numA += denA;
          numC += denC;
          fits = 1;
        }
      }
    } else if (limbs === 3) {
      if (Math.abs(y) < ylim && (y !== 0.0 || x === 0.0)) {
        t = y * invWidthSq;
        a = t + 6755399441055744.0 - 6755399441055744.0;
        t = a * widthSq;
        q = y - t;
        t = q * invWidth;
        b = t + 6755399441055744.0 - 6755399441055744.0;
        t = b * width;
        c = q - t;
        if (c + 6755399441055744.0 - 6755399441055744.0 === c) {
          denA += a;
          denB += b;
          denC += c;
          numA += denA;
          numB += denB;
          numC += denC;
          fits = 1;
        }
      }
    }
    if (fits === 0 && limbs === 0 && stickyBars > 0) {
      /* Both witnesses of the last failed fit are still in the window, so
       * it cannot fit either: its largest value can only be larger, which
       * only coarsens the scale the failing value already missed.
       */
      stickyBars -= 1;
      num = 0.0;
      den = 0.0;
      for (j = today - lookbackTotal; j <= today; j += 1) {
        den += inReal[j];
        num += den;
      }
    } else if (fits === 0) {
      /* The incoming value does not fit the scale: pick one from the
       * window alone.
       */
      maxAbs = 0.0;
      maxAt = 0;
      k = 0;
      for (j = today - lookbackTotal; j <= today; j += 1) {
        x = Math.abs(inReal[j]);
        if (x >= maxAbs) {
          maxAbs = x;
          maxAt = k;
        }
        k += 1;
      }
      /* The largest power of two not above the window's largest
       * magnitude. The search may start from any finite power of two, so
       * it starts from the last window's; keep it uncapped, or a start far
       * from the answer stops short and the fit then depends on the
       * previous window. A window with an infinite magnitude fails the fit
       * at any scale, so it keeps the last one.
       */
      if (maxAbs > 0.0 && maxAbs <= 1.7976931348623157e308) {
        while (half * 65536.0 <= maxAbs) {
          half *= 65536.0;
        }
        while (half * 2.0 <= maxAbs) {
          half *= 2.0;
        }
        while (half > maxAbs * 65536.0) {
          half *= 1.52587890625e-5;
        }
        while (half > maxAbs) {
          half *= 0.5;
        }
      }
      /* The largest scale each limb count allows, and the same with
       * headroom, at most 2^1022 so that the scale itself is finite. The
       * largest 3-limb scale is the most permissive there is: the window
       * fits some scale only if every value is an integer at that one. A
       * scale below 1 can round a small value to 0, which is not a fit.
       */
      scale2Lo = ylim2 * 0.5 / half;
      if (scale2Lo > 4.49423283715579e307) {
        scale2Lo = 4.49423283715579e307;
      }
      scale2Mid = scale2Lo / head2;
      scale3Lo = ylim3 * 0.5 / half;
      if (scale3Lo > 4.49423283715579e307) {
        scale3Lo = 4.49423283715579e307;
      }
      scale3Mid = scale3Lo / head3;
      /* The same pass sums the window as it stands, oldest first, for when
       * nothing fits: non-finite values, bits below 2^-1022, or too wide a
       * span of magnitudes.
       */
      fit2Mid = 1;
      fit2Lo = 1;
      fit3Mid = 1;
      failAt = -1;
      num = 0.0;
      den = 0.0;
      k = 0;
      for (j = today - lookbackTotal; j <= today; j += 1) {
        x = inReal[j];
        den += x;
        num += den;
        y = Math.abs(x * scale2Mid);
        if (!(y < ylim2) || y < 4503599627370496.0 && y + 4503599627370496.0 - 4503599627370496.0 !== y || y === 0.0 && x !== 0.0) {
          fit2Mid = 0;
        }
        y = Math.abs(x * scale2Lo);
        if (!(y < ylim2) || y < 4503599627370496.0 && y + 4503599627370496.0 - 4503599627370496.0 !== y || y === 0.0 && x !== 0.0) {
          fit2Lo = 0;
        }
        y = Math.abs(x * scale3Mid);
        if (!(y < ylim3) || y < 4503599627370496.0 && y + 4503599627370496.0 - 4503599627370496.0 !== y || y === 0.0 && x !== 0.0) {
          fit3Mid = 0;
        }
        y = Math.abs(x * scale3Lo);
        if (!(y < ylim3) || y < 4503599627370496.0 && y + 4503599627370496.0 - 4503599627370496.0 !== y || y === 0.0 && x !== 0.0) {
          failAt = k;
        }
        k += 1;
      }
      limbs = 0;
      if (failAt >= 0) {
        if (failAt < maxAt) {
          stickyBars = failAt;
        } else {
          stickyBars = maxAt;
        }
      } else {
        if (fit2Mid === 1 || fit2Lo === 1) {
          limbs = 2;
          width = width2;
          ylim = ylim2;
          if (fit2Mid === 1) {
            scale = scale2Mid;
          } else {
            scale = scale2Lo;
          }
        } else {
          limbs = 3;
          width = width3;
          ylim = ylim3;
          if (fit3Mid === 1) {
            scale = scale3Mid;
          } else {
            scale = scale3Lo;
          }
        }
        invWidth = 1.0 / width;
        widthSq = width * width;
        invWidthSq = invWidth * invWidth;
        denA = 0.0;
        denB = 0.0;
        denC = 0.0;
        numA = 0.0;
        numB = 0.0;
        numC = 0.0;
        if (limbs === 2) {
          for (j = today - lookbackTotal; j <= today; j += 1) {
            y = inReal[j] * scale;
            t = y * invWidth;
            a = t + 6755399441055744.0 - 6755399441055744.0;
            t = a * width;
            c = y - t;
            denA += a;
            denC += c;
            numA += denA;
            numC += denC;
          }
        } else {
          for (j = today - lookbackTotal; j <= today; j += 1) {
            y = inReal[j] * scale;
            t = y * invWidthSq;
            a = t + 6755399441055744.0 - 6755399441055744.0;
            t = a * widthSq;
            q = y - t;
            t = q * invWidth;
            b = t + 6755399441055744.0 - 6755399441055744.0;
            t = b * width;
            c = q - t;
            denA += a;
            denB += b;
            denC += c;
            numA += denA;
            numB += denB;
            numC += denC;
          }
        }
      }
    }
    /* One rounding each: the limbs are carried into range first, so the
     * last add sees two exact values.
     */
    if (limbs === 2) {
      t = denA * width;
      den = t + denC;
      t = numA * width;
      num = t + numC;
    } else if (limbs === 3) {
      t = denC * invWidth;
      q = t + 6755399441055744.0 - 6755399441055744.0;
      b = denB + q;
      t = q * width;
      c = denC - t;
      t = b * invWidth;
      q = t + 6755399441055744.0 - 6755399441055744.0;
      t = q * width;
      b = b - t;
      t = b * width;
      c = t + c;
      t = denA + q;
      t = t * widthSq;
      den = t + c;
      t = numC * invWidth;
      q = t + 6755399441055744.0 - 6755399441055744.0;
      b = numB + q;
      t = q * width;
      c = numC - t;
      t = b * invWidth;
      q = t + 6755399441055744.0 - 6755399441055744.0;
      t = q * width;
      b = b - t;
      t = b * width;
      c = t + c;
      t = numA + q;
      t = t * widthSq;
      num = t + c;
    }
    /* The denominator is a signed sum, so only an exact zero is degenerate.
     * It is answered with the flat-window value, which keeps every output a
     * function of its own window; an epsilon band would carry the quote unit
     * (#253).
     */
    if (den !== 0.0) {
      value = -num / den;
    } else {
      value = flatValue;
    }
    if (limbs === 2) {
      y = inReal[trailingIdx] * scale;
      t = y * invWidth;
      a = t + 6755399441055744.0 - 6755399441055744.0;
      t = a * width;
      c = y - t;
      denA -= a;
      denC -= c;
      numA -= periodDouble * a;
      numC -= periodDouble * c;
    } else if (limbs === 3) {
      y = inReal[trailingIdx] * scale;
      t = y * invWidthSq;
      a = t + 6755399441055744.0 - 6755399441055744.0;
      t = a * widthSq;
      q = y - t;
      t = q * invWidth;
      b = t + 6755399441055744.0 - 6755399441055744.0;
      t = b * width;
      c = q - t;
      denA -= a;
      denB -= b;
      denC -= c;
      numA -= periodDouble * a;
      numB -= periodDouble * b;
      numC -= periodDouble * c;
    }
    /* After the trailing value is read: outReal may be inReal. */
    outReal[((outIdx * outStride) | 0)] = value;
    trailingIdx += 1;
    outIdx += 1;
    today += 1;
  }
  outBegIdx.value = startIdx;
  outNBElement.value = outIdx;
  const cap_trailingIdx = today - trailingIdx;
  if (cap_trailingIdx < 1 || cap_trailingIdx > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const allocN_trailingIdx = cap_trailingIdx > 0 ? cap_trailingIdx : 1;
  const capRing_trailingIdx_inReal = new Float64Array(allocN_trailingIdx);
  capRing_trailingIdx_inReal.set(inReal.subarray(historyLen - cap_trailingIdx, historyLen), 0);
  const cap_j = lookbackTotal + 1;
  if (cap_j < 1 || cap_j > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  const capWin_j_inReal = inReal.slice(historyLen - cap_j, historyLen);
  sp.optInTimePeriod = optInTimePeriod;
  sp.lookbackTotal = lookbackTotal;
  sp.limbs = limbs;
  sp.stickyBars = stickyBars;
  sp.num = num;
  sp.den = den;
  sp.periodDouble = periodDouble;
  sp.flatValue = flatValue;
  sp.width2 = width2;
  sp.width3 = width3;
  sp.ylim2 = ylim2;
  sp.ylim3 = ylim3;
  sp.head2 = head2;
  sp.head3 = head3;
  sp.scale = scale;
  sp.width = width;
  sp.invWidth = invWidth;
  sp.widthSq = widthSq;
  sp.invWidthSq = invWidthSq;
  sp.ylim = ylim;
  sp.half = half;
  sp.denA = denA;
  sp.denB = denB;
  sp.denC = denC;
  sp.numA = numA;
  sp.numB = numB;
  sp.numC = numC;
  sp.ringPos_trailingIdx = 0;
  sp.ringCap_trailingIdx = cap_trailingIdx;
  sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
  sp.winPos_j = 0;
  sp.winCap_j = cap_j;
  sp.win_j_inReal = capWin_j_inReal;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function cgOpenAndFillInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): CgStream {
  const sp = CgStream.of(core);
  const retCode = cgOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CG openAndFill', inReal.length, startIdx, cgLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && cgLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('CG openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('CG openAndFill', retCode);
}

/** @internal */
export function cgOpenInternal(
  core: Core,
  inReal: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): CgStream {
  const sp = CgStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = cgOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('CG open', inReal.length, startIdx, cgLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && cgLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('CG open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('CG open', retCode);
}

/** @internal */
export function cgOpenPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): CgStream {
  optInTimePeriod = requireInt32('CG open', 'timePeriod', optInTimePeriod);
  requireArgument('CG open', 'real', inReal);
  requireHistory('CG open', 'real', inReal.length);
  return cgOpenInternal(core, float64(inReal), 0, optInTimePeriod);
}

/** @internal */
export function cgOpenAndFillPositional(
  inReal: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): CgStream {
  optInTimePeriod = requireInt32('CG openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('CG openAndFill', 'real', inReal);
  requireHistory('CG openAndFill', 'real', inReal.length);
  const guardLookback = cgLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('CG openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('CG openAndFill', inReal.length, guardLookback);
  requireOutput('CG openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inRealF64 = float64(inReal);
  if (outReal.buffer === inRealF64.buffer && outReal.byteOffset === inRealF64.byteOffset) {
    throw sharedMemory('CG openAndFill', 'outReal', 'real');
  }
  if (guardOutLen > 0 && overlaps(outReal, inRealF64)) {
    throw sharedMemory('CG openAndFill', 'outReal', 'real');
  }
  return cgOpenAndFillInternal(core, inRealF64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live CG stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link cg} there.
 *
 * @param real - Warm-up history. The series to measure.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link CgOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cgLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32 or a parameter outside its range.
 *
 * @example
 * ```ts
 * import { cgOpen } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const stream = cgOpen(data, { timePeriod: 10 });
 * const peeked = stream.peek(100.9); // a forming bar: commits nothing
 * const updated = stream.update(100.9); // the bar closes
 * ```
 */
export function cgOpen(real: ArrayLike<number>, options?: CgOptions): CgStream {
  const o = optionsOf('CG open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('CG open', o.core);
  return cgOpenPositional(real, optInTimePeriod, core);
}

/**
 * {@link cgOpen} that also computes the outputs over the whole history in the same
 * pass, bit-identically to {@link cg}.
 *
 * @param real - Warm-up history. The series to measure.
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link CgOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link cg} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `cgLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or a
 *   buffer too short, of the wrong type or starting where another buffer or a
 *   `Float64Array` input starts, or, on a history long enough to open, one sharing
 *   memory with another buffer or a `Float64Array` input.
 *
 * @example
 * ```ts
 * import { cgOpenAndFill } from 'ta-lib';
 *
 * const data = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const { stream, begIdx, values } = cgOpenAndFill(data, { timePeriod: 10 });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function cgOpenAndFill(real: ArrayLike<number>, options?: CgOptions): CgResult & { stream: CgStream } {
  const o = optionsOf('CG openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('CG openAndFill', o.core);
  const capacity = fillCapacity(cgLookbackPositional(optInTimePeriod, core), real);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = cgOpenAndFillPositional(real, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
