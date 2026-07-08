/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AC       Angelo Ciceri
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120906 AC   Creation (equal to MINMAX but outputs index)
 *
 */

int minmaxindex_lookback(int optInTimePeriod)
{
   return (optInTimePeriod-1);
}

TA_RetCode minmaxindex(int startIdx, int endIdx, const double inReal[], int optInTimePeriod, int *outBegIdx, int *outNBElement, int outMinIdx[], int outMaxIdx[])
{
   double highest, lowest, tmpHigh, tmpLow;
   int outIdx, nbInitialElementNeeded;
   int trailingIdx, today, i, highestIdx, lowestIdx;

   /* Identify the minimum number of price bar needed
    * to identify at least one output over the specified
    * period.
    */
   nbInitialElementNeeded = (optInTimePeriod-1);

   /* Move up the start index if there is not
    * enough initial data.
    */
   if( startIdx < nbInitialElementNeeded )
      startIdx = nbInitialElementNeeded;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* Proceed with the calculation for the requested range.
    * Note that this algorithm allows the input and
    * output to be the same buffer.
    */
   outIdx = 0;
   today       = startIdx;
   trailingIdx = startIdx-nbInitialElementNeeded;
   highestIdx  = -1;
   highest     = 0.0;
   lowestIdx   = -1;
   lowest      = 0.0;

   while( today <= endIdx )
   {
      tmpLow = tmpHigh = inReal[today];

      if( highestIdx < trailingIdx )
      {
         highestIdx = trailingIdx;
         highest = inReal[highestIdx];
         i = highestIdx;
         while( ++i<=today )
         {
            tmpHigh = inReal[i];
            if( tmpHigh > highest )
            {
               highestIdx = i;
               highest = tmpHigh;
            }
         }
      }
      else if( tmpHigh >= highest )
      {
         highestIdx = today;
         highest = tmpHigh;
      }

      if( lowestIdx < trailingIdx )
      {
         lowestIdx = trailingIdx;
         lowest = inReal[lowestIdx];
         i = lowestIdx;
         while( ++i<=today )
         {
            tmpLow = inReal[i];
            if( tmpLow < lowest )
            {
               lowestIdx = i;
               lowest = tmpLow;
            }
         }
      }
      else if( tmpLow <= lowest )
      {
         lowestIdx = today;
         lowest = tmpLow;
      }

      outMaxIdx[outIdx] = highestIdx;
      outMinIdx[outIdx] = lowestIdx;
      outIdx++;
      trailingIdx++;
      today++;
   }

   /* Keep the outBegIdx relative to the
    * caller input before returning.
    */
   *outBegIdx    = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
