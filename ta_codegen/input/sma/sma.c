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
 *  112400 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *
 */

int sma_lookback(int optInTimePeriod)
{
   return optInTimePeriod - 1;
}

TA_RetCode sma(int startIdx, int endIdx, const double *inReal, int optInTimePeriod, int *outBegIdx, int *outNBElement, double *outReal)
{
   double periodTotal;
   double tempReal;
   size_t i;
   size_t outIdx;
   size_t trailingIdx;
   size_t lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */
   lookbackTotal = (size_t)(optInTimePeriod-1);

   /* Move up the start index if there is not
    * enough initial data.
    */
   if( startIdx < lookbackTotal ) {
      startIdx = lookbackTotal;
   }

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* Do the MA calculation using tight loops. */
   /* Add-up the initial period, except for the last value. */
   periodTotal = 0.0;
   trailingIdx = startIdx - lookbackTotal;

   i = trailingIdx;
   if( optInTimePeriod > 1 )
   {
      while( i < startIdx ) {
         periodTotal += (double)(inReal[i]);
         i = i + 1;
      }
   }

   /* Proceed with the calculation for the requested range.
    * Note that this algorithm allows the inReal and
    * outReal to be the same buffer.
    */
   outIdx = 0;
   while( i <= endIdx )
   {
      periodTotal += (double)(inReal[i]);
      i = i + 1;
      tempReal = periodTotal;
      periodTotal -= (double)(inReal[trailingIdx]);
      trailingIdx = trailingIdx + 1;
      outReal[outIdx] = tempReal / (double)optInTimePeriod;
      outIdx = outIdx + 1;
   }

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
