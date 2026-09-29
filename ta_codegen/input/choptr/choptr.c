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
 *  092926 MF,CC  Initial version (#469).
 */

int choptr_lookback(int optInTimePeriod)
{
   return optInTimePeriod;
}

TA_RetCode choptr(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int today, outIdx, lookbackTotal, i;
   double highest, lowest, sumTR, logPeriod;
   double tempHT, tempLT, prevClose, trueHigh, trueLow;

   *outBegIdx = 0;
   *outNBElement = 0;

   lookbackTotal = choptr_lookback( optInTimePeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
      return TA_SUCCESS;

   logPeriod = log10( (double)optInTimePeriod );

   outIdx = 0;
   today = startIdx;
   while( today <= endIdx )
   {
      /* Re-summed oldest to newest every bar, never a running total: a
       * running total leaves a residue on a window of zero true ranges, which
       * the exact guard below would then read as a trend.
       */
      highest = inHigh[today];
      lowest = inLow[today];
      sumTR = 0.0;
      prevClose = 0.0;
      for( i = optInTimePeriod; i >= 0; i-- )
      {
         if( i < optInTimePeriod )
         {
            tempHT = inHigh[today-i];
            tempLT = inLow[today-i];
            trueHigh = tempHT;
            if( prevClose > trueHigh )
               trueHigh = prevClose;
            trueLow = tempLT;
            if( prevClose < trueLow )
               trueLow = prevClose;
            sumTR += trueHigh - trueLow;
            if( trueHigh > highest )
               highest = trueHigh;
            if( trueLow < lowest )
               lowest = trueLow;
         }
         prevClose = inClose[today-i];
      }

      /* The seeds are the raw extrema of today's bar, which its own true
       * high and low always enclose, so they never outrun the true box.
       *
       * Exact test, never an epsilon band (issue #253). Every true range lies
       * inside the box, so a positive sum implies a positive box. Keep the
       * "<=" form: a NaN sum must fall through to the log, not read as 100.
       * Keep the parenthesised quotient: it returns exactly 100 whenever the
       * computed ratio is exactly the period.
       */
      if( sumTR <= 0.0 )
         outReal[outIdx] = 100.0;
      else
         outReal[outIdx] = 100.0 * (log10( sumTR / (highest - lowest) ) / logPeriod);

      outIdx++;
      today++;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
