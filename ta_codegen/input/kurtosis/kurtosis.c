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

int kurtosis_lookback(int optInTimePeriod)
{
   return optInTimePeriod-1;
}

TA_RetCode kurtosis(int startIdx, int endIdx,
   const double *inReal,
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double *outReal)
{
   double tempReal, shift, total1, total2, total3, total4;
   double dev, dev2, residue, residueSq, moment2, moment4;
   double sampleVar, varSquared;
   double dPeriod, coefA, coefB, invPeriod, kurt;
   int i, j, outIdx, trailingIdx, windowStart;
   int nbInitialElementNeeded, barsSinceReseed, reseedPeriod;

   nbInitialElementNeeded = (optInTimePeriod-1);

   if( startIdx < nbInitialElementNeeded )
      startIdx = nbInitialElementNeeded;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
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
   coefA = ( dPeriod * (dPeriod+1.0) )
   / ( (dPeriod-1.0) * (dPeriod-2.0) * (dPeriod-3.0) );
   coefB = ( 3.0 * (dPeriod-1.0) * (dPeriod-1.0) )
   / ( (dPeriod-2.0) * (dPeriod-3.0) );
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
   for( j=trailingIdx; j < startIdx; j++ )
   {
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
   do
   {
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
      moment4 = total4
      - 4.0 * residue * total3
      + 6.0 * residueSq * total2
      - 3.0 * dPeriod * residueSq * residueSq;

      /* Remove the trailing value (prepares the next window). */
      dev = inReal[trailingIdx] - shift;
      dev2 = dev * dev;
      total1 -= dev;
      total2 -= dev2;
      total3 -= dev2 * dev;
      total4 -= dev2 * dev2;
      trailingIdx++;

      /* Rebuild when the window walked far enough from the anchor for the
       * expansion above to be subtracting like-sized quantities; when the value
       * just removed sat so far from the shift that its fourth power dwarfs
       * what survives (a large outlier passing through buries the small terms
       * below its ulp, and the residue it leaves is cancellation garbage); or
       * on the period derived above regardless.
       */
      barsSinceReseed--;
      if( moment2 < 0.000001 * total2
         || dev2 * dev2 > 1000000.0 * total4
         || barsSinceReseed <= 0 )
      {
         barsSinceReseed = reseedPeriod;

         windowStart = i - nbInitialElementNeeded;

         tempReal = 0.0;
         for( j=windowStart; j <= i; j++ )
            tempReal += inReal[j];
         shift = tempReal * invPeriod;

         total1 = 0.0;
         total2 = 0.0;
         total3 = 0.0;
         total4 = 0.0;
         for( j=windowStart; j <= i; j++ )
         {
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
         moment4 = total4
         - 4.0 * residue * total3
         + 6.0 * residueSq * total2
         - 3.0 * dPeriod * residueSq * residueSq;

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
      sampleVar = moment2 / (dPeriod-1.0);
      varSquared = sampleVar * sampleVar;
      kurt = coefA * (moment4 / varSquared) - coefB;

      outReal[outIdx++] = kurt;
      i++;
   } while( i <= endIdx );

   *outNBElement = outIdx;
   *outBegIdx = startIdx;

   return TA_SUCCESS;
}
