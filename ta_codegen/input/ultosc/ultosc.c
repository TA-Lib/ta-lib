/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  DM       Drew McCormack (http://www.trade-strategist.com)
 *  MF       Mario Fortier
 *  DX       Dex Hunter (https://github.com/dexhunter)
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  281206 DM   Initial Implementation
 *  010606 MF   Abstract local arrays. Detect divide by zero.
 *  073126 DX   Evaluate each bar's terms once via a CIRCBUF ring (PR #154).
 */

int ultosc_lookback(int optInTimePeriod1, int optInTimePeriod2, int optInTimePeriod3)
{
   int maxPeriod;

   /* Lookback for the Ultimate Oscillator is the lookback of the SMA with the longest
    * time period, plus 1 for the True Range.
    */
   maxPeriod = max( max(optInTimePeriod1, optInTimePeriod2), optInTimePeriod3);
   return sma_lookback( maxPeriod ) + 1;
}

TA_RetCode ultosc(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int optInTimePeriod1,
   int optInTimePeriod2,
   int optInTimePeriod3,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   /* The two per-bar terms the three moving sums are built from. Both are a
    * pure function of the bar, so each bar is evaluated once on entry and read
    * back when it leaves each of the three windows.
    */
   typedef struct { double closeMinusTrueLow; double trueRange; } UltOscTerm;

   double a1Total, a2Total, a3Total;
   double b1Total, b2Total, b3Total;
   double trueLow, trueRange, closeMinusTrueLow;
   double tempDouble, output, tempHT, tempLT, tempCY;
   int lookbackTotal;
   int longestPeriod, longestIndex;
   int i,j,today,outIdx;
   int trailingPos1, trailingPos2;

   int usedFlag[3];
   int periods[3];
   int sortedPeriods[3];

   /* One entry per bar of the longest window. Stays on the stack for every
    * period up to 32, which covers the 7/14/28 default.
    */
   CIRCBUF_PROLOG_CLASS( term, UltOscTerm, 32 ); /* Id, Type, Static Size */

   *outBegIdx = 0;
   *outNBElement = 0;

   /* Ensure that the time periods are ordered from shortest to longest.
    * Sort. */
   periods[0] = optInTimePeriod1;
   periods[1] = optInTimePeriod2;
   periods[2] = optInTimePeriod3;
   usedFlag[0] = 0;
   usedFlag[1] = 0;
   usedFlag[2] = 0;
   for ( i = 0; i < 3; ++i )
   {
      longestPeriod = 0;
      longestIndex = 0;
      for ( j = 0; j < 3; ++j )
      {
         if ( (usedFlag[j] == 0) && (periods[j] > longestPeriod) )
         {
            longestPeriod = periods[j];
            longestIndex = j;
         }
      }
      usedFlag[longestIndex] = 1;
      sortedPeriods[i] = longestPeriod;
   }
   optInTimePeriod1 = sortedPeriods[2];
   optInTimePeriod2 = sortedPeriods[1];
   optInTimePeriod3 = sortedPeriods[0];

   /* Adjust startIdx for lookback period. */
   lookbackTotal = ultosc_lookback( optInTimePeriod1, optInTimePeriod2, optInTimePeriod3 );
   if( startIdx < lookbackTotal ) startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx ) return TA_SUCCESS;

   CIRCBUF_INIT_CLASS( term, UltOscTerm, optInTimePeriod3 );

   /* Prime running totals used in moving averages.
    *
    * One pass over the longest warm-up window replaces three overlapping
    * passes. A bar inside the shorter windows is added to those totals as it
    * is reached, so every total still accumulates exactly the same bars in
    * exactly the same ascending order as three separate loops did.
    */
   a1Total = 0;
   b1Total = 0;
   a2Total = 0;
   b2Total = 0;
   a3Total = 0;
   b3Total = 0;

   for ( i = startIdx-optInTimePeriod3+1; i < startIdx; ++i )
   {
      tempLT = inLow[i];
      tempHT = inHigh[i];
      tempCY = inClose[i-1];
      trueLow = min( tempLT, tempCY );
      closeMinusTrueLow = inClose[i] - trueLow;
      trueRange = tempHT - tempLT;
      tempDouble = fabs( tempCY - tempHT );
      if( tempDouble > trueRange )
         trueRange = tempDouble;
      tempDouble = fabs( tempCY - tempLT  );
      if( tempDouble > trueRange )
         trueRange = tempDouble;

      term_closeMinusTrueLow[term_Idx] = closeMinusTrueLow;
      term_trueRange[term_Idx] = trueRange;
      CIRCBUF_NEXT(term);

      if( i >= startIdx-optInTimePeriod1+1 )
      {
         a1Total += closeMinusTrueLow;
         b1Total += trueRange;
      }
      if( i >= startIdx-optInTimePeriod2+1 )
      {
         a2Total += closeMinusTrueLow;
         b2Total += trueRange;
      }
      a3Total += closeMinusTrueLow;
      b3Total += trueRange;
   }

   /* Calculate oscillator */
   today = startIdx;
   outIdx = 0;

   /* The warm-up wrote optInTimePeriod3-1 bars, so term_Idx is the slot for
    * `today` and, once advanced past it, the slot of the bar leaving the
    * longest window. The two shorter windows trail it by a fixed offset.
    */
   trailingPos1 = term_Idx + optInTimePeriod3 - optInTimePeriod1 + 1;
   if( trailingPos1 >= optInTimePeriod3 ) trailingPos1 -= optInTimePeriod3;
   trailingPos2 = term_Idx + optInTimePeriod3 - optInTimePeriod2 + 1;
   if( trailingPos2 >= optInTimePeriod3 ) trailingPos2 -= optInTimePeriod3;

   while( today <= endIdx )
   {
      /* Add on today's terms */
      tempLT = inLow[today];
      tempHT = inHigh[today];
      tempCY = inClose[today-1];
      trueLow = min( tempLT, tempCY );
      closeMinusTrueLow = inClose[today] - trueLow;
      trueRange = tempHT - tempLT;
      tempDouble = fabs( tempCY - tempHT );
      if( tempDouble > trueRange )
         trueRange = tempDouble;
      tempDouble = fabs( tempCY - tempLT  );
      if( tempDouble > trueRange )
         trueRange = tempDouble;

      term_closeMinusTrueLow[term_Idx] = closeMinusTrueLow;
      term_trueRange[term_Idx] = trueRange;

      a1Total += closeMinusTrueLow;
      a2Total += closeMinusTrueLow;
      a3Total += closeMinusTrueLow;
      b1Total += trueRange;
      b2Total += trueRange;
      b3Total += trueRange;

      /* Calculate the oscillator value for today */
      output = 0.0;

      if( !TA_IS_ZERO(b1Total) ) output += 4.0*(a1Total/b1Total);
      if( !TA_IS_ZERO(b2Total) ) output += 2.0*(a2Total/b2Total);
      if( !TA_IS_ZERO(b3Total) ) output += a3Total/b3Total;

      /* Remove the trailing terms to prepare for next day. Each was evaluated
       * once, when its bar entered the ring.
       */
      a1Total -= term_closeMinusTrueLow[trailingPos1];
      b1Total -= term_trueRange[trailingPos1];
      trailingPos1++;
      if( trailingPos1 >= optInTimePeriod3 ) trailingPos1 = 0;

      a2Total -= term_closeMinusTrueLow[trailingPos2];
      b2Total -= term_trueRange[trailingPos2];
      trailingPos2++;
      if( trailingPos2 >= optInTimePeriod3 ) trailingPos2 = 0;

      CIRCBUF_NEXT(term);
      a3Total -= term_closeMinusTrueLow[term_Idx];
      b3Total -= term_trueRange[term_Idx];

      /* Last operation is to write the output. Must
       * be done after the trailing index have all been
       * taken care of because the caller is allowed
       * to have the input array to be also the output
       * array.
       */
      outReal[outIdx] = 100.0 * (output / 7.0);

      /* Increment indexes */
      outIdx++;
      today++;
   }

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   CIRCBUF_DESTROY(term);

   return TA_SUCCESS;
}
