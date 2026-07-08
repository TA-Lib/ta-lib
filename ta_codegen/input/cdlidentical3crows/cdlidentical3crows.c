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
 *  103104 AC   Creation
 *
 */

int cdlidentical3crows_lookback(void)
{
   return max( ShadowVeryShort_avgPeriod, Equal_avgPeriod
   ) + 2;
}

TA_RetCode cdlidentical3crows(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double ShadowVeryShortPeriodTotal[3];
   double EqualPeriodTotal[3];
   int i, outIdx, totIdx, ShadowVeryShortTrailingIdx, EqualTrailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdlidentical3crows_lookback();

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
   ShadowVeryShortPeriodTotal[2] = 0;
   ShadowVeryShortPeriodTotal[1] = 0;
   ShadowVeryShortPeriodTotal[0] = 0;
   ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
   EqualPeriodTotal[2] = 0;
   EqualPeriodTotal[1] = 0;
   EqualPeriodTotal[0] = 0;
   EqualTrailingIdx = startIdx - Equal_avgPeriod;

   i = ShadowVeryShortTrailingIdx;
   while( i < startIdx ) {
      ShadowVeryShortPeriodTotal[2] += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]);
      ShadowVeryShortPeriodTotal[1] += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]);
      ShadowVeryShortPeriodTotal[0] += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = EqualTrailingIdx;
   while( i < startIdx ) {
      EqualPeriodTotal[2] += ta_candlerange(Equal_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]);
      EqualPeriodTotal[1] += ta_candlerange(Equal_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]);
      i++;
   }
   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - three consecutive and declining black candlesticks
    * - each candle must have no or very short lower shadow
    * - each candle after the first must open at or very close to the prior candle's close
    * The meaning of "very short" is specified with TA_SetCandleSettings;
    * the meaning of "very close" is specified with TA_SetCandleSettings (Equal);
    * outInteger is negative (-1 to -100): identical three crows is always bearish;
    * the user should consider that identical 3 crows is significant when it appears after a mature advance or at high levels,
    * while this function does not consider it
    */
   outIdx = 0;
   do
   {
      if( ta_candlecolor(inClose[i-2], inOpen[i-2]) == -1 &&                                    // 1st black
         // very short lower shadow
         ta_lowershadow(inLow[i-2], inClose[i-2], inOpen[i-2]) < ta_candleaverage(ShadowVeryShort_rangeType, ShadowVeryShort_avgPeriod, ShadowVeryShort_factor, ShadowVeryShortPeriodTotal[2], inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) &&
         ta_candlecolor(inClose[i-1], inOpen[i-1]) == -1 &&                                    // 2nd black
         // very short lower shadow
         ta_lowershadow(inLow[i-1], inClose[i-1], inOpen[i-1]) < ta_candleaverage(ShadowVeryShort_rangeType, ShadowVeryShort_avgPeriod, ShadowVeryShort_factor, ShadowVeryShortPeriodTotal[1], inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) &&
         ta_candlecolor(inClose[i], inOpen[i]) == -1 &&                                      // 3rd black
         // very short lower shadow
         ta_lowershadow(inLow[i], inClose[i], inOpen[i]) < ta_candleaverage(ShadowVeryShort_rangeType, ShadowVeryShort_avgPeriod, ShadowVeryShort_factor, ShadowVeryShortPeriodTotal[0], inOpen[i], inHigh[i], inLow[i], inClose[i]) &&
         inClose[i-2] > inClose[i-1] &&                                  // three declining
         inClose[i-1] > inClose[i] &&
         // 2nd black opens very close to 1st close
         inOpen[i-1] <= inClose[i-2] + ta_candleaverage(Equal_rangeType, Equal_avgPeriod, Equal_factor, EqualPeriodTotal[2], inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) &&
         inOpen[i-1] >= inClose[i-2] - ta_candleaverage(Equal_rangeType, Equal_avgPeriod, Equal_factor, EqualPeriodTotal[2], inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) &&
         // 3rd black opens very close to 2nd close
         inOpen[i] <= inClose[i-1] + ta_candleaverage(Equal_rangeType, Equal_avgPeriod, Equal_factor, EqualPeriodTotal[1], inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) &&
         inOpen[i] >= inClose[i-1] - ta_candleaverage(Equal_rangeType, Equal_avgPeriod, Equal_factor, EqualPeriodTotal[1], inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1])
      ) {
         outInteger[outIdx++] = -100;
      }
      else {
         outInteger[outIdx++] = 0;
      }

      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      for (totIdx = 2; totIdx >= 0; --totIdx)
         ShadowVeryShortPeriodTotal[totIdx] += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i-totIdx], inHigh[i-totIdx], inLow[i-totIdx], inClose[i-totIdx])
      - ta_candlerange(ShadowVeryShort_rangeType, inOpen[ShadowVeryShortTrailingIdx-totIdx], inHigh[ShadowVeryShortTrailingIdx-totIdx], inLow[ShadowVeryShortTrailingIdx-totIdx], inClose[ShadowVeryShortTrailingIdx-totIdx]);
      for (totIdx = 2; totIdx >= 1; --totIdx)
         EqualPeriodTotal[totIdx] += ta_candlerange(Equal_rangeType, inOpen[i-totIdx], inHigh[i-totIdx], inLow[i-totIdx], inClose[i-totIdx])
      - ta_candlerange(Equal_rangeType, inOpen[EqualTrailingIdx-totIdx], inHigh[EqualTrailingIdx-totIdx], inLow[EqualTrailingIdx-totIdx], inClose[EqualTrailingIdx-totIdx]);
      i++;
      ShadowVeryShortTrailingIdx++;
      EqualTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
