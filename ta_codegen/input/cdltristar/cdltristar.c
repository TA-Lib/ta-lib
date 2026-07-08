/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AC       Angelo Ciceri
 *  CSB      Christopher Barnhouse
 *
 * Change history:
 *
 *  MMDDYY BY      Description
 *  -------------------------------------------------------------------
 *  100204 AC      Creation
 *  051005 CSB,AC  Fix #1199526 for out-of-bound write in output.
 */

int cdltristar_lookback(void)
{
   return BodyDoji_avgPeriod + 2;
}

TA_RetCode cdltristar(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double BodyPeriodTotal;
   int i, outIdx, BodyTrailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdltristar_lookback();

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
   BodyPeriodTotal = 0;
   BodyTrailingIdx = startIdx -2 - BodyDoji_avgPeriod;

   i = BodyTrailingIdx;
   while( i < startIdx-2 ) {
      BodyPeriodTotal += ta_candlerange(BodyDoji_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - 3 consecutive doji days
    * - the second doji is a star
    * The meaning of "doji" is specified with TA_SetCandleSettings
    * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish
    */
   i = startIdx;
   outIdx = 0;
   do
   {
      if( ta_realbody(inClose[i-2], inOpen[i-2]) <= ta_candleaverage(BodyDoji_rangeType, BodyDoji_avgPeriod, BodyDoji_factor, BodyPeriodTotal, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) &&    // 1st: doji
         ta_realbody(inClose[i-1], inOpen[i-1]) <= ta_candleaverage(BodyDoji_rangeType, BodyDoji_avgPeriod, BodyDoji_factor, BodyPeriodTotal, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) &&    // 2nd: doji
         ta_realbody(inClose[i], inOpen[i]) <= ta_candleaverage(BodyDoji_rangeType, BodyDoji_avgPeriod, BodyDoji_factor, BodyPeriodTotal, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) ) {     // 3rd: doji
         outInteger[outIdx] = 0;
         if ( ta_realbodygapup(inOpen[i-1], inClose[i-1], inOpen[i-2], inClose[i-2])                                                  // 2nd gaps up
            &&
            max(inOpen[i],inClose[i]) < max(inOpen[i-1],inClose[i-1])                  // 3rd is not higher than 2nd
         ) {
            outInteger[outIdx] = -100;
         }
         if ( ta_realbodygapdown(inOpen[i-1], inClose[i-1], inOpen[i-2], inClose[i-2])                                                // 2nd gaps down
            &&
            min(inOpen[i],inClose[i]) > min(inOpen[i-1],inClose[i-1])                  // 3rd is not lower than 2nd
         ) {
            outInteger[outIdx] = +100;
         }
         outIdx++;
      }
      else {
         outInteger[outIdx++] = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      BodyPeriodTotal += ta_candlerange(BodyDoji_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) - ta_candlerange(BodyDoji_rangeType, inOpen[BodyTrailingIdx], inHigh[BodyTrailingIdx], inLow[BodyTrailingIdx], inClose[BodyTrailingIdx]);
      i++;
      BodyTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
