/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  DM       Drew McCormack (http://www.trade-strategist.com)
 *  MF       Mario Fortier
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  281206 DM   Initial Implementation
 *  010606 MF   Abstract local arrays. Detect divide by zero.
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
   double a1Total, a2Total, a3Total;
   double b1Total, b2Total, b3Total;
   double trueLow, trueRange, closeMinusTrueLow;
   double tempDouble, output, tempHT, tempLT, tempCY;
   int lookbackTotal;
   int longestPeriod, longestIndex;
   int i,j,today,outIdx;
   int trailingIdx1, trailingIdx2, trailingIdx3;

   int usedFlag[3];
   int periods[3];
   int sortedPeriods[3];

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

   /* Prime running totals used in moving averages */
   a1Total = 0;
   b1Total = 0;
   for ( i = startIdx-optInTimePeriod1+1; i < startIdx; ++i )
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
      a1Total += closeMinusTrueLow;
      b1Total += trueRange;
   }

   a2Total = 0;
   b2Total = 0;
   for ( i = startIdx-optInTimePeriod2+1; i < startIdx; ++i )
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
      a2Total += closeMinusTrueLow;
      b2Total += trueRange;
   }

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
      a3Total += closeMinusTrueLow;
      b3Total += trueRange;
   }

   /* Calculate oscillator */
   today = startIdx;
   outIdx = 0;
   trailingIdx1 = today - optInTimePeriod1 + 1;
   trailingIdx2 = today - optInTimePeriod2 + 1;
   trailingIdx3 = today - optInTimePeriod3 + 1;
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

      /* Remove the trailing terms to prepare for next day */
      tempLT = inLow[trailingIdx1];
      tempHT = inHigh[trailingIdx1];
      tempCY = inClose[trailingIdx1-1];
      trueLow = min( tempLT, tempCY );
      closeMinusTrueLow = inClose[trailingIdx1] - trueLow;
      trueRange = tempHT - tempLT;
      tempDouble = fabs( tempCY - tempHT );
      if( tempDouble > trueRange )
         trueRange = tempDouble;
      tempDouble = fabs( tempCY - tempLT  );
      if( tempDouble > trueRange )
         trueRange = tempDouble;
      a1Total -= closeMinusTrueLow;
      b1Total -= trueRange;

      tempLT = inLow[trailingIdx2];
      tempHT = inHigh[trailingIdx2];
      tempCY = inClose[trailingIdx2-1];
      trueLow = min( tempLT, tempCY );
      closeMinusTrueLow = inClose[trailingIdx2] - trueLow;
      trueRange = tempHT - tempLT;
      tempDouble = fabs( tempCY - tempHT );
      if( tempDouble > trueRange )
         trueRange = tempDouble;
      tempDouble = fabs( tempCY - tempLT  );
      if( tempDouble > trueRange )
         trueRange = tempDouble;
      a2Total -= closeMinusTrueLow;
      b2Total -= trueRange;

      tempLT = inLow[trailingIdx3];
      tempHT = inHigh[trailingIdx3];
      tempCY = inClose[trailingIdx3-1];
      trueLow = min( tempLT, tempCY );
      closeMinusTrueLow = inClose[trailingIdx3] - trueLow;
      trueRange = tempHT - tempLT;
      tempDouble = fabs( tempCY - tempHT );
      if( tempDouble > trueRange )
         trueRange = tempDouble;
      tempDouble = fabs( tempCY - tempLT  );
      if( tempDouble > trueRange )
         trueRange = tempDouble;
      a3Total -= closeMinusTrueLow;
      b3Total -= trueRange;

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
      trailingIdx1++;
      trailingIdx2++;
      trailingIdx3++;
   }

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
