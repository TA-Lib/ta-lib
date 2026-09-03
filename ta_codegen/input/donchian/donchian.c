/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin
 *  MF       Mario Fortier
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090526 KL     First version (issue #342).
 *  090326 MF     Drop optInLag; the window ends at the current bar.
 */

int donchian_lookback(int optInTimePeriod)
{
   return optInTimePeriod-1;
}

TA_RetCode donchian(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outRealUpperBand[],
   double outRealMiddleBand[],
   double outRealLowerBand[])
{
   double lowest, highest, tmpLow, tmpHigh;
   int outIdx, nbInitialElementNeeded;
   int trailingIdx, lowestIdx, highestIdx, today, i;

   /* Donchian Channels over the optInTimePeriod bars ending at the current
    * bar:
    *
    *    Upper  = Highest High of the window
    *    Lower  = Lowest  Low  of the window
    *    Middle = (Upper + Lower) / 2
    *
    * The window includes the current bar, matching every other library and
    * charting platform. A breakout rule compares the current bar against the
    * PREVIOUS bar's band, which is where the one-bar offset belongs.
    *
    * Upper/Middle/Lower are bit-identical to MAX(high,N)/MIDPRICE(N)/MIN(low,N).
    */

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
    * output to be the same buffer: every position written (outIdx) sits
    * at or below trailingIdx, the oldest position any later bar reads.
    *
    * The highest high and lowest low of the window are cached with their
    * indices; the window is rescanned only when a cached extremum drops
    * out of it (same approach as MIN/MAX/WILLR and MIDPRICE).
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
      tmpHigh = inHigh[today];
      tmpLow  = inLow[today];

      if( highestIdx < trailingIdx )
      {
         highestIdx = trailingIdx;
         highest = inHigh[highestIdx];
         i = highestIdx;
         TA_UNROLL(4)
         while( ++i<=today )
         {
            tmpHigh = inHigh[i];
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
         lowest = inLow[lowestIdx];
         i = lowestIdx;
         TA_UNROLL(4)
         while( ++i<=today )
         {
            tmpLow = inLow[i];
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

      outRealUpperBand[outIdx]  = highest;
      outRealLowerBand[outIdx]  = lowest;
      outRealMiddleBand[outIdx] = (highest+lowest)/2.0;
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
