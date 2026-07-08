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
 *  120904 AC   Creation
 *
 */

int cdlpiercing_lookback(void)
{
   return BodyLong_avgPeriod + 1;
}

TA_RetCode cdlpiercing(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double BodyLongPeriodTotal[2];
   int i, outIdx, totIdx, BodyLongTrailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdlpiercing_lookback();

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
   BodyLongPeriodTotal[1] = 0;
   BodyLongPeriodTotal[0] = 0;
   BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;

   i = BodyLongTrailingIdx;
   while( i < startIdx ) {
      BodyLongPeriodTotal[1] += ta_candlerange(BodyLong_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]);
      BodyLongPeriodTotal[0] += ta_candlerange(BodyLong_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - first candle: long black candle
    * - second candle: long white candle with open below previous day low and close at least at 50% of previous day
    * real body
    * The meaning of "long" is specified with TA_SetCandleSettings
    * outInteger is positive (1 to 100): piercing pattern is always bullish
    * the user should consider that a piercing pattern is significant when it appears in a downtrend, while
    * this function does not consider it
    */
   outIdx = 0;

   do
   {
      if( ta_candlecolor(inClose[i-1], inOpen[i-1]) == -1 &&                                                        // 1st: black
         ta_realbody(inClose[i-1], inOpen[i-1]) > ta_candleaverage(BodyLong_rangeType, BodyLong_avgPeriod, BodyLong_factor, BodyLongPeriodTotal[1], inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) &&  //      long
         ta_candlecolor(inClose[i], inOpen[i]) == 1 &&                                                           // 2nd: white
         ta_realbody(inClose[i], inOpen[i]) > ta_candleaverage(BodyLong_rangeType, BodyLong_avgPeriod, BodyLong_factor, BodyLongPeriodTotal[0], inOpen[i], inHigh[i], inLow[i], inClose[i]) &&      //      long
         inOpen[i] < inLow[i-1] &&                                                           //      open below prior low
         inClose[i] < inOpen[i-1] &&                                                         //      close within prior body
         inClose[i] > inClose[i-1] + ta_realbody(inClose[i-1], inOpen[i-1]) * 0.5                                  //        above midpoint
      ) {
         outInteger[outIdx++] = 100;
      }
      else {
         outInteger[outIdx++] = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      for (totIdx = 1; totIdx >= 0; --totIdx)
         BodyLongPeriodTotal[totIdx] += ta_candlerange(BodyLong_rangeType, inOpen[i-totIdx], inHigh[i-totIdx], inLow[i-totIdx], inClose[i-totIdx])
      - ta_candlerange(BodyLong_rangeType, inOpen[BodyLongTrailingIdx-totIdx], inHigh[BodyLongTrailingIdx-totIdx], inLow[BodyLongTrailingIdx-totIdx], inClose[BodyLongTrailingIdx-totIdx]);
      i++;
      BodyLongTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
