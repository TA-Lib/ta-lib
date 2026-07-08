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
 *  121104 AC   Creation
 *
 */
int cdl2crows_lookback(void)
{
   return BodyLong_avgPeriod + 2;
}

TA_RetCode cdl2crows(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double BodyLongPeriodTotal;
   int i, outIdx, BodyLongTrailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdl2crows_lookback();

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
   BodyLongPeriodTotal = 0;
   BodyLongTrailingIdx = startIdx -2 - BodyLong_avgPeriod;

   i = BodyLongTrailingIdx;
   while( i < startIdx-2 ) {
      BodyLongPeriodTotal += ta_candlerange(BodyLong_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - first candle: long white candle
    * - second candle: black real body
    * - gap between the first and the second candle's real bodies
    * - third candle: black candle that opens within the second real body and closes within the first real body
    * The meaning of "long" is specified with TA_SetCandleSettings
    * outInteger is negative (-1 to -100): two crows is always bearish;
    * the user should consider that two crows is significant when it appears in an uptrend, while this function
    * does not consider the trend
    */
   outIdx = 0;
   do
   {
      if( ta_candlecolor(inClose[i-2], inOpen[i-2]) == 1 &&                                                         // 1st: white
         ta_realbody(inClose[i-2], inOpen[i-2]) > ta_candleaverage(BodyLong_rangeType, BodyLong_avgPeriod, BodyLong_factor, BodyLongPeriodTotal, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) &&     //      long
         ta_candlecolor(inClose[i-1], inOpen[i-1]) == -1 &&                                                        // 2nd: black
         ta_realbodygapup(inOpen[i-1], inClose[i-1], inOpen[i-2], inClose[i-2]) &&                                                        //      gapping up
         ta_candlecolor(inClose[i], inOpen[i]) == -1 &&                                                          // 3rd: black
         inOpen[i] < inOpen[i-1] && inOpen[i] > inClose[i-1] &&                              //      opening within 2nd rb
         inClose[i] > inOpen[i-2] && inClose[i] < inClose[i-2]                               //      closing within 1st rb
      ) {
         outInteger[outIdx++] = -100;
      }
      else {
         outInteger[outIdx++] = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      BodyLongPeriodTotal += ta_candlerange(BodyLong_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) - ta_candlerange(BodyLong_rangeType, inOpen[BodyLongTrailingIdx], inHigh[BodyLongTrailingIdx], inLow[BodyLongTrailingIdx], inClose[BodyLongTrailingIdx]);
      i++;
      BodyLongTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
