/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  091526 KL     First version (proposal-drafts issue #72).
 */

   /**
    * Number of leading input bars {@link Core#kurtosis} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of trailing values in the window, at least 4
    *        (default 30; range 4..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int kurtosisLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 4 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode kurtosisImpl( int startIdx,
                         int endIdx,
                         double inReal[],
                         int optInTimePeriod,
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         double outReal[] )
   {
      double tempReal = 0;
      double shift = 0;
      double total1 = 0;
      double total2 = 0;
      double total3 = 0;
      double total4 = 0;
      double dev = 0;
      double dev2 = 0;
      double residue = 0;
      double residueSq = 0;
      double moment2 = 0;
      double moment4 = 0;
      double sampleVar = 0;
      double varSquared = 0;
      double dPeriod = 0;
      double coefA = 0;
      double coefB = 0;
      double invPeriod = 0;
      double kurt = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int windowStart = 0;
      int nbInitialElementNeeded = 0;
      int barsSinceReseed = 0;
      int reseedPeriod = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 4 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      nbInitialElementNeeded = optInTimePeriod - 1;
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      /* G2, the sample-adjusted Fisher excess kurtosis. The two coefficients are
       * computed in double because their integer forms overflow: at the top of the
       * parameter range (n-1)(n-2)(n-3) is ~1e15, far past what an int holds, and
       * the product would wrap silently rather than fail.
       *
       * The (n-2)(n-3) denominators are why the range starts at 4 rather than 1.
       * The argument contract rejects anything below it, and the n-1 lookback
       * guarantees a full window at the first emitted bar, so the estimator is
       * never handed fewer than four points and needs no runtime branch for it.
       */
      dPeriod = (double)optInTimePeriod;
      coefA = dPeriod * (dPeriod + 1.0) / ((dPeriod - 1.0) * (dPeriod - 2.0) * (dPeriod - 3.0));
      coefB = 3.0 * (dPeriod - 1.0) * (dPeriod - 1.0) / ((dPeriod - 2.0) * (dPeriod - 3.0));
      invPeriod = 1.0 / dPeriod;
      /* Deviations are measured against a shift near the window, as var.c does, so
       * the running sums stay at deviation scale instead of at price scale. The
       * central moments are then recovered from the shifted sums by the binomial
       * expansion below, which puts back the residue the shift left behind.
       *
       * The RESEED PERIOD IS NOT var.c's. MEASURED on 1200-bar series at n=30,
       * worst relative error against a 60-digit reference computed per window:
       *
       *   rebuild every    32n (var.c)    8n        2n        n         n/4
       *   walk around 100   1.18e-06   9.33e-07  1.61e-08  1.28e-09  4.58e-12
       *   walk on 3.1e10    4.99e-06   4.99e-06  6.27e-10  1.45e-09  1.76e-12
       *   outlier 1e5/200   2.84e-13   2.84e-13  2.68e-13  1.51e-13  4.48e-14
       *
       * A fourth moment recovered against a stale shift pays (u/sigma)^4 where a
       * second pays (u/sigma)^2, so the shift goes stale four times faster in the
       * exponent and var.c's 32n leaves 1e-6 on an ordinary random walk. The
       * collapse trigger cannot catch that case -- the ratio it tests sits around
       * 0.24 there, six orders from firing -- so only the periodic rebuild can,
       * and it has to run at n/4.
       *
       * Rebuilding that often also beats rescanning the window every bar
       * (5.34e-11 and 4.30e-11 on the two walks): the rebuild anchors the shift at
       * the window MEAN, while a per-bar rescan can only anchor it at a window
       * VALUE, which sits further from centre and leaves a larger u. About a tenth
       * of the arithmetic, and more accurate.
       */
      reseedPeriod = optInTimePeriod / 4;
      trailingIdx = startIdx - nbInitialElementNeeded;
      shift = inReal[trailingIdx];
      total1 = 0.0;
      total2 = 0.0;
      total3 = 0.0;
      total4 = 0.0;
      for( j = trailingIdx; j < startIdx; j += 1 ) {
         dev = inReal[j] - shift;
         dev2 = dev * dev;
         total1 += dev;
         total2 += dev2;
         total3 += dev2 * dev;
         total4 += dev2 * dev2;
      }
      /* inReal and outReal may be the same buffer: each trailing value is consumed
       * before its slot is overwritten by the output.
       */
      i = startIdx;
      outIdx = 0;
      barsSinceReseed = reseedPeriod;
      do {
         dev = inReal[i] - shift;
         dev2 = dev * dev;
         total1 += dev;
         total2 += dev2;
         total3 += dev2 * dev;
         total4 += dev2 * dev2;
         /* Central moments from the shifted sums. `residue` is what the shift
          * left behind: ~0 right after a rebuild, growing as the window walks
          * away from the anchor. That growth is the quantity the rebuild period
          * bounds, and the reason a fourth moment needs a shorter period than a
          * second -- it enters here raised to the fourth power.
          */
         residue = total1 * invPeriod;
         residueSq = residue * residue;
         moment2 = total2 - dPeriod * residueSq;
         moment4 = Math.fma(6.0 * residueSq, total2, total4 - 4.0 * residue * total3) - 3.0 * dPeriod * residueSq * residueSq;
         /* Remove the trailing value (prepares the next window). */
         dev = inReal[trailingIdx] - shift;
         dev2 = dev * dev;
         total1 -= dev;
         total2 -= dev2;
         total3 -= dev2 * dev;
         total4 -= dev2 * dev2;
         trailingIdx += 1;
         /* Rebuild when the window walked far enough from the anchor for the
          * expansion above to be subtracting like-sized quantities; when the value
          * just removed sat so far from the shift that its fourth power dwarfs
          * what survives (a large outlier passing through buries the small terms
          * below its ulp, and the residue it leaves is cancellation garbage); or
          * on the period derived above regardless.
          */
         barsSinceReseed -= 1;
         if( moment2 < 0.000001 * total2 || dev2 * dev2 > 1000000.0 * total4 || barsSinceReseed <= 0 ) {
            barsSinceReseed = reseedPeriod;
            windowStart = i - nbInitialElementNeeded;
            tempReal = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               tempReal += inReal[j];
            }
            shift = tempReal * invPeriod;
            total1 = 0.0;
            total2 = 0.0;
            total3 = 0.0;
            total4 = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               dev = inReal[j] - shift;
               dev2 = dev * dev;
               total1 += dev;
               total2 += dev2;
               total3 += dev2 * dev;
               total4 += dev2 * dev2;
            }
            residue = total1 * invPeriod;
            residueSq = residue * residue;
            moment2 = total2 - dPeriod * residueSq;
            moment4 = Math.fma(6.0 * residueSq, total2, total4 - 4.0 * residue * total3) - 3.0 * dPeriod * residueSq * residueSq;
            /* Re-remove the trailing value under the new shift so the carried
             * state matches the non-rebuild path.
             */
            dev = inReal[windowStart] - shift;
            dev2 = dev * dev;
            total1 -= dev;
            total2 -= dev2;
            total3 -= dev2 * dev;
            total4 -= dev2 * dev2;
         }
         /* A window with no spread has no kurtosis to report, and there is no
          * defensible neutral to substitute: 0 asserts normality and -1.2 asserts
          * uniformity, neither of which a point mass supports. scipy returns nan
          * at both bias settings and Excel KURT answers #DIV/0!.
          *
          * So the division is left UNGUARDED, as rvol.c leaves its own, and the
          * function declares nan_inf_output. On a point mass the rebuild anchors
          * the shift at the single value, every deviation is exactly 0, and both
          * moments are exactly 0 -- so this is 0/0 and IEEE answers NaN without a
          * branch. A guard with an absolute epsilon would be a cliff at a price
          * level rather than a noise floor, which is the mistake #243 fixed in the
          * var/stddev/bbands family: a $100 instrument quoted in 1e-8 ticks had
          * every bar zeroed.
          *
          * Nothing NaN re-enters the running sums -- the division happens here, at
          * the output write, on sums that stay finite.
          */
         sampleVar = moment2 / (dPeriod - 1.0);
         varSquared = sampleVar * sampleVar;
         kurt = coefA * (moment4 / varSquared) - coefB;
         outReal[outIdx++] = kurt;
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   RetCode kurtosisImpl( int startIdx,
                         int endIdx,
                         float inReal[],
                         int optInTimePeriod,
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         double outReal[] )
   {
      double tempReal = 0;
      double shift = 0;
      double total1 = 0;
      double total2 = 0;
      double total3 = 0;
      double total4 = 0;
      double dev = 0;
      double dev2 = 0;
      double residue = 0;
      double residueSq = 0;
      double moment2 = 0;
      double moment4 = 0;
      double sampleVar = 0;
      double varSquared = 0;
      double dPeriod = 0;
      double coefA = 0;
      double coefB = 0;
      double invPeriod = 0;
      double kurt = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int windowStart = 0;
      int nbInitialElementNeeded = 0;
      int barsSinceReseed = 0;
      int reseedPeriod = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 4 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      nbInitialElementNeeded = optInTimePeriod - 1;
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      dPeriod = (double)optInTimePeriod;
      coefA = dPeriod * (dPeriod + 1.0) / ((dPeriod - 1.0) * (dPeriod - 2.0) * (dPeriod - 3.0));
      coefB = 3.0 * (dPeriod - 1.0) * (dPeriod - 1.0) / ((dPeriod - 2.0) * (dPeriod - 3.0));
      invPeriod = 1.0 / dPeriod;
      reseedPeriod = optInTimePeriod / 4;
      trailingIdx = startIdx - nbInitialElementNeeded;
      shift = (double)inReal[trailingIdx];
      total1 = 0.0;
      total2 = 0.0;
      total3 = 0.0;
      total4 = 0.0;
      for( j = trailingIdx; j < startIdx; j += 1 ) {
         dev = (double)inReal[j] - shift;
         dev2 = dev * dev;
         total1 += dev;
         total2 += dev2;
         total3 += dev2 * dev;
         total4 += dev2 * dev2;
      }
      i = startIdx;
      outIdx = 0;
      barsSinceReseed = reseedPeriod;
      do {
         dev = (double)inReal[i] - shift;
         dev2 = dev * dev;
         total1 += dev;
         total2 += dev2;
         total3 += dev2 * dev;
         total4 += dev2 * dev2;
         residue = total1 * invPeriod;
         residueSq = residue * residue;
         moment2 = total2 - dPeriod * residueSq;
         moment4 = Math.fma(6.0 * residueSq, total2, total4 - 4.0 * residue * total3) - 3.0 * dPeriod * residueSq * residueSq;
         dev = (double)inReal[trailingIdx] - shift;
         dev2 = dev * dev;
         total1 -= dev;
         total2 -= dev2;
         total3 -= dev2 * dev;
         total4 -= dev2 * dev2;
         trailingIdx += 1;
         barsSinceReseed -= 1;
         if( moment2 < 0.000001 * total2 || dev2 * dev2 > 1000000.0 * total4 || barsSinceReseed <= 0 ) {
            barsSinceReseed = reseedPeriod;
            windowStart = i - nbInitialElementNeeded;
            tempReal = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               tempReal += (double)inReal[j];
            }
            shift = tempReal * invPeriod;
            total1 = 0.0;
            total2 = 0.0;
            total3 = 0.0;
            total4 = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               dev = (double)inReal[j] - shift;
               dev2 = dev * dev;
               total1 += dev;
               total2 += dev2;
               total3 += dev2 * dev;
               total4 += dev2 * dev2;
            }
            residue = total1 * invPeriod;
            residueSq = residue * residue;
            moment2 = total2 - dPeriod * residueSq;
            moment4 = Math.fma(6.0 * residueSq, total2, total4 - 4.0 * residue * total3) - 3.0 * dPeriod * residueSq * residueSq;
            dev = (double)inReal[windowStart] - shift;
            dev2 = dev * dev;
            total1 -= dev;
            total2 -= dev2;
            total3 -= dev2 * dev;
            total4 -= dev2 * dev2;
         }
         sampleVar = moment2 / (dPeriod - 1.0);
         varSquared = sampleVar * sampleVar;
         kurt = coefA * (moment4 / varSquared) - coefB;
         outReal[outIdx++] = kurt;
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * The fourth standardised moment of the trailing window, minus 3 so a normal
    * window reads 0. A tail-weight measure: above 0 the window has fatter tails
    * and a sharper peak than a normal distribution of the same variance, below
    * 0 it is flatter. The companion to the shipped <a
    * href="https://ta-lib.org/functions/var">{@code VAR}</a> and <a
    * href="https://ta-lib.org/functions/stddev">{@code STDDEV}</a> in the same
    * group. {@code SKEW}, the third-moment sibling, is deliberately a separate
    * question — it carries its own convention fork and its own references, and
    * two functions that share only a word get independent docs and decisions.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/kurtosis">ta-lib.org/functions/kurtosis</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li><b>Which estimator.</b> This is {@code G2}, what Excel {@code KURT}, {@code scipy.stats.kurtosis(bias=False)} and R {@code e1071::kurtosis(type=2)} all compute, and the one that is unbiased under normality. The other form in circulation is the biased {@code g2 = m₄/m₂² − 3} over population moments, which NIST/SEMATECH gives. They are not a rounding convention apart: on a 9-point normal sample {@code G2 = 1.79450407519901} against {@code g2 = 0.342114639479481}.</li>
    * <li><b>A window with no spread returns NaN, not a number.</b> Excess kurtosis has no defensible neutral to fall back on — {@code 0} asserts normality and {@code −1.2} asserts uniformity, and a point mass supports neither. {@code scipy.stats.kurtosis} returns {@code nan} at both bias settings, and Excel {@code KURT} documents {@code #DIV/0!} when the sample's standard deviation is zero. This function declares {@code nan_inf_output} and leaves the division unguarded, so the answer arrives from IEEE rather than from a branch. It is deliberately a <i>different</i> answer from {@code VAR}'s floor-to-zero: a variance of zero says something true about the window, a kurtosis of a point mass does not.</li>
    * <li><b>Cancellation, and why the rebuild period is not {@code VAR}'s.</b> Deviations are taken against a shift near the window and the central moments recovered from the shifted sums, as {@code var.c} does. What does not carry over is the period: a fourth moment recovered against a stale shift pays {@code (u/σ)⁴} where a second pays {@code (u/σ)²}, so the shift goes stale four times faster in the exponent. MEASURED on 1200-bar series at {@code n = 30}, worst relative error per bar against a 60-digit reference: | rebuild every | 32n ({@code VAR}'s) | 8n | 2n | n | n/4 | |---|---|---|---|---|---| | random walk around 100 | 1.18e-06 | 9.33e-07 | 1.61e-08 | 1.28e-09 | 4.58e-12 | | random walk on 3.1e10 | 4.99e-06 | 4.99e-06 | 6.27e-10 | 1.45e-09 | 1.76e-12 | | outlier 1e5 every 200 bars | 2.84e-13 | 2.84e-13 | 2.68e-13 | 1.51e-13 | 4.48e-14 | {@code VAR}'s {@code 32n} leaves 1e-6 on an ordinary random walk, and the collapse trigger cannot catch it — the ratio it tests sits around 0.24 there, six orders from firing. Hence {@code n/4}.</li>
    * <li><b>Rebuilding beats rescanning.</b> Rescanning the whole window every bar measures 5.34e-11 and 4.30e-11 on those two walks — worse than the {@code n/4} rebuild, at roughly ten times the arithmetic. The rebuild anchors the shift at the window <i>mean</i>; a per-bar rescan can only anchor it at a window <i>value</i>, which sits further from centre.</li>
    * <li><b>The result is not bounded.</b> There is no clamp and no range assertion; a window dominated by one outlier is legitimately far above 0.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#kurtosisLookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal The series to measure.
    * @param optInTimePeriod Number of trailing values in the window, at least 4
    *        (default 30; range 4..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outReal Excess kurtosis of the trailing window, or NaN where the
    *        window has no spread. Must hold at least {@code endIdx - startIdx + 1}
    *        values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#var
    * @see Core#stddev
    * @see Core#bbands
    */
   public OutRange kurtosis( int startIdx,
                             int endIdx,
                             double inReal[],
                             int optInTimePeriod,
                             double outReal[] )
   {
      requireIndexRange("KURTOSIS", startIdx, endIdx);
      int guardStart = clampedStart("KURTOSIS", startIdx, kurtosisLookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("KURTOSIS", "inReal", inReal, guardInLen);
      requireLength("KURTOSIS", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = kurtosisImpl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("KURTOSIS", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * The fourth standardised moment of the trailing window, minus 3 so a normal
    * window reads 0. A tail-weight measure: above 0 the window has fatter tails
    * and a sharper peak than a normal distribution of the same variance, below
    * 0 it is flatter. The companion to the shipped <a
    * href="https://ta-lib.org/functions/var">{@code VAR}</a> and <a
    * href="https://ta-lib.org/functions/stddev">{@code STDDEV}</a> in the same
    * group. {@code SKEW}, the third-moment sibling, is deliberately a separate
    * question — it carries its own convention fork and its own references, and
    * two functions that share only a word get independent docs and decisions.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/kurtosis">ta-lib.org/functions/kurtosis</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li><b>Which estimator.</b> This is {@code G2}, what Excel {@code KURT}, {@code scipy.stats.kurtosis(bias=False)} and R {@code e1071::kurtosis(type=2)} all compute, and the one that is unbiased under normality. The other form in circulation is the biased {@code g2 = m₄/m₂² − 3} over population moments, which NIST/SEMATECH gives. They are not a rounding convention apart: on a 9-point normal sample {@code G2 = 1.79450407519901} against {@code g2 = 0.342114639479481}.</li>
    * <li><b>A window with no spread returns NaN, not a number.</b> Excess kurtosis has no defensible neutral to fall back on — {@code 0} asserts normality and {@code −1.2} asserts uniformity, and a point mass supports neither. {@code scipy.stats.kurtosis} returns {@code nan} at both bias settings, and Excel {@code KURT} documents {@code #DIV/0!} when the sample's standard deviation is zero. This function declares {@code nan_inf_output} and leaves the division unguarded, so the answer arrives from IEEE rather than from a branch. It is deliberately a <i>different</i> answer from {@code VAR}'s floor-to-zero: a variance of zero says something true about the window, a kurtosis of a point mass does not.</li>
    * <li><b>Cancellation, and why the rebuild period is not {@code VAR}'s.</b> Deviations are taken against a shift near the window and the central moments recovered from the shifted sums, as {@code var.c} does. What does not carry over is the period: a fourth moment recovered against a stale shift pays {@code (u/σ)⁴} where a second pays {@code (u/σ)²}, so the shift goes stale four times faster in the exponent. MEASURED on 1200-bar series at {@code n = 30}, worst relative error per bar against a 60-digit reference: | rebuild every | 32n ({@code VAR}'s) | 8n | 2n | n | n/4 | |---|---|---|---|---|---| | random walk around 100 | 1.18e-06 | 9.33e-07 | 1.61e-08 | 1.28e-09 | 4.58e-12 | | random walk on 3.1e10 | 4.99e-06 | 4.99e-06 | 6.27e-10 | 1.45e-09 | 1.76e-12 | | outlier 1e5 every 200 bars | 2.84e-13 | 2.84e-13 | 2.68e-13 | 1.51e-13 | 4.48e-14 | {@code VAR}'s {@code 32n} leaves 1e-6 on an ordinary random walk, and the collapse trigger cannot catch it — the ratio it tests sits around 0.24 there, six orders from firing. Hence {@code n/4}.</li>
    * <li><b>Rebuilding beats rescanning.</b> Rescanning the whole window every bar measures 5.34e-11 and 4.30e-11 on those two walks — worse than the {@code n/4} rebuild, at roughly ten times the arithmetic. The rebuild anchors the shift at the window <i>mean</i>; a per-bar rescan can only anchor it at a window <i>value</i>, which sits further from centre.</li>
    * <li><b>The result is not bounded.</b> There is no clamp and no range assertion; a window dominated by one outlier is legitimately far above 0.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#kurtosisLookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal The series to measure.
    * @param optInTimePeriod Number of trailing values in the window, at least 4
    *        (default 30; range 4..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outReal Excess kurtosis of the trailing window, or NaN where the
    *        window has no spread. Must hold at least {@code endIdx - startIdx + 1}
    *        values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#var
    * @see Core#stddev
    * @see Core#bbands
    */
   public OutRange kurtosis( int startIdx,
                             int endIdx,
                             float inReal[],
                             int optInTimePeriod,
                             double outReal[] )
   {
      requireIndexRange("KURTOSIS", startIdx, endIdx);
      int guardStart = clampedStart("KURTOSIS", startIdx, kurtosisLookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("KURTOSIS", "inReal", inReal, guardInLen);
      requireLength("KURTOSIS", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = kurtosisImpl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("KURTOSIS", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live KURTOSIS stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#kurtosis} over the same series.
    * Open with {@link Core#kurtosisOpen}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code clone} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code clone} never write the stream and may be called
    * concurrently after safe publication. Independent streams (a
    * {@code clone()} result included) are fully independent.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class KurtosisStream {
      private Core core;
      private int optInTimePeriod;
      private double shift;
      private double total1;
      private double total2;
      private double total3;
      private double total4;
      private double dPeriod;
      private double coefA;
      private double coefB;
      private double invPeriod;
      private int trailingIdx;
      private int nbInitialElementNeeded;
      private int barsSinceReseed;
      private int reseedPeriod;
      private int j;
      private int windowStart;
      private int i;
      private int xMask;
      private double[] x_inReal;
      private double cur_outReal;
      private int outRangeBegIdx;
      private int outRangeCount;

      private KurtosisStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#kurtosis} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count — a rejected one
       * changes nothing, and neither does {@code peek} — and
       * {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       * <p>The last bar it can reach is {@link Core#MAX_INDEX}; past that
       * {@code update} and {@code advance} throw
       * {@link IndexOutOfBoundsException}.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      /**
       * Count one bar this stream was not fed: {@link #outRange()} advances
       * by one and nothing else moves — {@link #value()} keeps answering the previous
       * output, which is this bar's output too.
       * <p>For a bar the caller leaves out: one an {@code update} rejected
       * and that will not be re-fed, or a session with no print. Without it
       * two handles on one feed drift a bar apart when only one of them skips.
       * <p>Throws {@link IndexOutOfBoundsException} once {@link #outRange()}
       * has reached bar {@link Core#MAX_INDEX}, the last one the batch tier
       * can address and the last this handle will count. {@code update}
       * throws the same there.
       */
      public void advance() {
         if( this.outRangeBegIdx + this.outRangeCount > MAX_INDEX )
            throw failure("KURTOSIS advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private KurtosisStream( KurtosisStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.shift = other.shift;
         this.total1 = other.total1;
         this.total2 = other.total2;
         this.total3 = other.total3;
         this.total4 = other.total4;
         this.dPeriod = other.dPeriod;
         this.coefA = other.coefA;
         this.coefB = other.coefB;
         this.invPeriod = other.invPeriod;
         this.trailingIdx = other.trailingIdx;
         this.nbInitialElementNeeded = other.nbInitialElementNeeded;
         this.barsSinceReseed = other.barsSinceReseed;
         this.reseedPeriod = other.reseedPeriod;
         this.j = other.j;
         this.windowStart = other.windowStart;
         this.i = other.i;
         this.xMask = other.xMask;
         this.x_inReal = other.x_inReal.clone();
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, returning the new current value.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so nothing moves — {@link #outRange()} included — and
       * {@link #value()} still answers the previous value. Re-feed the bar when a
       * corrected value arrives, or call {@link #advance()} to count it and
       * carry on; two handles on one feed drift a bar apart if neither
       * happens.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       * <p>Throws {@link IndexOutOfBoundsException} once {@link #outRange()}
       * has reached bar {@link Core#MAX_INDEX}, which no re-feed clears: the
       * handle has run out of index domain and only a shorter history can
       * start a new one.
       */
      public double update( double inReal ) {
         if( this.outRangeBegIdx + this.outRangeCount > MAX_INDEX )
            throw failure("KURTOSIS update", RetCode.OUT_OF_RANGE_END_INDEX);
         if( !Double.isFinite(inReal) )
            throw new TALibArgumentException("KURTOSIS update: BAD_PARAM", RetCode.BAD_PARAM);
         core.kurtosisStepImpl(this, inReal);
         this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may
       * run concurrently with each other, and its cost does not grow with the
       * period.
       * <p>It counts no bar, so it keeps answering past the
       * {@link Core#MAX_INDEX} ceiling {@code update} stops at.
       */
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TALibArgumentException("KURTOSIS peek: BAD_PARAM", RetCode.BAD_PARAM);
         KurtosisStream sp = this;
         double tempReal = 0.0;
         double dev = 0.0;
         double dev2 = 0.0;
         double residue = 0.0;
         double residueSq = 0.0;
         double moment2 = 0.0;
         double moment4 = 0.0;
         double sampleVar = 0.0;
         double varSquared = 0.0;
         double kurt = 0.0;
         int barsSinceReseed = sp.barsSinceReseed;
         double cur_outReal = 0.0;
         int j = sp.j;
         double shift = sp.shift;
         double total1 = sp.total1;
         double total2 = sp.total2;
         double total3 = sp.total3;
         double total4 = sp.total4;
         int trailingIdx = sp.trailingIdx;
         int windowStart = sp.windowStart;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         pkSlot0 = sp.i & sp.xMask;
         pkVal0 = inReal;
         dev = (((sp.i & sp.xMask) != pkSlot0) ? sp.x_inReal[sp.i & sp.xMask] : pkVal0) - shift;
         dev2 = dev * dev;
         total1 += dev;
         total2 += dev2;
         total3 += dev2 * dev;
         total4 += dev2 * dev2;
         /* Central moments from the shifted sums. `residue` is what the shift
          * left behind: ~0 right after a rebuild, growing as the window walks
          * away from the anchor. That growth is the quantity the rebuild period
          * bounds, and the reason a fourth moment needs a shorter period than a
          * second -- it enters here raised to the fourth power.
          */
         residue = total1 * sp.invPeriod;
         residueSq = residue * residue;
         moment2 = total2 - sp.dPeriod * residueSq;
         moment4 = Math.fma(6.0 * residueSq, total2, total4 - 4.0 * residue * total3) - 3.0 * sp.dPeriod * residueSq * residueSq;
         /* Remove the trailing value (prepares the next window). */
         dev = (((trailingIdx & sp.xMask) != pkSlot0) ? sp.x_inReal[trailingIdx & sp.xMask] : pkVal0) - shift;
         dev2 = dev * dev;
         total1 -= dev;
         total2 -= dev2;
         total3 -= dev2 * dev;
         total4 -= dev2 * dev2;
         trailingIdx += 1;
         /* Rebuild when the window walked far enough from the anchor for the
          * expansion above to be subtracting like-sized quantities; when the value
          * just removed sat so far from the shift that its fourth power dwarfs
          * what survives (a large outlier passing through buries the small terms
          * below its ulp, and the residue it leaves is cancellation garbage); or
          * on the period derived above regardless.
          */
         barsSinceReseed -= 1;
         if( moment2 < 0.000001 * total2 || dev2 * dev2 > 1000000.0 * total4 || barsSinceReseed <= 0 ) {
            barsSinceReseed = sp.reseedPeriod;
            windowStart = sp.i - sp.nbInitialElementNeeded;
            tempReal = 0.0;
            for( j = windowStart; j <= sp.i; j += 1 ) {
               tempReal += ((j & sp.xMask) != pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0;
            }
            shift = tempReal * sp.invPeriod;
            total1 = 0.0;
            total2 = 0.0;
            total3 = 0.0;
            total4 = 0.0;
            for( j = windowStart; j <= sp.i; j += 1 ) {
               dev = (((j & sp.xMask) != pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0) - shift;
               dev2 = dev * dev;
               total1 += dev;
               total2 += dev2;
               total3 += dev2 * dev;
               total4 += dev2 * dev2;
            }
            residue = total1 * sp.invPeriod;
            residueSq = residue * residue;
            moment2 = total2 - sp.dPeriod * residueSq;
            moment4 = Math.fma(6.0 * residueSq, total2, total4 - 4.0 * residue * total3) - 3.0 * sp.dPeriod * residueSq * residueSq;
            /* Re-remove the trailing value under the new shift so the carried
             * state matches the non-rebuild path.
             */
            dev = (((windowStart & sp.xMask) != pkSlot0) ? sp.x_inReal[windowStart & sp.xMask] : pkVal0) - shift;
            dev2 = dev * dev;
            total1 -= dev;
            total2 -= dev2;
            total3 -= dev2 * dev;
            total4 -= dev2 * dev2;
         }
         /* A window with no spread has no kurtosis to report, and there is no
          * defensible neutral to substitute: 0 asserts normality and -1.2 asserts
          * uniformity, neither of which a point mass supports. scipy returns nan
          * at both bias settings and Excel KURT answers #DIV/0!.
          *
          * So the division is left UNGUARDED, as rvol.c leaves its own, and the
          * function declares nan_inf_output. On a point mass the rebuild anchors
          * the shift at the single value, every deviation is exactly 0, and both
          * moments are exactly 0 -- so this is 0/0 and IEEE answers NaN without a
          * branch. A guard with an absolute epsilon would be a cliff at a price
          * level rather than a noise floor, which is the mistake #243 fixed in the
          * var/stddev/bbands family: a $100 instrument quoted in 1e-8 ticks had
          * every bar zeroed.
          *
          * Nothing NaN re-enters the running sums -- the division happens here, at
          * the output write, on sums that stay finite.
          */
         sampleVar = moment2 / (sp.dPeriod - 1.0);
         varSquared = sampleVar * sampleVar;
         kurt = sp.coefA * (moment4 / varSquared) - sp.coefB;
         cur_outReal = kurt;
         return cur_outReal;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public double value() {
         return this.cur_outReal;
      }

      /**
       * An independent fork of this stream: both evolve separately from here
       * on. Buffers are copied and sub-streams cloned recursively; the
       * {@link Core} reference is shared, since a {@code Core} is immutable
       * for a stream's lifetime.
       *
       * <p>Not the {@code Cloneable} protocol: this calls a copy constructor,
       * never {@code super.clone()}, so it throws nothing.
       *
       * @return an independent stream at the same bar
       */
      @Override
      public KurtosisStream clone() {
         return new KurtosisStream(this);
      }
   }
   private void kurtosisStepImpl( KurtosisStream sp, double inReal )
   {
      double tempReal = 0.0;
      double dev = 0.0;
      double dev2 = 0.0;
      double residue = 0.0;
      double residueSq = 0.0;
      double moment2 = 0.0;
      double moment4 = 0.0;
      double sampleVar = 0.0;
      double varSquared = 0.0;
      double kurt = 0.0;
      sp.x_inReal[sp.i & sp.xMask] = inReal;
      dev = sp.x_inReal[sp.i & sp.xMask] - sp.shift;
      dev2 = dev * dev;
      sp.total1 += dev;
      sp.total2 += dev2;
      sp.total3 += dev2 * dev;
      sp.total4 += dev2 * dev2;
      /* Central moments from the shifted sums. `residue` is what the shift
       * left behind: ~0 right after a rebuild, growing as the window walks
       * away from the anchor. That growth is the quantity the rebuild period
       * bounds, and the reason a fourth moment needs a shorter period than a
       * second -- it enters here raised to the fourth power.
       */
      residue = sp.total1 * sp.invPeriod;
      residueSq = residue * residue;
      moment2 = sp.total2 - sp.dPeriod * residueSq;
      moment4 = Math.fma(6.0 * residueSq, sp.total2, sp.total4 - 4.0 * residue * sp.total3) - 3.0 * sp.dPeriod * residueSq * residueSq;
      /* Remove the trailing value (prepares the next window). */
      dev = sp.x_inReal[sp.trailingIdx & sp.xMask] - sp.shift;
      dev2 = dev * dev;
      sp.total1 -= dev;
      sp.total2 -= dev2;
      sp.total3 -= dev2 * dev;
      sp.total4 -= dev2 * dev2;
      sp.trailingIdx += 1;
      /* Rebuild when the window walked far enough from the anchor for the
       * expansion above to be subtracting like-sized quantities; when the value
       * just removed sat so far from the shift that its fourth power dwarfs
       * what survives (a large outlier passing through buries the small terms
       * below its ulp, and the residue it leaves is cancellation garbage); or
       * on the period derived above regardless.
       */
      sp.barsSinceReseed -= 1;
      if( moment2 < 0.000001 * sp.total2 || dev2 * dev2 > 1000000.0 * sp.total4 || sp.barsSinceReseed <= 0 ) {
         sp.barsSinceReseed = sp.reseedPeriod;
         sp.windowStart = sp.i - sp.nbInitialElementNeeded;
         tempReal = 0.0;
         for( sp.j = sp.windowStart; sp.j <= sp.i; sp.j += 1 ) {
            tempReal += sp.x_inReal[sp.j & sp.xMask];
         }
         sp.shift = tempReal * sp.invPeriod;
         sp.total1 = 0.0;
         sp.total2 = 0.0;
         sp.total3 = 0.0;
         sp.total4 = 0.0;
         for( sp.j = sp.windowStart; sp.j <= sp.i; sp.j += 1 ) {
            dev = sp.x_inReal[sp.j & sp.xMask] - sp.shift;
            dev2 = dev * dev;
            sp.total1 += dev;
            sp.total2 += dev2;
            sp.total3 += dev2 * dev;
            sp.total4 += dev2 * dev2;
         }
         residue = sp.total1 * sp.invPeriod;
         residueSq = residue * residue;
         moment2 = sp.total2 - sp.dPeriod * residueSq;
         moment4 = Math.fma(6.0 * residueSq, sp.total2, sp.total4 - 4.0 * residue * sp.total3) - 3.0 * sp.dPeriod * residueSq * residueSq;
         /* Re-remove the trailing value under the new shift so the carried
          * state matches the non-rebuild path.
          */
         dev = sp.x_inReal[sp.windowStart & sp.xMask] - sp.shift;
         dev2 = dev * dev;
         sp.total1 -= dev;
         sp.total2 -= dev2;
         sp.total3 -= dev2 * dev;
         sp.total4 -= dev2 * dev2;
      }
      /* A window with no spread has no kurtosis to report, and there is no
       * defensible neutral to substitute: 0 asserts normality and -1.2 asserts
       * uniformity, neither of which a point mass supports. scipy returns nan
       * at both bias settings and Excel KURT answers #DIV/0!.
       *
       * So the division is left UNGUARDED, as rvol.c leaves its own, and the
       * function declares nan_inf_output. On a point mass the rebuild anchors
       * the shift at the single value, every deviation is exactly 0, and both
       * moments are exactly 0 -- so this is 0/0 and IEEE answers NaN without a
       * branch. A guard with an absolute epsilon would be a cliff at a price
       * level rather than a noise floor, which is the mistake #243 fixed in the
       * var/stddev/bbands family: a $100 instrument quoted in 1e-8 ticks had
       * every bar zeroed.
       *
       * Nothing NaN re-enters the running sums -- the division happens here, at
       * the output write, on sums that stay finite.
       */
      sampleVar = moment2 / (sp.dPeriod - 1.0);
      varSquared = sampleVar * sampleVar;
      kurt = sp.coefA * (moment4 / varSquared) - sp.coefB;
      sp.cur_outReal = kurt;
      sp.i += 1;
   }
   private RetCode kurtosisOpenImpl( KurtosisStream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double tempReal = 0;
      double shift = 0;
      double total1 = 0;
      double total2 = 0;
      double total3 = 0;
      double total4 = 0;
      double dev = 0;
      double dev2 = 0;
      double residue = 0;
      double residueSq = 0;
      double moment2 = 0;
      double moment4 = 0;
      double sampleVar = 0;
      double varSquared = 0;
      double dPeriod = 0;
      double coefA = 0;
      double coefB = 0;
      double invPeriod = 0;
      double kurt = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int windowStart = 0;
      int nbInitialElementNeeded = 0;
      int barsSinceReseed = 0;
      int reseedPeriod = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OUT_OF_RANGE_START_INDEX;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 4 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      nbInitialElementNeeded = optInTimePeriod - 1;
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      /* G2, the sample-adjusted Fisher excess kurtosis. The two coefficients are
       * computed in double because their integer forms overflow: at the top of the
       * parameter range (n-1)(n-2)(n-3) is ~1e15, far past what an int holds, and
       * the product would wrap silently rather than fail.
       *
       * The (n-2)(n-3) denominators are why the range starts at 4 rather than 1.
       * The argument contract rejects anything below it, and the n-1 lookback
       * guarantees a full window at the first emitted bar, so the estimator is
       * never handed fewer than four points and needs no runtime branch for it.
       */
      dPeriod = (double)optInTimePeriod;
      coefA = dPeriod * (dPeriod + 1.0) / ((dPeriod - 1.0) * (dPeriod - 2.0) * (dPeriod - 3.0));
      coefB = 3.0 * (dPeriod - 1.0) * (dPeriod - 1.0) / ((dPeriod - 2.0) * (dPeriod - 3.0));
      invPeriod = 1.0 / dPeriod;
      /* Deviations are measured against a shift near the window, as var.c does, so
       * the running sums stay at deviation scale instead of at price scale. The
       * central moments are then recovered from the shifted sums by the binomial
       * expansion below, which puts back the residue the shift left behind.
       *
       * The RESEED PERIOD IS NOT var.c's. MEASURED on 1200-bar series at n=30,
       * worst relative error against a 60-digit reference computed per window:
       *
       *   rebuild every    32n (var.c)    8n        2n        n         n/4
       *   walk around 100   1.18e-06   9.33e-07  1.61e-08  1.28e-09  4.58e-12
       *   walk on 3.1e10    4.99e-06   4.99e-06  6.27e-10  1.45e-09  1.76e-12
       *   outlier 1e5/200   2.84e-13   2.84e-13  2.68e-13  1.51e-13  4.48e-14
       *
       * A fourth moment recovered against a stale shift pays (u/sigma)^4 where a
       * second pays (u/sigma)^2, so the shift goes stale four times faster in the
       * exponent and var.c's 32n leaves 1e-6 on an ordinary random walk. The
       * collapse trigger cannot catch that case -- the ratio it tests sits around
       * 0.24 there, six orders from firing -- so only the periodic rebuild can,
       * and it has to run at n/4.
       *
       * Rebuilding that often also beats rescanning the window every bar
       * (5.34e-11 and 4.30e-11 on the two walks): the rebuild anchors the shift at
       * the window MEAN, while a per-bar rescan can only anchor it at a window
       * VALUE, which sits further from centre and leaves a larger u. About a tenth
       * of the arithmetic, and more accurate.
       */
      reseedPeriod = optInTimePeriod / 4;
      trailingIdx = startIdx - nbInitialElementNeeded;
      shift = inReal[trailingIdx];
      total1 = 0.0;
      total2 = 0.0;
      total3 = 0.0;
      total4 = 0.0;
      for( j = trailingIdx; j < startIdx; j += 1 ) {
         dev = inReal[j] - shift;
         dev2 = dev * dev;
         total1 += dev;
         total2 += dev2;
         total3 += dev2 * dev;
         total4 += dev2 * dev2;
      }
      /* inReal and outReal may be the same buffer: each trailing value is consumed
       * before its slot is overwritten by the output.
       */
      i = startIdx;
      outIdx = 0;
      barsSinceReseed = reseedPeriod;
      do {
         dev = inReal[i] - shift;
         dev2 = dev * dev;
         total1 += dev;
         total2 += dev2;
         total3 += dev2 * dev;
         total4 += dev2 * dev2;
         /* Central moments from the shifted sums. `residue` is what the shift
          * left behind: ~0 right after a rebuild, growing as the window walks
          * away from the anchor. That growth is the quantity the rebuild period
          * bounds, and the reason a fourth moment needs a shorter period than a
          * second -- it enters here raised to the fourth power.
          */
         residue = total1 * invPeriod;
         residueSq = residue * residue;
         moment2 = total2 - dPeriod * residueSq;
         moment4 = Math.fma(6.0 * residueSq, total2, total4 - 4.0 * residue * total3) - 3.0 * dPeriod * residueSq * residueSq;
         /* Remove the trailing value (prepares the next window). */
         dev = inReal[trailingIdx] - shift;
         dev2 = dev * dev;
         total1 -= dev;
         total2 -= dev2;
         total3 -= dev2 * dev;
         total4 -= dev2 * dev2;
         trailingIdx += 1;
         /* Rebuild when the window walked far enough from the anchor for the
          * expansion above to be subtracting like-sized quantities; when the value
          * just removed sat so far from the shift that its fourth power dwarfs
          * what survives (a large outlier passing through buries the small terms
          * below its ulp, and the residue it leaves is cancellation garbage); or
          * on the period derived above regardless.
          */
         barsSinceReseed -= 1;
         if( moment2 < 0.000001 * total2 || dev2 * dev2 > 1000000.0 * total4 || barsSinceReseed <= 0 ) {
            barsSinceReseed = reseedPeriod;
            windowStart = i - nbInitialElementNeeded;
            tempReal = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               tempReal += inReal[j];
            }
            shift = tempReal * invPeriod;
            total1 = 0.0;
            total2 = 0.0;
            total3 = 0.0;
            total4 = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               dev = inReal[j] - shift;
               dev2 = dev * dev;
               total1 += dev;
               total2 += dev2;
               total3 += dev2 * dev;
               total4 += dev2 * dev2;
            }
            residue = total1 * invPeriod;
            residueSq = residue * residue;
            moment2 = total2 - dPeriod * residueSq;
            moment4 = Math.fma(6.0 * residueSq, total2, total4 - 4.0 * residue * total3) - 3.0 * dPeriod * residueSq * residueSq;
            /* Re-remove the trailing value under the new shift so the carried
             * state matches the non-rebuild path.
             */
            dev = inReal[windowStart] - shift;
            dev2 = dev * dev;
            total1 -= dev;
            total2 -= dev2;
            total3 -= dev2 * dev;
            total4 -= dev2 * dev2;
         }
         /* A window with no spread has no kurtosis to report, and there is no
          * defensible neutral to substitute: 0 asserts normality and -1.2 asserts
          * uniformity, neither of which a point mass supports. scipy returns nan
          * at both bias settings and Excel KURT answers #DIV/0!.
          *
          * So the division is left UNGUARDED, as rvol.c leaves its own, and the
          * function declares nan_inf_output. On a point mass the rebuild anchors
          * the shift at the single value, every deviation is exactly 0, and both
          * moments are exactly 0 -- so this is 0/0 and IEEE answers NaN without a
          * branch. A guard with an absolute epsilon would be a cliff at a price
          * level rather than a noise floor, which is the mistake #243 fixed in the
          * var/stddev/bbands family: a $100 instrument quoted in 1e-8 ticks had
          * every bar zeroed.
          *
          * Nothing NaN re-enters the running sums -- the division happens here, at
          * the output write, on sums that stay finite.
          */
         sampleVar = moment2 / (dPeriod - 1.0);
         varSquared = sampleVar * sampleVar;
         kurt = coefA * (moment4 / varSquared) - coefB;
         outReal[outIdx++ * outStride] = kurt;
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int capX = i - trailingIdx + 1;
      if( capX < 1 || capX > historyLen ) {
         return RetCode.INTERNAL_ERROR;
      }
      int physX = 1;
      while( physX < capX ) {
         physX <<= 1;
      }
      double[] capX_inReal = new double[physX];
      for( int fillJ = historyLen - capX; fillJ < historyLen; fillJ++ ) {
         capX_inReal[fillJ & (physX - 1)] = inReal[fillJ];
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.shift = shift;
      sp.total1 = total1;
      sp.total2 = total2;
      sp.total3 = total3;
      sp.total4 = total4;
      sp.dPeriod = dPeriod;
      sp.coefA = coefA;
      sp.coefB = coefB;
      sp.invPeriod = invPeriod;
      sp.trailingIdx = trailingIdx;
      sp.nbInitialElementNeeded = nbInitialElementNeeded;
      sp.barsSinceReseed = barsSinceReseed;
      sp.reseedPeriod = reseedPeriod;
      sp.j = j;
      sp.windowStart = windowStart;
      sp.i = i;
      sp.xMask = physX - 1;
      sp.x_inReal = capX_inReal;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.SUCCESS;
   }
   /* kurtosisOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   KurtosisStream kurtosisOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      KurtosisStream sp = new KurtosisStream(this);
      RetCode retCode = kurtosisOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw new InsufficientHistoryException("KURTOSIS openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.INTERNAL_ERROR ) {
         throw new TALibStateException("KURTOSIS openAndFill: internal error", retCode);
      }
      throw new TALibArgumentException("KURTOSIS openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind kurtosisOpen (composition seam). */
   KurtosisStream kurtosisOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      KurtosisStream sp = new KurtosisStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = kurtosisOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw new InsufficientHistoryException("KURTOSIS open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.INTERNAL_ERROR ) {
         throw new TALibStateException("KURTOSIS open: internal error", retCode);
      }
      throw new TALibArgumentException("KURTOSIS open: " + retCode, retCode);
   }
   /**
    * Open a live KURTOSIS stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#kurtosis} at that bar.
    * <p>The history must hold at least {@code kurtosisLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} selects a parameter's documented default,
    * as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public KurtosisStream kurtosisOpen( double inReal[], int optInTimePeriod )
   {
      requireArgument("KURTOSIS open", "inReal", inReal);
      requireHistory("KURTOSIS open", inReal.length);
      return kurtosisOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#kurtosisOpen} that also fills the output array(s) bit-identically
    * to {@link Core#kurtosis} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link KurtosisStream#outRange()}.
    */
   public KurtosisStream kurtosisOpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("KURTOSIS openAndFill", "inReal", inReal);
      requireHistory("KURTOSIS openAndFill", inReal.length);
      int guardOutLen = openFillCount("KURTOSIS openAndFill", inReal.length, kurtosisLookback(optInTimePeriod));
      requireLength("KURTOSIS openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw new TALibArgumentException("KURTOSIS openAndFill: " + RetCode.BAD_PARAM, RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return kurtosisOpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
