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
 *  090426 MF,CC  Initial version (#346).
 */

int vhf_lookback(int optInTimePeriod)
{
   return optInTimePeriod;
}

TA_RetCode vhf(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int today, outIdx, lookbackTotal, i;
   double highest, lowest, sumChange, prev, tempReal;

   *outBegIdx = 0;
   *outNBElement = 0;

   lookbackTotal = vhf_lookback( optInTimePeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
      return TA_SUCCESS;

   outIdx = 0;
   today = startIdx;
   while( today <= endIdx )
   {
      /* The two windows are NOT co-terminal: the extrema span the
       * optInTimePeriod newest closes, while the optInTimePeriod changes reach
       * one bar further back. That is what makes the lookback optInTimePeriod
       * rather than optInTimePeriod-1.
       */
      highest = inReal[today];
      lowest = highest;
      prev = highest;
      sumChange = 0.0;
      for( i = optInTimePeriod; i >= 0; i-- )
      {
         tempReal = inReal[today-i];
         if( i < optInTimePeriod )
         {
            sumChange += fabs( tempReal - prev );
            if( tempReal > highest )
               highest = tempReal;
            if( tempReal < lowest )
               lowest = tempReal;
         }
         prev = tempReal;
      }

      /* A fresh sum of non-negative magnitudes is exactly zero only on an
       * exactly flat window, which forces highest-lowest to zero too. Guard on
       * exact zero, never an epsilon band: a band carries the quote unit and
       * zeroes the filter for anything priced under it (issue #253).
       */
      if( sumChange > 0.0 )
         outReal[outIdx] = (highest - lowest) / sumChange;
      else
         outReal[outIdx] = 0.0;

      outIdx++;
      today++;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
