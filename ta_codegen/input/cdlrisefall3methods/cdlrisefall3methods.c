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
 *  020605 AC   Creation
 *
 */

int cdlrisefall3methods_lookback(void)
{
   return max( BodyShort_avgPeriod, BodyLong_avgPeriod ) + 4;
}

TA_RetCode cdlrisefall3methods(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double BodyPeriodTotal[5];
   int i, outIdx, totIdx, BodyShortTrailingIdx, BodyLongTrailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdlrisefall3methods_lookback();

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
   BodyPeriodTotal[4] = 0;
   BodyPeriodTotal[3] = 0;
   BodyPeriodTotal[2] = 0;
   BodyPeriodTotal[1] = 0;
   BodyPeriodTotal[0] = 0;
   BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
   BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;

   i = BodyShortTrailingIdx;
   while( i < startIdx ) {
      BodyPeriodTotal[3] += ta_candlerange(BodyShort_rangeType, inOpen[i-3], inHigh[i-3], inLow[i-3], inClose[i-3]);
      BodyPeriodTotal[2] += ta_candlerange(BodyShort_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]);
      BodyPeriodTotal[1] += ta_candlerange(BodyShort_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]);
      i++;
   }
   i = BodyLongTrailingIdx;
   while( i < startIdx ) {
      BodyPeriodTotal[4] += ta_candlerange(BodyLong_rangeType, inOpen[i-4], inHigh[i-4], inLow[i-4], inClose[i-4]);
      BodyPeriodTotal[0] += ta_candlerange(BodyLong_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - first candle: long white (black) candlestick
    * - then: group of falling (rising) small real body candlesticks (commonly black (white)) that hold within
    *   the prior long candle's range: ideally they should be three but two or more than three are ok too
    * - final candle: long white (black) candle that opens above (below) the previous small candle's close
    *   and closes above (below) the first long candle's close
    * The meaning of "short" and "long" is specified with TA_SetCandleSettings; here only patterns with 3 small candles
    * are considered;
    * outInteger is positive (1 to 100) or negative (-1 to -100)
    */
   outIdx = 0;

   do
   {
      if( // white, 3 black, white  ||  black, 3 white, black
         ta_candlecolor(inClose[i-4], inOpen[i-4]) == -ta_candlecolor(inClose[i-3], inOpen[i-3]) &&
         ta_candlecolor(inClose[i-3], inOpen[i-3]) ==  ta_candlecolor(inClose[i-2], inOpen[i-2]) &&
         ta_candlecolor(inClose[i-2], inOpen[i-2]) ==  ta_candlecolor(inClose[i-1], inOpen[i-1]) &&
         ta_candlecolor(inClose[i-1], inOpen[i-1]) == -ta_candlecolor(inClose[i], inOpen[i]) &&
         // 2nd to 4th hold within 1st: a part of the real body must be within 1st range
         min(inOpen[i-3], inClose[i-3]) < inHigh[i-4] && max(inOpen[i-3], inClose[i-3]) > inLow[i-4] &&
         min(inOpen[i-2], inClose[i-2]) < inHigh[i-4] && max(inOpen[i-2], inClose[i-2]) > inLow[i-4] &&
         min(inOpen[i-1], inClose[i-1]) < inHigh[i-4] && max(inOpen[i-1], inClose[i-1]) > inLow[i-4] &&
         // 2nd to 4th are falling (rising)
         inClose[i-2] * ta_candlecolor(inClose[i-4], inOpen[i-4]) < inClose[i-3] * ta_candlecolor(inClose[i-4], inOpen[i-4]) &&
         inClose[i-1] * ta_candlecolor(inClose[i-4], inOpen[i-4]) < inClose[i-2] * ta_candlecolor(inClose[i-4], inOpen[i-4]) &&
         // 5th opens above (below) the prior close
         inOpen[i] * ta_candlecolor(inClose[i-4], inOpen[i-4]) > inClose[i-1] * ta_candlecolor(inClose[i-4], inOpen[i-4]) &&
         // 5th closes above (below) the 1st close
         inClose[i] * ta_candlecolor(inClose[i-4], inOpen[i-4]) > inClose[i-4] * ta_candlecolor(inClose[i-4], inOpen[i-4]) &&
         // 1st long, then 3 small, 5th long
         ta_realbody(inClose[i-4], inOpen[i-4]) > ta_candleaverage(BodyLong_rangeType, BodyLong_avgPeriod, BodyLong_factor, BodyPeriodTotal[4], inOpen[i-4], inHigh[i-4], inLow[i-4], inClose[i-4]) &&
         ta_realbody(inClose[i-3], inOpen[i-3]) < ta_candleaverage(BodyShort_rangeType, BodyShort_avgPeriod, BodyShort_factor, BodyPeriodTotal[3], inOpen[i-3], inHigh[i-3], inLow[i-3], inClose[i-3]) &&
         ta_realbody(inClose[i-2], inOpen[i-2]) < ta_candleaverage(BodyShort_rangeType, BodyShort_avgPeriod, BodyShort_factor, BodyPeriodTotal[2], inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) &&
         ta_realbody(inClose[i-1], inOpen[i-1]) < ta_candleaverage(BodyShort_rangeType, BodyShort_avgPeriod, BodyShort_factor, BodyPeriodTotal[1], inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) &&
         ta_realbody(inClose[i], inOpen[i])   > ta_candleaverage(BodyLong_rangeType, BodyLong_avgPeriod, BodyLong_factor, BodyPeriodTotal[0], inOpen[i], inHigh[i], inLow[i], inClose[i])
      ) {
         outInteger[outIdx++] = 100 * ta_candlecolor(inClose[i-4], inOpen[i-4]);
      }
      else {
         outInteger[outIdx++] = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      BodyPeriodTotal[4] += ta_candlerange(BodyLong_rangeType, inOpen[i-4], inHigh[i-4], inLow[i-4], inClose[i-4]) - ta_candlerange(BodyLong_rangeType, inOpen[BodyLongTrailingIdx-4], inHigh[BodyLongTrailingIdx-4], inLow[BodyLongTrailingIdx-4], inClose[BodyLongTrailingIdx-4]);
      for (totIdx = 3; totIdx >= 1; --totIdx)
         BodyPeriodTotal[totIdx] += ta_candlerange(BodyShort_rangeType, inOpen[i-totIdx], inHigh[i-totIdx], inLow[i-totIdx], inClose[i-totIdx])
      - ta_candlerange(BodyShort_rangeType, inOpen[BodyShortTrailingIdx-totIdx], inHigh[BodyShortTrailingIdx-totIdx], inLow[BodyShortTrailingIdx-totIdx], inClose[BodyShortTrailingIdx-totIdx]);
      BodyPeriodTotal[0] += ta_candlerange(BodyLong_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]) - ta_candlerange(BodyLong_rangeType, inOpen[BodyLongTrailingIdx], inHigh[BodyLongTrailingIdx], inLow[BodyLongTrailingIdx], inClose[BodyLongTrailingIdx]);

      i++;
      BodyShortTrailingIdx++;
      BodyLongTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
