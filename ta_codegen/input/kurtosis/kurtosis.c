/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  091526 KL     First version (proposal-drafts issue #72).
 *  092126 MF,CC  Rebuild against the peak moments (issue #433).
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
   double dPeriod, coefA, coefB, invPeriod, kurt, peak2, peak4;
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
    * parameter range (n-1)(n-2)(n-3) is ~1e15, far past what an int holds.
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

   /* Deviations are measured against a shift near the window, so the running
    * sums stay at deviation scale instead of at price scale, and the central
    * moments are recovered from them by the binomial expansion below. The
    * period bounds the rounding the slide accumulates and is the only way out
    * of a NaN or Inf input, on which every trigger comparison is false; a stale
    * shift is caught by the triggers in the loop, not by this.
    */
   reseedPeriod = 32 * optInTimePeriod;

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

   i = startIdx;
   outIdx = 0;
   barsSinceReseed = reseedPeriod;
   peak2 = total2;
   peak4 = total4;
   do
   {
      dev = inReal[i] - shift;
      dev2 = dev * dev;
      total1 += dev;
      total2 += dev2;
      total3 += dev2 * dev;
      total4 += dev2 * dev2;
      if( total2 > peak2 )
         peak2 = total2;
      if( total4 > peak4 )
         peak4 = total4;

      residue = total1 * invPeriod;
      residueSq = residue * residue;
      moment2 = total2 - dPeriod * residueSq;
      moment4 = total4
      - 4.0 * residue * total3
      + 6.0 * residueSq * total2
      - 3.0 * dPeriod * residueSq * residueSq;

      /* Rebuild once either central moment falls below 1% of the largest
       * shifted sum of its power held since the last rebuild. That catches a
       * shift several sigma stale, where the expansion above cancels as
       * (u/sigma)^4, and a series decaying back onto the shift or an outlier
       * leaving the window, which leave current sums made mostly of rounding
       * that scales with the peaks.
       */
      barsSinceReseed--;
      if( moment2 < 0.01 * peak2
         || moment4 < 0.01 * peak4
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

         /* The left-to-right sum misses the mean by many ulps at a long
          * period, and a window whose spread is under that miss would fire a
          * trigger again on every bar. The residue just measured is the
          * miss: adding it back lands within about an ulp of the mean, where
          * a flat window's deviations are exactly 0.
          */
         if( moment2 < 0.01 * total2 || moment4 < 0.01 * total4 )
         {
            shift += residue;

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
         }
         peak2 = total2;
         peak4 = total4;
      }

      /* No spread, no kurtosis: 0 would assert normality and -1.2 uniformity.
       * A flat window reaches here with every deviation exactly 0, so this is
       * 0/0 and IEEE answers NaN without a branch. A guard with an absolute
       * epsilon would be a cliff at a price level, not a noise floor (#243).
       */
      sampleVar = moment2 / (dPeriod-1.0);
      varSquared = sampleVar * sampleVar;
      kurt = coefA * (moment4 / varSquared) - coefB;

      /* inReal and outReal may be the same buffer: read the trailing value
       * before the output write.
       */
      dev = inReal[trailingIdx] - shift;
      dev2 = dev * dev;
      total1 -= dev;
      total2 -= dev2;
      total3 -= dev2 * dev;
      total4 -= dev2 * dev2;
      trailingIdx++;

      outReal[outIdx++] = kurt;
      i++;
   } while( i <= endIdx );

   *outNBElement = outIdx;
   *outBegIdx = startIdx;

   return TA_SUCCESS;
}
