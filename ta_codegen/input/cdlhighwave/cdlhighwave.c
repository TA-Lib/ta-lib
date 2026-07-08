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
 *  072404 AC   Creation
 *
 */

int cdlhighwave_lookback(void)
{
   return max( BodyShort_avgPeriod, ShadowVeryLong_avgPeriod );
}

TA_RetCode cdlhighwave(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double BodyPeriodTotal, ShadowPeriodTotal;
   int i, outIdx, BodyTrailingIdx, ShadowTrailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdlhighwave_lookback();

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
   BodyPeriodTotal = 0;
   BodyTrailingIdx = startIdx - BodyShort_avgPeriod;
   ShadowPeriodTotal = 0;
   ShadowTrailingIdx = startIdx - ShadowVeryLong_avgPeriod;

   i = BodyTrailingIdx;
   while( i < startIdx ) {
      BodyPeriodTotal += ta_candlerange(BodyShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = ShadowTrailingIdx;
   while( i < startIdx ) {
      ShadowPeriodTotal += ta_candlerange(ShadowVeryLong_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - short real body
    * - very long upper and lower shadow
    * The meaning of "short" and "very long" is specified with TA_SetCandleSettings
    * outInteger is positive (1 to 100) when white or negative (-1 to -100) when black;
    * it does not mean bullish or bearish
    */
   outIdx = 0;
   do
   {
      if( ta_realbody(inClose[i], inOpen[i]) < ta_candleaverage(BodyShort_rangeType, BodyShort_avgPeriod, BodyShort_factor, BodyPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i]) &&
         ta_uppershadow(inHigh[i], inClose[i], inOpen[i]) > ta_candleaverage(ShadowVeryLong_rangeType, ShadowVeryLong_avgPeriod, ShadowVeryLong_factor, ShadowPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i]) &&
         ta_lowershadow(inLow[i], inClose[i], inOpen[i]) > ta_candleaverage(ShadowVeryLong_rangeType, ShadowVeryLong_avgPeriod, ShadowVeryLong_factor, ShadowPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i]) ) {
         outInteger[outIdx++] = ta_candlecolor(inClose[i], inOpen[i]) * 100;
      }
      else {
         outInteger[outIdx++] = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      BodyPeriodTotal += ta_candlerange(BodyShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]) - ta_candlerange(BodyShort_rangeType, inOpen[BodyTrailingIdx], inHigh[BodyTrailingIdx], inLow[BodyTrailingIdx], inClose[BodyTrailingIdx]);
      ShadowPeriodTotal += ta_candlerange(ShadowVeryLong_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]) - ta_candlerange(ShadowVeryLong_rangeType, inOpen[ShadowTrailingIdx], inHigh[ShadowTrailingIdx], inLow[ShadowTrailingIdx], inClose[ShadowTrailingIdx]);
      i++;
      BodyTrailingIdx++;
      ShadowTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
