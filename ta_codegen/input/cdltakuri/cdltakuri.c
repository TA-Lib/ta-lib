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

int cdltakuri_lookback(void)
{
   return max( max( BodyDoji_avgPeriod, ShadowVeryShort_avgPeriod ),
      ShadowVeryLong_avgPeriod
   );
}

TA_RetCode cdltakuri(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double BodyDojiPeriodTotal, ShadowVeryShortPeriodTotal, ShadowVeryLongPeriodTotal;
   int i, outIdx, BodyDojiTrailingIdx, ShadowVeryShortTrailingIdx, ShadowVeryLongTrailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdltakuri_lookback();

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
   BodyDojiPeriodTotal = 0;
   BodyDojiTrailingIdx = startIdx - BodyDoji_avgPeriod;
   ShadowVeryShortPeriodTotal = 0;
   ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
   ShadowVeryLongPeriodTotal = 0;
   ShadowVeryLongTrailingIdx = startIdx - ShadowVeryLong_avgPeriod;

   i = BodyDojiTrailingIdx;
   while( i < startIdx ) {
      BodyDojiPeriodTotal += ta_candlerange(BodyDoji_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = ShadowVeryShortTrailingIdx;
   while( i < startIdx ) {
      ShadowVeryShortPeriodTotal += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = ShadowVeryLongTrailingIdx;
   while( i < startIdx ) {
      ShadowVeryLongPeriodTotal += ta_candlerange(ShadowVeryLong_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }

   /* Proceed with the calculation for the requested range.
    *
    * Must have:
    * - doji body
    * - open and close at the high of the day = no or very short upper shadow
    * - very long lower shadow
    * The meaning of "doji", "very short" and "very long" is specified with TA_SetCandleSettings
    * outInteger is always positive (1 to 100) but this does not mean it is bullish: takuri must be considered
    * relatively to the trend
    */
   outIdx = 0;

   do
   {
      if( ta_realbody(inClose[i], inOpen[i]) <= ta_candleaverage(BodyDoji_rangeType, BodyDoji_avgPeriod, BodyDoji_factor, BodyDojiPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i]) &&
         ta_uppershadow(inHigh[i], inClose[i], inOpen[i]) < ta_candleaverage(ShadowVeryShort_rangeType, ShadowVeryShort_avgPeriod, ShadowVeryShort_factor, ShadowVeryShortPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i]) &&
         ta_lowershadow(inLow[i], inClose[i], inOpen[i]) > ta_candleaverage(ShadowVeryLong_rangeType, ShadowVeryLong_avgPeriod, ShadowVeryLong_factor, ShadowVeryLongPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i])
      ) {
         outInteger[outIdx++] = 100;
      }
      else {
         outInteger[outIdx++] = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      BodyDojiPeriodTotal += ta_candlerange(BodyDoji_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]) - ta_candlerange(BodyDoji_rangeType, inOpen[BodyDojiTrailingIdx], inHigh[BodyDojiTrailingIdx], inLow[BodyDojiTrailingIdx], inClose[BodyDojiTrailingIdx]);
      ShadowVeryShortPeriodTotal += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i])
      - ta_candlerange(ShadowVeryShort_rangeType, inOpen[ShadowVeryShortTrailingIdx], inHigh[ShadowVeryShortTrailingIdx], inLow[ShadowVeryShortTrailingIdx], inClose[ShadowVeryShortTrailingIdx]);
      ShadowVeryLongPeriodTotal += ta_candlerange(ShadowVeryLong_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i])
      - ta_candlerange(ShadowVeryLong_rangeType, inOpen[ShadowVeryLongTrailingIdx], inHigh[ShadowVeryLongTrailingIdx], inLow[ShadowVeryLongTrailingIdx], inClose[ShadowVeryLongTrailingIdx]);
      i++;
      BodyDojiTrailingIdx++;
      ShadowVeryShortTrailingIdx++;
      ShadowVeryLongTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
