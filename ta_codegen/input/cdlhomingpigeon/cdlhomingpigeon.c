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
 *  032005 AC   Creation
 *
 */
int cdlhomingpigeon_lookback(void)
{
   return max( BodyShort_avgPeriod, BodyLong_avgPeriod ) + 1;
}

TA_RetCode cdlhomingpigeon(int startIdx, int endIdx,
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

   lookbackTotal = cdlhomingpigeon_lookback();

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
   BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
   BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;

   i = BodyLongTrailingIdx;
   while( i < startIdx ) {
      BodyLongPeriodTotal += ta_candlerange(BodyLong_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]);
      i++;
   }
   i = BodyShortTrailingIdx;
   while( i < startIdx ) {
      BodyShortPeriodTotal += ta_candlerange(BodyShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - first candle: long black candle
    * - second candle: short black real body completely inside the previous day's body
    * The meaning of "short" and "long" is specified with TA_SetCandleSettings
    * outInteger is positive (1 to 100): homing pigeon is always bullish;
    * the user should consider that homing pigeon is significant when it appears in a downtrend,
    * while this function does not consider the trend
    */
   outIdx = 0;
   do
   {
      if( ta_candlecolor(inClose[i-1], inOpen[i-1]) == -1 &&                                                            // 1st black
         ta_candlecolor(inClose[i], inOpen[i]) == -1 &&                                                              // 2nd black
         ta_realbody(inClose[i-1], inOpen[i-1]) > ta_candleaverage(BodyLong_rangeType, BodyLong_avgPeriod, BodyLong_factor, BodyLongPeriodTotal, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) &&         // 1st long
         ta_realbody(inClose[i], inOpen[i]) <= ta_candleaverage(BodyShort_rangeType, BodyShort_avgPeriod, BodyShort_factor, BodyShortPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i]) &&          // 2nd short
         inOpen[i] < inOpen[i-1] &&                                                              // 2nd engulfed by 1st
         inClose[i] > inClose[i-1]
      ) {
         outInteger[outIdx++] = 100;
      }
      else {
         outInteger[outIdx++] = 0;
      }

      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      BodyLongPeriodTotal += ta_candlerange(BodyLong_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) - ta_candlerange(BodyLong_rangeType, inOpen[BodyLongTrailingIdx-1], inHigh[BodyLongTrailingIdx-1], inLow[BodyLongTrailingIdx-1], inClose[BodyLongTrailingIdx-1]);
      BodyShortPeriodTotal += ta_candlerange(BodyShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]) - ta_candlerange(BodyShort_rangeType, inOpen[BodyShortTrailingIdx], inHigh[BodyShortTrailingIdx], inLow[BodyShortTrailingIdx], inClose[BodyShortTrailingIdx]);
      i++;
      BodyLongTrailingIdx++;
      BodyShortTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
