/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AC       Angelo Ciceri
 *  MF       Mario Fortier
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  032005 AC   Creation
 *  041305 MF   Minor modification for a compiler warning
 */

int cdlladderbottom_lookback(void)
{
   return ShadowVeryShort_avgPeriod + 4;
}

TA_RetCode cdlladderbottom(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double ShadowVeryShortPeriodTotal;
   int i, outIdx, ShadowVeryShortTrailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdlladderbottom_lookback();

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
   ShadowVeryShortPeriodTotal = 0;
   ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;

   i = ShadowVeryShortTrailingIdx;
   while( i < startIdx ) {
      ShadowVeryShortPeriodTotal += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]);
      i++;
   }
   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - three black candlesticks with consecutively lower opens and closes
    * - fourth candle: black candle with an upper shadow (it's supposed to be not very short)
    * - fifth candle: white candle that opens above prior candle's body and closes above prior candle's high
    * The meaning of "very short" is specified with TA_SetCandleSettings
    * outInteger is positive (1 to 100): ladder bottom is always bullish;
    * the user should consider that ladder bottom is significant when it appears in a downtrend,
    * while this function does not consider it
    */
   outIdx = 0;
   do
   {
      if(
         ta_candlecolor(inClose[i-4], inOpen[i-4]) == -1 && ta_candlecolor(inClose[i-3], inOpen[i-3]) == -1 && ta_candlecolor(inClose[i-2], inOpen[i-2]) == -1 &&  // 3 black candlesticks
         inOpen[i-4] > inOpen[i-3] && inOpen[i-3] > inOpen[i-2] &&           // with consecutively lower opens
         inClose[i-4] > inClose[i-3] && inClose[i-3] > inClose[i-2] &&       // and closes
         ta_candlecolor(inClose[i-1], inOpen[i-1]) == -1 &&                                        // 4th: black with an upper shadow
         ta_uppershadow(inHigh[i-1], inClose[i-1], inOpen[i-1]) > ta_candleaverage(ShadowVeryShort_rangeType, ShadowVeryShort_avgPeriod, ShadowVeryShort_factor, ShadowVeryShortPeriodTotal, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) &&
         ta_candlecolor(inClose[i], inOpen[i]) == 1 &&                                           // 5th: white
         inOpen[i] > inOpen[i-1] &&                                          // that opens above prior candle's body
         inClose[i] > inHigh[i-1]                                            // and closes above prior candle's high
      ) {
         outInteger[outIdx++] = 100;
      }
      else {
         outInteger[outIdx++] = 0;
      }

      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      ShadowVeryShortPeriodTotal += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1])
      - ta_candlerange(ShadowVeryShort_rangeType, inOpen[ShadowVeryShortTrailingIdx-1], inHigh[ShadowVeryShortTrailingIdx-1], inLow[ShadowVeryShortTrailingIdx-1], inClose[ShadowVeryShortTrailingIdx-1]);
      i++;
      ShadowVeryShortTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
