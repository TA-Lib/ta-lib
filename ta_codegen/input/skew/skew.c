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
 *  110126 MF,CC  First version (issue #569).
 */

int skew_lookback(int optInTimePeriod)
{
   return optInTimePeriod-1;
}

TA_RetCode skew(int startIdx, int endIdx,
   const double *inReal,
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double *outReal)
{
   double tempReal, shift, total1, total2, total3;
   double dev, dev2, residue, residueSq, moment2, moment3;
   double dPeriod, coef, invPeriod, skewness, peak2, denominator;
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

   /* G1 = n*sqrt(n-1)/(n-2) * M3 / M2^(3/2), over the central sums M2 and
    * M3. The n-2 is why the range starts at 3, and the n-1 lookback hands the
    * estimator a full window at the first emitted bar.
    */
   dPeriod = (double)optInTimePeriod;
   coef = ( dPeriod * sqrt(dPeriod-1.0) ) / (dPeriod-2.0);
   invPeriod = 1.0 / dPeriod;

   /* Deviations are measured against a shift near the window, and the
    * central sums are recovered from them below. The periodic rebuild bounds
    * the rounding the slide accumulates, and is the only way out of a NaN or
    * Inf input, on which every trigger comparison is false.
    */
   reseedPeriod = 32 * optInTimePeriod;

   trailingIdx = startIdx - nbInitialElementNeeded;
   shift = inReal[trailingIdx];

   total1 = 0.0;
   total2 = 0.0;
   total3 = 0.0;
   for( j=trailingIdx; j < startIdx; j++ )
   {
      dev = inReal[j] - shift;
      dev2 = dev * dev;
      total1 += dev;
      total2 += dev2;
      total3 += dev2 * dev;
   }

   i = startIdx;
   outIdx = 0;
   barsSinceReseed = reseedPeriod;
   peak2 = total2;
   do
   {
      dev = inReal[i] - shift;
      dev2 = dev * dev;
      total1 += dev;
      total2 += dev2;
      total3 += dev2 * dev;
      if( total2 > peak2 )
         peak2 = total2;

      residue = total1 * invPeriod;
      residueSq = residue * residue;
      moment2 = total2 - dPeriod * residueSq;
      moment3 = total3
      - 3.0 * residue * total2
      + 2.0 * dPeriod * residueSq * residue;

      /* Each bar rounds the cubic sum by at most eps times the peak second
       * sum to the power 3/2, so G1's error grows as (peak/M2)^(3/2), at most
       * 1000x before the rebuild at 1% of the peak.
       */
      barsSinceReseed--;
      if( moment2 < 0.01 * peak2 || barsSinceReseed <= 0 )
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
         for( j=windowStart; j <= i; j++ )
         {
            dev = inReal[j] - shift;
            dev2 = dev * dev;
            total1 += dev;
            total2 += dev2;
            total3 += dev2 * dev;
         }

         residue = total1 * invPeriod;
         residueSq = residue * residue;
         moment2 = total2 - dPeriod * residueSq;
         moment3 = total3
         - 3.0 * residue * total2
         + 2.0 * dPeriod * residueSq * residue;

         /* The left-to-right sum can miss the mean by more than the window's
          * spread, and the trigger would then fire again on every bar. Adding
          * the measured miss back lands within about an ulp of the mean, where
          * a flat window's deviations are exactly 0. Keep this threshold the
          * trigger's.
          */
         if( moment2 < 0.01 * total2 )
         {
            shift += residue;

            total1 = 0.0;
            total2 = 0.0;
            total3 = 0.0;
            for( j=windowStart; j <= i; j++ )
            {
               dev = inReal[j] - shift;
               dev2 = dev * dev;
               total1 += dev;
               total2 += dev2;
               total3 += dev2 * dev;
            }

            residue = total1 * invPeriod;
            residueSq = residue * residue;
            moment2 = total2 - dPeriod * residueSq;
            moment3 = total3
            - 3.0 * residue * total2
            + 2.0 * dPeriod * residueSq * residue;
         }
         peak2 = total2;
      }

      /* A window of equal values is symmetric, and reaches here with every
       * deviation exactly 0. The bound is the smallest normal double, not a
       * scale: a denominator under it comes with cubed deviations that have
       * lost their precision in the subnormals.
       */
      denominator = moment2 * sqrt(moment2);
      if( denominator < 2.2250738585072014e-308 )
         skewness = 0.0;
      else
         skewness = coef * moment3 / denominator;

      /* inReal and outReal may be the same buffer: read the trailing value
       * before the output write.
       */
      dev = inReal[trailingIdx] - shift;
      dev2 = dev * dev;
      total1 -= dev;
      total2 -= dev2;
      total3 -= dev2 * dev;
      trailingIdx++;

      outReal[outIdx++] = skewness;
      i++;
   } while( i <= endIdx );

   *outNBElement = outIdx;
   *outBegIdx = startIdx;

   return TA_SUCCESS;
}
