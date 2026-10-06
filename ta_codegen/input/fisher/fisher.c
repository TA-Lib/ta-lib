/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  100626 KL,CC  Creation (#485).
 */

int fisher_lookback(int optInTimePeriod)
{
   /* The channel needs its own n-1 bars before it has a value. On top of that
    * the two recursions carry a seed that decays rather than ending.
    *
    * The count is DERIVED rather than sampled. The seed reaches the output
    * through the smoothing pole alone -- 0.67 per bar, the same whatever the
    * period, so unlike EMA's this count does not grow with n. Two factors sit
    * between that pole and the output: the transform's derivative 1/(1-v^2),
    * which the clamp caps at 1/(1-0.999^2) = 500 and which is the only reason
    * this is finite at all, and the convolution with the transform's own 0.5
    * pole, worth 1/(0.67-0.5) = 5.9. So the seed's weight is under e^-K once
    *
    *     n >= (K + ln(500*5.9)) / ln(1/0.67) = (K + 8.0) / 0.4005
    *
    * which is 44.9 bars at K = 10 and 67.4 at K = 19. The form below is 2.5
    * bars per e-fold with those 9 e-folds of gain folded in: 48 and 70.
    *
    * A sampled worst case was NOT used. Over 400 input patterns and 22 seeds
    * it reads 33 and 58, and it kept climbing as the sample grew -- 30 and 54
    * at a tenth of it -- which is what a sampled maximum does when it is not
    * a bound.
    */
   return (optInTimePeriod - 1)
   + TA_UNSTABLE( TA_FUNC_UNST_FISHER, (5 * (K + 9) + 1) / 2 );
}

TA_RetCode fisher(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outFisher[], double outTrigger[])
{
   double price, highest, lowest, ratio, smoothed, fish, prevFish, tempReal;
   double tempHigh, tempLow;
   int outIdx, lookbackTotal, unstablePeriod, nbInitialElementNeeded;
   int today, trailingIdx, highestIdx, lowestIdx, i;

   nbInitialElementNeeded = fisher_lookback( optInTimePeriod );

   if( startIdx < nbInitialElementNeeded )
      startIdx = nbInitialElementNeeded;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   lookbackTotal = optInTimePeriod - 1;
   unstablePeriod = nbInitialElementNeeded - lookbackTotal;

   /* John F. Ehlers, "Using The Fisher Transform", Stocks & Commodities
    * V.20:11 (November 2002), pp.40-42, the EasyLanguage listing in Figure 4.
    *
    * The bar's midpoint is located in its rolling n-bar channel, rescaled to
    * (-1, +1), smoothed, clamped, and passed through atanh. What the
    * transform buys is the tail: a channel position is close to uniformly
    * distributed, and the Fisher transform of a uniform variable is close to
    * normal, so an extreme reading is rare rather than routine and a turn is
    * a sharp corner rather than a drift.
    *
    * Both recursions start from the author's zero seed and decay at their own
    * coefficient -- 0.67 for the smoothing, 0.5 for the transform -- so the
    * first bars carry the seed rather than the series. That is what the
    * unstable period discards.
    */
   smoothed = 0.0;
   prevFish = 0.0;

   today = startIdx - unstablePeriod;
   trailingIdx = today - lookbackTotal;
   highestIdx = -1;
   highest = 0.0;
   lowestIdx = -1;
   lowest = 0.0;
   outIdx = 0;

   while( today <= endIdx )
   {
      /* The channel, over the midpoints rather than over the highs and the
       * lows separately: this indicator reads one series, which happens to be
       * (H+L)/2, so both extremes come from that same series. STOCH's shape,
       * with the midpoint recomputed on the rare rescan rather than held in a
       * buffer the caller would have to own.
       */
      price = (inHigh[today] + inLow[today]) / 2.0;

      if( highestIdx < trailingIdx )
      {
         highestIdx = trailingIdx;
         tempHigh = inHigh[highestIdx];
         tempLow = inLow[highestIdx];
         highest = (tempHigh + tempLow) / 2.0;
         i = highestIdx;
         while( ++i <= today )
         {
            tempHigh = inHigh[i];
            tempLow = inLow[i];
            tempReal = (tempHigh + tempLow) / 2.0;
            if( tempReal > highest )
            {
               highestIdx = i;
               highest = tempReal;
            }
         }
      }
      else if( price >= highest )
      {
         highestIdx = today;
         highest = price;
      }

      if( lowestIdx < trailingIdx )
      {
         lowestIdx = trailingIdx;
         tempHigh = inHigh[lowestIdx];
         tempLow = inLow[lowestIdx];
         lowest = (tempHigh + tempLow) / 2.0;
         i = lowestIdx;
         while( ++i <= today )
         {
            tempHigh = inHigh[i];
            tempLow = inLow[i];
            tempReal = (tempHigh + tempLow) / 2.0;
            if( tempReal < lowest )
            {
               lowestIdx = i;
               lowest = tempReal;
            }
         }
      }
      else if( price <= lowest )
      {
         lowestIdx = today;
         lowest = price;
      }

      /* A flat window answers the neutral position rather than dividing by
       * its own zero range. The band is the range against its own two
       * extremes, STOCH's test: a fixed constant answers "flat" for every
       * window of an instrument quoted below it (#253), and an exact test
       * divides a machine-flat window into noise (#107). At 0.5 the bar
       * contributes nothing and both recursions decay on their own
       * coefficients.
       */
      tempReal = highest - lowest;
      if( !TA_IS_ZERO_SCALED( tempReal, fabs(highest) + fabs(lowest) ) )
         ratio = (price - lowest) / tempReal;
      else
         ratio = 0.5;

      /* The listing's .33*2*(r-.5) + .67*Value1[1]. */
      smoothed = 0.33 * 2.0 * (ratio - 0.5) + 0.67 * smoothed;

      /* The clamp is what keeps atanh finite, and the CLAMPED smoothed is what
       * the next bar's smoothing reads -- the listing assigns it back to
       * Value1 rather than holding it for the transform alone.
       */
      if( smoothed > 0.99 )
         smoothed = 0.999;
      if( smoothed < -0.99 )
         smoothed = -0.999;

      fish = 0.5 * log( (1.0 + smoothed) / (1.0 - smoothed) ) + 0.5 * prevFish;

      if( today >= startIdx )
      {
         outFisher[outIdx] = fish;
         /* The author's second plot is Fish[1]: the previous bar's smoothed,
          * which at the first output bar is the zero seed when no unstable
          * period was discarded, and the computed smoothed of the bar before it
          * when one was.
          */
         outTrigger[outIdx] = prevFish;
         outIdx = outIdx + 1;
      }

      prevFish = fish;
      trailingIdx++;
      today++;
   }

   *outNBElement = outIdx;
   *outBegIdx = startIdx;

   return TA_SUCCESS;
}
