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
 *  011505 AC   Creation
 *
 */

int cdlbreakaway_lookback(void)
{
   return BodyLong_avgPeriod + 4;
}

TA_RetCode cdlbreakaway(int startIdx, int endIdx,
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

   lookbackTotal = cdlbreakaway_lookback();

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
   BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;

   i = BodyLongTrailingIdx;
   while( i < startIdx ) {
      BodyLongPeriodTotal += ta_candlerange(BodyLong_rangeType, inOpen[i-4], inHigh[i-4], inLow[i-4], inClose[i-4]);
      i++;
   }
   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - first candle: long black (white)
    * - second candle: black (white) day whose body gaps down (up)
    * - third candle: black or white day with lower (higher) high and lower (higher) low than prior candle's
    * - fourth candle: black (white) day with lower (higher) high and lower (higher) low than prior candle's
    * - fifth candle: white (black) day that closes inside the gap, erasing the prior 3 days
    * The meaning of "long" is specified with TA_SetCandleSettings
    * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish;
    * the user should consider that breakaway is significant in a trend opposite to the last candle, while this
    * function does not consider it
    */
   outIdx = 0;
   do
   {
      if( ta_candlecolor(inClose[i-4], inOpen[i-4]) == ta_candlecolor(inClose[i-3], inOpen[i-3]) &&                   // 1st, 2nd, 4th same color, 5th opposite
         ta_candlecolor(inClose[i-3], inOpen[i-3]) == ta_candlecolor(inClose[i-1], inOpen[i-1]) &&
         ta_candlecolor(inClose[i-1], inOpen[i-1]) == -ta_candlecolor(inClose[i], inOpen[i]) &&
         ta_realbody(inClose[i-4], inOpen[i-4]) > ta_candleaverage(BodyLong_rangeType, BodyLong_avgPeriod, BodyLong_factor, BodyLongPeriodTotal, inOpen[i-4], inHigh[i-4], inLow[i-4], inClose[i-4]) &&     // 1st long
         (
         ( ta_candlecolor(inClose[i-4], inOpen[i-4]) == -1 &&                                // when 1st is black:
         ta_realbodygapdown(inOpen[i-3], inClose[i-3], inOpen[i-4], inClose[i-4]) &&                              // 2nd gaps down
         inHigh[i-2] < inHigh[i-3] && inLow[i-2] < inLow[i-3] &&     // 3rd has lower high and low than 2nd
         inHigh[i-1] < inHigh[i-2] && inLow[i-1] < inLow[i-2] &&     // 4th has lower high and low than 3rd
         inClose[i] > inOpen[i-3] && inClose[i] < inClose[i-4]       // 5th closes inside the gap
      )
         ||
         ( ta_candlecolor(inClose[i-4], inOpen[i-4]) == 1 &&                                 // when 1st is white:
         ta_realbodygapup(inOpen[i-3], inClose[i-3], inOpen[i-4], inClose[i-4]) &&                                // 2nd gaps up
         inHigh[i-2] > inHigh[i-3] && inLow[i-2] > inLow[i-3] &&     // 3rd has higher high and low than 2nd
         inHigh[i-1] > inHigh[i-2] && inLow[i-1] > inLow[i-2] &&     // 4th has higher high and low than 3rd
         inClose[i] < inOpen[i-3] && inClose[i] > inClose[i-4]       // 5th closes inside the gap
      )
      )
      ) {
         outInteger[outIdx++] = ta_candlecolor(inClose[i], inOpen[i]) * 100;
      }
      else {
         outInteger[outIdx++] = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      BodyLongPeriodTotal += ta_candlerange(BodyLong_rangeType, inOpen[i-4], inHigh[i-4], inLow[i-4], inClose[i-4])
      - ta_candlerange(BodyLong_rangeType, inOpen[BodyLongTrailingIdx-4], inHigh[BodyLongTrailingIdx-4], inLow[BodyLongTrailingIdx-4], inClose[BodyLongTrailingIdx-4]);
      i++;
      BodyLongTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
