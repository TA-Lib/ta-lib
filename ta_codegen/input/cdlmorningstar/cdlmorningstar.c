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
 *  100304 AC   Creation
 *
 */

int cdlmorningstar_lookback(double        optInPenetration)
{
   (void)optInPenetration;
   return max( BodyShort_avgPeriod, BodyLong_avgPeriod ) + 2;
}

TA_RetCode cdlmorningstar(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   double optInPenetration,
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double BodyShortPeriodTotal, BodyLongPeriodTotal, BodyShortPeriodTotal2;
   int i, outIdx, BodyShortTrailingIdx, BodyLongTrailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdlmorningstar_lookback(optInPenetration);

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
   BodyShortPeriodTotal2 = 0;
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
      BodyShortPeriodTotal2 += ta_candlerange(BodyShort_rangeType, inOpen[i+1], inHigh[i+1], inLow[i+1], inClose[i+1]);
      i++;
   }
   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - first candle: long black real body
    * - second candle: star (Short real body gapping down)
    * - third candle: white real body that moves well within the first candle's real body
    * The meaning of "short" and "long" is specified with TA_SetCandleSettings
    * The meaning of "moves well within" is specified with optInPenetration and "moves" should mean the real body should
    * not be short ("short" is specified with TA_SetCandleSettings) - Greg Morris wants it to be long, someone else want
    * it to be relatively long
    * outInteger is positive (1 to 100): morning star is always bullish;
    * the user should consider that a morning star is significant when it appears in a downtrend,
    * while this function does not consider the trend
    */
   outIdx = 0;
   do
   {
      if( ta_candlecolor(inClose[i-2], inOpen[i-2]) == -1 &&                                                            //           black
         ta_candlecolor(inClose[i], inOpen[i]) == 1 &&                                                               //          white real body
         ta_realbodygapdown(inOpen[i-1], inClose[i-1], inOpen[i-2], inClose[i-2]) &&                                                          //            gapping down
         inClose[i] > inClose[i-2] + ta_realbody(inClose[i-2], inOpen[i-2]) * optInPenetration &&                         //               closing well within 1st rb
         ta_realbody(inClose[i-2], inOpen[i-2]) > ta_candleaverage(BodyLong_rangeType, BodyLong_avgPeriod, BodyLong_factor, BodyLongPeriodTotal, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) &&         // 1st: long
         ta_realbody(inClose[i-1], inOpen[i-1]) <= ta_candleaverage(BodyShort_rangeType, BodyShort_avgPeriod, BodyShort_factor, BodyShortPeriodTotal, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) &&      // 2nd: short
         ta_realbody(inClose[i], inOpen[i]) > ta_candleaverage(BodyShort_rangeType, BodyShort_avgPeriod, BodyShort_factor, BodyShortPeriodTotal2, inOpen[i], inHigh[i], inLow[i], inClose[i])           // 3rd: longer than short
      ) {
         outInteger[outIdx++] = 100;
      }
      else {
         outInteger[outIdx++] = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      BodyLongPeriodTotal += ta_candlerange(BodyLong_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) - ta_candlerange(BodyLong_rangeType, inOpen[BodyLongTrailingIdx], inHigh[BodyLongTrailingIdx], inLow[BodyLongTrailingIdx], inClose[BodyLongTrailingIdx]);
      BodyShortPeriodTotal += ta_candlerange(BodyShort_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) - ta_candlerange(BodyShort_rangeType, inOpen[BodyShortTrailingIdx], inHigh[BodyShortTrailingIdx], inLow[BodyShortTrailingIdx], inClose[BodyShortTrailingIdx]);
      BodyShortPeriodTotal2 += ta_candlerange(BodyShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]) - ta_candlerange(BodyShort_rangeType, inOpen[BodyShortTrailingIdx+1], inHigh[BodyShortTrailingIdx+1], inLow[BodyShortTrailingIdx+1], inClose[BodyShortTrailingIdx+1]);
      i++;
      BodyLongTrailingIdx++;
      BodyShortTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
