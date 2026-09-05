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
 *  090426 MF,CC  Initial version (#369).
 */

int percentrank_lookback(int optInTimePeriod)
{
   return optInTimePeriod;
}

TA_RetCode percentrank(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int today, outIdx, lookbackTotal, i, count;
   double current;

   *outBegIdx = 0;
   *outNBElement = 0;

   lookbackTotal = percentrank_lookback( optInTimePeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
      return TA_SUCCESS;

   outIdx = 0;
   today = startIdx;
   while( today <= endIdx )
   {
      current = inReal[today];
      count = 0;
      for( i = optInTimePeriod; i >= 1; i-- )
      {
         if( inReal[today-i] < current )
            count++;
      }

      /* Divide, then scale. (count/N)*100 and 100*count/N are different
       * doubles and the reference implementations round the first way. */
      outReal[outIdx] = ((double)count / (double)optInTimePeriod) * 100.0;

      outIdx++;
      today++;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
