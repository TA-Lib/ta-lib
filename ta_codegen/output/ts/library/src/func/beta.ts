/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MW       Michael Williamson
 *  CC       Claude Code (AI assistant)
 *
 *
 * Change history:
 *
 *  MMDDYY BY    Description
 *  -------------------------------------------------------------------
 *  122006 MW    Initial Version
 *  071626 MF,CC Fix reversed inReal0/inReal1 roles in the algorithm
 *               description: inReal0 holds the index prices and inReal1
 *               the stock prices (SourceForge bug 98).
 *  082326 MF    Fix #242. Cancellation-free regression sums (shifted returns
 *               + reseed) and a scale-relative denominator test.
 *  082326 MF,CC #242 follow-up: restore TA_VAR's outlier trigger, at 1e3,
 *               on BOTH axes -- the output reads S_xy and S_y too.
 *  082326 MF,CC Fix #253. Test the base price of a return exactly instead of
 *               against the fixed TA_IS_ZERO band, which collapsed beta to
 *               zero for any instrument quoted small enough to fall under it.
 */

import { batchCapacity, clampedStart, coreOf, failure, fillCapacity, float64, insufficientHistory, nonFiniteBar, openFillCount, optionsOf, orDefault, overlaps, overlapsPartly, rejectedParams, requireArgument, requireHistory, requireHistoryLength, requireIndexRange, requireInt32, requireLength, requireOutput, requireSeries, sharedMemory, streamFailure } from '../runtime/checks.ts';
import { Core } from '../runtime/core.ts';
import { RetCode } from '../runtime/errors.ts';
import { MInteger } from '../runtime/mInteger.ts';
import { OutRange } from '../runtime/outRange.ts';

/** @internal */
export function betaLookbackPositional(optInTimePeriod: number, _core: Core = Core.DEFAULT): number {
  if ((optInTimePeriod | 0) !== optInTimePeriod) {
    return -1;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 5;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return -1;
  }
  return optInTimePeriod;
}

/** @internal */
export function betaDisplayShiftPositional(optInTimePeriod: number, outputIdx: number, core: Core = Core.DEFAULT): number {
  if (betaLookbackPositional(optInTimePeriod, core) < 0) {
    return -2147483648;
  }
  if ((outputIdx | 0) !== outputIdx || outputIdx < 0 || outputIdx >= 1) {
    return -2147483648;
  }
  return 0;
}

