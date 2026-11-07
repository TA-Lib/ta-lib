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
 *  110726 MF,CC  Initial version (#572).
 */

int sortino_lookback(int optInTimePeriod,
   double optInMAR,
   double optInAnnualization)
{
   (void)optInMAR;
   (void)optInAnnualization;

   return optInTimePeriod;
}

TA_RetCode sortino(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   double optInMAR,
   double optInAnnualization,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double periodTotal, downTotal, absTotal, peakDown, peakAbs;
   double windowTotal, windowDown, windowAbs;
   double price, prevPrice, trailingPrevPrice, windowPrevPrice, ret, dev;
   double invPeriod, sqrtAnnual, excess, ratio;
   int i, j, outIdx, trailingIdx, windowStart, barsSinceReseed;
   int nbBelow, nbAbove, windowBelow, windowAbove;

   if( startIdx < optInTimePeriod )
      startIdx = optInTimePeriod;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   invPeriod = 1.0 / (double)optInTimePeriod;
   sqrtAnnual = sqrt(optInAnnualization);

   /* The window holds the returns of the bars after trailingIdx up to i; a
    * bar's return reads the price before it. A return needs a non-zero base
    * price and nothing more: a zero base gives a zero return.
    *
    * The counts of returns below and above the MAR are exact, so they, not
    * the running sums, decide when the window has no downside.
    */
   trailingIdx = startIdx - optInTimePeriod;
   prevPrice = inReal[trailingIdx];

   periodTotal = 0.0;
   downTotal = 0.0;
   absTotal = 0.0;
   nbBelow = 0;
   nbAbove = 0;
   for( j=trailingIdx+1; j < startIdx; j++ )
   {
      price = inReal[j];
      ret = 0.0;
      if( prevPrice != 0.0 )
         ret = (price-prevPrice)/prevPrice;
      prevPrice = price;
      periodTotal += ret;
      absTotal += fabs(ret);
      dev = ret - optInMAR;
      if( dev < 0.0 )
      {
         downTotal += dev * dev;
         nbBelow++;
      }
      else if( dev > 0.0 )
         nbAbove++;
   }

   i = startIdx;
   outIdx = 0;
   barsSinceReseed = 32 * optInTimePeriod;
   peakDown = downTotal;
   peakAbs = absTotal;
   do
   {
      price = inReal[i];
      ret = 0.0;
      if( prevPrice != 0.0 )
         ret = (price-prevPrice)/prevPrice;
      prevPrice = price;
      periodTotal += ret;
      absTotal += fabs(ret);
      dev = ret - optInMAR;
      if( dev < 0.0 )
      {
         downTotal += dev * dev;
         nbBelow++;
      }
      else if( dev > 0.0 )
         nbAbove++;
      peakDown = ( downTotal > peakDown ) ? downTotal : peakDown;
      peakAbs = ( absTotal > peakAbs ) ? absTotal : peakAbs;

      windowTotal = periodTotal;
      windowDown = downTotal;
      windowAbs = absTotal;
      windowBelow = nbBelow;
      windowAbove = nbAbove;

      /* inReal and outReal may be the same buffer, and the trailing prices
       * sit in slots the output reaches: every read of the window happens
       * before this bar's write.
       */
      trailingPrevPrice = inReal[trailingIdx];
      trailingIdx++;
      price = inReal[trailingIdx];
      ret = 0.0;
      if( trailingPrevPrice != 0.0 )
         ret = (price-trailingPrevPrice)/trailingPrevPrice;
      periodTotal -= ret;
      absTotal -= fabs(ret);
      dev = ret - optInMAR;
      if( dev < 0.0 )
      {
         downTotal -= dev * dev;
         nbBelow--;
      }
      else if( dev > 0.0 )
         nbAbove--;

      /* Rebuild when the downside sum, or the sum of absolute returns, has
       * shrunk below 1% of the largest one held since the last rebuild, or at
       * least every 32 windows. The rounding a running sum carries scales
       * with that peak, so once a large return leaves, the mean or the
       * divisor can be all rounding.
       */
      barsSinceReseed--;
      if( windowDown < 0.01 * peakDown
         || windowAbs < 0.01 * peakAbs
         || barsSinceReseed <= 0 )
      {
         barsSinceReseed = 32 * optInTimePeriod;

         windowStart = i - optInTimePeriod + 1;
         windowPrevPrice = inReal[windowStart-1];
         windowTotal = 0.0;
         windowDown = 0.0;
         windowAbs = 0.0;
         for( j=windowStart; j <= i; j++ )
         {
            price = inReal[j];
            ret = 0.0;
            if( windowPrevPrice != 0.0 )
               ret = (price-windowPrevPrice)/windowPrevPrice;
            windowPrevPrice = price;
            windowTotal += ret;
            windowAbs += fabs(ret);
            dev = ret - optInMAR;
            if( dev < 0.0 )
               windowDown += dev * dev;
         }
         peakDown = windowDown;
         peakAbs = windowAbs;

         /* Re-remove the trailing return so the carried state matches the
          * path without a rebuild; the counts never drift.
          */
         price = inReal[windowStart];
         ret = 0.0;
         if( inReal[windowStart-1] != 0.0 )
            ret = (price-inReal[windowStart-1])/inReal[windowStart-1];
         periodTotal = windowTotal - ret;
         absTotal = windowAbs - fabs(ret);
         downTotal = windowDown;
         dev = ret - optInMAR;
         if( dev < 0.0 )
            downTotal -= dev * dev;
      }

      /* No downside is the plain quotient (ruled on #572): the mean excess
       * is then positive exactly when some return beats the MAR, giving
       * +Inf, and zero when every return equals it, giving NaN. The count
       * stands in for the mean so rounding cannot flip either case.
       * Annualize the per-bar ratio last, so an annualization of 1 is that
       * ratio bit for bit.
       */
      excess = windowTotal * invPeriod - optInMAR;
      if( windowBelow == 0 )
      {
         windowDown = 0.0;
         excess = (double)windowAbove;
      }
      ratio = excess / sqrt(windowDown * invPeriod);

      outReal[outIdx++] = sqrtAnnual * ratio;
      i++;
   } while( i <= endIdx );

   *outNBElement = outIdx;
   *outBegIdx = startIdx;

   return TA_SUCCESS;
}
