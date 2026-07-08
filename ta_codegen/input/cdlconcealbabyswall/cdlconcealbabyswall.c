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
 *  022705 AC   Creation
 *
 */

int cdlconcealbabyswall_lookback(void)
{
   return ShadowVeryShort_avgPeriod + 3;
}

TA_RetCode cdlconcealbabyswall(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double ShadowVeryShortPeriodTotal[4];
   int i, outIdx, totIdx, ShadowVeryShortTrailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdlconcealbabyswall_lookback();

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
   ShadowVeryShortPeriodTotal[3] = 0;
   ShadowVeryShortPeriodTotal[2] = 0;
   ShadowVeryShortPeriodTotal[1] = 0;
   ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;

   i = ShadowVeryShortTrailingIdx;
   while( i < startIdx ) {
      ShadowVeryShortPeriodTotal[3] += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i-3], inHigh[i-3], inLow[i-3], inClose[i-3]);
      ShadowVeryShortPeriodTotal[2] += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]);
      ShadowVeryShortPeriodTotal[1] += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]);
      i++;
   }
   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - first candle: black marubozu (very short shadows)
    * - second candle: black marubozu (very short shadows)
    * - third candle: black candle that opens gapping down but has an upper shadow that extends into the prior body
    * - fourth candle: black candle that completely engulfs the third candle, including the shadows
    * The meanings of "very short shadow" are specified with TA_SetCandleSettings;
    * outInteger is positive (1 to 100): concealing baby swallow is always bullish;
    * the user should consider that concealing baby swallow is significant when it appears in downtrend, while
    * this function does not consider it
    */
   outIdx = 0;

   do
   {
      if( ta_candlecolor(inClose[i-3], inOpen[i-3]) == -1 &&                                    // 1st black
         ta_candlecolor(inClose[i-2], inOpen[i-2]) == -1 &&                                    // 2nd black
         ta_candlecolor(inClose[i-1], inOpen[i-1]) == -1 &&                                    // 3rd black
         ta_candlecolor(inClose[i], inOpen[i]) == -1 &&                                      // 4th black
         // 1st: marubozu
         ta_lowershadow(inLow[i-3], inClose[i-3], inOpen[i-3]) < ta_candleaverage(ShadowVeryShort_rangeType, ShadowVeryShort_avgPeriod, ShadowVeryShort_factor, ShadowVeryShortPeriodTotal[3], inOpen[i-3], inHigh[i-3], inLow[i-3], inClose[i-3]) &&
         ta_uppershadow(inHigh[i-3], inClose[i-3], inOpen[i-3]) < ta_candleaverage(ShadowVeryShort_rangeType, ShadowVeryShort_avgPeriod, ShadowVeryShort_factor, ShadowVeryShortPeriodTotal[3], inOpen[i-3], inHigh[i-3], inLow[i-3], inClose[i-3]) &&
         // 2nd: marubozu
         ta_lowershadow(inLow[i-2], inClose[i-2], inOpen[i-2]) < ta_candleaverage(ShadowVeryShort_rangeType, ShadowVeryShort_avgPeriod, ShadowVeryShort_factor, ShadowVeryShortPeriodTotal[2], inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) &&
         ta_uppershadow(inHigh[i-2], inClose[i-2], inOpen[i-2]) < ta_candleaverage(ShadowVeryShort_rangeType, ShadowVeryShort_avgPeriod, ShadowVeryShort_factor, ShadowVeryShortPeriodTotal[2], inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) &&
         ta_realbodygapdown(inOpen[i-1], inClose[i-1], inOpen[i-2], inClose[i-2]) &&                                  // 3rd: opens gapping down
         //      and HAS an upper shadow
         ta_uppershadow(inHigh[i-1], inClose[i-1], inOpen[i-1]) > ta_candleaverage(ShadowVeryShort_rangeType, ShadowVeryShort_avgPeriod, ShadowVeryShort_factor, ShadowVeryShortPeriodTotal[1], inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) &&
         inHigh[i-1] > inClose[i-2] &&                                   //      that extends into the prior body
         inHigh[i] > inHigh[i-1] && inLow[i] < inLow[i-1]                // 4th: engulfs the 3rd including the shadows
      ) {
         outInteger[outIdx++] = 100;
      }
      else {
         outInteger[outIdx++] = 0;
      }

      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      for (totIdx = 3; totIdx >= 1; --totIdx)
         ShadowVeryShortPeriodTotal[totIdx] += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i-totIdx], inHigh[i-totIdx], inLow[i-totIdx], inClose[i-totIdx])
      - ta_candlerange(ShadowVeryShort_rangeType, inOpen[ShadowVeryShortTrailingIdx-totIdx], inHigh[ShadowVeryShortTrailingIdx-totIdx], inLow[ShadowVeryShortTrailingIdx-totIdx], inClose[ShadowVeryShortTrailingIdx-totIdx]);
      i++;
      ShadowVeryShortTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
