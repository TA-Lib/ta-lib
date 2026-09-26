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
 *  092526 MF,CC  Initial version (#450).
 */

int cg_lookback(int optInTimePeriod)
{
   return optInTimePeriod - 1;
}

TA_RetCode cg(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int today, outIdx, lookbackTotal, i;
   double num, den;

   *outBegIdx = 0;
   *outNBElement = 0;

   lookbackTotal = cg_lookback( optInTimePeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
      return TA_SUCCESS;

   outIdx = 0;
   today = startIdx;
   while( today <= endIdx )
   {
      /* Oldest first, the value i bars ago is in den for the last i+1
       * additions to num, which is its weight in the listing. Walking the
       * window newest first would reverse every weight.
       */
      num = 0.0;
      den = 0.0;
      for( i = optInTimePeriod-1; i >= 0; i-- )
      {
         den += inReal[today-i];
         num += den;
      }

      /* The denominator is a signed sum, so only an exact zero is degenerate.
       * It is answered with the flat-window value, which keeps every output a
       * function of its own window; an epsilon band would carry the quote unit
       * (#253).
       */
      if( den != 0.0 )
         outReal[outIdx] = -num / den;
      else
         outReal[outIdx] = -((double)optInTimePeriod + 1.0) * 0.5;

      outIdx++;
      today++;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
