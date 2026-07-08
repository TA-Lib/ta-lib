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

int cdlseparatinglines_lookback(void)
{
   return max( max( ShadowVeryShort_avgPeriod, BodyLong_avgPeriod ),
      Equal_avgPeriod
   ) + 1;
}

TA_RetCode cdlseparatinglines(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double ShadowVeryShortPeriodTotal, BodyLongPeriodTotal, EqualPeriodTotal;
   int i, outIdx, ShadowVeryShortTrailingIdx, BodyLongTrailingIdx, EqualTrailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdlseparatinglines_lookback();

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
   BodyLongPeriodTotal = 0;
   BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
   EqualPeriodTotal = 0;
   EqualTrailingIdx = startIdx - Equal_avgPeriod;

   i = ShadowVeryShortTrailingIdx;
   while( i < startIdx ) {
      ShadowVeryShortPeriodTotal += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = BodyLongTrailingIdx;
   while( i < startIdx ) {
      BodyLongPeriodTotal += ta_candlerange(BodyLong_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = EqualTrailingIdx;
   while( i < startIdx ) {
      EqualPeriodTotal += ta_candlerange(Equal_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]);
      i++;
   }
   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - first candle: black (white) candle
    * - second candle: bullish (bearish) belt hold with the same open as the prior candle
    * The meaning of "long body" and "very short shadow" of the belt hold is specified with TA_SetCandleSettings
    * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish;
    * the user should consider that separating lines is significant when coming in a trend and the belt hold has
    * the same direction of the trend, while this function does not consider it
    */
   outIdx = 0;
   do
   {
      if( ta_candlecolor(inClose[i-1], inOpen[i-1]) == -ta_candlecolor(inClose[i], inOpen[i]) &&                                        // opposite candles
         inOpen[i] <= inOpen[i-1] + ta_candleaverage(Equal_rangeType, Equal_avgPeriod, Equal_factor, EqualPeriodTotal, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) &&   // same open
         inOpen[i] >= inOpen[i-1] - ta_candleaverage(Equal_rangeType, Equal_avgPeriod, Equal_factor, EqualPeriodTotal, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) &&
         ta_realbody(inClose[i], inOpen[i]) > ta_candleaverage(BodyLong_rangeType, BodyLong_avgPeriod, BodyLong_factor, BodyLongPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i]) &&         // belt hold: long body
         (
         ( ta_candlecolor(inClose[i], inOpen[i]) == 1 &&                                               // with no lower shadow if bullish
         ta_lowershadow(inLow[i], inClose[i], inOpen[i]) < ta_candleaverage(ShadowVeryShort_rangeType, ShadowVeryShort_avgPeriod, ShadowVeryShort_factor, ShadowVeryShortPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i])
      )
         ||
         ( ta_candlecolor(inClose[i], inOpen[i]) == -1 &&                                              // with no upper shadow if bearish
         ta_uppershadow(inHigh[i], inClose[i], inOpen[i]) < ta_candleaverage(ShadowVeryShort_rangeType, ShadowVeryShort_avgPeriod, ShadowVeryShort_factor, ShadowVeryShortPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i])
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
      ShadowVeryShortPeriodTotal += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i])
      - ta_candlerange(ShadowVeryShort_rangeType, inOpen[ShadowVeryShortTrailingIdx], inHigh[ShadowVeryShortTrailingIdx], inLow[ShadowVeryShortTrailingIdx], inClose[ShadowVeryShortTrailingIdx]);
      BodyLongPeriodTotal += ta_candlerange(BodyLong_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]) - ta_candlerange(BodyLong_rangeType, inOpen[BodyLongTrailingIdx], inHigh[BodyLongTrailingIdx], inLow[BodyLongTrailingIdx], inClose[BodyLongTrailingIdx]);
      EqualPeriodTotal += ta_candlerange(Equal_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) - ta_candlerange(Equal_rangeType, inOpen[EqualTrailingIdx-1], inHigh[EqualTrailingIdx-1], inLow[EqualTrailingIdx-1], inClose[EqualTrailingIdx-1]);
      i++;
      ShadowVeryShortTrailingIdx++;
      BodyLongTrailingIdx++;
      EqualTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
