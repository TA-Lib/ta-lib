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
 *  071704 AC   Creation
 *
 */

int cdllongline_lookback(void)
{
   return max( BodyLong_avgPeriod, ShadowShort_avgPeriod );
}

TA_RetCode cdllongline(int startIdx, int endIdx,
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

   lookbackTotal = cdllongline_lookback();

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
   BodyTrailingIdx = startIdx - BodyLong_avgPeriod;
   ShadowPeriodTotal = 0;
   ShadowTrailingIdx = startIdx - ShadowShort_avgPeriod;

   i = BodyTrailingIdx;
   while( i < startIdx ) {
      BodyPeriodTotal += ta_candlerange(BodyLong_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = ShadowTrailingIdx;
   while( i < startIdx ) {
      ShadowPeriodTotal += ta_candlerange(ShadowShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - long real body
    * - short upper and lower shadow
    * The meaning of "long" and "short" is specified with TA_SetCandleSettings
    * outInteger is positive (1 to 100) when white (bullish), negative (-1 to -100) when black (bearish)
    */
   outIdx = 0;

   do
   {
      if( ta_realbody(inClose[i], inOpen[i]) > ta_candleaverage(BodyLong_rangeType, BodyLong_avgPeriod, BodyLong_factor, BodyPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i]) &&
         ta_uppershadow(inHigh[i], inClose[i], inOpen[i]) < ta_candleaverage(ShadowShort_rangeType, ShadowShort_avgPeriod, ShadowShort_factor, ShadowPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i]) &&
         ta_lowershadow(inLow[i], inClose[i], inOpen[i]) < ta_candleaverage(ShadowShort_rangeType, ShadowShort_avgPeriod, ShadowShort_factor, ShadowPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i]) ) {
         outInteger[outIdx++] = ta_candlecolor(inClose[i], inOpen[i]) * 100;
      }
      else {
         outInteger[outIdx++] = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      BodyPeriodTotal += ta_candlerange(BodyLong_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]) - ta_candlerange(BodyLong_rangeType, inOpen[BodyTrailingIdx], inHigh[BodyTrailingIdx], inLow[BodyTrailingIdx], inClose[BodyTrailingIdx]);
      ShadowPeriodTotal += ta_candlerange(ShadowShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]) - ta_candlerange(ShadowShort_rangeType, inOpen[ShadowTrailingIdx], inHigh[ShadowTrailingIdx], inLow[ShadowTrailingIdx], inClose[ShadowTrailingIdx]);
      i++;
      BodyTrailingIdx++;
      ShadowTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
