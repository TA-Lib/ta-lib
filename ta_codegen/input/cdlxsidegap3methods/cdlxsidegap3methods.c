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
 *  011605 AC   Creation
 *
 */

int cdlxsidegap3methods_lookback(void)
{
   return 2;
}

TA_RetCode cdlxsidegap3methods(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   int i, outIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdlxsidegap3methods_lookback();

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
   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - first candle: white (black) candle
    * - second candle: white (black) candle
    * - upside (downside) gap between the first and the second real bodies
    * - third candle: black (white) candle that opens within the second real body and closes within the first real body
    * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish;
    * the user should consider that up/downside gap 3 methods is significant when it appears in a trend, while this
    * function does not consider it
    */
   outIdx = 0;
   do
   {
      if( ta_candlecolor(inClose[i-2], inOpen[i-2]) == ta_candlecolor(inClose[i-1], inOpen[i-1]) &&                   // 1st and 2nd of same color
         ta_candlecolor(inClose[i-1], inOpen[i-1]) == -ta_candlecolor(inClose[i], inOpen[i]) &&                    // 3rd opposite color
         inOpen[i] < max(inClose[i-1], inOpen[i-1]) &&                   // 3rd opens within 2nd rb
         inOpen[i] > min(inClose[i-1], inOpen[i-1]) &&
         inClose[i] < max(inClose[i-2], inOpen[i-2]) &&                  // 3rd closes within 1st rb
         inClose[i] > min(inClose[i-2], inOpen[i-2]) &&
         ( (
         ta_candlecolor(inClose[i-2], inOpen[i-2]) == 1 &&                                 // when 1st is white
         ta_realbodygapup(inOpen[i-1], inClose[i-1], inOpen[i-2], inClose[i-2])                                   // upside gap
      ) ||
         (
         ta_candlecolor(inClose[i-2], inOpen[i-2]) == -1 &&                                // when 1st is black
         ta_realbodygapdown(inOpen[i-1], inClose[i-1], inOpen[i-2], inClose[i-2])                                 // downside gap
      )
      )
      ) {
         outInteger[outIdx++] = ta_candlecolor(inClose[i-2], inOpen[i-2]) * 100;
      }
      else {
         outInteger[outIdx++] = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      i++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
