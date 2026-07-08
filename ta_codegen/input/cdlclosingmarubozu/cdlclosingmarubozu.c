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
 *  022005 AC   Creation
 *
 */

int cdlclosingmarubozu_lookback(void)
{
   return max( BodyLong_avgPeriod, ShadowVeryShort_avgPeriod );
}

TA_RetCode cdlclosingmarubozu(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double BodyLongPeriodTotal, ShadowVeryShortPeriodTotal;
   int i, outIdx, BodyLongTrailingIdx, ShadowVeryShortTrailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdlclosingmarubozu_lookback();

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
   ShadowVeryShortPeriodTotal = 0;
   ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;

   i = BodyLongTrailingIdx;
   while( i < startIdx ) {
      BodyLongPeriodTotal += ta_candlerange(BodyLong_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = ShadowVeryShortTrailingIdx;
   while( i < startIdx ) {
      ShadowVeryShortPeriodTotal += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - long white (black) real body
    * - no or very short upper (lower) shadow
    * The meaning of "long" and "very short" is specified with TA_SetCandleSettings
    * outInteger is positive (1 to 100) when white (bullish), negative (-1 to -100) when black (bearish)
    */
   outIdx = 0;
   do
   {
      if( ta_realbody(inClose[i], inOpen[i]) > ta_candleaverage(BodyLong_rangeType, BodyLong_avgPeriod, BodyLong_factor, BodyLongPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i]) &&             // long body
         (
         ( // white body and very short lower shadow
         ta_candlecolor(inClose[i], inOpen[i]) == 1 &&
         ta_uppershadow(inHigh[i], inClose[i], inOpen[i]) < ta_candleaverage(ShadowVeryShort_rangeType, ShadowVeryShort_avgPeriod, ShadowVeryShort_factor, ShadowVeryShortPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i])
      ) ||
         ( // black body and very short upper shadow
         ta_candlecolor(inClose[i], inOpen[i]) == -1 &&
         ta_lowershadow(inLow[i], inClose[i], inOpen[i]) < ta_candleaverage(ShadowVeryShort_rangeType, ShadowVeryShort_avgPeriod, ShadowVeryShort_factor, ShadowVeryShortPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i])
      )
      ) ) {
         outInteger[outIdx++] = ta_candlecolor(inClose[i], inOpen[i]) * 100;
      }
      else {
         outInteger[outIdx++] = 0;
      }

      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      BodyLongPeriodTotal += ta_candlerange(BodyLong_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]) - ta_candlerange(BodyLong_rangeType, inOpen[BodyLongTrailingIdx], inHigh[BodyLongTrailingIdx], inLow[BodyLongTrailingIdx], inClose[BodyLongTrailingIdx]);
      ShadowVeryShortPeriodTotal += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i])
      - ta_candlerange(ShadowVeryShort_rangeType, inOpen[ShadowVeryShortTrailingIdx], inHigh[ShadowVeryShortTrailingIdx], inLow[ShadowVeryShortTrailingIdx], inClose[ShadowVeryShortTrailingIdx]);
      i++;
      BodyLongTrailingIdx++;
      ShadowVeryShortTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
