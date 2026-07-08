/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120802 MF   Template creation.
 *
 */

int sum_lookback(int optInTimePeriod)
{
   return optInTimePeriod-1;
}

TA_RetCode sum(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double periodTotal, tempReal;
   int i, outIdx, trailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */
   lookbackTotal = (optInTimePeriod-1);

   /* Move up the start index if there is not
    * enough initial data.
    */
   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* Do the MA calculation using tight loops. */

   /* Add-up the initial period, except for the last value. */
   periodTotal = 0;
   trailingIdx = startIdx-lookbackTotal;

   i=trailingIdx;
   if( optInTimePeriod > 1 )
   {
      while( i < startIdx )
         periodTotal += inReal[i++];
   }

   /* Proceed with the calculation for the requested range.
    * Note that this algorithm allows the inReal and
    * outReal to be the same buffer.
    */
   outIdx = 0;
   do
   {
      periodTotal += inReal[i++];
      tempReal = periodTotal;
      periodTotal -= inReal[trailingIdx++];
      outReal[outIdx++] = tempReal;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