/** @internal */
export function betaImpl(
  _core: Core,
  startIdx: number,
  endIdx: number,
  inReal0: Float64Array,
  inReal1: Float64Array,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): RetCode {
  let S_xx = 0;
  let S_xy = 0;
  let S_x = 0;
  let S_y = 0;
  let last_price_x = 0;
  let last_price_y = 0;
  let trailing_last_price_x = 0;
  let trailing_last_price_y = 0;
  let tmp_real = 0;
  let shift_x = 0;
  let shift_y = 0;
  let denom = 0;
  let denom_scale = 0;
  let prev_x = 0;
  let leaving_xx = 0;
  let leaving_yy = 0;
  let S_yy = 0;
  let prev_y = 0;
  let j = 0;
  let windowStart = 0;
  let barsSinceReseed = 0;
  let x = 0;
  let y = 0;
  let n = 0;
  let i = 0;
  let outIdx = 0;
  let trailingIdx = 0;
  let nbInitialElementNeeded = 0;
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    return RetCode.OUT_OF_RANGE_START_INDEX;
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 5;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  S_xx = 0.0;
  S_xy = 0.0;
  S_x = 0.0;
  S_y = 0.0;
  last_price_x = 0.0;
  last_price_y = 0.0;
  trailing_last_price_x = 0.0;
  trailing_last_price_y = 0.0;
  tmp_real = 0.0;
  shift_x = 0.0;
  shift_y = 0.0;
  denom = 0.0;
  denom_scale = 0.0;
  prev_x = 0.0;
  leaving_xx = 0.0;
  leaving_yy = 0.0;
  S_yy = 0.0;
  prev_y = 0.0;
  n = 0.0;
  /* sum of x * x */
  /* sum of x * y */
  /* sum of x */
  /* sum of y */
  /* the last price read from inReal0 */
  /* the last price read from inReal1 */
  /* same as last_price_x except used to remove elements from the trailing summation */
  /* same as last_price_y except used to remove elements from the trailing summation */
  /* temporary variable */
  /* origin the x returns are measured against */
  /* origin the y returns are measured against */
  /* n*S_xx - S_x*S_x, the regression denominator */
  /* n*S_xx, the scale denom is extracted from */
  /* price walked forward when rebuilding the window */
  /* squared x deviation the previous bar removed */
  /* squared y deviation the previous bar removed */
  /* sum of y * y, carried ONLY for the outlier trigger */
  /* the 'x' value, which is the last change between values in inReal0 */
  /* the 'y' value, which is the last change between values in inReal1 */
  /* DESCRIPTION OF ALGORITHM:
   *   The Beta 'algorithm' is a measure of a stocks volatility vs from index. The index prices
   *   are given in inReal0 and the stock prices are given in inReal1. The size of these vectors
   *   should be equal. The algorithm is to calculate the change between prices in both vectors
   *   and then 'plot' these changes are points in the Euclidean plane. The x value of the point
   *   is market return and the y value is the security return. The beta value is the slope of a
   *   linear regression through these points. A beta of 1 is simple the line y=x, so the stock
   *   varies percisely with the market. A beta of less than one means the stock varies less than
   *   the market and a beta of more than one means the stock varies more than market. A related
   *   value is the Alpha value (see TA_ALPHA) which is the Y-intercept of the same linear regression.
   */
  /* Validate the calculation method type and
   * identify the minimum number of input
   * consume before the first value is output..
   */
  nbInitialElementNeeded = optInTimePeriod;
  /* Move up the start index if there is not
   * enough initial data.
   */
  if (startIdx < nbInitialElementNeeded) {
    startIdx = nbInitialElementNeeded;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.SUCCESS;
  }
  /* Consume first input. */
  trailingIdx = startIdx - nbInitialElementNeeded;
  trailing_last_price_x = inReal0[trailingIdx];
  last_price_x = trailing_last_price_x;
  trailing_last_price_y = inReal1[trailingIdx];
  last_price_y = trailing_last_price_y;
  /* Measure the returns against a shift near the window, as TA_VAR does for
   * its values (#118) and TA_CORREL for its prices (#242).
   *
   * Returns are near-zero-mean, so on a series that jitters this changes
   * little. It is decisive on a series that DRIFTS: a steadily rising price
   * gives near-identical returns, S_x*S_x then equals n*S_xx to every digit,
   * and the denominator n*S_xx - S_x*S_x is left as pure rounding noise --
   * measured at +/-2e-16 of its own scale, sign included. Wilkinson's BIG and
   * LITTLE are exactly that shape, and beta of such a series against itself
   * came back 0 instead of 1.
   *
   * Anchor on the window's first return here; every later re-anchor uses the
   * window mean, which is better centred but costs a pass this one cannot
   * afford before the sums exist.
   */
  i = ++trailingIdx;
  /* A return needs a non-zero base price and nothing more: the test is exact,
   * not the fixed TA_IS_ZERO band it used to be. A price carries the quote
   * unit, so that band declared every bar of a small-quoted instrument
   * "no previous price", left every return at -shift, and collapsed beta to
   * zero (issue #253).
   */
  if (last_price_x !== 0.0) {
    shift_x = (inReal0[i] - last_price_x) / last_price_x;
  }
  if (last_price_y !== 0.0) {
    shift_y = (inReal1[i] - last_price_y) / last_price_y;
  }
  while (i < startIdx) {
    tmp_real = inReal0[i];
    if (last_price_x !== 0.0) {
      x = (tmp_real - last_price_x) / last_price_x - shift_x;
    } else {
      x = -shift_x;
    }
    last_price_x = tmp_real;
    tmp_real = inReal1[i++];
    if (last_price_y !== 0.0) {
      y = (tmp_real - last_price_y) / last_price_y - shift_y;
    } else {
      y = -shift_y;
    }
    last_price_y = tmp_real;
    S_xx += x * x;
    S_yy += y * y;
    S_xy += x * y;
    S_x += x;
    S_y += y;
  }
  outIdx = 0;
  /* First output always start at index zero */
  n = optInTimePeriod;
  barsSinceReseed = ((32 * optInTimePeriod) | 0);
  do {
    tmp_real = inReal0[i];
    if (last_price_x !== 0.0) {
      x = (tmp_real - last_price_x) / last_price_x - shift_x;
    } else {
      x = -shift_x;
    }
    last_price_x = tmp_real;
    tmp_real = inReal1[i++];
    if (last_price_y !== 0.0) {
      y = (tmp_real - last_price_y) / last_price_y - shift_y;
    } else {
      y = -shift_y;
    }
    last_price_y = tmp_real;
    S_xx += x * x;
    S_yy += y * y;
    S_xy += x * y;
    S_x += x;
    S_y += y;
    denom_scale = n * S_xx;
    denom = denom_scale - S_x * S_x;
    /* Re-anchor and rebuild when the shift has gone stale. Three triggers:
     * the denominator has shrunk below 1e-6 of the scale
     * it is extracted from; OR the return that just left sat so far from the
     * shift that its squared term dwarfs what remains; OR at least every 32
     * windows.
     *
     * The outlier trigger earns its multiply and compare here, contrary to
     * what "returns are stationary" suggests: a bad tick makes one return
     * enormous, the ordinary ones fall below its ulp and are never really
     * added, and when it leaves the subtraction takes back a term they were
     * never part of. The residue is a consistent OFFSET, so the cancellation
     * trigger above cannot see it -- denom/denom_scale stays ~1 -- and only
     * the periodic re-anchor recovers, up to 32*period bars later. Measured
     * without it: a 1e8 tick left 286 of 386 bars wrong, the worst by 0.36
     * ABSOLUTE. Cost is ~3% and mostly unmeasurable on the bench corpus
     * (randwalk/GBM/trend-chop), where it fires on 0.00% of bars -- but that
     * is a corpus figure, not a bound. Isolated against the same body without
     * the disjunct it is +16-20% on a stale-quote/illiquid series (1.5% fire
     * rate) and +54-64% on constructed near-flat or gapped shapes (5.1%).
     * The cost is the reseed it triggers, so it tracks the fire rate; on data
     * that never triggers it, the compare is free.
     *
     * BOTH axes are watched, and the y one is not redundant. The denominator
     * is x-only, so it is tempting to conclude -- as an earlier draft of this
     * did -- that a y trigger catches nothing. It catches plenty: the OUTPUT
     * also reads S_xy and S_y, which a y-side outlier corrupts with nothing on
     * the x side able to see it. Measured on test_beta_outlier_transit's own
     * ladder with the spike moved from px to py: 12 of 24 rungs fail without
     * the second disjunct, worst 156x relative; 0 of 24 with it. The earlier
     * experiment that found it inert was run on an x-only corpus, where it is
     * inert by construction. TA_CORREL, fixed by the same #242 work, watches
     * both from the start; this brings BETA level. S_yy exists only to scale
     * this test -- nothing else reads it.
     *
     * The threshold is 1e3, not the 1e6 a price-scale series takes, because a
     * return amplifies: a tick multiplying the price by k puts k-1 into the
     * return and (k-1)^2 into S_xx, so the ratio when that term leaves lands
     * an order or two below the value-scale case. At 1e6 a 1e5 tick slips
     * through and leaves a flat 2.5e-5 relative error on 285 of 386 bars.
     * Pinned by test_beta_outlier_transit.
     *
     * Reading the window here is safe when outReal aliases an input: the
     * outputs written so far occupy [0, outIdx-1] while windowStart-1 is
     * startIdx-optInTimePeriod+outIdx, which is >= outIdx.
     */
    barsSinceReseed -= 1;
    if (denom < 1e-6 * denom_scale || leaving_xx > 1000.0 * S_xx || leaving_yy > 1000.0 * S_yy || barsSinceReseed <= 0) {
      barsSinceReseed = ((32 * optInTimePeriod) | 0);
      windowStart = trailingIdx;
      /* Walk the window forward from the price the trailing cursor already
       * carries. A return needs its predecessor, and reading inReal[j-1]
       * would reach one slot BEFORE the window -- which the batch can do and
       * a streaming ring sized for the window cannot. trailing_last_price_*
       * IS that predecessor, so carrying it forward keeps every read inside
       * [trailingIdx, i-1] and the two paths stay identical.
       */
      prev_x = trailing_last_price_x;
      prev_y = trailing_last_price_y;
      tmp_real = 0.0;
      shift_y = 0.0;
      for (j = windowStart; j < i; j += 1) {
        if (prev_x !== 0.0) {
          tmp_real += (inReal0[j] - prev_x) / prev_x;
        }
        prev_x = inReal0[j];
        if (prev_y !== 0.0) {
          shift_y += (inReal1[j] - prev_y) / prev_y;
        }
        prev_y = inReal1[j];
      }
      shift_x = tmp_real / n;
      shift_y = shift_y / n;
      prev_x = trailing_last_price_x;
      prev_y = trailing_last_price_y;
      S_xx = 0.0;
      S_yy = 0.0;
      S_xy = 0.0;
      S_x = 0.0;
      S_y = 0.0;
      for (j = windowStart; j < i; j += 1) {
        if (prev_x !== 0.0) {
          x = (inReal0[j] - prev_x) / prev_x - shift_x;
        } else {
          x = -shift_x;
        }
        prev_x = inReal0[j];
        if (prev_y !== 0.0) {
          y = (inReal1[j] - prev_y) / prev_y - shift_y;
        } else {
          y = -shift_y;
        }
        prev_y = inReal1[j];
        S_xx += x * x;
        S_yy += y * y;
        S_xy += x * y;
        S_x += x;
        S_y += y;
      }
      denom_scale = n * S_xx;
      denom = denom_scale - S_x * S_x;
      /* n*S_xx - S_x*S_x is non-negative by Cauchy-Schwarz, but it is
       * extracted as a difference, so its SIGN is not guaranteed on a window
       * whose returns are all the same value. Enforce the invariant HERE and
       * not at the divide: a negative denom always reseeds on the same bar
       * (it makes the first trigger true whenever denom_scale is positive,
       * and denom_scale == 0 reduces that trigger to `denom < 0`), so the
       * divide below can rely on it being >= 0.
       */
      if (denom < 0.0) {
        denom = 0.0;
      }
    }
    /* Always read the trailing before writing the output because the input and output
     * buffer can be the same.
     */
    tmp_real = inReal0[trailingIdx];
    if (trailing_last_price_x !== 0.0) {
      x = (tmp_real - trailing_last_price_x) / trailing_last_price_x - shift_x;
    } else {
      x = -shift_x;
    }
    trailing_last_price_x = tmp_real;
    tmp_real = inReal1[trailingIdx];
    trailingIdx += 1;
    if (trailing_last_price_y !== 0.0) {
      y = (tmp_real - trailing_last_price_y) / trailing_last_price_y - shift_y;
    } else {
      y = -shift_y;
    }
    trailing_last_price_y = tmp_real;
    /* Write the output.
     *
     * The denominator is tested against ITS OWN scale, not a fixed band: it
     * is quadratic in the return volatility, so an absolute 1e-14 threshold
     * stops meaning "the regressor does not vary" and starts meaning "the
     * returns are small". The literal is TA_EPSILON, and the plain `>` also
     * rejects a negative denominator rather than dividing by it.
     */
    if (denom > 1e-14 * denom_scale) {
      outReal[outIdx++] = (n * S_xy - S_x * S_y) / denom;
    } else {
      outReal[outIdx++] = 0.0;
    }
    /* Remove the calculation starting with the trailingIdx. */
    leaving_xx = x * x;
    leaving_yy = y * y;
    S_xx -= x * x;
    S_yy -= y * y;
    S_xy -= x * y;
    S_x -= x;
    S_y -= y;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  return RetCode.SUCCESS;
}

const PARAM_RANGES = [['timePeriod', Core.INTEGER_DEFAULT, 1, 100000, 'in 1..100000']] as const;

/** @internal */
export function betaPositional(
  startIdx: number,
  endIdx: number,
  inReal0: ArrayLike<number>,
  inReal1: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): OutRange {
  startIdx = requireInt32('BETA', 'startIdx', startIdx);
  endIdx = requireInt32('BETA', 'endIdx', endIdx);
  optInTimePeriod = requireInt32('BETA', 'timePeriod', optInTimePeriod);
  requireIndexRange('BETA', startIdx, endIdx);
  const guardLookback = betaLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('BETA', PARAM_RANGES, optInTimePeriod);
  }
  const guardStart = clampedStart('BETA', startIdx, guardLookback);
  const guardInLen = endIdx + 1;
  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
  requireLength('BETA', 'real0', inReal0, guardInLen);
  requireLength('BETA', 'real1', inReal1, guardInLen);
  requireOutput('BETA', 'outReal', outReal, guardOutLen, Float64Array);
  const inReal0F64 = float64(inReal0);
  const inReal1F64 = float64(inReal1);
  if (overlapsPartly(outReal, inReal0F64)) {
    throw sharedMemory('BETA', 'outReal', 'real0');
  }
  if (overlapsPartly(outReal, inReal1F64)) {
    throw sharedMemory('BETA', 'outReal', 'real1');
  }
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const retCode = betaImpl(core, startIdx, endIdx, inReal0F64, inReal1F64, optInTimePeriod, outBegIdx, outNBElement, outReal);
  if (retCode !== RetCode.SUCCESS) {
    throw failure('BETA', retCode);
  }
  return new OutRange(outBegIdx.value, outNBElement.value);
}

