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

int cdltasukigap_lookback(void)
{
   return Near_avgPeriod + 2;
}

TA_RetCode cdltasukigap(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double NearPeriodTotal;
   int i, outIdx, NearTrailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdltasukigap_lookback();

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
   NearPeriodTotal = 0;
   NearTrailingIdx = startIdx - Near_avgPeriod;

   i = NearTrailingIdx;
   while( i < startIdx ) {
      NearPeriodTotal += ta_candlerange(Near_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]);
      i++;
   }
   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - upside (downside) gap
    * - first candle after the window: white (black) candlestick
    * - second candle: black (white) candlestick that opens within the previous real body and closes under (above)
    *   the previous real body inside the gap
    * - the size of two real bodies should be near the same
    * The meaning of "near" is specified with TA_SetCandleSettings
    * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish;
    * the user should consider that tasuki gap is significant when it appears in a trend, while this function does
    * not consider it
    */
   outIdx = 0;
   do
   {
      if(
         (
         ta_realbodygapup(inOpen[i-1], inClose[i-1], inOpen[i-2], inClose[i-2]) &&                                // upside gap
         ta_candlecolor(inClose[i-1], inOpen[i-1]) == 1 &&                                 // 1st: white
         ta_candlecolor(inClose[i], inOpen[i]) == -1 &&                                  // 2nd: black
         inOpen[i] < inClose[i-1] && inOpen[i] > inOpen[i-1] &&      //      that opens within the white rb
         inClose[i] < inOpen[i-1] &&                                 //      and closes under the white rb
         inClose[i] > max(inClose[i-2], inOpen[i-2]) &&              //      inside the gap
         // size of 2 rb near the same
         fabs(ta_realbody(inClose[i-1], inOpen[i-1]) - ta_realbody(inClose[i], inOpen[i])) < ta_candleaverage(Near_rangeType, Near_avgPeriod, Near_factor, NearPeriodTotal, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1])
      ) ||
         (
         ta_realbodygapdown(inOpen[i-1], inClose[i-1], inOpen[i-2], inClose[i-2]) &&                              // downside gap
         ta_candlecolor(inClose[i-1], inOpen[i-1]) == -1 &&                                // 1st: black
         ta_candlecolor(inClose[i], inOpen[i]) == 1 &&                                   // 2nd: white
         inOpen[i] < inOpen[i-1] && inOpen[i] > inClose[i-1] &&      //      that opens within the black rb
         inClose[i] > inOpen[i-1] &&                                 //      and closes above the black rb
         inClose[i] < min(inClose[i-2], inOpen[i-2]) &&              //      inside the gap
         // size of 2 rb near the same
         fabs(ta_realbody(inClose[i-1], inOpen[i-1]) - ta_realbody(inClose[i], inOpen[i])) < ta_candleaverage(Near_rangeType, Near_avgPeriod, Near_factor, NearPeriodTotal, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1])
      )
      ) {
         outInteger[outIdx++] = ta_candlecolor(inClose[i-1], inOpen[i-1]) * 100;
      }
      else {
         outInteger[outIdx++] = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      NearPeriodTotal += ta_candlerange(Near_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) - ta_candlerange(Near_rangeType, inOpen[NearTrailingIdx-1], inHigh[NearTrailingIdx-1], inLow[NearTrailingIdx-1], inClose[NearTrailingIdx-1]);
      i++;
      NearTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
