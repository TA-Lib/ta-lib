/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AC       Angelo Ciceri
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120806 AC   Creation (equal to MAX but outputs index)
 *  100526 MF,CC A tie names the newest bar from any start (#503)
 *
 */

int maxindex_lookback(int optInTimePeriod)
{
   return (optInTimePeriod-1);
}

TA_RetCode maxindex(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double highest, tmp;
   int outIdx, nbInitialElementNeeded;
   int trailingIdx, today, i, highestIdx;

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
    * (The integer output can never share the real input's buffer —
    * different element type; issue #130.)
    */
   outIdx = 0;
   today       = startIdx;
   trailingIdx = startIdx-nbInitialElementNeeded;
   highestIdx  = -1;
   highest     = 0.0;

   while( today <= endIdx )
   {
      tmp = inReal[today];

      if( highestIdx < trailingIdx )
      {
         highestIdx = trailingIdx;
         highest = inReal[highestIdx];
         i = highestIdx;
         TA_UNROLL(4)
         while( ++i<=today )
         {
            tmp = inReal[i];
            if( tmp >= highest )
            {
               highestIdx = i;
               highest = tmp;
            }
         }
      }
      else if( tmp >= highest )
      {
         highestIdx = today;
         highest = tmp;
      }

      outInteger[outIdx++] = highestIdx;
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

/* PRAGMA TA_ALT={BATCH,C} a compiler makes a min/max of this strict rescan, not of one that takes ties */
/* PRAGMA TA_ALT={BATCH,RUST} */
TA_RetCode maxindex_ALT1(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double highest, tmp;
   int outIdx, nbInitialElementNeeded;
   int trailingIdx, today, i, highestIdx;

   nbInitialElementNeeded = (optInTimePeriod-1);

   if( startIdx < nbInitialElementNeeded )
      startIdx = nbInitialElementNeeded;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   outIdx = 0;
   today       = startIdx;
   trailingIdx = startIdx-nbInitialElementNeeded;
   highestIdx  = -1;
   highest     = 0.0;

   while( today <= endIdx )
   {
      tmp = inReal[today];

      if( highestIdx < trailingIdx )
      {
         /* Newest bar first, and only a strictly better bar replaces it: a tie
          * names the newest bar.
          */
         highestIdx = today;
         highest = tmp;
         i = today;
         TA_UNROLL(4)
         while( i > trailingIdx )
         {
            i--;
            tmp = inReal[i];
            if( tmp > highest )
            {
               highestIdx = i;
               highest = tmp;
            }
         }
      }
      else if( tmp >= highest )
      {
         highestIdx = today;
         highest = tmp;
      }

      outInteger[outIdx++] = highestIdx;
      trailingIdx++;
      today++;
   }

   *outBegIdx    = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
