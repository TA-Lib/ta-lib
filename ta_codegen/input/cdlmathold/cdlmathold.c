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
 *  022005 AC     Creation
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 *  103126 MF,CC  Nest the cheap, selective pre-check conjuncts in an outer if
 *  110526 MF,CC  Carry the ranges of the previous bars instead of recomputing them
 *
 */

int cdlmathold_lookback(double        optInPenetration)
{
   (void)optInPenetration;
   return max( BodyShort_avgPeriod, BodyLong_avgPeriod ) + 4;
}

TA_RetCode cdlmathold(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   double optInPenetration,
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double BodyPeriodTotal[5];
   double BodyPeriodTotalRange1, BodyPeriodTotalTrailRange1, BodyPeriodTotalRange2, BodyPeriodTotalTrailRange2, BodyPeriodTotalRange3, BodyPeriodTotalTrailRange3;
   int i, outIdx, BodyShortTrailingIdx, BodyLongTrailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdlmathold_lookback(optInPenetration);

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
      i++;
   }
   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - first candle: long white candle
    * - upside gap between the first and the second bodies
    * - second candle: small black candle
    * - third and fourth candles: falling small real body candlesticks (commonly black) that hold within the long
    *   white candle's body and are higher than the reaction days of the rising three methods
    * - fifth candle: white candle that opens above the previous small candle's close and closes higher than the
    *   high of the highest reaction day
    * The meaning of "short" and "long" is specified with TA_SetCandleSettings;
    * "hold within" means "a part of the real body must be within";
    * optInPenetration is the maximum percentage of the first white body the reaction days can penetrate (it is
    * to specify how much the reaction days should be "higher than the reaction days of the rising three methods")
    */
   outIdx = 0;
   BodyPeriodTotalRange2 = ta_candlerange(BodyShort_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]);
   BodyPeriodTotalTrailRange2 = ta_candlerange(BodyShort_rangeType, inOpen[BodyShortTrailingIdx-2], inHigh[BodyShortTrailingIdx-2], inLow[BodyShortTrailingIdx-2], inClose[BodyShortTrailingIdx-2]);
   BodyPeriodTotalRange3 = ta_candlerange(BodyShort_rangeType, inOpen[i-3], inHigh[i-3], inLow[i-3], inClose[i-3]);
   BodyPeriodTotalTrailRange3 = ta_candlerange(BodyShort_rangeType, inOpen[BodyShortTrailingIdx-3], inHigh[BodyShortTrailingIdx-3], inLow[BodyShortTrailingIdx-3], inClose[BodyShortTrailingIdx-3]);
   do
   {
      if( // white, black, 2 black or white, white
         ta_candlecolor(inClose[i-4], inOpen[i-4]) == 1 &&
         ta_candlecolor(inClose[i-3], inOpen[i-3]) == -1 &&
         ta_candlecolor(inClose[i], inOpen[i]) == 1 ) {
         if( // upside gap 1st to 2nd
            ta_realbodygapup(inOpen[i-3], inClose[i-3], inOpen[i-4], inClose[i-4]) &&
            // 3rd to 4th hold within 1st: a part of the real body must be within 1st real body
            min(inOpen[i-2], inClose[i-2]) < inClose[i-4] &&
            min(inOpen[i-1], inClose[i-1]) < inClose[i-4] &&
            // reaction days penetrate first body less than optInPenetration percent
            min(inOpen[i-2], inClose[i-2]) > inClose[i-4] - ta_realbody(inClose[i-4], inOpen[i-4]) * optInPenetration &&
            min(inOpen[i-1], inClose[i-1]) > inClose[i-4] - ta_realbody(inClose[i-4], inOpen[i-4]) * optInPenetration &&
            // 2nd to 4th are falling
            max(inClose[i-2], inOpen[i-2]) < inOpen[i-3] &&
            max(inClose[i-1], inOpen[i-1]) < max(inClose[i-2], inOpen[i-2]) &&
            // 5th opens above the prior close
            inOpen[i] > inClose[i-1] &&
            // 5th closes above the highest high of the reaction days
            inClose[i] > max(max(inHigh[i-3], inHigh[i-2]), inHigh[i-1]) &&
            // 1st long, then 3 small
            ta_realbody(inClose[i-4], inOpen[i-4]) > ta_candleaverage(BodyLong_rangeType, BodyLong_avgPeriod, BodyLong_factor, BodyPeriodTotal[4], inOpen[i-4], inHigh[i-4], inLow[i-4], inClose[i-4]) &&
            ta_realbody(inClose[i-3], inOpen[i-3]) < ta_candleaverage(BodyShort_rangeType, BodyShort_avgPeriod, BodyShort_factor, BodyPeriodTotal[3], inOpen[i-3], inHigh[i-3], inLow[i-3], inClose[i-3]) &&
            ta_realbody(inClose[i-2], inOpen[i-2]) < ta_candleaverage(BodyShort_rangeType, BodyShort_avgPeriod, BodyShort_factor, BodyPeriodTotal[2], inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) &&
            ta_realbody(inClose[i-1], inOpen[i-1]) < ta_candleaverage(BodyShort_rangeType, BodyShort_avgPeriod, BodyShort_factor, BodyPeriodTotal[1], inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1])
         ) {
            outInteger[outIdx++] = 100;
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
      BodyPeriodTotal[4] += ta_candlerange(BodyLong_rangeType, inOpen[i-4], inHigh[i-4], inLow[i-4], inClose[i-4]) - ta_candlerange(BodyLong_rangeType, inOpen[BodyLongTrailingIdx-4], inHigh[BodyLongTrailingIdx-4], inLow[BodyLongTrailingIdx-4], inClose[BodyLongTrailingIdx-4]);
      BodyPeriodTotalRange1 = ta_candlerange(BodyShort_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]);
      BodyPeriodTotalTrailRange1 = ta_candlerange(BodyShort_rangeType, inOpen[BodyShortTrailingIdx-1], inHigh[BodyShortTrailingIdx-1], inLow[BodyShortTrailingIdx-1], inClose[BodyShortTrailingIdx-1]);
      BodyPeriodTotal[3] += BodyPeriodTotalRange3 - BodyPeriodTotalTrailRange3;
      BodyPeriodTotal[2] += BodyPeriodTotalRange2 - BodyPeriodTotalTrailRange2;
      BodyPeriodTotal[1] += BodyPeriodTotalRange1 - BodyPeriodTotalTrailRange1;
      BodyPeriodTotalRange3 = BodyPeriodTotalRange2;
      BodyPeriodTotalTrailRange3 = BodyPeriodTotalTrailRange2;
      BodyPeriodTotalRange2 = BodyPeriodTotalRange1;
      BodyPeriodTotalTrailRange2 = BodyPeriodTotalTrailRange1;
      i++;
      BodyShortTrailingIdx++;
      BodyLongTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
