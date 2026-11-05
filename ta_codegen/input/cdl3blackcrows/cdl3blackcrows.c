/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AC       Angelo Ciceri
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  103004 AC     Creation
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 *  103126 MF,CC  Nest the cheap, selective pre-check conjuncts in an outer if
 *  110526 MF,CC  Carry the ranges of the two previous bars instead of recomputing them
 *
 */
int cdl3blackcrows_lookback(void)
{
   return ShadowVeryShort_avgPeriod + 3;
}

TA_RetCode cdl3blackcrows(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double ShadowVeryShortPeriodTotal[3];
   double range1, range2, trailRange1, trailRange2, range0, trailRange0;
   int i, outIdx, ShadowVeryShortTrailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdl3blackcrows_lookback();

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

   i = ShadowVeryShortTrailingIdx;
   while( i < startIdx ) {
      ShadowVeryShortPeriodTotal[2] += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]);
      ShadowVeryShortPeriodTotal[1] += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]);
      ShadowVeryShortPeriodTotal[0] += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = startIdx;
   range2 = ta_candlerange(ShadowVeryShort_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]);
   range1 = ta_candlerange(ShadowVeryShort_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]);
   trailRange2 = ta_candlerange(ShadowVeryShort_rangeType, inOpen[ShadowVeryShortTrailingIdx-2], inHigh[ShadowVeryShortTrailingIdx-2], inLow[ShadowVeryShortTrailingIdx-2], inClose[ShadowVeryShortTrailingIdx-2]);
   trailRange1 = ta_candlerange(ShadowVeryShort_rangeType, inOpen[ShadowVeryShortTrailingIdx-1], inHigh[ShadowVeryShortTrailingIdx-1], inLow[ShadowVeryShortTrailingIdx-1], inClose[ShadowVeryShortTrailingIdx-1]);

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - three consecutive and declining black candlesticks
    * - each candle must have no or very short lower shadow
    * - each candle after the first must open within the prior candle's real body
    * - the first candle's close should be under the prior white candle's high
    * The meaning of "very short" is specified with TA_SetCandleSettings
    */
   outIdx = 0;
   do
   {
      if( ta_candlecolor(inClose[i-3], inOpen[i-3]) == 1 &&                         // white
         ta_candlecolor(inClose[i-2], inOpen[i-2]) == -1 &&                        // 1st black
         ta_candlecolor(inClose[i-1], inOpen[i-1]) == -1                        // 2nd black
      ) {
         if( ta_candlecolor(inClose[i], inOpen[i]) == -1 &&                            // 3rd black
            inOpen[i-1] < inOpen[i-2] && inOpen[i-1] > inClose[i-2] &&                // 2nd black opens within 1st black's rb
            inOpen[i] < inOpen[i-1] && inOpen[i] > inClose[i-1] &&                    // 3rd black opens within 2nd black's rb
            inHigh[i-3] > inClose[i-2] &&                                             // 1st black closes under prior candle's high
            inClose[i-2] > inClose[i-1] &&                                            // three declining
            inClose[i-1] > inClose[i] &&                                              // three declining
            // very short lower shadow
            ta_lowershadow(inLow[i-2], inClose[i-2], inOpen[i-2]) < ta_candleaverage(ShadowVeryShort_rangeType, ShadowVeryShort_avgPeriod, ShadowVeryShort_factor, ShadowVeryShortPeriodTotal[2], inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) &&
            // very short lower shadow
            ta_lowershadow(inLow[i-1], inClose[i-1], inOpen[i-1]) < ta_candleaverage(ShadowVeryShort_rangeType, ShadowVeryShort_avgPeriod, ShadowVeryShort_factor, ShadowVeryShortPeriodTotal[1], inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) &&
            // very short lower shadow
            ta_lowershadow(inLow[i], inClose[i], inOpen[i]) < ta_candleaverage(ShadowVeryShort_rangeType, ShadowVeryShort_avgPeriod, ShadowVeryShort_factor, ShadowVeryShortPeriodTotal[0], inOpen[i], inHigh[i], inLow[i], inClose[i])
         ) {
            outInteger[outIdx++] = -100;
         }
         else {
            outInteger[outIdx++] = 0;
         }
      }
      else {
         outInteger[outIdx++] = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      /* A bar's ranges for i-1 and i-2 are the ones computed one and two bars ago. */
      range0 = ta_candlerange(ShadowVeryShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      trailRange0 = ta_candlerange(ShadowVeryShort_rangeType, inOpen[ShadowVeryShortTrailingIdx], inHigh[ShadowVeryShortTrailingIdx], inLow[ShadowVeryShortTrailingIdx], inClose[ShadowVeryShortTrailingIdx]);
      ShadowVeryShortPeriodTotal[2] += range2 - trailRange2;
      ShadowVeryShortPeriodTotal[1] += range1 - trailRange1;
      ShadowVeryShortPeriodTotal[0] += range0 - trailRange0;
      range2 = range1;
      range1 = range0;
      trailRange2 = trailRange1;
      trailRange1 = trailRange0;
      i++;
      ShadowVeryShortTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
