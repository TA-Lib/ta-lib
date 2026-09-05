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
 *  090526 MF,CC  Initial version (#371).
 */

int fractal_lookback(int optInLeftBars, int optInRightBars)
{
   return optInLeftBars + optInRightBars;
}

TA_RetCode fractal(int startIdx, int endIdx,
   const double inHigh[], const double inLow[],
   int optInLeftBars, int optInRightBars,
   int *outBegIdx, int *outNBElement,
   int outSwingHigh[], int outSwingLow[])
{
   int today, outIdx, lookbackTotal, i;
   double pivotHigh, pivotLow, otherHigh, otherLow, tempHigh, tempLow;

   *outBegIdx = 0;
   *outNBElement = 0;

   lookbackTotal = fractal_lookback( optInLeftBars, optInRightBars );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
      return TA_SUCCESS;

   outIdx = 0;
   today = startIdx;
   while( today <= endIdx )
   {
      pivotHigh = 0.0;
      pivotLow = 0.0;
      otherHigh = 0.0;
      otherLow = 0.0;
      for( i = optInLeftBars + optInRightBars; i >= 0; i-- )
      {
         tempHigh = inHigh[today-i];
         tempLow = inLow[today-i];
         if( i == optInRightBars )
         {
            pivotHigh = tempHigh;
            pivotLow = tempLow;
         }
         else if( i == optInLeftBars + optInRightBars )
         {
            /* optInLeftBars is at least 1, so the oldest bar of the window is
             * never the pivot and always seeds the rest-of-window extrema.
             */
            otherHigh = tempHigh;
            otherLow = tempLow;
         }
         else
         {
            if( tempHigh > otherHigh )
               otherHigh = tempHigh;
            if( tempLow < otherLow )
               otherLow = tempLow;
         }
      }

      if( pivotHigh > otherHigh )
         outSwingHigh[outIdx] = 100;
      else
         outSwingHigh[outIdx] = 0;

      if( pivotLow < otherLow )
         outSwingLow[outIdx] = 100;
      else
         outSwingLow[outIdx] = 0;

      outIdx++;
      today++;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
