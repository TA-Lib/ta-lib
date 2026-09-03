/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090526 KL     First version (proposal-drafts #38).
 */

int donchian_lookback(int optInTimePeriod, int optInLag)
{
   /* The window ends optInLag bars behind the output bar, so the first
    * bar with a full window behind it is (optInTimePeriod-1)+optInLag.
    */
   return (optInTimePeriod-1) + optInLag;
}

TA_RetCode donchian(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   int optInTimePeriod,
   int optInLag,
   int *outBegIdx, int *outNBElement,
   double outRealUpperBand[],
   double outRealMiddleBand[],
   double outRealLowerBand[])
{
   double lowest, highest, tmpLow, tmpHigh;
   int outIdx, nbInitialElementNeeded;
   int trailingIdx, lowestIdx, highestIdx, today, winEnd, i;

   /* Donchian Channels over the window [today-optInLag-optInTimePeriod+1 ..
    * today-optInLag]:
    *
    *    Upper  = Highest High of the window
    *    Lower  = Lowest  Low  of the window
    *    Middle = (Upper + Lower) / 2
    *
    * The default optInLag=1 is Donchian's original rule: the bar being
    * evaluated is measured against a window it is NOT part of, which is
    * what lets price cross the band. optInLag=0 includes the current bar
    * (the TradingView/NinjaTrader/pandas form), making Upper/Lower/Middle
    * exactly MAX(high,N)/MIN(low,N)/MIDPRICE(N).
    *
    * The middle line is the Donchian centerline, not a moving average:
    * at optInLag=0 it is bit-identical to MIDPRICE.
    */

   /* Identify the minimum number of price bar needed
    * to identify at least one output over the specified
    * period.
    */
   nbInitialElementNeeded = (optInTimePeriod-1) + optInLag;

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
    * out of it (same approach as MIN/MAX/WILLR and the MIDPRICE streaming
    * tier). The window is the one ending optInLag bars behind the output
    * bar, so the scan cursor is winEnd, not today.
    */
   outIdx = 0;
   today       = startIdx;
   winEnd      = today - optInLag;
   trailingIdx = winEnd - (optInTimePeriod-1);

   highestIdx  = -1;
   highest     = 0.0;
   lowestIdx   = -1;
   lowest      = 0.0;

   while( today <= endIdx )
   {
      tmpHigh = inHigh[winEnd];
      tmpLow  = inLow[winEnd];

      if( highestIdx < trailingIdx )
      {
         highestIdx = trailingIdx;
         highest = inHigh[highestIdx];
         i = highestIdx;
         TA_UNROLL(4)
         while( ++i<=winEnd )
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
         highestIdx = winEnd;
         highest = tmpHigh;
      }

      if( lowestIdx < trailingIdx )
      {
         lowestIdx = trailingIdx;
         lowest = inLow[lowestIdx];
         i = lowestIdx;
         TA_UNROLL(4)
         while( ++i<=winEnd )
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
         lowestIdx = winEnd;
         lowest = tmpLow;
      }

      outRealUpperBand[outIdx]  = highest;
      outRealLowerBand[outIdx]  = lowest;
      outRealMiddleBand[outIdx] = (highest+lowest)/2.0;
      outIdx++;
      trailingIdx++;
      winEnd++;
      today++;
   }

   /* Keep the outBegIdx relative to the
    * caller input before returning.
    */
   *outBegIdx    = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
