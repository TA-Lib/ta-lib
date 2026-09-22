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
 *  091526 KL     First version (proposal-drafts issue #74).
 *  092126 MF,CC  Rebuild against the peak sum of squares (issue #430).
 *  092226 MF,CC  #434 branch-free peak update.
 */

int cti_lookback(int optInTimePeriod)
{
   return optInTimePeriod-1;
}

TA_RetCode cti(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double sumX, sumX2, sumXY, x, trailingX, peakX2;
   double shift, ssX, spXY, tempReal, invPeriod, dPeriod, sumY, ssY;
   int lookbackTotal, outIdx, today, trailingIdx, windowStart, j;
   int barsSinceReseed;

   lookbackTotal = optInTimePeriod-1;

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   *outBegIdx  = startIdx;
   trailingIdx = startIdx - lookbackTotal;

   dPeriod   = (double)optInTimePeriod;
   invPeriod = 1.0 / dPeriod;

   /* The ramp side is data-independent and collapses to two constants. With y
    * running 0..optInTimePeriod-1 as BARS AGO,
    *
    *    sumY = n(n-1)/2            ssY = sumY2 - sumY*sumY/n = n(n*n-1)/12
    *
    * Both are computed in double because their integer forms overflow: at the
    * top of the parameter range n*n*n is 1e15, far past what an int holds, and
    * the product would wrap silently rather than fail. ssY is strictly positive
    * for every n >= 2, which is why only the price side can make a window
    * degenerate.
    */
   sumY = dPeriod * (dPeriod-1.0) * 0.5;
   ssY  = dPeriod * (dPeriod*dPeriod - 1.0) / 12.0;

   /* Measure the price side against a shift near the window, as correl.c does
    * (#242): n*Sxx - Sx*Sx on raw price levels, as the author's listing writes
    * it, cancels catastrophically once the spread is small against the level.
    *
    * Anchor on the first window value here; a rebuild anchors on the window
    * mean instead (or on inReal[today] when the mean leaves the window
    * flat), which is better centred but costs a pass this one cannot afford
    * before the sums exist.
    */
   shift = inReal[trailingIdx];

   /* The initial window, less its last bar. This bar's y is startIdx-j, since
    * the first output is computed with `today` at startIdx.
    */
   sumX = sumX2 = sumXY = 0.0;
   for( j=trailingIdx; j < startIdx; j++ )
   {
      x = inReal[j] - shift;
      sumX  += x;
      sumX2 += x*x;
      sumXY += x * (double)(startIdx - j);
   }

   today = startIdx;
   outIdx = 0;
   barsSinceReseed = 32 * optInTimePeriod;
   peakX2 = sumX2;

   do
   {
      /* The incoming bar is zero bars ago, so it moves sumX and sumX2 and
       * leaves sumXY alone.
       */
      x = inReal[today] - shift;
      sumX  += x;
      sumX2 += x*x;
      peakX2 = ( sumX2 > peakX2 ) ? sumX2 : peakX2;

      ssX  = sumX2 - ((sumX*sumX)*invPeriod);
      spXY = sumXY - ((sumX*sumY)*invPeriod);

      /* Re-anchor and rebuild when the shift has gone stale: the price sum of
       * squares has shrunk below 1e-6 of the LARGEST sumX2 held since the last
       * rebuild, OR at least every 32 windows. Measure against that peak, not
       * the current sumX2: the rounding the running sums carry scales with the
       * peak, and a series that decays back onto the shift leaves a current
       * sumX2 made of nothing but that rounding. Only the price side is
       * watched -- the ramp side is exact constants and cannot drift.
       *
       * A vanishing spXY is NOT a trigger. It is a legitimate answer, a window
       * with no linear trend, and reseeding on it would rebuild on every bar of
       * ordinary sideways data.
       */
      barsSinceReseed--;
      if( ssX < 0.000001 * peakX2 || barsSinceReseed <= 0 )
      {
         barsSinceReseed = 32 * optInTimePeriod;
         windowStart = today - lookbackTotal;

         tempReal = 0.0;
         for( j=windowStart; j <= today; j++ )
            tempReal += inReal[j];
         shift = tempReal*invPeriod;

         sumX = sumX2 = sumXY = 0.0;
         for( j=windowStart; j <= today; j++ )
         {
            x = inReal[j] - shift;
            sumX  += x;
            sumX2 += x*x;
            sumXY += x * (double)(today - j);
         }

         /* A window flat to within the rounding of its own mean leaves ssX at
          * that rounding, which would fire the trigger again on every bar.
          * Anchored on one of its own values instead, ssX is at least half the
          * squared range and sumX2 at most n times it, so it cannot, short of
          * squares that underflow.
          */
         if( sumX2 - ((sumX*sumX)*invPeriod) < 0.000001 * sumX2 )
         {
            shift = inReal[today];
            sumX = sumX2 = sumXY = 0.0;
            for( j=windowStart; j <= today; j++ )
            {
               x = inReal[j] - shift;
               sumX  += x;
               sumX2 += x*x;
               sumXY += x * (double)(today - j);
            }
         }
         peakX2 = sumX2;

         ssX  = sumX2 - ((sumX*sumX)*invPeriod);
         spXY = sumXY - ((sumX*sumY)*invPeriod);
      }

      /* Save the trailing value before writing the output, since the input and
       * output might be the same array.
       */
      trailingX = inReal[trailingIdx] - shift;
      trailingIdx++;

      /* THE SIGN. y here is BARS AGO, so it runs backward in time and a rising
       * series correlates NEGATIVELY with it. Ehlers' listing counts the same
       * way and takes Y = -count, the positively-sloped line the indicator is
       * defined against; negating the coefficient once is the same thing, and
       * keeps the slide below in its bars-ago form. Without it every value is
       * reversed and every magnitude kept.
       *
       * ssY is a positive constant, so only ssX can make the window degenerate.
       * It is tested against its own scale because an absolute band would
       * reject a well-defined correlation on small-valued data. The product is
       * tested too, since at n = 2 the smallest subnormal ssX rounds it to
       * zero.
       *
       * An all-flat window therefore emits exactly 0.0 rather than NaN, which
       * is correl.c's precedent and what #112 requires of a successful call.
       */
      if( ssX > 0.00000000000001*sumX2 && ssX*ssY > 0.0 )
      {
         tempReal = -spXY / sqrt(ssX*ssY);

         /* A correlation coefficient cannot leave [-1,1]; rounding in the
          * three sums can still put it slightly outside.
          */
         if( tempReal > 1.0 )
            tempReal = 1.0;
         else if( tempReal < -1.0 )
            tempReal = -1.0;
         outReal[outIdx++] = tempReal;
      }
      else
         outReal[outIdx++] = 0.0;

      /* Slide the window one bar in O(1). Advancing it ages every retained
       * value by one bar, which raises each of their weights by 1 and so adds
       * the whole deviation sum; the departing value leaves at the full weight
       * optInTimePeriod-1 and also loses its place in that sum, which is the
       * remaining -1. The two together are exactly -optInTimePeriod*trailingX,
       * and sumX must still be the COMPLETE window's sum when it is read here
       * -- it is decremented on the line below, not above.
       */
      sumXY = sumXY + sumX - dPeriod*trailingX;
      sumX  -= trailingX;
      sumX2 -= trailingX*trailingX;

      today++;
   } while( today <= endIdx );

   *outNBElement = outIdx;

   return TA_SUCCESS;
}
