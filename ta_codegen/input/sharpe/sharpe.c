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
 *  110626 MF,CC  Initial version (#571).
 */

int sharpe_lookback(int optInTimePeriod,
   double optInRiskFreeRate,
   double optInAnnualization)
{
   (void)optInRiskFreeRate;
   (void)optInAnnualization;

   return optInTimePeriod;
}

TA_RetCode sharpe(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   double optInRiskFreeRate,
   double optInAnnualization,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double shift, periodTotal1, periodTotal2, peakTotal2;
   double price, prevPrice, trailingPrevPrice, windowPrevPrice, ret, dev;
   double dPeriod, invPeriod, meanDev, moment2, excess, sqrtAnnual, ratio;
   int i, j, outIdx, trailingIdx, windowStart, barsSinceReseed;

   if( startIdx < optInTimePeriod )
      startIdx = optInTimePeriod;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   dPeriod = (double)optInTimePeriod;
   invPeriod = 1.0 / dPeriod;
   sqrtAnnual = sqrt(optInAnnualization);

   /* The window holds the returns of the bars after trailingIdx up to i; a
    * bar's return reads the price before it.
    *
    * The variance of the excess returns is the variance of the returns, so
    * the risk-free rate enters only the mean. That keeps the sums at return
    * scale whatever the rate, and lets a flat price window reach an exact
    * zero spread.
    */
   trailingIdx = startIdx - optInTimePeriod;
   prevPrice = inReal[trailingIdx];

   /* A return needs a non-zero base price and nothing more: the test is
    * exact, never a band, since a price carries the quote unit. A zero base
    * gives a zero return.
    */
   price = inReal[trailingIdx+1];
   ret = 0.0;
   if( prevPrice != 0.0 )
      ret = (price-prevPrice)/prevPrice;
   shift = ret;

   periodTotal1 = 0.0;
   periodTotal2 = 0.0;
   for( j=trailingIdx+1; j < startIdx; j++ )
   {
      price = inReal[j];
      ret = 0.0;
      if( prevPrice != 0.0 )
         ret = (price-prevPrice)/prevPrice;
      prevPrice = price;
      dev = ret - shift;
      periodTotal1 += dev;
      periodTotal2 += dev * dev;
   }

   i = startIdx;
   outIdx = 0;
   barsSinceReseed = 32 * optInTimePeriod;
   peakTotal2 = periodTotal2;
   do
   {
      price = inReal[i];
      ret = 0.0;
      if( prevPrice != 0.0 )
         ret = (price-prevPrice)/prevPrice;
      prevPrice = price;
      dev = ret - shift;
      periodTotal1 += dev;
      periodTotal2 += dev * dev;
      peakTotal2 = ( periodTotal2 > peakTotal2 ) ? periodTotal2 : peakTotal2;

      meanDev = periodTotal1 * invPeriod;
      moment2 = periodTotal2 - meanDev * periodTotal1;

      /* inReal and outReal may be the same buffer, and the trailing prices
       * sit in slots the output reaches: every read of the window happens
       * before this bar's write. The oldest price read is the trailing
       * return's base, so the index stays on it.
       */
      trailingPrevPrice = inReal[trailingIdx];
      trailingIdx++;
      price = inReal[trailingIdx];
      ret = 0.0;
      if( trailingPrevPrice != 0.0 )
         ret = (price-trailingPrevPrice)/trailingPrevPrice;
      dev = ret - shift;
      periodTotal1 -= dev;
      periodTotal2 -= dev * dev;

      /* Rebuild against the window mean when the spread has shrunk below 1% of
       * the largest sum of squares held since the last rebuild, or at least
       * every 32 windows. The rounding the running sums carry scales with that
       * peak, so once a large return leaves, the current sum can be all
       * rounding; a flat run after a move must still come out exactly flat.
       * The spread is a divisor, so its relative error reaches the ratio.
       */
      barsSinceReseed--;
      if( moment2 < 0.01 * peakTotal2
         || barsSinceReseed <= 0 )
      {
         barsSinceReseed = 32 * optInTimePeriod;

         windowStart = i - optInTimePeriod + 1;

         windowPrevPrice = inReal[windowStart-1];
         dev = 0.0;
         for( j=windowStart; j <= i; j++ )
         {
            price = inReal[j];
            ret = 0.0;
            if( windowPrevPrice != 0.0 )
               ret = (price-windowPrevPrice)/windowPrevPrice;
            windowPrevPrice = price;
            dev += ret;
         }
         shift = dev * invPeriod;

         windowPrevPrice = inReal[windowStart-1];
         periodTotal1 = 0.0;
         periodTotal2 = 0.0;
         for( j=windowStart; j <= i; j++ )
         {
            price = inReal[j];
            ret = 0.0;
            if( windowPrevPrice != 0.0 )
               ret = (price-windowPrevPrice)/windowPrevPrice;
            windowPrevPrice = price;
            dev = ret - shift;
            periodTotal1 += dev;
            periodTotal2 += dev * dev;
         }

         meanDev = periodTotal1 * invPeriod;
         moment2 = periodTotal2 - meanDev * periodTotal1;

         /* A window of equal returns can sit off this mean by rounding, which
          * leaves a spread of rounding where the ratio must be the plain
          * quotient, and fires the trigger again on every bar. Anchored on
          * one of its own returns, its deviations are exactly zero.
          */
         if( moment2 < 0.000001 * periodTotal2 )
         {
            shift = ret;
            windowPrevPrice = inReal[windowStart-1];
            periodTotal1 = 0.0;
            periodTotal2 = 0.0;
            for( j=windowStart; j <= i; j++ )
            {
               price = inReal[j];
               ret = 0.0;
               if( windowPrevPrice != 0.0 )
                  ret = (price-windowPrevPrice)/windowPrevPrice;
               windowPrevPrice = price;
               dev = ret - shift;
               periodTotal1 += dev;
               periodTotal2 += dev * dev;
            }
            meanDev = periodTotal1 * invPeriod;
            moment2 = periodTotal2 - meanDev * periodTotal1;
         }
         peakTotal2 = periodTotal2;

         /* Re-remove the trailing return under the new shift so the carried
          * state matches the path without a rebuild.
          */
         price = inReal[windowStart];
         ret = 0.0;
         if( inReal[windowStart-1] != 0.0 )
            ret = (price-inReal[windowStart-1])/inReal[windowStart-1];
         dev = ret - shift;
         periodTotal1 -= dev;
         periodTotal2 -= dev * dev;
      }

      /* No spread is the plain quotient: +/-Inf by the sign of the excess
       * mean, NaN when it is exactly zero (ruled on #571). Annualize the
       * per-bar ratio last, so an annualization of 1 is that ratio bit for
       * bit.
       */
      excess = (shift + meanDev) - optInRiskFreeRate;
      ratio = excess / sqrt(moment2 / (dPeriod-1.0));

      outReal[outIdx++] = sqrtAnnual * ratio;
      i++;
   } while( i <= endIdx );

   *outNBElement = outIdx;
   *outBegIdx = startIdx;

   return TA_SUCCESS;
}
