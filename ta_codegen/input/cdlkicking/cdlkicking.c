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
 *  010705 AC   Creation
 *
 */

int cdlkicking_lookback(void)
{
   return max( ShadowVeryShort_avgPeriod, BodyLong_avgPeriod
   ) + 1;
}

TA_RetCode cdlkicking(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double ShadowVeryShortPeriodTotal[2];
   double BodyLongPeriodTotal[2];
   int i, outIdx, totIdx, ShadowVeryShortTrailingIdx, BodyLongTrailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdlkicking_lookback();

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
   ShadowVeryShortPeriodTotal[1] = 0;
   ShadowVeryShortPeriodTotal[0] = 0;
   ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
   BodyLongPeriodTotal[1] = 0;
   BodyLongPeriodTotal[0] = 0;
   BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;

   i = ShadowVeryShortTrailingIdx;
   while( i < startIdx ) {
      ShadowVeryShortPeriodTotal[1] += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]);
      ShadowVeryShortPeriodTotal[0] += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = BodyLongTrailingIdx;
   while( i < startIdx ) {
      BodyLongPeriodTotal[1] += ta_candlerange(BodyLong_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]);
      BodyLongPeriodTotal[0] += ta_candlerange(BodyLong_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - first candle: marubozu
    * - second candle: opposite color marubozu
    * - gap between the two candles: upside gap if black then white, downside gap if white then black
    * The meaning of "long body" and "very short shadow" is specified with TA_SetCandleSettings
    * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish
    */
   outIdx = 0;
   do
   {
      if( ta_candlecolor(inClose[i-1], inOpen[i-1]) == -ta_candlecolor(inClose[i], inOpen[i]) &&                                        // opposite candles
         // 1st marubozu
         ta_realbody(inClose[i-1], inOpen[i-1]) > ta_candleaverage(BodyLong_rangeType, BodyLong_avgPeriod, BodyLong_factor, BodyLongPeriodTotal[1], inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) &&
         ta_uppershadow(inHigh[i-1], inClose[i-1], inOpen[i-1]) < ta_candleaverage(ShadowVeryShort_rangeType, ShadowVeryShort_avgPeriod, ShadowVeryShort_factor, ShadowVeryShortPeriodTotal[1], inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) &&
         ta_lowershadow(inLow[i-1], inClose[i-1], inOpen[i-1]) < ta_candleaverage(ShadowVeryShort_rangeType, ShadowVeryShort_avgPeriod, ShadowVeryShort_factor, ShadowVeryShortPeriodTotal[1], inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) &&
         // 2nd marubozu
         ta_realbody(inClose[i], inOpen[i]) > ta_candleaverage(BodyLong_rangeType, BodyLong_avgPeriod, BodyLong_factor, BodyLongPeriodTotal[0], inOpen[i], inHigh[i], inLow[i], inClose[i]) &&
         ta_uppershadow(inHigh[i], inClose[i], inOpen[i]) < ta_candleaverage(ShadowVeryShort_rangeType, ShadowVeryShort_avgPeriod, ShadowVeryShort_factor, ShadowVeryShortPeriodTotal[0], inOpen[i], inHigh[i], inLow[i], inClose[i]) &&
         ta_lowershadow(inLow[i], inClose[i], inOpen[i]) < ta_candleaverage(ShadowVeryShort_rangeType, ShadowVeryShort_avgPeriod, ShadowVeryShort_factor, ShadowVeryShortPeriodTotal[0], inOpen[i], inHigh[i], inLow[i], inClose[i]) &&
         // gap
         (
         ( ta_candlecolor(inClose[i-1], inOpen[i-1]) == -1 && ta_candlegapup(inLow[i], inHigh[i-1]) )
         ||
         ( ta_candlecolor(inClose[i-1], inOpen[i-1]) == 1 && ta_candlegapdown(inHigh[i], inLow[i-1]) )
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
      for (totIdx = 1; totIdx >= 0; --totIdx) {
         BodyLongPeriodTotal[totIdx] += ta_candlerange(BodyLong_rangeType, inOpen[i-totIdx], inHigh[i-totIdx], inLow[i-totIdx], inClose[i-totIdx])
         - ta_candlerange(BodyLong_rangeType, inOpen[BodyLongTrailingIdx-totIdx], inHigh[BodyLongTrailingIdx-totIdx], inLow[BodyLongTrailingIdx-totIdx], inClose[BodyLongTrailingIdx-totIdx]);
         ShadowVeryShortPeriodTotal[totIdx] += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i-totIdx], inHigh[i-totIdx], inLow[i-totIdx], inClose[i-totIdx])
         - ta_candlerange(ShadowVeryShort_rangeType, inOpen[ShadowVeryShortTrailingIdx-totIdx], inHigh[ShadowVeryShortTrailingIdx-totIdx], inLow[ShadowVeryShortTrailingIdx-totIdx], inClose[ShadowVeryShortTrailingIdx-totIdx]);
      }
      i++;
      ShadowVeryShortTrailingIdx++;
      BodyLongTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
