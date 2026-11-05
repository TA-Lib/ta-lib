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
 *  121104 AC     Creation
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 *  103126 MF,CC  Nest the cheap, selective pre-check conjuncts in an outer if
 *  110526 MF,CC  Carry the ranges of the previous bars instead of recomputing them
 *
 */

int cdl3linestrike_lookback(void)
{
   return Near_avgPeriod + 3;
}

TA_RetCode cdl3linestrike(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double NearPeriodTotal[4];
   double NearPeriodTotalRange2, NearPeriodTotalTrailRange2, NearPeriodTotalRange3, NearPeriodTotalTrailRange3;
   int i, outIdx, NearTrailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdl3linestrike_lookback();

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
   NearPeriodTotal[3] = 0;
   NearPeriodTotal[2] = 0;
   NearTrailingIdx = startIdx - Near_avgPeriod;

   i = NearTrailingIdx;
   while( i < startIdx ) {
      NearPeriodTotal[3] += ta_candlerange(Near_rangeType, inOpen[i-3], inHigh[i-3], inLow[i-3], inClose[i-3]);
      NearPeriodTotal[2] += ta_candlerange(Near_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]);
      i++;
   }
   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - three white soldiers (three black crows): three white (black) candlesticks with consecutively higher (lower) closes,
    * each opening within or near the previous real body
    * - fourth candle: black (white) candle that opens above (below) prior candle's close and closes below (above)
    * the first candle's open
    * The meaning of "near" is specified with TA_SetCandleSettings
    */
   outIdx = 0;
   NearPeriodTotalRange3 = ta_candlerange(Near_rangeType, inOpen[i-3], inHigh[i-3], inLow[i-3], inClose[i-3]);
   NearPeriodTotalTrailRange3 = ta_candlerange(Near_rangeType, inOpen[NearTrailingIdx-3], inHigh[NearTrailingIdx-3], inLow[NearTrailingIdx-3], inClose[NearTrailingIdx-3]);
   do
   {
      if( ta_candlecolor(inClose[i-3], inOpen[i-3]) == ta_candlecolor(inClose[i-2], inOpen[i-2]) &&                                   // three with same color
         ta_candlecolor(inClose[i-2], inOpen[i-2]) == ta_candlecolor(inClose[i-1], inOpen[i-1]) &&
         ta_candlecolor(inClose[i], inOpen[i]) == -ta_candlecolor(inClose[i-1], inOpen[i-1])                                    // 4th opposite color
      ) {
         if( // 2nd opens within/near 1st rb
            inOpen[i-2] >= min( inOpen[i-3], inClose[i-3] ) - ta_candleaverage(Near_rangeType, Near_avgPeriod, Near_factor, NearPeriodTotal[3], inOpen[i-3], inHigh[i-3], inLow[i-3], inClose[i-3]) &&
            inOpen[i-2] <= max( inOpen[i-3], inClose[i-3] ) + ta_candleaverage(Near_rangeType, Near_avgPeriod, Near_factor, NearPeriodTotal[3], inOpen[i-3], inHigh[i-3], inLow[i-3], inClose[i-3]) &&
            // 3rd opens within/near 2nd rb
            inOpen[i-1] >= min( inOpen[i-2], inClose[i-2] ) - ta_candleaverage(Near_rangeType, Near_avgPeriod, Near_factor, NearPeriodTotal[2], inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) &&
            inOpen[i-1] <= max( inOpen[i-2], inClose[i-2] ) + ta_candleaverage(Near_rangeType, Near_avgPeriod, Near_factor, NearPeriodTotal[2], inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) &&
            (
            (   // if three white
            ta_candlecolor(inClose[i-1], inOpen[i-1]) == 1 &&
            inClose[i-1] > inClose[i-2] && inClose[i-2] > inClose[i-3] &&           // consecutive higher closes
            inOpen[i] > inClose[i-1] &&                                             // 4th opens above prior close
            inClose[i] < inOpen[i-3]                                                // 4th closes below 1st open
         ) ||
            (   // if three black
            ta_candlecolor(inClose[i-1], inOpen[i-1]) == -1 &&
            inClose[i-1] < inClose[i-2] && inClose[i-2] < inClose[i-3] &&           // consecutive lower closes
            inOpen[i] < inClose[i-1] &&                                             // 4th opens below prior close
            inClose[i] > inOpen[i-3]                                                // 4th closes above 1st open
         )
         )
         ) {
            outInteger[outIdx++] = ta_candlecolor(inClose[i-1], inOpen[i-1]) * 100;
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
      NearPeriodTotalRange2 = ta_candlerange(Near_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]);
      NearPeriodTotalTrailRange2 = ta_candlerange(Near_rangeType, inOpen[NearTrailingIdx-2], inHigh[NearTrailingIdx-2], inLow[NearTrailingIdx-2], inClose[NearTrailingIdx-2]);
      NearPeriodTotal[3] += NearPeriodTotalRange3 - NearPeriodTotalTrailRange3;
      NearPeriodTotal[2] += NearPeriodTotalRange2 - NearPeriodTotalTrailRange2;
      NearPeriodTotalRange3 = NearPeriodTotalRange2;
      NearPeriodTotalTrailRange3 = NearPeriodTotalTrailRange2;
      i++;
      NearTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
