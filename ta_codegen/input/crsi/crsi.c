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
 *  092226 MF,CC  Initial version (#431).
 */

int crsi_lookback(int optInTimePeriod, int optInStreakPeriod, int optInRankPeriod)
{
   int retValue;

   retValue = rsi_lookback( optInTimePeriod );
   retValue = max( retValue, rsi_lookback( optInStreakPeriod ) + 1 );
   retValue = max( retValue, percentrank_lookback( optInRankPeriod ) + rocp_lookback( 1 ) );

   return retValue;
}

TA_RetCode crsi(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int optInStreakPeriod,
   int optInRankPeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double *tempRSI;
   double *tempStreak;
   double *tempStreakRSI;
   TA_RetCode retCode;
   int lookbackTotal, anchorIdx, today, outIdx, i;
   int offsetRSI, offsetStreak;
   int tempBegIdx, rsiNb, streakNb, rocNb;
   double prevClose, close, streak;

   *outBegIdx = 0;
   *outNBElement = 0;

   lookbackTotal = crsi_lookback( optInTimePeriod, optInStreakPeriod, optInRankPeriod );

   if( lookbackTotal > endIdx )
      return TA_SUCCESS;

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Both RSI legs start on the bar lookbackTotal before startIdx (the streak,
    * a difference, one bar later), so the leg with the shorter lookback warms
    * up over the spare bars instead of starting cold at its own lookback.
    */
   anchorIdx = startIdx - lookbackTotal;

   tempStreak = malloc((endIdx-anchorIdx) * sizeof(double));
   if( !tempStreak )
      return TA_ALLOC_ERR;

   tempRSI = malloc((endIdx-anchorIdx-rsi_lookback(optInTimePeriod)+1) * sizeof(double));
   if( !tempRSI )
   {
      free( tempStreak );
      return TA_ALLOC_ERR;
   }

   tempStreakRSI = malloc((endIdx-anchorIdx-rsi_lookback(optInStreakPeriod)) * sizeof(double));
   if( !tempStreakRSI )
   {
      free( tempStreak );
      free( tempRSI );
      return TA_ALLOC_ERR;
   }

   streak = 0.0;
   prevClose = inReal[anchorIdx];
   outIdx = 0;
   today = anchorIdx+1;
   while( today <= endIdx )
   {
      close = inReal[today];
      if( close > prevClose )
         streak = (streak > 0.0) ? streak + 1.0 : 1.0;
      else if( close < prevClose )
         streak = (streak < 0.0) ? streak - 1.0 : -1.0;
      else
         streak = 0.0;
      tempStreak[outIdx++] = streak;
      prevClose = close;
      today++;
   }

   retCode = rsi( 0, outIdx-1, tempStreak, optInStreakPeriod,
      &tempBegIdx, &streakNb, tempStreakRSI );
   if( retCode != TA_SUCCESS )
   {
      free( tempStreak );
      free( tempRSI );
      free( tempStreakRSI );
      return retCode;
   }

   retCode = rsi( anchorIdx+rsi_lookback(optInTimePeriod), endIdx, inReal, optInTimePeriod,
      &tempBegIdx, &rsiNb, tempRSI );
   if( retCode != TA_SUCCESS )
   {
      free( tempStreak );
      free( tempRSI );
      free( tempStreakRSI );
      return retCode;
   }

   /* The streak is consumed: its buffer holds the returns. */
   retCode = rocp( startIdx-optInRankPeriod, endIdx, inReal, 1,
      &tempBegIdx, &rocNb, tempStreak );
   if( retCode != TA_SUCCESS )
   {
      free( tempStreak );
      free( tempRSI );
      free( tempStreakRSI );
      return retCode;
   }

   retCode = percentrank( 0, rocNb-1, tempStreak, optInRankPeriod,
      outBegIdx, outNBElement, outReal );
   if( retCode != TA_SUCCESS )
   {
      free( tempStreak );
      free( tempRSI );
      free( tempStreakRSI );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   offsetRSI    = rsiNb - *outNBElement;
   offsetStreak = streakNb - *outNBElement;

   for( i=0; i < (int)*outNBElement; i++ )
      outReal[i] = ( tempRSI[i+offsetRSI] + tempStreakRSI[i+offsetStreak] + outReal[i] ) / 3.0;

   free( tempStreak );
   free( tempRSI );
   free( tempStreakRSI );

   *outBegIdx = startIdx;

   return TA_SUCCESS;
}
