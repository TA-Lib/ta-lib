/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AC       Angelo Ciceri
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120305 AC   Creation
 *
 */

int cdlhikkake_lookback(void)
{
   return 5;
}

TA_RetCode cdlhikkake(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   int i, outIdx, lookbackTotal, patternIdx, patternResult;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdlhikkake_lookback();

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

   /* Do the calculation using tight loops. */
   /* Add-up the initial period, except for the last value. */
   patternIdx = 0;
   patternResult = 0;

   i = startIdx - 3;
   while( i < startIdx ) {
      /* copy here the pattern recognition code below */
      if( inHigh[i-1] < inHigh[i-2] && inLow[i-1] > inLow[i-2] &&             // 1st + 2nd: lower high and higher low
         ( ( inHigh[i] < inHigh[i-1] && inLow[i] < inLow[i-1] )              // (bull) 3rd: lower high and lower low
         ||
         ( inHigh[i] > inHigh[i-1] && inLow[i] > inLow[i-1] )              // (bear) 3rd: higher high and higher low
      )
      ) {
         patternResult = 100 * ( inHigh[i] < inHigh[i-1] ? 1 : -1 );
         patternIdx = i;
      } else
      /* search for confirmation if hikkake was no more than 3 bars ago */
      if( i <= patternIdx+3 &&
         ( ( patternResult > 0 && inClose[i] > inHigh[patternIdx-1] )    // close higher than the high of 2nd
         ||
         ( patternResult < 0 && inClose[i] < inLow[patternIdx-1] )     // close lower than the low of 2nd
      )
      ) {
         patternIdx = 0;
      }
      i++;
   }

   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - first and second candle: inside bar (2nd has lower high and higher low than 1st)
    * - third candle: lower high and lower low than 2nd (higher high and higher low than 2nd)
    * outInteger[hikkakebar] is positive (1 to 100) or negative (-1 to -100) meaning bullish or bearish hikkake
    * Confirmation could come in the next 3 days with:
    * - a day that closes higher than the high (lower than the low) of the 2nd candle
    * outInteger[confirmationbar] is equal to 100 + the bullish hikkake result or -100 - the bearish hikkake result
    * Note: if confirmation and a new hikkake come at the same bar, only the new hikkake is reported (the new hikkake
    * overwrites the confirmation of the old hikkake)
    */
   outIdx = 0;
   do
   {
      if( inHigh[i-1] < inHigh[i-2] && inLow[i-1] > inLow[i-2] &&             // 1st + 2nd: lower high and higher low
         ( ( inHigh[i] < inHigh[i-1] && inLow[i] < inLow[i-1] )              // (bull) 3rd: lower high and lower low
         ||
         ( inHigh[i] > inHigh[i-1] && inLow[i] > inLow[i-1] )              // (bear) 3rd: higher high and higher low
      )
      ) {
         patternResult = 100 * ( inHigh[i] < inHigh[i-1] ? 1 : -1 );
         patternIdx = i;
         outInteger[outIdx++] = patternResult;
      } else
      /* search for confirmation if hikkake was no more than 3 bars ago */
      if( i <= patternIdx+3 &&
         ( ( patternResult > 0 && inClose[i] > inHigh[patternIdx-1] )    // close higher than the high of 2nd
         ||
         ( patternResult < 0 && inClose[i] < inLow[patternIdx-1] )     // close lower than the low of 2nd
      )
      ) {
         outInteger[outIdx++] = patternResult + 100 * ( patternResult > 0 ? 1 : -1 );
         patternIdx = 0;
      } else {
         outInteger[outIdx++] = 0;
      }
      i++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
