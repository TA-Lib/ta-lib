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
 *  100626 MF,CC  Batch tier: block scan of the channel (#485).
 */

int fisher_lookback(int optInTimePeriod)
{
   /* The seed reaches the output through the smoothing pole alone, 0.67 per
    * bar whatever the period: 2.5 bars per e-fold. The 9 e-folds added to K
    * are the gain between that pole and the output: the transform's
    * derivative, which the clamp caps at 500, times 5.9 for the convolution
    * with its own 0.5 pole.
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
   CIRCBUF_PROLOG(sufHighest,double,30);
   CIRCBUF_PROLOG(preHighest,double,30);
   CIRCBUF_PROLOG(sufLowest,double,30);
   CIRCBUF_PROLOG(preLowest,double,30);
   double price, highest, lowest, ratio, smoothed, fish, prevFish, tempReal;
   double tempHigh, tempLow;
   int outIdx, lookbackTotal, unstablePeriod, nbInitialElementNeeded;
   int today, i, m, blockStart, blockNext, nAvail;

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

   /* Same values as fisher_ALT1 below, which carries the formula. Only the
    * channel differs: a Van Herk / Gil-Werman block scan (WILLR's, issue
    * #147) over the midpoints, so the cost per bar does not depend on the
    * period or on the shape of the input, where the cached extremum of
    * fisher_ALT1 rescans its whole window on every bar of a flat or
    * trending stretch. Every scratch array holds copies, so an output may
    * alias an input.
    */
   smoothed = 0.0;
   prevFish = 0.0;

   today = startIdx - unstablePeriod;
   blockStart = today - lookbackTotal;
   outIdx = 0;

   CIRCBUF_INIT( sufHighest, double, optInTimePeriod );
   CIRCBUF_INIT( preHighest, double, optInTimePeriod );
   CIRCBUF_INIT( sufLowest, double, optInTimePeriod );
   CIRCBUF_INIT( preLowest, double, optInTimePeriod );

   while( today <= endIdx )
   {
      /* Suffix extrema of the block [blockStart, today]. */
      i = today;
      tempHigh = inHigh[i];
      tempLow = inLow[i];
      highest = (tempHigh + tempLow) / 2.0;
      lowest = highest;
      sufHighest[optInTimePeriod - 1] = highest;
      sufLowest[optInTimePeriod - 1] = lowest;
      TA_UNROLL(4)
      while( i > blockStart )
      {
         i--;
         tempHigh = inHigh[i];
         tempLow = inLow[i];
         tempReal = (tempHigh + tempLow) / 2.0;
         if( tempReal > highest )
         {
            highest = tempReal;
         }
         if( tempReal < lowest )
         {
            lowest = tempReal;
         }
         sufHighest[i - blockStart] = highest;
         sufLowest[i - blockStart] = lowest;
      }

      /* Prefix extrema of the next block, clamped to what remains, stored
       * one slot up: slot 0 repeats the suffix so that bar 'today', whose
       * window is the block itself, runs the same combine as the others.
       */
      blockNext = blockStart + optInTimePeriod;
      nAvail = endIdx + 1 - blockNext;
      if( nAvail > optInTimePeriod - 1 )
      {
         nAvail = optInTimePeriod - 1;
      }
      preHighest[0] = sufHighest[0];
      preLowest[0] = sufLowest[0];
      if( nAvail > 0 )
      {
         tempHigh = inHigh[blockNext];
         tempLow = inLow[blockNext];
         highest = (tempHigh + tempLow) / 2.0;
         lowest = highest;
         preHighest[1] = highest;
         preLowest[1] = lowest;
         i = 1;
         TA_UNROLL(4)
         while( i < nAvail )
         {
            tempHigh = inHigh[blockNext + i];
            tempLow = inLow[blockNext + i];
            tempReal = (tempHigh + tempLow) / 2.0;
            if( tempReal > highest )
            {
               highest = tempReal;
            }
            if( tempReal < lowest )
            {
               lowest = tempReal;
            }
            preHighest[i + 1] = highest;
            preLowest[i + 1] = lowest;
            i++;
         }
      }

      m = 0;
      while( m <= nAvail )
      {
         highest = sufHighest[m];
         if( preHighest[m] > highest )
         {
            highest = preHighest[m];
         }
         lowest = sufLowest[m];
         if( preLowest[m] < lowest )
         {
            lowest = preLowest[m];
         }

         tempHigh = inHigh[today + m];
         tempLow = inLow[today + m];
         price = (tempHigh + tempLow) / 2.0;

         tempReal = highest - lowest;
         if( !TA_IS_ZERO_SCALED( tempReal, fabs(highest) + fabs(lowest) ) )
            ratio = (price - lowest) / tempReal;
         else
            ratio = 0.5;

         smoothed = 0.33 * 2.0 * (ratio - 0.5) + 0.67 * smoothed;

         if( smoothed > 0.99 )
            smoothed = 0.999;
         if( smoothed < -0.99 )
            smoothed = -0.999;

         fish = 0.5 * log( (1.0 + smoothed) / (1.0 - smoothed) ) + 0.5 * prevFish;

         if( today + m >= startIdx )
         {
            outFisher[outIdx] = fish;
            outTrigger[outIdx] = prevFish;
            outIdx = outIdx + 1;
         }

         prevFish = fish;
         m++;
      }

      today = today + nAvail + 1;
      blockStart = blockNext;
   }

   CIRCBUF_DESTROY(sufHighest);
   CIRCBUF_DESTROY(preHighest);
   CIRCBUF_DESTROY(sufLowest);
   CIRCBUF_DESTROY(preLowest);

   *outNBElement = outIdx;
   *outBegIdx = startIdx;

   return TA_SUCCESS;
}

/* PRAGMA TA_ALT={STREAM,ALL_LANGUAGES} the block scan cannot be a per-bar automaton */
/* PRAGMA TA_ALT={BATCH,JAVA} HotSpot keeps the branch of a double min/max, and a midpoint has no bit key */
TA_RetCode fisher_ALT1(int startIdx, int endIdx,
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
