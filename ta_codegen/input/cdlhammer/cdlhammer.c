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
 *  102304 AC   Creation
 *
 */

int cdlhammer_lookback(void)
{
   return max( max( max( BodyShort_avgPeriod, ShadowLong_avgPeriod ),
      ShadowVeryShort_avgPeriod ),
      Near_avgPeriod
   ) + 1;
}

TA_RetCode cdlhammer(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double BodyPeriodTotal, ShadowLongPeriodTotal, ShadowVeryShortPeriodTotal, NearPeriodTotal;
   int i, outIdx, BodyTrailingIdx, ShadowLongTrailingIdx, ShadowVeryShortTrailingIdx, NearTrailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdlhammer_lookback();

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
   BodyPeriodTotal = 0;
   BodyTrailingIdx = startIdx - BodyShort_avgPeriod;
   ShadowLongPeriodTotal = 0;
   ShadowLongTrailingIdx = startIdx - ShadowLong_avgPeriod;
   ShadowVeryShortPeriodTotal = 0;
   ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
   NearPeriodTotal = 0;
   NearTrailingIdx = startIdx -1 - Near_avgPeriod;

   i = BodyTrailingIdx;
   while( i < startIdx ) {
      BodyPeriodTotal += ta_candlerange(BodyShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = ShadowLongTrailingIdx;
   while( i < startIdx ) {
      ShadowLongPeriodTotal += ta_candlerange(ShadowLong_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = ShadowVeryShortTrailingIdx;
   while( i < startIdx ) {
      ShadowVeryShortPeriodTotal += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = NearTrailingIdx;
   while( i < startIdx-1 ) {
      NearPeriodTotal += ta_candlerange(Near_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - small real body
    * - long lower shadow
    * - no, or very short, upper shadow
    * - body below or near the lows of the previous candle
    * The meaning of "short", "long" and "near the lows" is specified with TA_SetCandleSettings;
    * outInteger is positive (1 to 100): hammer is always bullish;
    * the user should consider that a hammer must appear in a downtrend, while this function does not consider it
    */
   outIdx = 0;
   do
   {
      if( ta_realbody(inClose[i], inOpen[i]) < ta_candleaverage(BodyShort_rangeType, BodyShort_avgPeriod, BodyShort_factor, BodyPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i]) &&                        // small rb
         ta_lowershadow(inLow[i], inClose[i], inOpen[i]) > ta_candleaverage(ShadowLong_rangeType, ShadowLong_avgPeriod, ShadowLong_factor, ShadowLongPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i]) &&              // long lower shadow
         ta_uppershadow(inHigh[i], inClose[i], inOpen[i]) < ta_candleaverage(ShadowVeryShort_rangeType, ShadowVeryShort_avgPeriod, ShadowVeryShort_factor, ShadowVeryShortPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i]) &&    // very short upper shadow
         min( inClose[i], inOpen[i] ) <= inLow[i-1] + ta_candleaverage(Near_rangeType, Near_avgPeriod, Near_factor, NearPeriodTotal, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1])  // rb near the prior candle's lows
      ) {
         outInteger[outIdx++] = 100;
      }
      else {
         outInteger[outIdx++] = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      BodyPeriodTotal += ta_candlerange(BodyShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i])
      - ta_candlerange(BodyShort_rangeType, inOpen[BodyTrailingIdx], inHigh[BodyTrailingIdx], inLow[BodyTrailingIdx], inClose[BodyTrailingIdx]);
      ShadowLongPeriodTotal += ta_candlerange(ShadowLong_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i])
      - ta_candlerange(ShadowLong_rangeType, inOpen[ShadowLongTrailingIdx], inHigh[ShadowLongTrailingIdx], inLow[ShadowLongTrailingIdx], inClose[ShadowLongTrailingIdx]);
      ShadowVeryShortPeriodTotal += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i])
      - ta_candlerange(ShadowVeryShort_rangeType, inOpen[ShadowVeryShortTrailingIdx], inHigh[ShadowVeryShortTrailingIdx], inLow[ShadowVeryShortTrailingIdx], inClose[ShadowVeryShortTrailingIdx]);
      NearPeriodTotal += ta_candlerange(Near_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1])
      - ta_candlerange(Near_rangeType, inOpen[NearTrailingIdx], inHigh[NearTrailingIdx], inLow[NearTrailingIdx], inClose[NearTrailingIdx]);
      i++;
      BodyTrailingIdx++;
      ShadowLongTrailingIdx++;
      ShadowVeryShortTrailingIdx++;
      NearTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
