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
 *  121104 AC   Creation
 *
 */

int cdl3inside_lookback(void)
{
   return max( BodyShort_avgPeriod, BodyLong_avgPeriod ) + 2;
}

TA_RetCode cdl3inside(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double BodyShortPeriodTotal, BodyLongPeriodTotal;
   int i, outIdx, BodyShortTrailingIdx, BodyLongTrailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdl3inside_lookback();

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
   BodyShortPeriodTotal = 0;
   BodyLongTrailingIdx = startIdx -2 - BodyLong_avgPeriod;
   BodyShortTrailingIdx = startIdx -1 - BodyShort_avgPeriod;

   i = BodyLongTrailingIdx;
   while( i < startIdx-2 ) {
      BodyLongPeriodTotal += ta_candlerange(BodyLong_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = BodyShortTrailingIdx;
   while( i < startIdx-1 ) {
      BodyShortPeriodTotal += ta_candlerange(BodyShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - first candle: long white (black) real body
    * - second candle: short real body totally engulfed by the first
    * - third candle: black (white) candle that closes lower (higher) than the first candle's open
    * The meaning of "short" and "long" is specified with TA_SetCandleSettings
    * outInteger is positive (1 to 100) for the three inside up or negative (-1 to -100) for the three inside down;
    * the user should consider that a three inside up is significant when it appears in a downtrend and a three inside
    * down is significant when it appears in an uptrend, while this function does not consider the trend
    */
   outIdx = 0;
   do
   {
      if( max( inClose[i-1], inOpen[i-1] ) < max( inClose[i-2], inOpen[i-2] ) &&                  //      engulfed by 1st
         min( inClose[i-1], inOpen[i-1] ) > min( inClose[i-2], inOpen[i-2] ) &&
         ( ( ta_candlecolor(inClose[i-2], inOpen[i-2]) == 1 && ta_candlecolor(inClose[i], inOpen[i]) == -1 && inClose[i] < inOpen[i-2] )   // 3rd: opposite to 1st
         ||                                                                                    //      and closing out
         ( ta_candlecolor(inClose[i-2], inOpen[i-2]) == -1 && ta_candlecolor(inClose[i], inOpen[i]) == 1 && inClose[i] > inOpen[i-2] )
      ) &&
         ta_realbody(inClose[i-2], inOpen[i-2]) > ta_candleaverage(BodyLong_rangeType, BodyLong_avgPeriod, BodyLong_factor, BodyLongPeriodTotal, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) &&         // 1st: long
         ta_realbody(inClose[i-1], inOpen[i-1]) <= ta_candleaverage(BodyShort_rangeType, BodyShort_avgPeriod, BodyShort_factor, BodyShortPeriodTotal, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1])       // 2nd: short
      ) {
         outInteger[outIdx++] = -ta_candlecolor(inClose[i-2], inOpen[i-2]) * 100;
      }
      else {
         outInteger[outIdx++] = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      BodyLongPeriodTotal += ta_candlerange(BodyLong_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) - ta_candlerange(BodyLong_rangeType, inOpen[BodyLongTrailingIdx], inHigh[BodyLongTrailingIdx], inLow[BodyLongTrailingIdx], inClose[BodyLongTrailingIdx]);
      BodyShortPeriodTotal += ta_candlerange(BodyShort_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) - ta_candlerange(BodyShort_rangeType, inOpen[BodyShortTrailingIdx], inHigh[BodyShortTrailingIdx], inLow[BodyShortTrailingIdx], inClose[BodyShortTrailingIdx]);
      i++;
      BodyLongTrailingIdx++;
      BodyShortTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
