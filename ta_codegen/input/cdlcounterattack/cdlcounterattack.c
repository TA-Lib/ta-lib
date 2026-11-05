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
 *  010605 AC     Creation
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 *  110526 MF,CC  Carry the ranges of the previous bars instead of recomputing them
 *
 */

int cdlcounterattack_lookback(void)
{
   return max( Equal_avgPeriod, BodyLong_avgPeriod
   ) + 1;
}

TA_RetCode cdlcounterattack(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double EqualPeriodTotal;
   double BodyLongPeriodTotal[2];
   double BodyLongPeriodTotalRange0, BodyLongPeriodTotalTrailRange0, BodyLongPeriodTotalRange1, BodyLongPeriodTotalTrailRange1;
   int i, outIdx, EqualTrailingIdx, BodyLongTrailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdlcounterattack_lookback();

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
   EqualPeriodTotal = 0;
   EqualTrailingIdx = startIdx - Equal_avgPeriod;
   BodyLongPeriodTotal[1] = 0;
   BodyLongPeriodTotal[0] = 0;
   BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;

   i = EqualTrailingIdx;
   while( i < startIdx ) {
      EqualPeriodTotal += ta_candlerange(Equal_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]);
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
    * - first candle: long black (white)
    * - second candle: long white (black) with close equal to the prior close
    * The meaning of "equal" and "long" is specified with TA_SetCandleSettings
    */
   outIdx = 0;
   BodyLongPeriodTotalRange1 = ta_candlerange(BodyLong_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]);
   BodyLongPeriodTotalTrailRange1 = ta_candlerange(BodyLong_rangeType, inOpen[BodyLongTrailingIdx-1], inHigh[BodyLongTrailingIdx-1], inLow[BodyLongTrailingIdx-1], inClose[BodyLongTrailingIdx-1]);
   do
   {
      if( ta_candlecolor(inClose[i-1], inOpen[i-1]) == -ta_candlecolor(inClose[i], inOpen[i]) &&                                        // opposite candles
         ta_realbody(inClose[i-1], inOpen[i-1]) > ta_candleaverage(BodyLong_rangeType, BodyLong_avgPeriod, BodyLong_factor, BodyLongPeriodTotal[1], inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) &&     // 1st long
         ta_realbody(inClose[i], inOpen[i]) > ta_candleaverage(BodyLong_rangeType, BodyLong_avgPeriod, BodyLong_factor, BodyLongPeriodTotal[0], inOpen[i], inHigh[i], inLow[i], inClose[i]) &&         // 2nd long
         inClose[i] <= inClose[i-1] + ta_candleaverage(Equal_rangeType, Equal_avgPeriod, Equal_factor, EqualPeriodTotal, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) && // equal closes
         inClose[i] >= inClose[i-1] - ta_candleaverage(Equal_rangeType, Equal_avgPeriod, Equal_factor, EqualPeriodTotal, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1])
      ) {
         outInteger[outIdx++] = ta_candlecolor(inClose[i], inOpen[i]) * 100;
      }
      else {
         outInteger[outIdx++] = 0;
      }

      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      EqualPeriodTotal += ta_candlerange(Equal_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) - ta_candlerange(Equal_rangeType, inOpen[EqualTrailingIdx-1], inHigh[EqualTrailingIdx-1], inLow[EqualTrailingIdx-1], inClose[EqualTrailingIdx-1]);
      BodyLongPeriodTotalRange0 = ta_candlerange(BodyLong_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      BodyLongPeriodTotalTrailRange0 = ta_candlerange(BodyLong_rangeType, inOpen[BodyLongTrailingIdx], inHigh[BodyLongTrailingIdx], inLow[BodyLongTrailingIdx], inClose[BodyLongTrailingIdx]);
      BodyLongPeriodTotal[1] += BodyLongPeriodTotalRange1 - BodyLongPeriodTotalTrailRange1;
      BodyLongPeriodTotal[0] += BodyLongPeriodTotalRange0 - BodyLongPeriodTotalTrailRange0;
      BodyLongPeriodTotalRange1 = BodyLongPeriodTotalRange0;
      BodyLongPeriodTotalTrailRange1 = BodyLongPeriodTotalTrailRange0;
      i++;
      EqualTrailingIdx++;
      BodyLongTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