/**
 * Options of {@link beta}, {@link betaLookback}, {@link betaDisplayShift},
 * {@link betaOpen} and {@link betaOpenAndFill}. Every key is optional: an omitted
 * or `undefined` key takes its default, and a call ignores the keys it has no use
 * for.
 */
export interface BetaOptions {
  /**
   * Rolling window length (number of returns) for the regression sums (default 5;
   * range 1 to 100000; `Core.INTEGER_DEFAULT` selects the default).
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

/** What {@link beta} returns, and {@link betaOpenAndFill} with its stream. */
export interface BetaResult {
  /** Input bar of the first value; 0 when there is none. */
  begIdx: number;
  /**
   * Beta: regression slope of inReal1-returns on inReal0-returns. `values[k]` is the
   * value at input bar `begIdx + k`.
   */
  values: Float64Array;
}

/**
 * Number of leading input bars {@link beta} consumes before its first value: the
 * index of the first bar with a value when the whole series is requested.
 *
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link BetaOptions}.
 * @returns The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of
 *   range.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function betaLookback(options?: BetaOptions): number {
  const o = optionsOf('BETA lookback', options);
  return betaLookbackPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), coreOf('BETA lookback', o.core));
}

/**
 * How many bars ahead (positive) or behind (negative) of the bar that computed it
 * a chart draws one output of {@link beta}.
 *
 * Every output of this function is drawn at its own bar, so the answer is 0.
 *
 * @param outputIdx - Position of the output among the result's outputs, from 0.
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link BetaOptions}.
 * @returns The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a
 *   parameter is out of range or the index names no output.
 * @throws `TALibError` with code `TA_BAD_PARAM` when `options` is not an object or
 *   `core` is not a `Core`.
 */
export function betaDisplayShift(outputIdx: number, options?: BetaOptions): number {
  const o = optionsOf('BETA displayShift', options);
  return betaDisplayShiftPositional(orDefault(o.timePeriod, Core.INTEGER_DEFAULT), outputIdx, coreOf('BETA displayShift', o.core));
}

/**
 * Beta: the slope of a least-squares linear regression of one series' percentage
 * returns (y, from inReal1) against another's (x, from inReal0) over a rolling
 * window. Measures how much a security moves relative to a market index. Beta = 1
 * moves with the index; \< 1 less volatile, > 1 more volatile.
 *
 * Formula and more info at
 * [ta-lib.org/functions/beta](https://ta-lib.org/functions/beta).
 *
 * `values[k]` is the value at input bar `begIdx + k`; every output is exactly as
 * long as the number of values written, and nothing is padded. A range that ends
 * before {@link betaLookback} is a success with empty outputs.
 *
 * @param real0 - Series whose returns are the regression x (market/index)
 * @param real1 - Series whose returns are the regression y (security)
 * @param options - Parameters, the range `startIdx` to `endIdx` (inclusive),
 *   `core` and output buffers, each optional; see {@link BetaOptions}.
 * @returns `begIdx` and the values; see {@link BetaResult}. Each output is a view
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
 * import { beta } from 'ta-lib';
 *
 * const data0 = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const data1 = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i + 0.7));
 * const { begIdx, values } = beta(data0, data1, { timePeriod: 5 });
 * // values[k] is the value at bar begIdx + k
 * ```
 * @see [`correl`](https://ta-lib.org/functions/correl)
 * @see [`linearregSlope`](https://ta-lib.org/functions/linearreg_slope)
 * @see [`var_`](https://ta-lib.org/functions/var)
 * @see [`stddev`](https://ta-lib.org/functions/stddev)
 */
export function beta(real0: ArrayLike<number>, real1: ArrayLike<number>, options?: BetaOptions): BetaResult {
  const o = optionsOf('BETA', options);
  if (o.endIdx === undefined) {
    requireSeries('BETA', 'real0', real0);
    requireSeries('BETA', 'real1', real1);
  }
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('BETA', o.core);
  const startIdx = orDefault(o.startIdx, 0);
  const endIdx = o.endIdx === undefined ? real0.length - 1 : o.endIdx;
  const capacity = batchCapacity(startIdx, endIdx, betaLookbackPositional(optInTimePeriod, core), real0, real1);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const range = betaPositional(startIdx, endIdx, real0, real1, optInTimePeriod, outReal, core);
  return { begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}

/**** Streaming API *****/

/**
 * A live BETA stream: one value per closed bar, bit-identical to {@link beta} over
 * the same series. Open it with {@link betaOpen} or {@link betaOpenAndFill}; there
 * is nothing to close.
 *
 * Formula and more info at
 * [ta-lib.org/functions/beta](https://ta-lib.org/functions/beta).
 */
export class BetaStream {
  /** @internal */
  core: Core;
  /** @internal */
  optInTimePeriod: number = 0;
  /** @internal */
  S_xx: number = 0;
  /** @internal */
  S_xy: number = 0;
  /** @internal */
  S_x: number = 0;
  /** @internal */
  S_y: number = 0;
  /** @internal */
  last_price_x: number = 0;
  /** @internal */
  last_price_y: number = 0;
  /** @internal */
  trailing_last_price_x: number = 0;
  /** @internal */
  trailing_last_price_y: number = 0;
  /** @internal */
  shift_x: number = 0;
  /** @internal */
  shift_y: number = 0;
  /** @internal */
  leaving_xx: number = 0;
  /** @internal */
  leaving_yy: number = 0;
  /** @internal */
  S_yy: number = 0;
  /** @internal */
  barsSinceReseed: number = 0;
  /** @internal */
  n: number = 0;
  /** @internal */
  trailingIdx: number = 0;
  /** @internal */
  j: number = 0;
  /** @internal */
  i: number = 0;
  /** @internal */
  xMask: number = 0;
  /** @internal */
  x_inReal0: Float64Array = new Float64Array(1);
  /** @internal */
  x_inReal1: Float64Array = new Float64Array(1);
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
  static of(core: Core): BetaStream {
    return new BetaStream(core);
  }

  /**
   * The bars this stream has an output for, `[begIdx, begIdx + count)`, as
   * {@link beta} reports them over the same bars. Each accepted `update` and each
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
      throw failure('BETA advance', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    this.outRangeCount++;
  }

  /**
   * Commits one closed bar and returns the new value.
   *
   * @param inReal0 - The bar's `inReal0` value: Series whose returns are the
   *   regression x (market/index)
   * @param inReal1 - The bar's `inReal1` value: Series whose returns are the
   *   regression y (security)
   * @returns The new value.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite, before anything moves. `TALibError` with code
   *   `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.
   */
  update(inReal0: number, inReal1: number): number {
    if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {
      throw failure('BETA update', RetCode.OUT_OF_RANGE_END_INDEX);
    }
    if (!Number.isFinite(inReal0) || !Number.isFinite(inReal1)) {
      throw nonFiniteBar('BETA update', !Number.isFinite(inReal0) ? 'inReal0' : 'inReal1');
    }
    betaStepImpl(this, inReal0, inReal1);
    this.outRangeCount++;
    return this.cur_outReal;
  }

  /**
   * What the next `update` with the same bar would return, committing nothing.
   *
   * @param inReal0 - The bar's `inReal0` value: Series whose returns are the
   *   regression x (market/index)
   * @param inReal1 - The bar's `inReal1` value: Series whose returns are the
   *   regression y (security)
   * @returns The value `update` would return.
   * @throws `TALibError` with code `TA_BAD_PARAM` for a bar value that is not
   *   finite.
   */
  peek(inReal0: number, inReal1: number): number {
    if (!Number.isFinite(inReal0) || !Number.isFinite(inReal1)) {
      throw nonFiniteBar('BETA peek', !Number.isFinite(inReal0) ? 'inReal0' : 'inReal1');
    }
    const sp = this;
    let tmp_real = 0;
    let denom = 0;
    let denom_scale = 0;
    let prev_x = 0;
    let prev_y = 0;
    let windowStart = 0;
    let x = 0;
    let y = 0;
    let S_x = sp.S_x;
    let S_xx = sp.S_xx;
    let S_xy = sp.S_xy;
    let S_y = sp.S_y;
    let S_yy = sp.S_yy;
    let barsSinceReseed = sp.barsSinceReseed;
    let cur_outReal = 0;
    let i = sp.i;
    let j = sp.j;
    let last_price_x = sp.last_price_x;
    let last_price_y = sp.last_price_y;
    let shift_x = sp.shift_x;
    let shift_y = sp.shift_y;
    let trailingIdx = sp.trailingIdx;
    let trailing_last_price_x = sp.trailing_last_price_x;
    let trailing_last_price_y = sp.trailing_last_price_y;
    let pkSlot0 = -1;
    let pkVal0 = 0;
    let pkSlot1 = -1;
    let pkVal1 = 0;
    let pkIdx0 = 0;
    pkSlot0 = i & sp.xMask;
    pkVal0 = inReal0;
    pkSlot1 = i & sp.xMask;
    pkVal1 = inReal1;
    tmp_real = ((i & sp.xMask) !== pkSlot0) ? sp.x_inReal0[i & sp.xMask] : pkVal0;
    if (last_price_x !== 0.0) {
      x = (tmp_real - last_price_x) / last_price_x - shift_x;
    } else {
      x = -shift_x;
    }
    last_price_x = tmp_real;
    pkIdx0 = i++ & sp.xMask;
    tmp_real = (pkIdx0 !== pkSlot1) ? sp.x_inReal1[pkIdx0] : pkVal1;
    if (last_price_y !== 0.0) {
      y = (tmp_real - last_price_y) / last_price_y - shift_y;
    } else {
      y = -shift_y;
    }
    last_price_y = tmp_real;
    S_xx += x * x;
    S_yy += y * y;
    S_xy += x * y;
    S_x += x;
    S_y += y;
    denom_scale = sp.n * S_xx;
    denom = denom_scale - S_x * S_x;
    /* Re-anchor and rebuild when the shift has gone stale. Three triggers:
     * the denominator has shrunk below 1e-6 of the scale
     * it is extracted from; OR the return that just left sat so far from the
     * shift that its squared term dwarfs what remains; OR at least every 32
     * windows.
     *
     * The outlier trigger earns its multiply and compare here, contrary to
     * what "returns are stationary" suggests: a bad tick makes one return
     * enormous, the ordinary ones fall below its ulp and are never really
     * added, and when it leaves the subtraction takes back a term they were
     * never part of. The residue is a consistent OFFSET, so the cancellation
     * trigger above cannot see it -- denom/denom_scale stays ~1 -- and only
     * the periodic re-anchor recovers, up to 32*period bars later. Measured
     * without it: a 1e8 tick left 286 of 386 bars wrong, the worst by 0.36
     * ABSOLUTE. Cost is ~3% and mostly unmeasurable on the bench corpus
     * (randwalk/GBM/trend-chop), where it fires on 0.00% of bars -- but that
     * is a corpus figure, not a bound. Isolated against the same body without
     * the disjunct it is +16-20% on a stale-quote/illiquid series (1.5% fire
     * rate) and +54-64% on constructed near-flat or gapped shapes (5.1%).
     * The cost is the reseed it triggers, so it tracks the fire rate; on data
     * that never triggers it, the compare is free.
     *
     * BOTH axes are watched, and the y one is not redundant. The denominator
     * is x-only, so it is tempting to conclude -- as an earlier draft of this
     * did -- that a y trigger catches nothing. It catches plenty: the OUTPUT
     * also reads S_xy and S_y, which a y-side outlier corrupts with nothing on
     * the x side able to see it. Measured on test_beta_outlier_transit's own
     * ladder with the spike moved from px to py: 12 of 24 rungs fail without
     * the second disjunct, worst 156x relative; 0 of 24 with it. The earlier
     * experiment that found it inert was run on an x-only corpus, where it is
     * inert by construction. TA_CORREL, fixed by the same #242 work, watches
     * both from the start; this brings BETA level. S_yy exists only to scale
     * this test -- nothing else reads it.
     *
     * The threshold is 1e3, not the 1e6 a price-scale series takes, because a
     * return amplifies: a tick multiplying the price by k puts k-1 into the
     * return and (k-1)^2 into S_xx, so the ratio when that term leaves lands
     * an order or two below the value-scale case. At 1e6 a 1e5 tick slips
     * through and leaves a flat 2.5e-5 relative error on 285 of 386 bars.
     * Pinned by test_beta_outlier_transit.
     *
     * Reading the window here is safe when outReal aliases an input: the
     * outputs written so far occupy [0, outIdx-1] while windowStart-1 is
     * startIdx-optInTimePeriod+outIdx, which is >= outIdx.
     */
    barsSinceReseed -= 1;
    if (denom < 1e-6 * denom_scale || sp.leaving_xx > 1000.0 * S_xx || sp.leaving_yy > 1000.0 * S_yy || barsSinceReseed <= 0) {
      barsSinceReseed = ((32 * sp.optInTimePeriod) | 0);
      windowStart = trailingIdx;
      /* Walk the window forward from the price the trailing cursor already
       * carries. A return needs its predecessor, and reading inReal[j-1]
       * would reach one slot BEFORE the window -- which the batch can do and
       * a streaming ring sized for the window cannot. trailing_last_price_*
       * IS that predecessor, so carrying it forward keeps every read inside
       * [trailingIdx, i-1] and the two paths stay identical.
       */
      prev_x = trailing_last_price_x;
      prev_y = trailing_last_price_y;
      tmp_real = 0.0;
      shift_y = 0.0;
      for (j = windowStart; j < i; j += 1) {
        if (prev_x !== 0.0) {
          tmp_real += ((((j & sp.xMask) !== pkSlot0) ? sp.x_inReal0[j & sp.xMask] : pkVal0) - prev_x) / prev_x;
        }
        prev_x = ((j & sp.xMask) !== pkSlot0) ? sp.x_inReal0[j & sp.xMask] : pkVal0;
        if (prev_y !== 0.0) {
          shift_y += ((((j & sp.xMask) !== pkSlot1) ? sp.x_inReal1[j & sp.xMask] : pkVal1) - prev_y) / prev_y;
        }
        prev_y = ((j & sp.xMask) !== pkSlot1) ? sp.x_inReal1[j & sp.xMask] : pkVal1;
      }
      shift_x = tmp_real / sp.n;
      shift_y = shift_y / sp.n;
      prev_x = trailing_last_price_x;
      prev_y = trailing_last_price_y;
      S_xx = 0.0;
      S_yy = 0.0;
      S_xy = 0.0;
      S_x = 0.0;
      S_y = 0.0;
      for (j = windowStart; j < i; j += 1) {
        if (prev_x !== 0.0) {
          x = ((((j & sp.xMask) !== pkSlot0) ? sp.x_inReal0[j & sp.xMask] : pkVal0) - prev_x) / prev_x - shift_x;
        } else {
          x = -shift_x;
        }
        prev_x = ((j & sp.xMask) !== pkSlot0) ? sp.x_inReal0[j & sp.xMask] : pkVal0;
        if (prev_y !== 0.0) {
          y = ((((j & sp.xMask) !== pkSlot1) ? sp.x_inReal1[j & sp.xMask] : pkVal1) - prev_y) / prev_y - shift_y;
        } else {
          y = -shift_y;
        }
        prev_y = ((j & sp.xMask) !== pkSlot1) ? sp.x_inReal1[j & sp.xMask] : pkVal1;
        S_xx += x * x;
        S_yy += y * y;
        S_xy += x * y;
        S_x += x;
        S_y += y;
      }
      denom_scale = sp.n * S_xx;
      denom = denom_scale - S_x * S_x;
      /* n*S_xx - S_x*S_x is non-negative by Cauchy-Schwarz, but it is
       * extracted as a difference, so its SIGN is not guaranteed on a window
       * whose returns are all the same value. Enforce the invariant HERE and
       * not at the divide: a negative denom always reseeds on the same bar
       * (it makes the first trigger true whenever denom_scale is positive,
       * and denom_scale == 0 reduces that trigger to `denom < 0`), so the
       * divide below can rely on it being >= 0.
       */
      if (denom < 0.0) {
        denom = 0.0;
      }
    }
    /* Always read the trailing before writing the output because the input and output
     * buffer can be the same.
     */
    tmp_real = ((trailingIdx & sp.xMask) !== pkSlot0) ? sp.x_inReal0[trailingIdx & sp.xMask] : pkVal0;
    if (trailing_last_price_x !== 0.0) {
      x = (tmp_real - trailing_last_price_x) / trailing_last_price_x - shift_x;
    } else {
      x = -shift_x;
    }
    trailing_last_price_x = tmp_real;
    tmp_real = ((trailingIdx & sp.xMask) !== pkSlot1) ? sp.x_inReal1[trailingIdx & sp.xMask] : pkVal1;
    trailingIdx += 1;
    if (trailing_last_price_y !== 0.0) {
      y = (tmp_real - trailing_last_price_y) / trailing_last_price_y - shift_y;
    } else {
      y = -shift_y;
    }
    trailing_last_price_y = tmp_real;
    /* Write the output.
     *
     * The denominator is tested against ITS OWN scale, not a fixed band: it
     * is quadratic in the return volatility, so an absolute 1e-14 threshold
     * stops meaning "the regressor does not vary" and starts meaning "the
     * returns are small". The literal is TA_EPSILON, and the plain `>` also
     * rejects a negative denominator rather than dividing by it.
     */
    if (denom > 1e-14 * denom_scale) {
      cur_outReal = (sp.n * S_xy - S_x * S_y) / denom;
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
  clone(): BetaStream {
    const c = new BetaStream(this.core);
    c.optInTimePeriod = this.optInTimePeriod;
    c.S_xx = this.S_xx;
    c.S_xy = this.S_xy;
    c.S_x = this.S_x;
    c.S_y = this.S_y;
    c.last_price_x = this.last_price_x;
    c.last_price_y = this.last_price_y;
    c.trailing_last_price_x = this.trailing_last_price_x;
    c.trailing_last_price_y = this.trailing_last_price_y;
    c.shift_x = this.shift_x;
    c.shift_y = this.shift_y;
    c.leaving_xx = this.leaving_xx;
    c.leaving_yy = this.leaving_yy;
    c.S_yy = this.S_yy;
    c.barsSinceReseed = this.barsSinceReseed;
    c.n = this.n;
    c.trailingIdx = this.trailingIdx;
    c.j = this.j;
    c.i = this.i;
    c.xMask = this.xMask;
    c.x_inReal0 = this.x_inReal0.slice();
    c.x_inReal1 = this.x_inReal1.slice();
    c.cur_outReal = this.cur_outReal;
    c.outRangeBegIdx = this.outRangeBegIdx;
    c.outRangeCount = this.outRangeCount;
    return c;
  }
}

function betaStepImpl(sp: BetaStream, inReal0: number, inReal1: number): void {
  let tmp_real = 0;
  let denom = 0;
  let denom_scale = 0;
  let prev_x = 0;
  let prev_y = 0;
  let windowStart = 0;
  let x = 0;
  let y = 0;
  sp.x_inReal0[sp.i & sp.xMask] = inReal0;
  sp.x_inReal1[sp.i & sp.xMask] = inReal1;
  tmp_real = sp.x_inReal0[sp.i & sp.xMask];
  if (sp.last_price_x !== 0.0) {
    x = (tmp_real - sp.last_price_x) / sp.last_price_x - sp.shift_x;
  } else {
    x = -sp.shift_x;
  }
  sp.last_price_x = tmp_real;
  tmp_real = sp.x_inReal1[sp.i++ & sp.xMask];
  if (sp.last_price_y !== 0.0) {
    y = (tmp_real - sp.last_price_y) / sp.last_price_y - sp.shift_y;
  } else {
    y = -sp.shift_y;
  }
  sp.last_price_y = tmp_real;
  sp.S_xx += x * x;
  sp.S_yy += y * y;
  sp.S_xy += x * y;
  sp.S_x += x;
  sp.S_y += y;
  denom_scale = sp.n * sp.S_xx;
  denom = denom_scale - sp.S_x * sp.S_x;
  /* Re-anchor and rebuild when the shift has gone stale. Three triggers:
   * the denominator has shrunk below 1e-6 of the scale
   * it is extracted from; OR the return that just left sat so far from the
   * shift that its squared term dwarfs what remains; OR at least every 32
   * windows.
   *
   * The outlier trigger earns its multiply and compare here, contrary to
   * what "returns are stationary" suggests: a bad tick makes one return
   * enormous, the ordinary ones fall below its ulp and are never really
   * added, and when it leaves the subtraction takes back a term they were
   * never part of. The residue is a consistent OFFSET, so the cancellation
   * trigger above cannot see it -- denom/denom_scale stays ~1 -- and only
   * the periodic re-anchor recovers, up to 32*period bars later. Measured
   * without it: a 1e8 tick left 286 of 386 bars wrong, the worst by 0.36
   * ABSOLUTE. Cost is ~3% and mostly unmeasurable on the bench corpus
   * (randwalk/GBM/trend-chop), where it fires on 0.00% of bars -- but that
   * is a corpus figure, not a bound. Isolated against the same body without
   * the disjunct it is +16-20% on a stale-quote/illiquid series (1.5% fire
   * rate) and +54-64% on constructed near-flat or gapped shapes (5.1%).
   * The cost is the reseed it triggers, so it tracks the fire rate; on data
   * that never triggers it, the compare is free.
   *
   * BOTH axes are watched, and the y one is not redundant. The denominator
   * is x-only, so it is tempting to conclude -- as an earlier draft of this
   * did -- that a y trigger catches nothing. It catches plenty: the OUTPUT
   * also reads S_xy and S_y, which a y-side outlier corrupts with nothing on
   * the x side able to see it. Measured on test_beta_outlier_transit's own
   * ladder with the spike moved from px to py: 12 of 24 rungs fail without
   * the second disjunct, worst 156x relative; 0 of 24 with it. The earlier
   * experiment that found it inert was run on an x-only corpus, where it is
   * inert by construction. TA_CORREL, fixed by the same #242 work, watches
   * both from the start; this brings BETA level. S_yy exists only to scale
   * this test -- nothing else reads it.
   *
   * The threshold is 1e3, not the 1e6 a price-scale series takes, because a
   * return amplifies: a tick multiplying the price by k puts k-1 into the
   * return and (k-1)^2 into S_xx, so the ratio when that term leaves lands
   * an order or two below the value-scale case. At 1e6 a 1e5 tick slips
   * through and leaves a flat 2.5e-5 relative error on 285 of 386 bars.
   * Pinned by test_beta_outlier_transit.
   *
   * Reading the window here is safe when outReal aliases an input: the
   * outputs written so far occupy [0, outIdx-1] while windowStart-1 is
   * startIdx-optInTimePeriod+outIdx, which is >= outIdx.
   */
  sp.barsSinceReseed -= 1;
  if (denom < 1e-6 * denom_scale || sp.leaving_xx > 1000.0 * sp.S_xx || sp.leaving_yy > 1000.0 * sp.S_yy || sp.barsSinceReseed <= 0) {
    sp.barsSinceReseed = ((32 * sp.optInTimePeriod) | 0);
    windowStart = sp.trailingIdx;
    /* Walk the window forward from the price the trailing cursor already
     * carries. A return needs its predecessor, and reading inReal[j-1]
     * would reach one slot BEFORE the window -- which the batch can do and
     * a streaming ring sized for the window cannot. trailing_last_price_*
     * IS that predecessor, so carrying it forward keeps every read inside
     * [trailingIdx, i-1] and the two paths stay identical.
     */
    prev_x = sp.trailing_last_price_x;
    prev_y = sp.trailing_last_price_y;
    tmp_real = 0.0;
    sp.shift_y = 0.0;
    for (sp.j = windowStart; sp.j < sp.i; sp.j += 1) {
      if (prev_x !== 0.0) {
        tmp_real += (sp.x_inReal0[sp.j & sp.xMask] - prev_x) / prev_x;
      }
      prev_x = sp.x_inReal0[sp.j & sp.xMask];
      if (prev_y !== 0.0) {
        sp.shift_y += (sp.x_inReal1[sp.j & sp.xMask] - prev_y) / prev_y;
      }
      prev_y = sp.x_inReal1[sp.j & sp.xMask];
    }
    sp.shift_x = tmp_real / sp.n;
    sp.shift_y = sp.shift_y / sp.n;
    prev_x = sp.trailing_last_price_x;
    prev_y = sp.trailing_last_price_y;
    sp.S_xx = 0.0;
    sp.S_yy = 0.0;
    sp.S_xy = 0.0;
    sp.S_x = 0.0;
    sp.S_y = 0.0;
    for (sp.j = windowStart; sp.j < sp.i; sp.j += 1) {
      if (prev_x !== 0.0) {
        x = (sp.x_inReal0[sp.j & sp.xMask] - prev_x) / prev_x - sp.shift_x;
      } else {
        x = -sp.shift_x;
      }
      prev_x = sp.x_inReal0[sp.j & sp.xMask];
      if (prev_y !== 0.0) {
        y = (sp.x_inReal1[sp.j & sp.xMask] - prev_y) / prev_y - sp.shift_y;
      } else {
        y = -sp.shift_y;
      }
      prev_y = sp.x_inReal1[sp.j & sp.xMask];
      sp.S_xx += x * x;
      sp.S_yy += y * y;
      sp.S_xy += x * y;
      sp.S_x += x;
      sp.S_y += y;
    }
    denom_scale = sp.n * sp.S_xx;
    denom = denom_scale - sp.S_x * sp.S_x;
    /* n*S_xx - S_x*S_x is non-negative by Cauchy-Schwarz, but it is
     * extracted as a difference, so its SIGN is not guaranteed on a window
     * whose returns are all the same value. Enforce the invariant HERE and
     * not at the divide: a negative denom always reseeds on the same bar
     * (it makes the first trigger true whenever denom_scale is positive,
     * and denom_scale == 0 reduces that trigger to `denom < 0`), so the
     * divide below can rely on it being >= 0.
     */
    if (denom < 0.0) {
      denom = 0.0;
    }
  }
  /* Always read the trailing before writing the output because the input and output
   * buffer can be the same.
   */
  tmp_real = sp.x_inReal0[sp.trailingIdx & sp.xMask];
  if (sp.trailing_last_price_x !== 0.0) {
    x = (tmp_real - sp.trailing_last_price_x) / sp.trailing_last_price_x - sp.shift_x;
  } else {
    x = -sp.shift_x;
  }
  sp.trailing_last_price_x = tmp_real;
  tmp_real = sp.x_inReal1[sp.trailingIdx & sp.xMask];
  sp.trailingIdx += 1;
  if (sp.trailing_last_price_y !== 0.0) {
    y = (tmp_real - sp.trailing_last_price_y) / sp.trailing_last_price_y - sp.shift_y;
  } else {
    y = -sp.shift_y;
  }
  sp.trailing_last_price_y = tmp_real;
  /* Write the output.
   *
   * The denominator is tested against ITS OWN scale, not a fixed band: it
   * is quadratic in the return volatility, so an absolute 1e-14 threshold
   * stops meaning "the regressor does not vary" and starts meaning "the
   * returns are small". The literal is TA_EPSILON, and the plain `>` also
   * rejects a negative denominator rather than dividing by it.
   */
  if (denom > 1e-14 * denom_scale) {
    sp.cur_outReal = (sp.n * sp.S_xy - sp.S_x * sp.S_y) / denom;
  } else {
    sp.cur_outReal = 0.0;
  }
  /* Remove the calculation starting with the trailingIdx. */
  sp.leaving_xx = x * x;
  sp.leaving_yy = y * y;
  sp.S_xx -= x * x;
  sp.S_yy -= y * y;
  sp.S_xy -= x * y;
  sp.S_x -= x;
  sp.S_y -= y;
}

function betaOpenImpl(
  sp: BetaStream,
  inReal0: Float64Array,
  inReal1: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
  outStride: number,
): RetCode {
  let S_xx = 0;
  let S_xy = 0;
  let S_x = 0;
  let S_y = 0;
  let last_price_x = 0;
  let last_price_y = 0;
  let trailing_last_price_x = 0;
  let trailing_last_price_y = 0;
  let tmp_real = 0;
  let shift_x = 0;
  let shift_y = 0;
  let denom = 0;
  let denom_scale = 0;
  let prev_x = 0;
  let leaving_xx = 0;
  let leaving_yy = 0;
  let S_yy = 0;
  let prev_y = 0;
  let j = 0;
  let windowStart = 0;
  let barsSinceReseed = 0;
  let x = 0;
  let y = 0;
  let n = 0;
  let i = 0;
  let outIdx = 0;
  let trailingIdx = 0;
  let nbInitialElementNeeded = 0;
  const historyLen = inReal0.length;
  let endIdx = historyLen - 1;
  if (historyLen > Core.INDEX_MAX + 1) {
    return RetCode.OUT_OF_RANGE_END_INDEX;
  }
  if (inReal1.length !== inReal0.length) {
    return RetCode.BAD_PARAM;
  }
  if (optInTimePeriod === Core.INTEGER_DEFAULT) {
    optInTimePeriod = 5;
  } else if (optInTimePeriod < 1 || optInTimePeriod > 100000) {
    return RetCode.BAD_PARAM;
  }
  if (startIdx >= historyLen) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  S_xx = 0.0;
  S_xy = 0.0;
  S_x = 0.0;
  S_y = 0.0;
  last_price_x = 0.0;
  last_price_y = 0.0;
  trailing_last_price_x = 0.0;
  trailing_last_price_y = 0.0;
  tmp_real = 0.0;
  shift_x = 0.0;
  shift_y = 0.0;
  denom = 0.0;
  denom_scale = 0.0;
  prev_x = 0.0;
  leaving_xx = 0.0;
  leaving_yy = 0.0;
  S_yy = 0.0;
  prev_y = 0.0;
  n = 0.0;
  /* sum of x * x */
  /* sum of x * y */
  /* sum of x */
  /* sum of y */
  /* the last price read from inReal0 */
  /* the last price read from inReal1 */
  /* same as last_price_x except used to remove elements from the trailing summation */
  /* same as last_price_y except used to remove elements from the trailing summation */
  /* temporary variable */
  /* origin the x returns are measured against */
  /* origin the y returns are measured against */
  /* n*S_xx - S_x*S_x, the regression denominator */
  /* n*S_xx, the scale denom is extracted from */
  /* price walked forward when rebuilding the window */
  /* squared x deviation the previous bar removed */
  /* squared y deviation the previous bar removed */
  /* sum of y * y, carried ONLY for the outlier trigger */
  /* the 'x' value, which is the last change between values in inReal0 */
  /* the 'y' value, which is the last change between values in inReal1 */
  /* DESCRIPTION OF ALGORITHM:
   *   The Beta 'algorithm' is a measure of a stocks volatility vs from index. The index prices
   *   are given in inReal0 and the stock prices are given in inReal1. The size of these vectors
   *   should be equal. The algorithm is to calculate the change between prices in both vectors
   *   and then 'plot' these changes are points in the Euclidean plane. The x value of the point
   *   is market return and the y value is the security return. The beta value is the slope of a
   *   linear regression through these points. A beta of 1 is simple the line y=x, so the stock
   *   varies percisely with the market. A beta of less than one means the stock varies less than
   *   the market and a beta of more than one means the stock varies more than market. A related
   *   value is the Alpha value (see TA_ALPHA) which is the Y-intercept of the same linear regression.
   */
  /* Validate the calculation method type and
   * identify the minimum number of input
   * consume before the first value is output..
   */
  nbInitialElementNeeded = optInTimePeriod;
  /* Move up the start index if there is not
   * enough initial data.
   */
  if (startIdx < nbInitialElementNeeded) {
    startIdx = nbInitialElementNeeded;
  }
  /* Make sure there is still something to evaluate. */
  if (startIdx > endIdx) {
    outBegIdx.value = 0;
    outNBElement.value = 0;
    return RetCode.INSUFFICIENT_HISTORY;
  }
  /* Consume first input. */
  trailingIdx = startIdx - nbInitialElementNeeded;
  trailing_last_price_x = inReal0[trailingIdx];
  last_price_x = trailing_last_price_x;
  trailing_last_price_y = inReal1[trailingIdx];
  last_price_y = trailing_last_price_y;
  /* Measure the returns against a shift near the window, as TA_VAR does for
   * its values (#118) and TA_CORREL for its prices (#242).
   *
   * Returns are near-zero-mean, so on a series that jitters this changes
   * little. It is decisive on a series that DRIFTS: a steadily rising price
   * gives near-identical returns, S_x*S_x then equals n*S_xx to every digit,
   * and the denominator n*S_xx - S_x*S_x is left as pure rounding noise --
   * measured at +/-2e-16 of its own scale, sign included. Wilkinson's BIG and
   * LITTLE are exactly that shape, and beta of such a series against itself
   * came back 0 instead of 1.
   *
   * Anchor on the window's first return here; every later re-anchor uses the
   * window mean, which is better centred but costs a pass this one cannot
   * afford before the sums exist.
   */
  i = ++trailingIdx;
  /* A return needs a non-zero base price and nothing more: the test is exact,
   * not the fixed TA_IS_ZERO band it used to be. A price carries the quote
   * unit, so that band declared every bar of a small-quoted instrument
   * "no previous price", left every return at -shift, and collapsed beta to
   * zero (issue #253).
   */
  if (last_price_x !== 0.0) {
    shift_x = (inReal0[i] - last_price_x) / last_price_x;
  }
  if (last_price_y !== 0.0) {
    shift_y = (inReal1[i] - last_price_y) / last_price_y;
  }
  while (i < startIdx) {
    tmp_real = inReal0[i];
    if (last_price_x !== 0.0) {
      x = (tmp_real - last_price_x) / last_price_x - shift_x;
    } else {
      x = -shift_x;
    }
    last_price_x = tmp_real;
    tmp_real = inReal1[i++];
    if (last_price_y !== 0.0) {
      y = (tmp_real - last_price_y) / last_price_y - shift_y;
    } else {
      y = -shift_y;
    }
    last_price_y = tmp_real;
    S_xx += x * x;
    S_yy += y * y;
    S_xy += x * y;
    S_x += x;
    S_y += y;
  }
  outIdx = 0;
  /* First output always start at index zero */
  n = optInTimePeriod;
  barsSinceReseed = ((32 * optInTimePeriod) | 0);
  do {
    tmp_real = inReal0[i];
    if (last_price_x !== 0.0) {
      x = (tmp_real - last_price_x) / last_price_x - shift_x;
    } else {
      x = -shift_x;
    }
    last_price_x = tmp_real;
    tmp_real = inReal1[i++];
    if (last_price_y !== 0.0) {
      y = (tmp_real - last_price_y) / last_price_y - shift_y;
    } else {
      y = -shift_y;
    }
    last_price_y = tmp_real;
    S_xx += x * x;
    S_yy += y * y;
    S_xy += x * y;
    S_x += x;
    S_y += y;
    denom_scale = n * S_xx;
    denom = denom_scale - S_x * S_x;
    /* Re-anchor and rebuild when the shift has gone stale. Three triggers:
     * the denominator has shrunk below 1e-6 of the scale
     * it is extracted from; OR the return that just left sat so far from the
     * shift that its squared term dwarfs what remains; OR at least every 32
     * windows.
     *
     * The outlier trigger earns its multiply and compare here, contrary to
     * what "returns are stationary" suggests: a bad tick makes one return
     * enormous, the ordinary ones fall below its ulp and are never really
     * added, and when it leaves the subtraction takes back a term they were
     * never part of. The residue is a consistent OFFSET, so the cancellation
     * trigger above cannot see it -- denom/denom_scale stays ~1 -- and only
     * the periodic re-anchor recovers, up to 32*period bars later. Measured
     * without it: a 1e8 tick left 286 of 386 bars wrong, the worst by 0.36
     * ABSOLUTE. Cost is ~3% and mostly unmeasurable on the bench corpus
     * (randwalk/GBM/trend-chop), where it fires on 0.00% of bars -- but that
     * is a corpus figure, not a bound. Isolated against the same body without
     * the disjunct it is +16-20% on a stale-quote/illiquid series (1.5% fire
     * rate) and +54-64% on constructed near-flat or gapped shapes (5.1%).
     * The cost is the reseed it triggers, so it tracks the fire rate; on data
     * that never triggers it, the compare is free.
     *
     * BOTH axes are watched, and the y one is not redundant. The denominator
     * is x-only, so it is tempting to conclude -- as an earlier draft of this
     * did -- that a y trigger catches nothing. It catches plenty: the OUTPUT
     * also reads S_xy and S_y, which a y-side outlier corrupts with nothing on
     * the x side able to see it. Measured on test_beta_outlier_transit's own
     * ladder with the spike moved from px to py: 12 of 24 rungs fail without
     * the second disjunct, worst 156x relative; 0 of 24 with it. The earlier
     * experiment that found it inert was run on an x-only corpus, where it is
     * inert by construction. TA_CORREL, fixed by the same #242 work, watches
     * both from the start; this brings BETA level. S_yy exists only to scale
     * this test -- nothing else reads it.
     *
     * The threshold is 1e3, not the 1e6 a price-scale series takes, because a
     * return amplifies: a tick multiplying the price by k puts k-1 into the
     * return and (k-1)^2 into S_xx, so the ratio when that term leaves lands
     * an order or two below the value-scale case. At 1e6 a 1e5 tick slips
     * through and leaves a flat 2.5e-5 relative error on 285 of 386 bars.
     * Pinned by test_beta_outlier_transit.
     *
     * Reading the window here is safe when outReal aliases an input: the
     * outputs written so far occupy [0, outIdx-1] while windowStart-1 is
     * startIdx-optInTimePeriod+outIdx, which is >= outIdx.
     */
    barsSinceReseed -= 1;
    if (denom < 1e-6 * denom_scale || leaving_xx > 1000.0 * S_xx || leaving_yy > 1000.0 * S_yy || barsSinceReseed <= 0) {
      barsSinceReseed = ((32 * optInTimePeriod) | 0);
      windowStart = trailingIdx;
      /* Walk the window forward from the price the trailing cursor already
       * carries. A return needs its predecessor, and reading inReal[j-1]
       * would reach one slot BEFORE the window -- which the batch can do and
       * a streaming ring sized for the window cannot. trailing_last_price_*
       * IS that predecessor, so carrying it forward keeps every read inside
       * [trailingIdx, i-1] and the two paths stay identical.
       */
      prev_x = trailing_last_price_x;
      prev_y = trailing_last_price_y;
      tmp_real = 0.0;
      shift_y = 0.0;
      for (j = windowStart; j < i; j += 1) {
        if (prev_x !== 0.0) {
          tmp_real += (inReal0[j] - prev_x) / prev_x;
        }
        prev_x = inReal0[j];
        if (prev_y !== 0.0) {
          shift_y += (inReal1[j] - prev_y) / prev_y;
        }
        prev_y = inReal1[j];
      }
      shift_x = tmp_real / n;
      shift_y = shift_y / n;
      prev_x = trailing_last_price_x;
      prev_y = trailing_last_price_y;
      S_xx = 0.0;
      S_yy = 0.0;
      S_xy = 0.0;
      S_x = 0.0;
      S_y = 0.0;
      for (j = windowStart; j < i; j += 1) {
        if (prev_x !== 0.0) {
          x = (inReal0[j] - prev_x) / prev_x - shift_x;
        } else {
          x = -shift_x;
        }
        prev_x = inReal0[j];
        if (prev_y !== 0.0) {
          y = (inReal1[j] - prev_y) / prev_y - shift_y;
        } else {
          y = -shift_y;
        }
        prev_y = inReal1[j];
        S_xx += x * x;
        S_yy += y * y;
        S_xy += x * y;
        S_x += x;
        S_y += y;
      }
      denom_scale = n * S_xx;
      denom = denom_scale - S_x * S_x;
      /* n*S_xx - S_x*S_x is non-negative by Cauchy-Schwarz, but it is
       * extracted as a difference, so its SIGN is not guaranteed on a window
       * whose returns are all the same value. Enforce the invariant HERE and
       * not at the divide: a negative denom always reseeds on the same bar
       * (it makes the first trigger true whenever denom_scale is positive,
       * and denom_scale == 0 reduces that trigger to `denom < 0`), so the
       * divide below can rely on it being >= 0.
       */
      if (denom < 0.0) {
        denom = 0.0;
      }
    }
    /* Always read the trailing before writing the output because the input and output
     * buffer can be the same.
     */
    tmp_real = inReal0[trailingIdx];
    if (trailing_last_price_x !== 0.0) {
      x = (tmp_real - trailing_last_price_x) / trailing_last_price_x - shift_x;
    } else {
      x = -shift_x;
    }
    trailing_last_price_x = tmp_real;
    tmp_real = inReal1[trailingIdx];
    trailingIdx += 1;
    if (trailing_last_price_y !== 0.0) {
      y = (tmp_real - trailing_last_price_y) / trailing_last_price_y - shift_y;
    } else {
      y = -shift_y;
    }
    trailing_last_price_y = tmp_real;
    /* Write the output.
     *
     * The denominator is tested against ITS OWN scale, not a fixed band: it
     * is quadratic in the return volatility, so an absolute 1e-14 threshold
     * stops meaning "the regressor does not vary" and starts meaning "the
     * returns are small". The literal is TA_EPSILON, and the plain `>` also
     * rejects a negative denominator rather than dividing by it.
     */
    if (denom > 1e-14 * denom_scale) {
      outReal[((outIdx++ * outStride) | 0)] = (n * S_xy - S_x * S_y) / denom;
    } else {
      outReal[((outIdx++ * outStride) | 0)] = 0.0;
    }
    /* Remove the calculation starting with the trailingIdx. */
    leaving_xx = x * x;
    leaving_yy = y * y;
    S_xx -= x * x;
    S_yy -= y * y;
    S_xy -= x * y;
    S_x -= x;
    S_y -= y;
  } while (i <= endIdx);
  /* All done. Indicate the output limits and return. */
  outNBElement.value = outIdx;
  outBegIdx.value = startIdx;
  const capX = i - trailingIdx + 1;
  if (capX < 1 || capX > historyLen) {
    return RetCode.INTERNAL_ERROR;
  }
  let physX = 1;
  while (physX < capX) {
    physX <<= 1;
  }
  const capX_inReal0 = new Float64Array(physX);
  const capX_inReal1 = new Float64Array(physX);
  for (let fillJ = historyLen - capX; fillJ < historyLen; fillJ++) {
    capX_inReal0[fillJ & (physX - 1)] = inReal0[fillJ];
    capX_inReal1[fillJ & (physX - 1)] = inReal1[fillJ];
  }
  sp.optInTimePeriod = optInTimePeriod;
  sp.S_xx = S_xx;
  sp.S_xy = S_xy;
  sp.S_x = S_x;
  sp.S_y = S_y;
  sp.last_price_x = last_price_x;
  sp.last_price_y = last_price_y;
  sp.trailing_last_price_x = trailing_last_price_x;
  sp.trailing_last_price_y = trailing_last_price_y;
  sp.shift_x = shift_x;
  sp.shift_y = shift_y;
  sp.leaving_xx = leaving_xx;
  sp.leaving_yy = leaving_yy;
  sp.S_yy = S_yy;
  sp.barsSinceReseed = barsSinceReseed;
  sp.n = n;
  sp.trailingIdx = trailingIdx;
  sp.j = j;
  sp.i = i;
  sp.xMask = physX - 1;
  sp.x_inReal0 = capX_inReal0;
  sp.x_inReal1 = capX_inReal1;
  sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
  return RetCode.SUCCESS;
}

/** @internal */
export function betaOpenAndFillInternal(
  core: Core,
  inReal0: Float64Array,
  inReal1: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
  outBegIdx: MInteger,
  outNBElement: MInteger,
  outReal: Float64Array,
): BetaStream {
  const sp = BetaStream.of(core);
  const retCode = betaOpenImpl(sp, inReal0, inReal1, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('BETA openAndFill', inReal0.length, startIdx, betaLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && betaLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('BETA openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('BETA openAndFill', retCode);
}

/** @internal */
export function betaOpenInternal(
  core: Core,
  inReal0: Float64Array,
  inReal1: Float64Array,
  startIdx: number,
  optInTimePeriod: number,
): BetaStream {
  const sp = BetaStream.of(core);
  const outBegIdx = new MInteger();
  const outNBElement = new MInteger();
  const sink_outReal = new Float64Array(1);
  const retCode = betaOpenImpl(sp, inReal0, inReal1, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
  sp.outRangeBegIdx = outBegIdx.value;
  sp.outRangeCount = outNBElement.value;
  if (retCode === RetCode.SUCCESS) {
    return sp;
  }
  if (retCode === RetCode.INSUFFICIENT_HISTORY) {
    throw insufficientHistory('BETA open', inReal0.length, startIdx, betaLookbackPositional(optInTimePeriod, core));
  }
  if (retCode === RetCode.BAD_PARAM && betaLookbackPositional(optInTimePeriod, core) < 0) {
    throw rejectedParams('BETA open', PARAM_RANGES, optInTimePeriod);
  }
  throw streamFailure('BETA open', retCode);
}

/** @internal */
export function betaOpenPositional(
  inReal0: ArrayLike<number>,
  inReal1: ArrayLike<number>,
  optInTimePeriod: number,
  core: Core = Core.DEFAULT,
): BetaStream {
  optInTimePeriod = requireInt32('BETA open', 'timePeriod', optInTimePeriod);
  requireArgument('BETA open', 'real0', inReal0);
  requireHistory('BETA open', inReal0.length);
  requireArgument('BETA open', 'real1', inReal1);
  requireHistoryLength('BETA open', 'real1', inReal1.length, inReal0.length);
  return betaOpenInternal(core, float64(inReal0), float64(inReal1), 0, optInTimePeriod);
}

/** @internal */
export function betaOpenAndFillPositional(
  inReal0: ArrayLike<number>,
  inReal1: ArrayLike<number>,
  optInTimePeriod: number,
  outReal: Float64Array,
  core: Core = Core.DEFAULT,
): BetaStream {
  optInTimePeriod = requireInt32('BETA openAndFill', 'timePeriod', optInTimePeriod);
  requireArgument('BETA openAndFill', 'real0', inReal0);
  requireHistory('BETA openAndFill', inReal0.length);
  requireArgument('BETA openAndFill', 'real1', inReal1);
  const guardLookback = betaLookbackPositional(optInTimePeriod, core);
  if (guardLookback < 0) {
    throw rejectedParams('BETA openAndFill', PARAM_RANGES, optInTimePeriod);
  }
  const guardOutLen = openFillCount('BETA openAndFill', inReal0.length, guardLookback);
  requireHistoryLength('BETA openAndFill', 'real1', inReal1.length, inReal0.length);
  requireOutput('BETA openAndFill', 'outReal', outReal, guardOutLen, Float64Array);
  const inReal0F64 = float64(inReal0);
  const inReal1F64 = float64(inReal1);
  if (outReal.buffer === inReal0F64.buffer && outReal.byteOffset === inReal0F64.byteOffset) {
    throw sharedMemory('BETA openAndFill', 'outReal', 'real0');
  }
  if (outReal.buffer === inReal1F64.buffer && outReal.byteOffset === inReal1F64.byteOffset) {
    throw sharedMemory('BETA openAndFill', 'outReal', 'real1');
  }
  if (guardOutLen > 0 && overlaps(outReal, inReal0F64)) {
    throw sharedMemory('BETA openAndFill', 'outReal', 'real0');
  }
  if (guardOutLen > 0 && overlaps(outReal, inReal1F64)) {
    throw sharedMemory('BETA openAndFill', 'outReal', 'real1');
  }
  return betaOpenAndFillInternal(core, inReal0F64, inReal1F64, 0, optInTimePeriod, new MInteger(), new MInteger(), outReal);
}

/**
 * Opens a live BETA stream over the warm-up history: `value()` starts at the last
 * history bar, bit-identical to {@link beta} there.
 *
 * @param real0 - Warm-up history. Series whose returns are the regression x
 *   (market/index)
 * @param real1 - Warm-up history. Series whose returns are the regression y
 *   (security)
 * @param options - The parameters and `core`; other keys are ignored. See
 *   {@link BetaOptions}.
 * @returns The stream, positioned on the last history bar.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `betaLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range or
 *   histories of different lengths.
 *
 * @example
 * ```ts
 * import { betaOpen } from 'ta-lib';
 *
 * const data0 = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const data1 = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i + 0.7));
 * const stream = betaOpen(data0, data1, { timePeriod: 5 });
 * const peeked = stream.peek(100.9, 101.3); // a forming bar: commits nothing
 * const updated = stream.update(100.9, 101.3); // the bar closes
 * ```
 */
export function betaOpen(real0: ArrayLike<number>, real1: ArrayLike<number>, options?: BetaOptions): BetaStream {
  const o = optionsOf('BETA open', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('BETA open', o.core);
  return betaOpenPositional(real0, real1, optInTimePeriod, core);
}

/**
 * {@link betaOpen} that also computes the outputs over the whole history in the
 * same pass, bit-identically to {@link beta}.
 *
 * @param real0 - Warm-up history. Series whose returns are the regression x
 *   (market/index)
 * @param real1 - Warm-up history. Series whose returns are the regression y
 *   (security)
 * @param options - The parameters, `core` and output buffers; other keys are
 *   ignored. See {@link BetaOptions}.
 * @returns The stream, positioned on the last history bar, with `begIdx` and the
 *   values over the history as {@link beta} returns them; the range is the stream's
 *   `outRange()`.
 * @throws `TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter
 *   than `betaLookback(...) + 1` bars, an empty one included, and `TA_BAD_PARAM` for
 *   options that are not an object, a `core` that is not a `Core`, an absent input,
 *   an integer parameter that is not an int32, a parameter outside its range,
 *   histories of different lengths or a buffer too short, of the wrong type or
 *   starting where another buffer or a `Float64Array` input starts, or, on a history
 *   long enough to open, one sharing memory with another buffer or a `Float64Array`
 *   input.
 *
 * @example
 * ```ts
 * import { betaOpenAndFill } from 'ta-lib';
 *
 * const data0 = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i));
 * const data1 = Float64Array.from({ length: 252 }, (_, i) =>
 *   100 + 10 * Math.sin(0.1 * i + 0.7));
 * const { stream, begIdx, values } = betaOpenAndFill(data0, data1, {
 *   timePeriod: 5,
 * });
 * // values[k] is the value at bar begIdx + k
 * ```
 */
export function betaOpenAndFill(real0: ArrayLike<number>, real1: ArrayLike<number>, options?: BetaOptions): BetaResult & { stream: BetaStream } {
  const o = optionsOf('BETA openAndFill', options);
  const optInTimePeriod = orDefault(o.timePeriod, Core.INTEGER_DEFAULT);
  const core = coreOf('BETA openAndFill', o.core);
  const capacity = fillCapacity(betaLookbackPositional(optInTimePeriod, core), real0, real1);
  const outReal = o.outReal === undefined ? new Float64Array(capacity) : o.outReal;
  const stream = betaOpenAndFillPositional(real0, real1, optInTimePeriod, outReal, core);
  const range = stream.outRange();
  return { stream, begIdx: range.begIdx, values: outReal.subarray(0, range.count) };
}
