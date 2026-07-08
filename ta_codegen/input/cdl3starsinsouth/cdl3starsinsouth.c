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

int cdl3starsinsouth_lookback(void)
{
   return max( max( ShadowVeryShort_avgPeriod, ShadowLong_avgPeriod ),
      max( BodyLong_avgPeriod, BodyShort_avgPeriod )
   ) + 2;
}

TA_RetCode cdl3starsinsouth(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double BodyLongPeriodTotal, BodyShortPeriodTotal, ShadowLongPeriodTotal;
   double ShadowVeryShortPeriodTotal[2];
   int i, outIdx, totIdx, BodyLongTrailingIdx, BodyShortTrailingIdx, ShadowLongTrailingIdx, ShadowVeryShortTrailingIdx,
   lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdl3starsinsouth_lookback();

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
   ShadowLongPeriodTotal = 0;
   ShadowLongTrailingIdx = startIdx - ShadowLong_avgPeriod;
   ShadowVeryShortPeriodTotal[1] = 0;
   ShadowVeryShortPeriodTotal[0] = 0;
   ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
   BodyShortPeriodTotal = 0;
   BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;

   i = BodyLongTrailingIdx;
   while( i < startIdx ) {
      BodyLongPeriodTotal += ta_candlerange(BodyLong_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]);
      i++;
   }
   i = ShadowLongTrailingIdx;
   while( i < startIdx ) {
      ShadowLongPeriodTotal += ta_candlerange(ShadowLong_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]);
      i++;
   }
   i = ShadowVeryShortTrailingIdx;
   while( i < startIdx ) {
      ShadowVeryShortPeriodTotal[1] += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]);
      ShadowVeryShortPeriodTotal[0] += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = BodyShortTrailingIdx;
   while( i < startIdx ) {
      BodyShortPeriodTotal += ta_candlerange(BodyShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - first candle: long black candle with long lower shadow
    * - second candle: smaller black candle that opens higher than prior close but within prior candle's range
    *   and trades lower than prior close but not lower than prior low and closes off of its low (it has a shadow)
    * - third candle: small black marubozu (or candle with very short shadows) engulfed by prior candle's range
    * The meanings of "long body", "short body", "very short shadow" are specified with TA_SetCandleSettings;
    * outInteger is positive (1 to 100): 3 stars in the south is always bullish;
    * the user should consider that 3 stars in the south is significant when it appears in downtrend, while this function
    * does not consider it
    */
   outIdx = 0;
   do
   {
      if( ta_candlecolor(inClose[i-2], inOpen[i-2]) == -1 &&                                    // 1st black
         ta_candlecolor(inClose[i-1], inOpen[i-1]) == -1 &&                                    // 2nd black
         ta_candlecolor(inClose[i], inOpen[i]) == -1 &&                                      // 3rd black
         // 1st: long
         ta_realbody(inClose[i-2], inOpen[i-2]) > ta_candleaverage(BodyLong_rangeType, BodyLong_avgPeriod, BodyLong_factor, BodyLongPeriodTotal, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) &&
         //      with long lower shadow
         ta_lowershadow(inLow[i-2], inClose[i-2], inOpen[i-2]) > ta_candleaverage(ShadowLong_rangeType, ShadowLong_avgPeriod, ShadowLong_factor, ShadowLongPeriodTotal, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) &&
         ta_realbody(inClose[i-1], inOpen[i-1]) < ta_realbody(inClose[i-2], inOpen[i-2]) &&                          // 2nd: smaller candle
         inOpen[i-1] > inClose[i-2] && inOpen[i-1] <= inHigh[i-2] &&     //      that opens higher but within 1st range
         inLow[i-1] < inClose[i-2] &&                                    //      and trades lower than 1st close
         inLow[i-1] >= inLow[i-2] &&                                     //      but not lower than 1st low
         //      and has a lower shadow
         ta_lowershadow(inLow[i-1], inClose[i-1], inOpen[i-1]) > ta_candleaverage(ShadowVeryShort_rangeType, ShadowVeryShort_avgPeriod, ShadowVeryShort_factor, ShadowVeryShortPeriodTotal[1], inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) &&
         // 3rd: small marubozu
         ta_realbody(inClose[i], inOpen[i]) < ta_candleaverage(BodyShort_rangeType, BodyShort_avgPeriod, BodyShort_factor, BodyShortPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i]) &&
         ta_lowershadow(inLow[i], inClose[i], inOpen[i]) < ta_candleaverage(ShadowVeryShort_rangeType, ShadowVeryShort_avgPeriod, ShadowVeryShort_factor, ShadowVeryShortPeriodTotal[0], inOpen[i], inHigh[i], inLow[i], inClose[i]) &&
         ta_uppershadow(inHigh[i], inClose[i], inOpen[i]) < ta_candleaverage(ShadowVeryShort_rangeType, ShadowVeryShort_avgPeriod, ShadowVeryShort_factor, ShadowVeryShortPeriodTotal[0], inOpen[i], inHigh[i], inLow[i], inClose[i]) &&
         inLow[i] > inLow[i-1] && inHigh[i] < inHigh[i-1]                //      engulfed by prior candle's range
      ) {
         outInteger[outIdx++] = 100;
      }
      else {
         outInteger[outIdx++] = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      BodyLongPeriodTotal += ta_candlerange(BodyLong_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2])
      - ta_candlerange(BodyLong_rangeType, inOpen[BodyLongTrailingIdx-2], inHigh[BodyLongTrailingIdx-2], inLow[BodyLongTrailingIdx-2], inClose[BodyLongTrailingIdx-2]);
      ShadowLongPeriodTotal += ta_candlerange(ShadowLong_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2])
      - ta_candlerange(ShadowLong_rangeType, inOpen[ShadowLongTrailingIdx-2], inHigh[ShadowLongTrailingIdx-2], inLow[ShadowLongTrailingIdx-2], inClose[ShadowLongTrailingIdx-2]);
      for (totIdx = 1; totIdx >= 0; --totIdx)
         ShadowVeryShortPeriodTotal[totIdx] += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i-totIdx], inHigh[i-totIdx], inLow[i-totIdx], inClose[i-totIdx])
      - ta_candlerange(ShadowVeryShort_rangeType, inOpen[ShadowVeryShortTrailingIdx-totIdx], inHigh[ShadowVeryShortTrailingIdx-totIdx], inLow[ShadowVeryShortTrailingIdx-totIdx], inClose[ShadowVeryShortTrailingIdx-totIdx]);
      BodyShortPeriodTotal += ta_candlerange(BodyShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i])
      - ta_candlerange(BodyShort_rangeType, inOpen[BodyShortTrailingIdx], inHigh[BodyShortTrailingIdx], inLow[BodyShortTrailingIdx], inClose[BodyShortTrailingIdx]);
      i++;
      BodyLongTrailingIdx++;
      ShadowLongTrailingIdx++;
      ShadowVeryShortTrailingIdx++;
      BodyShortTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
